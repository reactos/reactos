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
#include "inaddr.h"

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

/* avoid #include <winsock2.h> */
#undef htons
#ifdef WORDS_BIGENDIAN
#define htons(s) ((USHORT)(s))
#else  /* WORDS_BIGENDIAN */
static inline USHORT __my_ushort_swap(USHORT s)
{
    return (s >> 8) | (s << 8);
}
#define htons(s) __my_ushort_swap(s)
#endif  /* WORDS_BIGENDIAN */



/* Function ptrs for ntdll calls */
static HMODULE hntdll = 0;
static PVOID     (WINAPI *pWinSqmStartSession)(PVOID unknown1, DWORD unknown2, DWORD unknown3);
static NTSTATUS  (WINAPI *pWinSqmEndSession)(PVOID unknown1);
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
static struct _TEB * (WINAPI *pNtCurrentTeb)(void);
static DWORD     (WINAPI *pRtlGetThreadErrorMode)(void);
static NTSTATUS  (WINAPI *pRtlSetThreadErrorMode)(DWORD, LPDWORD);
static IMAGE_BASE_RELOCATION *(WINAPI *pLdrProcessRelocationBlock)(void*,UINT,USHORT*,INT_PTR);
static CHAR *    (WINAPI *pRtlIpv4AddressToStringA)(const IN_ADDR *, LPSTR);
static NTSTATUS  (WINAPI *pRtlIpv4AddressToStringExA)(const IN_ADDR *, USHORT, LPSTR, PULONG);
static NTSTATUS  (WINAPI *pRtlIpv4StringToAddressA)(PCSTR, BOOLEAN, PCSTR *, IN_ADDR *);
static NTSTATUS  (WINAPI *pRtlIpv4StringToAddressExA)(PCSTR, BOOLEAN, IN_ADDR *, PUSHORT);
static NTSTATUS  (WINAPI *pLdrAddRefDll)(ULONG, HMODULE);
static NTSTATUS  (WINAPI *pLdrLockLoaderLock)(ULONG, ULONG*, ULONG_PTR*);
static NTSTATUS  (WINAPI *pLdrUnlockLoaderLock)(ULONG, ULONG_PTR);
static NTSTATUS  (WINAPI *pRtlGetCompressionWorkSpaceSize)(USHORT, PULONG, PULONG);
static NTSTATUS  (WINAPI *pRtlDecompressBuffer)(USHORT, PUCHAR, ULONG, const UCHAR*, ULONG, PULONG);
static NTSTATUS  (WINAPI *pRtlDecompressFragment)(USHORT, PUCHAR, ULONG, const UCHAR*, ULONG, ULONG, PULONG, PVOID);
static NTSTATUS  (WINAPI *pRtlCompressBuffer)(USHORT, const UCHAR*, ULONG, PUCHAR, ULONG, ULONG, PULONG, PVOID);

static HMODULE hkernel32 = 0;
static BOOL      (WINAPI *pIsWow64Process)(HANDLE, PBOOL);


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
        pWinSqmStartSession = (void *)GetProcAddress(hntdll, "WinSqmStartSession");
        pWinSqmEndSession = (void *)GetProcAddress(hntdll, "WinSqmEndSession");
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
        pNtCurrentTeb = (void *)GetProcAddress(hntdll, "NtCurrentTeb");
        pRtlGetThreadErrorMode = (void *)GetProcAddress(hntdll, "RtlGetThreadErrorMode");
        pRtlSetThreadErrorMode = (void *)GetProcAddress(hntdll, "RtlSetThreadErrorMode");
        pLdrProcessRelocationBlock  = (void *)GetProcAddress(hntdll, "LdrProcessRelocationBlock");
        pRtlIpv4AddressToStringA = (void *)GetProcAddress(hntdll, "RtlIpv4AddressToStringA");
        pRtlIpv4AddressToStringExA = (void *)GetProcAddress(hntdll, "RtlIpv4AddressToStringExA");
        pRtlIpv4StringToAddressA = (void *)GetProcAddress(hntdll, "RtlIpv4StringToAddressA");
        pRtlIpv4StringToAddressExA = (void *)GetProcAddress(hntdll, "RtlIpv4StringToAddressExA");
        pLdrAddRefDll = (void *)GetProcAddress(hntdll, "LdrAddRefDll");
        pLdrLockLoaderLock = (void *)GetProcAddress(hntdll, "LdrLockLoaderLock");
        pLdrUnlockLoaderLock = (void *)GetProcAddress(hntdll, "LdrUnlockLoaderLock");
        pRtlGetCompressionWorkSpaceSize = (void *)GetProcAddress(hntdll, "RtlGetCompressionWorkSpaceSize");
        pRtlDecompressBuffer = (void *)GetProcAddress(hntdll, "RtlDecompressBuffer");
        pRtlDecompressFragment = (void *)GetProcAddress(hntdll, "RtlDecompressFragment");
        pRtlCompressBuffer = (void *)GetProcAddress(hntdll, "RtlCompressBuffer");
    }
    hkernel32 = LoadLibraryA("kernel32.dll");
    ok(hkernel32 != 0, "LoadLibrary failed\n");
    if (hkernel32) {
        pIsWow64Process = (void *)GetProcAddress(hkernel32, "IsWow64Process");
    }
    strcpy((char*)src_aligned_block, src_src);
    ok(strlen(src) == 15, "Source must be 16 bytes long!\n");
}

#ifdef __i386__
const char stdcall3_thunk[] =
    "\x56"              /* push %esi */
    "\x89\xE6"          /* mov %esp, %esi */
    "\xFF\x74\x24\x14"  /* pushl 20(%esp) */
    "\xFF\x74\x24\x14"  /* pushl 20(%esp) */
    "\xFF\x74\x24\x14"  /* pushl 20(%esp) */
    "\xFF\x54\x24\x14"  /* calll 20(%esp) */
    "\x89\xF0"          /* mov %esi, %eax */
    "\x29\xE0"          /* sub %esp, %eax */
    "\x89\xF4"          /* mov %esi, %esp */
    "\x5E"              /* pop %esi */
    "\xC2\x10\x00"      /* ret $16 */
;

static INT (WINAPI *call_stdcall_func3)(PVOID func, PVOID arg0, DWORD arg1, DWORD arg2) = NULL;

static void test_WinSqm(void)
{
    INT args;

    if (!pWinSqmStartSession)
    {
        win_skip("WinSqmStartSession() is not available\n");
        return;
    }

    call_stdcall_func3 = (void*) VirtualAlloc( NULL, sizeof(stdcall3_thunk) - 1, MEM_COMMIT,
                                               PAGE_EXECUTE_READWRITE );
    memcpy( call_stdcall_func3, stdcall3_thunk, sizeof(stdcall3_thunk) - 1 );

    args = 3 - call_stdcall_func3( pWinSqmStartSession, NULL, 0, 0 ) / 4;
    ok(args == 3, "WinSqmStartSession expected to take %d arguments instead of 3\n", args);
    args = 3 - call_stdcall_func3( pWinSqmEndSession, NULL, 0, 0 ) / 4;
    ok(args == 1, "WinSqmEndSession expected to take %d arguments instead of 1\n", args);

    VirtualFree( call_stdcall_func3, 0, MEM_RELEASE );
}
#endif

#define COMP(str1,str2,cmplen,len) size = pRtlCompareMemory(str1, str2, cmplen); \
  ok(size == len, "Expected %ld, got %ld\n", size, (SIZE_T)len)

