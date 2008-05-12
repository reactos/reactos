/* DO NOT EDIT - This file generated automatically by extension_helper.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2005
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "utils.h"
#include "dispatch.h"

#ifndef NULL
# define NULL 0
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char UniformMatrix3fvARB_names[] = 
    "iiip\0" /* Parameter signature */
    "glUniformMatrix3fv\0"
    "glUniformMatrix3fvARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char ProgramParameter4fNV_names[] = 
    "iiffff\0" /* Parameter signature */
    "glProgramParameter4fNV\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3) || defined(need_GL_ARB_multisample)
static const char SampleCoverageARB_names[] = 
    "fi\0" /* Parameter signature */
    "glSampleCoverage\0"
    "glSampleCoverageARB\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char ConvolutionFilter1D_names[] = 
    "iiiiip\0" /* Parameter signature */
    "glConvolutionFilter1D\0"
    "glConvolutionFilter1DEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_occlusion_query)
static const char BeginQueryARB_names[] = 
    "ii\0" /* Parameter signature */
    "glBeginQuery\0"
    "glBeginQueryARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_NV_point_sprite)
static const char PointParameteriNV_names[] = 
    "ii\0" /* Parameter signature */
    "glPointParameteri\0"
    "glPointParameteriNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char GetProgramiv_names[] = 
    "iip\0" /* Parameter signature */
    "glGetProgramiv\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord3sARB_names[] = 
    "iiii\0" /* Parameter signature */
    "glMultiTexCoord3s\0"
    "glMultiTexCoord3sARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3iEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glSecondaryColor3i\0"
    "glSecondaryColor3iEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos3fMESA_names[] = 
    "fff\0" /* Parameter signature */
    "glWindowPos3f\0"
    "glWindowPos3fARB\0"
    "glWindowPos3fMESA\0"
    "";
#endif

#if defined(need_GL_SGIS_pixel_texture)
static const char PixelTexGenParameterfvSGIS_names[] = 
    "ip\0" /* Parameter signature */
    "glPixelTexGenParameterfvSGIS\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char ActiveTextureARB_names[] = 
    "i\0" /* Parameter signature */
    "glActiveTexture\0"
    "glActiveTextureARB\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_blit)
static const char BlitFramebufferEXT_names[] = 
    "iiiiiiiiii\0" /* Parameter signature */
    "glBlitFramebufferEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib4ubvNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4ubvNV\0"
    "";
#endif

#if defined(need_GL_NV_fragment_program)
static const char GetProgramNamedParameterdvNV_names[] = 
    "iipp\0" /* Parameter signature */
    "glGetProgramNamedParameterdvNV\0"
    "";
#endif

#if defined(need_GL_EXT_histogram)
static const char Histogram_names[] = 
    "iiii\0" /* Parameter signature */
    "glHistogram\0"
    "glHistogramEXT\0"
    "";
#endif

#if defined(need_GL_SGIS_texture4D)
static const char TexImage4DSGIS_names[] = 
    "iiiiiiiiiip\0" /* Parameter signature */
    "glTexImage4DSGIS\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos2dvMESA_names[] = 
    "p\0" /* Parameter signature */
    "glWindowPos2dv\0"
    "glWindowPos2dvARB\0"
    "glWindowPos2dvMESA\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiColor3fVertex3fvSUN_names[] = 
    "ppp\0" /* Parameter signature */
    "glReplacementCodeuiColor3fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_EXT_blend_equation_separate) || defined(need_GL_ATI_blend_equation_separate)
static const char BlendEquationSeparateEXT_names[] = 
    "ii\0" /* Parameter signature */
    "glBlendEquationSeparate\0"
    "glBlendEquationSeparateEXT\0"
    "glBlendEquationSeparateATI\0"
    "";
#endif

#if defined(need_GL_SGIX_list_priority)
static const char ListParameterfSGIX_names[] = 
    "iif\0" /* Parameter signature */
    "glListParameterfSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3bEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glSecondaryColor3b\0"
    "glSecondaryColor3bEXT\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord4fColor4fNormal3fVertex4fvSUN_names[] = 
    "pppp\0" /* Parameter signature */
    "glTexCoord4fColor4fNormal3fVertex4fvSUN\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib4svNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4svNV\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_vertex_buffer_object)
static const char GetBufferSubDataARB_names[] = 
    "iiip\0" /* Parameter signature */
    "glGetBufferSubData\0"
    "glGetBufferSubDataARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_vertex_buffer_object)
static const char BufferSubDataARB_names[] = 
    "iiip\0" /* Parameter signature */
    "glBufferSubData\0"
    "glBufferSubDataARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord2fColor4ubVertex3fvSUN_names[] = 
    "ppp\0" /* Parameter signature */
    "glTexCoord2fColor4ubVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char AttachShader_names[] = 
    "ii\0" /* Parameter signature */
    "glAttachShader\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib2fARB_names[] = 
    "iff\0" /* Parameter signature */
    "glVertexAttrib2f\0"
    "glVertexAttrib2fARB\0"
    "";
#endif

#if defined(need_GL_MESA_shader_debug)
static const char GetDebugLogLengthMESA_names[] = 
    "iii\0" /* Parameter signature */
    "glGetDebugLogLengthMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib3fARB_names[] = 
    "ifff\0" /* Parameter signature */
    "glVertexAttrib3f\0"
    "glVertexAttrib3fARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_occlusion_query)
static const char GetQueryivARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetQueryiv\0"
    "glGetQueryivARB\0"
    "";
#endif

#if defined(need_GL_EXT_texture3D)
static const char TexImage3D_names[] = 
    "iiiiiiiiip\0" /* Parameter signature */
    "glTexImage3D\0"
    "glTexImage3DEXT\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiVertex3fvSUN_names[] = 
    "pp\0" /* Parameter signature */
    "glReplacementCodeuiVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_occlusion_query)
static const char GetQueryObjectivARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetQueryObjectiv\0"
    "glGetQueryObjectivARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiTexCoord2fVertex3fvSUN_names[] = 
    "ppp\0" /* Parameter signature */
    "glReplacementCodeuiTexCoord2fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3) || defined(need_GL_ARB_texture_compression)
static const char CompressedTexSubImage2DARB_names[] = 
    "iiiiiiiip\0" /* Parameter signature */
    "glCompressedTexSubImage2D\0"
    "glCompressedTexSubImage2DARB\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char CombinerOutputNV_names[] = 
    "iiiiiiiiii\0" /* Parameter signature */
    "glCombinerOutputNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform2fARB_names[] = 
    "iff\0" /* Parameter signature */
    "glUniform2f\0"
    "glUniform2fARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib1svARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib1sv\0"
    "glVertexAttrib1svARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs1dvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs1dvNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform2ivARB_names[] = 
    "iip\0" /* Parameter signature */
    "glUniform2iv\0"
    "glUniform2ivARB\0"
    "";
#endif

#if defined(need_GL_HP_image_transform)
static const char GetImageTransformParameterfvHP_names[] = 
    "iip\0" /* Parameter signature */
    "glGetImageTransformParameterfvHP\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_blend)
static const char WeightubvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glWeightubvARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib1fvNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib1fvNV\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char CopyConvolutionFilter1D_names[] = 
    "iiiii\0" /* Parameter signature */
    "glCopyConvolutionFilter1D\0"
    "glCopyConvolutionFilter1DEXT\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiNormal3fVertex3fSUN_names[] = 
    "iffffff\0" /* Parameter signature */
    "glReplacementCodeuiNormal3fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentMaterialfvSGIX_names[] = 
    "iip\0" /* Parameter signature */
    "glFragmentMaterialfvSGIX\0"
    "";
#endif

#if defined(need_GL_EXT_blend_color)
static const char BlendColor_names[] = 
    "ffff\0" /* Parameter signature */
    "glBlendColor\0"
    "glBlendColorEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char UniformMatrix4fvARB_names[] = 
    "iiip\0" /* Parameter signature */
    "glUniformMatrix4fv\0"
    "glUniformMatrix4fvARB\0"
    "";
#endif

#if defined(need_GL_APPLE_vertex_array_object)
static const char DeleteVertexArraysAPPLE_names[] = 
    "ip\0" /* Parameter signature */
    "glDeleteVertexArraysAPPLE\0"
    "";
#endif

#if defined(need_GL_SGIX_instruments)
static const char ReadInstrumentsSGIX_names[] = 
    "i\0" /* Parameter signature */
    "glReadInstrumentsSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_2_1)
static const char UniformMatrix2x4fv_names[] = 
    "iiip\0" /* Parameter signature */
    "glUniformMatrix2x4fv\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char Color4ubVertex3fvSUN_names[] = 
    "pp\0" /* Parameter signature */
    "glColor4ubVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_SGIX_list_priority)
static const char GetListParameterivSGIX_names[] = 
    "iip\0" /* Parameter signature */
    "glGetListParameterivSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4NusvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4Nusv\0"
    "glVertexAttrib4NusvARB\0"
    "";
#endif

#if defined(need_GL_MESA_window_pos)
static const char WindowPos4svMESA_names[] = 
    "p\0" /* Parameter signature */
    "glWindowPos4svMESA\0"
    "";
#endif

#if defined(need_GL_ARB_shader_objects)
static const char CreateProgramObjectARB_names[] = 
    "\0" /* Parameter signature */
    "glCreateProgramObjectARB\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentLightModelivSGIX_names[] = 
    "ip\0" /* Parameter signature */
    "glFragmentLightModelivSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_2_1)
static const char UniformMatrix4x3fv_names[] = 
    "iiip\0" /* Parameter signature */
    "glUniformMatrix4x3fv\0"
    "";
#endif

#if defined(need_GL_EXT_texture_object)
static const char PrioritizeTextures_names[] = 
    "ipp\0" /* Parameter signature */
    "glPrioritizeTextures\0"
    "glPrioritizeTexturesEXT\0"
    "";
#endif

#if defined(need_GL_SGIX_async)
static const char AsyncMarkerSGIX_names[] = 
    "i\0" /* Parameter signature */
    "glAsyncMarkerSGIX\0"
    "";
#endif

#if defined(need_GL_SUN_global_alpha)
static const char GlobalAlphaFactorubSUN_names[] = 
    "i\0" /* Parameter signature */
    "glGlobalAlphaFactorubSUN\0"
    "";
#endif

#if defined(need_GL_MESA_shader_debug)
static const char ClearDebugLogMESA_names[] = 
    "iii\0" /* Parameter signature */
    "glClearDebugLogMESA\0"
    "";
#endif

#if defined(need_GL_EXT_histogram)
static const char ResetHistogram_names[] = 
    "i\0" /* Parameter signature */
    "glResetHistogram\0"
    "glResetHistogramEXT\0"
    "";
#endif

#if defined(need_GL_NV_fragment_program)
static const char GetProgramNamedParameterfvNV_names[] = 
    "iipp\0" /* Parameter signature */
    "glGetProgramNamedParameterfvNV\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_point_parameters) || defined(need_GL_EXT_point_parameters) || defined(need_GL_SGIS_point_parameters)
static const char PointParameterfEXT_names[] = 
    "if\0" /* Parameter signature */
    "glPointParameterf\0"
    "glPointParameterfARB\0"
    "glPointParameterfEXT\0"
    "glPointParameterfSGIS\0"
    "";
#endif

#if defined(need_GL_SGIX_polynomial_ffd)
static const char LoadIdentityDeformationMapSGIX_names[] = 
    "i\0" /* Parameter signature */
    "glLoadIdentityDeformationMapSGIX\0"
    "";
#endif

#if defined(need_GL_NV_fence)
static const char GenFencesNV_names[] = 
    "ip\0" /* Parameter signature */
    "glGenFencesNV\0"
    "";
#endif

#if defined(need_GL_HP_image_transform)
static const char ImageTransformParameterfHP_names[] = 
    "iif\0" /* Parameter signature */
    "glImageTransformParameterfHP\0"
    "";
#endif

#if defined(need_GL_ARB_matrix_palette)
static const char MatrixIndexusvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMatrixIndexusvARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char ProgramParameter4dvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glProgramParameter4dvNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char DisableVertexAttribArrayARB_names[] = 
    "i\0" /* Parameter signature */
    "glDisableVertexAttribArray\0"
    "glDisableVertexAttribArrayARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char StencilMaskSeparate_names[] = 
    "ii\0" /* Parameter signature */
    "glStencilMaskSeparate\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char ProgramLocalParameter4dARB_names[] = 
    "iidddd\0" /* Parameter signature */
    "glProgramLocalParameter4dARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3) || defined(need_GL_ARB_texture_compression)
static const char CompressedTexImage3DARB_names[] = 
    "iiiiiiiip\0" /* Parameter signature */
    "glCompressedTexImage3D\0"
    "glCompressedTexImage3DARB\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char GetConvolutionParameteriv_names[] = 
    "iip\0" /* Parameter signature */
    "glGetConvolutionParameteriv\0"
    "glGetConvolutionParameterivEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib1fARB_names[] = 
    "if\0" /* Parameter signature */
    "glVertexAttrib1f\0"
    "glVertexAttrib1fARB\0"
    "";
#endif

#if defined(need_GL_NV_fence)
static const char TestFenceNV_names[] = 
    "i\0" /* Parameter signature */
    "glTestFenceNV\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord1fvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord1fv\0"
    "glMultiTexCoord1fvARB\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char ColorFragmentOp2ATI_names[] = 
    "iiiiiiiiii\0" /* Parameter signature */
    "glColorFragmentOp2ATI\0"
    "";
#endif

#if defined(need_GL_IBM_vertex_array_lists)
static const char SecondaryColorPointerListIBM_names[] = 
    "iiipi\0" /* Parameter signature */
    "glSecondaryColorPointerListIBM\0"
    "";
#endif

#if defined(need_GL_SGIS_pixel_texture)
static const char GetPixelTexGenParameterivSGIS_names[] = 
    "ip\0" /* Parameter signature */
    "glGetPixelTexGenParameterivSGIS\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib4fNV_names[] = 
    "iffff\0" /* Parameter signature */
    "glVertexAttrib4fNV\0"
    "";
#endif

#if defined(need_GL_SUN_triangle_list)
static const char ReplacementCodeubSUN_names[] = 
    "i\0" /* Parameter signature */
    "glReplacementCodeubSUN\0"
    "";
#endif

#if defined(need_GL_SGIX_async)
static const char FinishAsyncSGIX_names[] = 
    "p\0" /* Parameter signature */
    "glFinishAsyncSGIX\0"
    "";
#endif

#if defined(need_GL_MESA_shader_debug)
static const char GetDebugLogMESA_names[] = 
    "iiiipp\0" /* Parameter signature */
    "glGetDebugLogMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_fog_coord)
static const char FogCoorddEXT_names[] = 
    "d\0" /* Parameter signature */
    "glFogCoordd\0"
    "glFogCoorddEXT\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char Color4ubVertex3fSUN_names[] = 
    "iiiifff\0" /* Parameter signature */
    "glColor4ubVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_fog_coord)
static const char FogCoordfEXT_names[] = 
    "f\0" /* Parameter signature */
    "glFogCoordf\0"
    "glFogCoordfEXT\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord2fVertex3fSUN_names[] = 
    "fffff\0" /* Parameter signature */
    "glTexCoord2fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_SUN_global_alpha)
static const char GlobalAlphaFactoriSUN_names[] = 
    "i\0" /* Parameter signature */
    "glGlobalAlphaFactoriSUN\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib2dNV_names[] = 
    "idd\0" /* Parameter signature */
    "glVertexAttrib2dNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char GetProgramInfoLog_names[] = 
    "iipp\0" /* Parameter signature */
    "glGetProgramInfoLog\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4NbvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4Nbv\0"
    "glVertexAttrib4NbvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_shader)
static const char GetActiveAttribARB_names[] = 
    "iiipppp\0" /* Parameter signature */
    "glGetActiveAttrib\0"
    "glGetActiveAttribARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib4ubNV_names[] = 
    "iiiii\0" /* Parameter signature */
    "glVertexAttrib4ubNV\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord2fColor4fNormal3fVertex3fSUN_names[] = 
    "ffffffffffff\0" /* Parameter signature */
    "glTexCoord2fColor4fNormal3fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char CombinerParameterfvNV_names[] = 
    "ip\0" /* Parameter signature */
    "glCombinerParameterfvNV\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs3dvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs3dvNV\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs4fvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs4fvNV\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentLightiSGIX_names[] = 
    "iii\0" /* Parameter signature */
    "glFragmentLightiSGIX\0"
    "";
#endif

#if defined(need_GL_EXT_polygon_offset)
static const char PolygonOffsetEXT_names[] = 
    "ff\0" /* Parameter signature */
    "glPolygonOffsetEXT\0"
    "";
#endif

#if defined(need_GL_SGIX_async)
static const char PollAsyncSGIX_names[] = 
    "p\0" /* Parameter signature */
    "glPollAsyncSGIX\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char DeleteFragmentShaderATI_names[] = 
    "i\0" /* Parameter signature */
    "glDeleteFragmentShaderATI\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord2fNormal3fVertex3fvSUN_names[] = 
    "ppp\0" /* Parameter signature */
    "glTexCoord2fNormal3fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3) || defined(need_GL_ARB_transpose_matrix)
static const char MultTransposeMatrixdARB_names[] = 
    "p\0" /* Parameter signature */
    "glMultTransposeMatrixd\0"
    "glMultTransposeMatrixdARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos2svMESA_names[] = 
    "p\0" /* Parameter signature */
    "glWindowPos2sv\0"
    "glWindowPos2svARB\0"
    "glWindowPos2svMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3) || defined(need_GL_ARB_texture_compression)
static const char CompressedTexImage1DARB_names[] = 
    "iiiiiip\0" /* Parameter signature */
    "glCompressedTexImage1D\0"
    "glCompressedTexImage1DARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib2sNV_names[] = 
    "iii\0" /* Parameter signature */
    "glVertexAttrib2sNV\0"
    "";
#endif

#if defined(need_GL_IBM_vertex_array_lists)
static const char NormalPointerListIBM_names[] = 
    "iipi\0" /* Parameter signature */
    "glNormalPointerListIBM\0"
    "";
#endif

#if defined(need_GL_EXT_vertex_array)
static const char IndexPointerEXT_names[] = 
    "iiip\0" /* Parameter signature */
    "glIndexPointerEXT\0"
    "";
#endif

#if defined(need_GL_EXT_vertex_array)
static const char NormalPointerEXT_names[] = 
    "iiip\0" /* Parameter signature */
    "glNormalPointerEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord3dARB_names[] = 
    "iddd\0" /* Parameter signature */
    "glMultiTexCoord3d\0"
    "glMultiTexCoord3dARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord2iARB_names[] = 
    "iii\0" /* Parameter signature */
    "glMultiTexCoord2i\0"
    "glMultiTexCoord2iARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN_names[] = 
    "iffffffff\0" /* Parameter signature */
    "glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord2svARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord2sv\0"
    "glMultiTexCoord2svARB\0"
    "";
#endif

#if defined(need_GL_SUN_triangle_list)
static const char ReplacementCodeubvSUN_names[] = 
    "p\0" /* Parameter signature */
    "glReplacementCodeubvSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform3iARB_names[] = 
    "iiii\0" /* Parameter signature */
    "glUniform3i\0"
    "glUniform3iARB\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char GetFragmentMaterialfvSGIX_names[] = 
    "iip\0" /* Parameter signature */
    "glGetFragmentMaterialfvSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char GetShaderInfoLog_names[] = 
    "iipp\0" /* Parameter signature */
    "glGetShaderInfoLog\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_blend)
static const char WeightivARB_names[] = 
    "ip\0" /* Parameter signature */
    "glWeightivARB\0"
    "";
#endif

#if defined(need_GL_SUN_global_alpha)
static const char GlobalAlphaFactordSUN_names[] = 
    "d\0" /* Parameter signature */
    "glGlobalAlphaFactordSUN\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs3fvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs3fvNV\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char GenerateMipmapEXT_names[] = 
    "i\0" /* Parameter signature */
    "glGenerateMipmapEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char ProgramParameter4dNV_names[] = 
    "iidddd\0" /* Parameter signature */
    "glProgramParameter4dNV\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char SetFragmentShaderConstantATI_names[] = 
    "ip\0" /* Parameter signature */
    "glSetFragmentShaderConstantATI\0"
    "";
#endif

#if defined(need_GL_NV_evaluators)
static const char GetMapAttribParameterivNV_names[] = 
    "iiip\0" /* Parameter signature */
    "glGetMapAttribParameterivNV\0"
    "";
#endif

#if defined(need_GL_ARB_shader_objects)
static const char CreateShaderObjectARB_names[] = 
    "i\0" /* Parameter signature */
    "glCreateShaderObjectARB\0"
    "";
#endif

#if defined(need_GL_SGIS_sharpen_texture)
static const char GetSharpenTexFuncSGIS_names[] = 
    "ip\0" /* Parameter signature */
    "glGetSharpenTexFuncSGIS\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_vertex_buffer_object)
static const char BufferDataARB_names[] = 
    "iipi\0" /* Parameter signature */
    "glBufferData\0"
    "glBufferDataARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_array_range)
static const char FlushVertexArrayRangeNV_names[] = 
    "\0" /* Parameter signature */
    "glFlushVertexArrayRangeNV\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char SampleMapATI_names[] = 
    "iii\0" /* Parameter signature */
    "glSampleMapATI\0"
    "";
#endif

#if defined(need_GL_EXT_vertex_array)
static const char VertexPointerEXT_names[] = 
    "iiiip\0" /* Parameter signature */
    "glVertexPointerEXT\0"
    "";
#endif

#if defined(need_GL_SGIS_texture_filter4)
static const char GetTexFilterFuncSGIS_names[] = 
    "iip\0" /* Parameter signature */
    "glGetTexFilterFuncSGIS\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char GetCombinerOutputParameterfvNV_names[] = 
    "iiip\0" /* Parameter signature */
    "glGetCombinerOutputParameterfvNV\0"
    "";
#endif

#if defined(need_GL_EXT_subtexture)
static const char TexSubImage1D_names[] = 
    "iiiiiip\0" /* Parameter signature */
    "glTexSubImage1D\0"
    "glTexSubImage1DEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib1sARB_names[] = 
    "ii\0" /* Parameter signature */
    "glVertexAttrib1s\0"
    "glVertexAttrib1sARB\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char FinalCombinerInputNV_names[] = 
    "iiii\0" /* Parameter signature */
    "glFinalCombinerInputNV\0"
    "";
#endif

#if defined(need_GL_SGIX_flush_raster)
static const char FlushRasterSGIX_names[] = 
    "\0" /* Parameter signature */
    "glFlushRasterSGIX\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiTexCoord2fVertex3fSUN_names[] = 
    "ifffff\0" /* Parameter signature */
    "glReplacementCodeuiTexCoord2fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform1fARB_names[] = 
    "if\0" /* Parameter signature */
    "glUniform1f\0"
    "glUniform1fARB\0"
    "";
#endif

#if defined(need_GL_EXT_texture_object)
static const char AreTexturesResident_names[] = 
    "ipp\0" /* Parameter signature */
    "glAreTexturesResident\0"
    "glAreTexturesResidentEXT\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char IsRenderbufferEXT_names[] = 
    "i\0" /* Parameter signature */
    "glIsRenderbufferEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ATI_separate_stencil)
