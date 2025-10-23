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
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "in6addr.h"
#include "inaddr.h"
#include "ip2string.h"
#ifndef __REACTOS__
#include "ddk/ntifs.h"
#else
#define FASTCALL __fastcall
NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemoryUlong(
    _In_ PVOID Source,
    _In_ SIZE_T Length,
    _In_ ULONG Pattern
);
#endif
#include "wine/test.h"
#include "wine/asm.h"
#include "wine/rbtree.h"

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

static BOOL is_win64 = (sizeof(void *) > sizeof(int));

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


#ifdef __ASM_USE_FASTCALL_WRAPPER
extern ULONG WINAPI wrap_fastcall_func1( void *func, ULONG a );
__ASM_STDCALL_FUNC( wrap_fastcall_func1, 8,
                   "popl %ecx\n\t"
                   "popl %eax\n\t"
                   "xchgl (%esp),%ecx\n\t"
                   "jmp *%eax" )
#define call_fastcall_func1(func,a) wrap_fastcall_func1(func,a)
#else
#define call_fastcall_func1(func,a) func(a)
#endif


/* Function ptrs for ntdll calls */
static HMODULE hntdll = 0;
static VOID      (WINAPI  *pRtlMoveMemory)(LPVOID,LPCVOID,SIZE_T);
static VOID      (WINAPI  *pRtlFillMemory)(LPVOID,SIZE_T,BYTE);
static VOID      (WINAPI  *pRtlFillMemoryUlong)(LPVOID,SIZE_T,ULONG);
static VOID      (WINAPI  *pRtlZeroMemory)(LPVOID,SIZE_T);
static USHORT    (FASTCALL *pRtlUshortByteSwap)(USHORT source);
static ULONG     (FASTCALL *pRtlUlongByteSwap)(ULONG source);
static ULONGLONG (FASTCALL *pRtlUlonglongByteSwap)(ULONGLONG source);
static DWORD     (WINAPI *pRtlGetThreadErrorMode)(void);
static NTSTATUS  (WINAPI *pRtlSetThreadErrorMode)(DWORD, LPDWORD);
static NTSTATUS  (WINAPI *pRtlIpv4AddressToStringExA)(const IN_ADDR *, USHORT, LPSTR, PULONG);
static NTSTATUS  (WINAPI *pRtlIpv4StringToAddressExA)(PCSTR, BOOLEAN, IN_ADDR *, PUSHORT);
static NTSTATUS  (WINAPI *pRtlIpv6AddressToStringExA)(struct in6_addr *, ULONG, USHORT, PCHAR, PULONG);
static NTSTATUS  (WINAPI *pRtlIpv6StringToAddressExA)(PCSTR, struct in6_addr *, PULONG, PUSHORT);
static NTSTATUS  (WINAPI *pRtlIpv6StringToAddressExW)(PCWSTR, struct in6_addr *, PULONG, PUSHORT);
static BOOL      (WINAPI *pRtlIsCriticalSectionLocked)(CRITICAL_SECTION *);
static BOOL      (WINAPI *pRtlIsCriticalSectionLockedByThread)(CRITICAL_SECTION *);
static NTSTATUS  (WINAPI *pRtlInitializeCriticalSectionEx)(CRITICAL_SECTION *, ULONG, ULONG);
static void *    (WINAPI *pRtlFindExportedRoutineByName)(HMODULE,const char *);
static NTSTATUS  (WINAPI *pLdrEnumerateLoadedModules)(void *, void *, void *);
static NTSTATUS  (WINAPI *pLdrRegisterDllNotification)(ULONG, PLDR_DLL_NOTIFICATION_FUNCTION, void *, void **);
static NTSTATUS  (WINAPI *pLdrUnregisterDllNotification)(void *);
static VOID      (WINAPI *pRtlGetDeviceFamilyInfoEnum)(ULONGLONG *,DWORD *,DWORD *);
static void      (WINAPI *pRtlRbInsertNodeEx)(RTL_RB_TREE *, RTL_BALANCED_NODE *, BOOLEAN, RTL_BALANCED_NODE *);
static void      (WINAPI *pRtlRbRemoveNode)(RTL_RB_TREE *, RTL_BALANCED_NODE *);
static DWORD     (WINAPI *pRtlConvertDeviceFamilyInfoToString)(DWORD *, DWORD *, WCHAR *, WCHAR *);
static NTSTATUS  (WINAPI *pRtlInitializeNtUserPfn)( const UINT64 *client_procsA, ULONG procsA_size,
                                                    const UINT64 *client_procsW, ULONG procsW_size,
                                                    const void *client_workers, ULONG workers_size );
static NTSTATUS  (WINAPI *pRtlRetrieveNtUserPfn)( const UINT64 **client_procsA,
                                                  const UINT64 **client_procsW,
                                                  const UINT64 **client_workers );
static NTSTATUS  (WINAPI *pRtlResetNtUserPfn)(void);

static HMODULE hkernel32 = 0;
static BOOL      (WINAPI *pIsWow64Process)(HANDLE, PBOOL);


#define LEN 16
static const char* src_src = "This is a test!"; /* 16 bytes long, incl NUL */
static WCHAR ws2_32dllW[] = {'w','s','2','_','3','2','.','d','l','l',0};
static WCHAR nsidllW[]    = {'n','s','i','.','d','l','l',0};
static WCHAR wintrustdllW[] = {'w','i','n','t','r','u','s','t','.','d','l','l',0};
static WCHAR crypt32dllW[] = {'c','r','y','p','t','3','2','.','d','l','l',0};
static ULONG src_aligned_block[4];
static ULONG dest_aligned_block[32];
static const char *src = (const char*)src_aligned_block;
static char* dest = (char*)dest_aligned_block;
const WCHAR *expected_dll = nsidllW;

static void InitFunctionPtrs(void)
{
    hntdll = LoadLibraryA("ntdll.dll");
    ok(hntdll != 0, "LoadLibrary failed\n");
    if (hntdll) {
	pRtlMoveMemory = (void *)GetProcAddress(hntdll, "RtlMoveMemory");
	pRtlFillMemory = (void *)GetProcAddress(hntdll, "RtlFillMemory");
	pRtlFillMemoryUlong = (void *)GetProcAddress(hntdll, "RtlFillMemoryUlong");
	pRtlZeroMemory = (void *)GetProcAddress(hntdll, "RtlZeroMemory");
        pRtlUshortByteSwap = (void *)GetProcAddress(hntdll, "RtlUshortByteSwap");
        pRtlUlongByteSwap = (void *)GetProcAddress(hntdll, "RtlUlongByteSwap");
        pRtlUlonglongByteSwap = (void *)GetProcAddress(hntdll, "RtlUlonglongByteSwap");
        pRtlGetThreadErrorMode = (void *)GetProcAddress(hntdll, "RtlGetThreadErrorMode");
        pRtlSetThreadErrorMode = (void *)GetProcAddress(hntdll, "RtlSetThreadErrorMode");
        pRtlIpv4AddressToStringExA = (void *)GetProcAddress(hntdll, "RtlIpv4AddressToStringExA");
        pRtlIpv4StringToAddressExA = (void *)GetProcAddress(hntdll, "RtlIpv4StringToAddressExA");
        pRtlIpv6AddressToStringExA = (void *)GetProcAddress(hntdll, "RtlIpv6AddressToStringExA");
        pRtlIpv6StringToAddressExA = (void *)GetProcAddress(hntdll, "RtlIpv6StringToAddressExA");
        pRtlIpv6StringToAddressExW = (void *)GetProcAddress(hntdll, "RtlIpv6StringToAddressExW");
        pRtlIsCriticalSectionLocked = (void *)GetProcAddress(hntdll, "RtlIsCriticalSectionLocked");
        pRtlIsCriticalSectionLockedByThread = (void *)GetProcAddress(hntdll, "RtlIsCriticalSectionLockedByThread");
        pRtlInitializeCriticalSectionEx = (void *)GetProcAddress(hntdll, "RtlInitializeCriticalSectionEx");
        pRtlFindExportedRoutineByName = (void *)GetProcAddress(hntdll, "RtlFindExportedRoutineByName");
        pLdrEnumerateLoadedModules = (void *)GetProcAddress(hntdll, "LdrEnumerateLoadedModules");
        pLdrRegisterDllNotification = (void *)GetProcAddress(hntdll, "LdrRegisterDllNotification");
        pLdrUnregisterDllNotification = (void *)GetProcAddress(hntdll, "LdrUnregisterDllNotification");
        pRtlGetDeviceFamilyInfoEnum = (void *)GetProcAddress(hntdll, "RtlGetDeviceFamilyInfoEnum");
        pRtlRbInsertNodeEx = (void *)GetProcAddress(hntdll, "RtlRbInsertNodeEx");
        pRtlRbRemoveNode = (void *)GetProcAddress(hntdll, "RtlRbRemoveNode");
        pRtlConvertDeviceFamilyInfoToString = (void *)GetProcAddress(hntdll, "RtlConvertDeviceFamilyInfoToString");
        pRtlInitializeNtUserPfn = (void *)GetProcAddress(hntdll, "RtlInitializeNtUserPfn");
        pRtlRetrieveNtUserPfn = (void *)GetProcAddress(hntdll, "RtlRetrieveNtUserPfn");
        pRtlResetNtUserPfn = (void *)GetProcAddress(hntdll, "RtlResetNtUserPfn");
    }
    hkernel32 = LoadLibraryA("kernel32.dll");
    ok(hkernel32 != 0, "LoadLibrary failed\n");
    if (hkernel32) {
        pIsWow64Process = (void *)GetProcAddress(hkernel32, "IsWow64Process");
    }
    strcpy((char*)src_aligned_block, src_src);
    ok(strlen(src) == 15, "Source must be 16 bytes long!\n");
}

static void test_RtlQueryProcessDebugInformation(void)
{
    DEBUG_BUFFER *buffer;
    NTSTATUS status;

    /* PDI_HEAPS | PDI_HEAP_BLOCKS */
    buffer = RtlCreateQueryDebugBuffer( 0, 0 );
    ok( buffer != NULL, "RtlCreateQueryDebugBuffer returned NULL" );

    status = RtlQueryProcessDebugInformation( GetCurrentThreadId(), PDI_HEAPS | PDI_HEAP_BLOCKS, buffer );
    ok( status == STATUS_INVALID_CID, "RtlQueryProcessDebugInformation returned %lx\n", status );

    status = RtlQueryProcessDebugInformation( GetCurrentProcessId(), PDI_HEAPS | PDI_HEAP_BLOCKS, buffer );
    ok( !status, "RtlQueryProcessDebugInformation returned %lx\n", status );
    ok( buffer->InfoClassMask == (PDI_HEAPS | PDI_HEAP_BLOCKS), "unexpected InfoClassMask %ld\n", buffer->InfoClassMask);
    ok( buffer->HeapInformation != NULL, "unexpected HeapInformation %p\n", buffer->HeapInformation);

    status = RtlDestroyQueryDebugBuffer( buffer );
    ok( !status, "RtlDestroyQueryDebugBuffer returned %lx\n", status );

    /* PDI_MODULES */
    buffer = RtlCreateQueryDebugBuffer( 0, 0 );
    ok( buffer != NULL, "RtlCreateQueryDebugBuffer returned NULL" );

    status = RtlQueryProcessDebugInformation( GetCurrentProcessId(), PDI_MODULES, buffer );
    ok( !status, "RtlQueryProcessDebugInformation returned %lx\n", status );
    ok( buffer->InfoClassMask == PDI_MODULES, "unexpected InfoClassMask %ld\n", buffer->InfoClassMask);
    ok( buffer->ModuleInformation != NULL, "unexpected ModuleInformation %p\n", buffer->ModuleInformation);

    status = RtlDestroyQueryDebugBuffer( buffer );
    ok( !status, "RtlDestroyQueryDebugBuffer returned %lx\n", status );
}

#define COMP(str1,str2,cmplen,len) size = RtlCompareMemory(str1, str2, cmplen); \
  ok(size == len, "Expected %Id, got %Id\n", size, (SIZE_T)len)

