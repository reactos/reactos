/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/winlogon.c
 * PURPOSE:         Logon
 * PROGRAMMERS:     Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Filip Navara
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(winlogon);

/* GLOBALS ******************************************************************/

HINSTANCE hAppInstance;
PWLSESSION WLSession = NULL;

/* FUNCTIONS *****************************************************************/

static BOOL
StartServicesManager(VOID)
{
	HANDLE ServicesInitEvent = NULL;
	STARTUPINFOW StartupInfo;
	PROCESS_INFORMATION ProcessInformation;
	DWORD Count;
	LPCWSTR ServiceString = L"services.exe";
	BOOL res;

	/* Start the service control manager (services.exe) */
	StartupInfo.cb = sizeof(StartupInfo);
	StartupInfo.lpReserved = NULL;
	StartupInfo.lpDesktop = NULL;
	StartupInfo.lpTitle = NULL;
	StartupInfo.dwFlags = 0;
	StartupInfo.cbReserved2 = 0;
	StartupInfo.lpReserved2 = 0;

	TRACE("WL: Creating new process - %S\n", ServiceString);

	res = CreateProcessW(
		ServiceString,
		NULL,
		NULL,
		NULL,
		FALSE,
		DETACHED_PROCESS,
		NULL,
		NULL,
		&StartupInfo,
		&ProcessInformation);
	if (!res)
	{
		ERR("WL: Failed to execute services (error %lu)\n", GetLastError());
		return FALSE;
	}

	/* Wait for event creation (by SCM) for max. 20 seconds */
	for (Count = 0; Count < 20; Count++)
	{
		Sleep(1000);

		TRACE("WL: Attempting to open event \"SvcctrlStartEvent_A3752DX\"\n");
		ServicesInitEvent = OpenEventW(
			SYNCHRONIZE,
			FALSE,
			L"SvcctrlStartEvent_A3752DX");
		if (ServicesInitEvent)
			break;
	}

	if (!ServicesInitEvent)
	{
		ERR("WL: Failed to open event \"SvcctrlStartEvent_A3752DX\"\n");
		return FALSE;
	}

	/* Wait for event signalization */
	WaitForSingleObject(ServicesInitEvent, INFINITE);
	CloseHandle(ServicesInitEvent);
	TRACE("WL: StartServicesManager() done.\n");

	return TRUE;
}


static BOOL
StartLsass(VOID)
{
	STARTUPINFOW StartupInfo;
	PROCESS_INFORMATION ProcessInformation;
	LPCWSTR ServiceString = L"lsass.exe";
	BOOL res;

	/* Start the service control manager (services.exe) */
	StartupInfo.cb = sizeof(StartupInfo);
	StartupInfo.lpReserved = NULL;
	StartupInfo.lpDesktop = NULL;
	StartupInfo.lpTitle = NULL;
	StartupInfo.dwFlags = 0;
	StartupInfo.cbReserved2 = 0;
	StartupInfo.lpReserved2 = 0;

	TRACE("WL: Creating new process - %S\n", ServiceString);

	res = CreateProcessW(
		ServiceString,
		NULL,
		NULL,
		NULL,
		FALSE,
		DETACHED_PROCESS,
		NULL,
		NULL,
		&StartupInfo,
		&ProcessInformation);

	return res;
}

BOOL
DisplayStatusMessage(
	IN PWLSESSION Session,
	IN HDESK hDesktop,
	IN UINT ResourceId)
{
	WCHAR StatusMsg[MAX_PATH];

	if (Session->Gina.Version < WLX_VERSION_1_3)
		return TRUE;

	if (Session->SuppressStatus)
		return TRUE;

	if (LoadStringW(hAppInstance, ResourceId, StatusMsg, MAX_PATH) == 0)
		return FALSE;

	return Session->Gina.Functions.WlxDisplayStatusMessage(Session->Gina.Context, hDesktop, 0, NULL, StatusMsg);
}

BOOL
RemoveStatusMessage(
	IN PWLSESSION Session)
{
	if (Session->Gina.Version < WLX_VERSION_1_3)
		return TRUE;

	return Session->Gina.Functions.WlxRemoveStatusMessage(Session->Gina.Context);
}

static INT_PTR CALLBACK
GinaLoadFailedWindowProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hwndDlg, IDOK);
					return TRUE;
			}
			break;
		}
		case WM_INITDIALOG:
		{
			int len;
			WCHAR templateText[MAX_PATH], text[MAX_PATH];

			len = GetDlgItemTextW(hwndDlg, IDC_GINALOADFAILED, templateText, MAX_PATH);
			if (len)
			{
				wsprintfW(text, templateText, (LPWSTR)lParam);
				SetDlgItemTextW(hwndDlg, IDC_GINALOADFAILED, text);
			}
			SetFocus(GetDlgItem(hwndDlg, IDOK));
			return TRUE;
		}
		case WM_CLOSE:
		{
			EndDialog(hwndDlg, IDCANCEL);
			return TRUE;
		}
	}

	return FALSE;
}

