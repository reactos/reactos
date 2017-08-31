/*
 *  ReactOS kernel
 *  Copyright (C) 2008 ReactOS Team
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
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/mui.c
 * PURPOSE:         Text-mode setup
 * PROGRAMMER:
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"
#include "mui.h"
#include "muifonts.h"
#include "muilanguages.h"
#include "registry.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

static
ULONG
FindLanguageIndex(
    IN PCWSTR LanguageId)
{
    ULONG lngIndex = 0;

    if (LanguageId == NULL)
    {
        /* Default to en-US */
        // return 0;   // FIXME!!
        LanguageId = L"00000409";
    }

    while (LanguageList[lngIndex].LanguageID != NULL)
    {
        if (_wcsicmp(LanguageList[lngIndex].LanguageID, LanguageId) == 0)
        {
            return lngIndex;
        }

        lngIndex++;
    }

    return 0;
}

BOOLEAN
IsLanguageAvailable(
    IN PCWSTR LanguageId)
{
    ULONG lngIndex = 0;

    while (LanguageList[lngIndex].LanguageID != NULL)
    {
        if (_wcsicmp(LanguageList[lngIndex].LanguageID, LanguageId) == 0)
            return TRUE;

        lngIndex++;
    }

    return FALSE;
}


PCWSTR
MUIDefaultKeyboardLayout(
    IN PCWSTR LanguageId)
{
    ULONG lngIndex = max(FindLanguageIndex(LanguageId), 0);
    return LanguageList[lngIndex].MuiLayouts[0].LayoutID;
}

PCWSTR
MUIGetOEMCodePage(
    IN PCWSTR LanguageId)
{
    ULONG lngIndex = max(FindLanguageIndex(LanguageId), 0);
    return LanguageList[lngIndex].OEMCPage;
}

PCWSTR
MUIGetGeoID(
    IN PCWSTR LanguageId)
{
    ULONG lngIndex = max(FindLanguageIndex(LanguageId), 0);
    return LanguageList[lngIndex].GeoID;
}

const MUI_LAYOUTS*
MUIGetLayoutsList(
    IN PCWSTR LanguageId)
{
    ULONG lngIndex = max(FindLanguageIndex(LanguageId), 0);
    return LanguageList[lngIndex].MuiLayouts;
}


static
BOOLEAN
AddHotkeySettings(
    IN PCWSTR Hotkey,
    IN PCWSTR LangHotkey,
    IN PCWSTR LayoutHotkey)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    ULONG Disposition;
    NTSTATUS Status;

    RtlInitUnicodeString(&KeyName,
                         L".DEFAULT\\Keyboard Layout\\Toggle");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_USERS, NULL),
                               NULL);

    Status =  NtCreateKey(&KeyHandle,
                          KEY_SET_VALUE,
                          &ObjectAttributes,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    RtlInitUnicodeString(&ValueName,
                         L"Hotkey");

    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           (PVOID)Hotkey,
                           (1 + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    RtlInitUnicodeString(&ValueName,
                         L"Language Hotkey");

    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           (PVOID)LangHotkey,
                           (1 + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    RtlInitUnicodeString(&ValueName,
                         L"Layout Hotkey");

    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           (PVOID)LayoutHotkey,
                           (1 + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    NtClose(KeyHandle);
    return TRUE;
}

BOOLEAN
AddKbLayoutsToRegistry(
    IN const MUI_LAYOUTS *MuiLayouts)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    HANDLE SubKeyHandle;
    NTSTATUS Status;
    ULONG Disposition;
    ULONG uIndex = 0;
    ULONG uCount = 0;
    WCHAR szKeyName[48] = L".DEFAULT\\Keyboard Layout";
    WCHAR szValueName[3 + 1];
    WCHAR szLangID[8 + 1];

    // Open the keyboard layout key
    RtlInitUnicodeString(&KeyName, szKeyName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_USERS, NULL),
                               NULL);

    Status =  NtCreateKey(&KeyHandle,
                          KEY_CREATE_SUB_KEY,
                          &ObjectAttributes,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    NtClose(KeyHandle);

    KeyName.MaximumLength = sizeof(szKeyName);
    Status = RtlAppendUnicodeToString(&KeyName, L"\\Preload");

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAppend failed! (%lx)\n", Status);
        DPRINT1("String is %wZ\n", &KeyName);
        return FALSE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_USERS, NULL),
                               NULL);

    Status = NtCreateKey(&KeyHandle,
                         KEY_SET_VALUE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    RtlInitUnicodeString(&KeyName, L".DEFAULT\\Keyboard Layout\\Substitutes");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_USERS, NULL),
                               NULL);

    Status =  NtCreateKey(&SubKeyHandle,
                          KEY_SET_VALUE,
                          &ObjectAttributes,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
        NtClose(SubKeyHandle);
        NtClose(KeyHandle);
        return FALSE;
    }

    while (MuiLayouts[uIndex].LangID != NULL)
    {
        if (uIndex > 19) break;

        swprintf(szValueName, L"%u", uIndex + 1);
        RtlInitUnicodeString(&ValueName, szValueName);

        swprintf(szLangID, L"0000%s", MuiLayouts[uIndex].LangID);

        if (_wcsicmp(szLangID, MuiLayouts[uIndex].LayoutID) == 0)
        {
            Status = NtSetValueKey(KeyHandle,
                                   &ValueName,
                                   0,
                                   REG_SZ,
                                   (PVOID)MuiLayouts[uIndex].LayoutID,
                                   (wcslen(MuiLayouts[uIndex].LayoutID)+1) * sizeof(WCHAR));
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtSetValueKey() failed (Status = %lx, uIndex = %d)\n", Status, uIndex);
                NtClose(SubKeyHandle);
                NtClose(KeyHandle);
                return FALSE;
            }
        }
        else
        {
            swprintf(szLangID, L"d%03lu%s", uCount, MuiLayouts[uIndex].LangID);
            Status = NtSetValueKey(KeyHandle,
                                   &ValueName,
                                   0,
                                   REG_SZ,
                                   (PVOID)szLangID,
                                   (wcslen(szLangID)+1) * sizeof(WCHAR));
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtSetValueKey() failed (Status = %lx, uIndex = %d)\n", Status, uIndex);
                NtClose(SubKeyHandle);
                NtClose(KeyHandle);
                return FALSE;
            }

            RtlInitUnicodeString(&ValueName, szLangID);

            Status = NtSetValueKey(SubKeyHandle,
                                   &ValueName,
                                   0,
                                   REG_SZ,
                                   (PVOID)MuiLayouts[uIndex].LayoutID,
                                   (wcslen(MuiLayouts[uIndex].LayoutID)+1) * sizeof(WCHAR));
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtSetValueKey() failed (Status = %lx, uIndex = %u)\n", Status, uIndex);
                NtClose(SubKeyHandle);
                NtClose(KeyHandle);
                return FALSE;
            }

            uCount++;
        }

        uIndex++;
    }

    if (uIndex > 1)
        AddHotkeySettings(L"2", L"2", L"1");
    else
        AddHotkeySettings(L"3", L"3", L"3");

    NtClose(SubKeyHandle);
    NtClose(KeyHandle);
    return TRUE;
}

