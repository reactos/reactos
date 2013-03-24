/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/handle.c
 * PURPOSE:         Console I/O Handles functions
 * PROGRAMMERS:
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "conio.h"

//#define NDEBUG
#include <debug.h>


/* PRIVATE FUNCTIONS **********************************************************/

static INT
AdjustHandleCounts(PCONSOLE_IO_HANDLE Entry, INT Change)
{
    Object_t *Object = Entry->Object;

    DPRINT1("AdjustHandleCounts(0x%p, %d), Object = 0x%p, Object->HandleCount = %d, Object->Type = %lu\n", Entry, Change, Object, Object->HandleCount, Object->Type);

    if (Entry->Access & GENERIC_READ)           Object->AccessRead += Change;
    if (Entry->Access & GENERIC_WRITE)          Object->AccessWrite += Change;
    if (!(Entry->ShareMode & FILE_SHARE_READ))  Object->ExclusiveRead += Change;
    if (!(Entry->ShareMode & FILE_SHARE_WRITE)) Object->ExclusiveWrite += Change;

    Object->HandleCount += Change;

    return Object->HandleCount;
}

static VOID
ConSrvCreateHandleEntry(PCONSOLE_IO_HANDLE Entry)
{
    /// LOCK /// Object_t *Object = Entry->Object;
    /// LOCK /// EnterCriticalSection(&Object->Console->Lock);
    AdjustHandleCounts(Entry, +1);
    /// LOCK /// LeaveCriticalSection(&Object->Console->Lock);
}

static VOID
ConSrvCloseHandleEntry(PCONSOLE_IO_HANDLE Entry)
{
    Object_t *Object = Entry->Object;
    if (Object != NULL)
    {
        /// LOCK /// PCONSOLE Console = Object->Console;
        /// LOCK /// EnterCriticalSection(&Console->Lock);

        /*
         * If this is a input handle, notify and dereference
         * all the waits related to this handle.
         */
        if (Object->Type == CONIO_INPUT_BUFFER_MAGIC)
        {
            PCONSOLE_INPUT_BUFFER InputBuffer = (PCONSOLE_INPUT_BUFFER)Object;

            /*
             * Wake up all the writing waiters related to this handle for this
             * input buffer, if any, then dereference them and purge them all
             * from the list.
             * To select them amongst all the waiters for this input buffer,
             * pass the handle pointer to the waiters, then they will check
             * whether or not they are related to this handle and if so, they
             * return.
             */
            CsrNotifyWait(&InputBuffer->ReadWaitQueue,
                          WaitAll,
                          NULL,
                          (PVOID)Entry);
            if (!IsListEmpty(&InputBuffer->ReadWaitQueue))
            {
                CsrDereferenceWait(&InputBuffer->ReadWaitQueue);
            }
        }

        /* If the last handle to a screen buffer is closed, delete it... */
        if (AdjustHandleCounts(Entry, -1) == 0)
        {
            if (Object->Type == CONIO_SCREEN_BUFFER_MAGIC)
            {
                PCONSOLE_SCREEN_BUFFER Buffer = (PCONSOLE_SCREEN_BUFFER)Object;
                /* ...unless it's the only buffer left. Windows allows deletion
                 * even of the last buffer, but having to deal with a lack of
                 * any active buffer might be error-prone. */
                if (Buffer->ListEntry.Flink != Buffer->ListEntry.Blink)
                    ConioDeleteScreenBuffer(Buffer);
            }
            else if (Object->Type == CONIO_INPUT_BUFFER_MAGIC)
            {
                DPRINT1("Closing the input buffer\n");
            }
        }

        /// LOCK /// LeaveCriticalSection(&Console->Lock);
        Entry->Object = NULL;
    }
}


/* Forward declaration, used in ConSrvInitHandlesTable */
static VOID ConSrvFreeHandlesTable(PCONSOLE_PROCESS_DATA ProcessData);

