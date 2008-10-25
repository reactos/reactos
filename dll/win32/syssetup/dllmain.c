/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Main file
 * FILE:              lib/syssetup/dllmain.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <commctrl.h>
#include <setupapi.h>

#include "globals.h"


/* GLOBALS *******************************************************************/

HINSTANCE hDllInstance;


/* FUNCTIONS *****************************************************************/

BOOL STDCALL
DllMain (HINSTANCE hInstance,
	 DWORD dwReason,
	 LPVOID lpReserved)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    INITCOMMONCONTROLSEX InitControls;

    InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitControls.dwICC = ICC_DATE_CLASSES | ICC_PROGRESS_CLASS | ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&InitControls);
    hDllInstance = hInstance;
  }

   return TRUE;
}

/* EOF */
