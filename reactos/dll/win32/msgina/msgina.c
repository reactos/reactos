/*
 *  ReactOS GINA
 *  Copyright (C) 2003-2004, 2006 ReactOS Team
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
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/msgina.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

#include "msgina.h"

//#define YDEBUG
#include <wine/debug.h>

extern HINSTANCE hDllInstance;

extern GINA_UI GinaGraphicalUI;
extern GINA_UI GinaTextUI;
static PGINA_UI pGinaUI;

/*
 * @implemented
 */
BOOL WINAPI
WlxNegotiate(
	IN DWORD dwWinlogonVersion,
	OUT PDWORD pdwDllVersion)
{
	TRACE("WlxNegotiate(%lx, %p)\n", dwWinlogonVersion, pdwDllVersion);

	if(!pdwDllVersion || (dwWinlogonVersion < WLX_VERSION_1_3))
		return FALSE;

	*pdwDllVersion = WLX_VERSION_1_3;

	return TRUE;
}

static LONG
ReadRegSzKey(
	IN HKEY hKey,
	IN LPCWSTR pszKey,
	OUT LPWSTR* pValue)
{
	LONG rc;
	DWORD dwType;
	DWORD cbData = 0;
	LPWSTR Value;

	rc = RegQueryValueExW(hKey, pszKey, NULL, &dwType, NULL, &cbData);
	if (rc != ERROR_SUCCESS)
		return rc;
	if (dwType != REG_SZ)
		return ERROR_FILE_NOT_FOUND;
	Value = HeapAlloc(GetProcessHeap(), 0, cbData + sizeof(WCHAR));
	if (!Value)
		return ERROR_NOT_ENOUGH_MEMORY;
	rc = RegQueryValueExW(hKey, pszKey, NULL, NULL, (LPBYTE)Value, &cbData);
	if (rc != ERROR_SUCCESS)
	{
		HeapFree(GetProcessHeap(), 0, Value);
		return rc;
	}
	/* NULL-terminate the string */
	Value[cbData / sizeof(WCHAR)] = '\0';

	*pValue = Value;
	return ERROR_SUCCESS;
}

static VOID
ChooseGinaUI(VOID)
{
	HKEY ControlKey = NULL;
	LPWSTR SystemStartOptions = NULL;
	LPWSTR CurrentOption, NextOption; /* Pointers into SystemStartOptions */
	BOOL ConsoleBoot = FALSE;
	LONG rc;

	rc = RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control",
		0,
		KEY_QUERY_VALUE,
		&ControlKey);

	rc = ReadRegSzKey(ControlKey, L"SystemStartOptions", &SystemStartOptions);
	if (rc != ERROR_SUCCESS)
		goto cleanup;

	/* Check for CMDCONS in SystemStartOptions */
	CurrentOption = SystemStartOptions;
	while (CurrentOption)
	{
		NextOption = wcschr(CurrentOption, L' ');
		if (NextOption)
			*NextOption = L'\0';
		if (wcsicmp(CurrentOption, L"CMDCONS") == 0)
		{
			TRACE("Found %S. Switching to console boot\n", CurrentOption);
			ConsoleBoot = TRUE;
			goto cleanup;
		}
		CurrentOption = NextOption ? NextOption + 1 : NULL;
	}

