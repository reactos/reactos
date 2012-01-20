/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "queryobj.h"
#include "mtypes.h"


/**
 * Allocate a new query object.  This is a fallback routine called via
 * ctx->Driver.NewQueryObject().
 * \param ctx - rendering context
 * \param id - the new object's ID
 * \return pointer to new query_object object or NULL if out of memory.
 */
struct gl_query_object *
_mesa_new_query_object(GLcontext *ctx, GLuint id)
{
   struct gl_query_object *q = MALLOC_STRUCT(gl_query_object);
   (void) ctx;
   if (q) {
      q->Id = id;
      q->Result = 0;
      q->Active = GL_FALSE;
      q->Ready = GL_TRUE;   /* correct, see spec */
   }
   return q;
}


/**
 * Begin a query.  Software driver fallback.
 * Called via ctx->Driver.BeginQuery().
 */
void
_mesa_begin_query(GLcontext *ctx, struct gl_query_object *q)
{
   /* no-op */
}


/**
 * End a query.  Software driver fallback.
 * Called via ctx->Driver.EndQuery().
 */
void
_mesa_end_query(GLcontext *ctx, struct gl_query_object *q)
{
   q->Ready = GL_TRUE;
}


/**
 * Wait for query to complete.  Software driver fallback.
 * Called via ctx->Driver.WaitQuery().
 */
void
_mesa_wait_query(GLcontext *ctx, struct gl_query_object *q)
{
   /* For software drivers, _mesa_end_query() should have completed the query.
    * For real hardware, implement a proper WaitQuery() driver function.
    */
   assert(q->Ready);
}


/**
 * Delete a query object.  Called via ctx->Driver.DeleteQuery().
 * Not removed from hash table here.
 */
void
_mesa_delete_query(GLcontext *ctx, struct gl_query_object *q)
{
   _mesa_free(q);
}


static struct gl_query_object *
lookup_query_object(GLcontext *ctx, GLuint id)
{
   return (struct gl_query_object *)
      _mesa_HashLookup(ctx->Query.QueryObjects, id);
}



void GLAPIENTRY
_mesa_GenQueriesARB(GLsizei n, GLuint *ids)
{
   GLuint first;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenQueriesARB(n < 0)");
      return;
   }

   /* No query objects can be active at this time! */
   if (ctx->Query.CurrentOcclusionObject ||
       ctx->Query.CurrentTimerObject) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGenQueriesARB");
      return;
   }

   first = _mesa_HashFindFreeKeyBlock(ctx->Query.QueryObjects, n);
   if (first) {
      GLsizei i;
      for (i = 0; i < n; i++) {
         struct gl_query_object *q
            = ctx->Driver.NewQueryObject(ctx, first + i);
         if (!q) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenQueriesARB");
            return;
         }
         ids[i] = first + i;
         _mesa_HashInsert(ctx->Query.QueryObjects, first + i, q);
      }
   }
}


void GLAPIENTRY
_mesa_DeleteQueriesARB(GLsizei n, const GLuint *ids)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteQueriesARB(n < 0)");
      return;
   }

   /* No query objects can be active at this time! */
   if (ctx->Query.CurrentOcclusionObject ||
       ctx->Query.CurrentTimerObject) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDeleteQueriesARB");
      return;
   }

   for (i = 0; i < n; i++) {
      if (ids[i] > 0) {
         struct gl_query_object *q = lookup_query_object(ctx, ids[i]);
         if (q) {
            ASSERT(!q->Active); /* should be caught earlier */
            _mesa_HashRemove(ctx->Query.QueryObjects, ids[i]);
            ctx->Driver.DeleteQuery(ctx, q);
         }
      }
   }
}


GLboolean GLAPIENTRY
_mesa_IsQueryARB(GLuint id)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (id && lookup_query_object(ctx, id))
      return GL_TRUE;
   else
      return GL_FALSE;
}


void GLAPIENTRY
_mesa_BeginQueryARB(GLenum target, GLuint id)
{
   struct gl_query_object *q;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_DEPTH);

   switch (target) {
      case GL_SAMPLES_PASSED_ARB:
         if (!ctx->Extensions.ARB_occlusion_query) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glBeginQueryARB(target)");
            return;
         }
         if (ctx->Query.CurrentOcclusionObject) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginQueryARB");
            return;
         }
         break;
