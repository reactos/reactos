/* $Id: context.h,v 1.9 1997/09/29 22:25:28 brianp Exp $ */

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
 * $Log: context.h,v $
 * Revision 1.9  1997/09/29 22:25:28  brianp
 * added const to a few function parameters
 *
 * Revision 1.8  1997/05/26 21:14:13  brianp
 * gl_create_visual() now takes red/green/blue/alpha_bits arguments
 *
 * Revision 1.7  1997/04/16 23:55:06  brianp
 * added gl_set_api_table()
 *
 * Revision 1.6  1997/04/12 17:12:43  brianp
 * added gl_get_current_context()
 *
 * Revision 1.5  1997/02/27 19:57:38  brianp
 * added gl_problem() function
 *
 * Revision 1.4  1997/02/10 19:49:50  brianp
 * added gl_ResizeBuffersMESA()
 *
 * Revision 1.3  1996/09/19 03:14:49  brianp
 * now just one parameter for gl_create_framebuffer()
 *
 * Revision 1.2  1996/09/15 14:20:22  brianp
 * added new functions for GLframebuffer and GLvisual support
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef CONTEXT_H
#define CONTEXT_H


#include "types.h"



#ifdef THREADS
   /*
    * A seperate GLcontext for each thread
    */
   extern GLcontext *gl_get_thread_context( void );
#else
   /*
    * All threads use same pointer to current context.
    */
   extern GLcontext *CC;
#endif



/*
 * There are three Mesa datatypes which are meant to be used by device
 * drivers:
 *   GLcontext:  this contains the Mesa rendering state
 *   GLvisual:  this describes the color buffer (rgb vs. ci), whether
 *              or not there's a depth buffer, stencil buffer, etc.
 *   GLframebuffer:  contains pointers to the depth buffer, stencil
 *                   buffer, accum buffer and alpha buffers.
 *
 * These types should be encapsulated by corresponding device driver
 * datatypes.  See xmesa.h and xmesaP.h for an example.
 *
 * The following functions create and destroy these datatypes.
 */


/*
 * Create/destroy a GLvisual.  A GLvisual is like a GLX visual.  It describes
 * the colorbuffer, depth buffer, stencil buffer and accum buffer which will
 * be used by the GL context and framebuffer.
 */
extern GLvisual *gl_create_visual( GLboolean rgb_flag,
                                   GLboolean alpha_flag,
                                   GLboolean db_flag,
                                   GLint depth_bits,
                                   GLint stencil_bits,
                                   GLint accum_bits,
                                   GLint index_bits,
                                   GLfloat red_scale,
                                   GLfloat green_scale,
                                   GLfloat blue_scale,
                                   GLfloat alpha_scale,
                                   GLint red_bits,
                                   GLint green_bits,
                                   GLint blue_bits,
                                   GLint alpha_bits );

extern void gl_destroy_visual( GLvisual *vis );


/*
 * Create/destroy a GLcontext.  A GLcontext is like a GLX context.  It
 * contains the rendering state.
 */
extern GLcontext *gl_create_context( GLvisual *visual,
                                     GLcontext *share_list,
                                     void *driver_ctx );

extern void gl_destroy_context( GLcontext *ctx );


/*
 * Create/destroy a GLframebuffer.  A GLframebuffer is like a GLX drawable.
 * It bundles up the depth buffer, stencil buffer and accum buffers into a
 * single entity.
 */
extern GLframebuffer *gl_create_framebuffer( GLvisual *visual );

extern void gl_destroy_framebuffer( GLframebuffer *buffer );



extern void gl_make_current( GLcontext *ctx, GLframebuffer *buffer );

extern GLcontext *gl_get_current_context(void);

extern void gl_copy_context(const GLcontext *src, GLcontext *dst, GLuint mask);

extern void gl_set_api_table( GLcontext *ctx, const struct gl_api_table *api );



/*
 * GL_MESA_resize_buffers extension
 */
extern void gl_ResizeBuffersMESA( GLcontext *ctx );



/*
 * Miscellaneous
 */

extern void gl_problem( const GLcontext *ctx, const char *s );

extern void gl_warning( const GLcontext *ctx, const char *s );

extern void gl_error( GLcontext *ctx, GLenum error, const char *s );

extern GLenum gl_GetError( GLcontext *ctx );


extern void gl_update_state( GLcontext *ctx );



#ifdef PROFILE
extern GLdouble gl_time( void );
#endif


#endif
