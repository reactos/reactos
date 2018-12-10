/* $Id: misc.h,v 1.2 1997/04/12 12:31:18 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.3
 * Copyright (C) 1995-1996  Brian Paul
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
 * $Log: misc.h,v $
 * Revision 1.2  1997/04/12 12:31:18  brianp
 * removed gl_Rectf()
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef MISC_H
#define MISC_H


#include "types.h"


extern void gl_ClearIndex( GLcontext *ctx, GLfloat c );

extern void gl_ClearColor( GLcontext *ctx, GLclampf red, GLclampf green,
                           GLclampf blue, GLclampf alpha );

extern void gl_Clear( GLcontext *ctx, GLbitfield mask );


extern const GLubyte *gl_GetString( GLcontext *ctx, GLenum name );

extern void gl_Finish( GLcontext *ctx );

extern void gl_Flush( GLcontext *ctx );

extern void gl_Hint( GLcontext *ctx, GLenum target, GLenum mode );

extern void gl_DrawBuffer( GLcontext *ctx, GLenum mode );

extern void gl_ReadBuffer( GLcontext *ctx, GLenum mode );


#endif
