/*
 * Mesa 3-D graphics library
 * Version:  6.0.1
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


/**
 * \file bufferobj.c
 * \brief Functions for the GL_ARB_vertex_buffer_object extension.
 * \author Brian Paul, Ian Romanick
 */


#include "glheader.h"
#include "hash.h"
#include "imports.h"
#include "context.h"
#include "bufferobj.h"


/**
 * Get the buffer object bound to the specified target in a GL context.
 *
 * \param ctx     GL context
 * \param target  Buffer object target to be retrieved.  Currently this must
 *                be either \c GL_ARRAY_BUFFER or \c GL_ELEMENT_ARRAY_BUFFER.
 * \param str     Name of caller for logging errors.
 * \return   A pointer to the buffer object bound to \c target in the
 *           specified context or \c NULL if \c target is invalid or no
 *           buffer object is bound.
 */
static INLINE struct gl_buffer_object *
buffer_object_get_target( GLcontext *ctx, GLenum target, const char * str )
{
   struct gl_buffer_object * bufObj = NULL;

   switch (target) {
      case GL_ARRAY_BUFFER_ARB:
         bufObj = ctx->Array.ArrayBufferObj;
         break;
      case GL_ELEMENT_ARRAY_BUFFER_ARB:
         bufObj = ctx->Array.ElementArrayBufferObj;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "gl%s(target)", str);
         return NULL;
   }

   if (bufObj->Name == 0)
      return NULL;

   return bufObj;
}


/**
 * Tests the subdata range parameters and sets the GL error code for
 * \c glBufferSubDataARB and \c glGetBufferSubDataARB.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param offset  Offset of the first byte of the subdata range.
 * \param size    Size, in bytes, of the subdata range.
 * \param str     Name of caller for logging errors.
 * \return   A pointer to the buffer object bound to \c target in the
 *           specified context or \c NULL if any of the parameter or state
 *           conditions for \c glBufferSubDataARB or \c glGetBufferSubDataARB
 *           are invalid.
 *
 * \sa glBufferSubDataARB, glGetBufferSubDataARB
 */
static struct gl_buffer_object *
buffer_object_subdata_range_good( GLcontext * ctx, GLenum target, 
                                  GLintptrARB offset, GLsizeiptrARB size,
                                  const char * str )
{
   struct gl_buffer_object *bufObj;

   if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(size < 0)", str);
      return NULL;
   }

   if (offset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(offset < 0)", str);
      return NULL;
   }

   bufObj = buffer_object_get_target( ctx, target, str );
   if ( bufObj == NULL ) {
      return NULL;
   }

   if ( (GLuint)(offset + size) > bufObj->Size ) {
      _mesa_error(ctx, GL_INVALID_VALUE,
		  "%s(size + offset > buffer size)", str);
      return NULL;
   }

   if ( bufObj->Pointer != NULL ) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", str);
      return NULL;
   }

   return bufObj;
}


/**
 * Allocate and initialize a new buffer object.
 * 
 * This function is intended to be called via
 * \c dd_function_table::NewBufferObject.
 */
struct gl_buffer_object *
_mesa_new_buffer_object( GLcontext *ctx, GLuint name, GLenum target )
{
   struct gl_buffer_object *obj;
   obj = MALLOC_STRUCT(gl_buffer_object);
   _mesa_initialize_buffer_object(obj, name, target);
   return obj;
}


/**
 * Delete a buffer object.
 * 
 * This function is intended to be called via
 * \c dd_function_table::DeleteBufferObject.
 */
void
_mesa_delete_buffer_object( GLcontext *ctx, struct gl_buffer_object *bufObj )
{
   if (bufObj->Data)
      _mesa_free(bufObj->Data);
   _mesa_free(bufObj);
}


/**
 * Initialize a buffer object to default values.
 */
