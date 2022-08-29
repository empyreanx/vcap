/*! \file vcap.h */

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

#include "vcap.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

//
// Memory mapped buffer defintion
//
typedef struct
{
    size_t size;
    void* data;
} vcap_buffer;

//
// Video device definition
//
struct vcap_device
{
    int fd;
    char path[512];
    char error_msg[2048];
    bool open;
    bool streaming;
    bool convert;
    uint32_t buffer_count;
    vcap_buffer* buffers;
    struct v4l2_capability caps;
};

//
// Iterator type
//
typedef enum
{
    VCAP_ITR_FMT,
    VCAP_ITR_SIZE,
    VCAP_ITR_RATE,
    VCAP_ITR_CTRL,
    VCAP_ITR_MENU,
} vcap_itr_type;

//
// Generic iterator
//
struct vcap_iterator
{
    vcap_itr_type type;

    vcap_device* vd;
    uint32_t index;
    int result;

    union
    {
        struct
        {
            vcap_format_info info;
        } fmt;

        struct
        {
            vcap_format_id fmt;
            vcap_size size;
        } size;

        struct
        {
            vcap_format_id fmt;
            vcap_size size;
            vcap_rate rate;
        } rate;

        struct
        {
            vcap_control_info info;
        } ctrl;

        struct
        {
            vcap_control_id ctrl;
            vcap_menu_item item;
        } menu;
    } data;
};

//==============================================================================
// Internal function declarations
//==============================================================================

// Internal malloc
static void* vcap_malloc(size_t size);

// Internal free
static void vcap_free(void* ptr);

// FOURCC character code to string
static void vcap_fourcc_string(uint32_t code, uint8_t* str);

// Extended ioctl function
static int vcap_ioctl(int fd, long unsigned request, void *arg);

// Query device capabilities, used in device enumeration
static int vcap_query_caps(const char* path, struct v4l2_capability* caps);

// Filters device list so that 'scandir' returns only video devices.
static int vcap_video_device_filter(const struct dirent *a);

// Convert device capabilities into device info struct
static void vcap_caps_to_info(const char* path, const struct v4l2_capability caps, vcap_device_info* info);

// Request a number of buffers for streaming
static int vcap_request_buffers(vcap_device* vd);

// Initialize streaming for the given device
static int vcap_init_stream(vcap_device* vd);

// Instructs V4L2 to dispose of any buffers
static int vcap_release_buffers(vcap_device* vd);

// Shutdown stream for given device
static int vcap_shutdown_stream(vcap_device* vd);

// Map memory buffers
static int vcap_map_buffers(vcap_device* vd);

// Unmap memory buffers
static int vcap_unmap_buffers(vcap_device* vd);

// Queue mapped buffers
static int vcap_queue_buffers(vcap_device* vd);

// Grab a frame using memory-mapped buffers
static int vcap_grab_mmap(vcap_device* vd, size_t size, uint8_t* data);

// Grab a frame using a direct read call
static int vcap_grab_read(vcap_device* vd, size_t size, uint8_t* data);

// Safe unsigned string copy
static void vcap_ustrcpy(uint8_t* dst, const uint8_t* src, size_t size);

// Safe regular string copy
static void vcap_strcpy(char* dst, const char* src, size_t size);

// Set error message for specified device
static void vcap_set_error_str(const char* func, int line, vcap_device* vd, const char* fmt, ...);

// Set error message (including errno infomation) for specified device
static void vcap_set_error_errno_str(const char* func, int line, vcap_device* vd, const char* fmt, ...);

// Converts a V4L2 control ID to VCAP control ID
static vcap_control_id vcap_convert_ctrl(uint32_t id);

// Converts a VCAP control ID to the corresponding V4L2 control ID
static uint32_t vcap_map_ctrl(vcap_control_id id);

// Converts a V4L2 control type ID to VCAP control type ID
static vcap_control_type vcap_convert_ctrl_type(uint32_t type);

// Returns true if control type is supported
static bool vcap_ctrl_type_supported(uint32_t type);

// Return string describing a control type
static const char* vcap_ctrl_type_str(vcap_control_type id);

// Enumerates formats
static int vcap_enum_fmts(vcap_device* vd, vcap_format_info* info, uint32_t index);

// Enumerates frame sizes for the given format
static int vcap_enum_sizes(vcap_device* vd, vcap_format_id fmt, vcap_size* size, uint32_t index);

// Enumerates frame rates for the given format and frame size
static int vcap_enum_rates(vcap_device* vd, vcap_format_id fmt, vcap_size size, vcap_rate* rate, uint32_t index);

// Enumerates camera controls
static int vcap_enum_ctrls(vcap_device* vd, vcap_control_info* info, uint32_t index);

// Enumerates a menu for the given control
static int vcap_enum_menu(vcap_device* vd, vcap_control_id ctrl, vcap_menu_item* item, uint32_t index);

// Converts a V4L2 format ID to a VCAP format ID
static vcap_format_id vcap_convert_fmt(uint32_t id);

// Converts a VCAP format ID to a V4L2 ID
static uint32_t vcap_map_fmt(vcap_format_id id);

// Global malloc function pointer
static vcap_malloc_fn global_malloc_fp = malloc;

// Global free function pointer
static vcap_free_fn global_free_fp = free;

//==============================================================================
// Macros
//==============================================================================

// Clear data structure
#define VCAP_CLEAR(arg) memset(&(arg), 0, sizeof(arg))

// Set error message
#define vcap_set_error(...) (vcap_set_error_str(__func__, __LINE__, __VA_ARGS__))

// Set message with errno information
#define vcap_set_error_errno(...) (vcap_set_error_errno_str( __func__, __LINE__,  __VA_ARGS__))

//==============================================================================
// Public API implementation
//==============================================================================

void vcap_set_alloc(vcap_malloc_fn malloc_fp, vcap_free_fn free_fp)
{
    global_malloc_fp = malloc_fp;
    global_free_fp = free_fp;
}

const char* vcap_get_error(vcap_device* vd)
{
    assert(vd != NULL);
    return vd->error_msg;
}

