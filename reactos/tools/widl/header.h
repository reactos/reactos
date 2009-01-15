/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
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

#ifndef __WIDL_HEADER_H
#define __WIDL_HEADER_H

#include "widltypes.h"

extern int is_ptrchain_attr(const var_t *var, enum attr_type t);
extern int is_aliaschain_attr(const type_t *var, enum attr_type t);
extern int is_attr(const attr_list_t *list, enum attr_type t);
extern void *get_attrp(const attr_list_t *list, enum attr_type t);
extern unsigned long get_attrv(const attr_list_t *list, enum attr_type t);
extern int is_void(const type_t *t);
extern int is_conformant_array(const type_t *t);
extern int is_declptr(const type_t *t);
extern const char* get_name(const var_t *v);
extern void write_type_left(FILE *h, type_t *t, int declonly);
extern void write_type_right(FILE *h, type_t *t, int is_field);
extern void write_type_def_or_decl(FILE *h, type_t *t, int is_field, const char *fmt, ...);
extern void write_type_decl(FILE *f, type_t *t, const char *fmt, ...);
extern void write_type_decl_left(FILE *f, type_t *t);
extern int needs_space_after(type_t *t);
extern int is_object(const attr_list_t *list);
extern int is_local(const attr_list_t *list);
extern int need_stub(const type_t *iface);
extern int need_proxy(const type_t *iface);
extern int need_stub_files(const statement_list_t *stmts);
extern int need_proxy_file(const statement_list_t *stmts);
extern const var_t *is_callas(const attr_list_t *list);
extern void write_args(FILE *h, const var_list_t *arg, const char *name, int obj, int do_indent);
extern void write_array(FILE *h, array_dims_t *v, int field);
extern const var_t* get_explicit_handle_var(const func_t* func);
extern const type_t* get_explicit_generic_handle_type(const var_t* var);
extern const var_t* get_explicit_generic_handle_var(const func_t* func);
extern const var_t* get_context_handle_var(const func_t* func);
extern int has_out_arg_or_return(const func_t *func);
extern void write_guid(FILE *f, const char *guid_prefix, const char *name,
                       const UUID *uuid);
extern int is_const_decl(const var_t *var);

static inline int last_ptr(const type_t *type)
{
    return is_ptr(type) && !is_declptr(type->ref);
}

static inline int last_array(const type_t *type)
{
    return is_array(type) && !is_array(type->ref);
}

static inline int is_string_type(const attr_list_t *attrs, const type_t *type)
{
    return ((is_attr(attrs, ATTR_STRING) || is_aliaschain_attr(type, ATTR_STRING))
            && (last_ptr(type) || last_array(type)));
}

static inline int is_context_handle(const type_t *type)
{
    const type_t *t;
    for (t = type; is_ptr(t); t = t->ref)
        if (is_attr(t->attrs, ATTR_CONTEXTHANDLE))
            return 1;
    return 0;
}

#endif
