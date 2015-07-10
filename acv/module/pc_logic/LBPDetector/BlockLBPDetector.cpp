#include "BlockLBPDetector.h"

#include "LBPDetector.h"
#include  <functional>

#include "ThreadManager/ThreadManager.h"
using namespace BoatDetection;

static bool IsEffectiveRegion(IplImage *maskImage, CvRect rect)
{
	const int thresh = 100;
	int nZeros = 0;
	cvSetImageROI(maskImage, rect);
	/*for (int i = rect.y; i < rect.y + rect.height - 1; i++) {
		for (int j = rect.x; i < rect.x + rect.width - 1; i++) {
			uchar *data = (uchar *)maskImage->imageData + i*maskImage->widthStep + j;
			if (*data == 0)
				++nZeros;
		}
	}*/
	nZeros = rect.width*rect.height -cvCountNonZero(maskImage);
	
	cvResetImageROI(maskImage);
	if (nZeros >= thresh)
		return false;
	return true;
}

bool BlockLBPDetector::DetectObject(const IplImage *image, CvRect *resultRect, Orientation *orientation, float threshold, int groupThreshold, float scaleStep)
{
	bool result;
	FindAllCandidates(image, threshold, scaleStep);
	result = MergeCandidates(groupThreshold);
	if (result)	{
		int weight = objectWeights[0];
		int idxMax = 0;
		for (int i = 1; i < objectWeights.size(); i++)
			if (weight < objectWeights[i]) {
				idxMax = i;
				weight = objectWeights[i];
			}
		*orientation = objectLabels[idxMax];
		*resultRect = objectLocations[idxMax];
	}
	return result;
}

bool BlockLBPDetector::DetectObjects(const IplImage *image, std::vector<CvRect>& resultRect, std::vector<Orientation> *orientation, float threshold, int groupThreshold, float scaleStep, IplImage *maskImage)
{
	bool result;
	//FindAllCandidates_aspectratio(image, threshold, scaleStep, maskRects);
	FindAllCandidates(image, threshold, scaleStep, maskImage);

	result = MergeCandidates(groupThreshold);
	if (result) {
		if (orientation != 0) *orientation = objectLabels;
		resultRect.clear();
		for (int i = 0; i < objectLocations.size(); i++)
			resultRect.push_back(objectLocations[i]);
	}
	return result;
}

bool BlockLBPDetector::DetectAllCandidates(const IplImage *image, std::vector<CvRect> *resultRect, float threshold, float scaleStep)
{
	bool result;
	result = FindAllCandidates(image, threshold, scaleStep);
	if (result) *resultRect = candidateLocations;
	return result;
}

bool BlockLBPDetector::DetectBestCandidate(const IplImage *image, CvRect *resultRect, float threshold, float scaleStep)
{
	bool result;
	result = FindAllCandidates(image, threshold, scaleStep);
	if (result) {
		float maxScore = detectionScores[0];
		int idxMax = 0;
		for (int i = 1; i < detectionScores.size(); i++) {
			if (maxScore < detectionScores[i]) {
				idxMax = i;
				maxScore = detectionScores[i];
			}
		}
		*resultRect = candidateLocations[idxMax];		
	}
	return result;
}

bool BlockLBPDetector::DetectBestNCandidates(const IplImage *image, vector<CvRect>& resultRects, float threshold, float scaleStep)
{
	bool result;
	BufferDLBP(image, scaleStep);
	FindAllCandidates_bybuffer(image, threshold, scaleStep, 0);
	//result = FindAllCandidates(image, threshold, scaleStep);
	if (!result) {
		resultRects.clear();
		return false;
	}

	vector<pair<float, int>> Idxes(detectionScores.size()); // Indexes after sort
	for (int i = 0; i < detectionScores.size(); i++) {
		Idxes[i].first = detectionScores[i];
		Idxes[i].second = i;
	}
	std::sort(Idxes.begin(), Idxes.end(), std::greater<pair<float, int>>());
	size_t n = min(resultRects.size(), detectionScores.size());
	resultRects.resize(n);
	for (int i = 0; i < n; i++) {
		resultRects[i] = candidateLocations[Idxes[i].second];
	}
	return result;
}

