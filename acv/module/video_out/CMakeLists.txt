set(MOD_NAME video_out)

include_directories(${OpenCV_INCLUDE_DIRS})

set(HEADER_FILES video_out.h)
set(SOURCE_FILES video_out.cpp)

add_library(${MOD_NAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})
qt5_use_modules(${MOD_NAME} Core)