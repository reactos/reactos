/*
 * Unit test suite for heap functions
 *
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2006 Detlef Riekenberg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wine/test.h"

#define MAGIC_DEAD 0xdeadbeef

static SIZE_T resize_9x(SIZE_T size)
{
    DWORD dwSizeAligned = (size + 3) & ~3;
    return max(dwSizeAligned, 12); /* at least 12 bytes */
}

static void test_sized_HeapAlloc(int nbytes)
{
    int success;
    char *buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nbytes);
    ok(buf != NULL, "allocate failed\n");
    ok(buf[0] == 0, "buffer not zeroed\n");
    success = HeapFree(GetProcessHeap(), 0, buf);
    ok(success, "free failed\n");
}

static void test_sized_HeapReAlloc(int nbytes1, int nbytes2)
{
    int success;
    char *buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nbytes1);
    ok(buf != NULL, "allocate failed\n");
    ok(buf[0] == 0, "buffer not zeroed\n");
    buf = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buf, nbytes2);
    ok(buf != NULL, "reallocate failed\n");
    ok(buf[nbytes2-1] == 0, "buffer not zeroed\n");
    success = HeapFree(GetProcessHeap(), 0, buf);
    ok(success, "free failed\n");
}

static void test_heap(void)
{
    LPVOID  mem;
    LPVOID  msecond;
    DWORD   res;
    UINT    flags;
    HGLOBAL gbl;
    HGLOBAL hsecond;
    SIZE_T  size;

    /* Heap*() functions */
    mem = HeapAlloc(GetProcessHeap(), 0, 0);
    ok(mem != NULL, "memory not allocated for size 0\n");

    mem = HeapReAlloc(GetProcessHeap(), 0, NULL, 10);
    ok(mem == NULL, "memory allocated by HeapReAlloc\n");

    for (size = 0; size <= 256; size++)
    {
        SIZE_T heap_size;
        mem = HeapAlloc(GetProcessHeap(), 0, size);
        heap_size = HeapSize(GetProcessHeap(), 0, mem);
        ok(heap_size == size || heap_size == resize_9x(size), 
            "HeapSize returned %lu instead of %lu or %lu\n", heap_size, size, resize_9x(size));
        HeapFree(GetProcessHeap(), 0, mem);
    }

    /* test some border cases of HeapAlloc and HeapReAlloc */
    mem = HeapAlloc(GetProcessHeap(), 0, 0);
    ok(mem != NULL, "memory not allocated for size 0\n");
    msecond = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, ~(SIZE_T)0 - 7);
    ok(msecond == NULL, "HeapReAlloc(~0 - 7) should have failed\n");
    msecond = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, ~(SIZE_T)0);
    ok(msecond == NULL, "HeapReAlloc(~0) should have failed\n");
    HeapFree(GetProcessHeap(), 0, mem);
    mem = HeapAlloc(GetProcessHeap(), 0, ~(SIZE_T)0);
    ok(mem == NULL, "memory allocated for size ~0\n");

    /* large blocks must be 16-byte aligned */
    mem = HeapAlloc(GetProcessHeap(), 0, 512 * 1024);
    ok( mem != NULL, "failed for size 512K\n" );
    ok( (ULONG_PTR)mem % 16 == 0 || broken((ULONG_PTR)mem % 16) /* win9x */,
        "512K block not 16-byte aligned\n" );
    HeapFree(GetProcessHeap(), 0, mem);

    /* Global*() functions */
    gbl = GlobalAlloc(GMEM_MOVEABLE, 0);
    ok(gbl != NULL, "global memory not allocated for size 0\n");

    gbl = GlobalReAlloc(gbl, 10, GMEM_MOVEABLE);
    ok(gbl != NULL, "Can't realloc global memory\n");
    size = GlobalSize(gbl);
    ok(size >= 10 && size <= 16, "Memory not resized to size 10, instead size=%ld\n", size);

    gbl = GlobalReAlloc(gbl, 0, GMEM_MOVEABLE);
    ok(gbl != NULL, "GlobalReAlloc should not fail on size 0\n");

    size = GlobalSize(gbl);
    ok(size == 0, "Memory not resized to size 0, instead size=%ld\n", size);
    ok(GlobalFree(gbl) == NULL, "Memory not freed\n");
    size = GlobalSize(gbl);
    ok(size == 0, "Memory should have been freed, size=%ld\n", size);

    gbl = GlobalReAlloc(0, 10, GMEM_MOVEABLE);
    ok(gbl == NULL, "global realloc allocated memory\n");

    /* GlobalLock / GlobalUnlock with a valid handle */
    gbl = GlobalAlloc(GMEM_MOVEABLE, 256);

    SetLastError(MAGIC_DEAD);
    mem = GlobalLock(gbl);      /* #1 */
    ok(mem != NULL, "returned %p with %d (expected '!= NULL')\n", mem, GetLastError());
    SetLastError(MAGIC_DEAD);
    flags = GlobalFlags(gbl);
    ok( flags == 1, "returned 0x%04x with %d (expected '0x0001')\n",
        flags, GetLastError());

    SetLastError(MAGIC_DEAD);
    msecond = GlobalLock(gbl);   /* #2 */
    ok( msecond == mem, "returned %p with %d (expected '%p')\n",
        msecond, GetLastError(), mem);
    SetLastError(MAGIC_DEAD);
    flags = GlobalFlags(gbl);
    ok( flags == 2, "returned 0x%04x with %d (expected '0x0002')\n",
        flags, GetLastError());
    SetLastError(MAGIC_DEAD);

    SetLastError(MAGIC_DEAD);
    res = GlobalUnlock(gbl);    /* #1 */
    ok(res, "returned %d with %d (expected '!= 0')\n", res, GetLastError());
    SetLastError(MAGIC_DEAD);
    flags = GlobalFlags(gbl);
    ok( flags , "returned 0x%04x with %d (expected '!= 0')\n",
        flags, GetLastError());

    SetLastError(MAGIC_DEAD);
    res = GlobalUnlock(gbl);    /* #0 */
    /* NT: ERROR_SUCCESS (documented on MSDN), 9x: untouched */
    ok(!res && ((GetLastError() == ERROR_SUCCESS) || (GetLastError() == MAGIC_DEAD)),
        "returned %d with %d (expected '0' with: ERROR_SUCCESS or "
        "MAGIC_DEAD)\n", res, GetLastError());
    SetLastError(MAGIC_DEAD);
    flags = GlobalFlags(gbl);
    ok( !flags , "returned 0x%04x with %d (expected '0')\n",
        flags, GetLastError());

    /* Unlock an already unlocked Handle */
    SetLastError(MAGIC_DEAD);
    res = GlobalUnlock(gbl);
    /* NT: ERROR_NOT_LOCKED,  9x: untouched */
    ok( !res &&
        ((GetLastError() == ERROR_NOT_LOCKED) || (GetLastError() == MAGIC_DEAD)),
        "returned %d with %d (expected '0' with: ERROR_NOT_LOCKED or "
        "MAGIC_DEAD)\n", res, GetLastError());
 
    GlobalFree(gbl);
    /* invalid handles are caught in windows: */
    SetLastError(MAGIC_DEAD);
    hsecond = GlobalFree(gbl);      /* invalid handle: free memory twice */
    ok( (hsecond == gbl) && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned %p with 0x%08x (expected %p with ERROR_INVALID_HANDLE)\n",
        hsecond, GetLastError(), gbl);
    SetLastError(MAGIC_DEAD);
    flags = GlobalFlags(gbl);
    ok( (flags == GMEM_INVALID_HANDLE) && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned 0x%04x with 0x%08x (expected GMEM_INVALID_HANDLE with "
        "ERROR_INVALID_HANDLE)\n", flags, GetLastError());
    SetLastError(MAGIC_DEAD);
    size = GlobalSize(gbl);
    ok( (size == 0) && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned %ld with 0x%08x (expected '0' with ERROR_INVALID_HANDLE)\n",
        size, GetLastError());

    SetLastError(MAGIC_DEAD);
    mem = GlobalLock(gbl);
    ok( (mem == NULL) && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned %p with 0x%08x (expected NULL with ERROR_INVALID_HANDLE)\n",
        mem, GetLastError());

    /* documented on MSDN: GlobalUnlock() return FALSE on failure.
       Win9x and wine return FALSE with ERROR_INVALID_HANDLE, but on 
       NT 3.51 and XPsp2, TRUE with ERROR_INVALID_HANDLE is returned.
       The similar Test for LocalUnlock() works on all Systems */
    SetLastError(MAGIC_DEAD);
    res = GlobalUnlock(gbl);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
        "returned %d with %d (expected ERROR_INVALID_HANDLE)\n",
        res, GetLastError());

    gbl = GlobalAlloc(GMEM_DDESHARE, 100);

    /* first free */
    mem = GlobalFree(gbl);
    ok(mem == NULL, "Expected NULL, got %p\n", mem);

    /* invalid free */
    if (sizeof(void *) != 8)  /* crashes on 64-bit Vista */
    {
        SetLastError(MAGIC_DEAD);
        mem = GlobalFree(gbl);
        ok(mem == gbl || broken(mem == NULL) /* nt4 */, "Expected gbl, got %p\n", mem);
        if (mem == gbl)
            ok(GetLastError() == ERROR_INVALID_HANDLE ||
               GetLastError() == ERROR_INVALID_PARAMETER, /* win9x */
               "Expected ERROR_INVALID_HANDLE or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    gbl = GlobalAlloc(GMEM_DDESHARE, 100);

    res = GlobalUnlock(gbl);
    ok(res == 1 ||
       res == 0, /* win9x */
       "Expected 1 or 0, got %d\n", res);

    res = GlobalUnlock(gbl);
    ok(res == 1 ||
       res == 0, /* win9x */
       "Expected 1 or 0, got %d\n", res);

    /* GlobalSize on an invalid handle */
    if (sizeof(void *) != 8)  /* crashes on 64-bit Vista */
    {
        SetLastError(MAGIC_DEAD);
        size = GlobalSize((HGLOBAL)0xc042);
        ok(size == 0, "Expected 0, got %ld\n", size);
        ok(GetLastError() == ERROR_INVALID_HANDLE ||
           GetLastError() == ERROR_INVALID_PARAMETER, /* win9x */
           "Expected ERROR_INVALID_HANDLE or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    /* ####################################### */
    /* Local*() functions */
    gbl = LocalAlloc(LMEM_MOVEABLE, 0);
    ok(gbl != NULL, "local memory not allocated for size 0\n");

    gbl = LocalReAlloc(gbl, 10, LMEM_MOVEABLE);
    ok(gbl != NULL, "Can't realloc local memory\n");
    size = LocalSize(gbl);
    ok(size >= 10 && size <= 16, "Memory not resized to size 10, instead size=%ld\n", size);

    gbl = LocalReAlloc(gbl, 0, LMEM_MOVEABLE);
    ok(gbl != NULL, "LocalReAlloc should not fail on size 0\n");

    size = LocalSize(gbl);
    ok(size == 0, "Memory not resized to size 0, instead size=%ld\n", size);
    ok(LocalFree(gbl) == NULL, "Memory not freed\n");
    size = LocalSize(gbl);
    ok(size == 0, "Memory should have been freed, size=%ld\n", size);

    gbl = LocalReAlloc(0, 10, LMEM_MOVEABLE);
    ok(gbl == NULL, "local realloc allocated memory\n");

    /* LocalLock / LocalUnlock with a valid handle */
    gbl = LocalAlloc(LMEM_MOVEABLE, 256);
    SetLastError(MAGIC_DEAD);
    mem = LocalLock(gbl);      /* #1 */
    ok(mem != NULL, "returned %p with %d (expected '!= NULL')\n", mem, GetLastError());
    SetLastError(MAGIC_DEAD);
    flags = LocalFlags(gbl);
    ok( flags == 1, "returned 0x%04x with %d (expected '0x0001')\n",
        flags, GetLastError());

    SetLastError(MAGIC_DEAD);
    msecond = LocalLock(gbl);   /* #2 */
    ok( msecond == mem, "returned %p with %d (expected '%p')\n",
        msecond, GetLastError(), mem);
    SetLastError(MAGIC_DEAD);
    flags = LocalFlags(gbl);
    ok( flags == 2, "returned 0x%04x with %d (expected '0x0002')\n",
        flags, GetLastError());
    SetLastError(MAGIC_DEAD);

    SetLastError(MAGIC_DEAD);
    res = LocalUnlock(gbl);    /* #1 */
    ok(res, "returned %d with %d (expected '!= 0')\n", res, GetLastError());
    SetLastError(MAGIC_DEAD);
    flags = LocalFlags(gbl);
    ok( flags , "returned 0x%04x with %d (expected '!= 0')\n",
        flags, GetLastError());

    SetLastError(MAGIC_DEAD);
    res = LocalUnlock(gbl);    /* #0 */
    /* NT: ERROR_SUCCESS (documented on MSDN), 9x: untouched */
    ok(!res && ((GetLastError() == ERROR_SUCCESS) || (GetLastError() == MAGIC_DEAD)),
        "returned %d with %d (expected '0' with: ERROR_SUCCESS or "
        "MAGIC_DEAD)\n", res, GetLastError());
    SetLastError(MAGIC_DEAD);
    flags = LocalFlags(gbl);
    ok( !flags , "returned 0x%04x with %d (expected '0')\n",
        flags, GetLastError());

    /* Unlock an already unlocked Handle */
    SetLastError(MAGIC_DEAD);
    res = LocalUnlock(gbl);
    /* NT: ERROR_NOT_LOCKED,  9x: untouched */
    ok( !res &&
        ((GetLastError() == ERROR_NOT_LOCKED) || (GetLastError() == MAGIC_DEAD)),
        "returned %d with %d (expected '0' with: ERROR_NOT_LOCKED or "
        "MAGIC_DEAD)\n", res, GetLastError());

    LocalFree(gbl);
    /* invalid handles are caught in windows: */
    SetLastError(MAGIC_DEAD);
    hsecond = LocalFree(gbl);       /* invalid handle: free memory twice */
    ok( (hsecond == gbl) && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned %p with 0x%08x (expected %p with ERROR_INVALID_HANDLE)\n",
        hsecond, GetLastError(), gbl);
    SetLastError(MAGIC_DEAD);
    flags = LocalFlags(gbl);
    ok( (flags == LMEM_INVALID_HANDLE) && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned 0x%04x with 0x%08x (expected LMEM_INVALID_HANDLE with "
        "ERROR_INVALID_HANDLE)\n", flags, GetLastError());
    SetLastError(MAGIC_DEAD);
    size = LocalSize(gbl);
    ok( (size == 0) && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned %ld with 0x%08x (expected '0' with ERROR_INVALID_HANDLE)\n",
        size, GetLastError());

    SetLastError(MAGIC_DEAD);
    mem = LocalLock(gbl);
    ok( (mem == NULL) && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned %p with 0x%08x (expected NULL with ERROR_INVALID_HANDLE)\n",
        mem, GetLastError());

    /* This Test works the same on all Systems (GlobalUnlock() is different) */
    SetLastError(MAGIC_DEAD);
    res = LocalUnlock(gbl);
    ok(!res && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned %d with %d (expected '0' with ERROR_INVALID_HANDLE)\n",
        res, GetLastError());

    /* trying to lock empty memory should give an error */
    gbl = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,0);
    ok(gbl != NULL, "returned NULL\n");
    SetLastError(MAGIC_DEAD);
    mem = GlobalLock(gbl);
    /* NT: ERROR_DISCARDED,  9x: untouched */
    ok( (mem == NULL) &&
        ((GetLastError() == ERROR_DISCARDED) || (GetLastError() == MAGIC_DEAD)),
        "returned %p with 0x%x/%d (expected 'NULL' with: ERROR_DISCARDED or "
        "MAGIC_DEAD)\n", mem, GetLastError(), GetLastError());

    GlobalFree(gbl);
}

static void test_obsolete_flags(void)
{
    static struct {
        UINT flags;
        UINT globalflags;
    } test_global_flags[] = {
        {GMEM_FIXED | GMEM_NOTIFY, 0},
        {GMEM_FIXED | GMEM_DISCARDABLE, 0},
        {GMEM_MOVEABLE | GMEM_NOTIFY, 0},
        {GMEM_MOVEABLE | GMEM_DDESHARE, GMEM_DDESHARE},
        {GMEM_MOVEABLE | GMEM_NOT_BANKED, 0},
        {GMEM_MOVEABLE | GMEM_NODISCARD, 0},
        {GMEM_MOVEABLE | GMEM_DISCARDABLE, GMEM_DISCARDABLE},
        {GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_DISCARDABLE | GMEM_LOWER | GMEM_NOCOMPACT | GMEM_NODISCARD |
         GMEM_NOT_BANKED | GMEM_NOTIFY, GMEM_DDESHARE | GMEM_DISCARDABLE},
    };

    unsigned int i;
    HGLOBAL gbl;
    UINT resultflags;

    UINT (WINAPI *pGlobalFlags)(HGLOBAL);

    pGlobalFlags = (void *) GetProcAddress(GetModuleHandleA("kernel32"), "GlobalFlags");

    if (!pGlobalFlags)
    {
        win_skip("GlobalFlags is not available\n");
        return;
    }

    for (i = 0; i < sizeof(test_global_flags)/sizeof(test_global_flags[0]); i++)
    {
        gbl = GlobalAlloc(test_global_flags[i].flags, 4);
        ok(gbl != NULL, "GlobalAlloc failed\n");

        SetLastError(MAGIC_DEAD);
        resultflags = pGlobalFlags(gbl);

        ok( resultflags == test_global_flags[i].globalflags ||
            broken(resultflags == (test_global_flags[i].globalflags & ~GMEM_DDESHARE)), /* win9x */
            "%u: expected 0x%08x, but returned 0x%08x with %d\n",
            i, test_global_flags[i].globalflags, resultflags, GetLastError() );

        GlobalFree(gbl);
    }
}

START_TEST(heap)
{
    test_heap();
    test_obsolete_flags();

    /* Test both short and very long blocks */
    test_sized_HeapAlloc(1);
    test_sized_HeapAlloc(1 << 20);
    test_sized_HeapReAlloc(1, 100);
    test_sized_HeapReAlloc(1, (1 << 20));
    test_sized_HeapReAlloc((1 << 20), (2 << 20));
    test_sized_HeapReAlloc((1 << 20), 1);
}