static void test_RtlCompareMemory(void)
{
  SIZE_T size;

  if (!pRtlCompareMemory)
  {
    win_skip("RtlCompareMemory is not available\n");
    return;
  }

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

    if (!pRtlCompareMemoryUlong)
    {
        win_skip("RtlCompareMemoryUlong is not available\n");
        return;
    }

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
  {
    win_skip("RtlMoveMemory is not available\n");
    return;
  }

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
  {
    win_skip("RtlFillMemory is not available\n");
    return;
  }

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
  {
    win_skip("RtlFillMemoryUlong is not available\n");
    return;
  }

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
  {
    win_skip("RtlZeroMemory is not available\n");
    return;
  }

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

    if ( !pRtlUlonglongByteSwap )
    {
        win_skip("RtlUlonglongByteSwap is not available\n");
        return;
    }

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

    if (!pRtlUniform)
    {
        win_skip("RtlUniform is not available\n");
        return;
    }

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


static ULONG my_RtlRandom(PULONG seed)
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

    if (!pRtlRandom)
    {
        win_skip("RtlRandom is not available\n");
        return;
    }

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

    if (!pRtlAreAllAccessesGranted)
    {
        win_skip("RtlAreAllAccessesGranted is not available\n");
        return;
    }

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

    if (!pRtlAreAnyAccessesGranted)
    {
        win_skip("RtlAreAnyAccessesGranted is not available\n");
        return;
    }

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
  {
    win_skip("RtlComputeCrc32 is not available\n");
    return;
  }

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

    if (!pRtlInitializeHandleTable)
    {
        win_skip("RtlInitializeHandleTable is not available\n");
        return;
    }

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

    if (!pRtlAllocateAndInitializeSid)
    {
        win_skip("RtlAllocateAndInitializeSid is not available\n");
        return;
    }

    ret = pRtlAllocateAndInitializeSid(&sia, 0, 1, 2, 3, 4, 5, 6, 7, 8, &psid);
    ok(!ret, "RtlAllocateAndInitializeSid error %08x\n", ret);
    ret = pRtlFreeSid(psid);
    ok(!ret, "RtlFreeSid error %08x\n", ret);

    /* these tests crash on XP */
    if (0)
    {
        pRtlAllocateAndInitializeSid(NULL, 0, 1, 2, 3, 4, 5, 6, 7, 8, &psid);
        pRtlAllocateAndInitializeSid(&sia, 0, 1, 2, 3, 4, 5, 6, 7, 8, NULL);
    }

    ret = pRtlAllocateAndInitializeSid(&sia, 9, 1, 2, 3, 4, 5, 6, 7, 8, &psid);
    ok(ret == STATUS_INVALID_SID, "wrong error %08x\n", ret);
}

static void test_RtlDeleteTimer(void)
{
    NTSTATUS ret;

    if (!pRtlDeleteTimer)
    {
        win_skip("RtlDeleteTimer is not available\n");
        return;
    }

    ret = pRtlDeleteTimer(NULL, NULL, NULL);
    ok(ret == STATUS_INVALID_PARAMETER_1 ||
       ret == STATUS_INVALID_PARAMETER, /* W2K */
       "expected STATUS_INVALID_PARAMETER_1 or STATUS_INVALID_PARAMETER, got %x\n", ret);
}

static void test_RtlThreadErrorMode(void)
{
    DWORD oldmode;
    BOOL is_wow64;
    DWORD mode;
    NTSTATUS status;

    if (!pRtlGetThreadErrorMode || !pRtlSetThreadErrorMode)
    {
        win_skip("RtlGetThreadErrorMode and/or RtlSetThreadErrorMode not available\n");
        return;
    }

    if (!pIsWow64Process || !pIsWow64Process(GetCurrentProcess(), &is_wow64))
        is_wow64 = FALSE;

    oldmode = pRtlGetThreadErrorMode();

    status = pRtlSetThreadErrorMode(0x70, &mode);
    ok(status == STATUS_SUCCESS ||
       status == STATUS_WAIT_1, /* Vista */
       "RtlSetThreadErrorMode failed with error 0x%08x\n", status);
    ok(mode == oldmode,
       "RtlSetThreadErrorMode returned mode 0x%x, expected 0x%x\n",
       mode, oldmode);
    ok(pRtlGetThreadErrorMode() == 0x70,
       "RtlGetThreadErrorMode returned 0x%x, expected 0x%x\n", mode, 0x70);
    if (!is_wow64 && pNtCurrentTeb)
        ok(pNtCurrentTeb()->HardErrorDisabled == 0x70,
           "The TEB contains 0x%x, expected 0x%x\n",
           pNtCurrentTeb()->HardErrorDisabled, 0x70);

    status = pRtlSetThreadErrorMode(0, &mode);
    ok(status == STATUS_SUCCESS ||
       status == STATUS_WAIT_1, /* Vista */
       "RtlSetThreadErrorMode failed with error 0x%08x\n", status);
    ok(mode == 0x70,
       "RtlSetThreadErrorMode returned mode 0x%x, expected 0x%x\n",
       mode, 0x70);
    ok(pRtlGetThreadErrorMode() == 0,
       "RtlGetThreadErrorMode returned 0x%x, expected 0x%x\n", mode, 0);
    if (!is_wow64 && pNtCurrentTeb)
        ok(pNtCurrentTeb()->HardErrorDisabled == 0,
           "The TEB contains 0x%x, expected 0x%x\n",
           pNtCurrentTeb()->HardErrorDisabled, 0);

    for (mode = 1; mode; mode <<= 1)
    {
        status = pRtlSetThreadErrorMode(mode, NULL);
        if (mode & 0x70)
            ok(status == STATUS_SUCCESS ||
               status == STATUS_WAIT_1, /* Vista */
               "RtlSetThreadErrorMode(%x,NULL) failed with error 0x%08x\n",
               mode, status);
        else
            ok(status == STATUS_INVALID_PARAMETER_1,
               "RtlSetThreadErrorMode(%x,NULL) returns 0x%08x, "
               "expected STATUS_INVALID_PARAMETER_1\n",
               mode, status);
    }

    pRtlSetThreadErrorMode(oldmode, NULL);
}

static void test_LdrProcessRelocationBlock(void)
{
    IMAGE_BASE_RELOCATION *ret;
    USHORT reloc;
    DWORD addr32;
    SHORT addr16;

    if(!pLdrProcessRelocationBlock) {
        win_skip("LdrProcessRelocationBlock not available\n");
        return;
    }

    addr32 = 0x50005;
    reloc = IMAGE_REL_BASED_HIGHLOW<<12;
    ret = pLdrProcessRelocationBlock(&addr32, 1, &reloc, 0x500050);
    ok((USHORT*)ret == &reloc+1, "ret = %p, expected %p\n", ret, &reloc+1);
    ok(addr32 == 0x550055, "addr32 = %x, expected 0x550055\n", addr32);

    addr16 = 0x505;
    reloc = IMAGE_REL_BASED_HIGH<<12;
    ret = pLdrProcessRelocationBlock(&addr16, 1, &reloc, 0x500060);
    ok((USHORT*)ret == &reloc+1, "ret = %p, expected %p\n", ret, &reloc+1);
    ok(addr16 == 0x555, "addr16 = %x, expected 0x555\n", addr16);

    addr16 = 0x505;
    reloc = IMAGE_REL_BASED_LOW<<12;
    ret = pLdrProcessRelocationBlock(&addr16, 1, &reloc, 0x500060);
    ok((USHORT*)ret == &reloc+1, "ret = %p, expected %p\n", ret, &reloc+1);
    ok(addr16 == 0x565, "addr16 = %x, expected 0x565\n", addr16);
}

static void test_RtlIpv4AddressToString(void)
{
    CHAR buffer[20];
    CHAR *res;
    IN_ADDR ip;
    DWORD_PTR len;

    if (!pRtlIpv4AddressToStringA)
    {
        win_skip("RtlIpv4AddressToStringA not available\n");
        return;
    }

    ip.S_un.S_un_b.s_b1 = 1;
    ip.S_un.S_un_b.s_b2 = 2;
    ip.S_un.S_un_b.s_b3 = 3;
    ip.S_un.S_un_b.s_b4 = 4;

    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringA(&ip, buffer);
    len = strlen(buffer);
    ok(res == (buffer + len), "got %p with '%s' (expected %p)\n", res, buffer, buffer + len);

    res = pRtlIpv4AddressToStringA(&ip, NULL);
    ok( (res == (char *)~0) ||
        broken(res == (char *)len),        /* XP and w2003 */
        "got %p (expected ~0)\n", res);

    if (0) {
        /* this crashes in windows */
        memset(buffer, '#', sizeof(buffer) - 1);
        buffer[sizeof(buffer) -1] = 0;
        res = pRtlIpv4AddressToStringA(NULL, buffer);
        trace("got %p with '%s'\n", res, buffer);
    }

    if (0) {
        /* this crashes in windows */
        res = pRtlIpv4AddressToStringA(NULL, NULL);
        trace("got %p\n", res);
    }
}

