/*
 * Unit test suite for heap functions
 *
 * Copyright 2002 Geoffrey Hausheer
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wine/winternl.h"
#include "wine/test.h"

#define MAGIC_DEAD 0xdeadbeef

/* some undocumented flags (names are made up) */
#define HEAP_PAGE_ALLOCS      0x01000000
#define HEAP_VALIDATE         0x10000000
#define HEAP_VALIDATE_ALL     0x20000000
#define HEAP_VALIDATE_PARAMS  0x40000000

static BOOL (WINAPI *pHeapQueryInformation)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T, PSIZE_T);
static ULONG (WINAPI *pRtlGetNtGlobalFlags)(void);

struct heap_layout
{
    DWORD_PTR unknown[2];
    DWORD pattern;
    DWORD flags;
    DWORD force_flags;
};

static SIZE_T resize_9x(SIZE_T size)
{
    DWORD dwSizeAligned = (size + 3) & ~3;
    return max(dwSizeAligned, 12); /* at least 12 bytes */
}

static void test_sized_HeapAlloc(int nbytes)
{
    BOOL success;
    char *buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nbytes);
    ok(buf != NULL, "allocate failed\n");
    ok(buf[0] == 0, "buffer not zeroed\n");
    success = HeapFree(GetProcessHeap(), 0, buf);
    ok(success, "free failed\n");
}

static void test_sized_HeapReAlloc(int nbytes1, int nbytes2)
{
    BOOL success;
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
    SIZE_T  size, size2;
    const SIZE_T max_size = 1024, init_size = 10;

    /* Heap*() functions */
    mem = HeapAlloc(GetProcessHeap(), 0, 0);
    ok(mem != NULL, "memory not allocated for size 0\n");
    HeapFree(GetProcessHeap(), 0, mem);

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

    /* GMEM_FIXED block expands in place only without flags */
    for (size = 1; size <= max_size; size <<= 1) {
        gbl = GlobalAlloc(GMEM_FIXED, init_size);
        SetLastError(MAGIC_DEAD);
        hsecond = GlobalReAlloc(gbl, size + init_size, 0);
        ok(hsecond == gbl || (hsecond == NULL && GetLastError() == ERROR_NOT_ENOUGH_MEMORY),
           "got %p with %x (expected %p or NULL) @%ld\n", hsecond, GetLastError(), gbl, size);
        GlobalFree(gbl);
    }

    /* GMEM_FIXED block can be relocated with GMEM_MOVEABLE */
    for (size = 1; size <= max_size; size <<= 1) {
        gbl = GlobalAlloc(GMEM_FIXED, init_size);
        SetLastError(MAGIC_DEAD);
        hsecond = GlobalReAlloc(gbl, size + init_size, GMEM_MOVEABLE);
        ok(hsecond != NULL,
           "got %p with %x (expected non-NULL) @%ld\n", hsecond, GetLastError(), size);
        mem = GlobalLock(hsecond);
        ok(mem == hsecond, "got %p (expected %p) @%ld\n", mem, hsecond, size);
        GlobalFree(hsecond);
    }

    gbl = GlobalAlloc(GMEM_DDESHARE, 100);

    res = GlobalUnlock(gbl);
    ok(res == 1 ||
       broken(res == 0), /* win9x */
       "Expected 1 or 0, got %d\n", res);

    res = GlobalUnlock(gbl);
    ok(res == 1 ||
       broken(res == 0), /* win9x */
       "Expected 1 or 0, got %d\n", res);

    GlobalFree(gbl);

    gbl = GlobalAlloc(GMEM_FIXED, 100);

    SetLastError(0xdeadbeef);
    res = GlobalUnlock(gbl);
    ok(res == 1 ||
       broken(res == 0), /* win9x */
       "Expected 1 or 0, got %d\n", res);
    ok(GetLastError() == 0xdeadbeef, "got %d\n", GetLastError());

    GlobalFree(gbl);

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

    /* LMEM_FIXED block expands in place only without flags */
    for (size = 1; size <= max_size; size <<= 1) {
        gbl = LocalAlloc(LMEM_FIXED, init_size);
        SetLastError(MAGIC_DEAD);
        hsecond = LocalReAlloc(gbl, size + init_size, 0);
        ok(hsecond == gbl || (hsecond == NULL && GetLastError() == ERROR_NOT_ENOUGH_MEMORY),
           "got %p with %x (expected %p or NULL) @%ld\n", hsecond, GetLastError(), gbl, size);
        LocalFree(gbl);
    }

    /* LMEM_FIXED memory can be relocated with LMEM_MOVEABLE */
    for (size = 1; size <= max_size; size <<= 1) {
        gbl = LocalAlloc(LMEM_FIXED, init_size);
        SetLastError(MAGIC_DEAD);
        hsecond = LocalReAlloc(gbl, size + init_size, LMEM_MOVEABLE);
        ok(hsecond != NULL,
           "got %p with %x (expected non-NULL) @%ld\n", hsecond, GetLastError(), size);
        mem = LocalLock(hsecond);
        ok(mem == hsecond, "got %p (expected %p) @%ld\n", mem, hsecond, size);
        LocalFree(hsecond);
    }

    /* trying to unlock pointer from LocalAlloc */
    gbl = LocalAlloc(LMEM_FIXED, 100);
    SetLastError(0xdeadbeef);
    res = LocalUnlock(gbl);
    ok(res == 0, "Expected 0, got %d\n", res);
    ok(GetLastError() == ERROR_NOT_LOCKED ||
       broken(GetLastError() == 0xdeadbeef) /* win9x */, "got %d\n", GetLastError());
    LocalFree(gbl);

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

    /* trying to get size from data pointer (GMEM_MOVEABLE) */
    gbl = GlobalAlloc(GMEM_MOVEABLE, 0x123);
    ok(gbl != NULL, "returned NULL\n");
    mem = GlobalLock(gbl);
    ok(mem != NULL, "returned NULL.\n");
    ok(gbl != mem, "unexpectedly equal.\n");

    size = GlobalSize(gbl);
    size2 = GlobalSize(mem);
    ok(size == 0x123, "got %lu\n", size);
    ok(size2 == 0x123, "got %lu\n", size2);

    GlobalFree(gbl);

    /* trying to get size from data pointer (GMEM_FIXED) */
    gbl = GlobalAlloc(GMEM_FIXED, 0x123);
    ok(gbl != NULL, "returned NULL\n");
    mem = GlobalLock(gbl);
    ok(mem != NULL, "returned NULL.\n");
    ok(gbl == mem, "got %p, %p.\n", gbl, mem);

    size = GlobalSize(gbl);
    ok(size == 0x123, "got %lu\n", size);

    GlobalFree(gbl);

    size = GlobalSize((void *)0xdeadbee0);
    ok(size == 0, "got %lu\n", size);

}