//
// Prints device information. The implementation of this function is very
// pedantic in terms of error checking. Every error condition is checked and
// reported. A user application may choose to ignore some error cases, trading
// a little robustness for some convenience.
//
int vcap_dump_info(vcap_device* vd, FILE* file)
{
    assert(vd != NULL);

    vcap_device_info info;
    vcap_get_device_info(vd, &info);

    //==========================================================================
    // Print device info
    //==========================================================================
    fprintf(file, "------------------------------------------------\n");
    fprintf(file, "Device: %s\n", info.path);
    fprintf(file, "Driver: %s\n", info.driver);
    fprintf(file, "Driver version: %s\n", info.version_str);
    fprintf(file, "Card: %s\n", info.card);
    fprintf(file, "Bus Info: %s\n", info.bus_info);
    fprintf(file, "------------------------------------------------\n");

    fprintf(file, "Streaming mode: ");

    if (info.streaming)
        fprintf(file, "Supported\n");
    else
        fprintf(file, "Not supported\n");

    fprintf(file, "Read mode: ");

    if (info.read)
        fprintf(file, "Supported\n");
    else
        fprintf(file, "Not supported\n");

    //==========================================================================
    // Enumerate formats
    //==========================================================================
    vcap_iterator* fmt_itr = vcap_format_iterator(vd);
    vcap_format_info fmt_info;

    while (vcap_next_format(fmt_itr, &fmt_info))
    {
        fprintf(file, "------------------------------------------------\n");
        fprintf(file, "Format: %s, FourCC: %s\n", fmt_info.name, fmt_info.fourcc);
        fprintf(file, "Sizes:\n");

        //======================================================================
        // Enumerate sizes
        //======================================================================
        vcap_iterator* size_itr = vcap_size_iterator(vd, fmt_info.id);
        vcap_size size;

        while (vcap_next_size(size_itr, &size))
        {
            fprintf(file, "   %u x %u: ", size.width, size.height);
            fprintf(file, "(Frame rates:");

            //==================================================================
            // Enumerate frame rates
            //==================================================================
            vcap_iterator* rate_itr = vcap_rate_iterator(vd, fmt_info.id, size);
            vcap_rate rate;

            while (vcap_next_rate(rate_itr, &rate))
            {
                fprintf(file, " %u/%u", rate.numerator, rate.denominator);
            }

            if (vcap_iterator_error(rate_itr))
            {
                vcap_free_iterator(fmt_itr);
                vcap_free_iterator(size_itr);
                vcap_free_iterator(rate_itr);
                return VCAP_ERROR;
            }

            vcap_free_iterator(rate_itr);

            fprintf(file, ")\n");
        }

        // Check for errors during frame size iteration
        if (vcap_iterator_error(size_itr))
        {
            vcap_free_iterator(fmt_itr);
            vcap_free_iterator(size_itr);
            return VCAP_ERROR;
        }

       vcap_free_iterator(size_itr);
    }

    // Check for errors during format iteration
    if (vcap_iterator_error(fmt_itr))
    {
        vcap_free_iterator(fmt_itr);
        return VCAP_ERROR;
    }

    vcap_free_iterator(fmt_itr);

    //==========================================================================
    // Enumerate controls
    //==========================================================================
    fprintf(file, "------------------------------------------------\n");
    fprintf(file, "Controls:\n");

    vcap_iterator* ctrl_itr = vcap_control_iterator(vd);
    vcap_control_info ctrl_info;

    while (vcap_next_control(ctrl_itr, &ctrl_info))
    {
        printf("   Name: %s, Type: %s\n", ctrl_info.name, ctrl_info.type_name);

        if (ctrl_info.type == VCAP_CTRL_TYPE_MENU || ctrl_info.type == VCAP_CTRL_TYPE_INTEGER_MENU)
        {
            printf("   Menu:\n");

            //==================================================================
            // Enumerate menu
            //==================================================================
            vcap_iterator* menu_itr = vcap_menu_iterator(vd, ctrl_info.id);
            vcap_menu_item menu_item;

            while (vcap_next_menu_item(menu_itr, &menu_item))
            {
                if (ctrl_info.type == VCAP_CTRL_TYPE_MENU)
                    printf("      %i : %s\n", menu_item.index, menu_item.label.str);
                else
                    printf("      %i : %li\n", menu_item.index, menu_item.label.num);
            }

            // Check for errors during menu iteration
            if (vcap_iterator_error(menu_itr))
            {
                vcap_free_iterator(ctrl_itr);
                vcap_free_iterator(menu_itr);
                return VCAP_ERROR;
            }

            vcap_free_iterator(menu_itr);
        }
    }

    // Check for errors during control iteration
    if (vcap_iterator_error(ctrl_itr))
    {
        vcap_free_iterator(ctrl_itr);
        return VCAP_ERROR;
    }

    vcap_free_iterator(ctrl_itr);

    return VCAP_OK;
}

// NOTE: This function requires the "free" function
int vcap_enumerate_devices(uint32_t index, vcap_device_info* info)
{
    assert(info != NULL);

    if (!info)
        return VCAP_ERROR;

    struct dirent **names;
    int n = scandir("/dev", &names, vcap_video_device_filter, alphasort);

    if (n < 0)
        return VCAP_ERROR;

    char path[512];

    // Loop through all valid entries in the "/dev" directory until count == index
    uint32_t count = 0;

    for (int i = 0; i < n; i++)
    {
        snprintf(path, sizeof(path), "/dev/%s", names[i]->d_name);

        struct v4l2_capability caps;

        if (vcap_query_caps(path, &caps) == VCAP_OK)
        {
            if (index == count)
            {
                vcap_caps_to_info(path, caps, info);
                free(names);
                return VCAP_OK;
            }
            else
            {
                count++;
            }
        }
    }

    free(names);

    return VCAP_INVALID;
}

vcap_device* vcap_create_device(const char* path, bool convert, uint32_t buffer_count)
{
    assert(path != NULL);

    vcap_device* vd = (vcap_device*)vcap_malloc(sizeof(vcap_device));

    if (!vd)
        return NULL; // Out of memory

    memset(vd, 0, sizeof(vcap_device));

    vd->fd = -1;
    vd->buffer_count = buffer_count;
    vd->streaming = false;
    vd->convert = convert;

    vcap_strcpy(vd->path, path, sizeof(vd->path));

    return vd;
}

void vcap_destroy_device(vcap_device* vd)
{
    assert(vd != NULL);

    if (vcap_is_open(vd))
        vcap_close(vd);

    vcap_free(vd);
}

int vcap_open(vcap_device* vd)
{
    assert(vd != NULL);

    if (vcap_is_open(vd))
    {
        vcap_set_error(vd, "Device %s is already open", vd->path);
        return VCAP_ERROR;
    }

    struct stat st;
    struct v4l2_capability caps;

    // Device must exist
    if (stat(vd->path, &st) == -1)
    {
        vcap_set_error_errno(vd, "Device %s does not exist", vd->path);
        return VCAP_ERROR;
    }

    // Device must be a character device
    if (!S_ISCHR(st.st_mode))
    {
        vcap_set_error_errno(vd, "Device %s is not a character device", vd->path);
        return VCAP_ERROR;
    }

    // Open the video device
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/func-open.html#func-open
    vd->fd = v4l2_open(vd->path, O_RDWR | O_NONBLOCK, 0);

    if (vd->fd == -1)
    {
        vcap_set_error_errno(vd, "Opening device %s failed", vd->path);
        return VCAP_ERROR;
    }

    // Ensure child processes dont't inherit the video device
    fcntl(vd->fd, F_SETFD, FD_CLOEXEC);

    // Obtain device capabilities
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-querycap.html
    if (vcap_ioctl(vd->fd, VIDIOC_QUERYCAP, &caps) == -1)
    {
        vcap_set_error_errno(vd, "Querying device %s capabilities failed", vd->path);
        v4l2_close(vd->fd);
        return VCAP_ERROR;
    }

    // Ensure video capture is supported
    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        vcap_set_error(vd, "Device %s does not support video capture", vd->path);
        v4l2_close(vd->fd);
        return VCAP_ERROR;
    }

    // Check I/O capabilities
    if (vd->buffer_count > 0)
    {
        // Ensure streaming is supported
        if (!(caps.capabilities & V4L2_CAP_STREAMING))
        {
            vcap_set_error(vd, "Device %s does not support streaming", vd->path);
            v4l2_close(vd->fd);
            return VCAP_ERROR;
        }
    }
    else
    {
        // Ensure read is supported
        if (!(caps.capabilities & V4L2_CAP_READWRITE))
        {
            vcap_set_error(vd, "Video device %s does not support read/write", vd->path);
            v4l2_close(vd->fd);
            return VCAP_ERROR;
        }
    }

    // Enables/disables format conversion
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/libv4l-introduction.html
    if (vd->convert)
        vd->fd = v4l2_fd_open(vd->fd, 0);
    else
        vd->fd = v4l2_fd_open(vd->fd, V4L2_DISABLE_CONVERSION);

    // Copy capabilities
    vd->caps = caps;
    vd->open = true;

    return VCAP_OK;
}

void vcap_close(vcap_device* vd)
{
    assert(vd != NULL);

    if (!vcap_is_open(vd))
        return;

    // No-op if device is not streaming, ignore errors
    vcap_stop_stream(vd);

    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/func-close.html
    if (vd->fd >= 0)
        v4l2_close(vd->fd);

    vd->open = false;
}

int vcap_start_stream(vcap_device* vd)
{
    assert(vd != NULL);

    if (vd->buffer_count > 0)
    {
        if (vcap_is_streaming(vd))
        {
            vcap_set_error(vd, "Device %s is already streaming", vd->path);
            return VCAP_ERROR;
        }

        if (vcap_init_stream(vd) == VCAP_ERROR)
            return VCAP_ERROR;

        // Turn stream on
        // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-streamon.html
    	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (vcap_ioctl(vd->fd, VIDIOC_STREAMON, &type) == -1)
        {
            vcap_set_error_errno(vd, "Unable to start stream on %s", vd->path);
            return VCAP_ERROR;
        }

        vd->streaming = true;
    }

    return VCAP_OK;
}

