add_executable(grab "grab.c")
target_link_libraries (grab vcap ${V4L2_LIBRARY})