int WINAPI
WinMain(
	IN HINSTANCE hInstance,
	IN HINSTANCE hPrevInstance,
	IN LPSTR lpCmdLine,
	IN int nShowCmd)
{
#if 0
	LSA_STRING ProcessName, PackageName;
	HANDLE LsaHandle;
	LSA_OPERATIONAL_MODE Mode;
	BOOLEAN Old;
	ULONG AuthenticationPackage;
	NTSTATUS Status;
#endif
	ULONG HardErrorResponse;
	MSG Msg;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nShowCmd);

	hAppInstance = hInstance;

	if (!RegisterLogonProcess(GetCurrentProcessId(), TRUE))
	{
		ERR("WL: Could not register logon process\n");
		HandleShutdown(NULL, WLX_SAS_ACTION_SHUTDOWN_POWER_OFF);
		NtShutdownSystem(ShutdownNoReboot);
		ExitProcess(0);
	}

	WLSession = (PWLSESSION)HeapAlloc(GetProcessHeap(), 0, sizeof(WLSESSION));
	if (!WLSession)
	{
		ERR("WL: Could not allocate memory for winlogon instance\n");
		NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, NULL, OptionOk, &HardErrorResponse);
		ExitProcess(1);
	}
	ZeroMemory(WLSession, sizeof(WLSESSION));
	WLSession->DialogTimeout = 120; /* 2 minutes */

	if (!CreateWindowStationAndDesktops(WLSession))
	{
		ERR("WL: Could not create window station and desktops\n");
		NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, NULL, OptionOk, &HardErrorResponse);
		ExitProcess(1);
	}
	LockWorkstation(WLSession);

	if (!StartServicesManager())
	{
		ERR("WL: Could not start services.exe\n");
		NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, NULL, OptionOk, &HardErrorResponse);
		ExitProcess(1);
	}

	if (!StartLsass())
	{
		ERR("WL: Failed to start lsass.exe service (error %lu)\n", GetLastError());
		NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, 0, OptionOk, &HardErrorResponse);
		ExitProcess(1);
	}

	/* Load and initialize gina */
	if (!GinaInit(WLSession))
	{
		ERR("WL: Failed to initialize Gina\n");
		DialogBoxParam(hAppInstance, MAKEINTRESOURCE(IDD_GINALOADFAILED), GetDesktopWindow(), GinaLoadFailedWindowProc, (LPARAM)L"");
		HandleShutdown(WLSession, WLX_SAS_ACTION_SHUTDOWN_REBOOT);
		ExitProcess(1);
	}

	DisplayStatusMessage(WLSession, WLSession->WinlogonDesktop, IDS_REACTOSISSTARTINGUP);

#if 0
	/* Connect to NetLogon service (lsass.exe) */
	/* Real winlogon uses "Winlogon" */
	RtlInitUnicodeString((PUNICODE_STRING)&ProcessName, L"Winlogon");
	Status = LsaRegisterLogonProcess(&ProcessName, &LsaHandle, &Mode);
	if (Status == STATUS_PORT_CONNECTION_REFUSED)
	{
		/* Add the 'SeTcbPrivilege' privilege and try again */
		Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, TRUE, &Old);
		if (!NT_SUCCESS(Status))
		{
			ERR("RtlAdjustPrivilege() failed with error %lu\n", LsaNtStatusToWinError(Status));
			return 1;
		}
		Status = LsaRegisterLogonProcess(&ProcessName, &LsaHandle, &Mode);
	}
	if (!NT_SUCCESS(Status))
	{
		ERR("LsaRegisterLogonProcess() failed with error %lu\n", LsaNtStatusToWinError(Status));
		return 1;
	}

	RtlInitUnicodeString((PUNICODE_STRING)&PackageName, MICROSOFT_KERBEROS_NAME_W);
	Status = LsaLookupAuthenticationPackage(LsaHandle, &PackageName, &AuthenticationPackage);
	if (!NT_SUCCESS(Status))
	{
		ERR("LsaLookupAuthenticationPackage() failed with error %lu\n", LsaNtStatusToWinError(Status));
		LsaDeregisterLogonProcess(LsaHandle);
		return 1;
	}
#endif

	/* Create a hidden window to get SAS notifications */
	if (!InitializeSAS(WLSession))
	{
		ERR("WL: Failed to initialize SAS\n");
		ExitProcess(2);
	}

	//DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_PREPARENETWORKCONNECTIONS);
	//DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_APPLYINGCOMPUTERSETTINGS);

	/* Display logged out screen */
	WLSession->LogonStatus = WKSTA_IS_LOGGED_OFF;
	RemoveStatusMessage(WLSession);

	/* Check for pending setup */
	if (GetSetupType() != 0)
	{
		TRACE("WL: Setup mode detected\n");

		/* Run setup and reboot when done */
		SwitchDesktop(WLSession->ApplicationDesktop);
		RunSetup();
	}
	else
		PostMessageW(WLSession->SASWindow, WLX_WM_SAS, WLX_SAS_TYPE_TIMEOUT, 0);

	/* Tell kernel that CurrentControlSet is good (needed
	 * to support Last good known configuration boot) */
	NtInitializeRegistry(CM_BOOT_FLAG_ACCEPTED);

	/* Message loop for the SAS window */
	while (GetMessageW(&Msg, WLSession->SASWindow, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessageW(&Msg);
	}

	/* We never go there */

	return 0;
}
