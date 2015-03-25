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
#include <vcap/vcap_decode.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <SDL.h>

typedef struct {
	int width;
	int height;
	SDL_Surface *screen;
	SDL_Surface *image;
} sdl_context_t;

int sdl_init(sdl_context_t* ctx, int width, int height);
int sdl_display(sdl_context_t* ctx, uint8_t* image);
void sdl_cleanup(sdl_context_t* ctx);

/*
 * Grabs frames from a camera and displays them in a window in real-time. 
 * You must have SDL version 1.2 install for this example to compile.
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
	
	//obtain the currently selected format
	uint32_t format_code, width, height;
	
	if (-1 == vcap_get_format(camera, &format_code, &width, &height)) {
		printf("Error: %s\n", vcap_error());
		return -1;
	}
	
	//allocated a buffer to store the decoded RGB image
	uint8_t* rgb_buffer = (uint8_t*)malloc(3 * width * height);
	
	//start capturing
	if (-1 == vcap_start_capture(camera)) {
		printf("Error: %s\n", vcap_error());
		return -1;
	}
	
	//some cameras require time to initialize
	sleep(3);
	
	sdl_context_t sdl_ctx;
	
	sdl_init(&sdl_ctx, width, height);
	
	SDL_Event event;
	
	while (SDL_PollEvent(&event) >= 0) {
		if (event.type == SDL_QUIT) {
			break;
		}
		
		uint8_t* raw_buffer;
		
		//grab and decode frame
		if (-1 == vcap_grab_frame(camera, &raw_buffer)) {
			printf("Error: %s\n", vcap_error());
			return -1;
		}
		
		if (-1 == vcap_decode(raw_buffer, rgb_buffer, format_code, width, height, 0)) {
			printf("Error: Unable to decode frame; format not supported\n");
			return -1;
		}
		
		//free raw buffer to prevent a memory leak
		free(raw_buffer);
		
		sdl_display(&sdl_ctx, rgb_buffer);
	}
	
	//clean up
	if (-1 == vcap_destroy_cameras(cameras, num_cameras)) {
		printf("Error: %s\n", vcap_error());
		return -1;
	}
	
	sdl_cleanup(&sdl_ctx);
	
	free(rgb_buffer);

	return 0;
}

/*
 * Initializes the display contexgt
 */
int sdl_init(sdl_context_t* ctx, int width, int height) {
	atexit(SDL_Quit);
	
	ctx->width = width;
	ctx->height = height;
	
	ctx->screen = SDL_SetVideoMode(width, height, 24, SDL_DOUBLEBUF);
	
	if (ctx->screen == NULL) {
		printf("Unable to set video mode: %s\n", SDL_GetError());
		return -1;
	}

    uint32_t rmask, gmask, bmask;

	#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
	#else
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
	#endif

	ctx->image = SDL_CreateRGBSurface(0, width, height, 24, rmask, gmask, bmask, 0);
	
	return 0;
}

void sdl_cleanup(sdl_context_t* ctx) {
	SDL_FreeSurface(ctx->screen);
	SDL_FreeSurface(ctx->image);
}

/*
 * Displays an image using SDL
 */
int sdl_display(sdl_context_t* ctx, uint8_t* image) {
	memcpy(ctx->image->pixels, image, 3 * ctx->width * ctx->height);	
	
	/*
	 * Apply the image to the display
	 */
	if (SDL_BlitSurface(ctx->image, NULL, ctx->screen, NULL) != 0) {
		printf("SDL_BlitSurface() Failed: %s\n", SDL_GetError());
		return -1;
	}
		
	/*
	 * Update the display
	 */
	SDL_Flip(ctx->screen);
	
	return 0;
}
