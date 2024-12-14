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

extern int pedantic;
extern int do_everything;
extern int do_header;
extern int do_typelib;
extern int do_old_typelib;
extern int do_proxies;
extern int do_client;
extern int do_server;
extern int do_regscript;
extern int do_idfile;
extern int do_dlldata;
extern int old_names;
extern int win32_packing;
extern int win64_packing;
extern int winrt_mode;
extern int use_abi_namespace;

extern char *input_name;
extern char *input_idl_name;
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
extern unsigned int pointer_size;
extern time_t now;

extern int line_number;
extern int char_number;

enum target_cpu
{
    CPU_x86, CPU_x86_64, CPU_POWERPC, CPU_ARM, CPU_ARM64, CPU_LAST = CPU_ARM64
};

extern enum target_cpu target_cpu;

enum stub_mode
{
    MODE_Os,  /* inline stubs */
    MODE_Oi,  /* old-style interpreted stubs */
    MODE_Oif  /* new-style fully interpreted stubs */
};
extern enum stub_mode get_stub_mode(void);

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

#endif
