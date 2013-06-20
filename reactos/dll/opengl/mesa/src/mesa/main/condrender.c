/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009  VMware, Inc.   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file condrender.c
 * Conditional rendering functions
 *
 * \author Brian Paul
 */

#include "glheader.h"
#include "condrender.h"
#include "enums.h"
#include "mtypes.h"
#include "queryobj.h"


void GLAPIENTRY
_mesa_BeginConditionalRender(GLuint queryId, GLenum mode)
{
   struct gl_query_object *q;
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->Extensions.NV_conditional_render || ctx->Query.CondRenderQuery ||
       queryId == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginConditionalRender()");
      return;
   }

   ASSERT(ctx->Query.CondRenderMode == GL_NONE);

   switch (mode) {
   case GL_QUERY_WAIT:
   case GL_QUERY_NO_WAIT:
   case GL_QUERY_BY_REGION_WAIT:
   case GL_QUERY_BY_REGION_NO_WAIT:
      /* OK */
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBeginConditionalRender(mode=%s)",
                  _mesa_lookup_enum_by_nr(mode));
      return;
   }

   q = _mesa_lookup_query_object(ctx, queryId);
   if (!q) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glBeginConditionalRender(bad queryId=%u)", queryId);
      return;
   }
   ASSERT(q->Id == queryId);

   if (q->Target != GL_SAMPLES_PASSED || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginConditionalRender()");
      return;
   }

   ctx->Query.CondRenderQuery = q;
   ctx->Query.CondRenderMode = mode;

   if (ctx->Driver.BeginConditionalRender)
      ctx->Driver.BeginConditionalRender(ctx, q, mode);
}


void APIENTRY
_mesa_EndConditionalRender(void)
{
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0x0);

   if (!ctx->Extensions.NV_conditional_render || !ctx->Query.CondRenderQuery) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glEndConditionalRender()");
      return;
   }

   if (ctx->Driver.EndConditionalRender)
      ctx->Driver.EndConditionalRender(ctx, ctx->Query.CondRenderQuery);

   ctx->Query.CondRenderQuery = NULL;
   ctx->Query.CondRenderMode = GL_NONE;
}


/**
 * This function is called by software rendering commands (all point,
 * line triangle drawing, glClear, glDrawPixels, glCopyPixels, and
 * glBitmap, glBlitFramebuffer) to determine if subsequent drawing
 * commands should be
 * executed or discarded depending on the current conditional
 * rendering state.  Ideally, this check would be implemented by the
 * GPU when doing hardware rendering.  XXX should this function be
 * called via a new driver hook?
 *
 * \return GL_TRUE if we should render, GL_FALSE if we should discard
 */
GLboolean
_mesa_check_conditional_render(struct gl_context *ctx)
{
   struct gl_query_object *q = ctx->Query.CondRenderQuery;

   if (!q) {
      /* no query in progress - draw normally */
      return GL_TRUE;
   }

   switch (ctx->Query.CondRenderMode) {
   case GL_QUERY_BY_REGION_WAIT:
      /* fall-through */
   case GL_QUERY_WAIT:
      if (!q->Ready) {
         ctx->Driver.WaitQuery(ctx, q);
      }
      return q->Result > 0;
   case GL_QUERY_BY_REGION_NO_WAIT:
      /* fall-through */
   case GL_QUERY_NO_WAIT:
      return q->Ready ? (q->Result > 0) : GL_TRUE;
   default:
      _mesa_problem(ctx, "Bad cond render mode %s in "
                    " _mesa_check_conditional_render()",
                    _mesa_lookup_enum_by_nr(ctx->Query.CondRenderMode));
      return GL_TRUE;
   }
}
