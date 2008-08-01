/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */



#ifndef BUFFEROBJ_H
#define BUFFEROBJ_H


#include "context.h"


/*
 * Internal functions
 */

extern void
_mesa_init_buffer_objects( GLcontext *ctx );

extern struct gl_buffer_object *
_mesa_new_buffer_object( GLcontext *ctx, GLuint name, GLenum target );

extern void
_mesa_delete_buffer_object( GLcontext *ctx, struct gl_buffer_object *bufObj );

extern struct gl_buffer_object *
_mesa_lookup_bufferobj(GLcontext *ctx, GLuint buffer);

extern void
_mesa_initialize_buffer_object( struct gl_buffer_object *obj,
				GLuint name, GLenum target );

extern void
_mesa_save_buffer_object( GLcontext *ctx, struct gl_buffer_object *obj );

extern void
_mesa_remove_buffer_object( GLcontext *ctx, struct gl_buffer_object *bufObj );

extern void
_mesa_buffer_data( GLcontext *ctx, GLenum target, GLsizeiptrARB size,
		   const GLvoid * data, GLenum usage,
		   struct gl_buffer_object * bufObj );

extern void
_mesa_buffer_subdata( GLcontext *ctx, GLenum target, GLintptrARB offset,
		      GLsizeiptrARB size, const GLvoid * data,
		      struct gl_buffer_object * bufObj );

extern void
_mesa_buffer_get_subdata( GLcontext *ctx, GLenum target, GLintptrARB offset,
			  GLsizeiptrARB size, GLvoid * data,
			  struct gl_buffer_object * bufObj );

extern void *
_mesa_buffer_map( GLcontext *ctx, GLenum target, GLenum access,
		  struct gl_buffer_object * bufObj );

extern GLboolean
_mesa_buffer_unmap( GLcontext *ctx, GLenum target,
                    struct gl_buffer_object * bufObj );

extern GLboolean
_mesa_validate_pbo_access(GLuint dimensions,
                          const struct gl_pixelstore_attrib *pack,
                          GLsizei width, GLsizei height, GLsizei depth,
                          GLenum format, GLenum type, const GLvoid *ptr);

extern void
_mesa_unbind_buffer_object( GLcontext *ctx, struct gl_buffer_object *bufObj );

/*
 * API functions
 */

extern void GLAPIENTRY
_mesa_BindBufferARB(GLenum target, GLuint buffer);

extern void GLAPIENTRY
_mesa_DeleteBuffersARB(GLsizei n, const GLuint * buffer);

extern void GLAPIENTRY
_mesa_GenBuffersARB(GLsizei n, GLuint * buffer);

extern GLboolean GLAPIENTRY
_mesa_IsBufferARB(GLuint buffer);

extern void GLAPIENTRY
_mesa_BufferDataARB(GLenum target, GLsizeiptrARB size, const GLvoid * data, GLenum usage);

extern void GLAPIENTRY
_mesa_BufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid * data);

extern void GLAPIENTRY
_mesa_GetBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, void * data);

extern void * GLAPIENTRY
_mesa_MapBufferARB(GLenum target, GLenum access);

extern GLboolean GLAPIENTRY
_mesa_UnmapBufferARB(GLenum target);

extern void GLAPIENTRY
_mesa_GetBufferParameterivARB(GLenum target, GLenum pname, GLint *params);

extern void GLAPIENTRY
_mesa_GetBufferPointervARB(GLenum target, GLenum pname, GLvoid **params);

#endif
