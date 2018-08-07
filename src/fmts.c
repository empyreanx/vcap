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

static uint32_t fmt_map[] = {
    /* RGB formats */
    V4L2_PIX_FMT_RGB332,
    V4L2_PIX_FMT_RGB444,
    V4L2_PIX_FMT_ARGB444,
    V4L2_PIX_FMT_XRGB444,
    V4L2_PIX_FMT_RGB555,
    V4L2_PIX_FMT_ARGB555,
    V4L2_PIX_FMT_XRGB555,
    V4L2_PIX_FMT_RGB565,
    V4L2_PIX_FMT_RGB555X,
    V4L2_PIX_FMT_ARGB555X,
    V4L2_PIX_FMT_XRGB555X,
    V4L2_PIX_FMT_RGB565X,
    V4L2_PIX_FMT_BGR666,
    V4L2_PIX_FMT_BGR24,
    V4L2_PIX_FMT_RGB24,
    V4L2_PIX_FMT_BGR32,
    V4L2_PIX_FMT_ABGR32,
    V4L2_PIX_FMT_XBGR32,
    V4L2_PIX_FMT_RGB32,
    V4L2_PIX_FMT_ARGB32,
    V4L2_PIX_FMT_XRGB32,

    /* Grey formats */
    V4L2_PIX_FMT_GREY,
    V4L2_PIX_FMT_Y4,
    V4L2_PIX_FMT_Y6,
    V4L2_PIX_FMT_Y10,
    V4L2_PIX_FMT_Y12,
    V4L2_PIX_FMT_Y16,
    V4L2_PIX_FMT_Y16_BE,

    /* Grey bit-packed formats */
    V4L2_PIX_FMT_Y10BPACK,

    /* Palette formats */
    V4L2_PIX_FMT_PAL8,

    /* Chrominance formats */
    V4L2_PIX_FMT_UV8,

    /* Luminance+Chrominance formats */
    V4L2_PIX_FMT_YUYV,
    V4L2_PIX_FMT_YYUV,
    V4L2_PIX_FMT_YVYU,
    V4L2_PIX_FMT_UYVY,
    V4L2_PIX_FMT_VYUY,
    V4L2_PIX_FMT_Y41P,
    V4L2_PIX_FMT_YUV444,
    V4L2_PIX_FMT_YUV555,
    V4L2_PIX_FMT_YUV565,
    V4L2_PIX_FMT_YUV32,
    V4L2_PIX_FMT_HI240,
    V4L2_PIX_FMT_HM12,
    V4L2_PIX_FMT_M420,

    /* two planes -- one Y, one Cr + Cb interleaved  */
    V4L2_PIX_FMT_NV12,
    V4L2_PIX_FMT_NV21,
    V4L2_PIX_FMT_NV16,
    V4L2_PIX_FMT_NV61,
    V4L2_PIX_FMT_NV24,
    V4L2_PIX_FMT_NV42,

    /* two non contiguous planes - one Y, one Cr + Cb interleaved  */
    V4L2_PIX_FMT_NV12M,
    V4L2_PIX_FMT_NV21M,
    V4L2_PIX_FMT_NV16M,
    V4L2_PIX_FMT_NV61M,
    V4L2_PIX_FMT_NV12MT,
    V4L2_PIX_FMT_NV12MT_16X16,

    /* three planes - Y Cb, Cr */
    V4L2_PIX_FMT_YUV410,
    V4L2_PIX_FMT_YVU410,
    V4L2_PIX_FMT_YUV411P,
    V4L2_PIX_FMT_YUV420,
    V4L2_PIX_FMT_YVU420,
    V4L2_PIX_FMT_YUV422P,

    /* three non contiguous planes - Y, Cb, Cr */
    V4L2_PIX_FMT_YUV420M,
    V4L2_PIX_FMT_YVU420M,
    V4L2_PIX_FMT_YUV422M,
    V4L2_PIX_FMT_YVU422M,
    V4L2_PIX_FMT_YUV444M,
    V4L2_PIX_FMT_YVU444M,

    /* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
    V4L2_PIX_FMT_SBGGR8,
    V4L2_PIX_FMT_SGBRG8,
    V4L2_PIX_FMT_SGRBG8,
    V4L2_PIX_FMT_SRGGB8,
    V4L2_PIX_FMT_SBGGR10,
    V4L2_PIX_FMT_SGBRG10,
    V4L2_PIX_FMT_SGRBG10,
    V4L2_PIX_FMT_SRGGB10,
            /* 10bit raw bayer packed, 5 bytes for every 4 pixels */
    V4L2_PIX_FMT_SBGGR10P,
    V4L2_PIX_FMT_SGBRG10P,
    V4L2_PIX_FMT_SGRBG10P,
    V4L2_PIX_FMT_SRGGB10P,
            /* 10bit raw bayer a-law compressed to 8 bits */
    V4L2_PIX_FMT_SBGGR10ALAW8,
    V4L2_PIX_FMT_SGBRG10ALAW8,
    V4L2_PIX_FMT_SGRBG10ALAW8,
    V4L2_PIX_FMT_SRGGB10ALAW8,
            /* 10bit raw bayer DPCM compressed to 8 bits */
    V4L2_PIX_FMT_SBGGR10DPCM8,
    V4L2_PIX_FMT_SGBRG10DPCM8,
    V4L2_PIX_FMT_SGRBG10DPCM8,
    V4L2_PIX_FMT_SRGGB10DPCM8,
    V4L2_PIX_FMT_SBGGR12,
    V4L2_PIX_FMT_SGBRG12,
    V4L2_PIX_FMT_SGRBG12,
    V4L2_PIX_FMT_SRGGB12,
            /* 12bit raw bayer packed, 6 bytes for every 4 pixels */
    V4L2_PIX_FMT_SBGGR12P,
    V4L2_PIX_FMT_SGBRG12P,
    V4L2_PIX_FMT_SGRBG12P,
    V4L2_PIX_FMT_SRGGB12P,
    V4L2_PIX_FMT_SBGGR16,
    V4L2_PIX_FMT_SGBRG16,
    V4L2_PIX_FMT_SGRBG16,
    V4L2_PIX_FMT_SRGGB16,

    /* HSV formats */
    V4L2_PIX_FMT_HSV24,
    V4L2_PIX_FMT_HSV32,

    /* compressed formats */
    V4L2_PIX_FMT_MJPEG,
    V4L2_PIX_FMT_JPEG,
    V4L2_PIX_FMT_DV,
    V4L2_PIX_FMT_MPEG,
    V4L2_PIX_FMT_H264,
    V4L2_PIX_FMT_H264_NO_SC,
    V4L2_PIX_FMT_H264_MVC,
    V4L2_PIX_FMT_H263,
    V4L2_PIX_FMT_MPEG1,
    V4L2_PIX_FMT_MPEG2,
    V4L2_PIX_FMT_MPEG4,
    V4L2_PIX_FMT_XVID,
    V4L2_PIX_FMT_VC1_ANNEX_G,
    V4L2_PIX_FMT_VC1_ANNEX_L,
    V4L2_PIX_FMT_VP8,
    V4L2_PIX_FMT_VP9,
    //V4L2_PIX_FMT_HEVC,
    //V4L2_PIX_FMT_FWHT,

    /*  Vendor-specific formats   */
    V4L2_PIX_FMT_CPIA1,
    V4L2_PIX_FMT_WNVA,
    V4L2_PIX_FMT_SN9C10X,
    V4L2_PIX_FMT_SN9C20X_I420,
    V4L2_PIX_FMT_PWC1,
    V4L2_PIX_FMT_PWC2,
    V4L2_PIX_FMT_ET61X251,
    V4L2_PIX_FMT_SPCA501,
    V4L2_PIX_FMT_SPCA505,
    V4L2_PIX_FMT_SPCA508,
    V4L2_PIX_FMT_SPCA561,
    V4L2_PIX_FMT_PAC207,
    V4L2_PIX_FMT_MR97310A,
    V4L2_PIX_FMT_JL2005BCD,
    V4L2_PIX_FMT_SN9C2028,
    V4L2_PIX_FMT_SQ905C,
    V4L2_PIX_FMT_PJPG,
    V4L2_PIX_FMT_OV511,
    V4L2_PIX_FMT_OV518,
    V4L2_PIX_FMT_STV0680,
    V4L2_PIX_FMT_TM6000,
    V4L2_PIX_FMT_CIT_YYVYUY,
    V4L2_PIX_FMT_KONICA420,
    V4L2_PIX_FMT_JPGL,
    V4L2_PIX_FMT_SE401,
    V4L2_PIX_FMT_S5C_UYVY_JPG,
    V4L2_PIX_FMT_Y8I,
    V4L2_PIX_FMT_Y12I,
    V4L2_PIX_FMT_Z16,
    V4L2_PIX_FMT_MT21C,
    V4L2_PIX_FMT_INZI,

    /* 10bit raw bayer packed, 32 bytes for every 25 pixels, last LSB 6 bits unused */
    /*V4L2_PIX_FMT_IPU3_SBGGR10,
    V4L2_PIX_FMT_IPU3_SGBRG10,
    V4L2_PIX_FMT_IPU3_SGRBG10,
    V4L2_PIX_FMT_IPU3_SRGGB10*/
};

