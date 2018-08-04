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

#include <stdio.h>

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

    file = fopen("out.raw", "w");

    if (!file) {
        perror("Cannot open image");
        goto error;
    }

    fwrite(frame->data, frame->length, 1, file);

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
