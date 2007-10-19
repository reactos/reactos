/*
 * Mesa 3-D graphics library
 * Version:  6.5
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
#include "macros.h"
#include "imports.h"
#include "fbobject.h"

#include "s_depth.h"
#include "s_context.h"
#include "s_span.h"


/**
 * Do depth test for a horizontal span of fragments.
 * Input:  zbuffer - array of z values in the zbuffer
 *         z - array of fragment z values
 * Return:  number of fragments which pass the test.
 */
static GLuint
depth_test_span16( GLcontext *ctx, GLuint n,
                   GLushort zbuffer[], const GLuint z[], GLubyte mask[] )
{
   GLuint passed = 0;

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] < zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] < zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] <= zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] <= zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] >= zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] >= zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] > zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] > zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] != zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] != zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] == zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] == zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  zbuffer[i] = z[i];
		  passed++;
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	    passed = n;
	 }
	 break;
      case GL_NEVER:
         _mesa_bzero(mask, n * sizeof(GLubyte));
	 break;
      default:
         _mesa_problem(ctx, "Bad depth func in depth_test_span16");
   }

   return passed;
}


static GLuint
depth_test_span32( GLcontext *ctx, GLuint n,
                   GLuint zbuffer[], const GLuint z[], GLubyte mask[] )
{
   GLuint passed = 0;

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] < zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] < zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] <= zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] <= zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] >= zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] >= zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] > zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] > zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] != zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] != zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] == zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] == zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  zbuffer[i] = z[i];
		  passed++;
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	    passed = n;
	 }
	 break;
      case GL_NEVER:
         _mesa_bzero(mask, n * sizeof(GLubyte));
	 break;
      default:
         _mesa_problem(ctx, "Bad depth func in depth_test_span32");
   }

   return passed;
}



/*
 * Apply depth test to span of fragments.
 */
static GLuint
depth_test_span( GLcontext *ctx, struct sw_span *span)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   const GLint x = span->x;
   const GLint y = span->y;
   const GLuint count = span->end;
   const GLuint *zValues = span->array->z;
   GLubyte *mask = span->array->mask;
   GLuint passed;

   ASSERT((span->arrayMask & SPAN_XY) == 0);
   ASSERT(span->arrayMask & SPAN_Z);

   if (rb->GetPointer(ctx, rb, 0, 0)) {
      /* Directly access buffer */
      if (ctx->DrawBuffer->Visual.depthBits <= 16) {
         GLushort *zbuffer = (GLushort *) rb->GetPointer(ctx, rb, x, y);
         passed = depth_test_span16(ctx, count, zbuffer, zValues, mask);
      }
      else {
         GLuint *zbuffer = (GLuint *) rb->GetPointer(ctx, rb, x, y);
         passed = depth_test_span32(ctx, count, zbuffer, zValues, mask);
      }
   }
   else {
      /* read depth values from buffer, test, write back */
      if (rb->DataType == GL_UNSIGNED_SHORT) {
         GLushort zbuffer[MAX_WIDTH];
         rb->GetRow(ctx, rb, count, x, y, zbuffer);
         passed = depth_test_span16(ctx, count, zbuffer, zValues, mask );
         rb->PutRow(ctx, rb, count, x, y, zbuffer, NULL);
      }
      else {
         GLuint zbuffer[MAX_WIDTH];
         ASSERT(rb->DataType == GL_UNSIGNED_INT);
         rb->GetRow(ctx, rb, count, x, y, zbuffer);
         passed = depth_test_span32(ctx, count, zbuffer, zValues, mask );
         rb->PutRow(ctx, rb, count, x, y, zbuffer, NULL);
      }
   }

   if (passed < count) {
      span->writeAll = GL_FALSE;
   }
   return passed;
}



#define Z_ADDRESS(X, Y)   (zStart + (Y) * stride + (X))


/*
 * Do depth testing for an array of fragments at assorted locations.
 */
static void
direct_depth_test_pixels16(GLcontext *ctx, GLushort *zStart, GLuint stride,
                           GLuint n, const GLint x[], const GLint y[],
                           const GLuint z[], GLubyte mask[] )
{
   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] < *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] < *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] <= *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] <= *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] >= *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] >= *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] > *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] > *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] != *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] != *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] == *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] == *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
		  *zptr = z[i];
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	 }
	 break;
      case GL_NEVER:
	 /* depth test never passes */
         _mesa_bzero(mask, n * sizeof(GLubyte));
	 break;
      default:
         _mesa_problem(ctx, "Bad depth func in direct_depth_test_pixels");
   }
}



/*
 * Do depth testing for an array of fragments with direct access to zbuffer.
 */
static void
direct_depth_test_pixels32(GLcontext *ctx, GLuint *zStart, GLuint stride,
                           GLuint n, const GLint x[], const GLint y[],
                           const GLuint z[], GLubyte mask[] )
{
   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] < *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] < *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] <= *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] <= *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] >= *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] >= *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] > *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] > *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] != *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] != *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] == *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  if (z[i] == *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
		  *zptr = z[i];
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	 }
	 break;
      case GL_NEVER:
	 /* depth test never passes */
         _mesa_bzero(mask, n * sizeof(GLubyte));
	 break;
      default:
         _mesa_problem(ctx, "Bad depth func in direct_depth_test_pixels");
   }
}




