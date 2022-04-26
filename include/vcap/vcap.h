/*! \file vcap.h
    \mainpage Vcap
    \version  0.2.0
    \author   James McLean <empyreanx@zoho.com>
    \date     August 4, 2018
    \brief    An open source Video4Linux2 (V4L2) capture library written in C.

    \section About
    Vcap aims to provide a concise API for working with cameras and other video
    capture devices that have drivers implementing the V4L2 spec. It is built
    on top of the libv4l userspace library (the only required dependency) which
    provides seamless decoding for a variety of formats.

    Vcap is built with performance in mind and thus eliminates the use of
    dynamic memory allocation during frame grabbing. Vcap provides simple,
    low-level access to device controls, enabling applications to make use of
    the full range of functionality provided by V4L2.
*/

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

#ifndef VCAP_H
#define VCAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ctrls.h"
#include "fmts.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct vcap_fmt_itr vcap_fmt_itr;       ///< Format iterator
typedef struct vcap_size_itr vcap_size_itr;     ///< Size iterator
typedef struct vcap_rate_itr vcap_rate_itr;     ///< Frame rate iterator
typedef struct vcap_ctrl_itr vcap_ctrl_itr;     ///< Control iterator
typedef struct vcap_menu_itr vcap_menu_itr;     ///< Menu iterator

typedef struct
{
    int fd;
    char path[512];
    struct v4l2_capability caps;
} vcap_fg;

///
/// \brief Video capture device infomation
///
typedef struct
{
    char path[512];             ///< Device path
    uint8_t driver[32];         ///< Device driver name
    uint8_t card[32];           ///< Video hardware info
    uint8_t bus_info[32];       ///< Bus info
    uint32_t version;           ///< Driver version
    uint8_t version_str[16];    ///< Driver version str
} vcap_device_info;

///
/// \brief Pixel format description
///
typedef struct
{
    vcap_fmt_id id;             ///< Format ID
    uint8_t name[32];           ///< Format name
    uint8_t fourcc[5];          ///< FourCC string
} vcap_fmt_desc;

///
/// \brief Frame dimensions
///
typedef struct
{
    uint32_t width;             ///< Frame width
    uint32_t height;            ///< Frame height
} vcap_size;

///
/// \brief Frame rate.
///
typedef struct
{
    uint32_t numerator;         ///< Interval numerator
    uint32_t denominator;       ///< Interval denominator
} vcap_rate;

///
/// \brief Control descriptor
///
typedef struct
{
    vcap_ctrl_id id;            ///< Control ID
    uint8_t name[32];           ///< Control name
    vcap_ctrl_type type;        ///< Control type
    uint8_t type_name[16];      ///< Control type name
    int32_t min;                ///< The minimum value of the control
    int32_t max;                ///< The maximum value of the control
    int32_t step;               ///< The spacing between consecutive values
    int32_t default_value;      ///< The default value of the control (set when the driver is first loaded)
    bool read_only;             ///< true if the control is permanently read-only and false if read/write
} vcap_ctrl_desc;

///
/// \brief Control menu item
///
typedef struct
{
    uint32_t index;             ///< Menu item index (value used to set the control)

    union
    {
        uint8_t name[32];       ///< Menu item name (used if control type is VCAP_CTRL_TYPE_MENU)
        int64_t value;          ///< Menu item value (used if control type is VCAP_CTRL_TYPE_INTEGER_MENU)
    };

} vcap_menu_item;

///
/// \brief Defines a rectangle
///
typedef struct
{
    int32_t top;                ///< Top edge of rectangle
    int32_t left;               ///< Left edge of rectangle
    int32_t width;              ///< Width of rectangle
    int32_t height;             ///< Height of rectangle
} vcap_rect;

///
/// \brief Defines a video frame
///
typedef struct
{
    vcap_fmt_id fmt;            ///< Frame format
    vcap_size size;             ///< Size of frame
    size_t stride;              ///< Length of row in bytes
    size_t length;              ///< Length of data in bytes
    uint8_t* data;              ///< Frame data
} vcap_frame;

#define VCAP_FMT_OK          0  ///< Format is supported
#define VCAP_FMT_INVALID    -1  ///< Format is not supported
#define VCAP_FMT_ERROR      -2  ///< Error reading format descriptor

