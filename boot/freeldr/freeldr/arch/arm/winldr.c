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

#define PFN_SHIFT                   12
#define LARGE_PFN_SHIFT             20

#define PTE_BASE                    0xC0000000
#define PDE_BASE                    0xC0400000
#define PDR_BASE                    0xFFD00000
#define VECTOR_BASE                 0xFFFF0000

#ifdef _ZOOM2_
#define IDMAP_BASE                  0x81000000
#define MMIO_BASE                   0x10000000
#else
#define IDMAP_BASE                  0x00000000
#define MMIO_BASE                   0x10000000
#endif

#define LowMemPageTableIndex        (IDMAP_BASE >> PDE_SHIFT)
#define MmioPageTableIndex          (MMIO_BASE >> PDE_SHIFT)
#define KernelPageTableIndex        (KSEG0_BASE >> PDE_SHIFT)
#define StartupPtePageTableIndex    (PTE_BASE >> PDE_SHIFT)
#define StartupPdePageTableIndex    (PDE_BASE >> PDE_SHIFT)
#define PdrPageTableIndex           (PDR_BASE >> PDE_SHIFT)
#define VectorPageTableIndex        (VECTOR_BASE >> PDE_SHIFT)

#ifndef _ZOOM2_
PVOID MempPdrBaseAddress = (PVOID)0x70000;
PVOID MempKernelBaseAddress = (PVOID)0;
#else
PVOID MempPdrBaseAddress = (PVOID)0x81100000;
PVOID MempKernelBaseAddress = (PVOID)0x80000000;
#endif

/* Converts a Physical Address into a Page Frame Number */
#define PaToPfn(p)                  ((p) >> PFN_SHIFT)
#define PaToLargePfn(p)             ((p) >> LARGE_PFN_SHIFT)
#define PaPtrToPfn(p)               (((ULONG_PTR)(p)) >> PFN_SHIFT)

/* Converts a Physical Address into a Coarse Page Table PFN */
#define PaPtrToPdePfn(p)            (((ULONG_PTR)(p)) >> CPT_SHIFT)

typedef struct _KPDR_PAGE
{
    PAGE_DIRECTORY_ARM PageDir;             // 0xC0400000 [0xFFD00000]
    CHAR HyperSpace[233 * PAGE_SIZE];       // 0xC0404000 [0xFFD04000]
    PAGE_TABLE_ARM KernelPageTable[3];      // 0xC04ED000 [0xFFDED000]
    CHAR SharedData[PAGE_SIZE];             // 0xC04F0000 [0xFFDF0000]
    CHAR KernelStack[KERNEL_STACK_SIZE];    // 0xC04F1000 [0xFFDF1000]
    CHAR PanicStack[KERNEL_STACK_SIZE];     // 0xC04F4000 [0xFFDF4000]
    CHAR InterruptStack[KERNEL_STACK_SIZE]; // 0xC04F7000 [0xFFDF7000]
    CHAR InitialProcess[PAGE_SIZE];         // 0xC04FA000 [0xFFDFA000]
    CHAR InitialThread[PAGE_SIZE];          // 0xC04FB000 [0xFFDFB000]
    CHAR Prcb[PAGE_SIZE];                   // 0xC04FC000 [0xFFDFC000]
    PAGE_TABLE_ARM PageDirPageTable;        // 0xC04FD000 [0xFFDFD000]
    PAGE_TABLE_ARM VectorPageTable;         // 0xC04FE000 [0xFFDFE000]
    CHAR Pcr[PAGE_SIZE];                    // 0xC04FF000 [0xFFDFF000]
} KPDR_PAGE, *PKPDR_PAGE;

C_ASSERT(sizeof(KPDR_PAGE) == (1 * 1024 * 1024));

HARDWARE_PTE_ARMV6 TempPte;
HARDWARE_LARGE_PTE_ARMV6 TempLargePte;
HARDWARE_PDE_ARMV6 TempPde;
PKPDR_PAGE PdrPage;

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

