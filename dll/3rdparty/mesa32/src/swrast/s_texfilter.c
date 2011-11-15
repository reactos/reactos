/*
 * Mesa 3-D graphics library
 * Version:  7.3
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "main/glheader.h"
#include "main/context.h"
#include "main/colormac.h"
#include "main/imports.h"
#include "main/texformat.h"

#include "s_context.h"
#include "s_texfilter.h"


/*
 * Note, the FRAC macro has to work perfectly.  Otherwise you'll sometimes
 * see 1-pixel bands of improperly weighted linear-filtered textures.
 * The tests/texwrap.c demo is a good test.
 * Also note, FRAC(x) doesn't truly return the fractional part of x for x < 0.
 * Instead, if x < 0 then FRAC(x) = 1 - true_frac(x).
 */
#define FRAC(f)  ((f) - IFLOOR(f))


/**
 * Constants for integer linear interpolation.
 */
#define ILERP_SCALE 65536.0F
#define ILERP_SHIFT 16


/**
 * Linear interpolation macros
 */
#define LERP(T, A, B)  ( (A) + (T) * ((B) - (A)) )
#define ILERP(IT, A, B)  ( (A) + (((IT) * ((B) - (A))) >> ILERP_SHIFT) )


/**
 * Do 2D/biliner interpolation of float values.
 * v00, v10, v01 and v11 are typically four texture samples in a square/box.
 * a and b are the horizontal and vertical interpolants.
 * It's important that this function is inlined when compiled with
 * optimization!  If we find that's not true on some systems, convert
 * to a macro.
 */
static INLINE GLfloat
lerp_2d(GLfloat a, GLfloat b,
        GLfloat v00, GLfloat v10, GLfloat v01, GLfloat v11)
{
   const GLfloat temp0 = LERP(a, v00, v10);
   const GLfloat temp1 = LERP(a, v01, v11);
   return LERP(b, temp0, temp1);
}


/**
 * Do 2D/biliner interpolation of integer values.
 * \sa lerp_2d
 */
static INLINE GLint
ilerp_2d(GLint ia, GLint ib,
         GLint v00, GLint v10, GLint v01, GLint v11)
{
   /* fixed point interpolants in [0, ILERP_SCALE] */
   const GLint temp0 = ILERP(ia, v00, v10);
   const GLint temp1 = ILERP(ia, v01, v11);
   return ILERP(ib, temp0, temp1);
}


/**
 * Do 3D/trilinear interpolation of float values.
 * \sa lerp_2d
 */
static INLINE GLfloat
lerp_3d(GLfloat a, GLfloat b, GLfloat c,
        GLfloat v000, GLfloat v100, GLfloat v010, GLfloat v110,
        GLfloat v001, GLfloat v101, GLfloat v011, GLfloat v111)
{
   const GLfloat temp00 = LERP(a, v000, v100);
   const GLfloat temp10 = LERP(a, v010, v110);
   const GLfloat temp01 = LERP(a, v001, v101);
   const GLfloat temp11 = LERP(a, v011, v111);
   const GLfloat temp0 = LERP(b, temp00, temp10);
   const GLfloat temp1 = LERP(b, temp01, temp11);
   return LERP(c, temp0, temp1);
}


/**
 * Do 3D/trilinear interpolation of integer values.
 * \sa lerp_2d
 */
static INLINE GLint
ilerp_3d(GLint ia, GLint ib, GLint ic,
         GLint v000, GLint v100, GLint v010, GLint v110,
         GLint v001, GLint v101, GLint v011, GLint v111)
{
   /* fixed point interpolants in [0, ILERP_SCALE] */
   const GLint temp00 = ILERP(ia, v000, v100);
   const GLint temp10 = ILERP(ia, v010, v110);
   const GLint temp01 = ILERP(ia, v001, v101);
   const GLint temp11 = ILERP(ia, v011, v111);
   const GLint temp0 = ILERP(ib, temp00, temp10);
   const GLint temp1 = ILERP(ib, temp01, temp11);
   return ILERP(ic, temp0, temp1);
}


/**
 * Do linear interpolation of colors.
 */
static INLINE void
lerp_rgba(GLchan result[4], GLfloat t, const GLchan a[4], const GLchan b[4])
{
#if CHAN_TYPE == GL_FLOAT
   result[0] = LERP(t, a[0], b[0]);
   result[1] = LERP(t, a[1], b[1]);
   result[2] = LERP(t, a[2], b[2]);
   result[3] = LERP(t, a[3], b[3]);
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
   result[0] = (GLchan) (LERP(t, a[0], b[0]) + 0.5);
   result[1] = (GLchan) (LERP(t, a[1], b[1]) + 0.5);
   result[2] = (GLchan) (LERP(t, a[2], b[2]) + 0.5);
   result[3] = (GLchan) (LERP(t, a[3], b[3]) + 0.5);
#else
   /* fixed point interpolants in [0, ILERP_SCALE] */
   const GLint it = IROUND_POS(t * ILERP_SCALE);
   ASSERT(CHAN_TYPE == GL_UNSIGNED_BYTE);
   result[0] = ILERP(it, a[0], b[0]);
   result[1] = ILERP(it, a[1], b[1]);
   result[2] = ILERP(it, a[2], b[2]);
   result[3] = ILERP(it, a[3], b[3]);
#endif
}


/**
 * Do bilinear interpolation of colors.
 */
static INLINE void
lerp_rgba_2d(GLchan result[4], GLfloat a, GLfloat b,
             const GLchan t00[4], const GLchan t10[4],
             const GLchan t01[4], const GLchan t11[4])
{
#if CHAN_TYPE == GL_FLOAT
   result[0] = lerp_2d(a, b, t00[0], t10[0], t01[0], t11[0]);
   result[1] = lerp_2d(a, b, t00[1], t10[1], t01[1], t11[1]);
   result[2] = lerp_2d(a, b, t00[2], t10[2], t01[2], t11[2]);
   result[3] = lerp_2d(a, b, t00[3], t10[3], t01[3], t11[3]);
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
   result[0] = (GLchan) (lerp_2d(a, b, t00[0], t10[0], t01[0], t11[0]) + 0.5);
   result[1] = (GLchan) (lerp_2d(a, b, t00[1], t10[1], t01[1], t11[1]) + 0.5);
   result[2] = (GLchan) (lerp_2d(a, b, t00[2], t10[2], t01[2], t11[2]) + 0.5);
   result[3] = (GLchan) (lerp_2d(a, b, t00[3], t10[3], t01[3], t11[3]) + 0.5);
#else
   const GLint ia = IROUND_POS(a * ILERP_SCALE);
   const GLint ib = IROUND_POS(b * ILERP_SCALE);
   ASSERT(CHAN_TYPE == GL_UNSIGNED_BYTE);
   result[0] = ilerp_2d(ia, ib, t00[0], t10[0], t01[0], t11[0]);
   result[1] = ilerp_2d(ia, ib, t00[1], t10[1], t01[1], t11[1]);
   result[2] = ilerp_2d(ia, ib, t00[2], t10[2], t01[2], t11[2]);
   result[3] = ilerp_2d(ia, ib, t00[3], t10[3], t01[3], t11[3]);
#endif
}


/**
 * Do trilinear interpolation of colors.
 */
static INLINE void
lerp_rgba_3d(GLchan result[4], GLfloat a, GLfloat b, GLfloat c,
             const GLchan t000[4], const GLchan t100[4],
             const GLchan t010[4], const GLchan t110[4],
             const GLchan t001[4], const GLchan t101[4],
             const GLchan t011[4], const GLchan t111[4])
{
   GLuint k;
   /* compiler should unroll these short loops */
#if CHAN_TYPE == GL_FLOAT
   for (k = 0; k < 4; k++) {
      result[k] = lerp_3d(a, b, c, t000[k], t100[k], t010[k], t110[k],
                                   t001[k], t101[k], t011[k], t111[k]);
   }
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
   for (k = 0; k < 4; k++) {
      result[k] = (GLchan)(lerp_3d(a, b, c,
                                   t000[k], t100[k], t010[k], t110[k],
                                   t001[k], t101[k], t011[k], t111[k]) + 0.5F);
   }
#else
   GLint ia = IROUND_POS(a * ILERP_SCALE);
   GLint ib = IROUND_POS(b * ILERP_SCALE);
   GLint ic = IROUND_POS(c * ILERP_SCALE);
   for (k = 0; k < 4; k++) {
      result[k] = ilerp_3d(ia, ib, ic, t000[k], t100[k], t010[k], t110[k],
                                       t001[k], t101[k], t011[k], t111[k]);
   }
#endif
}


/**
 * If A is a signed integer, A % B doesn't give the right value for A < 0
 * (in terms of texture repeat).  Just casting to unsigned fixes that.
 */
#define REMAINDER(A, B) ((unsigned) (A) % (unsigned) (B))


/**
 * Used to compute texel locations for linear sampling.
 * Input:
 *    wrapMode = GL_REPEAT, GL_CLAMP, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
 *    s = texcoord in [0,1]
 *    size = width (or height or depth) of texture
 * Output:
 *    i0, i1 = returns two nearest texel indexes
 *    weight = returns blend factor between texels
 */
static INLINE void
linear_texel_locations(GLenum wrapMode,
                       const struct gl_texture_image *img,
                       GLint size, GLfloat s,
                       GLint *i0, GLint *i1, GLfloat *weight)
{
   GLfloat u;
   switch (wrapMode) {
   case GL_REPEAT:
      u = s * size - 0.5F;
      if (img->_IsPowerOfTwo) {
         *i0 = IFLOOR(u) & (size - 1);
         *i1 = (*i0 + 1) & (size - 1);
      }
      else {
         *i0 = REMAINDER(IFLOOR(u), size);
         *i1 = REMAINDER(*i0 + 1, size);
      }
      break;
   case GL_CLAMP_TO_EDGE:
      if (s <= 0.0F)
         u = 0.0F;
      else if (s >= 1.0F)
         u = (GLfloat) size;
      else
         u = s * size;
      u -= 0.5F;
      *i0 = IFLOOR(u);
      *i1 = *i0 + 1;
      if (*i0 < 0)
         *i0 = 0;
      if (*i1 >= (GLint) size)
         *i1 = size - 1;
      break;
   case GL_CLAMP_TO_BORDER:
      {
         const GLfloat min = -1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         if (s <= min)
            u = min * size;
         else if (s >= max)
            u = max * size;
         else
            u = s * size;
         u -= 0.5F;
         *i0 = IFLOOR(u);
         *i1 = *i0 + 1;
      }
      break;
   case GL_MIRRORED_REPEAT:
      {
         const GLint flr = IFLOOR(s);
         if (flr & 1)
            u = 1.0F - (s - (GLfloat) flr);
         else
            u = s - (GLfloat) flr;
         u = (u * size) - 0.5F;
         *i0 = IFLOOR(u);
         *i1 = *i0 + 1;
         if (*i0 < 0)
            *i0 = 0;
         if (*i1 >= (GLint) size)
            *i1 = size - 1;
      }
      break;
   case GL_MIRROR_CLAMP_EXT:
      u = FABSF(s);
      if (u >= 1.0F)
         u = (GLfloat) size;
      else
         u *= size;
      u -= 0.5F;
      *i0 = IFLOOR(u);
      *i1 = *i0 + 1;
      break;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      u = FABSF(s);
      if (u >= 1.0F)
         u = (GLfloat) size;
      else
         u *= size;
      u -= 0.5F;
      *i0 = IFLOOR(u);
      *i1 = *i0 + 1;
      if (*i0 < 0)
         *i0 = 0;
      if (*i1 >= (GLint) size)
         *i1 = size - 1;
      break;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      {
         const GLfloat min = -1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         u = FABSF(s);
         if (u <= min)
            u = min * size;
         else if (u >= max)
            u = max * size;
         else
            u *= size;
         u -= 0.5F;
         *i0 = IFLOOR(u);
         *i1 = *i0 + 1;
      }
      break;
   case GL_CLAMP:
      if (s <= 0.0F)
         u = 0.0F;
      else if (s >= 1.0F)
         u = (GLfloat) size;
      else
         u = s * size;
      u -= 0.5F;
      *i0 = IFLOOR(u);
      *i1 = *i0 + 1;
      break;
   default:
      _mesa_problem(NULL, "Bad wrap mode");
      u = 0.0F;
   }
   *weight = FRAC(u);
}


/**
 * Used to compute texel location for nearest sampling.
 */
