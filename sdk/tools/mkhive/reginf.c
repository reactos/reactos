/*
 *  ReactOS kernel
 *  Copyright (C) 2003, 2006 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/reginf.c
 * PURPOSE:         Inf file import code
 * PROGRAMMERS:     Eric Kohl
 *                  Hervé Poussineau
 */

/* INCLUDES *****************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define NDEBUG
#include "mkhive.h"

#define FLG_ADDREG_BINVALUETYPE           0x00000001
#define FLG_ADDREG_NOCLOBBER              0x00000002
#define FLG_ADDREG_DELVAL                 0x00000004
#define FLG_ADDREG_APPEND                 0x00000008
#define FLG_ADDREG_KEYONLY                0x00000010
#define FLG_ADDREG_OVERWRITEONLY          0x00000020
#define FLG_ADDREG_TYPE_SZ                0x00000000
#define FLG_ADDREG_TYPE_MULTI_SZ          0x00010000
#define FLG_ADDREG_TYPE_EXPAND_SZ         0x00020000
#define FLG_ADDREG_TYPE_BINARY           (0x00000000 | FLG_ADDREG_BINVALUETYPE)
#define FLG_ADDREG_TYPE_DWORD            (0x00010000 | FLG_ADDREG_BINVALUETYPE)
#define FLG_ADDREG_TYPE_NONE             (0x00020000 | FLG_ADDREG_BINVALUETYPE)
#define FLG_ADDREG_TYPE_MASK             (0xFFFF0000 | FLG_ADDREG_BINVALUETYPE)


static const WCHAR HKCR[] = {'H','K','C','R',0};
static const WCHAR HKCU[] = {'H','K','C','U',0};
static const WCHAR HKLM[] = {'H','K','L','M',0};
static const WCHAR HKU[] = {'H','K','U',0};
static const WCHAR HKR[] = {'H','K','R',0};
static const WCHAR BCD[] = {'B','C','D',0};

static const WCHAR HKCRPath[] = {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\','S','O','F','T','W','A','R','E','\\','C','l','a','s','s','e','s','\\',0};
static const WCHAR HKCUPath[] = {'\\','R','e','g','i','s','t','r','y','\\','U','s','e','r','\\','.','D','E','F','A','U','L','T','\\',0};
static const WCHAR HKLMPath[] = {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\',0};
static const WCHAR HKUPath[] = {'\\','R','e','g','i','s','t','r','y','\\','U','s','e','r','\\',0};
static const WCHAR BCDPath[] = {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\',0};

static const WCHAR AddReg[] = {'A','d','d','R','e','g',0};
static const WCHAR DelReg[] = {'D','e','l','R','e','g',0};

/* FUNCTIONS ****************************************************************/

static BOOL
get_root_key(PWCHAR Name)
{
    if (!strcmpiW(Name, HKCR))
    {
        strcpyW(Name, HKCRPath);
        return TRUE;
    }

    if (!strcmpiW(Name, HKCU))
    {
        strcpyW(Name, HKCUPath);
        return TRUE;
    }

    if (!strcmpiW(Name, HKLM))
    {
        strcpyW(Name, HKLMPath);
        return TRUE;
    }

    if (!strcmpiW(Name, HKU))
    {
        strcpyW(Name, HKUPath);
        return TRUE;
    }

    if (!strcmpiW(Name, BCD))
    {
        strcpyW(Name, BCDPath);
        return TRUE;
    }

#if 0
    if (!strcmpiW(Name, HKR))
        return FALSE;
#endif

  return FALSE;
}


/***********************************************************************
 * append_multi_sz_value
 *
 * Append a multisz string to a multisz registry value.
 */
// NOTE: Synced with setupapi/install.c ; see also usetup/registry.c
static VOID
append_multi_sz_value(
    IN HKEY KeyHandle,
    IN PWCHAR ValueName,
    IN PWCHAR Strings,
    IN ULONG StringSize) // In characters
{
    ULONG Size, Total;   // In bytes
    ULONG Type;
    PWCHAR Buffer;
    PWCHAR p;
    size_t len;
    LONG Error;

    Error = RegQueryValueExW(KeyHandle,
                             ValueName,
                             NULL,
                             &Type,
                             NULL,
                             &Size);
    if ((Error != ERROR_SUCCESS) || (Type != REG_MULTI_SZ))
        return;

    Buffer = malloc(Size + StringSize * sizeof(WCHAR));
    if (Buffer == NULL)
        return;

    Error = RegQueryValueExW(KeyHandle,
                             ValueName,
                             NULL,
                             NULL,
                             (PUCHAR)Buffer,
                             &Size);
    if (Error != ERROR_SUCCESS)
        goto done;

    /* compare each string against all the existing ones */
    Total = Size;
    while (*Strings != 0)
    {
        len = strlenW(Strings) + 1;

        for (p = Buffer; *p != 0; p += strlenW(p) + 1)
            if (!strcmpiW(p, Strings))
                break;

        if (*p == 0)  /* not found, need to append it */
        {
            memcpy(p, Strings, len * sizeof(WCHAR));
            p[len] = 0;
            Total += len * sizeof(WCHAR);
        }
        Strings += len;
    }

    if (Total != Size)
    {
        DPRINT("setting value '%S' to '%S'\n", ValueName, Buffer);
        RegSetValueExW(KeyHandle,
                       ValueName,
                       0,
                       REG_MULTI_SZ,
                       (PUCHAR)Buffer,
                       Total + sizeof(WCHAR));
    }

done:
    free(Buffer);
}


