/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * \file mipmap.c  mipmap generation and teximage resizing functions.
 */

#include "imports.h"
#include "mipmap.h"
#include "texcompress.h"
#include "texformat.h"
#include "teximage.h"
#include "image.h"



/**
 * Average together two rows of a source image to produce a single new
 * row in the dest image.  It's legal for the two source rows to point
 * to the same data.  The source width must be equal to either the
 * dest width or two times the dest width.
 */
static void
do_row(const struct gl_texture_format *format, GLint srcWidth,
       const GLvoid *srcRowA, const GLvoid *srcRowB,
       GLint dstWidth, GLvoid *dstRow)
{
   const GLuint k0 = (srcWidth == dstWidth) ? 0 : 1;
   const GLuint colStride = (srcWidth == dstWidth) ? 1 : 2;

   /* This assertion is no longer valid with non-power-of-2 textures
   assert(srcWidth == dstWidth || srcWidth == 2 * dstWidth);
   */

   switch (format->MesaFormat) {
   case MESA_FORMAT_RGBA:
      {
         GLuint i, j, k;
         const GLchan (*rowA)[4] = (const GLchan (*)[4]) srcRowA;
         const GLchan (*rowB)[4] = (const GLchan (*)[4]) srcRowB;
         GLchan (*dst)[4] = (GLchan (*)[4]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) / 4;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) / 4;
            dst[i][2] = (rowA[j][2] + rowA[k][2] +
                         rowB[j][2] + rowB[k][2]) / 4;
            dst[i][3] = (rowA[j][3] + rowA[k][3] +
                         rowB[j][3] + rowB[k][3]) / 4;
         }
      }
      return;
   case MESA_FORMAT_RGB:
      {
         GLuint i, j, k;
         const GLchan (*rowA)[3] = (const GLchan (*)[3]) srcRowA;
         const GLchan (*rowB)[3] = (const GLchan (*)[3]) srcRowB;
         GLchan (*dst)[3] = (GLchan (*)[3]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) / 4;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) / 4;
            dst[i][2] = (rowA[j][2] + rowA[k][2] +
                         rowB[j][2] + rowB[k][2]) / 4;
         }
      }
      return;
   case MESA_FORMAT_ALPHA:
   case MESA_FORMAT_LUMINANCE:
   case MESA_FORMAT_INTENSITY:
      {
         GLuint i, j, k;
         const GLchan *rowA = (const GLchan *) srcRowA;
         const GLchan *rowB = (const GLchan *) srcRowB;
         GLchan *dst = (GLchan *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) / 4;
         }
      }
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA:
      {
         GLuint i, j, k;
         const GLchan (*rowA)[2] = (const GLchan (*)[2]) srcRowA;
         const GLchan (*rowB)[2] = (const GLchan (*)[2]) srcRowB;
         GLchan (*dst)[2] = (GLchan (*)[2]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) / 4;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) / 4;
         }
      }
      return;
   case MESA_FORMAT_Z32:
      {
         GLuint i, j, k;
         const GLuint *rowA = (const GLuint *) srcRowA;
         const GLuint *rowB = (const GLuint *) srcRowB;
         GLfloat *dst = (GLfloat *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i] = rowA[j] / 4 + rowA[k] / 4 + rowB[j] / 4 + rowB[k] / 4;
         }
      }
      return;
   case MESA_FORMAT_Z16:
      {
         GLuint i, j, k;
         const GLushort *rowA = (const GLushort *) srcRowA;
         const GLushort *rowB = (const GLushort *) srcRowB;
         GLushort *dst = (GLushort *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) / 4;
         }
      }
      return;
   /* Begin hardware formats */
   case MESA_FORMAT_RGBA8888:
   case MESA_FORMAT_RGBA8888_REV:
   case MESA_FORMAT_ARGB8888:
   case MESA_FORMAT_ARGB8888_REV:
#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SRGBA8:
#endif
      {
         GLuint i, j, k;
         const GLubyte (*rowA)[4] = (const GLubyte (*)[4]) srcRowA;
         const GLubyte (*rowB)[4] = (const GLubyte (*)[4]) srcRowB;
         GLubyte (*dst)[4] = (GLubyte (*)[4]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) / 4;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) / 4;
            dst[i][2] = (rowA[j][2] + rowA[k][2] +
                         rowB[j][2] + rowB[k][2]) / 4;
            dst[i][3] = (rowA[j][3] + rowA[k][3] +
                         rowB[j][3] + rowB[k][3]) / 4;
         }
      }
      return;
   case MESA_FORMAT_RGB888:
   case MESA_FORMAT_BGR888:
#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SRGB8:
#endif
      {
         GLuint i, j, k;
         const GLubyte (*rowA)[3] = (const GLubyte (*)[3]) srcRowA;
         const GLubyte (*rowB)[3] = (const GLubyte (*)[3]) srcRowB;
         GLubyte (*dst)[3] = (GLubyte (*)[3]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) / 4;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) / 4;
            dst[i][2] = (rowA[j][2] + rowA[k][2] +
                         rowB[j][2] + rowB[k][2]) / 4;
         }
      }
      return;
   case MESA_FORMAT_RGB565:
   case MESA_FORMAT_RGB565_REV:
      {
         GLuint i, j, k;
         const GLushort *rowA = (const GLushort *) srcRowA;
         const GLushort *rowB = (const GLushort *) srcRowB;
         GLushort *dst = (GLushort *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            const GLint rowAr0 = rowA[j] & 0x1f;
            const GLint rowAr1 = rowA[k] & 0x1f;
            const GLint rowBr0 = rowB[j] & 0x1f;
            const GLint rowBr1 = rowB[k] & 0x1f;
            const GLint rowAg0 = (rowA[j] >> 5) & 0x3f;
            const GLint rowAg1 = (rowA[k] >> 5) & 0x3f;
            const GLint rowBg0 = (rowB[j] >> 5) & 0x3f;
            const GLint rowBg1 = (rowB[k] >> 5) & 0x3f;
            const GLint rowAb0 = (rowA[j] >> 11) & 0x1f;
            const GLint rowAb1 = (rowA[k] >> 11) & 0x1f;
            const GLint rowBb0 = (rowB[j] >> 11) & 0x1f;
            const GLint rowBb1 = (rowB[k] >> 11) & 0x1f;
            const GLint red   = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
            const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
            const GLint blue  = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
            dst[i] = (blue << 11) | (green << 5) | red;
         }
      }
      return;
   case MESA_FORMAT_ARGB4444:
   case MESA_FORMAT_ARGB4444_REV:
      {
         GLuint i, j, k;
         const GLushort *rowA = (const GLushort *) srcRowA;
         const GLushort *rowB = (const GLushort *) srcRowB;
         GLushort *dst = (GLushort *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            const GLint rowAr0 = rowA[j] & 0xf;
            const GLint rowAr1 = rowA[k] & 0xf;
            const GLint rowBr0 = rowB[j] & 0xf;
            const GLint rowBr1 = rowB[k] & 0xf;
            const GLint rowAg0 = (rowA[j] >> 4) & 0xf;
            const GLint rowAg1 = (rowA[k] >> 4) & 0xf;
            const GLint rowBg0 = (rowB[j] >> 4) & 0xf;
            const GLint rowBg1 = (rowB[k] >> 4) & 0xf;
            const GLint rowAb0 = (rowA[j] >> 8) & 0xf;
            const GLint rowAb1 = (rowA[k] >> 8) & 0xf;
            const GLint rowBb0 = (rowB[j] >> 8) & 0xf;
            const GLint rowBb1 = (rowB[k] >> 8) & 0xf;
            const GLint rowAa0 = (rowA[j] >> 12) & 0xf;
            const GLint rowAa1 = (rowA[k] >> 12) & 0xf;
            const GLint rowBa0 = (rowB[j] >> 12) & 0xf;
            const GLint rowBa1 = (rowB[k] >> 12) & 0xf;
            const GLint red   = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
            const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
            const GLint blue  = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
            const GLint alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
            dst[i] = (alpha << 12) | (blue << 8) | (green << 4) | red;
         }
      }
      return;
   case MESA_FORMAT_ARGB1555:
   case MESA_FORMAT_ARGB1555_REV: /* XXX broken? */
      {
         GLuint i, j, k;
         const GLushort *rowA = (const GLushort *) srcRowA;
         const GLushort *rowB = (const GLushort *) srcRowB;
         GLushort *dst = (GLushort *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            const GLint rowAr0 = rowA[j] & 0x1f;
            const GLint rowAr1 = rowA[k] & 0x1f;
            const GLint rowBr0 = rowB[j] & 0x1f;
            const GLint rowBr1 = rowB[k] & 0xf;
            const GLint rowAg0 = (rowA[j] >> 5) & 0x1f;
            const GLint rowAg1 = (rowA[k] >> 5) & 0x1f;
            const GLint rowBg0 = (rowB[j] >> 5) & 0x1f;
            const GLint rowBg1 = (rowB[k] >> 5) & 0x1f;
            const GLint rowAb0 = (rowA[j] >> 10) & 0x1f;
            const GLint rowAb1 = (rowA[k] >> 10) & 0x1f;
            const GLint rowBb0 = (rowB[j] >> 10) & 0x1f;
            const GLint rowBb1 = (rowB[k] >> 10) & 0x1f;
            const GLint rowAa0 = (rowA[j] >> 15) & 0x1;
            const GLint rowAa1 = (rowA[k] >> 15) & 0x1;
            const GLint rowBa0 = (rowB[j] >> 15) & 0x1;
            const GLint rowBa1 = (rowB[k] >> 15) & 0x1;
            const GLint red   = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
            const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
            const GLint blue  = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
            const GLint alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
            dst[i] = (alpha << 15) | (blue << 10) | (green << 5) | red;
         }
      }
      return;
   case MESA_FORMAT_AL88:
   case MESA_FORMAT_AL88_REV:
#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SLA8:
#endif
      {
         GLuint i, j, k;
         const GLubyte (*rowA)[2] = (const GLubyte (*)[2]) srcRowA;
         const GLubyte (*rowB)[2] = (const GLubyte (*)[2]) srcRowB;
         GLubyte (*dst)[2] = (GLubyte (*)[2]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) >> 2;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) >> 2;
         }
      }
      return;
   case MESA_FORMAT_RGB332:
      {
         GLuint i, j, k;
         const GLubyte *rowA = (const GLubyte *) srcRowA;
         const GLubyte *rowB = (const GLubyte *) srcRowB;
         GLubyte *dst = (GLubyte *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            const GLint rowAr0 = rowA[j] & 0x3;
            const GLint rowAr1 = rowA[k] & 0x3;
            const GLint rowBr0 = rowB[j] & 0x3;
            const GLint rowBr1 = rowB[k] & 0x3;
            const GLint rowAg0 = (rowA[j] >> 2) & 0x7;
            const GLint rowAg1 = (rowA[k] >> 2) & 0x7;
            const GLint rowBg0 = (rowB[j] >> 2) & 0x7;
            const GLint rowBg1 = (rowB[k] >> 2) & 0x7;
            const GLint rowAb0 = (rowA[j] >> 5) & 0x7;
            const GLint rowAb1 = (rowA[k] >> 5) & 0x7;
            const GLint rowBb0 = (rowB[j] >> 5) & 0x7;
            const GLint rowBb1 = (rowB[k] >> 5) & 0x7;
            const GLint red   = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
            const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
            const GLint blue  = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
            dst[i] = (blue << 5) | (green << 2) | red;
         }
      }
      return;
   case MESA_FORMAT_A8:
   case MESA_FORMAT_L8:
   case MESA_FORMAT_I8:
   case MESA_FORMAT_CI8:
