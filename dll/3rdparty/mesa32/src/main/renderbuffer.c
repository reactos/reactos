/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * Functions for allocating/managing renderbuffers.
 * Also, routines for reading/writing software-based renderbuffer data as
 * ubytes, ushorts, uints, etc.
 *
 * The 'alpha8' renderbuffer is interesting.  It's used to add a software-based
 * alpha channel to RGB renderbuffers.  This is done by wrapping the RGB
 * renderbuffer with the alpha renderbuffer.  We can do this because of the
 * OO-nature of renderbuffers.
 *
 * Down the road we'll use this for run-time support of 8, 16 and 32-bit
 * color channels.  For example, Mesa may use 32-bit/float color channels
 * internally (swrast) and use wrapper renderbuffers to convert 32-bit
 * values down to 16 or 8-bit values for whatever kind of framebuffer we have.
 */


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "mtypes.h"
#include "fbobject.h"
#include "renderbuffer.h"

#include "rbadaptors.h"


/* 32-bit color index format.  Not a public format. */
#define COLOR_INDEX32 0x424243


/*
 * Routines for get/put values in common buffer formats follow.
 * Someday add support for arbitrary row stride to make them more
 * flexible.
 */

/**********************************************************************
 * Functions for buffers of 1 X GLubyte values.
 * Typically stencil.
 */

static void *
get_pointer_ubyte(GLcontext *ctx, struct gl_renderbuffer *rb,
                  GLint x, GLint y)
{
   if (!rb->Data)
      return NULL;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   /* Can't assert _ActualFormat since these funcs may be used for serveral
    * different formats (GL_ALPHA8, GL_STENCIL_INDEX8, etc).
    */
   return (GLubyte *) rb->Data + y * rb->Width + x;
}


static void
get_row_ubyte(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
              GLint x, GLint y, void *values)
{
   const GLubyte *src = (const GLubyte *) rb->Data + y * rb->Width + x;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   _mesa_memcpy(values, src, count * sizeof(GLubyte));
}


static void
get_values_ubyte(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                 const GLint x[], const GLint y[], void *values)
{
   GLubyte *dst = (GLubyte *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      const GLubyte *src = (GLubyte *) rb->Data + y[i] * rb->Width + x[i];
      dst[i] = *src;
   }
}


static void
put_row_ubyte(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
              GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const GLubyte *src = (const GLubyte *) values;
   GLubyte *dst = (GLubyte *) rb->Data + y * rb->Width + x;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i] = src[i];
         }
      }
   }
   else {
      _mesa_memcpy(dst, values, count * sizeof(GLubyte));
   }
}


static void
put_mono_row_ubyte(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   GLint x, GLint y, const void *value, const GLubyte *mask)
{
   const GLubyte val = *((const GLubyte *) value);
   GLubyte *dst = (GLubyte *) rb->Data + y * rb->Width + x;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i] = val;
         }
      }
   }
   else {
      GLuint i;
      for (i = 0; i < count; i++) {
         dst[i] = val;
      }
   }
}


static void
put_values_ubyte(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                 const GLint x[], const GLint y[],
                 const void *values, const GLubyte *mask)
{
   const GLubyte *src = (const GLubyte *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLubyte *dst = (GLubyte *) rb->Data + y[i] * rb->Width + x[i];
         *dst = src[i];
      }
   }
}


static void
put_mono_values_ubyte(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                      const GLint x[], const GLint y[],
                      const void *value, const GLubyte *mask)
{
   const GLubyte val = *((const GLubyte *) value);
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLubyte *dst = (GLubyte *) rb->Data + y[i] * rb->Width + x[i];
         *dst = val;
      }
   }
}


/**********************************************************************
 * Functions for buffers of 1 X GLushort values.
 * Typically depth/Z.
 */

static void *
get_pointer_ushort(GLcontext *ctx, struct gl_renderbuffer *rb,
                   GLint x, GLint y)
{
   if (!rb->Data)
      return NULL;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   ASSERT(rb->Width > 0);
   return (GLushort *) rb->Data + y * rb->Width + x;
}


static void
get_row_ushort(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, void *values)
{
   const void *src = rb->GetPointer(ctx, rb, x, y);
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   _mesa_memcpy(values, src, count * sizeof(GLushort));
}


static void
get_values_ushort(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], void *values)
{
   GLushort *dst = (GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   for (i = 0; i < count; i++) {
      const GLushort *src = (GLushort *) rb->Data + y[i] * rb->Width + x[i];
      dst[i] = *src;
   }
}


static void
put_row_ushort(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const GLushort *src = (const GLushort *) values;
   GLushort *dst = (GLushort *) rb->Data + y * rb->Width + x;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i] = src[i];
         }
      }
   }
   else {
      _mesa_memcpy(dst, src, count * sizeof(GLushort));
   }
}


static void
put_mono_row_ushort(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                    GLint x, GLint y, const void *value, const GLubyte *mask)
{
   const GLushort val = *((const GLushort *) value);
   GLushort *dst = (GLushort *) rb->Data + y * rb->Width + x;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i] = val;
         }
      }
   }
   else {
      GLuint i;
      for (i = 0; i < count; i++) {
         dst[i] = val;
      }
   }
}


static void
put_values_ushort(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], const void *values,
                  const GLubyte *mask)
{
   const GLushort *src = (const GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLushort *dst = (GLushort *) rb->Data + y[i] * rb->Width + x[i];
         *dst = src[i];
      }
   }
}
 

static void
put_mono_values_ushort(GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLuint count, const GLint x[], const GLint y[],
                       const void *value, const GLubyte *mask)
{
   const GLushort val = *((const GLushort *) value);
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            GLushort *dst = (GLushort *) rb->Data + y[i] * rb->Width + x[i];
            *dst = val;
         }
      }
   }
   else {
      GLuint i;
      for (i = 0; i < count; i++) {
         GLushort *dst = (GLushort *) rb->Data + y[i] * rb->Width + x[i];
         *dst = val;
      }
   }
}
 

/**********************************************************************
 * Functions for buffers of 1 X GLuint values.
 * Typically depth/Z or color index.
 */

static void *
get_pointer_uint(GLcontext *ctx, struct gl_renderbuffer *rb,
                 GLint x, GLint y)
{
   if (!rb->Data)
      return NULL;
   ASSERT(rb->DataType == GL_UNSIGNED_INT ||
          rb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   return (GLuint *) rb->Data + y * rb->Width + x;
}


static void
get_row_uint(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
             GLint x, GLint y, void *values)
{
   const void *src = rb->GetPointer(ctx, rb, x, y);
   ASSERT(rb->DataType == GL_UNSIGNED_INT ||
          rb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   _mesa_memcpy(values, src, count * sizeof(GLuint));
}


static void
get_values_uint(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                const GLint x[], const GLint y[], void *values)
{
   GLuint *dst = (GLuint *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_INT ||
          rb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   for (i = 0; i < count; i++) {
      const GLuint *src = (GLuint *) rb->Data + y[i] * rb->Width + x[i];
      dst[i] = *src;
   }
}


static void
put_row_uint(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
             GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const GLuint *src = (const GLuint *) values;
   GLuint *dst = (GLuint *) rb->Data + y * rb->Width + x;
   ASSERT(rb->DataType == GL_UNSIGNED_INT ||
          rb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i] = src[i];
         }
      }
   }
   else {
      _mesa_memcpy(dst, src, count * sizeof(GLuint));
   }
}


static void
put_mono_row_uint(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  GLint x, GLint y, const void *value, const GLubyte *mask)
{
   const GLuint val = *((const GLuint *) value);
   GLuint *dst = (GLuint *) rb->Data + y * rb->Width + x;
   ASSERT(rb->DataType == GL_UNSIGNED_INT ||
          rb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i] = val;
         }
      }
   }
   else {
      GLuint i;
      for (i = 0; i < count; i++) {
         dst[i] = val;
      }
   }
}


static void
put_values_uint(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                const GLint x[], const GLint y[], const void *values,
                const GLubyte *mask)
{
   const GLuint *src = (const GLuint *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_INT ||
          rb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLuint *dst = (GLuint *) rb->Data + y[i] * rb->Width + x[i];
         *dst = src[i];
      }
   }
}


