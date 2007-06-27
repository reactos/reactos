/*
 * Mesa 3-D graphics library
 * Version:  6.0.2
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


/*
 * Functions to implement the GL_ARB_occlusion_query extension.
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "occlude.h"
#include "mtypes.h"


struct occlusion_query
{
   GLenum Target;
   GLuint Id;
   GLuint PassedCounter;
   GLboolean Active;
};


/**
 * Allocate a new occlusion query object.
 * \param target - must be GL_SAMPLES_PASSED_ARB at this time
 * \param id - the object's ID
 * \return pointer to new occlusion_query object or NULL if out of memory.
 */
static struct occlusion_query *
new_query_object(GLenum target, GLuint id)
{
   struct occlusion_query *q = MALLOC_STRUCT(occlusion_query);
   if (q) {
      q->Target = target;
      q->Id = id;
      q->PassedCounter = 0;
      q->Active = GL_FALSE;
   }
   return q;
}


/**
 * Delete an occlusion query object.
 */
static void
delete_query_object(struct occlusion_query *q)
{
   FREE(q);
}


void GLAPIENTRY
_mesa_GenQueriesARB(GLsizei n, GLuint *ids)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenQueriesARB(n < 0)");
      return;
   }

   if (ctx->Occlusion.Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGenQueriesARB");
      return;
   }

   first = _mesa_HashFindFreeKeyBlock(ctx->Occlusion.QueryObjects, n);
   if (first) {
      GLsizei i;
      for (i = 0; i < n; i++) {
         struct occlusion_query *q = new_query_object(GL_SAMPLES_PASSED_ARB,
                                                      first + i);
         if (!q) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenQueriesARB");
            return;
         }
         ids[i] = first + i;
         _mesa_HashInsert(ctx->Occlusion.QueryObjects, first + i, q);
      }
   }
}


void GLAPIENTRY
_mesa_DeleteQueriesARB(GLsizei n, const GLuint *ids)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteQueriesARB(n < 0)");
      return;
   }

   if (ctx->Occlusion.Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDeleteQueriesARB");
      return;
   }

   for (i = 0; i < n; i++) {
      if (ids[i] > 0) {
         struct occlusion_query *q = (struct occlusion_query *)
            _mesa_HashLookup(ctx->Occlusion.QueryObjects, ids[i]);
         if (q) {
            _mesa_HashRemove(ctx->Occlusion.QueryObjects, ids[i]);
            delete_query_object(q);
         }
      }
   }
}


GLboolean GLAPIENTRY
_mesa_IsQueryARB(GLuint id)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (id && _mesa_HashLookup(ctx->Occlusion.QueryObjects, id))
      return GL_TRUE;
   else
      return GL_FALSE;
}


void GLAPIENTRY
_mesa_BeginQueryARB(GLenum target, GLuint id)
{
   GET_CURRENT_CONTEXT(ctx);
   struct occlusion_query *q;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_DEPTH);

   if (target != GL_SAMPLES_PASSED_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBeginQueryARB(target)");
      return;
   }

   if (id == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginQueryARB(id==0)");
      return;
   }

   if (ctx->Occlusion.CurrentQueryObject) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginQueryARB(target)");
      return;
   }

   q = (struct occlusion_query *)
      _mesa_HashLookup(ctx->Occlusion.QueryObjects, id);
   if (q && q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginQueryARB");
      return;
   }
   else if (!q) {
      q = new_query_object(target, id);
      if (!q) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBeginQueryARB");
         return;
      }
      _mesa_HashInsert(ctx->Occlusion.QueryObjects, id, q);
   }

   q->Active = GL_TRUE;
   q->PassedCounter = 0;
   ctx->Occlusion.Active = GL_TRUE;
   ctx->Occlusion.CurrentQueryObject = id;
   ctx->Occlusion.PassedCounter = 0;
}


void GLAPIENTRY
_mesa_EndQueryARB(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   struct occlusion_query *q = NULL;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_DEPTH);

   if (target != GL_SAMPLES_PASSED_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glEndQueryARB(target)");
      return;
   }

   if (ctx->Occlusion.CurrentQueryObject)
      q = (struct occlusion_query *)
         _mesa_HashLookup(ctx->Occlusion.QueryObjects,
                          ctx->Occlusion.CurrentQueryObject);
   if (!q || !q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glEndQuery with no glBeginQuery");
      return;
   }

   q->PassedCounter = ctx->Occlusion.PassedCounter;
   q->Active = GL_FALSE;
   ctx->Occlusion.Active = GL_FALSE;
   ctx->Occlusion.CurrentQueryObject = 0;
}


void GLAPIENTRY
_mesa_GetQueryivARB(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_SAMPLES_PASSED_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryivARB(target)");
      return;
   }

   switch (pname) {
      case GL_QUERY_COUNTER_BITS_ARB:
         *params = 8 * sizeof(GLuint);
         break;
      case GL_CURRENT_QUERY_ARB:
         *params = ctx->Occlusion.CurrentQueryObject;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryivARB(pname)");
         return;
   }
}


void GLAPIENTRY
_mesa_GetQueryObjectivARB(GLuint id, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct occlusion_query *q = NULL;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (id)
      q = (struct occlusion_query *)
         _mesa_HashLookup(ctx->Occlusion.QueryObjects, id);

   if (!q || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetQueryObjectivARB(id=%d)", id);
      return;
   }

   switch (pname) {
      case GL_QUERY_RESULT_ARB:
         *params = q->PassedCounter;
         break;
      case GL_QUERY_RESULT_AVAILABLE_ARB:
         /* XXX revisit when we have a hardware implementation! */
         *params = GL_TRUE;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryObjectivARB(pname)");
         return;
   }
}


void GLAPIENTRY
_mesa_GetQueryObjectuivARB(GLuint id, GLenum pname, GLuint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct occlusion_query *q = NULL;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (id)
      q = (struct occlusion_query *)
         _mesa_HashLookup(ctx->Occlusion.QueryObjects, id);
   if (!q || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetQueryObjectuivARB(id=%d", id);
      return;
   }

   switch (pname) {
      case GL_QUERY_RESULT_ARB:
         *params = q->PassedCounter;
         break;
      case GL_QUERY_RESULT_AVAILABLE_ARB:
         /* XXX revisit when we have a hardware implementation! */
         *params = GL_TRUE;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryObjectuivARB(pname)");
         return;
   }
}



/**
 * Allocate/init the context state related to occlusion query objects.
 */
void
_mesa_init_occlude(GLcontext *ctx)
{
#if FEATURE_ARB_occlusion_query
   ctx->Occlusion.QueryObjects = _mesa_NewHashTable();
#endif
   ctx->OcclusionResult = GL_FALSE;
   ctx->OcclusionResultSaved = GL_FALSE;
}


/**
 * Free the context state related to occlusion query objects.
 */
void
_mesa_free_occlude_data(GLcontext *ctx)
{
   while (1) {
      GLuint query = _mesa_HashFirstEntry(ctx->Occlusion.QueryObjects);
      if (query) {
         struct occlusion_query *q = (struct occlusion_query *)
            _mesa_HashLookup(ctx->Occlusion.QueryObjects, query);
         ASSERT(q);
         delete_query_object(q);
         _mesa_HashRemove(ctx->Occlusion.QueryObjects, query);
      }
      else {
         break;
      }
   }
   _mesa_DeleteHashTable(ctx->Occlusion.QueryObjects);
}
