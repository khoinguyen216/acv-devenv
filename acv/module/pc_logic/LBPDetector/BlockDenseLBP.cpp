#include "BlockDenseLBP.h"

BlockDenseLBP::BlockDenseLBP(IplImage *image, int width, int height, int blockWidth, int blockHeight, int strideWidth, int strideHeight, int padWidth, int padHeight)
	:DenseLBP(image, width, height, blockWidth, blockHeight, strideWidth, strideHeight), m_padWidth(padWidth), m_padHeight(padHeight)
{
	m_nBuffers = image->width * image->height / (m_strideWidth * m_strideHeight);
	m_histBuffer = new float[m_nBuffers * KernelULBPHist::GetNumberOfBins() + 2];
	m_labelBuffer = new int[m_nBuffers + 2];
	memset(m_labelBuffer, 0, (m_nBuffers + 2)*sizeof(int));
}

BlockDenseLBP::~BlockDenseLBP(void)
{
	delete []m_histBuffer;
	delete []m_labelBuffer;
}

void BlockDenseLBP::ResetBuffer(const IplImage *image)
{
	DenseLBP::ResetBuffer(image);
	memset(m_labelBuffer, 0, (m_nBuffers + 2)*sizeof(int));
}

void BlockDenseLBP::ExtractDenseLBP(const IplImage *image, CvRect roi)
{
	assert(image->depth == 8 && image->nChannels == 1);

	int i, j;

	// This value 
	int nums_c = (image->width+0.0f)/m_strideWidth;

	int nBins = m_ulbp->GetNumberOfBins();

	int idx = 0;
	for (i = 0; i < m_nBlocksY; i++) {
		for (j = 0; j < m_nBlocksX;j++) {
			CvRect rect;
			//int idx = i * cols_h + j;

			rect = cvRect(roi.x + j*m_strideWidth, roi.y + i*m_strideHeight, m_blockWidth, m_blockHeight);

			//扩充图像不必计算直方图，直接取为常数值
			if (rect.x < 0 || rect.y < 0 || rect.x + rect.width - 1 > image->width || rect.y + rect.height - 1 > image->height)
			{
				memcpy(m_denseHistogram + idx*nBins, padHistogram, nBins*sizeof(float));	
			}
			else
			{
				int idx_block = nums_c*((rect.y)/m_strideHeight) + (rect.x)/m_strideWidth;

				if (m_labelBuffer[idx_block] == 0)
				{
					m_ulbp->Compute(image, rect);
					m_ulbp->Normalize();
					memcpy(m_histBuffer+idx_block*nBins, m_ulbp->GetData(), nBins*sizeof(float));
					m_labelBuffer[idx_block] = 1;
				}				
				memcpy(m_denseHistogram + idx*nBins, m_histBuffer+idx_block*nBins, nBins*sizeof(float));
			}
			++idx;
		}
	}
}

 