/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2005       Alex Ionescu  <alex@relsoft.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#define _NTSYSTEM_
#include <freeldr.h>

#define NDEBUG
#include <debug.h>
#undef DbgPrint

/* Page Directory and Tables for non-PAE Systems */
extern PAGE_DIRECTORY_X86 startup_pagedirectory;
extern PAGE_DIRECTORY_X86 lowmem_pagetable;
extern PAGE_DIRECTORY_X86 kernel_pagetable;
extern PAGE_DIRECTORY_X86 hyperspace_pagetable;
extern PAGE_DIRECTORY_X86 apic_pagetable;
extern PAGE_DIRECTORY_X86 kpcr_pagetable;
extern PAGE_DIRECTORY_X86 kuser_pagetable;
extern ULONG_PTR KernelBase;
extern ROS_KERNEL_ENTRY_POINT KernelEntryPoint;
/* FUNCTIONS *****************************************************************/

/*++
 * FrLdrStartup
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
NTAPI
FrLdrStartup(ULONG Magic)
{
    /* Disable Interrupts */
    _disable();

    /* Re-initalize EFLAGS */
    Ke386EraseFlags();

    /* Initialize the page directory */
    FrLdrSetupPageDirectory();

    /* Initialize Paging, Write-Protection and Load NTOSKRNL */
    FrLdrSetupPae(Magic);
}

/*++
 * FrLdrSetupPae
 * INTERNAL
 *
 *     Configures PAE on a MP System, and sets the PDBR if it's supported, or if
 *     the system is UP.
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
FASTCALL
FrLdrSetupPae(ULONG Magic)
{
    ULONG_PTR PageDirectoryBaseAddress = (ULONG_PTR)&startup_pagedirectory;

    /* Set the PDBR */
    __writecr3(PageDirectoryBaseAddress);

    /* Enable Paging and Write Protect*/
    __writecr0(__readcr0() | CR0_PG | CR0_WP);

    /* Jump to Kernel */
    (*KernelEntryPoint)(Magic, &LoaderBlock);
}

/*++
 * FrLdrSetupPageDirectory
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
FrLdrSetupPageDirectory(VOID)
{
    PPAGE_DIRECTORY_X86 PageDir;
    ULONG KernelPageTableIndex;
    ULONG i;

    /* Get the Kernel Table Index */
    KernelPageTableIndex = KernelBase >> PDE_SHIFT;

    /* Get the Startup Page Directory */
    PageDir = (PPAGE_DIRECTORY_X86)&startup_pagedirectory;

    /* Set up the Low Memory PDE */
    PageDir->Pde[LowMemPageTableIndex].Valid = 1;
    PageDir->Pde[LowMemPageTableIndex].Write = 1;
    PageDir->Pde[LowMemPageTableIndex].PageFrameNumber = PaPtrToPfn(lowmem_pagetable);

    /* Set up the Kernel PDEs */
    PageDir->Pde[KernelPageTableIndex].Valid = 1;
    PageDir->Pde[KernelPageTableIndex].Write = 1;
    PageDir->Pde[KernelPageTableIndex].PageFrameNumber = PaPtrToPfn(kernel_pagetable);
    PageDir->Pde[KernelPageTableIndex + 1].Valid = 1;
    PageDir->Pde[KernelPageTableIndex + 1].Write = 1;
    PageDir->Pde[KernelPageTableIndex + 1].PageFrameNumber = PaPtrToPfn(kernel_pagetable + 4096);

    /* Set up the Startup PDE */
    PageDir->Pde[StartupPageTableIndex].Valid = 1;
    PageDir->Pde[StartupPageTableIndex].Write = 1;
    PageDir->Pde[StartupPageTableIndex].PageFrameNumber = PaPtrToPfn(startup_pagedirectory);

    /* Set up the Hyperspace PDE */
    PageDir->Pde[HyperspacePageTableIndex].Valid = 1;
    PageDir->Pde[HyperspacePageTableIndex].Write = 1;
    PageDir->Pde[HyperspacePageTableIndex].PageFrameNumber = PaPtrToPfn(hyperspace_pagetable);

    /* Set up the HAL PDE */
    PageDir->Pde[HalPageTableIndex].Valid = 1;
    PageDir->Pde[HalPageTableIndex].Write = 1;
    PageDir->Pde[HalPageTableIndex].PageFrameNumber = PaPtrToPfn(apic_pagetable);

    /* Set up Low Memory PTEs */
    PageDir = (PPAGE_DIRECTORY_X86)&lowmem_pagetable;
    for (i=0; i<1024; i++)
    {
        PageDir->Pde[i].Valid = 1;
        PageDir->Pde[i].Write = 1;
        PageDir->Pde[i].Owner = 1;
        PageDir->Pde[i].PageFrameNumber = PaToPfn(i * PAGE_SIZE);
    }

    /* Set up Kernel PTEs */
    PageDir = (PPAGE_DIRECTORY_X86)&kernel_pagetable;
    for (i=0; i<1536; i++)
    {
        PageDir->Pde[i].Valid = 1;
        PageDir->Pde[i].Write = 1;
        PageDir->Pde[i].PageFrameNumber = PaToPfn(KERNEL_BASE_PHYS + i * PAGE_SIZE);
    }

    /* Setup APIC Base */
    PageDir = (PPAGE_DIRECTORY_X86)&apic_pagetable;
    PageDir->Pde[0].Valid = 1;
    PageDir->Pde[0].Write = 1;
    PageDir->Pde[0].CacheDisable = 1;
    PageDir->Pde[0].WriteThrough = 1;
    PageDir->Pde[0].PageFrameNumber = PaToPfn(HAL_BASE);
    PageDir->Pde[0x200].Valid = 1;
    PageDir->Pde[0x200].Write = 1;
    PageDir->Pde[0x200].CacheDisable = 1;
    PageDir->Pde[0x200].WriteThrough = 1;
    PageDir->Pde[0x200].PageFrameNumber = PaToPfn(HAL_BASE + KERNEL_BASE_PHYS);

    /* Setup KUSER_SHARED_DATA Base */
    PageDir->Pde[0x1F0].Valid = 1;
    PageDir->Pde[0x1F0].Write = 1;
    PageDir->Pde[0x1F0].PageFrameNumber = 2;

    /* Setup KPCR Base*/
    PageDir->Pde[0x1FF].Valid = 1;
    PageDir->Pde[0x1FF].Write = 1;
    PageDir->Pde[0x1FF].PageFrameNumber = 1;

    /* Zero shared data */
    RtlZeroMemory((PVOID)(2 << MM_PAGE_SHIFT), PAGE_SIZE);
}

