/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/utils.c
 * PURPOSE:         Utility and Support Functions
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>
#ifdef _M_IX86
#include "i386/ketypes.h"
#elif defined _M_AMD64
#include "amd64/ketypes.h"
#endif

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

UNICODE_STRING Restricted = RTL_CONSTANT_STRING(L"Restricted");
BOOL bIsFileApiAnsi = TRUE; // set the file api to ansi or oem
PRTL_CONVERT_STRING Basep8BitStringToUnicodeString = RtlAnsiStringToUnicodeString;
PRTL_CONVERT_STRINGA BasepUnicodeStringTo8BitString = RtlUnicodeStringToAnsiString;
PRTL_COUNT_STRING BasepUnicodeStringTo8BitSize = BasepUnicodeStringToAnsiSize;
PRTL_COUNT_STRINGA Basep8BitStringToUnicodeSize = BasepAnsiStringToUnicodeSize;

/* FUNCTIONS ******************************************************************/

ULONG
NTAPI
BasepUnicodeStringToOemSize(IN PUNICODE_STRING String)
{
    return RtlUnicodeStringToOemSize(String);
}

ULONG
NTAPI
BasepOemStringToUnicodeSize(IN PANSI_STRING String)
{
    return RtlOemStringToUnicodeSize(String);
}

ULONG
NTAPI
BasepUnicodeStringToAnsiSize(IN PUNICODE_STRING String)
{
    return RtlUnicodeStringToAnsiSize(String);
}

ULONG
NTAPI
BasepAnsiStringToUnicodeSize(IN PANSI_STRING String)
{
    return RtlAnsiStringToUnicodeSize(String);
}

HANDLE
WINAPI
BaseGetNamedObjectDirectory(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE DirHandle, BnoHandle, Token, NewToken;

    if (BaseNamedObjectDirectory) return BaseNamedObjectDirectory;

    if (NtCurrentTeb()->IsImpersonating)
    {
        Status = NtOpenThreadToken(NtCurrentThread(),
                                   TOKEN_IMPERSONATE,
                                   TRUE,
                                   &Token);
        if (!NT_SUCCESS(Status)) return BaseNamedObjectDirectory;

        NewToken = NULL;
        Status = NtSetInformationThread(NtCurrentThread(),
                                        ThreadImpersonationToken,
                                        &NewToken,
                                        sizeof(HANDLE));
        if (!NT_SUCCESS (Status))
        {
            NtClose(Token);
            return BaseNamedObjectDirectory;
        }
    }
    else
    {
        Token = NULL;
    }

    RtlAcquirePebLock();
    if (BaseNamedObjectDirectory) goto Quickie;

    InitializeObjectAttributes(&ObjectAttributes,
                               &BaseStaticServerData->NamedObjectDirectory,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenDirectoryObject(&BnoHandle,
                                   DIRECTORY_QUERY |
                                   DIRECTORY_TRAVERSE |
                                   DIRECTORY_CREATE_OBJECT |
                                   DIRECTORY_CREATE_SUBDIRECTORY,
                                   &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        Status = NtOpenDirectoryObject(&DirHandle,
                                       DIRECTORY_TRAVERSE,
                                       &ObjectAttributes);

        if (NT_SUCCESS(Status))
        {
            InitializeObjectAttributes(&ObjectAttributes,
                                       (PUNICODE_STRING)&Restricted,
                                       OBJ_CASE_INSENSITIVE,
                                       DirHandle,
                                       NULL);

            Status = NtOpenDirectoryObject(&BnoHandle,
                                           DIRECTORY_QUERY |
                                           DIRECTORY_TRAVERSE |
                                           DIRECTORY_CREATE_OBJECT |
                                           DIRECTORY_CREATE_SUBDIRECTORY,
                                           &ObjectAttributes);
            NtClose(DirHandle);

        }
    }

    if (NT_SUCCESS(Status)) BaseNamedObjectDirectory = BnoHandle;

Quickie:

    RtlReleasePebLock();

    if (Token)
    {
        NtSetInformationThread(NtCurrentThread(),
                               ThreadImpersonationToken,
                               &Token,
                               sizeof(Token));

        NtClose(Token);
    }

    return BaseNamedObjectDirectory;
}

VOID
NTAPI
BasepLocateExeLdrEntry(IN PLDR_DATA_TABLE_ENTRY Entry,
                       IN PVOID Context,
                       OUT BOOLEAN *StopEnumeration)
{
    /* Make sure we get Entry, Context and valid StopEnumeration pointer */
    ASSERT(Entry);
    ASSERT(Context);
    ASSERT(StopEnumeration);

    /* If entry is already found - signal to stop */
    if (BasepExeLdrEntry)
    {
        *StopEnumeration = TRUE;
        return;
    }

    /* Otherwise keep enumerating until we find a match */
    if (Entry->DllBase == Context)
    {
        /* It matches, so remember the ldr entry */
        BasepExeLdrEntry = Entry;

        /* And stop enumeration */
        *StopEnumeration = TRUE;
    }
}

/*
 * Converts an ANSI or OEM String to the TEB StaticUnicodeString
 */
PUNICODE_STRING
WINAPI
Basep8BitStringToStaticUnicodeString(IN LPCSTR String)
{
    PUNICODE_STRING StaticString = &(NtCurrentTeb()->StaticUnicodeString);
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    /* Initialize an ANSI String */
    Status = RtlInitAnsiStringEx(&AnsiString, String);
    if (!NT_SUCCESS(Status))
    {
        Status = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        /* Convert it */
        Status = Basep8BitStringToUnicodeString(StaticString, &AnsiString, FALSE);
    }

    if (NT_SUCCESS(Status)) return StaticString;

    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
    }
    else
    {
        BaseSetLastNTError(Status);
    }

    return NULL;
}

