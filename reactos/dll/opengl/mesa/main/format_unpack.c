/*
 * Mesa 3-D graphics library
 *
 * Copyright (c) 2011 VMware, Inc.
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <precomp.h>

/* Expand 1, 2, 3, 4, 5, 6-bit values to fill 8 bits */

#define EXPAND_1_8(X)  ( (X) ? 0xff : 0x0 )

#define EXPAND_2_8(X)  ( ((X) << 6) | ((X) << 4) | ((X) << 2) | (X) )

#define EXPAND_3_8(X)  ( ((X) << 5) | ((X) << 2) | ((X) >> 1) )

#define EXPAND_4_8(X)  ( ((X) << 4) | (X) )

#define EXPAND_5_8(X)  ( ((X) << 3) | ((X) >> 2) )

#define EXPAND_6_8(X)  ( ((X) << 2) | ((X) >> 4) )

/**********************************************************************/
/*  Unpack, returning GLfloat colors                                  */
/**********************************************************************/

typedef void (*unpack_rgba_func)(const void *src, GLfloat dst[][4], GLuint n);


static void
unpack_RGBA8888(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i] >> 24)        );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( (s[i]      ) & 0xff );
   }
}

static void
unpack_RGBA8888_REV(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i]      ) & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( (s[i] >> 24)        );
   }
}

static void
unpack_ARGB8888(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i]      ) & 0xff );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( (s[i] >> 24)        );
   }
}

static void
unpack_ARGB8888_REV(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i] >> 24)        );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( (s[i]      ) & 0xff );
   }
}

static void
unpack_RGBX8888(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i] >> 24)        );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][ACOMP] = 1.0f;
   }
}

static void
unpack_RGBX8888_REV(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i]      ) & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][ACOMP] = 1.0f;
   }
}

static void
unpack_XRGB8888(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i]      ) & 0xff );
      dst[i][ACOMP] = 1.0f;
   }
}

static void
unpack_XRGB8888_REV(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i] >> 24)        );
      dst[i][ACOMP] = 1.0f;
   }
}

static void
unpack_RGB888(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( s[i*3+2] );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( s[i*3+1] );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( s[i*3+0] );
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_BGR888(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( s[i*3+0] );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( s[i*3+1] );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( s[i*3+2] );
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_RGB565(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >> 11) & 0x1f) * (1.0F / 31.0F);
      dst[i][GCOMP] = ((s[i] >> 5 ) & 0x3f) * (1.0F / 63.0F);
      dst[i][BCOMP] = ((s[i]      ) & 0x1f) * (1.0F / 31.0F);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_RGB565_REV(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLuint t = (s[i] >> 8) | (s[i] << 8); /* byte swap */
      dst[i][RCOMP] = UBYTE_TO_FLOAT( ((t >> 8) & 0xf8) | ((t >> 13) & 0x7) );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( ((t >> 3) & 0xfc) | ((t >>  9) & 0x3) );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( ((t << 3) & 0xf8) | ((t >>  2) & 0x7) );
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_ARGB4444(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >>  8) & 0xf) * (1.0F / 15.0F);
      dst[i][GCOMP] = ((s[i] >>  4) & 0xf) * (1.0F / 15.0F);
      dst[i][BCOMP] = ((s[i]      ) & 0xf) * (1.0F / 15.0F);
      dst[i][ACOMP] = ((s[i] >> 12) & 0xf) * (1.0F / 15.0F);
   }
}

static void
unpack_ARGB4444_REV(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i]      ) & 0xf) * (1.0F / 15.0F);
      dst[i][GCOMP] = ((s[i] >> 12) & 0xf) * (1.0F / 15.0F);
      dst[i][BCOMP] = ((s[i] >>  8) & 0xf) * (1.0F / 15.0F);
      dst[i][ACOMP] = ((s[i] >>  4) & 0xf) * (1.0F / 15.0F);
   }
}

static void
unpack_RGBA5551(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >> 11) & 0x1f) * (1.0F / 31.0F);
      dst[i][GCOMP] = ((s[i] >>  6) & 0x1f) * (1.0F / 31.0F);
      dst[i][BCOMP] = ((s[i] >>  1) & 0x1f) * (1.0F / 31.0F);
      dst[i][ACOMP] = ((s[i]      ) & 0x01) * 1.0F;
   }
}

static void
unpack_ARGB1555(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >> 10) & 0x1f) * (1.0F / 31.0F);
      dst[i][GCOMP] = ((s[i] >>  5) & 0x1f) * (1.0F / 31.0F);
      dst[i][BCOMP] = ((s[i] >>  0) & 0x1f) * (1.0F / 31.0F);
      dst[i][ACOMP] = ((s[i] >> 15) & 0x01) * 1.0F;
   }
}

static void
unpack_ARGB1555_REV(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLushort tmp = (s[i] << 8) | (s[i] >> 8); /* byteswap */
      dst[i][RCOMP] = ((tmp >> 10) & 0x1f) * (1.0F / 31.0F);
      dst[i][GCOMP] = ((tmp >>  5) & 0x1f) * (1.0F / 31.0F);
      dst[i][BCOMP] = ((tmp >>  0) & 0x1f) * (1.0F / 31.0F);
      dst[i][ACOMP] = ((tmp >> 15) & 0x01) * 1.0F;
   }
}

