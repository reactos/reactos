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

#include <stdio.h>
#include "reg.h"

struct key {
    HKEY root;      /* system key */
    WCHAR *subkey;  /* relative path to subkey */
    HKEY hkey;      /* handle to opened or created key */
    WCHAR *path;    /* full path to subkey */
};

enum operation {
    COPY_NO,
    COPY_YES,
    COPY_ALL
};

static void output_error(LONG rc)
{
    if (rc == ERROR_FILE_NOT_FOUND)
        output_message(STRING_KEY_NONEXIST);
    else
        output_message(STRING_ACCESS_DENIED);
}

static enum operation ask_overwrite_value(WCHAR *path, WCHAR *value)
{
    HMODULE hmod;
    static WCHAR Ybuffer[4];
    static WCHAR Nbuffer[4];
    static WCHAR Abuffer[4];
    static WCHAR defval[32];
    WCHAR answer[MAX_PATH];
    WCHAR *str;
    DWORD count;

    hmod = GetModuleHandleW(NULL);
    LoadStringW(hmod, STRING_YES, Ybuffer, ARRAY_SIZE(Ybuffer));
    LoadStringW(hmod, STRING_NO,  Nbuffer, ARRAY_SIZE(Nbuffer));
    LoadStringW(hmod, STRING_ALL, Abuffer, ARRAY_SIZE(Abuffer));
    LoadStringW(hmod, STRING_DEFAULT_VALUE, defval, ARRAY_SIZE(defval));

    str = (value && *value) ? value : defval;

    while (1)
    {
        output_message(STRING_COPY_CONFIRM, path, str);
        output_message(STRING_YESNOALL);

        ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), answer, ARRAY_SIZE(answer), &count, NULL);

        *answer = towupper(*answer);

        if (*answer == *Ybuffer)
            return COPY_YES;
        if (*answer == *Nbuffer)
            return COPY_NO;
        if (*answer == *Abuffer)
            return COPY_ALL;
    }
}

static int run_copy(struct key *src, struct key *dest, REGSAM sam, BOOL recurse, BOOL force)
{
    LONG rc;
    DWORD max_subkey_len;
    DWORD max_name_len, name_len;
    DWORD max_data_size, data_size;
    DWORD type, dispos, i;
    WCHAR *name = NULL;
    BYTE *data = NULL;

    if ((rc = RegOpenKeyExW(src->root, src->subkey, 0, KEY_READ|sam, &src->hkey)))
    {
        output_error(rc);
        return 1;
    }

    if ((rc = RegCreateKeyExW(dest->root, dest->subkey, 0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_READ|KEY_WRITE|sam, NULL, &dest->hkey, &dispos)))
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

        if (!force && dispos == REG_OPENED_EXISTING_KEY)
        {
            if (!RegQueryValueExW(dest->hkey, name, NULL, NULL, NULL, NULL))
            {
                enum operation op;

                op = ask_overwrite_value(src->path, name);
                if (op == COPY_NO) continue;
                if (op == COPY_ALL) force = TRUE;
            }
        }

        if ((rc = RegSetValueExW(dest->hkey, name, 0, type, data, data_size)))
        {
            output_error(rc);
            goto cleanup;
        }
    }

    for (i = 0; recurse; i++)
    {
        struct key subkey_src, subkey_dest;
        size_t path_len;

        name_len = max_name_len;

        rc = RegEnumKeyExW(src->hkey, i, name, &name_len, NULL, NULL, NULL, NULL);
        if (rc) break;

        subkey_src.root = src->hkey;
        subkey_src.subkey = name;

        subkey_dest.root = dest->hkey;
        subkey_dest.subkey = name;

        path_len = lstrlenW(src->path) + name_len + 2;

        if (!(subkey_src.path = malloc(path_len * sizeof(WCHAR))))
        {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        swprintf(subkey_src.path, path_len, L"%s\\%s", src->path, name);

        rc = run_copy(&subkey_src, &subkey_dest, sam, TRUE, force);

        free(subkey_src.path);

        if (rc) goto cleanup;
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
    REGSAM sam = 0;
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

        if (!lstrcmpiW(str, L"reg:32"))
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

    if (sam == (KEY_WOW64_32KEY|KEY_WOW64_64KEY))
        goto invalid;

    if (src.root == dest.root && !lstrcmpiW(src.subkey, dest.subkey))
    {
        output_message(STRING_COPY_SRC_DEST_SAME);
        return 1;
    }

    src.path = src.subkey;

    return run_copy(&src, &dest, sam, recurse, force);

invalid:
    output_message(STRING_INVALID_SYNTAX);
    output_message(STRING_FUNC_HELP, wcsupr(argvW[1]));
    return 1;
}
