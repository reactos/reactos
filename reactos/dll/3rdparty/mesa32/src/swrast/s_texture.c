/*
 * Mesa 3-D graphics library
 * Version:  6.4
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "macros.h"
#include "imports.h"
#include "pixel.h"
#include "texformat.h"
#include "teximage.h"

#include "s_context.h"
#include "s_texture.h"


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
 * Compute the remainder of a divided by b, but be careful with
 * negative values so that GL_REPEAT mode works right.
 */
static INLINE GLint
repeat_remainder(GLint a, GLint b)
{
   if (a >= 0)
      return a % b;
   else
      return (a + 1) % b + b - 1;
}


/**
 * Used to compute texel locations for linear sampling.
 * Input:
 *    wrapMode = GL_REPEAT, GL_CLAMP, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
 *    S = texcoord in [0,1]
 *    SIZE = width (or height or depth) of texture
 * Output:
 *    U = texcoord in [0, width]
 *    I0, I1 = two nearest texel indexes
 */
#define COMPUTE_LINEAR_TEXEL_LOCATIONS(wrapMode, S, U, SIZE, I0, I1)	\
{									\
   if (wrapMode == GL_REPEAT) {						\
      U = S * SIZE - 0.5F;						\
      if (tObj->_IsPowerOfTwo) {					\
         I0 = IFLOOR(U) & (SIZE - 1);					\
         I1 = (I0 + 1) & (SIZE - 1);					\
      }									\
      else {								\
         I0 = repeat_remainder(IFLOOR(U), SIZE);			\
         I1 = repeat_remainder(I0 + 1, SIZE);				\
      }									\
   }									\
   else if (wrapMode == GL_CLAMP_TO_EDGE) {				\
      if (S <= 0.0F)							\
         U = 0.0F;							\
      else if (S >= 1.0F)						\
         U = (GLfloat) SIZE;						\
      else								\
         U = S * SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
      if (I0 < 0)							\
         I0 = 0;							\
      if (I1 >= (GLint) SIZE)						\
         I1 = SIZE - 1;							\
   }									\
   else if (wrapMode == GL_CLAMP_TO_BORDER) {				\
      const GLfloat min = -1.0F / (2.0F * SIZE);			\
      const GLfloat max = 1.0F - min;					\
      if (S <= min)							\
         U = min * SIZE;						\
      else if (S >= max)						\
         U = max * SIZE;						\
      else								\
         U = S * SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
   }									\
   else if (wrapMode == GL_MIRRORED_REPEAT) {				\
      const GLint flr = IFLOOR(S);					\
      if (flr & 1)							\
         U = 1.0F - (S - (GLfloat) flr);	/* flr is odd */	\
      else								\
         U = S - (GLfloat) flr;		/* flr is even */		\
      U = (U * SIZE) - 0.5F;						\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
      if (I0 < 0)							\
         I0 = 0;							\
      if (I1 >= (GLint) SIZE)						\
         I1 = SIZE - 1;							\
   }									\
   else if (wrapMode == GL_MIRROR_CLAMP_EXT) {				\
      U = (GLfloat) fabs(S);						\
      if (U >= 1.0F)							\
         U = (GLfloat) SIZE;						\
      else								\
         U *= SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
   }									\
   else if (wrapMode == GL_MIRROR_CLAMP_TO_EDGE_EXT) {			\
      U = (GLfloat) fabs(S);						\
      if (U >= 1.0F)							\
         U = (GLfloat) SIZE;						\
      else								\
         U *= SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
      if (I0 < 0)							\
         I0 = 0;							\
      if (I1 >= (GLint) SIZE)						\
         I1 = SIZE - 1;							\
   }									\
   else if (wrapMode == GL_MIRROR_CLAMP_TO_BORDER_EXT) {		\
      const GLfloat min = -1.0F / (2.0F * SIZE);			\
      const GLfloat max = 1.0F - min;					\
      U = (GLfloat) fabs(S);						\
      if (U <= min)							\
         U = min * SIZE;						\
      else if (U >= max)						\
         U = max * SIZE;						\
      else								\
         U *= SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
   }									\
   else {								\
      ASSERT(wrapMode == GL_CLAMP);					\
      if (S <= 0.0F)							\
         U = 0.0F;							\
      else if (S >= 1.0F)						\
         U = (GLfloat) SIZE;						\
      else								\
         U = S * SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
   }									\
}


/**
 * Used to compute texel location for nearest sampling.
 */
#define COMPUTE_NEAREST_TEXEL_LOCATION(wrapMode, S, SIZE, I)		\
{									\
   if (wrapMode == GL_REPEAT) {						\
      /* s limited to [0,1) */						\
      /* i limited to [0,size-1] */					\
      I = IFLOOR(S * SIZE);						\
      if (tObj->_IsPowerOfTwo)						\
         I &= (SIZE - 1);						\
      else								\
         I = repeat_remainder(I, SIZE);					\
   }									\
   else if (wrapMode == GL_CLAMP_TO_EDGE) {				\
      /* s limited to [min,max] */					\
      /* i limited to [0, size-1] */					\
      const GLfloat min = 1.0F / (2.0F * SIZE);				\
      const GLfloat max = 1.0F - min;					\
      if (S < min)							\
         I = 0;								\
      else if (S > max)							\
         I = SIZE - 1;							\
      else								\
         I = IFLOOR(S * SIZE);						\
   }									\
   else if (wrapMode == GL_CLAMP_TO_BORDER) {				\
      /* s limited to [min,max] */					\
      /* i limited to [-1, size] */					\
      const GLfloat min = -1.0F / (2.0F * SIZE);			\
      const GLfloat max = 1.0F - min;					\
      if (S <= min)							\
         I = -1;							\
      else if (S >= max)						\
         I = SIZE;							\
      else								\
         I = IFLOOR(S * SIZE);						\
   }									\
   else if (wrapMode == GL_MIRRORED_REPEAT) {				\
      const GLfloat min = 1.0F / (2.0F * SIZE);				\
      const GLfloat max = 1.0F - min;					\
      const GLint flr = IFLOOR(S);					\
      GLfloat u;							\
      if (flr & 1)							\
         u = 1.0F - (S - (GLfloat) flr);	/* flr is odd */	\
      else								\
         u = S - (GLfloat) flr;		/* flr is even */		\
      if (u < min)							\
         I = 0;								\
      else if (u > max)							\
         I = SIZE - 1;							\
      else								\
         I = IFLOOR(u * SIZE);						\
   }									\
   else if (wrapMode == GL_MIRROR_CLAMP_EXT) {				\
      /* s limited to [0,1] */						\
      /* i limited to [0,size-1] */					\
      const GLfloat u = (GLfloat) fabs(S);				\
      if (u <= 0.0F)							\
         I = 0;								\
      else if (u >= 1.0F)						\
         I = SIZE - 1;							\
      else								\
         I = IFLOOR(u * SIZE);						\
   }									\
   else if (wrapMode == GL_MIRROR_CLAMP_TO_EDGE_EXT) {			\
      /* s limited to [min,max] */					\
      /* i limited to [0, size-1] */					\
      const GLfloat min = 1.0F / (2.0F * SIZE);				\
      const GLfloat max = 1.0F - min;					\
      const GLfloat u = (GLfloat) fabs(S);				\
      if (u < min)							\
         I = 0;								\
      else if (u > max)							\
         I = SIZE - 1;							\
      else								\
         I = IFLOOR(u * SIZE);						\
   }									\
   else if (wrapMode == GL_MIRROR_CLAMP_TO_BORDER_EXT) {		\
      /* s limited to [min,max] */					\
      /* i limited to [0, size-1] */					\
      const GLfloat min = -1.0F / (2.0F * SIZE);			\
      const GLfloat max = 1.0F - min;					\
      const GLfloat u = (GLfloat) fabs(S);				\
      if (u < min)							\
         I = -1;							\
      else if (u > max)							\
         I = SIZE;							\
      else								\
         I = IFLOOR(u * SIZE);						\
   }									\
   else {								\
      ASSERT(wrapMode == GL_CLAMP);					\
      /* s limited to [0,1] */						\
      /* i limited to [0,size-1] */					\
      if (S <= 0.0F)							\
         I = 0;								\
      else if (S >= 1.0F)						\
         I = SIZE - 1;							\
      else								\
         I = IFLOOR(S * SIZE);						\
   }									\
}


/* Power of two image sizes only */
#define COMPUTE_LINEAR_REPEAT_TEXEL_LOCATION(S, U, SIZE, I0, I1)	\
{									\
   U = S * SIZE - 0.5F;							\
   I0 = IFLOOR(U) & (SIZE - 1);						\
   I1 = (I0 + 1) & (SIZE - 1);						\
}


/*
 * Compute linear mipmap levels for given lambda.
 */
#define COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda, level)	\
{								\
   if (lambda < 0.0F)						\
      level = tObj->BaseLevel;					\
   else if (lambda > tObj->_MaxLambda)				\
      level = (GLint) (tObj->BaseLevel + tObj->_MaxLambda);	\
   else								\
      level = (GLint) (tObj->BaseLevel + lambda);		\
}


/*
 * Compute nearest mipmap level for given lambda.
 */
#define COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda, level)	\
{								\
   GLfloat l;							\
   if (lambda <= 0.5F)						\
      l = 0.0F;							\
   else if (lambda > tObj->_MaxLambda + 0.4999F)		\
      l = tObj->_MaxLambda + 0.4999F;				\
   else								\
      l = lambda;						\
   level = (GLint) (tObj->BaseLevel + l + 0.5F);		\
   if (level > tObj->_MaxLevel)					\
      level = tObj->_MaxLevel;					\
}



/*
 * Note, the FRAC macro has to work perfectly.  Otherwise you'll sometimes
 * see 1-pixel bands of improperly weighted linear-sampled texels.  The
 * tests/texwrap.c demo is a good test.
 * Also note, FRAC(x) doesn't truly return the fractional part of x for x < 0.
 * Instead, if x < 0 then FRAC(x) = 1 - true_frac(x).
 */
#define FRAC(f)  ((f) - IFLOOR(f))



/*
 * Bitflags for texture border color sampling.
 */
#define I0BIT   1
#define I1BIT   2
#define J0BIT   4
#define J1BIT   8
#define K0BIT  16
#define K1BIT  32



/*
 * The lambda[] array values are always monotonic.  Either the whole span
 * will be minified, magnified, or split between the two.  This function
 * determines the subranges in [0, n-1] that are to be minified or magnified.
 */
static INLINE void
compute_min_mag_ranges( GLfloat minMagThresh, GLuint n, const GLfloat lambda[],
                        GLuint *minStart, GLuint *minEnd,
                        GLuint *magStart, GLuint *magEnd )
{
   ASSERT(lambda != NULL);
#if 0
   /* Verify that lambda[] is monotonous.
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

   /* since lambda is monotonous-array use this check first */
   if (lambda[0] <= minMagThresh && lambda[n-1] <= minMagThresh) {
      /* magnification for whole span */
      *magStart = 0;
      *magEnd = n;
      *minStart = *minEnd = 0;
   }
   else if (lambda[0] > minMagThresh && lambda[n-1] > minMagThresh) {
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

/*
 * Return the texture sample for coordinate (s) using GL_NEAREST filter.
 */
static void
sample_1d_nearest(GLcontext *ctx,
                  const struct gl_texture_object *tObj,
                  const struct gl_texture_image *img,
                  const GLfloat texcoord[4], GLchan rgba[4])
{
   const GLint width = img->Width2;  /* without border, power of two */
   GLint i;
   (void) ctx;

   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapS, texcoord[0], width, i);

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



/*
 * Return the texture sample for coordinate (s) using GL_LINEAR filter.
 */
static void
sample_1d_linear(GLcontext *ctx,
                 const struct gl_texture_object *tObj,
                 const struct gl_texture_image *img,
                 const GLfloat texcoord[4], GLchan rgba[4])
{
   const GLint width = img->Width2;
   GLint i0, i1;
   GLfloat u;
   GLuint useBorderColor;
   (void) ctx;

   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapS, texcoord[0], u, width, i0, i1);

   useBorderColor = 0;
   if (img->Border) {
      i0 += img->Border;
      i1 += img->Border;
   }
   else {
      if (i0 < 0 || i0 >= width)   useBorderColor |= I0BIT;
      if (i1 < 0 || i1 >= width)   useBorderColor |= I1BIT;
   }

   {
      const GLfloat a = FRAC(u);
      GLchan t0[4], t1[4];  /* texels */

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

      /* do linear interpolation of texel colors */
#if CHAN_TYPE == GL_FLOAT
      rgba[0] = LERP(a, t0[0], t1[0]);
      rgba[1] = LERP(a, t0[1], t1[1]);
      rgba[2] = LERP(a, t0[2], t1[2]);
      rgba[3] = LERP(a, t0[3], t1[3]);
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
      rgba[0] = (GLchan) (LERP(a, t0[0], t1[0]) + 0.5);
      rgba[1] = (GLchan) (LERP(a, t0[1], t1[1]) + 0.5);
      rgba[2] = (GLchan) (LERP(a, t0[2], t1[2]) + 0.5);
      rgba[3] = (GLchan) (LERP(a, t0[3], t1[3]) + 0.5);
#else
      ASSERT(CHAN_TYPE == GL_UNSIGNED_BYTE);
      {
         /* fixed point interpolants in [0, ILERP_SCALE] */
         const GLint ia = IROUND_POS(a * ILERP_SCALE);
         rgba[0] = ILERP(ia, t0[0], t1[0]);
         rgba[1] = ILERP(ia, t0[1], t1[1]);
         rgba[2] = ILERP(ia, t0[2], t1[2]);
         rgba[3] = ILERP(ia, t0[3], t1[3]);
      }
#endif
   }
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
      GLint level;
      COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda[i], level);
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
      GLint level;
      COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda[i], level);
      sample_1d_linear(ctx, tObj, tObj->Image[0][level], texcoord[i], rgba[i]);
   }
}



