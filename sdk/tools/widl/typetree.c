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
#ifdef __REACTOS__
#include <stddef.h>
#endif

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "typetree.h"
#include "header.h"
#include "hash.h"

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
    t->signature = NULL;
    t->qualified_name = NULL;
    t->impl_name = NULL;
    t->param_name = NULL;
    t->short_name = NULL;
    memset(&t->details, 0, sizeof(t->details));
    t->typestring_offset = 0;
    t->ptrdesc = 0;
    t->ignore = (parse_only != 0);
    t->defined = FALSE;
    t->written = FALSE;
    t->user_types_registered = FALSE;
    t->tfswrite = FALSE;
    t->checked = FALSE;
    t->typelib_idx = -1;
    init_location( &t->where, NULL, NULL );
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

const char *type_get_decl_name(const type_t *type, enum name_type name_type)
{
    switch(name_type) {
    case NAME_DEFAULT:
        return type->name;
    case NAME_C:
        return type->c_name ? type->c_name : type->name;
    }

    assert(0);
    return NULL;
}

const char *type_get_name(const type_t *type, enum name_type name_type)
{
    switch(name_type) {
    case NAME_DEFAULT:
        return type->qualified_name ? type->qualified_name : type->name;
    case NAME_C:
        return type->c_name ? type->c_name : type->name;
    }

    assert(0);
    return NULL;
}

static size_t append_namespace(char **buf, size_t *len, size_t pos, struct namespace *namespace, const char *separator, const char *abi_prefix)
{
    int nested = namespace && !is_global_namespace(namespace);
    const char *name = nested ? namespace->name : abi_prefix;
    size_t n = 0;
    if (!name) return 0;
    if (nested) n += append_namespace(buf, len, pos + n, namespace->parent, separator, abi_prefix);
    n += strappend(buf, len, pos + n, "%s%s", name, separator);
    return n;
}

static size_t append_namespaces(char **buf, size_t *len, size_t pos, struct namespace *namespace, const char *prefix,
                                const char *separator, const char *suffix, const char *abi_prefix)
{
    int nested = namespace && !is_global_namespace(namespace);
    size_t n = 0;
    n += strappend(buf, len, pos + n, "%s", prefix);
    if (nested) n += append_namespace(buf, len, pos + n, namespace, separator, abi_prefix);
    if (suffix) n += strappend(buf, len, pos + n, "%s", suffix);
    else if (nested)
    {
        n -= strlen(separator);
        (*buf)[n] = 0;
    }
    return n;
}

static size_t append_pointer_stars(char **buf, size_t *len, size_t pos, type_t *type)
{
    size_t n = 0;
    for (; type && type->type_type == TYPE_POINTER; type = type_pointer_get_ref_type(type)) n += strappend(buf, len, pos + n, "*");
    return n;
}

static size_t append_type_signature(char **buf, size_t *len, size_t pos, type_t *type);

static size_t append_var_list_signature(char **buf, size_t *len, size_t pos, var_list_t *var_list)
{
    var_t *var;
    size_t n = 0;

    if (!var_list) n += strappend(buf, len, pos + n, ";");
    else LIST_FOR_EACH_ENTRY(var, var_list, var_t, entry)
    {
        n += strappend(buf, len, pos + n, ";");
        n += append_type_signature(buf, len, pos + n, var->declspec.type);
    }

    return n;
}

