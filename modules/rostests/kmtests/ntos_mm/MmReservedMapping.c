/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Reserved Mapping test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#ifdef _M_IX86

#define PTE_BASE    0xC0000000
#define MiAddressToPte(x) \
    ((PMMPTE)(((((ULONG)(x)) >> 12) << 2) + PTE_BASE))
#define MiPteToAddress(_Pte) ((PVOID)((ULONG)(_Pte) << 10))

#elif defined(_M_AMD64)

#define PTI_SHIFT  12L
#define PTE_BASE    0xFFFFF68000000000ULL
PMMPTE
FORCEINLINE
_MiAddressToPte(PVOID Address)
{
    ULONG64 Offset = (ULONG64)Address >> (PTI_SHIFT - 3);
    Offset &= 0xFFFFFFFFFULL << 3;
    return (PMMPTE)(PTE_BASE + Offset);
}
#define MiAddressToPte(x) _MiAddressToPte((PVOID)(x))

#endif

static
_Must_inspect_result_
_IRQL_requires_max_ (DISPATCH_LEVEL)
PMDL
(NTAPI
*pMmAllocatePagesForMdlEx)(
    _In_ PHYSICAL_ADDRESS LowAddress,
    _In_ PHYSICAL_ADDRESS HighAddress,
    _In_ PHYSICAL_ADDRESS SkipBytes,
    _In_ SIZE_T TotalBytes,
    _In_ MEMORY_CACHING_TYPE CacheType,
    _In_ ULONG Flags);

static
BOOLEAN
ValidateMapping(
    _In_ PVOID BaseAddress,
    _In_ ULONG TotalPtes,
    _In_ ULONG PoolTag,
    _In_ ULONG ValidPtes,
    _In_ PPFN_NUMBER Pfns)
{
    BOOLEAN Valid = TRUE;
#if defined(_M_IX86) || defined(_M_AMD64)
    PMMPTE PointerPte;
    ULONG i;

    PointerPte = MiAddressToPte(BaseAddress);
    for (i = 0; i < ValidPtes; i++)
    {
        Valid = Valid &&
                ok(PointerPte[i].u.Hard.Valid == 1,
                   "[%lu] PTE %p is not valid\n", i, &PointerPte[i]);

        Valid = Valid &&
                ok(PointerPte[i].u.Hard.PageFrameNumber == Pfns[i],
                   "[%lu] PTE %p has PFN %Ix, expected %Ix\n",
                   i, &PointerPte[i], PointerPte[i].u.Hard.PageFrameNumber, Pfns[i]);
    }
    for (; i < TotalPtes; i++)
    {
        Valid = Valid &&
                ok_eq_hex(PointerPte[i].u.Long, 0UL);
    }
    Valid = Valid &&
            ok_eq_tag(PointerPte[-1].u.Long, PoolTag & ~1);
    Valid = Valid &&
            ok_eq_ulong(PointerPte[-2].u.Long, (TotalPtes + 2) * 2);
#endif

    return Valid;
}

