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
static int vcap_request_buffers(vcap_vd* vd, int buffer_count);
static int vcap_map_buffers(vcap_vd* vd);
static int vcap_unmap_buffers(vcap_vd* vd);
static int vcap_queue_buffers(vcap_vd* vd);

static int vcap_video_device_filter(const struct dirent *a);

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
int vcap_dump_info(vcap_vd* vd, FILE* file)
{
    if (!vd)
    {
        VCAP_ERROR("Parameter 'vd' cannot be null");
        return -1;
    }

    int ret = 0;

    vcap_fmt_itr* fmt_itr = NULL;
    vcap_size_itr* size_itr = NULL;
    vcap_rate_itr* rate_itr = NULL;
    vcap_ctrl_itr* ctrl_itr = NULL;
    vcap_menu_itr* menu_itr = NULL;

    vcap_device_info info = { 0 };
    vcap_get_device_info(vd, &info);

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
    fmt_itr = vcap_new_fmt_itr(vd);

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
        size_itr = vcap_new_size_itr(vd, fmt_desc.id);

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
            rate_itr = vcap_new_rate_itr(vd, fmt_desc.id, size);

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
    ctrl_itr = vcap_new_ctrl_itr(vd);

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
            vcap_menu_itr* menu_itr = vcap_new_menu_itr(vd, ctrl_desc.id);

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

int vcap_query_caps(const char* path, struct v4l2_capability* caps)
{
    assert(path);
    assert(caps);

    struct stat st;
    int fd = -1;

    // Device must exist
    if (-1 == stat(path, &st))
       return -1;

    // Device must be a character device
    if (!S_ISCHR(st.st_mode))
        return -1;

    // Open the video device
    fd = v4l2_open(path, O_RDWR | O_NONBLOCK, 0);

    if (-1 == fd)
        return -1;

    // Obtain device capabilities
    if (-1 == vcap_ioctl(fd, VIDIOC_QUERYCAP, caps))
    {
        //VCAP_ERROR_ERRNO("Querying video device %s capabilities failed", path);
        v4l2_close(fd);
        return -1;
    }

    // Ensure video capture is supported
    if (!(caps->capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        //VCAP_ERROR("Video device %s does not support video capture", path);
        v4l2_close(fd);
        return -1;
    }

    v4l2_close(fd);

    return 0;
}

void vcap_caps_to_info(const char* path, const struct v4l2_capability caps, vcap_device_info* info)
{
    assert(info);

    // Copy device information
    vcap_strcpy(info->path, path, sizeof(info->path));
    vcap_strcpy((char*)info->driver, (char*)caps.driver, sizeof(info->driver));
    vcap_strcpy((char*)info->card, (char*)caps.card, sizeof(info->card));
    vcap_strcpy((char*)info->bus_info, (char*)caps.bus_info, sizeof(info->bus_info));
    info->version = caps.version;

    // Decode version
    snprintf((char*)info->version_str, sizeof(info->version_str), "%u.%u.%u",
            (caps.version >> 16) & 0xFF,
            (caps.version >> 8) & 0xFF,
            (caps.version & 0xFF));
}

int vcap_enum_devices(unsigned index, vcap_device_info* info)
{
    int count = 0;

    struct dirent **names;
    int n = scandir("/dev", &names, vcap_video_device_filter, alphasort);

    if (n < 0)
    {
        VCAP_ERROR("Failed to scan '/dev' directory");
        return VCAP_ENUM_ERROR;
    }

    char path[512];

    for (int i = 0; i < n; i++)
    {
        snprintf(path, sizeof(path), "/dev/%s", names[i]->d_name);

        struct v4l2_capability caps;

        if (0 == vcap_query_caps(path, &caps))
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

vcap_vd* vcap_create_device(const char* path, int buffer_count)
{
    vcap_vd* vd = vcap_malloc(sizeof(vcap_vd));
    memset(vd, 0, sizeof(vcap_vd));

    vd->fd = -1;
    vd->buffer_count = buffer_count;

    vcap_strcpy(vd->path, path, sizeof(vd->path));

    return vd;
}

void vcap_destroy_device(vcap_vd* vd)
{
    vcap_close(vd);
    vcap_free(vd);
}

int vcap_open(vcap_vd* vd)
{
    assert(vd);

    if (vd->open)
    {
        VCAP_ERROR("Device %s is already open", vd->path);
        return -1;
    }

    struct stat st;
    struct v4l2_capability caps;

    // Device must exist
    if (-1 == stat(vd->path, &st))
    {
        VCAP_ERROR_ERRNO("Video device %s does not exist", vd->path);
        return -1;
    }

    // Device must be a character device
    if (!S_ISCHR(st.st_mode))
    {
        VCAP_ERROR_ERRNO("Video device %s is not a character device", vd->path);
        return -1;
    }

    // Open the video device
    vd->fd = v4l2_open(vd->path, O_RDWR | O_NONBLOCK, 0);

    if (-1 == vd->fd)
    {
        VCAP_ERROR_ERRNO("Opening video device %s failed", vd->path);
        return -1;
    }

    // Ensure child processes dont't inherit the video device
    fcntl(vd->fd, F_SETFD, FD_CLOEXEC);

    // Obtain device capabilities
    if (-1 == vcap_ioctl(vd->fd, VIDIOC_QUERYCAP, &caps))
    {
        VCAP_ERROR_ERRNO("Querying video device %s capabilities failed", vd->path);
        vcap_close(vd);
        return -1;
    }

    // Ensure video capture is supported
    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        VCAP_ERROR("Video device %s does not support video capture", vd->path);
        vcap_close(vd);
        return -1;
    }

    // Ensure video capture is supported
    if (!(caps.capabilities & V4L2_CAP_STREAMING))
    {
        VCAP_ERROR("Video device %s does not support streaming", vd->path);
        vcap_close(vd);
        return -1;
    }

    // Copy capabilities
    vd->caps = caps;

    // Must be set to 0 to avoid bad things
    //vd->buffer_count = 0;

    vd->open = true;

    return 0;
}

int vcap_close(vcap_vd* vd)
{
    if (!vd->open)
    {
        VCAP_ERROR("Unable to close %s. Device is not open", vd->path);
        return -1;
    }

    if (vd->buffer_count > 0)
    {
        vcap_unmap_buffers(vd);
    }

    if (vd->fd >= 0)
        v4l2_close(vd->fd);

    return 0;
}

int vcap_init_stream(vcap_vd* vd, int buffer_count)
{
    if (buffer_count > 0)
    {
        if (-1 == vcap_request_buffers(vd, buffer_count))
            return -1;

        if (-1 == vcap_map_buffers(vd))
            return -1;

        if (-1 == vcap_queue_buffers(vd))
            return -1;
    }

    return 0;
}

int vcap_shutdown_stream(vcap_vd* vd)
{
    if (vd->buffer_count > 0)
    {
        if (-1 == vcap_unmap_buffers(vd))
            return -1;
    }

    return 0;
}

int vcap_start_stream(const vcap_vd* vd)
{
    if (vd->buffer_count > 0)
    {
    	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == vcap_ioctl(vd->fd, VIDIOC_STREAMON, &type))
        {
            VCAP_ERROR_ERRNO("Unable to start stream on %s", vd->path);
            return -1;
        }
    }

    return 0;
}

int vcap_stop_stream(const vcap_vd* vd)
{
    if (vd->buffer_count > 0)
    {
    	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == vcap_ioctl(vd->fd, VIDIOC_STREAMOFF, &type))
        {
            VCAP_ERROR_ERRNO("Unable to stop stream on %s", vd->path);
            return -1;
        }
    }

    return 0;
}

void vcap_get_device_info(const vcap_vd* vd, vcap_device_info* info)
{
    assert(vd);
    assert(info);

    vcap_caps_to_info(vd->path, vd->caps, info);
}

vcap_frame* vcap_alloc_frame(vcap_vd* vd)
{
    assert(vd);

    vcap_frame* frame = vcap_malloc(sizeof(vcap_frame));

    if (!frame)
    {
        VCAP_ERROR_ERRNO("Ran out of memory allocating frame");
        return NULL;
    }

    struct v4l2_format fmt;

    VCAP_CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_G_FMT, &fmt))
    {
        VCAP_ERROR_ERRNO("Unable to get format on device '%s'", vd->path);
        vcap_free(frame);
        return NULL;
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
        vcap_free(frame);
        return NULL;
    }

    return frame;
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
    assert(dst);
    assert(src);

    if (!src->data || src->length == 0)
    {
        VCAP_ERROR("Invalid source frame");
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
    assert(frame);

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

int vcap_update_frame(vcap_vd* vd, vcap_frame* frame)
{
    assert(vd);
    assert(frame);

    struct v4l2_format fmt;

    VCAP_CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_G_FMT, &fmt))
    {
        VCAP_ERROR_ERRNO("Unable to get format on device '%s'", vd->path);
        return -1;
    }

    int length = frame->length;

    frame->fmt = vcap_convert_fmt_id(fmt.fmt.pix.pixelformat);
    frame->size.width = fmt.fmt.pix.width;
    frame->size.height = fmt.fmt.pix.height;
    frame->stride = fmt.fmt.pix.bytesperline;
    frame->length = fmt.fmt.pix.sizeimage;

    if (length != frame->length)
        frame->data = realloc(frame->data, frame->length); // vcap_realloc

    return 0;
}

