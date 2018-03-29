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

#include <windows.h>
#include <errno.h>
#include <stdlib.h>
#include <wine/unicode.h>
#include <wine/debug.h>
#include <wine/heap.h>
#include "reg.h"

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

static const struct
{
    DWORD type;
    const WCHAR *name;
}
type_rels[] =
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

static const WCHAR newlineW[] = {'\n',0};

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

    SetLastError(NO_ERROR);
    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         fmt, 0, 0, (WCHAR *)&str, 0, &va_args);
    if (len == 0 && GetLastError() != NO_ERROR)
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

static void WINAPIV output_string(const WCHAR *fmt, ...)
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
        answer[0] = toupperW(answer[0]);
        if (answer[0] == Ybuffer[0])
            return TRUE;
        if (answer[0] == Nbuffer[0])
            return FALSE;
    }
}

static inline BOOL path_rootname_cmp(const WCHAR *input_path, const WCHAR *rootkey_name)
{
    DWORD length = strlenW(rootkey_name);

    return (!strncmpiW(input_path, rootkey_name, length) &&
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

static DWORD wchar_get_type(const WCHAR *type_name)
{
    DWORD i;

    if (!type_name)
        return REG_SZ;

    for (i = 0; i < ARRAY_SIZE(type_rels); i++)
    {
        if (!strcmpiW(type_rels[i].name, type_name))
            return type_rels[i].type;
    }

    return ~0u;
}

/* hexchar_to_byte from programs/regedit/hexedit.c */
static inline BYTE hexchar_to_byte(WCHAR ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    else if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    else
        return -1;
}

static LPBYTE get_regdata(const WCHAR *data, DWORD reg_type, WCHAR separator, DWORD *reg_count)
{
    static const WCHAR empty;
    LPBYTE out_data = NULL;
    *reg_count = 0;

    if (!data) data = &empty;

    switch (reg_type)
    {
        case REG_NONE:
        case REG_SZ:
        case REG_EXPAND_SZ:
        {
            *reg_count = (lstrlenW(data) + 1) * sizeof(WCHAR);
            out_data = heap_xalloc(*reg_count);
            lstrcpyW((LPWSTR)out_data,data);
            break;
        }
        case REG_DWORD:
     /* case REG_DWORD_LITTLE_ENDIAN: */
        case REG_DWORD_BIG_ENDIAN: /* Yes, this is correct! */
        {
            LPWSTR rest;
            unsigned long val;
            val = wcstoul(data, &rest, (tolowerW(data[1]) == 'x') ? 16 : 10);
            if (*rest || data[0] == '-' || (val == ~0u && errno == ERANGE)) {
                output_message(STRING_MISSING_INTEGER);
                break;
            }
            *reg_count = sizeof(DWORD);
            out_data = heap_xalloc(*reg_count);
            ((LPDWORD)out_data)[0] = val;
            break;
        }
        case REG_BINARY:
        {
            BYTE hex0, hex1;
            int i = 0, destByteIndex = 0, datalen = lstrlenW(data);
            *reg_count = ((datalen + datalen % 2) / 2) * sizeof(BYTE);
            out_data = heap_xalloc(*reg_count);
            if(datalen % 2)
            {
                hex1 = hexchar_to_byte(data[i++]);
                if(hex1 == 0xFF)
                    goto no_hex_data;
                out_data[destByteIndex++] = hex1;
            }
            for(;i + 1 < datalen;i += 2)
            {
                hex0 = hexchar_to_byte(data[i]);
                hex1 = hexchar_to_byte(data[i + 1]);
                if(hex0 == 0xFF || hex1 == 0xFF)
                    goto no_hex_data;
                out_data[destByteIndex++] = (hex0 << 4) | hex1;
            }
            break;
            no_hex_data:
            /* cleanup, print error */
            heap_free(out_data);
            output_message(STRING_MISSING_HEXDATA);
            out_data = NULL;
            break;
        }
        case REG_MULTI_SZ:
        {
            int i, destindex, len = strlenW(data);
            WCHAR *buffer = heap_xalloc((len + 2) * sizeof(WCHAR));

            for (i = 0, destindex = 0; i < len; i++, destindex++)
            {
                if (!separator && data[i] == '\\' && data[i + 1] == '0')
                {
                    buffer[destindex] = 0;
                    i++;
                }
                else if (data[i] == separator)
                    buffer[destindex] = 0;
                else
                    buffer[destindex] = data[i];

                if (destindex && !buffer[destindex - 1] && (!buffer[destindex] || destindex == 1))
                {
                    heap_free(buffer);
                    output_message(STRING_INVALID_STRING);
                    return NULL;
                }
            }
            buffer[destindex] = 0;
            if (destindex && buffer[destindex - 1])
                buffer[++destindex] = 0;
            *reg_count = (destindex + 1) * sizeof(WCHAR);
            return (BYTE *)buffer;
        }
        default:
            output_message(STRING_UNHANDLED_TYPE, reg_type, data);
    }

    return out_data;
}

static BOOL sane_path(const WCHAR *key)
{
    unsigned int i = strlenW(key);

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

static int reg_add(HKEY root, WCHAR *path, WCHAR *value_name, BOOL value_empty,
                   WCHAR *type, WCHAR separator, WCHAR *data, BOOL force)
{
    HKEY key;

    if (RegCreateKeyW(root, path, &key) != ERROR_SUCCESS)
    {
        output_message(STRING_INVALID_KEY);
        return 1;
    }

    if (value_name || value_empty || data)
    {
        DWORD reg_type;
        DWORD reg_count = 0;
        BYTE* reg_data = NULL;

        if (!force)
        {
            if (RegQueryValueExW(key, value_name, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                if (!ask_confirm(STRING_OVERWRITE_VALUE, value_name))
                {
                    RegCloseKey(key);
                    output_message(STRING_CANCELLED);
                    return 0;
                }
            }
        }

        reg_type = wchar_get_type(type);
        if (reg_type == ~0u)
        {
            RegCloseKey(key);
            output_message(STRING_UNSUPPORTED_TYPE, type);
            return 1;
        }
        if ((reg_type == REG_DWORD || reg_type == REG_DWORD_BIG_ENDIAN) && !data)
        {
             RegCloseKey(key);
             output_message(STRING_INVALID_CMDLINE);
             return 1;
        }

        if (!(reg_data = get_regdata(data, reg_type, separator, &reg_count)))
        {
            RegCloseKey(key);
            return 1;
        }

        RegSetValueExW(key, value_name, 0, reg_type, reg_data, reg_count);
        heap_free(reg_data);
    }

    RegCloseKey(key);
    output_message(STRING_SUCCESS);

    return 0;
}

static int reg_delete(HKEY root, WCHAR *path, WCHAR *key_name, WCHAR *value_name,
                      BOOL value_empty, BOOL value_all, BOOL force)
{
    HKEY key;

    if (!force)
    {
        BOOL ret;

        if (value_name || value_empty)
            ret = ask_confirm(STRING_DELETE_VALUE, value_name);
        else if (value_all)
            ret = ask_confirm(STRING_DELETE_VALUEALL, key_name);
        else
            ret = ask_confirm(STRING_DELETE_SUBKEY, key_name);

        if (!ret)
        {
            output_message(STRING_CANCELLED);
            return 0;
        }
    }

    /* Delete subtree only if no /v* option is given */
    if (!value_name && !value_empty && !value_all)
    {
        if (RegDeleteTreeW(root, path) != ERROR_SUCCESS)
        {
            output_message(STRING_CANNOT_FIND);
            return 1;
        }
        output_message(STRING_SUCCESS);
        return 0;
    }

    if (RegOpenKeyW(root, path, &key) != ERROR_SUCCESS)
    {
        output_message(STRING_CANNOT_FIND);
        return 1;
    }

    if (value_all)
    {
        DWORD max_value_len = 256, value_len;
        WCHAR *value_name;
        LONG rc;

        value_name = heap_xalloc(max_value_len * sizeof(WCHAR));

        while (1)
        {
            value_len = max_value_len;
            rc = RegEnumValueW(key, 0, value_name, &value_len, NULL, NULL, NULL, NULL);
            if (rc == ERROR_SUCCESS)
            {
                rc = RegDeleteValueW(key, value_name);
                if (rc != ERROR_SUCCESS)
                {
                    heap_free(value_name);
                    RegCloseKey(key);
                    output_message(STRING_VALUEALL_FAILED, key_name);
                    return 1;
                }
            }
            else if (rc == ERROR_MORE_DATA)
            {
                max_value_len *= 2;
                value_name = heap_xrealloc(value_name, max_value_len * sizeof(WCHAR));
            }
            else break;
        }
        heap_free(value_name);
    }
    else if (value_name || value_empty)
    {
        if (RegDeleteValueW(key, value_empty ? NULL : value_name) != ERROR_SUCCESS)
        {
            RegCloseKey(key);
            output_message(STRING_CANNOT_FIND);
            return 1;
        }
    }

    RegCloseKey(key);
    output_message(STRING_SUCCESS);
    return 0;
}

static WCHAR *reg_data_to_wchar(DWORD type, const BYTE *src, DWORD size_bytes)
{
    WCHAR *buffer = NULL;
    int i;

    switch (type)
    {
        case REG_SZ:
        case REG_EXPAND_SZ:
            buffer = heap_xalloc(size_bytes);
            strcpyW(buffer, (WCHAR *)src);
            break;
        case REG_NONE:
        case REG_BINARY:
        {
            WCHAR *ptr;
            static const WCHAR fmt[] = {'%','0','2','X',0};

            buffer = heap_xalloc((size_bytes * 2 + 1) * sizeof(WCHAR));
            ptr = buffer;
            for (i = 0; i < size_bytes; i++)
                ptr += sprintfW(ptr, fmt, src[i]);
            break;
        }
        case REG_DWORD:
     /* case REG_DWORD_LITTLE_ENDIAN: */
        case REG_DWORD_BIG_ENDIAN:
        {
            const int zero_x_dword = 10;
            static const WCHAR fmt[] = {'0','x','%','x',0};

            buffer = heap_xalloc((zero_x_dword + 1) * sizeof(WCHAR));
            sprintfW(buffer, fmt, *(DWORD *)src);
            break;
        }
        case REG_MULTI_SZ:
        {
            const int two_wchars = 2 * sizeof(WCHAR);
            DWORD tmp_size;
            const WCHAR *tmp = (const WCHAR *)src;
            int len, destindex;

            if (size_bytes <= two_wchars)
            {
                buffer = heap_xalloc(sizeof(WCHAR));
                *buffer = 0;
                return buffer;
            }

            tmp_size = size_bytes - two_wchars; /* exclude both null terminators */
            buffer = heap_xalloc(tmp_size * 2 + sizeof(WCHAR));
            len = tmp_size / sizeof(WCHAR);

            for (i = 0, destindex = 0; i < len; i++, destindex++)
            {
                if (tmp[i])
                    buffer[destindex] = tmp[i];
                else
                {
                    buffer[destindex++] = '\\';
                    buffer[destindex] = '0';
                }
            }
            buffer[destindex] = 0;
            break;
        }
    }
    return buffer;
}

static const WCHAR *reg_type_to_wchar(DWORD type)
{
    int i, array_size = ARRAY_SIZE(type_rels);

    for (i = 0; i < array_size; i++)
    {
        if (type == type_rels[i].type)
            return type_rels[i].name;
    }
    return NULL;
}

static void output_value(const WCHAR *value_name, DWORD type, BYTE *data, DWORD data_size)
{
    static const WCHAR fmt[] = {' ',' ',' ',' ','%','1',0};
    WCHAR defval[32];
    WCHAR *reg_data;

    if (value_name && value_name[0])
        output_string(fmt, value_name);
    else
    {
        LoadStringW(GetModuleHandleW(NULL), STRING_DEFAULT_VALUE, defval, ARRAY_SIZE(defval));
        output_string(fmt, defval);
    }
    output_string(fmt, reg_type_to_wchar(type));

    if (data)
    {
        reg_data = reg_data_to_wchar(type, data, data_size);
        output_string(fmt, reg_data);
        heap_free(reg_data);
    }
    else
    {
        LoadStringW(GetModuleHandleW(NULL), STRING_VALUE_NOT_SET, defval, ARRAY_SIZE(defval));
        output_string(fmt, defval);
    }
    output_string(newlineW);
}

WCHAR *build_subkey_path(WCHAR *path, DWORD path_len, WCHAR *subkey_name, DWORD subkey_len)
{
    WCHAR *subkey_path;
    static const WCHAR fmt[] = {'%','s','\\','%','s',0};

    subkey_path = heap_xalloc((path_len + subkey_len + 2) * sizeof(WCHAR));
    sprintfW(subkey_path, fmt, path, subkey_name);

    return subkey_path;
}

static unsigned int num_values_found = 0;

static int query_value(HKEY key, WCHAR *value_name, WCHAR *path, BOOL recurse)
{
    LONG rc;
    DWORD max_data_bytes = 2048, data_size;
    DWORD subkey_len;
    DWORD type, path_len, i;
    BYTE *data;
    WCHAR fmt[] = {'%','1','\n',0};
    WCHAR *subkey_name, *subkey_path;
    HKEY subkey;

    data = heap_xalloc(max_data_bytes);

    for (;;)
    {
        data_size = max_data_bytes;
        rc = RegQueryValueExW(key, value_name, NULL, &type, data, &data_size);
        if (rc == ERROR_MORE_DATA)
        {
            max_data_bytes = data_size;
            data = heap_xrealloc(data, max_data_bytes);
        }
        else break;
    }

    if (rc == ERROR_SUCCESS)
    {
        output_string(fmt, path);
        output_value(value_name, type, data, data_size);
        output_string(newlineW);
        num_values_found++;
    }

    heap_free(data);

    if (!recurse)
    {
        if (rc == ERROR_FILE_NOT_FOUND)
        {
            if (value_name && *value_name)
            {
                output_message(STRING_CANNOT_FIND);
                return 1;
            }
            output_string(fmt, path);
            output_value(NULL, REG_SZ, NULL, 0);
        }
        return 0;
    }

    subkey_name = heap_xalloc(MAX_SUBKEY_LEN * sizeof(WCHAR));

    path_len = strlenW(path);

    i = 0;
    for (;;)
    {
        subkey_len = MAX_SUBKEY_LEN;
        rc = RegEnumKeyExW(key, i, subkey_name, &subkey_len, NULL, NULL, NULL, NULL);
        if (rc == ERROR_SUCCESS)
        {
            subkey_path = build_subkey_path(path, path_len, subkey_name, subkey_len);
            if (!RegOpenKeyExW(key, subkey_name, 0, KEY_READ, &subkey))
            {
                query_value(subkey, value_name, subkey_path, recurse);
                RegCloseKey(subkey);
            }
            heap_free(subkey_path);
            i++;
        }
        else break;
    }

    heap_free(subkey_name);
    return 0;
}

static int query_all(HKEY key, WCHAR *path, BOOL recurse)
{
    LONG rc;
    DWORD max_value_len = 256, value_len;
    DWORD max_data_bytes = 2048, data_size;
    DWORD subkey_len;
    DWORD i, type, path_len;
    WCHAR fmt[] = {'%','1','\n',0};
    WCHAR fmt_path[] = {'%','1','\\','%','2','\n',0};
    WCHAR *value_name, *subkey_name, *subkey_path;
    BYTE *data;
    HKEY subkey;

    output_string(fmt, path);

    value_name = heap_xalloc(max_value_len * sizeof(WCHAR));
    data = heap_xalloc(max_data_bytes);

    i = 0;
    for (;;)
    {
        value_len = max_value_len;
        data_size = max_data_bytes;
        rc = RegEnumValueW(key, i, value_name, &value_len, NULL, &type, data, &data_size);
        if (rc == ERROR_SUCCESS)
        {
            output_value(value_name, type, data, data_size);
            i++;
        }
        else if (rc == ERROR_MORE_DATA)
        {
            if (data_size > max_data_bytes)
            {
                max_data_bytes = data_size;
                data = heap_xrealloc(data, max_data_bytes);
            }
            else
            {
                max_value_len *= 2;
                value_name = heap_xrealloc(value_name, max_value_len * sizeof(WCHAR));
            }
        }
        else break;
    }

    heap_free(data);
    heap_free(value_name);

    if (i || recurse)
        output_string(newlineW);

    subkey_name = heap_xalloc(MAX_SUBKEY_LEN * sizeof(WCHAR));

    path_len = strlenW(path);

    i = 0;
    for (;;)
    {
        subkey_len = MAX_SUBKEY_LEN;
        rc = RegEnumKeyExW(key, i, subkey_name, &subkey_len, NULL, NULL, NULL, NULL);
        if (rc == ERROR_SUCCESS)
        {
            if (recurse)
            {
                subkey_path = build_subkey_path(path, path_len, subkey_name, subkey_len);
                if (!RegOpenKeyExW(key, subkey_name, 0, KEY_READ, &subkey))
                {
                    query_all(subkey, subkey_path, recurse);
                    RegCloseKey(subkey);
                }
                heap_free(subkey_path);
            }
            else output_string(fmt_path, path, subkey_name);
            i++;
        }
        else break;
    }

    heap_free(subkey_name);

    if (i && !recurse)
        output_string(newlineW);

    return 0;
}

static int reg_query(HKEY root, WCHAR *path, WCHAR *key_name, WCHAR *value_name,
                     BOOL value_empty, BOOL recurse)
{
    HKEY key;
    int ret;

    if (RegOpenKeyExW(root, path, 0, KEY_READ, &key) != ERROR_SUCCESS)
    {
        output_message(STRING_CANNOT_FIND);
        return 1;
    }

    output_string(newlineW);

    if (value_name || value_empty)
    {
        ret = query_value(key, value_name, key_name, recurse);
        if (recurse)
            output_message(STRING_MATCHES_FOUND, num_values_found);
    }
    else
        ret = query_all(key, key_name, recurse);

    RegCloseKey(key);

    return ret;
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

    len = strlenW(root_rels[i].long_name);

    if (!path)
    {
        long_key = heap_xalloc((len + 1) * sizeof(WCHAR));
        strcpyW(long_key, root_rels[i].long_name);
        return long_key;
    }

    len += strlenW(path) + 1; /* add one for the backslash */
    long_key = heap_xalloc((len + 1) * sizeof(WCHAR));
    sprintfW(long_key, fmt, root_rels[i].long_name, path);
    return long_key;
}

BOOL parse_registry_key(const WCHAR *key, HKEY *root, WCHAR **path, WCHAR **long_key)
{
    if (!sane_path(key))
        return FALSE;

    *path = strchrW(key, '\\');
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

static BOOL is_switch(const WCHAR *s, const WCHAR c)
{
    if (strlenW(s) > 2)
        return FALSE;

    if ((s[0] == '/' || s[0] == '-') && (s[1] == c || s[1] == toupperW(c)))
        return TRUE;

    return FALSE;
}

static BOOL is_help_switch(const WCHAR *s)
{
    if (is_switch(s, '?') || is_switch(s, 'h'))
        return TRUE;

    return FALSE;
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

int wmain(int argc, WCHAR *argvW[])
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
        output_message(STRING_FUNC_HELP, struprW(argvW[1]));
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

            switch(tolowerW(argvW[i][1]))
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
                if (!ptr || strlenW(ptr) != 1)
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
