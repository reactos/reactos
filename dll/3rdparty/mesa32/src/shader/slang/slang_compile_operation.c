/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
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
 * \file slang_compile_operation.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "main/imports.h"
#include "slang_compile.h"
#include "slang_mem.h"


/**
 * Init a slang_operation object
 */
GLboolean
slang_operation_construct(slang_operation * oper)
{
   oper->type = SLANG_OPER_NONE;
   oper->children = NULL;
   oper->num_children = 0;
   oper->literal[0] = 0.0;
   oper->literal_size = 1;
   oper->a_id = SLANG_ATOM_NULL;
   oper->locals = _slang_variable_scope_new(NULL);
   if (oper->locals == NULL)
      return GL_FALSE;
   _slang_variable_scope_ctr(oper->locals);
   oper->fun = NULL;
   oper->var = NULL;
   return GL_TRUE;
}

void
slang_operation_destruct(slang_operation * oper)
{
   GLuint i;

   for (i = 0; i < oper->num_children; i++)
      slang_operation_destruct(oper->children + i);
   _slang_free(oper->children);
   slang_variable_scope_destruct(oper->locals);
   _slang_free(oper->locals);
   oper->children = NULL;
   oper->num_children = 0;
   oper->locals = NULL;
}


/**
 * Recursively traverse 'oper', replacing occurances of 'oldScope' with
 * 'newScope' in the oper->locals->outer_scope field.
 */
void
slang_replace_scope(slang_operation *oper,
                    slang_variable_scope *oldScope,
                    slang_variable_scope *newScope)
{
   GLuint i;
   if (oper->locals != newScope &&
       oper->locals->outer_scope == oldScope) {
      oper->locals->outer_scope = newScope;
   }
   for (i = 0; i < oper->num_children; i++) {
      slang_replace_scope(&oper->children[i], oldScope, newScope);
   }
}


/**
 * Recursively copy a slang_operation node.
 * \param x  copy target
 * \param y  copy source
 * \return GL_TRUE for success, GL_FALSE if failure
 */
GLboolean
slang_operation_copy(slang_operation * x, const slang_operation * y)
{
   slang_operation z;
   GLuint i;

   if (!slang_operation_construct(&z))
      return GL_FALSE;
   z.type = y->type;
   z.children = (slang_operation *)
      _slang_alloc(y->num_children * sizeof(slang_operation));
   if (z.children == NULL) {
      slang_operation_destruct(&z);
      return GL_FALSE;
   }
   for (z.num_children = 0; z.num_children < y->num_children;
        z.num_children++) {
      if (!slang_operation_construct(&z.children[z.num_children])) {
         slang_operation_destruct(&z);
         return GL_FALSE;
      }
   }
   for (i = 0; i < z.num_children; i++) {
      if (!slang_operation_copy(&z.children[i], &y->children[i])) {
         slang_operation_destruct(&z);
         return GL_FALSE;
      }
   }
   z.literal[0] = y->literal[0];
   z.literal[1] = y->literal[1];
   z.literal[2] = y->literal[2];
   z.literal[3] = y->literal[3];
   z.literal_size = y->literal_size;
   assert(y->literal_size >= 1);
   assert(y->literal_size <= 4);
   z.a_id = y->a_id;
   if (y->locals) {
      if (!slang_variable_scope_copy(z.locals, y->locals)) {
         slang_operation_destruct(&z);
         return GL_FALSE;
      }
   }
#if 0
   z.var = y->var;
   z.fun = y->fun;
#endif
   slang_operation_destruct(x);
   *x = z;

   /* If this operation declares a new scope, we need to make sure
    * all children point to it, not the original operation's scope!
    */
   if (x->type == SLANG_OPER_BLOCK_NEW_SCOPE) {
      slang_replace_scope(x, y->locals, x->locals);
   }

   return GL_TRUE;
}


slang_operation *
slang_operation_new(GLuint count)
{
   slang_operation *ops
       = (slang_operation *) _slang_alloc(count * sizeof(slang_operation));
   assert(count > 0);
   if (ops) {
      GLuint i;
      for (i = 0; i < count; i++)
         slang_operation_construct(ops + i);
   }
   return ops;
}


/**
 * Delete operation and all children
 */
void
slang_operation_delete(slang_operation *oper)
{
   slang_operation_destruct(oper);
   _slang_free(oper);
}


slang_operation *
slang_operation_grow(GLuint *numChildren, slang_operation **children)
{
   slang_operation *ops;

   ops = (slang_operation *)
      _slang_realloc(*children,
                     *numChildren * sizeof(slang_operation),
                     (*numChildren + 1) * sizeof(slang_operation));
   if (ops) {
      slang_operation *newOp = ops + *numChildren;
      if (!slang_operation_construct(newOp)) {
         _slang_free(ops);
         *children = NULL;
         return NULL;
      }
      *children = ops;
      (*numChildren)++;
      return newOp;
   }
   return NULL;
}

/**
 * Insert a new slang_operation into an array.
 * \param numElements  pointer to current array size (in/out)
 * \param array  address of the array (in/out)
 * \param pos  position to insert new element
 * \return  pointer to the new operation/element
 */
slang_operation *
slang_operation_insert(GLuint *numElements, slang_operation **array,
                       GLuint pos)
{
   slang_operation *ops;

   assert(pos <= *numElements);

   ops = (slang_operation *)
      _slang_alloc((*numElements + 1) * sizeof(slang_operation));
   if (ops) {
      slang_operation *newOp;
      newOp = ops + pos;
      if (pos > 0)
         _mesa_memcpy(ops, *array, pos * sizeof(slang_operation));
      if (pos < *numElements)
         _mesa_memcpy(newOp + 1, (*array) + pos,
                      (*numElements - pos) * sizeof(slang_operation));

      if (!slang_operation_construct(newOp)) {
         _slang_free(ops);
         *numElements = 0;
         *array = NULL;
         return NULL;
      }
      if (*array)
         _slang_free(*array);
      *array = ops;
      (*numElements)++;
      return newOp;
   }
   return NULL;
}


void
_slang_operation_swap(slang_operation *oper0, slang_operation *oper1)
{
   slang_operation tmp = *oper0;
   *oper0 = *oper1;
   *oper1 = tmp;
}


