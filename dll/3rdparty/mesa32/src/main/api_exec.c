/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * \file api_exec.c
 * Initialize dispatch table with the immidiate mode functions.
 */


#include "glheader.h"
#if FEATURE_accum
#include "accum.h"
#endif
#include "api_loopback.h"
#include "api_exec.h"
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
#include "shader/arbprogram.h"
#endif
#if FEATURE_ATI_fragment_shader
#include "shader/atifragshader.h"
#endif
#if FEATURE_attrib_stack
#include "attrib.h"
#endif
#include "blend.h"
#if FEATURE_ARB_vertex_buffer_object
#include "bufferobj.h"
#endif
#include "arrayobj.h"
#if FEATURE_draw_read_buffer
#include "buffers.h"
#endif
#include "clear.h"
#include "clip.h"
#if FEATURE_colortable
#include "colortab.h"
#endif
#include "context.h"
#if FEATURE_convolve
#include "convolve.h"
#endif
#include "depth.h"
#if FEATURE_dlist
#include "dlist.h"
#endif
#if FEATURE_drawpix
#include "drawpix.h"
#include "rastpos.h"
#endif
#include "enable.h"
#if FEATURE_evaluators
#include "eval.h"
#endif
#include "get.h"
#if FEATURE_feedback
#include "feedback.h"
#endif
#include "fog.h"
#if FEATURE_EXT_framebuffer_object
#include "fbobject.h"
#endif
#include "ffvertex_prog.h"
#include "framebuffer.h"
#include "hint.h"
#if FEATURE_histogram
#include "histogram.h"
#endif
#include "imports.h"
#include "light.h"
#include "lines.h"
#include "macros.h"
#include "matrix.h"
#include "multisample.h"
#if FEATURE_pixel_transfer
#include "pixel.h"
#endif
#include "pixelstore.h"
#include "points.h"
#include "polygon.h"
#if FEATURE_ARB_occlusion_query || FEATURE_EXT_timer_query
#include "queryobj.h"
#endif
#include "readpix.h"
#include "scissor.h"
#include "state.h"
#include "stencil.h"
#include "texenv.h"
#include "teximage.h"
#if FEATURE_texgen
#include "texgen.h"
#endif
#include "texobj.h"
#include "texparam.h"
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
#if FEATURE_draw_read_buffer
   SET_DrawBuffer(exec, _mesa_DrawBuffer);
   SET_ReadBuffer(exec, _mesa_ReadBuffer);
#endif
   SET_Enable(exec, _mesa_Enable);
   SET_Finish(exec, _mesa_Finish);
   SET_Flush(exec, _mesa_Flush);
   SET_FrontFace(exec, _mesa_FrontFace);
   SET_Frustum(exec, _mesa_Frustum);
   SET_GetError(exec, _mesa_GetError);
   SET_GetFloatv(exec, _mesa_GetFloatv);
   SET_GetString(exec, _mesa_GetString);
   SET_LineStipple(exec, _mesa_LineStipple);
   SET_LineWidth(exec, _mesa_LineWidth);
   SET_LoadIdentity(exec, _mesa_LoadIdentity);
   SET_LoadMatrixf(exec, _mesa_LoadMatrixf);
   SET_LogicOp(exec, _mesa_LogicOp);
   SET_MatrixMode(exec, _mesa_MatrixMode);
   SET_MultMatrixf(exec, _mesa_MultMatrixf);
   SET_Ortho(exec, _mesa_Ortho);
   SET_PixelStorei(exec, _mesa_PixelStorei);
   SET_PopMatrix(exec, _mesa_PopMatrix);
   SET_PushMatrix(exec, _mesa_PushMatrix);
   SET_Rotatef(exec, _mesa_Rotatef);
   SET_Scalef(exec, _mesa_Scalef);
   SET_Scissor(exec, _mesa_Scissor);
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
#if FEATURE_accum
   SET_Accum(exec, _mesa_Accum);
   SET_ClearAccum(exec, _mesa_ClearAccum);
#endif
#if FEATURE_dlist
   SET_CallList(exec, _mesa_CallList);
   SET_CallLists(exec, _mesa_CallLists);
   SET_DeleteLists(exec, _mesa_DeleteLists);
   SET_EndList(exec, _mesa_EndList);
   SET_GenLists(exec, _mesa_GenLists);
   SET_IsList(exec, _mesa_IsList);
   SET_ListBase(exec, _mesa_ListBase);
   SET_NewList(exec, _mesa_NewList);
#endif
   SET_ClearDepth(exec, _mesa_ClearDepth);
   SET_ClearIndex(exec, _mesa_ClearIndex);
   SET_ClipPlane(exec, _mesa_ClipPlane);
   SET_ColorMaterial(exec, _mesa_ColorMaterial);
   SET_CullParameterfvEXT(exec, _mesa_CullParameterfvEXT);
   SET_CullParameterdvEXT(exec, _mesa_CullParameterdvEXT);
   SET_DepthFunc(exec, _mesa_DepthFunc);
   SET_DepthMask(exec, _mesa_DepthMask);
   SET_DepthRange(exec, _mesa_DepthRange);
