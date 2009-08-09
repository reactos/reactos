/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
#include "bufferobj.h"
#include "colormac.h"
#include "context.h"
#include "image.h"
#include "histogram.h"



/*
 * XXX the packed pixel formats haven't been tested.
 */
static void
pack_histogram( GLcontext *ctx,
                GLuint n, CONST GLuint rgba[][4],
                GLenum format, GLenum type, GLvoid *destination,
                const struct gl_pixelstore_attrib *packing )
{
   const GLint comps = _mesa_components_in_format(format);
   GLuint luminance[MAX_WIDTH];

   if (format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA) {
      GLuint i;
      for (i = 0; i < n; i++) {
         luminance[i] = rgba[i][RCOMP] + rgba[i][GCOMP] + rgba[i][BCOMP];
      }
   }

#define PACK_MACRO(TYPE)					\
   {								\
      GLuint i;							\
      switch (format) {						\
         case GL_RED:						\
            for (i=0;i<n;i++)					\
               dst[i] = (TYPE) rgba[i][RCOMP];			\
            break;						\
         case GL_GREEN:						\
            for (i=0;i<n;i++)					\
               dst[i] = (TYPE) rgba[i][GCOMP];			\
            break;						\
         case GL_BLUE:						\
            for (i=0;i<n;i++)					\
               dst[i] = (TYPE) rgba[i][BCOMP];			\
            break;						\
         case GL_ALPHA:						\
            for (i=0;i<n;i++)					\
               dst[i] = (TYPE) rgba[i][ACOMP];			\
            break;						\
         case GL_LUMINANCE:					\
            for (i=0;i<n;i++)					\
               dst[i] = (TYPE) luminance[i];			\
            break;						\
         case GL_LUMINANCE_ALPHA:				\
            for (i=0;i<n;i++) {					\
               dst[i*2+0] = (TYPE) luminance[i];		\
               dst[i*2+1] = (TYPE) rgba[i][ACOMP];		\
            }							\
            break;						\
         case GL_RGB:						\
            for (i=0;i<n;i++) {					\
               dst[i*3+0] = (TYPE) rgba[i][RCOMP];		\
               dst[i*3+1] = (TYPE) rgba[i][GCOMP];		\
               dst[i*3+2] = (TYPE) rgba[i][BCOMP];		\
            }							\
            break;						\
         case GL_RGBA:						\
            for (i=0;i<n;i++) {					\
               dst[i*4+0] = (TYPE) rgba[i][RCOMP];		\
               dst[i*4+1] = (TYPE) rgba[i][GCOMP];		\
               dst[i*4+2] = (TYPE) rgba[i][BCOMP];		\
               dst[i*4+3] = (TYPE) rgba[i][ACOMP];		\
            }							\
            break;						\
         case GL_BGR:						\
            for (i=0;i<n;i++) {					\
               dst[i*3+0] = (TYPE) rgba[i][BCOMP];		\
               dst[i*3+1] = (TYPE) rgba[i][GCOMP];		\
               dst[i*3+2] = (TYPE) rgba[i][RCOMP];		\
            }							\
            break;						\
         case GL_BGRA:						\
            for (i=0;i<n;i++) {					\
               dst[i*4+0] = (TYPE) rgba[i][BCOMP];		\
               dst[i*4+1] = (TYPE) rgba[i][GCOMP];		\
               dst[i*4+2] = (TYPE) rgba[i][RCOMP];		\
               dst[i*4+3] = (TYPE) rgba[i][ACOMP];		\
            }							\
            break;						\
         case GL_ABGR_EXT:					\
            for (i=0;i<n;i++) {					\
               dst[i*4+0] = (TYPE) rgba[i][ACOMP];		\
               dst[i*4+1] = (TYPE) rgba[i][BCOMP];		\
               dst[i*4+2] = (TYPE) rgba[i][GCOMP];		\
               dst[i*4+3] = (TYPE) rgba[i][RCOMP];		\
            }							\
            break;						\
         default:						\
            _mesa_problem(ctx, "bad format in pack_histogram");	\
      }								\
   }

   switch (type) {
      case GL_UNSIGNED_BYTE:
         {
            GLubyte *dst = (GLubyte *) destination;
            PACK_MACRO(GLubyte);
         }
         break;
      case GL_BYTE:
         {
            GLbyte *dst = (GLbyte *) destination;
            PACK_MACRO(GLbyte);
         }
         break;
      case GL_UNSIGNED_SHORT:
         {
            GLushort *dst = (GLushort *) destination;
            PACK_MACRO(GLushort);
            if (packing->SwapBytes) {
               _mesa_swap2(dst, n * comps);
            }
         }
         break;
      case GL_SHORT:
         {
            GLshort *dst = (GLshort *) destination;
            PACK_MACRO(GLshort);
            if (packing->SwapBytes) {
               _mesa_swap2((GLushort *) dst, n * comps);
            }
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint *dst = (GLuint *) destination;
            PACK_MACRO(GLuint);
            if (packing->SwapBytes) {
               _mesa_swap4(dst, n * comps);
            }
         }
         break;
      case GL_INT:
         {
            GLint *dst = (GLint *) destination;
            PACK_MACRO(GLint);
            if (packing->SwapBytes) {
               _mesa_swap4((GLuint *) dst, n * comps);
            }
         }
         break;
      case GL_FLOAT:
         {
            GLfloat *dst = (GLfloat *) destination;
            PACK_MACRO(GLfloat);
            if (packing->SwapBytes) {
               _mesa_swap4((GLuint *) dst, n * comps);
            }
         }
         break;
      case GL_HALF_FLOAT_ARB:
         {
            /* temporarily store as GLuints */
            GLuint temp[4*HISTOGRAM_TABLE_SIZE];
            GLhalfARB *dst = (GLhalfARB *) destination;
            GLuint i;
            /* get GLuint values */
            PACK_MACRO(GLuint);
            /* convert to GLhalf */
            for (i = 0; i < n * comps; i++) {
               dst[i] = _mesa_float_to_half((GLfloat) temp[i]);
            }
            if (packing->SwapBytes) {
               _mesa_swap2((GLushort *) dst, n * comps);
            }
         }
         break;
      case GL_UNSIGNED_BYTE_3_3_2:
         if (format == GL_RGB) {
            GLubyte *dst = (GLubyte *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][RCOMP] & 0x7) << 5)
                      | ((rgba[i][GCOMP] & 0x7) << 2)
                      | ((rgba[i][BCOMP] & 0x3)     );
            }
         }
         else {
            GLubyte *dst = (GLubyte *) destination;
            GLuint i;
            ASSERT(format == GL_BGR);
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][BCOMP] & 0x7) << 5)
                      | ((rgba[i][GCOMP] & 0x7) << 2)
                      | ((rgba[i][RCOMP] & 0x3)     );
            }
         }
         break;
      case GL_UNSIGNED_BYTE_2_3_3_REV:
         if (format == GL_RGB) {
            GLubyte *dst = (GLubyte *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][RCOMP] & 0x3) << 6)
                      | ((rgba[i][GCOMP] & 0x7) << 3)
                      | ((rgba[i][BCOMP] & 0x7)     );
            }
         }
         else {
            GLubyte *dst = (GLubyte *) destination;
            GLuint i;
            ASSERT(format == GL_BGR);
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][BCOMP] & 0x3) << 6)
                      | ((rgba[i][GCOMP] & 0x7) << 3)
                      | ((rgba[i][RCOMP] & 0x7)     );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_6_5:
         if (format == GL_RGB) {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][RCOMP] & 0x1f) << 11)
                      | ((rgba[i][GCOMP] & 0x3f) <<  5)
                      | ((rgba[i][BCOMP] & 0x1f)      );
            }
         }
         else {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            ASSERT(format == GL_BGR);
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][BCOMP] & 0x1f) << 11)
                      | ((rgba[i][GCOMP] & 0x3f) <<  5)
                      | ((rgba[i][RCOMP] & 0x1f)      );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_6_5_REV:
         if (format == GL_RGB) {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][BCOMP] & 0x1f) << 11)
                      | ((rgba[i][GCOMP] & 0x3f) <<  5)
                      | ((rgba[i][RCOMP] & 0x1f)      );
            }
         }
         else {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            ASSERT(format == GL_BGR);
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][RCOMP] & 0x1f) << 11)
                      | ((rgba[i][GCOMP] & 0x3f) <<  5)
                      | ((rgba[i][BCOMP] & 0x1f)      );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
         if (format == GL_RGBA) {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][RCOMP] & 0xf) << 12)
                      | ((rgba[i][GCOMP] & 0xf) <<  8)
                      | ((rgba[i][BCOMP] & 0xf) <<  4)
                      | ((rgba[i][ACOMP] & 0xf)      );
            }
         }
         else if (format == GL_BGRA) {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][BCOMP] & 0xf) << 12)
                      | ((rgba[i][GCOMP] & 0xf) <<  8)
                      | ((rgba[i][RCOMP] & 0xf) <<  4)
                      | ((rgba[i][ACOMP] & 0xf)      );
            }
         }
         else {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            ASSERT(format == GL_ABGR_EXT);
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][ACOMP] & 0xf) << 12)
                      | ((rgba[i][BCOMP] & 0xf) <<  8)
                      | ((rgba[i][GCOMP] & 0xf) <<  4)
                      | ((rgba[i][RCOMP] & 0xf)      );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
         if (format == GL_RGBA) {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][ACOMP] & 0xf) << 12)
                      | ((rgba[i][BCOMP] & 0xf) <<  8)
                      | ((rgba[i][GCOMP] & 0xf) <<  4)
                      | ((rgba[i][RCOMP] & 0xf)      );
            }
         }
         else if (format == GL_BGRA) {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][ACOMP] & 0xf) << 12)
                      | ((rgba[i][RCOMP] & 0xf) <<  8)
                      | ((rgba[i][GCOMP] & 0xf) <<  4)
                      | ((rgba[i][BCOMP] & 0xf)      );
            }
         }
         else {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            ASSERT(format == GL_ABGR_EXT);
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][RCOMP] & 0xf) << 12)
                      | ((rgba[i][GCOMP] & 0xf) <<  8)
                      | ((rgba[i][BCOMP] & 0xf) <<  4)
                      | ((rgba[i][ACOMP] & 0xf)      );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_5_5_1:
         if (format == GL_RGBA) {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][RCOMP] & 0x1f) << 11)
                      | ((rgba[i][GCOMP] & 0x1f) <<  6)
                      | ((rgba[i][BCOMP] & 0x1f) <<  1)
                      | ((rgba[i][ACOMP] & 0x1)       );
            }
         }
         else if (format == GL_BGRA) {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][BCOMP] & 0x1f) << 11)
                      | ((rgba[i][GCOMP] & 0x1f) <<  6)
                      | ((rgba[i][RCOMP] & 0x1f) <<  1)
                      | ((rgba[i][ACOMP] & 0x1)       );
            }
         }
         else {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            ASSERT(format == GL_ABGR_EXT);
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][ACOMP] & 0x1f) << 11)
                      | ((rgba[i][BCOMP] & 0x1f) <<  6)
                      | ((rgba[i][GCOMP] & 0x1f) <<  1)
                      | ((rgba[i][RCOMP] & 0x1)       );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
         if (format == GL_RGBA) {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][ACOMP] & 0x1f) << 11)
                      | ((rgba[i][BCOMP] & 0x1f) <<  6)
                      | ((rgba[i][GCOMP] & 0x1f) <<  1)
                      | ((rgba[i][RCOMP] & 0x1)       );
            }
         }
         else if (format == GL_BGRA) {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][ACOMP] & 0x1f) << 11)
                      | ((rgba[i][RCOMP] & 0x1f) <<  6)
                      | ((rgba[i][GCOMP] & 0x1f) <<  1)
                      | ((rgba[i][BCOMP] & 0x1)       );
            }
         }
         else {
            GLushort *dst = (GLushort *) destination;
            GLuint i;
            ASSERT(format == GL_ABGR_EXT);
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][RCOMP] & 0x1f) << 11)
                      | ((rgba[i][GCOMP] & 0x1f) <<  6)
                      | ((rgba[i][BCOMP] & 0x1f) <<  1)
                      | ((rgba[i][ACOMP] & 0x1)       );
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8:
         if (format == GL_RGBA) {
            GLuint *dst = (GLuint *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][RCOMP] & 0xff) << 24)
                      | ((rgba[i][GCOMP] & 0xff) << 16)
                      | ((rgba[i][BCOMP] & 0xff) <<  8)
                      | ((rgba[i][ACOMP] & 0xff)      );
            }
         }
         else if (format == GL_BGRA) {
            GLuint *dst = (GLuint *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][BCOMP] & 0xff) << 24)
                      | ((rgba[i][GCOMP] & 0xff) << 16)
                      | ((rgba[i][RCOMP] & 0xff) <<  8)
                      | ((rgba[i][ACOMP] & 0xff)      );
            }
         }
         else {
            GLuint *dst = (GLuint *) destination;
            GLuint i;
            ASSERT(format == GL_ABGR_EXT);
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][ACOMP] & 0xff) << 24)
                      | ((rgba[i][BCOMP] & 0xff) << 16)
                      | ((rgba[i][GCOMP] & 0xff) <<  8)
                      | ((rgba[i][RCOMP] & 0xff)      );
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8_REV:
         if (format == GL_RGBA) {
            GLuint *dst = (GLuint *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][ACOMP] & 0xff) << 24)
                      | ((rgba[i][BCOMP] & 0xff) << 16)
                      | ((rgba[i][GCOMP] & 0xff) <<  8)
                      | ((rgba[i][RCOMP] & 0xff)      );
            }
         }
         else if (format == GL_BGRA) {
            GLuint *dst = (GLuint *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][ACOMP] & 0xff) << 24)
                      | ((rgba[i][RCOMP] & 0xff) << 16)
                      | ((rgba[i][GCOMP] & 0xff) <<  8)
                      | ((rgba[i][BCOMP] & 0xff)      );
            }
         }
         else {
            GLuint *dst = (GLuint *) destination;
            GLuint i;
            ASSERT(format == GL_ABGR_EXT);
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][RCOMP] & 0xff) << 24)
                      | ((rgba[i][GCOMP] & 0xff) << 16)
                      | ((rgba[i][BCOMP] & 0xff) <<  8)
                      | ((rgba[i][ACOMP] & 0xff)      );
            }
         }
         break;
      case GL_UNSIGNED_INT_10_10_10_2:
         if (format == GL_RGBA) {
            GLuint *dst = (GLuint *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][RCOMP] & 0x3ff) << 22)
                      | ((rgba[i][GCOMP] & 0x3ff) << 12)
                      | ((rgba[i][BCOMP] & 0x3ff) <<  2)
                      | ((rgba[i][ACOMP] & 0x3)        );
            }
         }
         else if (format == GL_BGRA) {
            GLuint *dst = (GLuint *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][BCOMP] & 0x3ff) << 22)
                      | ((rgba[i][GCOMP] & 0x3ff) << 12)
                      | ((rgba[i][RCOMP] & 0x3ff) <<  2)
                      | ((rgba[i][ACOMP] & 0x3)        );
            }
         }
         else {
            GLuint *dst = (GLuint *) destination;
            GLuint i;
            ASSERT(format == GL_ABGR_EXT);
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][ACOMP] & 0x3ff) << 22)
                      | ((rgba[i][BCOMP] & 0x3ff) << 12)
                      | ((rgba[i][GCOMP] & 0x3ff) <<  2)
                      | ((rgba[i][RCOMP] & 0x3)        );
            }
         }
         break;
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         if (format == GL_RGBA) {
            GLuint *dst = (GLuint *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][ACOMP] & 0x3ff) << 22)
                      | ((rgba[i][BCOMP] & 0x3ff) << 12)
                      | ((rgba[i][GCOMP] & 0x3ff) <<  2)
                      | ((rgba[i][RCOMP] & 0x3)        );
            }
         }
         else if (format == GL_BGRA) {
            GLuint *dst = (GLuint *) destination;
            GLuint i;
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][ACOMP] & 0x3ff) << 22)
                      | ((rgba[i][RCOMP] & 0x3ff) << 12)
                      | ((rgba[i][GCOMP] & 0x3ff) <<  2)
                      | ((rgba[i][BCOMP] & 0x3)        );
            }
         }
         else {
            GLuint *dst = (GLuint *) destination;
            GLuint i;
            ASSERT(format == GL_ABGR_EXT);
            for (i = 0; i < n; i++) {
               dst[i] = ((rgba[i][RCOMP] & 0x3ff) << 22)
                      | ((rgba[i][GCOMP] & 0x3ff) << 12)
                      | ((rgba[i][BCOMP] & 0x3ff) <<  2)
                      | ((rgba[i][ACOMP] & 0x3)        );
            }
         }
         break;
      default:
         _mesa_problem(ctx, "Bad type in pack_histogram");
   }

