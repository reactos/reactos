/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
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
 * Renderbuffer adaptors.
 * These fuctions are used to convert rendering from core Mesa's GLchan
 * colors to 8 or 16-bit color channels in RGBA renderbuffers.
 * This means Mesa can be compiled for 16 or 32-bit color processing
 * and still render into 8 and 16-bit/channel renderbuffers.
 */


#include "glheader.h"
#include "mtypes.h"
#include "colormac.h"
#include "renderbuffer.h"
#include "rbadaptors.h"


static void
Delete_wrapper(struct gl_renderbuffer *rb)
{
   /* Decrement reference count on the buffer we're wrapping and delete
    * it if refcount hits zero.
    */
   _mesa_reference_renderbuffer(&rb->Wrapped, NULL);

   /* delete myself */
   _mesa_delete_renderbuffer(rb);
}


static GLboolean
AllocStorage_wrapper(GLcontext *ctx, struct gl_renderbuffer *rb,
                     GLenum internalFormat, GLuint width, GLuint height)
{
   GLboolean b = rb->Wrapped->AllocStorage(ctx, rb->Wrapped, internalFormat,
                                           width, height);
   if (b) {
      rb->Width = width;
      rb->Height = height;
   }
   return b;
}


static void *
GetPointer_wrapper(GLcontext *ctx, struct gl_renderbuffer *rb,
                   GLint x, GLint y)
{
   (void) ctx;
   (void) rb;
   (void) x;
   (void) y;
   return NULL;
}


static void
GetRow_16wrap8(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, void *values)
{
   GLubyte values8[MAX_WIDTH * 4];
   GLushort *values16 = (GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);
   ASSERT(count <= MAX_WIDTH);

   /* get 8bpp values */
   rb->Wrapped->GetRow(ctx, rb->Wrapped, count, x, y, values8);

   /* convert 8bpp to 16bpp */
   for (i = 0; i < 4 * count; i++) {
      values16[i] = (values8[i] << 8) | values8[i];
   }
}


static void
GetValues_16wrap8(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], void *values)
{
   GLubyte values8[MAX_WIDTH * 4];
   GLushort *values16 = (GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);

   rb->Wrapped->GetValues(ctx, rb->Wrapped, count, x, y, values8);

   for (i = 0; i < 4 * count; i++) {
      values16[i] = (values8[i] << 8) | values8[i];
   }
}


static void
PutRow_16wrap8(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, const void *values, const GLubyte *mask)
{
   GLubyte values8[MAX_WIDTH * 4];
   GLushort *values16 = (GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < 4 * count; i++) {
      values8[i] = values16[i] >> 8;
   }
   rb->Wrapped->PutRow(ctx, rb->Wrapped, count, x, y, values8, mask);
}


static void
PutRowRGB_16wrap8(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  GLint x, GLint y, const void *values, const GLubyte *mask)
{
   GLubyte values8[MAX_WIDTH * 3];
   GLushort *values16 = (GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < 3 * count; i++) {
      values8[i] = values16[i] >> 8;
   }
   rb->Wrapped->PutRowRGB(ctx, rb->Wrapped, count, x, y, values8, mask);
}


static void
PutMonoRow_16wrap8(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   GLint x, GLint y, const void *value, const GLubyte *mask)
{
   GLubyte value8[4];
   GLushort *value16 = (GLushort *) value;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);
   value8[0] = value16[0] >> 8;
   value8[1] = value16[1] >> 8;
   value8[2] = value16[2] >> 8;
   value8[3] = value16[3] >> 8;
   rb->Wrapped->PutMonoRow(ctx, rb->Wrapped, count, x, y, value8, mask);
}


static void
PutValues_16wrap8(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], const void *values,
                  const GLubyte *mask)
{
   GLubyte values8[MAX_WIDTH * 4];
   GLushort *values16 = (GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < 4 * count; i++) {
      values8[i] = values16[i] >> 8;
   }
   rb->Wrapped->PutValues(ctx, rb->Wrapped, count, x, y, values8, mask);
}


static void
PutMonoValues_16wrap8(GLcontext *ctx, struct gl_renderbuffer *rb,
                      GLuint count, const GLint x[], const GLint y[],
                      const void *value, const GLubyte *mask)
{
   GLubyte value8[4];
   GLushort *value16 = (GLushort *) value;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);
   value8[0] = value16[0] >> 8;
   value8[1] = value16[1] >> 8;
   value8[2] = value16[2] >> 8;
   value8[3] = value16[3] >> 8;
   rb->Wrapped->PutMonoValues(ctx, rb->Wrapped, count, x, y, value8, mask);
}


