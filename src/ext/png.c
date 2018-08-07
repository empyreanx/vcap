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
#include "../priv.h"

#include <png.h>
#include <stdio.h>

int vcap_save_png(vcap_frame* frame, const char* path) {
    if (!frame) {
        VCAP_ERROR("Parameter 'frame' cannot be null");
        return -1;
    }

    if (!path) {
        VCAP_ERROR("Parameter 'path' cannot be null");
        return -1;
    }

    if (frame->fid != VCAP_FMT_RGB24) {
        VCAP_ERROR("Frame must contain RGB24 data");
        return -1;
    }

    FILE* file = fopen(path, "wb");

    if (!file) {
        VCAP_ERROR("Unable to open file for writing");
        return -1;
    }

    int code = 0;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytep* rows = NULL;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr) {
        VCAP_ERROR("Could not allocate PNG write struct\n");
        code = -1;
        goto finally;
    }

    info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr) {
        VCAP_ERROR("Could not allocate info struct\n");
        code = -1;
        goto finally;
    }

    rows = (png_bytep*)png_malloc(png_ptr, frame->size.height * sizeof(png_bytep));

    if (!rows) {
        VCAP_ERROR("Out of memory while allocating PNG rows");
        code = -1;
        goto finally;
    }

    for (int i = 0; i < frame->size.height; i++)
        rows[i] = (png_bytep)&frame->data[i * frame->stride];

    if (setjmp(png_jmpbuf(png_ptr))) {
        VCAP_ERROR("Unable to initialize I/O");
        code = -1;
        goto finally;
    }

    png_init_io(png_ptr, file);

    if (setjmp(png_jmpbuf(png_ptr))) {
        VCAP_ERROR("Writing header failed");
        code = -1;
        goto finally;
    }

    png_set_IHDR(png_ptr, info_ptr, frame->size.width, frame->size.height, 8,
                 PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    if (setjmp(png_jmpbuf(png_ptr))) {
        VCAP_ERROR("Error writing bytes");
        code = -1;
        goto finally;
    }

    png_write_image(png_ptr, rows);

    if (setjmp(png_jmpbuf(png_ptr))) {
        VCAP_ERROR("Writing end of file failed");
        code = -1;
        goto finally;
    }

    png_write_end(png_ptr, NULL);

finally:

    if (rows)
        png_free(png_ptr, rows);

    if (info_ptr)
        png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);

    if (png_ptr)
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

    if (file)
        fclose(file);

    return code;
}
