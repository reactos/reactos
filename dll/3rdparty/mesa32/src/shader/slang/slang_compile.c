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
 * \file slang_compile.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "imports.h"
#include "grammar_mesa.h"
#include "slang_utility.h"
#include "slang_compile.h"
#include "slang_preprocess.h"
#include "slang_storage.h"
#include "slang_assemble.h"
#include "slang_execute.h"

/*
	This is a straightforward implementation of the slang front-end compiler.
	Lots of error-checking functionality is missing but every well-formed shader source should
	compile successfully and execute as expected. However, some semantically ill-formed shaders
	may be accepted resulting in undefined behaviour.
*/

static void slang_variable_construct (slang_variable *);
static int slang_variable_copy (slang_variable *, const slang_variable *);
static void slang_struct_destruct (slang_struct *);
static int slang_struct_equal (const slang_struct *, const slang_struct *);
static void slang_variable_destruct (slang_variable *);

/* slang_type_specifier_type */

/* these must match with slang_type_specifier_type enum */
static const char *type_specifier_type_names[] = {
	"void",
	"bool",
	"bvec2",
	"bvec3",
	"bvec4",
	"int",
	"ivec2",
	"ivec3",
	"ivec4",
	"float",
	"vec2",
	"vec3",
	"vec4",
	"mat2",
	"mat3",
	"mat4",
	"sampler1D",
	"sampler2D",
	"sampler3D",
	"samplerCube",
	"sampler1DShadow",
	"sampler2DShadow",
	NULL
};

slang_type_specifier_type slang_type_specifier_type_from_string (const char *name)
{
	const char **p = type_specifier_type_names;
	while (*p != NULL)
	{
		if (slang_string_compare (*p, name) == 0)
			return (slang_type_specifier_type) (p - type_specifier_type_names);
		p++;
	}
	return slang_spec_void;
}

/* slang_type_specifier */

void slang_type_specifier_construct (slang_type_specifier *spec)
{
	spec->type = slang_spec_void;
	spec->_struct = NULL;
	spec->_array = NULL;
}

void slang_type_specifier_destruct (slang_type_specifier *spec)
{
	if (spec->_struct != NULL)
	{
		slang_struct_destruct (spec->_struct);
		slang_alloc_free (spec->_struct);
	}
	if (spec->_array != NULL)
	{
		slang_type_specifier_destruct (spec->_array);
		slang_alloc_free (spec->_array);
	}
}

int slang_type_specifier_copy (slang_type_specifier *x, const slang_type_specifier *y)
{
	slang_type_specifier_destruct (x);
	slang_type_specifier_construct (x);
	x->type = y->type;
	if (x->type == slang_spec_struct)
	{
		x->_struct = (slang_struct *) slang_alloc_malloc (sizeof (slang_struct));
		if (x->_struct == NULL)
			return 0;
		if (!slang_struct_construct_a (x->_struct))
		{
			slang_alloc_free (x->_struct);
			x->_struct = NULL;
			return 0;
		}
		return slang_struct_copy (x->_struct, y->_struct);
	}
	if (x->type == slang_spec_array)
	{
		x->_array = (slang_type_specifier *) slang_alloc_malloc (sizeof (slang_type_specifier));
		if (x->_array == NULL)
			return 0;
		slang_type_specifier_construct (x->_array);
		return slang_type_specifier_copy (x->_array, y->_array);
	}
	return 1;
}

int slang_type_specifier_equal (const slang_type_specifier *x, const slang_type_specifier *y)
{
	if (x->type != y->type)
		return 0;
	if (x->type == slang_spec_struct)
		return slang_struct_equal (x->_struct, y->_struct);
	if (x->type == slang_spec_array)
		return slang_type_specifier_equal (x->_array, y->_array);
	return 1;
}

/* slang_fully_specified_type */

static void slang_fully_specified_type_construct (slang_fully_specified_type *type)
{
	type->qualifier = slang_qual_none;
	slang_type_specifier_construct (&type->specifier);
}

static void slang_fully_specified_type_destruct (slang_fully_specified_type *type)
{
	slang_type_specifier_destruct (&type->specifier);
}

static int slang_fully_specified_type_copy (slang_fully_specified_type *x,
	const slang_fully_specified_type *y)
{
	slang_fully_specified_type_construct (x);
	slang_fully_specified_type_destruct (x);
	x->qualifier = y->qualifier;
	return slang_type_specifier_copy (&x->specifier, &y->specifier);
}

/* slang_variable_scope */

static void slang_variable_scope_construct (slang_variable_scope *scope)
{
	scope->variables = NULL;
	scope->num_variables = 0;
	scope->outer_scope = NULL;
}

static void slang_variable_scope_destruct (slang_variable_scope *scope)
{
	unsigned int i;
	for (i = 0; i < scope->num_variables; i++)
		slang_variable_destruct (scope->variables + i);
	slang_alloc_free (scope->variables);
}

static int slang_variable_scope_copy (slang_variable_scope *x, const slang_variable_scope *y)
{
	unsigned int i;
	slang_variable_scope_destruct (x);
	slang_variable_scope_construct (x);
	x->variables = (slang_variable *) slang_alloc_malloc (y->num_variables * sizeof (
		slang_variable));
	if (x->variables == NULL)
		return 0;
	x->num_variables = y->num_variables;
	for (i = 0; i < x->num_variables; i++)
		slang_variable_construct (x->variables + i);
	for (i = 0; i < x->num_variables; i++)
		if (!slang_variable_copy (x->variables + i, y->variables + i))
			return 0;
	x->outer_scope = y->outer_scope;
	return 1;
}

/* slang_operation */

int slang_operation_construct_a (slang_operation *oper)
{
	oper->type = slang_oper_none;
	oper->children = NULL;
	oper->num_children = 0;
	oper->literal = (float) 0;
	oper->identifier = NULL;
	oper->locals = (slang_variable_scope *) slang_alloc_malloc (sizeof (slang_variable_scope));
	if (oper->locals == NULL)
		return 0;
	slang_variable_scope_construct (oper->locals);
	return 1;
}

void slang_operation_destruct (slang_operation *oper)
{
	unsigned int i;
	for (i = 0; i < oper->num_children; i++)
		slang_operation_destruct (oper->children + i);
	slang_alloc_free (oper->children);
	slang_alloc_free (oper->identifier);
	slang_variable_scope_destruct (oper->locals);
	slang_alloc_free (oper->locals);
}

static int slang_operation_copy (slang_operation *x, const slang_operation *y)
{
	unsigned int i;
	for (i = 0; i < x->num_children; i++)
		slang_operation_destruct (x->children + i);
	slang_alloc_free (x->children);
	x->num_children = 0;
	slang_alloc_free (x->identifier);
	x->identifier = NULL;
	slang_variable_scope_destruct (x->locals);
	slang_variable_scope_construct (x->locals);

	x->type = y->type;
	x->children = (slang_operation *) slang_alloc_malloc (y->num_children * sizeof (
		slang_operation));
	if (x->children == NULL)
		return 0;
	for (i = 0; i < y->num_children; i++)
		if (!slang_operation_construct_a (x->children + i))
		{
			unsigned int j;
			for (j = 0; j < i; j++)
				slang_operation_destruct (x->children + j);
			slang_alloc_free (x->children);
			x->children = NULL;
			return 0;
		}
	x->num_children = y->num_children;
	for (i = 0; i < x->num_children; i++)
		if (!slang_operation_copy (x->children + i, y->children + i))
			return 0;
	x->literal = y->literal;
	if (y->identifier != NULL)
	{
		x->identifier = slang_string_duplicate (y->identifier);
		if (x->identifier == NULL)
			return 0;
	}
	if (!slang_variable_scope_copy (x->locals, y->locals))
		return 0;
	return 1;
}

/* slang_variable */

static void slang_variable_construct (slang_variable *var)
{
	slang_fully_specified_type_construct (&var->type);
	var->name = NULL;
	var->array_size = NULL;
	var->initializer = NULL;
	var->address = ~0;
}

static void slang_variable_destruct (slang_variable *var)
{
	slang_fully_specified_type_destruct (&var->type);
	slang_alloc_free (var->name);
	if (var->array_size != NULL)
	{
		slang_operation_destruct (var->array_size);
		slang_alloc_free (var->array_size);
	}
	if (var->initializer != NULL)
	{
		slang_operation_destruct (var->initializer);
		slang_alloc_free (var->initializer);
	}
}