int vcap_stop_stream(vcap_device* vd)
{
    assert(vd != NULL);

    if (vd->buffer_count > 0)
    {
        if (!vcap_is_streaming(vd))
        {
            vcap_set_error(vd, "Unable to stop stream on %s, device is not streaming", vd->path);
            return VCAP_ERROR;
        }

        // Turn stream off
        // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-streamon.html
    	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (vcap_ioctl(vd->fd, VIDIOC_STREAMOFF, &type) == -1)
        {
            vcap_set_error_errno(vd, "Unable to stop stream on %s", vd->path);
            return VCAP_ERROR;
        }

        // Disables and
        if (vcap_shutdown_stream(vd) == VCAP_ERROR)
            return VCAP_ERROR;

        vd->streaming = false;
    }

    return VCAP_OK;
}

bool vcap_is_open(vcap_device* vd)
{
    assert(vd != NULL);
    return vd->open;
}

bool vcap_is_streaming(vcap_device* vd)
{
    assert(vd != NULL);
    return vd->streaming;
}

int vcap_get_device_info(vcap_device* vd, vcap_device_info* info)
{
    assert(vd != NULL);
    assert(info != NULL);

    if (!info)
    {
        vcap_set_error(vd, "Argument can't be null");
        return VCAP_ERROR;
    }

    vcap_caps_to_info(vd->path, vd->caps, info);

    return VCAP_OK;
}

size_t vcap_get_image_size(vcap_device* vd)
{
    assert(vd != NULL);

    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-fmt.html
    struct v4l2_format fmt;
    VCAP_CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_G_FMT, &fmt) == -1)
    {
        vcap_set_error_errno(vd, "Unable to get format on device %s", vd->path);
        return 0;
    }

    return fmt.fmt.pix.sizeimage;
}

int vcap_grab(vcap_device* vd, size_t size, uint8_t* data)
{
    assert(vd != NULL);
    assert(data != NULL);

    if (!data)
    {
        vcap_set_error(vd, "Argument can't be null");
        return VCAP_ERROR;
    }

    if (vd->buffer_count > 0)
        return vcap_grab_mmap(vd, size, data);
    else
        return vcap_grab_read(vd, size, data);
}

//==============================================================================
// Iterator functions
//==============================================================================

bool vcap_iterator_error(vcap_iterator* itr)
{
    return itr->result == VCAP_ERROR;
}

void vcap_free_iterator(vcap_iterator* itr)
{
    if (!itr)
        return;

    vcap_free(itr);
}

//==============================================================================
// Format functions
//==============================================================================

int vcap_get_format_info(vcap_device* vd, vcap_format_id fmt, vcap_format_info* info)
{
    assert(vd != NULL);
    assert(info != NULL);

    if (!info)
    {
        vcap_set_error(vd, "Argument can't be null");
        return VCAP_ERROR;
    }

    // NOTE: Unfortunately there is no V4L2 function that returns information on
    // a format without enumerating them, as is done below.

    int result, index = 0;

    do
    {
        result = vcap_enum_fmts(vd, info, index);

        if (result == VCAP_ERROR)
            return VCAP_ERROR;

        if (result == VCAP_OK && info->id == fmt)
            return VCAP_OK;

    } while (result != VCAP_INVALID && ++index);

    vcap_set_error(vd, "Invalid format ID");
    return VCAP_INVALID;
}

vcap_iterator* vcap_format_iterator(vcap_device* vd)
{
    assert(vd != NULL);

    vcap_iterator* itr = (vcap_iterator*)vcap_malloc(sizeof(vcap_iterator));

    itr->type = VCAP_ITR_FMT;
    itr->vd = vd;
    itr->index = 0;
    itr->result = vcap_enum_fmts(itr->vd, &itr->data.fmt.info, ++itr->index);

    return itr;
}

bool vcap_next_format(vcap_iterator* itr, vcap_format_info* info)
{
    assert(itr != NULL);
    assert(info != NULL);

    // TODO: Check iterator type

    if (!info)
    {
        vcap_set_error(itr->vd, "Argument can't be null");
        itr->result = VCAP_ERROR;
        return false;
    }

    if (itr->result == VCAP_INVALID || itr->result == VCAP_ERROR)
        return false;

    *info = itr->data.fmt.info;

    itr->result = vcap_enum_fmts(itr->vd, &itr->data.fmt.info, ++itr->index);

    return true;
}

vcap_iterator* vcap_size_iterator(vcap_device* vd, vcap_format_id fmt)
{
    assert(vd != NULL);

    vcap_iterator* itr = (vcap_iterator*)vcap_malloc(sizeof(vcap_iterator));

    itr->type = VCAP_ITR_SIZE;
    itr->vd = vd;
    itr->index = 0;
    itr->data.size.fmt = fmt;
    itr->result = vcap_enum_sizes(itr->vd, fmt, &itr->data.size.size, ++itr->index);

    return itr;
}

bool vcap_next_size(vcap_iterator* itr, vcap_size* size)
{
    assert(itr != NULL);
    assert(size != NULL);

    // TODO: Check iterator type

    if (!size)
    {
        vcap_set_error(itr->vd, "Argument can't be null");
        itr->result = VCAP_ERROR;
        return false;
    }

    if (itr->result == VCAP_INVALID || itr->result == VCAP_ERROR)
        return false;

    *size = itr->data.size.size;

    itr->result = vcap_enum_sizes(itr->vd, itr->data.size.fmt, &itr->data.size.size, ++itr->index);

    return true;
}

vcap_iterator* vcap_rate_iterator(vcap_device* vd, vcap_format_id fmt, vcap_size size)
{
    assert(vd != NULL);

    vcap_iterator* itr = (vcap_iterator*)vcap_malloc(sizeof(vcap_iterator));

    itr->type = VCAP_ITR_RATE;
    itr->vd = vd;
    itr->index = 0;
    itr->data.rate.fmt = fmt;
    itr->data.rate.size = size;
    itr->result = vcap_enum_rates(vd, fmt, size, &itr->data.rate.rate, 0);

    return itr;
}

bool vcap_next_rate(vcap_iterator* itr, vcap_rate* rate)
{
    assert(itr != NULL);
    assert(rate != NULL);

    // TODO: Check iterator type

    if (!rate)
    {
        vcap_set_error(itr->vd, "Argument can't be null");
        itr->result = VCAP_ERROR;
        return false;
    }

    if (itr->result == VCAP_INVALID || itr->result == VCAP_ERROR)
        return false;

    *rate = itr->data.rate.rate;

    itr->result = vcap_enum_rates(itr->vd, itr->data.rate.fmt, itr->data.rate.size, &itr->data.rate.rate, ++itr->index);

    return true;
}

int vcap_get_format(vcap_device* vd, vcap_format_id* fmt, vcap_size* size)
{
    assert(vd != NULL);

    // Get format
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-fmt.html
    struct v4l2_format gfmt;
    VCAP_CLEAR(gfmt);

    gfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_G_FMT, &gfmt))
    {
        vcap_set_error_errno(vd, "Unable to get format on device %s", vd->path);
        return VCAP_ERROR;
    }

    // Get format ID, if requested
    if (fmt)
        *fmt = vcap_convert_fmt(gfmt.fmt.pix.pixelformat);

    // Get size, if requested
    if (size)
    {
        size->width  = gfmt.fmt.pix.width;
        size->height = gfmt.fmt.pix.height;
    }

    return VCAP_OK;
}

int vcap_set_format(vcap_device* vd, vcap_format_id fmt, vcap_size size)
{
    assert(vd != NULL);

    // Ensure format ID is within the proper range
    assert(fmt < VCAP_FMT_COUNT);

    if (fmt >= VCAP_FMT_COUNT)
    {
        vcap_set_error(vd, "Invalid argument (out of range)");
        return VCAP_ERROR;
    }

    bool streaming = vcap_is_streaming(vd);

    // NOTE: Some cameras return a busy signal when attempting to set a format
    // on the device. The only solution that seems work across multiple cameras
    // is closing the device and then immediately reopening it.
    vcap_close(vd);

    if (vcap_open(vd) == VCAP_ERROR)
        return VCAP_ERROR;

    // Specify desired format and set
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-fmt.html
    struct v4l2_format sfmt;
    VCAP_CLEAR(sfmt);

    sfmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    sfmt.fmt.pix.pixelformat = vcap_map_fmt(fmt);
    sfmt.fmt.pix.width       = size.width;
    sfmt.fmt.pix.height      = size.height;
    sfmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    if (vcap_ioctl(vd->fd, VIDIOC_S_FMT, &sfmt) == -1)
    {
        vcap_set_error_errno(vd, "Unable to set format on %s", vd->path);
        return VCAP_ERROR;
    }

    if (streaming && vcap_start_stream(vd) == VCAP_ERROR)
        return VCAP_ERROR;

    return VCAP_OK;
}

