/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 2005-2008  Brian Paul   All Rights Reserved.
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
 * Functions for constant folding, built-in constant lookup, and function
 * call casting.
 */


#include "main/imports.h"
#include "main/macros.h"
#include "main/get.h"
#include "slang_compile.h"
#include "slang_codegen.h"
#include "slang_simplify.h"
#include "slang_print.h"


#ifndef GL_MAX_FRAGMENT_UNIFORM_VECTORS
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS     0x8DFD
#endif
#ifndef GL_MAX_VERTEX_UNIFORM_VECTORS
#define GL_MAX_VERTEX_UNIFORM_VECTORS       0x8DFB
#endif
#ifndef GL_MAX_VARYING_VECTORS
#define GL_MAX_VARYING_VECTORS              0x8DFC
#endif


/**
 * Lookup the value of named constant, such as gl_MaxLights.
 * \return value of constant, or -1 if unknown
 */
GLint
_slang_lookup_constant(const char *name)
{
   struct constant_info {
      const char *Name;
      const GLenum Token;
   };
   static const struct constant_info info[] = {
      { "gl_MaxClipPlanes", GL_MAX_CLIP_PLANES },
      { "gl_MaxCombinedTextureImageUnits", GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS },
      { "gl_MaxDrawBuffers", GL_MAX_DRAW_BUFFERS },
      { "gl_MaxFragmentUniformComponents", GL_MAX_FRAGMENT_UNIFORM_COMPONENTS },
      { "gl_MaxLights", GL_MAX_LIGHTS },
      { "gl_MaxTextureUnits", GL_MAX_TEXTURE_UNITS },
      { "gl_MaxTextureCoords", GL_MAX_TEXTURE_COORDS },
      { "gl_MaxVertexAttribs", GL_MAX_VERTEX_ATTRIBS },
      { "gl_MaxVertexUniformComponents", GL_MAX_VERTEX_UNIFORM_COMPONENTS },
      { "gl_MaxVaryingFloats", GL_MAX_VARYING_FLOATS },
      { "gl_MaxVertexTextureImageUnits", GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS },
      { "gl_MaxTextureImageUnits", GL_MAX_TEXTURE_IMAGE_UNITS },
#if FEATURE_es2_glsl
      { "gl_MaxVertexUniformVectors", GL_MAX_VERTEX_UNIFORM_VECTORS },
      { "gl_MaxVaryingVectors", GL_MAX_VARYING_VECTORS },
      { "gl_MaxFragmentUniformVectors", GL_MAX_FRAGMENT_UNIFORM_VECTORS },
#endif
      { NULL, 0 }
   };
   GLuint i;

   for (i = 0; info[i].Name; i++) {
      if (strcmp(info[i].Name, name) == 0) {
         /* found */
         GLint value = -1;
         _mesa_GetIntegerv(info[i].Token, &value);
         ASSERT(value >= 0);  /* sanity check that glGetFloatv worked */
         return value;
      }
   }
   return -1;
}


static slang_operation_type
literal_type(slang_operation_type t1, slang_operation_type t2)
{
   if (t1 == SLANG_OPER_LITERAL_FLOAT || t2 == SLANG_OPER_LITERAL_FLOAT)
      return SLANG_OPER_LITERAL_FLOAT;
   else
      return SLANG_OPER_LITERAL_INT;
}


/**
 * Recursively traverse an AST tree, applying simplifications wherever
 * possible.
 * At the least, we do constant folding.  We need to do that much so that
 * compile-time expressions can be evaluated for things like array
 * declarations.  I.e.:  float foo[3 + 5];
 */
