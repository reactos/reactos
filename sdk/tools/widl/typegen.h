/*
 * Format String Generator for IDL Compiler
 *
 * Copyright 2005-2006 Eric Kohl
 * Copyright 2005 Robert Shearman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

enum pass
{
    PASS_IN,
    PASS_OUT,
    PASS_RETURN
};

enum remoting_phase
{
    PHASE_BUFFERSIZE,
    PHASE_MARSHAL,
    PHASE_UNMARSHAL,
    PHASE_FREE
};

enum typegen_detect_flags
{
    TDT_ALL_TYPES =      1 << 0,
    TDT_IGNORE_STRINGS = 1 << 1,
    TDT_IGNORE_RANGES =  1 << 2,
};

enum typegen_type
{
    TGT_INVALID,
    TGT_USER_TYPE,
    TGT_CTXT_HANDLE,
    TGT_CTXT_HANDLE_POINTER,
    TGT_STRING,
    TGT_POINTER,
    TGT_ARRAY,
    TGT_IFACE_POINTER,
    TGT_BASIC,
    TGT_ENUM,
    TGT_STRUCT,
    TGT_UNION,
    TGT_RANGE,
};

typedef int (*type_pred_t)(const type_t *);

void write_formatstringsdecl(FILE *f, int indent, const statement_list_t *stmts, type_pred_t pred);
void write_procformatstring(FILE *file, const statement_list_t *stmts, type_pred_t pred);
void write_typeformatstring(FILE *file, const statement_list_t *stmts, type_pred_t pred);
void write_procformatstring_offsets( FILE *file, const type_t *iface );
void print_phase_basetype(FILE *file, int indent, const char *local_var_prefix, enum remoting_phase phase,
                          enum pass pass, const var_t *var, const char *varname);
void write_parameter_conf_or_var_exprs(FILE *file, int indent, const char *local_var_prefix,
                                       enum remoting_phase phase, const var_t *var, int valid_variance);
void write_remoting_arguments(FILE *file, int indent, const var_t *func, const char *local_var_prefix,
                              enum pass pass, enum remoting_phase phase);
unsigned int get_size_procformatstring_func(const type_t *iface, const var_t *func);
unsigned int get_size_procformatstring(const statement_list_t *stmts, type_pred_t pred);
unsigned int get_size_typeformatstring(const statement_list_t *stmts, type_pred_t pred);
void assign_stub_out_args( FILE *file, int indent, const var_t *func, const char *local_var_prefix );
void declare_stub_args( FILE *file, int indent, const var_t *func );
void write_func_param_struct( FILE *file, const type_t *iface, const type_t *func,
                              const char *var_decl, int add_retval );
void write_pointer_checks( FILE *file, int indent, const var_t *func );
int write_expr_eval_routines(FILE *file, const char *iface);
void write_expr_eval_routine_list(FILE *file, const char *iface);
void write_user_quad_list(FILE *file);
void write_endpoints( FILE *f, const char *prefix, const str_list_t *list );
void write_client_call_routine( FILE *file, const type_t *iface, const var_t *func,
                                const char *prefix, unsigned int proc_offset );
void write_exceptions( FILE *file );
unsigned int type_memsize(const type_t *t);
int decl_indirect(const type_t *t);
int is_interpreted_func(const type_t *iface, const var_t *func);
void write_parameters_init(FILE *file, int indent, const var_t *func, const char *local_var_prefix);
void print(FILE *file, int indent, const char *format, va_list ap);
expr_t *get_size_is_expr(const type_t *t, const char *name);
int is_full_pointer_function(const var_t *func);
void write_full_pointer_init(FILE *file, int indent, const var_t *func, int is_server);
void write_full_pointer_free(FILE *file, int indent, const var_t *func);
unsigned char get_basic_fc(const type_t *type);
unsigned char get_pointer_fc(const type_t *type, const attr_list_t *attrs, int toplevel_param);
unsigned char get_struct_fc(const type_t *type);
enum typegen_type typegen_detect_type(const type_t *type, const attr_list_t *attrs, unsigned int flags);
unsigned int type_memsize_and_alignment(const type_t *t, unsigned int *align);
