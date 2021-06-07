/*
 * PROJECT:     ReactOS Console Utilities Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Console/terminal paging functionality.
 * COPYRIGHT:   Copyright 2017-2021 Hermes Belusca-Maito
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
#include <winnls.h> // for WideCharToMultiByte
#include <strsafe.h>

#include "conutils.h"
#include "stream.h"
#include "screen.h"
#include "pager.h"

// Temporary HACK
#define CON_STREAM_WRITE    ConStreamWrite

#define CP_SHIFTJIS 932  // Japanese Shift-JIS
#define CP_HANGUL   949  // Korean Hangul/Wansung
#define CP_JOHAB    1361 // Korean Johab
#define CP_GB2312   936  // Chinese Simplified (GB2312)
#define CP_BIG5     950  // Chinese Traditional (Big5)

/* IsFarEastCP(CodePage) */
#define IsCJKCodePage(CodePage) \
    ((CodePage) == CP_SHIFTJIS || (CodePage) == CP_HANGUL || \
  /* (CodePage) == CP_JOHAB || */ \
     (CodePage) == CP_BIG5     || (CodePage) == CP_GB2312)

static inline INT
GetWidthOfCharCJK(
    IN UINT nCodePage,
    IN WCHAR ch)
{
    INT ret = WideCharToMultiByte(nCodePage, 0, &ch, 1, NULL, 0, NULL, NULL);
    if (ret == 0)
        ret = 1;
    else if (ret > 2)
        ret = 2;
    return ret;
}

static VOID
ConCallPagerLine(
    IN OUT PCON_PAGER Pager,
    IN PCTCH line,
    IN DWORD cch)
{
    Pager->dwFlags &= ~CON_PAGER_FLAG_DONT_OUTPUT; /* Clear the flag */

    if (!Pager->PagerLine || !Pager->PagerLine(Pager, line, cch))
        CON_STREAM_WRITE(Pager->Screen->Stream, line, cch);
}