static float sse_inner(const float* a, const float* b, unsigned int size)
{
        float z = 0.0f, fres = 0.0f;
	#ifdef __linux__
		float ftmp[4] __attribute__ ((aligned(16))) = {0.0f, 0.0f, 0.0f, 0.0f};
	#else
        __declspec(align(16)) float ftmp[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	#endif
        __m128 mres;

        if ((size / 4) != 0) {
                mres = _mm_load_ss(&z);
                for (unsigned int i = 0; i < size / 4; i++)
                        mres = _mm_add_ps(mres, _mm_mul_ps(_mm_loadu_ps(&a[4*i]),
                        _mm_loadu_ps(&b[4*i])));

                //mres = a,b,c,d
                __m128 mv1 = _mm_movelh_ps(mres, mres);     //a,b,a,b
                __m128 mv2 = _mm_movehl_ps(mres, mres);     //c,d,c,d
                mres = _mm_add_ps(mv1, mv2);                //res[0],res[1]

                _mm_store_ps(ftmp, mres);                

                fres = ftmp[0] + ftmp[1];
        }

        if ((size % 4) != 0) {
                for (unsigned int i = size - size % 4; i < size; i++)
                        fres += a[i] * b[i];
        }

        return fres;
}


bool BlockLBPDetector::FindAllCandidates(const IplImage *image, float threshold, float scaleStep,IplImage *maskImage)
{
	bool result = false;
	assert (image->nChannels == 1 && image->depth == 8 );
	int imageWidth, imageHeight;
	imageWidth = image->width; imageHeight = image->height;

	BlockDenseLBP dlbp((IplImage *) image, m_width, m_height, m_blockWidth, 
		m_blockHeight, m_strideWidth, m_strideHeight, m_padWidth, m_padHeight);
	//if (detector.size() != dlbp.GetDimension()) return false;
	/*
	 * In case "linear_model.exe" misses the last few elements, add some zeors to detector to reach the right dimension
	 */
	size_t size = detector.size();
	if (size <= dlbp.GetDimension()) {
		for (int i = 0; i < dlbp.GetDimension() - size; i++)
			detector.push_back(0.0f);
	}
	else
		return false;

	float scale = 1.0;

	float *histogram;

	candidateLocations.clear();	
	candidateLabels.clear();
	detectionScores.clear();
	float scale_min = MAX(m_height/(float)imageHeight, m_width/(float)imageWidth);

	/*********************************************/
	/*******�������******************************/
	for (int f = 0; ; f++)		//��ͬ�߶�
	{
		if (scale < scale_min)
			break;

		int fwidth = cvRound(scale*imageWidth);
		int fheight = cvRound(scale*imageHeight);
		IplImage *resizeImage = cvCreateImage(cvSize(fwidth, fheight), 8, 1);
		//Here I don't re-create a new image with a size of (fwidth, fheight), instead, directly assign the size, is hopefully to increase the speed without the cost of allocating and deallocating
		IplImage *resizedMask = 0;		
		cvResize(image, resizeImage);

		if (maskImage != 0)
			resizedMask = cvCreateImage(cvSize(fwidth, fheight), 8, 1);
		cvResize(maskImage, resizedMask);

		dlbp.ResetBuffer(resizeImage);		
		
		int cols, rows;
		rows = (resizeImage->height - m_height)/m_strideHeight;
		cols = (resizeImage->width - m_width)/m_strideWidth;

		int offsetx_stride = m_padWidth / m_strideWidth;
		int offsety_stride = m_padHeight / m_strideHeight;
		//int offsetx_stride = 0;
		//int offsety_stride = 0;
		//for (int j = 0; (j)*winStride.height + HEIGHT + offsety < pimg->height; j++)
		//for (int i = 0; (i)*winStride.width + WIDTH + offsetx < pimg->width; i++)
		int maxi, maxj;
		maxj = rows + offsety_stride;
		maxi = cols + offsetx_stride;

		vector<cv::Rect> maskRects_;
		
		for (int j = -offsety_stride; j < maxj - 1; j++)
		for (int i = -offsetx_stride; i < maxi - 1; i++)
		{
			// Here I force the initial position of sliding window being (1, 1) instead of (0, 0), which is to solve border problem
			// The border problem is that at the image border some of pixels' neigbors are out of image, e.g., (0, 0)'s up-left neighbor.
			int offsetx = 1, offsety = 1;
			CvRect rct = cvRect(offsetx+i*m_strideWidth, offsety+j*m_strideHeight, 
				m_width, m_height);	

			if (maskImage != 0)
				if (!IsEffectiveRegion(resizedMask, rct))
					continue;
			
			dlbp.ExtractDenseLBP(resizeImage, rct);
			histogram = dlbp.GetDenseLBPFeature();

			float scoreDetection, scoreRecognition;
			scoreDetection = scoreRecognition = 0.;

			int k;
			
			scoreDetection = sse_inner(histogram, &detector[0], dlbp.GetDimension());

			//}
			//delete []histogram_long;
			CvRect rct_result;				
			rct_result.x = cvRound((rct.x) / scale);
			rct_result.y = cvRound((rct.y) / scale);
			rct_result.width = cvRound(rct.width / scale);
			rct_result.height = cvRound(rct.height / scale);


			if (scoreDetection - rhoDetector> threshold)	
			{
#ifdef _DEBUG
				FILE *pf = fopen("score.txt", "a");
				fprintf(pf, "%f\t", scoreDetection);
				fclose(pf);
#endif
				candidateLocations.push_back(rct_result);
				detectionScores.push_back(scoreDetection);

				//ʶ�����ҳ���
				if (recognizer.size() == dlbp.GetDimension()) {
					for (k = 0; k < dlbp.GetDimension() - 4; k += 4)
						scoreRecognition += histogram[k] * recognizer[k]
									+ histogram[k+1] * recognizer[k+1]
									+ histogram[k+2] * recognizer[k+2]
									+ histogram[k+3] * recognizer[k+3];

					for( ; k < dlbp.GetDimension(); k++ )
						scoreRecognition += histogram[k] * recognizer[k];

					if (scoreRecognition - rhoRecognizer > 0.0f)
						candidateLabels.push_back(1);
					else
						candidateLabels.push_back(-1);
				}
						
			}				
			
		}//for j
		scale /= scaleStep;
		//hklbp->reset_labelbuffer();
		cvReleaseImage(&resizeImage);
		if (maskImage != 0)
			cvReleaseImage(&resizedMask);	

	}//for f
	if (detectionScores.size() > 0) result = true;
	return result;
}

bool BlockLBPDetector::FindAllCandidates_aspectratio(const IplImage *image, float threshold, float scaleStep, const vector<cv::Rect> *maskRects)
{
	bool result = false;
	assert (image->nChannels == 1 && image->depth == 8 );
	int imageWidth, imageHeight;
	imageWidth = image->width; imageHeight = image->height;

	BlockDenseLBP dlbp((IplImage *) image, m_width, m_height, m_blockWidth, 
		m_blockHeight, m_strideWidth, m_strideHeight, m_padWidth, m_padHeight);
	//if (detector.size() != dlbp.GetDimension()) return false;
	/*
	 * In case "linear_model.exe" misses the last few elements, add some zeors to detector to reach the right dimension
	 */
	size_t size = detector.size();
	if (size <= dlbp.GetDimension()) {
		for (int i = 0; i < dlbp.GetDimension() - size; i++)
			detector.push_back(0.0f);
	}
	else
		return false;

	float scalex = 1.0, scaley = 1.0;

	float *histogram;

	candidateLocations.clear();	
	candidateLabels.clear();
	detectionScores.clear();
	float scale_minx = m_width/(float)imageWidth;
	float scale_miny = m_height/(float)imageHeight;


	IplImage *resizeImage = cvCreateImage(cvGetSize(image), 8, 1);

	/*********************************************/
	/*******�������******************************/
	for (int fy = 0; ; fy++) {
		if (scaley < scale_miny) break;
		for (int fx = 0; ; fx++) {
			if (scalex < scale_minx) break;

			int fwidth = cvRound(scalex*imageWidth);
			int fheight = cvRound(scaley*imageHeight);
		
			//Here I don't re-create a new image with a size of (fwidth, fheight), instead, directly assign the size, is hopefully to increase the speed without the cost of allocating and deallocating
			resizeImage->width = fwidth;
			resizeImage->height = fheight;
			cvResize(image, resizeImage);

			dlbp.ResetBuffer(resizeImage);		
		
			int cols, rows;
			rows = (resizeImage->height - m_height)/m_strideHeight;
			cols = (resizeImage->width - m_width)/m_strideWidth;

			int offsetx_stride = m_padWidth / m_strideWidth;
			int offsety_stride = m_padHeight / m_strideHeight;
			//int offsetx_stride = 0;
			//int offsety_stride = 0;
			//for (int j = 0; (j)*winStride.height + HEIGHT + offsety < pimg->height; j++)
			//for (int i = 0; (i)*winStride.width + WIDTH + offsetx < pimg->width; i++)
			int maxi, maxj;
			maxj = rows + offsety_stride;
			maxi = cols + offsetx_stride;

			vector<cv::Rect> maskRects_;
			if (maskRects != 0) {
				maskRects_.resize((*maskRects).size());
			// Resize mask rects
				for (int ir = 0; ir < (*maskRects).size(); ir++) {
					maskRects_[ir] = cv::Rect(cvRound((*maskRects)[ir].x*scalex), cvRound((*maskRects)[ir].y*scaley), cvRound((*maskRects)[ir].width*scalex), cvRound((*maskRects)[ir].height*scaley));
				}
			}
			for (int j = -offsety_stride; j < maxj - 1; j++)
			for (int i = -offsetx_stride; i < maxi - 1; i++)
			{
				// Here I force the initial position of sliding window being (1, 1) instead of (0, 0), which is to solve border problem
				// The border problem is that at the image border some of pixels' neigbors are out of image, e.g., (0, 0)'s up-left neighbor.
				int offsetx = 1, offsety = 1;
				CvRect rct = cvRect(offsetx+i*m_strideWidth, offsety+j*m_strideHeight, 
					m_width, m_height);	

				if (maskRects != 0) {
					if ( (*maskRects).size() > 0) { 
						bool intersected = false;
						for (int ir = 0; ir < (*maskRects).size(); ir++) {
							cv::Rect rct_(rct);
							cv::Rect maskRect = maskRects_[ir];
							if ((maskRect & rct_).area() / (rct_.area()+0.0) > 0.5) {
								intersected = true;
								break;
							}
						}
						if (!intersected) continue;
					} else
						continue;
				}

				dlbp.ExtractDenseLBP(resizeImage, rct);
				histogram = dlbp.GetDenseLBPFeature();

				float scoreDetection, scoreRecognition;
				scoreDetection = scoreRecognition = 0.;

				int k;
				/*long *histogram_long = new long[dlbp.GetDimension()];

				for(k = 0; k < dlbp.GetDimension(); k++ ) {
					histogram_long[k] = (long)(1e1 * histogram[k]);
				}*/

				//long score = 1;
				/*for (int kk = 0; kk < 10000; kk++)
				{*/
				/*for (k = 0; k < dlbp.GetDimension(); k += 1)
				{
					score += 2*histogram_long[k];
				}*/
				/*scoreDetection = 0;
				for(k = 0; k < dlbp.GetDimension(); k++ )
				{
					scoreDetection += histogram[k] ;
				}*/
				for (k = 0; k < (dlbp.GetDimension()/4)*4; k += 4)
				{
					scoreDetection += histogram[k] * detector[k] 
						+ histogram[k+1] * detector[k+1]
						+ histogram[k+2] * detector[k+2]
						+ histogram[k+3] * detector[k+3];
				}

				for( ; k < dlbp.GetDimension(); k++ )
				{
					scoreDetection += histogram[k] * detector[k];
				}
				//}
				//delete []histogram_long;
				CvRect rct_result;				
				rct_result.x = cvRound((rct.x) / scalex);
				rct_result.y = cvRound((rct.y) / scaley);
				rct_result.width = cvRound(rct.width / scalex);
				rct_result.height = cvRound(rct.height / scaley);


				if (scoreDetection - rhoDetector> threshold)	
				{
	#ifdef _DEBUG
					FILE *pf = fopen("score.txt", "a");
					fprintf(pf, "%f\t", scoreDetection);
					fclose(pf);
	#endif
					candidateLocations.push_back(rct_result);
					detectionScores.push_back(scoreDetection);

					//ʶ�����ҳ���
					if (recognizer.size() == dlbp.GetDimension()) {
						for (k = 0; k < dlbp.GetDimension() - 4; k += 4)
							scoreRecognition += histogram[k] * recognizer[k]
										+ histogram[k+1] * recognizer[k+1]
										+ histogram[k+2] * recognizer[k+2]
										+ histogram[k+3] * recognizer[k+3];

						for( ; k < dlbp.GetDimension(); k++ )
							scoreRecognition += histogram[k] * recognizer[k];

						if (scoreRecognition - rhoRecognizer > 0.0f)
							candidateLabels.push_back(1);
						else
							candidateLabels.push_back(-1);
					}
						
				}				
			
			}//for j
			scalex /= scaleStep;
			//hklbp->reset_labelbuffer();
		}//for fx
		scaley /= scaleStep;

	}//for fy
	cvReleaseImage(&resizeImage);
	if (detectionScores.size() > 0) result = true;
	return result;
}

bool BlockLBPDetector::MergeCandidates(int groupThreshold)
{
	bool result;
	std::vector<int> labels;	
	int nObjects = 0;

	if (candidateLocations.size() > 0)
	{
		//�õ���ľ��ο����merging
		int nclasses = cv::partition(candidateLocations, labels, SimilarRects(0.5, 0.2));
		//��������		
		std::vector<CvRect> rrects(nclasses);
		std::vector<int> orientLabels(nclasses, 0);
		std::vector<int> rweights(nclasses, 0);
		int nlabels = (int)labels.size();		

		for(int i = 0; i < nlabels; i++ )
		{
			int cls = labels[i];
			rrects[cls].x += candidateLocations[i].x;
			rrects[cls].y += candidateLocations[i].y;
			rrects[cls].width += candidateLocations[i].width;
			rrects[cls].height += candidateLocations[i].height;
			rweights[cls]++;
			if (candidateLabels.size() > 0)
				orientLabels[cls] += candidateLabels[i];
		}

		objectLocations.clear();
		objectLabels.clear();
		objectWeights.clear();

		for(int i = 0; i < nclasses; i++ )
		{
			if( rweights[i] < groupThreshold )
				continue;
			CvRect r = rrects[i];
			float s = 1.f/rweights[i];	
			objectWeights.push_back(rweights[i]);			
			objectLocations.push_back(cvRect(cvRound(r.x*s), cvRound(r.y*s), cvRound(r.width*s), cvRound(r.height*s)));
			if (candidateLabels.size() > 0) {
				if (orientLabels[i] > 0)
					objectLabels.push_back(ORIENTATION_LEFT);
				else if (orientLabels[i] < 0)
					objectLabels.push_back(ORIENTATION_RIGHT);
				else
					objectLabels.push_back(ORIENTATION_INVALID);
			}
			++nObjects;
		}		
	} // if candidateLocations.size > 0
	if (nObjects >= 1) result = true;
	else result = false;
	return result;
}


void BlockLBPDetector::ComputeDBLPBuffer(const IplImage *image, BlockDenseLBP **pplbp, CvSize sz, IplImage *maskImage)
{
	IplImage *resizeImage = cvCreateImage(sz, 8, 1);	
	cvResize(image, resizeImage);

	IplImage *resizedMask = 0;
	if (maskImage != 0) {
		resizedMask = cvCreateImage(sz, 8, 1); 
		cvResize(maskImage, resizedMask);
	}

	BlockDenseLBP *pdlbp = new BlockDenseLBP((IplImage *) resizeImage, m_width, m_height, m_blockWidth, 
		m_blockHeight, m_strideWidth, m_strideHeight, m_padWidth, m_padHeight);
	pdlbp->ResetBuffer(resizeImage);		
		
	int cols, rows;
	rows = (resizeImage->height - m_height)/m_strideHeight;
	cols = (resizeImage->width - m_width)/m_strideWidth;

	int offsetx_stride = m_padWidth / m_strideWidth;
	int offsety_stride = m_padHeight / m_strideHeight;
		
	int maxi, maxj;
	maxj = rows + offsety_stride;
	maxi = cols + offsetx_stride;		
		
	for (int j = -offsety_stride; j < maxj - 1; j++)
	for (int i = -offsetx_stride; i < maxi - 1; i++)
	{
		// Here I force the initial position of sliding window being (1, 1) instead of (0, 0), which is to solve border problem
		// The border problem is that at the image border some of pixels' neigbors are out of image, e.g., (0, 0)'s up-left neighbor.
		int offsetx = 1, offsety = 1;
		CvRect rct = cvRect(offsetx+i*m_strideWidth, offsety+j*m_strideHeight, 
			m_width, m_height);	

		if (maskImage != 0)
			if (!IsEffectiveRegion(resizedMask, rct))
				continue;
			
		pdlbp->ExtractDenseLBP(resizeImage, rct);
			
	}//for j
	*pplbp = pdlbp;
	cvReleaseImage(&resizeImage);
	if (maskImage != 0) 
		cvReleaseImage(&resizedMask);
}

void BlockLBPDetector::BufferDLBP(const IplImage *image, float scaleStep, IplImage *maskImage)
{
	this->ReleaseDLBPBuffer();
	
	bool result = false;
	assert (image->nChannels == 1 && image->depth == 8 );
	int imageWidth, imageHeight;
	imageWidth = image->width; imageHeight = image->height;

	float scale = 1.0;
	float scale_min = MAX(m_height/(float)imageHeight, m_width/(float)imageWidth);

	vector<CvSize> sizes;

	for (int f = 0; ; f++)		//��ͬ�߶�
	{
		if (scale < scale_min)
			break;

		int fwidth = cvRound(scale*imageWidth);
		int fheight = cvRound(scale*imageHeight);
		sizes.push_back(cvSize(fwidth, fheight));
		
		scale /= scaleStep;
	}
	m_dlbps.resize(sizes.size());

#ifdef USE_MULTITHREADS
	ThreadManager threadManager;
	unsigned int nThreads = threadManager.getMaxNbThreads();
	for (int f = 0; f < sizes.size(); f++) {
		threadManager.LaunchNewThread(
				boost::bind(
					&BlockLBPDetector::ComputeDBLPBuffer, this, image, &m_dlbps[f], sizes[f], maskImage));
	}
	threadManager.JoinAll();
#else
	for (int f = 0; f < sizes.size(); f++) {
		ComputeDBLPBuffer(image, &m_dlbps[f], sizes[f], maskImage);		
	}//for i
#endif
}

void BlockLBPDetector::DetectScale_bybuffer(const IplImage* image, BlockDenseLBP *dlbp, const CvSize sz, float scale, float threshold, IplImage *maskImage)
{
	float *histogram;
	
	IplImage *resizeImage = cvCreateImage(sz, 8, 1);
	
	cvResize(image, resizeImage);
	IplImage *resizedMask = 0;
	if (maskImage) {
		resizedMask = cvCreateImage(sz, 8, 1);
		cvResize(maskImage, resizedMask);
	}
		
	int cols, rows;
	rows = (resizeImage->height - m_height)/m_strideHeight;
	cols = (resizeImage->width - m_width)/m_strideWidth;

	int offsetx_stride = m_padWidth / m_strideWidth;
	int offsety_stride = m_padHeight / m_strideHeight;
		
	int maxi, maxj;
	maxj = rows + offsety_stride;
	maxi = cols + offsetx_stride;
	
	for (int j = -offsety_stride; j < maxj - 1; j++)
	for (int i = -offsetx_stride; i < maxi - 1; i++)
	{
		// Here I force the initial position of sliding window being (1, 1) instead of (0, 0), which is to solve border problem
		// The border problem is that at the image border some of pixels' neigbors are out of image, e.g., (0, 0)'s up-left neighbor.
		int offsetx = 1, offsety = 1;
		CvRect rct = cvRect(offsetx+i*m_strideWidth, offsety+j*m_strideHeight, 
			m_width, m_height);	

		if (maskImage != 0) {
			if (!IsEffectiveRegion(resizedMask, rct))
				continue;			
		}

		dlbp->ExtractDenseLBP(resizeImage, rct);
		histogram = dlbp->GetDenseLBPFeature();

		float scoreDetection, scoreRecognition;
		scoreDetection = scoreRecognition = 0;

		/*int k;
		for (k = 0; k < (m_dlbps[f]->GetDimension()/4)*4; k += 4)
		{
			scoreDetection += histogram[k] * detector[k] 
				+ histogram[k+1] * detector[k+1]
				+ histogram[k+2] * detector[k+2]
				+ histogram[k+3] * detector[k+3];
		}
		for( ; k < m_dlbps[f]->GetDimension(); k++ )
		{
			scoreDetection += histogram[k] * detector[k];
		}*/
		scoreDetection = sse_inner(histogram, &detector[0], dlbp->GetDimension());
			
		CvRect rct_result;				
		rct_result.x = cvRound((rct.x) / scale);
		rct_result.y = cvRound((rct.y) / scale);
		rct_result.width = cvRound(rct.width / scale);
		rct_result.height = cvRound(rct.height / scale);

		if (scoreDetection - rhoDetector> threshold)	
		{			
#ifdef USE_MULTITHREADS
			boost::unique_lock<boost::mutex> scoped_lock(guard);
			{
#endif
			candidateLocations.push_back(rct_result);
			detectionScores.push_back(scoreDetection);
#ifdef USE_MULTITHREADS
			}
#endif			
		}
	}//for j
	if (maskImage) {
		cvReleaseImage(&resizedMask);
	}
	cvReleaseImage(&resizeImage);

}

bool BlockLBPDetector::FindAllCandidates_bybuffer(const IplImage *image, float threshold, float scaleStep, IplImage *maskImage)
{
	bool result = false;
	assert (image->nChannels == 1 && image->depth == 8 );
	int imageWidth, imageHeight;
	imageWidth = image->width; imageHeight = image->height;

	float scale = 1.0;
	float *histogram;

	candidateLocations.clear();	
	candidateLabels.clear();
	detectionScores.clear();
	float scale_min = MAX(m_height/(float)imageHeight, m_width/(float)imageWidth);

	vector<CvSize> sizes;
	vector<float> scales;
	for (int f = 0; ; f++)		//��ͬ�߶�
	{
		if (scale < scale_min)
			break;

		int fwidth = cvRound(scale*imageWidth);
		int fheight = cvRound(scale*imageHeight);
		sizes.push_back(cvSize(fwidth, fheight));
		scales.push_back(scale);
		scale /= scaleStep;
	}
		
#ifdef USE_MULTITHREADS 
	{
	ThreadManager threadManager;
	unsigned int nThreads = threadManager.getMaxNbThreads();
	for (int f = 0; f < sizes.size(); f++) {
		threadManager.LaunchNewThread(
				boost::bind(
					&BlockLBPDetector::DetectScale_bybuffer, this, image, m_dlbps[f], sizes[f], scales[f], threshold, maskImage));
	}
	threadManager.JoinAll();
	threadManager.~ThreadManager();
	}
#else
	for (int f = 0; f < sizes.size(); f++) {
		DetectScale_bybuffer(image, m_dlbps[f], sizes[f], scales[f], threshold, maskImage);		
	}//for f	
#endif
	if (detectionScores.size() > 0) result = true;
	return result;
}

bool BlockLBPDetector::DetectObjects_byBuffer(const IplImage *image, std::vector<CvRect>& resultRect, std::vector<Orientation> *orientation, float threshold, int groupThreshold, float scaleStep, IplImage *maskImage)
{
	bool result;
	BufferDLBP(image, scaleStep, maskImage);
	FindAllCandidates_bybuffer(image, threshold, scaleStep, maskImage);


	result = MergeCandidates(groupThreshold);
	if (result) {
		resultRect.clear();
		for (int i = 0; i < objectLocations.size(); i++)
			resultRect.push_back(objectLocations[i]);
	}
	return result;
}