static void
unpack_AL44(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = (s[i] & 0xf) * (1.0F / 15.0F);
      dst[i][ACOMP] = ((s[i] >> 4) & 0xf) * (1.0F / 15.0F);
   }
}

static void
unpack_AL88(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = 
      dst[i][GCOMP] = 
      dst[i][BCOMP] = UBYTE_TO_FLOAT( s[i] & 0xff );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( s[i] >> 8 );
   }
}

static void
unpack_AL88_REV(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = 
      dst[i][GCOMP] = 
      dst[i][BCOMP] = UBYTE_TO_FLOAT( s[i] >> 8 );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( s[i] & 0xff );
   }
}

static void
unpack_AL1616(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = USHORT_TO_FLOAT( s[i] & 0xffff );
      dst[i][ACOMP] = USHORT_TO_FLOAT( s[i] >> 16 );
   }
}

static void
unpack_AL1616_REV(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = USHORT_TO_FLOAT( s[i] >> 16 );
      dst[i][ACOMP] = USHORT_TO_FLOAT( s[i] & 0xffff );
   }
}

static void
unpack_RGB332(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >> 5) & 0x7) * (1.0F / 7.0F);
      dst[i][GCOMP] = ((s[i] >> 2) & 0x7) * (1.0F / 7.0F);
      dst[i][BCOMP] = ((s[i]     ) & 0x3) * (1.0F / 3.0F);
      dst[i][ACOMP] = 1.0F;
   }
}


static void
unpack_A8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = UBYTE_TO_FLOAT(s[i]);
   }
}

static void
unpack_A16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = USHORT_TO_FLOAT(s[i]);
   }
}

static void
unpack_L8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = UBYTE_TO_FLOAT(s[i]);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_L16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = USHORT_TO_FLOAT(s[i]);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_I8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = UBYTE_TO_FLOAT(s[i]);
   }
}

static void
unpack_I16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = USHORT_TO_FLOAT(s[i]);
   }
}

static void
unpack_YCBCR(const void *src, GLfloat dst[][4], GLuint n)
{
   GLuint i;
   for (i = 0; i < n; i++) {
      const GLushort *src0 = ((const GLushort *) src) + i * 2; /* even */
      const GLushort *src1 = src0 + 1;         /* odd */
      const GLubyte y0 = (*src0 >> 8) & 0xff;  /* luminance */
      const GLubyte cb = *src0 & 0xff;         /* chroma U */
      const GLubyte y1 = (*src1 >> 8) & 0xff;  /* luminance */
      const GLubyte cr = *src1 & 0xff;         /* chroma V */
      const GLubyte y = (i & 1) ? y1 : y0;     /* choose even/odd luminance */
      GLfloat r = 1.164F * (y - 16) + 1.596F * (cr - 128);
      GLfloat g = 1.164F * (y - 16) - 0.813F * (cr - 128) - 0.391F * (cb - 128);
      GLfloat b = 1.164F * (y - 16) + 2.018F * (cb - 128);
      r *= (1.0F / 255.0F);
      g *= (1.0F / 255.0F);
      b *= (1.0F / 255.0F);
      dst[i][RCOMP] = CLAMP(r, 0.0F, 1.0F);
      dst[i][GCOMP] = CLAMP(g, 0.0F, 1.0F);
      dst[i][BCOMP] = CLAMP(b, 0.0F, 1.0F);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_YCBCR_REV(const void *src, GLfloat dst[][4], GLuint n)
{
   GLuint i;
   for (i = 0; i < n; i++) {
      const GLushort *src0 = ((const GLushort *) src) + i * 2; /* even */
      const GLushort *src1 = src0 + 1;         /* odd */
      const GLubyte y0 = *src0 & 0xff;         /* luminance */
      const GLubyte cr = (*src0 >> 8) & 0xff;  /* chroma V */
      const GLubyte y1 = *src1 & 0xff;         /* luminance */
      const GLubyte cb = (*src1 >> 8) & 0xff;  /* chroma U */
      const GLubyte y = (i & 1) ? y1 : y0;     /* choose even/odd luminance */
      GLfloat r = 1.164F * (y - 16) + 1.596F * (cr - 128);
      GLfloat g = 1.164F * (y - 16) - 0.813F * (cr - 128) - 0.391F * (cb - 128);
      GLfloat b = 1.164F * (y - 16) + 2.018F * (cb - 128);
      r *= (1.0F / 255.0F);
      g *= (1.0F / 255.0F);
      b *= (1.0F / 255.0F);
      dst[i][RCOMP] = CLAMP(r, 0.0F, 1.0F);
      dst[i][GCOMP] = CLAMP(g, 0.0F, 1.0F);
      dst[i][BCOMP] = CLAMP(b, 0.0F, 1.0F);
      dst[i][ACOMP] = 1.0F;
   }
}


static void
unpack_Z24_S8(const void *src, GLfloat dst[][4], GLuint n)
{
   /* only return Z, not stencil data */
   const GLuint *s = ((const GLuint *) src);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] =
      dst[i][1] =
      dst[i][2] = (s[i] >> 8) * scale;
      dst[i][3] = 1.0F;
      ASSERT(dst[i][0] >= 0.0F);
      ASSERT(dst[i][0] <= 1.0F);
   }
}

