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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WIDL_HEADER_H
#define __WIDL_HEADER_H

extern int is_attr(attr_t *a, enum attr_type t);
extern void *get_attrp(attr_t *a, enum attr_type t);
extern unsigned long get_attrv(attr_t *a, enum attr_type t);
extern int is_void(type_t *t, var_t *v);
extern void write_name(FILE *h, var_t *v);
extern char* get_name(var_t *v);
extern void write_type(FILE *h, type_t *t, var_t *v, char *n);
extern int is_object(attr_t *a);
extern int is_local(attr_t *a);
extern var_t *is_callas(attr_t *a);
extern void write_args(FILE *h, var_t *arg, char *name, int obj, int do_indent);
extern void write_forward(type_t *iface);
extern void write_interface(type_t *iface);
extern void write_dispinterface(type_t *iface);
extern void write_coclass(class_t *iface);
extern void write_typedef(type_t *type, var_t *names);
extern void write_expr(FILE *h, expr_t *e);
extern void write_constdef(var_t *v);
extern void write_externdef(var_t *v);
extern var_t* get_explicit_handle_var(func_t* func);
#endif
