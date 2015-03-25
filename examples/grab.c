/*
 * VCap: A Video4Linux2 Capture Library 
 * 
 * Copyright (C) 2014 James McLean
 * 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

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
