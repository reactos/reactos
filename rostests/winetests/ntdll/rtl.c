/* Unit test suite for Rtl* API functions
 *
 * Copyright 2003 Thomas Mertes
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
 *
 * NOTES
 * We use function pointers here as there is no import library for NTDLL on
 * windows.
 */

#include <stdlib.h>

#include "ntdll_test.h"

#ifndef __WINE_WINTERNL_H

typedef struct _RTL_HANDLE
{
    struct _RTL_HANDLE * Next;
} RTL_HANDLE;

typedef struct _RTL_HANDLE_TABLE
{
    ULONG MaxHandleCount;
    ULONG HandleSize;
    ULONG Unused[2];
    PVOID NextFree;
    PVOID FirstHandle;
    PVOID ReservedMemory;
    PVOID MaxHandle;
} RTL_HANDLE_TABLE;

#endif

/* Function ptrs for ntdll calls */
static HMODULE hntdll = 0;
static SIZE_T    (WINAPI  *pRtlCompareMemory)(LPCVOID,LPCVOID,SIZE_T);
static SIZE_T    (WINAPI  *pRtlCompareMemoryUlong)(PULONG, SIZE_T, ULONG);
static NTSTATUS  (WINAPI  *pRtlDeleteTimer)(HANDLE, HANDLE, HANDLE);
static VOID      (WINAPI  *pRtlMoveMemory)(LPVOID,LPCVOID,SIZE_T);
static VOID      (WINAPI  *pRtlFillMemory)(LPVOID,SIZE_T,BYTE);
static VOID      (WINAPI  *pRtlFillMemoryUlong)(LPVOID,SIZE_T,ULONG);
static VOID      (WINAPI  *pRtlZeroMemory)(LPVOID,SIZE_T);
static ULONGLONG (WINAPIV *pRtlUlonglongByteSwap)(ULONGLONG source);
static ULONG     (WINAPI  *pRtlUniform)(PULONG);
static ULONG     (WINAPI  *pRtlRandom)(PULONG);
static BOOLEAN   (WINAPI  *pRtlAreAllAccessesGranted)(ACCESS_MASK, ACCESS_MASK);
static BOOLEAN   (WINAPI  *pRtlAreAnyAccessesGranted)(ACCESS_MASK, ACCESS_MASK);
static DWORD     (WINAPI  *pRtlComputeCrc32)(DWORD,const BYTE*,INT);
static void      (WINAPI * pRtlInitializeHandleTable)(ULONG, ULONG, RTL_HANDLE_TABLE *);
static BOOLEAN   (WINAPI * pRtlIsValidIndexHandle)(const RTL_HANDLE_TABLE *, ULONG, RTL_HANDLE **);
static NTSTATUS  (WINAPI * pRtlDestroyHandleTable)(RTL_HANDLE_TABLE *);
static RTL_HANDLE * (WINAPI * pRtlAllocateHandle)(RTL_HANDLE_TABLE *, ULONG *);
static BOOLEAN   (WINAPI * pRtlFreeHandle)(RTL_HANDLE_TABLE *, RTL_HANDLE *);
static NTSTATUS  (WINAPI *pRtlAllocateAndInitializeSid)(PSID_IDENTIFIER_AUTHORITY,BYTE,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,PSID*);
static NTSTATUS  (WINAPI *pRtlFreeSid)(PSID);
#define LEN 16
static const char* src_src = "This is a test!"; /* 16 bytes long, incl NUL */
static ULONG src_aligned_block[4];
static ULONG dest_aligned_block[32];
static const char *src = (const char*)src_aligned_block;
static char* dest = (char*)dest_aligned_block;

static void InitFunctionPtrs(void)
{
    hntdll = LoadLibraryA("ntdll.dll");
    ok(hntdll != 0, "LoadLibrary failed\n");
    if (hntdll) {
	pRtlCompareMemory = (void *)GetProcAddress(hntdll, "RtlCompareMemory");
	pRtlCompareMemoryUlong = (void *)GetProcAddress(hntdll, "RtlCompareMemoryUlong");
        pRtlDeleteTimer = (void *)GetProcAddress(hntdll, "RtlDeleteTimer");
	pRtlMoveMemory = (void *)GetProcAddress(hntdll, "RtlMoveMemory");
	pRtlFillMemory = (void *)GetProcAddress(hntdll, "RtlFillMemory");
	pRtlFillMemoryUlong = (void *)GetProcAddress(hntdll, "RtlFillMemoryUlong");
	pRtlZeroMemory = (void *)GetProcAddress(hntdll, "RtlZeroMemory");
	pRtlUlonglongByteSwap = (void *)GetProcAddress(hntdll, "RtlUlonglongByteSwap");
	pRtlUniform = (void *)GetProcAddress(hntdll, "RtlUniform");
	pRtlRandom = (void *)GetProcAddress(hntdll, "RtlRandom");
	pRtlAreAllAccessesGranted = (void *)GetProcAddress(hntdll, "RtlAreAllAccessesGranted");
	pRtlAreAnyAccessesGranted = (void *)GetProcAddress(hntdll, "RtlAreAnyAccessesGranted");
	pRtlComputeCrc32 = (void *)GetProcAddress(hntdll, "RtlComputeCrc32");
	pRtlInitializeHandleTable = (void *)GetProcAddress(hntdll, "RtlInitializeHandleTable");
	pRtlIsValidIndexHandle = (void *)GetProcAddress(hntdll, "RtlIsValidIndexHandle");
	pRtlDestroyHandleTable = (void *)GetProcAddress(hntdll, "RtlDestroyHandleTable");
	pRtlAllocateHandle = (void *)GetProcAddress(hntdll, "RtlAllocateHandle");
	pRtlFreeHandle = (void *)GetProcAddress(hntdll, "RtlFreeHandle");
        pRtlAllocateAndInitializeSid = (void *)GetProcAddress(hntdll, "RtlAllocateAndInitializeSid");
        pRtlFreeSid = (void *)GetProcAddress(hntdll, "RtlFreeSid");
    }
    strcpy((char*)src_aligned_block, src_src);
    ok(strlen(src) == 15, "Source must be 16 bytes long!\n");
}

