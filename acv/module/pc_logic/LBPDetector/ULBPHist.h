/*
 * 2013-10-20, WANG Fanglin
 *
 * To compute the intrgral histogram of uniform LBP
 */
#ifndef _ULBPHIST_WFL_
#define _ULBPHIST_WFL_

#include "LBPUtils.h"
#include "opencv/cv.h"
#include "ExportDLBP.h"

#include <string>
#include <fstream>

using namespace std;

class ULBPHist
{
public:
	//LBPDETECTOR_API ULBPHist( const IplImage* image, int blockWidth, int blockHeight, int interpolate = 1)
	//	:m_blockWidth(blockWidth), m_blockHeight(blockHeight), m_nBits(8), m_buffer(0),m_interpolate(interpolate)
	//{
	//	//m_imageByteWidth = m_imageWidth % 4 == 0 ? m_imageWidth : (4 - m_imageWidth % 4 + m_imageWidth);
	//	InitMappingTable();
	//	InitNeighborhoodTable();
	//	ResetImageConfiguration(image);
	//}


	LBPDETECTOR_API ULBPHist( IplImage* image, int interpolate = 1):m_interpolate(interpolate), m_nBits(8), m_buffer(0)
	{
		//m_imageByteWidth = m_imageWidth % 4 == 0 ? m_imageWidth : (4 - m_imageWidth % 4 + m_imageWidth);
		InitMappingTable();
		InitNeighborhoodTable();
		m_image = image;
		m_buffer = new char[image->widthStep * image->height];
		m_image = (IplImage *)image;
	}
	/**
	 * Get the histogram data pointer
	 */
	const float* GetData(void) const
	{
		return m_histogram;
	}

	/**
	 * Get the histogram data pointer
	 */
	const char* GetLBP(void) const
	{
		return m_buffer;
	}

	/**
	 * Get the number of bins of histogram
	 */
	static int GetNumberOfBins(void);

	/**
	 * Write calculated histogram to a fiel
	 */
	LBPDETECTOR_API void Write2File(string filepath)
	{
		ofstream file(filepath);

		float sum = 0;
		for (int i = 0; i < m_nBins; i++) {
			file << i << ":" << m_histogram[i] << ",";
			sum += m_histogram[i];
		}
		file << sum;
		file.close();
	}

	virtual ~ULBPHist() {if (m_buffer) delete []m_buffer; m_buffer = 0;}
private:
	/**
	 * Initialize the mapping table which contains the mapping of a standard LBP pattern to its corresponding unifrom pattern
	 */
	void InitMappingTable(void);
	/**
	 * Calculate the point coordinates for circular sampling of the neighborhood
	 */
	void InitNeighborhoodTable(void);
public:
	/**
	 * The number of bins in a uniform LBP histogram. It's constantly 59
	 */
	static const int m_nBins = 59;

	/**
	 *
	 */
	/*LBPDETECTOR_API void SetBlockSize(int blockWidth, int blockHeight)
	{
		m_blockWidth =  blockWidth; m_blockHeight = blockHeight;
	}*/
public:
	//void ComputeLBPHistogram(const IplImage* image, const CvRect& roi, int interpolated = 1, bool useBuffer = true);
protected:

	/**
	 * The histogram data. Here we use float rather than double is because float type can be speeded up by SSE instructions
	 */
	float m_histogram[m_nBins];
	/**
	 * A lookup talbe which contains the mapping of a standard LBP pattern to its corresponding unifrom pattern
	 */
	int m_mappingTable[256];
	/**
	 * The number of bits for the LBP pattern
	 */
	int m_nBits;
	/**
	 * The width and height of the image block and the whole image
	 */
	int m_blockWidth, m_blockHeight;
	/*int m_imageWidth, m_imageHeight, m_imageByteWidth;	*/
	/**
	 * The buffer which stores the LBP value of each pixel, in order to avoid repetitive computation
	 * The size of m_buffer is byteWidth * height rather than width*height
	 */
	char *m_buffer;
	/*
	 *	Whether to use interpolation during computing LBP
	 */
	int m_interpolate;

	/*
	 *	Whether to use interpolation during computing LBP
	 */
	IplImage *m_image;
protected:
	/*
	 * Compute the ULBP value for image pixels
	 */
	void ComputeULBP(const IplImage* image);
	/*
	 * Compute the ULBP value for ROI pixels
	 */
	void ComputeULBP(const IplImage* image, const CvRect& roi);

public:
	virtual void Normalize() = 0;
	virtual void Compute(const IplImage* image, const CvRect& roi, bool useBuffer = true) = 0;
	/**
	 * Reset the buffer, it's necessary when switching to another image
	 */
	virtual void ResetBuffer(const IplImage *image);
	/*void ResetImageConfiguration(const IplImage* image)
	{
		m_imageWidth = image->width; m_imageHeight = image->height; m_imageByteWidth = image->widthStep;
	}*/
};
#endif
