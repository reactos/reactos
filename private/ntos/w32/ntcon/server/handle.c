/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    handle.c

Abstract:

    This file manages console and io handles.

Author:

    Therese Stowell (thereses) 16-Nov-1990

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// array of pointers to consoles
//

PCONSOLE_INFORMATION  InitialConsoleHandles[CONSOLE_INITIAL_CONSOLES];
PCONSOLE_INFORMATION  *ConsoleHandles;
ULONG NumberOfConsoleHandles;

CRITICAL_SECTION ConsoleHandleLock; // serializes console handle table access

ULONG ConsoleId=47; // unique number identifying console

//
// Macros to manipulate console handles
//

#define HandleFromIndex(i)  ((HANDLE)((i & 0xFFFF) | (ConsoleId++ << 16)))
#define IndexFromHandle(h)  ((USHORT)((ULONG_PTR)h & 0xFFFF))
#define ConsoleHandleTableLocked() (ConsoleHandleLock.OwningThread == NtCurrentTeb()->ClientId.UniqueThread)

VOID
AddProcessToList(
    IN OUT PCONSOLE_INFORMATION Console,
    IN OUT PCONSOLE_PROCESS_HANDLE ProcessHandleRecord,
    IN HANDLE ProcessHandle
    );

VOID
FreeInputHandle(
    IN PHANDLE_DATA HandleData
    );

NTSTATUS
InitializeConsoleHandleTable( VOID )

/*++

Routine Description:

    This routine initializes the global console handle table.

Arguments:

    none.

Return Value:

    none.

--*/

{
    NTSTATUS Status;

    Status = RtlInitializeCriticalSectionAndSpinCount(&ConsoleHandleLock,
                                                      0x80000000);

    RtlZeroMemory(InitialConsoleHandles, sizeof(InitialConsoleHandles));
    ConsoleHandles = InitialConsoleHandles;
    NumberOfConsoleHandles = NELEM(InitialConsoleHandles);

    return Status;
}


#ifdef DEBUG

VOID
LockConsoleHandleTable( VOID )

/*++

Routine Description:

    This routine locks the global console handle table. It also verifies
    that we're not in the USER critical section. This is necessary to
    prevent potential deadlocks. This routine is only defined in debug
    builds.

Arguments:

    none.

Return Value:

    none.

--*/

{
    RtlEnterCriticalSection(&ConsoleHandleLock);
}


VOID
UnlockConsoleHandleTable( VOID )

/*++

Routine Description:

    This routine unlocks the global console handle table. This routine
    is only defined in debug builds.

Arguments:

    none.

Return Value:

    none.

--*/

{
    RtlLeaveCriticalSection(&ConsoleHandleLock);
}


VOID
LockConsole(
    IN PCONSOLE_INFORMATION Console
    )

/*++

Routine Description:

    This routine locks the console.This routine is only defined
    in debug builds.

Arguments:

    none.

Return Value:

    none.

--*/

{
    ASSERT(!ConsoleHandleTableLocked());
    RtlEnterCriticalSection(&(Console->ConsoleLock));
    ASSERT(ConsoleLocked(Console));
}

#endif // DEBUG


NTSTATUS
DereferenceConsoleHandle(
    IN HANDLE ConsoleHandle,
    OUT PCONSOLE_INFORMATION *Console
    )

/*++

Routine Description:

    This routine converts a console handle value into a pointer to the
    console data structure.

Arguments:

    ConsoleHandle - console handle to convert.

    Console - On output, contains pointer to the console data structure.

Return Value:

    none.

Note:

    The console handle table lock must be held when calling this routine.

--*/

