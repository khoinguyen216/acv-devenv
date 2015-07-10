#include "rtsp_source.h"

#include <cassert>

#include <QDebug>
#include <QTimer>

#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <UsageEnvironment.hh>

#include "rtsp_handlers.h"
#include "sched_thread.h"
#include "MPRTSPClient.h"

rtsp_source::rtsp_source(QString const& s) : frame_source(s),
	timer_(new QTimer(this))
{
	sched_thread_ = new sched_thread();
}

rtsp_source::~rtsp_source()
{
	delete sched_;
	delete[] username_;
	delete[] password_;
	delete auth_;
}

void rtsp_source::start()
{
	if (client_ != nullptr)
		return;

	sched_	= BasicTaskScheduler::createNew();
	env_	= BasicUsageEnvironment::createNew(*sched_);

	// Validate and parse RTSP URL
	NetAddress	addr;
	portNumBits	port;
	bool url_ok = RTSPClient::parseRTSPURL(*env_,
							source_.toStdString().c_str(),
							username_, password_, addr, port, nullptr);
	if (!url_ok) {
		 emit error(QDateTime::currentDateTime(),
				 QString("%1 is malformed").arg(source_));
		 return;
	}

	// Remove username:password from the URL and put them into authenticator
	QString pattern = QString("//%1:%2@").arg(username_).arg(password_);
	source_.replace(pattern, "//");

	// Start scheduler and client
	sched_thread_->setScheduler(sched_);
	sched_thread_->enableScheduler();
	sched_thread_->start();

	client_ = MPRTSPClient::createNew(*env_, source_.toStdString().c_str(),
				this, 2, "kclient");
	if (client_ == nullptr) {
		emit error(QDateTime::currentDateTime(),
				QString("Could not create client for %1").arg(source_));
		return;
	}

	auth_ = new Authenticator(username_, password_);
	client_->sendDescribeCommand(after_describe, auth_);
	start_ts_.start();
	emit started();
}

void rtsp_source::stop()
{
	sched_thread_->disableScheduler();
	sched_thread_->wait();
	shutdown(client_);
	Medium::close(client_);
	client_ = nullptr;
	env_->reclaim();
	delete sched_; sched_ = nullptr;
	emit stopped();
}
