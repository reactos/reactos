/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 page table setup and kernel hand off
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>
#include <cportlib/uartinfo.h>
#include "../../winldr.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(WINDOWS);

/*
 * One root table is loaded into both TTBR0_EL1 and TTBR1_EL1. With 48 bit
 * halves the low 256 entries hold the user range and the high 256 entries
 * the kernel range, the same split as on x64.
 */

/* Memory attribute indices, the CacheType field of the PTE */
#define ATTR_NORMAL             0
#define ATTR_NORMAL_NONCACHED   1
#define ATTR_DEVICE_NGNRNE      2
#define ATTR_DEVICE_NGNRE       3

/* Normal write back, normal non cached, device nGnRnE and device nGnRE */
#define MAIR_VALUE              0x00000000040044FFULL

#define SHAREABILITY_INNER      3

/* TCR_EL1 for two 48 bit halves with 4 KB granules and write back table walks */
#define TCR_T0SZ(x)             ((ULONG64)(x) << 0)
#define TCR_IRGN0_WBWA          (1ULL << 8)
#define TCR_ORGN0_WBWA          (1ULL << 10)
#define TCR_SH0_INNER           (3ULL << 12)
#define TCR_TG0_4K              (0ULL << 14)
#define TCR_T1SZ(x)             ((ULONG64)(x) << 16)
#define TCR_IRGN1_WBWA          (1ULL << 24)
#define TCR_ORGN1_WBWA          (1ULL << 26)
#define TCR_SH1_INNER           (3ULL << 28)
#define TCR_TG1_4K              (2ULL << 30)
#define TCR_IPS(x)              ((ULONG64)(x) << 32)

/* PARange 5 is 48 bits, more is not reachable with 4 levels of 4 KB tables */
#define MAX_PA_RANGE            5

/* Host mode of the virtualization host extensions */
#define HCR_EL2_E2H             (1ULL << 34)

ULONG64 Arm64GetPhysicalAddressRange(VOID);
ULONG Arm64GetCurrentEl(VOID);
ULONG64 Arm64GetHypervisorConfig(VOID);
VOID Arm64SwitchTranslation(_In_ ULONG64 Mair, _In_ ULONG64 Tcr, _In_ ULONG64 Ttbr);

extern PUCHAR ComPortAddress;

/* GLOBALS ***************************************************************/

PHARDWARE_PTE PxeBase;
PFN_NUMBER SharedUserDataPfn;

/* FUNCTIONS **************************************************************/

static
VOID
MempMakeValidPte(
    _Out_ PHARDWARE_PTE Pte,
    _In_ PFN_NUMBER PageFrameNumber,
    _In_ ULONG CacheType)
{
    RtlZeroMemory(Pte, sizeof(*Pte));

    /* Bits 1:0 = 11 is a table at levels 0 to 2 and a page at level 3 */
    Pte->Valid = 1;
    Pte->NotLargePage = 1;
    Pte->CacheType = CacheType;
    Pte->Shareability = SHAREABILITY_INNER;
    Pte->Accessed = 1;
    Pte->PageFrameNumber = PageFrameNumber;

    if (CacheType != ATTR_NORMAL)
    {
        Pte->PrivilegedNoExecute = 1;
        Pte->UserNoExecute = 1;
    }
}

static
BOOLEAN
MempAllocatePageTables(VOID)
{
    TRACE(">>> MempAllocatePageTables\n");

    PxeBase = MmAllocateMemoryWithType(PAGE_SIZE, LoaderMemoryData);
    if (!PxeBase)
    {
        ERR("failed to allocate the root page table\n");
        return FALSE;
    }

    RtlZeroMemory(PxeBase, PAGE_SIZE);

    /* The root maps itself, which puts the page tables at PTE_BASE */
    MempMakeValidPte(&PxeBase[VAtoPXI(PXE_BASE)], PtrToPfn(PxeBase), ATTR_NORMAL);
    PxeBase[VAtoPXI(PXE_BASE)].PrivilegedNoExecute = 1;
    PxeBase[VAtoPXI(PXE_BASE)].UserNoExecute = 1;

    TRACE(">>> leave MempAllocatePageTables\n");
    return TRUE;
}

