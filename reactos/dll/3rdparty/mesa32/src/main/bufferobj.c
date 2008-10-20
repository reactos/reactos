/*
 * Mesa 3-D graphics library
 * Version:  7.2
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
#include "image.h"
#include "context.h"
#include "bufferobj.h"


/**
 * Get the buffer object bound to the specified target in a GL context.
 *
 * \param ctx     GL context
 * \param target  Buffer object target to be retrieved.  Currently this must
 *                be either \c GL_ARRAY_BUFFER or \c GL_ELEMENT_ARRAY_BUFFER.
 * \return   A pointer to the buffer object bound to \c target in the
 *           specified context or \c NULL if \c target is invalid.
 */
static INLINE struct gl_buffer_object *
get_buffer(GLcontext *ctx, GLenum target)
{
   struct gl_buffer_object * bufObj = NULL;

   switch (target) {
      case GL_ARRAY_BUFFER_ARB:
         bufObj = ctx->Array.ArrayBufferObj;
         break;
      case GL_ELEMENT_ARRAY_BUFFER_ARB:
         bufObj = ctx->Array.ElementArrayBufferObj;
         break;
      case GL_PIXEL_PACK_BUFFER_EXT:
         bufObj = ctx->Pack.BufferObj;
         break;
      case GL_PIXEL_UNPACK_BUFFER_EXT:
         bufObj = ctx->Unpack.BufferObj;
         break;
      default:
         /* error must be recorded by caller */
         return NULL;
   }

   /* bufObj should point to NullBufferObj or a user-created buffer object */
   ASSERT(bufObj);

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
 * \param caller  Name of calling function for recording errors.
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

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(target)", caller);
      return NULL;
   }
   if (bufObj->Name == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
      return NULL;
   }
   if (offset + size > bufObj->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
		  "%s(size + offset > buffer size)", caller);
      return NULL;
   }
   if (bufObj->Pointer) {
      /* Buffer is currently mapped */
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
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

   (void) ctx;

   obj = MALLOC_STRUCT(gl_buffer_object);
   _mesa_initialize_buffer_object(obj, name, target);
   return obj;
}


/**
 * Delete a buffer object.
 * 
 * This function is intended to be called via
 * \c dd_function_table::DeleteBuffer.
 */
void
_mesa_delete_buffer_object( GLcontext *ctx, struct gl_buffer_object *bufObj )
{
   (void) ctx;

   if (bufObj->Data)
      _mesa_free(bufObj->Data);

   /* assign strange values here to help w/ debugging */
   bufObj->RefCount = -1000;
   bufObj->Name = ~0;

   _mesa_free(bufObj);
}



/**
 * Set ptr to bufObj w/ reference counting.
 */
void
_mesa_reference_buffer_object(GLcontext *ctx,
                              struct gl_buffer_object **ptr,
                              struct gl_buffer_object *bufObj)
{
   if (*ptr == bufObj)
      return;

   if (*ptr) {
      /* Unreference the old texture */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_buffer_object *oldObj = *ptr;

      /*_glthread_LOCK_MUTEX(oldObj->Mutex);*/
      ASSERT(oldObj->RefCount > 0);
      oldObj->RefCount--;
#if 0
      printf("BufferObj %p %d DECR to %d\n",
             (void *) oldObj, oldObj->Name, oldObj->RefCount);
#endif
      deleteFlag = (oldObj->RefCount == 0);
      /*_glthread_UNLOCK_MUTEX(oldObj->Mutex);*/

      if (deleteFlag) {

         /* some sanity checking: don't delete a buffer still in use */
#if 0
         /* unfortunately, these tests are invalid during context tear-down */
	 ASSERT(ctx->Array.ArrayBufferObj != bufObj);
	 ASSERT(ctx->Array.ElementArrayBufferObj != bufObj);
	 ASSERT(ctx->Array.ArrayObj->Vertex.BufferObj != bufObj);
#endif

	 ASSERT(ctx->Driver.DeleteBuffer);
         ctx->Driver.DeleteBuffer(ctx, oldObj);
      }

      *ptr = NULL;
   }
   ASSERT(!*ptr);

