#include "pch.h"
#include "..\viewer\viewerp.h"
#include "viewer.h"

#ifdef __cplusplus
extern "C" {
#endif

VOID WINAPI
CSCViewCacheRunDllA(
    HWND hwnd,
    HINSTANCE hInstance,
    LPSTR pszCmdLineA,
    INT nCmdShow);

VOID WINAPI
CSCViewCacheRunDllW(
    HWND hwnd,
    HINSTANCE hInstance,
    LPWSTR pszCmdLineW,
    INT nCmdShow);

#ifdef __cplusplus
}
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
//  Returns:  Nothing.
//            
VOID WINAPI
CSCViewCacheW(
    INT iInitialView,
    LPCWSTR pszInitialShareW
    )
{
    CSCViewCacheInternalNSE();
//    CSCViewCacheInternalW(iInitialView, pszInitialShareW, false);
}


VOID WINAPI
CSCViewCacheA(
    INT iInitialView,
    LPCSTR pszInitialShareA
    )
{
    CSCViewCacheInternalNSE();
//    CSCViewCacheInternalA(iInitialView, pszInitialShareA, false);
}

VOID WINAPI
CSCViewShareSummaryW(
    LPCWSTR pszShareW,
    HWND hwndParent,
    BOOL bModal
    )
{
    CSCViewShareSummaryInternalW(pszShareW, hwndParent, bModal);
}


VOID WINAPI
CSCViewShareSummaryA(
    LPCSTR pszShareA,
    HWND hwndParent,
    BOOL bModal
    )
{
    CSCViewShareSummaryInternalA(pszShareA, hwndParent, bModal);
}


VOID WINAPI
CSCViewOptions(
    HWND hwndParent,
    BOOL bModal
    )
{
    CSCViewOptionsInternal(hwndParent, bModal);
}

//
// See CSCViewCacheRunDllW for command line argument information.
//
VOID WINAPI
CSCViewCacheRunDllA(
    HWND hwnd,
    HINSTANCE hInstance,
    LPSTR pszCmdLineA,
    INT nCmdShow
    )
{
    USES_CONVERSION;
    CSCViewCacheRunDllW(hwnd, hInstance, A2W(pszCmdLineA), nCmdShow);
}


//
// To call via rundll32.exe:
//
// rundll32 cscui.dll,CSCViewCacheRunDll <n> <share>
//
// where: <n> is the view type identifier:
//
//              1 = Share view (default)
//              2 = Details view.
//              3 = Stale view.
//
//        <share> is the name of the initial share to display.
//                Default is to display all shares.
//
VOID WINAPI
CSCViewCacheRunDllW(
    HWND hwnd,
    HINSTANCE hInstance,
    LPWSTR pszCmdLineW,
    INT nCmdShow
    )
{
    WCHAR szBlank[] = L"";
    LPWSTR psz = pszCmdLineW;
    LPWSTR pszEnd = pszCmdLineW;
    int iView = 1;
    while(*pszEnd && L' ' != *pszEnd)
        pszEnd++;

    if (pszEnd > psz)
    {
        *pszEnd = 0;
        iView = StrToIntW(psz);
        pszEnd = psz = pszEnd + 1;
    }

    while(*pszEnd && L' ' != *pszEnd)
        pszEnd++;

    if (*pszEnd)
        *pszEnd = 0;

    if (!(*psz))
        psz = szBlank;

    CSCViewCacheInternalW(iView, psz, true);
}