#define VCAP_CTRL_OK         0  ///< Control is supported
#define VCAP_CTRL_INACTIVE  -1  ///< Control is supported, but inactive
#define VCAP_CTRL_READ_ONLY -2  ///< Control is presently read-only
#define VCAP_CTRL_INVALID   -2  ///< Control is not supported
#define VCAP_CTRL_ERROR     -3  ///< Error reading control descriptor

#define VCAP_ENUM_OK         0  ///< Successfully enumerated item (valid index)
#define VCAP_ENUM_INVALID   -1  ///< Invalid index
#define VCAP_ENUM_DISABLED  -2  ///< Valid index, but item is disabled
#define VCAP_ENUM_ERROR     -3  ///< Error enumerating item

typedef void* (*vcap_malloc_func)(size_t size); ///< Custom malloc function type
typedef void (*vcap_free_func)(void* ptr);      ///< Custom free function type

//------------------------------------------------------------------------------
///
/// \brief  Set a custom 'malloc' and 'free' function
///
/// This is not strictly necessary since vcap does very little dynamic memory
/// allocation. However, if you are already using a custom allocator you might
/// as well use this function. By default Vcap uses the standard malloc and free.
///
void vcap_set_alloc(vcap_malloc_func malloc_func, vcap_free_func free_func);

//------------------------------------------------------------------------------
///
/// \brief  Deallocates memory
///
/// \param  ptr  Pointer to the memory to be freed
///
void vcap_free(void* ptr);

//------------------------------------------------------------------------------
///
/// \brief  Returns a string containing the last error message
///
const char* vcap_get_error();

//------------------------------------------------------------------------------
///
/// \brief  Prints device information to a file descriptor
///
/// This function prints (to a file descriptor) device information (path, driver,
/// bus info etc...), and enumerates and prints information about all formats
/// and controls. For example, to display device information to standard output,
/// call 'vcap_dump_info(fg, stdout)'.
///
/// \param  fg    Pointer to the frame grabber
/// \param  file  The file descriptor
///
/// \returns -1 on error and 0 otherwise
///
int vcap_dump_info(vcap_fg* fg, FILE* file);

//------------------------------------------------------------------------------
///
/// \brief  Enumerates video capture devices
///
/// Retrieves the video capture device at the specified 'index' and stores
/// the corresponding device information in the 'vcap_device' struct pointed to
/// by the 'device' parameter.
///
/// \param  device  Pointer to the device info struct
/// \param  index   The index of the device to query
///
/// \returns VCAP_ENUM_OK      if the device information was retrieved successfully,
///          VCAP_ENUM_INVALID if the index is invalid, and
///          VCAP_ENUM_ERROR   if querying the device failed.
///
int vcap_enum_devices(unsigned index, vcap_device_info* info);

//------------------------------------------------------------------------------
///
/// \brief  Opens a video capture device
///
/// Attempts to open a video capture device using the information stored in the
/// 'vcap_device' struct pointed to by the 'device' parameter. The function
/// returns a pointer to a frame grabber.
///
/// \param  device  Pointer to the device info
///
/// \returns NULL on error and a pointer to a frame grabber otherwise
///
int vcap_open(const char* path, vcap_fg* fg);

//------------------------------------------------------------------------------
///
/// \brief  Closes a video capture device
///
/// Closes the device and deallocates the frame grabber.
///
/// \param  fg  Pointer to the frame grabber
///
void vcap_close(const vcap_fg* fg);

int vcap_start_stream(const vcap_fg* fg);
int vcap_stop_stream(const vcap_fg* fg);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves video capture device information
///
/// This function retrieves video capture device information for the device at
/// the specified path.
///
/// \param  path    The path of the device handle
/// \param  device  Pointer to the struct in which to store the device info
///
/// \returns -1 on error and 0 otherwise
///
void vcap_get_device_info(const vcap_fg* fg, vcap_device_info* info);

//------------------------------------------------------------------------------
///
/// \brief  Allocates a video frame
///
/// Allocates a video frame used for capturing. The frame is used (and reused)
/// when reading data from the video capture device. If the format or frame size
/// is altered the frame should be deallocated and a new one allocated since the
/// size of the buffer will have changed.
///
/// \param  fg  Pointer to the frame grabber
///
/// \returns NULL on error and a pointer to the video frame otherwise
///
vcap_frame* vcap_alloc_frame(vcap_fg* fg);