static void
unpack_S8_Z24(const void *src, GLfloat dst[][4], GLuint n)
{
   /* only return Z, not stencil data */
   const GLuint *s = ((const GLuint *) src);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] =
      dst[i][1] =
      dst[i][2] = (s[i] & 0x00ffffff) * scale;
      dst[i][3] = 1.0F;
      ASSERT(dst[i][0] >= 0.0F);
      ASSERT(dst[i][0] <= 1.0F);
   }
}

static void
unpack_Z16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] =
      dst[i][1] =
      dst[i][2] = s[i] * (1.0F / 65535.0F);
      dst[i][3] = 1.0F;
   }
}

static void
unpack_X8_Z24(const void *src, GLfloat dst[][4], GLuint n)
{
   unpack_S8_Z24(src, dst, n);
}

static void
unpack_Z24_X8(const void *src, GLfloat dst[][4], GLuint n)
{
   unpack_Z24_S8(src, dst, n);
}

static void
unpack_Z32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] =
      dst[i][1] =
      dst[i][2] = s[i] * (1.0F / 0xffffffff);
      dst[i][3] = 1.0F;
   }
}


static void
unpack_S8(const void *src, GLfloat dst[][4], GLuint n)
{
   /* should never be used */
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] =
      dst[i][1] =
      dst[i][2] = 0.0F;
      dst[i][3] = 1.0F;
   }
}

static void
unpack_RGBA_INT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = (const GLbyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = (GLfloat) s[i*4+3];
   }
}

static void
unpack_RGBA_INT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = (GLfloat) s[i*4+3];
   }
}

static void
unpack_RGBA_INT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLint *s = (const GLint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = (GLfloat) s[i*4+3];
   }
}

static void
unpack_RGBA_UINT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = (GLfloat) s[i*4+3];
   }
}

static void
unpack_RGBA_UINT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = (GLfloat) s[i*4+3];
   }
}

static void
unpack_RGBA_UINT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = (GLfloat) s[i*4+3];
   }
}

static void
unpack_SIGNED_RGBA_16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = SHORT_TO_FLOAT_TEX( s[i*4+0] );
      dst[i][GCOMP] = SHORT_TO_FLOAT_TEX( s[i*4+1] );
      dst[i][BCOMP] = SHORT_TO_FLOAT_TEX( s[i*4+2] );
      dst[i][ACOMP] = SHORT_TO_FLOAT_TEX( s[i*4+3] );
   }
}

static void
unpack_RGBA_16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = USHORT_TO_FLOAT( s[i*4+0] );
      dst[i][GCOMP] = USHORT_TO_FLOAT( s[i*4+1] );
      dst[i][BCOMP] = USHORT_TO_FLOAT( s[i*4+2] );
      dst[i][ACOMP] = USHORT_TO_FLOAT( s[i*4+3] );
   }
}


/**
 * Return the unpacker function for the given format.
 */
static unpack_rgba_func
get_unpack_rgba_function(gl_format format)
{
   static unpack_rgba_func table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      table[MESA_FORMAT_NONE] = NULL;

      table[MESA_FORMAT_RGBA8888] = unpack_RGBA8888;
      table[MESA_FORMAT_RGBA8888_REV] = unpack_RGBA8888_REV;
      table[MESA_FORMAT_ARGB8888] = unpack_ARGB8888;
      table[MESA_FORMAT_ARGB8888_REV] = unpack_ARGB8888_REV;
      table[MESA_FORMAT_RGBX8888] = unpack_RGBX8888;
      table[MESA_FORMAT_RGBX8888_REV] = unpack_RGBX8888_REV;
      table[MESA_FORMAT_XRGB8888] = unpack_XRGB8888;
      table[MESA_FORMAT_XRGB8888_REV] = unpack_XRGB8888_REV;
      table[MESA_FORMAT_RGB888] = unpack_RGB888;
      table[MESA_FORMAT_BGR888] = unpack_BGR888;
      table[MESA_FORMAT_RGB565] = unpack_RGB565;
      table[MESA_FORMAT_RGB565_REV] = unpack_RGB565_REV;
      table[MESA_FORMAT_ARGB4444] = unpack_ARGB4444;
      table[MESA_FORMAT_ARGB4444_REV] = unpack_ARGB4444_REV;
      table[MESA_FORMAT_RGBA5551] = unpack_RGBA5551;
      table[MESA_FORMAT_ARGB1555] = unpack_ARGB1555;
      table[MESA_FORMAT_ARGB1555_REV] = unpack_ARGB1555_REV;
      table[MESA_FORMAT_AL44] = unpack_AL44;
      table[MESA_FORMAT_AL88] = unpack_AL88;
      table[MESA_FORMAT_AL88_REV] = unpack_AL88_REV;
      table[MESA_FORMAT_AL1616] = unpack_AL1616;
      table[MESA_FORMAT_AL1616_REV] = unpack_AL1616_REV;
      table[MESA_FORMAT_RGB332] = unpack_RGB332;
      table[MESA_FORMAT_A8] = unpack_A8;
      table[MESA_FORMAT_A16] = unpack_A16;
      table[MESA_FORMAT_L8] = unpack_L8;
      table[MESA_FORMAT_L16] = unpack_L16;
      table[MESA_FORMAT_I8] = unpack_I8;
      table[MESA_FORMAT_I16] = unpack_I16;
      table[MESA_FORMAT_YCBCR] = unpack_YCBCR;
      table[MESA_FORMAT_YCBCR_REV] = unpack_YCBCR_REV;
      table[MESA_FORMAT_Z16] = unpack_Z16;
      table[MESA_FORMAT_X8_Z24] = unpack_X8_Z24;
      table[MESA_FORMAT_Z24_X8] = unpack_Z24_X8;
      table[MESA_FORMAT_Z32] = unpack_Z32;
      table[MESA_FORMAT_S8] = unpack_S8;

      table[MESA_FORMAT_RGBA_INT8] = unpack_RGBA_INT8;
      table[MESA_FORMAT_RGBA_INT16] = unpack_RGBA_INT16;
      table[MESA_FORMAT_RGBA_INT32] = unpack_RGBA_INT32;
      table[MESA_FORMAT_RGBA_UINT8] = unpack_RGBA_UINT8;
      table[MESA_FORMAT_RGBA_UINT16] = unpack_RGBA_UINT16;
      table[MESA_FORMAT_RGBA_UINT32] = unpack_RGBA_UINT32;

      table[MESA_FORMAT_SIGNED_RGBA_16] = unpack_SIGNED_RGBA_16;
      table[MESA_FORMAT_RGBA_16] = unpack_RGBA_16;

      initialized = GL_TRUE;
   }

   return table[format];
}


