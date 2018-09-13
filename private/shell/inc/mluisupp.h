#ifndef _INC_MLUISUPP
#define _INC_MLUISUPP

#include <shlwapi.h>
#include <shlwapip.h>

#ifdef __cplusplus
extern "C"
{
#endif

//+------------------------------------------------------------------
// Multilang Pluggable UI support
// inline functions defs (to centralize code)
//+------------------------------------------------------------------

#ifdef UNICODE
#define MLLoadString            MLLoadStringW
#define MLLoadShellLangString   MLLoadShellLangStringW
#define MLBuildResURLWrap       MLBuildResURLWrapW
#define MLLoadResources         MLLoadResourcesW
#define SHHtmlHelpOnDemandWrap  SHHtmlHelpOnDemandWrapW
#define SHWinHelpOnDemandWrap   SHWinHelpOnDemandWrapW
#else
#define MLLoadString            MLLoadStringA
#define MLLoadShellLangString   MLLoadShellLangStringA
#define MLBuildResURLWrap       MLBuildResURLWrapA
#define MLLoadResources         MLLoadResourcesA
#define SHHtmlHelpOnDemandWrap  SHHtmlHelpOnDemandWrapA
#define SHWinHelpOnDemandWrap   SHWinHelpOnDemandWrapA
#endif

void        MLFreeResources(HINSTANCE hinstParent);
HINSTANCE   MLGetHinst();
HINSTANCE   MLLoadShellLangResources();

#ifdef MLUI_MESSAGEBOX
int         MLShellMessageBox(HWND hWnd, LPCTSTR pszMsg, LPCTSTR pszTitle, UINT fuStyle, ...);
#endif

//
// The following should be both A and W suffixed
//

int         MLLoadStringA(UINT id, LPSTR sz, UINT cchMax);
int         MLLoadStringW(UINT id, LPWSTR sz, UINT cchMax);

int         MLLoadShellLangStringA(UINT id, LPSTR sz, UINT cchMax);
int         MLLoadShellLangStringW(UINT id, LPWSTR sz, UINT cchMax);

HRESULT     MLBuildResURLWrapA(LPSTR    pszLibFile,
                               HMODULE  hModule,
                               DWORD    dwCrossCodePage,
                               LPSTR    pszResName,
                               LPSTR    pszResURL,
                               int      nBufSize,
                               LPSTR    pszParentDll);

HRESULT     MLBuildResURLWrapW(LPWSTR   pszLibFile,
                               HMODULE  hModule,
                               DWORD    dwCrossCodePage,
                               LPWSTR   pszResName,
                               LPWSTR   pszResURL,
                               int      nBufSize,
                               LPWSTR   pszParentDll);

void        MLLoadResourcesA(HINSTANCE hinstParent, LPSTR pszLocResDll);
void        MLLoadResourcesW(HINSTANCE hinstParent, LPWSTR pszLocResDll);

HWND        SHHtmlHelpOnDemandWrapA(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage);
HWND        SHHtmlHelpOnDemandWrapW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage);

BOOL        SHWinHelpOnDemandWrapA(HWND hwndCaller, LPCSTR lpszHelp, UINT uCommand, DWORD_PTR dwData);
BOOL        SHWinHelpOnDemandWrapW(HWND hwndCaller, LPCWSTR lpszHelp, UINT uCommand, DWORD_PTR dwData);

//
// End of: The following should be both A and W suffixed
//

#ifdef MLUI_INIT

// WARNING: do not attempt to access any of these members directly
// these members may not be initialized until appropriate accessors
// are called, for example hinstLocRes won't be intialized until
// you call MLGetHinst()... so just call the accessor.
struct tagMLUI_INFO
{
    HINSTANCE   hinstLocRes;
    HINSTANCE   hinstParent;
    WCHAR       szLocResDll[MAX_PATH];
    DWORD       dwCrossCodePage;
} g_mluiInfo;