static void test_HeapCreate(void)
{
    SYSTEM_INFO sysInfo;
    ULONG memchunk;
    HANDLE heap;
    LPVOID mem1,mem1a,mem3;
    UCHAR *mem2,*mem2a;
    UINT i;
    BOOL error;
    DWORD dwSize;

    /* Retrieve the page size for this system */
    GetSystemInfo(&sysInfo);
    ok(sysInfo.dwPageSize>0,"GetSystemInfo should return a valid page size\n");

    /* Create a Heap with a minimum and maximum size */
    /* Note that Windows and Wine seem to behave a bit differently with respect
       to memory allocation.  In Windows, you can't access all the memory
       specified in the heap (due to overhead), so choosing a reasonable maximum
       size for the heap was done mostly by trial-and-error on Win2k.  It may need
       more tweaking for otherWindows variants.
    */
    memchunk=10*sysInfo.dwPageSize;
    heap=HeapCreate(0,2*memchunk,5*memchunk);
    ok( !((ULONG_PTR)heap & 0xffff), "heap %p not 64K aligned\n", heap );

    /* Check that HeapCreate allocated the right amount of ram */
    mem1=HeapAlloc(heap,0,5*memchunk+1);
    ok(mem1==NULL,"HeapCreate allocated more Ram than it should have\n");
    HeapFree(heap,0,mem1);

    /* Check that a normal alloc works */
    mem1=HeapAlloc(heap,0,memchunk);
    ok(mem1!=NULL,"HeapAlloc failed\n");
    if(mem1) {
      ok(HeapSize(heap,0,mem1)>=memchunk, "HeapAlloc should return a big enough memory block\n");
    }

    /* Check that a 'zeroing' alloc works */
    mem2=HeapAlloc(heap,HEAP_ZERO_MEMORY,memchunk);
    ok(mem2!=NULL,"HeapAlloc failed\n");
    if(mem2) {
      ok(HeapSize(heap,0,mem2)>=memchunk,"HeapAlloc should return a big enough memory block\n");
      error=FALSE;
      for(i=0;i<memchunk;i++) {
        if(mem2[i]!=0) {
          error=TRUE;
        }
      }
      ok(!error,"HeapAlloc should have zeroed out its allocated memory\n");
    }

    /* Check that HeapAlloc returns NULL when requested way too much memory */
    mem3=HeapAlloc(heap,0,5*memchunk);
    ok(mem3==NULL,"HeapAlloc should return NULL\n");
    if(mem3) {
      ok(HeapFree(heap,0,mem3),"HeapFree didn't pass successfully\n");
    }

    /* Check that HeapRealloc works */
    mem2a=HeapReAlloc(heap,HEAP_ZERO_MEMORY,mem2,memchunk+5*sysInfo.dwPageSize);
    ok(mem2a!=NULL,"HeapReAlloc failed\n");
    if(mem2a) {
      ok(HeapSize(heap,0,mem2a)>=memchunk+5*sysInfo.dwPageSize,"HeapReAlloc failed\n");
      error=FALSE;
      for(i=0;i<5*sysInfo.dwPageSize;i++) {
        if(mem2a[memchunk+i]!=0) {
          error=TRUE;
        }
      }
      ok(!error,"HeapReAlloc should have zeroed out its allocated memory\n");
    }

    /* Check that HeapRealloc honours HEAP_REALLOC_IN_PLACE_ONLY */
    error=FALSE;
    mem1a=HeapReAlloc(heap,HEAP_REALLOC_IN_PLACE_ONLY,mem1,memchunk+sysInfo.dwPageSize);
    if(mem1a!=NULL) {
      if(mem1a!=mem1) {
        error=TRUE;
      }
    }
    ok(mem1a==NULL || !error,"HeapReAlloc didn't honour HEAP_REALLOC_IN_PLACE_ONLY\n");

    /* Check that HeapFree works correctly */
   if(mem1a) {
     ok(HeapFree(heap,0,mem1a),"HeapFree failed\n");
   } else {
     ok(HeapFree(heap,0,mem1),"HeapFree failed\n");
   }
   if(mem2a) {
     ok(HeapFree(heap,0,mem2a),"HeapFree failed\n");
   } else {
     ok(HeapFree(heap,0,mem2),"HeapFree failed\n");
   }

   /* 0-length buffer */
   mem1 = HeapAlloc(heap, 0, 0);
   ok(mem1 != NULL, "Reserved memory\n");

   dwSize = HeapSize(heap, 0, mem1);
   /* should work with 0-length buffer */
   ok(dwSize < 0xFFFFFFFF, "The size of the 0-length buffer\n");
   ok(HeapFree(heap, 0, mem1), "Freed the 0-length buffer\n");

   /* Check that HeapDestroy works */
   ok(HeapDestroy(heap),"HeapDestroy failed\n");
}


