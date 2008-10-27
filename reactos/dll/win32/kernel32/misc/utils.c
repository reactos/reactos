/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/utils.c
 * PURPOSE:         Utility and Support Functions
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ****************************************************************/

#include <k32.h>
#ifdef _M_IX86
#include "i386/ketypes.h"
#elif defined _M_AMD64
#include "amd64/ketypes.h"
#endif

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

PRTL_CONVERT_STRING Basep8BitStringToUnicodeString;

/* FUNCTIONS ****************************************************************/

/*
 * Converts an ANSI or OEM String to the specified Unicode String
 */
NTSTATUS
STDCALL
Basep8BitStringToLiveUnicodeString(OUT PUNICODE_STRING UnicodeString,
                                   IN LPCSTR String)
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    
    DPRINT("Basep8BitStringToLiveUnicodeString\n");
    
    /* Create the ANSI String */
    RtlInitAnsiString(&AnsiString, String);
    
    /* Convert from OEM or ANSI */
    Status = Basep8BitStringToUnicodeString(UnicodeString, &AnsiString, FALSE);
    
    /* Return Status */
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
    }
    return Status;
}


/*
 * Converts an ANSI or OEM String to the TEB StaticUnicodeString
 */
PUNICODE_STRING
STDCALL
Basep8BitStringToCachedUnicodeString(IN LPCSTR String)
{
    PUNICODE_STRING StaticString = &NtCurrentTeb()->StaticUnicodeString;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    
    DPRINT("Basep8BitStringToCachedUnicodeString\n");
    
    /* Initialize an ANSI String */
    RtlInitAnsiString(&AnsiString, String);
    
    /* Convert it */
    Status = Basep8BitStringToUnicodeString(StaticString, &AnsiString, FALSE);
    
    /* Handle failure */
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return NULL;
    }
    
    /* Return pointer to the string */
    return StaticString;
}

NTSTATUS
STDCALL
Basep8BitStringToHeapUnicodeString(OUT PUNICODE_STRING UnicodeString,
                                   IN LPCSTR String)
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    
    DPRINT("Basep8BitStringToCachedUnicodeString\n");
    
    /* Initialize an ANSI String */
    RtlInitAnsiString(&AnsiString, String);
    
    /* Convert it */
    Status = Basep8BitStringToUnicodeString(UnicodeString, &AnsiString, TRUE);
    
    /* Handle failure */
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
    }
    /* Return Status */
    return Status;
}

/*
 * Allocates space from the Heap and converts an Ansi String into it
 */
VOID
STDCALL
BasepAnsiStringToHeapUnicodeString(IN LPCSTR AnsiString,
                                   OUT LPWSTR* UnicodeString)
{
    ANSI_STRING AnsiTemp;
    UNICODE_STRING UnicodeTemp;
    
    DPRINT("BasepAnsiStringToHeapUnicodeString\n");
    
    /* First create the ANSI_STRING */
    RtlInitAnsiString(&AnsiTemp, AnsiString);
    
    if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeTemp,
                                                &AnsiTemp,
                                                TRUE)))
    {
        *UnicodeString = UnicodeTemp.Buffer;
    }
    else
    {
        *UnicodeString = NULL;
    }
}

/*
 * Converts lpSecurityAttributes + Object Name into ObjectAttributes.
 */
POBJECT_ATTRIBUTES
STDCALL
BasepConvertObjectAttributes(OUT POBJECT_ATTRIBUTES ObjectAttributes,
                             IN PSECURITY_ATTRIBUTES SecurityAttributes OPTIONAL,
                             IN PUNICODE_STRING ObjectName)
{
    ULONG Attributes = 0;
    HANDLE RootDirectory = 0;
    PVOID SecurityDescriptor = NULL;
    BOOLEAN NeedOba = FALSE;
    
    DPRINT("BasepConvertObjectAttributes. Security: %p, Name: %p\n",
            SecurityAttributes, ObjectName);
    
    /* Get the attributes if present */
    if (SecurityAttributes)
    {
        Attributes = SecurityAttributes->bInheritHandle ? OBJ_INHERIT : 0;
        SecurityDescriptor = SecurityAttributes->lpSecurityDescriptor;
        NeedOba = TRUE;
    }
    
    if (ObjectName)
    {
        Attributes |= OBJ_OPENIF;
        RootDirectory = hBaseDir;
        NeedOba = TRUE;
    }
    
    DPRINT("Attributes: %lx, RootDirectory: %lx, SecurityDescriptor: %p\n",
            Attributes, RootDirectory, SecurityDescriptor);
    
    /* Create the Object Attributes */
    if (NeedOba)
    {
        InitializeObjectAttributes(ObjectAttributes,
                                   ObjectName,
                                   Attributes,
                                   RootDirectory,
                                   SecurityDescriptor);
        return ObjectAttributes;
    }

    /* Nothing to return */
    return NULL;    
}

/*
 * Creates a stack for a thread or fiber
 */
NTSTATUS
STDCALL
BasepCreateStack(HANDLE hProcess,
                 ULONG StackReserve,
                 ULONG StackCommit,
                 PINITIAL_TEB InitialTeb)
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION SystemBasicInfo;
    PIMAGE_NT_HEADERS Headers;
    ULONG_PTR Stack = 0;
    BOOLEAN UseGuard = FALSE;
    
    DPRINT("BasepCreateStack (hProcess: %lx, Max: %lx, Current: %lx)\n",
            hProcess, StackReserve, StackCommit);
    
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
                                     0,
                                     &StackReserve,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failure to reserve stack\n");
        return Status;
    }
    
    /* Now set up some basic Initial TEB Parameters */
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
        InitialTeb->StackLimit = (PVOID)((ULONG_PTR)InitialTeb->StackLimit - GuardPageSize);
    }
    
    /* We are done! */
    return STATUS_SUCCESS;
}

