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
#include <windows.h>
#include <wine/heap.h>
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
            buffer = heap_xalloc(size_bytes);
            lstrcpyW(buffer, (WCHAR *)src);
            break;
        case REG_NONE:
        case REG_BINARY:
        {
            WCHAR *ptr;
            static const WCHAR fmt[] = {'%','0','2','X',0};

            buffer = heap_xalloc((size_bytes * 2 + 1) * sizeof(WCHAR));
            ptr = buffer;
            for (i = 0; i < size_bytes; i++)
                ptr += swprintf(ptr, 3, fmt, src[i]);
            break;
        }
        case REG_DWORD:
     /* case REG_DWORD_LITTLE_ENDIAN: */
        case REG_DWORD_BIG_ENDIAN:
        {
            const int zero_x_dword = 10;
            static const WCHAR fmt[] = {'0','x','%','x',0};

            buffer = heap_xalloc((zero_x_dword + 1) * sizeof(WCHAR));
            swprintf(buffer, zero_x_dword + 1, fmt, *(DWORD *)src);
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

static const WCHAR newlineW[] = {'\n',0};

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

    path_len = lstrlenW(path);

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

    path_len = lstrlenW(path);

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

int reg_query(HKEY root, WCHAR *path, WCHAR *key_name, WCHAR *value_name,
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
