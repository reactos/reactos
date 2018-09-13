#ifndef _INC_CSCVIEW_VIEWERP_H
#define _INC_CSCVIEW_VIEWERP_H

#ifndef _WINDOWS_
#   include <windows.h>
#endif


void SetDebugParams(void);
void LoadModuleHandle(void);


#ifdef UNICODE
#   define CSCViewCacheInternal        CSCViewCacheInternalW
#   define CSCViewShareSummaryInternal CSCViewShareSummaryInternalW
#else
#   define CSCViewCacheInternal        CSCViewCacheInternalA
#   define CSCViewShareSummaryInternal CSCViewShareSummaryInternalA
#endif

INT
CSCViewCacheInternalW(
    INT iInitialView,
    LPCWSTR pszInitialShareW,
    bool bWait);

INT
CSCViewCacheInternalA(
    INT iInitialView,
    LPCSTR pszInitialShareA,
    bool bWait);

INT CSCViewCacheInternalNSE(
    void);

VOID
CSCViewShareSummaryInternalW(
    LPCWSTR pszShareW,
    HWND hwndParent,
    BOOL bModal);

VOID
CSCViewShareSummaryInternalA(
    LPCSTR pszShareA,
    HWND hwndParent,
    BOOL bModal);

VOID
CSCViewOptionsInternal(
    HWND hwndParent,
    BOOL bModal);


#endif // _INC_CSCVIEW_VIEWERP_H

