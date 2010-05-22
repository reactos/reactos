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

static unsigned ObjectDefinitionsCount = 0;
static PCSRSS_OBJECT_DEFINITION ObjectDefinitions = NULL;

static
BOOL
CsrIsConsoleHandle(HANDLE Handle)
{
    return ((ULONG_PTR)Handle & 0x10000003) == 0x3;
}


NTSTATUS
FASTCALL
CsrRegisterObjectDefinitions(
    PCSRSS_OBJECT_DEFINITION NewDefinitions)
{
    unsigned NewCount;
    PCSRSS_OBJECT_DEFINITION Scan;
    PCSRSS_OBJECT_DEFINITION New;

    NewCount = 0;
    for (Scan = NewDefinitions; 0 != Scan->Type; Scan++)
    {
        NewCount++;
    }

    New = RtlAllocateHeap(Win32CsrApiHeap,
                          0,
                          (ObjectDefinitionsCount + NewCount)
                          * sizeof(CSRSS_OBJECT_DEFINITION));
    if (NULL == New)
    {
        DPRINT1("Unable to allocate memory\n");
        return STATUS_NO_MEMORY;
    }

    if (0 != ObjectDefinitionsCount)
    {
        RtlCopyMemory(New,
                      ObjectDefinitions,
                      ObjectDefinitionsCount * sizeof(CSRSS_OBJECT_DEFINITION));
        RtlFreeHeap(Win32CsrApiHeap, 0, ObjectDefinitions);
    }

    RtlCopyMemory(New + ObjectDefinitionsCount,
                  NewDefinitions,
                  NewCount * sizeof(CSRSS_OBJECT_DEFINITION));
    ObjectDefinitions = New;
    ObjectDefinitionsCount += NewCount;

    return STATUS_SUCCESS;
}

NTSTATUS
WINAPI
CsrGetObject(
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
WINAPI
CsrReleaseObjectByPointer(
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
WINAPI
CsrReleaseObject(
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

    return CsrReleaseObjectByPointer(Object);
}

NTSTATUS
WINAPI
CsrReleaseConsole(
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
            CsrReleaseObjectByPointer(HandleTable[i].Object);
    }
    RtlFreeHeap(Win32CsrApiHeap, 0, HandleTable);

    if (Console != NULL)
    {
        RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)&Console->Header.Lock);
        RemoveEntryList(&ProcessData->ProcessEntry);
        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)&Console->Header.Lock);
        CsrReleaseObjectByPointer(&Console->Header);
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
WINAPI
CsrInsertObject(
    PCSRSS_PROCESS_DATA ProcessData,
    PHANDLE Handle,
    Object_t *Object,
    DWORD Access,
    BOOL Inheritable)
{
    ULONG i;
    PVOID* Block;

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
        Block = _InterlockedExchangePointer((void* volatile)&ProcessData->HandleTable, Block);
        RtlFreeHeap( Win32CsrApiHeap, 0, Block );
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
CsrDuplicateHandleTable(
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

NTSTATUS
WINAPI
CsrVerifyObject(
    PCSRSS_PROCESS_DATA ProcessData,
    HANDLE Handle)
{
    ULONG_PTR h = (ULONG_PTR)Handle >> 2;

    if (h >= ProcessData->HandleTableSize ||
        ProcessData->HandleTable[h].Object == NULL)
    {
        return STATUS_INVALID_HANDLE;
    }

    return STATUS_SUCCESS;
}

CSR_API(CsrGetInputHandle)
{
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    if (ProcessData->Console)
    {
        Request->Status = CsrInsertObject(ProcessData,
                                          &Request->Data.GetInputHandleRequest.InputHandle,
                                          (Object_t *)ProcessData->Console,
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
        Request->Status = CsrInsertObject(ProcessData,
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

    return CsrReleaseObject(ProcessData, Request->Data.CloseHandleRequest.Handle);
}

CSR_API(CsrVerifyHandle)
{
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Request->Status = CsrVerifyObject(ProcessData, Request->Data.VerifyHandleRequest.Handle);
    if (!NT_SUCCESS(Request->Status))
    {
        DPRINT("CsrVerifyObject failed, status=%x\n", Request->Status);
    }

    return Request->Status;
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
    
    Request->Status = CsrInsertObject(ProcessData,
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
