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
/* $Id: setup.c,v 1.3 2004/06/04 23:46:02 navaraf Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS winlogon
 * FILE:            subsys/system/winlogon/setup.h
 * PURPOSE:         Setup support functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <stdio.h>
#include <ntsecapi.h>
#include <wchar.h>

#include "setup.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

DWORD
GetSetupType(VOID)
{
  DWORD dwError;
  HANDLE hKey;
  DWORD dwType;
  DWORD dwSize;
  DWORD dwSetupType;

  dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			 L"SYSTEM\\Setup", //TEXT("SYSTEM\\Setup"),
			 0,
			 KEY_QUERY_VALUE,
			 &hKey);
  if (dwError != ERROR_SUCCESS)
    {
      return 0;
    }

  dwSize = sizeof(DWORD);
  dwError = RegQueryValueEx (hKey,
			     L"SetupType", //TEXT("SetupType"),
			     NULL,
			     &dwType,
			     (LPBYTE)&dwSetupType,
			     &dwSize);
  RegCloseKey (hKey);
  if (dwError != ERROR_SUCCESS || dwType != REG_DWORD)
    {
      return 0;
    }

  return dwSetupType;
}


BOOL
SetSetupType (DWORD dwSetupType)
{
  DWORD dwError;
  HANDLE hKey;

  dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			 L"SYSTEM\\Setup", //TEXT("SYSTEM\\Setup"),
			 0,
			 KEY_QUERY_VALUE,
			 &hKey);
  if (dwError != ERROR_SUCCESS)
    {
      return FALSE;
    }

  dwError = RegSetValueEx (hKey,
			   L"SetupType", //TEXT("SetupType"),
			   0,
			   REG_DWORD,
			   (LPBYTE)&dwSetupType,
			   sizeof(DWORD));
  RegCloseKey (hKey);
  if (dwError != ERROR_SUCCESS)
    {
      return FALSE;
    }

  return TRUE;
}


BOOL
RunSetup (VOID)
{
  PROCESS_INFORMATION ProcessInformation;
  STARTUPINFO StartupInfo;
  WCHAR Shell[MAX_PATH];
  WCHAR CommandLine[MAX_PATH];
  BOOLEAN Result;
  DWORD dwError;
  HANDLE hKey;
  DWORD dwType;
  DWORD dwSize;
  DWORD dwExitCode;

  DPRINT ("RunSetup() called\n");

  dwError = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
			   L"SYSTEM\\Setup",
			   0,
			   KEY_QUERY_VALUE,
			   &hKey);
  if (dwError != ERROR_SUCCESS)
    {
      return FALSE;
    }

  dwSize = MAX_PATH;
  dwError = RegQueryValueExW (hKey,
			      L"CmdLine",
			      NULL,
			      &dwType,
			      (LPBYTE)Shell,
			      &dwSize);
  RegCloseKey (hKey);
  if (dwError != ERROR_SUCCESS)
    {
      return FALSE;
    }

  if (dwType == REG_EXPAND_SZ)
    {
      ExpandEnvironmentStringsW(Shell, CommandLine, MAX_PATH);
    }
  else if (dwType == REG_SZ)
    {
      wcscpy(CommandLine, Shell);
    }
  else
    {
      return FALSE;
    }

  DPRINT ("Winlogon: Should run '%S' now.\n", CommandLine);

  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.lpReserved = NULL;
  StartupInfo.lpDesktop = NULL;
  StartupInfo.lpTitle = NULL;
  StartupInfo.dwFlags = 0;
  StartupInfo.cbReserved2 = 0;
  StartupInfo.lpReserved2 = 0;

  DPRINT ("Winlogon: Creating new setup process\n");

  Result = CreateProcessW (NULL,
			   CommandLine,
			   NULL,
			   NULL,
			   FALSE,
			   DETACHED_PROCESS,
			   NULL,
			   NULL,
			   &StartupInfo,
			   &ProcessInformation);
  if (!Result)
    {
      DPRINT ("Winlogon: Failed to run setup process\n");
      return FALSE;
    }

  /* Wait for process termination */
  WaitForSingleObject (ProcessInformation.hProcess, INFINITE);

  GetExitCodeProcess (ProcessInformation.hProcess, &dwExitCode);

  CloseHandle (ProcessInformation.hThread);
  CloseHandle (ProcessInformation.hProcess);

  if (dwExitCode == 0)
    {
      SetSetupType (0);
    }

  DPRINT ("Winlogon: RunSetup() done.\n");

  return TRUE;
}


/* EOF */
