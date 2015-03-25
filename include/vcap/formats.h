/*
 * Vcap - a Video4Linux2 capture library
 * 
 * Copyright (C) 2014 James McLean
 * 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _VCAP_FORMATS_H
#define _VCAP_FORMATS_H

#include <stdint.h>

/**
 * \file 
 * \brief FOURCC pixel format codes.
 */

/**
 * \brief Converts a FOURCC character code to a string.
 */
void vcap_fourcc_str(uint32_t code, char* str);

/**
 * \brief Converts a FOURCC code to it's numerical representation.
 */
#define VCAP_FOURCC(a, b, c, d) (((uint32_t)(a) << 0) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

/* RGB formats */
#define VCAP_FMT_RGB332			VCAP_FOURCC('R', 'G', 'B', '1')	//  8  RGB-3-3-2 
#define VCAP_FMT_RGB444			VCAP_FOURCC('R', '4', '4', '4')	// 16  xxxxrrrr ggggbbbb
#define VCAP_FMT_RGB555			VCAP_FOURCC('R', 'G', 'B', 'O')	// 16  RGB-5-5-5
#define VCAP_FMT_RGB565			VCAP_FOURCC('R', 'G', 'B', 'P')	// 16  RGB-5-6-5
#define VCAP_FMT_RGB555X		VCAP_FOURCC('R', 'G', 'B', 'Q')	// 16  RGB-5-5-5 BE
#define VCAP_FMT_RGB565X		VCAP_FOURCC('R', 'G', 'B', 'R')	// 16  RGB-5-6-5 BE
#define VCAP_FMT_BGR666			VCAP_FOURCC('B', 'G', 'R', 'H')	// 18  BGR-6-6-6
#define VCAP_FMT_BGR24			VCAP_FOURCC('B', 'G', 'R', '3')	// 24  BGR-8-8-8
#define VCAP_FMT_RGB24			VCAP_FOURCC('R', 'G', 'B', '3')	// 24  RGB-8-8-8
#define VCAP_FMT_BGR32			VCAP_FOURCC('B', 'G', 'R', '4')	// 32  BGR-8-8-8-8
#define VCAP_FMT_RGB32			VCAP_FOURCC('R', 'G', 'B', '4')	// 32  RGB-8-8-8-8

/* Grey formats */
#define VCAP_FMT_GREY			VCAP_FOURCC('G', 'R', 'E', 'Y')	//  8  Greyscale
#define VCAP_FMT_Y4				VCAP_FOURCC('Y', '0', '4', ' ')	//  4  Greyscale
#define VCAP_FMT_Y6				VCAP_FOURCC('Y', '0', '6', ' ')	//  6  Greyscale
#define VCAP_FMT_Y10			VCAP_FOURCC('Y', '1', '0', ' ')	// 10  Greyscale
#define VCAP_FMT_Y12			VCAP_FOURCC('Y', '1', '2', ' ')	// 12  Greyscale
#define VCAP_FMT_Y16			VCAP_FOURCC('Y', '1', '6', ' ')	// 16  Greyscale

/* Grey bit-packed formats */
#define VCAP_FMT_Y10BPACK		VCAP_FOURCC('Y', '1', '0', 'B')	// 10  Greyscale bit-packed

/* Palette formats */
#define VCAP_FMT_PAL8			VCAP_FOURCC('P', 'A', 'L', '8')	//  8  8-bit palette

/* Chrominance formats */
#define VCAP_FMT_UV8			VCAP_FOURCC('U', 'V', '8', ' ')	//  8  UV 4:4