static const char StencilOpSeparate_names[] = 
    "iiii\0" /* Parameter signature */
    "glStencilOpSeparate\0"
    "glStencilOpSeparateATI\0"
    "";
#endif

#if defined(need_GL_SGI_color_table)
static const char ColorTableParameteriv_names[] = 
    "iip\0" /* Parameter signature */
    "glColorTableParameteriv\0"
    "glColorTableParameterivSGI\0"
    "";
#endif

#if defined(need_GL_IBM_vertex_array_lists)
static const char FogCoordPointerListIBM_names[] = 
    "iipi\0" /* Parameter signature */
    "glFogCoordPointerListIBM\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos3dMESA_names[] = 
    "ddd\0" /* Parameter signature */
    "glWindowPos3d\0"
    "glWindowPos3dARB\0"
    "glWindowPos3dMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_point_parameters) || defined(need_GL_EXT_point_parameters) || defined(need_GL_SGIS_point_parameters)
static const char PointParameterfvEXT_names[] = 
    "ip\0" /* Parameter signature */
    "glPointParameterfv\0"
    "glPointParameterfvARB\0"
    "glPointParameterfvEXT\0"
    "glPointParameterfvSGIS\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos2fvMESA_names[] = 
    "p\0" /* Parameter signature */
    "glWindowPos2fv\0"
    "glWindowPos2fvARB\0"
    "glWindowPos2fvMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3bvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glSecondaryColor3bv\0"
    "glSecondaryColor3bvEXT\0"
    "";
#endif

#if defined(need_GL_IBM_vertex_array_lists)
static const char VertexPointerListIBM_names[] = 
    "iiipi\0" /* Parameter signature */
    "glVertexPointerListIBM\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char GetProgramLocalParameterfvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetProgramLocalParameterfvARB\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentMaterialfSGIX_names[] = 
    "iif\0" /* Parameter signature */
    "glFragmentMaterialfSGIX\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord2fNormal3fVertex3fSUN_names[] = 
    "ffffffff\0" /* Parameter signature */
    "glTexCoord2fNormal3fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char RenderbufferStorageEXT_names[] = 
    "iiii\0" /* Parameter signature */
    "glRenderbufferStorageEXT\0"
    "";
#endif

#if defined(need_GL_NV_fence)
static const char IsFenceNV_names[] = 
    "i\0" /* Parameter signature */
    "glIsFenceNV\0"
    "";
#endif

#if defined(need_GL_ARB_shader_objects)
static const char AttachObjectARB_names[] = 
    "ii\0" /* Parameter signature */
    "glAttachObjectARB\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char GetFragmentLightivSGIX_names[] = 
    "iip\0" /* Parameter signature */
    "glGetFragmentLightivSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char UniformMatrix2fvARB_names[] = 
    "iiip\0" /* Parameter signature */
    "glUniformMatrix2fv\0"
    "glUniformMatrix2fvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord2fARB_names[] = 
    "iff\0" /* Parameter signature */
    "glMultiTexCoord2f\0"
    "glMultiTexCoord2fARB\0"
    "";
#endif

#if defined(need_GL_SGI_color_table) || defined(need_GL_EXT_paletted_texture)
static const char ColorTable_names[] = 
    "iiiiip\0" /* Parameter signature */
    "glColorTable\0"
    "glColorTableSGI\0"
    "glColorTableEXT\0"
    "";
#endif

#if defined(need_GL_NV_evaluators)
static const char MapControlPointsNV_names[] = 
    "iiiiiiiip\0" /* Parameter signature */
    "glMapControlPointsNV\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char ConvolutionFilter2D_names[] = 
    "iiiiiip\0" /* Parameter signature */
    "glConvolutionFilter2D\0"
    "glConvolutionFilter2DEXT\0"
    "";
#endif

#if defined(need_GL_NV_evaluators)
static const char MapParameterfvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glMapParameterfvNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib3dvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib3dv\0"
    "glVertexAttrib3dvARB\0"
    "";
#endif

#if defined(need_GL_PGI_misc_hints)
static const char HintPGI_names[] = 
    "ii\0" /* Parameter signature */
    "glHintPGI\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char ConvolutionParameteriv_names[] = 
    "iip\0" /* Parameter signature */
    "glConvolutionParameteriv\0"
    "glConvolutionParameterivEXT\0"
    "";
#endif

#if defined(need_GL_EXT_cull_vertex)
static const char CullParameterdvEXT_names[] = 
    "ip\0" /* Parameter signature */
    "glCullParameterdvEXT\0"
    "";
#endif

#if defined(need_GL_NV_fragment_program)
static const char ProgramNamedParameter4fNV_names[] = 
    "iipffff\0" /* Parameter signature */
    "glProgramNamedParameter4fNV\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char Color3fVertex3fSUN_names[] = 
    "ffffff\0" /* Parameter signature */
    "glColor3fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char ProgramEnvParameter4fvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glProgramEnvParameter4fvARB\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentLightModeliSGIX_names[] = 
    "ii\0" /* Parameter signature */
    "glFragmentLightModeliSGIX\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char ConvolutionParameterfv_names[] = 
    "iip\0" /* Parameter signature */
    "glConvolutionParameterfv\0"
    "glConvolutionParameterfvEXT\0"
    "";
#endif

#if defined(need_GL_3DFX_tbuffer)
static const char TbufferMask3DFX_names[] = 
    "i\0" /* Parameter signature */
    "glTbufferMask3DFX\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char LoadProgramNV_names[] = 
    "iiip\0" /* Parameter signature */
    "glLoadProgramNV\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib4fvNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4fvNV\0"
    "";
#endif

#if defined(need_GL_ARB_shader_objects)
static const char GetAttachedObjectsARB_names[] = 
    "iipp\0" /* Parameter signature */
    "glGetAttachedObjectsARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform3fvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glUniform3fv\0"
    "glUniform3fvARB\0"
    "";
#endif

#if defined(need_GL_EXT_draw_range_elements)
static const char DrawRangeElements_names[] = 
    "iiiiip\0" /* Parameter signature */
    "glDrawRangeElements\0"
    "glDrawRangeElementsEXT\0"
    "";
#endif

#if defined(need_GL_SGIX_sprite)
static const char SpriteParameterfvSGIX_names[] = 
    "ip\0" /* Parameter signature */
    "glSpriteParameterfvSGIX\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char CheckFramebufferStatusEXT_names[] = 
    "i\0" /* Parameter signature */
    "glCheckFramebufferStatusEXT\0"
    "";
#endif

#if defined(need_GL_SUN_global_alpha)
static const char GlobalAlphaFactoruiSUN_names[] = 
    "i\0" /* Parameter signature */
    "glGlobalAlphaFactoruiSUN\0"
    "";
#endif

#if defined(need_GL_ARB_shader_objects)
static const char GetHandleARB_names[] = 
    "i\0" /* Parameter signature */
    "glGetHandleARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char GetVertexAttribivARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetVertexAttribiv\0"
    "glGetVertexAttribivARB\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char GetCombinerInputParameterfvNV_names[] = 
    "iiiip\0" /* Parameter signature */
    "glGetCombinerInputParameterfvNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char CreateProgram_names[] = 
    "\0" /* Parameter signature */
    "glCreateProgram\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3) || defined(need_GL_ARB_transpose_matrix)
static const char LoadTransposeMatrixdARB_names[] = 
    "p\0" /* Parameter signature */
    "glLoadTransposeMatrixd\0"
    "glLoadTransposeMatrixdARB\0"
    "";
#endif

#if defined(need_GL_EXT_histogram)
static const char GetMinmax_names[] = 
    "iiiip\0" /* Parameter signature */
    "glGetMinmax\0"
    "glGetMinmaxEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char StencilFuncSeparate_names[] = 
    "iiii\0" /* Parameter signature */
    "glStencilFuncSeparate\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3sEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glSecondaryColor3s\0"
    "glSecondaryColor3sEXT\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char Color3fVertex3fvSUN_names[] = 
    "pp\0" /* Parameter signature */
    "glColor3fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_SUN_global_alpha)
static const char GlobalAlphaFactorbSUN_names[] = 
    "i\0" /* Parameter signature */
    "glGlobalAlphaFactorbSUN\0"
    "";
#endif

#if defined(need_GL_HP_image_transform)
static const char ImageTransformParameterfvHP_names[] = 
    "iip\0" /* Parameter signature */
    "glImageTransformParameterfvHP\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4ivARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4iv\0"
    "glVertexAttrib4ivARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib3fNV_names[] = 
    "ifff\0" /* Parameter signature */
    "glVertexAttrib3fNV\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs2dvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs2dvNV\0"
    "";
#endif

#if defined(need_GL_EXT_timer_query)
static const char GetQueryObjectui64vEXT_names[] = 
    "iip\0" /* Parameter signature */
    "glGetQueryObjectui64vEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord3fvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord3fv\0"
    "glMultiTexCoord3fvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3dEXT_names[] = 
    "ddd\0" /* Parameter signature */
    "glSecondaryColor3d\0"
    "glSecondaryColor3dEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char GetProgramParameterfvNV_names[] = 
    "iiip\0" /* Parameter signature */
    "glGetProgramParameterfvNV\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char TangentPointerEXT_names[] = 
    "iip\0" /* Parameter signature */
    "glTangentPointerEXT\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char Color4fNormal3fVertex3fvSUN_names[] = 
    "ppp\0" /* Parameter signature */
    "glColor4fNormal3fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_SGIX_instruments)
static const char GetInstrumentsSGIX_names[] = 
    "\0" /* Parameter signature */
    "glGetInstrumentsSGIX\0"
    "";
#endif

#if defined(need_GL_NV_evaluators)
static const char EvalMapsNV_names[] = 
    "ii\0" /* Parameter signature */
    "glEvalMapsNV\0"
    "";
#endif

#if defined(need_GL_EXT_subtexture)
static const char TexSubImage2D_names[] = 
    "iiiiiiiip\0" /* Parameter signature */
    "glTexSubImage2D\0"
    "glTexSubImage2DEXT\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentLightivSGIX_names[] = 
    "iip\0" /* Parameter signature */
    "glFragmentLightivSGIX\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char DeleteRenderbuffersEXT_names[] = 
    "ip\0" /* Parameter signature */
    "glDeleteRenderbuffersEXT\0"
    "";
#endif

#if defined(need_GL_EXT_pixel_transform)
static const char PixelTransformParameterfvEXT_names[] = 
    "iip\0" /* Parameter signature */
    "glPixelTransformParameterfvEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4bvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4bv\0"
    "glVertexAttrib4bvARB\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char AlphaFragmentOp2ATI_names[] = 
    "iiiiiiiii\0" /* Parameter signature */
    "glAlphaFragmentOp2ATI\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord4sARB_names[] = 
    "iiiii\0" /* Parameter signature */
    "glMultiTexCoord4s\0"
    "glMultiTexCoord4sARB\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char GetFragmentMaterialivSGIX_names[] = 
    "iip\0" /* Parameter signature */
    "glGetFragmentMaterialivSGIX\0"
    "";
#endif

#if defined(need_GL_MESA_window_pos)
static const char WindowPos4dMESA_names[] = 
    "dddd\0" /* Parameter signature */
    "glWindowPos4dMESA\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_blend)
static const char WeightPointerARB_names[] = 
    "iiip\0" /* Parameter signature */
    "glWeightPointerARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos2dMESA_names[] = 
    "dd\0" /* Parameter signature */
    "glWindowPos2d\0"
    "glWindowPos2dARB\0"
    "glWindowPos2dMESA\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char FramebufferTexture3DEXT_names[] = 
    "iiiiii\0" /* Parameter signature */
    "glFramebufferTexture3DEXT\0"
    "";
#endif

#if defined(need_GL_EXT_blend_minmax)
static const char BlendEquation_names[] = 
    "i\0" /* Parameter signature */
    "glBlendEquation\0"
    "glBlendEquationEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib3dNV_names[] = 
    "iddd\0" /* Parameter signature */
    "glVertexAttrib3dNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib3dARB_names[] = 
    "iddd\0" /* Parameter signature */
    "glVertexAttrib3d\0"
    "glVertexAttrib3dARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN_names[] = 
    "ppppp\0" /* Parameter signature */
    "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4fARB_names[] = 
    "iffff\0" /* Parameter signature */
    "glVertexAttrib4f\0"
    "glVertexAttrib4fARB\0"
    "";
#endif

#if defined(need_GL_EXT_index_func)
static const char IndexFuncEXT_names[] = 
    "if\0" /* Parameter signature */
    "glIndexFuncEXT\0"
    "";
#endif

#if defined(need_GL_SGIX_list_priority)
static const char GetListParameterfvSGIX_names[] = 
    "iip\0" /* Parameter signature */
    "glGetListParameterfvSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord2dvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord2dv\0"
    "glMultiTexCoord2dvARB\0"
    "";
#endif

#if defined(need_GL_EXT_cull_vertex)
static const char CullParameterfvEXT_names[] = 
    "ip\0" /* Parameter signature */
    "glCullParameterfvEXT\0"
    "";
#endif

#if defined(need_GL_NV_fragment_program)
static const char ProgramNamedParameter4fvNV_names[] = 
    "iipp\0" /* Parameter signature */
    "glProgramNamedParameter4fvNV\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColorPointerEXT_names[] = 
    "iiip\0" /* Parameter signature */
    "glSecondaryColorPointer\0"
    "glSecondaryColorPointerEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4fvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4fv\0"
    "glVertexAttrib4fvARB\0"
    "";
#endif

#if defined(need_GL_IBM_vertex_array_lists)
static const char ColorPointerListIBM_names[] = 
    "iiipi\0" /* Parameter signature */
    "glColorPointerListIBM\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char GetActiveUniformARB_names[] = 
    "iiipppp\0" /* Parameter signature */
    "glGetActiveUniform\0"
    "glGetActiveUniformARB\0"
    "";
#endif

#if defined(need_GL_HP_image_transform)
static const char ImageTransformParameteriHP_names[] = 
    "iii\0" /* Parameter signature */
    "glImageTransformParameteriHP\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord1svARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord1sv\0"
    "glMultiTexCoord1svARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_occlusion_query)
static const char EndQueryARB_names[] = 
    "i\0" /* Parameter signature */
    "glEndQuery\0"
    "glEndQueryARB\0"
    "";
#endif

#if defined(need_GL_NV_fence)
static const char DeleteFencesNV_names[] = 
    "ip\0" /* Parameter signature */
    "glDeleteFencesNV\0"
    "";
#endif

#if defined(need_GL_SGIX_polynomial_ffd)
static const char DeformationMap3dSGIX_names[] = 
    "iddiiddiiddiip\0" /* Parameter signature */
    "glDeformationMap3dSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char IsShader_names[] = 
    "i\0" /* Parameter signature */
    "glIsShader\0"
    "";
#endif

#if defined(need_GL_HP_image_transform)
static const char GetImageTransformParameterivHP_names[] = 
    "iip\0" /* Parameter signature */
    "glGetImageTransformParameterivHP\0"
    "";
#endif

#if defined(need_GL_MESA_window_pos)
static const char WindowPos4ivMESA_names[] = 
    "p\0" /* Parameter signature */
    "glWindowPos4ivMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord3svARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord3sv\0"
    "glMultiTexCoord3svARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord4iARB_names[] = 
    "iiiii\0" /* Parameter signature */
    "glMultiTexCoord4i\0"
    "glMultiTexCoord4iARB\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Binormal3ivEXT_names[] = 
    "p\0" /* Parameter signature */
    "glBinormal3ivEXT\0"
    "";
#endif

#if defined(need_GL_MESA_resize_buffers)
static const char ResizeBuffersMESA_names[] = 
    "\0" /* Parameter signature */
    "glResizeBuffersMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char GetUniformivARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetUniformiv\0"
    "glGetUniformivARB\0"
    "";
#endif

#if defined(need_GL_SGIS_pixel_texture)
static const char PixelTexGenParameteriSGIS_names[] = 
    "ii\0" /* Parameter signature */
    "glPixelTexGenParameteriSGIS\0"
    "";
#endif

#if defined(need_GL_INTEL_parallel_arrays)
static const char VertexPointervINTEL_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexPointervINTEL\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiColor4fNormal3fVertex3fvSUN_names[] = 
    "pppp\0" /* Parameter signature */
    "glReplacementCodeuiColor4fNormal3fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3uiEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glSecondaryColor3ui\0"
    "glSecondaryColor3uiEXT\0"
    "";
#endif

#if defined(need_GL_SGIX_instruments)
static const char StartInstrumentsSGIX_names[] = 
    "\0" /* Parameter signature */
    "glStartInstrumentsSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3usvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glSecondaryColor3usv\0"
    "glSecondaryColor3usvEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib2fvNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib2fvNV\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char ProgramLocalParameter4dvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glProgramLocalParameter4dvARB\0"
    "";
#endif

#if defined(need_GL_ARB_matrix_palette)
static const char MatrixIndexuivARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMatrixIndexuivARB\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Tangent3sEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glTangent3sEXT\0"
    "";
#endif

#if defined(need_GL_SUN_global_alpha)
static const char GlobalAlphaFactorfSUN_names[] = 
    "f\0" /* Parameter signature */
    "glGlobalAlphaFactorfSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord3iARB_names[] = 
    "iiii\0" /* Parameter signature */
    "glMultiTexCoord3i\0"
    "glMultiTexCoord3iARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char IsProgram_names[] = 
    "i\0" /* Parameter signature */
    "glIsProgram\0"
    "";
#endif

#if defined(need_GL_IBM_vertex_array_lists)
static const char TexCoordPointerListIBM_names[] = 
    "iiipi\0" /* Parameter signature */
    "glTexCoordPointerListIBM\0"
    "";
#endif

#if defined(need_GL_SUN_global_alpha)
static const char GlobalAlphaFactorusSUN_names[] = 
    "i\0" /* Parameter signature */
    "glGlobalAlphaFactorusSUN\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib2dvNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib2dvNV\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char FramebufferRenderbufferEXT_names[] = 
    "iiii\0" /* Parameter signature */
    "glFramebufferRenderbufferEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib1dvNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib1dvNV\0"
    "";
#endif

#if defined(need_GL_EXT_texture_object)
static const char GenTextures_names[] = 
    "ip\0" /* Parameter signature */
    "glGenTextures\0"
    "glGenTexturesEXT\0"
    "";
#endif

#if defined(need_GL_NV_fence)
static const char SetFenceNV_names[] = 
    "ii\0" /* Parameter signature */
    "glSetFenceNV\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char FramebufferTexture1DEXT_names[] = 
    "iiiii\0" /* Parameter signature */
    "glFramebufferTexture1DEXT\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char GetCombinerOutputParameterivNV_names[] = 
    "iiip\0" /* Parameter signature */
    "glGetCombinerOutputParameterivNV\0"
    "";
#endif

#if defined(need_GL_SGIS_pixel_texture)
static const char PixelTexGenParameterivSGIS_names[] = 
    "ip\0" /* Parameter signature */
    "glPixelTexGenParameterivSGIS\0"
    "";
#endif

#if defined(need_GL_EXT_texture_perturb_normal)
static const char TextureNormalEXT_names[] = 
    "i\0" /* Parameter signature */
    "glTextureNormalEXT\0"
    "";
#endif

#if defined(need_GL_IBM_vertex_array_lists)
static const char IndexPointerListIBM_names[] = 
    "iipi\0" /* Parameter signature */
    "glIndexPointerListIBM\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_blend)
static const char WeightfvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glWeightfvARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char ProgramParameter4fvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glProgramParameter4fvNV\0"
    "";
#endif

#if defined(need_GL_MESA_window_pos)
static const char WindowPos4fMESA_names[] = 
    "ffff\0" /* Parameter signature */
    "glWindowPos4fMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos3dvMESA_names[] = 
    "p\0" /* Parameter signature */
    "glWindowPos3dv\0"
    "glWindowPos3dvARB\0"
    "glWindowPos3dvMESA\0"
    "";
#endif

#if defined(need_GL_EXT_timer_query)
static const char GetQueryObjecti64vEXT_names[] = 
    "iip\0" /* Parameter signature */
    "glGetQueryObjecti64vEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord1dARB_names[] = 
    "id\0" /* Parameter signature */
    "glMultiTexCoord1d\0"
    "glMultiTexCoord1dARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_NV_point_sprite)
static const char PointParameterivNV_names[] = 
    "ip\0" /* Parameter signature */
    "glPointParameteriv\0"
    "glPointParameterivNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform2fvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glUniform2fv\0"
    "glUniform2fvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord3dvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord3dv\0"
    "glMultiTexCoord3dvARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN_names[] = 
    "pppp\0" /* Parameter signature */
    "glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_ARB_shader_objects)
static const char DeleteObjectARB_names[] = 
    "i\0" /* Parameter signature */
    "glDeleteObjectARB\0"
    "";
#endif

#if defined(need_GL_ARB_matrix_palette)
static const char MatrixIndexPointerARB_names[] = 
    "iiip\0" /* Parameter signature */
    "glMatrixIndexPointerARB\0"
    "";
#endif

#if defined(need_GL_NV_fragment_program)
static const char ProgramNamedParameter4dvNV_names[] = 
    "iipp\0" /* Parameter signature */
    "glProgramNamedParameter4dvNV\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Tangent3fvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glTangent3fvEXT\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char BindFramebufferEXT_names[] = 
    "ii\0" /* Parameter signature */
    "glBindFramebufferEXT\0"
    "";
#endif

#if defined(need_GL_SGIX_reference_plane)
static const char ReferencePlaneSGIX_names[] = 
    "p\0" /* Parameter signature */
    "glReferencePlaneSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char ValidateProgramARB_names[] = 
    "i\0" /* Parameter signature */
    "glValidateProgram\0"
    "glValidateProgramARB\0"
    "";
#endif

#if defined(need_GL_EXT_compiled_vertex_array)
static const char UnlockArraysEXT_names[] = 
    "\0" /* Parameter signature */
    "glUnlockArraysEXT\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord2fColor3fVertex3fSUN_names[] = 
    "ffffffff\0" /* Parameter signature */
    "glTexCoord2fColor3fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos3fvMESA_names[] = 
    "p\0" /* Parameter signature */
    "glWindowPos3fv\0"
    "glWindowPos3fvARB\0"
    "glWindowPos3fvMESA\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib1svNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib1svNV\0"
    "";
#endif

#if defined(need_GL_EXT_copy_texture)
static const char CopyTexSubImage3D_names[] = 
    "iiiiiiiii\0" /* Parameter signature */
    "glCopyTexSubImage3D\0"
    "glCopyTexSubImage3DEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib2dARB_names[] = 
    "idd\0" /* Parameter signature */
    "glVertexAttrib2d\0"
    "glVertexAttrib2dARB\0"
    "";
#endif

#if defined(need_GL_SGIS_texture_color_mask)
static const char TextureColorMaskSGIS_names[] = 
    "iiii\0" /* Parameter signature */
    "glTextureColorMaskSGIS\0"
    "";
#endif

#if defined(need_GL_SGI_color_table) || defined(need_GL_EXT_paletted_texture)
static const char GetColorTable_names[] = 
    "iiip\0" /* Parameter signature */
    "glGetColorTable\0"
    "glGetColorTableSGI\0"
    "glGetColorTableEXT\0"
    "";
#endif