#if FEATURE_EXT_timer_query
      case GL_TIME_ELAPSED_EXT:
         if (!ctx->Extensions.EXT_timer_query) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glBeginQueryARB(target)");
            return;
         }
         if (ctx->Query.CurrentTimerObject) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginQueryARB");
            return;
         }
         break;
#endif
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glBeginQueryARB(target)");
         return;
   }

   if (id == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginQueryARB(id==0)");
      return;
   }

   q = lookup_query_object(ctx, id);
   if (!q) {
      /* create new object */
      q = ctx->Driver.NewQueryObject(ctx, id);
      if (!q) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBeginQueryARB");
         return;
      }
      _mesa_HashInsert(ctx->Query.QueryObjects, id, q);
   }
   else {
      /* pre-existing object */
      if (q->Active) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBeginQueryARB(query already active)");
         return;
      }
   }

   q->Target = target;
   q->Active = GL_TRUE;
   q->Result = 0;
   q->Ready = GL_FALSE;

   if (target == GL_SAMPLES_PASSED_ARB) {
      ctx->Query.CurrentOcclusionObject = q;
   }
#if FEATURE_EXT_timer_query
   else if (target == GL_TIME_ELAPSED_EXT) {
      ctx->Query.CurrentTimerObject = q;
   }
#endif

   ctx->Driver.BeginQuery(ctx, q);
}


void GLAPIENTRY
_mesa_EndQueryARB(GLenum target)
{
   struct gl_query_object *q;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_DEPTH);

   switch (target) {
      case GL_SAMPLES_PASSED_ARB:
         if (!ctx->Extensions.ARB_occlusion_query) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glEndQueryARB(target)");
            return;
         }
         q = ctx->Query.CurrentOcclusionObject;
         ctx->Query.CurrentOcclusionObject = NULL;
         break;
#if FEATURE_EXT_timer_query
      case GL_TIME_ELAPSED_EXT:
         if (!ctx->Extensions.EXT_timer_query) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glEndQueryARB(target)");
            return;
         }
         q = ctx->Query.CurrentTimerObject;
         ctx->Query.CurrentTimerObject = NULL;
         break;
#endif
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glEndQueryARB(target)");
         return;
   }

   if (!q || !q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glEndQueryARB(no matching glBeginQueryARB)");
      return;
   }

   q->Active = GL_FALSE;
   ctx->Driver.EndQuery(ctx, q);
}


void GLAPIENTRY
_mesa_GetQueryivARB(GLenum target, GLenum pname, GLint *params)
{
   struct gl_query_object *q;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
      case GL_SAMPLES_PASSED_ARB:
         if (!ctx->Extensions.ARB_occlusion_query) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glEndQueryARB(target)");
            return;
         }
         q = ctx->Query.CurrentOcclusionObject;
         break;
#if FEATURE_EXT_timer_query
      case GL_TIME_ELAPSED_EXT:
         if (!ctx->Extensions.EXT_timer_query) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glEndQueryARB(target)");
            return;
         }
         q = ctx->Query.CurrentTimerObject;
         break;
#endif
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryivARB(target)");
         return;
   }

   switch (pname) {
      case GL_QUERY_COUNTER_BITS_ARB:
         *params = 8 * sizeof(q->Result);
         break;
      case GL_CURRENT_QUERY_ARB:
         *params = q ? q->Id : 0;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryivARB(pname)");
         return;
   }
}


void GLAPIENTRY
_mesa_GetQueryObjectivARB(GLuint id, GLenum pname, GLint *params)
{
   struct gl_query_object *q = NULL;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (id)
      q = lookup_query_object(ctx, id);

   if (!q || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetQueryObjectivARB(id=%d is invalid or active)", id);
      return;
   }

   switch (pname) {
      case GL_QUERY_RESULT_ARB:
         if (!q->Ready)
            ctx->Driver.WaitQuery(ctx, q);
         /* if result is too large for returned type, clamp to max value */
         if (q->Result > 0x7fffffff) {
            *params = 0x7fffffff;
         }
         else {
            *params = (GLint)q->Result;
         }
         break;
      case GL_QUERY_RESULT_AVAILABLE_ARB:
	 if (!q->Ready)
	    ctx->Driver.CheckQuery( ctx, q );
         *params = q->Ready;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryObjectivARB(pname)");
         return;
   }
}


