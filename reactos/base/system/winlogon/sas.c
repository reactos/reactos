/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/sas.c
 * PURPOSE:         Secure Attention Sequence
 * PROGRAMMERS:     Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 * UPDATE HISTORY:
 *                  Created 28/03/2004
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"

//#define YDEBUG
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(winlogon);

/* GLOBALS ******************************************************************/

#define WINLOGON_SAS_CLASS L"SAS Window class"
#define WINLOGON_SAS_TITLE L"SAS window"

#define HK_CTRL_ALT_DEL   0
#define HK_CTRL_SHIFT_ESC 1

#ifdef __USE_W32API
extern BOOL STDCALL SetLogonNotifyWindow(HWND Wnd, HWINSTA WinSta);
#endif

/* FUNCTIONS ****************************************************************/

static BOOL
StartTaskManager(
	IN OUT PWLSESSION Session)
{
	LPVOID lpEnvironment;
	BOOL ret;

	if (!Session->Gina.Functions.WlxStartApplication)
		return FALSE;

	if (!CreateEnvironmentBlock(
		&lpEnvironment,
		Session->UserToken,
		TRUE))
	{
		return FALSE;
	}

	ret = Session->Gina.Functions.WlxStartApplication(
		Session->Gina.Context,
		L"Default",
		lpEnvironment,
		L"taskmgr.exe");

	DestroyEnvironmentBlock(lpEnvironment);
	return ret;
}