static BOOL
ConPagerWorker(IN PCON_PAGER Pager)
{
    const DWORD ScreenColumns = Pager->ScreenColumns;
    const DWORD ScrollRows = Pager->ScrollRows;
    const PCTCH TextBuff = Pager->TextBuff;
    const DWORD cch = Pager->cch;
    LONG nTabWidth = Pager->nTabWidth;

    DWORD ich = Pager->ich;
    DWORD iColumn = Pager->iColumn;
    DWORD iLine = Pager->iLine;
    DWORD lineno = Pager->lineno;

    DWORD ichStart = ich;
    UINT nCodePage;
    BOOL IsCJK;
    UINT nWidthOfChar = 1;
    BOOL IsDoubleWidthCharTrailing = FALSE;

    if (ich >= cch)
        return FALSE;

    nCodePage = GetConsoleOutputCP();
    IsCJK = IsCJKCodePage(nCodePage);

    /* Normalize the tab width: if negative or too large,
     * cap it to the number of columns. */
    if (nTabWidth < 0)
        nTabWidth = ScreenColumns - 1;
    else
        nTabWidth = min(nTabWidth, ScreenColumns - 1);

    if (Pager->dwFlags & CON_PAGER_FLAG_EXPAND_TABS)
    {
ExpandTab:
        while ((Pager->nSpacePending > 0) && (iLine < ScrollRows))
        {
            ConCallPagerLine(Pager, L" ", 1);
            --(Pager->nSpacePending);
            ++iColumn;
            if (iColumn % ScreenColumns == 0)
            {
                if (!(Pager->dwFlags & CON_PAGER_FLAG_DONT_OUTPUT))
                    ++iLine;
            }
        }
    }

    /* Loop over each character in the buffer */
    for (; ich < cch && iLine < ScrollRows; ++ich)
    {
        Pager->lineno = lineno;

        /* NEWLINE character */
        if (TextBuff[ich] == TEXT('\n'))
        {
            /* Output the pending text, including the newline */
            ConCallPagerLine(Pager, &TextBuff[ichStart], ich - ichStart + 1);
            ichStart = ich + 1;
            if (!(Pager->dwFlags & CON_PAGER_FLAG_DONT_OUTPUT))
                ++iLine;
            iColumn = 0;

            /* Done with this line; start a new one */
            ++lineno;
            continue;
        }

        /* TAB character */
        if (TextBuff[ich] == TEXT('\t') &&
            (Pager->dwFlags & CON_PAGER_FLAG_EXPAND_TABS))
        {
            /* Output the pending text */
            ConCallPagerLine(Pager, &TextBuff[ichStart], ich - ichStart);

            /* Perform tab expansion, unless the tab width is zero */
            if (nTabWidth == 0)
            {
                ichStart = ich + 1;
                continue;
            }
            ichStart = ++ich;
            Pager->nSpacePending += nTabWidth - (iColumn % nTabWidth);
            goto ExpandTab;
        }

        /* FORM-FEED character */
        if (TextBuff[ich] == TEXT('\f') &&
            (Pager->dwFlags & CON_PAGER_FLAG_EXPAND_FF))
        {
            /* Output the pending text, skipping the form-feed */
            ConCallPagerLine(Pager, &TextBuff[ichStart], ich - ichStart);
            ichStart = ich + 1;
            // FIXME: Should we handle CON_PAGER_FLAG_DONT_OUTPUT ?

            /* Clear until the end of the screen */
            while (iLine < ScrollRows)
            {
                ConCallPagerLine(Pager, L"\n", 1);
                // CON_STREAM_WRITE(Pager->Screen->Stream, TEXT("\n"), 1);
                // if (!(Pager->dwFlags & CON_PAGER_FLAG_DONT_OUTPUT))
                ++iLine;
            }

            iColumn = 0;
            continue;
        }

        /* Other character - Handle double-width for CJK */

        if (IsCJK)
        {
            nWidthOfChar = GetWidthOfCharCJK(nCodePage, TextBuff[ich]);
            IsDoubleWidthCharTrailing = (nWidthOfChar == 2) &&
                                        ((iColumn + 1) % ScreenColumns == 0);
        }

        if ((iColumn + nWidthOfChar) % ScreenColumns == 0)
        {
            /* Output the pending text, including the last double-width character */
            ConCallPagerLine(Pager, &TextBuff[ichStart], ich - ichStart + 1);
            ichStart = ich + 1;
            if (!(Pager->dwFlags & CON_PAGER_FLAG_DONT_OUTPUT))
                ++iLine;
            iColumn += nWidthOfChar;
            continue;
        }

        if (IsDoubleWidthCharTrailing)
        {
            /* Output the pending text, excluding the last double-width character */
            ConCallPagerLine(Pager, &TextBuff[ichStart], ich - ichStart);
            ichStart = ich;
            if (!(Pager->dwFlags & CON_PAGER_FLAG_DONT_OUTPUT))
                CON_STREAM_WRITE(Pager->Screen->Stream, TEXT(" "), 1);
            --ich;
            if (!(Pager->dwFlags & CON_PAGER_FLAG_DONT_OUTPUT))
                ++iLine;
            ++iColumn;
            continue;
        }

        iColumn += nWidthOfChar;
    }

    /* Output the remaining text */
    if (iColumn % ScreenColumns > 0)
        ConCallPagerLine(Pager, &TextBuff[ichStart], ich - ichStart);

    if (iLine >= ScrollRows)
        iLine = 0; /* Reset the count of lines being printed */

    Pager->ich = ich;
    Pager->iColumn = iColumn;
    Pager->iLine = iLine;
    Pager->lineno = lineno;

    return (ich < cch);
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
    {
        /* Reset the output line count, the column index and the line number */
        Pager->iLine = 0;
        Pager->iColumn = 0;
        Pager->lineno = 1;
        Pager->nSpacePending = 0;
    }

    /* Get the size of the visual screen that can be printed to */
    if (!ConGetScreenInfo(Pager->Screen, &csbi))
    {
        /* We assume it's a file handle */
        ConCallPagerLine(Pager, szStr, len);
        return TRUE;
    }

    /* Fill the pager info */
    Pager->ScreenColumns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    Pager->ScreenRows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    Pager->ich = 0;
    Pager->cch = len;
    Pager->TextBuff = szStr;
    if (StartPaging)
    {
        /* Reset to display one page by default */
        Pager->ScrollRows = Pager->ScreenRows - 1;
    }

    if (len == 0 || szStr == NULL)
        return TRUE;

    /* Make sure the user doesn't have the screen too small */
    if (Pager->ScreenRows < 4)
    {
        ConCallPagerLine(Pager, szStr, len);
        return TRUE;
    }

    while (ConPagerWorker(Pager))
    {
        /* Recalculate the screen extent in case the user redimensions the window */
        if (ConGetScreenInfo(Pager->Screen, &csbi))
        {
            Pager->ScreenColumns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            Pager->ScreenRows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }

        /* Reset to display one page by default */
        Pager->ScrollRows = Pager->ScreenRows - 1;

        /* Prompt the user; give him some values for statistics */
        if (!PagePrompt(Pager, Pager->ich, Pager->cch))
            return FALSE;
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
