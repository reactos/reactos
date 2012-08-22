/*
 * Copyright © 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

const glsl_type glsl_type::_error_type =
   glsl_type(GL_INVALID_ENUM, GLSL_TYPE_ERROR, 0, 0, "");

const glsl_type glsl_type::_void_type =
   glsl_type(GL_INVALID_ENUM, GLSL_TYPE_VOID, 0, 0, "void");

const glsl_type glsl_type::_sampler3D_type =
   glsl_type(GL_SAMPLER_3D, GLSL_SAMPLER_DIM_3D, 0, 0, GLSL_TYPE_FLOAT,
	     "sampler3D");

const glsl_type *const glsl_type::error_type = & glsl_type::_error_type;
const glsl_type *const glsl_type::void_type = & glsl_type::_void_type;

/** \name Core built-in types
 *
 * These types exist in all versions of GLSL.
 */
/*@{*/

const glsl_type glsl_type::builtin_core_types[] = {
   glsl_type(GL_BOOL,         GLSL_TYPE_BOOL, 1, 1, "bool"),
   glsl_type(GL_BOOL_VEC2,    GLSL_TYPE_BOOL, 2, 1, "bvec2"),
   glsl_type(GL_BOOL_VEC3,    GLSL_TYPE_BOOL, 3, 1, "bvec3"),
   glsl_type(GL_BOOL_VEC4,    GLSL_TYPE_BOOL, 4, 1, "bvec4"),
   glsl_type(GL_INT,          GLSL_TYPE_INT, 1, 1, "int"),
   glsl_type(GL_INT_VEC2,     GLSL_TYPE_INT, 2, 1, "ivec2"),
   glsl_type(GL_INT_VEC3,     GLSL_TYPE_INT, 3, 1, "ivec3"),
   glsl_type(GL_INT_VEC4,     GLSL_TYPE_INT, 4, 1, "ivec4"),
   glsl_type(GL_FLOAT,        GLSL_TYPE_FLOAT, 1, 1, "float"),
   glsl_type(GL_FLOAT_VEC2,   GLSL_TYPE_FLOAT, 2, 1, "vec2"),
   glsl_type(GL_FLOAT_VEC3,   GLSL_TYPE_FLOAT, 3, 1, "vec3"),
   glsl_type(GL_FLOAT_VEC4,   GLSL_TYPE_FLOAT, 4, 1, "vec4"),
   glsl_type(GL_FLOAT_MAT2,   GLSL_TYPE_FLOAT, 2, 2, "mat2"),
   glsl_type(GL_FLOAT_MAT3,   GLSL_TYPE_FLOAT, 3, 3, "mat3"),
   glsl_type(GL_FLOAT_MAT4,   GLSL_TYPE_FLOAT, 4, 4, "mat4"),
   glsl_type(GL_SAMPLER_2D,   GLSL_SAMPLER_DIM_2D, 0, 0, GLSL_TYPE_FLOAT,
	     "sampler2D"),
   glsl_type(GL_SAMPLER_CUBE, GLSL_SAMPLER_DIM_CUBE, 0, 0, GLSL_TYPE_FLOAT,
	     "samplerCube"),
};

const glsl_type *const glsl_type::bool_type  = & builtin_core_types[0];
const glsl_type *const glsl_type::int_type   = & builtin_core_types[4];
const glsl_type *const glsl_type::ivec4_type = & builtin_core_types[7];
const glsl_type *const glsl_type::float_type = & builtin_core_types[8];
const glsl_type *const glsl_type::vec2_type = & builtin_core_types[9];
const glsl_type *const glsl_type::vec3_type = & builtin_core_types[10];
const glsl_type *const glsl_type::vec4_type = & builtin_core_types[11];
const glsl_type *const glsl_type::mat2_type = & builtin_core_types[12];
const glsl_type *const glsl_type::mat3_type = & builtin_core_types[13];
const glsl_type *const glsl_type::mat4_type = & builtin_core_types[14];
/*@}*/

/** \name GLSL structures that have not been deprecated.
 */
/*@{*/

static const struct glsl_struct_field gl_DepthRangeParameters_fields[] = {
   { glsl_type::float_type, "near" },
   { glsl_type::float_type, "far" },
   { glsl_type::float_type, "diff" },
};

