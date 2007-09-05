/*
 * Format String Generator for IDL Compiler
 *
 * Copyright 2005-2006 Eric Kohl
 * Copyright 2005-2006 Robert Shearman
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
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <limits.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "windef.h"
#include "wine/list.h"

#include "widl.h"
#include "typegen.h"

static const func_t *current_func;
static const type_t *current_structure;

static struct list expr_eval_routines = LIST_INIT(expr_eval_routines);
struct expr_eval_routine
{
    struct list entry;
    const type_t *structure;
    const expr_t *expr;
};

static size_t fields_memsize(const var_list_t *fields, unsigned int *align);
static size_t write_struct_tfs(FILE *file, type_t *type, const char *name, unsigned int *tfsoff);
static int write_embedded_types(FILE *file, const attr_list_t *attrs, type_t *type,
                                const char *name, int write_ptr, unsigned int *tfsoff);

const char *string_of_type(unsigned char type)
{
    switch (type)
    {
    case RPC_FC_BYTE: return "FC_BYTE";
    case RPC_FC_CHAR: return "FC_CHAR";
    case RPC_FC_SMALL: return "FC_SMALL";
    case RPC_FC_USMALL: return "FC_USMALL";
    case RPC_FC_WCHAR: return "FC_WCHAR";
    case RPC_FC_SHORT: return "FC_SHORT";
    case RPC_FC_USHORT: return "FC_USHORT";
    case RPC_FC_LONG: return "FC_LONG";
    case RPC_FC_ULONG: return "FC_ULONG";
    case RPC_FC_FLOAT: return "FC_FLOAT";
    case RPC_FC_HYPER: return "FC_HYPER";
    case RPC_FC_DOUBLE: return "FC_DOUBLE";
    case RPC_FC_ENUM16: return "FC_ENUM16";
    case RPC_FC_ENUM32: return "FC_ENUM32";
    case RPC_FC_IGNORE: return "FC_IGNORE";
    case RPC_FC_ERROR_STATUS_T: return "FC_ERROR_STATUS_T";
    case RPC_FC_RP: return "FC_RP";
    case RPC_FC_UP: return "FC_UP";
    case RPC_FC_OP: return "FC_OP";
    case RPC_FC_FP: return "FC_FP";
    case RPC_FC_ENCAPSULATED_UNION: return "FC_ENCAPSULATED_UNION";
    case RPC_FC_NON_ENCAPSULATED_UNION: return "FC_NON_ENCAPSULATED_UNION";
    case RPC_FC_STRUCT: return "FC_STRUCT";
    case RPC_FC_PSTRUCT: return "FC_PSTRUCT";
    case RPC_FC_CSTRUCT: return "FC_CSTRUCT";
    case RPC_FC_CPSTRUCT: return "FC_CPSTRUCT";
    case RPC_FC_CVSTRUCT: return "FC_CVSTRUCT";
    case RPC_FC_BOGUS_STRUCT: return "FC_BOGUS_STRUCT";
    case RPC_FC_SMFARRAY: return "FC_SMFARRAY";
    case RPC_FC_LGFARRAY: return "FC_LGFARRAY";
    case RPC_FC_SMVARRAY: return "FC_SMVARRAY";
    case RPC_FC_LGVARRAY: return "FC_LGVARRAY";
    case RPC_FC_CARRAY: return "FC_CARRAY";
    case RPC_FC_CVARRAY: return "FC_CVARRAY";
    case RPC_FC_BOGUS_ARRAY: return "FC_BOGUS_ARRAY";
    case RPC_FC_ALIGNM4: return "RPC_FC_ALIGNM4";
    case RPC_FC_ALIGNM8: return "RPC_FC_ALIGNM8";
    default:
        error("string_of_type: unknown type 0x%02x\n", type);
        return NULL;
    }
}

int is_struct(unsigned char type)
{
    switch (type)
    {
    case RPC_FC_STRUCT:
    case RPC_FC_PSTRUCT:
    case RPC_FC_CSTRUCT:
    case RPC_FC_CPSTRUCT:
    case RPC_FC_CVSTRUCT:
    case RPC_FC_BOGUS_STRUCT:
        return 1;
    default:
        return 0;
    }
}

static int is_non_complex_struct(const type_t *type)
{
    switch (type->type)
    {
    case RPC_FC_STRUCT:
    case RPC_FC_PSTRUCT:
    case RPC_FC_CSTRUCT:
    case RPC_FC_CPSTRUCT:
    case RPC_FC_CVSTRUCT:
        return 1;
    default:
        return 0;
    }
}

int is_union(unsigned char type)
{
    switch (type)
    {
    case RPC_FC_ENCAPSULATED_UNION:
    case RPC_FC_NON_ENCAPSULATED_UNION:
        return 1;
    default:
        return 0;
    }
}

static unsigned short user_type_offset(const char *name)
{
    user_type_t *ut;
    unsigned short off = 0;
    LIST_FOR_EACH_ENTRY(ut, &user_type_list, user_type_t, entry)
    {
        if (strcmp(name, ut->name) == 0)
            return off;
        ++off;
    }
    error("user_type_offset: couldn't find type (%s)\n", name);
    return 0;
}

static void update_tfsoff(type_t *type, unsigned int offset, FILE *file)
{
    type->typestring_offset = offset;
    if (file) type->tfswrite = FALSE;
}

static void guard_rec(type_t *type)
{
    /* types that contain references to themselves (like a linked list),
       need to be shielded from infinite recursion when writing embedded
       types  */
    if (type->typestring_offset)
        type->tfswrite = FALSE;
    else
        type->typestring_offset = 1;
}

static type_t *get_user_type(const type_t *t, const char **pname)
{
    for (;;)
    {
        type_t *ut = get_attrp(t->attrs, ATTR_WIREMARSHAL);
        if (ut)
        {
            if (pname)
                *pname = t->name;
            return ut;
        }

        if (t->kind == TKIND_ALIAS)
            t = t->orig;
        else
            return 0;
    }
}

static int is_user_type(const type_t *t)
{
    return get_user_type(t, NULL) != NULL;
}

static int is_embedded_complex(const type_t *type)
{
    unsigned char tc = type->type;
    return is_struct(tc) || is_union(tc) || is_array(type) || is_user_type(type)
        || (is_ptr(type) && type->ref->type == RPC_FC_IP);
}

static int compare_expr(const expr_t *a, const expr_t *b)
{
    int ret;

    if (a->type != b->type)
        return a->type - b->type;

    switch (a->type)
    {
        case EXPR_NUM:
        case EXPR_HEXNUM:
        case EXPR_TRUEFALSE:
            return a->u.lval - b->u.lval;
        case EXPR_DOUBLE:
            return a->u.dval - b->u.dval;
        case EXPR_IDENTIFIER:
            return strcmp(a->u.sval, b->u.sval);
        case EXPR_COND:
            ret = compare_expr(a->ref, b->ref);
            if (ret != 0)
                return ret;
            ret = compare_expr(a->u.ext, b->u.ext);
            if (ret != 0)
                return ret;
            return compare_expr(a->ext2, b->ext2);
        case EXPR_OR:
        case EXPR_AND:
        case EXPR_ADD:
        case EXPR_SUB:
        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_SHL:
        case EXPR_SHR:
            ret = compare_expr(a->ref, b->ref);
            if (ret != 0)
                return ret;
            return compare_expr(a->u.ext, b->u.ext);
        case EXPR_NOT:
        case EXPR_NEG:
        case EXPR_PPTR:
        case EXPR_CAST:
        case EXPR_SIZEOF:
            return compare_expr(a->ref, b->ref);
        case EXPR_VOID:
            return 0;
    }
    return -1;
}

