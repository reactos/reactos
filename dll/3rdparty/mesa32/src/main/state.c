/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
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
 */


/**
 * \file state.c
 * State management.
 * 
 * This file manages recalculation of derived values in the __GLcontextRec.
 * Also, this is where we initialize the API dispatch table.
 */

#include "glheader.h"
#include "accum.h"
#include "api_loopback.h"
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
#include "shader/arbprogram.h"
#endif
#if FEATURE_ATI_fragment_shader
#include "shader/atifragshader.h"
#endif
#include "attrib.h"
#include "blend.h"
#if FEATURE_ARB_vertex_buffer_object
#include "bufferobj.h"
#endif
#include "arrayobj.h"
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
#if FEATURE_EXT_framebuffer_object
#include "fbobject.h"
#endif
#include "framebuffer.h"
#include "hint.h"
#include "histogram.h"
#include "imports.h"
#include "light.h"
#include "lines.h"
#include "macros.h"
#include "matrix.h"
#include "pixel.h"
#include "points.h"
#include "polygon.h"
#if FEATURE_ARB_occlusion_query || FEATURE_EXT_timer_query
#include "queryobj.h"
#endif
#include "rastpos.h"
#include "state.h"
#include "stencil.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "mtypes.h"
#include "varray.h"
#if FEATURE_NV_vertex_program
#include "shader/nvprogram.h"
#endif
#if FEATURE_NV_fragment_program
#include "shader/nvprogram.h"
#include "shader/program.h"
#include "texenvprogram.h"
#endif
#if FEATURE_ARB_shader_objects
#include "shaders.h"
#endif
#include "debug.h"
#include "glapi/dispatch.h"



/**
 * Initialize a dispatch table with pointers to Mesa's immediate-mode
 * commands.
 *
 * Pointers to glBegin()/glEnd() object commands and a few others
 * are provided via the GLvertexformat interface.
 *
 * \param ctx  GL context to which \c exec belongs.
 * \param exec dispatch table.
 */
