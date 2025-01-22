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


/**
 * \file bufferobj.c
 * \brief Functions for the GL_ARB_vertex/pixel_buffer_object extensions.
 * \author Brian Paul, Ian Romanick
 */

#include <precomp.h>

/* Debug flags */
/*#define VBO_DEBUG*/
/*#define BOUNDS_CHECK*/


/**
 * Used as a placeholder for buffer objects between glGenBuffers() and
 * glBindBuffer() so that glIsBuffer() can work correctly.
 */
static struct gl_buffer_object DummyBufferObject;


/**
 * Return pointer to address of a buffer object target.
 * \param ctx  the GL context
 * \param target  the buffer object target to be retrieved.
 * \return   pointer to pointer to the buffer object bound to \c target in the
 *           specified context or \c NULL if \c target is invalid.
 */
static inline struct gl_buffer_object **
get_buffer_target(struct gl_context *ctx, GLenum target)
{
   switch (target) {
   case GL_ELEMENT_ARRAY_BUFFER_ARB:
      return &ctx->Array.ElementArrayBufferObj;
   default:
      return NULL;
   }
   return NULL;
}


/**
 * Get the buffer object bound to the specified target in a GL context.
 * \param ctx  the GL context
 * \param target  the buffer object target to be retrieved.
 * \return   pointer to the buffer object bound to \c target in the
 *           specified context or \c NULL if \c target is invalid.
 */
static inline struct gl_buffer_object *
get_buffer(struct gl_context *ctx, const char *func, GLenum target)
{
   struct gl_buffer_object **bufObj = get_buffer_target(ctx, target);

   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(target)", func);
      return NULL;
   }

   if (!_mesa_is_bufferobj(*bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(buffer 0)", func);
      return NULL;
   }

   return *bufObj;
}


static inline GLenum
default_access_mode(const struct gl_context *ctx)
{
   (void)ctx;
   return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
}


/**
 * Convert a GLbitfield describing the mapped buffer access flags
 * into one of GL_READ_WRITE, GL_READ_ONLY, or GL_WRITE_ONLY.
 */
static GLenum
simplified_access_mode(GLbitfield access)
{
   const GLbitfield rwFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
   if ((access & rwFlags) == rwFlags)
      return GL_READ_WRITE;
   if ((access & GL_MAP_READ_BIT) == GL_MAP_READ_BIT)
      return GL_READ_ONLY;
   if ((access & GL_MAP_WRITE_BIT) == GL_MAP_WRITE_BIT)
      return GL_WRITE_ONLY;
   return GL_READ_WRITE; /* this should never happen, but no big deal */
}


/**
 * Tests the subdata range parameters and sets the GL error code for
 * \c glBufferSubDataARB and \c glGetBufferSubDataARB.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param offset  Offset of the first byte of the subdata range.
 * \param size    Size, in bytes, of the subdata range.
 * \param caller  Name of calling function for recording errors.
 * \return   A pointer to the buffer object bound to \c target in the
 *           specified context or \c NULL if any of the parameter or state
 *           conditions for \c glBufferSubDataARB or \c glGetBufferSubDataARB
 *           are invalid.
 *
 * \sa glBufferSubDataARB, glGetBufferSubDataARB
 */
static struct gl_buffer_object *
buffer_object_subdata_range_good( struct gl_context * ctx, GLenum target, 
                                  GLintptrARB offset, GLsizeiptrARB size,
                                  const char *caller )
{
   struct gl_buffer_object *bufObj;

   if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(size < 0)", caller);
      return NULL;
   }

   if (offset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(offset < 0)", caller);
      return NULL;
   }

   bufObj = get_buffer(ctx, caller, target);
   if (!bufObj)
      return NULL;

   if (offset + size > bufObj->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
		  "%s(offset %lu + size %lu > buffer size %lu)", caller,
                  (unsigned long) offset,
                  (unsigned long) size,
                  (unsigned long) bufObj->Size);
      return NULL;
   }
   if (_mesa_bufferobj_mapped(bufObj)) {
      /* Buffer is currently mapped */
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
      return NULL;
   }

   return bufObj;
}


/**
 * Allocate and initialize a new buffer object.
 * 
 * Default callback for the \c dd_function_table::NewBufferObject() hook.
 */
