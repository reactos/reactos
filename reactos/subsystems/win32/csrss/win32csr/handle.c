/* $Id$
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

static unsigned ObjectDefinitionsCount = 2;
static CSRSS_OBJECT_DEFINITION ObjectDefinitions[] =
{
    { CONIO_CONSOLE_MAGIC,       ConioDeleteConsole },
    { CONIO_SCREEN_BUFFER_MAGIC, ConioDeleteScreenBuffer },
};

static
BOOL
CsrIsConsoleHandle(HANDLE Handle)
{
    return ((ULONG_PTR)Handle & 0x10000003) == 0x3;
}

NTSTATUS
FASTCALL
Win32CsrGetObject(
    PCSRSS_PROCESS_DATA ProcessData,
    HANDLE Handle,
    Object_t **Object,
    DWORD Access )
{
    ULONG_PTR h = (ULONG_PTR)Handle >> 2;

    DPRINT("CsrGetObject, Object: %x, %x, %x\n", 
           Object, Handle, ProcessData ? ProcessData->HandleTableSize : 0);

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    if (!CsrIsConsoleHandle(Handle) || h >= ProcessData->HandleTableSize
            || (*Object = ProcessData->HandleTable[h].Object) == NULL
            || ~ProcessData->HandleTable[h].Access & Access)
    {
        DPRINT1("CsrGetObject returning invalid handle (%x)\n", Handle);
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }
    _InterlockedIncrement(&(*Object)->ReferenceCount);
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    //   DbgPrint( "CsrGetObject returning\n" );
    return STATUS_SUCCESS;
}


NTSTATUS
FASTCALL
Win32CsrReleaseObjectByPointer(
    Object_t *Object)
{
    unsigned DefIndex;

    /* dec ref count */
    if (_InterlockedDecrement(&Object->ReferenceCount) == 0)
    {
        for (DefIndex = 0; DefIndex < ObjectDefinitionsCount; DefIndex++)
        {
            if (Object->Type == ObjectDefinitions[DefIndex].Type)
            {
                (ObjectDefinitions[DefIndex].CsrCleanupObjectProc)(Object);
                return STATUS_SUCCESS;
            }
        }

        DPRINT1("CSR: Error: releasing unknown object type 0x%x", Object->Type);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
Win32CsrReleaseObject(
    PCSRSS_PROCESS_DATA ProcessData,
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
    ProcessData->HandleTable[h].Object = NULL;
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    return Win32CsrReleaseObjectByPointer(Object);
}

NTSTATUS
FASTCALL
Win32CsrLockObject(PCSRSS_PROCESS_DATA ProcessData,
                   HANDLE Handle,
                   Object_t **Object,
                   DWORD Access,
                   LONG Type)
{
    NTSTATUS Status;

    Status = Win32CsrGetObject(ProcessData, Handle, Object, Access);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    if ((*Object)->Type != Type)
    {
        Win32CsrReleaseObjectByPointer(*Object);
        return STATUS_INVALID_HANDLE;
    }

    EnterCriticalSection(&((*Object)->Lock));

    return STATUS_SUCCESS;
}

VOID
FASTCALL
Win32CsrUnlockObject(Object_t *Object)
{
    LeaveCriticalSection(&(Object->Lock));
    Win32CsrReleaseObjectByPointer(Object);
}

NTSTATUS
WINAPI
Win32CsrReleaseConsole(
    PCSRSS_PROCESS_DATA ProcessData)
{
    ULONG HandleTableSize;
    PCSRSS_HANDLE HandleTable;
    PCSRSS_CONSOLE Console;
    ULONG i;

    /* Close all console handles and detach process from console */
    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    HandleTableSize = ProcessData->HandleTableSize;
    HandleTable = ProcessData->HandleTable;
    Console = ProcessData->Console;
    ProcessData->HandleTableSize = 0;
    ProcessData->HandleTable = NULL;
    ProcessData->Console = NULL;
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    for (i = 0; i < HandleTableSize; i++)
    {
        if (HandleTable[i].Object != NULL)
            Win32CsrReleaseObjectByPointer(HandleTable[i].Object);
    }
    RtlFreeHeap(Win32CsrApiHeap, 0, HandleTable);

    if (Console != NULL)
    {
        EnterCriticalSection(&Console->Header.Lock);
        RemoveEntryList(&ProcessData->ProcessEntry);
        LeaveCriticalSection(&Console->Header.Lock);
        Win32CsrReleaseObjectByPointer(&Console->Header);
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
FASTCALL
Win32CsrInsertObject(
    PCSRSS_PROCESS_DATA ProcessData,
    PHANDLE Handle,
    Object_t *Object,
    DWORD Access,
    BOOL Inheritable)
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
        Block = RtlAllocateHeap(Win32CsrApiHeap,
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
        RtlFreeHeap(Win32CsrApiHeap, 0, ProcessData->HandleTable);
        ProcessData->HandleTable = Block;
        ProcessData->HandleTableSize += 64;
    }
    ProcessData->HandleTable[i].Object = Object;
    ProcessData->HandleTable[i].Access = Access;
    ProcessData->HandleTable[i].Inheritable = Inheritable;
    *Handle = UlongToHandle((i << 2) | 0x3);
    _InterlockedIncrement( &Object->ReferenceCount );
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return(STATUS_SUCCESS);
}

NTSTATUS
WINAPI
Win32CsrDuplicateHandleTable(
    PCSRSS_PROCESS_DATA SourceProcessData,
    PCSRSS_PROCESS_DATA TargetProcessData)
{
    ULONG i;

    if (TargetProcessData->HandleTableSize)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlEnterCriticalSection(&SourceProcessData->HandleTableLock);

    /* we are called from CreateProcessData, it isn't necessary to lock the target process data */

    TargetProcessData->HandleTable = RtlAllocateHeap(Win32CsrApiHeap,
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
            _InterlockedIncrement( &SourceProcessData->HandleTable[i].Object->ReferenceCount );
        }
    }
    RtlLeaveCriticalSection(&SourceProcessData->HandleTableLock);
    return(STATUS_SUCCESS);
}

CSR_API(CsrGetInputHandle)
{
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    if (ProcessData->Console)
    {
        Request->Status = Win32CsrInsertObject(ProcessData,
                                               &Request->Data.GetInputHandleRequest.InputHandle,
                                               &ProcessData->Console->Header,
                                               Request->Data.GetInputHandleRequest.Access,
                                               Request->Data.GetInputHandleRequest.Inheritable);
    }
    else
    {
        Request->Data.GetInputHandleRequest.InputHandle = INVALID_HANDLE_VALUE;
        Request->Status = STATUS_SUCCESS;
    }

    return Request->Status;
}

CSR_API(CsrGetOutputHandle)
{
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    if (ProcessData->Console)
    {
        Request->Status = Win32CsrInsertObject(ProcessData,
                                               &Request->Data.GetOutputHandleRequest.OutputHandle,
                                               &ProcessData->Console->ActiveBuffer->Header,
                                               Request->Data.GetOutputHandleRequest.Access,
                                               Request->Data.GetOutputHandleRequest.Inheritable);
    }
    else
    {
        Request->Data.GetOutputHandleRequest.OutputHandle = INVALID_HANDLE_VALUE;
        Request->Status = STATUS_SUCCESS;
    }

    return Request->Status;
}

CSR_API(CsrCloseHandle)
{
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    return Win32CsrReleaseObject(ProcessData, Request->Data.CloseHandleRequest.Handle);
}

CSR_API(CsrVerifyHandle)
{
    ULONG_PTR Index;
    NTSTATUS Status = STATUS_SUCCESS;

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Index = (ULONG_PTR)Request->Data.VerifyHandleRequest.Handle >> 2;
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

CSR_API(CsrDuplicateHandle)
{
    ULONG_PTR Index;
    PCSRSS_HANDLE Entry;
    DWORD DesiredAccess;

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Index = (ULONG_PTR)Request->Data.DuplicateHandleRequest.Handle >> 2;
    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    if (Index >= ProcessData->HandleTableSize
        || (Entry = &ProcessData->HandleTable[Index])->Object == NULL)
    {
        DPRINT1("Couldn't dup invalid handle %p\n", Request->Data.DuplicateHandleRequest.Handle);
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }

    if (Request->Data.DuplicateHandleRequest.Options & DUPLICATE_SAME_ACCESS)
    {
        DesiredAccess = Entry->Access;
    }
    else
    {
        DesiredAccess = Request->Data.DuplicateHandleRequest.Access;
        /* Make sure the source handle has all the desired flags */
        if (~Entry->Access & DesiredAccess)
        {
            DPRINT1("Handle %p only has access %X; requested %X\n",
                Request->Data.DuplicateHandleRequest.Handle, Entry->Access, DesiredAccess);
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return STATUS_INVALID_PARAMETER;
        }
    }
    
    Request->Status = Win32CsrInsertObject(ProcessData,
                                           &Request->Data.DuplicateHandleRequest.Handle,
                                           Entry->Object,
                                           DesiredAccess,
                                           Request->Data.DuplicateHandleRequest.Inheritable);
    if (NT_SUCCESS(Request->Status)
        && Request->Data.DuplicateHandleRequest.Options & DUPLICATE_CLOSE_SOURCE)
    {
        /* Close the original handle. This cannot drop the count to 0, since a new handle now exists */
        _InterlockedDecrement(&Entry->Object->ReferenceCount);
        Entry->Object = NULL;
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return Request->Status;
}

CSR_API(CsrGetInputWaitHandle)
{
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Request->Data.GetConsoleInputWaitHandle.InputWaitHandle = ProcessData->ConsoleEvent;
    return STATUS_SUCCESS;
}

/* EOF */
