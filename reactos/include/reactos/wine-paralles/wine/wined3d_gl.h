/*
 * Direct3D wine OpenGL include file
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2004 Jason Edmeades
 *                     Raphael Junqueira
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

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef HAVE_OPENGL

#ifndef WINE_NATIVEWIN32

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

#define XMD_H /* This is to prevent the Xmd.h inclusion bug :-/ */
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#ifdef HAVE_GL_GLEXT_H
# include <GL/glext.h>
#endif
#undef  XMD_H

#undef APIENTRY
#define APIENTRY

#else

#include <windows.h>

#include <GL/gl.h>
#ifdef HAVE_GL_GLEXT_H
# include <GL/glext.h>
#endif

#endif
/****************************************************
 * OpenGL Extensions (EXT and ARB)
 *     #defines and functions pointer
 ****************************************************/

/* GL_ARB_depth_texture */
#ifndef GL_ARB_depth_texture
#define GL_ARB_depth_texture 1
#define GL_DEPTH_COMPONENT16_ARB          0x81A5
#define GL_DEPTH_COMPONENT24_ARB          0x81A6
#define GL_DEPTH_COMPONENT32_ARB          0x81A7
#define GL_TEXTURE_DEPTH_SIZE_ARB         0x884A
#define GL_DEPTH_TEXTURE_MODE_ARB         0x884B
#endif

/* GL_ARB_draw_buffers */
#ifndef GL_ARB_draw_buffers
#define GL_MAX_DRAW_BUFFERS_ARB           0x8824
#define GL_DRAW_BUFFER0_ARB               0x8825
#define GL_DRAW_BUFFER1_ARB               0x8826
#define GL_DRAW_BUFFER2_ARB               0x8827
#define GL_DRAW_BUFFER3_ARB               0x8828
#define GL_DRAW_BUFFER4_ARB               0x8829
#define GL_DRAW_BUFFER5_ARB               0x882A
#define GL_DRAW_BUFFER6_ARB               0x882B
#define GL_DRAW_BUFFER7_ARB               0x882C
#define GL_DRAW_BUFFER8_ARB               0x882D
#define GL_DRAW_BUFFER9_ARB               0x882E
#define GL_DRAW_BUFFER10_ARB              0x882F
#define GL_DRAW_BUFFER11_ARB              0x8830
#define GL_DRAW_BUFFER12_ARB              0x8831
#define GL_DRAW_BUFFER13_ARB              0x8832
#define GL_DRAW_BUFFER14_ARB              0x8833
#define GL_DRAW_BUFFER15_ARB              0x8834
#endif
typedef void (APIENTRY *PGLFNDRAWBUFFERSARBPROC) (GLsizei n, const GLenum *bufs);

/* GL_ARB_imaging */
#ifndef GL_ARB_imaging
#define GL_CONSTANT_COLOR                 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR       0x8002
#define GL_CONSTANT_ALPHA                 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA       0x8004
#define GL_BLEND_COLOR                    0x8005
#define GL_FUNC_ADD                       0x8006
#define GL_MIN                            0x8007
#define GL_MAX                            0x8008
#define GL_BLEND_EQUATION                 0x8009
#define GL_FUNC_SUBTRACT                  0x800A
#define GL_FUNC_REVERSE_SUBTRACT          0x800B
#define GL_CONVOLUTION_1D                 0x8010
#define GL_CONVOLUTION_2D                 0x8011
#define GL_SEPARABLE_2D                   0x8012
#define GL_CONVOLUTION_BORDER_MODE        0x8013
#define GL_CONVOLUTION_FILTER_SCALE       0x8014
#define GL_CONVOLUTION_FILTER_BIAS        0x8015
#define GL_REDUCE                         0x8016
#define GL_CONVOLUTION_FORMAT             0x8017
#define GL_CONVOLUTION_WIDTH              0x8018
#define GL_CONVOLUTION_HEIGHT             0x8019
#define GL_MAX_CONVOLUTION_WIDTH          0x801A
#define GL_MAX_CONVOLUTION_HEIGHT         0x801B
#define GL_POST_CONVOLUTION_RED_SCALE     0x801C
#define GL_POST_CONVOLUTION_GREEN_SCALE   0x801D
#define GL_POST_CONVOLUTION_BLUE_SCALE    0x801E
#define GL_POST_CONVOLUTION_ALPHA_SCALE   0x801F
#define GL_POST_CONVOLUTION_RED_BIAS      0x8020
#define GL_POST_CONVOLUTION_GREEN_BIAS    0x8021
#define GL_POST_CONVOLUTION_BLUE_BIAS     0x8022
#define GL_POST_CONVOLUTION_ALPHA_BIAS    0x8023
#define GL_HISTOGRAM                      0x8024
#define GL_PROXY_HISTOGRAM                0x8025
#define GL_HISTOGRAM_WIDTH                0x8026
#define GL_HISTOGRAM_FORMAT               0x8027
#define GL_HISTOGRAM_RED_SIZE             0x8028
#define GL_HISTOGRAM_GREEN_SIZE           0x8029
#define GL_HISTOGRAM_BLUE_SIZE            0x802A
#define GL_HISTOGRAM_ALPHA_SIZE           0x802B
#define GL_HISTOGRAM_LUMINANCE_SIZE       0x802C
#define GL_HISTOGRAM_SINK                 0x802D
#define GL_MINMAX                         0x802E
#define GL_MINMAX_FORMAT                  0x802F
#define GL_MINMAX_SINK                    0x8030
#define GL_TABLE_TOO_LARGE                0x8031
#define GL_COLOR_MATRIX                   0x80B1
#define GL_COLOR_MATRIX_STACK_DEPTH       0x80B2
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH   0x80B3
#define GL_POST_COLOR_MATRIX_RED_SCALE    0x80B4
#define GL_POST_COLOR_MATRIX_GREEN_SCALE  0x80B5
#define GL_POST_COLOR_MATRIX_BLUE_SCALE   0x80B6
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE  0x80B7
#define GL_POST_COLOR_MATRIX_RED_BIAS     0x80B8
#define GL_POST_COLOR_MATRIX_GREEN_BIAS   0x80B9
#define GL_POST_COLOR_MATRIX_BLUE_BIAS    0x80BA
#define GL_POST_COLOR_MATRIX_ALPHA_BIAS   0x80BB
#define GL_COLOR_TABLE                    0x80D0
#define GL_POST_CONVOLUTION_COLOR_TABLE   0x80D1
#define GL_POST_COLOR_MATRIX_COLOR_TABLE  0x80D2
#define GL_PROXY_COLOR_TABLE              0x80D3
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE 0x80D4
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE 0x80D5
#define GL_COLOR_TABLE_SCALE              0x80D6
#define GL_COLOR_TABLE_BIAS               0x80D7
#define GL_COLOR_TABLE_FORMAT             0x80D8
#define GL_COLOR_TABLE_WIDTH              0x80D9
#define GL_COLOR_TABLE_RED_SIZE           0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE         0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE          0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE         0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE     0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE     0x80DF
#define GL_CONSTANT_BORDER                0x8151
#define GL_REPLICATE_BORDER               0x8153
#define GL_CONVOLUTION_BORDER_COLOR       0x8154
#endif
typedef void (APIENTRY *PGLFNBLENDCOLORPROC) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (APIENTRY *PGLFNBLENDEQUATIONPROC) (GLenum mode);
/* GL_ARB_multitexture */
#ifndef GL_ARB_multitexture
#define GL_ARB_multitexture 1
#define GL_TEXTURE0_ARB                   0x84C0
#define GL_TEXTURE1_ARB                   0x84C1
#define GL_TEXTURE2_ARB                   0x84C2
#define GL_TEXTURE3_ARB                   0x84C3
#define GL_TEXTURE4_ARB                   0x84C4
#define GL_TEXTURE5_ARB                   0x84C5
#define GL_TEXTURE6_ARB                   0x84C6
#define GL_TEXTURE7_ARB                   0x84C7
#define GL_TEXTURE8_ARB                   0x84C8
#define GL_TEXTURE9_ARB                   0x84C9
#define GL_TEXTURE10_ARB                  0x84CA
#define GL_TEXTURE11_ARB                  0x84CB
#define GL_TEXTURE12_ARB                  0x84CC
#define GL_TEXTURE13_ARB                  0x84CD
#define GL_TEXTURE14_ARB                  0x84CE
#define GL_TEXTURE15_ARB                  0x84CF
#define GL_TEXTURE16_ARB                  0x84D0
#define GL_TEXTURE17_ARB                  0x84D1
#define GL_TEXTURE18_ARB                  0x84D2
#define GL_TEXTURE19_ARB                  0x84D3
#define GL_TEXTURE20_ARB                  0x84D4
#define GL_TEXTURE21_ARB                  0x84D5
#define GL_TEXTURE22_ARB                  0x84D6
#define GL_TEXTURE23_ARB                  0x84D7
#define GL_TEXTURE24_ARB                  0x84D8
#define GL_TEXTURE25_ARB                  0x84D9
#define GL_TEXTURE26_ARB                  0x84DA
#define GL_TEXTURE27_ARB                  0x84DB
#define GL_TEXTURE28_ARB                  0x84DC
#define GL_TEXTURE29_ARB                  0x84DD
#define GL_TEXTURE30_ARB                  0x84DE
#define GL_TEXTURE31_ARB                  0x84DF
#define GL_ACTIVE_TEXTURE_ARB             0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB      0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB          0x84E2
#endif
typedef void (APIENTRY *WINED3D_PFNGLACTIVETEXTUREARBPROC) (GLenum texture);
typedef void (APIENTRY *WINED3D_PFNGLCLIENTACTIVETEXTUREARBPROC) (GLenum texture);
typedef void (APIENTRY *WINED3D_PFNGLMULTITEXCOORD1FARBPROC) (GLenum target, GLfloat s);
typedef void (APIENTRY *WINED3D_PFNGLMULTITEXCOORD2FARBPROC) (GLenum target, GLfloat s, GLfloat t);
typedef void (APIENTRY *WINED3D_PFNGLMULTITEXCOORD3FARBPROC) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
typedef void (APIENTRY *WINED3D_PFNGLMULTITEXCOORD4FARBPROC) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);

/* GL_ARB_texture_cube_map */
#ifndef GL_ARB_texture_cube_map
#define GL_ARB_texture_cube_map 1
#define GL_NORMAL_MAP_ARB                   0x8511
#define GL_REFLECTION_MAP_ARB               0x8512
#define GL_TEXTURE_CUBE_MAP_ARB             0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB     0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB  0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB  0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB  0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB  0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB  0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB  0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB       0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB    0x851C
#endif

/* GL_ARB_point_parameters */
#ifndef GL_ARB_point_parameters
#define GL_ARB_point_parameters 1
#define GL_POINT_SIZE_MIN_ARB             0x8126
#define GL_POINT_SIZE_MAX_ARB             0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_ARB  0x8128
#define GL_POINT_DISTANCE_ATTENUATION_ARB 0x8129
#endif
typedef void (APIENTRY * PGLFNGLPOINTPARAMETERFARBPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PGLFNGLPOINTPARAMETERFVARBPROC) (GLenum pname, const GLfloat *params);
/* GL_ARB_vertex_blend */
#ifndef GL_ARB_vertex_blend
#define GL_ARB_vertex_blend 1
#define GL_MAX_VERTEX_UNITS_ARB           0x86A4
#define GL_ACTIVE_VERTEX_UNITS_ARB        0x86A5
#define GL_WEIGHT_SUM_UNITY_ARB           0x86A6
#define GL_VERTEX_BLEND_ARB               0x86A7
#define GL_CURRENT_WEIGHT_ARB             0x86A8
#define GL_WEIGHT_ARRAY_TYPE_ARB          0x86A9
#define GL_WEIGHT_ARRAY_STRIDE_ARB        0x86AA
#define GL_WEIGHT_ARRAY_SIZE_ARB          0x86AB
#define GL_WEIGHT_ARRAY_POINTER_ARB       0x86AC
#define GL_WEIGHT_ARRAY_ARB               0x86AD
#define GL_MODELVIEW0_ARB                 0x1700
#define GL_MODELVIEW1_ARB                 0x850A
#define GL_MODELVIEW2_ARB                 0x8722
#define GL_MODELVIEW3_ARB                 0x8723
#define GL_MODELVIEW4_ARB                 0x8724
#define GL_MODELVIEW5_ARB                 0x8725
#define GL_MODELVIEW6_ARB                 0x8726
#define GL_MODELVIEW7_ARB                 0x8727
#define GL_MODELVIEW8_ARB                 0x8728
#define GL_MODELVIEW9_ARB                 0x8729
#define GL_MODELVIEW10_ARB                0x872A
#define GL_MODELVIEW11_ARB                0x872B
#define GL_MODELVIEW12_ARB                0x872C
#define GL_MODELVIEW13_ARB                0x872D
#define GL_MODELVIEW14_ARB                0x872E
#define GL_MODELVIEW15_ARB                0x872F
#define GL_MODELVIEW16_ARB                0x8730
#define GL_MODELVIEW17_ARB                0x8731
#define GL_MODELVIEW18_ARB                0x8732
#define GL_MODELVIEW19_ARB                0x8733
#define GL_MODELVIEW20_ARB                0x8734
#define GL_MODELVIEW21_ARB                0x8735
#define GL_MODELVIEW22_ARB                0x8736
#define GL_MODELVIEW23_ARB                0x8737
#define GL_MODELVIEW24_ARB                0x8738
#define GL_MODELVIEW25_ARB                0x8739
#define GL_MODELVIEW26_ARB                0x873A
#define GL_MODELVIEW27_ARB                0x873B
#define GL_MODELVIEW28_ARB                0x873C
#define GL_MODELVIEW29_ARB                0x873D
#define GL_MODELVIEW30_ARB                0x873E
#define GL_MODELVIEW31_ARB                0x873F
#endif
typedef void (APIENTRY * PGLFNGLWEIGHTPOINTERARB) (GLint size, GLenum type, GLsizei stride, GLvoid* pointer);
/* GL_ARB_pixel_buffer_object */
#ifndef GL_ARB_pixel_buffer_object
#define GL_ARB_pixel_buffer_object 1
#endif
#define GL_PIXEL_PACK_BUFFER_ARB               0x88EB
#define GL_PIXEL_UNPACK_BUFFER_ARB             0x88EC
#define GL_PIXEL_PACK_BUFFER_BINDING_ARB       0x88ED
#define GL_PIXEL_UNPACK_BUFFER_BINDING_ARB     0x88EF
/* GL_EXT_framebuffer_object */
#ifndef GL_EXT_framebuffer_object
#define GL_EXT_framebuffer_object 1
#define GL_FRAMEBUFFER_EXT                     0x8D40
#define GL_RENDERBUFFER_EXT                    0x8D41
#define GL_STENCIL_INDEX1_EXT                  0x8D46
#define GL_STENCIL_INDEX4_EXT                  0x8D47
#define GL_STENCIL_INDEX8_EXT                  0x8D48
#define GL_STENCIL_INDEX16_EXT                 0x8D49
#define GL_RENDERBUFFER_WIDTH_EXT              0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT             0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT    0x8D44
#define GL_RENDERBUFFER_RED_SIZE_EXT           0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT         0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT          0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT         0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT         0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT       0x8D55
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT            0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT            0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT          0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT  0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT     0x8CD4
#define GL_COLOR_ATTACHMENT0_EXT                0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT                0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT                0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT                0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT                0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT                0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT                0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT                0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT                0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT                0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT               0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT               0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT               0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT               0x8CED
#define GL_COLOR_ATTACHMENT14_EXT               0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT               0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT                 0x8D00
#define GL_STENCIL_ATTACHMENT_EXT               0x8D20
#define GL_FRAMEBUFFER_COMPLETE_EXT                          0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT             0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT     0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT             0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT                0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT            0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT            0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT                       0x8CDD
#define GL_FRAMEBUFFER_BINDING_EXT             0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT            0x8CA7
#define GL_MAX_COLOR_ATTACHMENTS_EXT           0x8CDF
#define GL_MAX_RENDERBUFFER_SIZE_EXT           0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT   0x0506

