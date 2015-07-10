#ifndef __MULTIPLE_DETECTOR_WFL_
#define __MULTIPLE_DETECTOR_WFL_
#include "BlockLBPDetector.h"

class MultipleDetectors:public BlockLBPDetector
{
	/**
	 * Constructor
	 */
	LBPDETECTOR_API MultipleDetectors(int width, int height, int blockWidth, int blockHeight,
		int strideWidth, int strideHeight, int padWidth, int padHeight)
		:BlockLBPDetector(width, height, blockWidth, blockHeight, strideWidth, strideHeight, padWidth, padHeight)
	{
	}

	LBPDETECTOR_API void SetDetectors(vector<string> filePaths);

	LBPDETECTOR_API void DetectMultipleObjects(const IplImage *image, vector<vector<CvRect>>& resultMultiRects, float threshold, int groupThreshold, float scaleStep);

private:
	vector<vector<float>> detectors;
	vector<float> rhoDetectors;

};
#endif