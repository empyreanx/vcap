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
// MIT License
//
// Copyright 2022 James McLean
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//==============================================================================

#ifndef VCAP_H
#define VCAP_H

#include <linux/videodev2.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

///
/// \brief Control ID type
///
typedef uint32_t vcap_ctrl_id;

///
/// \brief Control type ID
///
typedef uint8_t vcap_ctrl_type;

///
/// \brief Format ID type
///
typedef uint32_t vcap_fmt_id;

///
/// \brief Device handle
///
typedef struct vcap_dev vcap_dev;

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
    bool stream;                ///< True if device supports streaming, false otherwise
    bool read;                  ///< True if device supports direct read, false otherwise
} vcap_dev_info;

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
/// \brief Pixel format description
///
typedef struct
{
    vcap_fmt_id id;             ///< Format ID
    uint8_t name[32];           ///< Format name
    uint8_t fourcc[5];          ///< FourCC string
} vcap_fmt_info;

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
} vcap_ctrl_info;

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

// Control iterator
typedef struct
{
    vcap_dev* vd;
    uint32_t index;
    int result;
    vcap_ctrl_info info;
} vcap_ctrl_itr;

// Menu iterator
typedef struct
{
    vcap_dev* vd;
    vcap_ctrl_id ctrl;
    uint32_t index;
    int result;
    vcap_menu_item item;
} vcap_menu_itr;

typedef struct
{
    vcap_dev* vd;
    uint32_t index;
    int result;
    vcap_fmt_info info;
} vcap_fmt_itr;

// Size iterator
typedef struct
{
    vcap_dev* vd;
    vcap_fmt_id fmt;
    uint32_t index;
    int result;
    vcap_size size;
} vcap_size_itr;

// Frame rate iterator
typedef struct
{
    vcap_dev* vd;
    vcap_fmt_id fmt;
    vcap_size size;
    uint32_t index;
    int result;
    vcap_rate rate;
} vcap_rate_itr;

///
/// \brief Format handling status codes
///
enum
{
    VCAP_FMT_OK      =  0,    ///< Format is supported
    VCAP_FMT_INVALID = -1,    ///< Format is not supported
    VCAP_FMT_ERROR   = -2     ///< Error reading format descriptor
};

///
/// \brief Control handling status codes
///
enum
{
    VCAP_CTRL_OK        =  0,  ///< Control is supported
    VCAP_CTRL_INACTIVE  = -1,  ///< Control is supported, but inactive
    VCAP_CTRL_READ_ONLY = -2,  ///< Control is presently read-only
    VCAP_CTRL_DISABLED  = -3,  ///< Control is supported, but disabled
    VCAP_CTRL_INVALID   = -4,  ///< Control is not supported
    VCAP_CTRL_ERROR     = -5   ///< Error reading control descriptor
};

///
/// \brief Enumeration handling status codes
///
enum
{
    VCAP_ENUM_OK       =  0,   ///< Successfully enumerated item (valid index)
    VCAP_ENUM_INVALID  = -1,   ///< Invalid index
    VCAP_ENUM_DISABLED = -2,   ///< Valid index, but item is disabled
    VCAP_ENUM_ERROR    = -3    ///< Error enumerating item
};

///
/// \brief Generic iterator error test
///
#define vcap_itr_error(itr) (VCAP_ENUM_ERROR == (itr)->result)

///
/// \brief Custom malloc function type
///
typedef void* (*vcap_malloc_fn)(size_t size);

///
/// \brief Custom free function type
///
typedef void  (*vcap_free_fn)(void* ptr);

//------------------------------------------------------------------------------
///
/// \brief  Set a custom 'malloc' and 'free' function
///
/// This is not strictly necessary since vcap does very little dynamic memory
/// allocation. However, if you are already using a custom allocator you might
/// as well use this function. By default Vcap uses the standard malloc and free.
///
void vcap_set_alloc(vcap_malloc_fn malloc_fp, vcap_free_fn free_fp);