#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SL8:
#endif
      {
         GLuint i, j, k;
         const GLubyte *rowA = (const GLubyte *) srcRowA;
         const GLubyte *rowB = (const GLubyte *) srcRowB;
         GLubyte *dst = (GLubyte *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) >> 2;
         }
      }
      return;
   case MESA_FORMAT_RGBA_FLOAT32:
      {
         GLuint i, j, k;
         const GLfloat (*rowA)[4] = (const GLfloat (*)[4]) srcRowA;
         const GLfloat (*rowB)[4] = (const GLfloat (*)[4]) srcRowB;
         GLfloat (*dst)[4] = (GLfloat (*)[4]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) * 0.25F;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) * 0.25F;
            dst[i][2] = (rowA[j][2] + rowA[k][2] +
                         rowB[j][2] + rowB[k][2]) * 0.25F;
            dst[i][3] = (rowA[j][3] + rowA[k][3] +
                         rowB[j][3] + rowB[k][3]) * 0.25F;
         }
      }
      return;
   case MESA_FORMAT_RGBA_FLOAT16:
      {
         GLuint i, j, k, comp;
         const GLhalfARB (*rowA)[4] = (const GLhalfARB (*)[4]) srcRowA;
         const GLhalfARB (*rowB)[4] = (const GLhalfARB (*)[4]) srcRowB;
         GLhalfARB (*dst)[4] = (GLhalfARB (*)[4]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            for (comp = 0; comp < 4; comp++) {
               GLfloat aj, ak, bj, bk;
               aj = _mesa_half_to_float(rowA[j][comp]);
               ak = _mesa_half_to_float(rowA[k][comp]);
               bj = _mesa_half_to_float(rowB[j][comp]);
               bk = _mesa_half_to_float(rowB[k][comp]);
               dst[i][comp] = _mesa_float_to_half((aj + ak + bj + bk) * 0.25F);
            }
         }
      }
      return;
   case MESA_FORMAT_RGB_FLOAT32:
      {
         GLuint i, j, k;
         const GLfloat (*rowA)[3] = (const GLfloat (*)[3]) srcRowA;
         const GLfloat (*rowB)[3] = (const GLfloat (*)[3]) srcRowB;
         GLfloat (*dst)[3] = (GLfloat (*)[3]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) * 0.25F;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) * 0.25F;
            dst[i][2] = (rowA[j][2] + rowA[k][2] +
                         rowB[j][2] + rowB[k][2]) * 0.25F;
         }
      }
      return;
   case MESA_FORMAT_RGB_FLOAT16:
      {
         GLuint i, j, k, comp;
         const GLhalfARB (*rowA)[3] = (const GLhalfARB (*)[3]) srcRowA;
         const GLhalfARB (*rowB)[3] = (const GLhalfARB (*)[3]) srcRowB;
         GLhalfARB (*dst)[3] = (GLhalfARB (*)[3]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            for (comp = 0; comp < 3; comp++) {
               GLfloat aj, ak, bj, bk;
               aj = _mesa_half_to_float(rowA[j][comp]);
               ak = _mesa_half_to_float(rowA[k][comp]);
               bj = _mesa_half_to_float(rowB[j][comp]);
               bk = _mesa_half_to_float(rowB[k][comp]);
               dst[i][comp] = _mesa_float_to_half((aj + ak + bj + bk) * 0.25F);
            }
         }
      }
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
      {
         GLuint i, j, k;
         const GLfloat (*rowA)[2] = (const GLfloat (*)[2]) srcRowA;
         const GLfloat (*rowB)[2] = (const GLfloat (*)[2]) srcRowB;
         GLfloat (*dst)[2] = (GLfloat (*)[2]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) * 0.25F;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) * 0.25F;
         }
      }
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16:
      {
         GLuint i, j, k, comp;
         const GLhalfARB (*rowA)[2] = (const GLhalfARB (*)[2]) srcRowA;
         const GLhalfARB (*rowB)[2] = (const GLhalfARB (*)[2]) srcRowB;
         GLhalfARB (*dst)[2] = (GLhalfARB (*)[2]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            for (comp = 0; comp < 2; comp++) {
               GLfloat aj, ak, bj, bk;
               aj = _mesa_half_to_float(rowA[j][comp]);
               ak = _mesa_half_to_float(rowA[k][comp]);
               bj = _mesa_half_to_float(rowB[j][comp]);
               bk = _mesa_half_to_float(rowB[k][comp]);
               dst[i][comp] = _mesa_float_to_half((aj + ak + bj + bk) * 0.25F);
            }
         }
      }
      return;
   case MESA_FORMAT_ALPHA_FLOAT32:
   case MESA_FORMAT_LUMINANCE_FLOAT32:
   case MESA_FORMAT_INTENSITY_FLOAT32:
      {
         GLuint i, j, k;
         const GLfloat *rowA = (const GLfloat *) srcRowA;
         const GLfloat *rowB = (const GLfloat *) srcRowB;
         GLfloat *dst = (GLfloat *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) * 0.25F;
         }
      }
      return;
   case MESA_FORMAT_ALPHA_FLOAT16:
   case MESA_FORMAT_LUMINANCE_FLOAT16:
   case MESA_FORMAT_INTENSITY_FLOAT16:
      {
         GLuint i, j, k;
         const GLhalfARB *rowA = (const GLhalfARB *) srcRowA;
         const GLhalfARB *rowB = (const GLhalfARB *) srcRowB;
         GLhalfARB *dst = (GLhalfARB *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            GLfloat aj, ak, bj, bk;
            aj = _mesa_half_to_float(rowA[j]);
            ak = _mesa_half_to_float(rowA[k]);
            bj = _mesa_half_to_float(rowB[j]);
            bk = _mesa_half_to_float(rowB[k]);
            dst[i] = _mesa_float_to_half((aj + ak + bj + bk) * 0.25F);
         }
      }
      return;

   default:
      _mesa_problem(NULL, "bad format in do_row()");
   }
}


/*
 * These functions generate a 1/2-size mipmap image from a source image.
 * Texture borders are handled by copying or averaging the source image's
 * border texels, depending on the scale-down factor.
 */