void
_mesa_init_exec_table(struct _glapi_table *exec)
{
#if _HAVE_FULL_GL
   _mesa_loopback_init_api_table( exec );
#endif

   /* load the dispatch slots we understand */
   SET_AlphaFunc(exec, _mesa_AlphaFunc);
   SET_BlendFunc(exec, _mesa_BlendFunc);
   SET_Clear(exec, _mesa_Clear);
   SET_ClearColor(exec, _mesa_ClearColor);
   SET_ClearStencil(exec, _mesa_ClearStencil);
   SET_ColorMask(exec, _mesa_ColorMask);
   SET_CullFace(exec, _mesa_CullFace);
   SET_Disable(exec, _mesa_Disable);
   SET_DrawBuffer(exec, _mesa_DrawBuffer);
   SET_Enable(exec, _mesa_Enable);
   SET_Finish(exec, _mesa_Finish);
   SET_Flush(exec, _mesa_Flush);
   SET_FrontFace(exec, _mesa_FrontFace);
   SET_Frustum(exec, _mesa_Frustum);
   SET_GetError(exec, _mesa_GetError);
   SET_GetFloatv(exec, _mesa_GetFloatv);
   SET_GetString(exec, _mesa_GetString);
   SET_InitNames(exec, _mesa_InitNames);
   SET_LineStipple(exec, _mesa_LineStipple);
   SET_LineWidth(exec, _mesa_LineWidth);
   SET_LoadIdentity(exec, _mesa_LoadIdentity);
   SET_LoadMatrixf(exec, _mesa_LoadMatrixf);
   SET_LoadName(exec, _mesa_LoadName);
   SET_LogicOp(exec, _mesa_LogicOp);
   SET_MatrixMode(exec, _mesa_MatrixMode);
   SET_MultMatrixf(exec, _mesa_MultMatrixf);
   SET_Ortho(exec, _mesa_Ortho);
   SET_PixelStorei(exec, _mesa_PixelStorei);
   SET_PopMatrix(exec, _mesa_PopMatrix);
   SET_PopName(exec, _mesa_PopName);
   SET_PushMatrix(exec, _mesa_PushMatrix);
   SET_PushName(exec, _mesa_PushName);
   SET_RasterPos2f(exec, _mesa_RasterPos2f);
   SET_RasterPos2fv(exec, _mesa_RasterPos2fv);
   SET_RasterPos2i(exec, _mesa_RasterPos2i);
   SET_RasterPos2iv(exec, _mesa_RasterPos2iv);
   SET_ReadBuffer(exec, _mesa_ReadBuffer);
   SET_RenderMode(exec, _mesa_RenderMode);
   SET_Rotatef(exec, _mesa_Rotatef);
   SET_Scalef(exec, _mesa_Scalef);
   SET_Scissor(exec, _mesa_Scissor);
   SET_SelectBuffer(exec, _mesa_SelectBuffer);
   SET_ShadeModel(exec, _mesa_ShadeModel);
   SET_StencilFunc(exec, _mesa_StencilFunc);
   SET_StencilMask(exec, _mesa_StencilMask);
   SET_StencilOp(exec, _mesa_StencilOp);
   SET_TexEnvfv(exec, _mesa_TexEnvfv);
   SET_TexEnvi(exec, _mesa_TexEnvi);
   SET_TexImage2D(exec, _mesa_TexImage2D);
   SET_TexParameteri(exec, _mesa_TexParameteri);
   SET_Translatef(exec, _mesa_Translatef);
   SET_Viewport(exec, _mesa_Viewport);
#if _HAVE_FULL_GL
   SET_Accum(exec, _mesa_Accum);
   SET_Bitmap(exec, _mesa_Bitmap);
   SET_CallList(exec, _mesa_CallList);
   SET_CallLists(exec, _mesa_CallLists);
   SET_ClearAccum(exec, _mesa_ClearAccum);
   SET_ClearDepth(exec, _mesa_ClearDepth);
   SET_ClearIndex(exec, _mesa_ClearIndex);
   SET_ClipPlane(exec, _mesa_ClipPlane);
   SET_ColorMaterial(exec, _mesa_ColorMaterial);
   SET_CopyPixels(exec, _mesa_CopyPixels);
   SET_CullParameterfvEXT(exec, _mesa_CullParameterfvEXT);
   SET_CullParameterdvEXT(exec, _mesa_CullParameterdvEXT);
   SET_DeleteLists(exec, _mesa_DeleteLists);
   SET_DepthFunc(exec, _mesa_DepthFunc);
   SET_DepthMask(exec, _mesa_DepthMask);
   SET_DepthRange(exec, _mesa_DepthRange);
   SET_DrawPixels(exec, _mesa_DrawPixels);
   SET_EndList(exec, _mesa_EndList);
   SET_FeedbackBuffer(exec, _mesa_FeedbackBuffer);
   SET_FogCoordPointerEXT(exec, _mesa_FogCoordPointerEXT);
   SET_Fogf(exec, _mesa_Fogf);
   SET_Fogfv(exec, _mesa_Fogfv);
   SET_Fogi(exec, _mesa_Fogi);
   SET_Fogiv(exec, _mesa_Fogiv);
   SET_GenLists(exec, _mesa_GenLists);
   SET_GetClipPlane(exec, _mesa_GetClipPlane);
   SET_GetBooleanv(exec, _mesa_GetBooleanv);
   SET_GetDoublev(exec, _mesa_GetDoublev);
   SET_GetIntegerv(exec, _mesa_GetIntegerv);
   SET_GetLightfv(exec, _mesa_GetLightfv);
   SET_GetLightiv(exec, _mesa_GetLightiv);
   SET_GetMapdv(exec, _mesa_GetMapdv);
   SET_GetMapfv(exec, _mesa_GetMapfv);
   SET_GetMapiv(exec, _mesa_GetMapiv);
   SET_GetMaterialfv(exec, _mesa_GetMaterialfv);
   SET_GetMaterialiv(exec, _mesa_GetMaterialiv);
   SET_GetPixelMapfv(exec, _mesa_GetPixelMapfv);
   SET_GetPixelMapuiv(exec, _mesa_GetPixelMapuiv);
   SET_GetPixelMapusv(exec, _mesa_GetPixelMapusv);
   SET_GetPolygonStipple(exec, _mesa_GetPolygonStipple);
   SET_GetTexEnvfv(exec, _mesa_GetTexEnvfv);
   SET_GetTexEnviv(exec, _mesa_GetTexEnviv);
   SET_GetTexLevelParameterfv(exec, _mesa_GetTexLevelParameterfv);
   SET_GetTexLevelParameteriv(exec, _mesa_GetTexLevelParameteriv);
   SET_GetTexParameterfv(exec, _mesa_GetTexParameterfv);
   SET_GetTexParameteriv(exec, _mesa_GetTexParameteriv);
   SET_GetTexGendv(exec, _mesa_GetTexGendv);
   SET_GetTexGenfv(exec, _mesa_GetTexGenfv);
   SET_GetTexGeniv(exec, _mesa_GetTexGeniv);
   SET_GetTexImage(exec, _mesa_GetTexImage);
   SET_Hint(exec, _mesa_Hint);
   SET_IndexMask(exec, _mesa_IndexMask);
   SET_IsEnabled(exec, _mesa_IsEnabled);
   SET_IsList(exec, _mesa_IsList);
   SET_LightModelf(exec, _mesa_LightModelf);
   SET_LightModelfv(exec, _mesa_LightModelfv);
   SET_LightModeli(exec, _mesa_LightModeli);
   SET_LightModeliv(exec, _mesa_LightModeliv);
   SET_Lightf(exec, _mesa_Lightf);
   SET_Lightfv(exec, _mesa_Lightfv);
   SET_Lighti(exec, _mesa_Lighti);
   SET_Lightiv(exec, _mesa_Lightiv);
   SET_ListBase(exec, _mesa_ListBase);
   SET_LoadMatrixd(exec, _mesa_LoadMatrixd);
   SET_Map1d(exec, _mesa_Map1d);
   SET_Map1f(exec, _mesa_Map1f);
   SET_Map2d(exec, _mesa_Map2d);
   SET_Map2f(exec, _mesa_Map2f);
   SET_MapGrid1d(exec, _mesa_MapGrid1d);
   SET_MapGrid1f(exec, _mesa_MapGrid1f);
   SET_MapGrid2d(exec, _mesa_MapGrid2d);
   SET_MapGrid2f(exec, _mesa_MapGrid2f);
   SET_MultMatrixd(exec, _mesa_MultMatrixd);
   SET_NewList(exec, _mesa_NewList);
   SET_PassThrough(exec, _mesa_PassThrough);
   SET_PixelMapfv(exec, _mesa_PixelMapfv);
   SET_PixelMapuiv(exec, _mesa_PixelMapuiv);
   SET_PixelMapusv(exec, _mesa_PixelMapusv);
   SET_PixelStoref(exec, _mesa_PixelStoref);
   SET_PixelTransferf(exec, _mesa_PixelTransferf);
   SET_PixelTransferi(exec, _mesa_PixelTransferi);
   SET_PixelZoom(exec, _mesa_PixelZoom);
   SET_PointSize(exec, _mesa_PointSize);
   SET_PolygonMode(exec, _mesa_PolygonMode);
   SET_PolygonOffset(exec, _mesa_PolygonOffset);
   SET_PolygonStipple(exec, _mesa_PolygonStipple);
   SET_PopAttrib(exec, _mesa_PopAttrib);
   SET_PushAttrib(exec, _mesa_PushAttrib);
   SET_RasterPos2d(exec, _mesa_RasterPos2d);
   SET_RasterPos2dv(exec, _mesa_RasterPos2dv);
   SET_RasterPos2s(exec, _mesa_RasterPos2s);
   SET_RasterPos2sv(exec, _mesa_RasterPos2sv);
   SET_RasterPos3d(exec, _mesa_RasterPos3d);
   SET_RasterPos3dv(exec, _mesa_RasterPos3dv);
   SET_RasterPos3f(exec, _mesa_RasterPos3f);
   SET_RasterPos3fv(exec, _mesa_RasterPos3fv);
   SET_RasterPos3i(exec, _mesa_RasterPos3i);
   SET_RasterPos3iv(exec, _mesa_RasterPos3iv);
   SET_RasterPos3s(exec, _mesa_RasterPos3s);
   SET_RasterPos3sv(exec, _mesa_RasterPos3sv);
   SET_RasterPos4d(exec, _mesa_RasterPos4d);
   SET_RasterPos4dv(exec, _mesa_RasterPos4dv);
   SET_RasterPos4f(exec, _mesa_RasterPos4f);
   SET_RasterPos4fv(exec, _mesa_RasterPos4fv);
   SET_RasterPos4i(exec, _mesa_RasterPos4i);
   SET_RasterPos4iv(exec, _mesa_RasterPos4iv);
   SET_RasterPos4s(exec, _mesa_RasterPos4s);
   SET_RasterPos4sv(exec, _mesa_RasterPos4sv);
   SET_ReadPixels(exec, _mesa_ReadPixels);
   SET_Rotated(exec, _mesa_Rotated);
   SET_Scaled(exec, _mesa_Scaled);
   SET_SecondaryColorPointerEXT(exec, _mesa_SecondaryColorPointerEXT);
   SET_TexEnvf(exec, _mesa_TexEnvf);
   SET_TexEnviv(exec, _mesa_TexEnviv);
   SET_TexGend(exec, _mesa_TexGend);
   SET_TexGendv(exec, _mesa_TexGendv);
   SET_TexGenf(exec, _mesa_TexGenf);
   SET_TexGenfv(exec, _mesa_TexGenfv);
   SET_TexGeni(exec, _mesa_TexGeni);
   SET_TexGeniv(exec, _mesa_TexGeniv);
   SET_TexImage1D(exec, _mesa_TexImage1D);
   SET_TexParameterf(exec, _mesa_TexParameterf);
   SET_TexParameterfv(exec, _mesa_TexParameterfv);
   SET_TexParameteriv(exec, _mesa_TexParameteriv);
   SET_Translated(exec, _mesa_Translated);
#endif

   /* 1.1 */
   SET_BindTexture(exec, _mesa_BindTexture);
   SET_DeleteTextures(exec, _mesa_DeleteTextures);
   SET_GenTextures(exec, _mesa_GenTextures);
#if _HAVE_FULL_GL
   SET_AreTexturesResident(exec, _mesa_AreTexturesResident);
   SET_ColorPointer(exec, _mesa_ColorPointer);
   SET_CopyTexImage1D(exec, _mesa_CopyTexImage1D);
   SET_CopyTexImage2D(exec, _mesa_CopyTexImage2D);
   SET_CopyTexSubImage1D(exec, _mesa_CopyTexSubImage1D);
   SET_CopyTexSubImage2D(exec, _mesa_CopyTexSubImage2D);
   SET_DisableClientState(exec, _mesa_DisableClientState);
   SET_EdgeFlagPointer(exec, _mesa_EdgeFlagPointer);
   SET_EnableClientState(exec, _mesa_EnableClientState);
   SET_GetPointerv(exec, _mesa_GetPointerv);
   SET_IndexPointer(exec, _mesa_IndexPointer);
   SET_InterleavedArrays(exec, _mesa_InterleavedArrays);
   SET_IsTexture(exec, _mesa_IsTexture);
   SET_NormalPointer(exec, _mesa_NormalPointer);
   SET_PopClientAttrib(exec, _mesa_PopClientAttrib);
   SET_PrioritizeTextures(exec, _mesa_PrioritizeTextures);
   SET_PushClientAttrib(exec, _mesa_PushClientAttrib);
   SET_TexCoordPointer(exec, _mesa_TexCoordPointer);
   SET_TexSubImage1D(exec, _mesa_TexSubImage1D);
   SET_TexSubImage2D(exec, _mesa_TexSubImage2D);
   SET_VertexPointer(exec, _mesa_VertexPointer);
#endif

   /* 1.2 */
#if _HAVE_FULL_GL
   SET_CopyTexSubImage3D(exec, _mesa_CopyTexSubImage3D);
   SET_TexImage3D(exec, _mesa_TexImage3D);
   SET_TexSubImage3D(exec, _mesa_TexSubImage3D);
#endif

   /* OpenGL 1.2  GL_ARB_imaging */
#if _HAVE_FULL_GL
   SET_BlendColor(exec, _mesa_BlendColor);
   SET_BlendEquation(exec, _mesa_BlendEquation);
   SET_BlendEquationSeparateEXT(exec, _mesa_BlendEquationSeparateEXT);
   SET_ColorSubTable(exec, _mesa_ColorSubTable);
   SET_ColorTable(exec, _mesa_ColorTable);
   SET_ColorTableParameterfv(exec, _mesa_ColorTableParameterfv);
   SET_ColorTableParameteriv(exec, _mesa_ColorTableParameteriv);
   SET_ConvolutionFilter1D(exec, _mesa_ConvolutionFilter1D);
   SET_ConvolutionFilter2D(exec, _mesa_ConvolutionFilter2D);
   SET_ConvolutionParameterf(exec, _mesa_ConvolutionParameterf);
   SET_ConvolutionParameterfv(exec, _mesa_ConvolutionParameterfv);
   SET_ConvolutionParameteri(exec, _mesa_ConvolutionParameteri);
   SET_ConvolutionParameteriv(exec, _mesa_ConvolutionParameteriv);
   SET_CopyColorSubTable(exec, _mesa_CopyColorSubTable);
   SET_CopyColorTable(exec, _mesa_CopyColorTable);
   SET_CopyConvolutionFilter1D(exec, _mesa_CopyConvolutionFilter1D);
   SET_CopyConvolutionFilter2D(exec, _mesa_CopyConvolutionFilter2D);
   SET_GetColorTable(exec, _mesa_GetColorTable);
   SET_GetColorTableParameterfv(exec, _mesa_GetColorTableParameterfv);
   SET_GetColorTableParameteriv(exec, _mesa_GetColorTableParameteriv);
   SET_GetConvolutionFilter(exec, _mesa_GetConvolutionFilter);
   SET_GetConvolutionParameterfv(exec, _mesa_GetConvolutionParameterfv);
   SET_GetConvolutionParameteriv(exec, _mesa_GetConvolutionParameteriv);
   SET_GetHistogram(exec, _mesa_GetHistogram);
   SET_GetHistogramParameterfv(exec, _mesa_GetHistogramParameterfv);
   SET_GetHistogramParameteriv(exec, _mesa_GetHistogramParameteriv);
   SET_GetMinmax(exec, _mesa_GetMinmax);
   SET_GetMinmaxParameterfv(exec, _mesa_GetMinmaxParameterfv);
   SET_GetMinmaxParameteriv(exec, _mesa_GetMinmaxParameteriv);
   SET_GetSeparableFilter(exec, _mesa_GetSeparableFilter);
   SET_Histogram(exec, _mesa_Histogram);
   SET_Minmax(exec, _mesa_Minmax);
   SET_ResetHistogram(exec, _mesa_ResetHistogram);
   SET_ResetMinmax(exec, _mesa_ResetMinmax);
   SET_SeparableFilter2D(exec, _mesa_SeparableFilter2D);
#endif

   /* OpenGL 2.0 */
   SET_StencilFuncSeparate(exec, _mesa_StencilFuncSeparate);
   SET_StencilMaskSeparate(exec, _mesa_StencilMaskSeparate);
   SET_StencilOpSeparate(exec, _mesa_StencilOpSeparate);
#if FEATURE_ARB_shader_objects
   SET_AttachShader(exec, _mesa_AttachShader);
   SET_CreateProgram(exec, _mesa_CreateProgram);
   SET_CreateShader(exec, _mesa_CreateShader);
   SET_DeleteProgram(exec, _mesa_DeleteProgram);
   SET_DeleteShader(exec, _mesa_DeleteShader);
   SET_DetachShader(exec, _mesa_DetachShader);
   SET_GetAttachedShaders(exec, _mesa_GetAttachedShaders);
   SET_GetProgramiv(exec, _mesa_GetProgramiv);
   SET_GetProgramInfoLog(exec, _mesa_GetProgramInfoLog);
   SET_GetShaderiv(exec, _mesa_GetShaderiv);
   SET_GetShaderInfoLog(exec, _mesa_GetShaderInfoLog);
   SET_IsProgram(exec, _mesa_IsProgram);
   SET_IsShader(exec, _mesa_IsShader);
#endif

   /* OpenGL 2.1 */
#if FEATURE_ARB_shader_objects
   SET_UniformMatrix2x3fv(exec, _mesa_UniformMatrix2x3fv);
   SET_UniformMatrix3x2fv(exec, _mesa_UniformMatrix3x2fv);
   SET_UniformMatrix2x4fv(exec, _mesa_UniformMatrix2x4fv);
   SET_UniformMatrix4x2fv(exec, _mesa_UniformMatrix4x2fv);
   SET_UniformMatrix3x4fv(exec, _mesa_UniformMatrix3x4fv);
   SET_UniformMatrix4x3fv(exec, _mesa_UniformMatrix4x3fv);
#endif


   /* 2. GL_EXT_blend_color */
#if 0
/*    SET_BlendColorEXT(exec, _mesa_BlendColorEXT); */
#endif

   /* 3. GL_EXT_polygon_offset */
#if _HAVE_FULL_GL
   SET_PolygonOffsetEXT(exec, _mesa_PolygonOffsetEXT);
#endif

   /* 6. GL_EXT_texture3d */
#if 0
/*    SET_CopyTexSubImage3DEXT(exec, _mesa_CopyTexSubImage3D); */
/*    SET_TexImage3DEXT(exec, _mesa_TexImage3DEXT); */
/*    SET_TexSubImage3DEXT(exec, _mesa_TexSubImage3D); */
#endif

   /* 11. GL_EXT_histogram */
#if 0
   SET_GetHistogramEXT(exec, _mesa_GetHistogram);
   SET_GetHistogramParameterfvEXT(exec, _mesa_GetHistogramParameterfv);
   SET_GetHistogramParameterivEXT(exec, _mesa_GetHistogramParameteriv);
   SET_GetMinmaxEXT(exec, _mesa_GetMinmax);
   SET_GetMinmaxParameterfvEXT(exec, _mesa_GetMinmaxParameterfv);
   SET_GetMinmaxParameterivEXT(exec, _mesa_GetMinmaxParameteriv);
#endif

   /* 14. SGI_color_table */
#if 0
   SET_ColorTableSGI(exec, _mesa_ColorTable);
   SET_ColorSubTableSGI(exec, _mesa_ColorSubTable);
   SET_GetColorTableSGI(exec, _mesa_GetColorTable);
   SET_GetColorTableParameterfvSGI(exec, _mesa_GetColorTableParameterfv);
   SET_GetColorTableParameterivSGI(exec, _mesa_GetColorTableParameteriv);
#endif

   /* 30. GL_EXT_vertex_array */
#if _HAVE_FULL_GL
   SET_ColorPointerEXT(exec, _mesa_ColorPointerEXT);
   SET_EdgeFlagPointerEXT(exec, _mesa_EdgeFlagPointerEXT);
   SET_IndexPointerEXT(exec, _mesa_IndexPointerEXT);
   SET_NormalPointerEXT(exec, _mesa_NormalPointerEXT);
   SET_TexCoordPointerEXT(exec, _mesa_TexCoordPointerEXT);
   SET_VertexPointerEXT(exec, _mesa_VertexPointerEXT);
#endif

   /* 37. GL_EXT_blend_minmax */
#if 0
   SET_BlendEquationEXT(exec, _mesa_BlendEquationEXT);
#endif

   /* 54. GL_EXT_point_parameters */
#if _HAVE_FULL_GL
   SET_PointParameterfEXT(exec, _mesa_PointParameterfEXT);
   SET_PointParameterfvEXT(exec, _mesa_PointParameterfvEXT);
#endif

   /* 97. GL_EXT_compiled_vertex_array */
#if _HAVE_FULL_GL
   SET_LockArraysEXT(exec, _mesa_LockArraysEXT);
   SET_UnlockArraysEXT(exec, _mesa_UnlockArraysEXT);
#endif

   /* 148. GL_EXT_multi_draw_arrays */
#if _HAVE_FULL_GL
   SET_MultiDrawArraysEXT(exec, _mesa_MultiDrawArraysEXT);
   SET_MultiDrawElementsEXT(exec, _mesa_MultiDrawElementsEXT);
#endif

   /* 173. GL_INGR_blend_func_separate */
#if _HAVE_FULL_GL
   SET_BlendFuncSeparateEXT(exec, _mesa_BlendFuncSeparateEXT);
#endif

   /* 196. GL_MESA_resize_buffers */
#if _HAVE_FULL_GL
   SET_ResizeBuffersMESA(exec, _mesa_ResizeBuffersMESA);
#endif

   /* 197. GL_MESA_window_pos */
#if _HAVE_FULL_GL
   SET_WindowPos2dMESA(exec, _mesa_WindowPos2dMESA);
   SET_WindowPos2dvMESA(exec, _mesa_WindowPos2dvMESA);
   SET_WindowPos2fMESA(exec, _mesa_WindowPos2fMESA);
   SET_WindowPos2fvMESA(exec, _mesa_WindowPos2fvMESA);
   SET_WindowPos2iMESA(exec, _mesa_WindowPos2iMESA);
   SET_WindowPos2ivMESA(exec, _mesa_WindowPos2ivMESA);
   SET_WindowPos2sMESA(exec, _mesa_WindowPos2sMESA);
   SET_WindowPos2svMESA(exec, _mesa_WindowPos2svMESA);
   SET_WindowPos3dMESA(exec, _mesa_WindowPos3dMESA);
   SET_WindowPos3dvMESA(exec, _mesa_WindowPos3dvMESA);
   SET_WindowPos3fMESA(exec, _mesa_WindowPos3fMESA);
   SET_WindowPos3fvMESA(exec, _mesa_WindowPos3fvMESA);
   SET_WindowPos3iMESA(exec, _mesa_WindowPos3iMESA);
   SET_WindowPos3ivMESA(exec, _mesa_WindowPos3ivMESA);
   SET_WindowPos3sMESA(exec, _mesa_WindowPos3sMESA);
   SET_WindowPos3svMESA(exec, _mesa_WindowPos3svMESA);
   SET_WindowPos4dMESA(exec, _mesa_WindowPos4dMESA);
   SET_WindowPos4dvMESA(exec, _mesa_WindowPos4dvMESA);
   SET_WindowPos4fMESA(exec, _mesa_WindowPos4fMESA);
   SET_WindowPos4fvMESA(exec, _mesa_WindowPos4fvMESA);
   SET_WindowPos4iMESA(exec, _mesa_WindowPos4iMESA);
   SET_WindowPos4ivMESA(exec, _mesa_WindowPos4ivMESA);
   SET_WindowPos4sMESA(exec, _mesa_WindowPos4sMESA);
   SET_WindowPos4svMESA(exec, _mesa_WindowPos4svMESA);
#endif

   /* 200. GL_IBM_multimode_draw_arrays */
#if _HAVE_FULL_GL
   SET_MultiModeDrawArraysIBM(exec, _mesa_MultiModeDrawArraysIBM);
   SET_MultiModeDrawElementsIBM(exec, _mesa_MultiModeDrawElementsIBM);
#endif

   /* 233. GL_NV_vertex_program */
#if FEATURE_NV_vertex_program
   SET_BindProgramNV(exec, _mesa_BindProgram);
   SET_DeleteProgramsNV(exec, _mesa_DeletePrograms);
   SET_ExecuteProgramNV(exec, _mesa_ExecuteProgramNV);
   SET_GenProgramsNV(exec, _mesa_GenPrograms);
   SET_AreProgramsResidentNV(exec, _mesa_AreProgramsResidentNV);
   SET_RequestResidentProgramsNV(exec, _mesa_RequestResidentProgramsNV);
   SET_GetProgramParameterfvNV(exec, _mesa_GetProgramParameterfvNV);
   SET_GetProgramParameterdvNV(exec, _mesa_GetProgramParameterdvNV);
   SET_GetProgramivNV(exec, _mesa_GetProgramivNV);
   SET_GetProgramStringNV(exec, _mesa_GetProgramStringNV);
   SET_GetTrackMatrixivNV(exec, _mesa_GetTrackMatrixivNV);
   SET_GetVertexAttribdvNV(exec, _mesa_GetVertexAttribdvNV);
   SET_GetVertexAttribfvNV(exec, _mesa_GetVertexAttribfvNV);
   SET_GetVertexAttribivNV(exec, _mesa_GetVertexAttribivNV);
   SET_GetVertexAttribPointervNV(exec, _mesa_GetVertexAttribPointervNV);
   SET_IsProgramNV(exec, _mesa_IsProgramARB);
   SET_LoadProgramNV(exec, _mesa_LoadProgramNV);
   SET_ProgramEnvParameter4dARB(exec, _mesa_ProgramEnvParameter4dARB); /* alias to ProgramParameter4dNV */
   SET_ProgramEnvParameter4dvARB(exec, _mesa_ProgramEnvParameter4dvARB);  /* alias to ProgramParameter4dvNV */
   SET_ProgramEnvParameter4fARB(exec, _mesa_ProgramEnvParameter4fARB);  /* alias to ProgramParameter4fNV */
   SET_ProgramEnvParameter4fvARB(exec, _mesa_ProgramEnvParameter4fvARB);  /* alias to ProgramParameter4fvNV */
   SET_ProgramParameters4dvNV(exec, _mesa_ProgramParameters4dvNV);
   SET_ProgramParameters4fvNV(exec, _mesa_ProgramParameters4fvNV);
   SET_TrackMatrixNV(exec, _mesa_TrackMatrixNV);
   SET_VertexAttribPointerNV(exec, _mesa_VertexAttribPointerNV);
   /* glVertexAttrib*NV functions handled in api_loopback.c */
#endif

   /* 273. GL_APPLE_vertex_array_object */
   SET_BindVertexArrayAPPLE(exec, _mesa_BindVertexArrayAPPLE);
   SET_DeleteVertexArraysAPPLE(exec, _mesa_DeleteVertexArraysAPPLE);
   SET_GenVertexArraysAPPLE(exec, _mesa_GenVertexArraysAPPLE);
   SET_IsVertexArrayAPPLE(exec, _mesa_IsVertexArrayAPPLE);

   /* 282. GL_NV_fragment_program */
#if FEATURE_NV_fragment_program
   SET_ProgramNamedParameter4fNV(exec, _mesa_ProgramNamedParameter4fNV);
   SET_ProgramNamedParameter4dNV(exec, _mesa_ProgramNamedParameter4dNV);
   SET_ProgramNamedParameter4fvNV(exec, _mesa_ProgramNamedParameter4fvNV);
   SET_ProgramNamedParameter4dvNV(exec, _mesa_ProgramNamedParameter4dvNV);
   SET_GetProgramNamedParameterfvNV(exec, _mesa_GetProgramNamedParameterfvNV);
   SET_GetProgramNamedParameterdvNV(exec, _mesa_GetProgramNamedParameterdvNV);
   SET_ProgramLocalParameter4dARB(exec, _mesa_ProgramLocalParameter4dARB);
   SET_ProgramLocalParameter4dvARB(exec, _mesa_ProgramLocalParameter4dvARB);
   SET_ProgramLocalParameter4fARB(exec, _mesa_ProgramLocalParameter4fARB);
   SET_ProgramLocalParameter4fvARB(exec, _mesa_ProgramLocalParameter4fvARB);
   SET_GetProgramLocalParameterdvARB(exec, _mesa_GetProgramLocalParameterdvARB);
   SET_GetProgramLocalParameterfvARB(exec, _mesa_GetProgramLocalParameterfvARB);
#endif

   /* 262. GL_NV_point_sprite */
#if _HAVE_FULL_GL
   SET_PointParameteriNV(exec, _mesa_PointParameteriNV);
   SET_PointParameterivNV(exec, _mesa_PointParameterivNV);
#endif

   /* 268. GL_EXT_stencil_two_side */
#if _HAVE_FULL_GL
   SET_ActiveStencilFaceEXT(exec, _mesa_ActiveStencilFaceEXT);
#endif

   /* ???. GL_EXT_depth_bounds_test */
   SET_DepthBoundsEXT(exec, _mesa_DepthBoundsEXT);

   /* ARB 1. GL_ARB_multitexture */
#if _HAVE_FULL_GL
   SET_ActiveTextureARB(exec, _mesa_ActiveTextureARB);
   SET_ClientActiveTextureARB(exec, _mesa_ClientActiveTextureARB);
#endif

   /* ARB 3. GL_ARB_transpose_matrix */
#if _HAVE_FULL_GL
   SET_LoadTransposeMatrixdARB(exec, _mesa_LoadTransposeMatrixdARB);
   SET_LoadTransposeMatrixfARB(exec, _mesa_LoadTransposeMatrixfARB);
   SET_MultTransposeMatrixdARB(exec, _mesa_MultTransposeMatrixdARB);
   SET_MultTransposeMatrixfARB(exec, _mesa_MultTransposeMatrixfARB);
#endif

   /* ARB 5. GL_ARB_multisample */
#if _HAVE_FULL_GL
   SET_SampleCoverageARB(exec, _mesa_SampleCoverageARB);
#endif

   /* ARB 12. GL_ARB_texture_compression */
#if _HAVE_FULL_GL
   SET_CompressedTexImage3DARB(exec, _mesa_CompressedTexImage3DARB);
   SET_CompressedTexImage2DARB(exec, _mesa_CompressedTexImage2DARB);
   SET_CompressedTexImage1DARB(exec, _mesa_CompressedTexImage1DARB);
   SET_CompressedTexSubImage3DARB(exec, _mesa_CompressedTexSubImage3DARB);
   SET_CompressedTexSubImage2DARB(exec, _mesa_CompressedTexSubImage2DARB);
   SET_CompressedTexSubImage1DARB(exec, _mesa_CompressedTexSubImage1DARB);
   SET_GetCompressedTexImageARB(exec, _mesa_GetCompressedTexImageARB);
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
   SET_VertexAttribPointerARB(exec, _mesa_VertexAttribPointerARB);
   SET_EnableVertexAttribArrayARB(exec, _mesa_EnableVertexAttribArrayARB);
   SET_DisableVertexAttribArrayARB(exec, _mesa_DisableVertexAttribArrayARB);
   SET_ProgramStringARB(exec, _mesa_ProgramStringARB);
   /* glBindProgramARB aliases glBindProgramNV */
   /* glDeleteProgramsARB aliases glDeleteProgramsNV */
   /* glGenProgramsARB aliases glGenProgramsNV */
   /* glIsProgramARB aliases glIsProgramNV */
   SET_GetVertexAttribdvARB(exec, _mesa_GetVertexAttribdvARB);
   SET_GetVertexAttribfvARB(exec, _mesa_GetVertexAttribfvARB);
   SET_GetVertexAttribivARB(exec, _mesa_GetVertexAttribivARB);
   /* glGetVertexAttribPointervARB aliases glGetVertexAttribPointervNV */
   SET_ProgramEnvParameter4dARB(exec, _mesa_ProgramEnvParameter4dARB);
   SET_ProgramEnvParameter4dvARB(exec, _mesa_ProgramEnvParameter4dvARB);
   SET_ProgramEnvParameter4fARB(exec, _mesa_ProgramEnvParameter4fARB);
   SET_ProgramEnvParameter4fvARB(exec, _mesa_ProgramEnvParameter4fvARB);
   SET_ProgramLocalParameter4dARB(exec, _mesa_ProgramLocalParameter4dARB);
   SET_ProgramLocalParameter4dvARB(exec, _mesa_ProgramLocalParameter4dvARB);
   SET_ProgramLocalParameter4fARB(exec, _mesa_ProgramLocalParameter4fARB);
   SET_ProgramLocalParameter4fvARB(exec, _mesa_ProgramLocalParameter4fvARB);
   SET_GetProgramEnvParameterdvARB(exec, _mesa_GetProgramEnvParameterdvARB);
   SET_GetProgramEnvParameterfvARB(exec, _mesa_GetProgramEnvParameterfvARB);
   SET_GetProgramLocalParameterdvARB(exec, _mesa_GetProgramLocalParameterdvARB);
   SET_GetProgramLocalParameterfvARB(exec, _mesa_GetProgramLocalParameterfvARB);
   SET_GetProgramivARB(exec, _mesa_GetProgramivARB);
   SET_GetProgramStringARB(exec, _mesa_GetProgramStringARB);
#endif

   /* ARB 28. GL_ARB_vertex_buffer_object */
#if FEATURE_ARB_vertex_buffer_object
   SET_BindBufferARB(exec, _mesa_BindBufferARB);
   SET_BufferDataARB(exec, _mesa_BufferDataARB);
   SET_BufferSubDataARB(exec, _mesa_BufferSubDataARB);
   SET_DeleteBuffersARB(exec, _mesa_DeleteBuffersARB);
   SET_GenBuffersARB(exec, _mesa_GenBuffersARB);
   SET_GetBufferParameterivARB(exec, _mesa_GetBufferParameterivARB);
   SET_GetBufferPointervARB(exec, _mesa_GetBufferPointervARB);
   SET_GetBufferSubDataARB(exec, _mesa_GetBufferSubDataARB);
   SET_IsBufferARB(exec, _mesa_IsBufferARB);
   SET_MapBufferARB(exec, _mesa_MapBufferARB);
   SET_UnmapBufferARB(exec, _mesa_UnmapBufferARB);
#endif

   /* ARB 29. GL_ARB_occlusion_query */
#if FEATURE_ARB_occlusion_query
   SET_GenQueriesARB(exec, _mesa_GenQueriesARB);
   SET_DeleteQueriesARB(exec, _mesa_DeleteQueriesARB);
   SET_IsQueryARB(exec, _mesa_IsQueryARB);
   SET_BeginQueryARB(exec, _mesa_BeginQueryARB);
   SET_EndQueryARB(exec, _mesa_EndQueryARB);
   SET_GetQueryivARB(exec, _mesa_GetQueryivARB);
   SET_GetQueryObjectivARB(exec, _mesa_GetQueryObjectivARB);
   SET_GetQueryObjectuivARB(exec, _mesa_GetQueryObjectuivARB);
#endif

   /* ARB 37. GL_ARB_draw_buffers */
   SET_DrawBuffersARB(exec, _mesa_DrawBuffersARB);
   
#if FEATURE_ARB_shader_objects
   SET_DeleteObjectARB(exec, _mesa_DeleteObjectARB);
   SET_GetHandleARB(exec, _mesa_GetHandleARB);
   SET_DetachObjectARB(exec, _mesa_DetachObjectARB);
   SET_CreateShaderObjectARB(exec, _mesa_CreateShaderObjectARB);
   SET_ShaderSourceARB(exec, _mesa_ShaderSourceARB);
   SET_CompileShaderARB(exec, _mesa_CompileShaderARB);
   SET_CreateProgramObjectARB(exec, _mesa_CreateProgramObjectARB);
   SET_AttachObjectARB(exec, _mesa_AttachObjectARB);
   SET_LinkProgramARB(exec, _mesa_LinkProgramARB);
   SET_UseProgramObjectARB(exec, _mesa_UseProgramObjectARB);
   SET_ValidateProgramARB(exec, _mesa_ValidateProgramARB);
   SET_Uniform1fARB(exec, _mesa_Uniform1fARB);
   SET_Uniform2fARB(exec, _mesa_Uniform2fARB);
   SET_Uniform3fARB(exec, _mesa_Uniform3fARB);
   SET_Uniform4fARB(exec, _mesa_Uniform4fARB);
   SET_Uniform1iARB(exec, _mesa_Uniform1iARB);
   SET_Uniform2iARB(exec, _mesa_Uniform2iARB);
   SET_Uniform3iARB(exec, _mesa_Uniform3iARB);
   SET_Uniform4iARB(exec, _mesa_Uniform4iARB);
   SET_Uniform1fvARB(exec, _mesa_Uniform1fvARB);
   SET_Uniform2fvARB(exec, _mesa_Uniform2fvARB);
   SET_Uniform3fvARB(exec, _mesa_Uniform3fvARB);
   SET_Uniform4fvARB(exec, _mesa_Uniform4fvARB);
   SET_Uniform1ivARB(exec, _mesa_Uniform1ivARB);
   SET_Uniform2ivARB(exec, _mesa_Uniform2ivARB);
   SET_Uniform3ivARB(exec, _mesa_Uniform3ivARB);
   SET_Uniform4ivARB(exec, _mesa_Uniform4ivARB);
   SET_UniformMatrix2fvARB(exec, _mesa_UniformMatrix2fvARB);
   SET_UniformMatrix3fvARB(exec, _mesa_UniformMatrix3fvARB);
   SET_UniformMatrix4fvARB(exec, _mesa_UniformMatrix4fvARB);
   SET_GetObjectParameterfvARB(exec, _mesa_GetObjectParameterfvARB);
   SET_GetObjectParameterivARB(exec, _mesa_GetObjectParameterivARB);
   SET_GetInfoLogARB(exec, _mesa_GetInfoLogARB);
   SET_GetAttachedObjectsARB(exec, _mesa_GetAttachedObjectsARB);
   SET_GetUniformLocationARB(exec, _mesa_GetUniformLocationARB);
   SET_GetActiveUniformARB(exec, _mesa_GetActiveUniformARB);
   SET_GetUniformfvARB(exec, _mesa_GetUniformfvARB);
   SET_GetUniformivARB(exec, _mesa_GetUniformivARB);
   SET_GetShaderSourceARB(exec, _mesa_GetShaderSourceARB);
#endif    /* FEATURE_ARB_shader_objects */

#if FEATURE_ARB_vertex_shader
   SET_BindAttribLocationARB(exec, _mesa_BindAttribLocationARB);
   SET_GetActiveAttribARB(exec, _mesa_GetActiveAttribARB);
   SET_GetAttribLocationARB(exec, _mesa_GetAttribLocationARB);
#endif    /* FEATURE_ARB_vertex_shader */

  /* GL_ATI_fragment_shader */
#if FEATURE_ATI_fragment_shader
   SET_GenFragmentShadersATI(exec, _mesa_GenFragmentShadersATI);
   SET_BindFragmentShaderATI(exec, _mesa_BindFragmentShaderATI);
   SET_DeleteFragmentShaderATI(exec, _mesa_DeleteFragmentShaderATI);
   SET_BeginFragmentShaderATI(exec, _mesa_BeginFragmentShaderATI);
   SET_EndFragmentShaderATI(exec, _mesa_EndFragmentShaderATI);
   SET_PassTexCoordATI(exec, _mesa_PassTexCoordATI);
   SET_SampleMapATI(exec, _mesa_SampleMapATI);
   SET_ColorFragmentOp1ATI(exec, _mesa_ColorFragmentOp1ATI);
   SET_ColorFragmentOp2ATI(exec, _mesa_ColorFragmentOp2ATI);
   SET_ColorFragmentOp3ATI(exec, _mesa_ColorFragmentOp3ATI);
   SET_AlphaFragmentOp1ATI(exec, _mesa_AlphaFragmentOp1ATI);
   SET_AlphaFragmentOp2ATI(exec, _mesa_AlphaFragmentOp2ATI);
   SET_AlphaFragmentOp3ATI(exec, _mesa_AlphaFragmentOp3ATI);
   SET_SetFragmentShaderConstantATI(exec, _mesa_SetFragmentShaderConstantATI);
#endif

#if FEATURE_EXT_framebuffer_object
   SET_IsRenderbufferEXT(exec, _mesa_IsRenderbufferEXT);
   SET_BindRenderbufferEXT(exec, _mesa_BindRenderbufferEXT);
   SET_DeleteRenderbuffersEXT(exec, _mesa_DeleteRenderbuffersEXT);
   SET_GenRenderbuffersEXT(exec, _mesa_GenRenderbuffersEXT);
   SET_RenderbufferStorageEXT(exec, _mesa_RenderbufferStorageEXT);
   SET_GetRenderbufferParameterivEXT(exec, _mesa_GetRenderbufferParameterivEXT);
   SET_IsFramebufferEXT(exec, _mesa_IsFramebufferEXT);
   SET_BindFramebufferEXT(exec, _mesa_BindFramebufferEXT);
   SET_DeleteFramebuffersEXT(exec, _mesa_DeleteFramebuffersEXT);
   SET_GenFramebuffersEXT(exec, _mesa_GenFramebuffersEXT);
   SET_CheckFramebufferStatusEXT(exec, _mesa_CheckFramebufferStatusEXT);
   SET_FramebufferTexture1DEXT(exec, _mesa_FramebufferTexture1DEXT);
   SET_FramebufferTexture2DEXT(exec, _mesa_FramebufferTexture2DEXT);
   SET_FramebufferTexture3DEXT(exec, _mesa_FramebufferTexture3DEXT);
   SET_FramebufferRenderbufferEXT(exec, _mesa_FramebufferRenderbufferEXT);
   SET_GetFramebufferAttachmentParameterivEXT(exec, _mesa_GetFramebufferAttachmentParameterivEXT);
   SET_GenerateMipmapEXT(exec, _mesa_GenerateMipmapEXT);
#endif

#if FEATURE_EXT_timer_query
   SET_GetQueryObjecti64vEXT(exec, _mesa_GetQueryObjecti64vEXT);
   SET_GetQueryObjectui64vEXT(exec, _mesa_GetQueryObjectui64vEXT);
#endif

#if FEATURE_EXT_framebuffer_blit
   SET_BlitFramebufferEXT(exec, _mesa_BlitFramebufferEXT);
#endif

   /* GL_EXT_gpu_program_parameters */
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
   SET_ProgramEnvParameters4fvEXT(exec, _mesa_ProgramEnvParameters4fvEXT);
   SET_ProgramLocalParameters4fvEXT(exec, _mesa_ProgramLocalParameters4fvEXT);
#endif

   /* GL_MESA_texture_array / GL_EXT_texture_array */
#if FEATURE_EXT_framebuffer_object
   SET_FramebufferTextureLayerEXT(exec, _mesa_FramebufferTextureLayerEXT);
#endif

   /* GL_ATI_separate_stencil */
   SET_StencilFuncSeparateATI(exec, _mesa_StencilFuncSeparateATI);
}



