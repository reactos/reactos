/* $Id: pointers.c,v 1.20 1997/10/29 01:29:09 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.5
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: pointers.c,v $
 * Revision 1.20  1997/10/29 01:29:09  brianp
 * added GL_EXT_point_parameters extension from Daniel Barrero
 *
 * Revision 1.19  1997/09/27 00:16:30  brianp
 * added GL_EXT_paletted_texture extension
 *
 * Revision 1.18  1997/07/24 01:23:44  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.17  1997/06/20 02:50:55  brianp
 * added Color4ubv API pointer
 *
 * Revision 1.16  1997/05/28 03:26:02  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.15  1997/05/24 12:08:25  brianp
 * broke gl_init_api_function_pointers() into sub-functions
 *
 * Revision 1.14  1997/04/24 01:50:53  brianp
 * optimized glColor3f, glColor3fv, glColor4fv
 *
 * Revision 1.13  1997/04/21 01:22:27  brianp
 * added gl_save_Rectf()
 *
 * Revision 1.12  1997/04/20 20:28:49  brianp
 * replaced abort() with gl_problem()
 *
 * Revision 1.11  1997/04/20 16:18:15  brianp
 * added glOrtho and glFrustum API pointers
 *
 * Revision 1.10  1997/04/16 23:55:33  brianp
 * added optimized glTexCoord2f code
 *
 * Revision 1.9  1997/04/14 22:18:23  brianp
 * added optimized glVertex3fv code
 *
 * Revision 1.8  1997/04/14 02:00:39  brianp
 * #include "texstate.h" instead of "texture.h"
 *
 * Revision 1.7  1997/04/12 12:33:12  brianp
 * added #include "rect.h"
 *
 * Revision 1.6  1997/04/07 02:57:13  brianp
 * added API.Vertex[23] functions
 *
 * Revision 1.5  1997/04/01 04:25:18  brianp
 * added pointer for LoadIdentity, changed #include's
 *
 * Revision 1.4  1997/02/10 19:49:29  brianp
 * added glResizeBuffersMESA() code
 *
 * Revision 1.3  1997/02/09 18:50:42  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.2  1996/10/16 00:52:22  brianp
 * gl_initialize_api_function_pointers() now gl_init_api_function_pointers()
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "accum.h"
#include "alpha.h"
#include "attrib.h"
#include "bitmap.h"
#include "blend.h"
#include "clip.h"
#include "context.h"
#include "colortab.h"
#include "copypix.h"
#include "depth.h"
#include "drawpix.h"
#include "enable.h"
#include "eval.h"
#include "feedback.h"
#include "fog.h"
#include "get.h"
#include "light.h"
#include "lines.h"
#include "dlist.h"
#include "logic.h"
#include "macros.h"
#include "masking.h"
#include "matrix.h"
#include "misc.h"
#include "pixel.h"
#include "points.h"
#include "polygon.h"
#include "rastpos.h"
#include "readpix.h"
#include "rect.h"
#include "scissor.h"
#include "stencil.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "types.h"
#include "varray.h"
#include "vbfill.h"
#endif



/*
 * For debugging
 */
static void check_pointers( struct gl_api_table *table )
{
   void **entry;
   int numentries = sizeof( struct gl_api_table ) / sizeof(void*);
   int i;

   entry = (void **) table;

   for (i=0;i<numentries;i++) {
      if (!entry[i]) {
         printf("found uninitialized function pointer at %d\n", i );
         gl_problem(NULL, "Missing pointer in pointers.c");
         /*abort()*/
      }
   }
}


/*
 * Assign all the pointers in 'table' to point to Mesa's immediate-mode
 * execution functions.
 */
