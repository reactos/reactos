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


#include "colormac.h"
#include "format_pack.h"
#include "macros.h"
#include "../../gallium/auxiliary/util/u_format_rgb9e5.h"
#include "../../gallium/auxiliary/util/u_format_r11g11b10f.h"


typedef void (*pack_ubyte_rgba_row_func)(GLuint n,
                                         const GLubyte src[][4], void *dst);

typedef void (*pack_float_rgba_row_func)(GLuint n,
                                         const GLfloat src[][4], void *dst);



static inline GLfloat
linear_to_srgb(GLfloat cl)
{
   if (cl < 0.0f)
      return 0.0f;
   else if (cl < 0.0031308f)
      return 12.92f * cl;
   else if (cl < 1.0f)
      return 1.055f * powf(cl, 0.41666f) - 0.055f;
   else
      return 1.0f;
}


static inline GLubyte
linear_float_to_srgb_ubyte(GLfloat cl)
{
   GLubyte res = FLOAT_TO_UBYTE(linear_to_srgb(cl));
   return res;
}


static inline GLubyte
linear_ubyte_to_srgb_ubyte(GLubyte cl)
{
   GLubyte res = FLOAT_TO_UBYTE(linear_to_srgb(cl / 255.0f));
   return res;
}




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


/* MESA_FORMAT_R8 */

static void
pack_ubyte_R8(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = src[RCOMP];
}

static void
pack_float_R8(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   GLubyte r;
   UNCLAMPED_FLOAT_TO_UBYTE(r, src[RCOMP]);
   d[0] = r;
}


/* MESA_FORMAT_GR88 */

static void
pack_ubyte_GR88(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = PACK_COLOR_88(src[GCOMP], src[RCOMP]);
}

static void
pack_float_GR88(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLubyte r, g;
   UNCLAMPED_FLOAT_TO_UBYTE(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, src[GCOMP]);
   *d = PACK_COLOR_88(g, r);
}


/* MESA_FORMAT_RG88 */

static void
pack_ubyte_RG88(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = PACK_COLOR_88(src[RCOMP], src[GCOMP]);
}

static void
pack_float_RG88(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLubyte r, g;
   UNCLAMPED_FLOAT_TO_UBYTE(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, src[GCOMP]);
   *d = PACK_COLOR_88(r, g);
}


/* MESA_FORMAT_R16 */

static void
pack_ubyte_R16(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = UBYTE_TO_USHORT(src[RCOMP]);
}

static void
pack_float_R16(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   UNCLAMPED_FLOAT_TO_USHORT(d[0], src[RCOMP]);
}


/* MESA_FORMAT_RG1616 */

static void
pack_ubyte_RG1616(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort g = UBYTE_TO_USHORT(src[GCOMP]);
   *d = PACK_COLOR_1616(g, r);
}