#define COMP(str1,str2,cmplen,len) size = pRtlCompareMemory(str1, str2, cmplen); \
  ok(size == len, "Expected %ld, got %ld\n", size, (SIZE_T)len)

static void test_RtlCompareMemory(void)
{
  SIZE_T size;

  if (!pRtlCompareMemory)
    return;

  strcpy(dest, src);

  COMP(src,src,0,0);
  COMP(src,src,LEN,LEN);
  dest[0] = 'x';
  COMP(src,dest,LEN,0);
}

static void test_RtlCompareMemoryUlong(void)
{
    ULONG a[10];
    ULONG result;

    a[0]= 0x0123;
    a[1]= 0x4567;
    a[2]= 0x89ab;
    a[3]= 0xcdef;
    result = pRtlCompareMemoryUlong(a, 0, 0x0123);
    ok(result == 0, "RtlCompareMemoryUlong(%p, 0, 0x0123) returns %u, expected 0\n", a, result);
    result = pRtlCompareMemoryUlong(a, 3, 0x0123);
    ok(result == 0, "RtlCompareMemoryUlong(%p, 3, 0x0123) returns %u, expected 0\n", a, result);
    result = pRtlCompareMemoryUlong(a, 4, 0x0123);
    ok(result == 4, "RtlCompareMemoryUlong(%p, 4, 0x0123) returns %u, expected 4\n", a, result);
    result = pRtlCompareMemoryUlong(a, 5, 0x0123);
    ok(result == 4, "RtlCompareMemoryUlong(%p, 5, 0x0123) returns %u, expected 4\n", a, result);
    result = pRtlCompareMemoryUlong(a, 7, 0x0123);
    ok(result == 4, "RtlCompareMemoryUlong(%p, 7, 0x0123) returns %u, expected 4\n", a, result);
    result = pRtlCompareMemoryUlong(a, 8, 0x0123);
    ok(result == 4, "RtlCompareMemoryUlong(%p, 8, 0x0123) returns %u, expected 4\n", a, result);
    result = pRtlCompareMemoryUlong(a, 9, 0x0123);
    ok(result == 4, "RtlCompareMemoryUlong(%p, 9, 0x0123) returns %u, expected 4\n", a, result);
    result = pRtlCompareMemoryUlong(a, 4, 0x0127);
    ok(result == 0, "RtlCompareMemoryUlong(%p, 4, 0x0127) returns %u, expected 0\n", a, result);
    result = pRtlCompareMemoryUlong(a, 4, 0x7123);
    ok(result == 0, "RtlCompareMemoryUlong(%p, 4, 0x7123) returns %u, expected 0\n", a, result);
    result = pRtlCompareMemoryUlong(a, 16, 0x4567);
    ok(result == 0, "RtlCompareMemoryUlong(%p, 16, 0x4567) returns %u, expected 0\n", a, result);

    a[1]= 0x0123;
    result = pRtlCompareMemoryUlong(a, 3, 0x0123);
    ok(result == 0, "RtlCompareMemoryUlong(%p, 3, 0x0123) returns %u, expected 0\n", a, result);
    result = pRtlCompareMemoryUlong(a, 4, 0x0123);
    ok(result == 4, "RtlCompareMemoryUlong(%p, 4, 0x0123) returns %u, expected 4\n", a, result);
    result = pRtlCompareMemoryUlong(a, 5, 0x0123);
    ok(result == 4, "RtlCompareMemoryUlong(%p, 5, 0x0123) returns %u, expected 4\n", a, result);
    result = pRtlCompareMemoryUlong(a, 7, 0x0123);
    ok(result == 4, "RtlCompareMemoryUlong(%p, 7, 0x0123) returns %u, expected 4\n", a, result);
    result = pRtlCompareMemoryUlong(a, 8, 0x0123);
    ok(result == 8, "RtlCompareMemoryUlong(%p, 8, 0x0123) returns %u, expected 8\n", a, result);
    result = pRtlCompareMemoryUlong(a, 9, 0x0123);
    ok(result == 8, "RtlCompareMemoryUlong(%p, 9, 0x0123) returns %u, expected 8\n", a, result);
}

#define COPY(len) memset(dest,0,sizeof(dest_aligned_block)); pRtlMoveMemory(dest, src, len)
#define CMP(str) ok(strcmp(dest,str) == 0, "Expected '%s', got '%s'\n", str, dest)

static void test_RtlMoveMemory(void)
{
  if (!pRtlMoveMemory)
    return;

  /* Length should be in bytes and not rounded. Use strcmp to ensure we
   * didn't write past the end (it checks for the final NUL left by memset)
   */
  COPY(0); CMP("");
  COPY(1); CMP("T");
  COPY(2); CMP("Th");
  COPY(3); CMP("Thi");
  COPY(4); CMP("This");
  COPY(5); CMP("This ");
  COPY(6); CMP("This i");
  COPY(7); CMP("This is");
  COPY(8); CMP("This is ");
  COPY(9); CMP("This is a");

  /* Overlapping */
  strcpy(dest, src); pRtlMoveMemory(dest, dest + 1, strlen(src) - 1);
  CMP("his is a test!!");
  strcpy(dest, src); pRtlMoveMemory(dest + 1, dest, strlen(src));
  CMP("TThis is a test!");
}

