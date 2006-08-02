/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/winlogon/sas.c
 * PURPOSE:         Secure Attention Sequence
 * PROGRAMMERS:     Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 * UPDATE HISTORY:
 *                  Created 28/03/2004
 */

#include "winlogon.h"

#define YDEBUG
#include <wine/debug.h>

#define WINLOGON_SAS_CLASS L"SAS Window class"
#define WINLOGON_SAS_TITLE L"SAS window"

#define HK_CTRL_ALT_DEL   0
#define HK_CTRL_SHIFT_ESC 1

#ifdef __USE_W32API
extern BOOL STDCALL SetLogonNotifyWindow(HWND Wnd, HWINSTA WinSta);
#endif

static BOOL
StartTaskManager(
	IN OUT PWLSESSION Session)
{
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcessInformation;

	if (Session->LogonStatus == WKSTA_IS_LOGGED_OFF)
		return FALSE;

	StartupInfo.cb = sizeof(StartupInfo);
	StartupInfo.lpReserved = NULL;
	StartupInfo.lpDesktop = NULL;
	StartupInfo.lpTitle = NULL;
	StartupInfo.dwFlags = 0;
	StartupInfo.cbReserved2 = 0;
	StartupInfo.lpReserved2 = 0;

	CreateProcessW(
		L"taskmgr.exe",
		NULL,
		NULL,
		NULL,
		FALSE,
		CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS,
		NULL,
		NULL,
		&StartupInfo,
		&ProcessInformation);

	CloseHandle (ProcessInformation.hProcess);
	CloseHandle (ProcessInformation.hThread);
	return TRUE;
}

static BOOL
HandleLogon(
	IN OUT PWLSESSION Session)
{
	PROFILEINFOW ProfileInfo;
	LPVOID lpEnvironment = NULL;
	BOOLEAN Old;

	if (!(Session->Options & WLX_LOGON_OPT_NO_PROFILE))
	{
		/* Load the user profile */
		ProfileInfo.dwSize = sizeof(PROFILEINFOW);
		ProfileInfo.dwFlags = 0;
		ProfileInfo.lpUserName = Session->MprNotifyInfo.pszUserName;
		ProfileInfo.lpProfilePath = Session->Profile.pszProfile;
		ProfileInfo.lpDefaultPath = Session->Profile.pszNetworkDefaultUserProfile;
		ProfileInfo.lpServerName = Session->Profile.pszServerName;
		ProfileInfo.lpPolicyPath = Session->Profile.pszPolicy;
		ProfileInfo.hProfile = NULL;

		if (!LoadUserProfileW(Session->UserToken, &ProfileInfo))
		{
			ERR("WL: LoadUserProfileW() failed\n");
			CloseHandle(Session->UserToken);
			return FALSE;
		}
	}

	/* Create environment block for the user */
	if (!CreateEnvironmentBlock(
		&lpEnvironment,
		Session->UserToken,
		TRUE))
	{
		ERR("WL: CreateEnvironmentBlock() failed\n");
		UnloadUserProfile(WLSession->UserToken, ProfileInfo.hProfile);
		CloseHandle(Session->UserToken);
		return FALSE;
	}
	/* FIXME: use Session->Profile.pszEnvironment */

	/* Get privilege */
	/* FIXME: who should do it? winlogon or gina? */
	/* FIXME: reverting to lower privileges after creating user shell? */
	RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, TRUE, FALSE, &Old);

	return Session->MsGina.Functions.WlxActivateUserShell(
		Session->MsGina.Context,
		L"WinSta0\\Default",//NULL, /* FIXME */
		NULL, /* FIXME */
		lpEnvironment);
}

static BOOL
HandleLogoff(
	IN OUT PWLSESSION Session)
{
	FIXME("FIXME: HandleLogoff() unimplemented\n");
	return FALSE;
}

