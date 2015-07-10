#ifndef __COLOR_LBP_DETECTOR_WFL__
#define __COLOR_LBP_DETECTOR_WFL__
#include "LBPDetector.h"
#include "opencv2/opencv.hpp"

class ColorLBPDetector:public LBPDetector
{
private:
	/**
	 * Disable the default empty constructor since I still don't need it
	 */
	LBPDETECTOR_API ColorLBPDetector(void);
	/**
	 * Disable the default copy constructor since I still don't need it
	 */
	LBPDETECTOR_API ColorLBPDetector(const ColorLBPDetector&);
	/**
	 * Disable the default assignment operator since I still don't need it
	 */
	LBPDETECTOR_API ColorLBPDetector& operator = (const ColorLBPDetector&);

public:
	/**
	 * Constructor
	 */
	LBPDETECTOR_API ColorLBPDetector(int width, int height, int blockWidth, int blockHeight,
		int strideWidth, int strideHeight, int padWidth, int padHeight);

	/**
	 * Destructor
	 */
	LBPDETECTOR_API ~ColorLBPDetector(void);

	/**
	 * Detect an object in an image. If there are more than one objects, only the most similar one is returned
	 */
	LBPDETECTOR_API bool DetectObject(const IplImage *image, CvRect *resultRect, Orientation *isLeft = false, 
		float threshold = 0.0f, int groupThreshold = 2, float scaleStep = 1.1);

	/**
	 * Detect all objects in an image.
	 */
	LBPDETECTOR_API bool DetectObjects(const IplImage *image, std::vector<CvRect>& resultRect, std::vector<Orientation> *orientation = 0, 
		float threshold = 0.0f, int groupThreshold = 2, float scaleStep = 1.1, const IplImage *maskImage = 0);

	/**
	 * Detect all candidates with positive classifying values. 
	 * A candidate cannot be regarded as an object before merging
	 */
	LBPDETECTOR_API bool DetectAllCandidates(const IplImage *image, std::vector<CvRect> *resultRect, float threshold = 0.0f, float scaleStep = 1.1);

	/**
	 * Detect the best candidate among those candidates with positive classifying values
	 */
	LBPDETECTOR_API bool DetectBestCandidate(const IplImage *image, CvRect *resultRect, float threshold = 0.0f, float scaleStep = 1.1);

	LBPDETECTOR_API bool DetectBestNCandidates(const IplImage *image, vector<CvRect>& resultRects, float threshold = 0.0f, float scaleStep = 1.1);
		
private:
	/**
	 * Find all candidates with positive classifying values
	 */
	LBPDETECTOR_API bool FindAllCandidates(const IplImage *image, float threshold = 0.0, float scaleStep = 1.1, const IplImage *maskImage = 0);
	 
	LBPDETECTOR_API bool FindAllCandidates_aspectratio(const IplImage *image, float threshold, float scaleStep, const IplImage *maskImage = 0);

	/**
	 * Merge candidates
	 */
	LBPDETECTOR_API bool MergeCandidates(int groupThreshold = 2);
	
private:
	//Copied from source code of opecv2.0 for the sake of merging
	

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
};


#endif