/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rtlexec.c

Abstract:

    User Mode routines for creating user mode processes and threads.

Author:

    Steve Wood (stevewo) 18-Aug-1989

Environment:

    User Mode or Kernel Mode

Revision History:

--*/

#include "ntrtlp.h"
#include <nturtl.h>
#include <string.h>
#include "init.h"
#include "ntos.h"
#define ROUND_UP( x, y )  ((ULONG)(x) + ((y)-1) & ~((y)-1))
#ifdef KERNEL
#define ISTERMINALSERVER() (SharedUserData->SuiteMask & (1 << TerminalServer))
#else
#define ISTERMINALSERVER() (USER_SHARED_DATA->SuiteMask & (1 << TerminalServer))
#endif

VOID
RtlpCopyProcString(
    IN OUT PWSTR *pDst,
    OUT PUNICODE_STRING DestString,
    IN PUNICODE_STRING SourceString,
    IN ULONG DstAlloc OPTIONAL
    );

NTSTATUS
RtlpOpenImageFile(
    IN PUNICODE_STRING ImagePathName,
    IN ULONG Attributes,
    OUT PHANDLE FileHandle,
    IN BOOLEAN ReportErrors
    );

NTSTATUS
RtlpFreeStack(
    IN HANDLE Process,
    IN PINITIAL_TEB InitialTeb
    );

NTSTATUS
RtlpCreateStack(
    IN HANDLE Process,
    IN SIZE_T MaximumStackSize OPTIONAL,
    IN SIZE_T CommittedStackSize OPTIONAL,
    IN ULONG ZeroBits OPTIONAL,
    OUT PINITIAL_TEB InitialTeb
    );

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(INIT,RtlpCopyProcString          )
#pragma alloc_text(INIT,RtlCreateProcessParameters  )
#pragma alloc_text(INIT,RtlDestroyProcessParameters )
#pragma alloc_text(INIT,RtlNormalizeProcessParams   )
#pragma alloc_text(INIT,RtlDeNormalizeProcessParams )
#pragma alloc_text(INIT,RtlpOpenImageFile           )
#pragma alloc_text(INIT,RtlpCreateStack             )
#pragma alloc_text(INIT,RtlpFreeStack               )
#pragma alloc_text(INIT,RtlCreateUserProcess        )
#pragma alloc_text(INIT,RtlCreateUserThread         )
#endif


VOID
RtlpCopyProcString(
    IN OUT PWSTR *pDst,
    OUT PUNICODE_STRING DestString,
    IN PUNICODE_STRING SourceString,
    IN ULONG DstAlloc OPTIONAL
    )
{
    if (!ARGUMENT_PRESENT( (ULONG_PTR)DstAlloc )) {
        DstAlloc = SourceString->MaximumLength;
        }

    if (SourceString->Buffer != NULL && SourceString->Length != 0) {
        RtlMoveMemory( *pDst,
                       SourceString->Buffer,
                       SourceString->Length
                     );
        }

    DestString->Buffer = *pDst;
    DestString->Length = SourceString->Length;
    DestString->MaximumLength = (USHORT)DstAlloc;

    *pDst = (PWSTR)((PCHAR)(*pDst) + ROUND_UP( DstAlloc, sizeof( ULONG ) ) );
    return;
}

NTSTATUS
RtlCreateProcessParameters(
    OUT PRTL_USER_PROCESS_PARAMETERS *ProcessParameters,
    IN PUNICODE_STRING ImagePathName,
    IN PUNICODE_STRING DllPath OPTIONAL,
    IN PUNICODE_STRING CurrentDirectory OPTIONAL,
    IN PUNICODE_STRING CommandLine OPTIONAL,
    IN PVOID Environment OPTIONAL,
    IN PUNICODE_STRING WindowTitle OPTIONAL,
    IN PUNICODE_STRING DesktopInfo OPTIONAL,
    IN PUNICODE_STRING ShellInfo OPTIONAL,
    IN PUNICODE_STRING RuntimeData OPTIONAL
    )

/*++

Routine Description:

    This function formats NT style RTL_USER_PROCESS_PARAMETERS
    record.  The record is self-contained in a single block of memory
    allocated by this function.  The allocation method is opaque and
    thus the record must be freed by calling the
    RtlDestroyProcessParameters function.

    The process parameters record is created in a de-normalized form,
    thus making it suitable for passing to the RtlCreateUserProcess
    function.  It is expected that the caller will fill in additional
    fields in the process parameters record after this function returns,
    but prior to calling RtlCreateUserProcess.

Arguments:

    ProcessParameters - Pointer to a variable that will receive the address
        of the process parameter structure created by this routinue.  The
        memory for the structure is allocated in an opaque manner and must
        be freed by calling RtlDestroyProcessParameters.

    ImagePathName - Required parameter that is the fully qualified NT
        path name of the image file that will be used to create the process
        that will received these parameters.

    DllPath - An optional parameter that is an NT String variable pointing
        to the search path the NT Loader is to use in the target process
        when searching for Dll modules.  If not specified, then the Dll
        search path is filled in from the current process's Dll search
        path.

    CurrentDirectory - An optional parameter that is an NT String variable
        pointing to the default directory string for the target process.
        If not specified, then the current directory string is filled in
        from the current process's current directory string.

    CommandLine - An optional parameter that is an NT String variable that
        will be passed to the target process as its command line.  If not
        specified, then the command line passed to the target process will
        be a null string.

    Environment - An optional parameter that is an opaque pointer to an
        environment variable block of the type created by
        RtlCreateEnvironment routine.  If not specified, then the target
        process will receive a copy of the calling process's environment
        variable block.

    WindowTitle - An optional parameter that is an NT String variable that
        points to the title string the target process is to use for its
        main window.  If not specified, then a null string will be passed
        to the target process as its default window title.

    DesktopInfo - An optional parameter that is an NT String variable that
        contains uninterpreted data that is passed as is to the target
        process.  If not specified, the target process will receive a
        pointer to an empty string.

    ShellInfo - An optional parameter that is an NT String variable that
        contains uninterpreted data that is passed as is to the target
        process.  If not specified, the target process will receive a
        pointer to an empty string.

    RuntimeData - An optional parameter that is an NT String variable that
        contains uninterpreted data that is passed as is to the target
        process.  If not specified, the target process will receive a
        pointer to an empty string.

Return Value:

    STATUS_SUCCESS - The process parameters is De-Normalized and
        contains entries for each of the specified argument and variable
        strings.

    STATUS_BUFFER_TOO_SMALL - The specified process parameters buffer is
        too small to contain the argument and environment strings. The value
        of ProcessParameters->Length is modified to contain the buffer
        size needed to contain the argument and variable strings.

--*/