cleanup:
	if (ConsoleBoot)
		pGinaUI = &GinaTextUI;
	else
		pGinaUI = &GinaGraphicalUI;

	if (ControlKey != NULL)
		RegCloseKey(ControlKey);
	HeapFree(GetProcessHeap(), 0, SystemStartOptions);
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
  pgContext->pWlxFuncs = (PWLX_DISPATCH_VERSION_1_3)pWinlogonFunctions;
  
  /* save the winlogon handle used to call the dispatch functions */
  pgContext->hWlx = hWlx;
  
  /* save window station */
  pgContext->station = lpWinsta;
  
  /* clear status window handle */
  pgContext->hStatusWindow = 0;
  
  /* notify winlogon that we will use the default SAS */
  pgContext->pWlxFuncs->WlxUseCtrlAltDel(hWlx);
  
  /* Locates the authentification package */
  //LsaRegisterLogonProcess(...);

  pgContext->AutoLogonState = AUTOLOGON_CHECK_REGISTRY;

  ChooseGinaUI();
  return pGinaUI->Initialize(pgContext);
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
  STARTUPINFOW StartupInfo;
  PROCESS_INFORMATION ProcessInformation;
  WCHAR CurrentDirectory[MAX_PATH];
  BOOL Ret;
  
  StartupInfo.cb = sizeof(STARTUPINFOW);
  StartupInfo.lpReserved = NULL;
  StartupInfo.lpTitle = pszCmdLine;
  StartupInfo.dwX = StartupInfo.dwY = StartupInfo.dwXSize = StartupInfo.dwYSize = 0L;
  StartupInfo.dwFlags = 0;
  StartupInfo.wShowWindow = SW_SHOW;  
  StartupInfo.lpReserved2 = NULL;
  StartupInfo.cbReserved2 = 0;
  StartupInfo.lpDesktop = pszDesktopName;
  
  GetWindowsDirectoryW (CurrentDirectory, MAX_PATH);
  Ret = CreateProcessAsUserW(pgContext->UserToken,
                            pszCmdLine,
                            NULL,
                            NULL,
                            NULL,
                            FALSE,
                            CREATE_UNICODE_ENVIRONMENT,
                            pEnvironment,
                            CurrentDirectory,
                            &StartupInfo,
                            &ProcessInformation);
  
  //VirtualFree(pEnvironment, 0, MEM_RELEASE);
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
  HKEY hKey;
  DWORD BufSize, ValueType;
  WCHAR pszUserInitApp[MAX_PATH];
  WCHAR pszExpUserInitApp[MAX_PATH];
  TRACE("WlxActivateUserShell()\n");
  /* get the path of userinit */
  if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", 
                  0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
  {ERR("GINA: Failed: 1\n");
    VirtualFree(pEnvironment, 0, MEM_RELEASE);
    return FALSE;
  }
  BufSize = MAX_PATH * sizeof(WCHAR);
  if((RegQueryValueExW(hKey, L"Userinit", NULL, &ValueType, (LPBYTE)pszUserInitApp, 
                     &BufSize) != ERROR_SUCCESS) || 
                     !((ValueType == REG_SZ) || (ValueType == REG_EXPAND_SZ)))
  {ERR("GINA: Failed: 2\n");
    RegCloseKey(hKey);
    VirtualFree(pEnvironment, 0, MEM_RELEASE);
    return FALSE;
  }
  RegCloseKey(hKey);
  ExpandEnvironmentStringsW(pszUserInitApp, pszExpUserInitApp, MAX_PATH);
  
  /* Start userinit */
  /* FIXME - allow to start more applications that are comma-separated */
  /* FIXME: Call VirtualFree(pEnvironment, 0, MEM_RELEASE); ? */
  return WlxStartApplication(pWlxContext, pszDesktopName, pEnvironment, pszExpUserInitApp);
}


/*
 * @implemented
 */
int WINAPI
WlxLoggedOnSAS(
	PVOID pWlxContext,
	DWORD dwSasType,
	PVOID pReserved)
{
	PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
	INT SasAction = WLX_SAS_ACTION_NONE;

	TRACE("WlxLoggedOnSAS(0x%lx)\n", dwSasType);

	switch (dwSasType)
	{
		case WLX_SAS_TYPE_CTRL_ALT_DEL:
		case WLX_SAS_TYPE_TIMEOUT:
		{
			SasAction = pGinaUI->LoggedOnSAS(pgContext, dwSasType);
			break;
		}
		case WLX_SAS_TYPE_SC_INSERT:
		{
			FIXME("WlxLoggedOnSAS: SasType WLX_SAS_TYPE_SC_INSERT not supported!\n");
			break;
		}
		case WLX_SAS_TYPE_SC_REMOVE:
		{
			FIXME("WlxLoggedOnSAS: SasType WLX_SAS_TYPE_SC_REMOVE not supported!\n");
			break;
		}
		default:
		{
			WARN("WlxLoggedOnSAS: Unknown SasType: 0x%x\n", dwSasType);
			break;
		}
	}

	return SasAction;
}

/*
 * @implemented
 */
BOOL WINAPI
WlxDisplayStatusMessage(
	IN PVOID pWlxContext,
	IN HDESK hDesktop,
	IN DWORD dwOptions,
	IN PWSTR pTitle,
	IN PWSTR pMessage)
{
	PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;

	TRACE("WlxDisplayStatusMessage(\"%S\")\n", pMessage);

	return pGinaUI->DisplayStatusMessage(pgContext, hDesktop, dwOptions, pTitle, pMessage);
}

