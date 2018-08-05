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

void vcap_set_alloc(vcap_malloc_func malloc_func, vcap_free_func free_func) {
    vcap_malloc = malloc_func;
    vcap_free = free_func;
}

const char* vcap_get_error() {
    return vcap_error_msg;
}

int vcap_dump_info(vcap_fg* fg) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    vcap_device device = fg->device;

    // Print device info
    printf("------------------------------------------------\n");
    printf("Device: %s\n", device.path);
    printf("Driver: %s\n", device.driver);
    printf("Driver version: %s\n", device.version_str);
    printf("Card: %s\n", device.card);
    printf("Bus Info: %s\n", device.bus_info);

    // Enumerate formats
    vcap_fmt_desc fmt_desc;
    vcap_fmt_itr* fmt_itr = vcap_new_fmt_itr(fg);

    while (vcap_fmt_itr_next(fmt_itr, &fmt_desc)) {
        printf("------------------------------------------------\n");
        printf("Format: %s, FourCC: %s\n", fmt_desc.name, fmt_desc.fourcc);
        printf("Sizes:\n");

        // Enumerate sizes
        vcap_size size;
        vcap_size_itr* size_itr = vcap_new_size_itr(fg, fmt_desc.id);

        while (vcap_size_itr_next(size_itr, &size)) {
            printf("   %u x %u: ", size.width, size.height);
            printf("(Frame rates:");

            // Enumerate frame rates
            vcap_rate rate;
            vcap_rate_itr* rate_itr = vcap_new_rate_itr(fg, fmt_desc.id, size);

            while (vcap_rate_itr_next(rate_itr, &rate)) {
                printf(" %u/%u", rate.numerator, rate.denominator);
            }

            if (vcap_rate_itr_error(rate_itr))
                return -1;

            vcap_free_rate_itr(rate_itr);

            printf(")\n");
        }

        if (vcap_size_itr_error(size_itr))
            return -1;

        vcap_free_size_itr(size_itr);
    }

    if (vcap_fmt_itr_error(fmt_itr))
        return -1;

    vcap_free_fmt_itr(fmt_itr);

    // Enumerate controls
    printf("------------------------------------------------\n");
    printf("Controls:\n");

    vcap_ctrl_desc ctrl_desc;
    vcap_ctrl_itr* ctrl_itr = vcap_new_ctrl_itr(fg);

    while (vcap_ctrl_itr_next(ctrl_itr, &ctrl_desc)) {
        printf("   Name: %s, Type: %s\n", ctrl_desc.name, ctrl_desc.type_name);

        if (ctrl_desc.type == VCAP_CTRL_TYPE_MENU || ctrl_desc.type == VCAP_CTRL_TYPE_INTEGER_MENU) {
            printf("   Menu:\n");

            // Enumerate menu
            vcap_menu_item menu_item;
            vcap_menu_itr* menu_itr = vcap_new_menu_itr(fg, ctrl_desc.id);

            while (vcap_menu_itr_next(menu_itr, &menu_item)) {
                if (ctrl_desc.type == VCAP_CTRL_TYPE_MENU)
                    printf("      %i : %s\n", menu_item.index, menu_item.name);
                else
                    printf("      %i : %li\n", menu_item.index, menu_item.value);
            }

            if (vcap_menu_itr_error(menu_itr))
                return -1;

            vcap_free_menu_itr(menu_itr);
        }
    }

    if (vcap_ctrl_itr_error(ctrl_itr))
        return -1;

    vcap_free_ctrl_itr(ctrl_itr);

    return 0;
}

int vcap_enum_devices(vcap_device* device, int index) {
    if (!device) {
        VCAP_ERROR("Parameter 'device' cannot be null");
        return VCAP_ENUM_ERROR;
    }

    int count = 0;

    struct dirent **names;
    int n = scandir("/dev", &names, video_device_filter, alphasort);

    if (n < 0) {
        VCAP_ERROR("Failed to scan '/dev' directory");
        return VCAP_ENUM_ERROR;
    }

    char path[512];

    for (int i = 0; i < n; i++) {
        snprintf(path, sizeof(path), "/dev/%s", names[i]->d_name);

        if (vcap_try_get_device(path, device) == 0) {
            if (index == count) {
                free(names);
                return VCAP_ENUM_OK;
            } else {
                count++;
            }
        }
    }

    free(names);

    return VCAP_ENUM_INVALID;
}

