/*
 * Mesa 3-D graphics library
 * Version:  7.6
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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


#include "mfeatures.h"
#include "mtypes.h"


/*
 * Internal functions
 */


/** Is the given buffer object currently mapped? */
static inline GLboolean
_mesa_bufferobj_mapped(const struct gl_buffer_object *obj)
{
   return obj->Pointer != NULL;
}

/**
 * Is the given buffer object a user-created buffer object?
 * Mesa uses default buffer objects in several places.  Default buffers
 * always have Name==0.  User created buffers have Name!=0.
 */
static inline GLboolean
_mesa_is_bufferobj(const struct gl_buffer_object *obj)
{
   return obj != NULL && obj->Name != 0;
}


extern void
_mesa_init_buffer_objects( struct gl_context *ctx );

extern void
_mesa_free_buffer_objects( struct gl_context *ctx );

extern void
_mesa_update_default_objects_buffer_objects(struct gl_context *ctx);


extern struct gl_buffer_object *
_mesa_lookup_bufferobj(struct gl_context *ctx, GLuint buffer);

extern void
_mesa_initialize_buffer_object( struct gl_context *ctx,
				struct gl_buffer_object *obj,
				GLuint name, GLenum target );

extern void
_mesa_reference_buffer_object_(struct gl_context *ctx,
                               struct gl_buffer_object **ptr,
                               struct gl_buffer_object *bufObj);

static inline void
_mesa_reference_buffer_object(struct gl_context *ctx,
                              struct gl_buffer_object **ptr,
                              struct gl_buffer_object *bufObj)
{
   if (*ptr != bufObj)
      _mesa_reference_buffer_object_(ctx, ptr, bufObj);
}


extern void
_mesa_init_buffer_object_functions(struct dd_function_table *driver);


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
_mesa_GetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params);

extern void GLAPIENTRY
_mesa_GetBufferPointervARB(GLenum target, GLenum pname, GLvoid **params);

extern void * GLAPIENTRY
_mesa_MapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length,
                     GLbitfield access);

extern void GLAPIENTRY
_mesa_FlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);

#if FEATURE_APPLE_object_purgeable
extern GLenum GLAPIENTRY
_mesa_ObjectPurgeableAPPLE(GLenum objectType, GLuint name, GLenum option);

extern GLenum GLAPIENTRY
_mesa_ObjectUnpurgeableAPPLE(GLenum objectType, GLuint name, GLenum option);

extern void GLAPIENTRY
_mesa_GetObjectParameterivAPPLE(GLenum objectType, GLuint name, GLenum pname, GLint* params);
#endif

#endif