/**
 * Unpack rgba colors, returning as GLfloat values.
 */
void
_mesa_unpack_rgba_row(gl_format format, GLuint n,
                      const void *src, GLfloat dst[][4])
{
   unpack_rgba_func unpack = get_unpack_rgba_function(format);
   unpack(src, dst, n);
}


/**********************************************************************/
/*  Unpack, returning GLubyte colors                                  */
/**********************************************************************/


static void
unpack_ubyte_RGBA8888(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i] >> 24);
      dst[i][GCOMP] = (s[i] >> 16) & 0xff;
      dst[i][BCOMP] = (s[i] >>  8) & 0xff;
      dst[i][ACOMP] = (s[i]      ) & 0xff;
   }
}

static void
unpack_ubyte_RGBA8888_REV(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i]      ) & 0xff;
      dst[i][GCOMP] = (s[i] >>  8) & 0xff;
      dst[i][BCOMP] = (s[i] >> 16) & 0xff;
      dst[i][ACOMP] = (s[i] >> 24);
   }
}

static void
unpack_ubyte_ARGB8888(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i] >> 16) & 0xff;
      dst[i][GCOMP] = (s[i] >>  8) & 0xff;
      dst[i][BCOMP] = (s[i]      ) & 0xff;
      dst[i][ACOMP] = (s[i] >> 24);
   }
}

static void
unpack_ubyte_ARGB8888_REV(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i] >>  8) & 0xff;
      dst[i][GCOMP] = (s[i] >> 16) & 0xff;
      dst[i][BCOMP] = (s[i] >> 24);
      dst[i][ACOMP] = (s[i]      ) & 0xff;
   }
}

static void
unpack_ubyte_RGBX8888(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i] >> 24);
      dst[i][GCOMP] = (s[i] >> 16) & 0xff;
      dst[i][BCOMP] = (s[i] >>  8) & 0xff;
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_RGBX8888_REV(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i]      ) & 0xff;
      dst[i][GCOMP] = (s[i] >>  8) & 0xff;
      dst[i][BCOMP] = (s[i] >> 16) & 0xff;
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_XRGB8888(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i] >> 16) & 0xff;
      dst[i][GCOMP] = (s[i] >>  8) & 0xff;
      dst[i][BCOMP] = (s[i]      ) & 0xff;
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_XRGB8888_REV(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i] >>  8) & 0xff;
      dst[i][GCOMP] = (s[i] >> 16) & 0xff;
      dst[i][BCOMP] = (s[i] >> 24);
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_RGB888(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i*3+2];
      dst[i][GCOMP] = s[i*3+1];
      dst[i][BCOMP] = s[i*3+0];
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_BGR888(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i*3+0];
      dst[i][GCOMP] = s[i*3+1];
      dst[i][BCOMP] = s[i*3+2];
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_RGB565(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = EXPAND_5_8((s[i] >> 11) & 0x1f);
      dst[i][GCOMP] = EXPAND_6_8((s[i] >> 5 ) & 0x3f);
      dst[i][BCOMP] = EXPAND_5_8( s[i]        & 0x1f);
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_RGB565_REV(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLuint t = (s[i] >> 8) | (s[i] << 8); /* byte swap */
      dst[i][RCOMP] = EXPAND_5_8((t >> 11) & 0x1f);
      dst[i][GCOMP] = EXPAND_6_8((t >> 5 ) & 0x3f);
      dst[i][BCOMP] = EXPAND_5_8( t        & 0x1f);
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_ARGB4444(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = EXPAND_4_8((s[i] >>  8) & 0xf);
      dst[i][GCOMP] = EXPAND_4_8((s[i] >>  4) & 0xf);
      dst[i][BCOMP] = EXPAND_4_8((s[i]      ) & 0xf);
      dst[i][ACOMP] = EXPAND_4_8((s[i] >> 12) & 0xf);
   }
}

static void
unpack_ubyte_ARGB4444_REV(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = EXPAND_4_8((s[i]      ) & 0xf);
      dst[i][GCOMP] = EXPAND_4_8((s[i] >> 12) & 0xf);
      dst[i][BCOMP] = EXPAND_4_8((s[i] >>  8) & 0xf);
      dst[i][ACOMP] = EXPAND_4_8((s[i] >>  4) & 0xf);
   }
}

