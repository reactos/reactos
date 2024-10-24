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

/* Special characters */
CHAR CharBullet                     = 0x07; /* bullet */
CHAR CharBlock                      = 0xDB; /* block */
CHAR CharHalfBlock                  = 0xDD; /* half-left block */
CHAR CharUpArrow                    = 0x18; /* up arrow */
CHAR CharDownArrow                  = 0x19; /* down arrow */
CHAR CharHorizontalLine             = 0xC4; /* horizontal line */
CHAR CharVerticalLine               = 0xB3; /* vertical line */
CHAR CharUpperLeftCorner            = 0xDA; /* upper left corner */
CHAR CharUpperRightCorner           = 0xBF; /* upper right corner */
CHAR CharLowerLeftCorner            = 0xC0; /* lower left corner */
CHAR CharLowerRightCorner           = 0xD9; /* lower right corner */
CHAR CharVertLineAndRightHorizLine  = 0xC3; /* |- (vertical line and right horizontal line) */
CHAR CharLeftHorizLineAndVertLine   = 0xB4; /* -| (left horizontal line and vertical line) */
CHAR CharDoubleHorizontalLine       = 0xCD; /* double horizontal line (and underline) */
CHAR CharDoubleVerticalLine         = 0xBA; /* double vertical line */
CHAR CharDoubleUpperLeftCorner      = 0xC9; /* double upper left corner */
CHAR CharDoubleUpperRightCorner     = 0xBB; /* double upper right corner */
CHAR CharDoubleLowerLeftCorner      = 0xC8; /* double lower left corner */
CHAR CharDoubleLowerRightCorner     = 0xBC; /* double lower right corner */


LANGID SelectedLanguageId;

static
ULONG
FindLanguageIndex(VOID)
{
    ULONG lngIndex = 0;

    if (SelectedLanguageId == 0)
    {
        /* Default to en-US */
        return 0; // FIXME!!
        // SelectedLanguageId = 0x0409;
    }

    while (ResourceList[lngIndex].MuiPages != NULL)
    {
        if (ResourceList[lngIndex].LanguageID == SelectedLanguageId)
            return lngIndex;
        ++lngIndex;
    }

    return 0;
}

#if 0
BOOLEAN
IsLanguageAvailable(
    _In_ LANGID LanguageId)
{
    ULONG lngIndex = 0;

    while (ResourceList[lngIndex].MuiPages != NULL)
    {
        if (ResourceList[lngIndex].LanguageID == LanguageId)
            return TRUE;
        ++lngIndex;
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
    const MUI_PAGE* Pages;

    lngIndex = FindLanguageIndex();
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
    ULONG lngIndex = FindLanguageIndex();
    return ResourceList[lngIndex].MuiErrors;
}

static
const MUI_STRING *
FindMUIStringEntries(VOID)
{
    ULONG lngIndex = FindLanguageIndex();
    return ResourceList[lngIndex].MuiStrings;
}


#if 0
VOID
MUISetCurrentLanguage(
    _In_ LANGID LanguageId)
{
    SelectedLanguageId = LanguageId;
}
#endif

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
                                (USHORT)strlen(entry[index].Buffer));
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

