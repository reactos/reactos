/*
 *  ReactOS Control Panel Applet
 *
 *  dllmain.c
 *
 *  Copyright (C) 2002  Robert Dickenson  <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>

HMODULE hModule;

BOOL APIENTRY 
DllMain(PVOID hinstDll,
	ULONG dwReason,
	PVOID reserved)
{

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
	//DisableThreadLibraryCalls(hinstDll);
	//RegInitialize();
        hModule = (HMODULE)hinstDll;
    break;

    case DLL_PROCESS_DETACH:
	//RegCleanup();
	break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
	break;
    }

   return TRUE;
}
