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
    \version  3.0.0
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
#define VCAP_VERSION_MAJOR 1
#define VCAP_VERSION_MINOR 0
#define VCAP_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

//
// NOTE: Format IDs, control IDs, and control types are located at the end of
// this file.
//

///
/// \brief Return status codes
///
enum
{
    VCAP_OK      =  0,        ///< Function executed without error
    VCAP_ERROR   = -1,        ///< Error while executing function
    VCAP_INVALID = -2         ///< An argument is invalid
};

///
/// \brief Device handle
///
typedef struct vcap_device vcap_device;

///
/// \brief Generic iterator type
///
typedef struct vcap_iterator vcap_iterator;

///
/// \brief Format ID type
///
typedef uint32_t vcap_format_id;

///
/// \brief Control ID type
///
typedef uint32_t vcap_control_id;

///
/// \brief Control type ID
///
typedef uint8_t vcap_control_type;

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
    uint8_t version_str[16];    ///< Driver version string
    bool streaming;             ///< True if device supports streaming, false otherwise
    bool read;                  ///< True if device supports direct read, false otherwise
} vcap_device_info;

///
/// \brief Format dimensions
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
    vcap_format_id id;          ///< Format ID
    uint8_t name[32];           ///< Format name
    uint8_t fourcc[5];          ///< FourCC string
} vcap_format_info;

///
/// \brief Control info
///
typedef struct
{
    vcap_control_id id;         ///< Control ID
    uint8_t name[32];           ///< Control name
    vcap_control_type type;     ///< Control type
    uint8_t type_name[16];      ///< Control type name
    int32_t min;                ///< The minimum value of the control
    int32_t max;                ///< The maximum value of the control
    int32_t step;               ///< The spacing between consecutive values
    int32_t default_value;      ///< The default value of the control (set when the driver is first loaded)
    bool slider;                ///< Hint that a UI control should be represented as a slider
} vcap_control_info;

///
/// \brief Control status
///
typedef struct
{
    bool read_only;             ///< This control is permanently write only
    bool write_only;            ///< This control is permanently readable only
    bool disabled;              ///< This control is permanently disabled
    bool inactive;              ///< This control is not applicable for the current device configuration
} vcap_control_status;

///
/// \brief Control menu item
///
typedef struct
{
    uint32_t index;            ///< Menu item index (value used to set the control)

    union
    {
        uint8_t str[32];       ///< Menu item name (used if control type is VCAP_CTRL_TYPE_MENU)
        int64_t num;           ///< Menu item value (used if control type is VCAP_CTRL_TYPE_INTEGER_MENU)
    } label;

} vcap_menu_item;

///
/// \brief Defines a rectangle (used by cropping functionality)
///
typedef struct
{
    int32_t top;                ///< Top edge of rectangle
    int32_t left;               ///< Left edge of rectangle
    int32_t width;              ///< Width of rectangle
    int32_t height;             ///< Height of rectangle
} vcap_rect;

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
const char* vcap_get_error(vcap_device* vd);

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
int vcap_dump_info(vcap_device* vd, FILE* file);

//------------------------------------------------------------------------------
///
/// \brief  Enumerates video capture devices
///
/// Retrieves the video capture device at the specified 'index' and stores
/// the corresponding device information in the 'vcap_device_info' struct pointed
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
int vcap_enumerate_devices(uint32_t index, vcap_device_info* info);

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
vcap_device* vcap_create_device(const char* path, bool convert, uint32_t buffer_count);

//------------------------------------------------------------------------------
///
/// \brief  Destroys a video device object, stopping capure and
///         releasing all resources
///
/// \param  vd  Pointer to the video device
///
void vcap_destroy_device(vcap_device* vd);

//------------------------------------------------------------------------------
///
/// \brief  Opens a video capture device
///
/// \param  vd  Pointer to the video device
///
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_open(vcap_device* vd);

//------------------------------------------------------------------------------
///
/// \brief  Stops capture and closes the video capture device
///
/// \param  vd  Pointer to the video device
///
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
void vcap_close(vcap_device* vd);

//------------------------------------------------------------------------------
///
/// \brief  Starts the video stream on the specified device
///
/// \param  vd  Pointer to the video device
///
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_start_stream(vcap_device* vd);

//------------------------------------------------------------------------------
///
/// \brief  Stops the video stream on the specified device
///
/// \param  vd  Pointer to the video device
///
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_stop_stream(vcap_device* vd);