/**********************************************************************/
/** \name State update logic */
/*@{*/


static void
update_separate_specular(GLcontext *ctx)
{
   if (NEED_SECONDARY_COLOR(ctx))
      ctx->_TriangleCaps |= DD_SEPARATE_SPECULAR;
   else
      ctx->_TriangleCaps &= ~DD_SEPARATE_SPECULAR;
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
   if (ctx->VertexProgram._Current
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_POS].Enabled) {
      min = ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_POS]._MaxElement;
   }
   else if (ctx->Array.ArrayObj->Vertex.Enabled) {
      min = ctx->Array.ArrayObj->Vertex._MaxElement;
   }
   else {
      /* can't draw anything without vertex positions! */
      min = 0;
   }

   /* 1 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_WEIGHT].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_WEIGHT]._MaxElement);
   }
   /* no conventional vertex weight array */

   /* 2 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_NORMAL].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_NORMAL]._MaxElement);
   }
   else if (ctx->Array.ArrayObj->Normal.Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->Normal._MaxElement);
   }

   /* 3 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR0].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR0]._MaxElement);
   }
   else if (ctx->Array.ArrayObj->Color.Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->Color._MaxElement);
   }

   /* 4 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR1].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR1]._MaxElement);
   }
   else if (ctx->Array.ArrayObj->SecondaryColor.Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->SecondaryColor._MaxElement);
   }

   /* 5 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_FOG].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_FOG]._MaxElement);
   }
   else if (ctx->Array.ArrayObj->FogCoord.Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->FogCoord._MaxElement);
   }

   /* 6 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR_INDEX].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR_INDEX]._MaxElement);
   }
   else if (ctx->Array.ArrayObj->Index.Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->Index._MaxElement);
   }


   /* 7 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_EDGEFLAG].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_EDGEFLAG]._MaxElement);
   }

   /* 8..15 */
   for (i = VERT_ATTRIB_TEX0; i <= VERT_ATTRIB_TEX7; i++) {
      if (ctx->VertexProgram._Enabled
          && ctx->Array.ArrayObj->VertexAttrib[i].Enabled) {
         min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[i]._MaxElement);
      }
      else if (i - VERT_ATTRIB_TEX0 < ctx->Const.MaxTextureCoordUnits
               && ctx->Array.ArrayObj->TexCoord[i - VERT_ATTRIB_TEX0].Enabled) {
         min = MIN2(min, ctx->Array.ArrayObj->TexCoord[i - VERT_ATTRIB_TEX0]._MaxElement);
      }
   }

   /* 16..31 */
   if (ctx->VertexProgram._Current) {
      for (i = VERT_ATTRIB_GENERIC0; i < VERT_ATTRIB_MAX; i++) {
         if (ctx->Array.ArrayObj->VertexAttrib[i].Enabled) {
            min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[i]._MaxElement);
         }
      }
   }

   if (ctx->Array.ArrayObj->EdgeFlag.Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->EdgeFlag._MaxElement);
   }

   /* _MaxElement is one past the last legal array element */
   ctx->Array._MaxElement = min;
}