{
    ULONG i;

    ASSERT(ConsoleHandleTableLocked());

    i = IndexFromHandle(ConsoleHandle);
    if ((i >= NumberOfConsoleHandles) ||
        ((*Console = ConsoleHandles[i]) == NULL) ||
        ((*Console)->ConsoleHandle != ConsoleHandle)) {
        *Console = NULL;
        return STATUS_INVALID_HANDLE;
    }
    if ((*Console)->Flags & CONSOLE_TERMINATING) {
        *Console = NULL;
        return STATUS_PROCESS_IS_TERMINATING;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
GrowConsoleHandleTable( VOID )

/*++

Routine Description:

    This routine grows the console handle table.

Arguments:

    none

Return Value:

--*/

{
    PCONSOLE_INFORMATION *NewTable;
    PCONSOLE_INFORMATION *OldTable;
    ULONG i;
    ULONG MaxConsoleHandles;

    ASSERT(ConsoleHandleTableLocked());

    MaxConsoleHandles = NumberOfConsoleHandles + CONSOLE_CONSOLE_HANDLE_INCREMENT;
    ASSERT(MaxConsoleHandles <= 0xFFFF);
    NewTable = (PCONSOLE_INFORMATION *)ConsoleHeapAlloc(MAKE_TAG( HANDLE_TAG ),MaxConsoleHandles * sizeof(PCONSOLE_INFORMATION));
    if (NewTable == NULL) {
        return STATUS_NO_MEMORY;
    }
    RtlCopyMemory(NewTable, ConsoleHandles,
                  NumberOfConsoleHandles * sizeof(PCONSOLE_INFORMATION));
    for (i=NumberOfConsoleHandles;i<MaxConsoleHandles;i++) {
        NewTable[i] = NULL;
    }
    OldTable = ConsoleHandles;
    ConsoleHandles = NewTable;
    NumberOfConsoleHandles = MaxConsoleHandles;
    if (OldTable != InitialConsoleHandles) {
        ConsoleHeapFree(OldTable);
    }
    return STATUS_SUCCESS;
}


NTSTATUS
AllocateConsoleHandle(
    OUT PHANDLE Handle
    )

/*++

Routine Description:

    This routine allocates a console handle from the global table.

Arguments:

    Handle - Pointer to store handle in.

Return Value:

Note:

    The console handle table lock must be held when calling this routine.

--*/

{
    ULONG i;
    NTSTATUS Status;

    ASSERT(ConsoleHandleTableLocked());

    //
    // have to start allocation at 1 because 0 indicates no console handle
    // in ConDllInitialize.
    //

    for (i=1;i<NumberOfConsoleHandles;i++) {
        if (ConsoleHandles[i] == NULL) {
            ConsoleHandles[i] = (PCONSOLE_INFORMATION) CONSOLE_HANDLE_ALLOCATED;
            *Handle = HandleFromIndex(i);
            return STATUS_SUCCESS;
        }
    }

    //
    // grow console handle table
    //

    Status = GrowConsoleHandleTable();
    if (!NT_SUCCESS(Status))
        return Status;
    for ( ;i<NumberOfConsoleHandles;i++) {
        if (ConsoleHandles[i] == NULL) {
            ConsoleHandles[i] = (PCONSOLE_INFORMATION) CONSOLE_HANDLE_ALLOCATED;
            *Handle = HandleFromIndex(i);
            return STATUS_SUCCESS;
        }
    }
    ASSERT (FALSE);
    return STATUS_UNSUCCESSFUL;
}



NTSTATUS
FreeConsoleHandle(
    IN HANDLE Handle
    )

/*++

Routine Description:

    This routine frees a console handle from the global table.

Arguments:

    Handle - Handle to free.

Return Value:

Note:

    The console handle table lock must be held when calling this routine.

--*/

{
    ULONG i;

    ASSERT(ConsoleHandleTableLocked());

    ASSERT (Handle != NULL);
    i = IndexFromHandle(Handle);
    if ((i >= NumberOfConsoleHandles) || (ConsoleHandles[i] == NULL)) {
        ASSERT (FALSE);
    } else {
        ConsoleHandles[i] = NULL;
    }
    return STATUS_SUCCESS;
}


NTSTATUS
ValidateConsole(
    IN PCONSOLE_INFORMATION Console
    )

/*++

Routine Description:

    This routine ensures that the given console pointer is valid.

Arguments:

    Console - Console pointer to validate.

--*/

{
    ULONG i;

    if (Console != NULL) {
        for (i = 0; i < NumberOfConsoleHandles; i++) {
            if (ConsoleHandles[i] == Console)
                return STATUS_SUCCESS;
        }
    }
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
InitializeIoHandleTable(
    IN OUT PCONSOLE_INFORMATION Console,
    OUT PCONSOLE_PER_PROCESS_DATA ProcessData,
    OUT PHANDLE StdIn,
    OUT PHANDLE StdOut,
    OUT PHANDLE StdErr
    )

/*++

Routine Description:

    This routine initializes a process's handle table for the first
    time (there is no parent process).  It also sets up stdin, stdout,
    and stderr.

Arguments:

    Console - Pointer to console information structure.

    ProcessData - Pointer to per process data structure.

    Stdin - Pointer in which to return StdIn handle.

    StdOut - Pointer in which to return StdOut handle.

    StdErr - Pointer in which to return StdErr handle.

Return Value:

--*/

{
    ULONG i;
    HANDLE Handle;
    NTSTATUS Status;
    PHANDLE_DATA HandleData;

    // HandleTablePtr gets set up by ConsoleAddProcessRoutine.
    // it will be != to HandleTable if the new process was created
    // using "start xxx" at the command line and cmd.exe has >
    // CONSOLE_INITIAL_IO_HANDLES.

    if (ProcessData->HandleTablePtr != ProcessData->HandleTable) {
        ASSERT(ProcessData->HandleTableSize != CONSOLE_INITIAL_IO_HANDLES);
        ConsoleHeapFree(ProcessData->HandleTablePtr);
        ProcessData->HandleTablePtr = ProcessData->HandleTable;
    }

    for (i=0;i<CONSOLE_INITIAL_IO_HANDLES;i++) {
        ProcessData->HandleTable[i].HandleType = CONSOLE_FREE_HANDLE;
    }

    ProcessData->HandleTableSize = CONSOLE_INITIAL_IO_HANDLES;
    ProcessData->Foo = 0xF00;

    //
    // set up stdin, stdout, stderr.  we don't do any cleanup in case
    // of errors because we're going to fail the console creation.
    //
    // stdin
    //

    Status = AllocateIoHandle(ProcessData,
                              CONSOLE_INPUT_HANDLE,
                              &Handle
                             );
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }
    Status = DereferenceIoHandleNoCheck(ProcessData,
                                 Handle,
                                 &HandleData
                                );
    ASSERT (NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }
    if (!InitializeInputHandle(HandleData,
                               &Console->InputBuffer)) {
        return STATUS_NO_MEMORY;
    }
    HandleData->HandleType |= CONSOLE_INHERITABLE;
    Status = ConsoleAddShare(GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             &Console->InputBuffer.ShareAccess,
                             HandleData
                            );
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }
    *StdIn = INDEX_TO_HANDLE(Handle);

    //
    // stdout
    //

    Status = AllocateIoHandle(ProcessData,
                              CONSOLE_OUTPUT_HANDLE,
                              &Handle
                             );
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }
    Status = DereferenceIoHandleNoCheck(ProcessData,
                                 Handle,
                                 &HandleData
                                );
    ASSERT (NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }
    InitializeOutputHandle(HandleData,Console->CurrentScreenBuffer);
    HandleData->HandleType |= CONSOLE_INHERITABLE;
    Status = ConsoleAddShare(GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             &Console->ScreenBuffers->ShareAccess,
                             HandleData
                            );
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }
    *StdOut = INDEX_TO_HANDLE(Handle);

    //
    // stderr
    //

    Status = AllocateIoHandle(ProcessData,
                              CONSOLE_OUTPUT_HANDLE,
                              &Handle
                             );
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }
    Status = DereferenceIoHandleNoCheck(ProcessData,
                                 Handle,
                                 &HandleData
                                );
    ASSERT (NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }
    InitializeOutputHandle(HandleData,Console->CurrentScreenBuffer);
    HandleData->HandleType |= CONSOLE_INHERITABLE;
    Status = ConsoleAddShare(GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             &Console->ScreenBuffers->ShareAccess,
                             HandleData
                            );
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }
    *StdErr = INDEX_TO_HANDLE(Handle);
    return STATUS_SUCCESS;
}

