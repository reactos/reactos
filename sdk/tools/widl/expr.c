/*
 * Expression Abstract Syntax Tree Functions
 *
 * Copyright 2002 Ove Kaaven
 * Copyright 2006-2008 Robert Shearman
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
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "widl.h"
#include "utils.h"
#include "expr.h"
#include "header.h"
#include "typetree.h"
#include "typegen.h"

static int is_integer_type(const type_t *type)
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

static int is_signed_integer_type(const type_t *type)
{
    switch (type_get_type(type))
    {
    case TYPE_ENUM:
        return FALSE;
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
            return type_basic_get_sign(type) < 0;
        case TYPE_BASIC_CHAR:
            return TRUE;
        case TYPE_BASIC_HYPER:
        case TYPE_BASIC_BYTE:
        case TYPE_BASIC_WCHAR:
        case TYPE_BASIC_ERROR_STATUS_T:
        case TYPE_BASIC_FLOAT:
        case TYPE_BASIC_DOUBLE:
        case TYPE_BASIC_HANDLE:
            return FALSE;
        }
        /* FALLTHROUGH */
    default:
        return FALSE;
    }
}

static int is_float_type(const type_t *type)
{
    return (type_get_type(type) == TYPE_BASIC &&
        (type_basic_get_type(type) == TYPE_BASIC_FLOAT ||
         type_basic_get_type(type) == TYPE_BASIC_DOUBLE));
}

expr_t *make_expr(enum expr_type type)
{
    expr_t *e = xmalloc(sizeof(expr_t));
    memset(e, 0, sizeof(*e));
    e->type = type;
    return e;
}

expr_t *make_exprl(enum expr_type type, const struct integer *integer)
{
    expr_t *e = xmalloc(sizeof(expr_t));
    memset(e, 0, sizeof(*e));
    e->type = type;
    e->u.integer = *integer;
    /* check for numeric constant */
    if (type == EXPR_NUM || type == EXPR_TRUEFALSE)
    {
        /* make sure true/false value is valid */
        assert(type != EXPR_TRUEFALSE || integer->value == 0 || integer->value == 1);
        e->is_const = TRUE;
        e->cval = integer->value;
    }
    return e;
}

expr_t *make_exprd(enum expr_type type, double val)
{
    expr_t *e = xmalloc(sizeof(expr_t));
    e->type = type;
    e->ref = NULL;
    e->u.dval = val;
    e->is_const = TRUE;
    e->cval = val;
    return e;
}

expr_t *make_exprs(enum expr_type type, char *val)
{
    expr_t *e;
    e = xmalloc(sizeof(expr_t));
    e->type = type;
    e->ref = NULL;
    e->u.sval = val;
    e->is_const = FALSE;
    /* check for predefined constants */
    switch (type)
    {
    case EXPR_IDENTIFIER:
    {
        var_t *c = find_const(val, 0);
        if (c)
        {
            e->u.sval = c->name;
            free(val);
            e->is_const = TRUE;
            e->cval = c->eval->cval;
        }
        break;
    }
    case EXPR_CHARCONST:
        if (!val[0])
            error_loc("empty character constant\n");
        else if (val[1])
            error_loc("multi-character constants are endian dependent\n");
        else
        {
            e->is_const = TRUE;
            e->cval = *val;
        }
        break;
    default:
        break;
    }
    return e;
}

