/*
 * Mesa 3-D graphics library
 * Version:  7.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
 * Copyright (C) 2008  VMware, Inc.   All Rights Reserved.
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
 * \file slang_builtin.c
 * Resolve built-in uniform vars.
 * \author Brian Paul
 */

#include "main/imports.h"
#include "main/mtypes.h"
#include "shader/program.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_statevars.h"
#include "shader/slang/slang_ir.h"
#include "shader/slang/slang_emit.h"
#include "shader/slang/slang_builtin.h"


/** special state token (see below) */
#define STATE_ARRAY ((gl_state_index) 0xfffff)


/**
 * Lookup GL state given a variable name, 0, 1 or 2 indexes and a field.
 * Allocate room for the state in the given param list and return position
 * in the list.
 * Yes, this is kind of ugly, but it works.
 */
static GLint
lookup_statevar(const char *var, GLint index1, GLint index2, const char *field,
                GLuint *swizzleOut,
                struct gl_program_parameter_list *paramList)
{
   /*
    * NOTE: The ARB_vertex_program extension specified that matrices get
    * loaded in registers in row-major order.  With GLSL, we want column-
    * major order.  So, we need to transpose all matrices here...
    */
   static const struct {
      const char *name;
      gl_state_index matrix;
      gl_state_index modifier;
   } matrices[] = {
      { "gl_ModelViewMatrix", STATE_MODELVIEW_MATRIX, STATE_MATRIX_TRANSPOSE },
      { "gl_ModelViewMatrixInverse", STATE_MODELVIEW_MATRIX, STATE_MATRIX_INVTRANS },
      { "gl_ModelViewMatrixTranspose", STATE_MODELVIEW_MATRIX, 0 },
      { "gl_ModelViewMatrixInverseTranspose", STATE_MODELVIEW_MATRIX, STATE_MATRIX_INVERSE },

      { "gl_ProjectionMatrix", STATE_PROJECTION_MATRIX, STATE_MATRIX_TRANSPOSE },
      { "gl_ProjectionMatrixInverse", STATE_PROJECTION_MATRIX, STATE_MATRIX_INVTRANS },
      { "gl_ProjectionMatrixTranspose", STATE_PROJECTION_MATRIX, 0 },
      { "gl_ProjectionMatrixInverseTranspose", STATE_PROJECTION_MATRIX, STATE_MATRIX_INVERSE },

      { "gl_ModelViewProjectionMatrix", STATE_MVP_MATRIX, STATE_MATRIX_TRANSPOSE },
      { "gl_ModelViewProjectionMatrixInverse", STATE_MVP_MATRIX, STATE_MATRIX_INVTRANS },
      { "gl_ModelViewProjectionMatrixTranspose", STATE_MVP_MATRIX, 0 },
      { "gl_ModelViewProjectionMatrixInverseTranspose", STATE_MVP_MATRIX, STATE_MATRIX_INVERSE },

      { "gl_TextureMatrix", STATE_TEXTURE_MATRIX, STATE_MATRIX_TRANSPOSE },
      { "gl_TextureMatrixInverse", STATE_TEXTURE_MATRIX, STATE_MATRIX_INVTRANS },
      { "gl_TextureMatrixTranspose", STATE_TEXTURE_MATRIX, 0 },
      { "gl_TextureMatrixInverseTranspose", STATE_TEXTURE_MATRIX, STATE_MATRIX_INVERSE },

      { "gl_NormalMatrix", STATE_MODELVIEW_MATRIX, STATE_MATRIX_TRANSPOSE },

      { NULL, 0, 0 }
   };
   gl_state_index tokens[STATE_LENGTH];
   GLuint i;
   GLboolean isMatrix = GL_FALSE;

   for (i = 0; i < STATE_LENGTH; i++) {
      tokens[i] = 0;
   }
   *swizzleOut = SWIZZLE_NOOP;