{
    PRTL_USER_PROCESS_PARAMETERS p;
    NTSTATUS Status;
    UNICODE_STRING NullString = {0, 1, L""};
    ULONG ByteCount;
    SIZE_T MaxByteCount;
    PWSTR pDst;
    PPEB Peb = NtCurrentPeb();
    HANDLE CurDirHandle;

    //
    // Acquire the Peb Lock for the duration while we copy information out
    // of it.
    //

    RtlAcquirePebLock();
    Status = STATUS_SUCCESS;
    p = NULL;
    CurDirHandle = NULL;
    try {
        //
        // For optional pointer parameters, default them to point to their
        // corresponding field in the current process's process parameter
        // structure or to a null string.
        //

        if (!ARGUMENT_PRESENT( DllPath )) {
            DllPath = &Peb->ProcessParameters->DllPath;
            }

        if (!ARGUMENT_PRESENT( CurrentDirectory )) {

            if ( Peb->ProcessParameters->CurrentDirectory.Handle ) {
                CurDirHandle = (HANDLE)((ULONG_PTR)Peb->ProcessParameters->CurrentDirectory.Handle & ~OBJ_HANDLE_TAGBITS);
                CurDirHandle = (HANDLE)((ULONG_PTR)CurDirHandle | RTL_USER_PROC_CURDIR_INHERIT);
                }
            CurrentDirectory = &Peb->ProcessParameters->CurrentDirectory.DosPath;
            }
        else {
            if ( Peb->ProcessParameters->CurrentDirectory.Handle ) {
                CurDirHandle = (HANDLE)((ULONG_PTR)Peb->ProcessParameters->CurrentDirectory.Handle & ~OBJ_HANDLE_TAGBITS);
                CurDirHandle = (HANDLE)((ULONG_PTR)CurDirHandle | RTL_USER_PROC_CURDIR_CLOSE);
                }
            }

        if (!ARGUMENT_PRESENT( CommandLine )) {
            CommandLine = ImagePathName;
            }

        if (!ARGUMENT_PRESENT( Environment )) {
            Environment = Peb->ProcessParameters->Environment;
            }

        if (!ARGUMENT_PRESENT( WindowTitle )) {
            WindowTitle = &NullString;
            }

        if (!ARGUMENT_PRESENT( DesktopInfo )) {
            DesktopInfo = &NullString;
            }

        if (!ARGUMENT_PRESENT( ShellInfo )) {
            ShellInfo = &NullString;
            }

        if (!ARGUMENT_PRESENT( RuntimeData )) {
            RuntimeData = &NullString;
            }

        //
        // Determine size need to contain the process parameter record
        // structure and all of the strings it will point to.  Each string
        // will be aligned on a ULONG byte boundary.
        //

        ByteCount = sizeof( **ProcessParameters );
        ByteCount += ROUND_UP( ImagePathName->Length + sizeof(UNICODE_NULL),    sizeof( ULONG ) );
        ByteCount += ROUND_UP( DllPath->MaximumLength,       sizeof( ULONG ) );
        ByteCount += ROUND_UP( DOS_MAX_PATH_LENGTH*2,        sizeof( ULONG ) );
        ByteCount += ROUND_UP( CommandLine->Length + sizeof(UNICODE_NULL),      sizeof( ULONG ) );
        ByteCount += ROUND_UP( WindowTitle->MaximumLength,   sizeof( ULONG ) );
        ByteCount += ROUND_UP( DesktopInfo->MaximumLength,   sizeof( ULONG ) );
        ByteCount += ROUND_UP( ShellInfo->MaximumLength,     sizeof( ULONG ) );
        ByteCount += ROUND_UP( RuntimeData->MaximumLength,   sizeof( ULONG ) );

        //
        // Allocate memory for the process parameter record.
        //

        MaxByteCount = ByteCount;
        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&p,
                                          0,
                                          &MaxByteCount,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
            return( Status );
            }

        p->MaximumLength = (ULONG) MaxByteCount;
        p->Length = ByteCount;
        p->Flags = RTL_USER_PROC_PARAMS_NORMALIZED;
        p->Environment = Environment;
        p->CurrentDirectory.Handle = CurDirHandle;

        //
        // Inherits ^C inhibit information
        //

        p->ConsoleFlags = Peb->ProcessParameters->ConsoleFlags;

        pDst = (PWSTR)(p + 1);
        RtlpCopyProcString( &pDst,
                            &p->CurrentDirectory.DosPath,
                            CurrentDirectory,
                            DOS_MAX_PATH_LENGTH*2
                          );

        RtlpCopyProcString( &pDst, &p->DllPath, DllPath, 0 );
        RtlpCopyProcString( &pDst, &p->ImagePathName, ImagePathName, ImagePathName->Length + sizeof(UNICODE_NULL) );
        if (CommandLine->Length == CommandLine->MaximumLength) {
            RtlpCopyProcString( &pDst, &p->CommandLine, CommandLine, 0 );
            }
        else {
            RtlpCopyProcString( &pDst, &p->CommandLine, CommandLine, CommandLine->Length + sizeof(UNICODE_NULL) );
            }
        RtlpCopyProcString( &pDst, &p->WindowTitle, WindowTitle, 0 );
        RtlpCopyProcString( &pDst, &p->DesktopInfo, DesktopInfo, 0 );
        RtlpCopyProcString( &pDst, &p->ShellInfo,   ShellInfo, 0 );
        if (RuntimeData->Length != 0) {
            RtlpCopyProcString( &pDst, &p->RuntimeData, RuntimeData, 0 );
            }
        else {
            p->RuntimeData.Buffer = NULL;
            p->RuntimeData.Length = 0;
            p->RuntimeData.MaximumLength = 0;
            }
        *ProcessParameters = RtlDeNormalizeProcessParams( p );
        p = NULL;
        }
    finally {
        if (AbnormalTermination()) {
            Status = STATUS_ACCESS_VIOLATION;
            }

        if (p != NULL) {
            RtlDestroyProcessParameters( p );
            }

        RtlReleasePebLock();
        }

    return( Status );
}



