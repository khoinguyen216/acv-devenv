#ifndef _LBPDETECTOR_H
#define _LBPDETECTOR_H
#include "ExportDLBP.h"
#include "BlockDenseLBP.h"

struct DLBPConfiguration
{
	DLBPConfiguration(int _windowWidth, int _windowHeight, int _blockWidth, int _blockHeight, 
		int _strideWidth, int _strideHeight, int _padWidth , int _padHeight)
		:width(_windowWidth), height(_windowHeight), blockWidth(_blockWidth), 
		blockHeight(_blockHeight), strideWidth(_strideWidth), strideHeight(_strideHeight),
		padWidth(_padWidth), padHeight(_padHeight)
	{}
	DLBPConfiguration() {}
	int width;
	int height;
	int blockWidth;
	int blockHeight;
	int strideWidth;
	int strideHeight;
	int padWidth;
	int padHeight;
};

class LBPDetector
{
private:
	/**
	 * Disable the default empty constructor since I still don't need it
	 */
	LBPDETECTOR_API LBPDetector(void);
	/**
	 * Disable the default copy constructor since I still don't need it
	 */
	LBPDETECTOR_API LBPDetector(const LBPDetector&);
	/**
	 * Disable the default assignment operator since I still don't need it
	 */
	LBPDETECTOR_API LBPDetector& operator = (const LBPDetector&);
public:
	enum Orientation{ORIENTATION_LEFT, ORIENTATION_RIGHT, ORIENTATION_INVALID};
public:
	LBPDETECTOR_API virtual bool DetectObjects(const IplImage *image, std::vector<CvRect>& resultRect, std::vector<Orientation> *orientation, float threshold, int groupThreshold, float scaleStep, const vector<cv::Rect> *maskRects = 0) {
		return false;
	}

	/**
	 * Constructor from struct.
	 * Added by Fanglin, 2014-09-22
	 */
	LBPDETECTOR_API LBPDetector(const DLBPConfiguration& config)
		:m_width(config.width), m_height(config.height), m_blockWidth(config.blockWidth), m_blockHeight(config.blockHeight), 
		m_strideWidth(config.strideWidth), m_strideHeight(config.strideHeight), m_padWidth(config.padWidth), m_padHeight(config.padHeight)
	{
		detector.clear();
		recognizer.clear();
	}
	/**
	 * Constructor
	 */
	LBPDETECTOR_API LBPDetector(int width, int height, int blockWidth, int blockHeight,
		int strideWidth, int strideHeight, int padWidth, int padHeight)
		:m_width(width), m_height(height), m_blockWidth(blockWidth), m_blockHeight(blockHeight), 
		m_strideWidth(strideWidth), m_strideHeight(strideHeight), m_padWidth(padWidth), m_padHeight(padHeight)
	{
		detector.clear();
		recognizer.clear();
	}

	/**
	 * Destructor
	 */
	LBPDETECTOR_API ~LBPDetector(void)
	{
		detector.clear();
		recognizer.clear();
	}

	/**
	 * Load the classifier of detector from a file
	 */
	LBPDETECTOR_API void LoadDetector(const char* filename);

	/**
	 * Load the classifier of recognizer from a file
	 */
	LBPDETECTOR_API void LoadRecognizer(const char* filename);

	/**
	 * Calculate the detection score of a feature vector
	 */
	LBPDETECTOR_API float CalDetectionScore(const float* histogram);

	/**
	 * Calculate the recogition score of a feature vector
	 */
	LBPDETECTOR_API float CalRecognitionScore(const float* histogram);

	/**
	 * Get the detection thrshold
	 */
	LBPDETECTOR_API const float GetDetectionThreshold() const {return rhoDetector;}
	/**
	 * Get the detection thrshold
	 */
	LBPDETECTOR_API void SetDetectionThreshold(float threshold) {rhoDetector = threshold;}

	/**
	 * Get the feature dimension
	 */
	LBPDETECTOR_API size_t GetDimension(float threshold) {return detector.size();}
protected:
	/**
	 * Calculate the recogition score given a feature vector
	 */
	LBPDETECTOR_API float CalClassificationScore(const std::vector<float>& classifier, const float* histogram);
	
	/**
	 * Load classifier
	 */
	void LoadClassifier(const char* filename, vector<float>& classifier, float& rho);
	
protected:
	std::vector<float> detector, recognizer;
	float rhoDetector, rhoRecognizer;
	
	std::vector<float> detectionScores;

	std::vector<CvRect> candidateLocations;
	std::vector<CvRect> objectLocations;
	std::vector<int> objectWeights;
	std::vector<int> candidateLabels;
	std::vector<Orientation> objectLabels;

protected:
	/**
	 * The size of the sliding window, lbp block, stride and pad
	 */
	int m_width, m_height;
	int m_blockWidth, m_blockHeight;
	int m_strideWidth, m_strideHeight;
	int m_padWidth, m_padHeight;
};


#endif