static INLINE GLint
nearest_texel_location(GLenum wrapMode,
                       const struct gl_texture_image *img,
                       GLint size, GLfloat s)
{
   GLint i;

   switch (wrapMode) {
   case GL_REPEAT:
      /* s limited to [0,1) */
      /* i limited to [0,size-1] */
      i = IFLOOR(s * size);
      if (img->_IsPowerOfTwo)
         i &= (size - 1);
      else
         i = REMAINDER(i, size);
      return i;
   case GL_CLAMP_TO_EDGE:
      {
         /* s limited to [min,max] */
         /* i limited to [0, size-1] */
         const GLfloat min = 1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         if (s < min)
            i = 0;
         else if (s > max)
            i = size - 1;
         else
            i = IFLOOR(s * size);
      }
      return i;
   case GL_CLAMP_TO_BORDER:
      {
         /* s limited to [min,max] */
         /* i limited to [-1, size] */
         const GLfloat min = -1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         if (s <= min)
            i = -1;
         else if (s >= max)
            i = size;
         else
            i = IFLOOR(s * size);
      }
      return i;
   case GL_MIRRORED_REPEAT:
      {
         const GLfloat min = 1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         const GLint flr = IFLOOR(s);
         GLfloat u;
         if (flr & 1)
            u = 1.0F - (s - (GLfloat) flr);
         else
            u = s - (GLfloat) flr;
         if (u < min)
            i = 0;
         else if (u > max)
            i = size - 1;
         else
            i = IFLOOR(u * size);
      }
      return i;
   case GL_MIRROR_CLAMP_EXT:
      {
         /* s limited to [0,1] */
         /* i limited to [0,size-1] */
         const GLfloat u = FABSF(s);
         if (u <= 0.0F)
            i = 0;
         else if (u >= 1.0F)
            i = size - 1;
         else
            i = IFLOOR(u * size);
      }
      return i;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      {
         /* s limited to [min,max] */
         /* i limited to [0, size-1] */
         const GLfloat min = 1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         const GLfloat u = FABSF(s);
         if (u < min)
            i = 0;
         else if (u > max)
            i = size - 1;
         else
            i = IFLOOR(u * size);
      }
      return i;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      {
         /* s limited to [min,max] */
         /* i limited to [0, size-1] */
         const GLfloat min = -1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         const GLfloat u = FABSF(s);
         if (u < min)
            i = -1;
         else if (u > max)
            i = size;
         else
            i = IFLOOR(u * size);
      }
      return i;
   case GL_CLAMP:
      /* s limited to [0,1] */
      /* i limited to [0,size-1] */
      if (s <= 0.0F)
         i = 0;
      else if (s >= 1.0F)
         i = size - 1;
      else
         i = IFLOOR(s * size);
      return i;
   default:
      _mesa_problem(NULL, "Bad wrap mode");
      return 0;
   }
}


/* Power of two image sizes only */
static INLINE void
linear_repeat_texel_location(GLuint size, GLfloat s,
                             GLint *i0, GLint *i1, GLfloat *weight)
{
   GLfloat u = s * size - 0.5F;
   *i0 = IFLOOR(u) & (size - 1);
   *i1 = (*i0 + 1) & (size - 1);
   *weight = FRAC(u);
}


/**
 * For linear interpolation between mipmap levels N and N+1, this function
 * computes N.
 */
static INLINE GLint
linear_mipmap_level(const struct gl_texture_object *tObj, GLfloat lambda)
{
   if (lambda < 0.0F)
      return tObj->BaseLevel;
   else if (lambda > tObj->_MaxLambda)
      return (GLint) (tObj->BaseLevel + tObj->_MaxLambda);
   else
      return (GLint) (tObj->BaseLevel + lambda);
}


/**
 * Compute the nearest mipmap level to take texels from.
 */
static INLINE GLint
nearest_mipmap_level(const struct gl_texture_object *tObj, GLfloat lambda)
{
   GLfloat l;
   GLint level;
   if (lambda <= 0.5F)
      l = 0.0F;
   else if (lambda > tObj->_MaxLambda + 0.4999F)
      l = tObj->_MaxLambda + 0.4999F;
   else
      l = lambda;
   level = (GLint) (tObj->BaseLevel + l + 0.5F);
   if (level > tObj->_MaxLevel)
      level = tObj->_MaxLevel;
   return level;
}



/*
 * Bitflags for texture border color sampling.
 */
#define I0BIT   1
#define I1BIT   2
#define J0BIT   4
#define J1BIT   8
#define K0BIT  16
#define K1BIT  32



/**
 * The lambda[] array values are always monotonic.  Either the whole span
 * will be minified, magnified, or split between the two.  This function
 * determines the subranges in [0, n-1] that are to be minified or magnified.
 */
static INLINE void
compute_min_mag_ranges(const struct gl_texture_object *tObj,
                       GLuint n, const GLfloat lambda[],
                       GLuint *minStart, GLuint *minEnd,
                       GLuint *magStart, GLuint *magEnd)
{
   GLfloat minMagThresh;

   /* we shouldn't be here if minfilter == magfilter */
   ASSERT(tObj->MinFilter != tObj->MagFilter);

   /* This bit comes from the OpenGL spec: */
   if (tObj->MagFilter == GL_LINEAR
       && (tObj->MinFilter == GL_NEAREST_MIPMAP_NEAREST ||
           tObj->MinFilter == GL_NEAREST_MIPMAP_LINEAR)) {
      minMagThresh = 0.5F;
   }
   else {
      minMagThresh = 0.0F;
   }

#if 0
   /* DEBUG CODE: Verify that lambda[] is monotonic.
    * We can't really use this because the inaccuracy in the LOG2 function
    * causes this test to fail, yet the resulting texturing is correct.
    */
   if (n > 1) {
      GLuint i;
      printf("lambda delta = %g\n", lambda[0] - lambda[n-1]);
      if (lambda[0] >= lambda[n-1]) { /* decreasing */
         for (i = 0; i < n - 1; i++) {
            ASSERT((GLint) (lambda[i] * 10) >= (GLint) (lambda[i+1] * 10));
         }
      }
      else { /* increasing */
         for (i = 0; i < n - 1; i++) {
            ASSERT((GLint) (lambda[i] * 10) <= (GLint) (lambda[i+1] * 10));
         }
      }
   }
#endif /* DEBUG */

   if (lambda[0] <= minMagThresh && (n <= 1 || lambda[n-1] <= minMagThresh)) {
      /* magnification for whole span */
      *magStart = 0;
      *magEnd = n;
      *minStart = *minEnd = 0;
   }
   else if (lambda[0] > minMagThresh && (n <=1 || lambda[n-1] > minMagThresh)) {
      /* minification for whole span */
      *minStart = 0;
      *minEnd = n;
      *magStart = *magEnd = 0;
   }
   else {
      /* a mix of minification and magnification */
      GLuint i;
      if (lambda[0] > minMagThresh) {
         /* start with minification */
         for (i = 1; i < n; i++) {
            if (lambda[i] <= minMagThresh)
               break;
         }
         *minStart = 0;
         *minEnd = i;
         *magStart = i;
         *magEnd = n;
      }
      else {
         /* start with magnification */
         for (i = 1; i < n; i++) {
            if (lambda[i] > minMagThresh)
               break;
         }
         *magStart = 0;
         *magEnd = i;
         *minStart = i;
         *minEnd = n;
      }
   }

#if 0
   /* Verify the min/mag Start/End values
    * We don't use this either (see above)
    */
   {
      GLint i;
      for (i = 0; i < n; i++) {
         if (lambda[i] > minMagThresh) {
            /* minification */
            ASSERT(i >= *minStart);
            ASSERT(i < *minEnd);
         }
         else {
            /* magnification */
            ASSERT(i >= *magStart);
            ASSERT(i < *magEnd);
         }
      }
   }
#endif
}


/**********************************************************************/
/*                    1-D Texture Sampling Functions                  */
/**********************************************************************/

/**
 * Return the texture sample for coordinate (s) using GL_NEAREST filter.
 */
static INLINE void
sample_1d_nearest(GLcontext *ctx,
                  const struct gl_texture_object *tObj,
                  const struct gl_texture_image *img,
                  const GLfloat texcoord[4], GLchan rgba[4])
{
   const GLint width = img->Width2;  /* without border, power of two */
   GLint i;
   i = nearest_texel_location(tObj->WrapS, img, width, texcoord[0]);
   /* skip over the border, if any */
   i += img->Border;
   if (i < 0 || i >= (GLint) img->Width) {
      /* Need this test for GL_CLAMP_TO_BORDER mode */
      COPY_CHAN4(rgba, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i, 0, 0, rgba);
   }
}


/**
 * Return the texture sample for coordinate (s) using GL_LINEAR filter.
 */
static INLINE void
sample_1d_linear(GLcontext *ctx,
                 const struct gl_texture_object *tObj,
                 const struct gl_texture_image *img,
                 const GLfloat texcoord[4], GLchan rgba[4])
{
   const GLint width = img->Width2;
   GLint i0, i1;
   GLbitfield useBorderColor = 0x0;
   GLfloat a;
   GLchan t0[4], t1[4];  /* texels */

   linear_texel_locations(tObj->WrapS, img, width, texcoord[0], &i0, &i1, &a);

   if (img->Border) {
      i0 += img->Border;
      i1 += img->Border;
   }
   else {
      if (i0 < 0 || i0 >= width)   useBorderColor |= I0BIT;
      if (i1 < 0 || i1 >= width)   useBorderColor |= I1BIT;
   }

   /* fetch texel colors */
   if (useBorderColor & I0BIT) {
      COPY_CHAN4(t0, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i0, 0, 0, t0);
   }
   if (useBorderColor & I1BIT) {
      COPY_CHAN4(t1, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i1, 0, 0, t1);
   }

   lerp_rgba(rgba, a, t0, t1);
}


static void
sample_1d_nearest_mipmap_nearest(GLcontext *ctx,
                                 const struct gl_texture_object *tObj,
                                 GLuint n, const GLfloat texcoord[][4],
                                 const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = nearest_mipmap_level(tObj, lambda[i]);
      sample_1d_nearest(ctx, tObj, tObj->Image[0][level], texcoord[i], rgba[i]);
   }
}


static void
sample_1d_linear_mipmap_nearest(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLuint n, const GLfloat texcoord[][4],
                                const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = nearest_mipmap_level(tObj, lambda[i]);
      sample_1d_linear(ctx, tObj, tObj->Image[0][level], texcoord[i], rgba[i]);
   }
}