#if defined(need_GL_SGI_color_table)
static const char CopyColorTable_names[] = 
    "iiiii\0" /* Parameter signature */
    "glCopyColorTable\0"
    "glCopyColorTableSGI\0"
    "";
#endif

#if defined(need_GL_EXT_histogram)
static const char GetHistogramParameterfv_names[] = 
    "iip\0" /* Parameter signature */
    "glGetHistogramParameterfv\0"
    "glGetHistogramParameterfvEXT\0"
    "";
#endif

#if defined(need_GL_INTEL_parallel_arrays)
static const char ColorPointervINTEL_names[] = 
    "iip\0" /* Parameter signature */
    "glColorPointervINTEL\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char AlphaFragmentOp1ATI_names[] = 
    "iiiiii\0" /* Parameter signature */
    "glAlphaFragmentOp1ATI\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord3ivARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord3iv\0"
    "glMultiTexCoord3ivARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord2sARB_names[] = 
    "iii\0" /* Parameter signature */
    "glMultiTexCoord2s\0"
    "glMultiTexCoord2sARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib1dvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib1dv\0"
    "glVertexAttrib1dvARB\0"
    "";
#endif

#if defined(need_GL_EXT_texture_object)
static const char DeleteTextures_names[] = 
    "ip\0" /* Parameter signature */
    "glDeleteTextures\0"
    "glDeleteTexturesEXT\0"
    "";
#endif

#if defined(need_GL_EXT_vertex_array)
static const char TexCoordPointerEXT_names[] = 
    "iiiip\0" /* Parameter signature */
    "glTexCoordPointerEXT\0"
    "";
#endif

#if defined(need_GL_SGIS_texture4D)
static const char TexSubImage4DSGIS_names[] = 
    "iiiiiiiiiiiip\0" /* Parameter signature */
    "glTexSubImage4DSGIS\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners2)
static const char CombinerStageParameterfvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glCombinerStageParameterfvNV\0"
    "";
#endif

#if defined(need_GL_SGIX_instruments)
static const char StopInstrumentsSGIX_names[] = 
    "i\0" /* Parameter signature */
    "glStopInstrumentsSGIX\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord4fColor4fNormal3fVertex4fSUN_names[] = 
    "fffffffffffffff\0" /* Parameter signature */
    "glTexCoord4fColor4fNormal3fVertex4fSUN\0"
    "";
#endif

#if defined(need_GL_SGIX_polynomial_ffd)
static const char DeformSGIX_names[] = 
    "i\0" /* Parameter signature */
    "glDeformSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char GetVertexAttribfvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetVertexAttribfv\0"
    "glGetVertexAttribfvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3ivEXT_names[] = 
    "p\0" /* Parameter signature */
    "glSecondaryColor3iv\0"
    "glSecondaryColor3ivEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_1)
static const char UniformMatrix4x2fv_names[] = 
    "iiip\0" /* Parameter signature */
    "glUniformMatrix4x2fv\0"
    "";
#endif

#if defined(need_GL_SGIS_detail_texture)
static const char GetDetailTexFuncSGIS_names[] = 
    "ip\0" /* Parameter signature */
    "glGetDetailTexFuncSGIS\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners2)
static const char GetCombinerStageParameterfvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glGetCombinerStageParameterfvNV\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Binormal3fEXT_names[] = 
    "fff\0" /* Parameter signature */
    "glBinormal3fEXT\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char Color4ubVertex2fvSUN_names[] = 
    "pp\0" /* Parameter signature */
    "glColor4ubVertex2fvSUN\0"
    "";
#endif

#if defined(need_GL_SGIS_texture_filter4)
static const char TexFilterFuncSGIS_names[] = 
    "iiip\0" /* Parameter signature */
    "glTexFilterFuncSGIS\0"
    "";
#endif

#if defined(need_GL_SGIS_multisample) || defined(need_GL_EXT_multisample)
static const char SampleMaskSGIS_names[] = 
    "fi\0" /* Parameter signature */
    "glSampleMaskSGIS\0"
    "glSampleMaskEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_shader)
static const char GetAttribLocationARB_names[] = 
    "ip\0" /* Parameter signature */
    "glGetAttribLocation\0"
    "glGetAttribLocationARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4ubvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4ubv\0"
    "glVertexAttrib4ubvARB\0"
    "";
#endif

#if defined(need_GL_SGIS_detail_texture)
static const char DetailTexFuncSGIS_names[] = 
    "iip\0" /* Parameter signature */
    "glDetailTexFuncSGIS\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char Normal3fVertex3fSUN_names[] = 
    "ffffff\0" /* Parameter signature */
    "glNormal3fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_EXT_copy_texture)
static const char CopyTexImage2D_names[] = 
    "iiiiiiii\0" /* Parameter signature */
    "glCopyTexImage2D\0"
    "glCopyTexImage2DEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_vertex_buffer_object)
static const char GetBufferPointervARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetBufferPointerv\0"
    "glGetBufferPointervARB\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char ProgramEnvParameter4fARB_names[] = 
    "iiffff\0" /* Parameter signature */
    "glProgramEnvParameter4fARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform3ivARB_names[] = 
    "iip\0" /* Parameter signature */
    "glUniform3iv\0"
    "glUniform3ivARB\0"
    "";
#endif

#if defined(need_GL_NV_fence)
static const char GetFenceivNV_names[] = 
    "iip\0" /* Parameter signature */
    "glGetFenceivNV\0"
    "";
#endif

#if defined(need_GL_MESA_window_pos)
static const char WindowPos4dvMESA_names[] = 
    "p\0" /* Parameter signature */
    "glWindowPos4dvMESA\0"
    "";
#endif

#if defined(need_GL_EXT_color_subtable)
static const char ColorSubTable_names[] = 
    "iiiiip\0" /* Parameter signature */
    "glColorSubTable\0"
    "glColorSubTableEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord4ivARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord4iv\0"
    "glMultiTexCoord4ivARB\0"
    "";
#endif

#if defined(need_GL_EXT_gpu_program_parameters)
static const char ProgramLocalParameters4fvEXT_names[] = 
    "iiip\0" /* Parameter signature */
    "glProgramLocalParameters4fvEXT\0"
    "";
#endif

#if defined(need_GL_NV_evaluators)
static const char GetMapAttribParameterfvNV_names[] = 
    "iiip\0" /* Parameter signature */
    "glGetMapAttribParameterfvNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4sARB_names[] = 
    "iiiii\0" /* Parameter signature */
    "glVertexAttrib4s\0"
    "glVertexAttrib4sARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_occlusion_query)
static const char GetQueryObjectuivARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetQueryObjectuiv\0"
    "glGetQueryObjectuivARB\0"
    "";
#endif

#if defined(need_GL_NV_evaluators)
static const char MapParameterivNV_names[] = 
    "iip\0" /* Parameter signature */
    "glMapParameterivNV\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char GenRenderbuffersEXT_names[] = 
    "ip\0" /* Parameter signature */
    "glGenRenderbuffersEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib2dvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib2dv\0"
    "glVertexAttrib2dvARB\0"
    "";
#endif

#if defined(need_GL_EXT_vertex_array)
static const char EdgeFlagPointerEXT_names[] = 
    "iip\0" /* Parameter signature */
    "glEdgeFlagPointerEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs2svNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs2svNV\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_blend)
static const char WeightbvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glWeightbvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib2fvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib2fv\0"
    "glVertexAttrib2fvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_vertex_buffer_object)
static const char GetBufferParameterivARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetBufferParameteriv\0"
    "glGetBufferParameterivARB\0"
    "";
#endif

#if defined(need_GL_SGIX_list_priority)
static const char ListParameteriSGIX_names[] = 
    "iii\0" /* Parameter signature */
    "glListParameteriSGIX\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiColor4fNormal3fVertex3fSUN_names[] = 
    "iffffffffff\0" /* Parameter signature */
    "glReplacementCodeuiColor4fNormal3fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_SGIX_instruments)
static const char InstrumentsBufferSGIX_names[] = 
    "ip\0" /* Parameter signature */
    "glInstrumentsBufferSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4NivARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4Niv\0"
    "glVertexAttrib4NivARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char GetAttachedShaders_names[] = 
    "iipp\0" /* Parameter signature */
    "glGetAttachedShaders\0"
    "";
#endif

#if defined(need_GL_APPLE_vertex_array_object)
static const char GenVertexArraysAPPLE_names[] = 
    "ip\0" /* Parameter signature */
    "glGenVertexArraysAPPLE\0"
    "";
#endif

#if defined(need_GL_EXT_gpu_program_parameters)
static const char ProgramEnvParameters4fvEXT_names[] = 
    "iiip\0" /* Parameter signature */
    "glProgramEnvParameters4fvEXT\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord2fColor4fNormal3fVertex3fvSUN_names[] = 
    "pppp\0" /* Parameter signature */
    "glTexCoord2fColor4fNormal3fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos2iMESA_names[] = 
    "ii\0" /* Parameter signature */
    "glWindowPos2i\0"
    "glWindowPos2iARB\0"
    "glWindowPos2iMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3fvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glSecondaryColor3fv\0"
    "glSecondaryColor3fvEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3) || defined(need_GL_ARB_texture_compression)
static const char CompressedTexSubImage1DARB_names[] = 
    "iiiiiip\0" /* Parameter signature */
    "glCompressedTexSubImage1D\0"
    "glCompressedTexSubImage1DARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char GetVertexAttribivNV_names[] = 
    "iip\0" /* Parameter signature */
    "glGetVertexAttribivNV\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char GetProgramStringARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetProgramStringARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char CompileShaderARB_names[] = 
    "i\0" /* Parameter signature */
    "glCompileShader\0"
    "glCompileShaderARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char DeleteShader_names[] = 
    "i\0" /* Parameter signature */
    "glDeleteShader\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform3fARB_names[] = 
    "ifff\0" /* Parameter signature */
    "glUniform3f\0"
    "glUniform3fARB\0"
    "";
#endif

#if defined(need_GL_SGIX_list_priority)
static const char ListParameterfvSGIX_names[] = 
    "iip\0" /* Parameter signature */
    "glListParameterfvSGIX\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Tangent3dvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glTangent3dvEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char GetVertexAttribfvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glGetVertexAttribfvNV\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos3sMESA_names[] = 
    "iii\0" /* Parameter signature */
    "glWindowPos3s\0"
    "glWindowPos3sARB\0"
    "glWindowPos3sMESA\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib2svNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib2svNV\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs1fvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs1fvNV\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord2fVertex3fvSUN_names[] = 
    "pp\0" /* Parameter signature */
    "glTexCoord2fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_MESA_window_pos)
static const char WindowPos4sMESA_names[] = 
    "iiii\0" /* Parameter signature */
    "glWindowPos4sMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4NuivARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4Nuiv\0"
    "glVertexAttrib4NuivARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char ClientActiveTextureARB_names[] = 
    "i\0" /* Parameter signature */
    "glClientActiveTexture\0"
    "glClientActiveTextureARB\0"
    "";
#endif

#if defined(need_GL_SGIX_pixel_texture)
static const char PixelTexGenSGIX_names[] = 
    "i\0" /* Parameter signature */
    "glPixelTexGenSGIX\0"
    "";
#endif

#if defined(need_GL_SUN_triangle_list)
static const char ReplacementCodeusvSUN_names[] = 
    "p\0" /* Parameter signature */
    "glReplacementCodeusvSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform4fARB_names[] = 
    "iffff\0" /* Parameter signature */
    "glUniform4f\0"
    "glUniform4fARB\0"
    "";
#endif

#if defined(need_GL_IBM_multimode_draw_arrays)
static const char MultiModeDrawArraysIBM_names[] = 
    "pppii\0" /* Parameter signature */
    "glMultiModeDrawArraysIBM\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program) || defined(need_GL_NV_vertex_program)
static const char IsProgramNV_names[] = 
    "i\0" /* Parameter signature */
    "glIsProgramARB\0"
    "glIsProgramNV\0"
    "";
#endif

#if defined(need_GL_SUN_triangle_list)
static const char ReplacementCodePointerSUN_names[] = 
    "iip\0" /* Parameter signature */
    "glReplacementCodePointerSUN\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char ProgramEnvParameter4dARB_names[] = 
    "iidddd\0" /* Parameter signature */
    "glProgramEnvParameter4dARB\0"
    "";
#endif

#if defined(need_GL_SGI_color_table)
static const char ColorTableParameterfv_names[] = 
    "iip\0" /* Parameter signature */
    "glColorTableParameterfv\0"
    "glColorTableParameterfvSGI\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentLightModelfSGIX_names[] = 
    "if\0" /* Parameter signature */
    "glFragmentLightModelfSGIX\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Binormal3bvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glBinormal3bvEXT\0"
    "";
#endif

#if defined(need_GL_EXT_texture_object)
static const char IsTexture_names[] = 
    "i\0" /* Parameter signature */
    "glIsTexture\0"
    "glIsTextureEXT\0"
    "";
#endif

#if defined(need_GL_EXT_vertex_weighting)
static const char VertexWeightfvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glVertexWeightfvEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib1dARB_names[] = 
    "id\0" /* Parameter signature */
    "glVertexAttrib1d\0"
    "glVertexAttrib1dARB\0"
    "";
#endif

#if defined(need_GL_HP_image_transform)
static const char ImageTransformParameterivHP_names[] = 
    "iip\0" /* Parameter signature */
    "glImageTransformParameterivHP\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_occlusion_query)
static const char DeleteQueriesARB_names[] = 
    "ip\0" /* Parameter signature */
    "glDeleteQueries\0"
    "glDeleteQueriesARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char Color4ubVertex2fSUN_names[] = 
    "iiiiff\0" /* Parameter signature */
    "glColor4ubVertex2fSUN\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentColorMaterialSGIX_names[] = 
    "ii\0" /* Parameter signature */
    "glFragmentColorMaterialSGIX\0"
    "";
#endif

#if defined(need_GL_ARB_matrix_palette)
static const char CurrentPaletteMatrixARB_names[] = 
    "i\0" /* Parameter signature */
    "glCurrentPaletteMatrixARB\0"
    "";
#endif

#if defined(need_GL_SGIS_multisample) || defined(need_GL_EXT_multisample)
static const char SamplePatternSGIS_names[] = 
    "i\0" /* Parameter signature */
    "glSamplePatternSGIS\0"
    "glSamplePatternEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_occlusion_query)
static const char IsQueryARB_names[] = 
    "i\0" /* Parameter signature */
    "glIsQuery\0"
    "glIsQueryARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiColor4ubVertex3fSUN_names[] = 
    "iiiiifff\0" /* Parameter signature */
    "glReplacementCodeuiColor4ubVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4usvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4usv\0"
    "glVertexAttrib4usvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char LinkProgramARB_names[] = 
    "i\0" /* Parameter signature */
    "glLinkProgram\0"
    "glLinkProgramARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib2fNV_names[] = 
    "iff\0" /* Parameter signature */
    "glVertexAttrib2fNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char ShaderSourceARB_names[] = 
    "iipp\0" /* Parameter signature */
    "glShaderSource\0"
    "glShaderSourceARB\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentMaterialiSGIX_names[] = 
    "iii\0" /* Parameter signature */
    "glFragmentMaterialiSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib3svARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib3sv\0"
    "glVertexAttrib3svARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3) || defined(need_GL_ARB_texture_compression)
static const char CompressedTexSubImage3DARB_names[] = 
    "iiiiiiiiiip\0" /* Parameter signature */
    "glCompressedTexSubImage3D\0"
    "glCompressedTexSubImage3DARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos2ivMESA_names[] = 
    "p\0" /* Parameter signature */
    "glWindowPos2iv\0"
    "glWindowPos2ivARB\0"
    "glWindowPos2ivMESA\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char IsFramebufferEXT_names[] = 
    "i\0" /* Parameter signature */
    "glIsFramebufferEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform4ivARB_names[] = 
    "iip\0" /* Parameter signature */
    "glUniform4iv\0"
    "glUniform4ivARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char GetVertexAttribdvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetVertexAttribdv\0"
    "glGetVertexAttribdvARB\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char GetSeparableFilter_names[] = 
    "iiippp\0" /* Parameter signature */
    "glGetSeparableFilter\0"
    "glGetSeparableFilterEXT\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Binormal3dEXT_names[] = 
    "ddd\0" /* Parameter signature */
    "glBinormal3dEXT\0"
    "";
#endif

#if defined(need_GL_SGIX_sprite)
static const char SpriteParameteriSGIX_names[] = 
    "ii\0" /* Parameter signature */
    "glSpriteParameteriSGIX\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char RequestResidentProgramsNV_names[] = 
    "ip\0" /* Parameter signature */
    "glRequestResidentProgramsNV\0"
    "";
#endif

#if defined(need_GL_SGIX_tag_sample_buffer)
static const char TagSampleBufferSGIX_names[] = 
    "\0" /* Parameter signature */
    "glTagSampleBufferSGIX\0"
    "";
#endif

#if defined(need_GL_SUN_triangle_list)
static const char ReplacementCodeusSUN_names[] = 
    "i\0" /* Parameter signature */
    "glReplacementCodeusSUN\0"
    "";
#endif

#if defined(need_GL_SGIX_list_priority)
static const char ListParameterivSGIX_names[] = 
    "iip\0" /* Parameter signature */
    "glListParameterivSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_multi_draw_arrays)
static const char MultiDrawElementsEXT_names[] = 
    "ipipi\0" /* Parameter signature */
    "glMultiDrawElements\0"
    "glMultiDrawElementsEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform1ivARB_names[] = 
    "iip\0" /* Parameter signature */
    "glUniform1iv\0"
    "glUniform1ivARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos2sMESA_names[] = 
    "ii\0" /* Parameter signature */
    "glWindowPos2s\0"
    "glWindowPos2sARB\0"
    "glWindowPos2sMESA\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_blend)
static const char WeightusvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glWeightusvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_fog_coord)
static const char FogCoordPointerEXT_names[] = 
    "iip\0" /* Parameter signature */
    "glFogCoordPointer\0"
    "glFogCoordPointerEXT\0"
    "";
#endif

#if defined(need_GL_EXT_index_material)
static const char IndexMaterialEXT_names[] = 
    "ii\0" /* Parameter signature */
    "glIndexMaterialEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3ubvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glSecondaryColor3ubv\0"
    "glSecondaryColor3ubvEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4dvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4dv\0"
    "glVertexAttrib4dvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_shader)
static const char BindAttribLocationARB_names[] = 
    "iip\0" /* Parameter signature */
    "glBindAttribLocation\0"
    "glBindAttribLocationARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord2dARB_names[] = 
    "idd\0" /* Parameter signature */
    "glMultiTexCoord2d\0"
    "glMultiTexCoord2dARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char ExecuteProgramNV_names[] = 
    "iip\0" /* Parameter signature */
    "glExecuteProgramNV\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char LightEnviSGIX_names[] = 
    "ii\0" /* Parameter signature */
    "glLightEnviSGIX\0"
    "";
#endif

#if defined(need_GL_SUN_triangle_list)
static const char ReplacementCodeuiSUN_names[] = 
    "i\0" /* Parameter signature */
    "glReplacementCodeuiSUN\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char FramebufferTexture2DEXT_names[] = 
    "iiiii\0" /* Parameter signature */
    "glFramebufferTexture2DEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribPointerNV_names[] = 
    "iiiip\0" /* Parameter signature */
    "glVertexAttribPointerNV\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char GetFramebufferAttachmentParameterivEXT_names[] = 
    "iiip\0" /* Parameter signature */
    "glGetFramebufferAttachmentParameterivEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord4dvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord4dv\0"
    "glMultiTexCoord4dvARB\0"
    "";
#endif

#if defined(need_GL_EXT_pixel_transform)
static const char PixelTransformParameteriEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glPixelTransformParameteriEXT\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord2fColor4ubVertex3fSUN_names[] = 
    "ffiiiifff\0" /* Parameter signature */
    "glTexCoord2fColor4ubVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform1iARB_names[] = 
    "ii\0" /* Parameter signature */
    "glUniform1i\0"
    "glUniform1iARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttribPointerARB_names[] = 
    "iiiiip\0" /* Parameter signature */
    "glVertexAttribPointer\0"
    "glVertexAttribPointerARB\0"
    "";
#endif

#if defined(need_GL_SGIS_sharpen_texture)
static const char SharpenTexFuncSGIS_names[] = 
    "iip\0" /* Parameter signature */
    "glSharpenTexFuncSGIS\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord4fvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord4fv\0"
    "glMultiTexCoord4fvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_1)
static const char UniformMatrix2x3fv_names[] = 
    "iiip\0" /* Parameter signature */
    "glUniformMatrix2x3fv\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char TrackMatrixNV_names[] = 
    "iiii\0" /* Parameter signature */
    "glTrackMatrixNV\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char CombinerParameteriNV_names[] = 
    "ii\0" /* Parameter signature */
    "glCombinerParameteriNV\0"
    "";
#endif

#if defined(need_GL_SGIX_async)
static const char DeleteAsyncMarkersSGIX_names[] = 
    "ii\0" /* Parameter signature */
    "glDeleteAsyncMarkersSGIX\0"
    "";
#endif

#if defined(need_GL_SGIX_async)
static const char IsAsyncMarkerSGIX_names[] = 
    "i\0" /* Parameter signature */
    "glIsAsyncMarkerSGIX\0"
    "";
#endif

#if defined(need_GL_SGIX_framezoom)
static const char FrameZoomSGIX_names[] = 
    "i\0" /* Parameter signature */
    "glFrameZoomSGIX\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char Normal3fVertex3fvSUN_names[] = 
    "pp\0" /* Parameter signature */
    "glNormal3fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4NsvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4Nsv\0"
    "glVertexAttrib4NsvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib3fvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib3fv\0"
    "glVertexAttrib3fvARB\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char DeleteFramebuffersEXT_names[] = 
    "ip\0" /* Parameter signature */
    "glDeleteFramebuffersEXT\0"
    "";
#endif

#if defined(need_GL_SUN_global_alpha)
static const char GlobalAlphaFactorsSUN_names[] = 
    "i\0" /* Parameter signature */
    "glGlobalAlphaFactorsSUN\0"
    "";
#endif

#if defined(need_GL_EXT_texture3D)
static const char TexSubImage3D_names[] = 
    "iiiiiiiiiip\0" /* Parameter signature */
    "glTexSubImage3D\0"
    "glTexSubImage3DEXT\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Tangent3fEXT_names[] = 
    "fff\0" /* Parameter signature */
    "glTangent3fEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3uivEXT_names[] = 
    "p\0" /* Parameter signature */
    "glSecondaryColor3uiv\0"
    "glSecondaryColor3uivEXT\0"
    "";
#endif

#if defined(need_GL_ARB_matrix_palette)
static const char MatrixIndexubvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMatrixIndexubvARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char Color4fNormal3fVertex3fSUN_names[] = 
    "ffffffffff\0" /* Parameter signature */
    "glColor4fNormal3fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_SGIS_pixel_texture)
static const char PixelTexGenParameterfSGIS_names[] = 
    "if\0" /* Parameter signature */
    "glPixelTexGenParameterfSGIS\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char CreateShader_names[] = 
    "i\0" /* Parameter signature */
    "glCreateShader\0"
    "";
#endif

