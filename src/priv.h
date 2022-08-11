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

void vcap_ustrcpy(uint8_t* dst, const uint8_t* src, size_t size);
void vcap_strcpy(char* dst, const char* src, size_t size);

void vcap_set_error(vcap_dev* vd, const char* fmt, ...);
void vcap_set_error_errno(vcap_dev* vd, const char* fmt, ...);
const char* vcap_get_error(vcap_dev* vd);

void vcap_set_global_error(const char* fmt, ...);
const char* vcap_get_global_error();

// Clear data structure
#define VCAP_CLEAR(arg) memset(&(arg), 0, sizeof(arg))

// FOURCC character code to string
void vcap_fourcc_string(uint32_t code, uint8_t* str);

// Extended ioctl function
int vcap_ioctl(int fd, int request, void *arg);

// Map V4L2 pixel format to Vcap format ID
vcap_ctrl_id vcap_convert_fmt_id(uint32_t id);

#endif // VCAP_PRIV_H
