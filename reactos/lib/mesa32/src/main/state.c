/**
 * \file state.c
 * State management.
 * 
 * This file manages recalculation of derived values in the __GLcontextRec.
 * Also, this is where we initialize the API dispatch table.
 */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
#include "accum.h"
#include "api_loopback.h"
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
#include "arbprogram.h"
#endif
#include "attrib.h"
#include "blend.h"
#if FEATURE_ARB_vertex_buffer_object
#include "bufferobj.h"
#endif
#include "buffers.h"
#include "clip.h"
#include "colortab.h"
#include "context.h"
#include "convolve.h"
#include "depth.h"
#include "dlist.h"
#include "drawpix.h"
#include "enable.h"
#include "eval.h"
#include "get.h"
#include "feedback.h"
#include "fog.h"
#include "hint.h"
#include "histogram.h"
#include "imports.h"
#include "light.h"
#include "lines.h"
#include "macros.h"
#include "matrix.h"
#if FEATURE_ARB_occlusion_query
#include "occlude.h"
#endif
#include "pixel.h"
#include "points.h"
#include "polygon.h"
#include "rastpos.h"
#include "state.h"
#include "stencil.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "mtypes.h"
#include "varray.h"
#if FEATURE_NV_vertex_program
#include "nvprogram.h"
#endif
#if FEATURE_NV_fragment_program
#include "nvfragprog.h"
#include "nvprogram.h"
#include "program.h"
#endif
#include "debug.h"

/* #include "math/m_matrix.h" */
/* #include "math/m_xform.h" */


/**********************************************************************/
/** \name Dispatch table setup */
/*@{*/

/**
 * Generic no-op dispatch function.
 *
 * Used in replacement of the functions which are not part of Mesa subset.
 *
 * Displays a message.
 */
static int
generic_noop(void)
{
   _mesa_problem(NULL, "User called no-op dispatch function (not part of Mesa subset?)");
   return 0;
}


/**
 * Set all pointers in the given dispatch table to point to a
 * generic no-op function - generic_noop().
 *
 * \param table dispatch table.
 * \param tableSize dispatch table size.
 */
void
_mesa_init_no_op_table(struct _glapi_table *table, GLuint tableSize)
{
   GLuint i;
   void **dispatch = (void **) table;
   for (i = 0; i < tableSize; i++) {
      dispatch[i] = (void *) generic_noop;
   }
}


/**
 * Initialize a dispatch table with pointers to Mesa's immediate-mode
 * commands.
 *
 * Pointers to glBegin()/glEnd() object commands and a few others
 * are provided via the GLvertexformat interface.
 *
 * \param exec dispatch table.
 * \param tableSize dispatch table size.
 */
