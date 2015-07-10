#ifndef __TARGET_WANGFANGLIN__
#define __TARGET_WANGFANGLIN__
#include "opencv2/opencv.hpp"
#include "utilities.h"
#include <assert.h>
#include <algorithm>
#include <vector>

using namespace cv;
using namespace std;

#define MINIMAL_SCORE	-10000

const Rect zeroRect = Rect(0, 0, 0, 0);

class Target
{
public:
	Target(const Rect& _bb, double _vx, double _vy, int _area, double _timeStamp):bb(_bb),vx(_vx),vy(_vy),area(_area),timeStamp(_timeStamp),isFromTracker(false)
	{
		this -> GetVelociyAndDirection();
	}

	Target(const Target& dr):bb(dr.bb),vx(dr.vx),vy(dr.vy),area(dr.area),timeStamp(dr.timeStamp),isFromTracker(dr.isFromTracker)
	{
		this->hist = dr.hist.clone();
		this -> GetVelociyAndDirection();
		m_imagePatch = dr.m_imagePatch.clone();
	}

	Target():bb(zeroRect),vx(0.0),vy(0.0),area(0),timeStamp(-1),isFromTracker(false)
	{
		this -> GetVelociyAndDirection();
	}

	Target& operator=(const Target& dr)
	{
		bb = dr.bb; vx = dr.vx; vy = dr.vy; area = dr.area;
		timeStamp = dr.timeStamp;
		this->hist = dr.hist.clone();
		m_imagePatch = dr.m_imagePatch.clone();
		this -> GetVelociyAndDirection();
		this->isFromTracker = dr.isFromTracker;		
		return *this;
	}

	void GetRotatedBB(Mat& mask);

	double GetGap_x(const Target& dr) const {
		//double result = 0.0;
		//if ((this->bb & dr.bb).area() == 0) {
		//	result = min(std::abs(this->bb.tl().x + 1 - dr.bb.br().x - 1 + 1), std::abs(this->bb.br().x + 1 - dr.bb.tl().x - 1 + 1));
		//	//int height = std::max(std::abs(this->bb.tl().y + 1 - dr.bb.br().y - 1 + 1), std::abs(this->bb.br().x + 1 - dr.bb.tl().x - 1 + 1));
		//}
		//// To be implemented
		//return result;

		// Added by Fanglin, 2014-09-23
		return GapDistance(this->bb.tl().x, this->bb.br().x, dr.bb.tl().x, dr.bb.br().x);
	}

	double GetGap_y(const Target& dr) const {
		return GapDistance(this->bb.tl().y, this->bb.br().y, dr.bb.tl().y, dr.bb.br().y);
	}

	/*
	 * Compute the distance of two range [min1, max1], [min2, max2]
	 * The distance will be zero if they intersect.
	 */
	double GapDistance(double min1, double max1, double min2, double max2) const
	{
		double mn = max(min1, min2);
		double mx = min(max1, max2);
		double dis = mn - mx;
		if (dis < 0.0) dis = 0.0;  // There is intersection
		return dis;
	}

	void ComputeHSVHist(const Mat& image, Rect& bb);

	void SetValues(const Rect& _bb, double _vx, double _vy, int _area, double _timeStamp)
	{
		bb = _bb;
		//velocity = _velocity;
		//direction = _direction;
		vx = _vx;
		vy = _vy;
		this -> GetVelociyAndDirection();
		timeStamp = _timeStamp;
		area = _area;
	}

	void Initialize(const Mat& image)
	{
		this -> GetVelociyAndDirection();
		ComputeHSVHist(image, bb);
		this->isFromTracker = false;
	}

public:

	// We trust the result from optical flow, so this should be strict
	// E.g., deviation of 0.5 has a high penalty
	double VelocitySim(const Target& object) const
	{
		double dis = this->velocity - object.velocity;
		double sigma = 1.0;
		double sim = exp(-dis*dis/(sigma*sigma));
		return sim;
	}
	// We trust the result from optical flow, so this should be strict
	// E.g., deviation of 30 degree has a high penalty
	double DirectionSim(const Target& object) const
	{
		double dis = DIFF_ANGLE(this->direction, object.direction);
		double sigma = 60.0;
		double sim = exp(-dis*dis/(sigma*sigma));
		//cout << "direction sim " << sim << "\n";
		return sim;
	}

