/*
 * Copyright 2008 Andrew Riedi
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

#include <stdlib.h>
#include "reg.h"
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(reg);

static const WCHAR short_hklm[] = {'H','K','L','M',0};
static const WCHAR short_hkcu[] = {'H','K','C','U',0};
static const WCHAR short_hkcr[] = {'H','K','C','R',0};
static const WCHAR short_hku[] = {'H','K','U',0};
static const WCHAR short_hkcc[] = {'H','K','C','C',0};
static const WCHAR long_hklm[] = {'H','K','E','Y','_','L','O','C','A','L','_','M','A','C','H','I','N','E',0};
static const WCHAR long_hkcu[] = {'H','K','E','Y','_','C','U','R','R','E','N','T','_','U','S','E','R',0};
static const WCHAR long_hkcr[] = {'H','K','E','Y','_','C','L','A','S','S','E','S','_','R','O','O','T',0};
static const WCHAR long_hku[] = {'H','K','E','Y','_','U','S','E','R','S',0};
static const WCHAR long_hkcc[] = {'H','K','E','Y','_','C','U','R','R','E','N','T','_','C','O','N','F','I','G',0};

static const struct
{
    HKEY key;
    const WCHAR *short_name;
    const WCHAR *long_name;
}
root_rels[] =
{
    {HKEY_LOCAL_MACHINE, short_hklm, long_hklm},
    {HKEY_CURRENT_USER, short_hkcu, long_hkcu},
    {HKEY_CLASSES_ROOT, short_hkcr, long_hkcr},
    {HKEY_USERS, short_hku, long_hku},
    {HKEY_CURRENT_CONFIG, short_hkcc, long_hkcc},
};

static const WCHAR type_none[] = {'R','E','G','_','N','O','N','E',0};
static const WCHAR type_sz[] = {'R','E','G','_','S','Z',0};
static const WCHAR type_expand_sz[] = {'R','E','G','_','E','X','P','A','N','D','_','S','Z',0};
static const WCHAR type_binary[] = {'R','E','G','_','B','I','N','A','R','Y',0};
static const WCHAR type_dword[] = {'R','E','G','_','D','W','O','R','D',0};
static const WCHAR type_dword_le[] = {'R','E','G','_','D','W','O','R','D','_','L','I','T','T','L','E','_','E','N','D','I','A','N',0};
static const WCHAR type_dword_be[] = {'R','E','G','_','D','W','O','R','D','_','B','I','G','_','E','N','D','I','A','N',0};
static const WCHAR type_multi_sz[] = {'R','E','G','_','M','U','L','T','I','_','S','Z',0};

const struct reg_type_rels type_rels[] =
{
    {REG_NONE, type_none},
    {REG_SZ, type_sz},
    {REG_EXPAND_SZ, type_expand_sz},
    {REG_BINARY, type_binary},
    {REG_DWORD, type_dword},
    {REG_DWORD_LITTLE_ENDIAN, type_dword_le},
    {REG_DWORD_BIG_ENDIAN, type_dword_be},
    {REG_MULTI_SZ, type_multi_sz},
};

void *heap_xalloc(size_t size)
{
    void *buf = heap_alloc(size);
    if (!buf)
    {
        ERR("Out of memory!\n");
        exit(1);
    }
    return buf;
}

void *heap_xrealloc(void *buf, size_t size)
{
    void *new_buf = heap_realloc(buf, size);

    if (!new_buf)
    {
        ERR("Out of memory!\n");
        exit(1);
    }

    return new_buf;
}

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
        msgA = heap_xalloc(len);

        WideCharToMultiByte(GetConsoleOutputCP(), 0, str, wlen, msgA, len, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        heap_free(msgA);
    }
}

static void output_formatstring(const WCHAR *fmt, __ms_va_list va_args)
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
    WCHAR fmt[1024];
    __ms_va_list va_args;

    if (!LoadStringW(GetModuleHandleW(NULL), id, fmt, ARRAY_SIZE(fmt)))
    {
        WINE_FIXME("LoadString failed with %d\n", GetLastError());
        return;
    }
    __ms_va_start(va_args, id);
    output_formatstring(fmt, va_args);
    __ms_va_end(va_args);
}

void WINAPIV output_string(const WCHAR *fmt, ...)
{
    __ms_va_list va_args;

    __ms_va_start(va_args, fmt);
    output_formatstring(fmt, va_args);
    __ms_va_end(va_args);
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
    static const WCHAR fmt[] = {'%','s','\\','%','s',0};

    subkey_path = heap_xalloc((path_len + subkey_len + 2) * sizeof(WCHAR));
    swprintf(subkey_path, fmt, path, subkey_name);

    return subkey_path;
}

static WCHAR *get_long_key(HKEY root, WCHAR *path)
{
    DWORD i, array_size = ARRAY_SIZE(root_rels), len;
    WCHAR *long_key;
    WCHAR fmt[] = {'%','s','\\','%','s',0};

    for (i = 0; i < array_size; i++)
    {
        if (root == root_rels[i].key)
            break;
    }

    len = lstrlenW(root_rels[i].long_name);

    if (!path)
    {
        long_key = heap_xalloc((len + 1) * sizeof(WCHAR));
        lstrcpyW(long_key, root_rels[i].long_name);
        return long_key;
    }

    len += lstrlenW(path) + 1; /* add one for the backslash */
    long_key = heap_xalloc((len + 1) * sizeof(WCHAR));
    swprintf(long_key, fmt, root_rels[i].long_name, path);
    return long_key;
}