void
_mesa_initialize_buffer_object( struct gl_buffer_object *obj,
				GLuint name, GLenum target )
{
   _mesa_bzero(obj, sizeof(struct gl_buffer_object));
   obj->RefCount = 1;
   obj->Name = name;
   obj->Usage = GL_STATIC_DRAW_ARB;
   obj->Access = GL_READ_WRITE_ARB;
}


/**
 * Add the given buffer object to the buffer object pool.
 */
void
_mesa_save_buffer_object( GLcontext *ctx, struct gl_buffer_object *obj )
{
   if (obj->Name > 0) {
      /* insert into hash table */
      _mesa_HashInsert(ctx->Shared->BufferObjects, obj->Name, obj);
   }
}


/**
 * Remove the given buffer object from the buffer object pool.
 * Do not deallocate the buffer object though.
 */
void
_mesa_remove_buffer_object( GLcontext *ctx, struct gl_buffer_object *bufObj )
{
   if (bufObj->Name > 0) {
      /* remove from hash table */
      _mesa_HashRemove(ctx->Shared->BufferObjects, bufObj->Name);
   }
}


/**
 * Allocate space for and store data in a buffer object.  Any data that was
 * previously stored in the buffer object is lost.  If \c data is \c NULL,
 * memory will be allocated, but no copy will occur.
 *
 * This function is intended to be called via
 * \c dd_function_table::BufferData.  This function need not set GL error
 * codes.  The input parameters will have been tested before calling.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param size    Size, in bytes, of the new data store.
 * \param data    Pointer to the data to store in the buffer object.  This
 *                pointer may be \c NULL.
 * \param usage   Hints about how the data will be used.
 * \param bufObj  Object to be used.
 *
 * \sa glBufferDataARB, dd_function_table::BufferData.
 */
void
_mesa_buffer_data( GLcontext *ctx, GLenum target, GLsizeiptrARB size,
		   const GLvoid * data, GLenum usage,
		   struct gl_buffer_object * bufObj )
{
   void * new_data;

   (void) target;

   new_data = _mesa_realloc( bufObj->Data, bufObj->Size, size );
   if ( new_data != NULL ) {
      bufObj->Data = (GLubyte *) new_data;
      bufObj->Size = size;
      bufObj->Usage = usage;

      if ( data != NULL ) {
	 _mesa_memcpy( bufObj->Data, data, size );
      }
   }
}


/**
 * Replace data in a subrange of buffer object.  If the data range
 * specified by \c size + \c offset extends beyond the end of the buffer or
 * if \c data is \c NULL, no copy is performed.
 *
 * This function is intended to be called by
 * \c dd_function_table::BufferSubData.  This function need not set GL error
 * codes.  The input parameters will have been tested before calling.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param offset  Offset of the first byte to be modified.
 * \param size    Size, in bytes, of the data range.
 * \param data    Pointer to the data to store in the buffer object.
 * \param bufObj  Object to be used.
 *
 * \sa glBufferSubDataARB, dd_function_table::BufferSubData.
 */
void
_mesa_buffer_subdata( GLcontext *ctx, GLenum target, GLintptrARB offset,
		      GLsizeiptrARB size, const GLvoid * data,
		      struct gl_buffer_object * bufObj )
{
   if ( (bufObj->Data != NULL)
	&& ((GLuint)(size + offset) <= bufObj->Size) ) {
      _mesa_memcpy( (GLubyte *) bufObj->Data + offset, data, size );
   }
}


/**
 * Retrieve data from a subrange of buffer object.  If the data range
 * specified by \c size + \c offset extends beyond the end of the buffer or
 * if \c data is \c NULL, no copy is performed.
 *
 * This function is intended to be called by
 * \c dd_function_table::BufferGetSubData.  This function need not set GL error
 * codes.  The input parameters will have been tested before calling.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param offset  Offset of the first byte to be modified.
 * \param size    Size, in bytes, of the data range.
 * \param data    Pointer to the data to store in the buffer object.
 * \param bufObj  Object to be used.
 *
 * \sa glBufferGetSubDataARB, dd_function_table::GetBufferSubData.
 */
