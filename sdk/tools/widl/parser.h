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

#include "widltypes.h"

int parser_parse(void);

extern void parser_warning( const struct location *where, const char *message );
extern void parser_error( const struct location *where, const char *message );
extern void init_location( struct location *copy, const struct location *begin, const struct location *end );

extern FILE *parser_in;
extern int parser_debug;
extern int yy_flex_debug;

extern int parse_only;

int is_type(const char *name);

int do_warning(const char *toggle, warning_list_t *wnum);
int is_warning_enabled(int warning);

extern char *find_input_file( const char *name, const char *parent );
extern FILE *open_input_file( const char *path );
extern void close_all_inputs(void);

#endif
