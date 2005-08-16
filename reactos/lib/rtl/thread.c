/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Rtl user thread functions
 * FILE:              lib/ntdll/rtl/thread.c
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
STDCALL
RtlpCreateUserStack(HANDLE hProcess,
                    ULONG StackReserve,
                    ULONG StackCommit,
                    ULONG StackZeroBits,
                    PINITIAL_TEB InitialTeb)
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION SystemBasicInfo;
    PIMAGE_NT_HEADERS Headers;
    ULONG_PTR Stack = 0;
    BOOLEAN UseGuard = FALSE;
    
    DPRINT("RtlpCreateUserStack\n");
    
    /* Get some memory information */
    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &SystemBasicInfo,
                                      sizeof(SYSTEM_BASIC_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failure to query system info\n");
        return Status;
    }
    
    /* Use the Image Settings if we are dealing with the current Process */
    if (hProcess == NtCurrentProcess())
    {
        /* Get the Image Headers */
        Headers = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);
        DPRINT("Headers: %p\n", Headers);
        
        /* If we didn't get the parameters, find them ourselves */
        StackReserve = (StackReserve) ? 
                        StackReserve : Headers->OptionalHeader.SizeOfStackReserve;
        StackCommit = (StackCommit) ? 
                       StackCommit : Headers->OptionalHeader.SizeOfStackCommit;
    }
    else
    {
        /* Use the System Settings if needed */
        StackReserve = (StackReserve) ? StackReserve :
                                        SystemBasicInfo.AllocationGranularity;
        StackCommit = (StackCommit) ? StackCommit : SystemBasicInfo.PageSize;
    }
    
    /* Align everything to Page Size */
    StackReserve = ROUND_UP(StackReserve, SystemBasicInfo.AllocationGranularity);
    StackCommit = ROUND_UP(StackCommit, SystemBasicInfo.PageSize);
    #if 1 // FIXME: Remove once Guard Page support is here
    StackCommit = StackReserve;
    #endif
    DPRINT("StackReserve: %lx, StackCommit: %lx\n", StackReserve, StackCommit);
    
    /* Reserve memory for the stack */
    Status = ZwAllocateVirtualMemory(hProcess,
                                     (PVOID*)&Stack,
                                     StackZeroBits,
                                     &StackReserve,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failure to reserve stack\n");
        return Status;
    }
    
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
        Stack -= SystemBasicInfo.PageSize;
        StackCommit += SystemBasicInfo.PageSize;
        UseGuard = TRUE;
    }
    
    DPRINT("AllocatedBase: %p, StackBase: %p, Stack: %p, StackCommit: %lx\n",
            InitialTeb->AllocatedStackBase, InitialTeb->StackBase, Stack,
            StackCommit);
    
    /* Allocate memory for the stack */
    Status = ZwAllocateVirtualMemory(hProcess,
                                     (PVOID*)&Stack,
                                     0,
                                     &StackCommit,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failure to allocate stack\n");
        return Status;
    }
    
    /* Now set the current Stack Limit */
    InitialTeb->StackLimit = (PVOID)Stack;
    DPRINT("StackLimit: %p\n", Stack);
    
    /* Create a guard page */
    if (UseGuard)
    {
        ULONG GuardPageSize = SystemBasicInfo.PageSize;
        ULONG Dummy;
        
        /* Attempt maximum space possible */        
        Status = ZwProtectVirtualMemory(hProcess,
                                        (PVOID*)&Stack,
                                        &GuardPageSize,
                                        PAGE_GUARD | PAGE_READWRITE,
                                        &Dummy);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failure to create guard page\n");
            return Status;
        }
        
        /* Update the Stack Limit keeping in mind the Guard Page */
        InitialTeb->StackLimit = (PVOID)((ULONG_PTR)InitialTeb->StackLimit -
                                         GuardPageSize);
        DPRINT1("StackLimit: %p\n", Stack);   
    }
    
    /* We are done! */
    return Status;
}

