/* $Id: proc.c,v 1.44 2002/08/26 11:24:28 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/proc.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <kernel32/error.h>
#include <wchar.h>
#include <string.h>
#include <napi/i386/segment.h>
#include <napi/teb.h>
#include <ntdll/csr.h>
#include <ntdll/ldr.h>


#define NDEBUG
#include <kernel32/kernel32.h>


/* GLOBALS *******************************************************************/

WaitForInputIdleType  lpfnGlobalRegisterWaitForInputIdle;

LPSTARTUPINFO lpLocalStartupInfo = NULL;

VOID STDCALL
RegisterWaitForInputIdle (WaitForInputIdleType	lpfnRegisterWaitForInputIdle);


/* FUNCTIONS ****************************************************************/

WINBOOL STDCALL
GetProcessId (HANDLE	hProcess, LPDWORD	lpProcessId);

WINBOOL
STDCALL
GetProcessAffinityMask (
	HANDLE	hProcess,
	LPDWORD	lpProcessAffinityMask,
	LPDWORD lpSystemAffinityMask
	)
{
	if (	(NULL == lpProcessAffinityMask)
		|| (NULL == lpSystemAffinityMask)
		)
	{
		SetLastError(ERROR_BAD_ARGUMENTS);
		return FALSE;
	}
	/* FIXME: check hProcess is actually a process */
	/* FIXME: query the kernel process object */
	*lpProcessAffinityMask = 0x00000001;
	*lpSystemAffinityMask  = 0x00000001;
	return TRUE;
}


WINBOOL
STDCALL
GetProcessShutdownParameters (
	LPDWORD	lpdwLevel,
	LPDWORD	lpdwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetProcessWorkingSetSize (
	HANDLE	hProcess,
	LPDWORD	lpMinimumWorkingSetSize,
	LPDWORD	lpMaximumWorkingSetSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
SetProcessShutdownParameters (
	DWORD	dwLevel,
	DWORD	dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetProcessWorkingSetSize (
	HANDLE	hProcess,
	DWORD	dwMinimumWorkingSetSize,
	DWORD	dwMaximumWorkingSetSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
GetProcessTimes (HANDLE		hProcess,
		 LPFILETIME	lpCreationTime,
		 LPFILETIME	lpExitTime,
		 LPFILETIME	lpKernelTime,
		 LPFILETIME	lpUserTime)
{
  NTSTATUS		Status;
  KERNEL_USER_TIMES	Kut;
  
  Status = NtQueryInformationProcess (hProcess,
				      ProcessTimes,
				      &Kut,
				      sizeof(Kut),
				      NULL
				      );
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return (FALSE);
    }
  
  lpCreationTime->dwLowDateTime	= Kut.CreateTime.u.LowPart;
  lpCreationTime->dwHighDateTime = Kut.CreateTime.u.HighPart;
  
  lpExitTime->dwLowDateTime = Kut.ExitTime.u.LowPart;
  lpExitTime->dwHighDateTime = Kut.ExitTime.u.HighPart;
	
  lpKernelTime->dwLowDateTime = Kut.KernelTime.u.LowPart;
  lpKernelTime->dwHighDateTime = Kut.KernelTime.u.HighPart;
  
  lpUserTime->dwLowDateTime = Kut.UserTime.u.LowPart;
  lpUserTime->dwHighDateTime = Kut.UserTime.u.HighPart;
  
  return (TRUE);
}


HANDLE STDCALL GetCurrentProcess (VOID)
{
	return((HANDLE)NtCurrentProcess());
}


HANDLE STDCALL GetCurrentThread (VOID)
{
   return((HANDLE)NtCurrentThread());
}


DWORD STDCALL GetCurrentProcessId (VOID)
{
   return((DWORD)GetTeb()->Cid.UniqueProcess);
}


WINBOOL
STDCALL
GetExitCodeProcess (
	HANDLE	hProcess,
	LPDWORD	lpExitCode
	)
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
	SetLastErrorByStatus (errCode);
	return FALSE;
     }
   memcpy(lpExitCode, &ProcessBasic.ExitStatus, sizeof(DWORD));
   return TRUE;
}


WINBOOL
STDCALL
GetProcessId (
	HANDLE	hProcess,
	LPDWORD	lpProcessId 
	)
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
	SetLastErrorByStatus (errCode);
	return FALSE;
     }
   memcpy( lpProcessId ,&ProcessBasic.UniqueProcessId,sizeof(DWORD));
   return TRUE;
}