static int enum_fmts(vcap_fg* fg, vcap_fmt_desc* desc, uint32_t index);
static int enum_sizes(vcap_fg* fg, vcap_fmt_id fid, vcap_size* size, uint32_t index);
static int enum_rates(vcap_fg* fg, vcap_fmt_id fid, vcap_size size, vcap_rate* rate, uint32_t index);

int vcap_get_fmt_desc(vcap_fg* fg, vcap_fmt_id fid, vcap_fmt_desc* desc) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return VCAP_ENUM_ERROR;
    }

    if (!desc) {
        VCAP_ERROR("Parameter 'desc' cannot be null");
        return VCAP_ENUM_ERROR;
    }

    int result, i = 0;

    do {
        result = enum_fmts(fg, desc, i);

        if (result == VCAP_ENUM_ERROR)
            return VCAP_FMT_ERROR;

        if (result == VCAP_ENUM_OK && desc->id == fid)
            return VCAP_FMT_OK;

    } while (result != VCAP_ENUM_INVALID && ++i);

    return VCAP_FMT_INVALID;
}

vcap_fmt_itr* vcap_new_fmt_itr(vcap_fg* fg) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return NULL;
    }

    vcap_fmt_itr* itr = vcap_malloc(sizeof(vcap_fmt_itr));

    if (!itr) {
        VCAP_ERROR_ERRNO("Out of memory allocating fmt iterator");
        return NULL;
    }

    itr->fg = fg;
    itr->index = 0;
    itr->result = enum_fmts(fg, &itr->desc, 0);

    return itr;
}

