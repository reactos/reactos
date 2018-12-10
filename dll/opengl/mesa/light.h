/* $Id: light.h,v 1.4 1997/04/01 04:09:16 brianp Exp $ */

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
 * $Log: light.h,v $
 * Revision 1.4  1997/04/01 04:09:16  brianp
 * removed shading functions
 *
 * Revision 1.3  1996/12/18 20:03:01  brianp
 * gl_set_material() now takes a bitmask instead of face and pname
 * added gl_material_bitmask()
 *
 * Revision 1.2  1996/12/07 10:21:58  brianp
 * added gl_set_material()
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef LIGHT_H
#define LIGHT_H


#include "types.h"


extern void gl_ShadeModel( GLcontext *ctx, GLenum mode );

extern void gl_ColorMaterial( GLcontext *ctx, GLenum face, GLenum mode );

extern void gl_Lightfv( GLcontext *ctx,
                        GLenum light, GLenum pname, const GLfloat *params,
                        GLint nparams );

extern void gl_LightModelfv( GLcontext *ctx,
                             GLenum pname, const GLfloat *params );


extern GLuint gl_material_bitmask( GLenum face, GLenum pname );

extern void gl_set_material( GLcontext *ctx, GLuint bitmask,
                             const GLfloat *params);

extern void gl_Materialfv( GLcontext *ctx,
                           GLenum face, GLenum pname, const GLfloat *params );



extern void gl_GetLightfv( GLcontext *ctx,
                           GLenum light, GLenum pname, GLfloat *params );

extern void gl_GetLightiv( GLcontext *ctx,
                           GLenum light, GLenum pname, GLint *params );


extern void gl_GetMaterialfv( GLcontext *ctx,
                              GLenum face, GLenum pname, GLfloat *params );

extern void gl_GetMaterialiv( GLcontext *ctx,
                              GLenum face, GLenum pname, GLint *params );


extern void gl_compute_spot_exp_table( struct gl_light *l );

extern void gl_compute_material_shine_table( struct gl_material *m );

extern void gl_update_lighting( GLcontext *ctx );


#endif

