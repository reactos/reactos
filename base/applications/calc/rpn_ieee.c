/*
 * ReactOS Calc (RPN encoder/decoder for IEEE-754 engine)
 *
 * Copyright 2007-2017, Carlo Bramini
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "calc.h"

typedef struct {
    calc_node_t     node;
    void           *next;
} stack_node_t;

typedef void (*operator_call)(calc_number_t *, calc_number_t *, calc_number_t *);

typedef struct {
    unsigned int prec;
    operator_call op_f;
    operator_call op_i;
    operator_call op_p;
} calc_operator_t;

static stack_node_t *stack;
static calc_node_t   temp;
static BOOL          percent_mode;

static void rpn_add_f(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_sub_f(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_mul_f(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_div_f(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_mod_f(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_pow_f(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_sqr_f(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_and_f(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_or_f (calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_xor_f(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_shl_f(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_shr_f(calc_number_t *r, calc_number_t *a, calc_number_t *b);

/* Integer mode calculations */
static void rpn_add_i(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_sub_i(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_mul_i(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_div_i(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_mod_i(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_and_i(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_or_i (calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_xor_i(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_shl_i(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_shr_i(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_lsr_i(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_rol_i(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_ror_i(calc_number_t *r, calc_number_t *a, calc_number_t *b);

/* Percentage mode calculations */
static void rpn_add_p(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_sub_p(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_mul_p(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_div_p(calc_number_t *r, calc_number_t *a, calc_number_t *b);

static const calc_operator_t operator_list[] = {
    { 0, NULL,      NULL,      NULL,      }, // RPN_OPERATOR_PARENT
    { 0, NULL,      NULL,      NULL,      }, // RPN_OPERATOR_PERCENT
    { 0, NULL,      NULL,      NULL,      }, // RPN_OPERATOR_EQUAL
    { 1, rpn_or_f,  rpn_or_i,  NULL,      }, // RPN_OPERATOR_OR
    { 2, rpn_xor_f, rpn_xor_i, NULL,      }, // RPN_OPERATOR_XOR
    { 3, rpn_and_f, rpn_and_i, NULL,      }, // RPN_OPERATOR_AND
    { 4, rpn_shl_f, rpn_shl_i, NULL,      }, // RPN_OPERATOR_LSH
    { 4, rpn_shr_f, rpn_shr_i, NULL,      }, // RPN_OPERATOR_RSH
    { 4, NULL,      rpn_lsr_i, NULL,      }, // RPN_OPERATOR_LSR
    { 4, NULL,      rpn_rol_i, NULL,      }, // RPN_OPERATOR_ROL
    { 4, NULL,      rpn_ror_i, NULL,      }, // RPN_OPERATOR_ROR
    { 5, rpn_add_f, rpn_add_i, rpn_add_p, }, // RPN_OPERATOR_ADD
    { 5, rpn_sub_f, rpn_sub_i, rpn_sub_p, }, // RPN_OPERATOR_SUB
    { 6, rpn_mul_f, rpn_mul_i, rpn_mul_p, }, // RPN_OPERATOR_MULT
    { 6, rpn_div_f, rpn_div_i, rpn_div_p, }, // RPN_OPERATOR_DIV
    { 6, rpn_mod_f, rpn_mod_i, NULL,      }, // RPN_OPERATOR_MOD
    { 7, rpn_pow_f, NULL,      NULL,      }, // RPN_OPERATOR_POW
    { 7, rpn_sqr_f, NULL,      NULL,      }, // RPN_OPERATOR_SQR
};

static calc_node_t *pop(void)
{
    void *next;

    if (stack == NULL)
        return NULL;

    /* copy the node */
    temp = stack->node;
    next = stack->next;

    /* free the node */
    free(stack);
    stack = next;

    return &temp;
}

static int is_stack_empty(void)
{
    return (stack == NULL);
}

static void push(calc_node_t *op)
{
    stack_node_t *z = (stack_node_t *)malloc(sizeof(stack_node_t));

    z->node = *op;
    z->next = stack;
    stack = z;
}
/*
static unsigned int get_prec(unsigned int opc)
{
    unsigned int x;

    for (x=0; x<SIZEOF(operator_list); x++)
        if (operator_list[x].opc == opc) break;
    return operator_list[x].prec;
}
*/
/* Real mode calculations */
static void rpn_add_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->f = a->f + b->f;
}

static void rpn_sub_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->f = a->f - b->f;
}

static void rpn_mul_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->f = a->f * b->f;
}

static void rpn_div_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    if (b->f == 0)
        calc.is_nan = TRUE;
    else
        r->f = a->f / b->f;
}

static void rpn_mod_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    double t;

    if (b->f == 0)
        calc.is_nan = TRUE;
    else {
        modf(a->f/b->f, &t);
        r->f = a->f - (t * b->f);
    }
}

static void rpn_and_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    calc_number_t ai, bi;

    ai.i = logic_dbl2int(a);
    bi.i = logic_dbl2int(b);

    r->f = (long double)(ai.i & bi.i);
}

static void rpn_or_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    calc_number_t ai, bi;

    ai.i = logic_dbl2int(a);
    bi.i = logic_dbl2int(b);

    r->f = (long double)(ai.i | bi.i);
}

static void rpn_xor_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    calc_number_t ai, bi;

    ai.i = logic_dbl2int(a);
    bi.i = logic_dbl2int(b);

    r->f = (long double)(ai.i ^ bi.i);
}

static void rpn_shl_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    calc_number_t n;

    modf(b->f, &n.f);

    r->f = a->f * pow(2., n.f);
}

static void rpn_shr_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    calc_number_t n;

    modf(b->f, &n.f);

    r->f = a->f / pow(2., n.f);
}

static void rpn_pow_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->f = pow(a->f, b->f);
    if (_finite(r->f) == 0 || _isnan(r->f))
        calc.is_nan = TRUE;
}

static void rpn_sqr_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    if (b->f == 0)
        calc.is_nan = TRUE;
    else {
        r->f = pow(a->f, 1./b->f);
        if (_finite(r->f) == 0 || _isnan(r->f))
            calc.is_nan = TRUE;
    }
}

/* Integer mode calculations */
static void rpn_add_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->i = a->i + b->i;
}

static void rpn_sub_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->i = a->i - b->i;
}

static void rpn_mul_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->i = a->i * b->i;
}

