#include "TrackingManager.h"
#include <string>
#include <iostream>
//#include "HungarianAlg.h"

using namespace std;
/* 
  The hierarchical association algorithm for multi-target tracking is implemented here.
  We try to follow the paper:
	Chang Huang, Bo Wu, and Ramakant Nevatia. 
	"Robust object tracking by hierarchical association of detection responses." 
	ECCV 2008. Springer Berlin Heidelberg, 2008. 788-801.
*/

void TrackingManager::RunTracking(const Mat& currFrame, double currTimeStamp)
{
	for (size_t i = 0; i < m_tracklets.size(); i++) {
		Tracklet *tracklet = m_tracklets[i];
		Rect trackedBB;
		bool isSuccess = false;
		isSuccess = tracklet->VisualTracking(currFrame, trackedBB, currTimeStamp);
	}
	m_prvFrame = currFrame.clone();
}

void TrackingManager::UpdateTrackers(const Mat& currFrame, double timeStamp)
{
	for (size_t i = 0; i < m_tracklets.size(); i++) {
		if (!(m_tracklets[i]->objects.back().isFromTracker)) 
			m_tracklets[i]->InitTracker(currFrame, m_tracklets[i]->objects.back().bb, timeStamp);
	}
}

/*
 * Although motion detection has alread done a correction, it has not work in tracklet level. 
 * Here it works on tracklet level which means tracking result will also work.
 * The work flow is like this:
 * - predict object postion for each tracklet
 * - check whether the predicted objects overlap with merged region (low nPixels/rect.area)
 */
void TrackingManager::MotionCorrection(vector<Target>& objects, double prvTimeStamp, double currTimeStamp)
{

	// Predict where previous objects will appear
	vector<Target> prdctObjects;
	double dt = currTimeStamp - prvTimeStamp;
	for (size_t i = 0; i < m_tracklets.size(); i++) {
		if (!m_tracklets[i]->IsEffective()) continue;
		Target target = m_tracklets[i]->objects.back();
		target.bb.x += dt*target.vx;
		target.bb.y += dt*target.vy;
		target.timeStamp = currTimeStamp;
		prdctObjects.push_back(target);
	}

	// Split objects
	vector<Target> newObjects;
	for (size_t i = 0; i < objects.size(); i++) {
		double ratio = objects[i].area / (objects[i].bb.area() + 0.0);
		//cout << "npixels/rect.area " << ratio << endl;

		// If the percentage of foreground pixels is too small, it might be due to merge of more than one objects.
		double ratioThreshold = 0.6;
		double overlapThreshold = 0.6;
		bool isOverlapped = false;
		if (ratio < ratioThreshold) {
			for (size_t j = 0; j < prdctObjects.size(); j++) {
				if ((objects[i].bb & prdctObjects[j].bb).area() / (prdctObjects[j].bb.area()+0.0) > overlapThreshold) {
					newObjects.push_back(prdctObjects[j]);
					if (!isOverlapped) isOverlapped = true;
				}
			}
		}
		if (isOverlapped) {
			objects.erase(objects.begin() + i);
			--i;
		}
	}

	objects.reserve(objects.size() + newObjects.size());
	objects.insert(objects.end(), newObjects.begin(), newObjects.end());
}

void TrackingManager::LowLevelAssociation(const Mat& currFrame, vector<Target>& objects, double prvTimeStamp)
{
	/*
		Check whether the object can be linked to a tracklet. Refer to 2.1 of the paper for more details. 
		Two steps:
		- Whether the object is similar to the last one of a tracklet.
		- Whether it's much more similar to the tracklet the rest tracklets
		If none satisfied, create a new tracklet with the input detection.
	 */	
	vector<vector<double>> linkScores(objects.size(), vector<double>(m_tracklets.size(), -1000));;
	vector<vector<bool>> linkFlags(objects.size(), vector<bool>(m_tracklets.size(), false));
		
	/* 
	 * Step 1: whether object can be linked to tracklet
	 */
	for (int i = 0; i < objects.size(); i++) {		
		Target& object = objects[i];
		for (int j = 0; j < m_tracklets.size(); j++) {			
			Tracklet *tracklet = m_tracklets[j];
			if (tracklet->IsLinkable(object, prvTimeStamp, &linkScores[i][j], 0.1)) {
				linkFlags[i][j] = true;
			}
		}
	}
	/* 
	 * Step 2: for each link, check whether it's better than all other possible ones
	 */
	int nOrigTracklets = m_tracklets.size();
	double theta2 = 0.0; //Appeared in the paper
	for (int i = 0; i < objects.size(); i++) {
		bool isLinked = false;
		for (int j = 0; j < nOrigTracklets; j++) {
			if (!linkFlags[i][j]) continue;
			// object-wise
			bool better = true;
			for (int ii = 0; ii < objects.size(); ii++) {
				if (ii == i) continue;
				if (linkScores[i][j] - linkScores[ii][j] < theta2) {
					better = false;
					break;
				}
			}
			if (!better) {
				continue;
			}
			// tracklet-wise
			for (int jj = 0; jj <nOrigTracklets; jj++) {
				if (jj == j) continue;
				if (linkScores[i][j] - linkScores[i][jj] < theta2) {
					better = false;
					break;
				}
			}
			if (better) {
				m_tracklets[j]->Link(objects[i]);
				isLinked = true;
				//cout << "linked, score is " << linkScores[i][j] << endl;
			}
		}
		if (!isLinked) CreateTracklet(objects[i], currFrame);
	}	
}

