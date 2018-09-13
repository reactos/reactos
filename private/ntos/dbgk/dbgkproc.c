/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dbgkproc.c

Abstract:

    This module implements process control primitives for the
    Dbg component of NT

Author:

    Mark Lucovsky (markl) 19-Jan-1990

Revision History:

--*/

#include "dbgkp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DbgkpSuspendProcess)
#pragma alloc_text(PAGE, DbgkpResumeProcess)
#pragma alloc_text(PAGE, DbgkpSectionHandleToFileHandle)
#pragma alloc_text(PAGE, DbgkCreateThread)
#pragma alloc_text(PAGE, DbgkExitThread)
#pragma alloc_text(PAGE, DbgkExitProcess)
#pragma alloc_text(PAGE, DbgkMapViewOfSection)
#pragma alloc_text(PAGE, DbgkUnMapViewOfSection)
#endif

VOID
DbgkpSuspendProcess(
    IN BOOLEAN CreateDeleteLockHeld
    )

/*++

Routine Description:

    This function causes all threads in the calling process except for
    the calling thread to suspend.

Arguments:

    CreateDeleteLockHeld - Supplies a flag that specifies whether or not
        the caller is holding the process create delete lock.  If the
        caller holds the lock, than this function will not aquire the
        lock before suspending the process.

Return Value:

    None.

--*/

{
    PEPROCESS Process;

    PAGED_CODE();

    Process = PsGetCurrentProcess();

    //
    // Freeze the execution of all threads in the current process, but
    // the calling thread.
    //
    if ( !CreateDeleteLockHeld ) {
        PsLockProcess(Process,KernelMode,PsLockWaitForever);
        }

    KeFreezeAllThreads();

    if ( !CreateDeleteLockHeld ) {
        PsUnlockProcess(Process);
        }


    return;
}

VOID
DbgkpResumeProcess(
    IN BOOLEAN CreateDeleteLockHeld
    )

/*++

Routine Description:

    This function causes all threads in the calling process except for
    the calling thread to resume.

Arguments:

    CreateDeleteLockHeld - Supplies a flag that specifies whether or not
        the caller is holding the process create delete lock.  If the
        caller holds the lock, than this function will not aquire the
        lock before suspending the process.

Return Value:

    None.

--*/

{

    PEPROCESS Process;

    PAGED_CODE();

    Process = PsGetCurrentProcess();
    //
    // Thaw the execution of all threads in the current process, but
    // the calling thread.
    //

    if ( !CreateDeleteLockHeld ) {
        PsLockProcess(Process,KernelMode,PsLockWaitForever);
        }

    KeThawAllThreads();

    if ( !CreateDeleteLockHeld ) {
        PsUnlockProcess(Process);
        }

    return;
}

HANDLE
DbgkpSectionHandleToFileHandle(
    IN HANDLE SectionHandle
    )

/*++

Routine Description:

    This function Opens a handle to the file associated with the processes
    section. The file is opened such that it can be dupped all the way to
    the UI where the UI can either map the file or read the file to get
    the debug info.

Arguments:

    SectionHandle - Supplies a handle to the section whose associated file
        is to be opened.

Return Value:

    NULL - The file could not be opened.

    NON-NULL - Returns a handle to the file associated with the specified
        section.

--*/

