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
  ULONG OldPageProtection;
  NTSTATUS Status;

  /* initialize initial teb */
  if ((StackReserve != NULL) && (*StackReserve > 0x100000))
    InitialTeb.StackReserve = *StackReserve;
  else
    InitialTeb.StackReserve = 0x100000; /* 1MByte */

  /* FIXME: use correct commit size */
#if 0
  if ((StackCommit != NULL) && (*StackCommit > PAGE_SIZE))
    InitialTeb.StackCommit = *StackCommit;
  else
    InitialTeb.StackCommit = PAGE_SIZE;
#endif
  InitialTeb.StackCommit = InitialTeb.StackReserve - PAGE_SIZE;

  /* add size of guard page */
  InitialTeb.StackCommit += PAGE_SIZE;

  /* Reserve stack */
  InitialTeb.StackAllocate = NULL;
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   &InitialTeb.StackAllocate,
				   0,
				   &InitialTeb.StackReserve,
				   MEM_RESERVE,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Error reserving stack space!\n");
      return(Status);
    }

  DPRINT("StackAllocate: %p ReserveSize: 0x%lX\n",
	 InitialTeb.StackAllocate, InitialTeb.StackReserve);

  InitialTeb.StackBase = (PVOID)((ULONG)InitialTeb.StackAllocate + InitialTeb.StackReserve);
  InitialTeb.StackLimit = (PVOID)((ULONG)InitialTeb.StackBase - InitialTeb.StackCommit);

  DPRINT("StackBase: %p  StackCommit: 0x%lX\n",
	 InitialTeb.StackBase, InitialTeb.StackCommit);

  /* Commit stack */
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   &InitialTeb.StackLimit,
				   0,
				   &InitialTeb.StackCommit,
				   MEM_COMMIT,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      /* release the stack space */
      NtFreeVirtualMemory(ProcessHandle,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);

      DPRINT("Error comitting stack page!\n");
      return(Status);
    }

  DPRINT("StackLimit: %p\nStackCommit: 0x%lX\n",
         InitialTeb.StackLimit,
         InitialTeb.StackCommit);

  /* Protect guard page */
  Status = NtProtectVirtualMemory(ProcessHandle,
				  InitialTeb.StackLimit,
				  PAGE_SIZE,
				  PAGE_GUARD | PAGE_READWRITE,
				  &OldPageProtection);
  if (!NT_SUCCESS(Status))
    {
      /* release the stack space */
      NtFreeVirtualMemory(ProcessHandle,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);

      DPRINT("Error protecting guard page!\n");
      return(Status);
    }

  /* initialize thread context */
  RtlInitializeContext(ProcessHandle,
		       &ThreadContext,
		       Parameter,
		       StartAddress,
		       &InitialTeb);

  /* create the thread */
  ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
  ObjectAttributes.RootDirectory = NULL;
  ObjectAttributes.ObjectName = NULL;
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
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);

      DPRINT("Error creating thread!\n");
      return(Status);
    }

  /* return committed stack size */
  if (StackCommit)
    *StackCommit = InitialTeb.StackCommit;

  /* return reserved stack size */
  if (StackReserve)
    *StackReserve = InitialTeb.StackReserve;

  /* return thread handle */
  if (ThreadHandle)
    *ThreadHandle = LocalThreadHandle;

  /* return client id */
  if (ClientId)
    {
      ClientId->UniqueProcess = LocalClientId.UniqueProcess;
      ClientId->UniqueThread  = LocalClientId.UniqueThread;
    }

  return(STATUS_SUCCESS);
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
                                      (PVOID)((ULONG)InitialTeb->StackBase - 8),
                                      Buffer,
                                      2 * sizeof(ULONG),
                                      &BytesWritten);
        return Status;
    }

    return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlFreeUserThreadStack(HANDLE ProcessHandle,
		       HANDLE ThreadHandle)
{
  THREAD_BASIC_INFORMATION ThreadInfo;
  NTSTATUS Status;
  ULONG BytesRead;
  ULONG RegionSize;
  PVOID StackBase;
  PTEB Teb;

  Status = NtQueryInformationThread(ThreadHandle,
				    ThreadBasicInformation,
				    &ThreadInfo,
				    sizeof(THREAD_BASIC_INFORMATION),
				    NULL);
  if (!NT_SUCCESS(Status))
    return(Status);

  if (ThreadInfo.TebBaseAddress == NULL)
    return(Status);

  Teb = (PTEB)ThreadInfo.TebBaseAddress;
  Status = NtReadVirtualMemory(ProcessHandle,
			       &Teb->DeallocationStack,
			       &StackBase,
			       sizeof(PVOID),
			       &BytesRead);
  if (!NT_SUCCESS(Status))
    return(Status);

  if (StackBase == NULL)
    return(Status);

  RegionSize = 0;
  Status = NtFreeVirtualMemory(ProcessHandle,
			       StackBase,
			       &RegionSize,
			       MEM_RELEASE);
  return(Status);
}

/* EOF */