int vcap_get_device(const char* path, vcap_device* device) {
    if (!path) {
        VCAP_ERROR("Parameter 'path' cannot be null");
        return -1;
    }

    if (!device) {
        VCAP_ERROR("Parameter 'device' cannot be null");
        return -1;
    }

    if (vcap_try_get_device(path, device) == -1) {
        VCAP_ERROR("Invalid device '%s'", path);
        return -1;
    }

    return 0;
}

vcap_fg* vcap_open(vcap_device* device) {
    if (!device) {
        VCAP_ERROR("Parameter 'device' cannot be null");
        return NULL;
    }

    struct stat st;
    struct v4l2_capability caps;

    vcap_fg* fg = vcap_malloc(sizeof(struct vcap_fg));

    // Allocate frame grabber
    if (!fg) {
        VCAP_ERROR_ERRNO("Ran out of memory allocating frame grabber");
        return NULL;
    }

    fg->fd = -1;
    fg->buffers = NULL;
    fg->buffer_count = 0;

    // Device must exist
    if (stat(device->path, &st) == -1) {
        VCAP_ERROR_ERRNO("Video device '%s' does not exist", device->path);
        goto error;
    }

    // Device must be a character device
    if (!S_ISCHR(st.st_mode)) {
        VCAP_ERROR_ERRNO("Video device '%s' is not a character device", device->path);
        goto error;
    }

    // Open the video device
    fg->fd = v4l2_open(device->path, O_RDWR | O_NONBLOCK, 0);

    if (fg->fd == -1) {
        VCAP_ERROR_ERRNO("Opening video device '%s' failed", device->path);
        goto error;
    }

    // Ensure child processes should't inherit the video device
    fcntl(fg->fd, F_SETFD, FD_CLOEXEC);

    // Obtain device capabilities
    if (vcap_ioctl(fg->fd, VIDIOC_QUERYCAP, &caps) == -1) {
        VCAP_ERROR_ERRNO("Querying video device '%s' capabilities failed", device->path);
        goto error;
    }

    // Ensure video capture is supported
    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        VCAP_ERROR("Video device '%s' does not support video capture", device->path);
        goto error;
    }

    fg->device = *device;

    return fg;

error:
    vcap_close(fg);
    return NULL;
}

void vcap_close(vcap_fg* fg) {
    if (!fg)
        return;

    if (fg->fd != -1)
        v4l2_close(fg->fd);

    vcap_free(fg);
}

vcap_frame* vcap_alloc_frame(vcap_fg* fg) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return NULL;
    }

    vcap_frame* frame = vcap_malloc(sizeof(vcap_frame));

    if (!frame) {
        VCAP_ERROR_ERRNO("Ran out of memory allocating frame");
        return NULL;
    }

    struct v4l2_format fmt;

    VCAP_CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(fg->fd, VIDIOC_G_FMT, &fmt)) {
        VCAP_ERROR_ERRNO("Unable to get format on device '%s'", fg->device.path);
        goto error;
    }

    frame->size.width = fmt.fmt.pix.width;
    frame->size.height = fmt.fmt.pix.height;
    frame->stride = fmt.fmt.pix.bytesperline;
    frame->length = fmt.fmt.pix.sizeimage;
    frame->data = vcap_malloc(frame->length);

    if (!frame->data) {
        VCAP_ERROR_ERRNO("Ran out of memory allocating frame data");
        goto error;
    }

    return frame;

error:

    vcap_free(frame);
    return NULL;
}

void vcap_free_frame(vcap_frame* frame) {
    if (!frame)
        return;

    vcap_free(frame->data);
    vcap_free(frame);
}