static NTSTATUS
ConSrvInitHandlesTable(IN OUT PCONSOLE_PROCESS_DATA ProcessData,
                       OUT PHANDLE pInputHandle,
                       OUT PHANDLE pOutputHandle,
                       OUT PHANDLE pErrorHandle)
{
    NTSTATUS Status;
    HANDLE InputHandle  = INVALID_HANDLE_VALUE,
           OutputHandle = INVALID_HANDLE_VALUE,
           ErrorHandle  = INVALID_HANDLE_VALUE;

    /*
     * Initialize the handles table. Use temporary variables to store
     * the handles values in such a way that, if we fail, we don't
     * return to the caller invalid handle values.
     *
     * Insert the IO handles.
     */

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    /* Insert the Input handle */
    Status = ConSrvInsertObject(ProcessData,
                                &InputHandle,
                                &ProcessData->Console->InputBuffer.Header,
                                GENERIC_READ | GENERIC_WRITE,
                                TRUE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to insert the input handle\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        ConSrvFreeHandlesTable(ProcessData);
        return Status;
    }

    /* Insert the Output handle */
    Status = ConSrvInsertObject(ProcessData,
                                &OutputHandle,
                                &ProcessData->Console->ActiveBuffer->Header,
                                GENERIC_READ | GENERIC_WRITE,
                                TRUE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to insert the output handle\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        ConSrvFreeHandlesTable(ProcessData);
        return Status;
    }

    /* Insert the Error handle */
    Status = ConSrvInsertObject(ProcessData,
                                &ErrorHandle,
                                &ProcessData->Console->ActiveBuffer->Header,
                                GENERIC_READ | GENERIC_WRITE,
                                TRUE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to insert the error handle\n");
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        ConSrvFreeHandlesTable(ProcessData);
        return Status;
    }

    /* Return the newly created handles */
    *pInputHandle  = InputHandle;
    *pOutputHandle = OutputHandle;
    *pErrorHandle  = ErrorHandle;

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return STATUS_SUCCESS;
}

static NTSTATUS
ConSrvInheritHandlesTable(IN PCONSOLE_PROCESS_DATA SourceProcessData,
                          IN PCONSOLE_PROCESS_DATA TargetProcessData)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    RtlEnterCriticalSection(&SourceProcessData->HandleTableLock);

    /* Inherit a handles table only if there is no already */
    if (TargetProcessData->HandleTable != NULL /* || TargetProcessData->HandleTableSize != 0 */)
    {
        Status = STATUS_UNSUCCESSFUL; /* STATUS_INVALID_PARAMETER */
        goto Quit;
    }

    /* Allocate a new handle table for the child process */
    TargetProcessData->HandleTable = RtlAllocateHeap(ConSrvHeap,
                                                     HEAP_ZERO_MEMORY,
                                                     SourceProcessData->HandleTableSize
                                                             * sizeof(CONSOLE_IO_HANDLE));
    if (TargetProcessData->HandleTable == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }

    TargetProcessData->HandleTableSize = SourceProcessData->HandleTableSize;

    /*
     * Parse the parent process' handles table and, for each handle,
     * do a copy of it and reference it, if the handle is inheritable.
     */
    for (i = 0; i < SourceProcessData->HandleTableSize; i++)
    {
        if (SourceProcessData->HandleTable[i].Object != NULL &&
            SourceProcessData->HandleTable[i].Inheritable)
        {
            /*
             * Copy the handle data and increment the reference count of the
             * pointed object (via the call to ConSrvCreateHandleEntry).
             */
            TargetProcessData->HandleTable[i] = SourceProcessData->HandleTable[i];
            ConSrvCreateHandleEntry(&TargetProcessData->HandleTable[i]);
        }
    }

Quit:
    RtlLeaveCriticalSection(&SourceProcessData->HandleTableLock);
    return Status;
}

static VOID
ConSrvFreeHandlesTable(PCONSOLE_PROCESS_DATA ProcessData)
{
    DPRINT1("ConSrvFreeHandlesTable\n");

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if (ProcessData->HandleTable != NULL)
    {
        ULONG i;

        /* Close all console handles and free the handle table memory */
        for (i = 0; i < ProcessData->HandleTableSize; i++)
        {
            ConSrvCloseHandleEntry(&ProcessData->HandleTable[i]);
        }
        RtlFreeHeap(ConSrvHeap, 0, ProcessData->HandleTable);
        ProcessData->HandleTable = NULL;
    }

    ProcessData->HandleTableSize = 0;

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
}

