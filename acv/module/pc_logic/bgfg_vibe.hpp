#ifndef BGFG_VIBE_HPP_INCLUDED
#define BGFG_VIBE_HPP_INCLUDED

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

struct bg_model {
	cv::Mat*** samples;
	cv::Mat** fgch;
	cv::Mat* fg;
};

class bgfg_vibe
{
#define rndSize 256
	unsigned char ri;
#define rdx ri++
public:
	bgfg_vibe();
	int R,N,noMin,phi,nochs;
	void init_model(cv::Mat& firstSample);
	void setphi(int phi);
	void rndreset();
	cv::Mat* fg(cv::Mat& frame);
	cv::Mat bg();
private:
	bool initDone;
	cv::RNG rnd;
	bg_model* model;
	void init();
	void fg1ch(cv::Mat& frame,cv::Mat** samples,cv::Mat* fg);
	int rndp[rndSize],rndn[rndSize],rnd8[rndSize];
};

#endif

