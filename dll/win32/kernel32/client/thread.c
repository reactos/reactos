/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/thread.c
 * PURPOSE:         Thread functions
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Ariadne (ariadne@xs4all.nl)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

#define SXS_SUPPORT_FIXME

typedef NTSTATUS (NTAPI *PCSR_CREATE_REMOTE_THREAD)(IN HANDLE ThreadHandle, IN PCLIENT_ID ClientId);

/* FUNCTIONS ******************************************************************/

NTSTATUS
WINAPI
BasepNotifyCsrOfThread(IN HANDLE ThreadHandle,
                       IN PCLIENT_ID ClientId)
{
    BASE_API_MESSAGE ApiMessage;
    PBASE_CREATE_THREAD CreateThreadRequest = &ApiMessage.Data.CreateThreadRequest;

    DPRINT("BasepNotifyCsrOfThread: Thread: %p, Handle %p\n",
            ClientId->UniqueThread, ThreadHandle);

    /* Fill out the request */
    CreateThreadRequest->ClientId = *ClientId;
    CreateThreadRequest->ThreadHandle = ThreadHandle;

    /* Call CSR */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepCreateThread),
                        sizeof(*CreateThreadRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        DPRINT1("Failed to tell CSRSS about new thread: %lx\n", ApiMessage.Status);
        return ApiMessage.Status;
    }

    /* Return Success */
    return STATUS_SUCCESS;
}

DECLSPEC_NORETURN
VOID
WINAPI
BaseThreadStartup(
    _In_ LPTHREAD_START_ROUTINE lpStartAddress,
    _In_ LPVOID lpParameter)
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
        ExitThread(lpStartAddress(lpParameter));
    }
    _SEH2_EXCEPT(UnhandledExceptionFilter(_SEH2_GetExceptionInformation()))
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