//------------------------------------------------------------------------------
///
/// \brief  Deallocates all resources held by 'frame'
///
/// Deallocates the frame data and the frame.
///
/// \param  frame  Pointer to the frame struct
///
void vcap_free_frame(vcap_frame* frame);

//------------------------------------------------------------------------------
///
/// \brief  Grabs a grabs a frame
///
/// Grabs a frame from a video capture device using the specified frame grabber.
///
/// \param  fg     Pointer to the frame grabber
/// \param  frame  Pointer to the frame
///
/// \returns -1 on error and 0 otherwise
///
int vcap_grab(vcap_fg* fg, vcap_frame* frame);

//------------------------------------------------------------------------------
///
/// \brief  Get cropping bounds
///
/// Retrieves a rectangle that determines the valid cropping area for the
/// specified frame grabber.
///
/// \param  fg     Pointer to the frame grabber
/// \param  rect   Pointer to the rectangle
///
/// \returns -1 on error and 0 otherwise
///
int vcap_get_crop_bounds(vcap_fg* fg, vcap_rect* rect);

//------------------------------------------------------------------------------
///
/// \brief  Resets cropping rectangle
///
/// Resets the cropping rectange to its default dimensions.
///
/// \param  fg  Pointer to the frame grabber
///
/// \returns -1 on error and 0 otherwise
///
int vcap_reset_crop(vcap_fg* fg);

//------------------------------------------------------------------------------
///
/// \brief  Get the current cropping rectangle
///
/// Retrieves the current cropping rectangle for the specified frame grabber
/// and stores it in 'rect'.
///
/// \param  fg    Pointer to the frame grabber
/// \param  rect  Pointer to rectangle
///
/// \returns -1 on error and 0 otherwise
///
int vcap_get_crop(vcap_fg* fg, vcap_rect* rect);

//------------------------------------------------------------------------------
///
/// \brief  Sets the cropping rectangle
///
/// Sets the cropping rectangle for the specified frame grabber.
///
/// \param  fg    Pointer to the frame grabber
/// \param  rect  Pointer to rectangle
///
/// \returns -1 on error and 0 otherwise
///
int vcap_set_crop(vcap_fg* fg, vcap_rect rect);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves a format descriptor
///
/// Retrieves a format descriptor for the specified format ID.
///
/// \param  fg    Pointer to the frame grabber
/// \param  fmt   The format ID
/// \param  desc  Pointer to the format descriptor
///
/// \returns VCAP_FMT_OK      if the format descriptor was retrieved successfully,
///          VCAP_FMT_INVALID if the format ID is invalid
///          VCAP_FMT_ERROR   if getting the format descriptor failed
///
int vcap_get_fmt_desc(vcap_fg* fg, vcap_fmt_id fmt, vcap_fmt_desc* desc);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new format iterator
///
/// Creates and initializes new format iterator for the specified frame grabber.
/// The iterator should be deallocated using 'vcap_free'.
/// \param  fg  Pointer to the frame grabber
///
/// \returns An initialized 'vcap_fmt_itr' struct
///
vcap_fmt_itr* vcap_new_fmt_itr(vcap_fg* fg);

//------------------------------------------------------------------------------
///
/// \brief  Advances the specified format iterator
///
/// Copies the current format descriptor into 'desc' and advances the iterator.
///
/// \param  itr   Pointer to iterator
/// \param  desc  Pointer to the format descriptor
///
/// \returns 0 if there was an error or there are no more formats, and 1 otherwise
///
bool vcap_fmt_itr_next(vcap_fmt_itr* itr, vcap_fmt_desc* desc);

//------------------------------------------------------------------------------
///
/// \brief  Checks if there was an error while creating or advancing the iterator
///
/// \param  itr  Pointer to iterator
///
/// \returns true if there was an error or false otherwise
///
bool vcap_fmt_itr_error(vcap_fmt_itr* itr);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new frame size iterator
///
/// Creates and initializes new frame size iterator for the specified frame
/// grabber and format ID. The iterator should be deallocated using 'vcap_free'.
///
/// \param  fg   Pointer to the frame grabber
/// \param  fmt  The format ID
///
/// \returns An initialized 'vcap_size_itr' struct
///
vcap_size_itr* vcap_new_size_itr(vcap_fg* fg, vcap_fmt_id fmt);