int vcap_get_rate(vcap_device* vd, vcap_rate* rate)
{
    assert(vd != NULL);
    assert(rate != NULL);

    if (!rate)
    {
        vcap_set_error(vd, "Argument can't be null");
        return VCAP_ERROR;
    }

    // Get frame rate
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-parm.html
    struct v4l2_streamparm parm;
    VCAP_CLEAR(parm);

    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_G_PARM, &parm) == -1)
    {
        vcap_set_error_errno(vd, "Unable to get frame rate on device %s", vd->path);
        return VCAP_ERROR;
    }

    // NOTE: We swap the numerator and denominator because Vcap uses frame rates
    // instead of intervals.
    rate->numerator   = parm.parm.capture.timeperframe.denominator;
    rate->denominator = parm.parm.capture.timeperframe.numerator;

    return VCAP_OK;
}

int vcap_set_rate(vcap_device* vd, vcap_rate rate)
{
    assert(vd != NULL);

    bool streaming = vcap_is_streaming(vd);

    if (streaming && vcap_stop_stream(vd) == VCAP_ERROR)
        return VCAP_ERROR;

    // Set frame rate
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-parm.html
    struct v4l2_streamparm parm;
    VCAP_CLEAR(parm);

    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    // NOTE: We swap the numerator and denominator because Vcap uses frame rates
    // instead of intervals.
    parm.parm.capture.timeperframe.numerator   = rate.denominator;
    parm.parm.capture.timeperframe.denominator = rate.numerator;

    if(vcap_ioctl(vd->fd, VIDIOC_S_PARM, &parm) == -1)
    {
        vcap_set_error_errno(vd, "Unable to set framerate on device %s", vd->path);
        return VCAP_ERROR;
    }

    if (streaming && vcap_start_stream(vd) == VCAP_ERROR)
        return VCAP_ERROR;

    return VCAP_OK;
}

//==============================================================================
// Control Functions
//==============================================================================

int vcap_get_control_info(vcap_device* vd, vcap_control_id ctrl, vcap_control_info* info)
{
    assert(vd != NULL);
    assert(info != NULL);

    // Ensure control ID is within the proper range
    assert(ctrl < VCAP_CTRL_COUNT);

    if (ctrl >= VCAP_CTRL_COUNT)
    {
        vcap_set_error(vd, "Invalid argument (out of range)");
        return VCAP_ERROR;
    }

    if (!info)
    {
        vcap_set_error(vd, "Argument can't be null");
        return VCAP_ERROR;
    }

    // Query specified control
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-queryctrl.html
    struct v4l2_queryctrl qctrl;
    VCAP_CLEAR(qctrl);

    qctrl.id = vcap_map_ctrl(ctrl);

    if (vcap_ioctl(vd->fd, VIDIOC_QUERYCTRL, &qctrl) == -1)
    {
        if (errno == EINVAL)
        {
            vcap_set_error(vd, "Invalid control ID");
            return VCAP_INVALID;
        }
        else
        {
            vcap_set_error_errno(vd, "Unable to read control info on device %s", vd->path);
            return VCAP_ERROR;
        }
    }

    // Test if control type is supported
    if (!vcap_ctrl_type_supported(qctrl.type))
    {
        vcap_set_error(vd, "Invalid control type");
        return VCAP_INVALID;
    }

    // Copy name
    vcap_ustrcpy(info->name, qctrl.name, sizeof(info->name));

    // Copy control ID
    info->id = vcap_convert_ctrl(qctrl.id);

    // Copy type
    info->type = vcap_convert_ctrl_type(qctrl.type);

    // Copy type string
    vcap_ustrcpy(info->type_name, (uint8_t*)vcap_ctrl_type_str(info->type), sizeof(info->type_name));

    // Min/Max/Step
    info->min  = qctrl.minimum;
    info->max  = qctrl.maximum;
    info->step = qctrl.step;

    // Default
    info->default_value = qctrl.default_value;

    return VCAP_OK;
}