expr_t *make_exprt(enum expr_type type, var_t *var, expr_t *expr)
{
    expr_t *e;
    type_t *tref;

    if (var->declspec.stgclass != STG_NONE && var->declspec.stgclass != STG_REGISTER)
        error_loc("invalid storage class for type expression\n");

    tref = var->declspec.type;

    e = xmalloc(sizeof(expr_t));
    e->type = type;
    e->ref = expr;
    e->u.tref = var->declspec;
    e->is_const = FALSE;
    if (type == EXPR_SIZEOF)
    {
        /* only do this for types that should be the same on all platforms */
        if (is_integer_type(tref) || is_float_type(tref))
        {
            e->is_const = TRUE;
            e->cval = type_memsize(tref);
        }
    }
    /* check for cast of constant expression */
    if (type == EXPR_CAST && expr->is_const)
    {
        if (is_integer_type(tref))
        {
            unsigned int cast_type_bits = type_memsize(tref) * 8;
            unsigned int cast_mask;

            e->is_const = TRUE;
            if (is_signed_integer_type(tref))
            {
                cast_mask = (1u << (cast_type_bits - 1)) - 1;
                if (expr->cval & (1u << (cast_type_bits - 1)))
                    e->cval = -((-expr->cval) & cast_mask);
                else
                    e->cval = expr->cval & cast_mask;
            }
            else
            {
                /* calculate ((1 << cast_type_bits) - 1) avoiding overflow */
                cast_mask = ((1u << (cast_type_bits - 1)) - 1) |
                            1u << (cast_type_bits - 1);
                e->cval = expr->cval & cast_mask;
            }
        }
        else
        {
            e->is_const = TRUE;
            e->cval = expr->cval;
        }
    }
    free(var);
    return e;
}

expr_t *make_expr1(enum expr_type type, expr_t *expr)
{
    expr_t *e;
    e = xmalloc(sizeof(expr_t));
    memset(e, 0, sizeof(*e));
    e->type = type;
    e->ref = expr;
    /* check for compile-time optimization */
    if (expr->is_const)
    {
        e->is_const = TRUE;
        switch (type)
        {
        case EXPR_LOGNOT:
            e->cval = !expr->cval;
            break;
        case EXPR_POS:
            e->cval = +expr->cval;
            break;
        case EXPR_NEG:
            e->cval = -expr->cval;
            break;
        case EXPR_NOT:
            e->cval = ~expr->cval;
            break;
        default:
            e->is_const = FALSE;
            break;
        }
    }
    return e;
}

expr_t *make_expr2(enum expr_type type, expr_t *expr1, expr_t *expr2)
{
    expr_t *e;
    e = xmalloc(sizeof(expr_t));
    e->type = type;
    e->ref = expr1;
    e->u.ext = expr2;
    e->is_const = FALSE;
    /* check for compile-time optimization */
    if (expr1->is_const && expr2->is_const)
    {
        e->is_const = TRUE;
        switch (type)
        {
        case EXPR_ADD:
            e->cval = expr1->cval + expr2->cval;
            break;
        case EXPR_SUB:
            e->cval = expr1->cval - expr2->cval;
            break;
        case EXPR_MOD:
            if (expr2->cval == 0)
            {
                error_loc("divide by zero in expression\n");
                e->cval = 0;
            }
            else
                e->cval = expr1->cval % expr2->cval;
            break;
        case EXPR_MUL:
            e->cval = expr1->cval * expr2->cval;
            break;
        case EXPR_DIV:
            if (expr2->cval == 0)
            {
                error_loc("divide by zero in expression\n");
                e->cval = 0;
            }
            else
                e->cval = expr1->cval / expr2->cval;
            break;
        case EXPR_OR:
            e->cval = expr1->cval | expr2->cval;
            break;
        case EXPR_AND:
            e->cval = expr1->cval & expr2->cval;
            break;
        case EXPR_SHL:
            e->cval = expr1->cval << expr2->cval;
            break;
        case EXPR_SHR:
            e->cval = expr1->cval >> expr2->cval;
            break;
        case EXPR_LOGOR:
            e->cval = expr1->cval || expr2->cval;
            break;
        case EXPR_LOGAND:
            e->cval = expr1->cval && expr2->cval;
            break;
        case EXPR_XOR:
            e->cval = expr1->cval ^ expr2->cval;
            break;
        case EXPR_EQUALITY:
            e->cval = expr1->cval == expr2->cval;
            break;
        case EXPR_INEQUALITY:
            e->cval = expr1->cval != expr2->cval;
            break;
        case EXPR_GTR:
            e->cval = expr1->cval > expr2->cval;
            break;
        case EXPR_LESS:
            e->cval = expr1->cval < expr2->cval;
            break;
        case EXPR_GTREQL:
            e->cval = expr1->cval >= expr2->cval;
            break;
        case EXPR_LESSEQL:
            e->cval = expr1->cval <= expr2->cval;
            break;
        default:
            e->is_const = FALSE;
            break;
        }
    }
    return e;
}

