/*
 * Mesa 3-D graphics library
 * Version:  6.3
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
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 *    Gareth Hughes
 */

#include "glheader.h"
#include "api_arrayelt.h"
#include "context.h"
#include "imports.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "vtxfmt.h"
#include "eval.h"
#include "dlist.h"
#include "main/dispatch.h"


#if FEATURE_beginend

/**
 * Use the per-vertex functions found in <vfmt> to initialoze the given
 * API dispatch table.
 */
static void
install_vtxfmt( struct _glapi_table *tab, const GLvertexformat *vfmt )
{
   _mesa_install_arrayelt_vtxfmt(tab, vfmt);

   SET_Color3f(tab, vfmt->Color3f);
   SET_Color3fv(tab, vfmt->Color3fv);
   SET_Color4f(tab, vfmt->Color4f);
   SET_Color4fv(tab, vfmt->Color4fv);
   SET_EdgeFlag(tab, vfmt->EdgeFlag);

   _mesa_install_eval_vtxfmt(tab, vfmt);

   SET_FogCoordfEXT(tab, vfmt->FogCoordfEXT);
   SET_FogCoordfvEXT(tab, vfmt->FogCoordfvEXT);
   SET_Indexf(tab, vfmt->Indexf);
   SET_Indexfv(tab, vfmt->Indexfv);
   SET_Materialfv(tab, vfmt->Materialfv);
   SET_MultiTexCoord1fARB(tab, vfmt->MultiTexCoord1fARB);
   SET_MultiTexCoord1fvARB(tab, vfmt->MultiTexCoord1fvARB);
   SET_MultiTexCoord2fARB(tab, vfmt->MultiTexCoord2fARB);
   SET_MultiTexCoord2fvARB(tab, vfmt->MultiTexCoord2fvARB);
   SET_MultiTexCoord3fARB(tab, vfmt->MultiTexCoord3fARB);
   SET_MultiTexCoord3fvARB(tab, vfmt->MultiTexCoord3fvARB);
   SET_MultiTexCoord4fARB(tab, vfmt->MultiTexCoord4fARB);
   SET_MultiTexCoord4fvARB(tab, vfmt->MultiTexCoord4fvARB);
   SET_Normal3f(tab, vfmt->Normal3f);
   SET_Normal3fv(tab, vfmt->Normal3fv);
   SET_SecondaryColor3fEXT(tab, vfmt->SecondaryColor3fEXT);
   SET_SecondaryColor3fvEXT(tab, vfmt->SecondaryColor3fvEXT);
   SET_TexCoord1f(tab, vfmt->TexCoord1f);
   SET_TexCoord1fv(tab, vfmt->TexCoord1fv);
   SET_TexCoord2f(tab, vfmt->TexCoord2f);
   SET_TexCoord2fv(tab, vfmt->TexCoord2fv);
   SET_TexCoord3f(tab, vfmt->TexCoord3f);
   SET_TexCoord3fv(tab, vfmt->TexCoord3fv);
   SET_TexCoord4f(tab, vfmt->TexCoord4f);
   SET_TexCoord4fv(tab, vfmt->TexCoord4fv);
   SET_Vertex2f(tab, vfmt->Vertex2f);
   SET_Vertex2fv(tab, vfmt->Vertex2fv);
   SET_Vertex3f(tab, vfmt->Vertex3f);
   SET_Vertex3fv(tab, vfmt->Vertex3fv);
   SET_Vertex4f(tab, vfmt->Vertex4f);
   SET_Vertex4fv(tab, vfmt->Vertex4fv);

   _mesa_install_dlist_vtxfmt(tab, vfmt);   /* glCallList / glCallLists */

   SET_Begin(tab, vfmt->Begin);
   SET_End(tab, vfmt->End);
   SET_PrimitiveRestartNV(tab, vfmt->PrimitiveRestartNV);

   SET_Rectf(tab, vfmt->Rectf);

   SET_DrawArrays(tab, vfmt->DrawArrays);
   SET_DrawElements(tab, vfmt->DrawElements);
   SET_DrawRangeElements(tab, vfmt->DrawRangeElements);
   SET_MultiDrawElementsEXT(tab, vfmt->MultiDrawElementsEXT);
   SET_DrawElementsBaseVertex(tab, vfmt->DrawElementsBaseVertex);
   SET_DrawRangeElementsBaseVertex(tab, vfmt->DrawRangeElementsBaseVertex);
   SET_MultiDrawElementsBaseVertex(tab, vfmt->MultiDrawElementsBaseVertex);
   SET_DrawArraysInstancedARB(tab, vfmt->DrawArraysInstanced);
   SET_DrawElementsInstancedARB(tab, vfmt->DrawElementsInstanced);
   SET_DrawElementsInstancedBaseVertex(tab, vfmt->DrawElementsInstancedBaseVertex);

   /* GL_NV_vertex_program */
   SET_VertexAttrib1fNV(tab, vfmt->VertexAttrib1fNV);
   SET_VertexAttrib1fvNV(tab, vfmt->VertexAttrib1fvNV);
   SET_VertexAttrib2fNV(tab, vfmt->VertexAttrib2fNV);
   SET_VertexAttrib2fvNV(tab, vfmt->VertexAttrib2fvNV);
   SET_VertexAttrib3fNV(tab, vfmt->VertexAttrib3fNV);
   SET_VertexAttrib3fvNV(tab, vfmt->VertexAttrib3fvNV);
   SET_VertexAttrib4fNV(tab, vfmt->VertexAttrib4fNV);
   SET_VertexAttrib4fvNV(tab, vfmt->VertexAttrib4fvNV);
#if FEATURE_ARB_vertex_program
   SET_VertexAttrib1fARB(tab, vfmt->VertexAttrib1fARB);
   SET_VertexAttrib1fvARB(tab, vfmt->VertexAttrib1fvARB);
   SET_VertexAttrib2fARB(tab, vfmt->VertexAttrib2fARB);
   SET_VertexAttrib2fvARB(tab, vfmt->VertexAttrib2fvARB);
   SET_VertexAttrib3fARB(tab, vfmt->VertexAttrib3fARB);
   SET_VertexAttrib3fvARB(tab, vfmt->VertexAttrib3fvARB);
   SET_VertexAttrib4fARB(tab, vfmt->VertexAttrib4fARB);
   SET_VertexAttrib4fvARB(tab, vfmt->VertexAttrib4fvARB);
#endif

   /* GL_EXT_gpu_shader4 / OpenGL 3.0 */
   SET_VertexAttribI1iEXT(tab, vfmt->VertexAttribI1i);
   SET_VertexAttribI2iEXT(tab, vfmt->VertexAttribI2i);
   SET_VertexAttribI3iEXT(tab, vfmt->VertexAttribI3i);
   SET_VertexAttribI4iEXT(tab, vfmt->VertexAttribI4i);
   SET_VertexAttribI2ivEXT(tab, vfmt->VertexAttribI2iv);
   SET_VertexAttribI3ivEXT(tab, vfmt->VertexAttribI3iv);
   SET_VertexAttribI4ivEXT(tab, vfmt->VertexAttribI4iv);

   SET_VertexAttribI1uiEXT(tab, vfmt->VertexAttribI1ui);
   SET_VertexAttribI2uiEXT(tab, vfmt->VertexAttribI2ui);
   SET_VertexAttribI3uiEXT(tab, vfmt->VertexAttribI3ui);
   SET_VertexAttribI4uiEXT(tab, vfmt->VertexAttribI4ui);
   SET_VertexAttribI2uivEXT(tab, vfmt->VertexAttribI2uiv);
   SET_VertexAttribI3uivEXT(tab, vfmt->VertexAttribI3uiv);
   SET_VertexAttribI4uivEXT(tab, vfmt->VertexAttribI4uiv);
}


/**
 * Install per-vertex functions into the API dispatch table for execution.
 */
void
_mesa_install_exec_vtxfmt(struct gl_context *ctx, const GLvertexformat *vfmt)
{
   if (ctx->API == API_OPENGL)
      install_vtxfmt( ctx->Exec, vfmt );
}


/**
 * Install per-vertex functions into the API dispatch table for display
 * list compilation.
 */
void
_mesa_install_save_vtxfmt(struct gl_context *ctx, const GLvertexformat *vfmt)
{
   if (ctx->API == API_OPENGL)
      install_vtxfmt( ctx->Save, vfmt );
}


#endif /* FEATURE_beginend */
