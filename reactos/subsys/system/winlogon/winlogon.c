/* $Id: winlogon.c,v 1.29 2004/03/28 12:21:41 weiden Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/winlogon/winlogon.c
 * PURPOSE:         Logon 
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <stdio.h>
#include <ntsecapi.h>
#include <wchar.h>
#include <userenv.h>

#include "setup.h"
#include "winlogon.h"
#include "resource.h"

#define NDEBUG
#include <debug.h>

#define SUPPORT_CONSOLESTART 1
#define START_LSASS          0

/* GLOBALS ******************************************************************/

BOOL
LoadGina(PMSGINAFUNCTIONS Functions, DWORD *DllVersion);
PWLSESSION
MsGinaInit(void);
void
SessionLoop(PWLSESSION Session);
BOOL
InitServices(void);
BOOL
WlxCreateWindowStationAndDesktops(PWLSESSION Session);

HINSTANCE hAppInstance;
PWLSESSION WLSession = NULL;

#if SUPPORT_CONSOLESTART
BOOL StartConsole = TRUE;
#endif

/* FUNCTIONS *****************************************************************/

static void
PrintString (WCHAR* fmt,...)
{
   WCHAR buffer[512];
   va_list ap;

   va_start(ap, fmt);
   wsprintf(buffer, fmt, ap);
   va_end(ap);

   OutputDebugString(buffer);
}


BOOL CALLBACK
ShutdownComputerProc (HWND hwndDlg,
		      UINT uMsg,
		      WPARAM wParam,
		      LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
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
  return FALSE;
}

static BOOLEAN
StartServices (VOID)
{
   HANDLE ServicesInitEvent;
   BOOLEAN Result;
   STARTUPINFO StartupInfo;
   PROCESS_INFORMATION ProcessInformation;
   DWORD Count;

   /* Start the service control manager (services.exe) */   
      
   StartupInfo.cb = sizeof(StartupInfo);
   StartupInfo.lpReserved = NULL;
   StartupInfo.lpDesktop = NULL;
   StartupInfo.lpTitle = NULL;
   StartupInfo.dwFlags = 0;
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved2 = 0;

#if 0   
   PrintString(L"WL: Creating new process - \"services.exe\".\n");
#endif

   Result = CreateProcess(L"services.exe",
                          NULL,
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
        PrintString(L"WL: Failed to execute services\n");
        return FALSE;
     }
   
   /* wait for event creation (by SCM) for max. 20 seconds */
   for (Count = 0; Count < 20; Count++)
     {
        Sleep(1000);
   
        //DbgPrint("WL: Attempting to open event \"SvcctrlStartEvent_A3725DX\"\n");
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
        DbgPrint("WL: Failed to open event \"SvcctrlStartEvent_A3725DX\"\n");
        return FALSE;
     }

   /* wait for event signalization */
   //DbgPrint("WL: Waiting forever on event handle: %x\n", ServicesInitEvent);
   WaitForSingleObject(ServicesInitEvent, INFINITE);
   //DbgPrint("WL: Closing event object \"SvcctrlStartEvent_A3725DX\"\n");
   CloseHandle(ServicesInitEvent);
   //DbgPrint("WL: StartServices() Done.\n");
      
   return TRUE;
}

#if START_LSASS
static BOOLEAN
StartLsass (VOID)
{
   HANDLE LsassInitEvent;
   BOOLEAN Result;
   STARTUPINFO StartupInfo;
   PROCESS_INFORMATION ProcessInformation;
   
   LsassInitEvent = CreateEvent(NULL,
                                TRUE,
                                FALSE,
                                L"\\LsassInitDone");
   
   if (LsassInitEvent == NULL)
     {
        DbgPrint("WL: Failed to create lsass notification event\n");
        return(FALSE);
     }
   
   /* Start the local security authority subsystem (lsass.exe) */
   
   StartupInfo.cb = sizeof(StartupInfo);
   StartupInfo.lpReserved = NULL;
   StartupInfo.lpDesktop = NULL;
   StartupInfo.lpTitle = NULL;
   StartupInfo.dwFlags = 0;
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved2 = 0;
   
   Result = CreateProcess(L"lsass.exe",
                          NULL,
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
        DbgPrint("WL: Failed to execute lsass\n");
        return(FALSE);
     }
   
   DPRINT("WL: Waiting for lsass\n");
   WaitForSingleObject(LsassInitEvent, INFINITE);
   CloseHandle(LsassInitEvent);
   
   return(TRUE);
}
#endif


