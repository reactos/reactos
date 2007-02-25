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
 * \file slang_assemble.c
 * slang intermediate code assembler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"
#include "slang_assemble.h"
#include "slang_compile.h"
#include "slang_storage.h"
#include "slang_assemble_constructor.h"
#include "slang_assemble_typeinfo.h"
#include "slang_assemble_conditional.h"
#include "slang_assemble_assignment.h"

/* slang_assembly */

static void slang_assembly_construct (slang_assembly *assem)
{
	assem->type = slang_asm_none;
}

static void slang_assembly_destruct (slang_assembly *assem)
{
}

/* slang_assembly_file */

void slang_assembly_file_construct (slang_assembly_file *file)
{
	file->code = NULL;
	file->count = 0;
}

void slang_assembly_file_destruct (slang_assembly_file *file)
{
	unsigned int i;

	for (i = 0; i < file->count; i++)
		slang_assembly_destruct (file->code + i);
	slang_alloc_free (file->code);
}

static int slang_assembly_file_push_new (slang_assembly_file *file)
{
	file->code = (slang_assembly *) slang_alloc_realloc (file->code, file->count * sizeof (
		slang_assembly), (file->count + 1) * sizeof (slang_assembly));
	if (file->code != NULL)
	{
		slang_assembly_construct (file->code + file->count);
		file->count++;
		return 1;
	}
	return 0;
}

static int slang_assembly_file_push_general (slang_assembly_file *file, slang_assembly_type type,
	GLfloat literal, GLuint label, GLuint size)
{
	slang_assembly *assem;
	if (!slang_assembly_file_push_new (file))
		return 0;
	assem = file->code + file->count - 1;
	assem->type = type;
	assem->literal = literal;
	assem->param[0] = label;
	assem->param[1] = size;
	return 1;
}

int slang_assembly_file_push (slang_assembly_file *file, slang_assembly_type type)
{
	return slang_assembly_file_push_general (file, type, (GLfloat) 0, 0, 0);
}

int slang_assembly_file_push_label (slang_assembly_file *file, slang_assembly_type type,
	GLuint label)
{
	return slang_assembly_file_push_general (file, type, (GLfloat) 0, label, 0);
}

int slang_assembly_file_push_label2 (slang_assembly_file *file, slang_assembly_type type,
	GLuint label1, GLuint label2)
{
	return slang_assembly_file_push_general (file, type, (GLfloat) 0, label1, label2);
}

int slang_assembly_file_push_literal (slang_assembly_file *file, slang_assembly_type type,
	GLfloat literal)
{
	return slang_assembly_file_push_general (file, type, literal, 0, 0);
}

/* utility functions */

static int sizeof_variable (slang_type_specifier *spec, slang_type_qualifier qual,
	slang_operation *array_size, slang_assembly_name_space *space, unsigned int *size)
{
	slang_storage_aggregate agg;

	slang_storage_aggregate_construct (&agg);
	if (!_slang_aggregate_variable (&agg, spec, array_size, space->funcs, space->structs))
	{
		slang_storage_aggregate_destruct (&agg);
		return 0;
	}
	*size += _slang_sizeof_aggregate (&agg);
	if (qual == slang_qual_out || qual == slang_qual_inout)
		*size += 4;
	slang_storage_aggregate_destruct (&agg);
	return 1;
}

static int sizeof_variable2 (slang_variable *var, slang_assembly_name_space *space,
	unsigned int *size)
{
	var->address = *size;
	if (var->type.qualifier == slang_qual_out || var->type.qualifier == slang_qual_inout)
		var->address += 4;
	return sizeof_variable (&var->type.specifier, var->type.qualifier, var->array_size, space,
		size);
}

static int sizeof_variables (slang_variable_scope *vars, unsigned int start, unsigned int stop,
	slang_assembly_name_space *space, unsigned int *size)
{
	unsigned int i;

	for (i = start; i < stop; i++)
		if (!sizeof_variable2 (vars->variables + i, space, size))
			return 0;
	return 1;
}

static int collect_locals (slang_operation *op, slang_assembly_name_space *space,
	unsigned int *size)
{
	unsigned int i;

	if (!sizeof_variables (op->locals, 0, op->locals->num_variables, space, size))
		return 0;
	for (i = 0; i < op->num_children; i++)
		if (!collect_locals (op->children + i, space, size))
			return 0;
	return 1;
}

/* _slang_locate_function() */

