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

#ifndef __WIDL_WIDL_H
#define __WIDL_WIDL_H

#include "../tools.h"
#include "widltypes.h"

#include <time.h>

extern int debuglevel;
#define DEBUGLEVEL_NONE		0x0000
#define DEBUGLEVEL_CHAT		0x0001
#define DEBUGLEVEL_DUMP		0x0002
#define DEBUGLEVEL_TRACE	0x0004
#define DEBUGLEVEL_PPMSG	0x0008
#define DEBUGLEVEL_PPLEX	0x0010
#define DEBUGLEVEL_PPTRACE	0x0020

extern int pedantic;
extern int do_everything;
extern int do_header;
extern int do_typelib;
extern int do_proxies;
extern int do_client;
extern int do_server;
extern int do_regscript;
extern int do_idfile;
extern int do_dlldata;
extern int old_names;
extern int old_typelib;
extern int winrt_mode;
extern int interpreted_mode;
extern int use_abi_namespace;

extern char *input_name;
extern char *idl_name;
extern char *acf_name;
extern char *header_name;
extern char *header_token;
extern char *local_stubs_name;
extern char *typelib_name;
extern char *dlldata_name;
extern char *proxy_name;
extern char *proxy_token;
extern char *client_name;
extern char *client_token;
extern char *server_name;
extern char *server_token;
extern char *regscript_name;
extern char *regscript_token;
extern const char *prefix_client;
extern const char *prefix_server;
extern unsigned int packing;
extern unsigned int pointer_size;
extern struct target target;
extern time_t now;

extern int open_typelib( const char *name );

extern void write_header(const statement_list_t *stmts);
extern void write_id_data(const statement_list_t *stmts);
extern void write_proxies(const statement_list_t *stmts);
extern void write_client(const statement_list_t *stmts);
extern void write_server(const statement_list_t *stmts);
extern void write_regscript(const statement_list_t *stmts);
extern void write_typelib_regscript(const statement_list_t *stmts);
extern void output_typelib_regscript( const typelib_t *typelib );
extern void write_local_stubs(const statement_list_t *stmts);
extern void write_dlldata(const statement_list_t *stmts);

extern void start_cplusplus_guard(FILE *fp);
extern void end_cplusplus_guard(FILE *fp);

/* attribute.c */

extern attr_t *attr_int( struct location where, enum attr_type attr_type, unsigned int val );
extern attr_t *attr_ptr( struct location where, enum attr_type attr_type, void *val );

extern int is_attr( const attr_list_t *list, enum attr_type attr_type );
extern int is_ptrchain_attr( const var_t *var, enum attr_type attr_type );
extern int is_aliaschain_attr( const type_t *type, enum attr_type attr_type );

extern unsigned int get_attrv( const attr_list_t *list, enum attr_type attr_type );
extern void *get_attrp( const attr_list_t *list, enum attr_type attr_type );
extern void *get_aliaschain_attrp( const type_t *type, enum attr_type attr_type );

typedef int (*map_attrs_filter_t)( attr_list_t *, const attr_t * );
extern attr_list_t *append_attr( attr_list_t *list, attr_t *attr );
extern attr_list_t *append_attr_list( attr_list_t *new_list, attr_list_t *old_list );
extern attr_list_t *append_attribs( attr_list_t *, attr_list_t * );
extern attr_list_t *map_attrs( const attr_list_t *list, map_attrs_filter_t filter );
extern attr_list_t *move_attr( attr_list_t *dst, attr_list_t *src, enum attr_type type );

extern attr_list_t *check_apicontract_attrs( const char *name, attr_list_t *attrs );
extern attr_list_t *check_coclass_attrs( const char *name, attr_list_t *attrs );
extern attr_list_t *check_dispiface_attrs( const char *name, attr_list_t *attrs );
extern attr_list_t *check_enum_attrs( attr_list_t *attrs );
extern attr_list_t *check_enum_member_attrs( attr_list_t *attrs );
extern attr_list_t *check_field_attrs( const char *name, attr_list_t *attrs );
extern attr_list_t *check_function_attrs( const char *name, attr_list_t *attrs );
extern attr_list_t *check_interface_attrs( const char *name, attr_list_t *attrs );
extern attr_list_t *check_library_attrs( const char *name, attr_list_t *attrs );
extern attr_list_t *check_module_attrs( const char *name, attr_list_t *attrs );
extern attr_list_t *check_runtimeclass_attrs( const char *name, attr_list_t *attrs );
extern attr_list_t *check_struct_attrs( attr_list_t *attrs );
extern attr_list_t *check_typedef_attrs( attr_list_t *attrs );
extern attr_list_t *check_union_attrs( attr_list_t *attrs );
extern void check_arg_attrs( const var_t *arg );

#endif