static void
unpack_ubyte_RGBA5551(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = EXPAND_5_8((s[i] >> 11) & 0x1f);
      dst[i][GCOMP] = EXPAND_5_8((s[i] >>  6) & 0x1f);
      dst[i][BCOMP] = EXPAND_5_8((s[i] >>  1) & 0x1f);
      dst[i][ACOMP] = EXPAND_1_8((s[i]      ) & 0x01);
   }
}

static void
unpack_ubyte_ARGB1555(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = EXPAND_5_8((s[i] >> 10) & 0x1f);
      dst[i][GCOMP] = EXPAND_5_8((s[i] >>  5) & 0x1f);
      dst[i][BCOMP] = EXPAND_5_8((s[i] >>  0) & 0x1f);
      dst[i][ACOMP] = EXPAND_1_8((s[i] >> 15) & 0x01);
   }
}

static void
unpack_ubyte_ARGB1555_REV(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLushort tmp = (s[i] << 8) | (s[i] >> 8); /* byteswap */
      dst[i][RCOMP] = EXPAND_5_8((tmp >> 10) & 0x1f);
      dst[i][GCOMP] = EXPAND_5_8((tmp >>  5) & 0x1f);
      dst[i][BCOMP] = EXPAND_5_8((tmp >>  0) & 0x1f);
      dst[i][ACOMP] = EXPAND_1_8((tmp >> 15) & 0x01);
   }
}

static void
unpack_ubyte_AL44(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = EXPAND_4_8(s[i] & 0xf);
      dst[i][ACOMP] = EXPAND_4_8(s[i] >> 4);
   }
}

static void
unpack_ubyte_AL88(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = 
      dst[i][GCOMP] = 
      dst[i][BCOMP] = EXPAND_4_8(s[i] & 0xff);
      dst[i][ACOMP] = EXPAND_4_8(s[i] >> 8);
   }
}

static void
unpack_ubyte_AL88_REV(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = 
      dst[i][GCOMP] = 
      dst[i][BCOMP] = EXPAND_4_8(s[i] >> 8);
      dst[i][ACOMP] = EXPAND_4_8(s[i] & 0xff);
   }
}

static void
unpack_ubyte_RGB332(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = EXPAND_3_8((s[i] >> 5) & 0x7);
      dst[i][GCOMP] = EXPAND_3_8((s[i] >> 2) & 0x7);
      dst[i][BCOMP] = EXPAND_2_8((s[i]     ) & 0x3);
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_A8(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0;
      dst[i][ACOMP] = s[i];
   }
}

static void
unpack_ubyte_L8(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = s[i];
      dst[i][ACOMP] = 0xff;
   }
}


static void
unpack_ubyte_I8(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = s[i];
   }
}


/**
 * Unpack rgba colors, returning as GLubyte values.  This should usually
 * only be used for unpacking formats that use 8 bits or less per channel.
 */