static int slang_variable_copy (slang_variable *x, const slang_variable *y)
{
	slang_variable_destruct (x);
	slang_variable_construct (x);
	if (!slang_fully_specified_type_copy (&x->type, &y->type))
		return 0;
	if (y->name != NULL)
	{
		x->name = slang_string_duplicate (y->name);
		if (x->name == NULL)
			return 0;
	}
	if (y->array_size != NULL)
	{
		x->array_size = (slang_operation *) slang_alloc_malloc (sizeof (slang_operation));
		if (x->array_size == NULL)
			return 0;
		if (!slang_operation_construct_a (x->array_size))
		{
			slang_alloc_free (x->array_size);
			x->array_size = NULL;
			return 0;
		}
		if (!slang_operation_copy (x->array_size, y->array_size))
			return 0;
	}
	if (y->initializer != NULL)
	{
		x->initializer = (slang_operation *) slang_alloc_malloc (sizeof (slang_operation));
		if (x->initializer == NULL)
			return 0;
		if (!slang_operation_construct_a (x->initializer))
		{
			slang_alloc_free (x->initializer);
			x->initializer = NULL;
			return 0;
		}
		if (!slang_operation_copy (x->initializer, y->initializer))
			return 0;
	}
	return 1;
}

slang_variable *_slang_locate_variable (slang_variable_scope *scope, const char *name, int all)
{
	unsigned int i;
	for (i = 0; i < scope->num_variables; i++)
		if (slang_string_compare (name, scope->variables[i].name) == 0)
			return scope->variables + i;
	if (all && scope->outer_scope != NULL)
		return _slang_locate_variable (scope->outer_scope, name, 1);
	return NULL;
}

/* slang_struct_scope */

static void slang_struct_scope_construct (slang_struct_scope *scope)
{
	scope->structs = NULL;
	scope->num_structs = 0;
	scope->outer_scope = NULL;
}

static void slang_struct_scope_destruct (slang_struct_scope *scope)
{
	unsigned int i;
	for (i = 0; i < scope->num_structs; i++)
		slang_struct_destruct (scope->structs + i);
	slang_alloc_free (scope->structs);
}

static int slang_struct_scope_copy (slang_struct_scope *x, const slang_struct_scope *y)
{
	unsigned int i;
	slang_struct_scope_destruct (x);
	slang_struct_scope_construct (x);
	x->structs = (slang_struct *) slang_alloc_malloc (y->num_structs * sizeof (slang_struct));
	if (x->structs == NULL)
		return 0;
	x->num_structs = y->num_structs;
	for (i = 0; i < x->num_structs; i++)
	{
		unsigned int j;
		if (!slang_struct_construct_a (x->structs + i))
		{
			for (j = 0; j < i; j++)
				slang_struct_destruct (x->structs + j);
			slang_alloc_free (x->structs);
			x->structs = NULL;
			return 0;
		}
	}
	for (i = 0; i < x->num_structs; i++)
		if (!slang_struct_copy (x->structs + i, y->structs + i))
			return 0;
	x->outer_scope = y->outer_scope;
	return 1;
}

slang_struct *slang_struct_scope_find (slang_struct_scope *stru, const char *name, int all_scopes)
{
	unsigned int i;
	for (i = 0; i < stru->num_structs; i++)
		if (slang_string_compare (name, stru->structs[i].name) == 0)
			return stru->structs + i;
	if (all_scopes && stru->outer_scope != NULL)
		return slang_struct_scope_find (stru->outer_scope, name, 1);
	return NULL;
}

/* slang_struct */

int slang_struct_construct_a (slang_struct *stru)
{
	stru->name = NULL;
	stru->fields = (slang_variable_scope *) slang_alloc_malloc (sizeof (slang_variable_scope));
	if (stru->fields == NULL)
		return 0;
	slang_variable_scope_construct (stru->fields);
	stru->structs = (slang_struct_scope *) slang_alloc_malloc (sizeof (slang_struct_scope));
	if (stru->structs == NULL)
	{
		slang_variable_scope_destruct (stru->fields);
		slang_alloc_free (stru->fields);
		return 0;
	}
	slang_struct_scope_construct (stru->structs);
	return 1;
}

static void slang_struct_destruct (slang_struct *stru)
{
	slang_alloc_free (stru->name);
	slang_variable_scope_destruct (stru->fields);
	slang_alloc_free (stru->fields);
	slang_struct_scope_destruct (stru->structs);
	slang_alloc_free (stru->structs);
}

int slang_struct_copy (slang_struct *x, const slang_struct *y)
{
	slang_alloc_free (x->name);
	x->name = NULL;
	slang_variable_scope_destruct (x->fields);
	slang_variable_scope_construct (x->fields);
	slang_struct_scope_destruct (x->structs);
	slang_struct_scope_construct (x->structs);
	if (y->name != NULL)
	{
		x->name = slang_string_duplicate (y->name);
		if (x->name == NULL)
			return 0;
	}
	if (!slang_variable_scope_copy (x->fields, y->fields))
		return 0;
	if (!slang_struct_scope_copy (x->structs, y->structs))
		return 0;
	return 1;
}

static int slang_struct_equal (const slang_struct *x, const slang_struct *y)
{
	unsigned int i;
	if (x->fields->num_variables != y->fields->num_variables)
		return 0;
	for (i = 0; i < x->fields->num_variables; i++)
	{
		slang_variable *varx = x->fields->variables + i;
		slang_variable *vary = y->fields->variables + i;
		if (slang_string_compare (varx->name, vary->name) != 0)
			return 0;
		if (!slang_type_specifier_equal (&varx->type.specifier, &vary->type.specifier))
			return 0;
		if (varx->type.specifier.type == slang_spec_array)
		{
			/* TODO compare array sizes */
		}
	}
	return 1;
}

/* slang_function */
/* XXX mem! */
static void slang_function_construct (slang_function *func)
{
	func->kind = slang_func_ordinary;
	slang_variable_construct (&func->header);
	func->parameters = (slang_variable_scope *) slang_alloc_malloc (sizeof (slang_variable_scope));
	slang_variable_scope_construct (func->parameters);
	func->body = NULL;
	func->address = ~0;
}

static void slang_function_destruct (slang_function *func)
{
	slang_variable_destruct (&func->header);
	slang_variable_scope_destruct (func->parameters);
	slang_alloc_free (func->parameters);
	if (func->body != NULL)
	{
		slang_operation_destruct (func->body);
		slang_alloc_free (func->body);
	}
}

/* slang_function_scope */

static void slang_function_scope_construct (slang_function_scope *scope)
{
	scope->functions = NULL;
	scope->num_functions = 0;
	scope->outer_scope = NULL;
}

static void slang_function_scope_destruct (slang_function_scope *scope)
{
	unsigned int i;
	for (i = 0; i < scope->num_functions; i++)
		slang_function_destruct (scope->functions + i);
	slang_alloc_free (scope->functions);
}

static int slang_function_scope_find_by_name (slang_function_scope *funcs, const char *name,
	int all_scopes)
{
	unsigned int i;
	for (i = 0; i < funcs->num_functions; i++)
		if (slang_string_compare (name, funcs->functions[i].header.name) == 0)
			return 1;
	if (all_scopes && funcs->outer_scope != NULL)
		return slang_function_scope_find_by_name (funcs->outer_scope, name, 1);
	return 0;
}

static slang_function *slang_function_scope_find (slang_function_scope *funcs, slang_function *fun,
	int all_scopes)
{
	unsigned int i;
	for (i = 0; i < funcs->num_functions; i++)
	{
		slang_function *f = funcs->functions + i;
		unsigned int j;
		if (slang_string_compare (fun->header.name, f->header.name) != 0)
			continue;
		if (fun->param_count != f->param_count)
			continue;
		for (j = 0; j < fun->param_count; j++)
		{
			if (!slang_type_specifier_equal (&fun->parameters->variables[j].type.specifier,
				&f->parameters->variables[j].type.specifier))
			{
				break;
			}
		}
		if (j == fun->param_count)
			return f;
	}
	if (all_scopes && funcs->outer_scope != NULL)
		return slang_function_scope_find (funcs->outer_scope, fun, 1);
	return NULL;
}

/* slang_translation_unit */

void slang_translation_unit_construct (slang_translation_unit *unit)
{
	slang_variable_scope_construct (&unit->globals);
	slang_function_scope_construct (&unit->functions);
	slang_struct_scope_construct (&unit->structs);
}

void slang_translation_unit_destruct (slang_translation_unit *unit)
{
	slang_variable_scope_destruct (&unit->globals);
	slang_function_scope_destruct (&unit->functions);
	slang_struct_scope_destruct (&unit->structs);
}

/* slang_info_log */

static char *out_of_memory = "error: out of memory\n";

void slang_info_log_construct (slang_info_log *log)
{
	log->text = NULL;
	log->dont_free_text = 0;
}

void slang_info_log_destruct (slang_info_log *log)
{
	if (!log->dont_free_text)
		slang_alloc_free (log->text);
}

static int slang_info_log_message (slang_info_log *log, const char *prefix, const char *msg)
{
	unsigned int new_size;
	if (log->dont_free_text)
		return 0;
	new_size = slang_string_length (prefix) + 3 + slang_string_length (msg);
	if (log->text != NULL)
	{
		log->text = (char *) slang_alloc_realloc (log->text, slang_string_length (log->text) + 1,
			new_size + slang_string_length (log->text) + 1);
	}
	else
	{
		log->text = (char *) slang_alloc_malloc (new_size + 1);
		if (log->text != NULL)
			*log->text = '\0';
	}
	if (log->text == NULL)
		return 0;
	slang_string_concat (log->text, prefix);
	slang_string_concat (log->text, ": ");
	slang_string_concat (log->text, msg);
	slang_string_concat (log->text, "\n");
	return 1;
}

