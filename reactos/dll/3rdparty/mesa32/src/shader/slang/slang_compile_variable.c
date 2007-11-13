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
 * \file slang_compile_variable.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_compile.h"
#include "slang_mem.h"


typedef struct
{
   const char *name;
   slang_type_specifier_type type;
} type_specifier_type_name;

static const type_specifier_type_name type_specifier_type_names[] = {
   {"void", SLANG_SPEC_VOID},
   {"bool", SLANG_SPEC_BOOL},
   {"bvec2", SLANG_SPEC_BVEC2},
   {"bvec3", SLANG_SPEC_BVEC3},
   {"bvec4", SLANG_SPEC_BVEC4},
   {"int", SLANG_SPEC_INT},
   {"ivec2", SLANG_SPEC_IVEC2},
   {"ivec3", SLANG_SPEC_IVEC3},
   {"ivec4", SLANG_SPEC_IVEC4},
   {"float", SLANG_SPEC_FLOAT},
   {"vec2", SLANG_SPEC_VEC2},
   {"vec3", SLANG_SPEC_VEC3},
   {"vec4", SLANG_SPEC_VEC4},
   {"mat2", SLANG_SPEC_MAT2},
   {"mat3", SLANG_SPEC_MAT3},
   {"mat4", SLANG_SPEC_MAT4},
   {"mat2x3", SLANG_SPEC_MAT23},
   {"mat3x2", SLANG_SPEC_MAT32},
   {"mat2x4", SLANG_SPEC_MAT24},
   {"mat4x2", SLANG_SPEC_MAT42},
   {"mat3x4", SLANG_SPEC_MAT34},
   {"mat4x3", SLANG_SPEC_MAT43},
   {"sampler1D", SLANG_SPEC_SAMPLER1D},
   {"sampler2D", SLANG_SPEC_SAMPLER2D},
   {"sampler3D", SLANG_SPEC_SAMPLER3D},
   {"samplerCube", SLANG_SPEC_SAMPLERCUBE},
   {"sampler1DShadow", SLANG_SPEC_SAMPLER1DSHADOW},
   {"sampler2DShadow", SLANG_SPEC_SAMPLER2DSHADOW},
   {"sampler2DRect", SLANG_SPEC_SAMPLER2DRECT},
   {"sampler2DRectShadow", SLANG_SPEC_SAMPLER2DRECTSHADOW},
   {NULL, SLANG_SPEC_VOID}
};

slang_type_specifier_type
slang_type_specifier_type_from_string(const char *name)
{
   const type_specifier_type_name *p = type_specifier_type_names;
   while (p->name != NULL) {
      if (slang_string_compare(p->name, name) == 0)
         break;
      p++;
   }
   return p->type;
}

const char *
slang_type_specifier_type_to_string(slang_type_specifier_type type)
{
   const type_specifier_type_name *p = type_specifier_type_names;
   while (p->name != NULL) {
      if (p->type == type)
         break;
      p++;
   }
   return p->name;
}

/* slang_fully_specified_type */

int
slang_fully_specified_type_construct(slang_fully_specified_type * type)
{
   type->qualifier = SLANG_QUAL_NONE;
   slang_type_specifier_ctr(&type->specifier);
   return 1;
}

void
slang_fully_specified_type_destruct(slang_fully_specified_type * type)
{
   slang_type_specifier_dtr(&type->specifier);
}

int
slang_fully_specified_type_copy(slang_fully_specified_type * x,
                                const slang_fully_specified_type * y)
{
   slang_fully_specified_type z;

   if (!slang_fully_specified_type_construct(&z))
      return 0;
   z.qualifier = y->qualifier;
   if (!slang_type_specifier_copy(&z.specifier, &y->specifier)) {
      slang_fully_specified_type_destruct(&z);
      return 0;
   }
   slang_fully_specified_type_destruct(x);
   *x = z;
   return 1;
}


static slang_variable *
slang_variable_new(void)
{
   slang_variable *v = (slang_variable *) _slang_alloc(sizeof(slang_variable));
   if (v) {
      if (!slang_variable_construct(v)) {
         _slang_free(v);
         v = NULL;
      }
   }
   return v;
}


static void
slang_variable_delete(slang_variable * var)
{
   slang_variable_destruct(var);
   _slang_free(var);
}


/*
 * slang_variable_scope
 */

slang_variable_scope *
_slang_variable_scope_new(slang_variable_scope *parent)
{
   slang_variable_scope *s;
   s = (slang_variable_scope *) _slang_alloc(sizeof(slang_variable_scope));
   if (s)
      s->outer_scope = parent;
   return s;
}