int vcap_fmt_itr_next(vcap_fmt_itr* itr, vcap_fmt_desc* desc) {
    if (!itr)
        return VCAP_FALSE;

    if (!desc) {
        VCAP_ERROR("Parameter 'desc' cannot be null");
        itr->result = VCAP_ENUM_ERROR;
        return VCAP_FALSE;
    }

    if (itr->result == VCAP_ENUM_INVALID || itr->result == VCAP_ENUM_ERROR)
        return VCAP_FALSE;

    *desc = itr->desc;

    itr->result = enum_fmts(itr->fg, &itr->desc, ++itr->index);

    return VCAP_TRUE;
}

int vcap_fmt_itr_error(vcap_fmt_itr* itr) {
    if (!itr) {
        VCAP_ERROR("Parameter 'itr' cannot be null");
        return VCAP_TRUE;
    }

    if (itr->result == VCAP_ENUM_ERROR)
        return VCAP_TRUE;
    else
        return VCAP_FALSE;
}

void vcap_free_fmt_itr(vcap_fmt_itr* itr) {
    if (itr)
        vcap_free(itr);
}

vcap_size_itr* vcap_new_size_itr(vcap_fg* fg, vcap_fmt_id fid) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return NULL;
    }

    if (fid < 0 || fid >= VCAP_FMT_UNKNOWN) {
        VCAP_ERROR("Invalid format (out of range)");
        return NULL;
    }

    vcap_size_itr* itr = vcap_malloc(sizeof(vcap_size_itr));

    if (!itr) {
        VCAP_ERROR_ERRNO("Out of memory allocating size iterator");
        return NULL;
    }

    itr->fg = fg;
    itr->fid = fid;
    itr->index = 0;
    itr->result = enum_sizes(fg, fid, &itr->size, 0);

    return itr;
}

