#ifndef __UTILITIES_WANGFANGLIN__
#define __UTILITIES_WANGFANGLIN__
#include <float.h>
#include <string>
#include <iostream>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/format.hpp>

#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;

#define SEPARATOR "*"
#define SECOND	1

#define N_ELEMS(X)	(sizeof(X) / sizeof(X[0]))

#define DEFAULT_CONFIG	"conf.xml.default"
#define USER_CONFIG		"conf.xml"
#define EMPTY_LENGTH_TABLE	"empty_length_table.txt"
#define EMPTY_WIDTH_TABLE	"empty_width_table.txt"
#define FULL_LENGTH_TABLE	"full_length_table.txt"
#define FULL_WIDTH_TABLE	"full_width_table.txt"

// Calculate the absolute difference of two angles ranged in [0, 360]
// E.g., for 15' and 345', the difference will be 30', instead of 330'
#define DIFF_ANGLE(a1, a2) 180 - abs(abs((a1) - (a2)) - 180)

#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif
//#define MINUS_INFINITE	(-DBL_MAX)
#define MINUS_INFINITE	-1e10


// For looking down angle only, to compute homography matrix
extern Point2f	g_origPoints[4];
extern Point2f	g_destPoints[4];
extern Mat		g_homography;
void ComputeHomography(void);

extern Mat g_backgroundImage;

class CntIoInfo;

extern vector<CntIoInfo> g_cntios;

struct VideoFrame
{
	cv::Mat image;
	double	timeStamp;
};

void ComputeDLBP(const Mat& imagePatch, vector<float>& hist);

struct Options{
	Options():videoUrl(""), outputFolderpath(""), resultFilepath(""), threshold_detector(0.0),
		isNightMode(false), outputDisplay(false), outputShip(false), isMultiThreading(false),
		nHistoFrames(10), angle(0), angleRange(15), vThresh(1.0), areaThresh(400),
		isLookingDown(false), nightStart(18), nightEnd(6), debug(false),threshold_length(0.2)
	{
	}

	string	videoUrl;
	string	outputFolderpath;
	string	resultFilepath;
	string	calibFilepath;

	Size	frameSize;

	float	threshold_detector;
	float	threshold_length;	// The threshold of ship length. The ship should be long enough.
	bool	isNightMode;
	bool	outputDisplay;		// To output the display image
	bool	outputShip;			// To output ship regions. For ship cropping only.
	bool	outputCompleteShip;	// To output full ship image patches.
	bool	isMultiThreading;	// To switch on multithreading
	/*
	 * For motion detection
	 */
	int		nHistoFrames;		// The number of historical frames for motion analysis
	int		angle;				// The motion angle to deal with
	int		angleRange;			// The angle range
	float	vThresh;			// The velocity threshold
	int		areaThresh;			// The area threshold

	bool	debug;				// switch to debug mode to display the intermediate result.
	/*
	 * The starting and ending time of night mode
	 */
	int		nightStart, nightEnd;
	bool	useSuntime;			// Force automatic mode switching using sunrise/sunset.
	double	lat;
	double	lon;

	/* Perspective transform. */
	std::vector<cv::Point2f>	realROI;	// ROI in real world. Described as a trapezoid.
	std::vector<cv::Point2f>	imageROI;	// ROI in image. Described as a rectangle.

	/* Dimension lookup tables. */
	bool						outputShipHistory;
	vector<vector<double>>		emptyLengthTable;
	vector<vector<double>>		emptyWidthTable;
	vector<vector<double>>		fullLengthTable;
	vector<vector<double>>		fullWidthTable;

	/* Grabcut margins. */
	double	grabcutMargins[4];

	/*
	 * The looking down mode
	 */
	bool		isLookingDown;		// Looking down view angle when the camera is mounted on bridge. In this case, the detection, tracking methods (except association) will be different.
	uint16_t 	httpServerPort;	// HTTP server port.
	bool		httpServerEnabled;
	uint32_t	httpMaxClients;

	uint16_t	rtspServerPort;
	bool		rtspServerEnabled;

	bool		dumpShip;		// Dump one image patch for every ship.
	bool		dumpShipWithTagged;

	bool		grid;			// Show grid.
	Size		gridCellSize;

	cv::Rect	vizroi;			// Visualization ROI.
	Size		vizroigrid;		// Visualization ROI grid.