static void
pack_float_RG1616(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r, g;
   UNCLAMPED_FLOAT_TO_USHORT(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(g, src[GCOMP]);
   *d = PACK_COLOR_1616(g, r);
}


/* MESA_FORMAT_RG1616_REV */

static void
pack_ubyte_RG1616_REV(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort g = UBYTE_TO_USHORT(src[GCOMP]);
   *d = PACK_COLOR_1616(r, g);
}


static void
pack_float_RG1616_REV(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r, g;
   UNCLAMPED_FLOAT_TO_USHORT(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(g, src[GCOMP]);
   *d = PACK_COLOR_1616(r, g);
}


/* MESA_FORMAT_ARGB2101010 */

static void
pack_ubyte_ARGB2101010(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort g = UBYTE_TO_USHORT(src[GCOMP]);
   GLushort b = UBYTE_TO_USHORT(src[BCOMP]);
   GLushort a = UBYTE_TO_USHORT(src[ACOMP]);
   *d = PACK_COLOR_2101010_US(a, r, g, b);
}

static void
pack_float_ARGB2101010(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r, g, b, a;
   UNCLAMPED_FLOAT_TO_USHORT(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(g, src[GCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(b, src[BCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(a, src[ACOMP]);
   *d = PACK_COLOR_2101010_US(a, r, g, b);
}


/* MESA_FORMAT_SRGB8 */

static void
pack_ubyte_SRGB8(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   d[2] = linear_ubyte_to_srgb_ubyte(src[RCOMP]);
   d[1] = linear_ubyte_to_srgb_ubyte(src[RCOMP]);
   d[0] = linear_ubyte_to_srgb_ubyte(src[RCOMP]);
}

static void
pack_float_SRGB8(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   d[2] = linear_float_to_srgb_ubyte(src[RCOMP]);
   d[1] = linear_float_to_srgb_ubyte(src[GCOMP]);
   d[0] = linear_float_to_srgb_ubyte(src[BCOMP]);
}


/* MESA_FORMAT_SRGBA8 */

static void
pack_ubyte_SRGBA8(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLubyte r = linear_ubyte_to_srgb_ubyte(src[RCOMP]);
   GLubyte g = linear_ubyte_to_srgb_ubyte(src[GCOMP]);
   GLubyte b = linear_ubyte_to_srgb_ubyte(src[BCOMP]);
   *d = PACK_COLOR_8888(r, g, b, src[ACOMP]);
}

static void
pack_float_SRGBA8(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLubyte r, g, b, a;
   r = linear_float_to_srgb_ubyte(src[RCOMP]);
   g = linear_float_to_srgb_ubyte(src[GCOMP]);
   b = linear_float_to_srgb_ubyte(src[BCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, src[ACOMP]);
   *d = PACK_COLOR_8888(r, g, b, a);
}


/* MESA_FORMAT_SARGB8 */

static void
pack_ubyte_SARGB8(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLubyte r = linear_ubyte_to_srgb_ubyte(src[RCOMP]);
   GLubyte g = linear_ubyte_to_srgb_ubyte(src[RCOMP]);
   GLubyte b = linear_ubyte_to_srgb_ubyte(src[RCOMP]);
   *d = PACK_COLOR_8888(src[ACOMP], r, g, b);
}

static void
pack_float_SARGB8(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLubyte r, g, b, a;
   r = linear_float_to_srgb_ubyte(src[RCOMP]);
   g = linear_float_to_srgb_ubyte(src[GCOMP]);
   b = linear_float_to_srgb_ubyte(src[BCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, src[ACOMP]);
   *d = PACK_COLOR_8888(a, r, g, b);
}


/* MESA_FORMAT_SL8 */

static void
pack_ubyte_SL8(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = linear_ubyte_to_srgb_ubyte(src[RCOMP]);
}

static void
pack_float_SL8(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   GLubyte l = linear_float_to_srgb_ubyte(src[RCOMP]);
   *d = l;
}


/* MESA_FORMAT_SLA8 */

static void
pack_ubyte_SLA8(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLubyte l = linear_ubyte_to_srgb_ubyte(src[RCOMP]);
   *d = PACK_COLOR_88(src[ACOMP], l);
}

static void
pack_float_SLA8(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLubyte a, l = linear_float_to_srgb_ubyte(src[RCOMP]);
   CLAMPED_FLOAT_TO_UBYTE(a, src[ACOMP]);
   *d = PACK_COLOR_88(a, l);
}


/* MESA_FORMAT_RGBA_FLOAT32 */

static void
pack_ubyte_RGBA_FLOAT32(const GLubyte src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = UBYTE_TO_FLOAT(src[0]);
   d[1] = UBYTE_TO_FLOAT(src[1]);
   d[2] = UBYTE_TO_FLOAT(src[2]);
   d[3] = UBYTE_TO_FLOAT(src[3]);
}

static void
pack_float_RGBA_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[0];
   d[1] = src[1];
   d[2] = src[2];
   d[3] = src[3];
}


/* MESA_FORMAT_RGBA_FLOAT16 */

static void
pack_ubyte_RGBA_FLOAT16(const GLubyte src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[0]));
   d[1] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[1]));
   d[2] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[2]));
   d[3] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[3]));
}

static void
pack_float_RGBA_FLOAT16(const GLfloat src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(src[0]);
   d[1] = _mesa_float_to_half(src[1]);
   d[2] = _mesa_float_to_half(src[2]);
   d[3] = _mesa_float_to_half(src[3]);
}


/* MESA_FORMAT_RGB_FLOAT32 */

static void
pack_ubyte_RGB_FLOAT32(const GLubyte src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = UBYTE_TO_FLOAT(src[0]);
   d[1] = UBYTE_TO_FLOAT(src[1]);
   d[2] = UBYTE_TO_FLOAT(src[2]);
}

static void
pack_float_RGB_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[0];
   d[1] = src[1];
   d[2] = src[2];
}


/* MESA_FORMAT_RGB_FLOAT16 */

static void
pack_ubyte_RGB_FLOAT16(const GLubyte src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[0]));
   d[1] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[1]));
   d[2] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[2]));
}

static void
pack_float_RGB_FLOAT16(const GLfloat src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(src[0]);
   d[1] = _mesa_float_to_half(src[1]);
   d[2] = _mesa_float_to_half(src[2]);
}


/* MESA_FORMAT_ALPHA_FLOAT32 */

static void
pack_ubyte_ALPHA_FLOAT32(const GLubyte src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = UBYTE_TO_FLOAT(src[ACOMP]);
}

static void
pack_float_ALPHA_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[ACOMP];
}


/* MESA_FORMAT_ALPHA_FLOAT16 */

static void
pack_ubyte_ALPHA_FLOAT16(const GLubyte src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[ACOMP]));
}

static void
pack_float_ALPHA_FLOAT16(const GLfloat src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(src[ACOMP]);
}


/* MESA_FORMAT_LUMINANCE_FLOAT32 (and INTENSITY_FLOAT32, R_FLOAT32) */

