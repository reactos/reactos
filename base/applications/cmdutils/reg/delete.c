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

#include "reg.h"

static BOOL op_delete_key = TRUE;

static void output_error(LONG rc)
{
    if (rc == ERROR_FILE_NOT_FOUND)
    {
        if (op_delete_key)
            output_message(STRING_KEY_NONEXIST);
        else
            output_message(STRING_VALUE_NONEXIST);
    }
    else
    {
        output_message(STRING_ACCESS_DENIED);
    }
}

static int run_delete(HKEY root, WCHAR *path, REGSAM sam, WCHAR *key_name, WCHAR *value_name,
                      BOOL value_empty, BOOL value_all, BOOL force)
{
    LONG rc;
    HKEY hkey;

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

    if ((rc = RegOpenKeyExW(root, path, 0, KEY_READ|KEY_SET_VALUE|sam, &hkey)))
    {
        output_error(rc);
        return 1;
    }

    /* Delete registry key if no /v* option is given */
    if (!value_name && !value_empty && !value_all)
    {
        if ((rc = RegDeleteTreeW(hkey, NULL)))
        {
            RegCloseKey(hkey);
            output_error(rc);
            return 1;
        }

        RegDeleteKeyW(hkey, L"");
        RegCloseKey(hkey);

        output_message(STRING_SUCCESS);
        return 0;
    }

    op_delete_key = FALSE;

    if (value_all)
    {
        DWORD max_value_len = 256, value_len;
        WCHAR *value_name;

        value_name = malloc(max_value_len * sizeof(WCHAR));

        while (1)
        {
            value_len = max_value_len;
            rc = RegEnumValueW(hkey, 0, value_name, &value_len, NULL, NULL, NULL, NULL);
            if (rc == ERROR_SUCCESS)
            {
                rc = RegDeleteValueW(hkey, value_name);
                if (rc != ERROR_SUCCESS)
                {
                    free(value_name);
                    RegCloseKey(hkey);
                    output_message(STRING_VALUEALL_FAILED, key_name);
                    output_error(rc);
                    return 1;
                }
            }
            else if (rc == ERROR_MORE_DATA)
            {
                max_value_len *= 2;
                value_name = realloc(value_name, max_value_len * sizeof(WCHAR));
            }
            else break;
        }
        free(value_name);
    }
    else if (value_name || value_empty)
    {
        if ((rc = RegDeleteValueW(hkey, value_name)))
        {
            RegCloseKey(hkey);
            output_error(rc);
            return 1;
        }
    }

    RegCloseKey(hkey);
    output_message(STRING_SUCCESS);
    return 0;
}

int reg_delete(int argc, WCHAR *argvW[])
{
    HKEY root;
    WCHAR *path, *key_name, *value_name = NULL;
    BOOL value_all = FALSE, value_empty = FALSE, force = FALSE;
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

        if (!lstrcmpiW(str, L"va"))
        {
            if (value_all) goto invalid;
            value_all = TRUE;
            continue;
        }
        else if (!lstrcmpiW(str, L"ve"))
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
        case 'f':
            if (force) goto invalid;
            force = TRUE;
            break;
        default:
            goto invalid;
        }
    }

    if ((value_name && value_empty) || (value_name && value_all) || (value_empty && value_all))
        goto invalid;

    if (sam == (KEY_WOW64_32KEY|KEY_WOW64_64KEY))
        goto invalid;

    key_name = get_long_key(root, path);

    return run_delete(root, path, sam, key_name, value_name, value_empty, value_all, force);

invalid:
    output_message(STRING_INVALID_SYNTAX);
    output_message(STRING_FUNC_HELP, wcsupr(argvW[1]));
    return 1;
}
