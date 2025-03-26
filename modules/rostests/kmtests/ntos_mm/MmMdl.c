/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Kernel-Mode Test Suite MDL test
 * COPYRIGHT:   Copyright 2015,2023 Thomas Faber (thomas.faber@reactos.org)
 * COPYRIGHT:   Copyright 2017 Pierre Schweitzer (pierre@reactos.org)
 */

#include <kmt_test.h>

static
VOID
TestMmAllocatePagesForMdl(VOID)
{
    PMDL Mdl;
    PHYSICAL_ADDRESS LowAddress;
    PHYSICAL_ADDRESS HighAddress;
    PHYSICAL_ADDRESS SkipBytes;
    PVOID SystemVa;
    PMDL Mdls[32];
    PVOID SystemVas[32];
    ULONG i;
    PPFN_NUMBER MdlPages;
    ULONG MdlPageCount;

    LowAddress.QuadPart = 0;
    HighAddress.QuadPart = -1;
    SkipBytes.QuadPart = 0;
    /* simple allocate/free */
    Mdl = MmAllocatePagesForMdl(LowAddress,
                                HighAddress,
                                SkipBytes,
                                2 * 1024 * 1024);
    ok(Mdl != NULL, "MmAllocatePagesForMdl failed\n");
    if (skip(Mdl != NULL, "No Mdl\n"))
        return;
    ok(MmGetMdlByteCount(Mdl) == 2 * 1024 * 1024, "Byte count: %lu\n", MmGetMdlByteCount(Mdl));
    ok(MmGetMdlVirtualAddress(Mdl) == NULL, "Virtual address: %p\n", MmGetMdlVirtualAddress(Mdl));
    ok(!(Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA), "MdlFlags: %lx\n", Mdl->MdlFlags);
    MdlPages = MmGetMdlPfnArray(Mdl);
    MdlPageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Mdl), MmGetMdlByteCount(Mdl));
    ok(MdlPageCount == 2 * 1024 * 1024 / PAGE_SIZE, "MdlPageCount = %lu\n", MdlPageCount);
    for (i = 0; i < MdlPageCount; i++)
    {
        ok(MdlPages[i] != 0 && MdlPages[i] != (PFN_NUMBER)-1,
           "MdlPages[%lu] = 0x%I64x\n", i, (ULONGLONG)MdlPages[i]);
    }
    MmFreePagesFromMdl(Mdl);
    ExFreePoolWithTag(Mdl, 0);

    /* Now map/unmap it */
    Mdl = MmAllocatePagesForMdl(LowAddress,
                                HighAddress,
                                SkipBytes,
                                2 * 1024 * 1024);
    ok(Mdl != NULL, "MmAllocatePagesForMdl failed\n");
    if (skip(Mdl != NULL, "No Mdl\n"))
        return;
    ok(MmGetMdlByteCount(Mdl) == 2 * 1024 * 1024, "Byte count: %lu\n", MmGetMdlByteCount(Mdl));
    ok(MmGetMdlVirtualAddress(Mdl) == NULL, "Virtual address: %p\n", MmGetMdlVirtualAddress(Mdl));
    ok(!(Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA), "MdlFlags: %lx\n", Mdl->MdlFlags);
    SystemVa = MmMapLockedPagesSpecifyCache(Mdl,
                                            KernelMode,
                                            MmCached,
                                            NULL,
                                            FALSE,
                                            NormalPagePriority);
    ok(SystemVa != NULL, "MmMapLockedPagesSpecifyCache failed\n");
    if (!skip(SystemVa != NULL, "No system VA\n"))
    {
        ok(MmGetMdlByteCount(Mdl) == 2 * 1024 * 1024, "Byte count: %lu\n", MmGetMdlByteCount(Mdl));
        ok(MmGetMdlVirtualAddress(Mdl) == NULL, "Virtual address: %p, System VA: %p\n", MmGetMdlVirtualAddress(Mdl), SystemVa);
        ok(Mdl->MappedSystemVa == SystemVa, "MappedSystemVa: %p, System VA: %p\n", Mdl->MappedSystemVa, SystemVa);
        ok((Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA), "MdlFlags: %lx\n", Mdl->MdlFlags);
        MmUnmapLockedPages(SystemVa, Mdl);
    }
    ok(MmGetMdlByteCount(Mdl) == 2 * 1024 * 1024, "Byte count: %lu\n", MmGetMdlByteCount(Mdl));
    ok(MmGetMdlVirtualAddress(Mdl) == NULL, "Virtual address: %p\n", MmGetMdlVirtualAddress(Mdl));
    ok(!(Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA), "MdlFlags: %lx\n", Mdl->MdlFlags);
    MmFreePagesFromMdl(Mdl);
    ExFreePoolWithTag(Mdl, 0);

    /* Now map it, and free without unmapping */
    Mdl = MmAllocatePagesForMdl(LowAddress,
                                HighAddress,
                                SkipBytes,
                                2 * 1024 * 1024);
    ok(Mdl != NULL, "MmAllocatePagesForMdl failed\n");
    if (skip(Mdl != NULL, "No Mdl\n"))
        return;
    ok(MmGetMdlByteCount(Mdl) == 2 * 1024 * 1024, "Byte count: %lu\n", MmGetMdlByteCount(Mdl));
    ok(MmGetMdlVirtualAddress(Mdl) == NULL, "Virtual address: %p\n", MmGetMdlVirtualAddress(Mdl));
    ok(!(Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA), "MdlFlags: %lx\n", Mdl->MdlFlags);
    SystemVa = MmMapLockedPagesSpecifyCache(Mdl,
                                            KernelMode,
                                            MmCached,
                                            NULL,
                                            FALSE,
                                            NormalPagePriority);
    ok(SystemVa != NULL, "MmMapLockedPagesSpecifyCache failed\n");
    ok(MmGetMdlByteCount(Mdl) == 2 * 1024 * 1024, "Byte count: %lu\n", MmGetMdlByteCount(Mdl));
    ok(MmGetMdlVirtualAddress(Mdl) == NULL, "Virtual address: %p, System VA: %p\n", MmGetMdlVirtualAddress(Mdl), SystemVa);
    ok(Mdl->MappedSystemVa == SystemVa, "MappedSystemVa: %p, System VA: %p\n", Mdl->MappedSystemVa, SystemVa);
    ok((Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA), "MdlFlags: %lx\n", Mdl->MdlFlags);
    MmFreePagesFromMdl(Mdl);
    ExFreePoolWithTag(Mdl, 0);

    /* try to allocate 2 GB -- should succeed (possibly with fewer pages) but not map */
    Mdl = MmAllocatePagesForMdl(LowAddress,
                                HighAddress,
                                SkipBytes,
                                2UL * 1024 * 1024 * 1024);
    ok(Mdl != NULL, "MmAllocatePagesForMdl failed for 2 GB\n");
    if (Mdl != NULL)
    {
        ok(MmGetMdlByteCount(Mdl) <= 2UL * 1024 * 1024 * 1024, "Byte count: %lu\n", MmGetMdlByteCount(Mdl));
        ok(MmGetMdlVirtualAddress(Mdl) == NULL, "Virtual address: %p\n", MmGetMdlVirtualAddress(Mdl));
        ok(!(Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA), "MdlFlags: %lx\n", Mdl->MdlFlags);
        MdlPages = MmGetMdlPfnArray(Mdl);
        MdlPageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Mdl), MmGetMdlByteCount(Mdl));
        ok(MdlPageCount <= 2UL * 1024 * 1024 * 1024 / PAGE_SIZE, "MdlPageCount = %lu\n", MdlPageCount);
        for (i = 0; i < MdlPageCount; i++)
        {
            if (MdlPages[i] == 0 ||
                MdlPages[i] == (PFN_NUMBER)-1)
            {
                ok(0, "MdlPages[%lu] = 0x%I64x\n", i, (ULONGLONG)MdlPages[i]);
            }
        }
        SystemVa = MmMapLockedPagesSpecifyCache(Mdl,
                                                KernelMode,
                                                MmCached,
                                                NULL,
                                                FALSE,
                                                NormalPagePriority);
        ok(SystemVa == NULL, "MmMapLockedPagesSpecifyCache succeeded for 2 GB\n");
        if (SystemVa != NULL)
            MmUnmapLockedPages(SystemVa, Mdl);
        ok(MmGetMdlByteCount(Mdl) <= 2UL * 1024 * 1024 * 1024, "Byte count: %lu\n", MmGetMdlByteCount(Mdl));
        ok(MmGetMdlVirtualAddress(Mdl) == NULL, "Virtual address: %p\n", MmGetMdlVirtualAddress(Mdl));
        ok(!(Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA), "MdlFlags: %lx\n", Mdl->MdlFlags);
        MmFreePagesFromMdl(Mdl);
        ExFreePoolWithTag(Mdl, 0);
    }

    /* now allocate and map 32 MB Mdls until we fail */
    for (i = 0; i < sizeof(Mdls) / sizeof(Mdls[0]); i++)
    {
        Mdls[i] = MmAllocatePagesForMdl(LowAddress,
                                        HighAddress,
                                        SkipBytes,
                                        32 * 1024 * 1024);
        if (Mdls[i] == NULL)
        {
            trace("MmAllocatePagesForMdl failed with i = %lu\n", i);
            break;
        }
        ok(MmGetMdlVirtualAddress(Mdls[i]) == NULL, "Virtual address: %p\n", MmGetMdlVirtualAddress(Mdls[i]));
        ok(!(Mdls[i]->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA), "MdlFlags: %lx\n", Mdls[i]->MdlFlags);
        SystemVas[i] = MmMapLockedPagesSpecifyCache(Mdls[i],
                                                    KernelMode,
                                                    MmCached,
                                                    NULL,
                                                    FALSE,
                                                    NormalPagePriority);
        if (SystemVas[i] == NULL)
        {
            ok(MmGetMdlByteCount(Mdls[i]) <= 32 * 1024 * 1024, "Byte count: %lu\n", MmGetMdlByteCount(Mdls[i]));
            ok(MmGetMdlVirtualAddress(Mdls[i]) == NULL, "Virtual address: %p\n", MmGetMdlVirtualAddress(Mdls[i]));
            ok(!(Mdls[i]->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA), "MdlFlags: %lx\n", Mdls[i]->MdlFlags);
            trace("MmMapLockedPagesSpecifyCache failed with i = %lu\n", i);
            break;
        }
        ok(MmGetMdlByteCount(Mdls[i]) == 32 * 1024 * 1024, "Byte count: %lu\n", MmGetMdlByteCount(Mdls[i]));
        ok(MmGetMdlVirtualAddress(Mdls[i]) == NULL, "Virtual address: %p, System VA: %p\n", MmGetMdlVirtualAddress(Mdls[i]), SystemVas[i]);
        ok(Mdls[i]->MappedSystemVa == SystemVas[i], "MappedSystemVa: %p\n", Mdls[i]->MappedSystemVa, SystemVas[i]);
        ok((Mdls[i]->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA), "MdlFlags: %lx\n", Mdls[i]->MdlFlags);
    }
    for (i = 0; i < sizeof(Mdls) / sizeof(Mdls[0]); i++)
    {
        if (Mdls[i] == NULL)
            break;
        if (SystemVas[i] != NULL)
            MmUnmapLockedPages(SystemVas[i], Mdls[i]);
        MmFreePagesFromMdl(Mdls[i]);
        ExFreePoolWithTag(Mdls[i], 0);
        if (SystemVas[i] == NULL)
            break;
    }
}

