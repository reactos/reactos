/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Physical Address Extension (PAE) paging mode definitions.
 * COPYRIGHT:   Copyright 2022 Vadim Galyant <vgal@rambler.ru>
 */

#pragma once

#define PAE_TABLES_START (ULONG_PTR)0xC0000000

#define PAE_PTE_PER_PAGE (PAGE_SIZE / sizeof(HARDWARE_PTE_X86_PAE)) // 0x200 (512)
#define PAE_PDE_PER_PAGE (PAGE_SIZE / sizeof(HARDWARE_PDE_X86_PAE)) // 0x200 (512)

#define PAE_PTE_MASK     (PAE_PTE_PER_PAGE - 1)
#define PAE_PDE_MASK     (PAE_PDE_PER_PAGE - 1)

typedef struct _PAGE_DIRECTORY_X86_PAE
{
    HARDWARE_PDE_X86_PAE PaePde[PAE_PDE_PER_PAGE];
} PAGE_DIRECTORY_X86_PAE, *PPAGE_DIRECTORY_X86_PAE;
C_ASSERT(sizeof(PAGE_DIRECTORY_X86_PAE) == PAGE_SIZE);

/* Number of Page Directories for PAE mode (two most significant bits in the VA) */
#define PAE_PD_COUNT   4
#define PAE_PD_PADDING (PAGE_SIZE - (PAE_PD_COUNT * sizeof(HARDWARE_PDPTE_X86_PAE)))
#define PDPTE_SHIFT    30

#define MAX_PAE_PDE_COUNT (PAE_PD_COUNT * PAE_PDE_PER_PAGE)

/* The size of the virtual memory area that is mapped using a single PDE */
#define PAE_PDE_MAPPED_VA (PAE_PTE_PER_PAGE * PAGE_SIZE)

/* Count PT and PTE need for HAL range */
#define HAL_PT_COUNT  ((0xFFFFFFFF - 0xFFC00000 + 1) / PAE_PDE_MAPPED_VA)
#define HAL_PTE_COUNT (HAL_PT_COUNT * PAE_PTE_PER_PAGE)

/* Page-Directory-Pointer-Table should be aligned on 32-byte boundary
   Allocating memory is always page-aligned, so this condition will be met.
*/
typedef struct _PAE_TABLES
{
    HARDWARE_PDPTE_X86_PAE Pdpte[PAE_PD_COUNT];  // PAE Page-Directory-Pointer-Table Entries (should be first)
    UCHAR Padding[PAE_PD_PADDING];
    PAGE_DIRECTORY_X86_PAE PaePd[PAE_PD_COUNT];  // PAE Page Directories
    HARDWARE_PTE_X86_PAE HalPt[HAL_PTE_COUNT];   // PAE Page Tables for HAL
    HARDWARE_PTE_X86_PAE PaePte[0];              // Start PAE Page Tables

} PAE_TABLES, *PPAE_TABLES;
C_ASSERT(sizeof(PAE_TABLES) == ((1 + PAE_PD_COUNT + 2) * PAGE_SIZE));

#define KERNEL_PDE_IDX (KSEG0_BASE / PAE_PDE_MAPPED_VA)

/* EOF */