#undef PACK_MACRO
}


/*
 * Given an internalFormat token passed to glHistogram or glMinMax,
 * return the corresponding base format.
 * Return -1 if invalid token.
 */
static GLint
base_histogram_format( GLenum format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return GL_ALPHA;
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return GL_LUMINANCE;
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return GL_LUMINANCE_ALPHA;
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return GL_RGB;
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return GL_RGBA;
      default:
         return -1;  /* error */
   }
}



/**********************************************************************
 * API functions
 */


void GLAPIENTRY
_mesa_GetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (!ctx->Extensions.EXT_histogram && !ctx->Extensions.ARB_imaging) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetMinmax");
      return;
   }

   if (target != GL_MINMAX) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetMinmax(target)");
      return;
   }

   if (format != GL_RED &&
       format != GL_GREEN &&
       format != GL_BLUE &&
       format != GL_ALPHA &&
       format != GL_RGB &&
       format != GL_BGR &&
       format != GL_RGBA &&
       format != GL_BGRA &&
       format != GL_ABGR_EXT &&
       format != GL_LUMINANCE &&
       format != GL_LUMINANCE_ALPHA) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetMinMax(format)");
   }

   if (!_mesa_is_legal_format_and_type(ctx, format, type)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetMinmax(format or type)");
      return;
   }

   if (ctx->Pack.BufferObj->Name) {
      /* pack min/max values into a PBO */
      GLubyte *buf;
      if (!_mesa_validate_pbo_access(1, &ctx->Pack, 2, 1, 1,
                                     format, type, values)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetMinMax(invalid PBO access)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              ctx->Pack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,"glGetMinMax(PBO is mapped)");
         return;
      }
      values = ADD_POINTERS(buf, values);
   }
   else if (!values) {
      /* not an error */
      return;
   }

   {
      GLfloat minmax[2][4];
      minmax[0][RCOMP] = CLAMP(ctx->MinMax.Min[RCOMP], 0.0F, 1.0F);
      minmax[0][GCOMP] = CLAMP(ctx->MinMax.Min[GCOMP], 0.0F, 1.0F);
      minmax[0][BCOMP] = CLAMP(ctx->MinMax.Min[BCOMP], 0.0F, 1.0F);
      minmax[0][ACOMP] = CLAMP(ctx->MinMax.Min[ACOMP], 0.0F, 1.0F);
      minmax[1][RCOMP] = CLAMP(ctx->MinMax.Max[RCOMP], 0.0F, 1.0F);
      minmax[1][GCOMP] = CLAMP(ctx->MinMax.Max[GCOMP], 0.0F, 1.0F);
      minmax[1][BCOMP] = CLAMP(ctx->MinMax.Max[BCOMP], 0.0F, 1.0F);
      minmax[1][ACOMP] = CLAMP(ctx->MinMax.Max[ACOMP], 0.0F, 1.0F);
      _mesa_pack_rgba_span_float(ctx, 2, minmax,
                                 format, type, values, &ctx->Pack, 0x0);
   }

   if (ctx->Pack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              ctx->Pack.BufferObj);
   }

   if (reset) {
      _mesa_ResetMinmax(GL_MINMAX);
   }
}


