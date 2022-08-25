/*
 * PROJECT:         GSoC 2022, kernel address sanitizer
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/mm/kasan.c
 * PURPOSE:         Manage shadow memory for kernel address sanitization
 * PROGRAMMERS:     Nick DiMeglio
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

/* GLOBALS *******************************************************************/

#define MM_KASAN_SHADOW_MEMORY 0x90000000
#define MM_KASAN_SHADOW_SIZE 0x10000000

/* FUNCTIONS *********************************************************/

CODE_SEG("INIT")
NTSTATUS
MiInitializeShadowMemory(VOID)
{
    PMMPDE MappingPde;
    PMMPTE MappingPte;
    MMPTE TempPte;
    PFN_NUMBER Pfn;
    PVOID ShadowStart = (PVOID) MM_KASAN_SHADOW_MEMORY;
    PVOID ShadowEnd = Add2Ptr(ShadowStart, MM_KASAN_SHADOW_SIZE);

    /* First map all PDEs */
    for (MappingPde = MiAddressToPde(ShadowStart);
         MappingPde < MiAddressToPde(ShadowEnd);
         MappingPde++)
    {
        ASSERT(MappingPde->u.Long == 0);

        /* Get a page */
        Pfn = MxGetNextPage(1);
        if (!Pfn)
            return STATUS_NO_MEMORY;

        MI_MAKE_HARDWARE_PTE_KERNEL(&TempPte, MiPdeToPte(MappingPde), MM_READWRITE, Pfn);
        MI_WRITE_VALID_PDE(MappingPde, TempPte);

        /* Zero out the page table */
        RtlZeroMemory(MiPdeToPte(MappingPde), PAGE_SIZE);
    }

    for (MappingPte = MiAddressToPte(ShadowStart);
         MappingPte < MiAddressToPte(ShadowEnd);
         MappingPte++)
    {
        /* Get a page */
        Pfn = MxGetNextPage(1);
        if (!Pfn)
            return STATUS_NO_MEMORY;

        MI_MAKE_HARDWARE_PTE_KERNEL(&TempPte, MappingPte, MM_READWRITE, Pfn);
        MI_WRITE_VALID_PTE(MappingPte, TempPte);

        /* Zero out the page */
        RtlZeroMemory(MiPteToAddress(MappingPte), PAGE_SIZE);
    }

    return STATUS_SUCCESS;
}