// BUGBUG REVIEW: These aren't thread safe... Do they need to be?
//
void MLLoadResourcesA(HINSTANCE hinstParent, LPSTR pszLocResDll)
{
#ifdef RIP
    RIP(hinstParent != NULL);
    RIP(pszLocResDll != NULL);
#endif

    if (g_mluiInfo.hinstLocRes == NULL)
    {
#ifdef MLUI_SUPPORT
        // plugUI: resource dll == ?
        // resource dll must be dynamically determined and loaded.
        // but we are NOT allowed to LoadLibrary during process attach.
        // therefore we cache the info we need and load later when
        // the first resource is requested.
        SHAnsiToUnicode(pszLocResDll, g_mluiInfo.szLocResDll, sizeof(g_mluiInfo.szLocResDll)/sizeof(g_mluiInfo.szLocResDll[0]));
        g_mluiInfo.hinstParent = hinstParent;
        g_mluiInfo.dwCrossCodePage = ML_CROSSCODEPAGE;
#else
        // non-plugUI: resource dll == parent dll
        g_mluiInfo.hinstLocRes = hinstParent;
#endif
    }
}

void MLLoadResourcesW(HINSTANCE hinstParent, LPWSTR pszLocResDll)
{
#ifdef RIP
    RIP(hinstParent != NULL);
    RIP(pszLocResDll != NULL);
#endif

    if (g_mluiInfo.hinstLocRes == NULL)
    {
#ifdef MLUI_SUPPORT
        // plugUI: resource dll == ?
        // resource dll must be dynamically determined and loaded.
        // but we are NOT allowed to LoadLibrary during process attach.
        // therefore we cache the info we need and load later when
        // the first resource is requested.
        StrCpyNW(g_mluiInfo.szLocResDll, pszLocResDll, sizeof(g_mluiInfo.szLocResDll)/sizeof(g_mluiInfo.szLocResDll[0]));
        g_mluiInfo.hinstParent = hinstParent;
        g_mluiInfo.dwCrossCodePage = ML_CROSSCODEPAGE;
#else
        // non-plugUI: resource dll == parent dll
        g_mluiInfo.hinstLocRes = hinstParent;
#endif
    }
}

void
MLFreeResources(HINSTANCE hinstParent)
{
    if (g_mluiInfo.hinstLocRes != NULL &&
        g_mluiInfo.hinstLocRes != hinstParent)
    {
        MLClearMLHInstance(g_mluiInfo.hinstLocRes);
        g_mluiInfo.hinstLocRes = NULL;
    }
}

// this is a private internal helper.
// don't you dare call it from anywhere except at
// the beginning of new ML* functions in this file
__inline void
_MLResAssure()
{
#ifdef MLUI_SUPPORT
    if(g_mluiInfo.hinstLocRes == NULL)
    {
        g_mluiInfo.hinstLocRes = MLLoadLibraryW(g_mluiInfo.szLocResDll,
                                               g_mluiInfo.hinstParent,
                                               g_mluiInfo.dwCrossCodePage);

        // we're guaranteed to at least have resources in the install language
        ASSERT(g_mluiInfo.hinstLocRes != NULL);
    }
#endif
}

int
MLLoadStringA(UINT id, LPSTR sz, UINT cchMax)
{
    _MLResAssure();
    return LoadStringA(g_mluiInfo.hinstLocRes, id, sz, cchMax);
}

int
MLLoadStringW(UINT id, LPWSTR sz, UINT cchMax)
{
    _MLResAssure();
    return LoadStringWrapW(g_mluiInfo.hinstLocRes, id, sz, cchMax);
}

int
MLLoadShellLangStringA(UINT id, LPSTR sz, UINT cchMax)
{
    HINSTANCE   hinstShellLangRes;
    int         nRet;

    hinstShellLangRes = MLLoadShellLangResources();
    
    nRet = LoadStringA(hinstShellLangRes, id, sz, cchMax);

    MLFreeLibrary(hinstShellLangRes);

    return nRet;
}

