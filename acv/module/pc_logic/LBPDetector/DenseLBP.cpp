#include "DenseLBP.h"

DenseLBP::DenseLBP(IplImage *image, int width, int height, int blockWidth, int blockHeight, int strideWidth, int strideHeight)
	:m_width(width), m_height(height), m_blockWidth(blockWidth),
	m_blockHeight(blockHeight), m_strideWidth(strideWidth), m_strideHeight(strideHeight)
{
	m_ulbp = new KernelULBPHist(image);
	//m_ulbp = new IntegralULBPHist(image);	
	/*m_nBlocksX = (m_width - m_blockWidth) / m_strideWidth + 1;	
	m_nBlocksY = (m_height - m_blockHeight) / m_strideHeight + 1;*/
	CvSize blockNum = ComputeBlockNum(m_width, m_height, m_blockWidth, m_blockHeight, m_strideWidth, m_strideHeight);
	m_nBlocksX = blockNum.width; m_nBlocksY = blockNum.height;
	m_dimension = m_nBlocksX * m_nBlocksY * ULBPHist::GetNumberOfBins();
	m_denseHistogram = new float[m_dimension];	
	InitPadHistogram();
}

DenseLBP::DenseLBP(ULBPHist *ulbp, int width, int height, int blockWidth, int blockHeight, int strideWidth, int strideHeight)
	:m_ulbp(ulbp), m_width(width), m_height(height), m_blockWidth(blockWidth),
	m_blockHeight(blockHeight), m_strideWidth(strideWidth), m_strideHeight(strideHeight)

{
	CvSize blockNum = ComputeBlockNum(m_width, m_height, m_blockWidth, m_blockHeight, m_strideWidth, m_strideHeight);
	m_nBlocksX = blockNum.width; m_nBlocksY = blockNum.height;
	m_dimension = m_nBlocksX * m_nBlocksY * ULBPHist::GetNumberOfBins();
	m_denseHistogram = new float[m_dimension];	
	InitPadHistogram();
}

DenseLBP::~DenseLBP(void)
{
	if (m_denseHistogram) delete []m_denseHistogram;
	if (padHistogram) delete []padHistogram;
	if (m_ulbp) delete m_ulbp;
	m_denseHistogram = 0;
	padHistogram = 0;
	m_ulbp = 0;
}

void DenseLBP::ResetBuffer(const IplImage *image)
{
	m_ulbp->ResetBuffer(image);	
}

void DenseLBP::InitPadHistogram()
{
	padHistogram = new float[m_ulbp->GetNumberOfBins()];
	memset(padHistogram, 0, m_ulbp->GetNumberOfBins()*sizeof(float));
	padHistogram[0] = 1.;
}

/**
 * Extract the dense LBP feature on a rectangular region of an image
 * @param image The input image
 * @param rect The specified region
 *
 * It's safe when pxiels lie outside of image
 */
void DenseLBP::ExtractDenseLBP(const IplImage *image, CvRect rect)
{
	assert(image->depth == 8 && image->nChannels == 1);

	int i, j;

	// This value 
	int nums_c = (image->width+0.0f)/m_strideWidth;

	int nBins = m_ulbp->GetNumberOfBins();

	int idx = 0;
	for (i = 0; i < m_nBlocksY; i++) {
		for (j = 0; j < m_nBlocksX;j++) {
			CvRect rect1;
			rect1 = cvRect(rect.x + j*m_strideWidth, rect.y + i*m_strideHeight, m_blockWidth, m_blockHeight);
			int idx_block = nums_c*((rect1.y)/m_strideHeight) + (rect1.x)/m_strideWidth;
			m_ulbp->Compute(image, rect1);
			m_ulbp->Normalize();							
			memcpy(m_denseHistogram + idx*nBins, m_ulbp->GetData(), nBins*sizeof(float));
			++idx;
		}
	}
}

 