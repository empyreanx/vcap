add_executable(info "info.c")
target_include_directories(info PUBLIC ${PROJECT_SOURCE_DIR} ${V4L2_INCLUDE_DIRS})
target_link_libraries (info vcap ${V4L2_LIBRARIES})
