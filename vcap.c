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

// Clear data structure
#define VCAP_CLEAR(arg) memset(&(arg), 0, sizeof(arg))

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
struct vcap_dev
{
    int fd;
    char path[512];
    char error_msg[2048];
    bool open;
    bool streaming;
    bool convert;
    unsigned buffer_count;
    vcap_buffer* buffers;
    struct v4l2_capability caps;
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
static int vcap_ioctl(int fd, int request, void *arg);

// Query device capabilities, used in device enumeration
static int vcap_query_caps(const char* path, struct v4l2_capability* caps);

// Filters device list so that 'scandir' returns only video devices.
static int vcap_video_device_filter(const struct dirent *a);

// Convert device capabilities into device info struct
static void vcap_caps_to_info(const char* path, const struct v4l2_capability caps, vcap_dev_info* info);

// Request a number of buffers for streaming
static int vcap_request_buffers(vcap_dev* vd, int buffer_count);

// Initialize streaming for the given device
static int vcap_init_stream(vcap_dev* vd);

// Shutdown stream for given device
static int vcap_shutdown_stream(vcap_dev* vd);

// Map memory buffers
static int vcap_map_buffers(vcap_dev* vd);

// Unmap memory buffers
static int vcap_unmap_buffers(vcap_dev* vd);

// Queue mapped buffers
static int vcap_queue_buffers(vcap_dev* vd);

// Grab a frame using memory-mapped buffers
static int vcap_grab_mmap(vcap_dev* vd, size_t size, uint8_t* data);

// Grab a frame using a direct read call
static int vcap_grab_read(vcap_dev* vd, size_t size, uint8_t* data);

// Safe unsigned string copy
static void vcap_ustrcpy(uint8_t* dst, const uint8_t* src, size_t size);

// Safe regular string copy
static void vcap_strcpy(char* dst, const char* src, size_t size);

// Set error message for specified device
static void vcap_set_error(vcap_dev* vd, const char* fmt, ...);

// Set error message (including errno infomation) for specified device
static void vcap_set_error_errno(vcap_dev* vd, const char* fmt, ...);

// Returns true if control type is supported
static bool vcap_type_supported(uint32_t type);

// Return string describing a control type
static const char* vcap_type_str(vcap_ctrl_type type);

// Enumerates formats
static int vcap_enum_fmts(vcap_dev* vd, vcap_fmt_info* info, uint32_t index);

// Enumerates frame sizes for the given format
static int vcap_enum_sizes(vcap_dev* vd, vcap_fmt_id fmt, vcap_size* size, uint32_t index);

// Enumerates frame rates for the given format and frame size
static int vcap_enum_rates(vcap_dev* vd, vcap_fmt_id fmt, vcap_size size, vcap_rate* rate, uint32_t index);

// Enumerates camera controls
static int vcap_enum_ctrls(vcap_dev* vd, vcap_ctrl_info* info, uint32_t index);

// Enumerates a menu for the given control
static int vcap_enum_menu(vcap_dev* vd, vcap_ctrl_id ctrl, vcap_menu_item* item, uint32_t index);

// Global malloc function pointer
static vcap_malloc_fn global_malloc_fp = malloc;

// Global free function pointer
static vcap_free_fn global_free_fp = free;

//==============================================================================
// Public API implementation
//==============================================================================

void vcap_set_alloc(vcap_malloc_fn malloc_fp, vcap_free_fn free_fp)
{
    global_malloc_fp = malloc_fp;
    global_free_fp = free_fp;
}

const char* vcap_get_error(vcap_dev* vd)
{
    assert(vd);
    return vd->error_msg;
}

//
// Prints device information. The implementation of this function is very
// pedantic in terms of error checking. Every error condition is checked and
// reported. A user application may choose to ignore some error cases, trading
// a little robustness for some convenience.
//
int vcap_dump_info(vcap_dev* vd, FILE* file)
{
    assert(vd);

    vcap_dev_info info = { 0 };
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

    fprintf(file, "Stream: ");

    if (info.stream)
        fprintf(file, "Supported\n");
    else
        fprintf(file, "Not supported\n");

    fprintf(file, "Read: ");

    if (info.read)
        fprintf(file, "Supported\n");
    else
        fprintf(file, "Not supported\n");

    //==========================================================================
    // Enumerate formats
    //==========================================================================
    vcap_fmt_info fmt_info;
    vcap_fmt_itr fmt_itr = vcap_new_fmt_itr(vd);

    while (vcap_fmt_itr_next(&fmt_itr, &fmt_info))
    {
        fprintf(file, "------------------------------------------------\n");
        fprintf(file, "Format: %s, FourCC: %s\n", fmt_info.name, fmt_info.fourcc);
        fprintf(file, "Sizes:\n");

        //======================================================================
        // Enumerate sizes
        //======================================================================
        vcap_size size;
        vcap_size_itr size_itr = vcap_new_size_itr(vd, fmt_info.id);

        while (vcap_size_itr_next(&size_itr, &size))
        {
            fprintf(file, "   %u x %u: ", size.width, size.height);
            fprintf(file, "(Frame rates:");

            //==================================================================
            // Enumerate frame rates
            //==================================================================
            vcap_rate rate;
            vcap_rate_itr rate_itr = vcap_new_rate_itr(vd, fmt_info.id, size);

            while (vcap_rate_itr_next(&rate_itr, &rate))
            {
                fprintf(file, " %u/%u", rate.numerator, rate.denominator);
            }

            if (vcap_itr_error(&rate_itr))
                return -1;

            fprintf(file, ")\n");
        }

        // Check for errors during frame size iteration
        if (vcap_itr_error(&size_itr))
            return -1;
    }

    // Check for errors during format iteration
    if (vcap_itr_error(&fmt_itr))
        return -1;

    //==========================================================================
    // Enumerate controls
    //==========================================================================
    fprintf(file, "------------------------------------------------\n");
    fprintf(file, "Controls:\n");

    vcap_ctrl_info ctrl_info;
    vcap_ctrl_itr ctrl_itr = vcap_new_ctrl_itr(vd);

    while (vcap_ctrl_itr_next(&ctrl_itr, &ctrl_info))
    {
        printf("   Name: %s, Type: %s\n", ctrl_info.name, ctrl_info.type_name);

        if (ctrl_info.type == V4L2_CTRL_TYPE_MENU || ctrl_info.type == V4L2_CTRL_TYPE_INTEGER_MENU)
        {
            printf("   Menu:\n");

            //==================================================================
            // Enumerate menu
            //==================================================================
            vcap_menu_item menu_item;
            vcap_menu_itr menu_itr = vcap_new_menu_itr(vd, ctrl_info.id);

            while (vcap_menu_itr_next(&menu_itr, &menu_item))
            {
                if (ctrl_info.type == V4L2_CTRL_TYPE_MENU)
                    printf("      %i : %s\n", menu_item.index, menu_item.name);
                else
                    printf("      %i : %li\n", menu_item.index, menu_item.value);
            }

            // Check for errors during menu iteration
            if (vcap_itr_error(&menu_itr))
                return -1;
        }
    }

    // Check for errors during control iteration
    if (vcap_itr_error(&ctrl_itr))
        return -1;

    return 0;
}

// NOTE: This function requires the "free" function
int vcap_enum_devices(unsigned index, vcap_dev_info* info)
{
    assert(info);

    if (!info)
        return VCAP_ENUM_ERROR;

    struct dirent **names;
    int n = scandir("/dev", &names, vcap_video_device_filter, alphasort);

    if (n < 0)
        return VCAP_ENUM_ERROR;

    char path[512];

    // Loop through all valid entries in the "/dev" directory until count == index
    int count = 0;

    for (int i = 0; i < n; i++)
    {
        snprintf(path, sizeof(path), "/dev/%s", names[i]->d_name);

        struct v4l2_capability caps;

        if (vcap_query_caps(path, &caps) == 0)
        {
            if (index == count)
            {
                vcap_caps_to_info(path, caps, info);
                free(names);
                return VCAP_ENUM_OK;
            }
            else
            {
                count++;
            }
        }
    }

    free(names);

    return VCAP_ENUM_INVALID;
}

vcap_dev* vcap_create_device(const char* path, bool convert, unsigned buffer_count)
{
    assert(path);

    vcap_dev* vd = vcap_malloc(sizeof(vcap_dev));
    memset(vd, 0, sizeof(vcap_dev));

    vd->fd = -1;
    vd->buffer_count = buffer_count;
    vd->streaming = false;
    vd->convert = convert;

    vcap_strcpy(vd->path, path, sizeof(vd->path));

    return vd;
}

void vcap_destroy_device(vcap_dev* vd)
{
    assert(vd);

    if (vcap_is_open(vd))
        vcap_close(vd);

    vcap_free(vd);
}

int vcap_open(vcap_dev* vd)
{
    assert(vd);

    if (vcap_is_open(vd))
    {
        vcap_set_error(vd, "Device %s is already open", vd->path);
        return -1;
    }

    struct stat st;
    struct v4l2_capability caps;

    // Device must exist
    if (stat(vd->path, &st) == -1)
    {
        vcap_set_error_errno(vd, "Device %s does not exist", vd->path);
        return -1;
    }

    // Device must be a character device
    if (!S_ISCHR(st.st_mode))
    {
        vcap_set_error_errno(vd, "Device %s is not a character device", vd->path);
        return -1;
    }

    // Open the video device
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/func-open.html#func-open
    vd->fd = v4l2_open(vd->path, O_RDWR | O_NONBLOCK, 0);

    if (vd->fd == -1)
    {
        vcap_set_error_errno(vd, "Opening device %s failed", vd->path);
        return -1;
    }

    // Ensure child processes dont't inherit the video device
    fcntl(vd->fd, F_SETFD, FD_CLOEXEC);

    // Obtain device capabilities
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-querycap.html
    if (vcap_ioctl(vd->fd, VIDIOC_QUERYCAP, &caps) == -1)
    {
        vcap_set_error_errno(vd, "Querying device %s capabilities failed", vd->path);
        v4l2_close(vd->fd);
        return -1;
    }

    // Ensure video capture is supported
    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        vcap_set_error(vd, "Device %s does not support video capture", vd->path);
        v4l2_close(vd->fd);
        return -1;
    }