static void test_RtlIpv4AddressToStringEx(void)
{
    CHAR ip_1234[] = "1.2.3.4";
    CHAR ip_1234_80[] = "1.2.3.4:80";
    LPSTR expect;
    CHAR buffer[30];
    NTSTATUS res;
    IN_ADDR ip;
    ULONG size;
    DWORD used;
    USHORT port;

    if (!pRtlIpv4AddressToStringExA)
    {
        win_skip("RtlIpv4AddressToStringExA not available\n");
        return;
    }

    ip.S_un.S_un_b.s_b1 = 1;
    ip.S_un.S_un_b.s_b2 = 2;
    ip.S_un.S_un_b.s_b3 = 3;
    ip.S_un.S_un_b.s_b4 = 4;

    port = htons(80);
    expect = ip_1234_80;

    size = sizeof(buffer);
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    used = strlen(buffer);
    ok( (res == STATUS_SUCCESS) &&
        (size == strlen(expect) + 1) && !strcmp(buffer, expect),
        "got 0x%x and size %d with '%s'\n", res, size, buffer);

    size = used + 1;
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    ok( (res == STATUS_SUCCESS) &&
        (size == strlen(expect) + 1) && !strcmp(buffer, expect),
        "got 0x%x and size %d with '%s'\n", res, size, buffer);

    size = used;
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    ok( (res == STATUS_INVALID_PARAMETER) && (size == used + 1),
        "got 0x%x and %d with '%s' (expected STATUS_INVALID_PARAMETER and %d)\n",
        res, size, buffer, used + 1);

    size = used - 1;
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    ok( (res == STATUS_INVALID_PARAMETER) && (size == used + 1),
        "got 0x%x and %d with '%s' (expected STATUS_INVALID_PARAMETER and %d)\n",
        res, size, buffer, used + 1);


    /* to get only the ip, use 0 as port */
    port = 0;
    expect = ip_1234;

    size = sizeof(buffer);
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    used = strlen(buffer);
    ok( (res == STATUS_SUCCESS) &&
        (size == strlen(expect) + 1) && !strcmp(buffer, expect),
        "got 0x%x and size %d with '%s'\n", res, size, buffer);

    size = used + 1;
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    ok( (res == STATUS_SUCCESS) &&
        (size == strlen(expect) + 1) && !strcmp(buffer, expect),
        "got 0x%x and size %d with '%s'\n", res, size, buffer);

    size = used;
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    ok( (res == STATUS_INVALID_PARAMETER) && (size == used + 1),
        "got 0x%x and %d with '%s' (expected STATUS_INVALID_PARAMETER and %d)\n",
        res, size, buffer, used + 1);

    size = used - 1;
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    ok( (res == STATUS_INVALID_PARAMETER) && (size == used + 1),
        "got 0x%x and %d with '%s' (expected STATUS_INVALID_PARAMETER and %d)\n",
        res, size, buffer, used + 1);


    /* parameters are checked */
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, 0, buffer, NULL);
    ok(res == STATUS_INVALID_PARAMETER,
        "got 0x%x with '%s' (expected STATUS_INVALID_PARAMETER)\n", res, buffer);

    size = sizeof(buffer);
    res = pRtlIpv4AddressToStringExA(&ip, 0, NULL, &size);
    ok( res == STATUS_INVALID_PARAMETER,
        "got 0x%x and size %d (expected STATUS_INVALID_PARAMETER)\n", res, size);

    size = sizeof(buffer);
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(NULL, 0, buffer, &size);
    ok( res == STATUS_INVALID_PARAMETER,
        "got 0x%x and size %d with '%s' (expected STATUS_INVALID_PARAMETER)\n",
        res, size, buffer);
}

static void test_RtlIpv4StringToAddress(void)
{
    NTSTATUS res;
    IN_ADDR ip, expected_ip;
    PCSTR terminator;
    CHAR dummy;
    struct
    {
        PCSTR address;
        NTSTATUS res;
        int terminator_offset;
        int ip[4];
        BOOL strict_is_different;
        NTSTATUS res_strict;
        int terminator_offset_strict;
        int ip_strict[4];
    } tests[] =
    {
        { "",                STATUS_INVALID_PARAMETER,  0, { -1 } },
        { " ",               STATUS_INVALID_PARAMETER,  0, { -1 } },
        { "1.1.1.1",         STATUS_SUCCESS,            7, {   1,   1,   1,   1 } },
        { "0.0.0.0",         STATUS_SUCCESS,            7, {   0,   0,   0,   0 } },
        { "255.255.255.255", STATUS_SUCCESS,           15, { 255, 255, 255, 255 } },
        { "255.255.255.255:123",
                             STATUS_SUCCESS,           15, { 255, 255, 255, 255 } },
        { "255.255.255.256", STATUS_INVALID_PARAMETER, 15, { -1 } },
        { "255.255.255.4294967295",
                             STATUS_INVALID_PARAMETER, 22, { -1 } },
        { "255.255.255.4294967296",
                             STATUS_INVALID_PARAMETER, 21, { -1 } },
        { "255.255.255.4294967297",
                             STATUS_INVALID_PARAMETER, 21, { -1 } },
        { "a",               STATUS_INVALID_PARAMETER,  0, { -1 } },
        { "1.1.1.0xaA",      STATUS_SUCCESS,           10, {   1,   1,   1, 170 },
                       TRUE, STATUS_INVALID_PARAMETER,  8, { -1 } },
        { "1.1.1.0XaA",      STATUS_SUCCESS,           10, {   1,   1,   1, 170 },
                       TRUE, STATUS_INVALID_PARAMETER,  8, { -1 } },
        { "1.1.1.0x",        STATUS_INVALID_PARAMETER,  8, { -1 } },
        { "1.1.1.0xff",      STATUS_SUCCESS,           10, {   1,   1,   1, 255 },
                       TRUE, STATUS_INVALID_PARAMETER,  8, { -1 } },
        { "1.1.1.0x100",     STATUS_INVALID_PARAMETER, 11, { -1 },
                       TRUE, STATUS_INVALID_PARAMETER,  8, { -1 } },
        { "1.1.1.0xffffffff",STATUS_INVALID_PARAMETER, 16, { -1 },
                       TRUE, STATUS_INVALID_PARAMETER,  8, { -1 } },
        { "1.1.1.0x100000000",
                             STATUS_INVALID_PARAMETER, 16, { -1, 0, 0, 0 },
                       TRUE, STATUS_INVALID_PARAMETER,  8, { -1 } },
        { "1.1.1.010",       STATUS_SUCCESS,            9, {   1,   1,   1,   8 },
                       TRUE, STATUS_INVALID_PARAMETER,  7, { -1 } },
        { "1.1.1.00",        STATUS_SUCCESS,            8, {   1,   1,   1,   0 },
                       TRUE, STATUS_INVALID_PARAMETER,  7, { -1 } },
        { "1.1.1.007",       STATUS_SUCCESS,            9, {   1,   1,   1,   7 },
                       TRUE, STATUS_INVALID_PARAMETER,  7, { -1 } },
        { "1.1.1.08",        STATUS_INVALID_PARAMETER,  7, { -1 } },
        { "1.1.1.008",       STATUS_SUCCESS,            8, {   1,   1,   1,   0 },
                       TRUE, STATUS_INVALID_PARAMETER,  7, { -1 } },
        { "1.1.1.0a",        STATUS_SUCCESS,            7, {   1,   1,   1,   0 } },
        { "1.1.1.0o10",      STATUS_SUCCESS,            7, {   1,   1,   1,   0 } },
        { "1.1.1.0b10",      STATUS_SUCCESS,            7, {   1,   1,   1,   0 } },
        { "1.1.1.-2",        STATUS_INVALID_PARAMETER,  6, { -1 } },
        { "1",               STATUS_SUCCESS,            1, {   0,   0,   0,   1 },
                       TRUE, STATUS_INVALID_PARAMETER,  1, { -1 } },
        { "-1",              STATUS_INVALID_PARAMETER,  0, { -1 } },
        { "203569230",       STATUS_SUCCESS,            9, {  12,  34,  56,  78 },
                       TRUE, STATUS_INVALID_PARAMETER,  9, { -1 } },
        { "1.223756",        STATUS_SUCCESS,            8, {   1,   3, 106,  12 },
                       TRUE, STATUS_INVALID_PARAMETER,  8, { -1 } },
        { "3.4.756",         STATUS_SUCCESS,            7, {   3,   4,   2, 244 },
                       TRUE, STATUS_INVALID_PARAMETER,  7, { -1 } },
        { "3.4.756.1",       STATUS_INVALID_PARAMETER,  9, { -1 } },
        { "3.4.65536",       STATUS_INVALID_PARAMETER,  9, { -1 } },
        { "3.4.5.6.7",       STATUS_INVALID_PARAMETER,  7, { -1 } },
        { "3.4.5.+6",        STATUS_INVALID_PARAMETER,  6, { -1 } },
        { " 3.4.5.6",        STATUS_INVALID_PARAMETER,  0, { -1 } },
        { "\t3.4.5.6",       STATUS_INVALID_PARAMETER,  0, { -1 } },
        { "3.4.5.6 ",        STATUS_SUCCESS,            7, {   3,   4,   5,   6 } },
        { "3. 4.5.6",        STATUS_INVALID_PARAMETER,  2, { -1 } },
        { ".",               STATUS_INVALID_PARAMETER,  1, { -1 } },
        { "..",              STATUS_INVALID_PARAMETER,  1, { -1 } },
        { "1.",              STATUS_INVALID_PARAMETER,  2, { -1 } },
        { "1..",             STATUS_INVALID_PARAMETER,  3, { -1 } },
        { ".1",              STATUS_INVALID_PARAMETER,  1, { -1 } },
        { ".1.",             STATUS_INVALID_PARAMETER,  1, { -1 } },
        { ".1.2.3",          STATUS_INVALID_PARAMETER,  1, { -1 } },
        { "0.1.2.3",         STATUS_SUCCESS,            7, {   0,   1,   2,   3 } },
        { "0.1.2.3.",        STATUS_INVALID_PARAMETER,  7, { -1 } },
        { "[0.1.2.3]",       STATUS_INVALID_PARAMETER,  0, { -1 } },
        { "::1",             STATUS_INVALID_PARAMETER,  0, { -1 } },
        { ":1",              STATUS_INVALID_PARAMETER,  0, { -1 } },
    };
    const int testcount = sizeof(tests) / sizeof(tests[0]);
    int i;

    if (!pRtlIpv4StringToAddressA)
    {
        skip("RtlIpv4StringToAddress not available\n");
        return;
    }

    if (0)
    {
        /* leaving either parameter NULL crashes on Windows */
        res = pRtlIpv4StringToAddressA(NULL, FALSE, &terminator, &ip);
        res = pRtlIpv4StringToAddressA("1.1.1.1", FALSE, NULL, &ip);
        res = pRtlIpv4StringToAddressA("1.1.1.1", FALSE, &terminator, NULL);
        /* same for the wide char version */
        /*
        res = pRtlIpv4StringToAddressW(NULL, FALSE, &terminatorW, &ip);
        res = pRtlIpv4StringToAddressW(L"1.1.1.1", FALSE, NULL, &ip);
        res = pRtlIpv4StringToAddressW(L"1.1.1.1", FALSE, &terminatorW, NULL);
        */
    }

    for (i = 0; i < testcount; i++)
    {
        /* non-strict */
        terminator = &dummy;
        ip.S_un.S_addr = 0xabababab;
        res = pRtlIpv4StringToAddressA(tests[i].address, FALSE, &terminator, &ip);
        ok(res == tests[i].res,
           "[%s] res = 0x%08x, expected 0x%08x\n",
           tests[i].address, res, tests[i].res);
        ok(terminator == tests[i].address + tests[i].terminator_offset,
           "[%s] terminator = %p, expected %p\n",
           tests[i].address, terminator, tests[i].address + tests[i].terminator_offset);
        if (tests[i].ip[0] == -1)
            expected_ip.S_un.S_addr = 0xabababab;
        else
        {
            expected_ip.S_un.S_un_b.s_b1 = tests[i].ip[0];
            expected_ip.S_un.S_un_b.s_b2 = tests[i].ip[1];
            expected_ip.S_un.S_un_b.s_b3 = tests[i].ip[2];
            expected_ip.S_un.S_un_b.s_b4 = tests[i].ip[3];
        }
        ok(ip.S_un.S_addr == expected_ip.S_un.S_addr,
           "[%s] ip = %08x, expected %08x\n",
           tests[i].address, ip.S_un.S_addr, expected_ip.S_un.S_addr);

        if (!tests[i].strict_is_different)
        {
            tests[i].res_strict = tests[i].res;
            tests[i].terminator_offset_strict = tests[i].terminator_offset;
            tests[i].ip_strict[0] = tests[i].ip[0];
            tests[i].ip_strict[1] = tests[i].ip[1];
            tests[i].ip_strict[2] = tests[i].ip[2];
            tests[i].ip_strict[3] = tests[i].ip[3];
        }
        /* strict */
        terminator = &dummy;
        ip.S_un.S_addr = 0xabababab;
        res = pRtlIpv4StringToAddressA(tests[i].address, TRUE, &terminator, &ip);
        ok(res == tests[i].res_strict,
           "[%s] res = 0x%08x, expected 0x%08x\n",
           tests[i].address, res, tests[i].res_strict);
        ok(terminator == tests[i].address + tests[i].terminator_offset_strict,
           "[%s] terminator = %p, expected %p\n",
           tests[i].address, terminator, tests[i].address + tests[i].terminator_offset_strict);
        if (tests[i].ip_strict[0] == -1)
            expected_ip.S_un.S_addr = 0xabababab;
        else
        {
            expected_ip.S_un.S_un_b.s_b1 = tests[i].ip_strict[0];
            expected_ip.S_un.S_un_b.s_b2 = tests[i].ip_strict[1];
            expected_ip.S_un.S_un_b.s_b3 = tests[i].ip_strict[2];
            expected_ip.S_un.S_un_b.s_b4 = tests[i].ip_strict[3];
        }
        ok(ip.S_un.S_addr == expected_ip.S_un.S_addr,
           "[%s] ip = %08x, expected %08x\n",
           tests[i].address, ip.S_un.S_addr, expected_ip.S_un.S_addr);
    }
}

