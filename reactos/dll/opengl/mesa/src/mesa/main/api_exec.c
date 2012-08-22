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


#include "mfeatures.h"
#include "accum.h"
#include "api_loopback.h"
#include "api_exec.h"
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
#include "arbprogram.h"
#endif
#include "atifragshader.h"
#include "attrib.h"
#include "blend.h"
#include "bufferobj.h"
#include "arrayobj.h"
#if FEATURE_draw_read_buffer
#include "buffers.h"
#endif
#include "clear.h"
#include "clip.h"
#include "colortab.h"
#include "condrender.h"
#include "context.h"
#include "convolve.h"
#include "depth.h"
#include "dlist.h"
#include "drawpix.h"
#include "rastpos.h"
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
#include "matrix.h"
#include "multisample.h"
#include "pixel.h"
#include "pixelstore.h"
#include "points.h"
#include "polygon.h"
#include "queryobj.h"
#include "readpix.h"
#if FEATURE_ARB_sampler_objects
#include "samplerobj.h"
#endif
#include "scissor.h"
#include "stencil.h"
#include "texenv.h"
#include "texgetimage.h"
#include "teximage.h"
#include "texgen.h"
#include "texobj.h"
#include "texparam.h"
#include "texstate.h"
#include "texstorage.h"
#include "texturebarrier.h"
#include "transformfeedback.h"
#include "mtypes.h"
#include "varray.h"
#include "viewport.h"
#if FEATURE_NV_vertex_program || FEATURE_NV_fragment_program
#include "nvprogram.h"
#endif
#if FEATURE_ARB_shader_objects
#include "shaderapi.h"
#include "uniforms.h"
#endif
#include "syncobj.h"
#include "main/dispatch.h"