static void
put_mono_values_uint(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                     const GLint x[], const GLint y[], const void *value,
                     const GLubyte *mask)
{
   const GLuint val = *((const GLuint *) value);
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_INT ||
          rb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLuint *dst = (GLuint *) rb->Data + y[i] * rb->Width + x[i];
         *dst = val;
      }
   }
}


/**********************************************************************
 * Functions for buffers of 3 X GLubyte (or GLbyte) values.
 * Typically color buffers.
 * NOTE: the incoming and outgoing colors are RGBA!  We ignore incoming
 * alpha values and return 255 for outgoing alpha values.
 */

static void *
get_pointer_ubyte3(GLcontext *ctx, struct gl_renderbuffer *rb,
                   GLint x, GLint y)
{
   ASSERT(rb->_ActualFormat == GL_RGB8);
   /* No direct access since this buffer is RGB but caller will be
    * treating it as if it were RGBA.
    */
   return NULL;
}


static void
get_row_ubyte3(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, void *values)
{
   const GLubyte *src = (const GLubyte *) rb->Data + 3 * (y * rb->Width + x);
   GLubyte *dst = (GLubyte *) values;
   GLuint i;
   ASSERT(rb->_ActualFormat == GL_RGB8);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      dst[i * 4 + 0] = src[i * 3 + 0];
      dst[i * 4 + 1] = src[i * 3 + 1];
      dst[i * 4 + 2] = src[i * 3 + 2];
      dst[i * 4 + 3] = 255;
   }
}


static void
get_values_ubyte3(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], void *values)
{
   GLubyte *dst = (GLubyte *) values;
   GLuint i;
   ASSERT(rb->_ActualFormat == GL_RGB8);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      const GLubyte *src
         = (GLubyte *) rb->Data + 3 * (y[i] * rb->Width + x[i]);
      dst[i * 4 + 0] = src[0];
      dst[i * 4 + 1] = src[1];
      dst[i * 4 + 2] = src[2];
      dst[i * 4 + 3] = 255;
   }
}


static void
put_row_ubyte3(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, const void *values, const GLubyte *mask)
{
   /* note: incoming values are RGB+A! */
   const GLubyte *src = (const GLubyte *) values;
   GLubyte *dst = (GLubyte *) rb->Data + 3 * (y * rb->Width + x);
   GLuint i;
   ASSERT(rb->_ActualFormat == GL_RGB8);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         dst[i * 3 + 0] = src[i * 4 + 0];
         dst[i * 3 + 1] = src[i * 4 + 1];
         dst[i * 3 + 2] = src[i * 4 + 2];
      }
   }
}


static void
put_row_rgb_ubyte3(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   GLint x, GLint y, const void *values, const GLubyte *mask)
{
   /* note: incoming values are RGB+A! */
   const GLubyte *src = (const GLubyte *) values;
   GLubyte *dst = (GLubyte *) rb->Data + 3 * (y * rb->Width + x);
   GLuint i;
   ASSERT(rb->_ActualFormat == GL_RGB8);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         dst[i * 3 + 0] = src[i * 3 + 0];
         dst[i * 3 + 1] = src[i * 3 + 1];
         dst[i * 3 + 2] = src[i * 3 + 2];
      }
   }
}


static void
put_mono_row_ubyte3(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                    GLint x, GLint y, const void *value, const GLubyte *mask)
{
   /* note: incoming value is RGB+A! */
   const GLubyte val0 = ((const GLubyte *) value)[0];
   const GLubyte val1 = ((const GLubyte *) value)[1];
   const GLubyte val2 = ((const GLubyte *) value)[2];
   GLubyte *dst = (GLubyte *) rb->Data + 3 * (y * rb->Width + x);
   ASSERT(rb->_ActualFormat == GL_RGB8);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   if (!mask && val0 == val1 && val1 == val2) {
      /* optimized case */
      _mesa_memset(dst, val0, 3 * count);
   }
   else {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            dst[i * 3 + 0] = val0;
            dst[i * 3 + 1] = val1;
            dst[i * 3 + 2] = val2;
         }
      }
   }
}


static void
put_values_ubyte3(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], const void *values,
                  const GLubyte *mask)
{
   /* note: incoming values are RGB+A! */
   const GLubyte *src = (const GLubyte *) values;
   GLuint i;
   ASSERT(rb->_ActualFormat == GL_RGB8);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLubyte *dst = (GLubyte *) rb->Data + 3 * (y[i] * rb->Width + x[i]);
         dst[0] = src[i * 4 + 0];
         dst[1] = src[i * 4 + 1];
         dst[2] = src[i * 4 + 2];
      }
   }
}


static void
put_mono_values_ubyte3(GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLuint count, const GLint x[], const GLint y[],
                       const void *value, const GLubyte *mask)
{
   /* note: incoming value is RGB+A! */
   const GLubyte val0 = ((const GLubyte *) value)[0];
   const GLubyte val1 = ((const GLubyte *) value)[1];
   const GLubyte val2 = ((const GLubyte *) value)[2];
   GLuint i;
   ASSERT(rb->_ActualFormat == GL_RGB8);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLubyte *dst = (GLubyte *) rb->Data + 3 * (y[i] * rb->Width + x[i]);
         dst[0] = val0;
         dst[1] = val1;
         dst[2] = val2;
      }
   }
}


/**********************************************************************
 * Functions for buffers of 4 X GLubyte (or GLbyte) values.
 * Typically color buffers.
 */

static void *
get_pointer_ubyte4(GLcontext *ctx, struct gl_renderbuffer *rb,
                   GLint x, GLint y)
{
   if (!rb->Data)
      return NULL;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(rb->_ActualFormat == GL_RGBA8);
   return (GLubyte *) rb->Data + 4 * (y * rb->Width + x);
}


static void
get_row_ubyte4(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, void *values)
{
   const GLubyte *src = (const GLubyte *) rb->Data + 4 * (y * rb->Width + x);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(rb->_ActualFormat == GL_RGBA8);
   _mesa_memcpy(values, src, 4 * count * sizeof(GLubyte));
}


static void
get_values_ubyte4(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], void *values)
{
   /* treat 4*GLubyte as 1*GLuint */
   GLuint *dst = (GLuint *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(rb->_ActualFormat == GL_RGBA8);
   for (i = 0; i < count; i++) {
      const GLuint *src = (GLuint *) rb->Data + (y[i] * rb->Width + x[i]);
      dst[i] = *src;
   }
}


static void
put_row_ubyte4(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, const void *values, const GLubyte *mask)
{
   /* treat 4*GLubyte as 1*GLuint */
   const GLuint *src = (const GLuint *) values;
   GLuint *dst = (GLuint *) rb->Data + (y * rb->Width + x);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(rb->_ActualFormat == GL_RGBA8);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i] = src[i];
         }
      }
   }
   else {
      _mesa_memcpy(dst, src, 4 * count * sizeof(GLubyte));
   }
}


static void
put_row_rgb_ubyte4(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   GLint x, GLint y, const void *values, const GLubyte *mask)
{
   /* Store RGB values in RGBA buffer */
   const GLubyte *src = (const GLubyte *) values;
   GLubyte *dst = (GLubyte *) rb->Data + 4 * (y * rb->Width + x);
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(rb->_ActualFormat == GL_RGBA8);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         dst[i * 4 + 0] = src[i * 3 + 0];
         dst[i * 4 + 1] = src[i * 3 + 1];
         dst[i * 4 + 2] = src[i * 3 + 2];
         dst[i * 4 + 3] = 0xff;
      }
   }
}


static void
put_mono_row_ubyte4(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                    GLint x, GLint y, const void *value, const GLubyte *mask)
{
   /* treat 4*GLubyte as 1*GLuint */
   const GLuint val = *((const GLuint *) value);
   GLuint *dst = (GLuint *) rb->Data + (y * rb->Width + x);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(rb->_ActualFormat == GL_RGBA8);
   if (!mask && val == 0) {
      /* common case */
      _mesa_bzero(dst, count * 4 * sizeof(GLubyte));
   }
   else {
      /* general case */
      if (mask) {
         GLuint i;
         for (i = 0; i < count; i++) {
            if (mask[i]) {
               dst[i] = val;
            }
         }
      }
      else {
         GLuint i;
         for (i = 0; i < count; i++) {
            dst[i] = val;
         }
      }
   }
}


