/*
 * ReactOS Access Control List Editor
 * Copyright (C) 2004 ReactOS Team
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
/* $Id$
 *
 * PROJECT:         ReactOS Access Control List Editor
 * FILE:            lib/acledit/acledit.c
 * PURPOSE:         Access Control List Editor
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *
 *                  Based on guess work and on this nice article:
 *                    http://www.sysinternals.com/ntw2k/info/acledit.shtml
 *
 * UPDATE HISTORY:
 *      07/09/2004  Created
 */
#include <windows.h>
#include "acleditint.h"
#include "resource.h"

HINSTANCE hDllInstance;


BOOL WINAPI
DllMain(
	HINSTANCE hinstDLL,
	DWORD     dwReason,
	LPVOID    lpvReserved)
{
  UNREFERENCED_PARAMETER(lpvReserved);

  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
      hDllInstance = hinstDLL;
      break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}