#define WRITE_FCTYPE(file, fctype, typestring_offset) \
    do { \
        if (file) \
            fprintf(file, "/* %2u */\n", typestring_offset); \
        print_file((file), 2, "0x%02x,    /* " #fctype " */\n", RPC_##fctype); \
    } \
    while (0)

static void print_file(FILE *file, int indent, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    print(file, indent, format, va);
    va_end(va);
}

void print(FILE *file, int indent, const char *format, va_list va)
{
    if (file)
    {
        if (format[0] != '\n')
            while (0 < indent--)
                fprintf(file, "    ");
        vfprintf(file, format, va);
    }
}

void write_parameters_init(FILE *file, int indent, const func_t *func)
{
    const var_t *var;

    if (!func->args)
        return;

    LIST_FOR_EACH_ENTRY( var, func->args, const var_t, entry )
    {
        const type_t *t = var->type;
        const char *n = var->name;
        if (decl_indirect(t))
            print_file(file, indent, "MIDL_memset(&%s, 0, sizeof %s);\n", n, n);
        else if (is_ptr(t) || is_array(t))
            print_file(file, indent, "%s = 0;\n", n);
    }

    fprintf(file, "\n");
}

static void write_formatdesc(FILE *f, int indent, const char *str)
{
    print_file(f, indent, "typedef struct _MIDL_%s_FORMAT_STRING\n", str);
    print_file(f, indent, "{\n");
    print_file(f, indent + 1, "short Pad;\n");
    print_file(f, indent + 1, "unsigned char Format[%s_FORMAT_STRING_SIZE];\n", str);
    print_file(f, indent, "} MIDL_%s_FORMAT_STRING;\n", str);
    print_file(f, indent, "\n");
}

void write_formatstringsdecl(FILE *f, int indent, ifref_list_t *ifaces, int for_objects)
{
    print_file(f, indent, "#define TYPE_FORMAT_STRING_SIZE %d\n",
               get_size_typeformatstring(ifaces, for_objects));

    print_file(f, indent, "#define PROC_FORMAT_STRING_SIZE %d\n",
               get_size_procformatstring(ifaces, for_objects));

    fprintf(f, "\n");
    write_formatdesc(f, indent, "TYPE");
    write_formatdesc(f, indent, "PROC");
    fprintf(f, "\n");
    print_file(f, indent, "static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;\n");
    print_file(f, indent, "static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;\n");
    print_file(f, indent, "\n");
}

static inline int is_base_type(unsigned char type)
{
    switch (type)
    {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_USMALL:
    case RPC_FC_SMALL:
    case RPC_FC_WCHAR:
    case RPC_FC_USHORT:
    case RPC_FC_SHORT:
    case RPC_FC_ULONG:
    case RPC_FC_LONG:
    case RPC_FC_HYPER:
    case RPC_FC_IGNORE:
    case RPC_FC_FLOAT:
    case RPC_FC_DOUBLE:
    case RPC_FC_ENUM16:
    case RPC_FC_ENUM32:
    case RPC_FC_ERROR_STATUS_T:
    case RPC_FC_BIND_PRIMITIVE:
        return TRUE;

    default:
        return FALSE;
    }
}

int decl_indirect(const type_t *t)
{
    return is_user_type(t)
        || (!is_base_type(t->type)
            && !is_ptr(t)
            && !is_array(t));
}

static size_t write_procformatstring_var(FILE *file, int indent,
                                         const var_t *var, int is_return)
{
    size_t size;
    const type_t *type = var->type;

    int is_in = is_attr(var->attrs, ATTR_IN);
    int is_out = is_attr(var->attrs, ATTR_OUT);

    if (!is_in && !is_out) is_in = TRUE;

    if (!type->declarray && is_base_type(type->type))
    {
        if (is_return)
            print_file(file, indent, "0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
        else
            print_file(file, indent, "0x4e,    /* FC_IN_PARAM_BASETYPE */\n");

        if (type->type == RPC_FC_BIND_PRIMITIVE)
        {
            print_file(file, indent, "0x%02x,    /* FC_IGNORE */\n", RPC_FC_IGNORE);
            size = 2; /* includes param type prefix */
        }
        else if (is_base_type(type->type))
        {
            print_file(file, indent, "0x%02x,    /* %s */\n", type->type, string_of_type(type->type));
            size = 2; /* includes param type prefix */
        }
        else
        {
            error("Unknown/unsupported type: %s (0x%02x)\n", var->name, type->type);
            size = 0;
        }
    }
    else
    {
        if (is_return)
            print_file(file, indent, "0x52,    /* FC_RETURN_PARAM */\n");
        else if (is_in && is_out)
            print_file(file, indent, "0x50,    /* FC_IN_OUT_PARAM */\n");
        else if (is_out)
            print_file(file, indent, "0x51,    /* FC_OUT_PARAM */\n");
        else
            print_file(file, indent, "0x4d,    /* FC_IN_PARAM */\n");

        print_file(file, indent, "0x01,\n");
        print_file(file, indent, "NdrFcShort(0x%x),\n", type->typestring_offset);
        size = 4; /* includes param type prefix */
    }
    return size;
}

void write_procformatstring(FILE *file, const ifref_list_t *ifaces, int for_objects)
{
    const ifref_t *iface;
    int indent = 0;
    const var_t *var;

    print_file(file, indent, "static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =\n");
    print_file(file, indent, "{\n");
    indent++;
    print_file(file, indent, "0,\n");
    print_file(file, indent, "{\n");
    indent++;

    if (ifaces) LIST_FOR_EACH_ENTRY( iface, ifaces, const ifref_t, entry )
    {
        if (for_objects != is_object(iface->iface->attrs) || is_local(iface->iface->attrs))
            continue;

        if (iface->iface->funcs)
        {
            const func_t *func;
            LIST_FOR_EACH_ENTRY( func, iface->iface->funcs, const func_t, entry )
            {
                if (is_local(func->def->attrs)) continue;
                /* emit argument data */
                if (func->args)
                {
                    LIST_FOR_EACH_ENTRY( var, func->args, const var_t, entry )
                        write_procformatstring_var(file, indent, var, FALSE);
                }

                /* emit return value data */
                var = func->def;
                if (is_void(var->type))
                {
                    print_file(file, indent, "0x5b,    /* FC_END */\n");
                    print_file(file, indent, "0x5c,    /* FC_PAD */\n");
                }
                else
                    write_procformatstring_var(file, indent, var, TRUE);
            }
        }
    }

    print_file(file, indent, "0x0\n");
    indent--;
    print_file(file, indent, "}\n");
    indent--;
    print_file(file, indent, "};\n");
    print_file(file, indent, "\n");
}

static int write_base_type(FILE *file, const type_t *type, unsigned int *typestring_offset)
{
    if (is_base_type(type->type))
    {
        print_file(file, 2, "0x%02x,\t/* %s */\n", type->type, string_of_type(type->type));
        *typestring_offset += 1;
        return 1;
    }

    return 0;
}

/* write conformance / variance descriptor */
static size_t write_conf_or_var_desc(FILE *file, const func_t *func, const type_t *structure,
                                     unsigned int baseoff, const expr_t *expr)
{
    unsigned char operator_type = 0;
    const char *operator_string = "no operators";
    const expr_t *subexpr;

    if (!structure)
    {
        /* Top-level conformance calculations are done inline.  */
        print_file (file, 2, "0x%x,\t/* Corr desc: parameter */\n",
                    RPC_FC_TOP_LEVEL_CONFORMANCE);
        print_file (file, 2, "0x0,\n");
        print_file (file, 2, "NdrFcShort(0x0),\n");
        return 4;
    }

    if (expr->is_const)
    {
        if (expr->cval > UCHAR_MAX * (USHRT_MAX + 1) + USHRT_MAX)
            error("write_conf_or_var_desc: constant value %ld is greater than "
                  "the maximum constant size of %d\n", expr->cval,
                  UCHAR_MAX * (USHRT_MAX + 1) + USHRT_MAX);

        print_file(file, 2, "0x%x, /* Corr desc: constant, val = %ld */\n",
                   RPC_FC_CONSTANT_CONFORMANCE, expr->cval);
        print_file(file, 2, "0x%x,\n", expr->cval & ~USHRT_MAX);
        print_file(file, 2, "NdrFcShort(0x%x),\n", expr->cval & USHRT_MAX);

        return 4;
    }

    subexpr = expr;
    switch (subexpr->type)
    {
    case EXPR_PPTR:
        subexpr = subexpr->ref;
        operator_type = RPC_FC_DEREFERENCE;
        operator_string = "FC_DEREFERENCE";
        break;
    case EXPR_DIV:
        if (subexpr->u.ext->is_const && (subexpr->u.ext->cval == 2))
        {
            subexpr = subexpr->ref;
            operator_type = RPC_FC_DIV_2;
            operator_string = "FC_DIV_2";
        }
        break;
    case EXPR_MUL:
        if (subexpr->u.ext->is_const && (subexpr->u.ext->cval == 2))
        {
            subexpr = subexpr->ref;
            operator_type = RPC_FC_MULT_2;
            operator_string = "FC_MULT_2";
        }
        break;
    case EXPR_SUB:
        if (subexpr->u.ext->is_const && (subexpr->u.ext->cval == 1))
        {
            subexpr = subexpr->ref;
            operator_type = RPC_FC_SUB_1;
            operator_string = "FC_SUB_1";
        }
        break;
    case EXPR_ADD:
        if (subexpr->u.ext->is_const && (subexpr->u.ext->cval == 1))
        {
            subexpr = subexpr->ref;
            operator_type = RPC_FC_ADD_1;
            operator_string = "FC_ADD_1";
        }
        break;
    default:
        break;
    }

    if (subexpr->type == EXPR_IDENTIFIER)
    {
        const type_t *correlation_variable = NULL;
        unsigned char correlation_variable_type;
        unsigned char param_type = 0;
        const char *param_type_string = NULL;
        size_t offset = 0;
        const var_t *var;

        if (structure->fields) LIST_FOR_EACH_ENTRY( var, structure->fields, const var_t, entry )
        {
            unsigned int align = 0;
            /* FIXME: take alignment into account */
            if (var->name && !strcmp(var->name, subexpr->u.sval))
            {
                correlation_variable = var->type;
                break;
            }
            offset += type_memsize(var->type, &align);
        }
        if (!correlation_variable)
            error("write_conf_or_var_desc: couldn't find variable %s in structure\n",
                  subexpr->u.sval);

        offset -= baseoff;
        correlation_variable_type = correlation_variable->type;

        switch (correlation_variable_type)
        {
        case RPC_FC_CHAR:
        case RPC_FC_SMALL:
            param_type = RPC_FC_SMALL;
            param_type_string = "FC_SMALL";
            break;
        case RPC_FC_BYTE:
        case RPC_FC_USMALL:
            param_type = RPC_FC_USMALL;
            param_type_string = "FC_USMALL";
            break;
        case RPC_FC_WCHAR:
        case RPC_FC_SHORT:
        case RPC_FC_ENUM16:
            param_type = RPC_FC_SHORT;
            param_type_string = "FC_SHORT";
            break;
        case RPC_FC_USHORT:
            param_type = RPC_FC_USHORT;
            param_type_string = "FC_USHORT";
            break;
        case RPC_FC_LONG:
        case RPC_FC_ENUM32:
            param_type = RPC_FC_LONG;
            param_type_string = "FC_LONG";
            break;
        case RPC_FC_ULONG:
            param_type = RPC_FC_ULONG;
            param_type_string = "FC_ULONG";
            break;
        case RPC_FC_RP:
        case RPC_FC_UP:
        case RPC_FC_OP:
        case RPC_FC_FP:
            if (sizeof(void *) == 4)  /* FIXME */
            {
                param_type = RPC_FC_LONG;
                param_type_string = "FC_LONG";
            }
            else
            {
                param_type = RPC_FC_HYPER;
                param_type_string = "FC_HYPER";
            }
            break;
        default:
            error("write_conf_or_var_desc: conformance variable type not supported 0x%x\n",
                correlation_variable_type);
        }

        print_file(file, 2, "0x%x, /* Corr desc: %s */\n",
                   RPC_FC_NORMAL_CONFORMANCE | param_type, param_type_string);
        print_file(file, 2, "0x%x, /* %s */\n", operator_type, operator_string);
        print_file(file, 2, "NdrFcShort(0x%x), /* offset = %d */\n",
                   offset, offset);
    }
    else
    {
        unsigned int callback_offset = 0;
        struct expr_eval_routine *eval;
        int found = 0;

        LIST_FOR_EACH_ENTRY(eval, &expr_eval_routines, struct expr_eval_routine, entry)
        {
            if (!strcmp (eval->structure->name, structure->name)
                && !compare_expr (eval->expr, expr))
            {
                found = 1;
                break;
            }
            callback_offset++;
        }

        if (!found)
        {
            eval = xmalloc (sizeof(*eval));
            eval->structure = structure;
            eval->expr = expr;
            list_add_tail (&expr_eval_routines, &eval->entry);
        }

        if (callback_offset > USHRT_MAX)
            error("Maximum number of callback routines reached\n");

        print_file(file, 2, "0x%x, /* Corr desc: */\n", RPC_FC_NORMAL_CONFORMANCE);
        print_file(file, 2, "0x%x, /* %s */\n", RPC_FC_CALLBACK, "FC_CALLBACK");
        print_file(file, 2, "NdrFcShort(0x%x), /* %u */\n", callback_offset, callback_offset);
    }
    return 4;
}

static size_t fields_memsize(const var_list_t *fields, unsigned int *align)
{
    int have_align = FALSE;
    size_t size = 0;
    const var_t *v;

    if (!fields) return 0;
    LIST_FOR_EACH_ENTRY( v, fields, const var_t, entry )
    {
        unsigned int falign = 0;
        size_t fsize = type_memsize(v->type, &falign);
        if (!have_align)
        {
            *align = falign;
            have_align = TRUE;
        }
        size = (size + (falign - 1)) & ~(falign - 1);
        size += fsize;
    }

    size = (size + (*align - 1)) & ~(*align - 1);
    return size;
}

static size_t union_memsize(const var_list_t *fields, unsigned int *pmaxa)
{
    size_t size, maxs = 0;
    unsigned int align = *pmaxa;
    const var_t *v;

    if (fields) LIST_FOR_EACH_ENTRY( v, fields, const var_t, entry )
    {
        /* we could have an empty default field with NULL type */
        if (v->type)
        {
            size = type_memsize(v->type, &align);
            if (maxs < size) maxs = size;
            if (*pmaxa < align) *pmaxa = align;
        }
    }

    return maxs;
}

size_t type_memsize(const type_t *t, unsigned int *align)
{
    size_t size = 0;

    if (t->declarray && is_conformant_array(t))
    {
        type_memsize(t->ref, align);
        size = 0;
    }
    else if (is_ptr(t) || is_conformant_array(t))
    {
        size = sizeof(void *);
        if (size > *align) *align = size;
    }
    else switch (t->type)
    {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_USMALL:
    case RPC_FC_SMALL:
        size = 1;
        if (size > *align) *align = size;
        break;
    case RPC_FC_WCHAR:
    case RPC_FC_USHORT:
    case RPC_FC_SHORT:
    case RPC_FC_ENUM16:
        size = 2;
        if (size > *align) *align = size;
        break;
    case RPC_FC_ULONG:
    case RPC_FC_LONG:
    case RPC_FC_ERROR_STATUS_T:
    case RPC_FC_ENUM32:
    case RPC_FC_FLOAT:
        size = 4;
        if (size > *align) *align = size;
        break;
    case RPC_FC_HYPER:
    case RPC_FC_DOUBLE:
        size = 8;
        if (size > *align) *align = size;
        break;
    case RPC_FC_STRUCT:
    case RPC_FC_CVSTRUCT:
    case RPC_FC_CPSTRUCT:
    case RPC_FC_CSTRUCT:
    case RPC_FC_PSTRUCT:
    case RPC_FC_BOGUS_STRUCT:
        size = fields_memsize(t->fields, align);
        break;
    case RPC_FC_ENCAPSULATED_UNION:
    case RPC_FC_NON_ENCAPSULATED_UNION:
        size = union_memsize(t->fields, align);
        break;
    case RPC_FC_SMFARRAY:
    case RPC_FC_LGFARRAY:
    case RPC_FC_SMVARRAY:
    case RPC_FC_LGVARRAY:
    case RPC_FC_BOGUS_ARRAY:
        size = t->dim * type_memsize(t->ref, align);
        break;
    default:
        error("type_memsize: Unknown type %d\n", t->type);
        size = 0;
    }

    return size;
}

static unsigned int write_nonsimple_pointer(FILE *file, const type_t *type, size_t offset)
{
    short absoff = type->ref->typestring_offset;
    short reloff = absoff - (offset + 2);
    int ptr_attr = is_ptr(type->ref) ? 0x10 : 0x0;

    print_file(file, 2, "0x%02x, 0x%x,\t/* %s */\n",
               type->type, ptr_attr, string_of_type(type->type));
    print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset= %hd (%hd) */\n",
               reloff, reloff, absoff);
    return 4;
}

static unsigned int write_simple_pointer(FILE *file, const type_t *type)
{
    print_file(file, 2, "0x%02x, 0x8,\t/* %s [simple_pointer] */\n",
               type->type, string_of_type(type->type));
    print_file(file, 2, "0x%02x,\t/* %s */\n", type->ref->type,
               string_of_type(type->ref->type));
    print_file(file, 2, "0x5c,\t/* FC_PAD */\n");
    return 4;
}

static size_t write_pointer_tfs(FILE *file, type_t *type, unsigned int *typestring_offset)
{
    unsigned int offset = *typestring_offset;

    print_file(file, 0, "/* %d */\n", offset);
    update_tfsoff(type, offset, file);

    if (type->ref->typestring_offset)
        *typestring_offset += write_nonsimple_pointer(file, type, offset);
    else if (is_base_type(type->ref->type))
        *typestring_offset += write_simple_pointer(file, type);

    return offset;
}

static int processed(const type_t *type)
{
    return type->typestring_offset && !type->tfswrite;
}

static void write_user_tfs(FILE *file, type_t *type, unsigned int *tfsoff)
{
    unsigned int start, absoff, flags;
    unsigned int align = 0, ualign = 0;
    const char *name;
    type_t *utype = get_user_type(type, &name);
    size_t usize = type_memsize(utype, &ualign);
    size_t size = type_memsize(type, &align);
    unsigned short funoff = user_type_offset(name);
    short reloff;

    guard_rec(type);

    if (is_base_type(utype->type))
    {
        absoff = *tfsoff;
        print_file(file, 0, "/* %d */\n", absoff);
        print_file(file, 2, "0x%x,\t/* %s */\n", utype->type, string_of_type(utype->type));
        print_file(file, 2, "0x5c,\t/* FC_PAD */\n");
        *tfsoff += 2;
    }
    else
    {
        if (!processed(utype))
            write_embedded_types(file, NULL, utype, utype->name, TRUE, tfsoff);
        absoff = utype->typestring_offset;
    }

    if (utype->type == RPC_FC_RP)
        flags = 0x40;
    else if (utype->type == RPC_FC_UP)
        flags = 0x80;
    else
        flags = 0;

    start = *tfsoff;
    update_tfsoff(type, start, file);
    print_file(file, 0, "/* %d */\n", start);
    print_file(file, 2, "0x%x,\t/* FC_USER_MARSHAL */\n", RPC_FC_USER_MARSHAL);
    print_file(file, 2, "0x%x,\t/* Alignment= %d, Flags= %02x */\n",
               flags | (align - 1), align - 1, flags);
    print_file(file, 2, "NdrFcShort(0x%hx),\t/* Function offset= %hu */\n", funoff, funoff);
    print_file(file, 2, "NdrFcShort(0x%lx),\t/* %lu */\n", usize, usize);
    print_file(file, 2, "NdrFcShort(0x%lx),\t/* %lu */\n", size, size);
    *tfsoff += 8;
    reloff = absoff - *tfsoff;
    print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset= %hd (%lu) */\n", reloff, reloff, absoff);
    *tfsoff += 2;
}

static void write_member_type(FILE *file, type_t *type, const var_t *field,
                              unsigned int *corroff, unsigned int *tfsoff)
{
    if (is_embedded_complex(type))
    {
        size_t absoff;
        short reloff;

        if (is_union(type->type) && is_attr(field->attrs, ATTR_SWITCHIS))
        {
            absoff = *corroff;
            *corroff += 8;
        }
        else
        {
            absoff = type->typestring_offset;
        }
        reloff = absoff - (*tfsoff + 2);

        print_file(file, 2, "0x4c,\t/* FC_EMBEDDED_COMPLEX */\n");
        /* FIXME: actually compute necessary padding */
        print_file(file, 2, "0x0,\t/* FIXME: padding */\n");
        print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset= %hd (%lu) */\n",
                   reloff, reloff, absoff);
        *tfsoff += 4;
    }
    else if (is_ptr(type))
    {
        print_file(file, 2, "0x8,\t/* FC_LONG */\n");
        *tfsoff += 1;
    }
    else if (!write_base_type(file, type, tfsoff))
        error("Unsupported member type 0x%x\n", type->type);
}