GLvoid
_slang_variable_scope_ctr(slang_variable_scope * self)
{
   self->variables = NULL;
   self->num_variables = 0;
   self->outer_scope = NULL;
}

void
slang_variable_scope_destruct(slang_variable_scope * scope)
{
   unsigned int i;

   if (!scope)
      return;
   for (i = 0; i < scope->num_variables; i++) {
      if (scope->variables[i])
         slang_variable_delete(scope->variables[i]);
   }
   _slang_free(scope->variables);
   /* do not free scope->outer_scope */
}

int
slang_variable_scope_copy(slang_variable_scope * x,
                          const slang_variable_scope * y)
{
   slang_variable_scope z;
   unsigned int i;

   _slang_variable_scope_ctr(&z);
   z.variables = (slang_variable **)
      _slang_alloc(y->num_variables * sizeof(slang_variable *));
   if (z.variables == NULL) {
      slang_variable_scope_destruct(&z);
      return 0;
   }
   for (z.num_variables = 0; z.num_variables < y->num_variables;
        z.num_variables++) {
      z.variables[z.num_variables] = slang_variable_new();
      if (!z.variables[z.num_variables]) {
         slang_variable_scope_destruct(&z);
         return 0;
      }
   }
   for (i = 0; i < z.num_variables; i++) {
      if (!slang_variable_copy(z.variables[i], y->variables[i])) {
         slang_variable_scope_destruct(&z);
         return 0;
      }
   }
   z.outer_scope = y->outer_scope;
   slang_variable_scope_destruct(x);
   *x = z;
   return 1;
}


/**
 * Grow the variable list by one.
 * \return  pointer to space for the new variable (will be initialized)
 */
slang_variable *
slang_variable_scope_grow(slang_variable_scope *scope)
{
   const int n = scope->num_variables;
   scope->variables = (slang_variable **)
         _slang_realloc(scope->variables,
                        n * sizeof(slang_variable *),
                        (n + 1) * sizeof(slang_variable *));
   if (!scope->variables)
      return NULL;

   scope->num_variables++;

   scope->variables[n] = slang_variable_new();
   if (!scope->variables[n])
      return NULL;

   return scope->variables[n];
}



/* slang_variable */

int
slang_variable_construct(slang_variable * var)
{
   if (!slang_fully_specified_type_construct(&var->type))
      return 0;
   var->a_name = SLANG_ATOM_NULL;
   var->array_len = 0;
   var->initializer = NULL;
   var->address = ~0;
   var->size = 0;
   var->isTemp = GL_FALSE;
   var->aux = NULL;
   return 1;
}


void
slang_variable_destruct(slang_variable * var)
{
   slang_fully_specified_type_destruct(&var->type);
   if (var->initializer != NULL) {
      slang_operation_destruct(var->initializer);
      _slang_free(var->initializer);
   }
#if 0
   if (var->aux) {
      _mesa_free(var->aux);
   }
#endif
}


int
slang_variable_copy(slang_variable * x, const slang_variable * y)
{
   slang_variable z;

   if (!slang_variable_construct(&z))
      return 0;
   if (!slang_fully_specified_type_copy(&z.type, &y->type)) {
      slang_variable_destruct(&z);
      return 0;
   }
   z.a_name = y->a_name;
   z.array_len = y->array_len;
   if (y->initializer != NULL) {
      z.initializer
         = (slang_operation *) _slang_alloc(sizeof(slang_operation));
      if (z.initializer == NULL) {
         slang_variable_destruct(&z);
         return 0;
      }
      if (!slang_operation_construct(z.initializer)) {
         _slang_free(z.initializer);
         slang_variable_destruct(&z);
         return 0;
      }
      if (!slang_operation_copy(z.initializer, y->initializer)) {
         slang_variable_destruct(&z);
         return 0;
      }
   }
   z.address = y->address;
   z.size = y->size;
   slang_variable_destruct(x);
   *x = z;
   return 1;
}


slang_variable *
_slang_locate_variable(const slang_variable_scope * scope,
                       const slang_atom a_name, GLboolean all)
{
   GLuint i;

   for (i = 0; i < scope->num_variables; i++)
      if (a_name == scope->variables[i]->a_name)
         return scope->variables[i];
   if (all && scope->outer_scope != NULL)
      return _slang_locate_variable(scope->outer_scope, a_name, 1);
   return NULL;
}
