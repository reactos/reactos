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
#include "i386/ketypes.h"

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *******************************************************/

NTSTATUS
NTAPI
RtlpCreateUserStack(IN HANDLE hProcess,
                    IN SIZE_T StackReserve OPTIONAL,
                    IN SIZE_T StackCommit OPTIONAL,
                    IN ULONG StackZeroBits OPTIONAL,
                    OUT PINITIAL_TEB InitialTeb)
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION SystemBasicInfo;
    PIMAGE_NT_HEADERS Headers;
    ULONG_PTR Stack = 0;
    BOOLEAN UseGuard = FALSE;
    ULONG Dummy, GuardPageSize;

    /* Get some memory information */
    Status = ZwQuerySystemInformation(SystemBasicInformation,
                                      &SystemBasicInfo,
                                      sizeof(SYSTEM_BASIC_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Use the Image Settings if we are dealing with the current Process */
    if (hProcess == NtCurrentProcess())
    {
        /* Get the Image Headers */
        Headers = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);

        /* If we didn't get the parameters, find them ourselves */
        if (!StackReserve) StackReserve = Headers->OptionalHeader.
                                          SizeOfStackReserve;
        if (!StackCommit) StackCommit = Headers->OptionalHeader.
                                        SizeOfStackCommit;
    }
    else
    {
        /* Use the System Settings if needed */
        if (!StackReserve) StackReserve = SystemBasicInfo.AllocationGranularity;
        if (!StackCommit) StackCommit = SystemBasicInfo.PageSize;
    }

    /* Align everything to Page Size */
    StackReserve = ROUND_UP(StackReserve, SystemBasicInfo.AllocationGranularity);
    StackCommit = ROUND_UP(StackCommit, SystemBasicInfo.PageSize);

    // FIXME: Remove once Guard Page support is here
    #if 1
    StackCommit = StackReserve;
    #endif

    /* Reserve memory for the stack */
    Status = ZwAllocateVirtualMemory(hProcess,
                                     (PVOID*)&Stack,
                                     StackZeroBits,
                                     &StackReserve,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Now set up some basic Initial TEB Parameters */
    InitialTeb->PreviousStackBase = NULL;
    InitialTeb->PreviousStackLimit = NULL;
    InitialTeb->AllocatedStackBase = (PVOID)Stack;
    InitialTeb->StackBase = (PVOID)(Stack + StackReserve);

    /* Update the Stack Position */
    Stack += StackReserve - StackCommit;

    /* Check if we will need a guard page */
    if (StackReserve > StackCommit)
    {
        /* Remove a page to set as guard page */
        Stack -= SystemBasicInfo.PageSize;
        StackCommit += SystemBasicInfo.PageSize;
        UseGuard = TRUE;
    }

    /* Allocate memory for the stack */
    Status = ZwAllocateVirtualMemory(hProcess,
                                     (PVOID*)&Stack,
                                     0,
                                     &StackCommit,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Now set the current Stack Limit */
    InitialTeb->StackLimit = (PVOID)Stack;

    /* Create a guard page */
    if (UseGuard)
    {
        /* Attempt maximum space possible */
        GuardPageSize = SystemBasicInfo.PageSize;
        Status = ZwProtectVirtualMemory(hProcess,
                                        (PVOID*)&Stack,
                                        &GuardPageSize,
                                        PAGE_GUARD | PAGE_READWRITE,
                                        &Dummy);
        if (!NT_SUCCESS(Status)) return Status;

        /* Update the Stack Limit keeping in mind the Guard Page */
        InitialTeb->StackLimit = (PVOID)((ULONG_PTR)InitialTeb->StackLimit -
                                         GuardPageSize);
    }

    /* We are done! */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlpFreeUserStack(IN HANDLE Process,
                  IN PINITIAL_TEB InitialTeb)
{
    ULONG Dummy = 0;
    NTSTATUS Status;

    /* Free the Stack */
    Status = ZwFreeVirtualMemory(Process,
                                 &InitialTeb->AllocatedStackBase,
                                 &Dummy,
                                 MEM_RELEASE);

    /* Clear the initial TEB */
    RtlZeroMemory(InitialTeb, sizeof(INITIAL_TEB));
    return Status;
}

/* FUNCTIONS ***************************************************************/

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
        if (ThreadHandle) *ThreadHandle = Handle;
        if (ClientId) *ClientId = ThreadCid;
    }

    /* Return success or the previous failure */
    return Status;
}

/*
 * FIXME: Should go in /i386
 @implemented
*/
VOID
NTAPI
RtlInitializeContext(IN HANDLE ProcessHandle,
                     OUT PCONTEXT ThreadContext,
                     IN PVOID ThreadStartParam  OPTIONAL,
                     IN PTHREAD_START_ROUTINE ThreadStartAddress,
                     IN PINITIAL_TEB InitialTeb)
{
    /*
     * Set the Initial Registers
     * This is based on NT's default values -- crazy apps might expect this...
     */
    ThreadContext->Ebp = 0;
    ThreadContext->Eax = 0;
    ThreadContext->Ebx = 1;
    ThreadContext->Ecx = 2;
    ThreadContext->Edx = 3;
    ThreadContext->Esi = 4;
    ThreadContext->Edi = 5;

    /* Set the Selectors */
    ThreadContext->SegGs = 0;
    ThreadContext->SegFs = KGDT_R3_TEB;
    ThreadContext->SegEs = KGDT_R3_DATA;
    ThreadContext->SegDs = KGDT_R3_DATA;
    ThreadContext->SegSs = KGDT_R3_DATA;
    ThreadContext->SegCs = KGDT_R3_CODE;

    /* Enable Interrupts */
    ThreadContext->EFlags = EFLAGS_INTERRUPT_MASK;

    /* Settings passed */
    ThreadContext->Eip = (ULONG)ThreadStartAddress;
    ThreadContext->Esp = (ULONG)InitialTeb;

    /* Only the basic Context is initialized */
    ThreadContext->ContextFlags = CONTEXT_CONTROL |
                                  CONTEXT_INTEGER |
                                  CONTEXT_SEGMENTS;

    /* Set up ESP to the right value */
    ThreadContext->Esp -= sizeof(PVOID);
    ZwWriteVirtualMemory(ProcessHandle,
                         (PVOID)ThreadContext->Esp,
                         (PVOID)&ThreadStartParam,
                         sizeof(PVOID),
                         NULL);

    /* Push it down one more notch for RETEIP */
    ThreadContext->Esp -= sizeof(PVOID);
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
    ULONG Dummy, Size = 0;
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

/* EOF */