static struct gl_buffer_object *
_mesa_new_buffer_object( struct gl_context *ctx, GLuint name, GLenum target )
{
   struct gl_buffer_object *obj;

   (void) ctx;

   obj = MALLOC_STRUCT(gl_buffer_object);
   _mesa_initialize_buffer_object(ctx, obj, name, target);
   return obj;
}


/**
 * Delete a buffer object.
 * 
 * Default callback for the \c dd_function_table::DeleteBuffer() hook.
 */
static void
_mesa_delete_buffer_object(struct gl_context *ctx,
                           struct gl_buffer_object *bufObj)
{
   (void) ctx;

   if (bufObj->Data)
      free(bufObj->Data);

   /* assign strange values here to help w/ debugging */
   bufObj->RefCount = -1000;
   bufObj->Name = ~0;

   _glthread_DESTROY_MUTEX(bufObj->Mutex);
   free(bufObj);
}



/**
 * Set ptr to bufObj w/ reference counting.
 * This is normally only called from the _mesa_reference_buffer_object() macro
 * when there's a real pointer change.
 */
void
_mesa_reference_buffer_object_(struct gl_context *ctx,
                               struct gl_buffer_object **ptr,
                               struct gl_buffer_object *bufObj)
{
   if (*ptr) {
      /* Unreference the old buffer */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_buffer_object *oldObj = *ptr;

      _glthread_LOCK_MUTEX(oldObj->Mutex);
      ASSERT(oldObj->RefCount > 0);
      oldObj->RefCount--;
#if 0
      printf("BufferObj %p %d DECR to %d\n",
             (void *) oldObj, oldObj->Name, oldObj->RefCount);
#endif
      deleteFlag = (oldObj->RefCount == 0);
      _glthread_UNLOCK_MUTEX(oldObj->Mutex);

      if (deleteFlag) {

         /* some sanity checking: don't delete a buffer still in use */
#if 0
         /* unfortunately, these tests are invalid during context tear-down */
	 ASSERT(ctx->Array.ArrayBufferObj != bufObj);
	 ASSERT(ctx->Array.ArrayObj->ElementArrayBufferObj != bufObj);
	 ASSERT(ctx->Array.ArrayObj->Vertex.BufferObj != bufObj);
#endif

	 ASSERT(ctx->Driver.DeleteBuffer);
         ctx->Driver.DeleteBuffer(ctx, oldObj);
      }

      *ptr = NULL;
   }
   ASSERT(!*ptr);

   if (bufObj) {
      /* reference new buffer */
      _glthread_LOCK_MUTEX(bufObj->Mutex);
      if (bufObj->RefCount == 0) {
         /* this buffer's being deleted (look just above) */
         /* Not sure this can every really happen.  Warn if it does. */
         _mesa_problem(NULL, "referencing deleted buffer object");
         *ptr = NULL;
      }
      else {
         bufObj->RefCount++;
#if 0
         printf("BufferObj %p %d INCR to %d\n",
                (void *) bufObj, bufObj->Name, bufObj->RefCount);
#endif
         *ptr = bufObj;
      }
      _glthread_UNLOCK_MUTEX(bufObj->Mutex);
   }
}


/**
 * Initialize a buffer object to default values.
 */
void
_mesa_initialize_buffer_object( struct gl_context *ctx,
				struct gl_buffer_object *obj,
				GLuint name, GLenum target )
{
   (void) target;

   memset(obj, 0, sizeof(struct gl_buffer_object));
   _glthread_INIT_MUTEX(obj->Mutex);
   obj->RefCount = 1;
   obj->Name = name;
   obj->Usage = GL_STATIC_DRAW_ARB;
   obj->AccessFlags = default_access_mode(ctx);
}


/**
 * Allocate space for and store data in a buffer object.  Any data that was
 * previously stored in the buffer object is lost.  If \c data is \c NULL,
 * memory will be allocated, but no copy will occur.
 *
 * This is the default callback for \c dd_function_table::BufferData()
 * Note that all GL error checking will have been done already.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param size    Size, in bytes, of the new data store.
 * \param data    Pointer to the data to store in the buffer object.  This
 *                pointer may be \c NULL.
 * \param usage   Hints about how the data will be used.
 * \param bufObj  Object to be used.
 *
 * \return GL_TRUE for success, GL_FALSE for failure
 * \sa glBufferDataARB, dd_function_table::BufferData.
 */
