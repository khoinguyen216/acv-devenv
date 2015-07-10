#include "MotionDetection.h"
#include <boost/bind.hpp>
#include "utilities.h"

namespace BoatDetection{

void MotionDetector::CheckRangeOfMat(Mat mat)
{
	//----------------------------------------------------------
	//To check the range of mat
	double minVal;
	double maxVal;
	Point minLoc;
	Point maxLoc;

	cv::minMaxLoc( mat, &minVal, &maxVal, &minLoc, &maxLoc );

	std::cout << "min val : " << minVal << endl;
	std::cout << "max val: " << maxVal << endl;
	// The end of checking the range of mat
	//-----------------------------------------------------------
}

// A dummy in order to use boost::bind
vector<Mat> prvParts, nextParts, flowParts;

void MotionDetector::calcOpticalFlowFarneback_dummy(int i, double pyr_scale, int levels, int winsize,
                           int iterations, int poly_n, double poly_sigma, int flags)
{
	calcOpticalFlowFarneback(prvParts[i], nextParts[i], flowParts[i], 0.5, 3, 15, 3, 5, 1.2, 0);
}

/*
 *	NOTE:
 */
void MotionDetector::CalcDenseOpticalFlow(const Mat& prvFrame, const Mat& currFrame,  optFlow& oflow, bool showResult)
{
	float gKernelWidth = 3;
	Mat smallFrame, prvs, next;
	cvtColor(prvFrame, prvs, COLOR_BGR2GRAY);
	//medianBlur (prvs, prvs, 5);
	GaussianBlur(prvs, prvs, Size(gKernelWidth, gKernelWidth), 0, 0);

	cvtColor(currFrame, next, COLOR_BGR2GRAY);
	//medianBlur (prvs, prvs, 5);
	GaussianBlur(next, next, Size(gKernelWidth, gKernelWidth), 0, 0);

	Mat flow;
	vector<Mat> channels(2);

	double pyr_scale = 0.7; //parameter, specifying the image scale (<1) to build pyramids for each image; pyr_scale=0.5 means a classical pyramid, where each next layer is twice smaller than the previous one.
	int nlevels = 1; // number of pyramid layers including the initial image; levels=1 means that no extra layers are created and only the original images are used.
	int winsize = 15;
	int niterations = 5; // number of iterations the algorithm does at each pyramid level.
	int poly_n = 5; //size of the pixel neighborhood used to find polynomial expansion in each pixel; larger values mean that the image will be approximated with smoother surfaces, yielding more robust algorithm and more blurred motion field, typically poly_n =5 or 7.
	double poly_sigma = 1.2; //standard deviation of the Gaussian that is used to smooth derivatives used as a basis for the polynomial expansion; for poly_n=5, you can set poly_sigma=1.1, for poly_n=7, a good value would be poly_sigma=1.5.
	//calcOpticalFlowFarneback(prvs, next, flow, pyr_scale, nlevels, winsize, niterations, poly_n, poly_sigma, OPTFLOW_FARNEBACK_GAUSSIAN);
	//GaussianBlur(flow, flow, Size(gKernelWidth, gKernelWidth), 0, 0);
	//----The originally used setting
	calcOpticalFlowFarneback(prvs, next, flow, 0.5, 3, 5, 5, 5, 1.2, 0);
	
	// Fanglin: this method doesn't work
	//calcOpticalFlowSF(prvFrame.clone(), currFrame.clone(), flow, 3, 2, 4);
	cv::GaussianBlur(flow, flow,  Size(gKernelWidth, gKernelWidth), 0, 0);

	split(flow, channels);


	cartToPolar(channels[0], channels[1], oflow.mag, oflow.ang);
	oflow.xflow = channels[0].clone();
	oflow.yflow = channels[1].clone();

	// The range is [0, 360]
	oflow.ang = oflow.ang * 360/M_PI/2.0;

	if (showResult) {
		Mat vmag, vang; // The image with different range
		normalize(oflow.mag, vmag, 255, 0, NORM_MINMAX);
		vang = oflow.ang/2.0;	// Hue range is [0, 180] for hsv space
		Mat umag, uang;	// The image for visualization
		vmag.convertTo(umag, CV_8UC1);
		vang.convertTo(uang, CV_8UC1);
		Mat h, s, v;
		s.create(oflow.mag.size(), CV_8UC1);
		s.setTo(Scalar(255));

		vector<Mat> chs;
		Mat hsv;
		h = uang;
		v = umag;
		chs.push_back(h);
		chs.push_back(s);
		chs.push_back(v);
		merge(chs, hsv);

		cvtColor(hsv, m_vFlow, COLOR_HSV2BGR);
	}
}

/*
 * Compute the diffusion of a set of angles.
 *
 * NOTE: the angle range is [0, 360]
 */
float MotionDetector::ComputeAngleDiffusion(double *val, int size)
{
	double maxDiff = 0.0;
	double result = 0;
	for (int i = 0; i < size - 1; i++) {
		for (int j = i+1; j < size; j++) {
			double ad = DIFF_ANGLE(val[i], val[j]);
			if (ad > maxDiff)
				maxDiff = ad;
			//result += ad;
		}
	}
	//result /= size*(size-1)/2.0;
	result = maxDiff;
	return result;
}

/*
 * Compute the diffusion of a set of magnitude values
 *
 * NOTE: the angle range is [0, 360]
 */
double MotionDetector::ComputeMagDiffusion(double *val, int size)
{
	double maxDiff = 0.0;
	double result = 0;
	for (int i = 0; i < size - 1; i++) {
		for (int j = i+1; j < size; j++) {
			double ad = abs(val[i] - val[j]);
			if (ad > maxDiff)
				maxDiff = ad;
			//result += ad;
		}
	}
	//result /= size*(size-1)/2.0;
	result = maxDiff;
	return result;
}

/*
 * Determine whether a pixel belongs to foreground based on its optical flow history.
 * If this pixel changes its speed value or direction too much then it cannot be the foreground.
 */
void MotionDetector::AnalyzeFlowHistory(const list<optFlow>& fp, Mat& mask, const Rect& roi, bool saveSamples)
{
	//const optFlow& currOptFlow = fp[idxCurrentFrame % nHistoFlows];
	const optFlow& currOptFlow = fp.back();
	int rows = currOptFlow.mag.rows;
	int cols = currOptFlow.mag.cols;

	mask.create(currOptFlow.mag.size(), CV_32FC1);
	mask.setTo(Scalar(1));
	Mat magVarMat, angVarMat;
	magVarMat = Mat::zeros(currOptFlow.mag.size(), CV_32FC1);
	angVarMat = Mat::zeros(currOptFlow.mag.size(), CV_32FC1);

	double *val = new double[fp.size()];

	// Fanglin (2014-09-22): This snippet was deprecated. Now we choose to analyze pixel value by pixel tracking as shown below.
	// Get angle and magnititude variance
	//for (int i = 0; i < currOptFlow.mag.rows; i++) {
	//	for (int j = 0; j < currOptFlow.mag.cols; j++) {
	//		// Get historical angle values
	//		for (list<optFlow>::const_iterator it = fp.begin(); it != fp.end(); ++it) {
	//			val[std::distance(fp.begin(), it)] = (*it).ang.at<float>(i, j);
	//		}
	//		double var_ang = ComputeAngleDiffusion(val, fp.size());
	//		angVarMat.at<float>(i, j) = var_ang;

	//		// Get historical magnitude values
	//		for (list<optFlow>::const_iterator it = fp.begin(); it != fp.end(); ++it) {
	//			val[std::distance(fp.begin(), it)] = (*it).mag.at<float>(i, j);

	//		}
	//		//Statistics st(val, oh.size());
	//		//double var_mag = st.getVariance();
	//		/*double var_mag = ComputeMagDiffusion(val, fp.size());
	//		magVarMat.at<float>(i, j) = var_mag;*/
	//	}
	//}

	// Get angle and magnititude variance
	// We use a backwards tracking method, each pixel will go back to the previous frames to find its status
	int x, y, width, height;
	if (roi == Rect()) {
		x = 0; y = 0; width = currOptFlow.mag.cols; height = currOptFlow.mag.rows;
	} else {
		x = roi.x; y = roi.y; width = roi.width; height = roi.height;
	}

	for (int i = y; i < y + height; i++) {
		for (int j = x; j < x + width; j++) {
			// Get historical angle values
			int bi = i, bj = j;
			double xdis = 0.0, ydis = 0.0;
			for (list<optFlow>::const_reverse_iterator it = fp.rbegin(); it != fp.rend(); ++it) {
				bi = cvRound(i - ydis);
				bj = cvRound(j - xdis);
				if (bi > rows - 1) bi = rows - 1;
				if (bi < 0) bi = 0;
				if (bj > cols - 1) bj = cols - 1;
				if (bj < 0) bj = 0;
				val[std::distance(fp.rbegin(), it)] = (*it).ang.at<float>(bi, bj);
				xdis += (*it).xflow.at<float>(bi, bj);
				ydis += (*it).yflow.at<float>(bi, bj);
			}
			double var_ang = ComputeAngleDiffusion(val, fp.size());
			angVarMat.at<float>(i, j) = var_ang;

			// Get historical magnitude values
			/*for (list<optFlow>::const_reverse_iterator it = fp.rbegin(); it != fp.rend(); ++it) {
				val[std::distance(fp.rbegin(), it)] = (*it).mag.at<float>(i, j);

			}*/
			//Statistics st(val, oh.size());
			//double var_mag = st.getVariance();
			/*double var_mag = ComputeMagDiffusion(val, fp.size());
			magVarMat.at<float>(i, j) = var_mag;*/
		}
	}

	// Suppress the pixels with large variance
	for (int i = y; i < y + height; i++) {
		for (int j = x; j < x + width; j++) {
			double valA = angVarMat.at<float>(i, j);
			//double valM = magVarMat.at<float>(i, j);
			if (valA < this->m_angleHistoDiffDegThre /*&& valM < 1.5*/)
				mask.at<float>(i, j) = 1;
			else
				mask.at<float>(i, j) = 0;
		}
	}


	// TODO: This snippet may be moved upwards to increase the efficiency. Because for most of the pixels we don't need to compuate their variance.
	for (int i = y; i < y + height; i++) {
		for (int j = x; j < x + width; j++) {
			double angle = currOptFlow.ang.at<float>(i, j);
			double magi = currOptFlow.mag.at<float>(i, j);

			/*if (mask.at<float>(i,j) < angleHistoryDiffusionThre)
				mask.at<float>(i,j) = 0;
			else
				mask.at<float>(i,j) = 1;*/

			/*if ((angle > angleThreshold && angle < 180-angleThreshold) || (angle > 180+angleThreshold && angle < 360-angleThreshold))
				mask.at<float>(i, j) = 0;*/

			if (!(DIFF_ANGLE(angle,m_angle) < m_angleThreshold || DIFF_ANGLE(angle, m_angle+180) < m_angleThreshold))
				mask.at<float>(i, j) = 0;

			if (magi < m_speedThreshold)
				mask.at<float>(i, j) = 0;
		}
	}

	if (saveSamples) {
		FILE *pf = fopen("output/p1.txt", "a");

		for (int i = 0; i < currOptFlow.mag.rows; i++) {
			for (int j = 0; j < currOptFlow.mag.cols; j++) {
				if (! (mask.at<float>(i,j) > 0)) continue;

				fprintf(pf, "+1 ");
				for (list<optFlow>::const_iterator it = fp.begin(); it != fp.end(); ++it) {
					val[std::distance(fp.begin(), it)] = (*it).xflow.at<float>(i, j)/10.0;
					fprintf(pf, "%d:%.8f ", std::distance(fp.begin(), it)+1, val[std::distance(fp.begin(), it)]);
				}

				// Get historical magnitude values
				for (list<optFlow>::const_iterator it = fp.begin(); it != fp.end(); ++it) {
					val[std::distance(fp.begin(), it)] = (*it).yflow.at<float>(i, j)/10.0;
					fprintf(pf, "%d:%.8f ", std::distance(fp.begin(), it)+fp.size()+1, val[std::distance(fp.begin(), it)]);
				}
				fprintf(pf, "\n");
			}
		}
		fclose(pf);
	}

	delete []val;
}

void MotionDetector::GetObjects(Mat& mask, const Mat& frame, const optFlow& of, double prvTimeStamp, double currTimeStamp, vector<Target>& objects)
{
	vector<Rect> rois;
	removeSmallRegions((float*)(mask.data), rois, mask.cols, mask.rows, this->m_pixelThreshold);
	//objects.resize(rois.size());


	// Get the bounding boxes, displacements at x, y directions, frameID for detection result, in order for tracking association
	for (int i = 0; i < rois.size(); i++) {

		// The aspect ratio should not be too big
		double aspectRatio = (rois[i].height + 0.0)/rois[i].width;
		/*if (aspectRatio > 0.5)
			continue;*/

		//objects[i].bb = rois[i];
		// Compute the average velocity and direction



		//vector<double> vx, vy;
		//vx.resize(npixels); vy.resize(npixels);
		//npixels = 0;

		//for (int y = rois[i].y; y < rois[i].br().y; y++) {
		//	for (int x = rois[i].x; x < rois[i].br().x; x++) {
		//		if (mask.at<float>(y, x) > 0) {
		//			vx[npixels] = (of.xflow.at<float>(y, x));
		//			vy[npixels] = (of.yflow.at<float>(y, x));
		//			++npixels;
		//		}
		//	}
		//}


		//// Get median value
		//double medianx, mediany;
		//std::sort(vx.begin(), vx.end());
		//std::sort(vy.begin(), vy.end());
		//int size = vx.size();
		//if (vx.size() % 2 == 0) {
		//	medianx = (vx[size / 2 - 1] + vx[size / 2]) / 2;
		//	mediany = (vy[size / 2 - 1] + vy[size / 2]) / 2;
		//} else {
		//	medianx =  vx[size / 2];
		//	mediany =  vy[size / 2];
		//}

		// Remove the object with less npxiels/rect.area. This might be ripples
		//cout << "npixels/rect.area " << ratio << endl;

		//if (ratio < 0.4)
			//continue;
		double vx, vy; int npixels;
		this->GetSpeed(rois[i], currTimeStamp - prvTimeStamp, vx, vy, npixels);

		double ratio = npixels / (rois[i].area() + 0.0);
		//cout << "npixels/rect.area " << ratio << endl;

		//if (ratio < 0.6) continue;

		Target object;
		object.SetValues(rois[i], vx, vy, npixels, currTimeStamp);
		//cout << " vx " << vx << " vy " << vy << endl;
		//RemoveMargin(object);
		object.Initialize(frame);
		objects.push_back(object);

		//std::cout << "object " << i <<", speed (" << xdis << ", " << ydis << ")" << endl;		

	}
}

/*
 *
 */
void MotionDetector::RemoveMargin(Target& object)
{
	////// Remove the margin introduced by optical flow
	int margin = 8;

	/*object.bb = Rect(object.bb.x + margin, object.bb.y + margin, object.bb.width - 2*margin, object.bb.height - 2l*margin);
	object.center.x = object.bb.x + object.bb.width/2.0;
	object.center.y = object.bb.y + object.bb.height/2.0;*/



	////// Shift the result caused by motion history analysis
	double ratio = 0.8;
	object.bb.x += cvRound(ratio*object.vx*m_nHistoFlows);
	object.bb.y += cvRound(ratio*object.vy*m_nHistoFlows);
	object.bb.width -= cvRound(0.5*ratio*object.vx*m_nHistoFlows);
	object.bb.height -= cvRound(0.5*ratio*object.vy*m_nHistoFlows);

		//objects[i].bb.y += 0.5*objects[i].ydis*nHistoFlows;
}


void MotionDetector::MotionDetection(const Mat& prvFrame, const Mat& currFrame,vector<Target>& objects, const double prvTimeStamp, const double currTimeStamp, bool showResult)
{
	objects.clear();
	optFlow oflow;
	double interval = currTimeStamp - prvTimeStamp;
	CalcDenseOpticalFlow(prvFrame, currFrame, oflow, showResult);
	// Save the optical flow in a pool for later analysis
	//flowPool[idxFrame % nHistoFlows] = oflow;
	m_flowPool.push_back(oflow);

	//int *p = new int[1024];

	Mat mask;
	if (m_flowPool.size() >= m_nHistoFlows) {
		AnalyzeFlowHistory(m_flowPool, mask, Rect(), false);
		

		// Morpholgoical operation
		int erosion_size = 4;
		Mat element = getStructuringElement( MORPH_RECT,
										   Size( 4*erosion_size + 1, 2*erosion_size+1 ),
										   Point( erosion_size, erosion_size ) );
		erode( mask, mask, element );
		dilate( mask, mask, element );

		optFlow& currOptFlow = m_flowPool.back();
		//Mat binaryMap;
		//BackgroundSubtraction(currFrame, binaryMap);
		if (1/*showResult*/) {
			m_vMask = mask.clone();
			normalize(m_vMask, m_vMask, 255, 0, NORM_MINMAX);
			m_vMask.convertTo(m_vMask, CV_8UC1);
			//imshow("segmentation", vMask);
		}
		GetObjects(mask, currFrame, currOptFlow, prvTimeStamp, currTimeStamp, objects);

		/*vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		Mat vmask = m_vMask.clone();
		cv::findContours(vmask, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		m_contour = Mat::zeros( vmask.size(), CV_8UC1 );
		RNG rng(12345);
		for( int i = 0; i< contours.size(); i++ ) {
			drawContours( m_contour, contours, i, CV_RGB(255, 255, 255), 1, 8, hierarchy, 0, Point() );
		}*/
		//imshow("countour", m_contour);
		m_flowPool.pop_front();
	}
	//m_speedFlow = m_flowPool.back().mag.clone();
	

	// Set the reference image for ssim
	if (objects.size() == 0)
		m_referenceImage = currFrame.clone();
}

void MotionDetector::MotionCorrection(vector<Target>& prvObjects, vector<Target>& objects, double prvTimeStamp, double currTimeStamp)
{
	// Predict where previous objects will appear
	vector<Target> prdctObjects = prvObjects;
	double dt = currTimeStamp - prvTimeStamp;
	for (size_t i = 0; i < prvObjects.size(); i++) {
		prdctObjects[i].bb.x += dt*prdctObjects[i].vx;
		prdctObjects[i].bb.y += dt*prdctObjects[i].vy;
		prdctObjects[i].timeStamp = currTimeStamp;
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
				if ((objects[i].bb & prdctObjects[j].bb).area() / (0.0+prdctObjects[j].bb.area()) > overlapThreshold) {
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


typedef struct _SimilarRects
{
	_SimilarRects(double _epsx, double _epsy) : epsx(_epsx), epsy(_epsy) {}
	inline bool operator()(const cv::Rect& r1, const cv::Rect& r2) const
	{
		double deltax = epsx*(MIN(r1.width, r2.width));
		double deltay = epsy*(MIN(r1.height, r2.height));

		return std::abs(r1.x - r2.x) <= deltax &&
			std::abs(r1.y - r2.y) <= deltay &&
			std::abs(r1.x + r1.width - r2.x - r2.width) <= deltax &&
			std::abs(r1.y + r1.height - r2.y - r2.height) <= deltay;
	}
	double epsx, epsy;
} SimilarRects;

bool MergeCandidates(int groupThreshold, const std::vector<CvRect>& candidateLocations, std::vector<CvRect>& objectLocations, vector<float>& detectionScores)
{
	bool result;
	std::vector<int> labels;
	int nObjects = 0;
	std::vector<float> objectWeights;
	std::vector<CvRect> maxRects;
	std::vector<float> maxScores;
	if (candidateLocations.size() > 0)
	{
		//Merge the obtained rectangles
		int nclasses = cv::partition(candidateLocations, labels, SimilarRects(0.5, 0.2));

		std::vector<CvRect> rrects(nclasses);
		std::vector<int> rweights(nclasses, 0);
		int nlabels = (int)labels.size();
		maxRects.resize(nclasses);
		maxScores = std::vector<float>(nclasses, -10000.0);
		for(int i = 0; i < nlabels; i++ )
		{
			int cls = labels[i];
			if (maxScores[cls] < detectionScores[i]) {
				maxScores[cls] = detectionScores[i];
				maxRects[cls] = candidateLocations[i];
			}
			rrects[cls].x += candidateLocations[i].x;
			rrects[cls].y += candidateLocations[i].y;
			rrects[cls].width += candidateLocations[i].width;
			rrects[cls].height += candidateLocations[i].height;
			rweights[cls]++;

		}

		objectLocations.clear();
		objectWeights.clear();

		for(int i = 0; i < nclasses; i++ )
		{
			if( rweights[i] < groupThreshold )
				continue;
			CvRect r = rrects[i];
			float s = 1.f/rweights[i];
			objectWeights.push_back(rweights[i]);
			//objectLocations.push_back(cvRect(cvRound(r.x*s), cvRound(r.y*s), cvRound(r.width*s), cvRound(r.height*s)));
			objectLocations.push_back(maxRects[i]);

			++nObjects;
		}
	} // if candidateLocations.size > 0

	if (nObjects >= 1) result = true;
	else result = false;
	return result;
}

void MotionDetector::GetSpeed(const Rect& roi, double timeInterval, double& vx, double& vy, int& nPixels)
{
	double xdis = 0.0, ydis = 0.0;
	nPixels = 0;
	const optFlow& of = m_flowPool.back();

	Mat roiMask = m_vMask(roi).clone();
	keepLargestRegion(roiMask.data, roiMask.cols, roiMask.rows);

	for (int y = 0; y < roi.height; y++) {
		for (int x = 0; x < roi.width; x++) {
			if (roiMask.at<uchar>(y, x) > 0) {
				xdis += of.xflow.at<float>(y+roi.y, x+roi.x);
				ydis += of.yflow.at<float>(y+roi.y, x+roi.x);
				++nPixels;
			}
		}
	}

	xdis /= nPixels; ydis /= nPixels;
	vx = xdis/timeInterval;
	vy = ydis/timeInterval;
}


}
