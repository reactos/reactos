/*
 * Mesa 3-D graphics library
 * Version:  7.2
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * (C) Copyright IBM Corporation 2006
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
 * BRIAN PAUL OR IBM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


/**
 * \file arrayobj.c
 * Functions for the GL_APPLE_vertex_array_object extension.
 *
 * \todo
 * The code in this file borrows a lot from bufferobj.c.  There's a certain
 * amount of cruft left over from that origin that may be unnecessary.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 * \author Brian Paul
 */


#include "glheader.h"
#include "hash.h"
#include "imports.h"
#include "context.h"
#if FEATURE_ARB_vertex_buffer_object
#include "bufferobj.h"
#endif
#include "arrayobj.h"
#include "glapi/dispatch.h"


/**
 * Look up the array object for the given ID.
 * 
 * \returns
 * Either a pointer to the array object with the specified ID or \c NULL for
 * a non-existent ID.  The spec defines ID 0 as being technically
 * non-existent.
 */

static INLINE struct gl_array_object *
lookup_arrayobj(GLcontext *ctx, GLuint id)
{
   return (id == 0) 
     ? NULL 
     : (struct gl_array_object *) _mesa_HashLookup(ctx->Shared->ArrayObjects,
						   id);
}


/**
 * Allocate and initialize a new vertex array object.
 * 
 * This function is intended to be called via
 * \c dd_function_table::NewArrayObject.
 */
struct gl_array_object *
_mesa_new_array_object( GLcontext *ctx, GLuint name )
{
   struct gl_array_object *obj = CALLOC_STRUCT(gl_array_object);
   if (obj)
      _mesa_initialize_array_object(ctx, obj, name);
   return obj;
}


/**
 * Delete an array object.
 * 
 * This function is intended to be called via
 * \c dd_function_table::DeleteArrayObject.
 */
void
_mesa_delete_array_object( GLcontext *ctx, struct gl_array_object *obj )
{
   (void) ctx;
   _mesa_free(obj);
}


void
_mesa_initialize_array_object( GLcontext *ctx,
			       struct gl_array_object *obj,
			       GLuint name )
{
   GLuint i;

   obj->Name = name;

   /* Vertex arrays */
   obj->Vertex.Size = 4;
   obj->Vertex.Type = GL_FLOAT;
   obj->Vertex.Stride = 0;
   obj->Vertex.StrideB = 0;
   obj->Vertex.Ptr = NULL;
   obj->Vertex.Enabled = GL_FALSE;
   obj->Normal.Type = GL_FLOAT;
   obj->Normal.Stride = 0;
   obj->Normal.StrideB = 0;
   obj->Normal.Ptr = NULL;
   obj->Normal.Enabled = GL_FALSE;
   obj->Color.Size = 4;
   obj->Color.Type = GL_FLOAT;
   obj->Color.Stride = 0;
   obj->Color.StrideB = 0;
   obj->Color.Ptr = NULL;
   obj->Color.Enabled = GL_FALSE;
   obj->SecondaryColor.Size = 4;
   obj->SecondaryColor.Type = GL_FLOAT;
   obj->SecondaryColor.Stride = 0;
   obj->SecondaryColor.StrideB = 0;
   obj->SecondaryColor.Ptr = NULL;
   obj->SecondaryColor.Enabled = GL_FALSE;
   obj->FogCoord.Size = 1;
   obj->FogCoord.Type = GL_FLOAT;
   obj->FogCoord.Stride = 0;
   obj->FogCoord.StrideB = 0;
   obj->FogCoord.Ptr = NULL;
   obj->FogCoord.Enabled = GL_FALSE;
   obj->Index.Type = GL_FLOAT;
   obj->Index.Stride = 0;
   obj->Index.StrideB = 0;
   obj->Index.Ptr = NULL;
   obj->Index.Enabled = GL_FALSE;
   for (i = 0; i < MAX_TEXTURE_COORD_UNITS; i++) {
      obj->TexCoord[i].Size = 4;
      obj->TexCoord[i].Type = GL_FLOAT;
      obj->TexCoord[i].Stride = 0;
      obj->TexCoord[i].StrideB = 0;
      obj->TexCoord[i].Ptr = NULL;
      obj->TexCoord[i].Enabled = GL_FALSE;
   }
   obj->EdgeFlag.Stride = 0;
   obj->EdgeFlag.StrideB = 0;
   obj->EdgeFlag.Ptr = NULL;
   obj->EdgeFlag.Enabled = GL_FALSE;
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      obj->VertexAttrib[i].Size = 4;
      obj->VertexAttrib[i].Type = GL_FLOAT;
      obj->VertexAttrib[i].Stride = 0;
      obj->VertexAttrib[i].StrideB = 0;
      obj->VertexAttrib[i].Ptr = NULL;
      obj->VertexAttrib[i].Enabled = GL_FALSE;
      obj->VertexAttrib[i].Normalized = GL_FALSE;
   }

