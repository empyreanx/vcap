if(NOT PNG_FOUND)
    message(FATAL_ERROR "You need libpng installed in order to build examples/png.c")
endif()

add_executable(png "png.c")
target_link_libraries(png vcap ${LIBRARIES})
