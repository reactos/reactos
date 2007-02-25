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

#if !defined SLANG_ASSEMBLE_CONSTRUCTOR_H
#define SLANG_ASSEMBLE_CONSTRUCTOR_H

#include "slang_assemble.h"
#include "slang_compile.h"

#if defined __cplusplus
extern "C" {
#endif

/*
	holds a complete information about vector swizzle - the <swizzle> array contains
	vector component sources indices, where 0 is "x", 1 is "y", ...
	example: "xwz" --> { 3, { 0, 3, 2, n/u } }
*/
typedef struct slang_swizzle_
{
	unsigned int num_components;
	unsigned int swizzle[4];
} slang_swizzle;

/*
	checks if a field selector is a general swizzle (an r-value swizzle with replicated
	components or an l-value swizzle mask) for a vector
	returns 1 if this is the case, <swz> is filled with swizzle information
	returns 0 otherwise
*/
int _slang_is_swizzle (const char *field, unsigned int rows, slang_swizzle *swz);

/*
	checks if a general swizzle is an l-value swizzle - these swizzles do not have
	duplicated fields and they are specified in order
	returns 1 if this is a swizzle mask
	returns 0 otherwise
*/
int _slang_is_swizzle_mask (const slang_swizzle *swz, unsigned int rows);

/*
	combines two swizzles to form single swizzle
	example: "wzyx.yx" --> "zw"
*/
void _slang_multiply_swizzles (slang_swizzle *, const slang_swizzle *, const slang_swizzle *);

int _slang_assemble_constructor (slang_assembly_file *file, slang_operation *op,
	slang_assembly_flow_control *flow, slang_assembly_name_space *space,
	slang_assembly_local_info *info);

int _slang_assemble_constructor_from_swizzle (slang_assembly_file *file, const slang_swizzle *swz,
	slang_type_specifier *spec, slang_type_specifier *master_spec, slang_assembly_local_info *info);

#ifdef __cplusplus
}
#endif

#endif

