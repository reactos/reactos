/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Handle table
 * FILE:            lib/rtl/handle.c
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

VOID
NTAPI
RtlInitializeHandleTable(
    ULONG TableSize,
    ULONG HandleSize,
    PRTL_HANDLE_TABLE HandleTable)
{
    /* Initialize handle table */
    memset(HandleTable, 0, sizeof(RTL_HANDLE_TABLE));
    HandleTable->MaximumNumberOfHandles = TableSize;
    HandleTable->SizeOfHandleTableEntry = HandleSize;
}


/*
 * @implemented
 */
VOID
NTAPI
RtlDestroyHandleTable(
    PRTL_HANDLE_TABLE HandleTable)
{
    PVOID ArrayPointer;
    SIZE_T ArraySize = 0;

    /* free handle array */
    if (HandleTable->CommittedHandles)
    {
        ArrayPointer = (PVOID)HandleTable->CommittedHandles;
        NtFreeVirtualMemory(NtCurrentProcess(),
                            &ArrayPointer,
                            &ArraySize,
                            MEM_RELEASE);
    }
}


/*
 * @implemented
 */
PRTL_HANDLE_TABLE_ENTRY
NTAPI
RtlAllocateHandle(
    PRTL_HANDLE_TABLE HandleTable,
    PULONG Index)
{
    PRTL_HANDLE_TABLE_ENTRY CurrentEntry, NextEntry;
    NTSTATUS Status;
    PRTL_HANDLE_TABLE_ENTRY HandleEntry;
    PVOID ArrayPointer;
    SIZE_T ArraySize;
    ULONG i, NumberOfEntries;

    /* Check if we are out of free handles entries */
    if (HandleTable->FreeHandles == NULL)
    {
        /* Check if we don't have uncomitted handle entries yet */
        if (HandleTable->UnCommittedHandles == NULL)
        {
            /* Use the maximum number of handle entries */
            ArraySize = HandleTable->SizeOfHandleTableEntry * HandleTable->MaximumNumberOfHandles;
            ArrayPointer = NULL;

            /* Reserve memory */
            Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                             &ArrayPointer,
                                             0,
                                             &ArraySize,
                                             MEM_RESERVE,
                                             PAGE_READWRITE);
            if (!NT_SUCCESS(Status))
                return NULL;

            /* Update handle array pointers */
            HandleTable->UnCommittedHandles = (PRTL_HANDLE_TABLE_ENTRY)ArrayPointer;
            HandleTable->MaxReservedHandles = (PRTL_HANDLE_TABLE_ENTRY)((ULONG_PTR)ArrayPointer + ArraySize);
        }

        /* Commit one reserved handle entry page */
        ArraySize = PAGE_SIZE;
        ArrayPointer = HandleTable->UnCommittedHandles;
        Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                         &ArrayPointer,
                                         0,
                                         &ArraySize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
            return NULL;

        /* Update handle array pointers */
        HandleTable->FreeHandles = (PRTL_HANDLE_TABLE_ENTRY)ArrayPointer;
        HandleTable->CommittedHandles = (PRTL_HANDLE_TABLE_ENTRY)ArrayPointer;
        HandleTable->UnCommittedHandles = (PRTL_HANDLE_TABLE_ENTRY)((ULONG_PTR)ArrayPointer + ArraySize);

        /* Calculate the number of entries we can store in the array */
        NumberOfEntries = ArraySize / HandleTable->SizeOfHandleTableEntry;

        /* Loop all entries, except the last one */
        CurrentEntry = HandleTable->FreeHandles;
        for (i = 0; i < NumberOfEntries - 1; i++)
        {
            /* Calculate the address of the next handle entry */
            NextEntry = (PRTL_HANDLE_TABLE_ENTRY)((ULONG_PTR)CurrentEntry +
                                                  HandleTable->SizeOfHandleTableEntry);

            /* Link the next entry */
            CurrentEntry->NextFree = NextEntry;

            /* Continue with the next entry */
            CurrentEntry = NextEntry;
        }

        /* CurrentEntry now points to the last entry, terminate the list here */
        CurrentEntry->NextFree = NULL;
    }

    /* remove handle from free list */
    HandleEntry = HandleTable->FreeHandles;
    HandleTable->FreeHandles = HandleEntry->NextFree;
    HandleEntry->NextFree = NULL;

    if (Index)
    {
        *Index = ((ULONG)((ULONG_PTR)HandleEntry - (ULONG_PTR)HandleTable->CommittedHandles) /
                  HandleTable->SizeOfHandleTableEntry);
    }

    return HandleEntry;
}


/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlFreeHandle(
    PRTL_HANDLE_TABLE HandleTable,
    PRTL_HANDLE_TABLE_ENTRY Handle)
{
#if DBG
    /* check if handle is valid */
    if (!RtlIsValidHandle(HandleTable, Handle))
    {
        DPRINT1("Invalid Handle! HandleTable=0x%p, Handle=0x%p, Handle->Flags=0x%x\n",
                HandleTable, Handle, Handle ? Handle->Flags : 0);
        return FALSE;
    }
#endif

    /* clear handle */
    memset(Handle, 0, HandleTable->SizeOfHandleTableEntry);

    /* add handle to free list */
    Handle->NextFree = HandleTable->FreeHandles;
    HandleTable->FreeHandles = Handle;

    return TRUE;
}


/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlIsValidHandle(
    PRTL_HANDLE_TABLE HandleTable,
    PRTL_HANDLE_TABLE_ENTRY Handle)
{
    if ((HandleTable != NULL)
            && (Handle >= HandleTable->CommittedHandles)
            && (Handle < HandleTable->MaxReservedHandles)
            && (Handle->Flags & RTL_HANDLE_VALID))
    {
        return TRUE;
    }
    return FALSE;
}


/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlIsValidIndexHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN ULONG Index,
    OUT PRTL_HANDLE_TABLE_ENTRY *Handle)
{
    PRTL_HANDLE_TABLE_ENTRY InternalHandle;

    DPRINT("RtlIsValidIndexHandle(HandleTable %p Index 0x%lx Handle %p)\n", HandleTable, Index, Handle);

    if (HandleTable == NULL)
        return FALSE;

    DPRINT("Handles %p HandleSize 0x%lx\n",
           HandleTable->CommittedHandles, HandleTable->SizeOfHandleTableEntry);

    InternalHandle = (PRTL_HANDLE_TABLE_ENTRY)((ULONG_PTR)HandleTable->CommittedHandles +
                     (HandleTable->SizeOfHandleTableEntry * Index));
    if (!RtlIsValidHandle(HandleTable, InternalHandle))
        return FALSE;

    DPRINT("InternalHandle %p\n", InternalHandle);

    if (Handle != NULL)
        *Handle = InternalHandle;

    return TRUE;
}

/* EOF */
