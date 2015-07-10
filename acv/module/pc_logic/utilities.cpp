#include <assert.h>
#include <iostream>
#include <fstream>
#include <queue>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include "utilities.h"
#include <iomanip>
#include <stack>
#include "ImageEnhancement.h"

#define NOMINMAX  // To prevent from using Windows' min and max macros

#ifdef _WIN32
#include <winsock2.h>
#include "windows.h"
#endif
#include <algorithm>

#include <opencv2/opencv.hpp>
#include "ssim.h"

using namespace boost::program_options;

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace prop = boost::property_tree;

using std::cout;
using std::endl;
using std::queue;

Options g_options;

Point2f	g_origPoints[4] = {Point2f(246, 64), Point2f(372, 64), Point2f(704, 354), Point2f(0, 354)};
Point2f	g_destPoints[4] = {Point2f(0, 0), Point2f(704, 0), Point2f(704, 290), Point2f(0, 290)};
Mat		g_homography;

std::stack<int64> tictoc_stack;

vector<CntIoInfo> g_cntios;

Mat g_backgroundImage;


void tic(void)
{
    tictoc_stack.push(cv::getTickCount());
}

void toc(void)
{
	if (tictoc_stack.size() == 0) return;
    std::cout << "Time elapsed: "
              << ( cv::getTickCount() - tictoc_stack.top()) / cv::getTickFrequency()
              << "s" << std::endl;
    tictoc_stack.pop();
}


double GetTimeStamp(void)
{
	// getTickCount gives you the number of clock cycles after a certain event, eg, after machine is switched on
	// We set the original point of time as the time when this function was firt called
	static int64 start = cv::getTickCount();
	return ((cv::getTickCount() - start) / cv::getTickFrequency());
}

double GetTimeStamp(int frameIndex, double fps)
{
	// getTickCount gives you the number of clock cycles after a certain event, eg, after machine is switched on
	return (frameIndex/fps);
}


bool IsVideoStream(const string& url)
{
	if (url.find("h264/rtsp://") == 0 || url.find("mjpg/rtsp://") == 0
		|| url.find("http://") == 0)
		return true;
	return false;
}

bool IsIdentical(const Mat& img1, const Mat& img2)
{
	if (!img1.data || !img2.data) return false;
	Mat diff;
	cv::compare(img1, img2, diff, CMP_NE);
	long n = 0;
	for (int i = 0; i < diff.rows; i++) {
		for (int j = 0; j < diff.cols; j++) {
			cv::Vec3b pixel = diff.at<cv::Vec3b>(i, j);
			if (pixel[0] == 255 || pixel[1] == 255 || pixel[2] == 255)
				++n;
		}
	}
	if (n > 10)
		return false;
	else
		return true;
}

void ComputeHomography(void)
{
	vector<Point2f> orig(g_origPoints, g_origPoints+sizeof(g_origPoints)/sizeof(g_origPoints[0]));
	vector<Point2f> dest(g_destPoints, g_destPoints+sizeof(g_destPoints)/sizeof(g_destPoints[0]));

	g_homography = cv::getPerspectiveTransform(orig, dest);
}

void SaveImage(const Mat& image, string folderPath, int frameID)
{
	stringstream ss;
	ss << folderPath << "/" << setw(6) << setfill('0') << frameID << ".jpg";
	imwrite(ss.str(), image);
}

void GetTimeString4Benny(char *str)
{
	namespace pt = boost::posix_time;

	pt::ptime now;
	pt::time_duration td;
	long rounded, mills;

	now = pt::microsec_clock::local_time();
	td = now.time_of_day();
	rounded = (td.hours() * 3600 + td.minutes() * 60 + td.seconds()) * 1000;
	mills = td.total_milliseconds() - rounded;

	sprintf(str, "%04d%02d%02d%02d%02d%02d%03d",
			now.date().year(), now.date().month(), now.date().day(),
			td.hours(), td.minutes(), td.seconds(), mills);
}

