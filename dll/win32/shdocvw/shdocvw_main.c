/*
 * SHDOCVW - Internet Explorer Web Control
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
 * Copyright 2004 Mike McCormack (for CodeWeavers)
 * Copyright 2008 Detlef Riekenberg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#include "wine/unicode.h"
#include "wine/debug.h"

#include "shdocvw.h"

#include "winreg.h"
#include "shlwapi.h"
#include "wininet.h"

#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

LONG SHDOCVW_refCount = 0;

HINSTANCE shdocvw_hinstance = 0;
static HMODULE SHDOCVW_hshell32 = 0;
static ITypeInfo *wb_typeinfo = NULL;

HRESULT get_typeinfo(ITypeInfo **typeinfo)
{
    ITypeLib *typelib;
    HRESULT hres;

    if(wb_typeinfo) {
        *typeinfo = wb_typeinfo;
        return S_OK;
    }

    hres = LoadRegTypeLib(&LIBID_SHDocVw, 1, 1, LOCALE_SYSTEM_DEFAULT, &typelib);
    if(FAILED(hres)) {
        ERR("LoadRegTypeLib failed: %08x\n", hres);
        return hres;
    }

    hres = ITypeLib_GetTypeInfoOfGuid(typelib, &IID_IWebBrowser2, &wb_typeinfo);
    ITypeLib_Release(typelib);

    *typeinfo = wb_typeinfo;
    return hres;
}

/*************************************************************************
 * SHDOCVW DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hinst, fdwReason, fImpLoad);
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        shdocvw_hinstance = hinst;
        register_iewindow_class();
        break;
    case DLL_PROCESS_DETACH:
        if (SHDOCVW_hshell32) FreeLibrary(SHDOCVW_hshell32);
        unregister_iewindow_class();
        if(wb_typeinfo)
            ITypeInfo_Release(wb_typeinfo);
        break;
    }
    return TRUE;
}

/*************************************************************************
 *              DllCanUnloadNow (SHDOCVW.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return SHDOCVW_refCount ? S_FALSE : S_OK;
}

/***********************************************************************
 *              DllGetVersion (SHDOCVW.@)
 */
HRESULT WINAPI DllGetVersion(DLLVERSIONINFO *info)
{
    if (info->cbSize != sizeof(DLLVERSIONINFO)) FIXME("support DLLVERSIONINFO2\n");

    /* this is what IE6 on Windows 98 reports */
    info->dwMajorVersion = 6;
    info->dwMinorVersion = 0;
    info->dwBuildNumber = 2600;
    info->dwPlatformID = DLLVER_PLATFORM_WINDOWS;

    return NOERROR;
}

/*************************************************************************
 *              DllInstall (SHDOCVW.@)
 */
HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
   FIXME("(%s, %s): stub!\n", bInstall ? "TRUE":"FALSE", debugstr_w(cmdline));

   return S_OK;
}

/*************************************************************************
 * SHDOCVW_LoadShell32
 *
 * makes sure the handle to shell32 is valid
 */
static BOOL SHDOCVW_LoadShell32(void)
{
     if (SHDOCVW_hshell32)
       return TRUE;
     return ((SHDOCVW_hshell32 = LoadLibraryA("shell32.dll")) != NULL);
}

/***********************************************************************
 *		@ (SHDOCVW.110)
 *
 * Called by Win98 explorer.exe main binary, definitely has 0
 * parameters.
 */
DWORD WINAPI WinList_Init(void)
{
    FIXME("(), stub!\n");
    return 0x0deadfeed;
}

/***********************************************************************
 *		@ (SHDOCVW.118)
 *
 * Called by Win98 explorer.exe main binary, definitely has only one
 * parameter.
 */
static BOOL (WINAPI *pShellDDEInit)(BOOL start) = NULL;

BOOL WINAPI ShellDDEInit(BOOL start)
{
    TRACE("(%d)\n", start);

    if (!pShellDDEInit)
    {
      if (!SHDOCVW_LoadShell32())
        return FALSE;
      pShellDDEInit = (void *)GetProcAddress(SHDOCVW_hshell32, (LPCSTR)188);
    }

    if (pShellDDEInit)
      return pShellDDEInit(start);
    else
      return FALSE;
}

