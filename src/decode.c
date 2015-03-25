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
#include <vcap/vcap.h> 

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#define VCAP_CLIP(color) (uint8_t)(((color)>0xFF)?0xff:(((color)<0)?0:(color)))

static void vcap_bgr24_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height);
static void vcap_rgb24_to_bgr24(uint8_t *src, uint8_t *dst, int width, int height);
static void vcap_rgb565_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height);
static void vcap_yuyv_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height);
static void vcap_yvyu_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height);
static void vcap_uyvy_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height);
static void vcap_yuv420_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height);
static void vcap_spca501_to_yuv420(uint8_t *src, uint8_t *dst, int width, int height);
static void vcap_spca505_to_yuv420(uint8_t *src, uint8_t *dst, int width, int height);
static void vcap_spca508_to_yuv420(uint8_t *src, uint8_t *dst, int width, int height);
static void vcap_yvu420_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height);

int vcap_decode(uint8_t *src, uint8_t *dst, uint32_t format_code, uint32_t width, uint32_t height, uint8_t bgr) {
	uint8_t *tmp_dst, *tmp_rgb, *tmp_yuv;
	
	switch (format_code) {
		case VCAP_FMT_RGB24:
			if (bgr)
				vcap_rgb24_to_bgr24(src, dst, width, height);
			else
				memcpy(dst, src, 3 * width * height);
			return 0;
			
		case VCAP_FMT_BGR24:
			if (bgr)
				memcpy(dst, src, 3 * width * height);
			else
				vcap_bgr24_to_rgb24(src, dst, width, height);
			return 0;
	}
	
	if (bgr) {
		tmp_rgb = (uint8_t*)malloc(3 * width * height);
		tmp_dst = tmp_rgb;
	} else {
		tmp_dst = dst;
	}
	
	int converted = 0;
	
	switch (format_code) {
		case VCAP_FMT_RGB565:
			vcap_rgb565_to_rgb24(src, tmp_dst, width, height);
			converted = 1;
			break;
			
		case VCAP_FMT_YUYV:
			vcap_yuyv_to_rgb24(src, tmp_dst, width, height);
			converted = 1;
			break;
			
		case VCAP_FMT_YVYU:
			vcap_yvyu_to_rgb24(src, tmp_dst, width, height);
			converted = 1;
			break;
			
		case VCAP_FMT_UYVY:
			vcap_uyvy_to_rgb24(src, tmp_dst, width, height);
			converted = 1;
			break;
		
		case VCAP_FMT_YUV420:
			vcap_yuv420_to_rgb24(src, tmp_dst, width, height);
			converted = 1;
			break;
		
		case VCAP_FMT_YVU420:
			vcap_yvu420_to_rgb24(src, tmp_dst, width, height);
			converted = 1;
			break;
	}
	
	if (converted) {
		if (bgr) {
			vcap_rgb24_to_bgr24(tmp_dst, dst, width, height);
			free(tmp_rgb);
		}
		
		return 0;
	}
	
	tmp_yuv = (uint8_t*)malloc(3 * width * height / 2);
	
	switch (format_code) {	
		case VCAP_FMT_SPCA501:
			vcap_spca501_to_yuv420(src, tmp_yuv, width, height);
			vcap_yuv420_to_rgb24(tmp_yuv, tmp_dst, width, height);
			converted = 1;
			break;
			
		case VCAP_FMT_SPCA505:
			vcap_spca505_to_yuv420(src, tmp_yuv, width, height);
			vcap_yuv420_to_rgb24(tmp_yuv, tmp_dst, width, height);
			converted = 1;
			break;
			
		case VCAP_FMT_SPCA508:
			vcap_spca508_to_yuv420(src, tmp_yuv, width, height);
			vcap_yuv420_to_rgb24(tmp_yuv, tmp_dst, width, height);
			converted = 1;
			break;
	}
	
	free(tmp_yuv);
	
	if (converted) {
		if (bgr) {
			vcap_rgb24_to_bgr24(tmp_dst, dst, width, height);
			free(tmp_rgb);
		}
		
		return 0;
	}
	
	printf("format_code: %d\n", format_code);
	
	char code_str[5];
	
	vcap_fourcc_str(format_code, code_str);
	
	vcap_set_error("Unable to decode format %s", code_str);
	
	return -1;
}

static void vcap_bgr24_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height) {
	uint32_t pixels = width * height;
	
	while (pixels--) {
		*dst++ = src[2];
		*dst++ = src[1];
		*dst++ = src[0];
		src += 3;
	}
}

static void vcap_rgb24_to_bgr24(uint8_t *src, uint8_t *dst, int width, int height) {
	uint32_t pixels = width * height;
	
	while (pixels--) {
		*dst++ = src[2];
		*dst++ = src[1];
		*dst++ = src[0];
		src += 3;
	}	
}

