#ifndef __DLBP_UTILITIES_PEOPLECOUNTING_WFL__
#define __DLBP_UTILITIES_PEOPLECOUNTING_WFL__
#include "opencv2/opencv.hpp"
#include "LBPDetector/BlockDenseLBP.h"
#include "LBPDetector/LBPDetector.h"
#include "LBPDetector/BlockLBPDetector.h"


using namespace cv;

void ComputeDLBP(const Mat& imagePatch, vector<float>& hist);

float DLBPClassification(const Mat& imgPatch1, LBPDetector& classifier);

extern DLBPConfiguration g_dlbp_config;
extern BlockLBPDetector *g_pDetector;

#endif