   if (bufObj) {
      /* reference new texture */
      /*_glthread_LOCK_MUTEX(tex->Mutex);*/
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
      /*_glthread_UNLOCK_MUTEX(tex->Mutex);*/
   }
}


/**
 * Initialize a buffer object to default values.
 */
void
_mesa_initialize_buffer_object( struct gl_buffer_object *obj,
				GLuint name, GLenum target )
{
   (void) target;

   _mesa_bzero(obj, sizeof(struct gl_buffer_object));
   obj->RefCount = 1;
   obj->Name = name;
   obj->Usage = GL_STATIC_DRAW_ARB;
   obj->Access = GL_READ_WRITE_ARB;
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

   (void) ctx; (void) target;

   new_data = _mesa_realloc( bufObj->Data, bufObj->Size, size );
   if (new_data) {
      bufObj->Data = (GLubyte *) new_data;
      bufObj->Size = size;
      bufObj->Usage = usage;

      if (data) {
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
   (void) ctx; (void) target;

   /* this should have been caught in _mesa_BufferSubData() */
   ASSERT(size + offset <= bufObj->Size);

   if (bufObj->Data) {
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
   (void) ctx; (void) target;

   if (bufObj->Data && ((GLsizeiptrARB) (size + offset) <= bufObj->Size)) {
      _mesa_memcpy( data, (GLubyte *) bufObj->Data + offset, size );
   }
}


/**
 * Fallback function called via ctx->Driver.MapBuffer().
 * Hardware drivers that really implement buffer objects should never use
 * this function.
 *
 * The function parameters will have been already tested for errors.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param access  Information about how the buffer will be accessed.
 * \param bufObj  Object to be mapped.
 * \return  A pointer to the object's internal data store that can be accessed
 *          by the processor
 *
 * \sa glMapBufferARB, dd_function_table::MapBuffer
 */
void *
_mesa_buffer_map( GLcontext *ctx, GLenum target, GLenum access,
		  struct gl_buffer_object *bufObj )
{
   (void) ctx;
   (void) target;
   (void) access;
   ASSERT(!bufObj->OnCard);
   /* Just return a direct pointer to the data */
   if (bufObj->Pointer) {
      /* already mapped! */
      return NULL;
   }
   bufObj->Pointer = bufObj->Data;
   return bufObj->Pointer;
}


/**
 * Fallback function called via ctx->Driver.MapBuffer().
 * Hardware drivers that really implement buffer objects should never use
 * function.
 *
 * The input parameters will have been already tested for errors.
 *
 * \sa glUnmapBufferARB, dd_function_table::UnmapBuffer
 */
GLboolean
_mesa_buffer_unmap( GLcontext *ctx, GLenum target,
                    struct gl_buffer_object *bufObj )
{
   (void) ctx;
   (void) target;
   ASSERT(!bufObj->OnCard);
   /* XXX we might assert here that bufObj->Pointer is non-null */
   bufObj->Pointer = NULL;
   return GL_TRUE;
}


/**
 * Initialize the state associated with buffer objects
 */
void
_mesa_init_buffer_objects( GLcontext *ctx )
{
   /* Allocate the default buffer object and set refcount so high that
    * it never gets deleted.
    * XXX with recent/improved refcounting this may not longer be needed.
    */
   ctx->Array.NullBufferObj = _mesa_new_buffer_object(ctx, 0, 0);
   if (ctx->Array.NullBufferObj)
      ctx->Array.NullBufferObj->RefCount = 1000;

   ctx->Array.ArrayBufferObj = ctx->Array.NullBufferObj;
   ctx->Array.ElementArrayBufferObj = ctx->Array.NullBufferObj;
}

/**
 * Bind the specified target to buffer for the specified context.
 */
static void
bind_buffer_object(GLcontext *ctx, GLenum target, GLuint buffer)
{
   struct gl_buffer_object *oldBufObj;
   struct gl_buffer_object *newBufObj = NULL;
   struct gl_buffer_object **bindTarget = NULL;

   switch (target) {
   case GL_ARRAY_BUFFER_ARB:
      bindTarget = &ctx->Array.ArrayBufferObj;
      break;
   case GL_ELEMENT_ARRAY_BUFFER_ARB:
      bindTarget = &ctx->Array.ElementArrayBufferObj;
      break;
   case GL_PIXEL_PACK_BUFFER_EXT:
      bindTarget = &ctx->Pack.BufferObj;
      break;
   case GL_PIXEL_UNPACK_BUFFER_EXT:
      bindTarget = &ctx->Unpack.BufferObj;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferARB(target)");
      return;
   }

   /* Get pointer to old buffer object (to be unbound) */
   oldBufObj = get_buffer(ctx, target);
   if (oldBufObj && oldBufObj->Name == buffer)
      return;   /* rebinding the same buffer object- no change */

   /*
    * Get pointer to new buffer object (newBufObj)
    */
   if (buffer == 0) {
      /* The spec says there's not a buffer object named 0, but we use
       * one internally because it simplifies things.
       */
      newBufObj = ctx->Array.NullBufferObj;
   }
   else {
      /* non-default buffer object */
      newBufObj = _mesa_lookup_bufferobj(ctx, buffer);
      if (!newBufObj) {
         /* if this is a new buffer object id, allocate a buffer object now */
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
   if (ctx->Driver.BindBuffer && newBufObj)
      ctx->Driver.BindBuffer( ctx, target, newBufObj );
}


/**
 * Update the default buffer objects in the given context to reference those
 * specified in the shared state and release those referencing the old 
 * shared state.
 */
void
_mesa_update_default_objects_buffer_objects(GLcontext *ctx)
{
   /* Bind the NullBufferObj to remove references to those
    * in the shared context hash table.
    */
   bind_buffer_object( ctx, GL_ARRAY_BUFFER_ARB, 0);
   bind_buffer_object( ctx, GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
   bind_buffer_object( ctx, GL_PIXEL_PACK_BUFFER_ARB, 0);
   bind_buffer_object( ctx, GL_PIXEL_UNPACK_BUFFER_ARB, 0);
}


/**
 * When we're about to read pixel data out of a PBO (via glDrawPixels,
 * glTexImage, etc) or write data into a PBO (via glReadPixels,
 * glGetTexImage, etc) we call this function to check that we're not
 * going to read out of bounds.
 *
 * XXX This would also be a convenient time to check that the PBO isn't
 * currently mapped.  Whoever calls this function should check for that.
 * Remember, we can't use a PBO when it's mapped!
 *
 * \param width  width of image to read/write
 * \param height  height of image to read/write
 * \param depth  depth of image to read/write
 * \param format  format of image to read/write
 * \param type  datatype of image to read/write
 * \param ptr  the user-provided pointer/offset
 * \return GL_TRUE if the PBO access is OK, GL_FALSE if the access would
 *         go out of bounds.
 */
GLboolean
_mesa_validate_pbo_access(GLuint dimensions,
                          const struct gl_pixelstore_attrib *pack,
                          GLsizei width, GLsizei height, GLsizei depth,
                          GLenum format, GLenum type, const GLvoid *ptr)
{
   GLvoid *start, *end;
   const GLubyte *sizeAddr; /* buffer size, cast to a pointer */

   ASSERT(pack->BufferObj->Name != 0);

   if (pack->BufferObj->Size == 0)
      /* no buffer! */
      return GL_FALSE;

   /* get address of first pixel we'll read */
   start = _mesa_image_address(dimensions, pack, ptr, width, height,
                               format, type, 0, 0, 0);

   /* get address just past the last pixel we'll read */
   end =  _mesa_image_address(dimensions, pack, ptr, width, height,
                              format, type, depth-1, height-1, width);


   sizeAddr = ((const GLubyte *) 0) + pack->BufferObj->Size;

   if ((const GLubyte *) start > sizeAddr) {
      /* This will catch negative values / wrap-around */
      return GL_FALSE;
   }
   if ((const GLubyte *) end > sizeAddr) {
      /* Image read goes beyond end of buffer */
      return GL_FALSE;
   }

   /* OK! */
   return GL_TRUE;
}


/**
 * If the source of glBitmap data is a PBO, check that we won't read out
 * of buffer bounds, then map the buffer.
 * If not sourcing from a PBO, just return the bitmap pointer.
 * This is a helper function for (some) drivers.
 * Return NULL if error.
 * If non-null return, must call _mesa_unmap_bitmap_pbo() when done.
 */
const GLubyte *
_mesa_map_bitmap_pbo(GLcontext *ctx,
                     const struct gl_pixelstore_attrib *unpack,
                     const GLubyte *bitmap)
{
   const GLubyte *buf;

   if (unpack->BufferObj->Name) {
      /* unpack from PBO */
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                              GL_READ_ONLY_ARB,
                                              unpack->BufferObj);
      if (!buf)
         return NULL;

      buf = ADD_POINTERS(buf, bitmap);
   }
   else {
      /* unpack from normal memory */
      buf = bitmap;
   }

   return buf;
}


/**
 * Counterpart to _mesa_map_bitmap_pbo()
 * This is a helper function for (some) drivers.
 */
void
_mesa_unmap_bitmap_pbo(GLcontext *ctx,
                       const struct gl_pixelstore_attrib *unpack)
{
   if (unpack->BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              unpack->BufferObj);
   }
}


/**
 * \sa _mesa_map_bitmap_pbo
 */
const GLvoid *
_mesa_map_drawpix_pbo(GLcontext *ctx,
                      const struct gl_pixelstore_attrib *unpack,
                      const GLvoid *pixels)
{
   const GLvoid *buf;

   if (unpack->BufferObj->Name) {
      /* unpack from PBO */
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                              GL_READ_ONLY_ARB,
                                              unpack->BufferObj);
      if (!buf)
         return NULL;

      buf = ADD_POINTERS(buf, pixels);
   }
   else {
      /* unpack from normal memory */
      buf = pixels;
   }

   return buf;
}


/**
 * \sa _mesa_unmap_bitmap_pbo
 */
void
_mesa_unmap_drapix_pbo(GLcontext *ctx,
                       const struct gl_pixelstore_attrib *unpack)
{
   if (unpack->BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              unpack->BufferObj);
   }
}


/**
 * If PBO is bound, map the buffer, return dest pointer in mapped buffer.
 * Call _mesa_unmap_readpix_pbo() when finished
 * \return NULL if error
 */
void *
_mesa_map_readpix_pbo(GLcontext *ctx,
                      const struct gl_pixelstore_attrib *pack,
                      GLvoid *dest)
{
   void *buf;

   if (pack->BufferObj->Name) {
      /* pack into PBO */
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              pack->BufferObj);
      if (!buf)
         return NULL;

      buf = ADD_POINTERS(buf, dest);
   }
   else {
      /* pack to normal memory */
      buf = dest;
   }

   return buf;
}


/**
 * Counterpart to _mesa_map_readpix_pbo()
 */
void
_mesa_unmap_readpix_pbo(GLcontext *ctx,
                        const struct gl_pixelstore_attrib *pack)
{
   if (pack->BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT, pack->BufferObj);
   }
}