static void
put_values_ubyte4(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], const void *values,
                  const GLubyte *mask)
{
   /* treat 4*GLubyte as 1*GLuint */
   const GLuint *src = (const GLuint *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(rb->_ActualFormat == GL_RGBA8);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLuint *dst = (GLuint *) rb->Data + (y[i] * rb->Width + x[i]);
         *dst = src[i];
      }
   }
}


static void
put_mono_values_ubyte4(GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLuint count, const GLint x[], const GLint y[],
                       const void *value, const GLubyte *mask)
{
   /* treat 4*GLubyte as 1*GLuint */
   const GLuint val = *((const GLuint *) value);
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(rb->_ActualFormat == GL_RGBA8);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLuint *dst = (GLuint *) rb->Data + (y[i] * rb->Width + x[i]);
         *dst = val;
      }
   }
}


/**********************************************************************
 * Functions for buffers of 4 X GLushort (or GLshort) values.
 * Typically accum buffer.
 */

static void *
get_pointer_ushort4(GLcontext *ctx, struct gl_renderbuffer *rb,
                    GLint x, GLint y)
{
   if (!rb->Data)
      return NULL;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT || rb->DataType == GL_SHORT);
   return (GLushort *) rb->Data + 4 * (y * rb->Width + x);
}


static void
get_row_ushort4(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                GLint x, GLint y, void *values)
{
   const GLshort *src = (const GLshort *) rb->Data + 4 * (y * rb->Width + x);
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT || rb->DataType == GL_SHORT);
   _mesa_memcpy(values, src, 4 * count * sizeof(GLshort));
}


static void
get_values_ushort4(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   const GLint x[], const GLint y[], void *values)
{
   GLushort *dst = (GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT || rb->DataType == GL_SHORT);
   for (i = 0; i < count; i++) {
      const GLushort *src
         = (GLushort *) rb->Data + 4 * (y[i] * rb->Width + x[i]);
      dst[i] = *src;
   }
}


static void
put_row_ushort4(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const GLushort *src = (const GLushort *) values;
   GLushort *dst = (GLushort *) rb->Data + 4 * (y * rb->Width + x);
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT || rb->DataType == GL_SHORT);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i * 4 + 0] = src[i * 4 + 0];
            dst[i * 4 + 1] = src[i * 4 + 1];
            dst[i * 4 + 2] = src[i * 4 + 2];
            dst[i * 4 + 3] = src[i * 4 + 3];
         }
      }
   }
   else {
      _mesa_memcpy(dst, src, 4 * count * sizeof(GLushort));
   }
}


static void
put_row_rgb_ushort4(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                    GLint x, GLint y, const void *values, const GLubyte *mask)
{
   /* Put RGB values in RGBA buffer */
   const GLushort *src = (const GLushort *) values;
   GLushort *dst = (GLushort *) rb->Data + 4 * (y * rb->Width + x);
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT || rb->DataType == GL_SHORT);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i * 4 + 0] = src[i * 3 + 0];
            dst[i * 4 + 1] = src[i * 3 + 1];
            dst[i * 4 + 2] = src[i * 3 + 2];
            dst[i * 4 + 3] = 0xffff;
         }
      }
   }
   else {
      _mesa_memcpy(dst, src, 4 * count * sizeof(GLushort));
   }
}


static void
put_mono_row_ushort4(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                     GLint x, GLint y, const void *value, const GLubyte *mask)
{
   const GLushort val0 = ((const GLushort *) value)[0];
   const GLushort val1 = ((const GLushort *) value)[1];
   const GLushort val2 = ((const GLushort *) value)[2];
   const GLushort val3 = ((const GLushort *) value)[3];
   GLushort *dst = (GLushort *) rb->Data + 4 * (y * rb->Width + x);
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT || rb->DataType == GL_SHORT);
   if (!mask && val0 == 0 && val1 == 0 && val2 == 0 && val3 == 0) {
      /* common case for clearing accum buffer */
      _mesa_bzero(dst, count * 4 * sizeof(GLushort));
   }
   else {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            dst[i * 4 + 0] = val0;
            dst[i * 4 + 1] = val1;
            dst[i * 4 + 2] = val2;
            dst[i * 4 + 3] = val3;
         }
      }
   }
}


static void
put_values_ushort4(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   const GLint x[], const GLint y[], const void *values,
                   const GLubyte *mask)
{
   const GLushort *src = (const GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT || rb->DataType == GL_SHORT);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLushort *dst = (GLushort *) rb->Data + 4 * (y[i] * rb->Width + x[i]);
         dst[0] = src[i * 4 + 0];
         dst[1] = src[i * 4 + 1];
         dst[2] = src[i * 4 + 2];
         dst[3] = src[i * 4 + 3];
      }
   }
}


static void
put_mono_values_ushort4(GLcontext *ctx, struct gl_renderbuffer *rb,
                        GLuint count, const GLint x[], const GLint y[],
                        const void *value, const GLubyte *mask)
{
   const GLushort val0 = ((const GLushort *) value)[0];
   const GLushort val1 = ((const GLushort *) value)[1];
   const GLushort val2 = ((const GLushort *) value)[2];
   const GLushort val3 = ((const GLushort *) value)[3];
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT || rb->DataType == GL_SHORT);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLushort *dst = (GLushort *) rb->Data + 4 * (y[i] * rb->Width + x[i]);
         dst[0] = val0;
         dst[1] = val1;
         dst[2] = val2;
         dst[3] = val3;
      }
   }
}



/**
 * This is a software fallback for the gl_renderbuffer->AllocStorage
 * function.
 * Device drivers will typically override this function for the buffers
 * which it manages (typically color buffers, Z and stencil).
 * Other buffers (like software accumulation and aux buffers) which the driver
 * doesn't manage can be handled with this function.
 *
 * This one multi-purpose function can allocate stencil, depth, accum, color
 * or color-index buffers!
 *
 * This function also plugs in the appropriate GetPointer, Get/PutRow and
 * Get/PutValues functions.
 */