static void write_end(FILE *file, unsigned int *tfsoff)
{
    if (*tfsoff % 2 == 0)
    {
        print_file(file, 2, "0x%x,\t\t/* FC_PAD */\n", RPC_FC_PAD);
        *tfsoff += 1;
    }
    print_file(file, 2, "0x%x,\t\t/* FC_END */\n", RPC_FC_END);
    *tfsoff += 1;
}

static void write_descriptors(FILE *file, type_t *type, unsigned int *tfsoff)
{
    unsigned int offset = 0;
    var_list_t *fs = type->fields;
    var_t *f;

    if (fs) LIST_FOR_EACH_ENTRY(f, fs, var_t, entry)
    {
        unsigned int align = 0;
        type_t *ft = f->type;
        if (is_union(ft->type) && is_attr(f->attrs, ATTR_SWITCHIS))
        {
            unsigned int absoff = ft->typestring_offset;
            short reloff = absoff - (*tfsoff + 6);
            print_file(file, 0, "/* %d */\n", *tfsoff);
            print_file(file, 2, "0x%x,\t/* %s */\n", ft->type, string_of_type(ft->type));
            print_file(file, 2, "0x%x,\t/* FIXME: always FC_LONG */\n", RPC_FC_LONG);
            write_conf_or_var_desc(file, current_func, current_structure, offset,
                                   get_attrp(f->attrs, ATTR_SWITCHIS));
            print_file(file, 2, "NdrFcShort(%hd),\t/* Offset= %hd (%u) */\n",
                       reloff, reloff, absoff);
            *tfsoff += 8;
        }

        /* FIXME: take alignment into account */
        offset += type_memsize(ft, &align);
    }
}

static int write_no_repeat_pointer_descriptions(
    FILE *file, type_t *type,
    size_t *offset_in_memory, size_t *offset_in_buffer,
    unsigned int *typestring_offset)
{
    int written = 0;
    unsigned int align;

    if (is_ptr(type))
    {
        print_file(file, 2, "0x%02x, /* FC_NO_REPEAT */\n", RPC_FC_NO_REPEAT);
        print_file(file, 2, "0x%02x, /* FC_PAD */\n", RPC_FC_PAD);

        /* pointer instance */
        print_file(file, 2, "NdrFcShort(0x%x), /* Memory offset = %d */\n", *offset_in_memory, *offset_in_memory);
        print_file(file, 2, "NdrFcShort(0x%x), /* Buffer offset = %d */\n", *offset_in_buffer, *offset_in_buffer);
        *typestring_offset += 6;

        if (processed(type->ref) || is_base_type(type->ref->type))
            write_pointer_tfs(file, type, typestring_offset);
        else
            error("write_pointer_description: type format string unknown\n");

        align = 0;
        *offset_in_memory += type_memsize(type, &align);
        /* FIXME: is there a case where these two are different? */
        align = 0;
        *offset_in_buffer += type_memsize(type, &align);

        return 1;
    }

    if (is_non_complex_struct(type))
    {
        const var_t *v;
        LIST_FOR_EACH_ENTRY( v, type->fields, const var_t, entry )
            written += write_no_repeat_pointer_descriptions(
                file, v->type,
                offset_in_memory, offset_in_buffer, typestring_offset);
    }
    else
    {
        align = 0;
        *offset_in_memory += type_memsize(type, &align);
        /* FIXME: is there a case where these two are different? */
        align = 0;
        *offset_in_buffer += type_memsize(type, &align);
    }

    return written;
}

static int write_pointer_description_offsets(
    FILE *file, const attr_list_t *attrs, type_t *type,
    size_t *offset_in_memory, size_t *offset_in_buffer,
    unsigned int *typestring_offset)
{
    int written = 0;
    unsigned int align;

    if (is_ptr(type) && type->ref->type != RPC_FC_IP)
    {
        if (offset_in_memory && offset_in_buffer)
        {
            /* pointer instance */
            /* FIXME: sometimes from end of structure, sometimes from beginning */
            print_file(file, 2, "NdrFcShort(0x%x), /* Memory offset = %d */\n", *offset_in_memory, *offset_in_memory);
            print_file(file, 2, "NdrFcShort(0x%x), /* Buffer offset = %d */\n", *offset_in_buffer, *offset_in_buffer);

            align = 0;
            *offset_in_memory += type_memsize(type, &align);
            /* FIXME: is there a case where these two are different? */
            align = 0;
            *offset_in_buffer += type_memsize(type, &align);
        }
        *typestring_offset += 4;

        if (processed(type->ref) || is_base_type(type->ref->type))
            write_pointer_tfs(file, type, typestring_offset);
        else
            error("write_pointer_description_offsets: type format string unknown\n");

        return 1;
    }

    if (is_array(type))
    {
        return write_pointer_description_offsets(
            file, attrs, type->ref, offset_in_memory, offset_in_buffer,
                                                 typestring_offset);
    }
    else if (is_non_complex_struct(type))
    {
        /* otherwise search for interesting fields to parse */
        const var_t *v;
        LIST_FOR_EACH_ENTRY( v, type->fields, const var_t, entry )
        {
            written += write_pointer_description_offsets(
                file, v->attrs, v->type, offset_in_memory, offset_in_buffer,
                typestring_offset);
        }
    }
    else
    {
        align = 0;
        if (offset_in_memory)
            *offset_in_memory += type_memsize(type, &align);
        /* FIXME: is there a case where these two are different? */
        align = 0;
        if (offset_in_buffer)
            *offset_in_buffer += type_memsize(type, &align);
    }

    return written;
}

/* Note: if file is NULL return value is number of pointers to write, else
 * it is the number of type format characters written */
static int write_fixed_array_pointer_descriptions(
    FILE *file, const attr_list_t *attrs, type_t *type,
    size_t *offset_in_memory, size_t *offset_in_buffer,
    unsigned int *typestring_offset)
{
    unsigned int align;
    int pointer_count = 0;

    if (type->type == RPC_FC_SMFARRAY || type->type == RPC_FC_LGFARRAY)
    {
        unsigned int temp = 0;
        /* unfortunately, this needs to be done in two passes to avoid
         * writing out redundant FC_FIXED_REPEAT descriptions */
        pointer_count = write_pointer_description_offsets(
            NULL, attrs, type->ref, NULL, NULL, &temp);
        if (pointer_count > 0)
        {
            unsigned int increment_size;
            size_t offset_of_array_pointer_mem = 0;
            size_t offset_of_array_pointer_buf = 0;

            align = 0;
            increment_size = type_memsize(type->ref, &align);

            print_file(file, 2, "0x%02x, /* FC_FIXED_REPEAT */\n", RPC_FC_FIXED_REPEAT);
            print_file(file, 2, "0x%02x, /* FC_PAD */\n", RPC_FC_PAD);
            print_file(file, 2, "NdrFcShort(0x%x), /* Iterations = %d */\n", type->dim, type->dim);
            print_file(file, 2, "NdrFcShort(0x%x), /* Increment = %d */\n", increment_size, increment_size);
            print_file(file, 2, "NdrFcShort(0x%x), /* Offset to array = %d */\n", *offset_in_memory, *offset_in_memory);
            print_file(file, 2, "NdrFcShort(0x%x), /* Number of pointers = %d */\n", pointer_count, pointer_count);
            *typestring_offset += 10;

            pointer_count = write_pointer_description_offsets(
                file, attrs, type, &offset_of_array_pointer_mem,
                &offset_of_array_pointer_buf, typestring_offset);
        }
    }
    else if (is_struct(type->type))
    {
        const var_t *v;
        LIST_FOR_EACH_ENTRY( v, type->fields, const var_t, entry )
        {
            pointer_count += write_fixed_array_pointer_descriptions(
                file, v->attrs, v->type, offset_in_memory, offset_in_buffer,
                typestring_offset);
        }
    }
    else
    {
        align = 0;
        if (offset_in_memory)
            *offset_in_memory += type_memsize(type, &align);
        /* FIXME: is there a case where these two are different? */
        align = 0;
        if (offset_in_buffer)
            *offset_in_buffer += type_memsize(type, &align);
    }

    return pointer_count;
}