static void test_RtlIpv4StringToAddressEx(void)
{
    NTSTATUS res;
    IN_ADDR ip, expected_ip;
    USHORT port;
    struct
    {
        PCSTR address;
        NTSTATUS res;
        int ip[4];
        USHORT port;
    } tests[] =
    {
        { "",               STATUS_INVALID_PARAMETER,   { -1 },         0xdead },
        { " ",              STATUS_INVALID_PARAMETER,   { -1 },         0xdead },
        { "1.1.1.1:",       STATUS_INVALID_PARAMETER,   { 1, 1, 1, 1 }, 0xdead },
        { "1.1.1.1+",       STATUS_INVALID_PARAMETER,   { 1, 1, 1, 1 }, 0xdead },
        { "1.1.1.1:1",      STATUS_SUCCESS,             { 1, 1, 1, 1 }, 0x100 },
        { "0.0.0.0:0",      STATUS_INVALID_PARAMETER,   { 0, 0, 0, 0 }, 0xdead },
        { "0.0.0.0:1",      STATUS_SUCCESS,             { 0, 0, 0, 0 }, 0x100 },
        { "1.2.3.4:65535",  STATUS_SUCCESS,             { 1, 2, 3, 4 }, 65535 },
        { "1.2.3.4:65536",  STATUS_INVALID_PARAMETER,   { 1, 2, 3, 4 }, 0xdead },
        { "1.2.3.4:0xffff", STATUS_SUCCESS,             { 1, 2, 3, 4 }, 65535 },
        { "1.2.3.4:0XfFfF", STATUS_SUCCESS,             { 1, 2, 3, 4 }, 65535 },
        { "1.2.3.4:011064", STATUS_SUCCESS,             { 1, 2, 3, 4 }, 0x3412 },
        { "1.2.3.4:1234a",  STATUS_INVALID_PARAMETER,   { 1, 2, 3, 4 }, 0xdead },
        { "1.2.3.4:1234+",  STATUS_INVALID_PARAMETER,   { 1, 2, 3, 4 }, 0xdead },
        { "1.2.3.4: 1234",  STATUS_INVALID_PARAMETER,   { 1, 2, 3, 4 }, 0xdead },
        { "1.2.3.4:\t1234", STATUS_INVALID_PARAMETER,   { 1, 2, 3, 4 }, 0xdead },
    };
    const int testcount = sizeof(tests) / sizeof(tests[0]);
    int i, Strict;

    if (!pRtlIpv4StringToAddressExA)
    {
        skip("RtlIpv4StringToAddressEx not available\n");
        return;
    }

    /* do not crash, and do not touch the ip / port. */
    ip.S_un.S_addr = 0xabababab;
    port = 0xdead;
    res = pRtlIpv4StringToAddressExA(NULL, FALSE, &ip, &port);
    ok(res == STATUS_INVALID_PARAMETER, "[null address] res = 0x%08x, expected 0x%08x\n", res, STATUS_INVALID_PARAMETER);
    ok(ip.S_un.S_addr == 0xabababab, "RtlIpv4StringToAddressExA should not touch the ip!, ip == %x\n", ip.S_un.S_addr);
    ok(port == 0xdead, "RtlIpv4StringToAddressExA should not touch the port!, port == %x\n", port);

    port = 0xdead;
    res = pRtlIpv4StringToAddressExA("1.1.1.1", FALSE, NULL, &port);
    ok(res == STATUS_INVALID_PARAMETER, "[null ip] res = 0x%08x, expected 0x%08x\n", res, STATUS_INVALID_PARAMETER);
    ok(port == 0xdead, "RtlIpv4StringToAddressExA should not touch the port!, port == %x\n", port);

    ip.S_un.S_addr = 0xabababab;
    port = 0xdead;
    res = pRtlIpv4StringToAddressExA("1.1.1.1", FALSE, &ip, NULL);
    ok(res == STATUS_INVALID_PARAMETER, "[null port] res = 0x%08x, expected 0x%08x\n", res, STATUS_INVALID_PARAMETER);
    ok(ip.S_un.S_addr == 0xabababab, "RtlIpv4StringToAddressExA should not touch the ip!, ip == %x\n", ip.S_un.S_addr);
    ok(port == 0xdead, "RtlIpv4StringToAddressExA should not touch the port!, port == %x\n", port);

    for (i = 0; i < testcount; i++)
    {
        /* Strict is only relevant for the ip address, so make sure that it does not influence the port */
        for (Strict = 0; Strict < 2; Strict++)
        {
            ip.S_un.S_addr = 0xabababab;
            port = 0xdead;
            res = pRtlIpv4StringToAddressExA(tests[i].address, Strict, &ip, &port);
            if (tests[i].ip[0] == -1)
            {
                expected_ip.S_un.S_addr = 0xabababab;
            }
            else
            {
                expected_ip.S_un.S_un_b.s_b1 = tests[i].ip[0];
                expected_ip.S_un.S_un_b.s_b2 = tests[i].ip[1];
                expected_ip.S_un.S_un_b.s_b3 = tests[i].ip[2];
                expected_ip.S_un.S_un_b.s_b4 = tests[i].ip[3];
            }
            ok(res == tests[i].res, "[%s] res = 0x%08x, expected 0x%08x\n",
               tests[i].address, res, tests[i].res);
            ok(ip.S_un.S_addr == expected_ip.S_un.S_addr, "[%s] ip = %08x, expected %08x\n",
               tests[i].address, ip.S_un.S_addr, expected_ip.S_un.S_addr);
            ok(port == tests[i].port, "[%s] port = %u, expected %u\n",
               tests[i].address, port, tests[i].port);
        }
    }
}