NTSTATUS
InheritIoHandleTable(
    IN PCONSOLE_INFORMATION Console,
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN PCONSOLE_PER_PROCESS_DATA ParentProcessData
    )

/*++

Routine Description:

    This routine creates a process's handle table from the parent
    process's handle table.  ProcessData contains the process data
    copied directly from the parent to the child process by CSR.
    This routine allocates a new handle table, if necessary, then
    invalidates non-inherited handles and increments the sharing
    and reference counts for inherited handles.

Arguments:

    ProcessData - Pointer to per process data structure.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    ULONG i;
    NTSTATUS Status;

    //
    // Copy handles from parent process.  If the table size
    // is CONSOLE_INITIAL_IO_HANDLES, CSR has done the copy
    // for us.
    //

    UNREFERENCED_PARAMETER(Console);

    ASSERT(ParentProcessData->Foo == 0xF00);
    ASSERT(ParentProcessData->HandleTableSize != 0);
    ASSERT(ParentProcessData->HandleTableSize <= 0x0000FFFF);

    if (ParentProcessData->HandleTableSize != CONSOLE_INITIAL_IO_HANDLES) {
        ProcessData->HandleTableSize = ParentProcessData->HandleTableSize;
        ProcessData->HandleTablePtr = (PHANDLE_DATA)ConsoleHeapAlloc(MAKE_TAG( HANDLE_TAG ),ProcessData->HandleTableSize * sizeof(HANDLE_DATA));

        if (ProcessData->HandleTablePtr == NULL) {
            ProcessData->HandleTablePtr = ProcessData->HandleTable;
            ProcessData->HandleTableSize = CONSOLE_INITIAL_IO_HANDLES;
            return STATUS_NO_MEMORY;
        }
        RtlCopyMemory(ProcessData->HandleTablePtr,
            ParentProcessData->HandleTablePtr,
            ProcessData->HandleTableSize * sizeof(HANDLE_DATA));
    }

    ASSERT(!(Console->Flags & CONSOLE_SHUTTING_DOWN));

    //
    // Allocate any memory associated with each handle.
    //

    Status = STATUS_SUCCESS;
    for (i=0;i<ProcessData->HandleTableSize;i++) {

        if (NT_SUCCESS(Status) && ProcessData->HandleTablePtr[i].HandleType & CONSOLE_INHERITABLE) {

            if (ProcessData->HandleTablePtr[i].HandleType & CONSOLE_INPUT_HANDLE) {
                ProcessData->HandleTablePtr[i].InputReadData = (PINPUT_READ_HANDLE_DATA)ConsoleHeapAlloc(MAKE_TAG( HANDLE_TAG ),sizeof(INPUT_READ_HANDLE_DATA));
                if (!ProcessData->HandleTablePtr[i].InputReadData) {
                    ProcessData->HandleTablePtr[i].HandleType = CONSOLE_FREE_HANDLE;
                    Status = STATUS_NO_MEMORY;
                    continue;
                }
                ProcessData->HandleTablePtr[i].InputReadData->InputHandleFlags = 0;
                ProcessData->HandleTablePtr[i].InputReadData->ReadCount = 0;
                Status = RtlInitializeCriticalSection(&ProcessData->HandleTablePtr[i].InputReadData->ReadCountLock);
                if (!NT_SUCCESS(Status)) {
                    ConsoleHeapFree(ProcessData->HandleTablePtr[i].InputReadData);
                    ProcessData->HandleTablePtr[i].InputReadData = NULL;
                    ProcessData->HandleTablePtr[i].HandleType = CONSOLE_FREE_HANDLE;
                    continue;
                }
            }
        }
        else {
            ProcessData->HandleTablePtr[i].HandleType = CONSOLE_FREE_HANDLE;
        }
    }

    //
    // If something failed, we need to free any input data we allocated and
    // free the handle table.
    //

    if (!NT_SUCCESS(Status)) {
        for (i=0;i<ProcessData->HandleTableSize;i++) {
            if (ProcessData->HandleTablePtr[i].HandleType & CONSOLE_INPUT_HANDLE) {
                FreeInputHandle(&ProcessData->HandleTablePtr[i]);
            }
        }
        if (ProcessData->HandleTableSize != CONSOLE_INITIAL_IO_HANDLES) {
            ConsoleHeapFree(ProcessData->HandleTablePtr);
            ProcessData->HandleTablePtr = ProcessData->HandleTable;
            ProcessData->HandleTableSize = CONSOLE_INITIAL_IO_HANDLES;
        }
        return Status;
    }

    //
    // All the memory allocations succeeded. Now go through and increment the
    // object reference counts and dup the shares.
    //

    for (i=0;i<ProcessData->HandleTableSize;i++) {
        if (ProcessData->HandleTablePtr[i].HandleType != CONSOLE_FREE_HANDLE) {
            PCONSOLE_SHARE_ACCESS ShareAccess;

            if (ProcessData->HandleTablePtr[i].HandleType & CONSOLE_INPUT_HANDLE) {
                ProcessData->HandleTablePtr[i].Buffer.InputBuffer->RefCount++;
                ShareAccess = &ProcessData->HandleTablePtr[i].Buffer.InputBuffer->ShareAccess;
            }
            else {
                ProcessData->HandleTablePtr[i].Buffer.ScreenBuffer->RefCount++;
                ShareAccess = &ProcessData->HandleTablePtr[i].Buffer.ScreenBuffer->ShareAccess;
            }

            Status = ConsoleDupShare(ProcessData->HandleTablePtr[i].Access,
                                     ProcessData->HandleTablePtr[i].ShareAccess,
                                     ShareAccess,
                                     &ProcessData->HandleTablePtr[i]
                                    );
            ASSERT (NT_SUCCESS(Status));
        }
    }
    ASSERT(ProcessData->Foo == 0xF00);

    return STATUS_SUCCESS;
}

NTSTATUS
ConsoleAddProcessRoutine(
    IN PCSR_PROCESS ParentProcess,
    IN PCSR_PROCESS Process
    )
{
    PCONSOLE_PER_PROCESS_DATA ProcessData, ParentProcessData;
    PCONSOLE_INFORMATION Console;
    PCONSOLE_PROCESS_HANDLE ProcessHandleRecord;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    ProcessData = CONSOLE_FROMPROCESSPERPROCESSDATA(Process);
    ProcessData->HandleTablePtr = ProcessData->HandleTable;
    ProcessData->HandleTableSize = CONSOLE_INITIAL_IO_HANDLES;
    CONSOLE_SETCONSOLEAPPFROMPROCESSDATA(ProcessData,FALSE);

    if (ParentProcess) {

        ProcessData->RootProcess = FALSE;
        ParentProcessData = CONSOLE_FROMPROCESSPERPROCESSDATA(ParentProcess);

        //
        // If both the parent and new processes are console apps,
        // inherit handles from the parent process.
        //

        if (ParentProcessData->ConsoleHandle != NULL &&
                (Process->Flags & CSR_PROCESS_CONSOLEAPP)) {
            if (!(NT_SUCCESS(RevalidateConsole(ParentProcessData->ConsoleHandle,
                                               &Console)))) {
                ProcessData->ConsoleHandle = NULL;
                return STATUS_PROCESS_IS_TERMINATING;
            }

            //
            // Don't add the process if the console is being shutdown.
            //

            if (Console->Flags & CONSOLE_SHUTTING_DOWN) {
                Status = STATUS_PROCESS_IS_TERMINATING;
            } else {
                ProcessHandleRecord = ConsoleHeapAlloc(MAKE_TAG( HANDLE_TAG ),sizeof(CONSOLE_PROCESS_HANDLE));
                if (ProcessHandleRecord == NULL) {
                    Status = STATUS_NO_MEMORY;
                } else {

                    //
                    // duplicate parent's handle table
                    //

                    ASSERT(ProcessData->Foo == 0xF00);
                    Status = InheritIoHandleTable(Console, ProcessData, ParentProcessData);
                    if (NT_SUCCESS(Status)) {
                        ProcessHandleRecord->Process = Process;
                        ProcessHandleRecord->CtrlRoutine = NULL;
                        ProcessHandleRecord->PropRoutine = NULL;
                        AddProcessToList(Console,ProcessHandleRecord,Process->ProcessHandle);

                        //
                        // increment console reference count
                        //

                        Console->RefCount++;
                    } else {
                        ConsoleHeapFree(ProcessHandleRecord);
                    }
                }
            }
            if (!NT_SUCCESS(Status)) {
                ProcessData->ConsoleHandle = NULL;
                for (i=0;i<CONSOLE_INITIAL_IO_HANDLES;i++) {
                    ProcessData->HandleTable[i].HandleType = CONSOLE_FREE_HANDLE;
                }
            }
            UnlockConsole(Console);
        } else
            ProcessData->ConsoleHandle = NULL;
    } else {
        ProcessData->ConsoleHandle = NULL;
    }
    return Status;
}

NTSTATUS
AllocateConsole(
    IN HANDLE ConsoleHandle,
    IN LPWSTR Title,
    IN USHORT TitleLength,
    IN HANDLE ClientProcessHandle,
    OUT PHANDLE StdIn,
    OUT PHANDLE StdOut,
    OUT PHANDLE StdErr,
    OUT PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN OUT PCONSOLE_INFO ConsoleInfo,
    IN BOOLEAN WindowVisible,
    IN DWORD dwConsoleThreadId
    )

/*++

Routine Description:

    This routine allocates and initialized a console and its associated
    data - input buffer and screen buffer.

Arguments:

    ConsoleHandle - Handle of console to allocate.

    dwWindowSize - Initial size of screen buffer window, in rows and columns.

    nFont - Initial number of font text is displayed in.

    dwScreenBufferSize - Initial size of screen buffer, in rows and columns.

    nInputBufferSize - Initial size of input buffer, in events.

    dwWindowFlags -

    StdIn - On return, contains handle to stdin.

    StdOut - On return, contains handle to stdout.

    StdErr - On return, contains handle to stderr.

    ProcessData - On return, contains the initialized per-process data.

Return Value:

Note:

    The console handle table lock must be held when calling this routine.

--*/

