//==============================================================================
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org/>
//==============================================================================

#include <vcap.h>

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>

// Consolidate SDL constructs in internal context
typedef struct
{
    int width;
    int height;
    SDL_Window  *window;
    SDL_Surface *screen;
    SDL_Surface *image;
} sdl_context_t;

// Opens window, get's window surface, allocates image
static sdl_context_t* sdl_create_context(int width, int height);

// Displays the image
static int sdl_display_image(sdl_context_t* ctx, uint8_t* image);

// Clean up, free context
static void sdl_destroy_context(sdl_context_t* ctx);

int main(int argc, char** argv)
{
    int index = 0;

    // First argument is device index
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

    if (vcap_set_fmt(vd, VCAP_FMT_RGB24, size) == -1)
    {
        printf("%s\n", vcap_get_error(vd));
        vcap_destroy_device(vd);
        return -1;
    }

    size_t image_size = vcap_get_image_size(vd);
    uint8_t image_data[image_size];

    sdl_context_t* sdl_ctx = sdl_create_context(size.width, size.height);

    if (!sdl_ctx)
    {
        printf("Unable to create internal SDL context\n");
        vcap_destroy_device(vd);
        return -1;
    }

    // Use streaming if the device supports it
    if (dev_info.stream)
        vcap_start_stream(vd);

    // Main loop
    bool done = false;

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

        // Grab an image from the device
        if (vcap_grab(vd, image_size, image_data) == -1)
        {
            printf("%s\n", vcap_get_error(vd));
            break;
        }

        // Display image
        if (sdl_display_image(sdl_ctx, image_data) == -1)
        {
            printf("Error displaying frame\n");
            break;
        }
    }

    // Clean up

    if (sdl_ctx)
        sdl_destroy_context(sdl_ctx);

    vcap_destroy_device(vd);

    return 0;
}

// Initializes the display context
static sdl_context_t* sdl_create_context(int width, int height)
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

// Releases all resources held by the SDL context
void sdl_destroy_context(sdl_context_t* ctx)
{
    SDL_DestroyWindow(ctx->window);
    SDL_FreeSurface(ctx->image);
    free(ctx);
}

// Displays an image using SDL
int sdl_display_image(sdl_context_t* ctx, uint8_t* image)
{
    memcpy(ctx->image->pixels, image, 3 * ctx->width * ctx->height);

    //Apply the image to the display
    if (SDL_BlitSurface(ctx->image, NULL, ctx->screen, NULL) != 0)
    {
        printf("SDL_BlitSurface() Failed: %s\n", SDL_GetError());
        return -1;
    }

    // Update the display
    SDL_UpdateWindowSurface(ctx->window);

    return 0;
}