static GLuint
depth_test_pixels( GLcontext *ctx, struct sw_span *span )
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   const GLuint count = span->end;
   const GLint *x = span->array->x;
   const GLint *y = span->array->y;
   const GLuint *z = span->array->z;
   GLubyte *mask = span->array->mask;

   if (rb->GetPointer(ctx, rb, 0, 0)) {
      /* Directly access values */
      if (rb->DataType == GL_UNSIGNED_SHORT) {
         GLushort *zStart = (GLushort *) rb->Data;
         GLuint stride = rb->Width;
         direct_depth_test_pixels16(ctx, zStart, stride, count, x, y, z, mask);
      }
      else {
         GLuint *zStart = (GLuint *) rb->Data;
         GLuint stride = rb->Width;
         direct_depth_test_pixels32(ctx, zStart, stride, count, x, y, z, mask);
      }
   }
   else {
      /* read depth values from buffer, test, write back */
      if (rb->DataType == GL_UNSIGNED_SHORT) {
         GLushort zbuffer[MAX_WIDTH];
         _swrast_get_values(ctx, rb, count, x, y, zbuffer, sizeof(GLushort));
         depth_test_span16(ctx, count, zbuffer, z, mask );
         rb->PutValues(ctx, rb, count, x, y, zbuffer, NULL);
      }
      else {
         GLuint zbuffer[MAX_WIDTH];
         ASSERT(rb->DataType == GL_UNSIGNED_INT);
         _swrast_get_values(ctx, rb, count, x, y, zbuffer, sizeof(GLuint));
         depth_test_span32(ctx, count, zbuffer, z, mask );
         rb->PutValues(ctx, rb, count, x, y, zbuffer, NULL);
      }
   }

   return count; /* not really correct, but OK */
}


/**
 * Apply depth (Z) buffer testing to the span.
 * \return approx number of pixels that passed (only zero is reliable)
 */
GLuint
_swrast_depth_test_span( GLcontext *ctx, struct sw_span *span)
{
   if (span->arrayMask & SPAN_XY)
      return depth_test_pixels(ctx, span);
   else
      return depth_test_span(ctx, span);
}


/**
 * GL_EXT_depth_bounds_test extension.
 * Discard fragments depending on whether the corresponding Z-buffer
 * values are outside the depth bounds test range.
 * Note: we test the Z buffer values, not the fragment Z values!
 * \return GL_TRUE if any fragments pass, GL_FALSE if no fragments pass
 */
GLboolean
_swrast_depth_bounds_test( GLcontext *ctx, struct sw_span *span )
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLuint zMin = (GLuint) (ctx->Depth.BoundsMin * fb->_DepthMaxF + 0.5F);
   GLuint zMax = (GLuint) (ctx->Depth.BoundsMax * fb->_DepthMaxF + 0.5F);
   GLubyte *mask = span->array->mask;
   const GLuint count = span->end;
   GLuint i;
   GLboolean anyPass = GL_FALSE;

   if (rb->DataType == GL_UNSIGNED_SHORT) {
      /* get 16-bit values */
      GLushort zbuffer16[MAX_WIDTH], *zbuffer;
      if (span->arrayMask & SPAN_XY) {
         _swrast_get_values(ctx, rb, count, span->array->x, span->array->y,
                            zbuffer16, sizeof(GLushort));
         zbuffer = zbuffer16;
      }
      else {
         zbuffer = rb->GetPointer(ctx, rb, span->x, span->y);
         if (!zbuffer) {
            rb->GetRow(ctx, rb, count, span->x, span->y, zbuffer16);
            zbuffer = zbuffer16;
         }
      }
      assert(zbuffer);

      /* Now do the tests */
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            if (zbuffer[i] < zMin || zbuffer[i] > zMax)
               mask[i] = GL_FALSE;
            else
               anyPass = GL_TRUE;
         }
      }
   }
   else {
      /* get 32-bit values */
      GLuint zbuffer32[MAX_WIDTH], *zbuffer;
      if (span->arrayMask & SPAN_XY) {
         _swrast_get_values(ctx, rb, count, span->array->x, span->array->y,
                            zbuffer32, sizeof(GLuint));
         zbuffer = zbuffer32;
      }
      else {
         zbuffer = rb->GetPointer(ctx, rb, span->x, span->y);
         if (!zbuffer) {
            rb->GetRow(ctx, rb, count, span->x, span->y, zbuffer32);
            zbuffer = zbuffer32;
         }
      }
      assert(zbuffer);

      /* Now do the tests */
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            if (zbuffer[i] < zMin || zbuffer[i] > zMax)
               mask[i] = GL_FALSE;
            else
               anyPass = GL_TRUE;
         }
      }
   }

   return anyPass;
}



/**********************************************************************/
/*****                      Read Depth Buffer                     *****/
/**********************************************************************/


