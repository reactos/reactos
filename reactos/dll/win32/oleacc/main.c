/*
 * Implementation of the OLEACC dll
 *
 * Copyright 2003 Mike McCormack for CodeWeavers
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
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oleacc);

HRESULT WINAPI CreateStdAccessibleObject( HWND hwnd, LONG idObject,
                             REFIID riidInterface, void** ppvObject )
{
    FIXME("%p %d %s %p\n", hwnd, idObject,
          debugstr_guid( riidInterface ), ppvObject );
    return E_NOTIMPL;
}

HRESULT WINAPI LresultFromObject( REFIID riid, WPARAM wParam, LPUNKNOWN pAcc )
{
    FIXME("%s %ld %p\n", debugstr_guid(riid), wParam, pAcc );
    return E_NOTIMPL;
}

HRESULT WINAPI AccessibleObjectFromWindow( HWND hwnd, DWORD dwObjectID,
                             REFIID riid, void** ppvObject )
{
    FIXME("%p %d %s %p\n", hwnd, dwObjectID,
          debugstr_guid( riid ), ppvObject );
    return E_NOTIMPL;
}

HRESULT WINAPI DllRegisterServer(void)
{
    FIXME("\n");
    return S_OK;
}

HRESULT WINAPI DllUnregisterServer(void)
{
    FIXME("\n");
    return S_OK;
}