GLboolean
_mesa_soft_renderbuffer_storage(GLcontext *ctx, struct gl_renderbuffer *rb,
                                GLenum internalFormat,
                                GLuint width, GLuint height)
{
   GLuint pixelSize;

   /* first clear these fields */
   rb->RedBits =
   rb->GreenBits =
   rb->BlueBits =
   rb->AlphaBits =
   rb->IndexBits =
   rb->DepthBits =
   rb->StencilBits = 0;

   switch (internalFormat) {
   case GL_RGB:
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      rb->_ActualFormat = GL_RGB8;
      rb->_BaseFormat = GL_RGB;
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->GetPointer = get_pointer_ubyte3;
      rb->GetRow = get_row_ubyte3;
      rb->GetValues = get_values_ubyte3;
      rb->PutRow = put_row_ubyte3;
      rb->PutRowRGB = put_row_rgb_ubyte3;
      rb->PutMonoRow = put_mono_row_ubyte3;
      rb->PutValues = put_values_ubyte3;
      rb->PutMonoValues = put_mono_values_ubyte3;
      rb->RedBits   = 8 * sizeof(GLubyte);
      rb->GreenBits = 8 * sizeof(GLubyte);
      rb->BlueBits  = 8 * sizeof(GLubyte);
      rb->AlphaBits = 0;
      pixelSize = 3 * sizeof(GLubyte);
      break;
   case GL_RGBA:
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
      rb->_ActualFormat = GL_RGBA8;
      rb->_BaseFormat = GL_RGBA;
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->GetPointer = get_pointer_ubyte4;
      rb->GetRow = get_row_ubyte4;
      rb->GetValues = get_values_ubyte4;
      rb->PutRow = put_row_ubyte4;
      rb->PutRowRGB = put_row_rgb_ubyte4;
      rb->PutMonoRow = put_mono_row_ubyte4;
      rb->PutValues = put_values_ubyte4;
      rb->PutMonoValues = put_mono_values_ubyte4;
      rb->RedBits   = 8 * sizeof(GLubyte);
      rb->GreenBits = 8 * sizeof(GLubyte);
      rb->BlueBits  = 8 * sizeof(GLubyte);
      rb->AlphaBits = 8 * sizeof(GLubyte);
      pixelSize = 4 * sizeof(GLubyte);
      break;
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      rb->_ActualFormat = GL_RGBA16;
      rb->_BaseFormat = GL_RGBA;
      rb->DataType = GL_UNSIGNED_SHORT;
      rb->GetPointer = get_pointer_ushort4;
      rb->GetRow = get_row_ushort4;
      rb->GetValues = get_values_ushort4;
      rb->PutRow = put_row_ushort4;
      rb->PutRowRGB = put_row_rgb_ushort4;
      rb->PutMonoRow = put_mono_row_ushort4;
      rb->PutValues = put_values_ushort4;
      rb->PutMonoValues = put_mono_values_ushort4;
      rb->RedBits   = 8 * sizeof(GLushort);
      rb->GreenBits = 8 * sizeof(GLushort);
      rb->BlueBits  = 8 * sizeof(GLushort);
      rb->AlphaBits = 8 * sizeof(GLushort);
      pixelSize = 4 * sizeof(GLushort);
      break;
#if 00
   case GL_ALPHA8:
      rb->_ActualFormat = GL_ALPHA8;
      rb->_BaseFormat = GL_RGBA; /* Yes, not GL_ALPHA! */
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->GetPointer = get_pointer_alpha8;
      rb->GetRow = get_row_alpha8;
      rb->GetValues = get_values_alpha8;
      rb->PutRow = put_row_alpha8;
      rb->PutRowRGB = NULL;
      rb->PutMonoRow = put_mono_row_alpha8;
      rb->PutValues = put_values_alpha8;
      rb->PutMonoValues = put_mono_values_alpha8;
      rb->RedBits   = 0; /*red*/
      rb->GreenBits = 0; /*green*/
      rb->BlueBits  = 0; /*blue*/
      rb->AlphaBits = 8 * sizeof(GLubyte);
      pixelSize = sizeof(GLubyte);
      break;
#endif
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
      rb->_ActualFormat = GL_STENCIL_INDEX8_EXT;
      rb->_BaseFormat = GL_STENCIL_INDEX;
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->GetPointer = get_pointer_ubyte;
      rb->GetRow = get_row_ubyte;
      rb->GetValues = get_values_ubyte;
      rb->PutRow = put_row_ubyte;
      rb->PutRowRGB = NULL;
      rb->PutMonoRow = put_mono_row_ubyte;
      rb->PutValues = put_values_ubyte;
      rb->PutMonoValues = put_mono_values_ubyte;
      rb->StencilBits = 8 * sizeof(GLubyte);
      pixelSize = sizeof(GLubyte);
      break;
   case GL_STENCIL_INDEX16_EXT:
      rb->_ActualFormat = GL_STENCIL_INDEX16_EXT;
      rb->_BaseFormat = GL_STENCIL_INDEX;
      rb->DataType = GL_UNSIGNED_SHORT;
      rb->GetPointer = get_pointer_ushort;
      rb->GetRow = get_row_ushort;
      rb->GetValues = get_values_ushort;
      rb->PutRow = put_row_ushort;
      rb->PutRowRGB = NULL;
      rb->PutMonoRow = put_mono_row_ushort;
      rb->PutValues = put_values_ushort;
      rb->PutMonoValues = put_mono_values_ushort;
      rb->StencilBits = 8 * sizeof(GLushort);
      pixelSize = sizeof(GLushort);
      break;
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16:
      rb->_ActualFormat = GL_DEPTH_COMPONENT16;
      rb->_BaseFormat = GL_DEPTH_COMPONENT;
      rb->DataType = GL_UNSIGNED_SHORT;
      rb->GetPointer = get_pointer_ushort;
      rb->GetRow = get_row_ushort;
      rb->GetValues = get_values_ushort;
      rb->PutRow = put_row_ushort;
      rb->PutRowRGB = NULL;
      rb->PutMonoRow = put_mono_row_ushort;
      rb->PutValues = put_values_ushort;
      rb->PutMonoValues = put_mono_values_ushort;
      rb->DepthBits = 8 * sizeof(GLushort);
      pixelSize = sizeof(GLushort);
      break;
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
      rb->_BaseFormat = GL_DEPTH_COMPONENT;
      rb->DataType = GL_UNSIGNED_INT;
      rb->GetPointer = get_pointer_uint;
      rb->GetRow = get_row_uint;
      rb->GetValues = get_values_uint;
      rb->PutRow = put_row_uint;
      rb->PutRowRGB = NULL;
      rb->PutMonoRow = put_mono_row_uint;
      rb->PutValues = put_values_uint;
      rb->PutMonoValues = put_mono_values_uint;
      if (internalFormat == GL_DEPTH_COMPONENT24) {
         rb->_ActualFormat = GL_DEPTH_COMPONENT24;
         rb->DepthBits = 24;
      }
      else {
         rb->_ActualFormat = GL_DEPTH_COMPONENT32;
         rb->DepthBits = 32;
      }
      pixelSize = sizeof(GLuint);
      break;
   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      rb->_ActualFormat = GL_DEPTH24_STENCIL8_EXT;
      rb->_BaseFormat = GL_DEPTH_STENCIL_EXT;
      rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
      rb->GetPointer = get_pointer_uint;
      rb->GetRow = get_row_uint;
      rb->GetValues = get_values_uint;
      rb->PutRow = put_row_uint;
      rb->PutRowRGB = NULL;
      rb->PutMonoRow = put_mono_row_uint;
      rb->PutValues = put_values_uint;
      rb->PutMonoValues = put_mono_values_uint;
      rb->DepthBits = 24;
      rb->StencilBits = 8;
      pixelSize = sizeof(GLuint);
      break;
   case GL_COLOR_INDEX8_EXT:
      rb->_ActualFormat = GL_COLOR_INDEX8_EXT;
      rb->_BaseFormat = GL_COLOR_INDEX;
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->GetPointer = get_pointer_ubyte;
      rb->GetRow = get_row_ubyte;
      rb->GetValues = get_values_ubyte;
      rb->PutRow = put_row_ubyte;
      rb->PutRowRGB = NULL;
      rb->PutMonoRow = put_mono_row_ubyte;
      rb->PutValues = put_values_ubyte;
      rb->PutMonoValues = put_mono_values_ubyte;
      rb->IndexBits = 8 * sizeof(GLubyte);
      pixelSize = sizeof(GLubyte);
      break;
   case GL_COLOR_INDEX16_EXT:
      rb->_ActualFormat = GL_COLOR_INDEX16_EXT;
      rb->_BaseFormat = GL_COLOR_INDEX;
      rb->DataType = GL_UNSIGNED_SHORT;
      rb->GetPointer = get_pointer_ushort;
      rb->GetRow = get_row_ushort;
      rb->GetValues = get_values_ushort;
      rb->PutRow = put_row_ushort;
      rb->PutRowRGB = NULL;
      rb->PutMonoRow = put_mono_row_ushort;
      rb->PutValues = put_values_ushort;
      rb->PutMonoValues = put_mono_values_ushort;
      rb->IndexBits = 8 * sizeof(GLushort);
      pixelSize = sizeof(GLushort);
      break;
   case COLOR_INDEX32:
      rb->_ActualFormat = COLOR_INDEX32;
      rb->_BaseFormat = GL_COLOR_INDEX;
      rb->DataType = GL_UNSIGNED_INT;
      rb->GetPointer = get_pointer_uint;
      rb->GetRow = get_row_uint;
      rb->GetValues = get_values_uint;
      rb->PutRow = put_row_uint;
      rb->PutRowRGB = NULL;
      rb->PutMonoRow = put_mono_row_uint;
      rb->PutValues = put_values_uint;
      rb->PutMonoValues = put_mono_values_uint;
      rb->IndexBits = 8 * sizeof(GLuint);
      pixelSize = sizeof(GLuint);
      break;
   default:
      _mesa_problem(ctx, "Bad internalFormat in _mesa_soft_renderbuffer_storage");
      return GL_FALSE;
   }

   ASSERT(rb->DataType);
   ASSERT(rb->GetPointer);
   ASSERT(rb->GetRow);
   ASSERT(rb->GetValues);
   ASSERT(rb->PutRow);
   ASSERT(rb->PutMonoRow);
   ASSERT(rb->PutValues);
   ASSERT(rb->PutMonoValues);

   /* free old buffer storage */
   if (rb->Data) {
      _mesa_free(rb->Data);
      rb->Data = NULL;
   }

   if (width > 0 && height > 0) {
      /* allocate new buffer storage */
      rb->Data = _mesa_malloc(width * height * pixelSize);
      if (rb->Data == NULL) {
         rb->Width = 0;
         rb->Height = 0;
         _mesa_error(ctx, GL_OUT_OF_MEMORY,
                     "software renderbuffer allocation (%d x %d x %d)",
                     width, height, pixelSize);
         return GL_FALSE;
      }
   }

   rb->Width = width;
   rb->Height = height;

   return GL_TRUE;
}