static size_t append_type_signature(char **buf, size_t *len, size_t pos, type_t *type)
{
    const struct uuid *uuid;
    size_t n = 0;

    if (!type) return 0;
    switch (type->type_type)
    {
    case TYPE_INTERFACE:
        if (!strcmp(type->name, "IInspectable")) n += strappend(buf, len, pos + n, "cinterface(IInspectable)");
        else if (type->signature) n += strappend(buf, len, pos + n, "%s", type->signature);
        else
        {
            if (!(uuid = get_attrp( type->attrs, ATTR_UUID )))
                error_at( &type->where, "cannot compute type signature, no uuid found for type %s.\n", type->name );

            n += strappend(buf, len, pos + n, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                           uuid->Data1, uuid->Data2, uuid->Data3,
                           uuid->Data4[0], uuid->Data4[1], uuid->Data4[2], uuid->Data4[3],
                           uuid->Data4[4], uuid->Data4[5], uuid->Data4[6], uuid->Data4[7]);
        }
        return n;
    case TYPE_DELEGATE:
        n += strappend(buf, len, pos + n, "delegate(");
        n += append_type_signature(buf, len, pos + n, type_delegate_get_iface(type));
        n += strappend(buf, len, pos + n, ")");
        return n;
    case TYPE_RUNTIMECLASS:
        n += strappend(buf, len, pos + n, "rc(");
        n += append_namespaces(buf, len, pos + n, type->namespace, "", ".", type->name, NULL);
        n += strappend(buf, len, pos + n, ";");
        n += append_type_signature(buf, len, pos + n, type_runtimeclass_get_default_iface(type, TRUE));
        n += strappend(buf, len, pos + n, ")");
        return n;
    case TYPE_POINTER:
        n += append_type_signature(buf, len, pos + n, type->details.pointer.ref.type);
        return n;
    case TYPE_ALIAS:
        if (!strcmp(type->name, "boolean")) n += strappend(buf, len, pos + n, "b1");
        else if (!strcmp(type->name, "GUID")) n += strappend(buf, len, pos + n, "g16");
        else if (!strcmp(type->name, "HSTRING")) n += strappend(buf, len, pos + n, "string");
        else n += append_type_signature(buf, len, pos + n, type->details.alias.aliasee.type);
        return n;
    case TYPE_STRUCT:
        n += strappend(buf, len, pos + n, "struct(");
        n += append_namespaces(buf, len, pos + n, type->namespace, "", ".", type->name, NULL);
        n += append_var_list_signature(buf, len, pos + n, type->details.structure->fields);
        n += strappend(buf, len, pos + n, ")");
        return n;
    case TYPE_BASIC:
        switch (type_basic_get_type(type))
        {
        case TYPE_BASIC_INT16:
            n += strappend(buf, len, pos + n, type_basic_get_sign(type) <= 0 ? "i2" : "u2");
            return n;
        case TYPE_BASIC_INT:
        case TYPE_BASIC_INT32:
        case TYPE_BASIC_LONG:
            n += strappend(buf, len, pos + n, type_basic_get_sign(type) <= 0 ? "i4" : "u4");
            return n;
        case TYPE_BASIC_INT64:
            n += strappend(buf, len, pos + n, type_basic_get_sign(type) <= 0 ? "i8" : "u8");
            return n;
        case TYPE_BASIC_INT8:
            assert(type_basic_get_sign(type) > 0); /* signature string for signed char isn't specified */
            n += strappend(buf, len, pos + n, "u1");
            return n;
        case TYPE_BASIC_FLOAT:
            n += strappend(buf, len, pos + n, "f4");
            return n;
        case TYPE_BASIC_DOUBLE:
            n += strappend(buf, len, pos + n, "f8");
            return n;
        case TYPE_BASIC_BYTE:
            n += strappend(buf, len, pos + n, "u1");
            return n;
        case TYPE_BASIC_INT3264:
        case TYPE_BASIC_CHAR:
        case TYPE_BASIC_HYPER:
        case TYPE_BASIC_WCHAR:
        case TYPE_BASIC_ERROR_STATUS_T:
        case TYPE_BASIC_HANDLE:
            error_at( &type->where, "unimplemented type signature for basic type %d.\n",
                      type_basic_get_type( type ) );
            break;
        }
    case TYPE_ENUM:
        n += strappend(buf, len, pos + n, "enum(");
        n += append_namespaces(buf, len, pos + n, type->namespace, "", ".", type->name, NULL);
        if (is_attr(type->attrs, ATTR_FLAGS)) n += strappend(buf, len, pos + n, ";u4");
        else n += strappend(buf, len, pos + n, ";i4");
        n += strappend(buf, len, pos + n, ")");
        return n;
    case TYPE_ARRAY:
    case TYPE_ENCAPSULATED_UNION:
    case TYPE_UNION:
    case TYPE_COCLASS:
    case TYPE_VOID:
    case TYPE_FUNCTION:
    case TYPE_BITFIELD:
    case TYPE_MODULE:
    case TYPE_APICONTRACT:
        error_at( &type->where, "unimplemented type signature for type %s of type %d.\n",
                  type->name, type->type_type );
        break;
    case TYPE_PARAMETERIZED_TYPE:
    case TYPE_PARAMETER:
        assert(0); /* should not be there */
        break;
    }

    return n;
}

char *format_namespace(struct namespace *namespace, const char *prefix, const char *separator, const char *suffix, const char *abi_prefix)
{
    size_t len = 0;
    char *buf = NULL;
    append_namespaces(&buf, &len, 0, namespace, prefix, separator, suffix, abi_prefix);
    return buf;
}

char *format_parameterized_type_name(type_t *type, typeref_list_t *params)
{
    size_t len = 0, pos = 0;
    char *buf = NULL;
    typeref_t *ref;

    pos += strappend(&buf, &len, pos, "%s<", type->name);
    if (params) LIST_FOR_EACH_ENTRY(ref, params, typeref_t, entry)
    {
        type = type_pointer_get_root_type(ref->type);
        pos += strappend(&buf, &len, pos, "%s", type->qualified_name);
        pos += append_pointer_stars(&buf, &len, pos, ref->type);
        if (list_next(params, &ref->entry)) pos += strappend(&buf, &len, pos, ",");
    }
    pos += strappend(&buf, &len, pos, " >");

    return buf;
}

static char const *parameterized_type_shorthands[][2] = {
    {"Windows__CFoundation__CCollections__C", "__F"},
    {"Windows_CFoundation_CCollections_C", "__F"},
    {"Windows__CFoundation__C", "__F"},
    {"Windows_CFoundation_C", "__F"},
};

static char *format_parameterized_type_c_name(type_t *type, typeref_list_t *params, const char *prefix, const char *separator)
{
    const char *tmp, *ns_prefix = "__x_", *abi_prefix = NULL;
    size_t len = 0, pos = 0;
    char *buf = NULL;
    int i, count = params ? list_count(params) : 0;
    typeref_t *ref;

    if (!strcmp(separator, "__C")) ns_prefix = "_C";
    else if (use_abi_namespace) abi_prefix = "ABI";

    pos += append_namespaces(&buf, &len, pos, type->namespace, ns_prefix, separator, "", abi_prefix);
    pos += strappend(&buf, &len, pos, "%s%s_%d", prefix, type->name, count);
    if (params) LIST_FOR_EACH_ENTRY(ref, params, typeref_t, entry)
    {
        type = type_pointer_get_root_type(ref->type);
        if ((tmp = type->param_name)) pos += strappend(&buf, &len, pos, "_%s", tmp);
        else pos += append_namespaces(&buf, &len, pos, type->namespace, "_", "__C", type->name, NULL);
    }

    for (i = 0; i < ARRAY_SIZE(parameterized_type_shorthands); ++i)
    {
        if ((tmp = strstr(buf, parameterized_type_shorthands[i][0])) &&
            (tmp - buf) == strlen(ns_prefix) + (abi_prefix ? 5 : 0))
        {
           tmp += strlen(parameterized_type_shorthands[i][0]);
           strcpy(buf, parameterized_type_shorthands[i][1]);
           memmove(buf + 3, tmp, len - (tmp - buf));
        }
    }

    return buf;
}

