/*
 * Internet Explorer wrapper
 *
 * Copyright 2006 Mike McCormack
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

//#include <windows.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <winver.h>

#include <advpub.h>
//#include <ole2.h>
//#include <rpcproxy.h>

#include <wine/unicode.h>
#include <wine/debug.h>

extern DWORD WINAPI IEWinMain(const WCHAR*, int);

static BOOL check_native_ie(void)
{
    DWORD handle, size;
    LPWSTR file_desc;
    UINT bytes;
    void* buf;
    BOOL ret;

    static const WCHAR browseui_dllW[] = {'b','r','o','w','s','e','u','i','.','d','l','l',0};
    static const WCHAR wineW[] = {'W','i','n','e',0};
    static const WCHAR file_desc_strW[] =
        {'\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o',
#ifndef __REACTOS__
         '\\','0','4','0','9','0','4','e','4',
#else
         '\\','0','4','0','9','0','4','b','0',
#endif
         '\\','F','i','l','e','D','e','s','c','r','i','p','t','i','o','n',0};

    size = GetFileVersionInfoSizeW(browseui_dllW, &handle);
    if(!size)
        return TRUE;

    buf = HeapAlloc(GetProcessHeap(), 0, size);
    GetFileVersionInfoW(browseui_dllW, 0, size,buf);

    ret = !VerQueryValueW(buf, file_desc_strW, (void**)&file_desc, &bytes) || !strstrW(file_desc, wineW);

    HeapFree(GetProcessHeap(), 0, buf);
    return ret;
}

static DWORD register_iexplore(BOOL doregister)
{
    HRESULT hres;

    if (check_native_ie()) {
        WINE_MESSAGE("Native IE detected, not doing registration\n");
        return 0;
    }

    hres = RegInstallA(NULL, doregister ? "RegisterIE" : "UnregisterIE", NULL);
    return FAILED(hres);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prev, WCHAR *cmdline, int show)
{
    static const WCHAR regserverW[] = {'r','e','g','s','e','r','v','e','r',0};
    static const WCHAR unregserverW[] = {'u','n','r','e','g','s','e','r','v','e','r',0};

    if(*cmdline == '-' || *cmdline == '/') {
        if(!strcmpiW(cmdline+1, regserverW))
            return register_iexplore(TRUE);
        if(!strcmpiW(cmdline+1, unregserverW))
            return register_iexplore(FALSE);
    }

    return IEWinMain(cmdline, show);
}
