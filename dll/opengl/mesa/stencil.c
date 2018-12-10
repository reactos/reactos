/* $Id: stencil.c,v 1.8 1998/01/01 00:52:11 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.6
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: stencil.c,v $
 * Revision 1.8  1998/01/01 00:52:11  brianp
 * added some tests to prevent crashing if Driver.DepthTestPixels not defined
 *
 * Revision 1.7  1997/07/24 01:21:56  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.6  1997/05/28 03:26:29  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.5  1997/04/29 01:26:37  brianp
 * fixed a few return statements which were missing values
 *
 * Revision 1.4  1997/04/20 20:29:11  brianp
 * replaced abort() with gl_problem()
 *
 * Revision 1.3  1997/02/27 19:58:35  brianp
 * don't try to clear stencil buffer if there isn't one
 *
 * Revision 1.2  1996/09/15 14:18:55  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdlib.h>
#include <string.h>
#include "context.h"
#include "dlist.h"
#include "macros.h"
#include "pb.h"
#include "stencil.h"
#include "types.h"
#endif


/*
 * Return the address of a stencil buffer value given the window coords:
 */
#define STENCIL_ADDRESS(X,Y)  (ctx->Buffer->Stencil + ctx->Buffer->Width * (Y) + (X))


void gl_ClearStencil( GLcontext *ctx, GLint s )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glClearStencil" );
      return;
   }
   ctx->Stencil.Clear = (GLstencil) s;
}



void gl_StencilFunc( GLcontext *ctx, GLenum func, GLint ref, GLuint mask )
{
   GLint maxref;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glStencilFunc" );
      return;
   }

   switch (func) {
      case GL_NEVER:
      case GL_LESS:
      case GL_LEQUAL:
      case GL_GREATER:
      case GL_GEQUAL:
      case GL_EQUAL:
      case GL_NOTEQUAL:
      case GL_ALWAYS:
         ctx->Stencil.Function = func;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glStencilFunc" );
         return;
   }

   maxref = (1 << STENCIL_BITS) - 1;
   ctx->Stencil.Ref = CLAMP( ref, 0, maxref );
   ctx->Stencil.ValueMask = mask;
}



void gl_StencilMask( GLcontext *ctx, GLuint mask )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glStencilMask" );
      return;
   }
   ctx->Stencil.WriteMask = (GLstencil) mask;
}



void gl_StencilOp( GLcontext *ctx, GLenum fail, GLenum zfail, GLenum zpass )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glStencilOp" );
      return;
   }
   switch (fail) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         ctx->Stencil.FailFunc = fail;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glStencilOp" );
         return;
   }
   switch (zfail) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         ctx->Stencil.ZFailFunc = zfail;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glStencilOp" );
         return;
   }
   switch (zpass) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         ctx->Stencil.ZPassFunc = zpass;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glStencilOp" );
         return;
   }
}



/* Stencil Logic:

IF stencil test fails THEN
   Don't write the pixel (RGBA,Z)
   Execute FailOp
ELSE
   Write the pixel
ENDIF

Perform Depth Test

IF depth test passes OR no depth buffer THEN
   Execute ZPass
   Write the pixel
ELSE
   Execute ZFail
ENDIF

*/




/*
 * Apply the given stencil operator for each pixel in the span whose
 * mask flag is set.
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in the span
 *         oper - the stencil buffer operator
 *         mask - array [n] of flag:  1=apply operator, 0=don't apply operator
 */
static void apply_stencil_op_to_span( GLcontext *ctx,
                                      GLuint n, GLint x, GLint y,
				      GLenum oper, GLubyte mask[] )
{
   GLint i;
   GLstencil s, ref;
   GLstencil wrtmask, invmask;
   GLstencil *stencil;

   wrtmask = ctx->Stencil.WriteMask;
   invmask = ~ctx->Stencil.WriteMask;
   ref = ctx->Stencil.Ref;
   stencil = STENCIL_ADDRESS( x, y );

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
		  stencil[i] = stencil[i] & invmask;
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
		  s = stencil[i];
		  stencil[i] = (invmask & s ) | (wrtmask & ref);
	       }
	    }
	 }
	 break;
      case GL_INCR:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  s = stencil[i];
		  if (s<0xff) {
		     stencil[i] = s+1;
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  /* VERIFY logic of adding 1 to a write-masked value */
		  s = stencil[i];
		  if (s<0xff) {
		     stencil[i] = (invmask & s) | (wrtmask & (s+1));
		  }
	       }
	    }
	 }
	 break;
      case GL_DECR:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  s = stencil[i];
		  if (s>0) {
		     stencil[i] = s-1;
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  /* VERIFY logic of subtracting 1 to a write-masked value */
		  s = stencil[i];
		  if (s>0) {
		     stencil[i] = (invmask & s) | (wrtmask & (s-1));
		  }
	       }
	    }
	 }
	 break;
      case GL_INVERT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  s = stencil[i];
		  stencil[i] = ~s;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  s = stencil[i];
		  stencil[i] = (invmask & s) | (wrtmask & ~s);
	       }
	    }
	 }
	 break;
      default:
         gl_problem(ctx, "Bad stencilop in apply_stencil_op_to_span");
   }
}




