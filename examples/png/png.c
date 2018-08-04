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

#include <png.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t *data;
    size_t size;
} png_buffer_t;

int rgb24_to_png(uint8_t *input, png_buffer_t *buffer, int width, int height);

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

    if (vcap_set_fmt(fg, VCAP_FMT_RGB24, size) == -1) {
        printf("%s\n", vcap_get_error());
        goto error;
    }

    frame = vcap_alloc_frame(fg);

    if (!frame) {
        printf("%s\n", vcap_get_error());
        goto error;
    }

    if (vcap_grab(fg, frame) == -1) {
        printf("%s\n", vcap_get_error());
        goto error;
    }

    //compress rgb24 to png
    png_buffer_t png_buffer;

    if (rgb24_to_png(frame->data, &png_buffer, frame->size.width, frame->size.height) == -1) {
        printf("Error converting data to PNG\n");
        goto error;
    }

    file = fopen("out.png", "wb");

    if (!file) {
        perror("Cannot open image");
        goto error;
    }

    fwrite(png_buffer.data, png_buffer.size, 1, file);

    if (ferror(file)) {
        perror("Unable to write to file");
        goto error;
    }

    fclose(file);
    vcap_free_frame(frame);
    vcap_close(fg);

    return 0;

error:

    if (file)
        fclose(file);

    if (frame)
        vcap_free_frame(frame);

    vcap_close(fg);

    return -1;
}

void png_write_to_buffer(png_structp png_ptr, png_bytep data, png_size_t length) {
    png_buffer_t* buffer = (png_buffer_t*)png_get_io_ptr(png_ptr);
    size_t new_size = length + buffer->size;

    if (buffer->data) {
        buffer->data = (uint8_t*)realloc(buffer->data, new_size);
    } else {
        buffer->data = (uint8_t*)malloc(new_size);
    }

    if (!buffer->data) {
        png_error(png_ptr, "Write error");
    }

    memcpy(buffer->data + buffer->size, data, length);
    buffer->size += length;
}

void png_dummy_flush(png_structp png_ptr) {
}

int rgb24_to_png(uint8_t *input, png_buffer_t *buffer, int width, int height) {
    /*
     * Initialize buffer info
     */
    buffer->size = 0;
    buffer->data = NULL;

    /*
     * More initalization
     */
    int code = 0;
    png_structp png_ptr;
    png_infop info_ptr;

    /*
     * Initialize write structure
     */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (png_ptr == NULL) {
        printf("Could not allocate write struct\n");
        return -1;
    }

    png_bytep* rows = (png_bytep*)png_malloc(png_ptr, height * sizeof(png_bytep));

    /*
     * Initialize info structure
     */
    info_ptr = png_create_info_struct(png_ptr);

    if (info_ptr == NULL) {
        printf("Could not allocate info struct\n");
        code = -1;
        goto finally;
    }

    /*
     * Setup exception handling
     */
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("Error during png creation\n");
        code = -1;
        goto finally;
    }

    /*
     * Write header (8 bit colour depth)
     */
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    for (int i = 0; i < height; i++) {
        rows[i] = (png_bytep)&input[3 * i * width];
    }

    png_set_rows(png_ptr, info_ptr, rows);

    png_set_write_fn(png_ptr, buffer, png_write_to_buffer, png_dummy_flush);

    png_write_info(png_ptr, info_ptr);

    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    finally: {
        if (info_ptr != NULL) {
            png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
        }

        if (png_ptr != NULL) {
            png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        }

        if (rows != NULL) {
            for (int i = 0; i < height; i++) {
                png_free(png_ptr, rows[i]);
            }

            png_free(png_ptr, rows);
        }
    }

    return code;
}