#if defined(need_GL_SGI_color_table) || defined(need_GL_EXT_paletted_texture)
static const char GetColorTableParameterfv_names[] = 
    "iip\0" /* Parameter signature */
    "glGetColorTableParameterfv\0"
    "glGetColorTableParameterfvSGI\0"
    "glGetColorTableParameterfvEXT\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentLightModelfvSGIX_names[] = 
    "ip\0" /* Parameter signature */
    "glFragmentLightModelfvSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord3fARB_names[] = 
    "ifff\0" /* Parameter signature */
    "glMultiTexCoord3f\0"
    "glMultiTexCoord3fARB\0"
    "";
#endif

#if defined(need_GL_SGIS_pixel_texture)
static const char GetPixelTexGenParameterfvSGIS_names[] = 
    "ip\0" /* Parameter signature */
    "glGetPixelTexGenParameterfvSGIS\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char GenFramebuffersEXT_names[] = 
    "ip\0" /* Parameter signature */
    "glGenFramebuffersEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char GetProgramParameterdvNV_names[] = 
    "iiip\0" /* Parameter signature */
    "glGetProgramParameterdvNV\0"
    "";
#endif

#if defined(need_GL_EXT_pixel_transform)
static const char PixelTransformParameterfEXT_names[] = 
    "iif\0" /* Parameter signature */
    "glPixelTransformParameterfEXT\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentLightfvSGIX_names[] = 
    "iip\0" /* Parameter signature */
    "glFragmentLightfvSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char DetachShader_names[] = 
    "ii\0" /* Parameter signature */
    "glDetachShader\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4NubARB_names[] = 
    "iiiii\0" /* Parameter signature */
    "glVertexAttrib4Nub\0"
    "glVertexAttrib4NubARB\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char GetProgramEnvParameterfvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetProgramEnvParameterfvARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char GetTrackMatrixivNV_names[] = 
    "iiip\0" /* Parameter signature */
    "glGetTrackMatrixivNV\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib3svNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib3svNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform4fvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glUniform4fv\0"
    "glUniform4fvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3) || defined(need_GL_ARB_transpose_matrix)
static const char MultTransposeMatrixfARB_names[] = 
    "p\0" /* Parameter signature */
    "glMultTransposeMatrixf\0"
    "glMultTransposeMatrixfARB\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char ColorFragmentOp1ATI_names[] = 
    "iiiiiii\0" /* Parameter signature */
    "glColorFragmentOp1ATI\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char GetUniformfvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetUniformfv\0"
    "glGetUniformfvARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN_names[] = 
    "iffffffffffff\0" /* Parameter signature */
    "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_ARB_shader_objects)
static const char DetachObjectARB_names[] = 
    "ii\0" /* Parameter signature */
    "glDetachObjectARB\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_blend)
static const char VertexBlendARB_names[] = 
    "i\0" /* Parameter signature */
    "glVertexBlendARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos3iMESA_names[] = 
    "iii\0" /* Parameter signature */
    "glWindowPos3i\0"
    "glWindowPos3iARB\0"
    "glWindowPos3iMESA\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char SeparableFilter2D_names[] = 
    "iiiiiipp\0" /* Parameter signature */
    "glSeparableFilter2D\0"
    "glSeparableFilter2DEXT\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiColor4ubVertex3fvSUN_names[] = 
    "ppp\0" /* Parameter signature */
    "glReplacementCodeuiColor4ubVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3) || defined(need_GL_ARB_texture_compression)
static const char CompressedTexImage2DARB_names[] = 
    "iiiiiiip\0" /* Parameter signature */
    "glCompressedTexImage2D\0"
    "glCompressedTexImage2DARB\0"
    "";
#endif

#if defined(need_GL_EXT_vertex_array)
static const char ArrayElement_names[] = 
    "i\0" /* Parameter signature */
    "glArrayElement\0"
    "glArrayElementEXT\0"
    "";
#endif

#if defined(need_GL_EXT_depth_bounds_test)
static const char DepthBoundsEXT_names[] = 
    "dd\0" /* Parameter signature */
    "glDepthBoundsEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char ProgramParameters4fvNV_names[] = 
    "iiip\0" /* Parameter signature */
    "glProgramParameters4fvNV\0"
    "";
#endif

#if defined(need_GL_SGIX_polynomial_ffd)
static const char DeformationMap3fSGIX_names[] = 
    "iffiiffiiffiip\0" /* Parameter signature */
    "glDeformationMap3fSGIX\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char GetProgramivNV_names[] = 
    "iip\0" /* Parameter signature */
    "glGetProgramivNV\0"
    "";
#endif

#if defined(need_GL_EXT_histogram)
static const char GetMinmaxParameteriv_names[] = 
    "iip\0" /* Parameter signature */
    "glGetMinmaxParameteriv\0"
    "glGetMinmaxParameterivEXT\0"
    "";
#endif

#if defined(need_GL_EXT_copy_texture)
static const char CopyTexImage1D_names[] = 
    "iiiiiii\0" /* Parameter signature */
    "glCopyTexImage1D\0"
    "glCopyTexImage1DEXT\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char AlphaFragmentOp3ATI_names[] = 
    "iiiiiiiiiiii\0" /* Parameter signature */
    "glAlphaFragmentOp3ATI\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char GetVertexAttribdvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glGetVertexAttribdvNV\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib3fvNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib3fvNV\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char GetFinalCombinerInputParameterivNV_names[] = 
    "iip\0" /* Parameter signature */
    "glGetFinalCombinerInputParameterivNV\0"
    "";
#endif

#if defined(need_GL_NV_evaluators)
static const char GetMapParameterivNV_names[] = 
    "iip\0" /* Parameter signature */
    "glGetMapParameterivNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform4iARB_names[] = 
    "iiiii\0" /* Parameter signature */
    "glUniform4i\0"
    "glUniform4iARB\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char ConvolutionParameteri_names[] = 
    "iii\0" /* Parameter signature */
    "glConvolutionParameteri\0"
    "glConvolutionParameteriEXT\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Binormal3sEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glBinormal3sEXT\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char ConvolutionParameterf_names[] = 
    "iif\0" /* Parameter signature */
    "glConvolutionParameterf\0"
    "glConvolutionParameterfEXT\0"
    "";
#endif

#if defined(need_GL_SGI_color_table) || defined(need_GL_EXT_paletted_texture)
static const char GetColorTableParameteriv_names[] = 
    "iip\0" /* Parameter signature */
    "glGetColorTableParameteriv\0"
    "glGetColorTableParameterivSGI\0"
    "glGetColorTableParameterivEXT\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char ProgramEnvParameter4dvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glProgramEnvParameter4dvARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs2fvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs2fvNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char UseProgramObjectARB_names[] = 
    "i\0" /* Parameter signature */
    "glUseProgram\0"
    "glUseProgramObjectARB\0"
    "";
#endif

#if defined(need_GL_NV_evaluators)
static const char GetMapParameterfvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glGetMapParameterfvNV\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char PassTexCoordATI_names[] = 
    "iii\0" /* Parameter signature */
    "glPassTexCoordATI\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char DeleteProgram_names[] = 
    "i\0" /* Parameter signature */
    "glDeleteProgram\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Tangent3ivEXT_names[] = 
    "p\0" /* Parameter signature */
    "glTangent3ivEXT\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Tangent3dEXT_names[] = 
    "ddd\0" /* Parameter signature */
    "glTangent3dEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3dvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glSecondaryColor3dv\0"
    "glSecondaryColor3dvEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_multi_draw_arrays)
static const char MultiDrawArraysEXT_names[] = 
    "ippi\0" /* Parameter signature */
    "glMultiDrawArrays\0"
    "glMultiDrawArraysEXT\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char BindRenderbufferEXT_names[] = 
    "ii\0" /* Parameter signature */
    "glBindRenderbufferEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord4dARB_names[] = 
    "idddd\0" /* Parameter signature */
    "glMultiTexCoord4d\0"
    "glMultiTexCoord4dARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3usEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glSecondaryColor3us\0"
    "glSecondaryColor3usEXT\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char ProgramLocalParameter4fvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glProgramLocalParameter4fvARB\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program) || defined(need_GL_NV_vertex_program)
static const char DeleteProgramsNV_names[] = 
    "ip\0" /* Parameter signature */
    "glDeleteProgramsARB\0"
    "glDeleteProgramsNV\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord1sARB_names[] = 
    "ii\0" /* Parameter signature */
    "glMultiTexCoord1s\0"
    "glMultiTexCoord1sARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiColor3fVertex3fSUN_names[] = 
    "iffffff\0" /* Parameter signature */
    "glReplacementCodeuiColor3fVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program) || defined(need_GL_NV_vertex_program)
static const char GetVertexAttribPointervNV_names[] = 
    "iip\0" /* Parameter signature */
    "glGetVertexAttribPointerv\0"
    "glGetVertexAttribPointervARB\0"
    "glGetVertexAttribPointervNV\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord1dvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord1dv\0"
    "glMultiTexCoord1dvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform2iARB_names[] = 
    "iii\0" /* Parameter signature */
    "glUniform2i\0"
    "glUniform2iARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char GetProgramStringNV_names[] = 
    "iip\0" /* Parameter signature */
    "glGetProgramStringNV\0"
    "";
#endif

#if defined(need_GL_EXT_vertex_array)
static const char ColorPointerEXT_names[] = 
    "iiiip\0" /* Parameter signature */
    "glColorPointerEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_vertex_buffer_object)
static const char MapBufferARB_names[] = 
    "ii\0" /* Parameter signature */
    "glMapBuffer\0"
    "glMapBufferARB\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Binormal3svEXT_names[] = 
    "p\0" /* Parameter signature */
    "glBinormal3svEXT\0"
    "";
#endif

#if defined(need_GL_EXT_light_texture)
static const char ApplyTextureEXT_names[] = 
    "i\0" /* Parameter signature */
    "glApplyTextureEXT\0"
    "";
#endif

#if defined(need_GL_EXT_light_texture)
static const char TextureMaterialEXT_names[] = 
    "ii\0" /* Parameter signature */
    "glTextureMaterialEXT\0"
    "";
#endif

#if defined(need_GL_EXT_light_texture)
static const char TextureLightEXT_names[] = 
    "i\0" /* Parameter signature */
    "glTextureLightEXT\0"
    "";
#endif

#if defined(need_GL_EXT_histogram)
static const char ResetMinmax_names[] = 
    "i\0" /* Parameter signature */
    "glResetMinmax\0"
    "glResetMinmaxEXT\0"
    "";
#endif

#if defined(need_GL_SGIX_sprite)
static const char SpriteParameterfSGIX_names[] = 
    "if\0" /* Parameter signature */
    "glSpriteParameterfSGIX\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib4sNV_names[] = 
    "iiiii\0" /* Parameter signature */
    "glVertexAttrib4sNV\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char GetConvolutionParameterfv_names[] = 
    "iip\0" /* Parameter signature */
    "glGetConvolutionParameterfv\0"
    "glGetConvolutionParameterfvEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs4dvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs4dvNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4dARB_names[] = 
    "idddd\0" /* Parameter signature */
    "glVertexAttrib4d\0"
    "glVertexAttrib4dARB\0"
    "";
#endif

#if defined(need_GL_NV_fragment_program)
static const char ProgramNamedParameter4dNV_names[] = 
    "iipdddd\0" /* Parameter signature */
    "glProgramNamedParameter4dNV\0"
    "";
#endif

#if defined(need_GL_EXT_vertex_weighting)
static const char VertexWeightfEXT_names[] = 
    "f\0" /* Parameter signature */
    "glVertexWeightfEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_fog_coord)
static const char FogCoordfvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glFogCoordfv\0"
    "glFogCoordfvEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord1ivARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord1iv\0"
    "glMultiTexCoord1ivARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3ubEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glSecondaryColor3ub\0"
    "glSecondaryColor3ubEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord2ivARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord2iv\0"
    "glMultiTexCoord2ivARB\0"
    "";
#endif

#if defined(need_GL_SGIS_fog_function)
static const char FogFuncSGIS_names[] = 
    "ip\0" /* Parameter signature */
    "glFogFuncSGIS\0"
    "";
#endif

#if defined(need_GL_EXT_copy_texture)
static const char CopyTexSubImage2D_names[] = 
    "iiiiiiii\0" /* Parameter signature */
    "glCopyTexSubImage2D\0"
    "glCopyTexSubImage2DEXT\0"
    "";
#endif

#if defined(need_GL_ARB_shader_objects)
static const char GetObjectParameterivARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetObjectParameterivARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord4fVertex4fSUN_names[] = 
    "ffffffff\0" /* Parameter signature */
    "glTexCoord4fVertex4fSUN\0"
    "";
#endif

#if defined(need_GL_APPLE_vertex_array_object)
static const char BindVertexArrayAPPLE_names[] = 
    "i\0" /* Parameter signature */
    "glBindVertexArrayAPPLE\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char GetProgramLocalParameterdvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetProgramLocalParameterdvARB\0"
    "";
#endif

#if defined(need_GL_EXT_histogram)
static const char GetHistogramParameteriv_names[] = 
    "iip\0" /* Parameter signature */
    "glGetHistogramParameteriv\0"
    "glGetHistogramParameterivEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord1iARB_names[] = 
    "ii\0" /* Parameter signature */
    "glMultiTexCoord1i\0"
    "glMultiTexCoord1iARB\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char GetConvolutionFilter_names[] = 
    "iiip\0" /* Parameter signature */
    "glGetConvolutionFilter\0"
    "glGetConvolutionFilterEXT\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char GetProgramivARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetProgramivARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_blend_func_separate) || defined(need_GL_INGR_blend_func_separate)
static const char BlendFuncSeparateEXT_names[] = 
    "iiii\0" /* Parameter signature */
    "glBlendFuncSeparate\0"
    "glBlendFuncSeparateEXT\0"
    "glBlendFuncSeparateINGR\0"
    "";
#endif

#if defined(need_GL_APPLE_vertex_array_object)
static const char IsVertexArrayAPPLE_names[] = 
    "i\0" /* Parameter signature */
    "glIsVertexArrayAPPLE\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char ProgramParameters4dvNV_names[] = 
    "iiip\0" /* Parameter signature */
    "glProgramParameters4dvNV\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord2fColor3fVertex3fvSUN_names[] = 
    "ppp\0" /* Parameter signature */
    "glTexCoord2fColor3fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Binormal3dvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glBinormal3dvEXT\0"
    "";
#endif

#if defined(need_GL_NV_fence)
static const char FinishFenceNV_names[] = 
    "i\0" /* Parameter signature */
    "glFinishFenceNV\0"
    "";
#endif

#if defined(need_GL_SGIS_fog_function)
static const char GetFogFuncSGIS_names[] = 
    "p\0" /* Parameter signature */
    "glGetFogFuncSGIS\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char GetUniformLocationARB_names[] = 
    "ip\0" /* Parameter signature */
    "glGetUniformLocation\0"
    "glGetUniformLocationARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3fEXT_names[] = 
    "fff\0" /* Parameter signature */
    "glSecondaryColor3f\0"
    "glSecondaryColor3fEXT\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char CombinerInputNV_names[] = 
    "iiiiii\0" /* Parameter signature */
    "glCombinerInputNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib3sARB_names[] = 
    "iiii\0" /* Parameter signature */
    "glVertexAttrib3s\0"
    "glVertexAttrib3sARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiNormal3fVertex3fvSUN_names[] = 
    "ppp\0" /* Parameter signature */
    "glReplacementCodeuiNormal3fVertex3fvSUN\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char ProgramStringARB_names[] = 
    "iiip\0" /* Parameter signature */
    "glProgramStringARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char TexCoord4fVertex4fvSUN_names[] = 
    "pp\0" /* Parameter signature */
    "glTexCoord4fVertex4fvSUN\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib3sNV_names[] = 
    "iiii\0" /* Parameter signature */
    "glVertexAttrib3sNV\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib1fNV_names[] = 
    "if\0" /* Parameter signature */
    "glVertexAttrib1fNV\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentLightfSGIX_names[] = 
    "iif\0" /* Parameter signature */
    "glFragmentLightfSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3) || defined(need_GL_ARB_texture_compression)
static const char GetCompressedTexImageARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetCompressedTexImage\0"
    "glGetCompressedTexImageARB\0"
    "";
#endif

#if defined(need_GL_EXT_vertex_weighting)
static const char VertexWeightPointerEXT_names[] = 
    "iiip\0" /* Parameter signature */
    "glVertexWeightPointerEXT\0"
    "";
#endif

#if defined(need_GL_EXT_histogram)
static const char GetHistogram_names[] = 
    "iiiip\0" /* Parameter signature */
    "glGetHistogram\0"
    "glGetHistogramEXT\0"
    "";
#endif

#if defined(need_GL_EXT_stencil_two_side)
static const char ActiveStencilFaceEXT_names[] = 
    "i\0" /* Parameter signature */
    "glActiveStencilFaceEXT\0"
    "";
#endif

#if defined(need_GL_ATI_separate_stencil)
static const char StencilFuncSeparateATI_names[] = 
    "iiii\0" /* Parameter signature */
    "glStencilFuncSeparateATI\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char GetShaderSourceARB_names[] = 
    "iipp\0" /* Parameter signature */
    "glGetShaderSource\0"
    "glGetShaderSourceARB\0"
    "";
#endif

#if defined(need_GL_SGIX_igloo_interface)
static const char IglooInterfaceSGIX_names[] = 
    "ip\0" /* Parameter signature */
    "glIglooInterfaceSGIX\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib4dNV_names[] = 
    "idddd\0" /* Parameter signature */
    "glVertexAttrib4dNV\0"
    "";
#endif

#if defined(need_GL_IBM_multimode_draw_arrays)
static const char MultiModeDrawElementsIBM_names[] = 
    "ppipii\0" /* Parameter signature */
    "glMultiModeDrawElementsIBM\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord4svARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord4sv\0"
    "glMultiTexCoord4svARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_occlusion_query)
static const char GenQueriesARB_names[] = 
    "ip\0" /* Parameter signature */
    "glGenQueries\0"
    "glGenQueriesARB\0"
    "";
#endif

#if defined(need_GL_SUN_vertex)
static const char ReplacementCodeuiVertex3fSUN_names[] = 
    "ifff\0" /* Parameter signature */
    "glReplacementCodeuiVertex3fSUN\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Tangent3iEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glTangent3iEXT\0"
    "";
#endif

#if defined(need_GL_SUN_mesh_array)
static const char DrawMeshArraysSUN_names[] = 
    "iiii\0" /* Parameter signature */
    "glDrawMeshArraysSUN\0"
    "";
#endif

#if defined(need_GL_NV_evaluators)
static const char GetMapControlPointsNV_names[] = 
    "iiiiiip\0" /* Parameter signature */
    "glGetMapControlPointsNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_draw_buffers) || defined(need_GL_ATI_draw_buffers)
static const char DrawBuffersARB_names[] = 
    "ip\0" /* Parameter signature */
    "glDrawBuffers\0"
    "glDrawBuffersARB\0"
    "glDrawBuffersATI\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char ProgramLocalParameter4fARB_names[] = 
    "iiffff\0" /* Parameter signature */
    "glProgramLocalParameter4fARB\0"
    "";
#endif

#if defined(need_GL_SGIX_sprite)
static const char SpriteParameterivSGIX_names[] = 
    "ip\0" /* Parameter signature */
    "glSpriteParameterivSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord1fARB_names[] = 
    "if\0" /* Parameter signature */
    "glMultiTexCoord1f\0"
    "glMultiTexCoord1fARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs4ubvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs4ubvNV\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_blend)
static const char WeightsvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glWeightsvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_shader_objects)
static const char Uniform1fvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glUniform1fv\0"
    "glUniform1fvARB\0"
    "";
#endif

#if defined(need_GL_EXT_copy_texture)
static const char CopyTexSubImage1D_names[] = 
    "iiiiii\0" /* Parameter signature */
    "glCopyTexSubImage1D\0"
    "glCopyTexSubImage1DEXT\0"
    "";
#endif

#if defined(need_GL_EXT_texture_object)
static const char BindTexture_names[] = 
    "ii\0" /* Parameter signature */
    "glBindTexture\0"
    "glBindTextureEXT\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char BeginFragmentShaderATI_names[] = 
    "\0" /* Parameter signature */
    "glBeginFragmentShaderATI\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord4fARB_names[] = 
    "iffff\0" /* Parameter signature */
    "glMultiTexCoord4f\0"
    "glMultiTexCoord4fARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs3svNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs3svNV\0"
    "";
#endif

#if defined(need_GL_SUN_triangle_list)
static const char ReplacementCodeuivSUN_names[] = 
    "p\0" /* Parameter signature */
    "glReplacementCodeuivSUN\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char EnableVertexAttribArrayARB_names[] = 
    "i\0" /* Parameter signature */
    "glEnableVertexAttribArray\0"
    "glEnableVertexAttribArrayARB\0"
    "";
#endif

#if defined(need_GL_INTEL_parallel_arrays)
static const char NormalPointervINTEL_names[] = 
    "ip\0" /* Parameter signature */
    "glNormalPointervINTEL\0"
    "";
#endif

#if defined(need_GL_EXT_convolution)
static const char CopyConvolutionFilter2D_names[] = 
    "iiiiii\0" /* Parameter signature */
    "glCopyConvolutionFilter2D\0"
    "glCopyConvolutionFilter2DEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos3ivMESA_names[] = 
    "p\0" /* Parameter signature */
    "glWindowPos3iv\0"
    "glWindowPos3ivARB\0"
    "glWindowPos3ivMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_vertex_buffer_object)
static const char IsBufferARB_names[] = 
    "i\0" /* Parameter signature */
    "glIsBuffer\0"
    "glIsBufferARB\0"
    "";
#endif

#if defined(need_GL_MESA_window_pos)
static const char WindowPos4iMESA_names[] = 
    "iiii\0" /* Parameter signature */
    "glWindowPos4iMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4uivARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4uiv\0"
    "glVertexAttrib4uivARB\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Tangent3bvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glTangent3bvEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_1)
static const char UniformMatrix3x4fv_names[] = 
    "iiip\0" /* Parameter signature */
    "glUniformMatrix3x4fv\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Binormal3fvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glBinormal3fvEXT\0"
    "";
#endif

#if defined(need_GL_INTEL_parallel_arrays)
static const char TexCoordPointervINTEL_names[] = 
    "iip\0" /* Parameter signature */
    "glTexCoordPointervINTEL\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_vertex_buffer_object)
static const char DeleteBuffersARB_names[] = 
    "ip\0" /* Parameter signature */
    "glDeleteBuffers\0"
    "glDeleteBuffersARB\0"
    "";
#endif

#if defined(need_GL_MESA_window_pos)
static const char WindowPos4fvMESA_names[] = 
    "p\0" /* Parameter signature */
    "glWindowPos4fvMESA\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib1sNV_names[] = 
    "ii\0" /* Parameter signature */
    "glVertexAttrib1sNV\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_secondary_color)
static const char SecondaryColor3svEXT_names[] = 
    "p\0" /* Parameter signature */
    "glSecondaryColor3sv\0"
    "glSecondaryColor3svEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3) || defined(need_GL_ARB_transpose_matrix)
static const char LoadTransposeMatrixfARB_names[] = 
    "p\0" /* Parameter signature */
    "glLoadTransposeMatrixf\0"
    "glLoadTransposeMatrixfARB\0"
    "";
#endif