void
_mesa_init_exec_table(struct _glapi_table *exec, GLuint tableSize)
{
   /* first initialize all dispatch slots to no-op */
   _mesa_init_no_op_table(exec, tableSize);

#if _HAVE_FULL_GL
   _mesa_loopback_init_api_table( exec );
#endif

   /* load the dispatch slots we understand */
   exec->AlphaFunc = _mesa_AlphaFunc;
   exec->BlendFunc = _mesa_BlendFunc;
   exec->Clear = _mesa_Clear;
   exec->ClearColor = _mesa_ClearColor;
   exec->ClearStencil = _mesa_ClearStencil;
   exec->ColorMask = _mesa_ColorMask;
   exec->CullFace = _mesa_CullFace;
   exec->Disable = _mesa_Disable;
   exec->DrawBuffer = _mesa_DrawBuffer;
   exec->Enable = _mesa_Enable;
   exec->Finish = _mesa_Finish;
   exec->Flush = _mesa_Flush;
   exec->FrontFace = _mesa_FrontFace;
   exec->Frustum = _mesa_Frustum;
   exec->GetError = _mesa_GetError;
   exec->GetFloatv = _mesa_GetFloatv;
   exec->GetString = _mesa_GetString;
   exec->InitNames = _mesa_InitNames;
   exec->LineStipple = _mesa_LineStipple;
   exec->LineWidth = _mesa_LineWidth;
   exec->LoadIdentity = _mesa_LoadIdentity;
   exec->LoadMatrixf = _mesa_LoadMatrixf;
   exec->LoadName = _mesa_LoadName;
   exec->LogicOp = _mesa_LogicOp;
   exec->MatrixMode = _mesa_MatrixMode;
   exec->MultMatrixf = _mesa_MultMatrixf;
   exec->Ortho = _mesa_Ortho;
   exec->PixelStorei = _mesa_PixelStorei;
   exec->PopMatrix = _mesa_PopMatrix;
   exec->PopName = _mesa_PopName;
   exec->PushMatrix = _mesa_PushMatrix;
   exec->PushName = _mesa_PushName;
   exec->RasterPos2f = _mesa_RasterPos2f;
   exec->RasterPos2fv = _mesa_RasterPos2fv;
   exec->RasterPos2i = _mesa_RasterPos2i;
   exec->RasterPos2iv = _mesa_RasterPos2iv;
   exec->ReadBuffer = _mesa_ReadBuffer;
   exec->RenderMode = _mesa_RenderMode;
   exec->Rotatef = _mesa_Rotatef;
   exec->Scalef = _mesa_Scalef;
   exec->Scissor = _mesa_Scissor;
   exec->SelectBuffer = _mesa_SelectBuffer;
   exec->ShadeModel = _mesa_ShadeModel;
   exec->StencilFunc = _mesa_StencilFunc;
   exec->StencilMask = _mesa_StencilMask;
   exec->StencilOp = _mesa_StencilOp;
   exec->TexEnvfv = _mesa_TexEnvfv;
   exec->TexEnvi = _mesa_TexEnvi;
   exec->TexImage2D = _mesa_TexImage2D;
   exec->TexParameteri = _mesa_TexParameteri; 
   exec->Translatef = _mesa_Translatef;
   exec->Viewport = _mesa_Viewport;
#if _HAVE_FULL_GL
   exec->Accum = _mesa_Accum;
   exec->Bitmap = _mesa_Bitmap;
   exec->CallList = _mesa_CallList;
   exec->CallLists = _mesa_CallLists;
   exec->ClearAccum = _mesa_ClearAccum;
   exec->ClearDepth = _mesa_ClearDepth;
   exec->ClearIndex = _mesa_ClearIndex;
   exec->ClipPlane = _mesa_ClipPlane;
   exec->ColorMaterial = _mesa_ColorMaterial;
   exec->CopyPixels = _mesa_CopyPixels;
   exec->DeleteLists = _mesa_DeleteLists;
   exec->DepthFunc = _mesa_DepthFunc;
   exec->DepthMask = _mesa_DepthMask;
   exec->DepthRange = _mesa_DepthRange;
   exec->DrawPixels = _mesa_DrawPixels;
   exec->EndList = _mesa_EndList;
   exec->FeedbackBuffer = _mesa_FeedbackBuffer;
   exec->FogCoordPointerEXT = _mesa_FogCoordPointerEXT;
   exec->Fogf = _mesa_Fogf;
   exec->Fogfv = _mesa_Fogfv;
   exec->Fogi = _mesa_Fogi;
   exec->Fogiv = _mesa_Fogiv;
   exec->GenLists = _mesa_GenLists;
   exec->GetClipPlane = _mesa_GetClipPlane;
   exec->GetBooleanv = _mesa_GetBooleanv;
   exec->GetDoublev = _mesa_GetDoublev;
   exec->GetIntegerv = _mesa_GetIntegerv;
   exec->GetLightfv = _mesa_GetLightfv;
   exec->GetLightiv = _mesa_GetLightiv;
   exec->GetMapdv = _mesa_GetMapdv;
   exec->GetMapfv = _mesa_GetMapfv;
   exec->GetMapiv = _mesa_GetMapiv;
   exec->GetMaterialfv = _mesa_GetMaterialfv;
   exec->GetMaterialiv = _mesa_GetMaterialiv;
   exec->GetPixelMapfv = _mesa_GetPixelMapfv;
   exec->GetPixelMapuiv = _mesa_GetPixelMapuiv;
   exec->GetPixelMapusv = _mesa_GetPixelMapusv;
   exec->GetPolygonStipple = _mesa_GetPolygonStipple;
   exec->GetTexEnvfv = _mesa_GetTexEnvfv;
   exec->GetTexEnviv = _mesa_GetTexEnviv;
   exec->GetTexLevelParameterfv = _mesa_GetTexLevelParameterfv;
   exec->GetTexLevelParameteriv = _mesa_GetTexLevelParameteriv;
   exec->GetTexParameterfv = _mesa_GetTexParameterfv;
   exec->GetTexParameteriv = _mesa_GetTexParameteriv;
   exec->GetTexGendv = _mesa_GetTexGendv;
   exec->GetTexGenfv = _mesa_GetTexGenfv;
   exec->GetTexGeniv = _mesa_GetTexGeniv;
   exec->GetTexImage = _mesa_GetTexImage;
   exec->Hint = _mesa_Hint;
   exec->IndexMask = _mesa_IndexMask;
   exec->IsEnabled = _mesa_IsEnabled;
   exec->IsList = _mesa_IsList;
   exec->LightModelf = _mesa_LightModelf;
   exec->LightModelfv = _mesa_LightModelfv;
   exec->LightModeli = _mesa_LightModeli;
   exec->LightModeliv = _mesa_LightModeliv;
   exec->Lightf = _mesa_Lightf;
   exec->Lightfv = _mesa_Lightfv;
   exec->Lighti = _mesa_Lighti;
   exec->Lightiv = _mesa_Lightiv;
   exec->ListBase = _mesa_ListBase;
   exec->LoadMatrixd = _mesa_LoadMatrixd;
   exec->Map1d = _mesa_Map1d;
   exec->Map1f = _mesa_Map1f;
   exec->Map2d = _mesa_Map2d;
   exec->Map2f = _mesa_Map2f;
   exec->MapGrid1d = _mesa_MapGrid1d;
   exec->MapGrid1f = _mesa_MapGrid1f;
   exec->MapGrid2d = _mesa_MapGrid2d;
   exec->MapGrid2f = _mesa_MapGrid2f;
   exec->MultMatrixd = _mesa_MultMatrixd;
   exec->NewList = _mesa_NewList;
   exec->PassThrough = _mesa_PassThrough;
   exec->PixelMapfv = _mesa_PixelMapfv;
   exec->PixelMapuiv = _mesa_PixelMapuiv;
   exec->PixelMapusv = _mesa_PixelMapusv;
   exec->PixelStoref = _mesa_PixelStoref;
   exec->PixelTransferf = _mesa_PixelTransferf;
   exec->PixelTransferi = _mesa_PixelTransferi;
   exec->PixelZoom = _mesa_PixelZoom;
   exec->PointSize = _mesa_PointSize;
   exec->PolygonMode = _mesa_PolygonMode;
   exec->PolygonOffset = _mesa_PolygonOffset;
   exec->PolygonStipple = _mesa_PolygonStipple;
   exec->PopAttrib = _mesa_PopAttrib;
   exec->PushAttrib = _mesa_PushAttrib;
   exec->RasterPos2d = _mesa_RasterPos2d;
   exec->RasterPos2dv = _mesa_RasterPos2dv;
   exec->RasterPos2s = _mesa_RasterPos2s;
   exec->RasterPos2sv = _mesa_RasterPos2sv;
   exec->RasterPos3d = _mesa_RasterPos3d;
   exec->RasterPos3dv = _mesa_RasterPos3dv;
   exec->RasterPos3f = _mesa_RasterPos3f;
   exec->RasterPos3fv = _mesa_RasterPos3fv;
   exec->RasterPos3i = _mesa_RasterPos3i;
   exec->RasterPos3iv = _mesa_RasterPos3iv;
   exec->RasterPos3s = _mesa_RasterPos3s;
   exec->RasterPos3sv = _mesa_RasterPos3sv;
   exec->RasterPos4d = _mesa_RasterPos4d;
   exec->RasterPos4dv = _mesa_RasterPos4dv;
   exec->RasterPos4f = _mesa_RasterPos4f;
   exec->RasterPos4fv = _mesa_RasterPos4fv;
   exec->RasterPos4i = _mesa_RasterPos4i;
   exec->RasterPos4iv = _mesa_RasterPos4iv;
   exec->RasterPos4s = _mesa_RasterPos4s;
   exec->RasterPos4sv = _mesa_RasterPos4sv;
   exec->ReadPixels = _mesa_ReadPixels;
   exec->Rotated = _mesa_Rotated;
   exec->Scaled = _mesa_Scaled;
   exec->SecondaryColorPointerEXT = _mesa_SecondaryColorPointerEXT;
   exec->TexEnvf = _mesa_TexEnvf;
   exec->TexEnviv = _mesa_TexEnviv;
   exec->TexGend = _mesa_TexGend;
   exec->TexGendv = _mesa_TexGendv;
   exec->TexGenf = _mesa_TexGenf;
   exec->TexGenfv = _mesa_TexGenfv;
   exec->TexGeni = _mesa_TexGeni;
   exec->TexGeniv = _mesa_TexGeniv;
   exec->TexImage1D = _mesa_TexImage1D;
   exec->TexParameterf = _mesa_TexParameterf;
   exec->TexParameterfv = _mesa_TexParameterfv;
   exec->TexParameteriv = _mesa_TexParameteriv;
   exec->Translated = _mesa_Translated;
#endif

   /* 1.1 */
   exec->BindTexture = _mesa_BindTexture;
   exec->DeleteTextures = _mesa_DeleteTextures;
   exec->GenTextures = _mesa_GenTextures;
#if _HAVE_FULL_GL
   exec->AreTexturesResident = _mesa_AreTexturesResident;
   exec->AreTexturesResidentEXT = _mesa_AreTexturesResident;
   exec->ColorPointer = _mesa_ColorPointer;
   exec->CopyTexImage1D = _mesa_CopyTexImage1D;
   exec->CopyTexImage2D = _mesa_CopyTexImage2D;
   exec->CopyTexSubImage1D = _mesa_CopyTexSubImage1D;
   exec->CopyTexSubImage2D = _mesa_CopyTexSubImage2D;
   exec->DisableClientState = _mesa_DisableClientState;
   exec->EdgeFlagPointer = _mesa_EdgeFlagPointer;
   exec->EnableClientState = _mesa_EnableClientState;
   exec->GenTexturesEXT = _mesa_GenTextures;
   exec->GetPointerv = _mesa_GetPointerv;
   exec->IndexPointer = _mesa_IndexPointer;
   exec->InterleavedArrays = _mesa_InterleavedArrays;
   exec->IsTexture = _mesa_IsTexture;
   exec->IsTextureEXT = _mesa_IsTexture;
   exec->NormalPointer = _mesa_NormalPointer;
   exec->PopClientAttrib = _mesa_PopClientAttrib;
   exec->PrioritizeTextures = _mesa_PrioritizeTextures;
   exec->PushClientAttrib = _mesa_PushClientAttrib;
   exec->TexCoordPointer = _mesa_TexCoordPointer;
   exec->TexSubImage1D = _mesa_TexSubImage1D;
   exec->TexSubImage2D = _mesa_TexSubImage2D;
   exec->VertexPointer = _mesa_VertexPointer;
#endif

   /* 1.2 */
#if _HAVE_FULL_GL
   exec->CopyTexSubImage3D = _mesa_CopyTexSubImage3D;
   exec->TexImage3D = _mesa_TexImage3D;
   exec->TexSubImage3D = _mesa_TexSubImage3D;
#endif

   /* OpenGL 1.2  GL_ARB_imaging */
#if _HAVE_FULL_GL
   exec->BlendColor = _mesa_BlendColor;
   exec->BlendEquation = _mesa_BlendEquation;
   exec->ColorSubTable = _mesa_ColorSubTable;
   exec->ColorTable = _mesa_ColorTable;
   exec->ColorTableParameterfv = _mesa_ColorTableParameterfv;
   exec->ColorTableParameteriv = _mesa_ColorTableParameteriv;
   exec->ConvolutionFilter1D = _mesa_ConvolutionFilter1D;
   exec->ConvolutionFilter2D = _mesa_ConvolutionFilter2D;
   exec->ConvolutionParameterf = _mesa_ConvolutionParameterf;
   exec->ConvolutionParameterfv = _mesa_ConvolutionParameterfv;
   exec->ConvolutionParameteri = _mesa_ConvolutionParameteri;
   exec->ConvolutionParameteriv = _mesa_ConvolutionParameteriv;
   exec->CopyColorSubTable = _mesa_CopyColorSubTable;
   exec->CopyColorTable = _mesa_CopyColorTable;
   exec->CopyConvolutionFilter1D = _mesa_CopyConvolutionFilter1D;
   exec->CopyConvolutionFilter2D = _mesa_CopyConvolutionFilter2D;
   exec->GetColorTable = _mesa_GetColorTable;
   exec->GetColorTableEXT = _mesa_GetColorTable;
   exec->GetColorTableParameterfv = _mesa_GetColorTableParameterfv;
   exec->GetColorTableParameterfvEXT = _mesa_GetColorTableParameterfv;
   exec->GetColorTableParameteriv = _mesa_GetColorTableParameteriv;
   exec->GetColorTableParameterivEXT = _mesa_GetColorTableParameteriv;
   exec->GetConvolutionFilter = _mesa_GetConvolutionFilter;
   exec->GetConvolutionFilterEXT = _mesa_GetConvolutionFilter;
   exec->GetConvolutionParameterfv = _mesa_GetConvolutionParameterfv;
   exec->GetConvolutionParameterfvEXT = _mesa_GetConvolutionParameterfv;
   exec->GetConvolutionParameteriv = _mesa_GetConvolutionParameteriv;
   exec->GetConvolutionParameterivEXT = _mesa_GetConvolutionParameteriv;
   exec->GetHistogram = _mesa_GetHistogram;
   exec->GetHistogramEXT = _mesa_GetHistogram;
   exec->GetHistogramParameterfv = _mesa_GetHistogramParameterfv;
   exec->GetHistogramParameterfvEXT = _mesa_GetHistogramParameterfv;
   exec->GetHistogramParameteriv = _mesa_GetHistogramParameteriv;
   exec->GetHistogramParameterivEXT = _mesa_GetHistogramParameteriv;
   exec->GetMinmax = _mesa_GetMinmax;
   exec->GetMinmaxEXT = _mesa_GetMinmax;
   exec->GetMinmaxParameterfv = _mesa_GetMinmaxParameterfv;
   exec->GetMinmaxParameterfvEXT = _mesa_GetMinmaxParameterfv;
   exec->GetMinmaxParameteriv = _mesa_GetMinmaxParameteriv;
   exec->GetMinmaxParameterivEXT = _mesa_GetMinmaxParameteriv;
   exec->GetSeparableFilter = _mesa_GetSeparableFilter;
   exec->GetSeparableFilterEXT = _mesa_GetSeparableFilter;
   exec->Histogram = _mesa_Histogram;
   exec->Minmax = _mesa_Minmax;
   exec->ResetHistogram = _mesa_ResetHistogram;
   exec->ResetMinmax = _mesa_ResetMinmax;
   exec->SeparableFilter2D = _mesa_SeparableFilter2D;
#endif

   /* 2. GL_EXT_blend_color */
#if 0
/*    exec->BlendColorEXT = _mesa_BlendColorEXT; */
#endif

   /* 3. GL_EXT_polygon_offset */
#if _HAVE_FULL_GL
   exec->PolygonOffsetEXT = _mesa_PolygonOffsetEXT;
#endif

   /* 6. GL_EXT_texture3d */
#if 0
/*    exec->CopyTexSubImage3DEXT = _mesa_CopyTexSubImage3D; */
/*    exec->TexImage3DEXT = _mesa_TexImage3DEXT; */
/*    exec->TexSubImage3DEXT = _mesa_TexSubImage3D; */
#endif

   /* 11. GL_EXT_histogram */
#if _HAVE_FULL_GL
   exec->GetHistogramEXT = _mesa_GetHistogram;
   exec->GetHistogramParameterfvEXT = _mesa_GetHistogramParameterfv;
   exec->GetHistogramParameterivEXT = _mesa_GetHistogramParameteriv;
   exec->GetMinmaxEXT = _mesa_GetMinmax;
   exec->GetMinmaxParameterfvEXT = _mesa_GetMinmaxParameterfv;
   exec->GetMinmaxParameterivEXT = _mesa_GetMinmaxParameteriv;
#endif

   /* ?. GL_SGIX_pixel_texture */
#if _HAVE_FULL_GL
   exec->PixelTexGenSGIX = _mesa_PixelTexGenSGIX;
#endif

   /* 15. GL_SGIS_pixel_texture */
#if _HAVE_FULL_GL
   exec->PixelTexGenParameteriSGIS = _mesa_PixelTexGenParameteriSGIS;
   exec->PixelTexGenParameterivSGIS = _mesa_PixelTexGenParameterivSGIS;
   exec->PixelTexGenParameterfSGIS = _mesa_PixelTexGenParameterfSGIS;
   exec->PixelTexGenParameterfvSGIS = _mesa_PixelTexGenParameterfvSGIS;
   exec->GetPixelTexGenParameterivSGIS = _mesa_GetPixelTexGenParameterivSGIS;
   exec->GetPixelTexGenParameterfvSGIS = _mesa_GetPixelTexGenParameterfvSGIS;
#endif

   /* 30. GL_EXT_vertex_array */
#if _HAVE_FULL_GL
   exec->ColorPointerEXT = _mesa_ColorPointerEXT;
   exec->EdgeFlagPointerEXT = _mesa_EdgeFlagPointerEXT;
   exec->IndexPointerEXT = _mesa_IndexPointerEXT;
   exec->NormalPointerEXT = _mesa_NormalPointerEXT;
   exec->TexCoordPointerEXT = _mesa_TexCoordPointerEXT;
   exec->VertexPointerEXT = _mesa_VertexPointerEXT;
#endif

   /* 37. GL_EXT_blend_minmax */
#if 0
   exec->BlendEquationEXT = _mesa_BlendEquationEXT;
#endif

   /* 54. GL_EXT_point_parameters */
#if _HAVE_FULL_GL
   exec->PointParameterfEXT = _mesa_PointParameterfEXT;
   exec->PointParameterfvEXT = _mesa_PointParameterfvEXT;
#endif

   /* 78. GL_EXT_paletted_texture */
#if 0
   exec->ColorTableEXT = _mesa_ColorTableEXT;
   exec->ColorSubTableEXT = _mesa_ColorSubTableEXT;
#endif
#if _HAVE_FULL_GL
   exec->GetColorTableEXT = _mesa_GetColorTable;
   exec->GetColorTableParameterfvEXT = _mesa_GetColorTableParameterfv;
   exec->GetColorTableParameterivEXT = _mesa_GetColorTableParameteriv;
#endif

   /* 97. GL_EXT_compiled_vertex_array */
#if _HAVE_FULL_GL
   exec->LockArraysEXT = _mesa_LockArraysEXT;
   exec->UnlockArraysEXT = _mesa_UnlockArraysEXT;
#endif

   /* 148. GL_EXT_multi_draw_arrays */
#if _HAVE_FULL_GL
   exec->MultiDrawArraysEXT = _mesa_MultiDrawArraysEXT;
   exec->MultiDrawElementsEXT = _mesa_MultiDrawElementsEXT;
#endif

   /* 173. GL_INGR_blend_func_separate */
#if _HAVE_FULL_GL
   exec->BlendFuncSeparateEXT = _mesa_BlendFuncSeparateEXT;
#endif

   /* 196. GL_MESA_resize_buffers */
#if _HAVE_FULL_GL
   exec->ResizeBuffersMESA = _mesa_ResizeBuffersMESA;
#endif

   /* 197. GL_MESA_window_pos */
#if _HAVE_FULL_GL
   exec->WindowPos2dMESA = _mesa_WindowPos2dMESA;
   exec->WindowPos2dvMESA = _mesa_WindowPos2dvMESA;
   exec->WindowPos2fMESA = _mesa_WindowPos2fMESA;
   exec->WindowPos2fvMESA = _mesa_WindowPos2fvMESA;
   exec->WindowPos2iMESA = _mesa_WindowPos2iMESA;
   exec->WindowPos2ivMESA = _mesa_WindowPos2ivMESA;
   exec->WindowPos2sMESA = _mesa_WindowPos2sMESA;
   exec->WindowPos2svMESA = _mesa_WindowPos2svMESA;
   exec->WindowPos3dMESA = _mesa_WindowPos3dMESA;
   exec->WindowPos3dvMESA = _mesa_WindowPos3dvMESA;
   exec->WindowPos3fMESA = _mesa_WindowPos3fMESA;
   exec->WindowPos3fvMESA = _mesa_WindowPos3fvMESA;
   exec->WindowPos3iMESA = _mesa_WindowPos3iMESA;
   exec->WindowPos3ivMESA = _mesa_WindowPos3ivMESA;
   exec->WindowPos3sMESA = _mesa_WindowPos3sMESA;
   exec->WindowPos3svMESA = _mesa_WindowPos3svMESA;
   exec->WindowPos4dMESA = _mesa_WindowPos4dMESA;
   exec->WindowPos4dvMESA = _mesa_WindowPos4dvMESA;
   exec->WindowPos4fMESA = _mesa_WindowPos4fMESA;
   exec->WindowPos4fvMESA = _mesa_WindowPos4fvMESA;
   exec->WindowPos4iMESA = _mesa_WindowPos4iMESA;
   exec->WindowPos4ivMESA = _mesa_WindowPos4ivMESA;
   exec->WindowPos4sMESA = _mesa_WindowPos4sMESA;
   exec->WindowPos4svMESA = _mesa_WindowPos4svMESA;
#endif

   /* 200. GL_IBM_multimode_draw_arrays */
#if _HAVE_FULL_GL
   exec->MultiModeDrawArraysIBM = _mesa_MultiModeDrawArraysIBM;
   exec->MultiModeDrawElementsIBM = _mesa_MultiModeDrawElementsIBM;
#endif

   /* 233. GL_NV_vertex_program */
#if FEATURE_NV_vertex_program
   exec->BindProgramNV = _mesa_BindProgram;
   exec->DeleteProgramsNV = _mesa_DeletePrograms;
   exec->ExecuteProgramNV = _mesa_ExecuteProgramNV;
   exec->GenProgramsNV = _mesa_GenPrograms;
   exec->AreProgramsResidentNV = _mesa_AreProgramsResidentNV;
   exec->RequestResidentProgramsNV = _mesa_RequestResidentProgramsNV;
   exec->GetProgramParameterfvNV = _mesa_GetProgramParameterfvNV;
   exec->GetProgramParameterdvNV = _mesa_GetProgramParameterdvNV;
   exec->GetProgramivNV = _mesa_GetProgramivNV;
   exec->GetProgramStringNV = _mesa_GetProgramStringNV;
   exec->GetTrackMatrixivNV = _mesa_GetTrackMatrixivNV;
   exec->GetVertexAttribdvNV = _mesa_GetVertexAttribdvNV;
   exec->GetVertexAttribfvNV = _mesa_GetVertexAttribfvNV;
   exec->GetVertexAttribivNV = _mesa_GetVertexAttribivNV;
   exec->GetVertexAttribPointervNV = _mesa_GetVertexAttribPointervNV;
   exec->IsProgramNV = _mesa_IsProgram;
   exec->LoadProgramNV = _mesa_LoadProgramNV;
   exec->ProgramParameter4dNV = _mesa_ProgramParameter4dNV;
   exec->ProgramParameter4dvNV = _mesa_ProgramParameter4dvNV;
   exec->ProgramParameter4fNV = _mesa_ProgramParameter4fNV;
   exec->ProgramParameter4fvNV = _mesa_ProgramParameter4fvNV;
   exec->ProgramParameters4dvNV = _mesa_ProgramParameters4dvNV;
   exec->ProgramParameters4fvNV = _mesa_ProgramParameters4fvNV;
   exec->TrackMatrixNV = _mesa_TrackMatrixNV;
   exec->VertexAttribPointerNV = _mesa_VertexAttribPointerNV;
   /* glVertexAttrib*NV functions handled in api_loopback.c */
#endif

   /* 282. GL_NV_fragment_program */
#if FEATURE_NV_fragment_program
   exec->ProgramNamedParameter4fNV = _mesa_ProgramNamedParameter4fNV;
   exec->ProgramNamedParameter4dNV = _mesa_ProgramNamedParameter4dNV;
   exec->ProgramNamedParameter4fvNV = _mesa_ProgramNamedParameter4fvNV;
   exec->ProgramNamedParameter4dvNV = _mesa_ProgramNamedParameter4dvNV;
   exec->GetProgramNamedParameterfvNV = _mesa_GetProgramNamedParameterfvNV;
   exec->GetProgramNamedParameterdvNV = _mesa_GetProgramNamedParameterdvNV;
   exec->ProgramLocalParameter4dARB = _mesa_ProgramLocalParameter4dARB;
   exec->ProgramLocalParameter4dvARB = _mesa_ProgramLocalParameter4dvARB;
   exec->ProgramLocalParameter4fARB = _mesa_ProgramLocalParameter4fARB;
   exec->ProgramLocalParameter4fvARB = _mesa_ProgramLocalParameter4fvARB;
   exec->GetProgramLocalParameterdvARB = _mesa_GetProgramLocalParameterdvARB;
   exec->GetProgramLocalParameterfvARB = _mesa_GetProgramLocalParameterfvARB;
#endif

   /* 262. GL_NV_point_sprite */
#if _HAVE_FULL_GL
   exec->PointParameteriNV = _mesa_PointParameteriNV;
   exec->PointParameterivNV = _mesa_PointParameterivNV;
#endif

   /* 268. GL_EXT_stencil_two_side */
#if _HAVE_FULL_GL
   exec->ActiveStencilFaceEXT = _mesa_ActiveStencilFaceEXT;
#endif

   /* ???. GL_EXT_depth_bounds_test */
   exec->DepthBoundsEXT = _mesa_DepthBoundsEXT;

   /* ARB 1. GL_ARB_multitexture */
#if _HAVE_FULL_GL
   exec->ActiveTextureARB = _mesa_ActiveTextureARB;
   exec->ClientActiveTextureARB = _mesa_ClientActiveTextureARB;
#endif

   /* ARB 3. GL_ARB_transpose_matrix */
#if _HAVE_FULL_GL
   exec->LoadTransposeMatrixdARB = _mesa_LoadTransposeMatrixdARB;
   exec->LoadTransposeMatrixfARB = _mesa_LoadTransposeMatrixfARB;
   exec->MultTransposeMatrixdARB = _mesa_MultTransposeMatrixdARB;
   exec->MultTransposeMatrixfARB = _mesa_MultTransposeMatrixfARB;
#endif

   /* ARB 5. GL_ARB_multisample */
#if _HAVE_FULL_GL
   exec->SampleCoverageARB = _mesa_SampleCoverageARB;
#endif

   /* ARB 12. GL_ARB_texture_compression */
#if _HAVE_FULL_GL
   exec->CompressedTexImage3DARB = _mesa_CompressedTexImage3DARB;
   exec->CompressedTexImage2DARB = _mesa_CompressedTexImage2DARB;
   exec->CompressedTexImage1DARB = _mesa_CompressedTexImage1DARB;
   exec->CompressedTexSubImage3DARB = _mesa_CompressedTexSubImage3DARB;
   exec->CompressedTexSubImage2DARB = _mesa_CompressedTexSubImage2DARB;
   exec->CompressedTexSubImage1DARB = _mesa_CompressedTexSubImage1DARB;
   exec->GetCompressedTexImageARB = _mesa_GetCompressedTexImageARB;
#endif

   /* ARB 14. GL_ARB_point_parameters */
   /* reuse EXT_point_parameters functions */

   /* ARB 26. GL_ARB_vertex_program */
   /* ARB 27. GL_ARB_fragment_program */
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
   /* glVertexAttrib1sARB aliases glVertexAttrib1sNV */
   /* glVertexAttrib1fARB aliases glVertexAttrib1fNV */
   /* glVertexAttrib1dARB aliases glVertexAttrib1dNV */
   /* glVertexAttrib2sARB aliases glVertexAttrib2sNV */
   /* glVertexAttrib2fARB aliases glVertexAttrib2fNV */
   /* glVertexAttrib2dARB aliases glVertexAttrib2dNV */
   /* glVertexAttrib3sARB aliases glVertexAttrib3sNV */
   /* glVertexAttrib3fARB aliases glVertexAttrib3fNV */
   /* glVertexAttrib3dARB aliases glVertexAttrib3dNV */
   /* glVertexAttrib4sARB aliases glVertexAttrib4sNV */
   /* glVertexAttrib4fARB aliases glVertexAttrib4fNV */
   /* glVertexAttrib4dARB aliases glVertexAttrib4dNV */
   /* glVertexAttrib4NubARB aliases glVertexAttrib4NubNV */
   /* glVertexAttrib1svARB aliases glVertexAttrib1svNV */
   /* glVertexAttrib1fvARB aliases glVertexAttrib1fvNV */
   /* glVertexAttrib1dvARB aliases glVertexAttrib1dvNV */
   /* glVertexAttrib2svARB aliases glVertexAttrib2svNV */
   /* glVertexAttrib2fvARB aliases glVertexAttrib2fvNV */
   /* glVertexAttrib2dvARB aliases glVertexAttrib2dvNV */
   /* glVertexAttrib3svARB aliases glVertexAttrib3svNV */
   /* glVertexAttrib3fvARB aliases glVertexAttrib3fvNV */
   /* glVertexAttrib3dvARB aliases glVertexAttrib3dvNV */
   /* glVertexAttrib4svARB aliases glVertexAttrib4svNV */
   /* glVertexAttrib4fvARB aliases glVertexAttrib4fvNV */
   /* glVertexAttrib4dvARB aliases glVertexAttrib4dvNV */
   /* glVertexAttrib4NubvARB aliases glVertexAttrib4NubvNV */
   /* glVertexAttrib4bvARB handled in api_loopback.c */
   /* glVertexAttrib4ivARB handled in api_loopback.c */
   /* glVertexAttrib4ubvARB handled in api_loopback.c */
   /* glVertexAttrib4usvARB handled in api_loopback.c */
   /* glVertexAttrib4uivARB handled in api_loopback.c */
   /* glVertexAttrib4NbvARB handled in api_loopback.c */
   /* glVertexAttrib4NsvARB handled in api_loopback.c */
   /* glVertexAttrib4NivARB handled in api_loopback.c */
   /* glVertexAttrib4NusvARB handled in api_loopback.c */
   /* glVertexAttrib4NuivARB handled in api_loopback.c */
   exec->VertexAttribPointerARB = _mesa_VertexAttribPointerARB;
   exec->EnableVertexAttribArrayARB = _mesa_EnableVertexAttribArrayARB;
   exec->DisableVertexAttribArrayARB = _mesa_DisableVertexAttribArrayARB;
   exec->ProgramStringARB = _mesa_ProgramStringARB;
   /* glBindProgramARB aliases glBindProgramNV */
   /* glDeleteProgramsARB aliases glDeleteProgramsNV */
   /* glGenProgramsARB aliases glGenProgramsNV */
   /* glIsProgramARB aliases glIsProgramNV */
   /* glGetVertexAttribdvARB aliases glGetVertexAttribdvNV */
   /* glGetVertexAttribfvARB aliases glGetVertexAttribfvNV */
   /* glGetVertexAttribivARB aliases glGetVertexAttribivNV */
   /* glGetVertexAttribPointervARB aliases glGetVertexAttribPointervNV */
   exec->ProgramEnvParameter4dARB = _mesa_ProgramEnvParameter4dARB;
   exec->ProgramEnvParameter4dvARB = _mesa_ProgramEnvParameter4dvARB;
   exec->ProgramEnvParameter4fARB = _mesa_ProgramEnvParameter4fARB;
   exec->ProgramEnvParameter4fvARB = _mesa_ProgramEnvParameter4fvARB;
   exec->ProgramLocalParameter4dARB = _mesa_ProgramLocalParameter4dARB;
   exec->ProgramLocalParameter4dvARB = _mesa_ProgramLocalParameter4dvARB;
   exec->ProgramLocalParameter4fARB = _mesa_ProgramLocalParameter4fARB;
   exec->ProgramLocalParameter4fvARB = _mesa_ProgramLocalParameter4fvARB;
   exec->GetProgramEnvParameterdvARB = _mesa_GetProgramEnvParameterdvARB;
   exec->GetProgramEnvParameterfvARB = _mesa_GetProgramEnvParameterfvARB;
   exec->GetProgramLocalParameterdvARB = _mesa_GetProgramLocalParameterdvARB;
   exec->GetProgramLocalParameterfvARB = _mesa_GetProgramLocalParameterfvARB;
   exec->GetProgramivARB = _mesa_GetProgramivARB;
   exec->GetProgramStringARB = _mesa_GetProgramStringARB;
#endif

   /* ARB 28. GL_ARB_vertex_buffer_object */
#if FEATURE_ARB_vertex_buffer_object
   exec->BindBufferARB = _mesa_BindBufferARB;
   exec->BufferDataARB = _mesa_BufferDataARB;
   exec->BufferSubDataARB = _mesa_BufferSubDataARB;
   exec->DeleteBuffersARB = _mesa_DeleteBuffersARB;
   exec->GenBuffersARB = _mesa_GenBuffersARB;
   exec->GetBufferParameterivARB = _mesa_GetBufferParameterivARB;
   exec->GetBufferPointervARB = _mesa_GetBufferPointervARB;
   exec->GetBufferSubDataARB = _mesa_GetBufferSubDataARB;
   exec->IsBufferARB = _mesa_IsBufferARB;
   exec->MapBufferARB = _mesa_MapBufferARB;
   exec->UnmapBufferARB = _mesa_UnmapBufferARB;
#endif

#if FEATURE_ARB_occlusion_query
   exec->GenQueriesARB = _mesa_GenQueriesARB;
   exec->DeleteQueriesARB = _mesa_DeleteQueriesARB;
   exec->IsQueryARB = _mesa_IsQueryARB;
   exec->BeginQueryARB = _mesa_BeginQueryARB;
   exec->EndQueryARB = _mesa_EndQueryARB;
   exec->GetQueryivARB = _mesa_GetQueryivARB;
   exec->GetQueryObjectivARB = _mesa_GetQueryObjectivARB;
   exec->GetQueryObjectuivARB = _mesa_GetQueryObjectuivARB;
#endif
}

