/*
 * SetupAPI device installer
 *
 * Copyright 2000 Andreas Mohr for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "setupx16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/***********************************************************************
 *		DiGetClassDevs (SETUPX.304)
 * Return a list of installed system devices.
 * Uses HKLM\\ENUM to list devices.
 */
RETERR16 WINAPI DiGetClassDevs16(LPLPDEVICE_INFO16 lplpdi,
                                 LPCSTR lpszClassName, HWND16 hwndParent, INT16 iFlags)
{
    LPDEVICE_INFO16 lpdi;

    FIXME("(%p, '%s', %04x, %04x), semi-stub.\n",
          lplpdi, lpszClassName, hwndParent, iFlags);
    lpdi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DEVICE_INFO16));
    lpdi->cbSize = sizeof(DEVICE_INFO16);
    *lplpdi = (LPDEVICE_INFO16)MapLS(lpdi);
    return OK;
}

/***********************************************************************
 *		DiBuildCompatDrvList (SETUPX.300)
 */
RETERR16 WINAPI DiBuildCompatDrvList16(LPDEVICE_INFO16 lpdi)
{
    FIXME("(%p): stub\n", lpdi);
    lpdi->lpCompatDrvList = NULL;
    return FALSE;
}

/***********************************************************************
 *	        DiBuildClassDrvList (SETUPX.301)
 */
RETERR16 WINAPI DiBuildClassDrvList16(LPDEVICE_INFO16 lpdi)
{
    FIXME("(%p): stub\n", lpdi);
    lpdi->lpCompatDrvList = NULL;
    return FALSE;
}

/***********************************************************************
 *		DiCallClassInstaller (SETUPX.308)
 */
RETERR16 WINAPI DiCallClassInstaller16(/*DI_FUNCTIONS*/WORD diFctn, LPDEVICE_INFO16 lpdi)
{
    FIXME("(%x, %p): stub\n", diFctn, lpdi);
    return FALSE;
}

/***********************************************************************
 *		DiCreateDevRegKey (SETUPX.318)
 */
RETERR16 WINAPI DiCreateDevRegKey16(LPDEVICE_INFO16 lpdi,
                                    VOID* p2, WORD w3,
                                    LPCSTR s4, WORD w5)
{
    FIXME("(%p, %p, %x, %s, %x): stub\n", lpdi, p2, w3, debugstr_a(s4), w5);
    return FALSE;
}

/***********************************************************************
 *		DiDeleteDevRegKey (SETUPX.344)
 */
RETERR16 WINAPI DiDeleteDevRegKey16(LPDEVICE_INFO16 lpdi, INT16 iFlags)
{
    FIXME("(%p, %x): stub\n", lpdi, iFlags);
    return FALSE;
}

/***********************************************************************
 *		DiCreateDeviceInfo (SETUPX.303)
 */
RETERR16 WINAPI DiCreateDeviceInfo16(LPLPDEVICE_INFO16 lplpdi,
                                     LPCSTR lpszDescription, DWORD dnDevnode,
                                     HKEY16 hkey, LPCSTR lpszRegsubkey,
                                     LPCSTR lpszClassName, HWND16 hwndParent)
{
    LPDEVICE_INFO16 lpdi;
    FIXME("(%p %s %08lx %x %s %s %x): stub\n", lplpdi,
          debugstr_a(lpszDescription), dnDevnode, hkey,
          debugstr_a(lpszRegsubkey), debugstr_a(lpszClassName), hwndParent);
    lpdi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DEVICE_INFO16));
    lpdi->cbSize = sizeof(DEVICE_INFO16);
    strcpy(lpdi->szClassName, lpszClassName);
    lpdi->hwndParent = hwndParent;
    *lplpdi = (LPDEVICE_INFO16)MapLS(lpdi);
    return OK;
}

/***********************************************************************
 *		DiDestroyDeviceInfoList (SETUPX.305)
 */
RETERR16 WINAPI DiDestroyDeviceInfoList16(LPDEVICE_INFO16 lpdi)
{
    FIXME("(%p): stub\n", lpdi);
    return FALSE;
}

/***********************************************************************
 *		DiOpenDevRegKey (SETUPX.319)
 */
RETERR16 WINAPI DiOpenDevRegKey16(LPDEVICE_INFO16 lpdi,
                                  LPHKEY16 lphk,INT16 iFlags)
{
    FIXME("(%p %p %d): stub\n", lpdi, lphk, iFlags);
    return FALSE;
}
