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
 * \file slang_storage.c
 * slang variable storage
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"
#include "slang_storage.h"
#include "slang_assemble.h"

/* slang_storage_array */

void slang_storage_array_construct (slang_storage_array *arr)
{
	arr->type = slang_stor_aggregate;
	arr->aggregate = NULL;
	arr->length = 0;
}

void slang_storage_array_destruct (slang_storage_array *arr)
{
	if (arr->aggregate != NULL)
	{
		slang_storage_aggregate_destruct (arr->aggregate);
		slang_alloc_free (arr->aggregate);
	}
}

/* slang_storage_aggregate */

void slang_storage_aggregate_construct (slang_storage_aggregate *agg)
{
	agg->arrays = NULL;
	agg->count = 0;
}

void slang_storage_aggregate_destruct (slang_storage_aggregate *agg)
{
	unsigned int i;
	for (i = 0; i < agg->count; i++)
		slang_storage_array_destruct (agg->arrays + i);
	slang_alloc_free (agg->arrays);
}

static slang_storage_array *slang_storage_aggregate_push_new (slang_storage_aggregate *agg)
{
	slang_storage_array *arr = NULL;
	agg->arrays = (slang_storage_array *) slang_alloc_realloc (agg->arrays, agg->count * sizeof (
		slang_storage_array), (agg->count + 1) * sizeof (slang_storage_array));
	if (agg->arrays != NULL)
	{
		arr = agg->arrays + agg->count;
		slang_storage_array_construct (arr);
		agg->count++;
	}
	return arr;
}

/* _slang_aggregate_variable() */

static int aggregate_vector (slang_storage_aggregate *agg, slang_storage_type basic_type,
	unsigned int row_count)
{
	slang_storage_array *arr = slang_storage_aggregate_push_new (agg);
	if (arr == NULL)
		return 0;
	arr->type = basic_type;
	arr->length = row_count;
	return 1;
}

static int aggregate_matrix (slang_storage_aggregate *agg, slang_storage_type basic_type,
	unsigned int dimension)
{
	slang_storage_array *arr = slang_storage_aggregate_push_new (agg);
	if (arr == NULL)
		return 0;
	arr->type = slang_stor_aggregate;
	arr->length = dimension;
	arr->aggregate = (slang_storage_aggregate *) slang_alloc_malloc (sizeof (
		slang_storage_aggregate));
	if (arr->aggregate == NULL)
		return 0;
	slang_storage_aggregate_construct (arr->aggregate);
	if (!aggregate_vector (arr->aggregate, basic_type, dimension))
		return 0;
	return 1;
}

static int aggregate_variables (slang_storage_aggregate *agg, const slang_variable_scope *vars,
	slang_function_scope *funcs, slang_struct_scope *structs)
{
	unsigned int i;
	for (i = 0; i < vars->num_variables; i++)
		if (!_slang_aggregate_variable (agg, &vars->variables[i].type.specifier,
			vars->variables[i].array_size, funcs, structs))
			return 0;
	return 1;
}

int _slang_aggregate_variable (slang_storage_aggregate *agg, slang_type_specifier *spec,
	slang_operation *array_size, slang_function_scope *funcs, slang_struct_scope *structs)
{
	switch (spec->type)
	{
	case slang_spec_bool:
		return aggregate_vector (agg, slang_stor_bool, 1);
	case slang_spec_bvec2:
		return aggregate_vector (agg, slang_stor_bool, 2);
	case slang_spec_bvec3:
		return aggregate_vector (agg, slang_stor_bool, 3);
	case slang_spec_bvec4:
		return aggregate_vector (agg, slang_stor_bool, 4);
	case slang_spec_int:
		return aggregate_vector (agg, slang_stor_int, 1);
	case slang_spec_ivec2:
		return aggregate_vector (agg, slang_stor_int, 2);
	case slang_spec_ivec3:
		return aggregate_vector (agg, slang_stor_int, 3);
	case slang_spec_ivec4:
		return aggregate_vector (agg, slang_stor_int, 4);
	case slang_spec_float:
		return aggregate_vector (agg, slang_stor_float, 1);
	case slang_spec_vec2:
		return aggregate_vector (agg, slang_stor_float, 2);
	case slang_spec_vec3:
		return aggregate_vector (agg, slang_stor_float, 3);
	case slang_spec_vec4:
		return aggregate_vector (agg, slang_stor_float, 4);
	case slang_spec_mat2:
		return aggregate_matrix (agg, slang_stor_float, 2);
	case slang_spec_mat3:
		return aggregate_matrix (agg, slang_stor_float, 3);
	case slang_spec_mat4:
		return aggregate_matrix (agg, slang_stor_float, 4);
	case slang_spec_sampler1D:
	case slang_spec_sampler2D:
	case slang_spec_sampler3D:
	case slang_spec_samplerCube:
	case slang_spec_sampler1DShadow:
	case slang_spec_sampler2DShadow:
		return aggregate_vector (agg, slang_stor_int, 1);
	case slang_spec_struct:
		return aggregate_variables (agg, spec->_struct->fields, funcs, structs);
	case slang_spec_array:
		{
			slang_storage_array *arr;
			slang_assembly_file file;
			slang_assembly_flow_control flow;
			slang_assembly_name_space space;
			slang_assembly_local_info info;
			slang_assembly_stack_info stk;

			arr = slang_storage_aggregate_push_new (agg);
			if (arr == NULL)
				return 0;
			arr->type = slang_stor_aggregate;
			arr->aggregate = (slang_storage_aggregate *) slang_alloc_malloc (sizeof (
				slang_storage_aggregate));
			if (arr->aggregate == NULL)
				return 0;
			slang_storage_aggregate_construct (arr->aggregate);
			if (!_slang_aggregate_variable (arr->aggregate, spec->_array, NULL, funcs, structs))
				return 0;
			slang_assembly_file_construct (&file);
			space.funcs = funcs;
			space.structs = structs;
			/* XXX: vars! */
			space.vars = NULL;
			if (!_slang_assemble_operation (&file, array_size, 0, &flow, &space, &info, &stk))
			{
				slang_assembly_file_destruct (&file);
				return 0;
			}
			/* TODO: evaluate array size */
			slang_assembly_file_destruct (&file);
			arr->length = 256;
		}
		return 1;
	default:
		return 0;
	}
}

/* _slang_sizeof_aggregate() */

unsigned int _slang_sizeof_aggregate (const slang_storage_aggregate *agg)
{
	unsigned int i, size = 0;
	for (i = 0; i < agg->count; i++)
	{
		unsigned int element_size;
		if (agg->arrays[i].type == slang_stor_aggregate)
			element_size = _slang_sizeof_aggregate (agg->arrays[i].aggregate);
		else
			element_size = sizeof (GLfloat);
		size += element_size * agg->arrays[i].length;
	}
	return size;
}

/* _slang_flatten_aggregate () */

int _slang_flatten_aggregate (slang_storage_aggregate *flat, const slang_storage_aggregate *agg)
{
	unsigned int i;
	for (i = 0; i < agg->count; i++)
	{
		unsigned int j;
		for (j = 0; j < agg->arrays[i].length; j++)
		{
			if (agg->arrays[i].type == slang_stor_aggregate)
			{
				if (!_slang_flatten_aggregate (flat, agg->arrays[i].aggregate))
					return 0;
			}
			else
			{
				slang_storage_array *arr;
				arr = slang_storage_aggregate_push_new (flat);
				if (arr == NULL)
					return 0;
				arr->type = agg->arrays[i].type;
				arr->length = 1;
			}
		}
	}
	return 1;
}

