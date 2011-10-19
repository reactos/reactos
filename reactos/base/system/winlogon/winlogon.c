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

BOOL
PlaySoundRoutine(
	IN LPCWSTR FileName,
	IN UINT bLogon,
	IN UINT Flags)
{
	typedef BOOL (WINAPI *PLAYSOUNDW)(LPCWSTR,HMODULE,DWORD);
	typedef UINT (WINAPI *WAVEOUTGETNUMDEVS)(VOID);
	PLAYSOUNDW Play;
	WAVEOUTGETNUMDEVS waveOutGetNumDevs;
	UINT NumDevs;
	HMODULE hLibrary;
	BOOL Ret = FALSE;

	hLibrary = LoadLibraryW(L"winmm.dll");
	if (hLibrary)
	{
		waveOutGetNumDevs = (WAVEOUTGETNUMDEVS)GetProcAddress(hLibrary, "waveOutGetNumDevs");
		if (waveOutGetNumDevs)
		{
			NumDevs = waveOutGetNumDevs();
			if (!NumDevs)
			{
				if (!bLogon)
				{
					Beep(500, 500);
				}
				FreeLibrary(hLibrary);
				return FALSE;
			}
		}

		Play = (PLAYSOUNDW)GetProcAddress(hLibrary, "PlaySoundW");
		if (Play)
		{
			Ret = Play(FileName, NULL, Flags);
		}
		FreeLibrary(hLibrary);
	}

	return Ret;
}

DWORD
WINAPI
PlayLogonSoundThread(
	IN LPVOID lpParameter)
{
	HKEY hKey;
	WCHAR szBuffer[MAX_PATH] = {0};
	WCHAR szDest[MAX_PATH];
	DWORD dwSize = sizeof(szBuffer);
	SERVICE_STATUS_PROCESS Info;

	ULONG Index = 0;

	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"AppEvents\\Schemes\\Apps\\.Default\\WindowsLogon\\.Current", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		ExitThread(0);
	}

	if (RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)szBuffer, &dwSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		ExitThread(0);
	}


	RegCloseKey(hKey);

	if (!szBuffer[0])
		ExitThread(0);


	szBuffer[MAX_PATH-1] = L'\0';
	if (ExpandEnvironmentStringsW(szBuffer, szDest, MAX_PATH))
	{
		SC_HANDLE hSCManager, hService;

		hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
		if (!hSCManager)
			ExitThread(0);;

		hService = OpenServiceW(hSCManager, L"wdmaud", GENERIC_READ);
		if (!hService)
		{
			CloseServiceHandle(hSCManager);
			TRACE("WL: failed to open sysaudio Status %x\n", GetLastError());
			ExitThread(0);
		}

		do
		{
			if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&Info, sizeof(SERVICE_STATUS_PROCESS), &dwSize))
			{
				TRACE("WL: QueryServiceStatusEx failed %x\n", GetLastError());
				break;
			}

			if (Info.dwCurrentState == SERVICE_RUNNING)
				break;

			Sleep(1000);

		}while(Index++ < 20);

		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);

		if (Info.dwCurrentState != SERVICE_RUNNING)
			ExitThread(0);

		PlaySoundRoutine(szDest, TRUE, SND_FILENAME);
	}
	ExitThread(0);
}



static BOOL
StartServicesManager(VOID)
{
	STARTUPINFOW StartupInfo;
	PROCESS_INFORMATION ProcessInformation;
	LPCWSTR ServiceString = L"services.exe";
	BOOL res;

	/* Start the service control manager (services.exe) */
	ZeroMemory(&StartupInfo, sizeof(STARTUPINFOW));
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

	TRACE("WL: Created new process - %S\n", ServiceString);

	CloseHandle(ProcessInformation.hThread);
	CloseHandle(ProcessInformation.hProcess);

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

	/* Start the local security authority subsystem (lsass.exe) */
	ZeroMemory(&StartupInfo, sizeof(STARTUPINFOW));
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

	TRACE("WL: Created new process - %S\n", ServiceString);

	CloseHandle(ProcessInformation.hThread);
	CloseHandle(ProcessInformation.hProcess);

	return res;
}