static void
make_1d_mipmap(const struct gl_texture_format *format, GLint border,
               GLint srcWidth, const GLubyte *srcPtr,
               GLint dstWidth, GLubyte *dstPtr)
{
   const GLint bpt = format->TexelBytes;
   const GLubyte *src;
   GLubyte *dst;

   /* skip the border pixel, if any */
   src = srcPtr + border * bpt;
   dst = dstPtr + border * bpt;

   /* we just duplicate the input row, kind of hack, saves code */
   do_row(format, srcWidth - 2 * border, src, src,
          dstWidth - 2 * border, dst);

   if (border) {
      /* copy left-most pixel from source */
      MEMCPY(dstPtr, srcPtr, bpt);
      /* copy right-most pixel from source */
      MEMCPY(dstPtr + (dstWidth - 1) * bpt,
             srcPtr + (srcWidth - 1) * bpt,
             bpt);
   }
}


static void
make_2d_mipmap(const struct gl_texture_format *format, GLint border,
               GLint srcWidth, GLint srcHeight,
	       const GLubyte *srcPtr, GLint srcRowStride,
               GLint dstWidth, GLint dstHeight,
	       GLubyte *dstPtr, GLint dstRowStride)
{
   const GLint bpt = format->TexelBytes;
   const GLint srcWidthNB = srcWidth - 2 * border;  /* sizes w/out border */
   const GLint dstWidthNB = dstWidth - 2 * border;
   const GLint dstHeightNB = dstHeight - 2 * border;
   const GLint srcRowBytes = bpt * srcRowStride;
   const GLint dstRowBytes = bpt * dstRowStride;
   const GLubyte *srcA, *srcB;
   GLubyte *dst;
   GLint row;

   /* Compute src and dst pointers, skipping any border */
   srcA = srcPtr + border * ((srcWidth + 1) * bpt);
   if (srcHeight > 1) 
      srcB = srcA + srcRowBytes;
   else
      srcB = srcA;
   dst = dstPtr + border * ((dstWidth + 1) * bpt);

   for (row = 0; row < dstHeightNB; row++) {
      do_row(format, srcWidthNB, srcA, srcB,
             dstWidthNB, dst);
      srcA += 2 * srcRowBytes;
      srcB += 2 * srcRowBytes;
      dst += dstRowBytes;
   }

   /* This is ugly but probably won't be used much */
   if (border > 0) {
      /* fill in dest border */
      /* lower-left border pixel */
      MEMCPY(dstPtr, srcPtr, bpt);
      /* lower-right border pixel */
      MEMCPY(dstPtr + (dstWidth - 1) * bpt,
             srcPtr + (srcWidth - 1) * bpt, bpt);
      /* upper-left border pixel */
      MEMCPY(dstPtr + dstWidth * (dstHeight - 1) * bpt,
             srcPtr + srcWidth * (srcHeight - 1) * bpt, bpt);
      /* upper-right border pixel */
      MEMCPY(dstPtr + (dstWidth * dstHeight - 1) * bpt,
             srcPtr + (srcWidth * srcHeight - 1) * bpt, bpt);
      /* lower border */
      do_row(format, srcWidthNB,
             srcPtr + bpt,
             srcPtr + bpt,
             dstWidthNB, dstPtr + bpt);
      /* upper border */
      do_row(format, srcWidthNB,
             srcPtr + (srcWidth * (srcHeight - 1) + 1) * bpt,
             srcPtr + (srcWidth * (srcHeight - 1) + 1) * bpt,
             dstWidthNB,
             dstPtr + (dstWidth * (dstHeight - 1) + 1) * bpt);
      /* left and right borders */
      if (srcHeight == dstHeight) {
         /* copy border pixel from src to dst */
         for (row = 1; row < srcHeight; row++) {
            MEMCPY(dstPtr + dstWidth * row * bpt,
                   srcPtr + srcWidth * row * bpt, bpt);
            MEMCPY(dstPtr + (dstWidth * row + dstWidth - 1) * bpt,
                   srcPtr + (srcWidth * row + srcWidth - 1) * bpt, bpt);
         }
      }
      else {
         /* average two src pixels each dest pixel */
         for (row = 0; row < dstHeightNB; row += 2) {
            do_row(format, 1,
                   srcPtr + (srcWidth * (row * 2 + 1)) * bpt,
                   srcPtr + (srcWidth * (row * 2 + 2)) * bpt,
                   1, dstPtr + (dstWidth * row + 1) * bpt);
            do_row(format, 1,
                   srcPtr + (srcWidth * (row * 2 + 1) + srcWidth - 1) * bpt,
                   srcPtr + (srcWidth * (row * 2 + 2) + srcWidth - 1) * bpt,
                   1, dstPtr + (dstWidth * row + 1 + dstWidth - 1) * bpt);
         }
      }
   }
}