int vcap_grab(vcap_fg* fg, vcap_frame* frame) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    if (!frame) {
        VCAP_ERROR("Parameter 'frame' cannot be null");
        return -1;
    }

    fd_set fds;
    int result;
    struct timeval tv;

    while (1) {
        do {
            FD_ZERO(&fds);
            FD_SET(fg->fd, &fds);

            // Timeout
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            result = select(fg->fd + 1, &fds, NULL, NULL, &tv);
        } while (result == -1 && errno == EINTR);

        if (result == -1) {
            VCAP_ERROR("Unable to grab frame on device '%s'", fg->device.path);
            return -1;
        }

        if (result == 0) {
            VCAP_ERROR("Unable to grab frame (timeout expired) on device '%s'", fg->device.path);
            return -1;
        }

        if (v4l2_read(fg->fd, frame->data, frame->length) == -1) {
            if (errno == EAGAIN) {
                continue;
            } else {
                VCAP_ERROR_ERRNO("Reading from device '%s' failed", fg->device.path);
                return -1;
            }
        }

        return 0; // Break out of loop
    }
}

int vcap_get_crop_bounds(vcap_fg* fg, vcap_rect* rect) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    if (!rect) {
        VCAP_ERROR("Parameter 'rect' cannot be null");
        return -1;
    }

    struct v4l2_cropcap cropcap;

    VCAP_CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(fg->fd, VIDIOC_CROPCAP, &cropcap) == -1) {
        if (errno == ENODATA || errno == EINVAL) {
            VCAP_ERROR("Cropping is not supported on device '%s'", fg->device.path);
            return -1;
        }
    }

    rect->top = cropcap.bounds.top;
    rect->left = cropcap.bounds.left;
    rect->width = cropcap.bounds.width;
    rect->height = cropcap.bounds.height;

    return 0;
}

int vcap_reset_crop(vcap_fg* fg) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    struct v4l2_cropcap cropcap;

    VCAP_CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(fg->fd, VIDIOC_CROPCAP, &cropcap) == -1) {
        if (errno == ENODATA || errno == EINVAL) {
            VCAP_ERROR("Cropping is not supported on device '%s'", fg->device.path);
            return -1;
        }
    }

    struct v4l2_crop crop;

    VCAP_CLEAR(crop);
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect;

    if (vcap_ioctl(fg->fd, VIDIOC_S_CROP, &crop) == -1) {
        VCAP_ERROR_ERRNO("Unable to set crop window on device '%s'", fg->device.path);
        return -1;
    }

    return 0;
}

int vcap_get_crop(vcap_fg* fg, vcap_rect* rect) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    if (!rect) {
        VCAP_ERROR("Parameter 'rect' cannot be null");
        return -1;
    }

    struct v4l2_crop crop;

    VCAP_CLEAR(crop);
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(fg->fd, VIDIOC_G_CROP, &crop) == -1) {
        if (errno == ENODATA) {
            VCAP_ERROR("Cropping is not supported on device '%s'", fg->device.path);
            return -1;
        } else {
            VCAP_ERROR_ERRNO("Unable to get crop window on device '%s'", fg->device.path);
            return -1;
        }
    }

    rect->left = crop.c.left;
    rect->top = crop.c.top;
    rect->width = crop.c.width;
    rect->height = crop.c.height;

    return 0;
}

int vcap_set_crop(vcap_fg* fg, vcap_rect rect) {
    if (!fg) {
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

    if (vcap_ioctl(fg->fd, VIDIOC_S_CROP, &crop) == -1) {
        if (errno == ENODATA) {
            VCAP_ERROR("Cropping is not supported on device '%s'", fg->device.path);
            return -1;
        } else {
            VCAP_ERROR_ERRNO("Unable to set crop window on device '%s'", fg->device.path);
            return -1;
        }
    }

    return 0;
}

static int video_device_filter(const struct dirent *a) {
    if (0 == strncmp(a->d_name, "video", 5))
        return 1;
    else
        return 0;
}
