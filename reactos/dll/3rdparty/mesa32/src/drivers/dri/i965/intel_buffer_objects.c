/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "imports.h"
#include "mtypes.h"
#include "bufferobj.h"

#include "intel_context.h"
#include "intel_buffer_objects.h"
#include "bufmgr.h"


/**
 * There is some duplication between mesa's bufferobjects and our
 * bufmgr buffers.  Both have an integer handle and a hashtable to
 * lookup an opaque structure.  It would be nice if the handles and
 * internal structure where somehow shared.
 */
static struct gl_buffer_object *intel_bufferobj_alloc( GLcontext *ctx, 
						       GLuint name, 
						       GLenum target )
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *obj = MALLOC_STRUCT(intel_buffer_object);

   _mesa_initialize_buffer_object(&obj->Base, name, target);

   /* XXX:  We generate our own handle, which is different to 'name' above.
    */
   bmGenBuffers(intel, "bufferobj", 1, &obj->buffer, 6);
   assert(obj->buffer);

   return &obj->Base;
}


/**
 * Deallocate/free a vertex/pixel buffer object.
 * Called via glDeleteBuffersARB().
 */
static void intel_bufferobj_free( GLcontext *ctx, 
				  struct gl_buffer_object *obj )
{ 
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);

   if (intel_obj->buffer) 
      bmDeleteBuffers( intel, 1, &intel_obj->buffer );
  
   _mesa_free(intel_obj);
}



/**
 * Allocate space for and store data in a buffer object.  Any data that was
 * previously stored in the buffer object is lost.  If data is NULL,
 * memory will be allocated, but no copy will occur.
 * Called via glBufferDataARB().
 */
static void intel_bufferobj_data( GLcontext *ctx, 
				  GLenum target, 
				  GLsizeiptrARB size,
				  const GLvoid *data, 
				  GLenum usage,
				  struct gl_buffer_object *obj )
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   /* XXX: do something useful with 'usage' (eg. populate flags
    * argument below)
    */
   assert(intel_obj);

   obj->Size = size;
   obj->Usage = usage;

   bmBufferDataAUB(intel, intel_obj->buffer, size, data, 0,
		   0, 0);
}


/**
 * Replace data in a subrange of buffer object.  If the data range
 * specified by size + offset extends beyond the end of the buffer or
 * if data is NULL, no copy is performed.
 * Called via glBufferSubDataARB().
 */
static void intel_bufferobj_subdata( GLcontext *ctx, 
				     GLenum target, 
				     GLintptrARB offset,
				     GLsizeiptrARB size, 
				     const GLvoid * data,
				     struct gl_buffer_object * obj )
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);
   bmBufferSubDataAUB(intel, intel_obj->buffer, offset, size, data, 0, 0);
}


/**
 * Called via glGetBufferSubDataARB().
 */
static void intel_bufferobj_get_subdata( GLcontext *ctx, 
					 GLenum target, 
					 GLintptrARB offset,
					 GLsizeiptrARB size, 
					 GLvoid * data,
					 struct gl_buffer_object * obj )
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);
   bmBufferGetSubData(intel, intel_obj->buffer, offset, size, data);
}



/**
 * Called via glMapBufferARB().
 */
static void *intel_bufferobj_map( GLcontext *ctx, 
				  GLenum target, 
				  GLenum access,
				  struct gl_buffer_object *obj )
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   /* XXX: Translate access to flags arg below:
    */
   assert(intel_obj);
   assert(intel_obj->buffer);
   obj->Pointer = bmMapBuffer(intel, intel_obj->buffer, 0);
   return obj->Pointer;
}


/**
 * Called via glMapBufferARB().
 */
static GLboolean intel_bufferobj_unmap( GLcontext *ctx,
					GLenum target,
					struct gl_buffer_object *obj )
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);
   assert(intel_obj->buffer);
   assert(obj->Pointer);
   bmUnmapBufferAUB(intel, intel_obj->buffer, 0, 0);
   obj->Pointer = NULL;
   return GL_TRUE;
}

struct buffer *intel_bufferobj_buffer( const struct intel_buffer_object *intel_obj )
{
   assert(intel_obj->Base.Name);
   assert(intel_obj->buffer);
   return intel_obj->buffer;
}  

void intel_bufferobj_init( struct intel_context *intel )
{
   GLcontext *ctx = &intel->ctx;

   ctx->Driver.NewBufferObject = intel_bufferobj_alloc;
   ctx->Driver.DeleteBuffer = intel_bufferobj_free;
   ctx->Driver.BufferData = intel_bufferobj_data;
   ctx->Driver.BufferSubData = intel_bufferobj_subdata;
   ctx->Driver.GetBufferSubData = intel_bufferobj_get_subdata;
   ctx->Driver.MapBuffer = intel_bufferobj_map;
   ctx->Driver.UnmapBuffer = intel_bufferobj_unmap;
}
