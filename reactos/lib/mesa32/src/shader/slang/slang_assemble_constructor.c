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
 * \file slang_assemble_constructor.c
 * slang constructor and vector swizzle assembler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"
#include "slang_assemble_constructor.h"
#include "slang_assemble_typeinfo.h"
#include "slang_storage.h"

/* _slang_is_swizzle() */

int _slang_is_swizzle (const char *field, unsigned int rows, slang_swizzle *swz)
{
	unsigned int i;
	int xyzw = 0, rgba = 0, stpq = 0;

	/* the swizzle can be at most 4-component long */
	swz->num_components = slang_string_length (field);
	if (swz->num_components > 4)
		return 0;

	for (i = 0; i < swz->num_components; i++)
	{
		/* mark which swizzle group is used */
		switch (field[i])
		{
		case 'x':
		case 'y':
		case 'z':
		case 'w':
			xyzw = 1;
			break;
		case 'r':
		case 'g':
		case 'b':
		case 'a':
			rgba = 1;
			break;
		case 's':
		case 't':
		case 'p':
		case 'q':
			stpq = 1;
			break;
		default:
			return 0;
		}

		/* collect swizzle component */
		switch (field[i])
		{
		case 'x':
		case 'r':
		case 's':
			swz->swizzle[i] = 0;
			break;
		case 'y':
		case 'g':
		case 't':
			if (rows < 2)
				return 0;
			swz->swizzle[i] = 1;
			break;
		case 'z':
		case 'b':
		case 'p':
			if (rows < 3)
				return 0;
			swz->swizzle[i] = 2;
			break;
		case 'w':
		case 'a':
		case 'q':
			if (rows < 4)
				return 0;
			swz->swizzle[i] = 3;
			break;
		}
	}

	/* only one swizzle group can be used */
	if ((xyzw && rgba) || (xyzw && stpq) || (rgba && stpq))
		return 0;

	return 1;
}

/* _slang_is_swizzle_mask() */

int _slang_is_swizzle_mask (const slang_swizzle *swz, unsigned int rows)
{
	unsigned int c, i;

	if (swz->num_components > rows)
		return 0;
	c = swz->swizzle[0];
	for (i = 1; i < swz->num_components; i++)
	{
		if (swz->swizzle[i] <= c)
			return 0;
		c = swz->swizzle[i];
	}
	return 1;
}

/* _slang_multiply_swizzles() */

void _slang_multiply_swizzles (slang_swizzle *dst, const slang_swizzle *left,
	const slang_swizzle *right)
{
	unsigned int i;
	dst->num_components = right->num_components;
	for (i = 0; i < right->num_components; i++)
		dst->swizzle[i] = left->swizzle[right->swizzle[i]];
}

/* _slang_assemble_constructor() */

static int constructor_aggregate (slang_assembly_file *file, const slang_storage_aggregate *flat,
	unsigned int *index, slang_operation *op, unsigned int size, slang_assembly_flow_control *flow,
	slang_assembly_name_space *space, slang_assembly_local_info *info)
{
	slang_assembly_typeinfo ti;
	int result;
	slang_storage_aggregate agg, flat_agg;
	slang_assembly_stack_info stk;
	unsigned int i;

	slang_assembly_typeinfo_construct (&ti);
	if (!(result = _slang_typeof_operation (op, space, &ti)))
		goto end1;

	slang_storage_aggregate_construct (&agg);
	if (!(result = _slang_aggregate_variable (&agg, &ti.spec, NULL, space->funcs, space->structs)))
		goto end2;

	slang_storage_aggregate_construct (&flat_agg);
	if (!(result = _slang_flatten_aggregate (&flat_agg, &agg)))
		goto end;

	if (!(result = _slang_assemble_operation (file, op, 0, flow, space, info, &stk)))
		goto end;

	for (i = 0; i < flat_agg.count; i++)
	{
		const slang_storage_array *arr1 = flat_agg.arrays + i;
		const slang_storage_array *arr2 = flat->arrays + *index;

		if (arr1->type != arr2->type)
		{
			/* TODO: convert (generic) from arr1 to arr2 */
		}
		(*index)++;
		/* TODO: watch the index, if it reaches the size, pop off the stack subsequent values */
	}

	result = 1;
end:
	slang_storage_aggregate_destruct (&flat_agg);
end2:
	slang_storage_aggregate_destruct (&agg);
end1:
	slang_assembly_typeinfo_destruct (&ti);
	return result;
}
/* XXX: general swizzle! */
int _slang_assemble_constructor (slang_assembly_file *file, slang_operation *op,
	slang_assembly_flow_control *flow, slang_assembly_name_space *space,
	slang_assembly_local_info *info)
{
	slang_assembly_typeinfo ti;
	int result;
	slang_storage_aggregate agg, flat;
	unsigned int size, index, i;

	slang_assembly_typeinfo_construct (&ti);
	if (!(result = _slang_typeof_operation (op, space, &ti)))
		goto end1;

	slang_storage_aggregate_construct (&agg);
	if (!(result = _slang_aggregate_variable (&agg, &ti.spec, NULL, space->funcs, space->structs)))
		goto end2;

	size = _slang_sizeof_aggregate (&agg);

	slang_storage_aggregate_construct (&flat);
	if (!(result = _slang_flatten_aggregate (&flat, &agg)))
		goto end;

	index = 0;
	for (i = 0; i < op->num_children; i++)
	{
		if (!(result = constructor_aggregate (file, &flat, &index, op->children + i, size, flow,
			space, info)))
			goto end;
		/* TODO: watch the index, if it reaches the size, raise an error */
	}

	result = 1;
end:
	slang_storage_aggregate_destruct (&flat);
end2:
	slang_storage_aggregate_destruct (&agg);
end1:
	slang_assembly_typeinfo_destruct (&ti);
	return result;
}