int vcap_grab_mmap(vcap_vd* vd, vcap_frame* frame)
{
    struct v4l2_buffer buf;

	//dequeue buffer
    VCAP_CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == vcap_ioctl(vd->fd, VIDIOC_DQBUF, &buf))
    {
        VCAP_ERROR_ERRNO("Could not dequeue buffer on %s", vd->path);
        return -1;
    }

    memcpy(frame->data, vd->buffers[buf.index].data, frame->length);

    if (-1 == vcap_ioctl(vd->fd, VIDIOC_QBUF, &buf)) {
        VCAP_ERROR_ERRNO("Could not requeue buffer on %s", vd->path);
        return -1;
    }

    return 0;
}

int vcap_grab_read(vcap_vd* vd, vcap_frame* frame)
{
    while (true)
    {
        if (v4l2_read(vd->fd, frame->data, frame->length) == -1)
        {
            if (errno == EAGAIN)
            {
                continue;
            }
            else
            {
                VCAP_ERROR_ERRNO("Reading from device %s failed", vd->path);
                return -1;
            }
        }

        return 0; // Break out of loop
    }
}

int vcap_grab(vcap_vd* vd, vcap_frame* frame)
{
    assert(vd);
    assert(frame);

    if (vd->buffer_count > 0)
        return vcap_grab_mmap(vd, frame);

    return vcap_grab_read(vd, frame);
}