{
    PCONSOLE_INFORMATION Console;
    NTSTATUS Status;
    BOOL Success;

    //
    // allocate console data
    //

    Console = (PCONSOLE_INFORMATION)ConsoleHeapAlloc( MAKE_TAG( CONSOLE_TAG ) | HEAP_ZERO_MEMORY,
                                              sizeof(CONSOLE_INFORMATION));
    if (Console == NULL) {
        return STATUS_NO_MEMORY;
    }
    ConsoleHandles[IndexFromHandle(ConsoleHandle)] = Console;

    Console->Flags = WindowVisible ? 0 : CONSOLE_NO_WINDOW;
    Console->hIcon = ConsoleInfo->hIcon;
    Console->hSmIcon = ConsoleInfo->hSmIcon;
    Console->iIconId = ConsoleInfo->iIconId;
    Console->dwHotKey = ConsoleInfo->dwHotKey;
#if !defined(FE_SB)
    Console->CP = OEMCP;
    Console->OutputCP = ConsoleOutputCP;
#endif
    Console->ReserveKeys = CONSOLE_NOSHORTCUTKEY;
    Console->ConsoleHandle = ConsoleHandle;
    Console->bIconInit = TRUE;
    Console->VerticalClientToWindow = VerticalClientToWindow;
    Console->HorizontalClientToWindow = HorizontalClientToWindow;
#if defined(FE_SB)
    SetConsoleCPInfo(Console,TRUE);
    SetConsoleCPInfo(Console,FALSE);
#endif

    //
    // must wait for window to be destroyed or client impersonation won't
    // work.
    //

    Status = NtDuplicateObject(NtCurrentProcess(),
                              CONSOLE_CLIENTTHREADHANDLE(CSR_SERVER_QUERYCLIENTTHREAD()),
                              NtCurrentProcess(),
                              &Console->ClientThreadHandle,
                              0,
                              FALSE,
                              DUPLICATE_SAME_ACCESS
                             );
    if (!NT_SUCCESS(Status)) {
        goto ErrorExit5;
    }

#if DBG
    //
    // Make sure the handle isn't protected so we can close it later
    //
    UnProtectHandle(Console->ClientThreadHandle);
#endif // DBG

    InitializeListHead(&Console->OutputQueue);
    InitializeListHead(&Console->ProcessHandleList);
    InitializeListHead(&Console->ExeAliasList);
    InitializeListHead(&Console->MessageQueue);

    Status = NtCreateEvent(&Console->InitEvents[INITIALIZATION_SUCCEEDED],
                           EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status)) {
        goto ErrorExit4a;
    }
    Status = NtCreateEvent(&Console->InitEvents[INITIALIZATION_FAILED],
                           EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status)) {
        goto ErrorExit4;
    }
    Status = RtlInitializeCriticalSection(&Console->ConsoleLock);
    if (!NT_SUCCESS(Status)) {
        goto ErrorExit3a;
    }
    InitializeConsoleCommandData(Console);

    //
    // initialize input buffer
    //