//------------------------------------------------------------------------------
///
/// \brief  Returns a device specific string containing the last error message
/// \param  vd    Pointer to the video device
///
const char* vcap_get_error(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Prints device information to a file descriptor
///
/// This function prints (to a file descriptor) device information (path, driver,
/// bus info etc...), and enumerates and prints information about all formats
/// and controls. For example, to display device information to standard output,
/// call 'vcap_dump_info(vd, stdout)'.
///
/// \param  vd    Pointer to the video device
/// \param  file  The file descriptor
///
/// \returns -1 on error and 0 otherwise
///
int vcap_dump_info(vcap_dev* vd, FILE* file);

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
int vcap_enum_devices(unsigned index, vcap_dev_info* info);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new video device object
///
/// \param  path          Path to system device handle
/// \param  convert       Enables automatic format conversion
/// \param  buffer_count  Number of streaming buffers if > 0, otherwise
///                       streaming is disabled
///
/// \returns NULL on error and a pointer to a video device otherwise
///
vcap_dev* vcap_create_device(const char* path, bool convert, int buffer_count);

//------------------------------------------------------------------------------
///
/// \brief  Destroys a video device object, releasing all resources
///
/// \param  vd  Pointer to the video device
///
void vcap_destroy_device(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Opens a video capture device
///
/// \param  vd  Pointer to the video device
///
/// \returns -1 on error and 0 otherwise
///
int vcap_open(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Closes a video capture device
///
/// Closes the device and deallocates the video device.
///
/// \param  vd  Pointer to the video device
///
/// \returns -1 on error and 0 otherwise
///
int vcap_close(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Starts the stream video stream on the specified device
///
/// \param  vd Pointer to the video device
///
/// \returns -1 on error and 0 otherwise
///
int vcap_start_stream(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Stops the stream video stream on the specified device
///
/// \param  vd Pointer to the video device
///
/// \returns -1 on error and 0 otherwise
///
int vcap_stop_stream(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Returns true if the device is open
/// \param  vd Pointer to the video device
///
bool vcap_is_open(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Returns true if the device is streaming
/// \param  vd Pointer to the video device
///
bool vcap_is_streaming(vcap_dev* vd);

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
int vcap_get_device_info(vcap_dev* vd, vcap_dev_info* info);

//------------------------------------------------------------------------------
///
/// \brief  Returns the size of the frame buffer for the current video device
///         configuration
///
/// \param  vd  Pointer to the video device
///
size_t vcap_get_buffer_size(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Grabs a frame
///
/// Grabs a frame from a video capture device using the specified video device.
///
/// \param  vd           Pointer to the video device
/// \param  buffer_size  Size of frame buffer in bytes
/// \param  buffer       Buffer to read into
///
/// \returns -1 on error and 0 otherwise
///
int vcap_grab(vcap_dev* vd, size_t buffer_size, uint8_t* buffer);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves a format descriptor
///
/// Retrieves a format descriptor for the specified format ID.
///
/// \param  vd    Pointer to the video device
/// \param  fmt   The format ID
/// \param  desc  Pointer to the format descriptor
///
/// \returns VCAP_FMT_OK      if the format descriptor was retrieved successfully,
///          VCAP_FMT_INVALID if the format ID is invalid
///          VCAP_FMT_ERROR   if getting the format descriptor failed
///
int vcap_get_fmt_info(vcap_dev* vd, vcap_fmt_id fmt, vcap_fmt_info* desc);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new format iterator
///
/// Creates and initializes new format iterator for the specified video device.
/// The iterator should be deallocated using 'vcap_free'.
/// \param  vd  Pointer to the video device
///
/// \returns An initialized 'vcap_fmt_itr' struct
///
vcap_fmt_itr vcap_new_fmt_itr(vcap_dev* vd);

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
bool vcap_fmt_itr_next(vcap_fmt_itr* itr, vcap_fmt_info* desc);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new frame size iterator
///
/// Creates and initializes new frame size iterator for the specified frame
/// grabber and format ID. The iterator should be deallocated using 'vcap_free'.
///
/// \param  vd   Pointer to the video device
/// \param  fmt  The format ID
///
/// \returns An initialized 'vcap_size_itr' struct
///
vcap_size_itr vcap_new_size_itr(vcap_dev* vd, vcap_fmt_id fmt);

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
/// \brief  Creates a new frame rate iterator
///
/// Creates and initializes new frame rate iterator for the specified frame
/// grabber, format ID, and frame size. The iterator should be deallocated
/// using 'vcap_free'.
///
/// \param  vd    Pointer to the video device
/// \param  fmt   The format ID
/// \param  size  The frame size
///
/// \returns An initialized 'vcap_rate_itr' struct
///
vcap_rate_itr vcap_new_rate_itr(vcap_dev* vd, vcap_fmt_id fmt, vcap_size size);

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
/// \brief  Gets the current format and frame size
///
/// Retrieves the current format ID and frame size simultaneously for the
/// specified video device.
///
/// \param  vd    Pointer to the video device
/// \param  fmt   Pointer to the format ID
/// \param  size  Pointer to the frame size
///
/// \returns -1 on error and 0 otherwise
///
int vcap_get_fmt(vcap_dev* vd, vcap_fmt_id* fmt, vcap_size* size);

//------------------------------------------------------------------------------
///
/// \brief  Sets the format and frame size
///
/// Sets the format ID and frame size simultaneously for the specified frame
/// grabber.
///
/// \param  vd    Pointer to the video device
/// \param  fmt   The format ID
/// \param  size  The frame size
///
/// \returns -1 on error and 0 otherwise
///
int vcap_set_fmt(vcap_dev* vd, vcap_fmt_id fmt, vcap_size size);

//------------------------------------------------------------------------------
///
/// \brief  Get the current frame rate
///
/// Retrieves the current frame rate for the specified video device.
///
/// \param  vd    Pointer to the video device
/// \param  rate  Pointer to the frame rate
///
/// \returns -1 on error and 0 otherwise
///
int vcap_get_rate(vcap_dev* vd, vcap_rate* rate);

//------------------------------------------------------------------------------
///
/// \brief  Sets the frame rate
///
/// Sets the frame rate for the specified video device.
///
/// \param  vd        Pointer to the video device
/// \param  rate  The frame rate
///
/// \returns -1 on error and 0 otherwise
///
int vcap_set_rate(vcap_dev* vd, vcap_rate rate);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves a control descriptor
///
/// Retrieves the control descriptor for the specified control ID.
///
/// \param  vd    Pointer to the video device
/// \param  ctrl  The control ID
/// \param  desc  Pointer to the control descriptor
///
/// \returns VCAP_CTRL_OK       if the control descriptor was retrieved successfully,
///          VCAP_CTRL_INACTIVE if the control ID is valid, but the control is inactive,
///          VCAP_CTRL_INVALID  if the control ID is invalid, and
///          VCAP_CTRL_ERROR    if getting the control descriptor failed
///
int vcap_get_ctrl_info(vcap_dev* vd, vcap_ctrl_id ctrl, vcap_ctrl_info* desc);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves a control descriptor
///
/// Retrieves the status of the control with the specified ID.
///
/// \param  vd    Pointer to the video device
/// \param  ctrl  The control ID
///
/// \returns VCAP_CTRL_OK        if the control descriptor was retrieved successfully,
///          VCAP_CTRL_INACTIVE  if the control ID is valid, but the control is inactive,
///          VCAP_CTRL_READ_ONLY if the control is active but currently read-only,
///          VCAP_CTRL_INVALID   if the control ID is invalid, and
///          VCAP_CTRL_ERROR     if getting the control descriptor failed
///
int vcap_ctrl_status(vcap_dev* vd, vcap_ctrl_id ctrl);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new control iterator
///
/// Creates and initializes new control iterator for the specified frame
/// grabber. The iterator should be deallocated using 'vcap_free'.
///
/// \param  vd  Pointer to the video device
///
/// \returns An initialized 'vcap_ctrl_itr' struct
///
vcap_ctrl_itr vcap_new_ctrl_itr(vcap_dev* vd);

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
bool vcap_ctrl_itr_next(vcap_ctrl_itr* itr, vcap_ctrl_info* desc);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new menu iterator
///
/// Creates and initializes a new menu iterator for the specified frame
/// grabber and control ID. The iterator should be deallocated using 'vcap_free'.
///
/// \param  vd   Pointer to the video device
/// \param  ctrl The control ID
///
/// \returns An initialized 'vcap_menu_itr' struct
///
vcap_menu_itr vcap_new_menu_itr(vcap_dev* vd, vcap_ctrl_id ctrl);

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
/// \brief  Gets a control's value
///
/// Retrieves the current value of a control with the specified control ID.
///
/// \param  vd     Pointer to the video device
/// \param  ctrl   The control ID
/// \param  value  The control value
///
/// \returns -1 on error and 0 otherwise
///
int vcap_get_ctrl(vcap_dev* vd, vcap_ctrl_id ctrl, int32_t* value);

//------------------------------------------------------------------------------
///
/// \brief  Sets a control's value
///
/// Sets the value of a control value with the specified control ID.
///
/// \param  vd     Pointer to the video device
/// \param  ctrl   The control ID
/// \param  value  The control value
///
/// \returns -1 on error and 0 otherwise
///
int vcap_set_ctrl(vcap_dev* vd, vcap_ctrl_id ctrl, int32_t value);

//------------------------------------------------------------------------------
///
/// \brief  Resets a control's value
///
/// Resets the value of the specified control to its default value.
///
/// \param  vd   Pointer to the video device
/// \param  ctrl The control ID
///
/// \returns -1 on error and 0 otherwise
///
int vcap_reset_ctrl(vcap_dev* vd, vcap_ctrl_id ctrl);

//------------------------------------------------------------------------------
///
/// \brief  Resets a all controls to defaults
///
/// Resets all controls to their default values.
///
/// \param  vd  Pointer to the video device
///
/// \returns -1 on error and 0 otherwise
///
int vcap_reset_all_ctrls(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Get cropping bounds
///
/// Retrieves a rectangle that determines the valid cropping area for the
/// specified video device.
///
/// \param  vd     Pointer to the video device
/// \param  rect   Pointer to the rectangle
///
/// \returns -1 on error and 0 otherwise
///
int vcap_get_crop_bounds(vcap_dev* vd, vcap_rect* rect);

//------------------------------------------------------------------------------
///
/// \brief  Resets cropping rectangle
///
/// Resets the cropping rectange to its default dimensions.
///
/// \param  vd  Pointer to the video device
///
/// \returns -1 on error and 0 otherwise
///
int vcap_reset_crop(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Get the current cropping rectangle
///
/// Retrieves the current cropping rectangle for the specified video device
/// and stores it in 'rect'.
///
/// \param  vd    Pointer to the video device
/// \param  rect  Pointer to rectangle
///
/// \returns -1 on error and 0 otherwise
///
int vcap_get_crop(vcap_dev* vd, vcap_rect* rect);

//------------------------------------------------------------------------------
///
/// \brief  Sets the cropping rectangle
///
/// Sets the cropping rectangle for the specified video device.
///
/// \param  vd    Pointer to the video device
/// \param  rect  Pointer to rectangle
///
/// \returns -1 on error and 0 otherwise
///
int vcap_set_crop(vcap_dev* vd, vcap_rect rect);

#ifdef __cplusplus
}
#endif

#endif // VCAP_H
