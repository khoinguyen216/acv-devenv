#ifndef _BLOCKDENSELBP_H
#define _BLOCKDENSELBP_H
/*
 * Created in 2009 in Shanghai. Modified on 2013-10-22. WANG Fanglin
 *
 * To retrieve dense LBP feature with block histogram buffer.
 * That is, histogram of each block is buffered after computing to avoid duplicated computation.
 * NOTE: it only works when the sliding window step size equals to the stride width of dense LBP.
 */


#include "KernelULBPHist.h"
#include "IntegralULBPHist.h"

#include "DenseLBP.h"

#include "ExportDLBP.h"

// TODO: nums_c might be wrong in ExtractDenseLBP(IplImage *image, CvRect roi)
/**
 * Extract the dense LBP feature from an image region
 * A dense LBP feature vector is obtained by concatenating a set of LBP feature vectors which are squentially extracted from each block in the image region
 * In case of sliding window search, to avoid repetitive computation, buffer is used. That is, LBP histogram for each block is saved and looked up.
 */
class BlockDenseLBP : public DenseLBP
{
private:
	/**
	 * Disable the default empty constructor since I still don't need it
	 */
	LBPDETECTOR_API BlockDenseLBP(void);
	/**
	 * Disable the default copy constructor since I still don't need it
	 */
	LBPDETECTOR_API BlockDenseLBP(const BlockDenseLBP&);
	/**
	 * Disable the default assignment operator since I still don't need it
	 */
	LBPDETECTOR_API BlockDenseLBP& operator = (const BlockDenseLBP&);
public:
	/**
	 * Constructor
	 */
	LBPDETECTOR_API BlockDenseLBP(IplImage *image, int width, int height, int blockWidth, int blockHeight,
		int strideWidth, int strideHeight, int padWidth, int padHeight);
	/**
	 * Destructor
	 */
	LBPDETECTOR_API ~BlockDenseLBP(void);
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
private:
	void InitPadHistogram();
private:
	int m_padWidth, m_padHeight;
	/**
	 * The number of block buffers
	 */
	int m_nBuffers;
	/**
	 * The hitogram data buffer of all blocks
	 */
	float *m_histBuffer;
	/**
	 * The hitogram label buffer which record whether a block has already been calculated or not
	 */
	int *m_labelBuffer;

};

#endif
