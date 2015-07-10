#include "DLBPUtilities.h"

static int sg_tw = 64;
static int sg_th = 64;
static int sg_bw = 16;
static int sg_bh = 16;
static int sg_sw = 8;
static int sg_sh = 8;

DLBPConfiguration g_dlbp_config(sg_tw, sg_th, sg_bw, sg_bh, sg_sw, sg_sh, 0, 0);
BlockLBPDetector *g_pDetector;

void ComputeDLBP(const Mat& imagePatch, vector<float>& hist)
{
	float epsilon = 1e-2;
	// Whether has already been verified
	
	Mat rszI, grayI;
	if (!imagePatch.data) return;

	Mat grayPatch;
	if (imagePatch.channels() == 3) {
		cv::cvtColor(imagePatch, grayPatch, CV_BGR2GRAY);
	} else {
		grayPatch = imagePatch;
	}

	resize(grayPatch, rszI, Size(sg_tw+2, sg_th+2));
	IplImage patch = rszI.operator IplImage();	
	BlockDenseLBP dlbp((&patch), 32, 32, 32, 32, 16, 16, 0, 0);
	dlbp.ResetBuffer((&patch));
	const CvRect rect = cvRect(1, 1, sg_tw, sg_th);
	dlbp.ExtractDenseLBP(&patch, rect);

	hist = std::vector<float>(dlbp.GetDenseLBPFeature(), dlbp.GetDenseLBPFeature() + dlbp.GetDimension());	
}


float DLBPClassification(const Mat& imgPatch, LBPDetector& classifier)
{
	Mat grayPatch;

	grayPatch = imgPatch;

	if (imgPatch.channels() == 3)
		cv::cvtColor(imgPatch, grayPatch, CV_BGR2GRAY);

	Mat rsz;
	cv::resize(grayPatch, rsz, Size(sg_tw+2, sg_th+2));
	BlockDenseLBP dlbp((&grayPatch.operator IplImage()), sg_tw, sg_th, sg_bw, sg_bh, sg_sw, sg_sh, 0, 0);
	dlbp.ResetBuffer(&grayPatch.operator IplImage());

	const CvRect rect = cvRect(1, 1, sg_tw, sg_th);
	dlbp.ExtractDenseLBP(&grayPatch.operator IplImage(), rect);

	float score = classifier.CalDetectionScore(dlbp.GetDenseLBPFeature()) - classifier.GetDetectionThreshold();
	return score;
}

float ColorSimilarity(const Mat& img1, const Mat& img2)
{
	return 0.0;
}