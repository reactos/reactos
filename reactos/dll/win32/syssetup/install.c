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
/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           System setup
 * FILE:              lib/syssetup/install.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <commctrl.h>
#include <stdio.h>
#include <io.h>
#include <tchar.h>
#include <stdlib.h>

#include <samlib/samlib.h>
#include <syssetup/syssetup.h>
#include <userenv.h>
#include <setupapi.h>

#include <shlobj.h>
#include <objidl.h>
#include <shlwapi.h>

#include "globals.h"
#include "resource.h"

#include <debug.h>

DWORD WINAPI
CMP_WaitNoPendingInstallEvents(DWORD dwTimeout);

/* GLOBALS ******************************************************************/

PSID DomainSid = NULL;
PSID AdminSid = NULL;

HINF hSysSetupInf = INVALID_HANDLE_VALUE;

/* FUNCTIONS ****************************************************************/

void
DebugPrint(char* fmt,...)
{
  char buffer[512];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  strcat(buffer, "\nRebooting now!");
  MessageBoxA(NULL,
	      buffer,
	      "ReactOS Setup",
	      MB_OK);
}


HRESULT CreateShellLink(LPCTSTR linkPath, LPCTSTR cmd, LPCTSTR arg, LPCTSTR dir, LPCTSTR iconPath, int icon_nr, LPCTSTR comment)
{
  IShellLink* psl;
  IPersistFile* ppf;
#ifndef _UNICODE
  WCHAR buffer[MAX_PATH];
#endif /* _UNICODE */

  HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (LPVOID*)&psl);

  if (SUCCEEDED(hr))
    {
      hr = psl->lpVtbl->SetPath(psl, cmd);

      if (arg)
        {
          hr = psl->lpVtbl->SetArguments(psl, arg);
        }

      if (dir)
        {
          hr = psl->lpVtbl->SetWorkingDirectory(psl, dir);
        }

      if (iconPath)
        {
          hr = psl->lpVtbl->SetIconLocation(psl, iconPath, icon_nr);
        }

      if (comment)
        {
          hr = psl->lpVtbl->SetDescription(psl, comment);
        }

      hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);

      if (SUCCEEDED(hr))
        {
#ifdef _UNICODE
          hr = ppf->lpVtbl->Save(ppf, linkPath, TRUE);
#else /* _UNICODE */
          MultiByteToWideChar(CP_ACP, 0, linkPath, -1, buffer, MAX_PATH);

          hr = ppf->lpVtbl->Save(ppf, buffer, TRUE);
#endif /* _UNICODE */

          ppf->lpVtbl->Release(ppf);
        }

      psl->lpVtbl->Release(psl);
    }

  return hr;
}


static BOOL
CreateShortcut(int csidl, LPCTSTR folder, UINT nIdName, LPCTSTR command, UINT nIdTitle)
{
  TCHAR path[MAX_PATH];
  TCHAR title[256];
  TCHAR name[256];
  LPTSTR p = path;

  if (!SHGetSpecialFolderPath(0, path, csidl, TRUE))
    return FALSE;

  if (folder)
    {
      p = PathAddBackslash(p);
      _tcscpy(p, folder);
    }

  p = PathAddBackslash(p);

  if(!LoadString(hDllInstance, nIdName, name, 256))
    return FALSE;
  _tcscpy(p, name);

  if (!LoadString(hDllInstance, nIdTitle, title, 256))
    return FALSE;

  return SUCCEEDED(CreateShellLink(path, command, _T(""), NULL, NULL, 0, title));
}


static BOOL
CreateShortcutFolder(int csidl, UINT nID, LPTSTR name, int nameLen)
{
  TCHAR path[MAX_PATH];
  LPTSTR p;

  if (!SHGetSpecialFolderPath(0, path, csidl, TRUE))
    return FALSE;

  if (!LoadString(hDllInstance, nID, name, nameLen))
    return FALSE;

  p = PathAddBackslash(path);
  _tcscpy(p, name);

  return CreateDirectory(path, NULL) || GetLastError()==ERROR_ALREADY_EXISTS;
}


