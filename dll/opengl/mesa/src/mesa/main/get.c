/*
 * Copyright (C) 2010  Brian Paul   All Rights Reserved.
 * Copyright (C) 2010  Intel Corporation
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
 * Author: Kristian Høgsberg <krh@bitplanet.net>
 */

#include "glheader.h"
#include "context.h"
#include "enable.h"
#include "enums.h"
#include "extensions.h"
#include "get.h"
#include "macros.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "state.h"
#include "texcompress.h"
#include "framebuffer.h"

/* This is a table driven implemetation of the glGet*v() functions.
 * The basic idea is that most getters just look up an int somewhere
 * in struct gl_context and then convert it to a bool or float according to
 * which of glGetIntegerv() glGetBooleanv() etc is being called.
 * Instead of generating code to do this, we can just record the enum
 * value and the offset into struct gl_context in an array of structs.  Then
 * in glGet*(), we lookup the struct for the enum in question, and use
 * the offset to get the int we need.
 *
 * Sometimes we need to look up a float, a boolean, a bit in a
 * bitfield, a matrix or other types instead, so we need to track the
 * type of the value in struct gl_context.  And sometimes the value isn't in
 * struct gl_context but in the drawbuffer, the array object, current texture
 * unit, or maybe it's a computed value.  So we need to also track
 * where or how to find the value.  Finally, we sometimes need to
 * check that one of a number of extensions are enabled, the GL
 * version or flush or call _mesa_update_state().  This is done by
 * attaching optional extra information to the value description
 * struct, it's sort of like an array of opcodes that describe extra
 * checks or actions.
 *
 * Putting all this together we end up with struct value_desc below,
 * and with a couple of macros to help, the table of struct value_desc
 * is about as concise as the specification in the old python script.
 */

#undef CONST

#define FLOAT_TO_BOOLEAN(X)   ( (X) ? GL_TRUE : GL_FALSE )
#define FLOAT_TO_FIXED(F)     ( ((F) * 65536.0f > INT_MAX) ? INT_MAX : \
                                ((F) * 65536.0f < INT_MIN) ? INT_MIN : \
                                (GLint) ((F) * 65536.0f) )

#define INT_TO_BOOLEAN(I)     ( (I) ? GL_TRUE : GL_FALSE )
#define INT_TO_FIXED(I)       ( ((I) > SHRT_MAX) ? INT_MAX : \
                                ((I) < SHRT_MIN) ? INT_MIN : \
                                (GLint) ((I) * 65536) )

#define INT64_TO_BOOLEAN(I)   ( (I) ? GL_TRUE : GL_FALSE )
#define INT64_TO_INT(I)       ( (GLint)((I > INT_MAX) ? INT_MAX : ((I < INT_MIN) ? INT_MIN : (I))) )

#define BOOLEAN_TO_INT(B)     ( (GLint) (B) )
#define BOOLEAN_TO_INT64(B)   ( (GLint64) (B) )
#define BOOLEAN_TO_FLOAT(B)   ( (B) ? 1.0F : 0.0F )
#define BOOLEAN_TO_FIXED(B)   ( (GLint) ((B) ? 1 : 0) << 16 )

#define ENUM_TO_INT64(E)      ( (GLint64) (E) )
#define ENUM_TO_FIXED(E)      (E)

enum value_type {
   TYPE_INVALID,
   TYPE_API_MASK,
   TYPE_INT,
   TYPE_INT_2,
   TYPE_INT_3,
   TYPE_INT_4,
   TYPE_INT_N,
   TYPE_INT64,
   TYPE_ENUM,
   TYPE_ENUM_2,
   TYPE_BOOLEAN,
   TYPE_BIT_0,
   TYPE_BIT_1,
   TYPE_BIT_2,
   TYPE_BIT_3,
   TYPE_BIT_4,
   TYPE_BIT_5,
   TYPE_BIT_6,
   TYPE_BIT_7,
   TYPE_FLOAT,
   TYPE_FLOAT_2,
   TYPE_FLOAT_3,
   TYPE_FLOAT_4,
   TYPE_FLOATN,
   TYPE_FLOATN_2,
   TYPE_FLOATN_3,
   TYPE_FLOATN_4,
   TYPE_DOUBLEN,
   TYPE_MATRIX,
   TYPE_MATRIX_T,
   TYPE_CONST
};

enum value_location {
   LOC_BUFFER,
   LOC_CONTEXT,
   LOC_ARRAY,
   LOC_TEXUNIT,
   LOC_CUSTOM
};

enum value_extra {
   EXTRA_END = 0x8000,
   EXTRA_VERSION_30,
   EXTRA_VERSION_31,
   EXTRA_VERSION_32,
   EXTRA_VERSION_ES2,
   EXTRA_NEW_BUFFERS, 
   EXTRA_NEW_FRAG_CLAMP,
   EXTRA_VALID_DRAW_BUFFER,
   EXTRA_VALID_TEXTURE_UNIT,
   EXTRA_VALID_CLIP_DISTANCE,
   EXTRA_FLUSH_CURRENT,
   EXTRA_GLSL_130,
};

#define NO_EXTRA NULL
#define NO_OFFSET 0

struct value_desc {
   GLenum pname;
   GLubyte location;  /**< enum value_location */
   GLubyte type;      /**< enum value_type */
   int offset;
   const int *extra;
};

union value {
   GLfloat value_float;
   GLfloat value_float_4[4];
   GLmatrix *value_matrix;
   GLint value_int;
   GLint value_int_4[4];
   GLint64 value_int64;
   GLenum value_enum;

   /* Sigh, see GL_COMPRESSED_TEXTURE_FORMATS_ARB handling */
   struct {
      GLint n, ints[100];
   } value_int_n;
   GLboolean value_bool;
};

#define BUFFER_FIELD(field, type) \
   LOC_BUFFER, type, offsetof(struct gl_framebuffer, field)
#define CONTEXT_FIELD(field, type) \
   LOC_CONTEXT, type, offsetof(struct gl_context, field)
#define ARRAY_FIELD(field, type) \
   LOC_ARRAY, type, offsetof(struct gl_array_object, field)
#define CONST(value) \
   LOC_CONTEXT, TYPE_CONST, value

#define BUFFER_INT(field) BUFFER_FIELD(field, TYPE_INT)
#define BUFFER_ENUM(field) BUFFER_FIELD(field, TYPE_ENUM)
#define BUFFER_BOOL(field) BUFFER_FIELD(field, TYPE_BOOLEAN)

#define CONTEXT_INT(field) CONTEXT_FIELD(field, TYPE_INT)
#define CONTEXT_INT2(field) CONTEXT_FIELD(field, TYPE_INT_2)
#define CONTEXT_INT64(field) CONTEXT_FIELD(field, TYPE_INT64)
#define CONTEXT_ENUM(field) CONTEXT_FIELD(field, TYPE_ENUM)
#define CONTEXT_ENUM2(field) CONTEXT_FIELD(field, TYPE_ENUM_2)
#define CONTEXT_BOOL(field) CONTEXT_FIELD(field, TYPE_BOOLEAN)
#define CONTEXT_BIT0(field) CONTEXT_FIELD(field, TYPE_BIT_0)
#define CONTEXT_BIT1(field) CONTEXT_FIELD(field, TYPE_BIT_1)
#define CONTEXT_BIT2(field) CONTEXT_FIELD(field, TYPE_BIT_2)
#define CONTEXT_BIT3(field) CONTEXT_FIELD(field, TYPE_BIT_3)
#define CONTEXT_BIT4(field) CONTEXT_FIELD(field, TYPE_BIT_4)
#define CONTEXT_BIT5(field) CONTEXT_FIELD(field, TYPE_BIT_5)
#define CONTEXT_BIT6(field) CONTEXT_FIELD(field, TYPE_BIT_6)
#define CONTEXT_BIT7(field) CONTEXT_FIELD(field, TYPE_BIT_7)
#define CONTEXT_FLOAT(field) CONTEXT_FIELD(field, TYPE_FLOAT)
#define CONTEXT_FLOAT2(field) CONTEXT_FIELD(field, TYPE_FLOAT_2)
#define CONTEXT_FLOAT3(field) CONTEXT_FIELD(field, TYPE_FLOAT_3)
#define CONTEXT_FLOAT4(field) CONTEXT_FIELD(field, TYPE_FLOAT_4)
#define CONTEXT_MATRIX(field) CONTEXT_FIELD(field, TYPE_MATRIX)
#define CONTEXT_MATRIX_T(field) CONTEXT_FIELD(field, TYPE_MATRIX_T)

#define ARRAY_INT(field) ARRAY_FIELD(field, TYPE_INT)
#define ARRAY_ENUM(field) ARRAY_FIELD(field, TYPE_ENUM)
#define ARRAY_BOOL(field) ARRAY_FIELD(field, TYPE_BOOLEAN)

#define EXT(f)					\
   offsetof(struct gl_extensions, f)

#define EXTRA_EXT(e)				\
   static const int extra_##e[] = {		\
      EXT(e), EXTRA_END				\
   }

#define EXTRA_EXT2(e1, e2)			\
   static const int extra_##e1##_##e2[] = {	\
      EXT(e1), EXT(e2), EXTRA_END		\
   }

/* The 'extra' mechanism is a way to specify extra checks (such as
 * extensions or specific gl versions) or actions (flush current, new
 * buffers) that we need to do before looking up an enum.  We need to
 * declare them all up front so we can refer to them in the value_desc
 * structs below. */

static const int extra_new_buffers[] = {
   EXTRA_NEW_BUFFERS,
   EXTRA_END
};

static const int extra_new_frag_clamp[] = {
   EXTRA_NEW_FRAG_CLAMP,
   EXTRA_END
};

static const int extra_valid_draw_buffer[] = {
   EXTRA_VALID_DRAW_BUFFER,
   EXTRA_END
};

static const int extra_valid_texture_unit[] = {
   EXTRA_VALID_TEXTURE_UNIT,
   EXTRA_END
};

static const int extra_valid_clip_distance[] = {
   EXTRA_VALID_CLIP_DISTANCE,
   EXTRA_END
};

static const int extra_flush_current_valid_texture_unit[] = {
   EXTRA_FLUSH_CURRENT,
   EXTRA_VALID_TEXTURE_UNIT,
   EXTRA_END
};

static const int extra_flush_current[] = {
   EXTRA_FLUSH_CURRENT,
   EXTRA_END
};

static const int extra_EXT_secondary_color_flush_current[] = {
   EXT(EXT_secondary_color),
   EXTRA_FLUSH_CURRENT,
   EXTRA_END
};

static const int extra_EXT_fog_coord_flush_current[] = {
   EXT(EXT_fog_coord),
   EXTRA_FLUSH_CURRENT,
   EXTRA_END
};

static const int extra_EXT_texture_integer[] = {
   EXT(EXT_texture_integer),
   EXTRA_END
};

static const int extra_GLSL_130[] = {
   EXTRA_GLSL_130,
   EXTRA_END
};

static const int extra_ARB_sampler_objects[] = {
   EXT(ARB_sampler_objects),
   EXTRA_END
};


EXTRA_EXT(ARB_ES2_compatibility);
EXTRA_EXT(ARB_texture_cube_map);
EXTRA_EXT(MESA_texture_array);
EXTRA_EXT2(EXT_secondary_color, ARB_vertex_program);
EXTRA_EXT(EXT_secondary_color);
EXTRA_EXT(EXT_fog_coord);
EXTRA_EXT(NV_fog_distance);
EXTRA_EXT(EXT_texture_filter_anisotropic);
EXTRA_EXT(IBM_rasterpos_clip);
EXTRA_EXT(NV_point_sprite);
EXTRA_EXT(NV_vertex_program);
EXTRA_EXT(NV_fragment_program);
EXTRA_EXT(NV_texture_rectangle);
EXTRA_EXT(EXT_stencil_two_side);
EXTRA_EXT(NV_light_max_exponent);
EXTRA_EXT(EXT_depth_bounds_test);
EXTRA_EXT(ARB_depth_clamp);
EXTRA_EXT(ATI_fragment_shader);
EXTRA_EXT(EXT_framebuffer_blit);
EXTRA_EXT(ARB_shader_objects);
EXTRA_EXT(EXT_provoking_vertex);
EXTRA_EXT(ARB_fragment_shader);
EXTRA_EXT(ARB_fragment_program);
EXTRA_EXT2(ARB_framebuffer_object, EXT_framebuffer_multisample);
EXTRA_EXT(EXT_framebuffer_object);
EXTRA_EXT(APPLE_vertex_array_object);
EXTRA_EXT(ARB_seamless_cube_map);
EXTRA_EXT(EXT_compiled_vertex_array);
EXTRA_EXT(ARB_sync);
EXTRA_EXT(ARB_vertex_shader);
EXTRA_EXT(EXT_transform_feedback);
EXTRA_EXT(ARB_transform_feedback2);
EXTRA_EXT(EXT_pixel_buffer_object);
EXTRA_EXT(ARB_vertex_program);
EXTRA_EXT2(NV_point_sprite, ARB_point_sprite);
EXTRA_EXT2(ARB_fragment_program, NV_fragment_program);
EXTRA_EXT2(ARB_vertex_program, NV_vertex_program);
EXTRA_EXT2(ARB_vertex_program, ARB_fragment_program);
EXTRA_EXT(ARB_geometry_shader4);
EXTRA_EXT(ARB_color_buffer_float);
EXTRA_EXT(ARB_copy_buffer);
EXTRA_EXT(EXT_framebuffer_sRGB);
EXTRA_EXT(ARB_texture_buffer_object);
EXTRA_EXT(OES_EGL_image_external);

static const int
extra_ARB_vertex_program_ARB_fragment_program_NV_vertex_program[] = {
   EXT(ARB_vertex_program),
   EXT(ARB_fragment_program),
   EXT(NV_vertex_program),
   EXTRA_END
};

static const int
extra_NV_vertex_program_ARB_vertex_program_ARB_fragment_program_NV_vertex_program[] = {
   EXT(NV_vertex_program),
   EXT(ARB_vertex_program),
   EXT(ARB_fragment_program),
   EXT(NV_vertex_program),
   EXTRA_END
};

static const int
extra_NV_primitive_restart[] = {
   EXT(NV_primitive_restart),
   EXTRA_END
};

static const int extra_version_30[] = { EXTRA_VERSION_30, EXTRA_END };
static const int extra_version_31[] = { EXTRA_VERSION_31, EXTRA_END };
static const int extra_version_32[] = { EXTRA_VERSION_32, EXTRA_END };

static const int
extra_ARB_vertex_program_version_es2[] = {
   EXT(ARB_vertex_program),
   EXTRA_VERSION_ES2,
   EXTRA_END
};

#define API_OPENGL_BIT (1 << API_OPENGL)
#define API_OPENGLES_BIT (1 << API_OPENGLES)
#define API_OPENGLES2_BIT (1 << API_OPENGLES2)

/* This is the big table describing all the enums we accept in
 * glGet*v().  The table is partitioned into six parts: enums
 * understood by all GL APIs (OpenGL, GLES and GLES2), enums shared
 * between OpenGL and GLES, enums exclusive to GLES, etc for the
 * remaining combinations.  When we add the enums to the hash table in
 * _mesa_init_get_hash(), we only add the enums for the API we're
 * instantiating and the different sections are guarded by #if
 * FEATURE_GL etc to make sure we only compile in the enums we may
 * need. */