#if defined(FE_SB)
    Status = CreateInputBuffer(ConsoleInfo->nInputBufferSize,
                               &Console->InputBuffer,
                               Console);
#else
    Status = CreateInputBuffer(ConsoleInfo->nInputBufferSize,
                               &Console->InputBuffer);
#endif
    if (!NT_SUCCESS(Status)) {
        goto ErrorExit3;
    }

    Console->Title = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( TITLE_TAG ),TitleLength+sizeof(WCHAR));
    if (Console->Title == NULL) {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit2;
    }
    RtlCopyMemory(Console->Title,Title,TitleLength);
    Console->Title[TitleLength/sizeof(WCHAR)] = (WCHAR)0;   // NULL terminate
    Console->TitleLength = TitleLength;

    Console->OriginalTitle = TranslateConsoleTitle(Console->Title, &Console->OriginalTitleLength, TRUE, FALSE);
    if (Console->OriginalTitle == NULL) {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit1;
    }

    Status = NtCreateEvent(&Console->TerminationEvent,
                           EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status)) {
        goto ErrorExit1a;
    }

    //
    // initialize screen buffer. we don't call OpenConsole to do this
    // because we need to specify the font, windowsize, etc.
    //

    Status = DoCreateScreenBuffer(Console,
                                  ConsoleInfo);
    if (!NT_SUCCESS(Status)){
        goto ErrorExit1b;
    }


    Console->CurrentScreenBuffer = Console->ScreenBuffers;
#if defined(FE_SB)
#if defined(FE_IME)
    SetUndetermineAttribute(Console) ;
#endif
    Status = CreateEUDC(Console);
    if (!NT_SUCCESS(Status)){
        goto ErrorExit1c;
    }
#endif
    Status = InitializeIoHandleTable(Console,
                                     ProcessData,
                                     StdIn,
                                     StdOut,
                                     StdErr
                                    );
    if (!NT_SUCCESS(Status)) {
        goto ErrorExit0;
    }

    //
    // map event handles
    //

    if (!MapHandle(ClientProcessHandle,
                   Console->InitEvents[INITIALIZATION_SUCCEEDED],
                   &ConsoleInfo->InitEvents[INITIALIZATION_SUCCEEDED]
                  )) {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit0;
    }
    if (!MapHandle(ClientProcessHandle,
                   Console->InitEvents[INITIALIZATION_FAILED],
                   &ConsoleInfo->InitEvents[INITIALIZATION_FAILED]
                  )) {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit0;
    }
    if (!MapHandle(ClientProcessHandle,
                   Console->InputBuffer.InputWaitEvent,
                   &ConsoleInfo->InputWaitHandle
                  )) {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit0;
    }

    Success = PostThreadMessage(dwConsoleThreadId,
                                CM_CREATE_CONSOLE_WINDOW,
                                (WPARAM)ConsoleHandle,
                                (LPARAM)ClientProcessHandle
                               );
    if (!Success) {
        KdPrint(("CONSRV: PostThreadMessage failed %d\n",GetLastError()));
        Status = STATUS_UNSUCCESSFUL;
        goto ErrorExit0;
    }

    return STATUS_SUCCESS;

ErrorExit0: Console->ScreenBuffers->RefCount = 0;
#if defined(FE_SB)
            if (Console->EudcInformation != NULL) {
                ConsoleHeapFree(Console->EudcInformation);
            }
ErrorExit1c:
#endif
            FreeScreenBuffer(Console->ScreenBuffers);
ErrorExit1b: NtClose(Console->TerminationEvent);
ErrorExit1a: ConsoleHeapFree(Console->OriginalTitle);
ErrorExit1: ConsoleHeapFree(Console->Title);
ErrorExit2: Console->InputBuffer.RefCount = 0;
            FreeInputBuffer(&Console->InputBuffer);
ErrorExit3: RtlDeleteCriticalSection(&Console->ConsoleLock);

