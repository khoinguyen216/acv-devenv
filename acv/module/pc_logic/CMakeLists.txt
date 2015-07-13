set(MOD_NAME pc_logic)

include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

file(GLOB LBPDetector_SRCS LBPDetector/*.h LBPDetector/*.cpp)
file(GLOB ThreadManager_SRCS ThreadManager/*.h ThreadManager/*.cpp)

set(HEADER_FILES
	bgfg_vibe.hpp
	bwlabel.h
	pc_event.h
	pc_logic.h
	ssim.h
	target.h
	tracklet.h
	utilities.h
	CompressiveTracker.h
	DLBPUtilities.h
	ImageEnhancement.h
	MotionDetection.h
	TrackingManager.h)
set(SOURCE_FILES
	bgfg_vibe.cpp
	pc_event.cpp
	pc_logic.cpp
	ssim.cpp
	target.cpp
	tracklet.cpp
	utilities.cpp
	CompressiveTracker.cpp
	DLBPUtilities.cpp
	ImageEnhancement.cpp
	MotionDetection.cpp
	TrackingManager.cpp)

add_definitions(-DLBPDETECTOR_API_STATIC )

add_library(${MOD_NAME} STATIC ${HEADER_FILES} ${SOURCE_FILES}
			${LBPDetector_SRCS} ${ThreadManager_SRCS})
qt5_use_modules(${MOD_NAME} Core)