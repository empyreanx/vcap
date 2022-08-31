add_executable(capture "capture.c")
target_include_directories(capture PUBLIC ${PROJECT_SOURCE_DIR} ${V4L2_INCLUDE_DIRS})
target_link_libraries (capture vcap ${V4L2_LIBRARIES})