const glsl_type glsl_type::builtin_structure_types[] = {
   glsl_type(gl_DepthRangeParameters_fields,
             Elements(gl_DepthRangeParameters_fields),
             "gl_DepthRangeParameters"),
};
/*@}*/

/** \name GLSL 1.00 / 1.10 structures that are deprecated in GLSL 1.30
 */
/*@{*/

static const struct glsl_struct_field gl_PointParameters_fields[] = {
   { glsl_type::float_type, "size" },
   { glsl_type::float_type, "sizeMin" },
   { glsl_type::float_type, "sizeMax" },
   { glsl_type::float_type, "fadeThresholdSize" },
   { glsl_type::float_type, "distanceConstantAttenuation" },
   { glsl_type::float_type, "distanceLinearAttenuation" },
   { glsl_type::float_type, "distanceQuadraticAttenuation" },
};

static const struct glsl_struct_field gl_MaterialParameters_fields[] = {
   { glsl_type::vec4_type, "emission" },
   { glsl_type::vec4_type, "ambient" },
   { glsl_type::vec4_type, "diffuse" },
   { glsl_type::vec4_type, "specular" },
   { glsl_type::float_type, "shininess" },
};

static const struct glsl_struct_field gl_LightSourceParameters_fields[] = {
   { glsl_type::vec4_type, "ambient" },
   { glsl_type::vec4_type, "diffuse" },
   { glsl_type::vec4_type, "specular" },
   { glsl_type::vec4_type, "position" },
   { glsl_type::vec4_type, "halfVector" },
   { glsl_type::vec3_type, "spotDirection" },
   { glsl_type::float_type, "spotExponent" },
   { glsl_type::float_type, "spotCutoff" },
   { glsl_type::float_type, "spotCosCutoff" },
   { glsl_type::float_type, "constantAttenuation" },
   { glsl_type::float_type, "linearAttenuation" },
   { glsl_type::float_type, "quadraticAttenuation" },
};

static const struct glsl_struct_field gl_LightModelParameters_fields[] = {
   { glsl_type::vec4_type, "ambient" },
};

static const struct glsl_struct_field gl_LightModelProducts_fields[] = {
   { glsl_type::vec4_type, "sceneColor" },
};

static const struct glsl_struct_field gl_LightProducts_fields[] = {
   { glsl_type::vec4_type, "ambient" },
   { glsl_type::vec4_type, "diffuse" },
   { glsl_type::vec4_type, "specular" },
};

static const struct glsl_struct_field gl_FogParameters_fields[] = {
   { glsl_type::vec4_type, "color" },
   { glsl_type::float_type, "density" },
   { glsl_type::float_type, "start" },
   { glsl_type::float_type, "end" },
   { glsl_type::float_type, "scale" },
};

const glsl_type glsl_type::builtin_110_deprecated_structure_types[] = {
   glsl_type(gl_PointParameters_fields,
             Elements(gl_PointParameters_fields),
             "gl_PointParameters"),
   glsl_type(gl_MaterialParameters_fields,
             Elements(gl_MaterialParameters_fields),
             "gl_MaterialParameters"),
   glsl_type(gl_LightSourceParameters_fields,
             Elements(gl_LightSourceParameters_fields),
             "gl_LightSourceParameters"),
   glsl_type(gl_LightModelParameters_fields,
             Elements(gl_LightModelParameters_fields),
             "gl_LightModelParameters"),
   glsl_type(gl_LightModelProducts_fields,
             Elements(gl_LightModelProducts_fields),
             "gl_LightModelProducts"),
   glsl_type(gl_LightProducts_fields,
             Elements(gl_LightProducts_fields),
             "gl_LightProducts"),
   glsl_type(gl_FogParameters_fields,
             Elements(gl_FogParameters_fields),
             "gl_FogParameters"),
};
/*@}*/

/** \name Types in GLSL 1.10 (but not GLSL ES 1.00)
 */
/*@{*/
const glsl_type glsl_type::builtin_110_types[] = {
   glsl_type(GL_SAMPLER_1D,   GLSL_SAMPLER_DIM_1D, 0, 0, GLSL_TYPE_FLOAT,
	     "sampler1D"),
   glsl_type(GL_SAMPLER_1D_SHADOW, GLSL_SAMPLER_DIM_1D, 1, 0, GLSL_TYPE_FLOAT,
	     "sampler1DShadow"),
   glsl_type(GL_SAMPLER_2D_SHADOW, GLSL_SAMPLER_DIM_2D, 1, 0, GLSL_TYPE_FLOAT,
	     "sampler2DShadow"),
};
/*@}*/

