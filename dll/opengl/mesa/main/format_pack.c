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


/**
 * Color, depth, stencil packing functions.
 * Used to pack basic color, depth and stencil formats to specific
 * hardware formats.
 *
 * There are both per-pixel and per-row packing functions:
 * - The former will be used by swrast to write values to the color, depth,
 *   stencil buffers when drawing points, lines and masked spans.
 * - The later will be used for image-oriented functions like glDrawPixels,
 *   glAccum, and glTexImage.
 */

#include <precomp.h>

typedef void (*pack_ubyte_rgba_row_func)(GLuint n,
                                         const GLubyte src[][4], void *dst);

typedef void (*pack_float_rgba_row_func)(GLuint n,
                                         const GLfloat src[][4], void *dst);

/*
 * MESA_FORMAT_RGBA8888
 */

static void
pack_ubyte_RGBA8888(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = PACK_COLOR_8888(src[RCOMP], src[GCOMP], src[BCOMP], src[ACOMP]);
}

static void
pack_float_RGBA8888(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_RGBA8888(v, dst);
}

static void
pack_row_ubyte_RGBA8888(GLuint n, const GLubyte src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i] = PACK_COLOR_8888(src[i][RCOMP], src[i][GCOMP],
                             src[i][BCOMP], src[i][ACOMP]);
   }
}

static void
pack_row_float_RGBA8888(GLuint n, const GLfloat src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_RGBA8888(v, d + i);
   }
}



/*
 * MESA_FORMAT_RGBA8888_REV
 */

static void
pack_ubyte_RGBA8888_REV(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = PACK_COLOR_8888(src[ACOMP], src[BCOMP], src[GCOMP], src[RCOMP]);
}

static void
pack_float_RGBA8888_REV(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_RGBA8888_REV(v, dst);
}

static void
pack_row_ubyte_RGBA8888_REV(GLuint n, const GLubyte src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i] = PACK_COLOR_8888(src[i][ACOMP], src[i][BCOMP],
                             src[i][GCOMP], src[i][RCOMP]);
   }
}

static void
pack_row_float_RGBA8888_REV(GLuint n, const GLfloat src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_RGBA8888_REV(v, d + i);
   }
}


/*
 * MESA_FORMAT_ARGB8888
 */

static void
pack_ubyte_ARGB8888(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = PACK_COLOR_8888(src[ACOMP], src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_ARGB8888(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_ARGB8888(v, dst);
}

static void
pack_row_ubyte_ARGB8888(GLuint n, const GLubyte src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i] = PACK_COLOR_8888(src[i][ACOMP], src[i][RCOMP],
                             src[i][GCOMP], src[i][BCOMP]);
   }
}

static void
pack_row_float_ARGB8888(GLuint n, const GLfloat src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_ARGB8888(v, d + i);
   }
}


/*
 * MESA_FORMAT_ARGB8888_REV
 */

static void
pack_ubyte_ARGB8888_REV(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = PACK_COLOR_8888(src[BCOMP], src[GCOMP], src[RCOMP], src[ACOMP]);
}

static void
pack_float_ARGB8888_REV(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_ARGB8888_REV(v, dst);
}

static void
pack_row_ubyte_ARGB8888_REV(GLuint n, const GLubyte src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i] = PACK_COLOR_8888(src[i][BCOMP], src[i][GCOMP],
                             src[i][RCOMP], src[i][ACOMP]);
   }
}

static void
pack_row_float_ARGB8888_REV(GLuint n, const GLfloat src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_ARGB8888_REV(v, d + i);
   }
}


/*
 * MESA_FORMAT_XRGB8888
 */

static void
pack_ubyte_XRGB8888(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = PACK_COLOR_8888(0x0, src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_XRGB8888(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_XRGB8888(v, dst);
}

static void
pack_row_ubyte_XRGB8888(GLuint n, const GLubyte src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i] = PACK_COLOR_8888(0, src[i][RCOMP], src[i][GCOMP], src[i][BCOMP]);
   }
}

static void
pack_row_float_XRGB8888(GLuint n, const GLfloat src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_XRGB8888(v, d + i);
   }
}


/*
 * MESA_FORMAT_XRGB8888_REV
 */

static void
pack_ubyte_XRGB8888_REV(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = PACK_COLOR_8888(src[BCOMP], src[GCOMP], src[RCOMP], 0);
}

static void
pack_float_XRGB8888_REV(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_XRGB8888_REV(v, dst);
}

static void
pack_row_ubyte_XRGB8888_REV(GLuint n, const GLubyte src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i] = PACK_COLOR_8888(src[i][BCOMP], src[i][GCOMP], src[i][RCOMP], 0);
   }
}