BOOLEAN
WinLdrMapSpecialPages(ULONG PcrBasePage)
{
    ULONG i;
    PHARDWARE_PTE_ARMV6 PointerPte;
    PHARDWARE_PDE_ARMV6 PointerPde;
    PHARDWARE_LARGE_PTE_ARMV6 LargePte;
    PFN_NUMBER Pfn;

    /* Setup the Startup PDE */
    LargePte = &PdrPage->PageDir.Pte[StartupPdePageTableIndex];
    TempLargePte.PageFrameNumber = PaToLargePfn((ULONG_PTR)&PdrPage->PageDir);
    *LargePte = TempLargePte;

    /* Map-in the PDR */
    LargePte = &PdrPage->PageDir.Pte[PdrPageTableIndex];
    *LargePte = TempLargePte;

    /* After this point, any MiAddressToPde is guaranteed not to fault */

    /*
     * Link them in the Startup PDE.
     * Note these are the entries in the PD at (MiAddressToPde(PTE_BASE)).
     */
    PointerPde = &PdrPage->PageDir.Pde[StartupPtePageTableIndex];
    Pfn = PaPtrToPdePfn(&PdrPage->PageDirPageTable);
    for (i = 0; i < 4; i++)
    {
        TempPde.PageFrameNumber = Pfn++;
        *PointerPde++ = TempPde;
    }

    /*
     * Now map these page tables in PTE space (MiAddressToPte(PTE_BASE)).
     * Note that they all live on a single page, since each is 1KB.
     */
    PointerPte = &PdrPage->PageDirPageTable.Pte[0x300];
    TempPte.PageFrameNumber = PaPtrToPfn(&PdrPage->PageDirPageTable);
    *PointerPte = TempPte;

    /*
     * After this point, MiAddressToPte((PDE_BASE) to MiAddressToPte(PDE_TOP))
     * is guaranteed not to fault.
     * Any subsequent page allocation will first need its page table created
     * and mapped in the PTE_BASE first, then the page table itself will be
     * editable through its flat PTE address.
     */

    /* Setup the Vector PDE */
    PointerPde = &PdrPage->PageDir.Pde[VectorPageTableIndex];
    TempPde.PageFrameNumber = PaPtrToPdePfn(&PdrPage->VectorPageTable);
    *PointerPde = TempPde;

    /* Setup the Vector PTEs */
    PointerPte = &PdrPage->VectorPageTable.Pte[0xF0];
    TempPte.PageFrameNumber = 0;
    *PointerPte = TempPte;

    /* TODO: Map in the kernel CPTs */
    return TRUE;
}

VOID
WinLdrSetupForNt(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                 IN PVOID *GdtIdt,
                 IN ULONG *PcrBasePage,
                 IN ULONG *TssBasePage)
{
    PKPDR_PAGE PdrPage = (PVOID)0xFFD00000;

    /* Load cache information */
    LoaderBlock->u.Arm.FirstLevelDcacheSize = FirstLevelDcacheSize;
    LoaderBlock->u.Arm.FirstLevelDcacheFillSize = FirstLevelDcacheFillSize;
    LoaderBlock->u.Arm.FirstLevelIcacheSize = FirstLevelIcacheSize;
    LoaderBlock->u.Arm.FirstLevelIcacheFillSize = FirstLevelIcacheFillSize;
    LoaderBlock->u.Arm.SecondLevelDcacheSize = SecondLevelDcacheSize;
    LoaderBlock->u.Arm.SecondLevelDcacheFillSize = SecondLevelDcacheFillSize;
    LoaderBlock->u.Arm.SecondLevelIcacheSize = SecondLevelIcacheSize;
    LoaderBlock->u.Arm.SecondLevelIcacheFillSize = SecondLevelIcacheSize;

    /* Write initial context information */
    LoaderBlock->KernelStack = (ULONG_PTR)PdrPage->KernelStack;
    LoaderBlock->KernelStack += KERNEL_STACK_SIZE;
    LoaderBlock->u.Arm.PanicStack = (ULONG_PTR)PdrPage->PanicStack;
    LoaderBlock->u.Arm.PanicStack += KERNEL_STACK_SIZE;
    LoaderBlock->u.Arm.InterruptStack = (ULONG_PTR)PdrPage->InterruptStack;
    LoaderBlock->u.Arm.InterruptStack += KERNEL_STACK_SIZE;
    LoaderBlock->Prcb = (ULONG_PTR)PdrPage->Prcb;
    LoaderBlock->Process = (ULONG_PTR)PdrPage->InitialProcess;
    LoaderBlock->Thread = (ULONG_PTR)PdrPage->InitialThread;
}

