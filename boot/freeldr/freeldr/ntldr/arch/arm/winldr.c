/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/freeldr/arch/arm/winldr.c
 * PURPOSE:         ARM Kernel Loader
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>
#include <debug.h>
#include <internal/arm/mm.h>
#include <internal/arm/intrin_i.h>
#include "../../winldr.h"


/* FUNCTIONS **************************************************************/

BOOLEAN
MempSetupPaging(IN PFN_NUMBER StartPage,
                IN PFN_NUMBER NumberOfPages,
                IN BOOLEAN KernelMapping)
{
    return TRUE;
}

VOID
MempUnmapPage(IN PFN_NUMBER Page)
{
    return;
}

VOID
MempDump(VOID)
{
    return;
}

VOID
WinLdrSetupForNt(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                 IN PVOID *GdtIdt,
                 IN PULONG PcrBasePage,
                 IN PULONG TssBasePage)
{

}

static
BOOLEAN
MempAllocatePageTables(VOID)
{


    /* Done */
    return TRUE;
}

VOID
WinLdrSetProcessorContext(
    _In_ USHORT OperatingSystemVersion)
{

}

VOID
WinLdrSetupMachineDependent(
    PLOADER_PARAMETER_BLOCK LoaderBlock)
{
}