/*
 * @implemented
 */
BOOL WINAPI
WlxRemoveStatusMessage(
	IN PVOID pWlxContext)
{
	PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;

	TRACE("WlxRemoveStatusMessage()\n");

	return pGinaUI->RemoveStatusMessage(pgContext);
}

static PWSTR
DuplicationString(PWSTR Str)
{
	DWORD cb;
	PWSTR NewStr;

	if (Str == NULL) return NULL;

	cb = (wcslen(Str) + 1) * sizeof(WCHAR);
	if ((NewStr = LocalAlloc(LMEM_FIXED, cb)))
		memcpy(NewStr, Str, cb);
	return NewStr;
}

BOOL
DoLoginTasks(
	IN OUT PGINA_CONTEXT pgContext,
	IN PWSTR UserName,
	IN PWSTR Domain,
	IN PWSTR Password)
{
	TOKEN_STATISTICS Stats;
	DWORD cbStats;

	if (!LogonUserW(UserName, Domain, Password,
		LOGON32_LOGON_INTERACTIVE, /* FIXME - use LOGON32_LOGON_UNLOCK instead! */
		LOGON32_PROVIDER_DEFAULT,
		&pgContext->UserToken))
	{
		WARN("LogonUserW() failed\n");
		return FALSE;
	}

	if (!pgContext->UserToken)
	{
		WARN("UserToken == NULL!\n");
		return FALSE;
	}

	*pgContext->pdwOptions = 0;
	*pgContext->pProfile = NULL;

	if (!GetTokenInformation(pgContext->UserToken,
		TokenStatistics,
		(PVOID)&Stats,
		sizeof(TOKEN_STATISTICS),
		&cbStats))
	{
		WARN("Couldn't get Authentication id from user token!\n");
		return FALSE;
	}
	*pgContext->pAuthenticationId = Stats.AuthenticationId; 
	pgContext->pNprNotifyInfo->pszUserName = DuplicationString(UserName);
	pgContext->pNprNotifyInfo->pszDomain = DuplicationString(Domain);
	pgContext->pNprNotifyInfo->pszPassword = DuplicationString(Password);
	pgContext->pNprNotifyInfo->pszOldPassword = NULL;
	return TRUE;
}

static BOOL
DoAutoLogon(
	IN PGINA_CONTEXT pgContext)
{
	HKEY WinLogonKey = NULL;
	LPWSTR AutoLogon = NULL;
	LPWSTR AutoCount = NULL;
	LPWSTR IgnoreShiftOverride = NULL;
	LPWSTR UserName = NULL;
	LPWSTR DomainName = NULL;
	LPWSTR Password = NULL;
	BOOL result = FALSE;
	LONG rc;

	TRACE("DoAutoLogon(): AutoLogonState = %lu\n",
		pgContext->AutoLogonState);

	if (pgContext->AutoLogonState == AUTOLOGON_DISABLED)
		return FALSE;

	rc = RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon",
		0,
		KEY_QUERY_VALUE,
		&WinLogonKey);
	if (rc != ERROR_SUCCESS)
		goto cleanup;

	if (pgContext->AutoLogonState == AUTOLOGON_CHECK_REGISTRY)
	{
		/* Set it by default to disabled, we might reenable it again later */
		pgContext->AutoLogonState = AUTOLOGON_DISABLED;

		rc = ReadRegSzKey(WinLogonKey, L"AutoAdminLogon", &AutoLogon);
		if (rc != ERROR_SUCCESS)
			goto cleanup;
		if (wcscmp(AutoLogon, L"1") != 0)
			goto cleanup;

		rc = ReadRegSzKey(WinLogonKey, L"AutoLogonCount", &AutoCount);
		if (rc == ERROR_SUCCESS && wcscmp(AutoCount, L"0") == 0)
			goto cleanup;
		else if (rc != ERROR_FILE_NOT_FOUND)
			goto cleanup;

		rc = ReadRegSzKey(WinLogonKey, L"IgnoreShiftOverride", &UserName);
		if (rc == ERROR_SUCCESS)
		{
			if (wcscmp(AutoLogon, L"1") != 0 && GetKeyState(VK_SHIFT) < 0)
				goto cleanup;
		}
		else if (GetKeyState(VK_SHIFT) < 0)
		{
			/* User pressed SHIFT */
			goto cleanup;
		}

		pgContext->AutoLogonState = AUTOLOGON_ONCE;
		result = TRUE;
	}
	else /* pgContext->AutoLogonState == AUTOLOGON_ONCE */
	{
		pgContext->AutoLogonState = AUTOLOGON_DISABLED;

		rc = ReadRegSzKey(WinLogonKey, L"DefaultUserName", &UserName);
		if (rc != ERROR_SUCCESS)
			goto cleanup;
		rc = ReadRegSzKey(WinLogonKey, L"DefaultDomainName", &DomainName);
		if (rc != ERROR_SUCCESS && rc != ERROR_FILE_NOT_FOUND)
			goto cleanup;
		rc = ReadRegSzKey(WinLogonKey, L"DefaultPassword", &Password);
		if (rc != ERROR_SUCCESS)
			goto cleanup;

		result = DoLoginTasks(pgContext, UserName, DomainName, Password);
	}

