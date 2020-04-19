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

#include <win/console.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* Console handle */
typedef struct _CONSOLE_IO_HANDLE
{
    PCONSOLE_IO_OBJECT Object;   /* The object on which the handle points to */
    ULONG   Access;
    ULONG   ShareMode;
    BOOLEAN Inheritable;
} CONSOLE_IO_HANDLE, *PCONSOLE_IO_HANDLE;


/* PRIVATE FUNCTIONS **********************************************************/

static LONG
AdjustHandleCounts(IN PCONSOLE_IO_HANDLE Handle,
                   IN LONG Change)
{
    PCONSOLE_IO_OBJECT Object = Handle->Object;

    DPRINT("AdjustHandleCounts(0x%p, %d), Object = 0x%p\n",
           Handle, Change, Object);
    DPRINT("\tAdjustHandleCounts(0x%p, %d), Object = 0x%p, Object->ReferenceCount = %d, Object->Type = %lu\n",
           Handle, Change, Object, Object->ReferenceCount, Object->Type);

    if (Handle->Access & GENERIC_READ)           Object->AccessRead += Change;
    if (Handle->Access & GENERIC_WRITE)          Object->AccessWrite += Change;
    if (!(Handle->ShareMode & FILE_SHARE_READ))  Object->ExclusiveRead += Change;
    if (!(Handle->ShareMode & FILE_SHARE_WRITE)) Object->ExclusiveWrite += Change;

    Object->ReferenceCount += Change;

    return Object->ReferenceCount;
}

static VOID
ConSrvCloseHandle(IN PCONSOLE_IO_HANDLE Handle)
{
    PCONSOLE_IO_OBJECT Object = Handle->Object;
    if (Object != NULL)
    {
        /*
         * If this is a input handle, notify and dereference
         * all the waits related to this handle.
         */
        if (Object->Type == INPUT_BUFFER)
        {
            // PCONSOLE_INPUT_BUFFER InputBuffer = (PCONSOLE_INPUT_BUFFER)Object;
            PCONSRV_CONSOLE Console = (PCONSRV_CONSOLE)Object->Console;

            /*
             * Wake up all the writing waiters related to this handle for this
             * input buffer, if any, then dereference them and purge them all
             * from the list.
             * To select them amongst all the waiters for this input buffer,
             * pass the handle pointer to the waiters, then they will check
             * whether or not they are related to this handle and if so, they
             * return.
             */
            CsrNotifyWait(&Console->ReadWaitQueue,
                          TRUE,
                          NULL,
                          (PVOID)Handle);
            if (!IsListEmpty(&Console->ReadWaitQueue))
            {
                CsrDereferenceWait(&Console->ReadWaitQueue);
            }
        }

        /* If the last handle to a screen buffer is closed, delete it... */
        if (AdjustHandleCounts(Handle, -1) == 0)
        {
            if (Object->Type == TEXTMODE_BUFFER || Object->Type == GRAPHICS_BUFFER)
            {
                PCONSOLE_SCREEN_BUFFER Buffer = (PCONSOLE_SCREEN_BUFFER)Object;
                /* ...unless it's the only buffer left. Windows allows deletion
                 * even of the last buffer, but having to deal with a lack of
                 * any active buffer might be error-prone. */
                if (Buffer->ListEntry.Flink != Buffer->ListEntry.Blink)
                    ConDrvDeleteScreenBuffer(Buffer);
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

        /* Invalidate (zero-out) this handle entry */
        // Handle->Object = NULL;
        // RtlZeroMemory(Handle, sizeof(*Handle));
    }
    RtlZeroMemory(Handle, sizeof(*Handle)); // Be sure the whole entry is invalidated.
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
             * pointed object (via the call to ConSrvCreateHandleEntry == AdjustHandleCounts).
             */
            TargetProcessData->HandleTable[j] = SourceProcessData->HandleTable[i];
            AdjustHandleCounts(&TargetProcessData->HandleTable[j], +1);
            ++j;
        }
    }

Quit:
    RtlLeaveCriticalSection(&SourceProcessData->HandleTableLock);
    return Status;
}

VOID
ConSrvFreeHandlesTable(IN PCONSOLE_PROCESS_DATA ProcessData)
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
                ConSrvCloseHandle(&ProcessData->HandleTable[i]);
            }
        }
        /* Free the handles table memory */
        ConsoleFreeHeap(ProcessData->HandleTable);
        ProcessData->HandleTable = NULL;
    }

    ProcessData->HandleTableSize = 0;

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
}






// ConSrvCreateObject
VOID
ConSrvInitObject(IN OUT PCONSOLE_IO_OBJECT Object,
                 IN CONSOLE_IO_OBJECT_TYPE Type,
                 IN PCONSOLE Console)
{
    ASSERT(Object);
    // if (!Object) return;

    Object->Type    = Type;
    Object->Console = Console;
    Object->ReferenceCount = 0;

    Object->AccessRead    = Object->AccessWrite    = 0;
    Object->ExclusiveRead = Object->ExclusiveWrite = 0;
}