#define FILL(len) memset(dest,0,sizeof(dest_aligned_block)); strcpy(dest, src); pRtlFillMemory(dest,len,'x')

static void test_RtlFillMemory(void)
{
  if (!pRtlFillMemory)
    return;

  /* Length should be in bytes and not rounded. Use strcmp to ensure we
   * didn't write past the end (the remainder of the string should match)
   */
  FILL(0); CMP("This is a test!");
  FILL(1); CMP("xhis is a test!");
  FILL(2); CMP("xxis is a test!");
  FILL(3); CMP("xxxs is a test!");
  FILL(4); CMP("xxxx is a test!");
  FILL(5); CMP("xxxxxis a test!");
  FILL(6); CMP("xxxxxxs a test!");
  FILL(7); CMP("xxxxxxx a test!");
  FILL(8); CMP("xxxxxxxxa test!");
  FILL(9); CMP("xxxxxxxxx test!");
}

#define LFILL(len) memset(dest,0,sizeof(dest_aligned_block)); strcpy(dest, src); pRtlFillMemoryUlong(dest,len,val)

static void test_RtlFillMemoryUlong(void)
{
  ULONG val = ('x' << 24) | ('x' << 16) | ('x' << 8) | 'x';
  if (!pRtlFillMemoryUlong)
    return;

  /* Length should be in bytes and not rounded. Use strcmp to ensure we
   * didn't write past the end (the remainder of the string should match)
   */
  LFILL(0); CMP("This is a test!");
  LFILL(1); CMP("This is a test!");
  LFILL(2); CMP("This is a test!");
  LFILL(3); CMP("This is a test!");
  LFILL(4); CMP("xxxx is a test!");
  LFILL(5); CMP("xxxx is a test!");
  LFILL(6); CMP("xxxx is a test!");
  LFILL(7); CMP("xxxx is a test!");
  LFILL(8); CMP("xxxxxxxxa test!");
  LFILL(9); CMP("xxxxxxxxa test!");
}

#define ZERO(len) memset(dest,0,sizeof(dest_aligned_block)); strcpy(dest, src); pRtlZeroMemory(dest,len)
#define MCMP(str) ok(memcmp(dest,str,LEN) == 0, "Memcmp failed\n")

static void test_RtlZeroMemory(void)
{
  if (!pRtlZeroMemory)
    return;

  /* Length should be in bytes and not rounded. */
  ZERO(0); MCMP("This is a test!");
  ZERO(1); MCMP("\0his is a test!");
  ZERO(2); MCMP("\0\0is is a test!");
  ZERO(3); MCMP("\0\0\0s is a test!");
  ZERO(4); MCMP("\0\0\0\0 is a test!");
  ZERO(5); MCMP("\0\0\0\0\0is a test!");
  ZERO(6); MCMP("\0\0\0\0\0\0s a test!");
  ZERO(7); MCMP("\0\0\0\0\0\0\0 a test!");
  ZERO(8); MCMP("\0\0\0\0\0\0\0\0a test!");
  ZERO(9); MCMP("\0\0\0\0\0\0\0\0\0 test!");
}

static void test_RtlUlonglongByteSwap(void)
{
    ULONGLONG result;

    if ( pRtlUlonglongByteSwap( 0 ) != 0 )
    {
        win_skip("Broken RtlUlonglongByteSwap in win2k\n");
        return;
    }

    result = pRtlUlonglongByteSwap( ((ULONGLONG)0x76543210 << 32) | 0x87654321 );
    ok( (((ULONGLONG)0x21436587 << 32) | 0x10325476) == result,
       "RtlUlonglongByteSwap(0x7654321087654321) returns 0x%x%08x, expected 0x2143658710325476\n",
       (DWORD)(result >> 32), (DWORD)result);
}


