/* $Id: winlogon.c,v 1.1 2000/12/05 02:38:08 ekohl Exp $
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

#include <ntos.h>
#include <windows.h>
#include <stdio.h>
#include <lsass/ntsecapi.h>

#include <wchar.h>


/* GLOBALS ******************************************************************/

/* FUNCTIONS *****************************************************************/

BOOLEAN StartServices(VOID)
{
   HANDLE ServicesInitEvent;
   BOOLEAN Result;
   STARTUPINFO StartupInfo;
   PROCESS_INFORMATION ProcessInformation;
   
   ServicesInitEvent = CreateEvent(NULL,
				   TRUE,
				   FALSE,
				   "\\ServicesInitDone");
   
   if (ServicesInitEvent == NULL)
     {
	DbgPrint("Failed to create services notification event\n");
	return(FALSE);
     }
   
   /* Start the Win32 subsystem (csrss.exe) */
   DbgPrint("WL: Executing services.exe\n");
   
   StartupInfo.cb = sizeof(StartupInfo);
   StartupInfo.lpReserved = NULL;
   StartupInfo.lpDesktop = NULL;
   StartupInfo.lpTitle = NULL;
   StartupInfo.dwFlags = 0;
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved2 = 0;
   
   Result = CreateProcess("C:\\reactos\\system32\\services.exe",
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
	DbgPrint("WL: Failed to execute services\n");
	return(FALSE);
     }
   
   DbgPrint("WL: Waiting for services\n");
   WaitForSingleObject(ServicesInitEvent, INFINITE);
   
   DbgPrint("WL: Finished waiting for services\n");
   return(TRUE);
}

BOOLEAN StartLsass(VOID)
{
   HANDLE LsassInitEvent;
   BOOLEAN Result;
   STARTUPINFO StartupInfo;
   PROCESS_INFORMATION ProcessInformation;
   
   LsassInitEvent = CreateEvent(NULL,
				TRUE,
				FALSE,
				"\\LsassInitDone");
   
   if (LsassInitEvent == NULL)
     {
	DbgPrint("Failed to create lsass notification event\n");
	return(FALSE);
     }
   
   /* Start the Win32 subsystem (csrss.exe) */
   DbgPrint("WL: Executing lsass.exe\n");
   
   StartupInfo.cb = sizeof(StartupInfo);
   StartupInfo.lpReserved = NULL;
   StartupInfo.lpDesktop = NULL;
   StartupInfo.lpTitle = NULL;
   StartupInfo.dwFlags = 0;
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved2 = 0;
   
   Result = CreateProcess("C:\\reactos\\system32\\lsass.exe",
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
   
   DbgPrint("WL: Waiting for lsass\n");
   WaitForSingleObject(LsassInitEvent, INFINITE);
   
   DbgPrint("WL: Finished waiting for lsass\n");
   return(TRUE);
}

VOID DoLoginUser(PCHAR Name, PCHAR Password)
{
   PROCESS_INFORMATION ProcessInformation;
   STARTUPINFO StartupInfo;
   BOOLEAN Result;
   
   StartupInfo.cb = sizeof(StartupInfo);
   StartupInfo.lpReserved = NULL;
   StartupInfo.lpDesktop = NULL;
   StartupInfo.lpTitle = NULL;
   StartupInfo.dwFlags = 0;
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved2 = 0;
   
   Result = CreateProcess("C:\\reactos\\system32\\shell.exe",
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
	DbgPrint("WL: Failed to execute user shell\n");
	return;
     }
   WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
}

int STDCALL
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
#if 0
   LSA_STRING ProcessName;
   NTSTATUS Status;
   HANDLE LsaHandle;
   LSA_OPERATIONAL_MODE Mode;
#endif
   CHAR LoginPrompt[] = "login:";
   CHAR PasswordPrompt[] = "password:";
   DWORD Result;
   CHAR LoginName[255];
   CHAR Password[255];
   ULONG i;
   
   /* 
    * FIXME: Create WindowStations here. At the moment lsass and services
    * share ours
    */
#if 0
   StartLsass();
   StartServices();
#endif
   
   /* FIXME: What name does the real WinLogon use? */
#if 0
   RtlInitUnicodeString((PUNICODE_STRING)&ProcessName, L"WinLogon");
   Status = LsaRegisterLogonProcess(&ProcessName, &LsaHandle, &Mode);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("WL: Failed to connect to lsass\n");
	return(1);
     }
#endif
   
   /* smss wouldn't have created a console for us */
   AllocConsole();
   
   /* Main loop */
   for (;;)
     {
	/* Display login prompt */
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
		     LoginPrompt,
//		     wcslen(LoginPrompt),
		     strlen(LoginPrompt),
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
//		     wcslen(PasswordPrompt),
		     strlen(PasswordPrompt),
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
	
	DoLoginUser(LoginName, Password);
     }
}