NTSTATUS
ConSrvInsertObject(IN PCONSOLE_PROCESS_DATA ProcessData,
                   OUT PHANDLE Handle,
                   IN PCONSOLE_IO_OBJECT Object,
                   IN ULONG Access,
                   IN BOOLEAN Inheritable,
                   IN ULONG ShareMode)
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
    AdjustHandleCounts(&ProcessData->HandleTable[i], +1);
    *Handle = ULongToHandle((i << 2) | 0x3);

    // RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return STATUS_SUCCESS;
}

NTSTATUS
ConSrvRemoveObject(IN PCONSOLE_PROCESS_DATA ProcessData,
                   IN HANDLE Handle)
{
    ULONG Index = HandleToULong(Handle) >> 2;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    ASSERT(ProcessData->HandleTable);
    // ASSERT( (ProcessData->HandleTable == NULL && ProcessData->HandleTableSize == 0) ||
    //         (ProcessData->HandleTable != NULL && ProcessData->HandleTableSize != 0) );

    if (Index >= ProcessData->HandleTableSize ||
        ProcessData->HandleTable[Index].Object == NULL)
    {
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }

    ASSERT(ProcessData->ConsoleHandle);
    ConSrvCloseHandle(&ProcessData->HandleTable[Index]);

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return STATUS_SUCCESS;
}

NTSTATUS
ConSrvGetObject(IN PCONSOLE_PROCESS_DATA ProcessData,
                IN HANDLE Handle,
                OUT PCONSOLE_IO_OBJECT* Object,
                OUT PVOID* Entry OPTIONAL,
                IN ULONG Access,
                IN BOOLEAN LockConsole,
                IN CONSOLE_IO_OBJECT_TYPE Type)
{
    // NTSTATUS Status;
    ULONG Index = HandleToULong(Handle) >> 2;
    PCONSOLE_IO_HANDLE HandleEntry = NULL;
    PCONSOLE_IO_OBJECT ObjectEntry = NULL;
    // PCONSRV_CONSOLE ObjectConsole;

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

    // Status = ConSrvGetConsole(ProcessData, &ObjectConsole, LockConsole);
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
ConSrvReleaseObject(IN PCONSOLE_IO_OBJECT Object,
                    IN BOOLEAN IsConsoleLocked)
{
    PCONSRV_CONSOLE ObjectConsole = (PCONSRV_CONSOLE)Object->Console;
    ConSrvReleaseConsole(ObjectConsole, IsConsoleLocked);
}


/* PUBLIC SERVER APIS *********************************************************/

/* API_NUMBER: ConsolepOpenConsole */
CON_API(SrvOpenConsole,
        CONSOLE_OPENCONSOLE, OpenConsoleRequest)
{
    /*
     * This API opens a handle to either the input buffer or to
     * a screen-buffer of the console of the current process.
     */

    NTSTATUS Status;
    DWORD DesiredAccess = OpenConsoleRequest->DesiredAccess;
    DWORD ShareMode = OpenConsoleRequest->ShareMode;
    PCONSOLE_IO_OBJECT Object;

    OpenConsoleRequest->Handle = INVALID_HANDLE_VALUE;

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
                                    &OpenConsoleRequest->Handle,
                                    Object,
                                    DesiredAccess,
                                    OpenConsoleRequest->InheritHandle,
                                    ShareMode);
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return Status;
}

/* API_NUMBER: ConsolepDuplicateHandle */
CON_API(SrvDuplicateHandle,
        CONSOLE_DUPLICATEHANDLE, DuplicateHandleRequest)
{
    NTSTATUS Status;
    HANDLE SourceHandle = DuplicateHandleRequest->SourceHandle;
    ULONG Index = HandleToULong(SourceHandle) >> 2;
    PCONSOLE_IO_HANDLE Entry;
    DWORD DesiredAccess;

    DuplicateHandleRequest->TargetHandle = INVALID_HANDLE_VALUE;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    // ASSERT( (ProcessData->HandleTable == NULL && ProcessData->HandleTableSize == 0) ||
    //         (ProcessData->HandleTable != NULL && ProcessData->HandleTableSize != 0) );

    if ( /** !IsConsoleHandle(SourceHandle)   || **/
        Index >= ProcessData->HandleTableSize ||
        (Entry = &ProcessData->HandleTable[Index])->Object == NULL)
    {
        DPRINT1("Couldn't duplicate invalid handle 0x%p\n", SourceHandle);
        Status = STATUS_INVALID_HANDLE;
        goto Quit;
    }

    if (DuplicateHandleRequest->Options & DUPLICATE_SAME_ACCESS)
    {
        DesiredAccess = Entry->Access;
    }
    else
    {
        DesiredAccess = DuplicateHandleRequest->DesiredAccess;
        /* Make sure the source handle has all the desired flags */
        if ((Entry->Access & DesiredAccess) == 0)
        {
            DPRINT1("Handle 0x%p only has access %X; requested %X\n",
                    SourceHandle, Entry->Access, DesiredAccess);
            Status = STATUS_INVALID_PARAMETER;
            goto Quit;
        }
    }

    /* Insert the new handle inside the process handles table */
    Status = ConSrvInsertObject(ProcessData,
                                &DuplicateHandleRequest->TargetHandle,
                                Entry->Object,
                                DesiredAccess,
                                DuplicateHandleRequest->InheritHandle,
                                Entry->ShareMode);
    if (NT_SUCCESS(Status) &&
        (DuplicateHandleRequest->Options & DUPLICATE_CLOSE_SOURCE))
    {
        /* Close the original handle if needed */
        ConSrvCloseHandle(Entry);
    }

Quit:
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return Status;
}