VOID
NTAPI
BaseDispatchApc(IN PAPCFUNC ApcRoutine,
                IN PVOID Data,
                IN PACTIVATION_CONTEXT ActivationContext)
{
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME ActivationFrame;

    /* Setup the activation context */
    ActivationFrame.Size = sizeof(ActivationFrame);
    ActivationFrame.Format = RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER;

    /* Check if caller wanted one */
    if (ActivationContext == INVALID_ACTIVATION_CONTEXT)
    {
        /* Do the APC directly */
        ApcRoutine((ULONG_PTR)Data);
        return;
    }

    /* Then activate it */
    RtlActivateActivationContextUnsafeFast(&ActivationFrame, ActivationContext);

    /* Call the routine under SEH */
    _SEH2_TRY
    {
        ApcRoutine((ULONG_PTR)Data);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {

    }
    _SEH2_END;

    /* Now de-activate and release the activation context */
    RtlDeactivateActivationContextUnsafeFast(&ActivationFrame);
    RtlReleaseActivationContext(ActivationContext);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
HANDLE
WINAPI
DECLSPEC_HOTPATCH
CreateThread(IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
             IN DWORD dwStackSize,
             IN LPTHREAD_START_ROUTINE lpStartAddress,
             IN LPVOID lpParameter,
             IN DWORD dwCreationFlags,
             OUT LPDWORD lpThreadId)
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
CreateRemoteThread(IN HANDLE hProcess,
                   IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
                   IN DWORD dwStackSize,
                   IN LPTHREAD_START_ROUTINE lpStartAddress,
                   IN LPVOID lpParameter,
                   IN DWORD dwCreationFlags,
                   OUT LPDWORD lpThreadId)
{
    NTSTATUS Status;
    INITIAL_TEB InitialTeb;
    CONTEXT Context;
    CLIENT_ID ClientId;
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    POBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hThread;
    ULONG Dummy;
    PTEB Teb;
    THREAD_BASIC_INFORMATION ThreadBasicInfo;
    PACTIVATION_CONTEXT_STACK ActivationContextStack = NULL;
    ACTIVATION_CONTEXT_BASIC_INFORMATION ActCtxInfo;
    ULONG_PTR Cookie;
    ULONG ReturnLength;
    SIZE_T ReturnSize;

    DPRINT("CreateRemoteThread: hProcess: %p dwStackSize: %lu lpStartAddress"
           ": %p lpParameter: %p, dwCreationFlags: %lx\n", hProcess,
           dwStackSize, lpStartAddress, lpParameter, dwCreationFlags);

    /* Clear the Context */
    RtlZeroMemory(&Context, sizeof(Context));

    /* Write PID */
    ClientId.UniqueProcess = hProcess;

    /* Create the Stack */
    Status = BaseCreateStack(hProcess,
                             (dwCreationFlags & STACK_SIZE_PARAM_IS_A_RESERVATION) ?
                                0 : dwStackSize,
                             (dwCreationFlags & STACK_SIZE_PARAM_IS_A_RESERVATION) ?
                                dwStackSize : 0,
                             &InitialTeb);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return NULL;
    }

    /* Create the Initial Context */
    BaseInitializeContext(&Context,
                          lpParameter,
                          lpStartAddress,
                          InitialTeb.StackBase,
                          1);

    /* Initialize the attributes for the thread object */
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
    if (!NT_SUCCESS(Status))
    {
        /* Fail the kernel create */
        BaseFreeThreadStack(hProcess, &InitialTeb);
        BaseSetLastNTError(Status);
        return NULL;
    }

    /* Are we in the same process? */
    if (hProcess == NtCurrentProcess())
    {
        /* Get the TEB */
        Status = NtQueryInformationThread(hThread,
                                          ThreadBasicInformation,
                                          &ThreadBasicInfo,
                                          sizeof(ThreadBasicInfo),
                                          &ReturnLength);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            DPRINT1("SXS: %s - Failing thread create because "
                    "NtQueryInformationThread() failed with status %08lx\n",
                    __FUNCTION__, Status);
            goto Quit;
        }

        /* Allocate the Activation Context Stack */
        Status = RtlAllocateActivationContextStack(&ActivationContextStack);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            DPRINT1("SXS: %s - Failing thread create because "
                    "RtlAllocateActivationContextStack() failed with status %08lx\n",
                    __FUNCTION__, Status);
            goto Quit;
        }

        /* Save it */
        Teb = ThreadBasicInfo.TebBaseAddress;
        Teb->ActivationContextStackPointer = ActivationContextStack;

        /* Query the Context */
        Status = RtlQueryInformationActivationContext(RTL_QUERY_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT,
                                                      NULL,
                                                      0,
                                                      ActivationContextBasicInformation,
                                                      &ActCtxInfo,
                                                      sizeof(ActCtxInfo),
                                                      &ReturnSize);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            DPRINT1("SXS: %s - Failing thread create because "
                    "RtlQueryInformationActivationContext() failed with status %08lx\n",
                    __FUNCTION__, Status);
            goto Quit;
        }

        /* Does it need to be activated? */
        if ((ActCtxInfo.hActCtx) && !(ActCtxInfo.dwFlags & 1))
        {
            /* Activate it */
            Status = RtlActivateActivationContextEx(RTL_ACTIVATE_ACTIVATION_CONTEXT_EX_FLAG_RELEASE_ON_STACK_DEALLOCATION,
                                                    Teb,
                                                    ActCtxInfo.hActCtx,
                                                    &Cookie);
            if (!NT_SUCCESS(Status))
            {
                /* Fail */
                DPRINT1("SXS: %s - Failing thread create because "
                        "RtlActivateActivationContextEx() failed with status %08lx\n",
                        __FUNCTION__, Status);
                goto Quit;
            }
        }

        /* Sync the service tag with the parent thread's one */
        Teb->SubProcessTag = NtCurrentTeb()->SubProcessTag;
    }

    /* Notify CSR */
    if (!BaseRunningInServerProcess)
    {
        Status = BasepNotifyCsrOfThread(hThread, &ClientId);
    }
    else
    {
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

Quit:
    if (!NT_SUCCESS(Status))
    {
        /* Failed to create the thread */

        /* Free the activation context stack */
        if (ActivationContextStack)
            RtlFreeActivationContextStack(ActivationContextStack);

        NtTerminateThread(hThread, Status);
        // FIXME: Wait for the thread to terminate?
        BaseFreeThreadStack(hProcess, &InitialTeb);
        NtClose(hThread);

        BaseSetLastNTError(Status);
        return NULL;
    }

    /* Success */
    if (lpThreadId)
        *lpThreadId = HandleToUlong(ClientId.UniqueThread);

    /* Resume the thread if asked */
    if (!(dwCreationFlags & CREATE_SUSPENDED))
        NtResumeThread(hThread, &Dummy);

    /* Return handle to thread */
    return hThread;
}

/*
 * @implemented
 */