static void
make_3d_mipmap(const struct gl_texture_format *format, GLint border,
               GLint srcWidth, GLint srcHeight, GLint srcDepth,
               const GLubyte *srcPtr, GLint srcRowStride,
               GLint dstWidth, GLint dstHeight, GLint dstDepth,
               GLubyte *dstPtr, GLint dstRowStride)
{
   const GLint bpt = format->TexelBytes;
   const GLint srcWidthNB = srcWidth - 2 * border;  /* sizes w/out border */
   const GLint srcDepthNB = srcDepth - 2 * border;
   const GLint dstWidthNB = dstWidth - 2 * border;
   const GLint dstHeightNB = dstHeight - 2 * border;
   const GLint dstDepthNB = dstDepth - 2 * border;
   GLvoid *tmpRowA, *tmpRowB;
   GLint img, row;
   GLint bytesPerSrcImage, bytesPerDstImage;
   GLint bytesPerSrcRow, bytesPerDstRow;
   GLint srcImageOffset, srcRowOffset;

   (void) srcDepthNB; /* silence warnings */

   /* Need two temporary row buffers */
   tmpRowA = _mesa_malloc(srcWidth * bpt);
   if (!tmpRowA)
      return;
   tmpRowB = _mesa_malloc(srcWidth * bpt);
   if (!tmpRowB) {
      _mesa_free(tmpRowA);
      return;
   }

   bytesPerSrcImage = srcWidth * srcHeight * bpt;
   bytesPerDstImage = dstWidth * dstHeight * bpt;

   bytesPerSrcRow = srcWidth * bpt;
   bytesPerDstRow = dstWidth * bpt;

   /* Offset between adjacent src images to be averaged together */
   srcImageOffset = (srcDepth == dstDepth) ? 0 : bytesPerSrcImage;

   /* Offset between adjacent src rows to be averaged together */
   srcRowOffset = (srcHeight == dstHeight) ? 0 : srcWidth * bpt;

   /*
    * Need to average together up to 8 src pixels for each dest pixel.
    * Break that down into 3 operations:
    *   1. take two rows from source image and average them together.
    *   2. take two rows from next source image and average them together.
    *   3. take the two averaged rows and average them for the final dst row.
    */

   /*
   _mesa_printf("mip3d %d x %d x %d  ->  %d x %d x %d\n",
          srcWidth, srcHeight, srcDepth, dstWidth, dstHeight, dstDepth);
   */

   for (img = 0; img < dstDepthNB; img++) {
      /* first source image pointer, skipping border */
      const GLubyte *imgSrcA = srcPtr
         + (bytesPerSrcImage + bytesPerSrcRow + border) * bpt * border
         + img * (bytesPerSrcImage + srcImageOffset);
      /* second source image pointer, skipping border */
      const GLubyte *imgSrcB = imgSrcA + srcImageOffset;
      /* address of the dest image, skipping border */
      GLubyte *imgDst = dstPtr
         + (bytesPerDstImage + bytesPerDstRow + border) * bpt * border
         + img * bytesPerDstImage;

      /* setup the four source row pointers and the dest row pointer */
      const GLubyte *srcImgARowA = imgSrcA;
      const GLubyte *srcImgARowB = imgSrcA + srcRowOffset;
      const GLubyte *srcImgBRowA = imgSrcB;
      const GLubyte *srcImgBRowB = imgSrcB + srcRowOffset;
      GLubyte *dstImgRow = imgDst;

      for (row = 0; row < dstHeightNB; row++) {
         /* Average together two rows from first src image */
         do_row(format, srcWidthNB, srcImgARowA, srcImgARowB,
                srcWidthNB, tmpRowA);
         /* Average together two rows from second src image */
         do_row(format, srcWidthNB, srcImgBRowA, srcImgBRowB,
                srcWidthNB, tmpRowB);
         /* Average together the temp rows to make the final row */
         do_row(format, srcWidthNB, tmpRowA, tmpRowB,
                dstWidthNB, dstImgRow);
         /* advance to next rows */
         srcImgARowA += bytesPerSrcRow + srcRowOffset;
         srcImgARowB += bytesPerSrcRow + srcRowOffset;
         srcImgBRowA += bytesPerSrcRow + srcRowOffset;
         srcImgBRowB += bytesPerSrcRow + srcRowOffset;
         dstImgRow += bytesPerDstRow;
      }
   }

   _mesa_free(tmpRowA);
   _mesa_free(tmpRowB);

   /* Luckily we can leverage the make_2d_mipmap() function here! */
   if (border > 0) {
      /* do front border image */
      make_2d_mipmap(format, 1, srcWidth, srcHeight, srcPtr, srcRowStride,
                     dstWidth, dstHeight, dstPtr, dstRowStride);
      /* do back border image */
      make_2d_mipmap(format, 1, srcWidth, srcHeight,
                     srcPtr + bytesPerSrcImage * (srcDepth - 1), srcRowStride,
                     dstWidth, dstHeight,
                     dstPtr + bytesPerDstImage * (dstDepth - 1), dstRowStride);
      /* do four remaining border edges that span the image slices */
      if (srcDepth == dstDepth) {
         /* just copy border pixels from src to dst */
         for (img = 0; img < dstDepthNB; img++) {
            const GLubyte *src;
            GLubyte *dst;

            /* do border along [img][row=0][col=0] */
            src = srcPtr + (img + 1) * bytesPerSrcImage;
            dst = dstPtr + (img + 1) * bytesPerDstImage;
            MEMCPY(dst, src, bpt);

            /* do border along [img][row=dstHeight-1][col=0] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (srcHeight - 1) * bytesPerSrcRow;
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (dstHeight - 1) * bytesPerDstRow;
            MEMCPY(dst, src, bpt);

            /* do border along [img][row=0][col=dstWidth-1] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (srcWidth - 1) * bpt;
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (dstWidth - 1) * bpt;
            MEMCPY(dst, src, bpt);

            /* do border along [img][row=dstHeight-1][col=dstWidth-1] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (bytesPerSrcImage - bpt);
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (bytesPerDstImage - bpt);
            MEMCPY(dst, src, bpt);
         }
      }
      else {
         /* average border pixels from adjacent src image pairs */
         ASSERT(srcDepthNB == 2 * dstDepthNB);
         for (img = 0; img < dstDepthNB; img++) {
            const GLubyte *src;
            GLubyte *dst;

            /* do border along [img][row=0][col=0] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage;
            dst = dstPtr + (img + 1) * bytesPerDstImage;
            do_row(format, 1, src, src + srcImageOffset, 1, dst);

            /* do border along [img][row=dstHeight-1][col=0] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (srcHeight - 1) * bytesPerSrcRow;
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (dstHeight - 1) * bytesPerDstRow;
            do_row(format, 1, src, src + srcImageOffset, 1, dst);

            /* do border along [img][row=0][col=dstWidth-1] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (srcWidth - 1) * bpt;
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (dstWidth - 1) * bpt;
            do_row(format, 1, src, src + srcImageOffset, 1, dst);

            /* do border along [img][row=dstHeight-1][col=dstWidth-1] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (bytesPerSrcImage - bpt);
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (bytesPerDstImage - bpt);
            do_row(format, 1, src, src + srcImageOffset, 1, dst);
         }
      }
   }
}


static void
make_1d_stack_mipmap(const struct gl_texture_format *format, GLint border,
                     GLint srcWidth, const GLubyte *srcPtr, GLuint srcRowStride,
                     GLint dstWidth, GLint dstHeight,
		     GLubyte *dstPtr, GLuint dstRowStride )
{
   const GLint bpt = format->TexelBytes;
   const GLint srcWidthNB = srcWidth - 2 * border;  /* sizes w/out border */
   const GLint dstWidthNB = dstWidth - 2 * border;
   const GLint dstHeightNB = dstHeight - 2 * border;
   const GLint srcRowBytes = bpt * srcRowStride;
   const GLint dstRowBytes = bpt * dstRowStride;
   const GLubyte *src;
   GLubyte *dst;
   GLint row;

   /* Compute src and dst pointers, skipping any border */
   src = srcPtr + border * ((srcWidth + 1) * bpt);
   dst = dstPtr + border * ((dstWidth + 1) * bpt);

   for (row = 0; row < dstHeightNB; row++) {
      do_row(format, srcWidthNB, src, src,
             dstWidthNB, dst);
      src += srcRowBytes;
      dst += dstRowBytes;
   }

   if (border) {
      /* copy left-most pixel from source */
      MEMCPY(dstPtr, srcPtr, bpt);
      /* copy right-most pixel from source */
      MEMCPY(dstPtr + (dstWidth - 1) * bpt,
             srcPtr + (srcWidth - 1) * bpt,
             bpt);
   }
}


/**
 * \bugs
 * There is quite a bit of refactoring that could be done with this function
 * and \c make_2d_mipmap.
 */
static void
make_2d_stack_mipmap(const struct gl_texture_format *format, GLint border,
                     GLint srcWidth, GLint srcHeight,
		     const GLubyte *srcPtr, GLint srcRowStride,
                     GLint dstWidth, GLint dstHeight, GLint dstDepth,
                     GLubyte *dstPtr, GLint dstRowStride)
{
   const GLint bpt = format->TexelBytes;
   const GLint srcWidthNB = srcWidth - 2 * border;  /* sizes w/out border */
   const GLint dstWidthNB = dstWidth - 2 * border;
   const GLint dstHeightNB = dstHeight - 2 * border;
   const GLint dstDepthNB = dstDepth - 2 * border;
   const GLint srcRowBytes = bpt * srcRowStride;
   const GLint dstRowBytes = bpt * dstRowStride;
   const GLubyte *srcA, *srcB;
   GLubyte *dst;
   GLint layer;
   GLint row;

   /* Compute src and dst pointers, skipping any border */
   srcA = srcPtr + border * ((srcWidth + 1) * bpt);
   if (srcHeight > 1) 
      srcB = srcA + srcRowBytes;
   else
      srcB = srcA;
   dst = dstPtr + border * ((dstWidth + 1) * bpt);

   for (layer = 0; layer < dstDepthNB; layer++) {
      for (row = 0; row < dstHeightNB; row++) {
         do_row(format, srcWidthNB, srcA, srcB,
                dstWidthNB, dst);
         srcA += 2 * srcRowBytes;
         srcB += 2 * srcRowBytes;
         dst += dstRowBytes;
      }

      /* This is ugly but probably won't be used much */
      if (border > 0) {
         /* fill in dest border */
         /* lower-left border pixel */
         MEMCPY(dstPtr, srcPtr, bpt);
         /* lower-right border pixel */
         MEMCPY(dstPtr + (dstWidth - 1) * bpt,
                srcPtr + (srcWidth - 1) * bpt, bpt);
         /* upper-left border pixel */
         MEMCPY(dstPtr + dstWidth * (dstHeight - 1) * bpt,
                srcPtr + srcWidth * (srcHeight - 1) * bpt, bpt);
         /* upper-right border pixel */
         MEMCPY(dstPtr + (dstWidth * dstHeight - 1) * bpt,
                srcPtr + (srcWidth * srcHeight - 1) * bpt, bpt);
         /* lower border */
         do_row(format, srcWidthNB,
                srcPtr + bpt,
                srcPtr + bpt,
                dstWidthNB, dstPtr + bpt);
         /* upper border */
         do_row(format, srcWidthNB,
                srcPtr + (srcWidth * (srcHeight - 1) + 1) * bpt,
                srcPtr + (srcWidth * (srcHeight - 1) + 1) * bpt,
                dstWidthNB,
                dstPtr + (dstWidth * (dstHeight - 1) + 1) * bpt);
         /* left and right borders */
         if (srcHeight == dstHeight) {
            /* copy border pixel from src to dst */
            for (row = 1; row < srcHeight; row++) {
               MEMCPY(dstPtr + dstWidth * row * bpt,
                      srcPtr + srcWidth * row * bpt, bpt);
               MEMCPY(dstPtr + (dstWidth * row + dstWidth - 1) * bpt,
                      srcPtr + (srcWidth * row + srcWidth - 1) * bpt, bpt);
            }
         }
         else {
            /* average two src pixels each dest pixel */
            for (row = 0; row < dstHeightNB; row += 2) {
               do_row(format, 1,
                      srcPtr + (srcWidth * (row * 2 + 1)) * bpt,
                      srcPtr + (srcWidth * (row * 2 + 2)) * bpt,
                      1, dstPtr + (dstWidth * row + 1) * bpt);
               do_row(format, 1,
                      srcPtr + (srcWidth * (row * 2 + 1) + srcWidth - 1) * bpt,
                      srcPtr + (srcWidth * (row * 2 + 2) + srcWidth - 1) * bpt,
                      1, dstPtr + (dstWidth * row + 1 + dstWidth - 1) * bpt);
            }
         }
      }
   }
}


/**
 * For GL_SGIX_generate_mipmap:
 * Generate a complete set of mipmaps from texObj's base-level image.
 * Stop at texObj's MaxLevel or when we get to the 1x1 texture.
 */
void
_mesa_generate_mipmap(GLcontext *ctx, GLenum target,
                      struct gl_texture_object *texObj)
{
   const struct gl_texture_image *srcImage;
   const struct gl_texture_format *convertFormat;
   const GLubyte *srcData = NULL;
   GLubyte *dstData = NULL;
   GLint level, maxLevels;

   ASSERT(texObj);
   /* XXX choose cube map face here??? */
   srcImage = texObj->Image[0][texObj->BaseLevel];
   ASSERT(srcImage);

   maxLevels = _mesa_max_texture_levels(ctx, texObj->Target);
   ASSERT(maxLevels > 0);  /* bad target */

   /* Find convertFormat - the format that do_row() will process */
   if (srcImage->IsCompressed) {
      /* setup for compressed textures */
      GLuint row;
      GLint  components, size;
      GLchan *dst;

      assert(texObj->Target == GL_TEXTURE_2D ||
             texObj->Target == GL_TEXTURE_CUBE_MAP_ARB);

      if (srcImage->_BaseFormat == GL_RGB) {
         convertFormat = &_mesa_texformat_rgb;
         components = 3;
      }
      else if (srcImage->_BaseFormat == GL_RGBA) {
         convertFormat = &_mesa_texformat_rgba;
         components = 4;
      }
      else {
         _mesa_problem(ctx, "bad srcImage->_BaseFormat in _mesa_generate_mipmaps");
         return;
      }

      /* allocate storage for uncompressed GL_RGB or GL_RGBA images */
      size = _mesa_bytes_per_pixel(srcImage->_BaseFormat, CHAN_TYPE)
         * srcImage->Width * srcImage->Height * srcImage->Depth + 20;
      /* 20 extra bytes, just be safe when calling last FetchTexel */
      srcData = (GLubyte *) _mesa_malloc(size);
      if (!srcData) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "generate mipmaps");
         return;
      }
      dstData = (GLubyte *) _mesa_malloc(size / 2);  /* 1/4 would probably be OK */
      if (!dstData) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "generate mipmaps");
         _mesa_free((void *) srcData);
         return;
      }

      /* decompress base image here */
      dst = (GLchan *) srcData;
      for (row = 0; row < srcImage->Height; row++) {
         GLuint col;
         for (col = 0; col < srcImage->Width; col++) {
            srcImage->FetchTexelc(srcImage, col, row, 0, dst);
            dst += components;
         }
      }
   }
   else {
      /* uncompressed */
      convertFormat = srcImage->TexFormat;
   }

   for (level = texObj->BaseLevel; level < texObj->MaxLevel
           && level < maxLevels - 1; level++) {
      /* generate image[level+1] from image[level] */
      const struct gl_texture_image *srcImage;
      struct gl_texture_image *dstImage;
      GLint srcWidth, srcHeight, srcDepth;
      GLint dstWidth, dstHeight, dstDepth;
      GLint border, bytesPerTexel;

      /* get src image parameters */
      srcImage = _mesa_select_tex_image(ctx, texObj, target, level);
      ASSERT(srcImage);
      srcWidth = srcImage->Width;
      srcHeight = srcImage->Height;
      srcDepth = srcImage->Depth;
      border = srcImage->Border;

      /* compute next (level+1) image size */
      if (srcWidth - 2 * border > 1) {
         dstWidth = (srcWidth - 2 * border) / 2 + 2 * border;
      }
      else {
         dstWidth = srcWidth; /* can't go smaller */
      }
      if ((srcHeight - 2 * border > 1) && 
          (texObj->Target != GL_TEXTURE_1D_ARRAY_EXT)) {
         dstHeight = (srcHeight - 2 * border) / 2 + 2 * border;
      }
      else {
         dstHeight = srcHeight; /* can't go smaller */
      }
      if ((srcDepth - 2 * border > 1) &&
               (texObj->Target != GL_TEXTURE_2D_ARRAY_EXT)) {
         dstDepth = (srcDepth - 2 * border) / 2 + 2 * border;
      }
      else {
         dstDepth = srcDepth; /* can't go smaller */
      }

      if (dstWidth == srcWidth &&
          dstHeight == srcHeight &&
          dstDepth == srcDepth) {
         /* all done */
         if (srcImage->IsCompressed) {
            _mesa_free((void *) srcData);
            _mesa_free(dstData);
         }
         return;
      }

      /* get dest gl_texture_image */
      dstImage = _mesa_get_tex_image(ctx, texObj, target, level + 1);
      if (!dstImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "generating mipmaps");
         return;
      }

      if (dstImage->ImageOffsets)
         _mesa_free(dstImage->ImageOffsets);

      /* Free old image data */
      if (dstImage->Data)
         ctx->Driver.FreeTexImageData(ctx, dstImage);

      /* initialize new image */
      _mesa_init_teximage_fields(ctx, target, dstImage, dstWidth, dstHeight,
                                 dstDepth, border, srcImage->InternalFormat);
      dstImage->DriverData = NULL;
      dstImage->TexFormat = srcImage->TexFormat;
      dstImage->FetchTexelc = srcImage->FetchTexelc;
      dstImage->FetchTexelf = srcImage->FetchTexelf;
      dstImage->IsCompressed = srcImage->IsCompressed;
      if (dstImage->IsCompressed) {
         dstImage->CompressedSize
            = ctx->Driver.CompressedTextureSize(ctx, dstImage->Width,
                                              dstImage->Height,
                                              dstImage->Depth,
                                              dstImage->TexFormat->MesaFormat);
         ASSERT(dstImage->CompressedSize > 0);
      }

      ASSERT(dstImage->TexFormat);
      ASSERT(dstImage->FetchTexelc);
      ASSERT(dstImage->FetchTexelf);

      /* Alloc new teximage data buffer.
       * Setup src and dest data pointers.
       */
      if (dstImage->IsCompressed) {
         dstImage->Data = _mesa_alloc_texmemory(dstImage->CompressedSize);
         if (!dstImage->Data) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "generating mipmaps");
            return;
         }
         /* srcData and dstData are already set */
         ASSERT(srcData);
         ASSERT(dstData);
      }
      else {
         bytesPerTexel = dstImage->TexFormat->TexelBytes;
         ASSERT(dstWidth * dstHeight * dstDepth * bytesPerTexel > 0);
         dstImage->Data = _mesa_alloc_texmemory(dstWidth * dstHeight
                                                * dstDepth * bytesPerTexel);
         if (!dstImage->Data) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "generating mipmaps");
            return;
         }
         srcData = (const GLubyte *) srcImage->Data;
         dstData = (GLubyte *) dstImage->Data;
      }

      /*
       * We use simple 2x2 averaging to compute the next mipmap level.
       */
      switch (target) {
         case GL_TEXTURE_1D:
            make_1d_mipmap(convertFormat, border,
                           srcWidth, srcData,
                           dstWidth, dstData);
            break;
         case GL_TEXTURE_2D:
         case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
         case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
         case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
         case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
         case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
         case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
            make_2d_mipmap(convertFormat, border,
                           srcWidth, srcHeight, srcData, srcImage->RowStride,
                           dstWidth, dstHeight, dstData, dstImage->RowStride);
            break;
         case GL_TEXTURE_3D:
            make_3d_mipmap(convertFormat, border,
                           srcWidth, srcHeight, srcDepth,
			   srcData, srcImage->RowStride,
                           dstWidth, dstHeight, dstDepth,
			   dstData, dstImage->RowStride);
            break;
         case GL_TEXTURE_1D_ARRAY_EXT:
            make_1d_stack_mipmap(convertFormat, border,
                                 srcWidth, srcData, srcImage->RowStride,
                                 dstWidth, dstHeight,
				 dstData, dstImage->RowStride);
            break;
         case GL_TEXTURE_2D_ARRAY_EXT:
            make_2d_stack_mipmap(convertFormat, border,
                                 srcWidth, srcHeight,
				 srcData, srcImage->RowStride,
                                 dstWidth, dstHeight,
				 dstDepth, dstData, dstImage->RowStride);
            break;
         case GL_TEXTURE_RECTANGLE_NV:
            /* no mipmaps, do nothing */
            break;
         default:
            _mesa_problem(ctx, "bad dimensions in _mesa_generate_mipmaps");
            return;
      }

      if (dstImage->IsCompressed) {
         GLubyte *temp;
         /* compress image from dstData into dstImage->Data */
         const GLenum srcFormat = convertFormat->BaseFormat;
         GLint dstRowStride
            = _mesa_compressed_row_stride(dstImage->TexFormat->MesaFormat, dstWidth);
         ASSERT(srcFormat == GL_RGB || srcFormat == GL_RGBA);
         dstImage->TexFormat->StoreImage(ctx, 2, dstImage->_BaseFormat,
                                         dstImage->TexFormat,
                                         dstImage->Data,
                                         0, 0, 0, /* dstX/Y/Zoffset */
                                         dstRowStride, 0, /* strides */
                                         dstWidth, dstHeight, 1, /* size */
                                         srcFormat, CHAN_TYPE,
                                         dstData, /* src data, actually */
                                         &ctx->DefaultPacking);
         /* swap src and dest pointers */
         temp = (GLubyte *) srcData;
         srcData = dstData;
         dstData = temp;
      }

   } /* loop over mipmap levels */
}


