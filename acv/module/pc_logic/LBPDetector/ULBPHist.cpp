#include "ULBPHist.h"
#include <math.h>

int ULBPHist::GetNumberOfBins()
{
	return m_nBins;
}

void ULBPHist::InitMappingTable(void)
{
	//非uniform的LBP模式被设为第59个bin
    for (int i = 0; i < 256; i++)
        m_mappingTable[i] = 58;
    for (int i = 0; i < 58; i++)
        m_mappingTable[uniformIndex[i]] = i;
}

void ULBPHist::InitNeighborhoodTable(void)
{
	double step = 2 * M_PI / BITS, tmpX, tmpY;
	int i;
	for (i=0;i<m_nBits;i++) {
		tmpX = PREDICATE * cos(i * step);
		tmpY = PREDICATE * sin(i * step);
		points[i].x = (int)tmpX;
		points[i].y = (int)tmpY;
		offsets[i].x = tmpX - points[i].x;
		offsets[i].y = tmpY - points[i].y;
		if (offsets[i].x < 1.0e-10 && offsets[i].x > -1.0e-10) /* rounding error */
			offsets[i].x = 0;
		if (offsets[i].y < 1.0e-10 && offsets[i].y > -1.0e-10) /* rounding error */
			offsets[i].y = 0;
		
		if (tmpX < 0 && offsets[i].x != 0)
			{
				points[i].x -= 1;
				offsets[i].x += 1;
			}
		if (tmpY < 0 && offsets[i].y != 0)
			{
				points[i].y -= 1;
				offsets[i].y += 1;
			}
	}
}

void ULBPHist::ResetBuffer(const IplImage *image)
{
	//Here we use the bytewith instead of width due to IplImage of opencv uses bytewidth to access each pixel
	if (m_buffer) delete []m_buffer; m_buffer = 0;
	m_buffer = new char[image->widthStep * image->height];	
	memset(m_buffer, -1, image->widthStep * image->height);	
	ComputeULBP(image);
}

void ULBPHist::ComputeULBP(const IplImage* image, const CvRect& roi)
{
	// To assure the buffer size is not small than the image size
	assert(image->depth == 8 && image->nChannels == 1);
	int columns = image->widthStep;
	int rows = roi.height;

	int leap = columns*PREDICATE;

	//tl：ROI上第一个点的左上相邻点的序号
	//int tl = (roi.x ) + (roi.y)*columns;
	int tl = (roi.x - 1) + (roi.y - 1)*columns;


	/*Set up a circularly indexed neighborhood using nine pointers.*/
    unsigned char	
		*p0 = (unsigned char*) image->imageData + tl,
		*p1 = p0 + PREDICATE,
		*p2 = p1 + PREDICATE,
		*p3 = p2 + leap,
		*p4 = p3 + leap,
		*p5 = p4 - PREDICATE,
		*p6 = p5 - PREDICATE,
		*p7 = p6 - leap,
		*center = p7 + PREDICATE;
	unsigned int value;
	int pred2 = PREDICATE << 1;
	pred2 = 0;
	pred2 += columns - roi.width;
	int r,c;

	memset(m_histogram, 0, sizeof(float)*m_nBins); /* Clear result histogram */

	int idx = 0, idx_buffer;
		
	if (!m_interpolate)
	{
		for (r=0;r<roi.height;r++)
		{
			for (c=0;c<roi.width;c++)
			{
				idx_buffer = (roi.y + r) * columns + c + roi.x;
				char tmp;
				if (m_buffer[idx_buffer] != -1)
				{
					p0++; p1++; p2++;
					p3++; p4++; p5++;
					p6++; p7++; center++;
					tmp = m_buffer[idx_buffer];
				}
				else
				{
					value = 0;

					/* Unrolled loop */
					COMPAB_MASK_INC(value, p1,1);
					COMPAB_MASK_INC(value, p3,3);
					COMPAB_MASK_INC(value, p5,5);
					COMPAB_MASK_INC(value, p7,7);

					/* Interpolate corner pixels */
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p0,5,columns)+0.5),0);
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p2,7,columns)+0.5),2);
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p4,1,columns)+0.5),4);
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p6,3,columns)+0.5),6);
					p0++;
					p2++;
					p4++;
					p6++;
					center++;
					tmp = m_mappingTable[value];
					m_buffer[idx_buffer] = tmp;
				}
				
				++idx;
			}
			p0 += pred2;
			p1 += pred2;
			p2 += pred2;
			p3 += pred2;
			p4 += pred2;
			p5 += pred2;
			p6 += pred2;
			p7 += pred2;
			center += pred2;
		}
	}
	else
	{
		p0 = center + points[5].x + points[5].y * columns;
		p2 = center + points[7].x + points[7].y * columns;
		p4 = center + points[1].x + points[1].y * columns;
		p6 = center + points[3].x + points[3].y * columns;
			
		for (r=0;r<roi.height;r++)
		{
			for (c=0;c<roi.width;c++)
			{
				idx_buffer = (roi.y + r) * columns + c + roi.x;
				char tmp = 0;
				
				if (m_buffer[idx_buffer] != -1)
				{
					p0++; p1++; p2++;
					p3++; p4++; p5++;
					p6++; p7++; center++;
					tmp = m_buffer[idx_buffer];
				}
				else
				{
					value = 0;

					/* Unrolled loop */
					COMPAB_MASK_INC(value, p1,1);
					COMPAB_MASK_INC(value, p3,3);
					COMPAB_MASK_INC(value, p5,5);
					COMPAB_MASK_INC(value, p7,7);

					/* Interpolate corner pixels */
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p0,5,columns)+0.5),0);
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p2,7,columns)+0.5),2);
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p4,1,columns)+0.5),4);
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p6,3,columns)+0.5),6);
					p0++;
					p2++;
					p4++;
					p6++;
					center++;
					tmp = m_mappingTable[value];
					m_buffer[idx_buffer] = tmp;
				}			
				
				++idx;
			}
			p0 += pred2;
			p1 += pred2;
			p2 += pred2;
			p3 += pred2;
			p4 += pred2;
			p5 += pred2;
			p6 += pred2;
			p7 += pred2;
			center += pred2;
		}
	}
}

