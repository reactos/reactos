/*
 * ReactOS Calc (RPN encoder/decoder for GMP/MPFR engine)
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

/* Percentage mode calculations */
static void rpn_add_p(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_sub_p(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_mul_p(calc_number_t *r, calc_number_t *a, calc_number_t *b);
static void rpn_div_p(calc_number_t *r, calc_number_t *a, calc_number_t *b);

static const calc_operator_t operator_list[] = {
    { 0, NULL,      NULL,      NULL,      }, // RPN_OPERATOR_PARENT
    { 0, NULL,      NULL,      NULL,      }, // RPN_OPERATOR_PERCENT
    { 0, NULL,      NULL,      NULL,      }, // RPN_OPERATOR_EQUAL
    { 1, rpn_or_f,  rpn_or_f,  NULL,      }, // RPN_OPERATOR_OR
    { 2, rpn_xor_f, rpn_xor_f, NULL,      }, // RPN_OPERATOR_XOR
    { 3, rpn_and_f, rpn_and_f, NULL,      }, // RPN_OPERATOR_AND
    { 4, rpn_shl_f, rpn_shl_f, NULL,      }, // RPN_OPERATOR_LSH
    { 4, rpn_shr_f, rpn_shr_f, NULL,      }, // RPN_OPERATOR_RSH
    { 5, rpn_add_f, rpn_add_i, rpn_add_p, }, // RPN_OPERATOR_ADD
    { 5, rpn_sub_f, rpn_sub_i, rpn_sub_p, }, // RPN_OPERATOR_SUB
    { 6, rpn_mul_f, rpn_mul_i, rpn_mul_p, }, // RPN_OPERATOR_MULT
    { 6, rpn_div_f, rpn_div_i, rpn_div_p, }, // RPN_OPERATOR_DIV
    { 6, rpn_mod_i, rpn_mod_i, NULL,      }, // RPN_OPERATOR_MOD
    { 7, rpn_pow_f, NULL,      NULL,      }, // RPN_OPERATOR_POW
    { 7, rpn_sqr_f, NULL,      NULL,      }, // RPN_OPERATOR_SQR
};

static void node_copy(calc_node_t *dst, calc_node_t *src)
{
    mpfr_set(dst->number.mf, src->number.mf, MPFR_DEFAULT_RND);
    dst->operation = src->operation;
}

static calc_node_t *pop(void)
{
    void *next;

    if (stack == NULL)
        return NULL;

    /* copy the node */
    node_copy(&temp, &stack->node);
    next = stack->next;

    /* free the node */
    mpfr_clear(stack->node.number.mf);
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

    mpfr_init_set(z->node.number.mf, op->number.mf, MPFR_DEFAULT_RND);
    z->node.operation = op->operation;
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

typedef void (*exec_call_t)
__GMP_PROTO ((mpz_ptr, mpz_srcptr, mpz_srcptr));

static void rpn_exec_int(calc_number_t *r, calc_number_t *a, calc_number_t *b, exec_call_t cb)
{
    mpz_t ai, bi;

    mpz_init(ai);
    mpz_init(bi);
    mpfr_get_z(ai, a->mf, MPFR_DEFAULT_RND);
    mpfr_get_z(bi, b->mf, MPFR_DEFAULT_RND);
    cb(ai, ai, bi);
    mpfr_set_z(r->mf, ai, MPFR_DEFAULT_RND);
    mpz_clear(ai);
    mpz_clear(bi);
}


/* Real mode calculations */
static void rpn_add_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    mpfr_add(r->mf, a->mf, b->mf, MPFR_DEFAULT_RND);
}

static void rpn_sub_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    mpfr_sub(r->mf, a->mf, b->mf, MPFR_DEFAULT_RND);
}

static void rpn_mul_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    mpfr_mul(r->mf, a->mf, b->mf, MPFR_DEFAULT_RND);
}

static void rpn_div_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    if (mpfr_sgn(b->mf) == 0)
        calc.is_nan = TRUE;
    else
        mpfr_div(r->mf, a->mf, b->mf, MPFR_DEFAULT_RND);
}

static void rpn_and_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    rpn_exec_int(r, a, b, mpz_and);
}

static void rpn_or_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    rpn_exec_int(r, a, b, mpz_ior);
}

static void rpn_xor_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    rpn_exec_int(r, a, b, mpz_xor);
}

static void rpn_shl_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    unsigned long e;

    mpfr_trunc(r->mf, b->mf);
    if (mpfr_fits_ulong_p(r->mf, MPFR_DEFAULT_RND) == 0)
        calc.is_nan = TRUE;
    else {
        e = mpfr_get_ui(r->mf, MPFR_DEFAULT_RND);
        mpfr_mul_2exp(r->mf, a->mf, e, MPFR_DEFAULT_RND);
    }
}

static void rpn_shr_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    unsigned long e;

    mpfr_trunc(r->mf, b->mf);
    if (mpfr_fits_ulong_p(r->mf, MPFR_DEFAULT_RND) == 0)
        calc.is_nan = TRUE;
    else {
        e = mpfr_get_ui(r->mf, MPFR_DEFAULT_RND);
        mpfr_div_2exp(r->mf, a->mf, e, MPFR_DEFAULT_RND);
    }
}