static GLboolean
_mesa_buffer_data( struct gl_context *ctx, GLenum target, GLsizeiptrARB size,
		   const GLvoid * data, GLenum usage,
		   struct gl_buffer_object * bufObj )
{
   void * new_data;

   (void) ctx; (void) target;

   new_data = _mesa_realloc( bufObj->Data, bufObj->Size, size );
   if (new_data) {
      bufObj->Data = (GLubyte *) new_data;
      bufObj->Size = size;
      bufObj->Usage = usage;

      if (data) {
	 memcpy( bufObj->Data, data, size );
      }

      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}


/**
 * Replace data in a subrange of buffer object.  If the data range
 * specified by \c size + \c offset extends beyond the end of the buffer or
 * if \c data is \c NULL, no copy is performed.
 *
 * This is the default callback for \c dd_function_table::BufferSubData()
 * Note that all GL error checking will have been done already.
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
static void
_mesa_buffer_subdata( struct gl_context *ctx, GLintptrARB offset,
		      GLsizeiptrARB size, const GLvoid * data,
		      struct gl_buffer_object * bufObj )
{
   (void) ctx;

   /* this should have been caught in _mesa_BufferSubData() */
   ASSERT(size + offset <= bufObj->Size);

   if (bufObj->Data) {
      memcpy( (GLubyte *) bufObj->Data + offset, data, size );
   }
}


/**
 * Retrieve data from a subrange of buffer object.  If the data range
 * specified by \c size + \c offset extends beyond the end of the buffer or
 * if \c data is \c NULL, no copy is performed.
 *
 * This is the default callback for \c dd_function_table::GetBufferSubData()
 * Note that all GL error checking will have been done already.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param offset  Offset of the first byte to be fetched.
 * \param size    Size, in bytes, of the data range.
 * \param data    Destination for data
 * \param bufObj  Object to be used.
 *
 * \sa glBufferGetSubDataARB, dd_function_table::GetBufferSubData.
 */
static void
_mesa_buffer_get_subdata( struct gl_context *ctx, GLintptrARB offset,
			  GLsizeiptrARB size, GLvoid * data,
			  struct gl_buffer_object * bufObj )
{
   (void) ctx;

   if (bufObj->Data && ((GLsizeiptrARB) (size + offset) <= bufObj->Size)) {
      memcpy( data, (GLubyte *) bufObj->Data + offset, size );
   }
}


/**
 * Default fallback for \c dd_function_table::MapBufferRange().
 * Called via glMapBufferRange().
 */
static void *
_mesa_buffer_map_range( struct gl_context *ctx, GLintptr offset,
                        GLsizeiptr length, GLbitfield access,
                        struct gl_buffer_object *bufObj )
{
   (void) ctx;
   assert(!_mesa_bufferobj_mapped(bufObj));
   /* Just return a direct pointer to the data */
   bufObj->Pointer = bufObj->Data + offset;
   bufObj->Length = length;
   bufObj->Offset = offset;
   bufObj->AccessFlags = access;
   return bufObj->Pointer;
}


/**
 * Default fallback for \c dd_function_table::FlushMappedBufferRange().
 * Called via glFlushMappedBufferRange().
 */
static void
_mesa_buffer_flush_mapped_range( struct gl_context *ctx,
                                 GLintptr offset, GLsizeiptr length,
                                 struct gl_buffer_object *obj )
{
   (void) ctx;
   (void) offset;
   (void) length;
   (void) obj;
   /* no-op */
}


/**
 * Default callback for \c dd_function_table::MapBuffer().
 *
 * The input parameters will have been already tested for errors.
 *
 * \sa glUnmapBufferARB, dd_function_table::UnmapBuffer
 */
static GLboolean
_mesa_buffer_unmap( struct gl_context *ctx, struct gl_buffer_object *bufObj )
{
   (void) ctx;
   /* XXX we might assert here that bufObj->Pointer is non-null */
   bufObj->Pointer = NULL;
   bufObj->Length = 0;
   bufObj->Offset = 0;
   bufObj->AccessFlags = 0x0;
   return GL_TRUE;
}


/**
 * Initialize the state associated with buffer objects
 */
void
_mesa_init_buffer_objects( struct gl_context *ctx )
{
   memset(&DummyBufferObject, 0, sizeof(DummyBufferObject));
   _glthread_INIT_MUTEX(DummyBufferObject.Mutex);
   DummyBufferObject.RefCount = 1000*1000*1000; /* never delete */
}