PCSTR
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

    sprintf(szErr, "Error: failed find string id %lu for language index %lu\n",
            Number, FindLanguageIndex());

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
    ULONG Index = 0;

    /* Get the MUI entry */
    entry = MUIGetEntry(Page, TextID);

    if (!entry)
        return;

    /* Ensure that the text string given by the text ID and page is not NULL */
    while (entry[Index].Buffer != NULL)
    {
        /* If text ID is not correct, skip the entry */
        if (entry[Index].TextID != TextID)
        {
            Index++;
            continue;
        }

        /* Remove the text by using CONSOLE_ClearTextXY() */
        CONSOLE_ClearTextXY(
            entry[Index].X,
            entry[Index].Y,
            (USHORT)strlen(entry[Index].Buffer));

        /* Increment the index and loop over next entires with the same ID */
        Index++;
    }
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
    ULONG Index = 0;

    /* Get the MUI entry */
    entry = MUIGetEntry(Page, TextID);

    if (!entry)
        return;

    /* Ensure that the text string given by the text ID and page is not NULL */
    while (entry[Index].Buffer != NULL)
    {
        /* If text ID is not correct, skip the entry */
        if (entry[Index].TextID != TextID)
        {
            Index++;
            continue;
        }

        /* Now, begin removing the text by calling CONSOLE_ClearStyledText() */
        CONSOLE_ClearStyledText(
            entry[Index].X,
            entry[Index].Y,
            Flags,
            (USHORT)strlen(entry[Index].Buffer));

        /* Increment the index and loop over next entires with the same ID */
        Index++;
    }
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
    ULONG Index = 0;

    /* Get the MUI entry */
    entry = MUIGetEntry(Page, TextID);

    if (!entry)
        return;

    /* Ensure that the text string given by the text ID and page is not NULL */
    while (entry[Index].Buffer != NULL)
    {
        /* If text ID is not correct, skip the entry */
        if (entry[Index].TextID != TextID)
        {
            Index++;
            continue;
        }

        /* Print the text to the console output by calling CONSOLE_SetTextXY() */
        CONSOLE_SetTextXY(entry[Index].X, entry[Index].Y, entry[Index].Buffer);

        /* Increment the index and loop over next entires with the same ID */
        Index++;
    }
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
    ULONG Index = 0;

    /* Get the MUI entry */
    entry = MUIGetEntry(Page, TextID);

    if (!entry)
        return;

    /* Ensure that the text string given by the text ID and page is not NULL */
    while (entry[Index].Buffer != NULL)
    {
        /* If text ID is not correct, skip the entry */
        if (entry[Index].TextID != TextID)
        {
            Index++;
            continue;
        }

        /* Print the text to the console output by calling CONSOLE_SetStyledText() */
        CONSOLE_SetStyledText(entry[Index].X, entry[Index].Y, Flags, entry[Index].Buffer);

        /* Increment the index and loop over next entires with the same ID */
        Index++;
    }
}

VOID
SetConsoleCodePage(VOID)
{
    UINT wCodePage;

#if 0
    ULONG lngIndex = 0;
    while (ResourceList[lngIndex].MuiPages != NULL)
    {
        if (ResourceList[lngIndex].LanguageID == SelectedLanguageId)
        {
            wCodePage = ResourceList[lngIndex].OEMCPage;
            SetConsoleOutputCP(wCodePage);
            return;
        }

        lngIndex++;
    }
#else
    wCodePage = MUIGetOEMCodePage(SelectedLanguageId);
    SetConsoleOutputCP(wCodePage);
#endif

    switch (wCodePage)
    {
        case 28606: /* Romanian */
        case 932: /* Japanese */
            /* Set special characters */
            CharBullet = 0x07;
            CharBlock = 0x01;
            CharHalfBlock = 0x02;
            CharUpArrow = 0x03;
            CharDownArrow = 0x04;
            CharHorizontalLine = 0x05;
            CharVerticalLine = 0x06;
            CharUpperLeftCorner = 0x08;
            CharUpperRightCorner = 0x09;
            CharLowerLeftCorner = 0x0B;
            CharLowerRightCorner = 0x0C;
            CharVertLineAndRightHorizLine = 0x0E;
            CharLeftHorizLineAndVertLine = 0x0F;
            CharDoubleHorizontalLine = 0x10;
            CharDoubleVerticalLine = 0x11;
            CharDoubleUpperLeftCorner = 0x12;
            CharDoubleUpperRightCorner = 0x13;
            CharDoubleLowerLeftCorner = 0x14;
            CharDoubleLowerRightCorner = 0x15;

            /* FIXME: Enter 640x400 video mode */
            break;

        default: /* Other codepages */
            /* Set special characters */
            CharBullet = 0x07;
            CharBlock = 0xDB;
            CharHalfBlock = 0xDD;
            CharUpArrow = 0x18;
            CharDownArrow = 0x19;
            CharHorizontalLine = 0xC4;
            CharVerticalLine = 0xB3;
            CharUpperLeftCorner = 0xDA;
            CharUpperRightCorner = 0xBF;
            CharLowerLeftCorner = 0xC0;
            CharLowerRightCorner = 0xD9;
            CharVertLineAndRightHorizLine = 0xC3;
            CharLeftHorizLineAndVertLine = 0xB4;
            CharDoubleHorizontalLine = 0xCD;
            CharDoubleVerticalLine = 0xBA;
            CharDoubleUpperLeftCorner = 0xC9;
            CharDoubleUpperRightCorner = 0xBB;
            CharDoubleLowerLeftCorner = 0xC8;
            CharDoubleLowerRightCorner = 0xBC;

            /* FIXME: Enter 720x400 video mode */
            break;
    }
}