void
_mesa_unpack_ubyte_rgba_row(gl_format format, GLuint n,
                            const void *src, GLubyte dst[][4])
{
   switch (format) {
   case MESA_FORMAT_RGBA8888:
      unpack_ubyte_RGBA8888(src, dst, n);
      break;
   case MESA_FORMAT_RGBA8888_REV:
      unpack_ubyte_RGBA8888_REV(src, dst, n);
      break;
   case MESA_FORMAT_ARGB8888:
      unpack_ubyte_ARGB8888(src, dst, n);
      break;
   case MESA_FORMAT_ARGB8888_REV:
      unpack_ubyte_ARGB8888_REV(src, dst, n);
      break;
   case MESA_FORMAT_RGBX8888:
      unpack_ubyte_RGBX8888(src, dst, n);
      break;
   case MESA_FORMAT_RGBX8888_REV:
      unpack_ubyte_RGBX8888_REV(src, dst, n);
      break;
   case MESA_FORMAT_XRGB8888:
      unpack_ubyte_XRGB8888(src, dst, n);
      break;
   case MESA_FORMAT_XRGB8888_REV:
      unpack_ubyte_XRGB8888_REV(src, dst, n);
      break;
   case MESA_FORMAT_RGB888:
      unpack_ubyte_RGB888(src, dst, n);
      break;
   case MESA_FORMAT_BGR888:
      unpack_ubyte_BGR888(src, dst, n);
      break;
   case MESA_FORMAT_RGB565:
      unpack_ubyte_RGB565(src, dst, n);
      break;
   case MESA_FORMAT_RGB565_REV:
      unpack_ubyte_RGB565_REV(src, dst, n);
      break;
   case MESA_FORMAT_ARGB4444:
      unpack_ubyte_ARGB4444(src, dst, n);
      break;
   case MESA_FORMAT_ARGB4444_REV:
      unpack_ubyte_ARGB4444_REV(src, dst, n);
      break;
   case MESA_FORMAT_RGBA5551:
      unpack_ubyte_RGBA5551(src, dst, n);
      break;
   case MESA_FORMAT_ARGB1555:
      unpack_ubyte_ARGB1555(src, dst, n);
      break;
   case MESA_FORMAT_ARGB1555_REV:
      unpack_ubyte_ARGB1555_REV(src, dst, n);
      break;
   case MESA_FORMAT_AL44:
      unpack_ubyte_AL44(src, dst, n);
      break;
   case MESA_FORMAT_AL88:
      unpack_ubyte_AL88(src, dst, n);
      break;
   case MESA_FORMAT_AL88_REV:
      unpack_ubyte_AL88_REV(src, dst, n);
      break;
   case MESA_FORMAT_RGB332:
      unpack_ubyte_RGB332(src, dst, n);
      break;
   case MESA_FORMAT_A8:
      unpack_ubyte_A8(src, dst, n);
      break;
   case MESA_FORMAT_L8:
      unpack_ubyte_L8(src, dst, n);
      break;
   case MESA_FORMAT_I8:
      unpack_ubyte_I8(src, dst, n);
      break;
   default:
      /* get float values, convert to ubyte */
      {
         GLfloat *tmp = (GLfloat *) malloc(n * 4 * sizeof(GLfloat));
         if (tmp) {
            GLuint i;
            _mesa_unpack_rgba_row(format, n, src, (GLfloat (*)[4]) tmp);
            for (i = 0; i < n; i++) {
               UNCLAMPED_FLOAT_TO_UBYTE(dst[i][0], tmp[i*4+0]);
               UNCLAMPED_FLOAT_TO_UBYTE(dst[i][1], tmp[i*4+1]);
               UNCLAMPED_FLOAT_TO_UBYTE(dst[i][2], tmp[i*4+2]);
               UNCLAMPED_FLOAT_TO_UBYTE(dst[i][3], tmp[i*4+3]);
            }
            free(tmp);
         }
      }
      break;
   }
}


/**********************************************************************/
/*  Unpack, returning GLuint colors                                   */
/**********************************************************************/

static void
unpack_int_rgba_RGBA_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   memcpy(dst, src, n * 4 * sizeof(GLuint));
}

static void
unpack_int_rgba_RGBA_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = src[i * 4 + 3];
   }
}

static void
unpack_int_rgba_RGBA_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = src[i * 4 + 3];
   }
}

static void
unpack_int_rgba_RGBA_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = src[i * 4 + 3];
   }
}

static void
unpack_int_rgba_RGBA_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = src[i * 4 + 3];
   }
}

static void
unpack_int_rgba_RGB_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 3 + 0];
      dst[i][1] = src[i * 3 + 1];
      dst[i][2] = src[i * 3 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RGB_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 3 + 0];
      dst[i][1] = src[i * 3 + 1];
      dst[i][2] = src[i * 3 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RGB_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 3 + 0];
      dst[i][1] = src[i * 3 + 1];
      dst[i][2] = src[i * 3 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RGB_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 3 + 0];
      dst[i][1] = src[i * 3 + 1];
      dst[i][2] = src[i * 3 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RGB_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 3 + 0];
      dst[i][1] = src[i * 3 + 1];
      dst[i][2] = src[i * 3 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_ALPHA_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = 0;
      dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_ALPHA_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = 0;
      dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_ALPHA_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = 0;
      dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_ALPHA_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = 0;
      dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_ALPHA_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = 0;
      dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_LUMINANCE_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_LUMINANCE_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_LUMINANCE_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_LUMINANCE_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_LUMINANCE_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i];
      dst[i][3] = 1;
   }
}


static void
unpack_int_rgba_LUMINANCE_ALPHA_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i * 2 + 0];
      dst[i][3] = src[i * 2 + 1];
   }
}

static void
unpack_int_rgba_LUMINANCE_ALPHA_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i * 2 + 0];
      dst[i][3] = src[i * 2 + 1];
   }
}

static void
unpack_int_rgba_LUMINANCE_ALPHA_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i * 2 + 0];
      dst[i][3] = src[i * 2 + 1];
   }
}

static void
unpack_int_rgba_LUMINANCE_ALPHA_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i * 2 + 0];
      dst[i][3] = src[i * 2 + 1];
   }
}

static void
unpack_int_rgba_LUMINANCE_ALPHA_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i * 2 + 0];
      dst[i][3] = src[i * 2 + 1];
   }
}

static void
unpack_int_rgba_INTENSITY_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_INTENSITY_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_INTENSITY_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_INTENSITY_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_INTENSITY_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = dst[i][3] = src[i];
   }
}