int
MLLoadShellLangStringW(UINT id, LPWSTR sz, UINT cchMax)
{
    HINSTANCE   hinstShellLangRes;
    int         nRet;

    hinstShellLangRes = MLLoadShellLangResources();
    
    nRet = LoadStringWrapW(hinstShellLangRes, id, sz, cchMax);

    MLFreeLibrary(hinstShellLangRes);

    return nRet;
}

HINSTANCE
MLGetHinst()
{
    _MLResAssure();
    return g_mluiInfo.hinstLocRes;
}

HINSTANCE
MLLoadShellLangResources()
{
    HINSTANCE hinst;
    
    hinst = MLLoadLibraryW(g_mluiInfo.szLocResDll,
                           g_mluiInfo.hinstParent,
                           ML_SHELL_LANGUAGE);

    // we're guaranteed to at least have resources in the install language
    // unless we're 100% toasted
    ASSERT(hinst != NULL);

    return hinst;
}

#ifdef MLUI_MESSAGEBOX
int MLShellMessageBox(HWND hWnd, LPCTSTR pszMsg, LPCTSTR pszTitle, UINT fuStyle, ...)
{
    va_list     vaList;
    int         nRet            = 0;
    LPTSTR      pszFormattedMsg = NULL;
    TCHAR       szTitleBuf[256];
    TCHAR       szBuffer[1024];

    //
    // prepare the message
    //

    if (IS_INTRESOURCE(pszMsg))
    {
        if (MLLoadShellLangString(LOWORD((DWORD_PTR)pszMsg), szBuffer, ARRAYSIZE(szBuffer)))
        {
            pszMsg = szBuffer;
        }
    }

    if (!IS_INTRESOURCE(pszMsg) &&  // the string load might have failed
        pszMsg != NULL)
    {
        va_start(vaList, fuStyle);

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                          pszMsg, 0, 0, (LPTSTR)&pszFormattedMsg, 0, &vaList))
        {
            pszMsg = pszFormattedMsg;
        }

        va_end(vaList);
    }

    //
    // prepare the title
    //

    if (!IS_INTRESOURCE(pszTitle) && pszTitle != NULL)
    {
        // do nothing
    }
    else if (pszTitle != NULL && MLLoadShellLangString(LOWORD((DWORD_PTR)pszTitle), szTitleBuf, ARRAYSIZE(szTitleBuf)))
    {
        pszTitle = szTitleBuf;
    }
    else if (hWnd && GetWindowText(hWnd, szTitleBuf, ARRAYSIZE(szTitleBuf)))
    {
        pszTitle = szTitleBuf;
    }
    else
    {
        pszTitle = TEXT("");
    }

    //
    // launch a MessageBox
    //

    nRet = MessageBox(hWnd, pszFormattedMsg, pszTitle, fuStyle | MB_SETFOREGROUND);

    if (pszFormattedMsg != NULL)
    {
        LocalFree(pszFormattedMsg);
    }

    return nRet;
}
#endif // MLUI_MESSAGEBOX

#include "htmlhelp.h"

HWND 
SHHtmlHelpOnDemandWrapA(HWND hwndCaller, 
                       LPCSTR pszFile,
                       UINT uCommand,
                       DWORD_PTR dwData,
                       DWORD dwCrossCodePage)
{
    BOOL    fEnabled;

#ifdef MLUI_SUPPORT
    fEnabled = TRUE;
#else
    fEnabled = FALSE;
#endif

    return SHHtmlHelpOnDemandA(hwndCaller,
                              pszFile,
                              uCommand,
                              dwData,
                              dwCrossCodePage,
                              fEnabled);
}

HWND 
SHHtmlHelpOnDemandWrapW(HWND hwndCaller, 
                       LPCWSTR pszFile,
                       UINT uCommand,
                       DWORD_PTR dwData,
                       DWORD dwCrossCodePage)
{
    BOOL    fEnabled;

#ifdef MLUI_SUPPORT
    fEnabled = TRUE;
#else
    fEnabled = FALSE;
#endif

    return SHHtmlHelpOnDemandW(hwndCaller,
                              pszFile,
                              uCommand,
                              dwData,
                              dwCrossCodePage,
                              fEnabled);
}