/**
 * Update derived vertex/fragment program state.
 */
static void
update_program(GLcontext *ctx)
{
   const struct gl_shader_program *shProg = ctx->Shader.CurrentProgram;

   /* These _Enabled flags indicate if the program is enabled AND valid. */
   ctx->VertexProgram._Enabled = ctx->VertexProgram.Enabled
      && ctx->VertexProgram.Current->Base.Instructions;
   ctx->FragmentProgram._Enabled = ctx->FragmentProgram.Enabled
      && ctx->FragmentProgram.Current->Base.Instructions;
   ctx->ATIFragmentShader._Enabled = ctx->ATIFragmentShader.Enabled
      && ctx->ATIFragmentShader.Current->Instructions[0];

   /*
    * Set the ctx->VertexProgram._Current and ctx->FragmentProgram._Current
    * pointers to the programs that should be enabled/used.
    *
    * These programs may come from several sources.  The priority is as
    * follows:
    *   1. OpenGL 2.0/ARB vertex/fragment shaders
    *   2. ARB/NV vertex/fragment programs
    *   3. Programs derived from fixed-function state.
    */

   _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current, NULL);

   if (shProg && shProg->LinkStatus) {
      /* Use shader programs */
      /* XXX this isn't quite right, since we may have either a vertex
       * _or_ fragment shader (not always both).
       */
      _mesa_reference_vertprog(ctx, &ctx->VertexProgram._Current,
                               shProg->VertexProgram);
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current,
                               shProg->FragmentProgram);
   }
   else {
      if (ctx->VertexProgram._Enabled) {
         /* use user-defined vertex program */
         _mesa_reference_vertprog(ctx, &ctx->VertexProgram._Current,
                                  ctx->VertexProgram.Current);
      }
      else if (ctx->VertexProgram._MaintainTnlProgram) {
         /* Use vertex program generated from fixed-function state.
          * The _Current pointer will get set in
          * _tnl_UpdateFixedFunctionProgram() later if appropriate.
          */
         _mesa_reference_vertprog(ctx, &ctx->VertexProgram._Current, NULL);
      }
      else {
         /* no vertex program */
         _mesa_reference_vertprog(ctx, &ctx->VertexProgram._Current, NULL);
      }

      if (ctx->FragmentProgram._Enabled) {
         /* use user-defined vertex program */
         _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current,
                                  ctx->FragmentProgram.Current);
      }
      else if (ctx->FragmentProgram._MaintainTexEnvProgram) {
         /* Use fragment program generated from fixed-function state.
          * The _Current pointer will get set in _mesa_UpdateTexEnvProgram()
          * later if appropriate.
          */
         _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current, NULL);
      }
      else {
         /* no fragment program */
         _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current, NULL);
      }
   }

   if (ctx->VertexProgram._Current)
      assert(ctx->VertexProgram._Current->Base.Parameters);
   if (ctx->FragmentProgram._Current)
      assert(ctx->FragmentProgram._Current->Base.Parameters);


   ctx->FragmentProgram._Active = ctx->FragmentProgram._Enabled;
   if (ctx->FragmentProgram._MaintainTexEnvProgram &&
       !ctx->FragmentProgram._Enabled) {
      if (ctx->FragmentProgram._UseTexEnvProgram)
	 ctx->FragmentProgram._Active = GL_TRUE;
   }
}


