/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            services/winlogon/winlogon.c
 * PURPOSE:         Logon
 * PROGRAMMERS:     Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Filip Navara
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES *****************************************************************/
#include "winlogon.h"

#define YDEBUG
#include <wine/debug.h>

/* GLOBALS ******************************************************************/

HINSTANCE hAppInstance;
PWLSESSION WLSession = NULL;

/* FUNCTIONS *****************************************************************/

static INT_PTR CALLBACK
ShutdownComputerWindowProc(
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
				case IDC_BTNSHTDOWNCOMPUTER:
					EndDialog(hwndDlg, IDC_BTNSHTDOWNCOMPUTER);
					break;
			}
			break;
		}
		case WM_INITDIALOG:
		{
			RemoveMenu(GetSystemMenu(hwndDlg, FALSE), SC_CLOSE, MF_BYCOMMAND);
			SetFocus(GetDlgItem(hwndDlg, IDC_BTNSHTDOWNCOMPUTER));
			break;
		}
	}
	return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

static BOOL
StartServicesManager(VOID)
{
   HANDLE ServicesInitEvent;
   BOOLEAN Result;
   STARTUPINFO StartupInfo;
   PROCESS_INFORMATION ProcessInformation;
   DWORD Count;
   WCHAR ServiceString[] = L"services.exe";

   /* Start the service control manager (services.exe) */

   StartupInfo.cb = sizeof(StartupInfo);
   StartupInfo.lpReserved = NULL;
   StartupInfo.lpDesktop = NULL;
   StartupInfo.lpTitle = NULL;
   StartupInfo.dwFlags = 0;
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved2 = 0;

#if 0
   DPRINT1(L"WL: Creating new process - \"services.exe\".\n");
#endif

   Result = CreateProcess(NULL,
                          ServiceString,
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
        DPRINT1("WL: Failed to execute services\n");
        return FALSE;
     }

   /* wait for event creation (by SCM) for max. 20 seconds */
   for (Count = 0; Count < 20; Count++)
     {
        Sleep(1000);

        DPRINT("WL: Attempting to open event \"SvcctrlStartEvent_A3725DX\"\n");
        ServicesInitEvent = OpenEvent(EVENT_ALL_ACCESS, //SYNCHRONIZE,
                                      FALSE,
                                      L"SvcctrlStartEvent_A3725DX");
        if (ServicesInitEvent != NULL)
          {
             break;
          }
     }

   if (ServicesInitEvent == NULL)
     {
        DPRINT1("WL: Failed to open event \"SvcctrlStartEvent_A3725DX\"\n");
        return FALSE;
     }

   /* wait for event signalization */
   DPRINT("WL: Waiting forever on event handle: %x\n", ServicesInitEvent);
   WaitForSingleObject(ServicesInitEvent, INFINITE);
   DPRINT("WL: Closing event object \"SvcctrlStartEvent_A3725DX\"\n");
   CloseHandle(ServicesInitEvent);
   DPRINT("WL: StartServicesManager() Done.\n");

   return TRUE;
}

static BOOL
StartCustomService(
	IN LPCWSTR ServiceName)
{
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;
	BOOL ret = FALSE;

	hSCManager = OpenSCManager(NULL, NULL, 0);
	if (!hSCManager)
		goto cleanup;

	hService = OpenService(hSCManager, ServiceName, SERVICE_START);
	if (!hService)
		goto cleanup;
#if 0
	if (!StartService(hService, 0, NULL))
		goto cleanup;
#endif

	ret = TRUE;

cleanup:
	if (hService)
		CloseServiceHandle(hService);
	if (hSCManager)
		CloseServiceHandle(hSCManager);
	return ret;
}

static BOOL
StartLsass(VOID)
{
	HANDLE LsassInitEvent;

	LsassInitEvent = CreateEvent(
		NULL,
		TRUE,
		FALSE,
		L"Global\\SECURITY_SERVICES_STARTED");
	if (!LsassInitEvent)
	{
		ERR("WL: Failed to create lsass notification event (error %lu)\n", GetLastError());
		return FALSE;
	}

	/* Start the local security authority subsystem (Netlogon service) */
	if (!StartCustomService(L"Netlogon"))
	{
		ERR("WL: Failed to start NetLogon service (error %lu)\n", GetLastError());
		return FALSE;
	}

#if 0
	WaitForSingleObject(LsassInitEvent, INFINITE);
#endif
	CloseHandle(LsassInitEvent);

	return TRUE;
}

