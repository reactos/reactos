/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef U_HALF_H
#define U_HALF_H

#include "pipe/p_compiler.h"
#include "util/u_math.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const uint32_t util_half_to_float_mantissa_table[2048];
extern const uint32_t util_half_to_float_exponent_table[64];
extern const uint32_t util_half_to_float_offset_table[64];
extern const uint16_t util_float_to_half_base_table[512];
extern const uint8_t util_float_to_half_shift_table[512];

/*
 * Note that if the half float is a signaling NaN, the x87 FPU will turn
 * it into a quiet NaN immediately upon loading into a float.
 *
 * Additionally, denormals may be flushed to zero.
 *
 * To avoid this, use the floatui functions instead of the float ones
 * when just doing conversion rather than computation on the resulting
 * floats.
 */

static INLINE uint32_t
util_half_to_floatui(uint16_t h)
{
   unsigned exp = h >> 10;
   return util_half_to_float_mantissa_table[util_half_to_float_offset_table[exp] + (h & 0x3ff)] + util_half_to_float_exponent_table[exp];
}

static INLINE float
util_half_to_float(uint16_t h)
{
   union fi r;
   r.ui = util_half_to_floatui(h);
   return r.f;
}

static INLINE uint16_t
util_floatui_to_half(uint32_t v)
{
   unsigned signexp = v >> 23;
   return util_float_to_half_base_table[signexp] + ((v & 0x007fffff) >> util_float_to_half_shift_table[signexp]);
}

static INLINE uint16_t
util_float_to_half(float f)
{
   union fi i;
   i.f = f;
   return util_floatui_to_half(i.ui);
}

#ifdef __cplusplus
}
#endif

#endif /* U_HALF_H */

