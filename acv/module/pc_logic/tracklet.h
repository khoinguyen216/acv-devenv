#ifndef __TRRACKLET_WANGFANGLIN
#define __TRRACKLET_WANGFANGLIN
/* 
  Some ideas here have refered to paper below:
	[1] Chang Huang, Bo Wu, and Ramakant Nevatia. 
	"Robust object tracking by hierarchical association of detection responses." 
	ECCV 2008. Springer Berlin Heidelberg, 2008. 788-801.
*/

#include "opencv2/opencv.hpp"
#include "target.h"
#include <float.h>
#include "utilities.h"
#include "CompressiveTracker.h"

using namespace cv;

class Tracklet
{
public:
	Tracklet()
	{
		objects.clear();
		detectionPrecision = 0.81;	// We set it temporarily
		detectionMissRate = 0.9;	// We set it temporarily

		// TODO: get the real time. Here we use 5 frames as interval.
		timeSpan[0] = 4*SECOND;
		timeSpan[1] = 6*SECOND;
		timeSpan[2] = 8*SECOND;

		Z[0] = 1.0;
		Z[1] = 0.9;
		Z[2] = 0.8;
		sum_vx = 0;
		sum_vy = 0;

		m_ct = 0;
		m_dlbp.clear();
		m_hasDenerated = false;
	}

	Tracklet(const Tracklet& t)
	{
		memcpy(this->timeSpan, t.timeSpan, 3*sizeof(double));
		memcpy(this->Z, t.Z, 3*sizeof(double));
		this->detectionPrecision = t.detectionPrecision;
		this->detectionMissRate = t.detectionMissRate;
		this->objects.clear();
		this->objects.resize(t.objects.size());
		std::copy(t.objects.begin(), t.objects.end(), this->objects.begin());
		this->sum_vx = t.sum_vx;
		this->sum_vy = t.sum_vy;
		m_ct = 0;
		m_backgroundImage = t.m_backgroundImage.clone();
		m_dlbp = t.m_dlbp;

		if (t.m_ct)
			m_ct = new CompressiveTracker(*(t.m_ct));
		m_trackerBB = t.m_trackerBB;
		m_hasDenerated = t.m_hasDenerated;
	}

	~Tracklet()
	{
		objects.clear();
		delete m_ct;
	}

	Tracklet& operator=(const Tracklet& t)
	{
		memcpy(this->timeSpan, t.timeSpan, 3*sizeof(double));
		memcpy(this->Z, t.Z, 3*sizeof(double));
		this->detectionPrecision = t.detectionPrecision;
		this->detectionMissRate = t.detectionMissRate;
		this->objects.clear();
		this->objects.resize(t.objects.size());
		std::copy(t.objects.begin(), t.objects.end(), this->objects.begin());
		this->sum_vx = t.sum_vx;
		this->sum_vy = t.sum_vy;

		m_backgroundImage = t.m_backgroundImage.clone();

		m_dlbp = t.m_dlbp;
		m_hasDenerated = t.m_hasDenerated;

		// For compressive tracker
		if (m_ct) {
			delete m_ct; m_ct = 0;
		}
		m_trackerBB = t.m_trackerBB;
		if (t.m_ct)
			m_ct = new CompressiveTracker(*(t.m_ct));
		return *this;
	}

public:
	void InitializeKalmanFilter(KalmanFilter& KF);
	bool IsLinkable(const Target& object, double prvTimeStamp, double *score, double threshold = 0.4);

	void Create(const Target& object, const Mat& frame)
	{
		objects.clear();
		objects.push_back(object);
		m_backgroundImage = frame.clone();
	}
	void Link(Target& object) 
	{
		objects.push_back(object);
	}
	/*void Create(DetectionResult& object)
	{
		objects.clear();
		objects.push_back(object);
	}*/

public:
	const Target& FirstObject() const 
	{
		return objects[0];
	}
	const Target& LastObject() const 
	{
		return objects[objects.size()-1];
	}
	double GetAveSpeedX() const
	{
		return sum_vx/objects.size();
	}
	double GetAveSpeedY() const
	{
		return sum_vy/objects.size();
	}
	bool Append(const Tracklet& tracklet);
	bool IsTemporalIntersected(const Tracklet& tracklet) const;

	// Get the time gap between unintersected tracklets
	double TimeGap(const Tracklet& tracklet) const
	{
		double result = 0.0;
		if (!this->IsTemporalIntersected(tracklet)) {
			result = min(abs(this->StartTime() - tracklet.EndTime()), abs(this->EndTime() - tracklet.StartTime()));
		}
		//std::cout << "Time gap " << result  << std::endl;
		return result;
	}
	
	bool Link(const Tracklet& tracklet);

	double StartTime() const
	{
		return this->objects.front().timeStamp;
	}
	double EndTime() const
	{
		return this->objects.back().timeStamp;
	}

	/*
	 * The link probability between two tracklets. Based on appearance, motion, and temopral 
	 * According to [1]:
	 *  - appearance feature of each tracklet is after RANSAC on all the contained detections
	 *  - motion model is after kalman filter
	 *
	 */
	double LinkProbability(const Tracklet& tracklet, int round);

	double AppearanceAffinity(const Tracklet& tracklet);

	double TemporalAffinity(const Tracklet& tracklet, int round);

	double DisAffinity(const Tracklet& tracklet);

	double InitilizationProb(int round)
	{
		double result = log(Z[round]*pow(detectionMissRate, timeSpan[round]/2));
		return result;
	}