NTSTATUS
FASTCALL
ConSrvInsertObject(PCONSOLE_PROCESS_DATA ProcessData,
                   PHANDLE Handle,
                   Object_t *Object,
                   DWORD Access,
                   BOOL Inheritable,
                   DWORD ShareMode)
{
#define IO_HANDLES_INCREMENT    2*3

    ULONG i;
    PCONSOLE_IO_HANDLE Block;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    for (i = 0; i < ProcessData->HandleTableSize; i++)
    {
        if (ProcessData->HandleTable[i].Object == NULL)
        {
            break;
        }
    }
    if (i >= ProcessData->HandleTableSize)
    {
        Block = RtlAllocateHeap(ConSrvHeap,
                                HEAP_ZERO_MEMORY,
                                (ProcessData->HandleTableSize +
                                    IO_HANDLES_INCREMENT) * sizeof(CONSOLE_IO_HANDLE));
        if (Block == NULL)
        {
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return STATUS_UNSUCCESSFUL;
        }
        RtlCopyMemory(Block,
                      ProcessData->HandleTable,
                      ProcessData->HandleTableSize * sizeof(CONSOLE_IO_HANDLE));
        RtlFreeHeap(ConSrvHeap, 0, ProcessData->HandleTable);
        ProcessData->HandleTable = Block;
        ProcessData->HandleTableSize += IO_HANDLES_INCREMENT;
    }

    ProcessData->HandleTable[i].Object      = Object;
    ProcessData->HandleTable[i].Access      = Access;
    ProcessData->HandleTable[i].Inheritable = Inheritable;
    ProcessData->HandleTable[i].ShareMode   = ShareMode;
    ConSrvCreateHandleEntry(&ProcessData->HandleTable[i]);
    *Handle = ULongToHandle((i << 2) | 0x3);

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
ConSrvRemoveObject(PCONSOLE_PROCESS_DATA ProcessData,
                   HANDLE Handle)
{
    ULONG_PTR h = (ULONG_PTR)Handle >> 2;
    Object_t *Object;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if (h >= ProcessData->HandleTableSize ||
        (Object = ProcessData->HandleTable[h].Object) == NULL)
    {
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }

    DPRINT1("ConSrvRemoveObject - Process 0x%p, Release 0x%p\n", ProcessData->Process, &ProcessData->HandleTable[h]);
    ConSrvCloseHandleEntry(&ProcessData->HandleTable[h]);

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
ConSrvGetObject(PCONSOLE_PROCESS_DATA ProcessData,
                HANDLE Handle,
                Object_t** Object,
                PCONSOLE_IO_HANDLE* Entry OPTIONAL,
                DWORD Access,
                BOOL LockConsole,
                ULONG Type)
{
    ULONG_PTR h = (ULONG_PTR)Handle >> 2;
    PCONSOLE_IO_HANDLE HandleEntry = NULL;
    Object_t* ObjectEntry = NULL;

    ASSERT(Object);
    if (Entry) *Entry = NULL;

    // DPRINT("ConSrvGetObject, Object: %x, %x, %x\n",
           // Object, Handle, ProcessData ? ProcessData->HandleTableSize : 0);

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if ( IsConsoleHandle(Handle) &&
         h < ProcessData->HandleTableSize )
    {
        HandleEntry = &ProcessData->HandleTable[h];
        ObjectEntry = HandleEntry->Object;
    }

    if ( HandleEntry == NULL ||
         ObjectEntry == NULL ||
         (HandleEntry->Access & Access) == 0 ||
         (Type != 0 && ObjectEntry->Type != Type) )
    {
        DPRINT1("ConSrvGetObject returning invalid handle (%x) of type %lu with access %lu\n", Handle, Type, Access);
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }

    _InterlockedIncrement(&ObjectEntry->Console->ReferenceCount);
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    if (LockConsole) EnterCriticalSection(&ObjectEntry->Console->Lock);

    /* Return the objects to the caller */
    *Object = ObjectEntry;
    if (Entry) *Entry = HandleEntry;

    return STATUS_SUCCESS;
}

VOID
FASTCALL
ConSrvReleaseObject(Object_t *Object,
                    BOOL IsConsoleLocked)
{
    ConSrvReleaseConsole(Object->Console, IsConsoleLocked);
}

NTSTATUS
FASTCALL
ConSrvAllocateConsole(PCONSOLE_PROCESS_DATA ProcessData,
                      PHANDLE pInputHandle,
                      PHANDLE pOutputHandle,
                      PHANDLE pErrorHandle,
                      PCONSOLE_START_INFO ConsoleStartInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;

    /* Initialize a new Console owned by this process */
    Status = ConSrvInitConsole(&ProcessData->Console, ConsoleStartInfo, ProcessData->Process);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Console initialization failed\n");
        return Status;
    }

    /* Initialize the handles table */
    Status = ConSrvInitHandlesTable(ProcessData,
                                    pInputHandle,
                                    pOutputHandle,
                                    pErrorHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize the handles table\n");
        ConSrvDeleteConsole(ProcessData->Console);
        ProcessData->Console = NULL;
        return Status;
    }

    /* Duplicate the Input Event */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               ProcessData->Console->InputBuffer.ActiveEvent,
                               ProcessData->Process->ProcessHandle,
                               &ProcessData->ConsoleEvent,
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject() failed: %lu\n", Status);
        ConSrvFreeHandlesTable(ProcessData);
        ConSrvDeleteConsole(ProcessData->Console);
        ProcessData->Console = NULL;
        return Status;
    }

    /* Insert the process into the processes list of the console */
    InsertHeadList(&ProcessData->Console->ProcessList, &ProcessData->ConsoleLink);

    /* Add a reference count because the process is tied to the console */
    _InterlockedIncrement(&ProcessData->Console->ReferenceCount);

    /* Update the internal info of the terminal */
    ConioRefreshInternalInfo(ProcessData->Console);

    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
ConSrvInheritConsole(PCONSOLE_PROCESS_DATA ProcessData,
                     PCONSOLE Console,
                     BOOL CreateNewHandlesTable,
                     PHANDLE pInputHandle,
                     PHANDLE pOutputHandle,
                     PHANDLE pErrorHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;

    /* Inherit the console */
    ProcessData->Console = Console;

    if (CreateNewHandlesTable)
    {
        /* Initialize the handles table */
        Status = ConSrvInitHandlesTable(ProcessData,
                                        pInputHandle,
                                        pOutputHandle,
                                        pErrorHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to initialize the handles table\n");
            ProcessData->Console = NULL;
            return Status;
        }
    }

    /* Duplicate the Input Event */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               ProcessData->Console->InputBuffer.ActiveEvent,
                               ProcessData->Process->ProcessHandle,
                               &ProcessData->ConsoleEvent,
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject() failed: %lu\n", Status);
        ConSrvFreeHandlesTable(ProcessData); // NOTE: Always free the handles table.
        ProcessData->Console = NULL;
        return Status;
    }

    /* Insert the process into the processes list of the console */
    InsertHeadList(&ProcessData->Console->ProcessList, &ProcessData->ConsoleLink);

    /* Add a reference count because the process is tied to the console */
    _InterlockedIncrement(&ProcessData->Console->ReferenceCount);

    /* Update the internal info of the terminal */
    ConioRefreshInternalInfo(ProcessData->Console);

    return STATUS_SUCCESS;
}

