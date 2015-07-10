/*
 * 2013-10-20, WANG Fanglin
 *
 * To compute the intrgral histogram of uniform LBP
 */
#ifndef _INTEGRALULBPHIST_WFL_
#define _INTEGRALULBPHIST_WFL_

#include "opencv/cv.h"
#include "ULBPHist.h"
#include "ExportDLBP.h"

class IntegralULBPHist:public ULBPHist
{
private:
	/**
	 Disable the default copy constructor since I still don't need it
	 */
	IntegralULBPHist(const IntegralULBPHist&);
	/**
	 Disable the default assignment operator since I still don't need it
	 */
	IntegralULBPHist& operator = (const IntegralULBPHist&);

public:
	LBPDETECTOR_API IntegralULBPHist( IplImage* image, int interpolate = 1)
		:ULBPHist(image, interpolate),m_integralHist(0)
	{
		ResetBuffer(image);
	}
		
	/**
	 * Compute the histogram of uniform LBP in a sub window of an image
	 *
	 * @param image The input image
	 * @param roi   The region of interest, the width and height should be consistent with the blockWidth and blockHeight
	 * @param result The output
	 * @param interpolated Whether to use interpolation
	 * @param useBuffer Whether to use buffer during the computation. If the integral histogram has already been calculated it should be set true, and false otherwise.
	 * and saved in a buffer for further looking up. Otherwise, it would be looked up in the buffer to avoid repetitive computation.
	 */
	LBPDETECTOR_API virtual void Compute(const IplImage* image, const CvRect& roi, bool useBuffer = true);
	/**
	 * Normalize the uniform LBP histogram by square root normalization
	 */
	LBPDETECTOR_API virtual void Normalize(void);

	 /**
	  * 
	  */
	LBPDETECTOR_API virtual void ResetBuffer(const IplImage *image)
	{
		ULBPHist::ResetBuffer(image);
		if (m_integralHist) {
			delete []m_integralHist;
			m_integralHist = 0;
		}
		try {
			m_integralHist = new float[image->width*image->height*this->m_nBins];
		} catch (...) {
			cout << "Image size is " << image->width << " x " << image->height << endl;
			cout << "Memory allocation failed maybe due to the image size is too large!" << endl;
			exit(0);
		}
		memset(m_integralHist, 0, image->width*image->height*this->m_nBins*sizeof(float));
		ComputeIntegralHist(image);
	}

	LBPDETECTOR_API ~IntegralULBPHist()
	{
		delete []m_integralHist;
	}

private:
	void ComputeIntegralHist(const IplImage *image);

	void AddTwoHist(float *out, const float *in1, const float *in2) {
		/*__m128 a, b, c;
		for (int i = 0; i < 56; i += 4)
		{
			a = _mm_loadu_ps(in1 + i);
			b = _mm_loadu_ps(in2 + i);
			c = _mm_sqrt_ps(c);
			_mm_storeu_ps(m_histogram + i, d);
		}
		for (int i = 56; i < 59; i++)*/
		for (int i = 0; i < 56; i++)
			out[i] = in1[i] + in2[i];
		for (int i = 56; i < this->m_nBins; i++)
			out[i] = in1[i] + in2[i];
	}

	void MinusTwoHist(float *out, const float *in1, const float *in2) {
		/*__m128 a, b, c;
		for (int i = 0; i < 56; i += 4)
		{
			a = _mm_loadu_ps(in1 + i);
			b = _mm_loadu_ps(in2 + i);
			c = _mm_sqrt_ps(c);
			_mm_storeu_ps(m_histogram + i, d);
		}
		for (int i = 56; i < 59; i++)*/
		for (int i = 0; i < 56; i++)
			out[i] = in1[i] - in2[i];
		for (int i = 56; i < this->m_nBins; i++)
			out[i] = in1[i] - in2[i];
	}
	
private:
	float *m_integralHist;	
};


#endif
