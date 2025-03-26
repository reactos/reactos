/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Kernel-Mode Test Suite Reserved Mapping test
 * COPYRIGHT:   Copyright 2015,2023 Thomas Faber (thomas.faber@reactos.org)
 * COPYRIGHT:   Copyright 2015 Pierre Schweitzer (pierre@reactos.org)
 */

#include <kmt_test.h>

static BOOLEAN g_IsPae;
static ULONG g_OsVersion;
static BOOLEAN g_IsReactOS;

#ifdef _M_IX86

#define IS_PAE() (g_IsPae)

#define PTE_BASE    0xC0000000

#define MiAddressToPteX86(x) \
    ((PULONG)(((((ULONG)(x)) >> 12) << 2) + PTE_BASE))
#define MiAddressToPtePAE(x) \
    ((PULONGLONG)(((((ULONG)(x)) >> 12) << 3) + PTE_BASE))

#define GET_PTE_VALUE_X86(Addr) (*MiAddressToPteX86(Addr))
#define GET_PTE_VALUE_PAE(Addr) (*MiAddressToPtePAE(Addr))
#define GET_PTE_VALUE(Addr) (IS_PAE() ? GET_PTE_VALUE_PAE(Addr) : GET_PTE_VALUE_X86(Addr))

#define PTE_IS_VALID(PteValue) ((PteValue) & 1)

#define PTE_GET_PFN_X86(PteValue) (((PteValue) >> PAGE_SHIFT) & 0x0fffffULL)
#define PTE_GET_PFN_PAE(PteValue) (((PteValue) >> PAGE_SHIFT) & 0xffffffULL)
#define PTE_GET_PFN(PteValue) (IS_PAE() ? PTE_GET_PFN_PAE(PteValue) : PTE_GET_PFN_X86(PteValue))

#elif defined(_M_AMD64)

#define PTI_SHIFT  12L
#define PTE_BASE    0xFFFFF68000000000ULL
PULONGLONG
FORCEINLINE
_MiAddressToPte(PVOID Address)
{
    ULONG64 Offset = (ULONG64)Address >> (PTI_SHIFT - 3);
    Offset &= 0xFFFFFFFFFULL << 3;
    return (PULONGLONG)(PTE_BASE + Offset);
}
#define MiAddressToPte(x) _MiAddressToPte((PVOID)(x))

#define GET_PTE_VALUE(Addr) (*_MiAddressToPte((PVOID)(Addr)))
#define PTE_IS_VALID(PteValue) ((PteValue) & 1)
#define PTE_GET_PFN(PteValue) (((PteValue) >> PAGE_SHIFT) & 0xFffffffffULL)

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
    PUCHAR CurrentAddress;
    ULONGLONG PteValue, ExpectedValue;
    ULONG i;

    for (i = 0; i < ValidPtes; i++)
    {
        CurrentAddress = (PUCHAR)BaseAddress + i * PAGE_SIZE;
        PteValue = GET_PTE_VALUE(CurrentAddress);
        Valid = Valid &&
                ok(PTE_IS_VALID(PteValue),
                   "[%lu] PTE for %p is not valid (0x%I64x)\n",
                   i, CurrentAddress, PteValue);

        Valid = Valid &&
                ok(PTE_GET_PFN(PteValue) == Pfns[i],
                   "[%lu] PTE for %p has PFN %Ix, expected %Ix\n",
                   i, CurrentAddress, PTE_GET_PFN(PteValue), Pfns[i]);
    }
    for (; i < TotalPtes; i++)
    {
        CurrentAddress = (PUCHAR)BaseAddress + i * PAGE_SIZE;
        PteValue = GET_PTE_VALUE(CurrentAddress);
        Valid = Valid &&
                ok(PteValue == 0,
                   "[%lu] PTE for %p is nonzero (0x%I64x)\n",
                   i, CurrentAddress, PteValue);
    }
    CurrentAddress = (PUCHAR)BaseAddress - 1 * PAGE_SIZE;
    PteValue = GET_PTE_VALUE(CurrentAddress);
    Valid = Valid &&
            ok(PteValue == (PoolTag & ~1ULL),
               "PTE for %p contains 0x%I64x, expected %x\n",
               CurrentAddress, PteValue, PoolTag & ~1);
    CurrentAddress = (PUCHAR)BaseAddress - 2 * PAGE_SIZE;
    PteValue = GET_PTE_VALUE(CurrentAddress);

    if (g_IsReactOS || g_OsVersion >= 0x0600)
    {
        /* On ReactOS and on Vista+ the size is stored in
         * the NextEntry field of a MMPTE_LIST structure */
#ifdef _M_IX86
        ExpectedValue = (TotalPtes + 2) << 12;
#elif defined(_M_AMD64)
        ExpectedValue = ((ULONG64)TotalPtes + 2) << 32;
#endif
    }
    else
    {
        /* On Windows 2003 the size is shifted by 1 bit only */
        ExpectedValue = (TotalPtes + 2) * 2;
    }
    Valid = Valid &&
            ok(PteValue == ExpectedValue,
               "PTE for %p contains 0x%I64x, expected %x\n",
               CurrentAddress, PteValue, ExpectedValue);
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

    g_IsPae = ExIsProcessorFeaturePresent(PF_PAE_ENABLED);
    g_OsVersion = SharedUserData->NtMajorVersion << 8 | SharedUserData->NtMinorVersion;
    g_IsReactOS = *(PULONG)(KI_USER_SHARED_DATA + PAGE_SIZE - sizeof(ULONG)) == 0x8eac705;
    ok(g_IsReactOS == 1, "Not reactos\n");

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
