/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/proc.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#define UNICODE
#include <windows.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>
#include <internal/i386/segment.h>
#include <internal/teb.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* TYPES *********************************************************************/

typedef struct _WSTARTUPINFO { 
  DWORD   cb; 
  LPWSTR  lpReserved; 
  LPWSTR  lpDesktop; 
  LPWSTR  lpTitle; 
  DWORD   dwX; 
  DWORD   dwY; 
  DWORD   dwXSize; 
  DWORD   dwYSize; 
  DWORD   dwXCountChars; 
  DWORD   dwYCountChars; 
  DWORD   dwFillAttribute; 
  DWORD   dwFlags; 
  WORD    wShowWindow; 
  WORD    cbReserved2; 
  LPBYTE  lpReserved2; 
  HANDLE  hStdInput; 
  HANDLE  hStdOutput; 
  HANDLE  hStdError; 
} WSTARTUPINFO, *LPWSTARTUPINFO; 

/* GLOBALS *******************************************************************/

WaitForInputIdleType  lpfnGlobalRegisterWaitForInputIdle;

VOID RegisterWaitForInputIdle(WaitForInputIdleType  lpfnRegisterWaitForInputIdle);

/* FUNCTIONS ****************************************************************/

WINBOOL STDCALL GetProcessId(HANDLE hProcess, LPDWORD lpProcessId);

FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
   UNIMPLEMENTED;
   return(NULL);
}

WINBOOL STDCALL GetProcessTimes(HANDLE hProcess,
				LPFILETIME lpCreationTime,
				LPFILETIME lpExitTime,
				LPFILETIME lpKernelTime,
				LPFILETIME lpUserTime)
{
   dprintf("GetProcessTimes is unimplemented\n");
   return(FALSE);
}

HANDLE STDCALL GetCurrentProcess(VOID)
{
	return (HANDLE)NtCurrentProcess();
}

HANDLE STDCALL GetCurrentThread(VOID)
{
	return (HANDLE)NtCurrentThread();
}

DWORD STDCALL GetCurrentProcessId(VOID)
{	
	return (DWORD)(GetTeb()->Cid).UniqueProcess;		
}

WINBOOL STDCALL GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode)
{
   NTSTATUS errCode;
   PROCESS_BASIC_INFORMATION ProcessBasic;
   ULONG BytesWritten;
   
   errCode = NtQueryInformationProcess(hProcess,
				       ProcessBasicInformation,
				       &ProcessBasic,
				       sizeof(PROCESS_BASIC_INFORMATION),
				       &BytesWritten);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   memcpy(lpExitCode, &ProcessBasic.ExitStatus, sizeof(DWORD));
   return TRUE;	
}

WINBOOL STDCALL GetProcessId(HANDLE hProcess, LPDWORD lpProcessId )
{
   NTSTATUS errCode;
   PROCESS_BASIC_INFORMATION ProcessBasic;
   ULONG BytesWritten;

   errCode = NtQueryInformationProcess(hProcess,
				       ProcessBasicInformation,
				       &ProcessBasic,
				       sizeof(PROCESS_BASIC_INFORMATION),
				       &BytesWritten);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   memcpy( lpProcessId ,&ProcessBasic.UniqueProcessId,sizeof(DWORD));
   return TRUE;	
}

PWSTR InternalAnsiToUnicode(PWSTR Out, LPCSTR In, ULONG MaxLength)
{
   ULONG i;
   
   if (In == NULL)
     {
	return(NULL);
     }
   else
     {
	i = 0;
   	while ((*In)!=0 && i < MaxLength)
	  {
	     Out[i] = *In;
	     In++;
	     i++;
	  }
   	Out[i] = 0;
	return(Out);
     }
}

HANDLE STDCALL OpenProcess(DWORD dwDesiredAccess,
			   WINBOOL bInheritHandle,
			   DWORD dwProcessId)
{
   NTSTATUS errCode;
   HANDLE ProcessHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID ClientId ;
   
   ClientId.UniqueProcess = (HANDLE)dwProcessId;
   ClientId.UniqueThread = INVALID_HANDLE_VALUE;
   
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = (HANDLE)NULL;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;
   ObjectAttributes.ObjectName = NULL;
   
   if (bInheritHandle == TRUE)
     ObjectAttributes.Attributes = OBJ_INHERIT;
   else
     ObjectAttributes.Attributes = 0;
   
   errCode = NtOpenProcess(&ProcessHandle, 
			   dwDesiredAccess, 
			   &ObjectAttributes, 
			   &ClientId);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return NULL;
     }
   return ProcessHandle;
}