#if defined(need_GL_EXT_vertex_array)
static const char GetPointerv_names[] = 
    "ip\0" /* Parameter signature */
    "glGetPointerv\0"
    "glGetPointervEXT\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Tangent3bEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glTangent3bEXT\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char CombinerParameterfNV_names[] = 
    "if\0" /* Parameter signature */
    "glCombinerParameterfNV\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program) || defined(need_GL_NV_vertex_program)
static const char BindProgramNV_names[] = 
    "ii\0" /* Parameter signature */
    "glBindProgramARB\0"
    "glBindProgramNV\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4svARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4sv\0"
    "glVertexAttrib4svARB\0"
    "";
#endif

#if defined(need_GL_MESA_shader_debug)
static const char CreateDebugObjectMESA_names[] = 
    "\0" /* Parameter signature */
    "glCreateDebugObjectMESA\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0)
static const char GetShaderiv_names[] = 
    "iip\0" /* Parameter signature */
    "glGetShaderiv\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char BindFragmentShaderATI_names[] = 
    "i\0" /* Parameter signature */
    "glBindFragmentShaderATI\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_vertex_buffer_object)
static const char UnmapBufferARB_names[] = 
    "i\0" /* Parameter signature */
    "glUnmapBuffer\0"
    "glUnmapBufferARB\0"
    "";
#endif

#if defined(need_GL_EXT_histogram)
static const char Minmax_names[] = 
    "iii\0" /* Parameter signature */
    "glMinmax\0"
    "glMinmaxEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_EXT_fog_coord)
static const char FogCoorddvEXT_names[] = 
    "p\0" /* Parameter signature */
    "glFogCoorddv\0"
    "glFogCoorddvEXT\0"
    "";
#endif

#if defined(need_GL_SUNX_constant_data)
static const char FinishTextureSUNX_names[] = 
    "\0" /* Parameter signature */
    "glFinishTextureSUNX\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char GetFragmentLightfvSGIX_names[] = 
    "iip\0" /* Parameter signature */
    "glGetFragmentLightfvSGIX\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char GetFinalCombinerInputParameterfvNV_names[] = 
    "iip\0" /* Parameter signature */
    "glGetFinalCombinerInputParameterfvNV\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char ColorFragmentOp3ATI_names[] = 
    "iiiiiiiiiiiii\0" /* Parameter signature */
    "glColorFragmentOp3ATI\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib2svARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib2sv\0"
    "glVertexAttrib2svARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char AreProgramsResidentNV_names[] = 
    "ipp\0" /* Parameter signature */
    "glAreProgramsResidentNV\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos3svMESA_names[] = 
    "p\0" /* Parameter signature */
    "glWindowPos3sv\0"
    "glWindowPos3svARB\0"
    "glWindowPos3svMESA\0"
    "";
#endif

#if defined(need_GL_EXT_color_subtable)
static const char CopyColorSubTable_names[] = 
    "iiiii\0" /* Parameter signature */
    "glCopyColorSubTable\0"
    "glCopyColorSubTableEXT\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_blend)
static const char WeightdvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glWeightdvARB\0"
    "";
#endif

#if defined(need_GL_SGIX_instruments)
static const char PollInstrumentsSGIX_names[] = 
    "p\0" /* Parameter signature */
    "glPollInstrumentsSGIX\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib4NubvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4Nubv\0"
    "glVertexAttrib4NubvARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib3dvNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib3dvNV\0"
    "";
#endif

#if defined(need_GL_ARB_shader_objects)
static const char GetObjectParameterfvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetObjectParameterfvARB\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program)
static const char GetProgramEnvParameterdvARB_names[] = 
    "iip\0" /* Parameter signature */
    "glGetProgramEnvParameterdvARB\0"
    "";
#endif

#if defined(need_GL_EXT_compiled_vertex_array)
static const char LockArraysEXT_names[] = 
    "ii\0" /* Parameter signature */
    "glLockArraysEXT\0"
    "";
#endif

#if defined(need_GL_EXT_pixel_transform)
static const char PixelTransformParameterivEXT_names[] = 
    "iip\0" /* Parameter signature */
    "glPixelTransformParameterivEXT\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char BinormalPointerEXT_names[] = 
    "iip\0" /* Parameter signature */
    "glBinormalPointerEXT\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib1dNV_names[] = 
    "id\0" /* Parameter signature */
    "glVertexAttrib1dNV\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char GetCombinerInputParameterivNV_names[] = 
    "iiiip\0" /* Parameter signature */
    "glGetCombinerInputParameterivNV\0"
    "";
#endif

#if defined(need_GL_VERSION_1_3)
static const char MultiTexCoord2fvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glMultiTexCoord2fv\0"
    "glMultiTexCoord2fvARB\0"
    "";
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const char GetRenderbufferParameterivEXT_names[] = 
    "iip\0" /* Parameter signature */
    "glGetRenderbufferParameterivEXT\0"
    "";
#endif

#if defined(need_GL_NV_register_combiners)
static const char CombinerParameterivNV_names[] = 
    "ip\0" /* Parameter signature */
    "glCombinerParameterivNV\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char GenFragmentShadersATI_names[] = 
    "i\0" /* Parameter signature */
    "glGenFragmentShadersATI\0"
    "";
#endif

#if defined(need_GL_EXT_vertex_array)
static const char DrawArrays_names[] = 
    "iii\0" /* Parameter signature */
    "glDrawArrays\0"
    "glDrawArraysEXT\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_blend)
static const char WeightuivARB_names[] = 
    "ip\0" /* Parameter signature */
    "glWeightuivARB\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib2sARB_names[] = 
    "iii\0" /* Parameter signature */
    "glVertexAttrib2s\0"
    "glVertexAttrib2sARB\0"
    "";
#endif

#if defined(need_GL_SGIX_async)
static const char GenAsyncMarkersSGIX_names[] = 
    "i\0" /* Parameter signature */
    "glGenAsyncMarkersSGIX\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Tangent3svEXT_names[] = 
    "p\0" /* Parameter signature */
    "glTangent3svEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_vertex_buffer_object)
static const char BindBufferARB_names[] = 
    "ii\0" /* Parameter signature */
    "glBindBuffer\0"
    "glBindBufferARB\0"
    "";
#endif

#if defined(need_GL_ARB_shader_objects)
static const char GetInfoLogARB_names[] = 
    "iipp\0" /* Parameter signature */
    "glGetInfoLogARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs4svNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs4svNV\0"
    "";
#endif

#if defined(need_GL_IBM_vertex_array_lists)
static const char EdgeFlagPointerListIBM_names[] = 
    "ipi\0" /* Parameter signature */
    "glEdgeFlagPointerListIBM\0"
    "";
#endif

#if defined(need_GL_VERSION_2_1)
static const char UniformMatrix3x2fv_names[] = 
    "iiip\0" /* Parameter signature */
    "glUniformMatrix3x2fv\0"
    "";
#endif

#if defined(need_GL_EXT_histogram)
static const char GetMinmaxParameterfv_names[] = 
    "iip\0" /* Parameter signature */
    "glGetMinmaxParameterfv\0"
    "glGetMinmaxParameterfvEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_2_0) || defined(need_GL_ARB_vertex_program)
static const char VertexAttrib1fvARB_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib1fv\0"
    "glVertexAttrib1fvARB\0"
    "";
#endif

#if defined(need_GL_VERSION_1_5) || defined(need_GL_ARB_vertex_buffer_object)
static const char GenBuffersARB_names[] = 
    "ip\0" /* Parameter signature */
    "glGenBuffers\0"
    "glGenBuffersARB\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttribs1svNV_names[] = 
    "iip\0" /* Parameter signature */
    "glVertexAttribs1svNV\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Binormal3bEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glBinormal3bEXT\0"
    "";
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const char FragmentMaterialivSGIX_names[] = 
    "iip\0" /* Parameter signature */
    "glFragmentMaterialivSGIX\0"
    "";
#endif

#if defined(need_GL_NV_vertex_array_range)
static const char VertexArrayRangeNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexArrayRangeNV\0"
    "";
#endif

#if defined(need_GL_ARB_vertex_program) || defined(need_GL_NV_vertex_program)
static const char GenProgramsNV_names[] = 
    "ip\0" /* Parameter signature */
    "glGenProgramsARB\0"
    "glGenProgramsNV\0"
    "";
#endif

#if defined(need_GL_NV_vertex_program)
static const char VertexAttrib4dvNV_names[] = 
    "ip\0" /* Parameter signature */
    "glVertexAttrib4dvNV\0"
    "";
#endif

#if defined(need_GL_ATI_fragment_shader)
static const char EndFragmentShaderATI_names[] = 
    "\0" /* Parameter signature */
    "glEndFragmentShaderATI\0"
    "";
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const char Binormal3iEXT_names[] = 
    "iii\0" /* Parameter signature */
    "glBinormal3iEXT\0"
    "";
#endif

#if defined(need_GL_VERSION_1_4) || defined(need_GL_ARB_window_pos) || defined(need_GL_MESA_window_pos)
static const char WindowPos2fMESA_names[] = 
    "ff\0" /* Parameter signature */
    "glWindowPos2f\0"
    "glWindowPos2fARB\0"
    "glWindowPos2fMESA\0"
    "";
#endif