static const struct value_desc values[] = {
   /* Enums shared between OpenGL, GLES1 and GLES2 */
   { 0, 0, TYPE_API_MASK,
     API_OPENGL_BIT | API_OPENGLES_BIT | API_OPENGLES2_BIT, NO_EXTRA},
   { GL_ALPHA_BITS, BUFFER_INT(Visual.alphaBits), extra_new_buffers },
   { GL_BLEND, CONTEXT_BIT0(Color.BlendEnabled), NO_EXTRA },
   { GL_BLEND_SRC, CONTEXT_ENUM(Color.Blend[0].SrcRGB), NO_EXTRA },
   { GL_BLUE_BITS, BUFFER_INT(Visual.blueBits), extra_new_buffers },
   { GL_COLOR_CLEAR_VALUE, LOC_CUSTOM, TYPE_FLOATN_4, 0, extra_new_frag_clamp },
   { GL_COLOR_WRITEMASK, LOC_CUSTOM, TYPE_INT_4, 0, NO_EXTRA },
   { GL_CULL_FACE, CONTEXT_BOOL(Polygon.CullFlag), NO_EXTRA },
   { GL_CULL_FACE_MODE, CONTEXT_ENUM(Polygon.CullFaceMode), NO_EXTRA },
   { GL_DEPTH_BITS, BUFFER_INT(Visual.depthBits), NO_EXTRA },
   { GL_DEPTH_CLEAR_VALUE, CONTEXT_FIELD(Depth.Clear, TYPE_DOUBLEN), NO_EXTRA },
   { GL_DEPTH_FUNC, CONTEXT_ENUM(Depth.Func), NO_EXTRA },
   { GL_DEPTH_RANGE, CONTEXT_FIELD(Viewport.Near, TYPE_FLOATN_2), NO_EXTRA },
   { GL_DEPTH_TEST, CONTEXT_BOOL(Depth.Test), NO_EXTRA },
   { GL_DEPTH_WRITEMASK, CONTEXT_BOOL(Depth.Mask), NO_EXTRA },
   { GL_DITHER, CONTEXT_BOOL(Color.DitherFlag), NO_EXTRA },
   { GL_FRONT_FACE, CONTEXT_ENUM(Polygon.FrontFace), NO_EXTRA },
   { GL_GREEN_BITS, BUFFER_INT(Visual.greenBits), extra_new_buffers },
   { GL_LINE_WIDTH, CONTEXT_FLOAT(Line.Width), NO_EXTRA },
   { GL_ALIASED_LINE_WIDTH_RANGE, CONTEXT_FLOAT2(Const.MinLineWidth), NO_EXTRA },
   { GL_MAX_ELEMENTS_VERTICES, CONTEXT_INT(Const.MaxArrayLockSize), NO_EXTRA },
   { GL_MAX_ELEMENTS_INDICES, CONTEXT_INT(Const.MaxArrayLockSize), NO_EXTRA },
   { GL_MAX_TEXTURE_SIZE, LOC_CUSTOM, TYPE_INT,
     offsetof(struct gl_context, Const.MaxTextureLevels), NO_EXTRA },
   { GL_MAX_VIEWPORT_DIMS, CONTEXT_INT2(Const.MaxViewportWidth), NO_EXTRA },
   { GL_PACK_ALIGNMENT, CONTEXT_INT(Pack.Alignment), NO_EXTRA },
   { GL_ALIASED_POINT_SIZE_RANGE, CONTEXT_FLOAT2(Const.MinPointSize), NO_EXTRA },
   { GL_POLYGON_OFFSET_FACTOR, CONTEXT_FLOAT(Polygon.OffsetFactor ), NO_EXTRA },
   { GL_POLYGON_OFFSET_UNITS, CONTEXT_FLOAT(Polygon.OffsetUnits ), NO_EXTRA },
   { GL_POLYGON_OFFSET_FILL, CONTEXT_BOOL(Polygon.OffsetFill), NO_EXTRA },
   { GL_RED_BITS, BUFFER_INT(Visual.redBits), extra_new_buffers },
   { GL_SCISSOR_BOX, LOC_CUSTOM, TYPE_INT_4, 0, NO_EXTRA },
   { GL_SCISSOR_TEST, CONTEXT_BOOL(Scissor.Enabled), NO_EXTRA },
   { GL_STENCIL_BITS, BUFFER_INT(Visual.stencilBits), NO_EXTRA },
   { GL_STENCIL_CLEAR_VALUE, CONTEXT_INT(Stencil.Clear), NO_EXTRA },
   { GL_STENCIL_FAIL, LOC_CUSTOM, TYPE_ENUM, NO_OFFSET, NO_EXTRA },
   { GL_STENCIL_FUNC, LOC_CUSTOM, TYPE_ENUM, NO_OFFSET, NO_EXTRA },
   { GL_STENCIL_PASS_DEPTH_FAIL, LOC_CUSTOM, TYPE_ENUM, NO_OFFSET, NO_EXTRA },
   { GL_STENCIL_PASS_DEPTH_PASS, LOC_CUSTOM, TYPE_ENUM, NO_OFFSET, NO_EXTRA },
   { GL_STENCIL_REF, LOC_CUSTOM, TYPE_INT, NO_OFFSET, NO_EXTRA },
   { GL_STENCIL_TEST, CONTEXT_BOOL(Stencil.Enabled), NO_EXTRA },
   { GL_STENCIL_VALUE_MASK, LOC_CUSTOM, TYPE_INT, NO_OFFSET, NO_EXTRA },
   { GL_STENCIL_WRITEMASK, LOC_CUSTOM, TYPE_INT, NO_OFFSET, NO_EXTRA },
   { GL_SUBPIXEL_BITS, CONTEXT_INT(Const.SubPixelBits), NO_EXTRA },
   { GL_TEXTURE_BINDING_2D, LOC_CUSTOM, TYPE_INT, TEXTURE_2D_INDEX, NO_EXTRA },
   { GL_UNPACK_ALIGNMENT, CONTEXT_INT(Unpack.Alignment), NO_EXTRA },
   { GL_VIEWPORT, LOC_CUSTOM, TYPE_INT_4, 0, NO_EXTRA },

   /* GL_ARB_multitexture */
   { GL_ACTIVE_TEXTURE, LOC_CUSTOM, TYPE_INT, 0, NO_EXTRA },

   /* Note that all the OES_* extensions require that the Mesa "struct
    * gl_extensions" include a member with the name of the extension.
    * That structure does not yet include OES extensions (and we're
    * not sure whether it will).  If it does, all the OES_*
    * extensions below should mark the dependency. */

   /* GL_ARB_texture_cube_map */
   { GL_TEXTURE_BINDING_CUBE_MAP_ARB, LOC_CUSTOM, TYPE_INT,
     TEXTURE_CUBE_INDEX, extra_ARB_texture_cube_map },
   { GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, LOC_CUSTOM, TYPE_INT,
     offsetof(struct gl_context, Const.MaxCubeTextureLevels),
     extra_ARB_texture_cube_map }, /* XXX: OES_texture_cube_map */

   /* XXX: OES_blend_subtract */
   { GL_BLEND_SRC_RGB_EXT, CONTEXT_ENUM(Color.Blend[0].SrcRGB), NO_EXTRA },
   { GL_BLEND_DST_RGB_EXT, CONTEXT_ENUM(Color.Blend[0].DstRGB), NO_EXTRA },
   { GL_BLEND_SRC_ALPHA_EXT, CONTEXT_ENUM(Color.Blend[0].SrcA), NO_EXTRA },
   { GL_BLEND_DST_ALPHA_EXT, CONTEXT_ENUM(Color.Blend[0].DstA), NO_EXTRA },

   /* GL_BLEND_EQUATION_RGB, which is what we're really after, is
    * defined identically to GL_BLEND_EQUATION. */
   { GL_BLEND_EQUATION, CONTEXT_ENUM(Color.Blend[0].EquationRGB), NO_EXTRA },
   { GL_BLEND_EQUATION_ALPHA_EXT, CONTEXT_ENUM(Color.Blend[0].EquationA), NO_EXTRA },

   /* GL_ARB_texture_compression */
   { GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, LOC_CUSTOM, TYPE_INT, 0, NO_EXTRA },
   { GL_COMPRESSED_TEXTURE_FORMATS_ARB, LOC_CUSTOM, TYPE_INT_N, 0, NO_EXTRA },

   /* GL_ARB_multisample */
   { GL_SAMPLE_ALPHA_TO_COVERAGE_ARB,
     CONTEXT_BOOL(Multisample.SampleAlphaToCoverage), NO_EXTRA },
   { GL_SAMPLE_COVERAGE_ARB, CONTEXT_BOOL(Multisample.SampleCoverage), NO_EXTRA },
   { GL_SAMPLE_COVERAGE_VALUE_ARB,
     CONTEXT_FLOAT(Multisample.SampleCoverageValue), NO_EXTRA },
   { GL_SAMPLE_COVERAGE_INVERT_ARB,
     CONTEXT_BOOL(Multisample.SampleCoverageInvert), NO_EXTRA },
   { GL_SAMPLE_BUFFERS_ARB, BUFFER_INT(Visual.sampleBuffers), NO_EXTRA },
   { GL_SAMPLES_ARB, BUFFER_INT(Visual.samples), NO_EXTRA },

   /* GL_SGIS_generate_mipmap */
   { GL_GENERATE_MIPMAP_HINT_SGIS, CONTEXT_ENUM(Hint.GenerateMipmap), NO_EXTRA },

   /* GL_ARB_vertex_buffer_object */
   { GL_ARRAY_BUFFER_BINDING_ARB, LOC_CUSTOM, TYPE_INT, 0, NO_EXTRA },

   /* GL_ARB_vertex_buffer_object */
   /* GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB - not supported */
   { GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, LOC_CUSTOM, TYPE_INT, 0, NO_EXTRA },

   /* GL_ARB_color_buffer_float */
   { GL_CLAMP_VERTEX_COLOR, CONTEXT_ENUM(Light.ClampVertexColor), extra_ARB_color_buffer_float },
   { GL_CLAMP_FRAGMENT_COLOR, CONTEXT_ENUM(Color.ClampFragmentColor), extra_ARB_color_buffer_float },
   { GL_CLAMP_READ_COLOR, CONTEXT_ENUM(Color.ClampReadColor), extra_ARB_color_buffer_float },

   /* GL_ARB_copy_buffer */
   { GL_COPY_READ_BUFFER, LOC_CUSTOM, TYPE_INT, 0, extra_ARB_copy_buffer },
   { GL_COPY_WRITE_BUFFER, LOC_CUSTOM, TYPE_INT, 0, extra_ARB_copy_buffer },

   /* GL_OES_read_format */
   { GL_IMPLEMENTATION_COLOR_READ_TYPE_OES, LOC_CUSTOM, TYPE_INT, 0,
     extra_new_buffers },
   { GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES, LOC_CUSTOM, TYPE_INT, 0,
     extra_new_buffers },

   /* GL_EXT_framebuffer_object */
   { GL_FRAMEBUFFER_BINDING_EXT, BUFFER_INT(Name),
     extra_EXT_framebuffer_object },
   { GL_RENDERBUFFER_BINDING_EXT, LOC_CUSTOM, TYPE_INT, 0,
     extra_EXT_framebuffer_object },
   { GL_MAX_RENDERBUFFER_SIZE_EXT, CONTEXT_INT(Const.MaxRenderbufferSize),
     extra_EXT_framebuffer_object },

   /* This entry isn't spec'ed for GLES 2, but is needed for Mesa's
    * GLSL: */
   { GL_MAX_CLIP_PLANES, CONTEXT_INT(Const.MaxClipPlanes), NO_EXTRA },

#if FEATURE_GL || FEATURE_ES1
   /* Enums in OpenGL and GLES1 */
   { 0, 0, TYPE_API_MASK, API_OPENGL_BIT | API_OPENGLES_BIT, NO_EXTRA },
   { GL_MAX_LIGHTS, CONTEXT_INT(Const.MaxLights), NO_EXTRA },
   { GL_LIGHT0, CONTEXT_BOOL(Light.Light[0].Enabled), NO_EXTRA },
   { GL_LIGHT1, CONTEXT_BOOL(Light.Light[1].Enabled), NO_EXTRA },
   { GL_LIGHT2, CONTEXT_BOOL(Light.Light[2].Enabled), NO_EXTRA },
   { GL_LIGHT3, CONTEXT_BOOL(Light.Light[3].Enabled), NO_EXTRA },
   { GL_LIGHT4, CONTEXT_BOOL(Light.Light[4].Enabled), NO_EXTRA },
   { GL_LIGHT5, CONTEXT_BOOL(Light.Light[5].Enabled), NO_EXTRA },
   { GL_LIGHT6, CONTEXT_BOOL(Light.Light[6].Enabled), NO_EXTRA },
   { GL_LIGHT7, CONTEXT_BOOL(Light.Light[7].Enabled), NO_EXTRA },
   { GL_LIGHTING, CONTEXT_BOOL(Light.Enabled), NO_EXTRA },
   { GL_LIGHT_MODEL_AMBIENT,
     CONTEXT_FIELD(Light.Model.Ambient[0], TYPE_FLOATN_4), NO_EXTRA },
   { GL_LIGHT_MODEL_TWO_SIDE, CONTEXT_BOOL(Light.Model.TwoSide), NO_EXTRA },
   { GL_ALPHA_TEST, CONTEXT_BOOL(Color.AlphaEnabled), NO_EXTRA },
   { GL_ALPHA_TEST_FUNC, CONTEXT_ENUM(Color.AlphaFunc), NO_EXTRA },
   { GL_ALPHA_TEST_REF, LOC_CUSTOM, TYPE_FLOATN, 0, extra_new_frag_clamp },
   { GL_BLEND_DST, CONTEXT_ENUM(Color.Blend[0].DstRGB), NO_EXTRA },
   { GL_CLIP_DISTANCE0, CONTEXT_BIT0(Transform.ClipPlanesEnabled), extra_valid_clip_distance },
   { GL_CLIP_DISTANCE1, CONTEXT_BIT1(Transform.ClipPlanesEnabled), extra_valid_clip_distance },
   { GL_CLIP_DISTANCE2, CONTEXT_BIT2(Transform.ClipPlanesEnabled), extra_valid_clip_distance },
   { GL_CLIP_DISTANCE3, CONTEXT_BIT3(Transform.ClipPlanesEnabled), extra_valid_clip_distance },
   { GL_CLIP_DISTANCE4, CONTEXT_BIT4(Transform.ClipPlanesEnabled), extra_valid_clip_distance },
   { GL_CLIP_DISTANCE5, CONTEXT_BIT5(Transform.ClipPlanesEnabled), extra_valid_clip_distance },
   { GL_CLIP_DISTANCE6, CONTEXT_BIT6(Transform.ClipPlanesEnabled), extra_valid_clip_distance },
   { GL_CLIP_DISTANCE7, CONTEXT_BIT7(Transform.ClipPlanesEnabled), extra_valid_clip_distance },
   { GL_COLOR_MATERIAL, CONTEXT_BOOL(Light.ColorMaterialEnabled), NO_EXTRA },
   { GL_CURRENT_COLOR,
     CONTEXT_FIELD(Current.Attrib[VERT_ATTRIB_COLOR0][0], TYPE_FLOATN_4),
     extra_flush_current },
   { GL_CURRENT_NORMAL,
     CONTEXT_FIELD(Current.Attrib[VERT_ATTRIB_NORMAL][0], TYPE_FLOATN_3),
     extra_flush_current },
   { GL_CURRENT_TEXTURE_COORDS, LOC_CUSTOM, TYPE_FLOAT_4, 0,
     extra_flush_current_valid_texture_unit },
   { GL_DISTANCE_ATTENUATION_EXT, CONTEXT_FLOAT3(Point.Params[0]), NO_EXTRA },
   { GL_FOG, CONTEXT_BOOL(Fog.Enabled), NO_EXTRA },
   { GL_FOG_COLOR, LOC_CUSTOM, TYPE_FLOATN_4, 0, extra_new_frag_clamp },
   { GL_FOG_DENSITY, CONTEXT_FLOAT(Fog.Density), NO_EXTRA },
   { GL_FOG_END, CONTEXT_FLOAT(Fog.End), NO_EXTRA },
   { GL_FOG_HINT, CONTEXT_ENUM(Hint.Fog), NO_EXTRA },
   { GL_FOG_MODE, CONTEXT_ENUM(Fog.Mode), NO_EXTRA },
   { GL_FOG_START, CONTEXT_FLOAT(Fog.Start), NO_EXTRA },
   { GL_LINE_SMOOTH, CONTEXT_BOOL(Line.SmoothFlag), NO_EXTRA },
   { GL_LINE_SMOOTH_HINT, CONTEXT_ENUM(Hint.LineSmooth), NO_EXTRA },
   { GL_LINE_WIDTH_RANGE, CONTEXT_FLOAT2(Const.MinLineWidthAA), NO_EXTRA },
   { GL_COLOR_LOGIC_OP, CONTEXT_BOOL(Color.ColorLogicOpEnabled), NO_EXTRA },
   { GL_LOGIC_OP_MODE, CONTEXT_ENUM(Color.LogicOp), NO_EXTRA },
   { GL_MATRIX_MODE, CONTEXT_ENUM(Transform.MatrixMode), NO_EXTRA },
   { GL_MAX_MODELVIEW_STACK_DEPTH, CONST(MAX_MODELVIEW_STACK_DEPTH), NO_EXTRA },
   { GL_MAX_PROJECTION_STACK_DEPTH, CONST(MAX_PROJECTION_STACK_DEPTH), NO_EXTRA },
   { GL_MAX_TEXTURE_STACK_DEPTH, CONST(MAX_TEXTURE_STACK_DEPTH), NO_EXTRA },
   { GL_MODELVIEW_MATRIX, CONTEXT_MATRIX(ModelviewMatrixStack.Top), NO_EXTRA },
   { GL_MODELVIEW_STACK_DEPTH, LOC_CUSTOM, TYPE_INT,
     offsetof(struct gl_context, ModelviewMatrixStack.Depth), NO_EXTRA },
   { GL_NORMALIZE, CONTEXT_BOOL(Transform.Normalize), NO_EXTRA },
   { GL_PACK_SKIP_IMAGES_EXT, CONTEXT_INT(Pack.SkipImages), NO_EXTRA },
   { GL_PERSPECTIVE_CORRECTION_HINT, CONTEXT_ENUM(Hint.PerspectiveCorrection), NO_EXTRA },
   { GL_POINT_SIZE, CONTEXT_FLOAT(Point.Size), NO_EXTRA },
   { GL_POINT_SIZE_RANGE, CONTEXT_FLOAT2(Const.MinPointSizeAA), NO_EXTRA },
   { GL_POINT_SMOOTH, CONTEXT_BOOL(Point.SmoothFlag), NO_EXTRA },
   { GL_POINT_SMOOTH_HINT, CONTEXT_ENUM(Hint.PointSmooth), NO_EXTRA },
   { GL_POINT_SIZE_MIN_EXT, CONTEXT_FLOAT(Point.MinSize), NO_EXTRA },
   { GL_POINT_SIZE_MAX_EXT, CONTEXT_FLOAT(Point.MaxSize), NO_EXTRA },
   { GL_POINT_FADE_THRESHOLD_SIZE_EXT, CONTEXT_FLOAT(Point.Threshold), NO_EXTRA },
   { GL_PROJECTION_MATRIX, CONTEXT_MATRIX(ProjectionMatrixStack.Top), NO_EXTRA },
   { GL_PROJECTION_STACK_DEPTH, LOC_CUSTOM, TYPE_INT,
     offsetof(struct gl_context, ProjectionMatrixStack.Depth), NO_EXTRA },
   { GL_RESCALE_NORMAL, CONTEXT_BOOL(Transform.RescaleNormals), NO_EXTRA },
   { GL_SHADE_MODEL, CONTEXT_ENUM(Light.ShadeModel), NO_EXTRA },
   { GL_TEXTURE_2D, LOC_CUSTOM, TYPE_BOOLEAN, 0, NO_EXTRA },
   { GL_TEXTURE_MATRIX, LOC_CUSTOM, TYPE_MATRIX, 0, extra_valid_texture_unit },
   { GL_TEXTURE_STACK_DEPTH, LOC_CUSTOM, TYPE_INT, 0,
     extra_valid_texture_unit  },

   { GL_VERTEX_ARRAY, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_POS].Enabled), NO_EXTRA },
   { GL_VERTEX_ARRAY_SIZE, ARRAY_INT(VertexAttrib[VERT_ATTRIB_POS].Size), NO_EXTRA },
   { GL_VERTEX_ARRAY_TYPE, ARRAY_ENUM(VertexAttrib[VERT_ATTRIB_POS].Type), NO_EXTRA },
   { GL_VERTEX_ARRAY_STRIDE, ARRAY_INT(VertexAttrib[VERT_ATTRIB_POS].Stride), NO_EXTRA },
   { GL_NORMAL_ARRAY, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_NORMAL].Enabled), NO_EXTRA },
   { GL_NORMAL_ARRAY_TYPE, ARRAY_ENUM(VertexAttrib[VERT_ATTRIB_NORMAL].Type), NO_EXTRA },
   { GL_NORMAL_ARRAY_STRIDE, ARRAY_INT(VertexAttrib[VERT_ATTRIB_NORMAL].Stride), NO_EXTRA },
   { GL_COLOR_ARRAY, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_COLOR0].Enabled), NO_EXTRA },
   { GL_COLOR_ARRAY_SIZE, ARRAY_INT(VertexAttrib[VERT_ATTRIB_COLOR0].Size), NO_EXTRA },
   { GL_COLOR_ARRAY_TYPE, ARRAY_ENUM(VertexAttrib[VERT_ATTRIB_COLOR0].Type), NO_EXTRA },
   { GL_COLOR_ARRAY_STRIDE, ARRAY_INT(VertexAttrib[VERT_ATTRIB_COLOR0].Stride), NO_EXTRA },
   { GL_TEXTURE_COORD_ARRAY,
     LOC_CUSTOM, TYPE_BOOLEAN, offsetof(struct gl_client_array, Enabled), NO_EXTRA },
   { GL_TEXTURE_COORD_ARRAY_SIZE,
     LOC_CUSTOM, TYPE_INT, offsetof(struct gl_client_array, Size), NO_EXTRA },
   { GL_TEXTURE_COORD_ARRAY_TYPE,
     LOC_CUSTOM, TYPE_ENUM, offsetof(struct gl_client_array, Type), NO_EXTRA },
   { GL_TEXTURE_COORD_ARRAY_STRIDE,
     LOC_CUSTOM, TYPE_INT, offsetof(struct gl_client_array, Stride), NO_EXTRA },

   /* GL_ARB_ES2_compatibility */
   { GL_SHADER_COMPILER, CONST(1), extra_ARB_ES2_compatibility },
   { GL_MAX_VARYING_VECTORS, CONTEXT_INT(Const.MaxVarying),
     extra_ARB_ES2_compatibility },
   { GL_MAX_VERTEX_UNIFORM_VECTORS, LOC_CUSTOM, TYPE_INT, 0,
     extra_ARB_ES2_compatibility },
   { GL_MAX_FRAGMENT_UNIFORM_VECTORS, LOC_CUSTOM, TYPE_INT, 0,
     extra_ARB_ES2_compatibility },

   /* GL_ARB_multitexture */
   { GL_MAX_TEXTURE_UNITS, CONTEXT_INT(Const.MaxTextureUnits), NO_EXTRA },
   { GL_CLIENT_ACTIVE_TEXTURE, LOC_CUSTOM, TYPE_INT, 0, NO_EXTRA },

   /* GL_ARB_texture_cube_map */
   { GL_TEXTURE_CUBE_MAP_ARB, LOC_CUSTOM, TYPE_BOOLEAN, 0, NO_EXTRA },
   /* S, T, and R are always set at the same time */
   { GL_TEXTURE_GEN_STR_OES, LOC_TEXUNIT, TYPE_BIT_0,
     offsetof(struct gl_texture_unit, TexGenEnabled), NO_EXTRA },

   /* GL_ARB_multisample */
   { GL_MULTISAMPLE_ARB, CONTEXT_BOOL(Multisample.Enabled), NO_EXTRA },
   { GL_SAMPLE_ALPHA_TO_ONE_ARB, CONTEXT_BOOL(Multisample.SampleAlphaToOne), NO_EXTRA },

   /* GL_ARB_vertex_buffer_object */
   { GL_VERTEX_ARRAY_BUFFER_BINDING_ARB, LOC_CUSTOM, TYPE_INT,
     offsetof(struct gl_array_object, VertexAttrib[VERT_ATTRIB_POS].BufferObj), NO_EXTRA },
   { GL_NORMAL_ARRAY_BUFFER_BINDING_ARB, LOC_CUSTOM, TYPE_INT,
     offsetof(struct gl_array_object, VertexAttrib[VERT_ATTRIB_NORMAL].BufferObj), NO_EXTRA },
   { GL_COLOR_ARRAY_BUFFER_BINDING_ARB, LOC_CUSTOM, TYPE_INT,
     offsetof(struct gl_array_object, VertexAttrib[VERT_ATTRIB_COLOR0].BufferObj), NO_EXTRA },
   { GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB, LOC_CUSTOM, TYPE_INT, NO_OFFSET, NO_EXTRA },

   /* GL_OES_point_sprite */
   { GL_POINT_SPRITE_NV,
     CONTEXT_BOOL(Point.PointSprite),
     extra_NV_point_sprite_ARB_point_sprite },

   /* GL_ARB_fragment_shader */
   { GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB,
     CONTEXT_INT(Const.FragmentProgram.MaxUniformComponents),
     extra_ARB_fragment_shader },

   /* GL_ARB_vertex_shader */
   { GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB,
     CONTEXT_INT(Const.VertexProgram.MaxUniformComponents),
     extra_ARB_vertex_shader },
   { GL_MAX_VARYING_FLOATS_ARB, LOC_CUSTOM, TYPE_INT, 0,
     extra_ARB_vertex_shader },

   /* GL_EXT_texture_lod_bias */
   { GL_MAX_TEXTURE_LOD_BIAS_EXT, CONTEXT_FLOAT(Const.MaxTextureLodBias),
     NO_EXTRA },

   /* GL_EXT_texture_filter_anisotropic */
   { GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,
     CONTEXT_FLOAT(Const.MaxTextureMaxAnisotropy),
     extra_EXT_texture_filter_anisotropic },