/*
 * Apply stencil test to a span of pixels before depth buffering.
 * Input:  n - number of pixels in the span
 *         x, y - coordinate of left-most pixel in the span
 *         mask - array [n] of flag:  0=skip the pixel, 1=stencil the pixel
 * Output:  mask - pixels which fail the stencil test will have their
 *                 mask flag set to 0.
 * Return:  0 = all pixels failed, 1 = zero or more pixels passed.
 */
GLint gl_stencil_span( GLcontext *ctx,
                       GLuint n, GLint x, GLint y, GLubyte mask[] )
{
   GLubyte fail[MAX_WIDTH];
   GLint allfail = 0;
   GLuint i;
   GLstencil r, s;
   GLstencil *stencil;

   stencil = STENCIL_ADDRESS( x, y );

   /*
    * Perform stencil test.  The results of this operation are stored
    * in the fail[] array:
    *   IF fail[i] is non-zero THEN
    *       the stencil fail operator is to be applied
    *   ELSE
    *       the stencil fail operator is not to be applied
    *   ENDIF
    */
   switch (ctx->Stencil.Function) {
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
	 allfail = 1;
	 break;
      case GL_LESS:
	 r = ctx->Stencil.Ref & ctx->Stencil.ValueMask;
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = stencil[i] & ctx->Stencil.ValueMask;
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
	 r = ctx->Stencil.Ref & ctx->Stencil.ValueMask;
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = stencil[i] & ctx->Stencil.ValueMask;
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
	 r = ctx->Stencil.Ref & ctx->Stencil.ValueMask;
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = stencil[i] & ctx->Stencil.ValueMask;
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
	 r = ctx->Stencil.Ref & ctx->Stencil.ValueMask;
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = stencil[i] & ctx->Stencil.ValueMask;
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
	 r = ctx->Stencil.Ref & ctx->Stencil.ValueMask;
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = stencil[i] & ctx->Stencil.ValueMask;
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
	 r = ctx->Stencil.Ref & ctx->Stencil.ValueMask;
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = stencil[i] & ctx->Stencil.ValueMask;
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
         gl_problem(ctx, "Bad stencil func in gl_stencil_span");
         return 0;
   }

   apply_stencil_op_to_span( ctx, n, x, y, ctx->Stencil.FailFunc, fail );

   return (allfail) ? 0 : 1;
}




/*
 * Apply the combination depth-buffer/stencil operator to a span of pixels.
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in span
 *         z - array [n] of z values
 * Input:  mask - array [n] of flags  (1=test this pixel, 0=skip the pixel)
 * Output:  mask - array [n] of flags (1=depth test passed, 0=failed) 
 */
void gl_depth_stencil_span( GLcontext *ctx,
                            GLuint n, GLint x, GLint y, const GLdepth z[],
			    GLubyte mask[] )
{
   if (ctx->Depth.Test==GL_FALSE) {
      /*
       * No depth buffer, just apply zpass stencil function to active pixels.
       */
      apply_stencil_op_to_span( ctx, n, x, y, ctx->Stencil.ZPassFunc, mask );
   }
   else {
      /*
       * Perform depth buffering, then apply zpass or zfail stencil function.
       */
      GLubyte passmask[MAX_WIDTH], failmask[MAX_WIDTH], oldmask[MAX_WIDTH];
      GLuint i;

      /* init pass and fail masks to zero, copy mask[] to oldmask[] */
      for (i=0;i<n;i++) {
	 passmask[i] = failmask[i] = 0;
         oldmask[i] = mask[i];
      }

      /* apply the depth test */
      if (ctx->Driver.DepthTestSpan)
         (*ctx->Driver.DepthTestSpan)( ctx, n, x, y, z, mask );

      /* set the stencil pass/fail flags according to result of depth test */
      for (i=0;i<n;i++) {
         if (oldmask[i]) {
            if (mask[i]) {
               passmask[i] = 1;
            }
            else {
               failmask[i] = 1;
            }
         }
      }

      /* apply the pass and fail operations */
      apply_stencil_op_to_span( ctx, n, x, y, ctx->Stencil.ZFailFunc, failmask );
      apply_stencil_op_to_span( ctx, n, x, y, ctx->Stencil.ZPassFunc, passmask );
   }
}