ErrorExit3a: NtClose(Console->InitEvents[INITIALIZATION_FAILED]);
ErrorExit4: NtClose(Console->InitEvents[INITIALIZATION_SUCCEEDED]);
ErrorExit4a: NtClose(Console->ClientThreadHandle);
ErrorExit5:  ConsoleHeapFree(Console);
    return Status;
}

VOID
DestroyConsole(
    IN PCONSOLE_INFORMATION Console
    )

/*++

Routine Description:

    This routine frees a console structure if it's not being referenced.

Arguments:

    Console - Console to free.

Return Value:


--*/

{
    HANDLE ConsoleHandle = Console->ConsoleHandle;

    //
    // Make sure we have the console locked and it really is going away.
    //

    ASSERT(ConsoleLocked(Console));
    ASSERT(Console->hWnd == NULL);

    //
    // Mark this console as being destroyed.
    //

    Console->Flags |= CONSOLE_IN_DESTRUCTION;

    //
    // Unlock this console.
    //

    RtlLeaveCriticalSection(&Console->ConsoleLock);

    //
    // If the console still exists and no one is waiting on it, free it.
    //

    LockConsoleHandleTable();
    if (Console == ConsoleHandles[IndexFromHandle(ConsoleHandle)] &&
        Console->ConsoleHandle == ConsoleHandle &&
        Console->ConsoleLock.OwningThread == NULL &&
        Console->WaitCount == 0) {

        FreeConsoleHandle(ConsoleHandle);
        RtlDeleteCriticalSection(&Console->ConsoleLock);
        ConsoleHeapFree(Console);
    }
    UnlockConsoleHandleTable();
}

VOID
FreeCon(
    IN PCONSOLE_INFORMATION Console
    )

/*++

Routine Description:

    This routine frees a console and its associated
    data - input buffer and screen buffer.

Arguments:

    ConsoleHandle - Handle of console to free.

Return Value:

Note:

    The console handle table lock must be held when calling this routine.

--*/

{
    HWND hWnd;

    Console->Flags |= CONSOLE_TERMINATING;
    NtSetEvent(Console->TerminationEvent,NULL);
    hWnd = Console->hWnd;

    //
    // Wait 10 seconds or until the input thread replies
    // to synchronize the window destruction with
    // the termination of the thread
    //

    if (hWnd != NULL) {
        UnlockConsole(Console);
        SendMessageTimeout(hWnd, CM_DESTROY_WINDOW, 0, 0, SMTO_BLOCK, 10000, NULL);
    } else {
        AbortCreateConsole(Console);
    }
}

VOID
InsertScreenBuffer(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    )

/*++

Routine Description:

    This routine inserts the screen buffer pointer into the console's
    list of screen buffers.

Arguments:

    Console - Pointer to console information structure.

    ScreenInfo - Pointer to screen information structure.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    ScreenInfo->Next = Console->ScreenBuffers;
    Console->ScreenBuffers = ScreenInfo;
}

VOID
RemoveScreenBuffer(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    )

/*++

Routine Description:

    This routine removes the screen buffer pointer from the console's
    list of screen buffers.

Arguments:

    Console - Pointer to console information structure.

    ScreenInfo - Pointer to screen information structure.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    PSCREEN_INFORMATION Prev,Cur;

    if (ScreenInfo == Console->ScreenBuffers) {
        Console->ScreenBuffers = ScreenInfo->Next;
        return;
    }
    Prev = Cur = Console->ScreenBuffers;
    while (Cur != NULL) {
        if (ScreenInfo == Cur)
            break;
        Prev = Cur;
        Cur = Cur->Next;
    }
    ASSERT (Cur != NULL);
    if (Cur != NULL) {
        Prev->Next = Cur->Next;
    }
}

NTSTATUS
GrowIoHandleTable(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData
    )

/*++

Routine Description:

    This routine grows the per-process io handle table.

Arguments:

    ProcessData - Pointer to the per-process data structure.

Return Value:

--*/

{
    PHANDLE_DATA NewTable;
    ULONG i;
    ULONG MaxFileHandles;

    ASSERT(ProcessData->Foo == 0xF00);
    MaxFileHandles = ProcessData->HandleTableSize + CONSOLE_IO_HANDLE_INCREMENT;
    NewTable = (PHANDLE_DATA)ConsoleHeapAlloc(MAKE_TAG( HANDLE_TAG ),MaxFileHandles * sizeof(HANDLE_DATA));
    if (NewTable == NULL) {
        return STATUS_NO_MEMORY;
    }
    RtlCopyMemory(NewTable, ProcessData->HandleTablePtr,
                  ProcessData->HandleTableSize * sizeof(HANDLE_DATA));
    for (i=ProcessData->HandleTableSize;i<MaxFileHandles;i++) {
        NewTable[i].HandleType = CONSOLE_FREE_HANDLE;
    }
    if (ProcessData->HandleTableSize != CONSOLE_INITIAL_IO_HANDLES) {
        ConsoleHeapFree(ProcessData->HandleTablePtr);
    }
    ProcessData->HandleTablePtr = NewTable;
    ProcessData->HandleTableSize = MaxFileHandles;
    ASSERT(ProcessData->Foo == 0xF00);
    ASSERT(ProcessData->HandleTableSize != 0);
    ASSERT(ProcessData->HandleTableSize <= 0x0000FFFF);
    return STATUS_SUCCESS;
}

VOID
FreeProcessData(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData
    )

/*++

Routine Description:

    This routine frees any per-process data allocated by the console.

Arguments:

    ProcessData - Pointer to the per-process data structure.

Return Value:

--*/

{
    if (ProcessData->HandleTableSize != CONSOLE_INITIAL_IO_HANDLES) {
        ConsoleHeapFree(ProcessData->HandleTablePtr);
        ProcessData->HandleTablePtr = ProcessData->HandleTable;
        ProcessData->HandleTableSize = CONSOLE_INITIAL_IO_HANDLES;
    }
}

