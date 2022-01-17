/*
 * Copyright 2008 Andrew Riedi
 * Copyright 2016-2017, 2021 Hugh McMaster
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

#include "reg.h"
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(reg);

static const struct
{
    HKEY key;
    const WCHAR *short_name;
    const WCHAR *long_name;
}
root_rels[] =
{
    {HKEY_LOCAL_MACHINE,  L"HKLM", L"HKEY_LOCAL_MACHINE"},
    {HKEY_CURRENT_USER,   L"HKCU", L"HKEY_CURRENT_USER"},
    {HKEY_CLASSES_ROOT,   L"HKCR", L"HKEY_CLASSES_ROOT"},
    {HKEY_USERS,          L"HKU",  L"HKEY_USERS"},
    {HKEY_CURRENT_CONFIG, L"HKCC", L"HKEY_CURRENT_CONFIG"},
};

const struct reg_type_rels type_rels[] =
{
    {REG_NONE,                L"REG_NONE"},
    {REG_SZ,                  L"REG_SZ"},
    {REG_EXPAND_SZ,           L"REG_EXPAND_SZ"},
    {REG_BINARY,              L"REG_BINARY"},
    {REG_DWORD,               L"REG_DWORD"},
    {REG_DWORD_LITTLE_ENDIAN, L"REG_DWORD_LITTLE_ENDIAN"},
    {REG_DWORD_BIG_ENDIAN,    L"REG_DWORD_BIG_ENDIAN"},
    {REG_MULTI_SZ,            L"REG_MULTI_SZ"},
};

void output_writeconsole(const WCHAR *str, DWORD wlen)
{
    DWORD count, ret;

    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), str, wlen, &count, NULL);
    if (!ret)
    {
        DWORD len;
        char  *msgA;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile(), assuming the console encoding is still the right
         * one in that case.
         */
        len = WideCharToMultiByte(GetConsoleOutputCP(), 0, str, wlen, NULL, 0, NULL, NULL);
        msgA = malloc(len);

        WideCharToMultiByte(GetConsoleOutputCP(), 0, str, wlen, msgA, len, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        free(msgA);
    }
}

static void output_formatstring(const WCHAR *fmt, va_list va_args)
{
    WCHAR *str;
    DWORD len;

    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         fmt, 0, 0, (WCHAR *)&str, 0, &va_args);
    if (len == 0 && GetLastError() != ERROR_NO_WORK_DONE)
    {
        WINE_FIXME("Could not format string: le=%u, fmt=%s\n", GetLastError(), wine_dbgstr_w(fmt));
        return;
    }
    output_writeconsole(str, len);
    LocalFree(str);
}

void WINAPIV output_message(unsigned int id, ...)
{
    WCHAR *fmt = NULL;
    int len;
    va_list va_args;

    if (!(len = LoadStringW(GetModuleHandleW(NULL), id, (WCHAR *)&fmt, 0)))
    {
        WINE_FIXME("LoadString failed with %d\n", GetLastError());
        return;
    }

    len++;
    fmt = malloc(len * sizeof(WCHAR));
    if (!fmt) return;

    LoadStringW(GetModuleHandleW(NULL), id, fmt, len);

    va_start(va_args, id);
    output_formatstring(fmt, va_args);
    va_end(va_args);

    free(fmt);
}

void WINAPIV output_string(const WCHAR *fmt, ...)
{
    va_list va_args;

    va_start(va_args, fmt);
    output_formatstring(fmt, va_args);
    va_end(va_args);
}

/* ask_confirm() adapted from programs/cmd/builtins.c */
BOOL ask_confirm(unsigned int msgid, WCHAR *reg_info)
{
    HMODULE hmod;
    WCHAR Ybuffer[4];
    WCHAR Nbuffer[4];
    WCHAR defval[32];
    WCHAR answer[MAX_PATH];
    WCHAR *str;
    DWORD count;

    hmod = GetModuleHandleW(NULL);
    LoadStringW(hmod, STRING_YES, Ybuffer, ARRAY_SIZE(Ybuffer));
    LoadStringW(hmod, STRING_NO,  Nbuffer, ARRAY_SIZE(Nbuffer));
    LoadStringW(hmod, STRING_DEFAULT_VALUE, defval, ARRAY_SIZE(defval));

    str = (reg_info && *reg_info) ? reg_info : defval;

    while (1)
    {
        output_message(msgid, str);
        output_message(STRING_YESNO);
        ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), answer, ARRAY_SIZE(answer), &count, NULL);
        answer[0] = towupper(answer[0]);
        if (answer[0] == Ybuffer[0])
            return TRUE;
        if (answer[0] == Nbuffer[0])
            return FALSE;
    }
}

static inline BOOL path_rootname_cmp(const WCHAR *input_path, const WCHAR *rootkey_name)
{
    DWORD length = lstrlenW(rootkey_name);

    return (!_wcsnicmp(input_path, rootkey_name, length) &&
            (input_path[length] == 0 || input_path[length] == '\\'));
}

HKEY path_get_rootkey(const WCHAR *path)
{
    DWORD i;

    for (i = 0; i < ARRAY_SIZE(root_rels); i++)
    {
        if (path_rootname_cmp(path, root_rels[i].short_name) ||
            path_rootname_cmp(path, root_rels[i].long_name))
            return root_rels[i].key;
    }

    return NULL;
}