static BOOLEAN
OpenRegistryKey (HANDLE *WinLogonKey)
{
   return ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                        L"SOFTWARE\\ReactOS\\Windows NT\\CurrentVersion\\WinLogon",
                                        0,
                                        KEY_QUERY_VALUE,
                                        WinLogonKey);
}


static BOOLEAN StartProcess(PWCHAR ValueName)
{
   BOOL StartIt;
   HANDLE WinLogonKey;
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

/*
static BOOL RestartShell(void)
{
  HANDLE WinLogonKey;
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

#if SUPPORT_CONSOLESTART
static BOOL StartIntoGUI(VOID)
{
  HANDLE WinLogonKey;
  DWORD Type, Size, Value;
  
  if(OpenRegistryKey(&WinLogonKey))
  {
    Size = sizeof(DWORD);
    if(ERROR_SUCCESS == RegQueryValueEx(WinLogonKey,
                                        L"StartGUI",
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


static PWCHAR
GetShell (WCHAR *CommandLine)
{
   HANDLE WinLogonKey;
   BOOL GotCommandLine;
   DWORD Type;
   DWORD Size;
   WCHAR Shell[_MAX_PATH];

   GotCommandLine = FALSE;
   if (OpenRegistryKey(&WinLogonKey))
     {
	Size = MAX_PATH;
	if (ERROR_SUCCESS == RegQueryValueEx(WinLogonKey,
                                         L"Shell",
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
	GetWindowsDirectory(CommandLine, MAX_PATH - 15);
	wcscat(CommandLine, L"\\explorer.exe");
     }

   return CommandLine;
}


static BOOL
DoLogonUser (PWCHAR Name,
	     PWCHAR Password)
{
  PROCESS_INFORMATION ProcessInformation;
  STARTUPINFO StartupInfo;
  WCHAR CommandLine[MAX_PATH];
  WCHAR CurrentDirectory[MAX_PATH];
  HANDLE hToken;
  PROFILEINFOW ProfileInfo;
  BOOL Result;
  LPVOID lpEnvironment = NULL;

  Result = LogonUserW (Name,
		       NULL,
		       Password,
		       LOGON32_LOGON_INTERACTIVE,
		       LOGON32_PROVIDER_DEFAULT,
		       &hToken);
  if (!Result)
    {
      DbgPrint ("WL: LogonUserW() failed\n");
      RtlDestroyEnvironment (lpEnvironment);
      return FALSE;
    }

  /* Load the user profile */
  ProfileInfo.dwSize = sizeof(PROFILEINFOW);
  ProfileInfo.dwFlags = 0;
  ProfileInfo.lpUserName = Name;
  ProfileInfo.lpProfilePath = NULL;
  ProfileInfo.lpDefaultPath = NULL;
  ProfileInfo.lpServerName = NULL;
  ProfileInfo.lpPolicyPath = NULL;
  ProfileInfo.hProfile = NULL;

  if (!LoadUserProfileW (hToken,
			 &ProfileInfo))
    {
      DbgPrint ("WL: LoadUserProfileW() failed\n");
      CloseHandle (hToken);
      RtlDestroyEnvironment (lpEnvironment);
      return FALSE;
    }

  if (!CreateEnvironmentBlock (&lpEnvironment,
			       hToken,
			       TRUE))
    {
      DbgPrint ("WL: CreateEnvironmentBlock() failed\n");
      return FALSE;
    }

  GetWindowsDirectoryW (CurrentDirectory, MAX_PATH);

  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.lpReserved = NULL;
  StartupInfo.lpDesktop = NULL;
  StartupInfo.lpTitle = NULL;
  StartupInfo.dwFlags = 0;
  StartupInfo.cbReserved2 = 0;
  StartupInfo.lpReserved2 = 0;

  Result = CreateProcessAsUserW (hToken,
				 NULL,
				 GetShell (CommandLine),
				 NULL,
				 NULL,
				 FALSE,
				 CREATE_NEW_CONSOLE,// | CREATE_UNICODE_ENVIRONMENT,
				 lpEnvironment, // NULL,
				 CurrentDirectory,
				 &StartupInfo,
				 &ProcessInformation);
  if (!Result)
    {
      DbgPrint ("WL: Failed to execute user shell %s\n", CommandLine);
      UnloadUserProfile (hToken,
			 ProfileInfo.hProfile);
      CloseHandle (hToken);
      DestroyEnvironmentBlock (lpEnvironment);
      return FALSE;
    }

  WaitForSingleObject (ProcessInformation.hProcess, INFINITE);
  CloseHandle (ProcessInformation.hProcess);
  CloseHandle (ProcessInformation.hThread);

  /* Unload user profile */
  UnloadUserProfile (hToken,
		     ProfileInfo.hProfile);

  CloseHandle (hToken);

  RtlDestroyEnvironment (lpEnvironment);

  return TRUE;
}
#endif


