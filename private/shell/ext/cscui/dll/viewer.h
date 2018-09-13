#ifndef __CSC_CACHE_VIEWER_H__
#define __CSC_CACHE_VIEWER_H__


#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifdef UNICODE
#    define CSCViewCache        CSCViewCacheW
#    define CSCViewShareSummary CSCViewShareSummaryW
#else
#    define CSCViewCache        CSCViewCacheA
#    define CSCViewShareSummary CSCViewShareSummaryA
#endif


#ifdef __cplusplus
extern "C" {
#endif
//
// Arguments:
//
//      iInitialView - Indicates initial view of cache to be displayed.
//
//              1 = Share view (default)
//              2 = Details view.
//              3 = Stale view.
//
//      pszInitialShare - Name of initial share to view.
//
//              i.e.: "\\worf\ntspecs"
//
//              "" means All Shares.
//
//  Returns:  0 = View successfully created.
//            
VOID WINAPI
CSCViewCacheW(
    INT iInitialView,
    LPCWSTR pszInitialShareW);

VOID WINAPI
CSCViewCacheA(
    INT iInitialView,
    LPCSTR pszInitialShareA);

VOID WINAPI
CSCViewShareSummaryW(
    LPCWSTR pszShareW,
    HWND hwndParent,
    BOOL bModal);

VOID WINAPI
CSCViewShareSummaryA(
    LPCSTR pszShareA,
    HWND hwndParent,
    BOOL bModal);

VOID WINAPI
CSCViewOptions(
    HWND hwndParent,
    BOOL bModal);

#ifdef __cplusplus
}
#endif

#endif // __CSC_CACHE_VIEWER_H__
