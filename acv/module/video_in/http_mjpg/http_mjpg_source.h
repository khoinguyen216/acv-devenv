#include "../frame_source.h"

#include <QTime>


class QTimer;
class MjpgCapture;

class http_mjpg_source : public frame_source {
	Q_OBJECT

	unsigned const DEFAULT_FPS = 50;

public:
	http_mjpg_source(QString const& s);
	~http_mjpg_source();

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
	MjpgCapture*		cap_ = 0;
	QTimer*				timer_;
	QTime				start_ts_;
};