int vcap_get_control_status(vcap_device* vd, vcap_control_id ctrl, vcap_control_status* status)
{
    assert(vd != NULL);
    assert(status != NULL);

    // Ensure control ID is within the proper range
    assert(ctrl < VCAP_CTRL_COUNT);

    if (ctrl >= VCAP_CTRL_COUNT)
    {
        vcap_set_error(vd, "Invalid argument (out of range)");
        return VCAP_ERROR;
    }

    if (!status)
    {
        vcap_set_error(vd, "Argument can't be null");
        return VCAP_ERROR;
    }

    // Query specified control.
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-queryctrl.html
    struct v4l2_queryctrl qctrl;
    VCAP_CLEAR(qctrl);

    qctrl.id = vcap_map_ctrl(ctrl);

    if (vcap_ioctl(vd->fd, VIDIOC_QUERYCTRL, &qctrl) == -1)
    {
        if (errno == EINVAL)
        {
            vcap_set_error(vd, "Invalid control ID");
            return VCAP_INVALID;
        }
        else
        {
            vcap_set_error_errno(vd, "Unable to check control status on device %s", vd->path);
            return VCAP_ERROR;
        }
    }

    *status = VCAP_CTRL_STATUS_OK;

    // Test if control type is supported
    if (!vcap_ctrl_type_supported(qctrl.type))
    {
        vcap_set_error(vd, "Invalid control type");
        return VCAP_INVALID;
    }

    // Test if control is inactive
    if (qctrl.flags & V4L2_CTRL_FLAG_INACTIVE)
        *status |= VCAP_CTRL_STATUS_INACTIVE;

    // Test if control is disabled
    if (qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
        *status |= VCAP_CTRL_STATUS_DISABLED;

    // Test if control is read only
    if (qctrl.flags & V4L2_CTRL_FLAG_READ_ONLY || qctrl.flags & V4L2_CTRL_FLAG_GRABBED)
        *status |= VCAP_CTRL_STATUS_READ_ONLY;

    return VCAP_OK;
}

vcap_iterator* vcap_control_iterator(vcap_device* vd)
{
    assert(vd != NULL);

    vcap_iterator* itr = (vcap_iterator*)vcap_malloc(sizeof(vcap_iterator));

    itr->type = VCAP_ITR_CTRL;
    itr->vd = vd;
    itr->index = 0;
    itr->result = vcap_enum_ctrls(vd, &itr->data.ctrl.info, 0);

    return itr;
}

bool vcap_next_control(vcap_iterator* itr, vcap_control_info* info)
{
    assert(itr != NULL);
    assert(info != NULL);

    // TODO: Check iterator type

    if (!info)
    {
        vcap_set_error(itr->vd, "Argument can't be null");
        itr->result = VCAP_ERROR;
        return false;
    }

    if (itr->result == VCAP_INVALID || itr->result == VCAP_ERROR)
        return false;

     *info = itr->data.ctrl.info;

    itr->result = vcap_enum_ctrls(itr->vd, &itr->data.ctrl.info, ++itr->index);

    return true;
}

vcap_iterator* vcap_menu_iterator(vcap_device* vd, vcap_control_id ctrl)
{
    assert(vd != NULL);

    vcap_iterator* itr = (vcap_iterator*)vcap_malloc(sizeof(vcap_iterator));

    itr->type = VCAP_ITR_MENU;
    itr->vd = vd;
    itr->index = 0;
    itr->data.menu.ctrl = ctrl;
    itr->result = vcap_enum_menu(vd, ctrl, &itr->data.menu.item, 0);

    return itr;
}

bool vcap_next_menu_item(vcap_iterator* itr, vcap_menu_item* item)
{
    assert(itr != NULL);
    assert(item != NULL);

    // TODO: Check iterator type

    if (!item)
    {
        vcap_set_error(itr->vd, "Argument can't be null");
        itr->result = VCAP_ERROR;
        return false;
    }

    if (itr->result == VCAP_INVALID || itr->result == VCAP_ERROR)
        return false;

    *item = itr->data.menu.item;

    itr->result = vcap_enum_menu(itr->vd, itr->data.menu.ctrl, &itr->data.menu.item, ++itr->index);

    return true;
}

int vcap_get_control(vcap_device* vd, vcap_control_id ctrl, int32_t* value)
{
    assert(vd != NULL);
    assert(value != NULL);

    // Ensure control ID is within the proper range
    assert(ctrl < VCAP_CTRL_COUNT);

    if (ctrl >= VCAP_CTRL_COUNT)
    {
        vcap_set_error(vd, "Invalid argument (out of range)");
        return VCAP_ERROR;
    }

    if (!value)
    {
        vcap_set_error(vd, "Argument can't be null");
        return VCAP_ERROR;
    }

    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-ctrl.html
    struct v4l2_control gctrl;
    VCAP_CLEAR(gctrl);

    gctrl.id = vcap_map_ctrl(ctrl);

    if (vcap_ioctl(vd->fd, VIDIOC_G_CTRL, &gctrl) == -1)
    {
        vcap_set_error_errno(vd, "Could not get control (%d) value on device %s", ctrl, vd->path);
        return VCAP_ERROR;
    }

    *value = gctrl.value;

    return VCAP_OK;
}

int vcap_set_control(vcap_device* vd, vcap_control_id ctrl, int32_t value)
{
    assert(vd != NULL);

    // Ensure control ID is within the proper range
    assert(ctrl < VCAP_CTRL_COUNT);

    if (ctrl >= VCAP_CTRL_COUNT)
    {
        vcap_set_error(vd, "Invalid argument (out of range)");
        return VCAP_ERROR;
    }

    // Specify control and value
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-ctrl.html
    struct v4l2_control sctrl;
    VCAP_CLEAR(sctrl);

    sctrl.id    = vcap_map_ctrl(ctrl);
    sctrl.value = value;

    // Set control
    if (vcap_ioctl(vd->fd, VIDIOC_S_CTRL, &sctrl) == -1)
    {
        vcap_set_error_errno(vd, "Could not set control (%d) value on device %s", ctrl, vd->path);
        return VCAP_ERROR;
    }

    return VCAP_OK;
}

int vcap_reset_control(vcap_device* vd, vcap_control_id ctrl)
{
    assert(vd != NULL);

    vcap_control_info info;

    int result = vcap_get_control_info(vd, ctrl, &info);

    if (result == VCAP_ERROR || result == VCAP_INVALID)
        return VCAP_ERROR;

    if (result == VCAP_OK)
    {
        if (vcap_set_control(vd, ctrl, info.default_value) == VCAP_ERROR)
            return VCAP_ERROR;
    }

    return VCAP_OK;
}

int vcap_reset_all_controls(vcap_device* vd)
{
    assert(vd != NULL);

    // Loop over all controlsa
    for (vcap_control_id ctrl = 0; ctrl < VCAP_CTRL_COUNT; ctrl++)
    {
        vcap_control_status status = 0;

        int result = vcap_get_control_status(vd, ctrl, &status);

        if (result == VCAP_ERROR)
            return VCAP_ERROR;

        if (result == VCAP_INVALID)
            continue;

        if (status != VCAP_CTRL_STATUS_OK)
            continue;

        if (vcap_reset_control(vd, ctrl) == -1)
            return VCAP_ERROR;
    }

    return VCAP_OK;
}

//==============================================================================
// Crop functions
//==============================================================================

int vcap_get_crop_bounds(vcap_device* vd, vcap_rect* rect)
{
    assert(vd != NULL);
    assert(rect != NULL);

    if (!rect)
    {
        vcap_set_error(vd, "Argument can't be null");
        return VCAP_ERROR;
    }

    // Get crop rectangle
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-cropcap.html
    struct v4l2_cropcap cropcap;
    VCAP_CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_CROPCAP, &cropcap) == -1)
    {
        if (errno == ENODATA || errno == EINVAL)
        {
            vcap_set_error(vd, "Cropping is not supported on device '%s'", vd->path);
            return VCAP_ERROR;
        }
    }

    // Copy rectangle bounds
    rect->top = cropcap.bounds.top;
    rect->left = cropcap.bounds.left;
    rect->width = cropcap.bounds.width;
    rect->height = cropcap.bounds.height;

    return VCAP_OK;
}

int vcap_reset_crop(vcap_device* vd)
{
    assert(vd != NULL);

    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-cropcap.html
    struct v4l2_cropcap cropcap;
    VCAP_CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_CROPCAP, &cropcap) == -1)
    {
        if (errno == ENODATA || errno == EINVAL)
        {
            vcap_set_error(vd, "Cropping is not supported on device '%s'", vd->path);
            return VCAP_ERROR;
        }
    }

    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-crop.html
    struct v4l2_crop crop;
    VCAP_CLEAR(crop);

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect;

    if (vcap_ioctl(vd->fd, VIDIOC_S_CROP, &crop) == -1)
    {
        vcap_set_error_errno(vd, "Unable to set crop window on device '%s'", vd->path);
        return VCAP_ERROR;
    }

    return VCAP_OK;
}

int vcap_get_crop(vcap_device* vd, vcap_rect* rect)
{
    assert(vd != NULL);
    assert(rect != NULL);

    if (!rect)
    {
        vcap_set_error(vd, "Argument can't be null");
        return VCAP_ERROR;
    }

    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-crop.html
    struct v4l2_crop crop;
    VCAP_CLEAR(crop);

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_G_CROP, &crop) == -1)
    {
        if (errno == ENODATA || errno == EINVAL)
        {
            vcap_set_error(vd, "Cropping is not supported on device %s", vd->path);
            return VCAP_ERROR;
        }
        else
        {
            vcap_set_error(vd, "Unable to get crop window on device %s", vd->path);
            return VCAP_ERROR;
        }
    }

    rect->top = crop.c.top;
    rect->left = crop.c.left;
    rect->width = crop.c.width;
    rect->height = crop.c.height;

    return VCAP_OK;
}

int vcap_set_crop(vcap_device* vd, vcap_rect rect)
{
    assert(vd != NULL);

    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-crop.html
    struct v4l2_crop crop;
    VCAP_CLEAR(crop);

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c.left = rect.left;
    crop.c.top = rect.top;
    crop.c.width = rect.width;
    crop.c.height = rect.height;

    if (vcap_ioctl(vd->fd, VIDIOC_S_CROP, &crop) == -1)
    {
        if (errno == ENODATA || errno == EINVAL)
        {
            vcap_set_error(vd, "Cropping is not supported on device %s", vd->path);
            return VCAP_ERROR;
        }
        else
        {
            vcap_set_error(vd, "Unable to set crop window on device %s", vd->path);
            return VCAP_ERROR;
        }
    }

    return VCAP_OK;
}

//==============================================================================
// Internal Functions
//==============================================================================

static void* vcap_malloc(size_t size)
{
    return global_malloc_fp(size);
}

static void vcap_free(void* ptr)
{
    global_free_fp(ptr);
}

static void vcap_fourcc_string(uint32_t code, uint8_t* str)
{
    assert(str);

    str[0] = (code >> 0) & 0xFF;
    str[1] = (code >> 8) & 0xFF;
    str[2] = (code >> 16) & 0xFF;
    str[3] = (code >> 24) & 0xFF;
    str[4] = '\0';
}

static int vcap_ioctl(int fd, long unsigned request, void *arg)
{
    assert(arg != NULL);

    int result;

    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/func-ioctl.html#func-ioctl

    do
    {
        result = v4l2_ioctl(fd, (int)request, arg);
    }
    while (result == -1 && (errno == EINTR || errno == EAGAIN));

    return result;
}

