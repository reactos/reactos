/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Freeloader
 * FILE:            boot/freeldr/freeldr/multiboot.c
 * PURPOSE:         ReactOS Loader
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Hartmut Birr - SMP/PAE Code
 */

#include <freeldr.h>
#include <internal/i386/ke.h>

#define NDEBUG
#include <debug.h>

#include "i386boot.h"
#include "i386mem.h"

#define STARTUP_BASE                0xF0000000
#define HYPERSPACE_BASE             0xF0800000
#define APIC_BASE                   0xFEC00000
#ifdef XEN_VER
#define KPCR_BASE                   0xFB000000
#else
#define KPCR_BASE                   0xFF000000
#endif

#define LowMemPageTableIndex        0
#define StartupPageTableIndex       (STARTUP_BASE >> PDN_SHIFT)
#define HyperspacePageTableIndex    (HYPERSPACE_BASE >> PDN_SHIFT)
#define KpcrPageTableIndex          (KPCR_BASE >> PDN_SHIFT)
#define ApicPageTableIndex          (APIC_BASE >> PDN_SHIFT)

#define LowMemPageTableIndexPae     0
#define StartupPageTableIndexPae    (STARTUP_BASE >> 21)
#define HyperspacePageTableIndexPae (HYPERSPACE_BASE >> 21)
#define KpcrPageTableIndexPae       (KPCR_BASE >> 21)
#define ApicPageTableIndexPae       (APIC_BASE >> 21)

/* FUNCTIONS *****************************************************************/

/*++
 * i386BootAddrToPfn
 * INTERNAL
 *
 *     Translate an address to a page frame number
 *
 * Params:
 *     Addr - address to translate
 *
 * Returns:
 *     Frame number.
 *
 * Remarks:
 *     None.
 *
 *--*/
ULONG
STDCALL
i386BootAddrToPfn(ULONG_PTR Addr)
{
  return PaToPfn(Addr);
}

/*++
 * i386BootStartup
 * INTERNAL
 *
 *     Prepares the system for loading the Kernel.
 *
 * Params:
 *     Magic - Multiboot Magic
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     None.
 *
 *--*/
VOID
STDCALL
i386BootStartup(ULONG Magic)
{
    BOOLEAN PaeModeEnabled;

    /* Disable Interrupts */
    Ke386DisableInterrupts();

    /* Re-initalize EFLAGS */
    Ke386EraseFlags();

    /* Get the PAE Mode */
    PaeModeEnabled = i386BootGetPaeMode();

    /* Initialize the page directory */
    i386BootSetupPageDirectory(PaeModeEnabled, TRUE, i386BootAddrToPfn);

    /* Initialize Paging, Write-Protection and Load NTOSKRNL */
    i386BootSetupPae(PaeModeEnabled, Magic);
}

/*++
 * i386BootSetupPae
 * INTERNAL
 *
 *     Configures PAE on a MP System, and sets the PDBR if it's supported, or if
 *     the system is UP.
 *
 * Params:
 *     PaeModeEnabled - TRUE if booting in PAE mode
 *     Magic - Multiboot Magic
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     None.
 *
 *--*/
VOID
FASTCALL
i386BootSetupPae(BOOLEAN PaeModeEnabled, ULONG Magic)
{
    ULONG_PTR PageDirectoryBaseAddress = (ULONG_PTR)&startup_pagedirectory;
    ASMCODE PagedJump;

    if (PaeModeEnabled)
    {
        PageDirectoryBaseAddress = (ULONG_PTR)&startup_pagedirectorytable_pae;

        /* Enable PAE */
        Ke386SetCr4(Ke386GetCr4() | X86_CR4_PAE);
    }

    /* Set the PDBR */
    Ke386SetPageTableDirectory(PageDirectoryBaseAddress);

    /* Enable Paging and Write Protect*/
    Ke386SetCr0(Ke386GetCr0() | X86_CR0_PG | X86_CR0_WP);

    /* Jump to Kernel */
    PagedJump = (ASMCODE)KernelEntryPoint;
    PagedJump(Magic, &LoaderBlock);
}