#if FEATURE_point_size_array
   obj->PointSize.Type = GL_FLOAT;
   obj->PointSize.Stride = 0;
   obj->PointSize.StrideB = 0;
   obj->PointSize.Ptr = NULL;
   obj->PointSize.Enabled = GL_FALSE;
   obj->PointSize.BufferObj = ctx->Array.NullBufferObj;
#endif

#if FEATURE_ARB_vertex_buffer_object
   /* Vertex array buffers */
   obj->Vertex.BufferObj = ctx->Array.NullBufferObj;
   obj->Normal.BufferObj = ctx->Array.NullBufferObj;
   obj->Color.BufferObj = ctx->Array.NullBufferObj;
   obj->SecondaryColor.BufferObj = ctx->Array.NullBufferObj;
   obj->FogCoord.BufferObj = ctx->Array.NullBufferObj;
   obj->Index.BufferObj = ctx->Array.NullBufferObj;
   for (i = 0; i < MAX_TEXTURE_COORD_UNITS; i++) {
      obj->TexCoord[i].BufferObj = ctx->Array.NullBufferObj;
   }
   obj->EdgeFlag.BufferObj = ctx->Array.NullBufferObj;
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      obj->VertexAttrib[i].BufferObj = ctx->Array.NullBufferObj;
   }
#endif
}


/**
 * Add the given array object to the array object pool.
 */
void
_mesa_save_array_object( GLcontext *ctx, struct gl_array_object *obj )
{
   if (obj->Name > 0) {
      /* insert into hash table */
      _mesa_HashInsert(ctx->Shared->ArrayObjects, obj->Name, obj);
   }
}


/**
 * Remove the given array object from the array object pool.
 * Do not deallocate the array object though.
 */
void
_mesa_remove_array_object( GLcontext *ctx, struct gl_array_object *obj )
{
   if (obj->Name > 0) {
      /* remove from hash table */
      _mesa_HashRemove(ctx->Shared->ArrayObjects, obj->Name);
   }
}


static void
unbind_buffer_object( GLcontext *ctx, struct gl_buffer_object *bufObj )
{
   if (bufObj != ctx->Array.NullBufferObj) {
      _mesa_reference_buffer_object(ctx, &bufObj, NULL);
   }
}


/**********************************************************************/
/* API Functions                                                      */
/**********************************************************************/

/**
 * Bind a new array.
 *
 * \todo
 * The binding could be done more efficiently by comparing the non-NULL
 * pointers in the old and new objects.  The only arrays that are "dirty" are
 * the ones that are non-NULL in either object.
 */
void GLAPIENTRY
_mesa_BindVertexArrayAPPLE( GLuint id )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_array_object * const oldObj = ctx->Array.ArrayObj;
   struct gl_array_object *newObj = NULL;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   ASSERT(oldObj != NULL);

   if ( oldObj->Name == id )
      return;   /* rebinding the same array object- no change */

   /*
    * Get pointer to new array object (newBufObj)
    */
   if (id == 0) {
      /* The spec says there is no array object named 0, but we use
       * one internally because it simplifies things.
       */
      newObj = ctx->Array.DefaultArrayObj;
   }
   else {
      /* non-default array object */
      newObj = lookup_arrayobj(ctx, id);
      if (!newObj) {
         /* If this is a new array object id, allocate an array object now.
	  */

	 newObj = (*ctx->Driver.NewArrayObject)(ctx, id);
         if (!newObj) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindVertexArrayAPPLE");
            return;
         }
         _mesa_save_array_object(ctx, newObj);
      }
   }


   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_ALL;
   ctx->Array.ArrayObj = newObj;


   /* Pass BindVertexArray call to device driver */
   if (ctx->Driver.BindArrayObject && newObj)
      (*ctx->Driver.BindArrayObject)( ctx, newObj );
}