void
_slang_simplify(slang_operation *oper,
                const slang_name_space * space,
                slang_atom_pool * atoms)
{
   GLboolean isFloat[4];
   GLboolean isBool[4];
   GLuint i, n;

   if (oper->type == SLANG_OPER_IDENTIFIER) {
      /* see if it's a named constant */
      GLint value = _slang_lookup_constant((char *) oper->a_id);
      /*printf("value[%s] = %d\n", (char*) oper->a_id, value);*/
      if (value >= 0) {
         oper->literal[0] =
         oper->literal[1] =
         oper->literal[2] =
         oper->literal[3] = (GLfloat) value;
         oper->type = SLANG_OPER_LITERAL_INT;
         return;
      }
      /* look for user-defined constant */
      {
         slang_variable *var;
         var = _slang_locate_variable(oper->locals, oper->a_id, GL_TRUE);
         if (var) {
            if (var->type.qualifier == SLANG_QUAL_CONST &&
                var->initializer &&
                (var->initializer->type == SLANG_OPER_LITERAL_INT ||
                 var->initializer->type == SLANG_OPER_LITERAL_FLOAT)) {
               oper->literal[0] = var->initializer->literal[0];
               oper->literal[1] = var->initializer->literal[1];
               oper->literal[2] = var->initializer->literal[2];
               oper->literal[3] = var->initializer->literal[3];
               oper->literal_size = var->initializer->literal_size;
               oper->type = var->initializer->type;
               /*
               printf("value[%s] = %f\n",
                      (char*) oper->a_id, oper->literal[0]);
               */
               return;
            }
         }
      }
   }

   /* first, simplify children */
   for (i = 0; i < oper->num_children; i++) {
      _slang_simplify(&oper->children[i], space, atoms);
   }

   /* examine children */
   n = MIN2(oper->num_children, 4);
   for (i = 0; i < n; i++) {
      isFloat[i] = (oper->children[i].type == SLANG_OPER_LITERAL_FLOAT ||
                   oper->children[i].type == SLANG_OPER_LITERAL_INT);
      isBool[i] = (oper->children[i].type == SLANG_OPER_LITERAL_BOOL);
   }
                              
   if (oper->num_children == 2 && isFloat[0] && isFloat[1]) {
      /* probably simple arithmetic */
      switch (oper->type) {
      case SLANG_OPER_ADD:
         for (i = 0; i < 4; i++) {
            oper->literal[i]
               = oper->children[0].literal[i] + oper->children[1].literal[i];
         }
         oper->literal_size = oper->children[0].literal_size;
         oper->type = literal_type(oper->children[0].type, 
                                   oper->children[1].type);
         slang_operation_destruct(oper);  /* frees unused children */
         return;
      case SLANG_OPER_SUBTRACT:
         for (i = 0; i < 4; i++) {
            oper->literal[i]
               = oper->children[0].literal[i] - oper->children[1].literal[i];
         }
         oper->literal_size = oper->children[0].literal_size;
         oper->type = literal_type(oper->children[0].type, 
                                   oper->children[1].type);
         slang_operation_destruct(oper);
         return;
      case SLANG_OPER_MULTIPLY:
         for (i = 0; i < 4; i++) {
            oper->literal[i]
               = oper->children[0].literal[i] * oper->children[1].literal[i];
         }
         oper->literal_size = oper->children[0].literal_size;
         oper->type = literal_type(oper->children[0].type, 
                                   oper->children[1].type);
         slang_operation_destruct(oper);
         return;
      case SLANG_OPER_DIVIDE:
         for (i = 0; i < 4; i++) {
            oper->literal[i]
               = oper->children[0].literal[i] / oper->children[1].literal[i];
         }
         oper->literal_size = oper->children[0].literal_size;
         oper->type = literal_type(oper->children[0].type, 
                                   oper->children[1].type);
         slang_operation_destruct(oper);
         return;
      default:
         ; /* nothing */
      }
   }

   if (oper->num_children == 1 && isFloat[0]) {
      switch (oper->type) {
      case SLANG_OPER_MINUS:
         for (i = 0; i < 4; i++) {
            oper->literal[i] = -oper->children[0].literal[i];
         }
         oper->literal_size = oper->children[0].literal_size;
         slang_operation_destruct(oper);
         oper->type = SLANG_OPER_LITERAL_FLOAT;
         return;
      case SLANG_OPER_PLUS:
         COPY_4V(oper->literal, oper->children[0].literal);
         oper->literal_size = oper->children[0].literal_size;
         slang_operation_destruct(oper);
         oper->type = SLANG_OPER_LITERAL_FLOAT;
         return;
      default:
         ; /* nothing */
      }
   }

   if (oper->num_children == 2 && isBool[0] && isBool[1]) {
      /* simple boolean expression */
      switch (oper->type) {
      case SLANG_OPER_LOGICALAND:
         for (i = 0; i < 4; i++) {
            const GLint a = oper->children[0].literal[i] ? 1 : 0;
            const GLint b = oper->children[1].literal[i] ? 1 : 0;
            oper->literal[i] = (GLfloat) (a && b);
         }
         oper->literal_size = oper->children[0].literal_size;
         slang_operation_destruct(oper);
         oper->type = SLANG_OPER_LITERAL_BOOL;
         return;
      case SLANG_OPER_LOGICALOR:
         for (i = 0; i < 4; i++) {
            const GLint a = oper->children[0].literal[i] ? 1 : 0;
            const GLint b = oper->children[1].literal[i] ? 1 : 0;
            oper->literal[i] = (GLfloat) (a || b);
         }
         oper->literal_size = oper->children[0].literal_size;
         slang_operation_destruct(oper);
         oper->type = SLANG_OPER_LITERAL_BOOL;
         return;
      case SLANG_OPER_LOGICALXOR:
         for (i = 0; i < 4; i++) {
            const GLint a = oper->children[0].literal[i] ? 1 : 0;
            const GLint b = oper->children[1].literal[i] ? 1 : 0;
            oper->literal[i] = (GLfloat) (a ^ b);
         }
         oper->literal_size = oper->children[0].literal_size;
         slang_operation_destruct(oper);
         oper->type = SLANG_OPER_LITERAL_BOOL;
         return;
      default:
         ; /* nothing */
      }
   }

   if (oper->num_children == 4
       && isFloat[0] && isFloat[1] && isFloat[2] && isFloat[3]) {
      /* vec4(flt, flt, flt, flt) constructor */
      if (oper->type == SLANG_OPER_CALL) {
         if (strcmp((char *) oper->a_id, "vec4") == 0) {
            oper->literal[0] = oper->children[0].literal[0];
            oper->literal[1] = oper->children[1].literal[0];
            oper->literal[2] = oper->children[2].literal[0];
            oper->literal[3] = oper->children[3].literal[0];
            oper->literal_size = 4;
            slang_operation_destruct(oper);
            oper->type = SLANG_OPER_LITERAL_FLOAT;
            return;
         }
      }
   }

   if (oper->num_children == 3 && isFloat[0] && isFloat[1] && isFloat[2]) {
      /* vec3(flt, flt, flt) constructor */
      if (oper->type == SLANG_OPER_CALL) {
         if (strcmp((char *) oper->a_id, "vec3") == 0) {
            oper->literal[0] = oper->children[0].literal[0];
            oper->literal[1] = oper->children[1].literal[0];
            oper->literal[2] = oper->children[2].literal[0];
            oper->literal[3] = oper->literal[2];
            oper->literal_size = 3;
            slang_operation_destruct(oper);
            oper->type = SLANG_OPER_LITERAL_FLOAT;
            return;
         }
      }
   }

   if (oper->num_children == 2 && isFloat[0] && isFloat[1]) {
      /* vec2(flt, flt) constructor */
      if (oper->type == SLANG_OPER_CALL) {
         if (strcmp((char *) oper->a_id, "vec2") == 0) {
            oper->literal[0] = oper->children[0].literal[0];
            oper->literal[1] = oper->children[1].literal[0];
            oper->literal[2] = oper->literal[1];
            oper->literal[3] = oper->literal[1];
            oper->literal_size = 2;
            slang_operation_destruct(oper); /* XXX oper->locals goes NULL! */
            oper->type = SLANG_OPER_LITERAL_FLOAT;
            assert(oper->num_children == 0);
            return;
         }
      }
   }

   if (oper->num_children == 1 && isFloat[0]) {
      /* vec2/3/4(flt, flt) constructor */
      if (oper->type == SLANG_OPER_CALL) {
         const char *func = (const char *) oper->a_id;
         if (strncmp(func, "vec", 3) == 0 && func[3] >= '2' && func[3] <= '4') {
            oper->literal[0] =
            oper->literal[1] =
            oper->literal[2] =
            oper->literal[3] = oper->children[0].literal[0];
            oper->literal_size = func[3] - '0';
            assert(oper->literal_size >= 2);
            assert(oper->literal_size <= 4);
            slang_operation_destruct(oper); /* XXX oper->locals goes NULL! */
            oper->type = SLANG_OPER_LITERAL_FLOAT;
            assert(oper->num_children == 0);
            return;
         }
      }
   }
}