    // Check I/O capabilities
    if (vd->buffer_count > 0)
    {
        // Ensure streaming is supported
        if (!(caps.capabilities & V4L2_CAP_STREAMING))
        {
            vcap_set_error(vd, "Device %s does not support streaming", vd->path);
            v4l2_close(vd->fd);
            return -1;
        }
    }
    else
    {
        // Ensure read is supported
        if (!(caps.capabilities & V4L2_CAP_READWRITE))
        {
            vcap_set_error(vd, "Video device %s does not support read/write", vd->path);
            v4l2_close(vd->fd);
            return -1;
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

    return 0;
}

int vcap_close(vcap_dev* vd)
{
    assert(vd);

    if (!vcap_is_open(vd))
    {
        vcap_set_error(vd, "Unable to close %s, device is not open", vd->path);
        return -1;
    }

    if (vd->buffer_count > 0)
    {
        if (vcap_is_streaming(vd) && vcap_stop_stream(vd) == -1) //TODO: close device anyway?
            return -1;
    }

    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/func-close.html
    if (vd->fd >= 0)
        v4l2_close(vd->fd);

    vd->open = false;

    return 0;
}

int vcap_start_stream(vcap_dev* vd)
{
    assert(vd);

    if (vcap_is_streaming(vd))
    {
        vcap_set_error(vd, "Device %s is already streaming", vd->path);
        return -1;
    }

    if (vd->buffer_count > 0)
    {
        if (vcap_init_stream(vd) == -1)
            return -1;

        // Turn stream on
        // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-streamon.html
    	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (vcap_ioctl(vd->fd, VIDIOC_STREAMON, &type) == -1)
        {
            vcap_set_error_errno(vd, "Unable to start stream on %s", vd->path);
            return -1;
        }

        vd->streaming = true;
    }

    return 0;
}

int vcap_stop_stream(vcap_dev* vd)
{
    assert(vd);

    if (!vcap_is_streaming(vd))
    {
        vcap_set_error(vd, "Unable to stop stream on %s, device is not streaming", vd->path);
        return -1;
    }

    if (vd->buffer_count > 0)
    {
        // Turn stream off
        // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-streamon.html
    	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (vcap_ioctl(vd->fd, VIDIOC_STREAMOFF, &type) == -1)
        {
            vcap_set_error_errno(vd, "Unable to stop stream on %s", vd->path);
            return -1;
        }

        // Disables and
        if (vcap_shutdown_stream(vd) == -1)
            return -1;

        vd->streaming = false;
    }

    return 0;
}

bool vcap_is_open(vcap_dev* vd)
{
    assert(vd);
    return vd->open;
}

bool vcap_is_streaming(vcap_dev* vd)
{
    assert(vd);
    return vd->streaming;
}

int vcap_get_device_info(vcap_dev* vd, vcap_dev_info* info)
{
    assert(vd);
    assert(info);

    if (!info)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

    vcap_caps_to_info(vd->path, vd->caps, info);

    return 0;
}

size_t vcap_get_image_size(vcap_dev* vd)
{
    assert(vd);

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

int vcap_grab(vcap_dev* vd, size_t size, uint8_t* data)
{
    assert(vd);
    assert(buffer);

    if (!data)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

    if (vd->buffer_count > 0)
        return vcap_grab_mmap(vd, size, data);
    else
        return vcap_grab_read(vd, size, data);
}

//==============================================================================
// Format functions
//==============================================================================

int vcap_get_fmt_info(vcap_dev* vd, vcap_fmt_id fmt, vcap_fmt_info* info)
{
    assert(vd);
    assert(info);

    if (!info)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return VCAP_FMT_ERROR;
    }

    // NOTE: Unfortunately there is no V4L2 function that returns information on
    // a format without enumerating them, as is done below.

    int result, i = 0;

    do
    {
        result = vcap_enum_fmts(vd, info, i);

        if (result == VCAP_ENUM_ERROR)
            return VCAP_FMT_ERROR;

        if (result == VCAP_ENUM_OK && info->id == fmt)
            return VCAP_FMT_OK;

    } while (result != VCAP_ENUM_INVALID && ++i);

    return VCAP_FMT_INVALID;
}

vcap_fmt_itr vcap_new_fmt_itr(vcap_dev* vd)
{
    assert(vd);

    vcap_fmt_itr itr = { 0 };
    itr.vd = vd;
    itr.index = 0;

    if (!vd)
    {
        vcap_set_error(vd, "Parameter can't be null");
        itr.result = VCAP_ENUM_ERROR;
    }
    else
    {
        itr.result = vcap_enum_fmts(vd, &itr.info, 0);
    }

    return itr;
}

bool vcap_fmt_itr_next(vcap_fmt_itr* itr, vcap_fmt_info* info)
{
    assert(itr);
    assert(info);

    if (!info)
    {
        vcap_set_error(itr->vd, "Parameter can't be null");
        itr->result = VCAP_ENUM_ERROR;
        return false;
    }

    if (itr->result == VCAP_ENUM_INVALID || itr->result == VCAP_ENUM_ERROR)
        return false;

    *info = itr->info;

    itr->result = vcap_enum_fmts(itr->vd, &itr->info, ++itr->index);

    return true;
}

vcap_size_itr vcap_new_size_itr(vcap_dev* vd, vcap_fmt_id fmt)
{
    assert(vd);

    vcap_size_itr itr;
    itr.vd = vd;
    itr.fmt = fmt;
    itr.index = 0;
    itr.result = vcap_enum_sizes(vd, fmt, &itr.size, 0);

    return itr;
}

bool vcap_size_itr_next(vcap_size_itr* itr, vcap_size* size)
{
    assert(itr);
    assert(size);

    if (!size)
    {
        vcap_set_error(itr->vd, "Parameter can't be null");
        itr->result = VCAP_ENUM_ERROR;
        return false;
    }

    if (itr->result == VCAP_ENUM_INVALID || itr->result == VCAP_ENUM_ERROR)
        return false;

    *size = itr->size;

    itr->result = vcap_enum_sizes(itr->vd, itr->fmt, &itr->size, ++itr->index);

    return true;
}

vcap_rate_itr vcap_new_rate_itr(vcap_dev* vd, vcap_fmt_id fmt, vcap_size size)
{
    assert(vd);

    vcap_rate_itr itr;
    itr.vd = vd;
    itr.fmt = fmt;
    itr.size = size;
    itr.index = 0;
    itr.result = vcap_enum_rates(vd, fmt, size, &itr.rate, 0);

    return itr;
}

bool vcap_rate_itr_next(vcap_rate_itr* itr, vcap_rate* rate)
{
    assert(itr);
    assert(rate);

    if (!rate)
    {
        vcap_set_error(itr->vd, "Parameter can't be null");
        itr->result = VCAP_ENUM_ERROR;
        return false;
    }

    if (itr->result == VCAP_ENUM_INVALID || itr->result == VCAP_ENUM_ERROR)
        return false;

    *rate = itr->rate;

    itr->result = vcap_enum_rates(itr->vd, itr->fmt, itr->size, &itr->rate, ++itr->index);

    return true;
}

int vcap_get_fmt(vcap_dev* vd, vcap_fmt_id* fmt, vcap_size* size)
{
    assert(vd);
    assert(size);

    if (!fmt || !size)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

    // Get format
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-fmt.html
    struct v4l2_format gfmt;

    VCAP_CLEAR(gfmt);
    gfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_G_FMT, &gfmt))
    {
        vcap_set_error_errno(vd, "Unable to get format on device %s", vd->path);
        return -1;
    }

