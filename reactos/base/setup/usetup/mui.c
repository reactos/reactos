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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/mui.c
 * PURPOSE:         Text-mode setup
 * PROGRAMMER:
 */

#include "usetup.h"
#include "interface/consup.h"
#include "errorcode.h"
#include "mui.h"

#define NDEBUG
#include <debug.h>

#include "lang/bg-BG.h"
#include "lang/cs-CZ.h"
#include "lang/en-US.h"
#include "lang/de-DE.h"
#include "lang/el-GR.h"
#include "lang/es-ES.h"
#include "lang/fr-FR.h"
#include "lang/it-IT.h"
#include "lang/pl-PL.h"
#include "lang/ru-RU.h"
#include "lang/sk-SK.h"
#include "lang/sv-SE.h"
#include "lang/uk-UA.h"
#include "lang/lt-LT.h"

static const MUI_LANGUAGE LanguageList[] =
{
  /* Lang ID,   DefKbdLayout, SecKbLayout, ANSI CP, OEM CP, MAC CP,   Language Name,      page strgs,error strings,    other strings */
  {L"00000409", L"00000409",  NULL,        L"1252", L"437", L"10000", L"English",         enUSPages, enUSErrorEntries, enUSStrings },
  {L"0000041C", L"0000041C",  L"00000409", L"1250", L"852", L"10029", L"Albanian",        enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000401", L"00000401",  L"00000409", L"1256", L"720", L"10004", L"Arabic",          enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000813", L"00000813",  L"00000409", L"1252", L"850", L"10000", L"Belgian (Dutch)", enUSPages, enUSErrorEntries, enUSStrings },
  {L"0000080C", L"0000080C",  L"00000409", L"1252", L"850", L"10000", L"Belgian (French)",enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000402", L"00000402",  L"00000409", L"1251", L"866", L"10007", L"Bulgarian",       bgBGPages, bgBGErrorEntries, bgBGStrings },
  {L"00000455", L"00000455",  L"00000409", L"0",    L"1",   L"2",     L"Burmese",         enUSPages, enUSErrorEntries, enUSStrings },// Unicode only
  {L"00000C0C", L"00000C0C",  L"00000409", L"1252", L"850", L"10000", L"Canadian (French)",enUSPages,enUSErrorEntries, enUSStrings },
  {L"00000403", L"00000403",  L"00000409", L"1252", L"850", L"10000", L"Catalan",         enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000804", L"00000804",  L"00000409", L"936",  L"936", L"10008", L"Chinese (PRC)",   enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000405", L"00000405",  L"00000409", L"1250", L"852", L"10029", L"Czech",           csCZPages, csCZErrorEntries, csCZStrings },
  {L"00000406", L"00000406",  L"00000409", L"1252", L"850", L"10000", L"Danish",          enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000407", L"00000407",  L"00000409", L"1252", L"850", L"10000", L"Deutsch",         deDEPages, deDEErrorEntries, deDEStrings },
  {L"00000413", L"00000813",  L"00000409", L"1252", L"850", L"10000", L"Dutch",           enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000425", L"00000425",  L"00000409", L"1257", L"775", L"10029", L"Estonian",        enUSPages, enUSErrorEntries, enUSStrings },
  {L"0000040B", L"0000040B",  L"00000409", L"1252", L"850", L"10000", L"Finnish",         enUSPages, enUSErrorEntries, enUSStrings },
  {L"0000040C", L"0000040C",  L"00000409", L"1252", L"850", L"10000", L"French",          frFRPages, frFRErrorEntries, frFRStrings },
  {L"00000437", L"00000437",  L"00000409", L"0",    L"1",   L"2",     L"Georgian",        enUSPages, enUSErrorEntries, enUSStrings },// Unicode only
  {L"00000408", L"00000408",  L"00000409", L"1253", L"737", L"10006", L"Greek",           elGRPages, elGRErrorEntries, elGRStrings },
  {L"0000040D", L"0000040D",  L"00000409", L"1255", L"862", L"10005", L"Hebrew",          enUSPages, enUSErrorEntries, enUSStrings },
  {L"0000040E", L"0000040E",  L"00000409", L"1250", L"852", L"10029", L"Hungarian",       enUSPages, enUSErrorEntries, enUSStrings },
  {L"0000040F", L"0000040F",  L"00000409", L"1252", L"850", L"10079", L"Icelandic",       enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000410", L"00000410",  L"00000409", L"1252", L"850", L"10000", L"Italian",         itITPages, itITErrorEntries, itITStrings },
  {L"00000411", L"00000411",  L"00000409", L"932",  L"932", L"10001", L"Japanese",        enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000412", L"00000412",  L"00000409", L"949",  L"949", L"10003", L"Korean",          enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000426", L"00000426",  L"00000409", L"1257", L"775", L"10029", L"Latvian",         enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000427", L"00000427",  L"00000409", L"1257", L"775", L"10029", L"Lithuanian",      ltLTPages, ltLTErrorEntries, ltLTStrings },
  {L"0000042F", L"0000042F",  L"00000409", L"1251", L"866", L"10007", L"Macedonian",      enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000414", L"00000414",  L"00000409", L"1252", L"850", L"10000", L"Norwegian",       enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000418", L"00000418",  L"00000409", L"1250", L"852", L"10029", L"Romanian",        enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000419", L"00000419",  L"00000409", L"1251", L"866", L"10007", L"Russkij",         ruRUPages, ruRUErrorEntries, ruRUStrings },
  {L"00000415", L"00000415",  L"00000409", L"1250", L"852", L"10029", L"Polski",          plPLPages, plPLErrorEntries, plPLStrings },
  {L"00000816", L"00000816",  L"00000409", L"1252", L"850", L"10000", L"Portuguese",      enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000C1A", L"00000C1A",  L"00000409", L"1251", L"855", L"10007", L"Serbian (Cyrillic)",enUSPages,enUSErrorEntries,enUSStrings },
  {L"0000081A", L"0000081A",  L"00000409", L"1250", L"852", L"10029", L"Serbian (Latin)", enUSPages, enUSErrorEntries, enUSStrings },
  {L"0000041B", L"0000041B",  L"00000409", L"1250", L"852", L"10029", L"Slovak",          skSKPages, skSKErrorEntries, skSKStrings },
  {L"0000040A", L"0000040A",  L"00000409", L"1252", L"850", L"10000", L"Spanish",         esESPages, esESErrorEntries, esESStrings },
  {L"00000807", L"00000807",  L"00000409", L"1252", L"850", L"10000", L"Swiss (German)",  enUSPages, enUSErrorEntries, enUSStrings },
  {L"0000041D", L"0000041D",  L"00000409", L"1252", L"850", L"10000", L"Swedish",         svSEPages, svSEErrorEntries, svSEStrings },
  {L"00000444", L"00000444",  L"00000409", L"1251", L"866", L"10007", L"Tatar",           enUSPages, enUSErrorEntries, enUSStrings },
  {L"0000041E", L"0000041E",  L"00000409", L"874",  L"874", L"10021", L"Thai",            enUSPages, enUSErrorEntries, enUSStrings },
  {L"0000041F", L"0000041F",  L"00000409", L"1254", L"857", L"10081", L"Turkish",         enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000422", L"00000422",  L"00000409", L"1251", L"866", L"10017", L"Ukrainian",       ukUAPages, ukUAErrorEntries, ukUAStrings },
  {L"00000809", L"00000809",  L"00000409", L"1252", L"850", L"10000", L"United Kingdom",  enUSPages, enUSErrorEntries, enUSStrings },
  {L"00000843", L"00000843",  L"00000409", L"1251", L"866", L"10007", L"Uzbek",           enUSPages, enUSErrorEntries, enUSStrings },
  {L"0000042A", L"0000042A",  L"00000409", L"1258", L"1258",L"10000", L"Vietnamese",      enUSPages, enUSErrorEntries, enUSStrings },
  {NULL, NULL, NULL, NULL, NULL, NULL}
};

extern
VOID
PopupError(IN PCCH Text,
           IN PCCH Status,
           IN PINPUT_RECORD Ir,
           IN ULONG WaitEvent);

static 
ULONG
FindLanguageIndex()
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
    }while (LanguageList[lngIndex].MuiPages != NULL);

    return 0;
}


static
const MUI_ENTRY *
FindMUIEntriesOfPage(IN ULONG PageNumber)
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
    }while (Pages[muiIndex].MuiEntry != NULL);

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
    return LanguageList[lngIndex].LanguageKeyboardLayoutID;
}

VOID
MUIDisplayPage(IN ULONG page)
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
        CONSOLE_SetStyledText (
		    entry[index].X, 
		    entry[index].Y, 
		    entry[index].Flags,
		    entry[index].Buffer);

        index++;
    }
    while (entry[index].Buffer != NULL);
}

VOID
MUIDisplayError(IN ULONG ErrorNum, OUT PINPUT_RECORD Ir, IN ULONG WaitEvent)
{
    const MUI_ERROR * entry;

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

    PopupError(entry[ErrorNum].ErrorText,
               entry[ErrorNum].ErrorStatus,
               Ir,
               WaitEvent);
}

LPSTR
MUIGetString(ULONG Number)
{
    ULONG i;
    const MUI_STRING * entry;
    CHAR szErr[100];

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

static BOOLEAN
AddKbLayoutsToRegistry(IN LPCWSTR DefKbLayout, IN LPCWSTR SecKbLayout)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;
    ULONG Disposition;
    WCHAR szKeyName[48] = L"\\Registry\\User\\.DEFAULT\\Keyboard Layout";

    // Open the keyboard layout key
    RtlInitUnicodeString(&KeyName,
                         szKeyName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status =  NtCreateKey(&KeyHandle,
                          KEY_ALL_ACCESS,
                          &ObjectAttributes,
                          0,
                          NULL,
                          0,
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

    if(!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAppend failed! (%lx)\n", Status);
        DPRINT1("String is %wZ\n", &KeyName);
        return FALSE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateKey(&KeyHandle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         &Disposition);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    /* Set def keyboard layout */
    RtlInitUnicodeString(&ValueName,
                         L"1");

    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           (PVOID)DefKbLayout,
                           (8 + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    if (SecKbLayout != NULL)
    {
        /* Set second keyboard layout */
        RtlInitUnicodeString(&ValueName,
                             L"2");

        Status = NtSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_SZ,
                               (PVOID)SecKbLayout,
                               (8 + 1) * sizeof(WCHAR));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
            NtClose(KeyHandle);
            return FALSE;
        }
    }

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
            return AddKbLayoutsToRegistry(LanguageList[lngIndex].LanguageKeyboardLayoutID,
                                          LanguageList[lngIndex].SecondLangKbLayoutID);
        }

        lngIndex++;
    }
    while (LanguageList[lngIndex].MuiPages != NULL);

    return FALSE;
}

static BOOLEAN
AddCodepageToRegistry(IN LPCWSTR ACPage, IN LPCWSTR OEMCPage, IN LPCWSTR MACCPage)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;

    // Open the nls codepage key
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\CodePage");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status =  NtOpenKey(&KeyHandle,
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
                           wcslen(ACPage) * sizeof(PWCHAR));
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
                           wcslen(OEMCPage) * sizeof(PWCHAR));
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
                           wcslen(MACCPage) * sizeof(PWCHAR));
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
AddCodePage(VOID)
{
    ULONG lngIndex = 0;
    do
    {
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , SelectedLanguageId) == 0)
        {
            return AddCodepageToRegistry(LanguageList[lngIndex].ACPage,
                                         LanguageList[lngIndex].OEMCPage,
                                         LanguageList[lngIndex].MACCPage);
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
