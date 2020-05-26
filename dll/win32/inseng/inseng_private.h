/*
 * Copyright 2016 Michael Müller
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

#pragma once

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#ifdef __REACTOS__
#include "winnls.h"
#endif
#include "ole2.h"
#include "rpcproxy.h"
#include "inseng.h"
#include "wine/heap.h"


static inline char *strdupA(const char *src)
{
    char *dest = heap_alloc(strlen(src) + 1);
    if (dest) strcpy(dest, src);
    return dest;
}

static inline WCHAR *strdupW(const WCHAR *src)
{
    WCHAR *dest;
    if (!src) return NULL;
    dest = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(src) + 1) * sizeof(WCHAR));
    if (dest) lstrcpyW(dest, src);
    return dest;
}

static inline LPWSTR strAtoW(const char *str)
{
    LPWSTR ret = NULL;

    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR))))
            MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

struct inf_value;
struct inf_section;
struct inf_file;

HRESULT inf_load(const char *path, struct inf_file **inf_file) DECLSPEC_HIDDEN;
void inf_free(struct inf_file *inf) DECLSPEC_HIDDEN;

BOOL inf_next_section(struct inf_file *inf, struct inf_section **sec) DECLSPEC_HIDDEN;
struct inf_section *inf_get_section(struct inf_file *inf, const char *name) DECLSPEC_HIDDEN;
char *inf_section_get_name(struct inf_section *section) DECLSPEC_HIDDEN;
BOOL inf_section_next_value(struct inf_section *sec, struct inf_value **value) DECLSPEC_HIDDEN;

struct inf_value *inf_get_value(struct inf_section *sec, const char *key) DECLSPEC_HIDDEN;
char *inf_value_get_key(struct inf_value *value) DECLSPEC_HIDDEN;
char *inf_value_get_value(struct inf_value *value) DECLSPEC_HIDDEN;

char *trim(char *str, char **last_chr, BOOL strip_quotes) DECLSPEC_HIDDEN;

void component_set_actual_download_size(ICifComponent *iface, DWORD size) DECLSPEC_HIDDEN;
void component_set_downloaded(ICifComponent *iface, BOOL value) DECLSPEC_HIDDEN;
void component_set_installed(ICifComponent *iface, BOOL value) DECLSPEC_HIDDEN;
 char *component_get_id(ICifComponent *iface) DECLSPEC_HIDDEN;
