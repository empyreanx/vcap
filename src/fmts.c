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

static int vcap_enum_fmts(vcap_dev* vd, vcap_fmt_info* info, uint32_t index);
static int vcap_enum_sizes(vcap_dev* vd, vcap_fmt_id fmt, vcap_size* size, uint32_t index);
static int vcap_enum_rates(vcap_dev* vd, vcap_fmt_id fmt, vcap_size size, vcap_rate* rate, uint32_t index);

int vcap_get_fmt_info(vcap_dev* vd, vcap_fmt_id fmt, vcap_fmt_info* info)
{
    assert(vd);

    if (!info)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return VCAP_FMT_ERROR;
    }

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

    if (!fmt || !size)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

    struct v4l2_format gfmt;

    VCAP_CLEAR(gfmt);
    gfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(vd->fd, VIDIOC_G_FMT, &gfmt))
    {
        vcap_set_error_errno(vd, "Unable to get format on device %s", vd->path);
        return -1;
    }

    *fmt = gfmt.fmt.pix.pixelformat;

    size->width = gfmt.fmt.pix.width;
    size->height = gfmt.fmt.pix.height;

    return 0;
}

int vcap_set_fmt(vcap_dev* vd, vcap_fmt_id fmt, vcap_size size)
{
    assert(vd);

    bool streaming = vcap_is_streaming(vd);

    if (vcap_is_open(vd) && vcap_close(vd) == -1)
        return -1;

    if (vcap_open(vd) == -1)
        return -1;

    struct v4l2_format sfmt;

    sfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    sfmt.fmt.pix.width = size.width;
    sfmt.fmt.pix.height = size.height;
    sfmt.fmt.pix.pixelformat = fmt;
    sfmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

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

    if (!rate)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

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

static int vcap_enum_fmts(vcap_dev* vd, vcap_fmt_info* info, uint32_t index)
{
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

