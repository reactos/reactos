//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       Shell32.cxx
//
//  Contents:   Dynamic wrappers for shell procedures.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#define _SHELL32_
#define _SHDOCVW_
#include <shellapi.h>
#endif

#ifndef X_SHFOLDER_HXX_
#define X_SHFOLDER_HXX_
#define _SHFOLDER_
#include "shfolder.h"
#endif

#ifndef X_CDERR_H_
#define X_CDERR_H_
#include <cderr.h>
#endif

int UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch = -1);

DYNLIB g_dynlibSHELL32 = { NULL, NULL, "SHELL32.DLL" };
DYNLIB g_dynlibSHFOLDER = { NULL, NULL, "SHFOLDER.DLL" };
extern DYNLIB g_dynlibSHDOCVW;

HINSTANCE APIENTRY
ShellExecuteA(HWND hwnd,	LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameter, LPCSTR lpDirectory, INT nShowCmd)
{
    static DYNPROC s_dynprocShellExecuteA =
            { NULL, &g_dynlibSHELL32, "ShellExecuteA" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocShellExecuteA);
    if (hr) 
        return (HINSTANCE)NULL;

    return (*(HINSTANCE (APIENTRY *)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT))s_dynprocShellExecuteA.pfn)
            (hwnd, lpOperation, lpFile, lpParameter, lpDirectory, nShowCmd);
}

HINSTANCE APIENTRY
ShellExecuteW(HWND hwnd,	LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameter, LPCWSTR lpDirectory, INT nShowCmd)
{
    CStrIn strInOperation(lpOperation);
    CStrIn strInFile(lpFile);
    CStrIn strInParameter(lpParameter);
    CStrIn strInDirectory(lpDirectory);

    return ShellExecuteA(hwnd, strInOperation, strInFile, strInParameter, strInDirectory, nShowCmd);
}


DWORD_PTR WINAPI 
SHGetFileInfoA(LPCSTR pszPath, DWORD dwFileAttributes, SHFILEINFOA FAR *psfi, UINT cbFileInfo, UINT uFlags)
{
    static DYNPROC s_dynprocSHGetFileInfoA =
            { NULL, &g_dynlibSHELL32, "SHGetFileInfoA" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocSHGetFileInfoA);
    if (hr) 
        return 0;

    return (*(DWORD_PTR (APIENTRY *)(LPCSTR, DWORD, SHFILEINFOA *, UINT, UINT))s_dynprocSHGetFileInfoA.pfn)
            (pszPath, dwFileAttributes, psfi, cbFileInfo, uFlags);
}

DWORD_PTR WINAPI 
SHGetFileInfoW(LPCWSTR pszPath, DWORD dwFileAttributes, SHFILEINFOW FAR *psfi, UINT cbFileInfo, UINT uFlags)
{
    Assert(cbFileInfo == sizeof(SHFILEINFOW));
    Assert(psfi);

    if (!g_fUnicodePlatform)
    {
        CStrIn      strPath(pszPath);
        SHFILEINFOA sfi;
        DWORD_PTR   dw;

        memset(&sfi, 0, sizeof(sfi));
        dw = SHGetFileInfoA(strPath, dwFileAttributes, &sfi, sizeof(sfi), uFlags);

        psfi->hIcon = sfi.hIcon;
        psfi->iIcon = sfi.iIcon;
        psfi->dwAttributes = sfi.dwAttributes;
        UnicodeFromMbcs(psfi->szDisplayName, ARRAY_SIZE(psfi->szDisplayName), sfi.szDisplayName);
        UnicodeFromMbcs(psfi->szTypeName, ARRAY_SIZE(psfi->szTypeName), sfi.szTypeName);

        return dw;
    }
    else
    {
        static DYNPROC s_dynprocSHGetFileInfoW =
                { NULL, &g_dynlibSHELL32, "SHGetFileInfoW" };

        HRESULT hr;

        hr = LoadProcedure(&s_dynprocSHGetFileInfoW);
        if (hr) 
            return 0;

        return (*(DWORD_PTR (APIENTRY *)(LPCWSTR, DWORD, SHFILEINFOW *, UINT, UINT))s_dynprocSHGetFileInfoW.pfn)
                (pszPath, dwFileAttributes, psfi, cbFileInfo, uFlags);
    }
}

#if !defined(_M_IX86) || defined(WINCE)