VOID
InitializeOutputHandle(
    PHANDLE_DATA HandleData,
    PSCREEN_INFORMATION ScreenBuffer
    )

/*++

Routine Description:

    This routine initializes the output-specific fields of the handle data
    structure.

Arguments:

    HandleData - Pointer to handle data structure.

    ScreenBuffer - Pointer to screen buffer data structure.

Return Value:

--*/

{
    HandleData->Buffer.ScreenBuffer = ScreenBuffer;
    HandleData->Buffer.ScreenBuffer->RefCount++;
}

BOOLEAN
InitializeInputHandle(
    PHANDLE_DATA HandleData,
    PINPUT_INFORMATION InputBuffer
    )

/*++

Routine Description:

    This routine initializes the input-specific fields of the handle data
    structure.

Arguments:

    HandleData - Pointer to handle data structure.

    InputBuffer - Pointer to input buffer data structure.

Return Value:

--*/

{
    NTSTATUS Status;

    HandleData->InputReadData = (PINPUT_READ_HANDLE_DATA)ConsoleHeapAlloc(MAKE_TAG( HANDLE_TAG ),sizeof(INPUT_READ_HANDLE_DATA));
    if (!HandleData->InputReadData) {
        return FALSE;
    }
    Status = RtlInitializeCriticalSection(&HandleData->InputReadData->ReadCountLock);
    if (!NT_SUCCESS(Status)) {
        ConsoleHeapFree(HandleData->InputReadData);
        HandleData->InputReadData = NULL;
        return FALSE;
    }
    HandleData->InputReadData->ReadCount = 0;
    HandleData->InputReadData->InputHandleFlags = 0;
    HandleData->Buffer.InputBuffer = InputBuffer;
    HandleData->Buffer.InputBuffer->RefCount++;
    return TRUE;
}

VOID
FreeInputHandle(
    IN PHANDLE_DATA HandleData
    )
{
    if (HandleData->InputReadData) {
        RtlDeleteCriticalSection(&HandleData->InputReadData->ReadCountLock);
        ConsoleHeapFree(HandleData->InputReadData);
        HandleData->InputReadData = NULL;
    }
}

NTSTATUS
AllocateIoHandle(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN ULONG HandleType,
    OUT PHANDLE Handle
    )

/*++

Routine Description:

    This routine allocates an input or output handle from the process's
    handle table.

    This routine initializes all non-type specific fields in the handle
    data structure.

Arguments:

    ProcessData - Pointer to per process data structure.

    HandleType - Flag indicating input or output handle.

    Handle - On return, contains allocated handle.  Handle is an index
    internally.  When returned to the API caller, it is translated into
    a handle.

Return Value:

Note:

    The console lock must be held when calling this routine.  The handle
    is allocated from the per-process handle table.  Holding the console
    lock serializes both threads within the calling process and any other
    process that shares the console.

--*/

