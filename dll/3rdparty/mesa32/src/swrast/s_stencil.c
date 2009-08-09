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


#include "main/glheader.h"
#include "main/context.h"
#include "main/imports.h"

#include "s_context.h"
#include "s_depth.h"
#include "s_stencil.h"
#include "s_span.h"



/* Stencil Logic:

IF stencil test fails THEN
   Apply fail-op to stencil value
   Don't write the pixel (RGBA,Z)
ELSE
   IF doing depth test && depth test fails THEN
      Apply zfail-op to stencil value
      Write RGBA and Z to appropriate buffers
   ELSE
      Apply zpass-op to stencil value
ENDIF

*/


/**
 * Apply the given stencil operator to the array of stencil values.
 * Don't touch stencil[i] if mask[i] is zero.
 * Input:  n - size of stencil array
 *         oper - the stencil buffer operator
 *         face - 0 or 1 for front or back face operation
 *         stencil - array of stencil values
 *         mask - array [n] of flag:  1=apply operator, 0=don't apply operator
 * Output:  stencil - modified values
 */
static void
apply_stencil_op( const GLcontext *ctx, GLenum oper, GLuint face,
                  GLuint n, GLstencil stencil[], const GLubyte mask[] )
{
   const GLstencil ref = ctx->Stencil.Ref[face];
   const GLstencil wrtmask = ctx->Stencil.WriteMask[face];
   const GLstencil invmask = (GLstencil) (~wrtmask);
   const GLstencil stencilMax = (1 << ctx->DrawBuffer->Visual.stencilBits) - 1;
   GLuint i;

   switch (oper) {
      case GL_KEEP:
         /* do nothing */
         break;
      case GL_ZERO:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  stencil[i] = 0;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  stencil[i] = (GLstencil) (stencil[i] & invmask);
	       }
	    }
	 }
	 break;
      case GL_REPLACE:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  stencil[i] = ref;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  GLstencil s = stencil[i];
		  stencil[i] = (GLstencil) ((invmask & s ) | (wrtmask & ref));
	       }
	    }
	 }
	 break;
      case GL_INCR:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  GLstencil s = stencil[i];
		  if (s < stencilMax) {
		     stencil[i] = (GLstencil) (s+1);
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  /* VERIFY logic of adding 1 to a write-masked value */
		  GLstencil s = stencil[i];
		  if (s < stencilMax) {
		     stencil[i] = (GLstencil) ((invmask & s) | (wrtmask & (s+1)));
		  }
	       }
	    }
	 }
	 break;
      case GL_DECR:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  GLstencil s = stencil[i];
		  if (s>0) {
		     stencil[i] = (GLstencil) (s-1);
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  /* VERIFY logic of subtracting 1 to a write-masked value */
		  GLstencil s = stencil[i];
		  if (s>0) {
		     stencil[i] = (GLstencil) ((invmask & s) | (wrtmask & (s-1)));
		  }
	       }
	    }
	 }
	 break;
      case GL_INCR_WRAP_EXT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  stencil[i]++;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil s = stencil[i];
                  stencil[i] = (GLstencil) ((invmask & s) | (wrtmask & (s+1)));
	       }
	    }
	 }
	 break;
      case GL_DECR_WRAP_EXT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  stencil[i]--;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil s = stencil[i];
                  stencil[i] = (GLstencil) ((invmask & s) | (wrtmask & (s-1)));
	       }
	    }
	 }
	 break;
      case GL_INVERT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  GLstencil s = stencil[i];
		  stencil[i] = (GLstencil) ~s;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  GLstencil s = stencil[i];
		  stencil[i] = (GLstencil) ((invmask & s) | (wrtmask & ~s));
	       }
	    }
	 }
	 break;
      default:
         _mesa_problem(ctx, "Bad stencil op in apply_stencil_op");
   }
}




