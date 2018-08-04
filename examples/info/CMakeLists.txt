add_executable(info "info.c")
target_link_libraries (info vcap ${V4L2_LIBRARY})
