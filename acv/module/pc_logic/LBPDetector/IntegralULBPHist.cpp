#include "IntegralULBPHist.h"
#include <iostream>
#include "opencv2/opencv.hpp"

using namespace std;

void IntegralULBPHist::ComputeIntegralHist(const IplImage *image)
{
	assert(image->nChannels == 1);
	// To assure the buffer size is not small than the image size
	int columns = image->width;
	int rows = image->height;

	float rowIntHist[this->m_nBins];

	int r,c;	

	int idx = 0, idx1;
	
	for (r=0;r<image->height;r++)
	{
		memset(rowIntHist, 0, this->m_nBins*sizeof(float));
		for (c=0;c<image->width;c++)
		{
			idx = r * image->width + c;
			idx1 = r * image->widthStep + c;	
			++rowIntHist[m_buffer[idx1]];
			float *upperIntHist = 0, *intHist = 0;
			intHist = m_integralHist + idx*this->m_nBins;
			if (r > 0) {
				int idx_upper = (r -1)*columns + c;
				upperIntHist = m_integralHist + idx_upper*this->m_nBins;
				AddTwoHist(intHist, upperIntHist, rowIntHist);
			} else if (r == 0) {
				memcpy(intHist, rowIntHist, this->m_nBins*sizeof(float));
			}				
		}
	}
	
}

/*
 * For the pixels beyond the image range, their LBP patterns are set to 58
 *
 */
void IntegralULBPHist::Compute(const IplImage* image, const CvRect& roi,bool useBuffer)
{

	if (!useBuffer)
		this->ResetBuffer(image);

	// Intialize histogram
	memset(m_histogram, 0, this->m_nBins*sizeof(m_histogram[0]));
	int columns = image->width;
	int rows = image->height;
	
	int nROIPixels = roi.width * roi.height;

	cv::Rect Rect(roi);
	cv::Rect imgRect(0, 0, image->width, image->height);
	cv::Rect intersect = Rect & imgRect;
	
	// If there is no intersection between the roi and the image
	if (intersect.width <= 0 || intersect.height <= 0){
		this->m_histogram[58] = roi.width * roi.height;
	} else {
		// If there is intersection
		float *h1, *h2, *h3, *h4;
		int x1, x2, y1, y2, idx1, idx2, idx3, idx4;
		x1 = intersect.x; x2 = intersect.x + intersect.width - 1; y1 = intersect.y; y2 = intersect.y + intersect.height - 1;
		
		idx1 = (y1-1) * columns + x1-1; idx2 = (y1-1) * columns + x2; idx3 = y2 * columns + x1-1; idx4 = y2 * columns + x2;
		h1 = m_integralHist + idx1*this->m_nBins;
		h2 = m_integralHist + idx2*this->m_nBins;
		h3 = m_integralHist + idx3*this->m_nBins;
		h4 = m_integralHist + idx4*this->m_nBins;

		// = h4
		if (x1 == 0 && y1 == 0)
		{
			memcpy(this->m_histogram, h4, sizeof(float)*ULBPHist::m_nBins); 
		}
		else if (x1 == 0) 
		{
			//h4 - h2
			MinusTwoHist(this->m_histogram, h4, h2);
		} else if (y1 == 0) 
		{
			//h4 - h3
			MinusTwoHist(this->m_histogram, h4, h3);
		} else {
			//(h1 + h4) - (h2 + h3)
			AddTwoHist(this->m_histogram, h1, h4);
			MinusTwoHist(this->m_histogram, this->m_histogram, h2);
			MinusTwoHist(this->m_histogram, this->m_histogram, h3);
		}

		// The out-of-range pixels are give LBP pattern 58
		int nOutterPixels = nROIPixels - intersect.width * intersect.height;
		this->m_histogram[58] += nOutterPixels;
	}
}

void IntegralULBPHist::Normalize()
{
	float sum = 0.0;
	
	// TODO: SSE optimiazation
	for (int i = 0; i < 59; i++)
		sum += m_histogram[i];

	__m128 a, b, c, d;
	float ch[4] = {sum, sum, sum, sum};
	a = _mm_loadu_ps(ch);
	
	for (int i = 0; i < 56; i += 4)
	{
		b = _mm_loadu_ps(m_histogram + i);
		c = _mm_div_ps(b, a);
		d = _mm_sqrt_ps(c);
		_mm_storeu_ps(m_histogram + i, d);
	}

	for (int i = 56; i < 59; i++)
		m_histogram[i] = sqrt(m_histogram[i] / sum);
}