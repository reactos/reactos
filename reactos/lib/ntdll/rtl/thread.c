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
#include <napi/i386/segment.h>
#include <napi/teb.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <ntdll/ntdll.h>


/* FUNCTIONS ***************************************************************/


NTSTATUS STDCALL
RtlCreateUserThread(HANDLE ProcessHandle,
                    PSECURITY_DESCRIPTOR SecurityDescriptor,
                    BOOLEAN CreateSuspended,
                    LONG StackZeroBits,
                    PULONG StackReserve,
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
    ULONG ReserveSize;
    ULONG CommitSize;
    ULONG GuardSize;
    ULONG RegionSize;
    NTSTATUS Status;

    /* initialize initial teb */
    if ((StackCommit != NULL) && (*StackCommit > PAGESIZE))
       CommitSize = *StackCommit;
    else
       CommitSize = PAGESIZE;

    if ((StackReserve != NULL) && (*StackReserve > 0x100000))
       ReserveSize = *StackReserve;
    else
       ReserveSize = 0x100000; /* 1MByte */

    GuardSize = PAGESIZE;

    RegionSize = 0;

    /* Reserve stack */
    InitialTeb.StackReserve = NULL;
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &InitialTeb.StackReserve,
                                     0,
                                     &ReserveSize,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Error reserving stack space!\n");
        return Status;
    }

    DPRINT("StackReserved: %p ReservedSize: 0x%lx\n",
           InitialTeb.StackReserve, ReserveSize);

    InitialTeb.StackBase = (PVOID)(InitialTeb.StackReserve + ReserveSize);
    InitialTeb.StackCommit = (PVOID)(InitialTeb.StackBase - CommitSize);
    InitialTeb.StackLimit = (PVOID)(InitialTeb.StackCommit - PAGESIZE);
    InitialTeb.StackCommitMax = (PVOID)(InitialTeb.StackReserve + PAGESIZE);

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
        /* release the stack space */
        NtFreeVirtualMemory(ProcessHandle,
                            InitialTeb.StackReserve,
                            &RegionSize,
                            MEM_RELEASE);

        DPRINT("Error comitting stack page!\n");
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
        /* release the stack space */
        NtFreeVirtualMemory(ProcessHandle,
                            InitialTeb.StackReserve,
                            &RegionSize,
                            MEM_RELEASE);

        DPRINT("Error comitting guard page!\n");
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
        /* release the stack space */
        NtFreeVirtualMemory(ProcessHandle,
                            InitialTeb.StackReserve,
                            &RegionSize,
                            MEM_RELEASE);

        DPRINT("Error creating thread!\n");
        return Status;
    }

    /* return committed stack size */
    if (StackCommit)
        *StackCommit = CommitSize;

    /* return reserved stack size */
    if (StackReserve)
        *StackReserve = ReserveSize;

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
    Context->SegFs = TEB_SELECTOR;
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


NTSTATUS STDCALL
RtlFreeUserThreadStack (HANDLE	ProcessHandle, HANDLE	ThreadHandle)
{
	THREAD_BASIC_INFORMATION ThreadInfo;
	NTSTATUS Status;
	ULONG BytesRead;
	ULONG RegionSize;
	PVOID StackBase;
	PNT_TEB Teb;

	Status = NtQueryInformationThread (ThreadHandle,
	                                   ThreadBasicInformation,
	                                   &ThreadInfo,
	                                   sizeof(THREAD_BASIC_INFORMATION),
	                                   NULL);
	if (!NT_SUCCESS(Status))
		return Status;

	if (ThreadInfo.TebBaseAddress == NULL)
		return Status;

	Teb = (PNT_TEB)ThreadInfo.TebBaseAddress;
	Status = NtReadVirtualMemory (ProcessHandle,
	                              &Teb->DeallocationStack,
	                              &StackBase,
	                              sizeof(PVOID),
	                              &BytesRead);
	if (!NT_SUCCESS(Status))
		return Status;

	if (StackBase == NULL)
		return Status;

	RegionSize = 0;
	Status = NtFreeVirtualMemory (ProcessHandle,
	                              StackBase,
	                              &RegionSize,
	                              MEM_RELEASE);

	return Status;
}

/* EOF */
