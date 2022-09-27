/*
 * Copyright 2021 Hugh McMaster
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

#ifndef __REG_TEST_H__
#define __REG_TEST_H__

#include <stdio.h>
#include <windows.h>
#include "wine/test.h"

/* Common #defines */
#define lok ok_(file,line)
#define KEY_WINE "Software\\Wine"
#define KEY_BASE KEY_WINE "\\reg_test"
#define REG_EXIT_SUCCESS 0
#define REG_EXIT_FAILURE 1

#define TODO_REG_TYPE    (0x0001u)
#define TODO_REG_SIZE    (0x0002u)
#define TODO_REG_DATA    (0x0004u)
#define TODO_REG_COMPARE (0x0008u)

/* add.c */
#define run_reg_exe(c,r) run_reg_exe_(__FILE__,__LINE__,c,r)
BOOL run_reg_exe_(const char *file, unsigned line, const char *cmd, DWORD *rc);

#define verify_reg(k,v,t,d,s,todo) verify_reg_(__FILE__,__LINE__,k,v,t,d,s,todo)
void verify_reg_(const char *file, unsigned line, HKEY hkey, const char *value,
                 DWORD exp_type, const void *exp_data, DWORD exp_size, DWORD todo);

#define verify_reg_nonexist(k,v) verify_reg_nonexist_(__FILE__,__LINE__,k,v)
void verify_reg_nonexist_(const char *file, unsigned line, HKEY hkey, const char *value);

#define open_key(r,p,s,k) open_key_(__FILE__,__LINE__,r,p,s,k)
void open_key_(const char *file, unsigned line, HKEY root, const char *path, REGSAM sam, HKEY *hkey);

#define close_key(k) close_key_(__FILE__,__LINE__,k)
void close_key_(const char *file, unsigned line, HKEY hkey);

#define verify_key(r,p,s) verify_key_(__FILE__,__LINE__,r,p,s)
void verify_key_(const char *file, unsigned line, HKEY root, const char *path, REGSAM sam);

#define verify_key_nonexist(r,p,s) verify_key_nonexist_(__FILE__,__LINE__,r,p,s)
void verify_key_nonexist_(const char *file, unsigned line, HKEY root, const char *path, REGSAM sam);

#define add_key(r,p,s,k) add_key_(__FILE__,__LINE__,r,p,s,k)
void add_key_(const char *file, unsigned line, const HKEY root, const char *path, REGSAM sam, HKEY *hkey);

#define delete_key(r,p,s) delete_key_(__FILE__,__LINE__,r,p,s)
void delete_key_(const char *file, unsigned line, HKEY root, const char *path, REGSAM sam);

#define delete_tree(r,p,s) delete_tree_(__FILE__,__LINE__,r,p,s)
LONG delete_tree_(const char *file, unsigned line, HKEY root, const char *path, REGSAM sam);

#define add_value(k,n,t,d,s) add_value_(__FILE__,__LINE__,k,n,t,d,s)
void add_value_(const char *file, unsigned line, HKEY hkey, const char *name,
                DWORD type, const void *data, size_t size);

#define delete_value(k,n) delete_value_(__FILE__,__LINE__,k,n)
void delete_value_(const char *file, unsigned line, HKEY hkey, const char *name);

/* export.c */
#define compare_export(f,e,todo) compare_export_(__FILE__,__LINE__,f,e,todo)
BOOL compare_export_(const char *file, unsigned line, const char *filename,
                     const char *expected, DWORD todo);
extern const char *empty_key_test;
extern const char *simple_data_test;
extern const char *complex_data_test;
extern const char *key_order_test;
extern const char *value_order_test;
extern const char *empty_hex_test;
extern const char *empty_hex_test2;
extern const char *hex_types_test;
extern const char *slashes_test;
extern const char *embedded_null_test;
extern const char *escaped_null_test;
extern const char *registry_view_test;

/* import.c */
BOOL is_elevated_process(void);

#define delete_file(f) delete_file_(__FILE__,__LINE__,f)
BOOL delete_file_(const char *file, unsigned line, const char *fname);

#define test_import_str(c,r) import_reg(__FILE__,__LINE__,c,FALSE,r)
#define test_import_wstr(c,r) import_reg(__FILE__,__LINE__,c,TRUE,r)
BOOL import_reg(const char *file, unsigned line, const char *contents, BOOL unicode, DWORD *rc);

#endif /* __REG_TEST_H__ */
