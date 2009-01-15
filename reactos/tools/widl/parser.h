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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WIDL_PARSER_H
#define __WIDL_PARSER_H

typedef struct
{
  type_t *interface;
  unsigned char old_pointer_default;
} interface_info_t;

int parser_parse(void);

extern FILE *parser_in;
extern char *parser_text;
extern int parser_debug;
extern int yy_flex_debug;

int parser_lex(void);

extern int import_stack_ptr;
int do_import(char *fname);
void abort_import(void);
void pop_import(void);

#define parse_only import_stack_ptr

int is_type(const char *name);

void check_functions(const type_t *iface);
func_list_t *gen_function_list(const statement_list_t *stmts);

#endif