static void
pack_row_float_XRGB8888_REV(GLuint n, const GLfloat src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_XRGB8888_REV(v, d + i);
   }
}


/*
 * MESA_FORMAT_RGB888
 */

static void
pack_ubyte_RGB888(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   d[2] = src[RCOMP];
   d[1] = src[GCOMP];
   d[0] = src[BCOMP];
}

static void
pack_float_RGB888(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   UNCLAMPED_FLOAT_TO_UBYTE(d[2], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(d[1], src[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(d[0], src[BCOMP]);
}

static void
pack_row_ubyte_RGB888(GLuint n, const GLubyte src[][4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i*3+2] = src[i][RCOMP];
      d[i*3+1] = src[i][GCOMP];
      d[i*3+0] = src[i][BCOMP];
   }
}

static void
pack_row_float_RGB888(GLuint n, const GLfloat src[][4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      d[i*3+2] = v[RCOMP];
      d[i*3+1] = v[GCOMP];
      d[i*3+0] = v[BCOMP];
   }
}


/*
 * MESA_FORMAT_BGR888
 */

static void
pack_ubyte_BGR888(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   d[2] = src[BCOMP];
   d[1] = src[GCOMP];
   d[0] = src[RCOMP];
}

static void
pack_float_BGR888(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   UNCLAMPED_FLOAT_TO_UBYTE(d[2], src[BCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(d[1], src[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(d[0], src[RCOMP]);
}

static void
pack_row_ubyte_BGR888(GLuint n, const GLubyte src[][4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i*3+2] = src[i][BCOMP];
      d[i*3+1] = src[i][GCOMP];
      d[i*3+0] = src[i][RCOMP];
   }
}

static void
pack_row_float_BGR888(GLuint n, const GLfloat src[][4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      d[i*3+2] = v[BCOMP];
      d[i*3+1] = v[GCOMP];
      d[i*3+0] = v[RCOMP];
   }
}


/*
 * MESA_FORMAT_RGB565
 */

static void
pack_ubyte_RGB565(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_565(src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_RGB565(const GLfloat src[4], void *dst)
{
   GLubyte v[3];
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], src[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], src[BCOMP]);
   pack_ubyte_RGB565(v, dst);
}

static void
pack_row_ubyte_RGB565(GLuint n, const GLubyte src[][4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      pack_ubyte_RGB565(src[i], d + i);
   }
}

static void
pack_row_float_RGB565(GLuint n, const GLfloat src[][4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_RGB565(v, d + i);
   }
}


/*
 * MESA_FORMAT_RGB565_REV
 */

static void
pack_ubyte_RGB565_REV(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_565_REV(src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_RGB565_REV(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLubyte r, g, b;
   UNCLAMPED_FLOAT_TO_UBYTE(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, src[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(b, src[BCOMP]);
   *d = PACK_COLOR_565_REV(r, g, b);
}

static void
pack_row_ubyte_RGB565_REV(GLuint n, const GLubyte src[][4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      pack_ubyte_RGB565_REV(src[i], d + i);
   }
}

static void
pack_row_float_RGB565_REV(GLuint n, const GLfloat src[][4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_RGB565_REV(v, d + i);
   }
}


/*
 * MESA_FORMAT_ARGB4444
 */

static void
pack_ubyte_ARGB4444(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_4444(src[ACOMP], src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_ARGB4444(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_ARGB4444(v, dst);
}

/* use fallback row packing functions */


/*
 * MESA_FORMAT_ARGB4444_REV
 */

static void
pack_ubyte_ARGB4444_REV(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_4444(src[GCOMP], src[BCOMP], src[ACOMP], src[RCOMP]);
}

static void
pack_float_ARGB4444_REV(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_ARGB4444_REV(v, dst);
}

/* use fallback row packing functions */


/*
 * MESA_FORMAT_RGBA5551
 */

static void
pack_ubyte_RGBA5551(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_5551(src[RCOMP], src[GCOMP], src[BCOMP], src[ACOMP]);
}

static void
pack_float_RGBA5551(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_RGBA5551(v, dst);
}

/* use fallback row packing functions */


/*
 * MESA_FORMAT_ARGB1555
 */

static void
pack_ubyte_ARGB1555(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_1555(src[ACOMP], src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_ARGB1555(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_ARGB1555(v, dst);
}


/* MESA_FORMAT_ARGB1555_REV */

static void
pack_ubyte_ARGB1555_REV(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst), tmp;
   tmp = PACK_COLOR_1555(src[ACOMP], src[RCOMP], src[GCOMP], src[BCOMP]);
   *d = (tmp >> 8) | (tmp << 8);
}

static void
pack_float_ARGB1555_REV(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_ARGB1555_REV(v, dst);
}


/* MESA_FORMAT_AL44 */

static void
pack_ubyte_AL44(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = PACK_COLOR_44(src[ACOMP], src[RCOMP]);
}

static void
pack_float_AL44(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], src[ACOMP]);
   pack_ubyte_AL44(v, dst);
}


/* MESA_FORMAT_AL88 */

static void
pack_ubyte_AL88(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_88(src[ACOMP], src[RCOMP]);
}

static void
pack_float_AL88(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], src[ACOMP]);
   pack_ubyte_AL88(v, dst);
}


/* MESA_FORMAT_AL88_REV */

static void
pack_ubyte_AL88_REV(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_88(src[RCOMP], src[ACOMP]);
}

static void
pack_float_AL88_REV(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], src[ACOMP]);
   pack_ubyte_AL88_REV(v, dst);
}


/* MESA_FORMAT_AL1616 */

static void
pack_ubyte_AL1616(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort l = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort a = UBYTE_TO_USHORT(src[ACOMP]);
   *d = PACK_COLOR_1616(a, l);
}

static void
pack_float_AL1616(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort l, a;
   UNCLAMPED_FLOAT_TO_USHORT(l, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(a, src[ACOMP]);
   *d = PACK_COLOR_1616(a, l);
}


/* MESA_FORMAT_AL1616_REV */

static void
pack_ubyte_AL1616_REV(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort l = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort a = UBYTE_TO_USHORT(src[ACOMP]);
   *d = PACK_COLOR_1616(l, a);
}

static void
pack_float_AL1616_REV(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort l, a;
   UNCLAMPED_FLOAT_TO_USHORT(l, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(a, src[ACOMP]);
   *d = PACK_COLOR_1616(l, a);
}


/* MESA_FORMAT_RGB332 */

static void
pack_ubyte_RGB332(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = PACK_COLOR_332(src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_RGB332(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], src[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], src[BCOMP]);
   pack_ubyte_RGB332(v, dst);
}


/* MESA_FORMAT_A8 */

static void
pack_ubyte_A8(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = src[ACOMP];
}

static void
pack_float_A8(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   UNCLAMPED_FLOAT_TO_UBYTE(d[0], src[ACOMP]);
}


/* MESA_FORMAT_A16 */

static void
pack_ubyte_A16(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = UBYTE_TO_USHORT(src[ACOMP]);
}

static void
pack_float_A16(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   UNCLAMPED_FLOAT_TO_USHORT(d[0], src[ACOMP]);
}


/* MESA_FORMAT_L8 */

static void
pack_ubyte_L8(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = src[RCOMP];
}

static void
pack_float_L8(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   UNCLAMPED_FLOAT_TO_UBYTE(d[0], src[RCOMP]);
}


/* MESA_FORMAT_L16 */

static void
pack_ubyte_L16(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = UBYTE_TO_USHORT(src[RCOMP]);
}

static void
pack_float_L16(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   UNCLAMPED_FLOAT_TO_USHORT(d[0], src[RCOMP]);
}


/* MESA_FORMAT_YCBCR */

static void
pack_ubyte_YCBCR(const GLubyte src[4], void *dst)
{
   /* todo */
}

static void
pack_float_YCBCR(const GLfloat src[4], void *dst)
{
   /* todo */
}


/* MESA_FORMAT_YCBCR_REV */

static void
pack_ubyte_YCBCR_REV(const GLubyte src[4], void *dst)
{
   /* todo */
}

static void
pack_float_YCBCR_REV(const GLfloat src[4], void *dst)
{
   /* todo */
}


/* MESA_FORMAT_RGBA_16 */

static void
pack_ubyte_RGBA_16(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   d[0] = UBYTE_TO_USHORT(src[RCOMP]);
   d[1] = UBYTE_TO_USHORT(src[GCOMP]);
   d[2] = UBYTE_TO_USHORT(src[BCOMP]);
   d[3] = UBYTE_TO_USHORT(src[ACOMP]);
}

static void
pack_float_RGBA_16(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   UNCLAMPED_FLOAT_TO_USHORT(d[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(d[1], src[GCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(d[2], src[BCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(d[3], src[ACOMP]);
}

/*
 * MESA_FORMAT_SIGNED_RGBA_16
 */

static void
pack_float_SIGNED_RGBA_16(const GLfloat src[4], void *dst)
{
   GLshort *d = (GLshort *) dst;
   d[0] = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
   d[1] = FLOAT_TO_SHORT(CLAMP(src[GCOMP], -1.0f, 1.0f));
   d[2] = FLOAT_TO_SHORT(CLAMP(src[BCOMP], -1.0f, 1.0f));
   d[3] = FLOAT_TO_SHORT(CLAMP(src[ACOMP], -1.0f, 1.0f));
}



/**
 * Return a function that can pack a GLubyte rgba[4] color.
 */
gl_pack_ubyte_rgba_func
_mesa_get_pack_ubyte_rgba_function(gl_format format)
{
   static gl_pack_ubyte_rgba_func table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      memset(table, 0, sizeof(table));

      table[MESA_FORMAT_NONE] = NULL;

      table[MESA_FORMAT_RGBA8888] = pack_ubyte_RGBA8888;
      table[MESA_FORMAT_RGBA8888_REV] = pack_ubyte_RGBA8888_REV;
      table[MESA_FORMAT_ARGB8888] = pack_ubyte_ARGB8888;
      table[MESA_FORMAT_ARGB8888_REV] = pack_ubyte_ARGB8888_REV;
      table[MESA_FORMAT_RGBX8888] = pack_ubyte_RGBA8888; /* reused */
      table[MESA_FORMAT_RGBX8888_REV] = pack_ubyte_RGBA8888_REV; /* reused */
      table[MESA_FORMAT_XRGB8888] = pack_ubyte_XRGB8888;
      table[MESA_FORMAT_XRGB8888_REV] = pack_ubyte_XRGB8888_REV;
      table[MESA_FORMAT_RGB888] = pack_ubyte_RGB888;
      table[MESA_FORMAT_BGR888] = pack_ubyte_BGR888;
      table[MESA_FORMAT_RGB565] = pack_ubyte_RGB565;
      table[MESA_FORMAT_RGB565_REV] = pack_ubyte_RGB565_REV;
      table[MESA_FORMAT_ARGB4444] = pack_ubyte_ARGB4444;
      table[MESA_FORMAT_ARGB4444_REV] = pack_ubyte_ARGB4444_REV;
      table[MESA_FORMAT_RGBA5551] = pack_ubyte_RGBA5551;
      table[MESA_FORMAT_ARGB1555] = pack_ubyte_ARGB1555;
      table[MESA_FORMAT_ARGB1555_REV] = pack_ubyte_ARGB1555_REV;
      table[MESA_FORMAT_AL44] = pack_ubyte_AL44;
      table[MESA_FORMAT_AL88] = pack_ubyte_AL88;
      table[MESA_FORMAT_AL88_REV] = pack_ubyte_AL88_REV;
      table[MESA_FORMAT_AL1616] = pack_ubyte_AL1616;
      table[MESA_FORMAT_AL1616_REV] = pack_ubyte_AL1616_REV;
      table[MESA_FORMAT_RGB332] = pack_ubyte_RGB332;
      table[MESA_FORMAT_A8] = pack_ubyte_A8;
      table[MESA_FORMAT_A16] = pack_ubyte_A16;
      table[MESA_FORMAT_L8] = pack_ubyte_L8;
      table[MESA_FORMAT_L16] = pack_ubyte_L16;
      table[MESA_FORMAT_I8] = pack_ubyte_L8; /* reuse pack_ubyte_L8 */
      table[MESA_FORMAT_I16] = pack_ubyte_L16; /* reuse pack_ubyte_L16 */
      table[MESA_FORMAT_YCBCR] = pack_ubyte_YCBCR;
      table[MESA_FORMAT_YCBCR_REV] = pack_ubyte_YCBCR_REV;

      /* should never convert RGBA to these formats */
      table[MESA_FORMAT_Z16] = NULL;
      table[MESA_FORMAT_X8_Z24] = NULL;
      table[MESA_FORMAT_Z24_X8] = NULL;
      table[MESA_FORMAT_Z32] = NULL;
      table[MESA_FORMAT_S8] = NULL;

      /* n/a */
      table[MESA_FORMAT_RGBA_INT8] = NULL; /* pack_ubyte_RGBA_INT8 */
      table[MESA_FORMAT_RGBA_INT16] = NULL; /* pack_ubyte_RGBA_INT16 */
      table[MESA_FORMAT_RGBA_INT32] = NULL; /* pack_ubyte_RGBA_INT32 */
      table[MESA_FORMAT_RGBA_UINT8] = NULL; /* pack_ubyte_RGBA_UINT8 */
      table[MESA_FORMAT_RGBA_UINT16] = NULL; /* pack_ubyte_RGBA_UINT16 */
      table[MESA_FORMAT_RGBA_UINT32] = NULL; /* pack_ubyte_RGBA_UINT32 */

      table[MESA_FORMAT_RGBA_16] = pack_ubyte_RGBA_16;

      table[MESA_FORMAT_SIGNED_RGBA_16] = NULL;


      table[MESA_FORMAT_RGBA_16] = pack_ubyte_RGBA_16;

      initialized = GL_TRUE;
   }

   return table[format];
}



/**
 * Return a function that can pack a GLfloat rgba[4] color.
 */
gl_pack_float_rgba_func
_mesa_get_pack_float_rgba_function(gl_format format)
{
   static gl_pack_float_rgba_func table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      memset(table, 0, sizeof(table));

      table[MESA_FORMAT_NONE] = NULL;

      table[MESA_FORMAT_RGBA8888] = pack_float_RGBA8888;
      table[MESA_FORMAT_RGBA8888_REV] = pack_float_RGBA8888_REV;
      table[MESA_FORMAT_ARGB8888] = pack_float_ARGB8888;
      table[MESA_FORMAT_ARGB8888_REV] = pack_float_ARGB8888_REV;
      table[MESA_FORMAT_RGBX8888] = pack_float_RGBA8888; /* reused */
      table[MESA_FORMAT_RGBX8888_REV] = pack_float_RGBA8888_REV; /* reused */
      table[MESA_FORMAT_XRGB8888] = pack_float_XRGB8888;
      table[MESA_FORMAT_XRGB8888_REV] = pack_float_XRGB8888_REV;
      table[MESA_FORMAT_RGB888] = pack_float_RGB888;
      table[MESA_FORMAT_BGR888] = pack_float_BGR888;
      table[MESA_FORMAT_RGB565] = pack_float_RGB565;
      table[MESA_FORMAT_RGB565_REV] = pack_float_RGB565_REV;
      table[MESA_FORMAT_ARGB4444] = pack_float_ARGB4444;
      table[MESA_FORMAT_ARGB4444_REV] = pack_float_ARGB4444_REV;
      table[MESA_FORMAT_RGBA5551] = pack_float_RGBA5551;
      table[MESA_FORMAT_ARGB1555] = pack_float_ARGB1555;
      table[MESA_FORMAT_ARGB1555_REV] = pack_float_ARGB1555_REV;

      table[MESA_FORMAT_AL44] = pack_float_AL44;
      table[MESA_FORMAT_AL88] = pack_float_AL88;
      table[MESA_FORMAT_AL88_REV] = pack_float_AL88_REV;
      table[MESA_FORMAT_AL1616] = pack_float_AL1616;
      table[MESA_FORMAT_AL1616_REV] = pack_float_AL1616_REV;
      table[MESA_FORMAT_RGB332] = pack_float_RGB332;
      table[MESA_FORMAT_A8] = pack_float_A8;
      table[MESA_FORMAT_A16] = pack_float_A16;
      table[MESA_FORMAT_L8] = pack_float_L8;
      table[MESA_FORMAT_L16] = pack_float_L16;
      table[MESA_FORMAT_I8] = pack_float_L8; /* reuse pack_float_L8 */
      table[MESA_FORMAT_I16] = pack_float_L16; /* reuse pack_float_L16 */
      table[MESA_FORMAT_YCBCR] = pack_float_YCBCR;
      table[MESA_FORMAT_YCBCR_REV] = pack_float_YCBCR_REV;

      /* should never convert RGBA to these formats */
      table[MESA_FORMAT_Z16] = NULL;
      table[MESA_FORMAT_X8_Z24] = NULL;
      table[MESA_FORMAT_Z24_X8] = NULL;
      table[MESA_FORMAT_Z32] = NULL;
      table[MESA_FORMAT_S8] = NULL;

      /* n/a */
      table[MESA_FORMAT_RGBA_INT8] = NULL;
      table[MESA_FORMAT_RGBA_INT16] = NULL;
      table[MESA_FORMAT_RGBA_INT32] = NULL;
      table[MESA_FORMAT_RGBA_UINT8] = NULL;
      table[MESA_FORMAT_RGBA_UINT16] = NULL;
      table[MESA_FORMAT_RGBA_UINT32] = NULL;

      table[MESA_FORMAT_RGBA_16] = pack_float_RGBA_16;

      table[MESA_FORMAT_SIGNED_RGBA_16] = pack_float_SIGNED_RGBA_16;


      initialized = GL_TRUE;
   }

   return table[format];
}



static pack_float_rgba_row_func
get_pack_float_rgba_row_function(gl_format format)
{
   static pack_float_rgba_row_func table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      /* We don't need a special row packing function for each format.
       * There's a generic fallback which uses a per-pixel packing function.
       */
      memset(table, 0, sizeof(table));

      table[MESA_FORMAT_RGBA8888] = pack_row_float_RGBA8888;
      table[MESA_FORMAT_RGBA8888_REV] = pack_row_float_RGBA8888_REV;
      table[MESA_FORMAT_ARGB8888] = pack_row_float_ARGB8888;
      table[MESA_FORMAT_ARGB8888_REV] = pack_row_float_ARGB8888_REV;
      table[MESA_FORMAT_RGBX8888] = pack_row_float_RGBA8888; /* reused */
      table[MESA_FORMAT_RGBX8888_REV] = pack_row_float_RGBA8888_REV; /* reused */
      table[MESA_FORMAT_XRGB8888] = pack_row_float_XRGB8888;
      table[MESA_FORMAT_XRGB8888_REV] = pack_row_float_XRGB8888_REV;
      table[MESA_FORMAT_RGB888] = pack_row_float_RGB888;
      table[MESA_FORMAT_BGR888] = pack_row_float_BGR888;
      table[MESA_FORMAT_RGB565] = pack_row_float_RGB565;
      table[MESA_FORMAT_RGB565_REV] = pack_row_float_RGB565_REV;

      initialized = GL_TRUE;
   }

   return table[format];
}



static pack_ubyte_rgba_row_func
get_pack_ubyte_rgba_row_function(gl_format format)
{
   static pack_ubyte_rgba_row_func table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      /* We don't need a special row packing function for each format.
       * There's a generic fallback which uses a per-pixel packing function.
       */
      memset(table, 0, sizeof(table));

      table[MESA_FORMAT_RGBA8888] = pack_row_ubyte_RGBA8888;
      table[MESA_FORMAT_RGBA8888_REV] = pack_row_ubyte_RGBA8888_REV;
      table[MESA_FORMAT_ARGB8888] = pack_row_ubyte_ARGB8888;
      table[MESA_FORMAT_ARGB8888_REV] = pack_row_ubyte_ARGB8888_REV;
      table[MESA_FORMAT_RGBX8888] = pack_row_ubyte_RGBA8888; /* reused */
      table[MESA_FORMAT_RGBX8888_REV] = pack_row_ubyte_RGBA8888_REV; /* reused */
      table[MESA_FORMAT_XRGB8888] = pack_row_ubyte_XRGB8888;
      table[MESA_FORMAT_XRGB8888_REV] = pack_row_ubyte_XRGB8888_REV;
      table[MESA_FORMAT_RGB888] = pack_row_ubyte_RGB888;
      table[MESA_FORMAT_BGR888] = pack_row_ubyte_BGR888;
      table[MESA_FORMAT_RGB565] = pack_row_ubyte_RGB565;
      table[MESA_FORMAT_RGB565_REV] = pack_row_ubyte_RGB565_REV;

      initialized = GL_TRUE;
   }

   return table[format];
}



/**
 * Pack a row of GLfloat rgba[4] values to the destination.
 */
void
_mesa_pack_float_rgba_row(gl_format format, GLuint n,
                          const GLfloat src[][4], void *dst)
{
   pack_float_rgba_row_func packrow = get_pack_float_rgba_row_function(format);
   if (packrow) {
      /* use "fast" function */
      packrow(n, src, dst);
   }
   else {
      /* slower fallback */
      gl_pack_float_rgba_func pack = _mesa_get_pack_float_rgba_function(format);
      GLuint dstStride = _mesa_get_format_bytes(format);
      GLubyte *dstPtr = (GLubyte *) dst;
      GLuint i;

      assert(pack);
      if (!pack)
         return;

      for (i = 0; i < n; i++) {
         pack(src[i], dstPtr);
         dstPtr += dstStride;
      }
   }
}


/**
 * Pack a row of GLubyte rgba[4] values to the destination.
 */
void
_mesa_pack_ubyte_rgba_row(gl_format format, GLuint n,
                          const GLubyte src[][4], void *dst)
{
   pack_ubyte_rgba_row_func packrow = get_pack_ubyte_rgba_row_function(format);
   if (packrow) {
      /* use "fast" function */
      packrow(n, src, dst);
   }
   else {
      /* slower fallback */
      gl_pack_ubyte_rgba_func pack = _mesa_get_pack_ubyte_rgba_function(format);
      const GLuint stride = _mesa_get_format_bytes(format);
      GLubyte *d = ((GLubyte *) dst);
      GLuint i;

      assert(pack);
      if (!pack)
         return;

      for (i = 0; i < n; i++) {
         pack(src[i], d);
         d += stride;
      }
   }
}


/**
 ** Pack float Z pixels
 **/

static void
pack_float_z_Z24_S8(const GLfloat *src, void *dst)
{
   /* don't disturb the stencil values */
   GLuint *d = ((GLuint *) dst);
   const GLdouble scale = (GLdouble) 0xffffff;
   GLuint s = *d & 0xff;
   GLuint z = (GLuint) (*src * scale);
   assert(z <= 0xffffff);
   *d = (z << 8) | s;
}

static void
pack_float_z_S8_Z24(const GLfloat *src, void *dst)
{
   /* don't disturb the stencil values */
   GLuint *d = ((GLuint *) dst);
   const GLdouble scale = (GLdouble) 0xffffff;
   GLuint s = *d & 0xff000000;
   GLuint z = (GLuint) (*src * scale);
   assert(z <= 0xffffff);
   *d = s | z;
}

static void
pack_float_z_Z16(const GLfloat *src, void *dst)
{
   GLushort *d = ((GLushort *) dst);
   const GLfloat scale = (GLfloat) 0xffff;
   *d = (GLushort) (*src * scale);
}

static void
pack_float_z_Z32(const GLfloat *src, void *dst)
{
   GLuint *d = ((GLuint *) dst);
   const GLdouble scale = (GLdouble) 0xffffffff;
   *d = (GLuint) (*src * scale);
}

gl_pack_float_z_func
_mesa_get_pack_float_z_func(gl_format format)
{
   switch (format) {
   case MESA_FORMAT_Z24_X8:
      return pack_float_z_Z24_S8;
   case MESA_FORMAT_X8_Z24:
      return pack_float_z_S8_Z24;
   case MESA_FORMAT_Z16:
      return pack_float_z_Z16;
   case MESA_FORMAT_Z32:
      return pack_float_z_Z32;
   default:
      _mesa_problem(NULL,
                    "unexpected format in _mesa_get_pack_float_z_func()");
      return NULL;
   }
}



/**
 ** Pack uint Z pixels.  The incoming src value is always in
 ** the range [0, 2^32-1].
 **/

static void
pack_uint_z_Z24_S8(const GLuint *src, void *dst)
{
   /* don't disturb the stencil values */
   GLuint *d = ((GLuint *) dst);
   GLuint s = *d & 0xff;
   GLuint z = *src & 0xffffff00;
   *d = z | s;
}

static void
pack_uint_z_S8_Z24(const GLuint *src, void *dst)
{
   /* don't disturb the stencil values */
   GLuint *d = ((GLuint *) dst);
   GLuint s = *d & 0xff000000;
   GLuint z = *src >> 8;
   *d = s | z;
}

static void
pack_uint_z_Z16(const GLuint *src, void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = *src >> 16;
}

static void
pack_uint_z_Z32(const GLuint *src, void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = *src;
}

gl_pack_uint_z_func
_mesa_get_pack_uint_z_func(gl_format format)
{
   switch (format) {
   case MESA_FORMAT_Z24_X8:
      return pack_uint_z_Z24_S8;
   case MESA_FORMAT_X8_Z24:
      return pack_uint_z_S8_Z24;
   case MESA_FORMAT_Z16:
      return pack_uint_z_Z16;
   case MESA_FORMAT_Z32:
      return pack_uint_z_Z32;
   default:
      _mesa_problem(NULL, "unexpected format in _mesa_get_pack_uint_z_func()");
      return NULL;
   }
}


/**
 ** Pack ubyte stencil pixels
 **/

static void
pack_ubyte_stencil_S8(const GLubyte *src, void *dst)
{
   GLubyte *d = (GLubyte *) dst;
   *d = *src;
}


gl_pack_ubyte_stencil_func
_mesa_get_pack_ubyte_stencil_func(gl_format format)
{
   return pack_ubyte_stencil_S8;
}



void
_mesa_pack_float_z_row(gl_format format, GLuint n,
                       const GLfloat *src, void *dst)
{
   switch (format) {
   case MESA_FORMAT_Z24_X8:
      {
         /* don't disturb the stencil values */
         GLuint *d = ((GLuint *) dst);
         const GLdouble scale = (GLdouble) 0xffffff;
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = d[i] & 0xff;
            GLuint z = (GLuint) (src[i] * scale);
            assert(z <= 0xffffff);
            d[i] = (z << 8) | s;
         }
      }
      break;
   case MESA_FORMAT_X8_Z24:
      {
         /* don't disturb the stencil values */
         GLuint *d = ((GLuint *) dst);
         const GLdouble scale = (GLdouble) 0xffffff;
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = d[i] & 0xff000000;
            GLuint z = (GLuint) (src[i] * scale);
            assert(z <= 0xffffff);
            d[i] = s | z;
         }
      }
      break;
   case MESA_FORMAT_Z16:
      {
         GLushort *d = ((GLushort *) dst);
         const GLfloat scale = (GLfloat) 0xffff;
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i] = (GLushort) (src[i] * scale);
         }
      }
      break;
   case MESA_FORMAT_Z32:
      {
         GLuint *d = ((GLuint *) dst);
         const GLdouble scale = (GLdouble) 0xffffffff;
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i] = (GLuint) (src[i] * scale);
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format in _mesa_pack_float_z_row()");
   }
}


/**
 * The incoming Z values are always in the range [0, 0xffffffff].
 */
void
_mesa_pack_uint_z_row(gl_format format, GLuint n,
                      const GLuint *src, void *dst)
{
   switch (format) {
   case MESA_FORMAT_Z24_X8:
      {
         /* don't disturb the stencil values */
         GLuint *d = ((GLuint *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = d[i] & 0xff;
            GLuint z = src[i] & 0xffffff00;
            d[i] = z | s;
         }
      }
      break;
   case MESA_FORMAT_X8_Z24:
      {
         /* don't disturb the stencil values */
         GLuint *d = ((GLuint *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = d[i] & 0xff000000;
            GLuint z = src[i] >> 8;
            d[i] = s | z;
         }
      }
      break;
   case MESA_FORMAT_Z16:
      {
         GLushort *d = ((GLushort *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i] = src[i] >> 16;
         }
      }
      break;
   case MESA_FORMAT_Z32:
      memcpy(dst, src, n * sizeof(GLfloat));
      break;
   default:
      _mesa_problem(NULL, "unexpected format in _mesa_pack_uint_z_row()");
   }
}


void
_mesa_pack_ubyte_stencil_row(gl_format format, GLuint n,
                             const GLubyte *src, void *dst)
{
   memcpy(dst, src, n * sizeof(GLubyte));
}


/**
 * Convert a boolean color mask to a packed color where each channel of
 * the packed value at dst will be 0 or ~0 depending on the colorMask.
 */
void
_mesa_pack_colormask(gl_format format, const GLubyte colorMask[4], void *dst)
{
   GLfloat maskColor[4];

   switch (_mesa_get_format_datatype(format)) {
   case GL_UNSIGNED_NORMALIZED:
      /* simple: 1.0 will convert to ~0 in the right bit positions */
      maskColor[0] = colorMask[0] ? 1.0 : 0.0;
      maskColor[1] = colorMask[1] ? 1.0 : 0.0;
      maskColor[2] = colorMask[2] ? 1.0 : 0.0;
      maskColor[3] = colorMask[3] ? 1.0 : 0.0;
      _mesa_pack_float_rgba_row(format, 1,
                                (const GLfloat (*)[4]) maskColor, dst);
      break;
   case GL_SIGNED_NORMALIZED:
   case GL_FLOAT:
      /* These formats are harder because it's hard to know the floating
       * point values that will convert to ~0 for each color channel's bits.
       * This solution just generates a non-zero value for each color channel
       * then fixes up the non-zero values to be ~0.
       * Note: we'll need to add special case code if we ever have to deal
       * with formats with unequal color channel sizes, like R11_G11_B10.
       * We issue a warning below for channel sizes other than 8,16,32.
       */
      {
         GLuint bits = _mesa_get_format_max_bits(format); /* bits per chan */
         GLuint bytes = _mesa_get_format_bytes(format);
         GLuint i;

         /* this should put non-zero values into the channels of dst */
         maskColor[0] = colorMask[0] ? -1.0f : 0.0f;
         maskColor[1] = colorMask[1] ? -1.0f : 0.0f;
         maskColor[2] = colorMask[2] ? -1.0f : 0.0f;
         maskColor[3] = colorMask[3] ? -1.0f : 0.0f;
         _mesa_pack_float_rgba_row(format, 1,
                                   (const GLfloat (*)[4]) maskColor, dst);

         /* fix-up the dst channels by converting non-zero values to ~0 */
         if (bits == 8) {
            GLubyte *d = (GLubyte *) dst;
            for (i = 0; i < bytes; i++) {
               d[i] = d[i] ? 0xffff : 0x0;
            }
         }
         else if (bits == 16) {
            GLushort *d = (GLushort *) dst;
            for (i = 0; i < bytes / 2; i++) {
               d[i] = d[i] ? 0xffff : 0x0;
            }
         }
         else if (bits == 32) {
            GLuint *d = (GLuint *) dst;
            for (i = 0; i < bytes / 4; i++) {
               d[i] = d[i] ? 0xffffffffU : 0x0;
            }
         }
         else {
            _mesa_problem(NULL, "unexpected size in _mesa_pack_colormask()");
            return;
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format data type in gen_color_mask()");
      return;
   }
}
