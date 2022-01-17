/*
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

#include <stdio.h>
#include "reg.h"

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

static WCHAR *reg_data_to_wchar(DWORD type, const BYTE *src, DWORD size_bytes)
{
    WCHAR *buffer = NULL;
    int i;

    switch (type)
    {
        case REG_SZ:
        case REG_EXPAND_SZ:
            buffer = malloc(size_bytes);
            lstrcpyW(buffer, (WCHAR *)src);
            break;
        case REG_NONE:
        case REG_BINARY:
        {
            WCHAR *ptr;

            buffer = malloc((size_bytes * 2 + 1) * sizeof(WCHAR));

            if (!size_bytes)
            {
                *buffer = 0;
                break;
            }

            ptr = buffer;

            for (i = 0; i < size_bytes; i++)
                ptr += swprintf(ptr, 3, L"%02X", src[i]);
            break;
        }
        case REG_DWORD:
     /* case REG_DWORD_LITTLE_ENDIAN: */
        case REG_DWORD_BIG_ENDIAN:
        {
            const int zero_x_dword = 10;

            buffer = malloc((zero_x_dword + 1) * sizeof(WCHAR));
            swprintf(buffer, zero_x_dword + 1, L"0x%x", *(DWORD *)src);
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
                buffer = malloc(sizeof(WCHAR));
                *buffer = 0;
                return buffer;
            }

            tmp_size = size_bytes - two_wchars; /* exclude both null terminators */
            buffer = malloc(tmp_size * 2 + sizeof(WCHAR));
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

static const WCHAR *newlineW = L"\n";

static void output_value(const WCHAR *value_name, DWORD type, BYTE *data, DWORD data_size)
{
    static const WCHAR *fmt = L"    %1";
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
        free(reg_data);
    }
    else
    {
        LoadStringW(GetModuleHandleW(NULL), STRING_VALUE_NOT_SET, defval, ARRAY_SIZE(defval));
        output_string(fmt, defval);
    }
    output_string(newlineW);
}

static unsigned int num_values_found = 0;
static REGSAM sam = 0;

static int query_value(HKEY hkey, WCHAR *value_name, WCHAR *path, BOOL recurse)
{
    LONG rc;
    DWORD max_data_bytes = 2048, data_size;
    DWORD subkey_len;
    DWORD type, path_len, i;
    BYTE *data;
    static const WCHAR *fmt = L"%1\n";
    WCHAR *subkey_name, *subkey_path;
    HKEY subkey;

    data = malloc(max_data_bytes);

    for (;;)
    {
        data_size = max_data_bytes;
        rc = RegQueryValueExW(hkey, value_name, NULL, &type, data, &data_size);
        if (rc == ERROR_MORE_DATA)
        {
            max_data_bytes = data_size;
            data = realloc(data, max_data_bytes);
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

    free(data);

    if (!recurse)
    {
        if (rc == ERROR_FILE_NOT_FOUND)
        {
            if (value_name && *value_name)
            {
                output_message(STRING_VALUE_NONEXIST);
                return 1;
            }
            output_string(fmt, path);
            output_value(NULL, REG_SZ, NULL, 0);
        }
        return 0;
    }

    subkey_name = malloc(MAX_SUBKEY_LEN * sizeof(WCHAR));

    path_len = lstrlenW(path);

    i = 0;
    for (;;)
    {
        subkey_len = MAX_SUBKEY_LEN;
        rc = RegEnumKeyExW(hkey, i, subkey_name, &subkey_len, NULL, NULL, NULL, NULL);
        if (rc == ERROR_SUCCESS)
        {
            subkey_path = build_subkey_path(path, path_len, subkey_name, subkey_len);
            if (!RegOpenKeyExW(hkey, subkey_name, 0, KEY_READ|sam, &subkey))
            {
                query_value(subkey, value_name, subkey_path, recurse);
                RegCloseKey(subkey);
            }
            free(subkey_path);
            i++;
        }
        else break;
    }

    free(subkey_name);
    return 0;
}

static int query_all(HKEY hkey, WCHAR *path, BOOL recurse, BOOL recursing)
{
    LONG rc;
    DWORD num_subkeys, num_values;
    DWORD max_value_len = 256, value_len;
    DWORD max_data_bytes = 2048, data_size;
    DWORD subkey_len;
    DWORD i, type, path_len;
    WCHAR *value_name, *subkey_name, *subkey_path;
    BYTE *data;
    HKEY subkey;

    rc = RegQueryInfoKeyW(hkey, NULL, NULL, NULL, &num_subkeys, NULL,
                          NULL, &num_values, NULL, NULL, NULL, NULL);
    if (rc) return 1;

    if (num_values || recursing)
        output_string(L"%1\n", path);

    value_name = malloc(max_value_len * sizeof(WCHAR));
    data = malloc(max_data_bytes);

    i = 0;
    for (;;)
    {
        value_len = max_value_len;
        data_size = max_data_bytes;
        rc = RegEnumValueW(hkey, i, value_name, &value_len, NULL, &type, data, &data_size);
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
                data = realloc(data, max_data_bytes);
            }
            else
            {
                max_value_len *= 2;
                value_name = realloc(value_name, max_value_len * sizeof(WCHAR));
            }
        }
        else break;
    }

    free(data);
    free(value_name);

    if (i || recursing)
        output_string(newlineW);

    if (!num_subkeys)
        return 0;

    subkey_name = malloc(MAX_SUBKEY_LEN * sizeof(WCHAR));

    path_len = lstrlenW(path);

    i = 0;
    for (;;)
    {
        subkey_len = MAX_SUBKEY_LEN;
        rc = RegEnumKeyExW(hkey, i, subkey_name, &subkey_len, NULL, NULL, NULL, NULL);
        if (rc == ERROR_SUCCESS)
        {
            if (recurse)
            {
                subkey_path = build_subkey_path(path, path_len, subkey_name, subkey_len);
                if (!RegOpenKeyExW(hkey, subkey_name, 0, KEY_READ|sam, &subkey))
                {
                    query_all(subkey, subkey_path, recurse, TRUE);
                    RegCloseKey(subkey);
                }
                free(subkey_path);
            }
            else output_string(L"%1\\%2\n", path, subkey_name);
            i++;
        }
        else break;
    }

    free(subkey_name);
    return 0;
}

