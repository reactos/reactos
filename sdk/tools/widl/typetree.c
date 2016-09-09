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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "typetree.h"
#include "header.h"

type_t *duptype(type_t *t, int dupname)
{
  type_t *d = alloc_type();

  *d = *t;
  if (dupname && t->name)
    d->name = xstrdup(t->name);

  return d;
}

type_t *make_type(enum type_type type)
{
    type_t *t = alloc_type();
    t->name = NULL;
    t->namespace = NULL;
    t->type_type = type;
    t->attrs = NULL;
    t->c_name = NULL;
    t->orig = NULL;
    memset(&t->details, 0, sizeof(t->details));
    t->typestring_offset = 0;
    t->ptrdesc = 0;
    t->ignore = (parse_only != 0);
    t->defined = FALSE;
    t->written = FALSE;
    t->user_types_registered = FALSE;
    t->tfswrite = FALSE;
    t->checked = FALSE;
    t->is_alias = FALSE;
    t->typelib_idx = -1;
    init_loc_info(&t->loc_info);
    return t;
}

static const var_t *find_arg(const var_list_t *args, const char *name)
{
    const var_t *arg;

    if (args) LIST_FOR_EACH_ENTRY(arg, args, const var_t, entry)
    {
        if (arg->name && !strcmp(name, arg->name))
            return arg;
    }

    return NULL;
}

const char *type_get_name(const type_t *type, enum name_type name_type)
{
    switch(name_type) {
    case NAME_DEFAULT:
        return type->name;
    case NAME_C:
        return type->c_name;
    }

    assert(0);
    return NULL;
}

static char *append_namespace(char *ptr, struct namespace *namespace, const char *separator)
{
    if(is_global_namespace(namespace)) {
        if(!use_abi_namespace)
            return ptr;
        strcpy(ptr, "ABI");
        strcat(ptr, separator);
        return ptr + strlen(ptr);
    }

    ptr = append_namespace(ptr, namespace->parent, separator);
    strcpy(ptr, namespace->name);
    strcat(ptr, separator);
    return ptr + strlen(ptr);
}

char *format_namespace(struct namespace *namespace, const char *prefix, const char *separator, const char *suffix)
{
    unsigned len = strlen(prefix) + strlen(suffix);
    unsigned sep_len = strlen(separator);
    struct namespace *iter;
    char *ret, *ptr;

    if(use_abi_namespace && !is_global_namespace(namespace))
        len += 3 /* strlen("ABI") */ + sep_len;

    for(iter = namespace; !is_global_namespace(iter); iter = iter->parent)
        len += strlen(iter->name) + sep_len;

    ret = xmalloc(len+1);
    strcpy(ret, prefix);
    ptr = append_namespace(ret + strlen(ret), namespace, separator);
    strcpy(ptr, suffix);

    return ret;
}

type_t *type_new_function(var_list_t *args)
{
    var_t *arg;
    type_t *t;
    unsigned int i = 0;

    if (args)
    {
        arg = LIST_ENTRY(list_head(args), var_t, entry);
        if (list_count(args) == 1 && !arg->name && arg->type && type_get_type(arg->type) == TYPE_VOID)
        {
            list_remove(&arg->entry);
            free(arg);
            free(args);
            args = NULL;
        }
    }
    if (args) LIST_FOR_EACH_ENTRY(arg, args, var_t, entry)
    {
        if (arg->type && type_get_type(arg->type) == TYPE_VOID)
            error_loc("argument '%s' has void type\n", arg->name);
        if (!arg->name)
        {
            if (i > 26 * 26)
                error_loc("too many unnamed arguments\n");
            else
            {
                int unique;
                do
                {
                    char name[3];
                    name[0] = i > 26 ? 'a' + i / 26 : 'a' + i;
                    name[1] = i > 26 ? 'a' + i % 26 : 0;
                    name[2] = 0;
                    unique = !find_arg(args, name);
                    if (unique)
                        arg->name = xstrdup(name);
                    i++;
                } while (!unique);
            }
        }
    }

    t = make_type(TYPE_FUNCTION);
    t->details.function = xmalloc(sizeof(*t->details.function));
    t->details.function->args = args;
    t->details.function->idx = -1;
    return t;
}

type_t *type_new_pointer(unsigned char pointer_default, type_t *ref, attr_list_t *attrs)
{
    type_t *t = make_type(TYPE_POINTER);
    t->details.pointer.def_fc = pointer_default;
    t->details.pointer.ref = ref;
    t->attrs = attrs;
    return t;
}

type_t *type_new_alias(type_t *t, const char *name)
{
    type_t *a = duptype(t, 0);

    a->name = xstrdup(name);
    a->attrs = NULL;
    a->orig = t;
    a->is_alias = TRUE;
    /* for pointer types */
    a->details = t->details;
    init_loc_info(&a->loc_info);

    return a;
}

