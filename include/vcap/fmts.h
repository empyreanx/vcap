/*! \file fmts.h */

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

#ifndef VCAP_FMTS_H
#define VCAP_FMTS_H

///
/// \brief Format IDs
///
typedef enum {

    /* RGB formats */
    VCAP_FMT_RGB332,    /*  8  RGB-3-3-2     */
    VCAP_FMT_RGB444,    /* 16  xxxxrrrr ggggbbbb */
    VCAP_FMT_ARGB444,   /* 16  aaaarrrr ggggbbbb */
    VCAP_FMT_XRGB444,   /* 16  xxxxrrrr ggggbbbb */
    VCAP_FMT_RGB555,    /* 16  RGB-5-5-5     */
    VCAP_FMT_ARGB555,   /* 16  ARGB-1-5-5-5  */
    VCAP_FMT_XRGB555,   /* 16  XRGB-1-5-5-5  */
    VCAP_FMT_RGB565,    /* 16  RGB-5-6-5     */
    VCAP_FMT_RGB555X,   /* 16  RGB-5-5-5 BE  */
    VCAP_FMT_ARGB555X,  /* 16  ARGB-5-5-5 BE */
    VCAP_FMT_XRGB555X,  /* 16  XRGB-5-5-5 BE */
    VCAP_FMT_RGB565X,   /* 16  RGB-5-6-5 BE  */
    VCAP_FMT_BGR666,    /* 18  BGR-6-6-6     */
    VCAP_FMT_BGR24,     /* 24  BGR-8-8-8     */
    VCAP_FMT_RGB24,     /* 24  RGB-8-8-8     */
    VCAP_FMT_BGR32,     /* 32  BGR-8-8-8-8   */
    VCAP_FMT_ABGR32,    /* 32  BGRA-8-8-8-8  */
    VCAP_FMT_XBGR32,    /* 32  BGRX-8-8-8-8  */
    VCAP_FMT_RGB32,     /* 32  RGB-8-8-8-8   */
    VCAP_FMT_ARGB32,    /* 32  ARGB-8-8-8-8  */
    VCAP_FMT_XRGB32,    /* 32  XRGB-8-8-8-8  */

    /* Grey formats */
    VCAP_FMT_GREY,      /*  8  Greyscale     */
    VCAP_FMT_Y4,        /*  4  Greyscale     */
    VCAP_FMT_Y6,        /*  6  Greyscale     */
    VCAP_FMT_Y10,       /* 10  Greyscale     */
    VCAP_FMT_Y12,       /* 12  Greyscale     */
    VCAP_FMT_Y16,       /* 16  Greyscale     */
    VCAP_FMT_Y16_BE,    /* 16  Greyscale BE  */

    /* Grey bit-packed formats */
    VCAP_FMT_Y10BPACK,    /* 10  Greyscale bit-packed */

    /* Palette formats */
    VCAP_FMT_PAL8,        /*  8  8-bit palette */

    /* Chrominance formats */
    VCAP_FMT_UV8,         /*  8  UV 4:4 */

    /* Luminance+Chrominance formats */
    VCAP_FMT_YUYV,    /* 16  YUV 4:2:2     */
    VCAP_FMT_YYUV,    /* 16  YUV 4:2:2     */
    VCAP_FMT_YVYU,    /* 16 YVU 4:2:2 */
    VCAP_FMT_UYVY,    /* 16  YUV 4:2:2     */
    VCAP_FMT_VYUY,    /* 16  YUV 4:2:2     */
    VCAP_FMT_Y41P,    /* 12  YUV 4:1:1     */
    VCAP_FMT_YUV444,  /* 16  xxxxyyyy uuuuvvvv */
    VCAP_FMT_YUV555,  /* 16  YUV-5-5-5     */
    VCAP_FMT_YUV565,  /* 16  YUV-5-6-5     */
    VCAP_FMT_YUV32,   /* 32  YUV-8-8-8-8   */
    VCAP_FMT_HI240,   /*  8  8-bit color   */
    VCAP_FMT_HM12,    /*  8  YUV 4:2:0 16x16 macroblocks */
    VCAP_FMT_M420,    /* 12  YUV 4:2:0 2 lines y, 1 line uv interleaved */

    /* two planes -- one Y, one Cr + Cb interleaved  */
    VCAP_FMT_NV12,    /* 12  Y/CbCr 4:2:0  */
    VCAP_FMT_NV21,    /* 12  Y/CrCb 4:2:0  */
    VCAP_FMT_NV16,    /* 16  Y/CbCr 4:2:2  */
    VCAP_FMT_NV61,    /* 16  Y/CrCb 4:2:2  */
    VCAP_FMT_NV24,    /* 24  Y/CbCr 4:4:4  */
    VCAP_FMT_NV42,    /* 24  Y/CrCb 4:4:4  */

    /* two non contiguous planes - one Y, one Cr + Cb interleaved  */
    VCAP_FMT_NV12M,         /* 12  Y/CbCr 4:2:0  */
    VCAP_FMT_NV21M,         /* 21  Y/CrCb 4:2:0  */
    VCAP_FMT_NV16M,         /* 16  Y/CbCr 4:2:2  */
    VCAP_FMT_NV61M,         /* 16  Y/CrCb 4:2:2  */
    VCAP_FMT_NV12MT,        /* 12  Y/CbCr 4:2:0 64x32 macroblocks */
    VCAP_FMT_NV12MT_16X16,  /* 12  Y/CbCr 4:2:0 16x16 macroblocks */

    /* three planes - Y Cb, Cr */
    VCAP_FMT_YUV410,  /*  9  YUV 4:1:0     */
    VCAP_FMT_YVU410,  /*  9  YVU 4:1:0     */
    VCAP_FMT_YUV411P, /* 12  YVU411 planar */
    VCAP_FMT_YUV420,  /* 12  YUV 4:2:0     */
    VCAP_FMT_YVU420,  /* 12  YVU 4:2:0     */
    VCAP_FMT_YUV422P, /* 16  YVU422 planar */

    /* three non contiguous planes - Y, Cb, Cr */
    VCAP_FMT_YUV420M, /* 12  YUV420 planar */
    VCAP_FMT_YVU420M, /* 12  YVU420 planar */
    VCAP_FMT_YUV422M, /* 16  YUV422 planar */
    VCAP_FMT_YVU422M, /* 16  YVU422 planar */
    VCAP_FMT_YUV444M, /* 24  YUV444 planar */
    VCAP_FMT_YVU444M, /* 24  YVU444 planar */

    /* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
    VCAP_FMT_SBGGR8,  /*  8  BGBG.. GRGR.. */
    VCAP_FMT_SGBRG8,  /*  8  GBGB.. RGRG.. */
    VCAP_FMT_SGRBG8,  /*  8  GRGR.. BGBG.. */
    VCAP_FMT_SRGGB8,  /*  8  RGRG.. GBGB.. */
    VCAP_FMT_SBGGR10, /* 10  BGBG.. GRGR.. */
    VCAP_FMT_SGBRG10, /* 10  GBGB.. RGRG.. */
    VCAP_FMT_SGRBG10, /* 10  GRGR.. BGBG.. */
    VCAP_FMT_SRGGB10, /* 10  RGRG.. GBGB.. */
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
    VCAP_FMT_SBGGR12, /* 12  BGBG.. GRGR.. */
    VCAP_FMT_SGBRG12, /* 12  GBGB.. RGRG.. */
    VCAP_FMT_SGRBG12, /* 12  GRGR.. BGBG.. */
    VCAP_FMT_SRGGB12, /* 12  RGRG.. GBGB.. */
            /* 12bit raw bayer packed, 6 bytes for every 4 pixels */
    VCAP_FMT_SBGGR12P,
    VCAP_FMT_SGBRG12P,
    VCAP_FMT_SGRBG12P,
    VCAP_FMT_SRGGB12P,
    VCAP_FMT_SBGGR16, /* 16  BGBG.. GRGR.. */
    VCAP_FMT_SGBRG16, /* 16  GBGB.. RGRG.. */
    VCAP_FMT_SGRBG16, /* 16  GRGR.. BGBG.. */
    VCAP_FMT_SRGGB16, /* 16  RGRG.. GBGB.. */

    /* HSV formats */
    VCAP_FMT_HSV24,
    VCAP_FMT_HSV32,

    /* compressed formats */
    VCAP_FMT_MJPEG,      /* Motion-JPEG   */
    VCAP_FMT_JPEG,        /* JFIF JPEG     */
    VCAP_FMT_DV,          /* 1394          */
    VCAP_FMT_MPEG,        /* MPEG-1/2/4 Multiplexed */
    VCAP_FMT_H264,        /* H264 with start codes */
    VCAP_FMT_H264_NO_SC,  /* H264 without start codes */
    VCAP_FMT_H264_MVC,    /* H264 MVC */
    VCAP_FMT_H263,        /* H263          */
    VCAP_FMT_MPEG1,       /* MPEG-1 ES     */
    VCAP_FMT_MPEG2,       /* MPEG-2 ES     */
    VCAP_FMT_MPEG4,       /* MPEG-4 part 2 ES */
    VCAP_FMT_XVID,        /* Xvid           */
    VCAP_FMT_VC1_ANNEX_G, /* SMPTE 421M Annex G compliant stream */
    VCAP_FMT_VC1_ANNEX_L, /* SMPTE 421M Annex L compliant stream */
    VCAP_FMT_VP8,         /* VP8 */
    VCAP_FMT_VP9,         /* VP9 */
    //VCAP_FMT_HEVC,        /* HEVC aka H.265 */
    //VCAP_FMT_FWHT,        /* Fast Walsh Hadamard Transform (vicodec) */

    /*  Vendor-specific formats   */
    VCAP_FMT_CPIA1,         /* cpia1 YUV */
    VCAP_FMT_WNVA,          /* Winnov hw compress */
    VCAP_FMT_SN9C10X,       /* SN9C10x compression */
    VCAP_FMT_SN9C20X_I420,  /* SN9C20x YUV 4:2:0 */
    VCAP_FMT_PWC1,          /* pwc older webcam */
    VCAP_FMT_PWC2,          /* pwc newer webcam */
    VCAP_FMT_ET61X251,      /* ET61X251 compression */
    VCAP_FMT_SPCA501,       /* YUYV per line */
    VCAP_FMT_SPCA505,       /* YYUV per line */
    VCAP_FMT_SPCA508,       /* YUVY per line */
    VCAP_FMT_SPCA561,       /* compressed GBRG bayer */
    VCAP_FMT_PAC207,        /* compressed BGGR bayer */
    VCAP_FMT_MR97310A,      /* compressed BGGR bayer */
    VCAP_FMT_JL2005BCD,     /* compressed RGGB bayer */
    VCAP_FMT_SN9C2028,      /* compressed GBRG bayer */
    VCAP_FMT_SQ905C,        /* compressed RGGB bayer */
    VCAP_FMT_PJPG,          /* Pixart 73xx JPEG */
    VCAP_FMT_OV511,         /* ov511 JPEG */
    VCAP_FMT_OV518,         /* ov518 JPEG */
    VCAP_FMT_STV0680,       /* stv0680 bayer */
    VCAP_FMT_TM6000,        /* tm5600/tm60x0 */
    VCAP_FMT_CIT_YYVYUY,    /* one line of Y then 1 line of VYUY */
    VCAP_FMT_KONICA420,     /* YUV420 planar in blocks of 256 pixels */
    VCAP_FMT_JPGL,          /* JPEG-Lite */
    VCAP_FMT_SE401,         /* se401 janggu compressed rgb */
    VCAP_FMT_S5C_UYVY_JPG,  /* S5C73M3 interleaved UYVY/JPEG */
    VCAP_FMT_Y8I,           /* Greyscale 8-bit L/R interleaved */
    VCAP_FMT_Y12I,          /* Greyscale 12-bit L/R interleaved */
    VCAP_FMT_Z16,           /* Depth data 16-bit */
    VCAP_FMT_MT21C,         /* Mediatek compressed block mode  */
    VCAP_FMT_INZI,          /* Intel Planar Greyscale 10-bit and Depth 16-bit */

    /* 10bit raw bayer packed, 32 bytes for every 25 pixels, last LSB 6 bits unused */
    //VCAP_FMT_IPU3_SBGGR10,       /* IPU3 packed 10-bit BGGR bayer */
    //VCAP_FMT_IPU3_SGBRG10,       /* IPU3 packed 10-bit GBRG bayer */
    //VCAP_FMT_IPU3_SGRBG10,       /* IPU3 packed 10-bit GRBG bayer */
    //VCAP_FMT_IPU3_SRGGB10,       /* IPU3 packed 10-bit RGGB bayer */

    VCAP_FMT_UNKNOWN
} vcap_fmt_id;

#endif // VCAP_FMTS_H