static void test_RtlCompareMemory(void)
{
  SIZE_T size;

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
    result = RtlCompareMemoryUlong(a, 0, 0x0123);
    ok(result == 0, "RtlCompareMemoryUlong(%p, 0, 0x0123) returns %lu, expected 0\n", a, result);
    result = RtlCompareMemoryUlong(a, 3, 0x0123);
    ok(result == 0, "RtlCompareMemoryUlong(%p, 3, 0x0123) returns %lu, expected 0\n", a, result);
    result = RtlCompareMemoryUlong(a, 4, 0x0123);
    ok(result == 4, "RtlCompareMemoryUlong(%p, 4, 0x0123) returns %lu, expected 4\n", a, result);
    result = RtlCompareMemoryUlong(a, 5, 0x0123);
    ok(result == 4 || !result /* arm64 */, "RtlCompareMemoryUlong(%p, 5, 0x0123) returns %lu, expected 4\n", a, result);
    result = RtlCompareMemoryUlong(a, 7, 0x0123);
    ok(result == 4 || !result /* arm64 */, "RtlCompareMemoryUlong(%p, 7, 0x0123) returns %lu, expected 4\n", a, result);
    result = RtlCompareMemoryUlong(a, 8, 0x0123);
    ok(result == 4, "RtlCompareMemoryUlong(%p, 8, 0x0123) returns %lu, expected 4\n", a, result);
    result = RtlCompareMemoryUlong(a, 9, 0x0123);
    ok(result == 4 || !result /* arm64 */, "RtlCompareMemoryUlong(%p, 9, 0x0123) returns %lu, expected 4\n", a, result);
    result = RtlCompareMemoryUlong(a, 4, 0x0127);
    ok(result == 0, "RtlCompareMemoryUlong(%p, 4, 0x0127) returns %lu, expected 0\n", a, result);
    result = RtlCompareMemoryUlong(a, 4, 0x7123);
    ok(result == 0 || result == 1 /* arm64 */, "RtlCompareMemoryUlong(%p, 4, 0x7123) returns %lu, expected 0\n", a, result);
    result = RtlCompareMemoryUlong(a, 16, 0x4567);
    ok(result == 0, "RtlCompareMemoryUlong(%p, 16, 0x4567) returns %lu, expected 0\n", a, result);

    a[1]= 0x0123;
    result = RtlCompareMemoryUlong(a, 3, 0x0123);
    ok(result == 0, "RtlCompareMemoryUlong(%p, 3, 0x0123) returns %lu, expected 0\n", a, result);
    result = RtlCompareMemoryUlong(a, 4, 0x0123);
    ok(result == 4, "RtlCompareMemoryUlong(%p, 4, 0x0123) returns %lu, expected 4\n", a, result);
    result = RtlCompareMemoryUlong(a, 5, 0x0123);
    ok(result == 4 || !result /* arm64 */, "RtlCompareMemoryUlong(%p, 5, 0x0123) returns %lu, expected 4\n", a, result);
    result = RtlCompareMemoryUlong(a, 7, 0x0123);
    ok(result == 4 || !result /* arm64 */, "RtlCompareMemoryUlong(%p, 7, 0x0123) returns %lu, expected 4\n", a, result);
    result = RtlCompareMemoryUlong(a, 8, 0x0123);
    ok(result == 8, "RtlCompareMemoryUlong(%p, 8, 0x0123) returns %lu, expected 8\n", a, result);
    result = RtlCompareMemoryUlong(a, 9, 0x0123);
    ok(result == 8 || !result /* arm64 */, "RtlCompareMemoryUlong(%p, 9, 0x0123) returns %lu, expected 8\n", a, result);
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

static void test_RtlByteSwap(void)
{
    ULONGLONG llresult;
    ULONG     lresult;
    USHORT    sresult;

#ifdef _WIN64
    /* the Rtl*ByteSwap() are always inlined and not exported from ntdll on 64bit */
    sresult = RtlUshortByteSwap( 0x1234 );
    ok( 0x3412 == sresult,
        "inlined RtlUshortByteSwap() returns 0x%x\n", sresult );
    lresult = RtlUlongByteSwap( 0x87654321 );
    ok( 0x21436587 == lresult,
        "inlined RtlUlongByteSwap() returns 0x%lx\n", lresult );
    llresult = RtlUlonglongByteSwap( 0x7654321087654321ull );
    ok( 0x2143658710325476 == llresult,
        "inlined RtlUlonglongByteSwap() returns %#I64x\n", llresult );
#else
    ok( pRtlUshortByteSwap != NULL, "RtlUshortByteSwap is not available\n" );
    if ( pRtlUshortByteSwap )
    {
        sresult = call_fastcall_func1( pRtlUshortByteSwap, 0x1234u );
        ok( 0x3412u == sresult,
            "ntdll.RtlUshortByteSwap() returns %#x\n", sresult );
    }

    ok( pRtlUlongByteSwap != NULL, "RtlUlongByteSwap is not available\n" );
    if ( pRtlUlongByteSwap )
    {
        lresult = call_fastcall_func1( pRtlUlongByteSwap, 0x87654321ul );
        ok( 0x21436587ul == lresult,
            "ntdll.RtlUlongByteSwap() returns %#lx\n", lresult );
    }

    ok( pRtlUlonglongByteSwap != NULL, "RtlUlonglongByteSwap is not available\n");
    if ( pRtlUlonglongByteSwap )
    {
        llresult = pRtlUlonglongByteSwap( 0x7654321087654321ull );
        ok( 0x2143658710325476ull == llresult,
            "ntdll.RtlUlonglongByteSwap() returns %#I64x\n", llresult );
    }
#endif
}


static void test_RtlUniform(void)
{
    const ULONG step = 0x7fff;
    ULONG num;
    ULONG seed;
    ULONG seed_bak;
    ULONG expected;
    ULONG result;

#ifdef __REACTOS__
    if (!is_reactos() && (_winver < _WIN32_WINNT_VISTA))
    {
        skip("Skipping tests for RtlUniform, because it's broken on Windows 2003\n");
        return;
    }
#endif // __REACTOS__

    /*
     * According to the documentation RtlUniform is using D.H. Lehmer's 1948
     * algorithm.  We assume a more generic version of this algorithm,
     * which is the linear congruential generator (LCG).  Its formula is:
     *
     *   X_(n+1) = (a * X_n + c) % m
     *
     * where a is the multiplier, c is the increment, and m is the modulus.
     *
     * According to the documentation, the random numbers are distributed over
     * [0..MAXLONG].  Therefore, the modulus is MAXLONG + 1:
     *
     *   X_(n+1) = (a * X_n + c) % (MAXLONG + 1)
     *
     * To find out the increment, we just call RtlUniform with seed set to 0.
     * This reveals c = 0x7fffffc3.
     */
    seed = 0;
    expected = 0x7fffffc3;
    result = RtlUniform(&seed);
    ok(result == expected,
        "RtlUniform(&seed (seed == 0)) returns %lx, expected %lx\n",
        result, expected);

    /*
     * The formula is now:
     *
     *   X_(n+1) = (a * X_n + 0x7fffffc3) % (MAXLONG + 1)
     *
     * If the modulus is correct, RtlUniform(0) shall equal RtlUniform(MAXLONG + 1).
     * However, testing reveals that this is not the case.
     * That is, the modulus in the documentation is incorrect.
     */
    seed = 0x80000000U;
    expected = 0x7fffffb1;
    result = RtlUniform(&seed);

    ok(result == expected,
        "RtlUniform(&seed (seed == 0x80000000)) returns %lx, expected %lx\n",
        result, expected);

    /*
     * We try another value for modulus, say MAXLONG.
     * We discover that RtlUniform(0) equals RtlUniform(MAXLONG), which means
     * the correct value for the modulus is actually MAXLONG.
     */
    seed = 0x7fffffff;
    expected = 0x7fffffc3;
    result = RtlUniform(&seed);
    ok(result == expected,
        "RtlUniform(&seed (seed == 0x7fffffff)) returns %lx, expected %lx\n",
        result, expected);

    /*
     * The formula is now:
     *
     *   X_(n+1) = (a * X_n + 0x7fffffc3) % MAXLONG
     *
     * To find out the multiplier we can use:
     *
     *   a = RtlUniform(1) - 0x7fffffc3 (mod MAXLONG)
     *
     * This way, we find out that a = -18 (mod MAXLONG),
     * which is congruent to 0x7fffffed (MAXLONG - 18).
     */
    seed = 1;
    expected = ((ULONGLONG)seed * 0x7fffffed + 0x7fffffc3) % MAXLONG;
    result = RtlUniform(&seed);
    ok(result == expected,
        "RtlUniform(&seed (seed == 1)) returns %lx, expected %lx\n",
        result, expected);

    num = 2;
    do
    {
        seed = num;
        expected = ((ULONGLONG)seed * 0x7fffffed + 0x7fffffc3) % 0x7fffffff;
        result = RtlUniform(&seed);
        ok(result == expected,
                "test: RtlUniform(&seed (seed == %lx)) returns %lx, expected %lx\n",
                num, result, expected);
        ok(seed == expected,
                "test: RtlUniform(&seed (seed == %lx)) sets seed to %lx, expected %lx\n",
                num, result, expected);

        num += step;
    } while (num >= 2 + step);

    seed = 0;
    for (num = 0; num <= 100000; num++) {
        expected = ((ULONGLONG)seed * 0x7fffffed + 0x7fffffc3) % 0x7fffffff;
        seed_bak = seed;
        result = RtlUniform(&seed);
        ok(result == expected,
                "test: %ld RtlUniform(&seed (seed == %lx)) returns %lx, expected %lx\n",
                num, seed_bak, result, expected);
        ok(seed == expected,
                "test: %ld RtlUniform(&seed (seed == %lx)) sets seed to %lx, expected %lx\n",
                num, seed_bak, result, expected);
    } /* for */
}


static void test_RtlRandom(void)
{
    int i, j;
    ULONG seed;
    ULONG res[512];

    seed = 0;
    for (i = 0; i < ARRAY_SIZE(res); i++)
    {
        res[i] = RtlRandom(&seed);
        ok(seed != res[i], "%i: seed is same as res %lx\n", i, seed);
        for (j = 0; j < i; j++)
            ok(res[i] != res[j], "res[%i] (%lx) is same as res[%i] (%lx)\n", j, res[j], i, res[i]);
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


static void test_RtlAreAllAccessesGranted(void)
{
    unsigned int test_num;
    BOOLEAN result;

    for (test_num = 0; test_num < ARRAY_SIZE(all_accesses); test_num++) {
	result = RtlAreAllAccessesGranted(all_accesses[test_num].GrantedAccess,
					  all_accesses[test_num].DesiredAccess);
	ok(all_accesses[test_num].result == result,
           "(test %d): RtlAreAllAccessesGranted(%08lx, %08lx) returns %d, expected %d\n",
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


static void test_RtlAreAnyAccessesGranted(void)
{
    unsigned int test_num;
    BOOLEAN result;

    for (test_num = 0; test_num < ARRAY_SIZE(any_accesses); test_num++) {
	result = RtlAreAnyAccessesGranted(any_accesses[test_num].GrantedAccess,
					  any_accesses[test_num].DesiredAccess);
	ok(any_accesses[test_num].result == result,
           "(test %d): RtlAreAnyAccessesGranted(%08lx, %08lx) returns %d, expected %d\n",
	   test_num, any_accesses[test_num].GrantedAccess,
	   any_accesses[test_num].DesiredAccess,
	   result, any_accesses[test_num].result);
    } /* for */
}

static void test_RtlComputeCrc32(void)
{
  DWORD crc = 0;

  crc = RtlComputeCrc32(crc, (const BYTE *)src, LEN);
  ok(crc == 0x40861dc2,"Expected 0x40861dc2, got %8lx\n", crc);
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

    RtlInitializeHandleTable(0x3FFF, sizeof(MY_HANDLE), &HandleTable);
    MyHandle = (MY_HANDLE *)RtlAllocateHandle(&HandleTable, &Index);
    ok(MyHandle != NULL, "RtlAllocateHandle failed\n");
    RtlpMakeHandleAllocated(&MyHandle->RtlHandle);
    MyHandle = NULL;
    result = RtlIsValidIndexHandle(&HandleTable, Index, (RTL_HANDLE **)&MyHandle);
    ok(result, "Handle %p wasn't valid\n", MyHandle);
    result = RtlFreeHandle(&HandleTable, &MyHandle->RtlHandle);
    ok(result, "Couldn't free handle %p\n", MyHandle);
    status = RtlDestroyHandleTable(&HandleTable);
    ok(status == STATUS_SUCCESS, "RtlDestroyHandleTable failed with error 0x%08lx\n", status);
}

static void test_RtlAllocateAndInitializeSid(void)
{
    NTSTATUS ret;
    SID_IDENTIFIER_AUTHORITY sia = {{ 1, 2, 3, 4, 5, 6 }};
    PSID psid;

    ret = RtlAllocateAndInitializeSid(&sia, 0, 1, 2, 3, 4, 5, 6, 7, 8, &psid);
    ok(!ret, "RtlAllocateAndInitializeSid error %08lx\n", ret);
    ret = RtlFreeSid(psid);
    ok(!ret, "RtlFreeSid error %08lx\n", ret);

    /* these tests crash on XP */
    if (0)
    {
        RtlAllocateAndInitializeSid(NULL, 0, 1, 2, 3, 4, 5, 6, 7, 8, &psid);
        RtlAllocateAndInitializeSid(&sia, 0, 1, 2, 3, 4, 5, 6, 7, 8, NULL);
    }

    ret = RtlAllocateAndInitializeSid(&sia, 9, 1, 2, 3, 4, 5, 6, 7, 8, &psid);
    ok(ret == STATUS_INVALID_SID, "wrong error %08lx\n", ret);
}

static void test_RtlDeleteTimer(void)
{
    NTSTATUS ret;

    ret = RtlDeleteTimer(NULL, NULL, NULL);
    ok(ret == STATUS_INVALID_PARAMETER_1 ||
       ret == STATUS_INVALID_PARAMETER, /* W2K */
       "expected STATUS_INVALID_PARAMETER_1 or STATUS_INVALID_PARAMETER, got %lx\n", ret);
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
       "RtlSetThreadErrorMode failed with error 0x%08lx\n", status);
    ok(mode == oldmode,
       "RtlSetThreadErrorMode returned mode 0x%lx, expected 0x%lx\n",
       mode, oldmode);
    ok(pRtlGetThreadErrorMode() == 0x70,
       "RtlGetThreadErrorMode returned 0x%lx, expected 0x%x\n", mode, 0x70);
    if (!is_wow64)
    {
        ok(NtCurrentTeb()->HardErrorMode == 0x70,
           "The TEB contains 0x%lx, expected 0x%x\n",
           NtCurrentTeb()->HardErrorMode, 0x70);
    }

    status = pRtlSetThreadErrorMode(0, &mode);
    ok(status == STATUS_SUCCESS ||
       status == STATUS_WAIT_1, /* Vista */
       "RtlSetThreadErrorMode failed with error 0x%08lx\n", status);
    ok(mode == 0x70,
       "RtlSetThreadErrorMode returned mode 0x%lx, expected 0x%x\n",
       mode, 0x70);
    ok(pRtlGetThreadErrorMode() == 0,
       "RtlGetThreadErrorMode returned 0x%lx, expected 0x%x\n", mode, 0);
    if (!is_wow64)
    {
        ok(NtCurrentTeb()->HardErrorMode == 0,
           "The TEB contains 0x%lx, expected 0x%x\n",
           NtCurrentTeb()->HardErrorMode, 0);
    }

    for (mode = 1; mode; mode <<= 1)
    {
        status = pRtlSetThreadErrorMode(mode, NULL);
        if (mode & 0x70)
            ok(status == STATUS_SUCCESS ||
               status == STATUS_WAIT_1, /* Vista */
               "RtlSetThreadErrorMode(%lx,NULL) failed with error 0x%08lx\n",
               mode, status);
        else
            ok(status == STATUS_INVALID_PARAMETER_1,
               "RtlSetThreadErrorMode(%lx,NULL) returns 0x%08lx, "
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

    addr32 = 0x50005;
    reloc = IMAGE_REL_BASED_HIGHLOW<<12;
    ret = LdrProcessRelocationBlock(&addr32, 1, &reloc, 0x500050);
    ok((USHORT*)ret == &reloc+1, "ret = %p, expected %p\n", ret, &reloc+1);
    ok(addr32 == 0x550055, "addr32 = %lx, expected 0x550055\n", addr32);

    addr16 = 0x505;
    reloc = IMAGE_REL_BASED_HIGH<<12;
    ret = LdrProcessRelocationBlock(&addr16, 1, &reloc, 0x500060);
    ok((USHORT*)ret == &reloc+1, "ret = %p, expected %p\n", ret, &reloc+1);
    ok(addr16 == 0x555, "addr16 = %x, expected 0x555\n", addr16);

    addr16 = 0x505;
    reloc = IMAGE_REL_BASED_LOW<<12;
    ret = LdrProcessRelocationBlock(&addr16, 1, &reloc, 0x500060);
    ok((USHORT*)ret == &reloc+1, "ret = %p, expected %p\n", ret, &reloc+1);
    ok(addr16 == 0x565, "addr16 = %x, expected 0x565\n", addr16);
}

static void test_RtlIpv4AddressToString(void)
{
    CHAR buffer[20];
    CHAR *res;
    IN_ADDR ip;
    DWORD_PTR len;

    ip.S_un.S_un_b.s_b1 = 1;
    ip.S_un.S_un_b.s_b2 = 2;
    ip.S_un.S_un_b.s_b3 = 3;
    ip.S_un.S_un_b.s_b4 = 4;

    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = RtlIpv4AddressToStringA(&ip, buffer);
    len = strlen(buffer);
    ok(res == (buffer + len), "got %p with '%s' (expected %p)\n", res, buffer, buffer + len);

    res = RtlIpv4AddressToStringA(&ip, NULL);
    ok( (res == (char *)~0) ||
        broken(res == (char *)len),        /* XP and w2003 */
        "got %p (expected ~0)\n", res);

    if (0) {
        /* this crashes in windows */
        memset(buffer, '#', sizeof(buffer) - 1);
        buffer[sizeof(buffer) -1] = 0;
        res = RtlIpv4AddressToStringA(NULL, buffer);
        trace("got %p with '%s'\n", res, buffer);
    }

    if (0) {
        /* this crashes in windows */
        res = RtlIpv4AddressToStringA(NULL, NULL);
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
        "got 0x%lx and size %ld with '%s'\n", res, size, buffer);

    size = used + 1;
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    ok( (res == STATUS_SUCCESS) &&
        (size == strlen(expect) + 1) && !strcmp(buffer, expect),
        "got 0x%lx and size %ld with '%s'\n", res, size, buffer);

    size = used;
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    ok( (res == STATUS_INVALID_PARAMETER) && (size == used + 1),
        "got 0x%lx and %ld with '%s' (expected STATUS_INVALID_PARAMETER and %ld)\n",
        res, size, buffer, used + 1);

    size = used - 1;
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    ok( (res == STATUS_INVALID_PARAMETER) && (size == used + 1),
        "got 0x%lx and %ld with '%s' (expected STATUS_INVALID_PARAMETER and %ld)\n",
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
        "got 0x%lx and size %ld with '%s'\n", res, size, buffer);

    size = used + 1;
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    ok( (res == STATUS_SUCCESS) &&
        (size == strlen(expect) + 1) && !strcmp(buffer, expect),
        "got 0x%lx and size %ld with '%s'\n", res, size, buffer);

    size = used;
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    ok( (res == STATUS_INVALID_PARAMETER) && (size == used + 1),
        "got 0x%lx and %ld with '%s' (expected STATUS_INVALID_PARAMETER and %ld)\n",
        res, size, buffer, used + 1);

    size = used - 1;
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, port, buffer, &size);
    ok( (res == STATUS_INVALID_PARAMETER) && (size == used + 1),
        "got 0x%lx and %ld with '%s' (expected STATUS_INVALID_PARAMETER and %ld)\n",
        res, size, buffer, used + 1);


    /* parameters are checked */
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(&ip, 0, buffer, NULL);
    ok(res == STATUS_INVALID_PARAMETER,
        "got 0x%lx with '%s' (expected STATUS_INVALID_PARAMETER)\n", res, buffer);

    size = sizeof(buffer);
    res = pRtlIpv4AddressToStringExA(&ip, 0, NULL, &size);
    ok( res == STATUS_INVALID_PARAMETER,
        "got 0x%lx and size %ld (expected STATUS_INVALID_PARAMETER)\n", res, size);

    size = sizeof(buffer);
    memset(buffer, '#', sizeof(buffer) - 1);
    buffer[sizeof(buffer) -1] = 0;
    res = pRtlIpv4AddressToStringExA(NULL, 0, buffer, &size);
    ok( res == STATUS_INVALID_PARAMETER,
        "got 0x%lx and size %ld with '%s' (expected STATUS_INVALID_PARAMETER)\n",
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
    { "1.2",                    STATUS_SUCCESS,            3, {   1,   0,   0,   2 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  3, { -1 } },
    { "1000.2000",              STATUS_INVALID_PARAMETER,  9, { -1 } },
    { "1.2.",                   STATUS_INVALID_PARAMETER,  4, { -1 } },
    { "1..2",                   STATUS_INVALID_PARAMETER,  3, { -1 } },
    { "1...2",                  STATUS_INVALID_PARAMETER,  3, { -1 } },
    { "1.2.3",                  STATUS_SUCCESS,            5, {   1,   2,   0,   3 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  5, { -1 } },
    { "1.2.3.",                 STATUS_INVALID_PARAMETER,  6, { -1 } },
    { "203569230",              STATUS_SUCCESS,            9, {  12,  34,  56,  78 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  9, { -1 } },
    { "1.223756",               STATUS_SUCCESS,            8, {   1,   3, 106,  12 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  8, { -1 } },
    { "3.4.756",                STATUS_SUCCESS,            7, {   3,   4,   2, 244 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  7, { -1 } },
    { "756.3.4",                STATUS_INVALID_PARAMETER,  7, { -1 } },
    { "3.756.4",                STATUS_INVALID_PARAMETER,  7, { -1 } },
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
    { ".1.2.3.4",               STATUS_INVALID_PARAMETER,  1, { -1 } },
    { "0.1.2.3",                STATUS_SUCCESS,            7, {   0,   1,   2,   3 } },
    { "0.1.2.3.",               STATUS_INVALID_PARAMETER,  7, { -1 } },
    { "[0.1.2.3]",              STATUS_INVALID_PARAMETER,  0, { -1 } },
    { "0x00010203",             STATUS_SUCCESS,           10, {   0,   1,   2,   3 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  2, { -1 } },
    { "0X00010203",             STATUS_SUCCESS,           10, {   0,   1,   2,   3 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  2, { -1 } },
    { "0x1234",                 STATUS_SUCCESS,            6, {   0,   0,  18,  52 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  2, { -1 } },
    { "0x123456789",            STATUS_SUCCESS,           11, {  35,  69, 103, 137 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  2, { -1 } },
    { "0x00010Q03",             STATUS_SUCCESS,            7, {   0,   0,   0,  16 }, strict_diff_4 | ex_fail_4,
                                STATUS_INVALID_PARAMETER,  2, { -1 } },
    { "x00010203",              STATUS_INVALID_PARAMETER,  0, { -1 } },
    { "1234BEEF",               STATUS_SUCCESS,            4, {   0,   0,   4, 210 }, strict_diff_4 | ex_fail_4,
                                STATUS_INVALID_PARAMETER,  4, { -1 } },
    { "017700000001",           STATUS_SUCCESS,           12, { 127,   0,   0,   1 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  1, { -1 } },
    { "0777",                   STATUS_SUCCESS,            4, {   0,   0,   1, 255 }, strict_diff_4,
                                STATUS_INVALID_PARAMETER,  1, { -1 } },
    { "::1",                    STATUS_INVALID_PARAMETER,  0, { -1 } },
    { ":1",                     STATUS_INVALID_PARAMETER,  0, { -1 } },
};

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
    int i;

    if (0)
    {
        /* leaving either parameter NULL crashes on Windows */
        res = RtlIpv4StringToAddressA(NULL, FALSE, &terminator, &ip);
        res = RtlIpv4StringToAddressA("1.1.1.1", FALSE, NULL, &ip);
        res = RtlIpv4StringToAddressA("1.1.1.1", FALSE, &terminator, NULL);
        /* same for the wide char version */
        /*
        res = RtlIpv4StringToAddressW(NULL, FALSE, &terminatorW, &ip);
        res = RtlIpv4StringToAddressW(L"1.1.1.1", FALSE, NULL, &ip);
        res = RtlIpv4StringToAddressW(L"1.1.1.1", FALSE, &terminatorW, NULL);
        */
    }

    for (i = 0; i < ARRAY_SIZE(ipv4_tests); i++)
    {
        /* non-strict */
        terminator = &dummy;
        ip.S_un.S_addr = 0xabababab;
        res = RtlIpv4StringToAddressA(ipv4_tests[i].address, FALSE, &terminator, &ip);
        ok(res == ipv4_tests[i].res,
           "[%s] res = 0x%08lx, expected 0x%08lx\n",
           ipv4_tests[i].address, res, ipv4_tests[i].res);
        ok(terminator == ipv4_tests[i].address + ipv4_tests[i].terminator_offset,
           "[%s] terminator = %p, expected %p\n",
           ipv4_tests[i].address, terminator, ipv4_tests[i].address + ipv4_tests[i].terminator_offset);

        init_ip4(&expected_ip, ipv4_tests[i].ip);
        ok(ip.S_un.S_addr == expected_ip.S_un.S_addr,
           "[%s] ip = %08lx, expected %08lx\n",
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
        res = RtlIpv4StringToAddressA(ipv4_tests[i].address, TRUE, &terminator, &ip);
        ok(res == ipv4_tests[i].res_strict,
           "[%s] res = 0x%08lx, expected 0x%08lx\n",
           ipv4_tests[i].address, res, ipv4_tests[i].res_strict);
        ok(terminator == ipv4_tests[i].address + ipv4_tests[i].terminator_offset_strict,
           "[%s] terminator = %p, expected %p\n",
           ipv4_tests[i].address, terminator, ipv4_tests[i].address + ipv4_tests[i].terminator_offset_strict);

        init_ip4(&expected_ip, ipv4_tests[i].ip_strict);
        ok(ip.S_un.S_addr == expected_ip.S_un.S_addr,
           "[%s] ip = %08lx, expected %08lx\n",
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
    unsigned int i;
    BOOLEAN strict;

    if (!pRtlIpv4StringToAddressExA)
    {
        win_skip("RtlIpv4StringToAddressEx not available\n");
        return;
    }

    /* do not crash, and do not touch the ip / port. */
    ip.S_un.S_addr = 0xabababab;
    port = 0xdead;
    res = pRtlIpv4StringToAddressExA(NULL, FALSE, &ip, &port);
    ok(res == STATUS_INVALID_PARAMETER, "[null address] res = 0x%08lx, expected 0x%08lx\n",
       res, STATUS_INVALID_PARAMETER);
    ok(ip.S_un.S_addr == 0xabababab, "RtlIpv4StringToAddressExA should not touch the ip!, ip == %lx\n", ip.S_un.S_addr);
    ok(port == 0xdead, "RtlIpv4StringToAddressExA should not touch the port!, port == %x\n", port);

    port = 0xdead;
    res = pRtlIpv4StringToAddressExA("1.1.1.1", FALSE, NULL, &port);
    ok(res == STATUS_INVALID_PARAMETER, "[null ip] res = 0x%08lx, expected 0x%08lx\n",
       res, STATUS_INVALID_PARAMETER);
    ok(port == 0xdead, "RtlIpv4StringToAddressExA should not touch the port!, port == %x\n", port);

    ip.S_un.S_addr = 0xabababab;
    port = 0xdead;
    res = pRtlIpv4StringToAddressExA("1.1.1.1", FALSE, &ip, NULL);
    ok(res == STATUS_INVALID_PARAMETER, "[null port] res = 0x%08lx, expected 0x%08lx\n",
       res, STATUS_INVALID_PARAMETER);
    ok(ip.S_un.S_addr == 0xabababab, "RtlIpv4StringToAddressExA should not touch the ip!, ip == %lx\n", ip.S_un.S_addr);
    ok(port == 0xdead, "RtlIpv4StringToAddressExA should not touch the port!, port == %x\n", port);

    /* first we run the non-ex testcases on the ex function */
    for (i = 0; i < ARRAY_SIZE(ipv4_tests); i++)
    {
        NTSTATUS expect_res = (ipv4_tests[i].flags & ex_fail_4) ? STATUS_INVALID_PARAMETER : ipv4_tests[i].res;

        /* non-strict */
        port = 0xdead;
        ip.S_un.S_addr = 0xabababab;
        res = pRtlIpv4StringToAddressExA(ipv4_tests[i].address, FALSE, &ip, &port);
        ok(res == expect_res, "[%s] res = 0x%08lx, expected 0x%08lx\n",
           ipv4_tests[i].address, res, expect_res);

        init_ip4(&expected_ip, ipv4_tests[i].ip);
        ok(ip.S_un.S_addr == expected_ip.S_un.S_addr, "[%s] ip = %08lx, expected %08lx\n",
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
        ok(res == expect_res, "[%s] res = 0x%08lx, expected 0x%08lx\n",
           ipv4_tests[i].address, res, expect_res);

        init_ip4(&expected_ip, ipv4_tests[i].ip_strict);
        ok(ip.S_un.S_addr == expected_ip.S_un.S_addr, "[%s] ip = %08lx, expected %08lx\n",
           ipv4_tests[i].address, ip.S_un.S_addr, expected_ip.S_un.S_addr);
    }


    for (i = 0; i < ARRAY_SIZE(ipv4_ex_tests); i++)
    {
        /* Strict is only relevant for the ip address, so make sure that it does not influence the port */
        for (strict = 0; strict < 2; strict++)
        {
            ip.S_un.S_addr = 0xabababab;
            port = 0xdead;
            res = pRtlIpv4StringToAddressExA(ipv4_ex_tests[i].address, strict, &ip, &port);
            ok(res == ipv4_ex_tests[i].res, "[%s] res = 0x%08lx, expected 0x%08lx\n",
               ipv4_ex_tests[i].address, res, ipv4_ex_tests[i].res);

            init_ip4(&expected_ip, ipv4_ex_tests[i].ip);
            ok(ip.S_un.S_addr == expected_ip.S_un.S_addr, "[%s] ip = %08lx, expected %08lx\n",
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
    /* win_broken: XP and Vista do not handle this correctly
        ex_fail: Ex function does need the string to be terminated, non-Ex does not.
        ex_skip: test doesn't make sense for Ex (f.e. it's invalid for non-Ex but valid for Ex) */
    enum { normal_6, win_broken_6 = 1, ex_fail_6 = 2, ex_skip_6 = 4, win_extra_zero = 8 } flags;
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
    { "0:0:0:0:0:0:0::",                                STATUS_SUCCESS,             15,
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
    { "0:a:b:c:d:e:f::",                                STATUS_SUCCESS,             15,
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
    { "1111:2222:3333:4444:5555:6666:7777::",           STATUS_SUCCESS,             36,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0 }, win_broken_6 },
    { "1111:2222:3333:4444:5555:6666::",                STATUS_SUCCESS,             31,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0 } },
    { "1111:2222:3333:4444:5555:6666::8888",            STATUS_SUCCESS,             35,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0x8888 } },
    { "1111:2222:3333:4444:5555:6666::7777:8888",       STATUS_SUCCESS,             35,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0x7777 }, ex_fail_6 },
    { "1111:2222:3333:4444:5555:6666:7777::8888",       STATUS_SUCCESS,             36,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0 }, ex_fail_6|win_broken_6 },
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
    { "1111:2222:3333:4444:5555::0x80000000",           STATUS_SUCCESS,             27,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0xffff }, ex_fail_6 },
    { "1111:2222:3333:4444::5555:0x012345678",          STATUS_SUCCESS,             27,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0, 0, 0x5555, 0x7856 }, ex_fail_6 },
    { "1111:2222:3333:4444::5555:0x123456789",          STATUS_SUCCESS,             27,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0, 0, 0x5555, 0xffff }, ex_fail_6 },
    { "1111:2222:3333:4444:5555:6666:0x12345678",       STATUS_INVALID_PARAMETER,   31,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0xabab, 0xabab }, ex_fail_6 },
    { "1111:2222:3333:4444:5555:6666:7777:0x80000000", STATUS_SUCCESS,             36,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0xffff }, ex_fail_6 },
    { "1111:2222:3333:4444:5555:6666:7777:0x012345678", STATUS_SUCCESS,             36,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x7856 }, ex_fail_6 },
    { "1111:2222:3333:4444:5555:6666:7777:0x123456789", STATUS_SUCCESS,             36,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0xffff }, ex_fail_6 },
    { "111:222:333:444:555:666:777:0x123456789abcdef0", STATUS_SUCCESS,             29,
            { 0x1101, 0x2202, 0x3303, 0x4404, 0x5505, 0x6606, 0x7707, 0xffff }, ex_fail_6 },
    { "1111:2222:3333:4444:5555::08888",                STATUS_INVALID_PARAMETER,   31,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333:4444:5555::08888::",              STATUS_INVALID_PARAMETER,   31,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333:4444:5555:6666:7777:fffff:",      STATUS_INVALID_PARAMETER,   40,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0xabab } },
    { "1111:2222:3333:4444:5555:6666::fffff:",          STATUS_INVALID_PARAMETER,   36,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0xabab, 0xabab } },
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
    { "::01234",                                        STATUS_INVALID_PARAMETER,   7,
            { 0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
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
    /* this one and the next one are incorrectly parsed before Windows 11,
        it adds one zero too many in front, cutting off the last digit. */
    { "::0:0:0:0:0:0:0",                                STATUS_SUCCESS,             15,
            { 0, 0, 0, 0, 0, 0, 0, 0 }, win_broken_6|win_extra_zero },
    { "::0:a:b:c:d:e:f",                                STATUS_SUCCESS,             15,
            { 0, 0, 0xa00, 0xb00, 0xc00, 0xd00, 0xe00, 0xf00 }, win_broken_6|win_extra_zero },
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
    { "2001:0000::01234.0",                             STATUS_INVALID_PARAMETER,   -1,
            { 0x120, 0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "2001:0::b.0",                                    STATUS_SUCCESS,             9,
            { 0x120, 0, 0, 0, 0, 0, 0, 0xb00 }, ex_fail_6 },
    { "2001::0:b.0",                                    STATUS_SUCCESS,             9,
            { 0x120, 0, 0, 0, 0, 0, 0, 0xb00 }, ex_fail_6 },
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
    { "0x1111",                                         STATUS_INVALID_PARAMETER,   1,
            { -1 } },
    { "1111:22223333:4444:5555:6666:1.2.3.4",           STATUS_INVALID_PARAMETER,   -1,
            { 0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:22223333:4444:5555:6666:7777:8888",         STATUS_INVALID_PARAMETER,   -1,
            { 0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:123456789:4444:5555:6666:7777:8888",        STATUS_INVALID_PARAMETER,   -1,
            { 0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:1234567890abcdef0:4444:5555:6666:7777:888", STATUS_INVALID_PARAMETER,   -1,
            { 0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:",                                     STATUS_INVALID_PARAMETER,   10,
            { 0x1111, 0x2222, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:1.2.3.4",                              STATUS_INVALID_PARAMETER,   17,
            { 0x1111, 0x2222, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333",                                 STATUS_INVALID_PARAMETER,   14,
            { 0x1111, 0x2222, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1111:2222:3333:4444:5555:6666::1.2.3.4",         STATUS_SUCCESS,             32,
            { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0x100 }, ex_fail_6 },
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
    { "1::001.2.3.4",                                   STATUS_SUCCESS,             12,
            { 0x100, 0, 0, 0, 0, 0, 0x201, 0x403 } },
    { "1::1.002.3.4",                                   STATUS_SUCCESS,             12,
            { 0x100, 0, 0, 0, 0, 0, 0x201, 0x403 } },
    { "1::0001.2.3.4",                                  STATUS_INVALID_PARAMETER,   -1,
            { 0x100, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.0002.3.4",                                  STATUS_INVALID_PARAMETER,   -1,
            { 0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.256.4",                                   STATUS_INVALID_PARAMETER,   -1,
            { 0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.4294967296.4",                            STATUS_INVALID_PARAMETER,   -1,
            { 0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.18446744073709551616.4",                  STATUS_INVALID_PARAMETER,   -1,
            { 0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.3.256",                                   STATUS_INVALID_PARAMETER,   12,
            { 0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.3.4294967296",                            STATUS_INVALID_PARAMETER,   19,
            { 0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.3.18446744073709551616",                  STATUS_INVALID_PARAMETER,   29,
            { 0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.3.300",                                   STATUS_INVALID_PARAMETER,   12,
            { 0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2.3.300.",                                  STATUS_INVALID_PARAMETER,   12,
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
    { "1::1.256:3.4",                                   STATUS_INVALID_PARAMETER,   8,
            { 0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1.2a.3.4",                                    STATUS_INVALID_PARAMETER,   6,
            { 0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::256.2.3.4",                                   STATUS_INVALID_PARAMETER,   -1,
            { 0x100, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "1::1a.2.3.4",                                    STATUS_SUCCESS,             5,
            { 0x100, 0, 0, 0, 0, 0, 0, 0x1a00 }, ex_fail_6 },
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
    { "::12345678",                                     STATUS_INVALID_PARAMETER,   10,
            { 0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "::123456789",                                    STATUS_INVALID_PARAMETER,   11,
            { 0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "::1234567890abcdef0",                            STATUS_INVALID_PARAMETER,   19,
            { 0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab } },
    { "::0x80000000",                                   STATUS_SUCCESS,             3,
            { 0, 0, 0, 0, 0, 0, 0, 0xffff }, ex_fail_6 },
    { "::0x012345678",                                  STATUS_SUCCESS,             3,
            { 0, 0, 0, 0, 0, 0, 0, 0x7856 }, ex_fail_6 },
    { "::0x123456789",                                  STATUS_SUCCESS,             3,
            { 0, 0, 0, 0, 0, 0, 0, 0xffff }, ex_fail_6 },
    { "::0x1234567890abcdef0",                          STATUS_SUCCESS,             3,
            { 0, 0, 0, 0, 0, 0, 0, 0xffff }, ex_fail_6 },
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
    static const struct
    {
        PCSTR address;
        int ip[8];
    } tests[] =
    {
        /* ipv4 addresses & ISATAP addresses */
        { "::13.1.68.3",                                { 0, 0, 0, 0, 0, 0, 0x10d, 0x344 } },
        { "::123.123.123.123",                          { 0, 0, 0, 0, 0, 0, 0x7b7b, 0x7b7b } },
        { "::ffff",                                     { 0, 0, 0, 0, 0, 0, 0, 0xffff } },
        { "::0.1.0.0",                                  { 0, 0, 0, 0, 0, 0, 0x100, 0 } },
        { "::ffff:13.1.68.3",                           { 0, 0, 0, 0, 0, 0xffff, 0x10d, 0x344 } },
        { "::feff:d01:4403",                            { 0, 0, 0, 0, 0, 0xfffe, 0x10d, 0x344 } },
        { "::fffe:d01:4403",                            { 0, 0, 0, 0, 0, 0xfeff, 0x10d, 0x344 } },
        { "::100:d01:4403",                             { 0, 0, 0, 0, 0, 1, 0x10d, 0x344 } },
        { "::1:d01:4403",                               { 0, 0, 0, 0, 0, 0x100, 0x10d, 0x344 } },
        { "::1:0:d01:4403",                             { 0, 0, 0, 0, 0x100, 0, 0x10d, 0x344 } },
        { "::fffe:d01:4403",                            { 0, 0, 0, 0, 0, 0xfeff, 0x10d, 0x344 } },
        { "::fffe:0:d01:4403",                          { 0, 0, 0, 0, 0xfeff, 0, 0x10d, 0x344 } },
        { "::ffff:0:4403",                              { 0, 0, 0, 0, 0, 0xffff, 0, 0x344 } },
        { "::ffff:0.1.0.0",                             { 0, 0, 0, 0, 0, 0xffff, 0x100, 0 } },
        { "::ffff:13.1.0.0",                            { 0, 0, 0, 0, 0, 0xffff, 0x10d, 0 } },
        { "::ffff:0:0",                                 { 0, 0, 0, 0, 0, 0xffff, 0, 0 } },
        { "::ffff:0:ffff",                              { 0, 0, 0, 0, 0, 0xffff, 0, 0xffff } },
        { "::ffff:0:0.1.0.0",                           { 0, 0, 0, 0, 0xffff, 0, 0x100, 0 } },
        { "::ffff:0:13.1.68.3",                         { 0, 0, 0, 0, 0xffff, 0, 0x10d, 0x344 } },
        { "::ffff:ffff:d01:4403",                       { 0, 0, 0, 0, 0xffff, 0xffff, 0x10d, 0x344 } },
        { "::ffff:0:0:d01:4403",                        { 0, 0, 0, 0xffff, 0, 0, 0x10d, 0x344 } },
        { "::ffff:255.255.255.255",                     { 0, 0, 0, 0, 0, 0xffff, 0xffff, 0xffff } },
        { "::ffff:129.144.52.38",                       { 0, 0, 0, 0, 0, 0xffff, 0x9081, 0x2634 } },
        { "::5efe:0.0.0.0",                             { 0, 0, 0, 0, 0, 0xfe5e, 0, 0 } },
        { "::5efe:129.144.52.38",                       { 0, 0, 0, 0, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "1111:2222:3333:4444:0:5efe:129.144.52.38",   { 0x1111, 0x2222, 0x3333, 0x4444, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "1111:2222:3333::5efe:129.144.52.38",         { 0x1111, 0x2222, 0x3333, 0, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "1111:2222::5efe:129.144.52.38",              { 0x1111, 0x2222, 0, 0, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "1111::5efe:129.144.52.38",                   { 0x1111, 0, 0, 0, 0, 0xfe5e, 0x9081, 0x2634 } },
        { "::300:5efe:8190:3426",                       { 0, 0, 0, 0, 3, 0xfe5e, 0x9081, 0x2634 } },
        { "::200:5efe:129.144.52.38",                   { 0, 0, 0, 0, 2, 0xfe5e, 0x9081, 0x2634 } },
        { "::100:5efe:8190:3426",                       { 0, 0, 0, 0, 1, 0xfe5e, 0x9081, 0x2634 } },
        /* 'normal' addresses */
        { "::1",                                        { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "::2",                                        { 0, 0, 0, 0, 0, 0, 0, 0x200 } },
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
    unsigned int i;

    memset(buffer, '#', sizeof(buffer));
    buffer[sizeof(buffer)-1] = 0;
    memset(&ip, 0, sizeof(ip));
    result = RtlIpv6AddressToStringA(&ip, buffer);

    len = strlen(buffer);
    ok(result == (buffer + len) && !strcmp(buffer, "::"),
       "got %p with '%s' (expected %p with '::')\n", result, buffer, buffer + len);

    result = RtlIpv6AddressToStringA(&ip, NULL);
    ok(result == (LPCSTR)~0 || broken(result == (LPCSTR)len) /* WinXP / Win2k3 */,
       "got %p, expected %p\n", result, (LPCSTR)~0);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        init_ip6(&ip, tests[i].ip);
        memset(buffer, '#', sizeof(buffer));
        buffer[sizeof(buffer)-1] = 0;

        result = RtlIpv6AddressToStringA(&ip, buffer);
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
    static const struct
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
    unsigned int i;

    if (!pRtlIpv6AddressToStringExA)
    {
        win_skip("RtlIpv6AddressToStringExA not available\n");
        return;
    }

    memset(buffer, '#', sizeof(buffer));
    buffer[sizeof(buffer)-1] = 0;
    memset(&ip, 0, sizeof(ip));
    len = sizeof(buffer);
    res = pRtlIpv6AddressToStringExA(&ip, 0, 0, buffer, &len);

    ok(res == STATUS_SUCCESS, "[validate] res = 0x%08lx, expected STATUS_SUCCESS\n", res);
    ok(len == 3 && !strcmp(buffer, "::"),
        "got len %ld with '%s' (expected 3 with '::')\n", len, buffer);

    memset(buffer, '#', sizeof(buffer));
    buffer[sizeof(buffer)-1] = 0;

    len = sizeof(buffer);
    res = pRtlIpv6AddressToStringExA(NULL, 0, 0, buffer, &len);
    ok(res == STATUS_INVALID_PARAMETER, "[null ip] res = 0x%08lx, expected STATUS_INVALID_PARAMETER\n", res);

    len = sizeof(buffer);
    res = pRtlIpv6AddressToStringExA(&ip, 0, 0, NULL, &len);
    ok(res == STATUS_INVALID_PARAMETER, "[null buffer] res = 0x%08lx, expected STATUS_INVALID_PARAMETER\n", res);

    res = pRtlIpv6AddressToStringExA(&ip, 0, 0, buffer, NULL);
    ok(res == STATUS_INVALID_PARAMETER, "[null length] res = 0x%08lx, expected STATUS_INVALID_PARAMETER\n", res);

    len = 2;
    memset(buffer, '#', sizeof(buffer));
    buffer[sizeof(buffer)-1] = 0;
    res = pRtlIpv6AddressToStringExA(&ip, 0, 0, buffer, &len);
    ok(res == STATUS_INVALID_PARAMETER, "[null length] res = 0x%08lx, expected STATUS_INVALID_PARAMETER\n", res);
    ok(buffer[0] == '#', "got first char %c (expected '#')\n", buffer[0]);
    ok(len == 3, "got len %ld (expected len 3)\n", len);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        init_ip6(&ip, tests[i].ip);
        len = sizeof(buffer);
        memset(buffer, '#', sizeof(buffer));
        buffer[sizeof(buffer)-1] = 0;

        res = pRtlIpv6AddressToStringExA(&ip, tests[i].scopeid, tests[i].port, buffer, &len);

        ok(res == STATUS_SUCCESS, "[validate] res = 0x%08lx, expected STATUS_SUCCESS\n", res);
        ok(len == (strlen(tests[i].address) + 1) && !strcmp(buffer, tests[i].address),
           "got len %ld with '%s' (expected %d with '%s')\n", len, buffer, (int)strlen(tests[i].address), tests[i].address);
    }
}

static void compare_RtlIpv6StringToAddressW(PCSTR name_a, int terminator_offset_a,
                                            const struct in6_addr *addr_a, NTSTATUS res_a)
{
    WCHAR name[512];
    NTSTATUS res;
    IN6_ADDR ip;
    PCWSTR terminator;

    RtlMultiByteToUnicodeN(name, sizeof(name), NULL, name_a, strlen(name_a) + 1);

    init_ip6(&ip, NULL);
    terminator = (void *)0xdeadbeef;
    res = RtlIpv6StringToAddressW(name, &terminator, &ip);
    ok(res == res_a, "[W:%s] res = 0x%08lx, expected 0x%08lx\n", name_a, res, res_a);

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

    res = RtlIpv6StringToAddressA("::", &terminator, &ip);
    ok(res == STATUS_SUCCESS, "[validate] res = 0x%08lx, expected STATUS_SUCCESS\n", res);
    if (0)
    {
        /* any of these crash */
        res = RtlIpv6StringToAddressA(NULL, &terminator, &ip);
        ok(res == STATUS_INVALID_PARAMETER, "[null string] res = 0x%08lx, expected STATUS_INVALID_PARAMETER\n", res);
        res = RtlIpv6StringToAddressA("::", NULL, &ip);
        ok(res == STATUS_INVALID_PARAMETER, "[null terminator] res = 0x%08lx, expected STATUS_INVALID_PARAMETER\n", res);
        res = RtlIpv6StringToAddressA("::", &terminator, NULL);
        ok(res == STATUS_INVALID_PARAMETER, "[null result] res = 0x%08lx, expected STATUS_INVALID_PARAMETER\n", res);
    }

    /* sanity check */
    ok(sizeof(ip) == sizeof(USHORT)* 8, "sizeof(ip)\n");

    for (i = 0; i < ARRAY_SIZE(ipv6_tests); i++)
    {
        init_ip6(&ip, NULL);
        terminator = (void *)0xdeadbeef;
        res = RtlIpv6StringToAddressA(ipv6_tests[i].address, &terminator, &ip);
        compare_RtlIpv6StringToAddressW(ipv6_tests[i].address, (terminator != (void *)0xdeadbeef) ?
                                        (terminator - ipv6_tests[i].address) : -1, &ip, res);

        if (ipv6_tests[i].flags & win_broken_6)
        {
            ok(res == ipv6_tests[i].res || broken(res == STATUS_INVALID_PARAMETER),
               "[%s] res = 0x%08lx, expected 0x%08lx\n",
               ipv6_tests[i].address, res, ipv6_tests[i].res);

            if (res == STATUS_INVALID_PARAMETER)
                continue;
        }
        else
        {
            ok(res == ipv6_tests[i].res,
               "[%s] res = 0x%08lx, expected 0x%08lx\n",
               ipv6_tests[i].address, res, ipv6_tests[i].res);
        }

        if (ipv6_tests[i].terminator_offset < 0)
        {
            ok(terminator == (void *)0xdeadbeef,
               "[%s] terminator = %p, expected it not to change\n",
               ipv6_tests[i].address, terminator);
        }
        else
        {
            if (ipv6_tests[i].flags & win_extra_zero)
                ok(terminator == ipv6_tests[i].address + ipv6_tests[i].terminator_offset ||
                   broken(terminator != ipv6_tests[i].address + ipv6_tests[i].terminator_offset),
                   "[%s] terminator = %p, expected %p\n",
                   ipv6_tests[i].address, terminator, ipv6_tests[i].address + ipv6_tests[i].terminator_offset);
            else
                ok(terminator == ipv6_tests[i].address + ipv6_tests[i].terminator_offset,
                   "[%s] terminator = %p, expected %p\n",
                   ipv6_tests[i].address, terminator, ipv6_tests[i].address + ipv6_tests[i].terminator_offset);
        }

        init_ip6(&expected_ip, ipv6_tests[i].ip);
        if (ipv6_tests[i].flags & win_extra_zero)
            ok(!memcmp(&ip, &expected_ip, sizeof(ip)) || broken(memcmp(&ip, &expected_ip, sizeof(ip))),
               "[%s] ip = %x:%x:%x:%x:%x:%x:%x:%x, expected %x:%x:%x:%x:%x:%x:%x:%x\n",
               ipv6_tests[i].address, ip.s6_words[0], ip.s6_words[1], ip.s6_words[2], ip.s6_words[3],
               ip.s6_words[4], ip.s6_words[5], ip.s6_words[6], ip.s6_words[7],
               expected_ip.s6_words[0], expected_ip.s6_words[1], expected_ip.s6_words[2], expected_ip.s6_words[3],
               expected_ip.s6_words[4], expected_ip.s6_words[5], expected_ip.s6_words[6], expected_ip.s6_words[7]);
        else
            ok(!memcmp(&ip, &expected_ip, sizeof(ip)),
               "[%s] ip = %x:%x:%x:%x:%x:%x:%x:%x, expected %x:%x:%x:%x:%x:%x:%x:%x\n",
               ipv6_tests[i].address, ip.s6_words[0], ip.s6_words[1], ip.s6_words[2], ip.s6_words[3],
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

    RtlMultiByteToUnicodeN(name, sizeof(name), NULL, name_a, strlen(name_a) + 1);

    init_ip6(&ip, NULL);
    res = pRtlIpv6StringToAddressExW(name, &ip, &scope, &port);

    ok(res == res_a, "[W:%s] res = 0x%08lx, expected 0x%08lx\n", name_a, res, res_a);
    ok(scope == scope_a, "[W:%s] scope = 0x%08lx, expected 0x%08lx\n", name_a, scope, scope_a);
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
    const char *simple_ip = "::";
    unsigned int i;

    if (!pRtlIpv6StringToAddressExW)
    {
        win_skip("RtlIpv6StringToAddressExW not available\n");
        /* we can continue, just not test W */
    }

    if (!pRtlIpv6StringToAddressExA)
    {
        win_skip("RtlIpv6StringToAddressExA not available\n");
        return;
    }

    res = pRtlIpv6StringToAddressExA(simple_ip, &ip, &scope, &port);
    ok(res == STATUS_SUCCESS, "[validate] res = 0x%08lx, expected STATUS_SUCCESS\n", res);

    init_ip6(&ip, NULL);
    init_ip6(&expected_ip, NULL);
    scope = 0xbadf00d;
    port = 0xbeef;
    res = pRtlIpv6StringToAddressExA(NULL, &ip, &scope, &port);
    ok(res == STATUS_INVALID_PARAMETER,
       "[null string] res = 0x%08lx, expected STATUS_INVALID_PARAMETER\n", res);
    ok(scope == 0xbadf00d, "[null string] scope = 0x%08lx, expected 0xbadf00d\n", scope);
    ok(port == 0xbeef, "[null string] port = 0x%08x, expected 0xbeef\n", port);
    ok(!memcmp(&ip, &expected_ip, sizeof(ip)),
       "[null string] ip is changed, expected it not to change\n");


    init_ip6(&ip, NULL);
    scope = 0xbadf00d;
    port = 0xbeef;
    res = pRtlIpv6StringToAddressExA(simple_ip, NULL, &scope, &port);
    ok(res == STATUS_INVALID_PARAMETER,
       "[null result] res = 0x%08lx, expected STATUS_INVALID_PARAMETER\n", res);
    ok(scope == 0xbadf00d, "[null result] scope = 0x%08lx, expected 0xbadf00d\n", scope);
    ok(port == 0xbeef, "[null result] port = 0x%08x, expected 0xbeef\n", port);
    ok(!memcmp(&ip, &expected_ip, sizeof(ip)),
       "[null result] ip is changed, expected it not to change\n");

    init_ip6(&ip, NULL);
    scope = 0xbadf00d;
    port = 0xbeef;
    res = pRtlIpv6StringToAddressExA(simple_ip, &ip, NULL, &port);
    ok(res == STATUS_INVALID_PARAMETER,
       "[null scope] res = 0x%08lx, expected STATUS_INVALID_PARAMETER\n", res);
    ok(scope == 0xbadf00d, "[null scope] scope = 0x%08lx, expected 0xbadf00d\n", scope);
    ok(port == 0xbeef, "[null scope] port = 0x%08x, expected 0xbeef\n", port);
    ok(!memcmp(&ip, &expected_ip, sizeof(ip)),
       "[null scope] ip is changed, expected it not to change\n");

    init_ip6(&ip, NULL);
    scope = 0xbadf00d;
    port = 0xbeef;
    res = pRtlIpv6StringToAddressExA(simple_ip, &ip, &scope, NULL);
    ok(res == STATUS_INVALID_PARAMETER,
       "[null port] res = 0x%08lx, expected STATUS_INVALID_PARAMETER\n", res);
    ok(scope == 0xbadf00d, "[null port] scope = 0x%08lx, expected 0xbadf00d\n", scope);
    ok(port == 0xbeef, "[null port] port = 0x%08x, expected 0xbeef\n", port);
    ok(!memcmp(&ip, &expected_ip, sizeof(ip)),
       "[null port] ip is changed, expected it not to change\n");

    /* sanity check */
    ok(sizeof(ip) == sizeof(USHORT)* 8, "sizeof(ip)\n");

    /* first we run all ip related tests, to make sure someone didn't accidentally reimplement instead of re-use. */
    for (i = 0; i < ARRAY_SIZE(ipv6_tests); i++)
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
            ok(scope == 0xbadf00d, "[%s] scope = 0x%08lx, expected 0xbadf00d\n",
               ipv6_tests[i].address, scope);
            ok(port == 0xbeef, "[%s] port = 0x%08x, expected 0xbeef\n",
               ipv6_tests[i].address, port);
        }
        else
        {
            ok(scope != 0xbadf00d, "[%s] scope = 0x%08lx, not expected 0xbadf00d\n",
               ipv6_tests[i].address, scope);
            ok(port != 0xbeef, "[%s] port = 0x%08x, not expected 0xbeef\n",
               ipv6_tests[i].address, port);
        }

        if (ipv6_tests[i].flags & win_broken_6)
        {
            ok(res == expect_ret || broken(res == STATUS_INVALID_PARAMETER),
               "[%s] res = 0x%08lx, expected 0x%08lx\n", ipv6_tests[i].address, res, expect_ret);

            if (res == STATUS_INVALID_PARAMETER)
                continue;
        }
        else
        {
            ok(res == expect_ret, "[%s] res = 0x%08lx, expected 0x%08lx\n",
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
    for (i = 0; i < ARRAY_SIZE(ipv6_ex_tests); i++)
    {
        scope = 0xbadf00d;
        port = 0xbeef;
        init_ip6(&ip, NULL);
        res = pRtlIpv6StringToAddressExA(ipv6_ex_tests[i].address, &ip, &scope, &port);
        compare_RtlIpv6StringToAddressExW(ipv6_ex_tests[i].address, &ip, res, scope, port);

        ok(res == ipv6_ex_tests[i].res, "[%s] res = 0x%08lx, expected 0x%08lx\n",
           ipv6_ex_tests[i].address, res, ipv6_ex_tests[i].res);
        ok(scope == ipv6_ex_tests[i].scope, "[%s] scope = 0x%08lx, expected 0x%08lx\n",
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

    mod = LoadLibraryA("comctl32.dll");
    ok(mod != NULL, "got %p\n", mod);
    ret = FreeLibrary(mod);
    ok(ret, "got %d\n", ret);

    mod2 = GetModuleHandleA("comctl32.dll");
    ok(mod2 == NULL, "got %p\n", mod2);

    /* load, addref and release 2 times */
    mod = LoadLibraryA("comctl32.dll");
    ok(mod != NULL, "got %p\n", mod);
    status = LdrAddRefDll(0, mod);
    ok(status == STATUS_SUCCESS, "got 0x%08lx\n", status);
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
    status = LdrAddRefDll(LDR_ADDREF_DLL_PIN, mod);
    ok(status == STATUS_SUCCESS, "got 0x%08lx\n", status);

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

    /* invalid flags */
    result = 10;
    magic = 0xdeadbeef;
    status = LdrLockLoaderLock(0x10, &result, &magic);
    ok(status == STATUS_INVALID_PARAMETER_1, "got 0x%08lx\n", status);
    ok(result == 0, "got %ld\n", result);
    ok(magic == 0, "got %Ix\n", magic);

    magic = 0xdeadbeef;
    status = LdrLockLoaderLock(0x10, NULL, &magic);
    ok(status == STATUS_INVALID_PARAMETER_1, "got 0x%08lx\n", status);
    ok(magic == 0, "got %Ix\n", magic);

    result = 10;
    status = LdrLockLoaderLock(0x10, &result, NULL);
    ok(status == STATUS_INVALID_PARAMETER_1, "got 0x%08lx\n", status);
    ok(result == 0, "got %ld\n", result);

    /* non-blocking mode, result is null */
    magic = 0xdeadbeef;
    status = LdrLockLoaderLock(0x2, NULL, &magic);
    ok(status == STATUS_INVALID_PARAMETER_2, "got 0x%08lx\n", status);
    ok(magic == 0, "got %Ix\n", magic);

    /* magic pointer is null */
    result = 10;
    status = LdrLockLoaderLock(0, &result, NULL);
    ok(status == STATUS_INVALID_PARAMETER_3, "got 0x%08lx\n", status);
    ok(result == 0, "got %ld\n", result);

    /* lock in non-blocking mode */
    result = 0;
    magic = 0;
    status = LdrLockLoaderLock(0x2, &result, &magic);
    ok(status == STATUS_SUCCESS, "got 0x%08lx\n", status);
    ok(result == 1, "got %ld\n", result);
    ok(magic != 0, "got %Ix\n", magic);
    LdrUnlockLoaderLock(0, magic);
}

static void test_RtlCompressBuffer(void)
{
    ULONG compress_workspace, decompress_workspace;
    static UCHAR test_buffer[] = "WineWineWine";
    static UCHAR buf1[0x1000], buf2[0x1000];
    ULONG final_size, buf_size;
    UCHAR *workspace = NULL;
    NTSTATUS status;

    compress_workspace = decompress_workspace = 0xdeadbeef;
    status = RtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1, &compress_workspace,
                                            &decompress_workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08lx\n", status);
    ok(compress_workspace != 0, "got wrong compress_workspace %lu\n", compress_workspace);
    workspace = HeapAlloc(GetProcessHeap(), 0, compress_workspace);
    ok(workspace != NULL, "HeapAlloc failed %ld\n", GetLastError());

    /* test compression format / engine */
    final_size = 0xdeadbeef;
    status = RtlCompressBuffer(COMPRESSION_FORMAT_NONE, test_buffer, sizeof(test_buffer),
                               buf1, sizeof(buf1) - 1, 4096, &final_size, workspace);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08lx\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %lu\n", final_size);

    final_size = 0xdeadbeef;
    status = RtlCompressBuffer(COMPRESSION_FORMAT_DEFAULT, test_buffer, sizeof(test_buffer),
                               buf1, sizeof(buf1) - 1, 4096, &final_size, workspace);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08lx\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %lu\n", final_size);

    final_size = 0xdeadbeef;
    status = RtlCompressBuffer(0xFF, test_buffer, sizeof(test_buffer),
                               buf1, sizeof(buf1) - 1, 4096, &final_size, workspace);
    ok(status == STATUS_UNSUPPORTED_COMPRESSION, "got wrong status 0x%08lx\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %lu\n", final_size);

    /* test compression */
    final_size = 0xdeadbeef;
    memset(buf1, 0x11, sizeof(buf1));
    status = RtlCompressBuffer(COMPRESSION_FORMAT_LZNT1, test_buffer, sizeof(test_buffer),
                               buf1, sizeof(buf1), 4096, &final_size, workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08lx\n", status);
    ok((*(WORD *)buf1 & 0x7000) == 0x3000, "no chunk signature found %04x\n", *(WORD *)buf1);
    todo_wine
    ok(final_size < sizeof(test_buffer), "got wrong final_size %lu\n", final_size);

    /* test decompression */
    buf_size = final_size;
    final_size = 0xdeadbeef;
    memset(buf2, 0x11, sizeof(buf2));
    status = RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf2, sizeof(buf2),
                                 buf1, buf_size, &final_size);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08lx\n", status);
    ok(final_size == sizeof(test_buffer), "got wrong final_size %lu\n", final_size);
    ok(!memcmp(buf2, test_buffer, sizeof(test_buffer)), "got wrong decoded data\n");
    ok(buf2[sizeof(test_buffer)] == 0x11, "too many bytes written\n");

    /* buffer too small */
    final_size = 0xdeadbeef;
    memset(buf1, 0x11, sizeof(buf1));
    status = RtlCompressBuffer(COMPRESSION_FORMAT_LZNT1, test_buffer, sizeof(test_buffer),
                               buf1, 4, 4096, &final_size, workspace);
    ok(status == STATUS_BUFFER_TOO_SMALL, "got wrong status 0x%08lx\n", status);

    HeapFree(GetProcessHeap(), 0, workspace);
}

static void test_RtlGetCompressionWorkSpaceSize(void)
{
    ULONG compress_workspace, decompress_workspace;
    NTSTATUS status;

    /* test invalid format / engine */
    status = RtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_NONE, &compress_workspace,
                                            &decompress_workspace);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08lx\n", status);

    status = RtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_DEFAULT, &compress_workspace,
                                            &decompress_workspace);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08lx\n", status);

    status = RtlGetCompressionWorkSpaceSize(0xFF, &compress_workspace, &decompress_workspace);
    ok(status == STATUS_UNSUPPORTED_COMPRESSION, "got wrong status 0x%08lx\n", status);

    /* test LZNT1 with normal and maximum compression */
    compress_workspace = decompress_workspace = 0xdeadbeef;
    status = RtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1, &compress_workspace,
                                            &decompress_workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08lx\n", status);
    ok(compress_workspace != 0, "got wrong compress_workspace %lu\n", compress_workspace);
    ok(decompress_workspace == 0x1000, "got wrong decompress_workspace %lu\n", decompress_workspace);

    compress_workspace = decompress_workspace = 0xdeadbeef;
    status = RtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM,
                                            &compress_workspace, &decompress_workspace);
    ok(status == STATUS_SUCCESS, "got wrong status 0x%08lx\n", status);
    ok(compress_workspace != 0, "got wrong compress_workspace %lu\n", compress_workspace);
    ok(decompress_workspace == 0x1000, "got wrong decompress_workspace %lu\n", decompress_workspace);
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
    static struct
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

    /* test compression format / engine */
    final_size = 0xdeadbeef;
    status = RtlDecompressBuffer(COMPRESSION_FORMAT_NONE, buf, sizeof(buf), test_lznt[0].compressed,
                                 test_lznt[0].compressed_size, &final_size);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08lx\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %lu\n", final_size);

    final_size = 0xdeadbeef;
    status = RtlDecompressBuffer(COMPRESSION_FORMAT_DEFAULT, buf, sizeof(buf), test_lznt[0].compressed,
                                 test_lznt[0].compressed_size, &final_size);
    ok(status == STATUS_INVALID_PARAMETER, "got wrong status 0x%08lx\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %lu\n", final_size);

    final_size = 0xdeadbeef;
    status = RtlDecompressBuffer(0xFF, buf, sizeof(buf), test_lznt[0].compressed,
                                 test_lznt[0].compressed_size, &final_size);
    ok(status == STATUS_UNSUPPORTED_COMPRESSION, "got wrong status 0x%08lx\n", status);
    ok(final_size == 0xdeadbeef, "got wrong final_size %lu\n", final_size);

    /* regular tests for RtlDecompressBuffer */
    for (i = 0; i < ARRAY_SIZE(test_lznt); i++)
    {
        trace("Running test %d (compressed_size=%lu, uncompressed_size=%lu, status=0x%08lx)\n",
              i, test_lznt[i].compressed_size, test_lznt[i].uncompressed_size, test_lznt[i].status);

        /* test with very big buffer */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                     test_lznt[i].compressed_size, &final_size);
        ok(status == test_lznt[i].status || broken(status == STATUS_BAD_COMPRESSION_BUFFER &&
           (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)), "%d: got wrong status 0x%08lx\n", i, status);
        if (!status)
        {
            ok(final_size == test_lznt[i].uncompressed_size,
               "%d: got wrong final_size %lu\n", i, final_size);
            ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size),
               "%d: got wrong decoded data\n", i);
            ok(buf[test_lznt[i].uncompressed_size] == 0x11,
               "%d: buf[%lu] was modified\n", i, test_lznt[i].uncompressed_size);
        }

        /* test that modifier for compression engine is ignored */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM, buf, sizeof(buf),
                                     test_lznt[i].compressed, test_lznt[i].compressed_size, &final_size);
        ok(status == test_lznt[i].status || broken(status == STATUS_BAD_COMPRESSION_BUFFER &&
           (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)), "%d: got wrong status 0x%08lx\n", i, status);
        if (!status)
        {
            ok(final_size == test_lznt[i].uncompressed_size,
               "%d: got wrong final_size %lu\n", i, final_size);
            ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size),
               "%d: got wrong decoded data\n", i);
            ok(buf[test_lznt[i].uncompressed_size] == 0x11,
               "%d: buf[%lu] was modified\n", i, test_lznt[i].uncompressed_size);
        }

        /* test with expected output size */
        if (test_lznt[i].uncompressed_size > 0)
        {
            final_size = 0xdeadbeef;
            memset(buf, 0x11, sizeof(buf));
            status = RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, test_lznt[i].uncompressed_size,
                                         test_lznt[i].compressed, test_lznt[i].compressed_size, &final_size);
            ok(status == test_lznt[i].status, "%d: got wrong status 0x%08lx\n", i, status);
            if (!status)
            {
                ok(final_size == test_lznt[i].uncompressed_size,
                   "%d: got wrong final_size %lu\n", i, final_size);
                ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size),
                   "%d: got wrong decoded data\n", i);
                ok(buf[test_lznt[i].uncompressed_size] == 0x11,
                   "%d: buf[%lu] was modified\n", i, test_lznt[i].uncompressed_size);
            }
        }

        /* test with smaller output size */
        if (test_lznt[i].uncompressed_size > 1)
        {
            final_size = 0xdeadbeef;
            memset(buf, 0x11, sizeof(buf));
            status = RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, test_lznt[i].uncompressed_size - 1,
                                         test_lznt[i].compressed, test_lznt[i].compressed_size, &final_size);
            ok(status == test_lznt[i].status ||
               broken(status == STATUS_BAD_COMPRESSION_BUFFER && (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_TRUNCATED)),
               "%d: got wrong status 0x%08lx\n", i, status);
            if (!status)
            {
                ok(final_size == test_lznt[i].uncompressed_size - 1,
                   "%d: got wrong final_size %lu\n", i, final_size);
                ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size - 1),
                   "%d: got wrong decoded data\n", i);
                ok(buf[test_lznt[i].uncompressed_size - 1] == 0x11,
                   "%d: buf[%lu] was modified\n", i, test_lznt[i].uncompressed_size - 1);
            }
        }

        /* test with zero output size */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, buf, 0, test_lznt[i].compressed,
                                     test_lznt[i].compressed_size, &final_size);
        if (is_incomplete_chunk(test_lznt[i].compressed, test_lznt[i].compressed_size, FALSE))
            ok(status == STATUS_BAD_COMPRESSION_BUFFER, "%d: got wrong status 0x%08lx\n", i, status);
        else
        {
            ok(status == STATUS_SUCCESS, "%d: got wrong status 0x%08lx\n", i, status);
            ok(final_size == 0, "%d: got wrong final_size %lu\n", i, final_size);
            ok(buf[0] == 0x11, "%d: buf[0] was modified\n", i);
        }

        /* test RtlDecompressFragment with offset = 0 */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = RtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                       test_lznt[i].compressed_size, 0, &final_size, workspace);
        if (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)
            todo_wine
            ok(status == STATUS_BAD_COMPRESSION_BUFFER, "%d: got wrong status 0x%08lx\n", i, status);
        else
            ok(status == test_lznt[i].status, "%d: got wrong status 0x%08lx\n", i, status);
        if (!status)
        {
            ok(final_size == test_lznt[i].uncompressed_size,
               "%d: got wrong final_size %lu\n", i, final_size);
            ok(!memcmp(buf, test_lznt[i].uncompressed, test_lznt[i].uncompressed_size),
               "%d: got wrong decoded data\n", i);
            ok(buf[test_lznt[i].uncompressed_size] == 0x11,
               "%d: buf[%lu] was modified\n", i, test_lznt[i].uncompressed_size);
        }

        /* test RtlDecompressFragment with offset = 1 */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = RtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                       test_lznt[i].compressed_size, 1, &final_size, workspace);
        if (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)
            todo_wine
            ok(status == STATUS_BAD_COMPRESSION_BUFFER, "%d: got wrong status 0x%08lx\n", i, status);
        else
            ok(status == test_lznt[i].status, "%d: got wrong status 0x%08lx\n", i, status);
        if (!status)
        {
            if (test_lznt[i].uncompressed_size == 0)
            {
                todo_wine
                ok(final_size == 4095, "%d: got wrong final_size %lu\n", i, final_size);
                /* Buffer doesn't contain any useful value on Windows */
                ok(buf[4095] == 0x11, "%d: buf[4095] was modified\n", i);
            }
            else
            {
                ok(final_size == test_lznt[i].uncompressed_size - 1,
                   "%d: got wrong final_size %lu\n", i, final_size);
                ok(!memcmp(buf, test_lznt[i].uncompressed + 1, test_lznt[i].uncompressed_size - 1),
                   "%d: got wrong decoded data\n", i);
                ok(buf[test_lznt[i].uncompressed_size - 1] == 0x11,
                   "%d: buf[%lu] was modified\n", i, test_lznt[i].uncompressed_size - 1);
            }
        }

        /* test RtlDecompressFragment with offset = 4095 */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = RtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                       test_lznt[i].compressed_size, 4095, &final_size, workspace);
        if (test_lznt[i].broken_flags & DECOMPRESS_BROKEN_FRAGMENT)
            todo_wine
            ok(status == STATUS_BAD_COMPRESSION_BUFFER, "%d: got wrong status 0x%08lx\n", i, status);
        else
            ok(status == test_lznt[i].status, "%d: got wrong status 0x%08lx\n", i, status);
        if (!status)
        {
            todo_wine
            ok(final_size == 1, "%d: got wrong final_size %lu\n", i, final_size);
            todo_wine
            ok(buf[0] == 0, "%d: padding is not zero\n", i);
            ok(buf[1] == 0x11, "%d: buf[1] was modified\n", i);
        }

        /* test RtlDecompressFragment with offset = 4096 */
        final_size = 0xdeadbeef;
        memset(buf, 0x11, sizeof(buf));
        status = RtlDecompressFragment(COMPRESSION_FORMAT_LZNT1, buf, sizeof(buf), test_lznt[i].compressed,
                                       test_lznt[i].compressed_size, 4096, &final_size, workspace);
        expected_status = is_incomplete_chunk(test_lznt[i].compressed, test_lznt[i].compressed_size, TRUE) ?
                          test_lznt[i].status : STATUS_SUCCESS;
        ok(status == expected_status, "%d: got wrong status 0x%08lx, expected 0x%08lx\n", i, status, expected_status);
        if (!status)
        {
            ok(final_size == 0, "%d: got wrong final_size %lu\n", i, final_size);
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
    ok(ret == TRUE, "expected TRUE, got %lu\n", ret);
    ret = pRtlIsCriticalSectionLockedByThread(&info->crit);
    ok(ret == FALSE, "expected FALSE, got %lu\n", ret);

    ReleaseSemaphore(info->semaphores[0], 1, NULL);
    ret = WaitForSingleObject(info->semaphores[1], 1000);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %lu\n", ret);

    ret = pRtlIsCriticalSectionLocked(&info->crit);
    ok(ret == FALSE, "expected FALSE, got %lu\n", ret);
    ret = pRtlIsCriticalSectionLockedByThread(&info->crit);
    ok(ret == FALSE, "expected FALSE, got %lu\n", ret);

    EnterCriticalSection(&info->crit);

    ret = pRtlIsCriticalSectionLocked(&info->crit);
    ok(ret == TRUE, "expected TRUE, got %lu\n", ret);
    ret = pRtlIsCriticalSectionLockedByThread(&info->crit);
    ok(ret == TRUE, "expected TRUE, got %lu\n", ret);

    ReleaseSemaphore(info->semaphores[0], 1, NULL);
    ret = WaitForSingleObject(info->semaphores[1], 1000);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %lu\n", ret);

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
    ok(info.semaphores[0] != NULL, "CreateSemaphore failed with %lu\n", GetLastError());
    info.semaphores[1] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(info.semaphores[1] != NULL, "CreateSemaphore failed with %lu\n", GetLastError());

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
    ok(thread != NULL, "CreateThread failed with %lu\n", GetLastError());
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
    ok(cs.DebugInfo == no_debug || broken(cs.DebugInfo != NULL && cs.DebugInfo != no_debug) /* < Win8 */,
       "expected DebugInfo != NULL and DebugInfo != ~0, got %p\n", cs.DebugInfo);
    ok(cs.LockCount == -1, "expected LockCount == -1, got %ld\n", cs.LockCount);
    ok(cs.RecursionCount == 0, "expected RecursionCount == 0, got %ld\n", cs.RecursionCount);
    ok(cs.LockSemaphore == NULL, "expected LockSemaphore == NULL, got %p\n", cs.LockSemaphore);
    ok(cs.SpinCount == 0 || broken(cs.SpinCount != 0) /* >= Win 8 */,
       "expected SpinCount == 0, got %Id\n", cs.SpinCount);
    RtlDeleteCriticalSection(&cs);

    memset(&cs, 0x11, sizeof(cs));
    pRtlInitializeCriticalSectionEx(&cs, 0, RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO);
    ok(cs.DebugInfo == no_debug, "expected DebugInfo == ~0, got %p\n", cs.DebugInfo);
    ok(cs.LockCount == -1, "expected LockCount == -1, got %ld\n", cs.LockCount);
    ok(cs.RecursionCount == 0, "expected RecursionCount == 0, got %ld\n", cs.RecursionCount);
    ok(cs.LockSemaphore == NULL, "expected LockSemaphore == NULL, got %p\n", cs.LockSemaphore);
    ok(cs.SpinCount == 0 || broken(cs.SpinCount != 0) /* >= Win 8 */,
       "expected SpinCount == 0, got %Id\n", cs.SpinCount);
    RtlDeleteCriticalSection(&cs);
}

static void test_RtlLeaveCriticalSection(void)
{
    RTL_CRITICAL_SECTION cs;
    NTSTATUS status;

    if (!pRtlInitializeCriticalSectionEx)
        return; /* Skip winxp */

    status = RtlInitializeCriticalSection(&cs);
    ok(!status, "RtlInitializeCriticalSection failed: %lx\n", status);

    status = RtlEnterCriticalSection(&cs);
    ok(!status, "RtlEnterCriticalSection failed: %lx\n", status);
    todo_wine
    ok(cs.LockCount == -2, "expected LockCount == -2, got %ld\n", cs.LockCount);
    ok(cs.RecursionCount == 1, "expected RecursionCount == 1, got %ld\n", cs.RecursionCount);
    ok(cs.OwningThread == ULongToHandle(GetCurrentThreadId()), "unexpected OwningThread\n");

    status = RtlLeaveCriticalSection(&cs);
    ok(!status, "RtlLeaveCriticalSection failed: %lx\n", status);
    ok(cs.LockCount == -1, "expected LockCount == -1, got %ld\n", cs.LockCount);
    ok(cs.RecursionCount == 0, "expected RecursionCount == 0, got %ld\n", cs.RecursionCount);
    ok(!cs.OwningThread, "unexpected OwningThread %p\n", cs.OwningThread);

    /*
     * Trying to leave a section that wasn't acquired modifies RecursionCount to an invalid value,
     * but doesn't modify LockCount so that an attempt to enter the section later will work.
     */
    status = RtlLeaveCriticalSection(&cs);
    ok(!status, "RtlLeaveCriticalSection failed: %lx\n", status);
    ok(cs.LockCount == -1, "expected LockCount == -1, got %ld\n", cs.LockCount);
    ok(cs.RecursionCount == -1, "expected RecursionCount == -1, got %ld\n", cs.RecursionCount);
    ok(!cs.OwningThread, "unexpected OwningThread %p\n", cs.OwningThread);

    /* and again */
    status = RtlLeaveCriticalSection(&cs);
    ok(!status, "RtlLeaveCriticalSection failed: %lx\n", status);
    ok(cs.LockCount == -1, "expected LockCount == -1, got %ld\n", cs.LockCount);
    ok(cs.RecursionCount == -2, "expected RecursionCount == -2, got %ld\n", cs.RecursionCount);
    ok(!cs.OwningThread, "unexpected OwningThread %p\n", cs.OwningThread);

    /* entering section fixes RecursionCount */
    status = RtlEnterCriticalSection(&cs);
    ok(!status, "RtlEnterCriticalSection failed: %lx\n", status);
    todo_wine
    ok(cs.LockCount == -2, "expected LockCount == -2, got %ld\n", cs.LockCount);
    ok(cs.RecursionCount == 1, "expected RecursionCount == 1, got %ld\n", cs.RecursionCount);
    ok(cs.OwningThread == ULongToHandle(GetCurrentThreadId()), "unexpected OwningThread\n");

    status = RtlLeaveCriticalSection(&cs);
    ok(!status, "RtlLeaveCriticalSection failed: %lx\n", status);
    ok(cs.LockCount == -1, "expected LockCount == -1, got %ld\n", cs.LockCount);
    ok(cs.RecursionCount == 0, "expected RecursionCount == 0, got %ld\n", cs.RecursionCount);
    ok(!cs.OwningThread, "unexpected OwningThread %p\n", cs.OwningThread);

    status = RtlDeleteCriticalSection(&cs);
    ok(!status, "RtlDeleteCriticalSection failed: %lx\n", status);
}

struct ldr_enum_context
{
    BOOL abort;
    BOOL found;
    int  count;
};

static void WINAPI ldr_enum_callback(LDR_DATA_TABLE_ENTRY *module, void *context, BOOLEAN *stop)
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
    ok(status == STATUS_SUCCESS, "LdrEnumerateLoadedModules failed with %08lx\n", status);
    ok(ctx.count > 1, "Expected more than one module, got %d\n", ctx.count);
    ok(ctx.found, "Could not find ntdll in list of modules\n");

    ctx.abort = TRUE;
    ctx.count = 0;
    status = pLdrEnumerateLoadedModules(NULL, ldr_enum_callback, &ctx);
    ok(status == STATUS_SUCCESS, "LdrEnumerateLoadedModules failed with %08lx\n", status);
    ok(ctx.count == 1, "Expected exactly one module, got %d\n", ctx.count);

    status = pLdrEnumerateLoadedModules((void *)0x1, ldr_enum_callback, (void *)0xdeadbeef);
    ok(status == STATUS_INVALID_PARAMETER, "expected STATUS_INVALID_PARAMETER, got 0x%08lx\n", status);

    status = pLdrEnumerateLoadedModules((void *)0xdeadbeef, ldr_enum_callback, (void *)0xdeadbeef);
    ok(status == STATUS_INVALID_PARAMETER, "expected STATUS_INVALID_PARAMETER, got 0x%08lx\n", status);

    status = pLdrEnumerateLoadedModules(NULL, NULL, (void *)0xdeadbeef);
    ok(status == STATUS_INVALID_PARAMETER, "expected STATUS_INVALID_PARAMETER, got 0x%08lx\n", status);
}

static void test_RtlMakeSelfRelativeSD(void)
{
    char buf[sizeof(SECURITY_DESCRIPTOR_RELATIVE) + 4];
    SECURITY_DESCRIPTOR_RELATIVE *sd_rel = (SECURITY_DESCRIPTOR_RELATIVE *)buf;
    SECURITY_DESCRIPTOR sd;
    NTSTATUS status;
    DWORD len;

    memset( &sd, 0, sizeof(sd) );
    sd.Revision = SECURITY_DESCRIPTOR_REVISION;

    len = 0;
    status = RtlMakeSelfRelativeSD( &sd, NULL, &len );
    ok( status == STATUS_BUFFER_TOO_SMALL, "got %08lx\n", status );
    ok( len == sizeof(*sd_rel), "got %lu\n", len );

    len += 4;
    status = RtlMakeSelfRelativeSD( &sd, sd_rel, &len );
    ok( status == STATUS_SUCCESS, "got %08lx\n", status );
    ok( len == sizeof(*sd_rel) + 4, "got %lu\n", len );

    len = 0;
    status = RtlAbsoluteToSelfRelativeSD( &sd, NULL, &len );
    ok( status == STATUS_BUFFER_TOO_SMALL, "got %08lx\n", status );
    ok( len == sizeof(*sd_rel), "got %lu\n", len );

    len += 4;
    status = RtlAbsoluteToSelfRelativeSD( &sd, sd_rel, &len );
    ok( status == STATUS_SUCCESS, "got %08lx\n", status );
    ok( len == sizeof(*sd_rel) + 4, "got %lu\n", len );

    sd.Control = SE_SELF_RELATIVE;
    status = RtlMakeSelfRelativeSD( &sd, sd_rel, &len );
    ok( status == STATUS_SUCCESS, "got %08lx\n", status );
    ok( len == sizeof(*sd_rel) + 4, "got %lu\n", len );

    status = RtlAbsoluteToSelfRelativeSD( &sd, sd_rel, &len );
    ok( status == STATUS_BAD_DESCRIPTOR_FORMAT, "got %08lx\n", status );
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
    LDR_DATA_TABLE_ENTRY *mod;
    DWORD *calls = context;
    LIST_ENTRY *mark;
    ULONG size;
    int i, j;

    *calls <<= 4;
    *calls |= reason;

    if (!lstrcmpiW(data->Loaded.BaseDllName->Buffer, expected_dll))
        return;

    ok(data->Loaded.Flags == 0, "Expected flags 0, got %lx\n", data->Loaded.Flags);
    ok(!lstrcmpiW(data->Loaded.BaseDllName->Buffer, expected_dll), "Expected %s, got %s\n",
       wine_dbgstr_w(expected_dll), wine_dbgstr_w(data->Loaded.BaseDllName->Buffer));
    ok(!!data->Loaded.DllBase, "Expected non zero base address\n");
    ok(data->Loaded.SizeOfImage, "Expected non zero image size\n");

    /* expect module to be last module listed in LdrData load order list */
    mark = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;
    mod = CONTAINING_RECORD(mark->Blink, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
    ok(mod->DllBase == data->Loaded.DllBase, "Expected base address %p, got %p\n",
       data->Loaded.DllBase, mod->DllBase);
    ok(!lstrcmpiW(mod->BaseDllName.Buffer, expected_dll), "Expected %s, got %s\n",
       wine_dbgstr_w(expected_dll), wine_dbgstr_w(mod->BaseDllName.Buffer));

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
    LDR_DATA_TABLE_ENTRY *mod;

    *calls <<= 4;
    *calls |= reason;

    if (reason != LDR_DLL_NOTIFICATION_REASON_LOADED)
        return;

    mark = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;
    mod = CONTAINING_RECORD(mark->Blink, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
    ok(mod->DllBase == data->Loaded.DllBase, "Expected base address %p, got %p\n",
       data->Loaded.DllBase, mod->DllBase);
    if (mod->DllBase != data->Loaded.DllBase)
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
    LDR_DATA_TABLE_ENTRY *mod;

    *calls <<= 4;
    *calls |= reason;

    if (reason != LDR_DLL_NOTIFICATION_REASON_LOADED)
        return;

    mark = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;
    mod = CONTAINING_RECORD(mark->Blink, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
    ok(mod->DllBase == data->Loaded.DllBase, "Expected base address %p, got %p\n",
       data->Loaded.DllBase, mod->DllBase);
    if (mod->DllBase != data->Loaded.DllBase)
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

    mod = LoadLibraryW(expected_dll);
    if(mod)
        FreeLibrary(mod);
    else
        expected_dll = ws2_32dllW; /* XP Default */

    /* generic test */
    status = pLdrRegisterDllNotification(0, ldr_notify_callback1, &calls, &cookie);
    ok(!status, "Expected STATUS_SUCCESS, got %08lx\n", status);

    calls = 0;
    mod = LoadLibraryW(expected_dll);
    ok(!!mod, "Failed to load library: %ld\n", GetLastError());
    ok(calls == LDR_DLL_NOTIFICATION_REASON_LOADED, "Expected LDR_DLL_NOTIFICATION_REASON_LOADED, got %lx\n", calls);

    calls = 0;
    FreeLibrary(mod);
    ok(calls == LDR_DLL_NOTIFICATION_REASON_UNLOADED, "Expected LDR_DLL_NOTIFICATION_REASON_UNLOADED, got %lx\n", calls);

    /* test order of callbacks */
    status = pLdrRegisterDllNotification(0, ldr_notify_callback2, &calls, &cookie2);
    ok(!status, "Expected STATUS_SUCCESS, got %08lx\n", status);

    calls = 0;
    mod = LoadLibraryW(expected_dll);
    ok(!!mod, "Failed to load library: %ld\n", GetLastError());
    ok(calls == 0x13, "Expected order 0x13, got %lx\n", calls);

    calls = 0;
    FreeLibrary(mod);
    ok(calls == 0x24, "Expected order 0x24, got %lx\n", calls);

    pLdrUnregisterDllNotification(cookie2);
    pLdrUnregisterDllNotification(cookie);

    /* test dll main order */
    status = pLdrRegisterDllNotification(0, ldr_notify_callback_dll_main, &calls, &cookie);
    ok(!status, "Expected STATUS_SUCCESS, got %08lx\n", status);

    calls = 0;
    mod = LoadLibraryW(expected_dll);
    ok(!!mod, "Failed to load library: %ld\n", GetLastError());
    ok(calls == 0x13, "Expected order 0x13, got %lx\n", calls);

    calls = 0;
    FreeLibrary(mod);
    ok(calls == 0x42, "Expected order 0x42, got %lx\n", calls);

    pLdrUnregisterDllNotification(cookie);

    /* test dll main order */
    status = pLdrRegisterDllNotification(0, ldr_notify_callback_fail, &calls, &cookie);
    ok(!status, "Expected STATUS_SUCCESS, got %08lx\n", status);

    calls = 0;
    mod = LoadLibraryW(expected_dll);
    ok(!mod, "Expected library to fail loading\n");
    ok(calls == 0x1342, "Expected order 0x1342, got %lx\n", calls);

    pLdrUnregisterDllNotification(cookie);

    /* test dll with dependencies */
    status = pLdrRegisterDllNotification(0, ldr_notify_callback_imports, &calls, &cookie);
    ok(!status, "Expected STATUS_SUCCESS, got %08lx\n", status);

    calls = 0;
    mod = LoadLibraryW(wintrustdllW);
    ok(!!mod, "Failed to load library: %ld\n", GetLastError());
    ok(calls == 0x12 || calls == 0x21, "got %lx\n", calls);

    FreeLibrary(mod);
    pLdrUnregisterDllNotification(cookie);
}

static BOOL test_dbg_print_except;
static LONG test_dbg_print_except_ret;

static LONG CALLBACK test_dbg_print_except_handler( EXCEPTION_POINTERS *eptrs )
{
    if (eptrs->ExceptionRecord->ExceptionCode == DBG_PRINTEXCEPTION_C)
    {
        ok( eptrs->ExceptionRecord->NumberParameters == 2,
            "Unexpected NumberParameters: %ld\n", eptrs->ExceptionRecord->NumberParameters );
        ok( eptrs->ExceptionRecord->ExceptionInformation[0] == strlen("test_DbgPrint: Hello World") + 1,
            "Unexpected ExceptionInformation[0]: %d\n", (int)eptrs->ExceptionRecord->ExceptionInformation[0] );
        ok( !strcmp((char *)eptrs->ExceptionRecord->ExceptionInformation[1], "test_DbgPrint: Hello World"),
            "Unexpected ExceptionInformation[1]: %s\n", wine_dbgstr_a((char *)eptrs->ExceptionRecord->ExceptionInformation[1]) );
        test_dbg_print_except = TRUE;
        return test_dbg_print_except_ret;
    }

    return (LONG)EXCEPTION_CONTINUE_SEARCH;
}

static NTSTATUS WINAPIV test_vDbgPrintEx( ULONG id, ULONG level, const char *fmt, ... )
{
    NTSTATUS status;
    va_list args;
    va_start( args, fmt );
    status = vDbgPrintEx( id, level, fmt, args );
    va_end( args );
    return status;
}

static NTSTATUS WINAPIV test_vDbgPrintExWithPrefix( const char *prefix, ULONG id, ULONG level, const char *fmt, ... )
{
    NTSTATUS status;
    va_list args;
    va_start( args, fmt );
    status = vDbgPrintExWithPrefix( prefix, id, level, fmt, args );
    va_end( args );
    return status;
}

static void test_DbgPrint(void)
{
    NTSTATUS status;
    void *handler = RtlAddVectoredExceptionHandler( TRUE, test_dbg_print_except_handler );
    PEB *Peb = NtCurrentTeb()->Peb;
    BOOL debugged = Peb->BeingDebugged;

    test_dbg_print_except = FALSE;
    test_dbg_print_except_ret = (LONG)EXCEPTION_EXECUTE_HANDLER;
    status = DbgPrint( "test_DbgPrint: %s", "Hello World" );
    ok( !status, "DbgPrint returned %lx\n", status );
    ok( !test_dbg_print_except, "DBG_PRINTEXCEPTION_C received\n" );

    Peb->BeingDebugged = TRUE;
    test_dbg_print_except = FALSE;
    test_dbg_print_except_ret = (LONG)EXCEPTION_EXECUTE_HANDLER;
    status = DbgPrint( "test_DbgPrint: %s", "Hello World" );
    ok( !status, "DbgPrint returned %lx\n", status );
    ok( test_dbg_print_except, "DBG_PRINTEXCEPTION_C not received\n" );

    test_dbg_print_except = FALSE;
    test_dbg_print_except_ret = (LONG)EXCEPTION_CONTINUE_EXECUTION;
    status = DbgPrint( "test_DbgPrint: %s", "Hello World" );
    ok( !status, "DbgPrint returned %lx\n", status );
    ok( test_dbg_print_except, "DBG_PRINTEXCEPTION_C not received\n" );

    test_dbg_print_except = FALSE;
    test_dbg_print_except_ret = (LONG)EXCEPTION_CONTINUE_SEARCH;
    status = DbgPrint( "test_DbgPrint: %s", "Hello World" );
    ok( !status, "DbgPrint returned %lx\n", status );
    ok( test_dbg_print_except, "DBG_PRINTEXCEPTION_C not received\n" );


    /* FIXME: NtSetDebugFilterState / DbgSetDebugFilterState are probably what's controlling these */

    test_dbg_print_except = FALSE;
    test_dbg_print_except_ret = (LONG)EXCEPTION_EXECUTE_HANDLER;
    status = DbgPrintEx( 0, DPFLTR_ERROR_LEVEL, "test_DbgPrint: %s", "Hello World" );
    ok( !status, "DbgPrintEx returned %lx\n", status );
    ok( test_dbg_print_except, "DBG_PRINTEXCEPTION_C not received\n" );

    test_dbg_print_except = FALSE;
    test_dbg_print_except_ret = (LONG)EXCEPTION_EXECUTE_HANDLER;
    status = DbgPrintEx( 0, DPFLTR_WARNING_LEVEL, "test_DbgPrint: %s", "Hello World" );
    ok( !status, "DbgPrintEx returned %lx\n", status );
    ok( !test_dbg_print_except, "DBG_PRINTEXCEPTION_C not received\n" );

    test_dbg_print_except = FALSE;
    test_dbg_print_except_ret = (LONG)EXCEPTION_EXECUTE_HANDLER;
    status = DbgPrintEx( 0, DPFLTR_MASK|(1 << DPFLTR_ERROR_LEVEL), "test_DbgPrint: %s", "Hello World" );
    ok( !status, "DbgPrintEx returned %lx\n", status );
    ok( test_dbg_print_except, "DBG_PRINTEXCEPTION_C not received\n" );

    test_dbg_print_except = FALSE;
    test_dbg_print_except_ret = (LONG)EXCEPTION_EXECUTE_HANDLER;
    status = DbgPrintEx( 0, DPFLTR_MASK|(1 << DPFLTR_WARNING_LEVEL), "test_DbgPrint: %s", "Hello World" );
    ok( !status, "DbgPrintEx returned %lx\n", status );
    ok( !test_dbg_print_except, "DBG_PRINTEXCEPTION_C not received\n" );


    test_dbg_print_except = FALSE;
    test_dbg_print_except_ret = (LONG)EXCEPTION_EXECUTE_HANDLER;
    status = test_vDbgPrintEx( 0, 0xFFFFFFFF, "test_DbgPrint: %s", "Hello World" );
    ok( !status, "vDbgPrintEx returned %lx\n", status );
    ok( test_dbg_print_except, "DBG_PRINTEXCEPTION_C not received\n" );

    test_dbg_print_except = FALSE;
    test_dbg_print_except_ret = (LONG)EXCEPTION_EXECUTE_HANDLER;
    status = test_vDbgPrintExWithPrefix( "test_", 0, 0xFFFFFFFF, "DbgPrint: %s", "Hello World" );
    ok( !status, "vDbgPrintExWithPrefix returned %lx\n", status );
    ok( test_dbg_print_except, "DBG_PRINTEXCEPTION_C not received\n" );

    Peb->BeingDebugged = debugged;
    RtlRemoveVectoredExceptionHandler( handler );
}

static BOOL test_heap_destroy_dbgstr = FALSE;
static BOOL test_heap_destroy_break = FALSE;

static LONG CALLBACK test_heap_destroy_except_handler( EXCEPTION_POINTERS *eptrs )
{
    if (eptrs->ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT)
    {
#if defined( __i386__ )
        eptrs->ContextRecord->Eip += 1;
        test_heap_destroy_break = TRUE;
        return (LONG)EXCEPTION_CONTINUE_EXECUTION;
#elif defined( __x86_64__ )
        eptrs->ContextRecord->Rip += 1;
        test_heap_destroy_break = TRUE;
        return (LONG)EXCEPTION_CONTINUE_EXECUTION;
#elif defined( __aarch64__ )
        eptrs->ContextRecord->Pc += 4;
        test_heap_destroy_break = TRUE;
        return (LONG)EXCEPTION_CONTINUE_EXECUTION;
#endif
    }

    if (eptrs->ExceptionRecord->ExceptionCode == DBG_PRINTEXCEPTION_C)
    {
        test_heap_destroy_dbgstr = TRUE;
        return (LONG)EXCEPTION_CONTINUE_EXECUTION;
    }

    return (LONG)EXCEPTION_CONTINUE_SEARCH;
}

/* partially copied from ntdll/heap.c */
#define HEAP_VALIDATE_PARAMS 0x40000000

struct heap
{
    DWORD_PTR unknown1[2];
    DWORD     unknown2[2];
    DWORD_PTR unknown3[4];
    DWORD     unknown4;
    DWORD_PTR unknown5[2];
    DWORD     unknown6[3];
    DWORD_PTR unknown7[2];
    DWORD     flags;
    DWORD     force_flags;
    DWORD_PTR unknown8[6];
};

static void test_RtlDestroyHeap(void)
{
    const struct heap invalid = {{0, 0}, {0, HEAP_VALIDATE_PARAMS}, {0, 0, 0, 0}, 0, {0, 0}, {0, 0, 0}, {0, 0}, HEAP_VALIDATE_PARAMS, 0, {0}};
    HANDLE heap = (HANDLE)&invalid, ret;
    PEB *Peb = NtCurrentTeb()->Peb;
    BOOL debugged;
    void *handler = RtlAddVectoredExceptionHandler( TRUE, test_heap_destroy_except_handler );

    test_heap_destroy_dbgstr = FALSE;
    test_heap_destroy_break = FALSE;
    debugged = Peb->BeingDebugged;
    Peb->BeingDebugged = TRUE;
    ret = RtlDestroyHeap( heap );
    ok( ret == heap, "RtlDestroyHeap(%p) returned %p\n", heap, ret );
    ok( test_heap_destroy_dbgstr, "HeapDestroy didn't call OutputDebugStrA\n" );
    ok( test_heap_destroy_break, "HeapDestroy didn't call DbgBreakPoint\n" );
    Peb->BeingDebugged = debugged;

    RtlRemoveVectoredExceptionHandler( handler );
}

struct commit_routine_context
{
    void *base;
    SIZE_T size;
};

static struct commit_routine_context commit_context;

static NTSTATUS NTAPI test_commit_routine(void *base, void **address, SIZE_T *size)
{
    commit_context.base = base;
    commit_context.size = *size;

    return VirtualAlloc(*address, *size, MEM_COMMIT, PAGE_READWRITE) ? 0 : STATUS_ASSERTION_FAILURE;
}

static void test_RtlCreateHeap(void)
{
    void *ptr, *base, *reserve;
    RTL_HEAP_PARAMETERS params;
    HANDLE heap;
    BOOL ret;

    heap = RtlCreateHeap(0, NULL, 0, 0, NULL, NULL);
    ok(!!heap, "Failed to create a heap.\n");
    RtlDestroyHeap(heap);

    memset(&params, 0, sizeof(params));
    heap = RtlCreateHeap(0, NULL, 0, 0, NULL, &params);
    ok(!!heap, "Failed to create a heap.\n");
    RtlDestroyHeap(heap);

    params.Length = 1;
    heap = RtlCreateHeap(0, NULL, 0, 0, NULL, &params);
    ok(!!heap, "Failed to create a heap.\n");
    RtlDestroyHeap(heap);

    params.Length = sizeof(params);
    params.CommitRoutine = test_commit_routine;
    params.InitialCommit = 0x1000;
    params.InitialReserve = 0x10000;

    heap = RtlCreateHeap(0, NULL, 0, 0, NULL, &params);
    todo_wine
    ok(!heap, "Unexpected heap.\n");
    if (heap)
        RtlDestroyHeap(heap);

    reserve = VirtualAlloc(NULL, 0x10000, MEM_RESERVE, PAGE_READWRITE);
    base = VirtualAlloc(reserve, 0x1000, MEM_COMMIT, PAGE_READWRITE);
    ok(!!base, "Unexpected pointer.\n");

    heap = RtlCreateHeap(0, base, 0, 0, NULL, &params);
    ok(!!heap, "Unexpected heap.\n");

    /* Using block size above initially committed size to trigger
       new allocation via user callback. */
    ptr = RtlAllocateHeap(heap, 0, 0x4000);
    ok(!!ptr, "Failed to allocate a block.\n");
    todo_wine
    ok(commit_context.base == base, "Unexpected base %p.\n", commit_context.base);
    todo_wine
    ok(!!commit_context.size, "Unexpected allocation size.\n");
    RtlFreeHeap(heap, 0, ptr);
    RtlDestroyHeap(heap);

    ret = VirtualFree(reserve, 0, MEM_RELEASE);
    todo_wine
    ok(ret, "Unexpected return value.\n");
}

static void test_RtlFirstFreeAce(void)
{
    PACL acl;
    PACE_HEADER first;
    BOOL ret;
    DWORD size;
    BOOLEAN found;

    size = sizeof(ACL) + (sizeof(ACCESS_ALLOWED_ACE));
    acl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    ret = InitializeAcl(acl, sizeof(ACL), ACL_REVISION);
    ok(ret, "InitializeAcl failed with error %ld\n", GetLastError());

    /* AceCount = 0 */
    first = (ACE_HEADER *)0xdeadbeef;
    found = RtlFirstFreeAce(acl, &first);
    ok(found, "RtlFirstFreeAce failed\n");
    ok(first == (PACE_HEADER)(acl + 1), "Failed to find ACL\n");

    acl->AclSize = sizeof(ACL) - 1;
    first = (ACE_HEADER *)0xdeadbeef;
    found = RtlFirstFreeAce(acl, &first);
    ok(found, "RtlFirstFreeAce failed\n");
    ok(first == NULL, "Found FirstAce = %p\n", first);

    /* AceCount = 1 */
    acl->AceCount = 1;
    acl->AclSize = size;
    first = (ACE_HEADER *)0xdeadbeef;
    found = RtlFirstFreeAce(acl, &first);
    ok(found, "RtlFirstFreeAce failed\n");
    ok(first == (PACE_HEADER)(acl + 1), "Failed to find ACL %p, %p\n", first, (PACE_HEADER)(acl + 1));

    acl->AclSize = sizeof(ACL) - 1;
    first = (ACE_HEADER *)0xdeadbeef;
    found = RtlFirstFreeAce(acl, &first);
    ok(!found, "RtlFirstFreeAce failed\n");
    ok(first == NULL, "Found FirstAce = %p\n", first);

    acl->AclSize = sizeof(ACL);
    first = (ACE_HEADER *)0xdeadbeef;
    found = RtlFirstFreeAce(acl, &first);
    ok(!found, "RtlFirstFreeAce failed\n");
    ok(first == NULL, "Found FirstAce = %p\n", first);

    HeapFree(GetProcessHeap(), 0, acl);
}

static void test_RtlInitializeSid(void)
{
    SID_IDENTIFIER_AUTHORITY sid_ident = { SECURITY_NT_AUTHORITY };
    char buffer[SECURITY_MAX_SID_SIZE];
    PSID sid = (PSID)&buffer;
    NTSTATUS status;

    status = RtlInitializeSid(sid, &sid_ident, 1);
    ok(!status, "Unexpected status %#lx.\n", status);

    status = RtlInitializeSid(sid, &sid_ident, SID_MAX_SUB_AUTHORITIES);
    ok(!status, "Unexpected status %#lx.\n", status);

    status = RtlInitializeSid(sid, &sid_ident, SID_MAX_SUB_AUTHORITIES + 1);
    ok(status == STATUS_INVALID_PARAMETER, "Unexpected status %#lx.\n", status);
}

static void test_RtlValidSecurityDescriptor(void)
{
    SECURITY_DESCRIPTOR *sd;
    NTSTATUS status;
    BOOLEAN ret;

    ret = RtlValidSecurityDescriptor(NULL);
    ok(!ret, "Unexpected return value %d.\n", ret);

    sd = calloc(1, SECURITY_DESCRIPTOR_MIN_LENGTH);

    ret = RtlValidSecurityDescriptor(sd);
    ok(!ret, "Unexpected return value %d.\n", ret);

    status = RtlCreateSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    ok(!status, "Unexpected return value %#lx.\n", status);

    ret = RtlValidSecurityDescriptor(sd);
    ok(ret, "Unexpected return value %d.\n", ret);

    free(sd);
}

static void test_RtlFindExportedRoutineByName(void)
{
    void *proc;

    if (!pRtlFindExportedRoutineByName)
    {
        win_skip( "RtlFindExportedRoutineByName is not present\n" );
        return;
    }
    proc = pRtlFindExportedRoutineByName( GetModuleHandleW( L"kernelbase" ), "CtrlRoutine" );
    ok( proc != NULL, "Expected non NULL address\n" );
    proc = pRtlFindExportedRoutineByName( GetModuleHandleW( L"kernel32" ), "CtrlRoutine" );
    ok( proc == NULL, "Shouldn't find forwarded function\n" );
}

static void test_RtlGetDeviceFamilyInfoEnum(void)
{
#if !defined(__REACTOS__) || _WIN32_WINNT >= _WIN32_WINNT_WIN10
    ULONGLONG version;
    DWORD family, form;

    if (!pRtlGetDeviceFamilyInfoEnum)
    {
        win_skip( "RtlGetDeviceFamilyInfoEnum is not present\n" );
        return;
    }

    version = 0x1234567;
    family = 1234567;
    form = 1234567;
    pRtlGetDeviceFamilyInfoEnum(&version, &family, &form);
    ok( version != 0x1234567, "got unexpected unchanged value 0x1234567\n" );
    ok( family <= DEVICEFAMILYINFOENUM_MAX, "got unexpected %lu\n", family );
    ok( form <= DEVICEFAMILYDEVICEFORM_MAX, "got unexpected %lu\n", form );
    trace( "UAP version is %#I64x, device family is %lu, form factor is %lu\n", version, family, form );
#endif
}

struct test_rb_tree_entry
{
    int value;
    struct rb_entry wine_rb_entry;
    RTL_BALANCED_NODE rtl_entry;
};

static int test_rb_tree_entry_compare( const void *key, const struct wine_rb_entry *entry )
{
    const struct test_rb_tree_entry *t = WINE_RB_ENTRY_VALUE(entry, struct test_rb_tree_entry, wine_rb_entry);
    const int *value = key;

    return *value - t->value;
}

static int test_rtl_rb_tree_entry_compare( const void *key, const RTL_BALANCED_NODE *entry )
{
    const struct test_rb_tree_entry *t = CONTAINING_RECORD(entry, struct test_rb_tree_entry, rtl_entry);
    const int *value = key;

    return *value - t->value;
}

static int rtl_rb_tree_put( RTL_RB_TREE *tree, const void *key, RTL_BALANCED_NODE *entry,
                            int (*compare_func)( const void *key, const RTL_BALANCED_NODE *entry ))
{
    RTL_BALANCED_NODE *parent = tree->root;
    BOOLEAN right = 0;
    int c;

    while (parent)
    {
        if (!(c = compare_func( key, parent ))) return -1;
        right = c > 0;
        if (!parent->Children[right]) break;
        parent = parent->Children[right];
    }
    pRtlRbInsertNodeEx( tree, parent, right, entry );
    return 0;
}

static struct test_rb_tree_entry *test_rb_tree_entry_from_wine_rb( struct rb_entry *entry )
{
    if (!entry) return NULL;
    return CONTAINING_RECORD(entry, struct test_rb_tree_entry, wine_rb_entry);
}

static struct test_rb_tree_entry *test_rb_tree_entry_from_rtl_rb( RTL_BALANCED_NODE *entry )
{
    if (!entry) return NULL;
    return CONTAINING_RECORD(entry, struct test_rb_tree_entry, rtl_entry);
}

static struct test_rb_tree_entry *test_rb_tree_entry_rtl_parent( struct test_rb_tree_entry *node )
{
    return test_rb_tree_entry_from_rtl_rb( (void *)(node->rtl_entry.ParentValue
                                           & ~(ULONG_PTR)RTL_BALANCED_NODE_RESERVED_PARENT_MASK) );
}

static void test_rb_tree(void)
{
    static int test_values[] = { 44, 51, 6, 66, 69, 20, 87, 80, 72, 86, 90, 16, 54, 61, 62, 14, 27, 39, 42, 41 };
    static const unsigned int count = ARRAY_SIZE(test_values);

    struct test_rb_tree_entry *nodes, *parent, *parent2;
    RTL_BALANCED_NODE *prev_min_entry = NULL;
    int ret, is_red, min_val;
    struct rb_tree wine_tree;
    RTL_RB_TREE rtl_tree;
    unsigned int i;

    if (!pRtlRbInsertNodeEx)
    {
        win_skip( "RtlRbInsertNodeEx is not present.\n" );
        return;
    }

    memset( &rtl_tree, 0, sizeof(rtl_tree) );
    nodes = malloc( count * sizeof(*nodes) );
    memset( nodes, 0xcc, count * sizeof(*nodes) );

    min_val = test_values[0];
    rb_init( &wine_tree, test_rb_tree_entry_compare );
    for (i = 0; i < count; ++i)
    {
        winetest_push_context( "i %u", i );
        nodes[i].value = test_values[i];
        ret = rb_put( &wine_tree, &nodes[i].value, &nodes[i].wine_rb_entry );
        ok( !ret, "got %d.\n", ret );
        parent = test_rb_tree_entry_from_wine_rb( nodes[i].wine_rb_entry.parent );
        ret = rtl_rb_tree_put( &rtl_tree, &nodes[i].value, &nodes[i].rtl_entry, test_rtl_rb_tree_entry_compare );
        ok( !ret, "got %d.\n", ret );
        parent2 = test_rb_tree_entry_rtl_parent( &nodes[i] );
        ok( parent == parent2, "got %p, %p.\n", parent, parent2 );
        is_red = nodes[i].rtl_entry.ParentValue & RTL_BALANCED_NODE_RESERVED_PARENT_MASK;
        ok( is_red == rb_is_red( &nodes[i].wine_rb_entry ), "got %d, expected %d.\n", is_red,
            rb_is_red( &nodes[i].wine_rb_entry ));

        parent = test_rb_tree_entry_from_wine_rb( wine_tree.root );
        parent2 = test_rb_tree_entry_from_rtl_rb( rtl_tree.root );
        ok( parent == parent2, "got %p, %p.\n", parent, parent2 );
        if (nodes[i].value <= min_val)
        {
            min_val = nodes[i].value;
            prev_min_entry = &nodes[i].rtl_entry;
        }
        ok( rtl_tree.min == prev_min_entry, "unexpected min tree entry.\n" );
        winetest_pop_context();
    }

    for (i = 0; i < count; ++i)
    {
        struct test_rb_tree_entry *node;

        winetest_push_context( "i %u", i );
        rb_remove( &wine_tree, &nodes[i].wine_rb_entry );
        pRtlRbRemoveNode( &rtl_tree, &nodes[i].rtl_entry );

        parent = test_rb_tree_entry_from_wine_rb( wine_tree.root );
        parent2 = test_rb_tree_entry_from_rtl_rb( rtl_tree.root );
        ok( parent == parent2, "got %p, %p.\n", parent, parent2 );

        parent = test_rb_tree_entry_from_wine_rb( rb_head( wine_tree.root ));
        parent2 = test_rb_tree_entry_from_rtl_rb( rtl_tree.min );
        ok( parent == parent2, "got %p, %p.\n", parent, parent2 );

        RB_FOR_EACH_ENTRY(node, &wine_tree, struct test_rb_tree_entry, wine_rb_entry)
        {
            is_red = node->rtl_entry.ParentValue & RTL_BALANCED_NODE_RESERVED_PARENT_MASK;
            ok( is_red == rb_is_red( &node->wine_rb_entry ), "got %d, expected %d.\n", is_red, rb_is_red( &node->wine_rb_entry ));
            parent = test_rb_tree_entry_from_wine_rb( node->wine_rb_entry.parent );
            parent2 = test_rb_tree_entry_rtl_parent( node );
            ok( parent == parent2, "got %p, %p.\n", parent, parent2 );
        }
        winetest_pop_context();
    }
    ok( !rtl_tree.root, "got %p.\n", rtl_tree.root );
    ok( !rtl_tree.min, "got %p.\n", rtl_tree.min );
    free(nodes);
}

static void test_RtlConvertDeviceFamilyInfoToString(void)
{
    DWORD device_family_size, device_form_size, ret;
    WCHAR device_family[16], device_form[16];

    if (!pRtlConvertDeviceFamilyInfoToString)
    {
        win_skip("RtlConvertDeviceFamilyInfoToString is unavailable.\n" );
        return;
    }

    if (0) /* Crash on Windows */
    {
    ret = pRtlConvertDeviceFamilyInfoToString(NULL, NULL, NULL, NULL);
    ok(ret == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", ret);

    device_family_size = 0;
    ret = pRtlConvertDeviceFamilyInfoToString(&device_family_size, NULL, NULL, NULL);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "Got unexpected status %#lx.\n", ret);
    ok(device_family_size == (wcslen(L"Windows.Desktop") + 1) * sizeof(WCHAR),
       "Got unexpected %#lx.\n", device_family_size);

    device_form_size = 0;
    ret = pRtlConvertDeviceFamilyInfoToString(NULL, &device_form_size, NULL, NULL);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "Got unexpected status %#lx.\n", ret);
    ok(device_form_size == (wcslen(L"Unknown") + 1) * sizeof(WCHAR), "Got unexpected %#lx.\n",
       device_form_size);

    ret = pRtlConvertDeviceFamilyInfoToString(&device_family_size, NULL, device_family, NULL);
    ok(ret == STATUS_SUCCESS, "Got unexpected status %#lx.\n", ret);
    ok(device_family_size == (wcslen(L"Windows.Desktop") + 1) * sizeof(WCHAR),
       "Got unexpected %#lx.\n", device_family_size);
    ok(!wcscmp(device_family, L"Windows.Desktop"), "Got unexpected %s.\n", wine_dbgstr_w(device_family));

    ret = pRtlConvertDeviceFamilyInfoToString(NULL, &device_form_size, NULL, device_form);
    ok(ret == STATUS_SUCCESS, "Got unexpected status %#lx.\n", ret);
    ok(device_form_size == (wcslen(L"Unknown") + 1) * sizeof(WCHAR), "Got unexpected %#lx.\n",
       device_form_size);
    ok(!wcscmp(device_form, L"Unknown"), "Got unexpected %s.\n", wine_dbgstr_w(device_form));

    ret = pRtlConvertDeviceFamilyInfoToString(&device_family_size, &device_form_size, NULL, NULL);
    ok(ret == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", ret);
    }

    device_family_size = wcslen(L"Windows.Desktop") * sizeof(WCHAR);
    device_form_size = wcslen(L"Unknown") * sizeof(WCHAR);
    ret = pRtlConvertDeviceFamilyInfoToString(&device_family_size, &device_form_size, NULL, NULL);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "Got unexpected status %#lx.\n", ret);
    ok(device_family_size == (wcslen(L"Windows.Desktop") + 1) * sizeof(WCHAR),
       "Got unexpected %#lx.\n", device_family_size);
    ok(device_form_size == (wcslen(L"Unknown") + 1) * sizeof(WCHAR), "Got unexpected %#lx.\n",
       device_form_size);

    ret = pRtlConvertDeviceFamilyInfoToString(&device_family_size, &device_form_size, device_family, device_form);
    ok(ret == STATUS_SUCCESS, "Got unexpected status %#lx.\n", ret);
    ok(!wcscmp(device_family, L"Windows.Desktop"), "Got unexpected %s.\n", wine_dbgstr_w(device_family));
    ok(!wcscmp(device_form, L"Unknown"), "Got unexpected %s.\n", wine_dbgstr_w(device_form));
}

static void test_user_procs(void)
{
    UINT64 ptrs[32], dummy[32] = { 0 };
    NTSTATUS status;
    const UINT64 *ptr_A, *ptr_W, *ptr_workers;
    ULONG size_A, size_W, size_workers;

    if (!pRtlRetrieveNtUserPfn || !pRtlInitializeNtUserPfn)
    {
        win_skip( "user procs not supported\n" );
        return;
    }

    status = pRtlRetrieveNtUserPfn( &ptr_A, &ptr_W, &ptr_workers );
    ok( !status || broken(!is_win64 && status == STATUS_INVALID_PARAMETER), /* <= win8 32-bit */
        "RtlRetrieveNtUserPfn failed %lx\n", status );
    if (status) return;

    /* assume that the tables are consecutive */
    size_A = (ptr_W - ptr_A) * sizeof(UINT64);
    size_W = (ptr_workers - ptr_W) * sizeof(UINT64);
    ok( size_A > 0x80 && size_A < 0x100, "unexpected size for %p %p %p\n", ptr_A, ptr_W, ptr_workers );
    ok( size_W == size_A, "unexpected size for %p %p %p\n", ptr_A, ptr_W, ptr_workers );
    memcpy( ptrs, ptr_A, size_A );

    status = pRtlInitializeNtUserPfn( dummy, size_A, dummy + 1, size_W, dummy + 2, 0 );
    ok( status == STATUS_INVALID_PARAMETER, "RtlInitializeNtUserPfn failed %lx\n", status );

    if (!pRtlResetNtUserPfn)
    {
        win_skip( "RtlResetNtUserPfn not supported\n" );
        return;
    }

    status = pRtlResetNtUserPfn();
    ok( !status, "RtlResetNtUserPfn failed %lx\n", status );
    ok( !memcmp( ptrs, ptr_A, size_A ), "pointers changed by reset\n" );

    /* can't do anything after reset except set them again */
    status = pRtlResetNtUserPfn();
    ok( status == STATUS_INVALID_PARAMETER, "RtlResetNtUserPfn failed %lx\n", status );
    status = pRtlRetrieveNtUserPfn( &ptr_A, &ptr_W, &ptr_workers );
    ok( status == STATUS_INVALID_PARAMETER, "RtlRetrieveNtUserPfn failed %lx\n", status );

    for (size_workers = 0x100; size_workers > 0; size_workers--)
    {
        status = pRtlInitializeNtUserPfn( dummy, size_A, dummy + 1, size_W, dummy + 2, size_workers );
        if (!status) break;
        ok( status == STATUS_INVALID_PARAMETER, "RtlInitializeNtUserPfn failed %lx\n", status );
    }
    trace( "got sizes %lx %lx %lx\n", size_A, size_W, size_workers );
    if (!size_workers) return;  /* something went wrong */
    ok( !memcmp( ptrs, ptr_A, size_A ), "pointers changed by init\n" );

    /* can't set twice without a reset */
    status = pRtlInitializeNtUserPfn( dummy, size_A, dummy + 1, size_W, dummy + 2, size_workers );
    ok( status == STATUS_INVALID_PARAMETER, "RtlInitializeNtUserPfn failed %lx\n", status );
    status = pRtlResetNtUserPfn();
    ok( !status, "RtlResetNtUserPfn failed %lx\n", status );
    status = pRtlInitializeNtUserPfn( dummy, size_A, dummy + 1, size_W, dummy + 2, size_workers );
    ok( !status, "RtlInitializeNtUserPfn failed %lx\n", status );
    ok( !memcmp( ptrs, ptr_A, size_A ), "pointers changed by init\n" );
}

START_TEST(rtl)
{
    InitFunctionPtrs();

    test_RtlQueryProcessDebugInformation();
    test_RtlCompareMemory();
    test_RtlCompareMemoryUlong();
    test_RtlMoveMemory();
    test_RtlFillMemory();
    test_RtlFillMemoryUlong();
    test_RtlZeroMemory();
    test_RtlByteSwap();
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
    test_RtlMakeSelfRelativeSD();
    test_LdrRegisterDllNotification();
    test_DbgPrint();
    test_RtlDestroyHeap();
    test_RtlCreateHeap();
    test_RtlFirstFreeAce();
    test_RtlInitializeSid();
    test_RtlValidSecurityDescriptor();
    test_RtlFindExportedRoutineByName();
    test_RtlGetDeviceFamilyInfoEnum();
    test_RtlConvertDeviceFamilyInfoToString();
    test_rb_tree();
    test_user_procs();
}