/*
 * Apply the given stencil operator for each pixel in the array whose
 * mask flag is set.
 * Input:  n - number of pixels in the span
 *         x, y - array of [n] pixels
 *         operator - the stencil buffer operator
 *         mask - array [n] of flag:  1=apply operator, 0=don't apply operator
 */
static void apply_stencil_op_to_pixels( GLcontext *ctx,
                                        GLuint n, const GLint x[],
				        const GLint y[],
				        GLenum oper, GLubyte mask[] )
{
   GLint i;
   GLstencil ref;
   GLstencil wrtmask, invmask;

   wrtmask = ctx->Stencil.WriteMask;
   invmask = ~ctx->Stencil.WriteMask;

   ref = ctx->Stencil.Ref;

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
		  *sptr = invmask & *sptr;
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
		  *sptr = (invmask & *sptr ) | (wrtmask & ref);
	       }
	    }
	 }
	 break;
      case GL_INCR:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  if (*sptr < 0xff) {
		     *sptr = *sptr + 1;
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  if (*sptr<0xff) {
		     *sptr = (invmask & *sptr) | (wrtmask & (*sptr+1));
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
		     *sptr = *sptr - 1;
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  if (*sptr>0) {
		     *sptr = (invmask & *sptr) | (wrtmask & (*sptr-1));
		  }
	       }
	    }
	 }
	 break;
      case GL_INVERT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = ~*sptr;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLstencil *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (invmask & *sptr) | (wrtmask & ~*sptr);
	       }
	    }
	 }
	 break;
      default:
         gl_problem(ctx, "Bad stencilop in apply_stencil_op_to_pixels");
   }
}



/*
 * Apply stencil test to an array of pixels before depth buffering.
 * Input:  n - number of pixels in the span
 *         x, y - array of [n] pixels to stencil
 *         mask - array [n] of flag:  0=skip the pixel, 1=stencil the pixel
 * Output:  mask - pixels which fail the stencil test will have their
 *                 mask flag set to 0.
 * Return:  0 = all pixels failed, 1 = zero or more pixels passed.
 */
GLint gl_stencil_pixels( GLcontext *ctx,
                         GLuint n, const GLint x[], const GLint y[],
			 GLubyte mask[] )
{
   GLubyte fail[PB_SIZE];
   GLstencil r, s;
   GLuint i;
   GLint allfail = 0;

   /*
    * Perform stencil test.  The results of this operation are stored
    * in the fail[] array:
    *   IF fail[i] is non-zero THEN
    *       the stencil fail operator is to be applied
    *   ELSE
    *       the stencil fail operator is not to be applied
    *   ENDIF
    */

   switch (ctx->Stencil.Function) {
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
	 allfail = 1;
	 break;
      case GL_LESS:
	 r = ctx->Stencil.Ref & ctx->Stencil.ValueMask;
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = *sptr & ctx->Stencil.ValueMask;
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
	 r = ctx->Stencil.Ref & ctx->Stencil.ValueMask;
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = *sptr & ctx->Stencil.ValueMask;
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
	 r = ctx->Stencil.Ref & ctx->Stencil.ValueMask;
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = *sptr & ctx->Stencil.ValueMask;
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
	 r = ctx->Stencil.Ref & ctx->Stencil.ValueMask;
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = *sptr & ctx->Stencil.ValueMask;
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
	 r = ctx->Stencil.Ref & ctx->Stencil.ValueMask;
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = *sptr & ctx->Stencil.ValueMask;
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
	 r = ctx->Stencil.Ref & ctx->Stencil.ValueMask;
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               GLstencil *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = *sptr & ctx->Stencil.ValueMask;
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
         gl_problem(ctx, "Bad stencil func in gl_stencil_pixels");
         return 0;
   }

   apply_stencil_op_to_pixels( ctx, n, x, y, ctx->Stencil.FailFunc, fail );

   return (allfail) ? 0 : 1;
}




/*
 * Apply the combination depth-buffer/stencil operator to a span of pixels.
 * Input:  n - number of pixels in the span
 *         x, y - array of [n] pixels to stencil
 *         z - array [n] of z values
 * Input:  mask - array [n] of flags  (1=test this pixel, 0=skip the pixel)
 * Output:  mask - array [n] of flags (1=depth test passed, 0=failed) 
 */
