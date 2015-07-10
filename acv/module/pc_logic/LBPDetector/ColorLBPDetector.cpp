#include "ColorLBPDetector.h"

#include "LBPDetector.h"
#include "ColorHist.h"
#include  <functional>


ColorLBPDetector::ColorLBPDetector(int width, int height, int blockWidth, int blockHeight,
		int strideWidth, int strideHeight, int padWidth, int padHeight)
	:LBPDetector(width, height,blockWidth, blockHeight, strideWidth, strideHeight, padWidth, padHeight)
{
	detector.clear();
	recognizer.clear();
}

ColorLBPDetector::~ColorLBPDetector(void)
{
	detector.clear();
	recognizer.clear();
}


bool ColorLBPDetector::DetectObject(const IplImage *image, CvRect *resultRect, Orientation *orientation, float threshold, int groupThreshold, float scaleStep)
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

bool ColorLBPDetector::DetectObjects(const IplImage *image, std::vector<CvRect>& resultRect, std::vector<Orientation> *orientation, float threshold, int groupThreshold, float scaleStep, const IplImage *maskImage)
{
	bool result;
	//FindAllCandidates_aspectratio(image, threshold, scaleStep, 0);
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

bool ColorLBPDetector::DetectAllCandidates(const IplImage *image, std::vector<CvRect> *resultRect, float threshold, float scaleStep)
{
	bool result;
	result = FindAllCandidates(image, threshold, scaleStep);
	if (result) *resultRect = candidateLocations;
	return result;
}

bool ColorLBPDetector::DetectBestCandidate(const IplImage *image, CvRect *resultRect, float threshold, float scaleStep)
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

bool ColorLBPDetector::DetectBestNCandidates(const IplImage *image, vector<CvRect>& resultRects, float threshold, float scaleStep)
{
	bool result;
	result = FindAllCandidates(image, threshold, scaleStep);
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
	int n = min(resultRects.size(), detectionScores.size());
	resultRects.resize(n);
	for (int i = 0; i < n; i++) {
		resultRects[i] = candidateLocations[Idxes[i].second];
	}
	return result;
}

bool ColorLBPDetector::FindAllCandidates(const IplImage *colorimage, float threshold, float scaleStep, const IplImage *maskImage)
{
	bool result = false;
	//assert (image->nChannels == 1 && image->depth == 8 );
	int imageWidth, imageHeight;
	IplImage *image = cvCreateImage(cvGetSize(colorimage), 8, 1);
	cvCvtColor(colorimage, image, CV_BGR2GRAY);
	imageWidth = image->width; imageHeight = image->height;

	BlockDenseLBP dlbp((IplImage *) image, m_width, m_height, m_blockWidth, 
		m_blockHeight, m_strideWidth, m_strideHeight, m_padWidth, m_padHeight);
	//if (detector.size() != dlbp.GetDimension()) return false;
	/*
	 * In case "linear_model.exe" misses the last few elements, add some zeors to detector to reach the right dimension
	 */
	int size = detector.size();
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

	IplImage *resizeImage = cvCreateImage(cvGetSize(image), 8, 1);
	IplImage *colorRszImage = cvCreateImage(cvGetSize(colorimage), 8, 3);
	IplImage *resizeMask = 0;
	if (maskImage != 0)
		resizeMask = cvCreateImage(cvGetSize(maskImage), 8, 1);

	IplImage *colorPatch = cvCreateImage(cvSize(m_width, m_height), 8, 3);

	/*********************************************/
	/*******穷举搜索******************************/
	for (int f = 0; ; f++)		//不同尺度
	{
		if (scale < scale_min)
			break;

		int fwidth = cvRound(scale*imageWidth);
		int fheight = cvRound(scale*imageHeight);
		
		//Here I don't re-create a new image with a size of (fwidth, fheight), instead, directly assign the size, is hopefully to increase the speed without the cost of allocating and deallocating
		resizeImage->width = fwidth;
		resizeImage->height = fheight;
		colorRszImage->width = fwidth;
		colorRszImage->height = fheight;
		cvResize(image, resizeImage);
		cvResize(colorimage, colorRszImage);
		
		if (maskImage != 0) {
			resizeMask->width = fwidth;
			resizeMask->height = fheight;
			cvResize(maskImage, resizeMask);
		}

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
			
			cvSetImageROI(colorRszImage, rct);
			cvCopy(colorRszImage, colorPatch);
			cvResetImageROI(colorRszImage);
			vector<float> colorhist;
			ComputeColorHist(cv::Mat(colorPatch), colorhist);

			bool iseffective = true;
			if (maskImage != 0) {
				int nZeros = 0;
				for (int y = rct.y; y < rct.y + rct.height - 1; y++) {
					for (int x = rct.x; x < rct.x + rct.width - 1; x++) {
						unsigned char *p =  (unsigned char *)resizeMask->imageData + y*resizeMask->widthStep + x;
						if (*p == 0)
							++nZeros;
						if (nZeros > 10) {
							iseffective = false;
							break;
						}
					}
					if (!iseffective) break;
				}
			}
			if (!iseffective) continue;

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

			for (; k < dlbp.GetDimension() + colorhist.size(); k++) {
				scoreDetection += colorhist[k-dlbp.GetDimension()]*detector[k];
			}

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

				//识别左右朝向
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

	}//for f
	cvReleaseImage(&resizeImage);
	cvReleaseImage(&colorRszImage);
	cvReleaseImage(&colorPatch);
	cvReleaseImage(&image);

	if (resizeMask != 0)
		cvReleaseImage(&resizeMask);
	if (detectionScores.size() > 0) result = true;
	return result;
}

bool ColorLBPDetector::FindAllCandidates_aspectratio(const IplImage *image, float threshold, float scaleStep, const IplImage *maskImage)
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
	int size = detector.size();
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
	/*******穷举搜索******************************/
	for (scaley = 1.0; scaley > scale_miny; scaley /= scaleStep) {
			//if (scaley > 0.6 || scaley < 0.4) continue;
		/*if (scalex / scaley < 0.5 || scalex /scaley > 3) {
			scaley /= scaleStep;
			continue;
		}*/

		for (scalex = 1.0; scalex > scale_minx; scalex /= scaleStep) {
			if (!(scalex > 0.1 && scalex < 0.8 && scaley > 0.1 && scaley < 0.8)) {
				continue;
			}

			if (scalex / scaley < 0.5 || scalex /scaley > 2) {
				continue;
			}

			int fwidth = cvRound(scalex*imageWidth);
			int fheight = cvRound(scaley*imageHeight);

			/*if (fwidth > 320)
				continue;
			if (fheight > 60)
				continue;*/
			/*if (fwidth*1.0/fheight > 15 || fwidth*1.0/fheight < 4.0)
				continue;*/
		
			cout << "fwidth " << fwidth << " fheight " << fheight << endl;

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
			
			for (int j = -offsety_stride; j < maxj - 1; j++)
			for (int i = -offsetx_stride; i < maxi - 1; i++)
			{
				// Here I force the initial position of sliding window being (1, 1) instead of (0, 0), which is to solve border problem
				// The border problem is that at the image border some of pixels' neigbors are out of image, e.g., (0, 0)'s up-left neighbor.
				int offsetx = 1, offsety = 1;
				CvRect rct = cvRect(offsetx+i*m_strideWidth, offsety+j*m_strideHeight, 
					m_width, m_height);	
				
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
					//cout << "score " << scoreDetection - rhoDetector<< endl;
					candidateLocations.push_back(rct_result);
					detectionScores.push_back(scoreDetection);

					//识别左右朝向
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

bool ColorLBPDetector::MergeCandidates(int groupThreshold)
{
	bool result;
	std::vector<int> labels;	
	int nObjects = 0;

	if (candidateLocations.size() > 0)
	{
		//得到后的矩形框进行merging
		int nclasses = cv::partition(candidateLocations, labels, SimilarRects(0.01, 0.01));
		//区分左右		
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