#ifndef ACV_VIDEO_OUT_H
#define ACV_VIDEO_OUT_H

#include "core/module.h"

#include <opencv2/opencv.hpp>


namespace acv {

class video_out : public module {
	Q_OBJECT

public:
	video_out();
	~video_out();

public slots:
	void start() override;
	void stop() override;
	void setopt(QString const& o, QString const& v) override;
	void on_frame(cv::Mat const& f, double ts);
};

}


#endif //ACV_VIDEO_OUT_H