/** \name Types added in GLSL 1.20
 */
/*@{*/

const glsl_type glsl_type::builtin_120_types[] = {
   glsl_type(GL_FLOAT_MAT2x3, GLSL_TYPE_FLOAT, 3, 2, "mat2x3"),
   glsl_type(GL_FLOAT_MAT2x4, GLSL_TYPE_FLOAT, 4, 2, "mat2x4"),
   glsl_type(GL_FLOAT_MAT3x2, GLSL_TYPE_FLOAT, 2, 3, "mat3x2"),
   glsl_type(GL_FLOAT_MAT3x4, GLSL_TYPE_FLOAT, 4, 3, "mat3x4"),
   glsl_type(GL_FLOAT_MAT4x2, GLSL_TYPE_FLOAT, 2, 4, "mat4x2"),
   glsl_type(GL_FLOAT_MAT4x3, GLSL_TYPE_FLOAT, 3, 4, "mat4x3"),
};
const glsl_type *const glsl_type::mat2x3_type = & builtin_120_types[0];
const glsl_type *const glsl_type::mat2x4_type = & builtin_120_types[1];
const glsl_type *const glsl_type::mat3x2_type = & builtin_120_types[2];
const glsl_type *const glsl_type::mat3x4_type = & builtin_120_types[3];
const glsl_type *const glsl_type::mat4x2_type = & builtin_120_types[4];
const glsl_type *const glsl_type::mat4x3_type = & builtin_120_types[5];
/*@}*/

/** \name Types added in GLSL 1.30
 */
/*@{*/

const glsl_type glsl_type::builtin_130_types[] = {
   glsl_type(GL_UNSIGNED_INT,      GLSL_TYPE_UINT, 1, 1, "uint"),
   glsl_type(GL_UNSIGNED_INT_VEC2, GLSL_TYPE_UINT, 2, 1, "uvec2"),
   glsl_type(GL_UNSIGNED_INT_VEC3, GLSL_TYPE_UINT, 3, 1, "uvec3"),
   glsl_type(GL_UNSIGNED_INT_VEC4, GLSL_TYPE_UINT, 4, 1, "uvec4"),

   /* 1D and 2D texture arrays - several of these are included only in
    * builtin_EXT_texture_array_types.
    */
   glsl_type(GL_INT_SAMPLER_1D_ARRAY,
	     GLSL_SAMPLER_DIM_1D, 0, 1,   GLSL_TYPE_INT, "isampler1DArray"),
   glsl_type(GL_UNSIGNED_INT_SAMPLER_1D_ARRAY,
	     GLSL_SAMPLER_DIM_1D, 0, 1,  GLSL_TYPE_UINT, "usampler1DArray"),
   glsl_type(GL_INT_SAMPLER_2D_ARRAY,
	     GLSL_SAMPLER_DIM_2D, 0, 1,   GLSL_TYPE_INT, "isampler2DArray"),
   glsl_type(GL_UNSIGNED_INT_SAMPLER_2D_ARRAY,
	     GLSL_SAMPLER_DIM_2D, 0, 1,  GLSL_TYPE_UINT, "usampler2DArray"),

   /* cube shadow samplers */
   glsl_type(GL_SAMPLER_CUBE_SHADOW,
	     GLSL_SAMPLER_DIM_CUBE, 1, 0, GLSL_TYPE_FLOAT, "samplerCubeShadow"),

   /* signed and unsigned integer samplers */
   glsl_type(GL_INT_SAMPLER_1D,
	     GLSL_SAMPLER_DIM_1D, 0, 0,   GLSL_TYPE_INT, "isampler1D"),
   glsl_type(GL_UNSIGNED_INT_SAMPLER_1D,
	     GLSL_SAMPLER_DIM_1D, 0, 0,  GLSL_TYPE_UINT, "usampler1D"),
   glsl_type(GL_INT_SAMPLER_2D,
	     GLSL_SAMPLER_DIM_2D, 0, 0,   GLSL_TYPE_INT, "isampler2D"),
   glsl_type(GL_UNSIGNED_INT_SAMPLER_2D,
	     GLSL_SAMPLER_DIM_2D, 0, 0,  GLSL_TYPE_UINT, "usampler2D"),
   glsl_type(GL_INT_SAMPLER_3D,
	     GLSL_SAMPLER_DIM_3D, 0, 0,   GLSL_TYPE_INT, "isampler3D"),
   glsl_type(GL_UNSIGNED_INT_SAMPLER_3D,
	     GLSL_SAMPLER_DIM_3D, 0, 0,  GLSL_TYPE_UINT, "usampler3D"),
   glsl_type(GL_INT_SAMPLER_CUBE,
	     GLSL_SAMPLER_DIM_CUBE, 0, 0,   GLSL_TYPE_INT, "isamplerCube"),
   glsl_type(GL_INT_SAMPLER_CUBE,
	     GLSL_SAMPLER_DIM_CUBE, 0, 0,  GLSL_TYPE_UINT, "usamplerCube"),
};