/*
 * Allocates space from the Heap and converts an Unicode String into it
 */
BOOLEAN
WINAPI
Basep8BitStringToDynamicUnicodeString(OUT PUNICODE_STRING UnicodeString,
                                      IN LPCSTR String)
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    /* Initialize an ANSI String */
    Status = RtlInitAnsiStringEx(&AnsiString, String);
    if (!NT_SUCCESS(Status))
    {
        Status = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        /* Convert it */
        Status = Basep8BitStringToUnicodeString(UnicodeString, &AnsiString, TRUE);
    }

    if (NT_SUCCESS(Status)) return TRUE;

    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
    }
    else
    {
        BaseSetLastNTError(Status);
    }

    return FALSE;
}

/*
 * Allocates space from the Heap and converts an Ansi String into it
 */
 /*NOTE: API IS A HACK */
VOID
WINAPI
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

PLARGE_INTEGER
WINAPI
BaseFormatTimeOut(OUT PLARGE_INTEGER Timeout,
                  IN DWORD dwMilliseconds)
{
    /* Check if this is an infinite wait, which means no timeout argument */
    if (dwMilliseconds == INFINITE) return NULL;

    /* Otherwise, convert the time to NT Format */
    Timeout->QuadPart = dwMilliseconds * -10000LL;
    return Timeout;
}

/*
 * Converts lpSecurityAttributes + Object Name into ObjectAttributes.
 */
