/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


#include "main/glheader.h"
#include "main/colormac.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/imports.h"
#include "main/mtypes.h"

#include "math/m_xform.h"

#include "t_context.h"
#include "t_pipeline.h"



/* EXT_vertex_cull.  Not really a big win, but probably depends on
 * your application.  This stage not included in the default pipeline.
 */
static GLboolean run_cull_stage( GLcontext *ctx,
				 struct tnl_pipeline_stage *stage )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;

   const GLfloat a = ctx->Transform.CullObjPos[0];
   const GLfloat b = ctx->Transform.CullObjPos[1];
   const GLfloat c = ctx->Transform.CullObjPos[2];
   GLfloat *norm = (GLfloat *)VB->AttribPtr[_TNL_ATTRIB_NORMAL]->data;
   GLuint stride = VB->AttribPtr[_TNL_ATTRIB_NORMAL]->stride;
   GLuint count = VB->Count;
   GLuint i;

   if (ctx->VertexProgram._Current ||
       !ctx->Transform.CullVertexFlag) 
      return GL_TRUE;

   VB->ClipOrMask &= ~CLIP_CULL_BIT;
   VB->ClipAndMask |= CLIP_CULL_BIT;

   for (i = 0 ; i < count ; i++) {
      GLfloat dp = (norm[0] * a + 
		    norm[1] * b +
		    norm[2] * c);

      if (dp < 0) {
	 VB->ClipMask[i] |= CLIP_CULL_BIT;
	 VB->ClipOrMask |= CLIP_CULL_BIT;
      }
      else {
	 VB->ClipMask[i] &= ~CLIP_CULL_BIT;
	 VB->ClipAndMask &= ~CLIP_CULL_BIT;
      }

      STRIDE_F(norm, stride);
   }

   return !(VB->ClipAndMask & CLIP_CULL_BIT);
}



const struct tnl_pipeline_stage _tnl_vertex_cull_stage =
{
   "EXT_cull_vertex",
   NULL,			/* private data */
   NULL,				/* ctr */
   NULL,				/* destructor */
   NULL,
   run_cull_stage		/* run -- initially set to init */
};
