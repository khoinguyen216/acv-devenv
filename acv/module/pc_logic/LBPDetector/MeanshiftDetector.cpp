#include "MeanShiftDetector.h"
#include <algorithm>

float MeanshiftDetector::Meanshift(const IplImage *image, int bandwidth, const CvRect &rect, float& deltaX, float& deltaY)
{
	float numerX, numerY, deno;
	numerX = numerY = deno = 0;
	float cx = (bandwidth*this->m_blockWidth - 1.0f)/2.0f, 
		cy = (bandwidth*this->m_blockHeight - 1.0f)/2.0f, dx, dy;

	int nBlocksX = (m_width - m_blockWidth) / m_strideWidth + 1;	
	int nBlocksY = (m_height - m_blockHeight) / m_strideHeight + 1;

	std::vector<float> detectorPart(ULBPHist::m_nBins);
	int idxBlock = 0;
	CvSize blockNum;
	blockNum = DenseLBP::ComputeBlockNum(m_width, m_height, m_blockWidth, m_blockHeight, m_strideWidth, m_strideHeight);
	float score = 0.0f;
	float weight[ULBPHist::m_nBins];

	for (int i = 0; i < blockNum.height; i++) {
		for (int j = 0; j < blockNum.width; j++) {
			memset(weight, 0, ULBPHist::m_nBins*sizeof(float));
			idxBlock = i*blockNum.width + j;
			CvRect rect1 = cvRect(rect.x + j*m_strideWidth, rect.y + i*m_strideHeight, m_blockWidth, m_blockHeight);
			
			int l, r, t, b;
			l = rect1.x - (bandwidth - 1)*m_blockWidth/2.0f;
			t = rect1.y - (bandwidth - 1)*m_blockHeight/2.0f;
			//r = l + m_blockWidth*bandwidth - 1;
			//b = t + m_blockHeight*bandwidth - 1;
			
			CvRect rectBand = cvRect(l, t, m_blockWidth*bandwidth, m_blockHeight*bandwidth);
			copy(this->detector.begin()+idxBlock*ULBPHist::m_nBins, this->detector.begin()+(idxBlock+1)*ULBPHist::m_nBins, detectorPart.begin());
			
			ulbp->Compute(image, rectBand);
			ulbp->Normalize();

			/// To be optimized by SSE
			int k;
			for(k = 0; k < (ULBPHist::m_nBins/4)*4; k+=4) {
				score += detectorPart[k] * (ulbp->GetData())[k];
				score += detectorPart[k+1] * (ulbp->GetData())[k+1];
				score += detectorPart[k+2] * (ulbp->GetData())[k+2];
				score += detectorPart[k+3] * (ulbp->GetData())[k+3];

				//if ( abs(ulbp->GetData()[k]) > 1e-5)
				weight[k] = detectorPart[k] / ulbp->GetData()[k] + 1e-4;
				weight[k+1] = detectorPart[k+1] / ulbp->GetData()[k+1] + 1e-4;
				weight[k+2] = detectorPart[k+2] / ulbp->GetData()[k+2] + 1e-4;
				weight[k+3] = detectorPart[k+3] / ulbp->GetData()[k+3] + 1e-4;
			}

			for(; k < ULBPHist::m_nBins; k++) {
				score += detectorPart[k] * (ulbp->GetData())[k];
				weight[k] = detectorPart[k] / ulbp->GetData()[k] + 1e-4;
			}

			/////// To be optimized by SSE
			for (int y = 0; y < rectBand.height; y++) {
				for (int x = 0; x <rectBand.width; x++) {
					int idxPixel = (rectBand.y + y) * image->widthStep + rectBand.x + x;
					char bin;
					if (idxPixel < 0 || idxPixel > image->widthStep * image->height)
						bin = 58;
					else
						bin = ulbp->GetLBP()[idxPixel];
					dx = x - cx; dy = y - cy;

					float coe = weight[bin];

					numerX += dx * coe;
					numerY += dy * coe;
					deno += coe;
				}
			}
		}
	}

	deltaX = numerX / deno;
	deltaY = numerY / deno;	

	score -= this->rhoDetector;

	return score;
}

float MeanshiftDetector::ComputeScore(const IplImage *image, const CvRect &rect)
{
	float score = 0.0f;
	//DenseLBP dlbp((IplImage*) image, m_width, m_height, m_blockWidth, m_blockHeight, m_strideWidth, m_strideHeight);
	dlbp->ExtractDenseLBP(image, rect);
	float *histogram = dlbp->GetDenseLBPFeature();
	int k;

	for (k = 0; k < (dlbp->GetDimension()/4)*4; k += 4)
	{
		score += histogram[k] * detector[k] 
			+ histogram[k+1] * detector[k+1]
			+ histogram[k+2] * detector[k+2]
			+ histogram[k+3] * detector[k+3];
	}

	for(; k < dlbp->GetDimension(); k++ )
	{
		score += histogram[k] * detector[k];
	}
	score -= this->rhoDetector;
	return score;

}