static int vcap_query_caps(const char* path, struct v4l2_capability* caps)
{
    assert(path != NULL);
    assert(caps != NULL);

    struct stat st;
    int fd = -1;

    // Device must exist
    if (stat(path, &st) == -1)
       return VCAP_ERROR;

    // Device must be a character device
    if (!S_ISCHR(st.st_mode))
        return VCAP_ERROR;

    // Open the video device
    fd = v4l2_open(path, O_RDWR | O_NONBLOCK, 0);

    if (fd == -1)
        return VCAP_ERROR;

    // Obtain device capabilities
    if (vcap_ioctl(fd, VIDIOC_QUERYCAP, caps) == -1)
    {
        v4l2_close(fd);
        return VCAP_ERROR;
    }

    // Ensure video capture is supported
    if (!(caps->capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        v4l2_close(fd);
        return VCAP_ERROR;
    }

    v4l2_close(fd);

    return VCAP_OK;
}

static int vcap_video_device_filter(const struct dirent* a)
{
    assert(a != NULL);

    if (0 == strncmp(a->d_name, "video", 5))
        return 1;
    else
        return 0;
}

static void vcap_caps_to_info(const char* path, const struct v4l2_capability caps, vcap_device_info* info)
{
    assert(path != NULL);
    assert(info != NULL);

    // Copy device information
    vcap_strcpy(info->path, path, sizeof(info->path));
    vcap_ustrcpy(info->driver, caps.driver, sizeof(info->driver));
    vcap_ustrcpy(info->card, caps.card, sizeof(info->card));
    vcap_ustrcpy(info->bus_info, caps.bus_info, sizeof(info->bus_info));
    info->version = caps.version;

    // Decode version
    snprintf((char*)info->version_str, sizeof(info->version_str), "%u.%u.%u",
            (caps.version >> 16) & 0xFF,
            (caps.version >> 8) & 0xFF,
            (caps.version & 0xFF));

    // Determines which output modes are available
    info->streaming = (bool)(caps.capabilities & V4L2_CAP_STREAMING);
    info->read = (bool)(caps.capabilities & V4L2_CAP_READWRITE);
}

static int vcap_request_buffers(vcap_device* vd)
{
    assert(vd != NULL);

    // Requests the specified number of buffers, returning the number of
    // available buffers
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-reqbufs.html
    struct v4l2_requestbuffers req;
    VCAP_CLEAR(req);

    req.count  = vd->buffer_count;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

	if (vcap_ioctl(vd->fd, VIDIOC_REQBUFS, &req) == -1)
	{
    	vcap_set_error_errno(vd, "Unable to request buffers on %s", vd->path);
		return VCAP_ERROR;
    }

    if (req.count == 0)
    {
        vcap_set_error(vd, "Invalid buffer count on %s", vd->path);
        return VCAP_ERROR;
    }

    // Changes the number of buffer in the video device to match the number of
    // available buffers
    vd->buffer_count = req.count;

    // Allocates the buffer objects
    vd->buffers = (vcap_buffer*)vcap_malloc(req.count * sizeof(vcap_buffer));

    return VCAP_OK;
}

static int vcap_init_stream(vcap_device* vd)
{
    assert(vd != NULL);

    if (vd->buffer_count > 0)
    {
        if (vcap_request_buffers(vd) == VCAP_ERROR)
            return VCAP_ERROR;

        // FIXME: don't continue if returned buffer_count == 0?

        if (vcap_map_buffers(vd) == VCAP_ERROR)
            return VCAP_ERROR;

        if (vcap_queue_buffers(vd) == VCAP_ERROR)
            return VCAP_ERROR;
    }

    return VCAP_OK;
}

static int vcap_release_buffers(vcap_device* vd)
{
    assert(vd != NULL);

    struct v4l2_requestbuffers req;
    VCAP_CLEAR(req);

    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    req.count  = 0;

	if (vcap_ioctl(vd->fd, VIDIOC_REQBUFS, &req) == -1)
	{
    	vcap_set_error_errno(vd, "Unable to request buffers on %s", vd->path);
		return VCAP_ERROR;
    }

    return VCAP_OK;
}

static int vcap_shutdown_stream(vcap_device* vd)
{
    assert(vd != NULL);

    if (vd->buffer_count > 0)
    {
        if (vcap_unmap_buffers(vd) == VCAP_ERROR)
            return VCAP_ERROR;

        if (vcap_release_buffers(vd) == VCAP_ERROR)
            return VCAP_ERROR;
    }

    return VCAP_OK;
}

static int vcap_map_buffers(vcap_device* vd)
{
    assert(vd != NULL);

     for (uint32_t i = 0; i < vd->buffer_count; i++)
     {
        // Query buffers, returning their size and other information
        // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-querybuf.html
        struct v4l2_buffer buf;
        VCAP_CLEAR(buf);

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if (vcap_ioctl(vd->fd, VIDIOC_QUERYBUF, &buf) == -1)
        {
            vcap_set_error_errno(vd, "Unable to query buffers on %s", vd->path);
            return VCAP_ERROR;
        }

        // Map buffers
        // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/func-mmap.html
        vd->buffers[i].size = buf.length;
        vd->buffers[i].data = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, vd->fd, buf.m.offset);

        if (vd->buffers[i].data == MAP_FAILED)
        {
            vcap_set_error(vd, "MMAP failed on %s", vd->path);
            return VCAP_ERROR;
        }
    }

    return VCAP_OK;
}

static int vcap_unmap_buffers(vcap_device* vd)
{
    assert(vd != NULL);

    // Unmap and free buffers
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/func-munmap.html
    for (uint32_t i = 0; i < vd->buffer_count; i++)
    {
        if (v4l2_munmap(vd->buffers[i].data, vd->buffers[i].size) == -1)
        {
            vcap_set_error_errno(vd, "Unmapping buffers failed on %s", vd->path);
            return VCAP_ERROR;
        }
    }

    vcap_free(vd->buffers);
    vd->buffer_count = 0;

    return VCAP_OK;
}

static int vcap_queue_buffers(vcap_device* vd)
{
    assert(vd != NULL);

    for (uint32_t i = 0; i < vd->buffer_count; i++)
    {
        // Places a buffer in the queue
        // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-qbuf.html
        struct v4l2_buffer buf;
        VCAP_CLEAR(buf);

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if (vcap_ioctl(vd->fd, VIDIOC_QBUF, &buf) == -1)
        {
            vcap_set_error_errno(vd, "Unable to queue buffers on device %s", vd->path);
            return VCAP_ERROR;
        }
	}

	return VCAP_OK;
}

static int vcap_grab_mmap(vcap_device* vd, size_t size, uint8_t* data)
{
    assert(vd != NULL);
    assert(data != NULL);

    if (!data)
    {
        vcap_set_error(vd, "Argument can't be null");
        return VCAP_ERROR;
    }

    if (!vcap_is_streaming(vd))
    {
        vcap_set_error(vd, "Stream on %s must be active in order to grab frame", vd->path);
        return VCAP_ERROR;
    }

    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(vd->fd, &fds);

    tv.tv_sec  = 1;
    tv.tv_usec = 0;

    struct v4l2_buffer buf;
    VCAP_CLEAR(buf);

    while (true)
    {
        int result = select(vd->fd + 1, &fds, NULL, NULL, &tv);

        if (result == -1)
        {
            if (EINTR == errno)
            {
                continue;
            }
            else
            {
                vcap_set_error_errno(vd, "Unable to read frame");
                return VCAP_ERROR;
            }
        }

        if (result == 0)
        {
            vcap_set_error(vd, "Timeout reached");
            return VCAP_ERROR;
        }

	    // Dequeue buffer
	    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-qbuf.htm

        VCAP_CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (vcap_ioctl(vd->fd, VIDIOC_DQBUF, &buf) == -1)
        {
            if (errno == EAGAIN)
            {
                continue;
            }
            else
            {
                vcap_set_error_errno(vd, "Could not dequeue buffer on %s", vd->path);
                return VCAP_ERROR;
            }
        }

        break;
    }

    // Copy buffer data
    memcpy(data, vd->buffers[buf.index].data, size);

    // Requeue buffer
	// https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-qbuf.html
    if (vcap_ioctl(vd->fd, VIDIOC_QBUF, &buf) == -1)
    {
        vcap_set_error_errno(vd, "Could not requeue buffer on %s", vd->path);
        return VCAP_ERROR;
    }

    return VCAP_OK;
}

