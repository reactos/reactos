/* $Id: thread.c,v 1.20 2000/09/05 23:01:07 ekohl Exp $
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

#include <ddk/ntddk.h>
#include <windows.h>
#include <kernel32/thread.h>
#include <ntdll/ldr.h>
#include <string.h>
#include <napi/i386/segment.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


static VOID ThreadAttachDlls (VOID);

/* Type for a DLL's entry point */
typedef
WINBOOL
STDCALL
(* PDLLMAIN_FUNC) (
	HANDLE	hInst,
	ULONG	ul_reason_for_call,
	LPVOID	lpReserved
	);

/* FUNCTIONS *****************************************************************/

static VOID STDCALL
ThreadStartup (LPTHREAD_START_ROUTINE lpStartAddress,
               LPVOID lpParameter)
{
   UINT uExitCode;

   ThreadAttachDlls ();

   /* FIXME: notify csrss of thread creation ?? */

   uExitCode = (lpStartAddress)(lpParameter);

   ExitThread(uExitCode);
}

static VOID
ThreadAttachDlls (VOID)
{
   PLIST_ENTRY ModuleListHead;
   PLIST_ENTRY Entry;
   PLDR_MODULE Module;

   DPRINT("ThreadAttachDlls() called\n");

   RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);

   ModuleListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
   Entry = ModuleListHead->Blink;

   while (Entry != ModuleListHead)
     {
	Module = CONTAINING_RECORD(Entry, LDR_MODULE, InInitializationOrderModuleList);

	if (Module->EntryPoint != 0)
	  {
	     PDLLMAIN_FUNC Entrypoint = (PDLLMAIN_FUNC)Module->EntryPoint;

	     DPRINT("Calling entry point at 0x%08x\n", Entrypoint);
	     Entrypoint (Module->BaseAddress,
			 DLL_THREAD_ATTACH,
			 NULL);
	  }

	Entry = Entry->Blink;
     }

   RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);

   DPRINT("ThreadAttachDlls() done\n");
}

HANDLE STDCALL CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,
			    DWORD dwStackSize,
			    LPTHREAD_START_ROUTINE lpStartAddress,
			    LPVOID lpParameter,
			    DWORD dwCreationFlags,
			    LPDWORD lpThreadId)
{
   return(CreateRemoteThread(NtCurrentProcess(),
			     lpThreadAttributes,
			     dwStackSize,
			     lpStartAddress,
			     lpParameter,
			     dwCreationFlags,
			     lpThreadId));
}

HANDLE STDCALL CreateRemoteThread(HANDLE hProcess,
				  LPSECURITY_ATTRIBUTES lpThreadAttributes,
				  DWORD dwStackSize,
				  LPTHREAD_START_ROUTINE lpStartAddress,
				  LPVOID lpParameter,
				  DWORD dwCreationFlags,
				  LPDWORD lpThreadId)
{	
   HANDLE ThreadHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID ClientId;
   CONTEXT ThreadContext;
   INITIAL_TEB InitialTeb;
   BOOLEAN CreateSuspended = FALSE;
   PVOID BaseAddress;
   DWORD StackSize;
   NTSTATUS Status;
   
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = NULL;
   ObjectAttributes.Attributes = 0;
   if (lpThreadAttributes != NULL) 
     {
	if (lpThreadAttributes->bInheritHandle)
	  ObjectAttributes.Attributes = OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = 
	  lpThreadAttributes->lpSecurityDescriptor;
     }
   ObjectAttributes.SecurityQualityOfService = NULL;
   
   if ((dwCreationFlags & CREATE_SUSPENDED) == CREATE_SUSPENDED)
     CreateSuspended = TRUE;
   else
     CreateSuspended = FALSE;

   StackSize = (dwStackSize == 0) ? 4096 : dwStackSize;

   BaseAddress = 0;

   Status = NtAllocateVirtualMemory(hProcess,
                                    &BaseAddress,
                                    0,
                                    (PULONG)&StackSize,
                                    MEM_COMMIT,
                                    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
        DPRINT("Could not allocate stack space!\n");
        return NULL;
     }


   DPRINT("Stack base address: %p\n", BaseAddress);
   
   memset(&ThreadContext,0,sizeof(CONTEXT));
   ThreadContext.Eip = (LONG)ThreadStartup;
   ThreadContext.SegGs = USER_DS;
   ThreadContext.SegFs = USER_DS;
   ThreadContext.SegEs = USER_DS;
   ThreadContext.SegDs = USER_DS;
   ThreadContext.SegCs = USER_CS;
   ThreadContext.SegSs = USER_DS;
   ThreadContext.Esp = (ULONG)(BaseAddress + StackSize - 12);
   ThreadContext.EFlags = (1<<1) + (1<<9);

   /* initialize call stack */
   *((PULONG)(BaseAddress + StackSize - 4)) = (ULONG)lpParameter;
   *((PULONG)(BaseAddress + StackSize - 8)) = (ULONG)lpStartAddress;
   *((PULONG)(BaseAddress + StackSize - 12)) = 0xdeadbeef;

   DPRINT("Esp: %p\n", ThreadContext.Esp);
   DPRINT("Eip: %p\n", ThreadContext.Eip);

   Status = NtCreateThread(&ThreadHandle,
                           THREAD_ALL_ACCESS,
                           &ObjectAttributes,
                           hProcess,
                           &ClientId,
                           &ThreadContext,
                           &InitialTeb,
                           CreateSuspended);

   if (!NT_SUCCESS(Status))
     {
        DPRINT("NtCreateThread() failed!\n");
        return NULL;
     }

   if ( lpThreadId != NULL )
     memcpy(lpThreadId, &ClientId.UniqueThread,sizeof(ULONG));
   
   return ThreadHandle;
}

