/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
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

      /* XXX verify these!!! */
      { "gl_NormalMatrix", STATE_MODELVIEW_MATRIX, STATE_MATRIX_TRANSPOSE },
      { "__NormalMatrixTranspose", STATE_MODELVIEW_MATRIX, 0 },

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
            tokens[1] = index1;
            index1 = 0; /* prevent extra addition at end of function */
         }
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
      tokens[0] = STATE_TEXENV_COLOR;
      tokens[1] = index1;
   }
   else if (strcmp(var, "gl_EyePlaneS") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_EYE_S;
   }
   else if (strcmp(var, "gl_EyePlaneT") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_EYE_T;
   }
   else if (strcmp(var, "gl_EyePlaneR") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_EYE_R;
   }
   else if (strcmp(var, "gl_EyePlaneQ") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_EYE_Q;
   }
   else if (strcmp(var, "gl_ObjectPlaneS") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_OBJECT_S;
   }
   else if (strcmp(var, "gl_ObjectPlaneT") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_OBJECT_T;
   }
   else if (strcmp(var, "gl_ObjectPlaneR") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_OBJECT_R;
   }
   else if (strcmp(var, "gl_ObjectPlaneQ") == 0) {
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
 * Allocate storage for a pre-defined uniform (a GL state variable).
 * As a memory-saving optimization, we try to only allocate storage for
 * state vars that are actually used.
 * For example, the "gl_LightSource" uniform is huge.  If we only use
 * a handful of gl_LightSource fields, we don't want to allocate storage
 * for all of gl_LightSource.
 *
 * Currently, all pre-defined uniforms are in one of these forms:
 *   var
 *   var[i]
 *   var.field
 *   var[i].field
 *   var[i][j]
 *
 * \return -1 upon error, else position in paramList of the state var/data
 */
GLint
_slang_alloc_statevar(slang_ir_node *n,
                      struct gl_program_parameter_list *paramList)
{
   slang_ir_node *n0 = n;
   const char *field = NULL, *var;
   GLint index1 = -1, index2 = -1, pos;
   GLuint swizzle;

   if (n->Opcode == IR_FIELD) {
      field = n->Field;
      n = n->Children[0];
   }

   if (n->Opcode == IR_ELEMENT) {
      /* XXX can only handle constant indexes for now */
      if (n->Children[1]->Opcode == IR_FLOAT) {
         index1 = (GLint) n->Children[1]->Value[0];
         n = n->Children[0];
      }
      else {
         return -1;
      }
   }

   if (n->Opcode == IR_ELEMENT) {
      /* XXX can only handle constant indexes for now */
      assert(n->Children[1]->Opcode == IR_FLOAT);
      index2 = (GLint) n->Children[1]->Value[0];
      n = n->Children[0];
   }

   assert(n->Opcode == IR_VAR);
   var = (char *) n->Var->a_name;

   pos = lookup_statevar(var, index1, index2, field, &swizzle, paramList);
   assert(pos >= 0);
   if (pos >= 0) {
      /* newly resolved storage for the statevar/constant/uniform */
      n0->Store->File = PROGRAM_STATE_VAR;
      n0->Store->Index = pos;
      n0->Store->Swizzle = swizzle;
      n0->Store->Parent = NULL;
   }
   return pos;
}