static
PHARDWARE_PTE
MempGetOrCreatePageDir(
    _In_opt_ PHARDWARE_PTE PdeBase,
    _In_ ULONG Index)
{
    PHARDWARE_PTE SubDir;

    if (!PdeBase)
        return NULL;

    if (PdeBase[Index].Valid)
        return (PVOID)((ULONG64)PdeBase[Index].PageFrameNumber * PAGE_SIZE);

    SubDir = MmAllocateMemoryWithType(PAGE_SIZE, LoaderMemoryData);
    if (!SubDir)
        return NULL;

    RtlZeroMemory(SubDir, PAGE_SIZE);
    MempMakeValidPte(&PdeBase[Index], PtrToPfn(SubDir), ATTR_NORMAL);
    return SubDir;
}

static
BOOLEAN
MempMapSinglePage(
    _In_ ULONG64 VirtualAddress,
    _In_ ULONG64 PhysicalAddress,
    _In_ ULONG CacheType)
{
    PHARDWARE_PTE PpeBase, PdeBase, PteBase;
    ULONG Index;

    PpeBase = MempGetOrCreatePageDir(PxeBase, VAtoPXI(VirtualAddress));
    PdeBase = MempGetOrCreatePageDir(PpeBase, VAtoPPI(VirtualAddress));
    PteBase = MempGetOrCreatePageDir(PdeBase, VAtoPDI(VirtualAddress));

    if (!PteBase)
    {
        ERR("!!!No Dir %p, %p, %p, %p\n", PxeBase, PpeBase, PdeBase, PteBase);
        return FALSE;
    }

    Index = VAtoPTI(VirtualAddress);
    if (PteBase[Index].Valid)
    {
        ERR("!!!Already mapped %ld\n", Index);
        return FALSE;
    }

    MempMakeValidPte(&PteBase[Index], PhysicalAddress / PAGE_SIZE, CacheType);
    return TRUE;
}

BOOLEAN
MempIsPageMapped(
    _In_ PVOID VirtualAddress)
{
    PHARDWARE_PTE Table = PxeBase;
    ULONG Index;

    Index = VAtoPXI(VirtualAddress);
    if (!Table[Index].Valid)
        return FALSE;

    Table = (PVOID)((ULONG64)Table[Index].PageFrameNumber * PAGE_SIZE);
    Index = VAtoPPI(VirtualAddress);
    if (!Table[Index].Valid)
        return FALSE;

    Table = (PVOID)((ULONG64)Table[Index].PageFrameNumber * PAGE_SIZE);
    Index = VAtoPDI(VirtualAddress);
    if (!Table[Index].Valid)
        return FALSE;

    Table = (PVOID)((ULONG64)Table[Index].PageFrameNumber * PAGE_SIZE);
    Index = VAtoPTI(VirtualAddress);
    return Table[Index].Valid;
}

static
PFN_NUMBER
MempMapRangeOfPages(
    _In_ ULONG64 VirtualAddress,
    _In_ ULONG64 PhysicalAddress,
    _In_ PFN_NUMBER cPages)
{
    PFN_NUMBER i;

    for (i = 0; i < cPages; i++)
    {
        if (!MempMapSinglePage(VirtualAddress, PhysicalAddress, ATTR_NORMAL))
        {
            ERR("Failed to map page %ld from %p to %p\n",
                i, (PVOID)VirtualAddress, (PVOID)PhysicalAddress);
            return i;
        }
        VirtualAddress += PAGE_SIZE;
        PhysicalAddress += PAGE_SIZE;
    }

    return i;
}

BOOLEAN
MempSetupPaging(
    _In_ PFN_NUMBER StartPage,
    _In_ PFN_NUMBER NumberOfPages,
    _In_ BOOLEAN KernelMapping)
{
    TRACE(">>> MempSetupPaging(0x%lx, %ld, %p)\n",
          StartPage, NumberOfPages, StartPage * PAGE_SIZE + KSEG0_BASE);

    /* Identity mapping, the loader keeps running there after the switch */
    if (MempMapRangeOfPages(StartPage * PAGE_SIZE,
                            StartPage * PAGE_SIZE,
                            NumberOfPages) != NumberOfPages)
    {
        ERR("Failed to map pages %ld, %ld\n", StartPage, NumberOfPages);
        return FALSE;
    }

    if (KernelMapping)
    {
        if (MempMapRangeOfPages(StartPage * PAGE_SIZE + KSEG0_BASE,
                                StartPage * PAGE_SIZE,
                                NumberOfPages) != NumberOfPages)
        {
            ERR("Failed to map pages %ld, %ld\n", StartPage, NumberOfPages);
            return FALSE;
        }
    }

    return TRUE;
}

VOID
MempUnmapPage(
    _In_ PFN_NUMBER Page)
{
    UNREFERENCED_PARAMETER(Page);
}