int vcap_get_crop_bounds(vcap_vd* vd, vcap_rect* rect)
{
    if (!vd)
    {
        VCAP_ERROR("Parameter 'vd' cannot be null");
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

    if (vcap_ioctl(vd->fd, VIDIOC_CROPCAP, &cropcap) == -1)
    {
        if (errno == ENODATA || errno == EINVAL)
        {
            VCAP_ERROR("Cropping is not supported on device '%s'", vd->path);
            return -1;
        }
    }

    rect->top = cropcap.bounds.top;
    rect->left = cropcap.bounds.left;
    rect->width = cropcap.bounds.width;
    rect->height = cropcap.bounds.height;

    return 0;
}

int vcap_reset_crop(vcap_vd* vd)
{
    if (!vd)
    {
        VCAP_ERROR("Parameter 'vd' cannot be null");
        return -1;
    }

    struct v4l2_cropcap cropcap;

    VCAP_CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_CROPCAP, &cropcap) == -1)
    {
        if (errno == ENODATA || errno == EINVAL)
        {
            VCAP_ERROR("Cropping is not supported on device '%s'", vd->path);
            return -1;
        }
    }

    struct v4l2_crop crop;

    VCAP_CLEAR(crop);
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect;

    if (vcap_ioctl(vd->fd, VIDIOC_S_CROP, &crop) == -1)
    {
        VCAP_ERROR_ERRNO("Unable to set crop window on device '%s'", vd->path);
        return -1;
    }

    return 0;
}

