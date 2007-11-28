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
#include "wine/list.h"

#include "widl.h"
#include "typegen.h"

static const func_t *current_func;
static const type_t *current_structure;

/* name of the structure variable for structure callbacks */
#define STRUCT_EXPR_EVAL_VAR "pS"

static struct list expr_eval_routines = LIST_INIT(expr_eval_routines);

struct expr_eval_routine
{
    struct list entry;
    const type_t *structure;
    size_t structure_size;
    const expr_t *expr;
};

static size_t type_memsize(const type_t *t, int ptr_level, const expr_t *array);
static size_t fields_memsize(const var_t *v);

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

static int print_file(FILE *file, int indent, const char *format, ...)
{
    va_list va;
    int i, r;

    if (!file) return 0;

    va_start(va, format);
    for (i = 0; i < indent; i++)
        fprintf(file, "    ");
    r = vfprintf(file, format, va);
    va_end(va);
    return r;
}

static inline int type_has_ref(const type_t *type)
{
    return (type->type == 0 && type->ref);
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

static size_t write_procformatstring_var(FILE *file, int indent,
    const var_t *var, int is_return, unsigned int *type_offset)
{
    size_t size;
    int ptr_level = var->ptr_level;
    const type_t *type = var->type;

    int is_in = is_attr(var->attrs, ATTR_IN);
    int is_out = is_attr(var->attrs, ATTR_OUT);

    if (!is_in && !is_out) is_in = TRUE;

    if (ptr_level == 0 && type_has_ref(type))
        type = type->ref;

    if (ptr_level == 0 && !var->array && is_base_type(type->type))
    {
        if (is_return)
            print_file(file, indent, "0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
        else
            print_file(file, indent, "0x4e,    /* FC_IN_PARAM_BASETYPE */\n");

        switch(type->type)
        {
#define CASE_BASETYPE(fctype) \
        case RPC_##fctype: \
            print_file(file, indent, "0x%02x,    /* " #fctype " */\n", RPC_##fctype); \
            size = 2; /* includes param type prefix */ \
            break

        CASE_BASETYPE(FC_BYTE);
        CASE_BASETYPE(FC_CHAR);
        CASE_BASETYPE(FC_WCHAR);
        CASE_BASETYPE(FC_USHORT);
        CASE_BASETYPE(FC_SHORT);
        CASE_BASETYPE(FC_ULONG);
        CASE_BASETYPE(FC_LONG);
        CASE_BASETYPE(FC_HYPER);
        CASE_BASETYPE(FC_IGNORE);
        CASE_BASETYPE(FC_USMALL);
        CASE_BASETYPE(FC_SMALL);
        CASE_BASETYPE(FC_FLOAT);
        CASE_BASETYPE(FC_DOUBLE);
        CASE_BASETYPE(FC_ERROR_STATUS_T);
#undef CASE_BASETYPE

        case RPC_FC_BIND_PRIMITIVE:
            print_file(file, indent, "0x%02x,    /* FC_IGNORE */\n", RPC_FC_IGNORE);
            size = 2; /* includes param type prefix */
            break;

        default:
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
        print_file(file, indent, "NdrFcShort(0x%x),\n", *type_offset);
        size = 4; /* includes param type prefix */
    }
    *type_offset += get_size_typeformatstring_var(var);
    return size;
}

void write_procformatstring(FILE *file, const ifref_t *ifaces)
{
    const ifref_t *iface = ifaces;
    int indent = 0;
    var_t *var;
    unsigned int type_offset = 2;

    print_file(file, indent, "static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =\n");
    print_file(file, indent, "{\n");
    indent++;
    print_file(file, indent, "0,\n");
    print_file(file, indent, "{\n");
    indent++;

    END_OF_LIST(iface);

    for (; iface; iface = PREV_LINK(iface))
    {
        if (is_object(iface->iface->attrs) || is_local(iface->iface->attrs))
            continue;

        if (iface->iface->funcs)
        {
            func_t *func = iface->iface->funcs;
            while (NEXT_LINK(func)) func = NEXT_LINK(func);
            for (; func; func = PREV_LINK(func))
            {
                /* emit argument data */
                if (func->args)
                {
                    var = func->args;
                    while (NEXT_LINK(var)) var = NEXT_LINK(var);
                    while (var)
                    {
                        write_procformatstring_var(file, indent, var, FALSE,
                                                   &type_offset);

                        var = PREV_LINK(var);
                    }
                }

                /* emit return value data */
                var = func->def;
                if (is_void(var->type, NULL))
                {
                    print_file(file, indent, "0x5b,    /* FC_END */\n");
                    print_file(file, indent, "0x5c,    /* FC_PAD */\n");
                }
                else
                    write_procformatstring_var(file, indent, var, TRUE,
                                               &type_offset);
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

/* write conformance / variance descriptor */
static size_t write_conf_or_var_desc(FILE *file, const func_t *func, const type_t *structure, const expr_t *expr)
{
    unsigned char operator_type = 0;
    const char *operator_string = "no operators";
    const expr_t *subexpr = expr;
    unsigned char correlation_type;

    if (!file) return 4; /* optimisation for sizing pass */

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
        unsigned char param_type = 0;
        const char *param_type_string = NULL;
        size_t offset;

        if (structure)
        {
            const var_t *var;

            for (offset = 0, var = structure->fields; var; var = NEXT_LINK(var))
            {
                offset -= type_memsize(var->type, var->ptr_level, var->array);
                if (!strcmp(var->name, subexpr->u.sval))
                {
                    correlation_variable = var->type;
                    break;
                }
            }
            if (!correlation_variable)
                error("write_conf_or_var_desc: couldn't find variable %s in structure\n",
                      subexpr->u.sval);

            correlation_type = RPC_FC_NORMAL_CONFORMANCE;
        }
        else
        {
            const var_t *var = func->args;

            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            /* FIXME: not all stack variables are sizeof(void *) */
            for (offset = 0; var; offset += sizeof(void *), var = PREV_LINK(var))
            {
                if (!strcmp(var->name, subexpr->u.sval))
                {
                    correlation_variable = var->type;
                    break;
                }
            }
            if (!correlation_variable)
                error("write_conf_or_var_desc: couldn't find variable %s in function\n",
                    subexpr->u.sval);

            correlation_type = RPC_FC_TOP_LEVEL_CONFORMANCE;
        }

        while (type_has_ref(correlation_variable))
            correlation_variable = correlation_variable->ref;

        switch (correlation_variable->type)
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
            param_type = RPC_FC_SHORT;
            param_type_string = "FC_SHORT";
            break;
        case RPC_FC_USHORT:
            param_type = RPC_FC_USHORT;
            param_type_string = "FC_USHORT";
            break;
        case RPC_FC_LONG:
            param_type = RPC_FC_LONG;
            param_type_string = "FC_LONG";
            break;
        case RPC_FC_ULONG:
            param_type = RPC_FC_ULONG;
            param_type_string = "FC_ULONG";
            break;
        default:
            error("write_conf_or_var_desc: conformance variable type not supported 0x%x\n",
                correlation_variable->type);
        }

        print_file(file, 2, "0x%x, /* Corr desc: %s%s */\n",
                correlation_type | param_type,
                correlation_type == RPC_FC_TOP_LEVEL_CONFORMANCE ? "parameter, " : "",
                param_type_string);
        print_file(file, 2, "0x%x, /* %s */\n", operator_type, operator_string);
        print_file(file, 2, "NdrFcShort(0x%x), /* %soffset = %d */\n",
                   offset,
                   correlation_type == RPC_FC_TOP_LEVEL_CONFORMANCE ? "x86 stack size / " : "",
                   offset);
    }
    else
    {
        unsigned int callback_offset = 0;

        if (structure)
        {
            struct expr_eval_routine *eval;
            int found = 0;

            LIST_FOR_EACH_ENTRY(eval, &expr_eval_routines, struct expr_eval_routine, entry)
            {
                if (!strcmp(eval->structure->name, structure->name) &&
                    !compare_expr(eval->expr, expr))
                {
                    found = 1;
                    break;
                }
                callback_offset++;
            }

            if (!found)
            {
                eval = xmalloc(sizeof(*eval));
                eval->structure = structure;
                eval->structure_size = fields_memsize(structure->fields);
                eval->expr = expr;
                list_add_tail(&expr_eval_routines, &eval->entry);
            }

            correlation_type = RPC_FC_NORMAL_CONFORMANCE;
        }
        else
        {
            error("write_conf_or_var_desc: top-level callback conformance unimplemented\n");
            correlation_type = RPC_FC_TOP_LEVEL_CONFORMANCE;
        }

        if (callback_offset > USHRT_MAX)
            error("Maximum number of callback routines reached\n");

        print_file(file, 2, "0x%x, /* Corr desc: %s */\n",
                   correlation_type,
                   correlation_type == RPC_FC_TOP_LEVEL_CONFORMANCE ? "parameter" : "");
        print_file(file, 2, "0x%x, /* %s */\n", RPC_FC_CALLBACK, "FC_CALLBACK");
        print_file(file, 2, "NdrFcShort(0x%x), /* %u */\n", callback_offset, callback_offset);
    }
    return 4;
}

static size_t fields_memsize(const var_t *v)
{
    size_t size = 0;
    const var_t *first = v;
    if (!v) return 0;
    while (NEXT_LINK(v)) v = NEXT_LINK(v);
    while (v) {
        size += type_memsize(v->type, v->ptr_level, v->array);
        if (v == first) break;
        v = PREV_LINK(v);
    }
    return size;
}

static size_t type_memsize(const type_t *t, int ptr_level, const expr_t *array)
{
    size_t size = 0;

    if (ptr_level)
        return sizeof(void *);

    if (type_has_ref(t))
        return type_memsize(t->ref, 0 /* FIXME */, NULL);

    switch (t->type)
    {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_USMALL:
    case RPC_FC_SMALL:
        size = 1;
        break;
    case RPC_FC_WCHAR:
    case RPC_FC_USHORT:
    case RPC_FC_SHORT:
    case RPC_FC_ENUM16:
        size = 2;
        break;
    case RPC_FC_ULONG:
    case RPC_FC_LONG:
    case RPC_FC_ERROR_STATUS_T:
    case RPC_FC_ENUM32:
    case RPC_FC_FLOAT:
        size = 4;
        break;
    case RPC_FC_HYPER:
    case RPC_FC_DOUBLE:
        size = 8;
        break;
    case RPC_FC_STRUCT:
    case RPC_FC_CVSTRUCT:
    case RPC_FC_CPSTRUCT:
    case RPC_FC_CSTRUCT:
    case RPC_FC_PSTRUCT:
    case RPC_FC_BOGUS_STRUCT:
    case RPC_FC_ENCAPSULATED_UNION:
    case RPC_FC_NON_ENCAPSULATED_UNION:
        size = fields_memsize(t->fields);
        break;
    default:
        error("type_memsize: Unknown type %d\n", t->type);
        size = 0;
    }

    if (array)
    {
        if (array->is_const)
            size *= array->cval;
        else
            size = 0;
    }

    return size;
}

size_t get_type_memsize(const type_t *type)
{
    return type_memsize(type, 0, NULL);
}

static int write_pointers(FILE *file, const attr_t *attrs,
                          const type_t *type, int ptr_level,
                          const expr_t *array, int level,
                          unsigned int *typestring_offset)
{
    int pointers_written = 0;
    const var_t *v;

    /* don't generate a pointer for first-level arrays since we want to
    * descend into them to write their pointers, not stop here */
    if ((level == 0 || ptr_level == 0) && is_array_type(attrs, ptr_level, array))
    {
        return write_pointers(file, NULL, type, 0, NULL, level + 1, typestring_offset);
    }

    if (ptr_level != 0)
    {
        /* FIXME: only general algorithm implemented, not the actual writing */
        error("write_pointers: Writing type format string for pointer is unimplemented\n");
        return 1;
    }

    /* FIXME: search through all refs for pointers too */
    while(type_has_ref(type))
        type = type->ref;

    switch (type->type)
    {
        /* note: don't descend into complex structures or unions since these
         * will always be generated as a separate type */
        case RPC_FC_STRUCT:
        case RPC_FC_CVSTRUCT:
        case RPC_FC_CPSTRUCT:
        case RPC_FC_CSTRUCT:
        case RPC_FC_PSTRUCT:
            v = type->fields;
            if (!v) break;
            while (NEXT_LINK(v)) v = NEXT_LINK(v);
            for (; v; v = PREV_LINK(v))
                pointers_written += write_pointers(file, v->attrs, v->type,
                                                   v->ptr_level, v->array,
                                                   level + 1,
                                                   typestring_offset);

            break;

        default:
            /* nothing to do */
            break;
    }

    return pointers_written;
}

static size_t write_pointer_description(FILE *file, const attr_t *attrs,
                                        const type_t *type, int ptr_level,
                                        const expr_t *array, int level,
                                        size_t typestring_offset)
{
    size_t size = 0;
    const var_t *v;

    /* don't generate a pointer for first-level arrays since we want to
     * descend into them to write their pointers, not stop here */
    if ((level == 0 || ptr_level == 0) && is_array_type(attrs, ptr_level, array))
    {
        return write_pointer_description(file, NULL, type, 0, NULL,
                                         level + 1, typestring_offset);
    }

    if (ptr_level != 0)
    {
        /* FIXME: only general algorithm implemented, not the actual writing */
        error("write_pointer_description: Writing pointer description is unimplemented\n");
        return 0;
    }

    /* FIXME: search through all refs for pointers too */

    switch (type->type)
    {
        /* note: don't descend into complex structures or unions since these
         * will always be generated as a separate type */
        case RPC_FC_STRUCT:
        case RPC_FC_CVSTRUCT:
        case RPC_FC_CPSTRUCT:
        case RPC_FC_CSTRUCT:
        case RPC_FC_PSTRUCT:
            v = type->fields;
            if (!v) break;
            while (NEXT_LINK(v)) v = NEXT_LINK(v);
            for (; v; v = PREV_LINK(v))
                size += write_pointer_description(file, v->attrs, v->type,
                                                  v->ptr_level, v->array,
                                                  level + 1,
                                                  typestring_offset);

            break;

        default:
            /* nothing to do */
            break;
    }

    return size;
}

static size_t write_string_tfs(FILE *file, const attr_t *attrs,
                               const type_t *type, const expr_t *array,
                               const char *name, unsigned int *typestring_offset)
{
    const expr_t *size_is = get_attrp(attrs, ATTR_SIZEIS);
    int has_size = size_is && (size_is->type != EXPR_VOID);
    size_t start_offset = *typestring_offset;
    unsigned char flags = 0;
    int pointer_type = get_attrv(attrs, ATTR_POINTERTYPE);
    if (!pointer_type)
        pointer_type = RPC_FC_RP;

    if (!get_attrp(attrs, ATTR_SIZEIS))
        flags |= RPC_FC_P_SIMPLEPOINTER;

    while (type_has_ref(type))
        type = type->ref;

    if ((type->type != RPC_FC_BYTE) && (type->type != RPC_FC_CHAR) && (type->type != RPC_FC_WCHAR))
    {
        error("write_string_tfs: Unimplemented for type 0x%x of name: %s\n", type->type, name);
        return start_offset;
    }

    print_file(file, 2,"0x%x, 0x%x,    /* %s%s */\n",
               pointer_type, flags,
               pointer_type == RPC_FC_FP ? "FC_FP" : (pointer_type == RPC_FC_UP ? "FC_UP" : "FC_RP"),
               (flags & RPC_FC_P_SIMPLEPOINTER) ? " [simple_pointer]" : "");
    *typestring_offset += 2;

    if (!(flags & RPC_FC_P_SIMPLEPOINTER))
    {
        print_file(file, 2, "NdrFcShort(0x2),\n");
        *typestring_offset += 2;
    }

    if (array && array->is_const)
    {
        if (array->cval > USHRT_MAX)
            error("array size for parameter %s exceeds %d bytes by %ld bytes\n",
                  name, USHRT_MAX, array->cval - USHRT_MAX);

        if (type->type == RPC_FC_CHAR)
            WRITE_FCTYPE(file, FC_CSTRING, *typestring_offset);
        else
            WRITE_FCTYPE(file, FC_WSTRING, *typestring_offset);
        print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
        *typestring_offset += 2;

        print_file(file, 2, "NdrFcShort(0x%x), /* %d */\n", array->cval, array->cval);
        *typestring_offset += 2;

        return start_offset;
    }
    else if (has_size)
    {
        if (type->type == RPC_FC_CHAR)
            WRITE_FCTYPE(file, FC_C_CSTRING, *typestring_offset);
        else
            WRITE_FCTYPE(file, FC_C_WSTRING, *typestring_offset);
        print_file(file, 2, "0x%x, /* FC_STRING_SIZED */\n", RPC_FC_STRING_SIZED);
        *typestring_offset += 2;

        *typestring_offset += write_conf_or_var_desc(file, current_func, NULL, size_is);

        return start_offset;
    }
    else
    {
        if (type->type == RPC_FC_CHAR)
            WRITE_FCTYPE(file, FC_C_CSTRING, *typestring_offset);
        else
            WRITE_FCTYPE(file, FC_C_WSTRING, *typestring_offset);
        print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
        *typestring_offset += 2;

        return start_offset;
    }
}

static size_t write_array_tfs(FILE *file, const attr_t *attrs,
                              const type_t *type, const expr_t *array,
                              const char *name, unsigned int *typestring_offset)
{
    const expr_t *length_is = get_attrp(attrs, ATTR_LENGTHIS);
    const expr_t *size_is = get_attrp(attrs, ATTR_SIZEIS);
    int has_length = length_is && (length_is->type != EXPR_VOID);
    int has_size = (size_is && (size_is->type != EXPR_VOID)) || !array->is_const;
    size_t start_offset;
    int pointer_type = get_attrv(attrs, ATTR_POINTERTYPE);
    if (!pointer_type)
        pointer_type = RPC_FC_RP;

    print_file(file, 2, "0x%x, 0x00,    /* %s */\n",
               pointer_type,
               pointer_type == RPC_FC_FP ? "FC_FP" : (pointer_type == RPC_FC_UP ? "FC_UP" : "FC_RP"));
    print_file(file, 2, "NdrFcShort(0x2),\n");
    *typestring_offset += 4;

    if (array && NEXT_LINK(array)) /* multi-dimensional array */
    {
        error("write_array_tfs: Multi-dimensional arrays not implemented yet (param %s)\n", name);
        return 0;
    }
    else
    {
        size_t pointer_start_offset = *typestring_offset;
        int has_pointer = 0;

        if (write_pointers(file, attrs, type, 0, array, 0, typestring_offset) > 0)
            has_pointer = 1;

        start_offset = *typestring_offset;

        if (!has_length && !has_size)
        {
            /* fixed array */
            size_t size = type_memsize(type, 0, array);
            if (size < USHRT_MAX)
            {
                WRITE_FCTYPE(file, FC_SMFARRAY, *typestring_offset);
                /* alignment */
                print_file(file, 2, "0x%x, /* 0 */\n", 0);
                /* size */
                print_file(file, 2, "NdrFcShort(0x%x), /* %d */\n", size, size);
                *typestring_offset += 4;
            }
            else
            {
                WRITE_FCTYPE(file, FC_LGFARRAY, *typestring_offset);
                /* alignment */
                print_file(file, 2, "0x%x, /* 0 */\n", 0);
                /* size */
                print_file(file, 2, "NdrFcLong(0x%x), /* %d */\n", size, size);
                *typestring_offset += 6;
            }

            if (has_pointer)
            {
                print_file(file, 2, "0x%x, /* FC_PP */\n", RPC_FC_PP);
                print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
                *typestring_offset += 2;
                *typestring_offset = write_pointer_description(file, attrs,
                    type, 0, array, 0, pointer_start_offset);
                print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
                *typestring_offset += 1;
            }

            print_file(file, 2, "0x0, /* FIXME: write out conversion data */\n");
            print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
            *typestring_offset += 2;

            return start_offset;
        }
        else if (has_length && !has_size)
        {
            /* varying array */
            size_t element_size = type_memsize(type, 0, NULL);
            size_t elements = array->cval;
            size_t total_size = element_size * elements;

            if (total_size < USHRT_MAX)
            {
                WRITE_FCTYPE(file, FC_SMVARRAY, *typestring_offset);
                /* alignment */
                print_file(file, 2, "0x%x, /* 0 */\n", 0);
                /* total size */
                print_file(file, 2, "NdrFcShort(0x%x), /* %d */\n", total_size, total_size);
                /* number of elements */
                print_file(file, 2, "NdrFcShort(0x%x), /* %d */\n", elements, elements);
                *typestring_offset += 6;
            }
            else
            {
                WRITE_FCTYPE(file, FC_LGVARRAY, *typestring_offset);
                /* alignment */
                print_file(file, 2, "0x%x, /* 0 */\n", 0);
                /* total size */
                print_file(file, 2, "NdrFcLong(0x%x), /* %d */\n", total_size, total_size);
                /* number of elements */
                print_file(file, 2, "NdrFcLong(0x%x), /* %d */\n", elements, elements);
                *typestring_offset += 10;
            }
            /* element size */
            print_file(file, 2, "NdrFcShort(0x%x), /* %d */\n", element_size, element_size);
            *typestring_offset += 2;

            *typestring_offset += write_conf_or_var_desc(file, current_func,
                                                         current_structure,
                                                         length_is);

            if (has_pointer)
            {
                print_file(file, 2, "0x%x, /* FC_PP */\n", RPC_FC_PP);
                print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
                *typestring_offset += 2;
                *typestring_offset += write_pointer_description(file, attrs,
                    type, 0, array, 0, pointer_start_offset);
                print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
                *typestring_offset += 1;
            }

            print_file(file, 2, "0x0, /* FIXME: write out conversion data */\n");
            print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
            *typestring_offset += 2;

            return start_offset;
        }
        else if (!has_length && has_size)
        {
            /* conformant array */
            size_t element_size = type_memsize(type, 0, NULL);

            WRITE_FCTYPE(file, FC_CARRAY, *typestring_offset);
            /* alignment */
            print_file(file, 2, "0x%x, /* 0 */\n", 0);
            /* element size */
            print_file(file, 2, "NdrFcShort(0x%x), /* %d */\n", element_size, element_size);
            *typestring_offset += 4;

            *typestring_offset += write_conf_or_var_desc(file, current_func,
                                                         current_structure,
                                                         size_is ? size_is : array);

            if (has_pointer)
            {
                print_file(file, 2, "0x%x, /* FC_PP */\n", RPC_FC_PP);
                print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
                *typestring_offset += 2;
                *typestring_offset += write_pointer_description(file, attrs,
                    type, 0, array, 0, pointer_start_offset);
                print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
                *typestring_offset += 1;
            }

            print_file(file, 2, "0x%x, /* FIXME: write out conversion data */\n", type->type);
            print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
            *typestring_offset += 2;

            return start_offset;
        }
        else
        {
            /* conformant varying array */
            size_t element_size = type_memsize(type, 0, NULL);

            WRITE_FCTYPE(file, FC_CVARRAY, *typestring_offset);
            /* alignment */
            print_file(file, 2, "0x%x, /* 0 */\n", 0);
            /* element size */
            print_file(file, 2, "NdrFcShort(0x%x), /* %d */\n", element_size, element_size);
            *typestring_offset += 4;

            *typestring_offset += write_conf_or_var_desc(file, current_func,
                                                         current_structure,
                                                         size_is ? size_is : array);
            *typestring_offset += write_conf_or_var_desc(file, current_func,
                                                         current_structure,
                                                         length_is);

            if (has_pointer)
            {
                print_file(file, 2, "0x%x, /* FC_PP */\n", RPC_FC_PP);
                print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
                *typestring_offset += 2;
                *typestring_offset += write_pointer_description(file, attrs,
                    type, 0, array, 0, pointer_start_offset);
                print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
                *typestring_offset += 1;
            }

            print_file(file, 2, "0x0, /* FIXME: write out conversion data */\n");
            print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
            *typestring_offset += 2;

            return start_offset;
        }
    }
}

static const var_t *find_array_or_string_in_struct(const type_t *type)
{
    /* last field is the first in the fields linked list */
    const var_t *last_field = type->fields;
    if (is_array_type(last_field->attrs, last_field->ptr_level, last_field->array))
        return last_field;

    assert((last_field->type->type == RPC_FC_CSTRUCT) ||
           (last_field->type->type == RPC_FC_CPSTRUCT) ||
           (last_field->type->type == RPC_FC_CVSTRUCT));

    return find_array_or_string_in_struct(last_field->type);
}

static size_t write_struct_members(FILE *file, const type_t *type)
{
    size_t typestring_size = 0;
    var_t *field;

    field = type->fields;
    while (NEXT_LINK(field)) field = NEXT_LINK(field);
    for (; field; field = PREV_LINK(field))
    {
        if (is_base_type(field->type->type))
        {
            switch (field->type->type)
            {
#define CASE_BASETYPE(fctype) \
            case RPC_##fctype: \
                print_file(file, 2, "0x%02x,\t\t/* " #fctype " */\n", RPC_##fctype); \
                typestring_size++; \
                break;
            CASE_BASETYPE(FC_BYTE);
            CASE_BASETYPE(FC_CHAR);
            CASE_BASETYPE(FC_SMALL);
            CASE_BASETYPE(FC_USMALL);
            CASE_BASETYPE(FC_WCHAR);
            CASE_BASETYPE(FC_SHORT);
            CASE_BASETYPE(FC_USHORT);
            CASE_BASETYPE(FC_LONG);
            CASE_BASETYPE(FC_ULONG);
            CASE_BASETYPE(FC_FLOAT);
            CASE_BASETYPE(FC_HYPER);
            CASE_BASETYPE(FC_DOUBLE);
            CASE_BASETYPE(FC_ENUM16);
            CASE_BASETYPE(FC_ENUM32);
            CASE_BASETYPE(FC_IGNORE);
            CASE_BASETYPE(FC_ERROR_STATUS_T);
            default:
                break;
#undef CASE_BASETYPE
            }
        }
        else
            error("Unsupported member type 0x%x\n", field->type->type);
    }

    if (typestring_size % 1)
    {
        print_file(file, 2, "0x%x,\t\t/* FC_PAD */\n", RPC_FC_PAD);
        typestring_size++;
    }

    print_file(file, 2, "0x%x,\t\t/* FC_END */\n", RPC_FC_END);
    typestring_size++;

    return typestring_size;
}

static size_t write_struct_tfs(FILE *file, const type_t *type,
                               const char *name, unsigned int *typestring_offset)
{
    unsigned int total_size;
    const var_t *array;
    size_t start_offset;
    size_t array_offset;
    size_t pointer_offset;

    switch (type->type)
    {
    case RPC_FC_STRUCT:
    case RPC_FC_PSTRUCT:
        total_size = type_memsize(type, 0, NULL);

        if (total_size > USHRT_MAX)
            error("structure size for parameter %s exceeds %d bytes by %d bytes\n",
                  name, USHRT_MAX, total_size - USHRT_MAX);

        if (type->type == RPC_FC_PSTRUCT)
        {
            pointer_offset = *typestring_offset;
            write_pointers(file, NULL, type, 0, NULL, 0, typestring_offset);
        }
        else pointer_offset = 0; /* silence warning */

        start_offset = *typestring_offset;
        if (type->type == RPC_FC_STRUCT)
            WRITE_FCTYPE(file, FC_STRUCT, *typestring_offset);
        else
            WRITE_FCTYPE(file, FC_PSTRUCT, *typestring_offset);
        /* alignment */
        print_file(file, 2, "0x3,\n"); /* FIXME */
        /* total size */
        print_file(file, 2, "NdrFcShort(0x%x), /* %u */\n", total_size, total_size);
        *typestring_offset += 4;

        if (type->type == RPC_FC_PSTRUCT)
        {
            print_file(file, 2, "0x%x, /* FC_PP */\n", RPC_FC_PP);
            print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
            *typestring_offset += 2;
            *typestring_offset += write_pointer_description(file, NULL,
                type, 0, NULL, 0, pointer_offset);
            print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
            *typestring_offset += 1;
        }

        /* member layout */
        *typestring_offset += write_struct_members(file, type);
        return start_offset;
    case RPC_FC_CSTRUCT:
    case RPC_FC_CPSTRUCT:
        total_size = type_memsize(type, 0, NULL);

        if (total_size > USHRT_MAX)
            error("structure size for parameter %s exceeds %d bytes by %d bytes\n",
                  name, USHRT_MAX, total_size - USHRT_MAX);

        array = find_array_or_string_in_struct(type);
        current_structure = type;
        array_offset = write_array_tfs(file, array->attrs, array->type,
                                       array->array, array->name,
                                       typestring_offset);
        current_structure = NULL;

        if (type->type == RPC_FC_CPSTRUCT)
        {
            pointer_offset = *typestring_offset;
            write_pointers(file, NULL, type, 0, NULL, 0, typestring_offset);
        }
        else pointer_offset = 0; /* silence warning */

        start_offset = *typestring_offset;
        if (type->type == RPC_FC_CSTRUCT)
            WRITE_FCTYPE(file, FC_CSTRUCT, *typestring_offset);
        else
            WRITE_FCTYPE(file, FC_CPSTRUCT, *typestring_offset);
        /* alignment */
        print_file(file, 2, "0x0,\n");
        /* total size */
        print_file(file, 2, "NdrFcShort(0x%x), /* %u */\n", total_size, total_size);
        *typestring_offset += 4;
        print_file(file, 2, "NdrFcShort(0x%x), /* offset = %d (%u) */\n",
                   array_offset - *typestring_offset,
                   array_offset - *typestring_offset,
                   array_offset);
        *typestring_offset += 2;

        if (type->type == RPC_FC_CPSTRUCT)
        {
            print_file(file, 2, "0x%x, /* FC_PP */\n", RPC_FC_PP);
            print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
            *typestring_offset += 2;
            *typestring_offset += write_pointer_description(file, NULL,
                type, 0, NULL, 0, pointer_offset);
            print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
            *typestring_offset += 1;
        }

        print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
        *typestring_offset += 1;

        return start_offset;
    case RPC_FC_CVSTRUCT:
        total_size = type_memsize(type, 0, NULL);

        if (total_size > USHRT_MAX)
            error("structure size for parameter %s exceeds %d bytes by %d bytes\n",
                  name, USHRT_MAX, total_size - USHRT_MAX);

        array = find_array_or_string_in_struct(type);
        current_structure = type;
        if (is_attr(array->attrs, ATTR_STRING))
            array_offset = write_string_tfs(file, array->attrs, array->type,
                                            array->array, array->name,
                                            typestring_offset);
        else
            array_offset = write_array_tfs(file, array->attrs, array->type,
                                           array->array, array->name,
                                           typestring_offset);
        current_structure = NULL;

        pointer_offset = *typestring_offset;
        if (!write_pointers(file, NULL, type, 0, NULL, 0, typestring_offset))
            pointer_offset = 0;

        start_offset = *typestring_offset;
        WRITE_FCTYPE(file, FC_CVSTRUCT, *typestring_offset);
        /* alignment */
        print_file(file, 2, "0x0,\n");
        /* total size */
        print_file(file, 2, "NdrFcShort(0x%x), /* %u */\n", total_size, total_size);
        *typestring_offset += 4;
        print_file(file, 2, "NdrFcShort(0x%x), /* offset = %d (%u) */\n",
                   array_offset - *typestring_offset,
                   array_offset - *typestring_offset,
                   array_offset);
        *typestring_offset += 2;

        if (pointer_offset != 0)
        {
            print_file(file, 2, "0x%x, /* FC_PP */\n", RPC_FC_PP);
            print_file(file, 2, "0x%x, /* FC_PAD */\n", RPC_FC_PAD);
            *typestring_offset += 2;
            *typestring_offset += write_pointer_description(file, NULL,
                type, 0, NULL, 0, pointer_offset);
            print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
            *typestring_offset += 1;
        }

        print_file(file, 2, "0x%x, /* FC_END */\n", RPC_FC_END);
        *typestring_offset += 1;

        return start_offset;
    default:
        error("write_struct_tfs: Unimplemented for type 0x%x\n", type->type);
        return *typestring_offset;
    }
}

static void write_pointer_only_tfs(FILE *file, const attr_t *attrs, size_t offset, unsigned int *typeformat_offset)
{
    int in_attr, out_attr;
    unsigned char flags = 0;
    int pointer_type = get_attrv(attrs, ATTR_POINTERTYPE);
    if (!pointer_type) pointer_type = RPC_FC_RP;
    in_attr = is_attr(attrs, ATTR_IN);
    out_attr = is_attr(attrs, ATTR_OUT);
    if (!in_attr && !out_attr) in_attr = 1;

    if (out_attr && !in_attr && pointer_type == RPC_FC_RP)
        flags |= 0x04;

    print_file(file, 2, "0x%x, 0x%x,\t\t/* %s%s */\n",
               pointer_type,
               flags,
               pointer_type == RPC_FC_FP ? "FC_FP" : (pointer_type == RPC_FC_UP ? "FC_UP" : "FC_RP"),
               (flags & 0x04) ? " [allocated_on_stack]" : "");
    print_file(file, 2, "NdrFcShort(0x%x),    /* %d */\n", offset, offset);
    *typeformat_offset += 4;
}

static size_t write_union_tfs(FILE *file, const attr_t *attrs,
                              const type_t *type, const char *name,
                              unsigned int *typeformat_offset)
{
    error("write_union_tfs: Unimplemented\n");
    return *typeformat_offset;
}

static size_t write_typeformatstring_var(FILE *file, int indent,
                                         const var_t *var, unsigned int *typeformat_offset)
{
    const type_t *type = var->type;
    int ptr_level = var->ptr_level;

    chat("write_typeformatstring_var: %s\n", var->name);

    while (TRUE)
    {
        chat("write_typeformatstring: type->type = 0x%x, type->name = %s, ptr_level = %d\n", type->type, type->name, ptr_level);

        if (is_string_type(var->attrs, ptr_level, var->array))
            return write_string_tfs(file, var->attrs, type, var->array, var->name, typeformat_offset);

        if (is_array_type(var->attrs, ptr_level, var->array))
            return write_array_tfs(file, var->attrs, type, var->array, var->name, typeformat_offset);

        if (ptr_level == 0)
        {
            /* follow reference if the type has one */
            if (type_has_ref(type))
            {
                type = type->ref;
                /* FIXME: get new ptr_level from type */
                continue;
            }

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
                return write_union_tfs(file, var->attrs, type, var->name, typeformat_offset);
            case RPC_FC_IGNORE:
            case RPC_FC_BIND_PRIMITIVE:
                /* nothing to do */
                return 0;
            default:
                error("write_typeformatstring_var: Unsupported type 0x%x for variable %s\n", type->type, var->name);
            }
        }
        else if (ptr_level == 1 && !type_has_ref(type))
        {
            size_t start_offset = *typeformat_offset;
            int in_attr = is_attr(var->attrs, ATTR_IN);
            int out_attr = is_attr(var->attrs, ATTR_OUT);
            int pointer_type = get_attrv(var->attrs, ATTR_POINTERTYPE);
            if (!pointer_type) pointer_type = RPC_FC_RP;

            /* special case for pointers to base types */
            switch (type->type)
            {
#define CASE_BASETYPE(fctype) \
            case RPC_##fctype: \
                print_file(file, indent, "0x%x, 0x%x,    /* %s %s[simple_pointer] */\n", \
                           pointer_type, \
                           (!in_attr && out_attr) ? 0x0C : 0x08, \
                           pointer_type == RPC_FC_FP ? "FC_FP" : (pointer_type == RPC_FC_UP ? "FC_UP" : "FC_RP"), \
                           (!in_attr && out_attr) ? "[allocated_on_stack] " : ""); \
                print_file(file, indent, "0x%02x,    /* " #fctype " */\n", RPC_##fctype); \
                print_file(file, indent, "0x5c,          /* FC_PAD */\n"); \
                *typeformat_offset += 4; \
                return start_offset
            CASE_BASETYPE(FC_BYTE);
            CASE_BASETYPE(FC_CHAR);
            CASE_BASETYPE(FC_SMALL);
            CASE_BASETYPE(FC_USMALL);
            CASE_BASETYPE(FC_WCHAR);
            CASE_BASETYPE(FC_SHORT);
            CASE_BASETYPE(FC_USHORT);
            CASE_BASETYPE(FC_LONG);
            CASE_BASETYPE(FC_ULONG);
            CASE_BASETYPE(FC_FLOAT);
            CASE_BASETYPE(FC_HYPER);
            CASE_BASETYPE(FC_DOUBLE);
            CASE_BASETYPE(FC_ENUM16);
            CASE_BASETYPE(FC_ENUM32);
            CASE_BASETYPE(FC_IGNORE);
            CASE_BASETYPE(FC_ERROR_STATUS_T);
            default:
                break;
            }
        }

        assert(ptr_level > 0);

        if (file)
            fprintf(file, "/* %2u */\n", *typeformat_offset);
        write_pointer_only_tfs(file, var->attrs, 2, typeformat_offset);

        ptr_level--;
    }
}


void write_typeformatstring(FILE *file, const ifref_t *ifaces)
{
    int indent = 0;
    var_t *var;
    unsigned int typeformat_offset;
    const ifref_t *iface = ifaces;

    print_file(file, indent, "static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =\n");
    print_file(file, indent, "{\n");
    indent++;
    print_file(file, indent, "0,\n");
    print_file(file, indent, "{\n");
    indent++;
    print_file(file, indent, "NdrFcShort(0x0),\n");
    typeformat_offset = 2;

    END_OF_LIST(iface);

    for (; iface; iface = PREV_LINK(iface))
    {
        if (is_object(iface->iface->attrs) || is_local(iface->iface->attrs))
            continue;

        if (iface->iface->funcs)
        {
            func_t *func = iface->iface->funcs;
            while (NEXT_LINK(func)) func = NEXT_LINK(func);
            for (; func; func = PREV_LINK(func))
            {
                current_func = func;
                if (func->args)
                {
                    var = func->args;
                    while (NEXT_LINK(var)) var = NEXT_LINK(var);
                    while (var)
                    {
                        write_typeformatstring_var(file, indent, var,
                                                   &typeformat_offset);
                        var = PREV_LINK(var);
                    }
                }
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

static unsigned int get_required_buffer_size_type(
    const type_t *type, int ptr_level, const expr_t *array,
    const char *name, unsigned int *alignment)
{
    *alignment = 0;
    if (ptr_level == 0 && !array && !type_has_ref(type))
    {
        switch (type->type)
        {
        case RPC_FC_BYTE:
        case RPC_FC_CHAR:
        case RPC_FC_USMALL:
        case RPC_FC_SMALL:
            *alignment = 4;
            return 1;

        case RPC_FC_WCHAR:
        case RPC_FC_USHORT:
        case RPC_FC_SHORT:
            *alignment = 4;
            return 2;

        case RPC_FC_ULONG:
        case RPC_FC_LONG:
        case RPC_FC_FLOAT:
        case RPC_FC_ERROR_STATUS_T:
            *alignment = 4;
            return 4;

        case RPC_FC_HYPER:
        case RPC_FC_DOUBLE:
            *alignment = 8;
            return 8;

        case RPC_FC_IGNORE:
        case RPC_FC_BIND_PRIMITIVE:
            return 0;

        case RPC_FC_STRUCT:
        {
            size_t size = 0;
            const var_t *field;
            for (field = type->fields; field; field = NEXT_LINK(field))
            {
                unsigned int alignment;
                size += get_required_buffer_size_type(
                    field->type, field->ptr_level, field->array, field->name,
                    &alignment);
            }
            return size;
        }

        default:
            error("get_required_buffer_size: Unknown/unsupported type: %s (0x%02x)\n", name, type->type);
            return 0;
        }
    }
    if (ptr_level == 0 && type_has_ref(type))
        return get_required_buffer_size_type(type->ref, 0 /* FIXME */, array, name, alignment);
    return 0;
}

unsigned int get_required_buffer_size(const var_t *var, unsigned int *alignment, enum pass pass)
{
    expr_t *size_is = get_attrp(var->attrs, ATTR_SIZEIS);
    int has_size = (size_is && (size_is->type != EXPR_VOID));
    int in_attr = is_attr(var->attrs, ATTR_IN);
    int out_attr = is_attr(var->attrs, ATTR_OUT);

    if (!in_attr && !out_attr)
        in_attr = 1;

    *alignment = 0;

    if (pass == PASS_OUT)
    {
        if (out_attr && var->ptr_level > 0)
        {
            type_t *type = var->type;
            while (type->type == 0 && type->ref)
                type = type->ref;

            if (type->type == RPC_FC_STRUCT)
            {
                const var_t *field;
                unsigned int size = 36;
                for (field = type->fields; field; field = NEXT_LINK(field))
                {
                    unsigned int align;
                    size += get_required_buffer_size_type(
                        field->type, field->ptr_level, field->array, field->name,
                        &align);
                }
                return size;
            }
            else
                return get_required_buffer_size_type(var->type, 0 /* FIXME */, var->array, var->name, alignment);
        }
        return 0;
    }
    else
    {
        if ((!out_attr || in_attr) && !has_size && !is_attr(var->attrs, ATTR_STRING) && !var->array)
        {
            if (var->ptr_level > 0 || (var->ptr_level == 0 && type_has_ref(var->type)))
            {
                type_t *type = var->type;
                while (type->type == 0 && type->ref)
                    type = type->ref;

                if (is_base_type(type->type))
                {
                    return 25;
                }
                else if (type->type == RPC_FC_STRUCT)
                {
                    unsigned int size = 36;
                    const var_t *field;
                    for (field = type->fields; field; field = NEXT_LINK(field))
                    {
                        unsigned int align;
                        size += get_required_buffer_size_type(
                            field->type, field->ptr_level, field->array, field->name,
                            &align);
                    }
                    return size;
                }
            }
        }

        return get_required_buffer_size_type(var->type, var->ptr_level, var->array, var->name, alignment);
    }
}

static void print_phase_function(FILE *file, int indent, const char *type,
                                 enum remoting_phase phase,
                                 const char *varname, unsigned int type_offset)
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
    print_file(file, indent, "%s%s,\n",
               (phase == PHASE_UNMARSHAL) ? "(unsigned char **)&" : "(unsigned char *)",
               varname);
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
    const type_t *type = var->type;
    unsigned int size;
    unsigned int alignment = 0;

    /* no work to do for other phases, buffer sizing is done elsewhere */
    if (phase != PHASE_MARSHAL && phase != PHASE_UNMARSHAL)
        return;

    while (type_has_ref(type))
        type = type->ref;

    switch (type->type)
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
            size = 2;
            alignment = 2;
            break;

        case RPC_FC_ULONG:
        case RPC_FC_LONG:
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
            error("print_phase_basetype: Unsupported type: %s (0x%02x, ptr_level: 0)\n", var->name, type->type);
            size = 0;
    }

    print_file(file, indent, "_StubMsg.Buffer = (unsigned char *)(((long)_StubMsg.Buffer + %u) & ~0x%x);\n",
                alignment - 1, alignment - 1);

    if (phase == PHASE_MARSHAL)
    {
        print_file(file, indent, "*(");
        write_type(file, var->type, NULL, var->tname);
        if (var->ptr_level)
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
        if (pass == PASS_IN && var->ptr_level)
            fprintf(file, " = (");
        else
            fprintf(file, " = *(");
        write_type(file, var->type, NULL, var->tname);
        fprintf(file, " *)_StubMsg.Buffer;\n");
    }

    print_file(file, indent, "_StubMsg.Buffer += sizeof(");
    write_type(file, var->type, NULL, var->tname);
    fprintf(file, ");\n");
}

/* returns whether the MaxCount, Offset or ActualCount members need to be
 * filled in for the specified phase */
static inline int is_size_needed_for_phase(enum remoting_phase phase)
{
    return (phase != PHASE_UNMARSHAL);
}

void write_remoting_arguments(FILE *file, int indent, const func_t *func,
                              unsigned int *type_offset, enum pass pass,
                              enum remoting_phase phase)
{
    const expr_t *length_is;
    const expr_t *size_is;
    int in_attr, out_attr, has_length, has_size, pointer_type;
    var_t *var;

    if (!func->args)
        return;

    var = func->args;
    while (NEXT_LINK(var)) var = NEXT_LINK(var);
    for (; var; *type_offset += get_size_typeformatstring_var(var), var = PREV_LINK(var))
    {
        const type_t *type = var->type;
        length_is = get_attrp(var->attrs, ATTR_LENGTHIS);
        size_is = get_attrp(var->attrs, ATTR_SIZEIS);
        has_length = length_is && (length_is->type != EXPR_VOID);
        has_size = (size_is && (size_is->type != EXPR_VOID)) || (var->array && !var->array->is_const);

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
            if (!in_attr)
                continue;
            break;
        case PASS_OUT:
            if (!out_attr)
                continue;
            break;
        case PASS_RETURN:
            break;
        }

        while (type_has_ref(type))
            type = type->ref;

        if (is_string_type(var->attrs, var->ptr_level, var->array))
        {
            if (var->array && var->array->is_const)
                print_phase_function(file, indent, "NonConformantString", phase, var->name, *type_offset);
            else
            {
                if (size_is && is_size_needed_for_phase(phase))
                {
                    print_file(file, indent, "_StubMsg.MaxCount = (unsigned long)");
                    write_expr(file, size_is, 1);
                    fprintf(file, ";\n");
                }

                if ((phase == PHASE_FREE) || (pointer_type == RPC_FC_UP))
                    print_phase_function(file, indent, "Pointer", phase, var->name, *type_offset);
                else
                    print_phase_function(file, indent, "ConformantString", phase, var->name,
                                         *type_offset + (has_size ? 4 : 2));
            }
        }
        else if (is_array_type(var->attrs, var->ptr_level, var->array))
        {
            const char *array_type;

            if (var->array && NEXT_LINK(var->array)) /* multi-dimensional array */
                array_type = "ComplexArray";
            else
            {
                if (!has_length && !has_size)
                    array_type = "FixedArray";
                else if (has_length && !has_size)
                {
                    if (is_size_needed_for_phase(phase))
                    {
                        print_file(file, indent, "_StubMsg.Offset = (unsigned long)0;\n"); /* FIXME */
                        print_file(file, indent, "_StubMsg.ActualCount = (unsigned long)");
                        write_expr(file, length_is, 1);
                        fprintf(file, ";\n\n");
                    }
                    array_type = "VaryingArray";
                }
                else if (!has_length && has_size)
                {
                    if (is_size_needed_for_phase(phase) && phase != PHASE_FREE)
                    {
                        print_file(file, indent, "_StubMsg.MaxCount = (unsigned long)");
                        write_expr(file, size_is ? size_is : var->array, 1);
                        fprintf(file, ";\n\n");
                    }
                    array_type = "ConformantArray";
                }
                else
                {
                    if (is_size_needed_for_phase(phase))
                    {
                        print_file(file, indent, "_StubMsg.MaxCount = (unsigned long)");
                        write_expr(file, size_is ? size_is : var->array, 1);
                        fprintf(file, ";\n");
                        print_file(file, indent, "_StubMsg.Offset = (unsigned long)0;\n"); /* FIXME */
                        print_file(file, indent, "_StubMsg.ActualCount = (unsigned long)");
                        write_expr(file, length_is, 1);
                        fprintf(file, ";\n\n");
                    }
                    array_type = "ConformantVaryingArray";
                }
            }

            if (!in_attr && phase == PHASE_FREE)
            {
                print_file(file, indent, "if (%s)\n", var->name);
                indent++;
                print_file(file, indent, "_StubMsg.pfnFree(%s);\n", var->name);
            }
            else if (phase != PHASE_FREE)
            {
                if (pointer_type == RPC_FC_UP)
                    print_phase_function(file, indent, "Pointer", phase, var->name, *type_offset);
                else
                    print_phase_function(file, indent, array_type, phase, var->name, *type_offset + 4);
            }
        }
        else if (var->ptr_level == 0 && is_base_type(type->type))
        {
            print_phase_basetype(file, indent, phase, pass, var, var->name);
        }
        else if (var->ptr_level == 0)
        {
            const char *ndrtype;

            switch (type->type)
            {
            case RPC_FC_STRUCT:
                ndrtype = "SimpleStruct";
                break;
            case RPC_FC_CSTRUCT:
            case RPC_FC_CPSTRUCT:
                ndrtype = "ConformantStruct";
                break;
            case RPC_FC_CVSTRUCT:
                ndrtype = "ConformantVaryingStruct";
                break;
            case RPC_FC_BOGUS_STRUCT:
                ndrtype = "ComplexStruct";
                break;
            default:
                error("write_remoting_arguments: Unsupported type: %s (0x%02x, ptr_level: %d)\n",
                    var->name, type->type, var->ptr_level);
                ndrtype = NULL;
            }

            print_phase_function(file, indent, ndrtype, phase, var->name, *type_offset);
        }
        else
        {
            if ((var->ptr_level == 1) && (pointer_type == RPC_FC_RP) && is_base_type(type->type))
            {
                print_phase_basetype(file, indent, phase, pass, var, var->name);
            }
            else if ((var->ptr_level == 1) && (pointer_type == RPC_FC_RP) && (type->type == RPC_FC_STRUCT))
            {
                if (phase != PHASE_BUFFERSIZE && phase != PHASE_FREE)
                    print_phase_function(file, indent, "SimpleStruct", phase, var->name, *type_offset + 4);
            }
            else
            {
                print_phase_function(file, indent, "Pointer", phase, var->name, *type_offset);
            }
        }
        fprintf(file, "\n");
    }
}


size_t get_size_procformatstring_var(const var_t *var)
{
    unsigned int type_offset = 2;
    return write_procformatstring_var(NULL, 0, var, FALSE, &type_offset);
}


size_t get_size_typeformatstring_var(const var_t *var)
{
    unsigned int type_offset = 0;
    write_typeformatstring_var(NULL, 0, var, &type_offset);
    return type_offset;
}

size_t get_size_procformatstring(const ifref_t *ifaces)
{
    const ifref_t *iface = ifaces;
    size_t size = 1;
    func_t *func;
    var_t *var;

    END_OF_LIST(iface);

    for (; iface; iface = PREV_LINK(iface))
    {
        if (is_object(iface->iface->attrs) || is_local(iface->iface->attrs))
            continue;

        if (iface->iface->funcs)
        {
            func = iface->iface->funcs;
            while (NEXT_LINK(func)) func = NEXT_LINK(func);
            while (func)
            {
                /* argument list size */
                if (func->args)
                {
                    var = func->args;
                    while (NEXT_LINK(var)) var = NEXT_LINK(var);
                    while (var)
                    {
                        size += get_size_procformatstring_var(var);
                        var = PREV_LINK(var);
                    }
                }

                var = func->def;
                /* return value size */
                if (is_void(var->type, NULL))
                    size += 2;
                else
                    size += get_size_procformatstring_var(var);

                func = PREV_LINK(func);
            }
        }
    }
    return size;
}

size_t get_size_typeformatstring(const ifref_t *ifaces)
{
    const ifref_t *iface = ifaces;
    size_t size = 3;
    func_t *func;
    var_t *var;

    END_OF_LIST(iface);

    for (; iface; iface = PREV_LINK(iface))
    {
        if (is_object(iface->iface->attrs) || is_local(iface->iface->attrs))
            continue;

        if (iface->iface->funcs)
        {
            func = iface->iface->funcs;
            while (NEXT_LINK(func)) func = NEXT_LINK(func);
            while (func)
            {
                /* argument list size */
                if (func->args)
                {
                    var = func->args;
                    while (NEXT_LINK(var)) var = NEXT_LINK(var);
                    while (var)
                    {
                        size += get_size_typeformatstring_var(var);
                        var = PREV_LINK(var);
                    }
                }

                func = PREV_LINK(func);
            }
        }
    }
    return size;
}

static void write_struct_expr(FILE *h, const expr_t *e, int brackets,
                              const var_t *fields, const char *structvar)
{
    switch (e->type) {
        case EXPR_VOID:
            break;
        case EXPR_NUM:
            fprintf(h, "%ld", e->u.lval);
            break;
        case EXPR_HEXNUM:
            fprintf(h, "0x%lx", e->u.lval);
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
            for (field = fields; field; field = NEXT_LINK(field))
            {
                if (!strcmp(e->u.sval, field->name))
                {
                    fprintf(h, "%s->%s", structvar, e->u.sval);
                    break;
                }
            }
            if (!field) error("no field found for identifier %s\n", e->u.sval);
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
            write_type(h, e->u.tref->ref, NULL, e->u.tref->name);
            fprintf(h, ")");
            write_struct_expr(h, e->ref, 1, fields, structvar);
            break;
        case EXPR_SIZEOF:
            fprintf(h, "sizeof(");
            write_type(h, e->u.tref->ref, NULL, e->u.tref->name);
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

int write_expr_eval_routines(FILE *file, const char *iface)
{
    int result = 0;
    struct expr_eval_routine *eval;
    unsigned short callback_offset = 0;

    LIST_FOR_EACH_ENTRY(eval, &expr_eval_routines, struct expr_eval_routine, entry)
    {
        int indent = 0;
        result = 1;
        print_file(file, indent, "static void __RPC_USER %s_%sExprEval_%04u(PMIDL_STUB_MESSAGE pStubMsg)\n",
                  iface, eval->structure->name, callback_offset);
        print_file(file, indent, "{\n");
        indent++;
        print_file(file, indent, "struct %s *" STRUCT_EXPR_EVAL_VAR " = (struct %s *)(pStubMsg->StackTop - %u);\n",
                   eval->structure->name, eval->structure->name, eval->structure_size);
        fprintf(file, "\n");
        print_file(file, indent, "pStubMsg->Offset = 0;\n"); /* FIXME */
        print_file(file, indent, "pStubMsg->MaxCount = (unsigned long)");
        write_struct_expr(file, eval->expr, 1, eval->structure->fields, STRUCT_EXPR_EVAL_VAR);
        fprintf(file, ";\n");
        indent--;
        print_file(file, indent, "}\n\n");
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
        print_file(file, 1, "%s_%sExprEval_%04u,\n",
                   iface, eval->structure->name, callback_offset);

        callback_offset++;
        list_remove(&eval->entry);
        free(eval);
    }

    fprintf(file, "};\n\n");
}