HANDLE
STDCALL
OpenProcess (
	DWORD	dwDesiredAccess,
	WINBOOL	bInheritHandle,
	DWORD	dwProcessId
	)
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
	SetLastErrorByStatus (errCode);
	return NULL;
     }
   return ProcessHandle;
}


UINT
STDCALL
WinExec (
	LPCSTR	lpCmdLine,
	UINT	uCmdShow
	)
{
   STARTUPINFOA StartupInfo;
   PROCESS_INFORMATION  ProcessInformation;
   HINSTANCE hInst;
   DWORD dosErr;

   StartupInfo.cb = sizeof(STARTUPINFOA);
   StartupInfo.wShowWindow = uCmdShow;
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
   if (NULL != lpfnGlobalRegisterWaitForInputIdle)
   {
     lpfnGlobalRegisterWaitForInputIdle (
	ProcessInformation.hProcess,
	10000
	);
   }
   NtClose (ProcessInformation.hProcess);
   NtClose (ProcessInformation.hThread);
   return 0;	
}


VOID
STDCALL
RegisterWaitForInputIdle (
	WaitForInputIdleType	lpfnRegisterWaitForInputIdle
	)
{
	lpfnGlobalRegisterWaitForInputIdle = lpfnRegisterWaitForInputIdle;
	return;
}


DWORD
STDCALL
WaitForInputIdle (
	HANDLE	hProcess,
	DWORD	dwMilliseconds
	)
{
	return 0;
}


VOID
STDCALL
Sleep (
	DWORD	dwMilliseconds
	)
{
   SleepEx (dwMilliseconds, FALSE);
   return;
}


DWORD STDCALL
SleepEx (DWORD	dwMilliseconds,
	 BOOL	bAlertable)
{
  TIME Interval;
  NTSTATUS errCode;
  
  if (dwMilliseconds != INFINITE)
    {
      /*
       * System time units are 100 nanoseconds (a nanosecond is a billionth of
       * a second).
       */
      Interval.QuadPart = dwMilliseconds;
      Interval.QuadPart = -(Interval.QuadPart * 10000);
    }  
  else
    {
      /* Approximately 292000 years hence */
      Interval.QuadPart = -0x7FFFFFFFFFFFFFFF;
    }

  errCode = NtDelayExecution (bAlertable, &Interval);
  if (!NT_SUCCESS(errCode))
    {
      SetLastErrorByStatus (errCode);
      return -1;
    }
  return 0;
}


VOID STDCALL
GetStartupInfoW(LPSTARTUPINFOW lpStartupInfo)
{
   PRTL_USER_PROCESS_PARAMETERS Params;

   if (lpStartupInfo == NULL)
     {
	SetLastError(ERROR_INVALID_PARAMETER);
	return;
     }

   Params = NtCurrentPeb ()->ProcessParameters;

   lpStartupInfo->cb = sizeof(STARTUPINFOW);
   lpStartupInfo->lpDesktop = Params->DesktopInfo.Buffer;
   lpStartupInfo->lpTitle = Params->WindowTitle.Buffer;
   lpStartupInfo->dwX = Params->StartingX;
   lpStartupInfo->dwY = Params->StartingY;
   lpStartupInfo->dwXSize = Params->CountX;
   lpStartupInfo->dwYSize = Params->CountY;
   lpStartupInfo->dwXCountChars = Params->CountCharsX;
   lpStartupInfo->dwYCountChars = Params->CountCharsY;
   lpStartupInfo->dwFillAttribute = Params->FillAttribute;
   lpStartupInfo->dwFlags = Params->Flags;
   lpStartupInfo->wShowWindow = Params->ShowWindowFlags;
   lpStartupInfo->lpReserved = Params->ShellInfo.Buffer;
   lpStartupInfo->cbReserved2 = Params->RuntimeInfo.Length;
   lpStartupInfo->lpReserved2 = (LPBYTE)Params->RuntimeInfo.Buffer;

   lpStartupInfo->hStdInput = Params->InputHandle;
   lpStartupInfo->hStdOutput = Params->OutputHandle;
   lpStartupInfo->hStdError = Params->ErrorHandle;
}


