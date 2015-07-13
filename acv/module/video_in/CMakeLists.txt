set(MOD_NAME video_in)

include_directories(
	${OpenCV_INCLUDE_DIRS}
	${LiveMedia_INCLUDE_DIRS}
	${FFmpeg_INCLUDE_DIRS})

set(HEADER_FILES
	video_in.h
	frame_source.h
	http_mjpg/http_mjpg_source.h
	rtsp/rtsp_source.h
	rtsp/sched_thread.h
	video/video_source.h)
set(SOURCE_FILES video_in.cpp
	http_mjpg/http_mjpg_source.cpp
	http_mjpg/MjpgCapture.cpp
	rtsp/DecoderSink.cpp
	rtsp/MPRTSPClient.cpp
	rtsp/rtsp_handlers.cpp
	rtsp/rtsp_source.cpp
	video/video_source.cpp)

add_library(${MOD_NAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})
qt5_use_modules(${MOD_NAME} Core)