void
_mesa_buffer_get_subdata( GLcontext *ctx, GLenum target, GLintptrARB offset,
			  GLsizeiptrARB size, GLvoid * data,
			  struct gl_buffer_object * bufObj )
{
   if ( (bufObj->Data != NULL)
	&& ((GLuint)(size + offset) <= bufObj->Size) ) {
      _mesa_memcpy( data, (GLubyte *) bufObj->Data + offset, size );
   }
}


/**
 * Maps the private data buffer into the processor's address space.
 *
 * This function is intended to be called by \c dd_function_table::MapBuffer.
 * This function need not set GL error codes.  The input parameters will have
 * been tested before calling.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param access  Information about how the buffer will be accessed.
 * \param bufObj  Object to be used.
 * \return  A pointer to the object's internal data store that can be accessed
 *          by the processor
 *
 * \sa glMapBufferARB, dd_function_table::MapBuffer
 */
void *
_mesa_buffer_map( GLcontext *ctx, GLenum target, GLenum access,
		  struct gl_buffer_object * bufObj )
{
   return bufObj->Data;
}


/**
 * Initialize the state associated with buffer objects
 */
void
_mesa_init_buffer_objects( GLcontext *ctx )
{
   GLuint i;

   ctx->Array.NullBufferObj = _mesa_new_buffer_object(ctx, 0, 0);
   ctx->Array.ArrayBufferObj = ctx->Array.NullBufferObj;
   ctx->Array.ElementArrayBufferObj = ctx->Array.NullBufferObj;

   /* Vertex array buffers */
   ctx->Array.Vertex.BufferObj = ctx->Array.NullBufferObj;
   ctx->Array.Normal.BufferObj = ctx->Array.NullBufferObj;
   ctx->Array.Color.BufferObj = ctx->Array.NullBufferObj;
   ctx->Array.SecondaryColor.BufferObj = ctx->Array.NullBufferObj;
   ctx->Array.FogCoord.BufferObj = ctx->Array.NullBufferObj;
   ctx->Array.Index.BufferObj = ctx->Array.NullBufferObj;
   for (i = 0; i < MAX_TEXTURE_UNITS; i++) {
      ctx->Array.TexCoord[i].BufferObj = ctx->Array.NullBufferObj;
   }
   ctx->Array.EdgeFlag.BufferObj = ctx->Array.NullBufferObj;
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      ctx->Array.VertexAttrib[i].BufferObj = ctx->Array.NullBufferObj;
   }

   /* Device drivers might override these assignments after the Mesa
    * context is initialized.
    */
   ctx->Driver.NewBufferObject = _mesa_new_buffer_object;
   ctx->Driver.DeleteBuffer = _mesa_delete_buffer_object;
   ctx->Driver.BindBuffer = NULL;
   ctx->Driver.BufferData = _mesa_buffer_data;
   ctx->Driver.BufferSubData = _mesa_buffer_subdata;
   ctx->Driver.GetBufferSubData = _mesa_buffer_get_subdata;
   ctx->Driver.MapBuffer = _mesa_buffer_map;
   ctx->Driver.UnmapBuffer = NULL;
}



/**********************************************************************/
/* API Functions                                                      */
/**********************************************************************/