BOOLEAN
AddKeyboardLayouts(
    IN PCWSTR LanguageId)
{
    ULONG lngIndex = 0;

    while (LanguageList[lngIndex].LanguageID != NULL)
    {
        if (_wcsicmp(LanguageList[lngIndex].LanguageID, LanguageId) == 0)
        {
            return AddKbLayoutsToRegistry(LanguageList[lngIndex].MuiLayouts);
        }

        lngIndex++;
    }

    return FALSE;
}

static
BOOLEAN
AddCodepageToRegistry(
    IN PCWSTR ACPage,
    IN PCWSTR OEMCPage,
    IN PCWSTR MACCPage)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;

    // Open the nls codepage key
    RtlInitUnicodeString(&KeyName,
                         L"SYSTEM\\CurrentControlSet\\Control\\NLS\\CodePage");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_LOCAL_MACHINE, NULL),
                               NULL);
    Status = NtOpenKey(&KeyHandle,
                       KEY_WRITE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    // Set ANSI codepage
    RtlInitUnicodeString(&ValueName, L"ACP");
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           (PVOID)ACPage,
                           (wcslen(ACPage)+1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    // Set OEM codepage
    RtlInitUnicodeString(&ValueName, L"OEMCP");
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           (PVOID)OEMCPage,
                           (wcslen(OEMCPage)+1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    // Set MAC codepage
    RtlInitUnicodeString(&ValueName, L"MACCP");
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           (PVOID)MACCPage,
                           (wcslen(MACCPage)+1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    NtClose(KeyHandle);

    return TRUE;
}

static
BOOLEAN
AddFontsSettingsToRegistry(
    IN const MUI_SUBFONT * MuiSubFonts)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;
    ULONG uIndex = 0;

    RtlInitUnicodeString(&KeyName,
                         L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_LOCAL_MACHINE, NULL),
                               NULL);
    Status = NtOpenKey(&KeyHandle,
                       KEY_WRITE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    while (MuiSubFonts[uIndex].FontName != NULL)
    {
        RtlInitUnicodeString(&ValueName, MuiSubFonts[uIndex].FontName);
        Status = NtSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_SZ,
                               (PVOID)MuiSubFonts[uIndex].SubFontName,
                               (wcslen(MuiSubFonts[uIndex].SubFontName)+1) * sizeof(WCHAR));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtSetValueKey() failed (Status = %lx, uIndex = %d)\n", Status, uIndex);
            NtClose(KeyHandle);
            return FALSE;
        }

        uIndex++;
    }

    NtClose(KeyHandle);

    return TRUE;
}

BOOLEAN
AddCodePage(
    IN PCWSTR LanguageId)
{
    ULONG lngIndex = 0;

    while (LanguageList[lngIndex].LanguageID != NULL)
    {
        if (_wcsicmp(LanguageList[lngIndex].LanguageID, LanguageId) == 0)
        {
            if (AddCodepageToRegistry(LanguageList[lngIndex].ACPage,
                                      LanguageList[lngIndex].OEMCPage,
                                      LanguageList[lngIndex].MACCPage)&&
                AddFontsSettingsToRegistry(LanguageList[lngIndex].MuiSubFonts))
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }

        lngIndex++;
    }

    return FALSE;
}

/* EOF */
