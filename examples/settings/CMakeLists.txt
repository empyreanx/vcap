find_package(Jansson)

if(JANSSON_FOUND)
	include_directories(${PROJECT_SOURCE_DIR} ${JANSSON_INCLUDE_DIR} ${V4L2_INCLUDE_DIRS})
	add_executable(settings "settings.c")
	target_link_libraries(settings vcap ${JANSSON_LIBRARY} ${V4L2_LIBRARIES})
else(JANSSON_FOUND)
    message(WARNING "You need Jansson installed in order to build examples/settings")
endif(JANSSON_FOUND)