/* Note: if file is NULL return value is number of pointers to write, else
 * it is the number of type format characters written */
static int write_conformant_array_pointer_descriptions(
    FILE *file, const attr_list_t *attrs, type_t *type,
    size_t *offset_in_memory, size_t *offset_in_buffer,
    unsigned int *typestring_offset)
{
    unsigned int align;
    int pointer_count = 0;

    if (is_conformant_array(type) && !type->length_is)
    {
        unsigned int temp = 0;
        /* unfortunately, this needs to be done in two passes to avoid
         * writing out redundant FC_VARIABLE_REPEAT descriptions */
        pointer_count = write_pointer_description_offsets(
            NULL, attrs, type->ref, NULL, NULL, &temp);
        if (pointer_count > 0)
        {
            unsigned int increment_size;
            size_t offset_of_array_pointer_mem = 0;
            size_t offset_of_array_pointer_buf = 0;

            align = 0;
            increment_size = type_memsize(type->ref, &align);

            if (increment_size > USHRT_MAX)
                error("array size of %u bytes is too large\n", increment_size);

            print_file(file, 2, "0x%02x, /* FC_VARIABLE_REPEAT */\n", RPC_FC_VARIABLE_REPEAT);
            print_file(file, 2, "0x%02x, /* FC_FIXED_OFFSET */\n", RPC_FC_FIXED_OFFSET);
            print_file(file, 2, "NdrFcShort(0x%x), /* Increment = %d */\n", increment_size, increment_size);
            print_file(file, 2, "NdrFcShort(0x%x), /* Offset to array = %d */\n", *offset_in_memory, *offset_in_memory);
            print_file(file, 2, "NdrFcShort(0x%x), /* Number of pointers = %d */\n", pointer_count, pointer_count);
            *typestring_offset += 8;

            pointer_count = write_pointer_description_offsets(
                file, attrs, type->ref, &offset_of_array_pointer_mem,
                &offset_of_array_pointer_buf, typestring_offset);
        }
    }
    else if (is_struct(type->type))
    {
        const var_t *v;
        LIST_FOR_EACH_ENTRY( v, type->fields, const var_t, entry )
        {
            pointer_count += write_conformant_array_pointer_descriptions(
                file, v->attrs, v->type, offset_in_memory, offset_in_buffer,
                typestring_offset);
        }
    }
    else
    {
        align = 0;
        if (offset_in_memory)
            *offset_in_memory += type_memsize(type, &align);
        /* FIXME: is there a case where these two are different? */
        align = 0;
        if (offset_in_buffer)
            *offset_in_buffer += type_memsize(type, &align);
    }

    return pointer_count;
}

/* Note: if file is NULL return value is number of pointers to write, else
 * it is the number of type format characters written */
static int write_varying_array_pointer_descriptions(
    FILE *file, const attr_list_t *attrs, type_t *type,
    size_t *offset_in_memory, size_t *offset_in_buffer,
    unsigned int *typestring_offset)
{
    unsigned int align;
    int pointer_count = 0;

    /* FIXME: do varying array searching here, but pointer searching in write_pointer_description_offsets */

    if (is_array(type) && type->length_is)
    {
        unsigned int temp = 0;
        /* unfortunately, this needs to be done in two passes to avoid
         * writing out redundant FC_VARIABLE_REPEAT descriptions */
        pointer_count = write_pointer_description_offsets(
            NULL, attrs, type->ref, NULL, NULL, &temp);
        if (pointer_count > 0)
        {
            unsigned int increment_size;
            size_t offset_of_array_pointer_mem = 0;
            size_t offset_of_array_pointer_buf = 0;

            align = 0;
            increment_size = type_memsize(type->ref, &align);

            if (increment_size > USHRT_MAX)
                error("array size of %u bytes is too large\n", increment_size);

            print_file(file, 2, "0x%02x, /* FC_VARIABLE_REPEAT */\n", RPC_FC_VARIABLE_REPEAT);
            print_file(file, 2, "0x%02x, /* FC_VARIABLE_OFFSET */\n", RPC_FC_VARIABLE_OFFSET);
            print_file(file, 2, "NdrFcShort(0x%x), /* Increment = %d */\n", increment_size, increment_size);
            print_file(file, 2, "NdrFcShort(0x%x), /* Offset to array = %d */\n", *offset_in_memory, *offset_in_memory);
            print_file(file, 2, "NdrFcShort(0x%x), /* Number of pointers = %d */\n", pointer_count, pointer_count);
            *typestring_offset += 8;

            pointer_count = write_pointer_description_offsets(
                file, attrs, type, &offset_of_array_pointer_mem,
                &offset_of_array_pointer_buf, typestring_offset);
        }
    }
    else if (is_struct(type->type))
    {
        const var_t *v;
        LIST_FOR_EACH_ENTRY( v, type->fields, const var_t, entry )
        {
            pointer_count += write_varying_array_pointer_descriptions(
                file, v->attrs, v->type, offset_in_memory, offset_in_buffer,
                typestring_offset);
        }
    }
    else
    {
        align = 0;
        if (offset_in_memory)
            *offset_in_memory += type_memsize(type, &align);
        /* FIXME: is there a case where these two are different? */
        align = 0;
        if (offset_in_buffer)
            *offset_in_buffer += type_memsize(type, &align);
    }

    return pointer_count;
}

static void write_pointer_description(FILE *file, type_t *type,
                                      unsigned int *typestring_offset)
{
    size_t offset_in_buffer;
    size_t offset_in_memory;

    /* pass 1: search for single instance of a pointer (i.e. don't descend
     * into arrays) */
    offset_in_memory = 0;
    offset_in_buffer = 0;
    write_no_repeat_pointer_descriptions(
        file, type,
        &offset_in_memory, &offset_in_buffer, typestring_offset);

    /* pass 2: search for pointers in fixed arrays */
    offset_in_memory = 0;
    offset_in_buffer = 0;
    write_fixed_array_pointer_descriptions(
        file, NULL, type,
        &offset_in_memory, &offset_in_buffer, typestring_offset);

    /* pass 3: search for pointers in conformant only arrays (but don't descend
     * into conformant varying or varying arrays) */
    offset_in_memory = 0;
    offset_in_buffer = 0;
    write_conformant_array_pointer_descriptions(
        file, NULL, type,
        &offset_in_memory, &offset_in_buffer, typestring_offset);

   /* pass 4: search for pointers in varying arrays */
    offset_in_memory = 0;
    offset_in_buffer = 0;
    write_varying_array_pointer_descriptions(
            file, NULL, type,
            &offset_in_memory, &offset_in_buffer, typestring_offset);
}

static int is_declptr(const type_t *t)
{
  return is_ptr(t) || (is_conformant_array(t) && !t->declarray);
}

static size_t write_string_tfs(FILE *file, const attr_list_t *attrs,
                               const type_t *type,
                               const char *name, unsigned int *typestring_offset)
{
    size_t start_offset = *typestring_offset;
    unsigned char rtype;

    if (is_declptr(type))
    {
        unsigned char flag = is_conformant_array(type) ? 0 : RPC_FC_P_SIMPLEPOINTER;
        int pointer_type = is_ptr(type) ? type->type : get_attrv(attrs, ATTR_POINTERTYPE);
        if (!pointer_type)
            pointer_type = RPC_FC_RP;
        print_file(file, 2,"0x%x, 0x%x,\t/* %s%s */\n",
                   pointer_type, flag, string_of_type(pointer_type),
                   flag ? " [simple_pointer]" : "");
        *typestring_offset += 2;
        if (!flag)
        {
            print_file(file, 2, "NdrFcShort(0x2),\n");
            *typestring_offset += 2;
        }
        rtype = type->ref->type;
    }
    else
        rtype = type->type;

    if ((rtype != RPC_FC_BYTE) && (rtype != RPC_FC_CHAR) && (rtype != RPC_FC_WCHAR))
    {
        error("write_string_tfs: Unimplemented for type 0x%x of name: %s\n", rtype, name);
        return start_offset;
    }

    if (type->declarray && !is_conformant_array(type))
    {
        /* FIXME: multi-dimensional array */
        if (0xffffuL < type->dim)
            error("array size for parameter %s exceeds %u bytes by %lu bytes\n",
                  name, 0xffffu, type->dim - 0xffffu);

        if (rtype == RPC_FC_CHAR)
            WRITE_FCTYPE(file, FC_CSTRING, *typestring_offset);
        else
            WRITE_FCTYPE(file, FC_WSTRING, *typestring_offset);
        print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
        *typestring_offset += 2;

        print_file(file, 2, "NdrFcShort(0x%x), /* %d */\n", type->dim, type->dim);
        *typestring_offset += 2;

        return start_offset;
    }
    else if (type->size_is)
    {
        unsigned int align = 0;

        if (rtype == RPC_FC_CHAR)
            WRITE_FCTYPE(file, FC_C_CSTRING, *typestring_offset);
        else
            WRITE_FCTYPE(file, FC_C_WSTRING, *typestring_offset);
        print_file(file, 2, "0x%x, /* FC_STRING_SIZED */\n", RPC_FC_STRING_SIZED);
        *typestring_offset += 2;

        *typestring_offset += write_conf_or_var_desc(
            file, current_func, current_structure,
            (type->declarray && current_structure
             ? type_memsize(current_structure, &align)
             : 0),
            type->size_is);

        return start_offset;
    }
    else
    {
        if (rtype == RPC_FC_WCHAR)
            WRITE_FCTYPE(file, FC_C_WSTRING, *typestring_offset);
        else
            WRITE_FCTYPE(file, FC_C_CSTRING, *typestring_offset);
        print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
        *typestring_offset += 2;

        return start_offset;
    }
}

static size_t write_array_tfs(FILE *file, const attr_list_t *attrs, type_t *type,
                              const char *name, unsigned int *typestring_offset)
{
    const expr_t *length_is = type->length_is;
    const expr_t *size_is = type->size_is;
    unsigned int align = 0;
    size_t size;
    size_t start_offset;
    int has_pointer;
    int pointer_type = get_attrv(attrs, ATTR_POINTERTYPE);
    if (!pointer_type)
        pointer_type = RPC_FC_RP;

    has_pointer = FALSE;
    if (write_embedded_types(file, attrs, type->ref, name, FALSE, typestring_offset))
        has_pointer = TRUE;

    size = type_memsize(type, &align);
    if (size == 0)              /* conformant array */
        size = type_memsize(type->ref, &align);

    start_offset = *typestring_offset;
    update_tfsoff(type, start_offset, file);
    print_file(file, 0, "/* %lu */\n", start_offset);
    print_file(file, 2, "0x%02x,\t/* %s */\n", type->type, string_of_type(type->type));
    print_file(file, 2, "0x%x,\t/* %d */\n", align - 1, align - 1);
    *typestring_offset += 2;

    align = 0;
    if (type->type != RPC_FC_BOGUS_ARRAY)
    {
        unsigned char tc = type->type;
        unsigned int baseoff
            = type->declarray && current_structure
            ? type_memsize(current_structure, &align)
            : 0;

        if (tc == RPC_FC_LGFARRAY || tc == RPC_FC_LGVARRAY)
        {
            print_file(file, 2, "NdrFcLong(0x%x),\t/* %lu */\n", size, size);
            *typestring_offset += 4;
        }
        else
        {
            print_file(file, 2, "NdrFcShort(0x%x),\t/* %lu */\n", size, size);
            *typestring_offset += 2;
        }

        if (is_conformant_array(type))
            *typestring_offset
                += write_conf_or_var_desc(file, current_func, current_structure,
                                          baseoff, size_is);

        if (type->type == RPC_FC_SMVARRAY || type->type == RPC_FC_LGVARRAY)
        {
            unsigned int elalign = 0;
            size_t elsize = type_memsize(type->ref, &elalign);

            if (type->type == RPC_FC_LGVARRAY)
            {
                print_file(file, 2, "NdrFcLong(0x%x),\t/* %lu */\n", type->dim, type->dim);
                *typestring_offset += 4;
            }
            else
            {
                print_file(file, 2, "NdrFcShort(0x%x),\t/* %lu */\n", type->dim, type->dim);
                *typestring_offset += 2;
            }

            print_file(file, 2, "NdrFcShort(0x%x),\t/* %lu */\n", elsize, elsize);
            *typestring_offset += 2;
        }

        if (length_is)
            *typestring_offset
                += write_conf_or_var_desc(file, current_func, current_structure,
                                          baseoff, length_is);

        if (has_pointer)
        {
            print_file(file, 2, "0x%x, /* FC_PP */\n", RPC_FC_PP);
            print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
            *typestring_offset += 2;
            write_pointer_description(file, type, typestring_offset);
            print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
            *typestring_offset += 1;
        }

        write_member_type(file, type->ref, NULL, NULL, typestring_offset);
        write_end(file, typestring_offset);
    }
    else
        error("%s: complex arrays unimplemented\n", name);

    return start_offset;
}