POBJECT_ATTRIBUTES
WINAPI
BaseFormatObjectAttributes(OUT POBJECT_ATTRIBUTES ObjectAttributes,
                           IN PSECURITY_ATTRIBUTES SecurityAttributes OPTIONAL,
                           IN PUNICODE_STRING ObjectName)
{
    ULONG Attributes;
    HANDLE RootDirectory;
    PVOID SecurityDescriptor;
    DPRINT("BaseFormatObjectAttributes. Security: %p, Name: %p\n",
            SecurityAttributes, ObjectName);

    /* Get the attributes if present */
    if (SecurityAttributes)
    {
        Attributes = SecurityAttributes->bInheritHandle ? OBJ_INHERIT : 0;
        SecurityDescriptor = SecurityAttributes->lpSecurityDescriptor;
    }
    else
    {
        if (!ObjectName) return NULL;
        Attributes = 0;
        SecurityDescriptor = NULL;
    }

    if (ObjectName)
    {
        Attributes |= OBJ_OPENIF;
        RootDirectory = BaseGetNamedObjectDirectory();
    }
    else
    {
        RootDirectory = NULL;
    }

    /* Create the Object Attributes */
    InitializeObjectAttributes(ObjectAttributes,
                               ObjectName,
                               Attributes,
                               RootDirectory,
                               SecurityDescriptor);
    DPRINT("Attributes: %lx, RootDirectory: %p, SecurityDescriptor: %p\n",
            Attributes, RootDirectory, SecurityDescriptor);
    return ObjectAttributes;
}

/*
 * Creates a stack for a thread or fiber
 */
NTSTATUS
WINAPI
BaseCreateStack(HANDLE hProcess,
                 SIZE_T StackReserve,
                 SIZE_T StackCommit,
                 PINITIAL_TEB InitialTeb)
{
    NTSTATUS Status;
    PIMAGE_NT_HEADERS Headers;
    ULONG_PTR Stack;
    BOOLEAN UseGuard;
    ULONG PageSize, Dummy, AllocationGranularity;
    SIZE_T StackReserveHeader, StackCommitHeader, GuardPageSize, GuaranteedStackCommit;
    DPRINT("BaseCreateStack (hProcess: %p, Max: %lx, Current: %lx)\n",
            hProcess, StackReserve, StackCommit);

    /* Read page size */
    PageSize = BaseStaticServerData->SysInfo.PageSize;
    AllocationGranularity = BaseStaticServerData->SysInfo.AllocationGranularity;

    /* Get the Image Headers */
    Headers = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);
    if (!Headers) return STATUS_INVALID_IMAGE_FORMAT;

    StackCommitHeader = Headers->OptionalHeader.SizeOfStackCommit;
    StackReserveHeader = Headers->OptionalHeader.SizeOfStackReserve;

    if (!StackReserve) StackReserve = StackReserveHeader;

    if (!StackCommit)
    {
        StackCommit = StackCommitHeader;
    }
    else if (StackCommit >= StackReserve)
    {
        StackReserve = ROUND_UP(StackCommit, 1024 * 1024);
    }

    StackCommit = ROUND_UP(StackCommit, PageSize);
    StackReserve = ROUND_UP(StackReserve, AllocationGranularity);

    GuaranteedStackCommit = NtCurrentTeb()->GuaranteedStackBytes;
    if ((GuaranteedStackCommit) && (StackCommit < GuaranteedStackCommit))
    {
        StackCommit = GuaranteedStackCommit;
    }

    if (StackCommit >= StackReserve)
    {
        StackReserve = ROUND_UP(StackCommit, 1024 * 1024);
    }

    StackCommit = ROUND_UP(StackCommit, PageSize);
    StackReserve = ROUND_UP(StackReserve, AllocationGranularity);

    /* Reserve memory for the stack */
    Stack = 0;
    Status = NtAllocateVirtualMemory(hProcess,
                                     (PVOID*)&Stack,
                                     0,
                                     &StackReserve,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failure to reserve stack: %lx\n", Status);
        return Status;
    }

    /* Now set up some basic Initial TEB Parameters */
    InitialTeb->AllocatedStackBase = (PVOID)Stack;
    InitialTeb->StackBase = (PVOID)(Stack + StackReserve);
    InitialTeb->PreviousStackBase = NULL;
    InitialTeb->PreviousStackLimit = NULL;

    /* Update the Stack Position */
    Stack += StackReserve - StackCommit;

    /* Check if we will need a guard page */
    if (StackReserve > StackCommit)
    {
        Stack -= PageSize;
        StackCommit += PageSize;
        UseGuard = TRUE;
    }
    else
    {
        UseGuard = FALSE;
    }

    /* Allocate memory for the stack */
    Status = NtAllocateVirtualMemory(hProcess,
                                     (PVOID*)&Stack,
                                     0,
                                     &StackCommit,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failure to allocate stack\n");
        GuardPageSize = 0;
        NtFreeVirtualMemory(hProcess, (PVOID*)&Stack, &GuardPageSize, MEM_RELEASE);
        return Status;
    }

    /* Now set the current Stack Limit */
    InitialTeb->StackLimit = (PVOID)Stack;

    /* Create a guard page */
    if (UseGuard)
    {
        /* Set the guard page */
        GuardPageSize = PAGE_SIZE;
        Status = NtProtectVirtualMemory(hProcess,
                                        (PVOID*)&Stack,
                                        &GuardPageSize,
                                        PAGE_GUARD | PAGE_READWRITE,
                                        &Dummy);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failure to set guard page\n");
            return Status;
        }

        /* Update the Stack Limit keeping in mind the Guard Page */
        InitialTeb->StackLimit = (PVOID)((ULONG_PTR)InitialTeb->StackLimit +
                                         GuardPageSize);
    }

    /* We are done! */
    return STATUS_SUCCESS;
}

