/*
 * PROJECT:     ReactOS Console Utilities Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Console/terminal paging functionality.
 * COPYRIGHT:   Copyright 2017-2018 ReactOS Team
 *              Copyright 2017-2018 Hermes Belusca-Maito
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
typedef BOOL (CALLBACK *CON_PAGER_ACTION_FN)(struct _CON_PAGER *Pager);
typedef BOOL (CALLBACK *CON_PAGER_OUTPUT_LINE_FN)(
    struct _CON_PAGER *Pager, LPCWSTR line, DWORD cch, DWORD lineno);

typedef struct _CON_PAGER
{
    PCON_SCREEN Screen;
    DWORD ScreenColumns;
    DWORD ScreenRows;
    DWORD ScrollRows;
    CON_PAGER_ACTION_FN PagerAction;
    CON_PAGER_OUTPUT_LINE_FN OutputLine;
    PCTCH TextBuff; /* the text buffer */
    DWORD ich; /* current index of character */
    DWORD cch; /* the total number of characters */
    DWORD iColumn; /* current index of column */
    DWORD iLine; /* current index of line */
} CON_PAGER, *PCON_PAGER;

#define INIT_CON_PAGER(pScreen)     {(pScreen), 0}

#define InitializeConPager(pPager, pScreen) \
do { \
    (pPager)->Screen = (pScreen);   \
    (pPager)->LineCount = 0;        \
} while (0)


typedef BOOL (__stdcall *PAGE_PROMPT)(
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