/*
 * This is really just needed in order to prevent warnings with some compilers.
 */
#if CHAN_TYPE == GL_FLOAT
#define CHAN_CAST
#else
#define CHAN_CAST (GLchan) (GLint)
#endif


static void
sample_1d_nearest_mipmap_linear(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLuint n, const GLfloat texcoord[][4],
                                const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level;
      COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda[i], level);
      if (level >= tObj->_MaxLevel) {
         sample_1d_nearest(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                           texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];
         const GLfloat f = FRAC(lambda[i]);
         sample_1d_nearest(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_1d_nearest(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         rgba[i][RCOMP] = CHAN_CAST ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
         rgba[i][GCOMP] = CHAN_CAST ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
         rgba[i][BCOMP] = CHAN_CAST ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
         rgba[i][ACOMP] = CHAN_CAST ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
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
      GLint level;
      COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda[i], level);
      if (level >= tObj->_MaxLevel) {
         sample_1d_linear(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                          texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];
         const GLfloat f = FRAC(lambda[i]);
         sample_1d_linear(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_1d_linear(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         rgba[i][RCOMP] = CHAN_CAST ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
         rgba[i][GCOMP] = CHAN_CAST ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
         rgba[i][BCOMP] = CHAN_CAST ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
         rgba[i][ACOMP] = CHAN_CAST ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
      }
   }
}



static void
sample_nearest_1d( GLcontext *ctx, GLuint texUnit,
                   const struct gl_texture_object *tObj, GLuint n,
                   const GLfloat texcoords[][4], const GLfloat lambda[],
                   GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) texUnit;
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_1d_nearest(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}



static void
sample_linear_1d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat texcoords[][4], const GLfloat lambda[],
                  GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) texUnit;
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_1d_linear(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}


/*
 * Given an (s) texture coordinate and lambda (level of detail) value,
 * return a texture sample.
 *
 */
static void
sample_lambda_1d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat texcoords[][4],
                  const GLfloat lambda[], GLchan rgba[][4] )
{
   GLuint minStart, minEnd;  /* texels with minification */
   GLuint magStart, magEnd;  /* texels with magnification */
   GLuint i;

   ASSERT(lambda != NULL);
   compute_min_mag_ranges(SWRAST_CONTEXT(ctx)->_MinMagThresh[texUnit],
                          n, lambda, &minStart, &minEnd, &magStart, &magEnd);

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


/*
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

   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapS, texcoord[0], width,  i);
   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapT, texcoord[1], height, j);

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
   GLuint useBorderColor;
   GLfloat u, v;
   (void) ctx;

   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapS, texcoord[0], u, width,  i0, i1);
   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapT, texcoord[1], v, height, j0, j1);

   useBorderColor = 0;
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

   {
      const GLfloat a = FRAC(u);
      const GLfloat b = FRAC(v);
#if CHAN_TYPE == GL_UNSIGNED_BYTE
      const GLint ia = IROUND_POS(a * ILERP_SCALE);
      const GLint ib = IROUND_POS(b * ILERP_SCALE);
#endif
      GLchan t00[4], t10[4], t01[4], t11[4]; /* sampled texel colors */

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

      /* do bilinear interpolation of texel colors */
#if CHAN_TYPE == GL_FLOAT
      rgba[0] = lerp_2d(a, b, t00[0], t10[0], t01[0], t11[0]);
      rgba[1] = lerp_2d(a, b, t00[1], t10[1], t01[1], t11[1]);
      rgba[2] = lerp_2d(a, b, t00[2], t10[2], t01[2], t11[2]);
      rgba[3] = lerp_2d(a, b, t00[3], t10[3], t01[3], t11[3]);
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
      rgba[0] = (GLchan) (lerp_2d(a, b, t00[0], t10[0], t01[0], t11[0]) + 0.5);
      rgba[1] = (GLchan) (lerp_2d(a, b, t00[1], t10[1], t01[1], t11[1]) + 0.5);
      rgba[2] = (GLchan) (lerp_2d(a, b, t00[2], t10[2], t01[2], t11[2]) + 0.5);
      rgba[3] = (GLchan) (lerp_2d(a, b, t00[3], t10[3], t01[3], t11[3]) + 0.5);
#else
      ASSERT(CHAN_TYPE == GL_UNSIGNED_BYTE);
      rgba[0] = ilerp_2d(ia, ib, t00[0], t10[0], t01[0], t11[0]);
      rgba[1] = ilerp_2d(ia, ib, t00[1], t10[1], t01[1], t11[1]);
      rgba[2] = ilerp_2d(ia, ib, t00[2], t10[2], t01[2], t11[2]);
      rgba[3] = ilerp_2d(ia, ib, t00[3], t10[3], t01[3], t11[3]);
#endif
   }
}


/*
 * As above, but we know WRAP_S == REPEAT and WRAP_T == REPEAT.
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
   GLfloat u, v;
   (void) ctx;
   (void) tObj;

   ASSERT(tObj->WrapS == GL_REPEAT);
   ASSERT(tObj->WrapT == GL_REPEAT);
   ASSERT(img->Border == 0);
   ASSERT(img->Format != GL_COLOR_INDEX);
   ASSERT(img->_IsPowerOfTwo);

   COMPUTE_LINEAR_REPEAT_TEXEL_LOCATION(texcoord[0], u, width,  i0, i1);
   COMPUTE_LINEAR_REPEAT_TEXEL_LOCATION(texcoord[1], v, height, j0, j1);

   {
      const GLfloat a = FRAC(u);
      const GLfloat b = FRAC(v);
#if CHAN_TYPE == GL_UNSIGNED_BYTE
      const GLint ia = IROUND_POS(a * ILERP_SCALE);
      const GLint ib = IROUND_POS(b * ILERP_SCALE);
#endif
      GLchan t00[4], t10[4], t01[4], t11[4]; /* sampled texel colors */

      img->FetchTexelc(img, i0, j0, 0, t00);
      img->FetchTexelc(img, i1, j0, 0, t10);
      img->FetchTexelc(img, i0, j1, 0, t01);
      img->FetchTexelc(img, i1, j1, 0, t11);

      /* do bilinear interpolation of texel colors */
#if CHAN_TYPE == GL_FLOAT
      rgba[0] = lerp_2d(a, b, t00[0], t10[0], t01[0], t11[0]);
      rgba[1] = lerp_2d(a, b, t00[1], t10[1], t01[1], t11[1]);
      rgba[2] = lerp_2d(a, b, t00[2], t10[2], t01[2], t11[2]);
      rgba[3] = lerp_2d(a, b, t00[3], t10[3], t01[3], t11[3]);
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
      rgba[0] = (GLchan) (lerp_2d(a, b, t00[0], t10[0], t01[0], t11[0]) + 0.5);
      rgba[1] = (GLchan) (lerp_2d(a, b, t00[1], t10[1], t01[1], t11[1]) + 0.5);
      rgba[2] = (GLchan) (lerp_2d(a, b, t00[2], t10[2], t01[2], t11[2]) + 0.5);
      rgba[3] = (GLchan) (lerp_2d(a, b, t00[3], t10[3], t01[3], t11[3]) + 0.5);
#else
      ASSERT(CHAN_TYPE == GL_UNSIGNED_BYTE);
      rgba[0] = ilerp_2d(ia, ib, t00[0], t10[0], t01[0], t11[0]);
      rgba[1] = ilerp_2d(ia, ib, t00[1], t10[1], t01[1], t11[1]);
      rgba[2] = ilerp_2d(ia, ib, t00[2], t10[2], t01[2], t11[2]);
      rgba[3] = ilerp_2d(ia, ib, t00[3], t10[3], t01[3], t11[3]);
#endif
   }
}



