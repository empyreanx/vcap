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

/*! \file vcap.h
    \mainpage Vcap
    \version  1.0.0
    \author   James McLean
    \date     August 20, 2022
    \brief    An open source Video4Linux2 (V4L2) capture library written in C.

    \subsection About

    Vcap aims to provide a concise API for working with cameras and other video
    capture devices that have drivers implementing the V4L2 specification. It is
    built on top of the libv4l userspace library (the only dependency) which
    provides seamless decoding for a variety of formats.

    Vcap provides simple, low-level access to device controls and formats,
    enabling applications to make easy use of the full range of functionality
    provided by V4L2.

    You can find the V4L2 documentation here:
    https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/v4l2.html
*/

#ifndef VCAP_H
#define VCAP_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

// Version information
#define VCAP_VERSION_MAJOR 3
#define VCAP_VERSION_MINOR 0
#define VCAP_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

//
// NOTE: Format IDs, control IDs, and control types are located at the bottom of
// this file.
//

///
/// \brief Return status codes
///
enum
{
    VCAP_OK      =  0,        ///< Function executed without error
    VCAP_ERROR   = -1,        ///< Error while executing function
    VCAP_INVALID = -2,        ///< Argument is invalid
};

///
/// \brief Control ID type
///
typedef uint32_t vcap_ctrl_id;

///
/// \brief Control type ID
///
typedef uint8_t vcap_ctrl_type;

///
/// \brief Control status
///
typedef uint16_t vcap_ctrl_status;

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
    bool streaming;             ///< True if device supports streaming, false otherwise
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

//
// \brief Control info
//
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

//
// Control iterator (internal use only)
//
typedef struct
{
    vcap_dev* vd;
    uint32_t index;
    int result;
    vcap_ctrl_info info;
} vcap_ctrl_itr;

//
// Menu iterator (internal use only)
//
typedef struct
{
    vcap_dev* vd;
    vcap_ctrl_id ctrl;
    uint32_t index;
    int result;
    vcap_menu_item item;
} vcap_menu_itr;

//
// Format iterator (internal use only)
//
typedef struct
{
    vcap_dev* vd;
    uint32_t index;
    int result;
    vcap_fmt_info info;
} vcap_fmt_itr;

//
// Size iterator (internal use only)
//
typedef struct
{
    vcap_dev* vd;
    vcap_fmt_id fmt;
    uint32_t index;
    int result;
    vcap_size size;
} vcap_size_itr;

//
// Frame rate iterator (internal use only)
//
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
/// \brief Generic iterator error test
///
#define vcap_itr_error(itr) (VCAP_ERROR == (itr)->result)

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
/// \param  vd  Pointer to the video device
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
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_dump_info(vcap_dev* vd, FILE* file);

//------------------------------------------------------------------------------
///
/// \brief  Enumerates video capture devices
///
/// Retrieves the video capture device at the specified 'index' and stores
/// the corresponding device information in the 'vcap_dev_info' struct pointed
/// to by the 'device' parameter. NOTE: this function calls a system function
/// that uses the default 'malloc' internally. Unfortunately this is
/// unavoidable.
///
/// \param  device  Pointer to the device info struct
/// \param  index   The index of the device to query
///
/// \returns VCAP_OK      if the device information was retrieved successfully,
///          VCAP_INVALID if the index is invalid, and
///          VCAP_ERROR   if querying the device failed.
///
int vcap_enum_devices(uint32_t index, vcap_dev_info* info);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new video device object
///
/// \param  path          Path to system device
/// \param  convert       Enables automatic format conversion
/// \param  buffer_count  Number of streaming buffers if > 0, otherwise
///                       streaming is disabled
///
/// \returns NULL on error and a pointer to a video device otherwise
///
vcap_dev* vcap_create_device(const char* path, bool convert, uint32_t buffer_count);

