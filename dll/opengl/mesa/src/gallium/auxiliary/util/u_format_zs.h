/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


#ifndef U_FORMAT_ZS_H_
#define U_FORMAT_ZS_H_


#include "pipe/p_compiler.h"


void
util_format_s8_uint_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_s8_uint_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z16_unorm_unpack_z_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z16_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z16_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z16_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride, const uint32_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_unorm_unpack_z_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride, const uint32_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_float_unpack_z_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_float_pack_z_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_float_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_float_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride, const uint32_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z24_unorm_s8_uint_unpack_z_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z24_unorm_s8_uint_pack_z_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z24_unorm_s8_uint_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z24_unorm_s8_uint_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride, const uint32_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z24_unorm_s8_uint_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z24_unorm_s8_uint_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_s8_uint_z24_unorm_unpack_z_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_s8_uint_z24_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_s8_uint_z24_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_s8_uint_z24_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride, const uint32_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_s8_uint_z24_unorm_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_s8_uint_z24_unorm_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z24x8_unorm_unpack_z_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z24x8_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z24x8_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z24x8_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride, const uint32_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_x8z24_unorm_unpack_z_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_x8z24_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_x8z24_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_x8z24_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride, const uint32_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_float_s8x24_uint_unpack_z_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_float_s8x24_uint_pack_z_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_float_s8x24_uint_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_float_s8x24_uint_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride, const uint32_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_float_s8x24_uint_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);


void
util_format_z32_float_s8x24_uint_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);

void
util_format_x24s8_uint_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);

void
util_format_x24s8_uint_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);

void
util_format_s8x24_uint_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);

void
util_format_s8x24_uint_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);

void
util_format_x32_s8x24_uint_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);

void
util_format_x32_s8x24_uint_pack_s_8uint(uint8_t *dst_row, unsigned dst_sride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height);
#endif /* U_FORMAT_ZS_H_ */