void
_mesa_unpack_uint_rgba_row(gl_format format, GLuint n,
                           const void *src, GLuint dst[][4])
{
   switch (format) {
      /* Since there won't be any sign extension happening, there's no need to
       * make separate paths for 32-bit-to-32-bit integer unpack.
       */
   case MESA_FORMAT_RGBA_UINT32:
   case MESA_FORMAT_RGBA_INT32:
      unpack_int_rgba_RGBA_UINT32(src, dst, n);
      break;

   case MESA_FORMAT_RGBA_UINT16:
      unpack_int_rgba_RGBA_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_RGBA_INT16:
      unpack_int_rgba_RGBA_INT16(src, dst, n);
      break;

   case MESA_FORMAT_RGBA_UINT8:
      unpack_int_rgba_RGBA_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_RGBA_INT8:
      unpack_int_rgba_RGBA_INT8(src, dst, n);
      break;

   case MESA_FORMAT_RGB_UINT32:
   case MESA_FORMAT_RGB_INT32:
      unpack_int_rgba_RGB_UINT32(src, dst, n);
      break;

   case MESA_FORMAT_RGB_UINT16:
      unpack_int_rgba_RGB_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_RGB_INT16:
      unpack_int_rgba_RGB_INT16(src, dst, n);
      break;

   case MESA_FORMAT_RGB_UINT8:
      unpack_int_rgba_RGB_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_RGB_INT8:
      unpack_int_rgba_RGB_INT8(src, dst, n);
      break;

   case MESA_FORMAT_ALPHA_UINT32:
   case MESA_FORMAT_ALPHA_INT32:
      unpack_int_rgba_ALPHA_UINT32(src, dst, n);
      break;

   case MESA_FORMAT_ALPHA_UINT16:
      unpack_int_rgba_ALPHA_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_ALPHA_INT16:
      unpack_int_rgba_ALPHA_INT16(src, dst, n);
      break;

   case MESA_FORMAT_ALPHA_UINT8:
      unpack_int_rgba_ALPHA_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_ALPHA_INT8:
      unpack_int_rgba_ALPHA_INT8(src, dst, n);
      break;

   case MESA_FORMAT_LUMINANCE_UINT32:
   case MESA_FORMAT_LUMINANCE_INT32:
      unpack_int_rgba_LUMINANCE_UINT32(src, dst, n);
      break;
   case MESA_FORMAT_LUMINANCE_UINT16:
      unpack_int_rgba_LUMINANCE_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_LUMINANCE_INT16:
      unpack_int_rgba_LUMINANCE_INT16(src, dst, n);
      break;

   case MESA_FORMAT_LUMINANCE_UINT8:
      unpack_int_rgba_LUMINANCE_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_LUMINANCE_INT8:
      unpack_int_rgba_LUMINANCE_INT8(src, dst, n);
      break;

   case MESA_FORMAT_LUMINANCE_ALPHA_UINT32:
   case MESA_FORMAT_LUMINANCE_ALPHA_INT32:
      unpack_int_rgba_LUMINANCE_ALPHA_UINT32(src, dst, n);
      break;

   case MESA_FORMAT_LUMINANCE_ALPHA_UINT16:
      unpack_int_rgba_LUMINANCE_ALPHA_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_LUMINANCE_ALPHA_INT16:
      unpack_int_rgba_LUMINANCE_ALPHA_INT16(src, dst, n);
      break;

   case MESA_FORMAT_LUMINANCE_ALPHA_UINT8:
      unpack_int_rgba_LUMINANCE_ALPHA_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_LUMINANCE_ALPHA_INT8:
      unpack_int_rgba_LUMINANCE_ALPHA_INT8(src, dst, n);
      break;

   case MESA_FORMAT_INTENSITY_UINT32:
   case MESA_FORMAT_INTENSITY_INT32:
      unpack_int_rgba_INTENSITY_UINT32(src, dst, n);
      break;

   case MESA_FORMAT_INTENSITY_UINT16:
      unpack_int_rgba_INTENSITY_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_INTENSITY_INT16:
      unpack_int_rgba_INTENSITY_INT16(src, dst, n);
      break;

   case MESA_FORMAT_INTENSITY_UINT8:
      unpack_int_rgba_INTENSITY_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_INTENSITY_INT8:
      unpack_int_rgba_INTENSITY_INT8(src, dst, n);
      break;

   default:
      _mesa_problem(NULL, "%s: bad format %s", __FUNCTION__,
                    _mesa_get_format_name(format));
      return;
   }
}

/**
 * Unpack a 2D rect of pixels returning float RGBA colors.
 * \param format  the source image format
 * \param src  start address of the source image
 * \param srcRowStride  source image row stride in bytes
 * \param dst  start address of the dest image
 * \param dstRowStride  dest image row stride in bytes
 * \param x  source image start X pos
 * \param y  source image start Y pos
 * \param width  width of rect region to convert
 * \param height  height of rect region to convert
 */