VOID
WINAPI
BaseFreeThreadStack(IN HANDLE hProcess,
                    IN PINITIAL_TEB InitialTeb)
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
WINAPI
BaseInitializeContext(IN PCONTEXT Context,
                       IN PVOID Parameter,
                       IN PVOID StartAddress,
                       IN PVOID StackAddress,
                       IN ULONG ContextType)
{
#ifdef _M_IX86
    ULONG ContextFlags;
    DPRINT("BaseInitializeContext: %p\n", Context);

    /* Setup the Initial Win32 Thread Context */
    Context->Eax = (ULONG)StartAddress;
    Context->Ebx = (ULONG)Parameter;
    Context->Esp = (ULONG)StackAddress;
    /* The other registers are undefined */

    /* Setup the Segments */
    Context->SegFs = KGDT_R3_TEB;
    Context->SegEs = KGDT_R3_DATA;
    Context->SegDs = KGDT_R3_DATA;
    Context->SegCs = KGDT_R3_CODE;
    Context->SegSs = KGDT_R3_DATA;
    Context->SegGs = 0;

    /* Set the Context Flags */
    ContextFlags = Context->ContextFlags;
    Context->ContextFlags = CONTEXT_FULL;

    /* Give it some room for the Parameter */
    Context->Esp -= sizeof(PVOID);

    /* Set the EFLAGS */
    Context->EFlags = 0x3000; /* IOPL 3 */

    /* What kind of context is being created? */
    if (ContextType == 1)
    {
        /* For Threads */
        Context->Eip = (ULONG)BaseThreadStartupThunk;
    }
    else if (ContextType == 2)
    {
        /* This is a fiber: make space for the return address */
        Context->Esp -= sizeof(PVOID);
        *((PVOID*)Context->Esp) = BaseFiberStartup;

        /* Is FPU state required? */
        Context->ContextFlags |= ContextFlags;
        if (ContextFlags == CONTEXT_FLOATING_POINT)
        {
            /* Set an initial state */
            Context->FloatSave.ControlWord = 0x27F;
            Context->FloatSave.StatusWord = 0;
            Context->FloatSave.TagWord = 0xFFFF;
            Context->FloatSave.ErrorOffset = 0;
            Context->FloatSave.ErrorSelector = 0;
            Context->FloatSave.DataOffset = 0;
            Context->FloatSave.DataSelector = 0;
            if (SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE])
                Context->Dr6 = 0x1F80;
        }
    }
    else
    {
        /* For first thread in a Process */
        Context->Eip = (ULONG)BaseProcessStartThunk;
    }