    // Get format ID, if requested
    if (fmt)
        *fmt = gfmt.fmt.pix.pixelformat;

    // Get size, if requested
    if (size)
    {
        size->width = gfmt.fmt.pix.width;
        size->height = gfmt.fmt.pix.height;
    }

    return 0;
}

int vcap_set_fmt(vcap_dev* vd, vcap_fmt_id fmt, vcap_size size)
{
    assert(vd);

    // NOTE: Some cameras return a device busy signal when attempting to set
    // the format on a device that has already been streaming. The only viable
    // solution is to close the camera, open it, set the format, and restart the
    // stream (if applicable).

    bool streaming = vcap_is_streaming(vd);

    if (vcap_is_open(vd) && vcap_close(vd) == -1)
        return -1;

    if (vcap_open(vd) == -1)
        return -1;

    // Specify desired format
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-fmt.html
    struct v4l2_format sfmt;

    sfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    sfmt.fmt.pix.width = size.width;
    sfmt.fmt.pix.height = size.height;
    sfmt.fmt.pix.pixelformat = fmt;
    sfmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    // Set format
    if (vcap_ioctl(vd->fd, VIDIOC_S_FMT, &sfmt) == -1)
    {
        vcap_set_error_errno(vd, "Unable to set format on %s", vd->path);
        return -1;
    }

    if (streaming && vcap_start_stream(vd) == -1)
        return -1;

    return 0;
}

