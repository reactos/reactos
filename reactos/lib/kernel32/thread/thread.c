/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/thread/thread.c
 * PURPOSE:         Thread functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Ariadne ( ariadne@xs4all.nl)
 *
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* FUNCTIONS *****************************************************************/
_SEH_FILTER(BaseThreadExceptionFilter)
{
   EXCEPTION_POINTERS * ExceptionInfo = _SEH_GetExceptionPointers();
   LONG ExceptionDisposition = EXCEPTION_EXECUTE_HANDLER;

   if (GlobalTopLevelExceptionFilter != NULL)
   {
      _SEH_TRY
      {
         ExceptionDisposition = GlobalTopLevelExceptionFilter(ExceptionInfo);
      }
      _SEH_HANDLE
      {
         ExceptionDisposition = UnhandledExceptionFilter(ExceptionInfo);
      }
      _SEH_END;
   }

   return ExceptionDisposition;
}

__declspec(noreturn)
VOID
STDCALL
BaseThreadStartup(LPTHREAD_START_ROUTINE lpStartAddress,
                  LPVOID lpParameter)
{
    volatile UINT uExitCode = 0;

    /* Attempt to call the Thread Start Address */
    _SEH_TRY
    {
        /* Get the exit code from the Thread Start */
        uExitCode = (lpStartAddress)((PVOID)lpParameter);
    }
    _SEH_EXCEPT(BaseThreadExceptionFilter)
    {
        /* Get the Exit code from the SEH Handler */
        uExitCode = _SEH_GetExceptionCode();
    } _SEH_END;
   
    /* Exit the Thread */
    ExitThread(uExitCode);
}

/*
 * @implemented
 */
HANDLE
STDCALL
CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,
             DWORD dwStackSize,
             LPTHREAD_START_ROUTINE lpStartAddress,
             LPVOID lpParameter,
             DWORD dwCreationFlags,
             LPDWORD lpThreadId)
{
    /* Act as if we're going to create a remote thread in ourselves */
    return CreateRemoteThread(NtCurrentProcess(),
                              lpThreadAttributes,
                              dwStackSize,
                              lpStartAddress,
                              lpParameter,
                              dwCreationFlags,
                              lpThreadId);
}

/*
 * @implemented
 */
