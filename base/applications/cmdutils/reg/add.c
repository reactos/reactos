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
            out_data = malloc(*reg_count);
            lstrcpyW((LPWSTR)out_data,data);
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
                output_message(STRING_MISSING_INTEGER);
                break;
            }
            *reg_count = sizeof(DWORD);
            out_data = malloc(*reg_count);
            ((LPDWORD)out_data)[0] = val;
            break;
        }
        case REG_BINARY:
        {
            BYTE hex0, hex1;
            int i = 0, destByteIndex = 0, datalen = lstrlenW(data);
            *reg_count = ((datalen + datalen % 2) / 2) * sizeof(BYTE);
            out_data = malloc(*reg_count);
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
            free(out_data);
            output_message(STRING_MISSING_HEXDATA);
            out_data = NULL;
            break;
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

int reg_add(HKEY root, WCHAR *path, WCHAR *value_name, BOOL value_empty,
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
        free(reg_data);
    }

    RegCloseKey(key);
    output_message(STRING_SUCCESS);

    return 0;
}