#endif /* FEATURE_GL || FEATURE_ES1 */

#if FEATURE_ES1
   { 0, 0, TYPE_API_MASK, API_OPENGLES_BIT },
   /* XXX: OES_matrix_get */
   { GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES },
   { GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES },
   { GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES },

   /* OES_point_size_array */
   { GL_POINT_SIZE_ARRAY_OES, ARRAY_FIELD(VertexAttrib[VERT_ATTRIB_POINT_SIZE].Enabled, TYPE_BOOLEAN) },
   { GL_POINT_SIZE_ARRAY_TYPE_OES, ARRAY_FIELD(VertexAttrib[VERT_ATTRIB_POINT_SIZE].Type, TYPE_ENUM) },
   { GL_POINT_SIZE_ARRAY_STRIDE_OES, ARRAY_FIELD(VertexAttrib[VERT_ATTRIB_POINT_SIZE].Stride, TYPE_INT) },
   { GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES, LOC_CUSTOM, TYPE_INT, 0 },
#endif /* FEATURE_ES1 */

#if FEATURE_GL || FEATURE_ES2
   { 0, 0, TYPE_API_MASK, API_OPENGL_BIT | API_OPENGLES2_BIT, NO_EXTRA },
   { GL_MAX_TEXTURE_COORDS_ARB, /* == GL_MAX_TEXTURE_COORDS_NV */
     CONTEXT_INT(Const.MaxTextureCoordUnits),
     extra_ARB_fragment_program_NV_fragment_program },

   /* GL_ARB_draw_buffers */
   { GL_MAX_DRAW_BUFFERS_ARB, CONTEXT_INT(Const.MaxDrawBuffers), NO_EXTRA },

   /* GL_EXT_framebuffer_object / GL_NV_fbo_color_attachments */
   { GL_MAX_COLOR_ATTACHMENTS, CONTEXT_INT(Const.MaxColorAttachments),
     extra_EXT_framebuffer_object },

   /* GL_ARB_draw_buffers / GL_NV_draw_buffers (for ES 2.0) */
   { GL_DRAW_BUFFER0_ARB, BUFFER_ENUM(ColorDrawBuffer[0]), NO_EXTRA },
   { GL_DRAW_BUFFER1_ARB, BUFFER_ENUM(ColorDrawBuffer[1]),
     extra_valid_draw_buffer },
   { GL_DRAW_BUFFER2_ARB, BUFFER_ENUM(ColorDrawBuffer[2]),
     extra_valid_draw_buffer },
   { GL_DRAW_BUFFER3_ARB, BUFFER_ENUM(ColorDrawBuffer[3]),
     extra_valid_draw_buffer },
   { GL_DRAW_BUFFER4_ARB, BUFFER_ENUM(ColorDrawBuffer[4]),
     extra_valid_draw_buffer },
   { GL_DRAW_BUFFER5_ARB, BUFFER_ENUM(ColorDrawBuffer[5]),
     extra_valid_draw_buffer },
   { GL_DRAW_BUFFER6_ARB, BUFFER_ENUM(ColorDrawBuffer[6]),
     extra_valid_draw_buffer },
   { GL_DRAW_BUFFER7_ARB, BUFFER_ENUM(ColorDrawBuffer[7]),
     extra_valid_draw_buffer },

   { GL_BLEND_COLOR_EXT, LOC_CUSTOM, TYPE_FLOATN_4, 0, extra_new_frag_clamp },
   /* GL_ARB_fragment_program */
   { GL_MAX_TEXTURE_IMAGE_UNITS_ARB, /* == GL_MAX_TEXTURE_IMAGE_UNITS_NV */
     CONTEXT_INT(Const.MaxTextureImageUnits),
     extra_ARB_fragment_program_NV_fragment_program },
   { GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB,
     CONTEXT_INT(Const.MaxVertexTextureImageUnits), extra_ARB_vertex_shader },
   { GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB,
     CONTEXT_INT(Const.MaxCombinedTextureImageUnits),
     extra_ARB_vertex_shader },

   /* GL_ARB_shader_objects
    * Actually, this token isn't part of GL_ARB_shader_objects, but is
    * close enough for now. */
   { GL_CURRENT_PROGRAM, LOC_CUSTOM, TYPE_INT, 0, extra_ARB_shader_objects },

   /* OpenGL 2.0 */
   { GL_STENCIL_BACK_FUNC, CONTEXT_ENUM(Stencil.Function[1]), NO_EXTRA },
   { GL_STENCIL_BACK_VALUE_MASK, CONTEXT_INT(Stencil.ValueMask[1]), NO_EXTRA },
   { GL_STENCIL_BACK_WRITEMASK, CONTEXT_INT(Stencil.WriteMask[1]), NO_EXTRA },
   { GL_STENCIL_BACK_REF, CONTEXT_INT(Stencil.Ref[1]), NO_EXTRA },
   { GL_STENCIL_BACK_FAIL, CONTEXT_ENUM(Stencil.FailFunc[1]), NO_EXTRA },
   { GL_STENCIL_BACK_PASS_DEPTH_FAIL, CONTEXT_ENUM(Stencil.ZFailFunc[1]), NO_EXTRA },
   { GL_STENCIL_BACK_PASS_DEPTH_PASS, CONTEXT_ENUM(Stencil.ZPassFunc[1]), NO_EXTRA },

   { GL_MAX_VERTEX_ATTRIBS_ARB,
     CONTEXT_INT(Const.VertexProgram.MaxAttribs),
     extra_ARB_vertex_program_version_es2 },

   /* OES_texture_3D */
   { GL_TEXTURE_BINDING_3D, LOC_CUSTOM, TYPE_INT, TEXTURE_3D_INDEX, NO_EXTRA },
   { GL_MAX_3D_TEXTURE_SIZE, LOC_CUSTOM, TYPE_INT,
     offsetof(struct gl_context, Const.Max3DTextureLevels), NO_EXTRA },

   /* GL_ARB_fragment_program/OES_standard_derivatives */
   { GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB,
     CONTEXT_ENUM(Hint.FragmentShaderDerivative), extra_ARB_fragment_shader },
#endif /* FEATURE_GL || FEATURE_ES2 */

#if FEATURE_ES2
   /* Enums unique to OpenGL ES 2.0 */
   { 0, 0, TYPE_API_MASK, API_OPENGLES2_BIT, NO_EXTRA },
   { GL_MAX_FRAGMENT_UNIFORM_VECTORS, LOC_CUSTOM, TYPE_INT, 0, NO_EXTRA },
   { GL_MAX_VARYING_VECTORS, CONTEXT_INT(Const.MaxVarying), NO_EXTRA },
   { GL_MAX_VERTEX_UNIFORM_VECTORS, LOC_CUSTOM, TYPE_INT, 0, NO_EXTRA },
   { GL_SHADER_COMPILER, CONST(1), NO_EXTRA },
   /* OES_get_program_binary */
   { GL_NUM_SHADER_BINARY_FORMATS, CONST(0), NO_EXTRA },
   { GL_SHADER_BINARY_FORMATS, CONST(0), NO_EXTRA },