VOID
FASTCALL
ConSrvRemoveConsole(PCONSOLE_PROCESS_DATA ProcessData)
{
    PCONSOLE Console;

    DPRINT1("ConSrvRemoveConsole\n");

    /* Close all console handles and free the handle table memory */
    ConSrvFreeHandlesTable(ProcessData);

    /* Detach process from console */
    Console = ProcessData->Console;
    if (Console != NULL)
    {
        DPRINT1("ConSrvRemoveConsole - Console->ReferenceCount = %lu - We are going to decrement it !\n", Console->ReferenceCount);
        ProcessData->Console = NULL;

        EnterCriticalSection(&Console->Lock);
        DPRINT1("ConSrvRemoveConsole - Locking OK\n");

        /* Remove ourselves from the console's list of processes */
        RemoveEntryList(&ProcessData->ConsoleLink);

        /* Update the internal info of the terminal */
        ConioRefreshInternalInfo(Console);

        /* Release the console */
        ConSrvReleaseConsole(Console, TRUE);
        //CloseHandle(ProcessData->ConsoleEvent);
        //ProcessData->ConsoleEvent = NULL;
    }
}

NTSTATUS
FASTCALL
ConSrvGetConsole(PCONSOLE_PROCESS_DATA ProcessData,
                 PCONSOLE* Console,
                 BOOL LockConsole)
{
    PCONSOLE ProcessConsole;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    ProcessConsole = ProcessData->Console;

    if (!ProcessConsole)
    {
        *Console = NULL;
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }

    InterlockedIncrement(&ProcessConsole->ReferenceCount);
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    if (LockConsole) EnterCriticalSection(&ProcessConsole->Lock);

    *Console = ProcessConsole;

    return STATUS_SUCCESS;
}

VOID FASTCALL
ConSrvReleaseConsole(PCONSOLE Console,
                     BOOL IsConsoleLocked)
{
    if (IsConsoleLocked) LeaveCriticalSection(&Console->Lock);

    /* Decrement reference count */
    if (_InterlockedDecrement(&Console->ReferenceCount) == 0)
        ConSrvDeleteConsole(Console);
}