VOID
WINAPI
ExitThread(IN DWORD uExitCode)
{
    NTSTATUS Status;
    ULONG LastThread;
    PRTL_CRITICAL_SECTION LoaderLock;

    /* Make sure loader lock isn't held */
    LoaderLock = NtCurrentPeb()->LoaderLock;
    if (LoaderLock) ASSERT(NtCurrentTeb()->ClientId.UniqueThread != LoaderLock->OwningThread);

    /*
     * Terminate process if this is the last thread
     * of the current process
     */
    Status = NtQueryInformationThread(NtCurrentThread(),
                                      ThreadAmILastThread,
                                      &LastThread,
                                      sizeof(LastThread),
                                      NULL);
    if ((NT_SUCCESS(Status)) && (LastThread)) ExitProcess(uExitCode);

    /* Notify DLLs and TLS Callbacks of termination */
    LdrShutdownThread();

    /* Tell the Kernel to free the Stack */
    NtCurrentTeb()->FreeStackOnTermination = TRUE;
    NtTerminateThread(NULL, uExitCode);

    /* We should never reach this place */
    ERROR_FATAL("It should not happen\n");
    while (TRUE); /* 'noreturn' function */
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenThread(IN DWORD dwDesiredAccess,
           IN BOOL bInheritHandle,
           IN DWORD dwThreadId)
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId;

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
GetThreadTimes(IN HANDLE hThread,
               OUT LPFILETIME lpCreationTime,
               OUT LPFILETIME lpExitTime,
               OUT LPFILETIME lpKernelTime,
               OUT LPFILETIME lpUserTime)
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
        return FALSE;
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
GetThreadContext(IN HANDLE hThread,
                 OUT LPCONTEXT lpContext)
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
SetThreadContext(IN HANDLE hThread,
                 IN CONST CONTEXT *lpContext)
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
GetExitCodeThread(IN HANDLE hThread,
                  OUT LPDWORD lpExitCode)
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
        return FALSE;
    }

    *lpExitCode = ThreadBasic.ExitStatus;
    return TRUE;
}

/*
 * @implemented
 */