/**
 * Bind the specified target to buffer for the specified context.
 * Called by glBindBuffer() and other functions.
 */
static void
bind_buffer_object(struct gl_context *ctx, GLenum target, GLuint buffer)
{
   struct gl_buffer_object *oldBufObj;
   struct gl_buffer_object *newBufObj = NULL;
   struct gl_buffer_object **bindTarget = NULL;

   bindTarget = get_buffer_target(ctx, target);
   if (!bindTarget) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferARB(target 0x%x)", target);
      return;
   }

   /* Get pointer to old buffer object (to be unbound) */
   oldBufObj = *bindTarget;
   if (oldBufObj && oldBufObj->Name == buffer && !oldBufObj->DeletePending)
      return;   /* rebinding the same buffer object- no change */

   /*
    * Get pointer to new buffer object (newBufObj)
    */
   if (buffer == 0) {
      /* The spec says there's not a buffer object named 0, but we use
       * one internally because it simplifies things.
       */
      newBufObj = ctx->Shared->NullBufferObj;
   }
   else {
      /* non-default buffer object */
      newBufObj = _mesa_lookup_bufferobj(ctx, buffer);
      if (!newBufObj || newBufObj == &DummyBufferObject) {
         /* If this is a new buffer object id, or one which was generated but
          * never used before, allocate a buffer object now.
          */
         ASSERT(ctx->Driver.NewBufferObject);
         newBufObj = ctx->Driver.NewBufferObject(ctx, buffer, target);
         if (!newBufObj) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindBufferARB");
            return;
         }
         _mesa_HashInsert(ctx->Shared->BufferObjects, buffer, newBufObj);
      }
   }
   
   /* bind new buffer */
   _mesa_reference_buffer_object(ctx, bindTarget, newBufObj);

   /* Pass BindBuffer call to device driver */
   if (ctx->Driver.BindBuffer)
      ctx->Driver.BindBuffer( ctx, target, newBufObj );
}


/**
 * Update the default buffer objects in the given context to reference those
 * specified in the shared state and release those referencing the old 
 * shared state.
 */
void
_mesa_update_default_objects_buffer_objects(struct gl_context *ctx)
{
   /* Bind the NullBufferObj to remove references to those
    * in the shared context hash table.
    */
   bind_buffer_object( ctx, GL_ARRAY_BUFFER_ARB, 0);
   bind_buffer_object( ctx, GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
}



/**
 * Return the gl_buffer_object for the given ID.
 * Always return NULL for ID 0.
 */
struct gl_buffer_object *
_mesa_lookup_bufferobj(struct gl_context *ctx, GLuint buffer)
{
   if (buffer == 0)
      return NULL;
   else
      return (struct gl_buffer_object *)
         _mesa_HashLookup(ctx->Shared->BufferObjects, buffer);
}


/**
 * If *ptr points to obj, set ptr = the Null/default buffer object.
 * This is a helper for buffer object deletion.
 * The GL spec says that deleting a buffer object causes it to get
 * unbound from all arrays in the current context.
 */
static void
unbind(struct gl_context *ctx,
       struct gl_buffer_object **ptr,
       struct gl_buffer_object *obj)
{
   if (*ptr == obj) {
      _mesa_reference_buffer_object(ctx, ptr, ctx->Shared->NullBufferObj);
   }
}


/**
 * Plug default/fallback buffer object functions into the device
 * driver hooks.
 */
void
_mesa_init_buffer_object_functions(struct dd_function_table *driver)
{
   /* GL_ARB_vertex/pixel_buffer_object */
   driver->NewBufferObject = _mesa_new_buffer_object;
   driver->DeleteBuffer = _mesa_delete_buffer_object;
   driver->BindBuffer = NULL;
   driver->BufferData = _mesa_buffer_data;
   driver->BufferSubData = _mesa_buffer_subdata;
   driver->GetBufferSubData = _mesa_buffer_get_subdata;
   driver->UnmapBuffer = _mesa_buffer_unmap;

   /* GL_ARB_map_buffer_range */
   driver->MapBufferRange = _mesa_buffer_map_range;
   driver->FlushMappedBufferRange = _mesa_buffer_flush_mapped_range;
}



/**********************************************************************/
/* API Functions                                                      */
/**********************************************************************/

void GLAPIENTRY
_mesa_BindBufferARB(GLenum target, GLuint buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBindBuffer(%s, %u)\n",
                  _mesa_lookup_enum_by_nr(target), buffer);

   bind_buffer_object(ctx, target, buffer);
}


