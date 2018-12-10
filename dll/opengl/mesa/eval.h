/* $Id: eval.h,v 1.2 1997/05/14 03:27:04 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.3
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
 * $Log: eval.h,v $
 * Revision 1.2  1997/05/14 03:27:04  brianp
 * removed context argument from gl_init_eval()
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef EVAL_H
#define EVAL_H


#include "types.h"


extern void gl_init_eval( void );


extern void gl_free_control_points( GLcontext *ctx,
                                    GLenum target, GLfloat *data );


extern GLfloat *gl_copy_map_points1f( GLenum target,
                                      GLint ustride, GLint uorder,
                                      const GLfloat *points );

extern GLfloat *gl_copy_map_points1d( GLenum target,
                                      GLint ustride, GLint uorder,
                                      const GLdouble *points );

extern GLfloat *gl_copy_map_points2f( GLenum target,
                                      GLint ustride, GLint uorder,
                                      GLint vstride, GLint vorder,
                                      const GLfloat *points );

extern GLfloat *gl_copy_map_points2d(GLenum target,
                                     GLint ustride, GLint uorder,
                                     GLint vstride, GLint vorder,
                                     const GLdouble *points );


extern void gl_Map1f( GLcontext* ctx,
                      GLenum target, GLfloat u1, GLfloat u2, GLint stride,
                      GLint order, const GLfloat *points, GLboolean retain );

extern void gl_Map2f( GLcontext* ctx, GLenum target,
                      GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
                      GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
                      const GLfloat *points, GLboolean retain );


extern void gl_EvalCoord1f( GLcontext* ctx, GLfloat u );

extern void gl_EvalCoord2f( GLcontext* ctx, GLfloat u, GLfloat v );


extern void gl_MapGrid1f( GLcontext* ctx, GLint un, GLfloat u1, GLfloat u2 );

extern void gl_MapGrid2f( GLcontext* ctx,
                          GLint un, GLfloat u1, GLfloat u2,
                          GLint vn, GLfloat v1, GLfloat v2 );


extern void gl_GetMapdv( GLcontext* ctx,
                         GLenum target, GLenum query, GLdouble *v );

extern void gl_GetMapfv( GLcontext* ctx,
                         GLenum target, GLenum query, GLfloat *v );

extern void gl_GetMapiv( GLcontext* ctx,
                         GLenum target, GLenum query, GLint *v );

extern void gl_EvalPoint1( GLcontext* ctx, GLint i );

extern void gl_EvalPoint2( GLcontext* ctx, GLint i, GLint j );

extern void gl_EvalMesh1( GLcontext* ctx, GLenum mode, GLint i1, GLint i2 );

extern void gl_EvalMesh2( GLcontext* ctx, GLenum mode,
                          GLint i1, GLint i2, GLint j1, GLint j2 );

#endif
