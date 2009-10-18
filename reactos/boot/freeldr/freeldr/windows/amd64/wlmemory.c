/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            freeldr/winldr/wlmemory.c
 * PURPOSE:         Memory related routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>

#include <ndk/asm.h>
#include <debug.h>

extern ULONG TotalNLSSize;
extern ULONG LoaderPagesSpanned;

// This is needed because headers define wrong one for ReactOS
#undef KIP0PCRADDRESS
#define KIP0PCRADDRESS                      0xffdff000

#define HYPER_SPACE_ENTRY       0x300

PCHAR  MemTypeDesc[]  = {
    "ExceptionBlock    ", // ?
    "SystemBlock       ", // ?
    "Free              ",
    "Bad               ", // used
    "LoadedProgram     ", // == Free
    "FirmwareTemporary ", // == Free
    "FirmwarePermanent ", // == Bad
    "OsloaderHeap      ", // used
    "OsloaderStack     ", // == Free
    "SystemCode        ",
    "HalCode           ",
    "BootDriver        ", // not used
    "ConsoleInDriver   ", // ?
    "ConsoleOutDriver  ", // ?
    "StartupDpcStack   ", // ?
    "StartupKernelStack", // ?
    "StartupPanicStack ", // ?
    "StartupPcrPage    ", // ?
    "StartupPdrPage    ", // ?
    "RegistryData      ", // used
    "MemoryData        ", // not used
    "NlsData           ", // used
    "SpecialMemory     ", // == Bad
    "BBTMemory         " // == Bad
    };

VOID
WinLdrpDumpMemoryDescriptors(PLOADER_PARAMETER_BLOCK LoaderBlock);


VOID
MempAddMemoryBlock(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                   ULONG BasePage,
                   ULONG PageCount,
                   ULONG Type);
VOID
WinLdrInsertDescriptor(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                       IN PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor);

VOID
WinLdrRemoveDescriptor(IN PMEMORY_ALLOCATION_DESCRIPTOR Descriptor);

VOID
WinLdrSetProcessorContext(PVOID GdtIdt, IN ULONG Pcr, IN ULONG Tss);

// This is needed only for SetProcessorContext routine
#pragma pack(2)
	typedef struct
	{
		USHORT Limit;
		ULONG Base;
	} GDTIDT;
#pragma pack(4)

/* GLOBALS ***************************************************************/

PHARDWARE_PTE PDE;
PHARDWARE_PTE HalPageTable;

PUCHAR PhysicalPageTablesBuffer;
PUCHAR KernelPageTablesBuffer;
ULONG PhysicalPageTables;
ULONG KernelPageTables;

MEMORY_ALLOCATION_DESCRIPTOR *Mad;
ULONG MadCount = 0;


/* FUNCTIONS **************************************************************/

BOOLEAN
MempAllocatePageTables()
{

	return TRUE;
}

VOID
MempAllocatePTE(ULONG Entry, PHARDWARE_PTE *PhysicalPT, PHARDWARE_PTE *KernelPT)
{

}

BOOLEAN
MempSetupPaging(IN ULONG StartPage,
				IN ULONG NumberOfPages)
{

	return TRUE;
}

VOID
MempDisablePages()
{

}

VOID
MempAddMemoryBlock(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                   ULONG BasePage,
                   ULONG PageCount,
                   ULONG Type)
{

}

#ifdef _M_IX86
VOID
WinLdrpMapApic()
{

}
#else
VOID
WinLdrpMapApic()
{
	/* Implement it for another arch */
}
#endif

BOOLEAN
WinLdrTurnOnPaging(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                   ULONG PcrBasePage,
                   ULONG TssBasePage,
                   PVOID GdtIdt)
{
return 1;
}

// Two special things this func does: it sorts descriptors,
// and it merges free ones
VOID
WinLdrInsertDescriptor(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                       IN PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor)
{

}

VOID
WinLdrSetProcessorContext(PVOID GdtIdt, IN ULONG Pcr, IN ULONG Tss)
{

}