static VOID
WaitForLsass(VOID)
{
    HANDLE hEvent;
    DWORD dwError;

    hEvent = CreateEventW(NULL,
                          TRUE,
                          FALSE,
                          L"LSA_RPC_SERVER_ACTIVE");
    if (hEvent == NULL)
    {
        dwError = GetLastError();
        TRACE("WL: Failed to create the notication event (Error %lu)\n", dwError);

        if (dwError == ERROR_ALREADY_EXISTS)
        {
            hEvent = OpenEventW(SYNCHRONIZE,
                                FALSE,
                                L"LSA_RPC_SERVER_ACTIVE");
            if (hEvent == NULL)
            {
               ERR("WL: Could not open the notification event (Error %lu)\n", GetLastError());
               return;
            }
        }
    }

    TRACE("WL: Wait for the LSA server!\n");
    WaitForSingleObject(hEvent, INFINITE);
    TRACE("WL: LSA server running!\n");

    CloseHandle(hEvent);
}


static BOOL
InitKeyboardLayouts()
{
    WCHAR wszKeyName[12], wszKLID[10];
	DWORD dwSize = sizeof(wszKLID), dwType, i;
    HKEY hKey;
    UINT Flags;
    BOOL bRet = FALSE;

    /* Open registry key with preloaded layouts */
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
	    ERR("RegOpenKeyExW failed!\n");
	    return FALSE;
	}

    i = 1;
    while(TRUE)
    {
        /* Read values with integer names only */
        swprintf(wszKeyName, L"%d", i);
        if (RegQueryValueExW(hKey, wszKeyName, NULL, &dwType, (LPBYTE)wszKLID, &dwSize) != ERROR_SUCCESS)
        {
            /* If we loaded at least one layout and there is no more
               registry values return TRUE */
            if (i > 1)
                bRet = TRUE;
            break;
        }

        /* Only REG_SZ values are valid */
        if (dwType != REG_SZ)
        {
            ERR("Wrong type!\n");
            break;
        }

        /* Load keyboard layout with given locale id */
        Flags = KLF_SUBSTITUTE_OK;
        if (i > 1)
            Flags |= KLF_NOTELLSHELL|KLF_REPLACELANG;
        else // First layout
            Flags |= KLF_ACTIVATE; // |0x40000000
        if (!LoadKeyboardLayoutW(wszKLID, Flags))
        {
            ERR("LoadKeyboardLayoutW failed!\n");
            break;
        }

        /* Move to the next entry */
        ++i;
    }

    /* Close the key now */
	RegCloseKey(hKey);

    return bRet;
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
	HANDLE hThread;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nShowCmd);

	hAppInstance = hInstance;

	if (!RegisterLogonProcess(GetCurrentProcessId(), TRUE))
	{
		ERR("WL: Could not register logon process\n");
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

    /* Load default keyboard layouts */
    if (!InitKeyboardLayouts())
    {
        ERR("WL: Could not preload keyboard layouts\n");
		NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, NULL, OptionOk, &HardErrorResponse);
		ExitProcess(1);
    }

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


	/* Wait for the LSA server */
	WaitForLsass();

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

	/* Play logon sound */
	hThread = CreateThread(NULL, 0, PlayLogonSoundThread, NULL, 0, NULL);
	if (hThread)
	{
		CloseHandle(hThread);
	}

	/* Tell kernel that CurrentControlSet is good (needed
	 * to support Last good known configuration boot) */
	NtInitializeRegistry(CM_BOOT_FLAG_ACCEPTED | 1);

	/* Message loop for the SAS window */
	while (GetMessageW(&Msg, WLSession->SASWindow, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessageW(&Msg);
	}

	/* We never go there */

	return 0;
}