/* The debug port stays reachable at its physical address */
static
VOID
WinLdrpMapDebugPorts(VOID)
{
    ULONG i;

    for (i = 1; i < RTL_NUMBER_OF(BaseArray); i++)
    {
        if (!MempIsPageMapped(UlongToPtr(BaseArray[i])))
            MempMapSinglePage(BaseArray[i] & ~(PAGE_SIZE - 1), BaseArray[i] & ~(PAGE_SIZE - 1), ATTR_DEVICE_NGNRNE);
    }

    if (ComPortAddress && !MempIsPageMapped(ComPortAddress))
    {
        MempMapSinglePage((ULONG_PTR)ComPortAddress & ~(PAGE_SIZE - 1),
                          (ULONG_PTR)ComPortAddress & ~(PAGE_SIZE - 1),
                          ATTR_DEVICE_NGNRNE);
    }
}

static
BOOLEAN
WinLdrMapSpecialPages(VOID)
{
    PHARDWARE_PTE PpeBase, PdeBase;

    if (!MempMapSinglePage(KI_USER_SHARED_DATA, SharedUserDataPfn * PAGE_SIZE, ATTR_NORMAL))
    {
        ERR("Could not map KI_USER_SHARED_DATA\n");
        return FALSE;
    }

    WinLdrpMapDebugPorts();

    /* Page tables for the 4 MB HAL address space */
    PpeBase = MempGetOrCreatePageDir(PxeBase, VAtoPXI(MM_HAL_VA_START));
    PdeBase = MempGetOrCreatePageDir(PpeBase, VAtoPPI(MM_HAL_VA_START));
    MempGetOrCreatePageDir(PdeBase, VAtoPDI(MM_HAL_VA_START));
    MempGetOrCreatePageDir(PdeBase, VAtoPDI(MM_HAL_VA_START + 2 * 1024 * 1024));

    return TRUE;
}

VOID
WinLdrSetProcessorContext(
    _In_ USHORT OperatingSystemVersion)
{
    ULONG64 PaRange, Tcr;

    UNREFERENCED_PARAMETER(OperatingSystemVersion);

    TRACE("WinLdrSetProcessorContext\n");

    _disable();

    PaRange = Arm64GetPhysicalAddressRange();
    if (PaRange > MAX_PA_RANGE)
        PaRange = MAX_PA_RANGE;

    Tcr = TCR_T0SZ(16) | TCR_IRGN0_WBWA | TCR_ORGN0_WBWA | TCR_SH0_INNER | TCR_TG0_4K |
          TCR_T1SZ(16) | TCR_IRGN1_WBWA | TCR_ORGN1_WBWA | TCR_SH1_INNER | TCR_TG1_4K |
          TCR_IPS(PaRange);

    Arm64SwitchTranslation(MAIR_VALUE, Tcr, (ULONG64)PxeBase);

    TRACE("leave WinLdrSetProcessorContext\n");
}

VOID
WinLdrSetupMachineDependent(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PVOID SharedUserDataAddress;
    ULONG El;

    LoaderBlock->u.I386.CommonDataArea = NULL;
    LoaderBlock->u.I386.MachineType = 0;

    /* The kernel runs at EL1, from EL2 the switch to the kernel tables drops there.
     * A host running with E2H has its EL1 register names redirected to EL2. */
    El = Arm64GetCurrentEl();
    if ((El != 1) && ((El != 2) || (Arm64GetHypervisorConfig() & HCR_EL2_E2H)))
    {
        FrLdrBugCheckWithMessage(MISSING_HARDWARE_REQUIREMENTS,
                                 __FILE__,
                                 __LINE__,
                                 "The loader must run at EL1 or at EL2 without E2H, it runs at EL%lu.",
                                 El);
    }

    SharedUserDataAddress = MmAllocateMemoryWithType(MM_PAGE_SIZE, LoaderStartupPcrPage);
    if (SharedUserDataAddress == NULL)
    {
        UiMessageBox("Can't allocate SharedUserData page.");
        return;
    }
    SharedUserDataPfn = (ULONG_PTR)SharedUserDataAddress >> MM_PAGE_SHIFT;
    RtlZeroMemory(SharedUserDataAddress, MM_PAGE_SIZE);

    if (!MempAllocatePageTables())
    {
        FrLdrBugCheckWithMessage(MEMORY_INIT_FAILURE,
                                 __FILE__,
                                 __LINE__,
                                 "Failed to allocate the page tables.");
    }

    WinLdrMapSpecialPages();
}

VOID
MempDump(VOID)
{
}
