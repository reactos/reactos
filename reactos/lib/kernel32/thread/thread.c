/* $Id: thread.c,v 1.58 2004/12/09 19:11:07 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/thread/thread.c
 * PURPOSE:         Thread functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *			Tls functions are modified from WINE
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS *****************************************************************/

/* FIXME: please put this in some header */
static EXCEPTION_DISPOSITION __cdecl
_except_handler(EXCEPTION_RECORD *ExceptionRecord,
		void * EstablisherFrame,
		CONTEXT *ContextRecord,
		void * DispatcherContext)
{
  ExitThread(0);

  /* We should not get to here */
  return(ExceptionContinueSearch);
}


__declspec(noreturn) void STDCALL
ThreadStartup
(
 LPTHREAD_START_ROUTINE lpStartAddress,
 LPVOID lpParameter
)
{
  volatile UINT uExitCode = 0;

  __try1(_except_handler)
  {
    /* FIXME: notify csrss of thread creation ?? */
    uExitCode = (lpStartAddress)(lpParameter);
  }
  __except1
  {
  }

  ExitThread(uExitCode);
}


/*
 * @implemented
 */
HANDLE STDCALL
CreateThread
(
 LPSECURITY_ATTRIBUTES lpThreadAttributes,
 DWORD dwStackSize,
 LPTHREAD_START_ROUTINE lpStartAddress,
 LPVOID lpParameter,
 DWORD dwCreationFlags,
 LPDWORD lpThreadId
)
{
 return CreateRemoteThread
 (
  NtCurrentProcess(),
  lpThreadAttributes,
  dwStackSize,
  lpStartAddress,
  lpParameter,
  dwCreationFlags,
  lpThreadId
 );
}


/*
 * @implemented
 */
HANDLE STDCALL
CreateRemoteThread
(
 HANDLE hProcess,
 LPSECURITY_ATTRIBUTES lpThreadAttributes,
 DWORD dwStackSize,
 LPTHREAD_START_ROUTINE lpStartAddress,
 LPVOID lpParameter,
 DWORD dwCreationFlags,
 LPDWORD lpThreadId
)
{
 HANDLE hThread;
 CLIENT_ID cidClientId;
 NTSTATUS nErrCode;
 ULONG_PTR nStackReserve;
 ULONG_PTR nStackCommit;
 OBJECT_ATTRIBUTES oaThreadAttribs;
 PIMAGE_NT_HEADERS pinhHeader =
  RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);

 DPRINT
 (
  "hProcess           %08X\n"
  "lpThreadAttributes %08X\n"
  "dwStackSize        %08X\n"
  "lpStartAddress     %08X\n"
  "lpParameter        %08X\n"
  "dwCreationFlags    %08X\n"
  "lpThreadId         %08X\n",
  hProcess,
  lpThreadAttributes,
  dwStackSize,
  lpStartAddress,
  lpParameter,
  dwCreationFlags,
  lpThreadId
 );

 /* FIXME: do more checks - e.g. the image may not have an optional header */
 if(pinhHeader == NULL)
 {
  nStackReserve = 0x100000;
  nStackCommit = PAGE_SIZE;
 }
 else
 {
  nStackReserve = pinhHeader->OptionalHeader.SizeOfStackReserve;
  nStackCommit = pinhHeader->OptionalHeader.SizeOfStackCommit;
 }

 /* FIXME: this should be defined in winbase.h */