static void vcap_rgb565_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height) {
	while (height >= 0) {
		for (int j = 0; j < width; j++) {
			unsigned short tmp = *(unsigned short *)src;
			
			/* Original format: rrrrrggg gggbbbbb */
			*dst++ = 0xf8 & (tmp >> 8);
			*dst++ = 0xfc & (tmp >> 3);
			*dst++ = 0xf8 & (tmp << 3);
			
			src += 2;
		}
	}
}

static void vcap_yuyv_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height) {
	while (--height >= 0) {
		for (int j = 0; j < width; j += 2) {
			int u = src[1];
			int v = src[3];
			int u1 = (((u - 128) << 7) +  (u - 128)) >> 6;
			int rg = (((u - 128) << 1) +  (u - 128) + ((v - 128) << 2) + ((v - 128) << 1)) >> 3;
			int v1 = (((v - 128) << 1) +  (v - 128)) >> 1;
			
			*dst++ = VCAP_CLIP(src[0] + v1);
			*dst++ = VCAP_CLIP(src[0] - rg);
			*dst++ = VCAP_CLIP(src[0] + u1);
			
			*dst++ = VCAP_CLIP(src[2] + v1);
			*dst++ = VCAP_CLIP(src[2] - rg);
			*dst++ = VCAP_CLIP(src[2] + u1);
			
			src += 4;
		}
	}
}

static void vcap_yvyu_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height) {
	while (--height >= 0) {
		for (int j = 0; j < width; j += 2) {
			int u = src[3];
			int v = src[1];
			int u1 = (((u - 128) << 7) +  (u - 128)) >> 6;
			int rg = (((u - 128) << 1) +  (u - 128) + ((v - 128) << 2) + ((v - 128) << 1)) >> 3;
			int v1 = (((v - 128) << 1) +  (v - 128)) >> 1;
			
			*dst++ = VCAP_CLIP(src[0] + v1);
			*dst++ = VCAP_CLIP(src[0] - rg);
			*dst++ = VCAP_CLIP(src[0] + u1);
			
			*dst++ = VCAP_CLIP(src[2] + v1);
			*dst++ = VCAP_CLIP(src[2] - rg);
			*dst++ = VCAP_CLIP(src[2] + u1);
			
			src += 4;
		}
	}
}

static void vcap_uyvy_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height) {
	while (--height >= 0) {
		for (int j = 0; j < width; j += 2) {
			int u = src[0];
			int v = src[2];
			int u1 = (((u - 128) << 7) +  (u - 128)) >> 6;
			int rg = (((u - 128) << 1) +  (u - 128) + ((v - 128) << 2) + ((v - 128) << 1)) >> 3;
			int v1 = (((v - 128) << 1) +  (v - 128)) >> 1;
			
			*dst++ = VCAP_CLIP(src[1] + v1);
			*dst++ = VCAP_CLIP(src[1] - rg);
			*dst++ = VCAP_CLIP(src[1] + u1);
			
			*dst++ = VCAP_CLIP(src[3] + v1);
			*dst++ = VCAP_CLIP(src[3] - rg);
			*dst++ = VCAP_CLIP(src[3] + u1);
			
			src += 4;
		}
	}
}

static void vcap_yuv420_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height) {
	const unsigned char *ysrc = src;
	const unsigned char *usrc, *vsrc;
	
	usrc = src + width * height;
	vsrc = usrc + (width * height) / 4;
	
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j += 2) {
			int u1 = (((*usrc - 128) << 7) +  (*usrc - 128)) >> 6;
			int rg = (((*usrc - 128) << 1) +  (*usrc - 128) + ((*vsrc - 128) << 2) + ((*vsrc - 128) << 1)) >> 3;
			int v1 = (((*vsrc - 128) << 1) +  (*vsrc - 128)) >> 1;
			
			*dst++ = VCAP_CLIP(*ysrc + v1);
			*dst++ = VCAP_CLIP(*ysrc - rg);
			*dst++ = VCAP_CLIP(*ysrc + u1);
			
			ysrc++;
			
			*dst++ = VCAP_CLIP(*ysrc + v1);
			*dst++ = VCAP_CLIP(*ysrc - rg);
			*dst++ = VCAP_CLIP(*ysrc + u1);
			
			ysrc++;
			usrc++;
			vsrc++;
		}
		
		/* Rewind u and v for next line */
		if (!(i & 1)) {
			usrc -= width / 2;
			vsrc -= width / 2;
		}
	}
}