static int vcap_grab_read(vcap_device* vd, size_t size, uint8_t* data)
{
    assert(vd != NULL);
    assert(data != NULL);

    if (!data)
    {
        vcap_set_error(vd, "Argument can't be null");
        return VCAP_ERROR;
    }

    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(vd->fd, &fds);

    tv.tv_sec  = 1;
    tv.tv_usec = 0;

    while (true)
    {
        int result = select(vd->fd + 1, &fds, NULL, NULL, &tv);

        if (result == -1)
        {
            if (EINTR == errno)
            {
                continue;
            }
            else
            {
                vcap_set_error_errno(vd, "Unable to read frame");
                return VCAP_ERROR;
            }
        }

        if (result == 0)
        {
            vcap_set_error(vd, "Timeout reached");
            return VCAP_ERROR;
        }

        if (v4l2_read(vd->fd, data, size) == -1)
        {
            if (errno == EAGAIN)
            {
                continue;
            }
            else
            {
                vcap_set_error_errno(vd, "Reading from device %s failed", vd->path);
                return VCAP_ERROR;
            }
        }

        return VCAP_OK; // Break out of loop
    }

    return VCAP_ERROR;
}

static void vcap_ustrcpy(uint8_t* dst, const uint8_t* src, size_t size)
{
    assert(dst != NULL);
    assert(src != NULL);

    snprintf((char*)dst, size, "%s", (char*)src);
}

static void vcap_strcpy(char* dst, const char* src, size_t size)
{
    assert(dst != NULL);
    assert(src != NULL);

    snprintf(dst, size, "%s", src);
}

static void vcap_set_error_str(const char* func, int line, vcap_device* vd, const char* fmt, ...)
{
    assert(vd != NULL);
    assert(fmt != NULL);

    char error_msg1[512];
    char error_msg2[512];

    snprintf(error_msg1, sizeof(error_msg1), "[%s:%d]", func, line);

    va_list args;
    va_start(args, fmt);
    vsnprintf(error_msg2, sizeof(error_msg2), fmt, args);
    va_end(args);

    snprintf(vd->error_msg, sizeof(vd->error_msg), "%s %s", error_msg1, error_msg2);
}

static void vcap_set_error_errno_str(const char* func, int line, vcap_device* vd, const char* fmt, ...)
{
    assert(vd != NULL);
    assert(fmt != NULL);

    char error_msg1[512];
    char error_msg2[512];

    snprintf(error_msg1, sizeof(error_msg1), "[%s:%d]", func, line);

    va_list args;
    va_start(args, fmt);
    vsnprintf(error_msg2, sizeof(error_msg2), fmt, args);
    va_end(args);

    snprintf(vd->error_msg, sizeof(vd->error_msg), "%s %s (%s)", error_msg1, error_msg2, strerror(errno));
}

//==============================================================================
// Enumeration Functions
//==============================================================================

static bool vcap_ctrl_type_supported(uint32_t type)
{
    switch (type)
    {
        case V4L2_CTRL_TYPE_INTEGER:
        case V4L2_CTRL_TYPE_BOOLEAN:
        case V4L2_CTRL_TYPE_MENU:
        case V4L2_CTRL_TYPE_BUTTON:
            return true;
    }

    return false;
}

static int vcap_enum_fmts(vcap_device* vd, vcap_format_info* info, uint32_t index)
{
    assert(vd != NULL);
    assert(info != NULL);

    // Enumerate formats
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-enum-fmt.html
    struct v4l2_fmtdesc fmtd;
    VCAP_CLEAR(fmtd);

    fmtd.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtd.index = index;

    if (vcap_ioctl(vd->fd, VIDIOC_ENUM_FMT, &fmtd) == -1)
    {
        if (errno == EINVAL)
        {
            return VCAP_INVALID;
        }
        else
        {
            vcap_set_error_errno(vd, "Unable to enumerate formats on device %s", vd->path);
            return VCAP_ERROR;
        }
    }

    // Copy description
    vcap_ustrcpy(info->name, fmtd.description, sizeof(info->name));

    // Convert FOURCC code
    vcap_fourcc_string(fmtd.pixelformat, info->fourcc);

    // Copy pixel format
    info->id = vcap_convert_fmt(fmtd.pixelformat);

    return VCAP_OK;
}

static int vcap_enum_sizes(vcap_device* vd, vcap_format_id fmt, vcap_size* size, uint32_t index)
{
    assert(vd != NULL);
    assert(size != NULL);

    // Ensure format ID is within the proper range
    assert(fmt < VCAP_FMT_COUNT);

    if (fmt >= VCAP_FMT_COUNT)
    {
        vcap_set_error(vd, "Invalid argument (out of range)");
        return VCAP_ERROR;
    }

    // Enumerate frame sizes
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-enum-framesizes.html
    struct v4l2_frmsizeenum fenum;
    VCAP_CLEAR(fenum);

    fenum.pixel_format = vcap_map_fmt(fmt);
    fenum.index = index;

    if (vcap_ioctl(vd->fd, VIDIOC_ENUM_FRAMESIZES, &fenum) == -1)
    {
        if (errno == EINVAL)
        {
            return VCAP_INVALID;
        } else
        {
            vcap_set_error_errno(vd, "Unable to enumerate sizes on device '%s'", vd->path);
            return VCAP_ERROR;
        }
    }

    // Only discrete sizes are supported
    if (fenum.type != V4L2_FRMSIZE_TYPE_DISCRETE)
        return VCAP_INVALID;

    size->width  = fenum.discrete.width;
    size->height = fenum.discrete.height;

    return VCAP_OK;
}

static int vcap_enum_rates(vcap_device* vd, vcap_format_id fmt, vcap_size size, vcap_rate* rate, uint32_t index)
{
    assert(vd != NULL);
    assert(rate != NULL);

    // Ensure format ID is within the proper range
    assert(fmt < VCAP_FMT_COUNT);

    if (fmt >= VCAP_FMT_COUNT)
    {
        vcap_set_error(vd, "Invalid argument (out of range)");
        return VCAP_ERROR;
    }

    // Enumerate frame rates
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-enum-frameintervals.html
    struct v4l2_frmivalenum frenum;
    VCAP_CLEAR(frenum);

    frenum.pixel_format = vcap_map_fmt(fmt);
    frenum.index  = index;
    frenum.width  = size.width;
    frenum.height = size.height;

    if (vcap_ioctl(vd->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frenum) == -1)
    {
        if (errno == EINVAL)
        {
            return VCAP_INVALID;
        }
        else
        {
            vcap_set_error_errno(vd, "Unable to enumerate frame rates on device %s", vd->path);
            return VCAP_ERROR;
        }
    }

    // Only discrete frame rates are supported
    if (frenum.type != V4L2_FRMIVAL_TYPE_DISCRETE)
        return VCAP_INVALID;

    // NOTE: We swap the numerator and denominator because Vcap uses frame rates
    // instead of intervals.
    rate->numerator   = frenum.discrete.denominator;
    rate->denominator = frenum.discrete.numerator;

    return VCAP_OK;
}

static int vcap_enum_ctrls(vcap_device* vd, vcap_control_info* info, uint32_t index)
{
    assert(vd != NULL);
    assert(info != NULL);

    uint32_t count = 0;

    // Enuemrate user controls
    for (vcap_control_id ctrl = 0; ctrl < VCAP_CTRL_COUNT; ctrl++)
    {
        int result = vcap_get_control_info(vd, ctrl, info);

        if (result == VCAP_ERROR)
            return VCAP_ERROR;

        if (result == VCAP_INVALID)
            continue;

        if (index == count)
            return VCAP_OK;
        else
            count++;
    }

    return VCAP_INVALID;
}