/**
 * Delete a set of buffer objects.
 * 
 * \param n      Number of buffer objects to delete.
 * \param ids    Array of \c n buffer object IDs.
 */
void GLAPIENTRY
_mesa_DeleteBuffersARB(GLsizei n, const GLuint *ids)
{
   GET_CURRENT_CONTEXT(ctx);
   GLsizei i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);
   FLUSH_VERTICES(ctx, 0);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteBuffersARB(n)");
      return;
   }

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   for (i = 0; i < n; i++) {
      struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, ids[i]);
      if (bufObj) {
         GLuint j;

         ASSERT(bufObj->Name == ids[i] || bufObj == &DummyBufferObject);

         if (_mesa_bufferobj_mapped(bufObj)) {
            /* if mapped, unmap it now */
            ctx->Driver.UnmapBuffer(ctx, bufObj);
            bufObj->AccessFlags = default_access_mode(ctx);
            bufObj->Pointer = NULL;
         }

         /* unbind any vertex pointers bound to this buffer */
         for (j = 0; j < Elements(ctx->Array.VertexAttrib); j++) {
            unbind(ctx, &ctx->Array.VertexAttrib[j].BufferObj, bufObj);
         }

         if (ctx->Array.ElementArrayBufferObj == bufObj) {
            _mesa_BindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
         }

         /* The ID is immediately freed for re-use */
         _mesa_HashRemove(ctx->Shared->BufferObjects, ids[i]);
         /* Make sure we do not run into the classic ABA problem on bind.
          * We don't want to allow re-binding a buffer object that's been
          * "deleted" by glDeleteBuffers().
          *
          * The explicit rebinding to the default object in the current context
          * prevents the above in the current context, but another context
          * sharing the same objects might suffer from this problem.
          * The alternative would be to do the hash lookup in any case on bind
          * which would introduce more runtime overhead than this.
          */
         bufObj->DeletePending = GL_TRUE;
         _mesa_reference_buffer_object(ctx, &bufObj, NULL);
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

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGenBuffers(%d)\n", n);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenBuffersARB");
      return;
   }

   if (!buffer) {
      return;
   }

   /*
    * This must be atomic (generation and allocation of buffer object IDs)
    */
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->BufferObjects, n);

   /* Insert the ID and pointer to dummy buffer object into hash table */
   for (i = 0; i < n; i++) {
      _mesa_HashInsert(ctx->Shared->BufferObjects, first + i,
                       &DummyBufferObject);
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
   struct gl_buffer_object *bufObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   bufObj = _mesa_lookup_bufferobj(ctx, id);
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   return bufObj && bufObj != &DummyBufferObject;
}


void GLAPIENTRY
_mesa_BufferDataARB(GLenum target, GLsizeiptrARB size,
                    const GLvoid * data, GLenum usage)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBufferData(%s, %ld, %p, %s)\n",
                  _mesa_lookup_enum_by_nr(target),
                  (long int) size, data,
                  _mesa_lookup_enum_by_nr(usage));

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

   bufObj = get_buffer(ctx, "glBufferDataARB", target);
   if (!bufObj)
      return;

   if (_mesa_bufferobj_mapped(bufObj)) {
      /* Unmap the existing buffer.  We'll replace it now.  Not an error. */
      ctx->Driver.UnmapBuffer(ctx, bufObj);
      bufObj->AccessFlags = default_access_mode(ctx);
      ASSERT(bufObj->Pointer == NULL);
   }  

   FLUSH_VERTICES(ctx, _NEW_BUFFER_OBJECT);

   bufObj->Written = GL_TRUE;

#ifdef VBO_DEBUG
   printf("glBufferDataARB(%u, sz %ld, from %p, usage 0x%x)\n",
                bufObj->Name, size, data, usage);
#endif

#ifdef BOUNDS_CHECK
   size += 100;
#endif

   ASSERT(ctx->Driver.BufferData);
   if (!ctx->Driver.BufferData( ctx, target, size, data, usage, bufObj )) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBufferDataARB()");
   }
}