#if FEATURE_drawpix
   SET_Bitmap(exec, _mesa_Bitmap);
   SET_CopyPixels(exec, _mesa_CopyPixels);
   SET_DrawPixels(exec, _mesa_DrawPixels);
#endif
#if FEATURE_feedback
   SET_InitNames(exec, _mesa_InitNames);
   SET_FeedbackBuffer(exec, _mesa_FeedbackBuffer);
   SET_LoadName(exec, _mesa_LoadName);
   SET_PassThrough(exec, _mesa_PassThrough);
   SET_PopName(exec, _mesa_PopName);
   SET_PushName(exec, _mesa_PushName);
   SET_SelectBuffer(exec, _mesa_SelectBuffer);
   SET_RenderMode(exec, _mesa_RenderMode);
#endif
   SET_FogCoordPointerEXT(exec, _mesa_FogCoordPointerEXT);
   SET_Fogf(exec, _mesa_Fogf);
   SET_Fogfv(exec, _mesa_Fogfv);
   SET_Fogi(exec, _mesa_Fogi);
   SET_Fogiv(exec, _mesa_Fogiv);
   SET_GetClipPlane(exec, _mesa_GetClipPlane);
   SET_GetBooleanv(exec, _mesa_GetBooleanv);
   SET_GetDoublev(exec, _mesa_GetDoublev);
   SET_GetIntegerv(exec, _mesa_GetIntegerv);
   SET_GetLightfv(exec, _mesa_GetLightfv);
   SET_GetLightiv(exec, _mesa_GetLightiv);
   SET_GetMaterialfv(exec, _mesa_GetMaterialfv);
   SET_GetMaterialiv(exec, _mesa_GetMaterialiv);
   SET_GetPolygonStipple(exec, _mesa_GetPolygonStipple);
   SET_GetTexEnvfv(exec, _mesa_GetTexEnvfv);
   SET_GetTexEnviv(exec, _mesa_GetTexEnviv);
   SET_GetTexLevelParameterfv(exec, _mesa_GetTexLevelParameterfv);
   SET_GetTexLevelParameteriv(exec, _mesa_GetTexLevelParameteriv);
   SET_GetTexParameterfv(exec, _mesa_GetTexParameterfv);
   SET_GetTexParameteriv(exec, _mesa_GetTexParameteriv);
   SET_GetTexImage(exec, _mesa_GetTexImage);
   SET_Hint(exec, _mesa_Hint);
   SET_IndexMask(exec, _mesa_IndexMask);
   SET_IsEnabled(exec, _mesa_IsEnabled);
   SET_LightModelf(exec, _mesa_LightModelf);
   SET_LightModelfv(exec, _mesa_LightModelfv);
   SET_LightModeli(exec, _mesa_LightModeli);
   SET_LightModeliv(exec, _mesa_LightModeliv);
   SET_Lightf(exec, _mesa_Lightf);
   SET_Lightfv(exec, _mesa_Lightfv);
   SET_Lighti(exec, _mesa_Lighti);
   SET_Lightiv(exec, _mesa_Lightiv);
   SET_LoadMatrixd(exec, _mesa_LoadMatrixd);
#if FEATURE_evaluators
   SET_GetMapdv(exec, _mesa_GetMapdv);
   SET_GetMapfv(exec, _mesa_GetMapfv);
   SET_GetMapiv(exec, _mesa_GetMapiv);
   SET_Map1d(exec, _mesa_Map1d);
   SET_Map1f(exec, _mesa_Map1f);
   SET_Map2d(exec, _mesa_Map2d);
   SET_Map2f(exec, _mesa_Map2f);
   SET_MapGrid1d(exec, _mesa_MapGrid1d);
   SET_MapGrid1f(exec, _mesa_MapGrid1f);
   SET_MapGrid2d(exec, _mesa_MapGrid2d);
   SET_MapGrid2f(exec, _mesa_MapGrid2f);
#endif
   SET_MultMatrixd(exec, _mesa_MultMatrixd);
#if FEATURE_pixel_transfer
   SET_GetPixelMapfv(exec, _mesa_GetPixelMapfv);
   SET_GetPixelMapuiv(exec, _mesa_GetPixelMapuiv);
   SET_GetPixelMapusv(exec, _mesa_GetPixelMapusv);
   SET_PixelMapfv(exec, _mesa_PixelMapfv);
   SET_PixelMapuiv(exec, _mesa_PixelMapuiv);
   SET_PixelMapusv(exec, _mesa_PixelMapusv);
   SET_PixelTransferf(exec, _mesa_PixelTransferf);
   SET_PixelTransferi(exec, _mesa_PixelTransferi);
   SET_PixelZoom(exec, _mesa_PixelZoom);
#endif
   SET_PixelStoref(exec, _mesa_PixelStoref);
   SET_PointSize(exec, _mesa_PointSize);
   SET_PolygonMode(exec, _mesa_PolygonMode);
   SET_PolygonOffset(exec, _mesa_PolygonOffset);
   SET_PolygonStipple(exec, _mesa_PolygonStipple);
