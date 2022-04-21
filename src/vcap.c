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
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

// Filters device list so that 'scandir' returns only video devices.
static int video_device_filter(const struct dirent *a);

const char* vcap_get_error()
{
    return vcap_get_error_priv();
}

void vcap_set_alloc(vcap_malloc_func malloc_func, vcap_free_func free_func)
{
    vcap_set_alloc_priv(malloc_func, free_func);
}

//
// Prints device information. The implementation of this function is very
// pedantic in terms of error checking. Every error condition is checked and
// reported. A user application may choose to ignore some error cases, trading
// a little robustness for some convenience.
//
int vcap_dump_info(vcap_fg* fg, FILE* file)
{
    if (!fg)
    {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    int ret = 0;

    vcap_fmt_itr* fmt_itr = NULL;
    vcap_size_itr* size_itr = NULL;
    vcap_rate_itr* rate_itr = NULL;
    vcap_ctrl_itr* ctrl_itr = NULL;
    vcap_menu_itr* menu_itr = NULL;

    vcap_device_info info = { 0 };
    vcap_get_device_info(fg, &info);

    //==========================================================================
    // Print device info
    //==========================================================================
    printf("------------------------------------------------\n");
    fprintf(file, "Device: %s\n", info.path);
    fprintf(file, "Driver: %s\n", info.driver);
    fprintf(file, "Driver version: %s\n", info.version_str);
    fprintf(file, "Card: %s\n", info.card);
    fprintf(file, "Bus Info: %s\n", info.bus_info);

    //==========================================================================
    // Enumerate formats
    //==========================================================================
    vcap_fmt_desc fmt_desc;
    fmt_itr = vcap_new_fmt_itr(fg);

    // Check for errors during format iterator allocation
    if (!fmt_itr)
    {
        VCAP_ERROR("%s", vcap_get_error());
        ret = -1; goto end;
    }

    while (vcap_fmt_itr_next(fmt_itr, &fmt_desc))
    {
        fprintf(file, "------------------------------------------------\n");
        fprintf(file, "Format: %s, FourCC: %s\n", fmt_desc.name, fmt_desc.fourcc);
        fprintf(file, "Sizes:\n");

        //======================================================================
        // Enumerate sizes
        //======================================================================
        vcap_size size;
        size_itr = vcap_new_size_itr(fg, fmt_desc.id);

        // Check for errors during frame size iterator allocation
        if (!size_itr)
        {
            VCAP_ERROR("%s", vcap_get_error());
            ret = -1; goto end;
        }

        while (vcap_size_itr_next(size_itr, &size))
        {
            fprintf(file, "   %u x %u: ", size.width, size.height);
            fprintf(file, "(Frame rates:");

            //==================================================================
            // Enumerate frame rates
            //==================================================================
            vcap_rate rate;
            rate_itr = vcap_new_rate_itr(fg, fmt_desc.id, size);

            // Check for errors during frame rate iterator allocation
            if (!rate_itr)
            {
                VCAP_ERROR("%s", vcap_get_error());
                ret = -1; goto end;
            }

            while (vcap_rate_itr_next(rate_itr, &rate))
            {
                fprintf(file, " %u/%u", rate.numerator, rate.denominator);
            }

            // Check for errors during frame rate iteration
            if (vcap_rate_itr_error(rate_itr))
            {
                VCAP_ERROR("%s", vcap_get_error());
                ret = -1; goto end;
            }

            vcap_free(rate_itr);
            rate_itr = NULL;

            fprintf(file, ")\n");
        }

        // Check for errors during frame size iteration
        if (vcap_size_itr_error(size_itr))
        {
            VCAP_ERROR("%s", vcap_get_error());
            ret = -1; goto end;
        }

        vcap_free(size_itr);
        size_itr = NULL;
    }

    // Check for errors during format iteration
    if (vcap_fmt_itr_error(fmt_itr))
    {
        VCAP_ERROR("%s", vcap_get_error());
        ret = -1; goto end;
    }

    vcap_free(fmt_itr);
    fmt_itr = NULL;

    //==========================================================================
    // Enumerate controls
    //==========================================================================
    fprintf(file, "------------------------------------------------\n");
    fprintf(file, "Controls:\n");

    vcap_ctrl_desc ctrl_desc;
    ctrl_itr = vcap_new_ctrl_itr(fg);

    // Check for errors during control iterator allocation
    if (!ctrl_itr)
    {
        VCAP_ERROR("%s", vcap_get_error());
        ret = -1; goto end;
    }

    while (vcap_ctrl_itr_next(ctrl_itr, &ctrl_desc))
    {
        printf("   Name: %s, Type: %s\n", ctrl_desc.name, ctrl_desc.type_name);

        if (ctrl_desc.type == VCAP_CTRL_TYPE_MENU || ctrl_desc.type == VCAP_CTRL_TYPE_INTEGER_MENU)
        {
            printf("   Menu:\n");

            //==================================================================
            // Enumerate menu
            //==================================================================
            vcap_menu_item menu_item;
            vcap_menu_itr* menu_itr = vcap_new_menu_itr(fg, ctrl_desc.id);

            // Check for errors during menu iterator allocation
            if (!menu_itr)
            {
                VCAP_ERROR("%s", vcap_get_error());
                ret = -1; goto end;
            }

            while (vcap_menu_itr_next(menu_itr, &menu_item))
            {
                if (ctrl_desc.type == VCAP_CTRL_TYPE_MENU)
                    printf("      %i : %s\n", menu_item.index, menu_item.name);
                else
                    printf("      %i : %li\n", menu_item.index, menu_item.value);
            }

            // Check for errors during menu iteration
            if (vcap_menu_itr_error(menu_itr))
            {
                VCAP_ERROR("%s", vcap_get_error());
                ret = -1; goto end;
            }

            vcap_free(menu_itr);
            menu_itr = NULL;
        }
    }

    // Check for errors during control iteration
    if (vcap_ctrl_itr_error(ctrl_itr))
    {
        VCAP_ERROR("%s", vcap_get_error());
        ret = -1; goto end;
    }

    vcap_free(ctrl_itr);
    ctrl_itr = NULL;

end:

    if (fmt_itr)
        vcap_free(fmt_itr);

    if (size_itr)
        vcap_free(size_itr);

    if (rate_itr)
        vcap_free(rate_itr);

    if (ctrl_itr)
        vcap_free(ctrl_itr);

    if (menu_itr)
        vcap_free(menu_itr);

    return ret;
}

int vcap_enum_devices(unsigned index, vcap_fg* fg)
{
    int count = 0;

    struct dirent **names;
    int n = scandir("/dev", &names, video_device_filter, alphasort);

    if (n < 0)
    {
        VCAP_ERROR("Failed to scan '/dev' directory");
        return VCAP_ENUM_ERROR;
    }

    char path[512];

    for (int i = 0; i < n; i++)
    {
        snprintf(path, sizeof(path), "/dev/%s", names[i]->d_name);

        if (vcap_open(path, fg) == 0)
        {
            if (index == count)
            {
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

int vcap_open(const char* path, vcap_fg* fg)
{
    struct stat st;
    struct v4l2_capability caps;

    fg->fd = -1;

    // Device must exist
    if (stat(path, &st) == -1)
    {
        VCAP_ERROR_ERRNO("Video device %s does not exist", path);
        return -1;
    }

    // Device must be a character device
    if (!S_ISCHR(st.st_mode))
    {
        VCAP_ERROR_ERRNO("Video device %s is not a character device", path);
        return -1;
    }

    // Open the video device
    fg->fd = v4l2_open(path, O_RDWR | O_NONBLOCK, 0);

    if (-1 == fg->fd)
    {
        VCAP_ERROR_ERRNO("Opening video device %s failed", path);
        return -1;
    }

    // Ensure child processes should't inherit the video device
    fcntl(fg->fd, F_SETFD, FD_CLOEXEC);

    // Obtain device capabilities
    if (vcap_ioctl(fg->fd, VIDIOC_QUERYCAP, &caps) == -1)
    {
        VCAP_ERROR_ERRNO("Querying video device %s capabilities failed", path);
        vcap_close(fg);
        return -1;
    }

    // Ensure video capture is supported
    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        VCAP_ERROR("Video device %s does not support video capture", path);
        vcap_close(fg);
        return -1;
    }

    // Copy path
    strncpy(fg->path, path, sizeof(fg->path));

    // Copy capabilities
    fg->caps = caps;

    return 0;
}

void vcap_close(const vcap_fg* fg)
{
    v4l2_close(fg->fd);
}

void vcap_get_device_info(const vcap_fg* fg, vcap_device_info* info)
{
    assert(fg);
    assert(info);

    struct v4l2_capability caps = fg->caps;

    // Copy device information
    strncpy(info->path, fg->path, sizeof(info->path));
    strncpy((char*)info->driver, (char*)caps.driver, sizeof(info->driver));
    strncpy((char*)info->card, (char*)caps.card, sizeof(info->card));
    strncpy((char*)info->bus_info, (char*)caps.bus_info, sizeof(info->bus_info));
    info->version = caps.version;

    // Decode version
    snprintf((char*)info->version_str, sizeof(info->version_str), "%u.%u.%u",
            (caps.version >> 16) & 0xFF,
            (caps.version >> 8) & 0xFF,
            (caps.version & 0xFF));
}

vcap_frame* vcap_alloc_frame(vcap_fg* fg)
{
    assert(fg);

    vcap_frame* frame = vcap_malloc(sizeof(vcap_frame));

    if (!frame)
    {
        VCAP_ERROR_ERRNO("Ran out of memory allocating frame");
        return NULL;
    }

    struct v4l2_format fmt;

    VCAP_CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(fg->fd, VIDIOC_G_FMT, &fmt))
    {
        VCAP_ERROR_ERRNO("Unable to get format on device '%s'", fg->path);
        goto error;
    }

    frame->fmt = vcap_convert_fmt_id(fmt.fmt.pix.pixelformat);
    frame->size.width = fmt.fmt.pix.width;
    frame->size.height = fmt.fmt.pix.height;
    frame->stride = fmt.fmt.pix.bytesperline;
    frame->length = fmt.fmt.pix.sizeimage;
    frame->data = vcap_malloc(frame->length);

    if (!frame->data)
    {
        VCAP_ERROR_ERRNO("Ran out of memory allocating frame data");
        goto error;
    }

    return frame;

error:

    vcap_free(frame);
    return NULL;
}

void vcap_free_frame(vcap_frame* frame)
{
    if (!frame)
        return;

    if (frame->data)
        vcap_free(frame->data);

    vcap_free(frame);
}

int vcap_copy_frame(vcap_frame* dst, vcap_frame* src)
{
    if (!dst)
    {
        VCAP_ERROR("Parameter 'dst' cannot be null");
        return -1;
    }

    if (!src)
    {
        VCAP_ERROR("Parameter 'src' cannot be null");
        return -1;
    }

    if (!src->data || src->length == 0)
    {
        VCAP_ERROR("Invalid frame 'src'");
        return -1;
    }

    if (!dst->data || src->length != dst->length)
    {
        vcap_free(dst->data);
        dst->data = vcap_malloc(src->length);

        if (!dst->data)
        {
            VCAP_ERROR("Out of memory while copying frame");
            return -1;
        }
    }

    dst->fmt = src->fmt;
    dst->size = src->size;
    dst->stride = src->stride;
    dst->length = src->length;

    memcpy(dst->data, src->data, dst->length);

    return 0;
}

vcap_frame* vcap_clone_frame(vcap_frame* frame)
{
    if (!frame)
    {
        VCAP_ERROR("Parameter 'frame' cannot be null");
        return NULL;
    }

    if (!frame->data || frame->length == 0)
    {
        VCAP_ERROR("Invalid frame");
        return NULL;
    }

    vcap_frame* clone = vcap_malloc(sizeof(vcap_frame));

    if (!clone)
    {
        VCAP_ERROR("Out of memory while cloning frame");
        return NULL;
    }

    clone->data = NULL;

    if (vcap_copy_frame(clone, frame) == -1)
    {
        VCAP_ERROR("%s", vcap_get_error());;
        vcap_free_frame(clone);
        return NULL;
    }

    return clone;
}

int vcap_grab(vcap_fg* fg, vcap_frame* frame)
{
    if (!fg)
    {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    if (!frame)
    {
        VCAP_ERROR("Parameter 'frame' cannot be null");
        return -1;
    }

    fd_set fds;
    int result;
    struct timeval tv;

    while (1)
    {
        do
        {
            FD_ZERO(&fds);
            FD_SET(fg->fd, &fds);

            // Timeout
            tv.tv_sec = 5;
            tv.tv_usec = 0;

            result = select(fg->fd + 1, &fds, NULL, NULL, &tv);
        }
        while (result == -1 && errno == EINTR);

        if (result == -1)
        {
            VCAP_ERROR("Unable to grab frame on device '%s'", fg->path);
            return -1;
        }

        if (result == 0)
        {
            VCAP_ERROR("Unable to grab frame (timeout expired) on device '%s'", fg->path);
            return -1;
        }

        if (v4l2_read(fg->fd, frame->data, frame->length) == -1)
        {
            if (errno == EAGAIN)
            {
                continue;
            }
            else
            {
                VCAP_ERROR_ERRNO("Reading from device '%s' failed", fg->path);
                return -1;
            }
        }

        return 0; // Break out of loop
    }
}

int vcap_get_crop_bounds(vcap_fg* fg, vcap_rect* rect)
{
    if (!fg)
    {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    if (!rect)
    {
        VCAP_ERROR("Parameter 'rect' cannot be null");
        return -1;
    }

    struct v4l2_cropcap cropcap;

    VCAP_CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(fg->fd, VIDIOC_CROPCAP, &cropcap) == -1)
    {
        if (errno == ENODATA || errno == EINVAL)
        {
            VCAP_ERROR("Cropping is not supported on device '%s'", fg->path);
            return -1;
        }
    }

    rect->top = cropcap.bounds.top;
    rect->left = cropcap.bounds.left;
    rect->width = cropcap.bounds.width;
    rect->height = cropcap.bounds.height;

    return 0;
}

int vcap_reset_crop(vcap_fg* fg)
{
    if (!fg)
    {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    struct v4l2_cropcap cropcap;

    VCAP_CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(fg->fd, VIDIOC_CROPCAP, &cropcap) == -1)
    {
        if (errno == ENODATA || errno == EINVAL)
        {
            VCAP_ERROR("Cropping is not supported on device '%s'", fg->path);
            return -1;
        }
    }

    struct v4l2_crop crop;

    VCAP_CLEAR(crop);
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect;

    if (vcap_ioctl(fg->fd, VIDIOC_S_CROP, &crop) == -1)
    {
        VCAP_ERROR_ERRNO("Unable to set crop window on device '%s'", fg->path);
        return -1;
    }

    return 0;
}

int vcap_get_crop(vcap_fg* fg, vcap_rect* rect)
{
    if (!fg)
    {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    if (!rect)
    {
        VCAP_ERROR("Parameter 'rect' cannot be null");
        return -1;
    }

    struct v4l2_crop crop;

    VCAP_CLEAR(crop);
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(fg->fd, VIDIOC_G_CROP, &crop) == -1)
    {
        if (errno == ENODATA || errno == EINVAL)
        {
            VCAP_ERROR("Cropping is not supported on device '%s'", fg->path);
            return -1;
        } else
        {
            VCAP_ERROR_ERRNO("Unable to get crop window on device '%s'", fg->path);
            return -1;
        }
    }

    rect->left = crop.c.left;
    rect->top = crop.c.top;
    rect->width = crop.c.width;
    rect->height = crop.c.height;

    return 0;
}

int vcap_set_crop(vcap_fg* fg, vcap_rect rect)
{
    if (!fg)
    {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    struct v4l2_crop crop;

    VCAP_CLEAR(crop);
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c.left = rect.left;
    crop.c.top = rect.top;
    crop.c.width = rect.width;
    crop.c.height = rect.height;

    if (vcap_ioctl(fg->fd, VIDIOC_S_CROP, &crop) == -1)
    {
        if (errno == ENODATA || errno == EINVAL)
        {
            VCAP_ERROR("Cropping is not supported on device '%s'", fg->path);
            return -1;
        } else
        {
            VCAP_ERROR_ERRNO("Unable to set crop window on device '%s'", fg->path);
            return -1;
        }
    }

    return 0;
}

static int video_device_filter(const struct dirent *a)
{
    if (0 == strncmp(a->d_name, "video", 5))
        return 1;
    else
        return 0;
}
