/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/handle.c
 * PURPOSE:         Console I/O Handles functions
 * PROGRAMMERS:     David Welch
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "include/conio2.h"
#include "handle.h"
#include "include/console.h"
#include "console.h"
#include "conoutput.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

typedef struct _CONSOLE_IO_HANDLE
{
    PCONSOLE_IO_OBJECT Object;   /* The object on which the handle points to */
    DWORD Access;
    BOOL Inheritable;
    DWORD ShareMode;
} CONSOLE_IO_HANDLE, *PCONSOLE_IO_HANDLE;


/* PRIVATE FUNCTIONS **********************************************************/

static INT
AdjustHandleCounts(PCONSOLE_IO_HANDLE Entry, INT Change)
{
    PCONSOLE_IO_OBJECT Object = Entry->Object;

    DPRINT("AdjustHandleCounts(0x%p, %d), Object = 0x%p\n", Entry, Change, Object);
    DPRINT("\tAdjustHandleCounts(0x%p, %d), Object = 0x%p, Object->HandleCount = %d, Object->Type = %lu\n", Entry, Change, Object, Object->HandleCount, Object->Type);

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
    /// LOCK /// PCONSOLE_IO_OBJECT Object = Entry->Object;
    /// LOCK /// EnterCriticalSection(&Object->Console->Lock);
    AdjustHandleCounts(Entry, +1);
    /// LOCK /// LeaveCriticalSection(&Object->Console->Lock);
}

static VOID
ConSrvCloseHandleEntry(PCONSOLE_IO_HANDLE Entry)
{
    PCONSOLE_IO_OBJECT Object = Entry->Object;
    if (Object != NULL)
    {
        /// LOCK /// PCONSOLE Console = Object->Console;
        /// LOCK /// EnterCriticalSection(&Console->Lock);

        /*
         * If this is a input handle, notify and dereference
         * all the waits related to this handle.
         */
        if (Object->Type == INPUT_BUFFER)
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
            if (Object->Type == TEXTMODE_BUFFER || Object->Type == GRAPHICS_BUFFER)
            {
                PCONSOLE_SCREEN_BUFFER Buffer = (PCONSOLE_SCREEN_BUFFER)Object;
                /* ...unless it's the only buffer left. Windows allows deletion
                 * even of the last buffer, but having to deal with a lack of
                 * any active buffer might be error-prone. */
                if (Buffer->ListEntry.Flink != Buffer->ListEntry.Blink)
                    ConioDeleteScreenBuffer(Buffer);
            }
            else if (Object->Type == INPUT_BUFFER)
            {
                DPRINT("Closing the input buffer\n");
            }
            else
            {
                DPRINT1("Invalid object type %d\n", Object->Type);
            }
        }

        /// LOCK /// LeaveCriticalSection(&Console->Lock);

        /* Invalidate (zero-out) this handle entry */
        // Entry->Object = NULL;
        // RtlZeroMemory(Entry, sizeof(*Entry));
    }
    RtlZeroMemory(Entry, sizeof(*Entry)); // Be sure the whole entry is invalidated.
}


/* Forward declaration, used in ConSrvInitHandlesTable */
static VOID ConSrvFreeHandlesTable(PCONSOLE_PROCESS_DATA ProcessData);

static NTSTATUS
ConSrvInitHandlesTable(IN OUT PCONSOLE_PROCESS_DATA ProcessData,
                       IN PCONSOLE Console,
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
                                &Console->InputBuffer.Header,
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
                                &Console->ActiveBuffer->Header,
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
                                &Console->ActiveBuffer->Header,
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

NTSTATUS
ConSrvInheritHandlesTable(IN PCONSOLE_PROCESS_DATA SourceProcessData,
                          IN PCONSOLE_PROCESS_DATA TargetProcessData)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i, j;

    RtlEnterCriticalSection(&SourceProcessData->HandleTableLock);

    /* Inherit a handles table only if there is no already */
    if (TargetProcessData->HandleTable != NULL /* || TargetProcessData->HandleTableSize != 0 */)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Quit;
    }

    /* Allocate a new handle table for the child process */
    TargetProcessData->HandleTable = ConsoleAllocHeap(HEAP_ZERO_MEMORY,
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
    for (i = 0, j = 0; i < SourceProcessData->HandleTableSize; i++)
    {
        if (SourceProcessData->HandleTable[i].Object != NULL &&
            SourceProcessData->HandleTable[i].Inheritable)
        {
            /*
             * Copy the handle data and increment the reference count of the
             * pointed object (via the call to ConSrvCreateHandleEntry).
             */
            TargetProcessData->HandleTable[j] = SourceProcessData->HandleTable[i];
            ConSrvCreateHandleEntry(&TargetProcessData->HandleTable[j]);
            ++j;
        }
    }

Quit:
    RtlLeaveCriticalSection(&SourceProcessData->HandleTableLock);
    return Status;
}