static void test_GlobalAlloc(void)
{
    ULONG memchunk;
    HGLOBAL mem1,mem2,mem2a,mem2b;
    UCHAR *mem2ptr;
    UINT i;
    BOOL error;
    memchunk=100000;

    SetLastError(NO_ERROR);
    /* Check that a normal alloc works */
    mem1=GlobalAlloc(0,memchunk);
    ok(mem1!=NULL,"GlobalAlloc failed\n");
    if(mem1) {
      ok(GlobalSize(mem1)>=memchunk, "GlobalAlloc should return a big enough memory block\n");
    }

    /* Check that a 'zeroing' alloc works */
    mem2=GlobalAlloc(GMEM_ZEROINIT,memchunk);
    ok(mem2!=NULL,"GlobalAlloc failed: error=%d\n",GetLastError());
    if(mem2) {
      ok(GlobalSize(mem2)>=memchunk,"GlobalAlloc should return a big enough memory block\n");
      mem2ptr=GlobalLock(mem2);
      ok(mem2ptr==mem2,"GlobalLock should have returned the same memory as was allocated\n");
      if(mem2ptr) {
        error=FALSE;
        for(i=0;i<memchunk;i++) {
          if(mem2ptr[i]!=0) {
            error=TRUE;
          }
        }
        ok(!error,"GlobalAlloc should have zeroed out its allocated memory\n");
      }
   }
    /* Check that GlobalReAlloc works */
    /* Check that we can change GMEM_FIXED to GMEM_MOVEABLE */
    mem2a=GlobalReAlloc(mem2,0,GMEM_MODIFY | GMEM_MOVEABLE);
    if(mem2a!=NULL) {
      mem2=mem2a;
      mem2ptr=GlobalLock(mem2a);
      ok(mem2ptr!=NULL && !GlobalUnlock(mem2a)&&GetLastError()==NO_ERROR,
         "Converting from FIXED to MOVEABLE didn't REALLY work\n");
    }

    /* Check that ReAllocing memory works as expected */
    mem2a=GlobalReAlloc(mem2,2*memchunk,GMEM_MOVEABLE | GMEM_ZEROINIT);
    ok(mem2a!=NULL,"GlobalReAlloc failed\n");
    if(mem2a) {
      ok(GlobalSize(mem2a)>=2*memchunk,"GlobalReAlloc failed\n");
      mem2ptr=GlobalLock(mem2a);
      ok(mem2ptr!=NULL,"GlobalLock Failed\n");
      if(mem2ptr) {
        error=FALSE;
        for(i=0;i<memchunk;i++) {
          if(mem2ptr[memchunk+i]!=0) {
            error=TRUE;
          }
        }
        ok(!error,"GlobalReAlloc should have zeroed out its allocated memory\n");

        /* Check that GlobalHandle works */
        mem2b=GlobalHandle(mem2ptr);
        ok(mem2b==mem2a,"GlobalHandle didn't return the correct memory handle %p/%p for %p\n",
           mem2a, mem2b, mem2ptr);
        /* Check that we can't discard locked memory */
        mem2b=GlobalDiscard(mem2a);
        if(mem2b==NULL) {
          ok(!GlobalUnlock(mem2a) && GetLastError()==NO_ERROR,"GlobalUnlock Failed\n");
        }
      }
    }
    if(mem1) {
      ok(GlobalFree(mem1)==NULL,"GlobalFree failed\n");
    }
    if(mem2a) {
      ok(GlobalFree(mem2a)==NULL,"GlobalFree failed\n");
    } else {
      ok(GlobalFree(mem2)==NULL,"GlobalFree failed\n");
    }
}


