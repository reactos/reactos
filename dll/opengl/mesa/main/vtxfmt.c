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

#include <precomp.h>

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
   SET_Normal3f(tab, vfmt->Normal3f);
   SET_Normal3fv(tab, vfmt->Normal3fv);
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

   SET_Rectf(tab, vfmt->Rectf);

   SET_DrawArrays(tab, vfmt->DrawArrays);
   SET_DrawElements(tab, vfmt->DrawElements);

   /* GL_NV_vertex_program */
   SET_VertexAttrib1fNV(tab, vfmt->VertexAttrib1fNV);
   SET_VertexAttrib1fvNV(tab, vfmt->VertexAttrib1fvNV);
   SET_VertexAttrib2fNV(tab, vfmt->VertexAttrib2fNV);
   SET_VertexAttrib2fvNV(tab, vfmt->VertexAttrib2fvNV);
   SET_VertexAttrib3fNV(tab, vfmt->VertexAttrib3fNV);
   SET_VertexAttrib3fvNV(tab, vfmt->VertexAttrib3fvNV);
   SET_VertexAttrib4fNV(tab, vfmt->VertexAttrib4fNV);
   SET_VertexAttrib4fvNV(tab, vfmt->VertexAttrib4fvNV);
}


/**
 * Install per-vertex functions into the API dispatch table for execution.
 */
void
_mesa_install_exec_vtxfmt(struct gl_context *ctx, const GLvertexformat *vfmt)
{
   install_vtxfmt( ctx->Exec, vfmt );
}


/**
 * Install per-vertex functions into the API dispatch table for display
 * list compilation.
 */
void
_mesa_install_save_vtxfmt(struct gl_context *ctx, const GLvertexformat *vfmt)
{
   install_vtxfmt( ctx->Save, vfmt );
}


#endif /* FEATURE_beginend */