/**
 * Return the gl_buffer_object for the given ID.
 * Always return NULL for ID 0.
 */
struct gl_buffer_object *
_mesa_lookup_bufferobj(GLcontext *ctx, GLuint buffer)
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
unbind(GLcontext *ctx,
       struct gl_buffer_object **ptr,
       struct gl_buffer_object *obj)
{
   if (*ptr == obj) {
      _mesa_reference_buffer_object(ctx, ptr, ctx->Array.NullBufferObj);
   }
}



/**********************************************************************/
/* API Functions                                                      */
/**********************************************************************/

void GLAPIENTRY
_mesa_BindBufferARB(GLenum target, GLuint buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

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

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteBuffersARB(n)");
      return;
   }

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   for (i = 0; i < n; i++) {
      struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, ids[i]);
      if (bufObj) {
         /* unbind any vertex pointers bound to this buffer */
         GLuint j;

         ASSERT(bufObj->Name == ids[i]);

         unbind(ctx, &ctx->Array.ArrayObj->Vertex.BufferObj, bufObj);
         unbind(ctx, &ctx->Array.ArrayObj->Normal.BufferObj, bufObj);
         unbind(ctx, &ctx->Array.ArrayObj->Color.BufferObj, bufObj);
         unbind(ctx, &ctx->Array.ArrayObj->SecondaryColor.BufferObj, bufObj);
         unbind(ctx, &ctx->Array.ArrayObj->FogCoord.BufferObj, bufObj);
         unbind(ctx, &ctx->Array.ArrayObj->Index.BufferObj, bufObj);
         unbind(ctx, &ctx->Array.ArrayObj->EdgeFlag.BufferObj, bufObj);
         for (j = 0; j < MAX_TEXTURE_UNITS; j++) {
            unbind(ctx, &ctx->Array.ArrayObj->TexCoord[j].BufferObj, bufObj);
         }
         for (j = 0; j < VERT_ATTRIB_MAX; j++) {
            unbind(ctx, &ctx->Array.ArrayObj->VertexAttrib[j].BufferObj, bufObj);
         }

         if (ctx->Array.ArrayBufferObj == bufObj) {
            _mesa_BindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
         }
         if (ctx->Array.ElementArrayBufferObj == bufObj) {
            _mesa_BindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
         }

         if (ctx->Pack.BufferObj == bufObj) {
            _mesa_BindBufferARB( GL_PIXEL_PACK_BUFFER_EXT, 0 );
         }
         if (ctx->Unpack.BufferObj == bufObj) {
            _mesa_BindBufferARB( GL_PIXEL_UNPACK_BUFFER_EXT, 0 );
         }

         /* The ID is immediately freed for re-use */
         _mesa_HashRemove(ctx->Shared->BufferObjects, bufObj->Name);
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

   /* Allocate new, empty buffer objects and return identifiers */
   for (i = 0; i < n; i++) {
      struct gl_buffer_object *bufObj;
      GLuint name = first + i;
      GLenum target = 0;
      bufObj = ctx->Driver.NewBufferObject( ctx, name, target );
      if (!bufObj) {
         _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenBuffersARB");
         return;
      }
      _mesa_HashInsert(ctx->Shared->BufferObjects, first + i, bufObj);
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

   return bufObj ? GL_TRUE : GL_FALSE;
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

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBufferDataARB(target)" );
      return;
   }
   if (bufObj->Name == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBufferDataARB" );
      return;
   }
   
   if (bufObj->Pointer) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBufferDataARB(buffer is mapped)" );
      return;
   }  

   ASSERT(ctx->Driver.BufferData);

   /* Give the buffer object to the driver!  <data> may be null! */
   ctx->Driver.BufferData( ctx, target, size, data, usage, bufObj );
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

   ASSERT(ctx->Driver.BufferSubData);
   ctx->Driver.BufferSubData( ctx, target, offset, size, data, bufObj );
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
   ctx->Driver.GetBufferSubData( ctx, target, offset, size, data, bufObj );
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

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMapBufferARB(target)" );
      return NULL;
   }
   if (bufObj->Name == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glMapBufferARB" );
      return NULL;
   }
   if (bufObj->Pointer) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glMapBufferARB(already mapped)");
      return NULL;
   }

   ASSERT(ctx->Driver.MapBuffer);
   bufObj->Pointer = ctx->Driver.MapBuffer( ctx, target, access, bufObj );
   if (!bufObj->Pointer) {
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

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glUnmapBufferARB(target)" );
      return GL_FALSE;
   }
   if (bufObj->Name == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUnmapBufferARB" );
      return GL_FALSE;
   }
   if (!bufObj->Pointer) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUnmapBufferARB");
      return GL_FALSE;
   }

   if (ctx->Driver.UnmapBuffer) {
      status = ctx->Driver.UnmapBuffer( ctx, target, bufObj );
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

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "GetBufferParameterivARB(target)" );
      return;
   }
   if (bufObj->Name == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "GetBufferParameterivARB" );
      return;
   }

   switch (pname) {
      case GL_BUFFER_SIZE_ARB:
         *params = (GLint) bufObj->Size;
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

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferPointervARB(target)" );
      return;
   }
   if (bufObj->Name == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetBufferPointervARB" );
      return;
   }

   *params = bufObj->Pointer;
}