BOOLEAN
MempAllocatePageTables(VOID)
{
    ULONG i;
    PHARDWARE_PTE_ARMV6 PointerPte;
    PHARDWARE_PDE_ARMV6 PointerPde;
    PHARDWARE_LARGE_PTE_ARMV6 LargePte;
    PFN_NUMBER Pfn;

    /* Setup templates */
    TempPte.Sbo = TempPte.Valid = TempLargePte.LargePage = TempLargePte.Sbo = TempPde.Valid = 1;

    /* Allocate the 1MB "PDR" (Processor Data Region). Must be 1MB aligned */
    PdrPage = MmAllocateMemoryAtAddress(sizeof(KPDR_PAGE),
                                        MempPdrBaseAddress,
                                        LoaderMemoryData);

    /* Setup the Low Memory PDE as an identity-mapped Large Page (1MB) */
    LargePte = &PdrPage->PageDir.Pte[LowMemPageTableIndex];
    TempLargePte.PageFrameNumber = PaToLargePfn(IDMAP_BASE);
    *LargePte = TempLargePte;

    /* Setup the MMIO PDE as two identity mapped large pages -- the kernel will blow these away later */
    LargePte = &PdrPage->PageDir.Pte[MmioPageTableIndex];
    Pfn = PaToLargePfn(MMIO_BASE);
    for (i = 0; i < 2; i++)
    {
        TempLargePte.PageFrameNumber = Pfn++;
        *LargePte++ = TempLargePte;
    }

    /* Setup the Kernel PDEs */
    PointerPde = &PdrPage->PageDir.Pde[KernelPageTableIndex];
    Pfn = PaPtrToPdePfn(PdrPage->KernelPageTable);
    for (i = 0; i < 12; i++)
    {
        TempPde.PageFrameNumber = Pfn;
        *PointerPde++ = TempPde;
        Pfn++;
    }

    /* Setup the Kernel PTEs */
    PointerPte = PdrPage->KernelPageTable[0].Pte;
    Pfn = PaPtrToPfn(MempKernelBaseAddress);
    for (i = 0; i < 3072; i++)
    {
        TempPte.PageFrameNumber = Pfn++;
        *PointerPte++ = TempPte;
    }

    /* Done */
    return TRUE;
}

VOID
WinLdrSetProcessorContext(VOID)
{
    ARM_CONTROL_REGISTER ControlRegister;
    ARM_TTB_REGISTER TtbRegister;
    ARM_DOMAIN_REGISTER DomainRegister;

    /* Set the TTBR */
    TtbRegister.AsUlong = (ULONG_PTR)&PdrPage->PageDir;
    ASSERT(TtbRegister.Reserved == 0);
    KeArmTranslationTableRegisterSet(TtbRegister);

    /* Disable domains and simply use access bits on PTEs */
    DomainRegister.AsUlong = 0;
    DomainRegister.Domain0 = ClientDomain;
    KeArmDomainRegisterSet(DomainRegister);

    /* Enable ARMv6+ paging (MMU), caches and the access bit */
    ControlRegister = KeArmControlRegisterGet();
    ControlRegister.MmuEnabled = TRUE;
    ControlRegister.ICacheEnabled = TRUE;
    ControlRegister.DCacheEnabled = TRUE;
    ControlRegister.ForceAp = TRUE;
    ControlRegister.ExtendedPageTables = TRUE;
    KeArmControlRegisterSet(ControlRegister);
}

VOID
WinLdrSetupMachineDependent(
    PLOADER_PARAMETER_BLOCK LoaderBlock)
{
}

VOID DiskStopFloppyMotor(VOID)
{
}

VOID
RealEntryPoint(VOID)
{
    BootMain("");
}

VOID
FrLdrBugCheckWithMessage(
    ULONG BugCode,
    PCHAR File,
    ULONG Line,
    PSTR Format,
    ...)
{

}
