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

#ifndef SLANG_SIMPLIFY_H
#define SLANG_SIMPLIFY_H


extern GLint
_slang_lookup_constant(const char *name);


extern void
_slang_simplify(slang_operation *oper,
                const slang_name_space * space,
                slang_atom_pool * atoms);


extern GLboolean
_slang_cast_func_params(slang_operation *callOper, const slang_function *fun,
                        const slang_name_space * space,
                        slang_atom_pool * atoms, slang_info_log *log);

extern GLboolean
_slang_adapt_call(slang_operation *callOper, const slang_function *fun,
                  const slang_name_space * space,
                  slang_atom_pool * atoms, slang_info_log *log);


#endif /* SLANG_SIMPLIFY_H */
