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

// Error macros
#define VCAP_ERROR(fmt, ...) vcap_set_error("[%s:%d] "fmt, __func__, __LINE__, ##__VA_ARGS__)
#define VCAP_ERROR_ERRNO(fmt, ...) vcap_set_error("[%s:%d] "fmt" (%s)", __func__, __LINE__, strerror(errno), ##__VA_ARGS__)

void vcap_ustrcpy(uint8_t* dst, const uint8_t* src, size_t size);
void vcap_strcpy(char* dst, const char* src, size_t size);

/*
// Format iterator
struct vcap_fmt_itr
{
    vcap_dev* vd;
    uint32_t index;
    int result;
    vcap_fmt_info info;
};

// Size iterator
struct vcap_size_itr
{
    vcap_dev* vd;
    vcap_fmt_id fmt;
    uint32_t index;
    int result;
    vcap_size size;
};

// Frame rate iterator
struct vcap_rate_itr
{
    vcap_dev* vd;
    vcap_fmt_id fmt;
    vcap_size size;
    uint32_t index;
    int result;
    vcap_rate rate;
};
*/

/*
// Control iterator
struct vcap_ctrl_itr
{
    vcap_dev* vd;
    uint32_t index;
    int result;
    vcap_ctrl_info info;
};

// Menu iterator
struct vcap_menu_itr
{
    vcap_dev* vd;
    vcap_ctrl_id ctrl;
    uint32_t index;
    int result;
    vcap_menu_item item;
};*/

// Clear data structure
#define VCAP_CLEAR(arg) memset(&(arg), 0, sizeof(arg))

// Sets the error message
void vcap_set_error(const char* fmt, ...);

// Private error message getter (avoids global variable)
const char* vcap_get_error_priv();

// Private allocator setter (avoids global variable)
void vcap_set_alloc_priv(vcap_malloc_func malloc_func, vcap_free_func free_func);

// Declare malloc function
void* vcap_malloc(size_t size);

// FOURCC character code to string
void vcap_fourcc_string(uint32_t code, uint8_t* str);

// Extended ioctl function
int vcap_ioctl(int fd, int request, void *arg);

// Map V4L2 pixel format to Vcap format ID
vcap_ctrl_id vcap_convert_fmt_id(uint32_t id);

#endif // VCAP_PRIV_H
