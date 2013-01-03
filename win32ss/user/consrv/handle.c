/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/handle.c
 * PURPOSE:         Console IO Handle functions
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include "consrv.h"
#include "conio.h"

#define NDEBUG
#include <debug.h>


/* PRIVATE FUNCTIONS *********************************************************/

static INT
AdjustHandleCounts(PCSRSS_HANDLE Entry, INT Change)
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
Win32CsrCreateHandleEntry(PCSRSS_HANDLE Entry)
{
    Object_t *Object = Entry->Object;
    EnterCriticalSection(&Object->Console->Lock);
    AdjustHandleCounts(Entry, +1);
    LeaveCriticalSection(&Object->Console->Lock);
}

static VOID
Win32CsrCloseHandleEntry(PCSRSS_HANDLE Entry)
{
    Object_t *Object = Entry->Object;

    if (Object != NULL)
    {
        PCSRSS_CONSOLE Console = Object->Console;
        EnterCriticalSection(&Console->Lock);

        if (Object->Type == CONIO_CONSOLE_MAGIC)
        {
            // LIST_ENTRY WaitQueue;

            /*
             * Wake-up all of the writing waiters if any, dereference them
             * and purge them all from the list.
             */
            CsrNotifyWait(&Console->ReadWaitQueue,
                              WaitAll,
                              NULL,
                              (PVOID)0xdeaddead);
            // InitializeListHead(&WaitQueue);

            // CsrMoveSatisfiedWait(&WaitQueue, &Console->ReadWaitQueue);
            if (!IsListEmpty(&Console->ReadWaitQueue /* &WaitQueue */))
            {
                CsrDereferenceWait(&Console->ReadWaitQueue /* &WaitQueue */);
            }
        }

        /* If the last handle to a screen buffer is closed, delete it... */
        if (AdjustHandleCounts(Entry, -1) == 0)
        {
            if (Object->Type == CONIO_SCREEN_BUFFER_MAGIC)
            {
                PCSRSS_SCREEN_BUFFER Buffer = (PCSRSS_SCREEN_BUFFER)Object;
                /* ...unless it's the only buffer left. Windows allows deletion
                 * even of the last buffer, but having to deal with a lack of
                 * any active buffer might be error-prone. */
                if (Buffer->ListEntry.Flink != Buffer->ListEntry.Blink)
                    ConioDeleteScreenBuffer(Buffer);
            }
            else if (Object->Type == CONIO_CONSOLE_MAGIC)
            {
                /* TODO: FIXME: Destroy here the console ?? */
                // ConioDeleteConsole(Console);
            }
        }

        LeaveCriticalSection(&Console->Lock);
        Entry->Object = NULL;
    }
}


/* FUNCTIONS *****************************************************************/