static void test_RtlUniform(void)
{
    ULONGLONG num;
    ULONG seed;
    ULONG seed_bak;
    ULONG expected;
    ULONG result;

/*
 * According to the documentation RtlUniform is using D.H. Lehmer's 1948
 * algorithm. This algorithm is:
 *
 * seed = (seed * const_1 + const_2) % const_3;
 *
 * According to the documentation the random number is distributed over
 * [0..MAXLONG]. Therefore const_3 is MAXLONG + 1:
 *
 * seed = (seed * const_1 + const_2) % (MAXLONG + 1);
 *
 * Because MAXLONG is 0x7fffffff (and MAXLONG + 1 is 0x80000000) the
 * algorithm can be expressed without division as:
 *
 * seed = (seed * const_1 + const_2) & MAXLONG;
 *
 * To find out const_2 we just call RtlUniform with seed set to 0:
 */
    seed = 0;
    expected = 0x7fffffc3;
    result = pRtlUniform(&seed);
    ok(result == expected,
        "RtlUniform(&seed (seed == 0)) returns %x, expected %x\n",
        result, expected);
/*
 * The algorithm is now:
 *
 * seed = (seed * const_1 + 0x7fffffc3) & MAXLONG;
 *
 * To find out const_1 we can use:
 *
 * const_1 = RtlUniform(1) - 0x7fffffc3;
 *
 * If that does not work a search loop can try all possible values of
 * const_1 and compare to the result to RtlUniform(1).
 * This way we find out that const_1 is 0xffffffed.
 *
 * For seed = 1 the const_2 is 0x7fffffc4:
 */
    seed = 1;
    expected = seed * 0xffffffed + 0x7fffffc3 + 1;
    result = pRtlUniform(&seed);
    ok(result == expected,
        "RtlUniform(&seed (seed == 1)) returns %x, expected %x\n",
        result, expected);
/*
 * For seed = 2 the const_2 is 0x7fffffc3:
 */
    seed = 2;
    expected = seed * 0xffffffed + 0x7fffffc3;
    result = pRtlUniform(&seed);

/*
 * Windows Vista uses different algorithms, so skip the rest of the tests
 * until that is figured out. Trace output for the failures is about 10.5 MB!
 */

    if (result == 0x7fffff9f) {
        skip("Most likely running on Windows Vista which uses a different algorithm\n");
        return;
    }

    ok(result == expected,
        "RtlUniform(&seed (seed == 2)) returns %x, expected %x\n",
        result, expected);

/*
 * More tests show that if seed is odd the result must be incremented by 1:
 */
    seed = 3;
    expected = seed * 0xffffffed + 0x7fffffc3 + (seed & 1);
    result = pRtlUniform(&seed);
    ok(result == expected,
        "RtlUniform(&seed (seed == 3)) returns %x, expected %x\n",
        result, expected);

    seed = 0x6bca1aa;
    expected = seed * 0xffffffed + 0x7fffffc3;
    result = pRtlUniform(&seed);
    ok(result == expected,
        "RtlUniform(&seed (seed == 0x6bca1aa)) returns %x, expected %x\n",
        result, expected);

    seed = 0x6bca1ab;
    expected = seed * 0xffffffed + 0x7fffffc3 + 1;
    result = pRtlUniform(&seed);
    ok(result == expected,
        "RtlUniform(&seed (seed == 0x6bca1ab)) returns %x, expected %x\n",
        result, expected);
/*
 * When seed is 0x6bca1ac there is an exception:
 */
    seed = 0x6bca1ac;
    expected = seed * 0xffffffed + 0x7fffffc3 + 2;
    result = pRtlUniform(&seed);
    ok(result == expected,
        "RtlUniform(&seed (seed == 0x6bca1ac)) returns %x, expected %x\n",
        result, expected);
/*
 * Note that up to here const_3 is not used
 * (the highest bit of the result is not set).
 *
 * Starting with 0x6bca1ad: If seed is even the result must be incremented by 1:
 */
    seed = 0x6bca1ad;
    expected = (seed * 0xffffffed + 0x7fffffc3) & MAXLONG;
    result = pRtlUniform(&seed);
    ok(result == expected,
        "RtlUniform(&seed (seed == 0x6bca1ad)) returns %x, expected %x\n",
        result, expected);

    seed = 0x6bca1ae;
    expected = (seed * 0xffffffed + 0x7fffffc3 + 1) & MAXLONG;
    result = pRtlUniform(&seed);
    ok(result == expected,
        "RtlUniform(&seed (seed == 0x6bca1ae)) returns %x, expected %x\n",
        result, expected);
/*
 * There are several ranges where for odd or even seed the result must be
 * incremented by 1. You can see this ranges in the following test.
 *
 * For a full test use one of the following loop heads:
 *
 *  for (num = 0; num <= 0xffffffff; num++) {
 *      seed = num;
 *      ...
 *
 *  seed = 0;
 *  for (num = 0; num <= 0xffffffff; num++) {
 *      ...
 */
    seed = 0;
    for (num = 0; num <= 100000; num++) {

	expected = seed * 0xffffffed + 0x7fffffc3;
	if (seed < 0x6bca1ac) {
	    expected = expected + (seed & 1);
	} else if (seed == 0x6bca1ac) {
	    expected = (expected + 2) & MAXLONG;
	} else if (seed < 0xd79435c) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0x1435e50b) {
	    expected = expected + (seed & 1);
	} else if (seed < 0x1af286ba) { 
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0x21af2869) {
	    expected = expected + (seed & 1);
	} else if (seed < 0x286bca18) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0x2f286bc7) {
	    expected = expected + (seed & 1);
	} else if (seed < 0x35e50d77) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0x3ca1af26) {
	    expected = expected + (seed & 1);
	} else if (seed < 0x435e50d5) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0x4a1af284) {
	    expected = expected + (seed & 1);
	} else if (seed < 0x50d79433) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0x579435e2) {
	    expected = expected + (seed & 1);
	} else if (seed < 0x5e50d792) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0x650d7941) {
	    expected = expected + (seed & 1);
	} else if (seed < 0x6bca1af0) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0x7286bc9f) {
	    expected = expected + (seed & 1);
	} else if (seed < 0x79435e4e) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0x7ffffffd) {
	    expected = expected + (seed & 1);
	} else if (seed < 0x86bca1ac) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed == 0x86bca1ac) {
	    expected = (expected + 1) & MAXLONG;
	} else if (seed < 0x8d79435c) {
	    expected = expected + (seed & 1);
	} else if (seed < 0x9435e50b) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0x9af286ba) {
	    expected = expected + (seed & 1);
	} else if (seed < 0xa1af2869) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0xa86bca18) {
	    expected = expected + (seed & 1);
	} else if (seed < 0xaf286bc7) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed == 0xaf286bc7) {
	    expected = (expected + 2) & MAXLONG;
	} else if (seed < 0xb5e50d77) {
	    expected = expected + (seed & 1);
	} else if (seed < 0xbca1af26) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0xc35e50d5) {
	    expected = expected + (seed & 1);
	} else if (seed < 0xca1af284) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0xd0d79433) {
	    expected = expected + (seed & 1);
	} else if (seed < 0xd79435e2) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0xde50d792) {
	    expected = expected + (seed & 1);
	} else if (seed < 0xe50d7941) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0xebca1af0) {
	    expected = expected + (seed & 1);
	} else if (seed < 0xf286bc9f) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else if (seed < 0xf9435e4e) {
	    expected = expected + (seed & 1);
	} else if (seed < 0xfffffffd) {
	    expected = (expected + (~seed & 1)) & MAXLONG;
	} else {
	    expected = expected + (seed & 1);
	} /* if */
        seed_bak = seed;
        result = pRtlUniform(&seed);
        ok(result == expected,
                "test: 0x%x%08x RtlUniform(&seed (seed == %x)) returns %x, expected %x\n",
                (DWORD)(num >> 32), (DWORD)num, seed_bak, result, expected);
        ok(seed == expected,
                "test: 0x%x%08x RtlUniform(&seed (seed == %x)) sets seed to %x, expected %x\n",
                (DWORD)(num >> 32), (DWORD)num, seed_bak, result, expected);
    } /* for */