static VOID
CreateRandomSid (PSID *Sid)
{
  SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
  LARGE_INTEGER SystemTime;
  PULONG Seed;

  NtQuerySystemTime (&SystemTime);
  Seed = &SystemTime.u.LowPart;

  RtlAllocateAndInitializeSid (&SystemAuthority,
			       4,
			       SECURITY_NT_NON_UNIQUE,
			       RtlUniform (Seed),
			       RtlUniform (Seed),
			       RtlUniform (Seed),
			       SECURITY_NULL_RID,
			       SECURITY_NULL_RID,
			       SECURITY_NULL_RID,
			       SECURITY_NULL_RID,
			       Sid);
}


static VOID
AppendRidToSid (PSID *Dst,
		PSID Src,
		ULONG NewRid)
{
  ULONG Rid[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  UCHAR RidCount;
  ULONG i;

  RidCount = *RtlSubAuthorityCountSid (Src);

  for (i = 0; i < RidCount; i++)
    Rid[i] = *RtlSubAuthoritySid (Src, i);

  if (RidCount < 8)
    {
      Rid[RidCount] = NewRid;
      RidCount++;
    }

  RtlAllocateAndInitializeSid (RtlIdentifierAuthoritySid (Src),
			       RidCount,
			       Rid[0],
			       Rid[1],
			       Rid[2],
			       Rid[3],
			       Rid[4],
			       Rid[5],
			       Rid[6],
			       Rid[7],
			       Dst);
}


static VOID
CreateTempDir(LPCWSTR VarName)
{
  TCHAR szTempDir[MAX_PATH];
  TCHAR szBuffer[MAX_PATH];
  DWORD dwLength;
  HKEY hKey;

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		   _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),
		   0,
		   KEY_ALL_ACCESS,
		   &hKey))
    {
      DebugPrint("Error: %lu\n", GetLastError());
      return;
    }

  /* Get temp dir */
  dwLength = MAX_PATH * sizeof(TCHAR);
  if (RegQueryValueEx(hKey,
		      VarName,
		      NULL,
		      NULL,
		      (LPBYTE)szBuffer,
		      &dwLength))
    {
      DebugPrint("Error: %lu\n", GetLastError());
      RegCloseKey(hKey);
      return;
    }

  /* Expand it */
  if (!ExpandEnvironmentStrings(szBuffer,
				szTempDir,
				MAX_PATH))
    {
      DebugPrint("Error: %lu\n", GetLastError());
      RegCloseKey(hKey);
      return;
    }

  /* Create profiles directory */
  if (!CreateDirectory(szTempDir, NULL))
    {
      if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
	  DebugPrint("Error: %lu\n", GetLastError());
	  RegCloseKey(hKey);
	  return;
	}
    }

  RegCloseKey(hKey);
}


BOOL
ProcessSysSetupInf(VOID)
{
  INFCONTEXT InfContext;
  TCHAR LineBuffer[256];
  DWORD LineLength;

  if (!SetupFindFirstLine(hSysSetupInf,
			  _T("DeviceInfsToInstall"),
			  NULL,
			  &InfContext))
  {
    return FALSE;
  }

  do
  {
    if (!SetupGetStringField(&InfContext,
			     0,
			     LineBuffer,
			     256,
			     &LineLength))
    {
      return FALSE;
    }

    if (!SetupDiInstallClass(NULL, LineBuffer, DI_QUIETINSTALL, NULL))
    {
      return FALSE;
    }
  }
  while (SetupFindNextLine(&InfContext, &InfContext));

  return TRUE;
}


