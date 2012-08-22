/*
 * Copyright © 2010 Intel Corporation
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

#include "ir.h"
#include "glsl_parser_extras.h"
#include "glsl_symbol_table.h"
#include "main/core.h"
#include "main/uniforms.h"
#include "program/prog_parameter.h"
#include "program/prog_statevars.h"
#include "program/prog_instruction.h"

static void generate_ARB_draw_buffers_variables(exec_list *,
						struct _mesa_glsl_parse_state *,
						bool, _mesa_glsl_parser_targets);

static void
generate_ARB_draw_instanced_variables(exec_list *,
                                      struct _mesa_glsl_parse_state *,
                                      bool, _mesa_glsl_parser_targets);

struct builtin_variable {
   enum ir_variable_mode mode;
   int slot;
   const char *type;
   const char *name;
};

static const builtin_variable builtin_core_vs_variables[] = {
   { ir_var_out, VERT_RESULT_HPOS, "vec4",  "gl_Position" },
   { ir_var_out, VERT_RESULT_PSIZ, "float", "gl_PointSize" },
};

static const builtin_variable builtin_core_fs_variables[] = {
   { ir_var_in,  FRAG_ATTRIB_WPOS,  "vec4",  "gl_FragCoord" },
   { ir_var_in,  FRAG_ATTRIB_FACE,  "bool",  "gl_FrontFacing" },
   { ir_var_out, FRAG_RESULT_COLOR, "vec4",  "gl_FragColor" },
};

static const builtin_variable builtin_100ES_fs_variables[] = {
   { ir_var_in,  FRAG_ATTRIB_PNTC,   "vec2",   "gl_PointCoord" },
};

static const builtin_variable builtin_110_fs_variables[] = {
   { ir_var_out, FRAG_RESULT_DEPTH, "float", "gl_FragDepth" },
};

static const builtin_variable builtin_110_deprecated_fs_variables[] = {
   { ir_var_in,  FRAG_ATTRIB_COL0,  "vec4",  "gl_Color" },
   { ir_var_in,  FRAG_ATTRIB_COL1,  "vec4",  "gl_SecondaryColor" },
   { ir_var_in,  FRAG_ATTRIB_FOGC,  "float", "gl_FogFragCoord" },
};

static const builtin_variable builtin_110_deprecated_vs_variables[] = {
   { ir_var_in,  VERT_ATTRIB_POS,         "vec4",  "gl_Vertex" },
   { ir_var_in,  VERT_ATTRIB_NORMAL,      "vec3",  "gl_Normal" },
   { ir_var_in,  VERT_ATTRIB_COLOR0,      "vec4",  "gl_Color" },
   { ir_var_in,  VERT_ATTRIB_COLOR1,      "vec4",  "gl_SecondaryColor" },
   { ir_var_in,  VERT_ATTRIB_TEX0,        "vec4",  "gl_MultiTexCoord0" },
   { ir_var_in,  VERT_ATTRIB_TEX1,        "vec4",  "gl_MultiTexCoord1" },
   { ir_var_in,  VERT_ATTRIB_TEX2,        "vec4",  "gl_MultiTexCoord2" },
   { ir_var_in,  VERT_ATTRIB_TEX3,        "vec4",  "gl_MultiTexCoord3" },
   { ir_var_in,  VERT_ATTRIB_TEX4,        "vec4",  "gl_MultiTexCoord4" },
   { ir_var_in,  VERT_ATTRIB_TEX5,        "vec4",  "gl_MultiTexCoord5" },
   { ir_var_in,  VERT_ATTRIB_TEX6,        "vec4",  "gl_MultiTexCoord6" },
   { ir_var_in,  VERT_ATTRIB_TEX7,        "vec4",  "gl_MultiTexCoord7" },
   { ir_var_in,  VERT_ATTRIB_FOG,         "float", "gl_FogCoord" },
   { ir_var_out, VERT_RESULT_CLIP_VERTEX, "vec4",  "gl_ClipVertex" },
   { ir_var_out, VERT_RESULT_COL0,        "vec4",  "gl_FrontColor" },
   { ir_var_out, VERT_RESULT_BFC0,        "vec4",  "gl_BackColor" },
   { ir_var_out, VERT_RESULT_COL1,        "vec4",  "gl_FrontSecondaryColor" },
   { ir_var_out, VERT_RESULT_BFC1,        "vec4",  "gl_BackSecondaryColor" },
   { ir_var_out, VERT_RESULT_FOGC,        "float", "gl_FogFragCoord" },
};

static const builtin_variable builtin_120_fs_variables[] = {
   { ir_var_in,  FRAG_ATTRIB_PNTC,   "vec2",   "gl_PointCoord" },
};

static const builtin_variable builtin_130_vs_variables[] = {
   { ir_var_system_value,  SYSTEM_VALUE_VERTEX_ID, "int",   "gl_VertexID" },
};

static const builtin_variable builtin_110_deprecated_uniforms[] = {
   { ir_var_uniform, -1, "mat4", "gl_ModelViewMatrix" },
   { ir_var_uniform, -1, "mat4", "gl_ProjectionMatrix" },
   { ir_var_uniform, -1, "mat4", "gl_ModelViewProjectionMatrix" },
   { ir_var_uniform, -1, "mat3", "gl_NormalMatrix" },
   { ir_var_uniform, -1, "mat4", "gl_ModelViewMatrixInverse" },
   { ir_var_uniform, -1, "mat4", "gl_ProjectionMatrixInverse" },
   { ir_var_uniform, -1, "mat4", "gl_ModelViewProjectionMatrixInverse" },
   { ir_var_uniform, -1, "mat4", "gl_ModelViewMatrixTranspose" },
   { ir_var_uniform, -1, "mat4", "gl_ProjectionMatrixTranspose" },
   { ir_var_uniform, -1, "mat4", "gl_ModelViewProjectionMatrixTranspose" },
   { ir_var_uniform, -1, "mat4", "gl_ModelViewMatrixInverseTranspose" },
   { ir_var_uniform, -1, "mat4", "gl_ProjectionMatrixInverseTranspose" },
   { ir_var_uniform, -1, "mat4", "gl_ModelViewProjectionMatrixInverseTranspose" },
   { ir_var_uniform, -1, "float", "gl_NormalScale" },
   { ir_var_uniform, -1, "gl_LightModelParameters", "gl_LightModel"},

   /* Mesa-internal ATI_envmap_bumpmap state. */
   { ir_var_uniform, -1, "vec2", "gl_BumpRotMatrix0MESA"},
   { ir_var_uniform, -1, "vec2", "gl_BumpRotMatrix1MESA"},
   { ir_var_uniform, -1, "vec4", "gl_FogParamsOptimizedMESA"},
};