static void
sample_2d_nearest_mipmap_nearest(GLcontext *ctx,
                                 const struct gl_texture_object *tObj,
                                 GLuint n, const GLfloat texcoord[][4],
                                 const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   for (i = 0; i < n; i++) {
      GLint level;
      COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda[i], level);
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
      GLint level;
      COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda[i], level);
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
      GLint level;
      COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda[i], level);
      if (level >= tObj->_MaxLevel) {
         sample_2d_nearest(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                           texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_2d_nearest(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_2d_nearest(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         rgba[i][RCOMP] = CHAN_CAST ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
         rgba[i][GCOMP] = CHAN_CAST ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
         rgba[i][BCOMP] = CHAN_CAST ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
         rgba[i][ACOMP] = CHAN_CAST ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
      }
   }
}



/* Trilinear filtering */
static void
sample_2d_linear_mipmap_linear( GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLuint n, const GLfloat texcoord[][4],
                                const GLfloat lambda[], GLchan rgba[][4] )
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level;
      COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda[i], level);
      if (level >= tObj->_MaxLevel) {
         sample_2d_linear(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                          texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_2d_linear(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_2d_linear(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         rgba[i][RCOMP] = CHAN_CAST ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
         rgba[i][GCOMP] = CHAN_CAST ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
         rgba[i][BCOMP] = CHAN_CAST ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
         rgba[i][ACOMP] = CHAN_CAST ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
      }
   }
}


static void
sample_2d_linear_mipmap_linear_repeat( GLcontext *ctx,
                                       const struct gl_texture_object *tObj,
                                       GLuint n, const GLfloat texcoord[][4],
                                       const GLfloat lambda[], GLchan rgba[][4] )
{
   GLuint i;
   ASSERT(lambda != NULL);
   ASSERT(tObj->WrapS == GL_REPEAT);
   ASSERT(tObj->WrapT == GL_REPEAT);
   ASSERT(tObj->_IsPowerOfTwo);
   for (i = 0; i < n; i++) {
      GLint level;
      COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda[i], level);
      if (level >= tObj->_MaxLevel) {
         sample_2d_linear_repeat(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                                 texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_2d_linear_repeat(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_2d_linear_repeat(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         rgba[i][RCOMP] = CHAN_CAST ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
         rgba[i][GCOMP] = CHAN_CAST ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
         rgba[i][BCOMP] = CHAN_CAST ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
         rgba[i][ACOMP] = CHAN_CAST ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
      }
   }
}


static void
sample_nearest_2d( GLcontext *ctx, GLuint texUnit,
                   const struct gl_texture_object *tObj, GLuint n,
                   const GLfloat texcoords[][4],
                   const GLfloat lambda[], GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) texUnit;
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_2d_nearest(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}



static void
sample_linear_2d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat texcoords[][4],
                  const GLfloat lambda[], GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) texUnit;
   (void) lambda;
   if (tObj->WrapS == GL_REPEAT && tObj->WrapT == GL_REPEAT
       && image->Border == 0) {
      for (i=0;i<n;i++) {
         sample_2d_linear_repeat(ctx, tObj, image, texcoords[i], rgba[i]);
      }
   }
   else {
      for (i=0;i<n;i++) {
         sample_2d_linear(ctx, tObj, image, texcoords[i], rgba[i]);
      }
   }
}


/*
 * Optimized 2-D texture sampling:
 *    S and T wrap mode == GL_REPEAT
 *    GL_NEAREST min/mag filter
 *    No border,
 *    RowStride == Width,
 *    Format = GL_RGB
 */
static void
opt_sample_rgb_2d( GLcontext *ctx, GLuint texUnit,
                   const struct gl_texture_object *tObj,
                   GLuint n, const GLfloat texcoords[][4],
                   const GLfloat lambda[], GLchan rgba[][4] )
{
   const struct gl_texture_image *img = tObj->Image[0][tObj->BaseLevel];
   const GLfloat width = (GLfloat) img->Width;
   const GLfloat height = (GLfloat) img->Height;
   const GLint colMask = img->Width - 1;
   const GLint rowMask = img->Height - 1;
   const GLint shift = img->WidthLog2;
   GLuint k;
   (void) ctx;
   (void) texUnit;
   (void) lambda;
   ASSERT(tObj->WrapS==GL_REPEAT);
   ASSERT(tObj->WrapT==GL_REPEAT);
   ASSERT(img->Border==0);
   ASSERT(img->Format==GL_RGB);
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


/*
 * Optimized 2-D texture sampling:
 *    S and T wrap mode == GL_REPEAT
 *    GL_NEAREST min/mag filter
 *    No border
 *    RowStride == Width,
 *    Format = GL_RGBA
 */
static void
opt_sample_rgba_2d( GLcontext *ctx, GLuint texUnit,
                    const struct gl_texture_object *tObj,
                    GLuint n, const GLfloat texcoords[][4],
                    const GLfloat lambda[], GLchan rgba[][4] )
{
   const struct gl_texture_image *img = tObj->Image[0][tObj->BaseLevel];
   const GLfloat width = (GLfloat) img->Width;
   const GLfloat height = (GLfloat) img->Height;
   const GLint colMask = img->Width - 1;
   const GLint rowMask = img->Height - 1;
   const GLint shift = img->WidthLog2;
   GLuint i;
   (void) ctx;
   (void) texUnit;
   (void) lambda;
   ASSERT(tObj->WrapS==GL_REPEAT);
   ASSERT(tObj->WrapT==GL_REPEAT);
   ASSERT(img->Border==0);
   ASSERT(img->Format==GL_RGBA);
   ASSERT(img->_IsPowerOfTwo);

   for (i = 0; i < n; i++) {
      const GLint col = IFLOOR(texcoords[i][0] * width) & colMask;
      const GLint row = IFLOOR(texcoords[i][1] * height) & rowMask;
      const GLint pos = (row << shift) | col;
      const GLchan *texel = ((GLchan *) img->Data) + (pos << 2);    /* pos*4 */
      COPY_CHAN4(rgba[i], texel);
   }
}


/*
 * Given an array of texture coordinate and lambda (level of detail)
 * values, return an array of texture sample.
 */
static void
sample_lambda_2d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj,
                  GLuint n, const GLfloat texcoords[][4],
                  const GLfloat lambda[], GLchan rgba[][4] )
{
   const struct gl_texture_image *tImg = tObj->Image[0][tObj->BaseLevel];
   GLuint minStart, minEnd;  /* texels with minification */
   GLuint magStart, magEnd;  /* texels with magnification */

   const GLboolean repeatNoBorderPOT = (tObj->WrapS == GL_REPEAT)
      && (tObj->WrapT == GL_REPEAT)
      && (tImg->Border == 0 && (tImg->Width == tImg->RowStride))
      && (tImg->Format != GL_COLOR_INDEX)
      && tImg->_IsPowerOfTwo;

   ASSERT(lambda != NULL);
   compute_min_mag_ranges(SWRAST_CONTEXT(ctx)->_MinMagThresh[texUnit],
                          n, lambda, &minStart, &minEnd, &magStart, &magEnd);

   if (minStart < minEnd) {
      /* do the minified texels */
      const GLuint m = minEnd - minStart;
      switch (tObj->MinFilter) {
      case GL_NEAREST:
         if (repeatNoBorderPOT) {
            switch (tImg->TexFormat->MesaFormat) {
            case MESA_FORMAT_RGB:
            case MESA_FORMAT_RGB888:
            /*case MESA_FORMAT_BGR888:*/
               opt_sample_rgb_2d(ctx, texUnit, tObj, m, texcoords + minStart,
                                 NULL, rgba + minStart);
               break;
            case MESA_FORMAT_RGBA:
            case MESA_FORMAT_RGBA8888:
            case MESA_FORMAT_ARGB8888:
            /*case MESA_FORMAT_ABGR8888:*/
            /*case MESA_FORMAT_BGRA8888:*/
	       opt_sample_rgba_2d(ctx, texUnit, tObj, m, texcoords + minStart,
                                  NULL, rgba + minStart);
               break;
            default:
               sample_nearest_2d(ctx, texUnit, tObj, m, texcoords + minStart,
                                 NULL, rgba + minStart );
            }
         }
         else {
            sample_nearest_2d(ctx, texUnit, tObj, m, texcoords + minStart,
                              NULL, rgba + minStart);
         }
         break;
      case GL_LINEAR:
	 sample_linear_2d(ctx, texUnit, tObj, m, texcoords + minStart,
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
            case MESA_FORMAT_RGB888:
            /*case MESA_FORMAT_BGR888:*/
               opt_sample_rgb_2d(ctx, texUnit, tObj, m, texcoords + magStart,
                                 NULL, rgba + magStart);
               break;
            case MESA_FORMAT_RGBA:
            case MESA_FORMAT_RGBA8888:
            case MESA_FORMAT_ARGB8888:
            /*case MESA_FORMAT_ABGR8888:*/
            /*case MESA_FORMAT_BGRA8888:*/
	       opt_sample_rgba_2d(ctx, texUnit, tObj, m, texcoords + magStart,
                                  NULL, rgba + magStart);
               break;
            default:
               sample_nearest_2d(ctx, texUnit, tObj, m, texcoords + magStart,
                                 NULL, rgba + magStart );
            }
         }
         else {
            sample_nearest_2d(ctx, texUnit, tObj, m, texcoords + magStart,
                              NULL, rgba + magStart);
         }
         break;
      case GL_LINEAR:
	 sample_linear_2d(ctx, texUnit, tObj, m, texcoords + magStart,
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

/*
 * Return the texture sample for coordinate (s,t,r) using GL_NEAREST filter.
 */
static void
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

   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapS, texcoord[0], width,  i);
   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapT, texcoord[1], height, j);
   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapR, texcoord[2], depth,  k);

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



/*
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
   GLuint useBorderColor;
   GLfloat u, v, w;
   (void) ctx;

   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapS, texcoord[0], u, width,  i0, i1);
   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapT, texcoord[1], v, height, j0, j1);
   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapR, texcoord[2], w, depth,  k0, k1);

   useBorderColor = 0;
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

   {
      const GLfloat a = FRAC(u);
      const GLfloat b = FRAC(v);
      const GLfloat c = FRAC(w);
#if CHAN_TYPE == GL_UNSIGNED_BYTE
      const GLint ia = IROUND_POS(a * ILERP_SCALE);
      const GLint ib = IROUND_POS(b * ILERP_SCALE);
      const GLint ic = IROUND_POS(c * ILERP_SCALE);
#endif
      GLchan t000[4], t010[4], t001[4], t011[4];
      GLchan t100[4], t110[4], t101[4], t111[4];

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
#if CHAN_TYPE == GL_FLOAT
      rgba[0] = lerp_3d(a, b, c,
                        t000[0], t100[0], t010[0], t110[0],
                        t001[0], t101[0], t011[0], t111[0]);
      rgba[1] = lerp_3d(a, b, c,
                        t000[1], t100[1], t010[1], t110[1],
                        t001[1], t101[1], t011[1], t111[1]);
      rgba[2] = lerp_3d(a, b, c,
                        t000[2], t100[2], t010[2], t110[2],
                        t001[2], t101[2], t011[2], t111[2]);
      rgba[3] = lerp_3d(a, b, c,
                        t000[3], t100[3], t010[3], t110[3],
                        t001[3], t101[3], t011[3], t111[3]);
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
      rgba[0] = (GLchan) (lerp_3d(a, b, c,
                                  t000[0], t100[0], t010[0], t110[0],
                                  t001[0], t101[0], t011[0], t111[0]) + 0.5F);
      rgba[1] = (GLchan) (lerp_3d(a, b, c,
                                  t000[1], t100[1], t010[1], t110[1],
                                  t001[1], t101[1], t011[1], t111[1]) + 0.5F);
      rgba[2] = (GLchan) (lerp_3d(a, b, c,
                                  t000[2], t100[2], t010[2], t110[2],
                                  t001[2], t101[2], t011[2], t111[2]) + 0.5F);
      rgba[3] = (GLchan) (lerp_3d(a, b, c,
                                  t000[3], t100[3], t010[3], t110[3],
                                  t001[3], t101[3], t011[3], t111[3]) + 0.5F);
#else
      ASSERT(CHAN_TYPE == GL_UNSIGNED_BYTE);
      rgba[0] = ilerp_3d(ia, ib, ic,
                         t000[0], t100[0], t010[0], t110[0],
                         t001[0], t101[0], t011[0], t111[0]);
      rgba[1] = ilerp_3d(ia, ib, ic,
                         t000[1], t100[1], t010[1], t110[1],
                         t001[1], t101[1], t011[1], t111[1]);
      rgba[2] = ilerp_3d(ia, ib, ic,
                         t000[2], t100[2], t010[2], t110[2],
                         t001[2], t101[2], t011[2], t111[2]);
      rgba[3] = ilerp_3d(ia, ib, ic,
                         t000[3], t100[3], t010[3], t110[3],
                         t001[3], t101[3], t011[3], t111[3]);
#endif
   }
}



static void
sample_3d_nearest_mipmap_nearest(GLcontext *ctx,
                                 const struct gl_texture_object *tObj,
                                 GLuint n, const GLfloat texcoord[][4],
                                 const GLfloat lambda[], GLchan rgba[][4] )
{
   GLuint i;
   for (i = 0; i < n; i++) {
      GLint level;
      COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda[i], level);
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
      GLint level;
      COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda[i], level);
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
      GLint level;
      COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda[i], level);
      if (level >= tObj->_MaxLevel) {
         sample_3d_nearest(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                           texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_3d_nearest(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_3d_nearest(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         rgba[i][RCOMP] = CHAN_CAST ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
         rgba[i][GCOMP] = CHAN_CAST ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
         rgba[i][BCOMP] = CHAN_CAST ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
         rgba[i][ACOMP] = CHAN_CAST ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
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
      GLint level;
      COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda[i], level);
      if (level >= tObj->_MaxLevel) {
         sample_3d_linear(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                          texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];  /* texels */
         const GLfloat f = FRAC(lambda[i]);
         sample_3d_linear(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_3d_linear(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         rgba[i][RCOMP] = CHAN_CAST ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
         rgba[i][GCOMP] = CHAN_CAST ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
         rgba[i][BCOMP] = CHAN_CAST ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
         rgba[i][ACOMP] = CHAN_CAST ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
      }
   }
}


static void
sample_nearest_3d(GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat texcoords[][4], const GLfloat lambda[],
                  GLchan rgba[][4])
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) texUnit;
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_3d_nearest(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}



static void
sample_linear_3d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat texcoords[][4],
		  const GLfloat lambda[], GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) texUnit;
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_3d_linear(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}


/*
 * Given an (s,t,r) texture coordinate and lambda (level of detail) value,
 * return a texture sample.
 */
