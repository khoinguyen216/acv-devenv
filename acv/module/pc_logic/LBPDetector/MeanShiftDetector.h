#ifndef _MEANSHIFTDLBP_H
#define _MEANSHIFTDLBP_H

#include "LBPDetector.h"
#include "ExportDLBP.h"

// TODO: nums_c might be wrong in ExtractDenseLBP(IplImage *image, CvRect roi)
/**
 * Extract the dense LBP feature from an image region
 * A dense LBP feature vector is obtained by concatenating a set of LBP feature vectors which are squentially extracted from each block in the image region
 * In case of sliding window search, to avoid repetitive computation, buffer is used. That is, LBP histogram for each block is saved and looked up.
 */
class MeanshiftDetector : public LBPDetector
{
private:
	/**
	 * Disable the default empty constructor since I still don't need it
	 */
	LBPDETECTOR_API MeanshiftDetector(void);
	/**
	 * Disable the default copy constructor since I still don't need it
	 */
	LBPDETECTOR_API MeanshiftDetector(const MeanshiftDetector&);
	/**
	 * Disable the default assignment operator since I still don't need it
	 */
	LBPDETECTOR_API MeanshiftDetector& operator = (const MeanshiftDetector&);
public:
	/**
	 * Constructor
	 */
	LBPDETECTOR_API MeanshiftDetector(const IplImage *image, int width, int height, int blockWidth, int blockHeight,
		int strideWidth, int strideHeight, int padWidth, int padHeight)
		:LBPDetector(width, height,blockWidth, blockHeight, strideWidth, strideHeight, padWidth, padHeight)
	{
		ulbp = new IntegralULBPHist((IplImage*)image);
		dlbp = new DenseLBP(ulbp, width, height,blockWidth, blockHeight, strideWidth, strideHeight);
	}

	/**
	 * Destructor
	 */
	LBPDETECTOR_API virtual ~MeanshiftDetector(void) {delete ulbp;}
	/**
	 * Compute detection score
	 */
	LBPDETECTOR_API float Meanshift(const IplImage *image, int bandwidth, const CvRect &rect, float& deltaX, float& deltaY);
	/**
	 *  
	 */
	LBPDETECTOR_API float ComputeScore(const IplImage *image, const CvRect &rect);
private:
	void InitPadHistogram();
private:
	ULBPHist *ulbp;
	DenseLBP *dlbp;
};

#endif