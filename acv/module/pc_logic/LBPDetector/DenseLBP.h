#ifndef _DENSELBP_H
#define _DENSELBP_H

#include "ULBPHist.h"
#include "KernelULBPHist.h"
#include "IntegralULBPHist.h"

#include "ExportDLBP.h"

// TODO: nums_c might be wrong in ExtractDenseLBP(IplImage *image, CvRect roi)
/**
 * Extract the dense LBP feature from an image region
 * A dense LBP feature vector is obtained by concatenating a set of LBP feature vectors which are squentially extracted from each block in the image region
 * In case of sliding window search, to avoid repetitive computation, buffer is used. That is, LBP histogram for each block is saved and looked up.
 */

class DenseLBP{
private:
	/**
	 * Disable the default empty constructor since I still don't need it
	 */
	LBPDETECTOR_API DenseLBP(void);
	/**
	 * Disable the default copy constructor since I still don't need it
	 */
	LBPDETECTOR_API DenseLBP(const DenseLBP&);
	/**
	 * Disable the default assignment operator since I still don't need it
	 */
	LBPDETECTOR_API DenseLBP& operator = (const DenseLBP&);
public:
	LBPDETECTOR_API static CvSize ComputeBlockNum(int width, int height, int blockWidth, int blockHeight, int strideWidth, int strideHeight)
	{
		int nBlocksX = (width - blockWidth) / strideWidth + 1;	
		int nBlocksY = (height - blockHeight) / strideHeight + 1;
		return cvSize(nBlocksX, nBlocksY);
	}
	/**
	 * Constructor
	 */
	LBPDETECTOR_API DenseLBP(IplImage *image, int width, int height, int blockWidth, int blockHeight,
		int strideWidth, int strideHeight);

	LBPDETECTOR_API DenseLBP(ULBPHist *ulbp, int width, int height, int blockWidth, int blockHeight,
		int strideWidth, int strideHeight);

	/**
	 * Destructor
	 */
	LBPDETECTOR_API virtual ~DenseLBP(void);
	/**
	 * Reset the buffer for histogram, which happens when changing the input image
	 * Actually, this function only resets the label buffer since which decides whether to use the histogram buffer
	 */
	LBPDETECTOR_API virtual void ResetBuffer(const IplImage *image);
	/**
	 * Extract the dense LBP feature on a rectangular region of an image
	 * @param image The input image
	 * @param rect The specified region
	 */
	LBPDETECTOR_API virtual void ExtractDenseLBP(const IplImage *image, CvRect rect);
	LBPDETECTOR_API const char* GetLBP(void) const {return m_ulbp->GetLBP();}
	/**
	 * Get the histogram result
	 */
	LBPDETECTOR_API float* GetDenseLBPFeature(void) const {	return m_denseHistogram;}
	/**
	 * Get the dimension of histogram
	 */
	LBPDETECTOR_API int GetDimension(void) const { return m_dimension;}
	/**
	 * Get window size
	 */
	LBPDETECTOR_API CvSize GetWindowSize(void) const { return cvSize(m_width, m_height); }
	/**
	 * Get stride size
	 */
	LBPDETECTOR_API CvSize GetStrideSize(void) const { return cvSize(m_strideWidth, m_strideHeight); }
	/**
	 * Get image size
	 */
	//LBPDETECTOR_API CvSize GetImageSize(void) const { return cvSize(m_imageWidth, m_imageHeight); }
	LBPDETECTOR_API CvSize GetBlockNum(void) const { return cvSize(m_nBlocksX, m_nBlocksY); }
private:
	void InitPadHistogram();
protected:
	/**
	 * The size of the whole image, image window, image block, stride and pad
	 */
	//int m_imageWidth, m_imageHeight;
	int m_width, m_height;
	int m_blockWidth, m_blockHeight;
	int m_strideWidth, m_strideHeight;
	/**
	 * The number of blocks along x, y direction in the image window
	 */
	int m_nBlocksX, m_nBlocksY;	
	/**
	 * The dimension of the whole dense histogram vector
	 */
	int m_dimension;
	/**
	 * The dense histogram vector
	 */
	float *m_denseHistogram;
protected:
	/**
	 * The histogram of a pad block.
	 * NOTICE: we assume there is only one pad pattern, that is {1 0 ....}. It's reansonable
	 */	
	float *padHistogram;
protected:
	/**
	 * The uniform kernel LBP object, the calculation of histogram for each block is hidden in class "UniformKernelLBP"
	 * NOTICE: we assume there is only one pad pattern, that is {1 0 ....}. It's reansonable
	 */
	ULBPHist *m_ulbp;

};

#endif