int STDCALL
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nShowCmd)
{
#if SUPPORT_CONSOLESTART
//  WCHAR LoginName[255];
//  WCHAR Password[255];
#endif
#if 0
  LSA_STRING ProcessName, PackageName;
  HANDLE LsaHandle;
  LSA_OPERATIONAL_MODE Mode;
  ULONG AuthenticationPackage;
#endif
  NTSTATUS Status;
  
  hAppInstance = hInstance;
  
#if START_LSASS
   if (StartProcess(L"StartLsass"))
     {
	if (!StartLsass())
	  {
	     DbgPrint("WL: Failed to start LSASS (0x%X)\n", GetLastError());
	  }
     }
#endif
  
  if(!(WLSession = MsGinaInit()))
  {
    DbgPrint("WL: Failed to initialize msgina.dll\n");
    NtShutdownSystem(ShutdownNoReboot);
    ExitProcess(0);
    return 0;
  }
  
  WLSession->LogonStatus = LOGON_INITIALIZING;
#if START_LSASS
  if(!RegisterLogonProcess(GetCurrentProcessId(), TRUE))
  {
    DbgPrint("WL: Could not register logon process\n");
    NtShutdownSystem(ShutdownNoReboot);
    ExitProcess(0);
    return 0;
  }
#endif
  
  if(!WlxCreateWindowStationAndDesktops(WLSession))
  {
    NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, 0, 0, 0);
    ExitProcess(1);
    return 1;
  }
  
  /*
   * Switch to winlogon desktop
   */
  /* FIXME: Do start up in the application desktop for now. */
  Status = NtSetInformationProcess(NtCurrentProcess(),
                                   ProcessDesktop,
                                   &WLSession->ApplicationDesktop,
                                   sizeof(HDESK));
  if(!NT_SUCCESS(Status))
  {
    DbgPrint("WL: Cannot set default desktop for winlogon.\n");
  }
  SetThreadDesktop(WLSession->ApplicationDesktop);
  if(!SwitchDesktop(WLSession->ApplicationDesktop))
  {
    DbgPrint("WL: Cannot switch to Winlogon desktop (0x%X)\n", GetLastError());
  }
  
  /* Check for pending setup */
  if (GetSetupType () != 0)
  {
    DPRINT ("Winlogon: CheckForSetup() in setup mode\n");
    
    /* Run setup and reboot when done */
    RunSetup();
    
    NtShutdownSystem(ShutdownReboot);
    ExitProcess(0);
    return 0;
  }
  