static void test_LdrAddRefDll(void)
{
    HMODULE mod, mod2;
    NTSTATUS status;
    BOOL ret;

    if (!pLdrAddRefDll)
    {
        win_skip( "LdrAddRefDll not supported\n" );
        return;
    }

    mod = LoadLibraryA("comctl32.dll");
    ok(mod != NULL, "got %p\n", mod);
    ret = FreeLibrary(mod);
    ok(ret, "got %d\n", ret);

    mod2 = GetModuleHandleA("comctl32.dll");
    ok(mod2 == NULL, "got %p\n", mod2);

    /* load, addref and release 2 times */
    mod = LoadLibraryA("comctl32.dll");
    ok(mod != NULL, "got %p\n", mod);
    status = pLdrAddRefDll(0, mod);
    ok(status == STATUS_SUCCESS, "got 0x%08x\n", status);
    ret = FreeLibrary(mod);
    ok(ret, "got %d\n", ret);

    mod2 = GetModuleHandleA("comctl32.dll");
    ok(mod2 != NULL, "got %p\n", mod2);
    ret = FreeLibrary(mod);
    ok(ret, "got %d\n", ret);

    mod2 = GetModuleHandleA("comctl32.dll");
    ok(mod2 == NULL, "got %p\n", mod2);

    /* pin refcount */
    mod = LoadLibraryA("comctl32.dll");
    ok(mod != NULL, "got %p\n", mod);
    status = pLdrAddRefDll(LDR_ADDREF_DLL_PIN, mod);
    ok(status == STATUS_SUCCESS, "got 0x%08x\n", status);

    ret = FreeLibrary(mod);
    ok(ret, "got %d\n", ret);
    ret = FreeLibrary(mod);
    ok(ret, "got %d\n", ret);
    ret = FreeLibrary(mod);
    ok(ret, "got %d\n", ret);
    ret = FreeLibrary(mod);
    ok(ret, "got %d\n", ret);

    mod2 = GetModuleHandleA("comctl32.dll");
    ok(mod2 != NULL, "got %p\n", mod2);
}

static void test_LdrLockLoaderLock(void)
{
    ULONG_PTR magic;
    ULONG result;
    NTSTATUS status;

    if (!pLdrLockLoaderLock)
    {
        win_skip("LdrLockLoaderLock() is not available\n");
        return;
    }

    /* invalid flags */
    result = 10;
    magic = 0xdeadbeef;
    status = pLdrLockLoaderLock(0x10, &result, &magic);
    ok(status == STATUS_INVALID_PARAMETER_1, "got 0x%08x\n", status);
    ok(result == 0, "got %d\n", result);
    ok(magic == 0, "got %lx\n", magic);

    magic = 0xdeadbeef;
    status = pLdrLockLoaderLock(0x10, NULL, &magic);
    ok(status == STATUS_INVALID_PARAMETER_1, "got 0x%08x\n", status);
    ok(magic == 0, "got %lx\n", magic);

    result = 10;
    status = pLdrLockLoaderLock(0x10, &result, NULL);
    ok(status == STATUS_INVALID_PARAMETER_1, "got 0x%08x\n", status);
    ok(result == 0, "got %d\n", result);

    /* non-blocking mode, result is null */
    magic = 0xdeadbeef;
    status = pLdrLockLoaderLock(0x2, NULL, &magic);
    ok(status == STATUS_INVALID_PARAMETER_2, "got 0x%08x\n", status);
    ok(magic == 0, "got %lx\n", magic);

    /* magic pointer is null */
    result = 10;
    status = pLdrLockLoaderLock(0, &result, NULL);
    ok(status == STATUS_INVALID_PARAMETER_3, "got 0x%08x\n", status);
    ok(result == 0, "got %d\n", result);

    /* lock in non-blocking mode */
    result = 0;
    magic = 0;
    status = pLdrLockLoaderLock(0x2, &result, &magic);
    ok(status == STATUS_SUCCESS, "got 0x%08x\n", status);
    ok(result == 1, "got %d\n", result);
    ok(magic != 0, "got %lx\n", magic);
    pLdrUnlockLoaderLock(0, magic);
}

static void test_RtlGetCompressionWorkSpaceSize(void)
{
    ULONG compress_workspace, decompress_workspace;
    NTSTATUS status;

    if (!pRtlGetCompressionWorkSpaceSize)
    {
        win_skip("RtlGetCompressionWorkSpaceSize is not available\n");
        return;
    }

    status = pRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_NONE, &compress_workspace,
                                             &decompress_workspace);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08x\n", status);

    status = pRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_DEFAULT, &compress_workspace,
                                             &decompress_workspace);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08x\n", status);

    status = pRtlGetCompressionWorkSpaceSize(0xFF, &compress_workspace, &decompress_workspace);
    ok(status == STATUS_UNSUPPORTED_COMPRESSION, "got wrong status 0x%08x\n", status);

    compress_workspace = decompress_workspace = 0xdeadbeef;
    status = pRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1, &compress_workspace,
                                             &decompress_workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    ok(compress_workspace != 0, "got wrong compress_workspace %d\n", compress_workspace);
    ok(decompress_workspace == 0x1000, "got wrong decompress_workspace %d\n", decompress_workspace);

    compress_workspace = decompress_workspace = 0xdeadbeef;
    status = pRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM,
                                             &compress_workspace, &decompress_workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    ok(compress_workspace != 0, "got wrong compress_workspace %d\n", compress_workspace);
    ok(decompress_workspace == 0x1000, "got wrong decompress_workspace %d\n", decompress_workspace);
}

/* helper for test_RtlDecompressBuffer, checks if a chunk is incomplete */
static BOOL is_incomplete_chunk(const UCHAR *compressed, ULONG compressed_size, BOOL check_all)
{
    ULONG chunk_size;
    if (compressed_size <= sizeof(WORD))
        return TRUE;
    while (compressed_size >= sizeof(WORD))
    {
        chunk_size = (*(WORD *)compressed & 0xFFF) + 1;
        if (compressed_size < sizeof(WORD) + chunk_size)
            return TRUE;
        if (!check_all)
            break;
        compressed      += sizeof(WORD) + chunk_size;
        compressed_size -= sizeof(WORD) + chunk_size;
    }
    return FALSE;
}

#define DECOMPRESS_BROKEN_TRUNCATED    1
#define DECOMPRESS_BROKEN_FRAGMENT     2

