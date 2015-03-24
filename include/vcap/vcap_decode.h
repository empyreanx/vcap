#ifndef _VCAP_DECODE_H
#define _VCAP_DECODE_H

#include <stdint.h>

/**
 * \file 
 * \brief Frame decoder.
 */

/**
 * \brief Decodes a frame to RGB24 or BGR24.
 */
int vcap_decode(uint8_t *src, uint8_t *dst, uint32_t format_code, uint32_t width, uint32_t height, uint8_t bgr);

#endif