/*++
 * i386BootGetPaeMode
 * INTERNAL
 *
 *     Determines whether PAE mode should be enabled or not.
 *
 * Params:
 *     None.
 *
 * Returns:
 *     PAE mode
 *
 * Remarks:
 *     None.
 *
 *--*/
BOOLEAN
FASTCALL
i386BootGetPaeMode(VOID)
{
    BOOLEAN PaeModeEnabled;

    /* FIXME: Read command line */
    PaeModeEnabled = FALSE;

    if (PaeModeEnabled)
    {
    }

    return PaeModeEnabled;
}

/*++
 * i386BootSetupPageDirectory
 * INTERNAL
 *
 *     Sets up the ReactOS Startup Page Directory.
 *
 * Params:
 *     None.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     We are setting PDEs, but using the equvivalent (for our purpose) PTE structure.
 *     As such, please note that PageFrameNumber == PageEntryNumber.
 *
 *--*/
VOID
FASTCALL
i386BootSetupPageDirectory(BOOLEAN PaeModeEnabled, BOOLEAN SetupApic,
                           ULONG (STDCALL *AddrToPfn)(ULONG_PTR Addr))
{
    PPAGE_DIRECTORY_X86 PageDir;
    PPAGE_DIRECTORY_TABLE_X64 PageDirTablePae;
    PPAGE_DIRECTORY_X64 PageDirPae;
    ULONG KernelPageTableIndex;
    ULONG i;

    if (PaeModeEnabled) {

        /* Get the Kernel Table Index */
        KernelPageTableIndex = (KernelBase >> 21);

        /* Get the Startup Page Directory Table */
        PageDirTablePae = (PPAGE_DIRECTORY_TABLE_X64)&startup_pagedirectorytable_pae;

        /* Get the Startup Page Directory */
        PageDirPae = (PPAGE_DIRECTORY_X64)&startup_pagedirectory_pae;

        /* Set the Startup page directory table */
        for (i = 0; i < 4; i++)
        {
            PageDirTablePae->Pde[i].Valid = 1;
            PageDirTablePae->Pde[i].PageFrameNumber = PaPtrToPfn(startup_pagedirectory_pae) + i;
        }

        /* Set up the Low Memory PDE */
        for (i = 0; i < 2; i++)
        {
            PageDirPae->Pde[LowMemPageTableIndexPae + i].Valid = 1;
            PageDirPae->Pde[LowMemPageTableIndexPae + i].Write = 1;
            PageDirPae->Pde[LowMemPageTableIndexPae + i].PageFrameNumber = PaPtrToPfn(lowmem_pagetable_pae) + i;
        }

        /* Set up the Kernel PDEs */
        for (i = 0; i < 3; i++)
        {
            PageDirPae->Pde[KernelPageTableIndex + i].Valid = 1;
            PageDirPae->Pde[KernelPageTableIndex + i].Write = 1;
            PageDirPae->Pde[KernelPageTableIndex + i].PageFrameNumber = PaPtrToPfn(kernel_pagetable_pae) + i;
        }

        /* Set up the Startup PDE */
        for (i = 0; i < 4; i++)
        {
            PageDirPae->Pde[StartupPageTableIndexPae + i].Valid = 1;
            PageDirPae->Pde[StartupPageTableIndexPae + i].Write = 1;
            PageDirPae->Pde[StartupPageTableIndexPae + i].PageFrameNumber = PaPtrToPfn(startup_pagedirectory_pae) + i;
        }

        /* Set up the Hyperspace PDE */
        for (i = 0; i < 2; i++)
        {
            PageDirPae->Pde[HyperspacePageTableIndexPae + i].Valid = 1;
            PageDirPae->Pde[HyperspacePageTableIndexPae + i].Write = 1;
            PageDirPae->Pde[HyperspacePageTableIndexPae + i].PageFrameNumber = PaPtrToPfn(hyperspace_pagetable_pae) + i;
        }

        /* Set up the Apic PDE */
        for (i = 0; i < 2; i++)
        {
            PageDirPae->Pde[ApicPageTableIndexPae + i].Valid = 1;
            PageDirPae->Pde[ApicPageTableIndexPae + i].Write = 1;
            PageDirPae->Pde[ApicPageTableIndexPae + i].PageFrameNumber = PaPtrToPfn(apic_pagetable_pae) + i;
        }

        /* Set up the KPCR PDE */
        PageDirPae->Pde[KpcrPageTableIndexPae].Valid = 1;
        PageDirPae->Pde[KpcrPageTableIndexPae].Write = 1;
        PageDirPae->Pde[KpcrPageTableIndexPae].PageFrameNumber = PaPtrToPfn(kpcr_pagetable_pae);

        /* Set up Low Memory PTEs */
        PageDirPae = (PPAGE_DIRECTORY_X64)&lowmem_pagetable_pae;
        for (i=0; i<1024; i++) {

            PageDirPae->Pde[i].Valid = 1;
            PageDirPae->Pde[i].Write = 1;
            PageDirPae->Pde[i].Owner = 1;
            PageDirPae->Pde[i].PageFrameNumber = i;
        }

        /* Set up Kernel PTEs */
        PageDirPae = (PPAGE_DIRECTORY_X64)&kernel_pagetable_pae;
        for (i=0; i<1536; i++) {

            PageDirPae->Pde[i].Valid = 1;
            PageDirPae->Pde[i].Write = 1;
            PageDirPae->Pde[i].PageFrameNumber = PaToPfn(KERNEL_BASE_PHYS) + i;
        }

        /* Set up APIC PTEs */
        PageDirPae = (PPAGE_DIRECTORY_X64)&apic_pagetable_pae;
        PageDirPae->Pde[0].Valid = 1;
        PageDirPae->Pde[0].Write = 1;
        PageDirPae->Pde[0].CacheDisable = 1;
        PageDirPae->Pde[0].WriteThrough = 1;
        PageDirPae->Pde[0].PageFrameNumber = PaToPfn(APIC_BASE);
        PageDirPae->Pde[0x200].Valid = 1;
        PageDirPae->Pde[0x200].Write = 1;
        PageDirPae->Pde[0x200].CacheDisable = 1;
        PageDirPae->Pde[0x200].WriteThrough = 1;
        PageDirPae->Pde[0x200].PageFrameNumber = PaToPfn(APIC_BASE + KERNEL_BASE_PHYS);

        /* Set up KPCR PTEs */
        PageDirPae = (PPAGE_DIRECTORY_X64)&kpcr_pagetable_pae;
        PageDirPae->Pde[0].Valid = 1;
        PageDirPae->Pde[0].Write = 1;
        PageDirPae->Pde[0].PageFrameNumber = 1;

    } else {

        /* Get the Kernel Table Index */
        KernelPageTableIndex = KernelBase >> PDN_SHIFT;

        /* Get the Startup Page Directory */
        PageDir = (PPAGE_DIRECTORY_X86)&startup_pagedirectory;

        /* Set up the Low Memory PDE */
        PageDir->Pde[LowMemPageTableIndex].Valid = 1;
        PageDir->Pde[LowMemPageTableIndex].Write = 1;
        PageDir->Pde[LowMemPageTableIndex].PageFrameNumber = (*AddrToPfn)((ULONG_PTR) &lowmem_pagetable);

        /* Set up the Kernel PDEs */
        PageDir->Pde[KernelPageTableIndex].Valid = 1;
        PageDir->Pde[KernelPageTableIndex].Write = 1;
        PageDir->Pde[KernelPageTableIndex].PageFrameNumber = (*AddrToPfn)((ULONG_PTR) &kernel_pagetable);
        PageDir->Pde[KernelPageTableIndex + 1].Valid = 1;
        PageDir->Pde[KernelPageTableIndex + 1].Write = 1;
        PageDir->Pde[KernelPageTableIndex + 1].PageFrameNumber = (*AddrToPfn)((ULONG_PTR) &kernel_pagetable + 4096);

        /* Set up the Startup PDE */
        PageDir->Pde[StartupPageTableIndex].Valid = 1;
        PageDir->Pde[StartupPageTableIndex].Write = 1;
        PageDir->Pde[StartupPageTableIndex].PageFrameNumber = (*AddrToPfn)((LONG_PTR) &startup_pagedirectory);

        /* Set up the Hyperspace PDE */
        PageDir->Pde[HyperspacePageTableIndex].Valid = 1;
        PageDir->Pde[HyperspacePageTableIndex].Write = 1;
        PageDir->Pde[HyperspacePageTableIndex].PageFrameNumber = (*AddrToPfn)((LONG_PTR) &hyperspace_pagetable);

        /* Set up the Apic PDE */
        if (SetupApic) {
            PageDir->Pde[ApicPageTableIndex].Valid = 1;
            PageDir->Pde[ApicPageTableIndex].Write = 1;
            PageDir->Pde[ApicPageTableIndex].PageFrameNumber = (*AddrToPfn)((LONG_PTR) &apic_pagetable);
        }

        /* Set up the KPCR PDE */
        PageDir->Pde[KpcrPageTableIndex].Valid = 1;
        PageDir->Pde[KpcrPageTableIndex].Write = 1;
        PageDir->Pde[KpcrPageTableIndex].PageFrameNumber = (*AddrToPfn)((LONG_PTR) &kpcr_pagetable);

        /* Set up Low Memory PTEs */
        PageDir = (PPAGE_DIRECTORY_X86)&lowmem_pagetable;
        for (i=0; i<1024; i++) {

            PageDir->Pde[i].Valid = 1;
            PageDir->Pde[i].Write = 1;
            PageDir->Pde[i].Owner = 1;
            PageDir->Pde[i].PageFrameNumber = (*AddrToPfn)(i * PAGE_SIZE);
        }

        /* Set up Kernel PTEs */
        PageDir = (PPAGE_DIRECTORY_X86)&kernel_pagetable;
        for (i=0; i<1536; i++) {

            PageDir->Pde[i].Valid = 1;
            PageDir->Pde[i].Write = 1;
            PageDir->Pde[i].PageFrameNumber = (*AddrToPfn)(KERNEL_BASE_PHYS + i * PAGE_SIZE);
        }

        /* Set up APIC PTEs */
        if (SetupApic) {
            PageDir = (PPAGE_DIRECTORY_X86)&apic_pagetable;
            PageDir->Pde[0].Valid = 1;
            PageDir->Pde[0].Write = 1;
            PageDir->Pde[0].CacheDisable = 1;
            PageDir->Pde[0].WriteThrough = 1;
            PageDir->Pde[0].PageFrameNumber = (*AddrToPfn)(APIC_BASE);
            PageDir->Pde[0x200].Valid = 1;
            PageDir->Pde[0x200].Write = 1;
            PageDir->Pde[0x200].CacheDisable = 1;
            PageDir->Pde[0x200].WriteThrough = 1;
            PageDir->Pde[0x200].PageFrameNumber = (*AddrToPfn)(APIC_BASE + KERNEL_BASE_PHYS);
        }

        /* Set up KPCR PTEs */
        PageDir = (PPAGE_DIRECTORY_X86)&kpcr_pagetable;
        PageDir->Pde[0].Valid = 1;
        PageDir->Pde[0].Write = 1;
        PageDir->Pde[0].PageFrameNumber = (*AddrToPfn)(PAGE_SIZE);
    }
    return;
}
