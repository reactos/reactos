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
        /* If the last handle to a screen buffer is closed, delete it */
        if (AdjustHandleCounts(Entry, -1) == 0
            && Object->Type == CONIO_SCREEN_BUFFER_MAGIC)
        {
            PCSRSS_SCREEN_BUFFER Buffer = (PCSRSS_SCREEN_BUFFER)Object;
            /* ...unless it's the only buffer left. Windows allows deletion
             * even of the last buffer, but having to deal with a lack of
             * any active buffer might be error-prone. */
            if (Buffer->ListEntry.Flink != Buffer->ListEntry.Blink)
                ConioDeleteScreenBuffer(Buffer);
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
            return(STATUS_UNSUCCESSFUL);
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

VOID
FASTCALL
Win32CsrUnlockObject(Object_t *Object)
{
    PCSRSS_CONSOLE Console = Object->Console;
    LeaveCriticalSection(&Console->Lock);
    /* dec ref count */
    if (_InterlockedDecrement(&Console->ReferenceCount) == 0)
        ConioDeleteConsole(&Console->Header);
}



NTSTATUS
NTAPI
ConsoleNewProcess(PCSR_PROCESS SourceProcess,
                  PCSR_PROCESS TargetProcess)
{
    PCONSOLE_PROCESS_DATA SourceProcessData, TargetProcessData;
    ULONG i;

    DPRINT1("ConsoleNewProcess inside\n");
    DPRINT1("SourceProcess = 0x%p ; TargetProcess = 0x%p\n", SourceProcess, TargetProcess);

    /* An empty target process is invalid */
    if (!TargetProcess)
        return STATUS_INVALID_PARAMETER;

    DPRINT1("ConsoleNewProcess - OK\n");

    TargetProcessData = ConsoleGetPerProcessData(TargetProcess);

    /* Initialize the new (target) process */
    TargetProcessData->Process = TargetProcess;
    RtlInitializeCriticalSection(&TargetProcessData->HandleTableLock);

    /* Do nothing if the source process is NULL */
    if (!SourceProcess)
        return STATUS_SUCCESS;

    SourceProcessData = ConsoleGetPerProcessData(SourceProcess);

    // TODO: Check if one of the processes is really a CONSOLE.
    /*
    if (!(CreateProcessRequest->CreationFlags & (CREATE_NEW_CONSOLE | DETACHED_PROCESS)))
    {
        // NewProcess == TargetProcess.
        NewProcess->ParentConsole = Process->Console;
        NewProcess->bInheritHandles = CreateProcessRequest->bInheritHandles;
    }
    */

    /* Only inherit if the if the flag was set */
    if (!TargetProcessData->bInheritHandles) return STATUS_SUCCESS;

    if (TargetProcessData->HandleTableSize)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlEnterCriticalSection(&SourceProcessData->HandleTableLock);

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

    for (i = 0; i < SourceProcessData->HandleTableSize; i++)
    {
        if (SourceProcessData->HandleTable[i].Object != NULL &&
            SourceProcessData->HandleTable[i].Inheritable)
        {
            TargetProcessData->HandleTable[i] = SourceProcessData->HandleTable[i];
            Win32CsrCreateHandleEntry(&TargetProcessData->HandleTable[i]);
        }
    }

    RtlLeaveCriticalSection(&SourceProcessData->HandleTableLock);

    return STATUS_SUCCESS;
}

VOID
WINAPI
Win32CsrReleaseConsole(PCSR_PROCESS Process)
{
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(Process);
    PCSRSS_CONSOLE Console;
    ULONG i;

    /* Close all console handles and detach process from console */
    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    for (i = 0; i < ProcessData->HandleTableSize; i++)
        Win32CsrCloseHandleEntry(&ProcessData->HandleTable[i]);
    ProcessData->HandleTableSize = 0;
    RtlFreeHeap(ConSrvHeap, 0, ProcessData->HandleTable);
    ProcessData->HandleTable = NULL;

    Console = ProcessData->Console;
    if (Console != NULL)
    {
        ProcessData->Console = NULL;
        EnterCriticalSection(&Console->Lock);
        RemoveEntryList(&ProcessData->ConsoleLink);
        LeaveCriticalSection(&Console->Lock);
        if (_InterlockedDecrement(&Console->ReferenceCount) == 0)
            ConioDeleteConsole(&Console->Header);
        //CloseHandle(ProcessData->ConsoleEvent);
        //ProcessData->ConsoleEvent = NULL;
    }
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
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

CSR_API(CsrGetInputWaitHandle)
{
    PCSRSS_GET_INPUT_WAIT_HANDLE GetConsoleInputWaitHandle = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetConsoleInputWaitHandle;

    GetConsoleInputWaitHandle->InputWaitHandle =
        ConsoleGetPerProcessData(CsrGetClientThread()->Process)->ConsoleEvent;

    return STATUS_SUCCESS;
}

/* EOF */