void Undistort_wfl( InputArray _src, OutputArray _dst, InputArray _cameraMatrix,
                    InputArray _distCoeffs, InputArray _newCameraMatrix = noArray()  )
{
    Mat src = _src.getMat(), cameraMatrix = _cameraMatrix.getMat();
    Mat distCoeffs = _distCoeffs.getMat(), newCameraMatrix = _newCameraMatrix.getMat();

    _dst.create( src.size(), src.type() );

    Mat dst = _dst.getMat();

    CV_Assert( dst.data != src.data );

    int stripe_size0 = min(max(1, (1 << 12) / max(src.cols, 1)), src.rows);
    Mat map1(stripe_size0, src.cols, CV_16SC2), map2(stripe_size0, src.cols, CV_16UC1);

    Mat_<double> A, Ar, I = Mat_<double>::eye(3,3);

    cameraMatrix.convertTo(A, CV_64F);
    if( distCoeffs.data )
        distCoeffs = Mat_<double>(distCoeffs);
    else
    {
        distCoeffs.create(5, 1, CV_64F);
        distCoeffs = 0.;
    }

    if( newCameraMatrix.data )
        newCameraMatrix.convertTo(Ar, CV_64F);
    else
        A.copyTo(Ar);

    double v0 = Ar(1, 2);
    for( int y = 0; y < src.rows; y += stripe_size0 )
    {
        int stripe_size = min( stripe_size0, src.rows - y );
        Ar(1, 2) = v0 - y;
        Mat map1_part = map1.rowRange(0, stripe_size),
            map2_part = map2.rowRange(0, stripe_size),
            dst_part = dst.rowRange(y, y + stripe_size);

        initUndistortRectifyMap( A, distCoeffs, I, Ar, Size(src.cols, stripe_size),
                                 map1_part.type(), map1_part, map2_part );
        remap( src, dst_part, map1_part, map2_part, INTER_LINEAR, BORDER_CONSTANT, 0);
    }
	/*imshow("dst", dst);
	waitKey(0);*/
}

/* Image undistortion. */
static float gs_t[] = {  560.14635, 0,  659.75681, 0, 560.32476,  390.05203, 0, 0, 1};
static float gs_c[] = {-0.31103,  0.06579, 0.00235,-0.00716, 0.00000};

bool LoadUndistortParameters(string const& path)
{
	size_t i;
	ifstream f(path);

	if (!f.is_open())
		return false;

	for (i = 0; i < sizeof(gs_t)/sizeof(gs_t[0]); i++)
		if (!(f >> gs_t[i])) {
			f.close();
			return false;
		}
	for (i = 0; i < sizeof(gs_c)/sizeof(gs_c[0]); i++)
		if (!(f >> gs_c[i])) {
			f.close();
			return false;
		}

	f.close();
	return true;
}

void Undistort(const Mat& image, Mat& undistorted)
{
	// camera_matrix=[fx 0 cx; 0 fy cy; 0 0 1]
	//Vivotek
	/*float t[] = {835.98033/2.0, 0, 664.18889/2.0, 0, 835.72336/2.0, 476.56859/2.0, 0, 0, 1};
	float c[] = { -0.33087, 0.11424, 0.00037, -0.00016, 0.00000}; */
	//Dahua first day


	cv::Mat cameraMatrix(3, 3, CV_32F, gs_t);
	cv::Mat distCoeffs(5, 1, CV_32F, gs_c);
	//cv::undistort(image, undistorted, cameraMatrix, distCoeffs);
	Undistort_wfl(image, undistorted, cameraMatrix, distCoeffs);
}

void OutputStream2Image(Mat& image)
{
	static std::stringstream ss("");	//subffer needs to be a global variable
	std::streambuf * old = std::cout.rdbuf(ss.rdbuf());

	string line;
	int fontFace = FONT_HERSHEY_SIMPLEX;
	double fontScale = 0.3;
	int thickness = 1, baseline = 0;
	Size textSize = getTextSize(ss.str(), fontFace,	fontScale, thickness, &baseline);
	int nlines = 1, margin = 10;
	while(std::getline(ss,line,'\n')){
		Point textOrg(0, nlines*(textSize.height+margin));
		putText(image, line, textOrg, fontFace, fontScale,
			CV_RGB(255,255,255), thickness, 8);// then put the text itself
			++nlines;
	}
	ss.str(""); ss.clear();
}

