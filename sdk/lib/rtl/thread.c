/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Rtl user thread functions
 * FILE:              lib/rtl/thread.c
 * PROGRAMERS:
 *                    Alex Ionescu (alex@relsoft.net)
 *                    Eric Kohl
 *                    KJK::Hyperion
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *******************************************************/

NTSTATUS
NTAPI
RtlpCreateUserStack(IN HANDLE ProcessHandle,
                    IN SIZE_T StackReserve OPTIONAL,
                    IN SIZE_T StackCommit OPTIONAL,
                    IN ULONG StackZeroBits OPTIONAL,
                    OUT PINITIAL_TEB InitialTeb)
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION SystemBasicInfo;
    PIMAGE_NT_HEADERS Headers;
    ULONG_PTR Stack;
    BOOLEAN UseGuard;
    ULONG Dummy;
    SIZE_T MinimumStackCommit, GuardPageSize;

    /* Get some memory information */
    Status = ZwQuerySystemInformation(SystemBasicInformation,
                                      &SystemBasicInfo,
                                      sizeof(SYSTEM_BASIC_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Use the Image Settings if we are dealing with the current Process */
    if (ProcessHandle == NtCurrentProcess())
    {
        /* Get the Image Headers */
        Headers = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);
        if (!Headers) return STATUS_INVALID_IMAGE_FORMAT;

        /* If we didn't get the parameters, find them ourselves */
        if (StackReserve == 0)
            StackReserve = Headers->OptionalHeader.SizeOfStackReserve;
        if (StackCommit == 0)
            StackCommit = Headers->OptionalHeader.SizeOfStackCommit;

        MinimumStackCommit = NtCurrentPeb()->MinimumStackCommit;
        if ((MinimumStackCommit != 0) && (StackCommit < MinimumStackCommit))
        {
            StackCommit = MinimumStackCommit;
        }
    }
    else
    {
        /* Use the System Settings if needed */
        if (StackReserve == 0)
            StackReserve = SystemBasicInfo.AllocationGranularity;
        if (StackCommit == 0)
            StackCommit = SystemBasicInfo.PageSize;
    }

    /* Check if the commit is higher than the reserve */
    if (StackCommit >= StackReserve)
    {
        /* Grow the reserve beyond the commit, up to 1MB alignment */
        StackReserve = ROUND_UP(StackCommit, 1024 * 1024);
    }

    /* Align everything to Page Size */
    StackCommit = ROUND_UP(StackCommit, SystemBasicInfo.PageSize);
    StackReserve = ROUND_UP(StackReserve, SystemBasicInfo.AllocationGranularity);

    /* Reserve memory for the stack */
    Stack = 0;
    Status = ZwAllocateVirtualMemory(ProcessHandle,
                                     (PVOID*)&Stack,
                                     StackZeroBits,
                                     &StackReserve,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Now set up some basic Initial TEB Parameters */
    InitialTeb->AllocatedStackBase = (PVOID)Stack;
    InitialTeb->StackBase = (PVOID)(Stack + StackReserve);
    InitialTeb->PreviousStackBase = NULL;
    InitialTeb->PreviousStackLimit = NULL;

    /* Update the stack position */
    Stack += StackReserve - StackCommit;

    /* Check if we can add a guard page */
    if (StackReserve >= StackCommit + SystemBasicInfo.PageSize)
    {
        Stack -= SystemBasicInfo.PageSize;
        StackCommit += SystemBasicInfo.PageSize;
        UseGuard = TRUE;
    }
    else
    {
        UseGuard = FALSE;
    }

    /* Allocate memory for the stack */
    Status = ZwAllocateVirtualMemory(ProcessHandle,
                                     (PVOID*)&Stack,
                                     0,
                                     &StackCommit,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        GuardPageSize = 0;
        ZwFreeVirtualMemory(ProcessHandle, (PVOID*)&Stack, &GuardPageSize, MEM_RELEASE);
        return Status;
    }

    /* Now set the current Stack Limit */
    InitialTeb->StackLimit = (PVOID)Stack;

    /* Create a guard page if needed */
    if (UseGuard)
    {
        GuardPageSize = SystemBasicInfo.PageSize;
        Status = ZwProtectVirtualMemory(ProcessHandle,
                                        (PVOID*)&Stack,
                                        &GuardPageSize,
                                        PAGE_GUARD | PAGE_READWRITE,
                                        &Dummy);
        if (!NT_SUCCESS(Status)) return Status;

        /* Update the Stack Limit keeping in mind the Guard Page */
        InitialTeb->StackLimit = (PVOID)((ULONG_PTR)InitialTeb->StackLimit +
                                         GuardPageSize);
    }

    /* We are done! */
    return STATUS_SUCCESS;
}

VOID
NTAPI
RtlpFreeUserStack(IN HANDLE ProcessHandle,
                  IN PINITIAL_TEB InitialTeb)
{
    SIZE_T Dummy = 0;

    /* Free the Stack */
    ZwFreeVirtualMemory(ProcessHandle,
                        &InitialTeb->AllocatedStackBase,
                        &Dummy,
                        MEM_RELEASE);

    /* Clear the initial TEB */
    RtlZeroMemory(InitialTeb, sizeof(*InitialTeb));
}

/* FUNCTIONS ***************************************************************/


/*
 * @implemented
 */
NTSTATUS
__cdecl
RtlSetThreadIsCritical(IN BOOLEAN NewValue,
                       OUT PBOOLEAN OldValue OPTIONAL,
                       IN BOOLEAN NeedBreaks)
{
    ULONG BreakOnTermination;

    /* Initialize to FALSE */
    if (OldValue) *OldValue = FALSE;

    /* Fail, if the critical breaks flag is required but is not set */
    if ((NeedBreaks) &&
        !(NtCurrentPeb()->NtGlobalFlag & FLG_ENABLE_SYSTEM_CRIT_BREAKS))
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Check if the caller wants the old value */
    if (OldValue)
    {
        /* Query and return the old break on termination flag for the process */
        ZwQueryInformationThread(NtCurrentThread(),
                                 ThreadBreakOnTermination,
                                 &BreakOnTermination,
                                 sizeof(ULONG),
                                 NULL);
        *OldValue = (BOOLEAN)BreakOnTermination;
    }

    /* Set the break on termination flag for the process */
    BreakOnTermination = NewValue;
    return ZwSetInformationThread(NtCurrentThread(),
                                  ThreadBreakOnTermination,
                                  &BreakOnTermination,
                                  sizeof(ULONG));
}

/*
 @implemented
*/
NTSTATUS
NTAPI
RtlCreateUserThread(IN HANDLE ProcessHandle,
                    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
                    IN BOOLEAN CreateSuspended,
                    IN ULONG StackZeroBits OPTIONAL,
                    IN SIZE_T StackReserve OPTIONAL,
                    IN SIZE_T StackCommit OPTIONAL,
                    IN PTHREAD_START_ROUTINE StartAddress,
                    IN PVOID Parameter OPTIONAL,
                    OUT PHANDLE ThreadHandle OPTIONAL,
                    OUT PCLIENT_ID ClientId OPTIONAL)
{
    NTSTATUS Status;
    HANDLE Handle;
    CLIENT_ID ThreadCid;
    INITIAL_TEB InitialTeb;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CONTEXT Context;

    /* First, we'll create the Stack */
    Status = RtlpCreateUserStack(ProcessHandle,
                                 StackReserve,
                                 StackCommit,
                                 StackZeroBits,
                                 &InitialTeb);
    if (!NT_SUCCESS(Status)) return Status;

    /* Next, we'll set up the Initial Context */
    RtlInitializeContext(ProcessHandle,
                         &Context,
                         Parameter,
                         StartAddress,
                         InitialTeb.StackBase);

    /* We are now ready to create the Kernel Thread Object */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               SecurityDescriptor);
    Status = ZwCreateThread(&Handle,
                            THREAD_ALL_ACCESS,
                            &ObjectAttributes,
                            ProcessHandle,
                            &ThreadCid,
                            &Context,
                            &InitialTeb,
                            CreateSuspended);
    if (!NT_SUCCESS(Status))
    {
        /* Free the stack */
        RtlpFreeUserStack(ProcessHandle, &InitialTeb);
    }
    else
    {
        /* Return thread data */
        if (ThreadHandle)
            *ThreadHandle = Handle;
        else
            NtClose(Handle);
        if (ClientId) *ClientId = ThreadCid;
    }

    /* Return success or the previous failure */
    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlExitUserThread(NTSTATUS Status)
{
    /* Call the Loader and tell him to notify the DLLs */
    LdrShutdownThread();

    /* Shut us down */
    NtCurrentTeb()->FreeStackOnTermination = TRUE;
    NtTerminateThread(NtCurrentThread(), Status);
}

/*
 @implemented
*/
VOID
NTAPI
RtlFreeUserThreadStack(HANDLE ProcessHandle,
                       HANDLE ThreadHandle)
{
    NTSTATUS Status;
    THREAD_BASIC_INFORMATION ThreadBasicInfo;
    SIZE_T Dummy, Size = 0;
    PVOID StackLocation;

    /* Query the Basic Info */
    Status = NtQueryInformationThread(ThreadHandle,
                                      ThreadBasicInformation,
                                      &ThreadBasicInfo,
                                      sizeof(THREAD_BASIC_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(Status) || !ThreadBasicInfo.TebBaseAddress) return;

    /* Get the deallocation stack */
    Status = NtReadVirtualMemory(ProcessHandle,
                                 &((PTEB)ThreadBasicInfo.TebBaseAddress)->
                                 DeallocationStack,
                                 &StackLocation,
                                 sizeof(PVOID),
                                 &Dummy);
    if (!NT_SUCCESS(Status) || !StackLocation) return;

    /* Free it */
    NtFreeVirtualMemory(ProcessHandle, &StackLocation, &Size, MEM_RELEASE);
}

PTEB
NTAPI
_NtCurrentTeb(VOID)
{
    /* Return the TEB */
    return NtCurrentTeb();
}

#if defined(_M_X64) || defined(_M_IX86)
#define RTL_REMOTECALL_MAX_ARG_COUNT 4
#endif

/**
 * @brief
 * Hijacks a remote thread to run at a specified location.
 *
 * @param[in] ProcessHandle
 * A handle to the target process.
 * Must have PROCESS_VM_WRITE privilege if writing context or arguments to the stack of target thread.
 *
 * @param[in] ThreadHandle
 * A handle to the target thread. Must have THREAD_GET_CONTEXT and THREAD_SET_CONTEXT privileges.
 * THREAD_SUSPEND_RESUME privilege is required if AlreadySuspended is not set.
 *
 * @param[in] CallSite
 * New location to run.
 *
 * @param[in] ArgumentCount
 * Specifies the number of entries in the Arguments array.
 * The maximum value is RTL_REMOTECALL_MAX_ARG_COUNT.
 *
 * @param[in] Arguments
 * A pointer to an array of arguments to pass.
 *
 * @param[in] PassContext
 * Passes an additional argument before Arguments, which is a pointer to a CONTEXT structure
 * captured before hijacking.
 * x64: The CONTEXT will be always written to target thread's stack regardless of PassContext.
 * 
 * @param[in] AlreadySuspended
 * Indicates the target thread is already suspended. If AlreadySuspended is not set,
 * RtlRemoteCall will suspend and resume the target thread.
 * x64: RAX will be set to STATUS_ALERTED if AlreadySuspended is set.
 *
 * @remarks
 * x64: Arguments will be written to R11-R15.
 * x86: Arguments will be written to the stack in order.
 * This function's behavior depends on platform, see the implementation and apitest for more details.
 */

NTSTATUS
NTAPI
RtlRemoteCall(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ThreadHandle,
    _In_ PVOID CallSite,
    _In_ ULONG ArgumentCount,
    _In_reads_(ArgumentCount) PULONG_PTR Arguments,
    _In_ BOOLEAN PassContext,
    _In_ BOOLEAN AlreadySuspended)
{
    /* Implemented on x64 and x86 currently */
#if !defined(_M_X64) && !defined(_M_IX86)
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
#else

    NTSTATUS Status;
    CONTEXT Context;
    ULONG ArgumentSize;
    ULONG_PTR NewStack, *ArgPtr;

    if (ArgumentCount > RTL_REMOTECALL_MAX_ARG_COUNT)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Suspend target thread if it is not suspended yet */
    if (!AlreadySuspended)
    {
        Status = NtSuspendThread(ThreadHandle, NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Get target thread context */
    Context.ContextFlags = CONTEXT_FULL;
    Status = NtGetContextThread(ThreadHandle, &Context);
    if (!NT_SUCCESS(Status))
    {
        goto _Exit;
    }

    /* x64 specific: Set RAX to STATUS_ALERTED if target thread is already suspended */
#ifdef _M_X64
    if (AlreadySuspended)
    {
        Context.Rax = STATUS_ALERTED;
    }
#endif

    /*
     * Write context to the stack.
     * x64 specific: ignore PassContext parameter, always write context to the stack.
     */
#ifdef _M_X64
    NewStack = Context.Rsp - sizeof(Context);
#else
    NewStack = Context.Esp;
    if (PassContext)
    {
        NewStack -= sizeof(Context);
#endif
        Status = NtWriteVirtualMemory(ProcessHandle,
                                      (PVOID)NewStack,
                                      &Context,
                                      sizeof(Context),
                                      NULL);
        if (!NT_SUCCESS(Status))
        {
            goto _Exit;
        }
#ifdef _M_X64
        Context.Rsp = NewStack;
#else
        Context.Esp = NewStack;
    }
#endif

    /*
     * Write parameters to target thread stack or context, and set new context.
     * If PassContext is set, the first parameter is a pointer to the context we just written.
     * x86 specific: write all parameters to the stack.
     * x64 specific: write parameters to R11-R15 registers.
     */
    ArgumentSize = ArgumentCount * sizeof(*Arguments);
#ifdef _M_X64
    ArgPtr = &Context.R11;
    if (PassContext)
    {
        *ArgPtr++ = NewStack;
    }
    if (ArgumentCount)
    {
        RtlCopyMemory(ArgPtr, Arguments, ArgumentSize);
    }
    Context.Rip = (ULONG_PTR)CallSite;
#else
    ULONG_PTR Args[RTL_REMOTECALL_MAX_ARG_COUNT + 1];

    if (PassContext)
    {
        Args[0] = NewStack;
        if (Arguments)
        {
            RtlCopyMemory(&Args[1], Arguments, ArgumentSize);
        }
        ArgumentSize += sizeof(*Arguments);
        ArgPtr = Args;
    }
    else
    {
        ArgPtr = Arguments;
    }
    if (ArgumentSize)
    {
        NewStack -= ArgumentSize;
        Status = NtWriteVirtualMemory(ProcessHandle,
                                      (PVOID)NewStack,
                                      ArgPtr,
                                      ArgumentSize,
                                      NULL);
        if (!NT_SUCCESS(Status))
        {
            goto _Exit;
        }
        Context.Esp = NewStack;
    }
    Context.Eip = (ULONG_PTR)CallSite;
#endif
    Status = NtSetContextThread(ThreadHandle, &Context);

_Exit:
    if (!AlreadySuspended)
    {
        NtResumeThread(ThreadHandle, NULL);
    }
    return Status;

#endif
}