/***********************************************************************
 *		@ (SHDOCVW.125)
 *
 * Called by Win98 explorer.exe main binary, definitely has 0
 * parameters.
 */
DWORD WINAPI RunInstallUninstallStubs(void)
{
    FIXME("(), stub!\n");
    return 0x0deadbee;
}

/***********************************************************************
 *              SetQueryNetSessionCount (SHDOCVW.@)
 */
DWORD WINAPI SetQueryNetSessionCount(DWORD arg)
{
    FIXME("(%u), stub!\n", arg);
    return 0;
}

/**********************************************************************
 * OpenURL  (SHDOCVW.@)
 */
void WINAPI OpenURL(HWND hWnd, HINSTANCE hInst, LPCSTR lpcstrUrl, int nShowCmd)
{
    FIXME("%p %p %s %d\n", hWnd, hInst, debugstr_a(lpcstrUrl), nShowCmd);
}

/**********************************************************************
 * Some forwards (by ordinal) to SHLWAPI
 */

static void* fetch_shlwapi_ordinal(UINT_PTR ord)
{
    static const WCHAR shlwapiW[] = {'s','h','l','w','a','p','i','.','d','l','l','\0'};
    static HANDLE h;

    if (!h && !(h = GetModuleHandleW(shlwapiW))) return NULL;
    return (void*)GetProcAddress(h, (const char*)ord);
}

/******************************************************************
 *		WhichPlatformFORWARD            (SHDOCVW.@)
 */
DWORD WINAPI WhichPlatformFORWARD(void)
{
    static DWORD (WINAPI *p)(void);

    if (p || (p = fetch_shlwapi_ordinal(276))) return p();
    return 1; /* not integrated, see shlwapi.WhichPlatform */
}

/******************************************************************
 *		StopWatchModeFORWARD            (SHDOCVW.@)
 */
void WINAPI StopWatchModeFORWARD(void)
{
    static void (WINAPI *p)(void);

    if (p || (p = fetch_shlwapi_ordinal(241))) p();
}

/******************************************************************
 *		StopWatchFlushFORWARD            (SHDOCVW.@)
 */
void WINAPI StopWatchFlushFORWARD(void)
{
    static void (WINAPI *p)(void);

    if (p || (p = fetch_shlwapi_ordinal(242))) p();
}

/******************************************************************
 *		StopWatchWFORWARD            (SHDOCVW.@)
 */