NTSTATUS
RtlDestroyProcessParameters(
    IN PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    )
{
    NTSTATUS Status;
    SIZE_T RegionSize;

    RegionSize = 0;
    Status = ZwFreeVirtualMemory( NtCurrentProcess(),
                                  (PVOID *)&ProcessParameters,
                                  &RegionSize,
                                  MEM_RELEASE
                                );

    return( Status );
}


#define RtlpNormalizeProcessParam( Base, p )        \
    if ((p) != NULL) {                              \
        (p) = (PWSTR)((PCHAR)(p) + (ULONG_PTR)(Base));  \
        }                                           \

#define RtlpDeNormalizeProcessParam( Base, p )      \
    if ((p) != NULL) {                              \
        (p) = (PWSTR)((PCHAR)(p) - (ULONG_PTR)(Base));  \
        }                                           \


PRTL_USER_PROCESS_PARAMETERS
RtlNormalizeProcessParams(
    IN OUT PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    )
{
    if (!ARGUMENT_PRESENT( ProcessParameters )) {
        return( NULL );
        }

    if (ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED) {
        return( ProcessParameters );
        }

    RtlpNormalizeProcessParam( ProcessParameters,
                               ProcessParameters->CurrentDirectory.DosPath.Buffer
                             );

    RtlpNormalizeProcessParam( ProcessParameters,
                               ProcessParameters->DllPath.Buffer
                             );

    RtlpNormalizeProcessParam( ProcessParameters,
                               ProcessParameters->ImagePathName.Buffer
                             );

    RtlpNormalizeProcessParam( ProcessParameters,
                               ProcessParameters->CommandLine.Buffer
                             );

    RtlpNormalizeProcessParam( ProcessParameters,
                               ProcessParameters->WindowTitle.Buffer
                             );

    RtlpNormalizeProcessParam( ProcessParameters,
                               ProcessParameters->DesktopInfo.Buffer
                             );

    RtlpNormalizeProcessParam( ProcessParameters,
                               ProcessParameters->ShellInfo.Buffer
                             );

    RtlpNormalizeProcessParam( ProcessParameters,
                               ProcessParameters->RuntimeData.Buffer
                             );
    ProcessParameters->Flags |= RTL_USER_PROC_PARAMS_NORMALIZED;

    return( ProcessParameters );
}

