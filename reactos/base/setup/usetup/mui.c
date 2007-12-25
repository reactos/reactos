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

#include "lang/en-US.h"
#include "lang/de-DE.h"
#include "lang/el-GR.h"
#include "lang/es-ES.h"
#include "lang/fr-FR.h"
#include "lang/it-IT.h"
#include "lang/ru-RU.h"
#include "lang/sv-SE.h"
#include "lang/uk-UA.h"

static MUI_LANGUAGE LanguageList[] =
{
    {
        L"00000409",    /* The Language ID */
        L"00000409",    /* Default Keyboard Layout for this language */
        L"English",     /* Language Name , not used just to make things easier when updating this file */
        enUSPages       /* Translated strings  */
    },
    {
        L"0000040C",
        L"0000040C",
        L"French",
        frFRPages
    },
    {
        L"00000407",
        L"00000407",
        L"German",
        deDEPages
    },
    {
        L"00000408",
        L"00000408",
        L"Greek",
        elGRPages
    },
    {
        L"00000410",
        L"00000410",
        L"Italian",
        itITPages
    },
    {
        L"00000419",
        L"00000419",
        L"Russian",
        ruRUPages
    },
    {
        L"0000040A",
        L"0000040A",
        L"Spanish",
        esESPages
    },
    {
        L"0000041D",
        L"0000041D",
        L"Swedish",
        svSEPages
    },
    {
        L"00000422",
        L"00000422",
        L"Ukrainian",
        ukUAPages
    },
    {
        NULL,
        NULL,
        NULL
    }
};

extern
VOID
PopupError(PCHAR Text,
	   PCHAR Status,
	   PINPUT_RECORD Ir,
	   ULONG WaitEvent);

static
MUI_ENTRY *
FindMUIEntriesOfPage (ULONG PageNumber)
{
    ULONG muiIndex = 0;
    ULONG lngIndex = 0;
    MUI_PAGE * Pages = NULL;

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

VOID
MUIDisplayPage(ULONG page)
{
    MUI_ENTRY * entry;
    int index;
    int flags;

    entry = FindMUIEntriesOfPage (page);
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
MUIDisplayError(ULONG ErrorNum, PINPUT_RECORD Ir, ULONG WaitEvent)
{
    if (ErrorNum >= ERROR_LAST_ERROR_CODE)
    {
        PopupError("invalid error number provided",
                    "press enter to continue",
                    Ir,
                    POPUP_WAIT_ENTER);

        return;
    }

    PopupError(enUSErrorEntries[ErrorNum].ErrorText,
               enUSErrorEntries[ErrorNum].ErrorStatus,
               Ir,
               WaitEvent);
}

/* EOF */