/***********************************************************************
 *            do_reg_operation
 *
 * Perform an add/delete registry operation depending on the flags.
 */
static BOOL
do_reg_operation(
    IN HKEY KeyHandle,
    IN PWCHAR ValueName,
    IN PINFCONTEXT Context,
    IN ULONG Flags)
{
    WCHAR EmptyStr = 0;
    ULONG Type;
    ULONG Size;
    LONG Error;

    if (Flags & FLG_ADDREG_DELVAL)  /* deletion */
    {
        if (ValueName)
        {
            RegDeleteValueW(KeyHandle, ValueName);
        }
        else
        {
            RegDeleteKeyW(KeyHandle, NULL);
        }

        return TRUE;
    }

    if (Flags & FLG_ADDREG_KEYONLY)
        return TRUE;

    if (Flags & (FLG_ADDREG_NOCLOBBER | FLG_ADDREG_OVERWRITEONLY))
    {
        Error = RegQueryValueExW(KeyHandle,
                                 ValueName,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
        if ((Error == ERROR_SUCCESS) && (Flags & FLG_ADDREG_NOCLOBBER))
            return TRUE;

        if ((Error != ERROR_SUCCESS) && (Flags & FLG_ADDREG_OVERWRITEONLY))
            return TRUE;
    }

    switch (Flags & FLG_ADDREG_TYPE_MASK)
    {
        case FLG_ADDREG_TYPE_SZ:
            Type = REG_SZ;
            break;

        case FLG_ADDREG_TYPE_MULTI_SZ:
            Type = REG_MULTI_SZ;
            break;

        case FLG_ADDREG_TYPE_EXPAND_SZ:
            Type = REG_EXPAND_SZ;
            break;

        case FLG_ADDREG_TYPE_BINARY:
            Type = REG_BINARY;
            break;

        case FLG_ADDREG_TYPE_DWORD:
            Type = REG_DWORD;
            break;

        case FLG_ADDREG_TYPE_NONE:
            Type = REG_NONE;
            break;

        default:
            Type = Flags >> 16;
            break;
    }

    if (!(Flags & FLG_ADDREG_BINVALUETYPE) ||
        (Type == REG_DWORD && InfHostGetFieldCount(Context) == 5))
    {
        PWCHAR Str = NULL;

        if (Type == REG_MULTI_SZ)
        {
            if (InfHostGetMultiSzField(Context, 5, NULL, 0, &Size) != 0)
                Size = 0;

            if (Size)
            {
                Str = malloc(Size * sizeof(WCHAR));
                if (Str == NULL)
                    return FALSE;

                InfHostGetMultiSzField(Context, 5, Str, Size, NULL);
            }

            if (Flags & FLG_ADDREG_APPEND)
            {
                if (Str == NULL)
                    return TRUE;

                DPRINT("append_multi_sz_value(ValueName = '%S')\n", ValueName);
                append_multi_sz_value(KeyHandle,
                                      ValueName,
                                      Str,
                                      Size);

                free(Str);
                return TRUE;
            }
            /* else fall through to normal string handling */
        }
        else
        {
            if (InfHostGetStringField(Context, 5, NULL, 0, &Size) != 0)
                Size = 0;

            if (Size)
            {
                Str = malloc(Size * sizeof(WCHAR));
                if (Str == NULL)
                    return FALSE;

                InfHostGetStringField(Context, 5, Str, Size, NULL);
            }
        }

        if (Type == REG_DWORD)
        {
            ULONG dw = Str ? strtoulW(Str, NULL, 0) : 0;

            DPRINT("setting dword '%S' to %x\n", ValueName, dw);

            RegSetValueExW(KeyHandle,
                           ValueName,
                           0,
                           Type,
                           (const PUCHAR)&dw,
                           sizeof(ULONG));
        }
        else
        {
            DPRINT("setting value '%S' to '%S'\n", ValueName, Str);

            if (Str)
            {
                RegSetValueExW(KeyHandle,
                               ValueName,
                               0,
                               Type,
                               (PVOID)Str,
                               Size * sizeof(WCHAR));
            }
            else
            {
                RegSetValueExW(KeyHandle,
                               ValueName,
                               0,
                               Type,
                               (PVOID)&EmptyStr,
                               sizeof(WCHAR));
            }
        }
        free(Str);
    }
    else  /* get the binary data */
    {
        PUCHAR Data = NULL;

        if (InfHostGetBinaryField(Context, 5, NULL, 0, &Size) != 0)
            Size = 0;

        if (Size)
        {
            Data = malloc(Size);
            if (Data == NULL)
                return FALSE;

            DPRINT("setting binary data '%S' len %d\n", ValueName, Size);
            InfHostGetBinaryField(Context, 5, Data, Size, NULL);
        }

        RegSetValueExW(KeyHandle,
                       ValueName,
                       0,
                       Type,
                       (PVOID)Data,
                       Size);

        free(Data);
    }

    return TRUE;
}

/***********************************************************************
 *            registry_callback
 *
 * Called once for each AddReg and DelReg entry in a given section.
 */
static BOOL
registry_callback(HINF hInf, PWCHAR Section, BOOL Delete)
{
    WCHAR Buffer[MAX_INF_STRING_LENGTH];
    PWCHAR ValuePtr;
    ULONG Flags;
    size_t Length;

    PINFCONTEXT Context = NULL;
    HKEY KeyHandle;
    BOOL Ok;

    Ok = InfHostFindFirstLine(hInf, Section, NULL, &Context) == 0;
    if (!Ok)
        return TRUE; /* Don't fail if the section isn't present */

    for (;Ok; Ok = (InfHostFindNextLine(Context, Context) == 0))
    {
        /* get root */
        if (InfHostGetStringField(Context, 1, Buffer, sizeof(Buffer)/sizeof(WCHAR), NULL) != 0)
            continue;
        if (!get_root_key(Buffer))
            continue;

        /* get key */
        Length = strlenW(Buffer);
        if (InfHostGetStringField(Context, 2, Buffer + Length, sizeof(Buffer)/sizeof(WCHAR) - (ULONG)Length, NULL) != 0)
            *Buffer = 0;

        DPRINT("KeyName: <%S>\n", Buffer);

        if (Delete)
        {
            Flags = FLG_ADDREG_DELVAL;
        }
        else
        {
            /* get flags */
            if (InfHostGetIntField(Context, 4, (INT *)&Flags) != 0)
                Flags = 0;
        }

        DPRINT("Flags: 0x%x\n", Flags);

        if (Delete || (Flags & FLG_ADDREG_OVERWRITEONLY))
        {
            if (RegOpenKeyW(NULL, Buffer, &KeyHandle) != ERROR_SUCCESS)
            {
                DPRINT("RegOpenKey(%S) failed\n", Buffer);
                continue;  /* ignore if it doesn't exist */
            }
        }
        else
        {
            if (RegCreateKeyW(NULL, Buffer, &KeyHandle) != ERROR_SUCCESS)
            {
                DPRINT("RegCreateKey(%S) failed\n", Buffer);
                continue;
            }
        }

        /* get value name */
        if (InfHostGetStringField(Context, 3, Buffer, sizeof(Buffer)/sizeof(WCHAR), NULL) == 0)
        {
            ValuePtr = Buffer;
        }
        else
        {
            ValuePtr = NULL;
        }

        /* and now do it */
        if (!do_reg_operation(KeyHandle, ValuePtr, Context, Flags))
        {
            return FALSE;
        }
    }

    InfHostFreeContext(Context);

    return TRUE;
}


BOOL
ImportRegistryFile(PCHAR FileName)
{
    HINF hInf;
    ULONG ErrorLine;

    /* Load inf file from install media. */
    if (InfHostOpenFile(&hInf, FileName, 0, &ErrorLine) != 0)
    {
        DPRINT1("InfHostOpenFile(%s) failed\n", FileName);
        return FALSE;
    }

    if (!registry_callback(hInf, (PWCHAR)DelReg, TRUE))
    {
        DPRINT1("registry_callback() for DelReg failed\n");
        InfHostCloseFile(hInf);
        return FALSE;
    }

    if (!registry_callback(hInf, (PWCHAR)AddReg, FALSE))
    {
        DPRINT1("registry_callback() for AddReg failed\n");
        InfHostCloseFile(hInf);
        return FALSE;
    }

    InfHostCloseFile(hInf);
    return TRUE;
}

/* EOF */