void IncreaseContrast(const Mat& image, Mat & new_image)
{
	 /*
	 new_image = Mat::zeros( image.size(), image.type() );

	 /// Initialize values
	 float alpha, beta;
	 alpha = 3; 	 beta = 50; //For xuguan night videos

	 /// Do the operation new_image(i,j) = alpha*image(i,j) + beta
	 for( int y = 0; y < image.rows; y++ )
		{ for( int x = 0; x < image.cols; x++ )
			 { for( int c = 0; c < 3; c++ )
				  {
		  new_image.at<Vec3b>(y,x)[c] =
			 saturate_cast<uchar>( alpha*( image.at<Vec3b>(y,x)[c] ) + beta );
				 }
		}
    }
	*/
	// Histogram equalization
	/*tic();
	Mat gray, gray1;
	cvtColor(image, gray, CV_BGR2GRAY);
	equalizeHist(gray, gray1);
	Mat table[] = {gray1, gray1, gray1};
	merge(table, 3, new_image);
	toc();*/

	// Increase contrast
	//tic();
	float strength = 3.0;
	Mat tmp = image.clone();

	ImageEnhancement ie(image.cols, image.rows);
	ie.Enhance((int *)image.data, image.step, (int*)tmp.data, tmp.step, image.cols, image.rows, strength);
	new_image = tmp.clone();
	//toc();

}