VOID STDCALL
GetStartupInfoA(LPSTARTUPINFOA lpStartupInfo)
{
   PRTL_USER_PROCESS_PARAMETERS Params;
   ANSI_STRING AnsiString;

   if (lpStartupInfo == NULL)
     {
	SetLastError(ERROR_INVALID_PARAMETER);
	return;
     }

   Params = NtCurrentPeb ()->ProcessParameters;

   RtlAcquirePebLock ();

   if (lpLocalStartupInfo == NULL)
     {
	/* create new local startup info (ansi) */
	lpLocalStartupInfo = RtlAllocateHeap (RtlGetProcessHeap (),
	                                      0,
	                                      sizeof(STARTUPINFOA));

	lpLocalStartupInfo->cb = sizeof(STARTUPINFOA);

	/* copy window title string */
	RtlUnicodeStringToAnsiString (&AnsiString,
	                              &Params->WindowTitle,
	                              TRUE);
	lpLocalStartupInfo->lpTitle = AnsiString.Buffer;

	/* copy desktop info string */
	RtlUnicodeStringToAnsiString (&AnsiString,
	                              &Params->DesktopInfo,
	                              TRUE);
	lpLocalStartupInfo->lpDesktop = AnsiString.Buffer;

	/* copy shell info string */
	RtlUnicodeStringToAnsiString (&AnsiString,
	                              &Params->ShellInfo,
	                              TRUE);
	lpLocalStartupInfo->lpReserved = AnsiString.Buffer;

	lpLocalStartupInfo->dwX = Params->StartingX;
	lpLocalStartupInfo->dwY = Params->StartingY;
	lpLocalStartupInfo->dwXSize = Params->CountX;
	lpLocalStartupInfo->dwYSize = Params->CountY;
	lpLocalStartupInfo->dwXCountChars = Params->CountCharsX;
	lpLocalStartupInfo->dwYCountChars = Params->CountCharsY;
	lpLocalStartupInfo->dwFillAttribute = Params->FillAttribute;
	lpLocalStartupInfo->dwFlags = Params->Flags;
	lpLocalStartupInfo->wShowWindow = Params->ShowWindowFlags;
	lpLocalStartupInfo->cbReserved2 = Params->RuntimeInfo.Length;
	lpLocalStartupInfo->lpReserved2 = (LPBYTE)Params->RuntimeInfo.Buffer;

	lpLocalStartupInfo->hStdInput = Params->InputHandle;
	lpLocalStartupInfo->hStdOutput = Params->OutputHandle;
	lpLocalStartupInfo->hStdError = Params->ErrorHandle;
     }

   RtlReleasePebLock ();

   /* copy local startup info data to external startup info */
   memcpy (lpStartupInfo,
           lpLocalStartupInfo,
           sizeof(STARTUPINFOA));
}


BOOL STDCALL
FlushInstructionCache (HANDLE	hProcess,
		       LPCVOID	lpBaseAddress,
		       DWORD	dwSize)
{
  NTSTATUS	errCode;
  
  errCode = NtFlushInstructionCache (hProcess,
				     (PVOID) lpBaseAddress,
				     dwSize);
  if (!NT_SUCCESS(errCode))
    {
      SetLastErrorByStatus (errCode);
      return FALSE;
    }
  return TRUE;
}


VOID STDCALL
ExitProcess (UINT	uExitCode)
{
  CSRSS_API_REQUEST CsrRequest;
  CSRSS_API_REPLY CsrReply;
  NTSTATUS Status;
  
  /* unload all dll's */
  LdrShutdownProcess ();

  /* notify csrss of process termination */
  CsrRequest.Type = CSRSS_TERMINATE_PROCESS;
  Status = CsrClientCallServer(&CsrRequest, 
			       &CsrReply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrReply.Status))
    {
      DbgPrint("Failed to tell csrss about terminating process. "
	       "Expect trouble.\n");
    }
  
  
  NtTerminateProcess (NtCurrentProcess (),
		      uExitCode);
}


WINBOOL STDCALL
TerminateProcess (HANDLE	hProcess,
		  UINT	uExitCode)
{
  NTSTATUS Status = NtTerminateProcess (hProcess, uExitCode);
      
  if (NT_SUCCESS(Status))
    {
      return TRUE;
    }
  SetLastErrorByStatus (Status);
  return FALSE;
}