DWORD
WINAPI
ResumeThread(IN HANDLE hThread)
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
TerminateThread(IN HANDLE hThread,
                IN DWORD dwExitCode)
{
    NTSTATUS Status;
#if DBG
    PRTL_CRITICAL_SECTION LoaderLock;
    THREAD_BASIC_INFORMATION ThreadInfo;
#endif /* DBG */

    /* Check for invalid thread handle */
    if (!hThread)
    {
        /* Fail if one was passed */
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

#if DBG
    /* Get the loader lock */
    LoaderLock = NtCurrentPeb()->LoaderLock;
    if (LoaderLock)
    {
        /* Get our TID */
        Status = NtQueryInformationThread(hThread,
                                          ThreadBasicInformation,
                                          &ThreadInfo,
                                          sizeof(ThreadInfo),
                                          NULL);
        if (NT_SUCCESS(Status))
        {
            /* If terminating the current thread, we must not hold the loader lock */
            if (NtCurrentTeb()->ClientId.UniqueThread == ThreadInfo.ClientId.UniqueThread)
                ASSERT(NtCurrentTeb()->ClientId.UniqueThread != LoaderLock->OwningThread);
        }
    }
#endif /* DBG */

    /* Now terminate the thread */
    Status = NtTerminateThread(hThread, dwExitCode);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* All done */
    return TRUE;
}

/*
 * @implemented
 */
DWORD
WINAPI
SuspendThread(IN HANDLE hThread)
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
SetThreadAffinityMask(IN HANDLE hThread,
                      IN DWORD_PTR dwThreadAffinityMask)
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
SetThreadPriority(IN HANDLE hThread,
                  IN int nPriority)
{
    LONG Prio = nPriority;
    NTSTATUS Status;

    /* Check if values forcing saturation should be used */
    if (Prio == THREAD_PRIORITY_TIME_CRITICAL)
    {
        /* This is 16 */
        Prio = (HIGH_PRIORITY + 1) / 2;
    }
    else if (Prio == THREAD_PRIORITY_IDLE)
    {
        /* This is -16 */
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
GetThreadPriority(IN HANDLE hThread)
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

    PriorityBoost = bDisablePriorityBoost != FALSE;

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
SetThreadIdealProcessor(IN HANDLE hThread,
                        IN DWORD dwIdealProcessor)
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
DWORD
WINAPI
GetProcessIdOfThread(IN HANDLE Thread)
{
    THREAD_BASIC_INFORMATION ThreadBasic;
    NTSTATUS Status;

    Status = NtQueryInformationThread(Thread,
                                      ThreadBasicInformation,
                                      &ThreadBasic,
                                      sizeof(THREAD_BASIC_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return HandleToUlong(ThreadBasic.ClientId.UniqueProcess);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetThreadId(IN HANDLE Thread)
{
    THREAD_BASIC_INFORMATION ThreadBasic;
    NTSTATUS Status;

    Status = NtQueryInformationThread(Thread,
                                      ThreadBasicInformation,
                                      &ThreadBasic,
                                      sizeof(THREAD_BASIC_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return HandleToUlong(ThreadBasic.ClientId.UniqueThread);
}

/*
 * @unimplemented
 */
LANGID
WINAPI
SetThreadUILanguage(IN LANGID LangId)
{
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    /* We only support LangId == 0, for selecting a language
     * identifier that best supports the NT Console. */
    if (LangId != 0)
    {
        BaseSetLastNTError(STATUS_NOT_SUPPORTED);
        return 0;
    }
#endif

    UNIMPLEMENTED;

    return LANGIDFROMLCID(NtCurrentTeb()->CurrentLocale);
}

/*
 * @implemented
 */
DWORD
WINAPI
QueueUserAPC(IN PAPCFUNC pfnAPC,
             IN HANDLE hThread,
             IN ULONG_PTR dwData)
{
    NTSTATUS Status;
    ACTIVATION_CONTEXT_BASIC_INFORMATION ActCtxInfo;

    /* Zero the activation context and query information on it */
    RtlZeroMemory(&ActCtxInfo, sizeof(ActCtxInfo));
    Status = RtlQueryInformationActivationContext(RTL_QUERY_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT,
                                                  NULL,
                                                  0,
                                                  ActivationContextBasicInformation,
                                                  &ActCtxInfo,
                                                  sizeof(ActCtxInfo),
                                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail due to SxS */
        DbgPrint("SXS: %s failing because RtlQueryInformationActivationContext()"
                 "returned status %08lx\n", __FUNCTION__, Status);
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Queue the APC */
    Status = NtQueueApcThread(hThread,
                              (PKNORMAL_ROUTINE)BaseDispatchApc,
                              pfnAPC,
                              (PVOID)dwData,
                              (ActCtxInfo.dwFlags & 1) ?
                              INVALID_ACTIVATION_CONTEXT : ActCtxInfo.hActCtx);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* All good */
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetThreadStackGuarantee(IN OUT PULONG StackSizeInBytes)
{
    PTEB Teb = NtCurrentTeb();
    ULONG GuaranteedStackBytes;
    ULONG AllocationSize;

    if (!StackSizeInBytes)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    AllocationSize = *StackSizeInBytes;

    /* Retrieve the current stack size */
    GuaranteedStackBytes = Teb->GuaranteedStackBytes;

    /* Return the size of the previous stack */
    *StackSizeInBytes = GuaranteedStackBytes;

    /*
     * If the new stack size is either zero or is less than the current size,
     * the previous stack size is returned and we return success.
     */
    if ((AllocationSize == 0) || (AllocationSize < GuaranteedStackBytes))
    {
        return TRUE;
    }

    // FIXME: Unimplemented!
    UNIMPLEMENTED_ONCE;

    // Temporary HACK for supporting applications!
    return TRUE; // FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetThreadIOPendingFlag(IN HANDLE hThread,
                       OUT PBOOL lpIOIsPending)
{
    ULONG IoPending;
    NTSTATUS Status;

    /* Query the flag */
    Status = NtQueryInformationThread(hThread,
                                      ThreadIsIoPending,
                                      &IoPending,
                                      sizeof(IoPending),
                                      NULL);
    if (NT_SUCCESS(Status))
    {
        /* Return the flag */
        *lpIOIsPending = IoPending ? TRUE : FALSE;
        return TRUE;
    }

    /* Fail */
    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
QueueUserWorkItem(IN LPTHREAD_START_ROUTINE Function,
                  IN PVOID Context,
                  IN ULONG Flags)
{
    NTSTATUS Status;

    /* NOTE: Rtl needs to safely call the function using a trampoline */
    Status = RtlQueueWorkItem((WORKERCALLBACKFUNC)Function, Context, Flags);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* All good */
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