#if defined(need_GL_3DFX_tbuffer)
static const struct dri_extension_function GL_3DFX_tbuffer_functions[] = {
    { TbufferMask3DFX_names, TbufferMask3DFX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_APPLE_vertex_array_object)
static const struct dri_extension_function GL_APPLE_vertex_array_object_functions[] = {
    { DeleteVertexArraysAPPLE_names, DeleteVertexArraysAPPLE_remap_index, -1 },
    { GenVertexArraysAPPLE_names, GenVertexArraysAPPLE_remap_index, -1 },
    { BindVertexArrayAPPLE_names, BindVertexArrayAPPLE_remap_index, -1 },
    { IsVertexArrayAPPLE_names, IsVertexArrayAPPLE_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_draw_buffers)
static const struct dri_extension_function GL_ARB_draw_buffers_functions[] = {
    { DrawBuffersARB_names, DrawBuffersARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_matrix_palette)
static const struct dri_extension_function GL_ARB_matrix_palette_functions[] = {
    { MatrixIndexusvARB_names, MatrixIndexusvARB_remap_index, -1 },
    { MatrixIndexuivARB_names, MatrixIndexuivARB_remap_index, -1 },
    { MatrixIndexPointerARB_names, MatrixIndexPointerARB_remap_index, -1 },
    { CurrentPaletteMatrixARB_names, CurrentPaletteMatrixARB_remap_index, -1 },
    { MatrixIndexubvARB_names, MatrixIndexubvARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_multisample)
static const struct dri_extension_function GL_ARB_multisample_functions[] = {
    { SampleCoverageARB_names, SampleCoverageARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_occlusion_query)
static const struct dri_extension_function GL_ARB_occlusion_query_functions[] = {
    { BeginQueryARB_names, BeginQueryARB_remap_index, -1 },
    { GetQueryivARB_names, GetQueryivARB_remap_index, -1 },
    { GetQueryObjectivARB_names, GetQueryObjectivARB_remap_index, -1 },
    { EndQueryARB_names, EndQueryARB_remap_index, -1 },
    { GetQueryObjectuivARB_names, GetQueryObjectuivARB_remap_index, -1 },
    { DeleteQueriesARB_names, DeleteQueriesARB_remap_index, -1 },
    { IsQueryARB_names, IsQueryARB_remap_index, -1 },
    { GenQueriesARB_names, GenQueriesARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_point_parameters)
static const struct dri_extension_function GL_ARB_point_parameters_functions[] = {
    { PointParameterfEXT_names, PointParameterfEXT_remap_index, -1 },
    { PointParameterfvEXT_names, PointParameterfvEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_shader_objects)
static const struct dri_extension_function GL_ARB_shader_objects_functions[] = {
    { UniformMatrix3fvARB_names, UniformMatrix3fvARB_remap_index, -1 },
    { Uniform2fARB_names, Uniform2fARB_remap_index, -1 },
    { Uniform2ivARB_names, Uniform2ivARB_remap_index, -1 },
    { UniformMatrix4fvARB_names, UniformMatrix4fvARB_remap_index, -1 },
    { CreateProgramObjectARB_names, CreateProgramObjectARB_remap_index, -1 },
    { Uniform3iARB_names, Uniform3iARB_remap_index, -1 },
    { CreateShaderObjectARB_names, CreateShaderObjectARB_remap_index, -1 },
    { Uniform1fARB_names, Uniform1fARB_remap_index, -1 },
    { AttachObjectARB_names, AttachObjectARB_remap_index, -1 },
    { UniformMatrix2fvARB_names, UniformMatrix2fvARB_remap_index, -1 },
    { GetAttachedObjectsARB_names, GetAttachedObjectsARB_remap_index, -1 },
    { Uniform3fvARB_names, Uniform3fvARB_remap_index, -1 },
    { GetHandleARB_names, GetHandleARB_remap_index, -1 },
    { GetActiveUniformARB_names, GetActiveUniformARB_remap_index, -1 },
    { GetUniformivARB_names, GetUniformivARB_remap_index, -1 },
    { Uniform2fvARB_names, Uniform2fvARB_remap_index, -1 },
    { DeleteObjectARB_names, DeleteObjectARB_remap_index, -1 },
    { ValidateProgramARB_names, ValidateProgramARB_remap_index, -1 },
    { Uniform3ivARB_names, Uniform3ivARB_remap_index, -1 },
    { CompileShaderARB_names, CompileShaderARB_remap_index, -1 },
    { Uniform3fARB_names, Uniform3fARB_remap_index, -1 },
    { Uniform4fARB_names, Uniform4fARB_remap_index, -1 },
    { LinkProgramARB_names, LinkProgramARB_remap_index, -1 },
    { ShaderSourceARB_names, ShaderSourceARB_remap_index, -1 },
    { Uniform4ivARB_names, Uniform4ivARB_remap_index, -1 },
    { Uniform1ivARB_names, Uniform1ivARB_remap_index, -1 },
    { Uniform1iARB_names, Uniform1iARB_remap_index, -1 },
    { Uniform4fvARB_names, Uniform4fvARB_remap_index, -1 },
    { GetUniformfvARB_names, GetUniformfvARB_remap_index, -1 },
    { DetachObjectARB_names, DetachObjectARB_remap_index, -1 },
    { Uniform4iARB_names, Uniform4iARB_remap_index, -1 },
    { UseProgramObjectARB_names, UseProgramObjectARB_remap_index, -1 },
    { Uniform2iARB_names, Uniform2iARB_remap_index, -1 },
    { GetObjectParameterivARB_names, GetObjectParameterivARB_remap_index, -1 },
    { GetUniformLocationARB_names, GetUniformLocationARB_remap_index, -1 },
    { GetShaderSourceARB_names, GetShaderSourceARB_remap_index, -1 },
    { Uniform1fvARB_names, Uniform1fvARB_remap_index, -1 },
    { GetObjectParameterfvARB_names, GetObjectParameterfvARB_remap_index, -1 },
    { GetInfoLogARB_names, GetInfoLogARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_texture_compression)
static const struct dri_extension_function GL_ARB_texture_compression_functions[] = {
    { CompressedTexSubImage2DARB_names, CompressedTexSubImage2DARB_remap_index, -1 },
    { CompressedTexImage3DARB_names, CompressedTexImage3DARB_remap_index, -1 },
    { CompressedTexImage1DARB_names, CompressedTexImage1DARB_remap_index, -1 },
    { CompressedTexSubImage1DARB_names, CompressedTexSubImage1DARB_remap_index, -1 },
    { CompressedTexSubImage3DARB_names, CompressedTexSubImage3DARB_remap_index, -1 },
    { CompressedTexImage2DARB_names, CompressedTexImage2DARB_remap_index, -1 },
    { GetCompressedTexImageARB_names, GetCompressedTexImageARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_transpose_matrix)
static const struct dri_extension_function GL_ARB_transpose_matrix_functions[] = {
    { MultTransposeMatrixdARB_names, MultTransposeMatrixdARB_remap_index, -1 },
    { LoadTransposeMatrixdARB_names, LoadTransposeMatrixdARB_remap_index, -1 },
    { MultTransposeMatrixfARB_names, MultTransposeMatrixfARB_remap_index, -1 },
    { LoadTransposeMatrixfARB_names, LoadTransposeMatrixfARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_vertex_blend)
static const struct dri_extension_function GL_ARB_vertex_blend_functions[] = {
    { WeightubvARB_names, WeightubvARB_remap_index, -1 },
    { WeightivARB_names, WeightivARB_remap_index, -1 },
    { WeightPointerARB_names, WeightPointerARB_remap_index, -1 },
    { WeightfvARB_names, WeightfvARB_remap_index, -1 },
    { WeightbvARB_names, WeightbvARB_remap_index, -1 },
    { WeightusvARB_names, WeightusvARB_remap_index, -1 },
    { VertexBlendARB_names, VertexBlendARB_remap_index, -1 },
    { WeightsvARB_names, WeightsvARB_remap_index, -1 },
    { WeightdvARB_names, WeightdvARB_remap_index, -1 },
    { WeightuivARB_names, WeightuivARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_vertex_buffer_object)
static const struct dri_extension_function GL_ARB_vertex_buffer_object_functions[] = {
    { GetBufferSubDataARB_names, GetBufferSubDataARB_remap_index, -1 },
    { BufferSubDataARB_names, BufferSubDataARB_remap_index, -1 },
    { BufferDataARB_names, BufferDataARB_remap_index, -1 },
    { GetBufferPointervARB_names, GetBufferPointervARB_remap_index, -1 },
    { GetBufferParameterivARB_names, GetBufferParameterivARB_remap_index, -1 },
    { MapBufferARB_names, MapBufferARB_remap_index, -1 },
    { IsBufferARB_names, IsBufferARB_remap_index, -1 },
    { DeleteBuffersARB_names, DeleteBuffersARB_remap_index, -1 },
    { UnmapBufferARB_names, UnmapBufferARB_remap_index, -1 },
    { BindBufferARB_names, BindBufferARB_remap_index, -1 },
    { GenBuffersARB_names, GenBuffersARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_vertex_program)
static const struct dri_extension_function GL_ARB_vertex_program_functions[] = {
    { VertexAttrib2fARB_names, VertexAttrib2fARB_remap_index, -1 },
    { VertexAttrib3fARB_names, VertexAttrib3fARB_remap_index, -1 },
    { VertexAttrib1svARB_names, VertexAttrib1svARB_remap_index, -1 },
    { VertexAttrib4NusvARB_names, VertexAttrib4NusvARB_remap_index, -1 },
    { DisableVertexAttribArrayARB_names, DisableVertexAttribArrayARB_remap_index, -1 },
    { ProgramLocalParameter4dARB_names, ProgramLocalParameter4dARB_remap_index, -1 },
    { VertexAttrib1fARB_names, VertexAttrib1fARB_remap_index, -1 },
    { VertexAttrib4NbvARB_names, VertexAttrib4NbvARB_remap_index, -1 },
    { VertexAttrib1sARB_names, VertexAttrib1sARB_remap_index, -1 },
    { GetProgramLocalParameterfvARB_names, GetProgramLocalParameterfvARB_remap_index, -1 },
    { VertexAttrib3dvARB_names, VertexAttrib3dvARB_remap_index, -1 },
    { ProgramEnvParameter4fvARB_names, ProgramEnvParameter4fvARB_remap_index, -1 },
    { GetVertexAttribivARB_names, GetVertexAttribivARB_remap_index, -1 },
    { VertexAttrib4ivARB_names, VertexAttrib4ivARB_remap_index, -1 },
    { VertexAttrib4bvARB_names, VertexAttrib4bvARB_remap_index, -1 },
    { VertexAttrib3dARB_names, VertexAttrib3dARB_remap_index, -1 },
    { VertexAttrib4fARB_names, VertexAttrib4fARB_remap_index, -1 },
    { VertexAttrib4fvARB_names, VertexAttrib4fvARB_remap_index, -1 },
    { ProgramLocalParameter4dvARB_names, ProgramLocalParameter4dvARB_remap_index, -1 },
    { VertexAttrib2dARB_names, VertexAttrib2dARB_remap_index, -1 },
    { VertexAttrib1dvARB_names, VertexAttrib1dvARB_remap_index, -1 },
    { GetVertexAttribfvARB_names, GetVertexAttribfvARB_remap_index, -1 },
    { VertexAttrib4ubvARB_names, VertexAttrib4ubvARB_remap_index, -1 },
    { ProgramEnvParameter4fARB_names, ProgramEnvParameter4fARB_remap_index, -1 },
    { VertexAttrib4sARB_names, VertexAttrib4sARB_remap_index, -1 },
    { VertexAttrib2dvARB_names, VertexAttrib2dvARB_remap_index, -1 },
    { VertexAttrib2fvARB_names, VertexAttrib2fvARB_remap_index, -1 },
    { VertexAttrib4NivARB_names, VertexAttrib4NivARB_remap_index, -1 },
    { GetProgramStringARB_names, GetProgramStringARB_remap_index, -1 },
    { VertexAttrib4NuivARB_names, VertexAttrib4NuivARB_remap_index, -1 },
    { IsProgramNV_names, IsProgramNV_remap_index, -1 },
    { ProgramEnvParameter4dARB_names, ProgramEnvParameter4dARB_remap_index, -1 },
    { VertexAttrib1dARB_names, VertexAttrib1dARB_remap_index, -1 },
    { VertexAttrib4usvARB_names, VertexAttrib4usvARB_remap_index, -1 },
    { VertexAttrib3svARB_names, VertexAttrib3svARB_remap_index, -1 },
    { GetVertexAttribdvARB_names, GetVertexAttribdvARB_remap_index, -1 },
    { VertexAttrib4dvARB_names, VertexAttrib4dvARB_remap_index, -1 },
    { VertexAttribPointerARB_names, VertexAttribPointerARB_remap_index, -1 },
    { VertexAttrib4NsvARB_names, VertexAttrib4NsvARB_remap_index, -1 },
    { VertexAttrib3fvARB_names, VertexAttrib3fvARB_remap_index, -1 },
    { VertexAttrib4NubARB_names, VertexAttrib4NubARB_remap_index, -1 },
    { GetProgramEnvParameterfvARB_names, GetProgramEnvParameterfvARB_remap_index, -1 },
    { ProgramEnvParameter4dvARB_names, ProgramEnvParameter4dvARB_remap_index, -1 },
    { ProgramLocalParameter4fvARB_names, ProgramLocalParameter4fvARB_remap_index, -1 },
    { DeleteProgramsNV_names, DeleteProgramsNV_remap_index, -1 },
    { GetVertexAttribPointervNV_names, GetVertexAttribPointervNV_remap_index, -1 },
    { VertexAttrib4dARB_names, VertexAttrib4dARB_remap_index, -1 },
    { GetProgramLocalParameterdvARB_names, GetProgramLocalParameterdvARB_remap_index, -1 },
    { GetProgramivARB_names, GetProgramivARB_remap_index, -1 },
    { VertexAttrib3sARB_names, VertexAttrib3sARB_remap_index, -1 },
    { ProgramStringARB_names, ProgramStringARB_remap_index, -1 },
    { ProgramLocalParameter4fARB_names, ProgramLocalParameter4fARB_remap_index, -1 },
    { EnableVertexAttribArrayARB_names, EnableVertexAttribArrayARB_remap_index, -1 },
    { VertexAttrib4uivARB_names, VertexAttrib4uivARB_remap_index, -1 },
    { BindProgramNV_names, BindProgramNV_remap_index, -1 },
    { VertexAttrib4svARB_names, VertexAttrib4svARB_remap_index, -1 },
    { VertexAttrib2svARB_names, VertexAttrib2svARB_remap_index, -1 },
    { VertexAttrib4NubvARB_names, VertexAttrib4NubvARB_remap_index, -1 },
    { GetProgramEnvParameterdvARB_names, GetProgramEnvParameterdvARB_remap_index, -1 },
    { VertexAttrib2sARB_names, VertexAttrib2sARB_remap_index, -1 },
    { VertexAttrib1fvARB_names, VertexAttrib1fvARB_remap_index, -1 },
    { GenProgramsNV_names, GenProgramsNV_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_vertex_shader)
static const struct dri_extension_function GL_ARB_vertex_shader_functions[] = {
    { GetActiveAttribARB_names, GetActiveAttribARB_remap_index, -1 },
    { GetAttribLocationARB_names, GetAttribLocationARB_remap_index, -1 },
    { BindAttribLocationARB_names, BindAttribLocationARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ARB_window_pos)
static const struct dri_extension_function GL_ARB_window_pos_functions[] = {
    { WindowPos3fMESA_names, WindowPos3fMESA_remap_index, -1 },
    { WindowPos2dvMESA_names, WindowPos2dvMESA_remap_index, -1 },
    { WindowPos2svMESA_names, WindowPos2svMESA_remap_index, -1 },
    { WindowPos3dMESA_names, WindowPos3dMESA_remap_index, -1 },
    { WindowPos2fvMESA_names, WindowPos2fvMESA_remap_index, -1 },
    { WindowPos2dMESA_names, WindowPos2dMESA_remap_index, -1 },
    { WindowPos3dvMESA_names, WindowPos3dvMESA_remap_index, -1 },
    { WindowPos3fvMESA_names, WindowPos3fvMESA_remap_index, -1 },
    { WindowPos2iMESA_names, WindowPos2iMESA_remap_index, -1 },
    { WindowPos3sMESA_names, WindowPos3sMESA_remap_index, -1 },
    { WindowPos2ivMESA_names, WindowPos2ivMESA_remap_index, -1 },
    { WindowPos2sMESA_names, WindowPos2sMESA_remap_index, -1 },
    { WindowPos3iMESA_names, WindowPos3iMESA_remap_index, -1 },
    { WindowPos3ivMESA_names, WindowPos3ivMESA_remap_index, -1 },
    { WindowPos3svMESA_names, WindowPos3svMESA_remap_index, -1 },
    { WindowPos2fMESA_names, WindowPos2fMESA_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ATI_blend_equation_separate)
static const struct dri_extension_function GL_ATI_blend_equation_separate_functions[] = {
    { BlendEquationSeparateEXT_names, BlendEquationSeparateEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ATI_draw_buffers)
static const struct dri_extension_function GL_ATI_draw_buffers_functions[] = {
    { DrawBuffersARB_names, DrawBuffersARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ATI_fragment_shader)
static const struct dri_extension_function GL_ATI_fragment_shader_functions[] = {
    { ColorFragmentOp2ATI_names, ColorFragmentOp2ATI_remap_index, -1 },
    { DeleteFragmentShaderATI_names, DeleteFragmentShaderATI_remap_index, -1 },
    { SetFragmentShaderConstantATI_names, SetFragmentShaderConstantATI_remap_index, -1 },
    { SampleMapATI_names, SampleMapATI_remap_index, -1 },
    { AlphaFragmentOp2ATI_names, AlphaFragmentOp2ATI_remap_index, -1 },
    { AlphaFragmentOp1ATI_names, AlphaFragmentOp1ATI_remap_index, -1 },
    { ColorFragmentOp1ATI_names, ColorFragmentOp1ATI_remap_index, -1 },
    { AlphaFragmentOp3ATI_names, AlphaFragmentOp3ATI_remap_index, -1 },
    { PassTexCoordATI_names, PassTexCoordATI_remap_index, -1 },
    { BeginFragmentShaderATI_names, BeginFragmentShaderATI_remap_index, -1 },
    { BindFragmentShaderATI_names, BindFragmentShaderATI_remap_index, -1 },
    { ColorFragmentOp3ATI_names, ColorFragmentOp3ATI_remap_index, -1 },
    { GenFragmentShadersATI_names, GenFragmentShadersATI_remap_index, -1 },
    { EndFragmentShaderATI_names, EndFragmentShaderATI_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_ATI_separate_stencil)
static const struct dri_extension_function GL_ATI_separate_stencil_functions[] = {
    { StencilOpSeparate_names, StencilOpSeparate_remap_index, -1 },
    { StencilFuncSeparateATI_names, StencilFuncSeparateATI_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_blend_color)
static const struct dri_extension_function GL_EXT_blend_color_functions[] = {
    { BlendColor_names, -1, 336 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_blend_equation_separate)
static const struct dri_extension_function GL_EXT_blend_equation_separate_functions[] = {
    { BlendEquationSeparateEXT_names, BlendEquationSeparateEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_blend_func_separate)
static const struct dri_extension_function GL_EXT_blend_func_separate_functions[] = {
    { BlendFuncSeparateEXT_names, BlendFuncSeparateEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_blend_minmax)
static const struct dri_extension_function GL_EXT_blend_minmax_functions[] = {
    { BlendEquation_names, -1, 337 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_color_subtable)
static const struct dri_extension_function GL_EXT_color_subtable_functions[] = {
    { ColorSubTable_names, -1, 346 },
    { CopyColorSubTable_names, -1, 347 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_compiled_vertex_array)
static const struct dri_extension_function GL_EXT_compiled_vertex_array_functions[] = {
    { UnlockArraysEXT_names, UnlockArraysEXT_remap_index, -1 },
    { LockArraysEXT_names, LockArraysEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_convolution)
static const struct dri_extension_function GL_EXT_convolution_functions[] = {
    { ConvolutionFilter1D_names, -1, 348 },
    { CopyConvolutionFilter1D_names, -1, 354 },
    { GetConvolutionParameteriv_names, -1, 358 },
    { ConvolutionFilter2D_names, -1, 349 },
    { ConvolutionParameteriv_names, -1, 353 },
    { ConvolutionParameterfv_names, -1, 351 },
    { GetSeparableFilter_names, -1, 359 },
    { SeparableFilter2D_names, -1, 360 },
    { ConvolutionParameteri_names, -1, 352 },
    { ConvolutionParameterf_names, -1, 350 },
    { GetConvolutionParameterfv_names, -1, 357 },
    { GetConvolutionFilter_names, -1, 356 },
    { CopyConvolutionFilter2D_names, -1, 355 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const struct dri_extension_function GL_EXT_coordinate_frame_functions[] = {
    { TangentPointerEXT_names, TangentPointerEXT_remap_index, -1 },
    { Binormal3ivEXT_names, Binormal3ivEXT_remap_index, -1 },
    { Tangent3sEXT_names, Tangent3sEXT_remap_index, -1 },
    { Tangent3fvEXT_names, Tangent3fvEXT_remap_index, -1 },
    { Binormal3fEXT_names, Binormal3fEXT_remap_index, -1 },
    { Tangent3dvEXT_names, Tangent3dvEXT_remap_index, -1 },
    { Binormal3bvEXT_names, Binormal3bvEXT_remap_index, -1 },
    { Binormal3dEXT_names, Binormal3dEXT_remap_index, -1 },
    { Tangent3fEXT_names, Tangent3fEXT_remap_index, -1 },
    { Binormal3sEXT_names, Binormal3sEXT_remap_index, -1 },
    { Tangent3ivEXT_names, Tangent3ivEXT_remap_index, -1 },
    { Tangent3dEXT_names, Tangent3dEXT_remap_index, -1 },
    { Binormal3svEXT_names, Binormal3svEXT_remap_index, -1 },
    { Binormal3dvEXT_names, Binormal3dvEXT_remap_index, -1 },
    { Tangent3iEXT_names, Tangent3iEXT_remap_index, -1 },
    { Tangent3bvEXT_names, Tangent3bvEXT_remap_index, -1 },
    { Binormal3fvEXT_names, Binormal3fvEXT_remap_index, -1 },
    { Tangent3bEXT_names, Tangent3bEXT_remap_index, -1 },
    { BinormalPointerEXT_names, BinormalPointerEXT_remap_index, -1 },
    { Tangent3svEXT_names, Tangent3svEXT_remap_index, -1 },
    { Binormal3bEXT_names, Binormal3bEXT_remap_index, -1 },
    { Binormal3iEXT_names, Binormal3iEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_copy_texture)
static const struct dri_extension_function GL_EXT_copy_texture_functions[] = {
    { CopyTexSubImage3D_names, -1, 373 },
    { CopyTexImage2D_names, -1, 324 },
    { CopyTexImage1D_names, -1, 323 },
    { CopyTexSubImage2D_names, -1, 326 },
    { CopyTexSubImage1D_names, -1, 325 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_cull_vertex)
static const struct dri_extension_function GL_EXT_cull_vertex_functions[] = {
    { CullParameterdvEXT_names, CullParameterdvEXT_remap_index, -1 },
    { CullParameterfvEXT_names, CullParameterfvEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_depth_bounds_test)
static const struct dri_extension_function GL_EXT_depth_bounds_test_functions[] = {
    { DepthBoundsEXT_names, DepthBoundsEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_draw_range_elements)
static const struct dri_extension_function GL_EXT_draw_range_elements_functions[] = {
    { DrawRangeElements_names, -1, 338 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_fog_coord)
static const struct dri_extension_function GL_EXT_fog_coord_functions[] = {
    { FogCoorddEXT_names, FogCoorddEXT_remap_index, -1 },
    { FogCoordfEXT_names, FogCoordfEXT_remap_index, -1 },
    { FogCoordPointerEXT_names, FogCoordPointerEXT_remap_index, -1 },
    { FogCoordfvEXT_names, FogCoordfvEXT_remap_index, -1 },
    { FogCoorddvEXT_names, FogCoorddvEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_framebuffer_blit)
static const struct dri_extension_function GL_EXT_framebuffer_blit_functions[] = {
    { BlitFramebufferEXT_names, BlitFramebufferEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_framebuffer_object)
static const struct dri_extension_function GL_EXT_framebuffer_object_functions[] = {
    { GenerateMipmapEXT_names, GenerateMipmapEXT_remap_index, -1 },
    { IsRenderbufferEXT_names, IsRenderbufferEXT_remap_index, -1 },
    { RenderbufferStorageEXT_names, RenderbufferStorageEXT_remap_index, -1 },
    { CheckFramebufferStatusEXT_names, CheckFramebufferStatusEXT_remap_index, -1 },
    { DeleteRenderbuffersEXT_names, DeleteRenderbuffersEXT_remap_index, -1 },
    { FramebufferTexture3DEXT_names, FramebufferTexture3DEXT_remap_index, -1 },
    { FramebufferRenderbufferEXT_names, FramebufferRenderbufferEXT_remap_index, -1 },
    { FramebufferTexture1DEXT_names, FramebufferTexture1DEXT_remap_index, -1 },
    { BindFramebufferEXT_names, BindFramebufferEXT_remap_index, -1 },
    { GenRenderbuffersEXT_names, GenRenderbuffersEXT_remap_index, -1 },
    { IsFramebufferEXT_names, IsFramebufferEXT_remap_index, -1 },
    { FramebufferTexture2DEXT_names, FramebufferTexture2DEXT_remap_index, -1 },
    { GetFramebufferAttachmentParameterivEXT_names, GetFramebufferAttachmentParameterivEXT_remap_index, -1 },
    { DeleteFramebuffersEXT_names, DeleteFramebuffersEXT_remap_index, -1 },
    { GenFramebuffersEXT_names, GenFramebuffersEXT_remap_index, -1 },
    { BindRenderbufferEXT_names, BindRenderbufferEXT_remap_index, -1 },
    { GetRenderbufferParameterivEXT_names, GetRenderbufferParameterivEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_gpu_program_parameters)
static const struct dri_extension_function GL_EXT_gpu_program_parameters_functions[] = {
    { ProgramLocalParameters4fvEXT_names, ProgramLocalParameters4fvEXT_remap_index, -1 },
    { ProgramEnvParameters4fvEXT_names, ProgramEnvParameters4fvEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_histogram)
static const struct dri_extension_function GL_EXT_histogram_functions[] = {
    { Histogram_names, -1, 367 },
    { ResetHistogram_names, -1, 369 },
    { GetMinmax_names, -1, 364 },
    { GetHistogramParameterfv_names, -1, 362 },
    { GetMinmaxParameteriv_names, -1, 366 },
    { ResetMinmax_names, -1, 370 },
    { GetHistogramParameteriv_names, -1, 363 },
    { GetHistogram_names, -1, 361 },
    { Minmax_names, -1, 368 },
    { GetMinmaxParameterfv_names, -1, 365 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_index_func)
static const struct dri_extension_function GL_EXT_index_func_functions[] = {
    { IndexFuncEXT_names, IndexFuncEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_index_material)
static const struct dri_extension_function GL_EXT_index_material_functions[] = {
    { IndexMaterialEXT_names, IndexMaterialEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_light_texture)
static const struct dri_extension_function GL_EXT_light_texture_functions[] = {
    { ApplyTextureEXT_names, ApplyTextureEXT_remap_index, -1 },
    { TextureMaterialEXT_names, TextureMaterialEXT_remap_index, -1 },
    { TextureLightEXT_names, TextureLightEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_multi_draw_arrays)
static const struct dri_extension_function GL_EXT_multi_draw_arrays_functions[] = {
    { MultiDrawElementsEXT_names, MultiDrawElementsEXT_remap_index, -1 },
    { MultiDrawArraysEXT_names, MultiDrawArraysEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_multisample)
static const struct dri_extension_function GL_EXT_multisample_functions[] = {
    { SampleMaskSGIS_names, SampleMaskSGIS_remap_index, -1 },
    { SamplePatternSGIS_names, SamplePatternSGIS_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_paletted_texture)
static const struct dri_extension_function GL_EXT_paletted_texture_functions[] = {
    { ColorTable_names, -1, 339 },
    { GetColorTable_names, -1, 343 },
    { GetColorTableParameterfv_names, -1, 344 },
    { GetColorTableParameteriv_names, -1, 345 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_pixel_transform)
static const struct dri_extension_function GL_EXT_pixel_transform_functions[] = {
    { PixelTransformParameterfvEXT_names, PixelTransformParameterfvEXT_remap_index, -1 },
    { PixelTransformParameteriEXT_names, PixelTransformParameteriEXT_remap_index, -1 },
    { PixelTransformParameterfEXT_names, PixelTransformParameterfEXT_remap_index, -1 },
    { PixelTransformParameterivEXT_names, PixelTransformParameterivEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_point_parameters)
static const struct dri_extension_function GL_EXT_point_parameters_functions[] = {
    { PointParameterfEXT_names, PointParameterfEXT_remap_index, -1 },
    { PointParameterfvEXT_names, PointParameterfvEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_polygon_offset)
static const struct dri_extension_function GL_EXT_polygon_offset_functions[] = {
    { PolygonOffsetEXT_names, PolygonOffsetEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_secondary_color)
static const struct dri_extension_function GL_EXT_secondary_color_functions[] = {
    { SecondaryColor3iEXT_names, SecondaryColor3iEXT_remap_index, -1 },
    { SecondaryColor3bEXT_names, SecondaryColor3bEXT_remap_index, -1 },
    { SecondaryColor3bvEXT_names, SecondaryColor3bvEXT_remap_index, -1 },
    { SecondaryColor3sEXT_names, SecondaryColor3sEXT_remap_index, -1 },
    { SecondaryColor3dEXT_names, SecondaryColor3dEXT_remap_index, -1 },
    { SecondaryColorPointerEXT_names, SecondaryColorPointerEXT_remap_index, -1 },
    { SecondaryColor3uiEXT_names, SecondaryColor3uiEXT_remap_index, -1 },
    { SecondaryColor3usvEXT_names, SecondaryColor3usvEXT_remap_index, -1 },
    { SecondaryColor3ivEXT_names, SecondaryColor3ivEXT_remap_index, -1 },
    { SecondaryColor3fvEXT_names, SecondaryColor3fvEXT_remap_index, -1 },
    { SecondaryColor3ubvEXT_names, SecondaryColor3ubvEXT_remap_index, -1 },
    { SecondaryColor3uivEXT_names, SecondaryColor3uivEXT_remap_index, -1 },
    { SecondaryColor3dvEXT_names, SecondaryColor3dvEXT_remap_index, -1 },
    { SecondaryColor3usEXT_names, SecondaryColor3usEXT_remap_index, -1 },
    { SecondaryColor3ubEXT_names, SecondaryColor3ubEXT_remap_index, -1 },
    { SecondaryColor3fEXT_names, SecondaryColor3fEXT_remap_index, -1 },
    { SecondaryColor3svEXT_names, SecondaryColor3svEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_stencil_two_side)
static const struct dri_extension_function GL_EXT_stencil_two_side_functions[] = {
    { ActiveStencilFaceEXT_names, ActiveStencilFaceEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_subtexture)
static const struct dri_extension_function GL_EXT_subtexture_functions[] = {
    { TexSubImage1D_names, -1, 332 },
    { TexSubImage2D_names, -1, 333 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_texture3D)
static const struct dri_extension_function GL_EXT_texture3D_functions[] = {
    { TexImage3D_names, -1, 371 },
    { TexSubImage3D_names, -1, 372 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_texture_object)
static const struct dri_extension_function GL_EXT_texture_object_functions[] = {
    { PrioritizeTextures_names, -1, 331 },
    { AreTexturesResident_names, -1, 322 },
    { GenTextures_names, -1, 328 },
    { DeleteTextures_names, -1, 327 },
    { IsTexture_names, -1, 330 },
    { BindTexture_names, -1, 307 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_texture_perturb_normal)
static const struct dri_extension_function GL_EXT_texture_perturb_normal_functions[] = {
    { TextureNormalEXT_names, TextureNormalEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_timer_query)
static const struct dri_extension_function GL_EXT_timer_query_functions[] = {
    { GetQueryObjectui64vEXT_names, GetQueryObjectui64vEXT_remap_index, -1 },
    { GetQueryObjecti64vEXT_names, GetQueryObjecti64vEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_vertex_array)
static const struct dri_extension_function GL_EXT_vertex_array_functions[] = {
    { IndexPointerEXT_names, IndexPointerEXT_remap_index, -1 },
    { NormalPointerEXT_names, NormalPointerEXT_remap_index, -1 },
    { VertexPointerEXT_names, VertexPointerEXT_remap_index, -1 },
    { TexCoordPointerEXT_names, TexCoordPointerEXT_remap_index, -1 },
    { EdgeFlagPointerEXT_names, EdgeFlagPointerEXT_remap_index, -1 },
    { ArrayElement_names, -1, 306 },
    { ColorPointerEXT_names, ColorPointerEXT_remap_index, -1 },
    { GetPointerv_names, -1, 329 },
    { DrawArrays_names, -1, 310 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_EXT_vertex_weighting)
static const struct dri_extension_function GL_EXT_vertex_weighting_functions[] = {
    { VertexWeightfvEXT_names, VertexWeightfvEXT_remap_index, -1 },
    { VertexWeightfEXT_names, VertexWeightfEXT_remap_index, -1 },
    { VertexWeightPointerEXT_names, VertexWeightPointerEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_HP_image_transform)
static const struct dri_extension_function GL_HP_image_transform_functions[] = {
    { GetImageTransformParameterfvHP_names, GetImageTransformParameterfvHP_remap_index, -1 },
    { ImageTransformParameterfHP_names, ImageTransformParameterfHP_remap_index, -1 },
    { ImageTransformParameterfvHP_names, ImageTransformParameterfvHP_remap_index, -1 },
    { ImageTransformParameteriHP_names, ImageTransformParameteriHP_remap_index, -1 },
    { GetImageTransformParameterivHP_names, GetImageTransformParameterivHP_remap_index, -1 },
    { ImageTransformParameterivHP_names, ImageTransformParameterivHP_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_IBM_multimode_draw_arrays)
static const struct dri_extension_function GL_IBM_multimode_draw_arrays_functions[] = {
    { MultiModeDrawArraysIBM_names, MultiModeDrawArraysIBM_remap_index, -1 },
    { MultiModeDrawElementsIBM_names, MultiModeDrawElementsIBM_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_IBM_vertex_array_lists)
static const struct dri_extension_function GL_IBM_vertex_array_lists_functions[] = {
    { SecondaryColorPointerListIBM_names, SecondaryColorPointerListIBM_remap_index, -1 },
    { NormalPointerListIBM_names, NormalPointerListIBM_remap_index, -1 },
    { FogCoordPointerListIBM_names, FogCoordPointerListIBM_remap_index, -1 },
    { VertexPointerListIBM_names, VertexPointerListIBM_remap_index, -1 },
    { ColorPointerListIBM_names, ColorPointerListIBM_remap_index, -1 },
    { TexCoordPointerListIBM_names, TexCoordPointerListIBM_remap_index, -1 },
    { IndexPointerListIBM_names, IndexPointerListIBM_remap_index, -1 },
    { EdgeFlagPointerListIBM_names, EdgeFlagPointerListIBM_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_INGR_blend_func_separate)
static const struct dri_extension_function GL_INGR_blend_func_separate_functions[] = {
    { BlendFuncSeparateEXT_names, BlendFuncSeparateEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_INTEL_parallel_arrays)
static const struct dri_extension_function GL_INTEL_parallel_arrays_functions[] = {
    { VertexPointervINTEL_names, VertexPointervINTEL_remap_index, -1 },
    { ColorPointervINTEL_names, ColorPointervINTEL_remap_index, -1 },
    { NormalPointervINTEL_names, NormalPointervINTEL_remap_index, -1 },
    { TexCoordPointervINTEL_names, TexCoordPointervINTEL_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_MESA_resize_buffers)
static const struct dri_extension_function GL_MESA_resize_buffers_functions[] = {
    { ResizeBuffersMESA_names, ResizeBuffersMESA_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_MESA_shader_debug)
static const struct dri_extension_function GL_MESA_shader_debug_functions[] = {
    { GetDebugLogLengthMESA_names, GetDebugLogLengthMESA_remap_index, -1 },
    { ClearDebugLogMESA_names, ClearDebugLogMESA_remap_index, -1 },
    { GetDebugLogMESA_names, GetDebugLogMESA_remap_index, -1 },
    { CreateDebugObjectMESA_names, CreateDebugObjectMESA_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_MESA_window_pos)
static const struct dri_extension_function GL_MESA_window_pos_functions[] = {
    { WindowPos3fMESA_names, WindowPos3fMESA_remap_index, -1 },
    { WindowPos2dvMESA_names, WindowPos2dvMESA_remap_index, -1 },
    { WindowPos4svMESA_names, WindowPos4svMESA_remap_index, -1 },
    { WindowPos2svMESA_names, WindowPos2svMESA_remap_index, -1 },
    { WindowPos3dMESA_names, WindowPos3dMESA_remap_index, -1 },
    { WindowPos2fvMESA_names, WindowPos2fvMESA_remap_index, -1 },
    { WindowPos4dMESA_names, WindowPos4dMESA_remap_index, -1 },
    { WindowPos2dMESA_names, WindowPos2dMESA_remap_index, -1 },
    { WindowPos4ivMESA_names, WindowPos4ivMESA_remap_index, -1 },
    { WindowPos4fMESA_names, WindowPos4fMESA_remap_index, -1 },
    { WindowPos3dvMESA_names, WindowPos3dvMESA_remap_index, -1 },
    { WindowPos3fvMESA_names, WindowPos3fvMESA_remap_index, -1 },
    { WindowPos4dvMESA_names, WindowPos4dvMESA_remap_index, -1 },
    { WindowPos2iMESA_names, WindowPos2iMESA_remap_index, -1 },
    { WindowPos3sMESA_names, WindowPos3sMESA_remap_index, -1 },
    { WindowPos4sMESA_names, WindowPos4sMESA_remap_index, -1 },
    { WindowPos2ivMESA_names, WindowPos2ivMESA_remap_index, -1 },
    { WindowPos2sMESA_names, WindowPos2sMESA_remap_index, -1 },
    { WindowPos3iMESA_names, WindowPos3iMESA_remap_index, -1 },
    { WindowPos3ivMESA_names, WindowPos3ivMESA_remap_index, -1 },
    { WindowPos4iMESA_names, WindowPos4iMESA_remap_index, -1 },
    { WindowPos4fvMESA_names, WindowPos4fvMESA_remap_index, -1 },
    { WindowPos3svMESA_names, WindowPos3svMESA_remap_index, -1 },
    { WindowPos2fMESA_names, WindowPos2fMESA_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_NV_evaluators)
static const struct dri_extension_function GL_NV_evaluators_functions[] = {
    { GetMapAttribParameterivNV_names, GetMapAttribParameterivNV_remap_index, -1 },
    { MapControlPointsNV_names, MapControlPointsNV_remap_index, -1 },
    { MapParameterfvNV_names, MapParameterfvNV_remap_index, -1 },
    { EvalMapsNV_names, EvalMapsNV_remap_index, -1 },
    { GetMapAttribParameterfvNV_names, GetMapAttribParameterfvNV_remap_index, -1 },
    { MapParameterivNV_names, MapParameterivNV_remap_index, -1 },
    { GetMapParameterivNV_names, GetMapParameterivNV_remap_index, -1 },
    { GetMapParameterfvNV_names, GetMapParameterfvNV_remap_index, -1 },
    { GetMapControlPointsNV_names, GetMapControlPointsNV_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_NV_fence)
static const struct dri_extension_function GL_NV_fence_functions[] = {
    { GenFencesNV_names, GenFencesNV_remap_index, -1 },
    { TestFenceNV_names, TestFenceNV_remap_index, -1 },
    { IsFenceNV_names, IsFenceNV_remap_index, -1 },
    { DeleteFencesNV_names, DeleteFencesNV_remap_index, -1 },
    { SetFenceNV_names, SetFenceNV_remap_index, -1 },
    { GetFenceivNV_names, GetFenceivNV_remap_index, -1 },
    { FinishFenceNV_names, FinishFenceNV_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_NV_fragment_program)
static const struct dri_extension_function GL_NV_fragment_program_functions[] = {
    { GetProgramNamedParameterdvNV_names, GetProgramNamedParameterdvNV_remap_index, -1 },
    { GetProgramNamedParameterfvNV_names, GetProgramNamedParameterfvNV_remap_index, -1 },
    { ProgramNamedParameter4fNV_names, ProgramNamedParameter4fNV_remap_index, -1 },
    { ProgramNamedParameter4fvNV_names, ProgramNamedParameter4fvNV_remap_index, -1 },
    { ProgramNamedParameter4dvNV_names, ProgramNamedParameter4dvNV_remap_index, -1 },
    { ProgramNamedParameter4dNV_names, ProgramNamedParameter4dNV_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_NV_point_sprite)
static const struct dri_extension_function GL_NV_point_sprite_functions[] = {
    { PointParameteriNV_names, PointParameteriNV_remap_index, -1 },
    { PointParameterivNV_names, PointParameterivNV_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_NV_register_combiners)
static const struct dri_extension_function GL_NV_register_combiners_functions[] = {
    { CombinerOutputNV_names, CombinerOutputNV_remap_index, -1 },
    { CombinerParameterfvNV_names, CombinerParameterfvNV_remap_index, -1 },
    { GetCombinerOutputParameterfvNV_names, GetCombinerOutputParameterfvNV_remap_index, -1 },
    { FinalCombinerInputNV_names, FinalCombinerInputNV_remap_index, -1 },
    { GetCombinerInputParameterfvNV_names, GetCombinerInputParameterfvNV_remap_index, -1 },
    { GetCombinerOutputParameterivNV_names, GetCombinerOutputParameterivNV_remap_index, -1 },
    { CombinerParameteriNV_names, CombinerParameteriNV_remap_index, -1 },
    { GetFinalCombinerInputParameterivNV_names, GetFinalCombinerInputParameterivNV_remap_index, -1 },
    { CombinerInputNV_names, CombinerInputNV_remap_index, -1 },
    { CombinerParameterfNV_names, CombinerParameterfNV_remap_index, -1 },
    { GetFinalCombinerInputParameterfvNV_names, GetFinalCombinerInputParameterfvNV_remap_index, -1 },
    { GetCombinerInputParameterivNV_names, GetCombinerInputParameterivNV_remap_index, -1 },
    { CombinerParameterivNV_names, CombinerParameterivNV_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_NV_register_combiners2)
static const struct dri_extension_function GL_NV_register_combiners2_functions[] = {
    { CombinerStageParameterfvNV_names, CombinerStageParameterfvNV_remap_index, -1 },
    { GetCombinerStageParameterfvNV_names, GetCombinerStageParameterfvNV_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_NV_vertex_array_range)
static const struct dri_extension_function GL_NV_vertex_array_range_functions[] = {
    { FlushVertexArrayRangeNV_names, FlushVertexArrayRangeNV_remap_index, -1 },
    { VertexArrayRangeNV_names, VertexArrayRangeNV_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_NV_vertex_program)
static const struct dri_extension_function GL_NV_vertex_program_functions[] = {
    { ProgramParameter4fNV_names, ProgramParameter4fNV_remap_index, -1 },
    { VertexAttrib4ubvNV_names, VertexAttrib4ubvNV_remap_index, -1 },
    { VertexAttrib4svNV_names, VertexAttrib4svNV_remap_index, -1 },
    { VertexAttribs1dvNV_names, VertexAttribs1dvNV_remap_index, -1 },
    { VertexAttrib1fvNV_names, VertexAttrib1fvNV_remap_index, -1 },
    { ProgramParameter4dvNV_names, ProgramParameter4dvNV_remap_index, -1 },
    { VertexAttrib4fNV_names, VertexAttrib4fNV_remap_index, -1 },
    { VertexAttrib2dNV_names, VertexAttrib2dNV_remap_index, -1 },
    { VertexAttrib4ubNV_names, VertexAttrib4ubNV_remap_index, -1 },
    { VertexAttribs3dvNV_names, VertexAttribs3dvNV_remap_index, -1 },
    { VertexAttribs4fvNV_names, VertexAttribs4fvNV_remap_index, -1 },
    { VertexAttrib2sNV_names, VertexAttrib2sNV_remap_index, -1 },
    { VertexAttribs3fvNV_names, VertexAttribs3fvNV_remap_index, -1 },
    { ProgramParameter4dNV_names, ProgramParameter4dNV_remap_index, -1 },
    { LoadProgramNV_names, LoadProgramNV_remap_index, -1 },
    { VertexAttrib4fvNV_names, VertexAttrib4fvNV_remap_index, -1 },
    { VertexAttrib3fNV_names, VertexAttrib3fNV_remap_index, -1 },
    { VertexAttribs2dvNV_names, VertexAttribs2dvNV_remap_index, -1 },
    { GetProgramParameterfvNV_names, GetProgramParameterfvNV_remap_index, -1 },
    { VertexAttrib3dNV_names, VertexAttrib3dNV_remap_index, -1 },
    { VertexAttrib2fvNV_names, VertexAttrib2fvNV_remap_index, -1 },
    { VertexAttrib2dvNV_names, VertexAttrib2dvNV_remap_index, -1 },
    { VertexAttrib1dvNV_names, VertexAttrib1dvNV_remap_index, -1 },
    { ProgramParameter4fvNV_names, ProgramParameter4fvNV_remap_index, -1 },
    { VertexAttrib1svNV_names, VertexAttrib1svNV_remap_index, -1 },
    { VertexAttribs2svNV_names, VertexAttribs2svNV_remap_index, -1 },
    { GetVertexAttribivNV_names, GetVertexAttribivNV_remap_index, -1 },
    { GetVertexAttribfvNV_names, GetVertexAttribfvNV_remap_index, -1 },
    { VertexAttrib2svNV_names, VertexAttrib2svNV_remap_index, -1 },
    { VertexAttribs1fvNV_names, VertexAttribs1fvNV_remap_index, -1 },
    { IsProgramNV_names, IsProgramNV_remap_index, -1 },
    { VertexAttrib2fNV_names, VertexAttrib2fNV_remap_index, -1 },
    { RequestResidentProgramsNV_names, RequestResidentProgramsNV_remap_index, -1 },
    { ExecuteProgramNV_names, ExecuteProgramNV_remap_index, -1 },
    { VertexAttribPointerNV_names, VertexAttribPointerNV_remap_index, -1 },
    { TrackMatrixNV_names, TrackMatrixNV_remap_index, -1 },
    { GetProgramParameterdvNV_names, GetProgramParameterdvNV_remap_index, -1 },
    { GetTrackMatrixivNV_names, GetTrackMatrixivNV_remap_index, -1 },
    { VertexAttrib3svNV_names, VertexAttrib3svNV_remap_index, -1 },
    { ProgramParameters4fvNV_names, ProgramParameters4fvNV_remap_index, -1 },
    { GetProgramivNV_names, GetProgramivNV_remap_index, -1 },
    { GetVertexAttribdvNV_names, GetVertexAttribdvNV_remap_index, -1 },
    { VertexAttrib3fvNV_names, VertexAttrib3fvNV_remap_index, -1 },
    { VertexAttribs2fvNV_names, VertexAttribs2fvNV_remap_index, -1 },
    { DeleteProgramsNV_names, DeleteProgramsNV_remap_index, -1 },
    { GetVertexAttribPointervNV_names, GetVertexAttribPointervNV_remap_index, -1 },
    { GetProgramStringNV_names, GetProgramStringNV_remap_index, -1 },
    { VertexAttrib4sNV_names, VertexAttrib4sNV_remap_index, -1 },
    { VertexAttribs4dvNV_names, VertexAttribs4dvNV_remap_index, -1 },
    { ProgramParameters4dvNV_names, ProgramParameters4dvNV_remap_index, -1 },
    { VertexAttrib3sNV_names, VertexAttrib3sNV_remap_index, -1 },
    { VertexAttrib1fNV_names, VertexAttrib1fNV_remap_index, -1 },
    { VertexAttrib4dNV_names, VertexAttrib4dNV_remap_index, -1 },
    { VertexAttribs4ubvNV_names, VertexAttribs4ubvNV_remap_index, -1 },
    { VertexAttribs3svNV_names, VertexAttribs3svNV_remap_index, -1 },
    { VertexAttrib1sNV_names, VertexAttrib1sNV_remap_index, -1 },
    { BindProgramNV_names, BindProgramNV_remap_index, -1 },
    { AreProgramsResidentNV_names, AreProgramsResidentNV_remap_index, -1 },
    { VertexAttrib3dvNV_names, VertexAttrib3dvNV_remap_index, -1 },
    { VertexAttrib1dNV_names, VertexAttrib1dNV_remap_index, -1 },
    { VertexAttribs4svNV_names, VertexAttribs4svNV_remap_index, -1 },
    { VertexAttribs1svNV_names, VertexAttribs1svNV_remap_index, -1 },
    { GenProgramsNV_names, GenProgramsNV_remap_index, -1 },
    { VertexAttrib4dvNV_names, VertexAttrib4dvNV_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_PGI_misc_hints)
static const struct dri_extension_function GL_PGI_misc_hints_functions[] = {
    { HintPGI_names, HintPGI_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIS_detail_texture)
static const struct dri_extension_function GL_SGIS_detail_texture_functions[] = {
    { GetDetailTexFuncSGIS_names, GetDetailTexFuncSGIS_remap_index, -1 },
    { DetailTexFuncSGIS_names, DetailTexFuncSGIS_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIS_fog_function)
static const struct dri_extension_function GL_SGIS_fog_function_functions[] = {
    { FogFuncSGIS_names, FogFuncSGIS_remap_index, -1 },
    { GetFogFuncSGIS_names, GetFogFuncSGIS_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIS_multisample)
static const struct dri_extension_function GL_SGIS_multisample_functions[] = {
    { SampleMaskSGIS_names, SampleMaskSGIS_remap_index, -1 },
    { SamplePatternSGIS_names, SamplePatternSGIS_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIS_pixel_texture)
static const struct dri_extension_function GL_SGIS_pixel_texture_functions[] = {
    { PixelTexGenParameterfvSGIS_names, PixelTexGenParameterfvSGIS_remap_index, -1 },
    { GetPixelTexGenParameterivSGIS_names, GetPixelTexGenParameterivSGIS_remap_index, -1 },
    { PixelTexGenParameteriSGIS_names, PixelTexGenParameteriSGIS_remap_index, -1 },
    { PixelTexGenParameterivSGIS_names, PixelTexGenParameterivSGIS_remap_index, -1 },
    { PixelTexGenParameterfSGIS_names, PixelTexGenParameterfSGIS_remap_index, -1 },
    { GetPixelTexGenParameterfvSGIS_names, GetPixelTexGenParameterfvSGIS_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIS_point_parameters)
static const struct dri_extension_function GL_SGIS_point_parameters_functions[] = {
    { PointParameterfEXT_names, PointParameterfEXT_remap_index, -1 },
    { PointParameterfvEXT_names, PointParameterfvEXT_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIS_sharpen_texture)
static const struct dri_extension_function GL_SGIS_sharpen_texture_functions[] = {
    { GetSharpenTexFuncSGIS_names, GetSharpenTexFuncSGIS_remap_index, -1 },
    { SharpenTexFuncSGIS_names, SharpenTexFuncSGIS_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIS_texture4D)
static const struct dri_extension_function GL_SGIS_texture4D_functions[] = {
    { TexImage4DSGIS_names, TexImage4DSGIS_remap_index, -1 },
    { TexSubImage4DSGIS_names, TexSubImage4DSGIS_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIS_texture_color_mask)
static const struct dri_extension_function GL_SGIS_texture_color_mask_functions[] = {
    { TextureColorMaskSGIS_names, TextureColorMaskSGIS_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIS_texture_filter4)
static const struct dri_extension_function GL_SGIS_texture_filter4_functions[] = {
    { GetTexFilterFuncSGIS_names, GetTexFilterFuncSGIS_remap_index, -1 },
    { TexFilterFuncSGIS_names, TexFilterFuncSGIS_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIX_async)
static const struct dri_extension_function GL_SGIX_async_functions[] = {
    { AsyncMarkerSGIX_names, AsyncMarkerSGIX_remap_index, -1 },
    { FinishAsyncSGIX_names, FinishAsyncSGIX_remap_index, -1 },
    { PollAsyncSGIX_names, PollAsyncSGIX_remap_index, -1 },
    { DeleteAsyncMarkersSGIX_names, DeleteAsyncMarkersSGIX_remap_index, -1 },
    { IsAsyncMarkerSGIX_names, IsAsyncMarkerSGIX_remap_index, -1 },
    { GenAsyncMarkersSGIX_names, GenAsyncMarkersSGIX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIX_flush_raster)
static const struct dri_extension_function GL_SGIX_flush_raster_functions[] = {
    { FlushRasterSGIX_names, FlushRasterSGIX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const struct dri_extension_function GL_SGIX_fragment_lighting_functions[] = {
    { FragmentMaterialfvSGIX_names, FragmentMaterialfvSGIX_remap_index, -1 },
    { FragmentLightModelivSGIX_names, FragmentLightModelivSGIX_remap_index, -1 },
    { FragmentLightiSGIX_names, FragmentLightiSGIX_remap_index, -1 },
    { GetFragmentMaterialfvSGIX_names, GetFragmentMaterialfvSGIX_remap_index, -1 },
    { FragmentMaterialfSGIX_names, FragmentMaterialfSGIX_remap_index, -1 },
    { GetFragmentLightivSGIX_names, GetFragmentLightivSGIX_remap_index, -1 },
    { FragmentLightModeliSGIX_names, FragmentLightModeliSGIX_remap_index, -1 },
    { FragmentLightivSGIX_names, FragmentLightivSGIX_remap_index, -1 },
    { GetFragmentMaterialivSGIX_names, GetFragmentMaterialivSGIX_remap_index, -1 },
    { FragmentLightModelfSGIX_names, FragmentLightModelfSGIX_remap_index, -1 },
    { FragmentColorMaterialSGIX_names, FragmentColorMaterialSGIX_remap_index, -1 },
    { FragmentMaterialiSGIX_names, FragmentMaterialiSGIX_remap_index, -1 },
    { LightEnviSGIX_names, LightEnviSGIX_remap_index, -1 },
    { FragmentLightModelfvSGIX_names, FragmentLightModelfvSGIX_remap_index, -1 },
    { FragmentLightfvSGIX_names, FragmentLightfvSGIX_remap_index, -1 },
    { FragmentLightfSGIX_names, FragmentLightfSGIX_remap_index, -1 },
    { GetFragmentLightfvSGIX_names, GetFragmentLightfvSGIX_remap_index, -1 },
    { FragmentMaterialivSGIX_names, FragmentMaterialivSGIX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIX_framezoom)
static const struct dri_extension_function GL_SGIX_framezoom_functions[] = {
    { FrameZoomSGIX_names, FrameZoomSGIX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIX_igloo_interface)
static const struct dri_extension_function GL_SGIX_igloo_interface_functions[] = {
    { IglooInterfaceSGIX_names, IglooInterfaceSGIX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIX_instruments)
static const struct dri_extension_function GL_SGIX_instruments_functions[] = {
    { ReadInstrumentsSGIX_names, ReadInstrumentsSGIX_remap_index, -1 },
    { GetInstrumentsSGIX_names, GetInstrumentsSGIX_remap_index, -1 },
    { StartInstrumentsSGIX_names, StartInstrumentsSGIX_remap_index, -1 },
    { StopInstrumentsSGIX_names, StopInstrumentsSGIX_remap_index, -1 },
    { InstrumentsBufferSGIX_names, InstrumentsBufferSGIX_remap_index, -1 },
    { PollInstrumentsSGIX_names, PollInstrumentsSGIX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIX_list_priority)
static const struct dri_extension_function GL_SGIX_list_priority_functions[] = {
    { ListParameterfSGIX_names, ListParameterfSGIX_remap_index, -1 },
    { GetListParameterivSGIX_names, GetListParameterivSGIX_remap_index, -1 },
    { GetListParameterfvSGIX_names, GetListParameterfvSGIX_remap_index, -1 },
    { ListParameteriSGIX_names, ListParameteriSGIX_remap_index, -1 },
    { ListParameterfvSGIX_names, ListParameterfvSGIX_remap_index, -1 },
    { ListParameterivSGIX_names, ListParameterivSGIX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIX_pixel_texture)
static const struct dri_extension_function GL_SGIX_pixel_texture_functions[] = {
    { PixelTexGenSGIX_names, PixelTexGenSGIX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIX_polynomial_ffd)
static const struct dri_extension_function GL_SGIX_polynomial_ffd_functions[] = {
    { LoadIdentityDeformationMapSGIX_names, LoadIdentityDeformationMapSGIX_remap_index, -1 },
    { DeformationMap3dSGIX_names, DeformationMap3dSGIX_remap_index, -1 },
    { DeformSGIX_names, DeformSGIX_remap_index, -1 },
    { DeformationMap3fSGIX_names, DeformationMap3fSGIX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIX_reference_plane)
static const struct dri_extension_function GL_SGIX_reference_plane_functions[] = {
    { ReferencePlaneSGIX_names, ReferencePlaneSGIX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIX_sprite)
static const struct dri_extension_function GL_SGIX_sprite_functions[] = {
    { SpriteParameterfvSGIX_names, SpriteParameterfvSGIX_remap_index, -1 },
    { SpriteParameteriSGIX_names, SpriteParameteriSGIX_remap_index, -1 },
    { SpriteParameterfSGIX_names, SpriteParameterfSGIX_remap_index, -1 },
    { SpriteParameterivSGIX_names, SpriteParameterivSGIX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGIX_tag_sample_buffer)
static const struct dri_extension_function GL_SGIX_tag_sample_buffer_functions[] = {
    { TagSampleBufferSGIX_names, TagSampleBufferSGIX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SGI_color_table)
static const struct dri_extension_function GL_SGI_color_table_functions[] = {
    { ColorTableParameteriv_names, -1, 341 },
    { ColorTable_names, -1, 339 },
    { GetColorTable_names, -1, 343 },
    { CopyColorTable_names, -1, 342 },
    { ColorTableParameterfv_names, -1, 340 },
    { GetColorTableParameterfv_names, -1, 344 },
    { GetColorTableParameteriv_names, -1, 345 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SUNX_constant_data)
static const struct dri_extension_function GL_SUNX_constant_data_functions[] = {
    { FinishTextureSUNX_names, FinishTextureSUNX_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SUN_global_alpha)
static const struct dri_extension_function GL_SUN_global_alpha_functions[] = {
    { GlobalAlphaFactorubSUN_names, GlobalAlphaFactorubSUN_remap_index, -1 },
    { GlobalAlphaFactoriSUN_names, GlobalAlphaFactoriSUN_remap_index, -1 },
    { GlobalAlphaFactordSUN_names, GlobalAlphaFactordSUN_remap_index, -1 },
    { GlobalAlphaFactoruiSUN_names, GlobalAlphaFactoruiSUN_remap_index, -1 },
    { GlobalAlphaFactorbSUN_names, GlobalAlphaFactorbSUN_remap_index, -1 },
    { GlobalAlphaFactorfSUN_names, GlobalAlphaFactorfSUN_remap_index, -1 },
    { GlobalAlphaFactorusSUN_names, GlobalAlphaFactorusSUN_remap_index, -1 },
    { GlobalAlphaFactorsSUN_names, GlobalAlphaFactorsSUN_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SUN_mesh_array)
static const struct dri_extension_function GL_SUN_mesh_array_functions[] = {
    { DrawMeshArraysSUN_names, DrawMeshArraysSUN_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SUN_triangle_list)
static const struct dri_extension_function GL_SUN_triangle_list_functions[] = {
    { ReplacementCodeubSUN_names, ReplacementCodeubSUN_remap_index, -1 },
    { ReplacementCodeubvSUN_names, ReplacementCodeubvSUN_remap_index, -1 },
    { ReplacementCodeusvSUN_names, ReplacementCodeusvSUN_remap_index, -1 },
    { ReplacementCodePointerSUN_names, ReplacementCodePointerSUN_remap_index, -1 },
    { ReplacementCodeusSUN_names, ReplacementCodeusSUN_remap_index, -1 },
    { ReplacementCodeuiSUN_names, ReplacementCodeuiSUN_remap_index, -1 },
    { ReplacementCodeuivSUN_names, ReplacementCodeuivSUN_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_SUN_vertex)
static const struct dri_extension_function GL_SUN_vertex_functions[] = {
    { ReplacementCodeuiColor3fVertex3fvSUN_names, ReplacementCodeuiColor3fVertex3fvSUN_remap_index, -1 },
    { TexCoord4fColor4fNormal3fVertex4fvSUN_names, TexCoord4fColor4fNormal3fVertex4fvSUN_remap_index, -1 },
    { TexCoord2fColor4ubVertex3fvSUN_names, TexCoord2fColor4ubVertex3fvSUN_remap_index, -1 },
    { ReplacementCodeuiVertex3fvSUN_names, ReplacementCodeuiVertex3fvSUN_remap_index, -1 },
    { ReplacementCodeuiTexCoord2fVertex3fvSUN_names, ReplacementCodeuiTexCoord2fVertex3fvSUN_remap_index, -1 },
    { ReplacementCodeuiNormal3fVertex3fSUN_names, ReplacementCodeuiNormal3fVertex3fSUN_remap_index, -1 },
    { Color4ubVertex3fvSUN_names, Color4ubVertex3fvSUN_remap_index, -1 },
    { Color4ubVertex3fSUN_names, Color4ubVertex3fSUN_remap_index, -1 },
    { TexCoord2fVertex3fSUN_names, TexCoord2fVertex3fSUN_remap_index, -1 },
    { TexCoord2fColor4fNormal3fVertex3fSUN_names, TexCoord2fColor4fNormal3fVertex3fSUN_remap_index, -1 },
    { TexCoord2fNormal3fVertex3fvSUN_names, TexCoord2fNormal3fVertex3fvSUN_remap_index, -1 },
    { ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN_names, ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN_remap_index, -1 },
    { ReplacementCodeuiTexCoord2fVertex3fSUN_names, ReplacementCodeuiTexCoord2fVertex3fSUN_remap_index, -1 },
    { TexCoord2fNormal3fVertex3fSUN_names, TexCoord2fNormal3fVertex3fSUN_remap_index, -1 },
    { Color3fVertex3fSUN_names, Color3fVertex3fSUN_remap_index, -1 },
    { Color3fVertex3fvSUN_names, Color3fVertex3fvSUN_remap_index, -1 },
    { Color4fNormal3fVertex3fvSUN_names, Color4fNormal3fVertex3fvSUN_remap_index, -1 },
    { ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN_names, ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN_remap_index, -1 },
    { ReplacementCodeuiColor4fNormal3fVertex3fvSUN_names, ReplacementCodeuiColor4fNormal3fVertex3fvSUN_remap_index, -1 },
    { ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN_names, ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN_remap_index, -1 },
    { TexCoord2fColor3fVertex3fSUN_names, TexCoord2fColor3fVertex3fSUN_remap_index, -1 },
    { TexCoord4fColor4fNormal3fVertex4fSUN_names, TexCoord4fColor4fNormal3fVertex4fSUN_remap_index, -1 },
    { Color4ubVertex2fvSUN_names, Color4ubVertex2fvSUN_remap_index, -1 },
    { Normal3fVertex3fSUN_names, Normal3fVertex3fSUN_remap_index, -1 },
    { ReplacementCodeuiColor4fNormal3fVertex3fSUN_names, ReplacementCodeuiColor4fNormal3fVertex3fSUN_remap_index, -1 },
    { TexCoord2fColor4fNormal3fVertex3fvSUN_names, TexCoord2fColor4fNormal3fVertex3fvSUN_remap_index, -1 },
    { TexCoord2fVertex3fvSUN_names, TexCoord2fVertex3fvSUN_remap_index, -1 },
    { Color4ubVertex2fSUN_names, Color4ubVertex2fSUN_remap_index, -1 },
    { ReplacementCodeuiColor4ubVertex3fSUN_names, ReplacementCodeuiColor4ubVertex3fSUN_remap_index, -1 },
    { TexCoord2fColor4ubVertex3fSUN_names, TexCoord2fColor4ubVertex3fSUN_remap_index, -1 },
    { Normal3fVertex3fvSUN_names, Normal3fVertex3fvSUN_remap_index, -1 },
    { Color4fNormal3fVertex3fSUN_names, Color4fNormal3fVertex3fSUN_remap_index, -1 },
    { ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN_names, ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN_remap_index, -1 },
    { ReplacementCodeuiColor4ubVertex3fvSUN_names, ReplacementCodeuiColor4ubVertex3fvSUN_remap_index, -1 },
    { ReplacementCodeuiColor3fVertex3fSUN_names, ReplacementCodeuiColor3fVertex3fSUN_remap_index, -1 },
    { TexCoord4fVertex4fSUN_names, TexCoord4fVertex4fSUN_remap_index, -1 },
    { TexCoord2fColor3fVertex3fvSUN_names, TexCoord2fColor3fVertex3fvSUN_remap_index, -1 },
    { ReplacementCodeuiNormal3fVertex3fvSUN_names, ReplacementCodeuiNormal3fVertex3fvSUN_remap_index, -1 },
    { TexCoord4fVertex4fvSUN_names, TexCoord4fVertex4fvSUN_remap_index, -1 },
    { ReplacementCodeuiVertex3fSUN_names, ReplacementCodeuiVertex3fSUN_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_VERSION_1_3)
static const struct dri_extension_function GL_VERSION_1_3_functions[] = {
    { SampleCoverageARB_names, SampleCoverageARB_remap_index, -1 },
    { MultiTexCoord3sARB_names, -1, 398 },
    { ActiveTextureARB_names, -1, 374 },
    { CompressedTexSubImage2DARB_names, CompressedTexSubImage2DARB_remap_index, -1 },
    { CompressedTexImage3DARB_names, CompressedTexImage3DARB_remap_index, -1 },
    { MultiTexCoord1fvARB_names, -1, 379 },
    { MultTransposeMatrixdARB_names, MultTransposeMatrixdARB_remap_index, -1 },
    { CompressedTexImage1DARB_names, CompressedTexImage1DARB_remap_index, -1 },
    { MultiTexCoord3dARB_names, -1, 392 },
    { MultiTexCoord2iARB_names, -1, 388 },
    { MultiTexCoord2svARB_names, -1, 391 },
    { MultiTexCoord2fARB_names, -1, 386 },
    { LoadTransposeMatrixdARB_names, LoadTransposeMatrixdARB_remap_index, -1 },
    { MultiTexCoord3fvARB_names, -1, 395 },
    { MultiTexCoord4sARB_names, -1, 406 },
    { MultiTexCoord2dvARB_names, -1, 385 },
    { MultiTexCoord1svARB_names, -1, 383 },
    { MultiTexCoord3svARB_names, -1, 399 },
    { MultiTexCoord4iARB_names, -1, 404 },
    { MultiTexCoord3iARB_names, -1, 396 },
    { MultiTexCoord1dARB_names, -1, 376 },
    { MultiTexCoord3dvARB_names, -1, 393 },
    { MultiTexCoord3ivARB_names, -1, 397 },
    { MultiTexCoord2sARB_names, -1, 390 },
    { MultiTexCoord4ivARB_names, -1, 405 },
    { CompressedTexSubImage1DARB_names, CompressedTexSubImage1DARB_remap_index, -1 },
    { ClientActiveTextureARB_names, -1, 375 },
    { CompressedTexSubImage3DARB_names, CompressedTexSubImage3DARB_remap_index, -1 },
    { MultiTexCoord2dARB_names, -1, 384 },
    { MultiTexCoord4dvARB_names, -1, 401 },
    { MultiTexCoord4fvARB_names, -1, 403 },
    { MultiTexCoord3fARB_names, -1, 394 },
    { MultTransposeMatrixfARB_names, MultTransposeMatrixfARB_remap_index, -1 },
    { CompressedTexImage2DARB_names, CompressedTexImage2DARB_remap_index, -1 },
    { MultiTexCoord4dARB_names, -1, 400 },
    { MultiTexCoord1sARB_names, -1, 382 },
    { MultiTexCoord1dvARB_names, -1, 377 },
    { MultiTexCoord1ivARB_names, -1, 381 },
    { MultiTexCoord2ivARB_names, -1, 389 },
    { MultiTexCoord1iARB_names, -1, 380 },
    { GetCompressedTexImageARB_names, GetCompressedTexImageARB_remap_index, -1 },
    { MultiTexCoord4svARB_names, -1, 407 },
    { MultiTexCoord1fARB_names, -1, 378 },
    { MultiTexCoord4fARB_names, -1, 402 },
    { LoadTransposeMatrixfARB_names, LoadTransposeMatrixfARB_remap_index, -1 },
    { MultiTexCoord2fvARB_names, -1, 387 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_VERSION_1_4)
static const struct dri_extension_function GL_VERSION_1_4_functions[] = {
    { PointParameteriNV_names, PointParameteriNV_remap_index, -1 },
    { SecondaryColor3iEXT_names, SecondaryColor3iEXT_remap_index, -1 },
    { WindowPos3fMESA_names, WindowPos3fMESA_remap_index, -1 },
    { WindowPos2dvMESA_names, WindowPos2dvMESA_remap_index, -1 },
    { SecondaryColor3bEXT_names, SecondaryColor3bEXT_remap_index, -1 },
    { PointParameterfEXT_names, PointParameterfEXT_remap_index, -1 },
    { FogCoorddEXT_names, FogCoorddEXT_remap_index, -1 },
    { FogCoordfEXT_names, FogCoordfEXT_remap_index, -1 },
    { WindowPos2svMESA_names, WindowPos2svMESA_remap_index, -1 },
    { WindowPos3dMESA_names, WindowPos3dMESA_remap_index, -1 },
    { PointParameterfvEXT_names, PointParameterfvEXT_remap_index, -1 },
    { WindowPos2fvMESA_names, WindowPos2fvMESA_remap_index, -1 },
    { SecondaryColor3bvEXT_names, SecondaryColor3bvEXT_remap_index, -1 },
    { SecondaryColor3sEXT_names, SecondaryColor3sEXT_remap_index, -1 },
    { SecondaryColor3dEXT_names, SecondaryColor3dEXT_remap_index, -1 },
    { WindowPos2dMESA_names, WindowPos2dMESA_remap_index, -1 },
    { SecondaryColorPointerEXT_names, SecondaryColorPointerEXT_remap_index, -1 },
    { SecondaryColor3uiEXT_names, SecondaryColor3uiEXT_remap_index, -1 },
    { SecondaryColor3usvEXT_names, SecondaryColor3usvEXT_remap_index, -1 },
    { WindowPos3dvMESA_names, WindowPos3dvMESA_remap_index, -1 },
    { PointParameterivNV_names, PointParameterivNV_remap_index, -1 },
    { WindowPos3fvMESA_names, WindowPos3fvMESA_remap_index, -1 },
    { SecondaryColor3ivEXT_names, SecondaryColor3ivEXT_remap_index, -1 },
    { WindowPos2iMESA_names, WindowPos2iMESA_remap_index, -1 },
    { SecondaryColor3fvEXT_names, SecondaryColor3fvEXT_remap_index, -1 },
    { WindowPos3sMESA_names, WindowPos3sMESA_remap_index, -1 },
    { WindowPos2ivMESA_names, WindowPos2ivMESA_remap_index, -1 },
    { MultiDrawElementsEXT_names, MultiDrawElementsEXT_remap_index, -1 },
    { WindowPos2sMESA_names, WindowPos2sMESA_remap_index, -1 },
    { FogCoordPointerEXT_names, FogCoordPointerEXT_remap_index, -1 },
    { SecondaryColor3ubvEXT_names, SecondaryColor3ubvEXT_remap_index, -1 },
    { SecondaryColor3uivEXT_names, SecondaryColor3uivEXT_remap_index, -1 },
    { WindowPos3iMESA_names, WindowPos3iMESA_remap_index, -1 },
    { SecondaryColor3dvEXT_names, SecondaryColor3dvEXT_remap_index, -1 },
    { MultiDrawArraysEXT_names, MultiDrawArraysEXT_remap_index, -1 },
    { SecondaryColor3usEXT_names, SecondaryColor3usEXT_remap_index, -1 },
    { FogCoordfvEXT_names, FogCoordfvEXT_remap_index, -1 },
    { SecondaryColor3ubEXT_names, SecondaryColor3ubEXT_remap_index, -1 },
    { BlendFuncSeparateEXT_names, BlendFuncSeparateEXT_remap_index, -1 },
    { SecondaryColor3fEXT_names, SecondaryColor3fEXT_remap_index, -1 },
    { WindowPos3ivMESA_names, WindowPos3ivMESA_remap_index, -1 },
    { SecondaryColor3svEXT_names, SecondaryColor3svEXT_remap_index, -1 },
    { FogCoorddvEXT_names, FogCoorddvEXT_remap_index, -1 },
    { WindowPos3svMESA_names, WindowPos3svMESA_remap_index, -1 },
    { WindowPos2fMESA_names, WindowPos2fMESA_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_VERSION_1_5)
static const struct dri_extension_function GL_VERSION_1_5_functions[] = {
    { BeginQueryARB_names, BeginQueryARB_remap_index, -1 },
    { GetBufferSubDataARB_names, GetBufferSubDataARB_remap_index, -1 },
    { BufferSubDataARB_names, BufferSubDataARB_remap_index, -1 },
    { GetQueryivARB_names, GetQueryivARB_remap_index, -1 },
    { GetQueryObjectivARB_names, GetQueryObjectivARB_remap_index, -1 },
    { BufferDataARB_names, BufferDataARB_remap_index, -1 },
    { EndQueryARB_names, EndQueryARB_remap_index, -1 },
    { GetBufferPointervARB_names, GetBufferPointervARB_remap_index, -1 },
    { GetQueryObjectuivARB_names, GetQueryObjectuivARB_remap_index, -1 },
    { GetBufferParameterivARB_names, GetBufferParameterivARB_remap_index, -1 },
    { DeleteQueriesARB_names, DeleteQueriesARB_remap_index, -1 },
    { IsQueryARB_names, IsQueryARB_remap_index, -1 },
    { MapBufferARB_names, MapBufferARB_remap_index, -1 },
    { GenQueriesARB_names, GenQueriesARB_remap_index, -1 },
    { IsBufferARB_names, IsBufferARB_remap_index, -1 },
    { DeleteBuffersARB_names, DeleteBuffersARB_remap_index, -1 },
    { UnmapBufferARB_names, UnmapBufferARB_remap_index, -1 },
    { BindBufferARB_names, BindBufferARB_remap_index, -1 },
    { GenBuffersARB_names, GenBuffersARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_VERSION_2_0)
static const struct dri_extension_function GL_VERSION_2_0_functions[] = {
    { UniformMatrix3fvARB_names, UniformMatrix3fvARB_remap_index, -1 },
    { GetProgramiv_names, GetProgramiv_remap_index, -1 },
    { BlendEquationSeparateEXT_names, BlendEquationSeparateEXT_remap_index, -1 },
    { AttachShader_names, AttachShader_remap_index, -1 },
    { VertexAttrib2fARB_names, VertexAttrib2fARB_remap_index, -1 },
    { VertexAttrib3fARB_names, VertexAttrib3fARB_remap_index, -1 },
    { Uniform2fARB_names, Uniform2fARB_remap_index, -1 },
    { VertexAttrib1svARB_names, VertexAttrib1svARB_remap_index, -1 },
    { Uniform2ivARB_names, Uniform2ivARB_remap_index, -1 },
    { UniformMatrix4fvARB_names, UniformMatrix4fvARB_remap_index, -1 },
    { VertexAttrib4NusvARB_names, VertexAttrib4NusvARB_remap_index, -1 },
    { DisableVertexAttribArrayARB_names, DisableVertexAttribArrayARB_remap_index, -1 },
    { StencilMaskSeparate_names, StencilMaskSeparate_remap_index, -1 },
    { VertexAttrib1fARB_names, VertexAttrib1fARB_remap_index, -1 },
    { GetProgramInfoLog_names, GetProgramInfoLog_remap_index, -1 },
    { VertexAttrib4NbvARB_names, VertexAttrib4NbvARB_remap_index, -1 },
    { GetActiveAttribARB_names, GetActiveAttribARB_remap_index, -1 },
    { Uniform3iARB_names, Uniform3iARB_remap_index, -1 },
    { GetShaderInfoLog_names, GetShaderInfoLog_remap_index, -1 },
    { VertexAttrib1sARB_names, VertexAttrib1sARB_remap_index, -1 },
    { Uniform1fARB_names, Uniform1fARB_remap_index, -1 },
    { StencilOpSeparate_names, StencilOpSeparate_remap_index, -1 },
    { UniformMatrix2fvARB_names, UniformMatrix2fvARB_remap_index, -1 },
    { VertexAttrib3dvARB_names, VertexAttrib3dvARB_remap_index, -1 },
    { Uniform3fvARB_names, Uniform3fvARB_remap_index, -1 },
    { GetVertexAttribivARB_names, GetVertexAttribivARB_remap_index, -1 },
    { CreateProgram_names, CreateProgram_remap_index, -1 },
    { StencilFuncSeparate_names, StencilFuncSeparate_remap_index, -1 },
    { VertexAttrib4ivARB_names, VertexAttrib4ivARB_remap_index, -1 },
    { VertexAttrib4bvARB_names, VertexAttrib4bvARB_remap_index, -1 },
    { VertexAttrib3dARB_names, VertexAttrib3dARB_remap_index, -1 },
    { VertexAttrib4fARB_names, VertexAttrib4fARB_remap_index, -1 },
    { VertexAttrib4fvARB_names, VertexAttrib4fvARB_remap_index, -1 },
    { GetActiveUniformARB_names, GetActiveUniformARB_remap_index, -1 },
    { IsShader_names, IsShader_remap_index, -1 },
    { GetUniformivARB_names, GetUniformivARB_remap_index, -1 },
    { IsProgram_names, IsProgram_remap_index, -1 },
    { Uniform2fvARB_names, Uniform2fvARB_remap_index, -1 },
    { ValidateProgramARB_names, ValidateProgramARB_remap_index, -1 },
    { VertexAttrib2dARB_names, VertexAttrib2dARB_remap_index, -1 },
    { VertexAttrib1dvARB_names, VertexAttrib1dvARB_remap_index, -1 },
    { GetVertexAttribfvARB_names, GetVertexAttribfvARB_remap_index, -1 },
    { GetAttribLocationARB_names, GetAttribLocationARB_remap_index, -1 },
    { VertexAttrib4ubvARB_names, VertexAttrib4ubvARB_remap_index, -1 },
    { Uniform3ivARB_names, Uniform3ivARB_remap_index, -1 },
    { VertexAttrib4sARB_names, VertexAttrib4sARB_remap_index, -1 },
    { VertexAttrib2dvARB_names, VertexAttrib2dvARB_remap_index, -1 },
    { VertexAttrib2fvARB_names, VertexAttrib2fvARB_remap_index, -1 },
    { VertexAttrib4NivARB_names, VertexAttrib4NivARB_remap_index, -1 },
    { GetAttachedShaders_names, GetAttachedShaders_remap_index, -1 },
    { CompileShaderARB_names, CompileShaderARB_remap_index, -1 },
    { DeleteShader_names, DeleteShader_remap_index, -1 },
    { Uniform3fARB_names, Uniform3fARB_remap_index, -1 },
    { VertexAttrib4NuivARB_names, VertexAttrib4NuivARB_remap_index, -1 },
    { Uniform4fARB_names, Uniform4fARB_remap_index, -1 },
    { VertexAttrib1dARB_names, VertexAttrib1dARB_remap_index, -1 },
    { VertexAttrib4usvARB_names, VertexAttrib4usvARB_remap_index, -1 },
    { LinkProgramARB_names, LinkProgramARB_remap_index, -1 },
    { ShaderSourceARB_names, ShaderSourceARB_remap_index, -1 },
    { VertexAttrib3svARB_names, VertexAttrib3svARB_remap_index, -1 },
    { Uniform4ivARB_names, Uniform4ivARB_remap_index, -1 },
    { GetVertexAttribdvARB_names, GetVertexAttribdvARB_remap_index, -1 },
    { Uniform1ivARB_names, Uniform1ivARB_remap_index, -1 },
    { VertexAttrib4dvARB_names, VertexAttrib4dvARB_remap_index, -1 },
    { BindAttribLocationARB_names, BindAttribLocationARB_remap_index, -1 },
    { Uniform1iARB_names, Uniform1iARB_remap_index, -1 },
    { VertexAttribPointerARB_names, VertexAttribPointerARB_remap_index, -1 },
    { VertexAttrib4NsvARB_names, VertexAttrib4NsvARB_remap_index, -1 },
    { VertexAttrib3fvARB_names, VertexAttrib3fvARB_remap_index, -1 },
    { CreateShader_names, CreateShader_remap_index, -1 },
    { DetachShader_names, DetachShader_remap_index, -1 },
    { VertexAttrib4NubARB_names, VertexAttrib4NubARB_remap_index, -1 },
    { Uniform4fvARB_names, Uniform4fvARB_remap_index, -1 },
    { GetUniformfvARB_names, GetUniformfvARB_remap_index, -1 },
    { Uniform4iARB_names, Uniform4iARB_remap_index, -1 },
    { UseProgramObjectARB_names, UseProgramObjectARB_remap_index, -1 },
    { DeleteProgram_names, DeleteProgram_remap_index, -1 },
    { GetVertexAttribPointervNV_names, GetVertexAttribPointervNV_remap_index, -1 },
    { Uniform2iARB_names, Uniform2iARB_remap_index, -1 },
    { VertexAttrib4dARB_names, VertexAttrib4dARB_remap_index, -1 },
    { GetUniformLocationARB_names, GetUniformLocationARB_remap_index, -1 },
    { VertexAttrib3sARB_names, VertexAttrib3sARB_remap_index, -1 },
    { GetShaderSourceARB_names, GetShaderSourceARB_remap_index, -1 },
    { DrawBuffersARB_names, DrawBuffersARB_remap_index, -1 },
    { Uniform1fvARB_names, Uniform1fvARB_remap_index, -1 },
    { EnableVertexAttribArrayARB_names, EnableVertexAttribArrayARB_remap_index, -1 },
    { VertexAttrib4uivARB_names, VertexAttrib4uivARB_remap_index, -1 },
    { VertexAttrib4svARB_names, VertexAttrib4svARB_remap_index, -1 },
    { GetShaderiv_names, GetShaderiv_remap_index, -1 },
    { VertexAttrib2svARB_names, VertexAttrib2svARB_remap_index, -1 },
    { VertexAttrib4NubvARB_names, VertexAttrib4NubvARB_remap_index, -1 },
    { VertexAttrib2sARB_names, VertexAttrib2sARB_remap_index, -1 },
    { VertexAttrib1fvARB_names, VertexAttrib1fvARB_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

#if defined(need_GL_VERSION_2_1)
static const struct dri_extension_function GL_VERSION_2_1_functions[] = {
    { UniformMatrix2x4fv_names, UniformMatrix2x4fv_remap_index, -1 },
    { UniformMatrix4x3fv_names, UniformMatrix4x3fv_remap_index, -1 },
    { UniformMatrix4x2fv_names, UniformMatrix4x2fv_remap_index, -1 },
    { UniformMatrix2x3fv_names, UniformMatrix2x3fv_remap_index, -1 },
    { UniformMatrix3x4fv_names, UniformMatrix3x4fv_remap_index, -1 },
    { UniformMatrix3x2fv_names, UniformMatrix3x2fv_remap_index, -1 },
    { NULL, 0, 0 }
};
#endif