static
VOID
TestMmBuildMdlForNonPagedPool(VOID)
{
    PVOID Page;
    PMDL Mdl;

    Page = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, 'Test');
    ok(Page != NULL, "ExAllocatePoolWithTag failed\n");
    if (skip(Page != NULL, "No buffer\n"))
        return;

    Mdl = IoAllocateMdl(Page, PAGE_SIZE, FALSE, FALSE, NULL);
    ok(Mdl != NULL, "IoAllocateMdl failed\n");
    if (skip(Mdl != NULL, "No MDL\n"))
        return;

    ok((Mdl->MdlFlags & MDL_PAGES_LOCKED) == 0, "MDL locked\n");
    ok((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0, "MDL from non paged\n");

    MmBuildMdlForNonPagedPool(Mdl);
    ok((Mdl->MdlFlags & MDL_PAGES_LOCKED) == 0, "MDL locked\n");
    ok((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) != 0, "MDL from paged\n");

    IoFreeMdl(Mdl);
    ExFreePoolWithTag(Page, 'Test');

    Page = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, 'Test');
    ok(Page != NULL, "ExAllocatePoolWithTag failed\n");
    if (skip(Page != NULL, "No buffer\n"))
        return;

    Mdl = IoAllocateMdl(Page, PAGE_SIZE, FALSE, FALSE, NULL);
    ok(Mdl != NULL, "IoAllocateMdl failed\n");
    if (skip(Mdl != NULL, "No MDL\n"))
        return;

    ok((Mdl->MdlFlags & MDL_PAGES_LOCKED) == 0, "MDL locked\n");
    ok((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0, "MDL from non paged\n");

    MmBuildMdlForNonPagedPool(Mdl);
    ok((Mdl->MdlFlags & MDL_PAGES_LOCKED) == 0, "MDL locked\n");
    ok((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) != 0, "MDL from paged\n");

    IoFreeMdl(Mdl);
    ExFreePoolWithTag(Page, 'Test');
}

START_TEST(MmMdl)
{
    TestMmAllocatePagesForMdl();
    TestMmBuildMdlForNonPagedPool();
}
