#include "tracklet.h"
#include "utilities.h"
#include "math.h"
#include <iostream>
#include <stdlib.h>

/* 
  Some ideas here refer to paper below:
	[1] Chang Huang, Bo Wu, and Ramakant Nevatia. 
	"Robust object tracking by hierarchical association of detection responses." 
	ECCV 2008. Springer Berlin Heidelberg, 2008. 788-801.
*/


bool Tracklet::IsLinkable(const Target& object, double prvTimeStamp, double* score, double simThreshold)
{
	bool result = false;
	if (objects.size() == 0)
		result = false;
	else {
		Target& lastObject = this->objects.back();
		// Whether is from last frame
		double interval = abs(lastObject.timeStamp - prvTimeStamp);
		if (interval < 1e-6) {
			*score = lastObject.Compare(object);
			//cout << "link score " << *score << endl;
			if (*score > simThreshold)
				result = true;
		}

	}
	return result;
}

bool Tracklet::IsTemporalIntersected(const Tracklet& tracklet) const
{
	// If there is time overlap between the two tracklets
	double st1, et1, st2, et2;
	bool result = true;
	st1 = this->StartTime();
	et1 = this->EndTime();
	st2 = tracklet.StartTime();
	et2 = tracklet.EndTime();
	
	if (st2 > et1 || st1 > et2)
		result = false;

	return result;
}

bool Tracklet::Append(const Tracklet& tracklet)
{
	if (this->IsTemporalIntersected(tracklet)) return false;
	if (!(this->EndTime() < tracklet.StartTime()))
		return false;

	// Allocate the space
	this->objects.reserve(this->objects.size() + tracklet.objects.size());
	this->objects.insert(this->objects.end(), tracklet.objects.begin(), tracklet.objects.end());
	
	//std::cout << "Early track (" << this->objects[this->objects.size()-1].bb.x << ", " << this->objects[this->objects.size()-1].bb.y << ") ";
	//std::cout << "Late track (" << tracklet.objects[0].bb.x << ", " << tracklet.objects[0].bb.y << ") " << std::endl;

	this->sum_vx += tracklet.sum_vx;
	this->sum_vy += tracklet.sum_vy;
}

// In [1], the authors use RANSAC to get a histogram from a set of histograms.
// I don't think it makes sense. Here instead, we just choose the middle detection for each tracklet.
double Tracklet::AppearanceAffinity(const Tracklet& tracklet)
{
	if (this->objects.size() == 0 || tracklet.objects.size() == 0)
		return DBL_EPSILON;
	
	int idxMid1, idxMid2;
	idxMid1 = this->objects.size()/2;
	idxMid2 = tracklet.objects.size()/2;
	Target& object1 = this->objects[idxMid1];
	const Target& object2 = tracklet.objects[idxMid2];
	return object1.AppearSim(object2);
}


//TODO: occupancy map mentioned in [1]
double Tracklet::TemporalAffinity(const Tracklet& tracklet, int round)
{
	if (this->IsTemporalIntersected(tracklet))
		return DBL_EPSILON;

	const Tracklet *earlyTrac, *lateTrac;	
		
	double timeGap = this->TimeGap(tracklet);	
	if (timeGap > timeSpan[round])
		return DBL_EPSILON;
	else {
		double result = Z[round]*pow(detectionMissRate, timeGap/SECOND/0.3);
		return result;
	}
}

double Tracklet::DisAffinity(const Tracklet& tracklet)
{
	return (this->objects.back().PosSim(tracklet.objects.back()));
}

/*
 * The link probability between two tracklets 
 * The app
 */
double Tracklet::LinkProbability(const Tracklet& tracklet, int round)
{
	double result = 1.0;

	// If there is time overlap between the two tracklets
	double st1, et1, st2, et2;
	st1 = this->objects.front().timeStamp;
	et1 = this->objects.back().timeStamp;
	st2 = tracklet.objects.front().timeStamp;
	et2 = tracklet.objects.back().timeStamp;

	if ((st1 >= st2 && st1 <= et2) || (et1 >= st2 && et1 <= et2) || (st2 >= st1 && st2 <= et1) || (et2 >= st1 && et2 <= et1))
		return MINUS_INFINITE;

	std::cout << "No overlap!" << std::endl;	
	double appearanceA = AppearanceAffinity(tracklet);
	double temporalA = TemporalAffinity(tracklet, round);
	result = log(appearanceA*temporalA + DBL_EPSILON);
	return result;
	
}

void Tracklet::InitTracker(const Mat& frame, const Rect& bb, double timeStamp)
{
	if (m_ct) {
		delete m_ct; m_ct = 0;
	}
	Mat grayImage;
	if (frame.channels() != 1)
		cvtColor(frame, grayImage, CV_BGR2GRAY);
	else
		grayImage = frame;

	m_ct = new CompressiveTracker(20, 10);
	m_trackerBB = bb;
	m_ct->init(grayImage, m_trackerBB);
	Rect nb = bb;
	nb &= Rect(0, 0, frame.cols, frame.rows);
	ComputeDLBP(grayImage(nb), this->m_dlbp);
	m_stTimeStamp = timeStamp;
}

bool Tracklet::VisualTracking(const Mat& frame, Rect& trackedBB, double currTimeStamp)
{
	if (!m_ct) return false;
	Mat grayImage;
	if (frame.channels() != 1)
		cvtColor(frame, grayImage, CV_BGR2GRAY);
	else
		grayImage = frame;
	
	// Get prediction from last frame
	//double dt = currTimeStamp - this->objects.back().timeStamp;
	//m_trackerBB.x += dt * this->objects.back().vx;
	//m_trackerBB.y += dt * this->objects.back().vy;
	//// To avoid out of bound
	//m_trackerBB &= Rect(0, 0, frame.cols, frame.rows);
	Rect prevBB = m_trackerBB;
	m_ct->processFrame(grayImage, m_trackerBB);	

	bool isBackground = false;
	Rect initBB = objects[0].bb;
	//if (m_trackerBB.width > 0 && m_trackerBB.height > 0)
	if (prevBB == Rect())
		return false;
	isBackground = SimilarImagePatch_ssim(g_backgroundImage, frame, initBB, m_trackerBB, 0.6);
	//isBackground = SimilarImagePatch_ssim(m_backgroundImage, frame, initBB, m_trackerBB, 0.5);

	bool isTarget;
	if (isBackground)
		m_trackerBB = Rect();
	trackedBB = m_trackerBB;
	return (!isBackground);
}