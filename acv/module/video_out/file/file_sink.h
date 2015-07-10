#ifndef ACV_FILE_SINK_H
#define ACV_FILE_SINK_H

#include "../frame_sink.h"

namespace acv {

class file_sink : public frame_sink {
	Q_OBJECT

public:
	file_sink(QString const& dest, double fps, unsigned w, unsigned h);
	~file_sink();

public slots:
	void start() override;
	void stop() override;
	void frame(cv::Mat const& f, double ts);

signals:
	void started() override;
	void stopped() override;
	void error(QDateTime const& ts, QString const& e) override;

private:
	cv::VideoWriter* writer_;
};

}


#endif //ACV_FILE_SINK_H
