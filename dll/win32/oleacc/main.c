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
#include "oleacc.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oleacc);

static HINSTANCE oleacc_handle = 0;

HRESULT WINAPI CreateStdAccessibleObject( HWND hwnd, LONG idObject,
                             REFIID riidInterface, void** ppvObject )
{
    FIXME("%p %d %s %p\n", hwnd, idObject,
          debugstr_guid( riidInterface ), ppvObject );
    return E_NOTIMPL;
}

LRESULT WINAPI LresultFromObject( REFIID riid, WPARAM wParam, LPUNKNOWN pAcc )
{
    FIXME("%s %ld %p\n", debugstr_guid(riid), wParam, pAcc );
    return E_NOTIMPL;
}

HRESULT WINAPI AccessibleObjectFromPoint( POINT ptScreen, IAccessible** ppacc, VARIANT* pvarChild )
{
    FIXME("{%d,%d} %p %p: stub\n", ptScreen.x, ptScreen.y, ppacc, pvarChild );
    return E_NOTIMPL;
}

HRESULT WINAPI AccessibleObjectFromWindow( HWND hwnd, DWORD dwObjectID,
                             REFIID riid, void** ppvObject )
{
    FIXME("%p %d %s %p\n", hwnd, dwObjectID,
          debugstr_guid( riid ), ppvObject );
    return E_NOTIMPL;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason,
                    LPVOID lpvReserved)
{
    TRACE("%p, %d, %p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            oleacc_handle = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }
    return TRUE;
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

void WINAPI GetOleaccVersionInfo(DWORD* pVersion, DWORD* pBuild)
{
    *pVersion = MAKELONG(2,4); /* Windows XP version of oleacc: 4.2.5406.0 */
    *pBuild = MAKELONG(0,5406);
}

UINT WINAPI GetRoleTextW(DWORD role, LPWSTR lpRole, UINT rolemax)
{
    INT ret;
    WCHAR *resptr;

    TRACE("%u %p %u\n", role, lpRole, rolemax);

    /* return role text length */
    if(!lpRole)
        return LoadStringW(oleacc_handle, role, (LPWSTR)&resptr, 0);

    ret = LoadStringW(oleacc_handle, role, lpRole, rolemax);
    if(!(ret > 0)){
        if(rolemax > 0) lpRole[0] = '\0';
        return 0;
    }

    return ret;
}

UINT WINAPI GetRoleTextA(DWORD role, LPSTR lpRole, UINT rolemax)
{
    UINT length;
    WCHAR *roletextW;

    TRACE("%u %p %u\n", role, lpRole, rolemax);

    length = GetRoleTextW(role, NULL, 0);
    if((length == 0) || (lpRole && !rolemax))
        return 0;

    roletextW = HeapAlloc(GetProcessHeap(), 0, (length + 1)*sizeof(WCHAR));
    if(!roletextW)
        return 0;

    GetRoleTextW(role, roletextW, length + 1);

    length = WideCharToMultiByte( CP_ACP, 0, roletextW, -1, NULL, 0, NULL, NULL );

    if(!lpRole){
        HeapFree(GetProcessHeap(), 0, roletextW);
        return length - 1;
    }

    WideCharToMultiByte( CP_ACP, 0, roletextW, -1, lpRole, rolemax, NULL, NULL );

    if(rolemax < length){
        lpRole[rolemax-1] = '\0';
        length = rolemax;
    }

    HeapFree(GetProcessHeap(), 0, roletextW);

    return length - 1;
}