int vcap_get_rate(vcap_dev* vd, vcap_rate* rate)
{
    assert(vd);
    assert(rate);

    if (!rate)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

    // Get frame rate
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-parm.html
    struct v4l2_streamparm parm;
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_G_PARM, &parm) == -1)
    {
        vcap_set_error_errno(vd, "Unable to get frame rate on device %s", vd->path);
        return -1;
    }

    // NOTE: We swap the numerator and denominator because Vcap uses frame rates
    // instead of intervals.
    rate->numerator = parm.parm.capture.timeperframe.denominator;
    rate->denominator = parm.parm.capture.timeperframe.numerator;

    return 0;
}

int vcap_set_rate(vcap_dev* vd, vcap_rate rate)
{
    assert(vd);

    // Set frame rate
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-parm.html
    struct v4l2_streamparm parm;
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    // NOTE: We swap the numerator and denominator because Vcap uses frame rates
    // instead of intervals.
    parm.parm.capture.timeperframe.numerator = rate.denominator;
    parm.parm.capture.timeperframe.denominator = rate.numerator;

    if(vcap_ioctl(vd->fd, VIDIOC_S_PARM, &parm) == -1)
    {
        vcap_set_error_errno(vd, "Unable to set framerate on device %s", vd->path);
        return -1;
    }

    return 0;
}

//==============================================================================
// Control Functions
//==============================================================================

int vcap_get_ctrl_info(vcap_dev* vd, vcap_ctrl_id ctrl, vcap_ctrl_info* info)
{
    assert(vd);
    assert(info);

    if (!info)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return VCAP_CTRL_ERROR;
    }

    // Query specified control
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-queryctrl.html
    struct v4l2_queryctrl qctrl;

    VCAP_CLEAR(qctrl);
    qctrl.id = ctrl;

    if (vcap_ioctl(vd->fd, VIDIOC_QUERYCTRL, &qctrl) == -1)
    {
        if (errno == EINVAL)
        {
            return VCAP_CTRL_INVALID;
        }
        else
        {
            vcap_set_error_errno(vd, "Unable to read control descriptor on device %s", vd->path);
            return VCAP_CTRL_ERROR;
        }
    }

    // Test if control type is supported
    if (!vcap_type_supported(qctrl.type))
        return VCAP_CTRL_INVALID;

    // Copy name
    vcap_ustrcpy(info->name, qctrl.name, sizeof(info->name));

    // Copy control ID
    info->id = qctrl.id;

    // Copy type
    info->type = qctrl.type;

    // Copy type string
    vcap_ustrcpy(info->type_name, (uint8_t*)vcap_type_str(info->type), sizeof(info->type_name));

    // Min/Max/Step/Default
    info->min = qctrl.minimum;
    info->max = qctrl.maximum;
    info->step = qctrl.step;
    info->default_value = qctrl.default_value;

    // Read-only flag
    if (qctrl.flags & V4L2_CTRL_FLAG_READ_ONLY)
        info->read_only = true;
    else
        info->read_only = false;

    return VCAP_CTRL_OK;
}

