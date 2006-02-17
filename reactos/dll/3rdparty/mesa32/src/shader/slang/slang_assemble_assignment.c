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

/**
 * \file slang_assemble_assignment.c
 * slang assignment expressions assembler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_assemble_assignment.h"
#include "slang_assemble_typeinfo.h"
#include "slang_storage.h"
#include "slang_utility.h"

/*
	_slang_assemble_assignment()

	copies values on the stack (<component 0> to <component N-1>) to a memory
	location pointed by <addr of variable>;

	in:
		+------------------+
		| addr of variable |
		+------------------+
		| component N-1    |
		| ...              |
		| component 0      |
		+------------------+

	out:
		+------------------+
		| addr of variable |
		+------------------+
*/
/* TODO: add support for swizzle mask */
static int assign_aggregate (slang_assembly_file *file, const slang_storage_aggregate *agg,
	unsigned int *index, unsigned int size, slang_assembly_local_info *info)
{
	unsigned int i;

	for (i = 0; i < agg->count; i++)
	{
		const slang_storage_array *arr = agg->arrays + i;
		unsigned int j;

		for (j = 0; j < arr->length; j++)
		{
			if (arr->type == slang_stor_aggregate)
			{
				if (!assign_aggregate (file, arr->aggregate, index, size, info))
					return 0;
			}
			else
			{
				slang_assembly_type ty;

				switch (arr->type)
				{
				case slang_stor_bool:
					ty = slang_asm_bool_copy;
					break;
				case slang_stor_int:
					ty = slang_asm_int_copy;
					break;
				case slang_stor_float:
					ty = slang_asm_float_copy;
					break;
				default:
					break;
				}
				if (!slang_assembly_file_push_label2 (file, ty, size - *index, *index))
					return 0;
				*index += 4;
			}
		}
	}
	return 1;
}

int _slang_assemble_assignment (slang_assembly_file *file, slang_operation *op,
	slang_assembly_name_space *space, slang_assembly_local_info *info)
{
	slang_assembly_typeinfo ti;
	int result;
	slang_storage_aggregate agg;
	unsigned int index, size;

	slang_assembly_typeinfo_construct (&ti);
	if (!_slang_typeof_operation (op, space, &ti))
	{
		slang_assembly_typeinfo_destruct (&ti);
		return 0;
	}

	slang_storage_aggregate_construct (&agg);
	if (!_slang_aggregate_variable (&agg, &ti.spec, NULL, space->funcs, space->structs))
	{
		slang_storage_aggregate_destruct (&agg);
		slang_assembly_typeinfo_destruct (&ti);
		return 0;
	}

	index = 0;
	size = _slang_sizeof_aggregate (&agg);
	result = assign_aggregate (file, &agg, &index, size, info);

	slang_storage_aggregate_destruct (&agg);
	slang_assembly_typeinfo_destruct (&ti);
	return result;
}

/*
	_slang_assemble_assign()

	performs unary (pre ++ and --) or binary (=, +=, -=, *=, /=) assignment on the operation's
	children
*/

int dereference (slang_assembly_file *file, slang_operation *op,
	slang_assembly_name_space *space, slang_assembly_local_info *info);

int call_function_name (slang_assembly_file *file, const char *name, slang_operation *params,
	unsigned int param_count, int assignment, slang_assembly_name_space *space,
	slang_assembly_local_info *info);

int _slang_assemble_assign (slang_assembly_file *file, slang_operation *op, const char *oper,
	int ref, slang_assembly_name_space *space, slang_assembly_local_info *info)
{
	slang_assembly_stack_info stk;
	slang_assembly_flow_control flow;

	if (!ref)
	{
		if (!slang_assembly_file_push_label2 (file, slang_asm_local_addr, info->addr_tmp, 4))
			return 0;
	}

	if (slang_string_compare ("=", oper) == 0)
	{
		if (!_slang_assemble_operation (file, op->children, 1, &flow, space, info, &stk))
			return 0;
		if (!_slang_assemble_operation (file, op->children + 1, 0, &flow, space, info, &stk))
			return 0;
		if (!_slang_assemble_assignment (file, op->children, space, info))
			return 0;
	}
	else
	{
		if (!call_function_name (file, oper, op->children, op->num_children, 1, space, info))
			return 0;
	}

	if (!ref)
	{
		if (!slang_assembly_file_push (file, slang_asm_addr_copy))
			return 0;
		if (!slang_assembly_file_push_label (file, slang_asm_local_free, 4))
			return 0;
		if (!dereference (file, op->children, space, info))
			return 0;
	}

	return 1;
}