expr_t *make_expr3(enum expr_type type, expr_t *expr1, expr_t *expr2, expr_t *expr3)
{
    expr_t *e;
    e = xmalloc(sizeof(expr_t));
    e->type = type;
    e->ref = expr1;
    e->u.ext = expr2;
    e->ext2 = expr3;
    e->is_const = FALSE;
    /* check for compile-time optimization */
    if (expr1->is_const && expr2->is_const && expr3->is_const)
    {
        e->is_const = TRUE;
        switch (type)
        {
        case EXPR_COND:
            e->cval = expr1->cval ? expr2->cval : expr3->cval;
            break;
        default:
            e->is_const = FALSE;
            break;
        }
    }
    return e;
}

struct expression_type
{
    int is_variable; /* is the expression resolved to a variable? */
    int is_temporary; /* should the type be freed? */
    type_t *type;
};

static void check_scalar_type(const struct expr_loc *expr_loc,
                              const type_t *cont_type, const type_t *type)
{
    if (!cont_type || (!is_integer_type( type ) && !is_ptr( type ) && !is_float_type( type )))
        error_at( &expr_loc->v->where, "scalar type required in expression%s%s\n",
                  expr_loc->attr ? " for attribute " : "", expr_loc->attr ? expr_loc->attr : "" );
}

static void check_arithmetic_type(const struct expr_loc *expr_loc,
                                  const type_t *cont_type, const type_t *type)
{
    if (!cont_type || (!is_integer_type( type ) && !is_float_type( type )))
        error_at( &expr_loc->v->where, "arithmetic type required in expression%s%s\n",
                  expr_loc->attr ? " for attribute " : "", expr_loc->attr ? expr_loc->attr : "" );
}

static void check_integer_type(const struct expr_loc *expr_loc,
                               const type_t *cont_type, const type_t *type)
{
    if (!cont_type || !is_integer_type( type ))
        error_at( &expr_loc->v->where, "integer type required in expression%s%s\n",
                  expr_loc->attr ? " for attribute " : "", expr_loc->attr ? expr_loc->attr : "" );
}

static type_t *find_identifier(const char *identifier, const type_t *cont_type, int *found_in_cont_type)
{
    type_t *type = NULL;
    const var_t *field;
    const var_list_t *fields = NULL;

    *found_in_cont_type = 0;

    if (cont_type)
    {
        switch (type_get_type(cont_type))
        {
        case TYPE_FUNCTION:
            fields = type_function_get_args(cont_type);
            break;
        case TYPE_STRUCT:
            fields = type_struct_get_fields(cont_type);
            break;
        case TYPE_UNION:
        case TYPE_ENCAPSULATED_UNION:
            fields = type_union_get_cases(cont_type);
            break;
        case TYPE_VOID:
        case TYPE_BASIC:
        case TYPE_ENUM:
        case TYPE_MODULE:
        case TYPE_COCLASS:
        case TYPE_INTERFACE:
        case TYPE_POINTER:
        case TYPE_ARRAY:
        case TYPE_BITFIELD:
        case TYPE_APICONTRACT:
        case TYPE_RUNTIMECLASS:
        case TYPE_PARAMETERIZED_TYPE:
        case TYPE_PARAMETER:
        case TYPE_DELEGATE:
            /* nothing to do */
            break;
        case TYPE_ALIAS:
            /* shouldn't get here because of using type_get_type above */
            assert(0);
            break;
        }
    }

    if (fields) LIST_FOR_EACH_ENTRY( field, fields, const var_t, entry )
        if (field->name && !strcmp(identifier, field->name))
        {
            type = field->declspec.type;
            *found_in_cont_type = 1;
            break;
        }

    if (!type)
    {
        var_t *const_var = find_const(identifier, 0);
        if (const_var) type = const_var->declspec.type;
    }

    return type;
}