	double TerminationProb(int round)
	{
		double result = log(Z[round]*pow(detectionMissRate, timeSpan[round]/2));
		return result;
	}

	// If the tracklet is too short, it will be not effective enough
	bool IsEffective() 
	{
		bool result = true;
		double duration = objects.back().timeStamp - objects.front().timeStamp;
		if (duration < 0.5*SECOND)
			result = false;
		//if (objects.size() < 5) result = false;
		return result;
	}

	// If the tracklet is too short, it will be not effective enough
	bool IsLongTracklet() 
	{
		bool result = true;
		double duration = objects.back().timeStamp - objects.front().timeStamp;
		if (duration < 1*SECOND)
			result = false;
		//if (objects.size() < 10) result = false;
		return result;
	}

	// Finds the intersection of two lines, or returns false.
	// The lines are defined by (o1, p1) and (o2, p2).
	bool Intersection(const Point2d& o1, const Point2d& p1, const Point2d& o2, const Point2d& p2, Point2d& r)
	{
		Point2d x = o2 - o1;
		Point2d d1 = p1 - o1;
		Point2d d2 = p2 - o2;

		float cross = d1.x*d2.y - d1.y*d2.x;
		if (abs(cross) < /*EPS*/1e-8)
			return false;

		double t1 = (x.x * d2.y - x.y * d2.x)/cross;
		r = o1 + d1 * t1;
		return true;
	}

	bool IsPointOnLineSegment(const Point2d& linePointA, const Point2d& linePointB, const Point2d& point) 
	{
		double EPSILON = 1e-2;
		double a = (linePointB.y - linePointA.y) / (linePointB.x - linePointA.x + DBL_EPSILON);
		double b = linePointA.y - a * linePointA.x;
		double px = point.x;
		double py = a*point.x+b;
		double dis = fabs(point.y - (a*point.x+b));
		
		/*if ( dis < EPSILON)
		{
			return true;
		}*/
		if ( dis < EPSILON && (px - linePointA.x)*(px - linePointB.x) <= 0 && (py - linePointA.y)*(py - linePointB.y) <= 0)
			return true;
		else
			return false;
	}

	bool AreLineSegmentsIntersecting(const Point2d& o1, const Point2d& p1, const Point2d& o2, const Point2d& p2)
	{
		Point2d interPoint;
		Intersection(o1, p1, o2, p2, interPoint);
		bool result = IsPointOnLineSegment(o1, p1, interPoint);
		result &= IsPointOnLineSegment(o2, p2, interPoint);
		return result;
	}

	// Whether this tracklet ends up crossing a line segment
	bool IsCrossingLineSegment(const Point2d& start, const Point2d& end)
	{
		bool result = false;
		// We only consider the last n points
		int nPoints = 5;
		if (this->objects.size() < nPoints)
			return false;
		Point2d ep =  GetRectCenter(this->objects.back().bb);
		for (int i = this->objects.size() - 2; i >= this->objects.size() - nPoints; i--) {
			Point2d cp = GetRectCenter(this->objects[i].bb);
			result = AreLineSegmentsIntersecting(start, end, ep, cp);
			if (result)
				break;
		}
		return result;
	}

	// Whether this tracklet ends up crossing a rectangular region
	bool IsCrossingRect(const Rect& rect)
	{
		bool result = true;
		// We only consider the last n points
		int nPoints = 5;
		if (this->objects.size() <= nPoints)
			return false;
		int nIntersection = 0;
		for (int i = this->objects.size() - 1; i >= this->objects.size() - nPoints; i--) {
			const Rect& bb = this->objects[i].bb;
			if ((bb & rect).area() > 0.3*rect.area()) {
				++nIntersection;
			}
		}
		if (nIntersection >= nPoints - 2)
			result = true;
		else 
			result = false;
		return result;
	}

	// Whether this tracklet ends up being big
	bool IsTargetBigEnough(int areaThreshold)
	{
		bool result = false;
		int nPoints = 5;
		if (this->objects.size() < nPoints)
			return false;

		int maxArea = 0;
		for (int i = 0; i < this->objects.size(); i++) {
			const Rect& bb = this->objects[i].bb;
			if (maxArea < bb.area())
				maxArea = bb.area();
			
		}

		cout << "max area " << maxArea << " areaThreshold "<< areaThreshold << endl;
		if (maxArea > areaThreshold)
			result = true;

		return result;
	}

	// The likelihood of the tracklet being positive
	double Likelihood_Positive()
	{
		//NOTE: log here is the natural logarithm with base e
		double result = objects.size() * log(detectionPrecision);
		return result;
	}
	// The likelihood of the tracklet being positive
	double Likelihood_Negative()
	{
		//NOTE: log here is the natural logarithm with base e
		double result = objects.size() * log(1.0- detectionPrecision);
		return result;
	}
	
public:
	vector<Target> objects;
	double detectionPrecision;
	double detectionMissRate;
	/// Initialization of kalman filter
	int timeSpan[3];
	double Z[3];
	int associationResult;
	double sum_vx, sum_vy;	

//For visual tracking
private:
	CompressiveTracker *m_ct;
	Mat				m_backgroundImage;
public:
	Rect			m_trackerBB;
	vector<float>	m_dlbp;
	double			m_stTimeStamp;	// Tracking start time stamp
	int				m_initIndex;		// object index for tracking initilization
	bool			m_hasDenerated;		// Whether the tracking has degenerated

public:
	void InitTracker(const Mat& frame, const Rect& bb, double timeStamp);
	bool VisualTracking(const Mat& frame, Rect& trackedBB, double currTimeStamp);

};

#endif