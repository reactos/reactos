/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/mm/verifier.c
* PURPOSE:         Mm Driver Verifier Routines
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

MM_DRIVER_VERIFIER_DATA MmVerifierData;
LIST_ENTRY MiVerifierDriverAddedThunkListHead;
KMUTANT MmSystemLoadLock;
ULONG MiActiveVerifierThunks;

extern LIST_ENTRY PsLoadedModuleList;

/* PRIVATE FUNCTIONS *********************************************************/

PLDR_DATA_TABLE_ENTRY
NTAPI
MiLookupDataTableEntry(IN PVOID Address)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry, FoundEntry = NULL;
    PLIST_ENTRY NextEntry;
    PAGED_CODE();

    /* Loop entries */
    NextEntry = PsLoadedModuleList.Flink;
    do
    {
        /* Get the loader entry */
        LdrEntry =  CONTAINING_RECORD(NextEntry,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

        /* Check if the address matches */
        if ((Address >= LdrEntry->DllBase) &&
            (Address < (PVOID)((ULONG_PTR)LdrEntry->DllBase + LdrEntry->SizeOfImage)))
        {
            /* Found a match */
            FoundEntry = LdrEntry;
            break;
        }

        /* Move on */
        NextEntry = NextEntry->Flink;
    } while(NextEntry != &PsLoadedModuleList);

    /* Return the entry */
    return FoundEntry;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
MmAddVerifierThunks(IN PVOID ThunkBuffer,
                    IN ULONG ThunkBufferSize)
{
    PDRIVER_VERIFIER_THUNK_PAIRS ThunkPairs, DriverThunkTable;
    ULONG ThunkCount;
    PDRIVER_SPECIFIED_VERIFIER_THUNKS ThunkTable;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PVOID ModuleBase, ModuleEnd;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Make sure the driver verifier is initialized */
    if (!MiVerifierDriverAddedThunkListHead.Flink) return STATUS_NOT_SUPPORTED;

    /* Get the thunk pairs and count them */
    ThunkPairs = (PDRIVER_VERIFIER_THUNK_PAIRS)ThunkBuffer;
    ThunkCount = ThunkBufferSize / sizeof(DRIVER_VERIFIER_THUNK_PAIRS);
    if (!ThunkCount) return STATUS_INVALID_PARAMETER_1;

    /* Now allocate our own thunk table */
    ThunkTable = ExAllocatePoolWithTag(PagedPool,
                                       sizeof(DRIVER_SPECIFIED_VERIFIER_THUNKS) +
                                       ThunkCount *
                                       sizeof(DRIVER_VERIFIER_THUNK_PAIRS),
                                       TAG('M', 'm', 'V', 't'));
    if (!ThunkTable) return STATUS_INSUFFICIENT_RESOURCES;

    /* Now copy the driver-fed part */
    DriverThunkTable = (PDRIVER_VERIFIER_THUNK_PAIRS)(ThunkTable + 1);
    RtlCopyMemory(DriverThunkTable,
                  ThunkPairs,
                  ThunkCount * sizeof(DRIVER_VERIFIER_THUNK_PAIRS));

    /* Acquire the system load lock */
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&MmSystemLoadLock,
                          WrVirtualMemory,
                          KernelMode,
                          FALSE,
                          NULL);

    /* Get the loader entry */
    LdrEntry = MiLookupDataTableEntry(DriverThunkTable->PristineRoutine);
    if (!LdrEntry)
    {
        /* Fail */
        Status = STATUS_INVALID_PARAMETER_2;
        goto Cleanup;
    }

    /* Get driver base and end */
    ModuleBase = LdrEntry->DllBase;
    ModuleEnd = (PVOID)((ULONG_PTR)LdrEntry->DllBase + LdrEntry->SizeOfImage);

    /* Loop all the thunks */
    for (i = 0; i < ThunkCount; i++)
    {
        /* Make sure it's in the driver */
        if (((ULONG_PTR)DriverThunkTable->PristineRoutine < (ULONG_PTR)ModuleBase) ||
            ((ULONG_PTR)DriverThunkTable->PristineRoutine >= (ULONG_PTR)ModuleEnd))
        {
            /* Nope, fail */
            Status = STATUS_INVALID_PARAMETER_2;
            goto Cleanup;
        }
    }

    /* Otherwise, add this entry */
    ThunkTable->DataTableEntry = LdrEntry;
    ThunkTable->NumberOfThunks = ThunkCount;
    MiActiveVerifierThunks++;
    InsertTailList(&MiVerifierDriverAddedThunkListHead,
                   &ThunkTable->ListEntry);
    ThunkTable = NULL;

Cleanup:
    /* Release the lock */
    KeReleaseMutant(&MmSystemLoadLock, 1, FALSE, FALSE);
    KeLeaveCriticalRegion();

    /* Free the table if we failed and return status */
    if (ThunkTable) ExFreePool(ThunkTable);
    return Status;
}

/*
 * @implemented
 */
LOGICAL
NTAPI
MmIsDriverVerifying(IN PDRIVER_OBJECT DriverObject)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    /* Get the loader entry */
    LdrEntry = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;
    if (!LdrEntry) return FALSE;

    /* Check if we're verifying or not */
    return (LdrEntry->Flags & LDRP_IMAGE_VERIFYING) ? TRUE: FALSE;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MmIsVerifierEnabled(OUT PULONG VerifierFlags)
{
    /* Check if we've actually added anything to the list */
    if (MiVerifierDriverAddedThunkListHead.Flink)
    {
        /* We have, read the verifier level */
        *VerifierFlags = MmVerifierData.Level;
        return STATUS_SUCCESS;
    }

    /* Otherwise, we're disabled */
    *VerifierFlags = 0;
    return STATUS_NOT_SUPPORTED;
}

/* EOF */