slang_function *_slang_locate_function (const char *name, slang_operation *params,
	unsigned int num_params, slang_assembly_name_space *space)
{
	unsigned int i;

	for (i = 0; i < space->funcs->num_functions; i++)
	{
		unsigned int j;
		slang_function *f = space->funcs->functions + i;

		if (slang_string_compare (name, f->header.name) != 0)
			continue;
		if (f->param_count != num_params)
			continue;
		for (j = 0; j < num_params; j++)
		{
			slang_assembly_typeinfo ti;
			slang_assembly_typeinfo_construct (&ti);
			if (!_slang_typeof_operation (params + j, space, &ti))
			{
				slang_assembly_typeinfo_destruct (&ti);
				return 0;
			}
			if (!slang_type_specifier_equal (&ti.spec, &f->parameters->variables[j].type.specifier))
			{
				slang_assembly_typeinfo_destruct (&ti);
				break;
			}
			slang_assembly_typeinfo_destruct (&ti);
			/* "out" and "inout" formal parameter requires the actual parameter to be l-value */
			if (!ti.can_be_referenced &&
				(f->parameters->variables[j].type.qualifier == slang_qual_out ||
				f->parameters->variables[j].type.qualifier == slang_qual_inout))
				break;
		}
		if (j == num_params)
			return f;
	}
	if (space->funcs->outer_scope != NULL)
	{
		slang_assembly_name_space my_space = *space;
		my_space.funcs = space->funcs->outer_scope;
		return _slang_locate_function (name, params, num_params, &my_space);
	}
	return NULL;
}

/* _slang_assemble_function() */

int _slang_assemble_function (slang_assembly_file *file, slang_function *fun,
	slang_assembly_name_space *space)
{
	unsigned int param_size, local_size;
	unsigned int skip, cleanup;
	slang_assembly_flow_control flow;
	slang_assembly_local_info info;
	slang_assembly_stack_info stk;

	fun->address = file->count;

	if (fun->body == NULL)
	{
		/* TODO: jump to the actual function body */
		return 1;
	}

	/* calculate return value and parameters size */
	param_size = 0;
	if (fun->header.type.specifier.type != slang_spec_void)
		if (!sizeof_variable (&fun->header.type.specifier, slang_qual_none, NULL, space,
			&param_size))
			return 0;
	info.ret_size = param_size;
	if (!sizeof_variables (fun->parameters, 0, fun->param_count, space, &param_size))
		return 0;

	/* calculate local variables size, take into account the four-byte return address and
	temporaries for various tasks */
	info.addr_tmp = param_size + 4;
	info.swizzle_tmp = param_size + 4 + 4;
	local_size = param_size + 4 + 4 + 16;
	if (!sizeof_variables (fun->parameters, fun->param_count, fun->parameters->num_variables, space,
		&local_size))
		return 0;
	if (!collect_locals (fun->body, space, &local_size))
		return 0;

	/* allocate local variable storage */
	if (!slang_assembly_file_push_label (file, slang_asm_local_alloc, local_size - param_size - 4))
		return 0;

	/* mark a new frame for function variable storage */
	if (!slang_assembly_file_push_label (file, slang_asm_enter, local_size))
		return 0;

	/* skip the cleanup jump */
	skip = file->count;
	if (!slang_assembly_file_push_new (file))
		return 0;
	file->code[skip].type = slang_asm_jump;

	/* all "return" statements will be directed here */
	flow.function_end = file->count;
	cleanup = file->count;
	if (!slang_assembly_file_push_new (file))
		return 0;
	file->code[cleanup].type = slang_asm_jump;

	/* execute the function body */
	file->code[skip].param[0] = file->count;
	if (!_slang_assemble_operation (file, fun->body, 0, &flow, space, &info, &stk))
		return 0;

	/* this is the end of the function - restore the old function frame */
	file->code[cleanup].param[0] = file->count;
	if (!slang_assembly_file_push (file, slang_asm_leave))
		return 0;

	/* free local variable storage */
	if (!slang_assembly_file_push_label (file, slang_asm_local_free, local_size - param_size - 4))
		return 0;

	/* jump out of the function */
	if (!slang_assembly_file_push (file, slang_asm_return))
		return 0;
	return 1;
}

int _slang_cleanup_stack (slang_assembly_file *file, slang_operation *op, int ref,
	slang_assembly_name_space *space)
{
	slang_assembly_typeinfo ti;
	unsigned int size;

	slang_assembly_typeinfo_construct (&ti);
	if (!_slang_typeof_operation (op, space, &ti))
	{
		slang_assembly_typeinfo_destruct (&ti);
		return 0;
	}
	if (ti.spec.type == slang_spec_void)
		size = 0;
	else if (ref)
		size = 4;
	else
	{
		size = 0;
		if (!sizeof_variable (&ti.spec, slang_qual_none, NULL, space, &size))
		{
			slang_assembly_typeinfo_destruct (&ti);
			return 0;
		}
	}
	slang_assembly_typeinfo_destruct (&ti);
	if (size != 0)
	{
		if (!slang_assembly_file_push_label (file, slang_asm_local_free, size))
			return 0;
	}
	return 1;
}