void GLAPIENTRY
_mesa_GetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (!ctx->Extensions.EXT_histogram && !ctx->Extensions.ARB_imaging) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetHistogram");
      return;
   }

   if (target != GL_HISTOGRAM) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetHistogram(target)");
      return;
   }

   if (format != GL_RED &&
       format != GL_GREEN &&
       format != GL_BLUE &&
       format != GL_ALPHA &&
       format != GL_RGB &&
       format != GL_BGR &&
       format != GL_RGBA &&
       format != GL_BGRA &&
       format != GL_ABGR_EXT &&
       format != GL_LUMINANCE &&
       format != GL_LUMINANCE_ALPHA) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetHistogram(format)");
   }

   if (!_mesa_is_legal_format_and_type(ctx, format, type)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetHistogram(format or type)");
      return;
   }

   if (ctx->Pack.BufferObj->Name) {
      /* pack min/max values into a PBO */
      GLubyte *buf;
      if (!_mesa_validate_pbo_access(1, &ctx->Pack, ctx->Histogram.Width, 1, 1,
                                     format, type, values)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetHistogram(invalid PBO access)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              ctx->Pack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx,GL_INVALID_OPERATION,"glGetHistogram(PBO is mapped)");
         return;
      }
      values = ADD_POINTERS(buf, values);
   }
   else if (!values) {
      /* not an error */
      return;
   }

   pack_histogram(ctx, ctx->Histogram.Width,
                  (CONST GLuint (*)[4]) ctx->Histogram.Count,
                  format, type, values, &ctx->Pack);

   if (ctx->Pack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              ctx->Pack.BufferObj);
   }

   if (reset) {
      GLuint i;
      for (i = 0; i < HISTOGRAM_TABLE_SIZE; i++) {
         ctx->Histogram.Count[i][0] = 0;
         ctx->Histogram.Count[i][1] = 0;
         ctx->Histogram.Count[i][2] = 0;
         ctx->Histogram.Count[i][3] = 0;
      }
   }
}


