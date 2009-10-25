/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/drvmgmt.c
 * PURPOSE:         ARM Memory Manager Driver Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::DRVMGMT"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* GLOBALS *******************************************************************/

MM_DRIVER_VERIFIER_DATA MmVerifierData;
LIST_ENTRY MiVerifierDriverAddedThunkListHead;
ULONG MiActiveVerifierThunks;
extern KMUTANT MmSystemLoadLock;
extern LIST_ENTRY PsLoadedModuleList;

/* PRIVATE FUNCTIONS *********************************************************/

PLDR_DATA_TABLE_ENTRY
NTAPI
MiLookupDataTableEntry(IN PVOID Address)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry, FoundEntry = NULL;
    PLIST_ENTRY NextEntry;
    PAGED_CODE();
    
    //
    // Loop entries
    //
    NextEntry = PsLoadedModuleList.Flink;
    do
    {
        //
        // Get the loader entry
        //
        LdrEntry =  CONTAINING_RECORD(NextEntry,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);
        
        //
        // Check if the address matches
        //
        if ((Address >= LdrEntry->DllBase) &&
            (Address < (PVOID)((ULONG_PTR)LdrEntry->DllBase +
                               LdrEntry->SizeOfImage)))
        {
            //
            // Found a match
            //
            FoundEntry = LdrEntry;
            break;
        }
        
        //
        // Move on
        //
        NextEntry = NextEntry->Flink;
    } while(NextEntry != &PsLoadedModuleList);
    
    //
    // Return the entry
    //
    return FoundEntry;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @unimplemented
 */