NTSTATUS
NTAPI
ConSrvNewProcess(PCSR_PROCESS SourceProcess,
                 PCSR_PROCESS TargetProcess)
{
    /**************************************************************************
     * This function is called whenever a new process (GUI or CUI) is created.
     *
     * Copy the parent's handles table here if both the parent and the child
     * processes are CUI. If we must actually create our proper console (and
     * thus do not inherit from the console handles of the parent's), then we
     * will clean this table in the next ConSrvConnect call. Why we are doing
     * this? It's because here, we still don't know whether or not we must create
     * a new console instead of inherit it from the parent, and, because in
     * ConSrvConnect we don't have any reference to the parent process anymore.
     **************************************************************************/

    PCONSOLE_PROCESS_DATA SourceProcessData, TargetProcessData;

    /* An empty target process is invalid */
    if (!TargetProcess) return STATUS_INVALID_PARAMETER;

    TargetProcessData = ConsoleGetPerProcessData(TargetProcess);

    /**** HACK !!!! ****/ RtlZeroMemory(TargetProcessData, sizeof(*TargetProcessData));

    /* Initialize the new (target) process */
    TargetProcessData->Process = TargetProcess;
    TargetProcessData->ConsoleEvent = NULL;
    TargetProcessData->Console = TargetProcessData->ParentConsole = NULL;
    TargetProcessData->ConsoleApp = ((TargetProcess->Flags & CsrProcessIsConsoleApp) ? TRUE : FALSE);

    // Testing
    TargetProcessData->HandleTableSize = 0;
    TargetProcessData->HandleTable = NULL;

    RtlInitializeCriticalSection(&TargetProcessData->HandleTableLock);

    /* Do nothing if the source process is NULL */
    if (!SourceProcess) return STATUS_SUCCESS;

    SourceProcessData = ConsoleGetPerProcessData(SourceProcess);

    /*
     * If both of the processes (parent and new child) are console applications,
     * then try to inherit handles from the parent process.
     */
    if ( SourceProcessData->Console != NULL && /* SourceProcessData->ConsoleApp */
         TargetProcessData->ConsoleApp )
    {
        NTSTATUS Status;

        Status = ConSrvInheritHandlesTable(SourceProcessData, TargetProcessData);
        if (!NT_SUCCESS(Status)) return Status;

        /* Temporary save the parent's console */
        TargetProcessData->ParentConsole = SourceProcessData->Console;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ConSrvConnect(IN PCSR_PROCESS CsrProcess,
              IN OUT PVOID ConnectionInfo,
              IN OUT PULONG ConnectionInfoLength)
{
    /**************************************************************************
     * This function is called whenever a CUI new process is created.
     **************************************************************************/

    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_CONNECTION_INFO ConnectInfo = (PCONSOLE_CONNECTION_INFO)ConnectionInfo;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrProcess);

    if ( ConnectionInfo       == NULL ||
         ConnectionInfoLength == NULL ||
        *ConnectionInfoLength != sizeof(CONSOLE_CONNECTION_INFO) )
    {
        DPRINT1("CONSRV: Connection failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* If we don't need a console, then get out of here */
    if (!ConnectInfo->ConsoleNeeded || !ProcessData->ConsoleApp) // In fact, it is for GUI apps.
    {
        return STATUS_SUCCESS;
    }

    /* If we don't have a console, then create a new one... */
    if (!ConnectInfo->Console ||
         ConnectInfo->Console != ProcessData->ParentConsole)
    {
        DPRINT1("ConSrvConnect - Allocate a new console\n");

        /*
         * We are about to create a new console. However when ConSrvNewProcess
         * was called, we didn't know that we wanted to create a new console and
         * therefore, we by default inherited the handles table from our parent
         * process. It's only now that we notice that in fact we do not need
         * them, because we've created a new console and thus we must use it.
         *
         * Therefore, free the console we can have and our handles table,
         * and recreate a new one later on.
         */
        ConSrvRemoveConsole(ProcessData);

        /* Initialize a new Console owned by the Console Leader Process */
        Status = ConSrvAllocateConsole(ProcessData,
                                       &ConnectInfo->InputHandle,
                                       &ConnectInfo->OutputHandle,
                                       &ConnectInfo->ErrorHandle,
                                       &ConnectInfo->ConsoleStartInfo);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Console allocation failed\n");
            return Status;
        }
    }
    else /* We inherit it from the parent */
    {
        DPRINT1("ConSrvConnect - Reuse current (parent's) console\n");

        /* Reuse our current console */
        Status = ConSrvInheritConsole(ProcessData,
                                      ConnectInfo->Console,
                                      FALSE,
                                      NULL,  // &ConnectInfo->InputHandle,
                                      NULL,  // &ConnectInfo->OutputHandle,
                                      NULL); // &ConnectInfo->ErrorHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Console inheritance failed\n");
            return Status;
        }
    }

    /* Return it to the caller */
    ConnectInfo->Console = ProcessData->Console;

    /* Input Wait Handle */
    ConnectInfo->InputWaitHandle = ProcessData->ConsoleEvent;

    /* Set the Property Dialog Handler */
    ProcessData->PropDispatcher = ConnectInfo->PropDispatcher;

    /* Set the Ctrl Dispatcher */
    ProcessData->CtrlDispatcher = ConnectInfo->CtrlDispatcher;

    return STATUS_SUCCESS;
}

VOID
NTAPI
ConSrvDisconnect(PCSR_PROCESS Process)
{
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(Process);

    /**************************************************************************
     * This function is called whenever a new process (GUI or CUI) is destroyed.
     **************************************************************************/

    DPRINT1("ConSrvDisconnect\n");

    if ( ProcessData->Console     != NULL ||
         ProcessData->HandleTable != NULL )
    {
        DPRINT1("ConSrvDisconnect - calling ConSrvRemoveConsole\n");
        ConSrvRemoveConsole(ProcessData);
    }

    RtlDeleteCriticalSection(&ProcessData->HandleTableLock);
}


/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvCloseHandle)
{
    PCONSOLE_CLOSEHANDLE CloseHandleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CloseHandleRequest;

    return ConSrvRemoveObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                 CloseHandleRequest->ConsoleHandle);
}

CSR_API(SrvVerifyConsoleIoHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_VERIFYHANDLE VerifyHandleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.VerifyHandleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    HANDLE ConsoleHandle = VerifyHandleRequest->ConsoleHandle;
    ULONG_PTR Index = (ULONG_PTR)ConsoleHandle >> 2;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if (!IsConsoleHandle(ConsoleHandle)    ||
        Index >= ProcessData->HandleTableSize ||
        ProcessData->HandleTable[Index].Object == NULL)
    {
        DPRINT("SrvVerifyConsoleIoHandle failed\n");
        Status = STATUS_INVALID_HANDLE;
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return Status;
}

CSR_API(SrvDuplicateHandle)
{
    PCONSOLE_IO_HANDLE Entry;
    DWORD DesiredAccess;
    PCONSOLE_DUPLICATEHANDLE DuplicateHandleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.DuplicateHandleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    HANDLE ConsoleHandle = DuplicateHandleRequest->ConsoleHandle;
    ULONG_PTR Index = (ULONG_PTR)ConsoleHandle >> 2;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if ( /** !IsConsoleHandle(ConsoleHandle)    || **/
        Index >= ProcessData->HandleTableSize ||
        (Entry = &ProcessData->HandleTable[Index])->Object == NULL)
    {
        DPRINT1("Couldn't duplicate invalid handle %p\n", ConsoleHandle);
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }

    if (DuplicateHandleRequest->Options & DUPLICATE_SAME_ACCESS)
    {
        DesiredAccess = Entry->Access;
    }
    else
    {
        DesiredAccess = DuplicateHandleRequest->Access;
        /* Make sure the source handle has all the desired flags */
        if ((Entry->Access & DesiredAccess) == 0)
        {
            DPRINT1("Handle %p only has access %X; requested %X\n",
                ConsoleHandle, Entry->Access, DesiredAccess);
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return STATUS_INVALID_PARAMETER;
        }
    }

    ApiMessage->Status = ConSrvInsertObject(ProcessData,
                                            &DuplicateHandleRequest->ConsoleHandle, // Use the new handle value!
                                            Entry->Object,
                                            DesiredAccess,
                                            DuplicateHandleRequest->Inheritable,
                                            Entry->ShareMode);
    if (NT_SUCCESS(ApiMessage->Status) &&
        DuplicateHandleRequest->Options & DUPLICATE_CLOSE_SOURCE)
    {
        ConSrvCloseHandleEntry(Entry);
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return ApiMessage->Status;
}

/* EOF */
