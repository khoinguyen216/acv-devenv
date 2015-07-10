#ifndef H264DECODERSINK_H
#define H264DECODERSINK_H

#include <liveMedia.hh>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <opencv2/opencv.hpp>


class DecoderSink : public MediaSink {

	unsigned const RECV_BUFSZ = 1048576;

public:
	static DecoderSink* createNew(UsageEnvironment&		env,
									MediaSubsession&	subsession,
									void*				consumer,
									char const*			streamid = nullptr);
private:
	DecoderSink(UsageEnvironment&	env,
				MediaSubsession&	subsession,
				void*				consumer,
				char const*			streamid);
	virtual ~DecoderSink();

	void init_decoder();
	cv::Mat convert_to_cvmat();
	static void afterGettingFrame(	void*		client_data,
									unsigned	framesz,
									unsigned	truncsz,
									timeval		presentation_time,
									unsigned	duration_in_microsecs);
	void afterGettingFrame( unsigned	framesz,
							unsigned	truncsz,
							timeval		presentation_time,
							unsigned	duration_in_microsecs);
	virtual Boolean continuePlaying();

private:
	uint8_t*	recvbuf_;
	char*		streamid_;
	void*		consumer_;
	MediaSubsession&	subsession_;

	// FFmpeg
	AVCodec*		decoder_		= nullptr;
	AVCodecContext*	decoderctx_		= nullptr;
	AVPacket		packet_;
	AVFrame*		decoded_frame_	= nullptr;
	AVFrame*		rgb_frame_		= nullptr;
	SwsContext*		swsctx_			= nullptr;
	unsigned		width_			= 0;
	unsigned		height_			= 0;
};

#endif
