/*
 * PROJECT:     ReactOS Console Utilities Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Console/terminal paging functionality.
 * COPYRIGHT:   Copyright 2017-2018 ReactOS Team
 *              Copyright 2017-2018 Hermes Belusca-Maito
 */

/**
 * @file    pager.c
 * @ingroup ConUtils
 *
 * @brief   Console/terminal paging functionality.
 **/

/* FIXME: Temporary HACK before we cleanly support UNICODE functions */
#define UNICODE
#define _UNICODE

#include <windef.h>
#include <winbase.h>
// #include <winnls.h>
#include <wincon.h>  // Console APIs (only if kernel32 support included)
#include <strsafe.h>

#include "conutils.h"
#include "stream.h"
#include "screen.h"
#include "pager.h"

// Temporary HACK
#define CON_STREAM_WRITE    ConStreamWrite



/* Returns TRUE when all the text is displayed, and FALSE if display is stopped */
BOOL
ConWritePaging(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN PTCHAR szStr,
    IN DWORD len)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    /* Used to see how big the screen is */
    DWORD ScreenLines = 0;

    /* Chars since start of line */
    DWORD CharSL;

    DWORD from = 0, i = 0;

    /* Parameters validation */
    if (!Pager)
        return FALSE;

    /* Reset LineCount and return if no string has been given */
    if (StartPaging == TRUE)
        Pager->LineCount = 0;
    if (szStr == NULL)
        return TRUE;

    /* Get the size of the visual screen that can be printed to */
    if (!ConGetScreenInfo(Pager->Screen, &csbi))
    {
        /* We assume it's a file handle */
        CON_STREAM_WRITE(Pager->Screen->Stream, szStr, len);
        return TRUE;
    }

    /*
     * Get the number of lines currently displayed on screen, minus 1
     * to account for the "press any key..." prompt from PagePrompt().
     */
    ScreenLines = (csbi.srWindow.Bottom - csbi.srWindow.Top);
    CharSL = csbi.dwCursorPosition.X;

    /* Make sure the user doesn't have the screen too small */
    if (ScreenLines < 4)
    {
        CON_STREAM_WRITE(Pager->Screen->Stream, szStr, len);
        return TRUE;
    }

    while (i < len)
    {
        /* Search until the end of a line is reached */
        if (szStr[i++] != TEXT('\n') && ++CharSL < csbi.dwSize.X)
            continue;

        Pager->LineCount++;
        CharSL = 0;

        if (Pager->LineCount >= ScreenLines)
        {
            CON_STREAM_WRITE(Pager->Screen->Stream, &szStr[from], i-from);
            from = i;

            /* Prompt the user; give him some values for statistics */
            // FIXME TODO: The prompt proc can also take ScreenLines ??
            if (!PagePrompt(Pager, from, len))
                return FALSE;

            // TODO: Recalculate 'ScreenLines' in case the user redimensions
            // the window during the prompt.

            /* Reset the number of lines being printed */
            Pager->LineCount = 0;
        }
    }
    if (i > from)
        CON_STREAM_WRITE(Pager->Screen->Stream, &szStr[from], i-from);

    return TRUE;
}

BOOL
ConPutsPaging(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN LPTSTR szStr)
{
    DWORD len;

    /* Return if no string has been given */
    if (szStr == NULL)
        return TRUE;

    len = (DWORD)wcslen(szStr);
    return ConWritePaging(Pager, PagePrompt, StartPaging, szStr, len);
}

BOOL
ConResPagingEx(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT uID)
{
    INT Len;
    PWCHAR szStr = NULL;

    Len = K32LoadStringW(hInstance, uID, (PWSTR)&szStr, 0);
    if (szStr && Len)
        return ConWritePaging(Pager, PagePrompt, StartPaging, szStr, Len);
    else
        return TRUE;
}

BOOL
ConResPaging(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN UINT uID)
{
    return ConResPagingEx(Pager, PagePrompt, StartPaging,
                          NULL /*GetModuleHandleW(NULL)*/, uID);
}

/* EOF */