int vcap_ctrl_status(vcap_dev* vd, vcap_ctrl_id ctrl)
{
    assert(vd);

    // Query specified control.
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-queryctrl.html
    struct v4l2_queryctrl qctrl;

    VCAP_CLEAR(qctrl);
    qctrl.id = ctrl;

    if (vcap_ioctl(vd->fd, VIDIOC_QUERYCTRL, &qctrl) == -1)
    {
        if (errno == EINVAL)
        {
            return VCAP_CTRL_INVALID;
        }
        else
        {
            vcap_set_error_errno(vd, "Unable to check control status on device %s", vd->path);
            return VCAP_CTRL_ERROR;
        }
    }

    // Test if control type is supported
    if (!vcap_type_supported(qctrl.type))
        return VCAP_CTRL_INVALID;

    // Test if control is read only
    if (qctrl.flags & V4L2_CTRL_FLAG_READ_ONLY || qctrl.flags & V4L2_CTRL_FLAG_GRABBED)
        return VCAP_CTRL_READ_ONLY;

    // Test if control is disabled
    if (qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
        return VCAP_CTRL_DISABLED;

    // Test if control is inactive
    if (qctrl.flags & V4L2_CTRL_FLAG_INACTIVE)
        return VCAP_CTRL_INACTIVE;

    return VCAP_CTRL_OK;
}

vcap_ctrl_itr vcap_new_ctrl_itr(vcap_dev* vd)
{
    assert(vd);

    vcap_ctrl_itr itr = { 0 };
    itr.vd = vd;
    itr.index = 0;
    itr.result = vcap_enum_ctrls(vd, &itr.info, 0);

    return itr;
}

bool vcap_ctrl_itr_next(vcap_ctrl_itr* itr, vcap_ctrl_info* info)
{
    assert(itr);
    assert(info);

    if (!info)
    {
        vcap_set_error(itr->vd, "Parameter can't be null");
        itr->result = VCAP_ENUM_ERROR;
        return false;
    }

    if (itr->result == VCAP_ENUM_INVALID || itr->result == VCAP_ENUM_ERROR)
        return false;

    *info = itr->info;

    itr->result = vcap_enum_ctrls(itr->vd, &itr->info, ++itr->index);

    return true;
}

vcap_menu_itr vcap_new_menu_itr(vcap_dev* vd, vcap_ctrl_id ctrl)
{
    assert(vd);

    vcap_menu_itr itr = { 0 };
    itr.vd = vd;
    itr.ctrl = ctrl;
    itr.index = 0;
    itr.result = vcap_enum_menu(vd, ctrl, &itr.item, 0);

    return itr;
}

bool vcap_menu_itr_next(vcap_menu_itr* itr, vcap_menu_item* item)
{
    assert(itr);
    assert(item);

    if (!item)
    {
        vcap_set_error(itr->vd, "Parameter can't be null");
        itr->result = VCAP_ENUM_ERROR;
        return false;
    }

    if (itr->result == VCAP_ENUM_INVALID || itr->result == VCAP_ENUM_ERROR)
        return false;

    *item = itr->item;

    itr->result = vcap_enum_menu(itr->vd, itr->ctrl, &itr->item, ++itr->index);

    return true;
}

int vcap_get_ctrl(vcap_dev* vd, vcap_ctrl_id ctrl, int32_t* value)
{
    assert(vd);
    assert(value);

    if (!value)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-ctrl.html
    struct v4l2_control gctrl;

    VCAP_CLEAR(gctrl);
    gctrl.id = ctrl;

    if (vcap_ioctl(vd->fd, VIDIOC_G_CTRL, &gctrl) == -1)
    {
        vcap_set_error_errno(vd, "Could not get control (%d) value on device %s", ctrl, vd->path);
        return -1;
    }

    *value = gctrl.value;

    return 0;
}

int vcap_set_ctrl(vcap_dev* vd, vcap_ctrl_id ctrl, int32_t value)
{
    assert(vd);

    // Specify control and value
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-g-ctrl.html
    struct v4l2_control sctrl;

    VCAP_CLEAR(sctrl);
    sctrl.id = ctrl;
    sctrl.value = value;

    // Set control
    if (vcap_ioctl(vd->fd, VIDIOC_S_CTRL, &sctrl) == -1)
    {
        vcap_set_error_errno(vd, "Could not set control (%d) value on device %s", ctrl, vd->path);
        return -1;
    }

    return 0;
}

int vcap_reset_ctrl(vcap_dev* vd, vcap_ctrl_id ctrl)
{
    assert(vd);

    vcap_ctrl_info info;

    int result = vcap_get_ctrl_info(vd, ctrl, &info);

    if (result == VCAP_CTRL_ERROR)
        return -1;

    if (result == VCAP_CTRL_INVALID)
    {
        vcap_set_error(vd, "Invalid control");
        return -1;
    }

    if (result == VCAP_CTRL_OK)
    {
        if (vcap_set_ctrl(vd, ctrl, info.default_value) == -1)
            return -1;
    }

    //TODO: is this the proper code for disabled/inactive controls?
    return 0;
}

// Last camera control ID plus one
#define VCAP_CID_CAMERA_CLASS_LASTP1 (V4L2_CID_CAMERA_CLASS_BASE+36)

int vcap_reset_all_ctrls(vcap_dev* vd)
{
    assert(vd);

    // Loop over user class controlsa
    for (vcap_ctrl_id ctrl = V4L2_CID_BASE; ctrl < V4L2_CID_LASTP1; ctrl++)
    {
        if (vcap_ctrl_status(vd, ctrl) != VCAP_CTRL_OK)
            continue;

        if (vcap_reset_ctrl(vd, ctrl) == -1)
            return -1;
    }

    // Loop over camera controls
    for (vcap_ctrl_id ctrl = V4L2_CID_CAMERA_CLASS_BASE; ctrl < VCAP_CID_CAMERA_CLASS_LASTP1; ctrl++)
    {
        if (vcap_ctrl_status(vd, ctrl) != VCAP_CTRL_OK)
            continue;

        if (vcap_reset_ctrl(vd, ctrl) == -1)
            return -1;
    }

    return 0;
}

//==============================================================================
// Crop functions
//==============================================================================

int vcap_get_crop_bounds(vcap_dev* vd, vcap_rect* rect)
{
    assert(vd);
    assert(rect);

    if (!rect)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
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
            return -1;
        }
    }

    // Copy rectangle bounds
    rect->top = cropcap.bounds.top;
    rect->left = cropcap.bounds.left;
    rect->width = cropcap.bounds.width;
    rect->height = cropcap.bounds.height;

    return 0;
}

