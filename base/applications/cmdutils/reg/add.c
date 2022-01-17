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

#include <errno.h>
#include "reg.h"

static DWORD wchar_get_type(const WCHAR *type_name)
{
    DWORD i;

    if (!type_name)
        return REG_SZ;

    for (i = 0; i < ARRAY_SIZE(type_rels); i++)
    {
        if (!wcsicmp(type_rels[i].name, type_name))
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

static BOOL get_regdata(const WCHAR *data, DWORD reg_type, WCHAR separator,
                        BYTE **data_bytes, DWORD *size_bytes)
{
    static const WCHAR empty;

    *size_bytes = 0;

    if (!data) data = &empty;

    switch (reg_type)
    {
        case REG_NONE:
        case REG_SZ:
        case REG_EXPAND_SZ:
        {
            *size_bytes = (lstrlenW(data) + 1) * sizeof(WCHAR);
            *data_bytes = malloc(*size_bytes);
            lstrcpyW((WCHAR *)*data_bytes, data);
            break;
        }
        case REG_DWORD:
     /* case REG_DWORD_LITTLE_ENDIAN: */
        case REG_DWORD_BIG_ENDIAN: /* Yes, this is correct! */
        {
            LPWSTR rest;
            unsigned long val;
            val = wcstoul(data, &rest, (towlower(data[1]) == 'x') ? 16 : 10);
            if (*rest || data[0] == '-' || (val == ~0u && errno == ERANGE)) {
                output_message(STRING_MISSING_NUMBER);
                return FALSE;
            }
            *size_bytes = sizeof(DWORD);
            *data_bytes = malloc(*size_bytes);
            *(DWORD *)*data_bytes = val;
            break;
        }
        case REG_BINARY:
        {
            BYTE hex0, hex1, *ptr;
            int i = 0, destByteIndex = 0, datalen = lstrlenW(data);

            if (!datalen) return TRUE;

            *size_bytes = ((datalen + datalen % 2) / 2) * sizeof(BYTE);
            *data_bytes = malloc(*size_bytes);

            if (datalen % 2)
            {
                hex1 = hexchar_to_byte(data[i++]);
                if (hex1 == 0xFF)
                    goto no_hex_data;
                *data_bytes[destByteIndex++] = hex1;
            }

            ptr = *data_bytes;

            for (; i + 1 < datalen; i += 2)
            {
                hex0 = hexchar_to_byte(data[i]);
                hex1 = hexchar_to_byte(data[i + 1]);
                if (hex0 == 0xFF || hex1 == 0xFF)
                    goto no_hex_data;
                ptr[destByteIndex++] = (hex0 << 4) | hex1;
            }
            break;

            no_hex_data:
            free(*data_bytes);
            *data_bytes = NULL;
            output_message(STRING_MISSING_HEXDATA);
            return FALSE;
        }
        case REG_MULTI_SZ:
        {
            int i, destindex, len = lstrlenW(data);
            WCHAR *buffer = malloc((len + 2) * sizeof(WCHAR));

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
                    free(buffer);
                    output_message(STRING_INVALID_STRING);
                    return FALSE;
                }
            }
            buffer[destindex] = 0;
            if (destindex && buffer[destindex - 1])
                buffer[++destindex] = 0;
            *size_bytes = (destindex + 1) * sizeof(WCHAR);
            *data_bytes = (BYTE *)buffer;
            break;
        }
        default:
            output_message(STRING_UNHANDLED_TYPE, reg_type, data);
    }

    return TRUE;
}

static int run_add(HKEY root, WCHAR *path, REGSAM sam, WCHAR *value_name, BOOL value_empty,
                   WCHAR *type, WCHAR separator, WCHAR *data, BOOL force)
{
    HKEY hkey;
    DWORD dispos, data_type, data_size;
    BYTE *reg_data = NULL;
    LONG rc;

    if (RegCreateKeyExW(root, path, 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ|KEY_WRITE|sam, NULL, &hkey, &dispos))
    {
        output_message(STRING_ACCESS_DENIED);
        return 1;
    }

    if (!force && dispos == REG_OPENED_EXISTING_KEY)
    {
        if (RegQueryValueExW(hkey, value_name, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            if (!ask_confirm(STRING_OVERWRITE_VALUE, value_name))
            {
                RegCloseKey(hkey);
                output_message(STRING_CANCELLED);
                return 0;
            }
        }
    }

    data_type = wchar_get_type(type);

    if (data_type == ~0u)
    {
        RegCloseKey(hkey);
        output_message(STRING_UNSUPPORTED_TYPE, type);
        return 1;
    }

    if ((data_type == REG_DWORD || data_type == REG_DWORD_BIG_ENDIAN) && !data)
    {
         RegCloseKey(hkey);
         output_message(STRING_INVALID_CMDLINE);
         return 1;
    }

    if (!get_regdata(data, data_type, separator, &reg_data, &data_size))
    {
        RegCloseKey(hkey);
        return 1;
    }

    rc = RegSetValueExW(hkey, value_name, 0, data_type, reg_data, data_size);

    free(reg_data);
    RegCloseKey(hkey);

    if (rc)
    {
        output_message(STRING_ACCESS_DENIED);
        return 1;
    }

    output_message(STRING_SUCCESS);

    return 0;
}

int reg_add(int argc, WCHAR *argvW[])
{
    HKEY root;
    WCHAR *path, *value_name = NULL, *type = NULL, *data = NULL, separator = '\0';
    BOOL value_empty = FALSE, force = FALSE;
    REGSAM sam = 0;
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
        case 't':
            if (type || !(type = argvW[++i]))
                goto invalid;
            break;
        case 'd':
            if (data || !(data = argvW[++i]))
                goto invalid;
            break;
        case 's':
            str = argvW[++i];
            if (separator || !str || lstrlenW(str) != 1)
                goto invalid;
            separator = str[0];
            break;
        case 'f':
            if (force) goto invalid;
            force = TRUE;
            break;
        default:
            goto invalid;
        }
    }

    if (value_name && value_empty)
        goto invalid;

    if (sam == (KEY_WOW64_32KEY|KEY_WOW64_64KEY))
        goto invalid;

    return run_add(root, path, sam, value_name, value_empty, type, separator, data, force);

invalid:
    output_message(STRING_INVALID_SYNTAX);
    output_message(STRING_FUNC_HELP, wcsupr(argvW[1]));
    return 1;
}
