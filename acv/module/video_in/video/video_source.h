#include "../frame_source.h"

#include <QTimer>


class QTimer;

class video_source : public frame_source {
	Q_OBJECT

	unsigned const DEFAULT_FPS = 25;

public:
	video_source(QString const& s);
	~video_source();

public slots:
	void start() override;
	void stop() override;

signals:
	void started() override;
	void stopped() override;
	void frame(cv::Mat const& f, double ts) override;
	void error(QDateTime const& ts, QString const& e) override;

private slots:
	void next();

private:
	cv::VideoCapture*	cap_ = 0;
	QTimer*				timer_;
};