int vcap_reset_crop(vcap_dev* vd)
{
    assert(vd);

    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-cropcap.html
    struct v4l2_cropcap cropcap;

    VCAP_CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_CROPCAP, &cropcap) == -1)
    {
        if (errno == ENODATA || errno == EINVAL)
        {
            vcap_set_error(vd, "Cropping is not supported on device '%s'", vd->path);
            return -1;
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
        return -1;
    }

    return 0;
}

int vcap_get_crop(vcap_dev* vd, vcap_rect* rect)
{
    assert(vd);
    assert(rect);

    if (!rect)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
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
            return -1;
        }
        else
        {
            vcap_set_error(vd, "Unable to get crop window on device %s", vd->path);
            return -1;
        }
    }

    rect->left = crop.c.left;
    rect->top = crop.c.top;
    rect->width = crop.c.width;
    rect->height = crop.c.height;

    return 0;
}

int vcap_set_crop(vcap_dev* vd, vcap_rect rect)
{
    if (!vd)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

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
            return -1;
        }
        else
        {
            vcap_set_error(vd, "Unable to set crop window on device %s", vd->path);
            return -1;
        }
    }

    return 0;
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

static int vcap_ioctl(int fd, int request, void *arg)
{
    assert(arg);

    int result;

    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/func-ioctl.html#func-ioctl

    do
    {
        result = v4l2_ioctl(fd, request, arg);
    }
    while (result == -1 && (errno == EINTR || errno == EAGAIN));

    return result;
}

static int vcap_query_caps(const char* path, struct v4l2_capability* caps)
{
    assert(path);
    assert(caps);

    struct stat st;
    int fd = -1;

    // Device must exist
    if (stat(path, &st) == -1)
       return -1;

    // Device must be a character device
    if (!S_ISCHR(st.st_mode))
        return -1;

    // Open the video device
    fd = v4l2_open(path, O_RDWR | O_NONBLOCK, 0);

    if (fd == -1)
        return -1;

    // Obtain device capabilities
    if (vcap_ioctl(fd, VIDIOC_QUERYCAP, caps) == -1)
    {
        v4l2_close(fd);
        return -1;
    }

    // Ensure video capture is supported
    if (!(caps->capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        v4l2_close(fd);
        return -1;
    }

    v4l2_close(fd);

    return 0;
}

static int vcap_video_device_filter(const struct dirent* a)
{
    assert(a);

    if (0 == strncmp(a->d_name, "video", 5))
        return 1;
    else
        return 0;
}

static void vcap_caps_to_info(const char* path, const struct v4l2_capability caps, vcap_dev_info* info)
{
    assert(path);
    assert(info);

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
    info->stream = caps.capabilities & V4L2_CAP_STREAMING;
    info->read = caps.capabilities & V4L2_CAP_READWRITE;
}

static int vcap_request_buffers(vcap_dev* vd, int buffer_count)
{
    assert(vd);

    // Requests the specified number of buffers, returning the number of
    // available buffers
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-reqbufs.html
    struct v4l2_requestbuffers req;

    VCAP_CLEAR(req);
    req.count = buffer_count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

	if (vcap_ioctl(vd->fd, VIDIOC_REQBUFS, &req) == -1)
	{
    	vcap_set_error_errno(vd, "Unable to request buffers on %s", vd->path);
		return -1;
    }

    if (req.count == 0)
    {
        vcap_set_error(vd, "Invalid buffer count on %s", vd->path);
        return -1;
    }

    // Changes the number of buffer in the video device to match the number of
    // available buffers
    vd->buffer_count = req.count;

    // Allocates the buffer objects
    vd->buffers = vcap_malloc(req.count * sizeof(vcap_buffer));

    return 0;
}

static int vcap_init_stream(vcap_dev* vd)
{
    assert(vd);

    if (vd->buffer_count > 0)
    {
        if (vcap_request_buffers(vd, vd->buffer_count) == -1)
            return -1;

        if (vcap_map_buffers(vd) == -1)
            return -1;

        if (vcap_queue_buffers(vd) == -1)
            return -1;
    }

    return 0;
}

static int vcap_shutdown_stream(vcap_dev* vd)
{
    assert(vd);

    if (vd->buffer_count > 0)
    {
        if (vcap_unmap_buffers(vd) == -1)
            return -1;
    }

    return 0;
}

static int vcap_map_buffers(vcap_dev* vd)
{
    assert(vd);

     for (int i = 0; i < vd->buffer_count; i++)
    {
        // Query buffers, returning their size and other information
        // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-querybuf.html
        struct v4l2_buffer buf;

        VCAP_CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (vcap_ioctl(vd->fd, VIDIOC_QUERYBUF, &buf) == -1)
        {
            vcap_set_error_errno(vd, "Unable to query buffers on %s", vd->path);
            return -1;
        }

        // Map buffers
        // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/func-mmap.html
        vd->buffers[i].size = buf.length;
        vd->buffers[i].data = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, vd->fd, buf.m.offset);

        if (vd->buffers[i].data == MAP_FAILED)
        {
            vcap_set_error(vd, "MMAP failed on %s", vd->path);
            return -1;
        }
    }

    return 0;
}