void GLAPIENTRY
_mesa_BufferSubDataARB(GLenum target, GLintptrARB offset,
                       GLsizeiptrARB size, const GLvoid * data)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   bufObj = buffer_object_subdata_range_good( ctx, target, offset, size,
                                              "glBufferSubDataARB" );
   if (!bufObj) {
      /* error already recorded */
      return;
   }

   if (size == 0)
      return;

   bufObj->Written = GL_TRUE;

   ASSERT(ctx->Driver.BufferSubData);
   ctx->Driver.BufferSubData( ctx, offset, size, data, bufObj );
}


void GLAPIENTRY
_mesa_GetBufferSubDataARB(GLenum target, GLintptrARB offset,
                          GLsizeiptrARB size, void * data)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   bufObj = buffer_object_subdata_range_good( ctx, target, offset, size,
                                              "glGetBufferSubDataARB" );
   if (!bufObj) {
      /* error already recorded */
      return;
   }

   ASSERT(ctx->Driver.GetBufferSubData);
   ctx->Driver.GetBufferSubData( ctx, offset, size, data, bufObj );
}


void * GLAPIENTRY
_mesa_MapBufferARB(GLenum target, GLenum access)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object * bufObj;
   GLbitfield accessFlags;
   void *map;

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, NULL);

   switch (access) {
   case GL_READ_ONLY_ARB:
      accessFlags = GL_MAP_READ_BIT;
      break;
   case GL_WRITE_ONLY_ARB:
      accessFlags = GL_MAP_WRITE_BIT;
      break;
   case GL_READ_WRITE_ARB:
      accessFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glMapBufferARB(access)");
      return NULL;
   }

   bufObj = get_buffer(ctx, "glMapBufferARB", target);
   if (!bufObj)
      return NULL;

   if (_mesa_bufferobj_mapped(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glMapBufferARB(already mapped)");
      return NULL;
   }

   if (!bufObj->Size) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glMapBuffer(buffer size = 0)");
      return NULL;
   }

   ASSERT(ctx->Driver.MapBufferRange);
   map = ctx->Driver.MapBufferRange(ctx, 0, bufObj->Size, accessFlags, bufObj);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glMapBufferARB(map failed)");
      return NULL;
   }
   else {
      /* The driver callback should have set these fields.
       * This is important because other modules (like VBO) might call
       * the driver function directly.
       */
      ASSERT(bufObj->Pointer == map);
      ASSERT(bufObj->Length == bufObj->Size);
      ASSERT(bufObj->Offset == 0);
      bufObj->AccessFlags = accessFlags;
   }

   if (access == GL_WRITE_ONLY_ARB || access == GL_READ_WRITE_ARB)
      bufObj->Written = GL_TRUE;

#ifdef VBO_DEBUG
   printf("glMapBufferARB(%u, sz %ld, access 0x%x)\n",
	  bufObj->Name, bufObj->Size, access);
   if (access == GL_WRITE_ONLY_ARB) {
      GLuint i;
      GLubyte *b = (GLubyte *) bufObj->Pointer;
      for (i = 0; i < bufObj->Size; i++)
         b[i] = i & 0xff;
   }
#endif

#ifdef BOUNDS_CHECK
   {
      GLubyte *buf = (GLubyte *) bufObj->Pointer;
      GLuint i;
      /* buffer is 100 bytes larger than requested, fill with magic value */
      for (i = 0; i < 100; i++) {
         buf[bufObj->Size - i - 1] = 123;
      }
   }
#endif

   return bufObj->Pointer;
}