/**
 * Wrap an 8-bit/channel renderbuffer with a 16-bit/channel
 * renderbuffer adaptor.
 */
struct gl_renderbuffer *
_mesa_new_renderbuffer_16wrap8(GLcontext *ctx, struct gl_renderbuffer *rb8)
{
   struct gl_renderbuffer *rb16;

   rb16 = _mesa_new_renderbuffer(ctx, rb8->Name);
   if (rb16) {
      ASSERT(rb8->DataType == GL_UNSIGNED_BYTE);
      ASSERT(rb8->_BaseFormat == GL_RGBA);

      _glthread_LOCK_MUTEX(rb8->Mutex);
      rb8->RefCount++;
      _glthread_UNLOCK_MUTEX(rb8->Mutex);

      rb16->InternalFormat = rb8->InternalFormat;
      rb16->_ActualFormat = rb8->_ActualFormat;
      rb16->_BaseFormat = rb8->_BaseFormat;
      rb16->DataType = GL_UNSIGNED_SHORT;
      /* Note: passing through underlying bits/channel */
      rb16->RedBits = rb8->RedBits;
      rb16->GreenBits = rb8->GreenBits;
      rb16->BlueBits = rb8->BlueBits;
      rb16->AlphaBits = rb8->AlphaBits;
      rb16->Wrapped = rb8;

      rb16->AllocStorage = AllocStorage_wrapper;
      rb16->Delete = Delete_wrapper;
      rb16->GetPointer = GetPointer_wrapper;
      rb16->GetRow = GetRow_16wrap8;
      rb16->GetValues = GetValues_16wrap8;
      rb16->PutRow = PutRow_16wrap8;
      rb16->PutRowRGB = PutRowRGB_16wrap8;
      rb16->PutMonoRow = PutMonoRow_16wrap8;
      rb16->PutValues = PutValues_16wrap8;
      rb16->PutMonoValues = PutMonoValues_16wrap8;
   }
   return rb16;
}




static void
GetRow_32wrap8(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, void *values)
{
   GLubyte values8[MAX_WIDTH * 4];
   GLfloat *values32 = (GLfloat *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);
   ASSERT(count <= MAX_WIDTH);

   /* get 8bpp values */
   rb->Wrapped->GetRow(ctx, rb->Wrapped, count, x, y, values8);

   /* convert 8bpp to 32bpp */
   for (i = 0; i < 4 * count; i++) {
      values32[i] = UBYTE_TO_FLOAT(values8[i]);
   }
}


static void
GetValues_32wrap8(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], void *values)
{
   GLubyte values8[MAX_WIDTH * 4];
   GLfloat *values32 = (GLfloat *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);

   rb->Wrapped->GetValues(ctx, rb->Wrapped, count, x, y, values8);

   for (i = 0; i < 4 * count; i++) {
      values32[i] = UBYTE_TO_FLOAT(values8[i]);
   }
}


static void
PutRow_32wrap8(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, const void *values, const GLubyte *mask)
{
   GLubyte values8[MAX_WIDTH * 4];
   GLfloat *values32 = (GLfloat *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < 4 * count; i++) {
      UNCLAMPED_FLOAT_TO_UBYTE(values8[i], values32[i]);
   }
   rb->Wrapped->PutRow(ctx, rb->Wrapped, count, x, y, values8, mask);
}


static void
PutRowRGB_32wrap8(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  GLint x, GLint y, const void *values, const GLubyte *mask)
{
   GLubyte values8[MAX_WIDTH * 3];
   GLfloat *values32 = (GLfloat *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < 3 * count; i++) {
      UNCLAMPED_FLOAT_TO_UBYTE(values8[i], values32[i]);
   }
   rb->Wrapped->PutRowRGB(ctx, rb->Wrapped, count, x, y, values8, mask);
}


