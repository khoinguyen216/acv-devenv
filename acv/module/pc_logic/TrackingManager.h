#ifndef __ASSOCIATION_WANGFANGLIN__
#define __ASSOCIATION_WANGFANGLIN__
#include "opencv2/opencv.hpp"
#include "tracklet.h"
#include "target.h"
#include "opencv/cv.h"
#include "utilities.h"
#include "DLBPUtilities.h"


using namespace cv;

class TrackingManager
{
public:
	TrackingManager():m_nTargets(0)
	{
		m_tracklets.clear();
		m_nFramesForInitTracking = 5;
	}
private:
	
public:
	/*
	 * Two adjacent objects should be temporal connectable, i.e., from concecutive frames
	 *
	 *	@prvTimeStamp, to verify whether the last object of this tracklet is from last frame
	 */
	void LowLevelAssociation(const Mat& currFrame, vector<Target>& objects, double prvTimeStamp);
	//Statistics to get result
	/*
	 * A very very simple middle level association
	 * Just do:
	 * 1. Try to combine two separate tracklets
	 */
	void SimpleMidLevelAssociation(const Mat& frame, double prvTimeStamp, double currTimeStamp, double vt);

	// Create a new tracklet with new detection when it cannot be linked to any tracklet.
	void CreateTracklet(Target& object, const Mat& frame)
	{
		Tracklet *newTracklet = new Tracklet;
		newTracklet->Create(object, frame);
		m_tracklets.push_back(newTracklet);			
	}
	/*
	 * Update trackers. 
	 * The principle is: if current object is from detection, then update the tracker, else if it's from tracking, then do not update.
	 */
	void UpdateTrackers(const Mat& currFrame, double timeStamp);

	void CorrectLowLevelAssociation(const Mat& currFrame, const Mat& foregroundMap, double prvTimeStamp, double currTimeStamp);
	// To show all the tracklets
	void Visualize(Mat& displayImage);

	void RunTracking(const Mat& currFrame, double currTimeStamp);

	void UpdateBackgroundImage(const Mat& currFrame, int nObjects)
	{
		if (nObjects == 0 && m_tracklets.size() == 0)
			g_backgroundImage = currFrame.clone();
	}

	/*
	 *	Correct objects from motion detection
	 *
	 *  The basic idea: if 
	 */
	void MotionCorrection(vector<Target>& objects, double prvTimeStamp, double currTimeStamp);

private:
	CvScalar random_color(CvRNG* rng)
	{
		int color = cvRandInt(rng);
		return CV_RGB(color&255, (color>>8)&255, (color>>16)&255);
	}
	void GenerateColorTable()
	{
		CvRNG rng(cv::getTickCount());

		for (int i = 0; i < 1000; i++) {
			int color = cvRandInt(&rng);
			m_colortable[i] = CV_RGB(color&255, (color>>8)&255, (color>>16)&255);
		}
	}

public:
	vector<Tracklet*>	m_tracklets;
	int					m_nTargets;
private:
	Scalar				m_colortable[1000];	
	int					m_nFramesForInitTracking;

	Mat					m_prvFrame;
};
#endif