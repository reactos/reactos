/*
 * PROJECT:     ReactOS Virtual Memory Allocation Test
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Replicate the GTK-Gnutella VMM virtual memory allocation
 *              pattern from mingw32.c (around line 4780) to expose the
 *              issue described in CORE-15087.
 *
 * The algorithm mirrors mingw_vmm_init() / mingw_valloc() / mingw_vfree_fragment()
 * from gtk-gnutella/src/lib/mingw32.c:
 *
 *  1. Probe available address space with GetSystemInfo().
 *  2. Iteratively reserve a "later" block (PAGE_NOACCESS) by reducing the
 *     requested size by 10% per attempt until VirtualAlloc succeeds, with
 *     VMM_MINSIZE (100 MiB) as the lower bound.
 *  3. Reserve the main VMM pool (PAGE_NOACCESS) by stepping down by the
 *     system allocation granularity per attempt.
 *  4. Release both probe reservations, rebalance to an 80/20 split of the
 *     total available space, then make the final VMM pool reservation.
 *  5. Serve allocations by committing pages from within the reserved pool
 *     (MEM_COMMIT | PAGE_READWRITE) using a sequential base pointer.
 *  6. Free allocations with MEM_DECOMMIT only - never MEM_RELEASE on
 *     sub-regions.  MEM_RELEASE is only used on the whole pool at shutdown.
 */

#define STANDALONE
#include <wine/test.h>
#include <string.h>
#include <windows.h>

/* Constants from gtk-gnutella/src/lib/mingw32.c */
#define VMM_MINSIZE     (100UL * 1024UL * 1024UL) /* 100 MiB minimum pool  */
#define VMM_GRANULARITY (  4UL * 1024UL * 1024UL) /* 4 MiB stepping stride */

/* ------------------------------------------------------------------ */
/* State that mirrors the mingw_vmm struct in mingw32.c               */
/* ------------------------------------------------------------------ */
static struct {
    void   *reserved;    /* base of reserved VMM pool          */
    void   *base;        /* next allocation address in pool     */
    size_t  size;        /* total size of reserved pool (bytes) */
    size_t  consumed;    /* bytes handed out so far             */
    size_t  allocated;   /* bytes currently live (committed)    */
} g_vmm;

/* ------------------------------------------------------------------ */
/* Round `n' up to the nearest multiple of `align' (must be pow2)     */
/* ------------------------------------------------------------------ */
static SIZE_T
round_up(SIZE_T n, SIZE_T align)
{
    return (n + align - 1) & ~(align - 1);
}