static VOID
ConSrvFreeHandlesTable(PCONSOLE_PROCESS_DATA ProcessData)
{
    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if (ProcessData->HandleTable != NULL)
    {
        ULONG i;

        /*
         * ProcessData->ConsoleHandle is NULL (and the assertion fails) when
         * ConSrvFreeHandlesTable is called in ConSrvConnect during the
         * allocation of a new console.
         */
        // ASSERT(ProcessData->ConsoleHandle);
        if (ProcessData->ConsoleHandle != NULL)
        {
            /* Close all the console handles */
            for (i = 0; i < ProcessData->HandleTableSize; i++)
            {
                ConSrvCloseHandleEntry(&ProcessData->HandleTable[i]);
            }
        }
        /* Free the handles table memory */
        ConsoleFreeHeap(ProcessData->HandleTable);
        ProcessData->HandleTable = NULL;
    }

    ProcessData->HandleTableSize = 0;

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
}

VOID
FASTCALL
ConSrvInitObject(IN OUT PCONSOLE_IO_OBJECT Object,
                 IN CONSOLE_IO_OBJECT_TYPE Type,
                 IN PCONSOLE Console)
{
    ASSERT(Object);
    // if (!Object) return;

    Object->Type    = Type;
    Object->Console = Console;
    Object->AccessRead    = Object->AccessWrite    = 0;
    Object->ExclusiveRead = Object->ExclusiveWrite = 0;
    Object->HandleCount   = 0;
}

NTSTATUS
FASTCALL
ConSrvInsertObject(PCONSOLE_PROCESS_DATA ProcessData,
                   PHANDLE Handle,
                   PCONSOLE_IO_OBJECT Object,
                   DWORD Access,
                   BOOL Inheritable,
                   DWORD ShareMode)
{
#define IO_HANDLES_INCREMENT    2 * 3

    ULONG i = 0;
    PCONSOLE_IO_HANDLE Block;

    // NOTE: Commented out because calling code always lock HandleTableLock before.
    // RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    ASSERT( (ProcessData->HandleTable == NULL && ProcessData->HandleTableSize == 0) ||
            (ProcessData->HandleTable != NULL && ProcessData->HandleTableSize != 0) );

    if (ProcessData->HandleTable)
    {
        for (i = 0; i < ProcessData->HandleTableSize; i++)
        {
            if (ProcessData->HandleTable[i].Object == NULL)
                break;
        }
    }

    if (i >= ProcessData->HandleTableSize)
    {
        /* Allocate a new handles table */
        Block = ConsoleAllocHeap(HEAP_ZERO_MEMORY,
                                 (ProcessData->HandleTableSize +
                                    IO_HANDLES_INCREMENT) * sizeof(CONSOLE_IO_HANDLE));
        if (Block == NULL)
        {
            // RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return STATUS_UNSUCCESSFUL;
        }

        /* If we previously had a handles table, free it and use the new one */
        if (ProcessData->HandleTable)
        {
            /* Copy the handles from the old table to the new one */
            RtlCopyMemory(Block,
                          ProcessData->HandleTable,
                          ProcessData->HandleTableSize * sizeof(CONSOLE_IO_HANDLE));
            ConsoleFreeHeap(ProcessData->HandleTable);
        }
        ProcessData->HandleTable = Block;
        ProcessData->HandleTableSize += IO_HANDLES_INCREMENT;
    }

    ProcessData->HandleTable[i].Object      = Object;
    ProcessData->HandleTable[i].Access      = Access;
    ProcessData->HandleTable[i].Inheritable = Inheritable;
    ProcessData->HandleTable[i].ShareMode   = ShareMode;
    ConSrvCreateHandleEntry(&ProcessData->HandleTable[i]);
    *Handle = ULongToHandle((i << 2) | 0x3);

    // RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