int vcap_size_itr_next(vcap_size_itr* itr, vcap_size* size) {
    if (!itr)
        return VCAP_FALSE;

    if (!size) {
        VCAP_ERROR("Parameter 'size' cannot be null");
        itr->result = VCAP_ENUM_ERROR;
        return VCAP_FALSE;
    }

    if (itr->result == VCAP_ENUM_INVALID || itr->result == VCAP_ENUM_ERROR)
        return VCAP_FALSE;

    *size = itr->size;

    itr->result = enum_sizes(itr->fg, itr->fid, &itr->size, ++itr->index);

    return VCAP_TRUE;
}

int vcap_size_itr_error(vcap_size_itr* itr) {
    if (!itr) {
        VCAP_ERROR("Parameter 'itr' cannot be null");
        return VCAP_TRUE;
    }

    if (itr->result == VCAP_ENUM_ERROR)
        return VCAP_TRUE;
    else
        return VCAP_FALSE;
}

void vcap_free_size_itr(vcap_size_itr* itr) {
    if (itr)
        vcap_free(itr);
}

vcap_rate_itr* vcap_new_rate_itr(vcap_fg* fg, vcap_fmt_id fid, vcap_size size) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return NULL;
    }

    if (fid < 0 || fid >= VCAP_FMT_UNKNOWN) {
        VCAP_ERROR("Invalid format (out of range)");
        return NULL;
    }

    vcap_rate_itr* itr = vcap_malloc(sizeof(vcap_rate_itr));

    if (!itr) {
        VCAP_ERROR_ERRNO("Out of memory allocating rate iterator");
        return NULL;
    }

    itr->fg = fg;
    itr->fid = fid;
    itr->size = size;
    itr->index = 0;
    itr->result = enum_rates(fg, fid, size, &itr->rate, 0);

    return itr;
}

int vcap_rate_itr_next(vcap_rate_itr* itr, vcap_rate* rate) {
    if (!itr)
        return VCAP_FALSE;

    if (!rate) {
        VCAP_ERROR("Parameter 'rate' cannot be null");
        itr->result = VCAP_ENUM_ERROR;
        return VCAP_FALSE;
    }

    if (itr->result == VCAP_ENUM_INVALID || itr->result == VCAP_ENUM_ERROR)
        return VCAP_FALSE;

    *rate = itr->rate;

    itr->result = enum_rates(itr->fg, itr->fid, itr->size, &itr->rate, ++itr->index);

    return VCAP_TRUE;
}

int vcap_rate_itr_error(vcap_rate_itr* itr) {
    if (!itr) {
        VCAP_ERROR("Parameter 'itr' cannot be null");
        return VCAP_TRUE;
    }

    if (itr->result == VCAP_ENUM_ERROR)
        return VCAP_TRUE;
    else
        return VCAP_FALSE;
}

void vcap_free_rate_itr(vcap_rate_itr* itr) {
    if (itr)
        vcap_free(itr);
}

int vcap_get_fmt(vcap_fg* fg, vcap_fmt_id* fid, vcap_size* size) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    if (!fid) {
        VCAP_ERROR("Parameter 'fid' cannot be null");
        return -1;
    }

    if (!size) {
        VCAP_ERROR("Parameter 'size' cannot be null");
        return -1;
    }

    struct v4l2_format fmt;

    VCAP_CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(fg->fd, VIDIOC_G_FMT, &fmt)) {
        VCAP_ERROR_ERRNO("Unable to get format on device '%s'", fg->device.path);
        return -1;
    }

    *fid = vcap_convert_fmt_id(fmt.fmt.pix.pixelformat);

    size->width = fmt.fmt.pix.width;
    size->height = fmt.fmt.pix.height;

    return 0;
}