static void
update_viewport_matrix(GLcontext *ctx)
{
   const GLfloat depthMax = ctx->DrawBuffer->_DepthMaxF;

   ASSERT(depthMax > 0);

   /* Compute scale and bias values. This is really driver-specific
    * and should be maintained elsewhere if at all.
    * NOTE: RasterPos uses this.
    */
   _math_matrix_viewport(&ctx->Viewport._WindowMap,
                         ctx->Viewport.X, ctx->Viewport.Y,
                         ctx->Viewport.Width, ctx->Viewport.Height,
                         ctx->Viewport.Near, ctx->Viewport.Far,
                         depthMax);
}


/**
 * Update derived multisample state.
 */
static void
update_multisample(GLcontext *ctx)
{
   ctx->Multisample._Enabled = GL_FALSE;
   if (ctx->Multisample.Enabled &&
       ctx->DrawBuffer &&
       ctx->DrawBuffer->Visual.sampleBuffers)
      ctx->Multisample._Enabled = GL_TRUE;
}


/**
 * Update derived color/blend/logicop state.
 */
static void
update_color(GLcontext *ctx)
{
   /* This is needed to support 1.1's RGB logic ops AND
    * 1.0's blending logicops.
    */
   ctx->Color._LogicOpEnabled = RGBA_LOGICOP_ENABLED(ctx);
}


