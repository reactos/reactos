/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: msgina.c,v 1.3 2003/11/24 17:24:29 weiden Exp $
 *
 * PROJECT:         ReactOS msgina.dll
 * FILE:            lib/msgina/msgina.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *      24-11-2003  Created
 */
#include <windows.h>
#include <WinWlx.h>
#include "msgina.h"

extern HINSTANCE hDllInstance;


/*
 * @implemented
 */
BOOL WINAPI
WlxNegotiate(
	DWORD  dwWinlogonVersion,
	PDWORD pdwDllVersion)
{
  if(!pdwDllVersion || (dwWinlogonVersion < GINA_VERSION))
    return FALSE;
  
  *pdwDllVersion = GINA_VERSION;
  
  return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
WlxInitialize(
	LPWSTR lpWinsta,
	HANDLE hWlx,
	PVOID  pvReserved,
	PVOID  pWinlogonFunctions,
	PVOID  *pWlxContext)
{
  PGINA_CONTEXT pgContext;
  
  pgContext = (PGINA_CONTEXT)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(GINA_CONTEXT));
  if(!pgContext)
    return FALSE;
  
  /* return the context to winlogon */
  *pWlxContext = (PVOID)pgContext;
  
  pgContext->hDllInstance = hDllInstance;
  
  /* save pointer to dispatch table */
  pgContext->pWlxFuncs = (PWLX_DISPATCH_VERSION_1_3) pWinlogonFunctions;
  
  /* save the winlogon handle used to call the dispatch functions */
  pgContext->hWlx = hWlx;
  
  /* save window station */
  pgContext->station = lpWinsta;
  
  /* notify winlogon that we will use the default SAS */
  pgContext->pWlxFuncs->WlxUseCtrlAltDel(hWlx);
  
  return TRUE;
}


BOOL STDCALL
DllMain(
	HINSTANCE hinstDLL,
	DWORD     dwReason,
	LPVOID    lpvReserved)
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
      /* fall through */
    case DLL_THREAD_ATTACH:
      hDllInstance = hinstDLL;
      break;
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}