/* API_NUMBER: ConsolepGetHandleInformation */
CON_API(SrvGetHandleInformation,
        CONSOLE_GETHANDLEINFO, GetHandleInfoRequest)
{
    NTSTATUS Status;
    HANDLE Handle = GetHandleInfoRequest->Handle;
    ULONG Index = HandleToULong(Handle) >> 2;
    PCONSOLE_IO_HANDLE Entry;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    ASSERT(ProcessData->HandleTable);
    // ASSERT( (ProcessData->HandleTable == NULL && ProcessData->HandleTableSize == 0) ||
    //         (ProcessData->HandleTable != NULL && ProcessData->HandleTableSize != 0) );

    if (!IsConsoleHandle(Handle)              ||
        Index >= ProcessData->HandleTableSize ||
        (Entry = &ProcessData->HandleTable[Index])->Object == NULL)
    {
        Status = STATUS_INVALID_HANDLE;
        goto Quit;
    }

    /*
     * Retrieve the handle information flags. The console server
     * doesn't support HANDLE_FLAG_PROTECT_FROM_CLOSE.
     */
    GetHandleInfoRequest->Flags = 0;
    if (Entry->Inheritable) GetHandleInfoRequest->Flags |= HANDLE_FLAG_INHERIT;

    Status = STATUS_SUCCESS;

Quit:
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return Status;
}

/* API_NUMBER: ConsolepSetHandleInformation */
CON_API(SrvSetHandleInformation,
        CONSOLE_SETHANDLEINFO, SetHandleInfoRequest)
{
    NTSTATUS Status;
    HANDLE Handle = SetHandleInfoRequest->Handle;
    ULONG Index = HandleToULong(Handle) >> 2;
    PCONSOLE_IO_HANDLE Entry;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    ASSERT(ProcessData->HandleTable);
    // ASSERT( (ProcessData->HandleTable == NULL && ProcessData->HandleTableSize == 0) ||
    //         (ProcessData->HandleTable != NULL && ProcessData->HandleTableSize != 0) );

    if (!IsConsoleHandle(Handle)              ||
        Index >= ProcessData->HandleTableSize ||
        (Entry = &ProcessData->HandleTable[Index])->Object == NULL)
    {
        Status = STATUS_INVALID_HANDLE;
        goto Quit;
    }

    /*
     * Modify the handle information flags. The console server
     * doesn't support HANDLE_FLAG_PROTECT_FROM_CLOSE.
     */
    if (SetHandleInfoRequest->Mask & HANDLE_FLAG_INHERIT)
    {
        Entry->Inheritable = ((SetHandleInfoRequest->Flags & HANDLE_FLAG_INHERIT) != 0);
    }

    Status = STATUS_SUCCESS;

Quit:
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return Status;
}

/* API_NUMBER: ConsolepCloseHandle */
CON_API(SrvCloseHandle,
        CONSOLE_CLOSEHANDLE, CloseHandleRequest)
{
    return ConSrvRemoveObject(ProcessData, CloseHandleRequest->Handle);
}

/* API_NUMBER: ConsolepVerifyIoHandle */
CON_API(SrvVerifyConsoleIoHandle,
        CONSOLE_VERIFYHANDLE, VerifyHandleRequest)
{
    HANDLE IoHandle = VerifyHandleRequest->Handle;
    ULONG Index = HandleToULong(IoHandle) >> 2;

    VerifyHandleRequest->IsValid = FALSE;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    // ASSERT( (ProcessData->HandleTable == NULL && ProcessData->HandleTableSize == 0) ||
    //         (ProcessData->HandleTable != NULL && ProcessData->HandleTableSize != 0) );

    if (!IsConsoleHandle(IoHandle)            ||
        Index >= ProcessData->HandleTableSize ||
        ProcessData->HandleTable[Index].Object == NULL)
    {
        DPRINT("SrvVerifyConsoleIoHandle failed\n");
    }
    else
    {
        VerifyHandleRequest->IsValid = TRUE;
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return STATUS_SUCCESS;
}

/* EOF */