static void init_exec_pointers( struct gl_api_table *table )
{
   table->Accum = gl_Accum;
   table->AlphaFunc = gl_AlphaFunc;
   table->AreTexturesResident = gl_AreTexturesResident;
   table->ArrayElement = gl_ArrayElement;
   table->Begin = gl_Begin;
   table->BindTexture = gl_BindTexture;
   table->Bitmap = gl_Bitmap;
   table->BlendFunc = gl_BlendFunc;
   table->CallList = gl_CallList;
   table->CallLists = gl_CallLists;
   table->Clear = gl_Clear;
   table->ClearAccum = gl_ClearAccum;
   table->ClearColor = gl_ClearColor;
   table->ClearDepth = gl_ClearDepth;
   table->ClearIndex = gl_ClearIndex;
   table->ClearStencil = gl_ClearStencil;
   table->ClipPlane = gl_ClipPlane;
   table->Color3f = gl_Color3f;
   table->Color3fv = gl_Color3fv;
   table->Color4f = gl_Color4f;
   table->Color4fv = gl_Color4fv;
   table->Color4ub = gl_Color4ub;
   table->Color4ubv = gl_Color4ubv;
   table->ColorMask = gl_ColorMask;
   table->ColorMaterial = gl_ColorMaterial;
   table->ColorPointer = gl_ColorPointer;
   table->ColorTable = gl_ColorTable;
   table->ColorSubTable = gl_ColorSubTable;
   table->CopyPixels = gl_CopyPixels;
   table->CopyTexImage1D = gl_CopyTexImage1D;
   table->CopyTexImage2D = gl_CopyTexImage2D;
   table->CopyTexSubImage1D = gl_CopyTexSubImage1D;
   table->CopyTexSubImage2D = gl_CopyTexSubImage2D;
   table->CullFace = gl_CullFace;
   table->DeleteLists = gl_DeleteLists;
   table->DeleteTextures = gl_DeleteTextures;
   table->DepthFunc = gl_DepthFunc;
   table->DepthMask = gl_DepthMask;
   table->DepthRange = gl_DepthRange;
   table->Disable = gl_Disable;
   table->DisableClientState = gl_DisableClientState;
   table->DrawArrays = gl_DrawArrays;
   table->DrawBuffer = gl_DrawBuffer;
   table->DrawElements = gl_DrawElements;
   table->DrawPixels = gl_DrawPixels;
   table->EdgeFlag = gl_EdgeFlag;
   table->EdgeFlagPointer = gl_EdgeFlagPointer;
   table->Enable = gl_Enable;
   table->EnableClientState = gl_EnableClientState;
   table->End = gl_End;
   table->EndList = gl_EndList;
   table->EvalCoord1f = gl_EvalCoord1f;
   table->EvalCoord2f = gl_EvalCoord2f;
   table->EvalMesh1 = gl_EvalMesh1;
   table->EvalMesh2 = gl_EvalMesh2;
   table->EvalPoint1 = gl_EvalPoint1;
   table->EvalPoint2 = gl_EvalPoint2;
   table->FeedbackBuffer = gl_FeedbackBuffer;
   table->Finish = gl_Finish;
   table->Flush = gl_Flush;
   table->Fogfv = gl_Fogfv;
   table->FrontFace = gl_FrontFace;
   table->Frustum = gl_Frustum;
   table->GenLists = gl_GenLists;
   table->GenTextures = gl_GenTextures;
   table->GetBooleanv = gl_GetBooleanv;
   table->GetClipPlane = gl_GetClipPlane;
   table->GetColorTable = gl_GetColorTable;
   table->GetColorTableParameteriv = gl_GetColorTableParameteriv;
   table->GetDoublev = gl_GetDoublev;
   table->GetError = gl_GetError;
   table->GetFloatv = gl_GetFloatv;
   table->GetIntegerv = gl_GetIntegerv;
   table->GetPointerv = gl_GetPointerv;
   table->GetLightfv = gl_GetLightfv;
   table->GetLightiv = gl_GetLightiv;
   table->GetMapdv = gl_GetMapdv;
   table->GetMapfv = gl_GetMapfv;
   table->GetMapiv = gl_GetMapiv;
   table->GetMaterialfv = gl_GetMaterialfv;
   table->GetMaterialiv = gl_GetMaterialiv;
   table->GetPixelMapfv = gl_GetPixelMapfv;
   table->GetPixelMapuiv = gl_GetPixelMapuiv;
   table->GetPixelMapusv = gl_GetPixelMapusv;
   table->GetPolygonStipple = gl_GetPolygonStipple;
   table->GetString = gl_GetString;
   table->GetTexEnvfv = gl_GetTexEnvfv;
   table->GetTexEnviv = gl_GetTexEnviv;
   table->GetTexGendv = gl_GetTexGendv;
   table->GetTexGenfv = gl_GetTexGenfv;
   table->GetTexGeniv = gl_GetTexGeniv;
   table->GetTexImage = gl_GetTexImage;
   table->GetTexLevelParameterfv = gl_GetTexLevelParameterfv;
   table->GetTexLevelParameteriv = gl_GetTexLevelParameteriv;
   table->GetTexParameterfv = gl_GetTexParameterfv;
   table->GetTexParameteriv = gl_GetTexParameteriv;
   table->Hint = gl_Hint;
   table->Indexf = gl_Indexf;
   table->Indexi = gl_Indexi;
   table->IndexMask = gl_IndexMask;
   table->IndexPointer = gl_IndexPointer;
   table->InitNames = gl_InitNames;
   table->InterleavedArrays = gl_InterleavedArrays;
   table->IsEnabled = gl_IsEnabled;
   table->IsList = gl_IsList;
   table->IsTexture = gl_IsTexture;
   table->LightModelfv = gl_LightModelfv;
   table->Lightfv = gl_Lightfv;
   table->LineStipple = gl_LineStipple;
   table->LineWidth = gl_LineWidth;
   table->ListBase = gl_ListBase;
   table->LoadIdentity = gl_LoadIdentity;
   table->LoadMatrixf = gl_LoadMatrixf;
   table->LoadName = gl_LoadName;
   table->LogicOp = gl_LogicOp;
   table->Map1f = gl_Map1f;
   table->Map2f = gl_Map2f;
   table->MapGrid1f = gl_MapGrid1f;
   table->MapGrid2f = gl_MapGrid2f;
   table->Materialfv = gl_Materialfv;
   table->MatrixMode = gl_MatrixMode;
   table->MultMatrixf = gl_MultMatrixf;
   table->NewList = gl_NewList;
   table->Normal3f = gl_Normal3f;
   table->NormalPointer = gl_NormalPointer;
   table->Normal3fv = gl_Normal3fv;
   table->Ortho = gl_Ortho;
   table->PassThrough = gl_PassThrough;
   table->PixelMapfv = gl_PixelMapfv;
   table->PixelStorei = gl_PixelStorei;
   table->PixelTransferf = gl_PixelTransferf;
   table->PixelZoom = gl_PixelZoom;
   table->PointSize = gl_PointSize;
   table->PolygonMode = gl_PolygonMode;
   table->PolygonOffset = gl_PolygonOffset;
   table->PolygonStipple = gl_PolygonStipple;
   table->PopAttrib = gl_PopAttrib;
   table->PopClientAttrib = gl_PopClientAttrib;
   table->PopMatrix = gl_PopMatrix;
   table->PopName = gl_PopName;
   table->PrioritizeTextures = gl_PrioritizeTextures;
   table->PushAttrib = gl_PushAttrib;
   table->PushClientAttrib = gl_PushClientAttrib;
   table->PushMatrix = gl_PushMatrix;
   table->PushName = gl_PushName;
   table->RasterPos4f = gl_RasterPos4f;
   table->ReadBuffer = gl_ReadBuffer;
   table->ReadPixels = gl_ReadPixels;
   table->Rectf = gl_Rectf;
   table->RenderMode = gl_RenderMode;
   table->Rotatef = gl_Rotatef;
   table->Scalef = gl_Scalef;
   table->Scissor = gl_Scissor;
   table->SelectBuffer = gl_SelectBuffer;
   table->ShadeModel = gl_ShadeModel;
   table->StencilFunc = gl_StencilFunc;
   table->StencilMask = gl_StencilMask;
   table->StencilOp = gl_StencilOp;
   table->TexCoord2f = gl_TexCoord2f;
   table->TexCoord4f = gl_TexCoord4f;
   table->TexCoordPointer = gl_TexCoordPointer;
   table->TexEnvfv = gl_TexEnvfv;
   table->TexGenfv = gl_TexGenfv;
   table->TexImage1D = gl_TexImage1D;
   table->TexImage2D = gl_TexImage2D;
   table->TexSubImage1D = gl_TexSubImage1D;
   table->TexSubImage2D = gl_TexSubImage2D;
   table->TexParameterfv = gl_TexParameterfv;
   table->Translatef = gl_Translatef;
   table->Vertex2f = gl_vertex2f_nop;
   table->Vertex3f = gl_vertex3f_nop;
   table->Vertex4f = gl_vertex4f_nop;
   table->Vertex3fv = gl_vertex3fv_nop;
   table->VertexPointer = gl_VertexPointer;
   table->Viewport = gl_Viewport;
}