//------------------------------------------------------------------------------
///
/// \brief  Advances the specified frame size iterator
///
/// Copies the current frame size into 'size' and advances the iterator.
///
/// \param  itr   Pointer to iterator
/// \param  size  Pointer to the frame size
///
/// \returns 0 if there was an error or there are no more sizes, and 1 otherwise
///
bool vcap_size_itr_next(vcap_size_itr* itr, vcap_size* size);

//------------------------------------------------------------------------------
///
/// \brief  Checks if there was an error while creating or advancing the iterator
///
/// \param  itr  Pointer to iterator
///
/// \returns true if there was an error or false otherwise
///
bool vcap_size_itr_error(vcap_size_itr* itr);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new frame rate iterator
///
/// Creates and initializes new frame rate iterator for the specified frame
/// grabber, format ID, and frame size. The iterator should be deallocated
/// using 'vcap_free'.
///
/// \param  fg    Pointer to the frame grabber
/// \param  fmt   The format ID
/// \param  size  The frame size
///
/// \returns An initialized 'vcap_rate_itr' struct
///
vcap_rate_itr* vcap_new_rate_itr(vcap_fg* fg, vcap_fmt_id fmt, vcap_size size);

//------------------------------------------------------------------------------
///
/// \brief  Advances the specified frame rate iterator
///
/// Copies the current frame rate into 'rate' and advances the iterator.
///
/// \param  itr   Pointer to iterator
/// \param  rate  Pointer to the frame rate
///
/// \returns 0 if there was an error or there are no more sizes, and 1 otherwise
///
bool vcap_rate_itr_next(vcap_rate_itr* itr, vcap_rate* rate);

//------------------------------------------------------------------------------
///
/// \brief  Checks if there was an error while creating or advancing the iterator
///
/// \param  itr  Pointer to iterator
///
/// \returns true if there was an error or false otherwise
///
bool vcap_rate_itr_error(vcap_rate_itr* itr);

//------------------------------------------------------------------------------
///
/// \brief  Gets the current format and frame size
///
/// Retrieves the current format ID and frame size simultaneously for the
/// specified frame grabber.
///
/// \param  fg    Pointer to the frame grabber
/// \param  fmt   Pointer to the format ID
/// \param  size  Pointer to the frame size
///
/// \returns -1 on error and 0 otherwise
///
int vcap_get_fmt(vcap_fg* fg, vcap_fmt_id* fmt, vcap_size* size);

//------------------------------------------------------------------------------
///
/// \brief  Sets the format and frame size
///
/// Sets the format ID and frame size simultaneously for the specified frame
/// grabber.
///
/// \param  fg    Pointer to the frame grabber
/// \param  fmt   The format ID
/// \param  size  The frame size
///
/// \returns -1 on error and 0 otherwise
///
int vcap_set_fmt(vcap_fg* fg, vcap_fmt_id fmt, vcap_size size);

//------------------------------------------------------------------------------
///
/// \brief  Get the current frame rate
///
/// Retrieves the current frame rate for the specified frame grabber.
///
/// \param  fg    Pointer to the frame grabber
/// \param  rate  Pointer to the frame rate
///
/// \returns -1 on error and 0 otherwise
///
int vcap_get_rate(vcap_fg* fg, vcap_rate* rate);

//------------------------------------------------------------------------------
///
/// \brief  Sets the frame rate
///
/// Sets the frame rate for the specified frame grabber.
///
/// \param  fg        Pointer to the frame grabber
/// \param  rate  The frame rate
///
/// \returns -1 on error and 0 otherwise
///
int vcap_set_rate(vcap_fg* fg, vcap_rate rate);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves a control descriptor
///
/// Retrieves the control descriptor for the specified control ID.
///
/// \param  fg    Pointer to the frame grabber
/// \param  ctrl  The control ID
/// \param  desc  Pointer to the control descriptor
///
/// \returns VCAP_CTRL_OK       if the control descriptor was retrieved successfully,
///          VCAP_CTRL_INACTIVE if the control ID is valid, but the control is inactive,
///          VCAP_CTRL_INVALID  if the control ID is invalid, and
///          VCAP_CTRL_ERROR    if getting the control descriptor failed
///
int vcap_get_ctrl_desc(vcap_fg* fg, vcap_ctrl_id ctrl, vcap_ctrl_desc* desc);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves a control descriptor
///
/// Retrieves the status of the control with the specified ID.
///
/// \param  fg    Pointer to the frame grabber
/// \param  ctrl  The control ID
///
/// \returns VCAP_CTRL_OK        if the control descriptor was retrieved successfully,
///          VCAP_CTRL_INACTIVE  if the control ID is valid, but the control is inactive,
///          VCAP_CTRL_READ_ONLY if the control is active but currently read-only,
///          VCAP_CTRL_INVALID   if the control ID is invalid, and
///          VCAP_CTRL_ERROR     if getting the control descriptor failed
///
int vcap_ctrl_status(vcap_fg* fg, vcap_ctrl_id ctrl);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new control iterator
///
/// Creates and initializes new control iterator for the specified frame
/// grabber. The iterator should be deallocated using 'vcap_free'.
///
/// \param  fg  Pointer to the frame grabber
///
/// \returns An initialized 'vcap_ctrl_itr' struct
///
vcap_ctrl_itr* vcap_new_ctrl_itr(vcap_fg* fg);