#endif
typedef GLboolean (APIENTRY * PGLFNGLISRENDERBUFFEREXTPROC)(GLuint renderbuffer);
typedef void (APIENTRY * PGLFNGLBINDRENDERBUFFEREXTPROC)(GLenum target, GLuint renderbuffer);
typedef void (APIENTRY * PGLFNGLDELETERENDERBUFFERSEXTPROC)(GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRY * PGLFNGLGENRENDERBUFFERSEXTPROC)(GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRY * PGLFNGLRENDERBUFFERSTORAGEEXTPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY * PGLFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)(GLenum target, GLenum pname, GLint *params);
typedef GLboolean (APIENTRY * PGLFNGLISFRAMEBUFFEREXTPROC)(GLuint framebuffer);
typedef void (APIENTRY * PGLFNGLBINDFRAMEBUFFEREXTPROC)(GLenum target, GLuint framebuffer);
typedef void (APIENTRY * PGLFNGLDELETEFRAMEBUFFERSEXTPROC)(GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY * PGLFNGLGENFRAMEBUFFERSEXTPROC)(GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRY * PGLFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)(GLenum target);
typedef void (APIENTRY * PGLFNGLFRAMEBUFFERTEXTURE1DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY * PGLFNGLFRAMEBUFFERTEXTURE2DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY * PGLFNGLFRAMEBUFFERTEXTURE3DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
typedef void (APIENTRY * PGLFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRY * PGLFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGLGENERATEMIPMAPEXTPROC)(GLenum target);
/* GL_EXT_framebuffer_blit */
#ifndef GL_EXT_framebuffer_blit
#define GL_EXT_framebuffer_blit 1
#define GL_READ_FRAMEBUFFER_EXT                0x8CA8
#define GL_DRAW_FRAMEBUFFER_EXT                0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_EXT        0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_EXT        0x8CAA
#endif
typedef void (APIENTRY * PGLFNGLBLITFRAMEBUFFEREXTPROC) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
/* GL_EXT_secondary_color */
#ifndef GL_EXT_secondary_color
#define GL_EXT_secondary_color 1
#define GL_COLOR_SUM_EXT                     0x8458
#define GL_CURRENT_SECONDARY_COLOR_EXT       0x8459
#define GL_SECONDARY_COLOR_ARRAY_SIZE_EXT    0x845A
#define GL_SECONDARY_COLOR_ARRAY_TYPE_EXT    0x845B
#define GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT  0x845C
#define GL_SECONDARY_COLOR_ARRAY_POINTER_EXT 0x845D
#define GL_SECONDARY_COLOR_ARRAY_EXT         0x845E
#endif
typedef void (APIENTRY * PGLFNGLSECONDARYCOLOR3FEXTPROC) (GLfloat red, GLfloat green, GLfloat blue);
typedef void (APIENTRY * PGLFNGLSECONDARYCOLOR3FVEXTPROC) (const GLfloat *v);
typedef void (APIENTRY * PGLFNGLSECONDARYCOLOR3UBEXTPROC) (GLubyte red, GLubyte green, GLubyte blue);
typedef void (APIENTRY * PGLFNGLSECONDARYCOLORPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
/* GL_EXT_paletted_texture */
#ifndef GL_EXT_paletted_texture
#define GL_EXT_paletted_texture 1
#define GL_COLOR_INDEX1_EXT               0x80E2
#define GL_COLOR_INDEX2_EXT               0x80E3
#define GL_COLOR_INDEX4_EXT               0x80E4
#define GL_COLOR_INDEX8_EXT               0x80E5
#define GL_COLOR_INDEX12_EXT              0x80E6
#define GL_COLOR_INDEX16_EXT              0x80E7
#define GL_TEXTURE_INDEX_SIZE_EXT         0x80ED
#endif
typedef void (APIENTRY * PGLFNGLCOLORTABLEEXTPROC) (GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
/* GL_EXT_point_parameters */
#ifndef GL_EXT_point_parameters
#define GL_EXT_point_parameters 1
#define GL_POINT_SIZE_MIN_EXT             0x8126
#define GL_POINT_SIZE_MAX_EXT             0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT  0x8128
#define GL_DISTANCE_ATTENUATION_EXT       0x8129
#endif
typedef void (APIENTRY * PGLFNGLPOINTPARAMETERFEXTPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PGLFNGLPOINTPARAMETERFVEXTPROC) (GLenum pname, const GLfloat *params);
/* GL_EXT_texture3D */
#ifndef GL_EXT_texture3D
#define GL_PACK_SKIP_IMAGES_EXT           0x806B
#define GL_PACK_IMAGE_HEIGHT_EXT          0x806C
#define GL_UNPACK_SKIP_IMAGES_EXT         0x806D
#define GL_UNPACK_IMAGE_HEIGHT_EXT        0x806E
#define GL_TEXTURE_3D_EXT                 0x806F
#define GL_PROXY_TEXTURE_3D_EXT           0x8070
#define GL_TEXTURE_DEPTH_EXT              0x8071
#define GL_TEXTURE_WRAP_R_EXT             0x8072
#define GL_MAX_3D_TEXTURE_SIZE_EXT        0x8073
#endif
typedef void (APIENTRY * PGLFNGLTEXIMAGE3DEXTPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY * PGLFNGLTEXSUBIMAGE3DEXTPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
/* GL_EXT_texture_env_combine */
#ifndef GL_EXT_texture_env_combine
#define GL_EXT_texture_env_combine 1
#define GL_COMBINE_EXT                    0x8570
#define GL_COMBINE_RGB_EXT                0x8571
#define GL_COMBINE_ALPHA_EXT              0x8572
#define GL_RGB_SCALE_EXT                  0x8573
#define GL_ADD_SIGNED_EXT                 0x8574
#define GL_INTERPOLATE_EXT                0x8575
#define GL_SUBTRACT_EXT                   0x84E7
#define GL_CONSTANT_EXT                   0x8576
#define GL_PRIMARY_COLOR_EXT              0x8577
#define GL_PREVIOUS_EXT                   0x8578
#define GL_SOURCE0_RGB_EXT                0x8580
#define GL_SOURCE1_RGB_EXT                0x8581
#define GL_SOURCE2_RGB_EXT                0x8582
#define GL_SOURCE3_RGB_EXT                0x8583
#define GL_SOURCE4_RGB_EXT                0x8584
#define GL_SOURCE5_RGB_EXT                0x8585
#define GL_SOURCE6_RGB_EXT                0x8586
#define GL_SOURCE7_RGB_EXT                0x8587
#define GL_SOURCE0_ALPHA_EXT              0x8588
#define GL_SOURCE1_ALPHA_EXT              0x8589
#define GL_SOURCE2_ALPHA_EXT              0x858A
#define GL_SOURCE3_ALPHA_EXT              0x858B
#define GL_SOURCE4_ALPHA_EXT              0x858C
#define GL_SOURCE5_ALPHA_EXT              0x858D
#define GL_SOURCE6_ALPHA_EXT              0x858E
#define GL_SOURCE7_ALPHA_EXT              0x858F
#define GL_OPERAND0_RGB_EXT               0x8590
#define GL_OPERAND1_RGB_EXT               0x8591
#define GL_OPERAND2_RGB_EXT               0x8592
#define GL_OPERAND3_RGB_EXT               0x8593
#define GL_OPERAND4_RGB_EXT               0x8594
#define GL_OPERAND5_RGB_EXT               0x8595
#define GL_OPERAND6_RGB_EXT               0x8596
#define GL_OPERAND7_RGB_EXT               0x8597
#define GL_OPERAND0_ALPHA_EXT             0x8598
#define GL_OPERAND1_ALPHA_EXT             0x8599
#define GL_OPERAND2_ALPHA_EXT             0x859A
#define GL_OPERAND3_ALPHA_EXT             0x859B
#define GL_OPERAND4_ALPHA_EXT             0x859C
#define GL_OPERAND5_ALPHA_EXT             0x859D
#define GL_OPERAND6_ALPHA_EXT             0x859E
#define GL_OPERAND7_ALPHA_EXT             0x859F
#endif
/* GL_EXT_texture_env_dot3 */
#ifndef GL_EXT_texture_env_dot3
#define GL_EXT_texture_env_dot3 1
// #define GL_DOT3_RGB_EXT			  0x8740
// #define GL_DOT3_RGBA_EXT		  0x8741
#endif
/* GL_EXT_texture_lod_bias */
#ifndef GL_EXT_texture_lod_bias
#define GL_EXT_texture_lod_bias 1
#define GL_MAX_TEXTURE_LOD_BIAS_EXT       0x84FD
#define GL_TEXTURE_FILTER_CONTROL_EXT     0x8500
#define GL_TEXTURE_LOD_BIAS_EXT           0x8501
#endif
/* GL_ARB_texture_border_clamp */
#ifndef GL_ARB_texture_border_clamp
#define GL_ARB_texture_border_clamp 1
#define GL_CLAMP_TO_BORDER_ARB            0x812D
#endif
/* GL_ARB_texture_mirrored_repeat (full support GL1.4) */
#ifndef GL_ARB_texture_mirrored_repeat
#define GL_ARB_texture_mirrored_repeat 1
#define GL_MIRRORED_REPEAT_ARB            0x8370
#endif
/* GL_ATI_texture_mirror_once */
#ifndef GL_ATI_texture_mirror_once
#define GL_ATI_texture_mirror_once 1
#define GL_MIRROR_CLAMP_ATI               0x8742
#define GL_MIRROR_CLAMP_TO_EDGE_ATI       0x8743
#endif
/* GL_ARB_texture_env_dot3 */
#ifndef GL_ARB_texture_env_dot3
#define GL_ARB_texture_env_dot3 1
#define GL_DOT3_RGB_ARB                   0x86AE
#define GL_DOT3_RGBA_ARB                  0x86AF
#endif
/* GL_EXT_texture_env_dot3 */
#ifndef GL_EXT_texture_env_dot3
#define GL_EXT_texture_env_dot3 1
//#define GL_DOT3_RGB_EXT                   0x8740
#define GL_DOT3_RGBA_EXT                  0x8741
#endif
/* GL_ARB_texture_float */
#ifndef GL_ARB_texture_float
#define GL_ARB_texture_float 1
#define GL_RGBA32F_ARB                    0x8814
#define GL_RGB32F_ARB                     0x8815
#define GL_RGBA16F_ARB                    0x881A
#define GL_RGB16F_ARB                     0x881B
#endif
/* GL_ARB_half_float_pixel */
#ifndef GL_ARB_half_float_pixel
#define GL_ARB_half_float_pixel
#define GL_HALF_FLOAT_ARB                 0x140B
#endif
/* GL_ARB_vertex_program */
#ifndef GL_ARB_vertex_program
#define GL_ARB_vertex_program 1
#define GL_VERTEX_PROGRAM_ARB             0x8620
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB  0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB    0x8643
#define GL_COLOR_SUM_ARB                  0x8458
#define GL_PROGRAM_FORMAT_ASCII_ARB       0x8875
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB   0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB   0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB 0x886A
#define GL_CURRENT_VERTEX_ATTRIB_ARB      0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB 0x8645
#define GL_PROGRAM_LENGTH_ARB             0x8627
#define GL_PROGRAM_FORMAT_ARB             0x8876
#define GL_PROGRAM_BINDING_ARB            0x8677
#define GL_PROGRAM_INSTRUCTIONS_ARB       0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB   0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB        0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB    0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A7
#define GL_PROGRAM_PARAMETERS_ARB         0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB     0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB  0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AB
#define GL_PROGRAM_ATTRIBS_ARB            0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB        0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB     0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB  0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB 0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB 0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB 0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#define GL_PROGRAM_STRING_ARB             0x8628
#define GL_PROGRAM_ERROR_POSITION_ARB     0x864B
#define GL_CURRENT_MATRIX_ARB             0x8641
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB   0x88B7
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB 0x8640
#define GL_MAX_VERTEX_ATTRIBS_ARB         0x8869
#define GL_MAX_PROGRAM_MATRICES_ARB       0x862F
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB 0x862E
#define GL_PROGRAM_ERROR_STRING_ARB       0x8874
#define GL_MATRIX0_ARB                    0x88C0
#define GL_MATRIX1_ARB                    0x88C1
#define GL_MATRIX2_ARB                    0x88C2
#define GL_MATRIX3_ARB                    0x88C3
#define GL_MATRIX4_ARB                    0x88C4
#define GL_MATRIX5_ARB                    0x88C5
#define GL_MATRIX6_ARB                    0x88C6
#define GL_MATRIX7_ARB                    0x88C7
#define GL_MATRIX8_ARB                    0x88C8
#define GL_MATRIX9_ARB                    0x88C9
#define GL_MATRIX10_ARB                   0x88CA
#define GL_MATRIX11_ARB                   0x88CB
#define GL_MATRIX12_ARB                   0x88CC
#define GL_MATRIX13_ARB                   0x88CD
#define GL_MATRIX14_ARB                   0x88CE
#define GL_MATRIX15_ARB                   0x88CF
#define GL_MATRIX16_ARB                   0x88D0
#define GL_MATRIX17_ARB                   0x88D1
#define GL_MATRIX18_ARB                   0x88D2
#define GL_MATRIX19_ARB                   0x88D3
#define GL_MATRIX20_ARB                   0x88D4
#define GL_MATRIX21_ARB                   0x88D5
#define GL_MATRIX22_ARB                   0x88D6
#define GL_MATRIX23_ARB                   0x88D7
#define GL_MATRIX24_ARB                   0x88D8
#define GL_MATRIX25_ARB                   0x88D9
#define GL_MATRIX26_ARB                   0x88DA
#define GL_MATRIX27_ARB                   0x88DB
#define GL_MATRIX28_ARB                   0x88DC
#define GL_MATRIX29_ARB                   0x88DD
#define GL_MATRIX30_ARB                   0x88DE
#define GL_MATRIX31_ARB                   0x88DF
#endif
typedef void (APIENTRY * PGLFNVERTEXATTRIB1DARBPROC) (GLuint index, GLdouble x);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1FARBPROC) (GLuint index, GLfloat x);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1SARBPROC) (GLuint index, GLshort x);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2DARBPROC) (GLuint index, GLdouble x, GLdouble y);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2FARBPROC) (GLuint index, GLfloat x, GLfloat y);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2SARBPROC) (GLuint index, GLshort x, GLshort y);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3DARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3FARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3SARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NBVARBPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NIVARBPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NSVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NUBARBPROC) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NUBVARBPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NUIVARBPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NUSVARBPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4BVARBPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4DARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4FARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4IVARBPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4SARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4UBVARBPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4UIVARBPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4USVARBPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIBPOINTERARBPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY * PGLFNENABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (APIENTRY * PGLFNDISABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (APIENTRY * PGLFNPROGRAMSTRINGARBPROC) (GLenum target, GLenum format, GLsizei len, const GLvoid *string);
typedef void (APIENTRY * PGLFNBINDPROGRAMARBPROC) (GLenum target, GLuint program);
typedef void (APIENTRY * PGLFNDELETEPROGRAMSARBPROC) (GLsizei n, const GLuint *programs);
typedef void (APIENTRY * PGLFNGENPROGRAMSARBPROC) (GLsizei n, GLuint *programs);
typedef void (APIENTRY * PGLFNPROGRAMENVPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PGLFNPROGRAMENVPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRY * PGLFNPROGRAMENVPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PGLFNPROGRAMENVPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRY * PGLFNPROGRAMLOCALPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PGLFNPROGRAMLOCALPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRY * PGLFNPROGRAMLOCALPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PGLFNPROGRAMLOCALPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRY * PGLFNGETPROGRAMENVPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRY * PGLFNGETPROGRAMENVPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRY * PGLFNGETPROGRAMLOCALPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRY * PGLFNGETPROGRAMLOCALPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRY * PGLFNGETPROGRAMIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETPROGRAMSTRINGARBPROC) (GLenum target, GLenum pname, GLvoid *string);
typedef void (APIENTRY * PGLFNGETVERTEXATTRIBDVARBPROC) (GLuint index, GLenum pname, GLdouble *params);
typedef void (APIENTRY * PGLFNGETVERTEXATTRIBFVARBPROC) (GLuint index, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PGLFNGETVERTEXATTRIBIVARBPROC) (GLuint index, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETVERTEXATTRIBPOINTERVARBPROC) (GLuint index, GLenum pname, GLvoid* *pointer);
typedef GLboolean (APIENTRY * PGLFNISPROGRAMARBPROC) (GLuint program);
#ifndef GL_ARB_fragment_program
#define GL_ARB_fragment_program 1
#define GL_FRAGMENT_PROGRAM_ARB           0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB   0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB   0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB   0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB 0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB 0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB 0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x8810
#define GL_MAX_TEXTURE_COORDS_ARB         0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB    0x8872
/* All ARB_fragment_program entry points are shared with ARB_vertex_program. */
#endif
/* GL_ARB_vertex_buffer_object */
#ifndef GL_ARB_vertex_buffer_object
#define GL_BUFFER_SIZE_ARB                0x8764
#define GL_BUFFER_USAGE_ARB               0x8765
#define GL_ARRAY_BUFFER_ARB               0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB       0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB       0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB  0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB  0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB   0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB   0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB   0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB       0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB 0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB  0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB          0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB   0x889F
#define GL_READ_ONLY_ARB                  0x88B8
#define GL_WRITE_ONLY_ARB                 0x88B9
#define GL_READ_WRITE_ARB                 0x88BA
#define GL_BUFFER_ACCESS_ARB              0x88BB
#define GL_BUFFER_MAPPED_ARB              0x88BC
#define GL_BUFFER_MAP_POINTER_ARB         0x88BD
#define GL_STREAM_DRAW_ARB                0x88E0
#define GL_STREAM_READ_ARB                0x88E1
#define GL_STREAM_COPY_ARB                0x88E2
#define GL_STATIC_DRAW_ARB                0x88E4
#define GL_STATIC_READ_ARB                0x88E5
#define GL_STATIC_COPY_ARB                0x88E6
#define GL_DYNAMIC_DRAW_ARB               0x88E8
#define GL_DYNAMIC_READ_ARB               0x88E9
#define GL_DYNAMIC_COPY_ARB               0x88EA
#endif
typedef void (APIENTRY * PGLFNBINDBUFFERARBPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRY * PGLFNDELETEBUFFERSARBPROC) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRY * PGLFNGENBUFFERSARBPROC) (GLsizei n, GLuint *buffers);
typedef GLboolean (APIENTRY * PGLFNISBUFFERARBPROC) (GLuint buffer);
typedef void (APIENTRY * PGLFNBUFFERDATAARBPROC) (GLenum target, ptrdiff_t size, const GLvoid *data, GLenum usage);
typedef void (APIENTRY * PGLFNBUFFERSUBDATAARBPROC) (GLenum target, ptrdiff_t offset, ptrdiff_t size, const GLvoid *data);
typedef void (APIENTRY * PGLFNGETBUFFERSUBDATAARBPROC) (GLenum target, ptrdiff_t offset, ptrdiff_t size, GLvoid *data);
typedef GLvoid* (APIENTRY * PGLFNMAPBUFFERARBPROC) (GLenum target, GLenum access);
typedef GLboolean (APIENTRY * PGLFNUNMAPBUFFERARBPROC) (GLenum target);
typedef void (APIENTRY * PGLFNGETBUFFERPARAMETERIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETBUFFERPOINTERVARBPROC) (GLenum target, GLenum pname, GLvoid* *params);
/* GL_EXT_fog_coord */
#ifndef GL_EXT_fog_coord
#define GL_EXT_fog_coord 1
#define GL_FOG_COORDINATE_SOURCE_EXT            0x8450
#define GL_FOG_COORDINATE_EXT                   0x8451
#define GL_FRAGMENT_DEPTH_EXT                   0x8452
#define GL_CURRENT_FOG_COORDINATE_EXT           0x8453
#define GL_FOG_COORDINATE_ARRAY_TYPE_EXT        0x8454
#define GL_FOG_COORDINATE_ARRAY_STRIDE_EXT      0x8455
#define GL_FOG_COORDINATE_ARRAY_POINTER_EXT     0x8456
#define GL_FOG_COORDINATE_ARRAY_EXT             0x8457
#endif /* GL_EXT_fog_coord */
typedef void (APIENTRY * PGLFNGLFOGCOORDFEXTPROC) (GLfloat intesity);
typedef void (APIENTRY * PGLFNGLFOGCOORDFVEXTPROC) (GLfloat intesity);
typedef void (APIENTRY * PGLFNGLFOGCOORDDEXTPROC) (GLfloat intesity);
typedef void (APIENTRY * PGLFNGLFOGCOORDDVEXTPROC) (GLfloat intesity);
typedef void (APIENTRY * PGLFNGLFOGCOORDPOINTEREXTPROC) (GLenum type, GLsizei stride, GLvoid *data);
/* GL_ARB_shader_objects (GLSL) */
#ifndef GL_ARB_shader_objects
#define GL_ARB_shader_objects 1
typedef char GLcharARB;
typedef unsigned int GLhandleARB;
#define GL_OBJECT_DELETE_STATUS_ARB             0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB            0x8B81
#define GL_OBJECT_LINK_STATUS_ARB               0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB           0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB           0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB          0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB           0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB 0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB      0x8B88
#endif
#ifndef GL_ARB_shading_language_100
#define GL_ARB_shading_language_100         1
#define GL_SHADING_LANGUAGE_VERSION_ARB     0x8B8C
#endif
#ifndef GL_ARB_fragment_shader
#define GL_ARB_fragment_shader 1
#define GL_FRAGMENT_SHADER_ARB                  0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB  0x8B49
#endif
#ifndef GL_ARB_vertex_shader
#define GL_ARB_vertex_shader 1
#define GL_VERTEX_SHADER_ARB              0x8B31
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB 0x8B4A
#define GL_PROGRAM_OBJECT_ARB             0x8B40
#define GL_SHADER_OBJECT_ARB              0x8B48
#define GL_OBJECT_TYPE_ARB                0x8B4E
#define GL_OBJECT_SUBTYPE_ARB             0x8B4F
#define GL_FLOAT_VEC2_ARB                 0x8B50
#define GL_FLOAT_VEC3_ARB                 0x8B51
#define GL_FLOAT_VEC4_ARB                 0x8B52
#define GL_INT_VEC2_ARB                   0x8B53
#define GL_INT_VEC3_ARB                   0x8B54
#define GL_INT_VEC4_ARB                   0x8B55
#define GL_BOOL_ARB                       0x8B56
#define GL_BOOL_VEC2_ARB                  0x8B57
#define GL_BOOL_VEC3_ARB                  0x8B58
#define GL_BOOL_VEC4_ARB                  0x8B59
#define GL_FLOAT_MAT2_ARB                 0x8B5A
#define GL_FLOAT_MAT3_ARB                 0x8B5B
#define GL_FLOAT_MAT4_ARB                 0x8B5C
#define GL_SAMPLER_1D_ARB                 0x8B5D
#define GL_SAMPLER_2D_ARB                 0x8B5E
#define GL_SAMPLER_3D_ARB                 0x8B5F
#define GL_SAMPLER_CUBE_ARB               0x8B60
#define GL_SAMPLER_1D_SHADOW_ARB          0x8B61
#define GL_SAMPLER_2D_SHADOW_ARB          0x8B62
#endif
typedef void (APIENTRY * WINED3D_PFNGLGETOBJECTPARAMETERIVARBPROC) (GLhandleARB obj, GLenum pname, GLint *params);
typedef void (APIENTRY * WINED3D_PFNGLGETOBJECTPARAMETERFVARBPROC) (GLhandleARB obj, GLenum pname, GLfloat *params);
typedef GLint (APIENTRY * WINED3D_PFNGLGETUNIFORMLOCATIONARBPROC) (GLhandleARB programObj, const GLcharARB *name);
typedef void (APIENTRY * WINED3D_PFNGLGETACTIVEUNIFORMARBPROC) (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM1IARBPROC) (GLint location, GLint v0);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM2IARBPROC) (GLint location, GLint v0, GLint v1);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM3IARBPROC) (GLint location, GLint v0, GLint v1, GLint v2);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM4IARBPROC) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM1FARBPROC) (GLint location, GLfloat v0);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM2FARBPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM3FARBPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM4FARBPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM1IVARBPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM2IVARBPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM3IVARBPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM4IVARBPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM1FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM2FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM3FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORM4FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORMMATRIX2FVARBPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORMMATRIX3FVARBPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * WINED3D_PFNGLUNIFORMMATRIX4FVARBPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * WINED3D_PFNGLGETUNIFORMFVARBPROC) (GLhandleARB programObj, GLint location, GLfloat *params);
typedef void (APIENTRY * WINED3D_PFNGLGETUNIFORMIVARBPROC) (GLhandleARB programObj, GLint location, GLint *params);
typedef void (APIENTRY * WINED3D_PFNGLGETINFOLOGARBPROC) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
typedef void (APIENTRY * WINED3D_PFNGLUSEPROGRAMOBJECTARBPROC) (GLhandleARB programObj);
typedef GLhandleARB (APIENTRY * WINED3D_PFNGLCREATESHADEROBJECTARBPROC) (GLenum shaderType);
typedef void (APIENTRY * WINED3D_PFNGLSHADERSOURCEARBPROC) (GLhandleARB shaderObj, GLsizei count, const GLcharARB* *string, const GLint *length);
typedef void (APIENTRY * WINED3D_PFNGLCOMPILESHADERARBPROC) (GLhandleARB shaderObj);
typedef GLhandleARB (APIENTRY * WINED3D_PFNGLCREATEPROGRAMOBJECTARBPROC) (void);
typedef void (APIENTRY * WINED3D_PFNGLATTACHOBJECTARBPROC) (GLhandleARB containerObj, GLhandleARB obj);
typedef void (APIENTRY * WINED3D_PFNGLLINKPROGRAMARBPROC) (GLhandleARB programObj);
typedef void (APIENTRY * WINED3D_PFNGLDETACHOBJECTARBPROC) (GLhandleARB containerObj, GLhandleARB attachedObj);
typedef void (APIENTRY * WINED3D_PFNGLDELETEOBJECTARBPROC) (GLhandleARB obj);
typedef void (APIENTRY * WINED3D_PFNGLVALIDATEPROGRAMARBPROC) (GLhandleARB programObj);
typedef void (APIENTRY * WINED3D_PFNGLGETATTACHEDOBJECTSARBPROC) (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
typedef GLhandleARB (APIENTRY * WINED3D_PFNGLGETHANDLEARBPROC) (GLenum pname);
typedef void (APIENTRY * WINED3D_PFNGLGETSHADERSOURCEARBPROC) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);
typedef void (APIENTRY * WINED3D_PFNGLBINDATTRIBLOCATIONARBPROC) (GLhandleARB programObj, GLuint index, const GLcharARB *name);
typedef GLint (APIENTRY * WINED3D_PFNGLGETATTRIBLOCATIONARBPROC) (GLhandleARB programObj, const GLcharARB *name);
/* GL_EXT_texture_compression_s3tc */
#ifndef GL_EXT_texture_compression_s3tc
#define GL_EXT_texture_compression_s3tc 1
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif
typedef void (APIENTRY * PGLFNCOMPRESSEDTEXIMAGE3DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRY * PGLFNCOMPRESSEDTEXIMAGE2DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRY * PGLFNCOMPRESSEDTEXIMAGE1DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRY * PGLFNCOMPRESSEDTEXSUBIMAGE3DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRY * PGLFNCOMPRESSEDTEXSUBIMAGE2DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRY * PGLFNCOMPRESSEDTEXSUBIMAGE1DPROC) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRY * PGLFNGETCOMPRESSEDTEXIMAGEPROC) (GLenum target, GLint level, void *img);
/* GL_EXT_stencil_wrap */
#ifndef GL_EXT_stencil_wrap
#define GL_EXT_stencil_wrap 1
#define GL_INCR_WRAP_EXT                  0x8507
#define GL_DECR_WRAP_EXT                  0x8508
#endif
/* GL_NV_fog_distance */
#ifndef GL_NV_fog_distance
#define GL_NV_fog_distance 1
#define GL_FOG_DISTANCE_MODE_NV           0x855A
#define GL_EYE_RADIAL_NV                  0x855B
#define GL_EYE_PLANE_ABSOLUTE_NV          0x855C
/* reuse GL_EYE_PLANE */
#endif
/* GL_NV_texgen_reflection */
#ifndef GL_NV_texgen_reflection
#define GL_NV_texgen_reflection 1
#define GL_NORMAL_MAP_NV                  0x8511
#define GL_REFLECTION_MAP_NV              0x8512
#endif
/* GL_NV_register_combiners */
#ifndef GL_NV_register_combiners
#define GL_NV_register_combiners 1
#define GL_REGISTER_COMBINERS_NV          0x8522
#define GL_VARIABLE_A_NV                  0x8523
#define GL_VARIABLE_B_NV                  0x8524
#define GL_VARIABLE_C_NV                  0x8525
#define GL_VARIABLE_D_NV                  0x8526
#define GL_VARIABLE_E_NV                  0x8527
#define GL_VARIABLE_F_NV                  0x8528
#define GL_VARIABLE_G_NV                  0x8529
#define GL_CONSTANT_COLOR0_NV             0x852A
#define GL_CONSTANT_COLOR1_NV             0x852B
#define GL_PRIMARY_COLOR_NV               0x852C
#define GL_SECONDARY_COLOR_NV             0x852D
#define GL_SPARE0_NV                      0x852E
#define GL_SPARE1_NV                      0x852F
#define GL_DISCARD_NV                     0x8530
#define GL_E_TIMES_F_NV                   0x8531
#define GL_SPARE0_PLUS_SECONDARY_COLOR_NV 0x8532
#define GL_UNSIGNED_IDENTITY_NV           0x8536
#define GL_UNSIGNED_INVERT_NV             0x8537
#define GL_EXPAND_NORMAL_NV               0x8538
#define GL_EXPAND_NEGATE_NV               0x8539
#define GL_HALF_BIAS_NORMAL_NV            0x853A
#define GL_HALF_BIAS_NEGATE_NV            0x853B
#define GL_SIGNED_IDENTITY_NV             0x853C
#define GL_SIGNED_NEGATE_NV               0x853D
#define GL_SCALE_BY_TWO_NV                0x853E
#define GL_SCALE_BY_FOUR_NV               0x853F
#define GL_SCALE_BY_ONE_HALF_NV           0x8540
#define GL_BIAS_BY_NEGATIVE_ONE_HALF_NV   0x8541
#define GL_COMBINER_INPUT_NV              0x8542
#define GL_COMBINER_MAPPING_NV            0x8543
#define GL_COMBINER_COMPONENT_USAGE_NV    0x8544
#define GL_COMBINER_AB_DOT_PRODUCT_NV     0x8545
#define GL_COMBINER_CD_DOT_PRODUCT_NV     0x8546
#define GL_COMBINER_MUX_SUM_NV            0x8547
#define GL_COMBINER_SCALE_NV              0x8548
#define GL_COMBINER_BIAS_NV               0x8549
#define GL_COMBINER_AB_OUTPUT_NV          0x854A
#define GL_COMBINER_CD_OUTPUT_NV          0x854B
#define GL_COMBINER_SUM_OUTPUT_NV         0x854C
#define GL_MAX_GENERAL_COMBINERS_NV       0x854D
#define GL_NUM_GENERAL_COMBINERS_NV       0x854E
#define GL_COLOR_SUM_CLAMP_NV             0x854F
#define GL_COMBINER0_NV                   0x8550
#define GL_COMBINER1_NV                   0x8551
#define GL_COMBINER2_NV                   0x8552
#define GL_COMBINER3_NV                   0x8553
#define GL_COMBINER4_NV                   0x8554
#define GL_COMBINER5_NV                   0x8555
#define GL_COMBINER6_NV                   0x8556
#define GL_COMBINER7_NV                   0x8557
/* reuse GL_TEXTURE0_ARB */
/* reuse GL_TEXTURE1_ARB */
/* reuse GL_ZERO */
/* reuse GL_NONE */
/* reuse GL_FOG */
#endif
typedef void (APIENTRY * PGLFNCOMBINERPARAMETERFVNVPROC) (GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PGLFNCOMBINERPARAMETERFNVPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PGLFNCOMBINERPARAMETERIVNVPROC) (GLenum pname, const GLint *params);
typedef void (APIENTRY * PGLFNCOMBINERPARAMETERINVPROC) (GLenum pname, GLint param);
typedef void (APIENTRY * PGLFNCOMBINERINPUTNVPROC) (GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
typedef void (APIENTRY * PGLFNCOMBINEROUTPUTNVPROC) (GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
typedef void (APIENTRY * PGLFNFINALCOMBINERINPUTNVPROC) (GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
typedef void (APIENTRY * PGLFNGETCOMBINERINPUTPARAMETERFVNVPROC) (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PGLFNGETCOMBINERINPUTPARAMETERIVNVPROC) (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETCOMBINEROUTPUTPARAMETERFVNVPROC) (GLenum stage, GLenum portion, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PGLFNGETCOMBINEROUTPUTPARAMETERIVNVPROC) (GLenum stage, GLenum portion, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETFINALCOMBINERINPUTPARAMETERFVNVPROC) (GLenum variable, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PGLFNGETFINALCOMBINERINPUTPARAMETERIVNVPROC) (GLenum variable, GLenum pname, GLint *params);
/* GL_NV_register_combiners2 */
#ifndef GL_NV_register_combiners2
#define GL_NV_register_combiners2 1
#define GL_PER_STAGE_CONSTANTS_NV         0x8535
#endif
typedef void (APIENTRY * PGLFNCOMBINERSTAGEPARAMETERFVNVPROC) (GLenum stage, GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PGLFNGETCOMBINERSTAGEPARAMETERFVNVPROC) (GLenum stage, GLenum pname, GLfloat *params);
/* GL_NV_texture_shader */
#ifndef GL_NV_texture_shader
#define GL_NV_texture_shader 1
#define GL_OFFSET_TEXTURE_RECTANGLE_NV    0x864C
#define GL_OFFSET_TEXTURE_RECTANGLE_SCALE_NV 0x864D
#define GL_DOT_PRODUCT_TEXTURE_RECTANGLE_NV 0x864E
#define GL_RGBA_UNSIGNED_DOT_PRODUCT_MAPPING_NV 0x86D9
#define GL_UNSIGNED_INT_S8_S8_8_8_NV      0x86DA
#define GL_UNSIGNED_INT_8_8_S8_S8_REV_NV  0x86DB
#define GL_DSDT_MAG_INTENSITY_NV          0x86DC
#define GL_SHADER_CONSISTENT_NV           0x86DD
#define GL_TEXTURE_SHADER_NV              0x86DE
#define GL_SHADER_OPERATION_NV            0x86DF
#define GL_CULL_MODES_NV                  0x86E0
#define GL_OFFSET_TEXTURE_MATRIX_NV       0x86E1
#define GL_OFFSET_TEXTURE_SCALE_NV        0x86E2
#define GL_OFFSET_TEXTURE_BIAS_NV         0x86E3
#define GL_OFFSET_TEXTURE_2D_MATRIX_NV    GL_OFFSET_TEXTURE_MATRIX_NV
#define GL_OFFSET_TEXTURE_2D_SCALE_NV     GL_OFFSET_TEXTURE_SCALE_NV
#define GL_OFFSET_TEXTURE_2D_BIAS_NV      GL_OFFSET_TEXTURE_BIAS_NV
#define GL_PREVIOUS_TEXTURE_INPUT_NV      0x86E4
#define GL_CONST_EYE_NV                   0x86E5
#define GL_PASS_THROUGH_NV                0x86E6
#define GL_CULL_FRAGMENT_NV               0x86E7
#define GL_OFFSET_TEXTURE_2D_NV           0x86E8
#define GL_DEPENDENT_AR_TEXTURE_2D_NV     0x86E9
#define GL_DEPENDENT_GB_TEXTURE_2D_NV     0x86EA
#define GL_DOT_PRODUCT_NV                 0x86EC
#define GL_DOT_PRODUCT_DEPTH_REPLACE_NV   0x86ED
#define GL_DOT_PRODUCT_TEXTURE_2D_NV      0x86EE
#define GL_DOT_PRODUCT_TEXTURE_CUBE_MAP_NV 0x86F0
#define GL_DOT_PRODUCT_DIFFUSE_CUBE_MAP_NV 0x86F1
#define GL_DOT_PRODUCT_REFLECT_CUBE_MAP_NV 0x86F2
#define GL_DOT_PRODUCT_CONST_EYE_REFLECT_CUBE_MAP_NV 0x86F3
#define GL_HILO_NV                        0x86F4
#define GL_DSDT_NV                        0x86F5
#define GL_DSDT_MAG_NV                    0x86F6
#define GL_DSDT_MAG_VIB_NV                0x86F7
#define GL_HILO16_NV                      0x86F8
#define GL_SIGNED_HILO_NV                 0x86F9
#define GL_SIGNED_HILO16_NV               0x86FA
#define GL_SIGNED_RGBA_NV                 0x86FB
#define GL_SIGNED_RGBA8_NV                0x86FC
#define GL_SIGNED_RGB_NV                  0x86FE
#define GL_SIGNED_RGB8_NV                 0x86FF
#define GL_SIGNED_LUMINANCE_NV            0x8701
#define GL_SIGNED_LUMINANCE8_NV           0x8702
#define GL_SIGNED_LUMINANCE_ALPHA_NV      0x8703
#define GL_SIGNED_LUMINANCE8_ALPHA8_NV    0x8704
#define GL_SIGNED_ALPHA_NV                0x8705
#define GL_SIGNED_ALPHA8_NV               0x8706
#define GL_SIGNED_INTENSITY_NV            0x8707
#define GL_SIGNED_INTENSITY8_NV           0x8708
#define GL_DSDT8_NV                       0x8709
#define GL_DSDT8_MAG8_NV                  0x870A
#define GL_DSDT8_MAG8_INTENSITY8_NV       0x870B
#define GL_SIGNED_RGB_UNSIGNED_ALPHA_NV   0x870C
#define GL_SIGNED_RGB8_UNSIGNED_ALPHA8_NV 0x870D
#define GL_HI_SCALE_NV                    0x870E
#define GL_LO_SCALE_NV                    0x870F
#define GL_DS_SCALE_NV                    0x8710
#define GL_DT_SCALE_NV                    0x8711
#define GL_MAGNITUDE_SCALE_NV             0x8712
#define GL_VIBRANCE_SCALE_NV              0x8713
#define GL_HI_BIAS_NV                     0x8714
#define GL_LO_BIAS_NV                     0x8715
#define GL_DS_BIAS_NV                     0x8716
#define GL_DT_BIAS_NV                     0x8717
#define GL_MAGNITUDE_BIAS_NV              0x8718
#define GL_VIBRANCE_BIAS_NV               0x8719
#define GL_TEXTURE_BORDER_VALUES_NV       0x871A
#define GL_TEXTURE_HI_SIZE_NV             0x871B
#define GL_TEXTURE_LO_SIZE_NV             0x871C
#define GL_TEXTURE_DS_SIZE_NV             0x871D
#define GL_TEXTURE_DT_SIZE_NV             0x871E
#define GL_TEXTURE_MAG_SIZE_NV            0x871F
#endif
/* GL_NV_texture_shader2 */
#ifndef GL_NV_texture_shader2
#define GL_NV_texture_shader2 1
#define GL_DOT_PRODUCT_TEXTURE_3D_NV      0x86EF
#endif
/* GL_NV_texture_shader3 */
#ifndef GL_NV_texture_shader3
#define GL_NV_texture_shader3 1
#define GL_OFFSET_PROJECTIVE_TEXTURE_2D_NV 0x8850
#define GL_OFFSET_PROJECTIVE_TEXTURE_2D_SCALE_NV 0x8851
#define GL_OFFSET_PROJECTIVE_TEXTURE_RECTANGLE_NV 0x8852
#define GL_OFFSET_PROJECTIVE_TEXTURE_RECTANGLE_SCALE_NV 0x8853
#define GL_OFFSET_HILO_TEXTURE_2D_NV      0x8854
#define GL_OFFSET_HILO_TEXTURE_RECTANGLE_NV 0x8855
#define GL_OFFSET_HILO_PROJECTIVE_TEXTURE_2D_NV 0x8856
#define GL_OFFSET_HILO_PROJECTIVE_TEXTURE_RECTANGLE_NV 0x8857
#define GL_DEPENDENT_HILO_TEXTURE_2D_NV   0x8858
#define GL_DEPENDENT_RGB_TEXTURE_3D_NV    0x8859
#define GL_DEPENDENT_RGB_TEXTURE_CUBE_MAP_NV 0x885A
#define GL_DOT_PRODUCT_PASS_THROUGH_NV    0x885B
#define GL_DOT_PRODUCT_TEXTURE_1D_NV      0x885C
#define GL_DOT_PRODUCT_AFFINE_DEPTH_REPLACE_NV 0x885D
#define GL_HILO8_NV                       0x885E
#define GL_SIGNED_HILO8_NV                0x885F
#define GL_FORCE_BLUE_TO_ONE_NV           0x8860
#endif
/* GL_ATI_texture_env_combine3 */
#ifndef GL_ATI_texture_env_combine3
#define GL_ATI_texture_env_combine3 1
#define GL_MODULATE_ADD_ATI               0x8744
#define GL_MODULATE_SIGNED_ADD_ATI        0x8745
#define GL_MODULATE_SUBTRACT_ATI          0x8746
/* #define ONE */
/* #define ZERO */
#endif

/**
 * Point sprites 
 */
/* GL_ARB_point_sprite */
#ifndef GL_ARB_point_sprite
#define GL_ARB_point_sprite 1
#define GL_POINT_SPRITE_ARB               0x8861
#define GL_COORD_REPLACE_ARB              0x8862
#endif
/**
 * @TODO: GL_NV_point_sprite 
 */

/**
 * Occlusion Queries 
 */
/* GL_ARB_occlusion_query */
#ifndef GL_ARB_occlusion_query
#define GL_ARB_occlusion_query 1
#define GL_SAMPLES_PASSED_ARB                             0x8914
#define GL_QUERY_COUNTER_BITS_ARB                         0x8864
#define GL_CURRENT_QUERY_ARB                              0x8865
#define GL_QUERY_RESULT_ARB                               0x8866
#define GL_QUERY_RESULT_AVAILABLE_ARB                     0x8867
#endif
typedef void (APIENTRY * PGLFNGENQUERIESARBPROC) (GLsizei n, GLuint *queries);
typedef void (APIENTRY * PGLFNDELETEQUERIESARBPROC) (GLsizei n, const GLuint *queries);
typedef GLboolean (APIENTRY * PGLFNISQUERYARBPROC) (GLuint query);
typedef void (APIENTRY * PGLFNBEGINQUERYARBPROC) (GLenum target, GLuint query);
typedef void (APIENTRY * PGLFNENDQUERYARBPROC) (GLenum target);
typedef void (APIENTRY * PGLFNGETQUERYIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETQUERYOBJECTIVARBPROC) (GLuint query, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETQUERYOBJECTUIVARBPROC) (GLuint query, GLenum pname, GLuint *params);
/* GL_HP_occlusion_test isn't complete, but it's constants are used by GL_NV_occlusion_query */
#ifndef GL_HP_occlusion_test
#define GL_HP_occlusion_test 1
#define GL_OCCLUSION_TEST_HP                 0x8165
#define GL_OCCLUSION_TEST_RESULT_HP          0x8165
#endif
/*  GL_NV_occlusion_query */
#ifndef GL_NV_occlusion_query
#define GL_NV_occlusion_query 1
#define GL_PIXEL_COUNTER_BITS_NV          0x8864
#define GL_CURRENT_OCCLUSION_QUERY_ID_NV  0x8865
#define GL_PIXEL_COUNT_NV                 0x8866
#define GL_PIXEL_COUNT_AVAILABLE_NV       0x8867
#endif
typedef void (APIENTRY * PGLFNGENOCCLUSIONQUERIESNVPROC) (GLsizei n, GLuint *ids);
typedef void (APIENTRY * PGLFNDELETEOCCLUSIONQUERIESNVPROC) (GLsizei n, const GLuint *ids);
typedef GLboolean (APIENTRY * PGLFNISOCCLUSIONQUERYNVPROC) (GLuint id);
typedef void (APIENTRY * PGLFNBEGINOCCLUSIONQUERYNVPROC) (GLuint id);
typedef void (APIENTRY * PGLFNENDOCCLUSIONQUERYNVPROC) (void);
typedef void (APIENTRY * PGLFNGETOCCLUSIONQUERYIVNVPROC) (GLuint id, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETOCCLUSIONQUERYUIVNVPROC) (GLuint id, GLenum pname, GLuint *params);
/* GL_EXT_stencil_two_side */
#ifndef GL_EXT_stencil_two_side
#define GL_EXT_stencil_two_side 1
#define GL_STENCIL_TEST_TWO_SIDE_EXT      0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT        0x8911
#endif
typedef void (APIENTRY * PGLFNACTIVESTENCILFACEEXTPROC) (GLenum face);
/* GL_ATI_separate_stencil */
#ifndef GL_ATI_separate_stencil
#define GL_ATI_separate_stencil 1
#define GL_STENCIL_BACK_FUNC_ATI          0x8800
#define GL_STENCIL_BACK_FAIL_ATI          0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL_ATI 0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS_ATI 0x8803
#endif
typedef void (APIENTRY * PGLFNSTENCILOPSEPARATEATIPROC) (GLenum, GLenum, GLenum, GLenum);
typedef void (APIENTRY * PGLFNSTENCILFUNCSEPARATEATIPROC) (GLenum, GLenum, GLint, GLuint);
/* GL_NV_fence */
#ifndef GL_NV_fence
#define GL_ALL_COMPLETED_NV                 0x84F2
#define GL_FENCE_STATUS_NV                  0x84F3
#define GL_FENCE_CONDITION_NV               0x84F4
#endif
typedef void (APIENTRY * PGLFNGENFENCESNVPROC) (GLsizei, GLuint *);
typedef void (APIENTRY * PGLFNDELETEFENCESNVPROC) (GLuint, const GLuint *);
typedef void (APIENTRY * PGLFNSETFENCENVPROC) (GLuint, GLenum);
typedef GLboolean (APIENTRY * PGLFNTESTFENCENVPROC) (GLuint);
typedef void (APIENTRY * PGLFNFINISHFENCENVPROC) (GLuint);
typedef GLboolean (APIENTRY * PGLFNISFENCENVPROC) (GLuint);
typedef void (APIENTRY * PGLFNGETFENCEIVNVPROC) (GLuint, GLenum, GLint *);
/* GL_APPLE_fence */
#ifndef GL_APPLE_fence
#define GL_DRAW_PIXELS_APPLE                0x8A0A
#define GL_FENCE_APPLE                      0x84F3
#endif
typedef void (APIENTRY * PGLFNGENFENCESAPPLEPROC) (GLsizei, GLuint *);
typedef void (APIENTRY * PGLFNDELETEFENCESAPPLEPROC) (GLuint, const GLuint *);
typedef void (APIENTRY * PGLFNSETFENCEAPPLEPROC) (GLuint);
typedef GLboolean (APIENTRY * PGLFNTESTFENCEAPPLEPROC) (GLuint);
typedef void (APIENTRY * PGLFNFINISHFENCEAPPLEPROC) (GLuint);
typedef GLboolean (APIENTRY * PGLFNISFENCEAPPLEPROC) (GLuint);
typedef GLboolean (APIENTRY * PGLFNTESTOBJECTAPPLEPROC) (GLenum, GLuint);
typedef void (APIENTRY * PGLFNFINISHOBJECTAPPLEPROC) (GLenum, GLuint);
/* GL_APPLE_client_storage */
#ifndef GL_APPLE_client_storage
#define GL_UNPACK_CLIENT_STORAGE_APPLE      0x85B2
#endif
/* GL_ATI_envmap_bumpmap */
#ifndef GL_ATI_envmap_bumpmap
#define GL_BUMP_ROT_MATRIX_ATI              0x8775
#define GL_BUMP_ROT_MATRIX_SIZE_ATI         0x8776
#define GL_BUMP_NUM_TEX_UNITS_ATI           0x8777
#define GL_BUMP_TEX_UNITS_ATI               0x8778
#define GL_DUDV_ATI                         0x8779
#define GL_DU8DV8_ATI                       0x877A
#define GL_BUMP_ENVMAP_ATI                  0x877B
#define GL_BUMP_TARGET_ATI                  0x877C
#endif
typedef void (APIENTRY * PGLFNTEXBUMPPARAMETERIVATIPROC) (GLenum, GLuint);
typedef void (APIENTRY * PGLFNTEXBUMPPARAMETERFVATIPROC) (GLenum, GLuint);
typedef void (APIENTRY * PGLFNGETTEXBUMPPARAMETERIVATIPROC) (GLenum, GLuint);
typedef void (APIENTRY * PGLFNGETTEXBUMPPARAMETERFVATIPROC) (GLenum, GLuint);

/* GL_VERSION_2_0 */
#ifndef GL_VERSION_2_0
#define GL_VERSION_2_0 1
#define GL_BLEND_EQUATION_RGB             GL_BLEND_EQUATION
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED    0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE       0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE     0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE       0x8625
#define GL_CURRENT_VERTEX_ATTRIB          0x8626
#define GL_VERTEX_PROGRAM_POINT_SIZE      0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE        0x8643
#define GL_VERTEX_ATTRIB_ARRAY_POINTER    0x8645
#define GL_STENCIL_BACK_FUNC              0x8800
#define GL_STENCIL_BACK_FAIL              0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL   0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS   0x8803
#define GL_MAX_DRAW_BUFFERS               0x8824
#define GL_DRAW_BUFFER0                   0x8825
#define GL_DRAW_BUFFER1                   0x8826
#define GL_DRAW_BUFFER2                   0x8827
#define GL_DRAW_BUFFER3                   0x8828
#define GL_DRAW_BUFFER4                   0x8829
#define GL_DRAW_BUFFER5                   0x882A
#define GL_DRAW_BUFFER6                   0x882B
#define GL_DRAW_BUFFER7                   0x882C
#define GL_DRAW_BUFFER8                   0x882D
#define GL_DRAW_BUFFER9                   0x882E
#define GL_DRAW_BUFFER10                  0x882F
#define GL_DRAW_BUFFER11                  0x8830
#define GL_DRAW_BUFFER12                  0x8831
#define GL_DRAW_BUFFER13                  0x8832
#define GL_DRAW_BUFFER14                  0x8833
#define GL_DRAW_BUFFER15                  0x8834
#define GL_BLEND_EQUATION_ALPHA           0x883D
#define GL_POINT_SPRITE                   0x8861
#define GL_COORD_REPLACE                  0x8862
#define GL_MAX_VERTEX_ATTRIBS             0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED 0x886A
#define GL_MAX_TEXTURE_COORDS             0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS        0x8872
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS  0x8B4A
#define GL_MAX_VARYING_FLOATS             0x8B4B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_SHADER_TYPE                    0x8B4F
#define GL_FLOAT_VEC2                     0x8B50
#define GL_FLOAT_VEC3                     0x8B51
#define GL_FLOAT_VEC4                     0x8B52
#define GL_INT_VEC2                       0x8B53
#define GL_INT_VEC3                       0x8B54
#define GL_INT_VEC4                       0x8B55
#define GL_BOOL                           0x8B56
#define GL_BOOL_VEC2                      0x8B57
#define GL_BOOL_VEC3                      0x8B58
#define GL_BOOL_VEC4                      0x8B59
#define GL_FLOAT_MAT2                     0x8B5A
#define GL_FLOAT_MAT3                     0x8B5B
#define GL_FLOAT_MAT4                     0x8B5C
#define GL_SAMPLER_1D                     0x8B5D
#define GL_SAMPLER_2D                     0x8B5E
#define GL_SAMPLER_3D                     0x8B5F
#define GL_SAMPLER_CUBE                   0x8B60
#define GL_SAMPLER_1D_SHADOW              0x8B61
#define GL_SAMPLER_2D_SHADOW              0x8B62
#define GL_DELETE_STATUS                  0x8B80
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_VALIDATE_STATUS                0x8B83
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_ATTACHED_SHADERS               0x8B85
#define GL_ACTIVE_UNIFORMS                0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH      0x8B87
#define GL_SHADER_SOURCE_LENGTH           0x8B88
#define GL_ACTIVE_ATTRIBUTES              0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH    0x8B8A
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT 0x8B8B
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
#define GL_CURRENT_PROGRAM                0x8B8D
#define GL_POINT_SPRITE_COORD_ORIGIN      0x8CA0
#define GL_LOWER_LEFT                     0x8CA1
#define GL_UPPER_LEFT                     0x8CA2
#define GL_STENCIL_BACK_REF               0x8CA3
#define GL_STENCIL_BACK_VALUE_MASK        0x8CA4
#define GL_STENCIL_BACK_WRITEMASK         0x8CA5
typedef char GLchar;
#endif
typedef void (APIENTRY * PGLFNBLENDEQUATIONSEPARATEPROC) (GLenum modeRGB, GLenum modeAlpha);
typedef void (APIENTRY * PGLFNDRAWBUFFERSPROC) (GLsizei n, const GLenum *bufs);
typedef void (APIENTRY * PGLFNSTENCILOPSEPARATEPROC) (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
typedef void (APIENTRY * PGLFNSTENCILFUNCSEPARATEPROC) (GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);
typedef void (APIENTRY * PGLFNSTENCILMASKSEPARATEPROC) (GLenum face, GLuint mask);
typedef void (APIENTRY * PGLFNATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRY * PGLFNBINDATTRIBLOCATIONPROC) (GLuint program, GLuint index, const GLchar *name);
typedef void (APIENTRY * PGLFNCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRY * PGLFNCREATEPROGRAMPROC) (void);
typedef GLuint (APIENTRY * PGLFNCREATESHADERPROC) (GLenum type);
typedef void (APIENTRY * PGLFNDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY * PGLFNDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRY * PGLFNDETACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRY * PGLFNDISABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRY * PGLFNENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRY * PGLFNGETACTIVEATTRIBPROC) (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
typedef void (APIENTRY * PGLFNGETACTIVEUNIFORMPROC) (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
typedef void (APIENTRY * PGLFNGETATTACHEDSHADERSPROC) (GLuint program, GLsizei maxCount, GLsizei *count, GLuint *obj);
typedef GLint (APIENTRY * PGLFNGETATTRIBLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRY * PGLFNGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY * PGLFNGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY * PGLFNGETSHADERSOURCEPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);
typedef GLint (APIENTRY * PGLFNGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRY * PGLFNGETUNIFORMFVPROC) (GLuint program, GLint location, GLfloat *params);
typedef void (APIENTRY * PGLFNGETUNIFORMIVPROC) (GLuint program, GLint location, GLint *params);
typedef void (APIENTRY * PGLFNGETVERTEXATTRIBDVPROC) (GLuint index, GLenum pname, GLdouble *params);
typedef void (APIENTRY * PGLFNGETVERTEXATTRIBFVPROC) (GLuint index, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PGLFNGETVERTEXATTRIBIVPROC) (GLuint index, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETVERTEXATTRIBPOINTERVPROC) (GLuint index, GLenum pname, GLvoid* *pointer);
typedef GLboolean (APIENTRY * PGLFNISPROGRAMPROC) (GLuint program);
typedef GLboolean (APIENTRY * PGLFNISSHADERPROC) (GLuint shader);
typedef void (APIENTRY * PGLFNLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRY * PGLFNSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar* *string, const GLint *length);
typedef void (APIENTRY * PGLFNUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY * PGLFNUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (APIENTRY * PGLFNUNIFORM2FPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY * PGLFNUNIFORM3FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY * PGLFNUNIFORM4FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY * PGLFNUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRY * PGLFNUNIFORM2IPROC) (GLint location, GLint v0, GLint v1);
typedef void (APIENTRY * PGLFNUNIFORM3IPROC) (GLint location, GLint v0, GLint v1, GLint v2);
typedef void (APIENTRY * PGLFNUNIFORM4IPROC) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void (APIENTRY * PGLFNUNIFORM1FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * PGLFNUNIFORM2FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * PGLFNUNIFORM3FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * PGLFNUNIFORM4FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * PGLFNUNIFORM1IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * PGLFNUNIFORM2IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * PGLFNUNIFORM3IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * PGLFNUNIFORM4IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * PGLFNUNIFORMMATRIX2FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * PGLFNUNIFORMMATRIX3FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * PGLFNUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * PGLFNVALIDATEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1DPROC) (GLuint index, GLdouble x);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1DVPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1FPROC) (GLuint index, GLfloat x);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1FVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1SPROC) (GLuint index, GLshort x);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1SVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2DPROC) (GLuint index, GLdouble x, GLdouble y);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2DVPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2FPROC) (GLuint index, GLfloat x, GLfloat y);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2FVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2SPROC) (GLuint index, GLshort x, GLshort y);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2SVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3DPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3DVPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3FPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3FVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3SPROC) (GLuint index, GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3SVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NBVPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NIVPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NSVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NUBPROC) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NUBVPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NUIVPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NUSVPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4BVPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4DPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4DVPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4FPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4FVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4IVPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4SPROC) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4SVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4UBVPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4UIVPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4USVPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);

