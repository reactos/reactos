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
#include <debug.h>

/* FIXME: NDK */
#define HIGH_PRIORITY 31
#define SXS_SUPPORT_FIXME

typedef NTSTATUS (NTAPI *PCSR_CREATE_REMOTE_THREAD)(IN HANDLE ThreadHandle, IN PCLIENT_ID ClientId);

NTSTATUS
WINAPI
BasepNotifyCsrOfThread(IN HANDLE ThreadHandle,
                       IN PCLIENT_ID ClientId);

/* FUNCTIONS *****************************************************************/
static
LONG BaseThreadExceptionFilter(EXCEPTION_POINTERS * ExceptionInfo)
{
   LONG ExceptionDisposition = EXCEPTION_EXECUTE_HANDLER;
   LPTOP_LEVEL_EXCEPTION_FILTER RealFilter;
   RealFilter = RtlDecodePointer(GlobalTopLevelExceptionFilter);
   
   if (RealFilter != NULL)
   {
      _SEH2_TRY
      {
         ExceptionDisposition = RealFilter(ExceptionInfo);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         ExceptionDisposition = UnhandledExceptionFilter(ExceptionInfo);
      }
      _SEH2_END;
   }

   return ExceptionDisposition;
}

__declspec(noreturn)
VOID
WINAPI
BaseThreadStartup(LPTHREAD_START_ROUTINE lpStartAddress,
                  LPVOID lpParameter)
{
    /* Attempt to call the Thread Start Address */
    _SEH2_TRY
    {
        /* Legacy check which is still used today for Win32 threads */
        if (NtCurrentTeb()->NtTib.Version == (30 << 8)) // OS/2 V3.0 ("Cruiser")
        {
            /* This registers the termination port with CSRSS */
            if (!BaseRunningInServerProcess) CsrNewThread();
        }

        /* Get the exit code from the Thread Start */
        ExitThread((lpStartAddress)((PVOID)lpParameter));
    }
    _SEH2_EXCEPT(BaseThreadExceptionFilter(_SEH2_GetExceptionInformation()))
    {
        /* Get the Exit code from the SEH Handler */
        if (!BaseRunningInServerProcess)
        {
            /* Kill the whole process, usually */
            ExitProcess(_SEH2_GetExceptionCode());
        }
        else
        {
            /* If running inside CSRSS, kill just this thread */
            ExitThread(_SEH2_GetExceptionCode());
        }
    }
    _SEH2_END;
}

/*
 * @implemented
 */