BOOL
SetDefaultLanguage(
	IN BOOL UserProfile)
{
	HKEY BaseKey;
	LPCWSTR SubKey;
	LPCWSTR ValueName;
	LONG rc;
	HKEY hKey = NULL;
	DWORD dwType, dwSize;
	LPWSTR Value = NULL;
	UNICODE_STRING ValueString;
	NTSTATUS Status;
	LCID Lcid;
	BOOL ret = FALSE;

	if (UserProfile)
	{
		BaseKey = HKEY_CURRENT_USER;
		SubKey = L"Control Panel\\International";
		ValueName = L"Locale";
	}
	else
	{
		BaseKey = HKEY_LOCAL_MACHINE;
		SubKey = L"System\\CurrentControlSet\\Control\\Nls\\Language";
		ValueName = L"Default";
	}

	rc = RegOpenKeyExW(
		BaseKey,
		SubKey,
		0,
		KEY_READ,
		&hKey);
	if (rc != ERROR_SUCCESS)
	{
		TRACE("RegOpenKeyEx() failed with error %lu\n", rc);
		goto cleanup;
	}
	rc = RegQueryValueExW(
		hKey,
		ValueName,
		NULL,
		&dwType,
		NULL,
		&dwSize);
	if (rc != ERROR_SUCCESS)
	{
		TRACE("RegQueryValueEx() failed with error %lu\n", rc);
		goto cleanup;
	}
	else if (dwType != REG_SZ)
	{
		TRACE("Wrong type for %S\\%S registry entry (got 0x%lx, expected 0x%lx)\n",
			SubKey, ValueName, dwType, REG_SZ);
		goto cleanup;
	}

	Value = HeapAlloc(GetProcessHeap(), 0, dwSize);
	if (!Value)
	{
		TRACE("HeapAlloc() failed\n");
		goto cleanup;
	}
	rc = RegQueryValueExW(
		hKey,
		ValueName,
		NULL,
		NULL,
		(LPBYTE)Value,
		&dwSize);
	if (rc != ERROR_SUCCESS)
	{
		TRACE("RegQueryValueEx() failed with error %lu\n", rc);
		goto cleanup;
	}

	/* Convert Value to a Lcid */
	ValueString.Length = ValueString.MaximumLength = (USHORT)dwSize;
	ValueString.Buffer = Value;
	Status = RtlUnicodeStringToInteger(&ValueString, 16, (PULONG)&Lcid);
	if (!NT_SUCCESS(Status))
	{
		TRACE("RtlUnicodeStringToInteger() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	TRACE("%s language is 0x%08lx\n",
		UserProfile ? "User" : "System", Lcid);
	Status = NtSetDefaultLocale(UserProfile, Lcid);
	if (!NT_SUCCESS(Status))
	{
		TRACE("NtSetDefaultLocale() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	ret = TRUE;

cleanup:
	if (hKey)
		RegCloseKey(hKey);
	if (Value)
		HeapFree(GetProcessHeap(), 0, Value);
	return ret;
}

static BOOL
HandleLogon(
	IN OUT PWLSESSION Session)
{
	PROFILEINFOW ProfileInfo;
	LPVOID lpEnvironment = NULL;
	BOOLEAN Old;
	BOOL ret = FALSE;

	/* Loading personal settings */
	DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_LOADINGYOURPERSONALSETTINGS);
	ProfileInfo.hProfile = INVALID_HANDLE_VALUE;
	if (0 == (Session->Options & WLX_LOGON_OPT_NO_PROFILE))
	{
		if (Session->Profile == NULL
		 || (Session->Profile->dwType != WLX_PROFILE_TYPE_V1_0
		  && Session->Profile->dwType != WLX_PROFILE_TYPE_V2_0))
		{
			ERR("WL: Wrong profile\n");
			goto cleanup;
		}

		/* Load the user profile */
		ZeroMemory(&ProfileInfo, sizeof(PROFILEINFOW));
		ProfileInfo.dwSize = sizeof(PROFILEINFOW);
		ProfileInfo.dwFlags = 0;
		ProfileInfo.lpUserName = Session->MprNotifyInfo.pszUserName;
		ProfileInfo.lpProfilePath = Session->Profile->pszProfile;
		if (Session->Profile->dwType >= WLX_PROFILE_TYPE_V2_0)
		{
			ProfileInfo.lpDefaultPath = Session->Profile->pszNetworkDefaultUserProfile;
			ProfileInfo.lpServerName = Session->Profile->pszServerName;
			ProfileInfo.lpPolicyPath = Session->Profile->pszPolicy;
		}

		if (!LoadUserProfileW(Session->UserToken, &ProfileInfo))
		{
			ERR("WL: LoadUserProfileW() failed\n");
			goto cleanup;
		}
	}

	/* Create environment block for the user */
	if (!CreateEnvironmentBlock(
		&lpEnvironment,
		Session->UserToken,
		TRUE))
	{
		WARN("WL: CreateEnvironmentBlock() failed\n");
		goto cleanup;
	}
	/* FIXME: Append variables of Session->Profile->pszEnvironment */

	//DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_APPLYINGYOURPERSONALSETTINGS);
	/* FIXME: UpdatePerUserSystemParameters(0, TRUE); */

	/* Get privilege */
	/* FIXME: who should do it? winlogon or gina? */
	/* FIXME: reverting to lower privileges after creating user shell? */
	RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, TRUE, FALSE, &Old);

	/* Set default language */
	if (!SetDefaultLanguage(TRUE))
	{
		WARN("WL: SetDefaultLanguage() failed\n");
		goto cleanup;
	}

	if (!Session->Gina.Functions.WlxActivateUserShell(
		Session->Gina.Context,
		L"Default",
		NULL, /* FIXME */
		lpEnvironment))
	{
		//WCHAR StatusMsg[256];
		WARN("WL: WlxActivateUserShell() failed\n");
		//LoadStringW(hAppInstance, IDS_FAILEDACTIVATEUSERSHELL, StatusMsg, sizeof(StatusMsg));
		//MessageBoxW(0, StatusMsg, NULL, MB_ICONERROR);
		goto cleanup;
	}

	if (!InitializeScreenSaver(Session))
		WARN("WL: Failed to initialize screen saver\n");

	Session->hProfileInfo = ProfileInfo.hProfile;
	ret = TRUE;

cleanup:
	HeapFree(GetProcessHeap(), 0, Session->Profile);
	Session->Profile = NULL;
	if (!ret
	 && ProfileInfo.hProfile != INVALID_HANDLE_VALUE)
	{
		UnloadUserProfile(WLSession->UserToken, ProfileInfo.hProfile);
	}
	if (lpEnvironment)
		DestroyEnvironmentBlock(lpEnvironment);
	RemoveStatusMessage(Session);
	if (!ret)
	{
	    Session->UserToken = NULL;
	    CloseHandle(Session->UserToken);
    }
	return ret;
}

#define EWX_ACTION_MASK 0xffffffeb
#define EWX_FLAGS_MASK  0x00000014

typedef struct tagLOGOFF_SHUTDOWN_DATA
{
  UINT Flags;
  PWLSESSION Session;
} LOGOFF_SHUTDOWN_DATA, *PLOGOFF_SHUTDOWN_DATA;

static DWORD WINAPI
LogoffShutdownThread(LPVOID Parameter)
{
	PLOGOFF_SHUTDOWN_DATA LSData = (PLOGOFF_SHUTDOWN_DATA)Parameter;

	if (LSData->Session->UserToken != NULL && !ImpersonateLoggedOnUser(LSData->Session->UserToken))
	{
		ERR("ImpersonateLoggedOnUser() failed with error %lu\n", GetLastError());
		return 0;
	}

	/* Close processes of the interactive user */
	if (!ExitWindowsEx(
		EWX_INTERNAL_KILL_USER_APPS | (LSData->Flags & EWX_FLAGS_MASK) |
		(EWX_LOGOFF == (LSData->Flags & EWX_ACTION_MASK) ? EWX_INTERNAL_FLAG_LOGOFF : 0),
		0))
	{
		ERR("Unable to kill user apps, error %lu\n", GetLastError());
		RevertToSelf();
		return 0;
	}

	/* FIXME: Call ExitWindowsEx() to terminate COM processes */

	if (LSData->Session->UserToken)
		RevertToSelf();

	return 1;
}

static NTSTATUS
HandleLogoff(
	IN OUT PWLSESSION Session,
	IN UINT Flags)
{
	PLOGOFF_SHUTDOWN_DATA LSData;
	HANDLE hThread;
	DWORD exitCode;

	DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_SAVEYOURSETTINGS);

	/* Prepare data for logoff thread */
	LSData = HeapAlloc(GetProcessHeap(), 0, sizeof(LOGOFF_SHUTDOWN_DATA));
	if (!LSData)
	{
		ERR("Failed to allocate mem for thread data\n");
		return STATUS_NO_MEMORY;
	}
	LSData->Flags = Flags;
	LSData->Session = Session;

	/* Run logoff thread */
	hThread = CreateThread(NULL, 0, LogoffShutdownThread, (LPVOID)LSData, 0, NULL);
	if (!hThread)
	{
		ERR("Unable to create logoff thread, error %lu\n", GetLastError());
		HeapFree(GetProcessHeap(), 0, LSData);
		return STATUS_UNSUCCESSFUL;
	}
	WaitForSingleObject(hThread, INFINITE);
	HeapFree(GetProcessHeap(), 0, LSData);
	if (!GetExitCodeThread(hThread, &exitCode))
	{
		ERR("Unable to get exit code of logoff thread (error %lu)\n", GetLastError());
		CloseHandle(hThread);
		return STATUS_UNSUCCESSFUL;
	}
	CloseHandle(hThread);
	if (exitCode == 0)
	{
		ERR("Logoff thread returned failure\n");
		return STATUS_UNSUCCESSFUL;
	}

	//UnloadUserProfile(Session->UserToken, Session->hProfileInfo);
	//CloseHandle(Session->UserToken);
	//UpdatePerUserSystemParameters(0, FALSE);
	Session->LogonStatus = WKSTA_IS_LOGGED_OFF;
	Session->UserToken = NULL;
	return STATUS_SUCCESS;
}