static BOOL
HandleShutdown(
	IN OUT PWLSESSION Session)
{
	FIXME("FIXME: HandleShutdown() unimplemented\n");
	return FALSE;
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
				Session->LogonStatus = WKSTA_IS_LOGGED_ON;
				SwitchDesktop(WLSession->ApplicationDesktop);
			}
			break;
		case WLX_SAS_ACTION_NONE: /* 0x02 */
			break;
		case WLX_SAS_ACTION_LOCK_WKSTA: /* 0x03 */
			if (Session->MsGina.Functions.WlxIsLockOk(Session->MsGina.Context))
			{
				SwitchDesktop(WLSession->WinlogonDesktop);
				Session->LogonStatus = WKSTA_IS_LOCKED;
				Session->MsGina.Functions.WlxDisplayLockedNotice(Session->MsGina.Context);
			}
			break;
		case WLX_SAS_ACTION_LOGOFF: /* 0x04 */
		case WLX_SAS_ACTION_SHUTDOWN: /* 0x05 */
			if (Session->LogonStatus != WKSTA_IS_LOGGED_OFF)
			{
				if (!Session->MsGina.Functions.WlxIsLogoffOk(Session->MsGina.Context))
					break;
				SwitchDesktop(WLSession->WinlogonDesktop);
				Session->MsGina.Functions.WlxLogoff(Session->MsGina.Context);
				HandleLogoff(Session);
				Session->LogonStatus = WKSTA_IS_LOGGED_OFF;
				Session->MsGina.Functions.WlxDisplaySASNotice(Session->MsGina.Context);
			}
			if (WLX_SHUTTINGDOWN(wlxAction))
				HandleShutdown(Session);
			break;
		case WLX_SAS_ACTION_TASKLIST: /* 0x07 */
			StartTaskManager(Session);
			break;
		default:
			WARN("Unknown SAS action 0x%lx\n", wlxAction);
	}
}

VOID
DispatchSAS(
	IN OUT PWLSESSION Session,
	IN DWORD dwSasType)
{
	DWORD wlxAction = WLX_SAS_ACTION_NONE;

	if (Session->LogonStatus == WKSTA_IS_LOGGED_ON)
		wlxAction = Session->MsGina.Functions.WlxLoggedOnSAS(Session->MsGina.Context, dwSasType, NULL);
	else if (Session->LogonStatus == WKSTA_IS_LOCKED)
		wlxAction = Session->MsGina.Functions.WlxWkstaLockedSAS(Session->MsGina.Context, dwSasType);
	else
	{
		/* Display a new dialog (if necessary) */
		switch (dwSasType)
		{
			case WLX_SAS_TYPE_TIMEOUT: /* 0x00 */
			{
				Session->MsGina.Functions.WlxDisplaySASNotice(Session->MsGina.Context);
				break;
			}
			case WLX_SAS_TYPE_CTRL_ALT_DEL: /* 0x01 */
			{
				PSID LogonSid = NULL; /* FIXME */

				ZeroMemory(&Session->Profile, sizeof(Session->Profile));
				Session->Options = 0;

				wlxAction = Session->MsGina.Functions.WlxLoggedOutSAS(
					Session->MsGina.Context,
					Session->SASAction,
					&Session->LogonId,
					LogonSid,
					&Session->Options,
					&Session->UserToken,
					&Session->MprNotifyInfo,
					(PVOID*)&Session->Profile);
				break;
			}
			default:
				WARN("Unknown SAS type 0x%lx\n", dwSasType);
		}
	}

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
  PLOGOFF_SHUTDOWN_DATA LSData = (PLOGOFF_SHUTDOWN_DATA) Parameter;

  if (! ImpersonateLoggedOnUser(LSData->Session->UserToken))
  {
    DPRINT1("ImpersonateLoggedOnUser failed with error %d\n", GetLastError());
    return 0;
  }
  if (! ExitWindowsEx(EWX_INTERNAL_KILL_USER_APPS | (LSData->Flags & EWX_FLAGS_MASK)
                      | (EWX_LOGOFF == (LSData->Flags & EWX_ACTION_MASK) ? EWX_INTERNAL_FLAG_LOGOFF : 0),
                      0))
  {
    DPRINT1("Unable to kill user apps, error %d\n", GetLastError());
    RevertToSelf();
    return 0;
  }
  RevertToSelf();

  HeapFree(GetProcessHeap(), 0, LSData);

  return 1;
}

