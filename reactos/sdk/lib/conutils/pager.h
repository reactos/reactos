/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Utilities Library
 * FILE:            sdk/lib/conutils/pager.h
 * PURPOSE:         Console/terminal paging functionality.
 * PROGRAMMERS:     - Hermes Belusca-Maito (for the library);
 *                  - All programmers who wrote the different console applications
 *                    from which I took those functions and improved them.
 */

#ifndef __PAGER_H__
#define __PAGER_H__

#ifndef _UNICODE
#error The ConUtils library at the moment only supports compilation with _UNICODE defined!
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
ConResPaging(
    IN PCON_PAGER Pager,
    IN PAGE_PROMPT PagePrompt,
    IN BOOL StartPaging,
    IN UINT uID);

#endif  /* __PAGER_H__ */