static void test_LocalAlloc(void)
{
    ULONG memchunk;
    HLOCAL mem1,mem2,mem2a,mem2b;
    UCHAR *mem2ptr;
    UINT i;
    BOOL error;
    memchunk=100000;

    /* Check that a normal alloc works */
    mem1=LocalAlloc(0,memchunk);
    ok(mem1!=NULL,"LocalAlloc failed: error=%d\n",GetLastError());
    if(mem1) {
      ok(LocalSize(mem1)>=memchunk, "LocalAlloc should return a big enough memory block\n");
    }

    /* Check that a 'zeroing' and lock alloc works */
    mem2=LocalAlloc(LMEM_ZEROINIT|LMEM_MOVEABLE,memchunk);
    ok(mem2!=NULL,"LocalAlloc failed: error=%d\n",GetLastError());
    if(mem2) {
      ok(LocalSize(mem2)>=memchunk,"LocalAlloc should return a big enough memory block\n");
      mem2ptr=LocalLock(mem2);
      ok(mem2ptr!=NULL,"LocalLock: error=%d\n",GetLastError());
      if(mem2ptr) {
        error=FALSE;
        for(i=0;i<memchunk;i++) {
          if(mem2ptr[i]!=0) {
            error=TRUE;
          }
        }
        ok(!error,"LocalAlloc should have zeroed out its allocated memory\n");
        SetLastError(0);
        error=LocalUnlock(mem2);
        ok(!error && GetLastError()==NO_ERROR,
           "LocalUnlock Failed: rc=%d err=%d\n",error,GetLastError());
      }
    }
   mem2a=LocalFree(mem2);
   ok(mem2a==NULL, "LocalFree failed: %p\n",mem2a);

   /* Reallocate mem2 as moveable memory */
   mem2=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,memchunk);
   ok(mem2!=NULL, "LocalAlloc failed to create moveable memory, error=%d\n",GetLastError());

   /* Check that ReAllocing memory works as expected */
    mem2a=LocalReAlloc(mem2,2*memchunk,LMEM_MOVEABLE | LMEM_ZEROINIT);
    ok(mem2a!=NULL,"LocalReAlloc failed, error=%d\n",GetLastError());
    if(mem2a) {
      ok(LocalSize(mem2a)>=2*memchunk,"LocalReAlloc failed\n");
      mem2ptr=LocalLock(mem2a);
      ok(mem2ptr!=NULL,"LocalLock Failed\n");
      if(mem2ptr) {
        error=FALSE;
        for(i=0;i<memchunk;i++) {
          if(mem2ptr[memchunk+i]!=0) {
            error=TRUE;
          }
        }
        ok(!error,"LocalReAlloc should have zeroed out its allocated memory\n");
        /* Check that LocalHandle works */
        mem2b=LocalHandle(mem2ptr);
        ok(mem2b==mem2a,"LocalHandle didn't return the correct memory handle %p/%p for %p\n",
           mem2a, mem2b, mem2ptr);
        /* Check that we can't discard locked memory */
        mem2b=LocalDiscard(mem2a);
        ok(mem2b==NULL,"Discarded memory we shouldn't have\n");
        SetLastError(NO_ERROR);
        ok(!LocalUnlock(mem2a) && GetLastError()==NO_ERROR, "LocalUnlock Failed\n");
      }
    }
    if(mem1) {
      ok(LocalFree(mem1)==NULL,"LocalFree failed\n");
    }
    if(mem2a) {
      ok(LocalFree(mem2a)==NULL,"LocalFree failed\n");
    } else {
      ok(LocalFree(mem2)==NULL,"LocalFree failed\n");
    }
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