#elif defined(_M_AMD64)
    DPRINT("BaseInitializeContext: %p\n", Context);

    /* Setup the Initial Win32 Thread Context */
    Context->Rax = (ULONG_PTR)StartAddress;
    Context->Rbx = (ULONG_PTR)Parameter;
    Context->Rsp = (ULONG_PTR)StackAddress;
    /* The other registers are undefined */

    /* Setup the Segments */
    Context->SegGs = KGDT64_R3_DATA | RPL_MASK;
    Context->SegEs = KGDT64_R3_DATA | RPL_MASK;
    Context->SegDs = KGDT64_R3_DATA | RPL_MASK;
    Context->SegCs = KGDT64_R3_CODE | RPL_MASK;
    Context->SegSs = KGDT64_R3_DATA | RPL_MASK;
    Context->SegFs = KGDT64_R3_CMTEB | RPL_MASK;

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
 * Beware about this function behavior:
 * - In case Keep is set to FALSE, then the function will only check
 * whether real time is allowed and won't grant the privilege. In that case
 * it will return TRUE if allowed, FALSE otherwise. Not a state!
 * It means you don't have to release privilege when calling with FALSE.
 */
PVOID
WINAPI
BasepIsRealtimeAllowed(IN BOOLEAN Keep)
{
    ULONG Privilege = SE_INC_BASE_PRIORITY_PRIVILEGE;
    PVOID State;
    NTSTATUS Status;

    Status = RtlAcquirePrivilege(&Privilege, 1, 0, &State);
    if (!NT_SUCCESS(Status)) return NULL;

    if (!Keep)
    {
        RtlReleasePrivilege(State);
        State = (PVOID)TRUE;
    }

    return State;
}

/*
 * Maps an image file into a section
 */