PRTL_USER_PROCESS_PARAMETERS
RtlDeNormalizeProcessParams(
    IN OUT PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    )
{
    if (!ARGUMENT_PRESENT( ProcessParameters )) {
        return( NULL );
        }

    if (!(ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) {
        return( ProcessParameters );
        }

    RtlpDeNormalizeProcessParam( ProcessParameters,
                                 ProcessParameters->CurrentDirectory.DosPath.Buffer
                               );

    RtlpDeNormalizeProcessParam( ProcessParameters,
                                 ProcessParameters->DllPath.Buffer
                               );

    RtlpDeNormalizeProcessParam( ProcessParameters,
                                 ProcessParameters->ImagePathName.Buffer
                               );

    RtlpDeNormalizeProcessParam( ProcessParameters,
                                 ProcessParameters->CommandLine.Buffer
                               );

    RtlpDeNormalizeProcessParam( ProcessParameters,
                                 ProcessParameters->WindowTitle.Buffer
                               );

    RtlpDeNormalizeProcessParam( ProcessParameters,
                                 ProcessParameters->DesktopInfo.Buffer
                               );

    RtlpDeNormalizeProcessParam( ProcessParameters,
                                 ProcessParameters->ShellInfo.Buffer
                               );

    RtlpDeNormalizeProcessParam( ProcessParameters,
                                 ProcessParameters->RuntimeData.Buffer
                               );

    ProcessParameters->Flags &= ~RTL_USER_PROC_PARAMS_NORMALIZED;
    return( ProcessParameters );
}

NTSTATUS
RtlpOpenImageFile(
    IN PUNICODE_STRING ImagePathName,
    IN ULONG Attributes,
    OUT PHANDLE FileHandle,
    IN BOOLEAN ReportErrors
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE File;
    IO_STATUS_BLOCK IoStatus;

    *FileHandle = NULL;

    InitializeObjectAttributes( &ObjectAttributes,
                                ImagePathName,
                                Attributes,
                                NULL,
                                NULL
                              );
    Status = ZwOpenFile( &File,
                         SYNCHRONIZE | FILE_EXECUTE,
                         &ObjectAttributes,
                         &IoStatus,
                         FILE_SHARE_READ | FILE_SHARE_DELETE,
                         FILE_NON_DIRECTORY_FILE
                         );

    if (!NT_SUCCESS( Status )) {
#if DBG
        if (ReportErrors) {
            DbgPrint( "NTRTL: RtlpOpenImageFile - NtCreateFile( %wZ ) failed.  Status == %X\n",
                      ImagePathName,
                      Status
                    );
            }
#endif // DBG
        return( Status );
        }

    *FileHandle = File;
    return( STATUS_SUCCESS );
}


NTSTATUS
RtlpCreateStack(
    IN HANDLE Process,
    IN SIZE_T MaximumStackSize OPTIONAL,
    IN SIZE_T CommittedStackSize OPTIONAL,
    IN ULONG ZeroBits OPTIONAL,
    OUT PINITIAL_TEB InitialTeb
    )
{
    NTSTATUS Status;
    PCH Stack;
    SYSTEM_BASIC_INFORMATION SysInfo;
    BOOLEAN GuardPage;
    SIZE_T RegionSize;
    ULONG OldProtect;
#if defined(_IA64_)
    PCH Bstore;
    SIZE_T CommittedBstoreSize;
    SIZE_T MaximumBstoreSize;
    SIZE_T MstackPlusBstoreSize;
#endif

    Status = ZwQuerySystemInformation( SystemBasicInformation,
                                       (PVOID)&SysInfo,
                                       sizeof( SysInfo ),
                                       NULL
                                     );
    if ( !NT_SUCCESS( Status ) ) {
        return( Status );
        }

    //
    // if stack is in the current process, then default to
    // the parameters from the image
    //

    if ( Process == NtCurrentProcess() ) {
        PPEB Peb;
        PIMAGE_NT_HEADERS NtHeaders;


        Peb = NtCurrentPeb();
        NtHeaders = RtlImageNtHeader(Peb->ImageBaseAddress);


        if (!MaximumStackSize) {
            MaximumStackSize = NtHeaders->OptionalHeader.SizeOfStackReserve;
            }

        if (!CommittedStackSize) {
            CommittedStackSize = NtHeaders->OptionalHeader.SizeOfStackCommit;
            }

        }
    else {

        if (!CommittedStackSize) {
            CommittedStackSize = SysInfo.PageSize;
            }

        if (!MaximumStackSize) {
            MaximumStackSize = SysInfo.AllocationGranularity;
            }

        }


    if ( CommittedStackSize >= MaximumStackSize ) {
        MaximumStackSize = ROUND_UP(CommittedStackSize, (1024*1024));
        }


    CommittedStackSize = ROUND_UP( CommittedStackSize, SysInfo.PageSize );
    MaximumStackSize = ROUND_UP( MaximumStackSize,
                                 SysInfo.AllocationGranularity
                               );

    Stack = NULL,

#if defined(_IA64_)

    //
    // Piggyback the backing store with the memory stack
    //

    CommittedBstoreSize = CommittedStackSize;
    MaximumBstoreSize = MaximumStackSize;
    MstackPlusBstoreSize = MaximumBstoreSize + MaximumStackSize;

    Status = ZwAllocateVirtualMemory( Process,
                                      (PVOID *)&Stack,
                                      ZeroBits,
                                      &MstackPlusBstoreSize,
                                      MEM_RESERVE,
                                      PAGE_READWRITE
                                    );
#else

    Status = ZwAllocateVirtualMemory( Process,
                                      (PVOID *)&Stack,
                                      ZeroBits,
                                      &MaximumStackSize,
                                      MEM_RESERVE,
                                      PAGE_READWRITE
                                    );
#endif // defined(_IA64_)

    if ( !NT_SUCCESS( Status ) ) {
#if DBG
        DbgPrint( "NTRTL: RtlpCreateStack( %lx ) failed.  Stack Reservation Status == %X\n",
                  Process,
                  Status
                );
#endif // DBG
        return( Status );
        }

#if defined(_IA64_)
    InitialTeb->OldInitialTeb.OldBStoreLimit = NULL;
#endif // defined(_IA64_)

    InitialTeb->OldInitialTeb.OldStackBase = NULL;
    InitialTeb->OldInitialTeb.OldStackLimit = NULL;
    InitialTeb->StackAllocationBase = Stack;
    InitialTeb->StackBase = Stack + MaximumStackSize;

    Stack += MaximumStackSize - CommittedStackSize;
    if (MaximumStackSize > CommittedStackSize) {
        Stack -= SysInfo.PageSize;
        CommittedStackSize += SysInfo.PageSize;
        GuardPage = TRUE;
        }
    else {
        GuardPage = FALSE;
        }
    Status = ZwAllocateVirtualMemory( Process,
                                      (PVOID *)&Stack,
                                      0,
                                      &CommittedStackSize,
                                      MEM_COMMIT,
                                      PAGE_READWRITE
                                    );
    InitialTeb->StackLimit = Stack;

    if ( !NT_SUCCESS( Status ) ) {
#if DBG
        DbgPrint( "NTRTL: RtlpCreateStack( %lx ) failed.  Stack Commit Status == %X\n",
                  Process,
                  Status
                );
#endif // DBG
        return( Status );
        }

    //
    // if we have space, create a guard page.
    //

    if (GuardPage) {
        RegionSize =  SysInfo.PageSize;
        Status = ZwProtectVirtualMemory( Process,
                                         (PVOID *)&Stack,
                                         &RegionSize,
                                         PAGE_GUARD | PAGE_READWRITE,
                                         &OldProtect);


        if ( !NT_SUCCESS( Status ) ) {
#if DBG
            DbgPrint( "NTRTL: RtlpCreateStack( %lx ) failed.  Guard Page Creation Status == %X\n",
                      Process,
                      Status
                    );
#endif // DBG
            return( Status );
            }
#if defined(_IA64_)
        InitialTeb->StackLimit = (PVOID)((PUCHAR)InitialTeb->StackLimit + RegionSize);
#else
        InitialTeb->StackLimit = (PVOID)((PUCHAR)InitialTeb->StackLimit - RegionSize);
#endif // defined(_IA64_)
        }

#if defined(_IA64_)

    //
    // Commit backing store pages and create guard pages if there is space
    //

    Bstore = InitialTeb->StackBase;
    if (MaximumBstoreSize > CommittedBstoreSize) {
        CommittedBstoreSize += SysInfo.PageSize;
        GuardPage = TRUE;
    } else {
        GuardPage = FALSE;
    }

    Status = ZwAllocateVirtualMemory( Process,
                                      (PVOID *)&Bstore,
                                      0,
                                      &CommittedBstoreSize,
                                      MEM_COMMIT,
                                      PAGE_READWRITE
                                    );

    InitialTeb->BStoreLimit = Bstore + CommittedBstoreSize;

    if ( !NT_SUCCESS(Status) ) {
#if DBG
        DbgPrint("NTRTL: RtlpCreateStack( %lx ) failed. Backing Store Commit Status == %X\n",
                 Process,
                 Status
                );
#endif // DBG
        return (Status);
    }

    if (GuardPage) {
        Bstore = (PCH)InitialTeb->BStoreLimit - SysInfo.PageSize;
        RegionSize = SysInfo.PageSize;
        Status = ZwProtectVirtualMemory(Process,
                                        (PVOID *)&Bstore,
                                        &RegionSize,
                                        PAGE_GUARD | PAGE_READWRITE,
                                        &OldProtect
                                       );
        if ( !NT_SUCCESS(Status) ) {
#if DBG
            DbgPrint("NTRTL: RtlpCreateStack( %lx ) failed.  Backing Store Guard Page Creation Status == %X\n",
                     Process,
                     Status
                    );
#endif // DBG
            return (Status);
        }
        InitialTeb->BStoreLimit = (PVOID)((PUCHAR)InitialTeb->BStoreLimit - RegionSize);
    }

#endif // defined(_IA64_)

    return( STATUS_SUCCESS );
}


NTSTATUS
RtlpFreeStack(
    IN HANDLE Process,
    IN PINITIAL_TEB InitialTeb
    )
{
    NTSTATUS Status;
    SIZE_T Zero;

    Zero = 0;
    Status = ZwFreeVirtualMemory( Process,
                                  &InitialTeb->StackAllocationBase,
                                  &Zero,
                                  MEM_RELEASE
                                );
    if ( !NT_SUCCESS( Status ) ) {
#if DBG
        DbgPrint( "NTRTL: RtlpFreeStack( %lx ) failed.  Stack DeCommit Status == %X\n",
                  Process,
                  Status
                );
#endif // DBG
        return( Status );
        }

    RtlZeroMemory( InitialTeb, sizeof( *InitialTeb ) );
    return( STATUS_SUCCESS );
}


NTSTATUS
RtlCreateUserProcess(
    IN PUNICODE_STRING NtImagePathName,
    IN ULONG Attributes,
    IN PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
    IN PSECURITY_DESCRIPTOR ProcessSecurityDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR ThreadSecurityDescriptor OPTIONAL,
    IN HANDLE ParentProcess OPTIONAL,
    IN BOOLEAN InheritHandles,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL,
    OUT PRTL_USER_PROCESS_INFORMATION ProcessInformation
    )

/*++

Routine Description:

    This function creates a user mode process with a single thread with
    a suspend count of one.  The address space of the new process is
    initialized with the contents of specified image file.  The caller
    can specify the Access Control List for the new process and thread.
    The caller can also specify the parent process to inherit process
    priority and processor affinity from.  The default is to inherit
    these from the current process.  Finally the caller can specify
    whether the new process is to inherit any of the object handles
    from the specified parent process or not.

    Information about the new process and thread is returned via
    the ProcessInformation parameter.

Arguments:

    NtImagePathName - A required pointer that points to the NT Path string
        that identifies the image file that is to be loaded into the
        child process.

    ProcessParameters - A required pointer that points to parameters that
        are to passed to the child process.

    ProcessSecurityDescriptor - An optional pointer to the Security Descriptor
        give to the new process.

    ThreadSecurityDescriptor - An optional pointer to the Security Descriptor
        give to the new thread.

    ParentProcess - An optional process handle that will used to inherit
        certain properties from.

    InheritHandles - A boolean value.  TRUE specifies that object handles
        associated with the specified parent process are to be inherited
        by the new process, provided they have the OBJ_INHERIT attribute.
        FALSE specifies that the new process is to inherit no handles.

    DebugPort - An optional handle to the debug port associated with this
        process.

    ExceptionPort - An optional handle to the exception port associated with this
        process.

    ProcessInformation - A pointer to a variable that receives information
        about the new process and thread.

Return Value:

    TBS.

--*/

{
    NTSTATUS Status;
    HANDLE Section, File;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PRTL_USER_PROCESS_PARAMETERS Parameters;
    SIZE_T ParameterLength;
    PVOID Environment;
    PWCHAR s;
    ULONG EnvironmentLength;
    SIZE_T RegionSize;
    PROCESS_BASIC_INFORMATION ProcessInfo;
    PPEB Peb;
    UNICODE_STRING Unicode;

    //
    // Zero output parameter and probe the addresses at the same time
    //

    RtlZeroMemory( ProcessInformation, sizeof( *ProcessInformation ) );
    ProcessInformation->Length = sizeof( *ProcessInformation );

    //
    // Open the specified image file.
    //

    Status = RtlpOpenImageFile( NtImagePathName,
                                Attributes & (OBJ_INHERIT | OBJ_CASE_INSENSITIVE),
                                &File,
                                TRUE
                              );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }


    //
    // Create a memory section backed by the opened image file
    //

    Status = ZwCreateSection( &Section,
                              SECTION_ALL_ACCESS,
                              NULL,
                              NULL,
                              PAGE_EXECUTE,
                              SEC_IMAGE,
                              File
                            );
    ZwClose( File );
    if ( !NT_SUCCESS( Status ) ) {
        return( Status );
        }


    //
    // Create the user mode process, defaulting the parent process to the
    // current process if one is not specified.  The new process will not
    // have a name nor will the handle be inherited by other processes.
    //

    if (!ARGUMENT_PRESENT( ParentProcess )) {
        ParentProcess = NtCurrentProcess();
        }

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL,
                                ProcessSecurityDescriptor );
    if ( RtlGetNtGlobalFlags() & FLG_ENABLE_CSRDEBUG ) {
        if ( wcsstr(NtImagePathName->Buffer,L"csrss") ||
             wcsstr(NtImagePathName->Buffer,L"CSRSS")
           ) {

            //
            // For Hydra we don't name the CSRSS process to avoid name
            // collissions when multiple CSRSS's are started
            //
            if (ISTERMINALSERVER()) {

                InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL,
                                            ProcessSecurityDescriptor );
            } else {

                RtlInitUnicodeString(&Unicode,L"\\WindowsSS");
                InitializeObjectAttributes( &ObjectAttributes, &Unicode, 0, NULL,
                                            ProcessSecurityDescriptor );
            }

            }
        }

    if ( !InheritHandles ) {
        ProcessParameters->CurrentDirectory.Handle = NULL;
        }
    Status = ZwCreateProcess( &ProcessInformation->Process,
                              PROCESS_ALL_ACCESS,
                              &ObjectAttributes,
                              ParentProcess,
                              InheritHandles,
                              Section,
                              DebugPort,
                              ExceptionPort
                            );
    if ( !NT_SUCCESS( Status ) ) {
        ZwClose( Section );
        return( Status );
        }


    //
    // Retreive the interesting information from the image header
    //

    Status = ZwQuerySection( Section,
                             SectionImageInformation,
                             &ProcessInformation->ImageInformation,
                             sizeof( ProcessInformation->ImageInformation ),
                             NULL
                           );
    if ( !NT_SUCCESS( Status ) ) {
        ZwClose( ProcessInformation->Process );
        ZwClose( Section );
        return( Status );
        }

    Status = ZwQueryInformationProcess( ProcessInformation->Process,
                                        ProcessBasicInformation,
                                        &ProcessInfo,
                                        sizeof( ProcessInfo ),
                                        NULL
                                      );
    if ( !NT_SUCCESS( Status ) ) {
        ZwClose( ProcessInformation->Process );
        ZwClose( Section );
        return( Status );
        }

    Peb = ProcessInfo.PebBaseAddress;

    //
    // Duplicate Native handles into new process if any specified.
    // Note that the duplicated handles will overlay the input values.
    //

    try {
        Status = STATUS_SUCCESS;

        if ( ProcessParameters->StandardInput ) {

            Status = ZwDuplicateObject(
                        ParentProcess,
                        ProcessParameters->StandardInput,
                        ProcessInformation->Process,
                        &ProcessParameters->StandardInput,
                        0L,
                        0L,
                        DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES
                        );
            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }
        }

        if ( ProcessParameters->StandardOutput ) {

            Status = ZwDuplicateObject(
                        ParentProcess,
                        ProcessParameters->StandardOutput,
                        ProcessInformation->Process,
                        &ProcessParameters->StandardOutput,
                        0L,
                        0L,
                        DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES
                        );
            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }
        }

        if ( ProcessParameters->StandardError ) {

            Status = ZwDuplicateObject(
                        ParentProcess,
                        ProcessParameters->StandardError,
                        ProcessInformation->Process,
                        &ProcessParameters->StandardError,
                        0L,
                        0L,
                        DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES
                        );
            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }
        }

    } finally {
        if ( !NT_SUCCESS(Status) ) {
            ZwClose( ProcessInformation->Process );
            ZwClose( Section );
            }
        }

    //
    // Possibly reserve some address space in the new process
    //

    if (ProcessInformation->ImageInformation.SubSystemType == IMAGE_SUBSYSTEM_NATIVE ) {
        if ( ProcessParameters->Flags & RTL_USER_PROC_RESERVE_1MB ) {

#if defined(_IA64_)
            Environment = (PVOID)(UADDRESS_BASE+4);
#else
            Environment = (PVOID)(4);
#endif
            RegionSize = (1024*1024)-(256);

            Status = ZwAllocateVirtualMemory( ProcessInformation->Process,
                                              (PVOID *)&Environment,
                                              0,
                                              &RegionSize,
                                              MEM_RESERVE,
                                              PAGE_READWRITE
                                            );
            if ( !NT_SUCCESS( Status ) ) {
                ZwClose( ProcessInformation->Process );
                ZwClose( Section );
                return( Status );
                }
            }
        }

    //
    // Allocate virtual memory in the new process and use NtWriteVirtualMemory
    // to write a copy of the process environment block into the address
    // space of the new process.  Save the address of the allocated block in
    // the process parameter block so the new process can access it.
    //

    if (s = (PWCHAR)ProcessParameters->Environment) {
        while (*s++) {
            while (*s++) {
                }
            }
        EnvironmentLength = (ULONG)(s - (PWCHAR)ProcessParameters->Environment) * sizeof(WCHAR);

        Environment = NULL;
        RegionSize = EnvironmentLength;
        Status = ZwAllocateVirtualMemory( ProcessInformation->Process,
                                          (PVOID *)&Environment,
                                          0,
                                          &RegionSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if ( !NT_SUCCESS( Status ) ) {
            ZwClose( ProcessInformation->Process );
            ZwClose( Section );
            return( Status );
            }

        Status = ZwWriteVirtualMemory( ProcessInformation->Process,
                                       Environment,
                                       ProcessParameters->Environment,
                                       EnvironmentLength,
                                       NULL
                                     );
        if ( !NT_SUCCESS( Status ) ) {
            ZwClose( ProcessInformation->Process );
            ZwClose( Section );
            return( Status );
            }

        ProcessParameters->Environment = Environment;
        }

    //
    // Allocate virtual memory in the new process and use NtWriteVirtualMemory
    // to write a copy of the process parameters block into the address
    // space of the new process.  Set the initial parameter to the new thread
    // to be the address of the block in the new process's address space.
    //

    Parameters = NULL;
    ParameterLength = ProcessParameters->MaximumLength;
    Status = ZwAllocateVirtualMemory( ProcessInformation->Process,
                                      (PVOID *)&Parameters,
                                      0,
                                      &ParameterLength,
                                      MEM_COMMIT,
                                      PAGE_READWRITE
                                    );
    if ( !NT_SUCCESS( Status ) ) {
        ZwClose( ProcessInformation->Process );
        ZwClose( Section );
        return( Status );
        }

    Status = ZwWriteVirtualMemory( ProcessInformation->Process,
                                   Parameters,
                                   ProcessParameters,
                                   ProcessParameters->Length,
                                   NULL
                                 );
    if ( !NT_SUCCESS( Status ) ) {
            ZwClose( ProcessInformation->Process );
            ZwClose( Section );
            return( Status );
            }

    Status = ZwWriteVirtualMemory( ProcessInformation->Process,
                                   &Peb->ProcessParameters,
                                   &Parameters,
                                   sizeof( Parameters ),
                                   NULL
                                 );
    if ( !NT_SUCCESS( Status ) ) {
        ZwClose( ProcessInformation->Process );
        ZwClose( Section );
        return( Status );
        }

    //
    // Create a suspended thread in the new process.  Specify the size and
    // position of the stack, along with the start address, initial parameter
    // and an SECURITY_DESCRIPTOR.  The new thread will not have a name and its handle will
    // not be inherited by other processes.
    //

    Status = RtlCreateUserThread(
                 ProcessInformation->Process,
                 ThreadSecurityDescriptor,
                 TRUE,
                 ProcessInformation->ImageInformation.ZeroBits,
                 ProcessInformation->ImageInformation.MaximumStackSize,
                 ProcessInformation->ImageInformation.CommittedStackSize,
                 (PUSER_THREAD_START_ROUTINE)
                     ProcessInformation->ImageInformation.TransferAddress,
                 (PVOID)Peb,
                 &ProcessInformation->Thread,
                 &ProcessInformation->ClientId
                 );
    if ( !NT_SUCCESS( Status ) ) {
        ZwClose( ProcessInformation->Process );
        ZwClose( Section );
        return( Status );
        }

    //
    // Now close the section and file handles.  The objects they represent
    // will not actually go away until the process is destroyed.
    //

    ZwClose( Section );

    //
    // Return success status
    //

    return( STATUS_SUCCESS );
}