/**
 * Apply stencil test to an array of stencil values (before depth buffering).
 * Input:  face - 0 or 1 for front or back-face polygons
 *         n - number of pixels in the array
 *         stencil - array of [n] stencil values
 *         mask - array [n] of flag:  0=skip the pixel, 1=stencil the pixel
 * Output:  mask - pixels which fail the stencil test will have their
 *                 mask flag set to 0.
 *          stencil - updated stencil values (where the test passed)
 * Return:  GL_FALSE = all pixels failed, GL_TRUE = zero or more pixels passed.
 */
static GLboolean
do_stencil_test( GLcontext *ctx, GLuint face, GLuint n, GLstencil stencil[],
                 GLubyte mask[] )
{
   GLubyte fail[MAX_WIDTH];
   GLboolean allfail = GL_FALSE;
   GLuint i;
   GLstencil r, s;
   const GLuint valueMask = ctx->Stencil.ValueMask[face];

   ASSERT(n <= MAX_WIDTH);

   /*
    * Perform stencil test.  The results of this operation are stored
    * in the fail[] array:
    *   IF fail[i] is non-zero THEN
    *       the stencil fail operator is to be applied
    *   ELSE
    *       the stencil fail operator is not to be applied
    *   ENDIF
    */
   switch (ctx->Stencil.Function[face]) {
      case GL_NEVER:
         /* never pass; always fail */
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       mask[i] = 0;
	       fail[i] = 1;
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 allfail = GL_TRUE;
	 break;
      case GL_LESS:
	 r = (GLstencil) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = (GLstencil) (stencil[i] & valueMask);
	       if (r < s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_LEQUAL:
	 r = (GLstencil) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = (GLstencil) (stencil[i] & valueMask);
	       if (r <= s) {
		  /* pass */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_GREATER:
	 r = (GLstencil) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = (GLstencil) (stencil[i] & valueMask);
	       if (r > s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_GEQUAL:
	 r = (GLstencil) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = (GLstencil) (stencil[i] & valueMask);
	       if (r >= s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_EQUAL:
	 r = (GLstencil) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = (GLstencil) (stencil[i] & valueMask);
	       if (r == s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_NOTEQUAL:
	 r = (GLstencil) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = (GLstencil) (stencil[i] & valueMask);
	       if (r != s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_ALWAYS:
	 /* always pass */
	 for (i=0;i<n;i++) {
	    fail[i] = 0;
	 }
	 break;
      default:
         _mesa_problem(ctx, "Bad stencil func in gl_stencil_span");
         return 0;
   }

   if (ctx->Stencil.FailFunc[face] != GL_KEEP) {
      apply_stencil_op( ctx, ctx->Stencil.FailFunc[face], face, n, stencil, fail );
   }

   return !allfail;
}


/**
 * Compute the zpass/zfail masks by comparing the pre- and post-depth test
 * masks.
 */
static INLINE void
compute_pass_fail_masks(GLuint n, const GLubyte origMask[],
                        const GLubyte newMask[],
                        GLubyte passMask[], GLubyte failMask[])
{
   GLuint i;
   for (i = 0; i < n; i++) {
      ASSERT(newMask[i] == 0 || newMask[i] == 1);
      passMask[i] = origMask[i] & newMask[i];
      failMask[i] = origMask[i] & (newMask[i] ^ 1);
   }
}


/**
 * Apply stencil and depth testing to the span of pixels.
 * Both software and hardware stencil buffers are acceptable.
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in span
 *         z - array [n] of z values
 *         mask - array [n] of flags  (1=test this pixel, 0=skip the pixel)
 * Output:  mask - array [n] of flags (1=stencil and depth test passed)
 * Return: GL_FALSE - all fragments failed the testing
 *         GL_TRUE - one or more fragments passed the testing
 *
 */
static GLboolean
stencil_and_ztest_span(GLcontext *ctx, SWspan *span, GLuint face)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->_StencilBuffer;
   GLstencil stencilRow[MAX_WIDTH];
   GLstencil *stencil;
   const GLuint n = span->end;
   const GLint x = span->x;
   const GLint y = span->y;
   GLubyte *mask = span->array->mask;

   ASSERT((span->arrayMask & SPAN_XY) == 0);
   ASSERT(ctx->Stencil.Enabled);
   ASSERT(n <= MAX_WIDTH);
#ifdef DEBUG
   if (ctx->Depth.Test) {
      ASSERT(span->arrayMask & SPAN_Z);
   }
#endif

   stencil = (GLstencil *) rb->GetPointer(ctx, rb, x, y);
   if (!stencil) {
      rb->GetRow(ctx, rb, n, x, y, stencilRow);
      stencil = stencilRow;
   }

   /*
    * Apply the stencil test to the fragments.
    * failMask[i] is 1 if the stencil test failed.
    */
   if (do_stencil_test( ctx, face, n, stencil, mask ) == GL_FALSE) {
      /* all fragments failed the stencil test, we're done. */
      span->writeAll = GL_FALSE;
      if (!rb->GetPointer(ctx, rb, 0, 0)) {
         /* put updated stencil values into buffer */
         rb->PutRow(ctx, rb, n, x, y, stencil, NULL);
      }
      return GL_FALSE;
   }

   /*
    * Some fragments passed the stencil test, apply depth test to them
    * and apply Zpass and Zfail stencil ops.
    */
   if (ctx->Depth.Test == GL_FALSE) {
      /*
       * No depth buffer, just apply zpass stencil function to active pixels.
       */
      apply_stencil_op( ctx, ctx->Stencil.ZPassFunc[face], face, n, stencil, mask );
   }
   else {
      /*
       * Perform depth buffering, then apply zpass or zfail stencil function.
       */
      GLubyte passMask[MAX_WIDTH], failMask[MAX_WIDTH], origMask[MAX_WIDTH];

      /* save the current mask bits */
      _mesa_memcpy(origMask, mask, n * sizeof(GLubyte));

      /* apply the depth test */
      _swrast_depth_test_span(ctx, span);

      compute_pass_fail_masks(n, origMask, mask, passMask, failMask);

      /* apply the pass and fail operations */
      if (ctx->Stencil.ZFailFunc[face] != GL_KEEP) {
         apply_stencil_op( ctx, ctx->Stencil.ZFailFunc[face], face,
                           n, stencil, failMask );
      }
      if (ctx->Stencil.ZPassFunc[face] != GL_KEEP) {
         apply_stencil_op( ctx, ctx->Stencil.ZPassFunc[face], face,
                           n, stencil, passMask );
      }
   }

   /*
    * Write updated stencil values back into hardware stencil buffer.
    */
   if (!rb->GetPointer(ctx, rb, 0, 0)) {
      rb->PutRow(ctx, rb, n, x, y, stencil, NULL);
   }
   
   span->writeAll = GL_FALSE;
   
   return GL_TRUE;  /* one or more fragments passed both tests */
}



/*
 * Return the address of a stencil buffer value given the window coords:
 */
#define STENCIL_ADDRESS(X, Y)  (stencilStart + (Y) * stride + (X))



/**
 * Apply the given stencil operator for each pixel in the array whose
 * mask flag is set.
 * \note  This is for software stencil buffers only.
 * Input:  n - number of pixels in the span
 *         x, y - array of [n] pixels
 *         operator - the stencil buffer operator
 *         mask - array [n] of flag:  1=apply operator, 0=don't apply operator
 */
static void
apply_stencil_op_to_pixels( GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            GLenum oper, GLuint face, const GLubyte mask[] )
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->_StencilBuffer;
   const GLstencil stencilMax = (1 << fb->Visual.stencilBits) - 1;
   const GLstencil ref = ctx->Stencil.Ref[face];
   const GLstencil wrtmask = ctx->Stencil.WriteMask[face];
   const GLstencil invmask = (GLstencil) (~wrtmask);
   GLuint i;
   GLstencil *stencilStart = (GLubyte *) rb->Data;
   const GLuint stride = rb->Width;

   ASSERT(rb->GetPointer(ctx, rb, 0, 0));
   ASSERT(sizeof(GLstencil) == 1);

   switch (oper) {
      case GL_KEEP:
         /* do nothing */
         break;
      case GL_ZERO:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = 0;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  *sptr = (GLstencil) (invmask & *sptr);
	       }
	    }
	 }
	 break;
      case GL_REPLACE:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = ref;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  *sptr = (GLstencil) ((invmask & *sptr ) | (wrtmask & ref));
	       }
	    }
	 }
	 break;
      case GL_INCR:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  if (*sptr < stencilMax) {
		     *sptr = (GLstencil) (*sptr + 1);
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  if (*sptr < stencilMax) {
		     *sptr = (GLstencil) ((invmask & *sptr) | (wrtmask & (*sptr+1)));
		  }
	       }
	    }
	 }
	 break;
      case GL_DECR:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  if (*sptr>0) {
		     *sptr = (GLstencil) (*sptr - 1);
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  if (*sptr>0) {
		     *sptr = (GLstencil) ((invmask & *sptr) | (wrtmask & (*sptr-1)));
		  }
	       }
	    }
	 }
	 break;
      case GL_INCR_WRAP_EXT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (GLstencil) (*sptr + 1);
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (GLstencil) ((invmask & *sptr) | (wrtmask & (*sptr+1)));
	       }
	    }
	 }
	 break;
      case GL_DECR_WRAP_EXT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (GLstencil) (*sptr - 1);
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (GLstencil) ((invmask & *sptr) | (wrtmask & (*sptr-1)));
	       }
	    }
	 }
	 break;
      case GL_INVERT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (GLstencil) (~*sptr);
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (GLstencil) ((invmask & *sptr) | (wrtmask & ~*sptr));
	       }
	    }
	 }
	 break;
      default:
         _mesa_problem(ctx, "Bad stencilop in apply_stencil_op_to_pixels");
   }
}



