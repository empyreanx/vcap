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

void vcap_ustrcpy(uint8_t* dst, const uint8_t* src, size_t size)
{
    snprintf((char*)dst, size, "%s", (char*)src);
}

void vcap_strcpy(char* dst, const char* src, size_t size)
{
    snprintf(dst, size, "%s", src);
}

const char* vcap_get_error_priv()
{
    return error_msg;
}

void vcap_set_alloc_priv(vcap_malloc_func malloc_func, vcap_free_func free_func)
{
    malloc_func_ptr = malloc_func;
    free_func_ptr = free_func;
}

void* vcap_malloc(size_t size)
{
    return malloc_func_ptr(size);
}

void vcap_free(void* ptr)
{
    free_func_ptr(ptr);
}

void vcap_set_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(error_tmp, sizeof(error_tmp), fmt, args);
    va_end(args);

    strncpy(error_msg, error_tmp, sizeof(error_msg));
}

void vcap_fourcc_string(uint32_t code, uint8_t* str)
{
    str[0] = (code >> 0) & 0xFF;
    str[1] = (code >> 8) & 0xFF;
    str[2] = (code >> 16) & 0xFF;
    str[3] = (code >> 24) & 0xFF;
    str[4] = '\0';
}

int vcap_ioctl(int fd, int request, void *arg)
{
    int result;

    do
    {
        result = v4l2_ioctl(fd, request, arg);
    }
    while (-1 == result && (EINTR == errno || EAGAIN == errno));

    return result;
}