NT_TEB *GetTeb(VOID)
{
	return NtCurrentTeb();
}

WINBOOL STDCALL SwitchToThread(VOID)
{
   NTSTATUS errCode;
   errCode = NtYieldExecution();
   return TRUE;
}

DWORD STDCALL GetCurrentThreadId()
{
   return((DWORD)(NtCurrentTeb()->Cid).UniqueThread);
}

VOID STDCALL ExitThread(UINT uExitCode)
{
   NTSTATUS errCode;
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
	ExitProcess (uExitCode);
     }

   /* FIXME: notify csrss of thread termination */

   LdrShutdownThread();

   errCode = NtTerminateThread(NtCurrentThread(),
			       uExitCode);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
     }
}

WINBOOL STDCALL GetThreadTimes(HANDLE hThread,
			       LPFILETIME lpCreationTime,
			       LPFILETIME lpExitTime,
			       LPFILETIME lpKernelTime,
			       LPFILETIME lpUserTime)
{
   NTSTATUS errCode;
   KERNEL_USER_TIMES KernelUserTimes;
   ULONG ReturnLength;
   
   errCode = NtQueryInformationThread(hThread,
				      ThreadTimes,
				      &KernelUserTimes,
				      sizeof(KERNEL_USER_TIMES),
				      &ReturnLength);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   memcpy(lpCreationTime, &KernelUserTimes.CreateTime, sizeof(FILETIME));
   memcpy(lpExitTime, &KernelUserTimes.ExitTime, sizeof(FILETIME));
   memcpy(lpKernelTime, &KernelUserTimes.KernelTime, sizeof(FILETIME));
   memcpy(lpUserTime, &KernelUserTimes.UserTime, sizeof(FILETIME));
   return TRUE;
}


WINBOOL STDCALL GetThreadContext(HANDLE hThread,
				 LPCONTEXT lpContext)
{
   NTSTATUS errCode;
   
   errCode = NtGetContextThread(hThread,
				lpContext);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   return TRUE;
}

WINBOOL STDCALL SetThreadContext(HANDLE hThread,
				 CONST CONTEXT *lpContext)
{
   NTSTATUS errCode;
   
   errCode = NtSetContextThread(hThread,
				(void *)lpContext);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   return TRUE;
}

WINBOOL STDCALL GetExitCodeThread(HANDLE hThread,
				  LPDWORD lpExitCode)
{
   NTSTATUS errCode;
   THREAD_BASIC_INFORMATION ThreadBasic;
   ULONG DataWritten;
   
   errCode = NtQueryInformationThread(hThread,
				      ThreadBasicInformation,
				      &ThreadBasic,
				      sizeof(THREAD_BASIC_INFORMATION),
				      &DataWritten);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   memcpy(lpExitCode, &ThreadBasic.ExitStatus, sizeof(DWORD));
   return TRUE;
}

DWORD STDCALL ResumeThread(HANDLE hThread)
{
   NTSTATUS errCode;
   ULONG PreviousResumeCount;
   
   errCode = NtResumeThread(hThread,
			    &PreviousResumeCount);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return  -1;
     }
   return PreviousResumeCount;
}


WINBOOL
STDCALL
TerminateThread (
	HANDLE	hThread,
	DWORD	dwExitCode
	)
{
   NTSTATUS errCode;

   errCode = NtTerminateThread(hThread,
			    dwExitCode);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return  FALSE;
     }
   return TRUE;
}


DWORD STDCALL SuspendThread(HANDLE hThread)
{
   NTSTATUS errCode;
   ULONG PreviousSuspendCount;
   
   errCode = NtSuspendThread(hThread,
			     &PreviousSuspendCount);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return -1;
     }
   return PreviousSuspendCount;
}

DWORD STDCALL SetThreadAffinityMask(HANDLE hThread,
				    DWORD dwThreadAffinityMask)
{
   return 0;
}

WINBOOL STDCALL SetThreadPriority(HANDLE hThread,
				  int nPriority)
{
   NTSTATUS errCode;
   THREAD_BASIC_INFORMATION ThreadBasic;
   ULONG DataWritten;
   
   errCode = NtQueryInformationThread(hThread,
				      ThreadBasicInformation,
				      &ThreadBasic,
				      sizeof(THREAD_BASIC_INFORMATION),
				      &DataWritten);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   ThreadBasic.BasePriority = nPriority;
   errCode = NtSetInformationThread(hThread,
				    ThreadBasicInformation,
				    &ThreadBasic,
				    sizeof(THREAD_BASIC_INFORMATION));
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   return TRUE;
}

int STDCALL GetThreadPriority(HANDLE hThread)
{
   NTSTATUS errCode;
   THREAD_BASIC_INFORMATION ThreadBasic;
   ULONG DataWritten;
   
   errCode = NtQueryInformationThread(hThread,
				      ThreadBasicInformation,
				      &ThreadBasic,
				      sizeof(THREAD_BASIC_INFORMATION),
				      &DataWritten);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return THREAD_PRIORITY_ERROR_RETURN;
     }
   return ThreadBasic.BasePriority;
}

/* EOF */