/**********************************************************************/
/**********************************************************************/
/**********************************************************************/


/**
 * Here we utilize the gl_renderbuffer->Wrapper field to put an alpha
 * buffer wrapper around an existing RGB renderbuffer (hw or sw).
 *
 * When PutRow is called (for example), we store the alpha values in
 * this buffer, then pass on the PutRow call to the wrapped RGB
 * buffer.
 */


static GLboolean
alloc_storage_alpha8(GLcontext *ctx, struct gl_renderbuffer *arb,
                     GLenum internalFormat, GLuint width, GLuint height)
{
   ASSERT(arb != arb->Wrapped);
   ASSERT(arb->_ActualFormat == GL_ALPHA8);

   /* first, pass the call to the wrapped RGB buffer */
   if (!arb->Wrapped->AllocStorage(ctx, arb->Wrapped, internalFormat,
                                  width, height)) {
      return GL_FALSE;
   }

   /* next, resize my alpha buffer */
   if (arb->Data) {
      _mesa_free(arb->Data);
   }

   arb->Data = _mesa_malloc(width * height * sizeof(GLubyte));
   if (arb->Data == NULL) {
      arb->Width = 0;
      arb->Height = 0;
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "software alpha buffer allocation");
      return GL_FALSE;
   }

   arb->Width = width;
   arb->Height = height;

   return GL_TRUE;
}


/**
 * Delete an alpha_renderbuffer object, as well as the wrapped RGB buffer.
 */
static void
delete_renderbuffer_alpha8(struct gl_renderbuffer *arb)
{
   if (arb->Data) {
      _mesa_free(arb->Data);
   }
   ASSERT(arb->Wrapped);
   ASSERT(arb != arb->Wrapped);
   arb->Wrapped->Delete(arb->Wrapped);
   arb->Wrapped = NULL;
   _mesa_free(arb);
}


static void *
get_pointer_alpha8(GLcontext *ctx, struct gl_renderbuffer *arb,
                   GLint x, GLint y)
{
   return NULL;   /* don't allow direct access! */
}


static void
get_row_alpha8(GLcontext *ctx, struct gl_renderbuffer *arb, GLuint count,
               GLint x, GLint y, void *values)
{
   /* NOTE: 'values' is RGBA format! */
   const GLubyte *src = (const GLubyte *) arb->Data + y * arb->Width + x;
   GLubyte *dst = (GLubyte *) values;
   GLuint i;
   ASSERT(arb != arb->Wrapped);
   ASSERT(arb->DataType == GL_UNSIGNED_BYTE);
   /* first, pass the call to the wrapped RGB buffer */
   arb->Wrapped->GetRow(ctx, arb->Wrapped, count, x, y, values);
   /* second, fill in alpha values from this buffer! */
   for (i = 0; i < count; i++) {
      dst[i * 4 + 3] = src[i];
   }
}


static void
get_values_alpha8(GLcontext *ctx, struct gl_renderbuffer *arb, GLuint count,
                  const GLint x[], const GLint y[], void *values)
{
   GLubyte *dst = (GLubyte *) values;
   GLuint i;
   ASSERT(arb != arb->Wrapped);
   ASSERT(arb->DataType == GL_UNSIGNED_BYTE);
   /* first, pass the call to the wrapped RGB buffer */
   arb->Wrapped->GetValues(ctx, arb->Wrapped, count, x, y, values);
   /* second, fill in alpha values from this buffer! */
   for (i = 0; i < count; i++) {
      const GLubyte *src = (GLubyte *) arb->Data + y[i] * arb->Width + x[i];
      dst[i * 4 + 3] = *src;
   }
}


static void
put_row_alpha8(GLcontext *ctx, struct gl_renderbuffer *arb, GLuint count,
               GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const GLubyte *src = (const GLubyte *) values;
   GLubyte *dst = (GLubyte *) arb->Data + y * arb->Width + x;
   GLuint i;
   ASSERT(arb != arb->Wrapped);
   ASSERT(arb->DataType == GL_UNSIGNED_BYTE);
   /* first, pass the call to the wrapped RGB buffer */
   arb->Wrapped->PutRow(ctx, arb->Wrapped, count, x, y, values, mask);
   /* second, store alpha in our buffer */
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         dst[i] = src[i * 4 + 3];
      }
   }
}


static void
put_row_rgb_alpha8(GLcontext *ctx, struct gl_renderbuffer *arb, GLuint count,
                   GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const GLubyte *src = (const GLubyte *) values;
   GLubyte *dst = (GLubyte *) arb->Data + y * arb->Width + x;
   GLuint i;
   ASSERT(arb != arb->Wrapped);
   ASSERT(arb->DataType == GL_UNSIGNED_BYTE);
   /* first, pass the call to the wrapped RGB buffer */
   arb->Wrapped->PutRowRGB(ctx, arb->Wrapped, count, x, y, values, mask);
   /* second, store alpha in our buffer */
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         dst[i] = src[i * 4 + 3];
      }
   }
}


static void
put_mono_row_alpha8(GLcontext *ctx, struct gl_renderbuffer *arb, GLuint count,
                    GLint x, GLint y, const void *value, const GLubyte *mask)
{
   const GLubyte val = ((const GLubyte *) value)[3];
   GLubyte *dst = (GLubyte *) arb->Data + y * arb->Width + x;
   ASSERT(arb != arb->Wrapped);
   ASSERT(arb->DataType == GL_UNSIGNED_BYTE);
   /* first, pass the call to the wrapped RGB buffer */
   arb->Wrapped->PutMonoRow(ctx, arb->Wrapped, count, x, y, value, mask);
   /* second, store alpha in our buffer */
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i] = val;
         }
      }
   }
   else {
      _mesa_memset(dst, val, count);
   }
}


static void
put_values_alpha8(GLcontext *ctx, struct gl_renderbuffer *arb, GLuint count,
                  const GLint x[], const GLint y[],
                  const void *values, const GLubyte *mask)
{
   const GLubyte *src = (const GLubyte *) values;
   GLuint i;
   ASSERT(arb != arb->Wrapped);
   ASSERT(arb->DataType == GL_UNSIGNED_BYTE);
   /* first, pass the call to the wrapped RGB buffer */
   arb->Wrapped->PutValues(ctx, arb->Wrapped, count, x, y, values, mask);
   /* second, store alpha in our buffer */
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLubyte *dst = (GLubyte *) arb->Data + y[i] * arb->Width + x[i];
         *dst = src[i * 4 + 3];
      }
   }
}


static void
put_mono_values_alpha8(GLcontext *ctx, struct gl_renderbuffer *arb,
                       GLuint count, const GLint x[], const GLint y[],
                       const void *value, const GLubyte *mask)
{
   const GLubyte val = ((const GLubyte *) value)[3];
   GLuint i;
   ASSERT(arb != arb->Wrapped);
   ASSERT(arb->DataType == GL_UNSIGNED_BYTE);
   /* first, pass the call to the wrapped RGB buffer */
   arb->Wrapped->PutValues(ctx, arb->Wrapped, count, x, y, value, mask);
   /* second, store alpha in our buffer */
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLubyte *dst = (GLubyte *) arb->Data + y[i] * arb->Width + x[i];
         *dst = val;
      }
   }
}


