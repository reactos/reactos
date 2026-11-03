/*
 * PROJECT:     ReactOS ARM64 HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Mappings inside the HAL virtual address range
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* Implemented in hal/halarm64/generic/cache.s */
VOID
NTAPI
HalpFlushTbEntry(
    _In_ PVOID VirtualAddress);

VOID
NTAPI
HalpPublishPteWrites(VOID);

/* FUNCTIONS *****************************************************************/

static
BOOLEAN
HalpIsRunFree(
    _In_ PMMPTE PointerPte,
    _In_ PFN_COUNT PageCount)
{
    PFN_COUNT i;

    for (i = 0; i < PageCount; i++)
    {
        if (PointerPte[i].u.Long != 0)
            return FALSE;
    }

    return TRUE;
}

/**
 * @brief Maps physical pages into the HAL address range.
 *
 * The loader already created the page tables for this range, so this works
 * before the memory manager is up.
 */
PVOID
NTAPI
HalpMapPhysicalMemory(
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ PFN_COUNT PageCount,
    _In_ BOOLEAN Device)
{
    PMMPTE FirstPte, LastPte, PointerPte;
    MMPTE TempPte;
    PFN_NUMBER Pfn;
    PFN_COUNT i;

    /* The offset inside the first page may need one more page */
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(PhysicalAddress.LowPart,
                                               PageCount * PAGE_SIZE);

    FirstPte = MiAddressToPte(MM_HAL_VA_START);
    LastPte = MiAddressToPte(MM_HAL_VA_END);

    for (PointerPte = FirstPte; PointerPte + PageCount <= LastPte + 1; PointerPte++)
    {
        if (HalpIsRunFree(PointerPte, PageCount))
            break;
    }

    if (PointerPte + PageCount > LastPte + 1)
        return NULL;

    TempPte.u.Long = 0;
    TempPte.u.Hard.Valid = 1;
    TempPte.u.Hard.NotLargePage = 1;
    TempPte.u.Hard.Accessed = 1;
    TempPte.u.Hard.PrivilegedNoExecute = 1;
    TempPte.u.Hard.UserNoExecute = 1;
    TempPte.u.Hard.Writable = 1;
    if (Device)
    {
        TempPte.u.Hard.CacheType = MI_CACHE_TYPE_DEVICE;
    }
    else
    {
        TempPte.u.Hard.CacheType = MI_CACHE_TYPE_CACHED;
        TempPte.u.Hard.Shareability = 3;
    }

    Pfn = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
    for (i = 0; i < PageCount; i++)
    {
        TempPte.u.Hard.PageFrameNumber = Pfn + i;
        PointerPte[i] = TempPte;
    }

    /* New entries are never cached, the walker only has to see the stores */
    HalpPublishPteWrites();

    return (PVOID)((ULONG_PTR)MiPteToAddress(PointerPte) + BYTE_OFFSET(PhysicalAddress.LowPart));
}

VOID
NTAPI
HalpUnmapVirtualAddress(
    _In_ PVOID VirtualAddress,
    _In_ PFN_COUNT PageCount)
{
    PMMPTE PointerPte;
    ULONG_PTR Address;
    PFN_COUNT i;

    if ((ULONG_PTR)VirtualAddress < MM_HAL_VA_START)
        return;

    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(VirtualAddress, PageCount * PAGE_SIZE);
    Address = (ULONG_PTR)PAGE_ALIGN(VirtualAddress);
    PointerPte = MiAddressToPte(Address);

    for (i = 0; i < PageCount; i++)
    {
        PointerPte[i].u.Long = 0;
        HalpFlushTbEntry((PVOID)(Address + i * PAGE_SIZE));
    }
}
