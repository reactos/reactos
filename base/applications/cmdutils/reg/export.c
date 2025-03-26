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

#include <stdio.h>
#include "reg.h"

static void write_file(HANDLE hFile, const WCHAR *str)
{
    DWORD written;

    WriteFile(hFile, str, lstrlenW(str) * sizeof(WCHAR), &written, NULL);
}

static WCHAR *escape_string(WCHAR *str, size_t str_len, size_t *line_len)
{
    size_t i, escape_count, pos;
    WCHAR *buf;

    for (i = 0, escape_count = 0; i < str_len; i++)
    {
        WCHAR c = str[i];

        if (!c) break;

        if (c == '\r' || c == '\n' || c == '\\' || c == '"')
            escape_count++;
    }

    buf = malloc((str_len + escape_count + 1) * sizeof(WCHAR));

    for (i = 0, pos = 0; i < str_len; i++, pos++)
    {
        WCHAR c = str[i];

        if (!c) break;

        switch (c)
        {
        case '\r':
            buf[pos++] = '\\';
            buf[pos] = 'r';
            break;
        case '\n':
            buf[pos++] = '\\';
            buf[pos] = 'n';
            break;
        case '\\':
            buf[pos++] = '\\';
            buf[pos] = '\\';
            break;
        case '"':
            buf[pos++] = '\\';
            buf[pos] = '"';
            break;
        default:
            buf[pos] = c;
        }
    }

    buf[pos] = 0;
    *line_len = pos;
    return buf;
}

static size_t export_value_name(HANDLE hFile, WCHAR *name, size_t len)
{
    static const WCHAR *default_name = L"@=";
    size_t line_len;

    if (name && *name)
    {
        WCHAR *str = escape_string(name, len, &line_len);
        WCHAR *buf = malloc((line_len + 4) * sizeof(WCHAR));
        line_len = swprintf(buf, L"\"%s\"=", str);
        write_file(hFile, buf);
        free(buf);
        free(str);
    }
    else
    {
        line_len = lstrlenW(default_name);
        write_file(hFile, default_name);
    }

    return line_len;
}

static void export_string_data(WCHAR **buf, WCHAR *data, size_t size)
{
    size_t len = 0, line_len;
    WCHAR *str;

    if (size)
        len = size / sizeof(WCHAR) - 1;
    str = escape_string(data, len, &line_len);
    *buf = malloc((line_len + 3) * sizeof(WCHAR));
    swprintf(*buf, L"\"%s\"", str);
    free(str);
}

static void export_dword_data(WCHAR **buf, DWORD *data)
{
    *buf = malloc(15 * sizeof(WCHAR));
    swprintf(*buf, L"dword:%08x", *data);
}

static size_t export_hex_data_type(HANDLE hFile, DWORD type)
{
    static const WCHAR *hex = L"hex:";
    size_t line_len;

    if (type == REG_BINARY)
    {
        line_len = lstrlenW(hex);
        write_file(hFile, hex);
    }
    else
    {
        WCHAR *buf = malloc(15 * sizeof(WCHAR));
        line_len = swprintf(buf, L"hex(%x):", type);
        write_file(hFile, buf);
        free(buf);
    }

    return line_len;
}

#define MAX_HEX_CHARS 77

static void export_hex_data(HANDLE hFile, WCHAR **buf, DWORD type,
                            DWORD line_len, void *data, DWORD size)
{
    size_t num_commas, i, pos;

    line_len += export_hex_data_type(hFile, type);

    if (!size) return;

    num_commas = size - 1;
    *buf = malloc(size * 3 * sizeof(WCHAR));

    for (i = 0, pos = 0; i < size; i++)
    {
        pos += swprintf(*buf + pos, L"%02x", ((BYTE *)data)[i]);
        if (i == num_commas) break;
        (*buf)[pos++] = ',';
        (*buf)[pos] = 0;
        line_len += 3;

        if (line_len >= MAX_HEX_CHARS)
        {
            write_file(hFile, *buf);
            write_file(hFile, L"\\\r\n  ");
            line_len = 2;
            pos = 0;
        }
    }
}

static void export_newline(HANDLE hFile)
{
    static const WCHAR *newline = L"\r\n";

    write_file(hFile, newline);
}

