/*
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

// This is needed because headers define wrong one for ReactOS
#undef KIP0PCRADDRESS
#define KIP0PCRADDRESS                      0xffdff000

/* Bits to shift to convert a Virtual Address into an Offset in the Page Table */
#define PFN_SHIFT 12

/* Bits to shift to convert a Virtual Address into an Offset in the Page Directory */
#define PDE_SHIFT 22

#define PDE_PER_PAGE (PAGE_SIZE / sizeof(HARDWARE_PDE_X86)) // 0x400 (1024)
#define PTE_PER_PAGE (PAGE_SIZE / sizeof(HARDWARE_PTE_X86)) // 0x400 (1024)

/* Converts a Physical Address Pointer into a Page Frame Number */
#define PaPtrToPfn(p) \
    (((ULONG_PTR)&p) >> PFN_SHIFT)

/* Converts a Physical Address into a Page Frame Number */
#define PaToPfn(p) \
    ((p) >> PFN_SHIFT)

#define STARTUP_BASE                0xC0000000
#define SELFMAP_ENTRY               0x300

#define LowMemPageTableIndex        0
#define StartupPageTableIndex       (STARTUP_BASE >> 22)
#define HalPageTableIndex           (HAL_BASE >> 22)

typedef struct _PAGE_DIRECTORY_X86
{
    HARDWARE_PTE Pde[1024];
} PAGE_DIRECTORY_X86, *PPAGE_DIRECTORY_X86;

/* PAE specific */

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

/* Number of Page Directories for PAE mode (4)
   (two most significant bits in the VA)
*/
#define PAE_PD_COUNT   (1 << 2)
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

/* end PAE specific */

void __cdecl i386DivideByZero(void);
void __cdecl i386DebugException(void);
void __cdecl i386NMIException(void);
void __cdecl i386Breakpoint(void);
void __cdecl i386Overflow(void);
void __cdecl i386BoundException(void);
void __cdecl i386InvalidOpcode(void);
void __cdecl i386FPUNotAvailable(void);
void __cdecl i386DoubleFault(void);
void __cdecl i386CoprocessorSegment(void);
void __cdecl i386InvalidTSS(void);
void __cdecl i386SegmentNotPresent(void);
void __cdecl i386StackException(void);
void __cdecl i386GeneralProtectionFault(void);
void __cdecl i386PageFault(void);
void __cdecl i386CoprocessorError(void);
void __cdecl i386AlignmentCheck(void);
void __cdecl i386MachineCheck(void);

/* EOF */