const glsl_type *const glsl_type::uint_type = & builtin_130_types[0];
const glsl_type *const glsl_type::uvec2_type = & builtin_130_types[1];
const glsl_type *const glsl_type::uvec3_type = & builtin_130_types[2];
const glsl_type *const glsl_type::uvec4_type = & builtin_130_types[3];
/*@}*/

/** \name Sampler types added by GL_ARB_texture_rectangle
 */
/*@{*/

const glsl_type glsl_type::builtin_ARB_texture_rectangle_types[] = {
   glsl_type(GL_SAMPLER_2D_RECT,
	     GLSL_SAMPLER_DIM_RECT, 0, 0, GLSL_TYPE_FLOAT, "sampler2DRect"),
   glsl_type(GL_SAMPLER_2D_RECT_SHADOW,
	     GLSL_SAMPLER_DIM_RECT, 1, 0, GLSL_TYPE_FLOAT, "sampler2DRectShadow"),
};
/*@}*/

/** \name Sampler types added by GL_EXT_texture_array
 */
/*@{*/

const glsl_type glsl_type::builtin_EXT_texture_array_types[] = {
   glsl_type(GL_SAMPLER_1D_ARRAY,
	     GLSL_SAMPLER_DIM_1D, 0, 1, GLSL_TYPE_FLOAT, "sampler1DArray"),
   glsl_type(GL_SAMPLER_2D_ARRAY,
	     GLSL_SAMPLER_DIM_2D, 0, 1, GLSL_TYPE_FLOAT, "sampler2DArray"),
   glsl_type(GL_SAMPLER_1D_ARRAY_SHADOW,
	     GLSL_SAMPLER_DIM_1D, 1, 1, GLSL_TYPE_FLOAT, "sampler1DArrayShadow"),
   glsl_type(GL_SAMPLER_2D_ARRAY_SHADOW,
	     GLSL_SAMPLER_DIM_2D, 1, 1, GLSL_TYPE_FLOAT, "sampler2DArrayShadow"),
};
/*@}*/

/** \name Sampler types added by GL_EXT_texture_buffer_object
 */
/*@{*/

const glsl_type glsl_type::builtin_EXT_texture_buffer_object_types[] = {
   glsl_type(GL_SAMPLER_BUFFER,
	     GLSL_SAMPLER_DIM_BUF, 0, 0, GLSL_TYPE_FLOAT, "samplerBuffer"),
   glsl_type(GL_INT_SAMPLER_BUFFER,
	     GLSL_SAMPLER_DIM_BUF, 0, 0,   GLSL_TYPE_INT, "isamplerBuffer"),
   glsl_type(GL_UNSIGNED_INT_SAMPLER_BUFFER,
	     GLSL_SAMPLER_DIM_BUF, 0, 0,  GLSL_TYPE_UINT, "usamplerBuffer"),
};
/*@}*/

/** \name Sampler types added by GL_OES_EGL_image_external
 */
/*@{*/

const glsl_type glsl_type::builtin_OES_EGL_image_external_types[] = {
   glsl_type(GL_SAMPLER_EXTERNAL_OES,
	     GLSL_SAMPLER_DIM_EXTERNAL, 0, 0, GLSL_TYPE_FLOAT, "samplerExternalOES"),
};
/*@}*/