/****************************************************
 * OpenGL Official Version 
 *  defines 
 ****************************************************/
/* GL_VERSION_1_3 */
#if !defined(GL_DOT3_RGBA)
# define GL_DOT3_RGBA                     0x8741
#endif
#if !defined(GL_SUBTRACT)
# define GL_SUBTRACT                      0x84E7
#endif


/****************************************************
 * OpenGL GLX Extensions
 *  defines and functions pointer
 ****************************************************/

#ifndef WINE_NATIVEWIN32
/****************************************************
 * OpenGL GLX Official Version
 *  defines and functions pointer
 ****************************************************/
/* GLX_VERSION_1_3 */
typedef GLXFBConfig * (APIENTRY * PGLXFNGLXGETFBCONFIGSPROC) (Display *dpy, int screen, int *nelements);
typedef GLXFBConfig * (APIENTRY * PGLXFNGLXCHOOSEFBCONFIGPROC) (Display *dpy, int screen, const int *attrib_list, int *nelements);
typedef int           (APIENTRY * PGLXFNGLXGETFBCONFIGATTRIBPROC) (Display *dpy, GLXFBConfig config, int attribute, int *value);
typedef XVisualInfo * (APIENTRY * PGLXFNGLXGETVISUALFROMFBCONFIGPROC) (Display *dpy, GLXFBConfig config);
typedef GLXWindow     (APIENTRY * PGLXFNGLXCREATEWINDOWPROC) (Display *dpy, GLXFBConfig config, Window win, const int *attrib_list);
typedef void          (APIENTRY * PGLXFNGLXDESTROYWINDOWPROC) (Display *dpy, GLXWindow win);
typedef GLXPixmap     (APIENTRY * PGLXFNGLXCREATEPIXMAPPROC) (Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attrib_list);
typedef void          (APIENTRY * PGLXFNGLXDESTROYPIXMAPPROC) (Display *dpy, GLXPixmap pixmap);
typedef GLXPbuffer    (APIENTRY * PGLXFNGLXCREATEPBUFFERPROC) (Display *dpy, GLXFBConfig config, const int *attrib_list);
typedef void          (APIENTRY * PGLXFNGLXDESTROYPBUFFERPROC) (Display *dpy, GLXPbuffer pbuf);
typedef void          (APIENTRY * PGLXFNGLXQUERYDRAWABLEPROC) (Display *dpy, GLXDrawable draw, int attribute, unsigned int *value);
typedef GLXContext    (APIENTRY * PGLXFNGLXCREATENEWCONTEXTPROC) (Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);
typedef Bool          (APIENTRY * PGLXFNGLXMAKECONTEXTCURRENTPROC) (Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
typedef GLXDrawable   (APIENTRY * PGLXFNGLXGETCURRENTREADDRAWABLEPROC) (void);
typedef Display *     (APIENTRY * PGLXFNGLXGETCURRENTDISPLAYPROC) (void);
typedef int           (APIENTRY * PGLXFNGLXQUERYCONTEXTPROC) (Display *dpy, GLXContext ctx, int attribute, int *value);
typedef void          (APIENTRY * PGLXFNGLXSELECTEVENTPROC) (Display *dpy, GLXDrawable draw, unsigned long event_mask);
typedef void          (APIENTRY * PGLXFNGLXGETSELECTEDEVENTPROC) (Display *dpy, GLXDrawable draw, unsigned long *event_mask);
#endif

/****************************************************
 * Enumerated types
 ****************************************************/
typedef enum _GL_Vendors {
  VENDOR_WINE   = 0x0,
  VENDOR_MESA   = 0x1,
  VENDOR_ATI    = 0x1002,
  VENDOR_NVIDIA = 0x10de,
  VENDOR_INTEL  = 0x8086
} GL_Vendors;

typedef enum _GL_Cards {
  CARD_WINE                       =    0x0,

  CARD_ATI_RAGE_128PRO            = 0x5246,
  CARD_ATI_RADEON_7200            = 0x5144,
  CARD_ATI_RADEON_8500            = 0x514c,
  CARD_ATI_RADEON_9500            = 0x4144,
  CARD_ATI_RADEON_X700            = 0x5e4c,
  CARD_ATI_RADEON_X1600           = 0x71c2,

  CARD_NVIDIA_RIVA_128            = 0x0018,
  CARD_NVIDIA_RIVA_TNT            = 0x0020,
  CARD_NVIDIA_RIVA_TNT2           = 0x0028,
  CARD_NVIDIA_GEFORCE             = 0x0100,
  CARD_NVIDIA_GEFORCE2_MX         = 0x0110,
  CARD_NVIDIA_GEFORCE2            = 0x0150,
  CARD_NVIDIA_GEFORCE3            = 0x0200,
  CARD_NVIDIA_GEFORCE4_MX         = 0x0170,
  CARD_NVIDIA_GEFORCE4_TI4200     = 0x0253,
  CARD_NVIDIA_GEFORCEFX_5200      = 0x0320,
  CARD_NVIDIA_GEFORCEFX_5600      = 0x0312,
  CARD_NVIDIA_GEFORCEFX_5800      = 0x0302,
  CARD_NVIDIA_GEFORCE_6200        = 0x014f,
  CARD_NVIDIA_GEFORCE_6600GT      = 0x0140,
  CARD_NVIDIA_GEFORCE_6800        = 0x0041,
  CARD_NVIDIA_GEFORCE_7800GT      = 0x0092,

  CARD_INTEL_845G                 = 0x2562,
  CARD_INTEL_I830G                = 0x3577,
  CARD_INTEL_I855G                = 0x3582,
  CARD_INTEL_I865G                = 0x2572,
  CARD_INTEL_I915G                = 0x2582,
  CARD_INTEL_I915GM               = 0x2592
} GL_Cards;

typedef enum _GL_VSVersion {
  VS_VERSION_NOT_SUPPORTED = 0x0,
  VS_VERSION_10 = 0x10,
  VS_VERSION_11 = 0x11,
  VS_VERSION_20 = 0x20,
  VS_VERSION_30 = 0x30,
  /*Force 32-bits*/
  VS_VERSION_FORCE_DWORD = 0x7FFFFFFF
} GL_VSVersion;

typedef enum _GL_PSVersion {
  PS_VERSION_NOT_SUPPORTED = 0x0,
  PS_VERSION_10 = 0x10,
  PS_VERSION_11 = 0x11,
  PS_VERSION_12 = 0x12,
  PS_VERSION_13 = 0x13,
  PS_VERSION_14 = 0x14,
  PS_VERSION_20 = 0x20,
  PS_VERSION_30 = 0x30,
  /*Force 32-bits*/
  PS_VERSION_FORCE_DWORD = 0x7FFFFFFF
} GL_PSVersion;

#define MAKEDWORD_VERSION(maj, min)  ((maj & 0x0000FFFF) << 16) | (min & 0x0000FFFF)

/* OpenGL Supported Extensions (ARB and EXT) */
typedef enum _GL_SupportedExt {
  /* ARB */
  ARB_DRAW_BUFFERS,
  ARB_FRAGMENT_PROGRAM,
  ARB_FRAGMENT_SHADER,
  ARB_IMAGING,
  ARB_MULTISAMPLE,
  ARB_MULTITEXTURE,
  ARB_OCCLUSION_QUERY,
  ARB_POINT_PARAMETERS,
  ARB_PIXEL_BUFFER_OBJECT,
  ARB_POINT_SPRITE,
  ARB_TEXTURE_COMPRESSION,
  ARB_TEXTURE_CUBE_MAP,
  ARB_TEXTURE_ENV_ADD,
  ARB_TEXTURE_ENV_COMBINE,
  ARB_TEXTURE_ENV_DOT3,
  ARB_TEXTURE_FLOAT,
  ARB_HALF_FLOAT_PIXEL,
  ARB_TEXTURE_BORDER_CLAMP,
  ARB_TEXTURE_MIRRORED_REPEAT,
  ARB_TEXTURE_NON_POWER_OF_TWO,
  ARB_VERTEX_PROGRAM,
  ARB_VERTEX_BLEND,
  ARB_VERTEX_BUFFER_OBJECT,
  ARB_VERTEX_SHADER,
  /* EXT */
  EXT_BLEND_MINMAX,
  EXT_FOG_COORD,
  EXT_FRAMEBUFFER_OBJECT,
  EXT_FRAMEBUFFER_BLIT,
  EXT_PALETTED_TEXTURE,
  EXT_PIXEL_BUFFER_OBJECT,
  EXT_POINT_PARAMETERS,
  EXT_SECONDARY_COLOR,
  EXT_STENCIL_TWO_SIDE,
  EXT_STENCIL_WRAP,
  EXT_TEXTURE3D,
  EXT_TEXTURE_COMPRESSION_S3TC,
  EXT_TEXTURE_FILTER_ANISOTROPIC,
  EXT_TEXTURE_LOD,
  EXT_TEXTURE_LOD_BIAS,
  EXT_TEXTURE_ENV_ADD,
  EXT_TEXTURE_ENV_COMBINE,
  EXT_TEXTURE_ENV_DOT3,
  EXT_VERTEX_WEIGHTING,
  /* NVIDIA */
  NV_FOG_DISTANCE,
  NV_FRAGMENT_PROGRAM,
  NV_OCCLUSION_QUERY,
  NV_REGISTER_COMBINERS,
  NV_REGISTER_COMBINERS2,
  NV_TEXGEN_REFLECTION,
  NV_TEXTURE_ENV_COMBINE4,
  NV_TEXTURE_SHADER,
  NV_TEXTURE_SHADER2,
  NV_TEXTURE_SHADER3,
  NV_VERTEX_PROGRAM,
  NV_FENCE,
  /* ATI */
  ATI_SEPARATE_STENCIL,
  ATI_TEXTURE_ENV_COMBINE3,
  ATI_TEXTURE_MIRROR_ONCE,
  EXT_VERTEX_SHADER,
  ATI_ENVMAP_BUMPMAP,
  /* APPLE */
  APPLE_FENCE,
  APPLE_CLIENT_STORAGE,

  OPENGL_SUPPORTED_EXT_END
} GL_SupportedExt;


/****************************************************
 * #Defines       
 ****************************************************/
#define GL_EXT_FUNCS_GEN \
    /** ARB Extensions **/ \
    /* GL_ARB_draw_buffers */ \
    USE_GL_FUNC(PGLFNDRAWBUFFERSARBPROC, glDrawBuffersARB); \
    /* GL_ARB_imaging */ \
    USE_GL_FUNC(PGLFNBLENDCOLORPROC,                 glBlendColor); \
    USE_GL_FUNC(PGLFNBLENDEQUATIONPROC,              glBlendEquation); \
    /* GL_ARB_multitexture */ \
    USE_GL_FUNC(WINED3D_PFNGLACTIVETEXTUREARBPROC,       glActiveTextureARB); \
    USE_GL_FUNC(WINED3D_PFNGLCLIENTACTIVETEXTUREARBPROC, glClientActiveTextureARB); \
    USE_GL_FUNC(WINED3D_PFNGLMULTITEXCOORD1FARBPROC,     glMultiTexCoord1fARB); \
    USE_GL_FUNC(WINED3D_PFNGLMULTITEXCOORD2FARBPROC,     glMultiTexCoord2fARB); \
    USE_GL_FUNC(WINED3D_PFNGLMULTITEXCOORD3FARBPROC,     glMultiTexCoord3fARB); \
    USE_GL_FUNC(WINED3D_PFNGLMULTITEXCOORD4FARBPROC,     glMultiTexCoord4fARB); \
    /* GL_ARB_occlusion_query */ \
    USE_GL_FUNC(PGLFNGENQUERIESARBPROC,              glGenQueriesARB); \
    USE_GL_FUNC(PGLFNDELETEQUERIESARBPROC,           glDeleteQueriesARB); \
    USE_GL_FUNC(PGLFNBEGINQUERYARBPROC,              glBeginQueryARB); \
    USE_GL_FUNC(PGLFNENDQUERYARBPROC,                glEndQueryARB); \
    USE_GL_FUNC(PGLFNGETQUERYOBJECTIVARBPROC,        glGetQueryObjectivARB); \
    USE_GL_FUNC(PGLFNGETQUERYOBJECTUIVARBPROC,       glGetQueryObjectuivARB); \
    /* GL_ARB_point_parameters */ \
    USE_GL_FUNC(PGLFNGLPOINTPARAMETERFARBPROC,       glPointParameterfARB); \
    USE_GL_FUNC(PGLFNGLPOINTPARAMETERFVARBPROC,      glPointParameterfvARB); \
    /* GL_ARB_texture_compression */ \
    USE_GL_FUNC(PGLFNCOMPRESSEDTEXIMAGE2DPROC,       glCompressedTexImage2DARB); \
    USE_GL_FUNC(PGLFNCOMPRESSEDTEXIMAGE3DPROC,       glCompressedTexImage3DARB); \
    USE_GL_FUNC(PGLFNCOMPRESSEDTEXSUBIMAGE2DPROC,    glCompressedTexSubImage2DARB); \
    USE_GL_FUNC(PGLFNCOMPRESSEDTEXSUBIMAGE3DPROC,    glCompressedTexSubImage3DARB); \
    USE_GL_FUNC(PGLFNGETCOMPRESSEDTEXIMAGEPROC,      glGetCompressedTexImageARB); \
    /* GL_ARB_vertex_blend */ \
    USE_GL_FUNC(PGLFNGLWEIGHTPOINTERARB,             glWeightPointerARB); \
    /* GL_ARB_vertex_buffer_object */ \
    USE_GL_FUNC(PGLFNBINDBUFFERARBPROC,              glBindBufferARB); \
    USE_GL_FUNC(PGLFNDELETEBUFFERSARBPROC,           glDeleteBuffersARB); \
    USE_GL_FUNC(PGLFNGENBUFFERSARBPROC,              glGenBuffersARB); \
    USE_GL_FUNC(PGLFNISBUFFERARBPROC,                glIsBufferARB); \
    USE_GL_FUNC(PGLFNBUFFERDATAARBPROC,              glBufferDataARB); \
    USE_GL_FUNC(PGLFNBUFFERSUBDATAARBPROC,           glBufferSubDataARB); \
    USE_GL_FUNC(PGLFNGETBUFFERSUBDATAARBPROC,        glGetBufferSubDataARB); \
    USE_GL_FUNC(PGLFNMAPBUFFERARBPROC,               glMapBufferARB); \
    USE_GL_FUNC(PGLFNUNMAPBUFFERARBPROC,             glUnmapBufferARB); \
    USE_GL_FUNC(PGLFNGETBUFFERPARAMETERIVARBPROC,    glGetBufferParameterivARB); \
    USE_GL_FUNC(PGLFNGETBUFFERPOINTERVARBPROC,       glGetBufferPointervARB); \
    /** EXT Extensions **/ \
    /* GL_EXT_fog_coord */ \
    USE_GL_FUNC(PGLFNGLFOGCOORDFEXTPROC,                glFogCoordfEXT); \
    USE_GL_FUNC(PGLFNGLFOGCOORDFVEXTPROC,               glFogCoordfvEXT); \
    USE_GL_FUNC(PGLFNGLFOGCOORDDEXTPROC,                glFogCoorddEXT); \
    USE_GL_FUNC(PGLFNGLFOGCOORDDVEXTPROC,               glFogCoordvEXT); \
    USE_GL_FUNC(PGLFNGLFOGCOORDPOINTEREXTPROC,          glFogCoordPointerEXT); \
    /* GL_EXT_framebuffer_object */ \
    USE_GL_FUNC(PGLFNGLISRENDERBUFFEREXTPROC,          glIsRenderbufferEXT); \
    USE_GL_FUNC(PGLFNGLBINDRENDERBUFFEREXTPROC,        glBindRenderbufferEXT); \
    USE_GL_FUNC(PGLFNGLDELETERENDERBUFFERSEXTPROC,     glDeleteRenderbuffersEXT); \
    USE_GL_FUNC(PGLFNGLGENRENDERBUFFERSEXTPROC,        glGenRenderbuffersEXT); \
    USE_GL_FUNC(PGLFNGLRENDERBUFFERSTORAGEEXTPROC,     glRenderbufferStorageEXT); \
    USE_GL_FUNC(PGLFNGLISFRAMEBUFFEREXTPROC,           glIsFramebufferEXT); \
    USE_GL_FUNC(PGLFNGLBINDFRAMEBUFFEREXTPROC,         glBindFramebufferEXT); \
    USE_GL_FUNC(PGLFNGLDELETEFRAMEBUFFERSEXTPROC,      glDeleteFramebuffersEXT); \
    USE_GL_FUNC(PGLFNGLGENFRAMEBUFFERSEXTPROC,         glGenFramebuffersEXT); \
    USE_GL_FUNC(PGLFNGLCHECKFRAMEBUFFERSTATUSEXTPROC,  glCheckFramebufferStatusEXT); \
    USE_GL_FUNC(PGLFNGLFRAMEBUFFERTEXTURE1DEXTPROC,    glFramebufferTexture1DEXT); \
    USE_GL_FUNC(PGLFNGLFRAMEBUFFERTEXTURE2DEXTPROC,    glFramebufferTexture2DEXT); \
    USE_GL_FUNC(PGLFNGLFRAMEBUFFERTEXTURE3DEXTPROC,    glFramebufferTexture3DEXT); \
    USE_GL_FUNC(PGLFNGLFRAMEBUFFERRENDERBUFFEREXTPROC, glFramebufferRenderbufferEXT); \
    USE_GL_FUNC(PGLFNGLGENERATEMIPMAPEXTPROC,          glGenerateMipmapEXT); \
    USE_GL_FUNC(PGLFNGLGETRENDERBUFFERPARAMETERIVEXTPROC, glGetRenderbufferParameterivEXT); \
    USE_GL_FUNC(PGLFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC, glGetFramebufferAttachmentParameterivEXT); \
    /* GL_EXT_framebuffer_blit */ \
    USE_GL_FUNC(PGLFNGLBLITFRAMEBUFFEREXTPROC, glBlitFramebufferEXT); \
    /* GL_EXT_paletted_texture */ \
    USE_GL_FUNC(PGLFNGLCOLORTABLEEXTPROC,             glColorTableEXT); \
    /* GL_EXT_point_parameters */ \
    USE_GL_FUNC(PGLFNGLPOINTPARAMETERFEXTPROC,        glPointParameterfEXT); \
    USE_GL_FUNC(PGLFNGLPOINTPARAMETERFVEXTPROC,       glPointParameterfvEXT); \
    /* GL_EXT_secondary_color */ \
    USE_GL_FUNC(PGLFNGLSECONDARYCOLOR3UBEXTPROC,      glSecondaryColor3ubEXT); \
    USE_GL_FUNC(PGLFNGLSECONDARYCOLOR3FEXTPROC,       glSecondaryColor3fEXT); \
    USE_GL_FUNC(PGLFNGLSECONDARYCOLOR3FVEXTPROC,      glSecondaryColor3fvEXT); \
    USE_GL_FUNC(PGLFNGLSECONDARYCOLORPOINTEREXTPROC,  glSecondaryColorPointerEXT); \
    /* GL_EXT_texture3D */ \
    USE_GL_FUNC(PGLFNGLTEXIMAGE3DEXTPROC,              glTexImage3DEXT); \
    USE_GL_FUNC(PGLFNGLTEXSUBIMAGE3DEXTPROC,           glTexSubImage3DEXT); \
    /* GL_ARB_vertex_program */ \
    USE_GL_FUNC(PGLFNGENPROGRAMSARBPROC,              glGenProgramsARB); \
    USE_GL_FUNC(PGLFNBINDPROGRAMARBPROC,              glBindProgramARB); \
    USE_GL_FUNC(PGLFNPROGRAMSTRINGARBPROC,            glProgramStringARB); \
    USE_GL_FUNC(PGLFNDELETEPROGRAMSARBPROC,           glDeleteProgramsARB); \
    USE_GL_FUNC(PGLFNPROGRAMENVPARAMETER4FVARBPROC,   glProgramEnvParameter4fvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIBPOINTERARBPROC,      glVertexAttribPointerARB); \
    USE_GL_FUNC(PGLFNENABLEVERTEXATTRIBARRAYARBPROC,  glEnableVertexAttribArrayARB); \
    USE_GL_FUNC(PGLFNDISABLEVERTEXATTRIBARRAYARBPROC, glDisableVertexAttribArrayARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB1DARBPROC,           glVertexAttrib1dARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB1DVARBPROC,          glVertexAttrib1dvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB1FARBPROC,           glVertexAttrib1fARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB1FVARBPROC,          glVertexAttrib1fvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB1SARBPROC,           glVertexAttrib1sARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB1SVARBPROC,          glVertexAttrib1svARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB2DARBPROC,           glVertexAttrib2dARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB2DVARBPROC,          glVertexAttrib2dvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB2FARBPROC,           glVertexAttrib2fARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB2FVARBPROC,          glVertexAttrib2fvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB2SARBPROC,           glVertexAttrib2sARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB2SVARBPROC,          glVertexAttrib2svARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB3DARBPROC,           glVertexAttrib3dARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB3DVARBPROC,          glVertexAttrib3dvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB3FARBPROC,           glVertexAttrib3fARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB3FVARBPROC,          glVertexAttrib3fvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB3SARBPROC,           glVertexAttrib3sARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB3SVARBPROC,          glVertexAttrib3svARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NBVARBPROC,         glVertexAttrib4NbvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NIVARBPROC,         glVertexAttrib4NivARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NSVARBPROC,         glVertexAttrib4NsvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NUBARBPROC,         glVertexAttrib4NubARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NUBVARBPROC,        glVertexAttrib4NubvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NUIVARBPROC,        glVertexAttrib4NuivARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NUSVARBPROC,        glVertexAttrib4NusvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4BVARBPROC,          glVertexAttrib4bvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4DARBPROC,           glVertexAttrib4dARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4DVARBPROC,          glVertexAttrib4dvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4FARBPROC,           glVertexAttrib4fARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4FVARBPROC,          glVertexAttrib4fvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4IVARBPROC,          glVertexAttrib4ivARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4SARBPROC,           glVertexAttrib4sARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4SVARBPROC,          glVertexAttrib4svARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4UBVARBPROC,         glVertexAttrib4ubvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4UIVARBPROC,         glVertexAttrib4uivARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4USVARBPROC,         glVertexAttrib4usvARB); \
    USE_GL_FUNC(PGLFNGETPROGRAMIVARBPROC,             glGetProgramivARB); \
    /* GL_ARB_shader_objects */ \
    USE_GL_FUNC(WINED3D_PFNGLGETOBJECTPARAMETERIVARBPROC,     glGetObjectParameterivARB); \
    USE_GL_FUNC(WINED3D_PFNGLGETOBJECTPARAMETERFVARBPROC,     glGetObjectParameterfvARB); \
    USE_GL_FUNC(WINED3D_PFNGLGETUNIFORMLOCATIONARBPROC,       glGetUniformLocationARB); \
    USE_GL_FUNC(WINED3D_PFNGLGETACTIVEUNIFORMARBPROC,         glGetActiveUniformARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM1IARBPROC,                glUniform1iARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM2IARBPROC,                glUniform2iARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM3IARBPROC,                glUniform3iARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM4IARBPROC,                glUniform4iARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM1IARBPROC,                glUniform1fARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM2FARBPROC,                glUniform2fARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM3FARBPROC,                glUniform3fARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM4FARBPROC,                glUniform4fARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM1IVARBPROC,               glUniform1fvARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM2IVARBPROC,               glUniform2fvARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM3IVARBPROC,               glUniform3fvARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM4FVARBPROC,               glUniform4fvARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM1IVARBPROC,               glUniform1ivARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM2IVARBPROC,               glUniform2ivARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM3IVARBPROC,               glUniform3ivARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORM4IVARBPROC,               glUniform4ivARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORMMATRIX2FVARBPROC,         glUniformMatrix2fvARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORMMATRIX3FVARBPROC,         glUniformMatrix3fvARB); \
    USE_GL_FUNC(WINED3D_PFNGLUNIFORMMATRIX4FVARBPROC,         glUniformMatrix4fvARB); \
    USE_GL_FUNC(WINED3D_PFNGLGETUNIFORMFVARBPROC,             glGetUniform4fvARB); \
    USE_GL_FUNC(WINED3D_PFNGLGETUNIFORMIVARBPROC,             glGetUniform4ivARB); \
    USE_GL_FUNC(WINED3D_PFNGLGETINFOLOGARBPROC,               glGetInfoLogARB); \
    USE_GL_FUNC(WINED3D_PFNGLUSEPROGRAMOBJECTARBPROC,         glUseProgramObjectARB); \
    USE_GL_FUNC(WINED3D_PFNGLCREATESHADEROBJECTARBPROC,       glCreateShaderObjectARB); \
    USE_GL_FUNC(WINED3D_PFNGLSHADERSOURCEARBPROC,             glShaderSourceARB); \
    USE_GL_FUNC(WINED3D_PFNGLCOMPILESHADERARBPROC,            glCompileShaderARB); \
    USE_GL_FUNC(WINED3D_PFNGLCREATEPROGRAMOBJECTARBPROC,      glCreateProgramObjectARB); \
    USE_GL_FUNC(WINED3D_PFNGLATTACHOBJECTARBPROC,             glAttachObjectARB); \
    USE_GL_FUNC(WINED3D_PFNGLLINKPROGRAMARBPROC,              glLinkProgramARB); \
    USE_GL_FUNC(WINED3D_PFNGLDETACHOBJECTARBPROC,             glDetachObjectARB); \
    USE_GL_FUNC(WINED3D_PFNGLDELETEOBJECTARBPROC,             glDeleteObjectARB); \
    USE_GL_FUNC(WINED3D_PFNGLVALIDATEPROGRAMARBPROC,          glValidateProgramARB); \
    USE_GL_FUNC(WINED3D_PFNGLGETATTACHEDOBJECTSARBPROC,       glGetAttachedObjectsARB); \
    USE_GL_FUNC(WINED3D_PFNGLGETHANDLEARBPROC,                glGetHandleARB); \
    USE_GL_FUNC(WINED3D_PFNGLGETSHADERSOURCEARBPROC,          glGetShaderSourceARB); \
    USE_GL_FUNC(WINED3D_PFNGLBINDATTRIBLOCATIONARBPROC,       glBindAttribLocationARB); \
    USE_GL_FUNC(WINED3D_PFNGLGETATTRIBLOCATIONARBPROC,        glGetAttribLocationARB); \
    /* GL_EXT_stencil_two_side */ \
    USE_GL_FUNC(PGLFNACTIVESTENCILFACEEXTPROC, glActiveStencilFaceEXT); \
    /* GL_ATI_separate_stencil */ \
    USE_GL_FUNC(PGLFNSTENCILOPSEPARATEATIPROC, glStencilOpSeparateATI); \
    USE_GL_FUNC(PGLFNSTENCILFUNCSEPARATEATIPROC, glStencilFuncSeparateATI); \
    /* GL_NV_register_combiners */ \
    USE_GL_FUNC(PGLFNCOMBINERINPUTNVPROC,                       glCombinerInputNV); \
    USE_GL_FUNC(PGLFNCOMBINEROUTPUTNVPROC,                      glCombinerOutputNV); \
    USE_GL_FUNC(PGLFNCOMBINERPARAMETERFNVPROC,                  glCombinerParameterfNV); \
    USE_GL_FUNC(PGLFNCOMBINERPARAMETERFVNVPROC,                 glCombinerParameterfvNV); \
    USE_GL_FUNC(PGLFNCOMBINERPARAMETERINVPROC,                  glCombinerParameteriNV); \
    USE_GL_FUNC(PGLFNCOMBINERPARAMETERIVNVPROC,                 glCombinerParameterivNV); \
    USE_GL_FUNC(PGLFNFINALCOMBINERINPUTNVPROC,                  glFinalCombinerInputNV); \
    /* GL_NV_fence */ \
    USE_GL_FUNC(PGLFNGENFENCESNVPROC,                           glGenFencesNV); \
    USE_GL_FUNC(PGLFNDELETEFENCESNVPROC,                        glDeleteFencesNV); \
    USE_GL_FUNC(PGLFNSETFENCENVPROC,                            glSetFenceNV); \
    USE_GL_FUNC(PGLFNTESTFENCENVPROC,                           glTestFenceNV); \
    USE_GL_FUNC(PGLFNFINISHFENCENVPROC,                         glFinishFenceNV); \
    USE_GL_FUNC(PGLFNISFENCENVPROC,                             glIsFenceNV); \
    USE_GL_FUNC(PGLFNGETFENCEIVNVPROC,                          glGetFenceivNV); \
    /* GL_APPLE_fence */ \
    USE_GL_FUNC(PGLFNGENFENCESAPPLEPROC,                        glGenFencesAPPLE); \
    USE_GL_FUNC(PGLFNDELETEFENCESAPPLEPROC,                     glDeleteFencesAPPLE); \
    USE_GL_FUNC(PGLFNSETFENCEAPPLEPROC,                         glSetFenceAPPLE); \
    USE_GL_FUNC(PGLFNTESTFENCEAPPLEPROC,                        glTestFenceAPPLE); \
    USE_GL_FUNC(PGLFNFINISHFENCEAPPLEPROC,                      glFinishFenceAPPLE); \
    USE_GL_FUNC(PGLFNISFENCEAPPLEPROC,                          glIsFenceAPPLE); \
    USE_GL_FUNC(PGLFNTESTOBJECTAPPLEPROC,                       glTestObjectAPPLE); \
    USE_GL_FUNC(PGLFNFINISHOBJECTAPPLEPROC,                     glFinishObjectAPPLE); \
    /* GL_ATI_envmap_bumpmap */ \
    USE_GL_FUNC(PGLFNTEXBUMPPARAMETERIVATIPROC,                 glTexBumpParameterivATI); \
    USE_GL_FUNC(PGLFNTEXBUMPPARAMETERFVATIPROC,                 glTexBumpParameterfvATI); \
    USE_GL_FUNC(PGLFNGETTEXBUMPPARAMETERIVATIPROC,              glGetTexBumpParameterivATI); \
    USE_GL_FUNC(PGLFNGETTEXBUMPPARAMETERFVATIPROC,              glGetTexBumpParameterfvATI); \

/* OpenGL 2.0 functions */
#define GL2_FUNCS_GEN \
    USE_GL_FUNC(PGLFNBLENDEQUATIONSEPARATEPROC, glBlendEquationSeparate); \
    USE_GL_FUNC(PGLFNDRAWBUFFERSPROC, glDrawBuffers); \
    USE_GL_FUNC(PGLFNSTENCILOPSEPARATEPROC, glStencilOpSeparate); \
    USE_GL_FUNC(PGLFNSTENCILFUNCSEPARATEPROC, glStencilFuncSeparate); \
    USE_GL_FUNC(PGLFNSTENCILMASKSEPARATEPROC, glStencilMaskSeparate); \
    USE_GL_FUNC(PGLFNATTACHSHADERPROC, glAttachShader); \
    USE_GL_FUNC(PGLFNBINDATTRIBLOCATIONPROC, glBindAttribLocation); \
    USE_GL_FUNC(PGLFNCOMPILESHADERPROC, glCompileShader); \
    USE_GL_FUNC(PGLFNCREATEPROGRAMPROC, glCreateProgram); \
    USE_GL_FUNC(PGLFNCREATESHADERPROC, glCreateShader); \
    USE_GL_FUNC(PGLFNDELETEPROGRAMPROC, glDeleteProgram); \
    USE_GL_FUNC(PGLFNDELETESHADERPROC, glDeleteShader); \
    USE_GL_FUNC(PGLFNDETACHSHADERPROC, glDetachShader); \
    USE_GL_FUNC(PGLFNDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray); \
    USE_GL_FUNC(PGLFNENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray); \
    USE_GL_FUNC(PGLFNGETACTIVEATTRIBPROC, glGetActiveAttrib); \
    USE_GL_FUNC(PGLFNGETACTIVEUNIFORMPROC, glGetActiveUniform); \
    USE_GL_FUNC(PGLFNGETATTACHEDSHADERSPROC, glGetAttachedShaders); \
    USE_GL_FUNC(PGLFNGETATTRIBLOCATIONPROC, glGetAttribLocation); \
    USE_GL_FUNC(PGLFNGETPROGRAMIVPROC, glGetProgramiv); \
    USE_GL_FUNC(PGLFNGETPROGRAMINFOLOGPROC, glGetProgramInfoLog); \
    USE_GL_FUNC(PGLFNGETSHADERIVPROC, glGetShaderiv); \
    USE_GL_FUNC(PGLFNGETSHADERINFOLOGPROC, glGetShaderInfoLog); \
    USE_GL_FUNC(PGLFNGETSHADERSOURCEPROC, glGetShaderSource); \
    USE_GL_FUNC(PGLFNGETUNIFORMLOCATIONPROC, glGetUniformLocation); \
    USE_GL_FUNC(PGLFNGETUNIFORMFVPROC, glGetUniformfv); \
    USE_GL_FUNC(PGLFNGETUNIFORMIVPROC, glGetUniformiv); \
    USE_GL_FUNC(PGLFNGETVERTEXATTRIBDVPROC, glGetVertexAttribdv); \
    USE_GL_FUNC(PGLFNGETVERTEXATTRIBFVPROC, glGetVertexAttribfv); \
    USE_GL_FUNC(PGLFNGETVERTEXATTRIBIVPROC, glGetVertexAttribiv); \
    USE_GL_FUNC(PGLFNGETVERTEXATTRIBPOINTERVPROC, glGetVertexAttribPointerv); \
    USE_GL_FUNC(PGLFNISPROGRAMPROC, glIsProgram); \
    USE_GL_FUNC(PGLFNISSHADERPROC, glIsShader); \
    USE_GL_FUNC(PGLFNLINKPROGRAMPROC, glLinkProgram); \
    USE_GL_FUNC(PGLFNSHADERSOURCEPROC, glShaderSource); \
    USE_GL_FUNC(PGLFNUSEPROGRAMPROC, glUseProgram); \
    USE_GL_FUNC(PGLFNUNIFORM1FPROC, glUniform1f); \
    USE_GL_FUNC(PGLFNUNIFORM2FPROC, glUniform2f); \
    USE_GL_FUNC(PGLFNUNIFORM3FPROC, glUniform3f); \
    USE_GL_FUNC(PGLFNUNIFORM4FPROC, glUniform4f); \
    USE_GL_FUNC(PGLFNUNIFORM1IPROC, glUniform1i); \
    USE_GL_FUNC(PGLFNUNIFORM2IPROC, glUniform2i); \
    USE_GL_FUNC(PGLFNUNIFORM3IPROC, glUniform3i); \
    USE_GL_FUNC(PGLFNUNIFORM4IPROC, glUniform4i); \
    USE_GL_FUNC(PGLFNUNIFORM1FVPROC, glUniform1fv); \
    USE_GL_FUNC(PGLFNUNIFORM2FVPROC, glUniform2fv); \
    USE_GL_FUNC(PGLFNUNIFORM3FVPROC, glUniform3fv); \
    USE_GL_FUNC(PGLFNUNIFORM4FVPROC, glUniform4fv); \
    USE_GL_FUNC(PGLFNUNIFORM1IVPROC, glUniform1iv); \
    USE_GL_FUNC(PGLFNUNIFORM2IVPROC, glUniform2iv); \
    USE_GL_FUNC(PGLFNUNIFORM3IVPROC, glUniform3iv); \
    USE_GL_FUNC(PGLFNUNIFORM4IVPROC, glUniform4iv); \
    USE_GL_FUNC(PGLFNUNIFORMMATRIX2FVPROC, glUniformMatrix2fv); \
    USE_GL_FUNC(PGLFNUNIFORMMATRIX3FVPROC, glUniformMatrix3fv); \
    USE_GL_FUNC(PGLFNUNIFORMMATRIX4FVPROC, glUniformMatrix4fv); \
    USE_GL_FUNC(PGLFNVALIDATEPROGRAMPROC, glValidateProgram); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB1DPROC, glVertexAttrib1d); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB1DVPROC, glVertexAttrib1dv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB1FPROC, glVertexAttrib1f); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB1FVPROC, glVertexAttrib1fv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB1SPROC, glVertexAttrib1s); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB1SVPROC, glVertexAttrib1sv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB2DPROC, glVertexAttrib2d); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB2DVPROC, glVertexAttrib2dv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB2FPROC, glVertexAttrib2f); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB2FVPROC, glVertexAttrib2fv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB2SPROC, glVertexAttrib2s); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB2SVPROC, glVertexAttrib2sv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB3DPROC, glVertexAttrib3d); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB3DVPROC, glVertexAttrib3dv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB3FPROC, glVertexAttrib3f); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB3FVPROC, glVertexAttrib3fv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB3SPROC, glVertexAttrib3s); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB3SVPROC, glVertexAttrib3sv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NBVPROC, glVertexAttrib4Nbv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NIVPROC, glVertexAttrib4Niv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NSVPROC, glVertexAttrib4Nsv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NUBPROC, glVertexAttrib4Nub); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NUBVPROC, glVertexAttrib4Nubv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NUIVPROC, glVertexAttrib4Nuiv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4NUSVPROC, glVertexAttrib4Nusv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4BVPROC, glVertexAttrib4bv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4DPROC, glVertexAttrib4d); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4DVPROC, glVertexAttrib4dv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4FPROC, glVertexAttrib4f); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4FVPROC, glVertexAttrib4fv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4IVPROC, glVertexAttrib4iv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4SVPROC, glVertexAttrib4sv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4UBVPROC, glVertexAttrib4ubv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4UIVPROC, glVertexAttrib4uiv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIB4USVPROC, glVertexAttrib4usv); \
    USE_GL_FUNC(PGLFNVERTEXATTRIBPOINTERPROC, glVertexAttribPointer); \

