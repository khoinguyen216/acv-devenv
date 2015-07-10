#ifndef FRAME_SOURCE_H
#define FRAME_SOURCE_H

#include <QDateTime>
#include <QObject>
#include <QString>

#include <opencv2/opencv.hpp>


class frame_source : public QObject {
public:
	frame_source(QString const& s) : source_(s) {}
	virtual ~frame_source() {}

private:
	frame_source(frame_source const &);
	frame_source& operator=(frame_source const &);

public slots:
	virtual void start() = 0;
	virtual void stop() = 0;

signals:
	virtual void started() = 0;
	virtual void stopped() = 0;
	virtual void frame(cv::Mat const& f, double ts) = 0;
	virtual void error(QDateTime const& ts, QString const& e) = 0;

protected:
	QString source_;
};

#endif
