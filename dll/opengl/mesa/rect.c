/* $Id: rect.c,v 1.5 1997/07/24 01:25:18 brianp Exp $ */

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
 * $Log: rect.c,v $
 * Revision 1.5  1997/07/24 01:25:18  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.4  1997/05/28 03:26:18  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.3  1997/04/29 01:27:13  brianp
 * added #include "vbfill.h"
 *
 * Revision 1.2  1997/04/21 01:22:08  brianp
 * added a comment
 *
 * Revision 1.1  1997/04/12 16:00:35  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "context.h"
#include "macros.h"
#include "rect.h"
#include "vbfill.h"
#endif



/*
 * Execute a glRect*() function.
 */
void gl_Rectf( GLcontext *ctx, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   /*
    * TODO: we could examine a bunch of state variables and ultimately
    * call the Driver->RectFunc() function to draw a screen-aligned
    * filled rectangle.  Someday...
    */

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glRect" );
      return;
   }
   gl_Begin( ctx, GL_QUADS );
   (*ctx->Exec.Vertex2f)( ctx, x1, y1 );
   (*ctx->Exec.Vertex2f)( ctx, x2, y1 );
   (*ctx->Exec.Vertex2f)( ctx, x2, y2 );
   (*ctx->Exec.Vertex2f)( ctx, x1, y2 );
   gl_End( ctx );
}