static void
sample_1d_nearest_mipmap_linear(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLuint n, const GLfloat texcoord[][4],
                                const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      if (level >= tObj->_MaxLevel) {
         sample_1d_nearest(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                           texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];
         const GLfloat f = FRAC(lambda[i]);
         sample_1d_nearest(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_1d_nearest(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


static void
sample_1d_linear_mipmap_linear(GLcontext *ctx,
                               const struct gl_texture_object *tObj,
                               GLuint n, const GLfloat texcoord[][4],
                               const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      if (level >= tObj->_MaxLevel) {
         sample_1d_linear(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                          texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];
         const GLfloat f = FRAC(lambda[i]);
         sample_1d_linear(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_1d_linear(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


/** Sample 1D texture, nearest filtering for both min/magnification */
static void
sample_nearest_1d( GLcontext *ctx,
                   const struct gl_texture_object *tObj, GLuint n,
                   const GLfloat texcoords[][4], const GLfloat lambda[],
                   GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) lambda;
   for (i = 0; i < n; i++) {
      sample_1d_nearest(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}


/** Sample 1D texture, linear filtering for both min/magnification */
static void
sample_linear_1d( GLcontext *ctx,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat texcoords[][4], const GLfloat lambda[],
                  GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) lambda;
   for (i = 0; i < n; i++) {
      sample_1d_linear(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}


/** Sample 1D texture, using lambda to choose between min/magnification */
static void
sample_lambda_1d( GLcontext *ctx,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat texcoords[][4],
                  const GLfloat lambda[], GLchan rgba[][4] )
{
   GLuint minStart, minEnd;  /* texels with minification */
   GLuint magStart, magEnd;  /* texels with magnification */
   GLuint i;

   ASSERT(lambda != NULL);
   compute_min_mag_ranges(tObj, n, lambda,
                          &minStart, &minEnd, &magStart, &magEnd);

   if (minStart < minEnd) {
      /* do the minified texels */
      const GLuint m = minEnd - minStart;
      switch (tObj->MinFilter) {
      case GL_NEAREST:
         for (i = minStart; i < minEnd; i++)
            sample_1d_nearest(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                              texcoords[i], rgba[i]);
         break;
      case GL_LINEAR:
         for (i = minStart; i < minEnd; i++)
            sample_1d_linear(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                             texcoords[i], rgba[i]);
         break;
      case GL_NEAREST_MIPMAP_NEAREST:
         sample_1d_nearest_mipmap_nearest(ctx, tObj, m, texcoords + minStart,
                                          lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_NEAREST:
         sample_1d_linear_mipmap_nearest(ctx, tObj, m, texcoords + minStart,
                                         lambda + minStart, rgba + minStart);
         break;
      case GL_NEAREST_MIPMAP_LINEAR:
         sample_1d_nearest_mipmap_linear(ctx, tObj, m, texcoords + minStart,
                                         lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_LINEAR:
         sample_1d_linear_mipmap_linear(ctx, tObj, m, texcoords + minStart,
                                        lambda + minStart, rgba + minStart);
         break;
      default:
         _mesa_problem(ctx, "Bad min filter in sample_1d_texture");
         return;
      }
   }

   if (magStart < magEnd) {
      /* do the magnified texels */
      switch (tObj->MagFilter) {
      case GL_NEAREST:
         for (i = magStart; i < magEnd; i++)
            sample_1d_nearest(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                              texcoords[i], rgba[i]);
         break;
      case GL_LINEAR:
         for (i = magStart; i < magEnd; i++)
            sample_1d_linear(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                             texcoords[i], rgba[i]);
         break;
      default:
         _mesa_problem(ctx, "Bad mag filter in sample_1d_texture");
         return;
      }
   }
}


/**********************************************************************/
/*                    2-D Texture Sampling Functions                  */
/**********************************************************************/


/**
 * Return the texture sample for coordinate (s,t) using GL_NEAREST filter.
 */
static INLINE void
sample_2d_nearest(GLcontext *ctx,
                  const struct gl_texture_object *tObj,
                  const struct gl_texture_image *img,
                  const GLfloat texcoord[4],
                  GLchan rgba[])
{
   const GLint width = img->Width2;    /* without border, power of two */
   const GLint height = img->Height2;  /* without border, power of two */
   GLint i, j;
   (void) ctx;

   i = nearest_texel_location(tObj->WrapS, img, width, texcoord[0]);
   j = nearest_texel_location(tObj->WrapT, img, height, texcoord[1]);

   /* skip over the border, if any */
   i += img->Border;
   j += img->Border;

   if (i < 0 || i >= (GLint) img->Width || j < 0 || j >= (GLint) img->Height) {
      /* Need this test for GL_CLAMP_TO_BORDER mode */
      COPY_CHAN4(rgba, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i, j, 0, rgba);
   }
}


/**
 * Return the texture sample for coordinate (s,t) using GL_LINEAR filter.
 * New sampling code contributed by Lynn Quam <quam@ai.sri.com>.
 */
static INLINE void
sample_2d_linear(GLcontext *ctx,
                 const struct gl_texture_object *tObj,
                 const struct gl_texture_image *img,
                 const GLfloat texcoord[4],
                 GLchan rgba[])
{
   const GLint width = img->Width2;
   const GLint height = img->Height2;
   GLint i0, j0, i1, j1;
   GLbitfield useBorderColor = 0x0;
   GLfloat a, b;
   GLchan t00[4], t10[4], t01[4], t11[4]; /* sampled texel colors */

   linear_texel_locations(tObj->WrapS, img, width, texcoord[0],  &i0, &i1, &a);
   linear_texel_locations(tObj->WrapT, img, height, texcoord[1], &j0, &j1, &b);

   if (img->Border) {
      i0 += img->Border;
      i1 += img->Border;
      j0 += img->Border;
      j1 += img->Border;
   }
   else {
      if (i0 < 0 || i0 >= width)   useBorderColor |= I0BIT;
      if (i1 < 0 || i1 >= width)   useBorderColor |= I1BIT;
      if (j0 < 0 || j0 >= height)  useBorderColor |= J0BIT;
      if (j1 < 0 || j1 >= height)  useBorderColor |= J1BIT;
   }

   /* fetch four texel colors */
   if (useBorderColor & (I0BIT | J0BIT)) {
      COPY_CHAN4(t00, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i0, j0, 0, t00);
   }
   if (useBorderColor & (I1BIT | J0BIT)) {
      COPY_CHAN4(t10, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i1, j0, 0, t10);
   }
   if (useBorderColor & (I0BIT | J1BIT)) {
      COPY_CHAN4(t01, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i0, j1, 0, t01);
   }
   if (useBorderColor & (I1BIT | J1BIT)) {
      COPY_CHAN4(t11, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i1, j1, 0, t11);
   }

   lerp_rgba_2d(rgba, a, b, t00, t10, t01, t11);
}


/**
 * As above, but we know WRAP_S == REPEAT and WRAP_T == REPEAT.
 * We don't have to worry about the texture border.
 */
static INLINE void
sample_2d_linear_repeat(GLcontext *ctx,
                        const struct gl_texture_object *tObj,
                        const struct gl_texture_image *img,
                        const GLfloat texcoord[4],
                        GLchan rgba[])
{
   const GLint width = img->Width2;
   const GLint height = img->Height2;
   GLint i0, j0, i1, j1;
   GLfloat wi, wj;
   GLchan t00[4], t10[4], t01[4], t11[4]; /* sampled texel colors */

   (void) ctx;

   ASSERT(tObj->WrapS == GL_REPEAT);
   ASSERT(tObj->WrapT == GL_REPEAT);
   ASSERT(img->Border == 0);
   ASSERT(img->TexFormat->BaseFormat != GL_COLOR_INDEX);
   ASSERT(img->_IsPowerOfTwo);

   linear_repeat_texel_location(width,  texcoord[0], &i0, &i1, &wi);
   linear_repeat_texel_location(height, texcoord[1], &j0, &j1, &wj);

   img->FetchTexelc(img, i0, j0, 0, t00);
   img->FetchTexelc(img, i1, j0, 0, t10);
   img->FetchTexelc(img, i0, j1, 0, t01);
   img->FetchTexelc(img, i1, j1, 0, t11);

   lerp_rgba_2d(rgba, wi, wj, t00, t10, t01, t11);
}


static void
sample_2d_nearest_mipmap_nearest(GLcontext *ctx,
                                 const struct gl_texture_object *tObj,
                                 GLuint n, const GLfloat texcoord[][4],
                                 const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   for (i = 0; i < n; i++) {
      GLint level = nearest_mipmap_level(tObj, lambda[i]);
      sample_2d_nearest(ctx, tObj, tObj->Image[0][level], texcoord[i], rgba[i]);
   }
}


static void
sample_2d_linear_mipmap_nearest(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLuint n, const GLfloat texcoord[][4],
                                const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = nearest_mipmap_level(tObj, lambda[i]);
      sample_2d_linear(ctx, tObj, tObj->Image[0][level], texcoord[i], rgba[i]);
   }
}


static void
sample_2d_nearest_mipmap_linear(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLuint n, const GLfloat texcoord[][4],
                                const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      if (level >= tObj->_MaxLevel) {
         sample_2d_nearest(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                           texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_2d_nearest(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_2d_nearest(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


static void
sample_2d_linear_mipmap_linear( GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLuint n, const GLfloat texcoord[][4],
                                const GLfloat lambda[], GLchan rgba[][4] )
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      if (level >= tObj->_MaxLevel) {
         sample_2d_linear(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                          texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_2d_linear(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_2d_linear(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


static void
sample_2d_linear_mipmap_linear_repeat(GLcontext *ctx,
                                      const struct gl_texture_object *tObj,
                                      GLuint n, const GLfloat texcoord[][4],
                                      const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   ASSERT(tObj->WrapS == GL_REPEAT);
   ASSERT(tObj->WrapT == GL_REPEAT);
   for (i = 0; i < n; i++) {
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      if (level >= tObj->_MaxLevel) {
         sample_2d_linear_repeat(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                                 texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_2d_linear_repeat(ctx, tObj, tObj->Image[0][level  ],
                                 texcoord[i], t0);
         sample_2d_linear_repeat(ctx, tObj, tObj->Image[0][level+1],
                                 texcoord[i], t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


/** Sample 2D texture, nearest filtering for both min/magnification */
static void
sample_nearest_2d(GLcontext *ctx,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat texcoords[][4],
                  const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) lambda;
   for (i = 0; i < n; i++) {
      sample_2d_nearest(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}


/** Sample 2D texture, linear filtering for both min/magnification */
static void
sample_linear_2d(GLcontext *ctx,
                 const struct gl_texture_object *tObj, GLuint n,
                 const GLfloat texcoords[][4],
                 const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) lambda;
   if (tObj->WrapS == GL_REPEAT &&
       tObj->WrapT == GL_REPEAT &&
       image->_IsPowerOfTwo &&
       image->Border == 0) {
      for (i = 0; i < n; i++) {
         sample_2d_linear_repeat(ctx, tObj, image, texcoords[i], rgba[i]);
      }
   }
   else {
      for (i = 0; i < n; i++) {
         sample_2d_linear(ctx, tObj, image, texcoords[i], rgba[i]);
      }
   }
}


/**
 * Optimized 2-D texture sampling:
 *    S and T wrap mode == GL_REPEAT
 *    GL_NEAREST min/mag filter
 *    No border, 
 *    RowStride == Width,
 *    Format = GL_RGB
 */
static void
opt_sample_rgb_2d(GLcontext *ctx,
                  const struct gl_texture_object *tObj,
                  GLuint n, const GLfloat texcoords[][4],
                  const GLfloat lambda[], GLchan rgba[][4])
{
   const struct gl_texture_image *img = tObj->Image[0][tObj->BaseLevel];
   const GLfloat width = (GLfloat) img->Width;
   const GLfloat height = (GLfloat) img->Height;
   const GLint colMask = img->Width - 1;
   const GLint rowMask = img->Height - 1;
   const GLint shift = img->WidthLog2;
   GLuint k;
   (void) ctx;
   (void) lambda;
   ASSERT(tObj->WrapS==GL_REPEAT);
   ASSERT(tObj->WrapT==GL_REPEAT);
   ASSERT(img->Border==0);
   ASSERT(img->TexFormat->MesaFormat==MESA_FORMAT_RGB);
   ASSERT(img->_IsPowerOfTwo);

   for (k=0; k<n; k++) {
      GLint i = IFLOOR(texcoords[k][0] * width) & colMask;
      GLint j = IFLOOR(texcoords[k][1] * height) & rowMask;
      GLint pos = (j << shift) | i;
      GLchan *texel = ((GLchan *) img->Data) + 3*pos;
      rgba[k][RCOMP] = texel[0];
      rgba[k][GCOMP] = texel[1];
      rgba[k][BCOMP] = texel[2];
   }
}


/**
 * Optimized 2-D texture sampling:
 *    S and T wrap mode == GL_REPEAT
 *    GL_NEAREST min/mag filter
 *    No border
 *    RowStride == Width,
 *    Format = GL_RGBA
 */
static void
opt_sample_rgba_2d(GLcontext *ctx,
                   const struct gl_texture_object *tObj,
                   GLuint n, const GLfloat texcoords[][4],
                   const GLfloat lambda[], GLchan rgba[][4])
{
   const struct gl_texture_image *img = tObj->Image[0][tObj->BaseLevel];
   const GLfloat width = (GLfloat) img->Width;
   const GLfloat height = (GLfloat) img->Height;
   const GLint colMask = img->Width - 1;
   const GLint rowMask = img->Height - 1;
   const GLint shift = img->WidthLog2;
   GLuint i;
   (void) ctx;
   (void) lambda;
   ASSERT(tObj->WrapS==GL_REPEAT);
   ASSERT(tObj->WrapT==GL_REPEAT);
   ASSERT(img->Border==0);
   ASSERT(img->TexFormat->MesaFormat==MESA_FORMAT_RGBA);
   ASSERT(img->_IsPowerOfTwo);

   for (i = 0; i < n; i++) {
      const GLint col = IFLOOR(texcoords[i][0] * width) & colMask;
      const GLint row = IFLOOR(texcoords[i][1] * height) & rowMask;
      const GLint pos = (row << shift) | col;
      const GLchan *texel = ((GLchan *) img->Data) + (pos << 2);    /* pos*4 */
      COPY_CHAN4(rgba[i], texel);
   }
}


/** Sample 2D texture, using lambda to choose between min/magnification */
static void
sample_lambda_2d(GLcontext *ctx,
                 const struct gl_texture_object *tObj,
                 GLuint n, const GLfloat texcoords[][4],
                 const GLfloat lambda[], GLchan rgba[][4])
{
   const struct gl_texture_image *tImg = tObj->Image[0][tObj->BaseLevel];
   GLuint minStart, minEnd;  /* texels with minification */
   GLuint magStart, magEnd;  /* texels with magnification */

   const GLboolean repeatNoBorderPOT = (tObj->WrapS == GL_REPEAT)
      && (tObj->WrapT == GL_REPEAT)
      && (tImg->Border == 0 && (tImg->Width == tImg->RowStride))
      && (tImg->TexFormat->BaseFormat != GL_COLOR_INDEX)
      && tImg->_IsPowerOfTwo;

   ASSERT(lambda != NULL);
   compute_min_mag_ranges(tObj, n, lambda,
                          &minStart, &minEnd, &magStart, &magEnd);

   if (minStart < minEnd) {
      /* do the minified texels */
      const GLuint m = minEnd - minStart;
      switch (tObj->MinFilter) {
      case GL_NEAREST:
         if (repeatNoBorderPOT) {
            switch (tImg->TexFormat->MesaFormat) {
            case MESA_FORMAT_RGB:
               opt_sample_rgb_2d(ctx, tObj, m, texcoords + minStart,
                                 NULL, rgba + minStart);
               break;
            case MESA_FORMAT_RGBA:
	       opt_sample_rgba_2d(ctx, tObj, m, texcoords + minStart,
                                  NULL, rgba + minStart);
               break;
            default:
               sample_nearest_2d(ctx, tObj, m, texcoords + minStart,
                                 NULL, rgba + minStart );
            }
         }
         else {
            sample_nearest_2d(ctx, tObj, m, texcoords + minStart,
                              NULL, rgba + minStart);
         }
         break;
      case GL_LINEAR:
	 sample_linear_2d(ctx, tObj, m, texcoords + minStart,
			  NULL, rgba + minStart);
         break;
      case GL_NEAREST_MIPMAP_NEAREST:
         sample_2d_nearest_mipmap_nearest(ctx, tObj, m,
                                          texcoords + minStart,
                                          lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_NEAREST:
         sample_2d_linear_mipmap_nearest(ctx, tObj, m, texcoords + minStart,
                                         lambda + minStart, rgba + minStart);
         break;
      case GL_NEAREST_MIPMAP_LINEAR:
         sample_2d_nearest_mipmap_linear(ctx, tObj, m, texcoords + minStart,
                                         lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_LINEAR:
         if (repeatNoBorderPOT)
            sample_2d_linear_mipmap_linear_repeat(ctx, tObj, m,
                  texcoords + minStart, lambda + minStart, rgba + minStart);
         else
            sample_2d_linear_mipmap_linear(ctx, tObj, m, texcoords + minStart,
                                        lambda + minStart, rgba + minStart);
         break;
      default:
         _mesa_problem(ctx, "Bad min filter in sample_2d_texture");
         return;
      }
   }

   if (magStart < magEnd) {
      /* do the magnified texels */
      const GLuint m = magEnd - magStart;

      switch (tObj->MagFilter) {
      case GL_NEAREST:
         if (repeatNoBorderPOT) {
            switch (tImg->TexFormat->MesaFormat) {
            case MESA_FORMAT_RGB:
               opt_sample_rgb_2d(ctx, tObj, m, texcoords + magStart,
                                 NULL, rgba + magStart);
               break;
            case MESA_FORMAT_RGBA:
	       opt_sample_rgba_2d(ctx, tObj, m, texcoords + magStart,
                                  NULL, rgba + magStart);
               break;
            default:
               sample_nearest_2d(ctx, tObj, m, texcoords + magStart,
                                 NULL, rgba + magStart );
            }
         }
         else {
            sample_nearest_2d(ctx, tObj, m, texcoords + magStart,
                              NULL, rgba + magStart);
         }
         break;
      case GL_LINEAR:
	 sample_linear_2d(ctx, tObj, m, texcoords + magStart,
			  NULL, rgba + magStart);
         break;
      default:
         _mesa_problem(ctx, "Bad mag filter in sample_lambda_2d");
      }
   }
}



/**********************************************************************/
/*                    3-D Texture Sampling Functions                  */
/**********************************************************************/

/**
 * Return the texture sample for coordinate (s,t,r) using GL_NEAREST filter.
 */
static INLINE void
sample_3d_nearest(GLcontext *ctx,
                  const struct gl_texture_object *tObj,
                  const struct gl_texture_image *img,
                  const GLfloat texcoord[4],
                  GLchan rgba[4])
{
   const GLint width = img->Width2;     /* without border, power of two */
   const GLint height = img->Height2;   /* without border, power of two */
   const GLint depth = img->Depth2;     /* without border, power of two */
   GLint i, j, k;
   (void) ctx;

   i = nearest_texel_location(tObj->WrapS, img, width, texcoord[0]);
   j = nearest_texel_location(tObj->WrapT, img, height, texcoord[1]);
   k = nearest_texel_location(tObj->WrapR, img, depth, texcoord[2]);

   if (i < 0 || i >= (GLint) img->Width ||
       j < 0 || j >= (GLint) img->Height ||
       k < 0 || k >= (GLint) img->Depth) {
      /* Need this test for GL_CLAMP_TO_BORDER mode */
      COPY_CHAN4(rgba, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i, j, k, rgba);
   }
}


/**
 * Return the texture sample for coordinate (s,t,r) using GL_LINEAR filter.
 */
static void
sample_3d_linear(GLcontext *ctx,
                 const struct gl_texture_object *tObj,
                 const struct gl_texture_image *img,
                 const GLfloat texcoord[4],
                 GLchan rgba[4])
{
   const GLint width = img->Width2;
   const GLint height = img->Height2;
   const GLint depth = img->Depth2;
   GLint i0, j0, k0, i1, j1, k1;
   GLbitfield useBorderColor = 0x0;
   GLfloat a, b, c;
   GLchan t000[4], t010[4], t001[4], t011[4];
   GLchan t100[4], t110[4], t101[4], t111[4];

   linear_texel_locations(tObj->WrapS, img, width, texcoord[0],  &i0, &i1, &a);
   linear_texel_locations(tObj->WrapT, img, height, texcoord[1], &j0, &j1, &b);
   linear_texel_locations(tObj->WrapR, img, depth, texcoord[2],  &k0, &k1, &c);

   if (img->Border) {
      i0 += img->Border;
      i1 += img->Border;
      j0 += img->Border;
      j1 += img->Border;
      k0 += img->Border;
      k1 += img->Border;
   }
   else {
      /* check if sampling texture border color */
      if (i0 < 0 || i0 >= width)   useBorderColor |= I0BIT;
      if (i1 < 0 || i1 >= width)   useBorderColor |= I1BIT;
      if (j0 < 0 || j0 >= height)  useBorderColor |= J0BIT;
      if (j1 < 0 || j1 >= height)  useBorderColor |= J1BIT;
      if (k0 < 0 || k0 >= depth)   useBorderColor |= K0BIT;
      if (k1 < 0 || k1 >= depth)   useBorderColor |= K1BIT;
   }

   /* Fetch texels */
   if (useBorderColor & (I0BIT | J0BIT | K0BIT)) {
      COPY_CHAN4(t000, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i0, j0, k0, t000);
   }
   if (useBorderColor & (I1BIT | J0BIT | K0BIT)) {
      COPY_CHAN4(t100, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i1, j0, k0, t100);
   }
   if (useBorderColor & (I0BIT | J1BIT | K0BIT)) {
      COPY_CHAN4(t010, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i0, j1, k0, t010);
   }
   if (useBorderColor & (I1BIT | J1BIT | K0BIT)) {
      COPY_CHAN4(t110, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i1, j1, k0, t110);
   }

   if (useBorderColor & (I0BIT | J0BIT | K1BIT)) {
      COPY_CHAN4(t001, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i0, j0, k1, t001);
   }
   if (useBorderColor & (I1BIT | J0BIT | K1BIT)) {
      COPY_CHAN4(t101, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i1, j0, k1, t101);
   }
   if (useBorderColor & (I0BIT | J1BIT | K1BIT)) {
      COPY_CHAN4(t011, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i0, j1, k1, t011);
   }
   if (useBorderColor & (I1BIT | J1BIT | K1BIT)) {
      COPY_CHAN4(t111, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i1, j1, k1, t111);
   }

   /* trilinear interpolation of samples */
   lerp_rgba_3d(rgba, a, b, c, t000, t100, t010, t110, t001, t101, t011, t111);
}


static void
sample_3d_nearest_mipmap_nearest(GLcontext *ctx,
                                 const struct gl_texture_object *tObj,
                                 GLuint n, const GLfloat texcoord[][4],
                                 const GLfloat lambda[], GLchan rgba[][4] )
{
   GLuint i;
   for (i = 0; i < n; i++) {
      GLint level = nearest_mipmap_level(tObj, lambda[i]);
      sample_3d_nearest(ctx, tObj, tObj->Image[0][level], texcoord[i], rgba[i]);
   }
}


static void
sample_3d_linear_mipmap_nearest(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLuint n, const GLfloat texcoord[][4],
                                const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = nearest_mipmap_level(tObj, lambda[i]);
      sample_3d_linear(ctx, tObj, tObj->Image[0][level], texcoord[i], rgba[i]);
   }
}


static void
sample_3d_nearest_mipmap_linear(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLuint n, const GLfloat texcoord[][4],
                                const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      if (level >= tObj->_MaxLevel) {
         sample_3d_nearest(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                           texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_3d_nearest(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_3d_nearest(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


static void
sample_3d_linear_mipmap_linear(GLcontext *ctx,
                               const struct gl_texture_object *tObj,
                               GLuint n, const GLfloat texcoord[][4],
                               const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      if (level >= tObj->_MaxLevel) {
         sample_3d_linear(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                          texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_3d_linear(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_3d_linear(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


/** Sample 3D texture, nearest filtering for both min/magnification */
static void
sample_nearest_3d(GLcontext *ctx,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat texcoords[][4], const GLfloat lambda[],
                  GLchan rgba[][4])
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) lambda;
   for (i = 0; i < n; i++) {
      sample_3d_nearest(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}


/** Sample 3D texture, linear filtering for both min/magnification */
static void
sample_linear_3d(GLcontext *ctx,
                 const struct gl_texture_object *tObj, GLuint n,
                 const GLfloat texcoords[][4],
		 const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) lambda;
   for (i = 0; i < n; i++) {
      sample_3d_linear(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}


/** Sample 3D texture, using lambda to choose between min/magnification */
static void
sample_lambda_3d(GLcontext *ctx,
                 const struct gl_texture_object *tObj, GLuint n,
                 const GLfloat texcoords[][4], const GLfloat lambda[],
                 GLchan rgba[][4])
{
   GLuint minStart, minEnd;  /* texels with minification */
   GLuint magStart, magEnd;  /* texels with magnification */
   GLuint i;

   ASSERT(lambda != NULL);
   compute_min_mag_ranges(tObj, n, lambda,
                          &minStart, &minEnd, &magStart, &magEnd);

   if (minStart < minEnd) {
      /* do the minified texels */
      GLuint m = minEnd - minStart;
      switch (tObj->MinFilter) {
      case GL_NEAREST:
         for (i = minStart; i < minEnd; i++)
            sample_3d_nearest(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                              texcoords[i], rgba[i]);
         break;
      case GL_LINEAR:
         for (i = minStart; i < minEnd; i++)
            sample_3d_linear(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                             texcoords[i], rgba[i]);
         break;
      case GL_NEAREST_MIPMAP_NEAREST:
         sample_3d_nearest_mipmap_nearest(ctx, tObj, m, texcoords + minStart,
                                          lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_NEAREST:
         sample_3d_linear_mipmap_nearest(ctx, tObj, m, texcoords + minStart,
                                         lambda + minStart, rgba + minStart);
         break;
      case GL_NEAREST_MIPMAP_LINEAR:
         sample_3d_nearest_mipmap_linear(ctx, tObj, m, texcoords + minStart,
                                         lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_LINEAR:
         sample_3d_linear_mipmap_linear(ctx, tObj, m, texcoords + minStart,
                                        lambda + minStart, rgba + minStart);
         break;
      default:
         _mesa_problem(ctx, "Bad min filter in sample_3d_texture");
         return;
      }
   }

   if (magStart < magEnd) {
      /* do the magnified texels */
      switch (tObj->MagFilter) {
      case GL_NEAREST:
         for (i = magStart; i < magEnd; i++)
            sample_3d_nearest(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                              texcoords[i], rgba[i]);
         break;
      case GL_LINEAR:
         for (i = magStart; i < magEnd; i++)
            sample_3d_linear(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                             texcoords[i], rgba[i]);
         break;
      default:
         _mesa_problem(ctx, "Bad mag filter in sample_3d_texture");
         return;
      }
   }
}


/**********************************************************************/
/*                Texture Cube Map Sampling Functions                 */
/**********************************************************************/

/**
 * Choose one of six sides of a texture cube map given the texture
 * coord (rx,ry,rz).  Return pointer to corresponding array of texture
 * images.
 */
static const struct gl_texture_image **
choose_cube_face(const struct gl_texture_object *texObj,
                 const GLfloat texcoord[4], GLfloat newCoord[4])
{
   /*
      major axis
      direction     target                             sc     tc    ma
      ----------    -------------------------------    ---    ---   ---
       +rx          TEXTURE_CUBE_MAP_POSITIVE_X_EXT    -rz    -ry   rx
       -rx          TEXTURE_CUBE_MAP_NEGATIVE_X_EXT    +rz    -ry   rx
       +ry          TEXTURE_CUBE_MAP_POSITIVE_Y_EXT    +rx    +rz   ry
       -ry          TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT    +rx    -rz   ry
       +rz          TEXTURE_CUBE_MAP_POSITIVE_Z_EXT    +rx    -ry   rz
       -rz          TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT    -rx    -ry   rz
   */
   const GLfloat rx = texcoord[0];
   const GLfloat ry = texcoord[1];
   const GLfloat rz = texcoord[2];
   const GLfloat arx = FABSF(rx), ary = FABSF(ry), arz = FABSF(rz);
   GLuint face;
   GLfloat sc, tc, ma;

   if (arx > ary && arx > arz) {
      if (rx >= 0.0F) {
         face = FACE_POS_X;
         sc = -rz;
         tc = -ry;
         ma = arx;
      }
      else {
         face = FACE_NEG_X;
         sc = rz;
         tc = -ry;
         ma = arx;
      }
   }
   else if (ary > arx && ary > arz) {
      if (ry >= 0.0F) {
         face = FACE_POS_Y;
         sc = rx;
         tc = rz;
         ma = ary;
      }
      else {
         face = FACE_NEG_Y;
         sc = rx;
         tc = -rz;
         ma = ary;
      }
   }
   else {
      if (rz > 0.0F) {
         face = FACE_POS_Z;
         sc = rx;
         tc = -ry;
         ma = arz;
      }
      else {
         face = FACE_NEG_Z;
         sc = -rx;
         tc = -ry;
         ma = arz;
      }
   }

   newCoord[0] = ( sc / ma + 1.0F ) * 0.5F;
   newCoord[1] = ( tc / ma + 1.0F ) * 0.5F;
   return (const struct gl_texture_image **) texObj->Image[face];
}


static void
sample_nearest_cube(GLcontext *ctx,
		    const struct gl_texture_object *tObj, GLuint n,
                    const GLfloat texcoords[][4], const GLfloat lambda[],
                    GLchan rgba[][4])
{
   GLuint i;
   (void) lambda;
   for (i = 0; i < n; i++) {
      const struct gl_texture_image **images;
      GLfloat newCoord[4];
      images = choose_cube_face(tObj, texcoords[i], newCoord);
      sample_2d_nearest(ctx, tObj, images[tObj->BaseLevel],
                        newCoord, rgba[i]);
   }
}


static void
sample_linear_cube(GLcontext *ctx,
		   const struct gl_texture_object *tObj, GLuint n,
                   const GLfloat texcoords[][4],
		   const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   (void) lambda;
   for (i = 0; i < n; i++) {
      const struct gl_texture_image **images;
      GLfloat newCoord[4];
      images = choose_cube_face(tObj, texcoords[i], newCoord);
      sample_2d_linear(ctx, tObj, images[tObj->BaseLevel],
                       newCoord, rgba[i]);
   }
}


static void
sample_cube_nearest_mipmap_nearest(GLcontext *ctx,
                                   const struct gl_texture_object *tObj,
                                   GLuint n, const GLfloat texcoord[][4],
                                   const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      const struct gl_texture_image **images;
      GLfloat newCoord[4];
      GLint level;
      images = choose_cube_face(tObj, texcoord[i], newCoord);

      /* XXX we actually need to recompute lambda here based on the newCoords.
       * But we would need the texcoords of adjacent fragments to compute that
       * properly, and we don't have those here.
       * For now, do an approximation:  subtracting 1 from the chosen mipmap
       * level seems to work in some test cases.
       * The same adjustment is done in the next few functions.
      */
      level = nearest_mipmap_level(tObj, lambda[i]);
      level = MAX2(level - 1, 0);

      sample_2d_nearest(ctx, tObj, images[level], newCoord, rgba[i]);
   }
}


static void
sample_cube_linear_mipmap_nearest(GLcontext *ctx,
                                  const struct gl_texture_object *tObj,
                                  GLuint n, const GLfloat texcoord[][4],
                                  const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      const struct gl_texture_image **images;
      GLfloat newCoord[4];
      GLint level = nearest_mipmap_level(tObj, lambda[i]);
      level = MAX2(level - 1, 0); /* see comment above */
      images = choose_cube_face(tObj, texcoord[i], newCoord);
      sample_2d_linear(ctx, tObj, images[level], newCoord, rgba[i]);
   }
}


static void
sample_cube_nearest_mipmap_linear(GLcontext *ctx,
                                  const struct gl_texture_object *tObj,
                                  GLuint n, const GLfloat texcoord[][4],
                                  const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      const struct gl_texture_image **images;
      GLfloat newCoord[4];
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      level = MAX2(level - 1, 0); /* see comment above */
      images = choose_cube_face(tObj, texcoord[i], newCoord);
      if (level >= tObj->_MaxLevel) {
         sample_2d_nearest(ctx, tObj, images[tObj->_MaxLevel],
                           newCoord, rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_2d_nearest(ctx, tObj, images[level  ], newCoord, t0);
         sample_2d_nearest(ctx, tObj, images[level+1], newCoord, t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


static void
sample_cube_linear_mipmap_linear(GLcontext *ctx,
                                 const struct gl_texture_object *tObj,
                                 GLuint n, const GLfloat texcoord[][4],
                                 const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      const struct gl_texture_image **images;
      GLfloat newCoord[4];
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      level = MAX2(level - 1, 0); /* see comment above */
      images = choose_cube_face(tObj, texcoord[i], newCoord);
      if (level >= tObj->_MaxLevel) {
         sample_2d_linear(ctx, tObj, images[tObj->_MaxLevel],
                          newCoord, rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];
         const GLfloat f = FRAC(lambda[i]);
         sample_2d_linear(ctx, tObj, images[level  ], newCoord, t0);
         sample_2d_linear(ctx, tObj, images[level+1], newCoord, t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


/** Sample cube texture, using lambda to choose between min/magnification */
static void
sample_lambda_cube(GLcontext *ctx,
		   const struct gl_texture_object *tObj, GLuint n,
		   const GLfloat texcoords[][4], const GLfloat lambda[],
		   GLchan rgba[][4])
{
   GLuint minStart, minEnd;  /* texels with minification */
   GLuint magStart, magEnd;  /* texels with magnification */

   ASSERT(lambda != NULL);
   compute_min_mag_ranges(tObj, n, lambda,
                          &minStart, &minEnd, &magStart, &magEnd);

   if (minStart < minEnd) {
      /* do the minified texels */
      const GLuint m = minEnd - minStart;
      switch (tObj->MinFilter) {
      case GL_NEAREST:
         sample_nearest_cube(ctx, tObj, m, texcoords + minStart,
                             lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR:
         sample_linear_cube(ctx, tObj, m, texcoords + minStart,
                            lambda + minStart, rgba + minStart);
         break;
      case GL_NEAREST_MIPMAP_NEAREST:
         sample_cube_nearest_mipmap_nearest(ctx, tObj, m,
                                            texcoords + minStart,
                                           lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_NEAREST:
         sample_cube_linear_mipmap_nearest(ctx, tObj, m,
                                           texcoords + minStart,
                                           lambda + minStart, rgba + minStart);
         break;
      case GL_NEAREST_MIPMAP_LINEAR:
         sample_cube_nearest_mipmap_linear(ctx, tObj, m,
                                           texcoords + minStart,
                                           lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_LINEAR:
         sample_cube_linear_mipmap_linear(ctx, tObj, m,
                                          texcoords + minStart,
                                          lambda + minStart, rgba + minStart);
         break;
      default:
         _mesa_problem(ctx, "Bad min filter in sample_lambda_cube");
      }
   }

   if (magStart < magEnd) {
      /* do the magnified texels */
      const GLuint m = magEnd - magStart;
      switch (tObj->MagFilter) {
      case GL_NEAREST:
         sample_nearest_cube(ctx, tObj, m, texcoords + magStart,
                             lambda + magStart, rgba + magStart);
         break;
      case GL_LINEAR:
         sample_linear_cube(ctx, tObj, m, texcoords + magStart,
                            lambda + magStart, rgba + magStart);
         break;
      default:
         _mesa_problem(ctx, "Bad mag filter in sample_lambda_cube");
      }
   }
}


/**********************************************************************/
/*               Texture Rectangle Sampling Functions                 */
/**********************************************************************/


/**
 * Do clamp/wrap for a texture rectangle coord, GL_NEAREST filter mode.
 */
static INLINE GLint
clamp_rect_coord_nearest(GLenum wrapMode, GLfloat coord, GLint max)
{
   switch (wrapMode) {
   case GL_CLAMP:
      return IFLOOR( CLAMP(coord, 0.0F, max - 1) );
   case GL_CLAMP_TO_EDGE:
      return IFLOOR( CLAMP(coord, 0.5F, max - 0.5F) );
   case GL_CLAMP_TO_BORDER:
      return IFLOOR( CLAMP(coord, -0.5F, max + 0.5F) );
   default:
      _mesa_problem(NULL, "bad wrapMode in clamp_rect_coord_nearest");
      return 0;
   }
}


/**
 * As above, but GL_LINEAR filtering.
 */
static INLINE void
clamp_rect_coord_linear(GLenum wrapMode, GLfloat coord, GLint max,
                        GLint *i0out, GLint *i1out, GLfloat *weight)
{
   GLfloat fcol;
   GLint i0, i1;
   switch (wrapMode) {
   case GL_CLAMP:
      /* Not exactly what the spec says, but it matches NVIDIA output */
      fcol = CLAMP(coord - 0.5F, 0.0, max-1);
      i0 = IFLOOR(fcol);
      i1 = i0 + 1;
      break;
   case GL_CLAMP_TO_EDGE:
      fcol = CLAMP(coord, 0.5F, max - 0.5F);
      fcol -= 0.5F;
      i0 = IFLOOR(fcol);
      i1 = i0 + 1;
      if (i1 > max - 1)
         i1 = max - 1;
      break;
   case GL_CLAMP_TO_BORDER:
      fcol = CLAMP(coord, -0.5F, max + 0.5F);
      fcol -= 0.5F;
      i0 = IFLOOR(fcol);
      i1 = i0 + 1;
   default:
      _mesa_problem(NULL, "bad wrapMode in clamp_rect_coord_linear");
      i0 = i1 = 0;
      fcol = 0.0F;
   }
   *i0out = i0;
   *i1out = i1;
   *weight = FRAC(fcol);
}


static void
sample_nearest_rect(GLcontext *ctx,
		    const struct gl_texture_object *tObj, GLuint n,
                    const GLfloat texcoords[][4], const GLfloat lambda[],
                    GLchan rgba[][4])
{
   const struct gl_texture_image *img = tObj->Image[0][0];
   const GLint width = img->Width;
   const GLint height = img->Height;
   GLuint i;

   (void) ctx;
   (void) lambda;

   ASSERT(tObj->WrapS == GL_CLAMP ||
          tObj->WrapS == GL_CLAMP_TO_EDGE ||
          tObj->WrapS == GL_CLAMP_TO_BORDER);
   ASSERT(tObj->WrapT == GL_CLAMP ||
          tObj->WrapT == GL_CLAMP_TO_EDGE ||
          tObj->WrapT == GL_CLAMP_TO_BORDER);
   ASSERT(img->TexFormat->BaseFormat != GL_COLOR_INDEX);

   for (i = 0; i < n; i++) {
      GLint row, col;
      col = clamp_rect_coord_nearest(tObj->WrapS, texcoords[i][0], width);
      row = clamp_rect_coord_nearest(tObj->WrapT, texcoords[i][1], height);
      if (col < 0 || col >= width || row < 0 || row >= height)
         COPY_CHAN4(rgba[i], tObj->_BorderChan);
      else
         img->FetchTexelc(img, col, row, 0, rgba[i]);
   }
}


static void
sample_linear_rect(GLcontext *ctx,
		   const struct gl_texture_object *tObj, GLuint n,
                   const GLfloat texcoords[][4],
		   const GLfloat lambda[], GLchan rgba[][4])
{
   const struct gl_texture_image *img = tObj->Image[0][0];
   const GLint width = img->Width;
   const GLint height = img->Height;
   GLuint i;

   (void) ctx;
   (void) lambda;

   ASSERT(tObj->WrapS == GL_CLAMP ||
          tObj->WrapS == GL_CLAMP_TO_EDGE ||
          tObj->WrapS == GL_CLAMP_TO_BORDER);
   ASSERT(tObj->WrapT == GL_CLAMP ||
          tObj->WrapT == GL_CLAMP_TO_EDGE ||
          tObj->WrapT == GL_CLAMP_TO_BORDER);
   ASSERT(img->TexFormat->BaseFormat != GL_COLOR_INDEX);

   for (i = 0; i < n; i++) {
      GLint i0, j0, i1, j1;
      GLchan t00[4], t01[4], t10[4], t11[4];
      GLfloat a, b;
      GLbitfield useBorderColor = 0x0;

      clamp_rect_coord_linear(tObj->WrapS, texcoords[i][0], width,
                              &i0, &i1, &a);
      clamp_rect_coord_linear(tObj->WrapT, texcoords[i][1], height,
                              &j0, &j1, &b);

      /* compute integer rows/columns */
      if (i0 < 0 || i0 >= width)   useBorderColor |= I0BIT;
      if (i1 < 0 || i1 >= width)   useBorderColor |= I1BIT;
      if (j0 < 0 || j0 >= height)  useBorderColor |= J0BIT;
      if (j1 < 0 || j1 >= height)  useBorderColor |= J1BIT;

      /* get four texel samples */
      if (useBorderColor & (I0BIT | J0BIT))
         COPY_CHAN4(t00, tObj->_BorderChan);
      else
         img->FetchTexelc(img, i0, j0, 0, t00);

      if (useBorderColor & (I1BIT | J0BIT))
         COPY_CHAN4(t10, tObj->_BorderChan);
      else
         img->FetchTexelc(img, i1, j0, 0, t10);

      if (useBorderColor & (I0BIT | J1BIT))
         COPY_CHAN4(t01, tObj->_BorderChan);
      else
         img->FetchTexelc(img, i0, j1, 0, t01);

      if (useBorderColor & (I1BIT | J1BIT))
         COPY_CHAN4(t11, tObj->_BorderChan);
      else
         img->FetchTexelc(img, i1, j1, 0, t11);

      lerp_rgba_2d(rgba[i], a, b, t00, t10, t01, t11);
   }
}


/** Sample Rect texture, using lambda to choose between min/magnification */
static void
sample_lambda_rect(GLcontext *ctx,
		   const struct gl_texture_object *tObj, GLuint n,
		   const GLfloat texcoords[][4], const GLfloat lambda[],
		   GLchan rgba[][4])
{
   GLuint minStart, minEnd, magStart, magEnd;

   /* We only need lambda to decide between minification and magnification.
    * There is no mipmapping with rectangular textures.
    */
   compute_min_mag_ranges(tObj, n, lambda,
                          &minStart, &minEnd, &magStart, &magEnd);

   if (minStart < minEnd) {
      if (tObj->MinFilter == GL_NEAREST) {
         sample_nearest_rect(ctx, tObj, minEnd - minStart,
                             texcoords + minStart, NULL, rgba + minStart);
      }
      else {
         sample_linear_rect(ctx, tObj, minEnd - minStart,
                            texcoords + minStart, NULL, rgba + minStart);
      }
   }
   if (magStart < magEnd) {
      if (tObj->MagFilter == GL_NEAREST) {
         sample_nearest_rect(ctx, tObj, magEnd - magStart,
                             texcoords + magStart, NULL, rgba + magStart);
      }
      else {
         sample_linear_rect(ctx, tObj, magEnd - magStart,
                            texcoords + magStart, NULL, rgba + magStart);
      }
   }
}



/**********************************************************************/
/*                2D Texture Array Sampling Functions                 */
/**********************************************************************/

/**
 * Return the texture sample for coordinate (s,t,r) using GL_NEAREST filter.
 */
static void
sample_2d_array_nearest(GLcontext *ctx,
                        const struct gl_texture_object *tObj,
                        const struct gl_texture_image *img,
                        const GLfloat texcoord[4],
                        GLchan rgba[4])
{
   const GLint width = img->Width2;     /* without border, power of two */
   const GLint height = img->Height2;   /* without border, power of two */
   const GLint depth = img->Depth;
   GLint i, j;
   GLint array;
   (void) ctx;

   i = nearest_texel_location(tObj->WrapS, img, width, texcoord[0]);
   j = nearest_texel_location(tObj->WrapT, img, height, texcoord[1]);
   array = clamp_rect_coord_nearest(tObj->WrapR, texcoord[2], depth);

   if (i < 0 || i >= (GLint) img->Width ||
       j < 0 || j >= (GLint) img->Height ||
       array < 0 || array >= (GLint) img->Depth) {
      /* Need this test for GL_CLAMP_TO_BORDER mode */
      COPY_CHAN4(rgba, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i, j, array, rgba);
   }
}


/**
 * Return the texture sample for coordinate (s,t,r) using GL_LINEAR filter.
 */
static void
sample_2d_array_linear(GLcontext *ctx,
                       const struct gl_texture_object *tObj,
                       const struct gl_texture_image *img,
                       const GLfloat texcoord[4],
                       GLchan rgba[4])
{
   const GLint width = img->Width2;
   const GLint height = img->Height2;
   const GLint depth = img->Depth;
   GLint i0, j0, i1, j1;
   GLint array;
   GLbitfield useBorderColor = 0x0;
   GLfloat a, b;
   GLchan t00[4], t01[4], t10[4], t11[4];

   linear_texel_locations(tObj->WrapS, img, width,  texcoord[0], &i0, &i1, &a);
   linear_texel_locations(tObj->WrapT, img, height, texcoord[1], &j0, &j1, &b);
   array = clamp_rect_coord_nearest(tObj->WrapR, texcoord[2], depth);

   if (array < 0 || array >= depth) {
      COPY_CHAN4(rgba, tObj->_BorderChan);
   }
   else {
      if (img->Border) {
	 i0 += img->Border;
	 i1 += img->Border;
	 j0 += img->Border;
	 j1 += img->Border;
      }
      else {
	 /* check if sampling texture border color */
	 if (i0 < 0 || i0 >= width)   useBorderColor |= I0BIT;
	 if (i1 < 0 || i1 >= width)   useBorderColor |= I1BIT;
	 if (j0 < 0 || j0 >= height)  useBorderColor |= J0BIT;
	 if (j1 < 0 || j1 >= height)  useBorderColor |= J1BIT;
      }

      /* Fetch texels */
      if (useBorderColor & (I0BIT | J0BIT)) {
	 COPY_CHAN4(t00, tObj->_BorderChan);
      }
      else {
	 img->FetchTexelc(img, i0, j0, array, t00);
      }
      if (useBorderColor & (I1BIT | J0BIT)) {
	 COPY_CHAN4(t10, tObj->_BorderChan);
      }
      else {
	 img->FetchTexelc(img, i1, j0, array, t10);
      }
      if (useBorderColor & (I0BIT | J1BIT)) {
	 COPY_CHAN4(t01, tObj->_BorderChan);
      }
      else {
	 img->FetchTexelc(img, i0, j1, array, t01);
      }
      if (useBorderColor & (I1BIT | J1BIT)) {
	 COPY_CHAN4(t11, tObj->_BorderChan);
      }
      else {
	 img->FetchTexelc(img, i1, j1, array, t11);
      }
      
      /* trilinear interpolation of samples */
      lerp_rgba_2d(rgba, a, b, t00, t10, t01, t11);
   }
}


static void
sample_2d_array_nearest_mipmap_nearest(GLcontext *ctx,
                                       const struct gl_texture_object *tObj,
                                       GLuint n, const GLfloat texcoord[][4],
                                       const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   for (i = 0; i < n; i++) {
      GLint level = nearest_mipmap_level(tObj, lambda[i]);
      sample_2d_array_nearest(ctx, tObj, tObj->Image[0][level], texcoord[i],
                              rgba[i]);
   }
}


static void
sample_2d_array_linear_mipmap_nearest(GLcontext *ctx,
                                      const struct gl_texture_object *tObj,
                                      GLuint n, const GLfloat texcoord[][4],
                                      const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = nearest_mipmap_level(tObj, lambda[i]);
      sample_2d_array_linear(ctx, tObj, tObj->Image[0][level],
                             texcoord[i], rgba[i]);
   }
}


static void
sample_2d_array_nearest_mipmap_linear(GLcontext *ctx,
                                      const struct gl_texture_object *tObj,
                                      GLuint n, const GLfloat texcoord[][4],
                                      const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      if (level >= tObj->_MaxLevel) {
         sample_2d_array_nearest(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                                 texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_2d_array_nearest(ctx, tObj, tObj->Image[0][level  ],
                                 texcoord[i], t0);
         sample_2d_array_nearest(ctx, tObj, tObj->Image[0][level+1],
                                 texcoord[i], t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


static void
sample_2d_array_linear_mipmap_linear(GLcontext *ctx,
                                     const struct gl_texture_object *tObj,
                                     GLuint n, const GLfloat texcoord[][4],
                                     const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      if (level >= tObj->_MaxLevel) {
         sample_2d_array_linear(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                          texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_2d_array_linear(ctx, tObj, tObj->Image[0][level  ],
                                texcoord[i], t0);
         sample_2d_array_linear(ctx, tObj, tObj->Image[0][level+1],
                                texcoord[i], t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


/** Sample 2D Array texture, nearest filtering for both min/magnification */
static void
sample_nearest_2d_array(GLcontext *ctx,
                        const struct gl_texture_object *tObj, GLuint n,
                        const GLfloat texcoords[][4], const GLfloat lambda[],
                        GLchan rgba[][4])
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) lambda;
   for (i = 0; i < n; i++) {
      sample_2d_array_nearest(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}



/** Sample 2D Array texture, linear filtering for both min/magnification */
static void
sample_linear_2d_array(GLcontext *ctx,
                       const struct gl_texture_object *tObj, GLuint n,
                       const GLfloat texcoords[][4],
                       const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) lambda;
   for (i = 0; i < n; i++) {
      sample_2d_array_linear(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}


/** Sample 2D Array texture, using lambda to choose between min/magnification */
static void
sample_lambda_2d_array(GLcontext *ctx,
                       const struct gl_texture_object *tObj, GLuint n,
                       const GLfloat texcoords[][4], const GLfloat lambda[],
                       GLchan rgba[][4])
{
   GLuint minStart, minEnd;  /* texels with minification */
   GLuint magStart, magEnd;  /* texels with magnification */
   GLuint i;

   ASSERT(lambda != NULL);
   compute_min_mag_ranges(tObj, n, lambda,
                          &minStart, &minEnd, &magStart, &magEnd);

   if (minStart < minEnd) {
      /* do the minified texels */
      GLuint m = minEnd - minStart;
      switch (tObj->MinFilter) {
      case GL_NEAREST:
         for (i = minStart; i < minEnd; i++)
            sample_2d_array_nearest(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                                    texcoords[i], rgba[i]);
         break;
      case GL_LINEAR:
         for (i = minStart; i < minEnd; i++)
            sample_2d_array_linear(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                                   texcoords[i], rgba[i]);
         break;
      case GL_NEAREST_MIPMAP_NEAREST:
         sample_2d_array_nearest_mipmap_nearest(ctx, tObj, m,
                                                texcoords + minStart,
                                                lambda + minStart,
                                                rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_NEAREST:
         sample_2d_array_linear_mipmap_nearest(ctx, tObj, m, 
                                               texcoords + minStart,
                                               lambda + minStart,
                                               rgba + minStart);
         break;
      case GL_NEAREST_MIPMAP_LINEAR:
         sample_2d_array_nearest_mipmap_linear(ctx, tObj, m,
                                               texcoords + minStart,
                                               lambda + minStart,
                                               rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_LINEAR:
         sample_2d_array_linear_mipmap_linear(ctx, tObj, m, 
                                              texcoords + minStart,
                                              lambda + minStart, 
                                              rgba + minStart);
         break;
      default:
         _mesa_problem(ctx, "Bad min filter in sample_2d_array_texture");
         return;
      }
   }

   if (magStart < magEnd) {
      /* do the magnified texels */
      switch (tObj->MagFilter) {
      case GL_NEAREST:
         for (i = magStart; i < magEnd; i++)
            sample_2d_array_nearest(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                              texcoords[i], rgba[i]);
         break;
      case GL_LINEAR:
         for (i = magStart; i < magEnd; i++)
            sample_2d_array_linear(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                                   texcoords[i], rgba[i]);
         break;
      default:
         _mesa_problem(ctx, "Bad mag filter in sample_2d_array_texture");
         return;
      }
   }
}




/**********************************************************************/
/*                1D Texture Array Sampling Functions                 */
/**********************************************************************/

/**
 * Return the texture sample for coordinate (s,t,r) using GL_NEAREST filter.
 */
static void
sample_1d_array_nearest(GLcontext *ctx,
                        const struct gl_texture_object *tObj,
                        const struct gl_texture_image *img,
                        const GLfloat texcoord[4],
                        GLchan rgba[4])
{
   const GLint width = img->Width2;     /* without border, power of two */
   const GLint height = img->Height;
   GLint i;
   GLint array;
   (void) ctx;

   i = nearest_texel_location(tObj->WrapS, img, width, texcoord[0]);
   array = clamp_rect_coord_nearest(tObj->WrapT, texcoord[1], height);

   if (i < 0 || i >= (GLint) img->Width ||
       array < 0 || array >= (GLint) img->Height) {
      /* Need this test for GL_CLAMP_TO_BORDER mode */
      COPY_CHAN4(rgba, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i, array, 0, rgba);
   }
}


/**
 * Return the texture sample for coordinate (s,t,r) using GL_LINEAR filter.
 */
static void
sample_1d_array_linear(GLcontext *ctx,
                       const struct gl_texture_object *tObj,
                       const struct gl_texture_image *img,
                       const GLfloat texcoord[4],
                       GLchan rgba[4])
{
   const GLint width = img->Width2;
   const GLint height = img->Height;
   GLint i0, i1;
   GLint array;
   GLbitfield useBorderColor = 0x0;
   GLfloat a;
   GLchan t0[4], t1[4];

   linear_texel_locations(tObj->WrapS, img, width, texcoord[0], &i0, &i1, &a);
   array = clamp_rect_coord_nearest(tObj->WrapT, texcoord[1], height);

   if (img->Border) {
      i0 += img->Border;
      i1 += img->Border;
   }
   else {
      /* check if sampling texture border color */
      if (i0 < 0 || i0 >= width)   useBorderColor |= I0BIT;
      if (i1 < 0 || i1 >= width)   useBorderColor |= I1BIT;
   }

   if (array < 0 || array >= height)   useBorderColor |= K0BIT;

   /* Fetch texels */
   if (useBorderColor & (I0BIT | K0BIT)) {
      COPY_CHAN4(t0, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i0, array, 0, t0);
   }
   if (useBorderColor & (I1BIT | K0BIT)) {
      COPY_CHAN4(t1, tObj->_BorderChan);
   }
   else {
      img->FetchTexelc(img, i1, array, 0, t1);
   }

   /* bilinear interpolation of samples */
   lerp_rgba(rgba, a, t0, t1);
}


static void
sample_1d_array_nearest_mipmap_nearest(GLcontext *ctx,
                                       const struct gl_texture_object *tObj,
                                       GLuint n, const GLfloat texcoord[][4],
                                       const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   for (i = 0; i < n; i++) {
      GLint level = nearest_mipmap_level(tObj, lambda[i]);
      sample_1d_array_nearest(ctx, tObj, tObj->Image[0][level], texcoord[i],
                              rgba[i]);
   }
}


static void
sample_1d_array_linear_mipmap_nearest(GLcontext *ctx,
                                      const struct gl_texture_object *tObj,
                                      GLuint n, const GLfloat texcoord[][4],
                                      const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = nearest_mipmap_level(tObj, lambda[i]);
      sample_1d_array_linear(ctx, tObj, tObj->Image[0][level],
                             texcoord[i], rgba[i]);
   }
}


static void
sample_1d_array_nearest_mipmap_linear(GLcontext *ctx,
                                      const struct gl_texture_object *tObj,
                                      GLuint n, const GLfloat texcoord[][4],
                                      const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      if (level >= tObj->_MaxLevel) {
         sample_1d_array_nearest(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                                 texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_1d_array_nearest(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_1d_array_nearest(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


static void
sample_1d_array_linear_mipmap_linear(GLcontext *ctx,
                                     const struct gl_texture_object *tObj,
                                     GLuint n, const GLfloat texcoord[][4],
                                     const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level = linear_mipmap_level(tObj, lambda[i]);
      if (level >= tObj->_MaxLevel) {
         sample_1d_array_linear(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                          texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_1d_array_linear(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_1d_array_linear(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         lerp_rgba(rgba[i], f, t0, t1);
      }
   }
}


/** Sample 1D Array texture, nearest filtering for both min/magnification */
static void
sample_nearest_1d_array(GLcontext *ctx,
                        const struct gl_texture_object *tObj, GLuint n,
                        const GLfloat texcoords[][4], const GLfloat lambda[],
                        GLchan rgba[][4])
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) lambda;
   for (i = 0; i < n; i++) {
      sample_1d_array_nearest(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}


/** Sample 1D Array texture, linear filtering for both min/magnification */
static void
sample_linear_1d_array(GLcontext *ctx,
                       const struct gl_texture_object *tObj, GLuint n,
                       const GLfloat texcoords[][4],
                       const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) lambda;
   for (i = 0; i < n; i++) {
      sample_1d_array_linear(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}


/** Sample 1D Array texture, using lambda to choose between min/magnification */
static void
sample_lambda_1d_array(GLcontext *ctx,
                       const struct gl_texture_object *tObj, GLuint n,
                       const GLfloat texcoords[][4], const GLfloat lambda[],
                       GLchan rgba[][4])
{
   GLuint minStart, minEnd;  /* texels with minification */
   GLuint magStart, magEnd;  /* texels with magnification */
   GLuint i;

   ASSERT(lambda != NULL);
   compute_min_mag_ranges(tObj, n, lambda,
                          &minStart, &minEnd, &magStart, &magEnd);

   if (minStart < minEnd) {
      /* do the minified texels */
      GLuint m = minEnd - minStart;
      switch (tObj->MinFilter) {
      case GL_NEAREST:
         for (i = minStart; i < minEnd; i++)
            sample_1d_array_nearest(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                                    texcoords[i], rgba[i]);
         break;
      case GL_LINEAR:
         for (i = minStart; i < minEnd; i++)
            sample_1d_array_linear(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                                   texcoords[i], rgba[i]);
         break;
      case GL_NEAREST_MIPMAP_NEAREST:
         sample_1d_array_nearest_mipmap_nearest(ctx, tObj, m, texcoords + minStart,
                                                lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_NEAREST:
         sample_1d_array_linear_mipmap_nearest(ctx, tObj, m, 
                                               texcoords + minStart,
                                               lambda + minStart,
                                               rgba + minStart);
         break;
      case GL_NEAREST_MIPMAP_LINEAR:
         sample_1d_array_nearest_mipmap_linear(ctx, tObj, m, texcoords + minStart,
                                               lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_LINEAR:
         sample_1d_array_linear_mipmap_linear(ctx, tObj, m, 
                                              texcoords + minStart,
                                              lambda + minStart, 
                                              rgba + minStart);
         break;
      default:
         _mesa_problem(ctx, "Bad min filter in sample_1d_array_texture");
         return;
      }
   }

   if (magStart < magEnd) {
      /* do the magnified texels */
      switch (tObj->MagFilter) {
      case GL_NEAREST:
         for (i = magStart; i < magEnd; i++)
            sample_1d_array_nearest(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                              texcoords[i], rgba[i]);
         break;
      case GL_LINEAR:
         for (i = magStart; i < magEnd; i++)
            sample_1d_array_linear(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                                   texcoords[i], rgba[i]);
         break;
      default:
         _mesa_problem(ctx, "Bad mag filter in sample_1d_array_texture");
         return;
      }
   }
}


/**
 * Sample a shadow/depth texture.
 */
static void
sample_depth_texture( GLcontext *ctx,
                      const struct gl_texture_object *tObj, GLuint n,
                      const GLfloat texcoords[][4], const GLfloat lambda[],
                      GLchan texel[][4] )
{
   const GLint baseLevel = tObj->BaseLevel;
   const struct gl_texture_image *img = tObj->Image[0][baseLevel];
   const GLint width = img->Width;
   const GLint height = img->Height;
   const GLint depth = img->Depth;
   const GLuint compare_coord = (tObj->Target == GL_TEXTURE_2D_ARRAY_EXT)
       ? 3 : 2;
   GLchan ambient;
   GLenum function;
   GLchan result;

   (void) lambda;

   ASSERT(img->TexFormat->BaseFormat == GL_DEPTH_COMPONENT ||
          img->TexFormat->BaseFormat == GL_DEPTH_STENCIL_EXT);

   ASSERT(tObj->Target == GL_TEXTURE_1D ||
          tObj->Target == GL_TEXTURE_2D ||
          tObj->Target == GL_TEXTURE_RECTANGLE_NV ||
          tObj->Target == GL_TEXTURE_1D_ARRAY_EXT ||
          tObj->Target == GL_TEXTURE_2D_ARRAY_EXT);

   UNCLAMPED_FLOAT_TO_CHAN(ambient, tObj->ShadowAmbient);

   /* XXXX if tObj->MinFilter != tObj->MagFilter, we're ignoring lambda */

   function = tObj->_Function;
   if (tObj->MagFilter == GL_NEAREST) {
      GLuint i;
      for (i = 0; i < n; i++) {
         GLfloat depthSample;
         GLint col, row, slice;

         switch (tObj->Target) {
         case GL_TEXTURE_RECTANGLE_ARB:
            col = clamp_rect_coord_nearest(tObj->WrapS, texcoords[i][0], width);
            row = clamp_rect_coord_nearest(tObj->WrapT, texcoords[i][1], height);
            slice = 0;
            break;
            
         case GL_TEXTURE_1D:
            col = nearest_texel_location(tObj->WrapS, img, width,
                                         texcoords[i][0]);
            row = 0;
            slice = 0;
            break;

         case GL_TEXTURE_2D:
            col = nearest_texel_location(tObj->WrapS, img, width,
                                         texcoords[i][0]);
            row = nearest_texel_location(tObj->WrapT, img, height,
                                         texcoords[i][1]);
            slice = 0;
            break;

         case GL_TEXTURE_1D_ARRAY_EXT:
            col = nearest_texel_location(tObj->WrapS, img, width,
                                         texcoords[i][0]);
            row = clamp_rect_coord_nearest(tObj->WrapT, texcoords[i][1], height);
            slice = 0;
            break;

         case GL_TEXTURE_2D_ARRAY_EXT:
            col = nearest_texel_location(tObj->WrapS, img, width,
                                         texcoords[i][0]);
            row = nearest_texel_location(tObj->WrapT, img, height,
                                         texcoords[i][1]);
            slice = clamp_rect_coord_nearest(tObj->WrapR, texcoords[i][2], depth);
            break;
         default:
            col = row = slice = 0;
         }

         if (col >= 0 && row >= 0 && col < width && row < height && 
             slice >= 0 && slice < depth) {
            img->FetchTexelf(img, col, row, slice, &depthSample);
         }
         else {
            depthSample = tObj->BorderColor[0];
         }

         switch (function) {
         case GL_LEQUAL:
            result = (texcoords[i][compare_coord] <= depthSample) ? CHAN_MAX : ambient;
            break;
         case GL_GEQUAL:
            result = (texcoords[i][compare_coord] >= depthSample) ? CHAN_MAX : ambient;
            break;
         case GL_LESS:
            result = (texcoords[i][compare_coord] < depthSample) ? CHAN_MAX : ambient;
            break;
         case GL_GREATER:
            result = (texcoords[i][compare_coord] > depthSample) ? CHAN_MAX : ambient;
            break;
         case GL_EQUAL:
            result = (texcoords[i][compare_coord] == depthSample) ? CHAN_MAX : ambient;
            break;
         case GL_NOTEQUAL:
            result = (texcoords[i][compare_coord] != depthSample) ? CHAN_MAX : ambient;
            break;
         case GL_ALWAYS:
            result = CHAN_MAX;
            break;
         case GL_NEVER:
            result = ambient;
            break;
         case GL_NONE:
            CLAMPED_FLOAT_TO_CHAN(result, depthSample);
            break;
         default:
            _mesa_problem(ctx, "Bad compare func in sample_depth_texture");
            return;
         }

         switch (tObj->DepthMode) {
         case GL_LUMINANCE:
            texel[i][RCOMP] = result;
            texel[i][GCOMP] = result;
            texel[i][BCOMP] = result;
            texel[i][ACOMP] = CHAN_MAX;
            break;
         case GL_INTENSITY:
            texel[i][RCOMP] = result;
            texel[i][GCOMP] = result;
            texel[i][BCOMP] = result;
            texel[i][ACOMP] = result;
            break;
         case GL_ALPHA:
            texel[i][RCOMP] = 0;
            texel[i][GCOMP] = 0;
            texel[i][BCOMP] = 0;
            texel[i][ACOMP] = result;
            break;
         default:
            _mesa_problem(ctx, "Bad depth texture mode");
         }
      }
   }
   else {
      GLuint i;
      ASSERT(tObj->MagFilter == GL_LINEAR);
      for (i = 0; i < n; i++) {
         GLfloat depth00, depth01, depth10, depth11;
         GLint i0, i1, j0, j1;
         GLint slice;
         GLfloat a, b;
         GLuint useBorderTexel;

         switch (tObj->Target) {
         case GL_TEXTURE_RECTANGLE_ARB:
            clamp_rect_coord_linear(tObj->WrapS, texcoords[i][0],
                                    width, &i0, &i1, &a);
            clamp_rect_coord_linear(tObj->WrapT, texcoords[i][1],
                                    height, &j0, &j1, &b);
            slice = 0;
            break;

         case GL_TEXTURE_1D:
         case GL_TEXTURE_2D:
            linear_texel_locations(tObj->WrapS, img, width,
                                   texcoords[i][0], &i0, &i1, &a);
            linear_texel_locations(tObj->WrapT, img, height,
                                   texcoords[i][1], &j0, &j1, &b);
            slice = 0;
            break;

         case GL_TEXTURE_1D_ARRAY_EXT:
            linear_texel_locations(tObj->WrapS, img, width,
                                   texcoords[i][0], &i0, &i1, &a);
            j0 = clamp_rect_coord_nearest(tObj->WrapT, texcoords[i][1], height);
            j1 = j0;
            slice = 0;
            break;

         case GL_TEXTURE_2D_ARRAY_EXT:
            linear_texel_locations(tObj->WrapS, img, width,
                                   texcoords[i][0], &i0, &i1, &a);
            linear_texel_locations(tObj->WrapT, img, height,
                                   texcoords[i][1], &j0, &j1, &b);
            slice = clamp_rect_coord_nearest(tObj->WrapR, texcoords[i][2], depth);
            break;
         default:
            slice = 0;
         }

         useBorderTexel = 0;
         if (img->Border) {
            i0 += img->Border;
            i1 += img->Border;
            if (tObj->Target != GL_TEXTURE_1D_ARRAY_EXT) {
               j0 += img->Border;
               j1 += img->Border;
            }
         }
         else {
            if (i0 < 0 || i0 >= (GLint) width)   useBorderTexel |= I0BIT;
            if (i1 < 0 || i1 >= (GLint) width)   useBorderTexel |= I1BIT;
            if (j0 < 0 || j0 >= (GLint) height)  useBorderTexel |= J0BIT;
            if (j1 < 0 || j1 >= (GLint) height)  useBorderTexel |= J1BIT;
         }

         if (slice < 0 || slice >= (GLint) depth) {
            depth00 = tObj->BorderColor[0];
            depth01 = tObj->BorderColor[0];
            depth10 = tObj->BorderColor[0];
            depth11 = tObj->BorderColor[0];
         }
         else {
            /* get four depth samples from the texture */
            if (useBorderTexel & (I0BIT | J0BIT)) {
               depth00 = tObj->BorderColor[0];
            }
            else {
               img->FetchTexelf(img, i0, j0, slice, &depth00);
            }
            if (useBorderTexel & (I1BIT | J0BIT)) {
               depth10 = tObj->BorderColor[0];
            }
            else {
               img->FetchTexelf(img, i1, j0, slice, &depth10);
            }

            if (tObj->Target != GL_TEXTURE_1D_ARRAY_EXT) {
               if (useBorderTexel & (I0BIT | J1BIT)) {
                  depth01 = tObj->BorderColor[0];
               }
               else {
                  img->FetchTexelf(img, i0, j1, slice, &depth01);
               }
               if (useBorderTexel & (I1BIT | J1BIT)) {
                  depth11 = tObj->BorderColor[0];
               }
               else {
                  img->FetchTexelf(img, i1, j1, slice, &depth11);
               }
            }
            else {
               depth01 = depth00;
               depth11 = depth10;
            }
         }

         if (0) {
            /* compute a single weighted depth sample and do one comparison */
            const GLfloat depthSample
               = lerp_2d(a, b, depth00, depth10, depth01, depth11);
            if ((depthSample <= texcoords[i][compare_coord] && function == GL_LEQUAL) ||
                (depthSample >= texcoords[i][compare_coord] && function == GL_GEQUAL)) {
               result  = ambient;
            }
            else {
               result = CHAN_MAX;
            }
         }
         else {
            /* Do four depth/R comparisons and compute a weighted result.
             * If this touches on somebody's I.P., I'll remove this code
             * upon request.
             */
            const GLfloat d = (CHAN_MAXF - (GLfloat) ambient) * 0.25F;
            GLfloat luminance = CHAN_MAXF;

            switch (function) {
            case GL_LEQUAL:
               if (depth00 <= texcoords[i][compare_coord])  luminance -= d;
               if (depth01 <= texcoords[i][compare_coord])  luminance -= d;
               if (depth10 <= texcoords[i][compare_coord])  luminance -= d;
               if (depth11 <= texcoords[i][compare_coord])  luminance -= d;
               result = (GLchan) luminance;
               break;
            case GL_GEQUAL:
               if (depth00 >= texcoords[i][compare_coord])  luminance -= d;
               if (depth01 >= texcoords[i][compare_coord])  luminance -= d;
               if (depth10 >= texcoords[i][compare_coord])  luminance -= d;
               if (depth11 >= texcoords[i][compare_coord])  luminance -= d;
               result = (GLchan) luminance;
               break;
            case GL_LESS:
               if (depth00 < texcoords[i][compare_coord])  luminance -= d;
               if (depth01 < texcoords[i][compare_coord])  luminance -= d;
               if (depth10 < texcoords[i][compare_coord])  luminance -= d;
               if (depth11 < texcoords[i][compare_coord])  luminance -= d;
               result = (GLchan) luminance;
               break;
            case GL_GREATER:
               if (depth00 > texcoords[i][compare_coord])  luminance -= d;
               if (depth01 > texcoords[i][compare_coord])  luminance -= d;
               if (depth10 > texcoords[i][compare_coord])  luminance -= d;
               if (depth11 > texcoords[i][compare_coord])  luminance -= d;
               result = (GLchan) luminance;
               break;
            case GL_EQUAL:
               if (depth00 == texcoords[i][compare_coord])  luminance -= d;
               if (depth01 == texcoords[i][compare_coord])  luminance -= d;
               if (depth10 == texcoords[i][compare_coord])  luminance -= d;
               if (depth11 == texcoords[i][compare_coord])  luminance -= d;
               result = (GLchan) luminance;
               break;
            case GL_NOTEQUAL:
               if (depth00 != texcoords[i][compare_coord])  luminance -= d;
               if (depth01 != texcoords[i][compare_coord])  luminance -= d;
               if (depth10 != texcoords[i][compare_coord])  luminance -= d;
               if (depth11 != texcoords[i][compare_coord])  luminance -= d;
               result = (GLchan) luminance;
               break;
            case GL_ALWAYS:
               result = 0;
               break;
            case GL_NEVER:
               result = CHAN_MAX;
               break;
            case GL_NONE:
               /* ordinary bilinear filtering */
               {
                  const GLfloat depthSample
                     = lerp_2d(a, b, depth00, depth10, depth01, depth11);
                  CLAMPED_FLOAT_TO_CHAN(result, depthSample);
               }
               break;
            default:
               _mesa_problem(ctx, "Bad compare func in sample_depth_texture");
               return;
            }
         }

         switch (tObj->DepthMode) {
         case GL_LUMINANCE:
            texel[i][RCOMP] = result;
            texel[i][GCOMP] = result;
            texel[i][BCOMP] = result;
            texel[i][ACOMP] = CHAN_MAX;
            break;
         case GL_INTENSITY:
            texel[i][RCOMP] = result;
            texel[i][GCOMP] = result;
            texel[i][BCOMP] = result;
            texel[i][ACOMP] = result;
            break;
         case GL_ALPHA:
            texel[i][RCOMP] = 0;
            texel[i][GCOMP] = 0;
            texel[i][BCOMP] = 0;
            texel[i][ACOMP] = result;
            break;
         default:
            _mesa_problem(ctx, "Bad depth texture mode");
         }
      }  /* for */
   }  /* if filter */
}


#if 0
/*
 * Experimental depth texture sampling function.
 */
static void
sample_depth_texture2(const GLcontext *ctx,
                     const struct gl_texture_unit *texUnit,
                     GLuint n, const GLfloat texcoords[][4],
                     GLchan texel[][4])
{
   const struct gl_texture_object *texObj = texUnit->_Current;
   const GLint baseLevel = texObj->BaseLevel;
   const struct gl_texture_image *texImage = texObj->Image[0][baseLevel];
   const GLuint width = texImage->Width;
   const GLuint height = texImage->Height;
   GLchan ambient;
   GLboolean lequal, gequal;

   if (texObj->Target != GL_TEXTURE_2D) {
      _mesa_problem(ctx, "only 2-D depth textures supported at this time");
      return;
   }

   if (texObj->MinFilter != texObj->MagFilter) {
      _mesa_problem(ctx, "mipmapped depth textures not supported at this time");
      return;
   }

   /* XXX the GL_SGIX_shadow extension spec doesn't say what to do if
    * GL_TEXTURE_COMPARE_SGIX == GL_TRUE but the current texture object
    * isn't a depth texture.
    */
   if (texImage->TexFormat->BaseFormat != GL_DEPTH_COMPONENT) {
      _mesa_problem(ctx,"GL_TEXTURE_COMPARE_SGIX enabled with non-depth texture");
      return;
   }

   UNCLAMPED_FLOAT_TO_CHAN(ambient, tObj->ShadowAmbient);

   if (texObj->CompareOperator == GL_TEXTURE_LEQUAL_R_SGIX) {
      lequal = GL_TRUE;
      gequal = GL_FALSE;
   }
   else {
      lequal = GL_FALSE;
      gequal = GL_TRUE;
   }

   {
      GLuint i;
      for (i = 0; i < n; i++) {
         const GLint K = 3;
         GLint col, row, ii, jj, imin, imax, jmin, jmax, samples, count;
         GLfloat w;
         GLchan lum;
         col = nearest_texel_location(texObj->WrapS, img, width,
                                      texcoords[i][0]);
         row = nearest_texel_location(texObj->WrapT, img, height,
                                      texcoords[i][1]);

         imin = col - K;
         imax = col + K;
         jmin = row - K;
         jmax = row + K;

         if (imin < 0)  imin = 0;
         if (imax >= width)  imax = width - 1;
         if (jmin < 0)  jmin = 0;
         if (jmax >= height) jmax = height - 1;

         samples = (imax - imin + 1) * (jmax - jmin + 1);
         count = 0;
         for (jj = jmin; jj <= jmax; jj++) {
            for (ii = imin; ii <= imax; ii++) {
               GLfloat depthSample;
               texImage->FetchTexelf(texImage, ii, jj, 0, &depthSample);
               if ((depthSample <= r[i] && lequal) ||
                   (depthSample >= r[i] && gequal)) {
                  count++;
               }
            }
         }

         w = (GLfloat) count / (GLfloat) samples;
         w = CHAN_MAXF - w * (CHAN_MAXF - (GLfloat) ambient);
         lum = (GLint) w;

         texel[i][RCOMP] = lum;
         texel[i][GCOMP] = lum;
         texel[i][BCOMP] = lum;
         texel[i][ACOMP] = CHAN_MAX;
      }
   }
}
#endif


/**
 * We use this function when a texture object is in an "incomplete" state.
 * When a fragment program attempts to sample an incomplete texture we
 * return black (see issue 23 in GL_ARB_fragment_program spec).
 * Note: fragment programs don't observe the texture enable/disable flags.
 */
static void
null_sample_func( GLcontext *ctx,
		  const struct gl_texture_object *tObj, GLuint n,
		  const GLfloat texcoords[][4], const GLfloat lambda[],
		  GLchan rgba[][4])
{
   GLuint i;
   (void) ctx;
   (void) tObj;
   (void) texcoords;
   (void) lambda;
   for (i = 0; i < n; i++) {
      rgba[i][RCOMP] = 0;
      rgba[i][GCOMP] = 0;
      rgba[i][BCOMP] = 0;
      rgba[i][ACOMP] = CHAN_MAX;
   }
}


/**
 * Choose the texture sampling function for the given texture object.
 */
texture_sample_func
_swrast_choose_texture_sample_func( GLcontext *ctx,
				    const struct gl_texture_object *t )
{
   if (!t || !t->_Complete) {
      return &null_sample_func;
   }
   else {
      const GLboolean needLambda = (GLboolean) (t->MinFilter != t->MagFilter);
      const GLenum format = t->Image[0][t->BaseLevel]->TexFormat->BaseFormat;

      switch (t->Target) {
      case GL_TEXTURE_1D:
         if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL_EXT) {
            return &sample_depth_texture;
         }
         else if (needLambda) {
            return &sample_lambda_1d;
         }
         else if (t->MinFilter == GL_LINEAR) {
            return &sample_linear_1d;
         }
         else {
            ASSERT(t->MinFilter == GL_NEAREST);
            return &sample_nearest_1d;
         }
      case GL_TEXTURE_2D:
         if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL_EXT) {
            return &sample_depth_texture;
         }
         else if (needLambda) {
            return &sample_lambda_2d;
         }
         else if (t->MinFilter == GL_LINEAR) {
            return &sample_linear_2d;
         }
         else {
            /* check for a few optimized cases */
            const struct gl_texture_image *img = t->Image[0][t->BaseLevel];
            ASSERT(t->MinFilter == GL_NEAREST);
            if (t->WrapS == GL_REPEAT &&
                t->WrapT == GL_REPEAT &&
                img->_IsPowerOfTwo &&
                img->Border == 0 &&
                img->TexFormat->MesaFormat == MESA_FORMAT_RGB) {
               return &opt_sample_rgb_2d;
            }
            else if (t->WrapS == GL_REPEAT &&
                     t->WrapT == GL_REPEAT &&
                     img->_IsPowerOfTwo &&
                     img->Border == 0 &&
                     img->TexFormat->MesaFormat == MESA_FORMAT_RGBA) {
               return &opt_sample_rgba_2d;
            }
            else {
               return &sample_nearest_2d;
            }
         }
      case GL_TEXTURE_3D:
         if (needLambda) {
            return &sample_lambda_3d;
         }
         else if (t->MinFilter == GL_LINEAR) {
            return &sample_linear_3d;
         }
         else {
            ASSERT(t->MinFilter == GL_NEAREST);
            return &sample_nearest_3d;
         }
      case GL_TEXTURE_CUBE_MAP:
         if (needLambda) {
            return &sample_lambda_cube;
         }
         else if (t->MinFilter == GL_LINEAR) {
            return &sample_linear_cube;
         }
         else {
            ASSERT(t->MinFilter == GL_NEAREST);
            return &sample_nearest_cube;
         }
      case GL_TEXTURE_RECTANGLE_NV:
         if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL_EXT) {
            return &sample_depth_texture;
         }
         else if (needLambda) {
            return &sample_lambda_rect;
         }
         else if (t->MinFilter == GL_LINEAR) {
            return &sample_linear_rect;
         }
         else {
            ASSERT(t->MinFilter == GL_NEAREST);
            return &sample_nearest_rect;
         }
      case GL_TEXTURE_1D_ARRAY_EXT:
         if (needLambda) {
            return &sample_lambda_1d_array;
         }
         else if (t->MinFilter == GL_LINEAR) {
            return &sample_linear_1d_array;
         }
         else {
            ASSERT(t->MinFilter == GL_NEAREST);
            return &sample_nearest_1d_array;
         }
      case GL_TEXTURE_2D_ARRAY_EXT:
         if (needLambda) {
            return &sample_lambda_2d_array;
         }
         else if (t->MinFilter == GL_LINEAR) {
            return &sample_linear_2d_array;
         }
         else {
            ASSERT(t->MinFilter == GL_NEAREST);
            return &sample_nearest_2d_array;
         }
      default:
         _mesa_problem(ctx,
                       "invalid target in _swrast_choose_texture_sample_func");
         return &null_sample_func;
      }
   }
}