static void rpn_div_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    if (b->i == 0)
        calc.is_nan = TRUE;
    else
        r->i = a->i / b->i;
}

static void rpn_mod_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    if (b->i == 0)
        calc.is_nan = TRUE;
    else
        r->i = a->i % b->i;
}

static void rpn_and_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->i = a->i & b->i;
}

static void rpn_or_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->i = a->i | b->i;
}

static void rpn_xor_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->i = a->i ^ b->i;
}

static void rpn_shl_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->i = a->i << b->i;
}

static void rpn_shr_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->i = a->i >> b->i;
}

static void rpn_lsr_i(calc_number_t *r, calc_number_t *a, calc_number_t *b) {
    if (b->i < 0) { r->i = a->i; calc.is_nan = TRUE; return; } // Shift count cannot be negative
    if (b->i >= 64) { r->i = 0; return; } // Shift is out of QWORD range
    r->i = (INT64)((UINT64)a->i >> (UINT64)b->i);
    // apply_int_mask will be called by run_operator
}

static void rpn_rol_i(calc_number_t *r, calc_number_t *a, calc_number_t *b) {
    UINT64 val = (UINT64)a->i;
    UINT shift = (UINT)b->i; // Implicitly takes lower bits, negative becomes large positive
    UINT bits;
    UINT64 mask;

    if (b->i < 0) { r->i = a->i; calc.is_nan = TRUE; return; } // Shift count cannot be negative

    switch (calc.size) {
        case IDC_RADIO_DWORD: bits = 32; mask = 0xFFFFFFFFULL; break;
        case IDC_RADIO_WORD:  bits = 16; mask = 0xFFFFULL;     break;
        case IDC_RADIO_BYTE:  bits = 8;  mask = 0xFFULL;       break;
        case IDC_RADIO_QWORD: default: bits = 64; mask = 0xFFFFFFFFFFFFFFFFULL; break;
    }
    val &= mask;
    if (bits == 0) { r->i = (INT64)val; return; } // Should not happen with current calc.size values
    shift %= bits;
    if (shift == 0) { r->i = (INT64)val; return; }
    r->i = (INT64)(((val << shift) | (val >> (bits - shift))) & mask);
}

static void rpn_ror_i(calc_number_t *r, calc_number_t *a, calc_number_t *b) {
    UINT64 val = (UINT64)a->i;
    UINT shift = (UINT)b->i; // Implicitly takes lower bits, negative becomes large positive
    UINT bits;
    UINT64 mask;

    if (b->i < 0) { r->i = a->i; calc.is_nan = TRUE; return; } // Shift count cannot be negative

    switch (calc.size) {
        case IDC_RADIO_DWORD: bits = 32; mask = 0xFFFFFFFFULL; break;
        case IDC_RADIO_WORD:  bits = 16; mask = 0xFFFFULL;     break;
        case IDC_RADIO_BYTE:  bits = 8;  mask = 0xFFULL;       break;
        case IDC_RADIO_QWORD: default: bits = 64; mask = 0xFFFFFFFFFFFFFFFFULL; break;
    }
    val &= mask;
    if (bits == 0) { r->i = (INT64)val; return; } // Should not happen
    shift %= bits;
    if (shift == 0) { r->i = (INT64)val; return; }
    r->i = (INT64)(((val >> shift) | (val << (bits - shift))) & mask);
}

