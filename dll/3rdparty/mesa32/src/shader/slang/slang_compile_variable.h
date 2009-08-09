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

#ifndef SLANG_COMPILE_VARIABLE_H
#define SLANG_COMPILE_VARIABLE_H


struct slang_ir_storage_;


/**
 * A shading language program variable.
 */
typedef struct slang_variable_
{
   slang_fully_specified_type type; /**< Variable's data type */
   slang_atom a_name;               /**< The variable's name (char *) */
   GLuint array_len;                /**< only if type == SLANG_SPEC_ARRAy */
   struct slang_operation_ *initializer; /**< Optional initializer code */
   GLuint size;                     /**< Variable's size in bytes */
   GLboolean isTemp;                /**< a named temporary (__resultTmp) */
   GLboolean declared;              /**< for debug */
   struct slang_ir_storage_ *store; /**< Storage for this var */
} slang_variable;


/**
 * Basically a list of variables, with a pointer to the parent scope.
 */
typedef struct slang_variable_scope_
{
   slang_variable **variables;  /**< Array [num_variables] of ptrs to vars */
   GLuint num_variables;
   struct slang_variable_scope_ *outer_scope;
} slang_variable_scope;


extern slang_variable_scope *
_slang_variable_scope_new(slang_variable_scope *parent);

extern GLvoid
_slang_variable_scope_ctr(slang_variable_scope *);

extern void
slang_variable_scope_destruct(slang_variable_scope *);

extern int
slang_variable_scope_copy(slang_variable_scope *,
                          const slang_variable_scope *);

extern slang_variable *
slang_variable_scope_grow(slang_variable_scope *);

extern int
slang_variable_construct(slang_variable *);

extern void
slang_variable_destruct(slang_variable *);

extern int
slang_variable_copy(slang_variable *, const slang_variable *);

extern slang_variable *
_slang_variable_locate(const slang_variable_scope *, const slang_atom a_name,
                       GLboolean all);


#endif /* SLANG_COMPILE_VARIABLE_H */
