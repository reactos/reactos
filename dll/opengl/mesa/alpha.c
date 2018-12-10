/* $Id: alpha.c,v 1.5 1997/07/24 01:24:28 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.4
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
 * $Log: alpha.c,v $
 * Revision 1.5  1997/07/24 01:24:28  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.4  1997/05/28 03:23:09  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.3  1997/03/13 03:07:53  brianp
 * optimized gl_alpha_test() by removing conditional from inside loops
 *
 * Revision 1.2  1996/09/15 14:15:54  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "alpha.h"
#include "context.h"
#include "types.h"
#include "dlist.h"
#include "macros.h"
#endif



void gl_AlphaFunc( GLcontext* ctx, GLenum func, GLclampf ref )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glAlphaFunc" );
      return;
   }
   switch (func) {
      case GL_NEVER:
      case GL_LESS:
      case GL_EQUAL:
      case GL_LEQUAL:
      case GL_GREATER:
      case GL_NOTEQUAL:
      case GL_GEQUAL:
      case GL_ALWAYS:
         ctx->Color.AlphaFunc = func;
         ctx->Color.AlphaRef = CLAMP( ref, 0.0F, 1.0F );
         ctx->Color.AlphaRefUbyte = (GLubyte) (ctx->Color.AlphaRef
                                              * ctx->Visual->AlphaScale);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glAlphaFunc(func)" );
         break;
   }
}




/*
 * Apply the alpha test to a span of pixels.
 * In/Out:  mask - current pixel mask.  Pixels which fail the alpha test
 *                 will set the corresponding mask flag to 0.
 * Return:  0 = all pixels in the span failed the alpha test.
 *          1 = one or more pixels passed the alpha test.
 */
GLint gl_alpha_test( GLcontext* ctx,
                     GLuint n, const GLubyte alpha[], GLubyte mask[] )
{
   GLuint i;
   GLubyte ref = ctx->Color.AlphaRefUbyte;

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Color.AlphaFunc) {
      case GL_LESS:
         for (i=0;i<n;i++) {
	    mask[i] &= (alpha[i] < ref);
	 }
	 return 1;
      case GL_LEQUAL:
         for (i=0;i<n;i++) {
	    mask[i] &= (alpha[i] <= ref);
	 }
	 return 1;
      case GL_GEQUAL:
         for (i=0;i<n;i++) {
	    mask[i] &= (alpha[i] >= ref);
	 }
	 return 1;
      case GL_GREATER:
         for (i=0;i<n;i++) {
	    mask[i] &= (alpha[i] > ref);
	 }
	 return 1;
      case GL_NOTEQUAL:
         for (i=0;i<n;i++) {
	    mask[i] &= (alpha[i] != ref);
	 }
	 return 1;
      case GL_EQUAL:
         for (i=0;i<n;i++) {
	    mask[i] &= (alpha[i] == ref);
	 }
	 return 1;
      case GL_ALWAYS:
	 /* do nothing */
	 return 1;
      case GL_NEVER:
         /* caller should check for zero! */
	 return 0;
      default:
	 gl_problem( ctx, "Invalid alpha test in gl_alpha_test" );
         return 0;
   }
   /* Never get here */
   return 1;
}