HANDLE
WINAPI
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
WINAPI
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
    Status = BaseCreateStack(hProcess,
                              dwStackSize,
                              dwCreationFlags & STACK_SIZE_PARAM_IS_A_RESERVATION ?
                              dwStackSize : 0,
                              &InitialTeb);
    if(!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return NULL;
    }

    /* Create Initial Context */
    BaseInitializeContext(&Context,
                           lpParameter,
                           lpStartAddress,
                           InitialTeb.StackBase,
                           1);

    /* initialize the attributes for the thread object */
    ObjectAttributes = BaseFormatObjectAttributes(&LocalObjectAttributes,
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
        BaseFreeThreadStack(hProcess, &InitialTeb);
        BaseSetLastNTError(Status);
        return NULL;
    }

    /* Are we in the same process? */
    if (hProcess == NtCurrentProcess())
    {
        PTEB Teb;
        PVOID ActivationContextStack = NULL;
        THREAD_BASIC_INFORMATION ThreadBasicInfo;
#ifndef SXS_SUPPORT_FIXME
        ACTIVATION_CONTEXT_BASIC_INFORMATION ActivationCtxInfo;
        ULONG_PTR Cookie;
#endif
        ULONG retLen;

        /* Get the TEB */
        Status = NtQueryInformationThread(hThread,
                                          ThreadBasicInformation,
                                          &ThreadBasicInfo,
                                          sizeof(ThreadBasicInfo),
                                          &retLen);
        if (NT_SUCCESS(Status))
        {
            /* Allocate the Activation Context Stack */
            Status = RtlAllocateActivationContextStack(&ActivationContextStack);
        }

        if (NT_SUCCESS(Status))
        {
            Teb = ThreadBasicInfo.TebBaseAddress;

            /* Save it */
            Teb->ActivationContextStackPointer = ActivationContextStack;
#ifndef SXS_SUPPORT_FIXME
            /* Query the Context */
            Status = RtlQueryInformationActivationContext(1,
                                                          0,
                                                          NULL,
                                                          ActivationContextBasicInformation,
                                                          &ActivationCtxInfo,
                                                          sizeof(ActivationCtxInfo),
                                                          &retLen);
            if (NT_SUCCESS(Status))
            {
                /* Does it need to be activated? */
                if (!ActivationCtxInfo.hActCtx)
                {
                    /* Activate it */
                    Status = RtlActivateActivationContext(1,
                                                          ActivationCtxInfo.hActCtx,
                                                          &Cookie);
                    if (!NT_SUCCESS(Status))
                        DPRINT1("RtlActivateActivationContext failed %x\n", Status);
                }
            }
            else
                DPRINT1("RtlQueryInformationActivationContext failed %x\n", Status);
#endif
        }
        else
            DPRINT1("RtlAllocateActivationContextStack failed %x\n", Status);
    }

    /* Notify CSR */
    if (!BaseRunningInServerProcess)
    {
        Status = BasepNotifyCsrOfThread(hThread, &ClientId);
    }
    else
    {
        DPRINT("Server thread in Server. Handle: %lx\n", hProcess);
        if (hProcess != NtCurrentProcess())
        {
            PCSR_CREATE_REMOTE_THREAD CsrCreateRemoteThread;
            
            /* Get the direct CSRSRV export */
            CsrCreateRemoteThread = (PCSR_CREATE_REMOTE_THREAD)
                                    GetProcAddress(GetModuleHandleA("csrsrv"),
                                                   "CsrCreateRemoteThread");
            if (CsrCreateRemoteThread)
            {
                /* Call it instead of going through LPC */
                Status = CsrCreateRemoteThread(hThread, &ClientId);
            }
        }
    }
    
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
    }

    /* Success */
    if(lpThreadId) *lpThreadId = HandleToUlong(ClientId.UniqueThread);

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
WINAPI
ExitThread(DWORD uExitCode)
{
    NTSTATUS Status;
    ULONG LastThread;

    /*
     * Terminate process if this is the last thread
     * of the current process
     */
    Status = NtQueryInformationThread(NtCurrentThread(),
                                      ThreadAmILastThread,
                                      &LastThread,
                                      sizeof(LastThread),
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

    /* We should never reach this place */
    DPRINT1("It should not happen\n");
    while (TRUE) ;
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenThread(DWORD dwDesiredAccess,
           BOOL bInheritHandle,
           DWORD dwThreadId)
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId ;

    ClientId.UniqueProcess = 0;
    ClientId.UniqueThread = ULongToHandle(dwThreadId);

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               (bInheritHandle ? OBJ_INHERIT : 0),
                               NULL,
                               NULL);

    Status = NtOpenThread(&ThreadHandle,
                          dwDesiredAccess,
                          &ObjectAttributes,
                          &ClientId);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
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
    return NtCurrentTeb();
}

/*
 * @implemented
 */
BOOL
WINAPI
SwitchToThread(VOID)
{
    return NtYieldExecution() != STATUS_NO_YIELD_PERFORMED;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetCurrentThreadId(VOID)
{
    return HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);
}

/*
 * @implemented
 */
BOOL
NTAPI
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
        BaseSetLastNTError(Status);
        return(FALSE);
    }

    *lpCreationTime = *(LPFILETIME)&KernelUserTimes.CreateTime;
    *lpExitTime = *(LPFILETIME)&KernelUserTimes.ExitTime;
    *lpKernelTime = *(LPFILETIME)&KernelUserTimes.KernelTime;
    *lpUserTime = *(LPFILETIME)&KernelUserTimes.UserTime;
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetThreadContext(HANDLE hThread,
                 LPCONTEXT lpContext)
{
    NTSTATUS Status;

    Status = NtGetContextThread(hThread, lpContext);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetThreadContext(HANDLE hThread,
                 CONST CONTEXT *lpContext)
{
    NTSTATUS Status;

    Status = NtSetContextThread(hThread, (PCONTEXT)lpContext);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
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
        BaseSetLastNTError(Status);
        return(FALSE);
    }

    *lpExitCode = ThreadBasic.ExitStatus;
    return TRUE;
}

/*
 * @implemented
 */
DWORD
WINAPI
ResumeThread(HANDLE hThread)
{
    ULONG PreviousResumeCount;
    NTSTATUS Status;

    Status = NtResumeThread(hThread, &PreviousResumeCount);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return -1;
    }

    return PreviousResumeCount;
}

/*
 * @implemented
 */
