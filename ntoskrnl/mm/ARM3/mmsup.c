/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/mmsup.c
 * PURPOSE:         ARM Memory Manager Support Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::MMSUP"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmMapUserAddressesToPage(IN PVOID BaseAddress,
                         IN SIZE_T NumberOfBytes,
                         IN PVOID PageAddress)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmAdjustWorkingSetSize(IN SIZE_T WorkingSetMinimumInBytes,
                       IN SIZE_T WorkingSetMaximumInBytes,
                       IN ULONG SystemCache,
                       IN BOOLEAN IncreaseOkay)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
MmSetAddressRangeModified(IN PVOID Address,
                          IN SIZE_T Length)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
MmIsAddressValid(IN PVOID VirtualAddress)
{
    //
    // Just check the Valid bit in the Address' PDE and PTE
    //
    if ((MiAddressToPde(VirtualAddress)->u.Hard.Valid == 0) ||
        (MiAddressToPte(VirtualAddress)->u.Hard.Valid == 0))
    {
        //
        // Attempting to access this page is guranteed to result in a page fault
        //
        return FALSE;
    }

    //
    // This address is valid now, but it will only stay so if the caller holds
    // the PFN lock
    //
    return TRUE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
MmIsNonPagedSystemAddressValid(IN PVOID VirtualAddress)
{
    DPRINT1("WARNING: %s returns bogus result\n", __FUNCTION__);
    return MmIsAddressValid(VirtualAddress);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmSetBankedSection(IN HANDLE ProcessHandle,
                   IN PVOID VirtualAddress,
                   IN ULONG BankLength,
                   IN BOOLEAN ReadWriteBank,
                   IN PVOID BankRoutine,
                   IN PVOID Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
MmIsRecursiveIoFault(VOID)
{
    PETHREAD Thread = PsGetCurrentThread();
    
    //
    // If any of these is true, this is a recursive fault
    //
    return ((Thread->DisablePageFaultClustering) | (Thread->ForwardClusterOnly));
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
MmIsThisAnNtAsSystem(VOID)
{
    /* Return if this is a server system */
    return MmProductType;
}

/*
 * @implemented
 */
MM_SYSTEMSIZE
NTAPI
MmQuerySystemSize(VOID)
{
    /* Return the low, medium or high memory system type */
    return MmSystemSize;
}

/* EOF */