static BOOL
EnableUserModePnpManager(VOID)
{
  SC_HANDLE hSCManager = NULL;
  SC_HANDLE hService = NULL;
  BOOL ret = FALSE;

  hSCManager = OpenSCManager(NULL, NULL, 0);
  if (hSCManager == NULL)
    goto cleanup;

  hService = OpenService(hSCManager, _T("PlugPlay"), SERVICE_CHANGE_CONFIG | SERVICE_START);
  if (hService == NULL)
    goto cleanup;

  ret = ChangeServiceConfig(
    hService,
    SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  if (!ret)
    goto cleanup;

  ret = StartService(hService, 0, NULL);
  if (!ret)
    goto cleanup;

  ret = TRUE;

cleanup:
  if (hSCManager != NULL)
    CloseServiceHandle(hSCManager);
  if (hService != NULL)
    CloseServiceHandle(hService);
  return ret;
}


DWORD STDCALL
InstallLiveCD (HINSTANCE hInstance)
{
  LONG rc;
  HKEY hKey = NULL;
  DWORD dwType;
  DWORD requiredSize;
  LPTSTR Shell = NULL;
  TCHAR CommandLine[MAX_PATH];
  STARTUPINFO StartupInfo;
  PROCESS_INFORMATION ProcessInformation;
  BOOL res;

  hSysSetupInf = SetupOpenInfFileW(
    L"syssetup.inf",
    NULL,
    INF_STYLE_WIN4,
    NULL);
  if (hSysSetupInf == INVALID_HANDLE_VALUE)
  {
    DebugPrint("SetupOpenInfFileW() failed to open 'syssetup.inf' (Error: %lu)\n", GetLastError());
    return 0;
  }

  if (!ProcessSysSetupInf())
  {
    DebugPrint("ProcessSysSetupInf() failed!\n");
    return 0;
  }

  SetupCloseInfFile(hSysSetupInf);

  if (!EnableUserModePnpManager())
  {
    DebugPrint("EnableUserModePnpManager() failed!\n");
    return 0;
  }

  /* Load the default shell */
  rc = RegOpenKeyEx(
    HKEY_LOCAL_MACHINE,
    TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), /* FIXME: should be REGSTR_PATH_WINLOGON */
    0,
    KEY_QUERY_VALUE,
    &hKey);
  if (rc != ERROR_SUCCESS)
    goto cleanup;
  rc = RegQueryValueEx(
    hKey,
    TEXT("Shell"),
    NULL,
    &dwType,
    NULL,
    &requiredSize);
  if (rc != ERROR_SUCCESS)
    goto cleanup;
  else if (dwType != REG_SZ && dwType != REG_EXPAND_SZ)
    goto cleanup;
  else if (requiredSize > (MAX_PATH - 1) * sizeof(TCHAR))
    goto cleanup;
  Shell = HeapAlloc(GetProcessHeap(), 0, requiredSize + sizeof(TCHAR));
  if (!Shell)
    goto cleanup;
  Shell[requiredSize / sizeof(WCHAR)] = '\0';
  rc = RegQueryValueEx(
    hKey,
    TEXT("Shell"),
    NULL,
    NULL,
    (LPBYTE)Shell,
    &requiredSize);
  if (rc != ERROR_SUCCESS)
    goto cleanup;
  if (dwType == REG_EXPAND_SZ)
    ExpandEnvironmentStrings(Shell, CommandLine, MAX_PATH);
  else if (dwType == REG_SZ)
    _tcscpy(CommandLine, Shell);

  /* Run the shell */
  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.lpReserved = NULL;
  StartupInfo.lpDesktop = NULL;
  StartupInfo.lpTitle = NULL;
  StartupInfo.dwFlags = 0;
  StartupInfo.cbReserved2 = 0;
  StartupInfo.lpReserved2 = 0;
  res = CreateProcess(
    CommandLine,
    NULL,
    NULL,
    NULL,
    FALSE,
    0,
    NULL,
    NULL,
    &StartupInfo,
    &ProcessInformation);
  if (!res)
    goto cleanup;

  /* Wait for process termination */
  WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

cleanup:
  if (hKey != NULL)
    RegCloseKey(hKey);
  HeapFree(GetProcessHeap(), 0, Shell);
  MessageBoxA(
    NULL,
    "You can shutdown your computer, or press ENTER to reboot",
    "ReactOS LiveCD",
    MB_OK);
  return 0;
}