#if 0
static BOOL
OpenRegistryKey(
	OUT HKEY *WinLogonKey)
{
   return ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                        L"SOFTWARE\\ReactOS\\Windows NT\\CurrentVersion\\WinLogon",
                                        0,
                                        KEY_QUERY_VALUE,
                                        WinLogonKey);
}
#endif

#if 0
static BOOL
StartProcess(
	IN PWCHAR ValueName)
{
   BOOL StartIt;
   HKEY WinLogonKey;
   DWORD Type;
   DWORD Size;
   DWORD StartValue;

   StartIt = TRUE;
   if (OpenRegistryKey(&WinLogonKey))
     {
	Size = sizeof(DWORD);
	if (ERROR_SUCCESS == RegQueryValueEx(WinLogonKey,
                                             ValueName,
	                                     NULL,
	                                     &Type,
	                                     (LPBYTE) &StartValue,
                                             &Size))
	   {
	   if (REG_DWORD == Type)
	     {
		StartIt = (0 != StartValue);
	     }
	   }
	RegCloseKey(WinLogonKey);
     }

   return StartIt;
}
#endif

/*
static BOOL RestartShell(
	IN OUT PWLSESSION Session)
{
  HKEY WinLogonKey;
  DWORD Type, Size, Value;

  if(OpenRegistryKey(&WinLogonKey))
  {
    Size = sizeof(DWORD);
    if(ERROR_SUCCESS == RegQueryValueEx(WinLogonKey,
                                        L"AutoRestartShell",
                                        NULL,
                                        &Type,
                                        (LPBYTE)&Value,
                                        &Size))
    {
      if(Type == REG_DWORD)
      {
        RegCloseKey(WinLogonKey);
        return (Value != 0);
      }
    }
    RegCloseKey(WinLogonKey);
  }
  return FALSE;
}
*/

#if 0
static PWCHAR
GetUserInit(
	OUT WCHAR *CommandLine,
	IN DWORD BufferLength)
{
   HKEY WinLogonKey;
   BOOL GotCommandLine;
   DWORD Type;
   DWORD Size;
   WCHAR Shell[_MAX_PATH];

   GotCommandLine = FALSE;
   if (OpenRegistryKey(&WinLogonKey))
     {
	Size = MAX_PATH;
	if (ERROR_SUCCESS == RegQueryValueEx(WinLogonKey,
                                         L"UserInit",
	                                     NULL,
	                                     &Type,
	                                     (LPBYTE) Shell,
                                         &Size))
	   {
	   if (REG_EXPAND_SZ == Type)
	     {
		ExpandEnvironmentStrings(Shell, CommandLine, _MAX_PATH);
		GotCommandLine = TRUE;
	     }
	   else if (REG_SZ == Type)
	     {
		wcscpy(CommandLine, Shell);
		GotCommandLine = TRUE;
	     }
	   }
	RegCloseKey(WinLogonKey);
     }

   if (! GotCommandLine)
     {
	GetSystemDirectory(CommandLine, MAX_PATH - 15);
	wcscat(CommandLine, L"\\userinit.exe");
     }

   return CommandLine;
}

