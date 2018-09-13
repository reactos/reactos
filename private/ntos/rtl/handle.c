/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    handle.c

Abstract:

    This module contains a simple handle allocator for use by the Local and
    Global memory allocators.

Author:

    Steve Wood (stevewo) 25-Jul-1991

Revision History:

--*/

#include "ntrtlp.h"

void
RtlInitializeHandleTable(
    IN ULONG MaximumNumberOfHandles,
    IN ULONG SizeOfHandleTableEntry,
    OUT PRTL_HANDLE_TABLE HandleTable
    )
{
    RtlZeroMemory( HandleTable, sizeof( *HandleTable ) );
    HandleTable->MaximumNumberOfHandles = MaximumNumberOfHandles;
    HandleTable->SizeOfHandleTableEntry = SizeOfHandleTableEntry;

    return;
}

NTSTATUS
RtlDestroyHandleTable(
    IN OUT PRTL_HANDLE_TABLE HandleTable
    )
{
    NTSTATUS Status;
    PVOID BaseAddress;
    SIZE_T ReserveSize;

    BaseAddress = HandleTable->CommittedHandles;
    ReserveSize = (PUCHAR)(HandleTable->MaxReservedHandles) -
                  (PUCHAR)(HandleTable->CommittedHandles);

    Status = NtFreeVirtualMemory( NtCurrentProcess(),
                                  &BaseAddress,
                                  &ReserveSize,
                                  MEM_RELEASE
                                );
    return Status;
}

PRTL_HANDLE_TABLE_ENTRY
RtlAllocateHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    OUT PULONG HandleIndex OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID BaseAddress;
    ULONG n;
    SIZE_T ReserveSize;
    SIZE_T CommitSize;
    PRTL_HANDLE_TABLE_ENTRY p, *pp;

    if (HandleTable->FreeHandles == NULL) {
        try {
            if (HandleTable->UnCommittedHandles == NULL) {
                ReserveSize = HandleTable->MaximumNumberOfHandles *
                              HandleTable->SizeOfHandleTableEntry;
                BaseAddress = NULL;
                Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                                  &BaseAddress,
                                                  0,
                                                  &ReserveSize,
                                                  MEM_RESERVE,
                                                  PAGE_READWRITE
                                                );

                if (NT_SUCCESS( Status )) {
                    HandleTable->CommittedHandles = (PRTL_HANDLE_TABLE_ENTRY)BaseAddress;
                    HandleTable->UnCommittedHandles = (PRTL_HANDLE_TABLE_ENTRY)BaseAddress;
                    HandleTable->MaxReservedHandles = (PRTL_HANDLE_TABLE_ENTRY)
                        ((PCHAR)BaseAddress + ReserveSize);
                    }
                }
            else {
                Status = STATUS_SUCCESS;
                }


            if (NT_SUCCESS( Status )) {
                p = HandleTable->UnCommittedHandles;
                if (p >= HandleTable->MaxReservedHandles) {
                    Status = STATUS_NO_MEMORY;
                    }
                else {
                    CommitSize = PAGE_SIZE;
                    Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                                      (PVOID *)&p,
                                                      0,
                                                      &CommitSize,
                                                      MEM_COMMIT,
                                                      PAGE_READWRITE
                                                    );
                    if (NT_SUCCESS( Status )) {
                        HandleTable->UnCommittedHandles = (PRTL_HANDLE_TABLE_ENTRY)
                                ((PCH)p + CommitSize);
                        }
                    }
                }

            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            Status = GetExceptionCode();
            }

        if (!NT_SUCCESS( Status )) {
            return NULL;
            }

        pp = &HandleTable->FreeHandles;
        while (p < HandleTable->UnCommittedHandles) {
            *pp = p;
            pp = &p->NextFree;
            p = (PRTL_HANDLE_TABLE_ENTRY)((PUCHAR)p + HandleTable->SizeOfHandleTableEntry);
            }
        }

    //
    // Remove handle table entry from head of free list.
    //

    p = HandleTable->FreeHandles;
    HandleTable->FreeHandles = p->NextFree;

    //
    // Clear free list link field, which also leaves the handle allocated bit
    // clear.  This allows the caller to mark it is allocated after they are
    // done filling in their portion.
    //

    p->NextFree = NULL;


    //
    // If requested, return the index of this handle table entry
    //
    if (ARGUMENT_PRESENT( HandleIndex )) {
        *HandleIndex = (ULONG) (((PCHAR)p - (PCHAR)HandleTable->CommittedHandles) / 
                                HandleTable->SizeOfHandleTableEntry);
        }

    //
    // Return a pointer to the handle table entry.
    //

    return p;
}


BOOLEAN
RtlFreeHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN PRTL_HANDLE_TABLE_ENTRY Handle
    )
{
#if DBG
    if (!RtlIsValidHandle( HandleTable, Handle )) {
        DbgPrint( "RTL: RtlFreeHandle( %lx ) - invalid handle\n", Handle );
        if (NtCurrentPeb()->BeingDebugged) {
            DbgBreakPoint();
            }
        return FALSE;
        }
#endif

    RtlZeroMemory( Handle, HandleTable->SizeOfHandleTableEntry );
    Handle->NextFree = HandleTable->FreeHandles;
    HandleTable->FreeHandles = Handle;
    return TRUE;
}



BOOLEAN
RtlIsValidHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN PRTL_HANDLE_TABLE_ENTRY Handle
    )
{
    if (Handle == NULL ||
        Handle < HandleTable->CommittedHandles ||
        Handle >= HandleTable->UnCommittedHandles ||
        (ULONG_PTR)Handle & (HandleTable->SizeOfHandleTableEntry - 1) ||
        !(Handle->Flags & RTL_HANDLE_ALLOCATED)
       ) {
        return FALSE;
        }
    else {
        return TRUE;
        }
}


BOOLEAN
RtlIsValidIndexHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN ULONG HandleIndex,
    OUT PRTL_HANDLE_TABLE_ENTRY *Handle
    )
{
    PRTL_HANDLE_TABLE_ENTRY p;

    p = (PRTL_HANDLE_TABLE_ENTRY)
        ((PCHAR)HandleTable->CommittedHandles + (HandleIndex * HandleTable->SizeOfHandleTableEntry));

    if (RtlIsValidHandle( HandleTable, p )) {
        *Handle = p;
        return TRUE;
        }
    else {
        return FALSE;
        }
}