void GLAPIENTRY
_mesa_GetHistogramParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.EXT_histogram && !ctx->Extensions.ARB_imaging) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetHistogramParameterfv");
      return;
   }

   if (target != GL_HISTOGRAM && target != GL_PROXY_HISTOGRAM) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetHistogramParameterfv(target)");
      return;
   }

   switch (pname) {
      case GL_HISTOGRAM_WIDTH:
         *params = (GLfloat) ctx->Histogram.Width;
         break;
      case GL_HISTOGRAM_FORMAT:
         *params = (GLfloat) ctx->Histogram.Format;
         break;
      case GL_HISTOGRAM_RED_SIZE:
         *params = (GLfloat) ctx->Histogram.RedSize;
         break;
      case GL_HISTOGRAM_GREEN_SIZE:
         *params = (GLfloat) ctx->Histogram.GreenSize;
         break;
      case GL_HISTOGRAM_BLUE_SIZE:
         *params = (GLfloat) ctx->Histogram.BlueSize;
         break;
      case GL_HISTOGRAM_ALPHA_SIZE:
         *params = (GLfloat) ctx->Histogram.AlphaSize;
         break;
      case GL_HISTOGRAM_LUMINANCE_SIZE:
         *params = (GLfloat) ctx->Histogram.LuminanceSize;
         break;
      case GL_HISTOGRAM_SINK:
         *params = (GLfloat) ctx->Histogram.Sink;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetHistogramParameterfv(pname)");
   }
}