static void test_HeapQueryInformation(void)
{
    ULONG info;
    SIZE_T size;
    BOOL ret;

    pHeapQueryInformation = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "HeapQueryInformation");
    if (!pHeapQueryInformation)
    {
        win_skip("HeapQueryInformation is not available\n");
        return;
    }

    if (0) /* crashes under XP */
    {
        size = 0;
        pHeapQueryInformation(0,
                                HeapCompatibilityInformation,
                                &info, sizeof(info), &size);
        size = 0;
        pHeapQueryInformation(GetProcessHeap(),
                                HeapCompatibilityInformation,
                                NULL, sizeof(info), &size);
    }

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pHeapQueryInformation(GetProcessHeap(),
                                HeapCompatibilityInformation,
                                NULL, 0, &size);
    ok(!ret, "HeapQueryInformation should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER got %u\n", GetLastError());
    ok(size == sizeof(ULONG), "expected 4, got %lu\n", size);

    SetLastError(0xdeadbeef);
    ret = pHeapQueryInformation(GetProcessHeap(),
                                HeapCompatibilityInformation,
                                NULL, 0, NULL);
    ok(!ret, "HeapQueryInformation should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER got %u\n", GetLastError());

    info = 0xdeadbeaf;
    SetLastError(0xdeadbeef);
    ret = pHeapQueryInformation(GetProcessHeap(),
                                HeapCompatibilityInformation,
                                &info, sizeof(info) + 1, NULL);
    ok(ret, "HeapQueryInformation error %u\n", GetLastError());
    ok(info == 0 || info == 1 || info == 2, "expected 0, 1 or 2, got %u\n", info);
}