int slang_info_log_error (slang_info_log *log, const char *msg, ...)
{
	va_list va;
	char buf[1024];

	va_start (va, msg);
	vsprintf (buf, msg, va);
	if (slang_info_log_message (log, "error", buf))
		return 1;
	slang_info_log_memory (log);
	va_end (va);
	return 0;
}

int slang_info_log_warning (slang_info_log *log, const char *msg, ...)
{
	va_list va;
	char buf[1024];

	va_start (va, msg);
	vsprintf (buf, msg, va);
	if (slang_info_log_message (log, "warning", buf))
		return 1;
	slang_info_log_memory (log);
	va_end (va);
	return 0;
}

void slang_info_log_memory (slang_info_log *log)
{
	if (!slang_info_log_message (log, "error", "out of memory"))
	{
		log->dont_free_text = 1;
		log->text = out_of_memory;
	}
}

/* slang_parse_ctx */

typedef struct slang_parse_ctx_
{
	const byte *I;
	slang_info_log *L;
	int parsing_builtin;
} slang_parse_ctx;

/* _slang_compile() */

static int parse_identifier (slang_parse_ctx *C, char **id)
{
	*id = slang_string_duplicate ((const char *) C->I);
	if (*id == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	C->I += strlen ((const char *) C->I) + 1;
	return 1;
}

static int parse_number (slang_parse_ctx *C, int *number)
{
	const int radix = (int) (*C->I++);
	*number = 0;
	while (*C->I != '\0')
	{
		int digit;
		if (*C->I >= '0' && *C->I <= '9')
			digit = (int) (*C->I - '0');
		else if (*C->I >= 'A' && *C->I <= 'Z')
			digit = (int) (*C->I - 'A') + 10;
		else
			digit = (int) (*C->I - 'a') + 10;
		*number = *number * radix + digit;
		C->I++;
	}
	C->I++;
	if (*number > 65535)
		slang_info_log_warning (C->L, "%d: literal integer overflow", *number);
	return 1;
}

static int parse_float (slang_parse_ctx *C, float *number)
{
	char *integral = NULL;
	char *fractional = NULL;
	char *exponent = NULL;
	char *whole = NULL;

	if (!parse_identifier (C, &integral))
		return 0;

	if (!parse_identifier (C, &fractional))
	{
		slang_alloc_free (integral);
		return 0;
	}

	if (!parse_identifier (C, &exponent))
	{
		slang_alloc_free (fractional);
		slang_alloc_free (integral);
		return 0;
	}

	whole = (char *) (slang_alloc_malloc ((strlen (integral) + strlen (fractional) + strlen (
		exponent) + 3) * sizeof (char)));
	if (whole == NULL)
	{
		slang_alloc_free (exponent);
		slang_alloc_free (fractional);
		slang_alloc_free (integral);
		slang_info_log_memory (C->L);
		return 0;
	}

	slang_string_copy (whole, integral);
	slang_string_concat (whole, ".");
	slang_string_concat (whole, fractional);
	slang_string_concat (whole, "E");
	slang_string_concat (whole, exponent);

	*number = (float) (atof (whole));

	slang_alloc_free (whole);
	slang_alloc_free (exponent);
	slang_alloc_free (fractional);
	slang_alloc_free (integral);
	return 1;
}

/* revision number - increment after each change affecting emitted output */
#define REVISION 2

static int check_revision (slang_parse_ctx *C)
{
	if (*C->I != REVISION)
	{
		slang_info_log_error (C->L, "internal compiler error");
		return 0;
	}
	C->I++;
	return 1;
}

static int parse_statement (slang_parse_ctx *, slang_operation *, slang_variable_scope *,
	slang_struct_scope *, slang_function_scope *);
static int parse_expression (slang_parse_ctx *, slang_operation *, slang_variable_scope *,
	slang_struct_scope *, slang_function_scope *);

/* type qualifier */
#define TYPE_QUALIFIER_NONE 0
#define TYPE_QUALIFIER_CONST 1
#define TYPE_QUALIFIER_ATTRIBUTE 2
#define TYPE_QUALIFIER_VARYING 3
#define TYPE_QUALIFIER_UNIFORM 4
#define TYPE_QUALIFIER_FIXEDOUTPUT 5
#define TYPE_QUALIFIER_FIXEDINPUT 6

static int parse_type_qualifier (slang_parse_ctx *C, slang_type_qualifier *qual)
{
	switch (*C->I++)
	{
	case TYPE_QUALIFIER_NONE:
		*qual = slang_qual_none;
		break;
	case TYPE_QUALIFIER_CONST:
		*qual = slang_qual_const;
		break;
	case TYPE_QUALIFIER_ATTRIBUTE:
		*qual = slang_qual_attribute;
		break;
	case TYPE_QUALIFIER_VARYING:
		*qual = slang_qual_varying;
		break;
	case TYPE_QUALIFIER_UNIFORM:
		*qual = slang_qual_uniform;
		break;
	case TYPE_QUALIFIER_FIXEDOUTPUT:
		*qual = slang_qual_fixedoutput;
		break;
	case TYPE_QUALIFIER_FIXEDINPUT:
		*qual = slang_qual_fixedinput;
		break;
	default:
		return 0;
	}
	return 1;
}

/* type specifier */
#define TYPE_SPECIFIER_VOID 0
#define TYPE_SPECIFIER_BOOL 1
#define TYPE_SPECIFIER_BVEC2 2
#define TYPE_SPECIFIER_BVEC3 3
#define TYPE_SPECIFIER_BVEC4 4
#define TYPE_SPECIFIER_INT 5
#define TYPE_SPECIFIER_IVEC2 6
#define TYPE_SPECIFIER_IVEC3 7
#define TYPE_SPECIFIER_IVEC4 8
#define TYPE_SPECIFIER_FLOAT 9
#define TYPE_SPECIFIER_VEC2 10
#define TYPE_SPECIFIER_VEC3 11
#define TYPE_SPECIFIER_VEC4 12
#define TYPE_SPECIFIER_MAT2 13
#define TYPE_SPECIFIER_MAT3 14
#define TYPE_SPECIFIER_MAT4 15
#define TYPE_SPECIFIER_SAMPLER1D 16
#define TYPE_SPECIFIER_SAMPLER2D 17
#define TYPE_SPECIFIER_SAMPLER3D 18
#define TYPE_SPECIFIER_SAMPLERCUBE 19
#define TYPE_SPECIFIER_SAMPLER1DSHADOW 20
#define TYPE_SPECIFIER_SAMPLER2DSHADOW 21
#define TYPE_SPECIFIER_STRUCT 22
#define TYPE_SPECIFIER_TYPENAME 23

/* structure field */
#define FIELD_NONE 0
#define FIELD_NEXT 1
#define FIELD_ARRAY 2

static int parse_type_specifier (slang_parse_ctx *C, slang_type_specifier *spec,
	slang_struct_scope *structs, slang_variable_scope *scope, slang_function_scope *funcs)
{
	switch (*C->I++)
	{
	case TYPE_SPECIFIER_VOID:
		spec->type = slang_spec_void;
		break;
	case TYPE_SPECIFIER_BOOL:
		spec->type = slang_spec_bool;
		break;
	case TYPE_SPECIFIER_BVEC2:
		spec->type = slang_spec_bvec2;
		break;
	case TYPE_SPECIFIER_BVEC3:
		spec->type = slang_spec_bvec3;
		break;
	case TYPE_SPECIFIER_BVEC4:
		spec->type = slang_spec_bvec4;
		break;
	case TYPE_SPECIFIER_INT:
		spec->type = slang_spec_int;
		break;
	case TYPE_SPECIFIER_IVEC2:
		spec->type = slang_spec_ivec2;
		break;
	case TYPE_SPECIFIER_IVEC3:
		spec->type = slang_spec_ivec3;
		break;
	case TYPE_SPECIFIER_IVEC4:
		spec->type = slang_spec_ivec4;
		break;
	case TYPE_SPECIFIER_FLOAT:
		spec->type = slang_spec_float;
		break;
	case TYPE_SPECIFIER_VEC2:
		spec->type = slang_spec_vec2;
		break;
	case TYPE_SPECIFIER_VEC3:
		spec->type = slang_spec_vec3;
		break;
	case TYPE_SPECIFIER_VEC4:
		spec->type = slang_spec_vec4;
		break;
	case TYPE_SPECIFIER_MAT2:
		spec->type = slang_spec_mat2;
		break;
	case TYPE_SPECIFIER_MAT3:
		spec->type = slang_spec_mat3;
		break;
	case TYPE_SPECIFIER_MAT4:
		spec->type = slang_spec_mat4;
		break;
	case TYPE_SPECIFIER_SAMPLER1D:
		spec->type = slang_spec_sampler1D;
		break;
	case TYPE_SPECIFIER_SAMPLER2D:
		spec->type = slang_spec_sampler2D;
		break;
	case TYPE_SPECIFIER_SAMPLER3D:
		spec->type = slang_spec_sampler3D;
		break;
	case TYPE_SPECIFIER_SAMPLERCUBE:
		spec->type = slang_spec_samplerCube;
		break;
	case TYPE_SPECIFIER_SAMPLER1DSHADOW:
		spec->type = slang_spec_sampler1DShadow;
		break;
	case TYPE_SPECIFIER_SAMPLER2DSHADOW:
		spec->type = slang_spec_sampler2DShadow;
		break;
	case TYPE_SPECIFIER_STRUCT:
		spec->type = slang_spec_struct;
		{
			char *name;
			if (!parse_identifier (C, &name))
				return 0;
			if (*name != '\0' && slang_struct_scope_find (structs, name, 0) != NULL)
			{
				slang_info_log_error (C->L, "%s: duplicate type name", name);
				slang_alloc_free (name);
				return 0;
			}
			spec->_struct = (slang_struct *) slang_alloc_malloc (sizeof (slang_struct));
			if (spec->_struct == NULL)
			{
				slang_alloc_free (name);
				slang_info_log_memory (C->L);
				return 0;
			}
			if (!slang_struct_construct_a (spec->_struct))
			{
				slang_alloc_free (spec->_struct);
				spec->_struct = NULL;
				slang_alloc_free (name);
				slang_info_log_memory (C->L);
				return 0;
			}
			spec->_struct->name = name;
			spec->_struct->structs->outer_scope = structs;
		}
		do
		{
			slang_type_specifier sp;
			slang_type_specifier_construct (&sp);
			if (!parse_type_specifier (C, &sp, spec->_struct->structs, scope, funcs))
			{
				slang_type_specifier_destruct (&sp);
				return 0;
			}
			do
			{
				slang_variable *var;
				spec->_struct->fields->variables = (slang_variable *) slang_alloc_realloc (
					spec->_struct->fields->variables,
					spec->_struct->fields->num_variables * sizeof (slang_variable),
					(spec->_struct->fields->num_variables + 1) * sizeof (slang_variable));
				if (spec->_struct->fields->variables == NULL)
				{
					slang_type_specifier_destruct (&sp);
					slang_info_log_memory (C->L);
					return 0;
				}
				var = spec->_struct->fields->variables + spec->_struct->fields->num_variables;
				spec->_struct->fields->num_variables++;
				slang_variable_construct (var);
				if (!slang_type_specifier_copy (&var->type.specifier, &sp))
				{
					slang_type_specifier_destruct (&sp);
					return 0;
				}
				if (!parse_identifier (C, &var->name))
				{
					slang_type_specifier_destruct (&sp);
					return 0;
				}
				switch (*C->I++)
				{
				case FIELD_NONE:
					break;
				case FIELD_ARRAY:
					var->array_size = (slang_operation *) slang_alloc_malloc (sizeof (
						slang_operation));
					if (var->array_size == NULL)
					{
						slang_type_specifier_destruct (&sp);
						slang_info_log_memory (C->L);
						return 0;
					}
					if (!slang_operation_construct_a (var->array_size))
					{
						slang_alloc_free (var->array_size);
						var->array_size = NULL;
						slang_type_specifier_destruct (&sp);
						slang_info_log_memory (C->L);
						return 0;
					}
					if (!parse_expression (C, var->array_size, scope, structs, funcs))
					{
						slang_type_specifier_destruct (&sp);
						return 0;
					}
					break;
				default:
					return 0;
				}
			}
			while (*C->I++ != FIELD_NONE);
		}
		while (*C->I++ != FIELD_NONE);
		if (*spec->_struct->name != '\0')
		{
			slang_struct *s;
			structs->structs = (slang_struct *) slang_alloc_realloc (structs->structs,
				structs->num_structs * sizeof (slang_struct),
				(structs->num_structs + 1) * sizeof (slang_struct));
			if (structs->structs == NULL)
			{
				slang_info_log_memory (C->L);
				return 0;
			}
			s = structs->structs + structs->num_structs;
			if (!slang_struct_construct_a (s))
				return 0;
			structs->num_structs++;
			if (!slang_struct_copy (s, spec->_struct))
				return 0;
		}
		break;
	case TYPE_SPECIFIER_TYPENAME:
		spec->type = slang_spec_struct;
		{
			char *name;
			slang_struct *stru;
			if (!parse_identifier (C, &name))
				return 0;
			stru = slang_struct_scope_find (structs, name, 1);
			if (stru == NULL)
			{
				slang_info_log_error (C->L, "%s: undeclared type name", name);
				slang_alloc_free (name);
				return 0;
			}
			slang_alloc_free (name);
			spec->_struct = (slang_struct *) slang_alloc_malloc (sizeof (slang_struct));
			if (spec->_struct == NULL)
			{
				slang_info_log_memory (C->L);
				return 0;
			}
			if (!slang_struct_construct_a (spec->_struct))
			{
				slang_alloc_free (spec->_struct);
				spec->_struct = NULL;
				return 0;
			}
			if (!slang_struct_copy (spec->_struct, stru))
				return 0;
		}
		break;
	default:
		return 0;
	}
	return 1;
}

static int parse_fully_specified_type (slang_parse_ctx *C, slang_fully_specified_type *type,
	slang_struct_scope *structs, slang_variable_scope *scope, slang_function_scope *funcs)
{
	if (!parse_type_qualifier (C, &type->qualifier))
		return 0;
	return parse_type_specifier (C, &type->specifier, structs, scope, funcs);
}

/* operation */
#define OP_END 0
#define OP_BLOCK_BEGIN_NO_NEW_SCOPE 1
#define OP_BLOCK_BEGIN_NEW_SCOPE 2
#define OP_DECLARE 3
#define OP_ASM 4
#define OP_BREAK 5
#define OP_CONTINUE 6
#define OP_DISCARD 7
#define OP_RETURN 8
#define OP_EXPRESSION 9
#define OP_IF 10
#define OP_WHILE 11
#define OP_DO 12
#define OP_FOR 13
#define OP_PUSH_VOID 14
#define OP_PUSH_BOOL 15
#define OP_PUSH_INT 16
#define OP_PUSH_FLOAT 17
#define OP_PUSH_IDENTIFIER 18
#define OP_SEQUENCE 19
#define OP_ASSIGN 20
#define OP_ADDASSIGN 21
#define OP_SUBASSIGN 22
#define OP_MULASSIGN 23
#define OP_DIVASSIGN 24
/*#define OP_MODASSIGN 25*/
/*#define OP_LSHASSIGN 26*/
/*#define OP_RSHASSIGN 27*/
/*#define OP_ORASSIGN 28*/
/*#define OP_XORASSIGN 29*/
/*#define OP_ANDASSIGN 30*/
#define OP_SELECT 31
#define OP_LOGICALOR 32
#define OP_LOGICALXOR 33
#define OP_LOGICALAND 34
/*#define OP_BITOR 35*/
/*#define OP_BITXOR 36*/
/*#define OP_BITAND 37*/
#define OP_EQUAL 38
#define OP_NOTEQUAL 39
#define OP_LESS 40
#define OP_GREATER 41
#define OP_LESSEQUAL 42
#define OP_GREATEREQUAL 43
/*#define OP_LSHIFT 44*/
/*#define OP_RSHIFT 45*/
#define OP_ADD 46
#define OP_SUBTRACT 47
#define OP_MULTIPLY 48
#define OP_DIVIDE 49
/*#define OP_MODULUS 50*/
#define OP_PREINCREMENT 51
#define OP_PREDECREMENT 52
#define OP_PLUS 53
#define OP_MINUS 54
/*#define OP_COMPLEMENT 55*/
#define OP_NOT 56
#define OP_SUBSCRIPT 57
#define OP_CALL 58
#define OP_FIELD 59
#define OP_POSTINCREMENT 60
#define OP_POSTDECREMENT 61

static int parse_child_operation (slang_parse_ctx *C, slang_operation *oper, int statement,
	slang_variable_scope *scope, slang_struct_scope *structs, slang_function_scope *funcs)
{
	oper->children = (slang_operation *) slang_alloc_realloc (oper->children,
		oper->num_children * sizeof (slang_operation),
		(oper->num_children + 1) * sizeof (slang_operation));
	if (oper->children == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	if (!slang_operation_construct_a (oper->children + oper->num_children))
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	oper->num_children++;
	if (statement)
		return parse_statement (C, oper->children + oper->num_children - 1, scope, structs, funcs);
	return parse_expression (C, oper->children + oper->num_children - 1, scope, structs, funcs);
}

static int parse_declaration (slang_parse_ctx *C, slang_variable_scope *, slang_struct_scope *,
	slang_function_scope *);

static int parse_statement (slang_parse_ctx *C, slang_operation *oper, slang_variable_scope *scope,
	slang_struct_scope *structs, slang_function_scope *funcs)
{
	oper->locals->outer_scope = scope;
	switch (*C->I++)
	{
	case OP_BLOCK_BEGIN_NO_NEW_SCOPE:
		oper->type = slang_oper_block_no_new_scope;
		while (*C->I != OP_END)
			if (!parse_child_operation (C, oper, 1, scope, structs, funcs))
				return 0;
		C->I++;
		break;
	case OP_BLOCK_BEGIN_NEW_SCOPE:
		oper->type = slang_oper_block_new_scope;
		while (*C->I != OP_END)
			if (!parse_child_operation (C, oper, 1, oper->locals, structs, funcs))
				return 0;
		C->I++;
		break;
	case OP_DECLARE:
		oper->type = slang_oper_variable_decl;
		{
			const unsigned int first_var = scope->num_variables;
			if (!parse_declaration (C, scope, structs, funcs))
				return 0;
			if (first_var < scope->num_variables)
			{
				const unsigned int num_vars = scope->num_variables - first_var;
				unsigned int i;
				oper->children = (slang_operation *) slang_alloc_malloc (num_vars * sizeof (
					slang_operation));
				if (oper->children == NULL)
				{
					slang_info_log_memory (C->L);
					return 0;
				}
				for (i = 0; i < num_vars; i++)
					if (!slang_operation_construct_a (oper->children + i))
					{
						unsigned int j;
						for (j = 0; j < i; j++)
							slang_operation_destruct (oper->children + j);
						slang_alloc_free (oper->children);
						oper->children = NULL;
						slang_info_log_memory (C->L);
						return 0;
					}
				oper->num_children = num_vars;
				for (i = first_var; i < scope->num_variables; i++)
				{
					slang_operation *o = oper->children + i - first_var;
					o->type = slang_oper_identifier;
					o->locals->outer_scope = scope;
					o->identifier = slang_string_duplicate (scope->variables[i].name);
					if (o->identifier == NULL)
					{
						slang_info_log_memory (C->L);
						return 0;
					}
				}
			}
		}
		break;
	case OP_ASM:
		oper->type = slang_oper_asm;
		if (!parse_identifier (C, &oper->identifier))
			return 0;
		while (*C->I != OP_END)
			if (!parse_child_operation (C, oper, 0, scope, structs, funcs))
				return 0;
		C->I++;
		break;
	case OP_BREAK:
		oper->type = slang_oper_break;
		break;
	case OP_CONTINUE:
		oper->type = slang_oper_continue;
		break;
	case OP_DISCARD:
		oper->type = slang_oper_discard;
		break;
	case OP_RETURN:
		oper->type = slang_oper_return;
		if (!parse_child_operation (C, oper, 0, scope, structs, funcs))
			return 0;
		break;
	case OP_EXPRESSION:
		oper->type = slang_oper_expression;
		if (!parse_child_operation (C, oper, 0, scope, structs, funcs))
			return 0;
		break;
	case OP_IF:
		oper->type = slang_oper_if;
		if (!parse_child_operation (C, oper, 0, scope, structs, funcs))
			return 0;
		if (!parse_child_operation (C, oper, 1, scope, structs, funcs))
			return 0;
		if (!parse_child_operation (C, oper, 1, scope, structs, funcs))
			return 0;
		break;
	case OP_WHILE:
		oper->type = slang_oper_while;
		if (!parse_child_operation (C, oper, 1, oper->locals, structs, funcs))
			return 0;
		if (!parse_child_operation (C, oper, 1, oper->locals, structs, funcs))
			return 0;
		break;
	case OP_DO:
		oper->type = slang_oper_do;
		if (!parse_child_operation (C, oper, 1, scope, structs, funcs))
			return 0;
		if (!parse_child_operation (C, oper, 0, scope, structs, funcs))
			return 0;
		break;
	case OP_FOR:
		oper->type = slang_oper_for;
		if (!parse_child_operation (C, oper, 1, oper->locals, structs, funcs))
			return 0;
		if (!parse_child_operation (C, oper, 1, oper->locals, structs, funcs))
			return 0;
		if (!parse_child_operation (C, oper, 0, oper->locals, structs, funcs))
			return 0;
		if (!parse_child_operation (C, oper, 1, oper->locals, structs, funcs))
			return 0;
		break;
	default:
		return 0;
	}
	return 1;
}

static int handle_trinary_expression (slang_parse_ctx *C, slang_operation *op,
	slang_operation **ops, unsigned int *num_ops)
{
	op->num_children = 3;
	op->children = (slang_operation *) slang_alloc_malloc (3 * sizeof (slang_operation));
	if (op->children == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	op->children[0] = (*ops)[*num_ops - 4];
	op->children[1] = (*ops)[*num_ops - 3];
	op->children[2] = (*ops)[*num_ops - 2];
	(*ops)[*num_ops - 4] = (*ops)[*num_ops - 1];
	*num_ops -= 3;
	*ops = (slang_operation *) slang_alloc_realloc (*ops, (*num_ops + 3) * sizeof (slang_operation),
		*num_ops * sizeof (slang_operation));
	if (*ops == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	return 1;
}

static int handle_binary_expression (slang_parse_ctx *C, slang_operation *op,
	slang_operation **ops, unsigned int *num_ops)
{
	op->num_children = 2;
	op->children = (slang_operation *) slang_alloc_malloc (2 * sizeof (slang_operation));
	if (op->children == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	op->children[0] = (*ops)[*num_ops - 3];
	op->children[1] = (*ops)[*num_ops - 2];
	(*ops)[*num_ops - 3] = (*ops)[*num_ops - 1];
	*num_ops -= 2;
	*ops = (slang_operation *) slang_alloc_realloc (*ops, (*num_ops + 2) * sizeof (slang_operation),
		*num_ops * sizeof (slang_operation));
	if (*ops == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	return 1;
}

static int handle_unary_expression (slang_parse_ctx *C, slang_operation *op,
	slang_operation **ops, unsigned int *num_ops)
{
	op->num_children = 1;
	op->children = (slang_operation *) slang_alloc_malloc (sizeof (slang_operation));
	if (op->children == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	op->children[0] = (*ops)[*num_ops - 2];
	(*ops)[*num_ops - 2] = (*ops)[*num_ops - 1];
	(*num_ops)--;
	*ops = (slang_operation *) slang_alloc_realloc (*ops, (*num_ops + 1) * sizeof (slang_operation),
		*num_ops * sizeof (slang_operation));
	if (*ops == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	return 1;
}

static int is_constructor_name (const char *name, slang_struct_scope *structs)
{
	if (slang_type_specifier_type_from_string (name) != slang_spec_void)
		return 1;
	return slang_struct_scope_find (structs, name, 1) != NULL;
}

static int parse_expression (slang_parse_ctx *C, slang_operation *oper, slang_variable_scope *scope,
	slang_struct_scope *structs, slang_function_scope *funcs)
{
	slang_operation *ops = NULL;
	unsigned int num_ops = 0;
	int number;

	while (*C->I != OP_END)
	{
		slang_operation *op;
		const unsigned int op_code = *C->I++;
		ops = (slang_operation *) slang_alloc_realloc (ops,
			num_ops * sizeof (slang_operation), (num_ops + 1) * sizeof (slang_operation));
		if (ops == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		op = ops + num_ops;
		if (!slang_operation_construct_a (op))
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		num_ops++;
		op->locals->outer_scope = scope;
		switch (op_code)
		{
		case OP_PUSH_VOID:
			op->type = slang_oper_void;
			break;
		case OP_PUSH_BOOL:
			op->type = slang_oper_literal_bool;
			if (!parse_number (C, &number))
				return 0;
			op->literal = (float) number;
			break;
		case OP_PUSH_INT:
			op->type = slang_oper_literal_int;
			if (!parse_number (C, &number))
				return 0;
			op->literal = (float) number;
			break;
		case OP_PUSH_FLOAT:
			op->type = slang_oper_literal_float;
			if (!parse_float (C, &op->literal))
				return 0;
			break;
		case OP_PUSH_IDENTIFIER:
			op->type = slang_oper_identifier;
			if (!parse_identifier (C, &op->identifier))
				return 0;
			break;
		case OP_SEQUENCE:
			op->type = slang_oper_sequence;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_ASSIGN:
			op->type = slang_oper_assign;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_ADDASSIGN:
			op->type = slang_oper_addassign;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_SUBASSIGN:
			op->type = slang_oper_subassign;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_MULASSIGN:
			op->type = slang_oper_mulassign;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_DIVASSIGN:
			op->type = slang_oper_divassign;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		/*case OP_MODASSIGN:*/
		/*case OP_LSHASSIGN:*/
		/*case OP_RSHASSIGN:*/
		/*case OP_ORASSIGN:*/
		/*case OP_XORASSIGN:*/
		/*case OP_ANDASSIGN:*/
		case OP_SELECT:
			op->type = slang_oper_select;
			if (!handle_trinary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_LOGICALOR:
			op->type = slang_oper_logicalor;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_LOGICALXOR:
			op->type = slang_oper_logicalxor;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_LOGICALAND:
			op->type = slang_oper_logicaland;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		/*case OP_BITOR:*/
		/*case OP_BITXOR:*/
		/*case OP_BITAND:*/
		case OP_EQUAL:
			op->type = slang_oper_equal;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_NOTEQUAL:
			op->type = slang_oper_notequal;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_LESS:
			op->type = slang_oper_less;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_GREATER:
			op->type = slang_oper_greater;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_LESSEQUAL:
			op->type = slang_oper_lessequal;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_GREATEREQUAL:
			op->type = slang_oper_greaterequal;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		/*case OP_LSHIFT:*/
		/*case OP_RSHIFT:*/
		case OP_ADD:
			op->type = slang_oper_add;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_SUBTRACT:
			op->type = slang_oper_subtract;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_MULTIPLY:
			op->type = slang_oper_multiply;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_DIVIDE:
			op->type = slang_oper_divide;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		/*case OP_MODULUS:*/
		case OP_PREINCREMENT:
			op->type = slang_oper_preincrement;
			if (!handle_unary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_PREDECREMENT:
			op->type = slang_oper_predecrement;
			if (!handle_unary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_PLUS:
			op->type = slang_oper_plus;
			if (!handle_unary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_MINUS:
			op->type = slang_oper_minus;
			if (!handle_unary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_NOT:
			op->type = slang_oper_not;
			if (!handle_unary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		/*case OP_COMPLEMENT:*/
		case OP_SUBSCRIPT:
			op->type = slang_oper_subscript;
			if (!handle_binary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_CALL:
			op->type = slang_oper_call;
			if (!parse_identifier (C, &op->identifier))
				return 0;
			while (*C->I != OP_END)
				if (!parse_child_operation (C, op, 0, scope, structs, funcs))
					return 0;
			C->I++;
			if (!C->parsing_builtin &&
				!slang_function_scope_find_by_name (funcs, op->identifier, 1) &&
				!is_constructor_name (op->identifier, structs))
			{
				slang_info_log_error (C->L, "%s: undeclared function name", op->identifier);
				return 0;
			}
			break;
		case OP_FIELD:
			op->type = slang_oper_field;
			if (!parse_identifier (C, &op->identifier))
				return 0;
			if (!handle_unary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_POSTINCREMENT:
			op->type = slang_oper_postincrement;
			if (!handle_unary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		case OP_POSTDECREMENT:
			op->type = slang_oper_postdecrement;
			if (!handle_unary_expression (C, op, &ops, &num_ops))
				return 0;
			break;
		default:
			return 0;
		}
	}
	C->I++;
	*oper = *ops;
	slang_alloc_free (ops);
	return 1;
}

/* parameter qualifier */
#define PARAM_QUALIFIER_IN 0
#define PARAM_QUALIFIER_OUT 1
#define PARAM_QUALIFIER_INOUT 2

/* function parameter array presence */
#define PARAMETER_ARRAY_NOT_PRESENT 0
#define PARAMETER_ARRAY_PRESENT 1

static int parse_parameter_declaration (slang_parse_ctx *C, slang_variable *param,
	slang_struct_scope *structs, slang_variable_scope *scope, slang_function_scope *funcs)
{
	slang_storage_aggregate agg;
	if (!parse_type_qualifier (C, &param->type.qualifier))
		return 0;
	switch (*C->I++)
	{
	case PARAM_QUALIFIER_IN:
		if (param->type.qualifier != slang_qual_const && param->type.qualifier != slang_qual_none)
		{
			slang_info_log_error (C->L, "invalid type qualifier");
			return 0;
		}
		break;
	case PARAM_QUALIFIER_OUT:
		if (param->type.qualifier == slang_qual_none)
			param->type.qualifier = slang_qual_out;
		else
		{
			slang_info_log_error (C->L, "invalid type qualifier");
			return 0;
		}
		break;
	case PARAM_QUALIFIER_INOUT:
		if (param->type.qualifier == slang_qual_none)
			param->type.qualifier = slang_qual_inout;
		else
		{
			slang_info_log_error (C->L, "invalid type qualifier");
			return 0;
		}
		break;
	default:
		return 0;
	}
	if (!parse_type_specifier (C, &param->type.specifier, structs, scope, funcs))
		return 0;
	if (!parse_identifier (C, &param->name))
		return 0;
	if (*C->I++ == PARAMETER_ARRAY_PRESENT)
	{
		param->array_size = (slang_operation *) slang_alloc_malloc (sizeof (slang_operation));
		if (param->array_size == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		if (!slang_operation_construct_a (param->array_size))
		{
			slang_alloc_free (param->array_size);
			param->array_size = NULL;
			slang_info_log_memory (C->L);
			return 0;
		}
		if (!parse_expression (C, param->array_size, scope, structs, funcs))
			return 0;
	}
	slang_storage_aggregate_construct (&agg);
	if (!_slang_aggregate_variable (&agg, &param->type.specifier, param->array_size, funcs,
		structs))
	{
		slang_storage_aggregate_destruct (&agg);
		return 0;
	}
	slang_storage_aggregate_destruct (&agg);
	return 1;
}

/* function type */
#define FUNCTION_ORDINARY 0
#define FUNCTION_CONSTRUCTOR 1
#define FUNCTION_OPERATOR 2

/* function parameter */
#define PARAMETER_NONE 0
#define PARAMETER_NEXT 1

/* operator type */
#define OPERATOR_ASSIGN 1
#define OPERATOR_ADDASSIGN 2
#define OPERATOR_SUBASSIGN 3
#define OPERATOR_MULASSIGN 4
#define OPERATOR_DIVASSIGN 5
/*#define OPERATOR_MODASSIGN 6*/
/*#define OPERATOR_LSHASSIGN 7*/
/*#define OPERATOR_RSHASSIGN 8*/
/*#define OPERATOR_ANDASSIGN 9*/
/*#define OPERATOR_XORASSIGN 10*/
/*#define OPERATOR_ORASSIGN 11*/
#define OPERATOR_LOGICALXOR 12
/*#define OPERATOR_BITOR 13*/
/*#define OPERATOR_BITXOR 14*/
/*#define OPERATOR_BITAND 15*/
#define OPERATOR_EQUAL 16
#define OPERATOR_NOTEQUAL 17
#define OPERATOR_LESS 18
#define OPERATOR_GREATER 19
#define OPERATOR_LESSEQUAL 20
#define OPERATOR_GREATEREQUAL 21
/*#define OPERATOR_LSHIFT 22*/
/*#define OPERATOR_RSHIFT 23*/
#define OPERATOR_MULTIPLY 24
#define OPERATOR_DIVIDE 25
/*#define OPERATOR_MODULUS 26*/
#define OPERATOR_INCREMENT 27
#define OPERATOR_DECREMENT 28
#define OPERATOR_PLUS 29
#define OPERATOR_MINUS 30
/*#define OPERATOR_COMPLEMENT 31*/
#define OPERATOR_NOT 32

static const struct {
	unsigned int o_code;
	const char *o_name;
} operator_names[] = {
	{ OPERATOR_INCREMENT, "++" },
	{ OPERATOR_ADDASSIGN, "+=" },
	{ OPERATOR_PLUS, "+" },
	{ OPERATOR_DECREMENT, "--" },
	{ OPERATOR_SUBASSIGN, "-=" },
	{ OPERATOR_MINUS, "-" },
	{ OPERATOR_NOTEQUAL, "!=" },
	{ OPERATOR_NOT, "!" },
	{ OPERATOR_MULASSIGN, "*=" },
	{ OPERATOR_MULTIPLY, "*" },
	{ OPERATOR_DIVASSIGN, "/=" },
	{ OPERATOR_DIVIDE, "/" },
	{ OPERATOR_LESSEQUAL, "<=" },
	/*{ OPERATOR_LSHASSIGN, "<<=" },*/
	/*{ OPERATOR_LSHIFT, "<<" },*/
	{ OPERATOR_LESS, "<" },
	{ OPERATOR_GREATEREQUAL, ">=" },
	/*{ OPERATOR_RSHASSIGN, ">>=" },*/
	/*{ OPERATOR_RSHIFT, ">>" },*/
	{ OPERATOR_GREATER, ">" },
	{ OPERATOR_EQUAL, "==" },
	{ OPERATOR_ASSIGN, "=" },
	/*{ OPERATOR_MODASSIGN, "%=" },*/
	/*{ OPERATOR_MODULUS, "%" },*/
	/*{ OPERATOR_ANDASSIGN, "&=" },*/
	/*{ OPERATOR_BITAND, "&" },*/
	/*{ OPERATOR_ORASSIGN, "|=" },*/
	/*{ OPERATOR_BITOR, "|" },*/
	/*{ OPERATOR_COMPLEMENT, "~" },*/
	/*{ OPERATOR_XORASSIGN, "^=" },*/
	{ OPERATOR_LOGICALXOR, "^^" }/*,*/
	/*{ OPERATOR_BITXOR, "^" }*/
};

static int parse_operator_name (slang_parse_ctx *C, char **pname)
{
	unsigned int i;
	for (i = 0; i < sizeof (operator_names) / sizeof (*operator_names); i++)
		if (operator_names[i].o_code == (unsigned int) (*C->I))
		{
			*pname = slang_string_duplicate (operator_names[i].o_name);
			if (*pname == NULL)
			{
				slang_info_log_memory (C->L);
				return 0;
			}
			C->I++;
			return 1;
		}
	return 0;
}

static int parse_function_prototype (slang_parse_ctx *C, slang_function *func,
	slang_struct_scope *structs, slang_variable_scope *scope, slang_function_scope *funcs)
{
	if (!parse_fully_specified_type (C, &func->header.type, structs, scope, funcs))
		return 0;
	switch (*C->I++)
	{
	case FUNCTION_ORDINARY:
		func->kind = slang_func_ordinary;
		if (!parse_identifier (C, &func->header.name))
			return 0;
		break;
	case FUNCTION_CONSTRUCTOR:
		func->kind = slang_func_constructor;
		if (func->header.type.specifier.type == slang_spec_struct)
			return 0;
		func->header.name = slang_string_duplicate (
			type_specifier_type_names[func->header.type.specifier.type]);
		if (func->header.name == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		break;
	case FUNCTION_OPERATOR:
		func->kind = slang_func_operator;
		if (!parse_operator_name (C, &func->header.name))
			return 0;
		break;
	default:
		return 0;
	}
	func->parameters->outer_scope = scope;
	while (*C->I++ == PARAMETER_NEXT)
	{
		func->parameters->variables = (slang_variable *) slang_alloc_realloc (
			func->parameters->variables,
			func->parameters->num_variables * sizeof (slang_variable),
			(func->parameters->num_variables + 1) * sizeof (slang_variable));
		if (func->parameters->variables == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		slang_variable_construct (func->parameters->variables + func->parameters->num_variables);
		func->parameters->num_variables++;
		if (!parse_parameter_declaration (C, func->parameters->variables +
			func->parameters->num_variables - 1, structs, scope, funcs))
			return 0;
	}
	func->param_count = func->parameters->num_variables;
	return 1;
}

static int parse_function_definition (slang_parse_ctx *C, slang_function *func,
	slang_struct_scope *structs, slang_variable_scope *scope, slang_function_scope *funcs)
{
	if (!parse_function_prototype (C, func, structs, scope, funcs))
		return 0;
	func->body = (slang_operation *) slang_alloc_malloc (sizeof (slang_operation));
	if (func->body == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	if (!slang_operation_construct_a (func->body))
	{
		slang_alloc_free (func->body);
		func->body = NULL;
		slang_info_log_memory (C->L);
		return 0;
	}
	if (!parse_statement (C, func->body, func->parameters, structs, funcs))
		return 0;
	return 1;
}

/* init declarator list */
#define DECLARATOR_NONE 0
#define DECLARATOR_NEXT 1

/* variable declaration */
#define VARIABLE_NONE 0
#define VARIABLE_IDENTIFIER 1
#define VARIABLE_INITIALIZER 2
#define VARIABLE_ARRAY_EXPLICIT 3
#define VARIABLE_ARRAY_UNKNOWN 4

static int parse_init_declarator (slang_parse_ctx *C, const slang_fully_specified_type *type,
	slang_variable_scope *vars, slang_struct_scope *structs, slang_function_scope *funcs)
{
	slang_variable *var;

	if (*C->I++ == VARIABLE_NONE)
		return 1;
	vars->variables = (slang_variable *) slang_alloc_realloc (vars->variables,
		vars->num_variables * sizeof (slang_variable),
		(vars->num_variables + 1) * sizeof (slang_variable));
	if (vars->variables == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	var = vars->variables + vars->num_variables;
	vars->num_variables++;
	slang_variable_construct (var);
	var->type.qualifier = type->qualifier;
	if (!parse_identifier (C, &var->name))
		return 0;
	switch (*C->I++)
	{
	case VARIABLE_NONE:
		if (!slang_type_specifier_copy (&var->type.specifier, &type->specifier))
			return 0;
		break;
	case VARIABLE_INITIALIZER:
		if (!slang_type_specifier_copy (&var->type.specifier, &type->specifier))
			return 0;
		var->initializer = (slang_operation *) slang_alloc_malloc (sizeof (slang_operation));
		if (var->initializer == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		if (!slang_operation_construct_a (var->initializer))
		{
			slang_alloc_free (var->initializer);
			var->initializer = NULL;
			slang_info_log_memory (C->L);
			return 0;
		}
		if (!parse_expression (C, var->initializer, vars, structs, funcs))
			return 0;
		break;
	case VARIABLE_ARRAY_UNKNOWN:
		var->type.specifier.type = slang_spec_array;
		var->type.specifier._array = (slang_type_specifier *) slang_alloc_malloc (sizeof (
			slang_type_specifier));
		if (var->type.specifier._array == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		slang_type_specifier_construct (var->type.specifier._array);
		if (!slang_type_specifier_copy (var->type.specifier._array, &type->specifier))
			return 0;
		break;
	case VARIABLE_ARRAY_EXPLICIT:
		var->type.specifier.type = slang_spec_array;
		var->type.specifier._array = (slang_type_specifier *) slang_alloc_malloc (sizeof (
			slang_type_specifier));
		if (var->type.specifier._array == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		slang_type_specifier_construct (var->type.specifier._array);
		if (!slang_type_specifier_copy (var->type.specifier._array, &type->specifier))
			return 0;
		var->array_size = (slang_operation *) slang_alloc_malloc (sizeof (slang_operation));
		if (var->array_size == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		if (!slang_operation_construct_a (var->array_size))
		{
			slang_alloc_free (var->array_size);
			var->array_size = NULL;
			slang_info_log_memory (C->L);
			return 0;
		}
		if (!parse_expression (C, var->array_size, vars, structs, funcs))
			return 0;
		break;
	default:
		return 0;
	}
	if (!(var->type.specifier.type == slang_spec_array && var->array_size == NULL))
	{
		slang_storage_aggregate agg;

		slang_storage_aggregate_construct (&agg);
		if (!_slang_aggregate_variable (&agg, &var->type.specifier, var->array_size, funcs,
			structs))
		{
			slang_storage_aggregate_destruct (&agg);
			return 0;
		}
		slang_storage_aggregate_destruct (&agg);
	}
	return 1;
}

static int parse_init_declarator_list (slang_parse_ctx *C, slang_variable_scope *vars,
	slang_struct_scope *structs, slang_function_scope *funcs)
{
	slang_fully_specified_type type;

	slang_fully_specified_type_construct (&type);
	if (!parse_fully_specified_type (C, &type, structs, vars, funcs))
	{
		slang_fully_specified_type_destruct (&type);
		return 0;
	}
	do
	{
		if (!parse_init_declarator (C, &type, vars, structs, funcs))
		{
			slang_fully_specified_type_destruct (&type);
			return 0;
		}
	}
	while (*C->I++ == DECLARATOR_NEXT);
	slang_fully_specified_type_destruct (&type);
	return 1;
}

static int parse_function (slang_parse_ctx *C, int definition, slang_struct_scope *structs,
	slang_function_scope *funcs, slang_variable_scope *scope, slang_function **parsed_func_ret)
{
	slang_function parsed_func, *found_func;

	/* parse function definition/declaration */
	slang_function_construct (&parsed_func);
	if (definition)
	{
		if (!parse_function_definition (C, &parsed_func, structs, scope, funcs))
		{
			slang_function_destruct (&parsed_func);
			return 0;
		}
	}
	else
	{
		if (!parse_function_prototype (C, &parsed_func, structs, scope, funcs))
		{
			slang_function_destruct (&parsed_func);
			return 0;
		}
	}

	/* find a function with a prototype matching the parsed one - only the current scope
	is being searched to allow built-in function overriding */
	found_func = slang_function_scope_find (funcs, &parsed_func, 0);
	if (found_func == NULL)
	{
		/* add the parsed function to the function list */
		funcs->functions = (slang_function *) slang_alloc_realloc (funcs->functions,
			funcs->num_functions * sizeof (slang_function), (funcs->num_functions + 1) * sizeof (
			slang_function));
		if (funcs->functions == NULL)
		{
			slang_info_log_memory (C->L);
			slang_function_destruct (&parsed_func);
			return 0;
		}
		funcs->functions[funcs->num_functions] = parsed_func;
		funcs->num_functions++;

		/* return the newly parsed function */
		*parsed_func_ret = funcs->functions + funcs->num_functions - 1;
	}
	else
	{
		/* TODO: check function return type qualifiers and specifiers */
		if (definition)
		{
			/* destroy the existing function declaration and replace it with the new one */
			if (found_func->body != NULL)
			{
				slang_info_log_error (C->L, "%s: function already has a body",
					parsed_func.header.name);
				slang_function_destruct (&parsed_func);
				return 0;
			}
			slang_function_destruct (found_func);
			*found_func = parsed_func;
		}
		else
		{
			/* another declaration of the same function prototype - ignore it */
			slang_function_destruct (&parsed_func);
		}

		/* return the found function */
		*parsed_func_ret = found_func;
	}

	/* assemble the parsed function */
	if (definition)
	{
static int x = 0;
static
		slang_assembly_file file;
		slang_assembly_name_space space;
x++;
if (x == 1)
		slang_assembly_file_construct (&file);
		space.funcs = funcs;
		space.structs = structs;
		space.vars = scope;
if (x == 1)
xxx_first (&file);
		(**parsed_func_ret).address = file.count;
		if (!_slang_assemble_function (&file, *parsed_func_ret, &space))
		{
			slang_assembly_file_destruct (&file);
			return 0;
		}
if (slang_string_compare ("main", (**parsed_func_ret).header.name) == 0)
{
xxx_prolog (&file, (**parsed_func_ret).address);
_slang_execute (&file);
slang_assembly_file_destruct (&file);
exit (0);
}
	}
	return 1;
}

/* declaration */
#define DECLARATION_FUNCTION_PROTOTYPE 1
#define DECLARATION_INIT_DECLARATOR_LIST 2

static int parse_declaration (slang_parse_ctx *C, slang_variable_scope *scope,
	slang_struct_scope *structs, slang_function_scope *funcs)
{
	slang_function *dummy_func;

	switch (*C->I++)
	{
	case DECLARATION_INIT_DECLARATOR_LIST:
		if (!parse_init_declarator_list (C, scope, structs, funcs))
			return 0;
		break;
	case DECLARATION_FUNCTION_PROTOTYPE:
		if (!parse_function (C, 0, structs, funcs, scope, &dummy_func))
			return 0;
		break;
	default:
		return 0;
	}
	return 1;
}

/* external declaration */
#define EXTERNAL_NULL 0
#define EXTERNAL_FUNCTION_DEFINITION 1
#define EXTERNAL_DECLARATION 2

static int parse_translation_unit (slang_parse_ctx *C, slang_translation_unit *unit)
{
	while (*C->I != EXTERNAL_NULL)
	{
		slang_function *func;

		switch (*C->I++)
		{
		case EXTERNAL_FUNCTION_DEFINITION:
			if (!parse_function (C, 1, &unit->structs, &unit->functions, &unit->globals, &func))
				return 0;
			break;
		case EXTERNAL_DECLARATION:
			if (!parse_declaration (C, &unit->globals, &unit->structs, &unit->functions))
				return 0;
			break;
		default:
			return 0;
		}
	}
	C->I++;
	return 1;
}

static int compile_binary (const byte *prod, slang_translation_unit *unit, slang_unit_type type,
	slang_info_log *log, slang_translation_unit *builtins)
{
	slang_parse_ctx C;

	/* set-up parse context */
	C.I = prod;
	C.L = log;
	C.parsing_builtin = builtins == NULL;

	if (!check_revision (&C))
		return 0;

	/* create translation unit object */
	slang_translation_unit_construct (unit);
	unit->type = type;

	if (builtins != NULL)
	{
		/* link to built-in functions */
		builtins[1].functions.outer_scope = &builtins[0].functions;
		builtins[2].functions.outer_scope = &builtins[1].functions;
		unit->functions.outer_scope = &builtins[2].functions;

		/* link to built-in variables - core unit does not define any */
		builtins[2].globals.outer_scope = &builtins[1].globals;
		unit->globals.outer_scope = &builtins[2].globals;

		/* link to built-in structure typedefs - only in common unit */
		unit->structs.outer_scope = &builtins[1].structs;
	}

	/* parse translation unit */
	if (!parse_translation_unit (&C, unit))
	{
		slang_translation_unit_destruct (unit);
		return 0;
	}

	return 1;
}

static int compile_with_grammar (grammar id, const char *source, slang_translation_unit *unit,
	slang_unit_type type, slang_info_log *log, slang_translation_unit *builtins)
{
	byte *prod;
	unsigned int size, start, version;

	/* retrieve version */
	if (!_slang_preprocess_version (source, &version, &start, log))
		return 0;

	/* check the syntax */
	if (!grammar_fast_check (id, (const byte *) source + start, &prod, &size, 65536))
	{
		char buf[1024];
		unsigned int pos;
		grammar_get_last_error ( (unsigned char*) buf, 1024, (int*) &pos);
		slang_info_log_error (log, buf);
		return 0;
	}

	if (!compile_binary (prod, unit, type, log, builtins))
	{
		grammar_alloc_free (prod);
		return 0;
	}

	grammar_alloc_free (prod);
	return 1;
}

static const char *slang_shader_syn =
#include "library/slang_shader_syn.h"
;
/*
static const byte slang_core_gc_bin[] = {
#include "library/slang_core_gc_bin.h"
};*/
static const byte slang_core_gc[] = {
#include "library/slang_core_gc.h"
};

static const byte slang_common_builtin_gc_bin[] = {
#include "library/slang_common_builtin_gc_bin.h"
};

static const byte slang_fragment_builtin_gc_bin[] = {
#include "library/slang_fragment_builtin_gc_bin.h"
};

static const byte slang_vertex_builtin_gc_bin[] = {
#include "library/slang_vertex_builtin_gc_bin.h"
};

int _slang_compile (const char *source, slang_translation_unit *unit, slang_unit_type type,
	slang_info_log *log)
{
	grammar id;
	slang_translation_unit builtin_units[3];
	slang_translation_unit *builtins = NULL;

	/* load slang grammar */
	id = grammar_load_from_text ((const byte *) slang_shader_syn);
	if (id == 0)
	{
		char buf[1024];
		unsigned int pos;
		grammar_get_last_error ( (unsigned char*) buf, 1024, (int*) &pos);
		slang_info_log_error (log, buf);
		return 0;
	}

	/* set shader type - the syntax is slightly different for different shaders */
	if (type == slang_unit_fragment_shader || type == slang_unit_fragment_builtin)
		grammar_set_reg8 (id, (const byte *) "shader_type", 1);
	else
		grammar_set_reg8 (id, (const byte *) "shader_type", 2);

	/* enable language extensions */
	grammar_set_reg8 (id, (const byte *) "parsing_builtin", 1);

	/* if parsing user-specified shader, load built-in library */
	if (type == slang_unit_fragment_shader || type == slang_unit_vertex_shader)
	{
		/*if (!compile_binary (slang_core_gc_bin, builtin_units,
			slang_unit_fragment_builtin, log, NULL))*/
		if (!compile_with_grammar (id, (const char*) slang_core_gc, builtin_units, slang_unit_fragment_builtin,
			log, NULL))
		{
			grammar_destroy (id);
			return 0;
		}
		if (!compile_binary (slang_common_builtin_gc_bin, builtin_units + 1,
			slang_unit_fragment_builtin, log, NULL))
		{
			slang_translation_unit_destruct (builtin_units);
			grammar_destroy (id);
			return 0;
		}
		if (type == slang_unit_fragment_shader)
		{
			if (!compile_binary (slang_fragment_builtin_gc_bin, builtin_units + 2,
				slang_unit_fragment_builtin, log, NULL))
			{
				slang_translation_unit_destruct (builtin_units);
				slang_translation_unit_destruct (builtin_units + 1);
				grammar_destroy (id);
				return 0;
			}
		}
		else if (type == slang_unit_vertex_shader)
		{
			if (!compile_binary (slang_vertex_builtin_gc_bin, builtin_units + 2,
				slang_unit_vertex_builtin, log, NULL))
			{
				slang_translation_unit_destruct (builtin_units);
				slang_translation_unit_destruct (builtin_units + 1);
				grammar_destroy (id);
				return 0;
			}
		}

		/* disable language extensions */
		grammar_set_reg8 (id, (const byte *) "parsing_builtin", 0);
		builtins = builtin_units;
	}

	/* compile the actual shader - pass-in built-in library for external shader */
	if (!compile_with_grammar (id, source, unit, type, log, builtins))
	{
		if (type == slang_unit_fragment_shader || type == slang_unit_vertex_shader)
		{
			slang_translation_unit_destruct (builtin_units);
			slang_translation_unit_destruct (builtin_units + 1);
			slang_translation_unit_destruct (builtin_units + 2);
		}
		grammar_destroy (id);
		return 0;
	}

	/* destroy built-in library */
	if (type == slang_unit_fragment_shader || type == slang_unit_vertex_shader)
	{
		slang_translation_unit_destruct (builtin_units);
		slang_translation_unit_destruct (builtin_units + 1);
		slang_translation_unit_destruct (builtin_units + 2);
	}
	grammar_destroy (id);
	return 1;
}

