#ifndef ACV_FRAME_SINK_H
#define ACV_FRAME_SINK_H

#include <QDateTime>
#include <QObject>
#include <QString>

#include <opencv2/opencv.hpp>


namespace acv {

class frame_sink : public QObject {
public:
	frame_sink(QString const& d, double fps, unsigned w, unsigned h) :
			dest_(d), fps_(fps), w_(w), h_(h) {}
	virtual ~frame_sink() {}

// Disable copy constructor and assignment operator
private:
	frame_sink(frame_sink const&);
	frame_sink& operator=(frame_sink const&);

public slots:
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void frame(cv::Mat const&, double ts) = 0;

signals:
	virtual void started() = 0;
	virtual void stopped() = 0;
	virtual void error(QDateTime const& ts, QString const& e) = 0;

protected:
	QString		dest_;
	double 		fps_;
	unsigned	w_;
	unsigned	h_;
};

}

#endif //ACV_FRAME_SINK_H
