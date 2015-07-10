#ifndef SCHED_THREAD_H
#define SCHED_THREAD_H

#include <QThread>

#include <liveMedia.hh>


class sched_thread : public QThread {
	Q_OBJECT

public:
	sched_thread() {}
	void setScheduler(TaskScheduler* sched) { sched_ = sched; }
	void enableScheduler() { stop_ = 0; }
	void disableScheduler() { stop_ = 1; }

private:
	void run() Q_DECL_OVERRIDE
	{
		sched_->doEventLoop(&stop_);
	}

private:
	TaskScheduler*	sched_;
	char			stop_ = 0;
};

#endif
