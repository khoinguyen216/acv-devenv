#include "video_in.h"

#include <QDebug>
#include <QThread>
#include <QTimer>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "http_mjpg/http_mjpg_source.h"
#include "rtsp/rtsp_source.h"
#include "video/video_source.h"


namespace acv {

module_options video_in_opts {
	{"source", {"Video source (file | http | rtsp)", OPT_STRING}},
	{"recover", {"Recover on error", OPT_BOOLEAN}},
	{"recover_interval", {"Recovery attempt interval in seconds",
						  OPT_DOUBLE}},
	{"read_timeout", {"Read timeout since last frame in seconds", OPT_DOUBLE}}
};

QHash<QString, module_socket> video_in_socks {
	{"control", {false, {
				{"start", "start()"},
				{"stop", "stop()"},
				{"setopt", "setopt(QString const&, QString const&"}}}},
	{"status", {true, {
				{"started", "started()"},
				{"stopped", "stopped()"},
				{"error", "error(QDateTime const&, QString const&)"},
				{"info", "info(QDateTime const&, QString const&)"}}}},
	{"vout", {true, {
				{"frame", "frame(cv::Mat const&, double)"}}}}
};

video_in::video_in()
{
	qRegisterMetaType<cv::Mat>("cv::Mat");

	av_register_all();

	optionlist_ = &video_in_opts;
	socketlist_ = &video_in_socks;

	workthread_ = new QThread(this);
	workthread_->start();

	read_deadline_ = new QTimer(this);
	connect(read_deadline_, &QTimer::timeout,
			this, &video_in::on_source_read_timeout);
	recover_timer_ = new QTimer(this);
}

video_in::~video_in() {

}

void video_in::start() {
	if (fs_ != nullptr)
		return;

	if (source_.startsWith("rtsp://")) {
		fs_ = new rtsp_source(source_);
	} else if (source_.startsWith("http://")) {
		fs_ = new http_mjpg_source(source_);
	} else {
		fs_ = new video_source(source_);
	}
	fs_->moveToThread(workthread_);
	connect_to_source(fs_);
	QMetaObject::invokeMethod(fs_, "start", Qt::QueuedConnection);
}

void video_in::stop() {
	QMetaObject::invokeMethod(fs_, "stop", Qt::QueuedConnection);
}

void video_in::setopt(QString const& o, QString const& v) {
	if (o == "source") {
		source_ = v;
	} else if (o == "recover") {
		recover_ = module_option::to_bool(v, false);
	} else if (o == "recover_interval") {
		recover_interval_ = module_option::to_double(v, recover_interval_);
	} else if (o == "read_timeout") {
		read_timeout_ = module_option::to_double(v, read_timeout_);
		read_deadline_->setInterval(read_timeout_ * 1000);
	} else {
	}
}

void video_in::on_source_started() {
	read_deadline_->start();
	emit started();
}

void video_in::on_source_stopped() {
	read_deadline_->stop();
	delete fs_; fs_ = NULL;
	emit stopped();
}

void video_in::on_source_frame(cv::Mat const& f, double ts) {
	emit frame(f, ts);
	read_deadline_->start();
}

void video_in::on_source_error(QDateTime const& ts, QString const& e) {
	stop();
	emit error(ts, e);
	read_deadline_->stop();
	if (recover_) {
		recover_timer_->singleShot(recover_interval_ * 1000,
								   this, SLOT(start()));
	}
}

void video_in::on_source_read_timeout() {
	on_source_error(QDateTime::currentDateTime(), "Read timeout");
}

void video_in::connect_to_source(frame_source *s) {
	connect(s, SIGNAL(started()), this, SLOT(on_source_started()));
	connect(s, SIGNAL(stopped()), this, SLOT(on_source_stopped()));
	connect(s, SIGNAL(frame(cv::Mat const&, double)),
			this, SLOT(on_source_frame(cv::Mat const&, double)));
	connect(s, SIGNAL(error(QDateTime const&, QString const&)),
			this, SLOT(on_source_error(QDateTime const&, QString const&)));
}

}