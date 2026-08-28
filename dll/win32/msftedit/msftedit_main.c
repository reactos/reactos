/*
 * msftedit main file
 *
 * Copyright (C) 2008 Rico Schüller
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
 *
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "richedit.h"
#include "imm.h"
#include "shlwapi.h"
#include "oleidl.h"
#include "initguid.h"
#include "textserv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msftedit);

/***********************************************************************
 * DllMain.
 */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    static HMODULE richedit;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        /* explicitly load riched20 since it creates the window classes at dll attach time */
        richedit = LoadLibraryW( L"riched20.dll" );
        DisableThreadLibraryCalls(inst);
        break;
    case DLL_PROCESS_DETACH:
        FreeLibrary( richedit );
        break;
    }
    return TRUE;
}

/***********************************************************************
 *              DllGetVersion (msftedit.@)
 */
HRESULT WINAPI DllGetVersion(DLLVERSIONINFO *info)
{
    if (info->cbSize != sizeof(DLLVERSIONINFO)) FIXME("support DLLVERSIONINFO2\n");

    /* this is what WINXP SP2 reports */
    info->dwMajorVersion = 41;
    info->dwMinorVersion = 15;
    info->dwBuildNumber = 1507;
    info->dwPlatformID = 1;
    return NOERROR;
}