BOOL
WINAPI
TerminateThread(HANDLE hThread,
                DWORD dwExitCode)
{
    NTSTATUS Status;

    if (!hThread)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Status = NtTerminateThread(hThread, dwExitCode);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
DWORD
WINAPI
SuspendThread(HANDLE hThread)
{
    ULONG PreviousSuspendCount;
    NTSTATUS Status;

    Status = NtSuspendThread(hThread, &PreviousSuspendCount);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return -1;
    }

    return PreviousSuspendCount;
}

/*
 * @implemented
 */
DWORD_PTR
WINAPI
SetThreadAffinityMask(HANDLE hThread,
                      DWORD_PTR dwThreadAffinityMask)
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
        BaseSetLastNTError(Status);
        return 0;
    }

    Status = NtSetInformationThread(hThread,
                                    ThreadAffinityMask,
                                    &AffinityMask,
                                    sizeof(KAFFINITY));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        ThreadBasic.AffinityMask = 0;
    }

    return ThreadBasic.AffinityMask;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetThreadPriority(HANDLE hThread,
                  int nPriority)
{
    LONG Prio = nPriority;
    NTSTATUS Status;

    /* Check if values forcing saturation should be used */
    if (Prio == THREAD_PRIORITY_TIME_CRITICAL)
    {
        Prio = (HIGH_PRIORITY + 1) / 2;
    }
    else if (Prio == THREAD_PRIORITY_IDLE)
    {
        Prio = -((HIGH_PRIORITY + 1) / 2);
    }

    /* Set the Base Priority */
    Status = NtSetInformationThread(hThread,
                                    ThreadBasePriority,
                                    &Prio,
                                    sizeof(LONG));
    if (!NT_SUCCESS(Status))
    {
        /* Failure */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Return */
    return TRUE;
}

/*
 * @implemented
 */
int
WINAPI
GetThreadPriority(HANDLE hThread)
{
    THREAD_BASIC_INFORMATION ThreadBasic;
    NTSTATUS Status;

    /* Query the Base Priority Increment */
    Status = NtQueryInformationThread(hThread,
                                      ThreadBasicInformation,
                                      &ThreadBasic,
                                      sizeof(THREAD_BASIC_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Failure */
        BaseSetLastNTError(Status);
        return THREAD_PRIORITY_ERROR_RETURN;
    }

    /* Do some conversions for saturation values */
    if (ThreadBasic.BasePriority == ((HIGH_PRIORITY + 1) / 2))
    {
        /* Win32 calls this "time critical" */
        ThreadBasic.BasePriority = THREAD_PRIORITY_TIME_CRITICAL;
    }
    else if (ThreadBasic.BasePriority == -((HIGH_PRIORITY + 1) / 2))
    {
        /* Win32 calls this "idle" */
        ThreadBasic.BasePriority = THREAD_PRIORITY_IDLE;
    }

    /* Return the final result */
    return ThreadBasic.BasePriority;
}

/*
 * @implemented
 */
BOOL
WINAPI
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
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *pDisablePriorityBoost = PriorityBoost;
    return TRUE;
}

/*
 * @implemented
 */
BOOL
NTAPI
SetThreadPriorityBoost(IN HANDLE hThread,
                       IN BOOL bDisablePriorityBoost)
{
    ULONG PriorityBoost;
    NTSTATUS Status;

    PriorityBoost = (ULONG)bDisablePriorityBoost;

    Status = NtSetInformationThread(hThread,
                                    ThreadPriorityBoost,
                                    &PriorityBoost,
                                    sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetThreadSelectorEntry(IN HANDLE hThread,
                       IN DWORD dwSelector,
                       OUT LPLDT_ENTRY lpSelectorEntry)
{
#ifdef _M_IX86
    DESCRIPTOR_TABLE_ENTRY DescriptionTableEntry;
    NTSTATUS Status;

    /* Set the selector and do the query */
    DescriptionTableEntry.Selector = dwSelector;
    Status = NtQueryInformationThread(hThread,
                                      ThreadDescriptorTableEntry,
                                      &DescriptionTableEntry,
                                      sizeof(DESCRIPTOR_TABLE_ENTRY),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Success, return the selector */
    *lpSelectorEntry = DescriptionTableEntry.Descriptor;
    return TRUE;
#else
    DPRINT1("Calling GetThreadSelectorEntry!\n");
    return FALSE;
#endif
}

/*
 * @implemented
 */
DWORD
WINAPI
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
        BaseSetLastNTError(Status);
        return -1;
    }

    return (DWORD)Status;
}

/*
 * @implemented
 */
DWORD WINAPI
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
    BaseSetLastNTError(Status);
    return 0;
  }

  return HandleToUlong(ThreadBasic.ClientId.UniqueProcess);
}

