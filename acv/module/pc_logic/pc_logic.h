#ifndef ACV_PC_LOGIC_H
#define ACV_PC_LOGIC_H

#include "core/module.h"

#include <vector>

#include <opencv2/opencv.hpp>

#include "bgfg_vibe.hpp"
#include "target.h"
#include "utilities.h"


namespace BoatDetection {
class MotionDetector;
}
class TrackingManager;

namespace acv {

class pc_logic : public module {
	Q_OBJECT

	enum state {
		STOPPED,
		WAIT_FOR_FIRST_FRAME,
		RUNNING
	};

public:
	pc_logic();
	~pc_logic();

public slots:
	void start() override;
	void stop() override;
	void setopt(QString const& o, QString const& v) override;
	void on_frame(cv::Mat const& f, double ts);

	void vi_started();

signals:
	// External
	void vi_start();
	void vi_stop();
	void viz(cv::Mat const& f, double ts);

private:
	state state_		= STOPPED;
	BoatDetection::MotionDetector*	md_			= NULL;
	TrackingManager*				trackmgr_	= NULL;
	std::vector<CntIoInfo>			cntios_;

	bgfg_vibe*	bgfg_ = NULL;
	cv::Size	mdsize_{160, 120};
	cv::Mat		prevframe_;
	double		prevts_ = 0;
	long		iframe_ = 0;
	int			skipframes_ = 0;

	std::vector<Target> prevobjects;
};

}


#endif //ACV_PC_LOGIC_H