VOID STDCALL
FatalAppExitA (UINT	uAction,
	       LPCSTR	lpMessageText)
{
  UNICODE_STRING MessageTextU;
  ANSI_STRING MessageText;
  
  RtlInitAnsiString (&MessageText, (LPSTR) lpMessageText);

  RtlAnsiStringToUnicodeString (&MessageTextU,
				&MessageText,
				TRUE);

  FatalAppExitW (uAction, MessageTextU.Buffer);

  RtlFreeUnicodeString (&MessageTextU);
}


VOID STDCALL
FatalAppExitW (UINT	uAction,
	       LPCWSTR	lpMessageText)
{
  return;
}


VOID STDCALL
FatalExit (int ExitCode)
{
  ExitProcess(ExitCode);
}


DWORD STDCALL
GetPriorityClass (HANDLE	hProcess)
{
  HANDLE		hProcessTmp;
  DWORD		CsrPriorityClass = 0; // This tells CSRSS we want to GET it!
  NTSTATUS	Status;
	
  Status = 
    NtDuplicateObject (GetCurrentProcess(),
		       hProcess,
		       GetCurrentProcess(),
		       &hProcessTmp,
		       (PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION),
		       FALSE,
		       0);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return (0); /* ERROR */
    }
  /* Ask CSRSS to set it */
  CsrSetPriorityClass (hProcessTmp, &CsrPriorityClass);
  NtClose (hProcessTmp);
  /* Translate CSR->W32 priorities */
  switch (CsrPriorityClass)
    {
    case CSR_PRIORITY_CLASS_NORMAL:
      return (NORMAL_PRIORITY_CLASS);	/* 32 */
    case CSR_PRIORITY_CLASS_IDLE:
      return (IDLE_PRIORITY_CLASS);	/* 64 */
    case CSR_PRIORITY_CLASS_HIGH:
      return (HIGH_PRIORITY_CLASS);	/* 128 */
    case CSR_PRIORITY_CLASS_REALTIME:
      return (REALTIME_PRIORITY_CLASS);	/* 256 */
    }
  SetLastError (ERROR_ACCESS_DENIED);
  return (0); /* ERROR */
}



WINBOOL STDCALL
SetPriorityClass (HANDLE	hProcess,
		  DWORD	dwPriorityClass)
{
  HANDLE		hProcessTmp;
  DWORD		CsrPriorityClass;
  NTSTATUS	Status;
  
  switch (dwPriorityClass)
    {
    case NORMAL_PRIORITY_CLASS:	/* 32 */
      CsrPriorityClass = CSR_PRIORITY_CLASS_NORMAL;
      break;
    case IDLE_PRIORITY_CLASS:	/* 64 */
      CsrPriorityClass = CSR_PRIORITY_CLASS_IDLE;
      break;
    case HIGH_PRIORITY_CLASS:	/* 128 */
      CsrPriorityClass = CSR_PRIORITY_CLASS_HIGH;
      break;
    case REALTIME_PRIORITY_CLASS:	/* 256 */
      CsrPriorityClass = CSR_PRIORITY_CLASS_REALTIME;
      break;
    default:
      SetLastError (ERROR_INVALID_PARAMETER);
      return (FALSE);
    }
  Status = 
    NtDuplicateObject (GetCurrentProcess(),
		       hProcess,
		       GetCurrentProcess(),
		       &hProcessTmp,
		       (PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION),
		       FALSE,
		       0);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return (FALSE); /* ERROR */
    }
  /* Ask CSRSS to set it */
  Status = CsrSetPriorityClass (hProcessTmp, &CsrPriorityClass);
  NtClose (hProcessTmp);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return (FALSE);
    }
  return (TRUE);
}


DWORD STDCALL
GetProcessVersion (DWORD	ProcessId)
{
  DWORD			Version = 0;
  PIMAGE_NT_HEADERS	NtHeader = NULL;
  PVOID			BaseAddress = NULL;

  /* Caller's */
  if (0 == ProcessId)
    {
      BaseAddress = (PVOID) NtCurrentPeb()->ImageBaseAddress;
      NtHeader = RtlImageNtHeader (BaseAddress);
      if (NULL != NtHeader)
	{
	  Version =
	    (NtHeader->OptionalHeader.MajorOperatingSystemVersion << 16) | 
	    (NtHeader->OptionalHeader.MinorOperatingSystemVersion);
	}
    }
  else /* other process */
    {
      /* FIXME: open the other process */
      SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }
  return (Version);
}



/* EOF */