#endif /* FEATURE_ES2 */

   /* GL_OES_EGL_image_external */
   { GL_TEXTURE_BINDING_EXTERNAL_OES, LOC_CUSTOM,
     TYPE_INT, TEXTURE_EXTERNAL_INDEX, extra_OES_EGL_image_external },
   { GL_TEXTURE_EXTERNAL_OES, LOC_CUSTOM,
     TYPE_BOOLEAN, 0, extra_OES_EGL_image_external },

#if FEATURE_GL
   /* Remaining enums are only in OpenGL */
   { 0, 0, TYPE_API_MASK, API_OPENGL_BIT, NO_EXTRA },
   { GL_ACCUM_RED_BITS, BUFFER_INT(Visual.accumRedBits), NO_EXTRA },
   { GL_ACCUM_GREEN_BITS, BUFFER_INT(Visual.accumGreenBits), NO_EXTRA },
   { GL_ACCUM_BLUE_BITS, BUFFER_INT(Visual.accumBlueBits), NO_EXTRA },
   { GL_ACCUM_ALPHA_BITS, BUFFER_INT(Visual.accumAlphaBits), NO_EXTRA },
   { GL_ACCUM_CLEAR_VALUE, CONTEXT_FIELD(Accum.ClearColor[0], TYPE_FLOATN_4), NO_EXTRA },
   { GL_ALPHA_BIAS, CONTEXT_FLOAT(Pixel.AlphaBias), NO_EXTRA },
   { GL_ALPHA_SCALE, CONTEXT_FLOAT(Pixel.AlphaScale), NO_EXTRA },
   { GL_ATTRIB_STACK_DEPTH, CONTEXT_INT(AttribStackDepth), NO_EXTRA },
   { GL_AUTO_NORMAL, CONTEXT_BOOL(Eval.AutoNormal), NO_EXTRA },
   { GL_AUX_BUFFERS, BUFFER_INT(Visual.numAuxBuffers), NO_EXTRA },
   { GL_BLUE_BIAS, CONTEXT_FLOAT(Pixel.BlueBias), NO_EXTRA },
   { GL_BLUE_SCALE, CONTEXT_FLOAT(Pixel.BlueScale), NO_EXTRA },
   { GL_CLIENT_ATTRIB_STACK_DEPTH, CONTEXT_INT(ClientAttribStackDepth), NO_EXTRA },
   { GL_COLOR_MATERIAL_FACE, CONTEXT_ENUM(Light.ColorMaterialFace), NO_EXTRA },
   { GL_COLOR_MATERIAL_PARAMETER, CONTEXT_ENUM(Light.ColorMaterialMode), NO_EXTRA },
   { GL_CURRENT_INDEX,
     CONTEXT_FLOAT(Current.Attrib[VERT_ATTRIB_COLOR_INDEX][0]),
     extra_flush_current },
   { GL_CURRENT_RASTER_COLOR,
     CONTEXT_FIELD(Current.RasterColor[0], TYPE_FLOATN_4), NO_EXTRA },
   { GL_CURRENT_RASTER_DISTANCE, CONTEXT_FLOAT(Current.RasterDistance), NO_EXTRA },
   { GL_CURRENT_RASTER_INDEX, CONST(1), NO_EXTRA },
   { GL_CURRENT_RASTER_POSITION, CONTEXT_FLOAT4(Current.RasterPos[0]), NO_EXTRA },
   { GL_CURRENT_RASTER_SECONDARY_COLOR,
     CONTEXT_FIELD(Current.RasterSecondaryColor[0], TYPE_FLOATN_4), NO_EXTRA },
   { GL_CURRENT_RASTER_TEXTURE_COORDS, LOC_CUSTOM, TYPE_FLOAT_4, 0,
     extra_valid_texture_unit },
   { GL_CURRENT_RASTER_POSITION_VALID, CONTEXT_BOOL(Current.RasterPosValid), NO_EXTRA },
   { GL_DEPTH_BIAS, CONTEXT_FLOAT(Pixel.DepthBias), NO_EXTRA },
   { GL_DEPTH_SCALE, CONTEXT_FLOAT(Pixel.DepthScale), NO_EXTRA },
   { GL_DOUBLEBUFFER, BUFFER_INT(Visual.doubleBufferMode), NO_EXTRA },
   { GL_DRAW_BUFFER, BUFFER_ENUM(ColorDrawBuffer[0]), NO_EXTRA },
   { GL_EDGE_FLAG, LOC_CUSTOM, TYPE_BOOLEAN, 0, NO_EXTRA },
   { GL_FEEDBACK_BUFFER_SIZE, CONTEXT_INT(Feedback.BufferSize), NO_EXTRA },
   { GL_FEEDBACK_BUFFER_TYPE, CONTEXT_ENUM(Feedback.Type), NO_EXTRA },
   { GL_FOG_INDEX, CONTEXT_FLOAT(Fog.Index), NO_EXTRA },
   { GL_GREEN_BIAS, CONTEXT_FLOAT(Pixel.GreenBias), NO_EXTRA },
   { GL_GREEN_SCALE, CONTEXT_FLOAT(Pixel.GreenScale), NO_EXTRA },
   { GL_INDEX_BITS, BUFFER_INT(Visual.indexBits), extra_new_buffers },
   { GL_INDEX_CLEAR_VALUE, CONTEXT_INT(Color.ClearIndex), NO_EXTRA },
   { GL_INDEX_MODE, CONST(0) , NO_EXTRA}, 
   { GL_INDEX_OFFSET, CONTEXT_INT(Pixel.IndexOffset), NO_EXTRA },
   { GL_INDEX_SHIFT, CONTEXT_INT(Pixel.IndexShift), NO_EXTRA },
   { GL_INDEX_WRITEMASK, CONTEXT_INT(Color.IndexMask), NO_EXTRA },
   { GL_LIGHT_MODEL_COLOR_CONTROL, CONTEXT_ENUM(Light.Model.ColorControl), NO_EXTRA },
   { GL_LIGHT_MODEL_LOCAL_VIEWER, CONTEXT_BOOL(Light.Model.LocalViewer), NO_EXTRA },
   { GL_LINE_STIPPLE, CONTEXT_BOOL(Line.StippleFlag), NO_EXTRA },
   { GL_LINE_STIPPLE_PATTERN, LOC_CUSTOM, TYPE_INT, 0, NO_EXTRA },
   { GL_LINE_STIPPLE_REPEAT, CONTEXT_INT(Line.StippleFactor), NO_EXTRA },
   { GL_LINE_WIDTH_GRANULARITY, CONTEXT_FLOAT(Const.LineWidthGranularity), NO_EXTRA },
   { GL_LIST_BASE, CONTEXT_INT(List.ListBase), NO_EXTRA },
   { GL_LIST_INDEX, LOC_CUSTOM, TYPE_INT, 0, NO_EXTRA },
   { GL_LIST_MODE, LOC_CUSTOM, TYPE_ENUM, 0, NO_EXTRA },
   { GL_INDEX_LOGIC_OP, CONTEXT_BOOL(Color.IndexLogicOpEnabled), NO_EXTRA },
   { GL_MAP1_COLOR_4, CONTEXT_BOOL(Eval.Map1Color4), NO_EXTRA },
   { GL_MAP1_GRID_DOMAIN, CONTEXT_FLOAT2(Eval.MapGrid1u1), NO_EXTRA },
   { GL_MAP1_GRID_SEGMENTS, CONTEXT_INT(Eval.MapGrid1un), NO_EXTRA },
   { GL_MAP1_INDEX, CONTEXT_BOOL(Eval.Map1Index), NO_EXTRA },
   { GL_MAP1_NORMAL, CONTEXT_BOOL(Eval.Map1Normal), NO_EXTRA },
   { GL_MAP1_TEXTURE_COORD_1, CONTEXT_BOOL(Eval.Map1TextureCoord1), NO_EXTRA },
   { GL_MAP1_TEXTURE_COORD_2, CONTEXT_BOOL(Eval.Map1TextureCoord2), NO_EXTRA },
   { GL_MAP1_TEXTURE_COORD_3, CONTEXT_BOOL(Eval.Map1TextureCoord3), NO_EXTRA },
   { GL_MAP1_TEXTURE_COORD_4, CONTEXT_BOOL(Eval.Map1TextureCoord4), NO_EXTRA },
   { GL_MAP1_VERTEX_3, CONTEXT_BOOL(Eval.Map1Vertex3), NO_EXTRA },
   { GL_MAP1_VERTEX_4, CONTEXT_BOOL(Eval.Map1Vertex4), NO_EXTRA },
   { GL_MAP2_COLOR_4, CONTEXT_BOOL(Eval.Map2Color4), NO_EXTRA },
   { GL_MAP2_GRID_DOMAIN, LOC_CUSTOM, TYPE_FLOAT_4, 0, NO_EXTRA },
   { GL_MAP2_GRID_SEGMENTS, CONTEXT_INT2(Eval.MapGrid2un), NO_EXTRA },
   { GL_MAP2_INDEX, CONTEXT_BOOL(Eval.Map2Index), NO_EXTRA },
   { GL_MAP2_NORMAL, CONTEXT_BOOL(Eval.Map2Normal), NO_EXTRA },
   { GL_MAP2_TEXTURE_COORD_1, CONTEXT_BOOL(Eval.Map2TextureCoord1), NO_EXTRA },
   { GL_MAP2_TEXTURE_COORD_2, CONTEXT_BOOL(Eval.Map2TextureCoord2), NO_EXTRA },
   { GL_MAP2_TEXTURE_COORD_3, CONTEXT_BOOL(Eval.Map2TextureCoord3), NO_EXTRA },
   { GL_MAP2_TEXTURE_COORD_4, CONTEXT_BOOL(Eval.Map2TextureCoord4), NO_EXTRA },
   { GL_MAP2_VERTEX_3, CONTEXT_BOOL(Eval.Map2Vertex3), NO_EXTRA },
   { GL_MAP2_VERTEX_4, CONTEXT_BOOL(Eval.Map2Vertex4), NO_EXTRA },
   { GL_MAP_COLOR, CONTEXT_BOOL(Pixel.MapColorFlag), NO_EXTRA },
   { GL_MAP_STENCIL, CONTEXT_BOOL(Pixel.MapStencilFlag), NO_EXTRA },
   { GL_MAX_ATTRIB_STACK_DEPTH, CONST(MAX_ATTRIB_STACK_DEPTH), NO_EXTRA },
   { GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, CONST(MAX_CLIENT_ATTRIB_STACK_DEPTH), NO_EXTRA },

   { GL_MAX_EVAL_ORDER, CONST(MAX_EVAL_ORDER), NO_EXTRA },
   { GL_MAX_LIST_NESTING, CONST(MAX_LIST_NESTING), NO_EXTRA },
   { GL_MAX_NAME_STACK_DEPTH, CONST(MAX_NAME_STACK_DEPTH), NO_EXTRA },
   { GL_MAX_PIXEL_MAP_TABLE, CONST(MAX_PIXEL_MAP_TABLE), NO_EXTRA },
   { GL_NAME_STACK_DEPTH, CONTEXT_INT(Select.NameStackDepth), NO_EXTRA },
   { GL_PACK_LSB_FIRST, CONTEXT_BOOL(Pack.LsbFirst), NO_EXTRA },
   { GL_PACK_ROW_LENGTH, CONTEXT_INT(Pack.RowLength), NO_EXTRA },
   { GL_PACK_SKIP_PIXELS, CONTEXT_INT(Pack.SkipPixels), NO_EXTRA },
   { GL_PACK_SKIP_ROWS, CONTEXT_INT(Pack.SkipRows), NO_EXTRA },
   { GL_PACK_SWAP_BYTES, CONTEXT_BOOL(Pack.SwapBytes), NO_EXTRA },
   { GL_PACK_IMAGE_HEIGHT_EXT, CONTEXT_INT(Pack.ImageHeight), NO_EXTRA },
   { GL_PACK_INVERT_MESA, CONTEXT_BOOL(Pack.Invert), NO_EXTRA },
   { GL_PIXEL_MAP_A_TO_A_SIZE, CONTEXT_INT(PixelMaps.AtoA.Size), NO_EXTRA },
   { GL_PIXEL_MAP_B_TO_B_SIZE, CONTEXT_INT(PixelMaps.BtoB.Size), NO_EXTRA },
   { GL_PIXEL_MAP_G_TO_G_SIZE, CONTEXT_INT(PixelMaps.GtoG.Size), NO_EXTRA },
   { GL_PIXEL_MAP_I_TO_A_SIZE, CONTEXT_INT(PixelMaps.ItoA.Size), NO_EXTRA },
   { GL_PIXEL_MAP_I_TO_B_SIZE, CONTEXT_INT(PixelMaps.ItoB.Size), NO_EXTRA },
   { GL_PIXEL_MAP_I_TO_G_SIZE, CONTEXT_INT(PixelMaps.ItoG.Size), NO_EXTRA },
   { GL_PIXEL_MAP_I_TO_I_SIZE, CONTEXT_INT(PixelMaps.ItoI.Size), NO_EXTRA },
   { GL_PIXEL_MAP_I_TO_R_SIZE, CONTEXT_INT(PixelMaps.ItoR.Size), NO_EXTRA },
   { GL_PIXEL_MAP_R_TO_R_SIZE, CONTEXT_INT(PixelMaps.RtoR.Size), NO_EXTRA },
   { GL_PIXEL_MAP_S_TO_S_SIZE, CONTEXT_INT(PixelMaps.StoS.Size), NO_EXTRA },
   { GL_POINT_SIZE_GRANULARITY, CONTEXT_FLOAT(Const.PointSizeGranularity), NO_EXTRA },
   { GL_POLYGON_MODE, CONTEXT_ENUM2(Polygon.FrontMode), NO_EXTRA },
   { GL_POLYGON_OFFSET_BIAS_EXT, CONTEXT_FLOAT(Polygon.OffsetUnits), NO_EXTRA },
   { GL_POLYGON_OFFSET_POINT, CONTEXT_BOOL(Polygon.OffsetPoint), NO_EXTRA },
   { GL_POLYGON_OFFSET_LINE, CONTEXT_BOOL(Polygon.OffsetLine), NO_EXTRA },
   { GL_POLYGON_SMOOTH, CONTEXT_BOOL(Polygon.SmoothFlag), NO_EXTRA },
   { GL_POLYGON_SMOOTH_HINT, CONTEXT_ENUM(Hint.PolygonSmooth), NO_EXTRA },
   { GL_POLYGON_STIPPLE, CONTEXT_BOOL(Polygon.StippleFlag), NO_EXTRA },
   { GL_READ_BUFFER, LOC_CUSTOM, TYPE_ENUM, NO_OFFSET, NO_EXTRA },
   { GL_RED_BIAS, CONTEXT_FLOAT(Pixel.RedBias), NO_EXTRA },
   { GL_RED_SCALE, CONTEXT_FLOAT(Pixel.RedScale), NO_EXTRA },
   { GL_RENDER_MODE, CONTEXT_ENUM(RenderMode), NO_EXTRA },
   { GL_RGBA_MODE, CONST(1), NO_EXTRA },
   { GL_SELECTION_BUFFER_SIZE, CONTEXT_INT(Select.BufferSize), NO_EXTRA },

   { GL_STEREO, BUFFER_INT(Visual.stereoMode), NO_EXTRA },

   { GL_TEXTURE_1D, LOC_CUSTOM, TYPE_BOOLEAN, NO_OFFSET, NO_EXTRA },
   { GL_TEXTURE_3D, LOC_CUSTOM, TYPE_BOOLEAN, NO_OFFSET, NO_EXTRA },
   { GL_TEXTURE_1D_ARRAY_EXT, LOC_CUSTOM, TYPE_BOOLEAN, NO_OFFSET, NO_EXTRA },
   { GL_TEXTURE_2D_ARRAY_EXT, LOC_CUSTOM, TYPE_BOOLEAN, NO_OFFSET, NO_EXTRA },

   { GL_TEXTURE_BINDING_1D, LOC_CUSTOM, TYPE_INT, TEXTURE_1D_INDEX, NO_EXTRA },
   { GL_TEXTURE_BINDING_1D_ARRAY, LOC_CUSTOM, TYPE_INT,
     TEXTURE_1D_ARRAY_INDEX, extra_MESA_texture_array },
   { GL_TEXTURE_BINDING_2D_ARRAY, LOC_CUSTOM, TYPE_INT,
     TEXTURE_1D_ARRAY_INDEX, extra_MESA_texture_array },
   { GL_MAX_ARRAY_TEXTURE_LAYERS_EXT,
     CONTEXT_INT(Const.MaxArrayTextureLayers), extra_MESA_texture_array },

   { GL_TEXTURE_GEN_S, LOC_TEXUNIT, TYPE_BIT_0,
     offsetof(struct gl_texture_unit, TexGenEnabled), NO_EXTRA },
   { GL_TEXTURE_GEN_T, LOC_TEXUNIT, TYPE_BIT_1,
     offsetof(struct gl_texture_unit, TexGenEnabled), NO_EXTRA },
   { GL_TEXTURE_GEN_R, LOC_TEXUNIT, TYPE_BIT_2,
     offsetof(struct gl_texture_unit, TexGenEnabled), NO_EXTRA },
   { GL_TEXTURE_GEN_Q, LOC_TEXUNIT, TYPE_BIT_3,
     offsetof(struct gl_texture_unit, TexGenEnabled), NO_EXTRA },
   { GL_UNPACK_LSB_FIRST, CONTEXT_BOOL(Unpack.LsbFirst), NO_EXTRA },
   { GL_UNPACK_ROW_LENGTH, CONTEXT_INT(Unpack.RowLength), NO_EXTRA },
   { GL_UNPACK_SKIP_PIXELS, CONTEXT_INT(Unpack.SkipPixels), NO_EXTRA },
   { GL_UNPACK_SKIP_ROWS, CONTEXT_INT(Unpack.SkipRows), NO_EXTRA },
   { GL_UNPACK_SWAP_BYTES, CONTEXT_BOOL(Unpack.SwapBytes), NO_EXTRA },
   { GL_UNPACK_SKIP_IMAGES_EXT, CONTEXT_INT(Unpack.SkipImages), NO_EXTRA },
   { GL_UNPACK_IMAGE_HEIGHT_EXT, CONTEXT_INT(Unpack.ImageHeight), NO_EXTRA },
   { GL_ZOOM_X, CONTEXT_FLOAT(Pixel.ZoomX), NO_EXTRA },
   { GL_ZOOM_Y, CONTEXT_FLOAT(Pixel.ZoomY), NO_EXTRA },

   /* Vertex arrays */
   { GL_VERTEX_ARRAY_COUNT_EXT, CONST(0), NO_EXTRA },
   { GL_NORMAL_ARRAY_COUNT_EXT, CONST(0), NO_EXTRA },
   { GL_COLOR_ARRAY_COUNT_EXT, CONST(0), NO_EXTRA },
   { GL_INDEX_ARRAY, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_COLOR_INDEX].Enabled), NO_EXTRA },
   { GL_INDEX_ARRAY_TYPE, ARRAY_ENUM(VertexAttrib[VERT_ATTRIB_COLOR_INDEX].Type), NO_EXTRA },
   { GL_INDEX_ARRAY_STRIDE, ARRAY_INT(VertexAttrib[VERT_ATTRIB_COLOR_INDEX].Stride), NO_EXTRA },
   { GL_INDEX_ARRAY_COUNT_EXT, CONST(0), NO_EXTRA },
   { GL_TEXTURE_COORD_ARRAY_COUNT_EXT, CONST(0), NO_EXTRA },
   { GL_EDGE_FLAG_ARRAY, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_EDGEFLAG].Enabled), NO_EXTRA },
   { GL_EDGE_FLAG_ARRAY_STRIDE, ARRAY_INT(VertexAttrib[VERT_ATTRIB_EDGEFLAG].Stride), NO_EXTRA },
   { GL_EDGE_FLAG_ARRAY_COUNT_EXT, CONST(0), NO_EXTRA },

   /* GL_ARB_texture_compression */
   { GL_TEXTURE_COMPRESSION_HINT_ARB, CONTEXT_INT(Hint.TextureCompression), NO_EXTRA },

   /* GL_EXT_compiled_vertex_array */
   { GL_ARRAY_ELEMENT_LOCK_FIRST_EXT, CONTEXT_INT(Array.LockFirst),
     extra_EXT_compiled_vertex_array },
   { GL_ARRAY_ELEMENT_LOCK_COUNT_EXT, CONTEXT_INT(Array.LockCount),
     extra_EXT_compiled_vertex_array },

   /* GL_ARB_transpose_matrix */
   { GL_TRANSPOSE_MODELVIEW_MATRIX_ARB,
     CONTEXT_MATRIX_T(ModelviewMatrixStack), NO_EXTRA },
   { GL_TRANSPOSE_PROJECTION_MATRIX_ARB,
     CONTEXT_MATRIX_T(ProjectionMatrixStack.Top), NO_EXTRA },
   { GL_TRANSPOSE_TEXTURE_MATRIX_ARB, CONTEXT_MATRIX_T(TextureMatrixStack), NO_EXTRA },

   /* GL_EXT_secondary_color */
   { GL_COLOR_SUM_EXT, CONTEXT_BOOL(Fog.ColorSumEnabled),
     extra_EXT_secondary_color_ARB_vertex_program },
   { GL_CURRENT_SECONDARY_COLOR_EXT,
     CONTEXT_FIELD(Current.Attrib[VERT_ATTRIB_COLOR1][0], TYPE_FLOATN_4),
     extra_EXT_secondary_color_flush_current },
   { GL_SECONDARY_COLOR_ARRAY_EXT, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_COLOR1].Enabled),
     extra_EXT_secondary_color },
   { GL_SECONDARY_COLOR_ARRAY_TYPE_EXT, ARRAY_ENUM(VertexAttrib[VERT_ATTRIB_COLOR1].Type),
     extra_EXT_secondary_color },
   { GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT, ARRAY_INT(VertexAttrib[VERT_ATTRIB_COLOR1].Stride),
     extra_EXT_secondary_color },
   { GL_SECONDARY_COLOR_ARRAY_SIZE_EXT, ARRAY_INT(VertexAttrib[VERT_ATTRIB_COLOR1].Size),
     extra_EXT_secondary_color },

   /* GL_EXT_fog_coord */
   { GL_CURRENT_FOG_COORDINATE_EXT,
     CONTEXT_FLOAT(Current.Attrib[VERT_ATTRIB_FOG][0]),
     extra_EXT_fog_coord_flush_current },
   { GL_FOG_COORDINATE_ARRAY_EXT, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_FOG].Enabled),
     extra_EXT_fog_coord },
   { GL_FOG_COORDINATE_ARRAY_TYPE_EXT, ARRAY_ENUM(VertexAttrib[VERT_ATTRIB_FOG].Type),
     extra_EXT_fog_coord },
   { GL_FOG_COORDINATE_ARRAY_STRIDE_EXT, ARRAY_INT(VertexAttrib[VERT_ATTRIB_FOG].Stride),
     extra_EXT_fog_coord },
   { GL_FOG_COORDINATE_SOURCE_EXT, CONTEXT_ENUM(Fog.FogCoordinateSource),
     extra_EXT_fog_coord },

   /* GL_NV_fog_distance */
   { GL_FOG_DISTANCE_MODE_NV, CONTEXT_ENUM(Fog.FogDistanceMode),
     extra_NV_fog_distance },

   /* GL_IBM_rasterpos_clip */
   { GL_RASTER_POSITION_UNCLIPPED_IBM,
     CONTEXT_BOOL(Transform.RasterPositionUnclipped),
     extra_IBM_rasterpos_clip },

   /* GL_NV_point_sprite */
   { GL_POINT_SPRITE_R_MODE_NV,
     CONTEXT_ENUM(Point.SpriteRMode), extra_NV_point_sprite },
   { GL_POINT_SPRITE_COORD_ORIGIN, CONTEXT_ENUM(Point.SpriteOrigin),
     extra_NV_point_sprite_ARB_point_sprite },

   /* GL_NV_vertex_program */
   { GL_VERTEX_PROGRAM_BINDING_NV, LOC_CUSTOM, TYPE_INT, 0,
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY0_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(0)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY1_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(1)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY2_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(2)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY3_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(3)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY4_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(4)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY5_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(5)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY6_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(6)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY7_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(7)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY8_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(8)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY9_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(9)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY10_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(10)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY11_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(11)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY12_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(12)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY13_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(13)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY14_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(14)].Enabled),
     extra_NV_vertex_program },
   { GL_VERTEX_ATTRIB_ARRAY15_NV, ARRAY_BOOL(VertexAttrib[VERT_ATTRIB_GENERIC(15)].Enabled),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB0_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[0]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB1_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[1]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB2_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[2]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB3_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[3]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB4_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[4]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB5_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[5]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB6_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[6]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB7_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[7]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB8_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[8]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB9_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[9]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB10_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[10]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB11_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[11]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB12_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[12]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB13_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[13]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB14_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[14]),
     extra_NV_vertex_program },
   { GL_MAP1_VERTEX_ATTRIB15_4_NV, CONTEXT_BOOL(Eval.Map1Attrib[15]),
     extra_NV_vertex_program },

   /* GL_NV_fragment_program */
   { GL_FRAGMENT_PROGRAM_NV, CONTEXT_BOOL(FragmentProgram.Enabled),
     extra_NV_fragment_program },
   { GL_FRAGMENT_PROGRAM_BINDING_NV, LOC_CUSTOM, TYPE_INT, 0,
     extra_NV_fragment_program },
   { GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV,
     CONST(MAX_NV_FRAGMENT_PROGRAM_PARAMS),
     extra_NV_fragment_program },

   /* GL_NV_texture_rectangle */
   { GL_TEXTURE_RECTANGLE_NV,
     LOC_CUSTOM, TYPE_BOOLEAN, 0, extra_NV_texture_rectangle },
   { GL_TEXTURE_BINDING_RECTANGLE_NV,
     LOC_CUSTOM, TYPE_INT, TEXTURE_RECT_INDEX, extra_NV_texture_rectangle },
   { GL_MAX_RECTANGLE_TEXTURE_SIZE_NV,
     CONTEXT_INT(Const.MaxTextureRectSize), extra_NV_texture_rectangle },

   /* GL_EXT_stencil_two_side */
   { GL_STENCIL_TEST_TWO_SIDE_EXT, CONTEXT_BOOL(Stencil.TestTwoSide),
	 extra_EXT_stencil_two_side },
   { GL_ACTIVE_STENCIL_FACE_EXT, LOC_CUSTOM, TYPE_ENUM, NO_OFFSET, NO_EXTRA },

   /* GL_NV_light_max_exponent */
   { GL_MAX_SHININESS_NV, CONTEXT_FLOAT(Const.MaxShininess),
     extra_NV_light_max_exponent },
   { GL_MAX_SPOT_EXPONENT_NV, CONTEXT_FLOAT(Const.MaxSpotExponent),
     extra_NV_light_max_exponent },
     
   /* GL_NV_primitive_restart */
   { GL_PRIMITIVE_RESTART_NV, CONTEXT_BOOL(Array.PrimitiveRestart),
     extra_NV_primitive_restart },
   { GL_PRIMITIVE_RESTART_INDEX_NV, CONTEXT_INT(Array.RestartIndex),
     extra_NV_primitive_restart },
 
   /* GL_ARB_vertex_buffer_object */
   { GL_INDEX_ARRAY_BUFFER_BINDING_ARB, LOC_CUSTOM, TYPE_INT,
     offsetof(struct gl_array_object, VertexAttrib[VERT_ATTRIB_COLOR_INDEX].BufferObj), NO_EXTRA },
   { GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB, LOC_CUSTOM, TYPE_INT,
     offsetof(struct gl_array_object, VertexAttrib[VERT_ATTRIB_EDGEFLAG].BufferObj), NO_EXTRA },
   { GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB, LOC_CUSTOM, TYPE_INT,
     offsetof(struct gl_array_object, VertexAttrib[VERT_ATTRIB_COLOR1].BufferObj), NO_EXTRA },
   { GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB, LOC_CUSTOM, TYPE_INT,
     offsetof(struct gl_array_object, VertexAttrib[VERT_ATTRIB_FOG].BufferObj), NO_EXTRA },

   /* GL_EXT_pixel_buffer_object */
   { GL_PIXEL_PACK_BUFFER_BINDING_EXT, LOC_CUSTOM, TYPE_INT, 0,
     extra_EXT_pixel_buffer_object },
   { GL_PIXEL_UNPACK_BUFFER_BINDING_EXT, LOC_CUSTOM, TYPE_INT, 0,
     extra_EXT_pixel_buffer_object },

   /* GL_ARB_vertex_program */
   { GL_VERTEX_PROGRAM_ARB, /* == GL_VERTEX_PROGRAM_NV */
     CONTEXT_BOOL(VertexProgram.Enabled),
     extra_ARB_vertex_program_NV_vertex_program },
   { GL_VERTEX_PROGRAM_POINT_SIZE_ARB, /* == GL_VERTEX_PROGRAM_POINT_SIZE_NV*/
     CONTEXT_BOOL(VertexProgram.PointSizeEnabled),
     extra_ARB_vertex_program_NV_vertex_program },
   { GL_VERTEX_PROGRAM_TWO_SIDE_ARB, /* == GL_VERTEX_PROGRAM_TWO_SIDE_NV */
     CONTEXT_BOOL(VertexProgram.TwoSideEnabled),
     extra_ARB_vertex_program_NV_vertex_program },
   { GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB, /* == GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV */
     CONTEXT_INT(Const.MaxProgramMatrixStackDepth),
     extra_ARB_vertex_program_ARB_fragment_program_NV_vertex_program },
   { GL_MAX_PROGRAM_MATRICES_ARB, /* == GL_MAX_TRACK_MATRICES_NV */
     CONTEXT_INT(Const.MaxProgramMatrices),
     extra_ARB_vertex_program_ARB_fragment_program_NV_vertex_program },
   { GL_CURRENT_MATRIX_STACK_DEPTH_ARB, /* == GL_CURRENT_MATRIX_STACK_DEPTH_NV */
     LOC_CUSTOM, TYPE_INT, 0,
     extra_ARB_vertex_program_ARB_fragment_program_NV_vertex_program },

   { GL_CURRENT_MATRIX_ARB, /* == GL_CURRENT_MATRIX_NV */
     LOC_CUSTOM, TYPE_MATRIX, 0,
     extra_ARB_vertex_program_ARB_fragment_program_NV_vertex_program },
   { GL_TRANSPOSE_CURRENT_MATRIX_ARB, /* == GL_CURRENT_MATRIX_NV */
     LOC_CUSTOM, TYPE_MATRIX, 0,
     extra_ARB_vertex_program_ARB_fragment_program },

   { GL_PROGRAM_ERROR_POSITION_ARB, /* == GL_PROGRAM_ERROR_POSITION_NV */
     CONTEXT_INT(Program.ErrorPos),
     extra_NV_vertex_program_ARB_vertex_program_ARB_fragment_program_NV_vertex_program },

   /* GL_ARB_fragment_program */
   { GL_FRAGMENT_PROGRAM_ARB, CONTEXT_BOOL(FragmentProgram.Enabled),
     extra_ARB_fragment_program },

   /* GL_EXT_depth_bounds_test */
   { GL_DEPTH_BOUNDS_TEST_EXT, CONTEXT_BOOL(Depth.BoundsTest),
     extra_EXT_depth_bounds_test },
   { GL_DEPTH_BOUNDS_EXT, CONTEXT_FLOAT2(Depth.BoundsMin),
     extra_EXT_depth_bounds_test },

   /* GL_ARB_depth_clamp*/
   { GL_DEPTH_CLAMP, CONTEXT_BOOL(Transform.DepthClamp),
     extra_ARB_depth_clamp },

   /* GL_ATI_fragment_shader */
   { GL_NUM_FRAGMENT_REGISTERS_ATI, CONST(6), extra_ATI_fragment_shader },
   { GL_NUM_FRAGMENT_CONSTANTS_ATI, CONST(8), extra_ATI_fragment_shader },
   { GL_NUM_PASSES_ATI, CONST(2), extra_ATI_fragment_shader },
   { GL_NUM_INSTRUCTIONS_PER_PASS_ATI, CONST(8), extra_ATI_fragment_shader },
   { GL_NUM_INSTRUCTIONS_TOTAL_ATI, CONST(16), extra_ATI_fragment_shader },
   { GL_COLOR_ALPHA_PAIRING_ATI, CONST(GL_TRUE), extra_ATI_fragment_shader },
   { GL_NUM_LOOPBACK_COMPONENTS_ATI, CONST(3), extra_ATI_fragment_shader },
   { GL_NUM_INPUT_INTERPOLATOR_COMPONENTS_ATI,
     CONST(3), extra_ATI_fragment_shader },

   /* GL_EXT_framebuffer_blit
    * NOTE: GL_DRAW_FRAMEBUFFER_BINDING_EXT == GL_FRAMEBUFFER_BINDING_EXT */
   { GL_READ_FRAMEBUFFER_BINDING_EXT, LOC_CUSTOM, TYPE_INT, 0,
     extra_EXT_framebuffer_blit },

   /* GL_EXT_provoking_vertex */
   { GL_PROVOKING_VERTEX_EXT,
     CONTEXT_ENUM(Light.ProvokingVertex), extra_EXT_provoking_vertex },
   { GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION_EXT,
     CONTEXT_BOOL(Const.QuadsFollowProvokingVertexConvention),
     extra_EXT_provoking_vertex },

   /* GL_ARB_framebuffer_object */
   { GL_MAX_SAMPLES, CONTEXT_INT(Const.MaxSamples),
     extra_ARB_framebuffer_object_EXT_framebuffer_multisample },

   /* GL_APPLE_vertex_array_object */
   { GL_VERTEX_ARRAY_BINDING_APPLE, ARRAY_INT(Name),
     extra_APPLE_vertex_array_object },

   /* GL_ARB_seamless_cube_map */
   { GL_TEXTURE_CUBE_MAP_SEAMLESS,
     CONTEXT_BOOL(Texture.CubeMapSeamless), extra_ARB_seamless_cube_map },

   /* GL_ARB_sync */
   { GL_MAX_SERVER_WAIT_TIMEOUT,
     CONTEXT_INT64(Const.MaxServerWaitTimeout), extra_ARB_sync },

   /* GL_EXT_texture_integer */
   { GL_RGBA_INTEGER_MODE_EXT, BUFFER_BOOL(_IntegerColor),
     extra_EXT_texture_integer },

   /* GL_EXT_transform_feedback */
   { GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, LOC_CUSTOM, TYPE_INT, 0,
     extra_EXT_transform_feedback },
   { GL_RASTERIZER_DISCARD, CONTEXT_BOOL(RasterDiscard),
     extra_EXT_transform_feedback },
   { GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS,
     CONTEXT_INT(Const.MaxTransformFeedbackInterleavedComponents),
     extra_EXT_transform_feedback },
   { GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS,
     CONTEXT_INT(Const.MaxTransformFeedbackSeparateAttribs),
     extra_EXT_transform_feedback },
   { GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS,
     CONTEXT_INT(Const.MaxTransformFeedbackSeparateComponents),
     extra_EXT_transform_feedback },

   /* GL_ARB_transform_feedback2 */
   { GL_TRANSFORM_FEEDBACK_BUFFER_PAUSED, LOC_CUSTOM, TYPE_BOOLEAN, 0,
     extra_ARB_transform_feedback2 },
   { GL_TRANSFORM_FEEDBACK_BUFFER_ACTIVE, LOC_CUSTOM, TYPE_BOOLEAN, 0,
     extra_ARB_transform_feedback2 },
   { GL_TRANSFORM_FEEDBACK_BINDING, LOC_CUSTOM, TYPE_INT, 0,
     extra_ARB_transform_feedback2 },

   /* GL_ARB_geometry_shader4 */
   { GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_ARB,
     CONTEXT_INT(Const.MaxGeometryTextureImageUnits),
     extra_ARB_geometry_shader4 },
   { GL_MAX_GEOMETRY_OUTPUT_VERTICES_ARB,
     CONTEXT_INT(Const.MaxGeometryOutputVertices),
     extra_ARB_geometry_shader4 },
   { GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_ARB,
     CONTEXT_INT(Const.MaxGeometryTotalOutputComponents),
     extra_ARB_geometry_shader4 },
   { GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_ARB,
     CONTEXT_INT(Const.GeometryProgram.MaxUniformComponents),
     extra_ARB_geometry_shader4 },
   { GL_MAX_GEOMETRY_VARYING_COMPONENTS_ARB,
     CONTEXT_INT(Const.MaxGeometryVaryingComponents),
     extra_ARB_geometry_shader4 },
   { GL_MAX_VERTEX_VARYING_COMPONENTS_ARB,
     CONTEXT_INT(Const.MaxVertexVaryingComponents),
     extra_ARB_geometry_shader4 },

   /* GL_ARB_color_buffer_float */
   { GL_RGBA_FLOAT_MODE_ARB, BUFFER_FIELD(Visual.floatMode, TYPE_BOOLEAN), 0 },

   /* GL_EXT_gpu_shader4 / GLSL 1.30 */
   { GL_MIN_PROGRAM_TEXEL_OFFSET,
     CONTEXT_INT(Const.MinProgramTexelOffset),
     extra_GLSL_130 },
   { GL_MAX_PROGRAM_TEXEL_OFFSET,
     CONTEXT_INT(Const.MaxProgramTexelOffset),
     extra_GLSL_130 },

   /* GL_ARB_texture_buffer_object */
   { GL_MAX_TEXTURE_BUFFER_SIZE_ARB, CONTEXT_INT(Const.MaxTextureBufferSize),
     extra_ARB_texture_buffer_object },
   { GL_TEXTURE_BINDING_BUFFER_ARB, LOC_CUSTOM, TYPE_INT, 0,
     extra_ARB_texture_buffer_object },
   { GL_TEXTURE_BUFFER_DATA_STORE_BINDING_ARB, LOC_CUSTOM, TYPE_INT,
     TEXTURE_BUFFER_INDEX, extra_ARB_texture_buffer_object },
   { GL_TEXTURE_BUFFER_FORMAT_ARB, LOC_CUSTOM, TYPE_INT, 0,
     extra_ARB_texture_buffer_object },
   { GL_TEXTURE_BUFFER_ARB, LOC_CUSTOM, TYPE_INT, 0,
     extra_ARB_texture_buffer_object },

   /* GL_ARB_sampler_objects / GL 3.3 */
   { GL_SAMPLER_BINDING,
     LOC_CUSTOM, TYPE_INT, GL_SAMPLER_BINDING, extra_ARB_sampler_objects },

   /* GL 3.0 */
   { GL_NUM_EXTENSIONS, LOC_CUSTOM, TYPE_INT, 0, extra_version_30 },
   { GL_MAJOR_VERSION, CONTEXT_INT(VersionMajor), extra_version_30 },
   { GL_MINOR_VERSION, CONTEXT_INT(VersionMinor), extra_version_30  },
   { GL_CONTEXT_FLAGS, CONTEXT_INT(Const.ContextFlags), extra_version_30  },

   /* GL3.0 / GL_EXT_framebuffer_sRGB */
   { GL_FRAMEBUFFER_SRGB_EXT, CONTEXT_BOOL(Color.sRGBEnabled), extra_EXT_framebuffer_sRGB },
   { GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, BUFFER_INT(Visual.sRGBCapable), extra_EXT_framebuffer_sRGB },

   /* GL 3.1 */
   /* NOTE: different enum values for GL_PRIMITIVE_RESTART_NV
    * vs. GL_PRIMITIVE_RESTART!
    */
   { GL_PRIMITIVE_RESTART, CONTEXT_BOOL(Array.PrimitiveRestart),
     extra_version_31 },
   { GL_PRIMITIVE_RESTART_INDEX, CONTEXT_INT(Array.RestartIndex),
     extra_version_31 },
 

   /* GL 3.2 */
   { GL_CONTEXT_PROFILE_MASK, CONTEXT_INT(Const.ProfileMask),
     extra_version_32 },

   /* GL_ARB_robustness */
   { GL_RESET_NOTIFICATION_STRATEGY_ARB, CONTEXT_ENUM(Const.ResetStrategy), NO_EXTRA },
