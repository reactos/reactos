
/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
   struct gl_tnl_module *tnl = &(ctx->TnlModule);			\
									\
   ASSERT( tnl->Current );						\
   ASSERT( tnl->SwapCount < NUM_VERTEX_FORMAT_ENTRIES );		\
									\
   /* Save the swapped function's dispatch entry so it can be */	\
   /* restored later. */						\
   tnl->Swapped[tnl->SwapCount][0] = (void *)&(ctx->Exec->FUNC);	\
   tnl->Swapped[tnl->SwapCount][1] = (void *)TAG(FUNC);			\
   tnl->SwapCount++;							\
									\
   if ( 0 )								\
      _mesa_debug(ctx, "   swapping gl" #FUNC"...\n" );			\
									\
   /* Install the tnl function pointer.	*/				\
   ctx->Exec->FUNC = tnl->Current->FUNC;				\
}

#define TAG(x) neutral_##x
#include "vtxfmt_tmp.h"



static void install_vtxfmt( struct _glapi_table *tab, GLvertexformat *vfmt )
{
   tab->ArrayElement = vfmt->ArrayElement;
   tab->Color3f = vfmt->Color3f;
   tab->Color3fv = vfmt->Color3fv;
   tab->Color4f = vfmt->Color4f;
   tab->Color4fv = vfmt->Color4fv;
   tab->EdgeFlag = vfmt->EdgeFlag;
   tab->EdgeFlagv = vfmt->EdgeFlagv;
   tab->EvalCoord1f = vfmt->EvalCoord1f;
   tab->EvalCoord1fv = vfmt->EvalCoord1fv;
   tab->EvalCoord2f = vfmt->EvalCoord2f;
   tab->EvalCoord2fv = vfmt->EvalCoord2fv;
   tab->EvalPoint1 = vfmt->EvalPoint1;
   tab->EvalPoint2 = vfmt->EvalPoint2;
   tab->FogCoordfEXT = vfmt->FogCoordfEXT;
   tab->FogCoordfvEXT = vfmt->FogCoordfvEXT;
   tab->Indexf = vfmt->Indexf;
   tab->Indexfv = vfmt->Indexfv;
   tab->Materialfv = vfmt->Materialfv;
   tab->MultiTexCoord1fARB = vfmt->MultiTexCoord1fARB;
   tab->MultiTexCoord1fvARB = vfmt->MultiTexCoord1fvARB;
   tab->MultiTexCoord2fARB = vfmt->MultiTexCoord2fARB;
   tab->MultiTexCoord2fvARB = vfmt->MultiTexCoord2fvARB;
   tab->MultiTexCoord3fARB = vfmt->MultiTexCoord3fARB;
   tab->MultiTexCoord3fvARB = vfmt->MultiTexCoord3fvARB;
   tab->MultiTexCoord4fARB = vfmt->MultiTexCoord4fARB;
   tab->MultiTexCoord4fvARB = vfmt->MultiTexCoord4fvARB;
   tab->Normal3f = vfmt->Normal3f;
   tab->Normal3fv = vfmt->Normal3fv;
   tab->SecondaryColor3fEXT = vfmt->SecondaryColor3fEXT;
   tab->SecondaryColor3fvEXT = vfmt->SecondaryColor3fvEXT;
   tab->TexCoord1f = vfmt->TexCoord1f;
   tab->TexCoord1fv = vfmt->TexCoord1fv;
   tab->TexCoord2f = vfmt->TexCoord2f;
   tab->TexCoord2fv = vfmt->TexCoord2fv;
   tab->TexCoord3f = vfmt->TexCoord3f;
   tab->TexCoord3fv = vfmt->TexCoord3fv;
   tab->TexCoord4f = vfmt->TexCoord4f;
   tab->TexCoord4fv = vfmt->TexCoord4fv;
   tab->Vertex2f = vfmt->Vertex2f;
   tab->Vertex2fv = vfmt->Vertex2fv;
   tab->Vertex3f = vfmt->Vertex3f;
   tab->Vertex3fv = vfmt->Vertex3fv;
   tab->Vertex4f = vfmt->Vertex4f;
   tab->Vertex4fv = vfmt->Vertex4fv;
   tab->CallList = vfmt->CallList;
   tab->CallLists = vfmt->CallLists;
   tab->Begin = vfmt->Begin;
   tab->End = vfmt->End;
   tab->VertexAttrib1fNV = vfmt->VertexAttrib1fNV;
   tab->VertexAttrib1fvNV = vfmt->VertexAttrib1fvNV;
   tab->VertexAttrib2fNV = vfmt->VertexAttrib2fNV;
   tab->VertexAttrib2fvNV = vfmt->VertexAttrib2fvNV;
   tab->VertexAttrib3fNV = vfmt->VertexAttrib3fNV;
   tab->VertexAttrib3fvNV = vfmt->VertexAttrib3fvNV;
   tab->VertexAttrib4fNV = vfmt->VertexAttrib4fNV;
   tab->VertexAttrib4fvNV = vfmt->VertexAttrib4fvNV;
   tab->Rectf = vfmt->Rectf;
   tab->DrawArrays = vfmt->DrawArrays;
   tab->DrawElements = vfmt->DrawElements;
   tab->DrawRangeElements = vfmt->DrawRangeElements;
   tab->EvalMesh1 = vfmt->EvalMesh1;
   tab->EvalMesh2 = vfmt->EvalMesh2;
   assert(tab->EvalMesh2);
}


void _mesa_init_exec_vtxfmt( GLcontext *ctx )
{
   install_vtxfmt( ctx->Exec, &neutral_vtxfmt );
   ctx->TnlModule.SwapCount = 0;
}


void _mesa_install_exec_vtxfmt( GLcontext *ctx, GLvertexformat *vfmt )
{
   ctx->TnlModule.Current = vfmt;
   _mesa_restore_exec_vtxfmt( ctx );
}

void _mesa_install_save_vtxfmt( GLcontext *ctx, GLvertexformat *vfmt )
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
      *(void **)tnl->Swapped[i][0] = tnl->Swapped[i][1];
   }

   tnl->SwapCount = 0;
}
