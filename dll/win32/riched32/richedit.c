/*
 * RichEdit32  functions
 *
 * This module is a simple wrapper for the RichEdit 2.0 control
 *
 * Copyright 2000 by Jean-Claude Batista
 * Copyright 2005 Mike McCormack
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
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winerror.h"
#include "winuser.h"
#include "richedit.h"
#include "shlwapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

/* Window procedure of the RichEdit 1.0 control in riched20.dll */
extern LRESULT WINAPI RichEdit10ANSIWndProc(HWND, UINT, WPARAM, LPARAM);


/* Registers the window class. */
static BOOL RICHED32_Register(void)
{
    WNDCLASSA wndClass;

    ZeroMemory(&wndClass, sizeof(WNDCLASSA));
    wndClass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
    wndClass.lpfnWndProc = RichEdit10ANSIWndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = sizeof(void *);
    wndClass.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = RICHEDIT_CLASS10A; /* WC_RICHED32A; */

    RegisterClassA(&wndClass);

    return TRUE;
}

/* Initialization function */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("\n");
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        return RICHED32_Register();

    case DLL_PROCESS_DETACH:
        if (lpvReserved) break;
        UnregisterClassA(RICHEDIT_CLASS10A, NULL);
        break;
    }
    return TRUE;
}

/***********************************************************************
 * DllGetVersion [RICHED32.2]
 *
 * Retrieves version information
 */
HRESULT WINAPI DllGetVersion (DLLVERSIONINFO *pdvi)
{
    TRACE("\n");

    if (pdvi->cbSize != sizeof(DLLVERSIONINFO))
	return E_INVALIDARG;

    pdvi->dwMajorVersion = 4;
    pdvi->dwMinorVersion = 0;
    pdvi->dwBuildNumber = 0;
    pdvi->dwPlatformID = 0;

    return S_OK;
}
