/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Rtl user thread functions
 * FILE:              lib/ntdll/rtl/thread.c
 * PROGRAMER:         Eric Kohl
 * REVISION HISTORY:
 *                    09/07/99: Created
 *                    09/10/99: Cleanup and full stack support.
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/i386/segment.h>
#include <string.h>

#define NDEBUG
#include <ntdll/ntdll.h>


/* FUNCTIONS ***************************************************************/


NTSTATUS STDCALL
RtlCreateUserThread(HANDLE ProcessHandle,
                    SECURITY_DESCRIPTOR *SecurityDescriptor,
                    BOOLEAN CreateSuspended,
                    LONG StackZeroBits,
                    PULONG StackReserved,
                    PULONG StackCommit,
                    PTHREAD_START_ROUTINE StartAddress,
                    PVOID   Parameter,
                    PHANDLE ThreadHandle,
                    PCLIENT_ID ClientId)
{
    HANDLE LocalThreadHandle;
    CLIENT_ID LocalClientId;
    OBJECT_ATTRIBUTES ObjectAttributes;
    INITIAL_TEB InitialTeb;
    CONTEXT ThreadContext;
    ULONG ReservedSize;
    ULONG CommitSize;
    ULONG GuardSize;
    NTSTATUS Status;

    /* initialize initial teb */
    if ((StackCommit != NULL) && (*StackCommit > PAGESIZE))
       CommitSize = *StackCommit;
    else
       CommitSize = PAGESIZE;

    if ((StackReserved != NULL) && (*StackReserved > 0x100000))
       ReservedSize = *StackReserved;
    else
       ReservedSize = 0x100000; /* 1MByte */

    GuardSize = PAGESIZE;

    /* Reserve stack */
    InitialTeb.StackReserved = NULL;
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &InitialTeb.StackReserved,
                                     0,
                                     &ReservedSize,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Error reserving stack space!\n");
        return Status;
    }

    DPRINT("StackReserved: %p ReservedSize: 0x%lx\n",
           InitialTeb.StackReserved, ReservedSize);

    InitialTeb.StackBase = (PVOID)(InitialTeb.StackReserved + ReservedSize);
    InitialTeb.StackCommit = (PVOID)(InitialTeb.StackBase - CommitSize);
    InitialTeb.StackLimit = (PVOID)(InitialTeb.StackCommit - PAGESIZE);
    InitialTeb.StackCommitMax = (PVOID)(InitialTeb.StackReserved + PAGESIZE);

    DPRINT("StackBase: %p\n",
           InitialTeb.StackBase);

    /* Commit stack page */
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &InitialTeb.StackCommit,
                                     0,
                                     &CommitSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Error committing stack page!\n");
        return Status;
    }

    DPRINT("StackCommit: %p CommitSize: 0x%lx\n",
           InitialTeb.StackCommit, CommitSize);

    /* Commit guard page */
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &InitialTeb.StackLimit,
                                     0,
                                     &GuardSize,
                                     MEM_COMMIT,
                                     PAGE_GUARD);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Error committing guard page!\n");
        return Status;
    }

    DPRINT("StackLimit: %p GuardSize: 0x%lx\n",
           InitialTeb.StackLimit, GuardSize);

    /* initialize thread context */
    RtlInitializeContext (ProcessHandle,
                          &ThreadContext,
                          Parameter,
                          StartAddress,
                          &InitialTeb);

    /* create the thread */
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.ObjectName = NULL;
//    ObjectAttributes.Attributes = 0;
    ObjectAttributes.Attributes = OBJ_INHERIT;
    ObjectAttributes.SecurityDescriptor = SecurityDescriptor;
    ObjectAttributes.SecurityQualityOfService = NULL;

    Status = NtCreateThread(&LocalThreadHandle,
                            THREAD_ALL_ACCESS,
                            &ObjectAttributes,
                            ProcessHandle,
                            &LocalClientId,
                            &ThreadContext,
                            &InitialTeb,
                            CreateSuspended);

    if (!NT_SUCCESS(Status))
    {
        ULONG RegionSize = 0;

        /* release the stack space */
        NtFreeVirtualMemory(ProcessHandle,
                            InitialTeb.StackReserved,
                            &RegionSize,
                            MEM_RELEASE);

        return Status;
    }

    /* return committed stack size */
    if (StackCommit)
        *StackCommit = CommitSize;

    /* return reserved stack size */
    if (StackCommit)
        *StackCommit = ReservedSize;

    /* return thread handle */
    if (ThreadHandle)
        *ThreadHandle = LocalThreadHandle;

    /* return client id */
    if (ClientId)
    {
        ClientId->UniqueProcess = LocalClientId.UniqueProcess;
        ClientId->UniqueThread  = LocalClientId.UniqueThread;
    }

    return Status;
}


NTSTATUS STDCALL
RtlInitializeContext(HANDLE ProcessHandle,
                     PCONTEXT Context,
                     PVOID Parameter,
                     PTHREAD_START_ROUTINE StartAddress,
                     PINITIAL_TEB InitialTeb)
{
    ULONG Buffer[2];
    ULONG BytesWritten;
    NTSTATUS Status;

    memset (Context, 0, sizeof(CONTEXT));

    Context->Eip = (LONG)StartAddress;
    Context->SegGs = USER_DS;
    Context->SegFs = USER_DS;
    Context->SegEs = USER_DS;
    Context->SegDs = USER_DS;
    Context->SegCs = USER_CS;
    Context->SegSs = USER_DS;        
    Context->Esp = (ULONG)InitialTeb->StackBase - 8;
    Context->EFlags = (1<<1) + (1<<9);

    /* prepare the thread stack for execution */
    if (ProcessHandle == NtCurrentProcess())
    {
        *((PULONG)(InitialTeb->StackBase - 4)) = (ULONG)Parameter;
        *((PULONG)(InitialTeb->StackBase - 8)) = 0xdeadbeef;
    }
    else
    {
        Buffer[0] = (ULONG)Parameter;
        Buffer[1] = 0xdeadbeef;
        
        Status = NtWriteVirtualMemory(ProcessHandle,
                                      (PVOID)(InitialTeb->StackBase - 4),
                                      Buffer,
                                      2 * sizeof(ULONG),
                                      &BytesWritten);
        return Status;
    }

    return STATUS_SUCCESS;
}

/* EOF */