/**
 * Read a span of depth values from the depth buffer.
 * This function does clipping before calling the device driver function.
 *
 * XXXX this is no longer a swrast function!!!
 *
 */
void
_swrast_read_depth_span( GLcontext *ctx, struct gl_renderbuffer *rb,
                         GLint n, GLint x, GLint y, GLuint depth[] )
{
   if (y < 0 || y >= (GLint) rb->Height ||
       x + (GLint) n <= 0 || x >= (GLint) rb->Width) {
      /* span is completely outside framebuffer */
      GLint i;
      for (i = 0; i < n; i++)
         depth[i] = 0;
      return;
   }

   if (x < 0) {
      GLint dx = -x;
      GLint i;
      for (i = 0; i < dx; i++)
         depth[i] = 0;
      x = 0;
      n -= dx;
      depth += dx;
   }
   if (x + n > (GLint) rb->Width) {
      GLint dx = x + n - (GLint) rb->Width;
      GLint i;
      for (i = 0; i < dx; i++)
         depth[n - i - 1] = 0;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   /* we'll always return 32-bit values to our caller */
   if (!rb) {
      _mesa_bzero(depth, n * sizeof(GLuint));
   }
   else if (rb->DataType == GL_UNSIGNED_INT) {
      rb->GetRow(ctx, rb, n, x, y, depth);
   }
   else {
      GLushort temp[MAX_WIDTH];
      GLuint i;
      ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
      rb->GetRow(ctx, rb, n, x, y, temp);
      for (i = 0; i < n; i++) {
         depth[i] = temp[i];
      }
   }
}


/**
 * Return a span of depth values from the depth buffer as floats in [0,1].
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  depth - the array of depth values
 */
void
_swrast_read_depth_span_float( GLcontext *ctx, struct gl_renderbuffer *rb,
                               GLint n, GLint x, GLint y, GLfloat depth[] )
{
   const GLfloat scale = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   GLuint temp[MAX_WIDTH];
   GLint i;

   assert(n <= MAX_WIDTH);

   _swrast_read_depth_span(ctx, rb, n, x, y, temp);
   for (i = 0; i < n; i++) {
      depth[i] = temp[i] * scale;
   }
}


/**
 * Clear the depth buffer.
 */
void
_swrast_clear_depth_buffer( GLcontext *ctx, struct gl_renderbuffer *rb )
{
   GLuint clearValue;
   GLint x, y, width, height;

   if (!rb || !ctx->Depth.Mask) {
      /* no depth buffer, or writing to it is disabled */
      return;
   }

   /* compute integer clearing value */
   if (ctx->Depth.Clear == 1.0) {
      clearValue = ctx->DrawBuffer->_DepthMax;
   }
   else {
      clearValue = (GLuint) (ctx->Depth.Clear * ctx->DrawBuffer->_DepthMaxF);
   }

   assert(rb->_BaseFormat == GL_DEPTH_COMPONENT);

   /* compute region to clear */
   x = ctx->DrawBuffer->_Xmin;
   y = ctx->DrawBuffer->_Ymin;
   width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;

   if (rb->GetPointer(ctx, rb, 0, 0)) {
      /* Direct buffer access is possible.  Either this is just malloc'd
       * memory, or perhaps the driver mmap'd the zbuffer memory.
       */
      if (rb->DataType == GL_UNSIGNED_SHORT) {
         if (width == rb->Width &&
             (clearValue & 0xff) == ((clearValue >> 8) & 0xff)) {
            /* optimized case */
            GLushort *dst = (GLushort *) rb->GetPointer(ctx, rb, x, y);
            GLuint len = width * height * sizeof(GLushort);
            _mesa_memset(dst, (clearValue & 0xff), len);
         }
         else {
            /* general case */
            GLint i, j;
            for (i = 0; i < height; i++) {
               GLushort *dst = (GLushort *) rb->GetPointer(ctx, rb, x, y + i);
               for (j = 0; j < width; j++) {
                  dst[j] = clearValue;
               }
            }
         }
      }
      else {
         GLint i, j;
         ASSERT(rb->DataType == GL_UNSIGNED_INT);
         for (i = 0; i < height; i++) {
            GLuint *dst = (GLuint *) rb->GetPointer(ctx, rb, x, y + i);
            for (j = 0; j < width; j++) {
               dst[j] = clearValue;
            }
         }
      }
   }
   else {
      /* Direct access not possible.  Use PutRow to write new values. */
      if (rb->DataType == GL_UNSIGNED_SHORT) {
         GLushort clearRow[MAX_WIDTH];
         GLint i, j;
         for (j = 0; j < width; j++) {
            clearRow[j] = clearValue;
         }
         for (i = 0; i < height; i++) {
            rb->PutRow(ctx, rb, width, x, y + i, clearRow, NULL);
         }
      }
      else {
         GLuint clearRow[MAX_WIDTH];
         GLint i, j;
         assert(rb->DataType == GL_UNSIGNED_INT);
         for (j = 0; j < width; j++) {
            clearRow[j] = clearValue;
         }
         for (i = 0; i < height; i++) {
            rb->PutRow(ctx, rb, width, x, y + i, clearRow, NULL);
         }
      }
   }
}
