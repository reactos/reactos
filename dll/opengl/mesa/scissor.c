/* $Id: scissor.c,v 1.6 1997/11/25 03:38:58 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.5
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
 * $Log: scissor.c,v $
 * Revision 1.6  1997/11/25 03:38:58  brianp
 * small optimization to gl_Scissor()  (Daryll Strauss)
 *
 * Revision 1.5  1997/07/24 01:21:56  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.4  1997/05/28 03:26:29  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.3  1997/05/17 03:17:50  brianp
 * faster gl_scissor_span() from Mats Lofkvist
 *
 * Revision 1.2  1996/09/15 14:18:37  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "context.h"
#include "macros.h"
#include "dlist.h"
#include "scissor.h"
#include "types.h"
#endif


void gl_Scissor( GLcontext *ctx,
                 GLint x, GLint y, GLsizei width, GLsizei height )
{
   if (width<0 || height<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glScissor" );
      return;
   }
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glBegin" );
      return;
   }

   if (x!=ctx->Scissor.X || y!=ctx->Scissor.Y || 
       width!=ctx->Scissor.Width || height!=ctx->Scissor.Height) {
      ctx->Scissor.X = x;
      ctx->Scissor.Y = y;
      ctx->Scissor.Width = width;
      ctx->Scissor.Height = height;
      ctx->NewState |= NEW_ALL;  /* TODO: this is overkill */
   }
}



/*
 * Apply the scissor test to a span of pixels.
 * Return:  0 = all pixels in the span are outside the scissor box.
 *          1 = one or more pixels passed the scissor test.
 */
GLint gl_scissor_span( GLcontext *ctx,
                       GLuint n, GLint x, GLint y, GLubyte mask[] )
{
   /* first check if whole span is outside the scissor box */
   if (y<ctx->Buffer->Ymin || y>ctx->Buffer->Ymax
       || x>ctx->Buffer->Xmax || x+(GLint)n-1<ctx->Buffer->Xmin) {
      return 0;
   }
   else {
      GLint i;
      GLint xMin = ctx->Buffer->Xmin;
      GLint xMax = ctx->Buffer->Xmax;
      for (i=0; x+i < xMin; i++) {
         mask[i] = 0;
      }
      for (i=(GLint)n-1; x+i > xMax; i--) {
         mask[i] = 0;
      }

      return 1;
   }
}




/*
 * Apply the scissor test to an array of pixels.
 */
GLuint gl_scissor_pixels( GLcontext *ctx,
                          GLuint n, const GLint x[], const GLint y[],
                          GLubyte mask[] )
{
   GLint xmin = ctx->Buffer->Xmin;
   GLint xmax = ctx->Buffer->Xmax;
   GLint ymin = ctx->Buffer->Ymin;
   GLint ymax = ctx->Buffer->Ymax;
   GLuint i;

   for (i=0;i<n;i++) {
      mask[i] &= (x[i]>=xmin) & (x[i]<=xmax) & (y[i]>=ymin) & (y[i]<=ymax);
   }

   return 1;
}