BOOL parse_registry_key(const WCHAR *key, HKEY *root, WCHAR **path, WCHAR **long_key)
{
    if (!sane_path(key))
        return FALSE;

    *path = wcschr(key, '\\');
    if (*path) (*path)++;

    *root = path_get_rootkey(key);
    if (!*root)
    {
        if (*path) *(*path - 1) = 0;
        output_message(STRING_INVALID_SYSTEM_KEY, key);
        return FALSE;
    }

    *long_key = get_long_key(*root, *path);

    return TRUE;
}

BOOL is_switch(const WCHAR *s, const WCHAR c)
{
    if (lstrlenW(s) > 2)
        return FALSE;

    if ((s[0] == '/' || s[0] == '-') && (s[1] == c || s[1] == towupper(c)))
        return TRUE;

    return FALSE;
}

static BOOL is_help_switch(const WCHAR *s)
{
    return (is_switch(s, '?') || is_switch(s, 'h'));
}

enum operations {
    REG_ADD,
    REG_DELETE,
    REG_IMPORT,
    REG_EXPORT,
    REG_QUERY,
    REG_INVALID
};

static enum operations get_operation(const WCHAR *str, int *op_help)
{
    struct op_info { const WCHAR *op; int id; int help_id; };

    static const WCHAR add[] = {'a','d','d',0};
    static const WCHAR delete[] = {'d','e','l','e','t','e',0};
    static const WCHAR import[] = {'i','m','p','o','r','t',0};
    static const WCHAR export[] = {'e','x','p','o','r','t',0};
    static const WCHAR query[] = {'q','u','e','r','y',0};

    static const struct op_info op_array[] =
    {
        { add,     REG_ADD,     STRING_ADD_USAGE },
        { delete,  REG_DELETE,  STRING_DELETE_USAGE },
        { import,  REG_IMPORT,  STRING_IMPORT_USAGE },
        { export,  REG_EXPORT,  STRING_EXPORT_USAGE },
        { query,   REG_QUERY,   STRING_QUERY_USAGE },
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
    int i, op, op_help, ret;
    BOOL show_op_help = FALSE;
    static const WCHAR switchVAW[] = {'v','a',0};
    static const WCHAR switchVEW[] = {'v','e',0};
    WCHAR *key_name, *path, *value_name = NULL, *type = NULL, *data = NULL, separator = '\0';
    BOOL value_empty = FALSE, value_all = FALSE, recurse = FALSE, force = FALSE;
    HKEY root;

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

    if (argc > 2)
        show_op_help = is_help_switch(argvW[2]);

    if (argc == 2 || ((show_op_help || op == REG_IMPORT) && argc > 3))
    {
        output_message(STRING_INVALID_SYNTAX);
        output_message(STRING_FUNC_HELP, _wcsupr(argvW[1]));
        return 1;
    }
    else if (show_op_help)
    {
        output_message(op_help);
        return 0;
    }

    if (op == REG_IMPORT)
        return reg_import(argvW[2]);

    if (op == REG_EXPORT)
        return reg_export(argc, argvW);

    if (!parse_registry_key(argvW[2], &root, &path, &key_name))
        return 1;

    for (i = 3; i < argc; i++)
    {
        if (argvW[i][0] == '/' || argvW[i][0] == '-')
        {
            WCHAR *ptr = &argvW[i][1];

            if (!lstrcmpiW(ptr, switchVEW))
            {
                value_empty = TRUE;
                continue;
            }
            else if (!lstrcmpiW(ptr, switchVAW))
            {
                value_all = TRUE;
                continue;
            }
            else if (!ptr[0] || ptr[1])
            {
                output_message(STRING_INVALID_CMDLINE);
                return 1;
            }

            switch(towlower(argvW[i][1]))
            {
            case 'v':
                if (value_name || !(value_name = argvW[++i]))
                {
                    output_message(STRING_INVALID_CMDLINE);
                    return 1;
                }
                break;
            case 't':
                if (type || !(type = argvW[++i]))
                {
                    output_message(STRING_INVALID_CMDLINE);
                    return 1;
                }
                break;
            case 'd':
                if (data || !(data = argvW[++i]))
                {
                    output_message(STRING_INVALID_CMDLINE);
                    return 1;
                }
                break;
            case 's':
                if (op == REG_QUERY)
                {
                    recurse = TRUE;
                    break;
                }

                ptr = argvW[++i];
                if (!ptr || lstrlenW(ptr) != 1)
                {
                    output_message(STRING_INVALID_CMDLINE);
                    return 1;
                }
                separator = ptr[0];
                break;
            case 'f':
                force = TRUE;
                break;
            default:
                output_message(STRING_INVALID_CMDLINE);
                return 1;
            }
        }
    }

    if ((value_name && value_empty) || (value_name && value_all) || (value_empty && value_all))
    {
        output_message(STRING_INVALID_CMDLINE);
        return 1;
    }

    if (op == REG_ADD)
        ret = reg_add(root, path, value_name, value_empty, type, separator, data, force);
    else if (op == REG_DELETE)
        ret = reg_delete(root, path, key_name, value_name, value_empty, value_all, force);
    else
        ret = reg_query(root, path, key_name, value_name, value_empty, recurse);
    return ret;
}
