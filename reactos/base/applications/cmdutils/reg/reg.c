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

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <wincon.h>
#include <shlwapi.h>
#include <wine/unicode.h>
#include <wine/debug.h>
#include "reg.h"

#define ARRAY_SIZE(A) (sizeof(A)/sizeof(*A))

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

static int reg_printfW(const WCHAR *msg, ...)
{
    va_list va_args;
    int wlen;
    DWORD count, ret;
    WCHAR msg_buffer[8192];

    va_start(va_args, msg);
    vsnprintfW(msg_buffer, 8192, msg, va_args);
    va_end(va_args);

    wlen = lstrlenW(msg_buffer);
    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), msg_buffer, wlen, &count, NULL);
    if (!ret)
    {
        DWORD len;
        char  *msgA;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile(), assuming the console encoding is still the right
         * one in that case.
         */
        len = WideCharToMultiByte(GetConsoleOutputCP(), 0, msg_buffer, wlen,
            NULL, 0, NULL, NULL);
        msgA = HeapAlloc(GetProcessHeap(), 0, len * sizeof(char));
        if (!msgA)
            return 0;

        WideCharToMultiByte(GetConsoleOutputCP(), 0, msg_buffer, wlen, msgA, len,
            NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        HeapFree(GetProcessHeap(), 0, msgA);
    }

    return count;
}

static int reg_message(int msg)
{
    static const WCHAR formatW[] = {'%','s',0};
    WCHAR msg_buffer[8192];

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer,
        sizeof(msg_buffer)/sizeof(WCHAR));
    return reg_printfW(formatW, msg_buffer);
}

static inline BOOL path_rootname_cmp(const WCHAR *input_path, const WCHAR *rootkey_name)
{
    DWORD length = strlenW(rootkey_name);

    return (!strncmpiW(input_path, rootkey_name, length) &&
            (input_path[length] == 0 || input_path[length] == '\\'));
}

static HKEY path_get_rootkey(const WCHAR *path)
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

static LPBYTE get_regdata(LPWSTR data, DWORD reg_type, WCHAR separator, DWORD *reg_count)
{
    LPBYTE out_data = NULL;
    *reg_count = 0;

    switch (reg_type)
    {
        case REG_SZ:
        {
            *reg_count = (lstrlenW(data) + 1) * sizeof(WCHAR);
            out_data = HeapAlloc(GetProcessHeap(),0,*reg_count);
            lstrcpyW((LPWSTR)out_data,data);
            break;
        }
        case REG_DWORD:
        {
            LPWSTR rest;
            DWORD val;
            val = strtolW(data, &rest, 0);
            if (rest == data) {
                static const WCHAR nonnumber[] = {'E','r','r','o','r',':',' ','/','d',' ','r','e','q','u','i','r','e','s',' ','n','u','m','b','e','r','.','\n',0};
                reg_printfW(nonnumber);
                break;
            }
            *reg_count = sizeof(DWORD);
            out_data = HeapAlloc(GetProcessHeap(),0,*reg_count);
            ((LPDWORD)out_data)[0] = val;
            break;
        }
        case REG_BINARY:
        {
            static const WCHAR nohex[] = {'E','r','r','o','r',':',' ','/','d',' ','r','e','q','u','i','r','e','s',' ','h','e','x',' ','d','a','t','a','.','\n',0};
            BYTE hex0, hex1;
            int i = 0, destByteIndex = 0, datalen = lstrlenW(data);
            *reg_count = ((datalen + datalen % 2) / 2) * sizeof(BYTE);
            out_data = HeapAlloc(GetProcessHeap(), 0, *reg_count);
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
            HeapFree(GetProcessHeap(), 0, out_data);
            reg_printfW(nohex);
            out_data = NULL;
            break;
        }
        default:
        {
            static const WCHAR unhandled[] = {'U','n','h','a','n','d','l','e','d',' ','T','y','p','e',' ','0','x','%','x',' ',' ','d','a','t','a',' ','%','s','\n',0};
            reg_printfW(unhandled, reg_type,data);
        }
    }

    return out_data;
}

static BOOL sane_path(const WCHAR *key)
{
    unsigned int i = strlenW(key);

    if (i < 3 || (key[i - 1] == '\\' && key[i - 2] == '\\'))
    {
        reg_message(STRING_INVALID_KEY);
        return FALSE;
    }

    if (key[0] == '\\' && key[1] == '\\' && key[2] != '\\')
    {
        reg_message(STRING_NO_REMOTE);
        return FALSE;
    }

    return TRUE;
}

