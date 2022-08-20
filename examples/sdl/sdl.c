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

#include <vcap.h>

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int width;
    int height;
    SDL_Window  *window;
    SDL_Surface *screen;
    SDL_Surface *image;
} sdl_context_t;

sdl_context_t* sdl_init(int width, int height);
int sdl_display(sdl_context_t* ctx, uint8_t* image);
void sdl_cleanup(sdl_context_t* ctx);

int main(int argc, char** argv)
{
    int index = 0;

    if (argc == 2)
        index = atoi(argv[1]);

    vcap_dev_info dev_info;

    // Find first video capture device
    int result = vcap_enum_devices(index, &dev_info);

    if (result == VCAP_ENUM_ERROR)
    {
        printf("Error while enumerating devices\n");
        return -1;
    }

    if (result == VCAP_ENUM_INVALID)
    {
        printf("Unable to find specified capture device\n");
        return -1;
    }

    // Create device
    vcap_dev* vd = vcap_create_device(dev_info.path, true, 3);

    // Open device
    result = vcap_open(vd);

    if (result == -1)
    {
        printf("%s\n", vcap_get_error(vd));
        vcap_destroy_device(vd);
        return -1;
    }

    vcap_size size = { 640, 480 };
    sdl_context_t* sdl_ctx = NULL;

    if (vcap_set_fmt(vd, V4L2_PIX_FMT_RGB24, size) == -1)
    {
        printf("%s\n", vcap_get_error(vd));
        vcap_destroy_device(vd);
        return -1;
    }

    size_t image_size = vcap_get_image_size(vd);
    uint8_t image_data[image_size];

    sdl_ctx = sdl_init(size.width, size.height);

    if (!sdl_ctx)
    {
        vcap_destroy_device(vd);
        return -1;
    }

    bool done = false;

    vcap_start_stream(vd);

    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    done = true;
                    break;

                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                            done = true;
                            break;
                    }
                    break;
            }
        }

        if (vcap_grab(vd, image_size, image_data) == -1)
        {
            printf("%s\n", vcap_get_error(vd));
            break;
        }

        if (sdl_display(sdl_ctx, image_data) == -1)
        {
            printf("Error displaying frame\n");
            break;
        }
    }

    if (sdl_ctx)
        sdl_cleanup(sdl_ctx);

    vcap_destroy_device(vd);

    return 0;
}

/*
 * Initializes the display contexgt
 */
sdl_context_t* sdl_init(int width, int height)
{
    atexit(SDL_Quit);

    sdl_context_t* ctx = malloc(sizeof(sdl_context_t));

    ctx->width = width;
    ctx->height = height;

    ctx->window = SDL_CreateWindow("Vcap Example",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   width, height, 0);

    if (ctx->window == NULL)
    {
        printf("Unable to set create window: %s\n", SDL_GetError());
        return NULL;
    }

    ctx->screen = SDL_GetWindowSurface(ctx->window);

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

void sdl_cleanup(sdl_context_t* ctx)
{
    //SDL_FreeSurface(ctx->screen);
    SDL_DestroyWindow(ctx->window);
    SDL_FreeSurface(ctx->image);
    free(ctx);
}

/*
 * Displays an image using SDL
 */
int sdl_display(sdl_context_t* ctx, uint8_t* image)
{
    memcpy(ctx->image->pixels, image, 3 * ctx->width * ctx->height);

    /*
     * Apply the image to the display
     */
    if (SDL_BlitSurface(ctx->image, NULL, ctx->screen, NULL) != 0)
    {
        printf("SDL_BlitSurface() Failed: %s\n", SDL_GetError());
        return -1;
    }

    /*
     * Update the display
     */
    SDL_UpdateWindowSurface(ctx->window);

    return 0;
}
