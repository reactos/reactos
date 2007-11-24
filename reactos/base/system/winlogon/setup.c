/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/setup.c
 * PURPOSE:         Setup support functions
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(winlogon);

/* FUNCTIONS ****************************************************************/

DWORD
GetSetupType(VOID)
{
  DWORD dwError;
  HKEY hKey;
  DWORD dwType;
  DWORD dwSize;
  DWORD dwSetupType;

  dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
			 L"SYSTEM\\Setup", //TEXT("SYSTEM\\Setup"),
			 0,
			 KEY_QUERY_VALUE,
			 &hKey);
  if (dwError != ERROR_SUCCESS)
    {
      return 0;
    }

  dwSize = sizeof(DWORD);
  dwError = RegQueryValueExW (hKey,
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


static DWORD WINAPI
RunSetupThreadProc (IN LPVOID lpParameter)
{
  PROCESS_INFORMATION ProcessInformation;
  STARTUPINFOW StartupInfo;
  WCHAR Shell[MAX_PATH];
  WCHAR CommandLine[MAX_PATH];
  BOOL Result;
  DWORD dwError;
  HKEY hKey;
  DWORD dwType;
  DWORD dwSize;
  DWORD dwExitCode;

  TRACE ("RunSetup() called\n");

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

  TRACE ("Should run '%S' now.\n", CommandLine);

  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.lpReserved = NULL;
  StartupInfo.lpDesktop = NULL;
  StartupInfo.lpTitle = NULL;
  StartupInfo.dwFlags = 0;
  StartupInfo.cbReserved2 = 0;
  StartupInfo.lpReserved2 = 0;

  TRACE ("Creating new setup process\n");

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
      TRACE ("Failed to run setup process\n");
      return FALSE;
    }

  /* Wait for process termination */
  WaitForSingleObject (ProcessInformation.hProcess, INFINITE);

  GetExitCodeProcess (ProcessInformation.hProcess, &dwExitCode);

  CloseHandle (ProcessInformation.hThread);
  CloseHandle (ProcessInformation.hProcess);

  TRACE ("RunSetup() done.\n");

  return TRUE;
}


BOOL
RunSetup (VOID)
{
	HANDLE hThread;

	hThread = CreateThread(NULL, 0, RunSetupThreadProc, NULL, 0, NULL);
	return hThread != NULL;
}

/* EOF */