static BOOL CALLBACK
ShutdownComputerWindowProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (uMsg)
	{
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_BTNSHTDOWNCOMPUTER:
					EndDialog(hwndDlg, IDC_BTNSHTDOWNCOMPUTER);
					return TRUE;
			}
			break;
		}
		case WM_INITDIALOG:
		{
			RemoveMenu(GetSystemMenu(hwndDlg, FALSE), SC_CLOSE, MF_BYCOMMAND);
			SetFocus(GetDlgItem(hwndDlg, IDC_BTNSHTDOWNCOMPUTER));
			return TRUE;
		}
	}
	return FALSE;
}

static VOID
UninitializeSAS(
	IN OUT PWLSESSION Session)
{
	if (Session->SASWindow)
	{
		DestroyWindow(Session->SASWindow);
		Session->SASWindow = NULL;
	}
	if (Session->hEndOfScreenSaverThread)
		SetEvent(Session->hEndOfScreenSaverThread);
	UnregisterClassW(WINLOGON_SAS_CLASS, hAppInstance);
}

NTSTATUS
HandleShutdown(
	IN OUT PWLSESSION Session,
	IN DWORD wlxAction)
{
	PLOGOFF_SHUTDOWN_DATA LSData;
	HANDLE hThread;
	DWORD exitCode;

	DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_REACTOSISSHUTTINGDOWN);

	/* Prepare data for shutdown thread */
	LSData = HeapAlloc(GetProcessHeap(), 0, sizeof(LOGOFF_SHUTDOWN_DATA));
	if (!LSData)
	{
		ERR("Failed to allocate mem for thread data\n");
		return STATUS_NO_MEMORY;
	}
	if (wlxAction == WLX_SAS_ACTION_SHUTDOWN_POWER_OFF)
		LSData->Flags = EWX_POWEROFF;
	else if (wlxAction == WLX_SAS_ACTION_SHUTDOWN_REBOOT)
		LSData->Flags = EWX_REBOOT;
	else
		LSData->Flags = EWX_SHUTDOWN;
	LSData->Session = Session;

	/* Run shutdown thread */
	hThread = CreateThread(NULL, 0, LogoffShutdownThread, (LPVOID)LSData, 0, NULL);
	if (!hThread)
	{
		ERR("Unable to create shutdown thread, error %lu\n", GetLastError());
		HeapFree(GetProcessHeap(), 0, LSData);
		return STATUS_UNSUCCESSFUL;
	}
	WaitForSingleObject(hThread, INFINITE);
	HeapFree(GetProcessHeap(), 0, LSData);
	if (!GetExitCodeThread(hThread, &exitCode))
	{
		ERR("Unable to get exit code of shutdown thread (error %lu)\n", GetLastError());
		CloseHandle(hThread);
		return STATUS_UNSUCCESSFUL;
	}
	CloseHandle(hThread);
	if (exitCode == 0)
	{
		ERR("Shutdown thread returned failure\n");
		return STATUS_UNSUCCESSFUL;
	}

	/* Destroy SAS window */
	UninitializeSAS(Session);

	FIXME("FIXME: Call SMSS API #1\n");
	if (wlxAction == WLX_SAS_ACTION_SHUTDOWN_REBOOT)
		NtShutdownSystem(ShutdownReboot);
	else
	{
		if (FALSE)
		{
			/* FIXME - only show this dialog if it's a shutdown and the computer doesn't support APM */
			DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_SHUTDOWNCOMPUTER), GetDesktopWindow(), ShutdownComputerWindowProc);
		}
		NtShutdownSystem(ShutdownNoReboot);
	}
	return STATUS_SUCCESS;
}

