/*
 *  ReactOS shell32 - main library entry point
 *
 *  dllmain.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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
/* $Id: dllmain.c,v 1.2 2002/09/24 15:06:10 robd Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/shell32/misc/dllmain.c
 * PURPOSE:         Library main function
 * PROGRAMMER:      Rex Jolliff (rex@lvcablemodem.com)
 */

//#include <ddk/ntddk.h>
#include <windows.h>
#include <cpl.h>
#include "..\control\control.h"

//#define NDEBUG
//#include <debug.h>
#ifdef _MSC_VER
#pragma warning (disable:4273) // : inconsistent dll linkage.  dllexport assumed.
#define STDCALL CALLBACK
#define WINBOOL BOOL

#else
#endif
#define DPRINT(a)
#define DPRINT1(a)


INT STDCALL
DllMain(PVOID hinstDll,
	ULONG dwReason,
	PVOID reserved)
{
  DPRINT("SHELL32: DllMain() called\n");

  switch (dwReason)
  {
  case DLL_PROCESS_ATTACH:
      hInst = hinstDll;
    break;

  case DLL_PROCESS_DETACH:
    break;
  }

  DPRINT1("SHELL32: DllMain() done\n");

  return TRUE;
}