NTSTATUS
FASTCALL
Win32CsrInsertObject(PCONSOLE_PROCESS_DATA ProcessData,
                     PHANDLE Handle,
                     Object_t *Object,
                     DWORD Access,
                     BOOL Inheritable,
                     DWORD ShareMode)
{
    ULONG i;
    PCSRSS_HANDLE Block;

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
                                (ProcessData->HandleTableSize + 64) * sizeof(CSRSS_HANDLE));
        if (Block == NULL)
        {
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return STATUS_UNSUCCESSFUL;
        }
        RtlCopyMemory(Block,
                      ProcessData->HandleTable,
                      ProcessData->HandleTableSize * sizeof(CSRSS_HANDLE));
        RtlFreeHeap(ConSrvHeap, 0, ProcessData->HandleTable);
        ProcessData->HandleTable = Block;
        ProcessData->HandleTableSize += 64;
    }
    ProcessData->HandleTable[i].Object      = Object;
    ProcessData->HandleTable[i].Access      = Access;
    ProcessData->HandleTable[i].Inheritable = Inheritable;
    ProcessData->HandleTable[i].ShareMode   = ShareMode;
    Win32CsrCreateHandleEntry(&ProcessData->HandleTable[i]);
    *Handle = UlongToHandle((i << 2) | 0x3);
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
Win32CsrReleaseObject(PCONSOLE_PROCESS_DATA ProcessData,
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
    Win32CsrCloseHandleEntry(&ProcessData->HandleTable[h]);

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
Win32CsrLockObject(PCONSOLE_PROCESS_DATA ProcessData,
                   HANDLE Handle,
                   Object_t **Object,
                   DWORD Access,
                   LONG Type)
{
    ULONG_PTR h = (ULONG_PTR)Handle >> 2;

    DPRINT("Win32CsrLockObject, Object: %x, %x, %x\n",
           Object, Handle, ProcessData ? ProcessData->HandleTableSize : 0);

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if ( !IsConsoleHandle(Handle) ||
         h >= ProcessData->HandleTableSize  ||
         (*Object = ProcessData->HandleTable[h].Object) == NULL ||
         ~ProcessData->HandleTable[h].Access & Access           ||
         (Type != 0 && (*Object)->Type != Type) )
    {
        DPRINT1("CsrGetObject returning invalid handle (%x)\n", Handle);
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }

    _InterlockedIncrement(&(*Object)->Console->ReferenceCount);
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    EnterCriticalSection(&((*Object)->Console->Lock));
    return STATUS_SUCCESS;
}

VOID FASTCALL
Win32CsrUnlockConsole(PCSRSS_CONSOLE Console)
{
    LeaveCriticalSection(&Console->Lock);

#if 0
    /* If it was the last held lock for the owning thread... */
    if (&Console->Lock.RecursionCount == 0)
    {
        /* ...dereference waiting threads if any */
        LIST_ENTRY WaitQueue;
        InitializeListHead(&WaitQueue);

        CsrMoveSatisfiedWait(&WaitQueue, Console->SatisfiedWaits);
        Console->SatisfiedWaits = NULL;
        if (!IsListEmpty(&WaitQueue))
        {
            CsrDereferenceWait(&WaitQueue);
        }
    }
#endif

    /* Decrement reference count */
    if (_InterlockedDecrement(&Console->ReferenceCount) == 0)
        ConioDeleteConsole(Console);
}

VOID
FASTCALL
Win32CsrUnlockObject(Object_t *Object)
{
    Win32CsrUnlockConsole(Object->Console);
}



/** Remark: this function can be called by SrvAttachConsole (not yet implemented) **/
NTSTATUS
NTAPI
ConsoleNewProcess(PCSR_PROCESS SourceProcess,
                  PCSR_PROCESS TargetProcess)
{
    /**************************************************************************
     * This function is called whenever a new process (GUI or CUI) is created.
     *
     * Copy the parent's handles table here if both the parent and the child
     * processes are CUI. If we must actually create our proper console (and
     * thus do not inherit from the console handles of the parent's), then we
     * will clean this table in the next ConsoleConnect call. Why we are doing
     * this? It's because here, we still don't know whether or not we must create
     * a new console instead of inherit it from the parent, and, because in
     * ConsoleConnect we don't have any reference to the parent process anymore.
     **************************************************************************/

    PCONSOLE_PROCESS_DATA SourceProcessData, TargetProcessData;
    ULONG i;

    DPRINT1("ConsoleNewProcess inside\n");
    DPRINT1("SourceProcess = 0x%p ; TargetProcess = 0x%p\n", SourceProcess, TargetProcess);

    /* An empty target process is invalid */
    if (!TargetProcess)
        return STATUS_INVALID_PARAMETER;

    DPRINT1("ConsoleNewProcess - OK\n");

    TargetProcessData = ConsoleGetPerProcessData(TargetProcess);
    DPRINT1("TargetProcessData = 0x%p\n", TargetProcessData);

    /**** HACK !!!! ****/ RtlZeroMemory(TargetProcessData, sizeof(*TargetProcessData));

    /* Initialize the new (target) process */
    TargetProcessData->Process = TargetProcess;
    TargetProcessData->ConsoleEvent = NULL;
    TargetProcessData->Console = TargetProcessData->ParentConsole = NULL;
    // TargetProcessData->bInheritHandles = FALSE;
    TargetProcessData->ConsoleApp = ((TargetProcess->Flags & CsrProcessIsConsoleApp) ? TRUE : FALSE);

    // Testing
    TargetProcessData->HandleTableSize = 0;
    TargetProcessData->HandleTable = NULL;

    /* HACK */ RtlZeroMemory(&TargetProcessData->HandleTableLock, sizeof(RTL_CRITICAL_SECTION));
    RtlInitializeCriticalSection(&TargetProcessData->HandleTableLock);

    /* Do nothing if the source process is NULL */
    if (!SourceProcess)
        return STATUS_SUCCESS;

    SourceProcessData = ConsoleGetPerProcessData(SourceProcess);
    DPRINT1("SourceProcessData = 0x%p\n", SourceProcessData);

    /*
     * If both of the processes (parent and new child) are console applications,
     * then try to inherit handles from the parent process.
     */
    if ( SourceProcessData->Console != NULL && /* SourceProcessData->ConsoleApp */
         TargetProcessData->ConsoleApp )
    {
/*
        if (TargetProcessData->HandleTableSize)
        {
            return STATUS_INVALID_PARAMETER;
        }
*/

        DPRINT1("ConsoleNewProcess - Copy the handle table (1)\n");
        /* Temporary "inherit" the console from the parent */
        TargetProcessData->ParentConsole = SourceProcessData->Console;
        RtlEnterCriticalSection(&SourceProcessData->HandleTableLock);
        DPRINT1("ConsoleNewProcess - Copy the handle table (2)\n");

        /* Allocate a new handle table for the child process */
        TargetProcessData->HandleTable = RtlAllocateHeap(ConSrvHeap,
                                                         HEAP_ZERO_MEMORY,
                                                         SourceProcessData->HandleTableSize
                                                                 * sizeof(CSRSS_HANDLE));
        if (TargetProcessData->HandleTable == NULL)
        {
            RtlLeaveCriticalSection(&SourceProcessData->HandleTableLock);
            return STATUS_UNSUCCESSFUL;
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
                 * pointed object (via the call to Win32CsrCreateHandleEntry).
                 */
                TargetProcessData->HandleTable[i] = SourceProcessData->HandleTable[i];
                Win32CsrCreateHandleEntry(&TargetProcessData->HandleTable[i]);
            }
        }

        RtlLeaveCriticalSection(&SourceProcessData->HandleTableLock);
    }
    else
    {
        DPRINT1("ConsoleNewProcess - We don't launch a Console process : SourceProcessData->Console = 0x%p ; TargetProcess->Flags = %lu\n", SourceProcessData->Console, TargetProcess->Flags);
    }

    return STATUS_SUCCESS;
}