static void test_RtlDecompressBuffer(void)
{
    static const UCHAR test_multiple_chunks[] = {0x03, 0x30, 'W', 'i', 'n', 'e',
                                                 0x03, 0x30, 'W', 'i', 'n', 'e'};
    static const struct
    {
        UCHAR compressed[32];
        ULONG compressed_size;
        NTSTATUS status;
        UCHAR uncompressed[32];
        ULONG uncompressed_size;
        DWORD broken_flags;
    }
    test_lznt[] =
    {
        /* 4 byte uncompressed chunk */
        {
            {0x03, 0x30, 'W', 'i', 'n', 'e'},
            6,
            STATUS_SUCCESS,
            "Wine",
            4,
            DECOMPRESS_BROKEN_FRAGMENT
        },
        /* 8 byte uncompressed chunk */
        {
            {0x07, 0x30, 'W', 'i', 'n', 'e', 'W', 'i', 'n', 'e'},
            10,
            STATUS_SUCCESS,
            "WineWine",
            8,
            DECOMPRESS_BROKEN_FRAGMENT
        },
        /* 4 byte compressed chunk */
        {
            {0x04, 0xB0, 0x00, 'W', 'i', 'n', 'e'},
            7,
            STATUS_SUCCESS,
            "Wine",
            4
        },
        /* 8 byte compressed chunk */
        {
            {0x08, 0xB0, 0x00, 'W', 'i', 'n', 'e', 'W', 'i', 'n', 'e'},
            11,
            STATUS_SUCCESS,
            "WineWine",
            8
        },
        /* compressed chunk using backwards reference */
        {
            {0x06, 0xB0, 0x10, 'W', 'i', 'n', 'e', 0x01, 0x30},
            9,
            STATUS_SUCCESS,
            "WineWine",
            8,
            DECOMPRESS_BROKEN_TRUNCATED
        },
        /* compressed chunk using backwards reference with length > bytes_read */
        {
            {0x06, 0xB0, 0x10, 'W', 'i', 'n', 'e', 0x05, 0x30},
            9,
            STATUS_SUCCESS,
            "WineWineWine",
            12,
            DECOMPRESS_BROKEN_TRUNCATED
        },
        /* same as above, but unused bits != 0 */
        {
            {0x06, 0xB0, 0x30, 'W', 'i', 'n', 'e', 0x01, 0x30},
            9,
            STATUS_SUCCESS,
            "WineWine",
            8,
            DECOMPRESS_BROKEN_TRUNCATED
        },
        /* compressed chunk without backwards reference and unused bits != 0 */
        {
            {0x01, 0xB0, 0x02, 'W'},
            4,
            STATUS_SUCCESS,
            "W",
            1
        },
        /* termination sequence after first chunk */
        {
            {0x03, 0x30, 'W', 'i', 'n', 'e', 0x00, 0x00, 0x03, 0x30, 'W', 'i', 'n', 'e'},
            14,
            STATUS_SUCCESS,
            "Wine",
            4,
            DECOMPRESS_BROKEN_FRAGMENT
        },
        /* compressed chunk using backwards reference with 4 bit offset, 12 bit length */
        {
            {0x14, 0xB0, 0x00, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                         0x00, 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                         0x01, 0x01, 0xF0},
            23,
            STATUS_SUCCESS,
            "ABCDEFGHIJKLMNOPABCD",
            20,
            DECOMPRESS_BROKEN_TRUNCATED
        },
        /* compressed chunk using backwards reference with 5 bit offset, 11 bit length */
        {
            {0x15, 0xB0, 0x00, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                         0x00, 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                         0x02, 'A', 0x00, 0x78},
            24,
            STATUS_SUCCESS,
            "ABCDEFGHIJKLMNOPABCD",
            20,
            DECOMPRESS_BROKEN_TRUNCATED
        },
        /* uncompressed chunk with invalid magic */
        {
            {0x03, 0x20, 'W', 'i', 'n', 'e'},
            6,
            STATUS_SUCCESS,
            "Wine",
            4,
            DECOMPRESS_BROKEN_FRAGMENT
        },
        /* compressed chunk with invalid magic */
        {
            {0x04, 0xA0, 0x00, 'W', 'i', 'n', 'e'},
            7,
            STATUS_SUCCESS,
            "Wine",
            4
        },
        /* garbage byte after end of buffer */
        {
            {0x00, 0xB0, 0x02, 0x01},
            4,
            STATUS_SUCCESS,
            "",
            0
        },
        /* empty compressed chunk */
        {
            {0x00, 0xB0, 0x00},
            3,
            STATUS_SUCCESS,
            "",
            0
        },
        /* empty compressed chunk with unused bits != 0 */
        {
            {0x00, 0xB0, 0x01},
            3,
            STATUS_SUCCESS,
            "",
            0
        },
        /* empty input buffer */
        {
            {0},
            0,
            STATUS_BAD_COMPRESSION_BUFFER,
        },
        /* incomplete chunk header */
        {
            {0x01},
            1,
            STATUS_BAD_COMPRESSION_BUFFER
        },
        /* incomplete chunk header */
        {
            {0x00, 0x30},
            2,
            STATUS_BAD_COMPRESSION_BUFFER
        },
        /* compressed chunk with invalid backwards reference */
        {
            {0x06, 0xB0, 0x10, 'W', 'i', 'n', 'e', 0x05, 0x40},
            9,
            STATUS_BAD_COMPRESSION_BUFFER
        },
        /* compressed chunk with incomplete backwards reference */
        {
            {0x05, 0xB0, 0x10, 'W', 'i', 'n', 'e', 0x05},
            8,
            STATUS_BAD_COMPRESSION_BUFFER
        },
        /* incomplete uncompressed chunk */
        {
            {0x07, 0x30, 'W', 'i', 'n', 'e'},
            6,
            STATUS_BAD_COMPRESSION_BUFFER
        },
        /* incomplete compressed chunk */
        {
            {0x08, 0xB0, 0x00, 'W', 'i', 'n', 'e'},
            7,
            STATUS_BAD_COMPRESSION_BUFFER
        },
        /* two compressed chunks, the second one incomplete */
        {
            {0x00, 0xB0, 0x02, 0x00, 0xB0},
            5,
            STATUS_BAD_COMPRESSION_BUFFER,
        }
    };

    static UCHAR buf[0x2000], workspace[0x1000];
    NTSTATUS status, expected_status;
    ULONG final_size;
    int i;

    if (!pRtlDecompressBuffer || !pRtlDecompressFragment)
    {
        win_skip("RtlDecompressBuffer or RtlDecompressFragment is not available\n");
        return;
    }

    /* test compression format / engine */
    final_size = 0xdeadbeef;
    status = pRtlDecompressBuffer(COMPRESSION_FORMAT_NONE, buf, sizeof(buf) - 1, test_lznt[0].compressed,
                                  test_lznt[0].compressed_size, &final_size);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08x\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %d\n", final_size);

    final_size = 0xdeadbeef;
    status = pRtlDecompressBuffer(COMPRESSION_FORMAT_DEFAULT, buf, sizeof(buf) - 1, test_lznt[0].compressed,
                                  test_lznt[0].compressed_size, &final_size);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08x\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %d\n", final_size);

    final_size = 0xdeadbeef;
    status = pRtlDecompressBuffer(0xFF, buf, sizeof(buf) - 1, test_lznt[0].compressed,
                                  test_lznt[0].compressed_size, &final_size);
    ok(status == STATUS_UNSUPPORTED_COMPRESSION, "got wrong status 0x%08x\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %d\n", final_size);

    /* regular tests for RtlDecompressBuffer */
    for (i = 0; i < sizeof(test_lznt) / sizeof(test_lznt[0]); i++)
    {
        trace("Running test %d (compressed_size=%d, compressed_size=%d, status=%d)\n",
              i, test_lznt[i].compressed_size, test_lznt[i].compressed_size, test_lznt[i].status);

        /* test with very big buffer */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                      test_lznt[i].compressed_size, &final_size);
        ok(status == test_lznt[i].status || broken(status == STATUS_BAD_COMPRESSION_BUFFER &&
           (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)), "%d: got wrong status 0x%08x\n", i, status);
        if (!status)
        {
            ok(final_size == test_lznt[i].uncompressed_size,
               "%d: got wrong final_size %d\n", i, final_size);
            ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size),
               "%d: got wrong decoded data\n", i);
            ok(buf[test_lznt[i].uncompressed_size] == 0x11,
               "%d: buf[%d] overwritten\n", i, test_lznt[i].uncompressed_size);
        }

        /* test that modifier for compression engine is ignored */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM, buf, sizeof(buf),
                                      test_lznt[i].compressed, test_lznt[i].compressed_size, &final_size);
        ok(status == test_lznt[i].status || broken(status == STATUS_BAD_COMPRESSION_BUFFER &&
           (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)), "%d: got wrong status 0x%08x\n", i, status);
        if (!status)
        {
            ok(final_size == test_lznt[i].uncompressed_size,
               "%d: got wrong final_size %d\n", i, final_size);
            ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size),
               "%d: got wrong decoded data\n", i);
            ok(buf[test_lznt[i].uncompressed_size] == 0x11,
               "%d: buf[%d] overwritten\n", i, test_lznt[i].uncompressed_size);
        }

        /* test with expected output size */
        if (test_lznt[i].uncompressed_size > 0)
        {
            final_size = 0xdeadbeef;
            memset(buf, 0x11, sizeof(buf));
            status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, test_lznt[i].uncompressed_size,
                                          test_lznt[i].compressed, test_lznt[i].compressed_size, &final_size);
            ok(status == test_lznt[i].status, "%d: got wrong status 0x%08x\n", i, status);
            if (!status)
            {
                ok(final_size == test_lznt[i].uncompressed_size,
                   "%d: got wrong final_size %d\n", i, final_size);
                ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size),
                   "%d: got wrong decoded data\n", i);
                ok(buf[test_lznt[i].uncompressed_size] == 0x11,
                   "%d: buf[%d] overwritten\n", i, test_lznt[i].uncompressed_size);
            }
        }

        /* test with smaller output size */
        if (test_lznt[i].uncompressed_size > 1)
        {
            final_size = 0xdeadbeef;
            memset(buf, 0x11, sizeof(buf));
            status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, test_lznt[i].uncompressed_size - 1,
                                          test_lznt[i].compressed, test_lznt[i].compressed_size, &final_size);
            ok(status == test_lznt[i].status || broken(status == STATUS_BAD_COMPRESSION_BUFFER &&
               (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_TRUNCATED)), "%d: got wrong status 0x%08x\n", i, status);
            if (!status)
            {
                ok(final_size == test_lznt[i].uncompressed_size - 1,
                   "%d: got wrong final_size %d\n", i, final_size);
                ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size - 1),
                   "%d: got wrong decoded data\n", i);
                ok(buf[test_lznt[i].uncompressed_size - 1] == 0x11,
                   "%d: buf[%d] overwritten\n", i, test_lznt[i].uncompressed_size - 1);
            }
        }

        /* test with zero output size */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, 0, test_lznt[i].compressed,
                                      test_lznt[i].compressed_size, &final_size);
        if (is_incomplete_chunk(test_lznt[i].compressed, test_lznt[i].compressed_size, FALSE))
        {
            ok(status == STATUS_BAD_COMPRESSION_BUFFER, "%d: got wrong status 0x%08x\n", i, status);
        }
        else
        {
            ok(status == STATUS_SUCCESS, "%d: got wrong status 0x%08x\n", i, status);
            ok(final_size == 0, "%d: got wrong final_size %d\n", i, final_size);
            ok(buf[0] == 0x11, "%d: buf[%d] overwritten\n", i, test_lznt[i].uncompressed_size);
        }

        /* test RtlDecompressBuffer with offset = 0 */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                        test_lznt[i].compressed_size, 0, &final_size, workspace);
        ok(status == test_lznt[i].status || broken(status == STATUS_BAD_COMPRESSION_BUFFER &&
           (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)), "%d: got wrong status 0x%08x\n", i, status);
        if (!status)
        {
            ok(final_size == test_lznt[i].uncompressed_size,
               "%d: got wrong final_size %d\n", i, final_size);
            ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size),
               "%d: got wrong decoded data\n", i);
            ok(buf[test_lznt[i].uncompressed_size] == 0x11,
               "%d: buf[%d] overwritten\n", i, test_lznt[i].uncompressed_size);
        }

        /* test RtlDecompressBuffer with offset = 1 */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                        test_lznt[i].compressed_size, 1, &final_size, workspace);
        ok(status == test_lznt[i].status || broken(status == STATUS_BAD_COMPRESSION_BUFFER &&
           (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)), "%d: got wrong status 0x%08x\n", i, status);
        if (!status)
        {
            if (test_lznt[i].uncompressed_size == 0)
            {
                todo_wine
                ok(final_size == 4095,
                   "%d: got wrong final size %d\n", i, final_size);
                /* Buffer doesn't contain any useful value on Windows */
                ok(buf[4095] == 0x11,
                   "%d: buf[4095] overwritten\n", i);
            }
            else
            {
                ok(final_size == test_lznt[i].uncompressed_size - 1,
                   "%d: got wrong final_size %d\n", i, final_size);
                ok(!memcmp(buf, test_lznt[i].uncompressed + 1, test_lznt[i].uncompressed_size - 1),
                   "%d: got wrong decoded data\n", i);
                ok(buf[test_lznt[i].uncompressed_size - 1] == 0x11,
                   "%d: buf[%d] overwritten\n", i, test_lznt[i].uncompressed_size - 1);
            }
        }

        /* test RtlDecompressBuffer with offset = 4095 */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                        test_lznt[i].compressed_size, 4095, &final_size, workspace);
        ok(status == test_lznt[i].status || broken(status == STATUS_BAD_COMPRESSION_BUFFER &&
           (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)), "%d: got wrong status 0x%08x\n", i, status);
        if (!status)
        {
            todo_wine
            ok(final_size == 1,
               "%d: got wrong final size %d\n", i, final_size);
            todo_wine
            ok(buf[0] == 0,
               "%d: padding is not zero\n", i);
            ok(buf[1] == 0x11,
               "%d: buf[1] overwritten\n", i);
        }

        /* test RtlDecompressBuffer with offset = 4096 */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                        test_lznt[i].compressed_size, 4096, &final_size, workspace);
        expected_status = is_incomplete_chunk(test_lznt[i].compressed, test_lznt[i].compressed_size, TRUE) ?
                          test_lznt[i].status : STATUS_SUCCESS;
        ok(status == expected_status, "%d: got wrong status 0x%08x, expected 0x%08x\n", i, status, expected_status);
        if (!status)
        {
            ok(final_size == 0,
               "%d: got wrong final size %d\n", i, final_size);
            ok(buf[0] == 0x11,
               "%d: buf[4096] overwritten\n", i);
        }
    }

    /* test decoding of multiple chunks with pRtlDecompressBuffer */
    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_multiple_chunks,
                                  sizeof(test_multiple_chunks), &final_size);
    ok(status == STATUS_SUCCESS || broken(status == STATUS_BAD_COMPRESSION_BUFFER),
       "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 4100, "got wrong final_size %d\n", final_size);
        ok(!memcmp(buf, "Wine", 4), "got wrong decoded data at offset 0\n");
        ok(buf[4] == 0 && buf[4095] == 0, "padding is not zero\n");
        ok(!memcmp(buf + 4096, "Wine", 4), "got wrong decoded data at offset 4096\n");
        ok(buf[4100] == 0x11, "buf[4100] overwritten\n");
    }

    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, 4097, test_multiple_chunks,
                                  sizeof(test_multiple_chunks), &final_size);
    ok(status == STATUS_SUCCESS || broken(status == STATUS_BAD_COMPRESSION_BUFFER),
       "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 4097, "got wrong final_size %d\n", final_size);
        ok(!memcmp(buf, "Wine", 4), "got wrong decoded data at offset 0\n");
        ok(buf[4] == 0 && buf[4095] == 0, "padding is not zero\n");
        ok(buf[4096], "got wrong decoded data at offset 4096\n");
        ok(buf[4097] == 0x11, "buf[4097] overwritten\n");
    }

    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, 4096, test_multiple_chunks,
                                  sizeof(test_multiple_chunks), &final_size);
    ok(status == STATUS_SUCCESS || broken(status == STATUS_BAD_COMPRESSION_BUFFER),
       "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 4, "got wrong final_size %d\n", final_size);
        ok(!memcmp(buf, "Wine", 4), "got wrong decoded data at offset 0\n");
        ok(buf[4] == 0x11, "buf[4] overwritten\n");
    }

    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, 4, test_multiple_chunks,
                                  sizeof(test_multiple_chunks), &final_size);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 4, "got wrong final_size %d\n", final_size);
        ok(!memcmp(buf, "Wine", 4), "got wrong decoded data at offset 0\n");
        ok(buf[4] == 0x11, "buf[4] overwritten\n");
    }

    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, 3, test_multiple_chunks,
                                  sizeof(test_multiple_chunks), &final_size);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 3, "got wrong final_size %d\n", final_size);
        ok(!memcmp(buf, "Wine", 3), "got wrong decoded data at offset 0\n");
        ok(buf[3] == 0x11, "buf[3] overwritten\n");
    }

    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, 0, test_multiple_chunks,
                                  sizeof(test_multiple_chunks), &final_size);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 0, "got wrong final_size %d\n", final_size);
        ok(buf[0] == 0x11, "buf[0] overwritten\n");
    }

    /* test multiple chunks in combination with RtlDecompressBuffer and offset=1 */
    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_multiple_chunks,
                                    sizeof(test_multiple_chunks), 1, &final_size, workspace);
    ok(status == STATUS_SUCCESS || broken(status == STATUS_BAD_COMPRESSION_BUFFER),
       "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 4099, "got wrong final_size %d\n", final_size);
        ok(!memcmp(buf, "ine", 3), "got wrong decoded data at offset 0\n");
        ok(buf[3] == 0 && buf[4094] == 0, "padding is not zero\n");
        ok(!memcmp(buf + 4095, "Wine", 4), "got wrong decoded data at offset 4095\n");
        ok(buf[4099] == 0x11, "buf[4099] overwritten\n");
    }

    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, 4096, test_multiple_chunks,
                                    sizeof(test_multiple_chunks), 1, &final_size, workspace);
    ok(status == STATUS_SUCCESS || broken(status == STATUS_BAD_COMPRESSION_BUFFER),
       "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 4096, "got wrong final_size %d\n", final_size);
        ok(!memcmp(buf, "ine", 3), "got wrong decoded data at offset 0\n");
        ok(buf[3] == 0 && buf[4094] == 0, "padding is not zero\n");
        ok(buf[4095] == 'W', "got wrong decoded data at offset 4095\n");
        ok(buf[4096] == 0x11, "buf[4096] overwritten\n");
    }

    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, 4095, test_multiple_chunks,
                                    sizeof(test_multiple_chunks), 1, &final_size, workspace);
    ok(status == STATUS_SUCCESS || broken(status == STATUS_BAD_COMPRESSION_BUFFER),
       "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 3, "got wrong final_size %d\n", final_size);
        ok(!memcmp(buf, "ine", 3), "got wrong decoded data at offset 0\n");
        ok(buf[4] == 0x11, "buf[4] overwritten\n");
    }

    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, 3, test_multiple_chunks,
                                    sizeof(test_multiple_chunks), 1, &final_size, workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 3, "got wrong final_size %d\n", final_size);
        ok(!memcmp(buf, "ine", 3), "got wrong decoded data at offset 0\n");
        ok(buf[3] == 0x11, "buf[3] overwritten\n");
    }

    /* test multiple chunks in combination with RtlDecompressBuffer and offset=4 */
    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_multiple_chunks,
                                    sizeof(test_multiple_chunks), 4, &final_size, workspace);
    ok(status == STATUS_SUCCESS || broken(status == STATUS_BAD_COMPRESSION_BUFFER),
       "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 4096, "got wrong final_size %d\n", final_size);
        ok(buf[0] == 0 && buf[4091] == 0, "padding is not zero\n");
        ok(!memcmp(buf + 4092, "Wine", 4), "got wrong decoded data at offset 4092\n");
        ok(buf[4096] == 0x11, "buf[4096] overwritten\n");
    }

    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_multiple_chunks,
                                    sizeof(test_multiple_chunks), 4095, &final_size, workspace);
    ok(status == STATUS_SUCCESS || broken(status == STATUS_BAD_COMPRESSION_BUFFER),
       "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 5, "got wrong final_size %d\n", final_size);
        ok(buf[0] == 0, "padding is not zero\n");
        ok(!memcmp(buf + 1, "Wine", 4), "got wrong decoded data at offset 1\n");
        ok(buf[5] == 0x11, "buf[5] overwritten\n");
    }

    final_size = 0xdeadbeef;
    memset(buf, 0x11, sizeof(buf));
    status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_multiple_chunks,
                                    sizeof(test_multiple_chunks), 4096, &final_size, workspace);
    ok(status == STATUS_SUCCESS || broken(status == STATUS_BAD_COMPRESSION_BUFFER),
       "got wrong status 0x%08x\n", status);
    if (!status)
    {
        ok(final_size == 4, "got wrong final_size %d\n", final_size);
        ok(!memcmp(buf, "Wine", 4), "got wrong decoded data at offset 0\n");
        ok(buf[4] == 0x11, "buf[4] overwritten\n");
    }

}

