add_executable(grab "grab.c")
target_include_directories(grab PUBLIC ${PROJECT_SOURCE_DIR} ${V4L2_INCLUDE_DIRS})
target_link_libraries (grab vcap ${V4L2_LIBRARIES})
