/*
 * Mesa 3-D graphics library
 * Version:  6.1
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
#include "context.h"
#include "macros.h"
#include "imports.h"

#include "s_context.h"
#include "s_depth.h"
#include "s_stencil.h"



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


/*
 * Return the address of a stencil buffer value given the window coords:
 */
#define STENCIL_ADDRESS(X,Y)  \
       (ctx->DrawBuffer->Stencil + ctx->DrawBuffer->Width * (Y) + (X))



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
		  if (s < STENCIL_MAX) {
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
		  if (s < STENCIL_MAX) {
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
stencil_and_ztest_span(GLcontext *ctx, struct sw_span *span, GLuint face)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
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
   
   /* Get initial stencil values */
   if (swrast->Driver.WriteStencilSpan) {
      /* Get stencil values from the hardware stencil buffer */
      ASSERT(swrast->Driver.ReadStencilSpan);
      (*swrast->Driver.ReadStencilSpan)(ctx, n, x, y, stencilRow);
      stencil = stencilRow;
   }
   else {
      /* Get pointer into software stencil buffer */
      stencil = STENCIL_ADDRESS(x, y);
   }
   
   /*
    * Apply the stencil test to the fragments.
    * failMask[i] is 1 if the stencil test failed.
    */
   if (do_stencil_test( ctx, face, n, stencil, mask ) == GL_FALSE) {
      /* all fragments failed the stencil test, we're done. */
      span->writeAll = GL_FALSE;
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
      GLubyte passmask[MAX_WIDTH], failmask[MAX_WIDTH], oldmask[MAX_WIDTH];
      GLuint i;

      /* save the current mask bits */
      MEMCPY(oldmask, mask, n * sizeof(GLubyte));

      /* apply the depth test */
      _swrast_depth_test_span(ctx, span);

      /* Set the stencil pass/fail flags according to result of depth testing.
       * if oldmask[i] == 0 then
       *    Don't touch the stencil value
       * else if oldmask[i] and newmask[i] then
       *    Depth test passed
       * else
       *    assert(oldmask[i] && !newmask[i])
       *    Depth test failed
       * endif
       */
      for (i=0;i<n;i++) {
         ASSERT(mask[i] == 0 || mask[i] == 1);
         passmask[i] = oldmask[i] & mask[i];
         failmask[i] = oldmask[i] & (mask[i] ^ 1);
      }

      /* apply the pass and fail operations */
      if (ctx->Stencil.ZFailFunc[face] != GL_KEEP) {
         apply_stencil_op( ctx, ctx->Stencil.ZFailFunc[face], face,
                           n, stencil, failmask );
      }
      if (ctx->Stencil.ZPassFunc[face] != GL_KEEP) {
         apply_stencil_op( ctx, ctx->Stencil.ZPassFunc[face], face,
                           n, stencil, passmask );
      }
   }

   /*
    * Write updated stencil values back into hardware stencil buffer.
    */
   if (swrast->Driver.WriteStencilSpan) {
      ASSERT(stencil == stencilRow);
      (swrast->Driver.WriteStencilSpan)(ctx, n, x, y, stencil, mask );
   }
   
   span->writeAll = GL_FALSE;
   
   return GL_TRUE;  /* one or more fragments passed both tests */
}




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
apply_stencil_op_to_pixels( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            GLenum oper, GLuint face, const GLubyte mask[] )
{
   const GLstencil ref = ctx->Stencil.Ref[face];
   const GLstencil wrtmask = ctx->Stencil.WriteMask[face];
   const GLstencil invmask = (GLstencil) (~wrtmask);
   GLuint i;

   ASSERT(!SWRAST_CONTEXT(ctx)->Driver.WriteStencilSpan);  /* software stencil buffer only! */

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
		  if (*sptr < STENCIL_MAX) {
		     *sptr = (GLstencil) (*sptr + 1);
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  if (*sptr < STENCIL_MAX) {
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
   GLubyte fail[MAX_WIDTH];
   GLstencil r, s;
   GLuint i;
   GLboolean allfail = GL_FALSE;
   const GLuint valueMask = ctx->Stencil.ValueMask[face];

  /* software stencil buffer only! */
   ASSERT(ctx->DrawBuffer->UseSoftwareStencilBuffer);
   ASSERT(!SWRAST_CONTEXT(ctx)->Driver.ReadStencilSpan);
   ASSERT(!SWRAST_CONTEXT(ctx)->Driver.WriteStencilSpan);

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
               GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
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
               GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
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
               GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
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
               GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
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
               GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
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
               GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
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
stencil_and_ztest_pixels( GLcontext *ctx, struct sw_span *span, GLuint face )
{
   const GLuint n = span->end;
   const GLint *x = span->array->x;
   const GLint *y = span->array->y;
   GLubyte *mask = span->array->mask;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   ASSERT(span->arrayMask & SPAN_XY);
   ASSERT(ctx->Stencil.Enabled);
   ASSERT(n <= MAX_WIDTH);

   if (swrast->Driver.WriteStencilPixels) {
      /*** Hardware stencil buffer ***/
      GLstencil stencil[MAX_WIDTH];
      GLubyte origMask[MAX_WIDTH];

      ASSERT(!ctx->DrawBuffer->UseSoftwareStencilBuffer);
      ASSERT(swrast->Driver.ReadStencilPixels);
      (*swrast->Driver.ReadStencilPixels)(ctx, n, x, y, stencil);

      MEMCPY(origMask, mask, n * sizeof(GLubyte));

      (void) do_stencil_test(ctx, face, n, stencil, mask);

      if (ctx->Depth.Test == GL_FALSE) {
         apply_stencil_op(ctx, ctx->Stencil.ZPassFunc[face], face,
                          n, stencil, mask);
      }
      else {
         _swrast_depth_test_span(ctx, span);

         if (ctx->Stencil.ZFailFunc[face] != GL_KEEP) {
            GLubyte failmask[MAX_WIDTH];
            GLuint i;
            for (i = 0; i < n; i++) {
               ASSERT(mask[i] == 0 || mask[i] == 1);
               failmask[i] = origMask[i] & (mask[i] ^ 1);
            }
            apply_stencil_op(ctx, ctx->Stencil.ZFailFunc[face], face,
                             n, stencil, failmask);
         }
         if (ctx->Stencil.ZPassFunc[face] != GL_KEEP) {
            GLubyte passmask[MAX_WIDTH];
            GLuint i;
            for (i = 0; i < n; i++) {
               ASSERT(mask[i] == 0 || mask[i] == 1);
               passmask[i] = origMask[i] & mask[i];
            }
            apply_stencil_op(ctx, ctx->Stencil.ZPassFunc[face], face,
                             n, stencil, passmask);
         }
      }

      /* Write updated stencil values into hardware stencil buffer */
      (swrast->Driver.WriteStencilPixels)(ctx, n, x, y, stencil, origMask);

      return GL_TRUE;
   }
   else {
      /*** Software stencil buffer ***/

      ASSERT(ctx->DrawBuffer->UseSoftwareStencilBuffer);

      if (stencil_test_pixels(ctx, face, n, x, y, mask) == GL_FALSE) {
         /* all fragments failed the stencil test, we're done. */
         return GL_FALSE;
      }

      if (ctx->Depth.Test==GL_FALSE) {
         apply_stencil_op_to_pixels(ctx, n, x, y,
                                    ctx->Stencil.ZPassFunc[face], face, mask);
      }
      else {
         GLubyte passmask[MAX_WIDTH], failmask[MAX_WIDTH], oldmask[MAX_WIDTH];
         GLuint i;

         MEMCPY(oldmask, mask, n * sizeof(GLubyte));

         _swrast_depth_test_span(ctx, span);

         for (i=0;i<n;i++) {
            ASSERT(mask[i] == 0 || mask[i] == 1);
            passmask[i] = oldmask[i] & mask[i];
            failmask[i] = oldmask[i] & (mask[i] ^ 1);
         }

         if (ctx->Stencil.ZFailFunc[face] != GL_KEEP) {
            apply_stencil_op_to_pixels(ctx, n, x, y,
                                       ctx->Stencil.ZFailFunc[face],
                                       face, failmask);
         }
         if (ctx->Stencil.ZPassFunc[face] != GL_KEEP) {
            apply_stencil_op_to_pixels(ctx, n, x, y,
                                       ctx->Stencil.ZPassFunc[face],
                                       face, passmask);
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
_swrast_stencil_and_ztest_span(GLcontext *ctx, struct sw_span *span)
{
   /* span->facing can only be non-zero if using two-sided stencil */
   ASSERT(ctx->Stencil.TestTwoSide || span->facing == 0);
   if (span->arrayMask & SPAN_XY)
      return stencil_and_ztest_pixels(ctx, span, span->facing);
   else
      return stencil_and_ztest_span(ctx, span, span->facing);
}


/**
 * Return a span of stencil values from the stencil buffer.
 * Used for glRead/CopyPixels
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  stencil - the array of stencil values
 */
void
_swrast_read_stencil_span( GLcontext *ctx,
                         GLint n, GLint x, GLint y, GLstencil stencil[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLint bufWidth = (GLint) ctx->DrawBuffer->Width;
   const GLint bufHeight = (GLint) ctx->DrawBuffer->Height;

   if (y < 0 || y >= bufHeight || x + n <= 0 || x >= bufWidth) {
      /* span is completely outside framebuffer */
      return; /* undefined values OK */
   }

   if (x < 0) {
      GLint dx = -x;
      x = 0;
      n -= dx;
      stencil += dx;
   }
   if (x + n > bufWidth) {
      GLint dx = x + n - bufWidth;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }


   ASSERT(n >= 0);
   if (swrast->Driver.ReadStencilSpan) {
      (*swrast->Driver.ReadStencilSpan)( ctx, (GLuint) n, x, y, stencil );
   }
   else if (ctx->DrawBuffer->Stencil) {
      const GLstencil *s = STENCIL_ADDRESS( x, y );
#if STENCIL_BITS == 8
      MEMCPY( stencil, s, n * sizeof(GLstencil) );
#else
      GLuint i;
      for (i=0;i<n;i++)
         stencil[i] = s[i];
#endif
   }
}



/**
 * Write a span of stencil values to the stencil buffer.
 * Used for glDraw/CopyPixels
 * Input:  n - how many pixels
 *         x, y - location of first pixel
 *         stencil - the array of stencil values
 */
void
_swrast_write_stencil_span( GLcontext *ctx, GLint n, GLint x, GLint y,
                          const GLstencil stencil[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLstencil *ssrc = stencil;
   const GLint bufWidth = (GLint) ctx->DrawBuffer->Width;
   const GLint bufHeight = (GLint) ctx->DrawBuffer->Height;

   if (y < 0 || y >= bufHeight || x + n <= 0 || x >= bufWidth) {
      /* span is completely outside framebuffer */
      return; /* undefined values OK */
   }

   if (x < 0) {
      GLint dx = -x;
      x = 0;
      n -= dx;
      ssrc += dx;
   }
   if (x + n > bufWidth) {
      GLint dx = x + n - bufWidth;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   if (swrast->Driver.WriteStencilSpan) {
      (*swrast->Driver.WriteStencilSpan)( ctx, n, x, y, ssrc, NULL );
   }
   else if (ctx->DrawBuffer->Stencil) {
      GLstencil *s = STENCIL_ADDRESS( x, y );
#if STENCIL_BITS == 8
      MEMCPY( s, ssrc, n * sizeof(GLstencil) );
#else
      GLuint i;
      for (i=0;i<n;i++)
         s[i] = ssrc[i];
#endif
   }
}



/**
 * Allocate a new stencil buffer.  If there's an old one it will be
 * deallocated first.  The new stencil buffer will be uninitialized.
 */
void
_swrast_alloc_stencil_buffer( GLframebuffer *buffer )
{
   /* deallocate current stencil buffer if present */
   if (buffer->Stencil) {
      MESA_PBUFFER_FREE(buffer->Stencil);
      buffer->Stencil = NULL;
   }

   /* allocate new stencil buffer */
   buffer->Stencil = (GLstencil *)
      MESA_PBUFFER_ALLOC(buffer->Width * buffer->Height * sizeof(GLstencil));
   if (!buffer->Stencil) {
      /* out of memory */
      _mesa_error( NULL, GL_OUT_OF_MEMORY, "_swrast_alloc_stencil_buffer" );
   }
}



/**
 * Clear the software (malloc'd) stencil buffer.
 */
static void
clear_software_stencil_buffer( GLcontext *ctx )
{
   if (ctx->Visual.stencilBits==0 || !ctx->DrawBuffer->Stencil) {
      /* no stencil buffer */
      return;
   }

   if (ctx->Scissor.Enabled) {
      /* clear scissor region only */
      const GLint width = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
      if (ctx->Stencil.WriteMask[0] != STENCIL_MAX) {
         /* must apply mask to the clear */
         GLint y;
         for (y = ctx->DrawBuffer->_Ymin; y < ctx->DrawBuffer->_Ymax; y++) {
            const GLstencil mask = ctx->Stencil.WriteMask[0];
            const GLstencil invMask = ~mask;
            const GLstencil clearVal = (ctx->Stencil.Clear & mask);
            GLstencil *stencil = STENCIL_ADDRESS( ctx->DrawBuffer->_Xmin, y );
            GLint i;
            for (i = 0; i < width; i++) {
               stencil[i] = (stencil[i] & invMask) | clearVal;
            }
         }
      }
      else {
         /* no masking */
         GLint y;
         for (y = ctx->DrawBuffer->_Ymin; y < ctx->DrawBuffer->_Ymax; y++) {
            GLstencil *stencil = STENCIL_ADDRESS( ctx->DrawBuffer->_Xmin, y );
#if STENCIL_BITS==8
            MEMSET( stencil, ctx->Stencil.Clear, width * sizeof(GLstencil) );
#else
            GLint i;
            for (i = 0; i < width; i++)
               stencil[i] = ctx->Stencil.Clear;
#endif
         }
      }
   }
   else {
      /* clear whole stencil buffer */
      if (ctx->Stencil.WriteMask[0] != STENCIL_MAX) {
         /* must apply mask to the clear */
         const GLuint n = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height;
         GLstencil *stencil = ctx->DrawBuffer->Stencil;
         const GLstencil mask = ctx->Stencil.WriteMask[0];
         const GLstencil invMask = ~mask;
         const GLstencil clearVal = (ctx->Stencil.Clear & mask);
         GLuint i;
         for (i = 0; i < n; i++) {
            stencil[i] = (stencil[i] & invMask) | clearVal;
         }
      }
      else {
         /* clear whole buffer without masking */
         const GLuint n = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height;
         GLstencil *stencil = ctx->DrawBuffer->Stencil;

#if STENCIL_BITS==8
         MEMSET(stencil, ctx->Stencil.Clear, n * sizeof(GLstencil) );
#else
         GLuint i;
         for (i = 0; i < n; i++) {
            stencil[i] = ctx->Stencil.Clear;
         }
#endif
      }
   }
}



/**
 * Clear the hardware (in graphics card) stencil buffer.
 * This is done with the Driver.WriteStencilSpan() and Driver.ReadStencilSpan()
 * functions.
 * Actually, if there is a hardware stencil buffer it really should have
 * been cleared in Driver.Clear()!  However, if the hardware does not
 * support scissored clears or masked clears (i.e. glStencilMask) then
 * we have to use the span-based functions.
 */
static void
clear_hardware_stencil_buffer( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   ASSERT(swrast->Driver.WriteStencilSpan);
   ASSERT(swrast->Driver.ReadStencilSpan);

   if (ctx->Scissor.Enabled) {
      /* clear scissor region only */
      const GLint x = ctx->DrawBuffer->_Xmin;
      const GLint width = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
      if (ctx->Stencil.WriteMask[0] != STENCIL_MAX) {
         /* must apply mask to the clear */
         GLint y;
         for (y = ctx->DrawBuffer->_Ymin; y < ctx->DrawBuffer->_Ymax; y++) {
            const GLstencil mask = ctx->Stencil.WriteMask[0];
            const GLstencil invMask = ~mask;
            const GLstencil clearVal = (ctx->Stencil.Clear & mask);
            GLstencil stencil[MAX_WIDTH];
            GLint i;
            (*swrast->Driver.ReadStencilSpan)(ctx, width, x, y, stencil);
            for (i = 0; i < width; i++) {
               stencil[i] = (stencil[i] & invMask) | clearVal;
            }
            (*swrast->Driver.WriteStencilSpan)(ctx, width, x, y, stencil, NULL);
         }
      }
      else {
         /* no masking */
         GLstencil stencil[MAX_WIDTH];
         GLint y, i;
         for (i = 0; i < width; i++) {
            stencil[i] = ctx->Stencil.Clear;
         }
         for (y = ctx->DrawBuffer->_Ymin; y < ctx->DrawBuffer->_Ymax; y++) {
            (*swrast->Driver.WriteStencilSpan)(ctx, width, x, y, stencil, NULL);
         }
      }
   }
   else {
      /* clear whole stencil buffer */
      if (ctx->Stencil.WriteMask[0] != STENCIL_MAX) {
         /* must apply mask to the clear */
         const GLstencil mask = ctx->Stencil.WriteMask[0];
         const GLstencil invMask = ~mask;
         const GLstencil clearVal = (ctx->Stencil.Clear & mask);
         const GLint width = ctx->DrawBuffer->Width;
         const GLint height = ctx->DrawBuffer->Height;
         const GLint x = ctx->DrawBuffer->_Xmin;
         GLint y;
         for (y = 0; y < height; y++) {
            GLstencil stencil[MAX_WIDTH];
            GLint i;
            (*swrast->Driver.ReadStencilSpan)(ctx, width, x, y, stencil);
            for (i = 0; i < width; i++) {
               stencil[i] = (stencil[i] & invMask) | clearVal;
            }
            (*swrast->Driver.WriteStencilSpan)(ctx, width, x, y, stencil, NULL);
         }
      }
      else {
         /* clear whole buffer without masking */
         const GLint width = ctx->DrawBuffer->Width;
         const GLint height = ctx->DrawBuffer->Height;
         const GLint x = ctx->DrawBuffer->_Xmin;
         GLstencil stencil[MAX_WIDTH];
         GLint y, i;
         for (i = 0; i < width; i++) {
            stencil[i] = ctx->Stencil.Clear;
         }
         for (y = 0; y < height; y++) {
            (*swrast->Driver.WriteStencilSpan)(ctx, width, x, y, stencil, NULL);
         }
      }
   }
}



/**
 * Clear the stencil buffer (hardware or software).
 */
void
_swrast_clear_stencil_buffer( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   if (swrast->Driver.WriteStencilSpan) {
      ASSERT(swrast->Driver.ReadStencilSpan);
      clear_hardware_stencil_buffer(ctx);
   }
   else {
      clear_software_stencil_buffer(ctx);
   }
}