/*
 * Further investigation shows: In the different regions the highest bit
 * is set or cleared when even or odd seeds need an increment by 1.
 * This leads to a simplified algorithm:
 *
 * seed = seed * 0xffffffed + 0x7fffffc3;
 * if (seed == 0xffffffff || seed == 0x7ffffffe) {
 *     seed = (seed + 2) & MAXLONG;
 * } else if (seed == 0x7fffffff) {
 *     seed = 0;
 * } else if ((seed & 0x80000000) == 0) {
 *     seed = seed + (~seed & 1);
 * } else {
 *     seed = (seed + (seed & 1)) & MAXLONG;
 * }
 *
 * This is also the algorithm used for RtlUniform of wine (see dlls/ntdll/rtl.c).
 *
 * Now comes the funny part:
 * It took me one weekend, to find the complicated algorithm and one day more,
 * to find the simplified algorithm. Several weeks later I found out: The value
 * MAXLONG (=0x7fffffff) is never returned, neither with the native function
 * nor with the simplified algorithm. In reality the native function and our
 * function return a random number distributed over [0..MAXLONG-1]. Note
 * that this is different from what native documentation states [0..MAXLONG].
 * Expressed with D.H. Lehmer's 1948 algorithm it looks like:
 *
 * seed = (seed * const_1 + const_2) % MAXLONG;
 *
 * Further investigations show that the real algorithm is:
 *
 * seed = (seed * 0x7fffffed + 0x7fffffc3) % MAXLONG;
 *
 * This is checked with the test below:
 */
    seed = 0;
    for (num = 0; num <= 100000; num++) {
	expected = (seed * 0x7fffffed + 0x7fffffc3) % 0x7fffffff;
        seed_bak = seed;
        result = pRtlUniform(&seed);
        ok(result == expected,
                "test: 0x%x%08x RtlUniform(&seed (seed == %x)) returns %x, expected %x\n",
                (DWORD)(num >> 32), (DWORD)num, seed_bak, result, expected);
        ok(seed == expected,
                "test: 0x%x%08x RtlUniform(&seed (seed == %x)) sets seed to %x, expected %x\n",
                (DWORD)(num >> 32), (DWORD)num, seed_bak, result, expected);
    } /* for */
/*
 * More tests show that RtlUniform does not return 0x7ffffffd for seed values
 * in the range [0..MAXLONG-1]. Additionally 2 is returned twice. This shows
 * that there is more than one cycle of generated randon numbers ...
 */
}