static void export_data(HANDLE hFile, WCHAR *value_name, DWORD value_len,
                        DWORD type, void *data, size_t size)
{
    WCHAR *buf = NULL;
    size_t line_len = export_value_name(hFile, value_name, value_len);

    switch (type)
    {
    case REG_SZ:
        export_string_data(&buf, data, size);
        break;
    case REG_DWORD:
        if (size)
        {
            export_dword_data(&buf, data);
            break;
        }
        /* fall through */
    case REG_NONE:
    case REG_EXPAND_SZ:
    case REG_BINARY:
    case REG_MULTI_SZ:
    default:
        export_hex_data(hFile, &buf, type, line_len, data, size);
        break;
    }

    if (size || type == REG_SZ)
    {
        write_file(hFile, buf);
        free(buf);
    }

    export_newline(hFile);
}

static void export_key_name(HANDLE hFile, WCHAR *name)
{
    WCHAR *buf;

    buf = malloc((lstrlenW(name) + 7) * sizeof(WCHAR));
    swprintf(buf, L"\r\n[%s]\r\n", name);
    write_file(hFile, buf);
    free(buf);
}

static int export_registry_data(HANDLE hFile, HKEY hkey, WCHAR *path, REGSAM sam)
{
    LONG rc;
    DWORD max_value_len = 256, value_len;
    DWORD max_data_bytes = 2048, data_size;
    DWORD subkey_len;
    DWORD i, type, path_len;
    WCHAR *value_name, *subkey_name, *subkey_path;
    BYTE *data;
    HKEY subkey;

    export_key_name(hFile, path);

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
            export_data(hFile, value_name, value_len, type, data, data_size);
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
                export_registry_data(hFile, subkey, subkey_path, sam);
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

static void export_file_header(HANDLE hFile)
{
    static const WCHAR header[] = L"\xFEFFWindows Registry Editor Version 5.00\r\n";

    write_file(hFile, header);
}

static HANDLE create_file(const WCHAR *filename, DWORD action)
{
    return CreateFileW(filename, GENERIC_WRITE, 0, NULL, action, FILE_ATTRIBUTE_NORMAL, NULL);
}

static HANDLE get_file_handle(WCHAR *filename, BOOL overwrite_file)
{
    HANDLE hFile = create_file(filename, overwrite_file ? CREATE_ALWAYS : CREATE_NEW);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();

        if (error == ERROR_FILE_EXISTS)
        {
            if (!ask_confirm(STRING_OVERWRITE_FILE, filename))
            {
                output_message(STRING_CANCELLED);
                exit(0);
            }

            hFile = create_file(filename, CREATE_ALWAYS);
        }
        else
        {
            WCHAR *str;

            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, 0, (WCHAR *)&str, 0, NULL);
            output_writeconsole(str, lstrlenW(str));
            LocalFree(str);
            exit(1);
        }
    }

    return hFile;
}

int reg_export(int argc, WCHAR *argvW[])
{
    HKEY root, hkey;
    WCHAR *path, *key_name;
    BOOL overwrite_file = FALSE;
    REGSAM sam = 0;
    HANDLE hFile;
    int i, ret;

    if (argc < 4) goto invalid;

    if (!parse_registry_key(argvW[2], &root, &path))
        return 1;

    for (i = 4; i < argc; i++)
    {
        WCHAR *str;

        if (argvW[i][0] != '/' && argvW[i][0] != '-')
            goto invalid;

        str = &argvW[i][1];

        if (is_char(*str, 'y') && !str[1])
            overwrite_file = TRUE;
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
        else
            goto invalid;
    }

    if (sam == (KEY_WOW64_32KEY|KEY_WOW64_64KEY))
        goto invalid;

    if (RegOpenKeyExW(root, path, 0, KEY_READ|sam, &hkey))
    {
        output_message(STRING_KEY_NONEXIST);
        return 1;
    }

    key_name = get_long_key(root, path);

    hFile = get_file_handle(argvW[3], overwrite_file);
    export_file_header(hFile);
    ret = export_registry_data(hFile, hkey, key_name, sam);
    export_newline(hFile);
    CloseHandle(hFile);

    RegCloseKey(hkey);

    return ret;

invalid:
    output_message(STRING_INVALID_SYNTAX);
    output_message(STRING_FUNC_HELP, _wcsupr(argvW[1]));
    return 1;
}
