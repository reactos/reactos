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

static inline INT GetWidthOfCharCJK(UINT nCodePage, WCHAR ch)
{
    INT ret = WideCharToMultiByte(nCodePage, 0, &ch, 1, NULL, 0, NULL, NULL);
    if (ret == 0)
        ret = 1;
    else if (ret > 2)
        ret = 2;
    return ret;
}

static BOOL CALLBACK
ConDefaultPagerLine(PCON_PAGER Pager, LPCWSTR line, DWORD cch, DWORD *pdwFlags)
{
    CON_STREAM_WRITE(Pager->Screen->Stream, line, cch);
    return TRUE;
}

static VOID CALLBACK
ConCallPagerLine(PCON_PAGER Pager, LPCWSTR line, DWORD cch, DWORD *pdwFlags)
{
    if (!Pager->PagerLine || !(*Pager->PagerLine)(Pager, line, cch, pdwFlags))
        (*Pager->DefPagerLine)(Pager, line, cch, pdwFlags);
}

static BOOL CALLBACK ConPagerDefaultAction(PCON_PAGER Pager)
{
    PCTCH TextBuff = Pager->TextBuff;
    DWORD ich = Pager->ich;
    DWORD cch = Pager->cch;
    DWORD iColumn = Pager->iColumn;
    DWORD iLine = Pager->iLine;
    DWORD lineno = Pager->lineno;
    DWORD ScreenColumns = Pager->ScreenColumns;
    DWORD ScrollRows = Pager->ScrollRows;
    DWORD ichLast = ich;
    UINT nWidthOfChar = 1;
    UINT nCodePage;
    BOOL IsCJK = FALSE;
    BOOL IsDoubleWidthCharTrailing = FALSE;
    DWORD dwFlags = 0;

    if (ich >= cch)
        return TRUE;

    nCodePage = GetConsoleOutputCP();
    IsCJK = IsCJKCodePage(nCodePage);

    for (; ich < cch && iLine < ScrollRows; ++ich)
    {
        Pager->lineno = lineno;
        if (IsCJK)
        {
            nWidthOfChar = GetWidthOfCharCJK(nCodePage, TextBuff[ich]);
            IsDoubleWidthCharTrailing = (nWidthOfChar == 2) &&
                                        (iColumn + 1 == ScreenColumns);
        }
        if (TextBuff[ich] == TEXT('\n') || iColumn + nWidthOfChar >= ScreenColumns)
        {
            dwFlags = CON_PAGER_LINE_FLAG_NEWLINE;
            ConCallPagerLine(Pager, &TextBuff[ichLast],
                             ich - ichLast + !IsDoubleWidthCharTrailing, &dwFlags);
            ichLast = ich + !IsDoubleWidthCharTrailing;
            if (IsDoubleWidthCharTrailing)
            {
                CON_STREAM_WRITE(Pager->Screen->Stream, L" ", 1);
                --ich;
            }
            if (dwFlags & CON_PAGER_LINE_FLAG_NEWLINE)
                ++iLine;
            if (TextBuff[ich] == TEXT('\n'))
                ++lineno;
            iColumn = 0;
            continue;
        }
        iColumn += nWidthOfChar;
    }

    if (iColumn > 0)
    {
        dwFlags &= ~CON_PAGER_LINE_FLAG_NEWLINE;
        ConCallPagerLine(Pager, &TextBuff[ichLast], ich - ichLast, &dwFlags);
    }

    if (iLine >= ScrollRows)
    {
        iLine = 0; /* Reset the count of lines being printed */
        iColumn = 0; /* Reset the index of column */
    }

    Pager->ich = ich;
    Pager->iColumn = iColumn;
    Pager->iLine = iLine;
    Pager->lineno = lineno;

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
    {
        Pager->iLine = 0; /* Reset the output line count */
        Pager->iColumn = 0; /* Reset the column index */
        Pager->lineno = 1; /* Reset the line number */
    }

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
    Pager->PagerAction = ConPagerDefaultAction;
    Pager->DefPagerLine = ConDefaultPagerLine;
    Pager->ich = 0;
    Pager->cch = len;
    Pager->TextBuff = szStr;
    if (StartPaging)
        Pager->ScrollRows = Pager->ScreenRows - 1;

    if (len == 0 || szStr == NULL)
        return TRUE;

    /* Make sure the user doesn't have the screen too small */
    if (Pager->ScreenRows < 4)
    {
        CON_STREAM_WRITE(Pager->Screen->Stream, szStr, len);
        return TRUE;
    }

    while (!(*Pager->PagerAction)(Pager))
    {
        /* Recalculate the screen extent in case the user redimensions the window. */
        if (ConGetScreenInfo(Pager->Screen, &csbi))
        {
            Pager->ScreenColumns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            Pager->ScreenRows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }

        /* PagePrompt might change this value */
        Pager->PagerAction = ConPagerDefaultAction;

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