UINT WINAPI ExtractIconExW(LPCWSTR lpszFile, int nIconIndex, HICON FAR *phiconLarge, 
	HICON FAR *phiconSmall, UINT nIcons)
{
    static DYNPROC s_dynprocExtractIconExW =
            { NULL, &g_dynlibSHELL32, "ExtractIconExW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocExtractIconExW);
    if (hr) 
        return 0;

    return (*(UINT (APIENTRY *)(LPCWSTR, int, HICON FAR *, HICON FAR *, UINT))s_dynprocExtractIconExW.pfn)
            (lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}

HICON WINAPI ExtractIconW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex)
{
    static DYNPROC s_dynprocExtractIconW =
            { NULL, &g_dynlibSHELL32, "ExtractIconW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocExtractIconW);
    if (hr) 
        return 0;

    return (*(HICON (APIENTRY *)(HINSTANCE, LPCWSTR, UINT))s_dynprocExtractIconW.pfn)
            (hInst, lpszExeFileName, nIconIndex);  
}

#endif

//+---------------------------------------------------------------------------
//
//  Function:   DoFileDownLoad
//
//  Synopsis:   calls the DoFileDownload proc in SHDOCVW.DLL
//
//  Arguments:  [pchHref]   ref to file to download
//
//
//----------------------------------------------------------------------------
HRESULT
DoFileDownLoad(const TCHAR *pchHref)
{
    static DYNPROC s_dynprocFileDownload =
            { NULL, &g_dynlibSHDOCVW, "DoFileDownload" };
    HRESULT hr = LoadProcedure(&s_dynprocFileDownload);

    Assert(pchHref);

    if (!hr)
    {
            hr = (*(HRESULT (APIENTRY*)(LPCWSTR))s_dynprocFileDownload.pfn)(pchHref); 
    }
    else
    {
            hr = E_FAIL;
    }

    return hr;
}

#ifdef NO_MARSHALLING

SHDOCAPI APIENTRY CoCreateInternetExplorer( REFIID iid, DWORD dwClsContext, void **ppvunk );

HRESULT APIENTRY
CoCreateInternetExplorer( REFIID iid, DWORD dwClsContext, void **ppvunk )
{
    static DYNPROC s_dynprocCoCreateInternetExplorer =
            { NULL, &g_dynlibSHDOCVW, "CoCreateInternetExplorer" };
    HRESULT hr = LoadProcedure(&s_dynprocCoCreateInternetExplorer);

    if (!hr)
    {
            hr = (*(HRESULT (APIENTRY*)(REFIID, DWORD, void**))s_dynprocCoCreateInternetExplorer.pfn)(iid, dwClsContext, ppvunk); 
    }
    else
    {
            hr = E_FAIL;
    }

    return hr;
}

#endif


HICON APIENTRY
ExtractAssociatedIconA(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIcon)
{
    static DYNPROC s_dynprocExtractAssociatedIconA =
            { NULL, &g_dynlibSHELL32, "ExtractAssociatedIconA" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocExtractAssociatedIconA);
    if (hr) 
        return (HICON) NULL;

    return (*(HICON (APIENTRY *)(HINSTANCE, LPSTR, LPWORD))s_dynprocExtractAssociatedIconA.pfn)
            (hInst, lpIconPath, lpiIcon);
}

HICON APIENTRY
ExtractAssociatedIconW(HINSTANCE hInst, LPWSTR lpIconPath, LPWORD lpiIcon)
{
    CStrIn strIconPath(lpIconPath);

    return ExtractAssociatedIconA(hInst, strIconPath, lpiIcon);
}

STDAPI
SHGetFolderPath(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, TCHAR * pchPath)
{
    static DYNPROC s_dynprocSHGetFolderPath =
            { NULL, &g_dynlibSHFOLDER, "SHGetFolderPathW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocSHGetFolderPath);
    if (hr) 
        return 0;

    return (*(HRESULT (STDAPICALLTYPE *)(HWND, int, HANDLE, DWORD, LPWSTR))s_dynprocSHGetFolderPath.pfn)
            (hwnd, csidl, hToken, dwFlags, pchPath);
}

STDAPI
SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pchPath)
{
    static DYNPROC s_dynprocSHGetFolderPathA =
            { NULL, &g_dynlibSHFOLDER, "SHGetFolderPathA" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocSHGetFolderPathA);
    if (hr) 
        return 0;

    return (*(HRESULT (STDAPICALLTYPE *)(HWND, int, HANDLE, DWORD, LPSTR))s_dynprocSHGetFolderPathA.pfn)
            (hwnd, csidl, hToken, dwFlags, pchPath);
}
