

#include <ddk/ntddk.h>
#include <internal/i386/segment.h>
#include <string.h>


//#define NDEBUG
#include <ntdll/ntdll.h>



static NTSTATUS
RtlpAllocateThreadStack(HANDLE ProcessHandle,
                        PULONG StackReserve,
                        PULONG StackCommit,
                        ULONG StackZeroBits,
                        PINITIAL_TEB InitialTeb)
{
    PVOID StackBase;  /* ebp-4 */


    NTSTATUS Status;  /* register */

#if 0
    Status = NtQuerySystemInformation(...);

    if (!NT_SUCCESS(Status))
        return Status;

    if (ProcessHandle == CurrentProcess)
    {

    }
    else
    {

    }


    /* ... */

    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &StackBase,
                                     ZeroBits,
                                     StackReserve,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);

    if (!NT_SUCCESS(Status))
        return Status;

    /* ... */

    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &StackBase,
                                     0,
                                     StackCommit,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);

    if (!NT_SUCCESS(Status))
        return Status;

    InitialTeb->... = StackBase;   /* + 0x0C */

    if (bProtect)
    {

        Status = NtProtectVirtualMemory(ProcessHandle,
                                        &StackBase,

        if (!NT_SUCCESS(Status))
            return Status;

        /* ... */
    }
#endif


    /* NOTE: This is a simplified implementation */

    StackBase = 0;

    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &StackBase,
                                     0,
                                     StackCommit,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);

    if (!NT_SUCCESS(Status))
         return Status;

    InitialTeb->StackBase = StackBase;
    InitialTeb->StackCommit = StackCommit;
    
    /* End of simplified implementation */

    return Status;
}


static NTSTATUS
RtlpFreeThreadStack(HANDLE ProcessHandle,
                    PINITIAL_TEB InitialTeb)
{
    NTSTATUS Status;
    ULONG RegionSize;

    RegionSize = 0;

    Status = NtFreeVirtualMemory(ProcessHandle,
                                 InitialTeb->StackBase,
                                 &RegionSize,
                                 MEM_RELEASE);

    if (NT_SUCCESS(Status))
    {
        InitialTeb->StackBase = NULL;
        /* ... */
    }

    return Status;
}


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
#if 0
    HANDLE LocalThreadHandle;
    CLIENT_ID LocalClientId;
    OBJECT_ATTRIBUTES ObjectAttributes;
    INITIAL_TEB InitialTeb;
    CONTEXT Context;
    NTSTATUS Status;

DPRINT("Checkpoint\n");
    /* create thread stack */
    Status = RtlpAllocateThreadStack(ProcessHandle,
                                     StackReserved,
                                     StackCommit,
                                     StackZeroBits,
                                     &InitialTeb);

    if (!NT_SUCCESS(Status))
        return Status;

DPRINT("Checkpoint\n");

    /* initialize thread context */
    RtlInitializeContext (ProcessHandle,
                          &Context,
                          DebugPort,
                          StartAddress,
                          &InitialTeb);
DPRINT("Checkpoint\n");

    /* initalize object attributes */
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.ObjectName = NULL;
    ObjectAttributes.Attributes = 0;
    ObjectAttributes.SecurityDescriptor = SecurityDescriptor;
    ObjectAttributes.SecurityQualityOfService = NULL;

    Status = NtCreateThread (&LocalThreadHandle,
                             0x1F03FF,                  /* ??? */
                             &ObjectAttributes,
                             ProcessHandle,
                             &LocalClientId,
                             &Context,
                             &InitialTeb,
                             CreateSuspended);
DPRINT("Checkpoint\n");

    if (!NT_SUCCESS(Status))
    {
        /* free thread stack */
        RtlpFreeThreadStack (ProcessHandle,
                             &InitialTeb);

        return Status;
    }

#endif

/* Begin of test code */

   HANDLE LocalThreadHandle;
   CLIENT_ID LocalClientId;
   OBJECT_ATTRIBUTES ObjectAttributes;
   INITIAL_TEB InitialTeb;
   CONTEXT ThreadContext;
   PVOID BaseAddress;
   NTSTATUS Status;

   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = NULL;
//   ObjectAttributes.Attributes = 0;
   ObjectAttributes.Attributes = OBJ_INHERIT;
   ObjectAttributes.SecurityDescriptor = SecurityDescriptor;
   ObjectAttributes.SecurityQualityOfService = NULL;


   if (*StackCommit < 4096)
      *StackCommit = 4096;

   BaseAddress = 0;
   ZwAllocateVirtualMemory(ProcessHandle,
			   &BaseAddress,
			   0,
                           StackCommit,
			   MEM_COMMIT,
			   PAGE_READWRITE);


   memset(&ThreadContext,0,sizeof(CONTEXT));
   ThreadContext.Eip = (LONG)StartAddress;
   ThreadContext.SegGs = USER_DS;
   ThreadContext.SegFs = USER_DS;
   ThreadContext.SegEs = USER_DS;
   ThreadContext.SegDs = USER_DS;
   ThreadContext.SegCs = USER_CS;
   ThreadContext.SegSs = USER_DS;        
   ThreadContext.Esp = (ULONG)(BaseAddress + *StackCommit);
   ThreadContext.EFlags = (1<<1) + (1<<9);


   Status = NtCreateThread(&LocalThreadHandle,
                           THREAD_ALL_ACCESS,
                           &ObjectAttributes,
                           ProcessHandle,
                           &LocalClientId,
                           &ThreadContext,
                           &InitialTeb,
                           CreateSuspended);

//   if ( lpThreadId != NULL )
//     memcpy(lpThreadId, &ClientId.UniqueThread,sizeof(ULONG));

/* End of test code */

    DPRINT("Checkpoint\n");

    /* return thread handle */
    if (ThreadHandle)
        *ThreadHandle = LocalThreadHandle;

    /* return client id */
    if (ClientId)
    {
        ClientId->UniqueProcess = LocalClientId.UniqueProcess;
        ClientId->UniqueThread  = LocalClientId.UniqueThread;
    }

    DPRINT("Checkpoint\n");

    return Status;
}


NTSTATUS STDCALL
RtlInitializeContext(HANDLE ProcessHandle,
                     PCONTEXT Context,
                     PVOID Parameter,
                     PTHREAD_START_ROUTINE StartAddress,
                     PINITIAL_TEB InitialTeb)
{
    NTSTATUS Status;

    /* NOTE: This is a simplified implementation */

    memset (Context, 0, sizeof(CONTEXT));

/* #if __X86__ */
    Context->Eip    = (ULONG)StartAddress;
    Context->SegGs  = USER_DS;
    Context->SegFs  = USER_DS;   /* USER_FS */
    Context->SegEs  = USER_DS;
    Context->SegDs  = USER_DS;
    Context->SegCs  = USER_CS;
    Context->SegSs  = USER_DS;
    Context->Esp    = (ULONG)InitialTeb->StackBase +
                      (DWORD)InitialTeb->StackCommit - 8;
    Context->EFlags = (1<<1)+(1<<9);


    /* copy Parameter to thread stack */
    *((PULONG)(InitialTeb->StackBase + (DWORD)InitialTeb->StackCommit - 4))
        = (DWORD)Parameter;


/* #endif */

    Status = STATUS_SUCCESS;

    /* End of simplified implementation */

    return Status;
}