/* Percent mode calculations */
static void rpn_add_p(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->f = a->f * (1. + b->f/100.);
}

static void rpn_sub_p(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->f = a->f * (1. - b->f/100.);
}

static void rpn_mul_p(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    r->f = a->f * b->f / 100.;
}

static void rpn_div_p(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    if (b->f == 0)
        calc.is_nan = TRUE;
    else
        r->f = a->f * 100. / b->f;
}

void run_operator(calc_node_t *result,
                  calc_node_t *a,
                  calc_node_t *b,
                  unsigned int operation)
{
    calc_number_t da, db, dc;
    DWORD         base = calc.base;

    da = a->number;
    db = b->number;
    if (a->base == IDC_RADIO_DEC && b->base != IDC_RADIO_DEC) {
        db.f = logic_int2dbl(&b->number);
        base = IDC_RADIO_DEC;
    } else
    if (a->base != IDC_RADIO_DEC && b->base == IDC_RADIO_DEC) {
        da.f = logic_int2dbl(&a->number);
        base = IDC_RADIO_DEC;
    }

    if (base == IDC_RADIO_DEC) {
        if (percent_mode) {
            percent_mode = FALSE;
            operator_list[operation].op_p(&dc, &da, &db);
        } else
            operator_list[operation].op_f(&dc, &da, &db);
        if (_finite(dc.f) == 0)
            calc.is_nan = TRUE;
    } else {
        operator_list[operation].op_i(&dc, &da, &db);
        /* apply final limiter to result */
        apply_int_mask(&dc);
    }

    if (a->base == IDC_RADIO_DEC && b->base != IDC_RADIO_DEC) {
        result->number.i = logic_dbl2int(&dc);
        apply_int_mask(&result->number);
    } else
    if (a->base != IDC_RADIO_DEC && b->base == IDC_RADIO_DEC)
        result->number.f = dc.f;
    else
        result->number = dc;
}

static void evalStack(calc_number_t *number)
{
    calc_node_t *op, ip;
    unsigned int prec;

    op = pop();
    ip = *op;
    prec = operator_list[ip.operation].prec;
    while (!is_stack_empty()) {
        op = pop();

        if (prec <= operator_list[op->operation].prec) {
            if (op->operation == RPN_OPERATOR_PARENT) continue;

            calc.prev = ip.number;
            run_operator(&ip, op, &ip, op->operation);
            if (calc.is_nan) {
                flush_postfix();
                return;
            }
        } else {
            push(op);
            break;
        }
    }

    if (ip.operation != RPN_OPERATOR_EQUAL && ip.operation != RPN_OPERATOR_PERCENT)
        push(&ip);

    calc.prev_operator = op->operation;

    *number = ip.number;
}

int exec_infix2postfix(calc_number_t *number, unsigned int func)
{
    calc_node_t tmp;

    if (is_stack_empty() && func == RPN_OPERATOR_EQUAL) {
        /* if a number has been entered with exponential */
        /* notation, I may update it with normal mode */
        if (calc.sci_in)
            return 1;
        return 0;
    }

    if (func == RPN_OPERATOR_PERCENT)
        percent_mode = TRUE;

    tmp.number = *number;
    tmp.base = calc.base;
    tmp.operation = func;

    push(&tmp);

    if (func == RPN_OPERATOR_NONE)
        return 0;

    if (func != RPN_OPERATOR_PARENT) {
        calc.last_operator = func;
        evalStack(number);
    }
    return 1;
}

void exec_change_infix(void)
{
    stack_node_t *op = stack;

    if (op == NULL)
        return;
    if (op->node.operation == RPN_OPERATOR_PARENT ||
        op->node.operation == RPN_OPERATOR_PERCENT ||
        op->node.operation == RPN_OPERATOR_EQUAL)
        return;
    /* remove the head, it will be re-inserted with new operator */
    pop();
}

void exec_closeparent(calc_number_t *number)
{
    calc_node_t *op, ip;

    ip.number = *number;
    ip.base = calc.base;
    while (!is_stack_empty()) {
        op = pop();

        if (op->operation == RPN_OPERATOR_PARENT)
            break;

        run_operator(&ip, op, &ip, op->operation);
        if (calc.is_nan) {
            flush_postfix();
            return;
        }
    }
    *number = ip.number;
}

int eval_parent_count(void)
{
    stack_node_t *s = stack;
    int           n = 0;

    while (s != NULL) {
        if (s->node.operation == RPN_OPERATOR_PARENT)
            n++;
        s = (stack_node_t *)(s->next);
    }
    return n;
}

void flush_postfix(void)
{
    while (!is_stack_empty())
        pop();
    /* clear prev and last typed operators */
    calc.prev_operator =
    calc.last_operator = 0;
}

void start_rpn_engine(void)
{
    stack = NULL;
}

void stop_rpn_engine(void)
{
}
