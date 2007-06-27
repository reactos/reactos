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

#if !defined SLANG_COMPILE_H
#define SLANG_COMPILE_H

#if defined __cplusplus
extern "C" {
#endif

typedef enum slang_type_qualifier_
{
	slang_qual_none,
	slang_qual_const,
	slang_qual_attribute,
	slang_qual_varying,
	slang_qual_uniform,
	slang_qual_out,
	slang_qual_inout,
	slang_qual_fixedoutput,	/* internal */
	slang_qual_fixedinput	/* internal */
} slang_type_qualifier;

typedef enum slang_type_specifier_type_
{
	slang_spec_void,
	slang_spec_bool,
	slang_spec_bvec2,
	slang_spec_bvec3,
	slang_spec_bvec4,
	slang_spec_int,
	slang_spec_ivec2,
	slang_spec_ivec3,
	slang_spec_ivec4,
	slang_spec_float,
	slang_spec_vec2,
	slang_spec_vec3,
	slang_spec_vec4,
	slang_spec_mat2,
	slang_spec_mat3,
	slang_spec_mat4,
	slang_spec_sampler1D,
	slang_spec_sampler2D,
	slang_spec_sampler3D,
	slang_spec_samplerCube,
	slang_spec_sampler1DShadow,
	slang_spec_sampler2DShadow,
	slang_spec_struct,
	slang_spec_array
} slang_type_specifier_type;

slang_type_specifier_type slang_type_specifier_type_from_string (const char *);

typedef struct slang_type_specifier_
{
	slang_type_specifier_type type;
	struct slang_struct_ *_struct;		/* spec_struct */
	struct slang_type_specifier_ *_array;	/* spec_array */
} slang_type_specifier;

void slang_type_specifier_construct (slang_type_specifier *);
void slang_type_specifier_destruct (slang_type_specifier *);
int slang_type_specifier_copy (slang_type_specifier *, const slang_type_specifier *);
int slang_type_specifier_equal (const slang_type_specifier *, const slang_type_specifier *);

typedef struct slang_fully_specified_type_
{
	slang_type_qualifier qualifier;
	slang_type_specifier specifier;
} slang_fully_specified_type;

typedef struct slang_variable_scope_
{
	struct slang_variable_ *variables;
	unsigned int num_variables;
	struct slang_variable_scope_ *outer_scope;
} slang_variable_scope;

typedef enum slang_operation_type_
{
	slang_oper_none,
	slang_oper_block_no_new_scope,
	slang_oper_block_new_scope,
	slang_oper_variable_decl,
	slang_oper_asm,
	slang_oper_break,
	slang_oper_continue,
	slang_oper_discard,
	slang_oper_return,
	slang_oper_expression,
	slang_oper_if,
	slang_oper_while,
	slang_oper_do,
	slang_oper_for,
	slang_oper_void,
	slang_oper_literal_bool,
	slang_oper_literal_int,
	slang_oper_literal_float,
	slang_oper_identifier,
	slang_oper_sequence,
	slang_oper_assign,
	slang_oper_addassign,
	slang_oper_subassign,
	slang_oper_mulassign,
	slang_oper_divassign,
	/*slang_oper_modassign,*/
	/*slang_oper_lshassign,*/
	/*slang_oper_rshassign,*/
	/*slang_oper_orassign,*/
	/*slang_oper_xorassign,*/
	/*slang_oper_andassign,*/
	slang_oper_select,
	slang_oper_logicalor,
	slang_oper_logicalxor,
	slang_oper_logicaland,
	/*slang_oper_bitor,*/
	/*slang_oper_bitxor,*/
	/*slang_oper_bitand,*/
	slang_oper_equal,
	slang_oper_notequal,
	slang_oper_less,
	slang_oper_greater,
	slang_oper_lessequal,
	slang_oper_greaterequal,
	/*slang_oper_lshift,*/
	/*slang_oper_rshift,*/
	slang_oper_add,
	slang_oper_subtract,
	slang_oper_multiply,
	slang_oper_divide,
	/*slang_oper_modulus,*/
	slang_oper_preincrement,
	slang_oper_predecrement,
	slang_oper_plus,
	slang_oper_minus,
	/*slang_oper_complement,*/
	slang_oper_not,
	slang_oper_subscript,
	slang_oper_call,
	slang_oper_field,
	slang_oper_postincrement,
	slang_oper_postdecrement
} slang_operation_type;

typedef struct slang_operation_
{
	slang_operation_type type;
	struct slang_operation_ *children;
	unsigned int num_children;
	float literal;		/* bool, literal_int, literal_float */
	char *identifier;	/* asm, identifier, call, field */
	slang_variable_scope *locals;
} slang_operation;

int slang_operation_construct_a (slang_operation *);
void slang_operation_destruct (slang_operation *);

typedef struct slang_variable_
{
	slang_fully_specified_type type;
	char *name;
	slang_operation *array_size;	/* spec_array */
	slang_operation *initializer;
	unsigned int address;
} slang_variable;

slang_variable *_slang_locate_variable (slang_variable_scope *scope, const char *name, int all);

typedef struct slang_struct_scope_
{
	struct slang_struct_ *structs;
	unsigned int num_structs;
	struct slang_struct_scope_ *outer_scope;
} slang_struct_scope;

struct slang_struct_ *slang_struct_scope_find (slang_struct_scope *, const char *, int);

typedef struct slang_struct_
{
	char *name;
	slang_variable_scope *fields;
	slang_struct_scope *structs;
} slang_struct;

int slang_struct_construct_a (slang_struct *);
int slang_struct_copy (slang_struct *, const slang_struct *);

typedef enum slang_function_kind_
{
	slang_func_ordinary,
	slang_func_constructor,
	slang_func_operator
} slang_function_kind;

typedef struct slang_function_
{
	slang_function_kind kind;
	slang_variable header;
	slang_variable_scope *parameters;
	unsigned int param_count;
	slang_operation *body;
	unsigned int address;
} slang_function;

typedef struct slang_function_scope_
{
	slang_function *functions;
	unsigned int num_functions;
	struct slang_function_scope_ *outer_scope;
} slang_function_scope;

typedef enum slang_unit_type_
{
	slang_unit_fragment_shader,
	slang_unit_vertex_shader,
	slang_unit_fragment_builtin,
	slang_unit_vertex_builtin
} slang_unit_type;
	
typedef struct slang_translation_unit_
{
	slang_variable_scope globals;
	slang_function_scope functions;
	slang_struct_scope structs;
	slang_unit_type type;
} slang_translation_unit;

void slang_translation_unit_construct (slang_translation_unit *);
void slang_translation_unit_destruct (slang_translation_unit *);

typedef struct slang_info_log_
{
	char *text;
	int dont_free_text;
} slang_info_log;

void slang_info_log_construct (slang_info_log *);
void slang_info_log_destruct (slang_info_log *);
int slang_info_log_error (slang_info_log *, const char *, ...);
int slang_info_log_warning (slang_info_log *, const char *, ...);
void slang_info_log_memory (slang_info_log *);

int _slang_compile (const char *, slang_translation_unit *, slang_unit_type type, slang_info_log *);

#ifdef __cplusplus
}
#endif

#endif