void GLAPIENTRY
_mesa_GetHistogramParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.EXT_histogram && !ctx->Extensions.ARB_imaging) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetHistogramParameteriv");
      return;
   }

   if (target != GL_HISTOGRAM && target != GL_PROXY_HISTOGRAM) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetHistogramParameteriv(target)");
      return;
   }

   switch (pname) {
      case GL_HISTOGRAM_WIDTH:
         *params = (GLint) ctx->Histogram.Width;
         break;
      case GL_HISTOGRAM_FORMAT:
         *params = (GLint) ctx->Histogram.Format;
         break;
      case GL_HISTOGRAM_RED_SIZE:
         *params = (GLint) ctx->Histogram.RedSize;
         break;
      case GL_HISTOGRAM_GREEN_SIZE:
         *params = (GLint) ctx->Histogram.GreenSize;
         break;
      case GL_HISTOGRAM_BLUE_SIZE:
         *params = (GLint) ctx->Histogram.BlueSize;
         break;
      case GL_HISTOGRAM_ALPHA_SIZE:
         *params = (GLint) ctx->Histogram.AlphaSize;
         break;
      case GL_HISTOGRAM_LUMINANCE_SIZE:
         *params = (GLint) ctx->Histogram.LuminanceSize;
         break;
      case GL_HISTOGRAM_SINK:
         *params = (GLint) ctx->Histogram.Sink;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetHistogramParameteriv(pname)");
   }
}