void ULBPHist::ComputeULBP(const IplImage* image)
{
	assert(image->nChannels == 1);
	// To assure the buffer size is not small than the image size
	int columns = image->width;
	int rows = image->height;
	int leap = image->widthStep*PREDICATE;


	/*Set up a circularly indexed neighborhood using nine pointers.*/
    unsigned char	
		*p0 = (unsigned char*) image->imageData - image->widthStep - 1, /* Set the top left pixel at (-1, -1)*/
		*p1 = p0 + PREDICATE,
		*p2 = p1 + PREDICATE,
		*p3 = p2 + leap,
		*p4 = p3 + leap,
		*p5 = p4 - PREDICATE,
		*p6 = p5 - PREDICATE,
		*p7 = p6 - leap,
		*center = p7 + PREDICATE;
	unsigned int value;
	int pred2 = PREDICATE << 1;
	pred2 = image->widthStep - columns;
	//pred2 += columns - roi.width;
	int r,c;	

	int idx = 0, idx1 = 0;
	
	if (!m_interpolate)
	{
		for (r=0;r<rows;r++)
		{
			for (c=0;c<columns;c++)
			{
				idx = r * columns + c;
				idx1 = r * image->widthStep + c;	
				value = 0;

				// Set the image border pixels with LBP pattern of a constant value, i.e., pattern 58
				if (r == 0 || c == 0 || r == rows - 1 || c == columns - 1) 
				{
					value = 58; // The bin 58 of uniform LBP
					p0++; p1++; p2++;
					p3++; p4++; p5++;
					p6++; p7++; center++;
				} else { 
					/* Unrolled loop */
					COMPAB_MASK_INC(value, p1,1);
					COMPAB_MASK_INC(value, p3,3);
					COMPAB_MASK_INC(value, p5,5);
					COMPAB_MASK_INC(value, p7,7);

					/* Interpolate corner pixels */
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p0,5,image->widthStep)+0.5),0);
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p2,7,image->widthStep)+0.5),2);
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p4,1,image->widthStep)+0.5),4);
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p6,3,image->widthStep)+0.5),6);
					p0++;
					p2++;
					p4++;
					p6++;
					center++;
				}
				m_buffer[idx1] = m_mappingTable[value];				
			}
			p0 += pred2;
			p1 += pred2;
			p2 += pred2;
			p3 += pred2;
			p4 += pred2;
			p5 += pred2;
			p6 += pred2;
			p7 += pred2;
			center += pred2;
		}
	}
	else
	{
		p0 = center + points[5].x + points[5].y * image->widthStep;
		p2 = center + points[7].x + points[7].y * image->widthStep;
		p4 = center + points[1].x + points[1].y * image->widthStep;
		p6 = center + points[3].x + points[3].y * image->widthStep;
			
		for (r=0;r<rows;r++)
		{
			for (c=0;c<columns;c++)
			{
				idx = r * columns + c;
				idx1 = r * image->widthStep + c;	
								
				value = 0;

				// Set the image border pixels with LBP pattern of a constant value, i.e., bin 58
				if (r == 0 || c == 0 || r == rows - 1 || c == columns - 1) 
				{
					value = 58; // The bin 58 of uniform LBP
					p0++; p1++; p2++;
					p3++; p4++; p5++;
					p6++; p7++; center++;
				} else { 
					/* Unrolled loop */
					COMPAB_MASK_INC(value, p1,1);
					COMPAB_MASK_INC(value, p3,3);
					COMPAB_MASK_INC(value, p5,5);
					COMPAB_MASK_INC(value, p7,7);

					/* Interpolate corner pixels */
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p0,5,image->widthStep)+0.5),0);
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p2,7,image->widthStep)+0.5),2);
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p4,1,image->widthStep)+0.5),4);
					COMPAB_MASK(value, (unsigned char)(interpolate_at_ptr(p6,3,image->widthStep)+0.5),6);
					p0++;
					p2++;
					p4++;
					p6++;
					center++;
				}
				m_buffer[idx1] = m_mappingTable[value];				
			}
			p0 += pred2;
			p1 += pred2;
			p2 += pred2;
			p3 += pred2;
			p4 += pred2;
			p5 += pred2;
			p6 += pred2;
			p7 += pred2;
			center += pred2;
		}
	}
}