//------------------------------------------------------------------------------
///
/// \brief  Returns true if the device is open
///
/// \param  vd  Pointer to the video device
///
bool vcap_is_open(vcap_device* vd);

//------------------------------------------------------------------------------
///
/// \brief  Returns true if the device is streaming
///
/// \param  vd  Pointer to the video device
///
bool vcap_is_streaming(vcap_device* vd);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves video capture device information
///
/// This function retrieves device information from an open video device.
///
/// \param  vd      An open video device
/// \param  device  Pointer to the struct in which to store the device info
///
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_get_device_info(vcap_device* vd, vcap_device_info* info);

//------------------------------------------------------------------------------
///
/// \brief  Returns the size of the device's frame buffer (image)
///
/// Return the size of the frame buffer for the current video device and
/// configuration (format/frame size). This size is used with the function
/// `vcap_capture`
///
/// \param  vd  Pointer to the video device
///
/// \return 0 on error and frame size otherwise
///
size_t vcap_get_image_size(vcap_device* vd);

//------------------------------------------------------------------------------
///
/// \brief  Captures a video frame (image)
///
/// Captures a frame from the video capture device.
///
/// \param  vd          Pointer to the video device
/// \param  image_size  Size of the image in bytes
/// \param  image_data  Previously allocated buffer to read into (equal to
///
/// \returns VCAP_ERROR on error and VCAP_OK otherwise
///
int vcap_capture(vcap_device* vd, size_t image_size, uint8_t* image_data);

//------------------------------------------------------------------------------
///
/// \brief Tests if an error occurred while creating or advancing an iterator
///
/// \param  iterator  The iterator to test
///
bool vcap_iterator_error(vcap_iterator* iterator);

//------------------------------------------------------------------------------
///
/// \brief Deallocates an iterator
///
/// \param  iterator  The iterator to deallocate
///
void vcap_free_iterator(vcap_iterator* iterator);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves format information
///
/// Retrieves format info for the specified format ID.
///
/// \param  vd    Pointer to the video device
/// \param  fmt   The format ID
/// \param  info  Pointer to the format information
///
/// \returns VCAP_OK      if the format info was retrieved successfully,
///          VCAP_ERROR   if getting the format info failed
///          VCAP_INVALID if the format ID is invalid
///
int vcap_get_format_info(vcap_device* vd, vcap_format_id fmt, vcap_format_info* info);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new format iterator
///
/// Creates and initializes new format iterator for the specified video device.
///
/// \param  vd  Pointer to the video device
///
/// \returns An initialized 'vcap_format_iterator' struct
///
vcap_iterator* vcap_format_iterator(vcap_device* vd);

//------------------------------------------------------------------------------
///
/// \brief  Copies format info and advances the format iterator
///
/// Copies the current format information into 'info' and advances the iterator.
///
/// \param  itr   Pointer to iterator
/// \param  info  Pointer to the format information
///
/// \returns false if there was an error or there are no more formats, and true
///          otherwise
///
bool vcap_next_format(vcap_iterator* itr, vcap_format_info* info);

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
/// \returns An initialized 'vcap_size_iterator' struct
///
vcap_iterator* vcap_size_iterator(vcap_device* vd, vcap_format_id fmt);

//------------------------------------------------------------------------------
///
/// \brief  Copies size info and advances the frame size iterator
///
/// Copies the current frame size into 'size' and advances the iterator.
///
/// \param  itr   Pointer to iterator
/// \param  size  Pointer to the frame size
///
/// \returns false if there was an error or there are no more sizes, and true
///          otherwise
///
bool vcap_next_size(vcap_iterator* itr, vcap_size* size);

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
/// \returns An initialized 'vcap_rate_iterator' struct
///
vcap_iterator* vcap_rate_iterator(vcap_device* vd, vcap_format_id fmt, vcap_size size);

///
/// \brief  Copies frame rate and advances the frame rate iterator
///
/// Copies the current frame rate into 'rate' and advances the iterator.
///
/// \param  itr   Pointer to iterator
/// \param  rate  Pointer to the frame rate
///
/// \returns false if there was an error or there are no more sizes, and true
///          otherwise
///
bool vcap_next_rate(vcap_iterator* itr, vcap_rate* rate);

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
///          VCAP_ERROR    if getting the format failed
///
int vcap_get_format(vcap_device* vd, vcap_format_id* fmt, vcap_size* size);

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
///          VCAP_ERROR    if setting the format failed
///
int vcap_set_format(vcap_device* vd, vcap_format_id fmt, vcap_size size);

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
int vcap_get_rate(vcap_device* vd, vcap_rate* rate);

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
int vcap_set_rate(vcap_device* vd, vcap_rate rate);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves control information
///
/// Retrieves control info for the specified control ID.
///
/// \param  vd    Pointer to the video device
/// \param  ctrl  The control ID
/// \param  info  Pointer to the control info
///
/// \returns VCAP_OK       if the control info was retrieved successfully,
///          VCAP_ERROR    if getting the control info failed
///          VCAP_INVALID  if the control ID is invalid, and
///
int vcap_get_control_info(vcap_device* vd, vcap_control_id ctrl, vcap_control_info* info);