#if SUPPORT_CONSOLESTART
 StartConsole = !StartIntoGUI();
 if(!StartConsole)
 {
#endif
  if(!InitializeSAS(WLSession))
  {
    DbgPrint("WL: Failed to initialize SAS\n");
    ExitProcess(2);
    return 2;
  }
#if SUPPORT_CONSOLESTART
 }
#endif
  
  InitServices();
   
#if 0
   /* real winlogon uses "Winlogon" */
   RtlInitUnicodeString((PUNICODE_STRING)&ProcessName, L"Winlogon");
   Status = LsaRegisterLogonProcess(&ProcessName, &LsaHandle, &Mode);
   if (!NT_SUCCESS(Status))
   {
     switch(Status)
     {
       case STATUS_PORT_CONNECTION_REFUSED:
         /* FIXME - we don't have the 'SeTcbPrivilege' pivilege, so set it or call
                    LsaAddAccountRights() and try again */
         DbgPrint("WL: LsaRegisterLogonProcess() returned STATUS_PORT_CONNECTION_REFUSED\n");
         break;
       case STATUS_NAME_TOO_LONG:
         DbgPrint("WL: LsaRegisterLogonProcess() returned STATUS_NAME_TOO_LONG\n");
         break;
       default:
         DbgPrint("WL: Failed to connect to LSASS\n");
         break;
     }
     return(1);
   }
   
   RtlInitUnicodeString((PUNICODE_STRING)&PackageName, L"Kerberos");
   Status = LsaLookupAuthenticationPackage(LsaHandle, &PackageName, &AuthenticationPackage);
   if (!NT_SUCCESS(Status))
   {
     LsaDeregisterLogonProcess(LsaHandle);
     DbgPrint("WL: Failed to lookup authentication package\n");
     return(1);
   }
#endif
   
   /* FIXME: Create a window class and associate a Winlogon
    *        window procedure with it.
    *        Register SAS with the window.
    *        Register for logoff notification
    */
   
   /* Main loop */
#if 0
   /* Display login prompt */
   WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
                LoginPrompt,
                strlen(LoginPrompt),  // wcslen(LoginPrompt),
                &Result,
                NULL);
   i = 0;
   do
     {
       ReadConsole(GetStdHandle(STD_INPUT_HANDLE),
                   &LoginName[i],
                   1,
                   &Result,
                   NULL);
       i++;
     } while (LoginName[i - 1] != '\n');
   LoginName[i - 1] = 0;
       
   /* Display password prompt */
   WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
                PasswordPrompt,
                strlen(PasswordPrompt),  // wcslen(PasswordPrompt),
                &Result,
                NULL);
   i = 0;
   do
     {
       ReadConsole(GetStdHandle(STD_INPUT_HANDLE),
                   &Password[i],
                   1,
                   &Result,
                   NULL);
       i++;
     } while (Password[i - 1] != '\n');
   Password[i - 1] =0;
#endif

#if SUPPORT_CONSOLESTART
 if(StartConsole)
 {
//   if (! DoLogonUser(LoginName, Password))
   if (! DoLogonUser(L"Administrator", L"Secret"))
     {
     }
   
   NtShutdownSystem(ShutdownNoReboot);
   ExitProcess(0);
 }
 else
 {
#endif
  
  SessionLoop(WLSession);
  
   /* FIXME - Flush disks and registry, ... */
   
   if(WLSession->LogonStatus == LOGON_SHUTDOWN)
   {
     /* FIXME - only show this dialog if it's a shutdown and the computer doesn't support APM */
     switch(DialogBox(hInstance, MAKEINTRESOURCE(IDD_SHUTDOWNCOMPUTER), 0, ShutdownComputerProc))
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
     DbgPrint("WL: LogonStatus != LOGON_SHUTDOWN!!!\n");
     ExitProcess(0);
   }
#if SUPPORT_CONSOLESTART
 }
#endif
   
   return 0;
}