/* ------------------------------------------------------------------ */
/* Phase 1-4: mingw_vmm_init() equivalent                             */
/* ------------------------------------------------------------------ */
static int
vmm_init(void)
{
    SYSTEM_INFO si;
    SIZE_T total_addr_space;
    SIZE_T granularity;
    SIZE_T mem_size;
    SIZE_T mem_latersize;
    SIZE_T mem_available;
    void  *mem_later = NULL;

    GetSystemInfo(&si);

    total_addr_space = (SIZE_T)si.lpMaximumApplicationAddress
                     - (SIZE_T)si.lpMinimumApplicationAddress;

    /* Use max of system granularity and VMM_GRANULARITY to speed up init */
    granularity = si.dwAllocationGranularity;
    if (granularity < VMM_GRANULARITY)
        granularity = VMM_GRANULARITY;

    trace("  Page size                      : %lu bytes\n",
           (unsigned long)si.dwPageSize);
    trace("  Allocation granularity         : %lu KiB\n",
           (unsigned long)(granularity / 1024));
    trace("  User address space             : %lu MiB\n",
           (unsigned long)(total_addr_space / (1024 * 1024)));

    mem_size = total_addr_space;

    /*
     * Step 1 - probe "later" block (mirrors the reserve_less: label).
     *
     * Start at mem_size, keep multiplying by 0.9 and clamp to VMM_MINSIZE.
     * This loop terminates because mem_latersize is reduced each iteration
     * and VirtualAlloc will eventually succeed once the request is small
     * enough to fit in contiguous free address space.
     */
    mem_latersize = mem_size;
    for (;;)
    {
        mem_latersize = (SIZE_T)(mem_latersize * 0.9);
        if (mem_latersize < VMM_MINSIZE)
            mem_latersize = VMM_MINSIZE;

        mem_later = VirtualAlloc(NULL, mem_latersize,
                                 MEM_RESERVE, PAGE_NOACCESS);
        if (mem_later != NULL)
            break;

        if (mem_latersize == VMM_MINSIZE)
        {
            ok(0, "probe: VirtualAlloc(MEM_RESERVE, %lu MiB, PAGE_NOACCESS)"
                 " failed at VMM_MINSIZE (error %lu)\n",
                 (unsigned long)(VMM_MINSIZE / (1024 * 1024)),
                 GetLastError());
            return 0;
        }
    }
    ok(1, "probe: reserved %lu MiB \"later\" block at %p (PAGE_NOACCESS)\n",
         (unsigned long)(mem_latersize / (1024 * 1024)), mem_later);

    /*
     * Step 2 - probe VMM pool by stepping down by granularity.
     */
    g_vmm.size = round_up(mem_size - mem_latersize, si.dwPageSize);
    g_vmm.reserved = NULL;

    while (g_vmm.reserved == NULL && g_vmm.size > VMM_MINSIZE)
    {
        g_vmm.reserved = VirtualAlloc(NULL, g_vmm.size,
                                      MEM_RESERVE, PAGE_NOACCESS);
        if (g_vmm.reserved == NULL)
            g_vmm.size -= granularity;
    }

    /* Release the "later" probe block now that we know what we got */
    VirtualFree(mem_later, 0, MEM_RELEASE);
    mem_later = NULL;

    if (g_vmm.reserved == NULL)
    {
        ok(0, "probe: could not reserve any VMM pool >= %lu MiB\n",
             (unsigned long)(VMM_MINSIZE / (1024 * 1024)));
        return 0;
    }
    ok(1, "probe: reserved %lu MiB VMM pool at %p (PAGE_NOACCESS)\n",
         (unsigned long)(g_vmm.size / (1024 * 1024)), g_vmm.reserved);

    /*
     * Step 3 - rebalance to 80/20 split of what we actually obtained.
     *
     * available = mem_latersize + g_vmm.size  (what we could reserve)
     * new later  = 20% * available
     * new pool   = 80% * available
     */
    mem_available  = mem_latersize + g_vmm.size;
    mem_latersize  = (SIZE_T)(0.2 * mem_available);
    g_vmm.size     = mem_available - mem_latersize;

    trace("  Available to VMM               : %lu MiB\n",
           (unsigned long)(mem_available / (1024 * 1024)));
    trace("  Final pool size (80%%)          : %lu MiB\n",
           (unsigned long)(g_vmm.size / (1024 * 1024)));
    trace("  \"Later\" headroom (20%%)         : %lu MiB\n",
           (unsigned long)(mem_latersize / (1024 * 1024)));

    /*
     * Step 4 - release probe reservation and make the final reservation
     * at the computed size so the kernel can place it optimally.
     */
    VirtualFree(g_vmm.reserved, 0, MEM_RELEASE);
    g_vmm.reserved = NULL;

    g_vmm.reserved = VirtualAlloc(NULL, g_vmm.size,
                                  MEM_RESERVE, PAGE_NOACCESS);
    if (g_vmm.reserved == NULL)
    {
        /* Fall back to VMM_MINSIZE */
        g_vmm.size    = VMM_MINSIZE;
        g_vmm.reserved = VirtualAlloc(NULL, g_vmm.size,
                                       MEM_RESERVE, PAGE_NOACCESS);
    }

    if (g_vmm.reserved == NULL)
    {
        ok(0, "init: final VirtualAlloc(MEM_RESERVE, %lu MiB, PAGE_NOACCESS)"
             " failed (error %lu)\n",
             (unsigned long)(g_vmm.size / (1024 * 1024)), GetLastError());
        return 0;
    }

    g_vmm.base      = g_vmm.reserved;
    g_vmm.consumed  = 0;
    g_vmm.allocated = 0;

    ok(1, "init: final VMM pool - %lu MiB at %p (PAGE_NOACCESS)\n",
         (unsigned long)(g_vmm.size / (1024 * 1024)), g_vmm.reserved);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Phase 5: mingw_valloc() - commit pages from within the pool        */
/* ------------------------------------------------------------------ */
static void *
vmm_alloc(SIZE_T size)
{
    SYSTEM_INFO si;
    void *hint;
    void *p;

    GetSystemInfo(&si);
    size = round_up(size, si.dwPageSize);

    if (g_vmm.consumed + size > g_vmm.size)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    hint = g_vmm.base;

    /* Commit within the already-reserved region */
    p = VirtualAlloc(hint, size, MEM_COMMIT, PAGE_READWRITE);
    if (p == NULL)
        return NULL;

    g_vmm.base       = (char *)g_vmm.base + size;
    g_vmm.consumed  += size;
    g_vmm.allocated += size;
    return p;
}

/* ------------------------------------------------------------------ */
/* Phase 6: mingw_vfree_fragment() - decommit only, never MEM_RELEASE */
/* ------------------------------------------------------------------ */
static int
vmm_free(void *addr, SIZE_T size)
{
    SYSTEM_INFO si;

    GetSystemInfo(&si);
    size = round_up(size, si.dwPageSize);

    if (!VirtualFree(addr, size, MEM_DECOMMIT))
        return -1;

    g_vmm.allocated -= size;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Shutdown: MEM_RELEASE the entire pool in one call                   */
/* ------------------------------------------------------------------ */
static void
vmm_shutdown(void)
{
    if (g_vmm.reserved != NULL)
    {
        VirtualFree(g_vmm.reserved, 0, MEM_RELEASE);
        g_vmm.reserved = NULL;
    }
}

/* ------------------------------------------------------------------ */
/* Helper: allocate, write, verify, decommit                           */
/* ------------------------------------------------------------------ */
static void
test_alloc_free(const char *label, SIZE_T size)
{
    SIZE_T i;
    unsigned char *p;
    SYSTEM_INFO si;

    GetSystemInfo(&si);

    p = (unsigned char *)vmm_alloc(size);
    if (p == NULL)
    {
        ok(0, "[%s] vmm_alloc(%lu KiB) failed (error %lu)\n",
             label, (unsigned long)(size / 1024), GetLastError());
        return;
    }

    /* Write a recognisable pattern to every page */
    for (i = 0; i < size; i += si.dwPageSize)
        p[i] = (unsigned char)(i & 0xFFu);

    /* Verify */
    for (i = 0; i < size; i += si.dwPageSize)
    {
        if (p[i] != (unsigned char)(i & 0xFFu))
        {
            ok(0, "[%s] read-back mismatch at offset %lu\n",
                 label, (unsigned long)i);
            vmm_free(p, size);
            return;
        }
    }

    if (vmm_free(p, size) != 0)
    {
        ok(0, "[%s] vmm_free(%lu KiB) failed (error %lu)\n",
             label, (unsigned long)(size / 1024), GetLastError());
        return;
    }

    ok(1, "[%s] commit %lu KiB, write, verify, MEM_DECOMMIT - OK\n",
         label, (unsigned long)(size / 1024));
}

/* ------------------------------------------------------------------ */
/* Entry point                                                          */
/* ------------------------------------------------------------------ */
START_TEST(vmm)
{
    trace("GTK-Gnutella VMM allocation test (CORE-15087 / mingw32.c:4780)\n");
    trace("----------------------------------------------------------------\n\n");

    /* Phase 1-4: initialize the VMM pool using the GTK-Gnutella algorithm */
    trace("[Phase 1-4] VMM pool initialization\n");
    if (!vmm_init())
    {
        ok(0, "VMM pool initialization failed - aborting\n");
        return;
    }
    trace("\n");

    /* Phase 5+6: commit / decommit at various sizes within the pool */
    trace("[Phase 5+6] Commit and decommit within the reserved pool\n");
    test_alloc_free("1 page   (4 KiB)",      4UL * 1024UL);
    test_alloc_free("16 pages (64 KiB)",     64UL * 1024UL);
    test_alloc_free("256 KiB",              256UL * 1024UL);
    test_alloc_free("1 MiB",           1UL * 1024UL * 1024UL);
    test_alloc_free("4 MiB",           4UL * 1024UL * 1024UL);
    test_alloc_free("16 MiB",         16UL * 1024UL * 1024UL);
    test_alloc_free("64 MiB",         64UL * 1024UL * 1024UL);
    trace("\n");

    /* Multiple live allocations at the same time */
    trace("[Phase 5+6] Multiple simultaneous live allocations\n");
    {
        void *a, *b, *c;

        a = vmm_alloc(1UL * 1024UL * 1024UL);
        ok(a != NULL, "vmm_alloc(1 MiB) failed (error %lu)\n", GetLastError());

        b = vmm_alloc(4UL * 1024UL * 1024UL);
        ok(b != NULL, "vmm_alloc(4 MiB) failed (error %lu)\n", GetLastError());

        c = vmm_alloc(16UL * 1024UL * 1024UL);
        ok(c != NULL, "vmm_alloc(16 MiB) failed (error %lu)\n", GetLastError());

        if (a != NULL && b != NULL && c != NULL)
        {
            memset(a, 0xAA, 1UL * 1024UL * 1024UL);
            memset(b, 0xBB, 4UL * 1024UL * 1024UL);
            memset(c, 0xCC, 16UL * 1024UL * 1024UL);
            ok(1, "3 live allocations (1+4+16 MiB) committed and written\n");

            vmm_free(c, 16UL * 1024UL * 1024UL);
            vmm_free(b,  4UL * 1024UL * 1024UL);
            vmm_free(a,  1UL * 1024UL * 1024UL);
            ok(1, "3 live allocations decommitted (MEM_DECOMMIT)\n");
        }
        else
        {
            if (a) vmm_free(a,  1UL * 1024UL * 1024UL);
            if (b) vmm_free(b,  4UL * 1024UL * 1024UL);
            if (c) vmm_free(c, 16UL * 1024UL * 1024UL);
        }
    }
    trace("\n");

    /* Phase shutdown: MEM_RELEASE the whole reservation */
    trace("[Shutdown] Release entire VMM pool (MEM_RELEASE)\n");
    vmm_shutdown();
    ok(1, "VMM pool released\n");
}
