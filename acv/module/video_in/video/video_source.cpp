#include "video_source.h"

#include <cassert>

#include <QDebug>


video_source::video_source(QString const& s) : frame_source(s),
	timer_(new QTimer(this))
{
	assert(!s.isEmpty());

	connect(timer_, SIGNAL(timeout()), this, SLOT(next()));
}

video_source::~video_source()
{
	delete cap_;
}

void video_source::start()
{
	if (cap_ != 0)
		return;

	cap_ = new cv::VideoCapture(source_.toStdString());
	if (cap_->isOpened()) {
		double fps = cap_->get(CV_CAP_PROP_FPS);
		if (fps < 0.1) fps = DEFAULT_FPS;
		timer_->setInterval(1000 / fps);
		timer_->start();
		emit started();
	} else {
		delete cap_;
		cap_ = 0;
		emit error(QDateTime::currentDateTime(),
				QString("Could not open %1").arg(source_));
	}
}

void video_source::stop()
{
	if (cap_) {
		timer_->stop();
		cap_->release();
		delete cap_; cap_ = 0;
		emit stopped();
	}
}

void video_source::next()
{
	assert(cap_ != 0 && cap_->isOpened());

	cv::Mat f;
	*cap_ >> f;

	if (f.empty()) {
		emit error(QDateTime::currentDateTime(), "Frame is empty");
		stop();
		return;
	}

	double ts = cap_->get(CV_CAP_PROP_POS_MSEC) / 1000;
	emit frame(f, ts);
}