static VOID
DoGenericAction(
	IN OUT PWLSESSION Session,
	IN DWORD wlxAction)
{
	switch (wlxAction)
	{
		case WLX_SAS_ACTION_LOGON: /* 0x01 */
			if (HandleLogon(Session))
			{
				SwitchDesktop(Session->ApplicationDesktop);
				Session->LogonStatus = WKSTA_IS_LOGGED_ON;
			}
			else
				Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
			break;
		case WLX_SAS_ACTION_NONE: /* 0x02 */
			break;
		case WLX_SAS_ACTION_LOCK_WKSTA: /* 0x03 */
			if (Session->Gina.Functions.WlxIsLockOk(Session->Gina.Context))
			{
				SwitchDesktop(WLSession->WinlogonDesktop);
				Session->LogonStatus = WKSTA_IS_LOCKED;
				Session->Gina.Functions.WlxDisplayLockedNotice(Session->Gina.Context);
			}
			break;
		case WLX_SAS_ACTION_LOGOFF: /* 0x04 */
		case WLX_SAS_ACTION_SHUTDOWN: /* 0x05 */
		case WLX_SAS_ACTION_SHUTDOWN_POWER_OFF: /* 0x0a */
		case WLX_SAS_ACTION_SHUTDOWN_REBOOT: /* 0x0b */
			if (Session->LogonStatus != WKSTA_IS_LOGGED_OFF)
			{
				if (!Session->Gina.Functions.WlxIsLogoffOk(Session->Gina.Context))
					break;
				SwitchDesktop(WLSession->WinlogonDesktop);
				Session->Gina.Functions.WlxLogoff(Session->Gina.Context);
				if (!NT_SUCCESS(HandleLogoff(Session, EWX_LOGOFF)))
				{
					RemoveStatusMessage(Session);
					break;
				}
			}
			if (WLX_SHUTTINGDOWN(wlxAction))
			{
				Session->Gina.Functions.WlxShutdown(Session->Gina.Context, wlxAction);
				if (!NT_SUCCESS(HandleShutdown(Session, wlxAction)))
				{
					RemoveStatusMessage(Session);
					Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
				}
			}
			else
			{
				RemoveStatusMessage(Session);
				Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
			}
			break;
		case WLX_SAS_ACTION_TASKLIST: /* 0x07 */
			SwitchDesktop(WLSession->ApplicationDesktop);
			StartTaskManager(Session);
			break;
		case WLX_SAS_ACTION_UNLOCK_WKSTA: /* 0x08 */
			SwitchDesktop(WLSession->ApplicationDesktop);
			Session->LogonStatus = WKSTA_IS_LOGGED_ON;
			break;
		default:
			WARN("Unknown SAS action 0x%lx\n", wlxAction);
	}
}