static struct gl_builtin_uniform_element gl_DepthRange_elements[] = {
   {"near", {STATE_DEPTH_RANGE, 0, 0}, SWIZZLE_XXXX},
   {"far", {STATE_DEPTH_RANGE, 0, 0}, SWIZZLE_YYYY},
   {"diff", {STATE_DEPTH_RANGE, 0, 0}, SWIZZLE_ZZZZ},
};

static struct gl_builtin_uniform_element gl_ClipPlane_elements[] = {
   {NULL, {STATE_CLIPPLANE, 0, 0}, SWIZZLE_XYZW}
};

static struct gl_builtin_uniform_element gl_Point_elements[] = {
   {"size", {STATE_POINT_SIZE}, SWIZZLE_XXXX},
   {"sizeMin", {STATE_POINT_SIZE}, SWIZZLE_YYYY},
   {"sizeMax", {STATE_POINT_SIZE}, SWIZZLE_ZZZZ},
   {"fadeThresholdSize", {STATE_POINT_SIZE}, SWIZZLE_WWWW},
   {"distanceConstantAttenuation", {STATE_POINT_ATTENUATION}, SWIZZLE_XXXX},
   {"distanceLinearAttenuation", {STATE_POINT_ATTENUATION}, SWIZZLE_YYYY},
   {"distanceQuadraticAttenuation", {STATE_POINT_ATTENUATION}, SWIZZLE_ZZZZ},
};

static struct gl_builtin_uniform_element gl_FrontMaterial_elements[] = {
   {"emission", {STATE_MATERIAL, 0, STATE_EMISSION}, SWIZZLE_XYZW},
   {"ambient", {STATE_MATERIAL, 0, STATE_AMBIENT}, SWIZZLE_XYZW},
   {"diffuse", {STATE_MATERIAL, 0, STATE_DIFFUSE}, SWIZZLE_XYZW},
   {"specular", {STATE_MATERIAL, 0, STATE_SPECULAR}, SWIZZLE_XYZW},
   {"shininess", {STATE_MATERIAL, 0, STATE_SHININESS}, SWIZZLE_XXXX},
};

static struct gl_builtin_uniform_element gl_BackMaterial_elements[] = {
   {"emission", {STATE_MATERIAL, 1, STATE_EMISSION}, SWIZZLE_XYZW},
   {"ambient", {STATE_MATERIAL, 1, STATE_AMBIENT}, SWIZZLE_XYZW},
   {"diffuse", {STATE_MATERIAL, 1, STATE_DIFFUSE}, SWIZZLE_XYZW},
   {"specular", {STATE_MATERIAL, 1, STATE_SPECULAR}, SWIZZLE_XYZW},
   {"shininess", {STATE_MATERIAL, 1, STATE_SHININESS}, SWIZZLE_XXXX},
};

static struct gl_builtin_uniform_element gl_LightSource_elements[] = {
   {"ambient", {STATE_LIGHT, 0, STATE_AMBIENT}, SWIZZLE_XYZW},
   {"diffuse", {STATE_LIGHT, 0, STATE_DIFFUSE}, SWIZZLE_XYZW},
   {"specular", {STATE_LIGHT, 0, STATE_SPECULAR}, SWIZZLE_XYZW},
   {"position", {STATE_LIGHT, 0, STATE_POSITION}, SWIZZLE_XYZW},
   {"halfVector", {STATE_LIGHT, 0, STATE_HALF_VECTOR}, SWIZZLE_XYZW},
   {"spotDirection", {STATE_LIGHT, 0, STATE_SPOT_DIRECTION},
    MAKE_SWIZZLE4(SWIZZLE_X,
		  SWIZZLE_Y,
		  SWIZZLE_Z,
		  SWIZZLE_Z)},
   {"spotCosCutoff", {STATE_LIGHT, 0, STATE_SPOT_DIRECTION}, SWIZZLE_WWWW},
   {"spotCutoff", {STATE_LIGHT, 0, STATE_SPOT_CUTOFF}, SWIZZLE_XXXX},
   {"spotExponent", {STATE_LIGHT, 0, STATE_ATTENUATION}, SWIZZLE_WWWW},
   {"constantAttenuation", {STATE_LIGHT, 0, STATE_ATTENUATION}, SWIZZLE_XXXX},
   {"linearAttenuation", {STATE_LIGHT, 0, STATE_ATTENUATION}, SWIZZLE_YYYY},
   {"quadraticAttenuation", {STATE_LIGHT, 0, STATE_ATTENUATION}, SWIZZLE_ZZZZ},
};

