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
/* $Id: msgina.c,v 1.4 2003/11/24 19:04:23 weiden Exp $
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
  pgContext->pWlxFuncs = (PWLX_DISPATCH_VERSION)pWinlogonFunctions;
  
  /* save the winlogon handle used to call the dispatch functions */
  pgContext->hWlx = hWlx;
  
  /* save window station */
  pgContext->station = lpWinsta;
  
  /* notify winlogon that we will use the default SAS */
  pgContext->pWlxFuncs->WlxUseCtrlAltDel(hWlx);
  
  return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
WlxStartApplication(
	PVOID pWlxContext,
	PWSTR pszDesktopName,
	PVOID pEnvironment,
	PWSTR pszCmdLine)
{
  PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  BOOL Ret;
  
  si.cb = sizeof(STARTUPINFO);
  si.lpReserved = NULL;
  si.lpTitle = pszCmdLine;
  si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
  si.dwFlags = 0;
  si.wShowWindow = SW_SHOW;  
  si.lpReserved2 = NULL;
  si.cbReserved2 = 0;
  si.lpDesktop = pszDesktopName;
  
  Ret = CreateProcessAsUser(pgContext->UserToken,
                            NULL,
                            pszCmdLine,
                            NULL,
                            NULL,
                            FALSE,
                            CREATE_UNICODE_ENVIRONMENT,
                            pEnvironment,
                            NULL,
                            &si,
                            &pi);
  
  VirtualFree(pEnvironment, 0, MEM_RELEASE);
  return Ret;
}

/*
 * @implemented
 */
BOOL WINAPI
WlxActivateUserShell(
	PVOID pWlxContext,
	PWSTR pszDesktopName,
	PWSTR pszMprLogonScript,
	PVOID pEnvironment)
{
  PGINA_CONTEXT pgContext = (PGINA_CONTEXT) pWlxContext;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  HKEY hKey;
  DWORD BufSize, ValueType;
  WCHAR pszUserInitApp[MAX_PATH + 1];
  BOOL Ret;
  
  /* get the path of userinit */
  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", 
                  0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
  {
    return FALSE;
  }
  BufSize = MAX_PATH * sizeof(WCHAR);
  if((RegQueryValueEx(hKey, L"Userinit", NULL, &ValueType, (LPBYTE)pszUserInitApp, 
                     &BufSize) != ERROR_SUCCESS) || (ValueType != REG_SZ))
  {
    RegCloseKey(hKey);
    return FALSE;
  }
  RegCloseKey(hKey);
  
  /* execute userinit */
  si.cb = sizeof(STARTUPINFO);
  si.lpReserved = NULL;
  si.lpTitle = L"userinit";
  si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
  si.dwFlags = 0;
  si.wShowWindow = SW_SHOW;  
  si.lpReserved2 = NULL;
  si.cbReserved2 = 0;
  si.lpDesktop = pszDesktopName;
  
  Ret = CreateProcessAsUser(pgContext->UserToken,
                            pszUserInitApp,
                            NULL,
                            NULL,
                            NULL,
                            FALSE,
                            CREATE_UNICODE_ENVIRONMENT,
                            pEnvironment,
                            NULL,
                            &si,
                            &pi);
  
  VirtualFree(pEnvironment, 0, MEM_RELEASE);
  return Ret;
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

