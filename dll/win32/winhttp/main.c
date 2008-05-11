/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "winhttp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

/******************************************************************
 *              DllMain (winhttp.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/******************************************************************
 *		DllGetClassObject (winhttp.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    FIXME("(%s %s %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************
 *              DllCanUnloadNow (winhttp.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("()\n");
    return S_FALSE;
}

/***********************************************************************
 *          DllRegisterServer (winhttp.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    FIXME("()\n");
    return S_OK;
}

/***********************************************************************
 *          DllUnregisterServer (winhttp.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    FIXME("()\n");
    return S_OK;
}

/***********************************************************************
 *          WinHttpCheckPlatform (winhttp.@)
 */
BOOL WINAPI WinHttpCheckPlatform(void)
{
    FIXME("stub\n");
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

/***********************************************************************
 *          WinHttpDetectAutoProxyConfigUrl (winhttp.@)
 */
BOOL WINAPI WinHttpDetectAutoProxyConfigUrl(DWORD flags, LPWSTR *url)
{
    FIXME("(%x %p)\n", flags, url);

    SetLastError(ERROR_WINHTTP_AUTODETECTION_FAILED);
    return FALSE;
}

/***********************************************************************
 *          WinHttpGetIEProxyConfigForCurrentUser (winhttp.@)
 */
BOOL WINAPI WinHttpGetIEProxyConfigForCurrentUser(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG* config)
{
    if(!config)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* TODO: read from HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings */
    FIXME("returning no proxy used\n");
    config->fAutoDetect = FALSE;
    config->lpszAutoConfigUrl = NULL;
    config->lpszProxy = NULL;
    config->lpszProxyBypass = NULL;

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

/***********************************************************************
 *          WinHttpOpen (winhttp.@)
 */
HINTERNET WINAPI WinHttpOpen(LPCWSTR pwszUserAgent, DWORD dwAccessType,
                             LPCWSTR pwszProxyName, LPCWSTR pwszProxyByPass,
                             DWORD dwFlags)
{
    FIXME("(%s, %d, %s, %s, 0x%x): stub\n", debugstr_w(pwszUserAgent),
        dwAccessType, debugstr_w(pwszProxyName), debugstr_w(pwszProxyByPass),
        dwFlags);

    SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
}