static char *format_parameterized_type_signature(type_t *type, typeref_list_t *params)
{
    size_t len = 0, pos = 0;
    char *buf = NULL;
    typeref_t *ref;
    const struct uuid *uuid;

    if (!(uuid = get_attrp( type->attrs, ATTR_UUID )))
        error_at( &type->where, "cannot compute type signature, no uuid found for type %s.\n", type->name );

    pos += strappend(&buf, &len, pos, "pinterface({%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                     uuid->Data1, uuid->Data2, uuid->Data3,
                     uuid->Data4[0], uuid->Data4[1], uuid->Data4[2], uuid->Data4[3],
                     uuid->Data4[4], uuid->Data4[5], uuid->Data4[6], uuid->Data4[7]);
    if (params) LIST_FOR_EACH_ENTRY(ref, params, typeref_t, entry)
    {
        pos += strappend(&buf, &len, pos, ";");
        pos += append_type_signature(&buf, &len, pos, ref->type);
    }
    pos += strappend(&buf, &len, pos, ")");

    return buf;
}

static char *format_parameterized_type_short_name(type_t *type, typeref_list_t *params, const char *prefix)
{
    size_t len = 0, pos = 0;
    char *buf = NULL;
    typeref_t *ref;

    pos += strappend(&buf, &len, pos, "%s%s", prefix, type->name);
    if (params) LIST_FOR_EACH_ENTRY(ref, params, typeref_t, entry)
    {
        type = type_pointer_get_root_type(ref->type);
        if (type->short_name) pos += strappend(&buf, &len, pos, "_%s", type->short_name);
        else pos += strappend(&buf, &len, pos, "_%s", type->name);
    }

    return buf;
}

static char *format_parameterized_type_impl_name(type_t *type, typeref_list_t *params, const char *prefix)
{
    size_t len = 0, pos = 0;
    char *buf = NULL;
    typeref_t *ref;
    type_t *iface;

    pos += strappend(&buf, &len, pos, "%s%s_impl<", prefix, type->name);
    if (params) LIST_FOR_EACH_ENTRY(ref, params, typeref_t, entry)
    {
        type = type_pointer_get_root_type(ref->type);
        if (type->type_type == TYPE_RUNTIMECLASS)
        {
            pos += strappend(&buf, &len, pos, "ABI::Windows::Foundation::Internal::AggregateType<%s", type->qualified_name);
            pos += append_pointer_stars(&buf, &len, pos, ref->type);
            iface = type_runtimeclass_get_default_iface(type, TRUE);
            pos += strappend(&buf, &len, pos, ", %s", iface->qualified_name);
            pos += append_pointer_stars(&buf, &len, pos, ref->type);
            pos += strappend(&buf, &len, pos, " >");
        }
        else
        {
            pos += strappend(&buf, &len, pos, "%s", type->qualified_name);
            pos += append_pointer_stars(&buf, &len, pos, ref->type);
        }
        if (list_next(params, &ref->entry)) pos += strappend(&buf, &len, pos, ", ");
    }
    pos += strappend(&buf, &len, pos, " >");

    return buf;
}