void ParseCommandLine(const int argc, const char* argv[])
{
	variables_map vm;
	bool isEffective = false;
	try {
		options_description optSect("Help information.");
		optSect.add_options()
			("help", "Show help information.")
			("n", "Switch to night mode. By default we use day mode. This is only working for video files. For live video stream, we determine it based on local time on machine.")
			("nt", value<string>(), "The night time period. Format: \"#,#\", 18,6 means starting from 18 ending at 6 o'clock.")
			("sun", "Force automatic day/night mode switching using sunrise/sunset.")
			("lat", value<double>(), "Current latitude")
			("lon", value<double>(), "Current longitude")
			("grid", value<string>(), "Enable grid. Cell size is given as WxH.")
			("vizroi", value<string>(), "Visualization ROI. Dimension is given as top,left,width,height.")
			("vizroigrid", value<string>(), "Enable grid in Visualization ROI. Dimension is given as rowsxcols.")
			("showgrid", "Show grid.")
			("nosplit", "Disable ship splitting.")
			("rt", "Show ship information in every frame.")
			("sh", "Output ship history to text file.")
			("camts", "Show camera timestamp.")
			("i", value<string>(), "The input video file path or URLs in form of either \"h264/rtsp://\" or \"mjpg/rtsp://\".")
			("s", value<string>()->implicit_value("V"), "Whether to save results to output folder. \"V\" to save result image into a video, \"S\" to save ship segmentaion, and \"B\" to save both.")
			("o", value<string>(), "The output folder path.")
			("l", value<float>()->default_value(0.15), "The threshold of ship length. The ship is supposed to be long enough. The fraction of the image length.")
			("r", value<string>()->default_value("result.txt"), "Save counting result to a text file. It should be a full path.")
			("t", value<float>()->default_value(0.0), "Ship recognition: the threshold for ship recognition. The object will be neglected with recognition score lower than this value.")
			("h", value<int>()->default_value(10), "Motion detection: the number of frames for movement history analysis, to remove irregular movement (in direction), e.g., river ripple.")
			("a", value<int>()->default_value(0), "Motion detection: the interested direction for moving objects, (with \"ar\") to specify the angle range to be processed. In degree. 0 for horizontal movment, and 90 for vertical ones.")
			("ar", value<int>()->default_value(15), "Motion detection: the angle torlerance range for movming objects. The greater it is the wider angle range will be accepted. The unit is in degree.")
			("v", value<float>()->default_value(0.5), "Motion detection: the velocity threshold. The object will be neglected if its velocity is lower than this value.")
			("cc", value<int>()->default_value(200), "Motion detection: the area threshold. The object will be neglected if its area (in pixel) is lower than this value.")
			("debug", "Switch on to show debug information including the intermediate result windows.")
			("lookdown", "Looking down view angle when the camera is mounted on bridge. In this case, the detection, tracking methods (except association) will be different.")
			("mt", "Turn on multithreading.")
			("httpport", value<uint16_t>(), "HTTP server port. If this is specified, the internal mjpg server will be used.")
			("httpmaxclients", value<uint32_t>()->default_value(1), "Maximum number of concurrent clients that HTTP server handles.")
			("rtspport", value<uint16_t>(), "RTSP server port. If this is specified, the internal rtsp server will be used.")
			("calib", value<string>(), "Camera calibration parameter file.")
			("dump", value<string>(), "\"ship\": save image patch for each ship. \"taggedship\": save ship image with information.");
			;
		options_description all("Copy right of KAI Square Pte Ltd 2014.\nShip counting module. ");
		all.add(optSect);
		parsed_options parsed=command_line_parser (argc, argv).options(all)
			.style(
			command_line_style::allow_short |
			command_line_style::allow_long| command_line_style::short_allow_adjacent |command_line_style:: short_allow_next
			| command_line_style::long_allow_adjacent |command_line_style:: long_allow_next
			| command_line_style::allow_sticky |command_line_style:: allow_dash_for_short|command_line_style::allow_long_disguise)
			.run();

		store(parsed, vm);
		vector<string> unrecognized;
		unrecognized=collect_unrecognized(parsed.options,include_positional);
		if(unrecognized.size()>0)
		{
			cout<<"unrecognized parameters"<<endl;
			exit(23);
		}
		if (argc < 2 || vm.count("help"))
		{
			cout << all;
			exit(-1);
		}
		if (vm.count("n"))
			g_options.isNightMode = true;
		if (vm.count("nt")) {
			string str = vm["nt"].as<string>();
			istringstream ss(str);
			string item;
			getline(ss, item, ',');
			g_options.nightStart = atoi(item.c_str());
			getline(ss, item, ',');
			g_options.nightEnd = atoi(item.c_str());
		}
		if (vm.count("sun")) {
			g_options.useSuntime = true;
			g_options.isNightMode = false;
		} else {
			g_options.useSuntime = false;
		}
		if (vm.count("lat")) {
			g_options.lat = vm["lat"].as<double>();
		}
		if (vm.count("lon")) {
			g_options.lon = vm["lon"].as<double>();
		}
		if (vm.count("grid")) {
			stringstream scanner(vm["grid"].as<string>());
			size_t w, h;
			char c;
			scanner >> w >> c >> h;
			g_options.gridCellSize = Size(w, h);
		}
		if (vm.count("vizroi")) {
			stringstream scanner(vm["vizroi"].as<string>());
			int x, y, w, h;
			char c;
			scanner >> x >> c >> y >> c >> w >> c >> h;
			g_options.vizroi = cv::Rect(x, y, w, h);
		}
		if (vm.count("vizroigrid")) {
			stringstream scanner(vm["vizroigrid"].as<string>());
			size_t r, c;
			char sep;
			scanner >> r >> sep >> c;
			g_options.vizroigrid = Size(c, r);
		}
		g_options.grid 		= vm.count("showgrid");

		g_options.realtime	= vm.count("rt") > 0;
		g_options.camtime	= vm.count("camts") > 0;

		g_options.outputShipHistory = vm.count("sh") > 0;

		if (vm.count("debug"))
			g_options.debug = true;
		if (vm.count("i")) {
			g_options.videoUrl =  vm["i"].as<string>();
			isEffective = true;
		}
		if (vm.count("o"))
			g_options.outputFolderpath = vm["o"].as<string>();
		if (vm.count("t"))
			g_options.threshold_detector = vm["t"].as<float>();
		if (vm.count("l"))
			g_options.threshold_length = vm["l"].as<float>();
		if (vm.count("h"))
			g_options.nHistoFrames = vm["h"].as<int>();
		if (vm.count("a"))
			g_options.angle = vm["a"].as<int>();
		if (vm.count("ar"))
			g_options.angleRange = vm["ar"].as<int>();
		if (vm.count("v"))
			g_options.vThresh = vm["v"].as<float>();
		if (vm.count("cc"))
			g_options.areaThresh = vm["cc"].as<int>();
		if (vm.count("s"))
		{
			string option = vm["s"].as<string>();
			if (option == "V") {
				g_options.outputDisplay = true;
				if (g_options.outputFolderpath == "") {
					cout << "The output folder should not be empty" << endl;
					exit(0);
				}
			} else if (option == "S") {
				g_options.outputShip = true;
				if (g_options.outputFolderpath == "") {
					cout << "The output folder should not be empty" << endl;
					exit(0);
				}
			} else if (option == "B") {
				g_options.outputDisplay = g_options.outputShip = true;
				if (g_options.outputFolderpath == "") {
					cout << "The output folder should not be empty" << endl;
					exit(0);
				}
			} else if (option == "CS") {
				g_options.outputCompleteShip = true;
			} else {
					cout << "Invalid value for saving which image" << endl;
			}
		}
		if (vm.count("r"))
			g_options.resultFilepath = vm["r"].as<string>();
		if (vm.count("calib"))
			g_options.calibFilepath = vm["calib"].as<string>();
		if (vm.count("t"))
			g_options.threshold_detector = vm["t"].as<float>();
		if (vm.count("lookdown"))
			g_options.isLookingDown = true;
		if (vm.count("mt"))
			g_options.isMultiThreading = true;
		if (vm.count("httpport")) {
			g_options.httpServerPort = vm["httpport"].as<uint16_t>();
			g_options.httpServerEnabled = true;
		} else {
			g_options.httpServerEnabled = false;
		}
		if (g_options.httpServerEnabled && vm.count("httpmaxclients")) {
			g_options.httpMaxClients = vm["httpmaxclients"].as<uint32_t>();
		}
		if (vm.count("rtspport")) {
			g_options.rtspServerPort = vm["rtspport"].as<uint16_t>();
			g_options.rtspServerEnabled = true;
		} else {
			g_options.rtspServerEnabled = false;
		}
		if (vm.count("dump")) {
			if (vm["dump"].as<string>() == "taggedship") {
				g_options.dumpShip = true;
				g_options.dumpShipWithTagged = true;
			}
			else if (vm["dump"].as<string>() == "ship") {
				g_options.dumpShip = true;
				g_options.dumpShipWithTagged = false;
			} else {
				g_options.dumpShip = false;
				g_options.dumpShipWithTagged = false;
			}
		}
		if (vm.count("nosplit")) {
			g_options.doSplit = false;
		} else {
			g_options.doSplit = true;
		}
	}
	catch(std::exception& e) {
		cout << e.what() << "\n";
		exit(-1);
	}
	if (!isEffective) {
		cout << "The input video stream of file has not been specified. The program will exit..." << endl;
		exit(-1);
	}
}