GLboolean GLAPIENTRY
_mesa_UnmapBufferARB(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   GLboolean status = GL_TRUE;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   bufObj = get_buffer(ctx, "glUnmapBufferARB", target);
   if (!bufObj)
      return GL_FALSE;

   if (!_mesa_bufferobj_mapped(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUnmapBufferARB");
      return GL_FALSE;
   }

#ifdef BOUNDS_CHECK
   if (bufObj->Access != GL_READ_ONLY_ARB) {
      GLubyte *buf = (GLubyte *) bufObj->Pointer;
      GLuint i;
      /* check that last 100 bytes are still = magic value */
      for (i = 0; i < 100; i++) {
         GLuint pos = bufObj->Size - i - 1;
         if (buf[pos] != 123) {
            _mesa_warning(ctx, "Out of bounds buffer object write detected"
                          " at position %d (value = %u)\n",
                          pos, buf[pos]);
         }
      }
   }
#endif

#ifdef VBO_DEBUG
   if (bufObj->AccessFlags & GL_MAP_WRITE_BIT) {
      GLuint i, unchanged = 0;
      GLubyte *b = (GLubyte *) bufObj->Pointer;
      GLint pos = -1;
      /* check which bytes changed */
      for (i = 0; i < bufObj->Size - 1; i++) {
         if (b[i] == (i & 0xff) && b[i+1] == ((i+1) & 0xff)) {
            unchanged++;
            if (pos == -1)
               pos = i;
         }
      }
      if (unchanged) {
         printf("glUnmapBufferARB(%u): %u of %ld unchanged, starting at %d\n",
                      bufObj->Name, unchanged, bufObj->Size, pos);
      }
   }
#endif

   status = ctx->Driver.UnmapBuffer( ctx, bufObj );
   bufObj->AccessFlags = default_access_mode(ctx);
   ASSERT(bufObj->Pointer == NULL);
   ASSERT(bufObj->Offset == 0);
   ASSERT(bufObj->Length == 0);

   return status;
}


void GLAPIENTRY
_mesa_GetBufferParameterivARB(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   bufObj = get_buffer(ctx, "glGetBufferParameterivARB", target);
   if (!bufObj)
      return;

   switch (pname) {
   case GL_BUFFER_SIZE_ARB:
      *params = (GLint) bufObj->Size;
      return;
   case GL_BUFFER_USAGE_ARB:
      *params = bufObj->Usage;
      return;
   case GL_BUFFER_ACCESS_ARB:
      *params = simplified_access_mode(bufObj->AccessFlags);
      return;
   case GL_BUFFER_MAPPED_ARB:
      *params = _mesa_bufferobj_mapped(bufObj);
      return;
   case GL_BUFFER_ACCESS_FLAGS:
      if (!ctx->Extensions.ARB_map_buffer_range)
         goto invalid_pname;
      *params = bufObj->AccessFlags;
      return;
   case GL_BUFFER_MAP_OFFSET:
      if (!ctx->Extensions.ARB_map_buffer_range)
         goto invalid_pname;
      *params = (GLint) bufObj->Offset;
      return;
   case GL_BUFFER_MAP_LENGTH:
      if (!ctx->Extensions.ARB_map_buffer_range)
         goto invalid_pname;
      *params = (GLint) bufObj->Length;
      return;
   default:
      ; /* fall-through */
   }

invalid_pname:
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferParameterivARB(pname=%s)",
               _mesa_lookup_enum_by_nr(pname));
}


/**
 * New in GL 3.2
 * This is pretty much a duplicate of GetBufferParameteriv() but the
 * GL_BUFFER_SIZE_ARB attribute will be 64-bits on a 64-bit system.
 */
void GLAPIENTRY
_mesa_GetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   bufObj = get_buffer(ctx, "glGetBufferParameteri64v", target);
   if (!bufObj)
      return;

   switch (pname) {
   case GL_BUFFER_SIZE_ARB:
      *params = bufObj->Size;
      return;
   case GL_BUFFER_USAGE_ARB:
      *params = bufObj->Usage;
      return;
   case GL_BUFFER_ACCESS_ARB:
      *params = simplified_access_mode(bufObj->AccessFlags);
      return;
   case GL_BUFFER_ACCESS_FLAGS:
      if (!ctx->Extensions.ARB_map_buffer_range)
         goto invalid_pname;
      *params = bufObj->AccessFlags;
      return;
   case GL_BUFFER_MAPPED_ARB:
      *params = _mesa_bufferobj_mapped(bufObj);
      return;
   case GL_BUFFER_MAP_OFFSET:
      if (!ctx->Extensions.ARB_map_buffer_range)
         goto invalid_pname;
      *params = bufObj->Offset;
      return;
   case GL_BUFFER_MAP_LENGTH:
      if (!ctx->Extensions.ARB_map_buffer_range)
         goto invalid_pname;
      *params = bufObj->Length;
      return;
   default:
      ; /* fall-through */
   }

invalid_pname:
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferParameteri64v(pname=%s)",
               _mesa_lookup_enum_by_nr(pname));
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

   bufObj = get_buffer(ctx, "glGetBufferPointervARB", target);
   if (!bufObj)
      return;

   *params = bufObj->Pointer;
}

/**
 * See GL_ARB_map_buffer_range spec
 */