int vcap_set_fmt(vcap_fg* fg, vcap_fmt_id fid, vcap_size size) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    if (fid < 0 || fid >= VCAP_FMT_UNKNOWN) {
        VCAP_ERROR("Invalid format (out of range)");
        return -1;
    }

    struct v4l2_format fmt;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = size.width;
    fmt.fmt.pix.height = size.height;
    fmt.fmt.pix.pixelformat = fmt_map[fid];
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (vcap_ioctl(fg->fd, VIDIOC_S_FMT, &fmt) == -1) {
        VCAP_ERROR_ERRNO("Unable to set format on device '%s'", fg->device.path);
        return -1;
    }

    return 0;
}

int vcap_get_rate(vcap_fg* fg, vcap_rate* rate) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    if (!rate) {
        VCAP_ERROR("Parameter 'rate' cannot be null");
        return -1;
    }

    struct v4l2_streamparm parm;
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(fg->fd, VIDIOC_G_PARM, &parm) == -1) {
        VCAP_ERROR_ERRNO("Unable to get frame rate on device '%s'", fg->device.path);
        return -1;
    }

    // NOTE: We swap the numerator and denominator because Vcap uses frame rates
    // instead of intervals.
    rate->numerator = parm.parm.capture.timeperframe.denominator;
    rate->denominator = parm.parm.capture.timeperframe.numerator;

    return 0;
}

int vcap_set_rate(vcap_fg* fg, vcap_rate rate) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    struct v4l2_streamparm parm;

    // NOTE: We swap the numerator and denominator because Vcap uses frame rates
    // instead of intervals.
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = rate.denominator;
    parm.parm.capture.timeperframe.denominator = rate.numerator;

    if(vcap_ioctl(fg->fd, VIDIOC_S_PARM, &parm) == -1) {
        VCAP_ERROR_ERRNO("Unable to set framerate on device %s", fg->device.path);
        return -1;
    }

    return 0;
}

static int enum_fmts(vcap_fg* fg, vcap_fmt_desc* desc, uint32_t index) {
    struct v4l2_fmtdesc fmtd;

    VCAP_CLEAR(fmtd);
    fmtd.index = index;
    fmtd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (vcap_ioctl(fg->fd, VIDIOC_ENUM_FMT, &fmtd) == -1) {
        if (errno == EINVAL) {
            return VCAP_ENUM_INVALID;
        } else {
            VCAP_ERROR_ERRNO("Unable to enumerate formats on device '%s'", fg->device.path);
            return VCAP_ENUM_ERROR;
        }
    }

    // Copy name
    assert(sizeof(desc->name) == sizeof(fmtd.description));
    memcpy(desc->name, fmtd.description, sizeof(desc->name));

    // Convert FOURCC code
    vcap_fourcc_string(fmtd.pixelformat, desc->fourcc);

    // Copy pixel format
    desc->id = vcap_convert_fmt_id(fmtd.pixelformat);

    return VCAP_ENUM_OK;
}

static int enum_sizes(vcap_fg* fg, vcap_fmt_id fid, vcap_size* size, uint32_t index) {
    struct v4l2_frmsizeenum fenum;

    VCAP_CLEAR(fenum);
    fenum.index = index;
    fenum.pixel_format = fmt_map[fid];

    if (vcap_ioctl(fg->fd, VIDIOC_ENUM_FRAMESIZES, &fenum) == -1) {
        if (errno == EINVAL) {
            return VCAP_ENUM_INVALID;
        } else {
            VCAP_ERROR_ERRNO("Unable to enumerate sizes on device '%s'", fg->device.path);
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

static int enum_rates(vcap_fg* fg, vcap_fmt_id fid, vcap_size size, vcap_rate* rate, uint32_t index) {
    struct v4l2_frmivalenum frenum;

    VCAP_CLEAR(frenum);
    frenum.index = index;
    frenum.pixel_format = fmt_map[fid];
    frenum.width = size.width;
    frenum.height = size.height;

    if (vcap_ioctl(fg->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frenum) == -1) {
        if (errno == EINVAL) {
            return VCAP_ENUM_INVALID;
        } else {
            VCAP_ERROR_ERRNO("Unable to enumerate frame rates on device '%s'", fg->device.path);
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

vcap_ctrl_id vcap_convert_fmt_id(uint32_t id) {
    for (int i = 0; i < VCAP_FMT_UNKNOWN; i++) {
        if (fmt_map[i] == id)
            return (vcap_ctrl_id)i;
    }

    return VCAP_FMT_UNKNOWN;
}