static void
sample_lambda_3d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat texcoords[][4], const GLfloat lambda[],
                  GLchan rgba[][4] )
{
   GLuint minStart, minEnd;  /* texels with minification */
   GLuint magStart, magEnd;  /* texels with magnification */
   GLuint i;

   ASSERT(lambda != NULL);
   compute_min_mag_ranges(SWRAST_CONTEXT(ctx)->_MinMagThresh[texUnit],
                          n, lambda, &minStart, &minEnd, &magStart, &magEnd);

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

/*
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
   const struct gl_texture_image **imgArray;
   const GLfloat arx = FABSF(rx),   ary = FABSF(ry),   arz = FABSF(rz);
   GLfloat sc, tc, ma;

   if (arx > ary && arx > arz) {
      if (rx >= 0.0F) {
         imgArray = (const struct gl_texture_image **) texObj->Image[FACE_POS_X];
         sc = -rz;
         tc = -ry;
         ma = arx;
      }
      else {
         imgArray = (const struct gl_texture_image **) texObj->Image[FACE_NEG_X];
         sc = rz;
         tc = -ry;
         ma = arx;
      }
   }
   else if (ary > arx && ary > arz) {
      if (ry >= 0.0F) {
         imgArray = (const struct gl_texture_image **) texObj->Image[FACE_POS_Y];
         sc = rx;
         tc = rz;
         ma = ary;
      }
      else {
         imgArray = (const struct gl_texture_image **) texObj->Image[FACE_NEG_Y];
         sc = rx;
         tc = -rz;
         ma = ary;
      }
   }
   else {
      if (rz > 0.0F) {
         imgArray = (const struct gl_texture_image **) texObj->Image[FACE_POS_Z];
         sc = rx;
         tc = -ry;
         ma = arz;
      }
      else {
         imgArray = (const struct gl_texture_image **) texObj->Image[FACE_NEG_Z];
         sc = -rx;
         tc = -ry;
         ma = arz;
      }
   }

   newCoord[0] = ( sc / ma + 1.0F ) * 0.5F;
   newCoord[1] = ( tc / ma + 1.0F ) * 0.5F;
   return imgArray;
}


static void
sample_nearest_cube(GLcontext *ctx, GLuint texUnit,
		    const struct gl_texture_object *tObj, GLuint n,
                    const GLfloat texcoords[][4], const GLfloat lambda[],
                    GLchan rgba[][4])
{
   GLuint i;
   (void) texUnit;
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
sample_linear_cube(GLcontext *ctx, GLuint texUnit,
		   const struct gl_texture_object *tObj, GLuint n,
                   const GLfloat texcoords[][4],
		   const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   (void) texUnit;
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
sample_cube_nearest_mipmap_nearest(GLcontext *ctx, GLuint texUnit,
                                   const struct gl_texture_object *tObj,
                                   GLuint n, const GLfloat texcoord[][4],
                                   const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   (void) texUnit;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      const struct gl_texture_image **images;
      GLfloat newCoord[4];
      GLint level;
      COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda[i], level);
      images = choose_cube_face(tObj, texcoord[i], newCoord);
      sample_2d_nearest(ctx, tObj, images[level], newCoord, rgba[i]);
   }
}


static void
sample_cube_linear_mipmap_nearest(GLcontext *ctx, GLuint texUnit,
                                  const struct gl_texture_object *tObj,
                                  GLuint n, const GLfloat texcoord[][4],
                                  const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   (void) texUnit;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      const struct gl_texture_image **images;
      GLfloat newCoord[4];
      GLint level;
      COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda[i], level);
      images = choose_cube_face(tObj, texcoord[i], newCoord);
      sample_2d_linear(ctx, tObj, images[level], newCoord, rgba[i]);
   }
}


static void
sample_cube_nearest_mipmap_linear(GLcontext *ctx, GLuint texUnit,
                                  const struct gl_texture_object *tObj,
                                  GLuint n, const GLfloat texcoord[][4],
                                  const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   (void) texUnit;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      const struct gl_texture_image **images;
      GLfloat newCoord[4];
      GLint level;
      COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda[i], level);
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
         rgba[i][RCOMP] = CHAN_CAST ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
         rgba[i][GCOMP] = CHAN_CAST ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
         rgba[i][BCOMP] = CHAN_CAST ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
         rgba[i][ACOMP] = CHAN_CAST ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
      }
   }
}


static void
sample_cube_linear_mipmap_linear(GLcontext *ctx, GLuint texUnit,
                                 const struct gl_texture_object *tObj,
                                 GLuint n, const GLfloat texcoord[][4],
                                 const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   (void) texUnit;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      const struct gl_texture_image **images;
      GLfloat newCoord[4];
      GLint level;
      COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda[i], level);
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
         rgba[i][RCOMP] = CHAN_CAST ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
         rgba[i][GCOMP] = CHAN_CAST ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
         rgba[i][BCOMP] = CHAN_CAST ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
         rgba[i][ACOMP] = CHAN_CAST ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
      }
   }
}


static void
sample_lambda_cube( GLcontext *ctx, GLuint texUnit,
		    const struct gl_texture_object *tObj, GLuint n,
		    const GLfloat texcoords[][4], const GLfloat lambda[],
		    GLchan rgba[][4])
{
   GLuint minStart, minEnd;  /* texels with minification */
   GLuint magStart, magEnd;  /* texels with magnification */

   ASSERT(lambda != NULL);
   compute_min_mag_ranges(SWRAST_CONTEXT(ctx)->_MinMagThresh[texUnit],
                          n, lambda, &minStart, &minEnd, &magStart, &magEnd);

   if (minStart < minEnd) {
      /* do the minified texels */
      const GLuint m = minEnd - minStart;
      switch (tObj->MinFilter) {
      case GL_NEAREST:
         sample_nearest_cube(ctx, texUnit, tObj, m, texcoords + minStart,
                             lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR:
         sample_linear_cube(ctx, texUnit, tObj, m, texcoords + minStart,
                            lambda + minStart, rgba + minStart);
         break;
      case GL_NEAREST_MIPMAP_NEAREST:
         sample_cube_nearest_mipmap_nearest(ctx, texUnit, tObj, m,
                                            texcoords + minStart,
                                           lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_NEAREST:
         sample_cube_linear_mipmap_nearest(ctx, texUnit, tObj, m,
                                           texcoords + minStart,
                                           lambda + minStart, rgba + minStart);
         break;
      case GL_NEAREST_MIPMAP_LINEAR:
         sample_cube_nearest_mipmap_linear(ctx, texUnit, tObj, m,
                                           texcoords + minStart,
                                           lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_LINEAR:
         sample_cube_linear_mipmap_linear(ctx, texUnit, tObj, m,
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
         sample_nearest_cube(ctx, texUnit, tObj, m, texcoords + magStart,
                             lambda + magStart, rgba + magStart);
         break;
      case GL_LINEAR:
         sample_linear_cube(ctx, texUnit, tObj, m, texcoords + magStart,
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

static void
sample_nearest_rect(GLcontext *ctx, GLuint texUnit,
		    const struct gl_texture_object *tObj, GLuint n,
                    const GLfloat texcoords[][4], const GLfloat lambda[],
                    GLchan rgba[][4])
{
   const struct gl_texture_image *img = tObj->Image[0][0];
   const GLfloat width = (GLfloat) img->Width;
   const GLfloat height = (GLfloat) img->Height;
   const GLint width_minus_1 = img->Width - 1;
   const GLint height_minus_1 = img->Height - 1;
   GLuint i;

   (void) ctx;
   (void) texUnit;
   (void) lambda;

   ASSERT(tObj->WrapS == GL_CLAMP ||
          tObj->WrapS == GL_CLAMP_TO_EDGE ||
          tObj->WrapS == GL_CLAMP_TO_BORDER);
   ASSERT(tObj->WrapT == GL_CLAMP ||
          tObj->WrapT == GL_CLAMP_TO_EDGE ||
          tObj->WrapT == GL_CLAMP_TO_BORDER);
   ASSERT(img->Format != GL_COLOR_INDEX);

   /* XXX move Wrap mode tests outside of loops for common cases */
   for (i = 0; i < n; i++) {
      GLint row, col;
      /* NOTE: we DO NOT use [0, 1] texture coordinates! */
      if (tObj->WrapS == GL_CLAMP) {
         col = IFLOOR( CLAMP(texcoords[i][0], 0.0F, width - 1) );
      }
      else if (tObj->WrapS == GL_CLAMP_TO_EDGE) {
         col = IFLOOR( CLAMP(texcoords[i][0], 0.5F, width - 0.5F) );
      }
      else {
         col = IFLOOR( CLAMP(texcoords[i][0], -0.5F, width + 0.5F) );
      }
      if (tObj->WrapT == GL_CLAMP) {
         row = IFLOOR( CLAMP(texcoords[i][1], 0.0F, height - 1) );
      }
      else if (tObj->WrapT == GL_CLAMP_TO_EDGE) {
         row = IFLOOR( CLAMP(texcoords[i][1], 0.5F, height - 0.5F) );
      }
      else {
         row = IFLOOR( CLAMP(texcoords[i][1], -0.5F, height + 0.5F) );
      }

      if (col < 0 || col > width_minus_1 || row < 0 || row > height_minus_1)
         COPY_CHAN4(rgba[i], tObj->_BorderChan);
      else
         img->FetchTexelc(img, col, row, 0, rgba[i]);
   }
}


static void
sample_linear_rect(GLcontext *ctx, GLuint texUnit,
		   const struct gl_texture_object *tObj, GLuint n,
                   const GLfloat texcoords[][4],
		   const GLfloat lambda[], GLchan rgba[][4])
{
   const struct gl_texture_image *img = tObj->Image[0][0];
   const GLfloat width = (GLfloat) img->Width;
   const GLfloat height = (GLfloat) img->Height;
   const GLint width_minus_1 = img->Width - 1;
   const GLint height_minus_1 = img->Height - 1;
   GLuint i;

   (void) ctx;
   (void) texUnit;
   (void) lambda;

   ASSERT(tObj->WrapS == GL_CLAMP ||
          tObj->WrapS == GL_CLAMP_TO_EDGE ||
          tObj->WrapS == GL_CLAMP_TO_BORDER);
   ASSERT(tObj->WrapT == GL_CLAMP ||
          tObj->WrapT == GL_CLAMP_TO_EDGE ||
          tObj->WrapT == GL_CLAMP_TO_BORDER);
   ASSERT(img->Format != GL_COLOR_INDEX);

   /* XXX lots of opportunity for optimization in this loop */
   for (i = 0; i < n; i++) {
      GLfloat frow, fcol;
      GLint i0, j0, i1, j1;
      GLchan t00[4], t01[4], t10[4], t11[4];
      GLfloat a, b;
      GLuint useBorderColor = 0;
#if CHAN_TYPE == GL_UNSIGNED_BYTE
      GLint ia, ib;
#endif

      /* NOTE: we DO NOT use [0, 1] texture coordinates! */
      if (tObj->WrapS == GL_CLAMP) {
         /* Not exactly what the spec says, but it matches NVIDIA output */
         fcol = CLAMP(texcoords[i][0] - 0.5F, 0.0, width_minus_1);
         i0 = IFLOOR(fcol);
         i1 = i0 + 1;
      }
      else if (tObj->WrapS == GL_CLAMP_TO_EDGE) {
         fcol = CLAMP(texcoords[i][0], 0.5F, width - 0.5F);
         fcol -= 0.5F;
         i0 = IFLOOR(fcol);
         i1 = i0 + 1;
         if (i1 > width_minus_1)
            i1 = width_minus_1;
      }
      else {
         ASSERT(tObj->WrapS == GL_CLAMP_TO_BORDER);
         fcol = CLAMP(texcoords[i][0], -0.5F, width + 0.5F);
         fcol -= 0.5F;
         i0 = IFLOOR(fcol);
         i1 = i0 + 1;
      }

      if (tObj->WrapT == GL_CLAMP) {
         /* Not exactly what the spec says, but it matches NVIDIA output */
         frow = CLAMP(texcoords[i][1] - 0.5F, 0.0, width_minus_1);
         j0 = IFLOOR(frow);
         j1 = j0 + 1;
      }
      else if (tObj->WrapT == GL_CLAMP_TO_EDGE) {
         frow = CLAMP(texcoords[i][1], 0.5F, height - 0.5F);
         frow -= 0.5F;
         j0 = IFLOOR(frow);
         j1 = j0 + 1;
         if (j1 > height_minus_1)
            j1 = height_minus_1;
      }
      else {
         ASSERT(tObj->WrapT == GL_CLAMP_TO_BORDER);
         frow = CLAMP(texcoords[i][1], -0.5F, height + 0.5F);
         frow -= 0.5F;
         j0 = IFLOOR(frow);
         j1 = j0 + 1;
      }

      /* compute integer rows/columns */
      if (i0 < 0 || i0 > width_minus_1)   useBorderColor |= I0BIT;
      if (i1 < 0 || i1 > width_minus_1)   useBorderColor |= I1BIT;
      if (j0 < 0 || j0 > height_minus_1)  useBorderColor |= J0BIT;
      if (j1 < 0 || j1 > height_minus_1)  useBorderColor |= J1BIT;

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

      /* compute interpolants */
      a = FRAC(fcol);
      b = FRAC(frow);
#if CHAN_TYPE == GL_UNSIGNED_BYTE
      ia = IROUND_POS(a * ILERP_SCALE);
      ib = IROUND_POS(b * ILERP_SCALE);
#endif

      /* do bilinear interpolation of texel colors */
#if CHAN_TYPE == GL_FLOAT
      rgba[i][0] = lerp_2d(a, b, t00[0], t10[0], t01[0], t11[0]);
      rgba[i][1] = lerp_2d(a, b, t00[1], t10[1], t01[1], t11[1]);
      rgba[i][2] = lerp_2d(a, b, t00[2], t10[2], t01[2], t11[2]);
      rgba[i][3] = lerp_2d(a, b, t00[3], t10[3], t01[3], t11[3]);
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
      rgba[i][0] = (GLchan) (lerp_2d(a, b, t00[0], t10[0], t01[0], t11[0]) + 0.5);
      rgba[i][1] = (GLchan) (lerp_2d(a, b, t00[1], t10[1], t01[1], t11[1]) + 0.5);
      rgba[i][2] = (GLchan) (lerp_2d(a, b, t00[2], t10[2], t01[2], t11[2]) + 0.5);
      rgba[i][3] = (GLchan) (lerp_2d(a, b, t00[3], t10[3], t01[3], t11[3]) + 0.5);
#else
      ASSERT(CHAN_TYPE == GL_UNSIGNED_BYTE);
      rgba[i][0] = ilerp_2d(ia, ib, t00[0], t10[0], t01[0], t11[0]);
      rgba[i][1] = ilerp_2d(ia, ib, t00[1], t10[1], t01[1], t11[1]);
      rgba[i][2] = ilerp_2d(ia, ib, t00[2], t10[2], t01[2], t11[2]);
      rgba[i][3] = ilerp_2d(ia, ib, t00[3], t10[3], t01[3], t11[3]);
#endif
   }
}


static void
sample_lambda_rect( GLcontext *ctx, GLuint texUnit,
		    const struct gl_texture_object *tObj, GLuint n,
		    const GLfloat texcoords[][4], const GLfloat lambda[],
		    GLchan rgba[][4])
{
   GLuint minStart, minEnd, magStart, magEnd;

   /* We only need lambda to decide between minification and magnification.
    * There is no mipmapping with rectangular textures.
    */
   compute_min_mag_ranges(SWRAST_CONTEXT(ctx)->_MinMagThresh[texUnit],
                          n, lambda, &minStart, &minEnd, &magStart, &magEnd);

   if (minStart < minEnd) {
      if (tObj->MinFilter == GL_NEAREST) {
         sample_nearest_rect( ctx, texUnit, tObj, minEnd - minStart,
                              texcoords + minStart, NULL, rgba + minStart);
      }
      else {
         sample_linear_rect( ctx, texUnit, tObj, minEnd - minStart,
                             texcoords + minStart, NULL, rgba + minStart);
      }
   }
   if (magStart < magEnd) {
      if (tObj->MagFilter == GL_NEAREST) {
         sample_nearest_rect( ctx, texUnit, tObj, magEnd - magStart,
                              texcoords + magStart, NULL, rgba + magStart);
      }
      else {
         sample_linear_rect( ctx, texUnit, tObj, magEnd - magStart,
                             texcoords + magStart, NULL, rgba + magStart);
      }
   }
}



/*
 * Sample a shadow/depth texture.
 */
static void
sample_depth_texture( GLcontext *ctx, GLuint unit,
                      const struct gl_texture_object *tObj, GLuint n,
                      const GLfloat texcoords[][4], const GLfloat lambda[],
                      GLchan texel[][4] )
{
   const GLint baseLevel = tObj->BaseLevel;
   const struct gl_texture_image *texImage = tObj->Image[0][baseLevel];
   const GLuint width = texImage->Width;
   const GLuint height = texImage->Height;
   GLchan ambient;
   GLenum function;
   GLchan result;

   (void) lambda;
   (void) unit;

   ASSERT(tObj->Image[0][tObj->BaseLevel]->Format == GL_DEPTH_COMPONENT);
   ASSERT(tObj->Target == GL_TEXTURE_1D ||
          tObj->Target == GL_TEXTURE_2D ||
          tObj->Target == GL_TEXTURE_RECTANGLE_NV);

   UNCLAMPED_FLOAT_TO_CHAN(ambient, tObj->ShadowAmbient);

   /* XXXX if tObj->MinFilter != tObj->MagFilter, we're ignoring lambda */

   /* XXX this could be precomputed and saved in the texture object */
   if (tObj->CompareFlag) {
      /* GL_SGIX_shadow */
      if (tObj->CompareOperator == GL_TEXTURE_LEQUAL_R_SGIX) {
         function = GL_LEQUAL;
      }
      else {
         ASSERT(tObj->CompareOperator == GL_TEXTURE_GEQUAL_R_SGIX);
         function = GL_GEQUAL;
      }
   }
   else if (tObj->CompareMode == GL_COMPARE_R_TO_TEXTURE_ARB) {
      /* GL_ARB_shadow */
      function = tObj->CompareFunc;
   }
   else {
      function = GL_NONE;  /* pass depth through as grayscale */
   }

   if (tObj->MagFilter == GL_NEAREST) {
      GLuint i;
      for (i = 0; i < n; i++) {
         GLfloat depthSample;
         GLint col, row;
         /* XXX fix for texture rectangle! */
         COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapS, texcoords[i][0], width, col);
         COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapT, texcoords[i][1], height, row);
         texImage->FetchTexelf(texImage, col, row, 0, &depthSample);

         switch (function) {
         case GL_LEQUAL:
            result = (texcoords[i][2] <= depthSample) ? CHAN_MAX : ambient;
            break;
         case GL_GEQUAL:
            result = (texcoords[i][2] >= depthSample) ? CHAN_MAX : ambient;
            break;
         case GL_LESS:
            result = (texcoords[i][2] < depthSample) ? CHAN_MAX : ambient;
            break;
         case GL_GREATER:
            result = (texcoords[i][2] > depthSample) ? CHAN_MAX : ambient;
            break;
         case GL_EQUAL:
            result = (texcoords[i][2] == depthSample) ? CHAN_MAX : ambient;
            break;
         case GL_NOTEQUAL:
            result = (texcoords[i][2] != depthSample) ? CHAN_MAX : ambient;
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
         GLfloat u, v;
         GLuint useBorderTexel;

         /* XXX fix for texture rectangle! */
         COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapS, texcoords[i][0], u, width, i0, i1);
         COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapT, texcoords[i][1], v, height,j0, j1);

         useBorderTexel = 0;
         if (texImage->Border) {
            i0 += texImage->Border;
            i1 += texImage->Border;
            j0 += texImage->Border;
            j1 += texImage->Border;
         }
         else {
            if (i0 < 0 || i0 >= (GLint) width)   useBorderTexel |= I0BIT;
            if (i1 < 0 || i1 >= (GLint) width)   useBorderTexel |= I1BIT;
            if (j0 < 0 || j0 >= (GLint) height)  useBorderTexel |= J0BIT;
            if (j1 < 0 || j1 >= (GLint) height)  useBorderTexel |= J1BIT;
         }

         /* get four depth samples from the texture */
         if (useBorderTexel & (I0BIT | J0BIT)) {
            depth00 = 1.0;
         }
         else {
            texImage->FetchTexelf(texImage, i0, j0, 0, &depth00);
         }
         if (useBorderTexel & (I1BIT | J0BIT)) {
            depth10 = 1.0;
         }
         else {
            texImage->FetchTexelf(texImage, i1, j0, 0, &depth10);
         }
         if (useBorderTexel & (I0BIT | J1BIT)) {
            depth01 = 1.0;
         }
         else {
            texImage->FetchTexelf(texImage, i0, j1, 0, &depth01);
         }
         if (useBorderTexel & (I1BIT | J1BIT)) {
            depth11 = 1.0;
         }
         else {
            texImage->FetchTexelf(texImage, i1, j1, 0, &depth11);
         }

         if (0) {
            /* compute a single weighted depth sample and do one comparison */
            const GLfloat a = FRAC(u + 1.0F);
            const GLfloat b = FRAC(v + 1.0F);
            const GLfloat depthSample
               = lerp_2d(a, b, depth00, depth10, depth01, depth11);
            if ((depthSample <= texcoords[i][2] && function == GL_LEQUAL) ||
                (depthSample >= texcoords[i][2] && function == GL_GEQUAL)) {
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
               if (depth00 <= texcoords[i][2])  luminance -= d;
               if (depth01 <= texcoords[i][2])  luminance -= d;
               if (depth10 <= texcoords[i][2])  luminance -= d;
               if (depth11 <= texcoords[i][2])  luminance -= d;
               result = (GLchan) luminance;
               break;
            case GL_GEQUAL:
               if (depth00 >= texcoords[i][2])  luminance -= d;
               if (depth01 >= texcoords[i][2])  luminance -= d;
               if (depth10 >= texcoords[i][2])  luminance -= d;
               if (depth11 >= texcoords[i][2])  luminance -= d;
               result = (GLchan) luminance;
               break;
            case GL_LESS:
               if (depth00 < texcoords[i][2])  luminance -= d;
               if (depth01 < texcoords[i][2])  luminance -= d;
               if (depth10 < texcoords[i][2])  luminance -= d;
               if (depth11 < texcoords[i][2])  luminance -= d;
               result = (GLchan) luminance;
               break;
            case GL_GREATER:
               if (depth00 > texcoords[i][2])  luminance -= d;
               if (depth01 > texcoords[i][2])  luminance -= d;
               if (depth10 > texcoords[i][2])  luminance -= d;
               if (depth11 > texcoords[i][2])  luminance -= d;
               result = (GLchan) luminance;
               break;
            case GL_EQUAL:
               if (depth00 == texcoords[i][2])  luminance -= d;
               if (depth01 == texcoords[i][2])  luminance -= d;
               if (depth10 == texcoords[i][2])  luminance -= d;
               if (depth11 == texcoords[i][2])  luminance -= d;
               result = (GLchan) luminance;
               break;
            case GL_NOTEQUAL:
               if (depth00 != texcoords[i][2])  luminance -= d;
               if (depth01 != texcoords[i][2])  luminance -= d;
               if (depth10 != texcoords[i][2])  luminance -= d;
               if (depth11 != texcoords[i][2])  luminance -= d;
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
                  const GLfloat a = FRAC(u + 1.0F);
                  const GLfloat b = FRAC(v + 1.0F);
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
   if (texImage->Format != GL_DEPTH_COMPONENT) {
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
         COMPUTE_NEAREST_TEXEL_LOCATION(texObj->WrapS, texcoords[i][0],
					width, col);
         COMPUTE_NEAREST_TEXEL_LOCATION(texObj->WrapT, texcoords[i][1],
					height, row);

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
 * Note: fragment programss don't observe the texture enable/disable flags.
 */
static void
null_sample_func( GLcontext *ctx, GLuint texUnit,
		  const struct gl_texture_object *tObj, GLuint n,
		  const GLfloat texcoords[][4], const GLfloat lambda[],
		  GLchan rgba[][4])
{
   GLuint i;
   (void) ctx;
   (void) texUnit;
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
 * Setup the texture sampling function for this texture object.
 */
texture_sample_func
_swrast_choose_texture_sample_func( GLcontext *ctx,
				    const struct gl_texture_object *t )
{
   if (!t || !t->Complete) {
      return &null_sample_func;
   }
   else {
      const GLboolean needLambda = (GLboolean) (t->MinFilter != t->MagFilter);
      const GLenum format = t->Image[0][t->BaseLevel]->Format;

      switch (t->Target) {
      case GL_TEXTURE_1D:
         if (format == GL_DEPTH_COMPONENT) {
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
         if (format == GL_DEPTH_COMPONENT) {
            return &sample_depth_texture;
         }
         else if (needLambda) {
            return &sample_lambda_2d;
         }
         else if (t->MinFilter == GL_LINEAR) {
            return &sample_linear_2d;
         }
         else {
            GLint baseLevel = t->BaseLevel;
            ASSERT(t->MinFilter == GL_NEAREST);
            if (t->WrapS == GL_REPEAT &&
                t->WrapT == GL_REPEAT &&
                t->_IsPowerOfTwo &&
                t->Image[0][baseLevel]->Border == 0 &&
                t->Image[0][baseLevel]->TexFormat->MesaFormat == MESA_FORMAT_RGB) {
               return &opt_sample_rgb_2d;
            }
            else if (t->WrapS == GL_REPEAT &&
                     t->WrapT == GL_REPEAT &&
                     t->_IsPowerOfTwo &&
                     t->Image[0][baseLevel]->Border == 0 &&
                     t->Image[0][baseLevel]->TexFormat->MesaFormat == MESA_FORMAT_RGBA) {
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
         if (needLambda) {
            return &sample_lambda_rect;
         }
         else if (t->MinFilter == GL_LINEAR) {
            return &sample_linear_rect;
         }
         else {
            ASSERT(t->MinFilter == GL_NEAREST);
            return &sample_nearest_rect;
         }
      default:
         _mesa_problem(ctx,
                       "invalid target in _swrast_choose_texture_sample_func");
         return &null_sample_func;
      }
   }
}


#define PROD(A,B)   ( (GLuint)(A) * ((GLuint)(B)+1) )
#define S_PROD(A,B) ( (GLint)(A) * ((GLint)(B)+1) )


/**
 * Do texture application for GL_ARB/EXT_texture_env_combine.
 * This function also supports GL_{EXT,ARB}_texture_env_dot3 and
 * GL_ATI_texture_env_combine3.  Since "classic" texture environments are
 * implemented using GL_ARB_texture_env_combine-like state, this same function
 * is used for classic texture environment application as well.
 *
 * \param ctx          rendering context
 * \param textureUnit  the texture unit to apply
 * \param n            number of fragments to process (span width)
 * \param primary_rgba incoming fragment color array
 * \param texelBuffer  pointer to texel colors for all texture units
 *
 * \param rgba         incoming colors, which get modified here
 */
static INLINE void
texture_combine( const GLcontext *ctx, GLuint unit, GLuint n,
                 CONST GLchan (*primary_rgba)[4],
                 CONST GLchan *texelBuffer,
                 GLchan (*rgba)[4] )
{
   const struct gl_texture_unit *textureUnit = &(ctx->Texture.Unit[unit]);
   const GLchan (*argRGB [3])[4];
   const GLchan (*argA [3])[4];
   const GLuint RGBshift = textureUnit->_CurrentCombine->ScaleShiftRGB;
   const GLuint Ashift   = textureUnit->_CurrentCombine->ScaleShiftA;
#if CHAN_TYPE == GL_FLOAT
   const GLchan RGBmult = (GLfloat) (1 << RGBshift);
   const GLchan Amult = (GLfloat) (1 << Ashift);
   static const GLchan one[4] = { 1.0, 1.0, 1.0, 1.0 };
   static const GLchan zero[4] = { 0.0, 0.0, 0.0, 0.0 };
#else
   const GLint half = (CHAN_MAX + 1) / 2;
   static const GLchan one[4] = { CHAN_MAX, CHAN_MAX, CHAN_MAX, CHAN_MAX };
   static const GLchan zero[4] = { 0, 0, 0, 0 };
#endif
   GLuint i, j;
   GLuint numColorArgs;
   GLuint numAlphaArgs;

   /* GLchan ccolor[3][4]; */
   DEFMNARRAY(GLchan, ccolor, 3, 3 * MAX_WIDTH, 4);  /* mac 32k limitation */
   CHECKARRAY(ccolor, return);  /* mac 32k limitation */

   ASSERT(ctx->Extensions.EXT_texture_env_combine ||
          ctx->Extensions.ARB_texture_env_combine);
   ASSERT(SWRAST_CONTEXT(ctx)->_AnyTextureCombine);


   /*
   printf("modeRGB 0x%x  modeA 0x%x  srcRGB1 0x%x  srcA1 0x%x  srcRGB2 0x%x  srcA2 0x%x\n",
          textureUnit->_CurrentCombine->ModeRGB,
          textureUnit->_CurrentCombine->ModeA,
          textureUnit->_CurrentCombine->SourceRGB[0],
          textureUnit->_CurrentCombine->SourceA[0],
          textureUnit->_CurrentCombine->SourceRGB[1],
          textureUnit->_CurrentCombine->SourceA[1]);
   */

   /*
    * Do operand setup for up to 3 operands.  Loop over the terms.
    */
   numColorArgs = textureUnit->_CurrentCombine->_NumArgsRGB;
   numAlphaArgs = textureUnit->_CurrentCombine->_NumArgsA;

   for (j = 0; j < numColorArgs; j++) {
      const GLenum srcRGB = textureUnit->_CurrentCombine->SourceRGB[j];


      switch (srcRGB) {
         case GL_TEXTURE:
            argRGB[j] = (const GLchan (*)[4])
               (texelBuffer + unit * (n * 4 * sizeof(GLchan)));
            break;
         case GL_PRIMARY_COLOR:
            argRGB[j] = primary_rgba;
            break;
         case GL_PREVIOUS:
            argRGB[j] = (const GLchan (*)[4]) rgba;
            break;
         case GL_CONSTANT:
            {
               GLchan (*c)[4] = ccolor[j];
               GLchan red, green, blue, alpha;
               UNCLAMPED_FLOAT_TO_CHAN(red,   textureUnit->EnvColor[0]);
               UNCLAMPED_FLOAT_TO_CHAN(green, textureUnit->EnvColor[1]);
               UNCLAMPED_FLOAT_TO_CHAN(blue,  textureUnit->EnvColor[2]);
               UNCLAMPED_FLOAT_TO_CHAN(alpha, textureUnit->EnvColor[3]);
               for (i = 0; i < n; i++) {
                  c[i][RCOMP] = red;
                  c[i][GCOMP] = green;
                  c[i][BCOMP] = blue;
                  c[i][ACOMP] = alpha;
               }
               argRGB[j] = (const GLchan (*)[4]) ccolor[j];
            }
            break;
	 /* GL_ATI_texture_env_combine3 allows GL_ZERO & GL_ONE as sources.
	  */
	 case GL_ZERO:
            argRGB[j] = & zero;
            break;
	 case GL_ONE:
            argRGB[j] = & one;
            break;
         default:
            /* ARB_texture_env_crossbar source */
            {
               const GLuint srcUnit = srcRGB - GL_TEXTURE0;
               ASSERT(srcUnit < ctx->Const.MaxTextureUnits);
               if (!ctx->Texture.Unit[srcUnit]._ReallyEnabled)
                  return;
               argRGB[j] = (const GLchan (*)[4])
                  (texelBuffer + srcUnit * (n * 4 * sizeof(GLchan)));
            }
      }

      if (textureUnit->_CurrentCombine->OperandRGB[j] != GL_SRC_COLOR) {
         const GLchan (*src)[4] = argRGB[j];
         GLchan (*dst)[4] = ccolor[j];

         /* point to new arg[j] storage */
         argRGB[j] = (const GLchan (*)[4]) ccolor[j];

         if (textureUnit->_CurrentCombine->OperandRGB[j] == GL_ONE_MINUS_SRC_COLOR) {
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = CHAN_MAX - src[i][RCOMP];
               dst[i][GCOMP] = CHAN_MAX - src[i][GCOMP];
               dst[i][BCOMP] = CHAN_MAX - src[i][BCOMP];
            }
         }
         else if (textureUnit->_CurrentCombine->OperandRGB[j] == GL_SRC_ALPHA) {
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = src[i][ACOMP];
               dst[i][GCOMP] = src[i][ACOMP];
               dst[i][BCOMP] = src[i][ACOMP];
            }
         }
         else {
            ASSERT(textureUnit->_CurrentCombine->OperandRGB[j] ==GL_ONE_MINUS_SRC_ALPHA);
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = CHAN_MAX - src[i][ACOMP];
               dst[i][GCOMP] = CHAN_MAX - src[i][ACOMP];
               dst[i][BCOMP] = CHAN_MAX - src[i][ACOMP];
            }
         }
      }
   }


   for (j = 0; j < numAlphaArgs; j++) {
      const GLenum srcA = textureUnit->_CurrentCombine->SourceA[j];

      switch (srcA) {
         case GL_TEXTURE:
            argA[j] = (const GLchan (*)[4])
               (texelBuffer + unit * (n * 4 * sizeof(GLchan)));
            break;
         case GL_PRIMARY_COLOR:
            argA[j] = primary_rgba;
            break;
         case GL_PREVIOUS:
            argA[j] = (const GLchan (*)[4]) rgba;
            break;
         case GL_CONSTANT:
            {
               GLchan alpha, (*c)[4] = ccolor[j];
               UNCLAMPED_FLOAT_TO_CHAN(alpha, textureUnit->EnvColor[3]);
               for (i = 0; i < n; i++)
                  c[i][ACOMP] = alpha;
               argA[j] = (const GLchan (*)[4]) ccolor[j];
            }
            break;
	 /* GL_ATI_texture_env_combine3 allows GL_ZERO & GL_ONE as sources.
	  */
	 case GL_ZERO:
            argA[j] = & zero;
            break;
	 case GL_ONE:
            argA[j] = & one;
            break;
         default:
            /* ARB_texture_env_crossbar source */
            {
               const GLuint srcUnit = srcA - GL_TEXTURE0;
               ASSERT(srcUnit < ctx->Const.MaxTextureUnits);
               if (!ctx->Texture.Unit[srcUnit]._ReallyEnabled)
                  return;
               argA[j] = (const GLchan (*)[4])
                  (texelBuffer + srcUnit * (n * 4 * sizeof(GLchan)));
            }
      }

      if (textureUnit->_CurrentCombine->OperandA[j] == GL_ONE_MINUS_SRC_ALPHA) {
         const GLchan (*src)[4] = argA[j];
         GLchan (*dst)[4] = ccolor[j];
         argA[j] = (const GLchan (*)[4]) ccolor[j];
         for (i = 0; i < n; i++) {
            dst[i][ACOMP] = CHAN_MAX - src[i][ACOMP];
         }
      }
   }

   /*
    * Do the texture combine.
    */
   switch (textureUnit->_CurrentCombine->ModeRGB) {
      case GL_REPLACE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            if (RGBshift) {
               for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
                  rgba[i][RCOMP] = arg0[i][RCOMP] * RGBmult;
                  rgba[i][GCOMP] = arg0[i][GCOMP] * RGBmult;
                  rgba[i][BCOMP] = arg0[i][BCOMP] * RGBmult;
#else
                  GLuint r = (GLuint) arg0[i][RCOMP] << RGBshift;
                  GLuint g = (GLuint) arg0[i][GCOMP] << RGBshift;
                  GLuint b = (GLuint) arg0[i][BCOMP] << RGBshift;
                  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
                  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
                  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
#endif
               }
            }
            else {
               for (i = 0; i < n; i++) {
                  rgba[i][RCOMP] = arg0[i][RCOMP];
                  rgba[i][GCOMP] = arg0[i][GCOMP];
                  rgba[i][BCOMP] = arg0[i][BCOMP];
               }
            }
         }
         break;
      case GL_MODULATE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - RGBshift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = arg0[i][RCOMP] * arg1[i][RCOMP] * RGBmult;
               rgba[i][GCOMP] = arg0[i][GCOMP] * arg1[i][GCOMP] * RGBmult;
               rgba[i][BCOMP] = arg0[i][BCOMP] * arg1[i][BCOMP] * RGBmult;
#else
               GLuint r = PROD(arg0[i][RCOMP], arg1[i][RCOMP]) >> shift;
               GLuint g = PROD(arg0[i][GCOMP], arg1[i][GCOMP]) >> shift;
               GLuint b = PROD(arg0[i][BCOMP], arg1[i][BCOMP]) >> shift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_ADD:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = (arg0[i][RCOMP] + arg1[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] + arg1[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] + arg1[i][BCOMP]) * RGBmult;
#else
               GLint r = ((GLint) arg0[i][RCOMP] + (GLint) arg1[i][RCOMP]) << RGBshift;
               GLint g = ((GLint) arg0[i][GCOMP] + (GLint) arg1[i][GCOMP]) << RGBshift;
               GLint b = ((GLint) arg0[i][BCOMP] + (GLint) arg1[i][BCOMP]) << RGBshift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_ADD_SIGNED:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = (arg0[i][RCOMP] + arg1[i][RCOMP] - 0.5) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] + arg1[i][GCOMP] - 0.5) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] + arg1[i][BCOMP] - 0.5) * RGBmult;
#else
               GLint r = (GLint) arg0[i][RCOMP] + (GLint) arg1[i][RCOMP] -half;
               GLint g = (GLint) arg0[i][GCOMP] + (GLint) arg1[i][GCOMP] -half;
               GLint b = (GLint) arg0[i][BCOMP] + (GLint) arg1[i][BCOMP] -half;
               r = (r < 0) ? 0 : r << RGBshift;
               g = (g < 0) ? 0 : g << RGBshift;
               b = (b < 0) ? 0 : b << RGBshift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_INTERPOLATE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argRGB[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - RGBshift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = (arg0[i][RCOMP] * arg2[i][RCOMP] +
                      arg1[i][RCOMP] * (CHAN_MAXF - arg2[i][RCOMP])) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] * arg2[i][GCOMP] +
                      arg1[i][GCOMP] * (CHAN_MAXF - arg2[i][GCOMP])) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] * arg2[i][BCOMP] +
                      arg1[i][BCOMP] * (CHAN_MAXF - arg2[i][BCOMP])) * RGBmult;
#else
               GLuint r = (PROD(arg0[i][RCOMP], arg2[i][RCOMP])
                           + PROD(arg1[i][RCOMP], CHAN_MAX - arg2[i][RCOMP]))
                              >> shift;
               GLuint g = (PROD(arg0[i][GCOMP], arg2[i][GCOMP])
                           + PROD(arg1[i][GCOMP], CHAN_MAX - arg2[i][GCOMP]))
                              >> shift;
               GLuint b = (PROD(arg0[i][BCOMP], arg2[i][BCOMP])
                           + PROD(arg1[i][BCOMP], CHAN_MAX - arg2[i][BCOMP]))
                              >> shift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_SUBTRACT:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = (arg0[i][RCOMP] - arg1[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] - arg1[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] - arg1[i][BCOMP]) * RGBmult;
#else
               GLint r = ((GLint) arg0[i][RCOMP] - (GLint) arg1[i][RCOMP]) << RGBshift;
               GLint g = ((GLint) arg0[i][GCOMP] - (GLint) arg1[i][GCOMP]) << RGBshift;
               GLint b = ((GLint) arg0[i][BCOMP] - (GLint) arg1[i][BCOMP]) << RGBshift;
               rgba[i][RCOMP] = (GLchan) CLAMP(r, 0, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) CLAMP(g, 0, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) CLAMP(b, 0, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_DOT3_RGB_EXT:
      case GL_DOT3_RGBA_EXT:
         {
            /* Do not scale the result by 1 2 or 4 */
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               GLchan dot = ((arg0[i][RCOMP]-0.5F) * (arg1[i][RCOMP]-0.5F) +
                             (arg0[i][GCOMP]-0.5F) * (arg1[i][GCOMP]-0.5F) +
                             (arg0[i][BCOMP]-0.5F) * (arg1[i][BCOMP]-0.5F))
                            * 4.0F;
               dot = CLAMP(dot, 0.0F, CHAN_MAXF);
#else
               GLint dot = (S_PROD((GLint)arg0[i][RCOMP] - half,
				   (GLint)arg1[i][RCOMP] - half) +
			    S_PROD((GLint)arg0[i][GCOMP] - half,
				   (GLint)arg1[i][GCOMP] - half) +
			    S_PROD((GLint)arg0[i][BCOMP] - half,
				   (GLint)arg1[i][BCOMP] - half)) >> 6;
               dot = CLAMP(dot, 0, CHAN_MAX);
#endif
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = (GLchan) dot;
            }
         }
         break;
      case GL_DOT3_RGB:
      case GL_DOT3_RGBA:
         {
            /* DO scale the result by 1 2 or 4 */
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               GLchan dot = ((arg0[i][RCOMP]-0.5F) * (arg1[i][RCOMP]-0.5F) +
                             (arg0[i][GCOMP]-0.5F) * (arg1[i][GCOMP]-0.5F) +
                             (arg0[i][BCOMP]-0.5F) * (arg1[i][BCOMP]-0.5F))
                            * 4.0F * RGBmult;
               dot = CLAMP(dot, 0.0, CHAN_MAXF);
#else
               GLint dot = (S_PROD((GLint)arg0[i][RCOMP] - half,
				   (GLint)arg1[i][RCOMP] - half) +
			    S_PROD((GLint)arg0[i][GCOMP] - half,
				   (GLint)arg1[i][GCOMP] - half) +
			    S_PROD((GLint)arg0[i][BCOMP] - half,
				   (GLint)arg1[i][BCOMP] - half)) >> 6;
               dot <<= RGBshift;
               dot = CLAMP(dot, 0, CHAN_MAX);
#endif
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = (GLchan) dot;
            }
         }
         break;
      case GL_MODULATE_ADD_ATI:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argRGB[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - RGBshift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = ((arg0[i][RCOMP] * arg2[i][RCOMP]) + arg1[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = ((arg0[i][GCOMP] * arg2[i][GCOMP]) + arg1[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = ((arg0[i][BCOMP] * arg2[i][BCOMP]) + arg1[i][BCOMP]) * RGBmult;
#else
               GLuint r = (PROD(arg0[i][RCOMP], arg2[i][RCOMP])
                           + ((GLuint) arg1[i][RCOMP] << CHAN_BITS)) >> shift;
               GLuint g = (PROD(arg0[i][GCOMP], arg2[i][GCOMP])
                           + ((GLuint) arg1[i][GCOMP] << CHAN_BITS)) >> shift;
               GLuint b = (PROD(arg0[i][BCOMP], arg2[i][BCOMP])
                           + ((GLuint) arg1[i][BCOMP] << CHAN_BITS)) >> shift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
#endif
            }
	 }
         break;
      case GL_MODULATE_SIGNED_ADD_ATI:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argRGB[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - RGBshift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = ((arg0[i][RCOMP] * arg2[i][RCOMP]) + arg1[i][RCOMP] - 0.5) * RGBmult;
               rgba[i][GCOMP] = ((arg0[i][GCOMP] * arg2[i][GCOMP]) + arg1[i][GCOMP] - 0.5) * RGBmult;
               rgba[i][BCOMP] = ((arg0[i][BCOMP] * arg2[i][BCOMP]) + arg1[i][BCOMP] - 0.5) * RGBmult;
#else
               GLint r = (S_PROD(arg0[i][RCOMP], arg2[i][RCOMP])
			  + (((GLint) arg1[i][RCOMP] - half) << CHAN_BITS))
		    >> shift;
               GLint g = (S_PROD(arg0[i][GCOMP], arg2[i][GCOMP])
			  + (((GLint) arg1[i][GCOMP] - half) << CHAN_BITS))
		    >> shift;
               GLint b = (S_PROD(arg0[i][BCOMP], arg2[i][BCOMP])
			  + (((GLint) arg1[i][BCOMP] - half) << CHAN_BITS))
		    >> shift;
               rgba[i][RCOMP] = (GLchan) CLAMP(r, 0, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) CLAMP(g, 0, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) CLAMP(b, 0, CHAN_MAX);
#endif
            }
	 }
         break;
      case GL_MODULATE_SUBTRACT_ATI:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argRGB[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - RGBshift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = ((arg0[i][RCOMP] * arg2[i][RCOMP]) - arg1[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = ((arg0[i][GCOMP] * arg2[i][GCOMP]) - arg1[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = ((arg0[i][BCOMP] * arg2[i][BCOMP]) - arg1[i][BCOMP]) * RGBmult;
#else
               GLint r = (S_PROD(arg0[i][RCOMP], arg2[i][RCOMP])
			  - ((GLint) arg1[i][RCOMP] << CHAN_BITS))
		    >> shift;
               GLint g = (S_PROD(arg0[i][GCOMP], arg2[i][GCOMP])
			  - ((GLint) arg1[i][GCOMP] << CHAN_BITS))
		    >> shift;
               GLint b = (S_PROD(arg0[i][BCOMP], arg2[i][BCOMP])
			  - ((GLint) arg1[i][BCOMP] << CHAN_BITS))
		    >> shift;
               rgba[i][RCOMP] = (GLchan) CLAMP(r, 0, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) CLAMP(g, 0, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) CLAMP(b, 0, CHAN_MAX);
#endif
            }
	 }
         break;
      default:
         _mesa_problem(ctx, "invalid combine mode");
   }

   switch (textureUnit->_CurrentCombine->ModeA) {
      case GL_REPLACE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            if (Ashift) {
               for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
                  GLchan a = arg0[i][ACOMP] * Amult;
#else
                  GLuint a = (GLuint) arg0[i][ACOMP] << Ashift;
#endif
                  rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
               }
            }
            else {
               for (i = 0; i < n; i++) {
                  rgba[i][ACOMP] = arg0[i][ACOMP];
               }
            }
         }
         break;
      case GL_MODULATE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - Ashift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = arg0[i][ACOMP] * arg1[i][ACOMP] * Amult;
#else
               GLuint a = (PROD(arg0[i][ACOMP], arg1[i][ACOMP]) >> shift);
               rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_ADD:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan  (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = (arg0[i][ACOMP] + arg1[i][ACOMP]) * Amult;
#else
               GLint a = ((GLint) arg0[i][ACOMP] + arg1[i][ACOMP]) << Ashift;
               rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_ADD_SIGNED:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = (arg0[i][ACOMP] + arg1[i][ACOMP] - 0.5F) * Amult;
#else
               GLint a = (GLint) arg0[i][ACOMP] + (GLint) arg1[i][ACOMP] -half;
               a = (a < 0) ? 0 : a << Ashift;
               rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_INTERPOLATE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argA[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - Ashift;
#endif
            for (i=0; i<n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = (arg0[i][ACOMP] * arg2[i][ACOMP] +
                                 arg1[i][ACOMP] * (CHAN_MAXF - arg2[i][ACOMP]))
                                * Amult;
#else
               GLuint a = (PROD(arg0[i][ACOMP], arg2[i][ACOMP])
                           + PROD(arg1[i][ACOMP], CHAN_MAX - arg2[i][ACOMP]))
                              >> shift;
               rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_SUBTRACT:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = (arg0[i][ACOMP] - arg1[i][ACOMP]) * Amult;
#else
               GLint a = ((GLint) arg0[i][ACOMP] - (GLint) arg1[i][ACOMP]) << Ashift;
               rgba[i][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_MODULATE_ADD_ATI:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argA[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - Ashift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = ((arg0[i][ACOMP] * arg2[i][ACOMP]) + arg1[i][ACOMP]) * Amult;
#else
               GLint a = (PROD(arg0[i][ACOMP], arg2[i][ACOMP])
			   + ((GLuint) arg1[i][ACOMP] << CHAN_BITS))
		    >> shift;
               rgba[i][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_MODULATE_SIGNED_ADD_ATI:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argA[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - Ashift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = ((arg0[i][ACOMP] * arg2[i][ACOMP]) + arg1[i][ACOMP] - 0.5F) * Amult;
#else
               GLint a = (S_PROD(arg0[i][ACOMP], arg2[i][ACOMP])
			  + (((GLint) arg1[i][ACOMP] - half) << CHAN_BITS))
		    >> shift;
               rgba[i][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_MODULATE_SUBTRACT_ATI:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argA[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - Ashift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = ((arg0[i][ACOMP] * arg2[i][ACOMP]) - arg1[i][ACOMP]) * Amult;
#else
               GLint a = (S_PROD(arg0[i][ACOMP], arg2[i][ACOMP])
			  - ((GLint) arg1[i][ACOMP] << CHAN_BITS))
		    >> shift;
               rgba[i][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
#endif
            }
         }
         break;
      default:
         _mesa_problem(ctx, "invalid combine mode");
   }

   /* Fix the alpha component for GL_DOT3_RGBA_EXT/ARB combining.
    * This is kind of a kludge.  It would have been better if the spec
    * were written such that the GL_COMBINE_ALPHA value could be set to
    * GL_DOT3.
    */
   if (textureUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA_EXT ||
       textureUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA) {
      for (i = 0; i < n; i++) {
	 rgba[i][ACOMP] = rgba[i][RCOMP];
      }
   }
   UNDEFARRAY(ccolor);  /* mac 32k limitation */
}
#undef PROD


/**
 * Apply a conventional OpenGL texture env mode (REPLACE, ADD, BLEND,
 * MODULATE, or DECAL) to an array of fragments.
 * Input:  textureUnit - pointer to texture unit to apply
 *         format - base internal texture format
 *         n - number of fragments
 *         primary_rgba - primary colors (may alias rgba for single texture)
 *         texels - array of texel colors
 * InOut:  rgba - incoming fragment colors modified by texel colors
 *                according to the texture environment mode.
 */
static void
texture_apply( const GLcontext *ctx,
               const struct gl_texture_unit *texUnit,
               GLuint n,
               CONST GLchan primary_rgba[][4], CONST GLchan texel[][4],
               GLchan rgba[][4] )
{
   GLint baseLevel;
   GLuint i;
   GLint Rc, Gc, Bc, Ac;
   GLenum format;
   (void) primary_rgba;

   ASSERT(texUnit);
   ASSERT(texUnit->_Current);

   baseLevel = texUnit->_Current->BaseLevel;
   ASSERT(texUnit->_Current->Image[0][baseLevel]);

   format = texUnit->_Current->Image[0][baseLevel]->Format;

   if (format == GL_COLOR_INDEX || format == GL_YCBCR_MESA) {
      format = GL_RGBA;  /* a bit of a hack */
   }
   else if (format == GL_DEPTH_COMPONENT) {
      format = texUnit->_Current->DepthMode;
   }

   switch (texUnit->EnvMode) {
      case GL_REPLACE:
	 switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
                  /* Av = At */
                  rgba[i][ACOMP] = texel[i][ACOMP];
	       }
	       break;
	    case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = Lt */
                  GLchan Lt = texel[i][RCOMP];
                  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = Lt;
                  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
                  GLchan Lt = texel[i][RCOMP];
		  /* Cv = Lt */
		  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = Lt;
		  /* Av = At */
		  rgba[i][ACOMP] = texel[i][ACOMP];
	       }
	       break;
	    case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = It */
                  GLchan It = texel[i][RCOMP];
                  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = It;
                  /* Av = It */
                  rgba[i][ACOMP] = It;
	       }
	       break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  rgba[i][RCOMP] = texel[i][RCOMP];
		  rgba[i][GCOMP] = texel[i][GCOMP];
		  rgba[i][BCOMP] = texel[i][BCOMP];
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  rgba[i][RCOMP] = texel[i][RCOMP];
		  rgba[i][GCOMP] = texel[i][GCOMP];
		  rgba[i][BCOMP] = texel[i][BCOMP];
		  /* Av = At */
		  rgba[i][ACOMP] = texel[i][ACOMP];
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_REPLACE) in texture_apply");
               return;
	 }
	 break;

      case GL_MODULATE:
         switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT( rgba[i][ACOMP], texel[i][ACOMP] );
	       }
	       break;
	    case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = LtCf */
                  GLchan Lt = texel[i][RCOMP];
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], Lt );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], Lt );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], Lt );
		  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = CfLt */
                  GLchan Lt = texel[i][RCOMP];
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], Lt );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], Lt );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], Lt );
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT( rgba[i][ACOMP], texel[i][ACOMP] );
	       }
	       break;
	    case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = CfIt */
                  GLchan It = texel[i][RCOMP];
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], It );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], It );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], It );
		  /* Av = AfIt */
		  rgba[i][ACOMP] = CHAN_PRODUCT( rgba[i][ACOMP], It );
	       }
	       break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = CfCt */
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], texel[i][RCOMP] );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], texel[i][GCOMP] );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], texel[i][BCOMP] );
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = CfCt */
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], texel[i][RCOMP] );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], texel[i][GCOMP] );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], texel[i][BCOMP] );
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT( rgba[i][ACOMP], texel[i][ACOMP] );
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_MODULATE) in texture_apply");
               return;
	 }
	 break;

      case GL_DECAL:
         switch (format) {
            case GL_ALPHA:
            case GL_LUMINANCE:
            case GL_LUMINANCE_ALPHA:
            case GL_INTENSITY:
               /* undefined */
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  rgba[i][RCOMP] = texel[i][RCOMP];
		  rgba[i][GCOMP] = texel[i][GCOMP];
		  rgba[i][BCOMP] = texel[i][BCOMP];
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-At) + CtAt */
		  GLint t = texel[i][ACOMP], s = CHAN_MAX - t;
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], s) + CHAN_PRODUCT(texel[i][RCOMP],t);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], s) + CHAN_PRODUCT(texel[i][GCOMP],t);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], s) + CHAN_PRODUCT(texel[i][BCOMP],t);
		  /* Av = Af */
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_DECAL) in texture_apply");
               return;
	 }
	 break;

      case GL_BLEND:
         Rc = (GLint) (texUnit->EnvColor[0] * CHAN_MAXF);
         Gc = (GLint) (texUnit->EnvColor[1] * CHAN_MAXF);
         Bc = (GLint) (texUnit->EnvColor[2] * CHAN_MAXF);
         Ac = (GLint) (texUnit->EnvColor[3] * CHAN_MAXF);
	 switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
		  /* Av = AfAt */
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], texel[i][ACOMP]);
	       }
	       break;
            case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Lt) + CcLt */
		  GLchan Lt = texel[i][RCOMP], s = CHAN_MAX - Lt;
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], s) + CHAN_PRODUCT(Rc, Lt);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], s) + CHAN_PRODUCT(Gc, Lt);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], s) + CHAN_PRODUCT(Bc, Lt);
		  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Lt) + CcLt */
		  GLchan Lt = texel[i][RCOMP], s = CHAN_MAX - Lt;
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], s) + CHAN_PRODUCT(Rc, Lt);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], s) + CHAN_PRODUCT(Gc, Lt);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], s) + CHAN_PRODUCT(Bc, Lt);
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP],texel[i][ACOMP]);
	       }
	       break;
            case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-It) + CcIt */
		  GLchan It = texel[i][RCOMP], s = CHAN_MAX - It;
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], s) + CHAN_PRODUCT(Rc, It);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], s) + CHAN_PRODUCT(Gc, It);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], s) + CHAN_PRODUCT(Bc, It);
                  /* Av = Af(1-It) + Ac*It */
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], s) + CHAN_PRODUCT(Ac, It);
               }
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Ct) + CcCt */
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], (CHAN_MAX-texel[i][RCOMP])) + CHAN_PRODUCT(Rc,texel[i][RCOMP]);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], (CHAN_MAX-texel[i][GCOMP])) + CHAN_PRODUCT(Gc,texel[i][GCOMP]);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], (CHAN_MAX-texel[i][BCOMP])) + CHAN_PRODUCT(Bc,texel[i][BCOMP]);
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Ct) + CcCt */
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], (CHAN_MAX-texel[i][RCOMP])) + CHAN_PRODUCT(Rc,texel[i][RCOMP]);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], (CHAN_MAX-texel[i][GCOMP])) + CHAN_PRODUCT(Gc,texel[i][GCOMP]);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], (CHAN_MAX-texel[i][BCOMP])) + CHAN_PRODUCT(Bc,texel[i][BCOMP]);
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP],texel[i][ACOMP]);
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_BLEND) in texture_apply");
               return;
	 }
	 break;

     /* XXX don't clamp results if GLchan is float??? */

      case GL_ADD:  /* GL_EXT_texture_add_env */
         switch (format) {
            case GL_ALPHA:
               for (i=0;i<n;i++) {
                  /* Rv = Rf */
                  /* Gv = Gf */
                  /* Bv = Bf */
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], texel[i][ACOMP]);
               }
               break;
            case GL_LUMINANCE:
               for (i=0;i<n;i++) {
                  GLuint Lt = texel[i][RCOMP];
                  GLuint r = rgba[i][RCOMP] + Lt;
                  GLuint g = rgba[i][GCOMP] + Lt;
                  GLuint b = rgba[i][BCOMP] + Lt;
                  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
                  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
                  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
                  /* Av = Af */
               }
               break;
            case GL_LUMINANCE_ALPHA:
               for (i=0;i<n;i++) {
                  GLuint Lt = texel[i][RCOMP];
                  GLuint r = rgba[i][RCOMP] + Lt;
                  GLuint g = rgba[i][GCOMP] + Lt;
                  GLuint b = rgba[i][BCOMP] + Lt;
                  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
                  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
                  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], texel[i][ACOMP]);
               }
               break;
            case GL_INTENSITY:
               for (i=0;i<n;i++) {
                  GLchan It = texel[i][RCOMP];
                  GLuint r = rgba[i][RCOMP] + It;
                  GLuint g = rgba[i][GCOMP] + It;
                  GLuint b = rgba[i][BCOMP] + It;
                  GLuint a = rgba[i][ACOMP] + It;
                  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
                  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
                  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
                  rgba[i][ACOMP] = MIN2(a, CHAN_MAX);
               }
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
                  GLuint r = rgba[i][RCOMP] + texel[i][RCOMP];
                  GLuint g = rgba[i][GCOMP] + texel[i][GCOMP];
                  GLuint b = rgba[i][BCOMP] + texel[i][BCOMP];
		  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
		  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
		  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
                  GLuint r = rgba[i][RCOMP] + texel[i][RCOMP];
                  GLuint g = rgba[i][GCOMP] + texel[i][GCOMP];
                  GLuint b = rgba[i][BCOMP] + texel[i][BCOMP];
		  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
		  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
		  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], texel[i][ACOMP]);
               }
               break;
            default:
               _mesa_problem(ctx, "Bad format (GL_ADD) in texture_apply");
               return;
	 }
	 break;

      default:
         _mesa_problem(ctx, "Bad env mode in texture_apply");
         return;
   }
}



