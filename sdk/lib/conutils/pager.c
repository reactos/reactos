/*
 * PROJECT:     ReactOS Console Utilities Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Console/terminal paging functionality.
 * COPYRIGHT:   Copyright 2017-2018 ReactOS Team
 *              Copyright 2017-2018 Hermes Belusca-Maito
 *              Copyright 2021 Katayama Hirofumi MZ
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

static BOOL ConPagerAction(PCON_PAGER Pager)
{
    PCTCH TextBuff = Pager->TextBuff;
    DWORD ich = Pager->ich, cch = Pager->cch, iLine = Pager->iLine, iColumn;
    DWORD ScreenColumns = Pager->ScreenColumns, ScreenRows = Pager->ScreenRows;
    DWORD ScrollRows = ScreenRows - 1, ichLast = ich, MaxRows = ScrollRows;

    if (ich >= cch)
        return TRUE;

    switch (Pager->PagerAction)
    {
        case CON_PAGER_ACTION_SHOW_LINE:
            MaxRows = iLine + 1;
            /* ...FALL THROUGH... */
        case CON_PAGER_ACTION_SHOW_PAGE:
            for (iColumn = 0; ich < cch && iLine < MaxRows; ++ich)
            {
                if (TextBuff[ich] == TEXT('\n') || iColumn + 1 >= ScreenColumns)
                {
                    CON_STREAM_WRITE(Pager->Screen->Stream, &TextBuff[ichLast],
                                     ich - ichLast + 1);
                    ichLast = ich + 1;
                    ++iLine;
                    iColumn = 0;
                    continue;
                }
                ++iColumn;
            }
            if (iLine < MaxRows)
            {
                CON_STREAM_WRITE(Pager->Screen->Stream, &TextBuff[ichLast], ich - ichLast);
                ++iLine;
            }
            break;
        case CON_PAGER_ACTION_DO_NOTHING:
            break;
    }

    if (iLine >= ScrollRows)
        iLine = 0; /* Reset the number of lines being printed */

    Pager->ich = ich;
    Pager->iLine = iLine;

    return ich >= cch;
}

/* Returns TRUE when all the text is displayed, and FALSE if display is stopped */
BOOL
ConWritePaging(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN PCTCH szStr,
    IN DWORD len)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    /* Parameters validation */
    if (!Pager)
        return FALSE;

    if (StartPaging)
        Pager->iLine = 0; /* Reset the line count */
    if (szStr == NULL)
        return TRUE; /* Return if no string has been given */

    /* Get the size of the visual screen that can be printed to */
    if (!ConGetScreenInfo(Pager->Screen, &csbi))
    {
        /* We assume it's a file handle */
        CON_STREAM_WRITE(Pager->Screen->Stream, szStr, len);
        return TRUE;
    }

    /* Fill the pager info */
    Pager->ScreenColumns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    Pager->ScreenRows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    Pager->ich = 0;
    Pager->cch = len;
    Pager->TextBuff = szStr;
    Pager->PagerAction = CON_PAGER_ACTION_DEFAULT;

    if (len == 0)
        return TRUE;

    /* Make sure the user doesn't have the screen too small */
    if (Pager->ScreenRows < 4)
    {
        CON_STREAM_WRITE(Pager->Screen->Stream, szStr, len);
        return TRUE;
    }

    while (!ConPagerAction(Pager))
    {
        /* PagePrompt might change this */
        Pager->PagerAction = CON_PAGER_ACTION_DEFAULT;

        /* Prompt the user; give him some values for statistics */
        if (!PagePrompt(Pager, Pager->ich, Pager->cch))
            return FALSE;

        /* Recalculate the screen extent in case the user redimensions
           the window during the prompt. */
        if (ConGetScreenInfo(Pager->Screen, &csbi))
        {
            Pager->ScreenColumns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            Pager->ScreenRows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }
    }

    return TRUE;
}

BOOL
ConPutsPaging(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN PCTSTR szStr)
{
    DWORD len;

    /* Return if no string has been given */
    if (szStr == NULL)
        return TRUE;

    len = wcslen(szStr);
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
    PCWSTR szStr = NULL;

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