static VOID
DispatchSAS(
	IN OUT PWLSESSION Session,
	IN DWORD dwSasType)
{
	DWORD wlxAction = WLX_SAS_ACTION_NONE;

	if (Session->LogonStatus == WKSTA_IS_LOGGED_ON)
		wlxAction = (DWORD)Session->Gina.Functions.WlxLoggedOnSAS(Session->Gina.Context, dwSasType, NULL);
	else if (Session->LogonStatus == WKSTA_IS_LOCKED)
		wlxAction = (DWORD)Session->Gina.Functions.WlxWkstaLockedSAS(Session->Gina.Context, dwSasType);
	else
	{
		/* Display a new dialog (if necessary) */
		switch (dwSasType)
		{
			case WLX_SAS_TYPE_TIMEOUT: /* 0x00 */
			{
				Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
				break;
			}
			default:
			{
				PSID LogonSid = NULL; /* FIXME */

				Session->Options = 0;

				wlxAction = (DWORD)Session->Gina.Functions.WlxLoggedOutSAS(
					Session->Gina.Context,
					Session->SASAction,
					&Session->LogonId,
					LogonSid,
					&Session->Options,
					&Session->UserToken,
					&Session->MprNotifyInfo,
					(PVOID*)&Session->Profile);
				break;
			}
		}
	}

	if (dwSasType == WLX_SAS_TYPE_SCRNSVR_TIMEOUT)
	{
		BOOL bSecure = TRUE;
		if (!Session->Gina.Functions.WlxScreenSaverNotify(Session->Gina.Context, &bSecure))
		{
			/* Skip start of screen saver */
			SetEvent(Session->hEndOfScreenSaver);
		}
		else
		{
			if (bSecure)
				DoGenericAction(Session, WLX_SAS_ACTION_LOCK_WKSTA);
			StartScreenSaver(Session);
		}
	}
	else if (dwSasType == WLX_SAS_TYPE_SCRNSVR_ACTIVITY)
		SetEvent(Session->hUserActivity);

	DoGenericAction(Session, wlxAction);
}