DWORD WINAPI StopWatchWFORWARD(DWORD dwClass, LPCWSTR lpszStr, DWORD dwUnknown,
                               DWORD dwMode, DWORD dwTimeStamp)
{
    static DWORD (WINAPI *p)(DWORD, LPCWSTR, DWORD, DWORD, DWORD);

    if (p || (p = fetch_shlwapi_ordinal(243)))
        return p(dwClass, lpszStr, dwUnknown, dwMode, dwTimeStamp);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************
 *		StopWatchAFORWARD            (SHDOCVW.@)
 */
DWORD WINAPI StopWatchAFORWARD(DWORD dwClass, LPCSTR lpszStr, DWORD dwUnknown,
                               DWORD dwMode, DWORD dwTimeStamp)
{
    static DWORD (WINAPI *p)(DWORD, LPCSTR, DWORD, DWORD, DWORD);

    if (p || (p = fetch_shlwapi_ordinal(244)))
        return p(dwClass, lpszStr, dwUnknown, dwMode, dwTimeStamp);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************
 *  URLSubRegQueryA (SHDOCVW.151)
 */
HRESULT WINAPI URLSubRegQueryA(LPCSTR regpath, LPCSTR name, DWORD type,
                               LPSTR out, DWORD outlen, DWORD unknown)
{
    CHAR buffer[INTERNET_MAX_URL_LENGTH];
    DWORD len;
    LONG res;

    TRACE("(%s, %s, %d, %p, %d, %d)\n", debugstr_a(regpath), debugstr_a(name),
            type, out, outlen, unknown);

    if (!out) return S_OK;

    len = sizeof(buffer);
    res = SHRegGetUSValueA(regpath, name, NULL, buffer,  &len, FALSE, NULL, 0);
    if (!res) {
        lstrcpynA(out, buffer, outlen);
        return S_OK;
    }

    return E_FAIL;
}

/******************************************************************
 *  ParseURLFromOutsideSourceW (SHDOCVW.170)
 */
DWORD WINAPI ParseURLFromOutsideSourceW(LPCWSTR url, LPWSTR out, LPDWORD plen, LPDWORD unknown)
{
    WCHAR buffer_in[INTERNET_MAX_URL_LENGTH];
    WCHAR buffer_out[INTERNET_MAX_URL_LENGTH];
    LPCWSTR ptr = url;
    HRESULT hr;
    DWORD needed;
    DWORD len;
    DWORD res = 0;


    TRACE("(%s, %p, %p, %p) len: %d, unknown: 0x%x\n", debugstr_w(url), out, plen, unknown,
            plen ? *plen : 0, unknown ? *unknown : 0);

    if (!PathIsURLW(ptr)) {
        len = sizeof(buffer_in) / sizeof(buffer_in[0]);
        buffer_in[0] = 0;
        hr = UrlApplySchemeW(ptr, buffer_in, &len, URL_APPLY_GUESSSCHEME);
        TRACE("got 0x%x with %s\n", hr, debugstr_w(buffer_in));
        if (hr != S_OK) {
            /* when we can't guess the scheme, use the default scheme */
            len = sizeof(buffer_in) / sizeof(buffer_in[0]);
            hr = UrlApplySchemeW(ptr, buffer_in, &len, URL_APPLY_DEFAULT);
        }

        if (hr == S_OK) {
            /* we parsed the url to buffer_in */
            ptr = buffer_in;
        }
        else
        {
            FIXME("call search hook for %s\n", debugstr_w(ptr));
        }
    }

    len = sizeof(buffer_out) / sizeof(buffer_out[0]);
    buffer_out[0] = '\0';
    hr = UrlCanonicalizeW(ptr, buffer_out, &len, URL_ESCAPE_SPACES_ONLY);
    needed = lstrlenW(buffer_out)+1;
    TRACE("got 0x%x with %s (need %d)\n", hr, debugstr_w(buffer_out), needed);

    if (*plen >= needed) {
        if (out != NULL) {
            lstrcpyW(out, buffer_out);
            res++;
        }
        needed--;
    }

    *plen = needed;

    TRACE("=> %d\n", res);
    return res;
}

/******************************************************************
 *  ParseURLFromOutsideSourceA (SHDOCVW.169)
 *
 * See ParseURLFromOutsideSourceW
 */
DWORD WINAPI ParseURLFromOutsideSourceA(LPCSTR url, LPSTR out, LPDWORD plen, LPDWORD unknown)
{
    WCHAR buffer[INTERNET_MAX_URL_LENGTH];
    LPWSTR urlW = NULL;
    DWORD needed;
    DWORD res;
    DWORD len;

    TRACE("(%s, %p, %p, %p) len: %d, unknown: 0x%x\n", debugstr_a(url), out, plen, unknown,
            plen ? *plen : 0, unknown ? *unknown : 0);

    if (url) {
        len = MultiByteToWideChar(CP_ACP, 0, url, -1, NULL, 0);
        urlW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, url, -1, urlW, len);
    }

    len = sizeof(buffer) / sizeof(buffer[0]);
    res = ParseURLFromOutsideSourceW(urlW, buffer, &len, unknown);
    HeapFree(GetProcessHeap(), 0, urlW);

    needed = WideCharToMultiByte(CP_ACP, 0, buffer, -1, NULL, 0, NULL, NULL);

    res = 0;
    if (*plen >= needed) {
        if (out != NULL) {
            WideCharToMultiByte(CP_ACP, 0, buffer, -1, out, *plen, NULL, NULL);
            res = needed;
        }
        needed--;
    }

    *plen = needed;

    TRACE("=> %d\n", res);
    return res;
}

/******************************************************************
 *  IEParseDisplayNameWithBCW (SHDOCVW.218)
 */
HRESULT WINAPI IEParseDisplayNameWithBCW(DWORD codepage, LPCWSTR lpszDisplayName, LPBC pbc, LPITEMIDLIST *ppidl)
{
    /* Guessing at parameter 3 based on IShellFolder's  ParseDisplayName */
    FIXME("stub: 0x%x %s %p %p\n",codepage,debugstr_w(lpszDisplayName),pbc,ppidl);
    return E_FAIL;
}