/*
 * @implemented
 */
DWORD WINAPI
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
    BaseSetLastNTError(Status);
    return 0;
  }

  return HandleToUlong(ThreadBasic.ClientId.UniqueThread);
}

/*
 * @unimplemented
 */
LANGID WINAPI
SetThreadUILanguage(LANGID LangId)
{
  DPRINT1("SetThreadUILanguage(0x%4x) unimplemented!\n", LangId);
  return LangId;
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
DWORD WINAPI
QueueUserAPC(PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData)
{
  NTSTATUS Status;

  Status = NtQueueApcThread(hThread, IntCallUserApc, pfnAPC,
                            (PVOID)dwData, NULL);
  if (!NT_SUCCESS(Status))
  {
    BaseSetLastNTError(Status);
    return 0;
  }

  return 1;
}

BOOL
WINAPI
SetThreadStackGuarantee(IN OUT PULONG StackSizeInBytes)
{
    STUB;
    return FALSE;
}


/*
 * @implemented
 */
BOOL WINAPI
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

  BaseSetLastNTError(Status);
  return FALSE;
}


typedef struct _QUEUE_USER_WORKITEM_CONTEXT
{
    LPTHREAD_START_ROUTINE Function;
    PVOID Context;
} QUEUE_USER_WORKITEM_CONTEXT, *PQUEUE_USER_WORKITEM_CONTEXT;

static VOID
NTAPI
InternalWorkItemTrampoline(PVOID Context)
{
    QUEUE_USER_WORKITEM_CONTEXT Info;

    ASSERT(Context);

    /* Save the context to the stack */
    Info = *(volatile QUEUE_USER_WORKITEM_CONTEXT *)Context;

    /* Free the context before calling the callback. This avoids
       a memory leak in case the thread dies... */
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                Context);

    /* Call the real callback */
    Info.Function(Info.Context);
}


/*
 * @implemented
 */