static void
PutMonoRow_32wrap8(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   GLint x, GLint y, const void *value, const GLubyte *mask)
{
   GLubyte value8[4];
   GLfloat *value32 = (GLfloat *) value;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);
   UNCLAMPED_FLOAT_TO_UBYTE(value8[0], value32[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(value8[1], value32[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(value8[2], value32[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(value8[3], value32[3]);
   rb->Wrapped->PutMonoRow(ctx, rb->Wrapped, count, x, y, value8, mask);
}


static void
PutValues_32wrap8(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], const void *values,
                  const GLubyte *mask)
{
   GLubyte values8[MAX_WIDTH * 4];
   GLfloat *values32 = (GLfloat *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < 4 * count; i++) {
      UNCLAMPED_FLOAT_TO_UBYTE(values8[i], values32[i]);
   }
   rb->Wrapped->PutValues(ctx, rb->Wrapped, count, x, y, values8, mask);
}


static void
PutMonoValues_32wrap8(GLcontext *ctx, struct gl_renderbuffer *rb,
                      GLuint count, const GLint x[], const GLint y[],
                      const void *value, const GLubyte *mask)
{
   GLubyte value8[4];
   GLfloat *value32 = (GLfloat *) value;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_BYTE);
   UNCLAMPED_FLOAT_TO_UBYTE(value8[0], value32[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(value8[1], value32[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(value8[2], value32[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(value8[3], value32[3]);
   rb->Wrapped->PutMonoValues(ctx, rb->Wrapped, count, x, y, value8, mask);
}


/**
 * Wrap an 8-bit/channel renderbuffer with a 32-bit/channel
 * renderbuffer adaptor.
 */
struct gl_renderbuffer *
_mesa_new_renderbuffer_32wrap8(GLcontext *ctx, struct gl_renderbuffer *rb8)
{
   struct gl_renderbuffer *rb32;

   rb32 = _mesa_new_renderbuffer(ctx, rb8->Name);
   if (rb32) {
      ASSERT(rb8->DataType == GL_UNSIGNED_BYTE);
      ASSERT(rb8->_BaseFormat == GL_RGBA);

      _glthread_LOCK_MUTEX(rb8->Mutex);
      rb8->RefCount++;
      _glthread_UNLOCK_MUTEX(rb8->Mutex);

      rb32->InternalFormat = rb8->InternalFormat;
      rb32->_ActualFormat = rb8->_ActualFormat;
      rb32->_BaseFormat = rb8->_BaseFormat;
      rb32->DataType = GL_FLOAT;
      /* Note: passing through underlying bits/channel */
      rb32->RedBits = rb8->RedBits;
      rb32->GreenBits = rb8->GreenBits;
      rb32->BlueBits = rb8->BlueBits;
      rb32->AlphaBits = rb8->AlphaBits;
      rb32->Wrapped = rb8;

      rb32->AllocStorage = AllocStorage_wrapper;
      rb32->Delete = Delete_wrapper;
      rb32->GetPointer = GetPointer_wrapper;
      rb32->GetRow = GetRow_32wrap8;
      rb32->GetValues = GetValues_32wrap8;
      rb32->PutRow = PutRow_32wrap8;
      rb32->PutRowRGB = PutRowRGB_32wrap8;
      rb32->PutMonoRow = PutMonoRow_32wrap8;
      rb32->PutValues = PutValues_32wrap8;
      rb32->PutMonoValues = PutMonoValues_32wrap8;
   }
   return rb32;
}




static void
GetRow_32wrap16(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                GLint x, GLint y, void *values)
{
   GLushort values16[MAX_WIDTH * 4];
   GLfloat *values32 = (GLfloat *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_SHORT);
   ASSERT(count <= MAX_WIDTH);

   /* get 16bpp values */
   rb->Wrapped->GetRow(ctx, rb->Wrapped, count, x, y, values16);

   /* convert 16bpp to 32bpp */
   for (i = 0; i < 4 * count; i++) {
      values32[i] = USHORT_TO_FLOAT(values16[i]);
   }
}


static void
GetValues_32wrap16(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   const GLint x[], const GLint y[], void *values)
{
   GLushort values16[MAX_WIDTH * 4];
   GLfloat *values32 = (GLfloat *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_SHORT);

   rb->Wrapped->GetValues(ctx, rb->Wrapped, count, x, y, values16);

   for (i = 0; i < 4 * count; i++) {
      values32[i] = USHORT_TO_FLOAT(values16[i]);
   }
}


static void
PutRow_32wrap16(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                GLint x, GLint y, const void *values, const GLubyte *mask)
{
   GLushort values16[MAX_WIDTH * 4];
   GLfloat *values32 = (GLfloat *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_SHORT);
   for (i = 0; i < 4 * count; i++) {
      UNCLAMPED_FLOAT_TO_USHORT(values16[i], values32[i]);
   }
   rb->Wrapped->PutRow(ctx, rb->Wrapped, count, x, y, values16, mask);
}


static void
PutRowRGB_32wrap16(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   GLint x, GLint y, const void *values, const GLubyte *mask)
{
   GLushort values16[MAX_WIDTH * 3];
   GLfloat *values32 = (GLfloat *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_SHORT);
   for (i = 0; i < 3 * count; i++) {
      UNCLAMPED_FLOAT_TO_USHORT(values16[i], values32[i]);
   }
   rb->Wrapped->PutRowRGB(ctx, rb->Wrapped, count, x, y, values16, mask);
}


static void
PutMonoRow_32wrap16(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                    GLint x, GLint y, const void *value, const GLubyte *mask)
{
   GLushort value16[4];
   GLfloat *value32 = (GLfloat *) value;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_SHORT);
   UNCLAMPED_FLOAT_TO_USHORT(value16[0], value32[0]);
   UNCLAMPED_FLOAT_TO_USHORT(value16[1], value32[1]);
   UNCLAMPED_FLOAT_TO_USHORT(value16[2], value32[2]);
   UNCLAMPED_FLOAT_TO_USHORT(value16[3], value32[3]);
   rb->Wrapped->PutMonoRow(ctx, rb->Wrapped, count, x, y, value16, mask);
}


static void
PutValues_32wrap16(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   const GLint x[], const GLint y[], const void *values,
                   const GLubyte *mask)
{
   GLushort values16[MAX_WIDTH * 4];
   GLfloat *values32 = (GLfloat *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_SHORT);
   for (i = 0; i < 4 * count; i++) {
      UNCLAMPED_FLOAT_TO_USHORT(values16[i], values32[i]);
   }
   rb->Wrapped->PutValues(ctx, rb->Wrapped, count, x, y, values16, mask);
}


static void
PutMonoValues_32wrap16(GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLuint count, const GLint x[], const GLint y[],
                       const void *value, const GLubyte *mask)
{
   GLushort value16[4];
   GLfloat *value32 = (GLfloat *) value;
   ASSERT(rb->DataType == GL_FLOAT);
   ASSERT(rb->Wrapped->DataType == GL_UNSIGNED_SHORT);
   UNCLAMPED_FLOAT_TO_USHORT(value16[0], value32[0]);
   UNCLAMPED_FLOAT_TO_USHORT(value16[1], value32[1]);
   UNCLAMPED_FLOAT_TO_USHORT(value16[2], value32[2]);
   UNCLAMPED_FLOAT_TO_USHORT(value16[3], value32[3]);
   rb->Wrapped->PutMonoValues(ctx, rb->Wrapped, count, x, y, value16, mask);
}


/**
 * Wrap an 16-bit/channel renderbuffer with a 32-bit/channel
 * renderbuffer adaptor.
 */
struct gl_renderbuffer *
_mesa_new_renderbuffer_32wrap16(GLcontext *ctx, struct gl_renderbuffer *rb16)
{
   struct gl_renderbuffer *rb32;

   rb32 = _mesa_new_renderbuffer(ctx, rb16->Name);
   if (rb32) {
      ASSERT(rb16->DataType == GL_UNSIGNED_SHORT);
      ASSERT(rb16->_BaseFormat == GL_RGBA);

      _glthread_LOCK_MUTEX(rb16->Mutex);
      rb16->RefCount++;
      _glthread_UNLOCK_MUTEX(rb16->Mutex);

      rb32->InternalFormat = rb16->InternalFormat;
      rb32->_ActualFormat = rb16->_ActualFormat;
      rb32->_BaseFormat = rb16->_BaseFormat;
      rb32->DataType = GL_FLOAT;
      /* Note: passing through underlying bits/channel */
      rb32->RedBits = rb16->RedBits;
      rb32->GreenBits = rb16->GreenBits;
      rb32->BlueBits = rb16->BlueBits;
      rb32->AlphaBits = rb16->AlphaBits;
      rb32->Wrapped = rb16;

      rb32->AllocStorage = AllocStorage_wrapper;
      rb32->Delete = Delete_wrapper;
      rb32->GetPointer = GetPointer_wrapper;
      rb32->GetRow = GetRow_32wrap16;
      rb32->GetValues = GetValues_32wrap16;
      rb32->PutRow = PutRow_32wrap16;
      rb32->PutRowRGB = PutRowRGB_32wrap16;
      rb32->PutMonoRow = PutMonoRow_32wrap16;
      rb32->PutValues = PutValues_32wrap16;
      rb32->PutMonoValues = PutMonoValues_32wrap16;
   }
   return rb32;
}