static BOOL
DoBrokenLogonUser(
	IN PWLSESSION WLSession,
	IN PWLX_MPR_NOTIFY_INFO pMprNotifyInfo)
{
  PROCESS_INFORMATION ProcessInformation;
  STARTUPINFO StartupInfo;
  WCHAR CommandLine[MAX_PATH];
  WCHAR CurrentDirectory[MAX_PATH];
  PROFILEINFOW ProfileInfo;
  BOOL Result;
  LPVOID lpEnvironment = NULL;
  MSG Msg;
  BOOLEAN Old;

  SwitchDesktop(WLSession->ApplicationDesktop);

  /* Load the user profile */
  ProfileInfo.dwSize = sizeof(PROFILEINFOW);
  ProfileInfo.dwFlags = 0;
  ProfileInfo.lpUserName = pMprNotifyInfo->pszUserName;
  ProfileInfo.lpProfilePath = NULL;
  ProfileInfo.lpDefaultPath = NULL;
  ProfileInfo.lpServerName = NULL;
  ProfileInfo.lpPolicyPath = NULL;
  ProfileInfo.hProfile = NULL;

  if (!LoadUserProfileW (WLSession->UserToken,
			 &ProfileInfo))
    {
      DPRINT1 ("WL: LoadUserProfileW() failed\n");
      CloseHandle (WLSession->UserToken);
      RtlDestroyEnvironment (lpEnvironment);
      return FALSE;
    }

  if (!CreateEnvironmentBlock (&lpEnvironment,
			       WLSession->UserToken,
			       TRUE))
    {
      DPRINT1("WL: CreateEnvironmentBlock() failed\n");
      return FALSE;
    }

  if (ImpersonateLoggedOnUser(WLSession->UserToken))
    {
      UpdatePerUserSystemParameters(0, TRUE);
      RevertToSelf();
    }

  GetWindowsDirectoryW (CurrentDirectory, MAX_PATH);

  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.lpReserved = NULL;
  StartupInfo.lpDesktop = NULL;
  StartupInfo.lpTitle = NULL;
  StartupInfo.dwFlags = 0;
  StartupInfo.cbReserved2 = 0;
  StartupInfo.lpReserved2 = 0;
  
  /* Get privilege */
  RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, TRUE, FALSE, &Old);

  Result = CreateProcessAsUserW(
				 WLSession->UserToken,
				 NULL,
				 GetUserInit (CommandLine, MAX_PATH),
				 NULL,
				 NULL,
				 FALSE,
				 CREATE_UNICODE_ENVIRONMENT,
				 lpEnvironment,
				 CurrentDirectory,
				 &StartupInfo,
				 &ProcessInformation);
  if (!Result)
    {
      DPRINT1("WL: Failed to execute user shell %ws\n", CommandLine);
      if (ImpersonateLoggedOnUser(WLSession->UserToken))
        {
          UpdatePerUserSystemParameters(0, FALSE);
          RevertToSelf();
        }
      UnloadUserProfile (WLSession->UserToken,
			 ProfileInfo.hProfile);
      CloseHandle (WLSession->UserToken);
      DestroyEnvironmentBlock (lpEnvironment);
      return FALSE;
    }
  /*WLSession->MsGina.Functions.WlxActivateUserShell(WLSession->MsGina.Context,
                                                   L"WinSta0\\Default",
                                                   NULL,
                                                   NULL);*/

  while (WaitForSingleObject (ProcessInformation.hProcess, 100) != WAIT_OBJECT_0)
  {
    if (PeekMessage(&Msg, WLSession->SASWindow, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
    }
  }

  CloseHandle (ProcessInformation.hProcess);
  CloseHandle (ProcessInformation.hThread);

  if (ImpersonateLoggedOnUser(WLSession->UserToken))
    {
      UpdatePerUserSystemParameters(0, FALSE);
      RevertToSelf();
    }

  /* Unload user profile */
  UnloadUserProfile (WLSession->UserToken,
		     ProfileInfo.hProfile);

  CloseHandle (WLSession->UserToken);

  RtlDestroyEnvironment (lpEnvironment);

  return TRUE;
}
#endif

static BOOL
DisplayStatusMessage(
	IN PWLSESSION Session,
	IN HDESK hDesktop,
	IN UINT ResourceId)
{
	WCHAR StatusMsg[MAX_PATH];

	if (Session->SuppressStatus)
		return TRUE;

	if (LoadString(hAppInstance, ResourceId, StatusMsg, MAX_PATH) == 0)
		return FALSE;

	return Session->MsGina.Functions.WlxDisplayStatusMessage(Session->MsGina.Context, hDesktop, 0, NULL, StatusMsg);
}

