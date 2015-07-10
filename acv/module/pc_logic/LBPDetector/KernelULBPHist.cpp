#include "KernelULBPHist.h"

void KernelULBPHist::InitWeightTable(void)
{
	if (m_kernelTable) delete []m_kernelTable;

	int r, c, idx;
	float cx, cy, x, y, dx, dy, kernel;
	cx = (m_blockWidth-1)/2.0f;
	cy = (m_blockHeight-1)/2.0f;	

	m_kernelTable = new float[m_blockHeight * m_blockWidth];

	float sigma = (m_blockWidth + m_blockHeight) / 2.0f;
	float scale = 1.0f / (sigma*sigma*2.0f);
	m_kernelSum = 0.0f;	

	for (y = 0; y < m_blockHeight; y++) 	{
		for (x = 0; x < m_blockWidth; x++) {
			idx = y*m_blockWidth + x;
			dx = x - cx; dy = y - cy;
			kernel = exp(-(dx*dx + dy*dy)*scale);
			//kernel = 1;
			m_kernelTable[idx] = kernel;
			m_kernelSum += kernel;
		}
	}
}

void KernelULBPHist::Compute(const IplImage* image, const CvRect& roi, bool useBuffer)
{
	if (m_blockWidth != roi.width || m_blockHeight != roi.height) {
		m_blockWidth = roi.width; m_blockHeight = roi.height;
		InitWeightTable();
	}
	memset(m_histogram, 0, sizeof(float)*m_nBins); /* Clear result histogram */

	assert(m_blockWidth == roi.width && m_blockHeight == roi.height);
	// To assure the buffer size is not small than the image size
	assert(image->depth == 8 && image->nChannels == 1);
	int columns = image->widthStep;
	int rows = roi.height;
	int r,c;
	int nPixels = image->widthStep * image->height;
	int idx = 0, idx_buffer;
	
	/*
	 * For the pixels beyond the image range, their LBP patterns are set to 58
	 *
	 */
	for (r=0;r<roi.height;r++)
	{
		for (c=0;c<roi.width;c++)
		{
			idx_buffer = (roi.y + r) * columns + c + roi.x;		
			if (idx_buffer >= 0 && idx_buffer < nPixels)
				m_histogram[m_buffer[idx_buffer]] += m_kernelTable[idx]; /* Increase histogram bin value */	
			else //// The out-of-range pixels are give LBP pattern 58
				m_histogram[58] += m_kernelTable[idx]; /* Increase histogram bin value */	
			++idx;
		}
	}
	
}

void KernelULBPHist::Normalize()
{
	__m128 a, b, c, d;
	float ch[4] = {m_kernelSum, m_kernelSum, m_kernelSum, m_kernelSum};
	a = _mm_loadu_ps(ch);
	
	for (int i = 0; i < 56; i += 4)
	{
		b = _mm_loadu_ps(m_histogram + i);
		c = _mm_div_ps(b, a);
		d = _mm_sqrt_ps(c);
		_mm_storeu_ps(m_histogram + i, d);
	}

	for (int i = 56; i < 59; i++)
		m_histogram[i] = sqrt(m_histogram[i] / m_kernelSum);
}