/*@}*/


/**********************************************************************/
/** \name State update logic */
/*@{*/


/*
 * Update items which depend on vertex/fragment programs.
 */
static void
update_program( GLcontext *ctx )
{
   if (ctx->FragmentProgram.Enabled && ctx->FragmentProgram.Current) {
      if (ctx->FragmentProgram.Current->InputsRead & (1 << FRAG_ATTRIB_COL1))
         ctx->_TriangleCaps |= DD_SEPARATE_SPECULAR;
   }
}


/**
 * Update state dependent on vertex arrays.
 */
static void
update_arrays( GLcontext *ctx )
{
   GLuint i, min;

   /* find min of _MaxElement values for all enabled arrays */

   /* 0 */
   if (ctx->VertexProgram.Enabled
       && ctx->Array.VertexAttrib[VERT_ATTRIB_POS].Enabled) {
      min = ctx->Array.VertexAttrib[VERT_ATTRIB_POS]._MaxElement;
   }
   else if (ctx->Array.Vertex.Enabled) {
      min = ctx->Array.Vertex._MaxElement;
   }
   else {
      /* can't draw anything without vertex positions! */
      min = 0;
   }

   /* 1 */
   if (ctx->VertexProgram.Enabled
       && ctx->Array.VertexAttrib[VERT_ATTRIB_WEIGHT].Enabled) {
      min = MIN2(min, ctx->Array.VertexAttrib[VERT_ATTRIB_WEIGHT]._MaxElement);
   }
   /* no conventional vertex weight array */

   /* 2 */
   if (ctx->VertexProgram.Enabled
       && ctx->Array.VertexAttrib[VERT_ATTRIB_NORMAL].Enabled) {
      min = MIN2(min, ctx->Array.VertexAttrib[VERT_ATTRIB_NORMAL]._MaxElement);
   }
   else if (ctx->Array.Normal.Enabled) {
      min = MIN2(min, ctx->Array.Normal._MaxElement);
   }

   /* 3 */
   if (ctx->VertexProgram.Enabled
       && ctx->Array.VertexAttrib[VERT_ATTRIB_COLOR0].Enabled) {
      min = MIN2(min, ctx->Array.VertexAttrib[VERT_ATTRIB_COLOR0]._MaxElement);
   }
   else if (ctx->Array.Color.Enabled) {
      min = MIN2(min, ctx->Array.Color._MaxElement);
   }

   /* 4 */
   if (ctx->VertexProgram.Enabled
       && ctx->Array.VertexAttrib[VERT_ATTRIB_COLOR1].Enabled) {
      min = MIN2(min, ctx->Array.VertexAttrib[VERT_ATTRIB_COLOR1]._MaxElement);
   }
   else if (ctx->Array.SecondaryColor.Enabled) {
      min = MIN2(min, ctx->Array.SecondaryColor._MaxElement);
   }

   /* 5 */
   if (ctx->VertexProgram.Enabled
       && ctx->Array.VertexAttrib[VERT_ATTRIB_FOG].Enabled) {
      min = MIN2(min, ctx->Array.VertexAttrib[VERT_ATTRIB_FOG]._MaxElement);
   }
   else if (ctx->Array.FogCoord.Enabled) {
      min = MIN2(min, ctx->Array.FogCoord._MaxElement);
   }

   /* 6 */
   if (ctx->VertexProgram.Enabled
       && ctx->Array.VertexAttrib[VERT_ATTRIB_SIX].Enabled) {
      min = MIN2(min, ctx->Array.VertexAttrib[VERT_ATTRIB_SIX]._MaxElement);
   }

   /* 7 */
   if (ctx->VertexProgram.Enabled
       && ctx->Array.VertexAttrib[VERT_ATTRIB_SEVEN].Enabled) {
      min = MIN2(min, ctx->Array.VertexAttrib[VERT_ATTRIB_SEVEN]._MaxElement);
   }

   /* 8..15 */
   for (i = VERT_ATTRIB_TEX0; i < VERT_ATTRIB_MAX; i++) {
      if (ctx->VertexProgram.Enabled
          && ctx->Array.VertexAttrib[i].Enabled) {
         min = MIN2(min, ctx->Array.VertexAttrib[i]._MaxElement);
      }
      else if (i - VERT_ATTRIB_TEX0 < ctx->Const.MaxTextureCoordUnits
               && ctx->Array.TexCoord[i - VERT_ATTRIB_TEX0].Enabled) {
         min = MIN2(min, ctx->Array.TexCoord[i - VERT_ATTRIB_TEX0]._MaxElement);
      }
   }

   if (ctx->Array.Index.Enabled) {
      min = MIN2(min, ctx->Array.Index._MaxElement);
   }

   if (ctx->Array.EdgeFlag.Enabled) {
      min = MIN2(min, ctx->Array.EdgeFlag._MaxElement);
   }

   /* _MaxElement is one past the last legal array element */
   ctx->Array._MaxElement = min;
}