/**
 * Apply stencil test to an array of pixels before depth buffering.
 *
 * \note Used for software stencil buffer only.
 * Input:  n - number of pixels in the span
 *         x, y - array of [n] pixels to stencil
 *         mask - array [n] of flag:  0=skip the pixel, 1=stencil the pixel
 * Output:  mask - pixels which fail the stencil test will have their
 *                 mask flag set to 0.
 * \return  GL_FALSE = all pixels failed, GL_TRUE = zero or more pixels passed.
 */
static GLboolean
stencil_test_pixels( GLcontext *ctx, GLuint face, GLuint n,
                     const GLint x[], const GLint y[], GLubyte mask[] )
{
   const struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->_StencilBuffer;
   GLubyte fail[MAX_WIDTH];
   GLstencil r, s;
   GLuint i;
   GLboolean allfail = GL_FALSE;
   const GLuint valueMask = ctx->Stencil.ValueMask[face];
   const GLstencil *stencilStart = (GLstencil *) rb->Data;
   const GLuint stride = rb->Width;

   ASSERT(rb->GetPointer(ctx, rb, 0, 0));
   ASSERT(sizeof(GLstencil) == 1);

   /*
    * Perform stencil test.  The results of this operation are stored
    * in the fail[] array:
    *   IF fail[i] is non-zero THEN
    *       the stencil fail operator is to be applied
    *   ELSE
    *       the stencil fail operator is not to be applied
    *   ENDIF
    */

   switch (ctx->Stencil.Function[face]) {
      case GL_NEVER:
         /* always fail */
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       mask[i] = 0;
	       fail[i] = 1;
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 allfail = GL_TRUE;
	 break;
      case GL_LESS:
	 r = (GLstencil) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               const GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = (GLstencil) (*sptr & valueMask);
	       if (r < s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_LEQUAL:
	 r = (GLstencil) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               const GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = (GLstencil) (*sptr & valueMask);
	       if (r <= s) {
		  /* pass */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_GREATER:
	 r = (GLstencil) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               const GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = (GLstencil) (*sptr & valueMask);
	       if (r > s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_GEQUAL:
	 r = (GLstencil) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               const GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = (GLstencil) (*sptr & valueMask);
	       if (r >= s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_EQUAL:
	 r = (GLstencil) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               const GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = (GLstencil) (*sptr & valueMask);
	       if (r == s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_NOTEQUAL:
	 r = (GLstencil) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               const GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = (GLstencil) (*sptr & valueMask);
	       if (r != s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_ALWAYS:
	 /* always pass */
	 for (i=0;i<n;i++) {
	    fail[i] = 0;
	 }
	 break;
      default:
         _mesa_problem(ctx, "Bad stencil func in gl_stencil_pixels");
         return 0;
   }

   if (ctx->Stencil.FailFunc[face] != GL_KEEP) {
      apply_stencil_op_to_pixels( ctx, n, x, y, ctx->Stencil.FailFunc[face],
                                  face, fail );
   }

   return !allfail;
}




/**
 * Apply stencil and depth testing to an array of pixels.
 * This is used both for software and hardware stencil buffers.
 *
 * The comments in this function are a bit sparse but the code is
 * almost identical to stencil_and_ztest_span(), which is well
 * commented.
 *
 * Input:  n - number of pixels in the array
 *         x, y - array of [n] pixel positions
 *         z - array [n] of z values
 *         mask - array [n] of flags  (1=test this pixel, 0=skip the pixel)
 * Output: mask - array [n] of flags (1=stencil and depth test passed)
 * Return: GL_FALSE - all fragments failed the testing
 *         GL_TRUE - one or more fragments passed the testing
 */
static GLboolean
stencil_and_ztest_pixels( GLcontext *ctx, SWspan *span, GLuint face )
{
   GLubyte passMask[MAX_WIDTH], failMask[MAX_WIDTH], origMask[MAX_WIDTH];
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->_StencilBuffer;
   const GLuint n = span->end;
   const GLint *x = span->array->x;
   const GLint *y = span->array->y;
   GLubyte *mask = span->array->mask;

   ASSERT(span->arrayMask & SPAN_XY);
   ASSERT(ctx->Stencil.Enabled);
   ASSERT(n <= MAX_WIDTH);

   if (!rb->GetPointer(ctx, rb, 0, 0)) {
      /* No direct access */
      GLstencil stencil[MAX_WIDTH];

      ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
      _swrast_get_values(ctx, rb, n, x, y, stencil, sizeof(GLubyte));

      _mesa_memcpy(origMask, mask, n * sizeof(GLubyte));          

      (void) do_stencil_test(ctx, face, n, stencil, mask);

      if (ctx->Depth.Test == GL_FALSE) {
         apply_stencil_op(ctx, ctx->Stencil.ZPassFunc[face], face,
                          n, stencil, mask);
      }
      else {
         GLubyte tmpMask[MAX_WIDTH]; 
         _mesa_memcpy(tmpMask, mask, n * sizeof(GLubyte));

         _swrast_depth_test_span(ctx, span);

         compute_pass_fail_masks(n, tmpMask, mask, passMask, failMask);

         if (ctx->Stencil.ZFailFunc[face] != GL_KEEP) {
            apply_stencil_op(ctx, ctx->Stencil.ZFailFunc[face], face,
                             n, stencil, failMask);
         }
         if (ctx->Stencil.ZPassFunc[face] != GL_KEEP) {
            apply_stencil_op(ctx, ctx->Stencil.ZPassFunc[face], face,
                             n, stencil, passMask);
         }
      }

      /* Write updated stencil values into hardware stencil buffer */
      rb->PutValues(ctx, rb, n, x, y, stencil, origMask);

      return GL_TRUE;
   }
   else {
      /* Direct access to stencil buffer */

      if (stencil_test_pixels(ctx, face, n, x, y, mask) == GL_FALSE) {
         /* all fragments failed the stencil test, we're done. */
         return GL_FALSE;
      }

      if (ctx->Depth.Test==GL_FALSE) {
         apply_stencil_op_to_pixels(ctx, n, x, y,
                                    ctx->Stencil.ZPassFunc[face], face, mask);
      }
      else {
         _mesa_memcpy(origMask, mask, n * sizeof(GLubyte));

         _swrast_depth_test_span(ctx, span);

         compute_pass_fail_masks(n, origMask, mask, passMask, failMask);

         if (ctx->Stencil.ZFailFunc[face] != GL_KEEP) {
            apply_stencil_op_to_pixels(ctx, n, x, y,
                                       ctx->Stencil.ZFailFunc[face],
                                       face, failMask);
         }
         if (ctx->Stencil.ZPassFunc[face] != GL_KEEP) {
            apply_stencil_op_to_pixels(ctx, n, x, y,
                                       ctx->Stencil.ZPassFunc[face],
                                       face, passMask);
         }
      }

      return GL_TRUE;  /* one or more fragments passed both tests */
   }
}


/**
 * /return GL_TRUE = one or more fragments passed,
 * GL_FALSE = all fragments failed.
 */
GLboolean
_swrast_stencil_and_ztest_span(GLcontext *ctx, SWspan *span)
{
   if (span->arrayMask & SPAN_XY)
      return stencil_and_ztest_pixels(ctx, span, span->facing);
   else
      return stencil_and_ztest_span(ctx, span, span->facing);
}


#if 0
GLuint
clip_span(GLuint bufferWidth, GLuint bufferHeight,
          GLint x, GLint y, GLuint *count)
{
   GLuint n = *count;
   GLuint skipPixels = 0;

   if (y < 0 || y >= bufferHeight || x + n <= 0 || x >= bufferWidth) {
      /* totally out of bounds */
      n = 0;
   }
   else {
      /* left clip */
      if (x < 0) {
         skipPixels = -x;
         x = 0;
         n -= skipPixels;
      }
      /* right clip */
      if (x + n > bufferWidth) {
         GLint dx = x + n - bufferWidth;
         n -= dx;
      }
   }

   *count = n;

   return skipPixels;
}
#endif


/**
 * Return a span of stencil values from the stencil buffer.
 * Used for glRead/CopyPixels
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  stencil - the array of stencil values
 */
void
_swrast_read_stencil_span(GLcontext *ctx, struct gl_renderbuffer *rb,
                          GLint n, GLint x, GLint y, GLstencil stencil[])
{
   if (y < 0 || y >= (GLint) rb->Height ||
       x + n <= 0 || x >= (GLint) rb->Width) {
      /* span is completely outside framebuffer */
      return; /* undefined values OK */
   }

   if (x < 0) {
      GLint dx = -x;
      x = 0;
      n -= dx;
      stencil += dx;
   }
   if (x + n > (GLint) rb->Width) {
      GLint dx = x + n - rb->Width;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   rb->GetRow(ctx, rb, n, x, y, stencil);
}



/**
 * Write a span of stencil values to the stencil buffer.  This function
 * applies the stencil write mask when needed.
 * Used for glDraw/CopyPixels
 * Input:  n - how many pixels
 *         x, y - location of first pixel
 *         stencil - the array of stencil values
 */
void
_swrast_write_stencil_span(GLcontext *ctx, GLint n, GLint x, GLint y,
                           const GLstencil stencil[] )
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->_StencilBuffer;
   const GLuint stencilMax = (1 << fb->Visual.stencilBits) - 1;
   const GLuint stencilMask = ctx->Stencil.WriteMask[0];

   if (y < 0 || y >= (GLint) rb->Height ||
       x + n <= 0 || x >= (GLint) rb->Width) {
      /* span is completely outside framebuffer */
      return; /* undefined values OK */
   }
   if (x < 0) {
      GLint dx = -x;
      x = 0;
      n -= dx;
      stencil += dx;
   }
   if (x + n > (GLint) rb->Width) {
      GLint dx = x + n - rb->Width;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   if ((stencilMask & stencilMax) != stencilMax) {
      /* need to apply writemask */
      GLstencil destVals[MAX_WIDTH], newVals[MAX_WIDTH];
      GLint i;
      rb->GetRow(ctx, rb, n, x, y, destVals);
      for (i = 0; i < n; i++) {
         newVals[i]
            = (stencil[i] & stencilMask) | (destVals[i] & ~stencilMask);
      }
      rb->PutRow(ctx, rb, n, x, y, newVals, NULL);
   }
   else {
      rb->PutRow(ctx, rb, n, x, y, stencil, NULL);
   }
}



/**
 * Clear the stencil buffer.
 */
void
_swrast_clear_stencil_buffer( GLcontext *ctx, struct gl_renderbuffer *rb )
{
   const GLubyte stencilBits = ctx->DrawBuffer->Visual.stencilBits;
   const GLuint mask = ctx->Stencil.WriteMask[0];
   const GLuint invMask = ~mask;
   const GLuint clearVal = (ctx->Stencil.Clear & mask);
   const GLuint stencilMax = (1 << stencilBits) - 1;
   GLint x, y, width, height;

   if (!rb || mask == 0)
      return;

   ASSERT(rb->DataType == GL_UNSIGNED_BYTE ||
          rb->DataType == GL_UNSIGNED_SHORT);

   ASSERT(rb->_BaseFormat == GL_STENCIL_INDEX);

   /* compute region to clear */
   x = ctx->DrawBuffer->_Xmin;
   y = ctx->DrawBuffer->_Ymin;
   width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;

   if (rb->GetPointer(ctx, rb, 0, 0)) {
      /* Direct buffer access */
      if ((mask & stencilMax) != stencilMax) {
         /* need to mask the clear */
         if (rb->DataType == GL_UNSIGNED_BYTE) {
            GLint i, j;
            for (i = 0; i < height; i++) {
               GLubyte *stencil = (GLubyte*) rb->GetPointer(ctx, rb, x, y + i);
               for (j = 0; j < width; j++) {
                  stencil[j] = (stencil[j] & invMask) | clearVal;
               }
            }
         }
         else {
            GLint i, j;
            for (i = 0; i < height; i++) {
               GLushort *stencil = (GLushort*) rb->GetPointer(ctx, rb, x, y + i);
               for (j = 0; j < width; j++) {
                  stencil[j] = (stencil[j] & invMask) | clearVal;
               }
            }
         }
      }
      else {
         /* no bit masking */
         if (width == (GLint) rb->Width && rb->DataType == GL_UNSIGNED_BYTE) {
            /* optimized case */
            /* Note: bottom-to-top raster assumed! */
            GLubyte *stencil = (GLubyte *) rb->GetPointer(ctx, rb, x, y);
            GLuint len = width * height * sizeof(GLubyte);
            _mesa_memset(stencil, clearVal, len);
         }
         else {
            /* general case */
            GLint i;
            for (i = 0; i < height; i++) {
               GLvoid *stencil = rb->GetPointer(ctx, rb, x, y + i);
               if (rb->DataType == GL_UNSIGNED_BYTE) {
                  _mesa_memset(stencil, clearVal, width);
               }
               else {
                  _mesa_memset16((short unsigned int*) stencil, clearVal, width);
               }
            }
         }
      }
   }
   else {
      /* no direct access */
      if ((mask & stencilMax) != stencilMax) {
         /* need to mask the clear */
         if (rb->DataType == GL_UNSIGNED_BYTE) {
            GLint i, j;
            for (i = 0; i < height; i++) {
               GLubyte stencil[MAX_WIDTH];
               rb->GetRow(ctx, rb, width, x, y + i, stencil);
               for (j = 0; j < width; j++) {
                  stencil[j] = (stencil[j] & invMask) | clearVal;
               }
               rb->PutRow(ctx, rb, width, x, y + i, stencil, NULL);
            }
         }
         else {
            GLint i, j;
            for (i = 0; i < height; i++) {
               GLushort stencil[MAX_WIDTH];
               rb->GetRow(ctx, rb, width, x, y + i, stencil);
               for (j = 0; j < width; j++) {
                  stencil[j] = (stencil[j] & invMask) | clearVal;
               }
               rb->PutRow(ctx, rb, width, x, y + i, stencil, NULL);
            }
         }
      }
      else {
         /* no bit masking */
         const GLubyte clear8 = (GLubyte) clearVal;
         const GLushort clear16 = (GLushort) clearVal;
         const void *clear;
         GLint i;
         if (rb->DataType == GL_UNSIGNED_BYTE) {
            clear = &clear8;
         }
         else {
            clear = &clear16;
         }
         for (i = 0; i < height; i++) {
            rb->PutMonoRow(ctx, rb, width, x, y + i, clear, NULL);
         }
      }
   }
}