static BOOL sane_path(const WCHAR *key)
{
    unsigned int i = lstrlenW(key);

    if (i < 3 || (key[i - 1] == '\\' && key[i - 2] == '\\'))
    {
        output_message(STRING_INVALID_KEY);
        return FALSE;
    }

    if (key[0] == '\\' && key[1] == '\\' && key[2] != '\\')
    {
        output_message(STRING_NO_REMOTE);
        return FALSE;
    }

    return TRUE;
}

WCHAR *build_subkey_path(WCHAR *path, DWORD path_len, WCHAR *subkey_name, DWORD subkey_len)
{
    WCHAR *subkey_path;

    subkey_path = malloc((path_len + subkey_len + 2) * sizeof(WCHAR));
    swprintf(subkey_path, L"%s\\%s", path, subkey_name);

    return subkey_path;
}

WCHAR *get_long_key(HKEY root, WCHAR *path)
{
    int i, len;
    WCHAR *long_key;

    for (i = 0; i < ARRAY_SIZE(root_rels); i++)
    {
        if (root == root_rels[i].key)
            break;
    }

    len = lstrlenW(root_rels[i].long_name);

    if (!path)
    {
        long_key = malloc((len + 1) * sizeof(WCHAR));
        lstrcpyW(long_key, root_rels[i].long_name);
        return long_key;
    }

    len += lstrlenW(path) + 1; /* add one for the concatenating backslash */
    long_key = malloc((len + 1) * sizeof(WCHAR));
    swprintf(long_key, L"%s\\%s", root_rels[i].long_name, path);
    return long_key;
}

BOOL parse_registry_key(const WCHAR *key, HKEY *root, WCHAR **path)
{
    WCHAR *p;

    if (!sane_path(key))
        return FALSE;

    *root = path_get_rootkey(key);
    if (!*root)
    {
        output_message(STRING_INVALID_SYSTEM_KEY);
        return FALSE;
    }

    *path = wcschr(key, '\\');

    if (!*path)
        return TRUE;

    (*path)++;

    if (!**path)
    {
        output_message(STRING_INVALID_SYSTEM_KEY);
        return FALSE;
    }

    p = *path + lstrlenW(*path) - 1;
    if (*p == '\\') *p = 0;

    return TRUE;
}

BOOL is_char(const WCHAR s, const WCHAR c)
{
    return (s == c || s == towupper(c));
}

BOOL is_switch(const WCHAR *s, const WCHAR c)
{
    if (lstrlenW(s) > 2)
        return FALSE;

    return ((s[0] == '/' || s[0] == '-') && is_char(s[1], c));
}

static BOOL is_help_switch(const WCHAR *s)
{
    return (is_switch(s, '?') || is_switch(s, 'h'));
}

enum operations {
    REG_ADD,
    REG_COPY,
    REG_DELETE,
    REG_EXPORT,
    REG_IMPORT,
    REG_QUERY,
    REG_INVALID
};

static enum operations get_operation(const WCHAR *str, int *op_help)
{
    struct op_info { const WCHAR *op; int id; int help_id; };

    static const struct op_info op_array[] =
    {
        { L"add",     REG_ADD,     STRING_ADD_USAGE },
        { L"copy",    REG_COPY,    STRING_COPY_USAGE },
        { L"delete",  REG_DELETE,  STRING_DELETE_USAGE },
        { L"export",  REG_EXPORT,  STRING_EXPORT_USAGE },
        { L"import",  REG_IMPORT,  STRING_IMPORT_USAGE },
        { L"query",   REG_QUERY,   STRING_QUERY_USAGE },
        { NULL,    -1,          0 }
    };

    const struct op_info *ptr;

    for (ptr = op_array; ptr->op; ptr++)
    {
        if (!lstrcmpiW(str, ptr->op))
        {
            *op_help = ptr->help_id;
            return ptr->id;
        }
    }

    return REG_INVALID;
}

int __cdecl wmain(int argc, WCHAR *argvW[])
{
    int op, op_help;

    if (argc == 1)
    {
        output_message(STRING_INVALID_SYNTAX);
        output_message(STRING_REG_HELP);
        return 1;
    }

    if (is_help_switch(argvW[1]))
    {
        output_message(STRING_USAGE);
        return 0;
    }

    op = get_operation(argvW[1], &op_help);

    if (op == REG_INVALID)
    {
        output_message(STRING_INVALID_OPTION, argvW[1]);
        output_message(STRING_REG_HELP);
        return 1;
    }
    else if (argc == 2) /* Valid operation, no arguments supplied */
        goto invalid;

    if (is_help_switch(argvW[2]))
    {
        if (argc > 3) goto invalid;

        output_message(op_help);
        output_message(STRING_REG_VIEW_USAGE);
        return 0;
    }

    if (op == REG_ADD)
        return reg_add(argc, argvW);

    if (op == REG_COPY)
        return reg_copy(argc, argvW);

    if (op == REG_DELETE)
        return reg_delete(argc, argvW);

    if (op == REG_EXPORT)
        return reg_export(argc, argvW);

    if (op == REG_IMPORT)
        return reg_import(argc, argvW);

    return reg_query(argc, argvW);

invalid:
    output_message(STRING_INVALID_SYNTAX);
    output_message(STRING_FUNC_HELP, wcsupr(argvW[1]));
    return 1;
}
