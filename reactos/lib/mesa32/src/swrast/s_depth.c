/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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

#include "s_depth.h"
#include "s_context.h"


/**
 * Return address of depth buffer value for given window coord.
 */
GLvoid *
_swrast_zbuffer_address(GLcontext *ctx, GLint x, GLint y)
{
   if (ctx->Visual.depthBits <= 16)
      return (GLushort *) ctx->DrawBuffer->DepthBuffer
         + ctx->DrawBuffer->Width * y + x;
   else
      return (GLuint *) ctx->DrawBuffer->DepthBuffer
         + ctx->DrawBuffer->Width * y + x;
}


#define Z_ADDRESS16( CTX, X, Y )				\
            ( ((GLushort *) (CTX)->DrawBuffer->DepthBuffer)	\
              + (CTX)->DrawBuffer->Width * (Y) + (X) )

#define Z_ADDRESS32( CTX, X, Y )				\
            ( ((GLuint *) (CTX)->DrawBuffer->DepthBuffer)	\
              + (CTX)->DrawBuffer->Width * (Y) + (X) )



/**********************************************************************/
/*****                   Depth Testing Functions                  *****/
/**********************************************************************/


/*
 * Do depth test for an array of fragments.  This is used both for
 * software and hardware Z buffers.
 * Input:  zbuffer - array of z values in the zbuffer
 *         z - array of fragment z values
 * Return:  number of fragments which pass the test.
 */
static GLuint
depth_test_span16( GLcontext *ctx, GLuint n,
                   GLushort zbuffer[], const GLdepth z[], GLubyte mask[] )
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
                   GLuint zbuffer[], const GLdepth z[], GLubyte mask[] )
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
 * Apply depth test to span of fragments.  Hardware or software z buffer.
 */
static GLuint
depth_test_span( GLcontext *ctx, struct sw_span *span)
{
   const GLint x = span->x;
   const GLint y = span->y;
   const GLuint n = span->end;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   ASSERT((span->arrayMask & SPAN_XY) == 0);
   ASSERT(span->arrayMask & SPAN_Z);
   
   if (swrast->Driver.ReadDepthSpan) {
      /* hardware-based depth buffer */
      GLdepth zbuffer[MAX_WIDTH];
      GLuint passed;
      (*swrast->Driver.ReadDepthSpan)(ctx, n, x, y, zbuffer);
      passed = depth_test_span32(ctx, n, zbuffer, span->array->z,
                                 span->array->mask);
      ASSERT(swrast->Driver.WriteDepthSpan);
      (*swrast->Driver.WriteDepthSpan)(ctx, n, x, y, zbuffer,
                                       span->array->mask);
      if (passed < n)
         span->writeAll = GL_FALSE;
      return passed;
   }
   else {
      GLuint passed;
      /* software depth buffer */
      if (ctx->Visual.depthBits <= 16) {
         GLushort *zptr = (GLushort *) Z_ADDRESS16(ctx, x, y);
         passed = depth_test_span16(ctx, n, zptr, span->array->z, span->array->mask);
      }
      else {
         GLuint *zptr = (GLuint *) Z_ADDRESS32(ctx, x, y);
         passed = depth_test_span32(ctx, n, zptr, span->array->z, span->array->mask);
      }
#if 1
      if (passed < span->end) {
         span->writeAll = GL_FALSE;
      }
#else
      /* this causes a glDrawPixels(GL_DEPTH_COMPONENT) conformance failure */
      if (passed < span->end) {
	  span->writeAll = GL_FALSE;
	  if (passed == 0) {
	      span->end = 0;
	      return 0;
	  }
	  while (span->end > 0  &&  span->mask[span->end - 1] == 0)
	      span->end --;
      }
#endif
      return passed;
   }
}




/*
 * Do depth testing for an array of fragments using software Z buffer.
 */
