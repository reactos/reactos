/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2005  Brian Paul   All Rights Reserved.
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

#if !defined SLANG_ASSEMBLE_ASSIGNMENT_H
#define SLANG_ASSEMBLE_ASSIGNMENT_H

#include "slang_assemble.h"

#if defined __cplusplus
extern "C" {
#endif

int _slang_assemble_assignment (slang_assembly_file *, slang_operation *,
	slang_assembly_name_space *, slang_assembly_local_info *);

int _slang_assemble_assign (slang_assembly_file *, slang_operation *, const char *, int ref,
	slang_assembly_name_space *, slang_assembly_local_info *);

#ifdef __cplusplus
}
#endif

#endif