static ULONG WINAPI my_RtlRandom(PULONG seed)
{
    static ULONG saved_value[128] =
    { /*   0 */ 0x4c8bc0aa, 0x4c022957, 0x2232827a, 0x2f1e7626, 0x7f8bdafb, 0x5c37d02a, 0x0ab48f72, 0x2f0c4ffa,
      /*   8 */ 0x290e1954, 0x6b635f23, 0x5d3885c0, 0x74b49ff8, 0x5155fa54, 0x6214ad3f, 0x111e9c29, 0x242a3a09,
      /*  16 */ 0x75932ae1, 0x40ac432e, 0x54f7ba7a, 0x585ccbd5, 0x6df5c727, 0x0374dad1, 0x7112b3f1, 0x735fc311,
      /*  24 */ 0x404331a9, 0x74d97781, 0x64495118, 0x323e04be, 0x5974b425, 0x4862e393, 0x62389c1d, 0x28a68b82,
      /*  32 */ 0x0f95da37, 0x7a50bbc6, 0x09b0091c, 0x22cdb7b4, 0x4faaed26, 0x66417ccd, 0x189e4bfa, 0x1ce4e8dd,
      /*  40 */ 0x5274c742, 0x3bdcf4dc, 0x2d94e907, 0x32eac016, 0x26d33ca3, 0x60415a8a, 0x31f57880, 0x68c8aa52,
      /*  48 */ 0x23eb16da, 0x6204f4a1, 0x373927c1, 0x0d24eb7c, 0x06dd7379, 0x2b3be507, 0x0f9c55b1, 0x2c7925eb,
      /*  56 */ 0x36d67c9a, 0x42f831d9, 0x5e3961cb, 0x65d637a8, 0x24bb3820, 0x4d08e33d, 0x2188754f, 0x147e409e,
      /*  64 */ 0x6a9620a0, 0x62e26657, 0x7bd8ce81, 0x11da0abb, 0x5f9e7b50, 0x23e444b6, 0x25920c78, 0x5fc894f0,
      /*  72 */ 0x5e338cbb, 0x404237fd, 0x1d60f80f, 0x320a1743, 0x76013d2b, 0x070294ee, 0x695e243b, 0x56b177fd,
      /*  80 */ 0x752492e1, 0x6decd52f, 0x125f5219, 0x139d2e78, 0x1898d11e, 0x2f7ee785, 0x4db405d8, 0x1a028a35,
      /*  88 */ 0x63f6f323, 0x1f6d0078, 0x307cfd67, 0x3f32a78a, 0x6980796c, 0x462b3d83, 0x34b639f2, 0x53fce379,
      /*  96 */ 0x74ba50f4, 0x1abc2c4b, 0x5eeaeb8d, 0x335a7a0d, 0x3973dd20, 0x0462d66b, 0x159813ff, 0x1e4643fd,
      /* 104 */ 0x06bc5c62, 0x3115e3fc, 0x09101613, 0x47af2515, 0x4f11ec54, 0x78b99911, 0x3db8dd44, 0x1ec10b9b,
      /* 112 */ 0x5b5506ca, 0x773ce092, 0x567be81a, 0x5475b975, 0x7a2cde1a, 0x494536f5, 0x34737bb4, 0x76d9750b,
      /* 120 */ 0x2a1f6232, 0x2e49644d, 0x7dddcbe7, 0x500cebdb, 0x619dab9e, 0x48c626fe, 0x1cda3193, 0x52dabe9d };
    ULONG rand;
    int pos;
    ULONG result;

    rand = (*seed * 0x7fffffed + 0x7fffffc3) % 0x7fffffff;
    *seed = (rand * 0x7fffffed + 0x7fffffc3) % 0x7fffffff;
    pos = *seed & 0x7f;
    result = saved_value[pos];
    saved_value[pos] = rand;
    return(result);
}


static void test_RtlRandom(void)
{
    ULONGLONG num;
    ULONG seed;
    ULONG seed_bak;
    ULONG seed_expected;
    ULONG result;
    ULONG result_expected;

/*
 * Unlike RtlUniform, RtlRandom is not documented. We guess that for
 * RtlRandom D.H. Lehmer's 1948 algorithm is used like stated in
 * the documentation of the RtlUniform function. This algorithm is:
 *
 * seed = (seed * const_1 + const_2) % const_3;
 *
 * According to the RtlUniform documentation the random number is
 * distributed over [0..MAXLONG], but in reality it is distributed
 * over [0..MAXLONG-1]. Therefore const_3 might be MAXLONG + 1 or
 * MAXLONG:
 *
 * seed = (seed * const_1 + const_2) % (MAXLONG + 1);
 *
 * or
 *
 * seed = (seed * const_1 + const_2) % MAXLONG;
 *
 * To find out const_2 we just call RtlRandom with seed set to 0:
 */
    seed = 0;
    result_expected = 0x320a1743;
    seed_expected =0x44b;
    result = pRtlRandom(&seed);

/*
 * Windows Vista uses different algorithms, so skip the rest of the tests
 * until that is figured out. Trace output for the failures is about 10.5 MB!
 */

    if (seed == 0x3fc) {
        skip("Most likely running on Windows Vista which uses a different algorithm\n");
        return;
    }

    ok(result == result_expected,
        "pRtlRandom(&seed (seed == 0)) returns %x, expected %x\n",
        result, result_expected);
    ok(seed == seed_expected,
        "pRtlRandom(&seed (seed == 0)) sets seed to %x, expected %x\n",
        seed, seed_expected);
/*
 * Seed is not equal to result as with RtlUniform. To see more we
 * call RtlRandom again with seed set to 0:
 */
    seed = 0;
    result_expected = 0x7fffffc3;
    seed_expected =0x44b;
    result = pRtlRandom(&seed);
    ok(result == result_expected,
        "RtlRandom(&seed (seed == 0)) returns %x, expected %x\n",
        result, result_expected);
    ok(seed == seed_expected,
        "RtlRandom(&seed (seed == 0)) sets seed to %x, expected %x\n",
        seed, seed_expected);
/*
 * Seed is set to the same value as before but the result is different.
 * To see more we call RtlRandom again with seed set to 0:
 */
    seed = 0;
    result_expected = 0x7fffffc3;
    seed_expected =0x44b;
    result = pRtlRandom(&seed);
    ok(result == result_expected,
        "RtlRandom(&seed (seed == 0)) returns %x, expected %x\n",
        result, result_expected);
    ok(seed == seed_expected,
        "RtlRandom(&seed (seed == 0)) sets seed to %x, expected %x\n",
        seed, seed_expected);
/*
 * Seed is again set to the same value as before. This time we also
 * have the same result as before. Interestingly the value of the
 * result is 0x7fffffc3 which is the same value used in RtlUniform
 * as const_2. If we do
 *
 * seed = 0;
 * result = RtlUniform(&seed);
 *
 * we get the same result (0x7fffffc3) as with
 *
 * seed = 0;
 * RtlRandom(&seed);
 * seed = 0;
 * result = RtlRandom(&seed);
 *
 * And there is another interesting thing. If we do
 *
 * seed = 0;
 * RtlUniform(&seed);
 * RtlUniform(&seed);
 *
 * seed is set to the value 0x44b which ist the same value that
 *
 * seed = 0;
 * RtlRandom(&seed);
 *
 * assigns to seed. Putting these two findings together leads to
 * the conclusion that RtlRandom saves the value in some variable,
 * like in the following algorithm:
 *
 * result = saved_value;
 * saved_value = RtlUniform(&seed);
 * RtlUniform(&seed);
 * return(result);
 *
 * Now we do further tests with seed set to 1:
 */
    seed = 1;
    result_expected = 0x7a50bbc6;
    seed_expected =0x5a1;
    result = pRtlRandom(&seed);
    ok(result == result_expected,
        "RtlRandom(&seed (seed == 1)) returns %x, expected %x\n",
        result, result_expected);
    ok(seed == seed_expected,
        "RtlRandom(&seed (seed == 1)) sets seed to %x, expected %x\n",
        seed, seed_expected);
/*
 * If there is just one saved_value the result now would be
 * 0x7fffffc3. From this test we can see that there is more than
 * one saved_value, like with this algorithm:
 *
 * result = saved_value[pos];
 * saved_value[pos] = RtlUniform(&seed);
 * RtlUniform(&seed);
 * return(result);
 *
 * But how is the value of pos determined? The calls to RtlUniform
 * create a sequence of random numbers. Every second random number
 * is put into the saved_value array and is used in some later call
 * of RtlRandom as result. The only reasonable source to determine
 * pos are the random numbers generated by RtlUniform which are not
 * put into the saved_value array. This are the values of seed
 * between the two calls of RtlUniform as in this algorithm:
 *
 * rand = RtlUniform(&seed);
 * RtlUniform(&seed);
 * pos = position(seed);
 * result = saved_value[pos];
 * saved_value[pos] = rand;
 * return(result);
 *
 * What remains to be determined is: The size of the saved_value array,
 * the initial values of the saved_value array and the function
 * position(seed). These tests are not shown here. 
 * The result of these tests is: The size of the saved_value array
 * is 128, the initial values can be seen in the my_RtlRandom
 * function and the position(seed) function is (seed & 0x7f).
 *
 * For a full test of RtlRandom use one of the following loop heads:
 *
 *  for (num = 0; num <= 0xffffffff; num++) {
 *      seed = num;
 *      ...
 *
 *  seed = 0;
 *  for (num = 0; num <= 0xffffffff; num++) {
 *      ...
 */
    seed = 0;
    for (num = 0; num <= 100000; num++) {
        seed_bak = seed;
	seed_expected = seed;
        result_expected = my_RtlRandom(&seed_expected);
	/* The following corrections are necessary because the */
	/* previous tests changed the saved_value array */
	if (num == 0) {
	    result_expected = 0x7fffffc3;
        } else if (num == 81) {
	    result_expected = 0x7fffffb1;
	} /* if */
        result = pRtlRandom(&seed);
        ok(result == result_expected,
                "test: 0x%x%08x RtlUniform(&seed (seed == %x)) returns %x, expected %x\n",
                (DWORD)(num >> 32), (DWORD)num, seed_bak, result, result_expected);
        ok(seed == seed_expected,
                "test: 0x%x%08x RtlUniform(&seed (seed == %x)) sets seed to %x, expected %x\n",
                (DWORD)(num >> 32), (DWORD)num, seed_bak, result, seed_expected);
    } /* for */
}