static int reg_add(WCHAR *key_name, WCHAR *value_name, BOOL value_empty,
    WCHAR *type, WCHAR separator, WCHAR *data, BOOL force)
{
    static const WCHAR stubW[] = {'A','D','D',' ','-',' ','%','s',
        ' ','%','s',' ','%','d',' ','%','s',' ','%','s',' ','%','d','\n',0};
    LPWSTR p;
    HKEY root,subkey;

    reg_printfW(stubW, key_name, value_name, value_empty, type, data, force);

    if (!sane_path(key_name))
        return 1;

    p = strchrW(key_name,'\\');
    if (!p)
    {
        reg_message(STRING_INVALID_KEY);
        return 1;
    }
    p++;

    root = path_get_rootkey(key_name);
    if (!root)
    {
        reg_message(STRING_INVALID_KEY);
        return 1;
    }

    if(RegCreateKeyW(root,p,&subkey)!=ERROR_SUCCESS)
    {
        reg_message(STRING_INVALID_KEY);
        return 1;
    }

    if (value_name || data)
    {
        DWORD reg_type;
        DWORD reg_count = 0;
        BYTE* reg_data = NULL;

        if (!force)
        {
            if (RegQueryValueW(subkey,value_name,NULL,NULL)==ERROR_SUCCESS)
            {
                /* FIXME:  Prompt for overwrite */
            }
        }

        reg_type = wchar_get_type(type);
        if (reg_type == ~0u)
        {
            RegCloseKey(subkey);
            reg_message(STRING_UNSUPPORTED_TYPE);
            return 1;
        }

        if (data)
            reg_data = get_regdata(data,reg_type,separator,&reg_count);

        RegSetValueExW(subkey,value_name,0,reg_type,reg_data,reg_count);
        HeapFree(GetProcessHeap(),0,reg_data);
    }

    RegCloseKey(subkey);
    reg_message(STRING_SUCCESS);

    return 0;
}

static int reg_delete(WCHAR *key_name, WCHAR *value_name, BOOL value_empty,
    BOOL value_all, BOOL force)
{
    LPWSTR p;
    HKEY root,subkey;

    static const WCHAR stubW[] = {'D','E','L','E','T','E',
        ' ','-',' ','%','s',' ','%','s',' ','%','d',' ','%','d',' ','%','d','\n'
        ,0};
    reg_printfW(stubW, key_name, value_name, value_empty, value_all, force);

    if (!sane_path(key_name))
        return 1;

    p = strchrW(key_name,'\\');
    if (!p)
    {
        reg_message(STRING_INVALID_KEY);
        return 1;
    }
    p++;

    root = path_get_rootkey(key_name);
    if (!root)
    {
        reg_message(STRING_INVALID_KEY);
        return 1;
    }

    if (value_name && value_empty)
    {
        reg_message(STRING_INVALID_CMDLINE);
        return 1;
    }

    if (value_empty && value_all)
    {
        reg_message(STRING_INVALID_CMDLINE);
        return 1;
    }

    if (!force)
    {
        /* FIXME:  Prompt for delete */
    }

    /* Delete subtree only if no /v* option is given */
    if (!value_name && !value_empty && !value_all)
    {
        if (RegDeleteTreeW(root,p)!=ERROR_SUCCESS)
        {
            reg_message(STRING_CANNOT_FIND);
            return 1;
        }
        reg_message(STRING_SUCCESS);
        return 0;
    }

    if(RegOpenKeyW(root,p,&subkey)!=ERROR_SUCCESS)
    {
        reg_message(STRING_CANNOT_FIND);
        return 1;
    }

    if (value_all)
    {
        LPWSTR szValue;
        DWORD maxValue;
        DWORD count;
        LONG rc;

        rc = RegQueryInfoKeyW(subkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            &maxValue, NULL, NULL, NULL);
        if (rc != ERROR_SUCCESS)
        {
            /* FIXME: failure */
            RegCloseKey(subkey);
            return 1;
        }
        maxValue++;
        szValue = HeapAlloc(GetProcessHeap(),0,maxValue*sizeof(WCHAR));

        while (1)
        {
            count = maxValue;
            rc = RegEnumValueW(subkey, 0, szValue, &count, NULL, NULL, NULL, NULL);
            if (rc == ERROR_SUCCESS)
            {
                rc = RegDeleteValueW(subkey, szValue);
                if (rc != ERROR_SUCCESS)
                    break;
            }
            else break;
        }
        if (rc != ERROR_SUCCESS)
        {
            /* FIXME  delete failed */
        }
    }
    else if (value_name)
    {
        if (RegDeleteValueW(subkey,value_name) != ERROR_SUCCESS)
        {
            RegCloseKey(subkey);
            reg_message(STRING_CANNOT_FIND);
            return 1;
        }
    }
    else if (value_empty)
    {
        RegSetValueExW(subkey,NULL,0,REG_SZ,NULL,0);
    }

    RegCloseKey(subkey);
    reg_message(STRING_SUCCESS);
    return 0;
}