/*
 * If __GLcontextRec::NewState is non-zero then this function \b must be called
 * before rendering any primitive.  Basically, function pointers and
 * miscellaneous flags are updated to reflect the current state of the state
 * machine.
 *
 * Calls dd_function_table::UpdateState to perform any internal state management
 * necessary.
 * 
 * \sa _mesa_update_modelview_project(), _mesa_update_texture(),
 * _mesa_update_buffers(), _mesa_update_polygon(), _mesa_update_lighting() and
 * _mesa_update_tnl_spaces().
 */
void _mesa_update_state( GLcontext *ctx )
{
   const GLuint new_state = ctx->NewState;

   if (MESA_VERBOSE & VERBOSE_STATE)
      _mesa_print_state("_mesa_update_state", new_state);

   if (new_state & (_NEW_MODELVIEW|_NEW_PROJECTION))
      _mesa_update_modelview_project( ctx, new_state );

   if (new_state & (_NEW_PROGRAM|_NEW_TEXTURE|_NEW_TEXTURE_MATRIX))
      _mesa_update_texture( ctx, new_state );

   if (new_state & (_NEW_SCISSOR|_NEW_BUFFERS))
      _mesa_update_buffers( ctx );

   if (new_state & _NEW_POLYGON)
      _mesa_update_polygon( ctx );

   if (new_state & _NEW_LIGHT)
      _mesa_update_lighting( ctx );

   if (new_state & _IMAGE_NEW_TRANSFER_STATE)
      _mesa_update_pixel( ctx, new_state );

   if (new_state & _NEW_PROGRAM)
      update_program( ctx );

   if (new_state & (_NEW_ARRAY | _NEW_PROGRAM))
      update_arrays( ctx );

   /* ctx->_NeedEyeCoords is now up to date.
    *
    * If the truth value of this variable has changed, update for the
    * new lighting space and recompute the positions of lights and the
    * normal transform.
    *
    * If the lighting space hasn't changed, may still need to recompute
    * light positions & normal transforms for other reasons.
    */
   if (new_state & _MESA_NEW_NEED_EYE_COORDS) 
      _mesa_update_tnl_spaces( ctx, new_state );

   /*
    * Here the driver sets up all the ctx->Driver function pointers
    * to it's specific, private functions, and performs any
    * internal state management necessary, including invalidating
    * state of active modules.
    *
    * Set ctx->NewState to zero to avoid recursion if
    * Driver.UpdateState() has to call FLUSH_VERTICES().  (fixed?)
    */
   ctx->NewState = 0;
   ctx->Driver.UpdateState(ctx, new_state);
   ctx->Array.NewState = 0;

   /* At this point we can do some assertions to be sure the required
    * device driver function pointers are all initialized.
    */
   _mesa_check_driver_hooks( ctx );
}

/*@}*/
