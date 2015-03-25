# Vcap
 
### Introduction

Vcap is a library written in C that aims to make using the Video4Linux2 API simple. It was designed to make the common 
case of capturing frames using memory-mapped device buffers quick and easy.

VCap was also written with computer vision applications in mind and thus allows setting camera controls that enhance or
interfere with computer vision and image processing techniques. VCap also includes a (growing) number of pixel format
decoding functions, enabling seamless support for many devices. 

In contrast with similar projects, VCap is licensed under the LGPL v2.1 which permits it's use in commercial products.

This project also has [C++ bindings](https://github.com/jrimclean/vcap-cpp).

### Building and 

You must have CMake and a C compiler installed. 

*$ cmake . && make*

To install:

*$ make install*

To generate documentation (if Doxygen is installed):

*$ make doc*
```cpp
#include <vcap/vcap.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Grabs raw data from a camera and saves it to the file 'image.raw'
 */
int main(int argc, char* argv[]) {
	vcap_camera_t* cameras;
	
	int num_cameras = vcap_cameras(&cameras);
	
	if (num_cameras <= 0) {
		printf("No cameras found!\n");
		return -1;
	}
	
	//open the first camera found
	vcap_camera_t* camera = &cameras[0];
	
	if (-1 == vcap_open_camera(camera)) {
		printf("Error: %s\n", vcap_error());
		return -1;
	}
	
	//start capturing
	if (-1 == vcap_start_capture(camera)) {
		printf("Error: %s\n", vcap_error());
		return -1;
	}
	
	//some cameras require time to initialize
	sleep(3);
	
	//grab frame
	uint8_t* raw_buffer;
	
	int buffer_size = vcap_grab_frame(camera, &raw_buffer);
	
	if (-1 == buffer_size) {
		printf("Error: %s\n", vcap_error());
		return -1;
	}	
	
	//write frame to file
	FILE* file = fopen("image.raw", "wb");
			
	if (fwrite(raw_buffer, buffer_size, 1, file) != 1)
		printf("Error writing to file!\n");
	else
		printf("Wrote output file 'image.raw' (%d bytes)\n", buffer_size);
			
	fclose(file);
	
	//clean up
	if (-1 == vcap_destroy_cameras(cameras, num_cameras)) {
		printf("Error: %s\n", vcap_error());
		return -1;
	}
	
	free(raw_buffer);
	
	return 0;
}
```

## License
Copyright (c) 2015 James McLean  
Licensed under LGPL v2.1.