/**
 * Apply texture mapping to a span of fragments.
 */
void
_swrast_texture_span( GLcontext *ctx, struct sw_span *span )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLchan primary_rgba[MAX_WIDTH][4];
   GLuint unit;

   ASSERT(span->end < MAX_WIDTH);
   ASSERT(span->arrayMask & SPAN_TEXTURE);

   /*
    * Save copy of the incoming fragment colors (the GL_PRIMARY_COLOR)
    */
   if (swrast->_AnyTextureCombine)
      MEMCPY(primary_rgba, span->array->rgba, 4 * span->end * sizeof(GLchan));

   /*
    * Must do all texture sampling before combining in order to
    * accomodate GL_ARB_texture_env_crossbar.
    */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (ctx->Texture.Unit[unit]._ReallyEnabled) {
         const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
         const struct gl_texture_object *curObj = texUnit->_Current;
         GLfloat *lambda = span->array->lambda[unit];
         GLchan (*texels)[4] = (GLchan (*)[4])
            (swrast->TexelBuffer + unit * (span->end * 4 * sizeof(GLchan)));

         /* adjust texture lod (lambda) */
         if (span->arrayMask & SPAN_LAMBDA) {
            if (texUnit->LodBias + curObj->LodBias != 0.0F) {
               /* apply LOD bias, but don't clamp yet */
               const GLfloat bias = CLAMP(texUnit->LodBias + curObj->LodBias,
                                          -ctx->Const.MaxTextureLodBias,
                                          ctx->Const.MaxTextureLodBias);
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  lambda[i] += bias;
               }
            }

            if (curObj->MinLod != -1000.0 || curObj->MaxLod != 1000.0) {
               /* apply LOD clamping to lambda */
               const GLfloat min = curObj->MinLod;
               const GLfloat max = curObj->MaxLod;
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  GLfloat l = lambda[i];
                  lambda[i] = CLAMP(l, min, max);
               }
            }
         }

         /* Sample the texture (span->end fragments) */
         swrast->TextureSample[unit]( ctx, unit, texUnit->_Current, span->end,
                         (const GLfloat (*)[4]) span->array->texcoords[unit],
                         lambda, texels );

         /* GL_SGI_texture_color_table */
         if (texUnit->ColorTableEnabled) {
            _mesa_lookup_rgba_chan(&texUnit->ColorTable, span->end, texels);
         }
      }
   }

   /*
    * OK, now apply the texture (aka texture combine/blend).
    * We modify the span->color.rgba values.
    */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (ctx->Texture.Unit[unit]._ReallyEnabled) {
         const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
         if (texUnit->_CurrentCombine != &texUnit->_EnvMode ) {
            texture_combine( ctx, unit, span->end,
                             (CONST GLchan (*)[4]) primary_rgba,
                             swrast->TexelBuffer,
                             span->array->rgba );
         }
         else {
            /* conventional texture blend */
            const GLchan (*texels)[4] = (const GLchan (*)[4])
               (swrast->TexelBuffer + unit *
                (span->end * 4 * sizeof(GLchan)));
            texture_apply( ctx, texUnit, span->end,
                           (CONST GLchan (*)[4]) primary_rgba, texels,
                           span->array->rgba );
         }
      }
   }
}
