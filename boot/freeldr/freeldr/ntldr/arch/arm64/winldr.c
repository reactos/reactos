
#include <freeldr.h>
#include <debug.h>
DBG_DEFAULT_CHANNEL(WINDOWS);
#include <internal/arm/mm.h>
//#include <internal/arm/intrin_i.h>
#include "../../winldr.h"

PLOADER_PARAMETER_BLOCK PubLoaderBlockVA;
KERNEL_ENTRY_POINT PubKiSystemStartup;

extern PVOID OsLoaderBase;
extern SIZE_T OsLoaderSize;

/* FUNCTIONS **************************************************************/


static
BOOLEAN
WinLdrMapSpecialPages(ULONG PcrBasePage)
{
     TRACE("WinLdrMapSpecialPages: Entry\n");
    /* TODO: Map in the kernel CPTs */
    return TRUE;
}


BOOLEAN
MempIsPageMapped(PVOID VirtualAddress)
{
    return TRUE;
}

BOOLEAN
MempSetupPaging(IN PFN_NUMBER StartPage,
                IN PFN_NUMBER NumberOfPages,
                IN BOOLEAN KernelMapping)
{
    return TRUE;
}

/* ************************************************************************/

VOID
WinLdrSetProcessorContext(VOID)
{

    TRACE("WinLdrSetProcessorContext: Entry\n");

}

VOID
WinLdrSetupMachineDependent(
    PLOADER_PARAMETER_BLOCK LoaderBlock)
{

}

VOID
MempUnmapPage(IN PFN_NUMBER Page)
{
    TRACE("Unmapping %X\n", Page);
    return;
}

VOID
MempDump(VOID)
{
    return;
}

/* THIS IS FINISHED SHOULDN'T BE CHANGED -----------------------------------*/
void _ChangeStack();
VOID
WinldrFinalizeBoot(PLOADER_PARAMETER_BLOCK LoaderBlockVA,
                   KERNEL_ENTRY_POINT KiSystemStartup)
{
    PubLoaderBlockVA = LoaderBlockVA;
    PubKiSystemStartup = KiSystemStartup;
}