void print_ptree(boost::property_tree::ptree const& pt)
{
	for (auto it = pt.begin(); it != pt.end(); ++it) {
		cout << it->first << ":" << it->second.get_value<string>() << endl;
		print_ptree(it->second);
	}
}

boost::property_tree::ptree
merge_ptree(boost::property_tree::ptree const& rptFirst, boost::property_tree::ptree const& rptSecond)
{
	// Take over first property tree
	boost::property_tree::ptree ptMerged = rptFirst;

	// Keep track of keys and values (subtrees) in second property tree
	queue<string> qKeys;
	queue<boost::property_tree::ptree> qValues;
	qValues.push( rptSecond );

	// Iterate over second property tree
	while( !qValues.empty() )
	{
		// Setup keys and corresponding values
		boost::property_tree::ptree ptree = qValues.front();
		qValues.pop();
		string keychain = "";
		if( !qKeys.empty() )
		{
			keychain = qKeys.front();
			qKeys.pop();
		}

	// Iterate over keys level-wise
		BOOST_FOREACH( const boost::property_tree::ptree::value_type& child, ptree )
		{
			// Leaf
			if( child.second.size() == 0 )
			{
				// No "." for first level entries
				string s;
				if( keychain != "" )
					s = keychain + "." + child.first.data();
				else
					s = child.first.data();

			// Put into combined property tree
				ptMerged.put( s, child.second.data() );
			}
			// Subtree
			else
			{
				// Put keys (identifiers of subtrees) and all of its parents (where present)
				// aside for later iteration. Keys on first level have no parents
				if( keychain != "" )
					qKeys.push( keychain + "." + child.first.data() );
				else
					qKeys.push( child.first.data() );

				// Put values (the subtrees) aside, too
				qValues.push( child.second );
			}
		}  // -- End of BOOST_FOREACH
	}  // --- End of while

	return ptMerged;
}
void load_config(boost::property_tree::ptree& conf,
	boost::filesystem::path const& defaultp,
	boost::filesystem::path const& userp, int const, char const* [])
{
	prop::ptree def_conf;
	prop::ptree user_conf;

	try {
		/* Read default configs. */
		prop::read_xml(defaultp.string(), def_conf);

		/* Read user config */
		if (fs::exists(userp)) {
			prop::read_xml(userp.string(), user_conf);
		}

		/* Overwrite global_config with local_config */
		conf = merge_ptree(def_conf, user_conf);
	} catch (std::exception const& e) {
		std::cout << e.what() << endl;
	}
}