static const var_t *find_array_or_string_in_struct(const type_t *type)
{
    const var_t *last_field = LIST_ENTRY( list_tail(type->fields), const var_t, entry );
    const type_t *ft = last_field->type;

    if (ft->declarray && is_conformant_array(ft))
        return last_field;

    if (ft->type == RPC_FC_CSTRUCT || ft->type == RPC_FC_CPSTRUCT || ft->type == RPC_FC_CVSTRUCT)
        return find_array_or_string_in_struct(last_field->type);
    else
        return NULL;
}

static void write_struct_members(FILE *file, const type_t *type,
                                 unsigned int *corroff, unsigned int *typestring_offset)
{
    const var_t *field;
    unsigned short offset = 0;

    if (type->fields) LIST_FOR_EACH_ENTRY( field, type->fields, const var_t, entry )
    {
        type_t *ft = field->type;
        if (!ft->declarray || !is_conformant_array(ft))
        {
            unsigned int align = 0;
            size_t size = type_memsize(ft, &align);
            if ((align - 1) & offset)
            {
                unsigned char fc = 0;
                switch (align)
                {
                case 4:
                    fc = RPC_FC_ALIGNM4;
                    break;
                case 8:
                    fc = RPC_FC_ALIGNM8;
                    break;
                default:
                    error("write_struct_members: cannot align type %d", ft->type);
                }
                print_file(file, 2, "0x%x,\t/* %s */\n", fc, string_of_type(fc));
                offset = (offset + (align - 1)) & ~(align - 1);
                *typestring_offset += 1;
            }
            write_member_type(file, ft, field, corroff, typestring_offset);
            offset += size;
        }
    }

    write_end(file, typestring_offset);
}

static size_t write_struct_tfs(FILE *file, type_t *type,
                               const char *name, unsigned int *tfsoff)
{
    const type_t *save_current_structure = current_structure;
    unsigned int total_size;
    const var_t *array;
    size_t start_offset;
    size_t array_offset;
    int has_pointers = 0;
    unsigned int align = 0;
    unsigned int corroff;
    var_t *f;

    guard_rec(type);
    current_structure = type;

    total_size = type_memsize(type, &align);
    if (total_size > USHRT_MAX)
        error("structure size for %s exceeds %d bytes by %d bytes\n",
              name, USHRT_MAX, total_size - USHRT_MAX);

    if (type->fields) LIST_FOR_EACH_ENTRY(f, type->fields, var_t, entry)
        has_pointers |= write_embedded_types(file, f->attrs, f->type, f->name,
                                             FALSE, tfsoff);

    array = find_array_or_string_in_struct(type);
    if (array && !processed(array->type))
        array_offset
            = is_attr(array->attrs, ATTR_STRING)
            ? write_string_tfs(file, array->attrs, array->type, array->name, tfsoff)
            : write_array_tfs(file, array->attrs, array->type, array->name, tfsoff);

    corroff = *tfsoff;
    write_descriptors(file, type, tfsoff);

    start_offset = *tfsoff;
    update_tfsoff(type, start_offset, file);
    print_file(file, 0, "/* %d */\n", start_offset);
    print_file(file, 2, "0x%x,\t/* %s */\n", type->type, string_of_type(type->type));
    print_file(file, 2, "0x%x,\t/* %d */\n", align - 1, align - 1);
    print_file(file, 2, "NdrFcShort(0x%x),\t/* %d */\n", total_size, total_size);
    *tfsoff += 4;

    if (array)
    {
        unsigned int absoff = array->type->typestring_offset;
        short reloff = absoff - *tfsoff;
        print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset= %hd (%lu) */\n",
                   reloff, reloff, absoff);
        *tfsoff += 2;
    }
    else if (type->type == RPC_FC_BOGUS_STRUCT)
    {
        print_file(file, 2, "NdrFcShort(0x0),\n");
        *tfsoff += 2;
    }

    if (type->type == RPC_FC_BOGUS_STRUCT)
    {

        print_file(file, 2, "NdrFcShort(0x0),\t/* FIXME: pointer stuff */\n");
        *tfsoff += 2;
    }
    else if ((type->type == RPC_FC_PSTRUCT) ||
             (type->type == RPC_FC_CPSTRUCT) ||
             (type->type == RPC_FC_CVSTRUCT && has_pointers))
    {
        print_file(file, 2, "0x%x, /* FC_PP */\n", RPC_FC_PP);
        print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
        *tfsoff += 2;
        write_pointer_description(file, type, tfsoff);
        print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
        *tfsoff += 1;
    }

    write_struct_members(file, type, &corroff, tfsoff);

    current_structure = save_current_structure;
    return start_offset;
}

static size_t write_pointer_only_tfs(FILE *file, const attr_list_t *attrs, int pointer_type,
                                     unsigned char flags, size_t offset,
                                     unsigned int *typeformat_offset)
{
    size_t start_offset = *typeformat_offset;
    short reloff = offset - (*typeformat_offset + 2);
    int in_attr, out_attr;
    in_attr = is_attr(attrs, ATTR_IN);
    out_attr = is_attr(attrs, ATTR_OUT);
    if (!in_attr && !out_attr) in_attr = 1;

    if (out_attr && !in_attr && pointer_type == RPC_FC_RP)
        flags |= 0x04;

    print_file(file, 2, "0x%x, 0x%x,\t\t/* %s",
               pointer_type,
               flags,
               string_of_type(pointer_type));
    if (file)
    {
        if (flags & 0x04)
            fprintf(file, " [allocated_on_stack]");
        if (flags & 0x10)
            fprintf(file, " [pointer_deref]");
        fprintf(file, " */\n");
    }

    print_file(file, 2, "NdrFcShort(0x%x),\t/* %d */\n", reloff, offset);
    *typeformat_offset += 4;

    return start_offset;
}

static void write_branch_type(FILE *file, const type_t *t, unsigned int *tfsoff)
{
    if (t == NULL)
    {
        print_file(file, 2, "NdrFcShort(0x0),\t/* No type */\n");
    }
    else if (is_base_type(t->type))
    {
        print_file(file, 2, "NdrFcShort(0x80%02x),\t/* Simple arm type: %s */\n",
                   t->type, string_of_type(t->type));
    }
    else if (t->typestring_offset)
    {
        short reloff = t->typestring_offset - *tfsoff;
        print_file(file, 2, "NdrFcShort(0x%x),\t/* Offset= %d (%d) */\n",
                   reloff, reloff, t->typestring_offset);
    }
    else
        error("write_branch_type: type unimplemented (0x%x)\n", t->type);

    *tfsoff += 2;
}

static size_t write_union_tfs(FILE *file, type_t *type, unsigned int *tfsoff)
{
    unsigned int align = 0;
    unsigned int start_offset;
    size_t size = type_memsize(type, &align);
    var_list_t *fields;
    size_t nbranch = 0;
    type_t *deftype = NULL;
    short nodeftype = 0xffff;
    var_t *f;

    guard_rec(type);

    if (type->type == RPC_FC_ENCAPSULATED_UNION)
    {
        const var_t *uv = LIST_ENTRY(list_tail(type->fields), const var_t, entry);
        fields = uv->type->fields;
    }
    else
        fields = type->fields;

    if (fields) LIST_FOR_EACH_ENTRY(f, fields, var_t, entry)
    {
        expr_list_t *cases = get_attrp(f->attrs, ATTR_CASE);
        if (cases)
            nbranch += list_count(cases);
        if (f->type)
            write_embedded_types(file, f->attrs, f->type, f->name, TRUE, tfsoff);
    }

    start_offset = *tfsoff;
    update_tfsoff(type, start_offset, file);
    print_file(file, 0, "/* %d */\n", start_offset);
    if (type->type == RPC_FC_ENCAPSULATED_UNION)
    {
        const var_t *sv = LIST_ENTRY(list_head(type->fields), const var_t, entry);
        const type_t *st = sv->type;

        switch (st->type)
        {
        case RPC_FC_CHAR:
        case RPC_FC_SMALL:
        case RPC_FC_USMALL:
        case RPC_FC_SHORT:
        case RPC_FC_USHORT:
        case RPC_FC_LONG:
        case RPC_FC_ULONG:
        case RPC_FC_ENUM16:
        case RPC_FC_ENUM32:
            print_file(file, 2, "0x%x,\t/* %s */\n", type->type, string_of_type(type->type));
            print_file(file, 2, "0x%x,\t/* Switch type= %s */\n",
                       0x40 | st->type, string_of_type(st->type));
            *tfsoff += 2;
            break;
        default:
            error("union switch type must be an integer, char, or enum\n");
        }
    }
    print_file(file, 2, "NdrFcShort(0x%x),\t/* %d */\n", size, size);
    print_file(file, 2, "NdrFcShort(0x%x),\t/* %d */\n", nbranch, nbranch);
    *tfsoff += 4;

    if (fields) LIST_FOR_EACH_ENTRY(f, fields, var_t, entry)
    {
        type_t *ft = f->type;
        expr_list_t *cases = get_attrp(f->attrs, ATTR_CASE);
        int deflt = is_attr(f->attrs, ATTR_DEFAULT);
        expr_t *c;

        if (cases == NULL && !deflt)
            error("union field %s with neither case nor default attribute\n", f->name);

        if (cases) LIST_FOR_EACH_ENTRY(c, cases, expr_t, entry)
        {
            /* MIDL doesn't check for duplicate cases, even though that seems
               like a reasonable thing to do, it just dumps them to the TFS
               like we're going to do here.  */
            print_file(file, 2, "NdrFcLong(0x%x),\t/* %d */\n", c->cval, c->cval);
            *tfsoff += 4;
            write_branch_type(file, ft, tfsoff);
        }

        /* MIDL allows multiple default branches, even though that seems
           illogical, it just chooses the last one, which is what we will
           do.  */
        if (deflt)
        {
            deftype = ft;
            nodeftype = 0;
        }
    }

    if (deftype)
    {
        write_branch_type(file, deftype, tfsoff);
    }
    else
    {
        print_file(file, 2, "NdrFcShort(0x%x),\n", nodeftype);
        *tfsoff += 2;
    }

    return start_offset;
}

static size_t write_ip_tfs(FILE *file, const func_t *func, const attr_list_t *attrs,
                           type_t *type, unsigned int *typeformat_offset)
{
    size_t i;
    size_t start_offset = *typeformat_offset;
    const var_t *iid = get_attrp(attrs, ATTR_IIDIS);

    if (iid)
    {
        expr_t expr;

        expr.type = EXPR_IDENTIFIER;
        expr.ref  = NULL;
        expr.u.sval = iid->name;
        expr.is_const = FALSE;
        print_file(file, 2, "0x2f,  /* FC_IP */\n");
        print_file(file, 2, "0x5c,  /* FC_PAD */\n");
        *typeformat_offset += write_conf_or_var_desc(file, func, NULL, 0, &expr) + 2;
    }
    else
    {
        const type_t *base = is_ptr(type) ? type->ref : type;
        const UUID *uuid = get_attrp(base->attrs, ATTR_UUID);

        if (! uuid)
            error("%s: interface %s missing UUID\n", __FUNCTION__, base->name);

        update_tfsoff(type, start_offset, file);
        print_file(file, 0, "/* %d */\n", start_offset);
        print_file(file, 2, "0x2f,\t/* FC_IP */\n");
        print_file(file, 2, "0x5a,\t/* FC_CONSTANT_IID */\n");
        print_file(file, 2, "NdrFcLong(0x%08lx),\n", uuid->Data1);
        print_file(file, 2, "NdrFcShort(0x%04x),\n", uuid->Data2);
        print_file(file, 2, "NdrFcShort(0x%04x),\n", uuid->Data3);
        for (i = 0; i < 8; ++i)
            print_file(file, 2, "0x%02x,\n", uuid->Data4[i]);

        if (file)
            fprintf(file, "\n");

        *typeformat_offset += 18;
    }
    return start_offset;
}