/* _slang_assemble_operation() */

/* XXX: general swizzle! */
static int dereference_aggregate (slang_assembly_file *file, const slang_storage_aggregate *agg,
	unsigned int index, unsigned int *size, slang_assembly_local_info *info)
{
	unsigned int i;

	for (i = agg->count; i > 0; i--)
	{
		const slang_storage_array *arr = agg->arrays + i - 1;
		unsigned int j;

		for (j = arr->length; j > 0; j--)
		{
			if (arr->type == slang_stor_aggregate)
			{
				if (!dereference_aggregate (file, arr->aggregate, index, size, info))
					return 0;
			}
			else
			{
				*size -= 4;
				if (!slang_assembly_file_push_label2 (file, slang_asm_local_addr, info->addr_tmp,
					4))
					return 0;
				if (!slang_assembly_file_push (file, slang_asm_addr_deref))
					return 0;
				if (!slang_assembly_file_push_label (file, slang_asm_addr_push, *size))
					return 0;
				if (!slang_assembly_file_push (file, slang_asm_addr_add))
					return 0;
				switch (arr->type)
				{
				case slang_stor_bool:
					if (!slang_assembly_file_push (file, slang_asm_bool_deref))
						return 0;
					break;
				case slang_stor_int:
					if (!slang_assembly_file_push (file, slang_asm_int_deref))
						return 0;
					break;
				case slang_stor_float:
					if (!slang_assembly_file_push (file, slang_asm_float_deref))
						return 0;
					break;
				}
				index += 4;
			}
		}
	}
	return 1;
}
/* XXX: general swizzle! */
int dereference (slang_assembly_file *file, slang_operation *op,
	slang_assembly_name_space *space, slang_assembly_local_info *info)
{
	slang_assembly_typeinfo ti;
	int result;
	slang_storage_aggregate agg;
	unsigned int size;

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

	size = _slang_sizeof_aggregate (&agg);
	result = dereference_aggregate (file, &agg, 0, &size, info);

	slang_storage_aggregate_destruct (&agg);
	slang_assembly_typeinfo_destruct (&ti);
	return result;
}

static int call_function (slang_assembly_file *file, slang_function *fun, slang_operation *params,
	unsigned int param_count, int assignment, slang_assembly_name_space *space,
	slang_assembly_local_info *info)
{
	unsigned int i;
	slang_assembly_stack_info stk;

	/* make room for the return value, if any */
	if (fun->header.type.specifier.type != slang_spec_void)
	{
		unsigned int ret_size = 0;
		if (!sizeof_variable (&fun->header.type.specifier, slang_qual_none, NULL, space, &ret_size))
			return 0;
		if (!slang_assembly_file_push_label (file, slang_asm_local_alloc, ret_size))
			return 0;
	}

	/* push the actual parameters on the stack */
	for (i = 0; i < param_count; i++)
	{
		slang_assembly_flow_control flow;

		if (fun->parameters->variables[i].type.qualifier == slang_qual_inout ||
			fun->parameters->variables[i].type.qualifier == slang_qual_out)
		{
			if (!slang_assembly_file_push_label2 (file, slang_asm_local_addr, info->addr_tmp, 4))
				return 0;
			/* TODO: optimize the "out" parameter case */
			/* TODO: inspect stk */
			if (!_slang_assemble_operation (file, params + i, 1, &flow, space, info, &stk))
				return 0;
			if (!slang_assembly_file_push (file, slang_asm_addr_copy))
				return 0;
			if (!slang_assembly_file_push (file, slang_asm_addr_deref))
				return 0;
			if (i == 0 && assignment)
			{
				if (!slang_assembly_file_push_label2 (file, slang_asm_local_addr, info->addr_tmp,
					4))
					return 0;
				if (!slang_assembly_file_push (file, slang_asm_addr_deref))
					return 0;
			}
			if (!dereference (file, params, space, info))
				return 0;
		}
		else
		{
			/* TODO: for "out" and "inout" parameters also push the address (first) */
			/* TODO: optimize the "out" parameter case */
			/* TODO: inspect stk */
			if (!_slang_assemble_operation (file, params + i, 0, &flow, space, info, &stk))
				return 0;
		}
	}

	/* call the function */
	if (!slang_assembly_file_push_label (file, slang_asm_call, fun->address))
		return 0;

	/* pop the parameters from the stack */
	for (i = param_count; i > 0; i--)
	{
		unsigned int j = i - 1;
		if (fun->parameters->variables[j].type.qualifier == slang_qual_inout ||
			fun->parameters->variables[j].type.qualifier == slang_qual_out)
		{
			if (!_slang_assemble_assignment (file, params + j, space, info))
				return 0;
			if (!slang_assembly_file_push_label (file, slang_asm_local_free, 4))
				return 0;
		}
		else
		{
			if (!_slang_cleanup_stack (file, params + j, 0, space))
				return 0;
		}
	}

	return 1;
}