BOOL
WINAPI
QueueUserWorkItem(
    LPTHREAD_START_ROUTINE Function,
    PVOID Context,
    ULONG Flags
    )
{
    PQUEUE_USER_WORKITEM_CONTEXT WorkItemContext;
    NTSTATUS Status;

    /* Save the context for the trampoline function */
    WorkItemContext = RtlAllocateHeap(RtlGetProcessHeap(),
                                      0,
                                      sizeof(*WorkItemContext));
    if (WorkItemContext == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    WorkItemContext->Function = Function;
    WorkItemContext->Context = Context;

    /* NOTE: Don't use Function directly since the callback signature
             differs. This might cause problems on certain platforms... */
    Status = RtlQueueWorkItem(InternalWorkItemTrampoline,
                              WorkItemContext,
                              Flags);
    if (!NT_SUCCESS(Status))
    {
        /* Free the allocated context in case of failure */
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    WorkItemContext);

        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
DWORD
WINAPI
TlsAlloc(VOID)
{
    ULONG Index;
    PTEB Teb;
    PPEB Peb;

    /* Get the PEB and TEB, lock the PEB */
    Teb = NtCurrentTeb();
    Peb = Teb->ProcessEnvironmentBlock;
    RtlAcquirePebLock();

    /* Try to get regular TEB slot */
    Index = RtlFindClearBitsAndSet(Peb->TlsBitmap, 1, 0);
    if (Index != 0xFFFFFFFF)
    {
        /* Clear the value. */
        Teb->TlsSlots[Index] = 0;
        RtlReleasePebLock();
        return Index;
    }

    /* If it fails, try to find expansion TEB slot. */
    Index = RtlFindClearBitsAndSet(Peb->TlsExpansionBitmap, 1, 0);
    if (Index != 0xFFFFFFFF)
    {
        /* Is there no expansion slot yet? */
        if (!Teb->TlsExpansionSlots)
        {
            /* Allocate an array */
            Teb->TlsExpansionSlots = RtlAllocateHeap(RtlGetProcessHeap(),
                                                     HEAP_ZERO_MEMORY,
                                                     TLS_EXPANSION_SLOTS *
                                                     sizeof(PVOID));
        }

        /* Did we get an array? */
        if (!Teb->TlsExpansionSlots)
        {
            /* Fail */
            RtlClearBits(Peb->TlsExpansionBitmap, Index, 1);
            Index = 0xFFFFFFFF;
            BaseSetLastNTError(STATUS_NO_MEMORY);
        }
        else
        {
            /* Clear the value. */
            Teb->TlsExpansionSlots[Index] = 0;
            Index += TLS_MINIMUM_AVAILABLE;
        }
    }
    else
    {
        /* Fail */
        BaseSetLastNTError(STATUS_NO_MEMORY);
    }

    /* Release the lock and return */
    RtlReleasePebLock();
    return Index;
}

/*
 * @implemented
 */
BOOL
WINAPI
TlsFree(IN DWORD Index)
{
    BOOL BitSet;
    PPEB Peb;
    ULONG TlsIndex;
    PVOID TlsBitmap;
    NTSTATUS Status;

    /* Acquire the PEB lock and grab the PEB */
    Peb = NtCurrentPeb();
    RtlAcquirePebLock();

    /* Check if the index is too high */
    if (Index >= TLS_MINIMUM_AVAILABLE)
    {
        /* Check if it can fit in the expansion slots */
        TlsIndex = Index - TLS_MINIMUM_AVAILABLE;
        if (TlsIndex >= TLS_EXPANSION_SLOTS)
        {
            /* It's invalid */
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            RtlReleasePebLock();
            return FALSE;
        }
        else
        {
            /* Use the expansion bitmap */
            TlsBitmap = Peb->TlsExpansionBitmap;
            Index = TlsIndex;
        }
    }
    else
    {
        /* Use the normal bitmap */
        TlsBitmap = Peb->TlsBitmap;
    }

    /* Check if the index was set */
    BitSet = RtlAreBitsSet(TlsBitmap, Index, 1);
    if (BitSet)
    {
        /* Tell the kernel to free the TLS cells */
        Status = NtSetInformationThread(NtCurrentThread(),
                                        ThreadZeroTlsCell,
                                        &Index,
                                        sizeof(DWORD));
        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            RtlReleasePebLock();
            return FALSE;
        }

        /* Clear the bit */
        RtlClearBits(TlsBitmap, Index, 1);
    }
    else
    {
        /* Fail */
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        RtlReleasePebLock();
        return FALSE;
    }

    /* Done! */
    RtlReleasePebLock();
    return TRUE;
}

/*
 * @implemented
 */
LPVOID
WINAPI
TlsGetValue(IN DWORD Index)
{
    PTEB Teb;

    /* Get the TEB and clear the last error */
    Teb = NtCurrentTeb();
    Teb->LastErrorValue = 0;

    /* Check for simple TLS index */
    if (Index < TLS_MINIMUM_AVAILABLE)
    {
        /* Return it */
        return Teb->TlsSlots[Index];
    }

    /* Check for valid index */
    if (Index >= TLS_EXPANSION_SLOTS + TLS_MINIMUM_AVAILABLE)
    {
        /* Fail */
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return NULL;
    }

    /* The expansion slots are allocated on demand, so check for it. */
    Teb->LastErrorValue = 0;
    if (!Teb->TlsExpansionSlots) return NULL;

    /* Return the value from the expansion slots */
    return Teb->TlsExpansionSlots[Index - TLS_MINIMUM_AVAILABLE];
}

/*
 * @implemented
 */
BOOL
WINAPI
TlsSetValue(IN DWORD Index,
            IN LPVOID Value)
{
    DWORD TlsIndex;
    PTEB Teb = NtCurrentTeb();

    /* Check for simple TLS index */
    if (Index < TLS_MINIMUM_AVAILABLE)
    {
        /* Return it */
        Teb->TlsSlots[Index] = Value;
        return TRUE;
    }

    /* Check if this is an expansion slot */
    TlsIndex = Index - TLS_MINIMUM_AVAILABLE;
    if (TlsIndex >= TLS_EXPANSION_SLOTS)
    {
        /* Fail */
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    /* Do we not have expansion slots? */
    if (!Teb->TlsExpansionSlots)
    {
        /* Get the PEB lock to see if we still need them */
        RtlAcquirePebLock();
        if (!Teb->TlsExpansionSlots)
        {
            /* Allocate them */
            Teb->TlsExpansionSlots = RtlAllocateHeap(RtlGetProcessHeap(),
                                                     HEAP_ZERO_MEMORY,
                                                     TLS_EXPANSION_SLOTS *
                                                     sizeof(PVOID));
            if (!Teb->TlsExpansionSlots)
            {
                /* Fail */
                RtlReleasePebLock();
                BaseSetLastNTError(STATUS_NO_MEMORY);
                return FALSE;
            }
        }

        /* Release the lock */
        RtlReleasePebLock();
    }

    /* Write the value */
    Teb->TlsExpansionSlots[TlsIndex] = Value;

    /* Success */
    return TRUE;
}


/* EOF */