type_t *type_new_module(char *name)
{
    type_t *type = get_type(TYPE_MODULE, name, NULL, 0);
    if (type->type_type != TYPE_MODULE || type->defined)
        error_loc("%s: redefinition error; original definition was at %s:%d\n",
                  type->name, type->loc_info.input_name, type->loc_info.line_number);
    type->name = name;
    return type;
}

type_t *type_new_coclass(char *name)
{
    type_t *type = get_type(TYPE_COCLASS, name, NULL, 0);
    if (type->type_type != TYPE_COCLASS || type->defined)
        error_loc("%s: redefinition error; original definition was at %s:%d\n",
                  type->name, type->loc_info.input_name, type->loc_info.line_number);
    type->name = name;
    return type;
}


type_t *type_new_array(const char *name, type_t *element, int declptr,
                       unsigned int dim, expr_t *size_is, expr_t *length_is,
                       unsigned char ptr_default_fc)
{
    type_t *t = make_type(TYPE_ARRAY);
    if (name) t->name = xstrdup(name);
    t->details.array.declptr = declptr;
    t->details.array.length_is = length_is;
    if (size_is)
        t->details.array.size_is = size_is;
    else
        t->details.array.dim = dim;
    t->details.array.elem = element;
    t->details.array.ptr_def_fc = ptr_default_fc;
    return t;
}

type_t *type_new_basic(enum type_basic_type basic_type)
{
    type_t *t = make_type(TYPE_BASIC);
    t->details.basic.type = basic_type;
    t->details.basic.sign = 0;
    return t;
}

type_t *type_new_int(enum type_basic_type basic_type, int sign)
{
    static type_t *int_types[TYPE_BASIC_INT_MAX+1][3];

    assert(basic_type <= TYPE_BASIC_INT_MAX);

    /* map sign { -1, 0, 1 } -> { 0, 1, 2 } */
    if (!int_types[basic_type][sign + 1])
    {
        int_types[basic_type][sign + 1] = type_new_basic(basic_type);
        int_types[basic_type][sign + 1]->details.basic.sign = sign;
    }
    return int_types[basic_type][sign + 1];
}

type_t *type_new_void(void)
{
    static type_t *void_type = NULL;
    if (!void_type)
        void_type = make_type(TYPE_VOID);
    return void_type;
}

type_t *type_new_enum(const char *name, struct namespace *namespace, int defined, var_list_t *enums)
{
    type_t *tag_type = name ? find_type(name, namespace, tsENUM) : NULL;
    type_t *t = make_type(TYPE_ENUM);
    t->name = name;
    t->namespace = namespace;

    if (tag_type && tag_type->details.enumeration)
        t->details.enumeration = tag_type->details.enumeration;
    else if (defined)
    {
        t->details.enumeration = xmalloc(sizeof(*t->details.enumeration));
        t->details.enumeration->enums = enums;
        t->defined = TRUE;
    }

    if (name)
    {
        if (defined)
            reg_type(t, name, namespace, tsENUM);
        else
            add_incomplete(t);
    }
    return t;
}

type_t *type_new_struct(char *name, struct namespace *namespace, int defined, var_list_t *fields)
{
    type_t *tag_type = name ? find_type(name, namespace, tsSTRUCT) : NULL;
    type_t *t;

    /* avoid creating duplicate typelib type entries */
    if (tag_type && do_typelib) return tag_type;

    t = make_type(TYPE_STRUCT);
    t->name = name;
    t->namespace = namespace;

    if (tag_type && tag_type->details.structure)
        t->details.structure = tag_type->details.structure;
    else if (defined)
    {
        t->details.structure = xmalloc(sizeof(*t->details.structure));
        t->details.structure->fields = fields;
        t->defined = TRUE;
    }
    if (name)
    {
        if (defined)
            reg_type(t, name, namespace, tsSTRUCT);
        else
            add_incomplete(t);
    }
    return t;
}

type_t *type_new_nonencapsulated_union(const char *name, int defined, var_list_t *fields)
{
    type_t *tag_type = name ? find_type(name, NULL, tsUNION) : NULL;
    type_t *t = make_type(TYPE_UNION);
    t->name = name;
    if (tag_type && tag_type->details.structure)
        t->details.structure = tag_type->details.structure;
    else if (defined)
    {
        t->details.structure = xmalloc(sizeof(*t->details.structure));
        t->details.structure->fields = fields;
        t->defined = TRUE;
    }
    if (name)
    {
        if (defined)
            reg_type(t, name, NULL, tsUNION);
        else
            add_incomplete(t);
    }
    return t;
}

