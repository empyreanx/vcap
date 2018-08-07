if(NOT JPEG_FOUND)
    message(FATAL_ERROR "You need libjpeg installed in order to build examples/jpeg.c")
endif()

add_executable(jpeg "jpeg.c")
target_link_libraries(jpeg vcap ${LIBRARIES})