/* Luminance+Chrominance formats */
#define VCAP_FMT_YVU410			VCAP_FOURCC('Y', 'V', 'U', '9')	//  9  YVU 4:1:0
#define VCAP_FMT_YVU420			VCAP_FOURCC('Y', 'V', '1', '2')	// 12  YVU 4:2:0
#define VCAP_FMT_YUYV			VCAP_FOURCC('Y', 'U', 'Y', 'V')	// 16  YUV 4:2:2
#define VCAP_FMT_YYUV			VCAP_FOURCC('Y', 'Y', 'U', 'V')	// 16  YUV 4:2:2
#define VCAP_FMT_YVYU			VCAP_FOURCC('Y', 'V', 'Y', 'U')	// 16  YVU 4:2:2
#define VCAP_FMT_UYVY			VCAP_FOURCC('U', 'Y', 'V', 'Y')	// 16  YUV 4:2:2
#define VCAP_FMT_VYUY			VCAP_FOURCC('V', 'Y', 'U', 'Y')	// 16  YUV 4:2:2
#define VCAP_FMT_YUV422P		VCAP_FOURCC('4', '2', '2', 'P')	// 16  YVU422 planar
#define VCAP_FMT_YUV411P		VCAP_FOURCC('4', '1', '1', 'P')	// 16  YVU411 planar
#define VCAP_FMT_Y41P			VCAP_FOURCC('Y', '4', '1', 'P')	// 12  YUV 4:1:1
#define VCAP_FMT_YUV444			VCAP_FOURCC('Y', '4', '4', '4')	// 16  xxxxyyyy uuuuvvvv
#define VCAP_FMT_YUV555			VCAP_FOURCC('Y', 'U', 'V', 'O')	// 16  YUV-5-5-5
#define VCAP_FMT_YUV565			VCAP_FOURCC('Y', 'U', 'V', 'P')	// 16  YUV-5-6-5
#define VCAP_FMT_YUV32			VCAP_FOURCC('Y', 'U', 'V', '4')	// 32  YUV-8-8-8-8
#define VCAP_FMT_YUV410			VCAP_FOURCC('Y', 'U', 'V', '9')	//  9  YUV 4:1:0
#define VCAP_FMT_YUV420			VCAP_FOURCC('Y', 'U', '1', '2')	// 12  YUV 4:2:0
#define VCAP_FMT_HI240			VCAP_FOURCC('H', 'I', '2', '4')	//  8  8-bit color
#define VCAP_FMT_HM12			VCAP_FOURCC('H', 'M', '1', '2')	//  8  YUV 4:2:0 16x16 macroblocks
#define VCAP_FMT_M420			VCAP_FOURCC('M', '4', '2', '0')	// 12  YUV 4:2:0 2 lines y, 1 line uv interleaved

/* two planes -- one Y, one Cr + Cb interleaved  */
#define VCAP_FMT_NV12			VCAP_FOURCC('N', 'V', '1', '2')	// 12  Y/CbCr 4:2:0
#define VCAP_FMT_NV21			VCAP_FOURCC('N', 'V', '2', '1')	// 12  Y/CrCb 4:2:0
#define VCAP_FMT_NV16			VCAP_FOURCC('N', 'V', '1', '6')	// 16  Y/CbCr 4:2:2
#define VCAP_FMT_NV61			VCAP_FOURCC('N', 'V', '6', '1')	// 16  Y/CrCb 4:2:2
#define VCAP_FMT_NV24			VCAP_FOURCC('N', 'V', '2', '4')	// 24  Y/CbCr 4:4:4
#define VCAP_FMT_NV42			VCAP_FOURCC('N', 'V', '4', '2')	// 24  Y/CrCb 4:4:4

/* two non contiguous planes - one Y, one Cr + Cb interleaved  */
#define VCAP_FMT_NV12M			VCAP_FOURCC('N', 'M', '1', '2')	// 12  Y/CbCr 4:2:0
#define VCAP_FMT_NV21M			VCAP_FOURCC('N', 'M', '2', '1')	// 21  Y/CrCb 4:2:0
#define VCAP_FMT_NV16M			VCAP_FOURCC('N', 'M', '1', '6')	// 16  Y/CbCr 4:2:2
#define VCAP_FMT_NV61M			VCAP_FOURCC('N', 'M', '6', '1')	// 16  Y/CrCb 4:2:2
#define VCAP_FMT_NV12MT			VCAP_FOURCC('T', 'M', '1', '2')	// 12  Y/CbCr 4:2:0 64x32 macroblocks
#define VCAP_FMT_NV12MT_16X16	VCAP_FOURCC('V', 'M', '1', '2')	// 12  Y/CbCr 4:2:0 16x16 macroblocks

/* three non contiguous planes - Y, Cb, Cr */
#define VCAP_FMT_YUV420M		VCAP_FOURCC('Y', 'M', '1', '2')	// 12  YUV420 planar
#define VCAP_FMT_YVU420M		VCAP_FOURCC('Y', 'M', '2', '1')	// 12  YVU420 planar

/* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
#define VCAP_FMT_SBGGR8			VCAP_FOURCC('B', 'A', '8', '1')	//  8  BGBG.. GRGR..
#define VCAP_FMT_SGBRG8			VCAP_FOURCC('G', 'B', 'R', 'G')	//  8  GBGB.. RGRG..
#define VCAP_FMT_SGRBG8			VCAP_FOURCC('G', 'R', 'B', 'G')	//  8  GRGR.. BGBG..
#define VCAP_FMT_SRGGB8			VCAP_FOURCC('R', 'G', 'G', 'B')	//  8  RGRG.. GBGB..
#define VCAP_FMT_SBGGR10		VCAP_FOURCC('B', 'G', '1', '0')	// 10  BGBG.. GRGR..
#define VCAP_FMT_SGBRG10		VCAP_FOURCC('G', 'B', '1', '0')	// 10  GBGB.. RGRG..
#define VCAP_FMT_SGRBG10		VCAP_FOURCC('B', 'A', '1', '0')	// 10  GRGR.. BGBG..
#define VCAP_FMT_SRGGB10		VCAP_FOURCC('R', 'G', '1', '0') 	// 10  RGRG.. GBGB..
#define VCAP_FMT_SBGGR12		VCAP_FOURCC('B', 'G', '1', '2')	// 12  BGBG.. GRGR..
#define VCAP_FMT_SGBRG12		VCAP_FOURCC('G', 'B', '1', '2')	// 12  GBGB.. RGRG..
#define VCAP_FMT_SGRBG12		VCAP_FOURCC('B', 'A', '1', '2')	// 12  GRGR.. BGBG..
#define VCAP_FMT_SRGGB12		VCAP_FOURCC('R', 'G', '1', '2')	// 12  RGRG.. GBGB..
	
/* 10bit raw bayer a-law compressed to 8 bits */
#define VCAP_FMT_SBGGR10ALAW8	VCAP_FOURCC('a', 'B', 'A', '8')
#define VCAP_FMT_SGBRG10ALAW8	VCAP_FOURCC('a', 'G', 'A', '8')
#define VCAP_FMT_SGRBG10ALAW8 	VCAP_FOURCC('a', 'g', 'A', '8')
#define VCAP_FMT_SRGGB10ALAW8	VCAP_FOURCC('a', 'R', 'A', '8')

/* 10bit raw bayer DPCM compressed to 8 bits */
#define VCAP_FMT_SBGGR10DPCM8	VCAP_FOURCC('b', 'B', 'A', '8')
#define VCAP_FMT_SGBRG10DPCM8 	VCAP_FOURCC('b', 'G', 'A', '8')
#define VCAP_FMT_SGRBG10DPCM8	VCAP_FOURCC('B', 'D', '1', '0')
#define VCAP_FMT_SRGGB10DPCM8	VCAP_FOURCC('b', 'R', 'A', '8')

/*
 * 10bit raw bayer, expanded to 16 bits
 * xxxxrrrrrrrrrrxxxxgggggggggg xxxxggggggggggxxxxbbbbbbbbbb...
 */
#define VCAP_FMT_SBGGR16		VCAP_FOURCC('B', 'Y', 'R', '2')	// 16  BGBG.. GRGR..

/* Compressed formats */
#define VCAP_FMT_MJPEG			VCAP_FOURCC('M', 'J', 'P', 'G')	// Motion-JPEG   */
#define VCAP_FMT_JPEG			VCAP_FOURCC('J', 'P', 'E', 'G')	// JFIF JPEG     */
#define VCAP_FMT_DV				VCAP_FOURCC('d', 'v', 's', 'd')	// 1394          */
#define VCAP_FMT_MPEG			VCAP_FOURCC('M', 'P', 'E', 'G')	// MPEG-1/2/4 Multiplexed */
#define VCAP_FMT_H264			VCAP_FOURCC('H', '2', '6', '4')	// H264 with start codes */
#define VCAP_FMT_H264_NO_SC		VCAP_FOURCC('A', 'V', 'C', '1')	// H264 without start codes */
#define VCAP_FMT_H264_MVC		VCAP_FOURCC('M', '2', '6', '4')	// H264 MVC */
#define VCAP_FMT_H263			VCAP_FOURCC('H', '2', '6', '3')	// H263          */
#define VCAP_FMT_MPEG1			VCAP_FOURCC('M', 'P', 'G', '1')	// MPEG-1 ES     */
#define VCAP_FMT_MPEG2			VCAP_FOURCC('M', 'P', 'G', '2')	// MPEG-2 ES     */
#define VCAP_FMT_MPEG4			VCAP_FOURCC('M', 'P', 'G', '4')	// MPEG-4 part 2 ES */
#define VCAP_FMT_XVID			VCAP_FOURCC('X', 'V', 'I', 'D')	// Xvid           */
#define VCAP_FMT_VC1_ANNEX_G	VCAP_FOURCC('V', 'C', '1', 'G')	// SMPTE 421M Annex G compliant stream */
#define VCAP_FMT_VC1_ANNEX_L	VCAP_FOURCC('V', 'C', '1', 'L')	// SMPTE 421M Annex L compliant stream */
#define VCAP_FMT_VP8			VCAP_FOURCC('V', 'P', '8', '0')	// VP8 */