static void test_heap_checks( DWORD flags )
{
    BYTE old, *p, *p2;
    BOOL ret;
    SIZE_T i, size, large_size = 3000 * 1024 + 37;

    if (flags & HEAP_PAGE_ALLOCS) return;  /* no tests for that case yet */
    trace( "testing heap flags %08x\n", flags );

    p = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 17 );
    ok( p != NULL, "HeapAlloc failed\n" );

    ret = HeapValidate( GetProcessHeap(), 0, p );
    ok( ret, "HeapValidate failed\n" );

    size = HeapSize( GetProcessHeap(), 0, p );
    ok( size == 17, "Wrong size %lu\n", size );

    ok( p[14] == 0, "wrong data %x\n", p[14] );
    ok( p[15] == 0, "wrong data %x\n", p[15] );
    ok( p[16] == 0, "wrong data %x\n", p[16] );

    if (flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        ok( p[17] == 0xab, "wrong padding %x\n", p[17] );
        ok( p[18] == 0xab, "wrong padding %x\n", p[18] );
        ok( p[19] == 0xab, "wrong padding %x\n", p[19] );
    }

    p2 = HeapReAlloc( GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, p, 14 );
    if (p2 == p)
    {
        if (flags & HEAP_TAIL_CHECKING_ENABLED)
        {
            ok( p[14] == 0xab, "wrong padding %x\n", p[14] );
            ok( p[15] == 0xab, "wrong padding %x\n", p[15] );
            ok( p[16] == 0xab, "wrong padding %x\n", p[16] );
        }
        else
        {
            ok( p[14] == 0, "wrong padding %x\n", p[14] );
            ok( p[15] == 0, "wrong padding %x\n", p[15] );
        }
    }
    else skip( "realloc in place failed\n");

    ret = HeapFree( GetProcessHeap(), 0, p );
    ok( ret, "HeapFree failed\n" );

    p = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 17 );
    ok( p != NULL, "HeapAlloc failed\n" );
    old = p[17];
    p[17] = 0xcc;

    if (flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        ret = HeapValidate( GetProcessHeap(), 0, p );
        ok( !ret, "HeapValidate succeeded\n" );

        /* other calls only check when HEAP_VALIDATE is set */
        if (flags & HEAP_VALIDATE)
        {
            size = HeapSize( GetProcessHeap(), 0, p );
            ok( size == ~(SIZE_T)0 || broken(size == ~0u), "Wrong size %lu\n", size );

            p2 = HeapReAlloc( GetProcessHeap(), 0, p, 14 );
            ok( p2 == NULL, "HeapReAlloc succeeded\n" );

            ret = HeapFree( GetProcessHeap(), 0, p );
            ok( !ret || broken(sizeof(void*) == 8), /* not caught on xp64 */
                "HeapFree succeeded\n" );
        }

        p[17] = old;
        size = HeapSize( GetProcessHeap(), 0, p );
        ok( size == 17, "Wrong size %lu\n", size );

        p2 = HeapReAlloc( GetProcessHeap(), 0, p, 14 );
        ok( p2 != NULL, "HeapReAlloc failed\n" );
        p = p2;
    }

    ret = HeapFree( GetProcessHeap(), 0, p );
    ok( ret, "HeapFree failed\n" );

    p = HeapAlloc( GetProcessHeap(), 0, 37 );
    ok( p != NULL, "HeapAlloc failed\n" );
    memset( p, 0xcc, 37 );

    ret = HeapFree( GetProcessHeap(), 0, p );
    ok( ret, "HeapFree failed\n" );

    if (flags & HEAP_FREE_CHECKING_ENABLED)
    {
        ok( p[16] == 0xee, "wrong data %x\n", p[16] );
        ok( p[17] == 0xfe, "wrong data %x\n", p[17] );
        ok( p[18] == 0xee, "wrong data %x\n", p[18] );
        ok( p[19] == 0xfe, "wrong data %x\n", p[19] );

        ret = HeapValidate( GetProcessHeap(), 0, NULL );
        ok( ret, "HeapValidate failed\n" );

        old = p[16];
        p[16] = 0xcc;
        ret = HeapValidate( GetProcessHeap(), 0, NULL );
        ok( !ret, "HeapValidate succeeded\n" );

        p[16] = old;
        ret = HeapValidate( GetProcessHeap(), 0, NULL );
        ok( ret, "HeapValidate failed\n" );
    }

    /* now test large blocks */

    p = HeapAlloc( GetProcessHeap(), 0, large_size );
    ok( p != NULL, "HeapAlloc failed\n" );

    ret = HeapValidate( GetProcessHeap(), 0, p );
    ok( ret, "HeapValidate failed\n" );

    size = HeapSize( GetProcessHeap(), 0, p );
    ok( size == large_size, "Wrong size %lu\n", size );

    ok( p[large_size - 2] == 0, "wrong data %x\n", p[large_size - 2] );
    ok( p[large_size - 1] == 0, "wrong data %x\n", p[large_size - 1] );

    if (flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        /* Windows doesn't do tail checking on large blocks */
        ok( p[large_size] == 0xab || broken(p[large_size] == 0), "wrong data %x\n", p[large_size] );
        ok( p[large_size+1] == 0xab || broken(p[large_size+1] == 0), "wrong data %x\n", p[large_size+1] );
        ok( p[large_size+2] == 0xab || broken(p[large_size+2] == 0), "wrong data %x\n", p[large_size+2] );
        if (p[large_size] == 0xab)
        {
            p[large_size] = 0xcc;
            ret = HeapValidate( GetProcessHeap(), 0, p );
            ok( !ret, "HeapValidate succeeded\n" );

            /* other calls only check when HEAP_VALIDATE is set */
            if (flags & HEAP_VALIDATE)
            {
                size = HeapSize( GetProcessHeap(), 0, p );
                ok( size == ~(SIZE_T)0, "Wrong size %lu\n", size );

                p2 = HeapReAlloc( GetProcessHeap(), 0, p, large_size - 3 );
                ok( p2 == NULL, "HeapReAlloc succeeded\n" );

                ret = HeapFree( GetProcessHeap(), 0, p );
                ok( !ret, "HeapFree succeeded\n" );
            }
            p[large_size] = 0xab;
        }
    }

    ret = HeapFree( GetProcessHeap(), 0, p );
    ok( ret, "HeapFree failed\n" );

    /* test block sizes when tail checking */
    if (flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        for (size = 0; size < 64; size++)
        {
            p = HeapAlloc( GetProcessHeap(), 0, size );
            for (i = 0; i < 32; i++) if (p[size + i] != 0xab) break;
            ok( i >= 8, "only %lu tail bytes for size %lu\n", i, size );
            HeapFree( GetProcessHeap(), 0, p );
        }
    }
}