//------------------------------------------------------------------------------
///
/// \brief  Retrieves control status information
///
/// Retrieves status information for the specified control ID.
///
/// \param  vd      Pointer to the video device
/// \param  ctrl    The control ID
/// \param  status  Pointer to the control info
///
/// \returns VCAP_OK       if the control info was retrieved successfully,
///          VCAP_ERROR    if getting the control info failed
///          VCAP_INVALID  if the control ID is invalid, and
///
int vcap_get_control_status(vcap_device* vd, vcap_control_id ctrl, vcap_control_status* status);

//------------------------------------------------------------------------------
///
/// \brief  Creates a new control iterator
///
/// Creates and initializes new control iterator for the specified device.
///
/// \param  vd  Pointer to the video device
///
/// \returns An initialized 'vcap_ctrl_iterator' struct
///
vcap_iterator* vcap_control_iterator(vcap_device* vd);

//------------------------------------------------------------------------------
///
/// \brief  Copies control info and advances the control iterator
///
/// Copies the current control information into 'info' and advances the iterator.
///
/// \param  itr   Pointer to iterator
/// \param  info  Pointer to the control information (output)
///
/// \returns false if there was an error or there are no more controls, and true
///          otherwise
///
bool vcap_next_control(vcap_iterator* itr, vcap_control_info* info);

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
/// \returns An initialized 'vcap_menu_iterator' struct
///
vcap_iterator* vcap_menu_iterator(vcap_device* vd, vcap_control_id ctrl);

//------------------------------------------------------------------------------
///
/// \brief  Copies control menu data and advances the iterator
///
/// Retrieves the current value of a control with the specified control ID.
///
/// \param  itr   Pointer to the iterator
/// \param  item  Menu item pointer (output)
///
/// \returns false if there was an error or there are no more controls, and true
///          otherwise
///
bool vcap_next_menu_item(vcap_iterator* itr, vcap_menu_item* item);

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
/// \returns VCAP_OK       if the control value was retrieved
///          VCAP_ERROR    if an error occured
///          VCAP_INVALID  if an the control ID is invaLid
///
int vcap_get_control(vcap_device* vd, vcap_control_id ctrl, int32_t* value);

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
/// \returns VCAP_OK       if the control value was set
///          VCAP_ERROR    if an error occured
///          VCAP_INVALID  if the control ID is invaLid
///
int vcap_set_control(vcap_device* vd, vcap_control_id ctrl, int32_t value);

//------------------------------------------------------------------------------
///
/// \brief  Resets a control's value
///
/// Resets the value of the specified control to its default value.
///
/// \param  vd   Pointer to the video device
/// \param  ctrl The control ID
///
/// \returns VCAP_OK       if the control value was reset
///          VCAP_ERROR    if an error occured
///          VCAP_INVALID  if the control ID is invaLid
///
int vcap_reset_control(vcap_device* vd, vcap_control_id ctrl);

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
int vcap_reset_all_controls(vcap_device* vd);
// int vcap_reset_all(vcap_device* vd);?

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
int vcap_get_crop_bounds(vcap_device* vd, vcap_rect* rect);

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
int vcap_reset_crop(vcap_device* vd);

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
int vcap_get_crop(vcap_device* vd, vcap_rect* rect);

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
int vcap_set_crop(vcap_device* vd, vcap_rect rect);

///
/// \brief Pixel format IDs
///
enum
{
    /* RGB formats */
    VCAP_FMT_BGR24,        ///< 24  BGR-8-8-8
    VCAP_FMT_RGB24,        ///< 24  RGB-8-8-8

    /* 8-Greyscale */
    VCAP_FMT_GREY,         ///<  8  Greyscale

    /* Luminance+Chrominance formats */
    VCAP_FMT_YUYV,         ///< 16  YUV 4:2:2
    VCAP_FMT_YVYU,         ///< 16 YVU 4:2:2
    VCAP_FMT_UYVY,         ///< 16  YUV 4:2:2
    VCAP_FMT_HM12,         ///<  8  YUV 4:2:0 16x16 macroblocks