#endif /* FEATURE_GL */
};

/* All we need now is a way to look up the value struct from the enum.
 * The code generated by gcc for the old generated big switch
 * statement is a big, balanced, open coded if/else tree, essentially
 * an unrolled binary search.  It would be natural to sort the new
 * enum table and use bsearch(), but we will use a read-only hash
 * table instead.  bsearch() has a nice guaranteed worst case
 * performance, but we're also guaranteed to hit that worst case
 * (log2(n) iterations) for about half the enums.  Instead, using an
 * open addressing hash table, we can find the enum on the first try
 * for 80% of the enums, 1 collision for 10% and never more than 5
 * collisions for any enum (typical numbers).  And the code is very
 * simple, even though it feels a little magic. */

static unsigned short table[1024];
static const int prime_factor = 89, prime_step = 281;

#ifdef GET_DEBUG
static void
print_table_stats(void)
{
   int i, j, collisions[11], count, hash, mask;
   const struct value_desc *d;

   count = 0;
   mask = Elements(table) - 1;
   memset(collisions, 0, sizeof collisions);

   for (i = 0; i < Elements(table); i++) {
      if (!table[i])
	 continue;
      count++;
      d = &values[table[i]];
      hash = (d->pname * prime_factor);
      j = 0;
      while (1) {
	 if (values[table[hash & mask]].pname == d->pname)
	    break;
	 hash += prime_step;
	 j++;
      }

      if (j < 10)
	 collisions[j]++;
      else
	 collisions[10]++;
   }

   printf("number of enums: %d (total %d)\n", count, Elements(values));
   for (i = 0; i < Elements(collisions) - 1; i++)
      if (collisions[i] > 0)
	 printf("  %d enums with %d %scollisions\n",
		collisions[i], i, i == 10 ? "or more " : "");
}
#endif

