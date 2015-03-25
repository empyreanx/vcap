/*
 * Vcap - a Video4Linux2 capture library
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

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	vcap_camera_t* cameras;
	
	int num_cameras = vcap_cameras(&cameras);
	
	if (num_cameras <= 0) {
		printf("No cameras found!\n");
		return -1;
	}
	
	printf("------------------------------------------------------\n");
	
	/*
	 * Iterate through cameras.
	 */
	for (int i = 0; i < num_cameras; i++) {
		vcap_camera_t *camera = &cameras[i];
			
		printf("Device: %s\n", camera->device);
		printf("Driver: %s\n", camera->driver);
		printf("Info: %s\n", camera->info);
		
		if (-1 == vcap_open_camera(camera)) {
			printf("Error: %s\n", vcap_error());
			return -1;
		}
		
		vcap_format_t* formats;
		
		int num_formats = vcap_get_formats(camera, &formats);
		
		if (-1 == num_formats) {
			printf("Error: %s\n", vcap_error());
			return -1;
		}
		
		/*
		 * Iterate through formats.
		 */
		for (int j = 0; j < num_formats; j++) {
			printf("%s (%s)\n", formats[j].code_str, formats[j].desc);
						
			/*
			 * Iterate through frame sizes.
			 */
			for (int k = 0; k < formats[j].num_sizes; k++) {
				printf("\t%dx%d", formats[j].sizes[k].width, formats[j].sizes[k].height);
				
				uint16_t* frame_rates;
				
				int num_frame_rates = vcap_get_frame_rates(camera, formats[j].code, formats[j].sizes[k].width, formats[j].sizes[k].height, &frame_rates);
				
				if (-1 == num_frame_rates) {
					printf("Error: %s\n", vcap_error());
					return -1;
				}
				
				if (num_frame_rates > 0) {
					printf(" (FPS: ");
					
					/*
					 * Iterate through frame rates.
					 */
					for (int l = 0; l < num_frame_rates; l++) {
						printf("%d", frame_rates[l]);
					
						if (l < num_frame_rates - 1)
							printf(", ");
					}
					
					printf(")");
				}
				
				printf("\n");
			}
		}
		
		vcap_destroy_formats(formats, num_formats);
		
		vcap_control_t* controls;
		
		int num_controls = vcap_get_controls(camera, &controls);
		
		if (-1 == num_controls) {
			printf("Error: %s\n", vcap_error());
			return -1;
		}
		
		if (num_controls > 0) {
			printf("Controls:\n");
			
			/*
			 * Iterate through controls.
			 */
			for (int j = 0; j < num_controls; j++) {
				printf("%s (min: %d, max: %d, step: %d, default: %d)", controls[j].name, controls[j].min, controls[j].max, controls[j].step, controls[j].default_value);
				
				if (VCAP_CTRL_TYPE_MENU == controls[j].type) {
					if (controls[j].menu_length > 0) {
						printf(" (Menu: ");
						
						for (int k = 0; k < controls[j].menu_length; k++) {
							printf("%d:%s", controls[j].menu[k].value, controls[j].menu[k].name);
							
							if (k < controls[j].menu_length - 1)
								printf(", ");
								
						}
						
						printf(")");
					}
				}
				
				printf("\n");
			}
			
			vcap_destroy_controls(controls, num_controls);
		}
		
		if (-1 == vcap_close_camera(camera)) {
			printf("Error: %s\n", vcap_error());
			return -1;
		}
		
		printf("------------------------------------------------------\n");
	}
	
	if (-1 == vcap_destroy_cameras(cameras, num_cameras)) {
		printf("Error: %s\n", vcap_error());
		return -1;
	}
	
	return 0;
}