cleanup:
	if (WinLogonKey != NULL)
		RegCloseKey(WinLogonKey);
	HeapFree(GetProcessHeap(), 0, AutoLogon);
	HeapFree(GetProcessHeap(), 0, AutoCount);
	HeapFree(GetProcessHeap(), 0, IgnoreShiftOverride);
	HeapFree(GetProcessHeap(), 0, UserName);
	HeapFree(GetProcessHeap(), 0, DomainName);
	HeapFree(GetProcessHeap(), 0, Password);
	TRACE("DoAutoLogon(): AutoLogonState = %lu, returning %d\n",
		pgContext->AutoLogonState, result);
	return result;
}

/*
 * @implemented
 */
VOID WINAPI
WlxDisplaySASNotice(
	IN PVOID pWlxContext)
{
	PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;

	TRACE("WlxDisplaySASNotice(%p)\n", pWlxContext);

	if (GetSystemMetrics(SM_REMOTESESSION))
	{
		/* User is remotely logged on. Don't display a notice */
		pgContext->pWlxFuncs->WlxSasNotify(pgContext->hWlx, WLX_SAS_TYPE_CTRL_ALT_DEL);
		return;
	}

	if (DoAutoLogon(pgContext))
	{
		/* Don't display the window, we want to do an automatic logon */
		pgContext->AutoLogonState = AUTOLOGON_ONCE;
		pgContext->pWlxFuncs->WlxSasNotify(pgContext->hWlx, WLX_SAS_TYPE_CTRL_ALT_DEL);
		return;
	}
	else
		pgContext->AutoLogonState = AUTOLOGON_DISABLED;

	pGinaUI->DisplaySASNotice(pgContext);

	TRACE("WlxDisplaySASNotice() done\n");
}

/*
 * @implemented
 */
INT WINAPI
WlxLoggedOutSAS(
	IN PVOID pWlxContext,
	IN DWORD dwSasType,
	OUT PLUID pAuthenticationId,
	IN OUT PSID pLogonSid,
	OUT PDWORD pdwOptions,
	OUT PHANDLE phToken,
	OUT PWLX_MPR_NOTIFY_INFO pNprNotifyInfo,
	OUT PVOID *pProfile)
{
	PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
	INT res;

	TRACE("WlxLoggedOutSAS()\n");

	pgContext->pAuthenticationId = pAuthenticationId;
	pgContext->pdwOptions = pdwOptions;
	pgContext->pNprNotifyInfo = pNprNotifyInfo;
	pgContext->pProfile = pProfile;

	if (!GetSystemMetrics(SM_REMOTESESSION) &&
	    DoAutoLogon(pgContext))
	{
		/* User is local and registry contains information
		 * to log on him automatically */
		*phToken = pgContext->UserToken;
		return WLX_SAS_ACTION_LOGON;
	}

	res = pGinaUI->LoggedOutSAS(pgContext);
	*phToken = pgContext->UserToken;
	return res;
}

/*
 * @implemented
 */
int WINAPI
WlxWkstaLockedSAS(
	PVOID pWlxContext,
	DWORD dwSasType)
{
	PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;

	TRACE("WlxWkstaLockedSAS()\n");

	return pGinaUI->LockedSAS(pgContext);
}

BOOL WINAPI
DllMain(
	IN HINSTANCE hinstDLL,
	IN DWORD dwReason,
	IN LPVOID lpvReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		hDllInstance = hinstDLL;

	return TRUE;
}