// Temporary ; move it to a header.
NTSTATUS WINAPI CsrInitConsole(PCSRSS_CONSOLE* NewConsole, int ShowCmd);

NTSTATUS
NTAPI
ConsoleConnect(IN PCSR_PROCESS CsrProcess,
               IN OUT PVOID ConnectionInfo,
               IN OUT PULONG ConnectionInfoLength)
{
    /**************************************************************************
     * This function is called whenever a CUI new process is created.
     **************************************************************************/

    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_CONNECTION_INFO ConnectInfo = (PCONSOLE_CONNECTION_INFO)ConnectionInfo;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrProcess);
    BOOLEAN NewConsole = FALSE;
    // PCSRSS_CONSOLE Console = NULL;

    DPRINT1("ConsoleConnect\n");

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
        DPRINT("ConsoleConnect - No console needed\n");
        return STATUS_SUCCESS;
    }

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    /* If we don't have a console, then create a new one... */
    if (!ConnectInfo->Console ||
         ConnectInfo->Console != ProcessData->ParentConsole)
    {
        // PCSRSS_CONSOLE Console;

        DPRINT1("ConsoleConnect - Allocate a new console\n");

        /* Initialize a new Console */
        NewConsole = TRUE;
        Status = CsrInitConsole(&ProcessData->Console, ConnectInfo->ShowCmd);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Console initialization failed\n");
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return Status;
        }
    }
    else /* We inherit it from the parent */
    {
        DPRINT1("ConsoleConnect - Reuse current (parent's) console\n");

        /* Reuse our current console */
        NewConsole = FALSE;
        ProcessData->Console = ConnectInfo->Console;
    }

    /* Insert the process into the processes list of the console */
    InsertHeadList(&ProcessData->Console->ProcessList, &ProcessData->ConsoleLink);

    /* Return it to the caller */
    ConnectInfo->Console = ProcessData->Console;

    /* Add a reference count because the process is tied to the console */
    _InterlockedIncrement(&ProcessData->Console->ReferenceCount);

    if (NewConsole /* || !ProcessData->bInheritHandles */)
    {
        /*
         * We've just created a new console. However when ConsoleNewProcess was
         * called, we didn't know that we wanted to create a new console and
         * therefore, we by default inherited the handles table from our parent
         * process. It's only now that we notice that in fact we do not need
         * them, because we've created a new console and thus we must use it.
         *
         * Therefore, free our handles table and recreate a new one.
         */

        ULONG i;

        /* Close all console handles and free the handle table memory */
        for (i = 0; i < ProcessData->HandleTableSize; i++)
        {
            Win32CsrCloseHandleEntry(&ProcessData->HandleTable[i]);
        }
        ProcessData->HandleTableSize = 0;
        RtlFreeHeap(ConSrvHeap, 0, ProcessData->HandleTable);
        ProcessData->HandleTable = NULL;

        /*
         * Create a new handle table - Insert the IO handles
         */

        /* Insert the Input handle */
        Status = Win32CsrInsertObject(ProcessData,
                                      &ConnectInfo->InputHandle,
                                      &ProcessData->Console->Header,
                                      GENERIC_READ | GENERIC_WRITE,
                                      TRUE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to insert the input handle\n");
            ConioDeleteConsole(ProcessData->Console);
            ProcessData->Console = NULL;
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return Status;
        }

        /* Insert the Output handle */
        Status = Win32CsrInsertObject(ProcessData,
                                      &ConnectInfo->OutputHandle,
                                      &ProcessData->Console->ActiveBuffer->Header,
                                      GENERIC_READ | GENERIC_WRITE,
                                      TRUE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to insert the output handle\n");
            ConioDeleteConsole(ProcessData->Console);
            Win32CsrReleaseObject(ProcessData,
                                  ConnectInfo->InputHandle);
            ProcessData->Console = NULL;
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return Status;
        }

        /* Insert the Error handle */
        Status = Win32CsrInsertObject(ProcessData,
                                      &ConnectInfo->ErrorHandle,
                                      &ProcessData->Console->ActiveBuffer->Header,
                                      GENERIC_READ | GENERIC_WRITE,
                                      TRUE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to insert the error handle\n");
            ConioDeleteConsole(ProcessData->Console);
            Win32CsrReleaseObject(ProcessData,
                                  ConnectInfo->OutputHandle);
            Win32CsrReleaseObject(ProcessData,
                                  ConnectInfo->InputHandle);
            ProcessData->Console = NULL;
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return Status;
        }
    }

    /* Duplicate the Event */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               ProcessData->Console->ActiveEvent,
                               ProcessData->Process->ProcessHandle,
                               &ProcessData->ConsoleEvent,
                               EVENT_ALL_ACCESS, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject() failed: %lu\n", Status);
        ConioDeleteConsole(ProcessData->Console);
        if (NewConsole /* || !ProcessData->bInheritHandles */)
        {
            Win32CsrReleaseObject(ProcessData,
                                  ConnectInfo->ErrorHandle);
            Win32CsrReleaseObject(ProcessData,
                                  ConnectInfo->OutputHandle);
            Win32CsrReleaseObject(ProcessData,
                                  ConnectInfo->InputHandle);
        }
        ProcessData->Console = NULL;
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return Status;
    }
    /* Input Wait Handle */
    ConnectInfo->InputWaitHandle = ProcessData->ConsoleEvent;

    /* Set the Ctrl Dispatcher */
    ProcessData->CtrlDispatcher = ConnectInfo->CtrlDispatcher;
    DPRINT("CSRSS:CtrlDispatcher address: %x\n", ProcessData->CtrlDispatcher);

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return STATUS_SUCCESS;
}