static void
copy_buffer_alpha8(struct gl_renderbuffer* dst, struct gl_renderbuffer* src)
{
   ASSERT(dst->_ActualFormat == GL_ALPHA8);
   ASSERT(src->_ActualFormat == GL_ALPHA8);
   ASSERT(dst->Width == src->Width);
   ASSERT(dst->Height == src->Height);

   _mesa_memcpy(dst->Data, src->Data, dst->Width * dst->Height * sizeof(GLubyte));
}


/**********************************************************************/
/**********************************************************************/
/**********************************************************************/


/**
 * Default GetPointer routine.  Always return NULL to indicate that
 * direct buffer access is not supported.
 */
static void *
nop_get_pointer(GLcontext *ctx, struct gl_renderbuffer *rb, GLint x, GLint y)
{
   return NULL;
}


/**
 * Initialize the fields of a gl_renderbuffer to default values.
 */
void
_mesa_init_renderbuffer(struct gl_renderbuffer *rb, GLuint name)
{
   _glthread_INIT_MUTEX(rb->Mutex);

   rb->Magic = RB_MAGIC;
   rb->ClassID = 0;
   rb->Name = name;
   rb->RefCount = 0;
   rb->Delete = _mesa_delete_renderbuffer;

   /* The rest of these should be set later by the caller of this function or
    * the AllocStorage method:
    */
   rb->AllocStorage = NULL;

   rb->Width = 0;
   rb->Height = 0;
   rb->InternalFormat = GL_NONE;
   rb->_ActualFormat = GL_NONE;
   rb->_BaseFormat = GL_NONE;
   rb->DataType = GL_NONE;
   rb->RedBits = rb->GreenBits = rb->BlueBits = rb->AlphaBits = 0;
   rb->IndexBits = 0;
   rb->DepthBits = 0;
   rb->StencilBits = 0;
   rb->Data = NULL;

   /* Point back to ourself so that we don't have to check for Wrapped==NULL
    * all over the drivers.
    */
   rb->Wrapped = rb;

   rb->GetPointer = nop_get_pointer;
   rb->GetRow = NULL;
   rb->GetValues = NULL;
   rb->PutRow = NULL;
   rb->PutRowRGB = NULL;
   rb->PutMonoRow = NULL;
   rb->PutValues = NULL;
   rb->PutMonoValues = NULL;
}


/**
 * Allocate a new gl_renderbuffer object.  This can be used for user-created
 * renderbuffers or window-system renderbuffers.
 */
struct gl_renderbuffer *
_mesa_new_renderbuffer(GLcontext *ctx, GLuint name)
{
   struct gl_renderbuffer *rb = CALLOC_STRUCT(gl_renderbuffer);
   if (rb) {
      _mesa_init_renderbuffer(rb, name);
   }
   return rb;
}


/**
 * Delete a gl_framebuffer.
 * This is the default function for renderbuffer->Delete().
 */
void
_mesa_delete_renderbuffer(struct gl_renderbuffer *rb)
{
   if (rb->Data) {
      _mesa_free(rb->Data);
   }
   _mesa_free(rb);
}


/**
 * Allocate a software-based renderbuffer.  This is called via the
 * ctx->Driver.NewRenderbuffer() function when the user creates a new
 * renderbuffer.
 * This would not be used for hardware-based renderbuffers.
 */
struct gl_renderbuffer *
_mesa_new_soft_renderbuffer(GLcontext *ctx, GLuint name)
{
   struct gl_renderbuffer *rb = _mesa_new_renderbuffer(ctx, name);
   if (rb) {
      rb->AllocStorage = _mesa_soft_renderbuffer_storage;
      /* Normally, one would setup the PutRow, GetRow, etc functions here.
       * But we're doing that in the _mesa_soft_renderbuffer_storage() function
       * instead.
       */
   }
   return rb;
}


/**
 * Add software-based color renderbuffers to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
GLboolean
_mesa_add_color_renderbuffers(GLcontext *ctx, struct gl_framebuffer *fb,
                              GLuint rgbBits, GLuint alphaBits,
                              GLboolean frontLeft, GLboolean backLeft,
                              GLboolean frontRight, GLboolean backRight)
{
   GLuint b;

   if (rgbBits > 16 || alphaBits > 16) {
      _mesa_problem(ctx,
                    "Unsupported bit depth in _mesa_add_color_renderbuffers");
      return GL_FALSE;
   }

   assert(MAX_COLOR_ATTACHMENTS >= 4);

   for (b = BUFFER_FRONT_LEFT; b <= BUFFER_BACK_RIGHT; b++) {
      struct gl_renderbuffer *rb;

      if (b == BUFFER_FRONT_LEFT && !frontLeft)
         continue;
      else if (b == BUFFER_BACK_LEFT && !backLeft)
         continue;
      else if (b == BUFFER_FRONT_RIGHT && !frontRight)
         continue;
      else if (b == BUFFER_BACK_RIGHT && !backRight)
         continue;

      assert(fb->Attachment[b].Renderbuffer == NULL);

      rb = _mesa_new_renderbuffer(ctx, 0);
      if (!rb) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating color buffer");
         return GL_FALSE;
      }

      if (rgbBits <= 8) {
         if (alphaBits)
            rb->_ActualFormat = GL_RGBA8;
         else
            rb->_ActualFormat = GL_RGB8;
      }
      else {
         assert(rgbBits <= 16);
         if (alphaBits)
            rb->_ActualFormat = GL_RGBA16;
         else
            rb->_ActualFormat = GL_RGBA16; /* don't really have RGB16 yet */
      }
      rb->InternalFormat = rb->_ActualFormat;

      rb->AllocStorage = _mesa_soft_renderbuffer_storage;
      _mesa_add_renderbuffer(fb, b, rb);
   }

   return GL_TRUE;
}


/**
 * Add software-based color index renderbuffers to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
GLboolean
_mesa_add_color_index_renderbuffers(GLcontext *ctx, struct gl_framebuffer *fb,
                                    GLuint indexBits,
                                    GLboolean frontLeft, GLboolean backLeft,
                                    GLboolean frontRight, GLboolean backRight)
{
   GLuint b;

   if (indexBits > 8) {
      _mesa_problem(ctx,
                "Unsupported bit depth in _mesa_add_color_index_renderbuffers");
      return GL_FALSE;
   }

   assert(MAX_COLOR_ATTACHMENTS >= 4);

   for (b = BUFFER_FRONT_LEFT; b <= BUFFER_BACK_RIGHT; b++) {
      struct gl_renderbuffer *rb;

      if (b == BUFFER_FRONT_LEFT && !frontLeft)
         continue;
      else if (b == BUFFER_BACK_LEFT && !backLeft)
         continue;
      else if (b == BUFFER_FRONT_RIGHT && !frontRight)
         continue;
      else if (b == BUFFER_BACK_RIGHT && !backRight)
         continue;

      assert(fb->Attachment[b].Renderbuffer == NULL);

      rb = _mesa_new_renderbuffer(ctx, 0);
      if (!rb) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating color buffer");
         return GL_FALSE;
      }

      if (indexBits <= 8) {
         /* only support GLuint for now */
         /*rb->InternalFormat = GL_COLOR_INDEX8_EXT;*/
         rb->_ActualFormat = COLOR_INDEX32;
      }
      else {
         rb->_ActualFormat = COLOR_INDEX32;
      }
      rb->InternalFormat = rb->_ActualFormat;

      rb->AllocStorage = _mesa_soft_renderbuffer_storage;
      _mesa_add_renderbuffer(fb, b, rb);
   }

   return GL_TRUE;
}


