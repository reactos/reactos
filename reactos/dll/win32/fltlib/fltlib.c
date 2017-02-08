/*
 * Copyright 2009 Detlef Riekenberg
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


#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(fltlib);

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* use native version */

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }
    return TRUE;
}

/**********************************************************************
 *      FilterConnectCommunicationPort         (FLTLIB.@)
 */
HRESULT WINAPI FilterConnectCommunicationPort(LPCWSTR lpPortName, DWORD dwOptions,
                                              LPVOID lpContext, DWORD dwSizeOfContext,
                                              LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                              HANDLE *hPort)
{
    FIXME("(%s, %d, %p, %d, %p, %p) stub\n", debugstr_w(lpPortName), dwOptions,
        lpContext, dwSizeOfContext, lpSecurityAttributes, hPort);

    *hPort = INVALID_HANDLE_VALUE;
    return E_NOTIMPL;
}

/**********************************************************************
 *      FilterFindFirst         (FLTLIB.@)
 */
HRESULT WINAPI FilterFindFirst(DWORD class, LPVOID buffer, DWORD size, LPDWORD bytes_returned,
                               LPHANDLE handle)
{
    FIXME("(%u, %p, %u, %p, %p) stub\n", class, buffer, size, bytes_returned, handle);
    return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
}

/**********************************************************************
 *      FilterFindClose         (FLTLIB.@)
 */
HRESULT WINAPI FilterFindClose(HANDLE handle)
{
    FIXME("(%p) stub\n", handle);
    return S_OK;
}

/**********************************************************************
 *      FilterLoad              (FLTLIB.@)
 */
HRESULT WINAPI FilterLoad(LPCWSTR filtername)
{
    FIXME("(%s) stub\n", debugstr_w(filtername));
    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
}

/**********************************************************************
 *      FilterUnload            (FLTLIB.@)
 */
HRESULT WINAPI FilterUnload(LPCWSTR filtername)
{
    FIXME("(%s) stub\n", debugstr_w(filtername));

    if (!filtername)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    return S_OK;
}
