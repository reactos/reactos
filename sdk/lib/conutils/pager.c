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
    Pager->dwFlags &= ~CON_PAGER_DONT_OUTPUT; /* Clear the flag */

    if (!Pager->PagerLine || !Pager->PagerLine(Pager, line, cch))
        CON_STREAM_WRITE(Pager->Screen->Stream, line, cch);
}

static BOOL
ConPagerWorker(IN PCON_PAGER Pager)
{
    const DWORD PageColumns = Pager->PageColumns;
    const DWORD ScrollRows = Pager->ScrollRows;
    const PCTCH TextBuff = Pager->TextBuff;
    const DWORD cch = Pager->cch;

    BOOL bFinitePaging = ((PageColumns > 0) && (Pager->PageRows > 0));
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
    if (PageColumns > 0) // if (bFinitePaging)
    {
        if (nTabWidth < 0)
            nTabWidth = PageColumns - 1;
        else
            nTabWidth = min(nTabWidth, PageColumns - 1);
    }
    else
    {
        /* If no column width is known, default to 8 spaces if the
         * original value is negative; otherwise keep the current one. */
        if (nTabWidth < 0)
            nTabWidth = 8;
    }

    if (Pager->dwFlags & CON_PAGER_EXPAND_TABS)
    {
ExpandTab:
        while (Pager->nSpacePending > 0)
        {
            /* Stop now if we have displayed more screen lines than requested */
            if (bFinitePaging && (iLine >= ScrollRows))
                break;

            ConCallPagerLine(Pager, L" ", 1);
            --(Pager->nSpacePending);
            ++iColumn;
            if ((PageColumns > 0) && (iColumn % PageColumns == 0))
            {
                if (!(Pager->dwFlags & CON_PAGER_DONT_OUTPUT))
                    ++iLine;
            }
        }
    }

    /* Loop over each character in the buffer */
    for (; ich < cch; ++ich)
    {
        /* Stop now if we have displayed more screen lines than requested */
        if (bFinitePaging && (iLine >= ScrollRows))
            break;

        Pager->lineno = lineno;

        /* NEWLINE character */
        if (TextBuff[ich] == TEXT('\n'))
        {
            /* Output the pending text, including the newline */
            ConCallPagerLine(Pager, &TextBuff[ichStart], ich - ichStart + 1);
            ichStart = ich + 1;
            if (!(Pager->dwFlags & CON_PAGER_DONT_OUTPUT))
                ++iLine;
            iColumn = 0;

            /* Done with this line; start a new one */
            ++lineno;
            continue;
        }

        /* TAB character */
        if (TextBuff[ich] == TEXT('\t') &&
            (Pager->dwFlags & CON_PAGER_EXPAND_TABS))
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
            (Pager->dwFlags & CON_PAGER_EXPAND_FF))
        {
            /* Output the pending text, skipping the form-feed */
            ConCallPagerLine(Pager, &TextBuff[ichStart], ich - ichStart);
            ichStart = ich + 1;
            // FIXME: Should we handle CON_PAGER_DONT_OUTPUT ?

            if (bFinitePaging)
            {
                /* Clear until the end of the screen */
                while (iLine < ScrollRows)
                {
                    ConCallPagerLine(Pager, L"\n", 1);
                    // CON_STREAM_WRITE(Pager->Screen->Stream, TEXT("\n"), 1);
                    // if (!(Pager->dwFlags & CON_PAGER_DONT_OUTPUT))
                    ++iLine;
                }
            }
            else
            {
                /* Just output a FORM-FEED character */
                ConCallPagerLine(Pager, L"\f", 1);
                // CON_STREAM_WRITE(Pager->Screen->Stream, L"\f", 1);
            }

            iColumn = 0;
            continue;
        }

        /* Other character - Handle double-width for CJK */

        if (IsCJK)
        {
            nWidthOfChar = GetWidthOfCharCJK(nCodePage, TextBuff[ich]);
            if (PageColumns > 0)
            {
                IsDoubleWidthCharTrailing = (nWidthOfChar == 2) &&
                                            ((iColumn + 1) % PageColumns == 0);
            }
        }

        /* Care about CJK character presentation only when outputting
         * to a device where the number of columns is known. */
        if (PageColumns > 0)
        {
            if ((iColumn + nWidthOfChar) % PageColumns == 0)
            {
                /* Output the pending text, including the last double-width character */
                ConCallPagerLine(Pager, &TextBuff[ichStart], ich - ichStart + 1);
                ichStart = ich + 1;
                if (!(Pager->dwFlags & CON_PAGER_DONT_OUTPUT))
                    ++iLine;
                iColumn += nWidthOfChar;
                continue;
            }

            if (IsDoubleWidthCharTrailing)
            {
                /* Output the pending text, excluding the last double-width character */
                ConCallPagerLine(Pager, &TextBuff[ichStart], ich - ichStart);
                ichStart = ich;
                if (!(Pager->dwFlags & CON_PAGER_DONT_OUTPUT))
                    CON_STREAM_WRITE(Pager->Screen->Stream, TEXT(" "), 1);
                --ich;
                if (!(Pager->dwFlags & CON_PAGER_DONT_OUTPUT))
                    ++iLine;
                ++iColumn;
                continue;
            }
        }

        iColumn += nWidthOfChar;
    }

    /* Output the remaining text */
    if (ich - ichStart > 0)
        ConCallPagerLine(Pager, &TextBuff[ichStart], ich - ichStart);

    if (iLine >= ScrollRows)
        iLine = 0; /* Reset the count of lines being printed */

    Pager->ich = ich;
    Pager->iColumn = iColumn;
    Pager->iLine = iLine;
    Pager->lineno = lineno;

    return (ich < cch);
}