void GLAPIENTRY
_mesa_GetQueryObjectuivARB(GLuint id, GLenum pname, GLuint *params)
{
   struct gl_query_object *q = NULL;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (id)
      q = lookup_query_object(ctx, id);

   if (!q || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetQueryObjectuivARB(id=%d is invalid or active)", id);
      return;
   }

   switch (pname) {
      case GL_QUERY_RESULT_ARB:
         if (!q->Ready)
            ctx->Driver.WaitQuery(ctx, q);
         /* if result is too large for returned type, clamp to max value */
         if (q->Result > 0xffffffff) {
            *params = 0xffffffff;
         }
         else {
            *params = (GLuint)q->Result;
         }
         break;
      case GL_QUERY_RESULT_AVAILABLE_ARB:
	 if (!q->Ready)
	    ctx->Driver.CheckQuery( ctx, q );
         *params = q->Ready;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryObjectuivARB(pname)");
         return;
   }
}


#if FEATURE_EXT_timer_query

/**
 * New with GL_EXT_timer_query
 */
void GLAPIENTRY
_mesa_GetQueryObjecti64vEXT(GLuint id, GLenum pname, GLint64EXT *params)
{
   struct gl_query_object *q = NULL;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (id)
      q = lookup_query_object(ctx, id);

   if (!q || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetQueryObjectui64vARB(id=%d is invalid or active)", id);
      return;
   }

   switch (pname) {
      case GL_QUERY_RESULT_ARB:
         if (!q->Ready)
            ctx->Driver.WaitQuery(ctx, q);
         *params = q->Result;
         break;
      case GL_QUERY_RESULT_AVAILABLE_ARB:
	 if (!q->Ready)
	    ctx->Driver.CheckQuery( ctx, q );
         *params = q->Ready;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryObjecti64vARB(pname)");
         return;
   }
}


/**
 * New with GL_EXT_timer_query
 */
void GLAPIENTRY
_mesa_GetQueryObjectui64vEXT(GLuint id, GLenum pname, GLuint64EXT *params)
{
   struct gl_query_object *q = NULL;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (id)
      q = lookup_query_object(ctx, id);

   if (!q || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetQueryObjectuui64vARB(id=%d is invalid or active)", id);
      return;
   }

   switch (pname) {
      case GL_QUERY_RESULT_ARB:
         if (!q->Ready)
            ctx->Driver.WaitQuery(ctx, q);
         *params = q->Result;
         break;
      case GL_QUERY_RESULT_AVAILABLE_ARB:
	 if (!q->Ready)
	    ctx->Driver.CheckQuery( ctx, q );
         *params = q->Ready;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryObjectui64vARB(pname)");
         return;
   }
}

#endif /* FEATURE_EXT_timer_query */


/**
 * Allocate/init the context state related to query objects.
 */
void
_mesa_init_query(GLcontext *ctx)
{
#if FEATURE_ARB_occlusion_query
   ctx->Query.QueryObjects = _mesa_NewHashTable();
   ctx->Query.CurrentOcclusionObject = NULL;
#endif
}


/**
 * Callback for deleting a query object.  Called by _mesa_HashDeleteAll().
 */
static void
delete_queryobj_cb(GLuint id, void *data, void *userData)
{
   struct gl_query_object *q= (struct gl_query_object *) data;
   GLcontext *ctx = (GLcontext *)userData;
   ctx->Driver.DeleteQuery(ctx, q);
}


/**
 * Free the context state related to query objects.
 */
void
_mesa_free_query_data(GLcontext *ctx)
{
   _mesa_HashDeleteAll(ctx->Query.QueryObjects, delete_queryobj_cb, ctx);
   _mesa_DeleteHashTable(ctx->Query.QueryObjects);
}