static void rpn_pow_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    mpfr_pow(r->mf, a->mf, b->mf, MPFR_DEFAULT_RND);
}

static void rpn_sqr_f(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    if (mpfr_sgn(b->mf) == 0)
        calc.is_nan = TRUE;
    else {
        mpfr_t tmp;

        mpfr_init(tmp);
        mpfr_set(tmp, b->mf, MPFR_DEFAULT_RND);
        mpfr_ui_div(tmp, 1, tmp, MPFR_DEFAULT_RND);
        mpfr_pow(r->mf, a->mf, tmp, MPFR_DEFAULT_RND);
        mpfr_clear(tmp);
    }
}

/* Integer mode calculations */
static void rpn_add_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    rpn_exec_int(r, a, b, mpz_add);
}

static void rpn_sub_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    rpn_exec_int(r, a, b, mpz_sub);
}

static void rpn_mul_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    rpn_exec_int(r, a, b, mpz_mul);
}

static void rpn_div_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    if (mpfr_sgn(b->mf) == 0)
        calc.is_nan = TRUE;
    else
        rpn_exec_int(r, a, b, mpz_tdiv_q);
}

static void rpn_mod_i(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    if (mpfr_sgn(b->mf) == 0)
        calc.is_nan = TRUE;
    else
        rpn_exec_int(r, a, b, mpz_tdiv_r);
}

/* Percent mode calculations */
static void rpn_add_p(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    mpfr_t tmp;

    mpfr_init(tmp);
    mpfr_set(tmp, b->mf, MPFR_DEFAULT_RND);
    mpfr_div_ui(tmp, tmp, 100, MPFR_DEFAULT_RND);
    mpfr_add_ui(tmp, tmp, 1, MPFR_DEFAULT_RND);
    mpfr_mul(r->mf, a->mf, tmp, MPFR_DEFAULT_RND);
    mpfr_clear(tmp);
}

static void rpn_sub_p(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    mpfr_t tmp;

    mpfr_init(tmp);
    mpfr_set(tmp, b->mf, MPFR_DEFAULT_RND);
    mpfr_div_ui(tmp, tmp, 100, MPFR_DEFAULT_RND);
    mpfr_sub_ui(tmp, tmp, 1, MPFR_DEFAULT_RND);
    mpfr_mul(r->mf, a->mf, tmp, MPFR_DEFAULT_RND);
    mpfr_clear(tmp);
}

static void rpn_mul_p(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    mpfr_mul(r->mf, a->mf, b->mf, MPFR_DEFAULT_RND);
    mpfr_div_ui(r->mf, r->mf, 100, MPFR_DEFAULT_RND);
}

static void rpn_div_p(calc_number_t *r, calc_number_t *a, calc_number_t *b)
{
    if (mpfr_sgn(b->mf) == 0)
        calc.is_nan = TRUE;
    else {
        mpfr_mul_ui(r->mf, a->mf, 100, MPFR_DEFAULT_RND);
        mpfr_div(r->mf, r->mf, b->mf, MPFR_DEFAULT_RND);
    }
}

void run_operator(calc_node_t *result,
                  calc_node_t *a,
                  calc_node_t *b,
                  unsigned int operation)
{
    if (calc.base == IDC_RADIO_DEC) {
        if (percent_mode) {
            percent_mode = FALSE;
            operator_list[operation].op_p(&result->number, &a->number, &b->number);
        } else
            operator_list[operation].op_f(&result->number, &a->number, &b->number);
    } else {
        operator_list[operation].op_i(&result->number, &a->number, &b->number);
        /* apply final limiter to result */
        apply_int_mask(&result->number);
    }
}

static void evalStack(calc_number_t *number)
{
    calc_node_t *op, ip;
    unsigned int prec;

    mpfr_init(ip.number.mf);
    op = pop();
    node_copy(&ip, op);
    prec = operator_list[ip.operation].prec;
    while (!is_stack_empty()) {
        op = pop();

        if (prec <= operator_list[op->operation].prec) {
            if (op->operation == RPN_OPERATOR_PARENT) continue;

            rpn_copy(&calc.prev, &ip.number);
            run_operator(&ip, op, &ip, op->operation);
            if (calc.is_nan) {
                flush_postfix();
                mpfr_clear(ip.number.mf);
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

    rpn_copy(number, &ip.number);
    mpfr_clear(ip.number.mf);
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

    mpfr_init(tmp.number.mf);
    rpn_copy(&tmp.number, number);
    tmp.operation = func;

    push(&tmp);
    mpfr_clear(tmp.number.mf);

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

    rpn_alloc(&ip.number);
    rpn_copy(&ip.number, number);
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
    rpn_copy(number, &ip.number);
    rpn_free(&ip.number);
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
    mpf_set_default_prec(512);
    mpfr_set_default_prec(512);
    stack = NULL;
    mpfr_init(calc.code.mf);
    mpfr_init(calc.prev.mf);
    mpfr_init(calc.memory.number.mf);
    mpfr_init(temp.number.mf);
    rpn_zero(&calc.memory.number);
}

void stop_rpn_engine(void)
{
    mpfr_clear(calc.code.mf);
    mpfr_clear(calc.prev.mf);
    mpfr_clear(calc.memory.number.mf);
    mpfr_clear(temp.number.mf);
}
