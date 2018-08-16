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
#include "in6addr.h"
#include "initguid.h"
#define COBJMACROS
#ifdef __REACTOS__
#include <objbase.h>
#endif
#include "shobjidl.h"

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
static DWORD     (WINAPI *pRtlGetThreadErrorMode)(void);
static NTSTATUS  (WINAPI *pRtlSetThreadErrorMode)(DWORD, LPDWORD);
static IMAGE_BASE_RELOCATION *(WINAPI *pLdrProcessRelocationBlock)(void*,UINT,USHORT*,INT_PTR);
static CHAR *    (WINAPI *pRtlIpv4AddressToStringA)(const IN_ADDR *, LPSTR);
static NTSTATUS  (WINAPI *pRtlIpv4AddressToStringExA)(const IN_ADDR *, USHORT, LPSTR, PULONG);
static NTSTATUS  (WINAPI *pRtlIpv4StringToAddressA)(PCSTR, BOOLEAN, PCSTR *, IN_ADDR *);
static NTSTATUS  (WINAPI *pRtlIpv4StringToAddressExA)(PCSTR, BOOLEAN, IN_ADDR *, PUSHORT);
static CHAR *    (WINAPI *pRtlIpv6AddressToStringA)(struct in6_addr *, PSTR);
static NTSTATUS  (WINAPI *pRtlIpv6AddressToStringExA)(struct in6_addr *, ULONG, USHORT, PCHAR, PULONG);
static NTSTATUS  (WINAPI *pRtlIpv6StringToAddressA)(PCSTR, PCSTR *, struct in6_addr *);
static NTSTATUS  (WINAPI *pRtlIpv6StringToAddressW)(PCWSTR, PCWSTR *, struct in6_addr *);
static NTSTATUS  (WINAPI *pRtlIpv6StringToAddressExA)(PCSTR, struct in6_addr *, PULONG, PUSHORT);
static NTSTATUS  (WINAPI *pRtlIpv6StringToAddressExW)(PCWSTR, struct in6_addr *, PULONG, PUSHORT);
static NTSTATUS  (WINAPI *pLdrAddRefDll)(ULONG, HMODULE);
static NTSTATUS  (WINAPI *pLdrLockLoaderLock)(ULONG, ULONG*, ULONG_PTR*);
static NTSTATUS  (WINAPI *pLdrUnlockLoaderLock)(ULONG, ULONG_PTR);
static NTSTATUS  (WINAPI *pRtlMultiByteToUnicodeN)(LPWSTR, DWORD, LPDWORD, LPCSTR, DWORD);
static NTSTATUS  (WINAPI *pRtlGetCompressionWorkSpaceSize)(USHORT, PULONG, PULONG);
static NTSTATUS  (WINAPI *pRtlDecompressBuffer)(USHORT, PUCHAR, ULONG, const UCHAR*, ULONG, PULONG);
static NTSTATUS  (WINAPI *pRtlDecompressFragment)(USHORT, PUCHAR, ULONG, const UCHAR*, ULONG, ULONG, PULONG, PVOID);
static NTSTATUS  (WINAPI *pRtlCompressBuffer)(USHORT, const UCHAR*, ULONG, PUCHAR, ULONG, ULONG, PULONG, PVOID);
static BOOL      (WINAPI *pRtlIsCriticalSectionLocked)(CRITICAL_SECTION *);
static BOOL      (WINAPI *pRtlIsCriticalSectionLockedByThread)(CRITICAL_SECTION *);
static NTSTATUS  (WINAPI *pRtlInitializeCriticalSectionEx)(CRITICAL_SECTION *, ULONG, ULONG);
static NTSTATUS  (WINAPI *pLdrEnumerateLoadedModules)(void *, void *, void *);
static NTSTATUS  (WINAPI *pRtlQueryPackageIdentity)(HANDLE, WCHAR*, SIZE_T*, WCHAR*, SIZE_T*, BOOLEAN*);
static NTSTATUS  (WINAPI *pRtlMakeSelfRelativeSD)(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,LPDWORD);
static NTSTATUS  (WINAPI *pRtlAbsoluteToSelfRelativeSD)(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,PULONG);
static NTSTATUS  (WINAPI *pLdrRegisterDllNotification)(ULONG, PLDR_DLL_NOTIFICATION_FUNCTION, void *, void **);
static NTSTATUS  (WINAPI *pLdrUnregisterDllNotification)(void *);

static HMODULE hkernel32 = 0;
static BOOL      (WINAPI *pIsWow64Process)(HANDLE, PBOOL);


#define LEN 16
static const char* src_src = "This is a test!"; /* 16 bytes long, incl NUL */
static WCHAR ws2_32dllW[] = {'w','s','2','_','3','2','.','d','l','l',0};
static WCHAR wintrustdllW[] = {'w','i','n','t','r','u','s','t','.','d','l','l',0};
static WCHAR crypt32dllW[] = {'c','r','y','p','t','3','2','.','d','l','l',0};
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
        pRtlGetThreadErrorMode = (void *)GetProcAddress(hntdll, "RtlGetThreadErrorMode");
        pRtlSetThreadErrorMode = (void *)GetProcAddress(hntdll, "RtlSetThreadErrorMode");
        pLdrProcessRelocationBlock  = (void *)GetProcAddress(hntdll, "LdrProcessRelocationBlock");
        pRtlIpv4AddressToStringA = (void *)GetProcAddress(hntdll, "RtlIpv4AddressToStringA");
        pRtlIpv4AddressToStringExA = (void *)GetProcAddress(hntdll, "RtlIpv4AddressToStringExA");
        pRtlIpv4StringToAddressA = (void *)GetProcAddress(hntdll, "RtlIpv4StringToAddressA");
        pRtlIpv4StringToAddressExA = (void *)GetProcAddress(hntdll, "RtlIpv4StringToAddressExA");
        pRtlIpv6AddressToStringA = (void *)GetProcAddress(hntdll, "RtlIpv6AddressToStringA");
        pRtlIpv6AddressToStringExA = (void *)GetProcAddress(hntdll, "RtlIpv6AddressToStringExA");
        pRtlIpv6StringToAddressA = (void *)GetProcAddress(hntdll, "RtlIpv6StringToAddressA");
        pRtlIpv6StringToAddressW = (void *)GetProcAddress(hntdll, "RtlIpv6StringToAddressW");
        pRtlIpv6StringToAddressExA = (void *)GetProcAddress(hntdll, "RtlIpv6StringToAddressExA");
        pRtlIpv6StringToAddressExW = (void *)GetProcAddress(hntdll, "RtlIpv6StringToAddressExW");
        pLdrAddRefDll = (void *)GetProcAddress(hntdll, "LdrAddRefDll");
        pLdrLockLoaderLock = (void *)GetProcAddress(hntdll, "LdrLockLoaderLock");
        pLdrUnlockLoaderLock = (void *)GetProcAddress(hntdll, "LdrUnlockLoaderLock");
        pRtlMultiByteToUnicodeN = (void *)GetProcAddress(hntdll, "RtlMultiByteToUnicodeN");
        pRtlGetCompressionWorkSpaceSize = (void *)GetProcAddress(hntdll, "RtlGetCompressionWorkSpaceSize");
        pRtlDecompressBuffer = (void *)GetProcAddress(hntdll, "RtlDecompressBuffer");
        pRtlDecompressFragment = (void *)GetProcAddress(hntdll, "RtlDecompressFragment");
        pRtlCompressBuffer = (void *)GetProcAddress(hntdll, "RtlCompressBuffer");
        pRtlIsCriticalSectionLocked = (void *)GetProcAddress(hntdll, "RtlIsCriticalSectionLocked");
        pRtlIsCriticalSectionLockedByThread = (void *)GetProcAddress(hntdll, "RtlIsCriticalSectionLockedByThread");
        pRtlInitializeCriticalSectionEx = (void *)GetProcAddress(hntdll, "RtlInitializeCriticalSectionEx");
        pLdrEnumerateLoadedModules = (void *)GetProcAddress(hntdll, "LdrEnumerateLoadedModules");
        pRtlQueryPackageIdentity = (void *)GetProcAddress(hntdll, "RtlQueryPackageIdentity");
        pRtlMakeSelfRelativeSD = (void *)GetProcAddress(hntdll, "RtlMakeSelfRelativeSD");
        pRtlAbsoluteToSelfRelativeSD = (void *)GetProcAddress(hntdll, "RtlAbsoluteToSelfRelativeSD");
        pLdrRegisterDllNotification = (void *)GetProcAddress(hntdll, "LdrRegisterDllNotification");
        pLdrUnregisterDllNotification = (void *)GetProcAddress(hntdll, "LdrUnregisterDllNotification");
    }
    hkernel32 = LoadLibraryA("kernel32.dll");
    ok(hkernel32 != 0, "LoadLibrary failed\n");
    if (hkernel32) {
        pIsWow64Process = (void *)GetProcAddress(hkernel32, "IsWow64Process");
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
       "RtlUlonglongByteSwap(0x7654321087654321) returns 0x%s, expected 0x2143658710325476\n",
       wine_dbgstr_longlong(result));
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
                "test: 0x%s RtlUniform(&seed (seed == %x)) returns %x, expected %x\n",
                wine_dbgstr_longlong(num), seed_bak, result, expected);
        ok(seed == expected,
                "test: 0x%s RtlUniform(&seed (seed == %x)) sets seed to %x, expected %x\n",
                wine_dbgstr_longlong(num), seed_bak, result, expected);
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
                "test: 0x%s RtlUniform(&seed (seed == %x)) returns %x, expected %x\n",
                wine_dbgstr_longlong(num), seed_bak, result, expected);
        ok(seed == expected,
                "test: 0x%s RtlUniform(&seed (seed == %x)) sets seed to %x, expected %x\n",
                wine_dbgstr_longlong(num), seed_bak, result, expected);
    } /* for */
/*
 * More tests show that RtlUniform does not return 0x7ffffffd for seed values
 * in the range [0..MAXLONG-1]. Additionally 2 is returned twice. This shows
 * that there is more than one cycle of generated randon numbers ...
 */
}


