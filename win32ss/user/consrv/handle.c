/* $Id: handle.c 57570 2012-10-17 23:10:40Z hbelusca $
 *
 * reactos/subsys/csrss/api/handle.c
 *
 * CSRSS handle functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <w32csr.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static
BOOL
CsrIsConsoleHandle(HANDLE Handle)
{
    return ((ULONG_PTR)Handle & 0x10000003) == 0x3;
}

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

NTSTATUS
FASTCALL
Win32CsrReleaseObject(PCSR_PROCESS ProcessData,
                      HANDLE Handle)
{
    ULONG_PTR h = (ULONG_PTR)Handle >> 2;
    Object_t *Object;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    if (h >= ProcessData->HandleTableSize
            || (Object = ProcessData->HandleTable[h].Object) == NULL)
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
Win32CsrLockObject(PCSR_PROCESS ProcessData,
                   HANDLE Handle,
                   Object_t **Object,
                   DWORD Access,
                   LONG Type)
{
    ULONG_PTR h = (ULONG_PTR)Handle >> 2;

    DPRINT("CsrGetObject, Object: %x, %x, %x\n",
           Object, Handle, ProcessData ? ProcessData->HandleTableSize : 0);

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    if (!CsrIsConsoleHandle(Handle) || h >= ProcessData->HandleTableSize
            || (*Object = ProcessData->HandleTable[h].Object) == NULL
            || ~ProcessData->HandleTable[h].Access & Access
            || (Type != 0 && (*Object)->Type != Type))
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

VOID
WINAPI
Win32CsrReleaseConsole(PCSR_PROCESS ProcessData)
{
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

NTSTATUS
FASTCALL
Win32CsrInsertObject(PCSR_PROCESS ProcessData,
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
    return(STATUS_SUCCESS);
}

NTSTATUS
WINAPI
Win32CsrDuplicateHandleTable(PCSR_PROCESS SourceProcessData,
                             PCSR_PROCESS TargetProcessData)
{
    ULONG i;
    
    /* Only inherit if the flag was set */
    if (!TargetProcessData->bInheritHandles) return STATUS_SUCCESS;

    if (TargetProcessData->HandleTableSize)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlEnterCriticalSection(&SourceProcessData->HandleTableLock);

    /* we are called from CreateProcessData, it isn't necessary to lock the target process data */

    TargetProcessData->HandleTable = RtlAllocateHeap(ConSrvHeap,
                                                     HEAP_ZERO_MEMORY,
                                                     SourceProcessData->HandleTableSize
                                                             * sizeof(CSRSS_HANDLE));
    if (TargetProcessData->HandleTable == NULL)
    {
        RtlLeaveCriticalSection(&SourceProcessData->HandleTableLock);
        return(STATUS_UNSUCCESSFUL);
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
    return(STATUS_SUCCESS);
}

CSR_API(CsrGetHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCSR_PROCESS ProcessData = CsrGetClientThread()->Process;

    ApiMessage->Data.GetInputHandleRequest.Handle = INVALID_HANDLE_VALUE;

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    if (ProcessData->Console)
    {
        DWORD DesiredAccess = ApiMessage->Data.GetInputHandleRequest.Access;
        DWORD ShareMode = ApiMessage->Data.GetInputHandleRequest.ShareMode;

        PCSRSS_CONSOLE Console = ProcessData->Console;
        Object_t *Object;

        EnterCriticalSection(&Console->Lock);
        if (ApiMessage->ApiNumber == GET_OUTPUT_HANDLE)
            Object = &Console->ActiveBuffer->Header;
        else
            Object = &Console->Header;

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
            Status = Win32CsrInsertObject(ProcessData,
                                          &ApiMessage->Data.GetInputHandleRequest.Handle,
                                          Object,
                                          DesiredAccess,
                                          ApiMessage->Data.GetInputHandleRequest.Inheritable,
                                          ShareMode);
        }
        LeaveCriticalSection(&Console->Lock);
    }
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return Status;
}

// CSR_API(CsrSetHandle) ??

CSR_API(SrvCloseHandle)
{
    return Win32CsrReleaseObject(CsrGetClientThread()->Process, ApiMessage->Data.CloseHandleRequest.Handle);
}

CSR_API(SrvVerifyConsoleIoHandle)
{
    ULONG_PTR Index;
    NTSTATUS Status = STATUS_SUCCESS;
    PCSR_PROCESS ProcessData = CsrGetClientThread()->Process;

    Index = (ULONG_PTR)ApiMessage->Data.VerifyHandleRequest.Handle >> 2;
    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    if (Index >= ProcessData->HandleTableSize ||
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
    ULONG_PTR Index;
    PCSRSS_HANDLE Entry;
    DWORD DesiredAccess;
    PCSR_PROCESS ProcessData = CsrGetClientThread()->Process;

    Index = (ULONG_PTR)ApiMessage->Data.DuplicateHandleRequest.Handle >> 2;
    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    if (Index >= ProcessData->HandleTableSize
        || (Entry = &ProcessData->HandleTable[Index])->Object == NULL)
    {
        DPRINT1("Couldn't dup invalid handle %p\n", ApiMessage->Data.DuplicateHandleRequest.Handle);
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }

    if (ApiMessage->Data.DuplicateHandleRequest.Options & DUPLICATE_SAME_ACCESS)
    {
        DesiredAccess = Entry->Access;
    }
    else
    {
        DesiredAccess = ApiMessage->Data.DuplicateHandleRequest.Access;
        /* Make sure the source handle has all the desired flags */
        if (~Entry->Access & DesiredAccess)
        {
            DPRINT1("Handle %p only has access %X; requested %X\n",
                ApiMessage->Data.DuplicateHandleRequest.Handle, Entry->Access, DesiredAccess);
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return STATUS_INVALID_PARAMETER;
        }
    }

    ApiMessage->Status = Win32CsrInsertObject(ProcessData,
                                           &ApiMessage->Data.DuplicateHandleRequest.Handle,
                                           Entry->Object,
                                           DesiredAccess,
                                           ApiMessage->Data.DuplicateHandleRequest.Inheritable,
                                           Entry->ShareMode);
    if (NT_SUCCESS(ApiMessage->Status)
        && ApiMessage->Data.DuplicateHandleRequest.Options & DUPLICATE_CLOSE_SOURCE)
    {
        Win32CsrCloseHandleEntry(Entry);
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return ApiMessage->Status;
}

CSR_API(CsrGetInputWaitHandle)
{
    ApiMessage->Data.GetConsoleInputWaitHandle.InputWaitHandle = CsrGetClientThread()->Process->ConsoleEvent;
    return STATUS_SUCCESS;
}

/* EOF */