int vcap_get_crop(vcap_vd* vd, vcap_rect* rect)
{
    if (!vd)
    {
        VCAP_ERROR("Parameter 'vd' cannot be null");
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

    if (vcap_ioctl(vd->fd, VIDIOC_G_CROP, &crop) == -1)
    {
        if (errno == ENODATA || errno == EINVAL)
        {
            VCAP_ERROR("Cropping is not supported on device '%s'", vd->path);
            return -1;
        } else
        {
            VCAP_ERROR_ERRNO("Unable to get crop window on device '%s'", vd->path);
            return -1;
        }
    }

    rect->left = crop.c.left;
    rect->top = crop.c.top;
    rect->width = crop.c.width;
    rect->height = crop.c.height;

    return 0;
}

int vcap_set_crop(vcap_vd* vd, vcap_rect rect)
{
    if (!vd)
    {
        VCAP_ERROR("Parameter 'vd' cannot be null");
        return -1;
    }

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
            VCAP_ERROR("Cropping is not supported on device '%s'", vd->path);
            return -1;
        } else
        {
            VCAP_ERROR_ERRNO("Unable to set crop window on device '%s'", vd->path);
            return -1;
        }
    }

    return 0;
}

static int vcap_request_buffers(vcap_vd* vd, int buffer_count)
{
    struct v4l2_requestbuffers req;

    VCAP_CLEAR(req);
    req.count = buffer_count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

	if (-1 == vcap_ioctl(vd->fd, VIDIOC_REQBUFS, &req))
	{
    	VCAP_ERROR_ERRNO("Unable to request buffers on %s", vd->path);
		return -1;
    }

    if (0 == req.count)
    {
        VCAP_ERROR("Invalid buffer count on %s", vd->path);
        return -1;
    }

    vd->buffer_count = req.count;
    vd->buffers = vcap_malloc(req.count * sizeof(vcap_buffer));

    return 0;
}

static int vcap_map_buffers(vcap_vd* vd)
{
    for (int i = 0; i < vd->buffer_count; i++)
    {
        struct v4l2_buffer buf;

        VCAP_CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == vcap_ioctl(vd->fd, VIDIOC_QUERYBUF, &buf))
        {
            VCAP_ERROR_ERRNO("Unable to query buffers on %s", vd->path);
            return -1;
        }

        vd->buffers[i].size = buf.length;
        vd->buffers[i].data = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, vd->fd, buf.m.offset);

        if (MAP_FAILED == vd->buffers[i].data)
        {
            VCAP_ERROR("MMAP failed on %s", vd->path);
            return -1;
        }
    }

    return 0;
}

static int vcap_unmap_buffers(vcap_vd* vd)
{
    for (int i = 0; i < vd->buffer_count; i++)
    {
        if (-1 == v4l2_munmap(vd->buffers[i].data, vd->buffers[i].size))
        {
            VCAP_ERROR_ERRNO("Unmapping buffers failed on %s", vd->path);
            return -1;
        }
    }

    vcap_free(vd->buffers);
    vd->buffer_count = 0;

    return 0;
}

static int vcap_queue_buffers(vcap_vd* vd)
{
    for (int i = 0; i < vd->buffer_count; i++)
    {
        struct v4l2_buffer buf;

        VCAP_CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == vcap_ioctl(vd->fd, VIDIOC_QBUF, &buf))
        {
            VCAP_ERROR_ERRNO("Unable to queue buffers on device %s", vd->path);
            return -1;
        }
	}

	return 0;
}

static int vcap_video_device_filter(const struct dirent *a)
{
    if (0 == strncmp(a->d_name, "video", 5))
        return 1;
    else
        return 0;
}