/**
 * Initialize the enum hash for a given API 
 *
 * This is called from one_time_init() to insert the enum values that
 * are valid for the API in question into the enum hash table.
 *
 * \param the current context, for determining the API in question
 */
void _mesa_init_get_hash(struct gl_context *ctx)
{
   int i, hash, index, mask;
   int api_mask = 0, api_bit;

   mask = Elements(table) - 1;
   api_bit = 1 << ctx->API;

   for (i = 0; i < Elements(values); i++) {
      if (values[i].type == TYPE_API_MASK) {
	 api_mask = values[i].offset;
	 continue;
      }
      if (!(api_mask & api_bit))
	 continue;

      hash = (values[i].pname * prime_factor) & mask;
      while (1) {
	 index = hash & mask;
	 if (!table[index]) {
	    table[index] = i;
	    break;
	 }
	 hash += prime_step;
      }
   }

#ifdef GET_DEBUG
   print_table_stats();
#endif
}

/**
 * Handle irregular enums
 *
 * Some values don't conform to the "well-known type at context
 * pointer + offset" pattern, so we have this function to catch all
 * the corner cases.  Typically, it's a computed value or a one-off
 * pointer to a custom struct or something.
 *
 * In this case we can't return a pointer to the value, so we'll have
 * to use the temporary variable 'v' declared back in the calling
 * glGet*v() function to store the result.
 *
 * \param ctx the current context
 * \param d the struct value_desc that describes the enum
 * \param v pointer to the tmp declared in the calling glGet*v() function
 */
