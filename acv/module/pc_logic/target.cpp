#include <algorithm>

#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "target.h"
#include "bwlabel.h"

namespace pt = boost::posix_time;

//TODO: add mask during calculation histogram
void Target::ComputeHSVHist(const Mat& image, Rect& bb)
{
	// To get the effective region
	Rect imgRect = Rect(0, 0, image.cols, image.rows);
	Rect effectiveRect = imgRect & bb;

	bb = effectiveRect;

	Mat imagePatch = image(effectiveRect);
	if (!imagePatch.data) {
		cout << "strange!!!!!!!!!!!!!!!!!!!!!!!" << endl;
		cout << "effectiveRect" << effectiveRect << endl;
		return;
	}
	m_imagePatch = imagePatch.clone();
	Mat hsv;
	cvtColor( imagePatch, hsv, CV_BGR2HSV );
	// Quantize the hue to 30 levels
    // and the saturation to 32 levels
    int hbins = 30, sbins = 32;
    int histSize[] = {hbins, sbins};
    // hue varies from 0 to 179, see cvtColor
    float hranges[] = { 0, 180 };
    // saturation varies from 0 (black-gray-white) to
    // 255 (pure spectrum color)
    float sranges[] = { 0, 256 };
    const float* ranges[] = { hranges, sranges };
    // we compute the histogram from the 0-th and 1-st channels
    int channels[] = {0, 1};

    calcHist( &hsv, 1, channels, Mat(), // do not use mask
             hist, 2, histSize, ranges,
             true, // the histogram is uniform
             false );

	//imwrite("pathch.png", imagePath);

	// Normalize the histogram
	hist /= (imagePatch.total()+0.0);
}

void SaveMotionSegmentation(const Mat& frame, string videoFilename, string foldePath, const vector<Target> objects, int frameID)
{
#ifdef __linux__
	char *fname;
	fname = basename((char *)videoFilename.c_str());
#else
	char drive[4096], dir[4096], fname[4096], ext[4096];
	_splitpath(videoFilename.c_str(), drive, dir, fname, ext);
#endif
	for (int i = 0; i < objects.size(); i++) {
		stringstream ss;
		ss << foldePath + "/" + fname + "_f" << frameID << "_o" << i << ".png";
		Rect rect = Rect(0, 0, frame.cols, frame.rows) & objects[i].bb;
		Mat imagaPatch = frame(rect);
		double r = (rect.width + 0.0)/rect.height;
		if (r > 2.5 && imagaPatch.data)
			imwrite(ss.str(), imagaPatch);
	}
}