void GLAPIENTRY
_mesa_GetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.EXT_histogram && !ctx->Extensions.ARB_imaging) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetMinmaxParameterfv");
      return;
   }
   if (target != GL_MINMAX) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetMinmaxParameterfv(target)");
      return;
   }
   if (pname == GL_MINMAX_FORMAT) {
      *params = (GLfloat) ctx->MinMax.Format;
   }
   else if (pname == GL_MINMAX_SINK) {
      *params = (GLfloat) ctx->MinMax.Sink;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetMinMaxParameterfv(pname)");
   }
}


void GLAPIENTRY
_mesa_GetMinmaxParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.EXT_histogram && !ctx->Extensions.ARB_imaging) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetMinmaxParameteriv");
      return;
   }
   if (target != GL_MINMAX) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetMinmaxParameteriv(target)");
      return;
   }
   if (pname == GL_MINMAX_FORMAT) {
      *params = (GLint) ctx->MinMax.Format;
   }
   else if (pname == GL_MINMAX_SINK) {
      *params = (GLint) ctx->MinMax.Sink;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetMinMaxParameteriv(pname)");
   }
}


void GLAPIENTRY
_mesa_Histogram(GLenum target, GLsizei width, GLenum internalFormat, GLboolean sink)
{
   GLuint i;
   GLboolean error = GL_FALSE;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx); /* sideeffects */

   if (!ctx->Extensions.EXT_histogram && !ctx->Extensions.ARB_imaging) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glHistogram");
      return;
   }

   if (target != GL_HISTOGRAM && target != GL_PROXY_HISTOGRAM) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glHistogram(target)");
      return;
   }

   if (width < 0 || width > HISTOGRAM_TABLE_SIZE) {
      if (target == GL_PROXY_HISTOGRAM) {
         error = GL_TRUE;
      }
      else {
         if (width < 0)
            _mesa_error(ctx, GL_INVALID_VALUE, "glHistogram(width)");
         else
            _mesa_error(ctx, GL_TABLE_TOO_LARGE, "glHistogram(width)");
         return;
      }
   }

   if (width != 0 && !_mesa_is_pow_two(width)) {
      if (target == GL_PROXY_HISTOGRAM) {
         error = GL_TRUE;
      }
      else {
         _mesa_error(ctx, GL_INVALID_VALUE, "glHistogram(width)");
         return;
      }
   }

   if (base_histogram_format(internalFormat) < 0) {
      if (target == GL_PROXY_HISTOGRAM) {
         error = GL_TRUE;
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glHistogram(internalFormat)");
         return;
      }
   }

   /* reset histograms */
   for (i = 0; i < HISTOGRAM_TABLE_SIZE; i++) {
      ctx->Histogram.Count[i][0] = 0;
      ctx->Histogram.Count[i][1] = 0;
      ctx->Histogram.Count[i][2] = 0;
      ctx->Histogram.Count[i][3] = 0;
   }

   if (error) {
      ctx->Histogram.Width = 0;
      ctx->Histogram.Format = 0;
      ctx->Histogram.RedSize       = 0;
      ctx->Histogram.GreenSize     = 0;
      ctx->Histogram.BlueSize      = 0;
      ctx->Histogram.AlphaSize     = 0;
      ctx->Histogram.LuminanceSize = 0;
   }
   else {
      ctx->Histogram.Width = width;
      ctx->Histogram.Format = internalFormat;
      ctx->Histogram.Sink = sink;
      ctx->Histogram.RedSize       = 8 * sizeof(GLuint);
      ctx->Histogram.GreenSize     = 8 * sizeof(GLuint);
      ctx->Histogram.BlueSize      = 8 * sizeof(GLuint);
      ctx->Histogram.AlphaSize     = 8 * sizeof(GLuint);
      ctx->Histogram.LuminanceSize = 8 * sizeof(GLuint);
   }

   ctx->NewState |= _NEW_PIXEL;
}


