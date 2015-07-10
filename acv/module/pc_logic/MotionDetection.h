#ifndef __MOTIONDETECTION_WANGFANGLIN__
#define __MOTIONDETECTION_WANGFANGLIN__

#include <math.h>
#include <iostream>
#include <algorithm>    // std::min_element, std::max_element
#include "opencv2/opencv.hpp"
#include "bwlabel.h"
#include "target.h"
#include "utilities.h"
#include "target.h"

using namespace cv;
using namespace std;

namespace BoatDetection{

typedef struct _optFlow
{
	Mat xflow;	// The displacement of x direction
	Mat yflow;	// The displacement of y direction
	Mat mag;	// The velocity, calculated by xflow and yflow
	Mat ang;	// The movement direction, calculated by xflow and yflow
} optFlow;

class MotionDetector{

public:
	MotionDetector() : m_nHistoFlows(10), m_angle(0),
		m_angleThreshold(30.0), m_speedThreshold(0.5), m_angleHistoDiffDegThre(15), m_pixelThreshold(900), m_detectorThreshold(0)
	{
	}
	MotionDetector(int _nHistoFlows, int _angle, double _angleThreshold, double _speedThreshold,
		double _angleHistoDiffDegThre, int _pixelThreshold,	double _detectionThreshold, bool isMultiThreading = false)
		: m_nHistoFlows(_nHistoFlows), m_angle(_angle), m_angleThreshold(_angleThreshold),
		m_speedThreshold(_speedThreshold), m_angleHistoDiffDegThre(_angleHistoDiffDegThre), m_pixelThreshold(_pixelThreshold),
		m_detectorThreshold(_detectionThreshold), m_isMultiThreading(isMultiThreading)
	{
	}
public:
	/*
	 *	NOTE: The returned @mag and @ang are double type
	 */
	void CalcDenseOpticalFlow(const Mat& prvFrame, const Mat& currFrame, optFlow& oflow, bool showResult);

	void MotionDetection(const Mat& prvFrame, const Mat& currFrame, vector<Target>& objects, double prvTimeStamp, double currTimeStamp, bool showResult);

	void MotionCorrection(vector<Target>& prvObjects, vector<Target>& objects, double prvTimeStamp, double currTimeStamp);

	void GetObjects(Mat& mask, const Mat& frame, const optFlow& of, double prvTimestap, double currTimeStamp, vector<Target>& objects);

	// Get the current optical flow for display
	const Mat& GetOptflowVisual() const
	{
		return m_vFlow;
	}

	// Get the current mask for display
	const Mat& GetMaskVisual() const
	{
		return m_vMask;
	}

	/*
	 * Image pixel clustering by using the pixel color directly
	 */
	void PixelClustering(const Mat& image, Mat& clusters);
private:

	void CheckRangeOfMat(Mat mat);
	/*
	 * Compute the diffusion of a set of angles.
	 *
	 * NOTE: the angle range is [0, 360]
	 */
	inline float ComputeAngleDiffusion(double *val, int size);

	/*
	 * Compute the diffusion of a set of magnitude values
	 *
	 */
	inline double ComputeMagDiffusion(double *val, int size);

	/*
	 * From http://www.cplusplus.com/forum/general/87903/
	 * To compute the distance from point to line
	 */
	double Distance_to_Line(cv::Point2f line_start, cv::Point2f line_end, cv::Point2f point)
	{
#ifdef __linux__
		double normalLength = hypot(line_end.x - line_start.x, line_end.y - line_start.y);
#else
		double normalLength = _hypot(line_end.x - line_start.x, line_end.y - line_start.y);
#endif
		double distance = (double)((point.x - line_start.x) * (line_start.y - line_end.y) - (point.y - line_start.y) * (line_start.x - line_end.x)) / normalLength;
		return std::abs(distance);
	}

	/*
	 * Determine whether a pixel belongs to foreground based on its optical flow history.
	 * If this pixel changes its speed value or direction too much then it cannot be the foreground.
	 */
	void AnalyzeFlowHistory(const list<optFlow>& fp, Mat& mask, const Rect& roi = Rect(), bool saveSamples = false);

	// A dummy in order to use boost::bind
	void calcOpticalFlowFarneback_dummy(int i, double pyr_scale, int levels, int winsize,
                           int iterations, int poly_n, double poly_sigma, int flags);


	void RemoveMargin(Target& object);

	/*
	 * Given an roi, get the average speed based on the optical flow map
	 */
	void GetSpeed(const Rect& roi, double timeInterval, double& vx, double& vy, int& nPixels);

private:
	int		m_nHistoFlows;
	double	m_angle;						// The angle we're interested
	double	m_angleThreshold;				// The angle tolerance value
	double	m_speedThreshold;				// The speed tolerance value
	double	m_angleHistoryDiffusionThre;	// The angle diffusion tolerance value
	double  m_angleHistoDiffDegThre;		//
	int		m_pixelThreshold;
	Mat		m_vFlow;						// The visualized optical flow image
	Mat		m_vMask;						// The visualized mask image
	Mat		m_contour;						// The motion contour
	Mat		m_speedFlow;						// The motion speed

	list<optFlow> m_flowPool;				// The optical flow pool to store historical ones

	double	m_detectorThreshold;				// For ship recognition
	bool	m_isMultiThreading;				// Whether to use multi-threading
	Mat		m_referenceImage;				// The reference image for SSIM

	//vector<DetectionResult> m_objects;	// The detected objects
};

}//namespace

#endif
