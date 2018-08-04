//==============================================================================
// Vcap - A Video4Linux2 capture library
//
// Copyright (C) 2018 James McLean
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
//==============================================================================

#include <vcap/vcap.h>

#include <SDL.h>

#include <stdio.h>

typedef struct {
    int width;
    int height;
    SDL_Surface *screen;
    SDL_Surface *image;
} sdl_context_t;

sdl_context_t* sdl_init(int width, int height);
int sdl_display(sdl_context_t* ctx, uint8_t* image);
void sdl_cleanup(sdl_context_t* ctx);

int main(int argc, char** argv) {
    vcap_device device;

    // Find first video capture device
    int ret = vcap_enum_devices(&device, 0);

    if (ret == VCAP_ENUM_ERROR) {
        printf("%s\n", vcap_get_error());
        return -1;
    }

    if (ret == VCAP_ENUM_INVALID) {
        printf("Error: Unable to find a video capture device\n");
        return -1;
    }

    // Open device
    vcap_fg* fg = vcap_open(&device);

    if (!fg) {
        printf("%s\n", vcap_get_error());
        return -1;
    }

    FILE* file = NULL;
    vcap_frame* frame = NULL;
    vcap_size size = { 640, 480 };
    sdl_context_t* sdl_ctx = NULL;

    if (vcap_set_fmt(fg, VCAP_FMT_RGB24, size) == -1) {
        printf("%s\n", vcap_get_error());
        goto error;
    }

    frame = vcap_alloc_frame(fg);

    if (!frame) {
        printf("%s\n", vcap_get_error());
        goto error;
    }

    sdl_ctx = sdl_init(frame->size.width, frame->size.height);

    if (!sdl_ctx)
        goto error;


    SDL_Event event;

    while (SDL_PollEvent(&event) >= 0) {
        if (event.type == SDL_QUIT)
            break;

        if (vcap_grab(fg, frame) == -1) {
            printf("%s\n", vcap_get_error());
            goto error;
        }

        if (sdl_display(sdl_ctx, frame->data) == -1)
            goto error;
    }

    sdl_cleanup(sdl_ctx);
    vcap_free_frame(frame);
    vcap_close(fg);

    return 0;

error:

    if (sdl_ctx)
        sdl_cleanup(sdl_ctx);

    if (file)
        fclose(file);

    if (frame)
        vcap_free_frame(frame);

    vcap_close(fg);

    return -1;
}

/*
 * Initializes the display contexgt
 */
sdl_context_t* sdl_init(int width, int height) {
    atexit(SDL_Quit);

    sdl_context_t* ctx = malloc(sizeof(sdl_context_t));

    ctx->width = width;
    ctx->height = height;

    ctx->screen = SDL_SetVideoMode(width, height, 24, SDL_DOUBLEBUF);

    if (ctx->screen == NULL) {
        printf("Unable to set video mode: %s\n", SDL_GetError());
        return NULL;
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

    return ctx;
}

void sdl_cleanup(sdl_context_t* ctx) {
    SDL_FreeSurface(ctx->screen);
    SDL_FreeSurface(ctx->image);
    free(ctx);
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
