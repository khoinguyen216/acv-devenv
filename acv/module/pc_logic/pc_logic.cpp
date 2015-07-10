#include "pc_logic.h"

#include <QDebug>

#include "bgfg_vibe.hpp"
#include "pc_event.h"
#include "MotionDetection.h"
#include "TrackingManager.h"


using std::vector;
using BoatDetection::MotionDetector;

namespace acv {

module_options pc_logic_opts{
		{"mdsize", {"Motion detection frame size (WxH)", OPT_STRING}},
		{"cntio", {"Cntio", OPT_STRING}}
};

QHash<QString, module_socket> pc_logic_socks{
		{"control", {false, {
				{"start", "start()"},
				{"stop", "stop()"},
				{"config", "config(QString const&, QString const&"}}}},
		{"vi_control", { true, {
				{"start", "vi_start()"},
				{"stop", "vi_stop()"}}}},
		{"vi_status", {false,  {
				{"started", "vi_started()"},
				{"stopped", "stopped()"},
				{"error", "error(QDateTime const&, QString const&)"},
				{"info", "info(QDateTime const&, QString const&)"}}}},
		{"vin",    {false,  {
				{"frame", "on_frame(cv::Mat const&, double)"}}}},
		{"viz",    {true,  {
				{"frame", "viz(cv::Mat const&, double)"}}}}
};

pc_logic::pc_logic() {
	qRegisterMetaType<cv::Mat>("cv::Mat");
	optionlist_ = &pc_logic_opts;
	socketlist_ = &pc_logic_socks;
}

pc_logic::~pc_logic() {

}

void pc_logic::start() {
	if (state_ != STOPPED)
		return;

	qDebug() << objectName() << "starting";

	emit vi_start();
	state_ = WAIT_FOR_FIRST_FRAME;
}

void pc_logic::stop() {
	delete md_; md_ = NULL;
	state_ = STOPPED;
}

void pc_logic::setopt(QString const& o, QString const& v) {
	if (o == "mdsize") {
		QStringList parts = v.split("x");
		if (parts.size() == 2) {
			mdsize_ = cv::Size(module_option::to_int(parts[0], mdsize_.width),
							   module_option::to_int(parts[1], mdsize_.width));
		}
	} else if (o == "cntio") {
		InitCntios(v.toStdString(), cntios_);
	} else {
	}
}

void pc_logic::on_frame(cv::Mat const& f, double ts) {
	if (state_ == WAIT_FOR_FIRST_FRAME) {
		md_ = new MotionDetector(3, 0, 360, 0.5, 90, 20 * 20, 0);
		trackmgr_ = new TrackingManager();
		bgfg_ = new bgfg_vibe();
		prevobjects.clear();
		prevts_ = ts;

		cv::resize(f, prevframe_, mdsize_);
		IncreaseContrast(prevframe_, prevframe_);

		trackmgr_->UpdateBackgroundImage(prevframe_, 0);
		bgfg_->init_model(prevframe_);

		state_ = RUNNING;
		return;
	}
	if (state_ != RUNNING)
		return;

	cv::Mat currframe;
	cv::resize(f, currframe, mdsize_);
	IncreaseContrast(currframe, currframe);

	cv::Mat fg_vibe = *(bgfg_->fg(currframe));

	// Draw cntio
	Mat disp_frame = currframe.clone();
	for (int k = 0; k < cntios_.size(); k++) {
		Point p1 = Point(cntios_[k].p2[0].x * currframe.cols,
						 cntios_[k].p2[0].y *
						 currframe.rows);
		Point p2 = Point(cntios_[k].p2.back().x * currframe.cols,
						 cntios_[k].p2.back().y *
						 currframe.rows);
		cv::line(disp_frame, p1, p2, CV_RGB(0, 255, 0), 1);
		for (int i = 0; i < cntios_[k].p1.size() - 1; i++) {
			Point p1 = Point(cntios_[k].p2[i].x * currframe.cols,
							 cntios_[k].p2[i].y *
							 currframe.rows);
			Point p2 = Point(cntios_[k].p2[i + 1].x * currframe.cols,
							 cntios_[k].p2[i + 1].y *
							 currframe.rows);
			cv::line(disp_frame, p1, p2, CV_RGB(0, 255, 0), 1);
		}
	}

	vector<Target> objects;
	md_->MotionDetection(prevframe_, currframe, objects, prevts_, ts, true);
	md_->MotionCorrection(prevobjects, objects, prevts_, ts);

	trackmgr_->MotionCorrection(objects, prevts_, ts);
	trackmgr_->LowLevelAssociation(currframe, objects, prevts_);
	trackmgr_->CorrectLowLevelAssociation(currframe, fg_vibe, prevts_, ts);
	trackmgr_->SimpleMidLevelAssociation(currframe, prevts_, ts, 1);
	trackmgr_->UpdateTrackers(currframe, ts);
	trackmgr_->UpdateBackgroundImage(currframe, objects.size());
	trackmgr_->Visualize(disp_frame);

	pc_event evt{};
	for (int i = 0; i < objects.size(); i++) {
		cv::rectangle(disp_frame, objects[i].bb, CV_RGB(0, 0, 255), 2);
		// Draw direction
		Point2f center = GetRectCenter(objects[i].bb);
		float r = 15;
		float alpha = objects[i].direction*M_PI/180-M_PI;
		Point2f dis(r*cos(alpha), r*sin(alpha));
		cv::line(disp_frame, center, dis+center, CV_RGB(0, 255, 0), 2);

		Target const& t = objects[i];

		evt.add_person(i, t.bb.x, t.bb.y, t.bb.width, t.bb.height);

		//Rect bb = Rect(0, 0, currFrame.cols, currFrame.rows);
		//bb &= objects[i].bb;
		//float score = DLBPClassification(currFrame(bb), peopleDetector);
		//cout << "class score " << score << endl;
	}
	if (objects.size() > 0)
		qDebug() << evt.to_json();

	// Combine all visualizations into one
	Mat wI;
	float displayScale = 1.0;
	if (true) {
		wI = Mat::zeros(2*disp_frame.rows, 2*disp_frame.cols, CV_8UC3);
		// Orignial overlayed with results
		disp_frame.copyTo(wI(Rect(0, 0, disp_frame.cols, disp_frame.rows)));

		// Optical flow
		//md.GetOptflowVisual().copyTo(wI(Rect(0, displayFrame.rows, displayFrame.cols, displayFrame.rows)));

		// Foreground from vibe
		Mat vVibe;
		cv::cvtColor(fg_vibe, vVibe, CV_GRAY2BGR);
		if (!fg_vibe.empty())
			vVibe.copyTo(wI(Rect(0, disp_frame.rows, disp_frame.cols, disp_frame.rows)));
		const Mat& mask = md_->GetMaskVisual();
		Mat vmask;
		cvtColor(mask, vmask, CV_GRAY2BGR);
		// Motion segmentation result
		if (vmask.data)	vmask.copyTo(wI(Rect(disp_frame.cols, 0, disp_frame.cols, disp_frame.rows)));

		cv::resize(wI, wI, Size(0, 0), displayScale, displayScale);

		imshow("result", wI);
		emit viz(wI, ts);
	}

	prevframe_ = currframe;
	prevts_ = ts;
	prevobjects.clear();
	prevobjects = objects;
}

void pc_logic::vi_started() {
}

}