{
    ULONG i;
    NTSTATUS Status;

    for (i=0;i<ProcessData->HandleTableSize;i++) {
        if (ProcessData->HandleTablePtr[i].HandleType == CONSOLE_FREE_HANDLE) {
            ProcessData->HandleTablePtr[i].HandleType = HandleType;
            *Handle = (HANDLE) i;

            return STATUS_SUCCESS;
        }
    }
    Status = GrowIoHandleTable(ProcessData);
    if (!NT_SUCCESS(Status))
        return Status;
    for ( ;i<ProcessData->HandleTableSize;i++) {
        if (ProcessData->HandleTablePtr[i].HandleType == CONSOLE_FREE_HANDLE) {
            ProcessData->HandleTablePtr[i].HandleType = HandleType;
            *Handle = (HANDLE) i;
            return STATUS_SUCCESS;
        }
    }
    ASSERT (FALSE);
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
FreeIoHandle(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN HANDLE Handle
    )

/*++

Routine Description:

    This routine frees an input or output handle from the process's
    handle table.

Arguments:

    ProcessData - Pointer to per process data structure.

    Handle - Handle to free.

Return Value:

Note:

    The console lock must be held when calling this routine.  The handle
    is freed from the per-process handle table.  Holding the console
    lock serializes both threads within the calling process and any other
    process that shares the console.

--*/

{
    NTSTATUS Status;
    PHANDLE_DATA HandleData;

    Status = DereferenceIoHandleNoCheck(ProcessData,
                                 Handle,
                                 &HandleData
                                );
    ASSERT (NT_SUCCESS(Status));
    if (HandleData->HandleType & CONSOLE_INPUT_HANDLE) {
        FreeInputHandle(HandleData);
    }
    HandleData->HandleType = CONSOLE_FREE_HANDLE;
    return STATUS_SUCCESS;
}

NTSTATUS
DereferenceIoHandleNoCheck(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN HANDLE Handle,
    OUT PHANDLE_DATA *HandleData
    )

/*++

Routine Description:

    This routine verifies a handle's validity, then returns a pointer to
    the handle data structure.

Arguments:

    ProcessData - Pointer to per process data structure.

    Handle - Handle to dereference.

    HandleData - On return, pointer to handle data structure.

Return Value:

--*/

{
    if (((ULONG_PTR)Handle >= ProcessData->HandleTableSize) ||
        (ProcessData->HandleTablePtr[(ULONG_PTR)Handle].HandleType == CONSOLE_FREE_HANDLE) ) {
        return STATUS_INVALID_HANDLE;
    }
    *HandleData = &ProcessData->HandleTablePtr[(ULONG_PTR)Handle];
    return STATUS_SUCCESS;
}

NTSTATUS
DereferenceIoHandle(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN HANDLE Handle,
    IN ULONG HandleType,
    IN ACCESS_MASK Access,
    OUT PHANDLE_DATA *HandleData
    )

/*++

Routine Description:

    This routine verifies a handle's validity, then returns a pointer to
    the handle data structure.

Arguments:

    ProcessData - Pointer to per process data structure.

    Handle - Handle to dereference.

    HandleData - On return, pointer to handle data structure.

Return Value:

--*/

{
    ULONG_PTR Index;

    if (!CONSOLE_HANDLE(Handle)) {
        return STATUS_INVALID_HANDLE;
    }
    Index = (ULONG_PTR)HANDLE_TO_INDEX(Handle);
    if ((Index >= ProcessData->HandleTableSize) ||
        (ProcessData->HandleTablePtr[Index].HandleType == CONSOLE_FREE_HANDLE) ||
        !(ProcessData->HandleTablePtr[Index].HandleType & HandleType) ||
        !(ProcessData->HandleTablePtr[Index].Access & Access) ) {
        return STATUS_INVALID_HANDLE;
    }
    *HandleData = &ProcessData->HandleTablePtr[Index];
    return STATUS_SUCCESS;
}


ULONG
SrvVerifyConsoleIoHandle(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine verifies that a console io handle is valid.

Arguments:

    ApiMessageData - Points to parameter structure.

Return Value:

--*/

{
    PCONSOLE_VERIFYIOHANDLE_MSG a = (PCONSOLE_VERIFYIOHANDLE_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    NTSTATUS Status;
    PHANDLE_DATA HandleData;
    PCONSOLE_PER_PROCESS_DATA ProcessData;

    UNREFERENCED_PARAMETER(ReplyStatus);

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (NT_SUCCESS(Status)) {
        ProcessData = CONSOLE_PERPROCESSDATA();
        Status = DereferenceIoHandleNoCheck(ProcessData,
                                     HANDLE_TO_INDEX(a->Handle),
                                     &HandleData
                                    );
        UnlockConsole(Console);
    }
    a->Valid = (NT_SUCCESS(Status));
    return STATUS_SUCCESS;
}


NTSTATUS
ApiPreamble(
    IN HANDLE ConsoleHandle,
    OUT PCONSOLE_INFORMATION *Console
    )
{
    NTSTATUS Status;

    //
    // If this process doesn't have a console handle, bail immediately.
    //

    if (ConsoleHandle == NULL || ConsoleHandle != CONSOLE_GETCONSOLEHANDLE()) {
        return STATUS_INVALID_HANDLE;
    }

#ifdef i386
    //Do not lock the console if we are in the special case:
    //(1). we are in the middle of handshaking with ntvdm doing
    //     full-screen to windowed mode transition
    //(2). the calling process is THE ntvdm process(this implies that the
    //     the console has vdm registered.
    //(3). the console handle is the same one.
    // if (1), (2) and (3) are true then the console is already locked
    // (locked by the windowproc while processing the WM_FULLSCREEN
    // message)

    RtlEnterCriticalSection(&ConsoleVDMCriticalSection);
    if (ConsoleVDMOnSwitching != NULL &&
        ConsoleVDMOnSwitching->ConsoleHandle == ConsoleHandle &&
        ConsoleVDMOnSwitching->VDMProcessId == CONSOLE_CLIENTPROCESSID())
    {
        KdPrint(("    ApiPreamble - Thread %lx Entered VDM CritSec\n", GetCurrentThreadId()));
        *Console = ConsoleVDMOnSwitching;
        return STATUS_SUCCESS;
    }
    RtlLeaveCriticalSection(&ConsoleVDMCriticalSection);
#endif

    Status = RevalidateConsole(ConsoleHandle,
                               Console
                              );
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }

    //
    // Make sure the console has been initialized and the window is valid
    //

    if ((*Console)->hWnd == NULL || ((*Console)->Flags & CONSOLE_TERMINATING)) {
        KdPrint(("CONSRV: bogus window for console %lx\n", *Console));
        UnlockConsole(*Console);
        return STATUS_INVALID_HANDLE;
    }

    return Status;
}

NTSTATUS
RevalidateConsole(
    IN HANDLE ConsoleHandle,
    OUT PCONSOLE_INFORMATION *Console
    )
{
    NTSTATUS Status;

    LockConsoleHandleTable();
    Status = DereferenceConsoleHandle(ConsoleHandle,
                                      Console
                                     );
    if (!NT_SUCCESS(Status)) {
        UnlockConsoleHandleTable();
        return Status;
    }

    //
    // The WaitCount ensures the console won't go away between the time
    // we unlock the console handle table and we lock the console.
    //

    InterlockedIncrement(&(*Console)->WaitCount);
    UnlockConsoleHandleTable();
    try {
        LockConsole(*Console);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        InterlockedDecrement(&(*Console)->WaitCount);
        return GetExceptionCode();
    }
    InterlockedDecrement(&(*Console)->WaitCount);

    //
    // If the console was marked for destruction while we were waiting to
    // lock it, try to destroy it and return.
    //

    if ((*Console)->Flags & CONSOLE_IN_DESTRUCTION) {
        DestroyConsole(*Console);
        *Console = NULL;
        return STATUS_INVALID_HANDLE;
    }

    //
    // If the console was marked for termination while we were waiting to
    // lock it, bail out.
    //

    if ((*Console)->Flags & CONSOLE_TERMINATING) {
        UnlockConsole(*Console);
        *Console = NULL;
        return STATUS_PROCESS_IS_TERMINATING;
    }

    return Status;
}


#if DBG

BOOLEAN
UnProtectHandle(
    HANDLE hObject
    )
{
    NTSTATUS Status;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;

    Status = NtQueryObject(hObject,
                           ObjectHandleFlagInformation,
                           &HandleInfo,
                           sizeof(HandleInfo),
                           NULL
                          );
    if (NT_SUCCESS(Status)) {
        HandleInfo.ProtectFromClose = FALSE;
        Status = NtSetInformationObject(hObject,
                                        ObjectHandleFlagInformation,
                                        &HandleInfo,
                                        sizeof(HandleInfo)
                                       );
        if (NT_SUCCESS(Status)) {
            return TRUE;
        }
    }

    return FALSE;
}

#endif // DBG