static void
find_custom_value(struct gl_context *ctx, const struct value_desc *d, union value *v)
{
   struct gl_buffer_object **buffer_obj;
   struct gl_client_array *array;
   GLuint unit, *p;

   switch (d->pname) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_TEXTURE_CUBE_MAP_ARB:
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_TEXTURE_EXTERNAL_OES:
      v->value_bool = _mesa_IsEnabled(d->pname);
      break;

   case GL_LINE_STIPPLE_PATTERN:
      /* This is the only GLushort, special case it here by promoting
       * to an int rather than introducing a new type. */
      v->value_int = ctx->Line.StipplePattern;
      break;

   case GL_CURRENT_RASTER_TEXTURE_COORDS:
      unit = ctx->Texture.CurrentUnit;
      v->value_float_4[0] = ctx->Current.RasterTexCoords[unit][0];
      v->value_float_4[1] = ctx->Current.RasterTexCoords[unit][1];
      v->value_float_4[2] = ctx->Current.RasterTexCoords[unit][2];
      v->value_float_4[3] = ctx->Current.RasterTexCoords[unit][3];
      break;

   case GL_CURRENT_TEXTURE_COORDS:
      unit = ctx->Texture.CurrentUnit;
      v->value_float_4[0] = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][0];
      v->value_float_4[1] = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][1];
      v->value_float_4[2] = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][2];
      v->value_float_4[3] = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][3];
      break;

   case GL_COLOR_WRITEMASK:
      v->value_int_4[0] = ctx->Color.ColorMask[0][RCOMP] ? 1 : 0;
      v->value_int_4[1] = ctx->Color.ColorMask[0][GCOMP] ? 1 : 0;
      v->value_int_4[2] = ctx->Color.ColorMask[0][BCOMP] ? 1 : 0;
      v->value_int_4[3] = ctx->Color.ColorMask[0][ACOMP] ? 1 : 0;
      break;

   case GL_EDGE_FLAG:
      v->value_bool = ctx->Current.Attrib[VERT_ATTRIB_EDGEFLAG][0] == 1.0;
      break;

   case GL_READ_BUFFER:
      v->value_enum = ctx->ReadBuffer->ColorReadBuffer;
      break;

   case GL_MAP2_GRID_DOMAIN:
      v->value_float_4[0] = ctx->Eval.MapGrid2u1;
      v->value_float_4[1] = ctx->Eval.MapGrid2u2;
      v->value_float_4[2] = ctx->Eval.MapGrid2v1;
      v->value_float_4[3] = ctx->Eval.MapGrid2v2;
      break;

   case GL_TEXTURE_STACK_DEPTH:
      unit = ctx->Texture.CurrentUnit;
      v->value_int = ctx->TextureMatrixStack[unit].Depth + 1;
      break;
   case GL_TEXTURE_MATRIX:
      unit = ctx->Texture.CurrentUnit;
      v->value_matrix = ctx->TextureMatrixStack[unit].Top;
      break;

   case GL_TEXTURE_COORD_ARRAY:
   case GL_TEXTURE_COORD_ARRAY_SIZE:
   case GL_TEXTURE_COORD_ARRAY_TYPE:
   case GL_TEXTURE_COORD_ARRAY_STRIDE:
      array = &ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_TEX(ctx->Array.ActiveTexture)];
      v->value_int = *(GLuint *) ((char *) array + d->offset);
      break;

   case GL_ACTIVE_TEXTURE_ARB:
      v->value_int = GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit;
      break;
   case GL_CLIENT_ACTIVE_TEXTURE_ARB:
      v->value_int = GL_TEXTURE0_ARB + ctx->Array.ActiveTexture;
      break;

   case GL_MODELVIEW_STACK_DEPTH:
   case GL_PROJECTION_STACK_DEPTH:
      v->value_int = *(GLint *) ((char *) ctx + d->offset) + 1;
      break;

   case GL_MAX_TEXTURE_SIZE:
   case GL_MAX_3D_TEXTURE_SIZE:
   case GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB:
      p = (GLuint *) ((char *) ctx + d->offset);
      v->value_int = 1 << (*p - 1);
      break;

   case GL_SCISSOR_BOX:
      v->value_int_4[0] = ctx->Scissor.X;
      v->value_int_4[1] = ctx->Scissor.Y;
      v->value_int_4[2] = ctx->Scissor.Width;
      v->value_int_4[3] = ctx->Scissor.Height;
      break;

   case GL_LIST_INDEX:
      v->value_int =
	 ctx->ListState.CurrentList ? ctx->ListState.CurrentList->Name : 0;
      break;
   case GL_LIST_MODE:
      if (!ctx->CompileFlag)
	 v->value_enum = 0;
      else if (ctx->ExecuteFlag)
	 v->value_enum = GL_COMPILE_AND_EXECUTE;
      else
	 v->value_enum = GL_COMPILE;
      break;

   case GL_VIEWPORT:
      v->value_int_4[0] = ctx->Viewport.X;
      v->value_int_4[1] = ctx->Viewport.Y;
      v->value_int_4[2] = ctx->Viewport.Width;
      v->value_int_4[3] = ctx->Viewport.Height;
      break;

   case GL_ACTIVE_STENCIL_FACE_EXT:
      v->value_enum = ctx->Stencil.ActiveFace ? GL_BACK : GL_FRONT;
      break;

   case GL_STENCIL_FAIL:
      v->value_enum = ctx->Stencil.FailFunc[ctx->Stencil.ActiveFace];
      break;
   case GL_STENCIL_FUNC:
      v->value_enum = ctx->Stencil.Function[ctx->Stencil.ActiveFace];
      break;
   case GL_STENCIL_PASS_DEPTH_FAIL:
      v->value_enum = ctx->Stencil.ZFailFunc[ctx->Stencil.ActiveFace];
      break;
   case GL_STENCIL_PASS_DEPTH_PASS:
      v->value_enum = ctx->Stencil.ZPassFunc[ctx->Stencil.ActiveFace];
      break;
   case GL_STENCIL_REF:
      v->value_int = ctx->Stencil.Ref[ctx->Stencil.ActiveFace];
      break;
   case GL_STENCIL_VALUE_MASK:
      v->value_int = ctx->Stencil.ValueMask[ctx->Stencil.ActiveFace];
      break;
   case GL_STENCIL_WRITEMASK:
      v->value_int = ctx->Stencil.WriteMask[ctx->Stencil.ActiveFace];
      break;

   case GL_NUM_EXTENSIONS:
      v->value_int = _mesa_get_extension_count(ctx);
      break;

   case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
      v->value_int = _mesa_get_color_read_type(ctx);
      break;
   case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
      v->value_int = _mesa_get_color_read_format(ctx);
      break;

   case GL_CURRENT_MATRIX_STACK_DEPTH_ARB:
      v->value_int = ctx->CurrentStack->Depth + 1;
      break;
   case GL_CURRENT_MATRIX_ARB:
   case GL_TRANSPOSE_CURRENT_MATRIX_ARB:
      v->value_matrix = ctx->CurrentStack->Top;
      break;

   case GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB:
      v->value_int = _mesa_get_compressed_formats(ctx, NULL);
      break;
   case GL_COMPRESSED_TEXTURE_FORMATS_ARB:
      v->value_int_n.n = 
	 _mesa_get_compressed_formats(ctx, v->value_int_n.ints);
      ASSERT(v->value_int_n.n <= 100);
      break;

   case GL_MAX_VARYING_FLOATS_ARB:
      v->value_int = ctx->Const.MaxVarying * 4;
      break;

   /* Various object names */

   case GL_TEXTURE_BINDING_1D:
   case GL_TEXTURE_BINDING_2D:
   case GL_TEXTURE_BINDING_3D:
   case GL_TEXTURE_BINDING_1D_ARRAY_EXT:
   case GL_TEXTURE_BINDING_2D_ARRAY_EXT:
   case GL_TEXTURE_BINDING_CUBE_MAP_ARB:
   case GL_TEXTURE_BINDING_RECTANGLE_NV:
   case GL_TEXTURE_BINDING_EXTERNAL_OES:
      unit = ctx->Texture.CurrentUnit;
      v->value_int =
	 ctx->Texture.Unit[unit].CurrentTex[d->offset]->Name;
      break;

   /* GL_ARB_vertex_buffer_object */
   case GL_VERTEX_ARRAY_BUFFER_BINDING_ARB:
   case GL_NORMAL_ARRAY_BUFFER_BINDING_ARB:
   case GL_COLOR_ARRAY_BUFFER_BINDING_ARB:
   case GL_INDEX_ARRAY_BUFFER_BINDING_ARB:
   case GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB:
   case GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB:
   case GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB:
      buffer_obj = (struct gl_buffer_object **)
	 ((char *) ctx->Array.ArrayObj + d->offset);
      v->value_int = (*buffer_obj)->Name;
      break;
   case GL_ARRAY_BUFFER_BINDING_ARB:
      v->value_int = ctx->Array.ArrayBufferObj->Name;
      break;
   case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB:
      v->value_int =
	 ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_TEX(ctx->Array.ActiveTexture)].BufferObj->Name;
      break;
   case GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB:
      v->value_int = ctx->Array.ArrayObj->ElementArrayBufferObj->Name;
      break;

   /* ARB_copy_buffer */
   case GL_COPY_READ_BUFFER:
      v->value_int = ctx->CopyReadBuffer->Name;
      break;
   case GL_COPY_WRITE_BUFFER:
      v->value_int = ctx->CopyWriteBuffer->Name;
      break;

   case GL_FRAGMENT_PROGRAM_BINDING_NV:
      v->value_int = 
	 ctx->FragmentProgram.Current ? ctx->FragmentProgram.Current->Base.Id : 0;
      break;
   case GL_VERTEX_PROGRAM_BINDING_NV:
      v->value_int =
	 ctx->VertexProgram.Current ? ctx->VertexProgram.Current->Base.Id : 0;
      break;
   case GL_PIXEL_PACK_BUFFER_BINDING_EXT:
      v->value_int = ctx->Pack.BufferObj->Name;
      break;
   case GL_PIXEL_UNPACK_BUFFER_BINDING_EXT:
      v->value_int = ctx->Unpack.BufferObj->Name;
      break;
   case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
      v->value_int = ctx->TransformFeedback.CurrentBuffer->Name;
      break;
   case GL_TRANSFORM_FEEDBACK_BUFFER_PAUSED:
      v->value_int = ctx->TransformFeedback.CurrentObject->Paused;
      break;
   case GL_TRANSFORM_FEEDBACK_BUFFER_ACTIVE:
      v->value_int = ctx->TransformFeedback.CurrentObject->Active;
      break;
   case GL_TRANSFORM_FEEDBACK_BINDING:
      v->value_int = ctx->TransformFeedback.CurrentObject->Name;
      break;
   case GL_CURRENT_PROGRAM:
      v->value_int =
	 ctx->Shader.ActiveProgram ? ctx->Shader.ActiveProgram->Name : 0;
      break;
   case GL_READ_FRAMEBUFFER_BINDING_EXT:
      v->value_int = ctx->ReadBuffer->Name;
      break;
   case GL_RENDERBUFFER_BINDING_EXT:
      v->value_int =
	 ctx->CurrentRenderbuffer ? ctx->CurrentRenderbuffer->Name : 0;
      break;
   case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
      v->value_int = ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_POINT_SIZE].BufferObj->Name;
      break;

   case GL_FOG_COLOR:
      if(ctx->Color._ClampFragmentColor)
         COPY_4FV(v->value_float_4, ctx->Fog.Color);
      else
         COPY_4FV(v->value_float_4, ctx->Fog.ColorUnclamped);
      break;
   case GL_COLOR_CLEAR_VALUE:
      if(ctx->Color._ClampFragmentColor) {
         v->value_float_4[0] = CLAMP(ctx->Color.ClearColor.f[0], 0.0F, 1.0F);
         v->value_float_4[1] = CLAMP(ctx->Color.ClearColor.f[1], 0.0F, 1.0F);
         v->value_float_4[2] = CLAMP(ctx->Color.ClearColor.f[2], 0.0F, 1.0F);
         v->value_float_4[3] = CLAMP(ctx->Color.ClearColor.f[3], 0.0F, 1.0F);
      } else
         COPY_4FV(v->value_float_4, ctx->Color.ClearColor.f);
      break;
   case GL_BLEND_COLOR_EXT:
      if(ctx->Color._ClampFragmentColor)
         COPY_4FV(v->value_float_4, ctx->Color.BlendColor);
      else
         COPY_4FV(v->value_float_4, ctx->Color.BlendColorUnclamped);
      break;
   case GL_ALPHA_TEST_REF:
      if(ctx->Color._ClampFragmentColor)
         v->value_float = ctx->Color.AlphaRef;
      else
         v->value_float = ctx->Color.AlphaRefUnclamped;
      break;
   case GL_MAX_VERTEX_UNIFORM_VECTORS:
      v->value_int = ctx->Const.VertexProgram.MaxUniformComponents / 4;
      break;

   case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
      v->value_int = ctx->Const.FragmentProgram.MaxUniformComponents / 4;
      break;

   /* GL_ARB_texture_buffer_object */
   case GL_TEXTURE_BUFFER_ARB:
      v->value_int = ctx->Texture.BufferObject->Name;
      break;
   case GL_TEXTURE_BINDING_BUFFER_ARB:
      unit = ctx->Texture.CurrentUnit;
      v->value_int =
         ctx->Texture.Unit[unit].CurrentTex[TEXTURE_BUFFER_INDEX]->Name;
      break;
   case GL_TEXTURE_BUFFER_DATA_STORE_BINDING_ARB:
      {
         struct gl_buffer_object *buf =
            ctx->Texture.Unit[ctx->Texture.CurrentUnit]
            .CurrentTex[TEXTURE_BUFFER_INDEX]->BufferObject;
         v->value_int = buf ? buf->Name : 0;
      }
      break;
   case GL_TEXTURE_BUFFER_FORMAT_ARB:
      v->value_int = ctx->Texture.Unit[ctx->Texture.CurrentUnit]
         .CurrentTex[TEXTURE_BUFFER_INDEX]->BufferObjectFormat;
      break;

   /* GL_ARB_sampler_objects */
   case GL_SAMPLER_BINDING:
      {
         struct gl_sampler_object *samp =
            ctx->Texture.Unit[ctx->Texture.CurrentUnit].Sampler;
         v->value_int = samp ? samp->Name : 0;
      }
      break;
   }   
}

/**
 * Check extra constraints on a struct value_desc descriptor
 *
 * If a struct value_desc has a non-NULL extra pointer, it means that
 * there are a number of extra constraints to check or actions to
 * perform.  The extras is just an integer array where each integer
 * encode different constraints or actions.
 *
 * \param ctx current context
 * \param func name of calling glGet*v() function for error reporting
 * \param d the struct value_desc that has the extra constraints
 *
 * \return GL_FALSE if one of the constraints was not satisfied,
 *     otherwise GL_TRUE.
 */
static GLboolean
check_extra(struct gl_context *ctx, const char *func, const struct value_desc *d)
{
   const GLuint version = ctx->VersionMajor * 10 + ctx->VersionMinor;
   int total, enabled;
   const int *e;

   total = 0;
   enabled = 0;
   for (e = d->extra; *e != EXTRA_END; e++)
      switch (*e) {
      case EXTRA_VERSION_30:
	 if (version >= 30) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_VERSION_31:
	 if (version >= 31) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_VERSION_32:
	 if (version >= 32) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_NEW_FRAG_CLAMP:
         if (ctx->NewState & (_NEW_BUFFERS | _NEW_FRAG_CLAMP))
            _mesa_update_state(ctx);
         break;
      case EXTRA_VERSION_ES2:
	 if (ctx->API == API_OPENGLES2) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_NEW_BUFFERS:
	 if (ctx->NewState & _NEW_BUFFERS)
	    _mesa_update_state(ctx);
	 break;
      case EXTRA_FLUSH_CURRENT:
	 FLUSH_CURRENT(ctx, 0);
	 break;
      case EXTRA_VALID_DRAW_BUFFER:
	 if (d->pname - GL_DRAW_BUFFER0_ARB >= ctx->Const.MaxDrawBuffers) {
	    _mesa_error(ctx, GL_INVALID_OPERATION, "%s(draw buffer %u)",
			func, d->pname - GL_DRAW_BUFFER0_ARB);
	    return GL_FALSE;
	 }
	 break;
      case EXTRA_VALID_TEXTURE_UNIT:
	 if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
	    _mesa_error(ctx, GL_INVALID_OPERATION, "%s(texture %u)",
			func, ctx->Texture.CurrentUnit);
	    return GL_FALSE;
	 }
	 break;
      case EXTRA_VALID_CLIP_DISTANCE:
	 if (d->pname - GL_CLIP_DISTANCE0 >= ctx->Const.MaxClipPlanes) {
	    _mesa_error(ctx, GL_INVALID_ENUM, "%s(clip distance %u)",
			func, d->pname - GL_CLIP_DISTANCE0);
	    return GL_FALSE;
	 }
	 break;
      case EXTRA_GLSL_130:
	 if (ctx->Const.GLSLVersion >= 130) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_END:
	 break;
      default: /* *e is a offset into the extension struct */
	 total++;
	 if (*(GLboolean *) ((char *) &ctx->Extensions + *e))
	    enabled++;
	 break;
      }

   if (total > 0 && enabled == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=%s)", func,
                  _mesa_lookup_enum_by_nr(d->pname));
      return GL_FALSE;
   }

   return GL_TRUE;
}

static const struct value_desc error_value =
   { 0, 0, TYPE_INVALID, NO_OFFSET, NO_EXTRA };

/**
 * Find the struct value_desc corresponding to the enum 'pname'.
 * 
 * We hash the enum value to get an index into the 'table' array,
 * which holds the index in the 'values' array of struct value_desc.
 * Once we've found the entry, we do the extra checks, if any, then
 * look up the value and return a pointer to it.
 *
 * If the value has to be computed (for example, it's the result of a
 * function call or we need to add 1 to it), we use the tmp 'v' to
 * store the result.
 * 
 * \param func name of glGet*v() func for error reporting
 * \param pname the enum value we're looking up
 * \param p is were we return the pointer to the value
 * \param v a tmp union value variable in the calling glGet*v() function
 *
 * \return the struct value_desc corresponding to the enum or a struct
 *     value_desc of TYPE_INVALID if not found.  This lets the calling
 *     glGet*v() function jump right into a switch statement and
 *     handle errors there instead of having to check for NULL.
 */
static const struct value_desc *
find_value(const char *func, GLenum pname, void **p, union value *v)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *unit;
   int mask, hash;
   const struct value_desc *d;

   mask = Elements(table) - 1;
   hash = (pname * prime_factor);
   while (1) {
      d = &values[table[hash & mask]];

      /* If the enum isn't valid, the hash walk ends with index 0,
       * which is the API mask entry at the beginning of values[]. */
      if (unlikely(d->type == TYPE_API_MASK)) {
	 _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=%s)", func,
                     _mesa_lookup_enum_by_nr(pname));
	 return &error_value;
      }

      if (likely(d->pname == pname))
	 break;

      hash += prime_step;
   }

   if (unlikely(d->extra && !check_extra(ctx, func, d)))
      return &error_value;

   switch (d->location) {
   case LOC_BUFFER:
      *p = ((char *) ctx->DrawBuffer + d->offset);
      return d;
   case LOC_CONTEXT:
      *p = ((char *) ctx + d->offset);
      return d;
   case LOC_ARRAY:
      *p = ((char *) ctx->Array.ArrayObj + d->offset);
      return d;
   case LOC_TEXUNIT:
      unit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      *p = ((char *) unit + d->offset);
      return d;
   case LOC_CUSTOM:
      find_custom_value(ctx, d, v);
      *p = v;
      return d;
   default:
      assert(0);
      break;
   }

   /* silence warning */
   return &error_value;
}

static const int transpose[] = {
   0, 4,  8, 12,
   1, 5,  9, 13,
   2, 6, 10, 14,
   3, 7, 11, 15
};