void GLAPIENTRY
_mesa_BindBufferARB(GLenum target, GLuint buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *oldBufObj;
   struct gl_buffer_object *newBufObj = 0;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   oldBufObj = buffer_object_get_target( ctx, target, "BindBufferARB" );
   if ( (oldBufObj != NULL) && (oldBufObj->Name == buffer) )
      return;   /* rebinding the same buffer object- no change */

   /*
    * Get pointer to new buffer object (newBufObj)
    */
   if ( buffer == 0 ) {
      /* The spec says there's not a buffer object named 0, but we use
       * one internally because it simplifies things.
       */
      newBufObj = ctx->Array.NullBufferObj;
   }
   else {
      /* non-default buffer object */
      const struct _mesa_HashTable *hash = ctx->Shared->BufferObjects;
      newBufObj = (struct gl_buffer_object *) _mesa_HashLookup(hash, buffer);
      if (!newBufObj) {
         /* if this is a new buffer object id, allocate a buffer object now */
	 newBufObj = (*ctx->Driver.NewBufferObject)(ctx, buffer, target);
         if (!newBufObj) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindBufferARB");
            return;
         }
         _mesa_save_buffer_object(ctx, newBufObj);
      }
      newBufObj->RefCount++;
   }
   
   switch (target) {
      case GL_ARRAY_BUFFER_ARB:
         ctx->Array.ArrayBufferObj = newBufObj;
         break;
      case GL_ELEMENT_ARRAY_BUFFER_ARB:
         ctx->Array.ElementArrayBufferObj = newBufObj;
         break;
   }

   /* Pass BindBuffer call to device driver */
   if ( (ctx->Driver.BindBuffer != NULL) && (newBufObj != NULL) )
      (*ctx->Driver.BindBuffer)( ctx, target, newBufObj );

   if ( oldBufObj != NULL ) {
      oldBufObj->RefCount--;
      assert(oldBufObj->RefCount >= 0);
      if (oldBufObj->RefCount == 0) {
	 assert(oldBufObj->Name != 0);
	 _mesa_remove_buffer_object(ctx, oldBufObj);
	 ASSERT(ctx->Driver.DeleteBuffer);
	 (*ctx->Driver.DeleteBuffer)( ctx, oldBufObj );
      }
   }
}


/**
 * Delete a set of buffer objects.
 * 
 * \param n      Number of buffer objects to delete.
 * \param buffer Array of \c n buffer object IDs.
 */
void GLAPIENTRY
_mesa_DeleteBuffersARB(GLsizei n, const GLuint *ids)
{
   GET_CURRENT_CONTEXT(ctx);
   GLsizei i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteBuffersARB(n)");
      return;
   }

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   for (i = 0; i < n; i++) {
      if (ids[i] != 0) {
         struct gl_buffer_object *bufObj = (struct gl_buffer_object *)
            _mesa_HashLookup(ctx->Shared->BufferObjects, ids[i]);
         if (bufObj) {
            /* unbind any vertex pointers bound to this buffer */
            GLuint j;

            ASSERT(bufObj->Name != 0);

            if (ctx->Array.Vertex.BufferObj == bufObj)
               ctx->Array.Vertex.BufferObj = ctx->Array.NullBufferObj;
            if (ctx->Array.Normal.BufferObj == bufObj)
               ctx->Array.Normal.BufferObj = ctx->Array.NullBufferObj;
            if (ctx->Array.Color.BufferObj == bufObj)
               ctx->Array.Color.BufferObj = ctx->Array.NullBufferObj;
            if (ctx->Array.SecondaryColor.BufferObj == bufObj)
               ctx->Array.SecondaryColor.BufferObj = ctx->Array.NullBufferObj;
            if (ctx->Array.FogCoord.BufferObj == bufObj)
               ctx->Array.FogCoord.BufferObj = ctx->Array.NullBufferObj;
            if (ctx->Array.Index.BufferObj == bufObj)
               ctx->Array.Index.BufferObj = ctx->Array.NullBufferObj;
            if (ctx->Array.EdgeFlag.BufferObj == bufObj)
               ctx->Array.EdgeFlag.BufferObj = ctx->Array.NullBufferObj;
            for (j = 0; j < MAX_TEXTURE_UNITS; j++) {
               if (ctx->Array.TexCoord[j].BufferObj == bufObj)
                  ctx->Array.TexCoord[j].BufferObj = ctx->Array.NullBufferObj;
            }
            for (j = 0; j < VERT_ATTRIB_MAX; j++) {
               if (ctx->Array.VertexAttrib[j].BufferObj == bufObj)
                  ctx->Array.VertexAttrib[j].BufferObj = ctx->Array.NullBufferObj;
            }

            /* if deleting bound buffers, rebind to zero */
            if (ctx->Array.ArrayBufferObj == bufObj) {
	       _mesa_BindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
            }
            if (ctx->Array.ElementArrayBufferObj == bufObj) {
	       _mesa_BindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
            }

            bufObj->RefCount--;
            if (bufObj->RefCount <= 0) {
               _mesa_remove_buffer_object(ctx, bufObj);
               ASSERT(ctx->Driver.DeleteBuffer);
               (*ctx->Driver.DeleteBuffer)(ctx, bufObj);
            }
         }
      }
   }

   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
}


