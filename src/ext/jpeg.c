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

#include "../priv.h"

#include <jpeglib.h>

#include <setjmp.h>
#include <stdio.h>

typedef struct {
    struct jpeg_error_mgr pub;
    jmp_buf jmp_buffer;
} error_mgr;

char last_error[JMSG_LENGTH_MAX];

void error_exit(j_common_ptr cinfo) {
    error_mgr* mgr = (error_mgr*)cinfo->err;

    // Create the message
    (*(cinfo->err->format_message))(cinfo, last_error);

    // Jump to the setjmp point
    longjmp(mgr->jmp_buffer, 1);
}

int vcap_save_jpeg(vcap_frame* frame, const char* path) {
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
    JSAMPROW row_ptr[1];
    struct jpeg_compress_struct cinfo;
    error_mgr jerr;

    if (setjmp(jerr.jmp_buffer)) {
        VCAP_ERROR("%s", last_error);
        VCAP_ERROR_GOTO(code, finally);
    }

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = error_exit;

    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, file);

    cinfo.image_width = frame->size.width;
    cinfo.image_height = frame->size.height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height) {
        row_ptr[0] = &frame->data[cinfo.next_scanline * frame->stride];
        jpeg_write_scanlines(&cinfo, row_ptr, 1);
    }

    jpeg_finish_compress(&cinfo);

finally:

    jpeg_destroy_compress(&cinfo);
    fclose(file);

    return code;
}