#ifndef STACK_SIZE_PARAM_IS_A_RESERVATION
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000
#endif

 /* use defaults */
 if(dwStackSize == 0);
 /* dwStackSize specifies the size to reserve */
 else if(dwCreationFlags & STACK_SIZE_PARAM_IS_A_RESERVATION)
  nStackReserve = dwStackSize;
 /* dwStackSize specifies the size to commit */
 else
  nStackCommit = dwStackSize;

 /* fix the stack reserve size */
 if(nStackCommit > nStackReserve)
  nStackReserve = ROUNDUP(nStackCommit, 0x100000);

 /* initialize the attributes for the thread object */
 InitializeObjectAttributes
 (
  &oaThreadAttribs,
  NULL,
  0,
  NULL,
  NULL
 );
 
 if(lpThreadAttributes)
 {
  /* make the handle inheritable */
  if(lpThreadAttributes->bInheritHandle)
   oaThreadAttribs.Attributes |= OBJ_INHERIT;

  /* user-defined security descriptor */
  oaThreadAttribs.SecurityDescriptor = lpThreadAttributes->lpSecurityDescriptor;
 }

 DPRINT
 (
  "RtlRosCreateUserThreadVa\n"
  "(\n"
  " ProcessHandle    %p,\n"
  " ObjectAttributes %p,\n"
  " CreateSuspended  %d,\n"
  " StackZeroBits    %d,\n"
  " StackReserve     %lu,\n"
  " StackCommit      %lu,\n"
  " StartAddress     %p,\n"
  " ThreadHandle     %p,\n"
  " ClientId         %p,\n"
  " ParameterCount   %u,\n"
  " Parameters[0]    %p,\n"
  " Parameters[1]    %p\n"
  ")\n",
  hProcess,
  &oaThreadAttribs,
  dwCreationFlags & CREATE_SUSPENDED,
  0,
  nStackReserve,
  nStackCommit,
  ThreadStartup,
  &hThread,
  &cidClientId,
  2,
  lpStartAddress,
  lpParameter
 );

 /* create the thread */
 nErrCode = RtlRosCreateUserThreadVa
 (
  hProcess,
  &oaThreadAttribs,
  dwCreationFlags & CREATE_SUSPENDED,
  0,
  &nStackReserve,
  &nStackCommit,
  (PTHREAD_START_ROUTINE)ThreadStartup,
  &hThread,
  &cidClientId,
  2,
  lpStartAddress,
  lpParameter
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  SetLastErrorByStatus(nErrCode);
  return NULL;
 }
 
 DPRINT
 (
  "StackReserve          %p\n"
  "StackCommit           %p\n"
  "ThreadHandle          %p\n"
  "ClientId.UniqueThread %p\n",
  nStackReserve,
  nStackCommit,
  hThread,
  cidClientId.UniqueThread
 );

 /* success */
 if(lpThreadId) *lpThreadId = (DWORD)cidClientId.UniqueThread;
 return hThread;
}


/*
 * @implemented
 */
HANDLE
STDCALL
OpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    )
{
   NTSTATUS errCode;
   HANDLE ThreadHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID ClientId ;
   
   ClientId.UniqueProcess = INVALID_HANDLE_VALUE;
   ClientId.UniqueThread = (HANDLE)dwThreadId;
   
   InitializeObjectAttributes (&ObjectAttributes,
			      NULL,
			      (bInheritHandle ? OBJ_INHERIT : 0),
			      NULL,
			      NULL);
   
   errCode = NtOpenThread(&ThreadHandle,
			   dwDesiredAccess,
			   &ObjectAttributes,
			   &ClientId);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus (errCode);
	return NULL;
     }
   return ThreadHandle;
}


/*
 * @implemented
 */
PTEB
GetTeb(VOID)
{
  return(NtCurrentTeb());
}


/*
 * @implemented
 */
BOOL STDCALL
SwitchToThread(VOID)
{
  NTSTATUS errCode;
  errCode = NtYieldExecution();
  return TRUE;
}


/*
 * @implemented
 */
DWORD STDCALL
GetCurrentThreadId(VOID)
{
  return((DWORD)(NtCurrentTeb()->Cid).UniqueThread);
}

/*
 * @implemented
 */
VOID STDCALL
ExitThread(DWORD uExitCode)
{
  NTSTATUS Status;
  BOOLEAN LastThread;

  /*
   * Terminate process if this is the last thread
   * of the current process
   */
  Status = NtQueryInformationThread(NtCurrentThread(),
				    ThreadAmILastThread,
				    &LastThread,
				    sizeof(BOOLEAN),
				    NULL);
  if (NT_SUCCESS(Status) && LastThread == TRUE)
    {
      ExitProcess(uExitCode);
    }

  /* FIXME: notify csrss of thread termination */

  LdrShutdownThread();

  RtlRosExitUserThread(uExitCode);
}


/*
 * @implemented
 */
