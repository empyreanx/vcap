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

static int vcap_request_buffers(vcap_dev* vd, int buffer_count);
static int vcap_init_stream(vcap_dev* vd);
static int vcap_shutdown_stream(vcap_dev* vd);
static int vcap_map_buffers(vcap_dev* vd);
static int vcap_unmap_buffers(vcap_dev* vd);
static int vcap_queue_buffers(vcap_dev* vd);

// Filters device list so that 'scandir' returns only video devices.
static int vcap_video_device_filter(const struct dirent *a);

static vcap_malloc_fn global_malloc_fp = malloc;
static vcap_free_fn global_free_fp = free;

void vcap_set_alloc(vcap_malloc_fn malloc_fp, vcap_free_fn free_fp)
{
    global_malloc_fp = malloc_fp;
    global_free_fp = free_fp;
}

static void* vcap_malloc(size_t size)
{
    return global_malloc_fp(size);
}

static void vcap_free(void* ptr)
{
    global_free_fp(ptr);
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
    printf("------------------------------------------------\n");
    fprintf(file, "Device: %s\n", info.path);
    fprintf(file, "Driver: %s\n", info.driver);
    fprintf(file, "Driver version: %s\n", info.version_str);
    fprintf(file, "Card: %s\n", info.card);
    fprintf(file, "Bus Info: %s\n", info.bus_info);

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

        if (ctrl_info.type == VCAP_CTRL_TYPE_MENU || ctrl_info.type == VCAP_CTRL_TYPE_INTEGER_MENU)
        {
            printf("   Menu:\n");

            //==================================================================
            // Enumerate menu
            //==================================================================
            vcap_menu_item menu_item;
            vcap_menu_itr menu_itr = vcap_new_menu_itr(vd, ctrl_info.id);

            while (vcap_menu_itr_next(&menu_itr, &menu_item))
            {
                if (ctrl_info.type == VCAP_CTRL_TYPE_MENU)
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
        vcap_set_global_error("Querying video device %s capabilities failed", path);
        v4l2_close(fd);
        return -1;
    }

    // Ensure video capture is supported
    if (!(caps->capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        vcap_set_global_error("Video device %s does not support video capture", path);
        v4l2_close(fd);
        return -1;
    }

    v4l2_close(fd);

    return 0;
}

void vcap_caps_to_info(const char* path, const struct v4l2_capability caps, vcap_dev_info* info)
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
}

int vcap_enum_devices(unsigned index, vcap_dev_info* info)
{
    int count = 0;

    struct dirent **names;
    int n = scandir("/dev", &names, vcap_video_device_filter, alphasort);

    if (n < 0)
    {
        vcap_set_global_error("Failed to scan '/dev' directory");
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

vcap_dev* vcap_create_device(const char* path, bool convert, int buffer_count)
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
    if (-1 == stat(vd->path, &st))
    {
        vcap_set_error_errno(vd, "Video device %s does not exist", vd->path);
        return -1;
    }

    // Device must be a character device
    if (!S_ISCHR(st.st_mode))
    {
        vcap_set_error_errno(vd, "Video device %s is not a character device", vd->path);
        return -1;
    }

    // Open the video device
    vd->fd = v4l2_open(vd->path, O_RDWR | O_NONBLOCK, 0);

    if (-1 == vd->fd)
    {
        vcap_set_error_errno(vd, "Opening video device %s failed", vd->path);
        return -1;
    }

    // Ensure child processes dont't inherit the video device
    fcntl(vd->fd, F_SETFD, FD_CLOEXEC);

    // Obtain device capabilities
    if (-1 == vcap_ioctl(vd->fd, VIDIOC_QUERYCAP, &caps))
    {
        vcap_set_error_errno(vd, "Querying video device %s capabilities failed", vd->path);
        v4l2_close(vd->fd);
        return -1;
    }

    // Ensure video capture is supported
    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        vcap_set_error(vd, "Video device %s does not support video capture", vd->path);
        v4l2_close(vd->fd);
        return -1;
    }

    // Check I/O capabilities
    if (vd->buffer_count > 0)
    {
        // Ensure streaming is supported
        if (!(caps.capabilities & V4L2_CAP_STREAMING))
        {
            vcap_set_error(vd, "Video device %s does not support streaming", vd->path);
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
        if (vcap_is_streaming(vd) && -1 == vcap_stop_stream(vd))
            return -1;
    }

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
        vcap_set_error(vd, "Unable to start stream on %s, device is already streaming", vd->path);
        return -1;
    }

    if (vd->buffer_count > 0)
    {
        if (-1 == vcap_init_stream(vd))
            return -1;

    	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == vcap_ioctl(vd->fd, VIDIOC_STREAMON, &type))
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
        vcap_set_error(vd, "Unable to close stream on %s, device is not streaming", vd->path);
        return -1;
    }

    if (vd->buffer_count > 0)
    {
    	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == vcap_ioctl(vd->fd, VIDIOC_STREAMOFF, &type))
        {
            vcap_set_error_errno(vd, "Unable to stop stream on %s", vd->path);
            return -1;
        }

        if (-1 == vcap_shutdown_stream(vd))
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

    if (!info)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

    vcap_caps_to_info(vd->path, vd->caps, info);

    return 0;
}