typedef struct {
    ACCESS_MASK GrantedAccess;
    ACCESS_MASK DesiredAccess;
    BOOLEAN result;
} all_accesses_t;

static const all_accesses_t all_accesses[] = {
    {0xFEDCBA76, 0xFEDCBA76, 1},
    {0x00000000, 0xFEDCBA76, 0},
    {0xFEDCBA76, 0x00000000, 1},
    {0x00000000, 0x00000000, 1},
    {0xFEDCBA76, 0xFEDCBA70, 1},
    {0xFEDCBA70, 0xFEDCBA76, 0},
    {0xFEDCBA76, 0xFEDC8A76, 1},
    {0xFEDC8A76, 0xFEDCBA76, 0},
    {0xFEDCBA76, 0xC8C4B242, 1},
    {0xC8C4B242, 0xFEDCBA76, 0},
};
#define NB_ALL_ACCESSES (sizeof(all_accesses)/sizeof(*all_accesses))


static void test_RtlAreAllAccessesGranted(void)
{
    unsigned int test_num;
    BOOLEAN result;

    for (test_num = 0; test_num < NB_ALL_ACCESSES; test_num++) {
	result = pRtlAreAllAccessesGranted(all_accesses[test_num].GrantedAccess,
					   all_accesses[test_num].DesiredAccess);
	ok(all_accesses[test_num].result == result,
           "(test %d): RtlAreAllAccessesGranted(%08x, %08x) returns %d, expected %d\n",
	   test_num, all_accesses[test_num].GrantedAccess,
	   all_accesses[test_num].DesiredAccess,
	   result, all_accesses[test_num].result);
    } /* for */
}


typedef struct {
    ACCESS_MASK GrantedAccess;
    ACCESS_MASK DesiredAccess;
    BOOLEAN result;
} any_accesses_t;

static const any_accesses_t any_accesses[] = {
    {0xFEDCBA76, 0xFEDCBA76, 1},
    {0x00000000, 0xFEDCBA76, 0},
    {0xFEDCBA76, 0x00000000, 0},
    {0x00000000, 0x00000000, 0},
    {0xFEDCBA76, 0x01234589, 0},
    {0x00040000, 0xFEDCBA76, 1},
    {0x00040000, 0xFED8BA76, 0},
    {0xFEDCBA76, 0x00040000, 1},
    {0xFED8BA76, 0x00040000, 0},
};
#define NB_ANY_ACCESSES (sizeof(any_accesses)/sizeof(*any_accesses))