NTSTATUS
RtlCreateUserThread(
    IN HANDLE Process,
    IN PSECURITY_DESCRIPTOR ThreadSecurityDescriptor OPTIONAL,
    IN BOOLEAN CreateSuspended,
    IN ULONG ZeroBits OPTIONAL,
    IN SIZE_T MaximumStackSize OPTIONAL,
    IN SIZE_T CommittedStackSize OPTIONAL,
    IN PUSER_THREAD_START_ROUTINE StartAddress,
    IN PVOID Parameter OPTIONAL,
    OUT PHANDLE Thread OPTIONAL,
    OUT PCLIENT_ID ClientId OPTIONAL
    )

/*++

Routine Description:

    This function creates a user mode thread in a user process.  The caller
    specifies the attributes of the new thread.  A handle to the thread, along
    with its Client Id are returned to the caller.

Arguments:

    Process - Handle to the target process in which to create the new thread.

    ThreadSecurityDescriptor - An optional pointer to the Security Descriptor
        give to the new thread.

    CreateSuspended - A boolean parameter that specifies whether or not the new
        thread is to be created suspended or not.  If TRUE, the new thread
        will be created with an initial suspend count of 1.  If FALSE then
        the new thread will be ready to run when this call returns.

    ZeroBits - This parameter is passed to the virtual memory manager
        when the stack is allocated.  Stacks are always allocated with the
        MEM_TOP_DOWN allocation attribute.

    MaximumStackSize - This is the maximum size of the stack.  This size
        will be rounded up to the next highest page boundary.  If zero is
        specified, then the default size will be 64K bytes.

    CommittedStackSize - This is the initial committed size of the stack.  This
        size is rounded up to the next highest page boundary and then an
        additional page is added for the guard page.  The resulting size
        will then be commited and the guard page protection initialized
        for the last committed page in the stack.

    StartAddress - The initial starting address of the thread.

    Parameter - An optional pointer to a 32-bit pointer parameter that is
        passed as a single argument to the procedure at the start address
        location.

    Thread - An optional pointer that, if specified, points to a variable that
        will receive the handle of the new thread.

    ClientId - An optional pointer that, if specified, points to a variable
        that will receive the Client Id of the new thread.

Return Value:

    TBS

--*/

