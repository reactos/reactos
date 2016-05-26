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

struct expr_loc
{
    const var_t *v;
    const char *attr;
};

extern expr_t *make_expr(enum expr_type type);
extern expr_t *make_exprl(enum expr_type type, int val);
extern expr_t *make_exprd(enum expr_type type, double val);
extern expr_t *make_exprs(enum expr_type type, char *val);
extern expr_t *make_exprt(enum expr_type type, var_t *var, expr_t *expr);
extern expr_t *make_expr1(enum expr_type type, expr_t *expr);
extern expr_t *make_expr2(enum expr_type type, expr_t *exp1, expr_t *exp2);
extern expr_t *make_expr3(enum expr_type type, expr_t *expr1, expr_t *expr2, expr_t *expr3);

extern const type_t *expr_resolve_type(const struct expr_loc *expr_loc, const type_t *cont_type, const expr_t *expr);
extern int compare_expr(const expr_t *a, const expr_t *b);

extern void write_expr(FILE *h, const expr_t *e, int brackets, int toplevel, const char *toplevel_prefix,
                       const type_t *cont_type, const char *local_var_prefix);