BOOL STDCALL
GetThreadTimes(HANDLE hThread,
	       LPFILETIME lpCreationTime,
	       LPFILETIME lpExitTime,
	       LPFILETIME lpKernelTime,
	       LPFILETIME lpUserTime)
{
  KERNEL_USER_TIMES KernelUserTimes;
  NTSTATUS Status;

  Status = NtQueryInformationThread(hThread,
				    ThreadTimes,
				    &KernelUserTimes,
				    sizeof(KERNEL_USER_TIMES),
				    NULL);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  lpCreationTime->dwLowDateTime = KernelUserTimes.CreateTime.u.LowPart;
  lpCreationTime->dwHighDateTime = KernelUserTimes.CreateTime.u.HighPart;
  
  lpExitTime->dwLowDateTime = KernelUserTimes.ExitTime.u.LowPart;
  lpExitTime->dwHighDateTime = KernelUserTimes.ExitTime.u.HighPart;
  
  lpKernelTime->dwLowDateTime = KernelUserTimes.KernelTime.u.LowPart;
  lpKernelTime->dwHighDateTime = KernelUserTimes.KernelTime.u.HighPart;
  
  lpUserTime->dwLowDateTime = KernelUserTimes.UserTime.u.LowPart;
  lpUserTime->dwHighDateTime = KernelUserTimes.UserTime.u.HighPart;

  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
GetThreadContext(HANDLE hThread,
		 LPCONTEXT lpContext)
{
  NTSTATUS Status;

  Status = NtGetContextThread(hThread,
			      lpContext);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
SetThreadContext(HANDLE hThread,
		 CONST CONTEXT *lpContext)
{
  NTSTATUS Status;

  Status = NtSetContextThread(hThread,
			      (void *)lpContext);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
GetExitCodeThread(HANDLE hThread,
		  LPDWORD lpExitCode)
{
  THREAD_BASIC_INFORMATION ThreadBasic;
  NTSTATUS Status;

  Status = NtQueryInformationThread(hThread,
				    ThreadBasicInformation,
				    &ThreadBasic,
				    sizeof(THREAD_BASIC_INFORMATION),
				    NULL);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  memcpy(lpExitCode, &ThreadBasic.ExitStatus, sizeof(DWORD));

  return(TRUE);
}


/*
 * @implemented
 */
DWORD STDCALL
ResumeThread(HANDLE hThread)
{
  ULONG PreviousResumeCount;
  NTSTATUS Status;

  Status = NtResumeThread(hThread,
			  &PreviousResumeCount);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(-1);
    }

  return(PreviousResumeCount);
}


/*
 * @implemented
 */
BOOL STDCALL
TerminateThread(HANDLE hThread,
		DWORD dwExitCode)
{
  NTSTATUS Status;

  if (0 == hThread)
    {
      SetLastError(ERROR_INVALID_HANDLE);
      return(FALSE);
    }

  Status = NtTerminateThread(hThread,
			     dwExitCode);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  return(TRUE);
}


/*
 * @implemented
 */
DWORD STDCALL
SuspendThread(HANDLE hThread)
{
  ULONG PreviousSuspendCount;
  NTSTATUS Status;

  Status = NtSuspendThread(hThread,
			   &PreviousSuspendCount);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(-1);
    }

  return(PreviousSuspendCount);
}


/*
 * @implemented
 */
DWORD STDCALL
SetThreadAffinityMask(HANDLE hThread,
		      DWORD dwThreadAffinityMask)
{
  THREAD_BASIC_INFORMATION ThreadBasic;
  KAFFINITY AffinityMask;
  NTSTATUS Status;

  AffinityMask = (KAFFINITY)dwThreadAffinityMask;

  Status = NtQueryInformationThread(hThread,
				    ThreadBasicInformation,
				    &ThreadBasic,
				    sizeof(THREAD_BASIC_INFORMATION),
				    NULL);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(0);
    }

  Status = NtSetInformationThread(hThread,
				  ThreadAffinityMask,
				  &AffinityMask,
				  sizeof(KAFFINITY));
  if (!NT_SUCCESS(Status))
    SetLastErrorByStatus(Status);

  return(ThreadBasic.AffinityMask);
}


/*
 * @implemented
 */
BOOL STDCALL
SetThreadPriority(HANDLE hThread,
		  int nPriority)
{
  ULONG Prio = nPriority;
  NTSTATUS Status;

  Status = NtSetInformationThread(hThread,
				  ThreadBasePriority,
				  &Prio,
				  sizeof(ULONG));

  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  return(TRUE);
}


/*
 * @implemented
 */
int STDCALL
GetThreadPriority(HANDLE hThread)
{
  THREAD_BASIC_INFORMATION ThreadBasic;
  NTSTATUS Status;

  Status = NtQueryInformationThread(hThread,
				    ThreadBasicInformation,
				    &ThreadBasic,
				    sizeof(THREAD_BASIC_INFORMATION),
				    NULL);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(THREAD_PRIORITY_ERROR_RETURN);
    }

  return(ThreadBasic.BasePriority);
}


/*
 * @implemented
 */