static void vcap_yvu420_to_rgb24(uint8_t *src, uint8_t *dst, int width, int height) {
	const unsigned char *ysrc = src;
	const unsigned char *usrc, *vsrc;
	
	vsrc = src + width * height;
	usrc = vsrc + (width * height) / 4;
	
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j += 2) {
			int u1 = (((*usrc - 128) << 7) +  (*usrc - 128)) >> 6;
			int rg = (((*usrc - 128) << 1) +  (*usrc - 128) + ((*vsrc - 128) << 2) + ((*vsrc - 128) << 1)) >> 3;
			int v1 = (((*vsrc - 128) << 1) +  (*vsrc - 128)) >> 1;
			
			*dst++ = VCAP_CLIP(*ysrc + v1);
			*dst++ = VCAP_CLIP(*ysrc - rg);
			*dst++ = VCAP_CLIP(*ysrc + u1);
			
			ysrc++;
			
			*dst++ = VCAP_CLIP(*ysrc + v1);
			*dst++ = VCAP_CLIP(*ysrc - rg);
			*dst++ = VCAP_CLIP(*ysrc + u1);
			
			ysrc++;
			usrc++;
			vsrc++;
		}
		
		/* Rewind u and v for next line */
		if (!(i & 1)) {
			usrc -= width / 2;
			vsrc -= width / 2;
		}
	}
}

static void vcap_spca501_to_yuv420(uint8_t *src, uint8_t *dst, int width, int height) {
	unsigned long *lsrc = (unsigned long *)src;
	
	for (int i = 0; i < height; i += 2) {
		/* -128 - 127 --> 0 - 255 and copy first line Y */
		unsigned long *ldst = (unsigned long *)(dst + i * width);
		
		for (int j = 0; j < width; j += sizeof(long)) {
			*ldst = *lsrc++;
			*ldst++ ^= 0x8080808080808080ULL;
		}
	
		/* -128 - 127 --> 0 - 255 and copy 1 line U */
		ldst = (unsigned long *)(dst + width * height + i * width / 4);
	
		for (int j = 0; j < width/2; j += sizeof(long)) {
			*ldst = *lsrc++;
			*ldst++ ^= 0x8080808080808080ULL;
		}
		
		/* -128 - 127 --> 0 - 255 and copy second line Y */
		ldst = (unsigned long *)(dst + i * width + width);
	
		for (int j = 0; j < width; j += sizeof(long)) {
			*ldst = *lsrc++;
			*ldst++ ^= 0x8080808080808080ULL;
		}
		
		/* -128 - 127 --> 0 - 255 and copy 1 line V */
		ldst = (unsigned long *)(dst + (width * height * 5) / 4 + i * width / 4);
	
		for (int j = 0; j < width/2; j += sizeof(long)) {
			*ldst = *lsrc++;
			*ldst++ ^= 0x8080808080808080ULL;
		}
	}
}

static void vcap_spca505_to_yuv420(uint8_t *src, uint8_t *dst, int width, int height) {
	unsigned long *lsrc = (unsigned long *)src;
	
	for (int i = 0; i < height; i += 2) {
		/* -128 - 127 --> 0 - 255 and copy 2 lines of Y */
		unsigned long *ldst = (unsigned long *)(dst + i * width);
		
		for (int j = 0; j < width*2; j += sizeof(long)) {
			*ldst = *lsrc++;
			*ldst++ ^= 0x8080808080808080ULL;
		}
		
		/* -128 - 127 --> 0 - 255 and copy 1 line U */
		ldst = (unsigned long *)(dst + width * height + i * width / 4);
		
		for (int j = 0; j < width/2; j += sizeof(long)) {
			*ldst = *lsrc++;
			*ldst++ ^= 0x8080808080808080ULL;
		}
		
		/* -128 - 127 --> 0 - 255 and copy 1 line V */
		ldst = (unsigned long *)(dst + (width * height * 5) / 4 + i * width / 4);
		
		for (int j = 0; j < width/2; j += sizeof(long)) {
			*ldst = *lsrc++;
			*ldst++ ^= 0x8080808080808080ULL;
		}
	}
}

static void vcap_spca508_to_yuv420(uint8_t *src, uint8_t *dst, int width, int height) {
	unsigned long *lsrc = (unsigned long *)src;
	
	for (int i = 0; i < height; i += 2) {
		/* -128 - 127 --> 0 - 255 and copy first line Y */
		unsigned long *ldst = (unsigned long *)(dst + i * width);
		
		for (int j = 0; j < width; j += sizeof(long)) {
			*ldst = *lsrc++;
			*ldst++ ^= 0x8080808080808080ULL;
		}
		
		/* -128 - 127 --> 0 - 255 and copy 1 line U */
		ldst = (unsigned long *)(dst + width * height + i * width / 4);
		
		for (int j = 0; j < width/2; j += sizeof(long)) {
			*ldst = *lsrc++;
			*ldst++ ^= 0x8080808080808080ULL;
		}
		
		/* -128 - 127 --> 0 - 255 and copy 1 line V */
		ldst = (unsigned long *)(dst + (width * height * 5) / 4 + i * width / 4);
		
		for (int j = 0; j < width/2; j += sizeof(long)) {
			*ldst = *lsrc++;
			*ldst++ ^= 0x8080808080808080ULL;
		}
		
		/* -128 - 127 --> 0 - 255 and copy second line Y */
		ldst = (unsigned long *)(dst + i * width + width);
		
		for (int j = 0; j < width; j += sizeof(long)) {
			*ldst = *lsrc++;
			*ldst++ ^= 0x8080808080808080ULL;
		}
	}
}