void GLAPIENTRY
_mesa_GetBooleanv(GLenum pname, GLboolean *params)
{
   const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
   void *p;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   d = find_value("glGetBooleanv", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
      break;
   case TYPE_CONST:
      params[0] = INT_TO_BOOLEAN(d->offset);
      break;

   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[3]);
   case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[2]);
   case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[1]);
   case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[0]);
      break;

   case TYPE_DOUBLEN:
      params[0] = FLOAT_TO_BOOLEAN(((GLdouble *) p)[0]);
      break;

   case TYPE_INT_4:
      params[3] = INT_TO_BOOLEAN(((GLint *) p)[3]);
   case TYPE_INT_3:
      params[2] = INT_TO_BOOLEAN(((GLint *) p)[2]);
   case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = INT_TO_BOOLEAN(((GLint *) p)[1]);
   case TYPE_INT:
   case TYPE_ENUM:
      params[0] = INT_TO_BOOLEAN(((GLint *) p)[0]);
      break;

   case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
	 params[i] = INT_TO_BOOLEAN(v.value_int_n.ints[i]);
      break;

   case TYPE_INT64:
      params[0] = INT64_TO_BOOLEAN(((GLint64 *) p)[0]);
      break;

   case TYPE_BOOLEAN:
      params[0] = ((GLboolean*) p)[0];
      break;		

   case TYPE_MATRIX:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_BOOLEAN(m->m[i]);
      break;

   case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_BOOLEAN(m->m[transpose[i]]);
      break;

   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
      params[0] = (*(GLbitfield *) p >> shift) & 1;
      break;
   }
}

void GLAPIENTRY
_mesa_GetFloatv(GLenum pname, GLfloat *params)
{
   const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
   void *p;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   d = find_value("glGetFloatv", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
      break;
   case TYPE_CONST:
      params[0] = (GLfloat) d->offset;
      break;

   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = ((GLfloat *) p)[3];
   case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = ((GLfloat *) p)[2];
   case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = ((GLfloat *) p)[1];
   case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = ((GLfloat *) p)[0];
      break;

   case TYPE_DOUBLEN:
      params[0] = ((GLdouble *) p)[0];
      break;

   case TYPE_INT_4:
      params[3] = (GLfloat) (((GLint *) p)[3]);
   case TYPE_INT_3:
      params[2] = (GLfloat) (((GLint *) p)[2]);
   case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = (GLfloat) (((GLint *) p)[1]);
   case TYPE_INT:
   case TYPE_ENUM:
      params[0] = (GLfloat) (((GLint *) p)[0]);
      break;

   case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
	 params[i] = INT_TO_FLOAT(v.value_int_n.ints[i]);
      break;

   case TYPE_INT64:
      params[0] = ((GLint64 *) p)[0];
      break;

   case TYPE_BOOLEAN:
      params[0] = BOOLEAN_TO_FLOAT(*(GLboolean*) p);
      break;		

   case TYPE_MATRIX:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = m->m[i];
      break;

   case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = m->m[transpose[i]];
      break;

   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
      params[0] = BOOLEAN_TO_FLOAT((*(GLbitfield *) p >> shift) & 1);
      break;
   }
}

void GLAPIENTRY
_mesa_GetIntegerv(GLenum pname, GLint *params)
{
   const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
   void *p;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   d = find_value("glGetIntegerv", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
      break;
   case TYPE_CONST:
      params[0] = d->offset;
      break;

   case TYPE_FLOAT_4:
      params[3] = IROUND(((GLfloat *) p)[3]);
   case TYPE_FLOAT_3:
      params[2] = IROUND(((GLfloat *) p)[2]);
   case TYPE_FLOAT_2:
      params[1] = IROUND(((GLfloat *) p)[1]);
   case TYPE_FLOAT:
      params[0] = IROUND(((GLfloat *) p)[0]);
      break;

   case TYPE_FLOATN_4:
      params[3] = FLOAT_TO_INT(((GLfloat *) p)[3]);
   case TYPE_FLOATN_3:
      params[2] = FLOAT_TO_INT(((GLfloat *) p)[2]);
   case TYPE_FLOATN_2:
      params[1] = FLOAT_TO_INT(((GLfloat *) p)[1]);
   case TYPE_FLOATN:
      params[0] = FLOAT_TO_INT(((GLfloat *) p)[0]);
      break;

   case TYPE_DOUBLEN:
      params[0] = FLOAT_TO_INT(((GLdouble *) p)[0]);
      break;

   case TYPE_INT_4:
      params[3] = ((GLint *) p)[3];
   case TYPE_INT_3:
      params[2] = ((GLint *) p)[2];
   case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = ((GLint *) p)[1];
   case TYPE_INT:
   case TYPE_ENUM:
      params[0] = ((GLint *) p)[0];
      break;

   case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
	 params[i] = v.value_int_n.ints[i];
      break;

   case TYPE_INT64:
      params[0] = INT64_TO_INT(((GLint64 *) p)[0]);
      break;

   case TYPE_BOOLEAN:
      params[0] = BOOLEAN_TO_INT(*(GLboolean*) p);
      break;		

   case TYPE_MATRIX:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_INT(m->m[i]);
      break;

   case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_INT(m->m[transpose[i]]);
      break;

   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
      params[0] = (*(GLbitfield *) p >> shift) & 1;
      break;
   }
}

#if FEATURE_ARB_sync
void GLAPIENTRY
_mesa_GetInteger64v(GLenum pname, GLint64 *params)
{
   const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
   void *p;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   d = find_value("glGetInteger64v", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
      break;
   case TYPE_CONST:
      params[0] = d->offset;
      break;

   case TYPE_FLOAT_4:
      params[3] = IROUND64(((GLfloat *) p)[3]);
   case TYPE_FLOAT_3:
      params[2] = IROUND64(((GLfloat *) p)[2]);
   case TYPE_FLOAT_2:
      params[1] = IROUND64(((GLfloat *) p)[1]);
   case TYPE_FLOAT:
      params[0] = IROUND64(((GLfloat *) p)[0]);
      break;

   case TYPE_FLOATN_4:
      params[3] = FLOAT_TO_INT64(((GLfloat *) p)[3]);
   case TYPE_FLOATN_3:
      params[2] = FLOAT_TO_INT64(((GLfloat *) p)[2]);
   case TYPE_FLOATN_2:
      params[1] = FLOAT_TO_INT64(((GLfloat *) p)[1]);
   case TYPE_FLOATN:
      params[0] = FLOAT_TO_INT64(((GLfloat *) p)[0]);
      break;

   case TYPE_DOUBLEN:
      params[0] = FLOAT_TO_INT64(((GLdouble *) p)[0]);
      break;

   case TYPE_INT_4:
      params[3] = ((GLint *) p)[3];
   case TYPE_INT_3:
      params[2] = ((GLint *) p)[2];
   case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = ((GLint *) p)[1];
   case TYPE_INT:
   case TYPE_ENUM:
      params[0] = ((GLint *) p)[0];
      break;

   case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
	 params[i] = INT_TO_BOOLEAN(v.value_int_n.ints[i]);
      break;

   case TYPE_INT64:
      params[0] = ((GLint64 *) p)[0];
      break;

   case TYPE_BOOLEAN:
      params[0] = ((GLboolean*) p)[0];
      break;		

   case TYPE_MATRIX:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_INT64(m->m[i]);
      break;

   case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_INT64(m->m[transpose[i]]);
      break;

   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
      params[0] = (*(GLbitfield *) p >> shift) & 1;
      break;
   }
}
#endif /* FEATURE_ARB_sync */

void GLAPIENTRY
_mesa_GetDoublev(GLenum pname, GLdouble *params)
{
   const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
   void *p;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   d = find_value("glGetDoublev", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
      break;
   case TYPE_CONST:
      params[0] = d->offset;
      break;

   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = ((GLfloat *) p)[3];
   case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = ((GLfloat *) p)[2];
   case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = ((GLfloat *) p)[1];
   case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = ((GLfloat *) p)[0];
      break;

   case TYPE_DOUBLEN:
      params[0] = ((GLdouble *) p)[0];
      break;

   case TYPE_INT_4:
      params[3] = ((GLint *) p)[3];
   case TYPE_INT_3:
      params[2] = ((GLint *) p)[2];
   case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = ((GLint *) p)[1];
   case TYPE_INT:
   case TYPE_ENUM:
      params[0] = ((GLint *) p)[0];
      break;

   case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
	 params[i] = v.value_int_n.ints[i];
      break;

   case TYPE_INT64:
      params[0] = ((GLint64 *) p)[0];
      break;

   case TYPE_BOOLEAN:
      params[0] = *(GLboolean*) p;
      break;		

   case TYPE_MATRIX:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = m->m[i];
      break;

   case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = m->m[transpose[i]];
      break;

   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
      params[0] = (*(GLbitfield *) p >> shift) & 1;
      break;
   }
}

static enum value_type
find_value_indexed(const char *func, GLenum pname, int index, union value *v)
{
   GET_CURRENT_CONTEXT(ctx);

   switch (pname) {

   case GL_BLEND:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.EXT_draw_buffers2)
	 goto invalid_enum;
      v->value_int = (ctx->Color.BlendEnabled >> index) & 1;
      return TYPE_INT;

   case GL_BLEND_SRC:
      /* fall-through */
   case GL_BLEND_SRC_RGB:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_draw_buffers_blend)
	 goto invalid_enum;
      v->value_int = ctx->Color.Blend[index].SrcRGB;
      return TYPE_INT;
   case GL_BLEND_SRC_ALPHA:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_draw_buffers_blend)
	 goto invalid_enum;
      v->value_int = ctx->Color.Blend[index].SrcA;
      return TYPE_INT;
   case GL_BLEND_DST:
      /* fall-through */
   case GL_BLEND_DST_RGB:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_draw_buffers_blend)
	 goto invalid_enum;
      v->value_int = ctx->Color.Blend[index].DstRGB;
      return TYPE_INT;
   case GL_BLEND_DST_ALPHA:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_draw_buffers_blend)
	 goto invalid_enum;
      v->value_int = ctx->Color.Blend[index].DstA;
      return TYPE_INT;
   case GL_BLEND_EQUATION_RGB:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_draw_buffers_blend)
	 goto invalid_enum;
      v->value_int = ctx->Color.Blend[index].EquationRGB;
      return TYPE_INT;
   case GL_BLEND_EQUATION_ALPHA:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_draw_buffers_blend)
	 goto invalid_enum;
      v->value_int = ctx->Color.Blend[index].EquationA;
      return TYPE_INT;

   case GL_COLOR_WRITEMASK:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.EXT_draw_buffers2)
	 goto invalid_enum;
      v->value_int_4[0] = ctx->Color.ColorMask[index][RCOMP] ? 1 : 0;
      v->value_int_4[1] = ctx->Color.ColorMask[index][GCOMP] ? 1 : 0;
      v->value_int_4[2] = ctx->Color.ColorMask[index][BCOMP] ? 1 : 0;
      v->value_int_4[3] = ctx->Color.ColorMask[index][ACOMP] ? 1 : 0;
      return TYPE_INT_4;

   case GL_TRANSFORM_FEEDBACK_BUFFER_START:
      if (index >= ctx->Const.MaxTransformFeedbackSeparateAttribs)
	 goto invalid_value;
      if (!ctx->Extensions.EXT_transform_feedback)
	 goto invalid_enum;
      v->value_int64 = ctx->TransformFeedback.CurrentObject->Offset[index];
      return TYPE_INT64;

   case GL_TRANSFORM_FEEDBACK_BUFFER_SIZE:
      if (index >= ctx->Const.MaxTransformFeedbackSeparateAttribs)
	 goto invalid_value;
      if (!ctx->Extensions.EXT_transform_feedback)
	 goto invalid_enum;
      v->value_int64 = ctx->TransformFeedback.CurrentObject->Size[index];
      return TYPE_INT64;

   case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
      if (index >= ctx->Const.MaxTransformFeedbackSeparateAttribs)
	 goto invalid_value;
      if (!ctx->Extensions.EXT_transform_feedback)
	 goto invalid_enum;
      v->value_int = ctx->TransformFeedback.CurrentObject->BufferNames[index];
      return TYPE_INT;
   }

 invalid_enum:
   _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=%s)", func,
               _mesa_lookup_enum_by_nr(pname));
   return TYPE_INVALID;
 invalid_value:
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(pname=%s)", func,
               _mesa_lookup_enum_by_nr(pname));
   return TYPE_INVALID;
}

void GLAPIENTRY
_mesa_GetBooleanIndexedv( GLenum pname, GLuint index, GLboolean *params )
{
   union value v;
   enum value_type type =
      find_value_indexed("glGetBooleanIndexedv", pname, index, &v);

   switch (type) {
   case TYPE_INT:
      params[0] = INT_TO_BOOLEAN(v.value_int);
      break;
   case TYPE_INT_4:
      params[0] = INT_TO_BOOLEAN(v.value_int_4[0]);
      params[1] = INT_TO_BOOLEAN(v.value_int_4[1]);
      params[2] = INT_TO_BOOLEAN(v.value_int_4[2]);
      params[3] = INT_TO_BOOLEAN(v.value_int_4[3]);
      break;
   case TYPE_INT64:
      params[0] = INT64_TO_BOOLEAN(v.value_int);
      break;
   default:
      ; /* nothing - GL error was recorded */
   }
}

void GLAPIENTRY
_mesa_GetIntegerIndexedv( GLenum pname, GLuint index, GLint *params )
{
   union value v;
   enum value_type type =
      find_value_indexed("glGetIntegerIndexedv", pname, index, &v);

   switch (type) {
   case TYPE_INT:
      params[0] = v.value_int;
      break;
   case TYPE_INT_4:
      params[0] = v.value_int_4[0];
      params[1] = v.value_int_4[1];
      params[2] = v.value_int_4[2];
      params[3] = v.value_int_4[3];
      break;
   case TYPE_INT64:
      params[0] = INT64_TO_INT(v.value_int);
      break;
   default:
      ; /* nothing - GL error was recorded */
   }
}

#if FEATURE_ARB_sync
void GLAPIENTRY
_mesa_GetInteger64Indexedv( GLenum pname, GLuint index, GLint64 *params )
{
   union value v;
   enum value_type type =
      find_value_indexed("glGetIntegerIndexedv", pname, index, &v);      

   switch (type) {
   case TYPE_INT:
      params[0] = v.value_int;
      break;
   case TYPE_INT_4:
      params[0] = v.value_int_4[0];
      params[1] = v.value_int_4[1];
      params[2] = v.value_int_4[2];
      params[3] = v.value_int_4[3];
      break;
   case TYPE_INT64:
      params[0] = v.value_int;
      break;
   default:
      ; /* nothing - GL error was recorded */
   }
}
#endif /* FEATURE_ARB_sync */

#if FEATURE_ES1
void GLAPIENTRY
_mesa_GetFixedv(GLenum pname, GLfixed *params)
{
   const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
   void *p;

   d = find_value("glGetDoublev", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
      break;
   case TYPE_CONST:
      params[0] = INT_TO_FIXED(d->offset);
      break;

   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = FLOAT_TO_FIXED(((GLfloat *) p)[3]);
   case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = FLOAT_TO_FIXED(((GLfloat *) p)[2]);
   case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = FLOAT_TO_FIXED(((GLfloat *) p)[1]);
   case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = FLOAT_TO_FIXED(((GLfloat *) p)[0]);
      break;

   case TYPE_DOUBLEN:
      params[0] = FLOAT_TO_FIXED(((GLdouble *) p)[0]);
      break;

   case TYPE_INT_4:
      params[3] = INT_TO_FIXED(((GLint *) p)[3]);
   case TYPE_INT_3:
      params[2] = INT_TO_FIXED(((GLint *) p)[2]);
   case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = INT_TO_FIXED(((GLint *) p)[1]);
   case TYPE_INT:
   case TYPE_ENUM:
      params[0] = INT_TO_FIXED(((GLint *) p)[0]);
      break;

   case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
	 params[i] = INT_TO_FIXED(v.value_int_n.ints[i]);
      break;

   case TYPE_INT64:
      params[0] = ((GLint64 *) p)[0];
      break;

   case TYPE_BOOLEAN:
      params[0] = BOOLEAN_TO_FIXED(((GLboolean*) p)[0]);
      break;		

   case TYPE_MATRIX:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_FIXED(m->m[i]);
      break;

   case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_FIXED(m->m[transpose[i]]);
      break;

   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
      params[0] = BOOLEAN_TO_FIXED((*(GLbitfield *) p >> shift) & 1);
      break;
   }
}
#endif