/*  Vendor-specific formats   */
#define VCAP_FMT_CPIA1			VCAP_FOURCC('C', 'P', 'I', 'A')	// cpia1 YUV */
#define VCAP_FMT_WNVA			VCAP_FOURCC('W', 'N', 'V', 'A')	// Winnov hw compress */
#define VCAP_FMT_SN9C10X		VCAP_FOURCC('S', '9', '1', '0')	// SN9C10x compression */
#define VCAP_FMT_SN9C20X_I420	VCAP_FOURCC('S', '9', '2', '0')	// SN9C20x YUV 4:2:0 */
#define VCAP_FMT_PWC1			VCAP_FOURCC('P', 'W', 'C', '1')	// pwc older webcam */
#define VCAP_FMT_PWC2			VCAP_FOURCC('P', 'W', 'C', '2')	// pwc newer webcam */
#define VCAP_FMT_ET61X251		VCAP_FOURCC('E', '6', '2', '5')	// ET61X251 compression */
#define VCAP_FMT_SPCA501		VCAP_FOURCC('S', '5', '0', '1')	// YUYV per line */
#define VCAP_FMT_SPCA505		VCAP_FOURCC('S', '5', '0', '5')	// YYUV per line */
#define VCAP_FMT_SPCA508		VCAP_FOURCC('S', '5', '0', '8')	// YUVY per line */
#define VCAP_FMT_SPCA561		VCAP_FOURCC('S', '5', '6', '1')	// compressed GBRG bayer */
#define VCAP_FMT_PAC207			VCAP_FOURCC('P', '2', '0', '7')	// compressed BGGR bayer */
#define VCAP_FMT_MR97310A		VCAP_FOURCC('M', '3', '1', '0')	// compressed BGGR bayer */
#define VCAP_FMT_JL2005BCD		VCAP_FOURCC('J', 'L', '2', '0')	// compressed RGGB bayer */
#define VCAP_FMT_SN9C2028		VCAP_FOURCC('S', 'O', 'N', 'X')	// compressed GBRG bayer */
#define VCAP_FMT_SQ905C			VCAP_FOURCC('9', '0', '5', 'C')	// compressed RGGB bayer */
#define VCAP_FMT_PJPG			VCAP_FOURCC('P', 'J', 'P', 'G')	// Pixart 73xx JPEG */
#define VCAP_FMT_OV511			VCAP_FOURCC('O', '5', '1', '1')	// ov511 JPEG */
#define VCAP_FMT_OV518			VCAP_FOURCC('O', '5', '1', '8')	// ov518 JPEG */
#define VCAP_FMT_STV0680		VCAP_FOURCC('S', '6', '8', '0')	// stv0680 bayer */
#define VCAP_FMT_TM6000			VCAP_FOURCC('T', 'M', '6', '0') 	// tm5600/tm60x0 */
#define VCAP_FMT_CIT_YYVYUY		VCAP_FOURCC('C', 'I', 'T', 'V')	// one line of Y then 1 line of VYUY */
#define VCAP_FMT_KONICA420		VCAP_FOURCC('K', 'O', 'N', 'I')	// YUV420 planar in blocks of 256 pixels */
#define VCAP_FMT_JPGL			VCAP_FOURCC('J', 'P', 'G', 'L')	// JPEG-Lite */
#define VCAP_FMT_SE401			VCAP_FOURCC('S', '4', '0', '1')	// se401 janggu compressed rgb */
#define VCAP_FMT_S5C_UYVY_JPG	VCAP_FOURCC('S', '5', 'C', 'I')	// S5C73M3 interleaved UYVY/JPEG */

#endif