static int vcap_unmap_buffers(vcap_dev* vd)
{
    assert(vd);

    // Unmap and free buffers
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/func-munmap.html
    for (int i = 0; i < vd->buffer_count; i++)
    {
        if (v4l2_munmap(vd->buffers[i].data, vd->buffers[i].size) == -1)
        {
            vcap_set_error_errno(vd, "Unmapping buffers failed on %s", vd->path);
            return -1;
        }
    }

    vcap_free(vd->buffers);
    vd->buffer_count = 0;

    return 0;
}

static int vcap_queue_buffers(vcap_dev* vd)
{
    assert(vd);

    for (int i = 0; i < vd->buffer_count; i++)
    {
        // Places a buffer in the queue
        // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-qbuf.html
        struct v4l2_buffer buf;

        VCAP_CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (vcap_ioctl(vd->fd, VIDIOC_QBUF, &buf) == -1)
        {
            vcap_set_error_errno(vd, "Unable to queue buffers on device %s", vd->path);
            return -1;
        }
	}

	return 0;
}

static int vcap_grab_mmap(vcap_dev* vd, size_t buffer_size, uint8_t* buffer)
{
    assert(vd);
    assert(buffer);

    if (!buffer)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

    struct v4l2_buffer buf;

	// Dequeue buffer
	// https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-qbuf.html
    VCAP_CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (vcap_ioctl(vd->fd, VIDIOC_DQBUF, &buf) == -1)
    {
        vcap_set_error_errno(vd, "Could not dequeue buffer on %s", vd->path);
        return -1;
    }

    memcpy(buffer, vd->buffers[buf.index].data, buffer_size);

    // Requeue buffer
	// https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-qbuf.html
    if (vcap_ioctl(vd->fd, VIDIOC_QBUF, &buf) == -1) {
        vcap_set_error_errno(vd, "Could not requeue buffer on %s", vd->path);
        return -1;
    }

    return 0;
}

static int vcap_grab_read(vcap_dev* vd, size_t buffer_size, uint8_t* buffer)
{
    assert(vd);
    assert(buffer);

    while (true)
    {
        if (v4l2_read(vd->fd, buffer, buffer_size) == -1)
        {
            if (errno == EAGAIN)
            {
                continue;
            }
            else
            {
                vcap_set_error_errno(vd, "Reading from device %s failed", vd->path);
                return -1;
            }
        }

        return 0; // Break out of loop
    }
}

static void vcap_ustrcpy(uint8_t* dst, const uint8_t* src, size_t size)
{
    assert(dst);
    assert(src);

    snprintf((char*)dst, size, "%s", (char*)src);
}

static void vcap_strcpy(char* dst, const char* src, size_t size)
{
    assert(dst);
    assert(src);

    snprintf(dst, size, "%s", src);
}

static void vcap_set_error(vcap_dev* vd, const char* fmt, ...)
{
    assert(vd);
    assert(fmt);

    char error_msg1[1024];
    char error_msg2[1024];

    snprintf(error_msg1, sizeof(error_msg1), "[%s:%d]", __func__, __LINE__);
    assert(vd);

    va_list args;
    va_start(args, fmt);
    vsnprintf(error_msg2, sizeof(error_msg2), fmt, args);
    va_end(args);

    snprintf(vd->error_msg, sizeof(vd->error_msg), "%s %s", error_msg1, error_msg2);
}

static void vcap_set_error_errno(vcap_dev* vd, const char* fmt, ...)
{
    assert(vd);
    assert(fmt);

    char error_msg1[1024];
    char error_msg2[1024];

    snprintf(error_msg1, sizeof(error_msg1), "[%s:%d] (%s)", __func__, __LINE__, strerror(errno));

    va_list args;
    va_start(args, fmt);
    vsnprintf(error_msg2, sizeof(error_msg2), fmt, args);
    va_end(args);

    snprintf(vd->error_msg, sizeof(vd->error_msg), "%s %s", error_msg1, error_msg2);
}

//==============================================================================
// Enumeration Functions
//==============================================================================

static bool vcap_type_supported(uint32_t type)
{
    switch (type)
    {
        case V4L2_CTRL_TYPE_INTEGER:
        case V4L2_CTRL_TYPE_BOOLEAN:
        case V4L2_CTRL_TYPE_MENU:
        case V4L2_CTRL_TYPE_INTEGER_MENU:
        case V4L2_CTRL_TYPE_BUTTON:
            return true;
    }

    return false;
}

static const char* vcap_type_str(vcap_ctrl_type type)
{
    switch (type)
    {
        case V4L2_CTRL_TYPE_INTEGER:
            return "Integer";

        case V4L2_CTRL_TYPE_BOOLEAN:
            return "Boolean";

        case V4L2_CTRL_TYPE_MENU:
            return "Menu";

        case V4L2_CTRL_TYPE_INTEGER_MENU:
            return "Integer Menu";

        case V4L2_CTRL_TYPE_BUTTON:
            return "Button";
    }

    return "Unknown";
}

static int vcap_enum_fmts(vcap_dev* vd, vcap_fmt_info* info, uint32_t index)
{
    assert(vd);
    assert(info);

    // Enumerate formats
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-enum-fmt.html
    struct v4l2_fmtdesc fmtd;

    VCAP_CLEAR(fmtd);
    fmtd.index = index;
    fmtd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_ENUM_FMT, &fmtd) == -1)
    {
        if (errno == EINVAL)
        {
            return VCAP_ENUM_INVALID;
        }
        else
        {
            vcap_set_error_errno(vd, "Unable to enumerate formats on device %s", vd->path);
            return VCAP_ENUM_ERROR;
        }
    }

    // Copy description
    vcap_ustrcpy(info->name, fmtd.description, sizeof(info->name));

    // Convert FOURCC code
    vcap_fourcc_string(fmtd.pixelformat, info->fourcc);

    // Copy pixel format
    info->id = fmtd.pixelformat;

    return VCAP_ENUM_OK;
}