{
    NTSTATUS Status;
    ANSI_STRING FileName;
    UNICODE_STRING UnicodeFileName;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;

    PAGED_CODE();

    Status = MmGetFileNameForSection(SectionHandle, (PSTRING)&FileName);
    if ( !NT_SUCCESS(Status) ) {
        return NULL;
        }

    Status = RtlAnsiStringToUnicodeString(&UnicodeFileName,&FileName,TRUE);
    ExFreePool(FileName.Buffer);
    if ( !NT_SUCCESS(Status) ) {
        return NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &UnicodeFileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = ZwOpenFile(
                &Handle,
                (ACCESS_MASK)(GENERIC_READ | SYNCHRONIZE),
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT
                );
    RtlFreeUnicodeString(&UnicodeFileName);
    if ( !NT_SUCCESS(Status) ) {
        return NULL;
        }
    else {
        return Handle;
        }
}


VOID
DbgkCreateThread(
    PVOID StartAddress
    )

/*++

Routine Description:

    This function is called when a new thread begins to execute. If the
    thread has an associated DebugPort, then a message is sent thru the
    port.

    If this thread is the first thread in the process, then this event
    is translated into a CreateProcessInfo message.

    If a message is sent, then while the thread is awaiting a reply,
    all other threads in the process are suspended.

Arguments:

    StartAddress - Supplies the start address for the thread that is
        starting.

Return Value:

    None.

--*/

{
    PVOID Port;
    DBGKM_APIMSG m;
    PDBGKM_CREATE_THREAD CreateThreadArgs;
    PDBGKM_CREATE_PROCESS CreateProcessArgs;
    PETHREAD Thread;
    PEPROCESS Process;
    PDBGKM_LOAD_DLL LoadDllArgs;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    PIMAGE_NT_HEADERS NtHeaders;

    PAGED_CODE();

    Process = PsGetCurrentProcess();

    Port = PsGetCurrentThread()->HideFromDebugger ? NULL : Process->DebugPort;

    if ( PsImageNotifyEnabled && !Process->Pcb.UserTime) {
        IMAGE_INFO ImageInfo;
        PIMAGE_NT_HEADERS NtHeaders;
        ANSI_STRING FileName;
        UNICODE_STRING UnicodeFileName;
        PUNICODE_STRING pUnicodeFileName;

        //
        // notification of main .exe
        //
        ImageInfo.Properties = 0;
        ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
        ImageInfo.ImageBase = Process->SectionBaseAddress;
        ImageInfo.ImageSize = 0;

        try {
            NtHeaders = RtlImageNtHeader(Process->SectionBaseAddress);
    
            if ( NtHeaders ) {
                ImageInfo.ImageSize = NtHeaders->OptionalHeader.SizeOfImage;
                }
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            ImageInfo.ImageSize = 0;
        }
        ImageInfo.ImageSelector = 0;
        ImageInfo.ImageSectionNumber = 0;

        pUnicodeFileName = NULL;
        Status = MmGetFileNameForSection(Process->SectionHandle, (PSTRING)&FileName);
        if ( NT_SUCCESS(Status) ) {
            Status = RtlAnsiStringToUnicodeString(&UnicodeFileName,&FileName,TRUE);
            ExFreePool(FileName.Buffer);
            if ( NT_SUCCESS(Status) ) {
                pUnicodeFileName = &UnicodeFileName;
                }
            }
        PsCallImageNotifyRoutines(
                    pUnicodeFileName,
                    Process->UniqueProcessId,
                    &ImageInfo
                    );
        if ( pUnicodeFileName ) {
            RtlFreeUnicodeString(pUnicodeFileName);
            }

        //
        // and of ntdll.dll
        //
        ImageInfo.Properties = 0;
        ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
        ImageInfo.ImageBase = PsSystemDllBase;
        ImageInfo.ImageSize = 0;

        try {
            NtHeaders = RtlImageNtHeader(PsSystemDllBase);
            if ( NtHeaders ) {
                ImageInfo.ImageSize = NtHeaders->OptionalHeader.SizeOfImage;
                }
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            ImageInfo.ImageSize = 0;
            }

        ImageInfo.ImageSelector = 0;
        ImageInfo.ImageSectionNumber = 0;

        RtlInitUnicodeString(&UnicodeFileName,L"\\SystemRoot\\System32\\ntdll.dll");
        PsCallImageNotifyRoutines(
                    &UnicodeFileName,
                    Process->UniqueProcessId,
                    &ImageInfo
                    );
        }


    if ( !Port ) {

        return;
    }

    Thread = PsGetCurrentThread();

    if ( Thread->DeadThread ) {
        return;
    }

    //
    // To determine if this should be turned into a create process,
    // all threads in the process are suspended. The process list
    // is then examined to see if the curent thread is the only thread
    // on the list. If so, this becomes a create process message
    //

    PsLockProcess(Process,KernelMode,PsLockWaitForever);

    //
    // If we are doing a debug attach, then the create process has
    // already occured. If this is the case, then the process has
    // accumulated some time, so set reported to true
    //

    if ( Process->Pcb.UserTime ) {
        Process->CreateProcessReported = TRUE;
        }

    if ( Process->CreateProcessReported == FALSE ) {

        //
        // This is a create process
        //

        Process->CreateProcessReported = TRUE;

        CreateThreadArgs = &m.u.CreateProcessInfo.InitialThread;
        CreateThreadArgs->SubSystemKey = 0;

        CreateProcessArgs = &m.u.CreateProcessInfo;
        CreateProcessArgs->SubSystemKey = 0;
        CreateProcessArgs->FileHandle = DbgkpSectionHandleToFileHandle(
                                            Process->SectionHandle
                                            );
        CreateProcessArgs->BaseOfImage = Process->SectionBaseAddress;
        CreateThreadArgs->StartAddress = NULL;
        CreateProcessArgs->DebugInfoFileOffset = 0;
        CreateProcessArgs->DebugInfoSize = 0;

        try {
            NtHeaders = RtlImageNtHeader(Process->SectionBaseAddress);
            if ( NtHeaders ) {
                CreateThreadArgs->StartAddress = (PVOID)(
                    NtHeaders->OptionalHeader.ImageBase +
                    NtHeaders->OptionalHeader.AddressOfEntryPoint);

                CreateProcessArgs->DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
                CreateProcessArgs->DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
                }
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            CreateThreadArgs->StartAddress = NULL;
            CreateProcessArgs->DebugInfoFileOffset = 0;
            CreateProcessArgs->DebugInfoSize = 0;
        }

        DBGKM_FORMAT_API_MSG(m,DbgKmCreateProcessApi,sizeof(*CreateProcessArgs));

        PsUnlockProcess(Process);

        DbgkpSendApiMessage(&m,Port,FALSE);
        ZwClose(CreateProcessArgs->FileHandle);

        LoadDllArgs = &m.u.LoadDll;
        LoadDllArgs->BaseOfDll = PsSystemDllBase;
        LoadDllArgs->DebugInfoFileOffset = 0;
        LoadDllArgs->DebugInfoSize = 0;

        try {
            NtHeaders = RtlImageNtHeader(PsSystemDllBase);
            if ( NtHeaders ) {
                LoadDllArgs->DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
                LoadDllArgs->DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
                }
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            LoadDllArgs->DebugInfoFileOffset = 0;
            LoadDllArgs->DebugInfoSize = 0;
        }

        //
        // Send load dll section for NT dll !
        //

        InitializeObjectAttributes(
            &Obja,
            &PsNtDllPathName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        Status = ZwOpenFile(
                    &LoadDllArgs->FileHandle,
                    (ACCESS_MASK)(GENERIC_READ | SYNCHRONIZE),
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_SYNCHRONOUS_IO_NONALERT
                    );

        if ( NT_SUCCESS(Status) ) {
            DBGKM_FORMAT_API_MSG(m,DbgKmLoadDllApi,sizeof(*LoadDllArgs));
            DbgkpSendApiMessage(&m,Port,TRUE);
        }
        ZwClose(LoadDllArgs->FileHandle);

    } else {

        CreateThreadArgs = &m.u.CreateThread;
        CreateThreadArgs->SubSystemKey = 0;
        CreateThreadArgs->StartAddress = StartAddress;

        DBGKM_FORMAT_API_MSG(m,DbgKmCreateThreadApi,sizeof(*CreateThreadArgs));

        PsUnlockProcess(Process);

        DbgkpSendApiMessage(&m,Port,TRUE);
    }


}

VOID
DbgkExitThread(
    NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function is called when a new thread terminates. At this
    point, the thread will no longer execute in user-mode. No other
    exit processing has occured.

    If a message is sent, then while the thread is awaiting a reply,
    all other threads in the process are suspended.

Arguments:

    ExitStatus - Supplies the ExitStatus of the exiting thread.

Return Value:

    None.

--*/

{
    PVOID Port;
    DBGKM_APIMSG m;
    PDBGKM_EXIT_THREAD args;
    PEPROCESS Process;

    PAGED_CODE();

    Process = PsGetCurrentProcess();
    Port = PsGetCurrentThread()->HideFromDebugger ? NULL : Process->DebugPort;

    if ( !Port ) {
        return;
    }

    if ( PsGetCurrentThread()->DeadThread ) {
        return;
    }

    args = &m.u.ExitThread;
    args->ExitStatus = ExitStatus;

    DBGKM_FORMAT_API_MSG(m,DbgKmExitThreadApi,sizeof(*args));

    DbgkpSuspendProcess(TRUE);

    DbgkpSendApiMessage(&m,Port,FALSE);

    DbgkpResumeProcess(TRUE);
}

VOID
DbgkExitProcess(
    NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function is called when a process terminates. The address
    space of the process is still intact, but no threads exist in
    the process.

Arguments:

    ExitStatus - Supplies the ExitStatus of the exiting process.

Return Value:

    None.

--*/

{
    PVOID Port;
    DBGKM_APIMSG m;
    PDBGKM_EXIT_PROCESS args;
    PEPROCESS Process;

    PAGED_CODE();

    Process = PsGetCurrentProcess();
    Port = PsGetCurrentThread()->HideFromDebugger ? NULL : Process->DebugPort;

    if ( !Port ) {
        return;
    }

    if ( PsGetCurrentThread()->DeadThread ) {
        return;
    }

    //
    // this ensures that other timed lockers of the process will bail
    // since this call is done while holding the process lock, and lock duration
    // is controlled by debugger
    //
    KeQuerySystemTime(&PsGetCurrentProcess()->ExitTime);

    args = &m.u.ExitProcess;
    args->ExitStatus = ExitStatus;

    DBGKM_FORMAT_API_MSG(m,DbgKmExitProcessApi,sizeof(*args));

    DbgkpSendApiMessage(&m,Port,FALSE);

}

VOID
DbgkMapViewOfSection(
    IN HANDLE SectionHandle,
    IN PVOID BaseAddress,
    IN ULONG SectionOffset,
    IN ULONG_PTR ViewSize
    )

/*++

Routine Description:

    This function is called when the current process successfully
    maps a view of an image section. If the process has an associated
    debug port, then a load dll message is sent.

Arguments:

    SectionHandle - Supplies a handle to the section mapped by the
        process.

    BaseAddress - Supplies the base address of where the section is
        mapped in the current process address space.

    SectionOffset - Supplies the offset in the section where the
        processes mapped view begins.

    ViewSize - Supplies the size of the mapped view.

Return Value:

    None.

--*/

{

    PVOID Port;
    DBGKM_APIMSG m;
    PDBGKM_LOAD_DLL LoadDllArgs;
    PEPROCESS Process;
    PIMAGE_NT_HEADERS NtHeaders;

    PAGED_CODE();

    Process = PsGetCurrentProcess();

    Port = PsGetCurrentThread()->HideFromDebugger ? NULL : Process->DebugPort;

    if ( !Port || KeGetPreviousMode() == KernelMode ) {
        return;
    }

    LoadDllArgs = &m.u.LoadDll;
    LoadDllArgs->FileHandle = DbgkpSectionHandleToFileHandle(SectionHandle);
    LoadDllArgs->BaseOfDll = BaseAddress;
    LoadDllArgs->DebugInfoFileOffset = 0;
    LoadDllArgs->DebugInfoSize = 0;

    try {
        NtHeaders = RtlImageNtHeader(BaseAddress);
        if ( NtHeaders ) {
            LoadDllArgs->DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
            LoadDllArgs->DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
            }
        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        LoadDllArgs->DebugInfoFileOffset = 0;
        LoadDllArgs->DebugInfoSize = 0;
    }

    DBGKM_FORMAT_API_MSG(m,DbgKmLoadDllApi,sizeof(*LoadDllArgs));

    DbgkpSendApiMessage(&m,Port,TRUE);
    ZwClose(LoadDllArgs->FileHandle);

}

VOID
DbgkUnMapViewOfSection(
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    This function is called when the current process successfully
    un maps a view of an image section. If the process has an associated
    debug port, then an "unmap view of section" message is sent.

Arguments:

    BaseAddress - Supplies the base address of the section being
        unmapped.

Return Value:

    None.

--*/

{

    PVOID Port;
    DBGKM_APIMSG m;
    PDBGKM_UNLOAD_DLL UnloadDllArgs;
    PEPROCESS Process;

    PAGED_CODE();

    Process = PsGetCurrentProcess();

    Port = PsGetCurrentThread()->HideFromDebugger ? NULL : Process->DebugPort;

    if ( !Port || KeGetPreviousMode() == KernelMode ) {
        return;
    }

    UnloadDllArgs = &m.u.UnloadDll;
    UnloadDllArgs->BaseAddress = BaseAddress;

    DBGKM_FORMAT_API_MSG(m,DbgKmUnloadDllApi,sizeof(*UnloadDllArgs));

    DbgkpSendApiMessage(&m,Port,TRUE);

}
