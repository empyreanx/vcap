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

#ifndef VCAP_PRIV_H
#define VCAP_PRIV_H

#include <vcap/vcap.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>

// Private macros
#define VCAP_ERROR(FMT, ...) vcap_set_error("%s: "FMT, __func__, ##__VA_ARGS__)
#define VCAP_ERROR_ERRNO(FMT, ...) vcap_set_error("%s: "FMT" (%s)", __func__, strerror(errno), ##__VA_ARGS__)
#define VCAP_CLEAR(ptr) memset(&(ptr), 0, sizeof(ptr))

// Private global variables
extern char vcap_error_msg[1024];
extern vcap_malloc_func vcap_malloc;
extern vcap_free_func vcap_free;

// Buffer holder for memory mapped buffers
typedef struct {
    size_t size;
    void* data;
} vcap_buffer;

// Frame grabber
struct vcap_fg {
    int fd;
    int buffer_count;
    vcap_buffer* buffers;
    vcap_device device;
};

// Format iterator
struct vcap_fmt_itr {
    vcap_fg* fg;
    uint32_t index;
    int result;
    vcap_fmt_desc desc;
};

// Size iterator
struct vcap_size_itr {
    vcap_fg* fg;
    vcap_fmt_id fid;
    uint32_t index;
    int result;
    vcap_size size;
};

// Frame rate iterator
struct vcap_rate_itr {
    vcap_fg* fg;
    vcap_fmt_id fid;
    vcap_size size;
    uint32_t index;
    int result;
    vcap_rate rate;
};

// Control iterator
struct vcap_ctrl_itr {
    vcap_fg* fg;
    uint32_t index;
    int result;
    vcap_ctrl_desc desc;
};

// Menu iterator
struct vcap_menu_itr {
    vcap_fg* fg;
    vcap_ctrl_id cid;
    uint32_t index;
    int result;
    vcap_menu_item item;
};

// Sets the error message
void vcap_set_error(const char* fmt, ...);

// Tries to open a device. If successful it retrieves the device info.
int vcap_try_get_device(const char* path, vcap_device* device);

// FOURCC character code to string
void vcap_fourcc_string(uint32_t code, uint8_t* str);

// Extended ioctl function
int vcap_ioctl(int fd, int request, void *arg);

// Map V4L2 pixel format to Vcap format ID
vcap_ctrl_id vcap_convert_fmt_id(uint32_t id);

#endif // VCAP_PRIV_H