BOOL
DisplayStatusMessage(PWLSESSION Session, HDESK hDesktop, DWORD dwOptions, PWSTR pTitle, PWSTR pMessage)
{
  if(Session->SuppressStatus)
  {
    return TRUE;
  }
  
  #if SUPPORT_CONSOLESTART
  if(StartConsole)
  {
    if(pMessage)
    {
      DbgPrint("WL-Status: %ws\n", pMessage);
    }
    return TRUE;
  }
  #endif
  
  return Session->MsGina.Functions.WlxDisplayStatusMessage(Session->MsGina.Context, hDesktop, dwOptions, pTitle, pMessage);
}

BOOL
InitServices(void)
{
  WCHAR StatusMsg[256];

  LoadString(hAppInstance, IDS_REACTOSISSTARTINGUP, StatusMsg, 256 * sizeof(WCHAR));
  DisplayStatusMessage(WLSession, WLSession->ApplicationDesktop, 0, NULL, StatusMsg);

  /* start system processes (services.exe & lsass.exe) */
  if(StartProcess(L"StartServices"))
  {
	if(!StartServices())
    {
      DbgPrint("WL: Failed to start Services (0x%X)\n", GetLastError());
    }
  }
  
  return TRUE;
}

DWORD
DoLogin(PWLSESSION Session)
{
  DWORD WlxAction, Options;
  WLX_MPR_NOTIFY_INFO MprNotifyInfo;
  PWLX_PROFILE_V2_0 Profile;
  PSID LogonSid;
  HANDLE Token;
  
  /* FIXME - Create a Logon Sid
  if(!(LogonSid = CreateUserLogonSid(NULL)))
  {
    return WLX_SAS_ACTION_NONE;
  }
  */
  
  Options = 0;
  WlxAction = Session->MsGina.Functions.WlxLoggedOutSAS(Session->MsGina.Context,
                                                        Session->SASAction,
                                                        &Session->LogonId,
                                                        LogonSid,
                                                        &Options,
                                                        &Token,
                                                        &MprNotifyInfo,
                                                        (PVOID*)&Profile);
  
  return WlxAction;
}