ConSrvRemoveObject(PCONSOLE_PROCESS_DATA ProcessData,
                   HANDLE Handle)
{
    ULONG Index = HandleToULong(Handle) >> 2;
    PCONSOLE_IO_OBJECT Object;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    ASSERT(ProcessData->HandleTable);

    if (Index >= ProcessData->HandleTableSize ||
        (Object = ProcessData->HandleTable[Index].Object) == NULL)
    {
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }

    ASSERT(ProcessData->ConsoleHandle);
    ConSrvCloseHandleEntry(&ProcessData->HandleTable[Index]);

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
ConSrvGetObject(PCONSOLE_PROCESS_DATA ProcessData,
                HANDLE Handle,
                PCONSOLE_IO_OBJECT* Object,
                PVOID* Entry OPTIONAL,
                DWORD Access,
                BOOL LockConsole,
                CONSOLE_IO_OBJECT_TYPE Type)
{
    // NTSTATUS Status;
    ULONG Index = HandleToULong(Handle) >> 2;
    PCONSOLE_IO_HANDLE HandleEntry = NULL;
    PCONSOLE_IO_OBJECT ObjectEntry = NULL;
    // PCONSOLE ObjectConsole;

    ASSERT(Object);
    if (Entry) *Entry = NULL;

    DPRINT("ConSrvGetObject -- Object: 0x%x, Handle: 0x%x\n", Object, Handle);

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if ( IsConsoleHandle(Handle) &&
         Index < ProcessData->HandleTableSize )
    {
        HandleEntry = &ProcessData->HandleTable[Index];
        ObjectEntry = HandleEntry->Object;
    }

    if ( HandleEntry == NULL ||
         ObjectEntry == NULL ||
         (HandleEntry->Access & Access) == 0 ||
         /*(Type != 0 && ObjectEntry->Type != Type)*/
         (Type != 0 && (ObjectEntry->Type & Type) == 0) )
    {
        DPRINT("ConSrvGetObject -- Invalid handle 0x%x of type %lu with access %lu ; retrieved object 0x%x (handle 0x%x) of type %lu with access %lu\n",
               Handle, Type, Access, ObjectEntry, HandleEntry, (ObjectEntry ? ObjectEntry->Type : 0), (HandleEntry ? HandleEntry->Access : 0));

        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    // Status = ConDrvGetConsole(&ObjectConsole, ProcessData->ConsoleHandle, LockConsole);
    // if (NT_SUCCESS(Status))
    if (ConDrvValidateConsoleUnsafe(ObjectEntry->Console, CONSOLE_RUNNING, LockConsole))
    {
        _InterlockedIncrement(&ObjectEntry->Console->ReferenceCount);

        /* Return the objects to the caller */
        *Object = ObjectEntry;
        if (Entry) *Entry = HandleEntry;

        // RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_SUCCESS;
    }
    else
    {
        // RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }
}

VOID
FASTCALL
ConSrvReleaseObject(PCONSOLE_IO_OBJECT Object,
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
    HANDLE ConsoleHandle;
    PCONSOLE Console;

    /*
     * We are about to create a new console. However when ConSrvNewProcess
     * was called, we didn't know that we wanted to create a new console and
     * therefore, we by default inherited the handles table from our parent
     * process. It's only now that we notice that in fact we do not need
     * them, because we've created a new console and thus we must use it.
     *
     * Therefore, free the handles table so that we can recreate
     * a new one later on.
     */
    ConSrvFreeHandlesTable(ProcessData);

    /* Initialize a new Console owned by this process */
    Status = ConSrvInitConsole(&ConsoleHandle,
                               &Console,
                               ConsoleStartInfo,
                               HandleToUlong(ProcessData->Process->ClientId.UniqueProcess));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Console initialization failed\n");
        return Status;
    }

    /* Assign the new console handle */
    ProcessData->ConsoleHandle = ConsoleHandle;

    /* Initialize the handles table */
    Status = ConSrvInitHandlesTable(ProcessData,
                                    Console,
                                    pInputHandle,
                                    pOutputHandle,
                                    pErrorHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize the handles table\n");
        ConSrvDeleteConsole(Console);
        ProcessData->ConsoleHandle = NULL;
        return Status;
    }

    /* Duplicate the Input Event */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               Console->InputBuffer.ActiveEvent,
                               ProcessData->Process->ProcessHandle,
                               &ProcessData->ConsoleEvent,
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject() failed: %lu\n", Status);
        ConSrvFreeHandlesTable(ProcessData);
        ConSrvDeleteConsole(Console);
        ProcessData->ConsoleHandle = NULL;
        return Status;
    }

    /* Insert the process into the processes list of the console */
    InsertHeadList(&Console->ProcessList, &ProcessData->ConsoleLink);

    /* Add a reference count because the process is tied to the console */
    _InterlockedIncrement(&Console->ReferenceCount);

    /* Update the internal info of the terminal */
    ConioRefreshInternalInfo(Console);

    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
ConSrvInheritConsole(PCONSOLE_PROCESS_DATA ProcessData,
                     HANDLE ConsoleHandle,
                     BOOL CreateNewHandlesTable,
                     PHANDLE pInputHandle,
                     PHANDLE pOutputHandle,
                     PHANDLE pErrorHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE Console;

    /* Validate and lock the console */
    if (!ConDrvValidateConsole(&Console,
                               ConsoleHandle,
                               CONSOLE_RUNNING, TRUE))
    {
        // FIXME: Find another status code
        return STATUS_UNSUCCESSFUL;
    }

    /* Inherit the console */
    ProcessData->ConsoleHandle = ConsoleHandle;

    if (CreateNewHandlesTable)
    {
        /*
         * We are about to create a new console. However when ConSrvNewProcess
         * was called, we didn't know that we wanted to create a new console and
         * therefore, we by default inherited the handles table from our parent
         * process. It's only now that we notice that in fact we do not need
         * them, because we've created a new console and thus we must use it.
         *
         * Therefore, free the handles table so that we can recreate
         * a new one later on.
         */
        ConSrvFreeHandlesTable(ProcessData);

        /* Initialize the handles table */
        Status = ConSrvInitHandlesTable(ProcessData,
                                        Console,
                                        pInputHandle,
                                        pOutputHandle,
                                        pErrorHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to initialize the handles table\n");
            ProcessData->ConsoleHandle = NULL;
            goto Quit;
        }
    }

    /* Duplicate the Input Event */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               Console->InputBuffer.ActiveEvent,
                               ProcessData->Process->ProcessHandle,
                               &ProcessData->ConsoleEvent,
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject() failed: %lu\n", Status);
        ConSrvFreeHandlesTable(ProcessData); // NOTE: Always free the handles table.
        ProcessData->ConsoleHandle = NULL;
        goto Quit;
    }

    /* Insert the process into the processes list of the console */
    InsertHeadList(&Console->ProcessList, &ProcessData->ConsoleLink);

    /* Add a reference count because the process is tied to the console */
    _InterlockedIncrement(&Console->ReferenceCount);

    /* Update the internal info of the terminal */
    ConioRefreshInternalInfo(Console);

    Status = STATUS_SUCCESS;

Quit:
    /* Unlock the console and return */
    LeaveCriticalSection(&Console->Lock);
    return Status;
}

VOID
FASTCALL
ConSrvRemoveConsole(PCONSOLE_PROCESS_DATA ProcessData)
{
    PCONSOLE Console;

    DPRINT("ConSrvRemoveConsole\n");

    // RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    /* Validate and lock the console */
    if (ConDrvValidateConsole(&Console,
                              ProcessData->ConsoleHandle,
                              CONSOLE_RUNNING, TRUE))
    {
        DPRINT("ConSrvRemoveConsole - Locking OK\n");

        /* Close all console handles and free the handles table */
        ConSrvFreeHandlesTable(ProcessData);

        /* Detach the process from the console */
        ProcessData->ConsoleHandle = NULL;

        /* Remove ourselves from the console's list of processes */
        RemoveEntryList(&ProcessData->ConsoleLink);

        /* Update the internal info of the terminal */
        ConioRefreshInternalInfo(Console);

        /* Release the console */
        DPRINT("ConSrvRemoveConsole - Decrement Console->ReferenceCount = %lu\n", Console->ReferenceCount);
        ConDrvReleaseConsole(Console, TRUE);
        //CloseHandle(ProcessData->ConsoleEvent);
        //ProcessData->ConsoleEvent = NULL;
    }

    // RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
}


/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvOpenConsole)
{
    /*
     * This API opens a handle to either the input buffer or to
     * a screen-buffer of the console of the current process.
     */

    NTSTATUS Status;
    PCONSOLE_OPENCONSOLE OpenConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.OpenConsoleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCONSOLE Console;

    DWORD DesiredAccess = OpenConsoleRequest->Access;
    DWORD ShareMode = OpenConsoleRequest->ShareMode;
    PCONSOLE_IO_OBJECT Object;

    OpenConsoleRequest->ConsoleHandle = INVALID_HANDLE_VALUE;

    Status = ConSrvGetConsole(ProcessData, &Console, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't get console, status %lx\n", Status);
        return Status;
    }

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    /*
     * Open a handle to either the active screen buffer or the input buffer.
     */
    if (OpenConsoleRequest->HandleType == HANDLE_OUTPUT)
    {
        Object = &Console->ActiveBuffer->Header;
    }
    else // HANDLE_INPUT
    {
        Object = &Console->InputBuffer.Header;
    }

    if (((DesiredAccess & GENERIC_READ)  && Object->ExclusiveRead  != 0) ||
        ((DesiredAccess & GENERIC_WRITE) && Object->ExclusiveWrite != 0) ||
        (!(ShareMode & FILE_SHARE_READ)  && Object->AccessRead     != 0) ||
        (!(ShareMode & FILE_SHARE_WRITE) && Object->AccessWrite    != 0))
    {
        DPRINT1("Sharing violation\n");
        Status = STATUS_SHARING_VIOLATION;
    }
    else
    {
        Status = ConSrvInsertObject(ProcessData,
                                    &OpenConsoleRequest->ConsoleHandle,
                                    Object,
                                    DesiredAccess,
                                    OpenConsoleRequest->Inheritable,
                                    ShareMode);
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvCloseHandle)
{
    NTSTATUS Status;
    PCONSOLE_CLOSEHANDLE CloseHandleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CloseHandleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCONSOLE Console;

    Status = ConSrvGetConsole(ProcessData, &Console, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't get console, status %lx\n", Status);
        return Status;
    }

    Status = ConSrvRemoveObject(ProcessData, CloseHandleRequest->ConsoleHandle);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvVerifyConsoleIoHandle)
{
    NTSTATUS Status;
    PCONSOLE_VERIFYHANDLE VerifyHandleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.VerifyHandleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCONSOLE Console;

    HANDLE ConsoleHandle = VerifyHandleRequest->ConsoleHandle;
    ULONG Index = HandleToULong(ConsoleHandle) >> 2;

    Status = ConSrvGetConsole(ProcessData, &Console, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't get console, status %lx\n", Status);
        return Status;
    }

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if (!IsConsoleHandle(ConsoleHandle)    ||
        Index >= ProcessData->HandleTableSize ||
        ProcessData->HandleTable[Index].Object == NULL)
    {
        DPRINT("SrvVerifyConsoleIoHandle failed\n");
        Status = STATUS_INVALID_HANDLE;
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvDuplicateHandle)
{
    NTSTATUS Status;
    PCONSOLE_DUPLICATEHANDLE DuplicateHandleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.DuplicateHandleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCONSOLE Console;

    HANDLE ConsoleHandle = DuplicateHandleRequest->ConsoleHandle;
    ULONG Index = HandleToULong(ConsoleHandle) >> 2;
    PCONSOLE_IO_HANDLE Entry;
    DWORD DesiredAccess;

    Status = ConSrvGetConsole(ProcessData, &Console, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't get console, status %lx\n", Status);
        return Status;
    }

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if ( /** !IsConsoleHandle(ConsoleHandle)    || **/
        Index >= ProcessData->HandleTableSize ||
        (Entry = &ProcessData->HandleTable[Index])->Object == NULL)
    {
        DPRINT1("Couldn't duplicate invalid handle %p\n", ConsoleHandle);
        Status = STATUS_INVALID_HANDLE;
        goto Quit;
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
            Status = STATUS_INVALID_PARAMETER;
            goto Quit;
        }
    }

    /* Insert the new handle inside the process handles table */
    Status = ConSrvInsertObject(ProcessData,
                                &DuplicateHandleRequest->ConsoleHandle, // Use the new handle value!
                                Entry->Object,
                                DesiredAccess,
                                DuplicateHandleRequest->Inheritable,
                                Entry->ShareMode);
    if (NT_SUCCESS(Status) &&
        (DuplicateHandleRequest->Options & DUPLICATE_CLOSE_SOURCE))
    {
        /* Close the original handle if needed */
        ConSrvCloseHandleEntry(Entry);
    }

Quit:
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

/* EOF */