#if FEATURE_attrib_stack
   SET_PopAttrib(exec, _mesa_PopAttrib);
   SET_PushAttrib(exec, _mesa_PushAttrib);
   SET_PopClientAttrib(exec, _mesa_PopClientAttrib);
   SET_PushClientAttrib(exec, _mesa_PushClientAttrib);
#endif
#if FEATURE_drawpix
   SET_RasterPos2f(exec, _mesa_RasterPos2f);
   SET_RasterPos2fv(exec, _mesa_RasterPos2fv);
   SET_RasterPos2i(exec, _mesa_RasterPos2i);
   SET_RasterPos2iv(exec, _mesa_RasterPos2iv);
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
#endif
   SET_ReadPixels(exec, _mesa_ReadPixels);
   SET_Rotated(exec, _mesa_Rotated);
   SET_Scaled(exec, _mesa_Scaled);
   SET_SecondaryColorPointerEXT(exec, _mesa_SecondaryColorPointerEXT);
   SET_TexEnvf(exec, _mesa_TexEnvf);
   SET_TexEnviv(exec, _mesa_TexEnviv);

#if FEATURE_texgen
   SET_GetTexGendv(exec, _mesa_GetTexGendv);
   SET_GetTexGenfv(exec, _mesa_GetTexGenfv);
   SET_GetTexGeniv(exec, _mesa_GetTexGeniv);
   SET_TexGend(exec, _mesa_TexGend);
   SET_TexGendv(exec, _mesa_TexGendv);
   SET_TexGenf(exec, _mesa_TexGenf);
   SET_TexGenfv(exec, _mesa_TexGenfv);
   SET_TexGeni(exec, _mesa_TexGeni);
   SET_TexGeniv(exec, _mesa_TexGeniv);
#endif

   SET_TexImage1D(exec, _mesa_TexImage1D);
   SET_TexParameterf(exec, _mesa_TexParameterf);
   SET_TexParameterfv(exec, _mesa_TexParameterfv);
   SET_TexParameteriv(exec, _mesa_TexParameteriv);
   SET_Translated(exec, _mesa_Translated);

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
   SET_PrioritizeTextures(exec, _mesa_PrioritizeTextures);
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
   SET_BlendColor(exec, _mesa_BlendColor);
   SET_BlendEquation(exec, _mesa_BlendEquation);
   SET_BlendEquationSeparateEXT(exec, _mesa_BlendEquationSeparateEXT);

#if FEATURE_colortable
   SET_ColorSubTable(exec, _mesa_ColorSubTable);
   SET_ColorTable(exec, _mesa_ColorTable);
   SET_ColorTableParameterfv(exec, _mesa_ColorTableParameterfv);
   SET_ColorTableParameteriv(exec, _mesa_ColorTableParameteriv);
   SET_CopyColorSubTable(exec, _mesa_CopyColorSubTable);
   SET_CopyColorTable(exec, _mesa_CopyColorTable);
   SET_GetColorTable(exec, _mesa_GetColorTable);
   SET_GetColorTableParameterfv(exec, _mesa_GetColorTableParameterfv);
   SET_GetColorTableParameteriv(exec, _mesa_GetColorTableParameteriv);
#endif

#if FEATURE_convolve
   SET_ConvolutionFilter1D(exec, _mesa_ConvolutionFilter1D);
   SET_ConvolutionFilter2D(exec, _mesa_ConvolutionFilter2D);
   SET_ConvolutionParameterf(exec, _mesa_ConvolutionParameterf);
   SET_ConvolutionParameterfv(exec, _mesa_ConvolutionParameterfv);
   SET_ConvolutionParameteri(exec, _mesa_ConvolutionParameteri);
   SET_ConvolutionParameteriv(exec, _mesa_ConvolutionParameteriv);
   SET_CopyConvolutionFilter1D(exec, _mesa_CopyConvolutionFilter1D);
   SET_CopyConvolutionFilter2D(exec, _mesa_CopyConvolutionFilter2D);
   SET_GetConvolutionFilter(exec, _mesa_GetConvolutionFilter);
   SET_GetConvolutionParameterfv(exec, _mesa_GetConvolutionParameterfv);
   SET_GetConvolutionParameteriv(exec, _mesa_GetConvolutionParameteriv);
   SET_SeparableFilter2D(exec, _mesa_SeparableFilter2D);
#endif
#if FEATURE_histogram
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
   SET_PointParameterfEXT(exec, _mesa_PointParameterf);
   SET_PointParameterfvEXT(exec, _mesa_PointParameterfv);
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
#if FEATURE_drawpix
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
   SET_PointParameteriNV(exec, _mesa_PointParameteri);
   SET_PointParameterivNV(exec, _mesa_PointParameteriv);
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
#if FEATURE_draw_read_buffer
   SET_DrawBuffersARB(exec, _mesa_DrawBuffersARB);
#endif

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