#ifndef WINE_NATIVEWIN32

#define GLX_EXT_FUNCS_GEN \
    /** GLX_VERSION_1_3 **/ \
    USE_GL_FUNC(PGLXFNGLXCREATEPBUFFERPROC,          glXCreatePbuffer); \
    USE_GL_FUNC(PGLXFNGLXDESTROYPBUFFERPROC,         glXDestroyPbuffer); \
    USE_GL_FUNC(PGLXFNGLXCREATEPIXMAPPROC,           glXCreatePixmap); \
    USE_GL_FUNC(PGLXFNGLXDESTROYPIXMAPPROC,          glXDestroyPixmap); \
    USE_GL_FUNC(PGLXFNGLXCREATENEWCONTEXTPROC,       glXCreateNewContext); \
    USE_GL_FUNC(PGLXFNGLXMAKECONTEXTCURRENTPROC,     glXMakeContextCurrent); \
    USE_GL_FUNC(PGLXFNGLXCHOOSEFBCONFIGPROC,         glXChooseFBConfig); \

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

/* Redefines the constants */
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIENTRY    WINAPI

#else

#define GLX_EXT_FUNCS_GEN

#endif

/****************************************************
 * Structures       
 ****************************************************/
#define USE_GL_FUNC(type, pfn) type pfn
typedef struct _WineD3D_GL_Info {

#ifndef WINE_NATIVE_WIN32
  DWORD  glx_version;
#endif
  DWORD  gl_version;

  GL_Vendors gl_vendor;
  GL_Cards   gl_card;
  DWORD  gl_driver_version;
  CHAR   gl_renderer[255];
  /**
   * CAPS Constants 
   */
  UINT   max_buffers;
  UINT   max_lights;
  UINT   max_textures;
  UINT   max_texture_stages;
  UINT   max_samplers;
  UINT   max_sampler_stages;
  UINT   max_clipplanes;
  UINT   max_texture_size;
  UINT   max_texture3d_size;
  float  max_pointsize;
  UINT   max_blends;
  UINT   max_anisotropy;
  UINT   max_aux_buffers;

  unsigned max_vshader_constantsF;
  unsigned max_pshader_constantsF;

  unsigned vs_arb_constantsF;
  unsigned vs_arb_max_instructions;
  unsigned vs_arb_max_temps;
  unsigned ps_arb_constantsF;
  unsigned ps_arb_max_instructions;
  unsigned ps_arb_max_temps;
  unsigned vs_glsl_constantsF;
  unsigned ps_glsl_constantsF;

  GL_PSVersion ps_arb_version;
  GL_PSVersion ps_nv_version;

  GL_VSVersion vs_arb_version;
  GL_VSVersion vs_nv_version;
  GL_VSVersion vs_ati_version;

  BOOL supported[OPENGL_SUPPORTED_EXT_END + 1];

  /** OpenGL EXT and ARB functions ptr */
  GL_EXT_FUNCS_GEN
  /** OpenGL GLX functions ptr */
  GLX_EXT_FUNCS_GEN
  /** OpenGL 2.0 functions ptr */
  /* GL2_FUNCS_GEN; */
  /**/
} WineD3D_GL_Info;
#undef USE_GL_FUNC

#endif /* HAVE_OPENGL */

#endif /* __WINE_WINED3D_GL */