//------------------------------------------------------------------------------
///
/// \brief  Advances the specified control iterator
///
/// Copies the current control descriptor into 'desc' and advances the iterator.
///
/// \param  itr   Pointer to iterator
/// \param  desc  Pointer to the control descriptor
///
/// \returns 0 if there was an error or there are no more controls, and 1 otherwise
///
bool vcap_ctrl_itr_next(vcap_ctrl_itr* itr, vcap_ctrl_desc* desc);

//------------------------------------------------------------------------------
///
/// \brief  Checks if there was an error while creating or advancing the iterator
///
/// \param  itr  Pointer to iterator
///
/// \returns true if there was an error or false otherwise
///
bool vcap_ctrl_itr_error(vcap_ctrl_itr* itr);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new menu iterator
///
/// Creates and initializes a new menu iterator for the specified frame
/// grabber and control ID. The iterator should be deallocated using 'vcap_free'.
///
/// \param  fg   Pointer to the frame grabber
/// \param  ctrl The control ID
///
/// \returns An initialized 'vcap_menu_itr' struct
///
vcap_menu_itr* vcap_new_menu_itr(vcap_fg* fg, vcap_ctrl_id ctrl);

//------------------------------------------------------------------------------
///
/// \brief  Advances the specified menu iterator
///
/// Copies the current menu item into 'item' and advances the iterator.
///
/// \param  itr   Pointer to iterator
/// \param  item  Pointer to the menu item
///
/// \returns 0 if there was an error or there are no more controls, and 1 otherwise
///
bool vcap_menu_itr_next(vcap_menu_itr* itr, vcap_menu_item* item);

//------------------------------------------------------------------------------
///
/// \brief  Checks if there was an error while creating or advancing the iterator
///
/// \param  itr  Pointer to iterator
///
/// \returns true if there was an error or false otherwise
///
bool vcap_menu_itr_error(vcap_menu_itr* itr);

//------------------------------------------------------------------------------
///
/// \brief  Gets a control's value
///
/// Retrieves the current value of a control with the specified control ID.
///
/// \param  fg     Pointer to the frame grabber
/// \param  ctrl   The control ID
/// \param  value  The control value
///
/// \returns -1 on error and 0 otherwise
///
int vcap_get_ctrl(vcap_fg* fg, vcap_ctrl_id ctrl, int32_t* value);

//------------------------------------------------------------------------------
///
/// \brief  Sets a control's value
///
/// Sets the value of a control value with the specified control ID.
///
/// \param  fg     Pointer to the frame grabber
/// \param  ctrl   The control ID
/// \param  value  The control value
///
/// \returns -1 on error and 0 otherwise
///
int vcap_set_ctrl(vcap_fg* fg, vcap_ctrl_id ctrl, int32_t value);

//------------------------------------------------------------------------------
///
/// \brief  Resets a control's value
///
/// Resets the value of the specified control to its default value.
///
/// \param  fg   Pointer to the frame grabber
/// \param  ctrl The control ID
///
/// \returns -1 on error and 0 otherwise
///
int vcap_reset_ctrl(vcap_fg* fg, vcap_ctrl_id ctrl);

//------------------------------------------------------------------------------
///
/// \brief  Resets a all controls to defaults
///
/// Resets all controls to their default values.
///
/// \param  fg  Pointer to the frame grabber
///
/// \returns -1 on error and 0 otherwise
///
int vcap_reset_all_ctrls(vcap_fg* fg);

#ifdef __cplusplus
}
#endif

#endif // VCAP_H