/**
 * Add software-based alpha renderbuffers to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
GLboolean
_mesa_add_alpha_renderbuffers(GLcontext *ctx, struct gl_framebuffer *fb,
                              GLuint alphaBits,
                              GLboolean frontLeft, GLboolean backLeft,
                              GLboolean frontRight, GLboolean backRight)
{
   GLuint b;

   /* for window system framebuffers only! */
   assert(fb->Name == 0);

   if (alphaBits > 8) {
      _mesa_problem(ctx,
                    "Unsupported bit depth in _mesa_add_alpha_renderbuffers");
      return GL_FALSE;
   }

   assert(MAX_COLOR_ATTACHMENTS >= 4);

   /* Wrap each of the RGB color buffers with an alpha renderbuffer.
    */
   for (b = BUFFER_FRONT_LEFT; b <= BUFFER_BACK_RIGHT; b++) {
      struct gl_renderbuffer *arb;

      if (b == BUFFER_FRONT_LEFT && !frontLeft)
         continue;
      else if (b == BUFFER_BACK_LEFT && !backLeft)
         continue;
      else if (b == BUFFER_FRONT_RIGHT && !frontRight)
         continue;
      else if (b == BUFFER_BACK_RIGHT && !backRight)
         continue;

      /* the RGB buffer to wrap must already exist!! */
      assert(fb->Attachment[b].Renderbuffer);

      /* only GLubyte supported for now */
      assert(fb->Attachment[b].Renderbuffer->DataType == GL_UNSIGNED_BYTE);

      /* allocate alpha renderbuffer */
      arb = _mesa_new_renderbuffer(ctx, 0);
      if (!arb) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating alpha buffer");
         return GL_FALSE;
      }

      /* wrap the alpha renderbuffer around the RGB renderbuffer */
      arb->Wrapped = fb->Attachment[b].Renderbuffer;

      /* Set up my alphabuffer fields and plug in my functions.
       * The functions will put/get the alpha values from/to RGBA arrays
       * and then call the wrapped buffer's functions to handle the RGB
       * values.
       */
      arb->InternalFormat = arb->Wrapped->InternalFormat;
      arb->_ActualFormat  = GL_ALPHA8;
      arb->_BaseFormat    = arb->Wrapped->_BaseFormat;
      arb->DataType       = arb->Wrapped->DataType;
      arb->AllocStorage   = alloc_storage_alpha8;
      arb->Delete         = delete_renderbuffer_alpha8;
      arb->GetPointer     = get_pointer_alpha8;
      arb->GetRow         = get_row_alpha8;
      arb->GetValues      = get_values_alpha8;
      arb->PutRow         = put_row_alpha8;
      arb->PutRowRGB      = put_row_rgb_alpha8;
      arb->PutMonoRow     = put_mono_row_alpha8;
      arb->PutValues      = put_values_alpha8;
      arb->PutMonoValues  = put_mono_values_alpha8;

      /* clear the pointer to avoid assertion/sanity check failure later */
      fb->Attachment[b].Renderbuffer = NULL;

      /* plug the alpha renderbuffer into the colorbuffer attachment */
      _mesa_add_renderbuffer(fb, b, arb);
   }

   return GL_TRUE;
}


/**
 * For framebuffers that use a software alpha channel wrapper
 * created by _mesa_add_alpha_renderbuffer or _mesa_add_soft_renderbuffers,
 * copy the back buffer alpha channel into the front buffer alpha channel.
 */
void
_mesa_copy_soft_alpha_renderbuffers(GLcontext *ctx, struct gl_framebuffer *fb)
{
   if (fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer &&
       fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer)
      copy_buffer_alpha8(fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer,
                         fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer);


   if (fb->Attachment[BUFFER_FRONT_RIGHT].Renderbuffer &&
       fb->Attachment[BUFFER_BACK_RIGHT].Renderbuffer)
      copy_buffer_alpha8(fb->Attachment[BUFFER_FRONT_RIGHT].Renderbuffer,
                         fb->Attachment[BUFFER_BACK_RIGHT].Renderbuffer);
}


/**
 * Add a software-based depth renderbuffer to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
GLboolean
_mesa_add_depth_renderbuffer(GLcontext *ctx, struct gl_framebuffer *fb,
                             GLuint depthBits)
{
   struct gl_renderbuffer *rb;

   if (depthBits > 32) {
      _mesa_problem(ctx,
                    "Unsupported depthBits in _mesa_add_depth_renderbuffer");
      return GL_FALSE;
   }

   assert(fb->Attachment[BUFFER_DEPTH].Renderbuffer == NULL);

   rb = _mesa_new_renderbuffer(ctx, 0);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating depth buffer");
      return GL_FALSE;
   }

   if (depthBits <= 16) {
      rb->_ActualFormat = GL_DEPTH_COMPONENT16;
   }
   else if (depthBits <= 24) {
      rb->_ActualFormat = GL_DEPTH_COMPONENT24;
   }
   else {
      rb->_ActualFormat = GL_DEPTH_COMPONENT32;
   }
   rb->InternalFormat = rb->_ActualFormat;

   rb->AllocStorage = _mesa_soft_renderbuffer_storage;
   _mesa_add_renderbuffer(fb, BUFFER_DEPTH, rb);

   return GL_TRUE;
}


/**
 * Add a software-based stencil renderbuffer to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
GLboolean
_mesa_add_stencil_renderbuffer(GLcontext *ctx, struct gl_framebuffer *fb,
                               GLuint stencilBits)
{
   struct gl_renderbuffer *rb;

   if (stencilBits > 16) {
      _mesa_problem(ctx,
                  "Unsupported stencilBits in _mesa_add_stencil_renderbuffer");
      return GL_FALSE;
   }

   assert(fb->Attachment[BUFFER_STENCIL].Renderbuffer == NULL);

   rb = _mesa_new_renderbuffer(ctx, 0);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating stencil buffer");
      return GL_FALSE;
   }

   if (stencilBits <= 8) {
      rb->_ActualFormat = GL_STENCIL_INDEX8_EXT;
   }
   else {
      /* not really supported (see s_stencil.c code) */
      rb->_ActualFormat = GL_STENCIL_INDEX16_EXT;
   }
   rb->InternalFormat = rb->_ActualFormat;

   rb->AllocStorage = _mesa_soft_renderbuffer_storage;
   _mesa_add_renderbuffer(fb, BUFFER_STENCIL, rb);

   return GL_TRUE;
}


/**
 * Add a software-based accumulation renderbuffer to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
GLboolean
_mesa_add_accum_renderbuffer(GLcontext *ctx, struct gl_framebuffer *fb,
                             GLuint redBits, GLuint greenBits,
                             GLuint blueBits, GLuint alphaBits)
{
   struct gl_renderbuffer *rb;

   if (redBits > 16 || greenBits > 16 || blueBits > 16 || alphaBits > 16) {
      _mesa_problem(ctx,
                    "Unsupported accumBits in _mesa_add_accum_renderbuffer");
      return GL_FALSE;
   }

   assert(fb->Attachment[BUFFER_ACCUM].Renderbuffer == NULL);

   rb = _mesa_new_renderbuffer(ctx, 0);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating accum buffer");
      return GL_FALSE;
   }

   rb->_ActualFormat = GL_RGBA16;
   rb->InternalFormat = GL_RGBA16;
   rb->AllocStorage = _mesa_soft_renderbuffer_storage;
   _mesa_add_renderbuffer(fb, BUFFER_ACCUM, rb);

   return GL_TRUE;
}



/**
 * Add a software-based accumulation renderbuffer to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 *
 * NOTE: color-index aux buffers not supported.
 */
GLboolean
_mesa_add_aux_renderbuffers(GLcontext *ctx, struct gl_framebuffer *fb,
                            GLuint colorBits, GLuint numBuffers)
{
   GLuint i;

   if (colorBits > 16) {
      _mesa_problem(ctx,
                    "Unsupported accumBits in _mesa_add_aux_renderbuffers");
      return GL_FALSE;
   }

   assert(numBuffers < MAX_AUX_BUFFERS);

   for (i = 0; i < numBuffers; i++) {
      struct gl_renderbuffer *rb = _mesa_new_renderbuffer(ctx, 0);

      assert(fb->Attachment[BUFFER_AUX0 + i].Renderbuffer == NULL);

      if (!rb) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating accum buffer");
         return GL_FALSE;
      }

      if (colorBits <= 8) {
         rb->_ActualFormat = GL_RGBA8;
      }
      else {
         rb->_ActualFormat = GL_RGBA16;
      }
      rb->InternalFormat = rb->_ActualFormat;

      rb->AllocStorage = _mesa_soft_renderbuffer_storage;
      _mesa_add_renderbuffer(fb, BUFFER_AUX0 + i, rb);
   }
   return GL_TRUE;
}