/**
 * Generate a set of unique buffer object IDs and store them in \c buffer.
 * 
 * \param n       Number of IDs to generate.
 * \param buffer  Array of \c n locations to store the IDs.
 */
void GLAPIENTRY
_mesa_GenBuffersARB(GLsizei n, GLuint *buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenBuffersARB");
      return;
   }

   if ( buffer == NULL ) {
      return;
   }

   /*
    * This must be atomic (generation and allocation of buffer object IDs)
    */
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->BufferObjects, n);

   /* Allocate new, empty buffer objects and return identifiers */
   for (i = 0; i < n; i++) {
      struct gl_buffer_object *bufObj;
      GLuint name = first + i;
      GLenum target = 0;
      bufObj = (*ctx->Driver.NewBufferObject)( ctx, name, target );
      if (!bufObj) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenBuffersARB");
         return;
      }
      _mesa_save_buffer_object(ctx, bufObj);
      buffer[i] = first + i;
   }

   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
}


/**
 * Determine if ID is the name of a buffer object.
 * 
 * \param id  ID of the potential buffer object.
 * \return  \c GL_TRUE if \c id is the name of a buffer object, 
 *          \c GL_FALSE otherwise.
 */
GLboolean GLAPIENTRY
_mesa_IsBufferARB(GLuint id)
{
   struct gl_buffer_object * bufObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (id == 0)
      return GL_FALSE;

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   bufObj = (struct gl_buffer_object *) _mesa_HashLookup(ctx->Shared->BufferObjects, id);
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   return (bufObj != NULL);
}


void GLAPIENTRY
_mesa_BufferDataARB(GLenum target, GLsizeiptrARB size,
                    const GLvoid * data, GLenum usage)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBufferDataARB(size < 0)");
      return;
   }

   switch (usage) {
      case GL_STREAM_DRAW_ARB:
      case GL_STREAM_READ_ARB:
      case GL_STREAM_COPY_ARB:
      case GL_STATIC_DRAW_ARB:
      case GL_STATIC_READ_ARB:
      case GL_STATIC_COPY_ARB:
      case GL_DYNAMIC_DRAW_ARB:
      case GL_DYNAMIC_READ_ARB:
      case GL_DYNAMIC_COPY_ARB:
         /* OK */
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glBufferDataARB(usage)");
         return;
   }

   bufObj = buffer_object_get_target( ctx, target, "BufferDataARB" );
   if ( bufObj == NULL ) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBufferDataARB" );
      return;
   }
   
   if (bufObj->Pointer) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBufferDataARB(buffer is mapped)" );
      return;
   }  

   ASSERT(ctx->Driver.BufferData);

   /* Give the buffer object to the driver!  <data> may be null! */
   (*ctx->Driver.BufferData)( ctx, target, size, data, usage, bufObj );
}