size_t vcap_get_buffer_size(vcap_dev* vd)
{
    assert(vd);

    struct v4l2_format fmt;

    VCAP_CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_G_FMT, &fmt))
    {
        vcap_set_error_errno(vd, "Unable to get format on device %s", vd->path);
        return 0;
    }

    return fmt.fmt.pix.sizeimage;
}

int vcap_grab_mmap(vcap_dev* vd, size_t buffer_size, uint8_t* buffer)
{
    assert(vd);

    if (!buffer)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

    struct v4l2_buffer buf;

	//dequeue buffer
    VCAP_CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == vcap_ioctl(vd->fd, VIDIOC_DQBUF, &buf))
    {
        vcap_set_error_errno(vd, "Could not dequeue buffer on %s", vd->path);
        return -1;
    }

    memcpy(buffer, vd->buffers[buf.index].data, buffer_size);

    if (-1 == vcap_ioctl(vd->fd, VIDIOC_QBUF, &buf)) {
        vcap_set_error_errno(vd, "Could not requeue buffer on %s", vd->path);
        return -1;
    }

    return 0;
}

int vcap_grab_read(vcap_dev* vd, size_t buffer_size, uint8_t* buffer)
{
    assert(vd);

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

int vcap_grab(vcap_dev* vd, size_t buffer_size, uint8_t* buffer)
{
    assert(vd);

    if (!buffer)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

    if (vd->buffer_count > 0)
        return vcap_grab_mmap(vd, buffer_size, buffer);
    else
        return vcap_grab_read(vd, buffer_size, buffer);
}

int vcap_get_crop_bounds(vcap_dev* vd, vcap_rect* rect)
{
    assert(vd);

    if (!rect)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

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

    rect->top = cropcap.bounds.top;
    rect->left = cropcap.bounds.left;
    rect->width = cropcap.bounds.width;
    rect->height = cropcap.bounds.height;

    return 0;
}

int vcap_reset_crop(vcap_dev* vd)
{
    assert(vd);

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

    if (!rect)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

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

static int vcap_request_buffers(vcap_dev* vd, int buffer_count)
{
    assert(vd);

    struct v4l2_requestbuffers req;

    VCAP_CLEAR(req);
    req.count = buffer_count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

	if (-1 == vcap_ioctl(vd->fd, VIDIOC_REQBUFS, &req))
	{
    	vcap_set_error_errno(vd, "Unable to request buffers on %s", vd->path);
		return -1;
    }

    if (0 == req.count)
    {
        vcap_set_error(vd, "Invalid buffer count on %s", vd->path);
        return -1;
    }

    vd->buffer_count = req.count;
    vd->buffers = vcap_malloc(req.count * sizeof(vcap_buffer));

    return 0;
}

static int vcap_init_stream(vcap_dev* vd)
{
    if (vd->buffer_count > 0)
    {
        if (-1 == vcap_request_buffers(vd, vd->buffer_count))
            return -1;

        if (-1 == vcap_map_buffers(vd))
            return -1;

        if (-1 == vcap_queue_buffers(vd))
            return -1;
    }

    return 0;
}

static int vcap_shutdown_stream(vcap_dev* vd)
{
    assert(vd);

    if (vd->buffer_count > 0)
    {
        if (-1 == vcap_unmap_buffers(vd))
            return -1;
    }

    return 0;
}

static int vcap_map_buffers(vcap_dev* vd)
{
    assert(vd);

     for (int i = 0; i < vd->buffer_count; i++)
    {
        struct v4l2_buffer buf;

        VCAP_CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == vcap_ioctl(vd->fd, VIDIOC_QUERYBUF, &buf))
        {
            vcap_set_error_errno(vd, "Unable to query buffers on %s", vd->path);
            return -1;
        }

        vd->buffers[i].size = buf.length;
        vd->buffers[i].data = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, vd->fd, buf.m.offset);

        if (MAP_FAILED == vd->buffers[i].data)
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

    for (int i = 0; i < vd->buffer_count; i++)
    {
        if (-1 == v4l2_munmap(vd->buffers[i].data, vd->buffers[i].size))
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
    for (int i = 0; i < vd->buffer_count; i++)
    {
        struct v4l2_buffer buf;

        VCAP_CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == vcap_ioctl(vd->fd, VIDIOC_QBUF, &buf))
        {
            vcap_set_error_errno(vd, "Unable to queue buffers on device %s", vd->path);
            return -1;
        }
	}

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