VOID
NTAPI
MmUnlockPageableImageSection(IN PVOID ImageSectionHandle)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmLockPageableSectionByHandle(IN PVOID ImageSectionHandle)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
MmLockPageableDataSection(IN PVOID AddressWithinSection)
{
    //
    // We should just find the section and call MmLockPageableSectionByHandle
    //
    UNIMPLEMENTED;
    return AddressWithinSection;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
MmPageEntireDriver(IN PVOID AddressWithinSection)
{
    //PMMPTE StartPte, EndPte;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PAGED_CODE();

    //
    // Get the loader entry
    //
    LdrEntry = MiLookupDataTableEntry(AddressWithinSection);
    if (!LdrEntry) return NULL;

    //
    // Check if paging of kernel mode is disabled or if the driver is mapped as
    // an image
    //
    if ((MmDisablePagingExecutive & 0x1) || (LdrEntry->SectionPointer))
    {
        //
        // Don't do anything, just return the base address
        //
        return LdrEntry->DllBase;
    }

    //
    // Wait for active DPCs to finish before we page out the driver
    //
    KeFlushQueuedDpcs();

    //
    // Get the PTE range for the whole driver image
    //
    //StartPte = MiGetPteAddress(LdrEntry->DllBase);
    //EndPte = MiGetPteAddress(LdrEntry->DllBase +
    //                         LdrEntry->SizeOfImage);

    //
    // Enable paging for the PTE range
    //
    //MiSetPagingOfDriver(StartPte, EndPte);

    //
    // Return the base address
    //
    return LdrEntry->DllBase;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmResetDriverPaging(IN PVOID AddressWithinSection)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
ULONG
NTAPI
MmTrimAllSystemPageableMemory(IN ULONG PurgeTransitionList)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MmAddVerifierThunks(IN PVOID ThunkBuffer,
                    IN ULONG ThunkBufferSize)
{
    PDRIVER_VERIFIER_THUNK_PAIRS ThunkTable;
    ULONG ThunkCount;
    PDRIVER_SPECIFIED_VERIFIER_THUNKS DriverThunks;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PVOID ModuleBase, ModuleEnd;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();
    
    //
    // Make sure the driver verifier is initialized
    //
    if (!MiVerifierDriverAddedThunkListHead.Flink) return STATUS_NOT_SUPPORTED;
    
    //
    // Get the thunk pairs and count them
    //
    ThunkCount = ThunkBufferSize / sizeof(DRIVER_VERIFIER_THUNK_PAIRS);
    if (!ThunkCount) return STATUS_INVALID_PARAMETER_1;
    
    //
    // Now allocate our own thunk table
    //
    DriverThunks = ExAllocatePoolWithTag(PagedPool,
                                         sizeof(*DriverThunks) +
                                         ThunkCount *
                                         sizeof(DRIVER_VERIFIER_THUNK_PAIRS),
                                         'tVmM');
    if (!DriverThunks) return STATUS_INSUFFICIENT_RESOURCES;
    
    //
    // Now copy the driver-fed part
    //
    ThunkTable = (PDRIVER_VERIFIER_THUNK_PAIRS)(DriverThunks + 1);
    RtlCopyMemory(ThunkTable,
                  ThunkBuffer,
                  ThunkCount * sizeof(DRIVER_VERIFIER_THUNK_PAIRS));
    
    //
    // Acquire the system load lock
    //
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&MmSystemLoadLock,
                          WrVirtualMemory,
                          KernelMode,
                          FALSE,
                          NULL);
    
    //
    // Get the loader entry
    //
    LdrEntry = MiLookupDataTableEntry(ThunkTable->PristineRoutine);
    if (!LdrEntry)
    {
        //
        // Fail
        //
        Status = STATUS_INVALID_PARAMETER_2;
        goto Cleanup;
    }
    
    //
    // Get driver base and end
    //
    ModuleBase = LdrEntry->DllBase;
    ModuleEnd = (PVOID)((ULONG_PTR)LdrEntry->DllBase + LdrEntry->SizeOfImage);
    
    //
    // Don't allow hooking the kernel or HAL
    //
    if (ModuleBase < (PVOID)(KSEG0_BASE + MmBootImageSize))
    {
        //
        // Fail
        //
        Status = STATUS_INVALID_PARAMETER_2;
        goto Cleanup;
    }
    
    //
    // Loop all the thunks
    //
    for (i = 0; i < ThunkCount; i++)
    {
        //
        // Make sure it's in the driver
        //
        if (((ULONG_PTR)ThunkTable->PristineRoutine < (ULONG_PTR)ModuleBase) ||
            ((ULONG_PTR)ThunkTable->PristineRoutine >= (ULONG_PTR)ModuleEnd))
        {
            //
            // Nope, fail
            //
            Status = STATUS_INVALID_PARAMETER_2;
            goto Cleanup;
        }
    }
    
    //
    // Otherwise, add this entry
    //
    DriverThunks->DataTableEntry = LdrEntry;
    DriverThunks->NumberOfThunks = ThunkCount;
    MiActiveVerifierThunks++;
    InsertTailList(&MiVerifierDriverAddedThunkListHead,
                   &DriverThunks->ListEntry);
    DriverThunks = NULL;
    
Cleanup:
    //
    // Release the lock
    //
    KeReleaseMutant(&MmSystemLoadLock, 1, FALSE, FALSE);
    KeLeaveCriticalRegion();
    
    //
    // Free the table if we failed and return status
    //
    if (DriverThunks) ExFreePool(DriverThunks);
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
    
    //
    // Get the loader entry
    //
    LdrEntry = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;
    if (!LdrEntry) return FALSE;
    
    //
    // Check if we're verifying or not
    //
    return (LdrEntry->Flags & LDRP_IMAGE_VERIFYING) ? TRUE: FALSE;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MmIsVerifierEnabled(OUT PULONG VerifierFlags)
{
    //
    // Check if we've actually added anything to the list
    //
    if (MiVerifierDriverAddedThunkListHead.Flink)
    {
        //
        // We have, read the verifier level
        //
        *VerifierFlags = MmVerifierData.Level;
        return STATUS_SUCCESS;
    }
    
    //
    // Otherwise, we're disabled
    //
    *VerifierFlags = 0;
    return STATUS_NOT_SUPPORTED;
}

/* EOF */