void
_mesa_unpack_rgba_block(gl_format format,
                        const void *src, GLint srcRowStride,
                        GLfloat dst[][4], GLint dstRowStride,
                        GLuint x, GLuint y, GLuint width, GLuint height)
{
   unpack_rgba_func unpack = get_unpack_rgba_function(format);
   const GLuint srcPixStride = _mesa_get_format_bytes(format);
   const GLuint dstPixStride = 4 * sizeof(GLfloat);
   const GLubyte *srcRow;
   GLubyte *dstRow;
   GLuint i;

   /* XXX needs to be fixed for compressed formats */

   srcRow = ((const GLubyte *) src) + srcRowStride * y + srcPixStride * x;
   dstRow = ((GLubyte *) dst) + dstRowStride * y + dstPixStride * x;

   for (i = 0; i < height; i++) {
      unpack(srcRow, (GLfloat (*)[4]) dstRow, width);

      dstRow += dstRowStride;
      srcRow += srcRowStride;
   }
}




typedef void (*unpack_float_z_func)(GLuint n, const void *src, GLfloat *dst);

static void
unpack_float_z_Z24_X8(GLuint n, const void *src, GLfloat *dst)
{
   /* only return Z, not stencil data */
   const GLuint *s = ((const GLuint *) src);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = (s[i] >> 8) * scale;
      ASSERT(dst[i] >= 0.0F);
      ASSERT(dst[i] <= 1.0F);
   }
}

static void
unpack_float_z_X8_Z24(GLuint n, const void *src, GLfloat *dst)
{
   /* only return Z, not stencil data */
   const GLuint *s = ((const GLuint *) src);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = (s[i] & 0x00ffffff) * scale;
      ASSERT(dst[i] >= 0.0F);
      ASSERT(dst[i] <= 1.0F);
   }
}

static void
unpack_float_z_Z16(GLuint n, const void *src, GLfloat *dst)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = s[i] * (1.0F / 65535.0F);
   }
}

static void
unpack_float_z_Z32(GLuint n, const void *src, GLfloat *dst)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = s[i] * (1.0F / 0xffffffff);
   }
}



/**
 * Unpack Z values.
 * The returned values will always be in the range [0.0, 1.0].
 */
void
_mesa_unpack_float_z_row(gl_format format, GLuint n,
                         const void *src, GLfloat *dst)
{
   unpack_float_z_func unpack;

   switch (format) {
   case MESA_FORMAT_Z24_X8:
      unpack = unpack_float_z_Z24_X8;
      break;
   case MESA_FORMAT_X8_Z24:
      unpack = unpack_float_z_X8_Z24;
      break;
   case MESA_FORMAT_Z16:
      unpack = unpack_float_z_Z16;
      break;
   case MESA_FORMAT_Z32:
      unpack = unpack_float_z_Z32;
      break;
   default:
      _mesa_problem(NULL, "bad format %s in _mesa_unpack_float_z_row",
                    _mesa_get_format_name(format));
      return;
   }

   unpack(n, src, dst);
}



typedef void (*unpack_uint_z_func)(const void *src, GLuint *dst, GLuint n);

static void
unpack_uint_z_Z24_X8(const void *src, GLuint *dst, GLuint n)
{
   /* only return Z, not stencil data */
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = (s[i] & 0xffffff00) | (s[i] >> 24);
   }
}

static void
unpack_uint_z_X8_Z24(const void *src, GLuint *dst, GLuint n)
{
   /* only return Z, not stencil data */
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = (s[i] << 8) | ((s[i] >> 16) & 0xff);
   }
}

static void
unpack_uint_z_Z16(const void *src, GLuint *dst, GLuint n)
{
   const GLushort *s = ((const GLushort *)src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = (s[i] << 16) | s[i];
   }
}

static void
unpack_uint_z_Z32(const void *src, GLuint *dst, GLuint n)
{
   memcpy(dst, src, n * sizeof(GLuint));
}


/**
 * Unpack Z values.
 * The returned values will always be in the range [0, 0xffffffff].
 */
void
_mesa_unpack_uint_z_row(gl_format format, GLuint n,
                        const void *src, GLuint *dst)
{
   unpack_uint_z_func unpack;
   const GLubyte *srcPtr = (GLubyte *) src;

   switch (format) {
   case MESA_FORMAT_Z24_X8:
      unpack = unpack_uint_z_Z24_X8;
      break;
   case MESA_FORMAT_X8_Z24:
      unpack = unpack_uint_z_X8_Z24;
      break;
   case MESA_FORMAT_Z16:
      unpack = unpack_uint_z_Z16;
      break;
   case MESA_FORMAT_Z32:
      unpack = unpack_uint_z_Z32;
      break;
   default:
      _mesa_problem(NULL, "bad format %s in _mesa_unpack_uint_z_row",
                    _mesa_get_format_name(format));
      return;
   }

   unpack(srcPtr, dst, n);
}


static void
unpack_ubyte_s_S8(const void *src, GLubyte *dst, GLuint n)
{
   memcpy(dst, src, n);
}

void
_mesa_unpack_ubyte_stencil_row(gl_format format, GLuint n,
			       const void *src, GLubyte *dst)
{
   switch (format) {
   case MESA_FORMAT_S8:
      unpack_ubyte_s_S8(src, dst, n);
      break;
   default:
      _mesa_problem(NULL, "bad format %s in _mesa_unpack_ubyte_s_row",
                    _mesa_get_format_name(format));
      return;
   }
}
