#ifndef ACV_VIDEO_OUT_H
#define ACV_VIDEO_OUT_H

#include "core/module.h"

#include <opencv2/opencv.hpp>
#include <QtCore/qdatetime.h>


class QThread;

namespace acv {

class frame_sink;

class video_out : public module {
	Q_OBJECT

	double const DEFAULT_FPS = 25;

public:
	video_out();
	~video_out();

public slots:
	// External
	void start() override;
	void stop() override;
	void setopt(QString const& o, QString const& v) override;
	void frame(cv::Mat const& f, double ts);

signals:
	// External
	void started();
	void stopped();
	void error(QDateTime const& ts, QString const& e);

private slots:
	// Internal
	void on_sink_started();
	void on_sink_stopped();
	void on_sink_error(QDateTime const& ts, QString const& e);

private:
	void connect_to_sink(frame_sink* s);

private:
	QString			dest_;
	double			fps_	= DEFAULT_FPS;
	unsigned		w_		= 640;
	unsigned 		h_		= 480;
	double 			max_duration_	= 0;

	frame_sink*		fs_			= NULL;
	QThread*		workthread_	= NULL;
};

}


#endif //ACV_VIDEO_OUT_H