static int vcap_enum_sizes(vcap_dev* vd, vcap_fmt_id fmt, vcap_size* size, uint32_t index)
{
    assert(vd);
    assert(size);

    // Enumerate frame sizes
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-enum-framesizes.html
    struct v4l2_frmsizeenum fenum;

    VCAP_CLEAR(fenum);
    fenum.index = index;
    fenum.pixel_format = fmt;

    if (vcap_ioctl(vd->fd, VIDIOC_ENUM_FRAMESIZES, &fenum) == -1)
    {
        if (errno == EINVAL)
        {
            return VCAP_ENUM_INVALID;
        } else
        {
            vcap_set_error_errno(vd, "Unable to enumerate sizes on device '%s'", vd->path);
            return VCAP_ENUM_ERROR;
        }
    }

    // Only discrete sizes are supported
    if (fenum.type != V4L2_FRMSIZE_TYPE_DISCRETE)
        return VCAP_ENUM_DISABLED;

    size->width  = fenum.discrete.width;
    size->height = fenum.discrete.height;

    return VCAP_ENUM_OK;
}

static int vcap_enum_rates(vcap_dev* vd, vcap_fmt_id fmt, vcap_size size, vcap_rate* rate, uint32_t index)
{
    assert(vd);
    assert(rate);

    // Enumerate frame rates
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-enum-frameintervals.html
    struct v4l2_frmivalenum frenum;

    VCAP_CLEAR(frenum);
    frenum.index = index;
    frenum.pixel_format = fmt;
    frenum.width = size.width;
    frenum.height = size.height;

    if (vcap_ioctl(vd->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frenum) == -1)
    {
        if (errno == EINVAL)
        {
            return VCAP_ENUM_INVALID;
        }
        else
        {
            vcap_set_error_errno(vd, "Unable to enumerate frame rates on device %s", vd->path);
            return VCAP_ENUM_ERROR;
        }
    }

    // Only discrete frame rates are supported
    if (frenum.type != V4L2_FRMIVAL_TYPE_DISCRETE)
        return VCAP_ENUM_DISABLED;

    // NOTE: We swap the numerator and denominator because Vcap uses frame rates
    // instead of intervals.
    rate->numerator = frenum.discrete.denominator;
    rate->denominator = frenum.discrete.numerator;

    return VCAP_ENUM_OK;
}

//TODO: DRY
static int vcap_enum_ctrls(vcap_dev* vd, vcap_ctrl_info* info, uint32_t index)
{
    assert(vd);
    assert(info);

    int count = 0;

    // Enuemrate user controls
    for (vcap_ctrl_id ctrl = V4L2_CID_BASE; ctrl < V4L2_CID_LASTP1; ctrl++)
    {
        int result = vcap_get_ctrl_info(vd, ctrl, info);

        if (result == VCAP_CTRL_ERROR)
            return VCAP_ENUM_ERROR;

        if (result == VCAP_CTRL_INVALID)
            continue;

        if (index == count)
            return VCAP_ENUM_OK;
        else
            count++;
    }

    // Enumerate camera controls
    for (vcap_ctrl_id ctrl = V4L2_CID_CAMERA_CLASS_BASE; ctrl < VCAP_CID_CAMERA_CLASS_LASTP1; ctrl++)
    {
        int result = vcap_get_ctrl_info(vd, ctrl, info);

        if (result == VCAP_CTRL_ERROR)
            return VCAP_ENUM_ERROR;

        if (result == VCAP_CTRL_INVALID)
            continue;

        if (index == count)
            return VCAP_ENUM_OK;
        else
            count++;
    }

    return VCAP_ENUM_INVALID;
}

static int vcap_enum_menu(vcap_dev* vd, vcap_ctrl_id ctrl, vcap_menu_item* item, uint32_t index)
{
    assert(vd);
    assert(item);

    // Check if supported and a menu
    vcap_ctrl_info info;

    int result = vcap_get_ctrl_info(vd, ctrl, &info);

    if (result == VCAP_CTRL_ERROR)
        return VCAP_ENUM_ERROR;

    if (result == VCAP_CTRL_INVALID)
    {
        vcap_set_error(vd, "Can't enumerate menu of an invalid control");
        return VCAP_ENUM_ERROR;
    }

    if (info.read_only) // TODO: necessary? required?
    {
        vcap_set_error(vd, "Can't enumerate menu of a read-only control");
        return VCAP_ENUM_ERROR;
    }

    if (info.type != V4L2_CTRL_TYPE_MENU && info.type != V4L2_CTRL_TYPE_INTEGER_MENU)
    {
        vcap_set_error(vd, "Control is not a menu");
        return VCAP_ENUM_ERROR;
    }

    if (index < info.min || index > info.max)
    {
        return VCAP_ENUM_INVALID;
    }

    // Loop through all entries in the menu until count == index

    uint32_t count = 0;

    for (int32_t i = info.min; i <= info.max; i += info.step)
    {
        // Query menu
        // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/vidioc-queryctrl.html
        struct v4l2_querymenu qmenu;

        VCAP_CLEAR(qmenu);
        qmenu.id = ctrl;
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
                return VCAP_ENUM_ERROR;
            }
        }

        if (index == count)
        {
            item->index = i;

            if (info.type == V4L2_CTRL_TYPE_MENU)
                vcap_ustrcpy(item->name, qmenu.name, sizeof(item->name));
            else
                item->value = qmenu.value;

            return VCAP_ENUM_OK;
        }
        else
        {
            count++;
        }
    }

    return VCAP_ENUM_INVALID;
}