    /* three planes - Y Cb,  Cr */
    VCAP_FMT_YUV420,       ///< 12  YUV 4:2:0
    VCAP_FMT_YVU420,       ///< 12  YVU 4:2:0

    /* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
    VCAP_FMT_SBGGR8,       ///<  8  BGBG.. GRGR..
    VCAP_FMT_SGBRG8,       ///<  8  GBGB.. RGRG..
    VCAP_FMT_SGRBG8,       ///<  8  GRGR.. BGBG..
    VCAP_FMT_SRGGB8,       ///<  8  RGRG.. GBGB..

    /* compressed formats */
    VCAP_FMT_MJPEG,        ///< Motion-JPEG
    VCAP_FMT_JPEG,         ///< JFIF JPEG

    /*  Vendor-specific formats */
    VCAP_FMT_SN9C10X,      ///< SN9C10x compression
    VCAP_FMT_SN9C20X_I420, ///< SN9C20x YUV 4:2:0
    VCAP_FMT_SPCA501,      ///< YUYV per line
    VCAP_FMT_SPCA505,      ///< YYUV per line
    VCAP_FMT_SPCA508,      ///< YUVY per line
    VCAP_FMT_SPCA561,      ///< compressed GBRG bayer
    VCAP_FMT_PAC207,       ///< compressed BGGR bayer
    VCAP_FMT_OV511,        ///< ov511 JPEG
    VCAP_FMT_OV518,        ///< ov518 JPEG
    VCAP_FMT_MR97310A,     ///< compressed BGGR bayer
    VCAP_FMT_SQ905C,       ///< compressed RGGB bayer
    VCAP_FMT_PJPG,         ///< Pixart 73xx JPEG
    VCAP_FMT_COUNT,        ///< Number of formats
    VCAP_FMT_UNKNOWN       ///< Unrecognized format
};

///
/// \brief Camera control types
///
enum
{
    VCAP_CTRL_TYPE_INTEGER,      ///< Control has an integer value
    VCAP_CTRL_TYPE_BOOLEAN,      ///< On/off control
    VCAP_CTRL_TYPE_MENU,         ///< Menu control where the label is represented by a string
    VCAP_CTRL_TYPE_INTEGER_MENU, ///< Menu control where the label is represented by an integer
    VCAP_CTRL_TYPE_BUTTON,       ///< Button control
    VCAP_CTRL_TYPE_UNKNOWN       ///< Control type is not supported
};

///
/// \brief Camera control IDs
///
enum
{
    /* User controls */
    VCAP_CTRL_BRIGHTNESS,                 ///< Brightness or black level (Integer)
    VCAP_CTRL_CONTRAST,                   ///< Contrast or luma gain (Integer)
    VCAP_CTRL_SATURATION,                 ///< Color saturation or chroma gain (Integer)
    VCAP_CTRL_HUE,                        ///< Hue or color balance (Integer)
    VCAP_CTRL_AUTO_WHITE_BALANCE,         ///< Automatic white balance (Boolean)
    VCAP_CTRL_DO_WHITE_BALANCE,           ///< Will do white balance and then keep the current setting (Button)
    VCAP_CTRL_RED_BALANCE,                ///< Red chroma balance (Integer)
    VCAP_CTRL_BLUE_BALANCE,               ///< Blue chroma balance (Integer)
    VCAP_CTRL_GAMMA,                      ///< Gamma adjust (Integer)
    VCAP_CTRL_EXPOSURE,                   ///< Exposure (Integer)
    VCAP_CTRL_AUTOGAIN,                   ///< Automatic gain/exposure (Boolean)
    VCAP_CTRL_GAIN,                       ///< Gain (Integer)
    VCAP_CTRL_HFLIP,                      ///< Flip image horizontally (Boolean)
    VCAP_CTRL_VFLIP,                      ///< Flip image vertically (Boolean)
    VCAP_CTRL_POWER_LINE_FREQUENCY,       ///< Power line frequency filter to avoid flicker (Enum)
    VCAP_CTRL_HUE_AUTO,                   ///< Automatic hue (Boolean)
    VCAP_CTRL_WHITE_BALANCE_TEMPERATURE,  ///< White balance a color temperature in Kelvin (Integer)
    VCAP_CTRL_SHARPNESS,                  ///< Sharpness filter (Integer)
    VCAP_CTRL_BACKLIGHT_COMPENSATION,     ///< Backlight compensation (Integer)
    VCAP_CTRL_CHROMA_AGC,                 ///< Chroma automatic gain (Boolean)
    VCAP_CTRL_CHROMA_GAIN,                ///< Adjusts the Chroma gain Integer
    VCAP_CTRL_COLOR_KILLER,               ///< Color killer (force a black & white image) (Boolean)
    VCAP_CTRL_AUTOBRIGHTNESS,             ///< Automatic brightness (Boolean)
    VCAP_CTRL_ROTATE,                     ///< Rotates the image by specified angle (Integer)
    VCAP_CTRL_ILLUMINATORS_1,             ///< Switch on or off the illuminator (Boolean)
    VCAP_CTRL_ILLUMINATORS_2,             ///< Switch on or off the illuminator (Boolean)
    VCAP_CTRL_ALPHA_COMPONENT,            ///< Alpha color component (Integer)