static int vcap_enum_menu(vcap_device* vd, vcap_control_id ctrl, vcap_menu_item* item, uint32_t index)
{
    assert(vd != NULL);
    assert(item != NULL);

    // Ensure control ID is within the proper range
    assert(ctrl < VCAP_CTRL_COUNT);

    if (ctrl >= VCAP_CTRL_COUNT)
    {
        vcap_set_error(vd, "Invalid argument (out of range)");
        return VCAP_ERROR;
    }

    // Check if supported and a menu
    vcap_control_info info;
    VCAP_CLEAR(info);

    int result = vcap_get_control_info(vd, ctrl, &info);

    if (result == VCAP_ERROR)
        return VCAP_ERROR;

    if (result == VCAP_INVALID)
    {
        vcap_set_error(vd, "Can't enumerate menu of an invalid control");
        return VCAP_ERROR;
    }

    if (info.type != VCAP_CTRL_TYPE_MENU)
    {
        vcap_set_error(vd, "Control is not a menu");
        return VCAP_ERROR;
    }

    if (index < (uint32_t)info.min || index > (uint32_t)info.max)
    {
        return VCAP_INVALID;
    }

    // Loop through all entries in the menu until count == index

    uint32_t count = 0;

    for (int32_t i = info.min; i <= info.max; i += info.step)
    {
        // Query menu
        // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-queryctrl.html
        struct v4l2_querymenu qmenu;
        VCAP_CLEAR(qmenu);

        qmenu.id    = vcap_map_ctrl(ctrl);
        qmenu.index = i;

        if (vcap_ioctl(vd->fd, VIDIOC_QUERYMENU, &qmenu) == -1)
        {
            if (errno == EINVAL)
            {
                continue;
            }
            else
            {
                vcap_set_error_errno(vd, "Unable to enumerate menu on device %s", vd->path);
                return VCAP_ERROR;
            }
        }

        if (index == count)
        {
            item->index = i;

            if (info.type == VCAP_CTRL_TYPE_MENU)
                vcap_ustrcpy(item->label.str, qmenu.name, sizeof(item->label.str));
            else
                item->label.num = qmenu.value;

            return VCAP_OK;
        }
        else
        {
            count++;
        }
    }

    return VCAP_INVALID;
}

static uint32_t ctrl_map[] = {
    V4L2_CID_BRIGHTNESS,                  // Integer
    V4L2_CID_CONTRAST,                    // Integer
    V4L2_CID_SATURATION,                  // Integer
    V4L2_CID_HUE,                         // Integer
    V4L2_CID_AUTO_WHITE_BALANCE,          // Boolean
    V4L2_CID_DO_WHITE_BALANCE,            // Button
    V4L2_CID_RED_BALANCE,                 // Integer
    V4L2_CID_BLUE_BALANCE,                // Integer
    V4L2_CID_GAMMA,                       // Integer
    V4L2_CID_EXPOSURE,                    // Integer
    V4L2_CID_AUTOGAIN,                    // Boolean
    V4L2_CID_GAIN,                        // Integer
    V4L2_CID_HFLIP,                       // Boolean
    V4L2_CID_VFLIP,                       // Boolean
    V4L2_CID_POWER_LINE_FREQUENCY,        // Enum
    V4L2_CID_HUE_AUTO,                    // Boolean
    V4L2_CID_WHITE_BALANCE_TEMPERATURE,   // Integer
    V4L2_CID_SHARPNESS,                   // Integer
    V4L2_CID_BACKLIGHT_COMPENSATION,      // Integer
    V4L2_CID_CHROMA_AGC,                  // Boolean
    V4L2_CID_CHROMA_GAIN,                 // Integer
    V4L2_CID_COLOR_KILLER,                // Boolean
    V4L2_CID_AUTOBRIGHTNESS,              // Boolean
    V4L2_CID_ROTATE,                      // Integer
    V4L2_CID_BG_COLOR,                    // Integer
    V4L2_CID_ILLUMINATORS_1,              // Boolean
    V4L2_CID_ILLUMINATORS_2,              // Boolean
    V4L2_CID_ALPHA_COMPONENT,             // Integer
    V4L2_CID_EXPOSURE_AUTO,               // Enum
    V4L2_CID_EXPOSURE_ABSOLUTE,           // Integer
    V4L2_CID_EXPOSURE_AUTO_PRIORITY,      // Boolean
    V4L2_CID_AUTO_EXPOSURE_BIAS,          // Integer Menu
    V4L2_CID_EXPOSURE_METERING,           // Enum
    V4L2_CID_PAN_RELATIVE,                // Integer
    V4L2_CID_TILT_RELATIVE,               // Integer
    V4L2_CID_PAN_RESET,                   // Button
    V4L2_CID_TILT_RESET,                  // Button
    V4L2_CID_PAN_ABSOLUTE,                // Integer
    V4L2_CID_TILT_ABSOLUTE,               // Integer
    V4L2_CID_FOCUS_ABSOLUTE,              // Integer
    V4L2_CID_FOCUS_RELATIVE,              // Integer
    V4L2_CID_FOCUS_AUTO,                  // Boolean
    V4L2_CID_AUTO_FOCUS_START,            // Button
    V4L2_CID_AUTO_FOCUS_STOP,             // Button
    V4L2_CID_AUTO_FOCUS_RANGE,            // Enum
    V4L2_CID_ZOOM_ABSOLUTE,               // Integer
    V4L2_CID_ZOOM_RELATIVE,               // Integer
    V4L2_CID_ZOOM_CONTINUOUS,             // Integer
    V4L2_CID_IRIS_ABSOLUTE,               // Integer
    V4L2_CID_IRIS_RELATIVE,               // Integer
    V4L2_CID_BAND_STOP_FILTER,            // Integer
    V4L2_CID_WIDE_DYNAMIC_RANGE,          // Boolean
    V4L2_CID_IMAGE_STABILIZATION,         // Boolean
    V4L2_CID_PAN_SPEED,                   // Integer
    V4L2_CID_TILT_SPEED                   // Integer
};

static uint32_t ctrl_type_map[] = {
    V4L2_CTRL_TYPE_INTEGER,
    V4L2_CTRL_TYPE_BOOLEAN,
    V4L2_CTRL_TYPE_MENU,
    V4L2_CTRL_TYPE_BUTTON
};

static const char* ctrl_type_str_map[] = {
    "Integer",
    "Boolean",
    "Menu",
    "Integer Menu",
    "Button",
    "Unknown"
};

static vcap_control_id vcap_convert_ctrl(uint32_t id)
{
    for (size_t i = 0; i < VCAP_CTRL_COUNT; i++)
    {
        if (ctrl_map[i] == id)
            return (vcap_control_id)i;
    }

    return VCAP_CTRL_UNKNOWN;
}

static uint32_t vcap_map_ctrl(vcap_control_id id)
{
    return ctrl_map[id];
}

static vcap_control_type vcap_convert_ctrl_type(uint32_t type)
{
    for (size_t i = 0; i < VCAP_CTRL_TYPE_UNKNOWN; i++)
    {
        if (ctrl_type_map[i] == type)
            return (vcap_control_type)i;
    }

    return VCAP_CTRL_TYPE_UNKNOWN;
}

static const char* vcap_ctrl_type_str(vcap_control_type id)
{
    return ctrl_type_str_map[id];
}

static uint32_t fmt_map[] = {
    /* RGB formats */
    V4L2_PIX_FMT_BGR24,
    V4L2_PIX_FMT_RGB24,
    /* 8-Greyscale */
    V4L2_PIX_FMT_GREY,
    /* Luminance+Chrominance formats */
    V4L2_PIX_FMT_YUYV,
    V4L2_PIX_FMT_YVYU,
    V4L2_PIX_FMT_UYVY,
    V4L2_PIX_FMT_HM12,
    /* three planes - Y Cb,  Cr */
    V4L2_PIX_FMT_YUV420,
    V4L2_PIX_FMT_YVU420,
    /* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
    V4L2_PIX_FMT_SBGGR8,
    V4L2_PIX_FMT_SGBRG8,
    V4L2_PIX_FMT_SGRBG8,
    V4L2_PIX_FMT_SRGGB8,
    /* compressed formats */
    V4L2_PIX_FMT_MJPEG,
    V4L2_PIX_FMT_JPEG,
    /*  Vendor-specific formats */
    V4L2_PIX_FMT_SN9C10X,
    V4L2_PIX_FMT_SN9C20X_I420,
    V4L2_PIX_FMT_SPCA501,
    V4L2_PIX_FMT_SPCA505,
    V4L2_PIX_FMT_SPCA508,
    V4L2_PIX_FMT_SPCA561,
    V4L2_PIX_FMT_PAC207,
    V4L2_PIX_FMT_OV511,
    V4L2_PIX_FMT_OV518,
    V4L2_PIX_FMT_MR97310A,
    V4L2_PIX_FMT_SQ905C,
    V4L2_PIX_FMT_PJPG,
};

static vcap_format_id vcap_convert_fmt(uint32_t id)
{
    for (size_t i = 0; i < VCAP_FMT_COUNT; i++)
    {
        if (fmt_map[i] == id)
            return (vcap_format_id)i;
    }

    return VCAP_FMT_UNKNOWN;
}

static uint32_t vcap_map_fmt(vcap_format_id id)
{
    return fmt_map[id];
}