	bool		realtime;		// Show ship information in every frame.
	bool		camtime;		// Show time from camera.

	bool		doSplit;		// Toggle ship splitting.
};

void tic(void);
void toc(void);
// Get time stamp for video stream
double GetTimeStamp(void);
// Get time stamp for video file
double GetTimeStamp(int frameIndex, double fps);

// The command line argument parsing
extern Options g_options;

bool IsIdentical(const Mat& img1, const Mat& img2);

bool IsVideoStream(const string& url);
void SaveImage(const Mat& image, string folderPath, int frameID);

void GetTimeString_Benny(char *str);

bool LoadUndistortParameters(string const& path);
void Undistort(const Mat& image, Mat& undistorted);

void OutputStream2Image(Mat& image);

void ParseCommandLine(const int argc, const char* argv[]);
void print_ptree(boost::property_tree::ptree const&);
boost::property_tree::ptree merge_ptree(boost::property_tree::ptree const&, boost::property_tree::ptree const&);
void load_config(boost::property_tree::ptree&, boost::filesystem::path const&, boost::filesystem::path const&, int const, char const* []);

void	LoadChineseCharacters();
void	PreprocessChineseCharacters(Size const& size,
									Scalar const& fg_color,
									Scalar const& bg_color);
bool	GetChineseCharacter(std::string const& name, cv::Mat& img);

bool	LoadShipDimensionLookupTables();

bool	LoadDoubleTableFromFile(string const& filep, vector<vector<double>>& t);

void IntializeVideoWriter(VideoWriter& vw, const string& inputFilepath, const string& outputFolder, const Size& frameSize);

bool IsNightByLocalTime(int startHour, int endHour);

void RecordProgramStartTime();
long GetProgramStartTime();

void IncreaseContrast(const Mat& image, Mat & new_image);

Point2f GetRectCenter(const Rect rect);

bool SimilarImagePatch_ssim(const Mat& backgroundImage, const Mat& currImage, const Rect& origBB, const Rect& currBB, float threshold = 0.65);
cv::Scalar HSVtoRGBcvScalar(int H, int S, int V);

std::vector<std::string> &ssplit(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> ssplit(const std::string &s, char delim);
std::string tostring(int i);
std::string ptime2str(boost::posix_time::ptime pt);
std::string now2str();
int rndint(int min,int max);

void InitCntios(string strCntios, vector<CntIoInfo>& cntios);

class CntIoInfo
{
public:
	std::string id;
	//cv::Rect r1;
	std::vector<cv::Point2f> p1;
	std::vector<cv::Point2f> p2;
	cv::Point2f c1;//centroid of p1
	cv::Point2f c2;//centroid of p2
	//cv::Rect r2;
	int d1;//p1->p2
	int d2;//p2->p1
	int p1cnt;
	int p2cnt;
	cv::Scalar color;
	CntIoInfo(std::vector<cv::Point2f> _p1,std::vector<cv::Point2f> _p2)
	{
		p1=_p1;
		p2=_p2;
		p1cnt=0;
		p2cnt=0;
		d1=0;
		d2=0;
		color=HSVtoRGBcvScalar(rndint(0,255),255,255);

		c1=cv::Point2f(0,0);
		c2=cv::Point2f(0,0);
		for(size_t i=0;i<p1.size();++i)
			c1=c1+p1[i];
		for(size_t i=0;i<p2.size();++i)
			c2=c2+p2[i];

		c1=cv::Point2f(c1.x/p1.size(),c1.y/p1.size());
		c2=cv::Point2f(c2.x/p2.size(),c2.y/p2.size());
	};

	
	bool  p1contains(cv::Point2f p)
	{
		return pointPolygonTest(p1,p,false)==1;
	}
	bool  p2contains(cv::Point2f p)
	{
		return pointPolygonTest(p2,p,false)==1;
	}
	std::string tojson()
	{
		std::string strtime=now2str();
		std::string jsonstr = boost::str(boost::format("{"
			"\"evt\":\"cntio\","
			"\"id\":\"%s\","
			"\"r1\":%d,"
			"\"r2\":%d,"
			"\"r1tor2\":%d,"
			"\"r2tor1\":%d,"
			"\"time\":\"%s\""
			"}\n")
			%id
			%p1cnt
			%p2cnt
			%d1
			%d2
			%strtime);
		return jsonstr;
	}
};

#endif