void * GLAPIENTRY
_mesa_MapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length,
                     GLbitfield access)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   void *map;

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, NULL);

   if (!ctx->Extensions.ARB_map_buffer_range) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(extension not supported)");
      return NULL;
   }

   if (offset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glMapBufferRange(offset = %ld)", (long)offset);
      return NULL;
   }

   if (length < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glMapBufferRange(length = %ld)", (long)length);
      return NULL;
   }

   if (access & ~(GL_MAP_READ_BIT |
                  GL_MAP_WRITE_BIT |
                  GL_MAP_INVALIDATE_RANGE_BIT |
                  GL_MAP_INVALIDATE_BUFFER_BIT |
                  GL_MAP_FLUSH_EXPLICIT_BIT |
                  GL_MAP_UNSYNCHRONIZED_BIT)) {
      /* generate an error if any undefind bit is set */
      _mesa_error(ctx, GL_INVALID_VALUE, "glMapBufferRange(access)");
      return NULL;
   }

   if ((access & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(access indicates neither read or write)");
      return NULL;
   }

   if ((access & GL_MAP_READ_BIT) &&
       (access & (GL_MAP_INVALIDATE_RANGE_BIT |
                  GL_MAP_INVALIDATE_BUFFER_BIT |
                  GL_MAP_UNSYNCHRONIZED_BIT))) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(invalid access flags)");
      return NULL;
   }

   if ((access & GL_MAP_FLUSH_EXPLICIT_BIT) &&
       ((access & GL_MAP_WRITE_BIT) == 0)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(invalid access flags)");
      return NULL;
   }

   bufObj = get_buffer(ctx, "glMapBufferRange", target);
   if (!bufObj)
      return NULL;

   if (offset + length > bufObj->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glMapBufferRange(offset + length > size)");
      return NULL;
   }

   if (_mesa_bufferobj_mapped(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(buffer already mapped)");
      return NULL;
   }

   if (!bufObj->Size) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glMapBufferRange(buffer size = 0)");
      return NULL;
   }

   /* Mapping zero bytes should return a non-null pointer. */
   if (!length) {
      static long dummy = 0;
      bufObj->Pointer = &dummy;
      bufObj->Length = length;
      bufObj->Offset = offset;
      bufObj->AccessFlags = access;
      return bufObj->Pointer;
   }

   ASSERT(ctx->Driver.MapBufferRange);
   map = ctx->Driver.MapBufferRange(ctx, offset, length, access, bufObj);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glMapBufferARB(map failed)");
   }
   else {
      /* The driver callback should have set all these fields.
       * This is important because other modules (like VBO) might call
       * the driver function directly.
       */
      ASSERT(bufObj->Pointer == map);
      ASSERT(bufObj->Length == length);
      ASSERT(bufObj->Offset == offset);
      ASSERT(bufObj->AccessFlags == access);
   }

   return map;
}


/**
 * See GL_ARB_map_buffer_range spec
 */
void GLAPIENTRY
_mesa_FlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.ARB_map_buffer_range) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glFlushMappedBufferRange(extension not supported)");
      return;
   }

   if (offset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glFlushMappedBufferRange(offset = %ld)", (long)offset);
      return;
   }

   if (length < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glFlushMappedBufferRange(length = %ld)", (long)length);
      return;
   }

   bufObj = get_buffer(ctx, "glFlushMappedBufferRange", target);
   if (!bufObj)
      return;

   if (!_mesa_bufferobj_mapped(bufObj)) {
      /* buffer is not mapped */
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glFlushMappedBufferRange(buffer is not mapped)");
      return;
   }

   if ((bufObj->AccessFlags & GL_MAP_FLUSH_EXPLICIT_BIT) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glFlushMappedBufferRange(GL_MAP_FLUSH_EXPLICIT_BIT not set)");
      return;
   }

   if (offset + length > bufObj->Length) {
      _mesa_error(ctx, GL_INVALID_VALUE,
		  "glFlushMappedBufferRange(offset %ld + length %ld > mapped length %ld)",
		  (long)offset, (long)length, (long)bufObj->Length);
      return;
   }

   ASSERT(bufObj->AccessFlags & GL_MAP_WRITE_BIT);

   if (ctx->Driver.FlushMappedBufferRange)
      ctx->Driver.FlushMappedBufferRange(ctx, offset, length, bufObj);
}