UINT WinExec (LPCSTR lpCmdLine, UINT uCmdShow)
{
   STARTUPINFO StartupInfo;
   PROCESS_INFORMATION  ProcessInformation; 	
   HINSTANCE hInst;
   DWORD dosErr;
   
   StartupInfo.cb = sizeof(STARTUPINFO);
   StartupInfo.wShowWindow = uCmdShow ;
   StartupInfo.dwFlags = 0;
   
   hInst = (HINSTANCE)CreateProcessA(NULL,
				     (PVOID)lpCmdLine,
				     NULL,
				     NULL,
				     FALSE,
				     0,
				     NULL,
				     NULL,
				     &StartupInfo, 
				     &ProcessInformation);
   if ( hInst == NULL ) 
     {
	dosErr = GetLastError();
	return dosErr;
     }
   if ( lpfnGlobalRegisterWaitForInputIdle != NULL )
     lpfnGlobalRegisterWaitForInputIdle(ProcessInformation.hProcess,10000);
   NtClose(ProcessInformation.hProcess);
   NtClose(ProcessInformation.hThread);
   return 0;	
}



VOID RegisterWaitForInputIdle(WaitForInputIdleType 
			      lpfnRegisterWaitForInputIdle)
{
   lpfnGlobalRegisterWaitForInputIdle = lpfnRegisterWaitForInputIdle; 
   return;
}

DWORD STDCALL WaitForInputIdle(HANDLE hProcess,	
			       DWORD dwMilliseconds)
{
   return 0;
}

VOID STDCALL Sleep(DWORD dwMilliseconds)
{
   SleepEx(dwMilliseconds,FALSE);
   return;
}

DWORD STDCALL SleepEx(DWORD dwMilliseconds, BOOL bAlertable)
{
	TIME Interval;
	NTSTATUS errCode;	

        Interval.QuadPart = dwMilliseconds * 1000;

	errCode = NtDelayExecution(bAlertable,&Interval);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return -1;
	}
	return 0;
}

VOID STDCALL GetStartupInfoW(LPSTARTUPINFO _lpStartupInfo)
{
   NT_PEB *pPeb = NtCurrentPeb();
   LPWSTARTUPINFO lpStartupInfo = (LPWSTARTUPINFO)_lpStartupInfo;
   
   if (lpStartupInfo == NULL)
     {
	SetLastError(ERROR_INVALID_PARAMETER);
	return;
     }
   
   lpStartupInfo->cb = sizeof(STARTUPINFO);
//   lstrcpyW(lpStartupInfo->lpDesktop, pPeb->StartupInfo->Desktop); 
//   lstrcpyW(lpStartupInfo->lpTitle, pPeb->StartupInfo->Title);
   lpStartupInfo->dwX = pPeb->StartupInfo->dwX; 
   lpStartupInfo->dwY = pPeb->StartupInfo->dwY; 
   lpStartupInfo->dwXSize = pPeb->StartupInfo->dwXSize; 
   lpStartupInfo->dwYSize = pPeb->StartupInfo->dwYSize; 
   lpStartupInfo->dwXCountChars = pPeb->StartupInfo->dwXCountChars; 
   lpStartupInfo->dwYCountChars = pPeb->StartupInfo->dwYCountChars; 
   lpStartupInfo->dwFillAttribute = pPeb->StartupInfo->dwFillAttribute; 
   lpStartupInfo->dwFlags = pPeb->StartupInfo->dwFlags; 
   lpStartupInfo->wShowWindow = pPeb->StartupInfo->wShowWindow; 
   //lpStartupInfo->cbReserved2 = pPeb->StartupInfo->cbReserved; 
   //lpStartupInfo->lpReserved = pPeb->StartupInfo->lpReserved1; 
   //lpStartupInfo->lpReserved2 = pPeb->StartupInfo->lpReserved2; 
   
        lpStartupInfo->cb = sizeof(STARTUPINFO);
    	lstrcpyW(lpStartupInfo->lpDesktop, pPeb->StartupInfo->Desktop); 
    	lstrcpyW(lpStartupInfo->lpTitle, pPeb->StartupInfo->Title);
    	lpStartupInfo->dwX = pPeb->StartupInfo->dwX; 
    	lpStartupInfo->dwY = pPeb->StartupInfo->dwY; 
    	lpStartupInfo->dwXSize = pPeb->StartupInfo->dwXSize; 
    	lpStartupInfo->dwYSize = pPeb->StartupInfo->dwYSize; 
    	lpStartupInfo->dwXCountChars = pPeb->StartupInfo->dwXCountChars; 
    	lpStartupInfo->dwYCountChars = pPeb->StartupInfo->dwYCountChars; 
    	lpStartupInfo->dwFillAttribute = pPeb->StartupInfo->dwFillAttribute; 
    	lpStartupInfo->dwFlags = pPeb->StartupInfo->dwFlags; 
    	lpStartupInfo->wShowWindow = pPeb->StartupInfo->wShowWindow; 
    	//lpStartupInfo->cbReserved2 = pPeb->StartupInfo->cbReserved; 
	//lpStartupInfo->lpReserved = pPeb->StartupInfo->lpReserved1; 
    	//lpStartupInfo->lpReserved2 = pPeb->StartupInfo->lpReserved2; 
	
    	lpStartupInfo->hStdInput = pPeb->StartupInfo->hStdInput; 
    	lpStartupInfo->hStdOutput = pPeb->StartupInfo->hStdOutput; 
    	lpStartupInfo->hStdError = pPeb->StartupInfo->hStdError; 
	
	
	
	return;
}


