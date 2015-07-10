#ifndef __COLOR_HIST_WFL__
#define __COLOR_HIST_WFL__
#include <opencv2/opencv.hpp>
#include <vector>
#include "ExportDLBP.h"

using namespace std;
using namespace cv;
LBPDETECTOR_API void ComputeColorHist(const Mat& image, vector<float>& hist);

#endif