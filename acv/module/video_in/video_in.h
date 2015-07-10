#ifndef ACV_VIDEO_IN_H
#define ACV_VIDEO_IN_H

#include "core/module.h"

#include <opencv2/opencv.hpp>

#include <QDateTime>


class QThread;
class QTimer;
class frame_source;

namespace acv {

class video_in : public module {
	Q_OBJECT

public:
	video_in();
	~video_in();

public slots:
	// External slots
	void start() override;
	void stop() override;
	void setopt(QString const& o, QString const& v) override;

private slots:
	void on_source_started();
	void on_source_stopped();
	void on_source_frame(cv::Mat const& f, double ts);
	void on_source_error(QDateTime const& ts, QString const& e);
	void on_source_read_timeout();

signals:
	// External signals
	void started();
	void stopped();
	void frame(cv::Mat const& f, double ts);
	void info(QDateTime const& ts, QString const& i);
	void error(QDateTime const& ts, QString const& e);

private:
	void connect_to_source(frame_source* s);

	// Options
	QString		source_;
	bool		recover_			= false;
	double		recover_interval_	= 0;
	double		read_timeout_		= 0;

	QThread*	workthread_	= NULL;
	QTimer*		read_deadline_ 	= NULL;
	QTimer*		recover_timer_	= NULL;
	frame_source* fs_ = NULL;
};

}


#endif //ACV_VIDEO_IN_H
