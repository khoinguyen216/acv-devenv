#include "file_sink.h"


namespace acv {

file_sink::file_sink(QString const &dest, double fps, unsigned w, unsigned h) :
		frame_sink(dest, fps, w, h) {
}

file_sink::~file_sink() {
	delete writer_;
}

void file_sink::start() {
	writer_ = new cv::VideoWriter();
	cv::Size sz{w_, h_};
	if (writer_->open(dest_.toStdString(),
					  CV_FOURCC('F', 'L', 'V', '1'), fps_, sz, true)) {
		emit started();
	} else {
		delete writer_;
		writer_ = NULL;
		emit error(QDateTime::currentDateTime(),
					QString("Could not open %1 for writing").arg(dest_));
	}
}

void file_sink::stop() {
	if (writer_) {
		writer_->release();
		delete writer_; writer_ = NULL;
		emit stopped();
	}
}

void file_sink::frame(cv::Mat const &f, double ts) {
	writer_->write(f);
}

}