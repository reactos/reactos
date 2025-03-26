/*
 * Copyright 2006 Alexandre Julliard
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
#include "winreg.h"
#include "winerror.h"
#include "shellapi.h"
#include "shlwapi.h"
#include "intshcut.h"
#include "winuser.h"
#include "commctrl.h"
#include "prsht.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(url);

/***********************************************************************
 *		DllMain  (URL.@)
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
#ifndef __REACTOS__
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
#endif
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( inst );
        break;
    }
    return TRUE;
}

/***********************************************************************
 * AddMIMEFileTypesPS (URL.@)
 *
 * Build and Manage a Filetype-Association Property Sheet
 *
 * PARAMS
 *  unknown1 [I] Pointer to an Read-Only Area
 *  lppsh    [I] PTR to the target PropertySheetHeader (ANSI)
 *
 * RETURNS
 *  Success: 0
 *
 */
DWORD WINAPI AddMIMEFileTypesPS(VOID * unknown1, LPPROPSHEETHEADERA lppsh)
{
    FIXME("(%p, %p): stub!\n", unknown1, lppsh);
    return 0;
}

/***********************************************************************
 * InetIsOffline    (URL.@)
 *
 */
BOOL WINAPI InetIsOffline(DWORD flags)
{
    FIXME("(%08x): stub!\n", flags);

    return FALSE;
}

/***********************************************************************
 * FileProtocolHandlerA    (URL.@)
 *
 * Handles a URL given to it and executes it.
 *
 * HWND hWnd - Parent Window
 * HINSTANCE hInst - ignored
 * LPCSTR pszUrl - The URL that needs to be handled
 * int nShowCmd - How to display the operation.
 */

void WINAPI FileProtocolHandlerA(HWND hWnd, HINSTANCE hInst, LPCSTR pszUrl, int nShowCmd)
{
    CHAR pszPath[MAX_PATH];
    DWORD size = MAX_PATH;
    HRESULT createpath = PathCreateFromUrlA(pszUrl,pszPath,&size,0);

    TRACE("(%p, %s, %d)\n",hWnd,debugstr_a(pszUrl),nShowCmd);

    ShellExecuteA(hWnd, NULL, createpath==S_OK ? pszPath : pszUrl, NULL, NULL, nShowCmd);
}

/***********************************************************************
 * OpenURLA    (URL.@)
 *
 */
void WINAPI OpenURLA(HWND hwnd, HINSTANCE inst, LPCSTR cmdline, INT show)
{
#ifdef __REACTOS__
    TRACE("(%p, %p, %s, %d)\n", hwnd, inst, debugstr_a(cmdline), show);
    ShellExecuteA(hwnd, NULL, cmdline, NULL, NULL, show);
#else
    FIXME("(%p, %p, %s, %d): stub!\n", hwnd, inst, debugstr_a(cmdline), show);
#endif
}

/***********************************************************************
 * TelnetProtocolHandlerA    (URL.@)
 *
 */

HRESULT WINAPI TelnetProtocolHandlerA(HWND hWnd, LPSTR lpStr)
{
    FIXME("(%p, %p): stub!\n",hWnd,lpStr);

    return E_NOTIMPL;
}