static void test_debug_heap( const char *argv0, DWORD flags )
{
    char keyname[MAX_PATH];
    char buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA startup;
    BOOL ret;
    DWORD err;
    HKEY hkey;
    const char *basename;

    if ((basename = strrchr( argv0, '\\' ))) basename++;
    else basename = argv0;

    sprintf( keyname, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\%s",
             basename );
    if (!strcmp( keyname + strlen(keyname) - 3, ".so" )) keyname[strlen(keyname) - 3] = 0;

    err = RegCreateKeyA( HKEY_LOCAL_MACHINE, keyname, &hkey );
    if (err == ERROR_ACCESS_DENIED)
    {
        skip("Not authorized to change the image file execution options\n");
        return;
    }
    ok( !err, "failed to create '%s' error %u\n", keyname, err );
    if (err) return;

    if (flags == 0xdeadbeef)  /* magic value for unsetting it */
        RegDeleteValueA( hkey, "GlobalFlag" );
    else
        RegSetValueExA( hkey, "GlobalFlag", 0, REG_DWORD, (BYTE *)&flags, sizeof(flags) );

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);

    sprintf( buffer, "%s heap.c 0x%x", argv0, flags );
    ret = CreateProcessA( NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info );
    ok( ret, "failed to create child process error %u\n", GetLastError() );
    if (ret)
    {
        winetest_wait_child_process( info.hProcess );
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    RegDeleteValueA( hkey, "GlobalFlag" );
    RegCloseKey( hkey );
    RegDeleteKeyA( HKEY_LOCAL_MACHINE, keyname );
}

static DWORD heap_flags_from_global_flag( DWORD flag )
{
    DWORD ret = 0;

    if (flag & FLG_HEAP_ENABLE_TAIL_CHECK)
        ret |= HEAP_TAIL_CHECKING_ENABLED;
    if (flag & FLG_HEAP_ENABLE_FREE_CHECK)
        ret |= HEAP_FREE_CHECKING_ENABLED;
    if (flag & FLG_HEAP_VALIDATE_PARAMETERS)
        ret |= HEAP_VALIDATE_PARAMS | HEAP_VALIDATE | HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED;
    if (flag & FLG_HEAP_VALIDATE_ALL)
        ret |= HEAP_VALIDATE_ALL | HEAP_VALIDATE | HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED;
    if (flag & FLG_HEAP_DISABLE_COALESCING)
        ret |= HEAP_DISABLE_COALESCE_ON_FREE;
    if (flag & FLG_HEAP_PAGE_ALLOCS)
        ret |= HEAP_PAGE_ALLOCS | HEAP_GROWABLE;
    return ret;
}

