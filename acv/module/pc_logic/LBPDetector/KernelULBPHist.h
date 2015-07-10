#ifndef	_UNIFORMKERNELLBP_H
#define _UNIFORMKERNELLBP_H

#include "opencv/cvaux.h"
#include "opencv/cv.h"
#include "ULBPHist.h"

//TODO: figure out it's to use byte width or width for InitBuffer() and ResetBuffer()
//Fixed: Here we use the bytewith instead of width due to IplImage of opencv uses bytewidth to access each pixel


/**
 * To calculate the uniform kernel LBP feature give an image block
 * Here we use square root normilization according the HOG-LBP paper in CVPR2009
 */
class KernelULBPHist:public ULBPHist
{
private:
	/**
	 Disable the default empty constructor since I still don't need it
	 */
	KernelULBPHist(void);
	/**
	 Disable the default copy constructor since I still don't need it
	 */
	KernelULBPHist(const KernelULBPHist&);
	/**
	 Disable the default assignment operator since I still don't need it
	 */
	KernelULBPHist& operator = (const KernelULBPHist&);
public:
	LBPDETECTOR_API KernelULBPHist( IplImage* image, int interpolate = 1)
		:ULBPHist(image, interpolate), m_kernelTable(0), m_blockWidth(0), m_blockHeight(0)
	{
		ResetBuffer(image);
	};
	LBPDETECTOR_API virtual ~KernelULBPHist(void) {
		if (m_kernelTable) delete []m_kernelTable; m_kernelTable =  0;
	}
	/**
	 * Compute the histogram of uniform LBP in a sub window of an image
	 *
	 * @param image The input image
	 * @param roi   The region of interest, the width and height should be consistent with the blockWidth and blockHeight
	 * @param result The output
	 * @param interpolated Whether to use interpolation
	 * @param useBuffer Whether to use buffer during the computation. If a pixels' LBP pattern is never computed, it would be computed
	 * and saved in a buffer for further looking up. Otherwise, it would be looked up in the buffer to avoid repetitive computation.
	 */
	LBPDETECTOR_API void Compute(const IplImage* image, const CvRect& roi, bool useBuffer = true);
	/**
	 * Normalize the uniform LBP histogram by square root normalization
	 */
	LBPDETECTOR_API virtual void Normalize(void);
protected:
	/**
	 * The width and height of the image block and the whole image
	 */	
	int m_blockWidth, m_blockHeight;
	/**
	 * A lookup table which contains the kernel values for differnt postions in a block, in order to avoid repetitive computation
	 */	
	float *m_kernelTable;
	/**
	 * The sum of all kernel value, to accelerate the computation
	 */
	float m_kernelSum;		
private:
	/**
	 * Initialize the weight table
	 */
	void InitWeightTable(void);	
	
};

#endif