type_t *type_new_function(var_list_t *args)
{
    var_t *arg;
    type_t *t;
    unsigned int i = 0;

    if (args)
    {
        arg = LIST_ENTRY(list_head(args), var_t, entry);
        if (list_count(args) == 1 && !arg->name && arg->declspec.type && type_get_type(arg->declspec.type) == TYPE_VOID)
        {
            list_remove(&arg->entry);
            free(arg);
            free(args);
            args = NULL;
        }
    }
    if (args) LIST_FOR_EACH_ENTRY(arg, args, var_t, entry)
    {
        if (arg->declspec.type && type_get_type(arg->declspec.type) == TYPE_VOID)
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
    t->details.function->retval = make_var(xstrdup("_RetVal"));
    return t;
}

type_t *type_new_pointer(type_t *ref)
{
    type_t *t = make_type(TYPE_POINTER);
    t->details.pointer.ref.type = ref;
    return t;
}

type_t *type_new_alias(const decl_spec_t *t, const char *name)
{
    type_t *a = make_type(TYPE_ALIAS);

    a->name = xstrdup(name);
    a->attrs = NULL;
    a->details.alias.aliasee = *t;
    init_location( &a->where, NULL, NULL );

    return a;
}

type_t *type_new_array(const char *name, const decl_spec_t *element, int declptr,
                       unsigned int dim, expr_t *size_is, expr_t *length_is)
{
    type_t *t = make_type(TYPE_ARRAY);
    if (name) t->name = xstrdup(name);
    t->details.array.declptr = declptr;
    t->details.array.length_is = length_is;
    if (size_is)
        t->details.array.size_is = size_is;
    else
        t->details.array.dim = dim;
    if (element)
        t->details.array.elem = *element;
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

static void define_type(type_t *type, const struct location *where)
{
    if (type->defined)
        error_loc("type %s already defined at %s:%d\n", type->name, type->where.input_name, type->where.first_line );

    type->defined = TRUE;
    type->defined_in_import = parse_only;
    type->where = *where;
}

type_t *type_new_enum(const char *name, struct namespace *namespace,
        int defined, var_list_t *enums, const struct location *where)
{
    type_t *t = NULL;

    if (name)
        t = find_type(name, namespace,tsENUM);

    if (!t)
    {
        t = make_type(TYPE_ENUM);
        t->name = name;
        t->namespace = namespace;
        if (name)
            reg_type(t, name, namespace, tsENUM);
    }

    if (defined)
    {
        t->details.enumeration = xmalloc(sizeof(*t->details.enumeration));
        t->details.enumeration->enums = enums;
        define_type(t, where);
    }

    return t;
}

type_t *type_new_struct(char *name, struct namespace *namespace,
        int defined, var_list_t *fields, const struct location *where)
{
    type_t *t = NULL;

    if (name)
        t = find_type(name, namespace, tsSTRUCT);

    if (!t)
    {
        t = make_type(TYPE_STRUCT);
        t->name = name;
        t->namespace = namespace;
        if (name)
            reg_type(t, name, namespace, tsSTRUCT);
    }

    if (defined)
    {
        t->details.structure = xmalloc(sizeof(*t->details.structure));
        t->details.structure->fields = fields;
        define_type(t, where);
    }

    return t;
}

type_t *type_new_nonencapsulated_union(const char *name, struct namespace *namespace,
        int defined, var_list_t *fields, const struct location *where)
{
    type_t *t = NULL;

    if (name)
        t = find_type(name, namespace, tsUNION);

    if (!t)
    {
        t = make_type(TYPE_UNION);
        t->name = name;
        t->namespace = namespace;
        if (name)
            reg_type(t, name, namespace, tsUNION);
    }

    if (!t->defined && defined)
    {
        t->details.structure = xmalloc(sizeof(*t->details.structure));
        t->details.structure->fields = fields;
        define_type(t, where);
    }

    return t;
}

type_t *type_new_encapsulated_union(char *name, var_t *switch_field,
        var_t *union_field, var_list_t *cases, const struct location *where)
{
    type_t *t = NULL;

    if (name)
        t = find_type(name, NULL, tsUNION);

    if (!t)
    {
        t = make_type(TYPE_ENCAPSULATED_UNION);
        t->name = name;
        if (name)
            reg_type(t, name, NULL, tsUNION);
    }
    t->type_type = TYPE_ENCAPSULATED_UNION;

    if (!union_field)
        union_field = make_var(xstrdup("tagged_union"));
    union_field->declspec.type = type_new_nonencapsulated_union(gen_name(), NULL, TRUE, cases, where);

    t->details.structure = xmalloc(sizeof(*t->details.structure));
    t->details.structure->fields = append_var(NULL, switch_field);
    t->details.structure->fields = append_var(t->details.structure->fields, union_field);
    define_type(t, where);

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
        case TYPE_BASIC_LONG:
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

static unsigned int compute_method_indexes(type_t *iface)
{
    unsigned int idx;
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
            func->func_idx = idx++;
    }

    return idx;
}

type_t *type_interface_declare(char *name, struct namespace *namespace)
{
    type_t *type = get_type(TYPE_INTERFACE, name, namespace, 0);
    if (type_get_type_detect_alias( type ) != TYPE_INTERFACE)
        error_loc( "interface %s previously not declared an interface at %s:%d\n", type->name,
                   type->where.input_name, type->where.first_line );
    return type;
}

type_t *type_interface_define(type_t *iface, attr_list_t *attrs, type_t *inherit,
        statement_list_t *stmts, typeref_list_t *requires, const struct location *where)
{
    if (iface == inherit)
        error_loc("interface %s can't inherit from itself\n",
                  iface->name);
    iface->attrs = check_interface_attrs(iface->name, attrs);
    iface->details.iface = xmalloc(sizeof(*iface->details.iface));
    iface->details.iface->disp_props = NULL;
    iface->details.iface->disp_methods = NULL;
    iface->details.iface->stmts = stmts;
    iface->details.iface->inherit = inherit;
    iface->details.iface->disp_inherit = NULL;
    iface->details.iface->async_iface = NULL;
    iface->details.iface->requires = requires;
    define_type(iface, where);
    compute_method_indexes(iface);
    return iface;
}

type_t *type_dispinterface_declare(char *name)
{
    type_t *type = get_type(TYPE_INTERFACE, name, NULL, 0);
    if (type_get_type_detect_alias( type ) != TYPE_INTERFACE)
        error_loc( "dispinterface %s previously not declared a dispinterface at %s:%d\n",
                   type->name, type->where.input_name, type->where.first_line );
    return type;
}

type_t *type_dispinterface_define(type_t *iface, attr_list_t *attrs,
        var_list_t *props, var_list_t *methods, const struct location *where)
{
    iface->attrs = check_dispiface_attrs(iface->name, attrs);
    iface->details.iface = xmalloc(sizeof(*iface->details.iface));
    iface->details.iface->disp_props = props;
    iface->details.iface->disp_methods = methods;
    iface->details.iface->stmts = NULL;
    iface->details.iface->inherit = find_type("IDispatch", NULL, 0);
    if (!iface->details.iface->inherit) error_loc("IDispatch is undefined\n");
    iface->details.iface->disp_inherit = NULL;
    iface->details.iface->async_iface = NULL;
    iface->details.iface->requires = NULL;
    define_type(iface, where);
    compute_method_indexes(iface);
    return iface;
}

type_t *type_dispinterface_define_from_iface(type_t *dispiface,
        attr_list_t *attrs, type_t *iface, const struct location *where)
{
    dispiface->attrs = check_dispiface_attrs(dispiface->name, attrs);
    dispiface->details.iface = xmalloc(sizeof(*dispiface->details.iface));
    dispiface->details.iface->disp_props = NULL;
    dispiface->details.iface->disp_methods = NULL;
    dispiface->details.iface->stmts = NULL;
    dispiface->details.iface->inherit = find_type("IDispatch", NULL, 0);
    if (!dispiface->details.iface->inherit) error_loc("IDispatch is undefined\n");
    dispiface->details.iface->disp_inherit = iface;
    dispiface->details.iface->async_iface = NULL;
    dispiface->details.iface->requires = NULL;
    define_type(dispiface, where);
    compute_method_indexes(dispiface);
    return dispiface;
}

type_t *type_module_declare(char *name)
{
    type_t *type = get_type(TYPE_MODULE, name, NULL, 0);
    if (type_get_type_detect_alias( type ) != TYPE_MODULE)
        error_loc( "module %s previously not declared a module at %s:%d\n", type->name,
                   type->where.input_name, type->where.first_line );
    return type;
}

type_t *type_module_define(type_t* module, attr_list_t *attrs,
        statement_list_t *stmts, const struct location *where)
{
    module->attrs = check_module_attrs(module->name, attrs);
    module->details.module = xmalloc(sizeof(*module->details.module));
    module->details.module->stmts = stmts;
    define_type(module, where);
    return module;
}

type_t *type_coclass_declare(char *name)
{
    type_t *type = get_type(TYPE_COCLASS, name, NULL, 0);
    if (type_get_type_detect_alias( type ) != TYPE_COCLASS)
        error_loc( "coclass %s previously not declared a coclass at %s:%d\n", type->name,
                   type->where.input_name, type->where.first_line );
    return type;
}

type_t *type_coclass_define(type_t *coclass, attr_list_t *attrs,
        typeref_list_t *ifaces, const struct location *where)
{
    coclass->attrs = check_coclass_attrs(coclass->name, attrs);
    coclass->details.coclass.ifaces = ifaces;
    define_type(coclass, where);
    return coclass;
}

type_t *type_runtimeclass_declare(char *name, struct namespace *namespace)
{
    type_t *type = get_type(TYPE_RUNTIMECLASS, name, namespace, 0);
    if (type_get_type_detect_alias( type ) != TYPE_RUNTIMECLASS)
        error_loc( "runtimeclass %s previously not declared a runtimeclass at %s:%d\n", type->name,
                   type->where.input_name, type->where.first_line );
    return type;
}

type_t *type_runtimeclass_define(type_t *runtimeclass, attr_list_t *attrs,
        typeref_list_t *ifaces, const struct location *where)
{
    typeref_t *ref, *required, *tmp;
    typeref_list_t *requires;

    runtimeclass->attrs = check_runtimeclass_attrs(runtimeclass->name, attrs);
    runtimeclass->details.runtimeclass.ifaces = ifaces;
    define_type(runtimeclass, where);
    if (!type_runtimeclass_get_default_iface(runtimeclass, FALSE) &&
        !get_attrp(runtimeclass->attrs, ATTR_STATIC))
        error_loc("runtimeclass %s must have a default interface or static factory\n", runtimeclass->name);

    if (ifaces) LIST_FOR_EACH_ENTRY(ref, ifaces, typeref_t, entry)
    {
        /* FIXME: this should probably not be allowed, here or in coclass, */
        /* but for now there's too many places in Wine IDL where it is to */
        /* even print a warning. */
        if (!(ref->type->defined)) continue;
        if (!(requires = type_iface_get_requires(ref->type))) continue;
        LIST_FOR_EACH_ENTRY(required, requires, typeref_t, entry)
        {
            int found = 0;

            LIST_FOR_EACH_ENTRY(tmp, ifaces, typeref_t, entry)
                if ((found = type_is_equal(tmp->type, required->type))) break;

            if (!found)
                error_loc("interface '%s' also requires interface '%s', "
                          "but runtimeclass '%s' does not implement it.\n",
                          ref->type->name, required->type->name, runtimeclass->name);
        }
    }

    return runtimeclass;
}

type_t *type_apicontract_declare(char *name, struct namespace *namespace)
{
    type_t *type = get_type(TYPE_APICONTRACT, name, namespace, 0);
    if (type_get_type_detect_alias( type ) != TYPE_APICONTRACT)
        error_loc( "apicontract %s previously not declared a apicontract at %s:%d\n", type->name,
                   type->where.input_name, type->where.first_line );
    return type;
}

type_t *type_apicontract_define(type_t *apicontract, attr_list_t *attrs, const struct location *where)
{
    apicontract->attrs = check_apicontract_attrs(apicontract->name, attrs);
    define_type(apicontract, where);
    return apicontract;
}

static void compute_delegate_iface_names(type_t *delegate, type_t *type, typeref_list_t *params)
{
    type_t *iface = delegate->details.delegate.iface;
    iface->namespace = delegate->namespace;
    iface->name = strmake("I%s", delegate->name);
    if (type) iface->c_name = format_parameterized_type_c_name(type, params, "I", "_C");
    else iface->c_name = format_namespace(delegate->namespace, "__x_", "_C", iface->name, use_abi_namespace ? "ABI" : NULL);
    if (type) iface->param_name = format_parameterized_type_c_name(type, params, "I", "__C");
    else iface->param_name = format_namespace(delegate->namespace, "_", "__C", iface->name, NULL);
    iface->qualified_name = format_namespace(delegate->namespace, "", "::", iface->name, use_abi_namespace ? "ABI" : NULL);
}

type_t *type_delegate_declare(char *name, struct namespace *namespace)
{
    type_t *type = get_type(TYPE_DELEGATE, name, namespace, 0);
    if (type_get_type_detect_alias( type ) != TYPE_DELEGATE)
        error_loc( "delegate %s previously not declared a delegate at %s:%d\n", type->name,
                   type->where.input_name, type->where.first_line );
    return type;
}

type_t *type_delegate_define(type_t *delegate, attr_list_t *attrs,
        statement_list_t *stmts, const struct location *where)
{
    type_t *iface;

    delegate->attrs = check_interface_attrs(delegate->name, attrs);

    iface = make_type(TYPE_INTERFACE);
    iface->attrs = delegate->attrs;
    iface->details.iface = xmalloc(sizeof(*iface->details.iface));
    iface->details.iface->disp_props = NULL;
    iface->details.iface->disp_methods = NULL;
    iface->details.iface->stmts = stmts;
    iface->details.iface->inherit = find_type("IUnknown", NULL, 0);
    if (!iface->details.iface->inherit) error_loc("IUnknown is undefined\n");
    iface->details.iface->disp_inherit = NULL;
    iface->details.iface->async_iface = NULL;
    iface->details.iface->requires = NULL;
    define_type(iface, where);
    iface->defined = TRUE;
    compute_method_indexes(iface);

    delegate->details.delegate.iface = iface;
    define_type(delegate, where);
    compute_delegate_iface_names(delegate, NULL, NULL);

    return delegate;
}

type_t *type_parameterized_interface_declare(char *name, struct namespace *namespace, typeref_list_t *params)
{
    type_t *type = get_type(TYPE_PARAMETERIZED_TYPE, name, namespace, 0);
    if (type_get_type_detect_alias( type ) != TYPE_PARAMETERIZED_TYPE)
        error_loc( "pinterface %s previously not declared a pinterface at %s:%d\n", type->name,
                   type->where.input_name, type->where.first_line );
    type->details.parameterized.type = make_type(TYPE_INTERFACE);
    type->details.parameterized.params = params;
    return type;
}

type_t *type_parameterized_interface_define(type_t *type, attr_list_t *attrs, type_t *inherit,
        statement_list_t *stmts, typeref_list_t *requires, const struct location *where)
{
    type_t *iface;

    /* The parameterized type UUID is actually a PIID that is then used as a seed to generate
     * a new type GUID with the rules described in:
     *   https://docs.microsoft.com/en-us/uwp/winrt-cref/winrt-type-system#parameterized-types
     * TODO: store type signatures for generated interfaces, and generate their GUIDs
     */
    type->attrs = check_interface_attrs(type->name, attrs);

    iface = type->details.parameterized.type;
    iface->details.iface = xmalloc(sizeof(*iface->details.iface));
    iface->details.iface->disp_props = NULL;
    iface->details.iface->disp_methods = NULL;
    iface->details.iface->stmts = stmts;
    iface->details.iface->inherit = inherit;
    iface->details.iface->disp_inherit = NULL;
    iface->details.iface->async_iface = NULL;
    iface->details.iface->requires = requires;

    iface->name = type->name;

    define_type(type, where);
    return type;
}

type_t *type_parameterized_delegate_declare(char *name, struct namespace *namespace, typeref_list_t *params)
{
    type_t *type = get_type(TYPE_PARAMETERIZED_TYPE, name, namespace, 0);
    if (type_get_type_detect_alias( type ) != TYPE_PARAMETERIZED_TYPE)
        error_loc( "pdelegate %s previously not declared a pdelegate at %s:%d\n", type->name,
                   type->where.input_name, type->where.first_line );
    type->details.parameterized.type = make_type(TYPE_DELEGATE);
    type->details.parameterized.params = params;
    return type;
}

type_t *type_parameterized_delegate_define(type_t *type, attr_list_t *attrs,
        statement_list_t *stmts, const struct location *where)
{
    type_t *iface, *delegate;

    type->attrs = check_interface_attrs(type->name, attrs);

    delegate = type->details.parameterized.type;
    delegate->attrs = type->attrs;
    delegate->details.delegate.iface = make_type(TYPE_INTERFACE);

    iface = delegate->details.delegate.iface;
    iface->details.iface = xmalloc(sizeof(*iface->details.iface));
    iface->details.iface->disp_props = NULL;
    iface->details.iface->disp_methods = NULL;
    iface->details.iface->stmts = stmts;
    iface->details.iface->inherit = find_type("IUnknown", NULL, 0);
    if (!iface->details.iface->inherit) error_loc("IUnknown is undefined\n");
    iface->details.iface->disp_inherit = NULL;
    iface->details.iface->async_iface = NULL;
    iface->details.iface->requires = NULL;

    delegate->name = type->name;
    compute_delegate_iface_names(delegate, type, type->details.parameterized.params);

    define_type(type, where);
    return type;
}

type_t *type_parameterized_type_specialize_partial(type_t *type, typeref_list_t *params)
{
    type_t *new_type = duptype(type, 0);
    new_type->details.parameterized.type = type;
    new_type->details.parameterized.params = params;
    return new_type;
}

static type_t *replace_type_parameters_in_type(type_t *type, typeref_list_t *orig, typeref_list_t *repl);

static typeref_list_t *replace_type_parameters_in_type_list(typeref_list_t *list, typeref_list_t *orig, typeref_list_t *repl)
{
    typeref_list_t *new_list = NULL;
    typeref_t *ref;

    if (!list) return list;

    LIST_FOR_EACH_ENTRY(ref, list, typeref_t, entry)
    {
        type_t *new_type = replace_type_parameters_in_type(ref->type, orig, repl);
        new_list = append_typeref(new_list, make_typeref(new_type));
    }

    return new_list;
}

static var_t *replace_type_parameters_in_var(var_t *var, typeref_list_t *orig, typeref_list_t *repl)
{
    var_t *new_var = xmalloc(sizeof(*new_var));
    *new_var = *var;
    list_init(&new_var->entry);
    new_var->declspec.type = replace_type_parameters_in_type(var->declspec.type, orig, repl);
    return new_var;
}

static var_list_t *replace_type_parameters_in_var_list(var_list_t *var_list, typeref_list_t *orig, typeref_list_t *repl)
{
    var_list_t *new_var_list;
    var_t *var, *new_var;

    if (!var_list) return var_list;

    new_var_list = xmalloc(sizeof(*new_var_list));
    list_init(new_var_list);

    LIST_FOR_EACH_ENTRY(var, var_list, var_t, entry)
    {
        new_var = replace_type_parameters_in_var(var, orig, repl);
        list_add_tail(new_var_list, &new_var->entry);
    }

    return new_var_list;
}

static statement_t *replace_type_parameters_in_statement( statement_t *stmt, typeref_list_t *orig,
                                                          typeref_list_t *repl, struct location *where )
{
    statement_t *new_stmt = xmalloc(sizeof(*new_stmt));
    *new_stmt = *stmt;
    list_init(&new_stmt->entry);

    switch (stmt->type)
    {
    case STMT_DECLARATION:
        new_stmt->u.var = replace_type_parameters_in_var(stmt->u.var, orig, repl);
        break;
    case STMT_TYPE:
    case STMT_TYPEREF:
        new_stmt->u.type = replace_type_parameters_in_type(stmt->u.type, orig, repl);
        break;
    case STMT_TYPEDEF:
        new_stmt->u.type_list = replace_type_parameters_in_type_list(stmt->u.type_list, orig, repl);
        break;
    case STMT_MODULE:
    case STMT_LIBRARY:
    case STMT_IMPORT:
    case STMT_IMPORTLIB:
    case STMT_PRAGMA:
    case STMT_CPPQUOTE:
        error_at( where, "unimplemented parameterized type replacement for statement type %d.\n", stmt->type );
        break;
    }

    return new_stmt;
}

static statement_list_t *replace_type_parameters_in_statement_list( statement_list_t *stmt_list, typeref_list_t *orig,
                                                                    typeref_list_t *repl, struct location *where )
{
    statement_list_t *new_stmt_list;
    statement_t *stmt, *new_stmt;

    if (!stmt_list) return stmt_list;

    new_stmt_list = xmalloc(sizeof(*new_stmt_list));
    list_init(new_stmt_list);

    LIST_FOR_EACH_ENTRY(stmt, stmt_list, statement_t, entry)
    {
        new_stmt = replace_type_parameters_in_statement( stmt, orig, repl, where );
        list_add_tail(new_stmt_list, &new_stmt->entry);
    }

    return new_stmt_list;
}

static type_t *replace_type_parameters_in_type(type_t *type, typeref_list_t *orig, typeref_list_t *repl)
{
    struct list *o, *r;
    type_t *t;

    if (!type) return type;
    switch (type->type_type)
    {
    case TYPE_VOID:
    case TYPE_BASIC:
    case TYPE_ENUM:
    case TYPE_BITFIELD:
    case TYPE_INTERFACE:
    case TYPE_RUNTIMECLASS:
    case TYPE_DELEGATE:
    case TYPE_STRUCT:
    case TYPE_ENCAPSULATED_UNION:
    case TYPE_UNION:
        return type;
    case TYPE_PARAMETER:
        if (!orig || !repl) return NULL;
        for (o = list_head(orig), r = list_head(repl); o && r;
             o = list_next(orig, o), r = list_next(repl, r))
            if (type == LIST_ENTRY(o, typeref_t, entry)->type)
                return LIST_ENTRY(r, typeref_t, entry)->type;
        return type;
    case TYPE_POINTER:
        t = replace_type_parameters_in_type(type->details.pointer.ref.type, orig, repl);
        if (t == type->details.pointer.ref.type) return type;
        type = duptype(type, 0);
        type->details.pointer.ref.type = t;
        return type;
    case TYPE_ALIAS:
        t = replace_type_parameters_in_type(type->details.alias.aliasee.type, orig, repl);
        if (t == type->details.alias.aliasee.type) return type;
        type = duptype(type, 0);
        type->details.alias.aliasee.type = t;
        return type;
    case TYPE_ARRAY:
        t = replace_type_parameters_in_type(type->details.array.elem.type, orig, repl);
        if (t == t->details.array.elem.type) return type;
        type = duptype(type, 0);
        t->details.array.elem.type = t;
        return type;
    case TYPE_FUNCTION:
        t = duptype(type, 0);
        t->details.function = xmalloc(sizeof(*t->details.function));
        t->details.function->args = replace_type_parameters_in_var_list(type->details.function->args, orig, repl);
        t->details.function->retval = replace_type_parameters_in_var(type->details.function->retval, orig, repl);
        return t;
    case TYPE_PARAMETERIZED_TYPE:
        t = type->details.parameterized.type;
        if (t->type_type != TYPE_PARAMETERIZED_TYPE) return find_parameterized_type(type, repl);
        repl = replace_type_parameters_in_type_list(type->details.parameterized.params, orig, repl);
        return replace_type_parameters_in_type(t, t->details.parameterized.params, repl);
    case TYPE_MODULE:
    case TYPE_COCLASS:
    case TYPE_APICONTRACT:
        error_at( &type->where, "unimplemented parameterized type replacement for type %s of type %d.\n",
                  type->name, type->type_type );
        break;
    }

    return type;
}

static void type_parameterized_interface_specialize(type_t *tmpl, type_t *iface, typeref_list_t *orig, typeref_list_t *repl)
{
    iface->details.iface = xmalloc(sizeof(*iface->details.iface));
    iface->details.iface->disp_methods = NULL;
    iface->details.iface->disp_props = NULL;
    iface->details.iface->stmts = replace_type_parameters_in_statement_list( tmpl->details.iface->stmts,
                                                                             orig, repl, &tmpl->where );
    iface->details.iface->inherit = replace_type_parameters_in_type(tmpl->details.iface->inherit, orig, repl);
    iface->details.iface->disp_inherit = NULL;
    iface->details.iface->async_iface = NULL;
    iface->details.iface->requires = NULL;
}

static void type_parameterized_delegate_specialize(type_t *tmpl, type_t *delegate, typeref_list_t *orig, typeref_list_t *repl)
{
    type_parameterized_interface_specialize(tmpl->details.delegate.iface, delegate->details.delegate.iface, orig, repl);
}

type_t *type_parameterized_type_specialize_declare(type_t *type, typeref_list_t *params)
{
    type_t *tmpl = type->details.parameterized.type;
    type_t *new_type = duptype(tmpl, 0);

    new_type->namespace = type->namespace;
    new_type->name = format_parameterized_type_name(type, params);
    reg_type(new_type, new_type->name, new_type->namespace, 0);
    new_type->c_name = format_parameterized_type_c_name(type, params, "", "_C");
    new_type->short_name = format_parameterized_type_short_name(type, params, "");
    new_type->param_name = format_parameterized_type_c_name(type, params, "", "__C");

    if (new_type->type_type == TYPE_DELEGATE)
    {
        new_type->details.delegate.iface = duptype(tmpl->details.delegate.iface, 0);
        compute_delegate_iface_names(new_type, type, params);
        new_type->details.delegate.iface->short_name = format_parameterized_type_short_name(type, params, "I");
    }

    return new_type;
}

static void compute_interface_signature_uuid(type_t *iface)
{
    static const char winrt_pinterface_namespace[] = {0x11,0xf4,0x7a,0xd5,0x7b,0x73,0x42,0xc0,0xab,0xae,0x87,0x8b,0x1e,0x16,0xad,0xee};
    static const int version = 5;
    struct sha1_context ctx;
    unsigned char hash[20];
    struct uuid *uuid;

    if (!(uuid = get_attrp(iface->attrs, ATTR_UUID)))
    {
        uuid = xmalloc(sizeof(*uuid));
        iface->attrs = append_attr( iface->attrs, attr_ptr( iface->where, ATTR_UUID, uuid ) );
    }

    sha1_init(&ctx);
    sha1_update(&ctx, winrt_pinterface_namespace, sizeof(winrt_pinterface_namespace));
    sha1_update(&ctx, iface->signature, strlen(iface->signature));
    sha1_finalize(&ctx, (unsigned int *)hash);

    /* https://tools.ietf.org/html/rfc4122:

       * Set the four most significant bits (bits 12 through 15) of the
         time_hi_and_version field to the appropriate 4-bit version number
         from Section 4.1.3.

       * Set the two most significant bits (bits 6 and 7) of the
         clock_seq_hi_and_reserved to zero and one, respectively.
    */

    hash[6] = ((hash[6] & 0x0f) | (version << 4));
    hash[8] = ((hash[8] & 0x3f) | 0x80);

    uuid->Data1 = ((unsigned int)hash[0] << 24) | ((unsigned int)hash[1] << 16) | ((unsigned int)hash[2] << 8) | hash[3];
    uuid->Data2 = ((unsigned short)hash[4] << 8) | hash[5];
    uuid->Data3 = ((unsigned short)hash[6] << 8) | hash[7];
    memcpy(&uuid->Data4, hash + 8, sizeof(*uuid) - offsetof(struct uuid, Data4));
}

type_t *type_parameterized_type_specialize_define(type_t *type)
{
    type_t *tmpl = type->details.parameterized.type;
    typeref_list_t *orig = tmpl->details.parameterized.params;
    typeref_list_t *repl = type->details.parameterized.params;
    type_t *iface = find_parameterized_type(tmpl, repl);

    if (type_get_type_detect_alias(type) != TYPE_PARAMETERIZED_TYPE ||
        type_get_type_detect_alias(tmpl) != TYPE_PARAMETERIZED_TYPE)
        error_loc( "cannot define non-parameterized type %s, declared at %s:%d\n", type->name,
                   type->where.input_name, type->where.first_line );

    if (type_get_type_detect_alias(tmpl->details.parameterized.type) == TYPE_INTERFACE &&
        type_get_type_detect_alias(iface) == TYPE_INTERFACE)
        type_parameterized_interface_specialize(tmpl->details.parameterized.type, iface, orig, repl);
    else if (type_get_type_detect_alias(tmpl->details.parameterized.type) == TYPE_DELEGATE &&
             type_get_type_detect_alias(iface) == TYPE_DELEGATE)
        type_parameterized_delegate_specialize(tmpl->details.parameterized.type, iface, orig, repl);
    else
        error_loc("pinterface/pdelegate %s previously not declared a pinterface/pdelegate at %s:%d\n",
                  iface->name, iface->where.input_name, iface->where.first_line);

    iface->impl_name = format_parameterized_type_impl_name(type, repl, "");
    iface->signature = format_parameterized_type_signature(type, repl);
    iface->defined = TRUE;
    if (iface->type_type == TYPE_DELEGATE)
    {
        iface = iface->details.delegate.iface;
        iface->impl_name = format_parameterized_type_impl_name(type, repl, "I");
        iface->signature = format_parameterized_type_signature(type, repl);
        iface->defined = TRUE;
    }
    compute_interface_signature_uuid(iface);
    compute_method_indexes(iface);
    return iface;
}

int type_is_equal(const type_t *type1, const type_t *type2)
{
    if (type1 == type2)
        return TRUE;
    if (type_get_type_detect_alias(type1) != type_get_type_detect_alias(type2))
        return FALSE;
    if (type1->namespace != type2->namespace)
        return FALSE;

    if (type1->name && type2->name)
        return !strcmp(type1->name, type2->name);
    else if ((!type1->name && type2->name) || (type1->name && !type2->name))
        return FALSE;

    /* FIXME: do deep inspection of types to determine if they are equal */

    return FALSE;
}