BOOL STDCALL
GetThreadPriorityBoost(IN HANDLE hThread,
		       OUT PBOOL pDisablePriorityBoost)
{
  ULONG PriorityBoost;
  NTSTATUS Status;

  Status = NtQueryInformationThread(hThread,
				    ThreadPriorityBoost,
				    &PriorityBoost,
				    sizeof(ULONG),
				    NULL);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  *pDisablePriorityBoost = !((BOOL)PriorityBoost);

  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
SetThreadPriorityBoost(IN HANDLE hThread,
		       IN BOOL bDisablePriorityBoost)
{
  ULONG PriorityBoost;
  NTSTATUS Status;

  PriorityBoost = (ULONG)!bDisablePriorityBoost;

  Status = NtSetInformationThread(hThread,
				  ThreadPriorityBoost,
				  &PriorityBoost,
				  sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
GetThreadSelectorEntry(IN HANDLE hThread,
		       IN DWORD dwSelector,
		       OUT LPLDT_ENTRY lpSelectorEntry)
{
  DESCRIPTOR_TABLE_ENTRY DescriptionTableEntry;
  NTSTATUS Status;
  
  DescriptionTableEntry.Selector = dwSelector;
  Status = NtQueryInformationThread(hThread,
                                    ThreadDescriptorTableEntry,
                                    &DescriptionTableEntry,
                                    sizeof(DESCRIPTOR_TABLE_ENTRY),
                                    NULL);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  *lpSelectorEntry = DescriptionTableEntry.Descriptor;
  return TRUE;
}


/*
 * @implemented
 */
DWORD STDCALL
SetThreadIdealProcessor(HANDLE hThread,
			DWORD dwIdealProcessor)
{
  NTSTATUS Status;

  Status = NtSetInformationThread(hThread,
				  ThreadIdealProcessor,
				  &dwIdealProcessor,
				  sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return -1;
    }

  return dwIdealProcessor;
}


/*
 * @implemented
 */
DWORD STDCALL
GetProcessIdOfThread(HANDLE Thread)
{
  THREAD_BASIC_INFORMATION ThreadBasic;
  NTSTATUS Status;

  Status = NtQueryInformationThread(Thread,
                                    ThreadBasicInformation,
                                    &ThreadBasic,
                                    sizeof(THREAD_BASIC_INFORMATION),
                                    NULL);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return 0;
  }

  return (DWORD)ThreadBasic.ClientId.UniqueProcess;
}


/*
 * @implemented
 */
DWORD STDCALL
GetThreadId(HANDLE Thread)
{
  THREAD_BASIC_INFORMATION ThreadBasic;
  NTSTATUS Status;

  Status = NtQueryInformationThread(Thread,
                                    ThreadBasicInformation,
                                    &ThreadBasic,
                                    sizeof(THREAD_BASIC_INFORMATION),
                                    NULL);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return 0;
  }

  return (DWORD)ThreadBasic.ClientId.UniqueThread;
}

/*
 * @unimplemented
 */
VOID STDCALL
SetThreadUILanguage(DWORD Unknown1)
{
  DPRINT1("SetThreadUILanguage(0x%x) unimplemented!\n", Unknown1);
}

static void CALLBACK
IntCallUserApc(PVOID Function, PVOID dwData, PVOID Argument3)
{
   PAPCFUNC pfnAPC = (PAPCFUNC)Function;
   pfnAPC((ULONG_PTR)dwData);
}

/*
 * @implemented
 */
DWORD STDCALL
QueueUserAPC(PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData)
{
  NTSTATUS Status;

  Status = NtQueueApcThread(hThread, IntCallUserApc, pfnAPC,
                            (PVOID)dwData, NULL);
  if (Status)
    SetLastErrorByStatus(Status);

  return NT_SUCCESS(Status);
}

/*
 * @implemented
 */
BOOL STDCALL
GetThreadIOPendingFlag(HANDLE hThread,
                       PBOOL lpIOIsPending)
{
  ULONG IoPending;
  NTSTATUS Status;
  
  if(lpIOIsPending == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Status = NtQueryInformationThread(hThread,
                                    ThreadIsIoPending,
                                    (PVOID)&IoPending,
                                    sizeof(IoPending),
                                    NULL);
  if(NT_SUCCESS(Status))
  {
    *lpIOIsPending = ((IoPending != 0) ? TRUE : FALSE);
    return TRUE;
  }
  
  SetLastErrorByStatus(Status);
  return FALSE;
}

/* EOF */