/**
 * Helper function for drivers which need to rescale texture images to
 * certain aspect ratios.
 * Nearest filtering only (for broken hardware that can't support
 * all aspect ratios).  This can be made a lot faster, but I don't
 * really care enough...
 */
void
_mesa_rescale_teximage2d(GLuint bytesPerPixel,
			 GLuint srcStrideInPixels,
			 GLuint dstRowStride,
			 GLint srcWidth, GLint srcHeight,
			 GLint dstWidth, GLint dstHeight,
			 const GLvoid *srcImage, GLvoid *dstImage)
{
   GLint row, col;

#define INNER_LOOP( TYPE, HOP, WOP )					\
   for ( row = 0 ; row < dstHeight ; row++ ) {				\
      GLint srcRow = row HOP hScale;					\
      for ( col = 0 ; col < dstWidth ; col++ ) {			\
	 GLint srcCol = col WOP wScale;					\
	 dst[col] = src[srcRow * srcStrideInPixels + srcCol];		\
      }									\
      dst = (TYPE *) ((GLubyte *) dst + dstRowStride);			\
   }									\

#define RESCALE_IMAGE( TYPE )						\
do {									\
   const TYPE *src = (const TYPE *)srcImage;				\
   TYPE *dst = (TYPE *)dstImage;					\
									\
   if ( srcHeight < dstHeight ) {					\
      const GLint hScale = dstHeight / srcHeight;			\
      if ( srcWidth < dstWidth ) {					\
	 const GLint wScale = dstWidth / srcWidth;			\
	 INNER_LOOP( TYPE, /, / );					\
      }									\
      else {								\
	 const GLint wScale = srcWidth / dstWidth;			\
	 INNER_LOOP( TYPE, /, * );					\
      }									\
   }									\
   else {								\
      const GLint hScale = srcHeight / dstHeight;			\
      if ( srcWidth < dstWidth ) {					\
	 const GLint wScale = dstWidth / srcWidth;			\
	 INNER_LOOP( TYPE, *, / );					\
      }									\
      else {								\
	 const GLint wScale = srcWidth / dstWidth;			\
	 INNER_LOOP( TYPE, *, * );					\
      }									\
   }									\
} while (0)

   switch ( bytesPerPixel ) {
   case 4:
      RESCALE_IMAGE( GLuint );
      break;

   case 2:
      RESCALE_IMAGE( GLushort );
      break;

   case 1:
      RESCALE_IMAGE( GLubyte );
      break;
   default:
      _mesa_problem(NULL,"unexpected bytes/pixel in _mesa_rescale_teximage2d");
   }
}


/**
 * Upscale an image by replication, not (typical) stretching.
 * We use this when the image width or height is less than a
 * certain size (4, 8) and we need to upscale an image.
 */
void
_mesa_upscale_teximage2d(GLsizei inWidth, GLsizei inHeight,
                         GLsizei outWidth, GLsizei outHeight,
                         GLint comps, const GLchan *src, GLint srcRowStride,
                         GLchan *dest )
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