static std::map<string, cv::Mat> gs_charmap;
void LoadChineseCharacters()
{
	gs_charmap.clear();

	cout << "Load chinese characters" << endl;
	/* Load all character files start with char_. */
	fs::directory_iterator dir_end;
	for (fs::directory_iterator it(fs::current_path()); it != dir_end; ++it) {
		if (it->path().extension() != ".png"
			|| it->path().filename().string().find("char_") != 0)
			continue;

		auto fname = it->path().filename().string();
		auto charname = fname.substr(5, fname.length() - 9);

		Mat charimg = imread(fname);
		if (charimg.data) {
			gs_charmap[charname] = charimg;
			cout << "\t\"" << charname << "\" in " << fname << endl;
		}
	}
}

void PreprocessChineseCharacters(Size const& size,
								Scalar const& fg_color,
								Scalar const& bg_color)
{
	for (auto &p : gs_charmap) {
		auto& im = p.second;

		/* Resize. */
		resize(im, im, size, 0, 0, INTER_CUBIC);

		/* Set foreground and background color. */
		for (size_t r = 0; r < im.rows; ++r) {
			for (size_t c = 0; c < im.cols; ++c) {
				if (im.at<Vec3b>(r, c) == Vec3b(0, 0, 0)) {
					im.at<Vec3b>(r, c)[0] = fg_color[0];
					im.at<Vec3b>(r, c)[1] = fg_color[1];
					im.at<Vec3b>(r, c)[2] = fg_color[2];
				} else {
					im.at<Vec3b>(r, c)[0] = bg_color[0];
					im.at<Vec3b>(r, c)[1] = bg_color[1];
					im.at<Vec3b>(r, c)[2] = bg_color[2];
				}
			}
		}
	}
}

bool GetChineseCharacter(std::string const& name, cv::Mat& img)
{
	if (gs_charmap.count(name) == 0)
		return false;

	img = gs_charmap.at(name);
	return true;
}

bool	LoadShipDimensionLookupTables()
{
	cout << "Load ship dimension lookup tables" << endl;
	if (!LoadDoubleTableFromFile(EMPTY_LENGTH_TABLE, g_options.emptyLengthTable)) {
		cerr << "Could not load lookup table from " << EMPTY_LENGTH_TABLE << endl;
		return false;
	}
	if (!LoadDoubleTableFromFile(EMPTY_WIDTH_TABLE, g_options.emptyWidthTable)) {
		cerr << "Could not load lookup table from " << EMPTY_WIDTH_TABLE << endl;
		return false;
	}
	if (!LoadDoubleTableFromFile(FULL_LENGTH_TABLE, g_options.fullLengthTable)) {
		cerr << "Could not load lookup table from " << FULL_LENGTH_TABLE << endl;
		return false;
	}
	if (!LoadDoubleTableFromFile(FULL_WIDTH_TABLE, g_options.fullWidthTable)) {
		cerr << "Could not load lookup table from " << FULL_WIDTH_TABLE << endl;
		return false;
	}
	return true;
}

bool	LoadDoubleTableFromFile(string const& filep, vector<vector<double>>& t)
{
	ifstream ifs(filep);

	if (!ifs.is_open()) {
		cerr << "Could not open " << filep << "for reading" << endl;
		return false;
	}

	double temp;
	string line;
	while (!ifs.eof()) {
		vector<double> row;
		getline(ifs, line);
		istringstream iss(line);
		while (iss >> temp) {
			row.push_back(temp);
		}
		t.push_back(row);
	}

	return true;
}

void IntializeVideoWriter(VideoWriter& vw, const string& inputFilepath, const string& outputFolder, const Size& frameSize)
{
	// Get ouput video file path
#ifdef __linux__
	char *fname;
	fname = basename((char *)inputFilepath.c_str());
#else
	char drive[4096], dir[4096], fname[4096], ext[4096];
	_splitpath(inputFilepath.c_str(), drive, dir, fname, ext);
#endif
	string outVideoPath = outputFolder + "/" + fname + "_result" + ".avi";
	if (!vw.open(outVideoPath, CV_FOURCC('F', 'L', 'V', '1'), 5, frameSize, true))
	//if (!vw.open(outVideoPath, -1, 5, frameSize, true))
		cout << "Open file for writing video failed " << endl;

}