/**
 * Create/attach software-based renderbuffers to the given framebuffer.
 * This is a helper routine for device drivers.  Drivers can just as well
 * call the individual _mesa_add_*_renderbuffer() routines directly.
 */
void
_mesa_add_soft_renderbuffers(struct gl_framebuffer *fb,
                             GLboolean color,
                             GLboolean depth,
                             GLboolean stencil,
                             GLboolean accum,
                             GLboolean alpha,
                             GLboolean aux)
{
   GLboolean frontLeft = GL_TRUE;
   GLboolean backLeft = fb->Visual.doubleBufferMode;
   GLboolean frontRight = fb->Visual.stereoMode;
   GLboolean backRight = fb->Visual.stereoMode && fb->Visual.doubleBufferMode;

   if (color) {
      if (fb->Visual.rgbMode) {
         assert(fb->Visual.redBits == fb->Visual.greenBits);
         assert(fb->Visual.redBits == fb->Visual.blueBits);
         _mesa_add_color_renderbuffers(NULL, fb,
                                       fb->Visual.redBits,
                                       fb->Visual.alphaBits,
                                       frontLeft, backLeft,
                                       frontRight, backRight);
      }
      else {
         _mesa_add_color_index_renderbuffers(NULL, fb,
                                             fb->Visual.indexBits,
                                             frontLeft, backLeft,
                                             frontRight, backRight);
      }
   }

   if (depth) {
      assert(fb->Visual.depthBits > 0);
      _mesa_add_depth_renderbuffer(NULL, fb, fb->Visual.depthBits);
   }

   if (stencil) {
      assert(fb->Visual.stencilBits > 0);
      _mesa_add_stencil_renderbuffer(NULL, fb, fb->Visual.stencilBits);
   }

   if (accum) {
      assert(fb->Visual.rgbMode);
      assert(fb->Visual.accumRedBits > 0);
      assert(fb->Visual.accumGreenBits > 0);
      assert(fb->Visual.accumBlueBits > 0);
      _mesa_add_accum_renderbuffer(NULL, fb,
                                   fb->Visual.accumRedBits,
                                   fb->Visual.accumGreenBits,
                                   fb->Visual.accumBlueBits,
                                   fb->Visual.accumAlphaBits);
   }

   if (aux) {
      assert(fb->Visual.rgbMode);
      assert(fb->Visual.numAuxBuffers > 0);
      _mesa_add_aux_renderbuffers(NULL, fb, fb->Visual.redBits,
                                  fb->Visual.numAuxBuffers);
   }

   if (alpha) {
      assert(fb->Visual.rgbMode);
      assert(fb->Visual.alphaBits > 0);
      _mesa_add_alpha_renderbuffers(NULL, fb, fb->Visual.alphaBits,
                                    frontLeft, backLeft,
                                    frontRight, backRight);
   }

#if 0
   if (multisample) {
      /* maybe someday */
   }
#endif
}


/**
 * Attach a renderbuffer to a framebuffer.
 */
void
_mesa_add_renderbuffer(struct gl_framebuffer *fb,
                       GLuint bufferName, struct gl_renderbuffer *rb)
{
   assert(fb);
   assert(rb);
   assert(bufferName < BUFFER_COUNT);

   /* There should be no previous renderbuffer on this attachment point,
    * with the exception of depth/stencil since the same renderbuffer may
    * be used for both.
    */
   assert(bufferName == BUFFER_DEPTH ||
          bufferName == BUFFER_STENCIL ||
          fb->Attachment[bufferName].Renderbuffer == NULL);

   /* winsys vs. user-created buffer cross check */
   if (fb->Name) {
      assert(rb->Name);
   }
   else {
      assert(!rb->Name);
   }

   /* If Mesa's compiled with deep color channels (16 or 32 bits / channel)
    * and the device driver is expecting 8-bit values (GLubyte), we can
    * use a "renderbuffer adaptor/wrapper" to do the necessary conversions.
    */
   if (rb->_BaseFormat == GL_RGBA) {
      if (CHAN_BITS == 16 && rb->DataType == GL_UNSIGNED_BYTE) {
         GET_CURRENT_CONTEXT(ctx);
         rb = _mesa_new_renderbuffer_16wrap8(ctx, rb);
      }
      else if (CHAN_BITS == 32 && rb->DataType == GL_UNSIGNED_BYTE) {
         GET_CURRENT_CONTEXT(ctx);
         rb = _mesa_new_renderbuffer_32wrap8(ctx, rb);
      }
      else if (CHAN_BITS == 32 && rb->DataType == GL_UNSIGNED_SHORT) {
         GET_CURRENT_CONTEXT(ctx);
         rb = _mesa_new_renderbuffer_32wrap16(ctx, rb);
      }
   }

   fb->Attachment[bufferName].Type = GL_RENDERBUFFER_EXT;
   fb->Attachment[bufferName].Complete = GL_TRUE;
   _mesa_reference_renderbuffer(&fb->Attachment[bufferName].Renderbuffer, rb);
}


/**
 * Remove the named renderbuffer from the given framebuffer.
 */
void
_mesa_remove_renderbuffer(struct gl_framebuffer *fb, GLuint bufferName)
{
   struct gl_renderbuffer *rb;

   assert(bufferName < BUFFER_COUNT);

   rb = fb->Attachment[bufferName].Renderbuffer;
   if (!rb)
      return;

   _mesa_reference_renderbuffer(&rb, NULL);

   fb->Attachment[bufferName].Renderbuffer = NULL;
}


/**
 * Set *ptr to point to rb.  If *ptr points to another renderbuffer,
 * dereference that buffer first.  The new renderbuffer's refcount will
 * be incremented.  The old renderbuffer's refcount will be decremented.
 */
void
_mesa_reference_renderbuffer(struct gl_renderbuffer **ptr,
                             struct gl_renderbuffer *rb)
{
   assert(ptr);
   if (*ptr == rb) {
      /* no change */
      return;
   }

   if (*ptr) {
      /* Unreference the old renderbuffer */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_renderbuffer *oldRb = *ptr;

      assert(oldRb->Magic == RB_MAGIC);
      _glthread_LOCK_MUTEX(oldRb->Mutex);
      assert(oldRb->Magic == RB_MAGIC);
      ASSERT(oldRb->RefCount > 0);
      oldRb->RefCount--;
      /*printf("RB DECR %p (%d) to %d\n", (void*) oldRb, oldRb->Name, oldRb->RefCount);*/
      deleteFlag = (oldRb->RefCount == 0);
      _glthread_UNLOCK_MUTEX(oldRb->Mutex);

      if (deleteFlag) {
         oldRb->Magic = 0; /* now invalid memory! */
         oldRb->Delete(oldRb);
      }

      *ptr = NULL;
   }
   assert(!*ptr);

   if (rb) {
      assert(rb->Magic == RB_MAGIC);
      /* reference new renderbuffer */
      _glthread_LOCK_MUTEX(rb->Mutex);
      rb->RefCount++;
      /*printf("RB INCR %p (%d) to %d\n", (void*) rb, rb->Name, rb->RefCount);*/
      _glthread_UNLOCK_MUTEX(rb->Mutex);
      *ptr = rb;
   }
}


/**
 * Create a new combined depth/stencil renderbuffer for implementing
 * the GL_EXT_packed_depth_stencil extension.
 * \return new depth/stencil renderbuffer
 */
struct gl_renderbuffer *
_mesa_new_depthstencil_renderbuffer(GLcontext *ctx, GLuint name)
{
   struct gl_renderbuffer *dsrb;

   dsrb = _mesa_new_renderbuffer(ctx, name);
   if (!dsrb)
      return NULL;

   /* init fields not covered by _mesa_new_renderbuffer() */
   dsrb->InternalFormat = GL_DEPTH24_STENCIL8_EXT;
   dsrb->_ActualFormat = GL_DEPTH24_STENCIL8_EXT;
   dsrb->AllocStorage = _mesa_soft_renderbuffer_storage;

   return dsrb;
}
