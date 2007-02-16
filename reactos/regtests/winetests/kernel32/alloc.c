/*
 * Unit test suite for memory allocation functions.
 *
 * Copyright 2002 Geoffrey Hausheer
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"


 /* The following functions don't have tests, because either I don't know how
   to test them, or they are WinNT only, or require multiple threads.
   Since the last two issues shouldn't really stop the tests from being
   written, assume for now that it is all due to the first case
       HeapCompact
       HeapLock
       HeapQueryInformation
       HeapSetInformation
       HeapUnlock
       HeapValidate
       HeapWalk
*/
/* In addition, these features aren't being tested
       HEAP_NO_SERIALIZE
       HEAP_GENERATE_EXCEPTIONS
       STATUS_ACCESS_VIOLATION (error code from HeapAlloc)
*/

static void test_Heap(void)
{
    SYSTEM_INFO sysInfo;
    ULONG memchunk;
    HANDLE heap;
    LPVOID mem1,mem1a,mem3;
    UCHAR *mem2,*mem2a;
    UINT error,i;
    DWORD dwSize;

/* Retrieve the page size for this system */
    sysInfo.dwPageSize=0;
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

/* Check that HeapCreate allocated the right amount of ram */
    todo_wine {
    /* Today HeapCreate seems to return a memory block larger than specified.
       MSDN says the maximum heap size should be dwMaximumSize rounded up to the
       nearest page boundary
    */
      mem1=HeapAlloc(heap,0,5*memchunk+1);
      ok(mem1==NULL,"HeapCreate allocated more Ram than it should have\n");
      HeapFree(heap,0,mem1);
    }

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
      error=0;
      for(i=0;i<memchunk;i++) {
        if(mem2[i]!=0) {
          error=1;
        }
      }
      ok(!error,"HeapAlloc should have zeroed out it's allocated memory\n");
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
      error=0;
      for(i=0;i<5*sysInfo.dwPageSize;i++) {
        if(mem2a[memchunk+i]!=0) {
          error=1;
        }
      }
      ok(!error,"HeapReAlloc should have zeroed out it's allocated memory\n");
    }

/* Check that HeapRealloc honours HEAP_REALLOC_IN_PLACE_ONLY */
    error=0;
    mem1a=HeapReAlloc(heap,HEAP_REALLOC_IN_PLACE_ONLY,mem1,memchunk+sysInfo.dwPageSize);
    if(mem1a!=NULL) {
      if(mem1a!=mem1) {
        error=1;
      }
    }
    ok(mem1a==NULL || error==0,"HeapReAlloc didn't honour HEAP_REALLOC_IN_PLACE_ONLY\n");

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
   ok((dwSize >= 0) && (dwSize < 0xFFFFFFFF),
      "The size of the 0-length buffer\n");
   ok(HeapFree(heap, 0, mem1), "Freed the 0-length buffer\n");

/* Check that HeapDestry works */
   ok(HeapDestroy(heap),"HeapDestroy failed\n");
}

