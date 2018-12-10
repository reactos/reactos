/* $Id: quads.c,v 1.5 1997/08/19 02:44:18 brianp Exp $ */

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
 * $Log: quads.c,v $
 * Revision 1.5  1997/08/19 02:44:18  brianp
 * added null_quad() and re-implemented gl_set_quad_function()
 *
 * Revision 1.4  1997/07/24 01:23:44  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.3  1997/05/28 03:26:18  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.2  1997/04/20 19:45:46  brianp
 * added a comment
 *
 * Revision 1.1  1997/04/12 12:24:07  brianp
 * Initial revision
 *
 */


/*
 * Quadrilateral rendering functions.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "types.h"
#include "quads.h"
#endif



/*
 * At this time there is no quadrilateral optimization.  Just call the
 * triangle function twice.
 * v0, v1, v2, v3 in CCW order = front facing.
 */
static void quad( GLcontext *ctx,
                  GLuint v0, GLuint v1, GLuint v2, GLuint v3, GLuint pv )
{
   (*ctx->Driver.TriangleFunc)( ctx, v0, v1, v3, pv );
   (*ctx->Driver.TriangleFunc)( ctx, v1, v2, v3, pv );
}



/*
 * Draw nothing (NULL raster mode)
 */
static void null_quad( GLcontext *ctx,
                       GLuint v0, GLuint v1, GLuint v2, GLuint v3, GLuint pv )
{
}



void gl_set_quad_function( GLcontext *ctx )
{
   if (ctx->RenderMode==GL_RENDER) {
      if (ctx->NoRaster) {
         ctx->Driver.QuadFunc = null_quad;
      }
      else if (ctx->Driver.QuadFunc) {
         /* Device driver will draw quads. */
      }
      else {
         ctx->Driver.QuadFunc = quad;
      }
   }
   else {
      /* if in feedback or selection mode we can fall back to triangle code */
      ctx->Driver.QuadFunc = quad;
   }      
}


