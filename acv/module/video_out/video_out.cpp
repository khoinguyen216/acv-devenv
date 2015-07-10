#include "video_out.h"

#include <QDebug>
#include <QThread>

#include "file/file_sink.h"


namespace acv {

module_options video_out_opts {
	{"dest", {"Video destination (file | http | rtsp)", OPT_STRING}},
	{"size", {"Frame size (WxH)", OPT_STRING}},
	{"fps", {"Frames per second", OPT_DOUBLE}},
	{"max_duration", {"Maximum output duration", OPT_DOUBLE}}
};

module_sockets video_out_socks {
	{"control", {false, {
				{"start", "start()"},
				{"stop", "stopped"},
				{"setopt", "setopt(QString const&, QString const&)"}}}},
	{"status", {true, {
				{"started", "started()"},
				{"stopped", "stopped()"},
				{"error", "error(QDateTime const&, QString const&)"},
				{"info", "info(QDateTime const&, QString const&)"}}}},
	{"vin", {false, {
				{"frame", "frame(cv::Mat const&, double)"}}}}
};

video_out::video_out() {
	qRegisterMetaType<cv::Mat>("cv::Mat");

	optionlist_ = &video_out_opts;
	socketlist_ = &video_out_socks;

	workthread_ = new QThread(this);
	workthread_->start();
}

video_out::~video_out() {

}

void video_out::start() {
	if (fs_ != NULL)
		return;

	if (dest_.startsWith("rtsp://")) {
		//fs_ = new rtsp_source(source_);
	} else if (dest_.startsWith("http://")) {
		//fs_ = new http_mjpg_source(source_);
	} else {
		fs_ = new file_sink(dest_, fps_, w_, h_);
	}
	fs_->moveToThread(workthread_);
	connect_to_sink(fs_);
	QMetaObject::invokeMethod(fs_, "start", Qt::QueuedConnection);
}

void video_out::stop() {
	QMetaObject::invokeMethod(fs_, "stop", Qt::QueuedConnection);
}

void video_out::setopt(QString const& o, QString const& v) {
	if (o == "dest") {
		QString trimmed = v.trimmed();
		if (trimmed == dest_)
			return;
		dest_ = trimmed;
	} else if (o == "size") {
		QStringList parts = v.split("x");
		if (parts.size() == 2) {
			w_ = module_option::to_int(parts[0], w_);
			h_ = module_option::to_int(parts[1], h_);
		}
	} else if (o == "fps") {
		fps_ = module_option::to_double(v, fps_);
	} else if (o == "max_duration") {
		max_duration_ = module_option::to_double(v, max_duration_);
	} else {
	}
}

void video_out::frame(cv::Mat const& f, double ts) {
	QMetaObject::invokeMethod(fs_, "frame",
							  Q_ARG(cv::Mat, f),
							  Q_ARG(double, ts));
}

void video_out::connect_to_sink(frame_sink *s) {
	connect(s, SIGNAL(started()), this, SLOT(on_sink_started()));
	connect(s, SIGNAL(stopped()), this, SLOT(on_sink_stopped()));
	connect(s, SIGNAL(error(QDateTime const&, QString const&)),
			this, SLOT(on_sink_error(QDateTime const&, QString const&)));
}

void video_out::on_sink_started() {
	emit started();
}

void video_out::on_sink_stopped() {
	delete fs_; fs_ = NULL;
	emit stopped();
}

void video_out::on_sink_error(QDateTime const &ts, QString const &e) {
	stop();
	emit error(ts, e);
}

}