NTSTATUS
WINAPI
BasepMapFile(IN LPCWSTR lpApplicationName,
             OUT PHANDLE hSection,
             IN PUNICODE_STRING ApplicationName)
{
    RTL_RELATIVE_NAME_U RelativeName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatusBlock;

    DPRINT("BasepMapFile\n");

    /* Zero out the Relative Directory */
    RelativeName.ContainingDirectory = NULL;

    /* Find the application name */
    if (!RtlDosPathNameToNtPathName_U(lpApplicationName,
                                      ApplicationName,
                                      NULL,
                                      &RelativeName))
    {
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

    DPRINT("ApplicationName %wZ\n", ApplicationName);
    DPRINT("RelativeName %wZ\n", &RelativeName.RelativeName);

    /* Did we get a relative name? */
    if (RelativeName.RelativeName.Length)
    {
        ApplicationName = &RelativeName.RelativeName;
    }

    /* Initialize the Object Attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               ApplicationName,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
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
        BaseSetLastNTError(Status);
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
    DPRINT("Section: %p for file: %p\n", *hSection, hFile);
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
WINAPI
Wow64EnableWow64FsRedirection(IN BOOLEAN Wow64EnableWow64FsRedirection)
{
    NTSTATUS Status;
    BOOL Result;

    Status = RtlWow64EnableFsRedirection(Wow64EnableWow64FsRedirection);
    if (NT_SUCCESS(Status))
    {
        Result = TRUE;
    }
    else
    {
        BaseSetLastNTError(Status);
        Result = FALSE;
    }
    return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
Wow64DisableWow64FsRedirection(IN PVOID *OldValue)
{
    NTSTATUS Status;
    BOOL Result;

    Status = RtlWow64EnableFsRedirectionEx((PVOID)TRUE, OldValue);
    if (NT_SUCCESS(Status))
    {
        Result = TRUE;
    }
    else
    {
        BaseSetLastNTError(Status);
        Result = FALSE;
    }
    return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
Wow64RevertWow64FsRedirection(IN PVOID OldValue)
{
    NTSTATUS Status;
    BOOL Result;

    Status = RtlWow64EnableFsRedirectionEx(OldValue, &OldValue);
    if (NT_SUCCESS(Status))
    {
        Result = TRUE;
    }
    else
    {
        BaseSetLastNTError(Status);
        Result = FALSE;
    }
    return Result;
}

/*
 * @implemented
 */
VOID
WINAPI
SetFileApisToOEM(VOID)
{
    /* Set the correct Base Api */
    Basep8BitStringToUnicodeString = (PRTL_CONVERT_STRING)RtlOemStringToUnicodeString;
    BasepUnicodeStringTo8BitString = RtlUnicodeStringToOemString;
    BasepUnicodeStringTo8BitSize = BasepUnicodeStringToOemSize;
    Basep8BitStringToUnicodeSize = BasepOemStringToUnicodeSize;

    /* FIXME: Old, deprecated way */
    bIsFileApiAnsi = FALSE;
}


/*
 * @implemented
 */
VOID
WINAPI
SetFileApisToANSI(VOID)
{
    /* Set the correct Base Api */
    Basep8BitStringToUnicodeString = RtlAnsiStringToUnicodeString;
    BasepUnicodeStringTo8BitString = RtlUnicodeStringToAnsiString;
    BasepUnicodeStringTo8BitSize = BasepUnicodeStringToAnsiSize;
    Basep8BitStringToUnicodeSize = BasepAnsiStringToUnicodeSize;

    /* FIXME: Old, deprecated way */
    bIsFileApiAnsi = TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AreFileApisANSI(VOID)
{
   return Basep8BitStringToUnicodeString == RtlAnsiStringToUnicodeString;
}

/*
 * @implemented
 */
VOID
WINAPI
BaseMarkFileForDelete(IN HANDLE FileHandle,
                      IN ULONG FileAttributes)
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileBasicInfo;
    FILE_DISPOSITION_INFORMATION FileDispositionInfo;

    /* If no attributes were given, get them */
    if (!FileAttributes)
    {
        FileBasicInfo.FileAttributes = 0;
        NtQueryInformationFile(FileHandle,
                               &IoStatusBlock,
                               &FileBasicInfo,
                               sizeof(FileBasicInfo),
                               FileBasicInformation);
        FileAttributes = FileBasicInfo.FileAttributes;
    }

    /* If file is marked as RO, reset its attributes */
    if (FileAttributes & FILE_ATTRIBUTE_READONLY)
    {
        RtlZeroMemory(&FileBasicInfo, sizeof(FileBasicInfo));
        FileBasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        NtSetInformationFile(FileHandle,
                             &IoStatusBlock,
                             &FileBasicInfo,
                             sizeof(FileBasicInfo),
                             FileBasicInformation);
    }

    /* Finally, mark the file for deletion */
    FileDispositionInfo.DeleteFile = TRUE;
    NtSetInformationFile(FileHandle,
                         &IoStatusBlock,
                         &FileDispositionInfo,
                         sizeof(FileDispositionInfo),
                         FileDispositionInformation);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
BaseCheckRunApp(IN DWORD Unknown1,
                IN DWORD Unknown2,
                IN DWORD Unknown3,
                IN DWORD Unknown4,
                IN DWORD Unknown5,
                IN DWORD Unknown6,
                IN DWORD Unknown7,
                IN DWORD Unknown8,
                IN DWORD Unknown9,
                IN DWORD Unknown10)
{
    STUB;
    return FALSE;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
BasepCheckWinSaferRestrictions(IN HANDLE UserToken,
                               IN LPWSTR ApplicationName,
                               IN HANDLE FileHandle,
                               OUT PBOOLEAN InJob,
                               OUT PHANDLE NewToken,
                               OUT PHANDLE JobHandle)
{
    NTSTATUS Status;
    
    /* Validate that there's a name */
    if ((ApplicationName) && *(ApplicationName))
    {
        /* Validate that the required output parameters are there */
        if ((InJob) && (NewToken) && (JobHandle))
        {
            /* Do the work (one day...) */
            UNIMPLEMENTED;
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Act as if SEH hit this */
            Status = STATUS_ACCESS_VIOLATION;
        }
    }
    else
    {
        /* Input is invalid */
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Return the status */
    return Status;
}
