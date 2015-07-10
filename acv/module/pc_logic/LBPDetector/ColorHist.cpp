#include "ColorHist.h"

using namespace cv;
void ComputeColorHist(const Mat& image, vector<float>& _hist)
{
	Mat hsv, src;
	src = image;
	cv::cvtColor(src, hsv, CV_BGR2HSV);

	// Quantize the hue to 30 levels
    // and the saturation to 32 levels
    int hbins = 9, sbins = 8;
    int histSize[] = {hbins, sbins};
    // hue varies from 0 to 179, see cvtColor
    float hranges[] = { 0, 180 };
    // saturation varies from 0 (black-gray-white) to
    // 255 (pure spectrum color)
    float sranges[] = { 0, 256 };
    const float* ranges[] = { hranges, sranges };
    MatND hist;
    // we compute the histogram from the 0-th and 1-st channels
    int channels[] = {0, 1};

    calcHist( &hsv, 1, channels, Mat(), // do not use mask
             hist, 2, histSize, ranges, true, // the histogram is uniform
             false );

    double maxVal=0;
    
	_hist.resize(hbins*sbins); 

	int idx = 0;
	for (int p = 0; p < histSize[0]; p++) {
		for (int q = 0; q < histSize[1]; q++) {
			_hist[idx] = hist.at<float>(p, q)/hsv.total();
			++idx;
		}			
	}	
}