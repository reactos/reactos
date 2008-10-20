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

#include "winhttp_private.h"

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

#define SCHEME_HTTP  3
#define SCHEME_HTTPS 4

BOOL WINAPI InternetCrackUrlW( LPCWSTR, DWORD, DWORD, LPURL_COMPONENTSW );
BOOL WINAPI InternetCreateUrlW( LPURL_COMPONENTS, DWORD, LPWSTR, LPDWORD );

/***********************************************************************
 *          WinHttpCrackUrl (winhttp.@)
 */
BOOL WINAPI WinHttpCrackUrl( LPCWSTR url, DWORD len, DWORD flags, LPURL_COMPONENTSW components )
{
    BOOL ret;

    TRACE("%s, %d, %x, %p\n", debugstr_w(url), len, flags, components);

    if ((ret = InternetCrackUrlW( url, len, flags, components )))
    {
        /* fix up an incompatibility between wininet and winhttp */
        if (components->nScheme == SCHEME_HTTP) components->nScheme = INTERNET_SCHEME_HTTP;
        else if (components->nScheme == SCHEME_HTTPS) components->nScheme = INTERNET_SCHEME_HTTPS;
        else
        {
            set_last_error( ERROR_WINHTTP_UNRECOGNIZED_SCHEME );
            return FALSE;
        }
    }
    return ret;
}

/***********************************************************************
 *          WinHttpCreateUrl (winhttp.@)
 */
BOOL WINAPI WinHttpCreateUrl( LPURL_COMPONENTS comps, DWORD flags, LPWSTR url, LPDWORD len )
{
    TRACE("%p, 0x%08x, %p, %p\n", comps, flags, url, len);
    return InternetCreateUrlW( comps, flags, url, len );
}
