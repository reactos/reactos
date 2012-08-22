/*
 * Mesa 3-D graphics library
 * Version:  7.6
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * (C) Copyright IBM Corporation 2006
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
#include "image.h"
#include "imports.h"
#include "context.h"
#include "mfeatures.h"
#include "bufferobj.h"
#include "arrayobj.h"
#include "macros.h"
#include "mtypes.h"
#include "varray.h"
#include "main/dispatch.h"


/**
 * Look up the array object for the given ID.
 * 
 * \returns
 * Either a pointer to the array object with the specified ID or \c NULL for
 * a non-existent ID.  The spec defines ID 0 as being technically
 * non-existent.
 */

static inline struct gl_array_object *
lookup_arrayobj(struct gl_context *ctx, GLuint id)
{
   if (id == 0)
      return NULL;
   else
      return (struct gl_array_object *)
         _mesa_HashLookup(ctx->Array.Objects, id);
}


/**
 * For all the vertex arrays in the array object, unbind any pointers
 * to any buffer objects (VBOs).
 * This is done just prior to array object destruction.
 */
static void
unbind_array_object_vbos(struct gl_context *ctx, struct gl_array_object *obj)
{
   GLuint i;

   for (i = 0; i < Elements(obj->VertexAttrib); i++)
      _mesa_reference_buffer_object(ctx, &obj->VertexAttrib[i].BufferObj, NULL);
}


/**
 * Allocate and initialize a new vertex array object.
 * 
 * This function is intended to be called via
 * \c dd_function_table::NewArrayObject.
 */
struct gl_array_object *
_mesa_new_array_object( struct gl_context *ctx, GLuint name )
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
_mesa_delete_array_object( struct gl_context *ctx, struct gl_array_object *obj )
{
   (void) ctx;
   unbind_array_object_vbos(ctx, obj);
   _mesa_reference_buffer_object(ctx, &obj->ElementArrayBufferObj, NULL);
   _glthread_DESTROY_MUTEX(obj->Mutex);
   free(obj);
}


/**
 * Set ptr to arrayObj w/ reference counting.
 */
void
_mesa_reference_array_object(struct gl_context *ctx,
                             struct gl_array_object **ptr,
                             struct gl_array_object *arrayObj)
{
   if (*ptr == arrayObj)
      return;

   if (*ptr) {
      /* Unreference the old array object */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_array_object *oldObj = *ptr;

      _glthread_LOCK_MUTEX(oldObj->Mutex);
      ASSERT(oldObj->RefCount > 0);
      oldObj->RefCount--;
#if 0
      printf("ArrayObj %p %d DECR to %d\n",
             (void *) oldObj, oldObj->Name, oldObj->RefCount);
#endif
      deleteFlag = (oldObj->RefCount == 0);
      _glthread_UNLOCK_MUTEX(oldObj->Mutex);

      if (deleteFlag) {
	 ASSERT(ctx->Driver.DeleteArrayObject);
         ctx->Driver.DeleteArrayObject(ctx, oldObj);
      }

      *ptr = NULL;
   }
   ASSERT(!*ptr);

   if (arrayObj) {
      /* reference new array object */
      _glthread_LOCK_MUTEX(arrayObj->Mutex);
      if (arrayObj->RefCount == 0) {
         /* this array's being deleted (look just above) */
         /* Not sure this can every really happen.  Warn if it does. */
         _mesa_problem(NULL, "referencing deleted array object");
         *ptr = NULL;
      }
      else {
         arrayObj->RefCount++;
#if 0
         printf("ArrayObj %p %d INCR to %d\n",
                (void *) arrayObj, arrayObj->Name, arrayObj->RefCount);
#endif
         *ptr = arrayObj;
      }
      _glthread_UNLOCK_MUTEX(arrayObj->Mutex);
   }
}



static void
init_array(struct gl_context *ctx,
           struct gl_client_array *array, GLint size, GLint type)
{
   array->Size = size;
   array->Type = type;
   array->Format = GL_RGBA; /* only significant for GL_EXT_vertex_array_bgra */
   array->Stride = 0;
   array->StrideB = 0;
   array->Ptr = NULL;
   array->Enabled = GL_FALSE;
   array->Normalized = GL_FALSE;
   array->Integer = GL_FALSE;
   array->_ElementSize = size * _mesa_sizeof_type(type);
   /* Vertex array buffers */
   _mesa_reference_buffer_object(ctx, &array->BufferObj,
                                 ctx->Shared->NullBufferObj);
}


