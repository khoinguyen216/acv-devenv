set(MOD_NAME peoplecount)

set(HEADER_FILES peoplecount.h)
set(SOURCE_FILES peoplecount.cpp)

add_library(${MOD_NAME} ${HEADER_FILES} ${SOURCE_FILES})
qt5_use_modules(${MOD_NAME} Core)
if(WITH_GUI)
	qt5_use_modules(${MOD_NAME} Widgets)
endif()
