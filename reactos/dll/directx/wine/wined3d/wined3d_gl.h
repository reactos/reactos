/*
 * Direct3D wine OpenGL include file
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2004 Jason Edmeades
 *                     Raphael Junqueira
 * Copyright 2007 Roderick Colenbrander
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINED3D_GL_H
#define __WINE_WINED3D_GL_H

#include "wine/wgl.h"

#define GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI 0x8837  /* not in the gl spec */

void (WINE_GLAPI *glDisableWINE)(GLenum cap) DECLSPEC_HIDDEN;
void (WINE_GLAPI *glEnableWINE)(GLenum cap) DECLSPEC_HIDDEN;

/* OpenGL extensions. */
enum wined3d_gl_extension
{
    WINED3D_GL_EXT_NONE,

    /* APPLE */
    APPLE_CLIENT_STORAGE,
    APPLE_FENCE,
    APPLE_FLOAT_PIXELS,
    APPLE_FLUSH_BUFFER_RANGE,
    APPLE_YCBCR_422,
    /* ARB */
    ARB_BLEND_FUNC_EXTENDED,
    ARB_COLOR_BUFFER_FLOAT,
    ARB_DEBUG_OUTPUT,
    ARB_DEPTH_BUFFER_FLOAT,
    ARB_DEPTH_CLAMP,
    ARB_DEPTH_TEXTURE,
    ARB_DRAW_BUFFERS,
    ARB_DRAW_ELEMENTS_BASE_VERTEX,
    ARB_DRAW_INSTANCED,
    ARB_FRAGMENT_PROGRAM,
    ARB_FRAGMENT_SHADER,
    ARB_FRAMEBUFFER_OBJECT,
    ARB_FRAMEBUFFER_SRGB,
    ARB_GEOMETRY_SHADER4,
    ARB_HALF_FLOAT_PIXEL,
    ARB_HALF_FLOAT_VERTEX,
    ARB_INSTANCED_ARRAYS,
    ARB_INTERNALFORMAT_QUERY2,
    ARB_MAP_BUFFER_ALIGNMENT,
    ARB_MAP_BUFFER_RANGE,
    ARB_MULTISAMPLE,
    ARB_MULTITEXTURE,
    ARB_OCCLUSION_QUERY,
    ARB_PIXEL_BUFFER_OBJECT,
    ARB_POINT_PARAMETERS,
    ARB_POINT_SPRITE,
    ARB_PROVOKING_VERTEX,
    ARB_SHADER_BIT_ENCODING,
    ARB_SHADER_OBJECTS,
    ARB_SHADER_TEXTURE_LOD,
    ARB_SHADING_LANGUAGE_100,
    ARB_SHADOW,
    ARB_SYNC,
    ARB_TEXTURE_BORDER_CLAMP,
    ARB_TEXTURE_COMPRESSION,
    ARB_TEXTURE_COMPRESSION_RGTC,
    ARB_TEXTURE_CUBE_MAP,
    ARB_TEXTURE_ENV_ADD,
    ARB_TEXTURE_ENV_COMBINE,
    ARB_TEXTURE_ENV_DOT3,
    ARB_TEXTURE_FLOAT,
    ARB_TEXTURE_MIRRORED_REPEAT,
    ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE,
    ARB_TEXTURE_NON_POWER_OF_TWO,
    ARB_TEXTURE_RECTANGLE,
    ARB_TEXTURE_RG,
    ARB_TIMER_QUERY,
    ARB_VERTEX_ARRAY_BGRA,
    ARB_VERTEX_BLEND,
    ARB_VERTEX_BUFFER_OBJECT,
    ARB_VERTEX_PROGRAM,
    ARB_VERTEX_SHADER,
    /* ATI */
    ATI_FRAGMENT_SHADER,
    ATI_SEPARATE_STENCIL,
    ATI_TEXTURE_COMPRESSION_3DC,
    ATI_TEXTURE_ENV_COMBINE3,
    ATI_TEXTURE_MIRROR_ONCE,
    /* EXT */
    EXT_BLEND_COLOR,
    EXT_BLEND_EQUATION_SEPARATE,
    EXT_BLEND_FUNC_SEPARATE,
    EXT_BLEND_MINMAX,
    EXT_BLEND_SUBTRACT,
    EXT_DRAW_BUFFERS2,
    EXT_DEPTH_BOUNDS_TEST,
    EXT_FOG_COORD,
    EXT_FRAMEBUFFER_BLIT,
    EXT_FRAMEBUFFER_MULTISAMPLE,
    EXT_FRAMEBUFFER_OBJECT,
    EXT_GPU_PROGRAM_PARAMETERS,
    EXT_GPU_SHADER4,
    EXT_PACKED_DEPTH_STENCIL,
    EXT_POINT_PARAMETERS,
    EXT_PROVOKING_VERTEX,
    EXT_SECONDARY_COLOR,
    EXT_STENCIL_TWO_SIDE,
    EXT_STENCIL_WRAP,
    EXT_TEXTURE3D,
    EXT_TEXTURE_COMPRESSION_RGTC,
    EXT_TEXTURE_COMPRESSION_S3TC,
    EXT_TEXTURE_ENV_ADD,
    EXT_TEXTURE_ENV_COMBINE,
    EXT_TEXTURE_ENV_DOT3,
    EXT_TEXTURE_FILTER_ANISOTROPIC,
    EXT_TEXTURE_LOD_BIAS,
    EXT_TEXTURE_MIRROR_CLAMP,
    EXT_TEXTURE_SRGB,
    EXT_TEXTURE_SRGB_DECODE,
    EXT_VERTEX_ARRAY_BGRA,
    /* NVIDIA */
    NV_DEPTH_CLAMP,
    NV_FENCE,
    NV_FOG_DISTANCE,
    NV_FRAGMENT_PROGRAM,
    NV_FRAGMENT_PROGRAM2,
    NV_FRAGMENT_PROGRAM_OPTION,
    NV_HALF_FLOAT,
    NV_LIGHT_MAX_EXPONENT,
    NV_POINT_SPRITE,
    NV_REGISTER_COMBINERS,
    NV_REGISTER_COMBINERS2,
    NV_TEXGEN_REFLECTION,
    NV_TEXTURE_ENV_COMBINE4,
    NV_TEXTURE_SHADER,
    NV_TEXTURE_SHADER2,
    NV_VERTEX_PROGRAM,
    NV_VERTEX_PROGRAM1_1,
    NV_VERTEX_PROGRAM2,
    NV_VERTEX_PROGRAM2_OPTION,
    NV_VERTEX_PROGRAM3,
    /* SGI */
    SGIS_GENERATE_MIPMAP,
    /* WGL extensions */
    WGL_ARB_PIXEL_FORMAT,
    WGL_EXT_SWAP_CONTROL,
    WGL_WINE_PIXEL_FORMAT_PASSTHROUGH,
    /* Internally used */
    WINED3D_GL_NORMALIZED_TEXRECT,
    WINED3D_GL_VERSION_2_0,

