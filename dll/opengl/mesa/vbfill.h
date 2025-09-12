/* $Id: vbfill.h,v 1.7 1997/06/20 02:30:56 brianp Exp $ */

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
 * $Log: vbfill.h,v $
 * Revision 1.7  1997/06/20 02:30:56  brianp
 * added Color4ubv API pointer
 *
 * Revision 1.6  1997/06/20 02:30:22  brianp
 * changed color components from GLfixed to GLubyte
 *
 * Revision 1.5  1997/04/24 01:50:53  brianp
 * optimized glColor3f, glColor3fv, glColor4fv
 *
 * Revision 1.4  1997/04/16 23:55:33  brianp
 * added optimized glTexCoord2f code
 *
 * Revision 1.3  1997/04/14 22:18:23  brianp
 * added optimized glVertex3fv code
 *
 * Revision 1.2  1997/04/07 03:00:19  brianp
 * added vertex[23] functions
 *
 * Revision 1.1  1997/04/02 03:13:56  brianp
 * Initial revision
 *
 */


#ifndef VBFILL_H
#define VBFILL_H


#include "types.h"


extern void gl_Normal3f( GLcontext *ctx, GLfloat nx, GLfloat ny, GLfloat nz );

extern void gl_Normal3fv( GLcontext *ctx, const GLfloat *normal );


extern void gl_Indexf( GLcontext *ctx, GLfloat c );

extern void gl_Indexi( GLcontext *ctx, GLint c );


extern void gl_Color3f( GLcontext *ctx,
                        GLfloat red, GLfloat green, GLfloat blue );

extern void gl_Color3fv( GLcontext *ctx, const GLfloat *c );

extern void gl_Color4f( GLcontext *ctx,
                        GLfloat red, GLfloat green,
                        GLfloat blue, GLfloat alpha );

extern void gl_Color4fv( GLcontext *ctx, const GLfloat *c );

extern void gl_Color4ub( GLcontext *ctx,
                         GLubyte red, GLubyte green,
                         GLubyte blue, GLubyte alpha );

extern void gl_Color4ub8bit( GLcontext *ctx,
                             GLubyte red, GLubyte green,
                             GLubyte blue, GLubyte alpha );

extern void gl_Color4ubv( GLcontext *ctx, const GLubyte *c );

extern void gl_Color4ubv8bit( GLcontext *ctx, const GLubyte *c );


extern void gl_ColorMat3f( GLcontext *ctx,
                           GLfloat red, GLfloat green, GLfloat blue );

extern void gl_ColorMat3fv( GLcontext *ctx, const GLfloat *c );

extern void gl_ColorMat3ub( GLcontext *ctx,
                            GLubyte red, GLubyte green, GLubyte blue );

extern void gl_ColorMat4f( GLcontext *ctx,
                           GLfloat red, GLfloat green,
                           GLfloat blue, GLfloat alpha );

extern void gl_ColorMat4fv( GLcontext *ctx, const GLfloat *c );

extern void gl_ColorMat4ub( GLcontext *ctx,
                            GLubyte red, GLubyte green,
                            GLubyte blue, GLubyte alpha );


extern void gl_TexCoord2f( GLcontext *ctx, GLfloat s, GLfloat t );

extern void gl_TexCoord4f( GLcontext *ctx,
                           GLfloat s, GLfloat t, GLfloat r, GLfloat q );


extern void gl_EdgeFlag( GLcontext *ctx, GLboolean flag );



extern void gl_vertex2f_nop( GLcontext *ctx, GLfloat x, GLfloat y );

extern void gl_vertex3f_nop( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z );

extern void gl_vertex4f_nop( GLcontext *ctx,
                             GLfloat x, GLfloat y, GLfloat z, GLfloat w );

extern void gl_vertex3fv_nop( GLcontext *ctx, const GLfloat *v );



extern void gl_set_vertex_function( GLcontext *ctx );

extern void gl_set_color_function( GLcontext *ctx );



extern void gl_eval_vertex( GLcontext *ctx,
                            const GLfloat vertex[4], const GLfloat normal[3],
			    const GLubyte color[4], GLuint index,
                            const GLfloat texcoord[4] );


extern void gl_Begin( GLcontext *ctx, GLenum p );

extern void gl_End( GLcontext *ctx );


#endif

