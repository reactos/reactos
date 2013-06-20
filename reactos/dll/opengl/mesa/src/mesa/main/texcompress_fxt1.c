/*
 * Mesa 3-D graphics library
 * Version:  7.1
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


/**
 * \file texcompress_fxt1.c
 * GL_3DFX_texture_compression_FXT1 support.
 */


#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "image.h"
#include "macros.h"
#include "mfeatures.h"
#include "mipmap.h"
#include "texcompress.h"
#include "texcompress_fxt1.h"
#include "texstore.h"
#include "swrast/s_context.h"


#if FEATURE_texture_fxt1


static void
fxt1_encode (GLuint width, GLuint height, GLint comps,
             const void *source, GLint srcRowStride,
             void *dest, GLint destRowStride);

void
fxt1_decode_1 (const void *texture, GLint stride,
               GLint i, GLint j, GLubyte *rgba);


/**
 * Store user's image in rgb_fxt1 format.
 */
GLboolean
_mesa_texstore_rgb_fxt1(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLint srcRowStride;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   ASSERT(dstFormat == MESA_FORMAT_RGB_FXT1);

   if (srcFormat != GL_RGB ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       srcPacking->RowLength != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGB/GLubyte */
      tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                             baseInternalFormat,
                                             _mesa_get_format_base_format(dstFormat),
                                             srcWidth, srcHeight, srcDepth,
                                             srcFormat, srcType, srcAddr,
                                             srcPacking);
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      pixels = tempImage;
      srcRowStride = 3 * srcWidth;
      srcFormat = GL_RGB;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);

      srcRowStride = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat,
                                            srcType) / sizeof(GLubyte);
   }

   dst = dstSlices[0];

   fxt1_encode(srcWidth, srcHeight, 3, pixels, srcRowStride,
               dst, dstRowStride);

   if (tempImage)
      free((void*) tempImage);

   return GL_TRUE;
}


/**
 * Store user's image in rgba_fxt1 format.
 */
GLboolean
_mesa_texstore_rgba_fxt1(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLint srcRowStride;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   ASSERT(dstFormat == MESA_FORMAT_RGBA_FXT1);

   if (srcFormat != GL_RGBA ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       srcPacking->SwapBytes) {
      /* convert image to RGBA/GLubyte */
      tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                             baseInternalFormat,
                                             _mesa_get_format_base_format(dstFormat),
                                             srcWidth, srcHeight, srcDepth,
                                             srcFormat, srcType, srcAddr,
                                             srcPacking);
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      pixels = tempImage;
      srcRowStride = 4 * srcWidth;
      srcFormat = GL_RGBA;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);

      srcRowStride = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat,
                                            srcType) / sizeof(GLubyte);
   }

   dst = dstSlices[0];

   fxt1_encode(srcWidth, srcHeight, 4, pixels, srcRowStride,
               dst, dstRowStride);

   if (tempImage)
      free((void*) tempImage);

   return GL_TRUE;
}