static int reg_query(WCHAR *key_name, WCHAR *value_name, BOOL value_empty,
    BOOL subkey)
{
    static const WCHAR stubW[] = {'S','T','U','B',' ','Q','U','E','R','Y',' ',
        '-',' ','%','s',' ','%','s',' ','%','d',' ','%','d','\n',0};
    reg_printfW(stubW, key_name, value_name, value_empty, subkey);

    return 1;
}

int wmain(int argc, WCHAR *argvW[])
{
    int i;

    static const WCHAR addW[] = {'a','d','d',0};
    static const WCHAR deleteW[] = {'d','e','l','e','t','e',0};
    static const WCHAR queryW[] = {'q','u','e','r','y',0};
    static const WCHAR slashDW[] = {'/','d',0};
    static const WCHAR slashFW[] = {'/','f',0};
    static const WCHAR slashHW[] = {'/','h',0};
    static const WCHAR slashSW[] = {'/','s',0};
    static const WCHAR slashTW[] = {'/','t',0};
    static const WCHAR slashVW[] = {'/','v',0};
    static const WCHAR slashVAW[] = {'/','v','a',0};
    static const WCHAR slashVEW[] = {'/','v','e',0};
    static const WCHAR slashHelpW[] = {'/','?',0};

    if (argc < 2 || !lstrcmpW(argvW[1], slashHelpW)
                 || !lstrcmpiW(argvW[1], slashHW))
    {
        reg_message(STRING_USAGE);
        return 0;
    }

    if (!lstrcmpiW(argvW[1], addW))
    {
        WCHAR *key_name, *value_name = NULL, *type = NULL, separator = '\0', *data = NULL;
        BOOL value_empty = FALSE, force = FALSE;

        if (argc < 3)
        {
            reg_message(STRING_INVALID_CMDLINE);
            return 1;
        }
        else if (argc == 3 && (!lstrcmpW(argvW[2], slashHelpW) ||
                               !lstrcmpiW(argvW[2], slashHW)))
        {
            reg_message(STRING_ADD_USAGE);
            return 0;
        }

        key_name = argvW[2];
        for (i = 1; i < argc; i++)
        {
            if (!lstrcmpiW(argvW[i], slashVW))
                value_name = argvW[++i];
            else if (!lstrcmpiW(argvW[i], slashVEW))
                value_empty = TRUE;
            else if (!lstrcmpiW(argvW[i], slashTW))
                type = argvW[++i];
            else if (!lstrcmpiW(argvW[i], slashSW))
                separator = argvW[++i][0];
            else if (!lstrcmpiW(argvW[i], slashDW))
                data = argvW[++i];
            else if (!lstrcmpiW(argvW[i], slashFW))
                force = TRUE;
        }
        return reg_add(key_name, value_name, value_empty, type, separator,
            data, force);
    }
    else if (!lstrcmpiW(argvW[1], deleteW))
    {
        WCHAR *key_name, *value_name = NULL;
        BOOL value_empty = FALSE, value_all = FALSE, force = FALSE;

        if (argc < 3)
        {
            reg_message(STRING_INVALID_CMDLINE);
            return 1;
        }
        else if (argc == 3 && (!lstrcmpW(argvW[2], slashHelpW) ||
                               !lstrcmpiW(argvW[2], slashHW)))
        {
            reg_message(STRING_DELETE_USAGE);
            return 0;
        }

        key_name = argvW[2];
        for (i = 1; i < argc; i++)
        {
            if (!lstrcmpiW(argvW[i], slashVW))
                value_name = argvW[++i];
            else if (!lstrcmpiW(argvW[i], slashVEW))
                value_empty = TRUE;
            else if (!lstrcmpiW(argvW[i], slashVAW))
                value_all = TRUE;
            else if (!lstrcmpiW(argvW[i], slashFW))
                force = TRUE;
        }
        return reg_delete(key_name, value_name, value_empty, value_all, force);
    }
    else if (!lstrcmpiW(argvW[1], queryW))
    {
        WCHAR *key_name, *value_name = NULL;
        BOOL value_empty = FALSE, subkey = FALSE;

        if (argc < 3)
        {
            reg_message(STRING_INVALID_CMDLINE);
            return 1;
        }
        else if (argc == 3 && (!lstrcmpW(argvW[2], slashHelpW) ||
                               !lstrcmpiW(argvW[2], slashHW)))
        {
            reg_message(STRING_QUERY_USAGE);
            return 0;
        }

        key_name = argvW[2];
        for (i = 1; i < argc; i++)
        {
            if (!lstrcmpiW(argvW[i], slashVW))
                value_name = argvW[++i];
            else if (!lstrcmpiW(argvW[i], slashVEW))
                value_empty = TRUE;
            else if (!lstrcmpiW(argvW[i], slashSW))
                subkey = TRUE;
        }
        return reg_query(key_name, value_name, value_empty, subkey);
    }
    else
    {
        reg_message(STRING_INVALID_CMDLINE);
        return 1;
    }
}
