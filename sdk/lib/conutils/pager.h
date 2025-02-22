/*
 * PROJECT:     ReactOS Console Utilities Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Console/terminal paging functionality.
 * COPYRIGHT:   Copyright 2017-2021 Hermes Belusca-Maito
 *              Copyright 2021 Katayama Hirofumi MZ
 */

/**
 * @file    pager.h
 * @ingroup ConUtils
 *
 * @brief   Console/terminal paging functionality.
 **/

#ifndef __PAGER_H__
#define __PAGER_H__

#pragma once

#ifndef _UNICODE
#error The ConUtils library at the moment only supports compilation with _UNICODE defined!
#endif

#ifdef __cplusplus
extern "C" {
#endif

// #include <wincon.h>

struct _CON_PAGER;

typedef BOOL
(__stdcall *CON_PAGER_LINE_FN)(
    IN OUT struct _CON_PAGER *Pager,
    IN PCTCH line,
    IN DWORD cch);

/* Flags for CON_PAGER */
#define CON_PAGER_EXPAND_TABS   (1 << 0)
#define CON_PAGER_EXPAND_FF     (1 << 1)
// Whether or not the pager will cache the line if it's incomplete (not NEWLINE-terminated).
#define CON_PAGER_CACHE_INCOMPLETE_LINE (1 << 2)

typedef struct _CON_PAGER
{
    /* Console screen properties */
    PCON_SCREEN Screen;
    DWORD PageColumns;
    DWORD PageRows;

    /* Paging parameters */
    CON_PAGER_LINE_FN PagerLine; /* The line function */
    DWORD dwFlags;  /* The CON_PAGER_... flags */
    LONG  nTabWidth;
    DWORD ScrollRows;

    /* Data buffer */
    PCTCH  CachedLine;    /* Cached line, HeapAlloc'ated */
    SIZE_T cchCachedLine; /* Its length (number of characters) */
    SIZE_T ich;           /* The current index of character in TextBuff (a user-provided source buffer) */

    /* Paging state */
    PCTCH  CurrentLine;   /* Pointer to the current line (either within a user-provided source buffer, or to CachedLine) */
    SIZE_T ichCurr;       /* The current index of character in CurrentLine */
    SIZE_T iEndLine;      /* End (length) of CurrentLine */
    DWORD  nSpacePending; /* Pending spaces for TAB expansion */
    DWORD iColumn;  /* The current index of column */
    DWORD iLine;    /* The physical output line count of screen */
    DWORD lineno;   /* The logical line number */
} CON_PAGER, *PCON_PAGER;

#define INIT_CON_PAGER(pScreen)     {(pScreen), 0}

#define InitializeConPager(pPager, pScreen)  \
do { \
    ZeroMemory((pPager), sizeof(*(pPager))); \
    (pPager)->Screen = (pScreen);            \
} while (0)


typedef BOOL
(__stdcall *PAGE_PROMPT)(
    IN PCON_PAGER Pager,
    IN DWORD Done,
    IN DWORD Total);

BOOL
ConWritePaging(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN PCTCH szStr,
    IN DWORD len);

BOOL
ConPutsPaging(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN PCTSTR szStr);

BOOL
ConResPagingEx(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT uID);

BOOL
ConResPaging(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN UINT uID);

#ifdef __cplusplus
}
#endif

#endif  /* __PAGER_H__ */

/* EOF */