static VOID
SessionLoop(
	IN OUT PWLSESSION Session)
{
	//WCHAR StatusMsg[256];
	//HANDLE hShutdownEvent;
	MSG Msg;

	Session->LogonStatus = WKSTA_IS_LOGGED_OFF;
	RemoveStatusMessage(Session);
	DispatchSAS(Session, WLX_SAS_TYPE_TIMEOUT);

	/* Message loop for the SAS window */
	while (GetMessage(&Msg, WLSession->SASWindow, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	/* Don't go there! */

   /*
   DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_PREPARENETWORKCONNECTIONS);
   Sleep(150);

   DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_APPLYINGCOMPUTERSETTINGS);
   Sleep(150);

   DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_LOADINGYOURPERSONALSETTINGS);
   Sleep(150);

   DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_APPLYINGYOURPERSONALSETTINGS);
   Sleep(150);

   RemoveStatusMessage(Session);

   if(!MsGinaInst->Functions->WlxActivateUserShell(MsGinaInst->Context,
                                                   L"WinSta0\\Default",
                                                   NULL,
                                                   NULL))
   {
     LoadString(hAppInstance, IDS_FAILEDACTIVATEUSERSHELL, StatusMsg, 256 * sizeof(WCHAR));
     MessageBox(0, StatusMsg, NULL, MB_ICONERROR);
     SetEvent(hShutdownEvent);
   }

   WaitForSingleObject(hShutdownEvent, INFINITE);
   CloseHandle(hShutdownEvent);

   DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_SAVEYOURSETTINGS);

   Sleep(150);

   MsGinaInst->Functions->WlxShutdown(MsGinaInst->Context, WLX_SAS_ACTION_SHUTDOWN);
   DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_REACTOSISSHUTTINGDOWN);

   Sleep(250);

   RemoveStatusMessage(Session);
   */
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

	hAppInstance = hInstance;

	if (!RegisterLogonProcess(GetCurrentProcessId(), TRUE))
	{
		ERR("WL: Could not register logon process\n");
		NtShutdownSystem(ShutdownNoReboot);
		ExitProcess(0);
		return 0;
	}

	WLSession = (PWLSESSION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WLSESSION));
	if (!WLSession)
	{
		NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, 0, 0, 0);
		ExitProcess(1);
		return 1;
	}
	WLSession->DialogTimeout = 120; /* 2 minutes */

	if (!CreateWindowStationAndDesktops(WLSession))
	{
		NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, 0, 0, 0);
		ExitProcess(1);
		return 1;
	}
	LockWorkstation(WLSession);

	/* Check for pending setup */
	if (GetSetupType() != 0)
	{
		DPRINT("Winlogon: CheckForSetup() in setup mode\n");

		/* Run setup and reboot when done */
		RemoveStatusMessage(WLSession);
		SwitchDesktop(WLSession->ApplicationDesktop);
		RunSetup();

		NtShutdownSystem(ShutdownReboot);
		ExitProcess(0);
		return 0;
	}

	/* Load and initialize gina */
	if (!GinaInit(WLSession))
	{
		ERR("WL: Failed to initialize Gina\n");
		NtShutdownSystem(ShutdownNoReboot);
		ExitProcess(0);
		return 0;
	}

	DisplayStatusMessage(WLSession, WLSession->WinlogonDesktop, IDS_REACTOSISSTARTINGUP);

	if (!StartServicesManager())
	{
		ERR("WL: Could not start services.exe\n");
		NtShutdownSystem(ShutdownNoReboot);
		ExitProcess(0);
		return 0;
	}

	if (!StartLsass())
	{
		DPRINT1("WL: Failed to start lsass.exe service (error %lu)\n", GetLastError());
		return 1;
	}

#if 0
	/* Connect to NetLogon service (lsass.exe) */
	/* Real winlogon uses "Winlogon" */
	RtlInitUnicodeString((PUNICODE_STRING)&ProcessName, L"Winlogon");
	Status = LsaRegisterLogonProcess(&ProcessName, &LsaHandle, &Mode);
	if (Status == STATUS_PORT_CONNECTION_REFUSED)
	{
		/* Add the 'SeTcbPrivilege' privilege and try again */
		RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, TRUE, &Old);
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
		return 2;
	}

	/* Main loop */
	SessionLoop(WLSession);

   /* FIXME - Flush disks and registry, ... */

   if(WLSession->LogonStatus == 0)
   {
     /* FIXME - only show this dialog if it's a shutdown and the computer doesn't support APM */
     switch(DialogBox(hInstance, MAKEINTRESOURCE(IDD_SHUTDOWNCOMPUTER), 0, ShutdownComputerWindowProc))
     {
       case IDC_BTNSHTDOWNCOMPUTER:
         NtShutdownSystem(ShutdownReboot);
         break;
       default:
         NtShutdownSystem(ShutdownNoReboot);
         break;
     }
     ExitProcess(0);
   }
   else
   {
     DPRINT1("WL: LogonStatus != LOGON_SHUTDOWN!!!\n");
     ExitProcess(0);
   }

   return 0;
}