static BOOL
RegisterHotKeys(
	IN PWLSESSION Session,
	IN HWND hwndSAS)
{
	/* Register Ctrl+Alt+Del Hotkey */
	if (!RegisterHotKey(hwndSAS, HK_CTRL_ALT_DEL, MOD_CONTROL | MOD_ALT, VK_DELETE))
	{
		ERR("WL: Unable to register Ctrl+Alt+Del hotkey!\n");
		return FALSE;
	}

	/* Register Ctrl+Shift+Esc (optional) */
	Session->TaskManHotkey = RegisterHotKey(hwndSAS, HK_CTRL_SHIFT_ESC, MOD_CONTROL | MOD_SHIFT, VK_ESCAPE);
	if (!Session->TaskManHotkey)
		WARN("WL: Warning: Unable to register Ctrl+Alt+Esc hotkey!\n");
	return TRUE;
}

static BOOL
UnregisterHotKeys(
	IN PWLSESSION Session,
	IN HWND hwndSAS)
{
	/* Unregister hotkeys */
	UnregisterHotKey(hwndSAS, HK_CTRL_ALT_DEL);

	if (Session->TaskManHotkey)
		UnregisterHotKey(hwndSAS, HK_CTRL_SHIFT_ESC);

	return TRUE;
}

static NTSTATUS
CheckForShutdownPrivilege(
	IN DWORD RequestingProcessId)
{
	HANDLE Process;
	HANDLE Token;
	BOOL CheckResult;
	PPRIVILEGE_SET PrivSet;

	TRACE("CheckForShutdownPrivilege()\n");

	Process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, RequestingProcessId);
	if (!Process)
	{
		WARN("OpenProcess() failed with error %lu\n", GetLastError());
		return STATUS_INVALID_HANDLE;
	}
	if (!OpenProcessToken(Process, TOKEN_QUERY, &Token))
	{
		WARN("OpenProcessToken() failed with error %lu\n", GetLastError());
		CloseHandle(Process);
		return STATUS_INVALID_HANDLE;
	}
	CloseHandle(Process);
	PrivSet = HeapAlloc(GetProcessHeap(), 0, sizeof(PRIVILEGE_SET) + sizeof(LUID_AND_ATTRIBUTES));
	if (!PrivSet)
	{
		ERR("Failed to allocate mem for privilege set\n");
		CloseHandle(Token);
		return STATUS_NO_MEMORY;
	}
	PrivSet->PrivilegeCount = 1;
	PrivSet->Control = PRIVILEGE_SET_ALL_NECESSARY;
	if (!LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &PrivSet->Privilege[0].Luid))
	{
		WARN("LookupPrivilegeValue() failed with error %lu\n", GetLastError());
		HeapFree(GetProcessHeap(), 0, PrivSet);
		CloseHandle(Token);
		return STATUS_UNSUCCESSFUL;
	}
	if (!PrivilegeCheck(Token, PrivSet, &CheckResult))
	{
		WARN("PrivilegeCheck() failed with error %lu\n", GetLastError());
		HeapFree(GetProcessHeap(), 0, PrivSet);
		CloseHandle(Token);
		return STATUS_ACCESS_DENIED;
	}
	HeapFree(GetProcessHeap(), 0, PrivSet);
	CloseHandle(Token);

	if (!CheckResult)
	{
		WARN("SE_SHUTDOWN privilege not enabled\n");
		return STATUS_ACCESS_DENIED;
	}
	return STATUS_SUCCESS;
}