static void test_child_heap( const char *arg )
{
    struct heap_layout *heap = GetProcessHeap();
    DWORD expected = strtoul( arg, 0, 16 );
    DWORD expect_heap;

    if (expected == 0xdeadbeef)  /* expected value comes from Session Manager global flags */
    {
        HKEY hkey;
        expected = 0;
        if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager", &hkey ))
        {
            char buffer[32];
            DWORD type, size = sizeof(buffer);

            if (!RegQueryValueExA( hkey, "GlobalFlag", 0, &type, (BYTE *)buffer, &size ))
            {
                if (type == REG_DWORD) expected = *(DWORD *)buffer;
                else if (type == REG_SZ) expected = strtoul( buffer, 0, 16 );
            }
            RegCloseKey( hkey );
        }
    }
    if (expected && !pRtlGetNtGlobalFlags())  /* not working on NT4 */
    {
        win_skip( "global flags not set\n" );
        return;
    }

    ok( pRtlGetNtGlobalFlags() == expected,
        "%s: got global flags %08x expected %08x\n", arg, pRtlGetNtGlobalFlags(), expected );

    expect_heap = heap_flags_from_global_flag( expected );

    if (!(heap->flags & HEAP_GROWABLE) || heap->pattern == 0xffeeffee)  /* vista layout */
    {
        ok( (heap->flags & ~HEAP_GROWABLE) == 0, "%s: got heap flags %08x\n", arg, heap->flags );
    }
    else if (heap->pattern == 0xeeeeeeee && heap->flags == 0xeeeeeeee)
    {
        ok( expected & FLG_HEAP_PAGE_ALLOCS, "%s: got heap flags 0xeeeeeeee without page alloc\n", arg );
    }
    else
    {
        ok( heap->flags == (expect_heap | HEAP_GROWABLE),
            "%s: got heap flags %08x expected %08x\n", arg, heap->flags, expect_heap );
        ok( heap->force_flags == (expect_heap & ~0x18000080),
            "%s: got heap force flags %08x expected %08x\n", arg, heap->force_flags, expect_heap );
        expect_heap = heap->flags;
    }

    test_heap_checks( expect_heap );
}

START_TEST(heap)
{
    int argc;
    char **argv;

    pRtlGetNtGlobalFlags = (void *)GetProcAddress( GetModuleHandleA("ntdll.dll"), "RtlGetNtGlobalFlags" );

    argc = winetest_get_mainargs( &argv );
    if (argc >= 3)
    {
        test_child_heap( argv[2] );
        return;
    }

    test_heap();
    test_obsolete_flags();
    test_HeapCreate();
    test_GlobalAlloc();
    test_LocalAlloc();

    /* Test both short and very long blocks */
    test_sized_HeapAlloc(1);
    test_sized_HeapAlloc(1 << 20);
    test_sized_HeapReAlloc(1, 100);
    test_sized_HeapReAlloc(1, (1 << 20));
    test_sized_HeapReAlloc((1 << 20), (2 << 20));
    test_sized_HeapReAlloc((1 << 20), 1);
    test_HeapQueryInformation();

    if (pRtlGetNtGlobalFlags)
    {
        test_debug_heap( argv[0], 0 );
        test_debug_heap( argv[0], FLG_HEAP_ENABLE_TAIL_CHECK );
        test_debug_heap( argv[0], FLG_HEAP_ENABLE_FREE_CHECK );
        test_debug_heap( argv[0], FLG_HEAP_VALIDATE_PARAMETERS );
        test_debug_heap( argv[0], FLG_HEAP_VALIDATE_ALL );
        test_debug_heap( argv[0], FLG_POOL_ENABLE_TAGGING );
        test_debug_heap( argv[0], FLG_HEAP_ENABLE_TAGGING );
        test_debug_heap( argv[0], FLG_HEAP_ENABLE_TAG_BY_DLL );
        test_debug_heap( argv[0], FLG_HEAP_DISABLE_COALESCING );
        test_debug_heap( argv[0], FLG_HEAP_PAGE_ALLOCS );
        test_debug_heap( argv[0], 0xdeadbeef );
    }
    else win_skip( "RtlGetNtGlobalFlags not found, skipping heap debug tests\n" );
}