static int is_valid_member_operand(const type_t *type)
{
    switch (type_get_type(type))
    {
    case TYPE_STRUCT:
    case TYPE_UNION:
    case TYPE_ENUM:
        return TRUE;
    default:
        return FALSE;
    }
}

static struct expression_type resolve_expression(const struct expr_loc *expr_loc,
                                                 const type_t *cont_type,
                                                 const expr_t *e)
{
    struct expression_type result;
    result.is_variable = FALSE;
    result.is_temporary = FALSE;
    result.type = NULL;
    switch (e->type)
    {
    case EXPR_VOID:
        break;
    case EXPR_NUM:
    case EXPR_TRUEFALSE:
        result.is_temporary = FALSE;
        result.type = type_new_int(e->u.integer.is_long ? TYPE_BASIC_LONG : TYPE_BASIC_INT, e->u.integer.is_unsigned);
        break;
    case EXPR_STRLIT:
        result.is_temporary = TRUE;
        result.type = type_new_pointer(type_new_int(TYPE_BASIC_CHAR, 0));
        break;
    case EXPR_WSTRLIT:
        result.is_temporary = TRUE;
        result.type = type_new_pointer(type_new_int(TYPE_BASIC_WCHAR, 0));
        break;
    case EXPR_CHARCONST:
        result.is_temporary = TRUE;
        result.type = type_new_int(TYPE_BASIC_CHAR, 0);
        break;
    case EXPR_DOUBLE:
        result.is_temporary = TRUE;
        result.type = type_new_basic(TYPE_BASIC_DOUBLE);
        break;
    case EXPR_IDENTIFIER:
    {
        int found_in_cont_type;
        result.is_variable = TRUE;
        result.is_temporary = FALSE;
        result.type = find_identifier(e->u.sval, cont_type, &found_in_cont_type);
        if (!result.type)
            error_at( &expr_loc->v->where, "identifier %s cannot be resolved in expression%s%s\n", e->u.sval,
                      expr_loc->attr ? " for attribute " : "", expr_loc->attr ? expr_loc->attr : "" );
        break;
    }
    case EXPR_LOGNOT:
        result = resolve_expression(expr_loc, cont_type, e->ref);
        check_scalar_type(expr_loc, cont_type, result.type);
        result.is_variable = FALSE;
        result.is_temporary = FALSE;
        result.type = type_new_int(TYPE_BASIC_INT, 0);
        break;
    case EXPR_NOT:
        result = resolve_expression(expr_loc, cont_type, e->ref);
        check_integer_type(expr_loc, cont_type, result.type);
        result.is_variable = FALSE;
        break;
    case EXPR_POS:
    case EXPR_NEG:
        result = resolve_expression(expr_loc, cont_type, e->ref);
        check_arithmetic_type(expr_loc, cont_type, result.type);
        result.is_variable = FALSE;
        break;
    case EXPR_ADDRESSOF:
        result = resolve_expression(expr_loc, cont_type, e->ref);
        if (!result.is_variable)
            error_at( &expr_loc->v->where, "address-of operator applied to non-variable type in expression%s%s\n",
                      expr_loc->attr ? " for attribute " : "", expr_loc->attr ? expr_loc->attr : "" );
        result.is_variable = FALSE;
        result.is_temporary = TRUE;
        result.type = type_new_pointer(result.type);
        break;
    case EXPR_PPTR:
        result = resolve_expression(expr_loc, cont_type, e->ref);
        if (result.type && is_ptr(result.type))
            result.type = type_pointer_get_ref_type(result.type);
        else if(result.type && is_array(result.type)
                            && type_array_is_decl_as_ptr(result.type))
            result.type = type_array_get_element_type(result.type);
        else
            error_at( &expr_loc->v->where, "dereference operator applied to non-pointer type in expression%s%s\n",
                      expr_loc->attr ? " for attribute " : "", expr_loc->attr ? expr_loc->attr : "" );
        break;
    case EXPR_CAST:
        result = resolve_expression(expr_loc, cont_type, e->ref);
        result.type = e->u.tref.type;
        break;
    case EXPR_SIZEOF:
        result.is_temporary = FALSE;
        result.type = type_new_int(TYPE_BASIC_INT, 0);
        break;
    case EXPR_SHL:
    case EXPR_SHR:
    case EXPR_MOD:
    case EXPR_MUL:
    case EXPR_DIV:
    case EXPR_ADD:
    case EXPR_SUB:
    case EXPR_AND:
    case EXPR_OR:
    case EXPR_XOR:
    {
        struct expression_type result_right;
        result = resolve_expression(expr_loc, cont_type, e->ref);
        result.is_variable = FALSE;
        result_right = resolve_expression(expr_loc, cont_type, e->u.ext);
        /* FIXME: these checks aren't strict enough for some of the operators */
        check_scalar_type(expr_loc, cont_type, result.type);
        check_scalar_type(expr_loc, cont_type, result_right.type);
        break;
    }
    case EXPR_LOGOR:
    case EXPR_LOGAND:
    case EXPR_EQUALITY:
    case EXPR_INEQUALITY:
    case EXPR_GTR:
    case EXPR_LESS:
    case EXPR_GTREQL:
    case EXPR_LESSEQL:
    {
        struct expression_type result_left, result_right;
        result_left = resolve_expression(expr_loc, cont_type, e->ref);
        result_right = resolve_expression(expr_loc, cont_type, e->u.ext);
        check_scalar_type(expr_loc, cont_type, result_left.type);
        check_scalar_type(expr_loc, cont_type, result_right.type);
        result.is_temporary = FALSE;
        result.type = type_new_int(TYPE_BASIC_INT, 0);
        break;
    }
    case EXPR_MEMBER:
        result = resolve_expression(expr_loc, cont_type, e->ref);
        if (result.type && is_valid_member_operand(result.type))
            result = resolve_expression(expr_loc, result.type, e->u.ext);
        else
            error_at( &expr_loc->v->where, "'.' or '->' operator applied to a type that isn't a structure, union or enumeration in expression%s%s\n",
                      expr_loc->attr ? " for attribute " : "", expr_loc->attr ? expr_loc->attr : "" );
        break;
    case EXPR_COND:
    {
        struct expression_type result_first, result_second, result_third;
        result_first = resolve_expression(expr_loc, cont_type, e->ref);
        check_scalar_type(expr_loc, cont_type, result_first.type);
        result_second = resolve_expression(expr_loc, cont_type, e->u.ext);
        result_third = resolve_expression(expr_loc, cont_type, e->ext2);
        check_scalar_type(expr_loc, cont_type, result_second.type);
        check_scalar_type(expr_loc, cont_type, result_third.type);
        if (!is_ptr( result_second.type ) ^ !is_ptr( result_third.type ))
            error_at( &expr_loc->v->where, "type mismatch in ?: expression\n" );
        /* FIXME: determine the correct return type */
        result = result_second;
        result.is_variable = FALSE;
        break;
    }
    case EXPR_ARRAY:
        result = resolve_expression(expr_loc, cont_type, e->ref);
        if (result.type && is_array(result.type))
        {
            struct expression_type index_result;
            result.type = type_array_get_element_type(result.type);
            index_result = resolve_expression(expr_loc, cont_type /* FIXME */, e->u.ext);
            if (!index_result.type || !is_integer_type( index_result.type ))
                error_at( &expr_loc->v->where, "array subscript not of integral type in expression%s%s\n",
                          expr_loc->attr ? " for attribute " : "", expr_loc->attr ? expr_loc->attr : "" );
        }
        else
        {
            error_at( &expr_loc->v->where, "array subscript operator applied to non-array type in expression%s%s\n",
                      expr_loc->attr ? " for attribute " : "", expr_loc->attr ? expr_loc->attr : "" );
        }
        break;
    }
    return result;
}