static int run_query(HKEY root, WCHAR *path, WCHAR *key_name, WCHAR *value_name,
                     BOOL value_empty, BOOL recurse)
{
    HKEY hkey;
    int ret;

    if (RegOpenKeyExW(root, path, 0, KEY_READ|sam, &hkey))
    {
        output_message(STRING_KEY_NONEXIST);
        return 1;
    }

    output_string(newlineW);

    if (value_name || value_empty)
    {
        ret = query_value(hkey, value_name, key_name, recurse);
        if (recurse)
            output_message(STRING_MATCHES_FOUND, num_values_found);
    }
    else
        ret = query_all(hkey, key_name, recurse, FALSE);

    RegCloseKey(hkey);

    return ret;
}

int reg_query(int argc, WCHAR *argvW[])
{
    HKEY root;
    WCHAR *path, *key_name, *value_name = NULL;
    BOOL value_empty = FALSE, recurse = FALSE;
    int i;

    if (!parse_registry_key(argvW[2], &root, &path))
        return 1;

    for (i = 3; i < argc; i++)
    {
        WCHAR *str;

        if (argvW[i][0] != '/' && argvW[i][0] != '-')
            goto invalid;

        str = &argvW[i][1];

        if (!lstrcmpiW(str, L"ve"))
        {
            if (value_empty) goto invalid;
            value_empty = TRUE;
            continue;
        }
        else if (!lstrcmpiW(str, L"reg:32"))
        {
            if (sam & KEY_WOW64_32KEY) goto invalid;
            sam |= KEY_WOW64_32KEY;
            continue;
        }
        else if (!lstrcmpiW(str, L"reg:64"))
        {
            if (sam & KEY_WOW64_64KEY) goto invalid;
            sam |= KEY_WOW64_64KEY;
            continue;
        }
        else if (!str[0] || str[1])
            goto invalid;

        switch (towlower(*str))
        {
        case 'v':
            if (value_name || !(value_name = argvW[++i]))
                goto invalid;
            break;
        case 's':
            if (recurse) goto invalid;
            recurse = TRUE;
            break;
        default:
            goto invalid;
        }
    }

    if (value_name && value_empty)
        goto invalid;

    if (sam == (KEY_WOW64_32KEY|KEY_WOW64_64KEY))
        goto invalid;

    key_name = get_long_key(root, path);

    return run_query(root, path, key_name, value_name, value_empty, recurse);

invalid:
    output_message(STRING_INVALID_SYNTAX);
    output_message(STRING_FUNC_HELP, wcsupr(argvW[1]));
    return 1;
}