/**
 * Initialize a gl_array_object's arrays.
 */
void
_mesa_initialize_array_object( struct gl_context *ctx,
			       struct gl_array_object *obj,
			       GLuint name )
{
   GLuint i;

   obj->Name = name;

   _glthread_INIT_MUTEX(obj->Mutex);
   obj->RefCount = 1;

   /* Init the individual arrays */
   for (i = 0; i < Elements(obj->VertexAttrib); i++) {
      switch (i) {
      case VERT_ATTRIB_WEIGHT:
         init_array(ctx, &obj->VertexAttrib[VERT_ATTRIB_WEIGHT], 1, GL_FLOAT);
         break;
      case VERT_ATTRIB_NORMAL:
         init_array(ctx, &obj->VertexAttrib[VERT_ATTRIB_NORMAL], 3, GL_FLOAT);
         break;
      case VERT_ATTRIB_COLOR1:
         init_array(ctx, &obj->VertexAttrib[VERT_ATTRIB_COLOR1], 3, GL_FLOAT);
         break;
      case VERT_ATTRIB_FOG:
         init_array(ctx, &obj->VertexAttrib[VERT_ATTRIB_FOG], 1, GL_FLOAT);
         break;
      case VERT_ATTRIB_COLOR_INDEX:
         init_array(ctx, &obj->VertexAttrib[VERT_ATTRIB_COLOR_INDEX], 1, GL_FLOAT);
         break;
      case VERT_ATTRIB_EDGEFLAG:
         init_array(ctx, &obj->VertexAttrib[VERT_ATTRIB_EDGEFLAG], 1, GL_BOOL);
         break;
#if FEATURE_point_size_array
      case VERT_ATTRIB_POINT_SIZE:
         init_array(ctx, &obj->VertexAttrib[VERT_ATTRIB_POINT_SIZE], 1, GL_FLOAT);
         break;
#endif
      default:
         init_array(ctx, &obj->VertexAttrib[i], 4, GL_FLOAT);
         break;
      }
   }

   _mesa_reference_buffer_object(ctx, &obj->ElementArrayBufferObj,
                                 ctx->Shared->NullBufferObj);
}


/**
 * Add the given array object to the array object pool.
 */
static void
save_array_object( struct gl_context *ctx, struct gl_array_object *obj )
{
   if (obj->Name > 0) {
      /* insert into hash table */
      _mesa_HashInsert(ctx->Array.Objects, obj->Name, obj);
   }
}


/**
 * Remove the given array object from the array object pool.
 * Do not deallocate the array object though.
 */
static void
remove_array_object( struct gl_context *ctx, struct gl_array_object *obj )
{
   if (obj->Name > 0) {
      /* remove from hash table */
      _mesa_HashRemove(ctx->Array.Objects, obj->Name);
   }
}



/**
 * Helper for update_arrays().
 * \return  min(current min, array->_MaxElement).
 */
static GLuint
update_min(GLuint min, struct gl_client_array *array)
{
   assert(array->Enabled);
   _mesa_update_array_max_element(array);
   return MIN2(min, array->_MaxElement);
}


/**
 * Examine vertex arrays to update the gl_array_object::_MaxElement field.
 */
void
_mesa_update_array_object_max_element(struct gl_context *ctx,
                                      struct gl_array_object *arrayObj)
{
   GLbitfield64 enabled = arrayObj->_Enabled;
   GLuint min = ~0u;

   while (enabled) {
      GLint attrib = _mesa_ffsll(enabled) - 1;
      enabled &= ~BITFIELD64_BIT(attrib);
      min = update_min(min, &arrayObj->VertexAttrib[attrib]);
   }

   /* _MaxElement is one past the last legal array element */
   arrayObj->_MaxElement = min;
}


/**********************************************************************/
/* API Functions                                                      */
/**********************************************************************/