static void test_RtlAreAnyAccessesGranted(void)
{
    unsigned int test_num;
    BOOLEAN result;

    for (test_num = 0; test_num < NB_ANY_ACCESSES; test_num++) {
	result = pRtlAreAnyAccessesGranted(any_accesses[test_num].GrantedAccess,
					   any_accesses[test_num].DesiredAccess);
	ok(any_accesses[test_num].result == result,
           "(test %d): RtlAreAnyAccessesGranted(%08x, %08x) returns %d, expected %d\n",
	   test_num, any_accesses[test_num].GrantedAccess,
	   any_accesses[test_num].DesiredAccess,
	   result, any_accesses[test_num].result);
    } /* for */
}

static void test_RtlComputeCrc32(void)
{
  DWORD crc = 0;

  if (!pRtlComputeCrc32)
    return;

  crc = pRtlComputeCrc32(crc, (const BYTE *)src, LEN);
  ok(crc == 0x40861dc2,"Expected 0x40861dc2, got %8x\n", crc);
}


typedef struct MY_HANDLE
{
    RTL_HANDLE RtlHandle;
    void * MyValue;
} MY_HANDLE;

static inline void RtlpMakeHandleAllocated(RTL_HANDLE * Handle)
{
    ULONG_PTR *AllocatedBit = (ULONG_PTR *)(&Handle->Next);
    *AllocatedBit = *AllocatedBit | 1;
}

static void test_HandleTables(void)
{
    BOOLEAN result;
    NTSTATUS status;
    ULONG Index;
    MY_HANDLE * MyHandle;
    RTL_HANDLE_TABLE HandleTable;

    pRtlInitializeHandleTable(0x3FFF, sizeof(MY_HANDLE), &HandleTable);
    MyHandle = (MY_HANDLE *)pRtlAllocateHandle(&HandleTable, &Index);
    ok(MyHandle != NULL, "RtlAllocateHandle failed\n");
    RtlpMakeHandleAllocated(&MyHandle->RtlHandle);
    MyHandle = NULL;
    result = pRtlIsValidIndexHandle(&HandleTable, Index, (RTL_HANDLE **)&MyHandle);
    ok(result, "Handle %p wasn't valid\n", MyHandle);
    result = pRtlFreeHandle(&HandleTable, &MyHandle->RtlHandle);
    ok(result, "Couldn't free handle %p\n", MyHandle);
    status = pRtlDestroyHandleTable(&HandleTable);
    ok(status == STATUS_SUCCESS, "RtlDestroyHandleTable failed with error 0x%08x\n", status);
}

static void test_RtlAllocateAndInitializeSid(void)
{
    NTSTATUS ret;
    SID_IDENTIFIER_AUTHORITY sia = {{ 1, 2, 3, 4, 5, 6 }};
    PSID psid;

    ret = pRtlAllocateAndInitializeSid(&sia, 0, 1, 2, 3, 4, 5, 6, 7, 8, &psid);
    ok(!ret, "RtlAllocateAndInitializeSid error %08x\n", ret);
    ret = pRtlFreeSid(psid);
    ok(!ret, "RtlFreeSid error %08x\n", ret);

    /* these tests crash on XP
    ret = pRtlAllocateAndInitializeSid(NULL, 0, 1, 2, 3, 4, 5, 6, 7, 8, &psid);
    ret = pRtlAllocateAndInitializeSid(&sia, 0, 1, 2, 3, 4, 5, 6, 7, 8, NULL);*/

    ret = pRtlAllocateAndInitializeSid(&sia, 9, 1, 2, 3, 4, 5, 6, 7, 8, &psid);
    ok(ret == STATUS_INVALID_SID, "wrong error %08x\n", ret);
}

static void test_RtlDeleteTimer(void)
{
    NTSTATUS ret;
    ret = pRtlDeleteTimer(NULL, NULL, NULL);
    ok(ret == STATUS_INVALID_PARAMETER_1 ||
       ret == STATUS_INVALID_PARAMETER, /* W2K */
       "expected STATUS_INVALID_PARAMETER_1 or STATUS_INVALID_PARAMETER, got %x\n", ret);
}

START_TEST(rtl)
{
    InitFunctionPtrs();

    if (pRtlCompareMemory)
        test_RtlCompareMemory();
    if (pRtlCompareMemoryUlong)
        test_RtlCompareMemoryUlong();
    if (pRtlMoveMemory)
        test_RtlMoveMemory();
    if (pRtlFillMemory)
        test_RtlFillMemory();
    if (pRtlFillMemoryUlong)
        test_RtlFillMemoryUlong();
    if (pRtlZeroMemory)
        test_RtlZeroMemory();
    if (pRtlUlonglongByteSwap)
        test_RtlUlonglongByteSwap();
    if (pRtlUniform)
        test_RtlUniform();
    if (pRtlRandom)
        test_RtlRandom();
    if (pRtlAreAllAccessesGranted)
        test_RtlAreAllAccessesGranted();
    if (pRtlAreAnyAccessesGranted)
        test_RtlAreAnyAccessesGranted();
    if (pRtlComputeCrc32)
        test_RtlComputeCrc32();
    if (pRtlInitializeHandleTable)
        test_HandleTables();
    if (pRtlAllocateAndInitializeSid)
        test_RtlAllocateAndInitializeSid();
    if (pRtlDeleteTimer)
        test_RtlDeleteTimer();
}