//------------------------------------------------------------------------------
///
/// \brief  Destroys a video device object, stopping capure and
///         releasing all resources
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
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_open(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Stops capture and closes the video capture device
///
/// \param  vd  Pointer to the video device
///
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
void vcap_close(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Starts the video stream on the specified device
///
/// \param  vd  Pointer to the video device
///
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_start_stream(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Stops the video stream on the specified device
///
/// \param  vd  Pointer to the video device
///
/// \returns -1 on error and 0 otherwise
///
int vcap_stop_stream(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Returns true if the device is open
///
/// \param  vd  Pointer to the video device
///
bool vcap_is_open(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Returns true if the device is streaming
///
/// \param  vd  Pointer to the video device
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
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_get_device_info(vcap_dev* vd, vcap_dev_info* info);

//------------------------------------------------------------------------------
///
/// \brief  Returns the size of the device's frame buffer (image)
///
/// Return the size of the frame buffer for the current video device and
/// configuration (format/frame size). This size is used with the function
/// `vcap_grab`
///
/// \param  vd  Pointer to the video device
///
/// \return 0 on error and frame size otherwise
///
size_t vcap_get_image_size(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Grabs a video frame (image)
///
/// Grabs a frame from the video capture device.
///
/// \param  vd    Pointer to the video device
/// \param  size  Size of the frame in bytes
/// \param  data  Previously allocated buffer to read into
///
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_grab(vcap_dev* vd, size_t size, uint8_t* data);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves format info
///
/// Retrieves format info for the specified format ID.
///
/// \param  vd    Pointer to the video device
/// \param  fmt   The format ID
/// \param  info  Pointer to the format information
///
/// \returns VCAP_OK      if the format info was retrieved successfully,
///          VCAP_INVALID if the format ID is invalid
///          VCAP_ERROR   if getting the format info failed
///
int vcap_get_fmt_info(vcap_dev* vd, vcap_fmt_id fmt, vcap_fmt_info* info);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new format iterator
///
/// Creates and initializes new format iterator for the specified video device.
///
/// \param  vd  Pointer to the video device
///
/// \returns An initialized 'vcap_fmt_itr' struct
///
vcap_fmt_itr vcap_new_fmt_itr(vcap_dev* vd);

//------------------------------------------------------------------------------
///
/// \brief  Advances the specified format iterator
///
/// Copies the current format information into 'info' and advances the iterator.
///
/// \param  itr   Pointer to iterator
/// \param  info  Pointer to the format information
///
/// \returns false if there was an error or there are no more formats, and true
///          otherwise
///
bool vcap_fmt_itr_next(vcap_fmt_itr* itr, vcap_fmt_info* info);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new frame size iterator
///
/// Creates and initializes new frame size iterator for the specified device
/// and format ID.
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
/// \returns false if there was an error or there are no more sizes, and true
///          otherwise
///
bool vcap_size_itr_next(vcap_size_itr* itr, vcap_size* size);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new frame rate iterator
///
/// Creates and initializes new frame rate iterator for the specified device,
/// format ID, and frame size.
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
/// \returns false if there was an error or there are no more sizes, and true
///          otherwise
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
/// \param  fmt   Pointer to the format ID (output)
/// \param  size  Pointer to the frame size (output)
///
/// \returns VCAP_OK       if the format was retrieved successfully,
///          VCAP_INVALID  if the format ID is invalid, and
///          VCAP_ERROR    if getting the format failed
///
int vcap_get_fmt(vcap_dev* vd, vcap_fmt_id* fmt, vcap_size* size);

//------------------------------------------------------------------------------
///
/// \brief  Sets the format and frame size
///
/// Sets the format ID and frame size simultaneously for the specified device
///
/// \param  vd    Pointer to the video device
/// \param  fmt   The format ID
/// \param  size  The frame size
///
/// \returns VCAP_OK       if the format was set successfully,
///          VCAP_INVALID  if the format ID is invalid, and
///          VCAP_ERROR    if setting the format failed
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
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_get_rate(vcap_dev* vd, vcap_rate* rate);

//------------------------------------------------------------------------------
///
/// \brief  Sets the frame rate
///
/// Sets the frame rate for the specified video device.
///
/// \param  vd    Pointer to the video device
/// \param  rate  The frame rate
///
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_set_rate(vcap_dev* vd, vcap_rate rate);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves control info
///
/// Retrieves control info for the specified control ID.
///
/// \param  vd    Pointer to the video device
/// \param  ctrl  The control ID
/// \param  info  Pointer to the control info
///
/// \returns VCAP_OK       if the control info was retrieved successfully,
///          VCAP_INVALID  if the control ID is invalid, and
///          VCAP_ERROR    if getting the control info failed
///
int vcap_get_ctrl_info(vcap_dev* vd, vcap_ctrl_id ctrl, vcap_ctrl_info* info);

///
/// \brief Control status codes
///
enum
{
    VCAP_CTRL_OK        = (0 << 0),  ///< Control is supported
    VCAP_CTRL_INACTIVE  = (1 << 0),  ///< Control is supported, but inactive
    VCAP_CTRL_READ_ONLY = (1 << 1),  ///< Control is presently read-only
    VCAP_CTRL_DISABLED  = (1 << 2),  ///< Control is supported, but disabled
};

//------------------------------------------------------------------------------
///
/// \brief  Returns the status of the control
///
/// Retrieves the status of the control with the specified ID and stores them
/// in a bitset
///
/// \param  vd    Pointer to the video device
/// \param  ctrl  The control ID
/// \param  ctrl  The control's status (output)
///
/// \returns VCAP_OK       if the control status was retrieved successfully,
///          VCAP_INVALID  if the control ID is invalid, and
///          VCAP_ERROR    if getting the control status failed
///
int vcap_get_ctrl_status(vcap_dev* vd, vcap_ctrl_id ctrl, vcap_ctrl_status* status);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new control iterator
///
/// Creates and initializes new control iterator for the specified device.
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
/// Copies the current control information into 'info' and advances the iterator.
///
/// \param  itr   Pointer to iterator
/// \param  info  Pointer to the control information (output)
///
/// \returns false if there was an error or there are no more controls, and true
///          otherwise
///
bool vcap_ctrl_itr_next(vcap_ctrl_itr* itr, vcap_ctrl_info* info);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new control menu iterator
///
/// Creates and initializes a new control menu iterator for the specified
/// device and control
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
/// \param  item  Pointer to the menu item (output)
///
/// \returns false if there was an error or there are no more menu items, and
///          true otherwise
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
/// \param  value  The control value (output)
///
/// \returns VCAP_OK      if the control value was retrieved
///          VCAP_INVALID if the control ID is invalid
///          VCAP_ERROR   if an error occured
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
/// \returns VCAP_OK      if the control value was set
///          VCAP_INVALID if the control ID is invalid
///          VCAP_ERROR   if an error occured
///
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
/// \returns VCAP_OK      if the control value was reset
///          VCAP_INVALID if the control ID is invalid
///          VCAP_ERROR   if an error occured
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
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
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
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
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
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
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
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
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
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_set_crop(vcap_dev* vd, vcap_rect rect);

///
/// \brief Pixel format IDs
///
enum
{
    /* RGB formats */
    VCAP_FMT_RGB332,    ///<  8  RGB-3-3-2
    VCAP_FMT_RGB444,    ///< 16  xxxxrrrr ggggbbbb
    VCAP_FMT_ARGB444,   ///< 16  aaaarrrr ggggbbbb
    VCAP_FMT_XRGB444,   ///< 16  xxxxrrrr ggggbbbb
    VCAP_FMT_RGB555,    ///< 16  RGB-5-5-5
    VCAP_FMT_ARGB555,   ///< 16  ARGB-1-5-5-5
    VCAP_FMT_XRGB555,   ///< 16  XRGB-1-5-5-5
    VCAP_FMT_RGB565,    ///< 16  RGB-5-6-5
    VCAP_FMT_RGB555X,   ///< 16  RGB-5-5-5 BE
    VCAP_FMT_ARGB555X,  ///< 16  ARGB-5-5-5 BE
    VCAP_FMT_XRGB555X,  ///< 16  XRGB-5-5-5 BE
    VCAP_FMT_RGB565X,   ///< 16  RGB-5-6-5 BE
    VCAP_FMT_BGR666,    ///< 18  BGR-6-6-6
    VCAP_FMT_BGR24,     ///< 24  BGR-8-8-8
    VCAP_FMT_RGB24,     ///< 24  RGB-8-8-8
    VCAP_FMT_BGR32,     ///< 32  BGR-8-8-8-8
    VCAP_FMT_ABGR32,    ///< 32  BGRA-8-8-8-8
    VCAP_FMT_XBGR32,    ///< 32  BGRX-8-8-8-8
    VCAP_FMT_RGB32,     ///< 32  RGB-8-8-8-8
    VCAP_FMT_ARGB32,    ///< 32  ARGB-8-8-8-8
    VCAP_FMT_XRGB32,    ///< 32  XRGB-8-8-8-8

    /* Grey formats */
    VCAP_FMT_GREY,      ///<  8  Greyscale
    VCAP_FMT_Y4,        ///<  4  Greyscale
    VCAP_FMT_Y6,        ///<  6  Greyscale
    VCAP_FMT_Y10,       ///< 10  Greyscale
    VCAP_FMT_Y12,       ///< 12  Greyscale
    VCAP_FMT_Y16,       ///< 16  Greyscale
    VCAP_FMT_Y16_BE,    ///< 16  Greyscale BE

    /* Grey bit-packed formats */
    VCAP_FMT_Y10BPACK,    ///< 10  Greyscale bit-packed

    /* Palette formats */
    VCAP_FMT_PAL8,        ///<  8  8-bit palette

    /* Chrominance formats */
    VCAP_FMT_UV8,         ///<  8  UV 4:4

    /* Luminance+Chrominance formats */
    VCAP_FMT_YUYV,    ///< 16  YUV 4:2:2
    VCAP_FMT_YYUV,    ///< 16  YUV 4:2:2
    VCAP_FMT_YVYU,    ///< 16 YVU 4:2:2
    VCAP_FMT_UYVY,    ///< 16  YUV 4:2:2
    VCAP_FMT_VYUY,    ///< 16  YUV 4:2:2
    VCAP_FMT_Y41P,    ///< 12  YUV 4:1:1
    VCAP_FMT_YUV444,  ///< 16  xxxxyyyy uuuuvvvv
    VCAP_FMT_YUV555,  ///< 16  YUV-5-5-5
    VCAP_FMT_YUV565,  ///< 16  YUV-5-6-5
    VCAP_FMT_YUV32,   ///< 32  YUV-8-8-8-8
    VCAP_FMT_HI240,   ///<  8  8-bit color
    VCAP_FMT_HM12,    ///<  8  YUV 4:2:0 16x16 macroblocks
    VCAP_FMT_M420,    ///< 12  YUV 4:2:0 2 lines y, 1 line uv interleaved

    /* two planes -- one Y, one Cr + Cb interleaved  */
    VCAP_FMT_NV12,    ///< 12  Y/CbCr 4:2:0
    VCAP_FMT_NV21,    ///< 12  Y/CrCb 4:2:0
    VCAP_FMT_NV16,    ///< 16  Y/CbCr 4:2:2
    VCAP_FMT_NV61,    ///< 16  Y/CrCb 4:2:2
    VCAP_FMT_NV24,    ///< 24  Y/CbCr 4:4:4
    VCAP_FMT_NV42,    ///< 24  Y/CrCb 4:4:4

    /* two non contiguous planes - one Y, one Cr + Cb interleaved  */
    VCAP_FMT_NV12M,         ///< 12  Y/CbCr 4:2:0
    VCAP_FMT_NV21M,         ///< 21  Y/CrCb 4:2:0
    VCAP_FMT_NV16M,         ///< 16  Y/CbCr 4:2:2
    VCAP_FMT_NV61M,         ///< 16  Y/CrCb 4:2:2
    VCAP_FMT_NV12MT,        ///< 12  Y/CbCr 4:2:0 64x32 macroblocks
    VCAP_FMT_NV12MT_16X16,  ///< 12  Y/CbCr 4:2:0 16x16 macroblocks

    /* three planes - Y Cb, Cr */
    VCAP_FMT_YUV410,  ///<  9  YUV 4:1:0
    VCAP_FMT_YVU410,  ///<  9  YVU 4:1:0
    VCAP_FMT_YUV411P, ///< 12  YVU411 planar
    VCAP_FMT_YUV420,  ///< 12  YUV 4:2:0
    VCAP_FMT_YVU420,  ///< 12  YVU 4:2:0
    VCAP_FMT_YUV422P, ///< 16  YVU422 planar

    /* three non contiguous planes - Y, Cb, Cr */
    VCAP_FMT_YUV420M, ///< 12  YUV420 planar
    VCAP_FMT_YVU420M, ///< 12  YVU420 planar
    VCAP_FMT_YUV422M, ///< 16  YUV422 planar
    VCAP_FMT_YVU422M, ///< 16  YVU422 planar
    VCAP_FMT_YUV444M, ///< 24  YUV444 planar
    VCAP_FMT_YVU444M, ///< 24  YVU444 planar

    /* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
    VCAP_FMT_SBGGR8,  ///<  8  BGBG.. GRGR..
    VCAP_FMT_SGBRG8,  ///<  8  GBGB.. RGRG..
    VCAP_FMT_SGRBG8,  ///<  8  GRGR.. BGBG..
    VCAP_FMT_SRGGB8,  ///<  8  RGRG.. GBGB..
    VCAP_FMT_SBGGR10, ///< 10  BGBG.. GRGR..
    VCAP_FMT_SGBRG10, ///< 10  GBGB.. RGRG..
    VCAP_FMT_SGRBG10, ///< 10  GRGR.. BGBG..
    VCAP_FMT_SRGGB10, ///< 10  RGRG.. GBGB..

    /* 10bit raw bayer packed, 5 bytes for every 4 pixels */
    VCAP_FMT_SBGGR10P,
    VCAP_FMT_SGBRG10P,
    VCAP_FMT_SGRBG10P,
    VCAP_FMT_SRGGB10P,

    /* 10bit raw bayer a-law compressed to 8 bits */
    VCAP_FMT_SBGGR10ALAW8,
    VCAP_FMT_SGBRG10ALAW8,
    VCAP_FMT_SGRBG10ALAW8,
    VCAP_FMT_SRGGB10ALAW8,

    /* 10bit raw bayer DPCM compressed to 8 bits */
    VCAP_FMT_SBGGR10DPCM8,
    VCAP_FMT_SGBRG10DPCM8,
    VCAP_FMT_SGRBG10DPCM8,
    VCAP_FMT_SRGGB10DPCM8,
    VCAP_FMT_SBGGR12, ///< 12  BGBG.. GRGR..
    VCAP_FMT_SGBRG12, ///< 12  GBGB.. RGRG..
    VCAP_FMT_SGRBG12, ///< 12  GRGR.. BGBG..
    VCAP_FMT_SRGGB12, ///< 12  RGRG.. GBGB..

    /* 12bit raw bayer packed, 6 bytes for every 4 pixels */
    VCAP_FMT_SBGGR12P,
    VCAP_FMT_SGBRG12P,
    VCAP_FMT_SGRBG12P,
    VCAP_FMT_SRGGB12P,
    VCAP_FMT_SBGGR16, ///< 16  BGBG.. GRGR..
    VCAP_FMT_SGBRG16, ///< 16  GBGB.. RGRG..
    VCAP_FMT_SGRBG16, ///< 16  GRGR.. BGBG..
    VCAP_FMT_SRGGB16, ///< 16  RGRG.. GBGB..

    /* HSV formats */
    VCAP_FMT_HSV24,
    VCAP_FMT_HSV32,

    /* compressed formats */
    VCAP_FMT_MJPEG,       ///< Motion-JPEG
    VCAP_FMT_JPEG,        ///< JFIF JPEG
    VCAP_FMT_DV,          ///< 1394
    VCAP_FMT_MPEG,        ///< MPEG-1/2/4 Multiplexed
    VCAP_FMT_H264,        ///< H264 with start codes
    VCAP_FMT_H264_NO_SC,  ///< H264 without start codes
    VCAP_FMT_H264_MVC,    ///< H264 MVC
    VCAP_FMT_H263,        ///< H263
    VCAP_FMT_MPEG1,       ///< MPEG-1 ES
    VCAP_FMT_MPEG2,       ///< MPEG-2 ES
    VCAP_FMT_MPEG4,       ///< MPEG-4 part 2 ES
    VCAP_FMT_XVID,        ///< Xvid
    VCAP_FMT_VC1_ANNEX_G, ///< SMPTE 421M Annex G compliant stream
    VCAP_FMT_VC1_ANNEX_L, ///< SMPTE 421M Annex L compliant stream
    VCAP_FMT_VP8,         ///< VP8
    VCAP_FMT_VP9,         ///< VP9

    /*  Vendor-specific formats   */
    VCAP_FMT_CPIA1,         ///< cpia1 YUV
    VCAP_FMT_WNVA,          ///< Winnov hw compress
    VCAP_FMT_SN9C10X,       ///< SN9C10x compression
    VCAP_FMT_SN9C20X_I420,  ///< SN9C20x YUV 4:2:0
    VCAP_FMT_PWC1,          ///< pwc older webcam
    VCAP_FMT_PWC2,          ///< pwc newer webcam
    VCAP_FMT_ET61X251,      ///< ET61X251 compression
    VCAP_FMT_SPCA501,       ///< YUYV per line
    VCAP_FMT_SPCA505,       ///< YYUV per line
    VCAP_FMT_SPCA508,       ///< YUVY per line
    VCAP_FMT_SPCA561,       ///< compressed GBRG bayer
    VCAP_FMT_PAC207,        ///< compressed BGGR bayer
    VCAP_FMT_MR97310A,      ///< compressed BGGR bayer
    VCAP_FMT_JL2005BCD,     ///< compressed RGGB bayer
    VCAP_FMT_SN9C2028,      ///< compressed GBRG bayer
    VCAP_FMT_SQ905C,        ///< compressed RGGB bayer
    VCAP_FMT_PJPG,          ///< Pixart 73xx JPEG
    VCAP_FMT_OV511,         ///< ov511 JPEG
    VCAP_FMT_OV518,         ///< ov518 JPEG
    VCAP_FMT_STV0680,       ///< stv0680 bayer
    VCAP_FMT_TM6000,        ///< tm5600/tm60x0
    VCAP_FMT_CIT_YYVYUY,    ///< one line of Y then 1 line of VYUY
    VCAP_FMT_KONICA420,     ///< YUV420 planar in blocks of 256 pixels
    VCAP_FMT_JPGL,          ///< JPEG-Lite
    VCAP_FMT_SE401,         ///< se401 janggu compressed rgb
    VCAP_FMT_S5C_UYVY_JPG,  ///< S5C73M3 interleaved UYVY/JPEG
    VCAP_FMT_Y8I,           ///< Greyscale 8-bit L/R interleaved
    VCAP_FMT_Y12I,          ///< Greyscale 12-bit L/R interleaved
    VCAP_FMT_Z16,           ///< Depth data 16-bit
    VCAP_FMT_MT21C,         ///< Mediatek compressed block mode
    VCAP_FMT_INZI,          ///< Intel Planar Greyscale 10-bit and Depth 16-bit
    VCAP_FMT_COUNT,
    VCAP_FMT_UNKNOWN,
};

///
/// \brief Camera control types
///
enum
{
    VCAP_CTRL_TYPE_INTEGER,
    VCAP_CTRL_TYPE_BOOLEAN,
    VCAP_CTRL_TYPE_MENU,
    VCAP_CTRL_TYPE_INTEGER_MENU,
    VCAP_CTRL_TYPE_BUTTON,
    VCAP_CTRL_TYPE_UNKNOWN,
};

///
/// \brief Camera control IDs
///
enum
{
    VCAP_CTRL_BRIGHTNESS,                  ///< Integer
    VCAP_CTRL_CONTRAST,                    ///< Integer
    VCAP_CTRL_SATURATION,                  ///< Integer
    VCAP_CTRL_HUE,                         ///< Integer
    VCAP_CTRL_AUTO_WHITE_BALANCE,          ///< Boolean
    VCAP_CTRL_DO_WHITE_BALANCE,            ///< Button
    VCAP_CTRL_RED_BALANCE,                 ///< Integer
    VCAP_CTRL_BLUE_BALANCE,                ///< Integer
    VCAP_CTRL_GAMMA,                       ///< Integer
    VCAP_CTRL_EXPOSURE,                    ///< Integer
    VCAP_CTRL_AUTOGAIN,                    ///< Boolean
    VCAP_CTRL_GAIN,                        ///< Integer
    VCAP_CTRL_HFLIP,                       ///< Boolean
    VCAP_CTRL_VFLIP,                       ///< Boolean
    VCAP_CTRL_POWER_LINE_FREQUENCY,        ///< Enum
    VCAP_CTRL_HUE_AUTO,                    ///< Boolean
    VCAP_CTRL_WHITE_BALANCE_TEMPERATURE,   ///< Integer
    VCAP_CTRL_SHARPNESS,                   ///< Integer
    VCAP_CTRL_BACKLIGHT_COMPENSATION,      ///< Integer
    VCAP_CTRL_CHROMA_AGC,                  ///< Boolean
    VCAP_CTRL_CHROMA_GAIN,                 ///< Integer
    VCAP_CTRL_COLOR_KILLER,                ///< Boolean
    VCAP_CTRL_AUTOBRIGHTNESS,              ///< Boolean
    VCAP_CTRL_ROTATE,                      ///< Integer
    VCAP_CTRL_BG_COLOR,                    ///< Integer
    VCAP_CTRL_ILLUMINATORS_1,              ///< Boolean
    VCAP_CTRL_ILLUMINATORS_2,              ///< Boolean
    VCAP_CTRL_ALPHA_COMPONENT,             ///< Integer
    VCAP_CTRL_EXPOSURE_AUTO,               ///< Enum
    VCAP_CTRL_EXPOSURE_ABSOLUTE,           ///< Integer
    VCAP_CTRL_EXPOSURE_AUTO_PRIORITY,      ///< Boolean
    VCAP_CTRL_AUTO_EXPOSURE_BIAS,          ///< Integer Menu
    VCAP_CTRL_EXPOSURE_METERING,           ///< Enum
    VCAP_CTRL_PAN_RELATIVE,                ///< Integer
    VCAP_CTRL_TILT_RELATIVE,               ///< Integer
    VCAP_CTRL_PAN_RESET,                   ///< Button
    VCAP_CTRL_TILT_RESET,                  ///< Button
    VCAP_CTRL_PAN_ABSOLUTE,                ///< Integer
    VCAP_CTRL_TILT_ABSOLUTE,               ///< Integer
    VCAP_CTRL_FOCUS_ABSOLUTE,              ///< Integer
    VCAP_CTRL_FOCUS_RELATIVE,              ///< Integer
    VCAP_CTRL_FOCUS_AUTO,                  ///< Boolean
    VCAP_CTRL_AUTO_FOCUS_START,            ///< Button
    VCAP_CTRL_AUTO_FOCUS_STOP,             ///< Button
    VCAP_CTRL_AUTO_FOCUS_RANGE,            ///< Enum
    VCAP_CTRL_ZOOM_ABSOLUTE,               ///< Integer
    VCAP_CTRL_ZOOM_RELATIVE,               ///< Integer
    VCAP_CTRL_ZOOM_CONTINUOUS,             ///< Integer
    VCAP_CTRL_IRIS_ABSOLUTE,               ///< Integer
    VCAP_CTRL_IRIS_RELATIVE,               ///< Integer
    VCAP_CTRL_BAND_STOP_FILTER,            ///< Integer
    VCAP_CTRL_WIDE_DYNAMIC_RANGE,          ///< Boolean
    VCAP_CTRL_IMAGE_STABILIZATION,         ///< Boolean
    VCAP_CTRL_PAN_SPEED,                   ///< Integer
    VCAP_CTRL_TILT_SPEED,                  ///< Integer
    VCAP_CTRL_COUNT,
    VCAP_CTRL_UNKNOWN,
};

#ifdef __cplusplus
}
#endif

#endif // VCAP_H
