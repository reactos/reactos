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
MUIDisplayErrorV(
    IN ULONG ErrorNum,
    OUT PINPUT_RECORD Ir,
    IN ULONG WaitEvent,
    IN va_list args)
{
    const MUI_ERROR* entry;
    CHAR Buffer[2048];

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

    vsprintf(Buffer, entry[ErrorNum].ErrorText, args);

    PopupError(Buffer,
               entry[ErrorNum].ErrorStatus,
               Ir,
               WaitEvent);
}

VOID
__cdecl
MUIDisplayError(
    IN ULONG ErrorNum,
    OUT PINPUT_RECORD Ir,
    IN ULONG WaitEvent,
    ...)
{
    va_list arg_ptr;

    va_start(arg_ptr, WaitEvent);
    MUIDisplayErrorV(ErrorNum, Ir, WaitEvent, arg_ptr);
    va_end(arg_ptr);
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

/**
 * @MUIGetEntry
 *
 * Retrieves a MUI entry of a page, given the page number and the text ID.
 *
 * @param[in]   Page
 *     The MUI (Multilingual User Interface) entry page number, as a unsigned long integer.
 *
 * @param[in]   TextID
 *      The text identification number (ID), as a unsigned integer. The parameter is used to identify
 *      its MUI properties like the coordinates, text style flag and its buffer content.
 *
 * @return
 *     Returns a constant MUI entry.
 *
 */
const MUI_ENTRY *
MUIGetEntry(
    IN ULONG Page,
    IN INT TextID)
{
    const MUI_ENTRY * entry;
    ULONG index;

    /* Retrieve the entries of a MUI page */
    entry = FindMUIEntriesOfPage(Page);
    if (!entry)
    {
        DPRINT("MUIGetEntryData(): Failed to get the translated entry page!\n");
        return NULL;
    }

    /* Loop over the ID entries and check if it matches with one of them */
    for (index = 0; entry[index].Buffer != NULL; index++)
    {
        if (entry[index].TextID == TextID)
        {
            /* They match so return the MUI entry */
            return &entry[index];
        }
    }

    /* Page number or ID are incorrect so in this case bail out */
    DPRINT("Couldn't get the MUI entry field from the page!\n");
    return NULL;
}

/**
 * @MUIClearText
 *
 * Clears a portion of text from the console output.
 *
 * @param[in]   Page
 *     The MUI (Multilingual User Interface) entry page number, as a unsigned long integer.
 *
 * @param[in]   TextID
 *      The text identification number (ID), as an integer. The parameter is used to identify
 *      its MUI properties like the coordinates, text style flag and its buffer content.
 *
 * @return
 *     Nothing.
 *
 */
VOID
MUIClearText(
    IN ULONG Page,
    IN INT TextID)
{
    const MUI_ENTRY * entry;

    /* Get the MUI entry */
    entry = MUIGetEntry(Page, TextID);

    if (!entry)
        return;

    /* Remove the text by using CONSOLE_ClearTextXY() */
    CONSOLE_ClearTextXY(
        entry->X,
        entry->Y,
        (ULONG)strlen(entry->Buffer));
}

/**
 * @MUIClearStyledText
 *
 * Clears a portion of text from the console output, given the actual state style flag of the text.
 *
 * @param[in]   Page
 *     The MUI (Multilingual User Interface) entry page number, as a unsigned long integer.
 *
 * @param[in]   TextID
 *      The text identification number (ID), as an integer. The parameter is used to identify
 *      its MUI properties like the coordinates, text style flag and its buffer content.
 *
 * @param[in]   Flags
 *      The text style flag, as an integer. The flag determines the style of the text, such
 *      as being highlighted, underlined, high padding and so on.
 *
 * @return
 *     Nothing.
 *
 */
VOID
MUIClearStyledText(
    IN ULONG Page,
    IN INT TextID,
    IN INT Flags)
{
    const MUI_ENTRY * entry;

    /* Get the MUI entry */
    entry = MUIGetEntry(Page, TextID);

    if (!entry)
        return;

    /* Now, begin removing the text by calling CONSOLE_ClearStyledText() */
    CONSOLE_ClearStyledText(
        entry->X,
        entry->Y,
        Flags,
        (ULONG)strlen(entry->Buffer));
}

/**
 * @MUISetText
 *
 * Prints a text to the console output.
 *
 * @param[in]   Page
 *     The MUI (Multilingual User Interface) entry page number, as a unsigned long integer.
 *
 * @param[in]   TextID
 *      The text identification number (ID), as an integer. The parameter is used to identify
 *      its MUI properties like the coordinates, text style flag and its buffer content.
 *
 * @return
 *     Nothing.
 *
 */
VOID
MUISetText(
    IN ULONG Page,
    IN INT TextID)
{
    const MUI_ENTRY * entry;

    /* Get the MUI entry */
    entry = MUIGetEntry(Page, TextID);

    if (!entry)
        return;

    /* Print the text to the console output by calling CONSOLE_SetTextXY() */
    CONSOLE_SetTextXY(entry->X, entry->Y, entry->Buffer);
}

/**
 * @MUISetStyledText
 *
 * Prints a text to the console output, with a style for it.
 *
 * @param[in]   Page
 *     The MUI (Multilingual User Interface) entry page number, as a unsigned long integer.
 *
 * @param[in]   TextID
 *      The text identification number (ID), as an integer. The parameter is used to identify
 *      its MUI properties like the coordinates, text style flag and its buffer content.
 *
 *  @param[in]   Flags
 *      The text style flag, as an integer. The flag determines the style of the text, such
 *      as being highlighted, underlined, high padding and so on.
 *
 * @return
 *     Nothing.
 *
 */
VOID
MUISetStyledText(
    IN ULONG Page,
    IN INT TextID,
    IN INT Flags)
{
    const MUI_ENTRY * entry;

    /* Get the MUI entry */
    entry = MUIGetEntry(Page, TextID);

    if (!entry)
        return;

    /* Print the text to the console output by calling CONSOLE_SetStyledText() */
    CONSOLE_SetStyledText(entry->X, entry->Y, Flags, entry->Buffer);
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