static int get_ptr_attr(const type_t *t, int def_type)
{
    while (TRUE)
    {
        int ptr_attr = get_attrv(t->attrs, ATTR_POINTERTYPE);
        if (ptr_attr)
            return ptr_attr;
        if (t->kind != TKIND_ALIAS)
            return def_type;
        t = t->orig;
    }
}

static size_t write_typeformatstring_var(FILE *file, int indent, const func_t *func,
                                         type_t *type, const var_t *var,
                                         unsigned int *typeformat_offset)
{
    int pointer_type;
    size_t offset;

    if (is_user_type(type))
    {
        write_user_tfs(file, type, typeformat_offset);
        return type->typestring_offset;
    }

    if (type == var->type)      /* top-level pointers */
    {
        int pointer_attr = get_attrv(var->attrs, ATTR_POINTERTYPE);
        if (pointer_attr != 0 && !is_ptr(type) && !is_array(type))
            error("'%s': pointer attribute applied to non-pointer type\n", var->name);

        if (pointer_attr == 0)
            pointer_attr = get_ptr_attr(type, RPC_FC_RP);

        pointer_type = pointer_attr;
    }
    else
        pointer_type = get_ptr_attr(type, RPC_FC_UP);

    if ((last_ptr(type) || last_array(type)) && is_ptrchain_attr(var, ATTR_STRING))
        return write_string_tfs(file, var->attrs, type, var->name, typeformat_offset);

    if (is_array(type))
        return write_array_tfs(file, var->attrs, type, var->name, typeformat_offset);

    if (!is_ptr(type))
    {
        /* basic types don't need a type format string */
        if (is_base_type(type->type))
            return 0;

        switch (type->type)
        {
        case RPC_FC_STRUCT:
        case RPC_FC_PSTRUCT:
        case RPC_FC_CSTRUCT:
        case RPC_FC_CPSTRUCT:
        case RPC_FC_CVSTRUCT:
        case RPC_FC_BOGUS_STRUCT:
            return write_struct_tfs(file, type, var->name, typeformat_offset);
        case RPC_FC_ENCAPSULATED_UNION:
        case RPC_FC_NON_ENCAPSULATED_UNION:
            return write_union_tfs(file, type, typeformat_offset);
        case RPC_FC_IGNORE:
        case RPC_FC_BIND_PRIMITIVE:
            /* nothing to do */
            return 0;
        default:
            error("write_typeformatstring_var: Unsupported type 0x%x for variable %s\n", type->type, var->name);
        }
    }
    else if (last_ptr(type))
    {
        size_t start_offset = *typeformat_offset;
        int in_attr = is_attr(var->attrs, ATTR_IN);
        int out_attr = is_attr(var->attrs, ATTR_OUT);
        const type_t *base = type->ref;

        if (base->type == RPC_FC_IP)
        {
            return write_ip_tfs(file, func, var->attrs, type, typeformat_offset);
        }

        /* special case for pointers to base types */
        if (is_base_type(base->type))
        {
            print_file(file, indent, "0x%x, 0x%x,    /* %s %s[simple_pointer] */\n",
                       pointer_type, (!in_attr && out_attr) ? 0x0C : 0x08,
                       string_of_type(pointer_type),
                       (!in_attr && out_attr) ? "[allocated_on_stack] " : "");
            print_file(file, indent, "0x%02x,    /* %s */\n", base->type, string_of_type(base->type));
            print_file(file, indent, "0x5c,          /* FC_PAD */\n");
            *typeformat_offset += 4;
            return start_offset;
        }
    }

    assert(is_ptr(type));

    offset = write_typeformatstring_var(file, indent, func, type->ref, var, typeformat_offset);
    if (file)
        fprintf(file, "/* %2u */\n", *typeformat_offset);
    return write_pointer_only_tfs(file, var->attrs, pointer_type,
                           !last_ptr(type) ? 0x10 : 0,
                           offset, typeformat_offset);
}

static void set_tfswrite(type_t *type, int val)
{
    while (type->tfswrite != val)
    {
        type_t *utype = get_user_type(type, NULL);

        type->tfswrite = val;

        if (utype)
            set_tfswrite(utype, val);

        if (type->kind == TKIND_ALIAS)
            type = type->orig;
        else if (is_ptr(type) || is_array(type))
            type = type->ref;
        else
        {
            if (type->fields)
            {
                var_t *v;
                LIST_FOR_EACH_ENTRY( v, type->fields, var_t, entry )
                    if (v->type)
                        set_tfswrite(v->type, val);
            }

            return;
        }
    }
}

static int write_embedded_types(FILE *file, const attr_list_t *attrs, type_t *type,
                                const char *name, int write_ptr, unsigned int *tfsoff)
{
    int retmask = 0;

    if (is_user_type(type))
    {
        write_user_tfs(file, type, tfsoff);
    }
    else if (is_ptr(type))
    {
        type_t *ref = type->ref;

        if (ref->type == RPC_FC_IP)
        {
            write_ip_tfs(file, NULL, attrs, type, tfsoff);
        }
        else
        {
            if (!processed(ref) && !is_base_type(ref->type))
                retmask |= write_embedded_types(file, NULL, ref, name, TRUE, tfsoff);

            if (write_ptr)
                write_pointer_tfs(file, type, tfsoff);

            retmask |= 1;
        }
    }
    else if (type->declarray && is_conformant_array(type))
        ;    /* conformant arrays and strings are handled specially */
    else if (is_array(type))
    {
        write_array_tfs(file, attrs, type, name, tfsoff);
    }
    else if (is_struct(type->type))
    {
        if (!processed(type))
            write_struct_tfs(file, type, name, tfsoff);
    }
    else if (is_union(type->type))
    {
        if (!processed(type))
            write_union_tfs(file, type, tfsoff);
    }
    else if (!is_base_type(type->type))
        error("write_embedded_types: unknown embedded type for %s (0x%x)\n",
              name, type->type);

    return retmask;
}

static void set_all_tfswrite(const ifref_list_t *ifaces, int val)
{
    const ifref_t * iface;
    const func_t *func;
    const var_t *var;

    if (ifaces)
        LIST_FOR_EACH_ENTRY( iface, ifaces, const ifref_t, entry )
            if (iface->iface->funcs)
                LIST_FOR_EACH_ENTRY( func, iface->iface->funcs, const func_t, entry )
                    if (func->args)
                        LIST_FOR_EACH_ENTRY( var, func->args, const var_t, entry )
                            set_tfswrite(var->type, val);
}

static size_t process_tfs(FILE *file, const ifref_list_t *ifaces, int for_objects)
{
    const var_t *var;
    const ifref_t *iface;
    unsigned int typeformat_offset = 2;

    if (ifaces) LIST_FOR_EACH_ENTRY( iface, ifaces, const ifref_t, entry )
    {
        if (for_objects != is_object(iface->iface->attrs) || is_local(iface->iface->attrs))
            continue;

        if (iface->iface->funcs)
        {
            const func_t *func;
            LIST_FOR_EACH_ENTRY( func, iface->iface->funcs, const func_t, entry )
            {
                if (is_local(func->def->attrs)) continue;

                current_func = func;
                if (func->args)
                    LIST_FOR_EACH_ENTRY( var, func->args, const var_t, entry )
                        update_tfsoff(
                            var->type,
                            write_typeformatstring_var(
                                file, 2, func, var->type, var,
                                &typeformat_offset),
                            file);
            }
        }
    }

    return typeformat_offset + 1;
}


void write_typeformatstring(FILE *file, const ifref_list_t *ifaces, int for_objects)
{
    int indent = 0;

    print_file(file, indent, "static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =\n");
    print_file(file, indent, "{\n");
    indent++;
    print_file(file, indent, "0,\n");
    print_file(file, indent, "{\n");
    indent++;
    print_file(file, indent, "NdrFcShort(0x0),\n");

    set_all_tfswrite(ifaces, TRUE);
    process_tfs(file, ifaces, for_objects);

    print_file(file, indent, "0x0\n");
    indent--;
    print_file(file, indent, "}\n");
    indent--;
    print_file(file, indent, "};\n");
    print_file(file, indent, "\n");
}

static unsigned int get_required_buffer_size_type(
    const type_t *type, const char *name, unsigned int *alignment)
{
    size_t size = 0;

    *alignment = 0;
    if (is_user_type(type))
    {
        const char *uname;
        const type_t *utype = get_user_type(type, &uname);
        size = get_required_buffer_size_type(utype, uname, alignment);
    }
    else if (!is_ptr(type))
    {
        switch (type->type)
        {
        case RPC_FC_BYTE:
        case RPC_FC_CHAR:
        case RPC_FC_USMALL:
        case RPC_FC_SMALL:
            *alignment = 4;
            size = 1;
            break;

        case RPC_FC_WCHAR:
        case RPC_FC_USHORT:
        case RPC_FC_SHORT:
        case RPC_FC_ENUM16:
            *alignment = 4;
            size = 2;
            break;

        case RPC_FC_ULONG:
        case RPC_FC_LONG:
        case RPC_FC_ENUM32:
        case RPC_FC_FLOAT:
        case RPC_FC_ERROR_STATUS_T:
            *alignment = 4;
            size = 4;
            break;

        case RPC_FC_HYPER:
        case RPC_FC_DOUBLE:
            *alignment = 8;
            size = 8;
            break;

        case RPC_FC_IGNORE:
        case RPC_FC_BIND_PRIMITIVE:
            return 0;

        case RPC_FC_STRUCT:
        case RPC_FC_PSTRUCT:
        {
            const var_t *field;
            if (!type->fields) return 0;
            LIST_FOR_EACH_ENTRY( field, type->fields, const var_t, entry )
            {
                unsigned int alignment;
                size += get_required_buffer_size_type(field->type, field->name,
                                                      &alignment);
            }
            break;
        }

        case RPC_FC_RP:
            if (is_base_type( type->ref->type ) || type->ref->type == RPC_FC_STRUCT)
                size = get_required_buffer_size_type( type->ref, name, alignment );
            break;

        case RPC_FC_SMFARRAY:
        case RPC_FC_LGFARRAY:
            size = type->dim * get_required_buffer_size_type(type->ref, name, alignment);
            break;

        case RPC_FC_SMVARRAY:
        case RPC_FC_LGVARRAY:
            get_required_buffer_size_type(type->ref, name, alignment);
            size = 0;
            break;

        case RPC_FC_CARRAY:
        case RPC_FC_CVARRAY:
            get_required_buffer_size_type(type->ref, name, alignment);
            size = sizeof(void *);
            break;

        default:
            error("get_required_buffer_size: Unknown/unsupported type: %s (0x%02x)\n", name, type->type);
            return 0;
        }
    }
    return size;
}

static unsigned int get_required_buffer_size(const var_t *var, unsigned int *alignment, enum pass pass)
{
    int in_attr = is_attr(var->attrs, ATTR_IN);
    int out_attr = is_attr(var->attrs, ATTR_OUT);

    if (!in_attr && !out_attr)
        in_attr = 1;

    *alignment = 0;

    if (pass == PASS_OUT)
    {
        if (out_attr && is_ptr(var->type))
        {
            type_t *type = var->type;

            if (type->type == RPC_FC_STRUCT)
            {
                const var_t *field;
                unsigned int size = 36;

                if (!type->fields) return size;
                LIST_FOR_EACH_ENTRY( field, type->fields, const var_t, entry )
                {
                    unsigned int align;
                    size += get_required_buffer_size_type(
                        field->type, field->name, &align);
                }
                return size;
            }
        }
        return 0;
    }
    else
    {
        if ((!out_attr || in_attr) && !var->type->size_is
            && !is_attr(var->attrs, ATTR_STRING) && !var->type->declarray)
        {
            if (is_ptr(var->type))
            {
                type_t *type = var->type;

                if (is_base_type(type->type))
                {
                    return 25;
                }
                else if (type->type == RPC_FC_STRUCT)
                {
                    unsigned int size = 36;
                    const var_t *field;

                    if (!type->fields) return size;
                    LIST_FOR_EACH_ENTRY( field, type->fields, const var_t, entry )
                    {
                        unsigned int align;
                        size += get_required_buffer_size_type(
                            field->type, field->name, &align);
                    }
                    return size;
                }
            }
        }

        return get_required_buffer_size_type(var->type, var->name, alignment);
    }
}