VOID
WINAPI
Win32CsrReleaseConsole(PCSR_PROCESS Process)
{
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(Process);
    PCSRSS_CONSOLE Console;
    ULONG i;

    DPRINT1("Win32CsrReleaseConsole\n");

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    /* Close all console handles and free the handle table memory */
    for (i = 0; i < ProcessData->HandleTableSize; i++)
    {
        Win32CsrCloseHandleEntry(&ProcessData->HandleTable[i]);
    }
    ProcessData->HandleTableSize = 0;
    RtlFreeHeap(ConSrvHeap, 0, ProcessData->HandleTable);
    ProcessData->HandleTable = NULL;

    /* Detach process from console */
    Console = ProcessData->Console;
    if (Console != NULL)
    {
        DPRINT1("Win32CsrReleaseConsole - Console->ReferenceCount = %lu - We are going to decrement it !\n", Console->ReferenceCount);
        ProcessData->Console = NULL;
        EnterCriticalSection(&Console->Lock);
        RemoveEntryList(&ProcessData->ConsoleLink);
        Win32CsrUnlockConsole(Console);
        //CloseHandle(ProcessData->ConsoleEvent);
        //ProcessData->ConsoleEvent = NULL;
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
}

VOID
WINAPI
ConsoleDisconnect(PCSR_PROCESS Process)
{
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(Process);

    /**************************************************************************
     * This function is called whenever a new process (GUI or CUI) is destroyed.
     *
     * Only do something if the process is a CUI. <-- modify this behaviour if
     *                                                we deal with a GUI which
     *                                                quits and acquired a
     *                                                console...
     **************************************************************************/

    DPRINT1("ConsoleDisconnect called\n");
    // if (ProcessData->Console != NULL)
    if (ProcessData->ConsoleApp)
    {
        DPRINT1("ConsoleDisconnect - calling Win32CsrReleaseConsole\n");
        Win32CsrReleaseConsole(Process);
    }

    RtlDeleteCriticalSection(&ProcessData->HandleTableLock);
}



CSR_API(SrvCloseHandle)
{
    PCSRSS_CLOSE_HANDLE CloseHandleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CloseHandleRequest;

    return Win32CsrReleaseObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                 CloseHandleRequest->Handle);
}

CSR_API(SrvVerifyConsoleIoHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCSRSS_VERIFY_HANDLE VerifyHandleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.VerifyHandleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    HANDLE Handle = VerifyHandleRequest->Handle;
    ULONG_PTR Index = (ULONG_PTR)Handle >> 2;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if (!IsConsoleHandle(Handle)    ||
        Index >= ProcessData->HandleTableSize ||
        ProcessData->HandleTable[Index].Object == NULL)
    {
        DPRINT("CsrVerifyObject failed\n");
        Status = STATUS_INVALID_HANDLE;
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return Status;
}

CSR_API(SrvDuplicateHandle)
{
    PCSRSS_HANDLE Entry;
    DWORD DesiredAccess;
    PCSRSS_DUPLICATE_HANDLE DuplicateHandleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.DuplicateHandleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    HANDLE Handle = DuplicateHandleRequest->Handle;
    ULONG_PTR Index = (ULONG_PTR)Handle >> 2;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    if ( /** !IsConsoleHandle(Handle)    || **/
        Index >= ProcessData->HandleTableSize ||
        (Entry = &ProcessData->HandleTable[Index])->Object == NULL)
    {
        DPRINT1("Couldn't duplicate invalid handle %p\n", Handle);
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
        if (~Entry->Access & DesiredAccess)
        {
            DPRINT1("Handle %p only has access %X; requested %X\n",
                Handle, Entry->Access, DesiredAccess);
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return STATUS_INVALID_PARAMETER;
        }
    }

    ApiMessage->Status = Win32CsrInsertObject(ProcessData,
                                              &DuplicateHandleRequest->Handle, // Use the new handle value!
                                              Entry->Object,
                                              DesiredAccess,
                                              DuplicateHandleRequest->Inheritable,
                                              Entry->ShareMode);
    if (NT_SUCCESS(ApiMessage->Status) &&
        DuplicateHandleRequest->Options & DUPLICATE_CLOSE_SOURCE)
    {
        Win32CsrCloseHandleEntry(Entry);
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return ApiMessage->Status;
}

/**
CSR_API(CsrGetInputWaitHandle)
{
    PCSRSS_GET_INPUT_WAIT_HANDLE GetConsoleInputWaitHandle = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetConsoleInputWaitHandle;

    GetConsoleInputWaitHandle->InputWaitHandle =
        ConsoleGetPerProcessData(CsrGetClientThread()->Process)->ConsoleEvent;

    return STATUS_SUCCESS;
}
**/

/* EOF */
