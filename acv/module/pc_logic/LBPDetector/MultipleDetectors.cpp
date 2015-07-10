#include "MultipleDetectors.h"

void MultipleDetectors::SetDetectors(vector<string> filePaths)
{
	detectors.resize(filePaths.size());
	rhoDetectors.resize(filePaths.size());
	for (int i = 0; i < filePaths.size(); i++)
		LoadClassifier(filePaths[i].c_str(), detectors[i], rhoDetectors[i]);
}

void MultipleDetectors::DetectMultipleObjects(const IplImage *image, vector<vector<CvRect>>& resultMultiRects, float threshold, int groupThreshold, float scaleStep)
{
	resultMultiRects.clear();
	resultMultiRects.resize(detectors.size());
	for (int i = 0; i < detectors.size(); i++) {
		vector<CvRect> rects;
		detector.clear();
		detector.resize(detectors[i].size());
		std::copy(detectors[i].begin(), detectors[i].end(), detector.begin());
		// Because "DetectObjects" only uses detector
		DetectObjects(image, rects, 0, groupThreshold, scaleStep);
		resultMultiRects.push_back(rects);
	}
}