static LRESULT CALLBACK
SASWindowProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	PWLSESSION Session = (PWLSESSION)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (uMsg)
	{
		case WM_HOTKEY:
		{
			switch (lParam)
			{
				case MAKELONG(MOD_CONTROL | MOD_ALT, VK_DELETE):
				{
					TRACE("SAS: CONTROL+ALT+DELETE\n");
					if (!Session->Gina.UseCtrlAltDelete)
						break;
					PostMessageW(Session->SASWindow, WLX_WM_SAS, WLX_SAS_TYPE_CTRL_ALT_DEL, 0);
					return TRUE;
				}
				case MAKELONG(MOD_CONTROL | MOD_SHIFT, VK_ESCAPE):
				{
					TRACE("SAS: CONTROL+SHIFT+ESCAPE\n");
					DoGenericAction(Session, WLX_SAS_ACTION_TASKLIST);
					return TRUE;
				}
			}
			break;
		}
		case WM_CREATE:
		{
			/* Get the session pointer from the create data */
			Session = (PWLSESSION)((LPCREATESTRUCT)lParam)->lpCreateParams;

			/* Save the Session pointer */
			SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)Session);

			return RegisterHotKeys(Session, hwndDlg);
		}
		case WM_DESTROY:
		{
			UnregisterHotKeys(Session, hwndDlg);
			return TRUE;
		}
		case WM_SETTINGCHANGE:
		{
			UINT uiAction = (UINT)wParam;
			if (uiAction == SPI_SETSCREENSAVETIMEOUT
			 || uiAction == SPI_SETSCREENSAVEACTIVE)
			{
				SetEvent(Session->hScreenSaverParametersChanged);
			}
			return TRUE;
		}
		case WLX_WM_SAS:
		{
			DispatchSAS(Session, (DWORD)wParam);
			return TRUE;
		}
		case PM_WINLOGON_EXITWINDOWS:
		{
			UINT Flags = (UINT)lParam;
			UINT Action = Flags & EWX_ACTION_MASK;
			DWORD wlxAction;

			/* Check parameters */
			switch (Action)
			{
				case EWX_LOGOFF: wlxAction = WLX_SAS_ACTION_LOGOFF; break;
				case EWX_SHUTDOWN: wlxAction = WLX_SAS_ACTION_SHUTDOWN; break;
				case EWX_REBOOT: wlxAction = WLX_SAS_ACTION_SHUTDOWN_REBOOT; break;
				case EWX_POWEROFF: wlxAction = WLX_SAS_ACTION_SHUTDOWN_POWER_OFF; break;
				default:
				{
					ERR("Invalid ExitWindows action 0x%x\n", Action);
					return STATUS_INVALID_PARAMETER;
				}
			}

			if (WLX_SHUTTINGDOWN(wlxAction))
			{
				NTSTATUS Status = CheckForShutdownPrivilege((DWORD)wParam);
				if (!NT_SUCCESS(Status))
					return Status;
			}
			DoGenericAction(Session, wlxAction);
			return 1;
		}
	}

	return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

BOOL
InitializeSAS(
	IN OUT PWLSESSION Session)
{
	WNDCLASSEXW swc;
	BOOL ret = FALSE;

	if (!SwitchDesktop(Session->WinlogonDesktop))
	{
		ERR("WL: Failed to switch to winlogon desktop\n");
		goto cleanup;
	}

	/* Register SAS window class */
	swc.cbSize = sizeof(WNDCLASSEXW);
	swc.style = CS_SAVEBITS;
	swc.lpfnWndProc = SASWindowProc;
	swc.cbClsExtra = 0;
	swc.cbWndExtra = 0;
	swc.hInstance = hAppInstance;
	swc.hIcon = NULL;
	swc.hCursor = NULL;
	swc.hbrBackground = NULL;
	swc.lpszMenuName = NULL;
	swc.lpszClassName = WINLOGON_SAS_CLASS;
	swc.hIconSm = NULL;
	if (RegisterClassExW(&swc) == 0)
	{
		ERR("WL: Failed to register SAS window class\n");
		goto cleanup;
	}

	/* Create invisible SAS window */
	Session->SASWindow = CreateWindowExW(
		0,
		WINLOGON_SAS_CLASS,
		WINLOGON_SAS_TITLE,
		WS_POPUP,
		0, 0, 0, 0, 0, 0,
		hAppInstance, Session);
	if (!Session->SASWindow)
	{
		ERR("WL: Failed to create SAS window\n");
		goto cleanup;
	}

	/* Register SAS window to receive SAS notifications */
	if (!SetLogonNotifyWindow(Session->SASWindow, Session->InteractiveWindowStation))
	{
		ERR("WL: Failed to register SAS window\n");
		goto cleanup;
	}

	if (!SetDefaultLanguage(FALSE))
		return FALSE;

	ret = TRUE;

cleanup:
	if (!ret)
		UninitializeSAS(Session);
	return ret;
}