void GLAPIENTRY
_mesa_BufferSubDataARB(GLenum target, GLintptrARB offset,
                       GLsizeiptrARB size, const GLvoid * data)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   bufObj = buffer_object_subdata_range_good( ctx, target, offset, size,
                                              "BufferSubDataARB" );
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBufferSubDataARB" );
      return;
   }

   if (bufObj->Pointer) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBufferSubDataARB(buffer is mapped)" );
      return;
   }  

   ASSERT(ctx->Driver.BufferSubData);
   (*ctx->Driver.BufferSubData)( ctx, target, offset, size, data, bufObj );
}


void GLAPIENTRY
_mesa_GetBufferSubDataARB(GLenum target, GLintptrARB offset,
                          GLsizeiptrARB size, void * data)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   bufObj = buffer_object_subdata_range_good( ctx, target, offset, size,
                                              "GetBufferSubDataARB" );
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetBufferSubDataARB" );
      return;
   }

   if (bufObj->Pointer) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetBufferSubDataARB(buffer is mapped)" );
      return;
   }  

   ASSERT(ctx->Driver.GetBufferSubData);
   (*ctx->Driver.GetBufferSubData)( ctx, target, offset, size, data, bufObj );
}


void * GLAPIENTRY
_mesa_MapBufferARB(GLenum target, GLenum access)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object * bufObj;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, NULL);

   switch (access) {
      case GL_READ_ONLY_ARB:
      case GL_WRITE_ONLY_ARB:
      case GL_READ_WRITE_ARB:
         /* OK */
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glMapBufferARB(access)");
         return NULL;
   }

   bufObj = buffer_object_get_target( ctx, target, "MapBufferARB" );
   if ( bufObj == NULL ) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glMapBufferARB" );
      return NULL;
   }

   if ( bufObj->Pointer != NULL ) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glMapBufferARB(already mapped)");
      return NULL;
   }

   ASSERT(ctx->Driver.MapBuffer);
   bufObj->Pointer = (*ctx->Driver.MapBuffer)( ctx, target, access, bufObj );
   if ( bufObj->Pointer == NULL ) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glMapBufferARB(access)");
   }

   bufObj->Access = access;

   return bufObj->Pointer;
}


GLboolean GLAPIENTRY
_mesa_UnmapBufferARB(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   GLboolean status = GL_TRUE;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   bufObj = buffer_object_get_target( ctx, target, "UnmapBufferARB" );
   if ( bufObj == NULL ) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUnmapBufferARB" );
      return GL_FALSE;
   }

   if ( bufObj->Pointer == NULL ) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUnmapBufferARB");
      return GL_FALSE;
   }

   if ( ctx->Driver.UnmapBuffer != NULL ) {
      status = (*ctx->Driver.UnmapBuffer)( ctx, target, bufObj );
   }

   bufObj->Access = GL_READ_WRITE_ARB; /* initial value, OK? */
   bufObj->Pointer = NULL;

   return status;
}


void GLAPIENTRY
_mesa_GetBufferParameterivARB(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   bufObj = buffer_object_get_target( ctx, target, "GetBufferParameterivARB" );
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "GetBufferParameterivARB" );
      return;
   }

   switch (pname) {
      case GL_BUFFER_SIZE_ARB:
         *params = bufObj->Size;
         break;
      case GL_BUFFER_USAGE_ARB:
         *params = bufObj->Usage;
         break;
      case GL_BUFFER_ACCESS_ARB:
         *params = bufObj->Access;
         break;
      case GL_BUFFER_MAPPED_ARB:
         *params = (bufObj->Pointer != NULL);
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferParameterivARB(pname)");
         return;
   }
}


void GLAPIENTRY
_mesa_GetBufferPointervARB(GLenum target, GLenum pname, GLvoid **params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object * bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (pname != GL_BUFFER_MAP_POINTER_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferPointervARB(pname)");
      return;
   }

   bufObj = buffer_object_get_target( ctx, target, "GetBufferPointervARB" );
   if ( bufObj == NULL ) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetBufferPointervARB" );
      return;
   }

   *params = bufObj->Pointer;
}
