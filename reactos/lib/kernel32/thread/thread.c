/* $Id: thread.c,v 1.37 2003/04/26 23:13:28 hyperion Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/thread/thread.c
 * PURPOSE:         Thread functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *			Tls functions are modified from WINE
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

//static VOID ThreadAttachDlls (VOID);

/* FUNCTIONS *****************************************************************/

/* FIXME: please put this in some header */
extern NTSTATUS CDECL RtlCreateUserThreadVa
(
 HANDLE ProcessHandle,
 POBJECT_ATTRIBUTES ObjectAttributes,
 BOOLEAN CreateSuspended,
 LONG StackZeroBits,
 PULONG StackReserve,
 PULONG StackCommit,
 PTHREAD_START_ROUTINE StartAddress,
 PHANDLE ThreadHandle,
 PCLIENT_ID ClientId,
 ULONG ParameterCount,
 ...
);

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


static VOID STDCALL
ThreadStartup(LPTHREAD_START_ROUTINE lpStartAddress,
	      LPVOID lpParameter)
{
  UINT uExitCode;

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


HANDLE STDCALL CreateThread
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


HANDLE STDCALL CreateRemoteThread
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
 PSECURITY_DESCRIPTOR pSD = NULL;
 HANDLE hThread;
 CLIENT_ID cidClientId;
 NTSTATUS nErrCode;
 ULONG_PTR nStackReserve;
 ULONG_PTR nStackCommit;
 OBJECT_ATTRIBUTES oaThreadAttribs;
 PIMAGE_NT_HEADERS pinhHeader =
  RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);

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

 /* create the thread */
 nErrCode = RtlCreateUserThreadVa
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

 /* success */
 if(lpThreadId) *lpThreadId = (DWORD)cidClientId.UniqueThread;
 return hThread;
}

PTEB
GetTeb(VOID)
{
  return(NtCurrentTeb());
}


WINBOOL STDCALL
SwitchToThread(VOID)
{
  NTSTATUS errCode;
  errCode = NtYieldExecution();
  return TRUE;
}


DWORD STDCALL
GetCurrentThreadId(VOID)
{
  return((DWORD)(NtCurrentTeb()->Cid).UniqueThread);
}


VOID STDCALL
ExitThread(DWORD uExitCode)
{
  BOOLEAN LastThread;
  NTSTATUS Status;

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

  Status = NtTerminateThread(NtCurrentThread(),
			     uExitCode);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
    }
}


WINBOOL STDCALL
GetThreadTimes(HANDLE hThread,
	       LPFILETIME lpCreationTime,
	       LPFILETIME lpExitTime,
	       LPFILETIME lpKernelTime,
	       LPFILETIME lpUserTime)
{
  KERNEL_USER_TIMES KernelUserTimes;
  ULONG ReturnLength;
  NTSTATUS Status;

  Status = NtQueryInformationThread(hThread,
				    ThreadTimes,
				    &KernelUserTimes,
				    sizeof(KERNEL_USER_TIMES),
				    &ReturnLength);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  memcpy(lpCreationTime, &KernelUserTimes.CreateTime, sizeof(FILETIME));
  memcpy(lpExitTime, &KernelUserTimes.ExitTime, sizeof(FILETIME));
  memcpy(lpKernelTime, &KernelUserTimes.KernelTime, sizeof(FILETIME));
  memcpy(lpUserTime, &KernelUserTimes.UserTime, sizeof(FILETIME));

  return(TRUE);
}


WINBOOL STDCALL
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


WINBOOL STDCALL
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


WINBOOL STDCALL
GetExitCodeThread(HANDLE hThread,
		  LPDWORD lpExitCode)
{
  THREAD_BASIC_INFORMATION ThreadBasic;
  ULONG DataWritten;
  NTSTATUS Status;

  Status = NtQueryInformationThread(hThread,
				    ThreadBasicInformation,
				    &ThreadBasic,
				    sizeof(THREAD_BASIC_INFORMATION),
				    &DataWritten);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  memcpy(lpExitCode, &ThreadBasic.ExitStatus, sizeof(DWORD));

  return(TRUE);
}


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


WINBOOL STDCALL
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


DWORD STDCALL
SetThreadAffinityMask(HANDLE hThread,
		      DWORD dwThreadAffinityMask)
{
  THREAD_BASIC_INFORMATION ThreadBasic;
  KAFFINITY AffinityMask;
  ULONG DataWritten;
  NTSTATUS Status;

  AffinityMask = (KAFFINITY)dwThreadAffinityMask;

  Status = NtQueryInformationThread(hThread,
				    ThreadBasicInformation,
				    &ThreadBasic,
				    sizeof(THREAD_BASIC_INFORMATION),
				    &DataWritten);
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


WINBOOL STDCALL
SetThreadPriority(HANDLE hThread,
		  int nPriority)
{
  THREAD_BASIC_INFORMATION ThreadBasic;
  ULONG DataWritten;
  NTSTATUS Status;

  Status = NtQueryInformationThread(hThread,
				    ThreadBasicInformation,
				    &ThreadBasic,
				    sizeof(THREAD_BASIC_INFORMATION),
				    &DataWritten);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  ThreadBasic.BasePriority = nPriority;

  Status = NtSetInformationThread(hThread,
				  ThreadBasicInformation,
				  &ThreadBasic,
				  sizeof(THREAD_BASIC_INFORMATION));
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  return(TRUE);
}


int STDCALL
GetThreadPriority(HANDLE hThread)
{
  THREAD_BASIC_INFORMATION ThreadBasic;
  ULONG DataWritten;
  NTSTATUS Status;

  Status = NtQueryInformationThread(hThread,
				    ThreadBasicInformation,
				    &ThreadBasic,
				    sizeof(THREAD_BASIC_INFORMATION),
				    &DataWritten);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(THREAD_PRIORITY_ERROR_RETURN);
    }

  return(ThreadBasic.BasePriority);
}


WINBOOL STDCALL
GetThreadPriorityBoost(IN HANDLE hThread,
		       OUT PBOOL pDisablePriorityBoost)
{
  ULONG PriorityBoost;
  ULONG DataWritten;
  NTSTATUS Status;

  Status = NtQueryInformationThread(hThread,
				    ThreadPriorityBoost,
				    &PriorityBoost,
				    sizeof(ULONG),
				    &DataWritten);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  *pDisablePriorityBoost = !((WINBOOL)PriorityBoost);

  return(TRUE);
}


WINBOOL STDCALL
SetThreadPriorityBoost(IN HANDLE hThread,
		       IN WINBOOL bDisablePriorityBoost)
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


WINBOOL STDCALL
GetThreadSelectorEntry(IN HANDLE hThread,
		       IN DWORD dwSelector,
		       OUT LPLDT_ENTRY lpSelectorEntry)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return(FALSE);
}


WINBOOL STDCALL
SetThreadIdealProcessor(HANDLE hThread,
			DWORD dwIdealProcessor)
{
  ULONG IdealProcessor;
  NTSTATUS Status;

  IdealProcessor = (ULONG)dwIdealProcessor;

  Status = NtSetInformationThread(hThread,
				  ThreadIdealProcessor,
				  &IdealProcessor,
				  sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  return(TRUE);
}

/* EOF */
