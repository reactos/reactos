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

#include <precomp.h>

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

   /* 3. GL_EXT_polygon_offset */
#if _HAVE_FULL_GL
   SET_PolygonOffsetEXT(exec, _mesa_PolygonOffsetEXT);
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

   /* 197. GL_MESA_window_pos */
   /* part of _mesa_init_rastpos_dispatch(exec); */

   /* 200. GL_IBM_multimode_draw_arrays */
#if _HAVE_FULL_GL
   SET_MultiModeDrawArraysIBM(exec, _mesa_MultiModeDrawArraysIBM);
   SET_MultiModeDrawElementsIBM(exec, _mesa_MultiModeDrawElementsIBM);
#endif

   /* 262. GL_NV_point_sprite */
#if _HAVE_FULL_GL
   SET_PointParameteriNV(exec, _mesa_PointParameteri);
   SET_PointParameterivNV(exec, _mesa_PointParameteriv);
#endif

   /* ???. GL_EXT_depth_bounds_test */
   SET_DepthBoundsEXT(exec, _mesa_DepthBoundsEXT);

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

   /* ARB 14. GL_ARB_point_parameters */
   /* reuse EXT_point_parameters functions */

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

#if FEATURE_ARB_map_buffer_range
   SET_MapBufferRange(exec, _mesa_MapBufferRange);
   SET_FlushMappedBufferRange(exec, _mesa_FlushMappedBufferRange);
#endif

   /* GL_EXT_texture_integer */
   SET_ClearColorIiEXT(exec, _mesa_ClearColorIiEXT);
   SET_ClearColorIuiEXT(exec, _mesa_ClearColorIuiEXT);
   SET_GetTexParameterIivEXT(exec, _mesa_GetTexParameterIiv);
   SET_GetTexParameterIuivEXT(exec, _mesa_GetTexParameterIuiv);
   SET_TexParameterIivEXT(exec, _mesa_TexParameterIiv);
   SET_TexParameterIuivEXT(exec, _mesa_TexParameterIuiv);

   /* GL_ARB_texture_storage */
   SET_TexStorage1D(exec, _mesa_TexStorage1D);
   SET_TexStorage2D(exec, _mesa_TexStorage2D);
   SET_TexStorage3D(exec, _mesa_TexStorage3D);
   SET_TextureStorage1DEXT(exec, _mesa_TextureStorage1DEXT);
   SET_TextureStorage2DEXT(exec, _mesa_TextureStorage2DEXT);
   SET_TextureStorage3DEXT(exec, _mesa_TextureStorage3DEXT);

   return exec;
}

#endif /* FEATURE_GL */