static void
pack_ubyte_LUMINANCE_FLOAT32(const GLubyte src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = UBYTE_TO_FLOAT(src[RCOMP]);
}

static void
pack_float_LUMINANCE_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[RCOMP];
}


/* MESA_FORMAT_LUMINANCE_FLOAT16 (and INTENSITY_FLOAT16, R_FLOAT32) */

static void
pack_ubyte_LUMINANCE_FLOAT16(const GLubyte src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[RCOMP]));
}

static void
pack_float_LUMINANCE_FLOAT16(const GLfloat src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(src[RCOMP]);
}


/* MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32 */

static void
pack_ubyte_LUMINANCE_ALPHA_FLOAT32(const GLubyte src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = UBYTE_TO_FLOAT(src[RCOMP]);
   d[1] = UBYTE_TO_FLOAT(src[ACOMP]);
}

static void
pack_float_LUMINANCE_ALPHA_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[RCOMP];
   d[1] = src[ACOMP];
}


/* MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16 */

static void
pack_ubyte_LUMINANCE_ALPHA_FLOAT16(const GLubyte src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[RCOMP]));
   d[1] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[ACOMP]));
}

static void
pack_float_LUMINANCE_ALPHA_FLOAT16(const GLfloat src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(src[RCOMP]);
   d[1] = _mesa_float_to_half(src[ACOMP]);
}


/* MESA_FORMAT_RG_FLOAT32 */

static void
pack_ubyte_RG_FLOAT32(const GLubyte src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = UBYTE_TO_FLOAT(src[RCOMP]);
   d[1] = UBYTE_TO_FLOAT(src[GCOMP]);
}

static void
pack_float_RG_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[RCOMP];
   d[1] = src[GCOMP];
}


/* MESA_FORMAT_RG_FLOAT16 */

static void
pack_ubyte_RG_FLOAT16(const GLubyte src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[RCOMP]));
   d[1] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[GCOMP]));
}

static void
pack_float_RG_FLOAT16(const GLfloat src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(src[RCOMP]);
   d[1] = _mesa_float_to_half(src[GCOMP]);
}


/* MESA_FORMAT_DUDV8 */

static void
pack_ubyte_DUDV8(const GLubyte src[4], void *dst)
{
   /* XXX is this ever used? */
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_88(src[0], src[1]);
}

