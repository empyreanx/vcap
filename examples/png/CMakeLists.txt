find_package(PNG)

if(PNG_FOUND)
	include_directories(${PNG_INCLUDE_DIR})
	add_executable(png "png.c")
	target_link_libraries(png vcap ${PNG_LIBRARY} ${V4L2_LIBRARY})
else(PNG_FOUND)
	message(FATAL_ERROR "You need libpng installed in order to build examples/png.c")
endif(PNG_FOUND)
