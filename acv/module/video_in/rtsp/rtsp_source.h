#include "../frame_source.h"

#include <QTime>


class QTimer;

class sched_thread;
class Authenticator;
class RTSPClient;
class TaskScheduler;
class UsageEnvironment;

class rtsp_source : public frame_source {
	Q_OBJECT

	unsigned const DEFAULT_FPS = 50;

public:
	rtsp_source(QString const& s);
	~rtsp_source();

public slots:
	void start() override;
	void stop() override;

signals:
	void started() override;
	void stopped() override;
	void frame(cv::Mat const& f, double ts) override;
	void error(QDateTime const& ts, QString const& e) override;

private:
	QTimer*				timer_			= nullptr;
	QTime				start_ts_;
	RTSPClient*			client_			= nullptr;
	TaskScheduler*		sched_			= nullptr;
	UsageEnvironment*	env_			= nullptr;
	sched_thread*		sched_thread_	= nullptr;
	char				watchvar_;
	char*				username_		= nullptr;
	char*				password_		= nullptr;
	Authenticator*		auth_			= nullptr;
};
