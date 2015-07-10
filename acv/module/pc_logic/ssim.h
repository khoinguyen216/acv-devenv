#include <opencv2/opencv.hpp>

#define ALLCHANNEL -1
double calcSSIM(const cv::Mat& src1, const cv::Mat& src2, cv::Mat& ssim_map, cv::Mat& mask, int channel = 0, int method=CV_BGR2YUV,const double K1 = 0.01, const double K2 = 0.03,	const int L = 255, const int downsamplewidth=256, const int gaussian_window=11, const double gaussian_sigma=1.5);
double calcSSIMBB(cv::Mat& src1, cv::Mat& src2, cv::Mat& ssim_map, int channel = 0, int method=CV_BGR2YUV, int boundx=0,int boundy=0,const double K1 = 0.01, const double K2 = 0.03,	const int L = 255, const int downsamplewidth=256, const int gaussian_window=11, const double gaussian_sigma=1.5);

double calcDSSIM(cv::Mat& src1, cv::Mat& src2, cv::Mat& ssim_map, cv::Mat& mask, int channel = 0, int method=CV_BGR2YUV, const double K1 = 0.01, const double K2 = 0.03,	const int L = 255, const int downsamplewidth=256, const int gaussian_window=11, const double gaussian_sigma=1.5);
double calcDSSIMBB(cv::Mat& src1, cv::Mat& src2, cv::Mat& ssim_map, int channel = 0, int method=CV_BGR2YUV, int boundx=0,int boundy=0,const double K1 = 0.01, const double K2 = 0.03,	const int L = 255, const int downsamplewidth=256, const int gaussian_window=11, const double gaussian_sigma=1.5);