static unsigned int get_function_buffer_size( const func_t *func, enum pass pass )
{
    const var_t *var;
    unsigned int total_size = 0, alignment;

    if (func->args)
    {
        LIST_FOR_EACH_ENTRY( var, func->args, const var_t, entry )
        {
            total_size += get_required_buffer_size(var, &alignment, pass);
            total_size += alignment;
        }
    }

    if (pass == PASS_OUT && !is_void(func->def->type))
    {
        total_size += get_required_buffer_size(func->def, &alignment, PASS_RETURN);
        total_size += alignment;
    }
    return total_size;
}

static void print_phase_function(FILE *file, int indent, const char *type,
                                 enum remoting_phase phase,
                                 const var_t *var, unsigned int type_offset)
{
    const char *function;
    switch (phase)
    {
    case PHASE_BUFFERSIZE:
        function = "BufferSize";
        break;
    case PHASE_MARSHAL:
        function = "Marshall";
        break;
    case PHASE_UNMARSHAL:
        function = "Unmarshall";
        break;
    case PHASE_FREE:
        function = "Free";
        break;
    default:
        assert(0);
        return;
    }

    print_file(file, indent, "Ndr%s%s(\n", type, function);
    indent++;
    print_file(file, indent, "&_StubMsg,\n");
    print_file(file, indent, "%s%s%s%s,\n",
               (phase == PHASE_UNMARSHAL) ? "(unsigned char **)" : "(unsigned char *)",
               (phase == PHASE_UNMARSHAL || decl_indirect(var->type)) ? "&" : "",
               (phase == PHASE_UNMARSHAL && decl_indirect(var->type)) ? "_p_" : "",
               var->name);
    print_file(file, indent, "(PFORMAT_STRING)&__MIDL_TypeFormatString.Format[%d]%s\n",
               type_offset, (phase == PHASE_UNMARSHAL) ? "," : ");");
    if (phase == PHASE_UNMARSHAL)
        print_file(file, indent, "0);\n");
    indent--;
}

void print_phase_basetype(FILE *file, int indent, enum remoting_phase phase,
                          enum pass pass, const var_t *var,
                          const char *varname)
{
    type_t *type = var->type;
    unsigned int size;
    unsigned int alignment = 0;
    unsigned char rtype;

    /* no work to do for other phases, buffer sizing is done elsewhere */
    if (phase != PHASE_MARSHAL && phase != PHASE_UNMARSHAL)
        return;

    rtype = is_ptr(type) ? type->ref->type : type->type;

    switch (rtype)
    {
        case RPC_FC_BYTE:
        case RPC_FC_CHAR:
        case RPC_FC_SMALL:
        case RPC_FC_USMALL:
            size = 1;
            alignment = 1;
            break;

        case RPC_FC_WCHAR:
        case RPC_FC_USHORT:
        case RPC_FC_SHORT:
        case RPC_FC_ENUM16:
            size = 2;
            alignment = 2;
            break;

        case RPC_FC_ULONG:
        case RPC_FC_LONG:
        case RPC_FC_ENUM32:
        case RPC_FC_FLOAT:
        case RPC_FC_ERROR_STATUS_T:
            size = 4;
            alignment = 4;
            break;

        case RPC_FC_HYPER:
        case RPC_FC_DOUBLE:
            size = 8;
            alignment = 8;
            break;

        case RPC_FC_IGNORE:
        case RPC_FC_BIND_PRIMITIVE:
            /* no marshalling needed */
            return;

        default:
            error("print_phase_basetype: Unsupported type: %s (0x%02x, ptr_level: 0)\n", var->name, rtype);
            size = 0;
    }

    print_file(file, indent, "_StubMsg.Buffer = (unsigned char *)(((long)_StubMsg.Buffer + %u) & ~0x%x);\n",
                alignment - 1, alignment - 1);

    if (phase == PHASE_MARSHAL)
    {
        print_file(file, indent, "*(");
        write_type(file, is_ptr(type) ? type->ref : type, FALSE, NULL);
        if (is_ptr(type))
            fprintf(file, " *)_StubMsg.Buffer = *");
        else
            fprintf(file, " *)_StubMsg.Buffer = ");
        fprintf(file, varname);
        fprintf(file, ";\n");
    }
    else if (phase == PHASE_UNMARSHAL)
    {
        if (pass == PASS_IN || pass == PASS_RETURN)
            print_file(file, indent, "");
        else
            print_file(file, indent, "*");
        fprintf(file, varname);
        if (pass == PASS_IN && is_ptr(type))
            fprintf(file, " = (");
        else
            fprintf(file, " = *(");
        write_type(file, is_ptr(type) ? type->ref : type, FALSE, NULL);
        fprintf(file, " *)_StubMsg.Buffer;\n");
    }

    print_file(file, indent, "_StubMsg.Buffer += sizeof(");
    write_type(file, var->type, FALSE, NULL);
    fprintf(file, ");\n");
}

/* returns whether the MaxCount, Offset or ActualCount members need to be
 * filled in for the specified phase */
static inline int is_size_needed_for_phase(enum remoting_phase phase)
{
    return (phase != PHASE_UNMARSHAL);
}

void write_remoting_arguments(FILE *file, int indent, const func_t *func,
                              enum pass pass, enum remoting_phase phase)
{
    int in_attr, out_attr, pointer_type;
    const var_t *var;

    if (!func->args)
        return;

    if (phase == PHASE_BUFFERSIZE)
    {
        unsigned int size = get_function_buffer_size( func, pass );
        print_file(file, indent, "_StubMsg.BufferLength = %u;\n", size);
    }

    LIST_FOR_EACH_ENTRY( var, func->args, const var_t, entry )
    {
        const type_t *type = var->type;
        unsigned char rtype;
        size_t start_offset = type->typestring_offset;

        pointer_type = get_attrv(var->attrs, ATTR_POINTERTYPE);
        if (!pointer_type)
            pointer_type = RPC_FC_RP;

        in_attr = is_attr(var->attrs, ATTR_IN);
        out_attr = is_attr(var->attrs, ATTR_OUT);
        if (!in_attr && !out_attr)
            in_attr = 1;

        switch (pass)
        {
        case PASS_IN:
            if (!in_attr) continue;
            break;
        case PASS_OUT:
            if (!out_attr) continue;
            break;
        case PASS_RETURN:
            break;
        }

        rtype = type->type;

        if (is_user_type(var->type))
        {
            print_phase_function(file, indent, "UserMarshal", phase, var, start_offset);
        }
        else if (is_string_type(var->attrs, var->type))
        {
            if (is_array(type) && !is_conformant_array(type))
                print_phase_function(file, indent, "NonConformantString", phase, var, start_offset);
            else
            {
                if (type->size_is && is_size_needed_for_phase(phase))
                {
                    print_file(file, indent, "_StubMsg.MaxCount = (unsigned long)");
                    write_expr(file, type->size_is, 1);
                    fprintf(file, ";\n");
                }

                if ((phase == PHASE_FREE) || (pointer_type == RPC_FC_UP))
                    print_phase_function(file, indent, "Pointer", phase, var, start_offset);
                else
                    print_phase_function(file, indent, "ConformantString", phase, var,
                                         start_offset + (type->size_is ? 4 : 2));
            }
        }
        else if (is_array(type))
        {
            unsigned char tc = type->type;
            const char *array_type;

            if (tc == RPC_FC_SMFARRAY || tc == RPC_FC_LGFARRAY)
                array_type = "FixedArray";
            else if (tc == RPC_FC_SMVARRAY || tc == RPC_FC_LGVARRAY)
            {
                if (is_size_needed_for_phase(phase))
                {
                    print_file(file, indent, "_StubMsg.Offset = (unsigned long)0;\n"); /* FIXME */
                    print_file(file, indent, "_StubMsg.ActualCount = (unsigned long)");
                    write_expr(file, type->length_is, 1);
                    fprintf(file, ";\n\n");
                }
                array_type = "VaryingArray";
            }
            else if (tc == RPC_FC_CARRAY)
            {
                if (is_size_needed_for_phase(phase) && phase != PHASE_FREE)
                {
                    print_file(file, indent, "_StubMsg.MaxCount = (unsigned long)");
                    write_expr(file, type->size_is, 1);
                    fprintf(file, ";\n\n");
                }
                array_type = "ConformantArray";
            }
            else if (tc == RPC_FC_CVARRAY)
            {
                if (is_size_needed_for_phase(phase))
                {
                    print_file(file, indent, "_StubMsg.MaxCount = (unsigned long)");
                    write_expr(file, type->size_is, 1);
                    fprintf(file, ";\n");
                    print_file(file, indent, "_StubMsg.Offset = (unsigned long)0;\n"); /* FIXME */
                    print_file(file, indent, "_StubMsg.ActualCount = (unsigned long)");
                    write_expr(file, type->length_is, 1);
                    fprintf(file, ";\n\n");
                }
                array_type = "ConformantVaryingArray";
            }
            else
                array_type = "ComplexArray";

            if (!in_attr && phase == PHASE_FREE)
            {
                print_file(file, indent, "if (%s)\n", var->name);
                indent++;
                print_file(file, indent, "_StubMsg.pfnFree(%s);\n", var->name);
            }
            else if (phase != PHASE_FREE)
            {
                if (pointer_type == RPC_FC_UP)
                    print_phase_function(file, indent, "Pointer", phase, var, start_offset);
                else
                    print_phase_function(file, indent, array_type, phase, var, start_offset);
            }
        }
        else if (!is_ptr(var->type) && is_base_type(rtype))
        {
            print_phase_basetype(file, indent, phase, pass, var, var->name);
        }
        else if (!is_ptr(var->type))
        {
            switch (rtype)
            {
            case RPC_FC_STRUCT:
            case RPC_FC_PSTRUCT:
                print_phase_function(file, indent, "SimpleStruct", phase, var, start_offset);
                break;
            case RPC_FC_CSTRUCT:
            case RPC_FC_CPSTRUCT:
                print_phase_function(file, indent, "ConformantStruct", phase, var, start_offset);
                break;
            case RPC_FC_CVSTRUCT:
                print_phase_function(file, indent, "ConformantVaryingStruct", phase, var, start_offset);
                break;
            case RPC_FC_BOGUS_STRUCT:
                print_phase_function(file, indent, "ComplexStruct", phase, var, start_offset);
                break;
            case RPC_FC_RP:
                if (is_base_type( var->type->ref->type ))
                {
                    print_phase_basetype(file, indent, phase, pass, var, var->name);
                }
                else if (var->type->ref->type == RPC_FC_STRUCT)
                {
                    if (phase != PHASE_BUFFERSIZE && phase != PHASE_FREE)
                        print_phase_function(file, indent, "SimpleStruct", phase, var, start_offset + 4);
                }
                else
                {
                    const var_t *iid;
                    if ((iid = get_attrp( var->attrs, ATTR_IIDIS )))
                        print_file( file, indent, "_StubMsg.MaxCount = (unsigned long)%s;\n", iid->name );
                    print_phase_function(file, indent, "Pointer", phase, var, start_offset);
                }
                break;
            default:
                error("write_remoting_arguments: Unsupported type: %s (0x%02x)\n", var->name, rtype);
            }
        }
        else
        {
            if (last_ptr(var->type) && (pointer_type == RPC_FC_RP) && is_base_type(rtype))
            {
                print_phase_basetype(file, indent, phase, pass, var, var->name);
            }
            else if (last_ptr(var->type) && (pointer_type == RPC_FC_RP) && (rtype == RPC_FC_STRUCT))
            {
                if (phase != PHASE_BUFFERSIZE && phase != PHASE_FREE)
                    print_phase_function(file, indent, "SimpleStruct", phase, var, start_offset + 4);
            }
            else
            {
                const var_t *iid;
                if ((iid = get_attrp( var->attrs, ATTR_IIDIS )))
                    print_file( file, indent, "_StubMsg.MaxCount = (unsigned long)%s;\n", iid->name );
                print_phase_function(file, indent, "Pointer", phase, var, start_offset);
            }
        }
        fprintf(file, "\n");
    }
}