void GLAPIENTRY
_mesa_Minmax(GLenum target, GLenum internalFormat, GLboolean sink)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.EXT_histogram && !ctx->Extensions.ARB_imaging) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glMinmax");
      return;
   }

   if (target != GL_MINMAX) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMinMax(target)");
      return;
   }

   if (base_histogram_format(internalFormat) < 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMinMax(internalFormat)");
      return;
   }

   if (ctx->MinMax.Sink == sink)
      return;
   FLUSH_VERTICES(ctx, _NEW_PIXEL);
   ctx->MinMax.Sink = sink;
}


void GLAPIENTRY
_mesa_ResetHistogram(GLenum target)
{
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx); /* sideeffects */

   if (!ctx->Extensions.EXT_histogram && !ctx->Extensions.ARB_imaging) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glResetHistogram");
      return;
   }

   if (target != GL_HISTOGRAM) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glResetHistogram(target)");
      return;
   }

   for (i = 0; i < HISTOGRAM_TABLE_SIZE; i++) {
      ctx->Histogram.Count[i][0] = 0;
      ctx->Histogram.Count[i][1] = 0;
      ctx->Histogram.Count[i][2] = 0;
      ctx->Histogram.Count[i][3] = 0;
   }

   ctx->NewState |= _NEW_PIXEL;
}


