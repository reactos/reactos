/*
 * COPYRIGHT:       See COPYING in the top directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/mmsup.c
 * PURPOSE:         Kernel memory managment functions
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

BOOLEAN IsThisAnNtAsSystem = FALSE;
MM_SYSTEMSIZE MmSystemSize = MmSmallSystem;

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
MiZeroPage(PFN_TYPE Page)
{
    KIRQL Irql;
    PVOID TempAddress;
    
    Irql = KeRaiseIrqlToDpcLevel();
    TempAddress = MiMapPageToZeroInHyperSpace(Page);
    if (TempAddress == NULL)
    {
        return(STATUS_NO_MEMORY);
    }
    memset(TempAddress, 0, PAGE_SIZE);
    MiUnmapPagesInZeroSpace(TempAddress, 1);
    KeLowerIrql(Irql);
    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MiCopyFromUserPage(PFN_TYPE DestPage, PVOID SourceAddress)
{
    PEPROCESS Process;
    KIRQL Irql;
    PVOID TempAddress;
    
    Process = PsGetCurrentProcess();
    TempAddress = MiMapPageInHyperSpace(Process, DestPage, &Irql);
    if (TempAddress == NULL)
    {
        return(STATUS_NO_MEMORY);
    }
    memcpy(TempAddress, SourceAddress, PAGE_SIZE);
    MiUnmapPageInHyperSpace(Process, TempAddress, Irql);
    return(STATUS_SUCCESS);
}

/* PRIVATE FUNCTIONS **********************************************************/

/* Miscellanea functions: they may fit somewhere else */

/*
 * @implemented
 */
BOOLEAN
NTAPI
MmIsRecursiveIoFault (VOID)
{
    PETHREAD Thread = PsGetCurrentThread();

    return (Thread->DisablePageFaultClustering | Thread->ForwardClusterOnly);
}

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
ULONG NTAPI
MmAdjustWorkingSetSize (ULONG Unknown0,
                        ULONG Unknown1,
                        ULONG Unknown2,
                        ULONG Unknown3)
{
   UNIMPLEMENTED;
   return (0);
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
MmSetAddressRangeModified (
    IN PVOID    Address,
    IN ULONG    Length
)
{
   UNIMPLEMENTED;
   return (FALSE);
}

/*
 * @implemented
 */
BOOLEAN NTAPI MmIsNonPagedSystemAddressValid(PVOID VirtualAddress)
{
    DPRINT1("WARNING: %s returns bogus result\n", __FUNCTION__);
    return MmIsAddressValid(VirtualAddress);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
MmIsThisAnNtAsSystem(VOID)
{
    return IsThisAnNtAsSystem;
}

/*
 * @implemented
 */
MM_SYSTEMSIZE
NTAPI
MmQuerySystemSize(VOID)
{
    return MmSystemSize;
}

/* EOF */