const type_t *expr_resolve_type(const struct expr_loc *expr_loc, const type_t *cont_type, const expr_t *expr)
{
    struct expression_type expr_type;
    expr_type = resolve_expression(expr_loc, cont_type, expr);
    return expr_type.type;
}

void write_expr(FILE *h, const expr_t *e, int brackets,
                int toplevel, const char *toplevel_prefix,
                const type_t *cont_type, const char *local_var_prefix)
{
    switch (e->type)
    {
    case EXPR_VOID:
        break;
    case EXPR_NUM:
        if (e->u.integer.is_hex)
            fprintf(h, "0x%x", e->u.integer.value);
        else
            fprintf(h, "%u", e->u.integer.value);
        if (e->u.integer.is_unsigned)
            fprintf(h, "u");
        if (e->u.integer.is_long)
            fprintf(h, "l");
        break;
    case EXPR_DOUBLE:
        fprintf(h, "%#.15g", e->u.dval);
        break;
    case EXPR_TRUEFALSE:
        if (e->u.integer.value == 0)
            fprintf(h, "FALSE");
        else
            fprintf(h, "TRUE");
        break;
    case EXPR_IDENTIFIER:
        if (toplevel && toplevel_prefix && cont_type)
        {
            int found_in_cont_type;
            find_identifier(e->u.sval, cont_type, &found_in_cont_type);
            if (found_in_cont_type)
            {
                fprintf(h, "%s%s", toplevel_prefix, e->u.sval);
                break;
            }
        }
        fprintf(h, "%s%s", local_var_prefix, e->u.sval);
        break;
    case EXPR_STRLIT:
        fprintf(h, "\"%s\"", e->u.sval);
        break;
    case EXPR_WSTRLIT:
        fprintf(h, "L\"%s\"", e->u.sval);
        break;
    case EXPR_CHARCONST:
        fprintf(h, "'%s'", e->u.sval);
        break;
    case EXPR_LOGNOT:
        fprintf(h, "!");
        write_expr(h, e->ref, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        break;
    case EXPR_NOT:
        fprintf(h, "~");
        write_expr(h, e->ref, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        break;
    case EXPR_POS:
        fprintf(h, "+");
        write_expr(h, e->ref, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        break;
    case EXPR_NEG:
        fprintf(h, "-");
        write_expr(h, e->ref, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        break;
    case EXPR_ADDRESSOF:
        fprintf(h, "&");
        write_expr(h, e->ref, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        break;
    case EXPR_PPTR:
        fprintf(h, "*");
        write_expr(h, e->ref, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        break;
    case EXPR_CAST:
        fprintf(h, "(");
        write_type_decl(h, &e->u.tref, NULL);
        fprintf(h, ")");
        write_expr(h, e->ref, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        break;
    case EXPR_SIZEOF:
        fprintf(h, "sizeof(");
        write_type_decl(h, &e->u.tref, NULL);
        fprintf(h, ")");
        break;
    case EXPR_SHL:
    case EXPR_SHR:
    case EXPR_MOD:
    case EXPR_MUL:
    case EXPR_DIV:
    case EXPR_ADD:
    case EXPR_SUB:
    case EXPR_AND:
    case EXPR_OR:
    case EXPR_LOGOR:
    case EXPR_LOGAND:
    case EXPR_XOR:
    case EXPR_EQUALITY:
    case EXPR_INEQUALITY:
    case EXPR_GTR:
    case EXPR_LESS:
    case EXPR_GTREQL:
    case EXPR_LESSEQL:
        if (brackets) fprintf(h, "(");
        write_expr(h, e->ref, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        switch (e->type)
        {
        case EXPR_SHL:          fprintf(h, " << "); break;
        case EXPR_SHR:          fprintf(h, " >> "); break;
        case EXPR_MOD:          fprintf(h, " %% "); break;
        case EXPR_MUL:          fprintf(h, " * "); break;
        case EXPR_DIV:          fprintf(h, " / "); break;
        case EXPR_ADD:          fprintf(h, " + "); break;
        case EXPR_SUB:          fprintf(h, " - "); break;
        case EXPR_AND:          fprintf(h, " & "); break;
        case EXPR_OR:           fprintf(h, " | "); break;
        case EXPR_LOGOR:        fprintf(h, " || "); break;
        case EXPR_LOGAND:       fprintf(h, " && "); break;
        case EXPR_XOR:          fprintf(h, " ^ "); break;
        case EXPR_EQUALITY:     fprintf(h, " == "); break;
        case EXPR_INEQUALITY:   fprintf(h, " != "); break;
        case EXPR_GTR:          fprintf(h, " > "); break;
        case EXPR_LESS:         fprintf(h, " < "); break;
        case EXPR_GTREQL:       fprintf(h, " >= "); break;
        case EXPR_LESSEQL:      fprintf(h, " <= "); break;
        default: break;
        }
        write_expr(h, e->u.ext, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        if (brackets) fprintf(h, ")");
        break;
    case EXPR_MEMBER:
        if (brackets) fprintf(h, "(");
        if (e->ref->type == EXPR_PPTR)
        {
            write_expr(h, e->ref->ref, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
            fprintf(h, "->");
        }
        else
        {
            write_expr(h, e->ref, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
            fprintf(h, ".");
        }
        write_expr(h, e->u.ext, 1, 0, toplevel_prefix, cont_type, "");
        if (brackets) fprintf(h, ")");
        break;
    case EXPR_COND:
        if (brackets) fprintf(h, "(");
        write_expr(h, e->ref, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        fprintf(h, " ? ");
        write_expr(h, e->u.ext, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        fprintf(h, " : ");
        write_expr(h, e->ext2, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        if (brackets) fprintf(h, ")");
        break;
    case EXPR_ARRAY:
        if (brackets) fprintf(h, "(");
        write_expr(h, e->ref, 1, toplevel, toplevel_prefix, cont_type, local_var_prefix);
        fprintf(h, "[");
        write_expr(h, e->u.ext, 1, 1, toplevel_prefix, cont_type, local_var_prefix);
        fprintf(h, "]");
        if (brackets) fprintf(h, ")");
        break;
    }
}

/* This is actually fairly involved to implement precisely, due to the
   effects attributes may have and things like that.  Right now this is
   only used for optimization, so just check for a very small set of
   criteria that guarantee the types are equivalent; assume every thing
   else is different.   */
static int compare_type(const type_t *a, const type_t *b)
{
    if (a == b
        || (a->name
            && b->name
            && strcmp(a->name, b->name) == 0))
        return 0;
    /* Ordering doesn't need to be implemented yet.  */
    return 1;
}

int compare_expr(const expr_t *a, const expr_t *b)
{
    int ret;

    if (a->type != b->type)
        return a->type - b->type;

    switch (a->type)
    {
        case EXPR_NUM:
        case EXPR_TRUEFALSE:
            return a->u.integer.value - b->u.integer.value;
        case EXPR_DOUBLE:
            return a->u.dval - b->u.dval;
        case EXPR_IDENTIFIER:
        case EXPR_STRLIT:
        case EXPR_WSTRLIT:
        case EXPR_CHARCONST:
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
        case EXPR_MOD:
        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_SHL:
        case EXPR_SHR:
        case EXPR_MEMBER:
        case EXPR_ARRAY:
        case EXPR_LOGOR:
        case EXPR_LOGAND:
        case EXPR_XOR:
        case EXPR_EQUALITY:
        case EXPR_INEQUALITY:
        case EXPR_GTR:
        case EXPR_LESS:
        case EXPR_GTREQL:
        case EXPR_LESSEQL:
            ret = compare_expr(a->ref, b->ref);
            if (ret != 0)
                return ret;
            return compare_expr(a->u.ext, b->u.ext);
        case EXPR_CAST:
            ret = compare_type(a->u.tref.type, b->u.tref.type);
            if (ret != 0)
                return ret;
            /* Fall through.  */
        case EXPR_NOT:
        case EXPR_NEG:
        case EXPR_PPTR:
        case EXPR_ADDRESSOF:
        case EXPR_LOGNOT:
        case EXPR_POS:
            return compare_expr(a->ref, b->ref);
        case EXPR_SIZEOF:
            return compare_type(a->u.tref.type, b->u.tref.type);
        case EXPR_VOID:
            return 0;
    }
    return -1;
}
