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
#include "api_loopback.h"
#include "context.h"
#include "imports.h"
#include "mtypes.h"
#include "state.h"
#include "vtxfmt.h"


/* The neutral vertex format.  This wraps all tnl module functions,
 * verifying that the currently-installed module is valid and then
 * installing the function pointers in a lazy fashion.  It records the
 * function pointers that have been swapped out, which allows a fast
 * restoration of the neutral module in almost all cases -- a typical
 * app might only require 4-6 functions to be modified from the neutral
 * baseline, and only restoring these is certainly preferable to doing
 * the entire module's 60 or so function pointers.
 */

#define PRE_LOOPBACK( FUNC )						\
{									\
   GET_CURRENT_CONTEXT(ctx);						\
   struct gl_tnl_module * const tnl = &(ctx->TnlModule);		\
   const int tmp_offset = _gloffset_ ## FUNC ;				\
									\
   ASSERT( tnl->Current );						\
   ASSERT( tnl->SwapCount < NUM_VERTEX_FORMAT_ENTRIES );		\
   ASSERT( tmp_offset >= 0 );						\
									\
   /* Save the swapped function's dispatch entry so it can be */	\
   /* restored later. */						\
   tnl->Swapped[tnl->SwapCount].location = & (((_glapi_proc *)ctx->Exec)[tmp_offset]); \
   tnl->Swapped[tnl->SwapCount].function = (_glapi_proc)TAG(FUNC);	\
   tnl->SwapCount++;							\
									\
   if ( 0 )								\
      _mesa_debug(ctx, "   swapping gl" #FUNC"...\n" );			\
									\
   /* Install the tnl function pointer.	*/				\
   SET_ ## FUNC(ctx->Exec, tnl->Current->FUNC);				\
}

#define TAG(x) neutral_##x
#include "vtxfmt_tmp.h"


/**
 * Use the per-vertex functions found in <vfmt> to initialze the given
 * API dispatch table.
 */
static void
install_vtxfmt( struct _glapi_table *tab, const GLvertexformat *vfmt )
{
   SET_ArrayElement(tab, vfmt->ArrayElement);
   SET_Color3f(tab, vfmt->Color3f);
   SET_Color3fv(tab, vfmt->Color3fv);
   SET_Color4f(tab, vfmt->Color4f);
   SET_Color4fv(tab, vfmt->Color4fv);
   SET_EdgeFlag(tab, vfmt->EdgeFlag);
   SET_EvalCoord1f(tab, vfmt->EvalCoord1f);
   SET_EvalCoord1fv(tab, vfmt->EvalCoord1fv);
   SET_EvalCoord2f(tab, vfmt->EvalCoord2f);
   SET_EvalCoord2fv(tab, vfmt->EvalCoord2fv);
   SET_EvalPoint1(tab, vfmt->EvalPoint1);
   SET_EvalPoint2(tab, vfmt->EvalPoint2);
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
   SET_CallList(tab, vfmt->CallList);
   SET_CallLists(tab, vfmt->CallLists);
   SET_Begin(tab, vfmt->Begin);
   SET_End(tab, vfmt->End);
   SET_Rectf(tab, vfmt->Rectf);
   SET_DrawArrays(tab, vfmt->DrawArrays);
   SET_DrawElements(tab, vfmt->DrawElements);
   SET_DrawRangeElements(tab, vfmt->DrawRangeElements);
   SET_EvalMesh1(tab, vfmt->EvalMesh1);
   SET_EvalMesh2(tab, vfmt->EvalMesh2);
   ASSERT(tab->EvalMesh2);

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
}


void _mesa_init_exec_vtxfmt( GLcontext *ctx )
{
   install_vtxfmt( ctx->Exec, &neutral_vtxfmt );
   ctx->TnlModule.SwapCount = 0;
}


void _mesa_install_exec_vtxfmt( GLcontext *ctx, const GLvertexformat *vfmt )
{
   ctx->TnlModule.Current = vfmt;
   _mesa_restore_exec_vtxfmt( ctx );
}


void _mesa_install_save_vtxfmt( GLcontext *ctx, const GLvertexformat *vfmt )
{
   install_vtxfmt( ctx->Save, vfmt );
}


void _mesa_restore_exec_vtxfmt( GLcontext *ctx )
{
   struct gl_tnl_module *tnl = &(ctx->TnlModule);
   GLuint i;

   /* Restore the neutral tnl module wrapper.
    */
   for ( i = 0 ; i < tnl->SwapCount ; i++ ) {
      *(tnl->Swapped[i].location) = tnl->Swapped[i].function;
   }

   tnl->SwapCount = 0;
}