static void test_RtlRandom(void)
{
    int i, j;
    ULONG seed;
    ULONG res[512];

    if (!pRtlRandom)
    {
        win_skip("RtlRandom is not available\n");
        return;
    }

    seed = 0;
    for (i = 0; i < sizeof(res) / sizeof(res[0]); i++)
    {
        res[i] = pRtlRandom(&seed);
        ok(seed != res[i], "%i: seed is same as res %x\n", i, seed);
        for (j = 0; j < i; j++)
            ok(res[i] != res[j], "res[%i] (%x) is same as res[%i] (%x)\n", j, res[j], i, res[i]);
    }
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
    if (!is_wow64)
    {
        ok(NtCurrentTeb()->HardErrorDisabled == 0x70,
           "The TEB contains 0x%x, expected 0x%x\n",
           NtCurrentTeb()->HardErrorDisabled, 0x70);
    }

    status = pRtlSetThreadErrorMode(0, &mode);
    ok(status == STATUS_SUCCESS ||
       status == STATUS_WAIT_1, /* Vista */
       "RtlSetThreadErrorMode failed with error 0x%08x\n", status);
    ok(mode == 0x70,
       "RtlSetThreadErrorMode returned mode 0x%x, expected 0x%x\n",
       mode, 0x70);
    ok(pRtlGetThreadErrorMode() == 0,
       "RtlGetThreadErrorMode returned 0x%x, expected 0x%x\n", mode, 0);
    if (!is_wow64)
    {
        ok(NtCurrentTeb()->HardErrorDisabled == 0,
           "The TEB contains 0x%x, expected 0x%x\n",
           NtCurrentTeb()->HardErrorDisabled, 0);
    }

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

static struct
{
    PCSTR address;
    NTSTATUS res;
    int terminator_offset;
    int ip[4];
    enum { normal_4, strict_diff_4 = 1, ex_fail_4 = 2 } flags;
    NTSTATUS res_strict;
    int terminator_offset_strict;
    int ip_strict[4];
} ipv4_tests[] =
{
    { "",                       STATUS_INVALID_PARAMETER,  0, { -1 } },
    { " ",                      STATUS_INVALID_PARAMETER,  0, { -1 } },
    { "1.1.1.1",                STATUS_SUCCESS,            7, {   1,   1,   1,   1 } },
    { "0.0.0.0",                STATUS_SUCCESS,            7, {   0,   0,   0,   0 } },
    { "255.255.255.255",        STATUS_SUCCESS,           15, { 255, 255, 255, 255 } },
    { "255.255.255.255:123",    STATUS_SUCCESS,           15, { 255, 255, 255, 255 } },
    { "255.255.255.256",        STATUS_INVALID_PARAMETER, 15, { -1 } },
    { "255.255.255.4294967295", STATUS_INVALID_PARAMETER, 22, { -1 } },
    { "255.255.255.4294967296", STATUS_INVALID_PARAMETER, 21, { -1 } },
    { "255.255.255.4294967297", STATUS_INVALID_PARAMETER, 21, { -1 } },
    { "a",                      STATUS_INVALID_PARAMETER,  0, { -1 } },
    { "1.1.1.0xaA",             STATUS_SUCCESS,           10, {   1,   1,   1, 170 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  8, { -1 } },
    { "1.1.1.0XaA",             STATUS_SUCCESS,           10, {   1,   1,   1, 170 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  8, { -1 } },
    { "1.1.1.0x",               STATUS_INVALID_PARAMETER,  8, { -1 } },
    { "1.1.1.0xff",             STATUS_SUCCESS,           10, {   1,   1,   1, 255 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  8, { -1 } },
    { "1.1.1.0x100",            STATUS_INVALID_PARAMETER, 11, { -1 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  8, { -1 } },
    { "1.1.1.0xffffffff",       STATUS_INVALID_PARAMETER, 16, { -1 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  8, { -1 } },
    { "1.1.1.0x100000000",      STATUS_INVALID_PARAMETER, 16, { -1, 0, 0, 0 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  8, { -1 } },
    { "1.1.1.010",              STATUS_SUCCESS,            9, {   1,   1,   1,   8 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  7, { -1 } },
    { "1.1.1.00",               STATUS_SUCCESS,            8, {   1,   1,   1,   0 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  7, { -1 } },
    { "1.1.1.007",              STATUS_SUCCESS,            9, {   1,   1,   1,   7 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  7, { -1 } },
    { "1.1.1.08",               STATUS_INVALID_PARAMETER,  7, { -1 } },
    { "1.1.1.008",              STATUS_SUCCESS,            8, {   1,   1,   1,   0 }, strict_diff_4 | ex_fail_4,
                                STATUS_INVALID_PARAMETER,  7, { -1 } },
    { "1.1.1.0a",               STATUS_SUCCESS,            7, {   1,   1,   1,   0 }, ex_fail_4 },
    { "1.1.1.0o10",             STATUS_SUCCESS,            7, {   1,   1,   1,   0 }, ex_fail_4 },
    { "1.1.1.0b10",             STATUS_SUCCESS,            7, {   1,   1,   1,   0 }, ex_fail_4 },
    { "1.1.1.-2",               STATUS_INVALID_PARAMETER,  6, { -1 } },
    { "1",                      STATUS_SUCCESS,            1, {   0,   0,   0,   1 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  1, { -1 } },
    { "-1",                     STATUS_INVALID_PARAMETER,  0, { -1 } },
    { "203569230",              STATUS_SUCCESS,            9, {  12,  34,  56,  78 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  9, { -1 } },
    { "1.223756",               STATUS_SUCCESS,            8, {   1,   3, 106,  12 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  8, { -1 } },
    { "3.4.756",                STATUS_SUCCESS,            7, {   3,   4,   2, 244 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  7, { -1 } },
    { "3.4.756.1",              STATUS_INVALID_PARAMETER,  9, { -1 } },
    { "3.4.65536",              STATUS_INVALID_PARAMETER,  9, { -1 } },
    { "3.4.5.6.7",              STATUS_INVALID_PARAMETER,  7, { -1 } },
    { "3.4.5.+6",               STATUS_INVALID_PARAMETER,  6, { -1 } },
    { " 3.4.5.6",               STATUS_INVALID_PARAMETER,  0, { -1 } },
    { "\t3.4.5.6",              STATUS_INVALID_PARAMETER,  0, { -1 } },
    { "3.4.5.6 ",               STATUS_SUCCESS,            7, {   3,   4,   5,   6 }, ex_fail_4 },
    { "3. 4.5.6",               STATUS_INVALID_PARAMETER,  2, { -1 } },
    { ".",                      STATUS_INVALID_PARAMETER,  1, { -1 } },
    { "..",                     STATUS_INVALID_PARAMETER,  1, { -1 } },
    { "1.",                     STATUS_INVALID_PARAMETER,  2, { -1 } },
    { "1..",                    STATUS_INVALID_PARAMETER,  3, { -1 } },
    { ".1",                     STATUS_INVALID_PARAMETER,  1, { -1 } },
    { ".1.",                    STATUS_INVALID_PARAMETER,  1, { -1 } },
    { ".1.2.3",                 STATUS_INVALID_PARAMETER,  1, { -1 } },
    { "0.1.2.3",                STATUS_SUCCESS,            7, {   0,   1,   2,   3 } },
    { "0.1.2.3.",               STATUS_INVALID_PARAMETER,  7, { -1 } },
    { "[0.1.2.3]",              STATUS_INVALID_PARAMETER,  0, { -1 } },
    { "::1",                    STATUS_INVALID_PARAMETER,  0, { -1 } },
    { ":1",                     STATUS_INVALID_PARAMETER,  0, { -1 } },
};
const unsigned int ipv4_testcount = sizeof(ipv4_tests) / sizeof(ipv4_tests[0]);

static void init_ip4(IN_ADDR* addr, const int src[4])
{
    if (!src || src[0] == -1)
    {
        addr->S_un.S_addr = 0xabababab;
    }
    else
    {
        addr->S_un.S_un_b.s_b1 = src[0];
        addr->S_un.S_un_b.s_b2 = src[1];
        addr->S_un.S_un_b.s_b3 = src[2];
        addr->S_un.S_un_b.s_b4 = src[3];
    }
}

static void test_RtlIpv4StringToAddress(void)
{
    NTSTATUS res;
    IN_ADDR ip, expected_ip;
    PCSTR terminator;
    CHAR dummy;
    unsigned int i;

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

    for (i = 0; i < ipv4_testcount; i++)
    {
        /* non-strict */
        terminator = &dummy;
        ip.S_un.S_addr = 0xabababab;
        res = pRtlIpv4StringToAddressA(ipv4_tests[i].address, FALSE, &terminator, &ip);
        ok(res == ipv4_tests[i].res,
           "[%s] res = 0x%08x, expected 0x%08x\n",
           ipv4_tests[i].address, res, ipv4_tests[i].res);
        ok(terminator == ipv4_tests[i].address + ipv4_tests[i].terminator_offset,
           "[%s] terminator = %p, expected %p\n",
           ipv4_tests[i].address, terminator, ipv4_tests[i].address + ipv4_tests[i].terminator_offset);

        init_ip4(&expected_ip, ipv4_tests[i].ip);
        ok(ip.S_un.S_addr == expected_ip.S_un.S_addr,
           "[%s] ip = %08x, expected %08x\n",
           ipv4_tests[i].address, ip.S_un.S_addr, expected_ip.S_un.S_addr);

        if (!(ipv4_tests[i].flags & strict_diff_4))
        {
            ipv4_tests[i].res_strict = ipv4_tests[i].res;
            ipv4_tests[i].terminator_offset_strict = ipv4_tests[i].terminator_offset;
            ipv4_tests[i].ip_strict[0] = ipv4_tests[i].ip[0];
            ipv4_tests[i].ip_strict[1] = ipv4_tests[i].ip[1];
            ipv4_tests[i].ip_strict[2] = ipv4_tests[i].ip[2];
            ipv4_tests[i].ip_strict[3] = ipv4_tests[i].ip[3];
        }
        /* strict */
        terminator = &dummy;
        ip.S_un.S_addr = 0xabababab;
        res = pRtlIpv4StringToAddressA(ipv4_tests[i].address, TRUE, &terminator, &ip);
        ok(res == ipv4_tests[i].res_strict,
           "[%s] res = 0x%08x, expected 0x%08x\n",
           ipv4_tests[i].address, res, ipv4_tests[i].res_strict);
        ok(terminator == ipv4_tests[i].address + ipv4_tests[i].terminator_offset_strict,
           "[%s] terminator = %p, expected %p\n",
           ipv4_tests[i].address, terminator, ipv4_tests[i].address + ipv4_tests[i].terminator_offset_strict);

        init_ip4(&expected_ip, ipv4_tests[i].ip_strict);
        ok(ip.S_un.S_addr == expected_ip.S_un.S_addr,
           "[%s] ip = %08x, expected %08x\n",
           ipv4_tests[i].address, ip.S_un.S_addr, expected_ip.S_un.S_addr);
    }
}

static void test_RtlIpv4StringToAddressEx(void)
{
    NTSTATUS res;
    IN_ADDR ip, expected_ip;
    USHORT port;
    static const struct
    {
        PCSTR address;
        NTSTATUS res;
        int ip[4];
        USHORT port;
    } ipv4_ex_tests[] =
    {
        { "",               STATUS_INVALID_PARAMETER,   { -1 },         0xdead },
        { " ",              STATUS_INVALID_PARAMETER,   { -1 },         0xdead },
        { "1.1.1.1:",       STATUS_INVALID_PARAMETER,   { 1, 1, 1, 1 }, 0xdead },
        { "1.1.1.1+",       STATUS_INVALID_PARAMETER,   { 1, 1, 1, 1 }, 0xdead },
        { "1.1.1.1:1",      STATUS_SUCCESS,             { 1, 1, 1, 1 }, 0x100 },
        { "256.1.1.1:1",    STATUS_INVALID_PARAMETER,   { -1 },         0xdead },
        { "-1.1.1.1:1",     STATUS_INVALID_PARAMETER,   { -1 },         0xdead },
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
    const unsigned int ipv4_ex_testcount = sizeof(ipv4_ex_tests) / sizeof(ipv4_ex_tests[0]);
    unsigned int i;
    BOOLEAN strict;

    if (!pRtlIpv4StringToAddressExA)
    {
        skip("RtlIpv4StringToAddressEx not available\n");
        return;
    }

    /* do not crash, and do not touch the ip / port. */
    ip.S_un.S_addr = 0xabababab;
    port = 0xdead;
    res = pRtlIpv4StringToAddressExA(NULL, FALSE, &ip, &port);
    ok(res == STATUS_INVALID_PARAMETER, "[null address] res = 0x%08x, expected 0x%08x\n",
       res, STATUS_INVALID_PARAMETER);
    ok(ip.S_un.S_addr == 0xabababab, "RtlIpv4StringToAddressExA should not touch the ip!, ip == %x\n", ip.S_un.S_addr);
    ok(port == 0xdead, "RtlIpv4StringToAddressExA should not touch the port!, port == %x\n", port);

    port = 0xdead;
    res = pRtlIpv4StringToAddressExA("1.1.1.1", FALSE, NULL, &port);
    ok(res == STATUS_INVALID_PARAMETER, "[null ip] res = 0x%08x, expected 0x%08x\n",
       res, STATUS_INVALID_PARAMETER);
    ok(port == 0xdead, "RtlIpv4StringToAddressExA should not touch the port!, port == %x\n", port);

    ip.S_un.S_addr = 0xabababab;
    port = 0xdead;
    res = pRtlIpv4StringToAddressExA("1.1.1.1", FALSE, &ip, NULL);
    ok(res == STATUS_INVALID_PARAMETER, "[null port] res = 0x%08x, expected 0x%08x\n",
       res, STATUS_INVALID_PARAMETER);
    ok(ip.S_un.S_addr == 0xabababab, "RtlIpv4StringToAddressExA should not touch the ip!, ip == %x\n", ip.S_un.S_addr);
    ok(port == 0xdead, "RtlIpv4StringToAddressExA should not touch the port!, port == %x\n", port);

    /* first we run the non-ex testcases on the ex function */
    for (i = 0; i < ipv4_testcount; i++)
    {
        NTSTATUS expect_res = (ipv4_tests[i].flags & ex_fail_4) ? STATUS_INVALID_PARAMETER : ipv4_tests[i].res;

        /* non-strict */
        port = 0xdead;
        ip.S_un.S_addr = 0xabababab;
        res = pRtlIpv4StringToAddressExA(ipv4_tests[i].address, FALSE, &ip, &port);
        ok(res == expect_res, "[%s] res = 0x%08x, expected 0x%08x\n",
           ipv4_tests[i].address, res, expect_res);

        init_ip4(&expected_ip, ipv4_tests[i].ip);
        ok(ip.S_un.S_addr == expected_ip.S_un.S_addr, "[%s] ip = %08x, expected %08x\n",
           ipv4_tests[i].address, ip.S_un.S_addr, expected_ip.S_un.S_addr);

        if (!(ipv4_tests[i].flags & strict_diff_4))
        {
            ipv4_tests[i].res_strict = ipv4_tests[i].res;
            ipv4_tests[i].terminator_offset_strict = ipv4_tests[i].terminator_offset;
            ipv4_tests[i].ip_strict[0] = ipv4_tests[i].ip[0];
            ipv4_tests[i].ip_strict[1] = ipv4_tests[i].ip[1];
            ipv4_tests[i].ip_strict[2] = ipv4_tests[i].ip[2];
            ipv4_tests[i].ip_strict[3] = ipv4_tests[i].ip[3];
        }
        /* strict */
        expect_res = (ipv4_tests[i].flags & ex_fail_4) ? STATUS_INVALID_PARAMETER : ipv4_tests[i].res_strict;
        port = 0xdead;
        ip.S_un.S_addr = 0xabababab;
        res = pRtlIpv4StringToAddressExA(ipv4_tests[i].address, TRUE, &ip, &port);
        ok(res == expect_res, "[%s] res = 0x%08x, expected 0x%08x\n",
           ipv4_tests[i].address, res, expect_res);

        init_ip4(&expected_ip, ipv4_tests[i].ip_strict);
        ok(ip.S_un.S_addr == expected_ip.S_un.S_addr, "[%s] ip = %08x, expected %08x\n",
           ipv4_tests[i].address, ip.S_un.S_addr, expected_ip.S_un.S_addr);
    }


    for (i = 0; i < ipv4_ex_testcount; i++)
    {
        /* Strict is only relevant for the ip address, so make sure that it does not influence the port */
        for (strict = 0; strict < 2; strict++)
        {
            ip.S_un.S_addr = 0xabababab;
            port = 0xdead;
            res = pRtlIpv4StringToAddressExA(ipv4_ex_tests[i].address, strict, &ip, &port);
            ok(res == ipv4_ex_tests[i].res, "[%s] res = 0x%08x, expected 0x%08x\n",
               ipv4_ex_tests[i].address, res, ipv4_ex_tests[i].res);

            init_ip4(&expected_ip, ipv4_ex_tests[i].ip);
            ok(ip.S_un.S_addr == expected_ip.S_un.S_addr, "[%s] ip = %08x, expected %08x\n",
               ipv4_ex_tests[i].address, ip.S_un.S_addr, expected_ip.S_un.S_addr);
            ok(port == ipv4_ex_tests[i].port, "[%s] port = %u, expected %u\n",
               ipv4_ex_tests[i].address, port, ipv4_ex_tests[i].port);
        }
    }
}

/* ipv6 addresses based on the set from https://github.com/beaugunderson/javascript-ipv6/tree/master/test/data */
static const struct
{
    PCSTR address;
    NTSTATUS res;
    int terminator_offset;
    int ip[8];
    /* win_broken: older versions of windows do not handle this correct
        ex_fail: Ex function does need the string to be terminated, non-Ex does not.
        ex_skip: test doesnt make sense for Ex (f.e. it's invalid for non-Ex but valid for Ex) */
    enum { normal_6, win_broken_6 = 1, ex_fail_6 = 2, ex_skip_6 = 4 } flags;
} ipv6_tests[] =
{
    { "0000:0000:0000:0000:0000:0000:0000:0000",        STATUS_SUCCESS,             39,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "0000:0000:0000:0000:0000:0000:0000:0001",        STATUS_SUCCESS,             39,
            { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
    { "0:0:0:0:0:0:0:0",                                STATUS_SUCCESS,             15,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "0:0:0:0:0:0:0:1",                                STATUS_SUCCESS,             15,
            { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
    { "0:0:0:0:0:0:0::",                                STATUS_SUCCESS,             13,
            { 0, 0, 0, 0, 0, 0, 0, 0 }, win_broken_6 },
    { "0:0:0:0:0:0:13.1.68.3",                          STATUS_SUCCESS,             21,
            { 0, 0, 0, 0, 0, 0, 0x10d, 0x344 } },
    { "0:0:0:0:0:0::",                                  STATUS_SUCCESS,             13,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "0:0:0:0:0::",                                    STATUS_SUCCESS,             11,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "0:0:0:0:0:FFFF:129.144.52.38",                   STATUS_SUCCESS,             28,
            { 0, 0, 0, 0, 0, 0xffff, 0x9081, 0x2634 } },
    { "0::",                                            STATUS_SUCCESS,             3,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "0:1:2:3:4:5:6:7",                                STATUS_SUCCESS,             15,
            { 0, 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700 } },
    { "1080:0:0:0:8:800:200c:417a",                     STATUS_SUCCESS,             26,
            { 0x8010, 0, 0, 0, 0x800, 0x8, 0x0c20, 0x7a41 } },
    { "0:a:b:c:d:e:f::",                                STATUS_SUCCESS,             13,
            { 0, 0xa00, 0xb00, 0xc00, 0xd00, 0xe00, 0xf00, 0 }, win_broken_6 },
    { "1111:2222:3333:4444:5555:6666:123.123.123.123",  STATUS_SUCCESS,             45,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
    { "1111:2222:3333:4444:5555:6666:7777:8888",        STATUS_SUCCESS,             39,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 } },
    { "1111:2222:3333:4444:0x5555:6666:7777:8888",      STATUS_INVALID_PARAMETER,   21,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333:4444:x555:6666:7777:8888",        STATUS_INVALID_PARAMETER,   20,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333:4444:0r5555:6666:7777:8888",      STATUS_INVALID_PARAMETER,   21,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333:4444:r5555:6666:7777:8888",       STATUS_INVALID_PARAMETER,   20,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333:4444:5555:6666:7777::",           STATUS_SUCCESS,             34,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0 }, win_broken_6 },
    { "1111:2222:3333:4444:5555:6666::",                STATUS_SUCCESS,             31,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0 } },
    { "1111:2222:3333:4444:5555:6666::8888",            STATUS_SUCCESS,             35,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0x8888 } },
    { "1111:2222:3333:4444:5555::",                     STATUS_SUCCESS,             26,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0 } },
    { "1111:2222:3333:4444:5555::123.123.123.123",      STATUS_SUCCESS,             41,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0x7b7b, 0x7b7b } },
    { "1111:2222:3333:4444:5555::0x1.123.123.123",      STATUS_SUCCESS,             27,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x100 }, ex_fail_6 },
    { "1111:2222:3333:4444:5555::0x88",                 STATUS_SUCCESS,             27,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8800 }, ex_fail_6 },
    { "1111:2222:3333:4444:5555::0X88",                 STATUS_SUCCESS,             27,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8800 }, ex_fail_6 },
    { "1111:2222:3333:4444:5555::0X",                   STATUS_SUCCESS,             27,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0 }, ex_fail_6 },
    { "1111:2222:3333:4444:5555::0X88:7777",            STATUS_SUCCESS,             27,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8800 }, ex_fail_6 },
    { "1111:2222:3333:4444:5555::0x8888",               STATUS_SUCCESS,             27,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8888 }, ex_fail_6 },
    { "1111:2222:3333:4444:5555::08888",                STATUS_INVALID_PARAMETER,   31,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333:4444:5555::fffff",                STATUS_INVALID_PARAMETER,   31,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333:4444::fffff",                     STATUS_INVALID_PARAMETER,   26,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333::fffff",                          STATUS_INVALID_PARAMETER,   21,
            { 0x1111, 0x2222, 0x3333, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333:4444:5555::7777:8888",            STATUS_SUCCESS,             35,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0x7777, 0x8888 } },
    { "1111:2222:3333:4444:5555::8888",                 STATUS_SUCCESS,             30,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8888 } },
    { "1111::",                                         STATUS_SUCCESS,             6,
            { 0x1111, 0, 0, 0, 0, 0, 0, 0 } },
    { "1111::123.123.123.123",                          STATUS_SUCCESS,             21,
            { 0x1111, 0, 0, 0, 0, 0, 0x7b7b, 0x7b7b } },
    { "1111::3333:4444:5555:6666:123.123.123.123",      STATUS_SUCCESS,             41,
            { 0x1111, 0, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
    { "1111::3333:4444:5555:6666:7777:8888",            STATUS_SUCCESS,             35,
            { 0x1111, 0, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 } },
    { "1111::4444:5555:6666:123.123.123.123",           STATUS_SUCCESS,             36,
            { 0x1111, 0, 0, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
    { "1111::4444:5555:6666:7777:8888",                 STATUS_SUCCESS,             30,
            { 0x1111, 0, 0, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 } },
    { "1111::5555:6666:123.123.123.123",                STATUS_SUCCESS,             31,
            { 0x1111, 0, 0, 0, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
    { "1111::5555:6666:7777:8888",                      STATUS_SUCCESS,             25,
            { 0x1111, 0, 0, 0, 0x5555, 0x6666, 0x7777, 0x8888 } },
    { "1111::6666:123.123.123.123",                     STATUS_SUCCESS,             26,
            { 0x1111, 0, 0, 0, 0, 0x6666, 0x7b7b, 0x7b7b } },
    { "1111::6666:7777:8888",                           STATUS_SUCCESS,             20,
            { 0x1111, 0, 0, 0, 0, 0x6666, 0x7777, 0x8888 } },
    { "1111::7777:8888",                                STATUS_SUCCESS,             15,
            { 0x1111, 0, 0, 0, 0, 0, 0x7777, 0x8888 } },
    { "1111::8888",                                     STATUS_SUCCESS,             10,
            { 0x1111, 0, 0, 0, 0, 0, 0, 0x8888 } },
    { "1:2:3:4:5:6:1.2.3.4",                            STATUS_SUCCESS,             19,
            { 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x201, 0x403 } },
    { "1:2:3:4:5:6:7:8",                                STATUS_SUCCESS,             15,
            { 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800 } },
    { "1:2:3:4:5:6::",                                  STATUS_SUCCESS,             13,
            { 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0, 0 } },
    { "1:2:3:4:5:6::8",                                 STATUS_SUCCESS,             14,
            { 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0, 0x800 } },
    { "2001:0000:1234:0000:0000:C1C0:ABCD:0876",        STATUS_SUCCESS,             39,
            { 0x120, 0, 0x3412, 0, 0, 0xc0c1, 0xcdab, 0x7608 } },
    { "2001:0000:4136:e378:8000:63bf:3fff:fdd2",        STATUS_SUCCESS,             39,
            { 0x120, 0, 0x3641, 0x78e3, 0x80, 0xbf63, 0xff3f, 0xd2fd } },
    { "2001:0db8:0:0:0:0:1428:57ab",                    STATUS_SUCCESS,             27,
            { 0x120, 0xb80d, 0, 0, 0, 0, 0x2814, 0xab57 } },
    { "2001:0db8:1234:ffff:ffff:ffff:ffff:ffff",        STATUS_SUCCESS,             39,
            { 0x120, 0xb80d, 0x3412, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff } },
    { "2001::CE49:7601:2CAD:DFFF:7C94:FFFE",            STATUS_SUCCESS,             35,
            { 0x120, 0, 0x49ce, 0x176, 0xad2c, 0xffdf, 0x947c, 0xfeff } },
    { "2001:db8:85a3::8a2e:370:7334",                   STATUS_SUCCESS,             28,
            { 0x120, 0xb80d, 0xa385, 0, 0, 0x2e8a, 0x7003, 0x3473 } },
    { "3ffe:0b00:0000:0000:0001:0000:0000:000a",        STATUS_SUCCESS,             39,
            { 0xfe3f, 0xb, 0, 0, 0x100, 0, 0, 0xa00 } },
    { "::",                                             STATUS_SUCCESS,             2,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "::%16",                                          STATUS_SUCCESS,             2,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "::/16",                                          STATUS_SUCCESS,             2,
            { 0, 0, 0, 0, 0, 0, 0, 0 }, ex_fail_6 },
    { "::0",                                            STATUS_SUCCESS,             3,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "::0:0",                                          STATUS_SUCCESS,             5,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "::0:0:0",                                        STATUS_SUCCESS,             7,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "::0:0:0:0",                                      STATUS_SUCCESS,             9,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "::0:0:0:0:0",                                    STATUS_SUCCESS,             11,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "::0:0:0:0:0:0",                                  STATUS_SUCCESS,             13,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
    /* this one and the next one are incorrectly parsed by windows,
        it adds one zero too many in front, cutting off the last digit. */
    { "::0:0:0:0:0:0:0",                                STATUS_SUCCESS,             13,
            { 0, 0, 0, 0, 0, 0, 0, 0 }, ex_fail_6 },
    { "::0:a:b:c:d:e:f",                                STATUS_SUCCESS,             13,
            { 0, 0, 0, 0xa00, 0xb00, 0xc00, 0xd00, 0xe00 }, ex_fail_6 },
    { "::123.123.123.123",                              STATUS_SUCCESS,             17,
            { 0, 0, 0, 0, 0, 0, 0x7b7b, 0x7b7b } },
    { "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",        STATUS_SUCCESS,             39,
            { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff } },

    { "':10.0.0.1",                                     STATUS_INVALID_PARAMETER,   0,
            { -1 } },
    { "-1",                                             STATUS_INVALID_PARAMETER,   0,
            { -1 } },
    { "02001:0000:1234:0000:0000:C1C0:ABCD:0876",       STATUS_INVALID_PARAMETER,   -1,
            { -1 } },
    { "2001:00000:1234:0000:0000:C1C0:ABCD:0876",       STATUS_INVALID_PARAMETER,   -1,
            { 0x120, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "2001:0000:01234:0000:0000:C1C0:ABCD:0876",       STATUS_INVALID_PARAMETER,   -1,
            { 0x120, 0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1.2.3.4",                                        STATUS_INVALID_PARAMETER,   7,
            { 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1.2.3.4:1111::5555",                             STATUS_INVALID_PARAMETER,   7,
            { 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1.2.3.4::5555",                                  STATUS_INVALID_PARAMETER,   7,
            { 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "11112222:3333:4444:5555:6666:1.2.3.4",           STATUS_INVALID_PARAMETER,   -1,
            { -1 } },
    { "11112222:3333:4444:5555:6666:7777:8888",         STATUS_INVALID_PARAMETER,   -1,
            { -1 } },
    { "1111",                                           STATUS_INVALID_PARAMETER,   4,
            { -1 } },
    { "1111:22223333:4444:5555:6666:1.2.3.4",           STATUS_INVALID_PARAMETER,   -1,
            { 0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:22223333:4444:5555:6666:7777:8888",         STATUS_INVALID_PARAMETER,   -1,
            { 0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:",                                     STATUS_INVALID_PARAMETER,   10,
            { 0x1111, 0x2222, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:1.2.3.4",                              STATUS_INVALID_PARAMETER,   17,
            { 0x1111, 0x2222, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333",                                 STATUS_INVALID_PARAMETER,   14,
            { 0x1111, 0x2222, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333:4444:5555:6666:7777:1.2.3.4",     STATUS_SUCCESS,             36,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x100 }, ex_fail_6 },
    { "1111:2222:3333:4444:5555:6666:7777:8888:",       STATUS_SUCCESS,             39,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 }, ex_fail_6 },
    { "1111:2222:3333:4444:5555:6666:7777:8888:1.2.3.4",STATUS_SUCCESS,             39,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 }, ex_fail_6 },
    { "1111:2222:3333:4444:5555:6666:7777:8888:9999",   STATUS_SUCCESS,             39,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 }, ex_fail_6 },
    { "1111:2222:::",                                   STATUS_SUCCESS,             11,
            { 0x1111, 0x2222, 0, 0, 0, 0, 0, 0 }, ex_fail_6 },
    { "1111::5555:",                                    STATUS_INVALID_PARAMETER,   11,
            { 0x1111, 0x5555, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111::3333:4444:5555:6666:7777::",               STATUS_SUCCESS,             30,
            { 0x1111, 0, 0, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777 }, ex_fail_6 },
    { "1111:2222:::4444:5555:6666:1.2.3.4",             STATUS_SUCCESS,             11,
            { 0x1111, 0x2222, 0, 0, 0, 0, 0, 0 }, ex_fail_6 },
    { "1111::3333::5555:6666:1.2.3.4",                  STATUS_SUCCESS,             10,
            { 0x1111, 0, 0, 0, 0, 0, 0, 0x3333 }, ex_fail_6 },
    { "12345::6:7:8",                                   STATUS_INVALID_PARAMETER,   -1,
            { -1 } },
    { "1::1.2.256.4",                                   STATUS_INVALID_PARAMETER,   -1,
            { 0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.3.256",                                   STATUS_INVALID_PARAMETER,   12,
            { 0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.3.300",                                   STATUS_INVALID_PARAMETER,   12,
            { 0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2::1",                                      STATUS_INVALID_PARAMETER,   6,
            { 0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.3.4::1",                                  STATUS_SUCCESS,             10,
            { 0x100, 0, 0, 0, 0, 0, 0x201, 0x403 }, ex_fail_6 },
    { "1::1.",                                          STATUS_INVALID_PARAMETER,   5,
            { 0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2",                                         STATUS_INVALID_PARAMETER,   6,
            { 0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.",                                        STATUS_INVALID_PARAMETER,   7,
            { 0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.3",                                       STATUS_INVALID_PARAMETER,   8,
            { 0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.3.",                                      STATUS_INVALID_PARAMETER,   9,
            { 0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.3.4",                                     STATUS_SUCCESS,             10,
            { 0x100, 0, 0, 0, 0, 0, 0x201, 0x403 } },
    { "1::1.2.3.900",                                   STATUS_INVALID_PARAMETER,   12,
            { 0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.300.4",                                   STATUS_INVALID_PARAMETER,   -1,
            { 0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.256.3.4",                                   STATUS_INVALID_PARAMETER,   -1,
            { 0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::256.2.3.4",                                   STATUS_INVALID_PARAMETER,   -1,
            { 0x100, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::2::3",                                        STATUS_SUCCESS,             4,
            { 0x100, 0, 0, 0, 0, 0, 0, 0x200 }, ex_fail_6 },
    { "2001:0000:1234: 0000:0000:C1C0:ABCD:0876",       STATUS_INVALID_PARAMETER,   15,
            { 0x120, 0, 0x3412, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "2001:0000:1234:0000:0000:C1C0:ABCD:0876  0",     STATUS_SUCCESS,             39,
            { 0x120, 0, 0x3412, 0, 0, 0xc0c1, 0xcdab, 0x7608 }, ex_fail_6 },
    { "2001:1:1:1:1:1:255Z255X255Y255",                 STATUS_INVALID_PARAMETER,   18,
            { 0x120, 0x100, 0x100, 0x100, 0x100, 0x100, 0xabab, 0xabab } },
    { "2001::FFD3::57ab",                               STATUS_SUCCESS,             10,
            { 0x120, 0, 0, 0, 0, 0, 0, 0xd3ff }, ex_fail_6 },
    { ":",                                              STATUS_INVALID_PARAMETER,   0,
            { -1 } },
    { ":1111:2222:3333:4444:5555:6666:1.2.3.4",         STATUS_INVALID_PARAMETER,   0,
            { -1 } },
    { ":1111:2222:3333:4444:5555:6666:7777:8888",       STATUS_INVALID_PARAMETER,   0,
            { -1 } },
    { ":1111::",                                        STATUS_INVALID_PARAMETER,   0,
            { -1 } },
    { "::-1",                                           STATUS_SUCCESS,             2,
            { 0, 0, 0, 0, 0, 0, 0, 0 }, ex_fail_6 },
    { "::.",                                            STATUS_SUCCESS,             2,
            { 0, 0, 0, 0, 0, 0, 0, 0 }, ex_fail_6 },
    { "::..",                                           STATUS_SUCCESS,             2,
            { 0, 0, 0, 0, 0, 0, 0, 0 }, ex_fail_6 },
    { "::...",                                          STATUS_SUCCESS,             2,
            { 0, 0, 0, 0, 0, 0, 0, 0 }, ex_fail_6 },
    { "XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:1.2.3.4",          STATUS_INVALID_PARAMETER,   0,
            { -1 } },
    { "[::]",                                           STATUS_INVALID_PARAMETER,   0,
            { -1 }, ex_skip_6 },
};
const unsigned int ipv6_testcount = sizeof(ipv6_tests) / sizeof(ipv6_tests[0]);

static void init_ip6(IN6_ADDR* addr, const int src[8])
{
    unsigned int j;
    if (!src || src[0] == -1)
    {
        for (j = 0; j < 8; ++j)
            addr->s6_words[j] = 0xabab;
    }
    else
    {
        for (j = 0; j < 8; ++j)
            addr->s6_words[j] = src[j];
    }
}

static void test_RtlIpv6AddressToString(void)
{
    CHAR buffer[50];
    LPCSTR result;
    IN6_ADDR ip;
    DWORD_PTR len;
    struct
    {
        PCSTR address;
        int ip[8];
    } tests[] =
    {
        /* ipv4 addresses & ISATAP addresses */
        { "::13.1.68.3",                                { 0, 0, 0, 0, 0, 0, 0x10d, 0x344 } },
        { "::ffff:13.1.68.3",                           { 0, 0, 0, 0, 0, 0xffff, 0x10d, 0x344 } },
        { "::feff:d01:4403",                            { 0, 0, 0, 0, 0, 0xfffe, 0x10d, 0x344 } },
        { "::fffe:d01:4403",                            { 0, 0, 0, 0, 0, 0xfeff, 0x10d, 0x344 } },
        { "::100:d01:4403",                             { 0, 0, 0, 0, 0, 1, 0x10d, 0x344 } },
        { "::1:d01:4403",                               { 0, 0, 0, 0, 0, 0x100, 0x10d, 0x344 } },
        { "::ffff:0:4403",                              { 0, 0, 0, 0, 0, 0xffff, 0, 0x344 } },
        { "::ffff:13.1.0.0",                            { 0, 0, 0, 0, 0, 0xffff, 0x10d, 0 } },
        { "::ffff:0:0",                                 { 0, 0, 0, 0, 0, 0xffff, 0, 0 } },
        { "::ffff:0:13.1.68.3",                         { 0, 0, 0, 0, 0xffff, 0, 0x10d, 0x344 } },
        { "::ffff:ffff:d01:4403",                       { 0, 0, 0, 0, 0xffff, 0xffff, 0x10d, 0x344 } },
        { "::ffff:0:0:d01:4403",                        { 0, 0, 0, 0xffff, 0, 0, 0x10d, 0x344 } },
        { "::ffff:255.255.255.255",                     { 0, 0, 0, 0, 0, 0xffff, 0xffff, 0xffff } },
        { "::ffff:129.144.52.38",                       { 0, 0, 0, 0, 0, 0xffff, 0x9081, 0x2634 } },
        { "::5efe:129.144.52.38",                       { 0, 0, 0, 0, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "1111:2222:3333:4444:0:5efe:129.144.52.38",   { 0x1111, 0x2222, 0x3333, 0x4444, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "1111:2222:3333::5efe:129.144.52.38",         { 0x1111, 0x2222, 0x3333, 0, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "1111:2222::5efe:129.144.52.38",              { 0x1111, 0x2222, 0, 0, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "1111::5efe:129.144.52.38",                   { 0x1111, 0, 0, 0, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "::200:5efe:129.144.52.38",                   { 0, 0, 0, 0, 2, 0xfe5e, 0x9081, 0x2634 } },
        { "::100:5efe:8190:3426",                       { 0, 0, 0, 0, 1, 0xfe5e, 0x9081, 0x2634 } },
        /* 'normal' addresses */
        { "::1",                                        { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "0:1:2:3:4:5:6:7",                            { 0, 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700 } },
        { "1080::8:800:200c:417a",                      { 0x8010, 0, 0, 0, 0x800, 0x8, 0x0c20, 0x7a41 } },
        { "1111:2222:3333:4444:5555:6666:7b7b:7b7b",    { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
        { "1111:2222:3333:4444:5555:6666:7777:8888",    { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 } },
        { "1111:2222:3333:4444:5555:6666::",            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0 } },
        { "1111:2222:3333:4444:5555:6666:0:8888",       { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0x8888 } },
        { "1111:2222:3333:4444:5555::",                 { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0 } },
        { "1111:2222:3333:4444:5555:0:7b7b:7b7b",       { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0x7b7b, 0x7b7b } },
        { "1111:2222:3333:4444:5555:0:7777:8888",       { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0x7777, 0x8888 } },
        { "1111:2222:3333:4444:5555::8888",             { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8888 } },
        { "1111::",                                     { 0x1111, 0, 0, 0, 0, 0, 0, 0 } },
        { "1111::7b7b:7b7b",                            { 0x1111, 0, 0, 0, 0, 0, 0x7b7b, 0x7b7b } },
        { "1111:0:3333:4444:5555:6666:7b7b:7b7b",       { 0x1111, 0, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
        { "1111:0:3333:4444:5555:6666:7777:8888",       { 0x1111, 0, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 } },
        { "1111::4444:5555:6666:7b7b:7b7b",             { 0x1111, 0, 0, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
        { "1111::4444:5555:6666:7777:8888",             { 0x1111, 0, 0, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 } },
        { "1111::5555:6666:7b7b:7b7b",                  { 0x1111, 0, 0, 0, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
        { "1111::5555:6666:7777:8888",                  { 0x1111, 0, 0, 0, 0x5555, 0x6666, 0x7777, 0x8888 } },
        { "1111::6666:7b7b:7b7b",                       { 0x1111, 0, 0, 0, 0, 0x6666, 0x7b7b, 0x7b7b } },
        { "1111::6666:7777:8888",                       { 0x1111, 0, 0, 0, 0, 0x6666, 0x7777, 0x8888 } },
        { "1111::7777:8888",                            { 0x1111, 0, 0, 0, 0, 0, 0x7777, 0x8888 } },
        { "1111::8888",                                 { 0x1111, 0, 0, 0, 0, 0, 0, 0x8888 } },
        { "1:2:3:4:5:6:102:304",                        { 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x201, 0x403 } },
        { "1:2:3:4:5:6:7:8",                            { 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800 } },
        { "1:2:3:4:5:6::",                              { 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0, 0 } },
        { "1:2:3:4:5:6:0:8",                            { 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0, 0x800 } },
        { "2001:0:1234::c1c0:abcd:876",                 { 0x120, 0, 0x3412, 0, 0, 0xc0c1, 0xcdab, 0x7608 } },
        { "2001:0:4136:e378:8000:63bf:3fff:fdd2",       { 0x120, 0, 0x3641, 0x78e3, 0x80, 0xbf63, 0xff3f, 0xd2fd } },
        { "2001:db8::1428:57ab",                        { 0x120, 0xb80d, 0, 0, 0, 0, 0x2814, 0xab57 } },
        { "2001:db8:1234:ffff:ffff:ffff:ffff:ffff",     { 0x120, 0xb80d, 0x3412, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff } },
        { "2001:0:ce49:7601:2cad:dfff:7c94:fffe",       { 0x120, 0, 0x49ce, 0x176, 0xad2c, 0xffdf, 0x947c, 0xfeff } },
        { "2001:db8:85a3::8a2e:370:7334",               { 0x120, 0xb80d, 0xa385, 0, 0, 0x2e8a, 0x7003, 0x3473 } },
        { "3ffe:b00::1:0:0:a",                          { 0xfe3f, 0xb, 0, 0, 0x100, 0, 0, 0xa00 } },
        { "::a:b:c:d:e",                                { 0, 0, 0, 0xa00, 0xb00, 0xc00, 0xd00, 0xe00 } },
        { "::123.123.123.123",                          { 0, 0, 0, 0, 0, 0, 0x7b7b, 0x7b7b } },
        { "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",    { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff } },
        { "1111:2222:3333:4444:5555:6666:7777:1",       { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x100 } },
        { "1111:2222:3333:4444:5555:6666:7777:8888",    { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 } },
        { "1111:2222::",                                { 0x1111, 0x2222, 0, 0, 0, 0, 0, 0 } },
        { "1111::3333:4444:5555:6666:7777",             { 0x1111, 0, 0, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777 } },
        { "1111:2222::",                                { 0x1111, 0x2222, 0, 0, 0, 0, 0, 0 } },
        { "1111::3333",                                 { 0x1111, 0, 0, 0, 0, 0, 0, 0x3333 } },
        { "2001:0:1234::c1c0:abcd:876",                 { 0x120, 0, 0x3412, 0, 0, 0xc0c1, 0xcdab, 0x7608 } },
        { "2001::ffd3",                                 { 0x120, 0, 0, 0, 0, 0, 0, 0xd3ff } },
    };
    const size_t testcount = sizeof(tests) / sizeof(tests[0]);
    unsigned int i;

    if (!pRtlIpv6AddressToStringA)
    {
        skip("RtlIpv6AddressToStringA not available\n");
        return;
    }

    memset(buffer, '#', sizeof(buffer));
    buffer[sizeof(buffer)-1] = 0;
    memset(&ip, 0, sizeof(ip));
    result = pRtlIpv6AddressToStringA(&ip, buffer);

    len = strlen(buffer);
    ok(result == (buffer + len) && !strcmp(buffer, "::"),
       "got %p with '%s' (expected %p with '::')\n", result, buffer, buffer + len);

    result = pRtlIpv6AddressToStringA(&ip, NULL);
    ok(result == (LPCSTR)~0 || broken(result == (LPCSTR)len) /* WinXP / Win2k3 */,
       "got %p, expected %p\n", result, (LPCSTR)~0);

    for (i = 0; i < testcount; i++)
    {
        init_ip6(&ip, tests[i].ip);
        memset(buffer, '#', sizeof(buffer));
        buffer[sizeof(buffer)-1] = 0;

        result = pRtlIpv6AddressToStringA(&ip, buffer);
        len = strlen(buffer);
        ok(result == (buffer + len) && !strcmp(buffer, tests[i].address),
           "got %p with '%s' (expected %p with '%s')\n", result, buffer, buffer + len, tests[i].address);

        ok(buffer[45] == 0 || broken(buffer[45] != 0) /* WinXP / Win2k3 */,
           "expected data at buffer[45] to always be NULL\n");
        ok(buffer[46] == '#', "expected data at buffer[46] not to change\n");
    }
}

static void test_RtlIpv6AddressToStringEx(void)
{
    CHAR buffer[70];
    NTSTATUS res;
    IN6_ADDR ip;
    ULONG len;
    struct
    {
        PCSTR address;
        ULONG scopeid;
        USHORT port;
        int ip[8];
    } tests[] =
    {
        /* ipv4 addresses & ISATAP addresses */
        { "::13.1.68.3",                                                0,          0, { 0, 0, 0, 0, 0, 0, 0x10d, 0x344 } },
        { "::13.1.68.3%1",                                              1,          0, { 0, 0, 0, 0, 0, 0, 0x10d, 0x344 } },
        { "::13.1.68.3%4294949819",                                     0xffffbbbb, 0, { 0, 0, 0, 0, 0, 0, 0x10d, 0x344 } },
        { "[::13.1.68.3%4294949819]:65518",                             0xffffbbbb, 0xeeff, { 0, 0, 0, 0, 0, 0, 0x10d, 0x344 } },
        { "[::13.1.68.3%4294949819]:256",                               0xffffbbbb, 1, { 0, 0, 0, 0, 0, 0, 0x10d, 0x344 } },
        { "[::13.1.68.3]:256",                                          0,          1, { 0, 0, 0, 0, 0, 0, 0x10d, 0x344 } },

        { "::1:d01:4403",                                               0,          0, { 0, 0, 0, 0, 0, 0x100, 0x10d, 0x344 } },
        { "::1:d01:4403%1",                                             1,          0, { 0, 0, 0, 0, 0, 0x100, 0x10d, 0x344 } },
        { "::1:d01:4403%4294949819",                                    0xffffbbbb, 0, { 0, 0, 0, 0, 0, 0x100, 0x10d, 0x344 } },
        { "[::1:d01:4403%4294949819]:65518",                            0xffffbbbb, 0xeeff, { 0, 0, 0, 0, 0, 0x100, 0x10d, 0x344 } },
        { "[::1:d01:4403%4294949819]:256",                              0xffffbbbb, 1, { 0, 0, 0, 0, 0, 0x100, 0x10d, 0x344 } },
        { "[::1:d01:4403]:256",                                         0,          1, { 0, 0, 0, 0, 0, 0x100, 0x10d, 0x344 } },

        { "1111:2222:3333:4444:0:5efe:129.144.52.38",                   0,          0, { 0x1111, 0x2222, 0x3333, 0x4444, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "1111:2222:3333:4444:0:5efe:129.144.52.38%1",                 1,          0, { 0x1111, 0x2222, 0x3333, 0x4444, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "1111:2222:3333:4444:0:5efe:129.144.52.38%4294949819",        0xffffbbbb, 0, { 0x1111, 0x2222, 0x3333, 0x4444, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "[1111:2222:3333:4444:0:5efe:129.144.52.38%4294949819]:65518",0xffffbbbb, 0xeeff, { 0x1111, 0x2222, 0x3333, 0x4444, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "[1111:2222:3333:4444:0:5efe:129.144.52.38%4294949819]:256",  0xffffbbbb, 1, { 0x1111, 0x2222, 0x3333, 0x4444, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "[1111:2222:3333:4444:0:5efe:129.144.52.38]:256",             0,          1, { 0x1111, 0x2222, 0x3333, 0x4444, 0, 0xfe5e, 0x9081, 0x2634 } },

        { "::1",                                                        0,          0, { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "::1%1",                                                      1,          0, { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "::1%4294949819",                                             0xffffbbbb, 0, { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "[::1%4294949819]:65518",                                     0xffffbbbb, 0xeeff, { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "[::1%4294949819]:256",                                       0xffffbbbb, 1, { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "[::1]:256",                                                  0,          1, { 0, 0, 0, 0, 0, 0, 0, 0x100 } },

        { "1111:2222:3333:4444:5555:6666:7b7b:7b7b",                    0,          0, { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
        { "1111:2222:3333:4444:5555:6666:7b7b:7b7b%1",                  1,          0, { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
        { "1111:2222:3333:4444:5555:6666:7b7b:7b7b%4294949819",         0xffffbbbb, 0, { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
        { "[1111:2222:3333:4444:5555:6666:7b7b:7b7b%4294949819]:65518", 0xffffbbbb, 0xeeff, { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
        { "[1111:2222:3333:4444:5555:6666:7b7b:7b7b%4294949819]:256",   0xffffbbbb, 1, { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },
        { "[1111:2222:3333:4444:5555:6666:7b7b:7b7b]:256",              0,          1, { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b } },

        { "1111::",                                                     0,          0, { 0x1111, 0, 0, 0, 0, 0, 0, 0 } },
        { "1111::%1",                                                   1,          0, { 0x1111, 0, 0, 0, 0, 0, 0, 0 } },
        { "1111::%4294949819",                                          0xffffbbbb, 0, { 0x1111, 0, 0, 0, 0, 0, 0, 0 } },
        { "[1111::%4294949819]:65518",                                  0xffffbbbb, 0xeeff, { 0x1111, 0, 0, 0, 0, 0, 0, 0 } },
        { "[1111::%4294949819]:256",                                    0xffffbbbb, 1, { 0x1111, 0, 0, 0, 0, 0, 0, 0 } },
        { "[1111::]:256",                                               0,          1, { 0x1111, 0, 0, 0, 0, 0, 0, 0 } },

        { "2001::ffd3",                                                 0,          0, { 0x120, 0, 0, 0, 0, 0, 0, 0xd3ff } },
        { "2001::ffd3%1",                                               1,          0, { 0x120, 0, 0, 0, 0, 0, 0, 0xd3ff } },
        { "2001::ffd3%4294949819",                                      0xffffbbbb, 0, { 0x120, 0, 0, 0, 0, 0, 0, 0xd3ff } },
        { "[2001::ffd3%4294949819]:65518",                              0xffffbbbb, 0xeeff, { 0x120, 0, 0, 0, 0, 0, 0, 0xd3ff } },
        { "[2001::ffd3%4294949819]:256",                                0xffffbbbb, 1, { 0x120, 0, 0, 0, 0, 0, 0, 0xd3ff } },
        { "[2001::ffd3]:256",                                           0,          1, { 0x120, 0, 0, 0, 0, 0, 0, 0xd3ff } },
    };
    const size_t testcount = sizeof(tests) / sizeof(tests[0]);
    unsigned int i;

    if (!pRtlIpv6AddressToStringExA)
    {
        skip("RtlIpv6AddressToStringExA not available\n");
        return;
    }

    memset(buffer, '#', sizeof(buffer));
    buffer[sizeof(buffer)-1] = 0;
    memset(&ip, 0, sizeof(ip));
    len = sizeof(buffer);
    res = pRtlIpv6AddressToStringExA(&ip, 0, 0, buffer, &len);

    ok(res == STATUS_SUCCESS, "[validate] res = 0x%08x, expected STATUS_SUCCESS\n", res);
    ok(len == 3 && !strcmp(buffer, "::"),
        "got len %d with '%s' (expected 3 with '::')\n", len, buffer);

    memset(buffer, '#', sizeof(buffer));
    buffer[sizeof(buffer)-1] = 0;

    len = sizeof(buffer);
    res = pRtlIpv6AddressToStringExA(NULL, 0, 0, buffer, &len);
    ok(res == STATUS_INVALID_PARAMETER, "[null ip] res = 0x%08x, expected STATUS_INVALID_PARAMETER\n", res);

    len = sizeof(buffer);
    res = pRtlIpv6AddressToStringExA(&ip, 0, 0, NULL, &len);
    ok(res == STATUS_INVALID_PARAMETER, "[null buffer] res = 0x%08x, expected STATUS_INVALID_PARAMETER\n", res);

    res = pRtlIpv6AddressToStringExA(&ip, 0, 0, buffer, NULL);
    ok(res == STATUS_INVALID_PARAMETER, "[null length] res = 0x%08x, expected STATUS_INVALID_PARAMETER\n", res);

    len = 2;
    memset(buffer, '#', sizeof(buffer));
    buffer[sizeof(buffer)-1] = 0;
    res = pRtlIpv6AddressToStringExA(&ip, 0, 0, buffer, &len);
    ok(res == STATUS_INVALID_PARAMETER, "[null length] res = 0x%08x, expected STATUS_INVALID_PARAMETER\n", res);
    ok(buffer[0] == '#', "got first char %c (expected '#')\n", buffer[0]);
    ok(len == 3, "got len %d (expected len 3)\n", len);

    for (i = 0; i < testcount; i++)
    {
        init_ip6(&ip, tests[i].ip);
        len = sizeof(buffer);
        memset(buffer, '#', sizeof(buffer));
        buffer[sizeof(buffer)-1] = 0;

        res = pRtlIpv6AddressToStringExA(&ip, tests[i].scopeid, tests[i].port, buffer, &len);

        ok(res == STATUS_SUCCESS, "[validate] res = 0x%08x, expected STATUS_SUCCESS\n", res);
        ok(len == (strlen(tests[i].address) + 1) && !strcmp(buffer, tests[i].address),
           "got len %d with '%s' (expected %d with '%s')\n", len, buffer, (int)strlen(tests[i].address), tests[i].address);
    }
}

static void compare_RtlIpv6StringToAddressW(PCSTR name_a, int terminator_offset_a,
                                            const struct in6_addr *addr_a, NTSTATUS res_a)
{
    WCHAR name[512];
    NTSTATUS res;
    IN6_ADDR ip;
    PCWSTR terminator;

    if (!pRtlIpv6StringToAddressW)
        return;

    pRtlMultiByteToUnicodeN(name, sizeof(name), NULL, name_a, strlen(name_a) + 1);

    init_ip6(&ip, NULL);
    terminator = (void *)0xdeadbeef;
    res = pRtlIpv6StringToAddressW(name, &terminator, &ip);
    ok(res == res_a, "[W:%s] res = 0x%08x, expected 0x%08x\n", name_a, res, res_a);

    if (terminator_offset_a < 0)
    {
        ok(terminator == (void *)0xdeadbeef,
           "[W:%s] terminator = %p, expected it not to change\n",
           name_a, terminator);
    }
    else
    {
        ok(terminator == name + terminator_offset_a,
           "[W:%s] terminator = %p, expected %p\n",
           name_a, terminator, name + terminator_offset_a);
    }

    ok(!memcmp(&ip, addr_a, sizeof(ip)),
       "[W:%s] ip = %x:%x:%x:%x:%x:%x:%x:%x, expected %x:%x:%x:%x:%x:%x:%x:%x\n",
       name_a,
       ip.s6_words[0], ip.s6_words[1], ip.s6_words[2], ip.s6_words[3],
       ip.s6_words[4], ip.s6_words[5], ip.s6_words[6], ip.s6_words[7],
       addr_a->s6_words[0], addr_a->s6_words[1], addr_a->s6_words[2], addr_a->s6_words[3],
       addr_a->s6_words[4], addr_a->s6_words[5], addr_a->s6_words[6], addr_a->s6_words[7]);
}

static void test_RtlIpv6StringToAddress(void)
{
    NTSTATUS res;
    IN6_ADDR ip, expected_ip;
    PCSTR terminator;
    unsigned int i;

    if (!pRtlIpv6StringToAddressW)
    {
        skip("RtlIpv6StringToAddressW not available\n");
        /* we can continue, just not test W */
    }

    if (!pRtlIpv6StringToAddressA)
    {
        skip("RtlIpv6StringToAddressA not available\n");
        return; /* all tests are centered around A, we cannot continue */
    }

    res = pRtlIpv6StringToAddressA("::", &terminator, &ip);
    ok(res == STATUS_SUCCESS, "[validate] res = 0x%08x, expected STATUS_SUCCESS\n", res);
    if (0)
    {
        /* any of these crash */
        res = pRtlIpv6StringToAddressA(NULL, &terminator, &ip);
        ok(res == STATUS_INVALID_PARAMETER, "[null string] res = 0x%08x, expected STATUS_INVALID_PARAMETER\n", res);
        res = pRtlIpv6StringToAddressA("::", NULL, &ip);
        ok(res == STATUS_INVALID_PARAMETER, "[null terminator] res = 0x%08x, expected STATUS_INVALID_PARAMETER\n", res);
        res = pRtlIpv6StringToAddressA("::", &terminator, NULL);
        ok(res == STATUS_INVALID_PARAMETER, "[null result] res = 0x%08x, expected STATUS_INVALID_PARAMETER\n", res);
    }

    /* sanity check */
    ok(sizeof(ip) == sizeof(USHORT)* 8, "sizeof(ip)\n");

    for (i = 0; i < ipv6_testcount; i++)
    {
        init_ip6(&ip, NULL);
        terminator = (void *)0xdeadbeef;
        res = pRtlIpv6StringToAddressA(ipv6_tests[i].address, &terminator, &ip);
        compare_RtlIpv6StringToAddressW(ipv6_tests[i].address, (terminator != (void *)0xdeadbeef) ?
                                        (terminator - ipv6_tests[i].address) : -1, &ip, res);

        if (ipv6_tests[i].flags & win_broken_6)
        {
            ok(res == ipv6_tests[i].res || broken(res == STATUS_INVALID_PARAMETER),
               "[%s] res = 0x%08x, expected 0x%08x\n",
               ipv6_tests[i].address, res, ipv6_tests[i].res);

            if (res == STATUS_INVALID_PARAMETER)
                continue;
        }
        else
        {
            ok(res == ipv6_tests[i].res,
               "[%s] res = 0x%08x, expected 0x%08x\n",
               ipv6_tests[i].address, res, ipv6_tests[i].res);
        }

        if (ipv6_tests[i].terminator_offset < 0)
        {
            ok(terminator == (void *)0xdeadbeef,
               "[%s] terminator = %p, expected it not to change\n",
               ipv6_tests[i].address, terminator);
        }
        else if (ipv6_tests[i].flags & win_broken_6)
        {
            PCSTR expected = ipv6_tests[i].address + ipv6_tests[i].terminator_offset;
            ok(terminator == expected || broken(terminator == expected + 2),
               "[%s] terminator = %p, expected %p\n",
               ipv6_tests[i].address, terminator, expected);
        }
        else
        {
            ok(terminator == ipv6_tests[i].address + ipv6_tests[i].terminator_offset,
               "[%s] terminator = %p, expected %p\n",
               ipv6_tests[i].address, terminator, ipv6_tests[i].address + ipv6_tests[i].terminator_offset);
        }

        init_ip6(&expected_ip, ipv6_tests[i].ip);
        ok(!memcmp(&ip, &expected_ip, sizeof(ip)),
           "[%s] ip = %x:%x:%x:%x:%x:%x:%x:%x, expected %x:%x:%x:%x:%x:%x:%x:%x\n",
           ipv6_tests[i].address,
           ip.s6_words[0], ip.s6_words[1], ip.s6_words[2], ip.s6_words[3],
           ip.s6_words[4], ip.s6_words[5], ip.s6_words[6], ip.s6_words[7],
           expected_ip.s6_words[0], expected_ip.s6_words[1], expected_ip.s6_words[2], expected_ip.s6_words[3],
           expected_ip.s6_words[4], expected_ip.s6_words[5], expected_ip.s6_words[6], expected_ip.s6_words[7]);
    }
}

static void compare_RtlIpv6StringToAddressExW(PCSTR name_a, const struct in6_addr *addr_a, HRESULT res_a, ULONG scope_a, USHORT port_a)
{
    WCHAR name[512];
    NTSTATUS res;
    IN6_ADDR ip;
    ULONG scope = 0xbadf00d;
    USHORT port = 0xbeef;

    if (!pRtlIpv6StringToAddressExW)
        return;

    pRtlMultiByteToUnicodeN(name, sizeof(name), NULL, name_a, strlen(name_a) + 1);

    init_ip6(&ip, NULL);
    res = pRtlIpv6StringToAddressExW(name, &ip, &scope, &port);

    ok(res == res_a, "[W:%s] res = 0x%08x, expected 0x%08x\n", name_a, res, res_a);
    ok(scope == scope_a, "[W:%s] scope = 0x%08x, expected 0x%08x\n", name_a, scope, scope_a);
    ok(port == port_a, "[W:%s] port = 0x%08x, expected 0x%08x\n", name_a, port, port_a);

    ok(!memcmp(&ip, addr_a, sizeof(ip)),
       "[W:%s] ip = %x:%x:%x:%x:%x:%x:%x:%x, expected %x:%x:%x:%x:%x:%x:%x:%x\n",
       name_a,
       ip.s6_words[0], ip.s6_words[1], ip.s6_words[2], ip.s6_words[3],
       ip.s6_words[4], ip.s6_words[5], ip.s6_words[6], ip.s6_words[7],
       addr_a->s6_words[0], addr_a->s6_words[1], addr_a->s6_words[2], addr_a->s6_words[3],
       addr_a->s6_words[4], addr_a->s6_words[5], addr_a->s6_words[6], addr_a->s6_words[7]);
}

static void test_RtlIpv6StringToAddressEx(void)
{
    NTSTATUS res;
    IN6_ADDR ip, expected_ip;
    ULONG scope;
    USHORT port;
    static const struct
    {
        PCSTR address;
        NTSTATUS res;
        ULONG scope;
        USHORT port;
        int ip[8];
    } ipv6_ex_tests[] =
    {
        { "[::]",                                           STATUS_SUCCESS,             0,          0,
            { 0, 0, 0, 0, 0, 0, 0, 0 } },
        { "[::1]:8080",                                     STATUS_SUCCESS,             0,          0x901f,
            { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "[::1]:0x80",                                     STATUS_SUCCESS,             0,          0x8000,
            { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "[::1]:0X80",                                     STATUS_SUCCESS,             0,          0x8000,
            { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "[::1]:080",                                      STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "[::1]:800000000080",                             STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80",   STATUS_SUCCESS,             0,          0x5000,
            { 0xdcfe, 0x98ba, 0x5476, 0x1032, 0xdcfe, 0x98ba, 0x5476, 0x1032 } },
        { "[1080:0:0:0:8:800:200C:417A]:1234",              STATUS_SUCCESS,             0,          0xd204,
            { 0x8010, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[3ffe:2a00:100:7031::1]:8080",                   STATUS_SUCCESS,             0,          0x901f,
            { 0xfe3f, 0x2a, 1, 0x3170, 0, 0, 0, 0x100 } },
        { "[ 3ffe:2a00:100:7031::1]:8080",                  STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { -1 } },
        { "[3ffe:2a00:100:7031::1 ]:8080",                  STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0xfe3f, 0x2a, 1, 0x3170, 0, 0, 0, 0x100 } },
        { "[3ffe:2a00:100:7031::1].8080",                   STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0xfe3f, 0x2a, 1, 0x3170, 0, 0, 0, 0x100 } },
        { "[1080::8:800:200C:417A]:8080",                   STATUS_SUCCESS,             0,          0x901f,
            { 0x8010, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[1080::8:800:200C:417A]!8080",                   STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0x8010, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[::FFFF:129.144.52.38]:80",                      STATUS_SUCCESS,             0,          0x5000,
            { 0, 0, 0, 0, 0, 0xffff, 0x9081, 0x2634 } },
        { "[::FFFF:129.144.52.38]:-80",                     STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0, 0, 0, 0, 0, 0xffff, 0x9081, 0x2634 } },
        { "[::FFFF:129.144.52.38]:999999999999",            STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0, 0, 0, 0, 0, 0xffff, 0x9081, 0x2634 } },
        { "[::FFFF:129.144.52.38%-8]:80",                   STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0, 0, 0, 0, 0, 0xffff, 0x9081, 0x2634 } },
        { "[::FFFF:129.144.52.38]:80",                      STATUS_SUCCESS,             0,          0x5000,
            { 0, 0, 0, 0, 0, 0xffff, 0x9081, 0x2634 } },
        { "[12345::6:7:8]:80",                              STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { -1 } },
        { "[ff01::8:800:200C:417A%16]:8080",                STATUS_SUCCESS,             16,         0x901f,
            { 0x1ff, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[ff01::8:800:200C:417A%100]:8080",               STATUS_SUCCESS,             100,        0x901f,
            { 0x1ff, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[ff01::8:800:200C:417A%1000]:8080",              STATUS_SUCCESS,             1000,       0x901f,
            { 0x1ff, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[ff01::8:800:200C:417A%10000]:8080",             STATUS_SUCCESS,             10000,      0x901f,
            { 0x1ff, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[ff01::8:800:200C:417A%1000000]:8080",           STATUS_SUCCESS,             1000000,    0x901f,
            { 0x1ff, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[ff01::8:800:200C:417A%4294967295]:8080",        STATUS_SUCCESS,             0xffffffff, 0x901f,
            { 0x1ff, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[ff01::8:800:200C:417A%4294967296]:8080",        STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0x1ff, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[ff01::8:800:200C:417A%-1]:8080",                STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0x1ff, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[ff01::8:800:200C:417A%0]:8080",                 STATUS_SUCCESS,             0,          0x901f,
            { 0x1ff, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[ff01::8:800:200C:417A%1",                       STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0x1ff, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[ff01::8:800:200C:417A%0x1000]:8080",            STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0x1ff, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
        { "[ff01::8:800:200C:417A/16]:8080",                STATUS_INVALID_PARAMETER,   0xbadf00d,  0xbeef,
            { 0x1ff, 0, 0, 0, 0x800, 8, 0xc20, 0x7a41 } },
    };
    const unsigned int ipv6_ex_testcount = sizeof(ipv6_ex_tests) / sizeof(ipv6_ex_tests[0]);
    const char *simple_ip = "::";
    unsigned int i;

    if (!pRtlIpv6StringToAddressExW)
    {
        skip("RtlIpv6StringToAddressExW not available\n");
        /* we can continue, just not test W */
    }

    if (!pRtlIpv6StringToAddressExA)
    {
        skip("RtlIpv6StringToAddressExA not available\n");
        return;
    }

    res = pRtlIpv6StringToAddressExA(simple_ip, &ip, &scope, &port);
    ok(res == STATUS_SUCCESS, "[validate] res = 0x%08x, expected STATUS_SUCCESS\n", res);

    init_ip6(&ip, NULL);
    init_ip6(&expected_ip, NULL);
    scope = 0xbadf00d;
    port = 0xbeef;
    res = pRtlIpv6StringToAddressExA(NULL, &ip, &scope, &port);
    ok(res == STATUS_INVALID_PARAMETER,
       "[null string] res = 0x%08x, expected STATUS_INVALID_PARAMETER\n", res);
    ok(scope == 0xbadf00d, "[null string] scope = 0x%08x, expected 0xbadf00d\n", scope);
    ok(port == 0xbeef, "[null string] port = 0x%08x, expected 0xbeef\n", port);
    ok(!memcmp(&ip, &expected_ip, sizeof(ip)),
       "[null string] ip is changed, expected it not to change\n");


    init_ip6(&ip, NULL);
    scope = 0xbadf00d;
    port = 0xbeef;
    res = pRtlIpv6StringToAddressExA(simple_ip, NULL, &scope, &port);
    ok(res == STATUS_INVALID_PARAMETER,
       "[null result] res = 0x%08x, expected STATUS_INVALID_PARAMETER\n", res);
    ok(scope == 0xbadf00d, "[null result] scope = 0x%08x, expected 0xbadf00d\n", scope);
    ok(port == 0xbeef, "[null result] port = 0x%08x, expected 0xbeef\n", port);
    ok(!memcmp(&ip, &expected_ip, sizeof(ip)),
       "[null result] ip is changed, expected it not to change\n");

    init_ip6(&ip, NULL);
    scope = 0xbadf00d;
    port = 0xbeef;
    res = pRtlIpv6StringToAddressExA(simple_ip, &ip, NULL, &port);
    ok(res == STATUS_INVALID_PARAMETER,
       "[null scope] res = 0x%08x, expected STATUS_INVALID_PARAMETER\n", res);
    ok(scope == 0xbadf00d, "[null scope] scope = 0x%08x, expected 0xbadf00d\n", scope);
    ok(port == 0xbeef, "[null scope] port = 0x%08x, expected 0xbeef\n", port);
    ok(!memcmp(&ip, &expected_ip, sizeof(ip)),
       "[null scope] ip is changed, expected it not to change\n");

    init_ip6(&ip, NULL);
    scope = 0xbadf00d;
    port = 0xbeef;
    res = pRtlIpv6StringToAddressExA(simple_ip, &ip, &scope, NULL);
    ok(res == STATUS_INVALID_PARAMETER,
       "[null port] res = 0x%08x, expected STATUS_INVALID_PARAMETER\n", res);
    ok(scope == 0xbadf00d, "[null port] scope = 0x%08x, expected 0xbadf00d\n", scope);
    ok(port == 0xbeef, "[null port] port = 0x%08x, expected 0xbeef\n", port);
    ok(!memcmp(&ip, &expected_ip, sizeof(ip)),
       "[null port] ip is changed, expected it not to change\n");

    /* sanity check */
    ok(sizeof(ip) == sizeof(USHORT)* 8, "sizeof(ip)\n");

    /* first we run all ip related tests, to make sure someone didnt accidentally reimplement instead of re-use. */
    for (i = 0; i < ipv6_testcount; i++)
    {
        ULONG scope = 0xbadf00d;
        USHORT port = 0xbeef;
        NTSTATUS expect_ret = (ipv6_tests[i].flags & ex_fail_6) ? STATUS_INVALID_PARAMETER : ipv6_tests[i].res;

        if (ipv6_tests[i].flags & ex_skip_6)
            continue;

        init_ip6(&ip, NULL);
        res = pRtlIpv6StringToAddressExA(ipv6_tests[i].address, &ip, &scope, &port);
        compare_RtlIpv6StringToAddressExW(ipv6_tests[i].address, &ip, res, scope, port);

        /* make sure nothing was changed if this function fails. */
        if (res == STATUS_INVALID_PARAMETER)
        {
            ok(scope == 0xbadf00d, "[%s] scope = 0x%08x, expected 0xbadf00d\n",
               ipv6_tests[i].address, scope);
            ok(port == 0xbeef, "[%s] port = 0x%08x, expected 0xbeef\n",
               ipv6_tests[i].address, port);
        }
        else
        {
            ok(scope != 0xbadf00d, "[%s] scope = 0x%08x, not expected 0xbadf00d\n",
               ipv6_tests[i].address, scope);
            ok(port != 0xbeef, "[%s] port = 0x%08x, not expected 0xbeef\n",
               ipv6_tests[i].address, port);
        }

        if (ipv6_tests[i].flags & win_broken_6)
        {
            ok(res == expect_ret || broken(res == STATUS_INVALID_PARAMETER),
               "[%s] res = 0x%08x, expected 0x%08x\n", ipv6_tests[i].address, res, expect_ret);

            if (res == STATUS_INVALID_PARAMETER)
                continue;
        }
        else
        {
            ok(res == expect_ret, "[%s] res = 0x%08x, expected 0x%08x\n",
               ipv6_tests[i].address, res, expect_ret);
        }

        /* If ex fails but non-ex does not we cannot check if the part that is converted
           before it failed was correct, since there is no data for it in the table. */
        if (res == expect_ret)
        {
            init_ip6(&expected_ip, ipv6_tests[i].ip);
            ok(!memcmp(&ip, &expected_ip, sizeof(ip)),
               "[%s] ip = %x:%x:%x:%x:%x:%x:%x:%x, expected %x:%x:%x:%x:%x:%x:%x:%x\n",
               ipv6_tests[i].address,
               ip.s6_words[0], ip.s6_words[1], ip.s6_words[2], ip.s6_words[3],
               ip.s6_words[4], ip.s6_words[5], ip.s6_words[6], ip.s6_words[7],
               expected_ip.s6_words[0], expected_ip.s6_words[1], expected_ip.s6_words[2], expected_ip.s6_words[3],
               expected_ip.s6_words[4], expected_ip.s6_words[5], expected_ip.s6_words[6], expected_ip.s6_words[7]);
        }
    }

    /* now we run scope / port related tests */
    for (i = 0; i < ipv6_ex_testcount; i++)
    {
        scope = 0xbadf00d;
        port = 0xbeef;
        init_ip6(&ip, NULL);
        res = pRtlIpv6StringToAddressExA(ipv6_ex_tests[i].address, &ip, &scope, &port);
        compare_RtlIpv6StringToAddressExW(ipv6_ex_tests[i].address, &ip, res, scope, port);

        ok(res == ipv6_ex_tests[i].res, "[%s] res = 0x%08x, expected 0x%08x\n",
           ipv6_ex_tests[i].address, res, ipv6_ex_tests[i].res);
        ok(scope == ipv6_ex_tests[i].scope, "[%s] scope = 0x%08x, expected 0x%08x\n",
           ipv6_ex_tests[i].address, scope, ipv6_ex_tests[i].scope);
        ok(port == ipv6_ex_tests[i].port, "[%s] port = 0x%08x, expected 0x%08x\n",
           ipv6_ex_tests[i].address, port, ipv6_ex_tests[i].port);

        init_ip6(&expected_ip, ipv6_ex_tests[i].ip);
        ok(!memcmp(&ip, &expected_ip, sizeof(ip)),
           "[%s] ip = %x:%x:%x:%x:%x:%x:%x:%x, expected %x:%x:%x:%x:%x:%x:%x:%x\n",
           ipv6_ex_tests[i].address,
           ip.s6_words[0], ip.s6_words[1], ip.s6_words[2], ip.s6_words[3],
           ip.s6_words[4], ip.s6_words[5], ip.s6_words[6], ip.s6_words[7],
           expected_ip.s6_words[0], expected_ip.s6_words[1], expected_ip.s6_words[2], expected_ip.s6_words[3],
           expected_ip.s6_words[4], expected_ip.s6_words[5], expected_ip.s6_words[6], expected_ip.s6_words[7]);
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

static void test_RtlCompressBuffer(void)
{
    ULONG compress_workspace, decompress_workspace;
    static const UCHAR test_buffer[] = "WineWineWine";
    static UCHAR buf1[0x1000], buf2[0x1000];
    ULONG final_size, buf_size;
    UCHAR *workspace = NULL;
    NTSTATUS status;

    if (!pRtlCompressBuffer || !pRtlDecompressBuffer || !pRtlGetCompressionWorkSpaceSize)
    {
        win_skip("skipping RtlCompressBuffer tests, required functions not available\n");
        return;
    }

    compress_workspace = decompress_workspace = 0xdeadbeef;
    status = pRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1, &compress_workspace,
                                             &decompress_workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    ok(compress_workspace != 0, "got wrong compress_workspace %u\n", compress_workspace);
    workspace = HeapAlloc(GetProcessHeap(), 0, compress_workspace);
    ok(workspace != NULL, "HeapAlloc failed %d\n", GetLastError());

    /* test compression format / engine */
    final_size = 0xdeadbeef;
    status = pRtlCompressBuffer(COMPRESSION_FORMAT_NONE, test_buffer, sizeof(test_buffer),
                                buf1, sizeof(buf1) - 1, 4096, &final_size, workspace);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08x\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %u\n", final_size);

    final_size = 0xdeadbeef;
    status = pRtlCompressBuffer(COMPRESSION_FORMAT_DEFAULT, test_buffer, sizeof(test_buffer),
                                buf1, sizeof(buf1) - 1, 4096, &final_size, workspace);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08x\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %u\n", final_size);

    final_size = 0xdeadbeef;
    status = pRtlCompressBuffer(0xFF, test_buffer, sizeof(test_buffer),
                                buf1, sizeof(buf1) - 1, 4096, &final_size, workspace);
    ok(status == STATUS_UNSUPPORTED_COMPRESSION, "got wrong status 0x%08x\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %u\n", final_size);

    /* test compression */
    final_size = 0xdeadbeef;
    memset(buf1, 0x11, sizeof(buf1));
    status = pRtlCompressBuffer(COMPRESSION_FORMAT_LZNT1, test_buffer, sizeof(test_buffer),
                                buf1, sizeof(buf1), 4096, &final_size, workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    ok((*(WORD *)buf1 & 0x7000) == 0x3000, "no chunk signature found %04x\n", *(WORD *)buf1);
    todo_wine
    ok(final_size < sizeof(test_buffer), "got wrong final_size %u\n", final_size);

    /* test decompression */
    buf_size = final_size;
    final_size = 0xdeadbeef;
    memset(buf2, 0x11, sizeof(buf2));
    status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf2, sizeof(buf2),
                                  buf1, buf_size, &final_size);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    ok(final_size == sizeof(test_buffer), "got wrong final_size %u\n", final_size);
    ok(!memcmp(buf2, test_buffer, sizeof(test_buffer)), "got wrong decoded data\n");
    ok(buf2[sizeof(test_buffer)] == 0x11, "too many bytes written\n");

    /* buffer too small */
    final_size = 0xdeadbeef;
    memset(buf1, 0x11, sizeof(buf1));
    status = pRtlCompressBuffer(COMPRESSION_FORMAT_LZNT1, test_buffer, sizeof(test_buffer),
                                buf1, 4, 4096, &final_size, workspace);
    ok(status == STATUS_BUFFER_TOO_SMALL, "got wrong status 0x%08x\n", status);

    HeapFree(GetProcessHeap(), 0, workspace);
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

    /* test invalid format / engine */
    status = pRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_NONE, &compress_workspace,
                                             &decompress_workspace);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08x\n", status);

    status = pRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_DEFAULT, &compress_workspace,
                                             &decompress_workspace);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08x\n", status);

    status = pRtlGetCompressionWorkSpaceSize(0xFF, &compress_workspace, &decompress_workspace);
    ok(status == STATUS_UNSUPPORTED_COMPRESSION, "got wrong status 0x%08x\n", status);

    /* test LZNT1 with normal and maximum compression */
    compress_workspace = decompress_workspace = 0xdeadbeef;
    status = pRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1, &compress_workspace,
                                             &decompress_workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    ok(compress_workspace != 0, "got wrong compress_workspace %u\n", compress_workspace);
    ok(decompress_workspace == 0x1000, "got wrong decompress_workspace %u\n", decompress_workspace);

    compress_workspace = decompress_workspace = 0xdeadbeef;
    status = pRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM,
                                             &compress_workspace, &decompress_workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08x\n", status);
    ok(compress_workspace != 0, "got wrong compress_workspace %u\n", compress_workspace);
    ok(decompress_workspace == 0x1000, "got wrong decompress_workspace %u\n", decompress_workspace);
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

#define DECOMPRESS_BROKEN_FRAGMENT     1 /* < Win 7 */
#define DECOMPRESS_BROKEN_TRUNCATED    2 /* broken on all machines */

static void test_RtlDecompressBuffer(void)
{
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
    status = pRtlDecompressBuffer(COMPRESSION_FORMAT_NONE, buf, sizeof(buf), test_lznt[0].compressed,
                                  test_lznt[0].compressed_size, &final_size);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08x\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %u\n", final_size);

    final_size = 0xdeadbeef;
    status = pRtlDecompressBuffer(COMPRESSION_FORMAT_DEFAULT, buf, sizeof(buf), test_lznt[0].compressed,
                                  test_lznt[0].compressed_size, &final_size);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08x\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %u\n", final_size);

    final_size = 0xdeadbeef;
    status = pRtlDecompressBuffer(0xFF, buf, sizeof(buf), test_lznt[0].compressed,
                                  test_lznt[0].compressed_size, &final_size);
    ok(status == STATUS_UNSUPPORTED_COMPRESSION, "got wrong status 0x%08x\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %u\n", final_size);

    /* regular tests for RtlDecompressBuffer */
    for (i = 0; i < sizeof(test_lznt) / sizeof(test_lznt[0]); i++)
    {
        trace("Running test %d (compressed_size=%u, uncompressed_size=%u, status=0x%08x)\n",
              i, test_lznt[i].compressed_size, test_lznt[i].uncompressed_size, test_lznt[i].status);

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
               "%d: got wrong final_size %u\n", i, final_size);
            ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size),
               "%d: got wrong decoded data\n", i);
            ok(buf[test_lznt[i].uncompressed_size] == 0x11,
               "%d: buf[%u] was modified\n", i, test_lznt[i].uncompressed_size);
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
               "%d: got wrong final_size %u\n", i, final_size);
            ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size),
               "%d: got wrong decoded data\n", i);
            ok(buf[test_lznt[i].uncompressed_size] == 0x11,
               "%d: buf[%u] was modified\n", i, test_lznt[i].uncompressed_size);
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
                   "%d: got wrong final_size %u\n", i, final_size);
                ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size),
                   "%d: got wrong decoded data\n", i);
                ok(buf[test_lznt[i].uncompressed_size] == 0x11,
                   "%d: buf[%u] was modified\n", i, test_lznt[i].uncompressed_size);
            }
        }

        /* test with smaller output size */
        if (test_lznt[i].uncompressed_size > 1)
        {
            final_size = 0xdeadbeef;
            memset(buf, 0x11, sizeof(buf));
            status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, test_lznt[i].uncompressed_size - 1,
                                          test_lznt[i].compressed, test_lznt[i].compressed_size, &final_size);
            if (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_TRUNCATED)
                todo_wine
                ok(status == STATUS_BAD_COMPRESSION_BUFFER, "%d: got wrong status 0x%08x\n", i, status);
            else
                ok(status == test_lznt[i].status, "%d: got wrong status 0x%08x\n", i, status);
            if (!status)
            {
                ok(final_size == test_lznt[i].uncompressed_size - 1,
                   "%d: got wrong final_size %u\n", i, final_size);
                ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size - 1),
                   "%d: got wrong decoded data\n", i);
                ok(buf[test_lznt[i].uncompressed_size - 1] == 0x11,
                   "%d: buf[%u] was modified\n", i, test_lznt[i].uncompressed_size - 1);
            }
        }

        /* test with zero output size */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = pRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, 0, test_lznt[i].compressed,
                                      test_lznt[i].compressed_size, &final_size);
        if (is_incomplete_chunk(test_lznt[i].compressed, test_lznt[i].compressed_size, FALSE))
            ok(status == STATUS_BAD_COMPRESSION_BUFFER, "%d: got wrong status 0x%08x\n", i, status);
        else
        {
            ok(status == STATUS_SUCCESS, "%d: got wrong status 0x%08x\n", i, status);
            ok(final_size == 0, "%d: got wrong final_size %u\n", i, final_size);
            ok(buf[0] == 0x11, "%d: buf[0] was modified\n", i);
        }

        /* test RtlDecompressFragment with offset = 0 */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                        test_lznt[i].compressed_size, 0, &final_size, workspace);
        if (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)
            todo_wine
            ok(status == STATUS_BAD_COMPRESSION_BUFFER, "%d: got wrong status 0x%08x\n", i, status);
        else
            ok(status == test_lznt[i].status, "%d: got wrong status 0x%08x\n", i, status);
        if (!status)
        {
            ok(final_size == test_lznt[i].uncompressed_size,
               "%d: got wrong final_size %u\n", i, final_size);
            ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size),
               "%d: got wrong decoded data\n", i);
            ok(buf[test_lznt[i].uncompressed_size] == 0x11,
               "%d: buf[%u] was modified\n", i, test_lznt[i].uncompressed_size);
        }

        /* test RtlDecompressFragment with offset = 1 */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                        test_lznt[i].compressed_size, 1, &final_size, workspace);
        if (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)
            todo_wine
            ok(status == STATUS_BAD_COMPRESSION_BUFFER, "%d: got wrong status 0x%08x\n", i, status);
        else
            ok(status == test_lznt[i].status, "%d: got wrong status 0x%08x\n", i, status);
        if (!status)
        {
            if (test_lznt[i].uncompressed_size == 0)
            {
                todo_wine
                ok(final_size == 4095, "%d: got wrong final_size %u\n", i, final_size);
                /* Buffer doesn't contain any useful value on Windows */
                ok(buf[4095] == 0x11, "%d: buf[4095] was modified\n", i);
            }
            else
            {
                ok(final_size == test_lznt[i].uncompressed_size - 1,
                   "%d: got wrong final_size %u\n", i, final_size);
                ok(!memcmp(buf, test_lznt[i].uncompressed + 1, test_lznt[i].uncompressed_size - 1),
                   "%d: got wrong decoded data\n", i);
                ok(buf[test_lznt[i].uncompressed_size - 1] == 0x11,
                   "%d: buf[%u] was modified\n", i, test_lznt[i].uncompressed_size - 1);
            }
        }

        /* test RtlDecompressFragment with offset = 4095 */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                        test_lznt[i].compressed_size, 4095, &final_size, workspace);
        if (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)
            todo_wine
            ok(status == STATUS_BAD_COMPRESSION_BUFFER, "%d: got wrong status 0x%08x\n", i, status);
        else
            ok(status == test_lznt[i].status, "%d: got wrong status 0x%08x\n", i, status);
        if (!status)
        {
            todo_wine
            ok(final_size == 1, "%d: got wrong final_size %u\n", i, final_size);
            todo_wine
            ok(buf[0] == 0, "%d: padding is not zero\n", i);
            ok(buf[1] == 0x11, "%d: buf[1] was modified\n", i);
        }

        /* test RtlDecompressFragment with offset = 4096 */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = pRtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                        test_lznt[i].compressed_size, 4096, &final_size, workspace);
        expected_status = is_incomplete_chunk(test_lznt[i].compressed, test_lznt[i].compressed_size, TRUE) ?
                          test_lznt[i].status : STATUS_SUCCESS;
        ok(status == expected_status, "%d: got wrong status 0x%08x, expected 0x%08x\n", i, status, expected_status);
        if (!status)
        {
            ok(final_size == 0, "%d: got wrong final_size %u\n", i, final_size);
            ok(buf[0] == 0x11, "%d: buf[4096] was modified\n", i);
        }
    }
}

#undef DECOMPRESS_BROKEN_FRAGMENT
#undef DECOMPRESS_BROKEN_TRUNCATED

struct critsect_locked_info
{
    CRITICAL_SECTION crit;
    HANDLE semaphores[2];
};

static DWORD WINAPI critsect_locked_thread(void *param)
{
    struct critsect_locked_info *info = param;
    DWORD ret;

    ret = pRtlIsCriticalSectionLocked(&info->crit);
    ok(ret == TRUE, "expected TRUE, got %u\n", ret);
    ret = pRtlIsCriticalSectionLockedByThread(&info->crit);
    ok(ret == FALSE, "expected FALSE, got %u\n", ret);

    ReleaseSemaphore(info->semaphores[0], 1, NULL);
    ret = WaitForSingleObject(info->semaphores[1], 1000);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %u\n", ret);

    ret = pRtlIsCriticalSectionLocked(&info->crit);
    ok(ret == FALSE, "expected FALSE, got %u\n", ret);
    ret = pRtlIsCriticalSectionLockedByThread(&info->crit);
    ok(ret == FALSE, "expected FALSE, got %u\n", ret);

    EnterCriticalSection(&info->crit);

    ret = pRtlIsCriticalSectionLocked(&info->crit);
    ok(ret == TRUE, "expected TRUE, got %u\n", ret);
    ret = pRtlIsCriticalSectionLockedByThread(&info->crit);
    ok(ret == TRUE, "expected TRUE, got %u\n", ret);

    ReleaseSemaphore(info->semaphores[0], 1, NULL);
    ret = WaitForSingleObject(info->semaphores[1], 1000);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %u\n", ret);

    LeaveCriticalSection(&info->crit);
    return 0;
}

static void test_RtlIsCriticalSectionLocked(void)
{
    struct critsect_locked_info info;
    HANDLE thread;
    BOOL ret;

    if (!pRtlIsCriticalSectionLocked || !pRtlIsCriticalSectionLockedByThread)
    {
        win_skip("skipping RtlIsCriticalSectionLocked tests, required functions not available\n");
        return;
    }

    InitializeCriticalSection(&info.crit);
    info.semaphores[0] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(info.semaphores[0] != NULL, "CreateSemaphore failed with %u\n", GetLastError());
    info.semaphores[1] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(info.semaphores[1] != NULL, "CreateSemaphore failed with %u\n", GetLastError());

    ret = pRtlIsCriticalSectionLocked(&info.crit);
    ok(ret == FALSE, "expected FALSE, got %u\n", ret);
    ret = pRtlIsCriticalSectionLockedByThread(&info.crit);
    ok(ret == FALSE, "expected FALSE, got %u\n", ret);

    EnterCriticalSection(&info.crit);

    ret = pRtlIsCriticalSectionLocked(&info.crit);
    ok(ret == TRUE, "expected TRUE, got %u\n", ret);
    ret = pRtlIsCriticalSectionLockedByThread(&info.crit);
    ok(ret == TRUE, "expected TRUE, got %u\n", ret);

    thread = CreateThread(NULL, 0, critsect_locked_thread, &info, 0, NULL);
    ok(thread != NULL, "CreateThread failed with %u\n", GetLastError());
    ret = WaitForSingleObject(info.semaphores[0], 1000);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %u\n", ret);

    LeaveCriticalSection(&info.crit);

    ReleaseSemaphore(info.semaphores[1], 1, NULL);
    ret = WaitForSingleObject(info.semaphores[0], 1000);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %u\n", ret);

    ret = pRtlIsCriticalSectionLocked(&info.crit);
    ok(ret == TRUE, "expected TRUE, got %u\n", ret);
    ret = pRtlIsCriticalSectionLockedByThread(&info.crit);
    ok(ret == FALSE, "expected FALSE, got %u\n", ret);

    ReleaseSemaphore(info.semaphores[1], 1, NULL);
    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %u\n", ret);

    CloseHandle(thread);
    CloseHandle(info.semaphores[0]);
    CloseHandle(info.semaphores[1]);
    DeleteCriticalSection(&info.crit);
}

static void test_RtlInitializeCriticalSectionEx(void)
{
    static const CRITICAL_SECTION_DEBUG *no_debug = (void *)~(ULONG_PTR)0;
    CRITICAL_SECTION cs;

    if (!pRtlInitializeCriticalSectionEx)
    {
        win_skip("RtlInitializeCriticalSectionEx is not available\n");
        return;
    }

    memset(&cs, 0x11, sizeof(cs));
    pRtlInitializeCriticalSectionEx(&cs, 0, 0);
    ok((cs.DebugInfo != NULL && cs.DebugInfo != no_debug) || broken(cs.DebugInfo == no_debug) /* >= Win 8 */,
       "expected DebugInfo != NULL and DebugInfo != ~0, got %p\n", cs.DebugInfo);
    ok(cs.LockCount == -1, "expected LockCount == -1, got %d\n", cs.LockCount);
    ok(cs.RecursionCount == 0, "expected RecursionCount == 0, got %d\n", cs.RecursionCount);
    ok(cs.LockSemaphore == NULL, "expected LockSemaphore == NULL, got %p\n", cs.LockSemaphore);
    ok(cs.SpinCount == 0 || broken(cs.SpinCount != 0) /* >= Win 8 */,
       "expected SpinCount == 0, got %ld\n", cs.SpinCount);
    RtlDeleteCriticalSection((PRTL_CRITICAL_SECTION)&cs);

    memset(&cs, 0x11, sizeof(cs));
    pRtlInitializeCriticalSectionEx(&cs, 0, RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO);
    todo_wine
    ok(cs.DebugInfo == no_debug, "expected DebugInfo == ~0, got %p\n", cs.DebugInfo);
    ok(cs.LockCount == -1, "expected LockCount == -1, got %d\n", cs.LockCount);
    ok(cs.RecursionCount == 0, "expected RecursionCount == 0, got %d\n", cs.RecursionCount);
    ok(cs.LockSemaphore == NULL, "expected LockSemaphore == NULL, got %p\n", cs.LockSemaphore);
    ok(cs.SpinCount == 0 || broken(cs.SpinCount != 0) /* >= Win 8 */,
       "expected SpinCount == 0, got %ld\n", cs.SpinCount);
    RtlDeleteCriticalSection((PRTL_CRITICAL_SECTION)&cs);
}

static void test_RtlLeaveCriticalSection(void)
{
    RTL_CRITICAL_SECTION cs;
    NTSTATUS status;

    if (!pRtlInitializeCriticalSectionEx)
        return; /* Skip winxp */

    status = RtlInitializeCriticalSection(&cs);
    ok(!status, "RtlInitializeCriticalSection failed: %x\n", status);

    status = RtlEnterCriticalSection(&cs);
    ok(!status, "RtlEnterCriticalSection failed: %x\n", status);
    todo_wine
    ok(cs.LockCount == -2, "expected LockCount == -2, got %d\n", cs.LockCount);
    ok(cs.RecursionCount == 1, "expected RecursionCount == 1, got %d\n", cs.RecursionCount);
    ok(cs.OwningThread == ULongToHandle(GetCurrentThreadId()), "unexpected OwningThread\n");

    status = RtlLeaveCriticalSection(&cs);
    ok(!status, "RtlLeaveCriticalSection failed: %x\n", status);
    ok(cs.LockCount == -1, "expected LockCount == -1, got %d\n", cs.LockCount);
    ok(cs.RecursionCount == 0, "expected RecursionCount == 0, got %d\n", cs.RecursionCount);
    ok(!cs.OwningThread, "unexpected OwningThread %p\n", cs.OwningThread);

    /*
     * Trying to leave a section that wasn't acquired modifies RecursionCount to an invalid value,
     * but doesn't modify LockCount so that an attempt to enter the section later will work.
     */
    status = RtlLeaveCriticalSection(&cs);
    ok(!status, "RtlLeaveCriticalSection failed: %x\n", status);
    ok(cs.LockCount == -1, "expected LockCount == -1, got %d\n", cs.LockCount);
    ok(cs.RecursionCount == -1, "expected RecursionCount == -1, got %d\n", cs.RecursionCount);
    ok(!cs.OwningThread, "unexpected OwningThread %p\n", cs.OwningThread);

    /* and again */
    status = RtlLeaveCriticalSection(&cs);
    ok(!status, "RtlLeaveCriticalSection failed: %x\n", status);
    ok(cs.LockCount == -1, "expected LockCount == -1, got %d\n", cs.LockCount);
    ok(cs.RecursionCount == -2, "expected RecursionCount == -2, got %d\n", cs.RecursionCount);
    ok(!cs.OwningThread, "unexpected OwningThread %p\n", cs.OwningThread);

    /* entering section fixes RecursionCount */
    status = RtlEnterCriticalSection(&cs);
    ok(!status, "RtlEnterCriticalSection failed: %x\n", status);
    todo_wine
    ok(cs.LockCount == -2, "expected LockCount == -2, got %d\n", cs.LockCount);
    ok(cs.RecursionCount == 1, "expected RecursionCount == 1, got %d\n", cs.RecursionCount);
    ok(cs.OwningThread == ULongToHandle(GetCurrentThreadId()), "unexpected OwningThread\n");

    status = RtlLeaveCriticalSection(&cs);
    ok(!status, "RtlLeaveCriticalSection failed: %x\n", status);
    ok(cs.LockCount == -1, "expected LockCount == -1, got %d\n", cs.LockCount);
    ok(cs.RecursionCount == 0, "expected RecursionCount == 0, got %d\n", cs.RecursionCount);
    ok(!cs.OwningThread, "unexpected OwningThread %p\n", cs.OwningThread);

    status = RtlDeleteCriticalSection(&cs);
    ok(!status, "RtlDeleteCriticalSection failed: %x\n", status);
}

struct ldr_enum_context
{
    BOOL abort;
    BOOL found;
    int  count;
};

static void WINAPI ldr_enum_callback(LDR_MODULE *module, void *context, BOOLEAN *stop)
{
    static const WCHAR ntdllW[] = {'n','t','d','l','l','.','d','l','l',0};
    struct ldr_enum_context *ctx = context;

    if (!lstrcmpiW(module->BaseDllName.Buffer, ntdllW))
        ctx->found = TRUE;

    ctx->count++;
    *stop = ctx->abort;
}

static void test_LdrEnumerateLoadedModules(void)
{
    struct ldr_enum_context ctx;
    NTSTATUS status;

    if (!pLdrEnumerateLoadedModules)
    {
        win_skip("LdrEnumerateLoadedModules not available\n");
        return;
    }

    ctx.abort = FALSE;
    ctx.found = FALSE;
    ctx.count = 0;
    status = pLdrEnumerateLoadedModules(NULL, ldr_enum_callback, &ctx);
    ok(status == STATUS_SUCCESS, "LdrEnumerateLoadedModules failed with %08x\n", status);
    ok(ctx.count > 1, "Expected more than one module, got %d\n", ctx.count);
    ok(ctx.found, "Could not find ntdll in list of modules\n");

    ctx.abort = TRUE;
    ctx.count = 0;
    status = pLdrEnumerateLoadedModules(NULL, ldr_enum_callback, &ctx);
    ok(status == STATUS_SUCCESS, "LdrEnumerateLoadedModules failed with %08x\n", status);
    ok(ctx.count == 1, "Expected exactly one module, got %d\n", ctx.count);

    status = pLdrEnumerateLoadedModules((void *)0x1, ldr_enum_callback, (void *)0xdeadbeef);
    ok(status == STATUS_INVALID_PARAMETER, "expected STATUS_INVALID_PARAMETER, got 0x%08x\n", status);

    status = pLdrEnumerateLoadedModules((void *)0xdeadbeef, ldr_enum_callback, (void *)0xdeadbeef);
    ok(status == STATUS_INVALID_PARAMETER, "expected STATUS_INVALID_PARAMETER, got 0x%08x\n", status);

    status = pLdrEnumerateLoadedModules(NULL, NULL, (void *)0xdeadbeef);
    ok(status == STATUS_INVALID_PARAMETER, "expected STATUS_INVALID_PARAMETER, got 0x%08x\n", status);
}

static void test_RtlMakeSelfRelativeSD(void)
{
    char buf[sizeof(SECURITY_DESCRIPTOR_RELATIVE) + 4];
    SECURITY_DESCRIPTOR_RELATIVE *sd_rel = (SECURITY_DESCRIPTOR_RELATIVE *)buf;
    SECURITY_DESCRIPTOR sd;
    NTSTATUS status;
    DWORD len;

    if (!pRtlMakeSelfRelativeSD || !pRtlAbsoluteToSelfRelativeSD)
    {
        win_skip( "RtlMakeSelfRelativeSD/RtlAbsoluteToSelfRelativeSD not available\n" );
        return;
    }

    memset( &sd, 0, sizeof(sd) );
    sd.Revision = SECURITY_DESCRIPTOR_REVISION;

    len = 0;
    status = pRtlMakeSelfRelativeSD( &sd, NULL, &len );
    ok( status == STATUS_BUFFER_TOO_SMALL, "got %08x\n", status );
    ok( len == sizeof(*sd_rel), "got %u\n", len );

    len += 4;
    status = pRtlMakeSelfRelativeSD( &sd, sd_rel, &len );
    ok( status == STATUS_SUCCESS, "got %08x\n", status );
    ok( len == sizeof(*sd_rel) + 4, "got %u\n", len );

    len = 0;
    status = pRtlAbsoluteToSelfRelativeSD( &sd, NULL, &len );
    ok( status == STATUS_BUFFER_TOO_SMALL, "got %08x\n", status );
    ok( len == sizeof(*sd_rel), "got %u\n", len );

    len += 4;
    status = pRtlAbsoluteToSelfRelativeSD( &sd, sd_rel, &len );
    ok( status == STATUS_SUCCESS, "got %08x\n", status );
    ok( len == sizeof(*sd_rel) + 4, "got %u\n", len );

    sd.Control = SE_SELF_RELATIVE;
    status = pRtlMakeSelfRelativeSD( &sd, sd_rel, &len );
    ok( status == STATUS_SUCCESS, "got %08x\n", status );
    ok( len == sizeof(*sd_rel) + 4, "got %u\n", len );

    status = pRtlAbsoluteToSelfRelativeSD( &sd, sd_rel, &len );
    ok( status == STATUS_BAD_DESCRIPTOR_FORMAT, "got %08x\n", status );
}

static void test_RtlQueryPackageIdentity(void)
{
    const WCHAR programW[] = {'M','i','c','r','o','s','o','f','t','.','W','i','n','d','o','w','s','.',
                              'P','h','o','t','o','s','_','8','w','e','k','y','b','3','d','8','b','b','w','e','!','A','p','p',0};
    const WCHAR fullnameW[] = {'M','i','c','r','o','s','o','f','t','.','W','i','n','d','o','w','s','.',
                               'P','h','o','t','o','s', 0};
    const WCHAR appidW[] = {'A','p','p',0};
    IApplicationActivationManager *manager;
    WCHAR buf1[MAX_PATH], buf2[MAX_PATH];
    HANDLE process, token;
    SIZE_T size1, size2;
    NTSTATUS status;
    DWORD processid;
    HRESULT hr;
    BOOL ret;

    if (!pRtlQueryPackageIdentity)
    {
        win_skip("RtlQueryPackageIdentity not available\n");
        return;
    }

    size1 = size2 = MAX_PATH * sizeof(WCHAR);
    status = pRtlQueryPackageIdentity((HANDLE)~(ULONG_PTR)3, buf1, &size1, buf2, &size2, NULL);
    ok(status == STATUS_NOT_FOUND, "expected STATUS_NOT_FOUND, got %08x\n", status);

    CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    hr = CoCreateInstance(&CLSID_ApplicationActivationManager, NULL, CLSCTX_LOCAL_SERVER,
                          &IID_IApplicationActivationManager, (void **)&manager);
    if (FAILED(hr))
    {
        todo_wine win_skip("Failed to create ApplicationActivationManager (%x)\n", hr);
        goto done;
    }

    hr = IApplicationActivationManager_ActivateApplication(manager, programW, NULL,
                                                           AO_NOERRORUI, &processid);
    if (FAILED(hr))
    {
        todo_wine win_skip("Failed to start program (%x)\n", hr);
        IApplicationActivationManager_Release(manager);
        goto done;
    }

    process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE, processid);
    ok(process != NULL, "OpenProcess failed with %u\n", GetLastError());
    ret = OpenProcessToken(process, TOKEN_QUERY, &token);
    ok(ret, "OpenProcessToken failed with error %u\n", GetLastError());

    size1 = size2 = MAX_PATH * sizeof(WCHAR);
    status = pRtlQueryPackageIdentity(token, buf1, &size1, buf2, &size2, NULL);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);

    ok(!memcmp(buf1, fullnameW, sizeof(fullnameW) - sizeof(WCHAR)),
       "Expected buf1 to begin with %s, got %s\n", wine_dbgstr_w(fullnameW), wine_dbgstr_w(buf1));
    ok(size1 >= sizeof(WCHAR) && !(size1 % sizeof(WCHAR)), "Unexpected size1 = %lu\n", size1);
    ok(buf1[size1 / sizeof(WCHAR) - 1] == 0, "Expected buf1[%lu] == 0\n", size1 / sizeof(WCHAR) - 1);

    ok(!lstrcmpW(buf2, appidW), "Expected buf2 to be %s, got %s\n", wine_dbgstr_w(appidW), wine_dbgstr_w(buf2));
    ok(size2 >= sizeof(WCHAR) && !(size2 % sizeof(WCHAR)), "Unexpected size2 = %lu\n", size2);
    ok(buf2[size2 / sizeof(WCHAR) - 1] == 0, "Expected buf2[%lu] == 0\n", size2 / sizeof(WCHAR) - 1);

    CloseHandle(token);
    TerminateProcess(process, 0);
    CloseHandle(process);

done:
    CoUninitialize();
}

static DWORD (CALLBACK *orig_entry)(HMODULE,DWORD,LPVOID);
static DWORD *dll_main_data;

static inline void *get_rva( HMODULE module, DWORD va )
{
    return (void *)((char *)module + va);
}

static void CALLBACK ldr_notify_callback1(ULONG reason, LDR_DLL_NOTIFICATION_DATA *data, void *context)
{
    const IMAGE_IMPORT_DESCRIPTOR *imports;
    const IMAGE_THUNK_DATA *import_list;
    IMAGE_THUNK_DATA *thunk_list;
    DWORD *calls = context;
    LIST_ENTRY *mark;
    LDR_MODULE *mod;
    ULONG size;
    int i, j;

    *calls <<= 4;
    *calls |= reason;

    ok(data->Loaded.Flags == 0, "Expected flags 0, got %x\n", data->Loaded.Flags);
    ok(!lstrcmpiW(data->Loaded.BaseDllName->Buffer, ws2_32dllW), "Expected ws2_32.dll, got %s\n",
       wine_dbgstr_w(data->Loaded.BaseDllName->Buffer));
    ok(!!data->Loaded.DllBase, "Expected non zero base address\n");
    ok(data->Loaded.SizeOfImage, "Expected non zero image size\n");

    /* expect module to be last module listed in LdrData load order list */
    mark = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;
    mod = CONTAINING_RECORD(mark->Blink, LDR_MODULE, InMemoryOrderModuleList);
    ok(mod->BaseAddress == data->Loaded.DllBase, "Expected base address %p, got %p\n",
       data->Loaded.DllBase, mod->BaseAddress);
    ok(!lstrcmpiW(mod->BaseDllName.Buffer, ws2_32dllW), "Expected ws2_32.dll, got %s\n",
       wine_dbgstr_w(mod->BaseDllName.Buffer));

    /* show that imports have already been resolved */
    imports = RtlImageDirectoryEntryToData(data->Loaded.DllBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size);
    ok(!!imports, "Expected dll to have imports\n");

    for (i = 0; imports[i].Name; i++)
    {
        thunk_list = get_rva(data->Loaded.DllBase, (DWORD)imports[i].FirstThunk);
        if (imports[i].OriginalFirstThunk)
            import_list = get_rva(data->Loaded.DllBase, (DWORD)imports[i].OriginalFirstThunk);
        else
            import_list = thunk_list;

        for (j = 0; import_list[j].u1.Ordinal; j++)
        {
            ok(thunk_list[j].u1.AddressOfData > data->Loaded.SizeOfImage,
               "Import has not been resolved: %p\n", (void*)thunk_list[j].u1.Function);
        }
    }
}

static void CALLBACK ldr_notify_callback2(ULONG reason, LDR_DLL_NOTIFICATION_DATA *data, void *context)
{
    DWORD *calls = context;
    *calls <<= 4;
    *calls |= reason + 2;
}

static BOOL WINAPI fake_dll_main(HINSTANCE instance, DWORD reason, void* reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        *dll_main_data <<= 4;
        *dll_main_data |= 3;
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        *dll_main_data <<= 4;
        *dll_main_data |= 4;
    }
    return orig_entry(instance, reason, reserved);
}

static void CALLBACK ldr_notify_callback_dll_main(ULONG reason, LDR_DLL_NOTIFICATION_DATA *data, void *context)
{
    DWORD *calls = context;
    LIST_ENTRY *mark;
    LDR_MODULE *mod;

    *calls <<= 4;
    *calls |= reason;

    if (reason != LDR_DLL_NOTIFICATION_REASON_LOADED)
        return;

    mark = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;
    mod = CONTAINING_RECORD(mark->Blink, LDR_MODULE, InMemoryOrderModuleList);
    ok(mod->BaseAddress == data->Loaded.DllBase, "Expected base address %p, got %p\n",
       data->Loaded.DllBase, mod->BaseAddress);
    if (mod->BaseAddress != data->Loaded.DllBase)
       return;

    orig_entry = mod->EntryPoint;
    mod->EntryPoint = fake_dll_main;
    dll_main_data = calls;
}

static BOOL WINAPI fake_dll_main_fail(HINSTANCE instance, DWORD reason, void* reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        *dll_main_data <<= 4;
        *dll_main_data |= 3;
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        *dll_main_data <<= 4;
        *dll_main_data |= 4;
    }
    return FALSE;
}

static void CALLBACK ldr_notify_callback_fail(ULONG reason, LDR_DLL_NOTIFICATION_DATA *data, void *context)
{
    DWORD *calls = context;
    LIST_ENTRY *mark;
    LDR_MODULE *mod;

    *calls <<= 4;
    *calls |= reason;

    if (reason != LDR_DLL_NOTIFICATION_REASON_LOADED)
        return;

    mark = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;
    mod = CONTAINING_RECORD(mark->Blink, LDR_MODULE, InMemoryOrderModuleList);
    ok(mod->BaseAddress == data->Loaded.DllBase, "Expected base address %p, got %p\n",
       data->Loaded.DllBase, mod->BaseAddress);
    if (mod->BaseAddress != data->Loaded.DllBase)
       return;

    orig_entry = mod->EntryPoint;
    mod->EntryPoint = fake_dll_main_fail;
    dll_main_data = calls;
}

static void CALLBACK ldr_notify_callback_imports(ULONG reason, LDR_DLL_NOTIFICATION_DATA *data, void *context)
{
    DWORD *calls = context;

    if (reason != LDR_DLL_NOTIFICATION_REASON_LOADED)
        return;

    if (!lstrcmpiW(data->Loaded.BaseDllName->Buffer, crypt32dllW))
    {
        *calls <<= 4;
        *calls |= 1;
    }

    if (!lstrcmpiW(data->Loaded.BaseDllName->Buffer, wintrustdllW))
    {
        *calls <<= 4;
        *calls |= 2;
    }
}

static void test_LdrRegisterDllNotification(void)
{
    void *cookie, *cookie2;
    NTSTATUS status;
    HMODULE mod;
    DWORD calls;

    if (!pLdrRegisterDllNotification || !pLdrUnregisterDllNotification)
    {
        win_skip("Ldr(Un)RegisterDllNotification not available\n");
        return;
    }

    /* generic test */
    status = pLdrRegisterDllNotification(0, ldr_notify_callback1, &calls, &cookie);
    ok(!status, "Expected STATUS_SUCCESS, got %08x\n", status);

    calls = 0;
    mod = LoadLibraryW(ws2_32dllW);
    ok(!!mod, "Failed to load library: %d\n", GetLastError());
    ok(calls == LDR_DLL_NOTIFICATION_REASON_LOADED, "Expected LDR_DLL_NOTIFICATION_REASON_LOADED, got %x\n", calls);

    calls = 0;
    FreeLibrary(mod);
    ok(calls == LDR_DLL_NOTIFICATION_REASON_UNLOADED, "Expected LDR_DLL_NOTIFICATION_REASON_UNLOADED, got %x\n", calls);

    /* test order of callbacks */
    status = pLdrRegisterDllNotification(0, ldr_notify_callback2, &calls, &cookie2);
    ok(!status, "Expected STATUS_SUCCESS, got %08x\n", status);

    calls = 0;
    mod = LoadLibraryW(ws2_32dllW);
    ok(!!mod, "Failed to load library: %d\n", GetLastError());
    ok(calls == 0x13, "Expected order 0x13, got %x\n", calls);

    calls = 0;
    FreeLibrary(mod);
    ok(calls == 0x24, "Expected order 0x24, got %x\n", calls);

    pLdrUnregisterDllNotification(cookie2);
    pLdrUnregisterDllNotification(cookie);

    /* test dll main order */
    status = pLdrRegisterDllNotification(0, ldr_notify_callback_dll_main, &calls, &cookie);
    ok(!status, "Expected STATUS_SUCCESS, got %08x\n", status);

    calls = 0;
    mod = LoadLibraryW(ws2_32dllW);
    ok(!!mod, "Failed to load library: %d\n", GetLastError());
    ok(calls == 0x13, "Expected order 0x13, got %x\n", calls);

    calls = 0;
    FreeLibrary(mod);
    ok(calls == 0x42, "Expected order 0x42, got %x\n", calls);

    pLdrUnregisterDllNotification(cookie);

    /* test dll main order */
    status = pLdrRegisterDllNotification(0, ldr_notify_callback_fail, &calls, &cookie);
    ok(!status, "Expected STATUS_SUCCESS, got %08x\n", status);

    calls = 0;
    mod = LoadLibraryW(ws2_32dllW);
    ok(!mod, "Expected library to fail loading\n");
    ok(calls == 0x1342, "Expected order 0x1342, got %x\n", calls);

    pLdrUnregisterDllNotification(cookie);

    /* test dll with dependencies */
    status = pLdrRegisterDllNotification(0, ldr_notify_callback_imports, &calls, &cookie);
    ok(!status, "Expected STATUS_SUCCESS, got %08x\n", status);

    calls = 0;
    mod = LoadLibraryW(wintrustdllW);
    ok(!!mod, "Failed to load library: %d\n", GetLastError());
    ok(calls == 0x12, "Expected order 0x12, got %x\n", calls);

    FreeLibrary(mod);
    pLdrUnregisterDllNotification(cookie);
}

START_TEST(rtl)
{
    InitFunctionPtrs();

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
    test_RtlIpv6AddressToString();
    test_RtlIpv6AddressToStringEx();
    test_RtlIpv6StringToAddress();
    test_RtlIpv6StringToAddressEx();
    test_LdrAddRefDll();
    test_LdrLockLoaderLock();
    test_RtlCompressBuffer();
    test_RtlGetCompressionWorkSpaceSize();
    test_RtlDecompressBuffer();
    test_RtlIsCriticalSectionLocked();
    test_RtlInitializeCriticalSectionEx();
    test_RtlLeaveCriticalSection();
    test_LdrEnumerateLoadedModules();
    test_RtlQueryPackageIdentity();
    test_RtlMakeSelfRelativeSD();
    test_LdrRegisterDllNotification();
}