/**
 * Insert casts to try to adapt actual parameters to formal parameters for a
 * function call when an exact match for the parameter types is not found.
 * Example:
 *   void foo(int i, bool b) {}
 *   x = foo(3.15, 9);
 * Gets translated into:
 *   x = foo(int(3.15), bool(9))
 */
GLboolean
_slang_cast_func_params(slang_operation *callOper, const slang_function *fun,
                        const slang_name_space * space,
                        slang_atom_pool * atoms, slang_info_log *log)
{
   const GLboolean haveRetValue = _slang_function_has_return_value(fun);
   const int numParams = fun->param_count - haveRetValue;
   int i;
   int dbg = 0;

   if (dbg)
      printf("Adapt call of %d args to func %s (%d params)\n",
             callOper->num_children, (char*) fun->header.a_name, numParams);

   for (i = 0; i < numParams; i++) {
      slang_typeinfo argType;
      slang_variable *paramVar = fun->parameters->variables[i];

      /* Get type of arg[i] */
      if (!slang_typeinfo_construct(&argType))
         return GL_FALSE;
      if (!_slang_typeof_operation_(&callOper->children[i], space,
                                    &argType, atoms, log)) {
         slang_typeinfo_destruct(&argType);
         return GL_FALSE;
      }

      /* see if arg type matches parameter type */
      if (!slang_type_specifier_equal(&argType.spec,
                                      &paramVar->type.specifier)) {
         /* need to adapt arg type to match param type */
         const char *constructorName =
            slang_type_specifier_type_to_string(paramVar->type.specifier.type);
         slang_operation *child = slang_operation_new(1);

         if (dbg)
            printf("Need to adapt types of arg %d\n", i);

         slang_operation_copy(child, &callOper->children[i]);
         child->locals->outer_scope = callOper->children[i].locals;

#if 0
         if (_slang_sizeof_type_specifier(&argType.spec) >
             _slang_sizeof_type_specifier(&paramVar->type.specifier)) {
         }
#endif

         callOper->children[i].type = SLANG_OPER_CALL;
         callOper->children[i].a_id = slang_atom_pool_atom(atoms, constructorName);
         callOper->children[i].num_children = 1;
         callOper->children[i].children = child;
      }

      slang_typeinfo_destruct(&argType);
   }

   if (dbg) {
      printf("===== New call to %s with cast arguments ===============\n",
             (char*) fun->header.a_name);
      slang_print_tree(callOper, 5);
   }

   return GL_TRUE;
}


