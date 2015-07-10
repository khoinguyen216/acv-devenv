set(MOD_NAME acv)

include_directories(${Boost_INCLUDE_DIRS} ${Lua_INCLUDE_DIRS})

link_directories(
	${FFmpeg_LIBDIR}
	${LiveMedia_LIBDIR}
	${Lua_LIBDIR}
	${OpenCV_LIBDIR})

set(HEADER_FILES
	app.h cable_info.h event.h module.h module_factory.h
	module_graph.h script_cmd.h script_if.h)
set(SOURCE_FILES main.cpp app.cpp cable_info.cpp event.cpp
	module.cpp module_factory.cpp
	module_graph.cpp script_cmd.cpp script_if.cpp)

add_executable(${MOD_NAME} ${HEADER_FILES} ${SOURCE_FILES})
qt5_use_modules(${MOD_NAME} Core Network)
if(WITH_GUI)
	qt5_use_modules(${MOD_NAME} Widgets)
endif()

target_link_libraries(${MOD_NAME} peoplecount
	video_in video_out pc_logic
	${Boost_LIBRARIES} ${FFmpeg_LIBRARIES} ${LiveMedia_LIBRARIES}
	${Lua_LIBRARIES}
	${OpenCV_LIBRARIES})