static void
pack_float_DUDV8(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLbyte du, dv;
   du = FLOAT_TO_BYTE(CLAMP(src[0], 0.0F, 1.0F));
   dv = FLOAT_TO_BYTE(CLAMP(src[1], 0.0F, 1.0F));
   *d = PACK_COLOR_88(du, dv);
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
 * MESA_FORMAT_SIGNED_R8
 */

static void
pack_float_SIGNED_R8(const GLfloat src[4], void *dst)
{
   GLbyte *d = (GLbyte *) dst;
   *d = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_SIGNED_RG88_REV
 */

static void
pack_float_SIGNED_RG88_REV(const GLfloat src[4], void *dst)
{
   GLushort *d = (GLushort *) dst;
   GLbyte r = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte g = FLOAT_TO_BYTE(CLAMP(src[GCOMP], -1.0f, 1.0f));
   *d = (g << 8) | r;
}


/*
 * MESA_FORMAT_SIGNED_RGBX8888
 */

static void
pack_float_SIGNED_RGBX8888(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLbyte r = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte g = FLOAT_TO_BYTE(CLAMP(src[GCOMP], -1.0f, 1.0f));
   GLbyte b = FLOAT_TO_BYTE(CLAMP(src[BCOMP], -1.0f, 1.0f));
   GLbyte a = 127;
   *d = PACK_COLOR_8888(r, g, b, a);
}


/*
 * MESA_FORMAT_SIGNED_RGBA8888
 */

static void
pack_float_SIGNED_RGBA8888(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLbyte r = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte g = FLOAT_TO_BYTE(CLAMP(src[GCOMP], -1.0f, 1.0f));
   GLbyte b = FLOAT_TO_BYTE(CLAMP(src[BCOMP], -1.0f, 1.0f));
   GLbyte a = FLOAT_TO_BYTE(CLAMP(src[ACOMP], -1.0f, 1.0f));
   *d = PACK_COLOR_8888(r, g, b, a);
}


/*
 * MESA_FORMAT_SIGNED_RGBA8888_REV
 */

static void
pack_float_SIGNED_RGBA8888_REV(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLbyte r = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte g = FLOAT_TO_BYTE(CLAMP(src[GCOMP], -1.0f, 1.0f));
   GLbyte b = FLOAT_TO_BYTE(CLAMP(src[BCOMP], -1.0f, 1.0f));
   GLbyte a = FLOAT_TO_BYTE(CLAMP(src[ACOMP], -1.0f, 1.0f));
   *d = PACK_COLOR_8888(a, b, g, r);
}


/*
 * MESA_FORMAT_SIGNED_R16
 */

static void
pack_float_SIGNED_R16(const GLfloat src[4], void *dst)
{
   GLshort *d = (GLshort *) dst;
   *d = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_SIGNED_GR1616
 */

static void
pack_float_SIGNED_GR1616(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLshort r = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLshort g = FLOAT_TO_SHORT(CLAMP(src[GCOMP], -1.0f, 1.0f));
   *d = (g << 16) | (r & 0xffff);
}


/*
 * MESA_FORMAT_SIGNED_RGB_16
 */

static void
pack_float_SIGNED_RGB_16(const GLfloat src[4], void *dst)
{
   GLshort *d = (GLshort *) dst;
   d[0] = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
   d[1] = FLOAT_TO_SHORT(CLAMP(src[GCOMP], -1.0f, 1.0f));
   d[2] = FLOAT_TO_SHORT(CLAMP(src[BCOMP], -1.0f, 1.0f));
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


/*
 * MESA_FORMAT_SIGNED_A8
 */

static void
pack_float_SIGNED_A8(const GLfloat src[4], void *dst)
{
   GLbyte *d = (GLbyte *) dst;
   *d = FLOAT_TO_BYTE(CLAMP(src[ACOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_SIGNED_L8
 */

static void
pack_float_SIGNED_L8(const GLfloat src[4], void *dst)
{
   GLbyte *d = (GLbyte *) dst;
   *d = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_SIGNED_AL88
 */

static void
pack_float_SIGNED_AL88(const GLfloat src[4], void *dst)
{
   GLushort *d = (GLushort *) dst;
   GLbyte l = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte a = FLOAT_TO_BYTE(CLAMP(src[ACOMP], -1.0f, 1.0f));
   *d = (a << 8) | l;
}


/*
 * MESA_FORMAT_SIGNED_A16
 */

static void
pack_float_SIGNED_A16(const GLfloat src[4], void *dst)
{
   GLshort *d = (GLshort *) dst;
   *d = FLOAT_TO_SHORT(CLAMP(src[ACOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_SIGNED_L16
 */

static void
pack_float_SIGNED_L16(const GLfloat src[4], void *dst)
{
   GLshort *d = (GLshort *) dst;
   *d = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_SIGNED_AL1616
 */

static void
pack_float_SIGNED_AL1616(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLshort l = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLshort a = FLOAT_TO_SHORT(CLAMP(src[ACOMP], -1.0f, 1.0f));
   *d = PACK_COLOR_1616(a, l);
}


/*
 * MESA_FORMAT_RGB9_E5_FLOAT;
 */

static void
pack_float_RGB9_E5_FLOAT(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   *d = float3_to_rgb9e5(src);
}

static void
pack_ubyte_RGB9_E5_FLOAT(const GLubyte src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLfloat rgb[3];
   rgb[0] = UBYTE_TO_FLOAT(src[RCOMP]);
   rgb[1] = UBYTE_TO_FLOAT(src[GCOMP]);
   rgb[2] = UBYTE_TO_FLOAT(src[BCOMP]);
   *d = float3_to_rgb9e5(rgb);
}



/*
 * MESA_FORMAT_R11_G11_B10_FLOAT;
 */

static void
pack_ubyte_R11_G11_B10_FLOAT(const GLubyte src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLfloat rgb[3];
   rgb[0] = UBYTE_TO_FLOAT(src[RCOMP]);
   rgb[1] = UBYTE_TO_FLOAT(src[GCOMP]);
   rgb[2] = UBYTE_TO_FLOAT(src[BCOMP]);
   *d = float3_to_r11g11b10f(rgb);
}

static void
pack_float_R11_G11_B10_FLOAT(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   *d = float3_to_r11g11b10f(src);
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
      table[MESA_FORMAT_R8] = pack_ubyte_R8;
      table[MESA_FORMAT_GR88] = pack_ubyte_GR88;
      table[MESA_FORMAT_RG88] = pack_ubyte_RG88;
      table[MESA_FORMAT_R16] = pack_ubyte_R16;
      table[MESA_FORMAT_RG1616] = pack_ubyte_RG1616;
      table[MESA_FORMAT_RG1616_REV] = pack_ubyte_RG1616_REV;
      table[MESA_FORMAT_ARGB2101010] = pack_ubyte_ARGB2101010;

      /* should never convert RGBA to these formats */
      table[MESA_FORMAT_Z24_S8] = NULL;
      table[MESA_FORMAT_S8_Z24] = NULL;
      table[MESA_FORMAT_Z16] = NULL;
      table[MESA_FORMAT_X8_Z24] = NULL;
      table[MESA_FORMAT_Z24_X8] = NULL;
      table[MESA_FORMAT_Z32] = NULL;
      table[MESA_FORMAT_S8] = NULL;

      /* sRGB */
      table[MESA_FORMAT_SRGB8] = pack_ubyte_SRGB8;
      table[MESA_FORMAT_SRGBA8] = pack_ubyte_SRGBA8;
      table[MESA_FORMAT_SARGB8] = pack_ubyte_SARGB8;
      table[MESA_FORMAT_SL8] = pack_ubyte_SL8;
      table[MESA_FORMAT_SLA8] = pack_ubyte_SLA8;

      /* n/a */
      table[MESA_FORMAT_SRGB_DXT1] = NULL; /* pack_ubyte_SRGB_DXT1; */
      table[MESA_FORMAT_SRGBA_DXT1] = NULL; /* pack_ubyte_SRGBA_DXT1; */
      table[MESA_FORMAT_SRGBA_DXT3] = NULL; /* pack_ubyte_SRGBA_DXT3; */
      table[MESA_FORMAT_SRGBA_DXT5] = NULL; /* pack_ubyte_SRGBA_DXT5; */

      table[MESA_FORMAT_RGB_FXT1] = NULL; /* pack_ubyte_RGB_FXT1; */
      table[MESA_FORMAT_RGBA_FXT1] = NULL; /* pack_ubyte_RGBA_FXT1; */
      table[MESA_FORMAT_RGB_DXT1] = NULL; /* pack_ubyte_RGB_DXT1; */
      table[MESA_FORMAT_RGBA_DXT1] = NULL; /* pack_ubyte_RGBA_DXT1; */
      table[MESA_FORMAT_RGBA_DXT3] = NULL; /* pack_ubyte_RGBA_DXT3; */
      table[MESA_FORMAT_RGBA_DXT5] = NULL; /* pack_ubyte_RGBA_DXT5; */

      table[MESA_FORMAT_RGBA_FLOAT32] = pack_ubyte_RGBA_FLOAT32;
      table[MESA_FORMAT_RGBA_FLOAT16] = pack_ubyte_RGBA_FLOAT16;
      table[MESA_FORMAT_RGB_FLOAT32] = pack_ubyte_RGB_FLOAT32;
      table[MESA_FORMAT_RGB_FLOAT16] = pack_ubyte_RGB_FLOAT16;
      table[MESA_FORMAT_ALPHA_FLOAT32] = pack_ubyte_ALPHA_FLOAT32;
      table[MESA_FORMAT_ALPHA_FLOAT16] = pack_ubyte_ALPHA_FLOAT16;
      table[MESA_FORMAT_LUMINANCE_FLOAT32] = pack_ubyte_LUMINANCE_FLOAT32;
      table[MESA_FORMAT_LUMINANCE_FLOAT16] = pack_ubyte_LUMINANCE_FLOAT16;
      table[MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32] = pack_ubyte_LUMINANCE_ALPHA_FLOAT32;
      table[MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16] = pack_ubyte_LUMINANCE_ALPHA_FLOAT16;
      table[MESA_FORMAT_INTENSITY_FLOAT32] = pack_ubyte_LUMINANCE_FLOAT32;
      table[MESA_FORMAT_INTENSITY_FLOAT16] = pack_ubyte_LUMINANCE_FLOAT16;
      table[MESA_FORMAT_R_FLOAT32] = pack_ubyte_LUMINANCE_FLOAT32;
      table[MESA_FORMAT_R_FLOAT16] = pack_ubyte_LUMINANCE_FLOAT16;
      table[MESA_FORMAT_RG_FLOAT32] = pack_ubyte_RG_FLOAT32;
      table[MESA_FORMAT_RG_FLOAT16] = pack_ubyte_RG_FLOAT16;

      /* n/a */
      table[MESA_FORMAT_RGBA_INT8] = NULL; /* pack_ubyte_RGBA_INT8 */
      table[MESA_FORMAT_RGBA_INT16] = NULL; /* pack_ubyte_RGBA_INT16 */
      table[MESA_FORMAT_RGBA_INT32] = NULL; /* pack_ubyte_RGBA_INT32 */
      table[MESA_FORMAT_RGBA_UINT8] = NULL; /* pack_ubyte_RGBA_UINT8 */
      table[MESA_FORMAT_RGBA_UINT16] = NULL; /* pack_ubyte_RGBA_UINT16 */
      table[MESA_FORMAT_RGBA_UINT32] = NULL; /* pack_ubyte_RGBA_UINT32 */

      table[MESA_FORMAT_DUDV8] = pack_ubyte_DUDV8;

      table[MESA_FORMAT_RGBA_16] = pack_ubyte_RGBA_16;

      /* n/a */
      table[MESA_FORMAT_SIGNED_R8] = NULL;
      table[MESA_FORMAT_SIGNED_RG88_REV] = NULL;
      table[MESA_FORMAT_SIGNED_RGBX8888] = NULL;
      table[MESA_FORMAT_SIGNED_RGBA8888] = NULL;
      table[MESA_FORMAT_SIGNED_RGBA8888_REV] = NULL;
      table[MESA_FORMAT_SIGNED_R16] = NULL;
      table[MESA_FORMAT_SIGNED_GR1616] = NULL;
      table[MESA_FORMAT_SIGNED_RGB_16] = NULL;
      table[MESA_FORMAT_SIGNED_RGBA_16] = NULL;
      table[MESA_FORMAT_SIGNED_A8] = NULL;
      table[MESA_FORMAT_SIGNED_L8] = NULL;
      table[MESA_FORMAT_SIGNED_AL88] = NULL;
      table[MESA_FORMAT_SIGNED_I8] = NULL;
      table[MESA_FORMAT_SIGNED_A16] = NULL;
      table[MESA_FORMAT_SIGNED_L16] = NULL;
      table[MESA_FORMAT_SIGNED_AL1616] = NULL;
      table[MESA_FORMAT_SIGNED_I16] = NULL;


      table[MESA_FORMAT_RGBA_16] = pack_ubyte_RGBA_16;

      table[MESA_FORMAT_RGB9_E5_FLOAT] = pack_ubyte_RGB9_E5_FLOAT;
      table[MESA_FORMAT_R11_G11_B10_FLOAT] = pack_ubyte_R11_G11_B10_FLOAT;

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
      table[MESA_FORMAT_R8] = pack_float_R8;
      table[MESA_FORMAT_GR88] = pack_float_GR88;
      table[MESA_FORMAT_RG88] = pack_float_RG88;
      table[MESA_FORMAT_R16] = pack_float_R16;
      table[MESA_FORMAT_RG1616] = pack_float_RG1616;
      table[MESA_FORMAT_RG1616_REV] = pack_float_RG1616_REV;
      table[MESA_FORMAT_ARGB2101010] = pack_float_ARGB2101010;

      /* should never convert RGBA to these formats */
      table[MESA_FORMAT_Z24_S8] = NULL;
      table[MESA_FORMAT_S8_Z24] = NULL;
      table[MESA_FORMAT_Z16] = NULL;
      table[MESA_FORMAT_X8_Z24] = NULL;
      table[MESA_FORMAT_Z24_X8] = NULL;
      table[MESA_FORMAT_Z32] = NULL;
      table[MESA_FORMAT_S8] = NULL;

      table[MESA_FORMAT_SRGB8] = pack_float_SRGB8;
      table[MESA_FORMAT_SRGBA8] = pack_float_SRGBA8;
      table[MESA_FORMAT_SARGB8] = pack_float_SARGB8;
      table[MESA_FORMAT_SL8] = pack_float_SL8;
      table[MESA_FORMAT_SLA8] = pack_float_SLA8;

      /* n/a */
      table[MESA_FORMAT_SRGB_DXT1] = NULL;
      table[MESA_FORMAT_SRGBA_DXT1] = NULL;
      table[MESA_FORMAT_SRGBA_DXT3] = NULL;
      table[MESA_FORMAT_SRGBA_DXT5] = NULL;

      table[MESA_FORMAT_RGB_FXT1] = NULL;
      table[MESA_FORMAT_RGBA_FXT1] = NULL;
      table[MESA_FORMAT_RGB_DXT1] = NULL;
      table[MESA_FORMAT_RGBA_DXT1] = NULL;
      table[MESA_FORMAT_RGBA_DXT3] = NULL;
      table[MESA_FORMAT_RGBA_DXT5] = NULL;

      table[MESA_FORMAT_RGBA_FLOAT32] = pack_float_RGBA_FLOAT32;
      table[MESA_FORMAT_RGBA_FLOAT16] = pack_float_RGBA_FLOAT16;
      table[MESA_FORMAT_RGB_FLOAT32] = pack_float_RGB_FLOAT32;
      table[MESA_FORMAT_RGB_FLOAT16] = pack_float_RGB_FLOAT16;
      table[MESA_FORMAT_ALPHA_FLOAT32] = pack_float_ALPHA_FLOAT32;
      table[MESA_FORMAT_ALPHA_FLOAT16] = pack_float_ALPHA_FLOAT16;
      table[MESA_FORMAT_LUMINANCE_FLOAT32] = pack_float_LUMINANCE_FLOAT32;
      table[MESA_FORMAT_LUMINANCE_FLOAT16] = pack_float_LUMINANCE_FLOAT16;
      table[MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32] = pack_float_LUMINANCE_ALPHA_FLOAT32;
      table[MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16] = pack_float_LUMINANCE_ALPHA_FLOAT16;

      table[MESA_FORMAT_INTENSITY_FLOAT32] = pack_float_LUMINANCE_FLOAT32;
      table[MESA_FORMAT_INTENSITY_FLOAT16] = pack_float_LUMINANCE_FLOAT16;
      table[MESA_FORMAT_R_FLOAT32] = pack_float_LUMINANCE_FLOAT32;
      table[MESA_FORMAT_R_FLOAT16] = pack_float_LUMINANCE_FLOAT16;
      table[MESA_FORMAT_RG_FLOAT32] = pack_float_RG_FLOAT32;
      table[MESA_FORMAT_RG_FLOAT16] = pack_float_RG_FLOAT16;

      /* n/a */
      table[MESA_FORMAT_RGBA_INT8] = NULL;
      table[MESA_FORMAT_RGBA_INT16] = NULL;
      table[MESA_FORMAT_RGBA_INT32] = NULL;
      table[MESA_FORMAT_RGBA_UINT8] = NULL;
      table[MESA_FORMAT_RGBA_UINT16] = NULL;
      table[MESA_FORMAT_RGBA_UINT32] = NULL;

      table[MESA_FORMAT_DUDV8] = pack_float_DUDV8;

      table[MESA_FORMAT_RGBA_16] = pack_float_RGBA_16;

      table[MESA_FORMAT_SIGNED_R8] = pack_float_SIGNED_R8;
      table[MESA_FORMAT_SIGNED_RG88_REV] = pack_float_SIGNED_RG88_REV;
      table[MESA_FORMAT_SIGNED_RGBX8888] = pack_float_SIGNED_RGBX8888;
      table[MESA_FORMAT_SIGNED_RGBA8888] = pack_float_SIGNED_RGBA8888;
      table[MESA_FORMAT_SIGNED_RGBA8888_REV] = pack_float_SIGNED_RGBA8888_REV;
      table[MESA_FORMAT_SIGNED_R16] = pack_float_SIGNED_R16;
      table[MESA_FORMAT_SIGNED_GR1616] = pack_float_SIGNED_GR1616;
      table[MESA_FORMAT_SIGNED_RGB_16] = pack_float_SIGNED_RGB_16;
      table[MESA_FORMAT_SIGNED_RGBA_16] = pack_float_SIGNED_RGBA_16;
      table[MESA_FORMAT_SIGNED_A8] = pack_float_SIGNED_A8;
      table[MESA_FORMAT_SIGNED_L8] = pack_float_SIGNED_L8;
      table[MESA_FORMAT_SIGNED_AL88] = pack_float_SIGNED_AL88;
      table[MESA_FORMAT_SIGNED_I8] = pack_float_SIGNED_L8; /* reused */
      table[MESA_FORMAT_SIGNED_A16] = pack_float_SIGNED_A16;
      table[MESA_FORMAT_SIGNED_L16] = pack_float_SIGNED_L16;
      table[MESA_FORMAT_SIGNED_AL1616] = pack_float_SIGNED_AL1616;
      table[MESA_FORMAT_SIGNED_I16] = pack_float_SIGNED_L16; /* reused */

      table[MESA_FORMAT_RGB9_E5_FLOAT] = pack_float_RGB9_E5_FLOAT;
      table[MESA_FORMAT_R11_G11_B10_FLOAT] = pack_float_R11_G11_B10_FLOAT;

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

static void
pack_float_z_Z32_FLOAT(const GLfloat *src, void *dst)
{
   GLfloat *d = (GLfloat *) dst;
   *d = *src;
}

gl_pack_float_z_func
_mesa_get_pack_float_z_func(gl_format format)
{
   switch (format) {
   case MESA_FORMAT_Z24_S8:
   case MESA_FORMAT_Z24_X8:
      return pack_float_z_Z24_S8;
   case MESA_FORMAT_S8_Z24:
   case MESA_FORMAT_X8_Z24:
      return pack_float_z_S8_Z24;
   case MESA_FORMAT_Z16:
      return pack_float_z_Z16;
   case MESA_FORMAT_Z32:
      return pack_float_z_Z32;
   case MESA_FORMAT_Z32_FLOAT:
   case MESA_FORMAT_Z32_FLOAT_X24S8:
      return pack_float_z_Z32_FLOAT;
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

static void
pack_uint_z_Z32_FLOAT(const GLuint *src, void *dst)
{
   GLuint *d = ((GLuint *) dst);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffffff;
   *d = *src * scale;
   assert(*d >= 0.0f);
   assert(*d <= 1.0f);
}

static void
pack_uint_z_Z32_FLOAT_X24S8(const GLuint *src, void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffffff;
   *d = *src * scale;
   assert(*d >= 0.0f);
   assert(*d <= 1.0f);
}

gl_pack_uint_z_func
_mesa_get_pack_uint_z_func(gl_format format)
{
   switch (format) {
   case MESA_FORMAT_Z24_S8:
   case MESA_FORMAT_Z24_X8:
      return pack_uint_z_Z24_S8;
   case MESA_FORMAT_S8_Z24:
   case MESA_FORMAT_X8_Z24:
      return pack_uint_z_S8_Z24;
   case MESA_FORMAT_Z16:
      return pack_uint_z_Z16;
   case MESA_FORMAT_Z32:
      return pack_uint_z_Z32;
   case MESA_FORMAT_Z32_FLOAT:
      return pack_uint_z_Z32_FLOAT;
   case MESA_FORMAT_Z32_FLOAT_X24S8:
      return pack_uint_z_Z32_FLOAT_X24S8;
   default:
      _mesa_problem(NULL, "unexpected format in _mesa_get_pack_uint_z_func()");
      return NULL;
   }
}


/**
 ** Pack ubyte stencil pixels
 **/

static void
pack_ubyte_stencil_Z24_S8(const GLubyte *src, void *dst)
{
   /* don't disturb the Z values */
   GLuint *d = ((GLuint *) dst);
   GLuint s = *src;
   GLuint z = *d & 0xffffff00;
   *d = z | s;
}

static void
pack_ubyte_stencil_S8_Z24(const GLubyte *src, void *dst)
{
   /* don't disturb the Z values */
   GLuint *d = ((GLuint *) dst);
   GLuint s = *src << 24;
   GLuint z = *d & 0xffffff;
   *d = s | z;
}

static void
pack_ubyte_stencil_S8(const GLubyte *src, void *dst)
{
   GLubyte *d = (GLubyte *) dst;
   *d = *src;
}

static void
pack_ubyte_stencil_Z32_FLOAT_X24S8(const GLubyte *src, void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[1] = *src;
}


gl_pack_ubyte_stencil_func
_mesa_get_pack_ubyte_stencil_func(gl_format format)
{
   switch (format) {
   case MESA_FORMAT_Z24_S8:
      return pack_ubyte_stencil_Z24_S8;
   case MESA_FORMAT_S8_Z24:
      return pack_ubyte_stencil_S8_Z24;
   case MESA_FORMAT_S8:
      return pack_ubyte_stencil_S8;
   case MESA_FORMAT_Z32_FLOAT_X24S8:
      return pack_ubyte_stencil_Z32_FLOAT_X24S8;
   default:
      _mesa_problem(NULL,
                    "unexpected format in _mesa_pack_ubyte_stencil_func()");
      return NULL;
   }
}



void
_mesa_pack_float_z_row(gl_format format, GLuint n,
                       const GLfloat *src, void *dst)
{
   switch (format) {
   case MESA_FORMAT_Z24_S8:
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
   case MESA_FORMAT_S8_Z24:
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
   case MESA_FORMAT_Z32_FLOAT:
      memcpy(dst, src, n * sizeof(GLfloat));
      break;
   case MESA_FORMAT_Z32_FLOAT_X24S8:
      {
         GLfloat *d = ((GLfloat *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i * 2] = src[i];
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
   case MESA_FORMAT_Z24_S8:
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
   case MESA_FORMAT_S8_Z24:
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
   case MESA_FORMAT_Z32_FLOAT:
      {
         GLuint *d = ((GLuint *) dst);
         const GLdouble scale = 1.0 / (GLdouble) 0xffffffff;
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i] = src[i] * scale;
            assert(d[i] >= 0.0f);
            assert(d[i] <= 1.0f);
         }
      }
      break;
   case MESA_FORMAT_Z32_FLOAT_X24S8:
      {
         GLfloat *d = ((GLfloat *) dst);
         const GLdouble scale = 1.0 / (GLdouble) 0xffffffff;
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i * 2] = src[i] * scale;
            assert(d[i * 2] >= 0.0f);
            assert(d[i * 2] <= 1.0f);
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format in _mesa_pack_uint_z_row()");
   }
}


void
_mesa_pack_ubyte_stencil_row(gl_format format, GLuint n,
                             const GLubyte *src, void *dst)
{
   switch (format) {
   case MESA_FORMAT_Z24_S8:
      {
         /* don't disturb the Z values */
         GLuint *d = ((GLuint *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = src[i];
            GLuint z = d[i] & 0xffffff00;
            d[i] = z | s;
         }
      }
      break;
   case MESA_FORMAT_S8_Z24:
      {
         /* don't disturb the Z values */
         GLuint *d = ((GLuint *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = src[i] << 24;
            GLuint z = d[i] & 0xffffff;
            d[i] = s | z;
         }
      }
      break;
   case MESA_FORMAT_S8:
      memcpy(dst, src, n * sizeof(GLubyte));
      break;
   case MESA_FORMAT_Z32_FLOAT_X24S8:
      {
         GLuint *d = dst;
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i * 2 + 1] = src[i];
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format in _mesa_pack_ubyte_stencil_row()");
   }
}


/**
 * Incoming Z/stencil values are always in uint_24_8 format.
 */
void
_mesa_pack_uint_24_8_depth_stencil_row(gl_format format, GLuint n,
                                       const GLuint *src, void *dst)
{
   switch (format) {
   case MESA_FORMAT_Z24_S8:
      memcpy(dst, src, n * sizeof(GLuint));
      break;
   case MESA_FORMAT_S8_Z24:
      {
         GLuint *d = ((GLuint *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = src[i] << 24;
            GLuint z = src[i] >> 8;
            d[i] = s | z;
         }
      }
      break;
   default:
      _mesa_problem(NULL, "bad format %s in _mesa_pack_ubyte_s_row",
                    _mesa_get_format_name(format));
      return;
   }
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