/**
 * @name ConWritePaging
 *     Pages the contents of a user-specified character buffer on the screen.
 *
 * @param[in]   Pager
 *     Pager object that describes where the paged output is issued.
 *
 * @param[in]   PagePrompt
 *     A user-specific callback, called when a page has been displayed.
 *
 * @param[in]   StartPaging
 *     Set to TRUE for initializing the paging operation; FALSE during paging.
 *
 * @param[in]   szStr
 *     Pointer to the character buffer whose contents are to be paged.
 *
 * @param[in]   len
 *     Length of the character buffer pointed by @p szStr, specified
 *     in number of characters.
 *
 * @return
 *     TRUE when all the contents of the character buffer has been displayed;
 *     FALSE if the paging operation has been stopped (controlled via @p PagePrompt).
 **/
BOOL
ConWritePaging(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN PCTCH szStr,
    IN DWORD len)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    BOOL bIsConsole;

    /* Parameters validation */
    if (!Pager)
        return FALSE;

    /* Get the size of the visual screen that can be printed to */
    bIsConsole = ConGetScreenInfo(Pager->Screen, &csbi);
    if (bIsConsole)
    {
        /* Calculate the console screen extent */
        Pager->PageColumns = csbi.dwSize.X;
        Pager->PageRows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
    else
    {
        /* We assume it's a file handle */
        Pager->PageColumns = 0;
        Pager->PageRows = 0;
    }

    if (StartPaging)
    {
        if (bIsConsole && (Pager->PageRows >= 2))
        {
            /* Reset to display one page by default */
            Pager->ScrollRows = Pager->PageRows - 1;
        }
        else
        {
            /* File output, or single line: all lines are displayed at once; reset to a default value */
            Pager->ScrollRows = 0;
        }
    }

    if (StartPaging)
    {
        /* Reset the paging state */
        Pager->nSpacePending = 0;
        Pager->iColumn = 0;
        Pager->iLine = 0;
        Pager->lineno = 1;
    }

    Pager->TextBuff = szStr;
    Pager->cch = len;
    Pager->ich = 0;

    if (len == 0 || szStr == NULL)
        return TRUE;

    while (ConPagerWorker(Pager))
    {
        /* Prompt the user only when we display to a console and the screen
         * is not too small: at least one line for the actual paged text and
         * one line for the prompt. */
        if (bIsConsole && (Pager->PageRows >= 2))
        {
            /* Reset to display one page by default */
            Pager->ScrollRows = Pager->PageRows - 1;

            /* Prompt the user; give him some values for statistics */
            if (!PagePrompt(Pager, Pager->ich, Pager->cch))
                return FALSE;
        }

        /* If we display to a console, recalculate its screen extent
         * in case the user has redimensioned it during the prompt. */
        if (bIsConsole && ConGetScreenInfo(Pager->Screen, &csbi))
        {
            Pager->PageColumns = csbi.dwSize.X;
            Pager->PageRows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
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