void TrackingManager::CorrectLowLevelAssociation(const Mat& currFrame, const Mat& fgMap, double prvTimeStamp, double currTimeStamp)
{
	for (size_t i = 0; i < m_tracklets.size(); i++) {
		const Target& lastObject = m_tracklets[i]->objects.back();
		double interval = abs(lastObject.timeStamp - prvTimeStamp);
		double duration = lastObject.timeStamp - m_tracklets[i]->objects.front().timeStamp;
		if (!m_tracklets[i]->IsEffective()) continue;
		if (interval < 1e-6) { // The last object is from last frame which means low level association failed to append a new object
			// Do tracking
			Rect trackedBB;
			bool isSuccessful = m_tracklets[i]->VisualTracking(currFrame, trackedBB, currTimeStamp);
			vector<float> dlbpHist;
			ComputeDLBP(currFrame(trackedBB), dlbpHist);
			float dis = 0;

			// Calculate degenration of tracking result
			// If the tracking result degenerates too fast, it means the tracked object should be background then tracking should be terminated
			// How to determine the degeneration:
			// 1. get foreground (based on background subtraction)
			// 2. calculate the ratio of foreground pixels
			// 3. The idea is if object vanishes, the degeneration will be faster. Although foreground will be also learnt as background, but the speed is much lower. 
			int nNonzeros = 0;
			Mat roi = fgMap(trackedBB);
			for (int r = 0; r < roi.rows; r++) {
				for (int c = 0; c < roi.cols; c++) {
					if (roi.at<uchar>(r,c) != 0)
						++nNonzeros;
				}
			}
			float ratio = 1.0*nNonzeros/roi.total();
			cout << "foreground ratio" << ratio << endl;

			float degRatioThre = 0.2;
			float degTimeThre = 2*SECOND;
			if (ratio < degRatioThre && currTimeStamp - m_tracklets[i]->m_stTimeStamp < degTimeThre) {
				m_tracklets[i]->m_hasDenerated = true;
				cout << "The tracking has degenerated!" << endl;
			}


			for (size_t n = 0; n < dlbpHist.size(); n++) {
				float d = dlbpHist[n] - m_tracklets[i]->m_dlbp[n];
				dis += d*d;
			}
			dis = sqrt(dis);
			cout << "DLBP distance " << dis << endl;
			
			if (dis > 5) isSuccessful = false;
			if (isSuccessful) {
				Target trackedObj = lastObject;
				trackedObj.timeStamp = currTimeStamp;
				trackedObj.isFromTracker = true;
				trackedObj.bb = trackedBB;
				m_tracklets[i]->Link(trackedObj);
			}
		}
	}	
}


