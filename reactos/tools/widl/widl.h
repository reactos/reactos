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

extern int win32;
extern int pedantic;
extern int do_everything;
extern int do_header;
extern int do_typelib;
extern int do_proxies;
extern int do_client;
extern int do_server;
extern int do_idfile;
extern int do_dlldata;
extern int old_names;

extern char *input_name;
extern char *header_name;
extern char *local_stubs_name;
extern char *typelib_name;
extern char *dlldata_name;
extern char *proxy_name;
extern char *proxy_token;
extern char *client_name;
extern char *client_token;
extern char *server_name;
extern char *server_token;
extern const char *prefix_client;
extern const char *prefix_server;
extern time_t now;

extern int line_number;
extern int char_number;

extern FILE* header;
extern FILE* local_stubs;
extern FILE* idfile;

extern void write_proxies(const statement_list_t *stmts);
extern void write_client(const statement_list_t *stmts);
extern void write_server(const statement_list_t *stmts);
extern void write_dlldata(const statement_list_t *stmts);

#endif
