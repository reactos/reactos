/*
 * Copyright (C) 2011 Marek Olšák <maraeo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Copied from EXT_texture_shared_exponent and edited. */

#ifndef RGB9E5_H
#define RGB9E5_H

#include <math.h>
#include <assert.h>

#define RGB9E5_EXPONENT_BITS          5
#define RGB9E5_MANTISSA_BITS          9
#define RGB9E5_EXP_BIAS               15
#define RGB9E5_MAX_VALID_BIASED_EXP   31

#define MAX_RGB9E5_EXP               (RGB9E5_MAX_VALID_BIASED_EXP - RGB9E5_EXP_BIAS)
#define RGB9E5_MANTISSA_VALUES       (1<<RGB9E5_MANTISSA_BITS)
#define MAX_RGB9E5_MANTISSA          (RGB9E5_MANTISSA_VALUES-1)
#define MAX_RGB9E5                   (((float)MAX_RGB9E5_MANTISSA)/RGB9E5_MANTISSA_VALUES * (1<<MAX_RGB9E5_EXP))
#define EPSILON_RGB9E5               ((1.0/RGB9E5_MANTISSA_VALUES) / (1<<RGB9E5_EXP_BIAS))

typedef union {
   unsigned int raw;
   float value;
   struct {
#if defined(MESA_BIG_ENDIAN) || defined(PIPE_ARCH_BIG_ENDIAN)
      unsigned int negative:1;
      unsigned int biasedexponent:8;
      unsigned int mantissa:23;
#else
      unsigned int mantissa:23;
      unsigned int biasedexponent:8;
      unsigned int negative:1;
#endif
   } field;
} float754;

typedef union {
   unsigned int raw;
   struct {
#if defined(MESA_BIG_ENDIAN) || defined(PIPE_ARCH_BIG_ENDIAN)
      unsigned int biasedexponent:RGB9E5_EXPONENT_BITS;
      unsigned int b:RGB9E5_MANTISSA_BITS;
      unsigned int g:RGB9E5_MANTISSA_BITS;
      unsigned int r:RGB9E5_MANTISSA_BITS;
#else
      unsigned int r:RGB9E5_MANTISSA_BITS;
      unsigned int g:RGB9E5_MANTISSA_BITS;
      unsigned int b:RGB9E5_MANTISSA_BITS;
      unsigned int biasedexponent:RGB9E5_EXPONENT_BITS;
#endif
   } field;
} rgb9e5;

static INLINE float rgb9e5_ClampRange(float x)
{
   if (x > 0.0) {
      if (x >= MAX_RGB9E5) {
         return MAX_RGB9E5;
      } else {
         return x;
      }
   } else {
      /* NaN gets here too since comparisons with NaN always fail! */
      return 0.0;
   }
}

/* Ok, FloorLog2 is not correct for the denorm and zero values, but we
   are going to do a max of this value with the minimum rgb9e5 exponent
   that will hide these problem cases. */
static INLINE int rgb9e5_FloorLog2(float x)
{
   float754 f;

   f.value = x;
   return (f.field.biasedexponent - 127);
}

static INLINE unsigned float3_to_rgb9e5(const float rgb[3])
{
   rgb9e5 retval;
   float maxrgb;
   int rm, gm, bm;
   float rc, gc, bc;
   int exp_shared, maxm;
   double denom;

   rc = rgb9e5_ClampRange(rgb[0]);
   gc = rgb9e5_ClampRange(rgb[1]);
   bc = rgb9e5_ClampRange(rgb[2]);

   maxrgb = MAX3(rc, gc, bc);
   exp_shared = MAX2(-RGB9E5_EXP_BIAS-1, rgb9e5_FloorLog2(maxrgb)) + 1 + RGB9E5_EXP_BIAS;
   assert(exp_shared <= RGB9E5_MAX_VALID_BIASED_EXP);
   assert(exp_shared >= 0);
   /* This pow function could be replaced by a table. */
   denom = pow(2, exp_shared - RGB9E5_EXP_BIAS - RGB9E5_MANTISSA_BITS);

   maxm = (int) floor(maxrgb / denom + 0.5);
   if (maxm == MAX_RGB9E5_MANTISSA+1) {
      denom *= 2;
      exp_shared += 1;
      assert(exp_shared <= RGB9E5_MAX_VALID_BIASED_EXP);
   } else {
      assert(maxm <= MAX_RGB9E5_MANTISSA);
   }

   rm = (int) floor(rc / denom + 0.5);
   gm = (int) floor(gc / denom + 0.5);
   bm = (int) floor(bc / denom + 0.5);

   assert(rm <= MAX_RGB9E5_MANTISSA);
   assert(gm <= MAX_RGB9E5_MANTISSA);
   assert(bm <= MAX_RGB9E5_MANTISSA);
   assert(rm >= 0);
   assert(gm >= 0);
   assert(bm >= 0);

   retval.field.r = rm;
   retval.field.g = gm;
   retval.field.b = bm;
   retval.field.biasedexponent = exp_shared;

   return retval.raw;
}

static INLINE void rgb9e5_to_float3(unsigned rgb, float retval[3])
{
   rgb9e5 v;
   int exponent;
   float scale;

   v.raw = rgb;
   exponent = v.field.biasedexponent - RGB9E5_EXP_BIAS - RGB9E5_MANTISSA_BITS;
   scale = (float) pow(2, exponent);

   retval[0] = v.field.r * scale;
   retval[1] = v.field.g * scale;
   retval[2] = v.field.b * scale;
}

#endif
