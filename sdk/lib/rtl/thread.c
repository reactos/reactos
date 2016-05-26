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

NTSTATUS
NTAPI
RtlRemoteCall(IN HANDLE Process,
              IN HANDLE Thread,
              IN PVOID CallSite,
              IN ULONG ArgumentCount,
              IN PULONG Arguments,
              IN BOOLEAN PassContext,
              IN BOOLEAN AlreadySuspended)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
