#ifndef _BLOCKLBPDETECTOR_H
#define _BLOCKLBPDETECTOR_H
/*
 * Created in 2009 in Shanghai. Modified on 2013-10-22. WANG Fanglin
 *
 * Object detector works with BlockDenseLBP feature
 */
#include "LBPDetector.h"
#include "opencv2/opencv.hpp"

#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>


#define USE_MULTITHREADS

class BlockLBPDetector:public LBPDetector
{
private:
	/**
	 * Disable the default empty constructor since I still don't need it
	 */
	LBPDETECTOR_API BlockLBPDetector(void);
	/**
	 * Disable the default copy constructor since I still don't need it
	 */
	LBPDETECTOR_API BlockLBPDetector(const BlockLBPDetector&);
	/**
	 * Disable the default assignment operator since I still don't need it
	 */
	LBPDETECTOR_API BlockLBPDetector& operator = (const BlockLBPDetector&);

public:
	/**
	 * Constructor
	 */
	LBPDETECTOR_API BlockLBPDetector(int width, int height, int blockWidth, int blockHeight,
		int strideWidth, int strideHeight, int padWidth, int padHeight)
		:LBPDetector(width, height,blockWidth, blockHeight, strideWidth, strideHeight, padWidth, padHeight)
	{
		detector.clear();
		recognizer.clear();
	}

	/**
	 * Constructor from struct.
	 * Added by Fanglin, 2014-09-22
	 */
	LBPDETECTOR_API BlockLBPDetector(const DLBPConfiguration& config)
		:LBPDetector(config)
	{
		detector.clear();
		recognizer.clear();
	}

	/**
	 * Destructor
	 */
	LBPDETECTOR_API ~BlockLBPDetector(void)
	{
		detector.clear();
		recognizer.clear();
		this->ReleaseDLBPBuffer();
	}

	/**
	 * Detect an object in an image. If there are more than one objects, only the most similar one is returned
	 */
	LBPDETECTOR_API bool DetectObject(const IplImage *image, CvRect *resultRect, Orientation *isLeft = NULL,
		float threshold = 0.0f, int groupThreshold = 2, float scaleStep = 1.1);

	/**
	 * Detect all objects in an image.
	 */
	LBPDETECTOR_API bool DetectObjects(const IplImage *image, std::vector<CvRect>& resultRect, std::vector<Orientation> *orientation = 0,
		float threshold = 0.0f, int groupThreshold = 2, float scaleStep = 1.1, IplImage *maskImage = 0);

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

	LBPDETECTOR_API bool DetectObjects_byBuffer(const IplImage *image, std::vector<CvRect>& resultRect, std::vector<Orientation> *orientation = 0, 
			float threshold = 0.0f, int groupThreshold = 2, float scaleStep = 1.1, IplImage *maskImage = 0);

	LBPDETECTOR_API void BufferDLBP(const IplImage *image, float scaleStep = 1.1, IplImage *maskImage = 0);

private:
	/**
	 * Find all candidates with positive classifying values
	 */
	LBPDETECTOR_API bool FindAllCandidates(const IplImage *image, float threshold = 0.0, float scaleStep = 1.1, IplImage *maskImage = 0);

	LBPDETECTOR_API bool FindAllCandidates_aspectratio(const IplImage *image, float threshold, float scaleStep, const vector<cv::Rect> *maskRects);
	LBPDETECTOR_API bool FindAllCandidates_bybuffer(const IplImage *image, float threshold, float scaleStep, IplImage *maskImage = 0);

		// For multithreading
	void ComputeDBLPBuffer(const IplImage *image, BlockDenseLBP **pplbp, CvSize sz, IplImage *maskImage = 0);
	void DetectScale_bybuffer(const IplImage* image, BlockDenseLBP *dlbp, const CvSize sz, float scale, float threshold, IplImage *maskImage = 0);

	/**
	 * Merge candidates
	 */
	LBPDETECTOR_API bool MergeCandidates(int groupThreshold = 2);
private:
	void ReleaseDLBPBuffer() {
		for (int i = 0; i < m_dlbps.size(); i++) {
			delete m_dlbps[i]; m_dlbps[i] = 0;
		}
		m_dlbps.clear();
	}
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

private:
	vector<BlockDenseLBP*> m_dlbps;

#ifdef USE_MULTITHREADS
	boost::mutex guard;
#endif

};


#endif
