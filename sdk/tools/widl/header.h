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

#include "typetree.h"

extern const char* get_name(const var_t *v);
extern void write_type_left(FILE *h, const decl_spec_t *ds, enum name_type name_type, bool define, int write_callconv);
extern void write_type_right(FILE *h, type_t *t, int is_field);
extern void write_type_decl(FILE *f, const decl_spec_t *t, const char *name);
extern void write_type_decl_left(FILE *f, const decl_spec_t *ds);
extern unsigned int get_context_handle_offset( const type_t *type );
extern unsigned int get_generic_handle_offset( const type_t *type );
extern int needs_space_after(type_t *t);
extern int is_object(const type_t *iface);
extern int is_local(const attr_list_t *list);
extern int count_methods(const type_t *iface);
extern const statement_t * get_callas_source(const type_t *iface, const var_t *def);
extern int need_stub(const type_t *iface);
extern int need_proxy(const type_t *iface);
extern int need_inline_stubs(const type_t *iface);
extern int need_stub_files(const statement_list_t *stmts);
extern int need_proxy_file(const statement_list_t *stmts);
extern int need_proxy_delegation(const statement_list_t *stmts);
extern int need_inline_stubs_file(const statement_list_t *stmts);
extern const var_t *is_callas(const attr_list_t *list);
extern void write_args(FILE *h, const var_list_t *arg, const char *name, int obj, int do_indent, enum name_type name_type);
extern const type_t* get_explicit_generic_handle_type(const var_t* var);
extern const var_t *get_func_handle_var( const type_t *iface, const var_t *func,
                                         unsigned char *explicit_fc, unsigned char *implicit_fc );
extern int has_out_arg_or_return(const var_t *func);
extern int is_const_decl(const var_t *var);

extern void write_serialize_functions(FILE *file, const type_t *type, const type_t *iface);

static inline int is_ptr(const type_t *t)
{
    return type_get_type(t) == TYPE_POINTER;
}

static inline int is_array(const type_t *t)
{
    return type_get_type(t) == TYPE_ARRAY;
}

static inline int is_func(const type_t *t)
{
    return type_get_type(t) == TYPE_FUNCTION;
}

static inline int is_void(const type_t *t)
{
    return type_get_type(t) == TYPE_VOID;
}

static inline int is_declptr(const type_t *t)
{
    return is_ptr(t) || (type_get_type(t) == TYPE_ARRAY && type_array_is_decl_as_ptr(t));
}

static inline int is_conformant_array(const type_t *t)
{
    return is_array(t) && type_array_has_conformance(t);
}

static inline int last_ptr(const type_t *type)
{
    return is_ptr(type) && !is_declptr(type_pointer_get_ref_type(type));
}

static inline int last_array(const type_t *type)
{
    return is_array(type) && !is_array(type_array_get_element_type(type));
}

static inline int is_string_type(const attr_list_t *attrs, const type_t *type)
{
    return ((is_attr(attrs, ATTR_STRING) || is_aliaschain_attr(type, ATTR_STRING))
            && (last_ptr(type) || last_array(type)));
}

static inline int is_context_handle(const type_t *type)
{
    const type_t *t;
    for (t = type;
         is_ptr(t) || type_is_alias(t);
         t = type_is_alias(t) ? type_alias_get_aliasee_type(t) : type_pointer_get_ref_type(t))
        if (is_attr(t->attrs, ATTR_CONTEXTHANDLE))
            return 1;
    return 0;
}

#endif