void
SessionLoop(PWLSESSION Session)
{
  //WCHAR StatusMsg[256];
 // HANDLE hShutdownEvent;
  DWORD WlxAction;
  MSG Msg;
  
  WlxAction = WLX_SAS_ACTION_NONE;
  Session->LogonStatus = LOGON_NONE;
  while(WlxAction == WLX_SAS_ACTION_NONE)
  {
    RemoveStatusMessage(Session);
    if(Session->LogonStatus == LOGON_NONE)
    {
      Session->LogonStatus = LOGON_SHOWINGLOGON;
      /* we're ready to display a logon window,
         don't timeout dialogboxes here */
      WlxSetTimeout(Session->MsGina.Context, 0);
      Session->SuppressStatus = TRUE;
      /* tell msgina to show a window telling the user one can logon */
      #if SUPPORT_CONSOLESTART
      if(!StartConsole)
      #endif
      DisplaySASNotice(Session);
      Session->SuppressStatus = FALSE;
      
      if(Session->SASAction == WLX_SAS_ACTION_LOGOFF)
      {
        /* the system wants to log off here */
        Session->LogonStatus = LOGON_SHUTDOWN;
        break;
      }
    }
    
    WlxAction = DoLogin(Session);
    if(WlxAction == WLX_SAS_ACTION_LOGOFF)
    {
      /* the user doesn't want to login, instead pressed cancel
         we should display the window again so one can logon again */
      /* FIXME - disconnect any connections in case we did a remote logon */
      DbgPrint("WL: DoLogin failed\n");
      WlxAction = WLX_SAS_ACTION_NONE;
    }
    if(WlxAction == WLX_SAS_ACTION_NONE)
    {
      if(Session->SASAction == WLX_SAS_ACTION_LOGOFF)
      {
        /* system is about to shut down, leave the main loop */
        Session->LogonStatus = LOGON_SHUTDOWN;
        break;
      }
      Session->LogonStatus = LOGON_NONE;
      continue;
    }
    
    /* FIXME - don't leave the loop when suspending the computer */
    if(WLX_SUSPENDING(WlxAction))
    {
      Session->LogonStatus = LOGON_NONE;
      WlxAction = WLX_SAS_ACTION_NONE;
      /* don't leave the loop */
      continue;
    }
    
    if(WLX_SHUTTINGDOWN(WlxAction))
    {
      Session->LogonStatus = LOGON_SHUTDOWN;
      /* leave the loop here */
      break;
    }
    
    /* Message loop for the SAS window */
    while(GetMessage(&Msg, 0, 0, 0))
    {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
    }
  }
   /*
   LoadString(hAppInstance, IDS_PREPARENETWORKCONNECTIONS, StatusMsg, 256 * sizeof(WCHAR));
   MsGinaInst->Functions->WlxDisplayStatusMessage(MsGinaInst->Context,
                                                  ApplicationDesktop,
                                                  0,
                                                  NULL,
                                                  StatusMsg);
   

   Sleep(150);
   
   LoadString(hAppInstance, IDS_APPLYINGCOMPUTERSETTINGS, StatusMsg, 256 * sizeof(WCHAR));
   MsGinaInst->Functions->WlxDisplayStatusMessage(MsGinaInst->Context,
                                                  ApplicationDesktop,
                                                  0,
                                                  NULL,
                                                  StatusMsg);
   

   Sleep(150);

   MsGinaInst->Functions->WlxRemoveStatusMessage(MsGinaInst->Context);
   MsGinaInst->Functions->WlxRemoveStatusMessage(MsGinaInst->Context);
   MsGinaInst->Functions->WlxRemoveStatusMessage(MsGinaInst->Context);
      

    Sleep(250);
   
   LoadString(hAppInstance, IDS_LOADINGYOURPERSONALSETTINGS, StatusMsg, 256 * sizeof(WCHAR));
   MsGinaInst->Functions->WlxDisplayStatusMessage(MsGinaInst->Context,
                                                  ApplicationDesktop,
                                                  0,
                                                  NULL,
                                                  StatusMsg);

   Sleep(150);
   
   LoadString(hAppInstance, IDS_APPLYINGYOURPERSONALSETTINGS, StatusMsg, 256 * sizeof(WCHAR));
   MsGinaInst->Functions->WlxDisplayStatusMessage(MsGinaInst->Context,
                                                  ApplicationDesktop,
                                                  0,
                                                  NULL,
                                                  StatusMsg);
   

   Sleep(150);
   
   MsGinaInst->Functions->WlxRemoveStatusMessage(MsGinaInst->Context);
   MsGinaInst->Functions->WlxRemoveStatusMessage(MsGinaInst->Context);
   
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

   LoadString(hAppInstance, IDS_SAVEYOURSETTINGS, StatusMsg, 256 * sizeof(WCHAR));
   MsGinaInst->Functions->WlxDisplayStatusMessage(MsGinaInst->Context,
                                                  ApplicationDesktop,
                                                  0,
                                                  NULL,
                                                  StatusMsg);
   

   Sleep(150);
   
   MsGinaInst->Functions->WlxShutdown(MsGinaInst->Context, WLX_SAS_ACTION_SHUTDOWN);
   
   LoadString(hAppInstance, IDS_REACTOSISSHUTTINGDOWN, StatusMsg, 256 * sizeof(WCHAR));
   MsGinaInst->Functions->WlxDisplayStatusMessage(MsGinaInst->Context,
                                                  ApplicationDesktop,
                                                  0,
                                                  NULL,
                                                  StatusMsg);
   

   Sleep(250);
   
   MsGinaInst->Functions->WlxRemoveStatusMessage(MsGinaInst->Context);
   MsGinaInst->Functions->WlxRemoveStatusMessage(MsGinaInst->Context);
   */
}