/**
 * Delete a set of array objects.
 * 
 * \param n      Number of array objects to delete.
 * \param ids    Array of \c n array object IDs.
 */
void GLAPIENTRY
_mesa_DeleteVertexArraysAPPLE(GLsizei n, const GLuint *ids)
{
   GET_CURRENT_CONTEXT(ctx);
   GLsizei i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteVertexArrayAPPLE(n)");
      return;
   }

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   for (i = 0; i < n; i++) {
      struct gl_array_object *obj = lookup_arrayobj(ctx, ids[i]);

      if ( obj != NULL ) {
	 ASSERT( obj->Name == ids[i] );


	 /* If the array object is currently bound, the spec says "the binding
	  * for that object reverts to zero and the default vertex array
	  * becomes current."
	  */
	 if ( obj == ctx->Array.ArrayObj ) {
	    CALL_BindVertexArrayAPPLE( ctx->Exec, (0) );
	 }

#if FEATURE_ARB_vertex_buffer_object
	 /* Unbind any buffer objects that might be bound to arrays in
	  * this array object.
	  */
	 unbind_buffer_object( ctx, obj->Vertex.BufferObj );
	 unbind_buffer_object( ctx, obj->Normal.BufferObj );
	 unbind_buffer_object( ctx, obj->Color.BufferObj );
	 unbind_buffer_object( ctx, obj->SecondaryColor.BufferObj );
	 unbind_buffer_object( ctx, obj->FogCoord.BufferObj );
	 unbind_buffer_object( ctx, obj->Index.BufferObj );
	 for (i = 0; i < MAX_TEXTURE_COORD_UNITS; i++) {
	    unbind_buffer_object( ctx, obj->TexCoord[i].BufferObj );
	 }
	 unbind_buffer_object( ctx, obj->EdgeFlag.BufferObj );
	 for (i = 0; i < VERT_ATTRIB_MAX; i++) {
	    unbind_buffer_object( ctx, obj->VertexAttrib[i].BufferObj );
	 }
#endif

	 /* The ID is immediately freed for re-use */
	 _mesa_remove_array_object(ctx, obj);
	 ctx->Driver.DeleteArrayObject(ctx, obj);
      }
   }

   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
}


/**
 * Generate a set of unique array object IDs and store them in \c arrays.
 * 
 * \param n       Number of IDs to generate.
 * \param arrays  Array of \c n locations to store the IDs.
 */
void GLAPIENTRY
_mesa_GenVertexArraysAPPLE(GLsizei n, GLuint *arrays)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenVertexArraysAPPLE");
      return;
   }

   if (!arrays) {
      return;
   }

   /*
    * This must be atomic (generation and allocation of array object IDs)
    */
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->ArrayObjects, n);

   /* Allocate new, empty array objects and return identifiers */
   for (i = 0; i < n; i++) {
      struct gl_array_object *obj;
      GLuint name = first + i;

      obj = (*ctx->Driver.NewArrayObject)( ctx, name );
      if (!obj) {
         _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenVertexArraysAPPLE");
         return;
      }
      _mesa_save_array_object(ctx, obj);
      arrays[i] = first + i;
   }

   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
}


/**
 * Determine if ID is the name of an array object.
 * 
 * \param id  ID of the potential array object.
 * \return  \c GL_TRUE if \c id is the name of a array object, 
 *          \c GL_FALSE otherwise.
 */
GLboolean GLAPIENTRY
_mesa_IsVertexArrayAPPLE( GLuint id )
{
   struct gl_array_object * obj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (id == 0)
      return GL_FALSE;

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   obj = lookup_arrayobj(ctx, id);
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   return (obj != NULL) ? GL_TRUE : GL_FALSE;
}
