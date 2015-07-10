#include "video_out.h"

#include <QDebug>


namespace acv {

module_options video_out_opts {
	{"dest", {"Video destination (file | http | rtsp)", OPT_STRING}},
	{"max_duration", {"Maximum output duration", OPT_DOUBLE}}
};

module_sockets video_out_socks {
	{"control", {false, {
				{"start", "start()"},
				{"stop", "stopped"},
				{"config", "config(QString const&, QString const&)"}}}},
	{"status", {true, {
				{"started", "started()"},
				{"stopped", "stopped()"},
				{"error", "error(QDateTime const&, QString const&)"},
				{"info", "info(QDateTime const&, QString const&)"}}}},
	{"vin", {false, {
				{"frame", "on_frame(cv::Mat const&, double)"}}}}
};

video_out::video_out() {
	qRegisterMetaType<cv::Mat>("cv::Mat");

	optionlist_ = &video_out_opts;
	socketlist_ = &video_out_socks;
	qDebug() << "video_out";
}

video_out::~video_out() {

}

void video_out::start() {

}

void video_out::setopt(QString const& o, QString const& v) {

}

void video_out::stop() {

}

void video_out::on_frame(cv::Mat const& f, double ts) {
	qDebug() << objectName() << "received frame";
}

}