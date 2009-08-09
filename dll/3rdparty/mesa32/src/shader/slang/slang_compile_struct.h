/*
 * Mesa 3-D graphics library
 * Version:  6.5
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

#if !defined SLANG_COMPILE_STRUCT_H
#define SLANG_COMPILE_STRUCT_H

#if defined __cplusplus
extern "C" {
#endif

struct slang_function_;

typedef struct slang_struct_scope_
{
   struct slang_struct_ *structs;
   GLuint num_structs;
   struct slang_struct_scope_ *outer_scope;
} slang_struct_scope;

extern GLvoid
_slang_struct_scope_ctr (slang_struct_scope *);

void slang_struct_scope_destruct (slang_struct_scope *);
int slang_struct_scope_copy (slang_struct_scope *, const slang_struct_scope *);
struct slang_struct_ *slang_struct_scope_find (slang_struct_scope *, slang_atom, int);

typedef struct slang_struct_
{
   slang_atom a_name;
   struct slang_variable_scope_ *fields;
   slang_struct_scope *structs;
   struct slang_function_ *constructor;
} slang_struct;

int slang_struct_construct (slang_struct *);
void slang_struct_destruct (slang_struct *);
int slang_struct_copy (slang_struct *, const slang_struct *);
int slang_struct_equal (const slang_struct *, const slang_struct *);

#ifdef __cplusplus
}
#endif

#endif