DWORD STDCALL
InstallReactOS (HINSTANCE hInstance)
{
  TCHAR sAccessories[256];
  TCHAR sGames[256];
  TCHAR szBuffer[MAX_PATH];
  TCHAR Path[MAX_PATH];

# if 0
  OutputDebugStringA ("InstallReactOS() called\n");

  if (!InitializeSetupActionLog (FALSE))
    {
      OutputDebugStringA ("InitializeSetupActionLog() failed\n");
    }

  LogItem (SYSSETUP_SEVERITY_INFORMATION,
	   L"ReactOS Setup starting");

  LogItem (SYSSETUP_SEVERITY_FATAL_ERROR,
	   L"Buuuuuuaaaah!");

  LogItem (SYSSETUP_SEVERITY_INFORMATION,
	   L"ReactOS Setup finished");

  TerminateSetupActionLog ();
#endif
#if 0
  UNICODE_STRING SidString;
#endif
  ULONG LastError;

  if (!InitializeProfiles ())
    {
      DebugPrint ("InitializeProfiles() failed\n");
      return 0;
    }

  CoInitialize(NULL);
  SetUserDefaultLCID(GetSystemDefaultLCID());
  SetThreadLocale(GetSystemDefaultLCID());

  /* create desktop shortcuts */
  CreateShortcut(CSIDL_DESKTOP, NULL, IDS_SHORT_CMD, _T("cmd.exe"), IDS_CMT_CMD);

  /* create program startmenu shortcuts */
  CreateShortcut(CSIDL_PROGRAMS, NULL, IDS_SHORT_EXPLORER, _T("explorer.exe"), IDS_CMT_EXPLORER);
  /* workaround to stop empty links for trunk builds */
  if(GetSystemDirectory(szBuffer, MAX_PATH)) 
  {
    _tcscpy(Path, szBuffer);
    if((_taccess(_tcscat(Path, _T("\\downloader.exe")), 0 )) != -1)
        CreateShortcut(CSIDL_PROGRAMS, NULL, IDS_SHORT_DOWNLOADER, _T("downloader.exe"), IDS_CMT_DOWNLOADER);

    _tcscpy(Path, szBuffer);
    if((_taccess(_tcscat(Path, _T("\\getfirefox.exe")), 0 )) != -1)
      CreateShortcut(CSIDL_PROGRAMS, NULL, IDS_SHORT_FIREFOX, _T("getfirefox.exe"), IDS_CMT_GETFIREFOX);
  }


  /* create administritive tools startmenu shortcuts */
  CreateShortcut(CSIDL_COMMON_ADMINTOOLS, NULL, IDS_SHORT_SERVICE, _T("servman.exe"), IDS_CMT_SERVMAN);
  CreateShortcut(CSIDL_COMMON_ADMINTOOLS, NULL, IDS_SHORT_DEVICE, _T("devmgmt.exe"), IDS_CMT_DEVMGMT);

  /* create and fill Accessories subfolder */
  if (CreateShortcutFolder(CSIDL_PROGRAMS, IDS_ACCESSORIES, sAccessories, 256)) 
  {
	CreateShortcut(CSIDL_PROGRAMS, sAccessories, IDS_SHORT_CALC, _T("calc.exe"), IDS_CMT_CALC);
	CreateShortcut(CSIDL_PROGRAMS, sAccessories, IDS_SHORT_CMD, _T("cmd.exe"), IDS_CMT_CMD);
    CreateShortcut(CSIDL_PROGRAMS, sAccessories, IDS_SHORT_NOTEPAD, _T("notepad.exe"), IDS_CMT_NOTEPAD);
    CreateShortcut(CSIDL_PROGRAMS, sAccessories, IDS_SHORT_REGEDIT, _T("regedit.exe"), IDS_CMT_REGEDIT);
    CreateShortcut(CSIDL_PROGRAMS, sAccessories, IDS_SHORT_WORDPAD, _T("wordpad.exe"), IDS_CMT_WORDPAD);
    if(GetSystemDirectory(szBuffer, MAX_PATH)) 
    {
      _tcscpy(Path, szBuffer);
      if((_taccess(_tcscat(Path, _T("\\screenshot.exe")), 0 )) != -1)
        CreateShortcut(CSIDL_PROGRAMS, sAccessories, IDS_SHORT_SNAP, _T("screenshot.exe"), IDS_CMT_SCREENSHOT);
    }
  }

  /* create Games subfolder and fill if the exe is available */
  if (CreateShortcutFolder(CSIDL_PROGRAMS, IDS_GAMES, sGames, 256)) 
  {
    CreateShortcut(CSIDL_PROGRAMS, sGames, IDS_SHORT_SOLITAIRE, _T("sol.exe"), IDS_CMT_SOLITAIRE);
    CreateShortcut(CSIDL_PROGRAMS, sGames, IDS_SHORT_WINEMINE, _T("winemine.exe"), IDS_CMT_WINEMINE);
  }

  CoUninitialize();

  /* Create the semi-random Domain-SID */
  CreateRandomSid (&DomainSid);
  if (DomainSid == NULL)
    {
      DebugPrint ("Domain-SID creation failed!\n");
      return 0;
    }

#if 0
  RtlConvertSidToUnicodeString (&SidString, DomainSid, TRUE);
  DebugPrint ("Domain-SID: %wZ\n", &SidString);
  RtlFreeUnicodeString (&SidString);
#endif

  /* Initialize the Security Account Manager (SAM) */
  if (!SamInitializeSAM ())
    {
      DebugPrint ("SamInitializeSAM() failed!\n");
      RtlFreeSid (DomainSid);
      return 0;
    }

  /* Set the Domain SID (aka Computer SID) */
  if (!SamSetDomainSid (DomainSid))
    {
      DebugPrint ("SamSetDomainSid() failed!\n");
      RtlFreeSid (DomainSid);
      return 0;
    }

  /* Append the Admin-RID */
  AppendRidToSid(&AdminSid, DomainSid, DOMAIN_USER_RID_ADMIN);

#if 0
  RtlConvertSidToUnicodeString (&SidString, DomainSid, TRUE);
  DebugPrint ("Admin-SID: %wZ\n", &SidString);
  RtlFreeUnicodeString (&SidString);
#endif

  /* Create the Administrator account */
  if (!SamCreateUser(L"Administrator", L"", AdminSid))
  {
    /* Check what the error was.
     * If the Admin Account already exists, then it means Setup
     * wasn't allowed to finish properly. Instead of rebooting
     * and not completing it, let it restart instead
     */
    LastError = GetLastError();
    if (LastError != ERROR_USER_EXISTS)
    {
      DebugPrint("SamCreateUser() failed!\n");
      RtlFreeSid(AdminSid);
      RtlFreeSid(DomainSid);
      return 0;
    }
  }

  /* Create the Administrator profile */
  if (!CreateUserProfileW(AdminSid, L"Administrator"))
  {
    DebugPrint("CreateUserProfileW() failed!\n");
    RtlFreeSid(AdminSid);
    RtlFreeSid(DomainSid);
    return 0;
  }

  RtlFreeSid(AdminSid);
  RtlFreeSid(DomainSid);

  CreateTempDir(L"TEMP");
  CreateTempDir(L"TMP");

  if (GetWindowsDirectory(szBuffer, sizeof(szBuffer) / sizeof(TCHAR)))
  {
    PathAddBackslash(szBuffer);
    _tcscat(szBuffer, _T("system"));
    CreateDirectory(szBuffer, NULL);
  }

  hSysSetupInf = SetupOpenInfFileW(L"syssetup.inf",
				   NULL,
				   INF_STYLE_WIN4,
				   NULL);
  if (hSysSetupInf == INVALID_HANDLE_VALUE)
  {
    DebugPrint("SetupOpenInfFileW() failed to open 'syssetup.inf' (Error: %lu)\n", GetLastError());
    return 0;
  }

  if (!ProcessSysSetupInf())
  {
    DebugPrint("ProcessSysSetupInf() failed!\n");
    return 0;
  }

  if (!EnableUserModePnpManager())
  {
    DebugPrint("EnableUserModePnpManager() failed!\n");
    return 0;
  }

  if (CMP_WaitNoPendingInstallEvents(INFINITE) != WAIT_OBJECT_0)
  {
    DebugPrint("CMP_WaitNoPendingInstallEvents() failed!\n");
    return 0;
  }

  InstallWizard();

  SetupCloseInfFile(hSysSetupInf);

  /// THE FOLLOWING DPRINT IS FOR THE SYSTEM REGRESSION TOOL
  /// DO NOT REMOVE!!!
  DbgPrint("SYSREG_CHECKPOINT:SYSSETUP_COMPLETE\n");

  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
SetupChangeFontSize(HANDLE hWnd,
                    LPCWSTR lpszFontSize)
{
  return FALSE;
}