void GLAPIENTRY
_mesa_ResetMinmax(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (!ctx->Extensions.EXT_histogram && !ctx->Extensions.ARB_imaging) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glResetMinmax");
      return;
   }

   if (target != GL_MINMAX) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glResetMinMax(target)");
      return;
   }

   ctx->MinMax.Min[RCOMP] = 1000;    ctx->MinMax.Max[RCOMP] = -1000;
   ctx->MinMax.Min[GCOMP] = 1000;    ctx->MinMax.Max[GCOMP] = -1000;
   ctx->MinMax.Min[BCOMP] = 1000;    ctx->MinMax.Max[BCOMP] = -1000;
   ctx->MinMax.Min[ACOMP] = 1000;    ctx->MinMax.Max[ACOMP] = -1000;
   ctx->NewState |= _NEW_PIXEL;
}



/**********************************************************************/
/*****                      Initialization                        *****/
/**********************************************************************/

void _mesa_init_histogram( GLcontext * ctx )
{
   int i;

   /* Histogram group */
   ctx->Histogram.Width = 0;
   ctx->Histogram.Format = GL_RGBA;
   ctx->Histogram.Sink = GL_FALSE;
   ctx->Histogram.RedSize       = 0;
   ctx->Histogram.GreenSize     = 0;
   ctx->Histogram.BlueSize      = 0;
   ctx->Histogram.AlphaSize     = 0;
   ctx->Histogram.LuminanceSize = 0;
   for (i = 0; i < HISTOGRAM_TABLE_SIZE; i++) {
      ctx->Histogram.Count[i][0] = 0;
      ctx->Histogram.Count[i][1] = 0;
      ctx->Histogram.Count[i][2] = 0;
      ctx->Histogram.Count[i][3] = 0;
   }

   /* Min/Max group */
   ctx->MinMax.Format = GL_RGBA;
   ctx->MinMax.Sink = GL_FALSE;
   ctx->MinMax.Min[RCOMP] = 1000;    ctx->MinMax.Max[RCOMP] = -1000;
   ctx->MinMax.Min[GCOMP] = 1000;    ctx->MinMax.Max[GCOMP] = -1000;
   ctx->MinMax.Min[BCOMP] = 1000;    ctx->MinMax.Max[BCOMP] = -1000;
   ctx->MinMax.Min[ACOMP] = 1000;    ctx->MinMax.Max[ACOMP] = -1000;
}