/* _slang_assemble_constructor_from_swizzle() */
/* XXX: wrong */
int _slang_assemble_constructor_from_swizzle (slang_assembly_file *file, const slang_swizzle *swz,
	slang_type_specifier *spec, slang_type_specifier *master_spec, slang_assembly_local_info *info)
{
	unsigned int master_rows, i;
	switch (master_spec->type)
	{
	case slang_spec_bool:
	case slang_spec_int:
	case slang_spec_float:
		master_rows = 1;
		break;
	case slang_spec_bvec2:
	case slang_spec_ivec2:
	case slang_spec_vec2:
		master_rows = 2;
		break;
	case slang_spec_bvec3:
	case slang_spec_ivec3:
	case slang_spec_vec3:
		master_rows = 3;
		break;
	case slang_spec_bvec4:
	case slang_spec_ivec4:
	case slang_spec_vec4:
		master_rows = 4;
		break;
	default:
	   break;
	}
	for (i = 0; i < master_rows; i++)
	{
		switch (master_spec->type)
		{
		case slang_spec_bool:
		case slang_spec_bvec2:
		case slang_spec_bvec3:
		case slang_spec_bvec4:
			if (!slang_assembly_file_push_label2 (file, slang_asm_bool_copy, (master_rows - i) * 4,
				i * 4))
				return 0;
			break;
		case slang_spec_int:
		case slang_spec_ivec2:
		case slang_spec_ivec3:
		case slang_spec_ivec4:
			if (!slang_assembly_file_push_label2 (file, slang_asm_int_copy, (master_rows - i) * 4,
				i * 4))
				return 0;
			break;
		case slang_spec_float:
		case slang_spec_vec2:
		case slang_spec_vec3:
		case slang_spec_vec4:
			if (!slang_assembly_file_push_label2 (file, slang_asm_float_copy,
				(master_rows - i) * 4, i * 4))
				return 0;
			break;
		default:
		      break;
		}
	}
	if (!slang_assembly_file_push_label (file, slang_asm_local_free, 4))
		return 0;
	for (i = swz->num_components; i > 0; i--)
	{
		unsigned int n = i - 1;
		if (!slang_assembly_file_push_label2 (file, slang_asm_local_addr, info->swizzle_tmp, 16))
			return 0;
		if (!slang_assembly_file_push_label (file, slang_asm_addr_push, swz->swizzle[n] * 4))
			return 0;
		if (!slang_assembly_file_push (file, slang_asm_addr_add))
			return 0;
		switch (master_spec->type)
		{
		case slang_spec_bool:
		case slang_spec_bvec2:
		case slang_spec_bvec3:
		case slang_spec_bvec4:
			if (!slang_assembly_file_push (file, slang_asm_bool_deref))
				return 0;
			break;
		case slang_spec_int:
		case slang_spec_ivec2:
		case slang_spec_ivec3:
		case slang_spec_ivec4:
			if (!slang_assembly_file_push (file, slang_asm_int_deref))
				return 0;
			break;
		case slang_spec_float:
		case slang_spec_vec2:
		case slang_spec_vec3:
		case slang_spec_vec4:
			if (!slang_assembly_file_push (file, slang_asm_float_deref))
				return 0;
			break;
		default:
		   break;
		}
	}
	return 1;
}

