/*
 * IDL Type Tree
 *
 * Copyright 2008 Robert Shearman
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

#include "widltypes.h"
#include <assert.h>

#ifndef WIDL_TYPE_TREE_H
#define WIDL_TYPE_TREE_H

enum name_type {
    NAME_DEFAULT,
    NAME_C
};

type_t *type_new_function(var_list_t *args);
type_t *type_new_pointer(unsigned char pointer_default, type_t *ref, attr_list_t *attrs);
type_t *type_new_alias(type_t *t, const char *name);
type_t *type_new_module(char *name);
type_t *type_new_array(const char *name, type_t *element, int declptr,
                       unsigned int dim, expr_t *size_is, expr_t *length_is,
                       unsigned char ptr_default_fc);
type_t *type_new_basic(enum type_basic_type basic_type);
type_t *type_new_int(enum type_basic_type basic_type, int sign);
type_t *type_new_void(void);
type_t *type_new_coclass(char *name);
type_t *type_new_enum(const char *name, struct namespace *namespace, int defined, var_list_t *enums);
type_t *type_new_struct(char *name, struct namespace *namespace, int defined, var_list_t *fields);
type_t *type_new_nonencapsulated_union(const char *name, int defined, var_list_t *fields);
type_t *type_new_encapsulated_union(char *name, var_t *switch_field, var_t *union_field, var_list_t *cases);
type_t *type_new_bitfield(type_t *field_type, const expr_t *bits);
void type_interface_define(type_t *iface, type_t *inherit, statement_list_t *stmts);
void type_dispinterface_define(type_t *iface, var_list_t *props, var_list_t *methods);
void type_dispinterface_define_from_iface(type_t *dispiface, type_t *iface);
void type_module_define(type_t *module, statement_list_t *stmts);
type_t *type_coclass_define(type_t *coclass, ifref_list_t *ifaces);
int type_is_equal(const type_t *type1, const type_t *type2);
const char *type_get_name(const type_t *type, enum name_type name_type);

/* FIXME: shouldn't need to export this */
type_t *duptype(type_t *t, int dupname);

/* un-alias the type until finding the non-alias type */
static inline type_t *type_get_real_type(const type_t *type)
{
    if (type->is_alias)
        return type_get_real_type(type->orig);
    else
        return (type_t *)type;
}

static inline enum type_type type_get_type(const type_t *type)
{
    return type_get_type_detect_alias(type_get_real_type(type));
}

static inline enum type_basic_type type_basic_get_type(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_BASIC);
    return type->details.basic.type;
}

static inline int type_basic_get_sign(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_BASIC);
    return type->details.basic.sign;
}

static inline var_list_t *type_struct_get_fields(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_STRUCT);
    return type->details.structure->fields;
}

static inline var_list_t *type_function_get_args(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_FUNCTION);
    return type->details.function->args;
}

static inline var_t *type_function_get_retval(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_FUNCTION);
    return type->details.function->retval;
}

static inline type_t *type_function_get_rettype(const type_t *type)
{
    return type_function_get_retval(type)->type;
}

static inline var_list_t *type_enum_get_values(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_ENUM);
    return type->details.enumeration->enums;
}

static inline var_t *type_union_get_switch_value(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_ENCAPSULATED_UNION);
    return LIST_ENTRY(list_head(type->details.structure->fields), var_t, entry);
}

static inline var_list_t *type_encapsulated_union_get_fields(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_ENCAPSULATED_UNION);
    return type->details.structure->fields;
}

static inline var_list_t *type_union_get_cases(const type_t *type)
{
    enum type_type type_type;

    type = type_get_real_type(type);
    type_type = type_get_type(type);

    assert(type_type == TYPE_UNION || type_type == TYPE_ENCAPSULATED_UNION);
    if (type_type == TYPE_ENCAPSULATED_UNION)
    {
        const var_t *uv = LIST_ENTRY(list_tail(type->details.structure->fields), const var_t, entry);
        return uv->type->details.structure->fields;
    }
    else
        return type->details.structure->fields;
}

static inline statement_list_t *type_iface_get_stmts(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_INTERFACE);
    return type->details.iface->stmts;
}

static inline type_t *type_iface_get_inherit(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_INTERFACE);
    return type->details.iface->inherit;
}

static inline var_list_t *type_dispiface_get_props(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_INTERFACE);
    return type->details.iface->disp_props;
}

static inline var_list_t *type_dispiface_get_methods(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_INTERFACE);
    return type->details.iface->disp_methods;
}

static inline int type_is_defined(const type_t *type)
{
    return type->defined;
}

static inline int type_is_complete(const type_t *type)
{
    switch (type_get_type_detect_alias(type))
    {
    case TYPE_FUNCTION:
        return (type->details.function != NULL);
    case TYPE_INTERFACE:
        return (type->details.iface != NULL);
    case TYPE_ENUM:
        return (type->details.enumeration != NULL);
    case TYPE_UNION:
    case TYPE_ENCAPSULATED_UNION:
    case TYPE_STRUCT:
        return (type->details.structure != NULL);
    case TYPE_VOID:
    case TYPE_BASIC:
    case TYPE_ALIAS:
    case TYPE_MODULE:
    case TYPE_COCLASS:
    case TYPE_POINTER:
    case TYPE_ARRAY:
    case TYPE_BITFIELD:
        return TRUE;
    }
    return FALSE;
}

static inline int type_array_has_conformance(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_ARRAY);
    return (type->details.array.size_is != NULL);
}

static inline int type_array_has_variance(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_ARRAY);
    return (type->details.array.length_is != NULL);
}

static inline unsigned int type_array_get_dim(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_ARRAY);
    return type->details.array.dim;
}

static inline expr_t *type_array_get_conformance(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_ARRAY);
    return type->details.array.size_is;
}

static inline expr_t *type_array_get_variance(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_ARRAY);
    return type->details.array.length_is;
}

static inline type_t *type_array_get_element(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_ARRAY);
    return type->details.array.elem;
}

static inline int type_array_is_decl_as_ptr(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_ARRAY);
    return type->details.array.declptr;
}

static inline unsigned char type_array_get_ptr_default_fc(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_ARRAY);
    return type->details.array.ptr_def_fc;
}

static inline int type_is_alias(const type_t *type)
{
    return type->is_alias;
}

static inline type_t *type_alias_get_aliasee(const type_t *type)
{
    assert(type_is_alias(type));
    return type->orig;
}

static inline ifref_list_t *type_coclass_get_ifaces(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_COCLASS);
    return type->details.coclass.ifaces;
}

static inline type_t *type_pointer_get_ref(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_POINTER);
    return type->details.pointer.ref;
}

static inline unsigned char type_pointer_get_default_fc(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_POINTER);
    return type->details.pointer.def_fc;
}

static inline type_t *type_bitfield_get_field(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_BITFIELD);
    return type->details.bitfield.field;
}

static inline const expr_t *type_bitfield_get_bits(const type_t *type)
{
    type = type_get_real_type(type);
    assert(type_get_type(type) == TYPE_BITFIELD);
    return type->details.bitfield.bits;
}

#endif /* WIDL_TYPE_TREE_H */
