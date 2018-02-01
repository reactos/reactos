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


typedef struct _CON_PAGER
{
    PCON_SCREEN Screen;

    // TODO: Add more properties. Maybe those extra parameters
    // of PAGE_PROMPT could go there?

    /* Used to count number of lines since last pause */
    DWORD LineCount;
} CON_PAGER, *PCON_PAGER;

#define INIT_CON_PAGER(pScreen)     {(pScreen), 0}

#define InitializeConPager(pPager, pScreen) \
do { \
    (pPager)->Screen = (pScreen);   \
    (pPager)->LineCount = 0;        \
} while (0)


                                      // Pager,         Done,     Total
typedef BOOL (__stdcall *PAGE_PROMPT)(IN PCON_PAGER, IN DWORD, IN DWORD);

BOOL
ConWritePaging(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN PTCHAR szStr,
    IN DWORD len);

BOOL
ConPutsPaging(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN LPTSTR szStr);

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