/*
 * Assign all the pointers in 'table' to point to Mesa's display list
 * building functions.
 */
static void init_dlist_pointers( struct gl_api_table *table )
{
   table->Accum = gl_save_Accum;
   table->AlphaFunc = gl_save_AlphaFunc;
   table->AreTexturesResident = gl_AreTexturesResident;
   table->ArrayElement = gl_save_ArrayElement;
   table->Begin = gl_save_Begin;
   table->BindTexture = gl_save_BindTexture;
   table->Bitmap = gl_save_Bitmap;
   table->BlendFunc = gl_save_BlendFunc;
   table->CallList = gl_save_CallList;
   table->CallLists = gl_save_CallLists;
   table->Clear = gl_save_Clear;
   table->ClearAccum = gl_save_ClearAccum;
   table->ClearColor = gl_save_ClearColor;
   table->ClearDepth = gl_save_ClearDepth;
   table->ClearIndex = gl_save_ClearIndex;
   table->ClearStencil = gl_save_ClearStencil;
   table->ClipPlane = gl_save_ClipPlane;
   table->Color3f = gl_save_Color3f;
   table->Color3fv = gl_save_Color3fv;
   table->Color4f = gl_save_Color4f;
   table->Color4fv = gl_save_Color4fv;
   table->Color4ub = gl_save_Color4ub;
   table->Color4ubv = gl_save_Color4ubv;
   table->ColorMask = gl_save_ColorMask;
   table->ColorMaterial = gl_save_ColorMaterial;
   table->ColorPointer = gl_ColorPointer;
   table->ColorTable = gl_save_ColorTable;
   table->ColorSubTable = gl_save_ColorSubTable;
   table->CopyPixels = gl_save_CopyPixels;
   table->CopyTexImage1D = gl_save_CopyTexImage1D;
   table->CopyTexImage2D = gl_save_CopyTexImage2D;
   table->CopyTexSubImage1D = gl_save_CopyTexSubImage1D;
   table->CopyTexSubImage2D = gl_save_CopyTexSubImage2D;
   table->CullFace = gl_save_CullFace;
   table->DeleteLists = gl_DeleteLists;   /* NOT SAVED */
   table->DeleteTextures = gl_DeleteTextures;  /* NOT SAVED */
   table->DepthFunc = gl_save_DepthFunc;
   table->DepthMask = gl_save_DepthMask;
   table->DepthRange = gl_save_DepthRange;
   table->Disable = gl_save_Disable;
   table->DisableClientState = gl_DisableClientState;  /* NOT SAVED */
   table->DrawArrays = gl_save_DrawArrays;
   table->DrawBuffer = gl_save_DrawBuffer;
   table->DrawElements = gl_save_DrawElements;
   table->DrawPixels = gl_DrawPixels;   /* SPECIAL CASE */
   table->EdgeFlag = gl_save_EdgeFlag;
   table->EdgeFlagPointer = gl_EdgeFlagPointer;
   table->Enable = gl_save_Enable;
   table->EnableClientState = gl_EnableClientState;   /* NOT SAVED */
   table->End = gl_save_End;
   table->EndList = gl_EndList;   /* NOT SAVED */
   table->EvalCoord1f = gl_save_EvalCoord1f;
   table->EvalCoord2f = gl_save_EvalCoord2f;
   table->EvalMesh1 = gl_save_EvalMesh1;
   table->EvalMesh2 = gl_save_EvalMesh2;
   table->EvalPoint1 = gl_save_EvalPoint1;
   table->EvalPoint2 = gl_save_EvalPoint2;
   table->FeedbackBuffer = gl_FeedbackBuffer;   /* NOT SAVED */
   table->Finish = gl_Finish;   /* NOT SAVED */
   table->Flush = gl_Flush;   /* NOT SAVED */
   table->Fogfv = gl_save_Fogfv;
   table->FrontFace = gl_save_FrontFace;
   table->Frustum = gl_save_Frustum;
   table->GenLists = gl_GenLists;   /* NOT SAVED */
   table->GenTextures = gl_GenTextures;   /* NOT SAVED */

   /* NONE OF THESE COMMANDS ARE COMPILED INTO DISPLAY LISTS */
   table->GetBooleanv = gl_GetBooleanv;
   table->GetClipPlane = gl_GetClipPlane;
   table->GetColorTable = gl_GetColorTable;
   table->GetColorTableParameteriv = gl_GetColorTableParameteriv;
   table->GetDoublev = gl_GetDoublev;
   table->GetError = gl_GetError;
   table->GetFloatv = gl_GetFloatv;
   table->GetIntegerv = gl_GetIntegerv;
   table->GetString = gl_GetString;
   table->GetLightfv = gl_GetLightfv;
   table->GetLightiv = gl_GetLightiv;
   table->GetMapdv = gl_GetMapdv;
   table->GetMapfv = gl_GetMapfv;
   table->GetMapiv = gl_GetMapiv;
   table->GetMaterialfv = gl_GetMaterialfv;
   table->GetMaterialiv = gl_GetMaterialiv;
   table->GetPixelMapfv = gl_GetPixelMapfv;
   table->GetPixelMapuiv = gl_GetPixelMapuiv;
   table->GetPixelMapusv = gl_GetPixelMapusv;
   table->GetPointerv = gl_GetPointerv;
   table->GetPolygonStipple = gl_GetPolygonStipple;
   table->GetTexEnvfv = gl_GetTexEnvfv;
   table->GetTexEnviv = gl_GetTexEnviv;
   table->GetTexGendv = gl_GetTexGendv;
   table->GetTexGenfv = gl_GetTexGenfv;
   table->GetTexGeniv = gl_GetTexGeniv;
   table->GetTexImage = gl_GetTexImage;
   table->GetTexLevelParameterfv = gl_GetTexLevelParameterfv;
   table->GetTexLevelParameteriv = gl_GetTexLevelParameteriv;
   table->GetTexParameterfv = gl_GetTexParameterfv;
   table->GetTexParameteriv = gl_GetTexParameteriv;

   table->Hint = gl_save_Hint;
   table->IndexMask = gl_save_IndexMask;
   table->Indexf = gl_save_Indexf;
   table->Indexi = gl_save_Indexi;
   table->IndexPointer = gl_IndexPointer;
   table->InitNames = gl_save_InitNames;
   table->InterleavedArrays = gl_save_InterleavedArrays;
   table->IsEnabled = gl_IsEnabled;   /* NOT SAVED */
   table->IsTexture = gl_IsTexture;   /* NOT SAVED */
   table->IsList = gl_IsList;   /* NOT SAVED */
   table->LightModelfv = gl_save_LightModelfv;
   table->Lightfv = gl_save_Lightfv;
   table->LineStipple = gl_save_LineStipple;
   table->LineWidth = gl_save_LineWidth;
   table->ListBase = gl_save_ListBase;
   table->LoadIdentity = gl_save_LoadIdentity;
   table->LoadMatrixf = gl_save_LoadMatrixf;
   table->LoadName = gl_save_LoadName;
   table->LogicOp = gl_save_LogicOp;
   table->Map1f = gl_save_Map1f;
   table->Map2f = gl_save_Map2f;
   table->MapGrid1f = gl_save_MapGrid1f;
   table->MapGrid2f = gl_save_MapGrid2f;
   table->Materialfv = gl_save_Materialfv;
   table->MatrixMode = gl_save_MatrixMode;
   table->MultMatrixf = gl_save_MultMatrixf;
   table->NewList = gl_save_NewList;
   table->Normal3f = gl_save_Normal3f;
   table->Normal3fv = gl_save_Normal3fv;
   table->NormalPointer = gl_NormalPointer;  /* NOT SAVED */
   table->Ortho = gl_save_Ortho;
   table->PassThrough = gl_save_PassThrough;
   table->PixelMapfv = gl_save_PixelMapfv;
   table->PixelStorei = gl_PixelStorei;   /* NOT SAVED */
   table->PixelTransferf = gl_save_PixelTransferf;
   table->PixelZoom = gl_save_PixelZoom;
   table->PointSize = gl_save_PointSize;
   table->PolygonMode = gl_save_PolygonMode;
   table->PolygonOffset = gl_save_PolygonOffset;
   table->PolygonStipple = gl_save_PolygonStipple;
   table->PopAttrib = gl_save_PopAttrib;
   table->PopClientAttrib = gl_PopClientAttrib;  /* NOT SAVED */
   table->PopMatrix = gl_save_PopMatrix;
   table->PopName = gl_save_PopName;
   table->PrioritizeTextures = gl_save_PrioritizeTextures;
   table->PushAttrib = gl_save_PushAttrib;
   table->PushClientAttrib = gl_PushClientAttrib;  /* NOT SAVED */
   table->PushMatrix = gl_save_PushMatrix;
   table->PushName = gl_save_PushName;
   table->RasterPos4f = gl_save_RasterPos4f;
   table->ReadBuffer = gl_save_ReadBuffer;
   table->ReadPixels = gl_ReadPixels;   /* NOT SAVED */
   table->Rectf = gl_save_Rectf;
   table->RenderMode = gl_RenderMode;   /* NOT SAVED */
   table->Rotatef = gl_save_Rotatef;
   table->Scalef = gl_save_Scalef;
   table->Scissor = gl_save_Scissor;
   table->SelectBuffer = gl_SelectBuffer;   /* NOT SAVED */
   table->ShadeModel = gl_save_ShadeModel;
   table->StencilFunc = gl_save_StencilFunc;
   table->StencilMask = gl_save_StencilMask;
   table->StencilOp = gl_save_StencilOp;
   table->TexCoord2f = gl_save_TexCoord2f;
   table->TexCoord4f = gl_save_TexCoord4f;
   table->TexCoordPointer = gl_TexCoordPointer;  /* NOT SAVED */
   table->TexEnvfv = gl_save_TexEnvfv;
   table->TexGenfv = gl_save_TexGenfv;
   table->TexImage1D = gl_save_TexImage1D;
   table->TexImage2D = gl_save_TexImage2D;
   table->TexSubImage1D = gl_save_TexSubImage1D;
   table->TexSubImage2D = gl_save_TexSubImage2D;
   table->TexParameterfv = gl_save_TexParameterfv;
   table->Translatef = gl_save_Translatef;
   table->Vertex2f = gl_save_Vertex2f;
   table->Vertex3f = gl_save_Vertex3f;
   table->Vertex4f = gl_save_Vertex4f;
   table->Vertex3fv = gl_save_Vertex3fv;
   table->VertexPointer = gl_VertexPointer;  /* NOT SAVED */
   table->Viewport = gl_save_Viewport;
}



void gl_init_api_function_pointers( GLcontext *ctx )
{
   init_exec_pointers( &ctx->Exec );

   init_dlist_pointers( &ctx->Save );

   /* make sure there's no NULL pointers */
   check_pointers( &ctx->Exec );
   check_pointers( &ctx->Save );
}