type_t *type_new_encapsulated_union(char *name, var_t *switch_field, var_t *union_field, var_list_t *cases)
{
    type_t *t = get_type(TYPE_ENCAPSULATED_UNION, name, NULL, tsUNION);
    if (!union_field) union_field = make_var( xstrdup("tagged_union") );
    union_field->type = type_new_nonencapsulated_union(NULL, TRUE, cases);
    t->details.structure = xmalloc(sizeof(*t->details.structure));
    t->details.structure->fields = append_var( NULL, switch_field );
    t->details.structure->fields = append_var( t->details.structure->fields, union_field );
    t->defined = TRUE;
    return t;
}

static int is_valid_bitfield_type(const type_t *type)
{
    switch (type_get_type(type))
    {
    case TYPE_ENUM:
        return TRUE;
    case TYPE_BASIC:
        switch (type_basic_get_type(type))
        {
        case TYPE_BASIC_INT8:
        case TYPE_BASIC_INT16:
        case TYPE_BASIC_INT32:
        case TYPE_BASIC_INT64:
        case TYPE_BASIC_INT:
        case TYPE_BASIC_INT3264:
        case TYPE_BASIC_CHAR:
        case TYPE_BASIC_HYPER:
        case TYPE_BASIC_BYTE:
        case TYPE_BASIC_WCHAR:
        case TYPE_BASIC_ERROR_STATUS_T:
            return TRUE;
        case TYPE_BASIC_FLOAT:
        case TYPE_BASIC_DOUBLE:
        case TYPE_BASIC_HANDLE:
            return FALSE;
        }
        return FALSE;
    default:
        return FALSE;
    }
}

type_t *type_new_bitfield(type_t *field, const expr_t *bits)
{
    type_t *t;

    if (!is_valid_bitfield_type(field))
        error_loc("bit-field has invalid type\n");

    if (bits->cval < 0)
        error_loc("negative width for bit-field\n");

    /* FIXME: validate bits->cval <= memsize(field) * 8 */

    t = make_type(TYPE_BITFIELD);
    t->details.bitfield.field = field;
    t->details.bitfield.bits = bits;
    return t;
}

static int compute_method_indexes(type_t *iface)
{
    int idx;
    statement_t *stmt;

    if (!iface->details.iface)
        return 0;

    if (type_iface_get_inherit(iface))
        idx = compute_method_indexes(type_iface_get_inherit(iface));
    else
        idx = 0;

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        var_t *func = stmt->u.var;
        if (!is_callas(func->attrs))
            func->type->details.function->idx = idx++;
    }

    return idx;
}

void type_interface_define(type_t *iface, type_t *inherit, statement_list_t *stmts)
{
    iface->details.iface = xmalloc(sizeof(*iface->details.iface));
    iface->details.iface->disp_props = NULL;
    iface->details.iface->disp_methods = NULL;
    iface->details.iface->stmts = stmts;
    iface->details.iface->inherit = inherit;
    iface->defined = TRUE;
    compute_method_indexes(iface);
}

void type_dispinterface_define(type_t *iface, var_list_t *props, var_list_t *methods)
{
    iface->details.iface = xmalloc(sizeof(*iface->details.iface));
    iface->details.iface->disp_props = props;
    iface->details.iface->disp_methods = methods;
    iface->details.iface->stmts = NULL;
    iface->details.iface->inherit = find_type("IDispatch", NULL, 0);
    if (!iface->details.iface->inherit) error_loc("IDispatch is undefined\n");
    iface->defined = TRUE;
    compute_method_indexes(iface);
}

void type_dispinterface_define_from_iface(type_t *dispiface, type_t *iface)
{
    type_dispinterface_define(dispiface, iface->details.iface->disp_props,
                              iface->details.iface->disp_methods);
}

void type_module_define(type_t *module, statement_list_t *stmts)
{
    if (module->details.module) error_loc("multiple definition error\n");
    module->details.module = xmalloc(sizeof(*module->details.module));
    module->details.module->stmts = stmts;
    module->defined = TRUE;
}

type_t *type_coclass_define(type_t *coclass, ifref_list_t *ifaces)
{
    coclass->details.coclass.ifaces = ifaces;
    coclass->defined = TRUE;
    return coclass;
}

int type_is_equal(const type_t *type1, const type_t *type2)
{
    if (type_get_type_detect_alias(type1) != type_get_type_detect_alias(type2))
        return FALSE;

    if (type1->name && type2->name)
        return !strcmp(type1->name, type2->name);
    else if ((!type1->name && type2->name) || (type1->name && !type2->name))
        return FALSE;

    /* FIXME: do deep inspection of types to determine if they are equal */

    return FALSE;
}