HWND
MLHtmlHelpWrap(HWND hwndCaller,
               LPCTSTR pszFile,
               UINT uCommand,
               DWORD dwData,
               DWORD dwCrossCodePage)
{
    HWND    hwnd;

#ifdef MLUI_SUPPORT
    hwnd = MLHtmlHelp(hwndCaller,
                      pszFile,
                      uCommand,
                      dwData,
                      dwCrossCodePage);
#else
    hwnd = HtmlHelp(hwndCaller,
                    pszFile,
                    uCommand,
                    dwData);
#endif

    return hwnd;
}

BOOL
SHWinHelpOnDemandWrapA(HWND hwndCaller,
                      LPCSTR lpszHelp,
                      UINT uCommand,
                      DWORD_PTR dwData)
{
    BOOL    fEnabled;

#ifdef MLUI_SUPPORT
    fEnabled = TRUE;
#else
    fEnabled = FALSE;
#endif

    return SHWinHelpOnDemandA(hwndCaller,
                             lpszHelp,
                             uCommand,
                             dwData,
                             fEnabled);

}

BOOL
SHWinHelpOnDemandWrapW(HWND hwndCaller,
                      LPCWSTR lpszHelp,
                      UINT uCommand,
                      DWORD_PTR dwData)
{
    BOOL    fEnabled;

#ifdef MLUI_SUPPORT
    fEnabled = TRUE;
#else
    fEnabled = FALSE;
#endif

    return SHWinHelpOnDemandW(hwndCaller,
                             lpszHelp,
                             uCommand,
                             dwData,
                             fEnabled);

}

BOOL
MLWinHelpWrap(HWND hwndCaller,
                   LPCTSTR lpszHelp,
                   UINT uCommand,
                   DWORD dwData)
{
    BOOL    fRet;

#ifdef MLUI_SUPPORT
    fRet = MLWinHelp(hwndCaller,
                     lpszHelp,
                     uCommand,
                     dwData);
#else
    fRet = WinHelp(hwndCaller,
                   lpszHelp,
                   uCommand,
                   dwData);
#endif

    return fRet;
}

HRESULT
MLBuildResURLWrapA(LPSTR    pszLibFile,
                   HMODULE  hModule,
                   DWORD    dwCrossCodePage,
                   LPSTR    pszResName,
                   LPSTR    pszResURL,
                   int      nBufSize,
                   LPSTR    pszParentDll)
{
    HRESULT hr;

#ifdef MLUI_SUPPORT
    hr = MLBuildResURLA(pszLibFile,
                        hModule,
                        dwCrossCodePage,
                        pszResName,
                        pszResURL,
                        nBufSize);
#else
    wnsprintfA(pszResURL, nBufSize, "res://%s/%s", pszParentDll, pszResName);
    hr = S_OK;
#endif

    return hr;
}

HRESULT
MLBuildResURLWrapW(LPWSTR   pszLibFile,
                   HMODULE  hModule,
                   DWORD    dwCrossCodePage,
                   LPWSTR   pszResName,
                   LPWSTR   pszResURL,
                   int      nBufSize,
                   LPWSTR   pszParentDll)
{
    HRESULT hr;

#ifdef MLUI_SUPPORT
    hr = MLBuildResURLW(pszLibFile,
                        hModule,
                        dwCrossCodePage,
                        pszResName,
                        pszResURL,
                        nBufSize);
#else
    wnsprintfW(pszResURL, nBufSize, L"res://%s/%s", pszParentDll, pszResName);
    hr = S_OK;
#endif

    return hr;
}

#endif  // MLUI_INIT

#ifdef __cplusplus
};
#endif

#endif  // _INC_MLUISUPP
