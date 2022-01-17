/*
 * Copyright 2021 Hugh McMaster
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

struct key {
    HKEY root;      /* system key */
    WCHAR *subkey;  /* path to subkey */
    HKEY hkey;      /* handle to opened or created key */
};

static void output_error(LONG rc)
{
    if (rc == ERROR_FILE_NOT_FOUND)
        output_message(STRING_KEY_NONEXIST);
    else
        output_message(STRING_ACCESS_DENIED);
}

static int run_copy(struct key *src, struct key *dest, BOOL recurse, BOOL force)
{
    LONG rc;
    DWORD max_subkey_len;
    DWORD max_name_len, name_len;
    DWORD max_data_size, data_size;
    DWORD type, i;
    WCHAR *name = NULL;
    BYTE *data = NULL;

    if ((rc = RegOpenKeyExW(src->root, src->subkey, 0, KEY_READ, &src->hkey)))
    {
        output_error(rc);
        return 1;
    }

    if ((rc = RegCreateKeyExW(dest->root, dest->subkey, 0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_WRITE, NULL, &dest->hkey, NULL)))
    {
        RegCloseKey(src->hkey);
        output_error(rc);
        return 1;
    }

    rc = RegQueryInfoKeyW(src->hkey, NULL, NULL, NULL, NULL, &max_subkey_len, NULL,
                          NULL, &max_name_len, &max_data_size, NULL, NULL);
    if (rc) goto cleanup;

    max_name_len = max(max_subkey_len, max_name_len) + 1;

    if (!(name = malloc(max_name_len * sizeof(WCHAR))))
    {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    if (!(data = malloc(max_data_size)))
    {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    for (i = 0; ; i++)
    {
        name_len = max_name_len;
        data_size = max_data_size;

        rc = RegEnumValueW(src->hkey, i, name, &name_len, NULL, &type, data, &data_size);
        if (rc == ERROR_NO_MORE_ITEMS) break;
        if (rc) goto cleanup;

        if ((rc = RegSetValueExW(dest->hkey, name, 0, type, data, data_size)))
        {
            output_error(rc);
            goto cleanup;
        }
    }

cleanup:
    free(name);
    free(data);

    RegCloseKey(src->hkey);
    RegCloseKey(dest->hkey);

    return rc != ERROR_NO_MORE_ITEMS;
}

int reg_copy(int argc, WCHAR *argvW[])
{
    struct key src, dest;
    BOOL recurse = FALSE, force = FALSE;
    int i;

    if (argc == 3)
        goto invalid;

    if (!parse_registry_key(argvW[2], &src.root, &src.subkey))
        return 1;

    if (!parse_registry_key(argvW[3], &dest.root, &dest.subkey))
        return 1;

    for (i = 4; i < argc; i++)
    {
        WCHAR *str;

        if (argvW[i][0] != '/' && argvW[i][0] != '-')
            goto invalid;

        str = &argvW[i][1];

        if (!lstrcmpiW(str, L"reg:32") || !lstrcmpiW(str, L"reg:64"))
            continue;
        else if (!str[0] || str[1])
            goto invalid;

        switch (towlower(*str))
        {
        case 's':
            if (recurse) goto invalid;
            recurse = TRUE;
            break;
        case 'f':
            if (force) goto invalid;
            force = TRUE;
            break;
        default:
            goto invalid;
        }
    }

    return run_copy(&src, &dest, recurse, force);

invalid:
    output_message(STRING_INVALID_SYNTAX);
    output_message(STRING_FUNC_HELP, wcsupr(argvW[1]));
    return 1;
}
