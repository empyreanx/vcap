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

#include "priv.h"

#include <linux/videodev2.h>
#include <libv4l2.h>

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char error_msg[1024];
static char error_tmp[1024];

static vcap_malloc_func malloc_func_ptr = malloc;
static vcap_free_func free_func_ptr = free;

const char* vcap_get_error_priv() {
    return error_msg;
}

void vcap_set_alloc_priv(vcap_malloc_func malloc_func, vcap_free_func free_func) {
    malloc_func_ptr = malloc_func;
    free_func_ptr = free_func;
}

void* vcap_malloc(size_t size) {
    return malloc_func_ptr(size);
}

void vcap_free(void* ptr) {
    free_func_ptr(ptr);
}

void vcap_set_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(error_tmp, sizeof(error_tmp), fmt, args);
    va_end(args);

    strncpy(error_msg, error_tmp, sizeof(error_msg));
}

int vcap_try_get_device(const char* path, vcap_device* device) {
    struct stat st;
    struct v4l2_capability caps;

    int fd = -1;

    // Device must exist
    if (stat(path, &st) == -1)
        goto error;

    // Device must be a character device
    if (!S_ISCHR(st.st_mode))
        goto error;

    // Open the video device
    fd = v4l2_open(path, O_RDONLY | O_NONBLOCK, 0);

    if (fd == -1)
        goto error;

    // Get the device capabilities
    if (vcap_ioctl(fd, VIDIOC_QUERYCAP, &caps) == -1)
        goto error;

    // Ensure video capture is supported
    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        goto error;

    // Copy device information
    strncpy((char*)device->path, (char*)path, sizeof(device->path));

    assert(sizeof(device->driver) == sizeof(caps.driver));
    memcpy(device->driver, caps.driver, sizeof(device->driver));

    assert(sizeof(device->card) == sizeof(caps.card));
    memcpy(device->card, caps.card, sizeof(device->card));

    assert(sizeof(device->bus_info) == sizeof(caps.bus_info));
    memcpy(device->bus_info, caps.bus_info, sizeof(device->bus_info));

    // Decode version
    snprintf((char*)device->version_str, sizeof(device->version_str), "%u.%u.%u",
        (caps.version >> 16) & 0xFF,
        (caps.version >> 8) & 0xFF,
        (caps.version & 0xFF));

    device->version = caps.version;

    // Close device
    v4l2_close(fd);

    return 0;

error:
    // If the device was opened, make sure it is closed.
    if (fd != -1)
        v4l2_close(fd);

    return -1;
}

void vcap_fourcc_string(uint32_t code, uint8_t* str) {
    str[0] = (code >> 0) & 0xFF;
    str[1] = (code >> 8) & 0xFF;
    str[2] = (code >> 16) & 0xFF;
    str[3] = (code >> 24) & 0xFF;
    str[4] = '\0';
}

int vcap_ioctl(int fd, int request, void *arg) {
    int result;

    do {
        result = v4l2_ioctl(fd, request, arg);
    } while (result == -1 && (errno == EINTR || errno == EAGAIN));

    return result;
}