void gl_depth_stencil_pixels( GLcontext *ctx,
                              GLuint n, const GLint x[], const GLint y[],
			      const GLdepth z[], GLubyte mask[] )
{
   if (ctx->Depth.Test==GL_FALSE) {
      /*
       * No depth buffer, just apply zpass stencil function to active pixels.
       */
      apply_stencil_op_to_pixels( ctx, n, x, y, ctx->Stencil.ZPassFunc, mask );
   }
   else {
      /*
       * Perform depth buffering, then apply zpass or zfail stencil function.
       */
      GLubyte passmask[PB_SIZE], failmask[PB_SIZE], oldmask[PB_SIZE];
      GLuint i;

      /* init pass and fail masks to zero */
      for (i=0;i<n;i++) {
	 passmask[i] = failmask[i] = 0;
         oldmask[i] = mask[i];
      }

      /* apply the depth test */
      if (ctx->Driver.DepthTestPixels)
         (*ctx->Driver.DepthTestPixels)( ctx, n, x, y, z, mask );

      /* set the stencil pass/fail flags according to result of depth test */
      for (i=0;i<n;i++) {
         if (oldmask[i]) {
            if (mask[i]) {
               passmask[i] = 1;
            }
            else {
               failmask[i] = 1;
            }
         }
      }

      /* apply the pass and fail operations */
      apply_stencil_op_to_pixels( ctx, n, x, y,
                                  ctx->Stencil.ZFailFunc, failmask );
      apply_stencil_op_to_pixels( ctx, n, x, y,
                                  ctx->Stencil.ZPassFunc, passmask );
   }

}



/*
 * Return a span of stencil values from the stencil buffer.
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  stencil - the array of stencil values
 */
void gl_read_stencil_span( GLcontext *ctx,
                           GLuint n, GLint x, GLint y, GLubyte stencil[] )
{
   GLstencil *s;

   if (ctx->Buffer->Stencil) {
      s = STENCIL_ADDRESS( x, y );
      MEMCPY( stencil, s, n * sizeof(GLubyte) );
   }
}



/*
 * Write a span of stencil values to the stencil buffer.
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 *         stencil - the array of stencil values
 */
void gl_write_stencil_span( GLcontext *ctx,
                            GLuint n, GLint x, GLint y,
			    const GLubyte stencil[] )
{
   GLstencil *s;

   if (ctx->Buffer->Stencil) {
      s = STENCIL_ADDRESS( x, y );
      MEMCPY( s, stencil, n * sizeof(GLubyte) );
   }
}



/*
 * Allocate a new stencil buffer.  If there's an old one it will be
 * deallocated first.  The new stencil buffer will be uninitialized.
 */
void gl_alloc_stencil_buffer( GLcontext *ctx )
{
   GLuint buffersize = ctx->Buffer->Width * ctx->Buffer->Height;

   /* deallocate current stencil buffer if present */
   if (ctx->Buffer->Stencil) {
      free(ctx->Buffer->Stencil);
      ctx->Buffer->Stencil = NULL;
   }

   /* allocate new stencil buffer */
   ctx->Buffer->Stencil = (GLstencil *) malloc(buffersize * sizeof(GLstencil));
   if (!ctx->Buffer->Stencil) {
      /* out of memory */
      ctx->Stencil.Enabled = GL_FALSE;
      gl_error( ctx, GL_OUT_OF_MEMORY, "gl_alloc_stencil_buffer" );
   }
}




/*
 * Clear the stencil buffer.  If the stencil buffer doesn't exist yet we'll
 * allocate it now.
 */
void gl_clear_stencil_buffer( GLcontext *ctx )
{
   if (ctx->Visual->StencilBits==0 || !ctx->Buffer->Stencil) {
      /* no stencil buffer */
      return;
   }

   if (ctx->Scissor.Enabled) {
      /* clear scissor region only */
      GLint y;
      GLint width = ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1;
      for (y=ctx->Buffer->Ymin; y<=ctx->Buffer->Ymax; y++) {
         GLstencil *ptr = STENCIL_ADDRESS( ctx->Buffer->Xmin, y );
         MEMSET( ptr, ctx->Stencil.Clear, width * sizeof(GLstencil) );
      }
   }
   else {
      /* clear whole stencil buffer */
      MEMSET( ctx->Buffer->Stencil, ctx->Stencil.Clear,
              ctx->Buffer->Width * ctx->Buffer->Height * sizeof(GLstencil) );
   }
}