NTSTATUS
STDCALL
RtlpFreeUserStack(HANDLE hProcess,
                  PINITIAL_TEB InitialTeb)
{
    ULONG Dummy = 0;
    
    /* Free the Stack */
    return ZwFreeVirtualMemory(hProcess,
                               &InitialTeb->AllocatedStackBase,
                               &Dummy,
                               MEM_RELEASE);
}
    
/* FUNCTIONS ***************************************************************/

/*
 @implemented
*/
NTSTATUS 
STDCALL 
RtlCreateUserThread(HANDLE ProcessHandle,
                    PSECURITY_DESCRIPTOR SecurityDescriptor,
                    BOOLEAN CreateSuspended,
                    LONG StackZeroBits,
                    ULONG StackReserve,
                    ULONG StackCommit,
                    PTHREAD_START_ROUTINE StartAddress,
                    PVOID Parameter,
                    PHANDLE ThreadHandle,
                    PCLIENT_ID ClientId)
{
    NTSTATUS Status;
    HANDLE Handle;
    CLIENT_ID ThreadCid;
    INITIAL_TEB InitialTeb;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CONTEXT Context;
    
    DPRINT1("RtlCreateUserThread: (hProcess: %lx, Suspended: %lx,"
            "ZeroBits: %lx, StackReserve: %lx, StackCommit: %lx,"
            "StartAddress: %p, Parameter: %lx)\n", ProcessHandle,
            CreateSuspended, StackZeroBits, StackReserve, StackCommit,
            StartAddress, Parameter);
    
    /* First, we'll create the Stack */
    Status = RtlpCreateUserStack(ProcessHandle,
                                 StackReserve,
                                 StackCommit,
                                 StackZeroBits,
                                 &InitialTeb);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failure to create User Stack\n");
        return Status;
    }
    
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
        DPRINT1("Failure to create Thread\n");
        
        /* Free the stack */
        RtlpFreeUserStack(ProcessHandle, &InitialTeb);
    }
    else
    {
        DPRINT("Thread created: %lx\n", Handle);
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
STDCALL
RtlInitializeContext(IN HANDLE ProcessHandle,
                     OUT PCONTEXT ThreadContext,
                     IN PVOID ThreadStartParam  OPTIONAL,
                     IN PTHREAD_START_ROUTINE ThreadStartAddress,
                     IN PINITIAL_TEB InitialTeb)
{
    DPRINT("RtlInitializeContext: (hProcess: %lx, ThreadContext: %p, Teb: %p\n",
            ProcessHandle, ThreadContext, InitialTeb);

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
    ThreadContext->SegFs = TEB_SELECTOR;
    ThreadContext->SegEs = USER_DS;
    ThreadContext->SegDs = USER_DS;
    ThreadContext->SegCs = USER_CS;
    ThreadContext->SegSs = USER_DS;
    
    /* Enable Interrupts */
    ThreadContext->EFlags = 0x200; /*X86_EFLAGS_IF */
    
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
 @implemented
*/
VOID 
STDCALL 
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
    if (!NT_SUCCESS(Status) || !ThreadBasicInfo.TebBaseAddress)
    {
        DPRINT1("Could not query info, or TEB is NULL\n");
        return;
    }
    
    /* Get the deallocation stack */
    Status = NtReadVirtualMemory(ProcessHandle,
                                 &((PTEB)ThreadBasicInfo.TebBaseAddress)->
                                 DeallocationStack,
                                 &StackLocation,
                                 sizeof(PVOID),
                                 &Dummy);
    if (!NT_SUCCESS(Status) || !StackLocation)
    {
        DPRINT1("Could not read Deallocation Base\n");
        return;
    }
    
    /* Free it */
    NtFreeVirtualMemory(ProcessHandle, &StackLocation, &Size, MEM_RELEASE);
}

/* EOF */