static struct gl_builtin_uniform_element gl_LightModel_elements[] = {
   {"ambient", {STATE_LIGHTMODEL_AMBIENT, 0}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_FrontLightModelProduct_elements[] = {
   {"sceneColor", {STATE_LIGHTMODEL_SCENECOLOR, 0}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_BackLightModelProduct_elements[] = {
   {"sceneColor", {STATE_LIGHTMODEL_SCENECOLOR, 1}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_FrontLightProduct_elements[] = {
   {"ambient", {STATE_LIGHTPROD, 0, 0, STATE_AMBIENT}, SWIZZLE_XYZW},
   {"diffuse", {STATE_LIGHTPROD, 0, 0, STATE_DIFFUSE}, SWIZZLE_XYZW},
   {"specular", {STATE_LIGHTPROD, 0, 0, STATE_SPECULAR}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_BackLightProduct_elements[] = {
   {"ambient", {STATE_LIGHTPROD, 0, 1, STATE_AMBIENT}, SWIZZLE_XYZW},
   {"diffuse", {STATE_LIGHTPROD, 0, 1, STATE_DIFFUSE}, SWIZZLE_XYZW},
   {"specular", {STATE_LIGHTPROD, 0, 1, STATE_SPECULAR}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_TextureEnvColor_elements[] = {
   {NULL, {STATE_TEXENV_COLOR, 0}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_EyePlaneS_elements[] = {
   {NULL, {STATE_TEXGEN, 0, STATE_TEXGEN_EYE_S}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_EyePlaneT_elements[] = {
   {NULL, {STATE_TEXGEN, 0, STATE_TEXGEN_EYE_T}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_EyePlaneR_elements[] = {
   {NULL, {STATE_TEXGEN, 0, STATE_TEXGEN_EYE_R}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_EyePlaneQ_elements[] = {
   {NULL, {STATE_TEXGEN, 0, STATE_TEXGEN_EYE_Q}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_ObjectPlaneS_elements[] = {
   {NULL, {STATE_TEXGEN, 0, STATE_TEXGEN_OBJECT_S}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_ObjectPlaneT_elements[] = {
   {NULL, {STATE_TEXGEN, 0, STATE_TEXGEN_OBJECT_T}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_ObjectPlaneR_elements[] = {
   {NULL, {STATE_TEXGEN, 0, STATE_TEXGEN_OBJECT_R}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_ObjectPlaneQ_elements[] = {
   {NULL, {STATE_TEXGEN, 0, STATE_TEXGEN_OBJECT_Q}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_Fog_elements[] = {
   {"color", {STATE_FOG_COLOR}, SWIZZLE_XYZW},
   {"density", {STATE_FOG_PARAMS}, SWIZZLE_XXXX},
   {"start", {STATE_FOG_PARAMS}, SWIZZLE_YYYY},
   {"end", {STATE_FOG_PARAMS}, SWIZZLE_ZZZZ},
   {"scale", {STATE_FOG_PARAMS}, SWIZZLE_WWWW},
};

static struct gl_builtin_uniform_element gl_NormalScale_elements[] = {
   {NULL, {STATE_NORMAL_SCALE}, SWIZZLE_XXXX},
};

static struct gl_builtin_uniform_element gl_BumpRotMatrix0MESA_elements[] = {
   {NULL, {STATE_INTERNAL, STATE_ROT_MATRIX_0}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_BumpRotMatrix1MESA_elements[] = {
   {NULL, {STATE_INTERNAL, STATE_ROT_MATRIX_1}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_FogParamsOptimizedMESA_elements[] = {
   {NULL, {STATE_INTERNAL, STATE_FOG_PARAMS_OPTIMIZED}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_CurrentAttribVertMESA_elements[] = {
   {NULL, {STATE_INTERNAL, STATE_CURRENT_ATTRIB, 0}, SWIZZLE_XYZW},
};

static struct gl_builtin_uniform_element gl_CurrentAttribFragMESA_elements[] = {
   {NULL, {STATE_INTERNAL, STATE_CURRENT_ATTRIB_MAYBE_VP_CLAMPED, 0}, SWIZZLE_XYZW},
};

#define MATRIX(name, statevar, modifier)				\
   static struct gl_builtin_uniform_element name ## _elements[] = {	\
      { NULL, { statevar, 0, 0, 0, modifier}, SWIZZLE_XYZW },		\
      { NULL, { statevar, 0, 1, 1, modifier}, SWIZZLE_XYZW },		\
      { NULL, { statevar, 0, 2, 2, modifier}, SWIZZLE_XYZW },		\
      { NULL, { statevar, 0, 3, 3, modifier}, SWIZZLE_XYZW },		\
   }

MATRIX(gl_ModelViewMatrix,
       STATE_MODELVIEW_MATRIX, STATE_MATRIX_TRANSPOSE);
MATRIX(gl_ModelViewMatrixInverse,
       STATE_MODELVIEW_MATRIX, STATE_MATRIX_INVTRANS);
MATRIX(gl_ModelViewMatrixTranspose,
       STATE_MODELVIEW_MATRIX, 0);
MATRIX(gl_ModelViewMatrixInverseTranspose,
       STATE_MODELVIEW_MATRIX, STATE_MATRIX_INVERSE);

MATRIX(gl_ProjectionMatrix,
       STATE_PROJECTION_MATRIX, STATE_MATRIX_TRANSPOSE);
MATRIX(gl_ProjectionMatrixInverse,
       STATE_PROJECTION_MATRIX, STATE_MATRIX_INVTRANS);
MATRIX(gl_ProjectionMatrixTranspose,
       STATE_PROJECTION_MATRIX, 0);
MATRIX(gl_ProjectionMatrixInverseTranspose,
       STATE_PROJECTION_MATRIX, STATE_MATRIX_INVERSE);

MATRIX(gl_ModelViewProjectionMatrix,
       STATE_MVP_MATRIX, STATE_MATRIX_TRANSPOSE);
MATRIX(gl_ModelViewProjectionMatrixInverse,
       STATE_MVP_MATRIX, STATE_MATRIX_INVTRANS);
MATRIX(gl_ModelViewProjectionMatrixTranspose,
       STATE_MVP_MATRIX, 0);
MATRIX(gl_ModelViewProjectionMatrixInverseTranspose,
       STATE_MVP_MATRIX, STATE_MATRIX_INVERSE);

MATRIX(gl_TextureMatrix,
       STATE_TEXTURE_MATRIX, STATE_MATRIX_TRANSPOSE);
MATRIX(gl_TextureMatrixInverse,
       STATE_TEXTURE_MATRIX, STATE_MATRIX_INVTRANS);
MATRIX(gl_TextureMatrixTranspose,
       STATE_TEXTURE_MATRIX, 0);
MATRIX(gl_TextureMatrixInverseTranspose,
       STATE_TEXTURE_MATRIX, STATE_MATRIX_INVERSE);

static struct gl_builtin_uniform_element gl_NormalMatrix_elements[] = {
   { NULL, { STATE_MODELVIEW_MATRIX, 0, 0, 0, STATE_MATRIX_INVERSE},
     MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_Z) },
   { NULL, { STATE_MODELVIEW_MATRIX, 0, 1, 1, STATE_MATRIX_INVERSE},
     MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_Z) },
   { NULL, { STATE_MODELVIEW_MATRIX, 0, 2, 2, STATE_MATRIX_INVERSE},
     MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_Z) },
};

#undef MATRIX

#define STATEVAR(name) {#name, name ## _elements, Elements(name ## _elements)}

const struct gl_builtin_uniform_desc _mesa_builtin_uniform_desc[] = {
   STATEVAR(gl_DepthRange),
   STATEVAR(gl_ClipPlane),
   STATEVAR(gl_Point),
   STATEVAR(gl_FrontMaterial),
   STATEVAR(gl_BackMaterial),
   STATEVAR(gl_LightSource),
   STATEVAR(gl_LightModel),
   STATEVAR(gl_FrontLightModelProduct),
   STATEVAR(gl_BackLightModelProduct),
   STATEVAR(gl_FrontLightProduct),
   STATEVAR(gl_BackLightProduct),
   STATEVAR(gl_TextureEnvColor),
   STATEVAR(gl_EyePlaneS),
   STATEVAR(gl_EyePlaneT),
   STATEVAR(gl_EyePlaneR),
   STATEVAR(gl_EyePlaneQ),
   STATEVAR(gl_ObjectPlaneS),
   STATEVAR(gl_ObjectPlaneT),
   STATEVAR(gl_ObjectPlaneR),
   STATEVAR(gl_ObjectPlaneQ),
   STATEVAR(gl_Fog),

   STATEVAR(gl_ModelViewMatrix),
   STATEVAR(gl_ModelViewMatrixInverse),
   STATEVAR(gl_ModelViewMatrixTranspose),
   STATEVAR(gl_ModelViewMatrixInverseTranspose),

   STATEVAR(gl_ProjectionMatrix),
   STATEVAR(gl_ProjectionMatrixInverse),
   STATEVAR(gl_ProjectionMatrixTranspose),
   STATEVAR(gl_ProjectionMatrixInverseTranspose),

   STATEVAR(gl_ModelViewProjectionMatrix),
   STATEVAR(gl_ModelViewProjectionMatrixInverse),
   STATEVAR(gl_ModelViewProjectionMatrixTranspose),
   STATEVAR(gl_ModelViewProjectionMatrixInverseTranspose),

   STATEVAR(gl_TextureMatrix),
   STATEVAR(gl_TextureMatrixInverse),
   STATEVAR(gl_TextureMatrixTranspose),
   STATEVAR(gl_TextureMatrixInverseTranspose),

   STATEVAR(gl_NormalMatrix),
   STATEVAR(gl_NormalScale),

   STATEVAR(gl_BumpRotMatrix0MESA),
   STATEVAR(gl_BumpRotMatrix1MESA),
   STATEVAR(gl_FogParamsOptimizedMESA),
   STATEVAR(gl_CurrentAttribVertMESA),
   STATEVAR(gl_CurrentAttribFragMESA),

   {NULL, NULL, 0}
};

static ir_variable *
add_variable(exec_list *instructions, glsl_symbol_table *symtab,
	     const char *name, const glsl_type *type,
	     enum ir_variable_mode mode, int slot)
{
   ir_variable *var = new(symtab) ir_variable(type, name, mode);

   switch (var->mode) {
   case ir_var_auto:
   case ir_var_in:
   case ir_var_const_in:
   case ir_var_uniform:
   case ir_var_system_value:
      var->read_only = true;
      break;
   case ir_var_inout:
   case ir_var_out:
      break;
   default:
      assert(0);
      break;
   }

   var->location = slot;
   var->explicit_location = (slot >= 0);

   /* Once the variable is created an initialized, add it to the symbol table
    * and add the declaration to the IR stream.
    */
   instructions->push_tail(var);

   symtab->add_variable(var);
   return var;
}

static ir_variable *
add_uniform(exec_list *instructions, glsl_symbol_table *symtab,
	    const char *name, const glsl_type *type)
{
   ir_variable *const uni =
      add_variable(instructions, symtab, name, type, ir_var_uniform, -1);

   unsigned i;
   for (i = 0; _mesa_builtin_uniform_desc[i].name != NULL; i++) {
      if (strcmp(_mesa_builtin_uniform_desc[i].name, name) == 0) {
	 break;
      }
   }

   assert(_mesa_builtin_uniform_desc[i].name != NULL);
   const struct gl_builtin_uniform_desc* const statevar =
      &_mesa_builtin_uniform_desc[i];

   const unsigned array_count = type->is_array() ? type->length : 1;
   uni->num_state_slots = array_count * statevar->num_elements;

   ir_state_slot *slots =
      ralloc_array(uni, ir_state_slot, uni->num_state_slots);

   uni->state_slots = slots;

   for (unsigned a = 0; a < array_count; a++) {
      for (unsigned j = 0; j < statevar->num_elements; j++) {
	 struct gl_builtin_uniform_element *element = &statevar->elements[j];

	 memcpy(slots->tokens, element->tokens, sizeof(element->tokens));
	 if (type->is_array()) {
	    if (strcmp(name, "gl_CurrentAttribVertMESA") == 0 ||
		strcmp(name, "gl_CurrentAttribFragMESA") == 0) {
	       slots->tokens[2] = a;
	    } else {
	       slots->tokens[1] = a;
	    }
	 }

	 slots->swizzle = element->swizzle;
	 slots++;
      }
   }

   return uni;
}

static void
add_builtin_variable(exec_list *instructions, glsl_symbol_table *symtab,
		     const builtin_variable *proto)
{
   /* Create a new variable declaration from the description supplied by
    * the caller.
    */
   const glsl_type *const type = symtab->get_type(proto->type);

   assert(type != NULL);

   if (proto->mode == ir_var_uniform) {
      add_uniform(instructions, symtab, proto->name, type);
   } else {
      add_variable(instructions, symtab, proto->name, type, proto->mode,
		   proto->slot);
   }
}

static ir_variable *
add_builtin_constant(exec_list *instructions, glsl_symbol_table *symtab,
		     const char *name, int value)
{
   ir_variable *const var = add_variable(instructions, symtab,
					 name, glsl_type::int_type,
					 ir_var_auto, -1);
   var->constant_value = new(var) ir_constant(value);
   var->constant_initializer = new(var) ir_constant(value);
   var->has_initializer = true;
   return var;
}

/* Several constants in GLSL ES have different names than normal desktop GLSL.
 * Therefore, this function should only be called on the ES path.
 */
static void
generate_100ES_uniforms(exec_list *instructions,
		     struct _mesa_glsl_parse_state *state)
{
   glsl_symbol_table *const symtab = state->symbols;

   add_builtin_constant(instructions, symtab, "gl_MaxVertexAttribs",
			state->Const.MaxVertexAttribs);
   add_builtin_constant(instructions, symtab, "gl_MaxVertexUniformVectors",
			state->Const.MaxVertexUniformComponents);
   add_builtin_constant(instructions, symtab, "gl_MaxVaryingVectors",
			state->Const.MaxVaryingFloats / 4);
   add_builtin_constant(instructions, symtab, "gl_MaxVertexTextureImageUnits",
			state->Const.MaxVertexTextureImageUnits);
   add_builtin_constant(instructions, symtab, "gl_MaxCombinedTextureImageUnits",
			state->Const.MaxCombinedTextureImageUnits);
   add_builtin_constant(instructions, symtab, "gl_MaxTextureImageUnits",
			state->Const.MaxTextureImageUnits);
   add_builtin_constant(instructions, symtab, "gl_MaxFragmentUniformVectors",
			state->Const.MaxFragmentUniformComponents);

   add_uniform(instructions, symtab, "gl_DepthRange",
	       state->symbols->get_type("gl_DepthRangeParameters"));
}

static void
generate_110_uniforms(exec_list *instructions,
		      struct _mesa_glsl_parse_state *state)
{
   glsl_symbol_table *const symtab = state->symbols;

   for (unsigned i = 0
	   ; i < Elements(builtin_110_deprecated_uniforms)
	   ; i++) {
      add_builtin_variable(instructions, symtab,
			   & builtin_110_deprecated_uniforms[i]);
   }

   add_builtin_constant(instructions, symtab, "gl_MaxLights",
			state->Const.MaxLights);
   add_builtin_constant(instructions, symtab, "gl_MaxClipPlanes",
			state->Const.MaxClipPlanes);
   add_builtin_constant(instructions, symtab, "gl_MaxTextureUnits",
			state->Const.MaxTextureUnits);
   add_builtin_constant(instructions, symtab, "gl_MaxTextureCoords",
			state->Const.MaxTextureCoords);
   add_builtin_constant(instructions, symtab, "gl_MaxVertexAttribs",
			state->Const.MaxVertexAttribs);
   add_builtin_constant(instructions, symtab, "gl_MaxVertexUniformComponents",
			state->Const.MaxVertexUniformComponents);
   add_builtin_constant(instructions, symtab, "gl_MaxVaryingFloats",
			state->Const.MaxVaryingFloats);
   add_builtin_constant(instructions, symtab, "gl_MaxVertexTextureImageUnits",
			state->Const.MaxVertexTextureImageUnits);
   add_builtin_constant(instructions, symtab, "gl_MaxCombinedTextureImageUnits",
			state->Const.MaxCombinedTextureImageUnits);
   add_builtin_constant(instructions, symtab, "gl_MaxTextureImageUnits",
			state->Const.MaxTextureImageUnits);
   add_builtin_constant(instructions, symtab, "gl_MaxFragmentUniformComponents",
			state->Const.MaxFragmentUniformComponents);

   const glsl_type *const mat4_array_type =
      glsl_type::get_array_instance(glsl_type::mat4_type,
				    state->Const.MaxTextureCoords);

   add_uniform(instructions, symtab, "gl_TextureMatrix", mat4_array_type);
   add_uniform(instructions, symtab, "gl_TextureMatrixInverse", mat4_array_type);
   add_uniform(instructions, symtab, "gl_TextureMatrixTranspose", mat4_array_type);
   add_uniform(instructions, symtab, "gl_TextureMatrixInverseTranspose", mat4_array_type);

   add_uniform(instructions, symtab, "gl_DepthRange",
		symtab->get_type("gl_DepthRangeParameters"));

   add_uniform(instructions, symtab, "gl_ClipPlane",
	       glsl_type::get_array_instance(glsl_type::vec4_type,
					     state->Const.MaxClipPlanes));
   add_uniform(instructions, symtab, "gl_Point",
	       symtab->get_type("gl_PointParameters"));

   const glsl_type *const material_parameters_type =
      symtab->get_type("gl_MaterialParameters");
   add_uniform(instructions, symtab, "gl_FrontMaterial", material_parameters_type);
   add_uniform(instructions, symtab, "gl_BackMaterial", material_parameters_type);

   const glsl_type *const light_source_array_type =
      glsl_type::get_array_instance(symtab->get_type("gl_LightSourceParameters"), state->Const.MaxLights);

   add_uniform(instructions, symtab, "gl_LightSource", light_source_array_type);

   const glsl_type *const light_model_products_type =
      symtab->get_type("gl_LightModelProducts");
   add_uniform(instructions, symtab, "gl_FrontLightModelProduct",
	       light_model_products_type);
   add_uniform(instructions, symtab, "gl_BackLightModelProduct",
	       light_model_products_type);

   const glsl_type *const light_products_type =
      glsl_type::get_array_instance(symtab->get_type("gl_LightProducts"),
				    state->Const.MaxLights);
   add_uniform(instructions, symtab, "gl_FrontLightProduct", light_products_type);
   add_uniform(instructions, symtab, "gl_BackLightProduct", light_products_type);

   add_uniform(instructions, symtab, "gl_TextureEnvColor",
	       glsl_type::get_array_instance(glsl_type::vec4_type,
					     state->Const.MaxTextureUnits));

   const glsl_type *const texcoords_vec4 =
      glsl_type::get_array_instance(glsl_type::vec4_type,
				    state->Const.MaxTextureCoords);
   add_uniform(instructions, symtab, "gl_EyePlaneS", texcoords_vec4);
   add_uniform(instructions, symtab, "gl_EyePlaneT", texcoords_vec4);
   add_uniform(instructions, symtab, "gl_EyePlaneR", texcoords_vec4);
   add_uniform(instructions, symtab, "gl_EyePlaneQ", texcoords_vec4);
   add_uniform(instructions, symtab, "gl_ObjectPlaneS", texcoords_vec4);
   add_uniform(instructions, symtab, "gl_ObjectPlaneT", texcoords_vec4);
   add_uniform(instructions, symtab, "gl_ObjectPlaneR", texcoords_vec4);
   add_uniform(instructions, symtab, "gl_ObjectPlaneQ", texcoords_vec4);

   add_uniform(instructions, symtab, "gl_Fog",
	       symtab->get_type("gl_FogParameters"));

   /* Mesa-internal current attrib state */
   const glsl_type *const vert_attribs =
      glsl_type::get_array_instance(glsl_type::vec4_type, VERT_ATTRIB_MAX);
   add_uniform(instructions, symtab, "gl_CurrentAttribVertMESA", vert_attribs);
   const glsl_type *const frag_attribs =
      glsl_type::get_array_instance(glsl_type::vec4_type, FRAG_ATTRIB_MAX);
   add_uniform(instructions, symtab, "gl_CurrentAttribFragMESA", frag_attribs);
}

/* This function should only be called for ES, not desktop GL. */
static void
generate_100ES_vs_variables(exec_list *instructions,
			  struct _mesa_glsl_parse_state *state)
{
   for (unsigned i = 0; i < Elements(builtin_core_vs_variables); i++) {
      add_builtin_variable(instructions, state->symbols,
			   & builtin_core_vs_variables[i]);
   }

   generate_100ES_uniforms(instructions, state);

   generate_ARB_draw_buffers_variables(instructions, state, false,
				       vertex_shader);
}


static void
generate_110_vs_variables(exec_list *instructions,
			  struct _mesa_glsl_parse_state *state)
{
   for (unsigned i = 0; i < Elements(builtin_core_vs_variables); i++) {
      add_builtin_variable(instructions, state->symbols,
			   & builtin_core_vs_variables[i]);
   }

   for (unsigned i = 0
	   ; i < Elements(builtin_110_deprecated_vs_variables)
	   ; i++) {
      add_builtin_variable(instructions, state->symbols,
			   & builtin_110_deprecated_vs_variables[i]);
   }
   generate_110_uniforms(instructions, state);

   /* From page 54 (page 60 of the PDF) of the GLSL 1.20 spec:
    *
    *     "As with all arrays, indices used to subscript gl_TexCoord must
    *     either be an integral constant expressions, or this array must be
    *     re-declared by the shader with a size. The size can be at most
    *     gl_MaxTextureCoords. Using indexes close to 0 may aid the
    *     implementation in preserving varying resources."
    */
   const glsl_type *const vec4_array_type =
      glsl_type::get_array_instance(glsl_type::vec4_type, 0);

   add_variable(instructions, state->symbols,
		"gl_TexCoord", vec4_array_type, ir_var_out, VERT_RESULT_TEX0);

   generate_ARB_draw_buffers_variables(instructions, state, false,
				       vertex_shader);
}


static void
generate_120_vs_variables(exec_list *instructions,
			  struct _mesa_glsl_parse_state *state)
{
   /* GLSL version 1.20 did not add any built-in variables in the vertex
    * shader.
    */
   generate_110_vs_variables(instructions, state);
}


static void
generate_130_uniforms(exec_list *instructions,
		      struct _mesa_glsl_parse_state *state)
{
   glsl_symbol_table *const symtab = state->symbols;

   add_builtin_constant(instructions, symtab, "gl_MaxClipDistances",
                        state->Const.MaxClipPlanes);
   add_builtin_constant(instructions, symtab, "gl_MaxVaryingComponents",
			state->Const.MaxVaryingFloats);
}


static void
generate_130_vs_variables(exec_list *instructions,
			  struct _mesa_glsl_parse_state *state)
{
   generate_120_vs_variables(instructions, state);

   for (unsigned i = 0; i < Elements(builtin_130_vs_variables); i++) {
      add_builtin_variable(instructions, state->symbols,
			   & builtin_130_vs_variables[i]);
   }

   generate_130_uniforms(instructions, state);

   /* From the GLSL 1.30 spec, section 7.1 (Vertex Shader Special
    * Variables):
    *
    *   The gl_ClipDistance array is predeclared as unsized and must
    *   be sized by the shader either redeclaring it with a size or
    *   indexing it only with integral constant expressions.
    *
    * We represent this in Mesa by initially declaring the array as
    * size 0.
    */
   const glsl_type *const clip_distance_array_type =
      glsl_type::get_array_instance(glsl_type::float_type, 0);

   add_variable(instructions, state->symbols,
		"gl_ClipDistance", clip_distance_array_type, ir_var_out,
                VERT_RESULT_CLIP_DIST0);

}


static void
initialize_vs_variables(exec_list *instructions,
			struct _mesa_glsl_parse_state *state)
{

   switch (state->language_version) {
   case 100:
      generate_100ES_vs_variables(instructions, state);
      break;
   case 110:
      generate_110_vs_variables(instructions, state);
      break;
   case 120:
      generate_120_vs_variables(instructions, state);
      break;
   case 130:
      generate_130_vs_variables(instructions, state);
      break;
   }

   if (state->ARB_draw_instanced_enable)
      generate_ARB_draw_instanced_variables(instructions, state, false,
                                            vertex_shader);
}


/* This function should only be called for ES, not desktop GL. */
static void
generate_100ES_fs_variables(exec_list *instructions,
			  struct _mesa_glsl_parse_state *state)
{
   for (unsigned i = 0; i < Elements(builtin_core_fs_variables); i++) {
      add_builtin_variable(instructions, state->symbols,
			   & builtin_core_fs_variables[i]);
   }

   for (unsigned i = 0; i < Elements(builtin_100ES_fs_variables); i++) {
      add_builtin_variable(instructions, state->symbols,
			   & builtin_100ES_fs_variables[i]);
   }

   generate_100ES_uniforms(instructions, state);

   generate_ARB_draw_buffers_variables(instructions, state, false,
				       fragment_shader);
}

static void
generate_110_fs_variables(exec_list *instructions,
			  struct _mesa_glsl_parse_state *state)
{
   for (unsigned i = 0; i < Elements(builtin_core_fs_variables); i++) {
      add_builtin_variable(instructions, state->symbols,
			   & builtin_core_fs_variables[i]);
   }

   for (unsigned i = 0; i < Elements(builtin_110_fs_variables); i++) {
      add_builtin_variable(instructions, state->symbols,
			   & builtin_110_fs_variables[i]);
   }

   for (unsigned i = 0
	   ; i < Elements(builtin_110_deprecated_fs_variables)
	   ; i++) {
      add_builtin_variable(instructions, state->symbols,
			   & builtin_110_deprecated_fs_variables[i]);
   }
   generate_110_uniforms(instructions, state);

   /* From page 54 (page 60 of the PDF) of the GLSL 1.20 spec:
    *
    *     "As with all arrays, indices used to subscript gl_TexCoord must
    *     either be an integral constant expressions, or this array must be
    *     re-declared by the shader with a size. The size can be at most
    *     gl_MaxTextureCoords. Using indexes close to 0 may aid the
    *     implementation in preserving varying resources."
    */
   const glsl_type *const vec4_array_type =
      glsl_type::get_array_instance(glsl_type::vec4_type, 0);

   add_variable(instructions, state->symbols,
		"gl_TexCoord", vec4_array_type, ir_var_in, FRAG_ATTRIB_TEX0);

   generate_ARB_draw_buffers_variables(instructions, state, false,
				       fragment_shader);
}


static void
generate_ARB_draw_buffers_variables(exec_list *instructions,
				    struct _mesa_glsl_parse_state *state,
				    bool warn, _mesa_glsl_parser_targets target)
{
   /* gl_MaxDrawBuffers is available in all shader stages.
    */
   ir_variable *const mdb =
      add_builtin_constant(instructions, state->symbols, "gl_MaxDrawBuffers",
			   state->Const.MaxDrawBuffers);

   if (warn)
      mdb->warn_extension = "GL_ARB_draw_buffers";

   /* gl_FragData is only available in the fragment shader.
    */
   if (target == fragment_shader) {
      const glsl_type *const vec4_array_type =
	 glsl_type::get_array_instance(glsl_type::vec4_type,
				       state->Const.MaxDrawBuffers);

      ir_variable *const fd =
	 add_variable(instructions, state->symbols,
		      "gl_FragData", vec4_array_type,
		      ir_var_out, FRAG_RESULT_DATA0);

      if (warn)
	 fd->warn_extension = "GL_ARB_draw_buffers";
   }
}


static void
generate_ARB_draw_instanced_variables(exec_list *instructions,
                                      struct _mesa_glsl_parse_state *state,
                                      bool warn,
                                      _mesa_glsl_parser_targets target)
{
   /* gl_InstanceIDARB is only available in the vertex shader.
    */
   if (target == vertex_shader) {
      ir_variable *const inst =
         add_variable(instructions, state->symbols,
		      "gl_InstanceIDARB", glsl_type::int_type,
		      ir_var_system_value, SYSTEM_VALUE_INSTANCE_ID);

      if (warn)
         inst->warn_extension = "GL_ARB_draw_instanced";
   }
}


static void
generate_ARB_shader_stencil_export_variables(exec_list *instructions,
					     struct _mesa_glsl_parse_state *state,
					     bool warn)
{
   /* gl_FragStencilRefARB is only available in the fragment shader.
    */
   ir_variable *const fd =
      add_variable(instructions, state->symbols,
		   "gl_FragStencilRefARB", glsl_type::int_type,
		   ir_var_out, FRAG_RESULT_STENCIL);

   if (warn)
      fd->warn_extension = "GL_ARB_shader_stencil_export";
}

static void
generate_AMD_shader_stencil_export_variables(exec_list *instructions,
					     struct _mesa_glsl_parse_state *state,
					     bool warn)
{
   /* gl_FragStencilRefAMD is only available in the fragment shader.
    */
   ir_variable *const fd =
      add_variable(instructions, state->symbols,
		   "gl_FragStencilRefAMD", glsl_type::int_type,
		   ir_var_out, FRAG_RESULT_STENCIL);

   if (warn)
      fd->warn_extension = "GL_AMD_shader_stencil_export";
}

static void
generate_120_fs_variables(exec_list *instructions,
			  struct _mesa_glsl_parse_state *state)
{
   generate_110_fs_variables(instructions, state);

   for (unsigned i = 0
	   ; i < Elements(builtin_120_fs_variables)
	   ; i++) {
      add_builtin_variable(instructions, state->symbols,
			   & builtin_120_fs_variables[i]);
   }
}

static void
generate_130_fs_variables(exec_list *instructions,
			  struct _mesa_glsl_parse_state *state)
{
   generate_120_fs_variables(instructions, state);

   generate_130_uniforms(instructions, state);

   /* From the GLSL 1.30 spec, section 7.2 (Fragment Shader Special
    * Variables):
    *
    *   The built-in input variable gl_ClipDistance array contains linearly
    *   interpolated values for the vertex values written by the vertex shader
    *   to the gl_ClipDistance vertex output variable. This array must be
    *   sized in the fragment shader either implicitly or explicitly to be the
    *   same size as it was sized in the vertex shader.
    *
    * In other words, the array must be pre-declared as implicitly sized.  We
    * represent this in Mesa by initially declaring the array as size 0.
    */
   const glsl_type *const clip_distance_array_type =
      glsl_type::get_array_instance(glsl_type::float_type, 0);

   add_variable(instructions, state->symbols,
		"gl_ClipDistance", clip_distance_array_type, ir_var_in,
                FRAG_ATTRIB_CLIP_DIST0);
}

static void
initialize_fs_variables(exec_list *instructions,
			struct _mesa_glsl_parse_state *state)
{

   switch (state->language_version) {
   case 100:
      generate_100ES_fs_variables(instructions, state);
      break;
   case 110:
      generate_110_fs_variables(instructions, state);
      break;
   case 120:
      generate_120_fs_variables(instructions, state);
      break;
   case 130:
      generate_130_fs_variables(instructions, state);
      break;
   }

   if (state->ARB_shader_stencil_export_enable)
      generate_ARB_shader_stencil_export_variables(instructions, state,
						   state->ARB_shader_stencil_export_warn);

   if (state->AMD_shader_stencil_export_enable)
      generate_AMD_shader_stencil_export_variables(instructions, state,
						   state->AMD_shader_stencil_export_warn);
}

void
_mesa_glsl_initialize_variables(exec_list *instructions,
				struct _mesa_glsl_parse_state *state)
{
   switch (state->target) {
   case vertex_shader:
      initialize_vs_variables(instructions, state);
      break;
   case geometry_shader:
      break;
   case fragment_shader:
      initialize_fs_variables(instructions, state);
      break;
   }
}