/*
 * Check polygon state and set DD_TRI_CULL_FRONT_BACK and/or DD_TRI_OFFSET
 * in ctx->_TriangleCaps if needed.
 */
static void
update_polygon(GLcontext *ctx)
{
   ctx->_TriangleCaps &= ~(DD_TRI_CULL_FRONT_BACK | DD_TRI_OFFSET);

   if (ctx->Polygon.CullFlag && ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
      ctx->_TriangleCaps |= DD_TRI_CULL_FRONT_BACK;

   if (   ctx->Polygon.OffsetPoint
       || ctx->Polygon.OffsetLine
       || ctx->Polygon.OffsetFill)
      ctx->_TriangleCaps |= DD_TRI_OFFSET;
}


/**
 * Update the ctx->_TriangleCaps bitfield.
 * XXX that bitfield should really go away someday!
 * This function must be called after other update_*() functions since
 * there are dependencies on some other derived values.
 */
#if 0
static void
update_tricaps(GLcontext *ctx, GLbitfield new_state)
{
   ctx->_TriangleCaps = 0;

   /*
    * Points
    */
   if (1/*new_state & _NEW_POINT*/) {
      if (ctx->Point.SmoothFlag)
         ctx->_TriangleCaps |= DD_POINT_SMOOTH;
      if (ctx->Point.Size != 1.0F)
         ctx->_TriangleCaps |= DD_POINT_SIZE;
      if (ctx->Point._Attenuated)
         ctx->_TriangleCaps |= DD_POINT_ATTEN;
   }

   /*
    * Lines
    */
   if (1/*new_state & _NEW_LINE*/) {
      if (ctx->Line.SmoothFlag)
         ctx->_TriangleCaps |= DD_LINE_SMOOTH;
      if (ctx->Line.StippleFlag)
         ctx->_TriangleCaps |= DD_LINE_STIPPLE;
      if (ctx->Line.Width != 1.0)
         ctx->_TriangleCaps |= DD_LINE_WIDTH;
   }

   /*
    * Polygons
    */
   if (1/*new_state & _NEW_POLYGON*/) {
      if (ctx->Polygon.SmoothFlag)
         ctx->_TriangleCaps |= DD_TRI_SMOOTH;
      if (ctx->Polygon.StippleFlag)
         ctx->_TriangleCaps |= DD_TRI_STIPPLE;
      if (ctx->Polygon.FrontMode != GL_FILL
          || ctx->Polygon.BackMode != GL_FILL)
         ctx->_TriangleCaps |= DD_TRI_UNFILLED;
      if (ctx->Polygon.CullFlag
          && ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
         ctx->_TriangleCaps |= DD_TRI_CULL_FRONT_BACK;
      if (ctx->Polygon.OffsetPoint ||
          ctx->Polygon.OffsetLine ||
          ctx->Polygon.OffsetFill)
         ctx->_TriangleCaps |= DD_TRI_OFFSET;
   }

   /*
    * Lighting and shading
    */
   if (ctx->Light.Enabled && ctx->Light.Model.TwoSide)
      ctx->_TriangleCaps |= DD_TRI_LIGHT_TWOSIDE;
   if (ctx->Light.ShadeModel == GL_FLAT)
      ctx->_TriangleCaps |= DD_FLATSHADE;
   if (NEED_SECONDARY_COLOR(ctx))
      ctx->_TriangleCaps |= DD_SEPARATE_SPECULAR;

   /*
    * Stencil
    */
   if (ctx->Stencil._TestTwoSide)
      ctx->_TriangleCaps |= DD_TRI_TWOSTENCIL;
}
#endif


/**
 * Compute derived GL state.
 * If __GLcontextRec::NewState is non-zero then this function \b must
 * be called before rendering anything.
 *
 * Calls dd_function_table::UpdateState to perform any internal state
 * management necessary.
 * 
 * \sa _mesa_update_modelview_project(), _mesa_update_texture(),
 * _mesa_update_buffer_bounds(),
 * _mesa_update_lighting() and _mesa_update_tnl_spaces().
 */
void
_mesa_update_state_locked( GLcontext *ctx )
{
   GLbitfield new_state = ctx->NewState;

   if (MESA_VERBOSE & VERBOSE_STATE)
      _mesa_print_state("_mesa_update_state", new_state);

   if (new_state & _NEW_PROGRAM)
      update_program( ctx );

   if (new_state & (_NEW_MODELVIEW|_NEW_PROJECTION))
      _mesa_update_modelview_project( ctx, new_state );

   if (new_state & (_NEW_PROGRAM|_NEW_TEXTURE|_NEW_TEXTURE_MATRIX))
      _mesa_update_texture( ctx, new_state );

   if (new_state & (_NEW_BUFFERS | _NEW_COLOR | _NEW_PIXEL))
      _mesa_update_framebuffer(ctx);

   if (new_state & (_NEW_SCISSOR | _NEW_BUFFERS | _NEW_VIEWPORT))
      _mesa_update_draw_buffer_bounds( ctx );

   if (new_state & _NEW_POLYGON)
      update_polygon( ctx );

   if (new_state & _NEW_LIGHT)
      _mesa_update_lighting( ctx );

   if (new_state & _NEW_STENCIL)
      _mesa_update_stencil( ctx );

   if (new_state & _IMAGE_NEW_TRANSFER_STATE)
      _mesa_update_pixel( ctx, new_state );

   if (new_state & _DD_NEW_SEPARATE_SPECULAR)
      update_separate_specular( ctx );

   if (new_state & (_NEW_ARRAY | _NEW_PROGRAM))
      update_arrays( ctx );

   if (new_state & (_NEW_BUFFERS | _NEW_VIEWPORT))
      update_viewport_matrix(ctx);

   if (new_state & _NEW_MULTISAMPLE)
      update_multisample( ctx );

   if (new_state & _NEW_COLOR)
      update_color( ctx );

#if 0
   if (new_state & (_NEW_POINT | _NEW_LINE | _NEW_POLYGON | _NEW_LIGHT
                    | _NEW_STENCIL | _DD_NEW_SEPARATE_SPECULAR))
      update_tricaps( ctx, new_state );
#endif

   if (ctx->FragmentProgram._MaintainTexEnvProgram) {
      if (new_state & (_NEW_TEXTURE | _DD_NEW_SEPARATE_SPECULAR | _NEW_FOG))
	 _mesa_UpdateTexEnvProgram(ctx);
   }

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
    * Give the driver a chance to act upon the new_state flags.
    * The driver might plug in different span functions, for example.
    * Also, this is where the driver can invalidate the state of any
    * active modules (such as swrast_setup, swrast, tnl, etc).
    *
    * Set ctx->NewState to zero to avoid recursion if
    * Driver.UpdateState() has to call FLUSH_VERTICES().  (fixed?)
    */
   new_state = ctx->NewState;
   ctx->NewState = 0;
   ctx->Driver.UpdateState(ctx, new_state);
   ctx->Array.NewState = 0;
}


/* This is the usual entrypoint for state updates:
 */
void
_mesa_update_state( GLcontext *ctx )
{
   _mesa_lock_context_textures(ctx);
   _mesa_update_state_locked(ctx);
   _mesa_unlock_context_textures(ctx);
}



/*@}*/