   /* first, look if var is a pre-defined matrix */
   for (i = 0; matrices[i].name; i++) {
      if (strcmp(var, matrices[i].name) == 0) {
         tokens[0] = matrices[i].matrix;
         /* tokens[1], [2] and [3] filled below */
         tokens[4] = matrices[i].modifier;
         isMatrix = GL_TRUE;
         break;
      }
   }

   if (isMatrix) {
      if (tokens[0] == STATE_TEXTURE_MATRIX) {
         if (index1 >= 0) {
            tokens[1] = index1; /* which texture matrix */
            index1 = 0; /* prevent extra addition at end of function */
         }
      }
      if (index1 < 0) {
         /* index1 is unused: prevent extra addition at end of function */
         index1 = 0;
      }
   }
   else if (strcmp(var, "gl_DepthRange") == 0) {
      tokens[0] = STATE_DEPTH_RANGE;
      if (strcmp(field, "near") == 0) {
         *swizzleOut = SWIZZLE_XXXX;
      }
      else if (strcmp(field, "far") == 0) {
         *swizzleOut = SWIZZLE_YYYY;
      }
      else if (strcmp(field, "diff") == 0) {
         *swizzleOut = SWIZZLE_ZZZZ;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_ClipPlane") == 0) {
      if (index1 < 0)
         return -1;
      tokens[0] = STATE_CLIPPLANE;
      tokens[1] = index1;
   }
   else if (strcmp(var, "gl_Point") == 0) {
      if (strcmp(field, "size") == 0) {
         tokens[0] = STATE_POINT_SIZE;
         *swizzleOut = SWIZZLE_XXXX;
      }
      else if (strcmp(field, "sizeMin") == 0) {
         tokens[0] = STATE_POINT_SIZE;
         *swizzleOut = SWIZZLE_YYYY;
      }
      else if (strcmp(field, "sizeMax") == 0) {
         tokens[0] = STATE_POINT_SIZE;
         *swizzleOut = SWIZZLE_ZZZZ;
      }
      else if (strcmp(field, "fadeThresholdSize") == 0) {
         tokens[0] = STATE_POINT_SIZE;
         *swizzleOut = SWIZZLE_WWWW;
      }
      else if (strcmp(field, "distanceConstantAttenuation") == 0) {
         tokens[0] = STATE_POINT_ATTENUATION;
         *swizzleOut = SWIZZLE_XXXX;
      }
      else if (strcmp(field, "distanceLinearAttenuation") == 0) {
         tokens[0] = STATE_POINT_ATTENUATION;
         *swizzleOut = SWIZZLE_YYYY;
      }
      else if (strcmp(field, "distanceQuadraticAttenuation") == 0) {
         tokens[0] = STATE_POINT_ATTENUATION;
         *swizzleOut = SWIZZLE_ZZZZ;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_FrontMaterial") == 0 ||
            strcmp(var, "gl_BackMaterial") == 0) {
      tokens[0] = STATE_MATERIAL;
      if (strcmp(var, "gl_FrontMaterial") == 0)
         tokens[1] = 0;
      else
         tokens[1] = 1;
      if (strcmp(field, "emission") == 0) {
         tokens[2] = STATE_EMISSION;
      }
      else if (strcmp(field, "ambient") == 0) {
         tokens[2] = STATE_AMBIENT;
      }
      else if (strcmp(field, "diffuse") == 0) {
         tokens[2] = STATE_DIFFUSE;
      }
      else if (strcmp(field, "specular") == 0) {
         tokens[2] = STATE_SPECULAR;
      }
      else if (strcmp(field, "shininess") == 0) {
         tokens[2] = STATE_SHININESS;
         *swizzleOut = SWIZZLE_XXXX;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_LightSource") == 0) {
      if (!field || index1 < 0)
         return -1;

      tokens[0] = STATE_LIGHT;
      tokens[1] = index1;

      if (strcmp(field, "ambient") == 0) {
         tokens[2] = STATE_AMBIENT;
      }
      else if (strcmp(field, "diffuse") == 0) {
         tokens[2] = STATE_DIFFUSE;
      }
      else if (strcmp(field, "specular") == 0) {
         tokens[2] = STATE_SPECULAR;
      }
      else if (strcmp(field, "position") == 0) {
         tokens[2] = STATE_POSITION;
      }
      else if (strcmp(field, "halfVector") == 0) {
         tokens[2] = STATE_HALF_VECTOR;
      }
      else if (strcmp(field, "spotDirection") == 0) {
         tokens[2] = STATE_SPOT_DIRECTION;
      }
      else if (strcmp(field, "spotCosCutoff") == 0) {
         tokens[2] = STATE_SPOT_DIRECTION;
         *swizzleOut = SWIZZLE_WWWW;
      }
      else if (strcmp(field, "spotCutoff") == 0) {
         tokens[2] = STATE_SPOT_CUTOFF;
         *swizzleOut = SWIZZLE_XXXX;
      }
      else if (strcmp(field, "spotExponent") == 0) {
         tokens[2] = STATE_ATTENUATION;
         *swizzleOut = SWIZZLE_WWWW;
      }
      else if (strcmp(field, "constantAttenuation") == 0) {
         tokens[2] = STATE_ATTENUATION;
         *swizzleOut = SWIZZLE_XXXX;
      }
      else if (strcmp(field, "linearAttenuation") == 0) {
         tokens[2] = STATE_ATTENUATION;
         *swizzleOut = SWIZZLE_YYYY;
      }
      else if (strcmp(field, "quadraticAttenuation") == 0) {
         tokens[2] = STATE_ATTENUATION;
         *swizzleOut = SWIZZLE_ZZZZ;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_LightModel") == 0) {
      if (strcmp(field, "ambient") == 0) {
         tokens[0] = STATE_LIGHTMODEL_AMBIENT;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_FrontLightModelProduct") == 0) {
      if (strcmp(field, "sceneColor") == 0) {
         tokens[0] = STATE_LIGHTMODEL_SCENECOLOR;
         tokens[1] = 0;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_BackLightModelProduct") == 0) {
      if (strcmp(field, "sceneColor") == 0) {
         tokens[0] = STATE_LIGHTMODEL_SCENECOLOR;
         tokens[1] = 1;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_FrontLightProduct") == 0 ||
            strcmp(var, "gl_BackLightProduct") == 0) {
      if (index1 < 0 || !field)
         return -1;

      tokens[0] = STATE_LIGHTPROD;
      tokens[1] = index1; /* light number */
      if (strcmp(var, "gl_FrontLightProduct") == 0) {
         tokens[2] = 0; /* front */
      }
      else {
         tokens[2] = 1; /* back */
      }
      if (strcmp(field, "ambient") == 0) {
         tokens[3] = STATE_AMBIENT;
      }
      else if (strcmp(field, "diffuse") == 0) {
         tokens[3] = STATE_DIFFUSE;
      }
      else if (strcmp(field, "specular") == 0) {
         tokens[3] = STATE_SPECULAR;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_TextureEnvColor") == 0) {
      if (index1 < 0)
         return -1;
      tokens[0] = STATE_TEXENV_COLOR;
      tokens[1] = index1;
   }
   else if (strcmp(var, "gl_EyePlaneS") == 0) {
      if (index1 < 0)
         return -1;
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_EYE_S;
   }
   else if (strcmp(var, "gl_EyePlaneT") == 0) {
      if (index1 < 0)
         return -1;
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_EYE_T;
   }
   else if (strcmp(var, "gl_EyePlaneR") == 0) {
      if (index1 < 0)
         return -1;
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_EYE_R;
   }
   else if (strcmp(var, "gl_EyePlaneQ") == 0) {
      if (index1 < 0)
         return -1;
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_EYE_Q;
   }
   else if (strcmp(var, "gl_ObjectPlaneS") == 0) {
      if (index1 < 0)
         return -1;
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_OBJECT_S;
   }
   else if (strcmp(var, "gl_ObjectPlaneT") == 0) {
      if (index1 < 0)
         return -1;
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_OBJECT_T;
   }
   else if (strcmp(var, "gl_ObjectPlaneR") == 0) {
      if (index1 < 0)
         return -1;
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_OBJECT_R;
   }
   else if (strcmp(var, "gl_ObjectPlaneQ") == 0) {
      if (index1 < 0)
         return -1;
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_OBJECT_Q;
   }
   else if (strcmp(var, "gl_Fog") == 0) {
      if (strcmp(field, "color") == 0) {
         tokens[0] = STATE_FOG_COLOR;
      }
      else if (strcmp(field, "density") == 0) {
         tokens[0] = STATE_FOG_PARAMS;
         *swizzleOut = SWIZZLE_XXXX;
      }
      else if (strcmp(field, "start") == 0) {
         tokens[0] = STATE_FOG_PARAMS;
         *swizzleOut = SWIZZLE_YYYY;
      }
      else if (strcmp(field, "end") == 0) {
         tokens[0] = STATE_FOG_PARAMS;
         *swizzleOut = SWIZZLE_ZZZZ;
      }
      else if (strcmp(field, "scale") == 0) {
         tokens[0] = STATE_FOG_PARAMS;
         *swizzleOut = SWIZZLE_WWWW;
      }
      else {
         return -1;
      }
   }
   else {
      return -1;
   }

   if (isMatrix) {
      /* load all four columns of matrix */
      GLint pos[4];
      GLuint j;
      for (j = 0; j < 4; j++) {
         tokens[2] = tokens[3] = j; /* jth row of matrix */
         pos[j] = _mesa_add_state_reference(paramList, tokens);
         assert(pos[j] >= 0);
         ASSERT(pos[j] >= 0);
      }
      return pos[0] + index1;
   }
   else {
      /* allocate a single register */
      GLint pos = _mesa_add_state_reference(paramList, tokens);
      ASSERT(pos >= 0);
      return pos;
   }
}



/**
 * Given a variable name and datatype, emit uniform/constant buffer
 * entries which will store that state variable.
 * For example, if name="gl_LightSource" we'll emit 64 state variable
 * vectors/references and return position where that data starts.  This will
 * allow run-time array indexing into the light source array.
 *
 * Note that this is a recursive function.
 *
 * \return -1 if error, else index of start of data in the program parameter list
 */
static GLint
emit_statevars(const char *name, int array_len,
               const slang_type_specifier *type,
               gl_state_index tokens[STATE_LENGTH],
               struct gl_program_parameter_list *paramList)
{
   if (type->type == SLANG_SPEC_ARRAY) {
      GLint i, pos;
      assert(array_len > 0);
      if (strcmp(name, "gl_ClipPlane") == 0) {
         tokens[0] = STATE_CLIPPLANE;
      }
      else if (strcmp(name, "gl_LightSource") == 0) {
         tokens[0] = STATE_LIGHT;
      }
      else if (strcmp(name, "gl_FrontLightProduct") == 0) {
         tokens[0] = STATE_LIGHTPROD;
         tokens[2] = 0; /* front */
      }
      else if (strcmp(name, "gl_BackLightProduct") == 0) {
         tokens[0] = STATE_LIGHTPROD;
         tokens[2] = 1; /* back */
      }
      else if (strcmp(name, "gl_TextureEnvColor") == 0) {
         tokens[0] = STATE_TEXENV_COLOR;
      }
      else if (strcmp(name, "gl_EyePlaneS") == 0) {
         tokens[0] = STATE_TEXGEN_EYE_S;
      }
      else if (strcmp(name, "gl_EyePlaneT") == 0) {
         tokens[0] = STATE_TEXGEN_EYE_T;
      }
      else if (strcmp(name, "gl_EyePlaneR") == 0) {
         tokens[0] = STATE_TEXGEN_EYE_R;
      }
      else if (strcmp(name, "gl_EyePlaneQ") == 0) {
         tokens[0] = STATE_TEXGEN_EYE_Q;
      }
      else if (strcmp(name, "gl_ObjectPlaneS") == 0) {
         tokens[0] = STATE_TEXGEN_OBJECT_S;
      }
      else if (strcmp(name, "gl_ObjectPlaneT") == 0) {
         tokens[0] = STATE_TEXGEN_OBJECT_T;
      }
      else if (strcmp(name, "gl_ObjectPlaneR") == 0) {
         tokens[0] = STATE_TEXGEN_OBJECT_R;
      }
      else if (strcmp(name, "gl_ObjectPlaneQ") == 0) {
         tokens[0] = STATE_TEXGEN_OBJECT_Q;
      }
      else {
         return -1; /* invalid array name */
      }
      for (i = 0; i < array_len; i++) {
         GLint p;
         tokens[1] = i;
         p = emit_statevars(NULL, 0, type->_array, tokens, paramList);
         if (i == 0)
            pos = p;
      }
      return pos;
   }
   else if (type->type == SLANG_SPEC_STRUCT) {
      const slang_variable_scope *fields = type->_struct->fields;
      GLuint i, pos;
      for (i = 0; i < fields->num_variables; i++) {
         const slang_variable *var = fields->variables[i];
         GLint p = emit_statevars(var->a_name, 0, &var->type.specifier,
                                  tokens, paramList);
         if (i == 0)
            pos = p;
      }
      return pos;
   }
   else {
      GLint pos;
      assert(type->type == SLANG_SPEC_VEC4 ||
             type->type == SLANG_SPEC_VEC3 ||
             type->type == SLANG_SPEC_VEC2 ||
             type->type == SLANG_SPEC_FLOAT ||
             type->type == SLANG_SPEC_IVEC4 ||
             type->type == SLANG_SPEC_IVEC3 ||
             type->type == SLANG_SPEC_IVEC2 ||
             type->type == SLANG_SPEC_INT);
      if (name) {
         GLint t;

         if (tokens[0] == STATE_LIGHT)
            t = 2;
         else if (tokens[0] == STATE_LIGHTPROD)
            t = 3;
         else
            return -1; /* invalid array name */

         if (strcmp(name, "ambient") == 0) {
            tokens[t] = STATE_AMBIENT;
         }
         else if (strcmp(name, "diffuse") == 0) {
            tokens[t] = STATE_DIFFUSE;
         }
         else if (strcmp(name, "specular") == 0) {
            tokens[t] = STATE_SPECULAR;
         }
         else if (strcmp(name, "position") == 0) {
            tokens[t] = STATE_POSITION;
         }
         else if (strcmp(name, "halfVector") == 0) {
            tokens[t] = STATE_HALF_VECTOR;
         }
         else if (strcmp(name, "spotDirection") == 0) {
            tokens[t] = STATE_SPOT_DIRECTION; /* xyz components */
         }
         else if (strcmp(name, "spotCosCutoff") == 0) {
            tokens[t] = STATE_SPOT_DIRECTION; /* w component */
         }

         else if (strcmp(name, "constantAttenuation") == 0) {
            tokens[t] = STATE_ATTENUATION; /* x component */
         }
         else if (strcmp(name, "linearAttenuation") == 0) {
            tokens[t] = STATE_ATTENUATION; /* y component */
         }
         else if (strcmp(name, "quadraticAttenuation") == 0) {
            tokens[t] = STATE_ATTENUATION; /* z component */
         }
         else if (strcmp(name, "spotExponent") == 0) {
            tokens[t] = STATE_ATTENUATION; /* w = spot exponent */
         }

         else if (strcmp(name, "spotCutoff") == 0) {
            tokens[t] = STATE_SPOT_CUTOFF; /* x component */
         }

         else {
            return -1; /* invalid field name */
         }
      }

      pos = _mesa_add_state_reference(paramList, tokens);
      return pos;
   }

   return 1;
}


/**
 * Unroll the named built-in uniform variable into a sequence of state
 * vars in the given parameter list.
 */
static GLint
alloc_state_var_array(const slang_variable *var,
                      struct gl_program_parameter_list *paramList)
{
   gl_state_index tokens[STATE_LENGTH];
   GLuint i;
   GLint pos;

   /* Initialize the state tokens array.  This is very important.
    * When we call _mesa_add_state_reference() it'll searches the parameter
    * list to see if the given statevar token sequence is already present.
    * This is normally a good thing since it prevents redundant values in the
    * constant buffer.
    *
    * But when we're building arrays of state this can be bad.  For example,
    * consider this fragment of GLSL code:
    *   foo = gl_LightSource[3].diffuse;
    *   ...
    *   bar = gl_LightSource[i].diffuse;
    *
    * When we unroll the gl_LightSource array (for "bar") we want to re-emit
    * gl_LightSource[3].diffuse and not re-use the first instance (from "foo")
    * since that would upset the array layout.  We handle this situation by
    * setting the last token in the state var token array to the special
    * value STATE_ARRAY.
    * This token will only be set for array state.  We can hijack the last
    * element in the array for this since it's never used for light, clipplane
    * or texture env array state.
    */
   for (i = 0; i < STATE_LENGTH; i++)
      tokens[i] = 0;
   tokens[STATE_LENGTH - 1] = STATE_ARRAY;

   pos = emit_statevars(var->a_name, var->array_len, &var->type.specifier,
                        tokens, paramList);

   return pos;
}



/**
 * Allocate storage for a pre-defined uniform (a GL state variable).
 * As a memory-saving optimization, we try to only allocate storage for
 * state vars that are actually used.
 *
 * Arrays such as gl_LightSource are handled specially.  For an expression
 * like "gl_LightSource[2].diffuse", we can allocate a single uniform/constant
 * slot and return the index.  In this case, we return direct=TRUE.
 *
 * Buf for something like "gl_LightSource[i].diffuse" we don't know the value
 * of 'i' at compile time so we need to "unroll" the gl_LightSource array
 * into a consecutive sequence of uniform/constant slots so it can be indexed
 * at runtime.  In this case, we return direct=FALSE.
 *
 * Currently, all pre-defined uniforms are in one of these forms:
 *   var
 *   var[i]
 *   var.field
 *   var[i].field
 *   var[i][j]
 *
 * \return -1 upon error, else position in paramList of the state variable/data
 */
GLint
_slang_alloc_statevar(slang_ir_node *n,
                      struct gl_program_parameter_list *paramList,
                      GLboolean *direct)
{
   slang_ir_node *n0 = n;
   const char *field = NULL;
   GLint index1 = -1, index2 = -1;
   GLuint swizzle;

   *direct = GL_TRUE;

   if (n->Opcode == IR_FIELD) {
      field = n->Field;
      n = n->Children[0];
   }

   if (n->Opcode == IR_ELEMENT) {
      if (n->Children[1]->Opcode == IR_FLOAT) {
         index1 = (GLint) n->Children[1]->Value[0];
      }
      else {
         *direct = GL_FALSE;
      }
      n = n->Children[0];
   }

   if (n->Opcode == IR_ELEMENT) {
      /* XXX can only handle constant indexes for now */
      if (n->Children[1]->Opcode == IR_FLOAT) {
         index2 = (GLint) n->Children[1]->Value[0];
      }
      else {
         *direct = GL_FALSE;
      }
      n = n->Children[0];
   }

   assert(n->Opcode == IR_VAR);

   if (*direct) {
      const char *var = (const char *) n->Var->a_name;
      GLint pos =
         lookup_statevar(var, index1, index2, field, &swizzle, paramList);
      if (pos >= 0) {
         /* newly resolved storage for the statevar/constant/uniform */
         n0->Store->File = PROGRAM_STATE_VAR;
         n0->Store->Index = pos;
         n0->Store->Swizzle = swizzle;
         n0->Store->Parent = NULL;
         return pos;
      }
   }

   *direct = GL_FALSE;
   return alloc_state_var_array(n->Var, paramList);
}
