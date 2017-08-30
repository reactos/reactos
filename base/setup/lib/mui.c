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

extern
VOID
PopupError(IN PCCH Text,
           IN PCCH Status,
           IN PINPUT_RECORD Ir,
           IN ULONG WaitEvent);

static
ULONG
FindLanguageIndex(VOID)
{
    ULONG lngIndex = 0;

    if (SelectedLanguageId == NULL)
    {
        /* default to english */
        return 0;
    }

    do
    {
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , SelectedLanguageId) == 0)
        {
            return lngIndex;
        }

        lngIndex++;
    } while (LanguageList[lngIndex].MuiPages != NULL);

    return 0;
}


BOOLEAN
IsLanguageAvailable(
    PWCHAR LanguageId)
{
    ULONG lngIndex = 0;

    do
    {
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , LanguageId) == 0)
            return TRUE;

        lngIndex++;
    } while (LanguageList[lngIndex].MuiPages != NULL);

    return FALSE;
}


static
const MUI_ENTRY *
FindMUIEntriesOfPage(
    IN ULONG PageNumber)
{
    ULONG muiIndex = 0;
    ULONG lngIndex;
    const MUI_PAGE * Pages = NULL;

    lngIndex = max(FindLanguageIndex(), 0);
    Pages = LanguageList[lngIndex].MuiPages;

    do
    {
         if (Pages[muiIndex].Number == PageNumber)
             return Pages[muiIndex].MuiEntry;

         muiIndex++;
    } while (Pages[muiIndex].MuiEntry != NULL);

    return NULL;
}


static
const MUI_ERROR *
FindMUIErrorEntries(VOID)
{
    ULONG lngIndex = max(FindLanguageIndex(), 0);
    return LanguageList[lngIndex].MuiErrors;
}


static
const MUI_STRING *
FindMUIStringEntries(VOID)
{
    ULONG lngIndex = max(FindLanguageIndex(), 0);
    return LanguageList[lngIndex].MuiStrings;
}


LPCWSTR
MUIDefaultKeyboardLayout(VOID)
{
    ULONG lngIndex = max(FindLanguageIndex(), 0);
    return LanguageList[lngIndex].MuiLayouts[0].LayoutID;
}


PWCHAR
MUIGetGeoID(VOID)
{
    ULONG lngIndex = max(FindLanguageIndex(), 0);
    return LanguageList[lngIndex].GeoID;
}


const MUI_LAYOUTS *
MUIGetLayoutsList(VOID)
{
    ULONG lngIndex = max(FindLanguageIndex(), 0);
    return LanguageList[lngIndex].MuiLayouts;
}


VOID
MUIClearPage(
    IN ULONG page)
{
    const MUI_ENTRY * entry;
    int index;

    entry = FindMUIEntriesOfPage(page);
    if (!entry)
    {
        PopupError("Error: Failed to find translated page",
                   NULL,
                   NULL,
                   POPUP_WAIT_NONE);
        return;
    }

    index = 0;
    do
    {
        CONSOLE_ClearStyledText(entry[index].X,
                                entry[index].Y,
                                entry[index].Flags,
                                strlen(entry[index].Buffer));
        index++;
    }
    while (entry[index].Buffer != NULL);
}


VOID
MUIDisplayPage(
    IN ULONG page)
{
    const MUI_ENTRY * entry;
    int index;

    entry = FindMUIEntriesOfPage(page);
    if (!entry)
    {
        PopupError("Error: Failed to find translated page",
                   NULL,
                   NULL,
                   POPUP_WAIT_NONE);
        return;
    }

    index = 0;
    do
    {
        CONSOLE_SetStyledText(entry[index].X,
                              entry[index].Y,
                              entry[index].Flags,
                              entry[index].Buffer);

        index++;
    }
    while (entry[index].Buffer != NULL);
}


VOID
MUIDisplayError(
    IN ULONG ErrorNum,
    OUT PINPUT_RECORD Ir,
    IN ULONG WaitEvent,
    ...)
{
    const MUI_ERROR * entry;
    CHAR Buffer[2048];
    va_list ap;

    if (ErrorNum >= ERROR_LAST_ERROR_CODE)
    {
        PopupError("Invalid error number provided",
                   "Press ENTER to continue",
                   Ir,
                   POPUP_WAIT_ENTER);

        return;
    }

    entry = FindMUIErrorEntries();
    if (!entry)
    {
        PopupError("Error: Failed to find translated error message",
                   NULL,
                   NULL,
                   POPUP_WAIT_NONE);
        return;
    }

    va_start(ap, WaitEvent);
    vsprintf(Buffer, entry[ErrorNum].ErrorText, ap);
    va_end(ap);

    PopupError(Buffer,
               entry[ErrorNum].ErrorStatus,
               Ir,
               WaitEvent);
}


LPSTR
MUIGetString(
    ULONG Number)
{
    ULONG i;
    const MUI_STRING * entry;
    CHAR szErr[128];

    entry = FindMUIStringEntries();
    if (entry)
    {
        for (i = 0; entry[i].Number != 0; i++)
        {
            if (entry[i].Number == Number)
            {
                return entry[i].String;
            }
        }
    }

    sprintf(szErr, "Error: failed find string id %lu for language index %lu\n", Number, FindLanguageIndex());

    PopupError(szErr,
               NULL,
               NULL,
               POPUP_WAIT_NONE);

    return "<nostring>";
}


static
BOOLEAN
AddHotkeySettings(
    IN LPCWSTR Hotkey,
    IN LPCWSTR LangHotkey,
    IN LPCWSTR LayoutHotkey)
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

    if(!NT_SUCCESS(Status))
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

    if(NT_SUCCESS(Status))
        NtClose(KeyHandle);
    else
    {
        DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

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

    if(!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
        NtClose(SubKeyHandle);
        NtClose(KeyHandle);
        return FALSE;
    }

    do
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
    while (MuiLayouts[uIndex].LangID != NULL);

    if (uIndex > 1)
        AddHotkeySettings(L"2", L"2", L"1");
    else
        AddHotkeySettings(L"3", L"3", L"3");

    NtClose(SubKeyHandle);
    NtClose(KeyHandle);
    return TRUE;
}


BOOLEAN
AddKeyboardLayouts(VOID)
{
    ULONG lngIndex = 0;

    do
    {
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , SelectedLanguageId) == 0)
        {
            return AddKbLayoutsToRegistry(LanguageList[lngIndex].MuiLayouts);
        }

        lngIndex++;
    }
    while (LanguageList[lngIndex].MuiPages != NULL);

    return FALSE;
}


static
BOOLEAN
AddCodepageToRegistry(
    IN LPCWSTR ACPage,
    IN LPCWSTR OEMCPage,
    IN LPCWSTR MACCPage)
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

    do
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
    while (MuiSubFonts[uIndex].FontName != NULL);

    NtClose(KeyHandle);

    return TRUE;
}


BOOLEAN
AddCodePage(VOID)
{
    ULONG lngIndex = 0;
    do
    {
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , SelectedLanguageId) == 0)
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
    while (LanguageList[lngIndex].MuiPages != NULL);

    return FALSE;
}


VOID
SetConsoleCodePage(VOID)
{
    ULONG lngIndex = 0;
    UINT wCodePage;

    do
    {
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , SelectedLanguageId) == 0)
        {
            wCodePage = (UINT) wcstoul(LanguageList[lngIndex].OEMCPage, NULL, 10);
            SetConsoleOutputCP(wCodePage);
            return;
        }

        lngIndex++;
    }
    while (LanguageList[lngIndex].MuiPages != NULL);
}

/* EOF */
