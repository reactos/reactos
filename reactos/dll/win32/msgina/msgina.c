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

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(msgina);

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

	if (!pValue)
		return ERROR_INVALID_PARAMETER;

	*pValue = NULL;
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
		if (wcsicmp(CurrentOption, L"CONSOLE") == 0)
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

	UNREFERENCED_PARAMETER(pvReserved);

	pgContext = (PGINA_CONTEXT)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(GINA_CONTEXT));
	if(!pgContext)
	{
		WARN("LocalAlloc() failed\n");
		return FALSE;
	}

	/* Return the context to winlogon */
	*pWlxContext = (PVOID)pgContext;
	pgContext->hDllInstance = hDllInstance;

	/* Save pointer to dispatch table */
	pgContext->pWlxFuncs = (PWLX_DISPATCH_VERSION_1_3)pWinlogonFunctions;

	/* Save the winlogon handle used to call the dispatch functions */
	pgContext->hWlx = hWlx;

	/* Save window station */
	pgContext->station = lpWinsta;

	/* Clear status window handle */
	pgContext->hStatusWindow = 0;

	/* Notify winlogon that we will use the default SAS */
	pgContext->pWlxFuncs->WlxUseCtrlAltDel(hWlx);

	/* Locates the authentification package */
	//LsaRegisterLogonProcess(...);

	/* Check autologon settings the first time */
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
	HANDLE hAppToken;
	UINT len;
	BOOL ret;

	len = GetWindowsDirectoryW(CurrentDirectory, MAX_PATH);
	if (len == 0 || len > MAX_PATH)
	{
		WARN("GetWindowsDirectoryW() failed\n");
		return FALSE;
	}

	ret = DuplicateTokenEx(pgContext->UserToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &hAppToken);
	if (!ret)
	{
		WARN("DuplicateTokenEx() failed with error %lu\n", GetLastError());
		return FALSE;
	}

	ZeroMemory(&StartupInfo, sizeof(STARTUPINFOW));
	ZeroMemory(&ProcessInformation, sizeof(PROCESS_INFORMATION));
	StartupInfo.cb = sizeof(STARTUPINFOW);
	StartupInfo.lpTitle = pszCmdLine;
	StartupInfo.dwX = StartupInfo.dwY = StartupInfo.dwXSize = StartupInfo.dwYSize = 0L;
	StartupInfo.dwFlags = 0;
	StartupInfo.wShowWindow = SW_SHOW;
	StartupInfo.lpDesktop = pszDesktopName;

	len = GetWindowsDirectoryW(CurrentDirectory, MAX_PATH);
	if (len == 0 || len > MAX_PATH)
	{
		WARN("GetWindowsDirectoryW() failed\n");
		return FALSE;
	}
	ret = CreateProcessAsUserW(
		hAppToken,
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
	CloseHandle(hAppToken);
	if (!ret)
		WARN("CreateProcessAsUserW() failed with error %lu\n", GetLastError());
	return ret;
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
	WCHAR pszUserInitApp[MAX_PATH + 1];
	WCHAR pszExpUserInitApp[MAX_PATH];
	DWORD len;
	LONG rc;

	TRACE("WlxActivateUserShell()\n");

	UNREFERENCED_PARAMETER(pszMprLogonScript);

	/* Get the path of userinit */
	rc = RegOpenKeyExW(
		HKEY_LOCAL_MACHINE, 
		L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
		0,
		KEY_QUERY_VALUE,
		&hKey);
	if (rc != ERROR_SUCCESS)
	{
		WARN("RegOpenKeyExW() failed with error %lu\n", rc);
		return FALSE;
	}

	/* Query userinit application */
	BufSize = sizeof(pszUserInitApp) - sizeof(UNICODE_NULL);
	rc = RegQueryValueExW(
		hKey,
		L"Userinit",
		NULL,
		&ValueType,
		(LPBYTE)pszUserInitApp,
		&BufSize);
	RegCloseKey(hKey);
	if (rc != ERROR_SUCCESS || (ValueType != REG_SZ && ValueType != REG_EXPAND_SZ))
	{
		WARN("RegQueryValueExW() failed with error %lu\n", rc);
		return FALSE;
	}
	pszUserInitApp[MAX_PATH] = UNICODE_NULL;

	len = ExpandEnvironmentStringsW(pszUserInitApp, pszExpUserInitApp, MAX_PATH);
	if (len > MAX_PATH)
	{
		WARN("ExpandEnvironmentStringsW() failed. Required size %lu\n", len);
		return FALSE;
	}

	/* Start userinit app */
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

	UNREFERENCED_PARAMETER(pReserved);

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
	LPWSTR ProfilePath = NULL;
	TOKEN_STATISTICS Stats;
	PWLX_PROFILE_V1_0 pProfile = NULL;
	DWORD cbStats, cbSize;
	BOOL bResult;

	if (!LogonUserW(UserName, Domain, Password,
		LOGON32_LOGON_INTERACTIVE,
		LOGON32_PROVIDER_DEFAULT,
		&pgContext->UserToken))
	{
		WARN("LogonUserW() failed\n");
		goto cleanup;
	}

	/* Get profile path */
	cbSize = 0;
	bResult = GetProfilesDirectoryW(NULL, &cbSize);
	if (!bResult && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		ProfilePath = HeapAlloc(GetProcessHeap(), 0, cbSize * sizeof(WCHAR));
		if (!ProfilePath)
		{
			WARN("HeapAlloc() failed\n");
			goto cleanup;
		}
		bResult = GetProfilesDirectoryW(ProfilePath, &cbSize);
	}
	if (!bResult)
	{
		WARN("GetUserProfileDirectoryW() failed\n");
		goto cleanup;
	}

	/* Allocate memory for profile */
	pProfile = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WLX_PROFILE_V1_0));
	if (!pProfile)
	{
		WARN("HeapAlloc() failed\n");
		goto cleanup;
	}
	pProfile->dwType = WLX_PROFILE_TYPE_V1_0;
	pProfile->pszProfile = ProfilePath;

	if (!GetTokenInformation(pgContext->UserToken,
		TokenStatistics,
		(PVOID)&Stats,
		sizeof(TOKEN_STATISTICS),
		&cbStats))
	{
		WARN("Couldn't get Authentication id from user token!\n");
		goto cleanup;
	}
	*pgContext->pAuthenticationId = Stats.AuthenticationId; 
	pgContext->pMprNotifyInfo->pszUserName = DuplicationString(UserName);
	pgContext->pMprNotifyInfo->pszDomain = DuplicationString(Domain);
	pgContext->pMprNotifyInfo->pszPassword = DuplicationString(Password);
	pgContext->pMprNotifyInfo->pszOldPassword = NULL;
	*pgContext->pdwOptions = 0;
	*pgContext->pProfile = pProfile;
	return TRUE;

cleanup:
	HeapFree(GetProcessHeap(), 0, pProfile);
	HeapFree(GetProcessHeap(), 0, ProfilePath);
	return FALSE;
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
	OUT PWLX_MPR_NOTIFY_INFO pMprNotifyInfo,
	OUT PVOID *pProfile)
{
	PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
	INT res;

	TRACE("WlxLoggedOutSAS()\n");

	UNREFERENCED_PARAMETER(dwSasType);
	UNREFERENCED_PARAMETER(pLogonSid);

	pgContext->pAuthenticationId = pAuthenticationId;
	pgContext->pdwOptions = pdwOptions;
	pgContext->pMprNotifyInfo = pMprNotifyInfo;
	pgContext->pProfile = pProfile;

	if (0 == GetSystemMetrics(SM_REMOTESESSION) &&
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

	UNREFERENCED_PARAMETER(dwSasType);

	return pGinaUI->LockedSAS(pgContext);
}

BOOL WINAPI
DllMain(
	IN HINSTANCE hinstDLL,
	IN DWORD dwReason,
	IN LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
		hDllInstance = hinstDLL;

	return TRUE;
}