/* The following functions don't have tests, because either I don't know how
   to test them, or they are WinNT only, or require multiple threads.
   Since the last two issues shouldn't really stop the tests from being
   written, assume for now that it is all due to the first case
       GlobalFlags
       GlobalMemoryStatus
       GlobalMemoryStatusEx
*/
/* In addition, these features aren't being tested
       GMEM_DISCADABLE
       GMEM_NOCOMPACT
*/
static void test_Global(void)
{
    ULONG memchunk;
    HGLOBAL mem1,mem2,mem2a,mem2b;
    UCHAR *mem2ptr;
    UINT error,i;
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
    ok(mem2!=NULL,"GlobalAlloc failed: error=%ld\n",GetLastError());
    if(mem2) {
      ok(GlobalSize(mem2)>=memchunk,"GlobalAlloc should return a big enough memory block\n");
      mem2ptr=GlobalLock(mem2);
      ok(mem2ptr==mem2,"GlobalLock should have returned the same memory as was allocated\n");
      if(mem2ptr) {
        error=0;
        for(i=0;i<memchunk;i++) {
          if(mem2ptr[i]!=0) {
            error=1;
          }
        }
        ok(!error,"GlobalAlloc should have zeroed out it's allocated memory\n");
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
        error=0;
        for(i=0;i<memchunk;i++) {
          if(mem2ptr[memchunk+i]!=0) {
            error=1;
          }
        }
        ok(!error,"GlobalReAlloc should have zeroed out it's allocated memory\n");

/* Check that GlobalHandle works */
        mem2b=GlobalHandle(mem2ptr);
        ok(mem2b==mem2a,"GlobalHandle didn't return the correct memory handle\n");

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


/* The following functions don't have tests, because either I don't know how
   to test them, or they are WinNT only, or require multiple threads.
   Since the last two issues shouldn't really stop the tests from being
   written, assume for now that it is all due to the first case
       LocalDiscard
       LocalFlags
*/
/* In addition, these features aren't being tested
       LMEM_DISCADABLE
       LMEM_NOCOMPACT
*/
static void test_Local(void)
{
    ULONG memchunk;
    HLOCAL mem1,mem2,mem2a,mem2b;
    UCHAR *mem2ptr;
    UINT error,i;
    memchunk=100000;

/* Check that a normal alloc works */
    mem1=LocalAlloc(0,memchunk);
    ok(mem1!=NULL,"LocalAlloc failed: error=%ld\n",GetLastError());
    if(mem1) {
      ok(LocalSize(mem1)>=memchunk, "LocalAlloc should return a big enough memory block\n");
    }

/* Check that a 'zeroing' and lock alloc works */
    mem2=LocalAlloc(LMEM_ZEROINIT|LMEM_MOVEABLE,memchunk);
    ok(mem2!=NULL,"LocalAlloc failed: error=%ld\n",GetLastError());
    if(mem2) {
      ok(LocalSize(mem2)>=memchunk,"LocalAlloc should return a big enough memory block\n");
      mem2ptr=LocalLock(mem2);
      ok(mem2ptr!=NULL,"LocalLock: error=%ld\n",GetLastError());
      if(mem2ptr) {
        error=0;
        for(i=0;i<memchunk;i++) {
          if(mem2ptr[i]!=0) {
            error=1;
          }
        }
        ok(!error,"LocalAlloc should have zeroed out it's allocated memory\n");
        SetLastError(0);
        error=LocalUnlock(mem2);
        ok(error==0 && GetLastError()==NO_ERROR,
           "LocalUnlock Failed: rc=%d err=%ld\n",error,GetLastError());
      }
    }
   mem2a=LocalFree(mem2);
   ok(mem2a==NULL, "LocalFree failed: %p\n",mem2a);

/* Reallocate mem2 as moveable memory */
   mem2=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,memchunk);
   ok(mem2!=NULL, "LocalAlloc failed to create moveable memory, error=%ld\n",GetLastError());

/* Check that ReAllocing memory works as expected */
    mem2a=LocalReAlloc(mem2,2*memchunk,LMEM_MOVEABLE | LMEM_ZEROINIT);
    ok(mem2a!=NULL,"LocalReAlloc failed, error=%ld\n",GetLastError());
    if(mem2a) {
      ok(LocalSize(mem2a)>=2*memchunk,"LocalReAlloc failed\n");
      mem2ptr=LocalLock(mem2a);
      ok(mem2ptr!=NULL,"LocalLock Failed\n");
      if(mem2ptr) {
        error=0;
        for(i=0;i<memchunk;i++) {
          if(mem2ptr[memchunk+i]!=0) {
            error=1;
          }
        }
        ok(!error,"LocalReAlloc should have zeroed out it's allocated memory\n");
/* Check that LocalHandle works */
        mem2b=LocalHandle(mem2ptr);
        ok(mem2b==mem2a,"LocalHandle didn't return the correct memory handle\n");
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

/* The Virtual* routines are not tested as thoroughly,
   since I don't really understand how to use them correctly :)
   The following routines are not tested at all
      VirtualAllocEx
      VirtualFreeEx
      VirtualLock
      VirtualProtect
      VirtualProtectEx
      VirtualQuery
      VirtualQueryEx
      VirtualUnlock
    And the only features (flags) being tested are
      MEM_COMMIT
      MEM_RELEASE
      PAGE_READWRITE
    Testing the rest requires using exceptions, which I really don't
    understand well
*/
static void test_Virtual(void)
{
    SYSTEM_INFO sysInfo;
    ULONG memchunk;
    UCHAR *mem1;
    UINT error,i;

/* Retrieve the page size for this system */
    sysInfo.dwPageSize=0;
    GetSystemInfo(&sysInfo);
    ok(sysInfo.dwPageSize>0,"GetSystemInfo should return a valid page size\n");

/* Choose a reasonable allocation size */
    memchunk=10*sysInfo.dwPageSize;

/* Check that a normal alloc works */
    mem1=VirtualAlloc(NULL,memchunk,MEM_COMMIT,PAGE_READWRITE);
    ok(mem1!=NULL,"VirtualAlloc failed\n");
    if(mem1) {
/* check that memory is initialized to 0 */
      error=0;
      for(i=0;i<memchunk;i++) {
        if(mem1[i]!=0) {
          error=1;
        }
      }
      ok(!error,"VirtualAlloc did not initialize memory to '0's\n");
/* Check that we can read/write to memory */
      error=0;
      for(i=0;i<memchunk;i+=100) {
        mem1[i]='a';
        if(mem1[i]!='a') {
          error=1;
        }
      }
      ok(!error,"Virtual memory was not writable\n");
    }
    ok(VirtualFree(mem1,0,MEM_RELEASE),"VirtualFree failed\n");
}
START_TEST(alloc)
{
    test_Heap();
    test_Global();
    test_Local();
    test_Virtual();
}