static
VOID
TestMap(
    _In_ PVOID Mapping,
    _In_ ULONG TotalPtes,
    _In_ ULONG PoolTag)
{
    PMDL Mdl;
    PHYSICAL_ADDRESS ZeroPhysical;
    PHYSICAL_ADDRESS MaxPhysical;
    PVOID BaseAddress;
    PPFN_NUMBER MdlPages;
    ULONG i;

    if (skip(pMmAllocatePagesForMdlEx != NULL, "MmAllocatePagesForMdlEx unavailable\n"))
    {
        return;
    }

    ZeroPhysical.QuadPart = 0;
    MaxPhysical.QuadPart = 0xffffffffffffffffLL;

    /* Create a one-page MDL and map it */
    Mdl = pMmAllocatePagesForMdlEx(ZeroPhysical,
                                   MaxPhysical,
                                   ZeroPhysical,
                                   PAGE_SIZE,
                                   MmCached,
                                   0);
    if (skip(Mdl != NULL, "No MDL\n"))
    {
        return;
    }

    MdlPages = (PVOID)(Mdl + 1);

    BaseAddress = MmMapLockedPagesWithReservedMapping(Mapping,
                                                      PoolTag,
                                                      Mdl,
                                                      MmCached);
    ok(BaseAddress != NULL, "MmMapLockedPagesWithReservedMapping failed\n");
    if (!skip(BaseAddress != NULL, "Failed to map MDL\n"))
    {
        ok_eq_pointer(BaseAddress, Mapping);

        ok_bool_true(ValidateMapping(BaseAddress, TotalPtes, PoolTag, 1, MdlPages),
                     "ValidateMapping returned");

        KmtStartSeh()
            *(volatile ULONG *)BaseAddress = 0x01234567;
        KmtEndSeh(STATUS_SUCCESS);

        MmUnmapReservedMapping(BaseAddress,
                               PoolTag,
                               Mdl);

        ok_bool_true(ValidateMapping(Mapping, TotalPtes, PoolTag, 0, NULL),
                     "ValidateMapping returned");
    }

    /* Try again but at an unaligned address */
    BaseAddress = MmMapLockedPagesWithReservedMapping((PUCHAR)Mapping + sizeof(ULONG),
                                                      PoolTag,
                                                      Mdl,
                                                      MmCached);
    ok(BaseAddress != NULL, "MmMapLockedPagesWithReservedMapping failed\n");
    if (!skip(BaseAddress != NULL, "Failed to map MDL\n"))
    {
        ok_eq_pointer(BaseAddress, (PUCHAR)Mapping + sizeof(ULONG));

        ok_bool_true(ValidateMapping(BaseAddress, TotalPtes, PoolTag, 1, MdlPages),
                     "ValidateMapping returned");

        KmtStartSeh()
            *(volatile ULONG *)BaseAddress = 0x01234567;
        KmtEndSeh(STATUS_SUCCESS);

        MmUnmapReservedMapping(BaseAddress,
                               PoolTag,
                               Mdl);

        ok_bool_true(ValidateMapping(Mapping, TotalPtes, PoolTag, 0, NULL),
                     "ValidateMapping returned");
    }

    MmFreePagesFromMdl(Mdl);

    /* Map all pages */
    Mdl = pMmAllocatePagesForMdlEx(ZeroPhysical,
                                   MaxPhysical,
                                   ZeroPhysical,
                                   TotalPtes * PAGE_SIZE,
                                   MmCached,
                                   0);
    if (skip(Mdl != NULL, "No MDL\n"))
    {
        return;
    }

    MdlPages = (PVOID)(Mdl + 1);

    BaseAddress = MmMapLockedPagesWithReservedMapping(Mapping,
                                                      PoolTag,
                                                      Mdl,
                                                      MmCached);
    ok(BaseAddress != NULL, "MmMapLockedPagesWithReservedMapping failed\n");
    if (!skip(BaseAddress != NULL, "Failed to map MDL\n"))
    {
        ok_eq_pointer(BaseAddress, Mapping);

        ok_bool_true(ValidateMapping(BaseAddress, TotalPtes, PoolTag, TotalPtes, MdlPages),
                     "ValidateMapping returned");

        for (i = 0; i < TotalPtes; i++)
        {
            KmtStartSeh()
                *((volatile ULONG *)BaseAddress + i * PAGE_SIZE / sizeof(ULONG)) = 0x01234567;
            KmtEndSeh(STATUS_SUCCESS);
        }

        MmUnmapReservedMapping(BaseAddress,
                               PoolTag,
                               Mdl);

        ok_bool_true(ValidateMapping(Mapping, TotalPtes, PoolTag, 0, NULL),
                     "ValidateMapping returned");
    }

    MmFreePagesFromMdl(Mdl);

    /* Try to map more pages than we reserved */
    Mdl = pMmAllocatePagesForMdlEx(ZeroPhysical,
                                   MaxPhysical,
                                   ZeroPhysical,
                                   (TotalPtes + 1) * PAGE_SIZE,
                                   MmCached,
                                   0);
    if (skip(Mdl != NULL, "No MDL\n"))
    {
        return;
    }

    BaseAddress = MmMapLockedPagesWithReservedMapping(Mapping,
                                                      PoolTag,
                                                      Mdl,
                                                      MmCached);
    ok_eq_pointer(BaseAddress, NULL);
    if (BaseAddress)
    {
        MmUnmapReservedMapping(BaseAddress,
                               PoolTag,
                               Mdl);
    }

    MmFreePagesFromMdl(Mdl);
}

START_TEST(MmReservedMapping)
{
    PVOID Mapping;

    pMmAllocatePagesForMdlEx = KmtGetSystemRoutineAddress(L"MmAllocatePagesForMdlEx");

    /* one byte - single page */
    Mapping = MmAllocateMappingAddress(1, 'MRmK');
    ok(Mapping != NULL, "MmAllocateMappingAddress failed\n");
    if (!skip(Mapping != NULL, "No mapping\n"))
    {
        ok_bool_true(ValidateMapping(Mapping, 1, 'MRmK', 0, NULL),
                     "ValidateMapping returned");

        MmFreeMappingAddress(Mapping, 'MRmK');
    }

    /* 10 pages */
    Mapping = MmAllocateMappingAddress(10 * PAGE_SIZE, 'MRmK' & ~1);
    ok(Mapping != NULL, "MmAllocateMappingAddress failed\n");
    if (!skip(Mapping != NULL, "No mapping\n"))
    {
        ok_bool_true(ValidateMapping(Mapping, 10, 'MRmK', 0, NULL),
                     "ValidateMapping returned");

        /* PAGE_FAULT_IN_NONPAGED_AREA can't be caught with SEH */
        if (0)
        {
            (void)*(volatile UCHAR *)Mapping;
        }

        TestMap(Mapping, 10, 'MRmK');

        MmFreeMappingAddress(Mapping, 'MRmK');
    }

    /* PoolTag = 0 */
    Mapping = MmAllocateMappingAddress(1, 0);
    ok(Mapping == NULL, "MmAllocateMappingAddress failed\n");
    if (Mapping != NULL)
    {
        MmFreeMappingAddress(Mapping, 0);
    }

    /* PoolTag = 1 */
    Mapping = MmAllocateMappingAddress(1, 1);
    ok(Mapping != NULL, "MmAllocateMappingAddress failed\n");
    if (Mapping != NULL)
    {
        ok_bool_true(ValidateMapping(Mapping, 1, 1, 0, NULL),
                     "ValidateMapping returned");

        TestMap(Mapping, 1, 1);

        MmFreeMappingAddress(Mapping, 1);
    }

    /* Free an unaligned address */
    Mapping = MmAllocateMappingAddress(PAGE_SIZE, 'MRmK');
    ok(Mapping != NULL, "MmAllocateMappingAddress failed\n");
    if (Mapping != NULL)
    {
        ok_bool_true(ValidateMapping(Mapping, 1, 'MRmK', 0, NULL),
                     "ValidateMapping returned");

        TestMap(Mapping, 1, 'MRmK');

        MmFreeMappingAddress((PUCHAR)Mapping + sizeof(ULONG), 'MRmK');
    }
}
