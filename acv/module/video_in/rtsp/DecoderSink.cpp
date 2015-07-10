#include "DecoderSink.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include <QDebug>


DecoderSink* DecoderSink::createNew(UsageEnvironment&	env,
									MediaSubsession&	subsession,
									void*				consumer,
									char const*			streamid)
{
	return new DecoderSink(env, subsession, consumer, streamid);
}

DecoderSink::DecoderSink(UsageEnvironment&	env,
						MediaSubsession&	subsession,
						void				*consumer,
						char const*			streamid) : MediaSink(env),
	consumer_(consumer),
	subsession_(subsession)
{
	streamid_ = strdup(streamid);
	recvbuf_ = new uint8_t[RECV_BUFSZ];

	init_decoder();
	recvbuf_[0] = recvbuf_[1] = recvbuf_[2] = 0; recvbuf_[3] = 1;
}

DecoderSink::~DecoderSink()
{
	 delete[] recvbuf_;
	 free(streamid_);
	 if (decoderctx_) {
		 avcodec_close(decoderctx_);
	 }
	 av_frame_free(&decoded_frame_);
	 av_frame_free(&rgb_frame_);
}

void DecoderSink::init_decoder()
{
	av_init_packet(&packet_);
	decoded_frame_	= av_frame_alloc();
	rgb_frame_		= av_frame_alloc();

	if (strcmp(subsession_.codecName(), "JPEG") == 0) {
		decoder_ = avcodec_find_decoder(CODEC_ID_MJPEG);
		assert(decoder_ != nullptr);

		decoderctx_ = avcodec_alloc_context3(decoder_);
		if (decoder_->capabilities & CODEC_CAP_TRUNCATED)
			decoderctx_->flags |= CODEC_CAP_TRUNCATED;
		decoderctx_->flags2 |= CODEC_FLAG2_CHUNKS;
		decoderctx_->thread_count = 2;

		int error = avcodec_open2(decoderctx_, decoder_, nullptr);
		assert(error >= 0);

		return;
	}

	// Create PPS/SPS from SpropParameterSets from DESCRIBE
	unsigned nrecords;
	SPropRecord *spr = parseSPropParameterSets(
			subsession_.fmtp_spropparametersets(), nrecords);
	size_t pos = 0;
	for (unsigned i = 0; i < nrecords; ++i) {
		SPropRecord& rec = spr[i];
		recvbuf_[pos++] = 0;
		recvbuf_[pos++] = 0;
		recvbuf_[pos++] = 0;
		recvbuf_[pos++] = 1;
		memcpy(recvbuf_ + pos, rec.sPropBytes, rec.sPropLength);
		pos += rec.sPropLength;
	}

	// Feed PPS/SPS to the decoder
	decoder_ = avcodec_find_decoder(CODEC_ID_H264);
	assert(decoder_ != nullptr);

	decoderctx_ = avcodec_alloc_context3(decoder_);
	if (decoder_->capabilities & CODEC_CAP_TRUNCATED)
		decoderctx_->flags |= CODEC_CAP_TRUNCATED;
	decoderctx_->flags2 |= CODEC_FLAG2_CHUNKS;
	decoderctx_->thread_count = 2;

	int error = avcodec_open2(decoderctx_, decoder_, nullptr);
	assert(error >= 0);

	packet_.data = recvbuf_;
	packet_.size = pos;
	while (packet_.size > 0) {
		int got_frame;
		int len = avcodec_decode_video2(decoderctx_, decoded_frame_,
										&got_frame, &packet_);
		packet_.size -= len;
		packet_.data += len;
	}
}

cv::Mat DecoderSink::convert_to_cvmat()
{
	cv::Mat m = cv::Mat::zeros(decoded_frame_->height, decoded_frame_->width,
								CV_8UC3);
	if (swsctx_ == nullptr
		|| decoded_frame_->width != width_
		|| decoded_frame_->height != height_) {

		if (swsctx_ != nullptr)
			sws_freeContext(swsctx_);

		width_ = decoded_frame_->width;
		height_ = decoded_frame_->height;
		swsctx_ = sws_getCachedContext(nullptr,
				width_, height_, (AVPixelFormat)decoded_frame_->format,
				width_, height_, PIX_FMT_BGR24,
				SWS_BICUBIC, nullptr, nullptr, nullptr);
	}

	avpicture_fill((AVPicture*)rgb_frame_, m.data, PIX_FMT_BGR24, width_, height_);

	sws_scale(swsctx_, decoded_frame_->data, decoded_frame_->linesize,
				0, height_, rgb_frame_->data, rgb_frame_->linesize);
	return m;
}

void DecoderSink::afterGettingFrame(void*		client_data,
										unsigned	framesz,
										unsigned	truncsz,
										timeval		presentation_time,
										unsigned	duration_in_microsecs)
{
	DecoderSink* sink = static_cast<DecoderSink*>(client_data);
	sink->afterGettingFrame(framesz, truncsz,
			presentation_time, duration_in_microsecs);
}

void DecoderSink::afterGettingFrame(unsigned	framesz,
										unsigned	truncsz,
										timeval		presentation_time,
										unsigned	duration_in_microsecs)
{
	unsigned char const start_code[] = {0x0, 0x0, 0x0, 0x1};

	if (strcmp(subsession_.codecName(), "H264") == 0)
		packet_.size = framesz + 4;
	else
		packet_.size = framesz;
	packet_.data = recvbuf_;

	while (packet_.size > 0) {
		int got_frame;
		int len = avcodec_decode_video2(decoderctx_, decoded_frame_,
										&got_frame, &packet_);
		if (got_frame) {
			cv::Mat m = convert_to_cvmat();
			double ts = presentation_time.tv_sec +
						presentation_time.tv_usec / 1000000.0;
			QMetaObject::invokeMethod((QObject*)consumer_, "frame",
					Qt::QueuedConnection,
					Q_ARG(cv::Mat, m),
					Q_ARG(double, ts));
		}
		packet_.size -= len;
		packet_.data += len;
	}

	continuePlaying();
}

Boolean DecoderSink::continuePlaying()
{
	if (fSource == 0) return False;

	if (strcmp(subsession_.codecName(), "H264") == 0)
		fSource->getNextFrame(recvbuf_ + 4, RECV_BUFSZ - 4, afterGettingFrame, this,
				onSourceClosure, this);
	else
		fSource->getNextFrame(recvbuf_, RECV_BUFSZ - 4, afterGettingFrame, this,
				onSourceClosure, this);

	return True;
}