/**
 * Helper for _mesa_BindVertexArray() and _mesa_BindVertexArrayAPPLE().
 * \param genRequired  specifies behavour when id was not generated with
 *                     glGenVertexArrays().
 */
static void
bind_vertex_array(struct gl_context *ctx, GLuint id, GLboolean genRequired)
{
   struct gl_array_object * const oldObj = ctx->Array.ArrayObj;
   struct gl_array_object *newObj = NULL;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   ASSERT(oldObj != NULL);

   if ( oldObj->Name == id )
      return;   /* rebinding the same array object- no change */

   /*
    * Get pointer to new array object (newObj)
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
         if (genRequired) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glBindVertexArray(id)");
            return;
         }

         /* For APPLE version, generate a new array object now */
	 newObj = (*ctx->Driver.NewArrayObject)(ctx, id);
         if (!newObj) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindVertexArrayAPPLE");
            return;
         }

         save_array_object(ctx, newObj);
      }

      if (!newObj->_Used) {
         /* The "Interactions with APPLE_vertex_array_object" section of the
          * GL_ARB_vertex_array_object spec says:
          *
          *     "The first bind call, either BindVertexArray or
          *     BindVertexArrayAPPLE, determines the semantic of the object."
          */
         newObj->ARBsemantics = genRequired;
         newObj->_Used = GL_TRUE;
      }
   }

   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= VERT_BIT_ALL;
   _mesa_reference_array_object(ctx, &ctx->Array.ArrayObj, newObj);

   /* Pass BindVertexArray call to device driver */
   if (ctx->Driver.BindArrayObject && newObj)
      ctx->Driver.BindArrayObject(ctx, newObj);
}


/**
 * ARB version of glBindVertexArray()
 * This function behaves differently from glBindVertexArrayAPPLE() in
 * that this function requires all ids to have been previously generated
 * by glGenVertexArrays[APPLE]().
 */
void GLAPIENTRY
_mesa_BindVertexArray( GLuint id )
{
   GET_CURRENT_CONTEXT(ctx);
   bind_vertex_array(ctx, id, GL_TRUE);
}


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
   bind_vertex_array(ctx, id, GL_FALSE);
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

	 /* The ID is immediately freed for re-use */
	 remove_array_object(ctx, obj);

         /* Unreference the array object. 
          * If refcount hits zero, the object will be deleted.
          */
         _mesa_reference_array_object(ctx, &obj, NULL);
      }
   }
}


/**
 * Generate a set of unique array object IDs and store them in \c arrays.
 * Helper for _mesa_GenVertexArrays[APPLE]() functions below.
 * \param n       Number of IDs to generate.
 * \param arrays  Array of \c n locations to store the IDs.
 * \param vboOnly Will arrays have to reside in VBOs?
 */
static void 
gen_vertex_arrays(struct gl_context *ctx, GLsizei n, GLuint *arrays)
{
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

   first = _mesa_HashFindFreeKeyBlock(ctx->Array.Objects, n);

   /* Allocate new, empty array objects and return identifiers */
   for (i = 0; i < n; i++) {
      struct gl_array_object *obj;
      GLuint name = first + i;

      obj = (*ctx->Driver.NewArrayObject)( ctx, name );
      if (!obj) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenVertexArraysAPPLE");
         return;
      }
      save_array_object(ctx, obj);
      arrays[i] = first + i;
   }
}


/**
 * ARB version of glGenVertexArrays()
 * All arrays will be required to live in VBOs.
 */
void GLAPIENTRY
_mesa_GenVertexArrays(GLsizei n, GLuint *arrays)
{
   GET_CURRENT_CONTEXT(ctx);
   gen_vertex_arrays(ctx, n, arrays);
}


/**
 * APPLE version of glGenVertexArraysAPPLE()
 * Arrays may live in VBOs or ordinary memory.
 */
void GLAPIENTRY
_mesa_GenVertexArraysAPPLE(GLsizei n, GLuint *arrays)
{
   GET_CURRENT_CONTEXT(ctx);
   gen_vertex_arrays(ctx, n, arrays);
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

   obj = lookup_arrayobj(ctx, id);

   return (obj != NULL) ? GL_TRUE : GL_FALSE;
}