bool IsNightByLocalTime(int startHour, int endHour)
{
	pt::ptime now = pt::second_clock::local_time();
	int h = now.time_of_day().hours();
	int m = now.time_of_day().minutes();
	if (h >= startHour || h <= endHour)
		return true;
	return false;
}

namespace pt = boost::posix_time;

long long g_starttime = 0;

void RecordProgramStartTime()
{
	assert(g_starttime == 0);

	pt::ptime now(pt::microsec_clock::local_time());
	pt::ptime epoch(boost::gregorian::date(1970, 1, 1));

	g_starttime = (now - epoch).total_milliseconds();
	cout << "start time " << g_starttime << endl;
}

long GetProgramStartTime()
{
	assert(g_starttime > 0);

	return g_starttime;
}

Point2f GetRectCenter(const Rect rect)
{
	return Point2f(rect.x + rect.width/2.0, rect.y + rect.height/2.0);
}

bool SimilarImagePatch_ssim(const Mat& backgroundImage, const Mat& currImage, const Rect& origBB, const Rect& currBB, float threshold)
{
	Mat ssim, mask;
	Mat origROI, currROI;
	origROI = backgroundImage(currBB);
	currROI = currImage(currBB);
	/*imshow("origROI", origROI);
	imshow("currROI", currROI);*/
	double sim = calcSSIM(origROI, currROI, ssim, mask, 0, CV_BGR2YUV, 0.01, 0.03,255, 24/*origROI.cols*/, 5, 1.5);
	bool result = false;
	cout << "ssim " << sim << endl;
	if (sim > threshold)
		result = true;
	return result;
}
int rndint(int min,int max)
{
	return min + (rand() % (int)(max - min + 1));
}
std::vector<std::string> &ssplit(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while(getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}
std::vector<std::string> ssplit(const std::string &s, char delim) {
	std::vector<std::string> elems;
	return ssplit(s, delim, elems);
}
std::string tostring(int i)
{
	std::stringstream ss;
	ss << i;
	std::string s(ss.str());
	return s;
}
std::string ptime2str(boost::posix_time::ptime pt)
{
	std::string ret;
	boost::posix_time::time_facet* facet = new  boost::posix_time::time_facet("%Y%m%dT%H%M%S%FZ");
	std::stringstream date_stream;
	date_stream.imbue(std::locale(date_stream.getloc(), facet));
	date_stream <<pt;
	ret=date_stream.str();
	// don't explicitly delete facet
	return ret;
}
std::string now2str()
{
	return ptime2str(boost::posix_time::microsec_clock::universal_time());
}
void InitCntios(string strCntios, vector<CntIoInfo>& cntios)
{
	cntios.clear();
	if (strCntios.find(";") == std::string::npos)
	{

		std::vector<std::string> splited = ssplit(strCntios, ',');

		std::vector<cv::Point2f> p1;
		std::vector<cv::Point2f> p2;


		float x1 = atof(splited[0].c_str());
		float y1 = atof(splited[1].c_str());
		float x2 = atof(splited[2].c_str());
		float y2 = atof(splited[3].c_str());

		p1.push_back(Point2f(x1, y1));
		p1.push_back(Point2f(x2, y1));
		p1.push_back(Point2f(x2, y2));
		p1.push_back(Point2f(x1, y2));

		//p2
		x1 = atof(splited[4].c_str());
		y1 = atof(splited[5].c_str());
		x2 = atof(splited[6].c_str());
		y2 = atof(splited[7].c_str());			

		p2.push_back(Point2f(x1, y1));
		p2.push_back(Point2f(x2, y1));
		p2.push_back(Point2f(x2, y2));
		p2.push_back(Point2f(x1, y2));
		CntIoInfo cntio = CntIoInfo(p1, p2);
		cntio.id = tostring((int)0);
		cntios.push_back(cntio);

	}
	else
	{
		std::vector<std::string> splited = ssplit(strCntios, ';');

		std::vector<std::string> sp1 = ssplit(splited[0], ',');
		std::vector<std::string> sp2 = ssplit(splited[1], ',');

		std::vector<cv::Point2f> p1;
		std::vector<cv::Point2f> p2;

		if (sp1.size() == 4)
		{
			float x1 = atof(sp1[0].c_str());
			float y1 = atof(sp1[1].c_str());
			float x2 = atof(sp1[2].c_str());
			float y2 = atof(sp1[3].c_str());

			p1.push_back(Point2f(x1, y1));
			p1.push_back(Point2f(x2, y1));
			p1.push_back(Point2f(x2, y2));
			p1.push_back(Point2f(x1, y2));
		}
		else
		{
			for (size_t p1i = 0; p1i < sp1.size(); p1i += 2)
			{
				float x1 = atof(sp1[p1i].c_str());
				float y1 = atof(sp1[p1i + 1].c_str());
				p1.push_back(Point2f(x1, y1));
			}
		}

		if (sp2.size() == 4)
		{
			float x1 = atof(sp2[0].c_str());
			float y1 = atof(sp2[1].c_str());
			float x2 = atof(sp2[2].c_str());
			float y2 = atof(sp2[3].c_str());			

			p2.push_back(Point2f(x1, y1));
			p2.push_back(Point2f(x2, y1));
			p2.push_back(Point2f(x2, y2));
			p2.push_back(Point2f(x1, y2));
		}
		else
		{
			for (size_t p2i = 0; p2i < sp2.size(); p2i += 2)
			{
				float x1 = atof(sp2[p2i].c_str());
				float y1 = atof(sp2[p2i + 1].c_str());
					
				p2.push_back(Point2f(x1, y1));
			}
		}

		// in case we resize frames
		/*float scale = 0.5;
		for (size_t i = 0; i < p1.size(); i++) {
			p1[i].x *= scale;
			p1[i].y *= scale;
		}
		for (size_t i = 0; i < p2.size(); i++) {
			p2[i].x *= scale;
			p2[i].y *= scale;
		}*/
		CntIoInfo cntio = CntIoInfo(p1, p2);
		cntio.id = tostring((int)0);
		cntios.push_back(cntio);
	}
}

cv::Scalar HSVtoRGBcvScalar(int H, int S, int V) {

	int bH = H; // H component
	int bS = S; // S component
	int bV = V; // V component
	double fH, fS, fV;
	double fR, fG, fB;
	const double double_TO_BYTE = 255.0f;
	const double BYTE_TO_double = 1.0f / double_TO_BYTE;

	// Convert from 8-bit integers to doubles
	fH = (double)bH * BYTE_TO_double;
	fS = (double)bS * BYTE_TO_double;
	fV = (double)bV * BYTE_TO_double;

	// Convert from HSV to RGB, using double ranges 0.0 to 1.0
	int iI;
	double fI, fF, p, q, t;

	if( bS == 0 ) {
		// achromatic (grey)
		fR = fG = fB = fV;
	}
	else {
		// If Hue == 1.0, then wrap it around the circle to 0.0
		if (fH>= 1.0f)
			fH = 0.0f;

		fH *= 6.0; // sector 0 to 5
		fI = floor( fH ); // integer part of h (0,1,2,3,4,5 or 6)
		iI = (int) fH; // " " " "
		fF = fH - fI; // factorial part of h (0 to 1)

		p = fV * ( 1.0f - fS );
		q = fV * ( 1.0f - fS * fF );
		t = fV * ( 1.0f - fS * ( 1.0f - fF ) );

		switch( iI ) {
		case 0:
			fR = fV;
			fG = t;
			fB = p;
			break;
		case 1:
			fR = q;
			fG = fV;
			fB = p;
			break;
		case 2:
			fR = p;
			fG = fV;
			fB = t;
			break;
		case 3:
			fR = p;
			fG = q;
			fB = fV;
			break;
		case 4:
			fR = t;
			fG = p;
			fB = fV;
			break;
		default: // case 5 (or 6):
			fR = fV;
			fG = p;
			fB = q;
			break;
		}
	}

	// Convert from doubles to 8-bit integers
	int bR = (int)(fR * double_TO_BYTE);
	int bG = (int)(fG * double_TO_BYTE);
	int bB = (int)(fB * double_TO_BYTE);

	// Clip the values to make sure it fits within the 8bits.
	if (bR > 255)
		bR = 255;
	if (bR < 0)
		bR = 0;
	if (bG >255)
		bG = 255;
	if (bG < 0)
		bG = 0;
	if (bB > 255)
		bB = 255;
	if (bB < 0)
		bB = 0;

	// Set the RGB cvScalar with G B R, you can use this values as you want too..
	return cv::Scalar(bB,bG,bR); // R component
}