HANDLE
STDCALL
CreateRemoteThread(HANDLE hProcess,
                   LPSECURITY_ATTRIBUTES lpThreadAttributes,
                   DWORD dwStackSize,
                   LPTHREAD_START_ROUTINE lpStartAddress,
                   LPVOID lpParameter,
                   DWORD dwCreationFlags,
                   LPDWORD lpThreadId)
{
    NTSTATUS Status;
    INITIAL_TEB InitialTeb;
    CONTEXT Context;
    CLIENT_ID ClientId;
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    POBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hThread;
    ULONG Dummy;
    
    DPRINT("CreateRemoteThread: hProcess: %ld dwStackSize: %ld lpStartAddress"
            ": %p lpParameter: %lx, dwCreationFlags: %lx\n", hProcess, 
            dwStackSize, lpStartAddress, lpParameter, dwCreationFlags);
    
    /* Clear the Context */
    RtlZeroMemory(&Context, sizeof(CONTEXT));
    
    /* Write PID */
    ClientId.UniqueProcess = hProcess;
    
    /* Create the Stack */
    Status = BasepCreateStack(hProcess,
                              dwStackSize,
                              dwCreationFlags & STACK_SIZE_PARAM_IS_A_RESERVATION ?
                              dwStackSize : 0,
                              &InitialTeb);    
    if(!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return NULL;
    }
    
    /* Create Initial Context */
    BasepInitializeContext(&Context,
                           lpParameter,
                           lpStartAddress,
                           InitialTeb.StackBase,
                           1);
    
    /* initialize the attributes for the thread object */
    ObjectAttributes = BasepConvertObjectAttributes(&LocalObjectAttributes,
                                                    lpThreadAttributes,
                                                    NULL);
    
    /* Create the Kernel Thread Object */
    Status = NtCreateThread(&hThread,
                            THREAD_ALL_ACCESS,
                            ObjectAttributes,
                            hProcess,
                            &ClientId,
                            &Context,
                            &InitialTeb,
                            TRUE);
    if(!NT_SUCCESS(Status))
    {
        BasepFreeStack(hProcess, &InitialTeb);
        SetLastErrorByStatus(Status);
        return NULL;
    }
    
    #ifdef SXS_SUPPORT_ENABLED
    /* Are we in the same process? */
    if (Process = NtCurrentProcess())
    {
        PTEB Teb;
        PVOID ActivationContextStack;
        PTHREAD_BASIC_INFORMATION ThreadBasicInfo;
        PACTIVATION_CONTEXT_BASIC_INFORMATION ActivationCtxInfo;
        ULONG_PTR Cookie;
        
        /* Get the TEB */
        Status = NtQueryInformationThread(hThread,
                                          ThreadBasicIformation,
                                          &ThreadBasicInfo,
                                          sizeof(ThreadBasicInfo),
                                          NULL);
                                          
        /* Allocate the Activation Context Stack */
        Status = RtlAllocateActivationContextStack(&ActivationContextStack);
        Teb = ThreadBasicInfo.TebBaseAddress;
        
        /* Save it */
        Teb->ActivationContextStackPointer = ActivationContextStack;
        
        /* Query the Context */
        Status = RtlQueryInformationActivationContext(1,
                                                      0,
                                                      NULL,
                                                      ActivationContextBasicInformation,
                                                      &ActivationCtxInfo,
                                                      sizeof(ActivationCtxInfo),
                                                      NULL);
                                                      
        /* Does it need to be activated? */
        if (!ActivationCtxInfo.hActCtx)
        {
            /* Activate it */
            Status = RtlActivateActivationContextEx(1,
                                                    Teb,
                                                    ActivationCtxInfo.hActCtx,
                                                    &Cookie);
        }
    }
    #endif
    
    /* FIXME: Notify CSR */

    /* Success */
    if(lpThreadId) *lpThreadId = (DWORD)ClientId.UniqueThread;
    
    /* Resume it if asked */
    if (!(dwCreationFlags & CREATE_SUSPENDED))
    {
        NtResumeThread(hThread, &Dummy);
    }
    
    /* Return handle to thread */
    return hThread;
}

/*
 * @implemented
 */
VOID
STDCALL
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
    if (NT_SUCCESS(Status) && LastThread)
    {
        /* Exit the Process */
        ExitProcess(uExitCode);
    }

    /* Notify DLLs and TLS Callbacks of termination */
    LdrShutdownThread();
    
    /* Tell the Kernel to free the Stack */
    NtCurrentTeb()->FreeStackOnTermination = TRUE;
    NtTerminateThread(NULL, uExitCode);
    
    /* We will never reach this place. This silences the compiler */
    ExitThread(uExitCode);
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

   ClientId.UniqueProcess = 0;
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
  NTSTATUS Status;
  Status = NtYieldExecution();
  return Status != STATUS_NO_YIELD_PERFORMED;
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
LANGID STDCALL
SetThreadUILanguage(WORD wReserved)
{
  DPRINT1("SetThreadUILanguage(0x%4x) unimplemented!\n", wReserved);
  return 0;
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


/*
 * @implemented
 */
VOID STDCALL
Sleep(DWORD dwMilliseconds)
{
  SleepEx(dwMilliseconds, FALSE);
  return;
}


/*
 * @implemented
 */
DWORD STDCALL
SleepEx(DWORD dwMilliseconds,
	BOOL bAlertable)
{
  LARGE_INTEGER Interval;
  NTSTATUS errCode;

  if (dwMilliseconds != INFINITE)
    {
      /*
       * System time units are 100 nanoseconds (a nanosecond is a billionth of
       * a second).
       */
      Interval.QuadPart = -((ULONGLONG)dwMilliseconds * 10000);
    }
  else
    {
      /* Approximately 292000 years hence */
      Interval.QuadPart = -0x7FFFFFFFFFFFFFFFLL;
    }

  errCode = NtDelayExecution ((bAlertable ? TRUE : FALSE), &Interval);
  if (!NT_SUCCESS(errCode))
    {
      SetLastErrorByStatus (errCode);
      return -1;
    }
  return 0;
}

/* EOF */
