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

/* Steal from mminit.c to allocate shadow memory
*  This function is not in mm.h- is there a better way to pull it in here?
*/
CODE_SEG("INIT")
VOID NTAPI
MiCreateArm3StaticMemoryArea(PVOID BaseAddress, SIZE_T Size, BOOLEAN Executable);

//
// Helper function to create initial memory areas.
// The created area is always read/write.
//
CODE_SEG("INIT")
VOID
MiReserveShadowMemory(VOID)
{
    MiCreateArm3StaticMemoryArea(
        (PVOID) MM_KASAN_SHADOW_MEMORY, MM_KASAN_SHADOW_SIZE, FALSE);
}

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

    for (MappingPte = MiAddressToPte(ShadowStart); MappingPte < MiAddressToPte(ShadowEnd); MappingPte++)
    {
        // Check if the PDE is mapped
        MappingPde = MiPteToPde(MappingPte);
        if (MappingPde->u.Hard.Valid == 0)
        {
            // Map the PDE
            Pfn = MxGetNextPage(1);
            if (!Pfn)
                return STATUS_NO_MEMORY;
            MI_MAKE_HARDWARE_PTE_KERNEL(&TempPte, MappingPte, MM_READWRITE, Pfn);
            MI_WRITE_VALID_PDE(MappingPde, TempPte);

            // Zero out the page table
            RtlZeroMemory(MiPdeToPte(MappingPte), PAGE_SIZE);
        }

        // Map the PTE
        Pfn = MxGetNextPage(1);
        if (!Pfn)
            return STATUS_NO_MEMORY;
        MI_MAKE_HARDWARE_PTE_KERNEL(&TempPte, MappingPte, MM_READWRITE, Pfn);
        MI_WRITE_VALID_PTE(MappingPte, TempPte);

        // Zero out the page
        RtlZeroMemory(MiPteToAddress(MappingPte), PAGE_SIZE);
    }

    return STATUS_SUCCESS;
}