void
_mesa_fetch_texel_2d_f_rgba_fxt1( const struct swrast_texture_image *texImage,
                                  GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* just sample as GLubyte and convert to float here */
   GLubyte rgba[4];
   (void) k;
   fxt1_decode_1(texImage->Map, texImage->RowStride, i, j, rgba);
   texel[RCOMP] = UBYTE_TO_FLOAT(rgba[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(rgba[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(rgba[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(rgba[ACOMP]);
}


void
_mesa_fetch_texel_2d_f_rgb_fxt1( const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* just sample as GLubyte and convert to float here */
   GLubyte rgba[4];
   (void) k;
   fxt1_decode_1(texImage->Map, texImage->RowStride, i, j, rgba);
   texel[RCOMP] = UBYTE_TO_FLOAT(rgba[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(rgba[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(rgba[BCOMP]);
   texel[ACOMP] = 1.0F;
}



/***************************************************************************\
 * FXT1 encoder
 *
 * The encoder was built by reversing the decoder,
 * and is vaguely based on Texus2 by 3dfx. Note that this code
 * is merely a proof of concept, since it is highly UNoptimized;
 * moreover, it is sub-optimal due to initial conditions passed
 * to Lloyd's algorithm (the interpolation modes are even worse).
\***************************************************************************/


#define MAX_COMP 4 /* ever needed maximum number of components in texel */
#define MAX_VECT 4 /* ever needed maximum number of base vectors to find */
#define N_TEXELS 32 /* number of texels in a block (always 32) */
#define LL_N_REP 50 /* number of iterations in lloyd's vq */
#define LL_RMS_D 10 /* fault tolerance (maximum delta) */
#define LL_RMS_E 255 /* fault tolerance (maximum error) */
#define ALPHA_TS 2 /* alpha threshold: (255 - ALPHA_TS) deemed opaque */
#define ISTBLACK(v) (*((GLuint *)(v)) == 0)


/*
 * Define a 64-bit unsigned integer type and macros
 */
#if 1

#define FX64_NATIVE 1

typedef uint64_t Fx64;

#define FX64_MOV32(a, b) a = b
#define FX64_OR32(a, b)  a |= b
#define FX64_SHL(a, c)   a <<= c

#else

#define FX64_NATIVE 0

typedef struct {
   GLuint lo, hi;
} Fx64;

#define FX64_MOV32(a, b) a.lo = b
#define FX64_OR32(a, b)  a.lo |= b

#define FX64_SHL(a, c)                                 \
   do {                                                \
       if ((c) >= 32) {                                \
          a.hi = a.lo << ((c) - 32);                   \
          a.lo = 0;                                    \
       } else {                                        \
          a.hi = (a.hi << (c)) | (a.lo >> (32 - (c))); \
          a.lo <<= (c);                                \
       }                                               \
   } while (0)

#endif


#define F(i) (GLfloat)1 /* can be used to obtain an oblong metric: 0.30 / 0.59 / 0.11 */
#define SAFECDOT 1 /* for paranoids */

#define MAKEIVEC(NV, NC, IV, B, V0, V1)  \
   do {                                  \
      /* compute interpolation vector */ \
      GLfloat d2 = 0.0F;                 \
      GLfloat rd2;                       \
                                         \
      for (i = 0; i < NC; i++) {         \
         IV[i] = (V1[i] - V0[i]) * F(i); \
         d2 += IV[i] * IV[i];            \
      }                                  \
      rd2 = (GLfloat)NV / d2;            \
      B = 0;                             \
      for (i = 0; i < NC; i++) {         \
         IV[i] *= F(i);                  \
         B -= IV[i] * V0[i];             \
         IV[i] *= rd2;                   \
      }                                  \
      B = B * rd2 + 0.5f;                \
   } while (0)

#define CALCCDOT(TEXEL, NV, NC, IV, B, V)\
   do {                                  \
      GLfloat dot = 0.0F;                \
      for (i = 0; i < NC; i++) {         \
         dot += V[i] * IV[i];            \
      }                                  \
      TEXEL = (GLint)(dot + B);          \
      if (SAFECDOT) {                    \
         if (TEXEL < 0) {                \
            TEXEL = 0;                   \
         } else if (TEXEL > NV) {        \
            TEXEL = NV;                  \
         }                               \
      }                                  \
   } while (0)


static GLint
fxt1_bestcol (GLfloat vec[][MAX_COMP], GLint nv,
              GLubyte input[MAX_COMP], GLint nc)
{
   GLint i, j, best = -1;
   GLfloat err = 1e9; /* big enough */

   for (j = 0; j < nv; j++) {
      GLfloat e = 0.0F;
      for (i = 0; i < nc; i++) {
         e += (vec[j][i] - input[i]) * (vec[j][i] - input[i]);
      }
      if (e < err) {
         err = e;
         best = j;
      }
   }

   return best;
}


static GLint
fxt1_worst (GLfloat vec[MAX_COMP],
            GLubyte input[N_TEXELS][MAX_COMP], GLint nc, GLint n)
{
   GLint i, k, worst = -1;
   GLfloat err = -1.0F; /* small enough */

   for (k = 0; k < n; k++) {
      GLfloat e = 0.0F;
      for (i = 0; i < nc; i++) {
         e += (vec[i] - input[k][i]) * (vec[i] - input[k][i]);
      }
      if (e > err) {
         err = e;
         worst = k;
      }
   }

   return worst;
}


static GLint
fxt1_variance (GLdouble variance[MAX_COMP],
               GLubyte input[N_TEXELS][MAX_COMP], GLint nc, GLint n)
{
   GLint i, k, best = 0;
   GLint sx, sx2;
   GLdouble var, maxvar = -1; /* small enough */
   GLdouble teenth = 1.0 / n;

   for (i = 0; i < nc; i++) {
      sx = sx2 = 0;
      for (k = 0; k < n; k++) {
         GLint t = input[k][i];
         sx += t;
         sx2 += t * t;
      }
      var = sx2 * teenth - sx * sx * teenth * teenth;
      if (maxvar < var) {
         maxvar = var;
         best = i;
      }
      if (variance) {
         variance[i] = var;
      }
   }

   return best;
}


static GLint
fxt1_choose (GLfloat vec[][MAX_COMP], GLint nv,
             GLubyte input[N_TEXELS][MAX_COMP], GLint nc, GLint n)
{
#if 0
   /* Choose colors from a grid.
    */
   GLint i, j;

   for (j = 0; j < nv; j++) {
      GLint m = j * (n - 1) / (nv - 1);
      for (i = 0; i < nc; i++) {
         vec[j][i] = input[m][i];
      }
   }
#else
   /* Our solution here is to find the darkest and brightest colors in
    * the 8x4 tile and use those as the two representative colors.
    * There are probably better algorithms to use (histogram-based).
    */
   GLint i, j, k;
   GLint minSum = 2000; /* big enough */
   GLint maxSum = -1; /* small enough */
   GLint minCol = 0; /* phoudoin: silent compiler! */
   GLint maxCol = 0; /* phoudoin: silent compiler! */

   struct {
      GLint flag;
      GLint key;
      GLint freq;
      GLint idx;
   } hist[N_TEXELS];
   GLint lenh = 0;

   memset(hist, 0, sizeof(hist));

   for (k = 0; k < n; k++) {
      GLint l;
      GLint key = 0;
      GLint sum = 0;
      for (i = 0; i < nc; i++) {
         key <<= 8;
         key |= input[k][i];
         sum += input[k][i];
      }
      for (l = 0; l < n; l++) {
         if (!hist[l].flag) {
            /* alloc new slot */
            hist[l].flag = !0;
            hist[l].key = key;
            hist[l].freq = 1;
            hist[l].idx = k;
            lenh = l + 1;
            break;
         } else if (hist[l].key == key) {
            hist[l].freq++;
            break;
         }
      }
      if (minSum > sum) {
         minSum = sum;
         minCol = k;
      }
      if (maxSum < sum) {
         maxSum = sum;
         maxCol = k;
      }
   }

   if (lenh <= nv) {
      for (j = 0; j < lenh; j++) {
         for (i = 0; i < nc; i++) {
            vec[j][i] = (GLfloat)input[hist[j].idx][i];
         }
      }
      for (; j < nv; j++) {
         for (i = 0; i < nc; i++) {
            vec[j][i] = vec[0][i];
         }
      }
      return 0;
   }

   for (j = 0; j < nv; j++) {
      for (i = 0; i < nc; i++) {
         vec[j][i] = ((nv - 1 - j) * input[minCol][i] + j * input[maxCol][i] + (nv - 1) / 2) / (GLfloat)(nv - 1);
      }
   }
#endif

   return !0;
}


static GLint
fxt1_lloyd (GLfloat vec[][MAX_COMP], GLint nv,
            GLubyte input[N_TEXELS][MAX_COMP], GLint nc, GLint n)
{
   /* Use the generalized lloyd's algorithm for VQ:
    *     find 4 color vectors.
    *
    *     for each sample color
    *         sort to nearest vector.
    *
    *     replace each vector with the centroid of its matching colors.
    *
    *     repeat until RMS doesn't improve.
    *
    *     if a color vector has no samples, or becomes the same as another
    *     vector, replace it with the color which is farthest from a sample.
    *
    * vec[][MAX_COMP]           initial vectors and resulting colors
    * nv                        number of resulting colors required
    * input[N_TEXELS][MAX_COMP] input texels
    * nc                        number of components in input / vec
    * n                         number of input samples
    */

   GLint sum[MAX_VECT][MAX_COMP]; /* used to accumulate closest texels */
   GLint cnt[MAX_VECT]; /* how many times a certain vector was chosen */
   GLfloat error, lasterror = 1e9;

   GLint i, j, k, rep;

   /* the quantizer */
   for (rep = 0; rep < LL_N_REP; rep++) {
      /* reset sums & counters */
      for (j = 0; j < nv; j++) {
         for (i = 0; i < nc; i++) {
            sum[j][i] = 0;
         }
         cnt[j] = 0;
      }
      error = 0;

      /* scan whole block */
      for (k = 0; k < n; k++) {
#if 1
         GLint best = -1;
         GLfloat err = 1e9; /* big enough */
         /* determine best vector */
         for (j = 0; j < nv; j++) {
            GLfloat e = (vec[j][0] - input[k][0]) * (vec[j][0] - input[k][0]) +
                      (vec[j][1] - input[k][1]) * (vec[j][1] - input[k][1]) +
                      (vec[j][2] - input[k][2]) * (vec[j][2] - input[k][2]);
            if (nc == 4) {
               e += (vec[j][3] - input[k][3]) * (vec[j][3] - input[k][3]);
            }
            if (e < err) {
               err = e;
               best = j;
            }
         }
#else
         GLint best = fxt1_bestcol(vec, nv, input[k], nc, &err);
#endif
         assert(best >= 0);
         /* add in closest color */
         for (i = 0; i < nc; i++) {
            sum[best][i] += input[k][i];
         }
         /* mark this vector as used */
         cnt[best]++;
         /* accumulate error */
         error += err;
      }

      /* check RMS */
      if ((error < LL_RMS_E) ||
          ((error < lasterror) && ((lasterror - error) < LL_RMS_D))) {
         return !0; /* good match */
      }
      lasterror = error;

      /* move each vector to the barycenter of its closest colors */
      for (j = 0; j < nv; j++) {
         if (cnt[j]) {
            GLfloat div = 1.0F / cnt[j];
            for (i = 0; i < nc; i++) {
               vec[j][i] = div * sum[j][i];
            }
         } else {
            /* this vec has no samples or is identical with a previous vec */
            GLint worst = fxt1_worst(vec[j], input, nc, n);
            for (i = 0; i < nc; i++) {
               vec[j][i] = input[worst][i];
            }
         }
      }
   }

   return 0; /* could not converge fast enough */
}


static void
fxt1_quantize_CHROMA (GLuint *cc,
                      GLubyte input[N_TEXELS][MAX_COMP])
{
   const GLint n_vect = 4; /* 4 base vectors to find */
   const GLint n_comp = 3; /* 3 components: R, G, B */
   GLfloat vec[MAX_VECT][MAX_COMP];
   GLint i, j, k;
   Fx64 hi; /* high quadword */
   GLuint lohi, lolo; /* low quadword: hi dword, lo dword */

   if (fxt1_choose(vec, n_vect, input, n_comp, N_TEXELS) != 0) {
      fxt1_lloyd(vec, n_vect, input, n_comp, N_TEXELS);
   }

   FX64_MOV32(hi, 4); /* cc-chroma = "010" + unused bit */
   for (j = n_vect - 1; j >= 0; j--) {
      for (i = 0; i < n_comp; i++) {
         /* add in colors */
         FX64_SHL(hi, 5);
         FX64_OR32(hi, (GLuint)(vec[j][i] / 8.0F));
      }
   }
   ((Fx64 *)cc)[1] = hi;

   lohi = lolo = 0;
   /* right microtile */
   for (k = N_TEXELS - 1; k >= N_TEXELS/2; k--) {
      lohi <<= 2;
      lohi |= fxt1_bestcol(vec, n_vect, input[k], n_comp);
   }
   /* left microtile */
   for (; k >= 0; k--) {
      lolo <<= 2;
      lolo |= fxt1_bestcol(vec, n_vect, input[k], n_comp);
   }
   cc[1] = lohi;
   cc[0] = lolo;
}


static void
fxt1_quantize_ALPHA0 (GLuint *cc,
                      GLubyte input[N_TEXELS][MAX_COMP],
                      GLubyte reord[N_TEXELS][MAX_COMP], GLint n)
{
   const GLint n_vect = 3; /* 3 base vectors to find */
   const GLint n_comp = 4; /* 4 components: R, G, B, A */
   GLfloat vec[MAX_VECT][MAX_COMP];
   GLint i, j, k;
   Fx64 hi; /* high quadword */
   GLuint lohi, lolo; /* low quadword: hi dword, lo dword */

   /* the last vector indicates zero */
   for (i = 0; i < n_comp; i++) {
      vec[n_vect][i] = 0;
   }

   /* the first n texels in reord are guaranteed to be non-zero */
   if (fxt1_choose(vec, n_vect, reord, n_comp, n) != 0) {
      fxt1_lloyd(vec, n_vect, reord, n_comp, n);
   }

   FX64_MOV32(hi, 6); /* alpha = "011" + lerp = 0 */
   for (j = n_vect - 1; j >= 0; j--) {
      /* add in alphas */
      FX64_SHL(hi, 5);
      FX64_OR32(hi, (GLuint)(vec[j][ACOMP] / 8.0F));
   }
   for (j = n_vect - 1; j >= 0; j--) {
      for (i = 0; i < n_comp - 1; i++) {
         /* add in colors */
         FX64_SHL(hi, 5);
         FX64_OR32(hi, (GLuint)(vec[j][i] / 8.0F));
      }
   }
   ((Fx64 *)cc)[1] = hi;

   lohi = lolo = 0;
   /* right microtile */
   for (k = N_TEXELS - 1; k >= N_TEXELS/2; k--) {
      lohi <<= 2;
      lohi |= fxt1_bestcol(vec, n_vect + 1, input[k], n_comp);
   }
   /* left microtile */
   for (; k >= 0; k--) {
      lolo <<= 2;
      lolo |= fxt1_bestcol(vec, n_vect + 1, input[k], n_comp);
   }
   cc[1] = lohi;
   cc[0] = lolo;
}


static void
fxt1_quantize_ALPHA1 (GLuint *cc,
                      GLubyte input[N_TEXELS][MAX_COMP])
{
   const GLint n_vect = 3; /* highest vector number in each microtile */
   const GLint n_comp = 4; /* 4 components: R, G, B, A */
   GLfloat vec[1 + 1 + 1][MAX_COMP]; /* 1.5 extrema for each sub-block */
   GLfloat b, iv[MAX_COMP]; /* interpolation vector */
   GLint i, j, k;
   Fx64 hi; /* high quadword */
   GLuint lohi, lolo; /* low quadword: hi dword, lo dword */

   GLint minSum;
   GLint maxSum;
   GLint minColL = 0, maxColL = 0;
   GLint minColR = 0, maxColR = 0;
   GLint sumL = 0, sumR = 0;
   GLint nn_comp;
   /* Our solution here is to find the darkest and brightest colors in
    * the 4x4 tile and use those as the two representative colors.
    * There are probably better algorithms to use (histogram-based).
    */
   nn_comp = n_comp;
   while ((minColL == maxColL) && nn_comp) {
       minSum = 2000; /* big enough */
       maxSum = -1; /* small enough */
       for (k = 0; k < N_TEXELS / 2; k++) {
           GLint sum = 0;
           for (i = 0; i < nn_comp; i++) {
               sum += input[k][i];
           }
           if (minSum > sum) {
               minSum = sum;
               minColL = k;
           }
           if (maxSum < sum) {
               maxSum = sum;
               maxColL = k;
           }
           sumL += sum;
       }
       
       nn_comp--;
   }

   nn_comp = n_comp;
   while ((minColR == maxColR) && nn_comp) {
       minSum = 2000; /* big enough */
       maxSum = -1; /* small enough */
       for (k = N_TEXELS / 2; k < N_TEXELS; k++) {
           GLint sum = 0;
           for (i = 0; i < nn_comp; i++) {
               sum += input[k][i];
           }
           if (minSum > sum) {
               minSum = sum;
               minColR = k;
           }
           if (maxSum < sum) {
               maxSum = sum;
               maxColR = k;
           }
           sumR += sum;
       }

       nn_comp--;
   }

   /* choose the common vector (yuck!) */
   {
      GLint j1, j2;
      GLint v1 = 0, v2 = 0;
      GLfloat err = 1e9; /* big enough */
      GLfloat tv[2 * 2][MAX_COMP]; /* 2 extrema for each sub-block */
      for (i = 0; i < n_comp; i++) {
         tv[0][i] = input[minColL][i];
         tv[1][i] = input[maxColL][i];
         tv[2][i] = input[minColR][i];
         tv[3][i] = input[maxColR][i];
      }
      for (j1 = 0; j1 < 2; j1++) {
         for (j2 = 2; j2 < 4; j2++) {
            GLfloat e = 0.0F;
            for (i = 0; i < n_comp; i++) {
               e += (tv[j1][i] - tv[j2][i]) * (tv[j1][i] - tv[j2][i]);
            }
            if (e < err) {
               err = e;
               v1 = j1;
               v2 = j2;
            }
         }
      }
      for (i = 0; i < n_comp; i++) {
         vec[0][i] = tv[1 - v1][i];
         vec[1][i] = (tv[v1][i] * sumL + tv[v2][i] * sumR) / (sumL + sumR);
         vec[2][i] = tv[5 - v2][i];
      }
   }

   /* left microtile */
   cc[0] = 0;
   if (minColL != maxColL) {
      /* compute interpolation vector */
      MAKEIVEC(n_vect, n_comp, iv, b, vec[0], vec[1]);

      /* add in texels */
      lolo = 0;
      for (k = N_TEXELS / 2 - 1; k >= 0; k--) {
         GLint texel;
         /* interpolate color */
         CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
         /* add in texel */
         lolo <<= 2;
         lolo |= texel;
      }
      
      cc[0] = lolo;
   }

   /* right microtile */
   cc[1] = 0;
   if (minColR != maxColR) {
      /* compute interpolation vector */
      MAKEIVEC(n_vect, n_comp, iv, b, vec[2], vec[1]);

      /* add in texels */
      lohi = 0;
      for (k = N_TEXELS - 1; k >= N_TEXELS / 2; k--) {
         GLint texel;
         /* interpolate color */
         CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
         /* add in texel */
         lohi <<= 2;
         lohi |= texel;
      }

      cc[1] = lohi;
   }

   FX64_MOV32(hi, 7); /* alpha = "011" + lerp = 1 */
   for (j = n_vect - 1; j >= 0; j--) {
      /* add in alphas */
      FX64_SHL(hi, 5);
      FX64_OR32(hi, (GLuint)(vec[j][ACOMP] / 8.0F));
   }
   for (j = n_vect - 1; j >= 0; j--) {
      for (i = 0; i < n_comp - 1; i++) {
         /* add in colors */
         FX64_SHL(hi, 5);
         FX64_OR32(hi, (GLuint)(vec[j][i] / 8.0F));
      }
   }
   ((Fx64 *)cc)[1] = hi;
}


static void
fxt1_quantize_HI (GLuint *cc,
                  GLubyte input[N_TEXELS][MAX_COMP],
                  GLubyte reord[N_TEXELS][MAX_COMP], GLint n)
{
   const GLint n_vect = 6; /* highest vector number */
   const GLint n_comp = 3; /* 3 components: R, G, B */
   GLfloat b = 0.0F;       /* phoudoin: silent compiler! */
   GLfloat iv[MAX_COMP];   /* interpolation vector */
   GLint i, k;
   GLuint hihi; /* high quadword: hi dword */

   GLint minSum = 2000; /* big enough */
   GLint maxSum = -1; /* small enough */
   GLint minCol = 0; /* phoudoin: silent compiler! */
   GLint maxCol = 0; /* phoudoin: silent compiler! */

   /* Our solution here is to find the darkest and brightest colors in
    * the 8x4 tile and use those as the two representative colors.
    * There are probably better algorithms to use (histogram-based).
    */
   for (k = 0; k < n; k++) {
      GLint sum = 0;
      for (i = 0; i < n_comp; i++) {
         sum += reord[k][i];
      }
      if (minSum > sum) {
         minSum = sum;
         minCol = k;
      }
      if (maxSum < sum) {
         maxSum = sum;
         maxCol = k;
      }
   }

   hihi = 0; /* cc-hi = "00" */
   for (i = 0; i < n_comp; i++) {
      /* add in colors */
      hihi <<= 5;
      hihi |= reord[maxCol][i] >> 3;
   }
   for (i = 0; i < n_comp; i++) {
      /* add in colors */
      hihi <<= 5;
      hihi |= reord[minCol][i] >> 3;
   }
   cc[3] = hihi;
   cc[0] = cc[1] = cc[2] = 0;

   /* compute interpolation vector */
   if (minCol != maxCol) {
      MAKEIVEC(n_vect, n_comp, iv, b, reord[minCol], reord[maxCol]);
   }

   /* add in texels */
   for (k = N_TEXELS - 1; k >= 0; k--) {
      GLint t = k * 3;
      GLuint *kk = (GLuint *)((char *)cc + t / 8);
      GLint texel = n_vect + 1; /* transparent black */

      if (!ISTBLACK(input[k])) {
         if (minCol != maxCol) {
            /* interpolate color */
            CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
            /* add in texel */
            kk[0] |= texel << (t & 7);
         }
      } else {
         /* add in texel */
         kk[0] |= texel << (t & 7);
      }
   }
}


static void
fxt1_quantize_MIXED1 (GLuint *cc,
                      GLubyte input[N_TEXELS][MAX_COMP])
{
   const GLint n_vect = 2; /* highest vector number in each microtile */
   const GLint n_comp = 3; /* 3 components: R, G, B */
   GLubyte vec[2 * 2][MAX_COMP]; /* 2 extrema for each sub-block */
   GLfloat b, iv[MAX_COMP]; /* interpolation vector */
   GLint i, j, k;
   Fx64 hi; /* high quadword */
   GLuint lohi, lolo; /* low quadword: hi dword, lo dword */

   GLint minSum;
   GLint maxSum;
   GLint minColL = 0, maxColL = -1;
   GLint minColR = 0, maxColR = -1;

   /* Our solution here is to find the darkest and brightest colors in
    * the 4x4 tile and use those as the two representative colors.
    * There are probably better algorithms to use (histogram-based).
    */
   minSum = 2000; /* big enough */
   maxSum = -1; /* small enough */
   for (k = 0; k < N_TEXELS / 2; k++) {
      if (!ISTBLACK(input[k])) {
         GLint sum = 0;
         for (i = 0; i < n_comp; i++) {
            sum += input[k][i];
         }
         if (minSum > sum) {
            minSum = sum;
            minColL = k;
         }
         if (maxSum < sum) {
            maxSum = sum;
            maxColL = k;
         }
      }
   }
   minSum = 2000; /* big enough */
   maxSum = -1; /* small enough */
   for (; k < N_TEXELS; k++) {
      if (!ISTBLACK(input[k])) {
         GLint sum = 0;
         for (i = 0; i < n_comp; i++) {
            sum += input[k][i];
         }
         if (minSum > sum) {
            minSum = sum;
            minColR = k;
         }
         if (maxSum < sum) {
            maxSum = sum;
            maxColR = k;
         }
      }
   }

   /* left microtile */
   if (maxColL == -1) {
      /* all transparent black */
      cc[0] = ~0u;
      for (i = 0; i < n_comp; i++) {
         vec[0][i] = 0;
         vec[1][i] = 0;
      }
   } else {
      cc[0] = 0;
      for (i = 0; i < n_comp; i++) {
         vec[0][i] = input[minColL][i];
         vec[1][i] = input[maxColL][i];
      }
      if (minColL != maxColL) {
         /* compute interpolation vector */
         MAKEIVEC(n_vect, n_comp, iv, b, vec[0], vec[1]);

         /* add in texels */
         lolo = 0;
         for (k = N_TEXELS / 2 - 1; k >= 0; k--) {
            GLint texel = n_vect + 1; /* transparent black */
            if (!ISTBLACK(input[k])) {
               /* interpolate color */
               CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
            }
            /* add in texel */
            lolo <<= 2;
            lolo |= texel;
         }
         cc[0] = lolo;
      }
   }

   /* right microtile */
   if (maxColR == -1) {
      /* all transparent black */
      cc[1] = ~0u;
      for (i = 0; i < n_comp; i++) {
         vec[2][i] = 0;
         vec[3][i] = 0;
      }
   } else {
      cc[1] = 0;
      for (i = 0; i < n_comp; i++) {
         vec[2][i] = input[minColR][i];
         vec[3][i] = input[maxColR][i];
      }
      if (minColR != maxColR) {
         /* compute interpolation vector */
         MAKEIVEC(n_vect, n_comp, iv, b, vec[2], vec[3]);

         /* add in texels */
         lohi = 0;
         for (k = N_TEXELS - 1; k >= N_TEXELS / 2; k--) {
            GLint texel = n_vect + 1; /* transparent black */
            if (!ISTBLACK(input[k])) {
               /* interpolate color */
               CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
            }
            /* add in texel */
            lohi <<= 2;
            lohi |= texel;
         }
         cc[1] = lohi;
      }
   }

   FX64_MOV32(hi, 9 | (vec[3][GCOMP] & 4) | ((vec[1][GCOMP] >> 1) & 2)); /* chroma = "1" */
   for (j = 2 * 2 - 1; j >= 0; j--) {
      for (i = 0; i < n_comp; i++) {
         /* add in colors */
         FX64_SHL(hi, 5);
         FX64_OR32(hi, vec[j][i] >> 3);
      }
   }
   ((Fx64 *)cc)[1] = hi;
}


static void
fxt1_quantize_MIXED0 (GLuint *cc,
                      GLubyte input[N_TEXELS][MAX_COMP])
{
   const GLint n_vect = 3; /* highest vector number in each microtile */
   const GLint n_comp = 3; /* 3 components: R, G, B */
   GLubyte vec[2 * 2][MAX_COMP]; /* 2 extrema for each sub-block */
   GLfloat b, iv[MAX_COMP]; /* interpolation vector */
   GLint i, j, k;
   Fx64 hi; /* high quadword */
   GLuint lohi, lolo; /* low quadword: hi dword, lo dword */

   GLint minColL = 0, maxColL = 0;
   GLint minColR = 0, maxColR = 0;
#if 0
   GLint minSum;
   GLint maxSum;

   /* Our solution here is to find the darkest and brightest colors in
    * the 4x4 tile and use those as the two representative colors.
    * There are probably better algorithms to use (histogram-based).
    */
   minSum = 2000; /* big enough */
   maxSum = -1; /* small enough */
   for (k = 0; k < N_TEXELS / 2; k++) {
      GLint sum = 0;
      for (i = 0; i < n_comp; i++) {
         sum += input[k][i];
      }
      if (minSum > sum) {
         minSum = sum;
         minColL = k;
      }
      if (maxSum < sum) {
         maxSum = sum;
         maxColL = k;
      }
   }
   minSum = 2000; /* big enough */
   maxSum = -1; /* small enough */
   for (; k < N_TEXELS; k++) {
      GLint sum = 0;
      for (i = 0; i < n_comp; i++) {
         sum += input[k][i];
      }
      if (minSum > sum) {
         minSum = sum;
         minColR = k;
      }
      if (maxSum < sum) {
         maxSum = sum;
         maxColR = k;
      }
   }
#else
   GLint minVal;
   GLint maxVal;
   GLint maxVarL = fxt1_variance(NULL, input, n_comp, N_TEXELS / 2);
   GLint maxVarR = fxt1_variance(NULL, &input[N_TEXELS / 2], n_comp, N_TEXELS / 2);

   /* Scan the channel with max variance for lo & hi
    * and use those as the two representative colors.
    */
   minVal = 2000; /* big enough */
   maxVal = -1; /* small enough */
   for (k = 0; k < N_TEXELS / 2; k++) {
      GLint t = input[k][maxVarL];
      if (minVal > t) {
         minVal = t;
         minColL = k;
      }
      if (maxVal < t) {
         maxVal = t;
         maxColL = k;
      }
   }
   minVal = 2000; /* big enough */
   maxVal = -1; /* small enough */
   for (; k < N_TEXELS; k++) {
      GLint t = input[k][maxVarR];
      if (minVal > t) {
         minVal = t;
         minColR = k;
      }
      if (maxVal < t) {
         maxVal = t;
         maxColR = k;
      }
   }
#endif

   /* left microtile */
   cc[0] = 0;
   for (i = 0; i < n_comp; i++) {
      vec[0][i] = input[minColL][i];
      vec[1][i] = input[maxColL][i];
   }
   if (minColL != maxColL) {
      /* compute interpolation vector */
      MAKEIVEC(n_vect, n_comp, iv, b, vec[0], vec[1]);

      /* add in texels */
      lolo = 0;
      for (k = N_TEXELS / 2 - 1; k >= 0; k--) {
         GLint texel;
         /* interpolate color */
         CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
         /* add in texel */
         lolo <<= 2;
         lolo |= texel;
      }

      /* funky encoding for LSB of green */
      if ((GLint)((lolo >> 1) & 1) != (((vec[1][GCOMP] ^ vec[0][GCOMP]) >> 2) & 1)) {
         for (i = 0; i < n_comp; i++) {
            vec[1][i] = input[minColL][i];
            vec[0][i] = input[maxColL][i];
         }
         lolo = ~lolo;
      }
      
      cc[0] = lolo;
   }

   /* right microtile */
   cc[1] = 0;
   for (i = 0; i < n_comp; i++) {
      vec[2][i] = input[minColR][i];
      vec[3][i] = input[maxColR][i];
   }
   if (minColR != maxColR) {
      /* compute interpolation vector */
      MAKEIVEC(n_vect, n_comp, iv, b, vec[2], vec[3]);

      /* add in texels */
      lohi = 0;
      for (k = N_TEXELS - 1; k >= N_TEXELS / 2; k--) {
         GLint texel;
         /* interpolate color */
         CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
         /* add in texel */
         lohi <<= 2;
         lohi |= texel;
      }

      /* funky encoding for LSB of green */
      if ((GLint)((lohi >> 1) & 1) != (((vec[3][GCOMP] ^ vec[2][GCOMP]) >> 2) & 1)) {
         for (i = 0; i < n_comp; i++) {
            vec[3][i] = input[minColR][i];
            vec[2][i] = input[maxColR][i];
         }
         lohi = ~lohi;
      }

      cc[1] = lohi;
   }

   FX64_MOV32(hi, 8 | (vec[3][GCOMP] & 4) | ((vec[1][GCOMP] >> 1) & 2)); /* chroma = "1" */
   for (j = 2 * 2 - 1; j >= 0; j--) {
      for (i = 0; i < n_comp; i++) {
         /* add in colors */
         FX64_SHL(hi, 5);
         FX64_OR32(hi, vec[j][i] >> 3);
      }
   }
   ((Fx64 *)cc)[1] = hi;
}


static void
fxt1_quantize (GLuint *cc, const GLubyte *lines[], GLint comps)
{
   GLint trualpha;
   GLubyte reord[N_TEXELS][MAX_COMP];

   GLubyte input[N_TEXELS][MAX_COMP];
   GLint i, k, l;

   if (comps == 3) {
      /* make the whole block opaque */
      memset(input, -1, sizeof(input));
   }

   /* 8 texels each line */
   for (l = 0; l < 4; l++) {
      for (k = 0; k < 4; k++) {
         for (i = 0; i < comps; i++) {
            input[k + l * 4][i] = *lines[l]++;
         }
      }
      for (; k < 8; k++) {
         for (i = 0; i < comps; i++) {
            input[k + l * 4 + 12][i] = *lines[l]++;
         }
      }
   }

   /* block layout:
    * 00, 01, 02, 03, 08, 09, 0a, 0b
    * 10, 11, 12, 13, 18, 19, 1a, 1b
    * 04, 05, 06, 07, 0c, 0d, 0e, 0f
    * 14, 15, 16, 17, 1c, 1d, 1e, 1f
    */

   /* [dBorca]
    * stupidity flows forth from this
    */
   l = N_TEXELS;
   trualpha = 0;
   if (comps == 4) {
      /* skip all transparent black texels */
      l = 0;
      for (k = 0; k < N_TEXELS; k++) {
         /* test all components against 0 */
         if (!ISTBLACK(input[k])) {
            /* texel is not transparent black */
            COPY_4UBV(reord[l], input[k]);
            if (reord[l][ACOMP] < (255 - ALPHA_TS)) {
               /* non-opaque texel */
               trualpha = !0;
            }
            l++;
         }
      }
   }

#if 0
   if (trualpha) {
      fxt1_quantize_ALPHA0(cc, input, reord, l);
   } else if (l == 0) {
      cc[0] = cc[1] = cc[2] = -1;
      cc[3] = 0;
   } else if (l < N_TEXELS) {
      fxt1_quantize_HI(cc, input, reord, l);
   } else {
      fxt1_quantize_CHROMA(cc, input);
   }
   (void)fxt1_quantize_ALPHA1;
   (void)fxt1_quantize_MIXED1;
   (void)fxt1_quantize_MIXED0;
#else
   if (trualpha) {
      fxt1_quantize_ALPHA1(cc, input);
   } else if (l == 0) {
      cc[0] = cc[1] = cc[2] = ~0u;
      cc[3] = 0;
   } else if (l < N_TEXELS) {
      fxt1_quantize_MIXED1(cc, input);
   } else {
      fxt1_quantize_MIXED0(cc, input);
   }
   (void)fxt1_quantize_ALPHA0;
   (void)fxt1_quantize_HI;
   (void)fxt1_quantize_CHROMA;
#endif
}



/**
 * Upscale an image by replication, not (typical) stretching.
 * We use this when the image width or height is less than a
 * certain size (4, 8) and we need to upscale an image.
 */
static void
upscale_teximage2d(GLsizei inWidth, GLsizei inHeight,
                   GLsizei outWidth, GLsizei outHeight,
                   GLint comps, const GLubyte *src, GLint srcRowStride,
                   GLubyte *dest )
{
   GLint i, j, k;

   ASSERT(outWidth >= inWidth);
   ASSERT(outHeight >= inHeight);
#if 0
   ASSERT(inWidth == 1 || inWidth == 2 || inHeight == 1 || inHeight == 2);
   ASSERT((outWidth & 3) == 0);
   ASSERT((outHeight & 3) == 0);
#endif

   for (i = 0; i < outHeight; i++) {
      const GLint ii = i % inHeight;
      for (j = 0; j < outWidth; j++) {
         const GLint jj = j % inWidth;
         for (k = 0; k < comps; k++) {
            dest[(i * outWidth + j) * comps + k]
               = src[ii * srcRowStride + jj * comps + k];
         }
      }
   }
}


static void
fxt1_encode (GLuint width, GLuint height, GLint comps,
             const void *source, GLint srcRowStride,
             void *dest, GLint destRowStride)
{
   GLuint x, y;
   const GLubyte *data;
   GLuint *encoded = (GLuint *)dest;
   void *newSource = NULL;

   assert(comps == 3 || comps == 4);

   /* Replicate image if width is not M8 or height is not M4 */
   if ((width & 7) | (height & 3)) {
      GLint newWidth = (width + 7) & ~7;
      GLint newHeight = (height + 3) & ~3;
      newSource = malloc(comps * newWidth * newHeight * sizeof(GLubyte));
      if (!newSource) {
         GET_CURRENT_CONTEXT(ctx);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "texture compression");
         goto cleanUp;
      }
      upscale_teximage2d(width, height, newWidth, newHeight,
                         comps, (const GLubyte *) source,
                         srcRowStride, (GLubyte *) newSource);
      source = newSource;
      width = newWidth;
      height = newHeight;
      srcRowStride = comps * newWidth;
   }

   data = (const GLubyte *) source;
   destRowStride = (destRowStride - width * 2) / 4;
   for (y = 0; y < height; y += 4) {
      GLuint offs = 0 + (y + 0) * srcRowStride;
      for (x = 0; x < width; x += 8) {
         const GLubyte *lines[4];
         lines[0] = &data[offs];
         lines[1] = lines[0] + srcRowStride;
         lines[2] = lines[1] + srcRowStride;
         lines[3] = lines[2] + srcRowStride;
         offs += 8 * comps;
         fxt1_quantize(encoded, lines, comps);
         /* 128 bits per 8x4 block */
         encoded += 4;
      }
      encoded += destRowStride;
   }

 cleanUp:
   if (newSource != NULL) {
      free(newSource);
   }
}


/***************************************************************************\
 * FXT1 decoder
 *
 * The decoder is based on GL_3DFX_texture_compression_FXT1
 * specification and serves as a concept for the encoder.
\***************************************************************************/


/* lookup table for scaling 5 bit colors up to 8 bits */
static const GLubyte _rgb_scale_5[] = {
   0,   8,   16,  25,  33,  41,  49,  58,
   66,  74,  82,  90,  99,  107, 115, 123,
   132, 140, 148, 156, 165, 173, 181, 189,
   197, 206, 214, 222, 230, 239, 247, 255
};

/* lookup table for scaling 6 bit colors up to 8 bits */
static const GLubyte _rgb_scale_6[] = {
   0,   4,   8,   12,  16,  20,  24,  28,
   32,  36,  40,  45,  49,  53,  57,  61,
   65,  69,  73,  77,  81,  85,  89,  93,
   97,  101, 105, 109, 113, 117, 121, 125,
   130, 134, 138, 142, 146, 150, 154, 158,
   162, 166, 170, 174, 178, 182, 186, 190,
   194, 198, 202, 206, 210, 215, 219, 223,
   227, 231, 235, 239, 243, 247, 251, 255
};


#define CC_SEL(cc, which) (((GLuint *)(cc))[(which) / 32] >> ((which) & 31))
#define UP5(c) _rgb_scale_5[(c) & 31]
#define UP6(c, b) _rgb_scale_6[(((c) & 31) << 1) | ((b) & 1)]
#define LERP(n, t, c0, c1) (((n) - (t)) * (c0) + (t) * (c1) + (n) / 2) / (n)


static void
fxt1_decode_1HI (const GLubyte *code, GLint t, GLubyte *rgba)
{
   const GLuint *cc;

   t *= 3;
   cc = (const GLuint *)(code + t / 8);
   t = (cc[0] >> (t & 7)) & 7;

   if (t == 7) {
      rgba[RCOMP] = rgba[GCOMP] = rgba[BCOMP] = rgba[ACOMP] = 0;
   } else {
      GLubyte r, g, b;
      cc = (const GLuint *)(code + 12);
      if (t == 0) {
         b = UP5(CC_SEL(cc, 0));
         g = UP5(CC_SEL(cc, 5));
         r = UP5(CC_SEL(cc, 10));
      } else if (t == 6) {
         b = UP5(CC_SEL(cc, 15));
         g = UP5(CC_SEL(cc, 20));
         r = UP5(CC_SEL(cc, 25));
      } else {
         b = LERP(6, t, UP5(CC_SEL(cc, 0)), UP5(CC_SEL(cc, 15)));
         g = LERP(6, t, UP5(CC_SEL(cc, 5)), UP5(CC_SEL(cc, 20)));
         r = LERP(6, t, UP5(CC_SEL(cc, 10)), UP5(CC_SEL(cc, 25)));
      }
      rgba[RCOMP] = r;
      rgba[GCOMP] = g;
      rgba[BCOMP] = b;
      rgba[ACOMP] = 255;
   }
}


static void
fxt1_decode_1CHROMA (const GLubyte *code, GLint t, GLubyte *rgba)
{
   const GLuint *cc;
   GLuint kk;

   cc = (const GLuint *)code;
   if (t & 16) {
      cc++;
      t &= 15;
   }
   t = (cc[0] >> (t * 2)) & 3;

   t *= 15;
   cc = (const GLuint *)(code + 8 + t / 8);
   kk = cc[0] >> (t & 7);
   rgba[BCOMP] = UP5(kk);
   rgba[GCOMP] = UP5(kk >> 5);
   rgba[RCOMP] = UP5(kk >> 10);
   rgba[ACOMP] = 255;
}


static void
fxt1_decode_1MIXED (const GLubyte *code, GLint t, GLubyte *rgba)
{
   const GLuint *cc;
   GLuint col[2][3];
   GLint glsb, selb;

   cc = (const GLuint *)code;
   if (t & 16) {
      t &= 15;
      t = (cc[1] >> (t * 2)) & 3;
      /* col 2 */
      col[0][BCOMP] = (*(const GLuint *)(code + 11)) >> 6;
      col[0][GCOMP] = CC_SEL(cc, 99);
      col[0][RCOMP] = CC_SEL(cc, 104);
      /* col 3 */
      col[1][BCOMP] = CC_SEL(cc, 109);
      col[1][GCOMP] = CC_SEL(cc, 114);
      col[1][RCOMP] = CC_SEL(cc, 119);
      glsb = CC_SEL(cc, 126);
      selb = CC_SEL(cc, 33);
   } else {
      t = (cc[0] >> (t * 2)) & 3;
      /* col 0 */
      col[0][BCOMP] = CC_SEL(cc, 64);
      col[0][GCOMP] = CC_SEL(cc, 69);
      col[0][RCOMP] = CC_SEL(cc, 74);
      /* col 1 */
      col[1][BCOMP] = CC_SEL(cc, 79);
      col[1][GCOMP] = CC_SEL(cc, 84);
      col[1][RCOMP] = CC_SEL(cc, 89);
      glsb = CC_SEL(cc, 125);
      selb = CC_SEL(cc, 1);
   }

   if (CC_SEL(cc, 124) & 1) {
      /* alpha[0] == 1 */

      if (t == 3) {
         /* zero */
         rgba[RCOMP] = rgba[BCOMP] = rgba[GCOMP] = rgba[ACOMP] = 0;
      } else {
         GLubyte r, g, b;
         if (t == 0) {
            b = UP5(col[0][BCOMP]);
            g = UP5(col[0][GCOMP]);
            r = UP5(col[0][RCOMP]);
         } else if (t == 2) {
            b = UP5(col[1][BCOMP]);
            g = UP6(col[1][GCOMP], glsb);
            r = UP5(col[1][RCOMP]);
         } else {
            b = (UP5(col[0][BCOMP]) + UP5(col[1][BCOMP])) / 2;
            g = (UP5(col[0][GCOMP]) + UP6(col[1][GCOMP], glsb)) / 2;
            r = (UP5(col[0][RCOMP]) + UP5(col[1][RCOMP])) / 2;
         }
         rgba[RCOMP] = r;
         rgba[GCOMP] = g;
         rgba[BCOMP] = b;
         rgba[ACOMP] = 255;
      }
   } else {
      /* alpha[0] == 0 */
      GLubyte r, g, b;
      if (t == 0) {
         b = UP5(col[0][BCOMP]);
         g = UP6(col[0][GCOMP], glsb ^ selb);
         r = UP5(col[0][RCOMP]);
      } else if (t == 3) {
         b = UP5(col[1][BCOMP]);
         g = UP6(col[1][GCOMP], glsb);
         r = UP5(col[1][RCOMP]);
      } else {
         b = LERP(3, t, UP5(col[0][BCOMP]), UP5(col[1][BCOMP]));
         g = LERP(3, t, UP6(col[0][GCOMP], glsb ^ selb),
                        UP6(col[1][GCOMP], glsb));
         r = LERP(3, t, UP5(col[0][RCOMP]), UP5(col[1][RCOMP]));
      }
      rgba[RCOMP] = r;
      rgba[GCOMP] = g;
      rgba[BCOMP] = b;
      rgba[ACOMP] = 255;
   }
}


static void
fxt1_decode_1ALPHA (const GLubyte *code, GLint t, GLubyte *rgba)
{
   const GLuint *cc;
   GLubyte r, g, b, a;

   cc = (const GLuint *)code;
   if (CC_SEL(cc, 124) & 1) {
      /* lerp == 1 */
      GLuint col0[4];

      if (t & 16) {
         t &= 15;
         t = (cc[1] >> (t * 2)) & 3;
         /* col 2 */
         col0[BCOMP] = (*(const GLuint *)(code + 11)) >> 6;
         col0[GCOMP] = CC_SEL(cc, 99);
         col0[RCOMP] = CC_SEL(cc, 104);
         col0[ACOMP] = CC_SEL(cc, 119);
      } else {
         t = (cc[0] >> (t * 2)) & 3;
         /* col 0 */
         col0[BCOMP] = CC_SEL(cc, 64);
         col0[GCOMP] = CC_SEL(cc, 69);
         col0[RCOMP] = CC_SEL(cc, 74);
         col0[ACOMP] = CC_SEL(cc, 109);
      }

      if (t == 0) {
         b = UP5(col0[BCOMP]);
         g = UP5(col0[GCOMP]);
         r = UP5(col0[RCOMP]);
         a = UP5(col0[ACOMP]);
      } else if (t == 3) {
         b = UP5(CC_SEL(cc, 79));
         g = UP5(CC_SEL(cc, 84));
         r = UP5(CC_SEL(cc, 89));
         a = UP5(CC_SEL(cc, 114));
      } else {
         b = LERP(3, t, UP5(col0[BCOMP]), UP5(CC_SEL(cc, 79)));
         g = LERP(3, t, UP5(col0[GCOMP]), UP5(CC_SEL(cc, 84)));
         r = LERP(3, t, UP5(col0[RCOMP]), UP5(CC_SEL(cc, 89)));
         a = LERP(3, t, UP5(col0[ACOMP]), UP5(CC_SEL(cc, 114)));
      }
   } else {
      /* lerp == 0 */

      if (t & 16) {
         cc++;
         t &= 15;
      }
      t = (cc[0] >> (t * 2)) & 3;

      if (t == 3) {
         /* zero */
         r = g = b = a = 0;
      } else {
         GLuint kk;
         cc = (const GLuint *)code;
         a = UP5(cc[3] >> (t * 5 + 13));
         t *= 15;
         cc = (const GLuint *)(code + 8 + t / 8);
         kk = cc[0] >> (t & 7);
         b = UP5(kk);
         g = UP5(kk >> 5);
         r = UP5(kk >> 10);
      }
   }
   rgba[RCOMP] = r;
   rgba[GCOMP] = g;
   rgba[BCOMP] = b;
   rgba[ACOMP] = a;
}


void
fxt1_decode_1 (const void *texture, GLint stride, /* in pixels */
               GLint i, GLint j, GLubyte *rgba)
{
   static void (*decode_1[]) (const GLubyte *, GLint, GLubyte *) = {
      fxt1_decode_1HI,     /* cc-high   = "00?" */
      fxt1_decode_1HI,     /* cc-high   = "00?" */
      fxt1_decode_1CHROMA, /* cc-chroma = "010" */
      fxt1_decode_1ALPHA,  /* alpha     = "011" */
      fxt1_decode_1MIXED,  /* mixed     = "1??" */
      fxt1_decode_1MIXED,  /* mixed     = "1??" */
      fxt1_decode_1MIXED,  /* mixed     = "1??" */
      fxt1_decode_1MIXED   /* mixed     = "1??" */
   };

   const GLubyte *code = (const GLubyte *)texture +
                         ((j / 4) * (stride / 8) + (i / 8)) * 16;
   GLint mode = CC_SEL(code, 125);
   GLint t = i & 7;

   if (t & 4) {
      t += 12;
   }
   t += (j & 3) * 4;

   decode_1[mode](code, t, rgba);
}


#endif /* FEATURE_texture_fxt1 */