static void test_RtlCompressBuffer(void)
{
    ULONG compress_workspace, decompress_workspace;
    static const UCHAR test_buffer[] = "WineWineWine";
    static UCHAR buf1[0x1000], buf2[0x1000], *workspace;
    ULONG final_size, buf_size;
    NTSTATUS status;

    if (!pRtlCompressBuffer || !pRtlGetCompressionWorkSpaceSize)
    {
        win_skip("RtlCompressBuffer or RtlGetCompressionWorkSpaceSize is not available\n");
        return;
    }

    compress_workspace = decompress_workspace = 0xdeadbeef;
    status = pRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1, &compress_workspace,
                                             &decompress_workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    ok(compress_workspace != 0, "got wrong compress_workspace %d\n", compress_workspace);

    workspace = HeapAlloc( GetProcessHeap(), 0, compress_workspace );
    ok(workspace != NULL, "HeapAlloc failed %x\n", GetLastError());

    /* test compression format / engine */
    final_size = 0xdeadbeef;
    status = pRtlCompressBuffer(COMPRESSION_FORMAT_NONE, test_buffer, sizeof(test_buffer),
                                buf1, sizeof(buf1) - 1, 4096, &final_size, workspace);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08x\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %d\n", final_size);

    final_size = 0xdeadbeef;
    status = pRtlCompressBuffer(COMPRESSION_FORMAT_DEFAULT, test_buffer, sizeof(test_buffer),
                                buf1, sizeof(buf1) - 1, 4096, &final_size, workspace);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08x\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %d\n", final_size);

    final_size = 0xdeadbeef;
    status = pRtlCompressBuffer(0xFF, test_buffer, sizeof(test_buffer),
                                buf1, sizeof(buf1) - 1, 4096, &final_size, workspace);
    ok(status == STATUS_UNSUPPORTED_COMPRESSION, "got wrong status 0x%08x\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %d\n", final_size);

    /* test compression */
    final_size = 0xdeadbeef;
    memset(buf1, 0x11, sizeof(buf1));
    status = pRtlCompressBuffer(COMPRESSION_FORMAT_LZNT1, test_buffer, sizeof(test_buffer),
                                buf1, sizeof(buf1), 4096, &final_size, workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    ok((*(WORD *)buf1 & 0x7000) == 0x3000, "no chunk signature found %04x\n", *(WORD *)buf1);
    buf_size = final_size;
    todo_wine
    ok(final_size < sizeof(test_buffer), "got wrong final_size %d\n", final_size);

    /* test decompression */
    final_size = 0xdeadbeef;
    memset(buf2, 0x11, sizeof(buf2));
    status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf2, sizeof(buf2),
                                  buf1, buf_size, &final_size);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    ok(final_size == sizeof(test_buffer), "got wrong final_size %d\n", final_size);
    ok(!memcmp(buf2, test_buffer, sizeof(test_buffer)), "got wrong decoded data\n");
    ok(buf2[sizeof(test_buffer)] == 0x11, "buf[%u] overwritten\n", (DWORD)sizeof(test_buffer));

    /* buffer too small */
    final_size = 0xdeadbeef;
    memset(buf1, 0x11, sizeof(buf1));
    status = pRtlCompressBuffer(COMPRESSION_FORMAT_LZNT1, test_buffer, sizeof(test_buffer),
                                buf1, 4, 4096, &final_size, workspace);
    ok(status == STATUS_BUFFER_TOO_SMALL, "got wrong status 0x%08x\n", status);

    HeapFree(GetProcessHeap(), 0, workspace);
}

START_TEST(rtl)
{
    InitFunctionPtrs();

#ifdef __i386__
    test_WinSqm();
#else
    skip("stdcall-style parameter checks are not supported on this platform.\n");
#endif

    test_RtlCompareMemory();
    test_RtlCompareMemoryUlong();
    test_RtlMoveMemory();
    test_RtlFillMemory();
    test_RtlFillMemoryUlong();
    test_RtlZeroMemory();
    test_RtlUlonglongByteSwap();
    test_RtlUniform();
    test_RtlRandom();
    test_RtlAreAllAccessesGranted();
    test_RtlAreAnyAccessesGranted();
    test_RtlComputeCrc32();
    test_HandleTables();
    test_RtlAllocateAndInitializeSid();
    test_RtlDeleteTimer();
    test_RtlThreadErrorMode();
    test_LdrProcessRelocationBlock();
    test_RtlIpv4AddressToString();
    test_RtlIpv4AddressToStringEx();
    test_RtlIpv4StringToAddress();
    test_RtlIpv4StringToAddressEx();
    test_LdrAddRefDll();
    test_LdrLockLoaderLock();
    test_RtlGetCompressionWorkSpaceSize();
    test_RtlDecompressBuffer();
    test_RtlCompressBuffer();
}