static LRESULT
HandleExitWindows(PWLSESSION Session, DWORD RequestingProcessId, UINT Flags)
{
  UINT Action;
  HANDLE Process;
  HANDLE Token;
  HANDLE Thread;
  BOOL CheckResult;
  PPRIVILEGE_SET PrivSet;
  PLOGOFF_SHUTDOWN_DATA LSData;

  /* Check parameters */
  Action = Flags & EWX_ACTION_MASK;
  if (EWX_LOGOFF != Action && EWX_SHUTDOWN != Action && EWX_REBOOT != Action
      && EWX_POWEROFF != Action)
  {
    DPRINT1("Invalid ExitWindows action 0x%x\n", Action);
    return STATUS_INVALID_PARAMETER;
  }

  /* Check privilege */
  if (EWX_LOGOFF != Action)
  {
    Process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, RequestingProcessId);
    if (NULL == Process)
    {
      DPRINT1("OpenProcess failed with error %d\n", GetLastError());
      return STATUS_INVALID_HANDLE;
    }
    if (! OpenProcessToken(Process, TOKEN_QUERY, &Token))
    {
      DPRINT1("OpenProcessToken failed with error %d\n", GetLastError());
      CloseHandle(Process);
      return STATUS_INVALID_HANDLE;
    }
    CloseHandle(Process);
    PrivSet = HeapAlloc(GetProcessHeap(), 0, sizeof(PRIVILEGE_SET) + sizeof(LUID_AND_ATTRIBUTES));
    if (NULL == PrivSet)
    {
      DPRINT1("Failed to allocate mem for privilege set\n");
      CloseHandle(Token);
      return STATUS_NO_MEMORY;
    }
    PrivSet->PrivilegeCount = 1;
    PrivSet->Control = PRIVILEGE_SET_ALL_NECESSARY;
    if (! LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &PrivSet->Privilege[0].Luid))
    {
      DPRINT1("LookupPrivilegeValue failed with error %d\n", GetLastError());
      HeapFree(GetProcessHeap(), 0, PrivSet);
      CloseHandle(Token);
      return STATUS_UNSUCCESSFUL;
    }
    if (! PrivilegeCheck(Token, PrivSet, &CheckResult))
    {
      DPRINT1("PrivilegeCheck failed with error %d\n", GetLastError());
      HeapFree(GetProcessHeap(), 0, PrivSet);
      CloseHandle(Token);
      return STATUS_ACCESS_DENIED;
    }
    HeapFree(GetProcessHeap(), 0, PrivSet);
    CloseHandle(Token);
    if (! CheckResult)
    {
      DPRINT1("SE_SHUTDOWN privilege not enabled\n");
      return STATUS_ACCESS_DENIED;
    }
  }

  LSData = HeapAlloc(GetProcessHeap(), 0, sizeof(LOGOFF_SHUTDOWN_DATA));
  if (NULL == LSData)
  {
    DPRINT1("Failed to allocate mem for thread data\n");
    return STATUS_NO_MEMORY;
  }
  LSData->Flags = Flags;
  LSData->Session = Session;
  Thread = CreateThread(NULL, 0, LogoffShutdownThread, (LPVOID) LSData, 0, NULL);
  if (NULL == Thread)
  {
    DPRINT1("Unable to create shutdown thread, error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, LSData);
    return STATUS_UNSUCCESSFUL;
  }
  CloseHandle(Thread);

  return 1;
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
					DispatchSAS(Session, WLX_SAS_TYPE_CTRL_ALT_DEL);
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
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)Session);

			return RegisterHotKeys(Session, hwndDlg);
		}
		case WM_DESTROY:
		{
			UnregisterHotKeys(Session, hwndDlg);
			return TRUE;
		}
		case PM_WINLOGON_EXITWINDOWS:
		{
			return HandleExitWindows(Session, (DWORD) wParam, (UINT) lParam);
		}
	}

	return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
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
}

BOOL
InitializeSAS(
	IN OUT PWLSESSION Session)
{
	WNDCLASSEX swc;

	/* register SAS window class.
	 * WARNING! MAKE SURE WE ARE IN THE WINLOGON DESKTOP! */
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
	RegisterClassEx(&swc); /* FIXME: check return code */

	/* create invisible SAS window */
	DPRINT1("Session %p\n", Session);
	Session->SASWindow = CreateWindowEx(
		0,
		WINLOGON_SAS_CLASS,
		WINLOGON_SAS_TITLE,
		WS_POPUP,
		0, 0, 0, 0, 0, 0,
		hAppInstance, Session);
	if (!Session->SASWindow)
	{
		ERR("WL: Failed to create SAS window\n");
		return FALSE;
	}

	/* Register SAS window to receive SAS notifications */
	if (!SetLogonNotifyWindow(Session->SASWindow, Session->InteractiveWindowStation))
	{
		UninitializeSAS(Session);
		ERR("WL: Failed to register SAS window\n");
		return FALSE;
	}

	return TRUE;
}