void TrackingManager::SimpleMidLevelAssociation(const Mat& frame, double prvTimeStamp, double currTimeStamp, double vThreshold) 
{
	vector<int> combinedIndex;
	// Combine tracklets

	// Remove the old and invalid tracklets
	int thresholdHistory = 2*SECOND;

	for (int i = 0; i < (int)m_tracklets.size(); i++)	{
		//cout << "Average vx, vy for tracklet " << i << " " << m_tracklets[i].GetAveSpeedX();
		//cout << " " << m_tracklets[i].GetAveSpeedY() << endl;	


		// Remove the ineffective trakclets ends in last frame
		if (!m_tracklets[i]->IsEffective() && abs(prvTimeStamp - m_tracklets[i]->EndTime())< 1e-6) {
			delete m_tracklets[i];
			m_tracklets.erase(m_tracklets.begin() + i);
				//cout << "after " << m_tracklets.size() <<endl;
			i--;
			continue;
		}

		double vanishTime = currTimeStamp - m_tracklets[i]->EndTime();


		// Increase number
		if (vanishTime > vThreshold*SECOND ||  m_tracklets[i]->m_hasDenerated) {
			/*if (m_tracklets[i].IsCrossingLineSegment(g_point1, g_point2)) {
				cout << "*****Crossing!*****" << endl;
				
			}*/		
			Point2f first, last;
			first = GetRectCenter(m_tracklets[i]->objects.front().bb);
			last = GetRectCenter(m_tracklets[i]->objects.back().bb);
			first.x /= frame.cols;			first.y /= frame.rows;
			last.x /= frame.cols;			last.y /= frame.rows;
			
			for(size_t j=0;j<g_cntios.size();j++) {
				if(g_cntios[j].p1contains(first) &&
					g_cntios[j].p2contains(last))
				{
					g_cntios[j].d1++;
				}

				if(g_cntios[j].p2contains(first) && 
					g_cntios[j].p1contains(last) )
				{
					g_cntios[j].d2++;
				}
			}

			++m_nTargets;
			delete m_tracklets[i];
			m_tracklets.erase(m_tracklets.begin() + i);
			//cout << "after " << m_tracklets.size() <<endl;
			i--;
			
		}
	}	

	// Combine tracklets
	for (int i = 0; i < (int)m_tracklets.size()-1; i++)	{
		if (!m_tracklets[i]->IsEffective()) continue;
		Tracklet *ti = m_tracklets[i];
		for (int j = i+1; j < (int)m_tracklets.size(); j++)	{
			if (!m_tracklets[j]->IsEffective()) continue;
			Tracklet *tj = m_tracklets[j];
			double affinity = ti->DisAffinity(*tj)* ti->AppearanceAffinity(*tj)* ti->TemporalAffinity(*tj, 0);
			cout << "Affinity " << affinity << endl;
			if (affinity > 0.2) {
				cout << "appreance sim " << ti->AppearanceAffinity(*tj) << endl;
				if (ti->Append(*tj)) combinedIndex.push_back(j);
				if (tj->Append(*ti)) combinedIndex.push_back(i);					
			}
		}
	}

	if (combinedIndex.size() > 0) {
		// Remove the tracklets being combined
		for (int i = 0; i < (int)m_tracklets.size(); i++)	{
			bool combined = false;
			for (int j = 0; j < combinedIndex.size(); j++) {
				if (i == combinedIndex[j]) {
					combined = true;
					break;
				}
			}
			//if (!combined) tmpAssociation.push_back(m_tracklets[i]);
			if (combined) {
				delete m_tracklets[i];
				m_tracklets.erase(m_tracklets.begin() + i);
				i--;
			}

		}
		
	}

}

// To show all the tracklets
void TrackingManager::Visualize(Mat& displayImage)
{
	static bool visualization = false;
	if (!visualization) {
		GenerateColorTable();
		visualization = true;
	}

	std::stringstream ss;

	int thickness = 2;
	// Draw bb of each tracklet
	for (int i = 0; i < m_tracklets.size(); i++) {
		Tracklet *tracklet = m_tracklets[i];	
		Scalar color = m_colortable[(m_nTargets+i)%1000];
		if (tracklet->objects.size() > 1) {
			for (int j = 0; j < tracklet->objects.size() - 1; j++) {
		        Point start = GetRectCenter(tracklet->objects[j].bb);
				Point end = GetRectCenter(tracklet->objects[j+1].bb);				
				//rectangle(displayImage, m_tracklets[i].objects[j].bb.tl(), m_tracklets[i].objects[j].bb.br(), color, 2);
				line( displayImage, start, end, color, thickness, 8);
			}
		}

		//Draw tracking results
		if (tracklet->objects.size() > m_nFramesForInitTracking) {
			for (int i = 0; i < tracklet->objects.size(); i++) {
				cv::rectangle(displayImage, tracklet->m_trackerBB, CV_RGB(255, 0, 255), 2);
			}
		}
	}
	

	//ss << "total " << m_nTargets;

	for(size_t j=0;j<g_cntios.size();j++) {
		ss << g_cntios[j].d2 << ", " << g_cntios[j].d1 << "; ";
	}

	string text = ss.str();
	int fontFace = FONT_HERSHEY_SIMPLEX;
	double fontScale = 0.5;
	
	int baseline=0;
	Size textSize = getTextSize(text, fontFace,
                            fontScale, 2, &baseline);
	Point textOrg(0, textSize.height);	
	putText(displayImage, text, textOrg, fontFace, fontScale,
        CV_RGB(255,0,0), thickness, 8);// then put the text itself


}