	double PosSim(const Target& object, double sigma = 10) const
	{
		double disx, disy;
		disx = this->bb.x + this->bb.width/2.0 - object.bb.x - object.bb.width/2.0;
		disy = this->bb.y + this->bb.height/2.0 - object.bb.y - object.bb.height/2.0;
		//double sigma = 10;
		double sim = exp(-(disx*disx + disy*disy)/(2*sigma*sigma));
		//cout << "pos dis" << sqrt(disx*disx + disy*disy) << " pos sim " << sim << endl;
		return sim;
	}

	bool IsClose2ImageBorder(int imageWidth, float ratio = 0.2) const
	{
		if (this->bb.x < ratio*imageWidth || this->bb.x + this->bb.width > (1-ratio)*imageWidth)
			return true;
		else
			return false;
	}
	// Size estimation is from segmantation which is not that accurate.
	// The penalty here is not strict.
	double SizeSim(const Target& object, double sigma = 0.8) const
	{
		/*double dis = this->area - object.area;
		double sigma = 2500.0;
		double sim = exp(-dis*dis/(sigma*sigma));*/

		double difference = (this->area + 0.0)/ object.area;
		if (difference > 1) difference = 1.0/difference;
		difference = 1 - difference;
		double sim = exp(-difference*difference/(sigma*sigma));
		//cout << "size sim " << sim << "\t";
		return sim;
	}

	// Color histogram is quite robust, hence its penalty is at a middele stage.
	double AppearSim(const Target& object) const
	{
		if (!this->hist.data || !object.hist.data) return DBL_EPSILON;
		// 4 methods for hsit comparison: CV_COMP_CORREL, CV_COMP_CHISQR, CV_COMP_INTERSECT,  CV_COMP_BHATTACHARYYA
		double sim = compareHist(this->hist, object.hist, CV_COMP_CORREL);
		//cout << "appea sim " << sim  << "\t";
		return sim;
	}

	// Get the similarity of two objects
	double Compare(const Target& object) const
	{
		double sim = 0.0;
		if (this->isFromTracker || object.isFromTracker)
			sim = SizeSim(object) * AppearSim(object) * PosSim(object, 20);
		else
			sim = /*VelocitySim(object) * */DirectionSim(object) * SizeSim(object) * AppearSim(object) * PosSim(object, 20);


		//std::cout << "sim " << sim << std::endl;
		return sim;
	}

	void PersepctiveTransform(const Point2f& in, const Mat& h, Point2f& out)
	{
		out.x = h.at<double>(0, 0)*in.x + h.at<double>(0,1)*in.y + h.at<double>(0,2);
		out.x /= h.at<double>(2, 0)*in.x + h.at<double>(2,1)*in.y + h.at<double>(2,2);
		out.y = h.at<double>(1, 0)*in.x + h.at<double>(1,1)*in.y + h.at<double>(1,2);
		out.y /= h.at<double>(2, 0)*in.x + h.at<double>(2,1)*in.y + h.at<double>(2,2);
	}

	double Compare_DLBP(const Target& object) const;
		
private:
	void GetVelociyAndDirection()
	{
		this->velocity = sqrt(vx*vx + vy*vy);
		this->direction = atan2(vy, vx)*180/M_PI + 180;
	}

public:
	Rect	bb;
	double	velocity;		// Velocity magnitude
	double	direction;		// Ranged in [0, 360]
	double  vx, vy;			// Velocity in x and y directions
	MatND	hist;			// The histogram feature
	double	timeStamp;		// What time this object was detected
	int		area;			// The area the boat candidate, the number of pxiels in silhouette image
	Mat		m_imagePatch;
	bool    isFromTracker;	// True if it's generated by tracker
private:
	int		bbBorderThresh_;	// Number of pixel thresholds for bounding box.
	int		bbBorderThresv_;
};

void SaveMotionSegmentation(const Mat& frame, string videoFilename, string foldePath, const vector<Target> objects, int frameID);

#endif