    /* Camera controls */
    VCAP_CTRL_EXPOSURE_AUTO,              ///< Automatic exposure time and/or iris aperture (Enum)
    VCAP_CTRL_EXPOSURE_ABSOLUTE,          ///< Exposure time of the camera sensor (Integer)
    VCAP_CTRL_EXPOSURE_AUTO_PRIORITY,     ///< Dynamically vary the frame rate when using APERTURE_PRIORITY (Boolean)
    VCAP_CTRL_AUTO_EXPOSURE_BIAS,         ///< Determines the automatic exposure compensation (Integer Menu)
    VCAP_CTRL_EXPOSURE_METERING,          ///< Amount of light available for the frame exposure (Enum)
    VCAP_CTRL_PAN_RELATIVE,               ///< Turns the camera horizontally (Integer)
    VCAP_CTRL_TILT_RELATIVE,              ///< Turns the camera vertically by the specified amount (Integer)
    VCAP_CTRL_PAN_RESET,                  ///< Turns the camera horizontally to the default position (Button)
    VCAP_CTRL_TILT_RESET,                 ///< Turns the camera vertically to the default position (Button)
    VCAP_CTRL_PAN_ABSOLUTE,               ///< Turns the camera horizontally to the specified position (Integer)
    VCAP_CTRL_TILT_ABSOLUTE,              ///< Turns the camera vertically to the specified position (Integer)
    VCAP_CTRL_FOCUS_ABSOLUTE,             ///< Sets the focal point of the camera (Integer)
    VCAP_CTRL_FOCUS_RELATIVE,             ///< Moves the focal point of the camera (Integer)
    VCAP_CTRL_FOCUS_AUTO,                 ///< Enables continuous automatic focus adjustments (Boolean)
    VCAP_CTRL_AUTO_FOCUS_START,           ///< Starts single auto focus process (Button)
    VCAP_CTRL_AUTO_FOCUS_STOP,            ///< Aborts automatic focusing (Button)
    VCAP_CTRL_AUTO_FOCUS_RANGE,           ///< Auto focus distance range (Enum)
    VCAP_CTRL_ZOOM_ABSOLUTE,              ///< Objective lens focal length (Integer)
    VCAP_CTRL_ZOOM_RELATIVE,              ///< Objective lens focal length relative to current value (Integer)
    VCAP_CTRL_ZOOM_CONTINUOUS,            ///< Continuous zoom (Integer)
    VCAP_CTRL_IRIS_ABSOLUTE,              ///< Sets the camera's aperture (Integer)
    VCAP_CTRL_IRIS_RELATIVE,              ///< Sets the camera's aperture relative to the curret value (Integer)
    VCAP_CTRL_PRIVACY,                    ///< Prevent video from being acquired by the camera (Boolean)
    VCAP_CTRL_BAND_STOP_FILTER,           ///< Band-stop filter of a camera sensor (Integer)
    VCAP_CTRL_WIDE_DYNAMIC_RANGE,         ///< Wide dynamic range (clear images where intensity of illumination varies significantly) (Boolean)
    VCAP_CTRL_IMAGE_STABILIZATION,        ///< Image stabilization (Boolean)
    VCAP_CTRL_PAN_SPEED,                  ///< Turns the camera horizontally at the specified speed (Integer)
    VCAP_CTRL_TILT_SPEED,                 ///< Turns the camera vertically at the specified speed (Integer)
    VCAP_CTRL_COUNT,                      ///< Number of controls
    VCAP_CTRL_UNKNOWN                     ///< Unsupported control
};

#ifdef __cplusplus
}
#endif

#endif // VCAP_H
