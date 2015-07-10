#include "LBPDetector.h"

void LBPDetector::LoadClassifier(const char* filename, vector<float>& classifier, float& rho)
{
	FILE *pf = fopen(filename, "r");
	if (!pf) return;
	classifier.clear();
	while(1) {
		float value;
		fscanf(pf, "%f", &value);
		if (EOF != fscanf(pf, "f, ")) {
			classifier.push_back(value);
		}
		else {
			rho = value;
			break;
		}
	}
	fclose(pf);
}

void LBPDetector::LoadDetector(const char* filename)
{
	this->LoadClassifier(filename, detector, rhoDetector);
	/*FILE *pf = fopen(filename, "r");
	if (!pf) return;
	detector.clear();
	while(1) {
		float value;
		fscanf(pf, "%f", &value);
		if (EOF != fscanf(pf, "f, ")) {
			detector.push_back(value);
		}
		else {
			rhoDetector = value;
			break;
		}
	}
	fclose(pf);*/
}

void LBPDetector::LoadRecognizer(const char* filename)
{
	FILE *pf = fopen(filename, "r");
	if (!pf) return;
	recognizer.clear();
	while(1) {
		float value;
		fscanf(pf, "%ff", &value);
		if (EOF != fscanf(pf, ", ")) {
			recognizer.push_back(value);
		}
		else {
			rhoRecognizer = value;
			break;
		}
	}
	fclose(pf);
}

float LBPDetector::CalDetectionScore(const float* histogram) {
	 return CalClassificationScore(detector, histogram);
}

float LBPDetector::CalRecognitionScore(const float* histogram) {
	return CalClassificationScore(recognizer, histogram);
}

float LBPDetector::CalClassificationScore(const std::vector<float>& classifier, const float* histogram) {
	if (classifier.size() == 0) return 1000000.;		//It's still not sure what to return in this cae. To be determined.
	int k;
	int dimension = classifier.size() - 1;
	float score = 0;
	for (k = 0; k < dimension - 4; k += 4)
	{
		score += histogram[k] * classifier[k] 
			+ histogram[k+1] * classifier[k+1]
			+ histogram[k+2] * classifier[k+2]
			+ histogram[k+3] * classifier[k+3];
	}

	for( ; k < dimension; k++ )
	{
		score += histogram[k] * classifier[k];
	}
	return score;
}