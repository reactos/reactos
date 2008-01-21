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
#include "errorcode.h"
#include "mui.h"

#define NDEBUG
#include <debug.h>

#include "lang/bg-BG.h"
#include "lang/en-US.h"
#include "lang/de-DE.h"
#include "lang/el-GR.h"
#include "lang/es-ES.h"
#include "lang/fr-FR.h"
#include "lang/it-IT.h"
#include "lang/pl-PL.h"
#include "lang/ru-RU.h"
#include "lang/sv-SE.h"
#include "lang/uk-UA.h"
#include "lang/lt-LT.h"

static const MUI_LANGUAGE LanguageList[] =
{
    {
        L"00000409",        /* The Language ID */
        L"00000409",        /* Default Keyboard Layout for this language */
        L"1252",            /* ANSI Codepage */
        L"437",             /* OEM Codepage */
        L"10000",           /* MAC Codepage */
        L"English",         /* Language Name , not used just to make things easier when updating this file */
        enUSPages,          /* Translated page strings  */
        enUSErrorEntries    /* Translated error strings */
    },
    {
        L"00000402",
        L"00000402",
        L"1251",
        L"866",
        L"10007",
        L"Bulgarian",
        bgBGPages,
        bgBGErrorEntries
    },
    {
        L"00000403",
        L"00000403",
        L"1252",
        L"850",
        L"10000",
        L"Catalan",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"00000804",
        L"00000804",
        L"936",
        L"936",
        L"10008",
        L"Chinese (PRC)",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"00000406",
        L"00000406",
        L"1252",
        L"850",
        L"10000",
        L"Danish",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"00000413",
        L"00000813",
        L"1252",
        L"850",
        L"10000",
        L"Dutch",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"0000040B",
        L"0000040B",
        L"1252",
        L"850",
        L"10000",
        L"Finnish",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"0000040C",
        L"0000040C",
        L"1252",
        L"850",
        L"10000",
        L"French",
        frFRPages,
        frFRErrorEntries
    },
    {
        L"00000407",
        L"00000407",
        L"1252",
        L"850",
        L"10000",
        L"German",
        deDEPages,
        deDEErrorEntries
    },
    {
        L"00000408",
        L"00000408",
        L"1253",
        L"737",
        L"10006",
        L"Greek",
        enUSPages, //elGRPages,
        enUSErrorEntries //elGRErrorEntries
    },
    {
        L"0000040D",
        L"0000040D",
        L"1255",
        L"862",
        L"10005",
        L"Hebrew",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"0000040E",
        L"0000040E",
        L"1250",
        L"852",
        L"10029",
        L"Hungarian",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"00000410",
        L"00000410",
        L"1252",
        L"850",
        L"10000",
        L"Italian",
        itITPages,
        itITErrorEntries
    },
    {
        L"00000411",
        L"00000411",
        L"932",
        L"932",
        L"10001",
        L"Japanese",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"00000412",
        L"00000412",
        L"949",
        L"949",
        L"10003",
        L"Korean",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"00000427",
        L"00000427",
        L"1257",
        L"775",
        L"10029",
        L"Lithuanian",
        ltLTPages,
        ltLTErrorEntries
    },
    {
        L"00000414",
        L"00000414",
        L"1252",
        L"850",
        L"10000",
        L"Norwegian",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"00000419",
        L"00000419",
        L"1251",
        L"866",
        L"10007",
        L"Russian",
        ruRUPages,
        ruRUErrorEntries
    },
    {
        L"0000041B",
        L"0000041B",
        L"1250",
        L"852",
        L"10029",
        L"Slovak",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"0000040A",
        L"0000040A",
        L"1252",
        L"850",
        L"10000",
        L"Spanish",
        esESPages,
        esESErrorEntries
    },
    {
        L"00000415",
        L"00000415",
        L"1250",
        L"852",
        L"10029",
        L"Polish",
        plPLPages,
        plPLErrorEntries
    },
    {
        L"00000816",
        L"00000816",
        L"1252",
        L"850",
        L"10000",
        L"Portuguese",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"0000041D",
        L"0000041D",
        L"1252",
        L"850",
        L"10000",
        L"Swedish",
        svSEPages,
        svSEErrorEntries
    },
    {
        L"0000041E",
        L"0000041E",
        L"874",
        L"874",
        L"10021",
        L"Thai",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"0000041F",
        L"0000041F",
        L"1254",
        L"857",
        L"10081",
        L"Turkish",
        enUSPages,
        enUSErrorEntries
    },
    {
        L"00000422",
        L"00000422",
        L"1251",
        L"866",
        L"10017",
        L"Ukrainian",
        ukUAPages,
        ukUAErrorEntries
    },
    {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    }
};

extern
VOID
PopupError(IN PCCH Text,
           IN PCCH Status,
           IN PINPUT_RECORD Ir,
           IN ULONG WaitEvent);

static
const MUI_ENTRY *
FindMUIEntriesOfPage(IN ULONG PageNumber)
{
    ULONG muiIndex = 0;
    ULONG lngIndex = 0;
    const MUI_PAGE * Pages = NULL;

    do
    {
        /* First we search the language list till we find current selected language messages */
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , SelectedLanguageId) == 0)
        {
            /* Get all available pages for this language */
            Pages = LanguageList[lngIndex].MuiPages;

            do
            {
                /* Get page messages */
                if (Pages[muiIndex].Number == PageNumber)
                    return Pages[muiIndex].MuiEntry;

                muiIndex++;
            }
            while (Pages[muiIndex].MuiEntry != NULL);
        }

        lngIndex++;
    }
    while (LanguageList[lngIndex].MuiPages != NULL);

    return NULL;
}

static
const MUI_ERROR *
FindMUIErrorEntries(VOID)
{
    ULONG lngIndex = 0;

    do
    {
        /* First we search the language list till we find current selected language messages */
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , SelectedLanguageId) == 0)
        {
            /* Get all available error messages for this language */
            return LanguageList[lngIndex].MuiErrors;
        }

        lngIndex++;
    }
    while (LanguageList[lngIndex].MuiPages != NULL);

    return NULL;
}

LPCWSTR
MUIDefaultKeyboardLayout(VOID)
{
    ULONG lngIndex = 0;
    do
    {
        /* First we search the language list till we find current selected language messages */
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , SelectedLanguageId) == 0)
        {
            /* Return default keyboard layout */
            return LanguageList[lngIndex].LanguageKeyboardLayoutID;
        }

        lngIndex++;
    }
    while (LanguageList[lngIndex].MuiPages != NULL);

    return NULL;
}

VOID
MUIDisplayPage(IN ULONG page)
{
    const MUI_ENTRY * entry;
    int index;
    int flags;

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
        flags = entry[index].Flags;
        switch(flags)
        {
            case TEXT_NORMAL:
                CONSOLE_SetTextXY(entry[index].X, entry[index].Y, entry[index].Buffer);
                break;
            case TEXT_HIGHLIGHT:
                CONSOLE_SetHighlightedTextXY(entry[index].X, entry[index].Y, entry[index].Buffer);
                break;
            case TEXT_UNDERLINE:
                CONSOLE_SetUnderlinedTextXY(entry[index].X, entry[index].Y, entry[index].Buffer);
                break;
            case TEXT_STATUS:
                CONSOLE_SetStatusText(entry[index].Buffer);
                break;
            default:
                break;
        }
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