#if FEATURE_GL


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
struct _glapi_table *
_mesa_create_exec_table(void)
{
   struct _glapi_table *exec;

   exec = _mesa_alloc_dispatch_table(_gloffset_COUNT);
   if (exec == NULL)
      return NULL;

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

   _mesa_init_accum_dispatch(exec);
   _mesa_init_dlist_dispatch(exec);

   SET_ClearDepth(exec, _mesa_ClearDepth);
   SET_ClearIndex(exec, _mesa_ClearIndex);
   SET_ClipPlane(exec, _mesa_ClipPlane);
   SET_ColorMaterial(exec, _mesa_ColorMaterial);
   SET_DepthFunc(exec, _mesa_DepthFunc);
   SET_DepthMask(exec, _mesa_DepthMask);
   SET_DepthRange(exec, _mesa_DepthRange);

   _mesa_init_drawpix_dispatch(exec);
   _mesa_init_feedback_dispatch(exec);

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

   _mesa_init_eval_dispatch(exec);

   SET_MultMatrixd(exec, _mesa_MultMatrixd);

   _mesa_init_pixel_dispatch(exec);

   SET_PixelStoref(exec, _mesa_PixelStoref);
   SET_PointSize(exec, _mesa_PointSize);
   SET_PolygonMode(exec, _mesa_PolygonMode);
   SET_PolygonOffset(exec, _mesa_PolygonOffset);
   SET_PolygonStipple(exec, _mesa_PolygonStipple);

   _mesa_init_attrib_dispatch(exec);
   _mesa_init_rastpos_dispatch(exec);

   SET_ReadPixels(exec, _mesa_ReadPixels);
   SET_Rotated(exec, _mesa_Rotated);
   SET_Scaled(exec, _mesa_Scaled);
   SET_SecondaryColorPointerEXT(exec, _mesa_SecondaryColorPointerEXT);
   SET_TexEnvf(exec, _mesa_TexEnvf);
   SET_TexEnviv(exec, _mesa_TexEnviv);

   _mesa_init_texgen_dispatch(exec);

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

   _mesa_init_colortable_dispatch(exec);
   _mesa_init_convolve_dispatch(exec);
   _mesa_init_histogram_dispatch(exec);

   /* OpenGL 2.0 */
   SET_StencilFuncSeparate(exec, _mesa_StencilFuncSeparate);
   SET_StencilMaskSeparate(exec, _mesa_StencilMaskSeparate);
   SET_StencilOpSeparate(exec, _mesa_StencilOpSeparate);

#if FEATURE_ARB_shader_objects
   _mesa_init_shader_dispatch(exec);
   _mesa_init_shader_uniform_dispatch(exec);
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

   /* 95. GL_ARB_ES2_compatibility */
   SET_ClearDepthf(exec, _mesa_ClearDepthf);
   SET_DepthRangef(exec, _mesa_DepthRangef);

   /* 97. GL_EXT_compiled_vertex_array */
#if _HAVE_FULL_GL
   SET_LockArraysEXT(exec, _mesa_LockArraysEXT);
   SET_UnlockArraysEXT(exec, _mesa_UnlockArraysEXT);
#endif

   /* 148. GL_EXT_multi_draw_arrays */
#if _HAVE_FULL_GL
   SET_MultiDrawArraysEXT(exec, _mesa_MultiDrawArraysEXT);
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
   /* part of _mesa_init_rastpos_dispatch(exec); */

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

   /* 285. GL_NV_primitive_restart */
   SET_PrimitiveRestartIndexNV(exec, _mesa_PrimitiveRestartIndex);

   /* ???. GL_EXT_depth_bounds_test */
   SET_DepthBoundsEXT(exec, _mesa_DepthBoundsEXT);

   /* 352. GL_EXT_transform_feedback */
   /* ARB 93. GL_ARB_transform_feedback2 */
   _mesa_init_transform_feedback_dispatch(exec);

   /* 364. GL_EXT_provoking_vertex */
   SET_ProvokingVertexEXT(exec, _mesa_ProvokingVertexEXT);

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

   /* ARB 104. GL_ARB_robustness */
   SET_GetnCompressedTexImageARB(exec, _mesa_GetnCompressedTexImageARB);
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

   /* ARB 29. GL_ARB_occlusion_query */
   _mesa_init_queryobj_dispatch(exec);

   /* ARB 37. GL_ARB_draw_buffers */
#if FEATURE_draw_read_buffer
   SET_DrawBuffersARB(exec, _mesa_DrawBuffersARB);
#endif

   /* ARB 104. GL_ARB_robustness */
   SET_GetGraphicsResetStatusARB(exec, _mesa_GetGraphicsResetStatusARB);
   SET_GetnPolygonStippleARB(exec, _mesa_GetnPolygonStippleARB);
   SET_GetnTexImageARB(exec, _mesa_GetnTexImageARB);
   SET_ReadnPixelsARB(exec, _mesa_ReadnPixelsARB);

   /* GL_ARB_sync */
   _mesa_init_sync_dispatch(exec);

  /* GL_ATI_fragment_shader */
   _mesa_init_ati_fragment_shader_dispatch(exec);

  /* GL_ATI_envmap_bumpmap */
   SET_GetTexBumpParameterivATI(exec, _mesa_GetTexBumpParameterivATI);
   SET_GetTexBumpParameterfvATI(exec, _mesa_GetTexBumpParameterfvATI);
   SET_TexBumpParameterivATI(exec, _mesa_TexBumpParameterivATI);
   SET_TexBumpParameterfvATI(exec, _mesa_TexBumpParameterfvATI);

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

#if FEATURE_ARB_framebuffer_object
   /* The ARB_fbo functions are the union of
    * GL_EXT_fbo, GL_EXT_framebuffer_blit, GL_EXT_texture_array
    */
   SET_RenderbufferStorageMultisample(exec, _mesa_RenderbufferStorageMultisample);
#endif

#if FEATURE_ARB_map_buffer_range
   SET_MapBufferRange(exec, _mesa_MapBufferRange);
   SET_FlushMappedBufferRange(exec, _mesa_FlushMappedBufferRange);
#endif

   /* GL_ARB_copy_buffer */
   SET_CopyBufferSubData(exec, _mesa_CopyBufferSubData);

   /* GL_ARB_vertex_array_object */
   SET_BindVertexArray(exec, _mesa_BindVertexArray);
   SET_GenVertexArrays(exec, _mesa_GenVertexArrays);

   /* GL_EXT_draw_buffers2 */
   SET_ColorMaskIndexedEXT(exec, _mesa_ColorMaskIndexed);
   SET_GetBooleanIndexedvEXT(exec, _mesa_GetBooleanIndexedv);
   SET_GetIntegerIndexedvEXT(exec, _mesa_GetIntegerIndexedv);
   SET_EnableIndexedEXT(exec, _mesa_EnableIndexed);
   SET_DisableIndexedEXT(exec, _mesa_DisableIndexed);
   SET_IsEnabledIndexedEXT(exec, _mesa_IsEnabledIndexed);

   /* GL_NV_conditional_render */
   SET_BeginConditionalRenderNV(exec, _mesa_BeginConditionalRender);
   SET_EndConditionalRenderNV(exec, _mesa_EndConditionalRender);

#if FEATURE_OES_EGL_image
   SET_EGLImageTargetTexture2DOES(exec, _mesa_EGLImageTargetTexture2DOES);
   SET_EGLImageTargetRenderbufferStorageOES(exec, _mesa_EGLImageTargetRenderbufferStorageOES);
#endif

#if FEATURE_APPLE_object_purgeable
   SET_ObjectPurgeableAPPLE(exec, _mesa_ObjectPurgeableAPPLE);
   SET_ObjectUnpurgeableAPPLE(exec, _mesa_ObjectUnpurgeableAPPLE);
   SET_GetObjectParameterivAPPLE(exec, _mesa_GetObjectParameterivAPPLE);
#endif

#if FEATURE_ARB_geometry_shader4
   SET_FramebufferTextureARB(exec, _mesa_FramebufferTextureARB);
   SET_FramebufferTextureFaceARB(exec, _mesa_FramebufferTextureFaceARB);
#endif

   SET_ClampColorARB(exec, _mesa_ClampColorARB);

   /* GL_EXT_texture_integer */
   SET_ClearColorIiEXT(exec, _mesa_ClearColorIiEXT);
   SET_ClearColorIuiEXT(exec, _mesa_ClearColorIuiEXT);
   SET_GetTexParameterIivEXT(exec, _mesa_GetTexParameterIiv);
   SET_GetTexParameterIuivEXT(exec, _mesa_GetTexParameterIuiv);
   SET_TexParameterIivEXT(exec, _mesa_TexParameterIiv);
   SET_TexParameterIuivEXT(exec, _mesa_TexParameterIuiv);

   /* GL_EXT_gpu_shader4 / OpenGL 3.0 */
   SET_GetVertexAttribIivEXT(exec, _mesa_GetVertexAttribIiv);
   SET_GetVertexAttribIuivEXT(exec, _mesa_GetVertexAttribIuiv);
   SET_VertexAttribIPointerEXT(exec, _mesa_VertexAttribIPointer);

   /* GL 3.0 (functions not covered by other extensions) */
   SET_ClearBufferiv(exec, _mesa_ClearBufferiv);
   SET_ClearBufferuiv(exec, _mesa_ClearBufferuiv);
   SET_ClearBufferfv(exec, _mesa_ClearBufferfv);
   SET_ClearBufferfi(exec, _mesa_ClearBufferfi);
   SET_GetStringi(exec, _mesa_GetStringi);
   SET_ClampColor(exec, _mesa_ClampColorARB);

   /* GL_ARB_instanced_arrays */
   SET_VertexAttribDivisorARB(exec, _mesa_VertexAttribDivisor);

   /* GL_ARB_draw_buffer_blend */
   SET_BlendFunciARB(exec, _mesa_BlendFunci);
   SET_BlendFuncSeparateiARB(exec, _mesa_BlendFuncSeparatei);
   SET_BlendEquationiARB(exec, _mesa_BlendEquationi);
   SET_BlendEquationSeparateiARB(exec, _mesa_BlendEquationSeparatei);

   /* GL_NV_texture_barrier */
   SET_TextureBarrierNV(exec, _mesa_TextureBarrierNV);
 
   /* GL_ARB_texture_buffer_object */
   SET_TexBufferARB(exec, _mesa_TexBuffer);

   /* GL_ARB_texture_storage */
   SET_TexStorage1D(exec, _mesa_TexStorage1D);
   SET_TexStorage2D(exec, _mesa_TexStorage2D);
   SET_TexStorage3D(exec, _mesa_TexStorage3D);
   SET_TextureStorage1DEXT(exec, _mesa_TextureStorage1DEXT);
   SET_TextureStorage2DEXT(exec, _mesa_TextureStorage2DEXT);
   SET_TextureStorage3DEXT(exec, _mesa_TextureStorage3DEXT);

#if FEATURE_ARB_sampler_objects
   _mesa_init_sampler_object_dispatch(exec);
#endif

   return exec;
}

#endif /* FEATURE_GL */
