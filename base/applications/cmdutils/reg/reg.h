/*
 * Copyright 2017 Hugh McMaster
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

#ifndef __REG_H__
#define __REG_H__

#include <stdlib.h>
#include <windows.h>
#include "resource.h"

#define MAX_SUBKEY_LEN   257

/* reg.c */
struct reg_type_rels {
    DWORD type;
    const WCHAR *name;
};

extern const struct reg_type_rels type_rels[8];

void output_writeconsole(const WCHAR *str, DWORD wlen);
void WINAPIV output_message(unsigned int id, ...);
void WINAPIV output_string(const WCHAR *fmt, ...);
BOOL ask_confirm(unsigned int msgid, WCHAR *reg_info);
HKEY path_get_rootkey(const WCHAR *path);
WCHAR *build_subkey_path(WCHAR *path, DWORD path_len, WCHAR *subkey_name, DWORD subkey_len);
BOOL parse_registry_key(const WCHAR *key, HKEY *root, WCHAR **path, WCHAR **long_key);
BOOL is_switch(const WCHAR *s, const WCHAR c);

/* add.c */
int reg_add(HKEY root, WCHAR *path, WCHAR *value_name, BOOL value_empty,
            WCHAR *type, WCHAR separator, WCHAR *data, BOOL force);

/* delete.c */
int reg_delete(HKEY root, WCHAR *path, WCHAR *key_name, WCHAR *value_name,
               BOOL value_empty, BOOL value_all, BOOL force);

/* export.c */
int reg_export(int argc, WCHAR *argvW[]);

/* import.c */
int reg_import(int argc, WCHAR *argvW[]);

/* query.c */
int reg_query(int argc, WCHAR *argvW[]);

#endif /* __REG_H__ */