VOID STDCALL GetStartupInfoA(LPSTARTUPINFO lpStartupInfo)
{
   NT_PEB *pPeb = NtCurrentPeb();
   ULONG i = 0;
   
   if (lpStartupInfo == NULL) 
     {
	SetLastError(ERROR_INVALID_PARAMETER);
	return;
     }
	
   lpStartupInfo->cb = sizeof(STARTUPINFO);
   i = 0;
   
   while ((pPeb->StartupInfo->Desktop[i])!=0 && i < MAX_PATH)
     {
	lpStartupInfo->lpDesktop[i] = (unsigned char)
	  pPeb->StartupInfo->Desktop[i];
	i++;
     }
   lpStartupInfo->lpDesktop[i] = 0;
   
   i = 0;
   while ((pPeb->StartupInfo->Title[i])!=0 && i < MAX_PATH)
     {
	lpStartupInfo->lpTitle[i] = (unsigned char)pPeb->StartupInfo->Title[i];
	i++;
     }
   lpStartupInfo->lpTitle[i] = 0;
   
   lpStartupInfo->dwX = pPeb->StartupInfo->dwX; 
   lpStartupInfo->dwY = pPeb->StartupInfo->dwY; 
   lpStartupInfo->dwXSize = pPeb->StartupInfo->dwXSize; 
   lpStartupInfo->dwYSize = pPeb->StartupInfo->dwYSize; 
   lpStartupInfo->dwXCountChars = pPeb->StartupInfo->dwXCountChars; 
   lpStartupInfo->dwYCountChars = pPeb->StartupInfo->dwYCountChars; 
   lpStartupInfo->dwFillAttribute = pPeb->StartupInfo->dwFillAttribute; 
   lpStartupInfo->dwFlags = pPeb->StartupInfo->dwFlags; 
   lpStartupInfo->wShowWindow = pPeb->StartupInfo->wShowWindow; 
   //lpStartupInfo->cbReserved2 = pPeb->StartupInfo->cbReserved; 
   //lpStartupInfo->lpReserved = pPeb->StartupInfo->lpReserved1; 
   //lpStartupInfo->lpReserved2 = pPeb->StartupInfo->lpReserved2; 
   
   lpStartupInfo->hStdInput = pPeb->StartupInfo->hStdInput; 
   lpStartupInfo->hStdOutput = pPeb->StartupInfo->hStdOutput; 
   lpStartupInfo->hStdError = pPeb->StartupInfo->hStdError; 
   
   return;
}

BOOL STDCALL FlushInstructionCache(HANDLE hProcess,	
				   LPCVOID lpBaseAddress,	
				   DWORD dwSize)
{
   NTSTATUS errCode;
   errCode = NtFlushInstructionCache(hProcess,(PVOID)lpBaseAddress,dwSize);
   if (!NT_SUCCESS(errCode))
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   return TRUE;
}

VOID STDCALL ExitProcess(UINT uExitCode)
{
   NtTerminateProcess(NtCurrentProcess(), uExitCode);
}

WINBOOL STDCALL TerminateProcess(HANDLE hProcess, UINT uExitCode)
{
   NTSTATUS errCode;
   errCode = NtTerminateProcess(hProcess, uExitCode);
   if (!NT_SUCCESS(errCode))
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   return TRUE;
}

VOID STDCALL FatalAppExitA(UINT uAction, LPCSTR lpMessageText)
{
   WCHAR MessageTextW[MAX_PATH];
   UINT i;
   i = 0;
   while ((*lpMessageText)!=0 && i < 35)
     {
	MessageTextW[i] = *lpMessageText;
	lpMessageText++;
	i++;
     }
   MessageTextW[i] = 0;
   
   return FatalAppExitW(uAction,MessageTextW);
}


	
VOID STDCALL FatalAppExitW(UINT uAction, LPCWSTR lpMessageText)
{
   return;	
}