int call_function_name (slang_assembly_file *file, const char *name, slang_operation *params,
	unsigned int param_count, int assignment, slang_assembly_name_space *space,
	slang_assembly_local_info *info)
{
	slang_function *fun = _slang_locate_function (name, params, param_count, space);
	if (fun == NULL)
		return 0;
	return call_function (file, fun, params, param_count, assignment, space, info);
}

static int call_function_name_dummyint (slang_assembly_file *file, const char *name,
	slang_operation *params, slang_assembly_name_space *space, slang_assembly_local_info *info)
{
	slang_operation p2[2];
	int result;

	p2[0] = *params;
	if (!slang_operation_construct_a (p2 + 1))
		return 0;
	p2[1].type = slang_oper_literal_int;
	result = call_function_name (file, name, p2, 2, 0, space, info);
	slang_operation_destruct (p2 + 1);
	return result;
}

static int call_asm_instruction (slang_assembly_file *file, const char *name)
{
	const struct
	{
		const char *name;
		slang_assembly_type code1, code2;
	} inst[] = {
		{ "float_to_int",   slang_asm_float_to_int,   slang_asm_int_copy },
		{ "int_to_float",   slang_asm_int_to_float,   slang_asm_float_copy },
		{ "float_copy",     slang_asm_float_copy,     slang_asm_none },
		{ "int_copy",       slang_asm_int_copy,       slang_asm_none },
		{ "bool_copy",      slang_asm_bool_copy,      slang_asm_none },
		{ "float_add",      slang_asm_float_add,      slang_asm_float_copy },
		{ "float_multiply", slang_asm_float_multiply, slang_asm_float_copy },
		{ "float_divide",   slang_asm_float_divide,   slang_asm_float_copy },
		{ "float_negate",   slang_asm_float_negate,   slang_asm_float_copy },
		{ "float_less",     slang_asm_float_less,     slang_asm_bool_copy },
		{ "float_equal",    slang_asm_float_equal,    slang_asm_bool_copy },
		{ NULL,             slang_asm_none,           slang_asm_none }
	};
	unsigned int i;

	for (i = 0; inst[i].name != NULL; i++)
		if (slang_string_compare (name, inst[i].name) == 0)
			break;
	if (inst[i].name == NULL)
		return 0;

	if (!slang_assembly_file_push_label2 (file, inst[i].code1, 4, 0))
		return 0;
	if (inst[i].code2 != slang_asm_none)
		if (!slang_assembly_file_push_label2 (file, inst[i].code2, 4, 0))
			return 0;

	/* clean-up the stack from the remaining dst address */
	if (!slang_assembly_file_push_label (file, slang_asm_local_free, 4))
		return 0;

	return 1;
}

/* XXX: general swizzle! */
static int equality_aggregate (slang_assembly_file *file, const slang_storage_aggregate *agg,
	unsigned int *index, unsigned int size, slang_assembly_local_info *info, unsigned int z_label)
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
				if (!equality_aggregate (file, arr->aggregate, index, size, info, z_label))
					return 0;
			}
			else
			{
				if (!slang_assembly_file_push_label2 (file, slang_asm_float_equal, size + *index,
					*index))
					return 0;
				*index += 4;
				if (!slang_assembly_file_push_label (file, slang_asm_jump_if_zero, z_label))
					return 0;
			}
		}
	}
	return 1;
}
/* XXX: general swizzle! */
static int equality (slang_assembly_file *file, slang_operation *op,
	slang_assembly_name_space *space, slang_assembly_local_info *info, int equal)
{
	slang_assembly_typeinfo ti;
	int result;
	slang_storage_aggregate agg;
	unsigned int index, size;
	unsigned int skip_jump, true_label, true_jump, false_label, false_jump;

	/* get type of operation */
	slang_assembly_typeinfo_construct (&ti);
	if (!_slang_typeof_operation (op, space, &ti))
	{
		slang_assembly_typeinfo_destruct (&ti);
		return 0;
	}

	/* convert it to an aggregate */
	slang_storage_aggregate_construct (&agg);
	if (!(result = _slang_aggregate_variable (&agg, &ti.spec, NULL, space->funcs, space->structs)))
		goto end;

	/* compute the size of the agregate - there are two such aggregates on the stack */
	size = _slang_sizeof_aggregate (&agg);

	/* jump to the actual data-comparison code */
	skip_jump = file->count;
	if (!(result = slang_assembly_file_push (file, slang_asm_jump)))
		goto end;

	/* pop off the stack the compared data and push 1 */
	true_label = file->count;
	if (!(result = slang_assembly_file_push_label (file, slang_asm_local_free, size * 2)))
		goto end;
	if (!(result = slang_assembly_file_push_literal (file, slang_asm_bool_push, 1.0f)))
		goto end;
	true_jump = file->count;
	if (!(result = slang_assembly_file_push (file, slang_asm_jump)))
		goto end;

	false_label = file->count;
	if (!(result = slang_assembly_file_push_label (file, slang_asm_local_free, size * 2)))
		goto end;
	if (!(result = slang_assembly_file_push_literal (file, slang_asm_bool_push, 0.0f)))
		goto end;
	false_jump = file->count;
	if (!(result = slang_assembly_file_push (file, slang_asm_jump)))
		goto end;

	file->code[skip_jump].param[0] = file->count;

	/* compare the data on stack, it will eventually jump either to true or false label */
	index = 0;
	if (!(result = equality_aggregate (file, &agg, &index, size, info,
		equal ? false_label : true_label)))
		goto end;
	if (!(result = slang_assembly_file_push_label (file, slang_asm_jump,
		equal ? true_label : false_label)))
		goto end;

	file->code[true_jump].param[0] = file->count;
	file->code[false_jump].param[0] = file->count;

	result = 1;
end:
	slang_storage_aggregate_destruct (&agg);
	slang_assembly_typeinfo_destruct (&ti);
	return result;
}