VOID
STDCALL
BasepFreeStack(HANDLE hProcess,
               PINITIAL_TEB InitialTeb)
{
    SIZE_T Dummy = 0;
    
    /* Free the Stack */
    NtFreeVirtualMemory(hProcess,
                        &InitialTeb->AllocatedStackBase,
                        &Dummy,
                        MEM_RELEASE);
}
               
/*
 * Creates the Initial Context for a Thread or Fiber
 */
VOID
STDCALL
BasepInitializeContext(IN PCONTEXT Context,
                       IN PVOID Parameter,
                       IN PVOID StartAddress,
                       IN PVOID StackAddress,
                       IN ULONG ContextType)
{
#ifdef __i386__
    DPRINT("BasepInitializeContext: %p\n", Context);
    
    /* Setup the Initial Win32 Thread Context */
    Context->Eax = (ULONG)StartAddress;
    Context->Ebx = (ULONG)Parameter;
    Context->Esp = (ULONG)StackAddress;
    /* The other registers are undefined */

    /* Setup the Segments */
    Context->SegFs = KGDT_R3_TEB | RPL_MASK;
    Context->SegEs = KGDT_R3_DATA | RPL_MASK;
    Context->SegDs = KGDT_R3_DATA | RPL_MASK;
    Context->SegCs = KGDT_R3_CODE | RPL_MASK;
    Context->SegSs = KGDT_R3_DATA | RPL_MASK;
    Context->SegGs = 0;

    /* Set the EFLAGS */
    Context->EFlags = 0x3000; /* IOPL 3 */

    if (ContextType == 1)      /* For Threads */
    {
        Context->Eip = (ULONG)BaseThreadStartupThunk;
    }
    else if (ContextType == 2) /* For Fibers */
    {
        Context->Eip = (ULONG)BaseFiberStartup;
    }
    else                       /* For first thread in a Process */
    {
        Context->Eip = (ULONG)BaseProcessStartThunk;
    }
    
    /* Set the Context Flags */
    Context->ContextFlags = CONTEXT_FULL;
    
    /* Give it some room for the Parameter */
    Context->Esp -= sizeof(PVOID);
#elif defined(__x86_64__)
    DPRINT("BasepInitializeContext: %p\n", Context);
    
    /* Setup the Initial Win32 Thread Context */
    Context->Rax = (ULONG_PTR)StartAddress;
    Context->Rbx = (ULONG_PTR)Parameter;
    Context->Rsp = (ULONG_PTR)StackAddress;
    /* The other registers are undefined */

    /* Setup the Segments */
    Context->SegGs = KGDT_64_DATA | RPL_MASK;
    Context->SegEs = KGDT_64_DATA | RPL_MASK;
    Context->SegDs = KGDT_64_DATA | RPL_MASK;
    Context->SegCs = KGDT_64_R3_CODE | RPL_MASK;
    Context->SegSs = KGDT_64_DATA | RPL_MASK;
    Context->SegFs = KGDT_32_R3_TEB;

    /* Set the EFLAGS */
    Context->EFlags = 0x3000; /* IOPL 3 */

    if (ContextType == 1)      /* For Threads */
    {
        Context->Rip = (ULONG_PTR)BaseThreadStartupThunk;
    }
    else if (ContextType == 2) /* For Fibers */
    {
        Context->Rip = (ULONG_PTR)BaseFiberStartup;
    }
    else                       /* For first thread in a Process */
    {
        Context->Rip = (ULONG_PTR)BaseProcessStartThunk;
    }
    
    /* Set the Context Flags */
    Context->ContextFlags = CONTEXT_FULL;
    
    /* Give it some room for the Parameter */
    Context->Rsp -= sizeof(PVOID);
#else
#warning Unknown architecture
    UNIMPLEMENTED;
    DbgBreakPoint();
#endif
}

/*
 * Checks if the privilege for Real-Time Priority is there
 */
BOOLEAN
STDCALL
BasepCheckRealTimePrivilege(VOID)
{
    return TRUE;
}

/*
 * Maps an image file into a section
 */
NTSTATUS
STDCALL
BasepMapFile(IN LPCWSTR lpApplicationName,
             OUT PHANDLE hSection,
             IN PUNICODE_STRING ApplicationName)
{
    CURDIR RelativeName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    
    DPRINT("BasepMapFile\n");
    
    /* Zero out the Relative Directory */
    RelativeName.Handle = NULL;

    /* Find the application name */
    RtlDosPathNameToNtPathName_U(lpApplicationName,
                                 ApplicationName,
                                 NULL,
                                 &RelativeName);
    DPRINT("ApplicationName %wZ\n", ApplicationName);
    DPRINT("RelativeName %wZ\n", &RelativeName.DosPath);
    
    /* Did we get a relative name? */
    if (RelativeName.DosPath.Length)
    {
        ApplicationName = &RelativeName.DosPath;
    }

    /* Initialize the Object Attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               ApplicationName,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.Handle,
                               NULL);

    /* Try to open the executable */
    Status = NtOpenFile(&hFile,
                        SYNCHRONIZE | FILE_EXECUTE | FILE_READ_DATA,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_DELETE | FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open file\n");
        SetLastErrorByStatus (Status);
        return Status;
    }
    
    /* Create a section for this file */
    Status = NtCreateSection(hSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_IMAGE,
                             hFile);
    NtClose(hFile);
    
    /* Return status */
    DPRINT("Section: %lx for file: %lx\n", *hSection, hFile);
    return Status;
}