{
    NTSTATUS Status;
    CONTEXT ThreadContext;
    OBJECT_ATTRIBUTES ObjectAttributes;
    INITIAL_TEB InitialTeb;
    HANDLE ThreadHandle;
    CLIENT_ID ThreadClientId;

    //
    // Allocate a stack for this thread in the address space of the target
    // process.
    //

    Status = RtlpCreateStack( Process,
                              MaximumStackSize,
                              CommittedStackSize,
                              ZeroBits,
                              &InitialTeb
                            );
    if ( !NT_SUCCESS( Status ) ) {
        return( Status );
        }

    //
    // Create an initial context for the new thread.
    //


    RtlInitializeContext( Process,
                          &ThreadContext,
                          Parameter,
                          (PVOID)StartAddress,
                          InitialTeb.StackBase
                        );

    //
    // Now create a thread in the target process.  The new thread will
    // not have a name and its handle will not be inherited by other
    // processes.
    //

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL,
                                ThreadSecurityDescriptor );
    Status = ZwCreateThread( &ThreadHandle,
                             THREAD_ALL_ACCESS,
                             &ObjectAttributes,
                             Process,
                             &ThreadClientId,
                             &ThreadContext,
                             &InitialTeb,
                             CreateSuspended
                           );
    if (!NT_SUCCESS( Status )) {
#if DBG
        DbgPrint( "NTRTL: RtlCreateUserThread Failed. NtCreateThread Status == %X\n",
                  Status );
#endif // DBG
        RtlpFreeStack( Process, &InitialTeb );
        }
    else {
        if (ARGUMENT_PRESENT( Thread )) {
            *Thread = ThreadHandle;
            }

        if (ARGUMENT_PRESENT( ClientId )) {
            *ClientId = ThreadClientId;
            }

        }

    //
    // Return status
    //

    return( Status );
}


VOID
RtlFreeUserThreadStack(
    HANDLE hProcess,
    HANDLE hThread
    )
{
    NTSTATUS Status;
    PTEB Teb;
    THREAD_BASIC_INFORMATION ThreadInfo;
    PVOID StackDeallocationBase;
    ULONG Length;
    SIZE_T Size;

    Status = NtQueryInformationThread( hThread,
                                       ThreadBasicInformation,
                                       &ThreadInfo,
                                       sizeof( ThreadInfo ),
                                       NULL
                                     );
    Teb = ThreadInfo.TebBaseAddress;
    if (!NT_SUCCESS( Status ) || !Teb) {
        return;
        }

    Status = NtReadVirtualMemory( hProcess,
                                  &Teb->DeallocationStack,
                                  &StackDeallocationBase,
                                  sizeof( StackDeallocationBase ),
                                  &Length
                                );
    if (!NT_SUCCESS( Status ) || !StackDeallocationBase) {
        return;
        }

    Size = 0;
    NtFreeVirtualMemory( hProcess, &StackDeallocationBase, &Size, MEM_RELEASE );
    return;
}