int _slang_assemble_operation (slang_assembly_file *file, slang_operation *op, int reference,
	slang_assembly_flow_control *flow, slang_assembly_name_space *space,
	slang_assembly_local_info *info, slang_assembly_stack_info *stk)
{
	unsigned int assem;

	stk->swizzle_mask = 0;

	assem = file->count;
	if (!slang_assembly_file_push_new (file))
		return 0;

	switch (op->type)
	{
	case slang_oper_block_no_new_scope:
	case slang_oper_block_new_scope:
		{
			unsigned int i;
			for (i = 0; i < op->num_children; i++)
			{
				slang_assembly_stack_info stk;
				if (!_slang_assemble_operation (file, op->children + i, 0, flow, space, info, &stk))
					return 0;
				/* TODO: pass-in stk to cleanup */
				if (!_slang_cleanup_stack (file, op->children + i, 0, space))
					return 0;
			}
		}
		break;
	case slang_oper_variable_decl:
		{
			unsigned int i;

			for (i = 0; i < op->num_children; i++)
			{
				/* TODO: perform initialization of op->children[i] */
				/* TODO: clean-up stack */
			}
		}
		break;
	case slang_oper_asm:
		{
			unsigned int i;
			for (i = 0; i < op->num_children; i++)
			{
				slang_assembly_stack_info stk;
				if (!_slang_assemble_operation (file, op->children + i, i == 0, flow, space, info,
					&stk))
					return 0;
				/* TODO: inspect stk */
			}
			if (!call_asm_instruction (file, op->identifier))
				return 0;
		}
		break;
	case slang_oper_break:
		file->code[assem].type = slang_asm_jump;
		file->code[assem].param[0] = flow->loop_end;
		break;
	case slang_oper_continue:
		file->code[assem].type = slang_asm_jump;
		file->code[assem].param[0] = flow->loop_start;
		break;
	case slang_oper_discard:
		file->code[assem].type = slang_asm_discard;
		if (!slang_assembly_file_push (file, slang_asm_exit))
			return 0;
		break;
	case slang_oper_return:
		if (info->ret_size != 0)
		{
			slang_assembly_stack_info stk;
			if (!slang_assembly_file_push_label2 (file, slang_asm_local_addr, 0, info->ret_size))
				return 0;
			if (!_slang_assemble_operation (file, op->children, 0, flow, space, info, &stk))
				return 0;
			/* TODO: inspect stk */
			if (!_slang_assemble_assignment (file, op->children, space, info))
				return 0;
			if (!slang_assembly_file_push_label (file, slang_asm_local_free, 4))
				return 0;
		}
		if (!slang_assembly_file_push_label (file, slang_asm_jump, flow->function_end))
			return 0;
		break;
	case slang_oper_expression:
		{
			slang_assembly_stack_info stk;
			if (!_slang_assemble_operation (file, op->children, reference, flow, space, info, &stk))
				return 0;
			/* TODO: inspect stk */
		}
		break;
	case slang_oper_if:
		if (!_slang_assemble_if (file, op, flow, space, info))
			return 0;
		break;
	case slang_oper_while:
		if (!_slang_assemble_while (file, op, flow, space, info))
			return 0;
		break;
	case slang_oper_do:
		if (!_slang_assemble_do (file, op, flow, space, info))
			return 0;
		break;
	case slang_oper_for:
		if (!_slang_assemble_for (file, op, flow, space, info))
			return 0;
		break;
	case slang_oper_void:
		break;
	case slang_oper_literal_bool:
		file->code[assem].type = slang_asm_bool_push;
		file->code[assem].literal = op->literal;
		break;
	case slang_oper_literal_int:
		file->code[assem].type = slang_asm_int_push;
		file->code[assem].literal = op->literal;
		break;
	case slang_oper_literal_float:
		file->code[assem].type = slang_asm_float_push;
		file->code[assem].literal = op->literal;
		break;
	case slang_oper_identifier:
		{
			slang_variable *var;
			unsigned int size;
			var = _slang_locate_variable (op->locals, op->identifier, 1);
			if (var == NULL)
				return 0;
			size = 0;
			if (!sizeof_variable (&var->type.specifier, slang_qual_none, var->array_size, space,
				&size))
				return 0;
			if (var->initializer != NULL)
			{
				assert (!"var->initializer, oper_identifier");
			}
			else
			{
				if (!reference)
				{
					if (!slang_assembly_file_push_label2 (file, slang_asm_local_addr,
						info->addr_tmp, 4))
						return 0;
				}
				/* XXX: globals! */
				if (!slang_assembly_file_push_label2 (file, slang_asm_local_addr, var->address,
					size))
					return 0;
				if (!reference)
				{
					if (!slang_assembly_file_push (file, slang_asm_addr_copy))
						return 0;
					if (!slang_assembly_file_push_label (file, slang_asm_local_free, 4))
						return 0;
					if (!dereference (file, op, space, info))
						return 0;
				}
			}
		}
		break;
	case slang_oper_sequence:
		{
			slang_assembly_stack_info stk;
			if (!_slang_assemble_operation (file, op->children, 0, flow, space, info, &stk))
				return 0;
			/* TODO: pass-in stk to cleanup */
			if (!_slang_cleanup_stack (file, op->children, 0, space))
				return 0;
			if (!_slang_assemble_operation (file, op->children + 1, 0, flow, space, info,
				&stk))
				return 0;
			/* TODO: inspect stk */
		}
		break;
	case slang_oper_assign:
		if (!_slang_assemble_assign (file, op, "=", reference, space, info))
			return 0;
		break;
	case slang_oper_addassign:
		if (!_slang_assemble_assign (file, op, "+=", reference, space, info))
			return 0;
		break;
	case slang_oper_subassign:
		if (!_slang_assemble_assign (file, op, "-=", reference, space, info))
			return 0;
		break;
	case slang_oper_mulassign:
		if (!_slang_assemble_assign (file, op, "*=", reference, space, info))
			return 0;
		break;
	/*case slang_oper_modassign:*/
	/*case slang_oper_lshassign:*/
	/*case slang_oper_rshassign:*/
	/*case slang_oper_orassign:*/
	/*case slang_oper_xorassign:*/
	/*case slang_oper_andassign:*/
	case slang_oper_divassign:
		if (!_slang_assemble_assign (file, op, "/=", reference, space, info))
			return 0;
		break;
	case slang_oper_select:
		if (!_slang_assemble_select (file, op, flow, space, info))
			return 0;
		break;
	case slang_oper_logicalor:
		if (!_slang_assemble_logicalor (file, op, flow, space, info))
			return 0;
		break;
	case slang_oper_logicaland:
		if (!_slang_assemble_logicaland (file, op, flow, space, info))
			return 0;
		break;
	case slang_oper_logicalxor:
		if (!call_function_name (file, "^^", op->children, 2, 0, space, info))
			return 0;
		break;
	/*case slang_oper_bitor:*/
	/*case slang_oper_bitxor:*/
	/*case slang_oper_bitand:*/
	case slang_oper_less:
		if (!call_function_name (file, "<", op->children, 2, 0, space, info))
			return 0;
		break;
	case slang_oper_greater:
		if (!call_function_name (file, ">", op->children, 2, 0, space, info))
			return 0;
		break;
	case slang_oper_lessequal:
		if (!call_function_name (file, "<=", op->children, 2, 0, space, info))
			return 0;
		break;
	case slang_oper_greaterequal:
		if (!call_function_name (file, ">=", op->children, 2, 0, space, info))
			return 0;
		break;
	/*case slang_oper_lshift:*/
	/*case slang_oper_rshift:*/
	case slang_oper_add:
		if (!call_function_name (file, "+", op->children, 2, 0, space, info))
			return 0;
		break;
	case slang_oper_subtract:
		if (!call_function_name (file, "-", op->children, 2, 0, space, info))
			return 0;
		break;
	case slang_oper_multiply:
		if (!call_function_name (file, "*", op->children, 2, 0, space, info))
			return 0;
		break;
	/*case slang_oper_modulus:*/
	case slang_oper_divide:
		if (!call_function_name (file, "/", op->children, 2, 0, space, info))
			return 0;
		break;
	case slang_oper_equal:
		{
			slang_assembly_stack_info stk;
			if (!_slang_assemble_operation (file, op->children, 0, flow, space, info, &stk))
				return 0;
			/* TODO: inspect stk */
			if (!_slang_assemble_operation (file, op->children + 1, 0, flow, space, info, &stk))
				return 0;
			/* TODO: inspect stk */
			if (!equality (file, op->children, space, info, 1))
				return 0;
		}
		break;
	case slang_oper_notequal:
		{
			slang_assembly_stack_info stk;
			if (!_slang_assemble_operation (file, op->children, 0, flow, space, info, &stk))
				return 0;
			/* TODO: inspect stk */
			if (!_slang_assemble_operation (file, op->children + 1, 0, flow, space, info, &stk))
				return 0;
			/* TODO: inspect stk */
			if (!equality (file, op->children, space, info, 0))
				return 0;
		}
		break;
	case slang_oper_preincrement:
		if (!_slang_assemble_assign (file, op, "++", reference, space, info))
			return 0;
		break;
	case slang_oper_predecrement:
		if (!_slang_assemble_assign (file, op, "--", reference, space, info))
			return 0;
		break;
	case slang_oper_plus:
		if (!call_function_name (file, "+", op->children, 1, 0, space, info))
			return 0;
		break;
	case slang_oper_minus:
		if (!call_function_name (file, "-", op->children, 1, 0, space, info))
			return 0;
		break;
	/*case slang_oper_complement:*/
	case slang_oper_not:
		if (!call_function_name (file, "!", op->children, 1, 0, space, info))
			return 0;
		break;
	case slang_oper_subscript:
		{
			slang_assembly_stack_info _stk;
			slang_assembly_typeinfo ti_arr, ti_elem;
			unsigned int arr_size = 0, elem_size = 0;
			if (!_slang_assemble_operation (file, op->children, reference, flow, space, info,
				&_stk))
				return 0;
			if (!_slang_assemble_operation (file, op->children + 1, 0, flow, space, info, &_stk))
				return 0;
			slang_assembly_typeinfo_construct (&ti_arr);
			if (!_slang_typeof_operation (op->children, space, &ti_arr))
			{
				slang_assembly_typeinfo_destruct (&ti_arr);
				return 0;
			}
			if (!sizeof_variable (&ti_arr.spec, slang_qual_none, NULL, space, &arr_size))
			{
				slang_assembly_typeinfo_destruct (&ti_arr);
				return 0;
			}
			slang_assembly_typeinfo_construct (&ti_elem);
			if (!_slang_typeof_operation (op, space, &ti_elem))
			{
				slang_assembly_typeinfo_destruct (&ti_arr);
				slang_assembly_typeinfo_destruct (&ti_elem);
				return 0;
			}
			if (!sizeof_variable (&ti_elem.spec, slang_qual_none, NULL, space, &elem_size))
			{
				slang_assembly_typeinfo_destruct (&ti_arr);
				slang_assembly_typeinfo_destruct (&ti_elem);
				return 0;
			}
			if (!slang_assembly_file_push (file, slang_asm_int_to_addr))
			{
				slang_assembly_typeinfo_destruct (&ti_arr);
				slang_assembly_typeinfo_destruct (&ti_elem);
				return 0;
			}
			if (!slang_assembly_file_push_label (file, slang_asm_addr_push, elem_size))
			{
				slang_assembly_typeinfo_destruct (&ti_arr);
				slang_assembly_typeinfo_destruct (&ti_elem);
				return 0;
			}
			if (!slang_assembly_file_push (file, slang_asm_addr_multiply))
			{
				slang_assembly_typeinfo_destruct (&ti_arr);
				slang_assembly_typeinfo_destruct (&ti_elem);
				return 0;
			}
			if (reference)
			{
				if (!slang_assembly_file_push (file, slang_asm_addr_add))
				{
					slang_assembly_typeinfo_destruct (&ti_arr);
					slang_assembly_typeinfo_destruct (&ti_elem);
					return 0;
				}
			}
			else
			{
				unsigned int i;
				for (i = 0; i < elem_size; i += 4)
				{
					if (!slang_assembly_file_push_label2 (file, slang_asm_float_move,
						arr_size - elem_size + i + 4, i + 4))
					{
						slang_assembly_typeinfo_destruct (&ti_arr);
						slang_assembly_typeinfo_destruct (&ti_elem);
						return 0;
					}
				}
				if (!slang_assembly_file_push_label (file, slang_asm_local_free, 4))
				{
					slang_assembly_typeinfo_destruct (&ti_arr);
					slang_assembly_typeinfo_destruct (&ti_elem);
					return 0;
				}
				if (!slang_assembly_file_push_label (file, slang_asm_local_free,
					arr_size - elem_size))
				{
					slang_assembly_typeinfo_destruct (&ti_arr);
					slang_assembly_typeinfo_destruct (&ti_elem);
					return 0;
				}
			}
			slang_assembly_typeinfo_destruct (&ti_arr);
			slang_assembly_typeinfo_destruct (&ti_elem);
		}
		break;
	case slang_oper_call:
		{
			slang_function *fun = _slang_locate_function (op->identifier, op->children,
				op->num_children, space);
			if (fun == NULL)
			{
				if (!_slang_assemble_constructor (file, op, flow, space, info))
					return 0;
			}
			else
			{
				if (!call_function (file, fun, op->children, op->num_children, 0, space, info))
					return 0;
			}
		}
		break;
	case slang_oper_field:
		{
			slang_assembly_typeinfo ti_after, ti_before;
			slang_assembly_stack_info _stk;
			slang_assembly_typeinfo_construct (&ti_after);
			if (!_slang_typeof_operation (op, space, &ti_after))
			{
				slang_assembly_typeinfo_destruct (&ti_after);
				return 0;
			}
			slang_assembly_typeinfo_construct (&ti_before);
			if (!_slang_typeof_operation (op->children, space, &ti_before))
			{
				slang_assembly_typeinfo_destruct (&ti_after);
				slang_assembly_typeinfo_destruct (&ti_before);
				return 0;
			}
			if (!reference && ti_after.is_swizzled)
			{
				if (!slang_assembly_file_push_label2 (file, slang_asm_local_addr,
					info->swizzle_tmp, 16))
				{
					slang_assembly_typeinfo_destruct (&ti_after);
					slang_assembly_typeinfo_destruct (&ti_before);
					return 0;
				}
			}
			if (!_slang_assemble_operation (file, op->children, reference, flow, space, info,
				&_stk))
			{
				slang_assembly_typeinfo_destruct (&ti_after);
				slang_assembly_typeinfo_destruct (&ti_before);
				return 0;
			}
			/* TODO: inspect stk */
			if (ti_after.is_swizzled)
			{
				if (reference)
				{
					if (ti_after.swz.num_components == 1)
					{
						if (!slang_assembly_file_push_label (file, slang_asm_addr_push,
							ti_after.swz.swizzle[0] * 4))
						{
							slang_assembly_typeinfo_destruct (&ti_after);
							slang_assembly_typeinfo_destruct (&ti_before);
							return 0;
						}
						if (!slang_assembly_file_push (file, slang_asm_addr_add))
						{
							slang_assembly_typeinfo_destruct (&ti_after);
							slang_assembly_typeinfo_destruct (&ti_before);
							return 0;
						}
					}
					else
					{
						unsigned int i;
						for (i = 0; i < ti_after.swz.num_components; i++)
							stk->swizzle_mask |= 1 << ti_after.swz.swizzle[i];
					}
				}
				else
				{
					if (!_slang_assemble_constructor_from_swizzle (file, &ti_after.swz,
						&ti_after.spec, &ti_before.spec, info))
					{
						slang_assembly_typeinfo_destruct (&ti_after);
						slang_assembly_typeinfo_destruct (&ti_before);
						return 0;
					}
				}
			}
			else
			{
				if (reference)
				{
					/* TODO: struct field address */
				}
				else
				{
					/* TODO: struct field value */
				}
			}
			slang_assembly_typeinfo_destruct (&ti_after);
			slang_assembly_typeinfo_destruct (&ti_before);
		}
		break;
	case slang_oper_postincrement:
		if (!call_function_name_dummyint (file, "++", op->children, space, info))
			return 0;
		if (!dereference (file, op, space, info))
			return 0;
		break;
	case slang_oper_postdecrement:
		if (!call_function_name_dummyint (file, "--", op->children, space, info))
			return 0;
		if (!dereference (file, op, space, info))
			return 0;
		break;
	default:
		return 0;
	}
	return 1;
}









void xxx_first (slang_assembly_file *file)
{
	slang_assembly_file_push (file, slang_asm_jump);
}

void xxx_prolog (slang_assembly_file *file, unsigned int addr)
{
	file->code[0].param[0] = file->count;
	slang_assembly_file_push_label (file, slang_asm_call, addr);
	slang_assembly_file_push (file, slang_asm_exit);
}