size_t get_size_procformatstring_var(const var_t *var)
{
    return write_procformatstring_var(NULL, 0, var, FALSE);
}


size_t get_size_procformatstring_func(const func_t *func)
{
    const var_t *var;
    size_t size = 0;

    /* argument list size */
    if (func->args)
        LIST_FOR_EACH_ENTRY( var, func->args, const var_t, entry )
            size += get_size_procformatstring_var(var);

    /* return value size */
    if (is_void(func->def->type))
        size += 2; /* FC_END and FC_PAD */
    else
        size += get_size_procformatstring_var(func->def);

    return size;
}

size_t get_size_procformatstring(const ifref_list_t *ifaces, int for_objects)
{
    const ifref_t *iface;
    size_t size = 1;
    const func_t *func;

    if (ifaces) LIST_FOR_EACH_ENTRY( iface, ifaces, const ifref_t, entry )
    {
        if (for_objects != is_object(iface->iface->attrs) || is_local(iface->iface->attrs))
            continue;

        if (iface->iface->funcs)
            LIST_FOR_EACH_ENTRY( func, iface->iface->funcs, const func_t, entry )
                if (!is_local(func->def->attrs))
                    size += get_size_procformatstring_func( func );
    }
    return size;
}

size_t get_size_typeformatstring(const ifref_list_t *ifaces, int for_objects)
{
    set_all_tfswrite(ifaces, FALSE);
    return process_tfs(NULL, ifaces, for_objects);
}

static void write_struct_expr(FILE *h, const expr_t *e, int brackets,
                              const var_list_t *fields, const char *structvar)
{
    switch (e->type) {
        case EXPR_VOID:
            break;
        case EXPR_NUM:
            fprintf(h, "%lu", e->u.lval);
            break;
        case EXPR_HEXNUM:
            fprintf(h, "0x%lx", e->u.lval);
            break;
        case EXPR_DOUBLE:
            fprintf(h, "%#.15g", e->u.dval);
            break;
        case EXPR_TRUEFALSE:
            if (e->u.lval == 0)
                fprintf(h, "FALSE");
            else
                fprintf(h, "TRUE");
            break;
        case EXPR_IDENTIFIER:
        {
            const var_t *field;
            LIST_FOR_EACH_ENTRY( field, fields, const var_t, entry )
                if (!strcmp(e->u.sval, field->name))
                {
                    fprintf(h, "%s->%s", structvar, e->u.sval);
                    break;
                }

            if (&field->entry == fields) error("no field found for identifier %s\n", e->u.sval);
            break;
        }
        case EXPR_NEG:
            fprintf(h, "-");
            write_struct_expr(h, e->ref, 1, fields, structvar);
            break;
        case EXPR_NOT:
            fprintf(h, "~");
            write_struct_expr(h, e->ref, 1, fields, structvar);
            break;
        case EXPR_PPTR:
            fprintf(h, "*");
            write_struct_expr(h, e->ref, 1, fields, structvar);
            break;
        case EXPR_CAST:
            fprintf(h, "(");
            write_type(h, e->u.tref, FALSE, NULL);
            fprintf(h, ")");
            write_struct_expr(h, e->ref, 1, fields, structvar);
            break;
        case EXPR_SIZEOF:
            fprintf(h, "sizeof(");
            write_type(h, e->u.tref, FALSE, NULL);
            fprintf(h, ")");
            break;
        case EXPR_SHL:
        case EXPR_SHR:
        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_ADD:
        case EXPR_SUB:
        case EXPR_AND:
        case EXPR_OR:
            if (brackets) fprintf(h, "(");
            write_struct_expr(h, e->ref, 1, fields, structvar);
            switch (e->type) {
                case EXPR_SHL: fprintf(h, " << "); break;
                case EXPR_SHR: fprintf(h, " >> "); break;
                case EXPR_MUL: fprintf(h, " * "); break;
                case EXPR_DIV: fprintf(h, " / "); break;
                case EXPR_ADD: fprintf(h, " + "); break;
                case EXPR_SUB: fprintf(h, " - "); break;
                case EXPR_AND: fprintf(h, " & "); break;
                case EXPR_OR:  fprintf(h, " | "); break;
                default: break;
            }
            write_struct_expr(h, e->u.ext, 1, fields, structvar);
            if (brackets) fprintf(h, ")");
            break;
        case EXPR_COND:
            if (brackets) fprintf(h, "(");
            write_struct_expr(h, e->ref, 1, fields, structvar);
            fprintf(h, " ? ");
            write_struct_expr(h, e->u.ext, 1, fields, structvar);
            fprintf(h, " : ");
            write_struct_expr(h, e->ext2, 1, fields, structvar);
            if (brackets) fprintf(h, ")");
            break;
    }
}


void declare_stub_args( FILE *file, int indent, const func_t *func )
{
    int in_attr, out_attr;
    int i = 0;
    const var_t *def = func->def;
    const var_t *var;

    /* declare return value '_RetVal' */
    if (!is_void(def->type))
    {
        print_file(file, indent, "");
        write_type_left(file, def->type);
        fprintf(file, " _RetVal;\n");
    }

    if (!func->args)
        return;

    LIST_FOR_EACH_ENTRY( var, func->args, const var_t, entry )
    {
        int is_string = is_attr(var->attrs, ATTR_STRING);

        in_attr = is_attr(var->attrs, ATTR_IN);
        out_attr = is_attr(var->attrs, ATTR_OUT);
        if (!out_attr && !in_attr)
            in_attr = 1;

        if (!in_attr && !var->type->size_is && !is_string)
        {
            print_file(file, indent, "");
            write_type(file, var->type->ref, FALSE, "_W%u", i++);
            fprintf(file, ";\n");
        }

        print_file(file, indent, "");
        write_type_left(file, var->type);
        fprintf(file, " ");
        if (var->type->declarray) {
            fprintf(file, "( *");
            write_name(file, var);
            fprintf(file, " )");
        } else
            write_name(file, var);
        write_type_right(file, var->type, FALSE);
        fprintf(file, ";\n");

        if (decl_indirect(var->type))
            print_file(file, indent, "void *_p_%s = &%s;\n",
                       var->name, var->name);
    }
}


void assign_stub_out_args( FILE *file, int indent, const func_t *func )
{
    int in_attr, out_attr;
    int i = 0, sep = 0;
    const var_t *var;

    if (!func->args)
        return;

    LIST_FOR_EACH_ENTRY( var, func->args, const var_t, entry )
    {
        int is_string = is_attr(var->attrs, ATTR_STRING);
        in_attr = is_attr(var->attrs, ATTR_IN);
        out_attr = is_attr(var->attrs, ATTR_OUT);
        if (!out_attr && !in_attr)
            in_attr = 1;

        if (!in_attr)
        {
            print_file(file, indent, "");
            write_name(file, var);

            if (var->type->size_is)
            {
                unsigned int size, align = 0;
                type_t *type = var->type;

                fprintf(file, " = NdrAllocate(&_StubMsg, ");
                for ( ; type->size_is ; type = type->ref)
                {
                    write_expr(file, type->size_is, TRUE);
                    fprintf(file, " * ");
                }
                size = type_memsize(type, &align);
                fprintf(file, "%u);\n", size);
            }
            else if (!is_string)
            {
                fprintf(file, " = &_W%u;\n", i);
                if (is_ptr(var->type) && !last_ptr(var->type))
                    print_file(file, indent, "_W%u = 0;\n", i);
                i++;
            }

            sep = 1;
        }
    }
    if (sep)
        fprintf(file, "\n");
}


int write_expr_eval_routines(FILE *file, const char *iface)
{
    static const char *var_name = "pS";
    int result = 0;
    struct expr_eval_routine *eval;
    unsigned short callback_offset = 0;

    LIST_FOR_EACH_ENTRY(eval, &expr_eval_routines, struct expr_eval_routine, entry)
    {
        const char *name = eval->structure->name;
        const var_list_t *fields = eval->structure->fields;
        unsigned align = 0;
        result = 1;
        print_file(file, 0, "static void __RPC_USER %s_%sExprEval_%04u(PMIDL_STUB_MESSAGE pStubMsg)\n",
                   iface, name, callback_offset);
        print_file(file, 0, "{\n");
        print_file (file, 1, "%s *%s = (%s *)(pStubMsg->StackTop - %u);\n",
                    name, var_name, name, type_memsize (eval->structure, &align));
        print_file(file, 1, "pStubMsg->Offset = 0;\n"); /* FIXME */
        print_file(file, 1, "pStubMsg->MaxCount = (unsigned long)");
        write_struct_expr(file, eval->expr, 1, fields, var_name);
        fprintf(file, ";\n");
        print_file(file, 0, "}\n\n");
        callback_offset++;
    }
    return result;
}

void write_expr_eval_routine_list(FILE *file, const char *iface)
{
    struct expr_eval_routine *eval;
    struct expr_eval_routine *cursor;
    unsigned short callback_offset = 0;

    fprintf(file, "static const EXPR_EVAL ExprEvalRoutines[] =\n");
    fprintf(file, "{\n");

    LIST_FOR_EACH_ENTRY_SAFE(eval, cursor, &expr_eval_routines, struct expr_eval_routine, entry)
    {
        const char *name = eval->structure->name;
        print_file(file, 1, "%s_%sExprEval_%04u,\n", iface, name, callback_offset);
        callback_offset++;
        list_remove(&eval->entry);
        free(eval);
    }

    fprintf(file, "};\n\n");
}

void write_user_quad_list(FILE *file)
{
    user_type_t *ut;

    if (list_empty(&user_type_list))
        return;

    fprintf(file, "static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[] =\n");
    fprintf(file, "{\n");
    LIST_FOR_EACH_ENTRY(ut, &user_type_list, user_type_t, entry)
    {
        const char *sep = &ut->entry == list_tail(&user_type_list) ? "" : ",";
        print_file(file, 1, "{\n");
        print_file(file, 2, "(USER_MARSHAL_SIZING_ROUTINE)%s_UserSize,\n", ut->name);
        print_file(file, 2, "(USER_MARSHAL_MARSHALLING_ROUTINE)%s_UserMarshal,\n", ut->name);
        print_file(file, 2, "(USER_MARSHAL_UNMARSHALLING_ROUTINE)%s_UserUnmarshal,\n", ut->name);
        print_file(file, 2, "(USER_MARSHAL_FREEING_ROUTINE)%s_UserFree\n", ut->name);
        print_file(file, 1, "}%s\n", sep);
    }
    fprintf(file, "};\n\n");
}

void write_endpoints( FILE *f, const char *prefix, const str_list_t *list )
{
    const struct str_list_entry_t *endpoint;
    const char *p;

    /* this should be an array of RPC_PROTSEQ_ENDPOINT but we want const strings */
    print_file( f, 0, "static const unsigned char * %s__RpcProtseqEndpoint[][2] =\n{\n", prefix );
    LIST_FOR_EACH_ENTRY( endpoint, list, const struct str_list_entry_t, entry )
    {
        print_file( f, 1, "{ (const unsigned char *)\"" );
        for (p = endpoint->str; *p && *p != ':'; p++)
        {
            if (*p == '"' || *p == '\\') fputc( '\\', f );
            fputc( *p, f );
        }
        if (!*p) goto error;
        if (p[1] != '[') goto error;

        fprintf( f, "\", (const unsigned char *)\"" );
        for (p += 2; *p && *p != ']'; p++)
        {
            if (*p == '"' || *p == '\\') fputc( '\\', f );
            fputc( *p, f );
        }
        if (*p != ']') goto error;
        fprintf( f, "\" },\n" );
    }
    print_file( f, 0, "};\n\n" );
    return;

error:
    error("Invalid endpoint syntax '%s'\n", endpoint->str);
}