/**
 * Adapt the arguments for a function call to match the parameters of
 * the given function.
 * This is for:
 * 1. converting/casting argument types to match parameters
 * 2. breaking up vector/matrix types into individual components to
 *    satisfy constructors.
 */
GLboolean
_slang_adapt_call(slang_operation *callOper, const slang_function *fun,
                  const slang_name_space * space,
                  slang_atom_pool * atoms, slang_info_log *log)
{
   const GLboolean haveRetValue = _slang_function_has_return_value(fun);
   const int numParams = fun->param_count - haveRetValue;
   int i;
   int dbg = 0;

   if (dbg)
      printf("Adapt %d args to %d parameters for %s\n",
             callOper->num_children, numParams, (char *) fun->header.a_name);

   /* Only try adapting for constructors */
   if (fun->kind != SLANG_FUNC_CONSTRUCTOR)
      return GL_FALSE;

   if (callOper->num_children != numParams) {
      /* number of arguments doesn't match number of parameters */

      /* For constructor calls, we can try to unroll vector/matrix args
       * into individual floats/ints and try to match the function params.
       */
      for (i = 0; i < numParams; i++) {
         slang_typeinfo argType;
         GLint argSz, j;

         /* Get type of arg[i] */
         if (!slang_typeinfo_construct(&argType))
            return GL_FALSE;
         if (!_slang_typeof_operation_(&callOper->children[i], space,
                                       &argType, atoms, log)) {
            slang_typeinfo_destruct(&argType);
            return GL_FALSE;
         }

         /*
           paramSz = _slang_sizeof_type_specifier(&paramVar->type.specifier);
           assert(paramSz == 1);
         */
         argSz = _slang_sizeof_type_specifier(&argType.spec);
         if (argSz > 1) {
            slang_operation origArg;
            /* break up arg[i] into components */
            if (dbg)
               printf("Break up arg %d from 1 to %d elements\n", i, argSz);

            slang_operation_construct(&origArg);
            slang_operation_copy(&origArg, &callOper->children[i]);

            /* insert argSz-1 new children/args */
            for (j = 0; j < argSz - 1; j++) {
               (void) slang_operation_insert(&callOper->num_children,
                                             &callOper->children, i);
            }

            /* replace arg[i+j] with subscript/index oper */
            for (j = 0; j < argSz; j++) {
               callOper->children[i + j].type = SLANG_OPER_SUBSCRIPT;
               callOper->children[i + j].locals = _slang_variable_scope_new(callOper->locals);
               callOper->children[i + j].num_children = 2;
               callOper->children[i + j].children = slang_operation_new(2);
               slang_operation_copy(&callOper->children[i + j].children[0],
                                    &origArg);
               callOper->children[i + j].children[1].type
                  = SLANG_OPER_LITERAL_INT;
               callOper->children[i + j].children[1].literal[0] = (GLfloat) j;
            }
         }
      }
   }

   if (callOper->num_children < (GLuint) numParams) {
      /* still not enough args for all params */
      return GL_FALSE;
   }
   else if (callOper->num_children > (GLuint) numParams) {
      /* now too many arguments */
      /* just truncate */
      callOper->num_children = (GLuint) numParams;
   }

   if (dbg) {
      printf("===== New call to %s with adapted arguments ===============\n",
             (char*) fun->header.a_name);
      slang_print_tree(callOper, 5);
   }

   return GL_TRUE;
}