static void
software_depth_test_pixels16( GLcontext *ctx, GLuint n,
                              const GLint x[], const GLint y[],
                              const GLdepth z[], GLubyte mask[] )
{
   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
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
         _mesa_problem(ctx, "Bad depth func in software_depth_test_pixels");
   }
}



/*
 * Do depth testing for an array of fragments using software Z buffer.
 */
static void
software_depth_test_pixels32( GLcontext *ctx, GLuint n,
                              const GLint x[], const GLint y[],
                              const GLdepth z[], GLubyte mask[] )
{
   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
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
         _mesa_problem(ctx, "Bad depth func in software_depth_test_pixels");
   }
}



/*
 * Do depth testing for an array of pixels using hardware Z buffer.
 * Input/output:  zbuffer - array of depth values from Z buffer
 * Input:  z - array of fragment z values.
 */
static void
hardware_depth_test_pixels( GLcontext *ctx, GLuint n, GLdepth zbuffer[],
                            const GLdepth z[], GLubyte mask[] )
{
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
		  if (z[i] <= zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
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
		  if (z[i] <= zbuffer[i]) {
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
		  if (z[i] >= zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
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
		  if (z[i] >= zbuffer[i]) {
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
		  if (z[i] > zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
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
		  if (z[i] > zbuffer[i]) {
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
		  if (z[i] != zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
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
		  if (z[i] != zbuffer[i]) {
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
		  if (z[i] == zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
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
		  if (z[i] == zbuffer[i]) {
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
		  zbuffer[i] = z[i];
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
         _mesa_problem(ctx, "Bad depth func in hardware_depth_test_pixels");
   }
}



static GLuint
depth_test_pixels( GLcontext *ctx, struct sw_span *span )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint n = span->end;
   const GLint *x = span->array->x;
   const GLint *y = span->array->y;
   const GLdepth *z = span->array->z;
   GLubyte *mask = span->array->mask;

   if (swrast->Driver.ReadDepthPixels) {
      /* read depth values from hardware Z buffer */
      GLdepth zbuffer[MAX_WIDTH];
      (*swrast->Driver.ReadDepthPixels)(ctx, n, x, y, zbuffer);

      hardware_depth_test_pixels( ctx, n, zbuffer, z, mask );

      /* update hardware Z buffer with new values */
      assert(swrast->Driver.WriteDepthPixels);
      (*swrast->Driver.WriteDepthPixels)(ctx, n, x, y, zbuffer, mask );
   }
   else {
      /* software depth testing */
      if (ctx->Visual.depthBits <= 16)
         software_depth_test_pixels16(ctx, n, x, y, z, mask);
      else
         software_depth_test_pixels32(ctx, n, x, y, z, mask);
   }
   return n; /* not really correct, but OK */
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
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLdepth zMin = (GLdepth) (ctx->Depth.BoundsMin * ctx->DepthMaxF + 0.5F);
   GLdepth zMax = (GLdepth) (ctx->Depth.BoundsMax * ctx->DepthMaxF + 0.5F);
   GLubyte *mask = span->array->mask;
   GLuint i;
   GLboolean anyPass = GL_FALSE;

   if (swrast->Driver.ReadDepthPixels) {
      /* read depth values from hardware Z buffer */
      GLdepth zbuffer[MAX_WIDTH];
      ASSERT(span->end <= MAX_WIDTH);
      if (span->arrayMask & SPAN_XY)
         (*swrast->Driver.ReadDepthPixels)(ctx, span->end, span->array->x,
                                           span->array->y, zbuffer);
      else
         (*swrast->Driver.ReadDepthSpan)(ctx, span->end, span->x, span->y,
                                         zbuffer);
      for (i = 0; i < span->end; i++) {
         if (mask[i]) {
            if (zbuffer[i] < zMin || zbuffer[i] > zMax)
               mask[i] = GL_FALSE;
            else
               anyPass = GL_TRUE;
         }
      }
   }
   else {
      /* software Z buffer */
      if (span->arrayMask & SPAN_XY) {
         if (ctx->Visual.depthBits <= 16) {
            /* 16 bits / Z */
            for (i = 0; i < span->end; i++) {
               if (mask[i]) {
                  const GLushort *zPtr = Z_ADDRESS16(ctx, span->array->x[i],
                                                     span->array->y[i]);
                  if (*zPtr < zMin || *zPtr > zMax)
                     mask[i] = GL_FALSE;
                  else
                     anyPass = GL_TRUE;
               }
            }
         }
         else {
            /* 32 bits / Z */
            for (i = 0; i < span->end; i++) {
               if (mask[i]) {
                  const GLuint *zPtr = Z_ADDRESS32(ctx, span->array->x[i],
                                                   span->array->y[i]);
                  if (*zPtr < zMin || *zPtr > zMax)
                     mask[i] = GL_FALSE;
                  else
                     anyPass = GL_TRUE;
               }
            }
         }
      }
      else {
         if (ctx->Visual.depthBits <= 16) {
            /* 16 bits / Z */
            const GLushort *zPtr = Z_ADDRESS16(ctx, span->x, span->y);
            for (i = 0; i < span->end; i++) {
               if (mask[i]) {
                  if (zPtr[i] < zMin || zPtr[i] > zMax)
                     mask[i] = GL_FALSE;
                  else
                     anyPass = GL_TRUE;
               }
            }
         }
         else {
            /* 32 bits / Z */
            const GLuint *zPtr = Z_ADDRESS32(ctx, span->x, span->y);
            for (i = 0; i < span->end; i++) {
               if (mask[i]) {
                  if (zPtr[i] < zMin || zPtr[i] > zMax)
                     mask[i] = GL_FALSE;
                  else
                     anyPass = GL_TRUE;
               }
            }
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
 */
void
_swrast_read_depth_span( GLcontext *ctx,
                       GLint n, GLint x, GLint y, GLdepth depth[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (y < 0 || y >= (GLint) ctx->DrawBuffer->Height ||
       x + (GLint) n <= 0 || x >= (GLint) ctx->DrawBuffer->Width) {
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
   if (x + n > (GLint) ctx->DrawBuffer->Width) {
      GLint dx = x + n - (GLint) ctx->DrawBuffer->Width;
      GLint i;
      for (i = 0; i < dx; i++)
         depth[n - i - 1] = 0;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   if (ctx->DrawBuffer->DepthBuffer) {
      /* read from software depth buffer */
      if (ctx->Visual.depthBits <= 16) {
         const GLushort *zptr = Z_ADDRESS16( ctx, x, y );
         GLint i;
         for (i = 0; i < n; i++) {
            depth[i] = zptr[i];
         }
      }
      else {
         const GLuint *zptr = Z_ADDRESS32( ctx, x, y );
         GLint i;
         for (i = 0; i < n; i++) {
            depth[i] = zptr[i];
         }
      }
   }
   else if (swrast->Driver.ReadDepthSpan) {
      /* read from hardware depth buffer */
      (*swrast->Driver.ReadDepthSpan)( ctx, n, x, y, depth );
   }
   else {
      /* no depth buffer */
      _mesa_bzero(depth, n * sizeof(GLfloat));
   }

}




/**
 * Return a span of depth values from the depth buffer as floats in [0,1].
 * This is used for both hardware and software depth buffers.
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  depth - the array of depth values
 */
void
_swrast_read_depth_span_float( GLcontext *ctx,
                             GLint n, GLint x, GLint y, GLfloat depth[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLfloat scale = 1.0F / ctx->DepthMaxF;

   if (y < 0 || y >= (GLint) ctx->DrawBuffer->Height ||
       x + (GLint) n <= 0 || x >= (GLint) ctx->DrawBuffer->Width) {
      /* span is completely outside framebuffer */
      GLint i;
      for (i = 0; i < n; i++)
         depth[i] = 0.0F;
      return;
   }

   if (x < 0) {
      GLint dx = -x;
      GLint i;
      for (i = 0; i < dx; i++)
         depth[i] = 0.0F;
      n -= dx;
      x = 0;
   }
   if (x + n > (GLint) ctx->DrawBuffer->Width) {
      GLint dx = x + n - (GLint) ctx->DrawBuffer->Width;
      GLint i;
      for (i = 0; i < dx; i++)
         depth[n - i - 1] = 0.0F;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   if (ctx->DrawBuffer->DepthBuffer) {
      /* read from software depth buffer */
      if (ctx->Visual.depthBits <= 16) {
         const GLushort *zptr = Z_ADDRESS16( ctx, x, y );
         GLint i;
         for (i = 0; i < n; i++) {
            depth[i] = (GLfloat) zptr[i] * scale;
         }
      }
      else {
         const GLuint *zptr = Z_ADDRESS32( ctx, x, y );
         GLint i;
         for (i = 0; i < n; i++) {
            depth[i] = (GLfloat) zptr[i] * scale;
         }
      }
   }
   else if (swrast->Driver.ReadDepthSpan) {
      /* read from hardware depth buffer */
      GLdepth d[MAX_WIDTH];
      GLint i;
      assert(n <= MAX_WIDTH);
      (*swrast->Driver.ReadDepthSpan)( ctx, n, x, y, d );
      for (i = 0; i < n; i++) {
	 depth[i] = d[i] * scale;
      }
   }
   else {
      /* no depth buffer */
      _mesa_bzero(depth, n * sizeof(GLfloat));
   }
}



/**********************************************************************/
/*****                Allocate and Clear Depth Buffer             *****/
/**********************************************************************/



/**
 * Allocate a new depth buffer.  If there's already a depth buffer allocated
 * it will be free()'d.  The new depth buffer will be uniniitalized.
 * This function is only called through Driver.alloc_depth_buffer.
 */
void
_swrast_alloc_depth_buffer( GLframebuffer *buffer )
{
   GLint bytesPerValue;

   ASSERT(buffer->UseSoftwareDepthBuffer);

   /* deallocate current depth buffer if present */
   if (buffer->DepthBuffer) {
      MESA_PBUFFER_FREE(buffer->DepthBuffer);
      buffer->DepthBuffer = NULL;
   }

   /* allocate new depth buffer, but don't initialize it */
   if (buffer->Visual.depthBits <= 16)
      bytesPerValue = sizeof(GLushort);
   else
      bytesPerValue = sizeof(GLuint);

   buffer->DepthBuffer = MESA_PBUFFER_ALLOC(buffer->Width * buffer->Height
                                            * bytesPerValue);

   if (!buffer->DepthBuffer) {
      /* out of memory */
      GET_CURRENT_CONTEXT(ctx);
      if (ctx) {
         ctx->Depth.Test = GL_FALSE;
         ctx->NewState |= _NEW_DEPTH;
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "Couldn't allocate depth buffer");
      }
   }
}


/**
 * Clear the depth buffer.  If the depth buffer doesn't exist yet we'll
 * allocate it now.
 * This function is only called through Driver.clear_depth_buffer.
 */
void
_swrast_clear_depth_buffer( GLcontext *ctx )
{
   if (ctx->Visual.depthBits == 0
       || !ctx->DrawBuffer->DepthBuffer
       || !ctx->Depth.Mask) {
      /* no depth buffer, or writing to it is disabled */
      return;
   }

   /* The loops in this function have been written so the IRIX 5.3
    * C compiler can unroll them.  Hopefully other compilers can too!
    */

   if (ctx->Scissor.Enabled) {
      /* only clear scissor region */
      if (ctx->Visual.depthBits <= 16) {
         const GLushort clearValue = (GLushort) (ctx->Depth.Clear * ctx->DepthMax);
         const GLint rows = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
         const GLint cols = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
         const GLint rowStride = ctx->DrawBuffer->Width;
         GLushort *dRow = (GLushort *) ctx->DrawBuffer->DepthBuffer
            + ctx->DrawBuffer->_Ymin * rowStride + ctx->DrawBuffer->_Xmin;
         GLint i, j;
         for (i = 0; i < rows; i++) {
            for (j = 0; j < cols; j++) {
               dRow[j] = clearValue;
            }
            dRow += rowStride;
         }
      }
      else {
         const GLuint clearValue = (GLuint) (ctx->Depth.Clear * ctx->DepthMax);
         const GLint rows = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
         const GLint cols = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
         const GLint rowStride = ctx->DrawBuffer->Width;
         GLuint *dRow = (GLuint *) ctx->DrawBuffer->DepthBuffer
            + ctx->DrawBuffer->_Ymin * rowStride + ctx->DrawBuffer->_Xmin;
         GLint i, j;
         for (i = 0; i < rows; i++) {
            for (j = 0; j < cols; j++) {
               dRow[j] = clearValue;
            }
            dRow += rowStride;
         }
      }
   }
   else {
      /* clear whole buffer */
      if (ctx->Visual.depthBits <= 16) {
         const GLushort clearValue = (GLushort) (ctx->Depth.Clear * ctx->DepthMax);
         if ((clearValue & 0xff) == (clearValue >> 8)) {
            if (clearValue == 0) {
               _mesa_bzero(ctx->DrawBuffer->DepthBuffer,
                     2*ctx->DrawBuffer->Width*ctx->DrawBuffer->Height);
            }
            else {
               /* lower and upper bytes of clear_value are same, use MEMSET */
               MEMSET( ctx->DrawBuffer->DepthBuffer, clearValue & 0xff,
                       2 * ctx->DrawBuffer->Width * ctx->DrawBuffer->Height);
            }
         }
         else {
            GLushort *d = (GLushort *) ctx->DrawBuffer->DepthBuffer;
            GLint n = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height;
            while (n >= 16) {
               d[0] = clearValue;    d[1] = clearValue;
               d[2] = clearValue;    d[3] = clearValue;
               d[4] = clearValue;    d[5] = clearValue;
               d[6] = clearValue;    d[7] = clearValue;
               d[8] = clearValue;    d[9] = clearValue;
               d[10] = clearValue;   d[11] = clearValue;
               d[12] = clearValue;   d[13] = clearValue;
               d[14] = clearValue;   d[15] = clearValue;
               d += 16;
               n -= 16;
            }
            while (n > 0) {
               *d++ = clearValue;
               n--;
            }
         }
      }
      else {
         /* >16 bit depth buffer */
         const GLuint clearValue = (GLuint) (ctx->Depth.Clear * ctx->DepthMax);
         if (clearValue == 0) {
            _mesa_bzero(ctx->DrawBuffer->DepthBuffer,
                ctx->DrawBuffer->Width*ctx->DrawBuffer->Height*sizeof(GLuint));
         }
         else {
            GLint n = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height;
            GLuint *d = (GLuint *) ctx->DrawBuffer->DepthBuffer;
            while (n >= 16) {
               d[0] = clearValue;    d[1] = clearValue;
               d[2] = clearValue;    d[3] = clearValue;
               d[4] = clearValue;    d[5] = clearValue;
               d[6] = clearValue;    d[7] = clearValue;
               d[8] = clearValue;    d[9] = clearValue;
               d[10] = clearValue;   d[11] = clearValue;
               d[12] = clearValue;   d[13] = clearValue;
               d[14] = clearValue;   d[15] = clearValue;
               d += 16;
               n -= 16;
            }
            while (n > 0) {
               *d++ = clearValue;
               n--;
            }
         }
      }
   }
}