    WINED3D_GL_EXT_COUNT,
};

#include "wine/wglext.h"

#define GL_EXT_FUNCS_GEN \
    /* GL_APPLE_fence */ \
    USE_GL_FUNC(glDeleteFencesAPPLE) \
    USE_GL_FUNC(glFinishFenceAPPLE) \
    USE_GL_FUNC(glFinishObjectAPPLE) \
    USE_GL_FUNC(glGenFencesAPPLE) \
    USE_GL_FUNC(glIsFenceAPPLE) \
    USE_GL_FUNC(glSetFenceAPPLE) \
    USE_GL_FUNC(glTestFenceAPPLE) \
    USE_GL_FUNC(glTestObjectAPPLE) \
    /* GL_APPLE_flush_buffer_range */ \
    USE_GL_FUNC(glBufferParameteriAPPLE) \
    USE_GL_FUNC(glFlushMappedBufferRangeAPPLE) \
    /* GL_ARB_blend_func_extended */ \
    USE_GL_FUNC(glBindFragDataLocationIndexed) \
    USE_GL_FUNC(glGetFragDataIndex) \
    /* GL_ARB_color_buffer_float */ \
    USE_GL_FUNC(glClampColorARB) \
    /* GL_ARB_debug_output */ \
    USE_GL_FUNC(glDebugMessageCallbackARB) \
    USE_GL_FUNC(glDebugMessageControlARB) \
    USE_GL_FUNC(glDebugMessageInsertARB) \
    USE_GL_FUNC(glGetDebugMessageLogARB) \
    /* GL_ARB_draw_buffers */ \
    USE_GL_FUNC(glDrawBuffersARB) \
    /* GL_ARB_draw_elements_base_vertex */ \
    USE_GL_FUNC(glDrawElementsBaseVertex) \
    USE_GL_FUNC(glDrawElementsInstancedBaseVertex) \
    USE_GL_FUNC(glDrawRangeElementsBaseVertex) \
    USE_GL_FUNC(glMultiDrawElementsBaseVertex) \
    /* GL_ARB_draw_instanced */ \
    USE_GL_FUNC(glDrawArraysInstancedARB) \
    USE_GL_FUNC(glDrawElementsInstancedARB) \
    /* GL_ARB_framebuffer_object */ \
    USE_GL_FUNC(glBindFramebuffer) \
    USE_GL_FUNC(glBindRenderbuffer) \
    USE_GL_FUNC(glBlitFramebuffer) \
    USE_GL_FUNC(glCheckFramebufferStatus) \
    USE_GL_FUNC(glDeleteFramebuffers) \
    USE_GL_FUNC(glDeleteRenderbuffers) \
    USE_GL_FUNC(glFramebufferRenderbuffer) \
    USE_GL_FUNC(glFramebufferTexture1D) \
    USE_GL_FUNC(glFramebufferTexture2D) \
    USE_GL_FUNC(glFramebufferTexture3D) \
    USE_GL_FUNC(glFramebufferTextureLayer) \
    USE_GL_FUNC(glGenFramebuffers) \
    USE_GL_FUNC(glGenRenderbuffers) \
    USE_GL_FUNC(glGenerateMipmap) \
    USE_GL_FUNC(glGetFramebufferAttachmentParameteriv) \
    USE_GL_FUNC(glGetRenderbufferParameteriv) \
    USE_GL_FUNC(glIsFramebuffer) \
    USE_GL_FUNC(glIsRenderbuffer) \
    USE_GL_FUNC(glRenderbufferStorage) \
    USE_GL_FUNC(glRenderbufferStorageMultisample) \
    /* GL_ARB_geometry_shader4 */ \
    USE_GL_FUNC(glFramebufferTextureARB) \
    USE_GL_FUNC(glFramebufferTextureFaceARB) \
    USE_GL_FUNC(glFramebufferTextureLayerARB) \
    USE_GL_FUNC(glProgramParameteriARB) \
    /* GL_ARB_instanced_arrays */ \
    USE_GL_FUNC(glVertexAttribDivisorARB) \
    /* GL_ARB_internalformat_query */ \
    USE_GL_FUNC(glGetInternalformativ) \
    /* GL_ARB_internalformat_query2 */ \
    USE_GL_FUNC(glGetInternalformati64v) \
    /* GL_ARB_map_buffer_range */ \
    USE_GL_FUNC(glFlushMappedBufferRange) \
    USE_GL_FUNC(glMapBufferRange) \
    /* GL_ARB_multisample */ \
    USE_GL_FUNC(glSampleCoverageARB) \
    /* GL_ARB_multitexture */ \
    USE_GL_FUNC(glActiveTextureARB) \
    USE_GL_FUNC(glClientActiveTextureARB) \
    USE_GL_FUNC(glMultiTexCoord1fARB) \
    USE_GL_FUNC(glMultiTexCoord1fvARB) \
    USE_GL_FUNC(glMultiTexCoord2fARB) \
    USE_GL_FUNC(glMultiTexCoord2fvARB) \
    USE_GL_FUNC(glMultiTexCoord2svARB) \
    USE_GL_FUNC(glMultiTexCoord3fARB) \
    USE_GL_FUNC(glMultiTexCoord3fvARB) \
    USE_GL_FUNC(glMultiTexCoord4fARB) \
    USE_GL_FUNC(glMultiTexCoord4fvARB) \
    USE_GL_FUNC(glMultiTexCoord4svARB) \
    /* GL_ARB_occlusion_query */ \
    USE_GL_FUNC(glBeginQueryARB) \
    USE_GL_FUNC(glDeleteQueriesARB) \
    USE_GL_FUNC(glEndQueryARB) \
    USE_GL_FUNC(glGenQueriesARB) \
    USE_GL_FUNC(glGetQueryivARB) \
    USE_GL_FUNC(glGetQueryObjectivARB) \
    USE_GL_FUNC(glGetQueryObjectuivARB) \
    USE_GL_FUNC(glIsQueryARB) \
    /* GL_ARB_point_parameters */ \
    USE_GL_FUNC(glPointParameterfARB) \
    USE_GL_FUNC(glPointParameterfvARB) \
    /* GL_ARB_provoking_vertex */ \
    USE_GL_FUNC(glProvokingVertex) \
    /* GL_ARB_shader_objects */ \
    USE_GL_FUNC(glAttachObjectARB) \
    USE_GL_FUNC(glBindAttribLocationARB) \
    USE_GL_FUNC(glCompileShaderARB) \
    USE_GL_FUNC(glCreateProgramObjectARB) \
    USE_GL_FUNC(glCreateShaderObjectARB) \
    USE_GL_FUNC(glDeleteObjectARB) \
    USE_GL_FUNC(glDetachObjectARB) \
    USE_GL_FUNC(glGetActiveUniformARB) \
    USE_GL_FUNC(glGetAttachedObjectsARB) \
    USE_GL_FUNC(glGetAttribLocationARB) \
    USE_GL_FUNC(glGetHandleARB) \
    USE_GL_FUNC(glGetInfoLogARB) \
    USE_GL_FUNC(glGetObjectParameterfvARB) \
    USE_GL_FUNC(glGetObjectParameterivARB) \
    USE_GL_FUNC(glGetShaderSourceARB) \
    USE_GL_FUNC(glGetUniformLocationARB) \
    USE_GL_FUNC(glGetUniformfvARB) \
    USE_GL_FUNC(glGetUniformivARB) \
    USE_GL_FUNC(glLinkProgramARB) \
    USE_GL_FUNC(glShaderSourceARB) \
    USE_GL_FUNC(glUniform1fARB) \
    USE_GL_FUNC(glUniform1fvARB) \
    USE_GL_FUNC(glUniform1iARB) \
    USE_GL_FUNC(glUniform1ivARB) \
    USE_GL_FUNC(glUniform2fARB) \
    USE_GL_FUNC(glUniform2fvARB) \
    USE_GL_FUNC(glUniform2iARB) \
    USE_GL_FUNC(glUniform2ivARB) \
    USE_GL_FUNC(glUniform3fARB) \
    USE_GL_FUNC(glUniform3fvARB) \
    USE_GL_FUNC(glUniform3iARB) \
    USE_GL_FUNC(glUniform3ivARB) \
    USE_GL_FUNC(glUniform4fARB) \
    USE_GL_FUNC(glUniform4fvARB) \
    USE_GL_FUNC(glUniform4iARB) \
    USE_GL_FUNC(glUniform4ivARB) \
    USE_GL_FUNC(glUniformMatrix2fvARB) \
    USE_GL_FUNC(glUniformMatrix3fvARB) \
    USE_GL_FUNC(glUniformMatrix4fvARB) \
    USE_GL_FUNC(glUseProgramObjectARB) \
    USE_GL_FUNC(glValidateProgramARB) \
    /* GL_ARB_sync */ \
    USE_GL_FUNC(glClientWaitSync) \
    USE_GL_FUNC(glDeleteSync) \
    USE_GL_FUNC(glFenceSync) \
    USE_GL_FUNC(glGetInteger64v) \
    USE_GL_FUNC(glGetSynciv) \
    USE_GL_FUNC(glIsSync) \
    USE_GL_FUNC(glWaitSync) \
    /* GL_ARB_texture_compression */ \
    USE_GL_FUNC(glCompressedTexImage2DARB) \
    USE_GL_FUNC(glCompressedTexImage3DARB) \
    USE_GL_FUNC(glCompressedTexSubImage2DARB) \
    USE_GL_FUNC(glCompressedTexSubImage3DARB) \
    USE_GL_FUNC(glGetCompressedTexImageARB) \
    /* GL_ARB_timer_query */ \
    USE_GL_FUNC(glQueryCounter) \
    USE_GL_FUNC(glGetQueryObjectui64v) \
    /* GL_ARB_vertex_blend */ \
    USE_GL_FUNC(glVertexBlendARB) \
    USE_GL_FUNC(glWeightPointerARB) \
    USE_GL_FUNC(glWeightbvARB) \
    USE_GL_FUNC(glWeightdvARB) \
    USE_GL_FUNC(glWeightfvARB) \
    USE_GL_FUNC(glWeightivARB) \
    USE_GL_FUNC(glWeightsvARB) \
    USE_GL_FUNC(glWeightubvARB) \
    USE_GL_FUNC(glWeightuivARB) \
    USE_GL_FUNC(glWeightusvARB) \
    /* GL_ARB_vertex_buffer_object */ \
    USE_GL_FUNC(glBindBufferARB) \
    USE_GL_FUNC(glBufferDataARB) \
    USE_GL_FUNC(glBufferSubDataARB) \
    USE_GL_FUNC(glDeleteBuffersARB) \
    USE_GL_FUNC(glGenBuffersARB) \
    USE_GL_FUNC(glGetBufferParameterivARB) \
    USE_GL_FUNC(glGetBufferPointervARB) \
    USE_GL_FUNC(glGetBufferSubDataARB) \
    USE_GL_FUNC(glIsBufferARB) \
    USE_GL_FUNC(glMapBufferARB) \
    USE_GL_FUNC(glUnmapBufferARB) \
    /* GL_ARB_vertex_program */ \
    USE_GL_FUNC(glBindProgramARB) \
    USE_GL_FUNC(glDeleteProgramsARB) \
    USE_GL_FUNC(glDisableVertexAttribArrayARB) \
    USE_GL_FUNC(glEnableVertexAttribArrayARB) \
    USE_GL_FUNC(glGenProgramsARB) \
    USE_GL_FUNC(glGetProgramivARB) \
    USE_GL_FUNC(glProgramEnvParameter4fvARB) \
    USE_GL_FUNC(glProgramLocalParameter4fvARB) \
    USE_GL_FUNC(glProgramStringARB) \
    USE_GL_FUNC(glVertexAttrib1dARB) \
    USE_GL_FUNC(glVertexAttrib1dvARB) \
    USE_GL_FUNC(glVertexAttrib1fARB) \
    USE_GL_FUNC(glVertexAttrib1fvARB) \
    USE_GL_FUNC(glVertexAttrib1sARB) \
    USE_GL_FUNC(glVertexAttrib1svARB) \
    USE_GL_FUNC(glVertexAttrib2dARB) \
    USE_GL_FUNC(glVertexAttrib2dvARB) \
    USE_GL_FUNC(glVertexAttrib2fARB) \
    USE_GL_FUNC(glVertexAttrib2fvARB) \
    USE_GL_FUNC(glVertexAttrib2sARB) \
    USE_GL_FUNC(glVertexAttrib2svARB) \
    USE_GL_FUNC(glVertexAttrib3dARB) \
    USE_GL_FUNC(glVertexAttrib3dvARB) \
    USE_GL_FUNC(glVertexAttrib3fARB) \
    USE_GL_FUNC(glVertexAttrib3fvARB) \
    USE_GL_FUNC(glVertexAttrib3sARB) \
    USE_GL_FUNC(glVertexAttrib3svARB) \
    USE_GL_FUNC(glVertexAttrib4NbvARB) \
    USE_GL_FUNC(glVertexAttrib4NivARB) \
    USE_GL_FUNC(glVertexAttrib4NsvARB) \
    USE_GL_FUNC(glVertexAttrib4NubARB) \
    USE_GL_FUNC(glVertexAttrib4NubvARB) \
    USE_GL_FUNC(glVertexAttrib4NuivARB) \
    USE_GL_FUNC(glVertexAttrib4NusvARB) \
    USE_GL_FUNC(glVertexAttrib4bvARB) \
    USE_GL_FUNC(glVertexAttrib4dARB) \
    USE_GL_FUNC(glVertexAttrib4dvARB) \
    USE_GL_FUNC(glVertexAttrib4fARB) \
    USE_GL_FUNC(glVertexAttrib4fvARB) \
    USE_GL_FUNC(glVertexAttrib4ivARB) \
    USE_GL_FUNC(glVertexAttrib4sARB) \
    USE_GL_FUNC(glVertexAttrib4svARB) \
    USE_GL_FUNC(glVertexAttrib4ubvARB) \
    USE_GL_FUNC(glVertexAttrib4uivARB) \
    USE_GL_FUNC(glVertexAttrib4usvARB) \
    USE_GL_FUNC(glVertexAttribPointerARB) \
    /* GL_ATI_fragment_shader */ \
    USE_GL_FUNC(glAlphaFragmentOp1ATI) \
    USE_GL_FUNC(glAlphaFragmentOp2ATI) \
    USE_GL_FUNC(glAlphaFragmentOp3ATI) \
    USE_GL_FUNC(glBeginFragmentShaderATI) \
    USE_GL_FUNC(glBindFragmentShaderATI) \
    USE_GL_FUNC(glColorFragmentOp1ATI) \
    USE_GL_FUNC(glColorFragmentOp2ATI) \
    USE_GL_FUNC(glColorFragmentOp3ATI) \
    USE_GL_FUNC(glDeleteFragmentShaderATI) \
    USE_GL_FUNC(glEndFragmentShaderATI) \
    USE_GL_FUNC(glGenFragmentShadersATI) \
    USE_GL_FUNC(glPassTexCoordATI) \
    USE_GL_FUNC(glSampleMapATI) \
    USE_GL_FUNC(glSetFragmentShaderConstantATI) \
    /* GL_ATI_separate_stencil */ \
    USE_GL_FUNC(glStencilOpSeparateATI) \
    USE_GL_FUNC(glStencilFuncSeparateATI) \
    /* GL_EXT_blend_color */ \
    USE_GL_FUNC(glBlendColorEXT) \
    /* GL_EXT_blend_equation_separate */ \
    USE_GL_FUNC(glBlendFuncSeparateEXT) \
    /* GL_EXT_blend_func_separate */ \
    USE_GL_FUNC(glBlendEquationSeparateEXT) \
    /* GL_EXT_blend_minmax */ \
    USE_GL_FUNC(glBlendEquationEXT) \
    /* GL_EXT_depth_bounds_test */ \
    USE_GL_FUNC(glDepthBoundsEXT) \
    /* GL_EXT_draw_buffers2 */ \
    USE_GL_FUNC(glColorMaskIndexedEXT) \
    USE_GL_FUNC(glDisableIndexedEXT) \
    USE_GL_FUNC(glEnableIndexedEXT) \
    USE_GL_FUNC(glGetBooleanIndexedvEXT) \
    USE_GL_FUNC(glGetIntegerIndexedvEXT) \
    USE_GL_FUNC(glIsEnabledIndexedEXT) \
    /* GL_EXT_fog_coord */ \
    USE_GL_FUNC(glFogCoordPointerEXT) \
    USE_GL_FUNC(glFogCoorddEXT) \
    USE_GL_FUNC(glFogCoorddvEXT) \
    USE_GL_FUNC(glFogCoordfEXT) \
    USE_GL_FUNC(glFogCoordfvEXT) \
    /* GL_EXT_framebuffer_blit */ \
    USE_GL_FUNC(glBlitFramebufferEXT) \
    /* GL_EXT_framebuffer_multisample */ \
    USE_GL_FUNC(glRenderbufferStorageMultisampleEXT) \
    /* GL_EXT_framebuffer_object */ \
    USE_GL_FUNC(glBindFramebufferEXT) \
    USE_GL_FUNC(glBindRenderbufferEXT) \
    USE_GL_FUNC(glCheckFramebufferStatusEXT) \
    USE_GL_FUNC(glDeleteFramebuffersEXT) \
    USE_GL_FUNC(glDeleteRenderbuffersEXT) \
    USE_GL_FUNC(glFramebufferRenderbufferEXT) \
    USE_GL_FUNC(glFramebufferTexture1DEXT) \
    USE_GL_FUNC(glFramebufferTexture2DEXT) \
    USE_GL_FUNC(glFramebufferTexture3DEXT) \
    USE_GL_FUNC(glGenFramebuffersEXT) \
    USE_GL_FUNC(glGenRenderbuffersEXT) \
    USE_GL_FUNC(glGenerateMipmapEXT) \
    USE_GL_FUNC(glGetFramebufferAttachmentParameterivEXT) \
    USE_GL_FUNC(glGetRenderbufferParameterivEXT) \
    USE_GL_FUNC(glIsFramebufferEXT) \
    USE_GL_FUNC(glIsRenderbufferEXT) \
    USE_GL_FUNC(glRenderbufferStorageEXT) \
    /* GL_EXT_gpu_program_parameters */ \
    USE_GL_FUNC(glProgramEnvParameters4fvEXT) \
    USE_GL_FUNC(glProgramLocalParameters4fvEXT) \
    /* GL_EXT_gpu_shader4 */\
    USE_GL_FUNC(glBindFragDataLocationEXT) \
    USE_GL_FUNC(glGetFragDataLocationEXT) \
    USE_GL_FUNC(glGetUniformuivEXT) \
    USE_GL_FUNC(glGetVertexAttribIivEXT) \
    USE_GL_FUNC(glGetVertexAttribIuivEXT) \
    USE_GL_FUNC(glUniform1uiEXT) \
    USE_GL_FUNC(glUniform1uivEXT) \
    USE_GL_FUNC(glUniform2uiEXT) \
    USE_GL_FUNC(glUniform2uivEXT) \
    USE_GL_FUNC(glUniform3uiEXT) \
    USE_GL_FUNC(glUniform3uivEXT) \
    USE_GL_FUNC(glUniform4uiEXT) \
    USE_GL_FUNC(glUniform4uivEXT) \
    USE_GL_FUNC(glVertexAttribI1iEXT) \
    USE_GL_FUNC(glVertexAttribI1ivEXT) \
    USE_GL_FUNC(glVertexAttribI1uiEXT) \
    USE_GL_FUNC(glVertexAttribI1uivEXT) \
    USE_GL_FUNC(glVertexAttribI2iEXT) \
    USE_GL_FUNC(glVertexAttribI2ivEXT) \
    USE_GL_FUNC(glVertexAttribI2uiEXT) \
    USE_GL_FUNC(glVertexAttribI2uivEXT) \
    USE_GL_FUNC(glVertexAttribI3iEXT) \
    USE_GL_FUNC(glVertexAttribI3ivEXT) \
    USE_GL_FUNC(glVertexAttribI3uiEXT) \
    USE_GL_FUNC(glVertexAttribI3uivEXT) \
    USE_GL_FUNC(glVertexAttribI4bvEXT) \
    USE_GL_FUNC(glVertexAttribI4iEXT) \
    USE_GL_FUNC(glVertexAttribI4ivEXT) \
    USE_GL_FUNC(glVertexAttribI4svEXT) \
    USE_GL_FUNC(glVertexAttribI4ubvEXT) \
    USE_GL_FUNC(glVertexAttribI4uiEXT) \
    USE_GL_FUNC(glVertexAttribI4uivEXT) \
    USE_GL_FUNC(glVertexAttribI4usvEXT) \
    USE_GL_FUNC(glVertexAttribIPointerEXT) \
    /* GL_EXT_point_parameters */ \
    USE_GL_FUNC(glPointParameterfEXT) \
    USE_GL_FUNC(glPointParameterfvEXT) \
    /* GL_EXT_provoking_vertex */ \
    USE_GL_FUNC(glProvokingVertexEXT) \
    /* GL_EXT_secondary_color */ \
    USE_GL_FUNC(glSecondaryColor3fEXT) \
    USE_GL_FUNC(glSecondaryColor3fvEXT) \
    USE_GL_FUNC(glSecondaryColor3ubEXT) \
    USE_GL_FUNC(glSecondaryColor3ubvEXT) \
    USE_GL_FUNC(glSecondaryColorPointerEXT) \
    /* GL_EXT_stencil_two_side */ \
    USE_GL_FUNC(glActiveStencilFaceEXT) \
    /* GL_EXT_texture3D */ \
    USE_GL_FUNC(glTexImage3D) \
    USE_GL_FUNC(glTexImage3DEXT) \
    USE_GL_FUNC(glTexSubImage3D) \
    USE_GL_FUNC(glTexSubImage3DEXT) \
    /* GL_NV_fence */ \
    USE_GL_FUNC(glDeleteFencesNV) \
    USE_GL_FUNC(glFinishFenceNV) \
    USE_GL_FUNC(glGenFencesNV) \
    USE_GL_FUNC(glGetFenceivNV) \
    USE_GL_FUNC(glIsFenceNV) \
    USE_GL_FUNC(glSetFenceNV) \
    USE_GL_FUNC(glTestFenceNV) \
    /* GL_NV_half_float */ \
    USE_GL_FUNC(glColor3hNV) \
    USE_GL_FUNC(glColor3hvNV) \
    USE_GL_FUNC(glColor4hNV) \
    USE_GL_FUNC(glColor4hvNV) \
    USE_GL_FUNC(glFogCoordhNV) \
    USE_GL_FUNC(glFogCoordhvNV) \
    USE_GL_FUNC(glMultiTexCoord1hNV) \
    USE_GL_FUNC(glMultiTexCoord1hvNV) \
    USE_GL_FUNC(glMultiTexCoord2hNV) \
    USE_GL_FUNC(glMultiTexCoord2hvNV) \
    USE_GL_FUNC(glMultiTexCoord3hNV) \
    USE_GL_FUNC(glMultiTexCoord3hvNV) \
    USE_GL_FUNC(glMultiTexCoord4hNV) \
    USE_GL_FUNC(glMultiTexCoord4hvNV) \
    USE_GL_FUNC(glNormal3hNV) \
    USE_GL_FUNC(glNormal3hvNV) \
    USE_GL_FUNC(glSecondaryColor3hNV) \
    USE_GL_FUNC(glSecondaryColor3hvNV) \
    USE_GL_FUNC(glTexCoord1hNV) \
    USE_GL_FUNC(glTexCoord1hvNV) \
    USE_GL_FUNC(glTexCoord2hNV) \
    USE_GL_FUNC(glTexCoord2hvNV) \
    USE_GL_FUNC(glTexCoord3hNV) \
    USE_GL_FUNC(glTexCoord3hvNV) \
    USE_GL_FUNC(glTexCoord4hNV) \
    USE_GL_FUNC(glTexCoord4hvNV) \
    USE_GL_FUNC(glVertex2hNV) \
    USE_GL_FUNC(glVertex2hvNV) \
    USE_GL_FUNC(glVertex3hNV) \
    USE_GL_FUNC(glVertex3hvNV) \
    USE_GL_FUNC(glVertex4hNV) \
    USE_GL_FUNC(glVertex4hvNV) \
    USE_GL_FUNC(glVertexAttrib1hNV) \
    USE_GL_FUNC(glVertexAttrib1hvNV) \
    USE_GL_FUNC(glVertexAttrib2hNV) \
    USE_GL_FUNC(glVertexAttrib2hvNV) \
    USE_GL_FUNC(glVertexAttrib3hNV) \
    USE_GL_FUNC(glVertexAttrib3hvNV) \
    USE_GL_FUNC(glVertexAttrib4hNV) \
    USE_GL_FUNC(glVertexAttrib4hvNV) \
    USE_GL_FUNC(glVertexAttribs1hvNV) \
    USE_GL_FUNC(glVertexAttribs2hvNV) \
    USE_GL_FUNC(glVertexAttribs3hvNV) \
    USE_GL_FUNC(glVertexAttribs4hvNV) \
    USE_GL_FUNC(glVertexWeighthNV) \
    USE_GL_FUNC(glVertexWeighthvNV) \
    /* GL_NV_point_sprite */ \
    USE_GL_FUNC(glPointParameteri) \
    USE_GL_FUNC(glPointParameteriNV) \
    USE_GL_FUNC(glPointParameteriv) \
    USE_GL_FUNC(glPointParameterivNV) \
    /* GL_NV_register_combiners */ \
    USE_GL_FUNC(glCombinerInputNV) \
    USE_GL_FUNC(glCombinerOutputNV) \
    USE_GL_FUNC(glCombinerParameterfNV) \
    USE_GL_FUNC(glCombinerParameterfvNV) \
    USE_GL_FUNC(glCombinerParameteriNV) \
    USE_GL_FUNC(glCombinerParameterivNV) \
    USE_GL_FUNC(glFinalCombinerInputNV) \
    /* WGL extensions */ \
    USE_GL_FUNC(wglChoosePixelFormatARB) \
    USE_GL_FUNC(wglGetExtensionsStringARB) \
    USE_GL_FUNC(wglGetPixelFormatAttribfvARB) \
    USE_GL_FUNC(wglGetPixelFormatAttribivARB) \
    USE_GL_FUNC(wglSetPixelFormatWINE) \
    USE_GL_FUNC(wglSwapIntervalEXT)

#endif /* __WINE_WINED3D_GL */
