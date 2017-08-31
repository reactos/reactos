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

#include "usetup.h"
#include "muilanguages.h"

#define NDEBUG
#include <debug.h>

static
ULONG
FindLanguageIndex(VOID)
{
    ULONG lngIndex = 0;

    if (SelectedLanguageId == NULL)
    {
        /* Default to en-US */
        return 0;   // FIXME!!
        // SelectedLanguageId = L"00000409";
    }

    while (ResourceList[lngIndex].MuiPages != NULL)
    {
        if (_wcsicmp(ResourceList[lngIndex].LanguageID, SelectedLanguageId) == 0)
        {
            return lngIndex;
        }

        lngIndex++;
    }

    return 0;
}


#if 0
BOOLEAN
IsLanguageAvailable(
    PWCHAR LanguageId)
{
    ULONG lngIndex = 0;

    while (ResourceList[lngIndex].MuiPages != NULL)
    {
        if (_wcsicmp(ResourceList[lngIndex].LanguageID, LanguageId) == 0)
            return TRUE;

        lngIndex++;
    }

    return FALSE;
}
#endif


static
const MUI_ENTRY *
FindMUIEntriesOfPage(
    IN ULONG PageNumber)
{
    ULONG muiIndex = 0;
    ULONG lngIndex;
    const MUI_PAGE * Pages = NULL;

    lngIndex = max(FindLanguageIndex(), 0);
    Pages = ResourceList[lngIndex].MuiPages;

    while (Pages[muiIndex].MuiEntry != NULL)
    {
        if (Pages[muiIndex].Number == PageNumber)
            return Pages[muiIndex].MuiEntry;

        muiIndex++;
    }

    return NULL;
}

static
const MUI_ERROR *
FindMUIErrorEntries(VOID)
{
    ULONG lngIndex = max(FindLanguageIndex(), 0);
    return ResourceList[lngIndex].MuiErrors;
}

static
const MUI_STRING *
FindMUIStringEntries(VOID)
{
    ULONG lngIndex = max(FindLanguageIndex(), 0);
    return ResourceList[lngIndex].MuiStrings;
}


VOID
MUIClearPage(
    IN ULONG page)
{
    const MUI_ENTRY * entry;
    ULONG index;

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
    while (entry[index].Buffer != NULL)
    {
        CONSOLE_ClearStyledText(entry[index].X,
                                entry[index].Y,
                                entry[index].Flags,
                                strlen(entry[index].Buffer));
        index++;
    }
}

VOID
MUIDisplayPage(
    IN ULONG page)
{
    const MUI_ENTRY * entry;
    ULONG index;

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
    while (entry[index].Buffer != NULL)
    {
        CONSOLE_SetStyledText(entry[index].X,
                              entry[index].Y,
                              entry[index].Flags,
                              entry[index].Buffer);

        index++;
    }
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

VOID
SetConsoleCodePage(VOID)
{
    UINT wCodePage;

#if 0
    ULONG lngIndex = 0;

    while (ResourceList[lngIndex].MuiPages != NULL)
    {
        if (_wcsicmp(ResourceList[lngIndex].LanguageID, SelectedLanguageId) == 0)
        {
            wCodePage = (UINT) wcstoul(ResourceList[lngIndex].OEMCPage, NULL, 10);
            SetConsoleOutputCP(wCodePage);
            return;
        }

        lngIndex++;
    }
#else
    wCodePage = (UINT)wcstoul(MUIGetOEMCodePage(SelectedLanguageId), NULL, 10);
    SetConsoleOutputCP(wCodePage);
#endif
}

/* EOF */
