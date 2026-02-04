/* Unit test suite for *Information* Registry API functions
 *
 * Copyright 2005 Paul Vriens
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
 */

#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "ddk/ntddk.h"
#include "psapi.h"
#include "wine/test.h"

#ifdef __REACTOS__
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define AlwaysOn DEPPolicyAlwaysOn
#endif

static NTSTATUS (WINAPI * pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI * pNtSetSystemInformation)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG);
static NTSTATUS (WINAPI * pRtlGetNativeSystemInformation)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI * pNtQuerySystemInformationEx)(SYSTEM_INFORMATION_CLASS, void*, ULONG, void*, ULONG, ULONG*);
static NTSTATUS (WINAPI * pNtPowerInformation)(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG);
static NTSTATUS (WINAPI * pNtQueryInformationThread)(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI * pNtSetInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG);
static NTSTATUS (WINAPI * pNtSetInformationThread)(HANDLE, THREADINFOCLASS, PVOID, ULONG);
static NTSTATUS (WINAPI * pNtReadVirtualMemory)(HANDLE, const void*, void*, SIZE_T, SIZE_T*);
static NTSTATUS (WINAPI * pNtQueryVirtualMemory)(HANDLE, LPCVOID, MEMORY_INFORMATION_CLASS , PVOID , SIZE_T , SIZE_T *);
static NTSTATUS (WINAPI * pNtCreateSection)(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*,const LARGE_INTEGER*,ULONG,ULONG,HANDLE);
static NTSTATUS (WINAPI * pNtMapViewOfSection)(HANDLE,HANDLE,PVOID*,ULONG_PTR,SIZE_T,const LARGE_INTEGER*,SIZE_T*,SECTION_INHERIT,ULONG,ULONG);
static NTSTATUS (WINAPI * pNtUnmapViewOfSection)(HANDLE,PVOID);
static NTSTATUS (WINAPI * pNtClose)(HANDLE);
static ULONG    (WINAPI * pNtGetCurrentProcessorNumber)(void);
static BOOL     (WINAPI * pGetLogicalProcessorInformationEx)(LOGICAL_PROCESSOR_RELATIONSHIP,SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*,DWORD*);
static DEP_SYSTEM_POLICY_TYPE (WINAPI * pGetSystemDEPPolicy)(void);
static NTSTATUS (WINAPI * pNtOpenThread)(HANDLE *, ACCESS_MASK, const OBJECT_ATTRIBUTES *, const CLIENT_ID *);
static NTSTATUS (WINAPI * pNtQueryObject)(HANDLE, OBJECT_INFORMATION_CLASS, void *, ULONG, ULONG *);
static NTSTATUS (WINAPI * pNtCreateDebugObject)( HANDLE *, ACCESS_MASK, OBJECT_ATTRIBUTES *, ULONG );
static NTSTATUS (WINAPI * pNtSetInformationDebugObject)(HANDLE,DEBUGOBJECTINFOCLASS,PVOID,ULONG,ULONG*);
static NTSTATUS (WINAPI * pDbgUiConvertStateChangeStructure)(DBGUI_WAIT_STATE_CHANGE*,DEBUG_EVENT*);
static HANDLE   (WINAPI * pDbgUiGetThreadDebugObject)(void);
static void     (WINAPI * pDbgUiSetThreadDebugObject)(HANDLE);
static NTSTATUS (WINAPI * pNtSystemDebugControl)(SYSDBG_COMMAND,PVOID,ULONG,PVOID,ULONG,PULONG);

static BOOL is_wow64;
static BOOL old_wow64;

/* one_before_last_pid is used to be able to compare values of a still running process
   with the output of the test_query_process_times and test_query_process_handlecount tests.
*/
static DWORD one_before_last_pid = 0;

static inline DWORD_PTR get_affinity_mask(DWORD num_cpus)
{
    if (num_cpus >= sizeof(DWORD_PTR) * 8) return ~(DWORD_PTR)0;
    return ((DWORD_PTR)1 << num_cpus) - 1;
}

#define NTDLL_GET_PROC(func) do {                     \
    p ## func = (void*)GetProcAddress(hntdll, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
    } \
  } while(0)

/* Firmware table providers */
#define ACPI 0x41435049
#define FIRM 0x4649524D
#define RSMB 0x52534D42

static void InitFunctionPtrs(void)
{
    /* All needed functions are NT based, so using GetModuleHandle is a good check */
    HMODULE hntdll = GetModuleHandleA("ntdll");
    HMODULE hkernel32 = GetModuleHandleA("kernel32");

    NTDLL_GET_PROC(NtQuerySystemInformation);
    NTDLL_GET_PROC(NtQuerySystemInformationEx);
    NTDLL_GET_PROC(NtSetSystemInformation);
    NTDLL_GET_PROC(RtlGetNativeSystemInformation);
    NTDLL_GET_PROC(NtPowerInformation);
    NTDLL_GET_PROC(NtQueryInformationThread);
    NTDLL_GET_PROC(NtSetInformationProcess);
    NTDLL_GET_PROC(NtSetInformationThread);
    NTDLL_GET_PROC(NtReadVirtualMemory);
    NTDLL_GET_PROC(NtQueryVirtualMemory);
    NTDLL_GET_PROC(NtClose);
    NTDLL_GET_PROC(NtCreateSection);
    NTDLL_GET_PROC(NtMapViewOfSection);
    NTDLL_GET_PROC(NtUnmapViewOfSection);
    NTDLL_GET_PROC(NtOpenThread);
    NTDLL_GET_PROC(NtQueryObject);
    NTDLL_GET_PROC(NtCreateDebugObject);
    NTDLL_GET_PROC(NtSetInformationDebugObject);
    NTDLL_GET_PROC(NtGetCurrentProcessorNumber);
    NTDLL_GET_PROC(DbgUiConvertStateChangeStructure);
    NTDLL_GET_PROC(DbgUiGetThreadDebugObject);
    NTDLL_GET_PROC(DbgUiSetThreadDebugObject);
    NTDLL_GET_PROC(NtSystemDebugControl);

    if (!IsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

    if (is_wow64)
    {
        TEB64 *teb64 = ULongToPtr( NtCurrentTeb()->GdiBatchCount );

        if (teb64)
        {
            PEB64 *peb64 = ULongToPtr(teb64->Peb);
            old_wow64 = !peb64->LdrData;
        }
    }

    pGetSystemDEPPolicy = (void *)GetProcAddress(hkernel32, "GetSystemDEPPolicy");
    pGetLogicalProcessorInformationEx = (void *) GetProcAddress(hkernel32, "GetLogicalProcessorInformationEx");
}

static void test_query_basic(void)
{
    NTSTATUS status;
    ULONG i, ReturnLength;
    SYSTEM_BASIC_INFORMATION sbi, sbi2, sbi3;

    /* This test also covers some basic parameter testing that should be the same for 
     * every information class
    */

    /* Use a nonexistent info class */
    status = pNtQuerySystemInformation(-1, NULL, 0, NULL);
    ok( status == STATUS_INVALID_INFO_CLASS || status == STATUS_NOT_IMPLEMENTED /* vista */,
        "Expected STATUS_INVALID_INFO_CLASS or STATUS_NOT_IMPLEMENTED, got %08lx\n", status);

    /* Use an existing class but with a zero-length buffer */
    status = pNtQuerySystemInformation(SystemBasicInformation, NULL, 0, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Use an existing class, correct length but no SystemInformation buffer */
    status = pNtQuerySystemInformation(SystemBasicInformation, NULL, sizeof(sbi), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER /* vista */,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER, got %08lx\n", status);

    /* Use an existing class, correct length, a pointer to a buffer but no ReturnLength pointer */
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Check a too large buffer size */
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Finally some correct calls */
    memset(&sbi, 0xcc, sizeof(sbi));
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(sbi) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);

    /* Check if we have some return values */
    if (winetest_debug > 1) trace("Number of Processors : %d\n", sbi.NumberOfProcessors);
    ok( sbi.NumberOfProcessors > 0, "Expected more than 0 processors, got %d\n", sbi.NumberOfProcessors);

    memset(&sbi2, 0xcc, sizeof(sbi2));
    status = pRtlGetNativeSystemInformation(SystemBasicInformation, &sbi2, sizeof(sbi2), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx.\n", status);
    ok( sizeof(sbi2) == ReturnLength, "Unexpected length %lu.\n", ReturnLength);

    ok( sbi.unknown == sbi2.unknown, "Expected unknown %#lx, got %#lx.\n", sbi.unknown, sbi2.unknown);
    ok( sbi.KeMaximumIncrement == sbi2.KeMaximumIncrement, "Expected KeMaximumIncrement %lu, got %lu.\n",
            sbi.KeMaximumIncrement, sbi2.KeMaximumIncrement);
    ok( sbi.PageSize == sbi2.PageSize, "Expected PageSize field %lu, %lu.\n", sbi.PageSize, sbi2.PageSize);
    ok( sbi.MmNumberOfPhysicalPages == sbi2.MmNumberOfPhysicalPages,
            "Expected MmNumberOfPhysicalPages %lu, got %lu.\n",
            sbi.MmNumberOfPhysicalPages, sbi2.MmNumberOfPhysicalPages);
    ok( sbi.MmLowestPhysicalPage == sbi2.MmLowestPhysicalPage, "Expected MmLowestPhysicalPage %lu, got %lu.\n",
            sbi.MmLowestPhysicalPage, sbi2.MmLowestPhysicalPage);
    ok( sbi.MmHighestPhysicalPage == sbi2.MmHighestPhysicalPage, "Expected MmHighestPhysicalPage %lu, got %lu.\n",
            sbi.MmHighestPhysicalPage, sbi2.MmHighestPhysicalPage);
    /* Higher 32 bits of AllocationGranularity is sometimes garbage on Windows. */
    ok( (ULONG)sbi.AllocationGranularity == (ULONG)sbi2.AllocationGranularity,
            "Expected AllocationGranularity %#Ix, got %#Ix.\n",
            sbi.AllocationGranularity, sbi2.AllocationGranularity);
    ok( sbi.LowestUserAddress == sbi2.LowestUserAddress, "Expected LowestUserAddress %p, got %p.\n",
            sbi.LowestUserAddress, sbi2.LowestUserAddress);
    ok( sbi.ActiveProcessorsAffinityMask == sbi2.ActiveProcessorsAffinityMask,
            "Expected ActiveProcessorsAffinityMask %#Ix, got %#Ix.\n",
            sbi.ActiveProcessorsAffinityMask, sbi2.ActiveProcessorsAffinityMask);
    ok( sbi.NumberOfProcessors == sbi2.NumberOfProcessors, "Expected NumberOfProcessors %u, got %u.\n",
            sbi.NumberOfProcessors, sbi2.NumberOfProcessors);
#ifdef _WIN64
    ok( sbi.HighestUserAddress == sbi2.HighestUserAddress, "Expected HighestUserAddress %p, got %p.\n",
            (void *)sbi.HighestUserAddress, (void *)sbi2.HighestUserAddress);
#else
    ok( sbi.HighestUserAddress == (void *)0x7ffeffff, "wrong limit %p\n", sbi.HighestUserAddress);
    todo_wine_if( old_wow64 )
    ok( sbi2.HighestUserAddress == (is_wow64 ? (void *)0xfffeffff : (void *)0x7ffeffff),
        "wrong limit %p\n", sbi.HighestUserAddress);
#endif

    memset(&sbi3, 0xcc, sizeof(sbi3));
    status = pNtQuerySystemInformation(SystemNativeBasicInformation, &sbi3, sizeof(sbi3), &ReturnLength);
#ifdef _WIN64
    ok( status == STATUS_SUCCESS || broken(status == STATUS_INVALID_INFO_CLASS), "got %08lx\n", status);
    if (!status)
    {
        ok( sizeof(sbi3) == ReturnLength, "Unexpected length %lu.\n", ReturnLength);
        ok( !memcmp( &sbi2, &sbi3, offsetof(SYSTEM_BASIC_INFORMATION,NumberOfProcessors)+1 ),
            "info is different\n" );
    }
#else
    ok( status == STATUS_INVALID_INFO_CLASS || broken(status == STATUS_SUCCESS), /* some Win8 */
        "got %08lx\n", status);
    status = pRtlGetNativeSystemInformation( SystemNativeBasicInformation, &sbi3, sizeof(sbi3), &ReturnLength );
    ok( !status || status == STATUS_INFO_LENGTH_MISMATCH ||
        broken(status == STATUS_INVALID_INFO_CLASS) || broken(status == STATUS_NOT_IMPLEMENTED),
        "failed %lx\n", status );
    if (!status || status == STATUS_INFO_LENGTH_MISMATCH)
        todo_wine_if( old_wow64 )
        ok( !status == !is_wow64, "got wrong status %lx wow64 %u\n", status, is_wow64 );
    if (!status)
    {
        ok( sizeof(sbi3) == ReturnLength, "Unexpected length %lu.\n", ReturnLength);
        ok( !memcmp( &sbi2, &sbi3, offsetof(SYSTEM_BASIC_INFORMATION,NumberOfProcessors)+1 ),
            "info is different\n" );
    }
    else if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        /* SystemNativeBasicInformation uses the 64-bit structure on Wow64 */
        struct
        {
            DWORD     unknown;
            ULONG     KeMaximumIncrement;
            ULONG     PageSize;
            ULONG     MmNumberOfPhysicalPages;
            ULONG     MmLowestPhysicalPage;
            ULONG     MmHighestPhysicalPage;
            ULONG64   AllocationGranularity;
            ULONG64   LowestUserAddress;
            ULONG64   HighestUserAddress;
            ULONG64   ActiveProcessorsAffinityMask;
            BYTE      NumberOfProcessors;
        } sbi64;

        ok( ReturnLength == sizeof(sbi64), "len %lx\n", ReturnLength );
        memset( &sbi64, 0xcc, sizeof(sbi64) );
        ReturnLength = 0;
        status = pRtlGetNativeSystemInformation( SystemNativeBasicInformation, &sbi64, sizeof(sbi64), &ReturnLength );
        ok( !status, "failed %lx\n", status );
        ok( ReturnLength == sizeof(sbi64), "len %lx\n", ReturnLength );

        ok( sbi.unknown == sbi64.unknown, "unknown %#lx / %#lx\n", sbi.unknown, sbi64.unknown);
        ok( sbi.KeMaximumIncrement == sbi64.KeMaximumIncrement, "KeMaximumIncrement %lu / %lu\n",
            sbi.KeMaximumIncrement, sbi64.KeMaximumIncrement);
        ok( sbi.PageSize == sbi64.PageSize, "PageSize %lu / %lu\n", sbi.PageSize, sbi64.PageSize);
        ok( sbi.MmNumberOfPhysicalPages == sbi64.MmNumberOfPhysicalPages,
            "MmNumberOfPhysicalPages %lu / %lu\n",
            sbi.MmNumberOfPhysicalPages, sbi64.MmNumberOfPhysicalPages);
        ok( sbi.MmLowestPhysicalPage == sbi64.MmLowestPhysicalPage, "MmLowestPhysicalPage %lu / %lu\n",
            sbi.MmLowestPhysicalPage, sbi64.MmLowestPhysicalPage);
        ok( sbi.MmHighestPhysicalPage == sbi64.MmHighestPhysicalPage, "MmHighestPhysicalPage %lu / %lu\n",
            sbi.MmHighestPhysicalPage, sbi64.MmHighestPhysicalPage);
        ok( sbi.AllocationGranularity == (ULONG_PTR)sbi64.AllocationGranularity,
            "AllocationGranularity %#Ix / %#Ix\n", sbi.AllocationGranularity,
            (ULONG_PTR)sbi64.AllocationGranularity);
        ok( (ULONG_PTR)sbi.LowestUserAddress == sbi64.LowestUserAddress, "LowestUserAddress %p / %s\n",
            sbi.LowestUserAddress, wine_dbgstr_longlong(sbi64.LowestUserAddress));
        ok( sbi.ActiveProcessorsAffinityMask == sbi64.ActiveProcessorsAffinityMask,
            "ActiveProcessorsAffinityMask %#Ix / %s\n",
            sbi.ActiveProcessorsAffinityMask, wine_dbgstr_longlong(sbi64.ActiveProcessorsAffinityMask));
        ok( sbi.NumberOfProcessors == sbi64.NumberOfProcessors, "NumberOfProcessors %u / %u\n",
            sbi.NumberOfProcessors, sbi64.NumberOfProcessors);
        ok( sbi64.HighestUserAddress == 0x7ffffffeffff, "wrong limit %s\n",
            wine_dbgstr_longlong(sbi64.HighestUserAddress));
    }
#endif

    memset(&sbi3, 0xcc, sizeof(sbi3));
    status = pNtQuerySystemInformation(SystemEmulationBasicInformation, &sbi3, sizeof(sbi3), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx.\n", status);
    ok( sizeof(sbi3) == ReturnLength, "Unexpected length %lu.\n", ReturnLength);
    ok( !memcmp( &sbi, &sbi3, offsetof(SYSTEM_BASIC_INFORMATION,NumberOfProcessors)+1 ),
        "info is different\n" );

    for (i = 0; i < 256; i++)
    {
        NTSTATUS expect = pNtQuerySystemInformation( i, NULL, 0, &ReturnLength );
        status = pRtlGetNativeSystemInformation( i, NULL, 0, &ReturnLength );
        switch (i)
        {
        case SystemNativeBasicInformation:
            ok( status == STATUS_INVALID_INFO_CLASS || status == STATUS_INFO_LENGTH_MISMATCH ||
                broken(status == STATUS_NOT_IMPLEMENTED) /* vista */, "%lu: %lx / %lx\n", i, status, expect );
            break;
        case SystemBasicInformation:
        case SystemCpuInformation:
        case SystemEmulationBasicInformation:
        case SystemEmulationProcessorInformation:
            ok( status == expect, "%lu: %lx / %lx\n", i, status, expect );
            break;
        default:
            if (is_wow64)  /* only a few info classes are supported on Wow64 */
                todo_wine_if (is_wow64 && status != STATUS_INVALID_INFO_CLASS)
                ok( status == STATUS_INVALID_INFO_CLASS ||
                    broken(status == STATUS_NOT_IMPLEMENTED), /* vista */
                    "%lu: %lx\n", i, status );
            else
                ok( status == expect, "%lu: %lx / %lx\n", i, status, expect );
            break;
        }
    }
}

static void test_query_cpu(void)
{
    NTSTATUS status;
    ULONG len, buffer[16];
    SYSTEM_PROCESSOR_FEATURES_INFORMATION features;
    SYSTEM_CPU_INFORMATION sci, sci2, sci3;

    memset(&sci, 0xcc, sizeof(sci));
    status = pNtQuerySystemInformation(SystemCpuInformation, &sci, sizeof(sci), &len);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(sci) == len, "Inconsistent length %ld\n", len);

    memset(&sci2, 0xcc, sizeof(sci2));
    status = pRtlGetNativeSystemInformation(SystemCpuInformation, &sci2, sizeof(sci2), &len);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx.\n", status);
    ok( sizeof(sci2) == len, "Unexpected length %lu.\n", len);

    if (is_wow64)
    {
        ok( sci.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL, "ProcessorArchitecture wrong %x\n",
            sci.ProcessorArchitecture );
        ok( sci2.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
            sci2.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64,
            "ProcessorArchitecture wrong %x\n", sci2.ProcessorArchitecture );
    }
    else
        ok( sci.ProcessorArchitecture == sci2.ProcessorArchitecture,
            "ProcessorArchitecture differs %x / %x\n",
            sci.ProcessorArchitecture, sci2.ProcessorArchitecture );

    if (sci2.ProcessorArchitecture != PROCESSOR_ARCHITECTURE_ARM64)
    {
        /* Check if we have some return values */
        if (winetest_debug > 1) trace("Processor FeatureSet : %08lx\n", sci.ProcessorFeatureBits);
        ok( sci.ProcessorFeatureBits != 0, "Expected some features for this processor, got %08lx\n",
            sci.ProcessorFeatureBits);
    }
    ok( sci.ProcessorLevel == sci2.ProcessorLevel, "ProcessorLevel differs %x / %x\n",
        sci.ProcessorLevel, sci2.ProcessorLevel );
    ok( sci.ProcessorRevision == sci2.ProcessorRevision, "ProcessorRevision differs %x / %x\n",
        sci.ProcessorRevision, sci2.ProcessorRevision );
    ok( sci.MaximumProcessors == sci2.MaximumProcessors, "MaximumProcessors differs %x / %x\n",
        sci.MaximumProcessors, sci2.MaximumProcessors );
    ok( sci.ProcessorFeatureBits == sci2.ProcessorFeatureBits, "ProcessorFeatureBits differs %lx / %lx\n",
        sci.ProcessorFeatureBits, sci2.ProcessorFeatureBits );

    memset(&sci3, 0xcc, sizeof(sci3));
    status = pNtQuerySystemInformation(SystemEmulationProcessorInformation, &sci3, sizeof(sci3), &len);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx.\n", status);
    ok( sizeof(sci3) == len, "Unexpected length %lu.\n", len);

#ifdef _WIN64
    if (sci2.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
        ok( sci3.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM, "ProcessorArchitecture wrong %x\n",
            sci3.ProcessorArchitecture );
    else
        ok( sci3.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL, "ProcessorArchitecture wrong %x\n",
            sci3.ProcessorArchitecture );
#else
    ok( sci.ProcessorArchitecture == sci3.ProcessorArchitecture,
        "ProcessorArchitecture differs %x / %x\n",
        sci.ProcessorArchitecture, sci3.ProcessorArchitecture );
#endif
    ok( sci.ProcessorLevel == sci3.ProcessorLevel, "ProcessorLevel differs %x / %x\n",
        sci.ProcessorLevel, sci3.ProcessorLevel );
    ok( sci.ProcessorRevision == sci3.ProcessorRevision, "ProcessorRevision differs %x / %x\n",
        sci.ProcessorRevision, sci3.ProcessorRevision );
    ok( sci.MaximumProcessors == sci3.MaximumProcessors, "MaximumProcessors differs %x / %x\n",
        sci.MaximumProcessors, sci3.MaximumProcessors );
    ok( sci.ProcessorFeatureBits == sci3.ProcessorFeatureBits, "ProcessorFeatureBits differs %lx / %lx\n",
        sci.ProcessorFeatureBits, sci3.ProcessorFeatureBits );

    len = 0xdeadbeef;
    status = pNtQuerySystemInformation( SystemProcessorFeaturesInformation, &features, sizeof(features), &len );
    if (status != STATUS_NOT_SUPPORTED && status != STATUS_INVALID_INFO_CLASS)
    {
        ok( !status, "SystemProcessorFeaturesInformation failed %lx\n", status );
        ok( len == sizeof(features), "wrong len %lu\n", len );
        ok( (ULONG)features.ProcessorFeatureBits == sci.ProcessorFeatureBits, "wrong bits %I64x / %lx\n",
            features.ProcessorFeatureBits, sci.ProcessorFeatureBits );
    }
    else skip( "SystemProcessorFeaturesInformation is not supported\n" );

    len = 0xdeadbeef;
    status = pNtQuerySystemInformation( SystemProcessorBrandString, buffer, sizeof(buffer), &len );
#ifdef __REACTOS__
    if (GetNTVersion() >= _WIN32_WINNT_VISTA)
#else
    if (status != STATUS_NOT_SUPPORTED)
#endif
    {
        ok( !status, "SystemProcessorBrandString failed %lx\n", status );
        ok( len == 49, "wrong len %lu\n", len );
        trace( "got %s len %u\n", debugstr_a( (char *)buffer ), lstrlenA( (char *)buffer ));

        len = 0xdeadbeef;
        status = pNtQuerySystemInformation( SystemProcessorBrandString, buffer, 49, &len );
        ok( !status, "SystemProcessorBrandString failed %lx\n", status );
        ok( len == 49, "wrong len %lu\n", len );

        len = 0xdeadbeef;
        status = pNtQuerySystemInformation( SystemProcessorBrandString, buffer, 48, &len );
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "SystemProcessorBrandString failed %lx\n", status );
        ok( len == 49, "wrong len %lu\n", len );

        len = 0xdeadbeef;
        status = pNtQuerySystemInformation( SystemProcessorBrandString, (char *)buffer + 1, 49, &len );
        ok( status == STATUS_DATATYPE_MISALIGNMENT, "SystemProcessorBrandString failed %lx\n", status );
        ok( len == 0xdeadbeef, "wrong len %lu\n", len );
    }
    else skip( "SystemProcessorBrandString is not supported\n" );
}

static void test_query_performance(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    ULONGLONG buffer[sizeof(SYSTEM_PERFORMANCE_INFORMATION)/sizeof(ULONGLONG) + 5];
    DWORD size = sizeof(SYSTEM_PERFORMANCE_INFORMATION);

    status = pNtQuerySystemInformation(SystemPerformanceInformation, buffer, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = pNtQuerySystemInformation(SystemPerformanceInformation, buffer, size, &ReturnLength);
    if (status == STATUS_INFO_LENGTH_MISMATCH && is_wow64)
    {
        /* size is larger on wow64 under w2k8/win7 */
        size += 16;
        status = pNtQuerySystemInformation(SystemPerformanceInformation, buffer, size, &ReturnLength);
    }
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( ReturnLength == size, "Inconsistent length %ld\n", ReturnLength);

    status = pNtQuerySystemInformation(SystemPerformanceInformation, buffer, size + 2, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( ReturnLength == size || ReturnLength == size + 2 /* win8+ */,
        "Inconsistent length %ld\n", ReturnLength);

    /* Not return values yet, as struct members are unknown */
}

static void test_query_timeofday(void)
{
    NTSTATUS status;
    ULONG ReturnLength;

    /* Copy of our winternl.h structure turned into a private one */
    typedef struct _SYSTEM_TIMEOFDAY_INFORMATION_PRIVATE {
        LARGE_INTEGER liKeBootTime;
        LARGE_INTEGER liKeSystemTime;
        LARGE_INTEGER liExpTimeZoneBias;
        ULONG uCurrentTimeZoneId;
        DWORD dwUnknown1[5];
    } SYSTEM_TIMEOFDAY_INFORMATION_PRIVATE;

    SYSTEM_TIMEOFDAY_INFORMATION_PRIVATE sti;
  
    status = pNtQuerySystemInformation( SystemTimeOfDayInformation, &sti, 0, &ReturnLength );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( 0 == ReturnLength, "ReturnLength should be 0, it is (%ld)\n", ReturnLength);

    sti.uCurrentTimeZoneId = 0xdeadbeef;
    status = pNtQuerySystemInformation( SystemTimeOfDayInformation, &sti, 24, &ReturnLength );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( 24 == ReturnLength, "ReturnLength should be 24, it is (%ld)\n", ReturnLength);
    ok( 0xdeadbeef == sti.uCurrentTimeZoneId, "This part of the buffer should not have been filled\n");

    sti.uCurrentTimeZoneId = 0xdeadbeef;
    status = pNtQuerySystemInformation( SystemTimeOfDayInformation, &sti, 32, &ReturnLength );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( 32 == ReturnLength, "ReturnLength should be 32, it is (%ld)\n", ReturnLength);
    ok( 0xdeadbeef != sti.uCurrentTimeZoneId, "Buffer should have been partially filled\n");

    status = pNtQuerySystemInformation( SystemTimeOfDayInformation, &sti, 49, &ReturnLength );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( ReturnLength == 0 || ReturnLength == sizeof(sti) /* vista */,
        "ReturnLength should be 0, it is (%ld)\n", ReturnLength);

    status = pNtQuerySystemInformation( SystemTimeOfDayInformation, &sti, sizeof(sti), &ReturnLength );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(sti) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);

    /* Check if we have some return values */
    if (winetest_debug > 1) trace("uCurrentTimeZoneId : (%ld)\n", sti.uCurrentTimeZoneId);
}

static void test_query_process( BOOL extended )
{
    NTSTATUS status;
    DWORD last_pid;
    ULONG ReturnLength;
    int i = 0, k = 0;
    PROCESS_BASIC_INFORMATION pbi;
    THREAD_BASIC_INFORMATION tbi;
    OBJECT_ATTRIBUTES attr;
    CLIENT_ID cid;
    HANDLE handle;

    /* Copy of our winternl.h structure turned into a private one */
    typedef struct _SYSTEM_PROCESS_INFORMATION_PRIVATE {
        ULONG NextEntryOffset;
        DWORD dwThreadCount;
        LARGE_INTEGER WorkingSetPrivateSize;
        ULONG HardFaultCount;
        ULONG NumberOfThreadsHighWatermark;
        ULONGLONG CycleTime;
        FILETIME ftCreationTime;
        FILETIME ftUserTime;
        FILETIME ftKernelTime;
        UNICODE_STRING ProcessName;
        DWORD dwBasePriority;
        HANDLE UniqueProcessId;
        HANDLE ParentProcessId;
        ULONG HandleCount;
        ULONG SessionId;
        ULONG_PTR UniqueProcessKey;
        VM_COUNTERS_EX vmCounters;
        IO_COUNTERS ioCounters;
        SYSTEM_THREAD_INFORMATION ti[1];
    } SYSTEM_PROCESS_INFORMATION_PRIVATE;

    BOOL is_process_wow64 = FALSE, current_process_found = FALSE;
    SYSTEM_PROCESS_INFORMATION_PRIVATE *spi, *spi_buf;
    SYSTEM_EXTENDED_THREAD_INFORMATION *ti;
    SYSTEM_INFORMATION_CLASS info_class;
    void *expected_address;
    ULONG thread_info_size;

    if (extended)
    {
        info_class = SystemExtendedProcessInformation;
        thread_info_size = sizeof(SYSTEM_EXTENDED_THREAD_INFORMATION);
    }
    else
    {
        info_class = SystemProcessInformation;
        thread_info_size = sizeof(SYSTEM_THREAD_INFORMATION);
    }

    /* test ReturnLength */
    ReturnLength = 0;
    status = pNtQuerySystemInformation( info_class, NULL, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH got %08lx\n", status);
    ok( ReturnLength > 0, "got 0 length\n" );

    /* W2K3 and later returns the needed length, the rest returns 0. */
    if (!ReturnLength)
    {
        win_skip( "Zero return length, skipping tests." );
        return;
    }

    winetest_push_context( "extended %d", extended );

    spi_buf = HeapAlloc(GetProcessHeap(), 0, ReturnLength);
    status = pNtQuerySystemInformation(info_class, spi_buf, ReturnLength, &ReturnLength);

    /* Sometimes new process or threads appear between the call and increase the size,
     * otherwise the previously returned buffer size should be sufficient. */
    ok( status == STATUS_SUCCESS || status == STATUS_INFO_LENGTH_MISMATCH,
        "Expected STATUS_SUCCESS, got %08lx\n", status );

    spi = spi_buf;

    for (;;)
    {
        DWORD_PTR tid;
        DWORD j;

        winetest_push_context( "i %u (%s)", i, debugstr_w(spi->ProcessName.Buffer) );

        i++;

        last_pid = (DWORD_PTR)spi->UniqueProcessId;
        ok( !(last_pid & 3), "Unexpected PID low bits: %p\n", spi->UniqueProcessId );

        if (last_pid == GetCurrentProcessId())
            current_process_found = TRUE;

        if (extended && is_wow64 && spi->UniqueProcessId)
        {
            InitializeObjectAttributes( &attr, NULL, 0, NULL, NULL );
            cid.UniqueProcess = spi->UniqueProcessId;
            cid.UniqueThread = 0;
            status = NtOpenProcess( &handle, PROCESS_QUERY_LIMITED_INFORMATION, &attr, &cid );
            ok( status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
                "Got unexpected status %#lx, pid %p.\n", status, spi->UniqueProcessId );

            if (!status)
            {
                ULONG_PTR info;

                status = NtQueryInformationProcess( handle, ProcessWow64Information, &info, sizeof(info), NULL );
                ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
                is_process_wow64 = !!info;
                NtClose( handle );
            }
        }

        for (j = 0; j < spi->dwThreadCount; j++)
        {
            ti = (SYSTEM_EXTENDED_THREAD_INFORMATION *)((BYTE *)spi->ti + j * thread_info_size);

            k++;
            ok ( ti->ThreadInfo.ClientId.UniqueProcess == spi->UniqueProcessId,
                 "The owning pid of the thread (%p) doesn't equal the pid (%p) of the process\n",
                 ti->ThreadInfo.ClientId.UniqueProcess, spi->UniqueProcessId );

            tid = (DWORD_PTR)ti->ThreadInfo.ClientId.UniqueThread;
            ok( !(tid & 3), "Unexpected TID low bits: %p\n", ti->ThreadInfo.ClientId.UniqueThread );

            if (extended)
            {
                todo_wine ok( !!ti->StackBase, "Got NULL StackBase.\n" );
                todo_wine ok( !!ti->StackLimit, "Got NULL StackLimit.\n" );
#ifdef __REACTOS__
                if ((GetNTVersion() >= _WIN32_WINNT_VISTA) && !is_reactos()) // Broken on Win 2003
#endif
                ok( !!ti->Win32StartAddress, "Got NULL Win32StartAddress.\n" );

                cid.UniqueProcess = 0;
                cid.UniqueThread = ti->ThreadInfo.ClientId.UniqueThread;

                InitializeObjectAttributes( &attr, NULL, 0, NULL, NULL );
                status = NtOpenThread( &handle, THREAD_QUERY_INFORMATION, &attr, &cid );
                if (!status)
                {
                    THREAD_BASIC_INFORMATION tbi;

                    status = pNtQueryInformationThread( handle, ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
                    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
                    expected_address = tbi.TebBaseAddress;
                    if (is_wow64 && is_process_wow64)
                        expected_address = (BYTE *)expected_address - 0x2000;
#ifdef __REACTOS__
                    if ((GetNTVersion() < _WIN32_WINNT_VISTA) && !is_reactos()) // Broken on Win 2003
                        expected_address = NULL;
#endif
                    if (!is_wow64 && !is_process_wow64 && !tbi.TebBaseAddress)
                        win_skip( "Could not get TebBaseAddress, thread %lu.\n", j );
                    else
                        ok( ti->TebBase == expected_address || (is_wow64 && !expected_address && !!ti->TebBase),
                            "Got unexpected TebBase %p, expected %p.\n", ti->TebBase, expected_address );

                    NtClose( handle );
                }
            }
        }

        if (!spi->NextEntryOffset)
        {
            winetest_pop_context();
            break;
        }
        one_before_last_pid = last_pid;

        spi = (SYSTEM_PROCESS_INFORMATION_PRIVATE*)((char*)spi + spi->NextEntryOffset);
        winetest_pop_context();
    }
    ok( current_process_found, "Test process not found.\n" );
    if (winetest_debug > 1) trace("%u processes, %u threads\n", i, k);

    if (one_before_last_pid == 0) one_before_last_pid = last_pid;

    HeapFree( GetProcessHeap(), 0, spi_buf);

#ifdef __REACTOS__
    if (GetNTVersion() < _WIN32_WINNT_VISTA)
    {
        win_skip("Skipping ClientId tests on pre-NT6.\n");
    }
    else
    {
#endif
    for (i = 1; i < 4; ++i)
    {
        InitializeObjectAttributes( &attr, NULL, 0, NULL, NULL );
        cid.UniqueProcess = ULongToHandle(GetCurrentProcessId() + i);
        cid.UniqueThread = 0;

        status = NtOpenProcess( &handle, PROCESS_QUERY_LIMITED_INFORMATION, &attr, &cid );
        ok( status == STATUS_SUCCESS || broken( status == STATUS_ACCESS_DENIED ) /* wxppro */,
            "NtOpenProcess returned:%lx\n", status );
        if (status != STATUS_SUCCESS) continue;

        status = NtQueryInformationProcess( handle, ProcessBasicInformation, &pbi, sizeof(pbi), NULL );
        ok( status == STATUS_SUCCESS, "NtQueryInformationProcess returned:%lx\n", status );
        ok( pbi.UniqueProcessId == GetCurrentProcessId(),
            "Expected pid %p, got %p\n", ULongToHandle(GetCurrentProcessId()), ULongToHandle(pbi.UniqueProcessId) );

        NtClose( handle );
    }

    for (i = 1; i < 4; ++i)
    {
        InitializeObjectAttributes( &attr, NULL, 0, NULL, NULL );
        cid.UniqueProcess = 0;
        cid.UniqueThread = ULongToHandle(GetCurrentThreadId() + i);

        status = NtOpenThread( &handle, THREAD_QUERY_LIMITED_INFORMATION, &attr, &cid );
        ok( status == STATUS_SUCCESS || broken( status == STATUS_ACCESS_DENIED ) /* wxppro */,
            "NtOpenThread returned:%lx\n", status );
        if (status != STATUS_SUCCESS) continue;

        status = pNtQueryInformationThread( handle, ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
        ok( status == STATUS_SUCCESS, "NtQueryInformationThread returned:%lx\n", status );
        ok( tbi.ClientId.UniqueThread == ULongToHandle(GetCurrentThreadId()),
            "Expected tid %p, got %p\n", ULongToHandle(GetCurrentThreadId()), tbi.ClientId.UniqueThread );

        NtClose( handle );
    }
#ifdef __REACTOS__
    }
#endif
    winetest_pop_context();
}

static void test_query_procperf(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    ULONG NeededLength;
    SYSTEM_BASIC_INFORMATION sbi;
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* sppi;

    /* Find out the number of processors */
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    NeededLength = sbi.NumberOfProcessors * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

    sppi = HeapAlloc(GetProcessHeap(), 0, NeededLength);

    status = pNtQuerySystemInformation(SystemProcessorPerformanceInformation, sppi, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Try it for 1 processor */
    sppi->KernelTime.QuadPart = 0xdeaddead;
    sppi->UserTime.QuadPart = 0xdeaddead;
    sppi->IdleTime.QuadPart = 0xdeaddead;
    status = pNtQuerySystemInformation(SystemProcessorPerformanceInformation, sppi,
                                       sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) == ReturnLength,
        "Inconsistent length %ld\n", ReturnLength);
    ok (sppi->KernelTime.QuadPart != 0xdeaddead, "KernelTime unchanged\n");
    ok (sppi->UserTime.QuadPart != 0xdeaddead, "UserTime unchanged\n");
    ok (sppi->IdleTime.QuadPart != 0xdeaddead, "IdleTime unchanged\n");

    /* Try it for all processors */
    sppi->KernelTime.QuadPart = 0xdeaddead;
    sppi->UserTime.QuadPart = 0xdeaddead;
    sppi->IdleTime.QuadPart = 0xdeaddead;
    status = pNtQuerySystemInformation(SystemProcessorPerformanceInformation, sppi, NeededLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( NeededLength == ReturnLength, "Inconsistent length (%ld) <-> (%ld)\n", NeededLength, ReturnLength);
    ok (sppi->KernelTime.QuadPart != 0xdeaddead, "KernelTime unchanged\n");
    ok (sppi->UserTime.QuadPart != 0xdeaddead, "UserTime unchanged\n");
    ok (sppi->IdleTime.QuadPart != 0xdeaddead, "IdleTime unchanged\n");

    /* A too large given buffer size */
    sppi = HeapReAlloc(GetProcessHeap(), 0, sppi , NeededLength + 2);
    sppi->KernelTime.QuadPart = 0xdeaddead;
    sppi->UserTime.QuadPart = 0xdeaddead;
    sppi->IdleTime.QuadPart = 0xdeaddead;
    status = pNtQuerySystemInformation(SystemProcessorPerformanceInformation, sppi, NeededLength + 2, &ReturnLength);
    ok( status == STATUS_SUCCESS || status == STATUS_INFO_LENGTH_MISMATCH /* vista */,
        "Expected STATUS_SUCCESS or STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( NeededLength == ReturnLength, "Inconsistent length (%ld) <-> (%ld)\n", NeededLength, ReturnLength);
    if (status == STATUS_SUCCESS)
    {
        ok (sppi->KernelTime.QuadPart != 0xdeaddead, "KernelTime unchanged\n");
        ok (sppi->UserTime.QuadPart != 0xdeaddead, "UserTime unchanged\n");
        ok (sppi->IdleTime.QuadPart != 0xdeaddead, "IdleTime unchanged\n");
    }
    else /* vista and 2008 */
    {
        ok (sppi->KernelTime.QuadPart == 0xdeaddead, "KernelTime changed\n");
        ok (sppi->UserTime.QuadPart == 0xdeaddead, "UserTime changed\n");
        ok (sppi->IdleTime.QuadPart == 0xdeaddead, "IdleTime changed\n");
    }

    HeapFree( GetProcessHeap(), 0, sppi);
}

static void test_query_module(void)
{
    const RTL_PROCESS_MODULE_INFORMATION_EX *infoex;
    RTL_PROCESS_MODULES *info;
    NTSTATUS status;
    ULONG size, i;
    char *buffer;

    status = pNtQuerySystemInformation(SystemModuleInformation, NULL, 0, &size);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got %#lx\n", status);
    ok(size > 0, "expected nonzero size\n");

    info = malloc(size);
    status = pNtQuerySystemInformation(SystemModuleInformation, info, size, &size);
    ok(!status, "got %#lx\n", status);

    ok(info->ModulesCount > 0, "Expected some modules to be loaded\n");

    for (i = 0; i < info->ModulesCount; i++)
    {
        RTL_PROCESS_MODULE_INFORMATION *module = &info->Modules[i];

        ok(module->LoadOrderIndex == i, "%lu: got index %u\n", i, module->LoadOrderIndex);
        ok(module->ImageBaseAddress || is_wow64, "%lu: got NULL address for %s\n", i, module->Name);
        ok(module->ImageSize, "%lu: got 0 size\n", i);
        ok(module->LoadCount, "%lu: got 0 load count\n", i);
    }

    free(info);

    status = pNtQuerySystemInformation(SystemModuleInformationEx, NULL, 0, &size);
    if (status == STATUS_INVALID_INFO_CLASS)
    {
        win_skip("SystemModuleInformationEx is not supported.\n");
        return;
    }
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got %#lx\n", status);
    ok(size > 0, "expected nonzero size\n");

    buffer = malloc(size);
    status = pNtQuerySystemInformation(SystemModuleInformationEx, buffer, size, &size);
    ok(!status, "got %#lx\n", status);

    infoex = (const void *)buffer;
    for (i = 0; infoex->NextOffset; i++)
    {
        const RTL_PROCESS_MODULE_INFORMATION *module = &infoex->BaseInfo;

        ok(module->LoadOrderIndex == i, "%lu: got index %u\n", i, module->LoadOrderIndex);
        ok(module->ImageBaseAddress || is_wow64, "%lu: got NULL address for %s\n", i, module->Name);
        ok(module->ImageSize, "%lu: got 0 size\n", i);
        ok(module->LoadCount, "%lu: got 0 load count\n", i);

        infoex = (const void *)((const char *)infoex + infoex->NextOffset);
    }
    ok(((char *)infoex - buffer) + sizeof(infoex->NextOffset) == size,
            "got size %lu, null terminator %Iu\n", size, (char *)infoex - buffer);

    free(buffer);

}

static void test_query_handle(void)
{
    NTSTATUS status;
    ULONG ExpectedLength, ReturnLength;
    ULONG SystemInformationLength = sizeof(SYSTEM_HANDLE_INFORMATION);
    SYSTEM_HANDLE_INFORMATION* shi = HeapAlloc(GetProcessHeap(), 0, SystemInformationLength);
    HANDLE EventHandle;
    BOOL found, ret;
    INT i;

    EventHandle = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok( EventHandle != NULL, "CreateEventA failed %lu\n", GetLastError() );
    ret = SetHandleInformation(EventHandle, HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE,
            HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE);
    ok(ret, "got error %lu\n", GetLastError());

    /* Request the needed length : a SystemInformationLength greater than one struct sets ReturnLength */
    ReturnLength = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( ReturnLength != 0xdeadbeef, "Expected valid ReturnLength\n" );

    SystemInformationLength = ReturnLength;
    shi = HeapReAlloc(GetProcessHeap(), 0, shi , SystemInformationLength);
    memset(shi, 0x55, SystemInformationLength);

    ReturnLength = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);
    while (status == STATUS_INFO_LENGTH_MISMATCH) /* Vista / 2008 */
    {
        SystemInformationLength *= 2;
        shi = HeapReAlloc(GetProcessHeap(), 0, shi, SystemInformationLength);
        memset(shi, 0x55, SystemInformationLength);
        status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);
    }
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status );
    ExpectedLength = FIELD_OFFSET(SYSTEM_HANDLE_INFORMATION, Handle[shi->Count]);
    ok( ReturnLength == ExpectedLength || broken(ReturnLength == ExpectedLength - sizeof(DWORD)), /* Vista / 2008 */
        "Expected length %lu, got %lu\n", ExpectedLength, ReturnLength );
    ok( shi->Count > 1, "Expected more than 1 handle, got %lu\n", shi->Count );
    ok( shi->Handle[1].HandleValue != 0x5555 || broken( shi->Handle[1].HandleValue == 0x5555 ), /* Vista / 2008 */
        "Uninitialized second handle\n" );
    if (shi->Handle[1].HandleValue == 0x5555)
    {
        win_skip("Skipping broken SYSTEM_HANDLE_INFORMATION\n");
        CloseHandle(EventHandle);
        goto done;
    }

    found = FALSE;
    for (i = 0; i < shi->Count; i++)
    {
        if (shi->Handle[i].OwnerPid == GetCurrentProcessId() &&
                (HANDLE)(ULONG_PTR)shi->Handle[i].HandleValue == EventHandle)
        {
            ok(shi->Handle[i].HandleFlags == (OBJ_INHERIT | OBJ_PROTECT_CLOSE),
                    "got attributes %#x\n", shi->Handle[i].HandleFlags);
            found = TRUE;
            break;
        }
    }
    ok( found, "Expected to find event handle %p (pid %lx) in handle list\n", EventHandle, GetCurrentProcessId() );

    ret = SetHandleInformation(EventHandle, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
    ok(ret, "got error %lu\n", GetLastError());
    CloseHandle(EventHandle);

    ReturnLength = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);
    while (status == STATUS_INFO_LENGTH_MISMATCH) /* Vista / 2008 */
    {
        SystemInformationLength *= 2;
        shi = HeapReAlloc(GetProcessHeap(), 0, shi, SystemInformationLength);
        status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);
    }
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status );
    for (i = 0, found = FALSE; i < shi->Count && !found; i++)
        found = (shi->Handle[i].OwnerPid == GetCurrentProcessId()) &&
                ((HANDLE)(ULONG_PTR)shi->Handle[i].HandleValue == EventHandle);
    ok( !found, "Unexpectedly found event handle in handle list\n" );

    status = pNtQuerySystemInformation(SystemHandleInformation, NULL, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", status );

done:
    HeapFree( GetProcessHeap(), 0, shi);
}

static void test_query_handle_ex(void)
{
    SYSTEM_HANDLE_INFORMATION_EX *info = malloc(sizeof(SYSTEM_HANDLE_INFORMATION_EX));
    ULONG size, expect_size;
    NTSTATUS status;
    unsigned int i;
    HANDLE event;
    BOOL found, ret;

    event = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(event != NULL, "failed to create event, error %lu\n", GetLastError());
    ret = SetHandleInformation(event, HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE,
            HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE);
    ok(ret, "got error %lu\n", GetLastError());

    size = 0;
    status = pNtQuerySystemInformation(SystemExtendedHandleInformation, info, sizeof(SYSTEM_HANDLE_INFORMATION_EX), &size);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got %#lx\n", status);
    ok(size > sizeof(SYSTEM_HANDLE_INFORMATION_EX), "got size %lu\n", size);

    while (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        info = realloc(info, size);
        status = pNtQuerySystemInformation(SystemExtendedHandleInformation, info, size, &size);
    }
    ok(!status, "got %#lx\n", status);
    expect_size = FIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[info->NumberOfHandles]);
    ok(size == expect_size, "expected size %lu, got %lu\n", expect_size, size);
    ok(info->NumberOfHandles > 1, "got %Iu handles\n", info->NumberOfHandles);

    found = FALSE;
    for (i = 0; i < info->NumberOfHandles; ++i)
    {
        if (info->Handles[i].UniqueProcessId == GetCurrentProcessId()
                && (HANDLE)info->Handles[i].HandleValue == event)
        {
            ok(info->Handles[i].HandleAttributes == (OBJ_INHERIT | OBJ_PROTECT_CLOSE),
                    "got flags %#lx\n", info->Handles[i].HandleAttributes);
            ok(info->Handles[i].GrantedAccess == EVENT_ALL_ACCESS, "got access %#lx\n", info->Handles[i].GrantedAccess);
            found = TRUE;
        }
        ok(!info->Handles[i].CreatorBackTraceIndex, "got backtrace index %u\n", info->Handles[i].CreatorBackTraceIndex);
    }
    ok(found, "event handle not found\n");

    ret = SetHandleInformation(event, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
    ok(ret, "got error %lu\n", GetLastError());
    CloseHandle(event);

    status = pNtQuerySystemInformation(SystemExtendedHandleInformation, info, size, &size);
    while (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        info = realloc(info, size);
        status = pNtQuerySystemInformation(SystemExtendedHandleInformation, info, size, &size);
    }
    ok(!status, "got %#lx\n", status);
    expect_size = FIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[info->NumberOfHandles]);
    ok(size == expect_size, "expected size %lu, got %lu\n", expect_size, size);
    ok(info->NumberOfHandles > 1, "got %Iu handles\n", info->NumberOfHandles);

    found = FALSE;
    for (i = 0; i < info->NumberOfHandles; ++i)
    {
        if (info->Handles[i].UniqueProcessId == GetCurrentProcessId()
                && (HANDLE)info->Handles[i].HandleValue == event)
        {
            found = TRUE;
            break;
        }
    }
    ok(!found, "event handle found\n");

    status = pNtQuerySystemInformation(SystemExtendedHandleInformation, NULL, sizeof(SYSTEM_HANDLE_INFORMATION_EX), &size);
    ok( status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", status );

    free(info);
}

static void test_query_cache(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    BYTE buffer[128];
    SYSTEM_CACHE_INFORMATION *sci = (SYSTEM_CACHE_INFORMATION *) buffer;
    ULONG expected;
    INT i;

    expected = sizeof(SYSTEM_CACHE_INFORMATION);
    for (i = sizeof(buffer); i>= expected; i--)
    {
        ReturnLength = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemFileCacheInformation, sci, i, &ReturnLength);
        ok(!status && (ReturnLength == expected),
            "%d: got 0x%lx and %lu (expected STATUS_SUCCESS and %lu)\n", i, status, ReturnLength, expected);
    }

    /* buffer too small for the full result.
       Up to win7, the function succeeds with a partial result. */
    status = pNtQuerySystemInformation(SystemFileCacheInformation, sci, i, &ReturnLength);
    if (!status)
    {
        expected = 3 * sizeof(ULONG);
        for (; i>= expected; i--)
        {
            ReturnLength = 0xdeadbeef;
            status = pNtQuerySystemInformation(SystemFileCacheInformation, sci, i, &ReturnLength);
            ok(!status && (ReturnLength == expected),
                "%d: got 0x%lx and %lu (expected STATUS_SUCCESS and %lu)\n", i, status, ReturnLength, expected);
        }
    }

    /* buffer too small for the result, this call will always fail */
    ReturnLength = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemFileCacheInformation, sci, i, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH &&
        ((ReturnLength == expected) || broken(!ReturnLength) || broken(ReturnLength == 0xfffffff0)),
        "%d: got 0x%lx and %lu (expected STATUS_INFO_LENGTH_MISMATCH and %lu)\n", i, status, ReturnLength, expected);

    if (0) {
        /* this crashes on some vista / win7 machines */
        ReturnLength = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemFileCacheInformation, sci, 0, &ReturnLength);
        ok( status == STATUS_INFO_LENGTH_MISMATCH &&
            ((ReturnLength == expected) || broken(!ReturnLength) || broken(ReturnLength == 0xfffffff0)),
            "0: got 0x%lx and %lu (expected STATUS_INFO_LENGTH_MISMATCH and %lu)\n", status, ReturnLength, expected);
    }
}

static void test_query_interrupt(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    ULONG NeededLength;
    SYSTEM_BASIC_INFORMATION sbi;
    SYSTEM_INTERRUPT_INFORMATION* sii;

    /* Find out the number of processors */
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    NeededLength = sbi.NumberOfProcessors * sizeof(SYSTEM_INTERRUPT_INFORMATION);

    sii = HeapAlloc(GetProcessHeap(), 0, NeededLength);

    status = pNtQuerySystemInformation(SystemInterruptInformation, sii, 0, &ReturnLength);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok(ReturnLength == NeededLength, "got %lu\n", ReturnLength);

    /* Try it for all processors */
    status = pNtQuerySystemInformation(SystemInterruptInformation, sii, NeededLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Windows XP and W2K3 (and others?) always return 0 for the ReturnLength
     * No test added for this as it's highly unlikely that an app depends on this
    */

    HeapFree( GetProcessHeap(), 0, sii);
}

static void test_time_adjustment(void)
{
    SYSTEM_TIME_ADJUSTMENT_QUERY query;
    SYSTEM_TIME_ADJUSTMENT adjust;
    NTSTATUS status;
    ULONG len;

    memset( &query, 0xcc, sizeof(query) );
    status = pNtQuerySystemInformation( SystemTimeAdjustmentInformation, &query, sizeof(query), &len );
    ok( status == STATUS_SUCCESS, "got %08lx\n", status );
    ok( len == sizeof(query) || broken(!len) /* winxp */, "wrong len %lu\n", len );
    ok( query.TimeAdjustmentDisabled == TRUE || query.TimeAdjustmentDisabled == FALSE,
        "wrong value %x\n", query.TimeAdjustmentDisabled );

    status = pNtQuerySystemInformation( SystemTimeAdjustmentInformation, &query, sizeof(query)-1, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %08lx\n", status );
    ok( len == sizeof(query) || broken(!len) /* winxp */, "wrong len %lu\n", len );

    status = pNtQuerySystemInformation( SystemTimeAdjustmentInformation, &query, sizeof(query)+1, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %08lx\n", status );
    ok( len == sizeof(query) || broken(!len) /* winxp */, "wrong len %lu\n", len );

    adjust.TimeAdjustment = query.TimeAdjustment;
    adjust.TimeAdjustmentDisabled = query.TimeAdjustmentDisabled;
    status = pNtSetSystemInformation( SystemTimeAdjustmentInformation, &adjust, sizeof(adjust) );
    ok( status == STATUS_SUCCESS || status == STATUS_PRIVILEGE_NOT_HELD, "got %08lx\n", status );
    status = pNtSetSystemInformation( SystemTimeAdjustmentInformation, &adjust, sizeof(adjust)-1 );
    todo_wine
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %08lx\n", status );
    status = pNtSetSystemInformation( SystemTimeAdjustmentInformation, &adjust, sizeof(adjust)+1 );
    todo_wine
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %08lx\n", status );
}

static void test_query_kerndebug(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    /* some Windows version expect alignment */
    SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX DECLSPEC_ALIGN(4) skdi_ex;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION DECLSPEC_ALIGN(4) skdi;

    status = pNtQuerySystemInformation(SystemKernelDebuggerInformation, &skdi, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = pNtQuerySystemInformation(SystemKernelDebuggerInformation, &skdi, sizeof(skdi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(skdi) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);

    status = pNtQuerySystemInformation(SystemKernelDebuggerInformation, &skdi, sizeof(skdi) + 2, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(skdi) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);

    status = pNtQuerySystemInformation(SystemKernelDebuggerInformationEx, &skdi_ex, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH
            || status == STATUS_NOT_IMPLEMENTED    /* before win7 */
            || status == STATUS_INVALID_INFO_CLASS /* wow64 on Win10 */,
            "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    if (status != STATUS_INFO_LENGTH_MISMATCH)
    {
        win_skip( "NtQuerySystemInformation(SystemKernelDebuggerInformationEx) is not implemented.\n" );
    }
    else
    {
        status = pNtQuerySystemInformation(SystemKernelDebuggerInformationEx, &skdi_ex,
                sizeof(skdi_ex), &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( sizeof(skdi_ex) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);

        status = pNtQuerySystemInformation(SystemKernelDebuggerInformationEx, &skdi_ex,
                sizeof(skdi_ex) + 2, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( sizeof(skdi_ex) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);
    }
}

static void test_query_regquota(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    SYSTEM_REGISTRY_QUOTA_INFORMATION srqi;

    status = pNtQuerySystemInformation(SystemRegistryQuotaInformation, &srqi, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = pNtQuerySystemInformation(SystemRegistryQuotaInformation, &srqi, sizeof(srqi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(srqi) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);

    status = pNtQuerySystemInformation(SystemRegistryQuotaInformation, &srqi, sizeof(srqi) + 2, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(srqi) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);
}

static void test_query_logicalproc(void)
{
    NTSTATUS status;
    ULONG len, i, proc_no;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *slpi;
    SYSTEM_INFO si;

    GetSystemInfo(&si);

    status = pNtQuerySystemInformation(SystemLogicalProcessorInformation, NULL, 0, &len);
    if (status == STATUS_INVALID_INFO_CLASS) /* wow64 win8+, arm64 */
    {
        skip("SystemLogicalProcessorInformation is not supported\n");
        return;
    }
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok(len%sizeof(*slpi) == 0, "Incorrect length %ld\n", len);

    slpi = HeapAlloc(GetProcessHeap(), 0, len);
    status = pNtQuerySystemInformation(SystemLogicalProcessorInformation, slpi, len, &len);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    proc_no = 0;
    for(i=0; i<len/sizeof(*slpi); i++) {
        switch(slpi[i].Relationship) {
        case RelationProcessorCore:
            /* Get number of logical processors */
            for(; slpi[i].ProcessorMask; slpi[i].ProcessorMask /= 2)
                proc_no += slpi[i].ProcessorMask%2;
            break;
        default:
            break;
        }
    }
    ok(proc_no > 0, "No processors were found\n");
    if(si.dwNumberOfProcessors <= 32)
        ok(proc_no == si.dwNumberOfProcessors, "Incorrect number of logical processors: %ld, expected %ld\n",
                proc_no, si.dwNumberOfProcessors);

    HeapFree(GetProcessHeap(), 0, slpi);
}

static void test_query_logicalprocex(void)
{
    static const char * const names[] = { "Core", "NumaNode", "Cache", "Package", "Group", "Die", "NumaNodeEx", "Module" };
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *infoex, *infoex_public, *infoex_core, *infoex_numa, *infoex_cache,
                                            *infoex_package, *infoex_group, *infoex_die, *infoex_numa_ex,
                                            *infoex_module, *ex;
    DWORD relationship, len, len_public, len_core, len_numa, len_cache, len_package, len_group, len_die, len_numa_ex,
          len_module, len_union, ret_len;
    unsigned int i, j;
    NTSTATUS status;
    BOOL ret;

    if (!pNtQuerySystemInformationEx)
        return;

    len = 0;
    relationship = RelationAll;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), NULL, 0, &len);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got 0x%08lx\n", status);
    ok(len > 0, "got %lu\n", len);

    len_core = 0;
    relationship = RelationProcessorCore;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), NULL, 0, &len_core);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got 0x%08lx\n", status);
    ok(len_core > 0, "got %lu\n", len_core);

    len_numa = 0;
    relationship = RelationNumaNode;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), NULL, 0, &len_numa);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got 0x%08lx\n", status);
    ok(len_numa > 0, "got %lu\n", len_numa);

    len_cache = 0;
    relationship = RelationCache;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), NULL, 0, &len_cache);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got 0x%08lx\n", status);
    ok(len_cache > 0, "got %lu\n", len_cache);

    len_package = 0;
    relationship = RelationProcessorPackage;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), NULL, 0, &len_package);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got 0x%08lx\n", status);
    ok(len_package > 0, "got %lu\n", len_package);

    len_group = 0;
    relationship = RelationGroup;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), NULL, 0, &len_group);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got 0x%08lx\n", status);
    ok(len_group > 0, "got %lu\n", len_group);

    relationship = RelationProcessorDie;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), NULL, 0, &len_die);
    todo_wine ok(status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_UNSUCCESSFUL || broken(status == STATUS_SUCCESS),
       "got 0x%08lx\n", status);

    len_numa_ex = 0;
    relationship = RelationNumaNodeEx;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), NULL, 0, &len_numa_ex);
    todo_wine ok(status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_UNSUCCESSFUL || broken(status == STATUS_SUCCESS),
       "got 0x%08lx\n", status);

    len_module = 0;
    relationship = RelationProcessorModule;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), NULL, 0, &len_module);
    todo_wine ok(status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_UNSUCCESSFUL || broken(status == STATUS_SUCCESS),
       "got 0x%08lx\n", status);

    len_public = 0;
    ret = pGetLogicalProcessorInformationEx(RelationAll, NULL, &len_public);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %d, error %ld\n", ret, GetLastError());
    ok(len == len_public, "got %lu, expected %lu\n", len_public, len);

    infoex = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    infoex_public = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len_public);
    infoex_core = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len_core);
    infoex_numa = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len_numa);
    infoex_cache = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len_cache);
    infoex_package = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len_package);
    infoex_group = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len_group);
    infoex_die = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len_die);
    infoex_numa_ex = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len_numa_ex);
    infoex_module = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len_module);

    relationship = RelationAll;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), infoex, len, &ret_len);
    ok(status == STATUS_SUCCESS, "got 0x%08lx\n", status);
    ok(ret_len == len, "got %08lx expected %08lx\n", ret_len, len);

    ret = pGetLogicalProcessorInformationEx(RelationAll, infoex_public, &len_public);
    ok(ret, "got %d, error %ld\n", ret, GetLastError());
    ok(!memcmp(infoex, infoex_public, len), "returned info data mismatch\n");

    /* Test for RelationAll. */
    for (i = 0; status == STATUS_SUCCESS && i < len; )
    {
        ex = (void *)(((char *)infoex) + i);
        ok(ex->Size, "%u: got size 0\n", i);

        if (winetest_debug <= 1)
        {
            i += ex->Size;
            continue;
        }

        trace("infoex[%u].Size: %lu\n", i, ex->Size);
        switch (ex->Relationship)
        {
        case RelationProcessorCore:
        case RelationProcessorPackage:
        case RelationProcessorDie:
        case RelationProcessorModule:
            trace("infoex[%u].Relationship: 0x%x (%s)\n", i, ex->Relationship, names[ex->Relationship]);
            trace("infoex[%u].Processor.Flags: 0x%x\n", i, ex->Processor.Flags);
            trace("infoex[%u].Processor.EfficiencyClass: 0x%x\n", i, ex->Processor.EfficiencyClass);
            trace("infoex[%u].Processor.GroupCount: 0x%x\n", i, ex->Processor.GroupCount);
            for (j = 0; j < ex->Processor.GroupCount; ++j)
            {
                trace("infoex[%u].Processor.GroupMask[%u].Mask: 0x%Ix\n", i, j, ex->Processor.GroupMask[j].Mask);
                trace("infoex[%u].Processor.GroupMask[%u].Group: 0x%x\n", i, j, ex->Processor.GroupMask[j].Group);
            }
            break;
        case RelationNumaNode:
        case RelationNumaNodeEx:
            trace("infoex[%u].Relationship: 0x%x (%s)\n", i, ex->Relationship, names[ex->Relationship]);
            trace("infoex[%u].NumaNode.NodeNumber: 0x%lx\n", i, ex->NumaNode.NodeNumber);
            trace("infoex[%u].NumaNode.GroupMask.Mask: 0x%Ix\n", i, ex->NumaNode.GroupMask.Mask);
            trace("infoex[%u].NumaNode.GroupMask.Group: 0x%x\n", i, ex->NumaNode.GroupMask.Group);
            break;
        case RelationCache:
            trace("infoex[%u].Relationship: 0x%x (Cache)\n", i, ex->Relationship);
            trace("infoex[%u].Cache.Level: 0x%x\n", i, ex->Cache.Level);
            trace("infoex[%u].Cache.Associativity: 0x%x\n", i, ex->Cache.Associativity);
            trace("infoex[%u].Cache.LineSize: 0x%x\n", i, ex->Cache.LineSize);
            trace("infoex[%u].Cache.CacheSize: 0x%lx\n", i, ex->Cache.CacheSize);
            trace("infoex[%u].Cache.Type: 0x%x\n", i, ex->Cache.Type);
            trace("infoex[%u].Cache.GroupMask.Mask: 0x%Ix\n", i, ex->Cache.GroupMask.Mask);
            trace("infoex[%u].Cache.GroupMask.Group: 0x%x\n", i, ex->Cache.GroupMask.Group);
            break;
        case RelationGroup:
            trace("infoex[%u].Relationship: 0x%x (Group)\n", i, ex->Relationship);
            trace("infoex[%u].Group.MaximumGroupCount: 0x%x\n", i, ex->Group.MaximumGroupCount);
            trace("infoex[%u].Group.ActiveGroupCount: 0x%x\n", i, ex->Group.ActiveGroupCount);
            for (j = 0; j < ex->Group.ActiveGroupCount; ++j)
            {
                trace("infoex[%u].Group.GroupInfo[%u].MaximumProcessorCount: 0x%x\n", i, j, ex->Group.GroupInfo[j].MaximumProcessorCount);
                trace("infoex[%u].Group.GroupInfo[%u].ActiveProcessorCount: 0x%x\n", i, j, ex->Group.GroupInfo[j].ActiveProcessorCount);
                trace("infoex[%u].Group.GroupInfo[%u].ActiveProcessorMask: 0x%Ix\n", i, j, ex->Group.GroupInfo[j].ActiveProcessorMask);
            }
            break;
        default:
            ok(0, "Got invalid relationship value: 0x%x\n", ex->Relationship);
            break;
        }

        i += ex->Size;
    }

    /* Test Relationship filtering. */

    relationship = RelationProcessorCore;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), infoex_core, len_core, &len_core);
    ok(status == STATUS_SUCCESS, "got 0x%08lx\n", status);

    for (i = 0; status == STATUS_SUCCESS && i < len_core;)
    {
        ex = (void *)(((char*)infoex_core) + i);
        ok(ex->Size, "%u: got size 0\n", i);
        ok(ex->Relationship == RelationProcessorCore, "%u: got relationship %#x\n", i, ex->Relationship);
        i += ex->Size;
    }

    relationship = RelationNumaNode;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), infoex_numa, len_numa, &len_numa);
    ok(status == STATUS_SUCCESS, "got 0x%08lx\n", status);

    for (i = 0; status == STATUS_SUCCESS && i < len_numa;)
    {
        ex = (void *)(((char*)infoex_numa) + i);
        ok(ex->Size, "%u: got size 0\n", i);
        ok(ex->Relationship == RelationNumaNode, "%u: got relationship %#x\n", i, ex->Relationship);
        i += ex->Size;
    }

    relationship = RelationCache;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), infoex_cache, len_cache, &len_cache);
    ok(status == STATUS_SUCCESS, "got 0x%08lx\n", status);

    for (i = 0; status == STATUS_SUCCESS && i < len_cache;)
    {
        ex = (void *)(((char*)infoex_cache) + i);
        ok(ex->Size, "%u: got size 0\n", i);
        ok(ex->Relationship == RelationCache, "%u: got relationship %#x\n", i, ex->Relationship);
        i += ex->Size;
    }

    relationship = RelationProcessorPackage;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), infoex_package, len_package, &len_package);
    ok(status == STATUS_SUCCESS, "got 0x%08lx\n", status);

    for (i = 0; status == STATUS_SUCCESS && i < len_package;)
    {
        ex = (void *)(((char*)infoex_package) + i);
        ok(ex->Size, "%u: got size 0\n", i);
        ok(ex->Relationship == RelationProcessorPackage, "%u: got relationship %#x\n", i, ex->Relationship);
        i += ex->Size;
    }

    relationship = RelationGroup;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), infoex_group, len_group, &len_group);
    ok(status == STATUS_SUCCESS, "got 0x%08lx\n", status);

    for (i = 0; status == STATUS_SUCCESS && i < len_group;)
    {
        ex = (void *)(((char *)infoex_group) + i);
        ok(ex->Size, "%u: got size 0\n", i);
        ok(ex->Relationship == RelationGroup, "%u: got relationship %#x\n", i, ex->Relationship);
        i += ex->Size;
    }

    len_union = len_core + len_numa + len_cache + len_package + len_group + len_module;
    ok(len == len_union, "Expected %lu, got %lu\n", len, len_union);

    HeapFree(GetProcessHeap(), 0, infoex);
    HeapFree(GetProcessHeap(), 0, infoex_public);
    HeapFree(GetProcessHeap(), 0, infoex_core);
    HeapFree(GetProcessHeap(), 0, infoex_numa);
    HeapFree(GetProcessHeap(), 0, infoex_cache);
    HeapFree(GetProcessHeap(), 0, infoex_package);
    HeapFree(GetProcessHeap(), 0, infoex_group);
    HeapFree(GetProcessHeap(), 0, infoex_die);
    HeapFree(GetProcessHeap(), 0, infoex_numa_ex);
    HeapFree(GetProcessHeap(), 0, infoex_module);
}

static void test_query_cpusetinfo(void)
{
    SYSTEM_CPU_SET_INFORMATION *info;
    unsigned int i, cpu_count;
    ULONG len, expected_len;
    NTSTATUS status;
    SYSTEM_INFO si;
    HANDLE process;

    if (!pNtQuerySystemInformationEx)
        return;

    GetSystemInfo(&si);
    cpu_count = si.dwNumberOfProcessors;
    expected_len = cpu_count * sizeof(*info);

    process = GetCurrentProcess();

    status = pNtQuerySystemInformationEx(SystemCpuSetInformation, &process, sizeof(process), NULL, 0, &len);
    if (status == STATUS_INVALID_INFO_CLASS)
    {
        win_skip("SystemCpuSetInformation is not supported\n");
        return;
    }

    ok(status == STATUS_BUFFER_TOO_SMALL, "Got unexpected status %#lx.\n", status);
    ok(len == expected_len, "Got unexpected length %lu.\n", len);

    len = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemCpuSetInformation, NULL, 0, &len);
    ok(status == STATUS_INVALID_PARAMETER || status == STATUS_INVALID_INFO_CLASS,
            "Got unexpected status %#lx.\n", status);
    ok(len == 0xdeadbeef, "Got unexpected len %lu.\n", len);

    len = 0xdeadbeef;
    process = (HANDLE)0xdeadbeef;
    status = pNtQuerySystemInformationEx(SystemCpuSetInformation, &process, sizeof(process), NULL, 0, &len);
    ok(status == STATUS_INVALID_HANDLE, "Got unexpected status %#lx.\n", status);
    ok(len == 0xdeadbeef, "Got unexpected length %lu.\n", len);

    len = 0xdeadbeef;
    process = NULL;
    status = pNtQuerySystemInformationEx(SystemCpuSetInformation, &process, 4 * sizeof(process), NULL, 0, &len);
    ok((status == STATUS_INVALID_PARAMETER && len == 0xdeadbeef)
            || (status == STATUS_BUFFER_TOO_SMALL && len == expected_len),
            "Got unexpected status %#lx, length %lu.\n", status, len);

    len = 0xdeadbeef;
    status = pNtQuerySystemInformationEx(SystemCpuSetInformation, NULL, sizeof(process), NULL, 0, &len);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status);
    ok(len == 0xdeadbeef, "Got unexpected length %lu.\n", len);

    status = pNtQuerySystemInformationEx(SystemCpuSetInformation, &process, sizeof(process), NULL, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "Got unexpected status %#lx.\n", status);
    ok(len == expected_len, "Got unexpected length %lu.\n", len);

    len = 0xdeadbeef;
    status = pNtQuerySystemInformationEx(SystemCpuSetInformation, &process, sizeof(process), NULL,
            expected_len, &len);
    ok(status == STATUS_ACCESS_VIOLATION, "Got unexpected status %#lx.\n", status);
    ok(len == 0xdeadbeef, "Got unexpected length %lu.\n", len);

    info = malloc(expected_len);
    len = 0;
    status = pNtQuerySystemInformationEx(SystemCpuSetInformation, &process, sizeof(process), info, expected_len, &len);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(len == expected_len, "Got unexpected length %lu.\n", len);

    for (i = 0; i < cpu_count; ++i)
    {
        SYSTEM_CPU_SET_INFORMATION *d = &info[i];

        ok(d->Size == sizeof(*d), "Got unexpected size %lu, i %u.\n", d->Size, i);
        ok(d->Type == CpuSetInformation, "Got unexpected type %u, i %u.\n", d->Type, i);
        ok(d->CpuSet.Id == 0x100 + i, "Got unexpected Id %#lx, i %u.\n", d->CpuSet.Id, i);
        ok(!d->CpuSet.Group, "Got unexpected Group %u, i %u.\n", d->CpuSet.Group, i);
        ok(d->CpuSet.LogicalProcessorIndex == i, "Got unexpected LogicalProcessorIndex %u, i %u.\n",
                d->CpuSet.LogicalProcessorIndex, i);
        ok(!d->CpuSet.AllFlags, "Got unexpected AllFlags %#x, i %u.\n", d->CpuSet.AllFlags, i);
    }
    free(info);
}

static void test_query_firmware(void)
{
    static const ULONG min_sfti_len = FIELD_OFFSET(SYSTEM_FIRMWARE_TABLE_INFORMATION, TableBuffer);
    ULONG len1, len2;
    NTSTATUS status;
    SYSTEM_FIRMWARE_TABLE_INFORMATION *sfti;

    sfti = HeapAlloc(GetProcessHeap(), 0, sizeof(*sfti));
    ok(!!sfti, "Failed to allocate memory\n");

    sfti->ProviderSignature = 0;
    sfti->Action = 0;
    sfti->TableID = 0;

    status = pNtQuerySystemInformation(SystemFirmwareTableInformation, sfti, min_sfti_len - 1, &len1);
    ok(status == STATUS_INFO_LENGTH_MISMATCH || broken(status == STATUS_INVALID_INFO_CLASS) /* xp */,
       "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    if (len1 == 0) /* xp, 2003 */
    {
        win_skip("SystemFirmwareTableInformation is not available\n");
        HeapFree(GetProcessHeap(), 0, sfti);
        return;
    }
    ok(len1 == min_sfti_len, "Expected length %lu, got %lu\n", min_sfti_len, len1);

    status = pNtQuerySystemInformation(SystemFirmwareTableInformation, sfti, min_sfti_len, &len1);
    ok(status == STATUS_NOT_IMPLEMENTED, "Expected STATUS_NOT_IMPLEMENTED, got %08lx\n", status);
    ok(len1 == 0, "Expected length 0, got %lu\n", len1);

    sfti->ProviderSignature = RSMB;
    sfti->Action = SystemFirmwareTable_Get;

    status = pNtQuerySystemInformation(SystemFirmwareTableInformation, sfti, min_sfti_len, &len1);
#ifdef __REACTOS__
    if (status == STATUS_INVALID_DEVICE_REQUEST)
    {
        win_skip("SystemFirmwareTableInformation not available\n");
        HeapFree(GetProcessHeap(), 0, sfti);
        return;
    }
#endif
    ok(status == STATUS_BUFFER_TOO_SMALL, "Expected STATUS_BUFFER_TOO_SMALL, got %08lx\n", status);
    ok(len1 >= min_sfti_len, "Expected length >= %lu, got %lu\n", min_sfti_len, len1);
    ok(sfti->TableBufferLength == len1 - min_sfti_len,
       "Expected length %lu, got %lu\n", len1 - min_sfti_len, sfti->TableBufferLength);

    sfti = HeapReAlloc(GetProcessHeap(), 0, sfti, len1);
    ok(!!sfti, "Failed to allocate memory\n");

    status = pNtQuerySystemInformation(SystemFirmwareTableInformation, sfti, len1, &len2);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok(len2 == len1, "Expected length %lu, got %lu\n", len1, len2);
    ok(sfti->TableBufferLength == len1 - min_sfti_len,
       "Expected length %lu, got %lu\n", len1 - min_sfti_len, sfti->TableBufferLength);

    HeapFree(GetProcessHeap(), 0, sfti);
}

static void test_query_battery(void)
{
    SYSTEM_BATTERY_STATE bs;
    NTSTATUS status;
    DWORD time_left;

    memset(&bs, 0x23, sizeof(bs));
    status = NtPowerInformation(SystemBatteryState, NULL, 0, &bs, sizeof(bs));
    if (status == STATUS_NOT_IMPLEMENTED)
    {
        skip("SystemBatteryState not implemented\n");
        return;
    }
    ok(status == STATUS_SUCCESS, "expected success\n");

    if (winetest_debug > 1)
    {
        trace("Battery state:\n");
        trace("AcOnLine          : %u\n", bs.AcOnLine);
        trace("BatteryPresent    : %u\n", bs.BatteryPresent);
        trace("Charging          : %u\n", bs.Charging);
        trace("Discharging       : %u\n", bs.Discharging);
        trace("Tag               : %u\n", bs.Tag);
        trace("MaxCapacity       : %lu\n", bs.MaxCapacity);
        trace("RemainingCapacity : %lu\n", bs.RemainingCapacity);
        trace("Rate              : %ld\n", (LONG)bs.Rate);
        trace("EstimatedTime     : %lu\n", bs.EstimatedTime);
        trace("DefaultAlert1     : %lu\n", bs.DefaultAlert1);
        trace("DefaultAlert2     : %lu\n", bs.DefaultAlert2);
    }

    ok(bs.MaxCapacity >= bs.RemainingCapacity,
       "expected MaxCapacity %lu to be greater than or equal to RemainingCapacity %lu\n",
       bs.MaxCapacity, bs.RemainingCapacity);

    if (!bs.BatteryPresent)
        time_left = 0;
    else if (!bs.Charging && (LONG)bs.Rate < 0)
        time_left = 3600 * bs.RemainingCapacity / -(LONG)bs.Rate;
    else
        time_left = ~0u;
    ok(bs.EstimatedTime == time_left,
       "expected %lu minutes remaining got %lu minutes\n", time_left, bs.EstimatedTime);
}

static void test_query_processor_power_info(void)
{
    NTSTATUS status;
    PROCESSOR_POWER_INFORMATION* ppi;
    ULONG size;
    SYSTEM_INFO si;
    int i;

    GetSystemInfo(&si);
    size = si.dwNumberOfProcessors * sizeof(PROCESSOR_POWER_INFORMATION);
    ppi = HeapAlloc(GetProcessHeap(), 0, size);

    /* If size < (sizeof(PROCESSOR_POWER_INFORMATION) * NumberOfProcessors), Win7 returns
     * STATUS_BUFFER_TOO_SMALL. WinXP returns STATUS_SUCCESS for any value of size.  It copies as
     * many whole PROCESSOR_POWER_INFORMATION structures that there is room for.  Even if there is
     * not enough room for one structure, WinXP still returns STATUS_SUCCESS having done nothing.
     *
     * If ppi == NULL, Win7 returns STATUS_INVALID_PARAMETER while WinXP returns STATUS_SUCCESS
     * and does nothing.
     *
     * The same behavior is seen with CallNtPowerInformation (in powrprof.dll).
     */

    if (si.dwNumberOfProcessors > 1)
    {
        for(i = 0; i < si.dwNumberOfProcessors; i++)
            ppi[i].Number = 0xDEADBEEF;

        /* Call with a buffer size that is large enough to hold at least one but not large
         * enough to hold them all.  This will be STATUS_SUCCESS on WinXP but not on Win7 */
        status = pNtPowerInformation(ProcessorInformation, 0, 0, ppi, size - sizeof(PROCESSOR_POWER_INFORMATION));
        if (status == STATUS_SUCCESS)
        {
            /* lax version found on older Windows like WinXP */
            ok( (ppi[si.dwNumberOfProcessors - 2].Number != 0xDEADBEEF) &&
                (ppi[si.dwNumberOfProcessors - 1].Number == 0xDEADBEEF),
                "Expected all but the last record to be overwritten.\n");

            status = pNtPowerInformation(ProcessorInformation, 0, 0, 0, size);
            ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

            for(i = 0; i < si.dwNumberOfProcessors; i++)
                ppi[i].Number = 0xDEADBEEF;
            status = pNtPowerInformation(ProcessorInformation, 0, 0, ppi, sizeof(PROCESSOR_POWER_INFORMATION) - 1);
            ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
            for(i = 0; i < si.dwNumberOfProcessors; i++)
                if (ppi[i].Number != 0xDEADBEEF) break;
            ok( i == si.dwNumberOfProcessors, "Expected untouched buffer\n");
        }
        else
        {
            /* picky version found on newer Windows like Win7 */
            ok( ppi[1].Number == 0xDEADBEEF, "Expected untouched buffer.\n");
            ok( status == STATUS_BUFFER_TOO_SMALL, "Expected STATUS_BUFFER_TOO_SMALL, got %08lx\n", status);

            status = pNtPowerInformation(ProcessorInformation, 0, 0, 0, size);
            ok( status == STATUS_SUCCESS || status == STATUS_INVALID_PARAMETER, "Got %08lx\n", status);

            status = pNtPowerInformation(ProcessorInformation, 0, 0, ppi, 0);
            ok( status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INVALID_PARAMETER, "Got %08lx\n", status);
        }
    }
    else
    {
        skip("Test needs more than one processor.\n");
    }

    status = pNtPowerInformation(ProcessorInformation, 0, 0, ppi, size);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    HeapFree(GetProcessHeap(), 0, ppi);
}

static void test_query_process_wow64(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    ULONG_PTR pbi[2], dummy;

    memset(&dummy, 0xcc, sizeof(dummy));

    /* Do not give a handle and buffer */
    status = NtQueryInformationProcess(NULL, ProcessWow64Information, NULL, 0, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Use a correct info class and buffer size, but still no handle and buffer */
    status = NtQueryInformationProcess(NULL, ProcessWow64Information, NULL, sizeof(ULONG_PTR), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE, got %08lx\n", status);

    /* Use a correct info class, buffer size and handle, but no buffer */
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, NULL, sizeof(ULONG_PTR), NULL);
    ok( status == STATUS_ACCESS_VIOLATION , "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", status);

    /* Use a correct info class, buffer and buffer size, but no handle */
    pbi[0] = pbi[1] = dummy;
    status = NtQueryInformationProcess(NULL, ProcessWow64Information, pbi, sizeof(ULONG_PTR), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);
    ok( pbi[0] == dummy, "pbi[0] changed to %Ix\n", pbi[0]);
    ok( pbi[1] == dummy, "pbi[1] changed to %Ix\n", pbi[1]);

    /* Use a greater buffer size */
    pbi[0] = pbi[1] = dummy;
    status = NtQueryInformationProcess(NULL, ProcessWow64Information, pbi, sizeof(ULONG_PTR) + 1, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( pbi[0] == dummy, "pbi[0] changed to %Ix\n", pbi[0]);
    ok( pbi[1] == dummy, "pbi[1] changed to %Ix\n", pbi[1]);

    /* Use no ReturnLength */
    pbi[0] = pbi[1] = dummy;
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, pbi, sizeof(ULONG_PTR), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( is_wow64 == (pbi[0] != 0), "is_wow64 %x, pbi[0] %Ix\n", is_wow64, pbi[0]);
    if (is_wow64)
        ok( (void *)pbi[0] == NtCurrentTeb()->Peb, "pbi[0] %Ix / %p\n", pbi[0], NtCurrentTeb()->Peb);
    ok( pbi[1] == dummy, "pbi[1] changed to %Ix\n", pbi[1]);
    /* Test written size on 64 bit by checking high 32 bit buffer */
    if (sizeof(ULONG_PTR) > sizeof(DWORD))
    {
        DWORD *ptr = (DWORD *)pbi;
        ok( ptr[1] != (DWORD)dummy, "ptr[1] unchanged!\n");
    }

    /* Finally some correct calls */
    pbi[0] = pbi[1] = dummy;
    ReturnLength = 0xdeadbeef;
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, pbi, sizeof(ULONG_PTR), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( is_wow64 == (pbi[0] != 0), "is_wow64 %x, pbi[0] %Ix\n", is_wow64, pbi[0]);
    if (is_wow64)
        ok( (void *)pbi[0] == NtCurrentTeb()->Peb, "pbi[0] %Ix / %p\n", pbi[0], NtCurrentTeb()->Peb);
    ok( pbi[1] == dummy, "pbi[1] changed to %Ix\n", pbi[1]);
    ok( ReturnLength == sizeof(ULONG_PTR), "Inconsistent length %ld\n", ReturnLength);

    /* Everything is correct except a too small buffer size */
    pbi[0] = pbi[1] = dummy;
    ReturnLength = 0xdeadbeef;
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, pbi, sizeof(ULONG_PTR) - 1, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( pbi[0] == dummy, "pbi[0] changed to %Ix\n", pbi[0]);
    ok( pbi[1] == dummy, "pbi[1] changed to %Ix\n", pbi[1]);
    ok( ReturnLength == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", ReturnLength);

    /* Everything is correct except a too large buffer size */
    pbi[0] = pbi[1] = dummy;
    ReturnLength = 0xdeadbeef;
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, pbi, sizeof(ULONG_PTR) + 1, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( pbi[0] == dummy, "pbi[0] changed to %Ix\n", pbi[0]);
    ok( pbi[1] == dummy, "pbi[1] changed to %Ix\n", pbi[1]);
    ok( ReturnLength == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", ReturnLength);
}

static void test_query_process_basic(void)
{
    NTSTATUS status;
    ULONG ReturnLength;

    typedef struct _PROCESS_BASIC_INFORMATION_PRIVATE {
        DWORD_PTR ExitStatus;
        PPEB      PebBaseAddress;
        DWORD_PTR AffinityMask;
        DWORD_PTR BasePriority;
        ULONG_PTR UniqueProcessId;
        ULONG_PTR InheritedFromUniqueProcessId;
    } PROCESS_BASIC_INFORMATION_PRIVATE;

    PROCESS_BASIC_INFORMATION_PRIVATE pbi;

    /* This test also covers some basic parameter testing that should be the same for
     * every information class
    */

    status = NtQueryInformationProcess(NULL, -1, NULL, 0, NULL);
    ok( status == STATUS_INVALID_INFO_CLASS || status == STATUS_NOT_IMPLEMENTED /* vista */,
        "Expected STATUS_INVALID_INFO_CLASS or STATUS_NOT_IMPLEMENTED, got %08lx\n", status);

    status = NtQueryInformationProcess(NULL, ProcessBasicInformation, NULL, 0, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = NtQueryInformationProcess(NULL, ProcessBasicInformation, NULL, sizeof(pbi), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08lx\n", status);

    status = NtQueryInformationProcess(NULL, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    status = NtQueryInformationProcess(NULL, ProcessBasicInformation, &pbi, sizeof(pbi) * 2, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(pbi) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);

    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( sizeof(pbi) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);

    if (winetest_debug > 1) trace("ProcessID : %Ix\n", pbi.UniqueProcessId);
    ok( pbi.UniqueProcessId > 0, "Expected a ProcessID > 0, got 0\n");
}

static void dump_vm_counters(const char *header, const VM_COUNTERS_EX *pvi)
{
    trace("%s:\n", header);
    trace("PeakVirtualSize           : %Iu\n", pvi->PeakVirtualSize);
    trace("VirtualSize               : %Iu\n", pvi->VirtualSize);
    trace("PageFaultCount            : %lu\n",  pvi->PageFaultCount);
    trace("PeakWorkingSetSize        : %Iu\n", pvi->PeakWorkingSetSize);
    trace("WorkingSetSize            : %Iu\n", pvi->WorkingSetSize);
    trace("QuotaPeakPagedPoolUsage   : %Iu\n", pvi->QuotaPeakPagedPoolUsage);
    trace("QuotaPagedPoolUsage       : %Iu\n", pvi->QuotaPagedPoolUsage);
    trace("QuotaPeakNonPagePoolUsage : %Iu\n", pvi->QuotaPeakNonPagedPoolUsage);
    trace("QuotaNonPagePoolUsage     : %Iu\n", pvi->QuotaNonPagedPoolUsage);
    trace("PagefileUsage             : %Iu\n", pvi->PagefileUsage);
    trace("PeakPagefileUsage         : %Iu\n", pvi->PeakPagefileUsage);
}

static void test_query_process_vm(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    VM_COUNTERS_EX pvi;
    HANDLE process;
    SIZE_T prev_size;
    const SIZE_T alloc_size = 16 * 1024 * 1024;
    void *ptr;

    status = NtQueryInformationProcess(NULL, ProcessVmCounters, NULL, sizeof(pvi), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08lx\n", status);

    status = NtQueryInformationProcess(NULL, ProcessVmCounters, &pvi, sizeof(VM_COUNTERS), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessVmCounters, &pvi, 24, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessVmCounters, &pvi, sizeof(VM_COUNTERS), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( ReturnLength == sizeof(VM_COUNTERS), "Inconsistent length %ld\n", ReturnLength);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessVmCounters, &pvi, 46, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    todo_wine ok( ReturnLength == sizeof(VM_COUNTERS), "wrong size %ld\n", ReturnLength);

    /* Check if we have some return values */
    if (winetest_debug > 1)
        dump_vm_counters("VM counters for GetCurrentProcess", &pvi);
    ok( pvi.WorkingSetSize > 0, "Expected a WorkingSetSize > 0\n");
    ok( pvi.PagefileUsage > 0, "Expected a PagefileUsage > 0\n");

    process = OpenProcess(PROCESS_VM_READ, FALSE, GetCurrentProcessId());
    status = NtQueryInformationProcess(process, ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_ACCESS_DENIED, "Expected STATUS_ACCESS_DENIED, got %08lx\n", status);
    CloseHandle(process);

    process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, GetCurrentProcessId());
    status = NtQueryInformationProcess(process, ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS || broken(!process) /* XP */, "Expected STATUS_SUCCESS, got %08lx\n", status);
    CloseHandle(process);

    memset(&pvi, 0, sizeof(pvi));
    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
    status = NtQueryInformationProcess(process, ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( pvi.PrivateUsage == pvi.PagefileUsage, "wrong value %Iu/%Iu\n", pvi.PrivateUsage, pvi.PagefileUsage );

    /* Check if we have some return values */
    if (winetest_debug > 1)
        dump_vm_counters("VM counters for GetCurrentProcessId", &pvi);
    ok( pvi.WorkingSetSize > 0, "Expected a WorkingSetSize > 0\n");
    ok( pvi.PagefileUsage > 0, "Expected a PagefileUsage > 0\n");

    CloseHandle(process);

    /* Check if we have real counters */
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( pvi.PrivateUsage == pvi.PagefileUsage, "wrong value %Iu/%Iu\n", pvi.PrivateUsage, pvi.PagefileUsage );
    prev_size = pvi.VirtualSize;
    if (winetest_debug > 1)
        dump_vm_counters("VM counters before VirtualAlloc", &pvi);
    ptr = VirtualAlloc(NULL, alloc_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    ok( ptr != NULL, "VirtualAlloc failed, err %lu\n", GetLastError());
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( pvi.PrivateUsage == pvi.PagefileUsage, "wrong value %Iu/%Iu\n", pvi.PrivateUsage, pvi.PagefileUsage );
    if (winetest_debug > 1)
        dump_vm_counters("VM counters after VirtualAlloc", &pvi);
    todo_wine ok( pvi.VirtualSize >= prev_size + alloc_size,
        "Expected to be greater than %Iu, got %Iu\n", prev_size + alloc_size, pvi.VirtualSize);
    VirtualFree( ptr, 0, MEM_RELEASE);

    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( pvi.PrivateUsage == pvi.PagefileUsage, "wrong value %Iu/%Iu\n", pvi.PrivateUsage, pvi.PagefileUsage );
    prev_size = pvi.VirtualSize;
    if (winetest_debug > 1)
        dump_vm_counters("VM counters before VirtualAlloc", &pvi);
    ptr = VirtualAlloc(NULL, alloc_size, MEM_RESERVE, PAGE_READWRITE);
    ok( ptr != NULL, "VirtualAlloc failed, err %lu\n", GetLastError());
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( pvi.PrivateUsage == pvi.PagefileUsage, "wrong value %Iu/%Iu\n", pvi.PrivateUsage, pvi.PagefileUsage );
    if (winetest_debug > 1)
        dump_vm_counters("VM counters after VirtualAlloc(MEM_RESERVE)", &pvi);
    todo_wine ok( pvi.VirtualSize >= prev_size + alloc_size,
        "Expected to be greater than %Iu, got %Iu\n", prev_size + alloc_size, pvi.VirtualSize);
    prev_size = pvi.VirtualSize;

    ptr = VirtualAlloc(ptr, alloc_size, MEM_COMMIT, PAGE_READWRITE);
    ok( ptr != NULL, "VirtualAlloc failed, err %lu\n", GetLastError());
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( pvi.PrivateUsage == pvi.PagefileUsage, "wrong value %Iu/%Iu\n", pvi.PrivateUsage, pvi.PagefileUsage );
    if (winetest_debug > 1)
        dump_vm_counters("VM counters after VirtualAlloc(MEM_COMMIT)", &pvi);
    ok( pvi.VirtualSize == prev_size,
        "Expected to equal to %Iu, got %Iu\n", prev_size, pvi.VirtualSize);
    VirtualFree( ptr, 0, MEM_RELEASE);
}

static void test_query_process_io(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    IO_COUNTERS pii;

    status = NtQueryInformationProcess(NULL, ProcessIoCounters, NULL, sizeof(pii), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08lx\n", status);

    status = NtQueryInformationProcess(NULL, ProcessIoCounters, &pii, sizeof(pii), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessIoCounters, &pii, 24, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessIoCounters, &pii, sizeof(pii), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(pii) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessIoCounters, &pii, sizeof(pii) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( sizeof(pii) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);

    /* Check if we have some return values */
    if (winetest_debug > 1) trace("OtherOperationCount : 0x%s\n", wine_dbgstr_longlong(pii.OtherOperationCount));
    todo_wine
    {
        ok( pii.OtherOperationCount > 0, "Expected an OtherOperationCount > 0\n");
    }
}

static void test_query_process_times(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    HANDLE process;
    SYSTEMTIME UTC, Local;
    KERNEL_USER_TIMES spti;

    status = NtQueryInformationProcess(NULL, ProcessTimes, NULL, sizeof(spti), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08lx\n", status);

    status = NtQueryInformationProcess(NULL, ProcessTimes, &spti, sizeof(spti), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessTimes, &spti, 24, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, one_before_last_pid);
    if (!process)
    {
        if (winetest_debug > 1) trace("Could not open process with ID : %ld, error : %lu. Going to use current one.\n", one_before_last_pid, GetLastError());
        process = GetCurrentProcess();
    }
    else
        trace("ProcessTimes for process with ID : %ld\n", one_before_last_pid);

    status = NtQueryInformationProcess( process, ProcessTimes, &spti, sizeof(spti), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(spti) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);
    CloseHandle(process);

    FileTimeToSystemTime((const FILETIME *)&spti.CreateTime, &UTC);
    SystemTimeToTzSpecificLocalTime(NULL, &UTC, &Local);
    if (winetest_debug > 1) trace("CreateTime : %02d/%02d/%04d %02d:%02d:%02d\n", Local.wMonth, Local.wDay, Local.wYear,
           Local.wHour, Local.wMinute, Local.wSecond);

    FileTimeToSystemTime((const FILETIME *)&spti.ExitTime, &UTC);
    SystemTimeToTzSpecificLocalTime(NULL, &UTC, &Local);
    if (winetest_debug > 1) trace("ExitTime   : %02d/%02d/%04d %02d:%02d:%02d\n", Local.wMonth, Local.wDay, Local.wYear,
           Local.wHour, Local.wMinute, Local.wSecond);

    FileTimeToSystemTime((const FILETIME *)&spti.KernelTime, &Local);
    if (winetest_debug > 1) trace("KernelTime : %02d:%02d:%02d.%03d\n", Local.wHour, Local.wMinute, Local.wSecond, Local.wMilliseconds);

    FileTimeToSystemTime((const FILETIME *)&spti.UserTime, &Local);
    if (winetest_debug > 1) trace("UserTime   : %02d:%02d:%02d.%03d\n", Local.wHour, Local.wMinute, Local.wSecond, Local.wMilliseconds);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessTimes, &spti, sizeof(spti) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( sizeof(spti) == ReturnLength ||
        ReturnLength == 0 /* vista */ ||
        broken(is_wow64),  /* returns garbage on wow64 */
        "Inconsistent length %ld\n", ReturnLength);
}

static void test_query_process_debug_port(int argc, char **argv)
{
    DWORD_PTR debug_port = 0xdeadbeef;
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    NTSTATUS status;
    BOOL ret;
    ULONG len;

    sprintf(cmdline, "%s %s %s", argv[0], argv[1], "debuggee");

    si.cb = sizeof(si);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed, last error %#lx.\n", GetLastError());
    if (!ret) return;

    status = NtQueryInformationProcess(NULL, ProcessDebugPort,
            NULL, 0, NULL);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %#lx.\n", status);

    status = NtQueryInformationProcess(NULL, ProcessDebugPort,
            NULL, sizeof(debug_port), NULL);
    ok(status == STATUS_INVALID_HANDLE || status == STATUS_ACCESS_VIOLATION /* XP */, "got %#lx\n", status);

    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort,
            NULL, sizeof(debug_port), NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %#lx.\n", status);

    len = 0xdeadbeef;
    status = NtQueryInformationProcess(NULL, ProcessDebugPort,
            &debug_port, sizeof(debug_port), &len);
    ok(status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %#lx.\n", status);
    ok(len == 0xdeadbeef || broken(len != sizeof(debug_port)), /* wow64 */
       "len set to %lx\n", len );

    len = 0xdeadbeef;
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort,
            &debug_port, sizeof(debug_port) - 1, &len);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %#lx.\n", status);
    ok(len == 0xdeadbeef || broken(len != sizeof(debug_port)), /* wow64 */
       "len set to %lx\n", len );

    len = 0xdeadbeef;
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort,
            &debug_port, sizeof(debug_port) + 1, &len);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %#lx.\n", status);
    ok(len == 0xdeadbeef || broken(len != sizeof(debug_port)), /* wow64 */
       "len set to %lx\n", len );

    len = 0xdeadbeef;
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort,
            &debug_port, sizeof(debug_port), &len);
    ok(!status, "NtQueryInformationProcess failed, status %#lx.\n", status);
    ok(debug_port == 0, "Expected port 0, got %#Ix.\n", debug_port);
    ok(len == sizeof(debug_port), "len set to %lx\n", len );

    len = 0xdeadbeef;
    status = NtQueryInformationProcess(pi.hProcess, ProcessDebugPort,
            &debug_port, sizeof(debug_port), &len);
    ok(!status, "NtQueryInformationProcess failed, status %#lx.\n", status);
    ok(debug_port == ~(DWORD_PTR)0, "Expected port %#Ix, got %#Ix.\n", ~(DWORD_PTR)0, debug_port);
    ok(len == sizeof(debug_port), "len set to %lx\n", len );

    for (;;)
    {
        DEBUG_EVENT ev;

        ret = WaitForDebugEvent(&ev, INFINITE);
        ok(ret, "WaitForDebugEvent failed, last error %#lx.\n", GetLastError());
        if (!ret) break;

        if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) break;

        ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
        ok(ret, "ContinueDebugEvent failed, last error %#lx.\n", GetLastError());
        if (!ret) break;
    }

    ret = CloseHandle(pi.hThread);
    ok(ret, "CloseHandle failed, last error %#lx.\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "CloseHandle failed, last error %#lx.\n", GetLastError());
}

static void subtest_query_process_debug_port_custom_dacl(int argc, char **argv, ACCESS_MASK access, PSID sid)
{
    HANDLE old_debug_obj, debug_obj;
    OBJECT_ATTRIBUTES attr;
    SECURITY_DESCRIPTOR sd;
    union {
        ACL acl;
        DWORD buffer[(sizeof(ACL) +
                      (offsetof(ACCESS_ALLOWED_ACE, SidStart) + SECURITY_MAX_SID_SIZE) +
                      sizeof(DWORD) - 1) / sizeof(DWORD)];
    } acl;
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    DEBUG_EVENT ev;
    NTSTATUS status;
    BOOL ret;

    InitializeAcl(&acl.acl, sizeof(acl), ACL_REVISION);
    AddAccessAllowedAce(&acl.acl, ACL_REVISION, access, sid);
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, &acl.acl, FALSE);

    InitializeObjectAttributes(&attr, NULL, 0, NULL, &sd);
    status = NtCreateDebugObject(&debug_obj, MAXIMUM_ALLOWED, &attr, DEBUG_KILL_ON_CLOSE);
    ok(SUCCEEDED(status), "Failed to create debug object: %#010lx\n", status);
    if (FAILED(status)) return;

    old_debug_obj = pDbgUiGetThreadDebugObject();
    pDbgUiSetThreadDebugObject(debug_obj);

    sprintf(cmdline, "%s %s %s %lu", argv[0], argv[1], "debuggee:dbgport", access);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                         DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed, last error %#lx.\n", GetLastError());
    if (!ret) goto close_debug_obj;

    do
    {
        ret = WaitForDebugEvent(&ev, INFINITE);
        ok(ret, "WaitForDebugEvent failed, last error %#lx.\n", GetLastError());
        if (!ret) break;

        ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
        ok(ret, "ContinueDebugEvent failed, last error %#lx.\n", GetLastError());
        if (!ret) break;
    } while (ev.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    wait_child_process(pi.hProcess);
    ret = CloseHandle(pi.hThread);
    ok(ret, "CloseHandle failed, last error %#lx.\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "CloseHandle failed, last error %#lx.\n", GetLastError());

close_debug_obj:
    pDbgUiSetThreadDebugObject(old_debug_obj);
    NtClose(debug_obj);
}

static TOKEN_OWNER *get_current_owner(void)
{
    TOKEN_OWNER *owner;
    ULONG length = 0;
    HANDLE token;
    BOOL ret;

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &token);
    ok(ret, "Failed to get process token: %lu\n", GetLastError());

    ret = GetTokenInformation(token, TokenOwner, NULL, 0, &length);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "GetTokenInformation failed: %lu\n", GetLastError());
    ok(length != 0, "Failed to get token owner information length: %lu\n", GetLastError());

    owner = HeapAlloc(GetProcessHeap(), 0, length);
    ret = GetTokenInformation(token, TokenOwner, owner, length, &length);
    ok(ret, "Failed to get token owner information: %lu)\n", GetLastError());

    CloseHandle(token);
    return owner;
}

static void test_query_process_debug_port_custom_dacl(int argc, char **argv)
{
    static const ACCESS_MASK all_access_masks[] = {
        GENERIC_ALL,
        DEBUG_ALL_ACCESS,
        STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE,
    };
    TOKEN_OWNER *owner;
    int i;

    if (!pDbgUiSetThreadDebugObject)
    {
        win_skip("DbgUiGetThreadDebugObject not found\n");
        return;
    }

    if (!pDbgUiGetThreadDebugObject)
    {
        win_skip("DbgUiSetThreadDebugObject not found\n");
        return;
    }

    owner = get_current_owner();

    for (i = 0; i < ARRAY_SIZE(all_access_masks); i++)
    {
        ACCESS_MASK access = all_access_masks[i];

        winetest_push_context("debug object access %08lx", access);
        subtest_query_process_debug_port_custom_dacl(argc, argv, access, owner->Owner);
        winetest_pop_context();
    }

    HeapFree(GetProcessHeap(), 0, owner);
}

static void test_query_process_priority(void)
{
    PROCESS_PRIORITY_CLASS priority[2];
    ULONG ReturnLength;
    DWORD orig_priority;
    NTSTATUS status;
    BOOL ret;

    status = NtQueryInformationProcess(NULL, ProcessPriorityClass, NULL, sizeof(priority[0]), NULL);
    ok(status == STATUS_ACCESS_VIOLATION || broken(status == STATUS_INVALID_HANDLE) /* w2k3 */,
       "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", status);

    status = NtQueryInformationProcess(NULL, ProcessPriorityClass, &priority, sizeof(priority[0]), NULL);
    ok(status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessPriorityClass, &priority, 1, &ReturnLength);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessPriorityClass, &priority, sizeof(priority), &ReturnLength);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    orig_priority = GetPriorityClass(GetCurrentProcess());
    ret = SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
    ok(ret, "Failed to set priority class: %lu\n", GetLastError());

    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessPriorityClass, &priority, sizeof(priority[0]), &ReturnLength);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok(priority[0].PriorityClass == PROCESS_PRIOCLASS_BELOW_NORMAL,
       "Expected PROCESS_PRIOCLASS_BELOW_NORMAL, got %u\n", priority[0].PriorityClass);

    ret = SetPriorityClass(GetCurrentProcess(), orig_priority);
    ok(ret, "Failed to reset priority class: %lu\n", GetLastError());
}

static void test_query_process_handlecount(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    DWORD handlecount;
    BYTE buffer[2 * sizeof(DWORD)];
    HANDLE process;

    status = NtQueryInformationProcess(NULL, ProcessHandleCount, NULL, sizeof(handlecount), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08lx\n", status);

    status = NtQueryInformationProcess(NULL, ProcessHandleCount, &handlecount, sizeof(handlecount), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessHandleCount, &handlecount, 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, one_before_last_pid);
    if (!process)
    {
        trace("Could not open process with ID : %ld, error : %lu. Going to use current one.\n", one_before_last_pid, GetLastError());
        process = GetCurrentProcess();
    }
    else
        if (winetest_debug > 1) trace("ProcessHandleCount for process with ID : %ld\n", one_before_last_pid);

    status = NtQueryInformationProcess( process, ProcessHandleCount, &handlecount, sizeof(handlecount), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(handlecount) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);
    CloseHandle(process);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessHandleCount, buffer, sizeof(buffer), &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_SUCCESS,
        "Expected STATUS_INFO_LENGTH_MISMATCH or STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(handlecount) == ReturnLength, "Inconsistent length %ld\n", ReturnLength);

    /* Check if we have some return values */
    if (winetest_debug > 1) trace("HandleCount : %ld\n", handlecount);
    todo_wine
    {
        ok( handlecount > 0, "Expected some handles, got 0\n");
    }
}

static void test_query_process_image_file_name(void)
{
    static const WCHAR deviceW[] = {'\\','D','e','v','i','c','e','\\'};
    NTSTATUS status;
    ULONG ReturnLength;
    UNICODE_STRING image_file_name;
    UNICODE_STRING *buffer = NULL;

    status = NtQueryInformationProcess(NULL, ProcessImageFileName, &image_file_name, sizeof(image_file_name), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessImageFileName, &image_file_name, 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessImageFileName, &image_file_name, sizeof(image_file_name), &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    buffer = malloc(ReturnLength);
    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessImageFileName, buffer, ReturnLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    todo_wine
    ok(!memcmp(buffer->Buffer, deviceW, sizeof(deviceW)),
        "Expected image name to begin with \\Device\\, got %s\n",
        wine_dbgstr_wn(buffer->Buffer, buffer->Length / sizeof(WCHAR)));
    free(buffer);

    status = NtQueryInformationProcess(NULL, ProcessImageFileNameWin32, &image_file_name, sizeof(image_file_name), NULL);
    if (status == STATUS_INVALID_INFO_CLASS)
    {
        win_skip("ProcessImageFileNameWin32 is not supported\n");
        return;
    }
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessImageFileNameWin32, &image_file_name, 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessImageFileNameWin32, &image_file_name, sizeof(image_file_name), &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    buffer = malloc(ReturnLength);
    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessImageFileNameWin32, buffer, ReturnLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok(memcmp(buffer->Buffer, deviceW, sizeof(deviceW)),
        "Expected image name not to begin with \\Device\\, got %s\n",
        wine_dbgstr_wn(buffer->Buffer, buffer->Length / sizeof(WCHAR)));
    free(buffer);
}

static void test_query_process_image_info(void)
{
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress );
    NTSTATUS status;
    SECTION_IMAGE_INFORMATION info;
    ULONG len;

    status = NtQueryInformationProcess( NULL, ProcessImageInformation, &info, sizeof(info), &len );
    ok( status == STATUS_INVALID_HANDLE || broken(status == STATUS_INVALID_PARAMETER), /* winxp */
        "got %08lx\n", status);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessImageInformation, &info, sizeof(info)-1, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %08lx\n", status);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessImageInformation, &info, sizeof(info)+1, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %08lx\n", status);

    memset( &info, 0xcc, sizeof(info) );
    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessImageInformation, &info, sizeof(info), &len );
    ok( status == STATUS_SUCCESS, "got %08lx\n", status);
    ok( len == sizeof(info), "wrong len %lu\n", len );

    ok( info.MajorSubsystemVersion == nt->OptionalHeader.MajorSubsystemVersion,
        "wrong major version %x/%x\n",
        info.MajorSubsystemVersion, nt->OptionalHeader.MajorSubsystemVersion );
    ok( info.MinorSubsystemVersion == nt->OptionalHeader.MinorSubsystemVersion,
        "wrong minor version %x/%x\n",
        info.MinorSubsystemVersion, nt->OptionalHeader.MinorSubsystemVersion );
    ok( info.MajorOperatingSystemVersion == nt->OptionalHeader.MajorOperatingSystemVersion ||
        broken( !info.MajorOperatingSystemVersion ),  /* <= win8 */
        "wrong major OS version %x/%x\n",
        info.MajorOperatingSystemVersion, nt->OptionalHeader.MajorOperatingSystemVersion );
#ifdef __REACTOS__
    if (GetNTVersion() < _WIN32_WINNT_WIN7)
        ok( info.MinorOperatingSystemVersion == 0, "wrong minor version %x/%x\n", info.MinorOperatingSystemVersion, 0 );
    else
#endif
    ok( info.MinorOperatingSystemVersion == nt->OptionalHeader.MinorOperatingSystemVersion,
        "wrong minor OS version %x/%x\n",
        info.MinorOperatingSystemVersion, nt->OptionalHeader.MinorOperatingSystemVersion );
}

static void test_query_process_debug_object_handle(int argc, char **argv)
{
    char cmdline[MAX_PATH];
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi;
    BOOL ret;
    HANDLE debug_object;
    NTSTATUS status;
    ULONG len;

    sprintf(cmdline, "%s %s %s", argv[0], argv[1], "debuggee");

    si.cb = sizeof(si);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL,
                        NULL, &si, &pi);
    ok(ret, "CreateProcess failed with last error %lu\n", GetLastError());
    if (!ret) return;

    len = 0xdeadbeef;
    status = NtQueryInformationProcess(NULL, ProcessDebugObjectHandle, NULL, 0, &len);
    ok(status == STATUS_INFO_LENGTH_MISMATCH,
       "Expected NtQueryInformationProcess to return STATUS_INFO_LENGTH_MISMATCH, got 0x%08lx\n",
       status);
    ok(len == 0xdeadbeef || broken(len == 0xfffffffc || len == 0xffc), /* wow64 */
       "len set to %lx\n", len );

    len = 0xdeadbeef;
    status = NtQueryInformationProcess(NULL, ProcessDebugObjectHandle, NULL, sizeof(debug_object), &len);
    ok(status == STATUS_INVALID_HANDLE ||
       status == STATUS_ACCESS_VIOLATION, /* XP */
       "Expected NtQueryInformationProcess to return STATUS_INVALID_HANDLE, got 0x%08lx\n", status);
    ok(len == 0xdeadbeef || broken(len == 0xfffffffc || len == 0xffc), /* wow64 */
       "len set to %lx\n", len );

    status = NtQueryInformationProcess(GetCurrentProcess(),
            ProcessDebugObjectHandle, NULL, sizeof(debug_object), &len);
    ok(status == STATUS_ACCESS_VIOLATION,
       "Expected NtQueryInformationProcess to return STATUS_ACCESS_VIOLATION, got 0x%08lx\n", status);
    ok(len == 0xdeadbeef || broken(len == 0xfffffffc || len == 0xffc), /* wow64 */
       "len set to %lx\n", len );

    status = NtQueryInformationProcess(NULL, ProcessDebugObjectHandle,
            &debug_object, sizeof(debug_object), NULL);
    ok(status == STATUS_INVALID_HANDLE,
       "Expected NtQueryInformationProcess to return STATUS_ACCESS_VIOLATION, got 0x%08lx\n", status);

    len = 0xdeadbeef;
    status = NtQueryInformationProcess(GetCurrentProcess(),
            ProcessDebugObjectHandle, &debug_object, sizeof(debug_object) - 1, &len);
    ok(status == STATUS_INFO_LENGTH_MISMATCH,
       "Expected NtQueryInformationProcess to return STATUS_INFO_LENGTH_MISMATCH, got 0x%08lx\n", status);
    ok(len == 0xdeadbeef || broken(len == 0xfffffffc || len == 0xffc), /* wow64 */
       "len set to %lx\n", len );

    len = 0xdeadbeef;
    status = NtQueryInformationProcess(GetCurrentProcess(),
            ProcessDebugObjectHandle, &debug_object, sizeof(debug_object) + 1, &len);
    ok(status == STATUS_INFO_LENGTH_MISMATCH,
       "Expected NtQueryInformationProcess to return STATUS_INFO_LENGTH_MISMATCH, got 0x%08lx\n", status);
    ok(len == 0xdeadbeef || broken(len == 0xfffffffc || len == 0xffc), /* wow64 */
       "len set to %lx\n", len );

    len = 0xdeadbeef;
    debug_object = (HANDLE)0xdeadbeef;
    status = NtQueryInformationProcess(GetCurrentProcess(),
            ProcessDebugObjectHandle, &debug_object,
            sizeof(debug_object), &len);
    ok(status == STATUS_PORT_NOT_SET,
       "Expected NtQueryInformationProcess to return STATUS_PORT_NOT_SET, got 0x%08lx\n", status);
    ok(debug_object == NULL ||
       broken(debug_object == (HANDLE)0xdeadbeef), /* Wow64 */
       "Expected debug object handle to be NULL, got %p\n", debug_object);
    ok(len == sizeof(debug_object), "len set to %lx\n", len );

    len = 0xdeadbeef;
    debug_object = (HANDLE)0xdeadbeef;
    status = NtQueryInformationProcess(pi.hProcess, ProcessDebugObjectHandle,
            &debug_object, sizeof(debug_object), &len);
    ok(status == STATUS_SUCCESS,
       "Expected NtQueryInformationProcess to return STATUS_SUCCESS, got 0x%08lx\n", status);
    ok(debug_object != NULL,
       "Expected debug object handle to be non-NULL, got %p\n", debug_object);
    ok(len == sizeof(debug_object), "len set to %lx\n", len );
    status = NtClose( debug_object );
    ok( !status, "NtClose failed %lx\n", status );

    for (;;)
    {
        DEBUG_EVENT ev;

        ret = WaitForDebugEvent(&ev, INFINITE);
        ok(ret, "WaitForDebugEvent failed with last error %lu\n", GetLastError());
        if (!ret) break;

        if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) break;

        ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
        ok(ret, "ContinueDebugEvent failed with last error %lu\n", GetLastError());
        if (!ret) break;
    }

    ret = CloseHandle(pi.hThread);
    ok(ret, "CloseHandle failed with last error %lu\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "CloseHandle failed with last error %lu\n", GetLastError());
}

static void test_query_process_debug_flags(int argc, char **argv)
{
    static const DWORD test_flags[] = { DEBUG_PROCESS,
                                        DEBUG_ONLY_THIS_PROCESS,
                                        DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS,
                                        CREATE_SUSPENDED };
    DWORD debug_flags = 0xdeadbeef;
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    NTSTATUS status;
    DEBUG_EVENT ev;
    DWORD result;
    BOOL ret;
    int i, j;

    /* test invalid arguments */
    status = NtQueryInformationProcess(NULL, ProcessDebugFlags, NULL, 0, NULL);
    ok(status == STATUS_INFO_LENGTH_MISMATCH || broken(status == STATUS_INVALID_INFO_CLASS) /* WOW64 */,
            "Expected STATUS_INFO_LENGTH_MISMATCH, got %#lx.\n", status);

    status = NtQueryInformationProcess(NULL, ProcessDebugFlags, NULL, sizeof(debug_flags), NULL);
    ok(status == STATUS_INVALID_HANDLE || status == STATUS_ACCESS_VIOLATION || broken(status == STATUS_INVALID_INFO_CLASS) /* WOW64 */,
            "Expected STATUS_INVALID_HANDLE, got %#lx.\n", status);

    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessDebugFlags,
            NULL, sizeof(debug_flags), NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %#lx.\n", status);

    status = NtQueryInformationProcess(NULL, ProcessDebugFlags,
            &debug_flags, sizeof(debug_flags), NULL);
    ok(status == STATUS_INVALID_HANDLE || broken(status == STATUS_INVALID_INFO_CLASS) /* WOW64 */,
            "Expected STATUS_INVALID_HANDLE, got %#lx.\n", status);

    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessDebugFlags,
            &debug_flags, sizeof(debug_flags) - 1, NULL);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %#lx.\n", status);

    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessDebugFlags,
            &debug_flags, sizeof(debug_flags) + 1, NULL);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %#lx.\n", status);

    /* test ProcessDebugFlags of current process */
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessDebugFlags,
            &debug_flags, sizeof(debug_flags), NULL);
    ok(!status, "NtQueryInformationProcess failed, status %#lx.\n", status);
    ok(debug_flags == TRUE, "Expected flag TRUE, got %lx.\n", debug_flags);

    for (i = 0; i < ARRAY_SIZE(test_flags); i++)
    {
        DWORD expected_flags = !(test_flags[i] & DEBUG_ONLY_THIS_PROCESS);
        sprintf(cmdline, "%s %s %s", argv[0], argv[1], "debuggee");

        si.cb = sizeof(si);
        ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, test_flags[i], NULL, NULL, &si, &pi);
        ok(ret, "CreateProcess failed, last error %#lx.\n", GetLastError());

        if (!(test_flags[i] & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)))
        {
            /* test ProcessDebugFlags before attaching with debugger */
            status = NtQueryInformationProcess(pi.hProcess, ProcessDebugFlags,
                    &debug_flags, sizeof(debug_flags), NULL);
            ok(!status, "NtQueryInformationProcess failed, status %#lx.\n", status);
            ok(debug_flags == TRUE, "Expected flag TRUE, got %lx.\n", debug_flags);

            ret = DebugActiveProcess(pi.dwProcessId);
            ok(ret, "DebugActiveProcess failed, last error %#lx.\n", GetLastError());
            expected_flags = FALSE;
        }

        /* test ProcessDebugFlags after attaching with debugger */
        status = NtQueryInformationProcess(pi.hProcess, ProcessDebugFlags,
                &debug_flags, sizeof(debug_flags), NULL);
        ok(!status, "NtQueryInformationProcess failed, status %#lx.\n", status);
        ok(debug_flags == expected_flags, "Expected flag %lx, got %lx.\n", expected_flags, debug_flags);

        if (!(test_flags[i] & CREATE_SUSPENDED))
        {
            /* Continue a couple of times to make sure the process is fully initialized,
             * otherwise Windows XP deadlocks in the following DebugActiveProcess(). */
            for (;;)
            {
                ret = WaitForDebugEvent(&ev, 1000);
                ok(ret, "WaitForDebugEvent failed, last error %#lx.\n", GetLastError());
                if (!ret) break;

                if (ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) break;

                ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
                ok(ret, "ContinueDebugEvent failed, last error %#lx.\n", GetLastError());
                if (!ret) break;
            }

            result = SuspendThread(pi.hThread);
            ok(result == 0, "Expected 0, got %lu.\n", result);
        }

        ret = DebugActiveProcessStop(pi.dwProcessId);
        ok(ret, "DebugActiveProcessStop failed, last error %#lx.\n", GetLastError());

        /* test ProcessDebugFlags after detaching debugger */
        status = NtQueryInformationProcess(pi.hProcess, ProcessDebugFlags,
                &debug_flags, sizeof(debug_flags), NULL);
        ok(!status, "NtQueryInformationProcess failed, status %#lx.\n", status);
        ok(debug_flags == expected_flags, "Expected flag %lx, got %lx.\n", expected_flags, debug_flags);

        ret = DebugActiveProcess(pi.dwProcessId);
        ok(ret, "DebugActiveProcess failed, last error %#lx.\n", GetLastError());

        /* test ProcessDebugFlags after re-attaching debugger */
        status = NtQueryInformationProcess(pi.hProcess, ProcessDebugFlags,
                &debug_flags, sizeof(debug_flags), NULL);
        ok(!status, "NtQueryInformationProcess failed, status %#lx.\n", status);
        ok(debug_flags == FALSE, "Expected flag FALSE, got %lx.\n", debug_flags);

        result = ResumeThread(pi.hThread);
        todo_wine ok(result == 2, "Expected 2, got %lu.\n", result);

        /* Wait until the process is terminated. On Windows XP the process randomly
         * gets stuck in a non-continuable exception, so stop after 100 iterations.
         * On Windows 2003, the debugged process disappears (or stops?) without
         * any EXIT_PROCESS_DEBUG_EVENT after a couple of events. */
        for (j = 0; j < 100; j++)
        {
            ret = WaitForDebugEvent(&ev, 1000);
            ok(ret || broken(GetLastError() == ERROR_SEM_TIMEOUT),
                "WaitForDebugEvent failed, last error %#lx.\n", GetLastError());
            if (!ret) break;

            if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) break;

            ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
            ok(ret, "ContinueDebugEvent failed, last error %#lx.\n", GetLastError());
            if (!ret) break;
        }
        ok(j < 100 || broken(j >= 100) /* Win XP */, "Expected less than 100 debug events.\n");

        /* test ProcessDebugFlags after process has terminated */
        status = NtQueryInformationProcess(pi.hProcess, ProcessDebugFlags,
                &debug_flags, sizeof(debug_flags), NULL);
        ok(!status, "NtQueryInformationProcess failed, status %#lx.\n", status);
        ok(debug_flags == FALSE, "Expected flag FALSE, got %lx.\n", debug_flags);

        ret = CloseHandle(pi.hThread);
        ok(ret, "CloseHandle failed, last error %#lx.\n", GetLastError());
        ret = CloseHandle(pi.hProcess);
        ok(ret, "CloseHandle failed, last error %#lx.\n", GetLastError());
    }
}

static void test_query_process_quota_limits(void)
{
    QUOTA_LIMITS qlimits;
    NTSTATUS status;
    HANDLE process;
    ULONG ret_len;

    status = NtQueryInformationProcess(NULL, ProcessQuotaLimits, NULL, sizeof(qlimits), NULL);
    ok(status == STATUS_INVALID_HANDLE, "NtQueryInformationProcess failed, status %#lx.\n", status);

    status = NtQueryInformationProcess(NULL, ProcessQuotaLimits, &qlimits, sizeof(qlimits), NULL);
    ok(status == STATUS_INVALID_HANDLE, "NtQueryInformationProcess failed, status %#lx.\n", status);

    process = GetCurrentProcess();
    status = NtQueryInformationProcess( process, ProcessQuotaLimits, &qlimits, 2, &ret_len);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryInformationProcess failed, status %#lx.\n", status);

    memset(&qlimits, 0, sizeof(qlimits));
    status = NtQueryInformationProcess( process, ProcessQuotaLimits, &qlimits, sizeof(qlimits), &ret_len);
    ok(status == STATUS_SUCCESS, "NtQueryInformationProcess failed, status %#lx.\n", status);
    ok(sizeof(qlimits) == ret_len, "len set to %lx\n", ret_len);
    ok(qlimits.MinimumWorkingSetSize == 204800,"Expected MinimumWorkingSetSize = 204800, got %s\n",
        wine_dbgstr_longlong(qlimits.MinimumWorkingSetSize));
    ok(qlimits.MaximumWorkingSetSize == 1413120,"Expected MaximumWorkingSetSize = 1413120, got %s\n",
        wine_dbgstr_longlong(qlimits.MaximumWorkingSetSize));
    ok(qlimits.PagefileLimit == ~0,"Expected PagefileLimit = ~0, got %s\n",
        wine_dbgstr_longlong(qlimits.PagefileLimit));
    ok(qlimits.TimeLimit.QuadPart == ~0,"Expected TimeLimit = ~0, got %s\n",
        wine_dbgstr_longlong(qlimits.TimeLimit.QuadPart));

    if (winetest_debug > 1)
    {
        trace("Quota Limits:\n");
        trace("PagedPoolLimit: %s\n", wine_dbgstr_longlong(qlimits.PagedPoolLimit));
        trace("NonPagedPoolLimit: %s\n", wine_dbgstr_longlong(qlimits.NonPagedPoolLimit));
    }

    memset(&qlimits, 0, sizeof(qlimits));
    status = NtQueryInformationProcess( process, ProcessQuotaLimits, &qlimits, sizeof(qlimits) * 2, &ret_len);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryInformationProcess failed, status %#lx.\n", status);
    ok(sizeof(qlimits) == ret_len, "len set to %lx\n", ret_len);

    memset(&qlimits, 0, sizeof(qlimits));
    status = NtQueryInformationProcess( process, ProcessQuotaLimits, &qlimits, sizeof(qlimits) - 1, &ret_len);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryInformationProcess failed, status %#lx.\n", status);
    ok(sizeof(qlimits) == ret_len, "len set to %lx\n", ret_len);

    memset(&qlimits, 0, sizeof(qlimits));
    status = NtQueryInformationProcess( process, ProcessQuotaLimits, &qlimits, sizeof(qlimits) + 1, &ret_len);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryInformationProcess failed, status %#lx.\n", status);
    ok(sizeof(qlimits) == ret_len, "len set to %lx\n", ret_len);
}

static void test_readvirtualmemory(void)
{
    HANDLE process;
    NTSTATUS status;
    SIZE_T readcount;
    static const char teststring[] = "test string";
    char buffer[12];

    process = OpenProcess(PROCESS_VM_READ, FALSE, GetCurrentProcessId());
    ok(process != 0, "Expected to be able to open own process for reading memory\n");

    /* normal operation */
    status = pNtReadVirtualMemory(process, teststring, buffer, 12, &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( readcount == 12, "Expected to read 12 bytes, got %Id\n",readcount);
    ok( strcmp(teststring, buffer) == 0, "Expected read memory to be the same as original memory\n");

    /* no number of bytes */
    memset(buffer, 0, 12);
    status = pNtReadVirtualMemory(process, teststring, buffer, 12, NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( strcmp(teststring, buffer) == 0, "Expected read memory to be the same as original memory\n");

    /* illegal remote address */
    todo_wine{
    status = pNtReadVirtualMemory(process, (void *) 0x1234, buffer, 12, &readcount);
    ok( status == STATUS_PARTIAL_COPY, "Expected STATUS_PARTIAL_COPY, got %08lx\n", status);
    if (status == STATUS_PARTIAL_COPY)
        ok( readcount == 0, "Expected to read 0 bytes, got %Id\n",readcount);
    }

    /* 0 handle */
    status = pNtReadVirtualMemory(0, teststring, buffer, 12, &readcount);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);
    ok( readcount == 0, "Expected to read 0 bytes, got %Id\n",readcount);

    /* pseudo handle for current process*/
    memset(buffer, 0, 12);
    status = pNtReadVirtualMemory((HANDLE)-1, teststring, buffer, 12, &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( readcount == 12, "Expected to read 12 bytes, got %Id\n",readcount);
    ok( strcmp(teststring, buffer) == 0, "Expected read memory to be the same as original memory\n");

    /* illegal local address */
    status = pNtReadVirtualMemory(process, teststring, (void *)0x1234, 12, &readcount);
    ok( status == STATUS_ACCESS_VIOLATION || broken(status == STATUS_PARTIAL_COPY) /* Win10 */,
        "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", status);
    if (status == STATUS_ACCESS_VIOLATION)
        ok( readcount == 0, "Expected to read 0 bytes, got %Id\n",readcount);

    CloseHandle(process);
}

static void test_mapprotection(void)
{
    HANDLE h;
    void* addr;
    MEMORY_BASIC_INFORMATION info;
    ULONG oldflags, flagsize, flags = MEM_EXECUTE_OPTION_ENABLE;
    LARGE_INTEGER size, offset;
    NTSTATUS status;
    SIZE_T retlen, count;
    void (*f)(void);
    BOOL reset_flags = FALSE;

    /* Switch to being a noexec unaware process */
    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &oldflags, sizeof (oldflags), &flagsize);
    if (status == STATUS_INVALID_PARAMETER)
    {
        skip("Unable to query process execute flags on this platform\n");
        return;
    }
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status );
    if (winetest_debug > 1) trace("Process execute flags %08lx\n", oldflags);

    if (!(oldflags & MEM_EXECUTE_OPTION_ENABLE))
    {
        if (oldflags & MEM_EXECUTE_OPTION_PERMANENT)
        {
            skip("Unable to turn off noexec\n");
            return;
        }

        if (pGetSystemDEPPolicy && pGetSystemDEPPolicy() == AlwaysOn)
        {
            skip("System policy requires noexec\n");
            return;
        }

        status = pNtSetInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &flags, sizeof(flags) );
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status );
        reset_flags = TRUE;
    }

    size.u.LowPart  = 0x2000;
    size.u.HighPart = 0;
    status = pNtCreateSection ( &h,
        STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE,
        NULL,
        &size,
        PAGE_READWRITE,
        SEC_COMMIT | SEC_NOCACHE,
        0
    );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    offset.u.LowPart  = 0;
    offset.u.HighPart = 0;
    count = 0x2000;
    addr = NULL;
    status = pNtMapViewOfSection ( h, GetCurrentProcess(), &addr, 0, 0, &offset, &count, ViewShare, 0, PAGE_READWRITE);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

#if defined(__x86_64__) || defined(__i386__)
    *(unsigned char*)addr = 0xc3;       /* lret ... in both i386 and x86_64 */
#elif defined(__arm__)
    *(unsigned long*)addr = 0xe12fff1e; /* bx lr */
#elif defined(__aarch64__)
    *(unsigned long*)addr = 0xd65f03c0; /* ret */
#else
    ok(0, "Add a return opcode for your architecture or expect a crash in this test\n");
#endif
    if (winetest_debug > 1) trace("trying to execute code in the readwrite only mapped anon file...\n");
    f = addr;f();
    if (winetest_debug > 1) trace("...done.\n");

    status = pNtQueryVirtualMemory( GetCurrentProcess(), addr, MemoryBasicInformation, &info, sizeof(info), &retlen );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( retlen == sizeof(info), "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok((info.Protect & ~PAGE_NOCACHE) == PAGE_READWRITE, "addr.Protect is not PAGE_READWRITE, but 0x%lx\n", info.Protect);

    status = pNtUnmapViewOfSection( GetCurrentProcess(), (char *)addr + 0x1050 );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    pNtClose (h);

    if (reset_flags)
        pNtSetInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &oldflags, sizeof(oldflags) );
}

static void test_threadstack(void)
{
    PROCESS_STACK_ALLOCATION_INFORMATION info = { 0x100000, 0, (void *)0xdeadbeef };
    PROCESS_STACK_ALLOCATION_INFORMATION_EX info_ex = { 0 };
    MEMORY_BASIC_INFORMATION meminfo;
    SIZE_T retlen;
    NTSTATUS status;

    info.ReserveSize = 0x100000;
    info.StackBase = (void *)0xdeadbeef;
    status = pNtSetInformationProcess( GetCurrentProcess(), ProcessThreadStackAllocation, &info, sizeof(info) );
    ok( !status, "NtSetInformationProcess failed %08lx\n", status );
    ok( info.StackBase != (void *)0xdeadbeef, "stackbase not set\n" );

    status = pNtQueryVirtualMemory( GetCurrentProcess(), info.StackBase, MemoryBasicInformation,
                                    &meminfo, sizeof(meminfo), &retlen );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( retlen == sizeof(meminfo), "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( meminfo.AllocationBase == info.StackBase, "wrong base %p/%p\n",
        meminfo.AllocationBase, info.StackBase );
    ok( meminfo.RegionSize == info.ReserveSize, "wrong size %Ix/%Ix\n",
        meminfo.RegionSize, info.ReserveSize );
    ok( meminfo.State == MEM_RESERVE, "wrong state %lx\n", meminfo.State );
    ok( meminfo.Protect == 0, "wrong protect %lx\n", meminfo.Protect );
    ok( meminfo.Type == MEM_PRIVATE, "wrong type %lx\n", meminfo.Type );

    info_ex.AllocInfo = info;
    status = pNtSetInformationProcess( GetCurrentProcess(), ProcessThreadStackAllocation,
                                       &info_ex, sizeof(info_ex) );
    if (status != STATUS_INVALID_PARAMETER)
    {
        ok( !status, "NtSetInformationProcess failed %08lx\n", status );
        ok( info_ex.AllocInfo.StackBase != info.StackBase, "stackbase not set\n" );
        status = pNtQueryVirtualMemory( GetCurrentProcess(), info_ex.AllocInfo.StackBase,
                                        MemoryBasicInformation, &meminfo, sizeof(meminfo), &retlen );
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( retlen == sizeof(meminfo), "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( meminfo.AllocationBase == info_ex.AllocInfo.StackBase, "wrong base %p/%p\n",
            meminfo.AllocationBase, info_ex.AllocInfo.StackBase );
        ok( meminfo.RegionSize == info_ex.AllocInfo.ReserveSize, "wrong size %Ix/%Ix\n",
            meminfo.RegionSize, info_ex.AllocInfo.ReserveSize );
        ok( meminfo.State == MEM_RESERVE, "wrong state %lx\n", meminfo.State );
        ok( meminfo.Protect == 0, "wrong protect %lx\n", meminfo.Protect );
        ok( meminfo.Type == MEM_PRIVATE, "wrong type %lx\n", meminfo.Type );
        VirtualFree( info_ex.AllocInfo.StackBase, 0, MEM_FREE );
        status = pNtSetInformationProcess( GetCurrentProcess(), ProcessThreadStackAllocation,
                                           &info, sizeof(info) - 1 );
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtSetInformationProcess failed %08lx\n", status );
        status = pNtSetInformationProcess( GetCurrentProcess(), ProcessThreadStackAllocation,
                                           &info, sizeof(info) + 1 );
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtSetInformationProcess failed %08lx\n", status );
        status = pNtSetInformationProcess( GetCurrentProcess(), ProcessThreadStackAllocation,
                                           &info_ex, sizeof(info_ex) - 1 );
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtSetInformationProcess failed %08lx\n", status );
        status = pNtSetInformationProcess( GetCurrentProcess(), ProcessThreadStackAllocation,
                                           &info_ex, sizeof(info_ex) + 1 );
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtSetInformationProcess failed %08lx\n", status );
    }
    else win_skip( "ProcessThreadStackAllocation ex not supported\n" );

    VirtualFree( info.StackBase, 0, MEM_FREE );
}

static void test_queryvirtualmemory(void)
{
    NTSTATUS status;
    SIZE_T readcount, prev;
    static const char teststring[] = "test string";
    static char datatestbuf[42] = "abc";
    static char rwtestbuf[42];
    MEMORY_BASIC_INFORMATION mbi;
    char stackbuf[42];
    HMODULE module;
    void *user_shared_data = (void *)0x7ffe0000;
    void *buffer[256];
    MEMORY_SECTION_NAME *name = (MEMORY_SECTION_NAME *)buffer;
    SYSTEM_BASIC_INFORMATION sbi;

    module = GetModuleHandleA( "ntdll.dll" );
    status = pNtQueryVirtualMemory(NtCurrentProcess(), module, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %Id\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok (mbi.AllocationBase == module, "mbi.AllocationBase is 0x%p, expected 0x%p\n", mbi.AllocationBase, module);
    ok (mbi.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "mbi.AllocationProtect is 0x%lx, expected 0x%x\n", mbi.AllocationProtect, PAGE_EXECUTE_WRITECOPY);
    ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%lx, expected 0x%x\n", mbi.State, MEM_COMMIT);
    ok (mbi.Protect == PAGE_READONLY, "mbi.Protect is 0x%lx, expected 0x%x\n", mbi.Protect, PAGE_READONLY);
    ok (mbi.Type == MEM_IMAGE, "mbi.Type is 0x%lx, expected 0x%x\n", mbi.Type, MEM_IMAGE);

    module = GetModuleHandleA( "ntdll.dll" );
    status = pNtQueryVirtualMemory(NtCurrentProcess(), pNtQueryVirtualMemory, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %Id\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok (mbi.AllocationBase == module, "mbi.AllocationBase is 0x%p, expected 0x%p\n", mbi.AllocationBase, module);
    ok (mbi.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "mbi.AllocationProtect is 0x%lx, expected 0x%x\n", mbi.AllocationProtect, PAGE_EXECUTE_WRITECOPY);
    ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%lx, expected 0x%x\n", mbi.State, MEM_COMMIT);
    ok (mbi.Protect == PAGE_EXECUTE_READ, "mbi.Protect is 0x%lx, expected 0x%x\n", mbi.Protect, PAGE_EXECUTE_READ);

    status = pNtQueryVirtualMemory(NtCurrentProcess(), GetProcessHeap(), MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %Id\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok (mbi.AllocationProtect == PAGE_READWRITE || mbi.AllocationProtect == PAGE_EXECUTE_READWRITE,
        "mbi.AllocationProtect is 0x%lx\n", mbi.AllocationProtect);
    ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%lx, expected 0x%x\n", mbi.State, MEM_COMMIT);
    ok (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READWRITE,
        "mbi.Protect is 0x%lx\n", mbi.Protect);

    status = pNtQueryVirtualMemory(NtCurrentProcess(), stackbuf, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %Id\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok (mbi.AllocationProtect == PAGE_READWRITE, "mbi.AllocationProtect is 0x%lx, expected 0x%x\n", mbi.AllocationProtect, PAGE_READWRITE);
    ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%lx, expected 0x%x\n", mbi.State, MEM_COMMIT);
    ok (mbi.Protect == PAGE_READWRITE, "mbi.Protect is 0x%lx, expected 0x%x\n", mbi.Protect, PAGE_READWRITE);

    module = GetModuleHandleA( NULL );
    status = pNtQueryVirtualMemory(NtCurrentProcess(), teststring, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %Id\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok (mbi.AllocationBase == module, "mbi.AllocationBase is 0x%p, expected 0x%p\n", mbi.AllocationBase, module);
    ok (mbi.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "mbi.AllocationProtect is 0x%lx, expected 0x%x\n", mbi.AllocationProtect, PAGE_EXECUTE_WRITECOPY);
    ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%lx, expected 0x%X\n", mbi.State, MEM_COMMIT);
    ok (mbi.Protect == PAGE_READONLY, "mbi.Protect is 0x%lx, expected 0x%X\n", mbi.Protect, PAGE_READONLY);

    status = pNtQueryVirtualMemory(NtCurrentProcess(), datatestbuf, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %Id\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok (mbi.AllocationBase == module, "mbi.AllocationBase is 0x%p, expected 0x%p\n", mbi.AllocationBase, module);
    ok (mbi.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "mbi.AllocationProtect is 0x%lx, expected 0x%x\n", mbi.AllocationProtect, PAGE_EXECUTE_WRITECOPY);
    ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%lx, expected 0x%X\n", mbi.State, MEM_COMMIT);
    ok (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_WRITECOPY,
        "mbi.Protect is 0x%lx\n", mbi.Protect);

    status = pNtQueryVirtualMemory(NtCurrentProcess(), rwtestbuf, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %Id\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    if (mbi.AllocationBase == module)
    {
        ok (mbi.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "mbi.AllocationProtect is 0x%lx, expected 0x%x\n", mbi.AllocationProtect, PAGE_EXECUTE_WRITECOPY);
        ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%lx, expected 0x%X\n", mbi.State, MEM_COMMIT);
        ok (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_WRITECOPY,
            "mbi.Protect is 0x%lx\n", mbi.Protect);
    }
    else skip( "bss is outside of module\n" );  /* this can happen on Mac OS */

    status = pNtQueryVirtualMemory(NtCurrentProcess(), user_shared_data, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok(readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %Id\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok(mbi.AllocationBase == user_shared_data, "mbi.AllocationBase is 0x%p, expected 0x%p\n", mbi.AllocationBase, user_shared_data);
    ok(mbi.AllocationProtect == PAGE_READONLY, "mbi.AllocationProtect is 0x%lx, expected 0x%x\n", mbi.AllocationProtect, PAGE_READONLY);
    ok(mbi.State == MEM_COMMIT, "mbi.State is 0x%lx, expected 0x%X\n", mbi.State, MEM_COMMIT);
    ok(mbi.Protect == PAGE_READONLY, "mbi.Protect is 0x%lx\n", mbi.Protect);
    ok(mbi.Type == MEM_PRIVATE, "mbi.Type is 0x%lx, expected 0x%x\n", mbi.Type, MEM_PRIVATE);
    ok(mbi.RegionSize == 0x1000, "mbi.RegionSize is 0x%Ix, expected 0x%x\n", mbi.RegionSize, 0x1000);

    /* check error code when addr is higher than user space limit */
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), NULL);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    status = pNtQueryVirtualMemory(NtCurrentProcess(), sbi.LowestUserAddress, MemoryBasicInformation, &mbi, sizeof(mbi), &readcount);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    status = pNtQueryVirtualMemory(NtCurrentProcess(), (char *)sbi.LowestUserAddress-1, MemoryBasicInformation, &mbi, sizeof(mbi), &readcount);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    status = pNtQueryVirtualMemory(NtCurrentProcess(), sbi.HighestUserAddress, MemoryBasicInformation, &mbi, sizeof(mbi), &readcount);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    status = pNtQueryVirtualMemory(NtCurrentProcess(), (char *)sbi.HighestUserAddress+1, MemoryBasicInformation, &mbi, sizeof(mbi), &readcount);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %08lx\n", status);
    status = pNtQueryVirtualMemory(NtCurrentProcess(), (void *)~0, MemoryBasicInformation, &mbi, sizeof(mbi), &readcount);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %08lx\n", status);

    /* check error code when len is less than MEMORY_BASIC_INFORMATION size */
    status = pNtQueryVirtualMemory(NtCurrentProcess(), GetProcessHeap(), MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION) - 1, &readcount);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    module = GetModuleHandleA( "ntdll.dll" );
    memset(buffer, 0xcc, sizeof(buffer));
    readcount = 0xdeadbeef;
    status = pNtQueryVirtualMemory(NtCurrentProcess(), module, MemoryMappedFilenameInformation,
                                   name, sizeof(*name) + 16, &readcount);
    ok(status == STATUS_BUFFER_OVERFLOW, "got %08lx\n", status);
    ok(name->SectionFileName.Length == 0xcccc || broken(!name->SectionFileName.Length),  /* vista64 */
       "Wrong len %u\n", name->SectionFileName.Length);
    ok(readcount > sizeof(*name), "Wrong count %Iu\n", readcount);

    memset(buffer, 0xcc, sizeof(buffer));
    readcount = 0xdeadbeef;
    status = pNtQueryVirtualMemory(NtCurrentProcess(), (char *)module + 1234, MemoryMappedFilenameInformation,
                                   name, sizeof(buffer), &readcount);
    ok(status == STATUS_SUCCESS, "got %08lx\n", status);
    ok(name->SectionFileName.Buffer == (WCHAR *)(name + 1), "Wrong ptr %p/%p\n",
       name->SectionFileName.Buffer, name + 1 );
    ok(name->SectionFileName.Length != 0xcccc, "Wrong len %u\n", name->SectionFileName.Length);
    ok(name->SectionFileName.MaximumLength == name->SectionFileName.Length + sizeof(WCHAR),
       "Wrong maxlen %u/%u\n", name->SectionFileName.MaximumLength, name->SectionFileName.Length);
    ok(readcount == sizeof(name->SectionFileName) + name->SectionFileName.MaximumLength,
       "Wrong count %Iu/%u\n", readcount, name->SectionFileName.MaximumLength);
    ok( !name->SectionFileName.Buffer[name->SectionFileName.Length / sizeof(WCHAR)],
        "buffer not null-terminated\n" );

    memset(buffer, 0xcc, sizeof(buffer));
    status = pNtQueryVirtualMemory(NtCurrentProcess(), (char *)module + 1234, MemoryMappedFilenameInformation,
                                   name, sizeof(buffer), NULL);
    ok(status == STATUS_SUCCESS, "got %08lx\n", status);

    status = pNtQueryVirtualMemory(NtCurrentProcess(), (char *)module + 1234, MemoryMappedFilenameInformation,
                                   NULL, sizeof(buffer), NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "got %08lx\n", status);

    memset(buffer, 0xcc, sizeof(buffer));
    prev = readcount;
    readcount = 0xdeadbeef;
    status = pNtQueryVirtualMemory(NtCurrentProcess(), (char *)module + 321, MemoryMappedFilenameInformation,
                                   name, sizeof(*name) - 1, &readcount);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got %08lx\n", status);
    ok(name->SectionFileName.Length == 0xcccc, "Wrong len %u\n", name->SectionFileName.Length);
    ok(readcount == prev, "Wrong count %Iu\n", readcount);

    memset(buffer, 0xcc, sizeof(buffer));
    readcount = 0xdeadbeef;
    status = pNtQueryVirtualMemory((HANDLE)0xdead, (char *)module + 1234, MemoryMappedFilenameInformation,
                                   name, sizeof(buffer), &readcount);
    ok(status == STATUS_INVALID_HANDLE, "got %08lx\n", status);
    ok(readcount == 0xdeadbeef || broken(readcount == 1024 + sizeof(*name)), /* wow64 */
       "Wrong count %Iu\n", readcount);

    memset(buffer, 0xcc, sizeof(buffer));
    readcount = 0xdeadbeef;
    status = pNtQueryVirtualMemory(NtCurrentProcess(), buffer, MemoryMappedFilenameInformation,
                                   name, sizeof(buffer), &readcount);
    ok(status == STATUS_INVALID_ADDRESS, "got %08lx\n", status);
    ok(name->SectionFileName.Length == 0xcccc, "Wrong len %u\n", name->SectionFileName.Length);
    ok(readcount == 0xdeadbeef || broken(readcount == 1024 + sizeof(*name)), /* wow64 */
       "Wrong count %Iu\n", readcount);

    readcount = 0xdeadbeef;
    status = pNtQueryVirtualMemory(NtCurrentProcess(), (void *)0x1234, MemoryMappedFilenameInformation,
                                   name, sizeof(buffer), &readcount);
    ok(status == STATUS_INVALID_ADDRESS, "got %08lx\n", status);
    ok(name->SectionFileName.Length == 0xcccc, "Wrong len %u\n", name->SectionFileName.Length);
    ok(readcount == 0xdeadbeef || broken(readcount == 1024 + sizeof(*name)), /* wow64 */
       "Wrong count %Iu\n", readcount);

    readcount = 0xdeadbeef;
    status = pNtQueryVirtualMemory(NtCurrentProcess(), (void *)0x1234, MemoryMappedFilenameInformation,
                                   name, sizeof(*name) - 1, &readcount);
    ok(status == STATUS_INVALID_ADDRESS, "got %08lx\n", status);
    ok(name->SectionFileName.Length == 0xcccc, "Wrong len %u\n", name->SectionFileName.Length);
    ok(readcount == 0xdeadbeef || broken(readcount == 15), /* wow64 */
       "Wrong count %Iu\n", readcount);
}

static void test_affinity(void)
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION pbi;
    DWORD_PTR proc_affinity, thread_affinity;
    THREAD_BASIC_INFORMATION tbi;
    SYSTEM_INFO si;

    GetSystemInfo(&si);
    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), NULL );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    proc_affinity = pbi.AffinityMask;
    ok( proc_affinity == get_affinity_mask( si.dwNumberOfProcessors ), "Unexpected process affinity\n" );
    if (si.dwNumberOfProcessors < 8 * sizeof(DWORD_PTR))
    {
        proc_affinity = (DWORD_PTR)1 << si.dwNumberOfProcessors;
        status = pNtSetInformationProcess( GetCurrentProcess(), ProcessAffinityMask, &proc_affinity, sizeof(proc_affinity) );
        ok( status == STATUS_INVALID_PARAMETER,
            "Expected STATUS_INVALID_PARAMETER, got %08lx\n", status);
    }
    proc_affinity = 0;
    status = pNtSetInformationProcess( GetCurrentProcess(), ProcessAffinityMask, &proc_affinity, sizeof(proc_affinity) );
    ok( status == STATUS_INVALID_PARAMETER,
        "Expected STATUS_INVALID_PARAMETER, got %08lx\n", status);

    status = pNtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( tbi.AffinityMask == get_affinity_mask( si.dwNumberOfProcessors ), "Unexpected thread affinity\n" );
    if (si.dwNumberOfProcessors < 8 * sizeof(DWORD_PTR))
    {
        thread_affinity = (DWORD_PTR)1 << si.dwNumberOfProcessors;
        status = pNtSetInformationThread( GetCurrentThread(), ThreadAffinityMask, &thread_affinity, sizeof(thread_affinity) );
        ok( status == STATUS_INVALID_PARAMETER,
            "Expected STATUS_INVALID_PARAMETER, got %08lx\n", status);
    }
    thread_affinity = 0;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadAffinityMask, &thread_affinity, sizeof(thread_affinity) );
    ok( status == STATUS_INVALID_PARAMETER,
        "Expected STATUS_INVALID_PARAMETER, got %08lx\n", status);

    thread_affinity = 1;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadAffinityMask, &thread_affinity, sizeof(thread_affinity) );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    status = pNtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( tbi.AffinityMask == 1, "Unexpected thread affinity\n" );

    /* NOTE: Pre-Vista does not allow bits to be set that are higher than the highest set bit in process affinity mask */
    thread_affinity = (pbi.AffinityMask << 1) | pbi.AffinityMask;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadAffinityMask, &thread_affinity, sizeof(thread_affinity) );
    ok( broken(status == STATUS_INVALID_PARAMETER) || (status == STATUS_SUCCESS), "Expected STATUS_SUCCESS, got %08lx\n", status );
    if (status == STATUS_SUCCESS)
    {
        status = pNtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status );
        ok( tbi.AffinityMask == pbi.AffinityMask, "Unexpected thread affinity. Expected %Ix, got %Ix\n", pbi.AffinityMask, tbi.AffinityMask );
    }

    thread_affinity = ~(DWORD_PTR)0 - 1;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadAffinityMask, &thread_affinity, sizeof(thread_affinity) );
    ok( broken(status == STATUS_INVALID_PARAMETER) || (status == STATUS_SUCCESS), "Expected STATUS_SUCCESS, got %08lx\n", status );
    if (status == STATUS_SUCCESS)
    {
        status = pNtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status );
        ok( tbi.AffinityMask == (pbi.AffinityMask & (~(DWORD_PTR)0 - 1)), "Unexpected thread affinity. Expected %Ix, got %Ix\n", pbi.AffinityMask & (~(DWORD_PTR)0 - 1), tbi.AffinityMask );
    }

    /* NOTE: Pre-Vista does not recognize the "all processors" flag (all bits set) */
    thread_affinity = ~(DWORD_PTR)0;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadAffinityMask, &thread_affinity, sizeof(thread_affinity) );
    ok( broken(status == STATUS_INVALID_PARAMETER) || status == STATUS_SUCCESS,
        "Expected STATUS_SUCCESS, got %08lx\n", status);

    if (si.dwNumberOfProcessors <= 1)
    {
        skip("only one processor, skipping affinity testing\n");
        return;
    }

    /* Test thread affinity mask resulting from "all processors" flag */
    if (status == STATUS_SUCCESS)
    {
        status = pNtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
        ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( tbi.AffinityMask == get_affinity_mask( si.dwNumberOfProcessors ), "unexpected affinity %#Ix\n", tbi.AffinityMask );
    }
    else
        skip("Cannot test thread affinity mask for 'all processors' flag\n");

    proc_affinity = 2;
    status = pNtSetInformationProcess( GetCurrentProcess(), ProcessAffinityMask, &proc_affinity, sizeof(proc_affinity) );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), NULL );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    proc_affinity = pbi.AffinityMask;
    ok( proc_affinity == 2, "Unexpected process affinity\n" );
    /* Setting the process affinity changes the thread affinity to match */
    status = pNtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( tbi.AffinityMask == 2, "Unexpected thread affinity\n" );
    /* The thread affinity is restricted to the process affinity */
    thread_affinity = 1;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadAffinityMask, &thread_affinity, sizeof(thread_affinity) );
    ok( status == STATUS_INVALID_PARAMETER,
        "Expected STATUS_INVALID_PARAMETER, got %08lx\n", status);

    proc_affinity = get_affinity_mask( si.dwNumberOfProcessors );
    status = pNtSetInformationProcess( GetCurrentProcess(), ProcessAffinityMask, &proc_affinity, sizeof(proc_affinity) );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    /* Resetting the process affinity also resets the thread affinity */
    status = pNtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( tbi.AffinityMask == get_affinity_mask( si.dwNumberOfProcessors ),
        "Unexpected thread affinity\n" );
}

static DWORD WINAPI hide_from_debugger_thread(void *arg)
{
    HANDLE stop_event = arg;
    WaitForSingleObject( stop_event, INFINITE );
    return 0;
}

static void test_HideFromDebugger(void)
{
    NTSTATUS status;
    HANDLE thread, stop_event;
    ULONG dummy;

    dummy = 0;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadHideFromDebugger, &dummy, sizeof(ULONG) );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status );
    dummy = 0;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadHideFromDebugger, &dummy, 1 );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status );
    status = pNtSetInformationThread( (HANDLE)0xdeadbeef, ThreadHideFromDebugger, NULL, 0 );
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status );
    status = pNtSetInformationThread( GetCurrentThread(), ThreadHideFromDebugger, NULL, 0 );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status );
    dummy = 0;
    status = NtQueryInformationThread( GetCurrentThread(), ThreadHideFromDebugger, &dummy, sizeof(ULONG), NULL );
    if (status == STATUS_INVALID_INFO_CLASS)
    {
        win_skip("ThreadHideFromDebugger not available\n");
        return;
    }

    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status );
    dummy = 0;
    status = NtQueryInformationThread( (HANDLE)0xdeadbeef, ThreadHideFromDebugger, &dummy, sizeof(ULONG), NULL );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status );
    dummy = 0;
    status = NtQueryInformationThread( GetCurrentThread(), ThreadHideFromDebugger, &dummy, 1, NULL );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status );
    ok( dummy == 1, "Expected dummy == 1, got %08lx\n", dummy );

    stop_event = CreateEventA( NULL, FALSE, FALSE, NULL );
    ok( stop_event != NULL, "CreateEvent failed\n" );
    thread = CreateThread( NULL, 0, hide_from_debugger_thread, stop_event, 0, NULL );
    ok( thread != INVALID_HANDLE_VALUE, "CreateThread failed with %ld\n", GetLastError() );

    dummy = 0;
    status = NtQueryInformationThread( thread, ThreadHideFromDebugger, &dummy, 1, NULL );
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );
    ok( dummy == 0, "Expected dummy == 0, got %08lx\n", dummy );

    status = pNtSetInformationThread( thread, ThreadHideFromDebugger, NULL, 0 );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status );

    dummy = 0;
    status = NtQueryInformationThread( thread, ThreadHideFromDebugger, &dummy, 1, NULL );
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );
    ok( dummy == 1, "Expected dummy == 1, got %08lx\n", dummy );

    SetEvent( stop_event );
    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );
    CloseHandle( stop_event );
}

static void test_NtGetCurrentProcessorNumber(void)
{
    NTSTATUS status;
    SYSTEM_INFO si;
    PROCESS_BASIC_INFORMATION pbi;
    THREAD_BASIC_INFORMATION tbi;
    DWORD_PTR old_process_mask;
    DWORD_PTR old_thread_mask;
    DWORD_PTR new_mask;
    ULONG current_cpu;
    ULONG i;

    if (!pNtGetCurrentProcessorNumber) {
        win_skip("NtGetCurrentProcessorNumber not available\n");
        return;
    }

    GetSystemInfo(&si);
    current_cpu = pNtGetCurrentProcessorNumber();
    if (winetest_debug > 1) trace("dwNumberOfProcessors: %ld, current processor: %ld\n", si.dwNumberOfProcessors, current_cpu);

    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    old_process_mask = pbi.AffinityMask;
    ok(status == STATUS_SUCCESS, "got 0x%lx (expected STATUS_SUCCESS)\n", status);

    status = pNtQueryInformationThread(GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL);
    old_thread_mask = tbi.AffinityMask;
    ok(status == STATUS_SUCCESS, "got 0x%lx (expected STATUS_SUCCESS)\n", status);

    /* allow the test to run on all processors */
    new_mask = get_affinity_mask( si.dwNumberOfProcessors );
    status = pNtSetInformationProcess(GetCurrentProcess(), ProcessAffinityMask, &new_mask, sizeof(new_mask));
    ok(status == STATUS_SUCCESS, "got 0x%lx (expected STATUS_SUCCESS)\n", status);

    for (i = 0; i < si.dwNumberOfProcessors; i++)
    {
        new_mask = (DWORD_PTR)1 << i;
        status = pNtSetInformationThread(GetCurrentThread(), ThreadAffinityMask, &new_mask, sizeof(new_mask));
        ok(status == STATUS_SUCCESS, "%ld: got 0x%lx (expected STATUS_SUCCESS)\n", i, status);

        status = pNtQueryInformationThread(GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL);
        ok(status == STATUS_SUCCESS, "%ld: got 0x%lx (expected STATUS_SUCCESS)\n", i, status);

        current_cpu = pNtGetCurrentProcessorNumber();
        ok((current_cpu == i), "%ld (new_mask 0x%Ix): running on processor %ld (AffinityMask: 0x%Ix)\n",
                                i, new_mask, current_cpu, tbi.AffinityMask);
    }

    /* restore old values */
    status = pNtSetInformationProcess(GetCurrentProcess(), ProcessAffinityMask, &old_process_mask, sizeof(old_process_mask));
    ok(status == STATUS_SUCCESS, "got 0x%lx (expected STATUS_SUCCESS)\n", status);

    status = pNtSetInformationThread(GetCurrentThread(), ThreadAffinityMask, &old_thread_mask, sizeof(old_thread_mask));
    ok(status == STATUS_SUCCESS, "got 0x%lx (expected STATUS_SUCCESS)\n", status);
}

static void test_ThreadEnableAlignmentFaultFixup(void)
{
    NTSTATUS status;
    ULONG dummy;

    dummy = 0;
    status = NtQueryInformationThread( GetCurrentThread(), ThreadEnableAlignmentFaultFixup, &dummy, sizeof(ULONG), NULL );
    ok( status == STATUS_INVALID_INFO_CLASS, "Expected STATUS_INVALID_INFO_CLASS, got %08lx\n", status );
    status = NtQueryInformationThread( GetCurrentThread(), ThreadEnableAlignmentFaultFixup, &dummy, 1, NULL );
    ok( status == STATUS_INVALID_INFO_CLASS, "Expected STATUS_INVALID_INFO_CLASS, got %08lx\n", status );

    dummy = 1;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadEnableAlignmentFaultFixup, &dummy, sizeof(ULONG) );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status );
    status = pNtSetInformationThread( (HANDLE)0xdeadbeef, ThreadEnableAlignmentFaultFixup, NULL, 0 );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status );
    status = pNtSetInformationThread( (HANDLE)0xdeadbeef, ThreadEnableAlignmentFaultFixup, NULL, 1 );
    ok( status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", status );
    status = pNtSetInformationThread( (HANDLE)0xdeadbeef, ThreadEnableAlignmentFaultFixup, &dummy, 1 );
    todo_wine ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status );
    status = pNtSetInformationThread( GetCurrentProcess(), ThreadEnableAlignmentFaultFixup, &dummy, 1 );
    todo_wine ok( status == STATUS_OBJECT_TYPE_MISMATCH, "Expected STATUS_OBJECT_TYPE_MISMATCH, got %08lx\n", status );
    dummy = 1;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadEnableAlignmentFaultFixup, &dummy, 1 );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status );

    dummy = 0;
    status = pNtSetInformationThread( GetCurrentProcess(), ThreadEnableAlignmentFaultFixup, &dummy, 8 );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status );
}

static DWORD WINAPI start_address_thread(void *arg)
{
    PRTL_THREAD_START_ROUTINE entry;
    NTSTATUS status;
    DWORD ret;

    entry = NULL;
    ret = 0xdeadbeef;
    status = pNtQueryInformationThread(GetCurrentThread(), ThreadQuerySetWin32StartAddress,
                                       &entry, sizeof(entry), &ret);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
    ok(ret == sizeof(entry), "NtQueryInformationThread returned %lu bytes\n", ret);
    ok(entry == (void *)start_address_thread, "expected %p, got %p\n", start_address_thread, entry);
    return 0;
}

static void test_thread_start_address(void)
{
    PRTL_THREAD_START_ROUTINE entry, expected_entry;
    IMAGE_NT_HEADERS *nt;
    NTSTATUS status;
    HANDLE thread;
    void *module;
    DWORD ret;

    module = GetModuleHandleA(0);
    ok(module != NULL, "expected non-NULL address for module\n");
    nt = RtlImageNtHeader(module);
    ok(nt != NULL, "expected non-NULL address for NT header\n");

    entry = NULL;
    ret = 0xdeadbeef;
    status = pNtQueryInformationThread(GetCurrentThread(), ThreadQuerySetWin32StartAddress,
                                       &entry, sizeof(entry), &ret);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
    ok(ret == sizeof(entry), "NtQueryInformationThread returned %lu bytes\n", ret);
    expected_entry = (void *)((char *)module + nt->OptionalHeader.AddressOfEntryPoint);
    ok(entry == expected_entry, "expected %p, got %p\n", expected_entry, entry);

    entry = (void *)0xdeadbeef;
    status = pNtSetInformationThread(GetCurrentThread(), ThreadQuerySetWin32StartAddress,
                                     &entry, sizeof(entry));
    ok(status == STATUS_SUCCESS || status == STATUS_INVALID_PARAMETER, /* >= Vista */
       "expected STATUS_SUCCESS or STATUS_INVALID_PARAMETER, got %08lx\n", status);

    if (status == STATUS_SUCCESS)
    {
        entry = NULL;
        ret = 0xdeadbeef;
        status = pNtQueryInformationThread(GetCurrentThread(), ThreadQuerySetWin32StartAddress,
                                           &entry, sizeof(entry), &ret);
        ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
        ok(ret == sizeof(entry), "NtQueryInformationThread returned %lu bytes\n", ret);
        ok(entry == (void *)0xdeadbeef, "expected 0xdeadbeef, got %p\n", entry);
    }

    thread = CreateThread(NULL, 0, start_address_thread, NULL, 0, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "CreateThread failed with %ld\n", GetLastError());
    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %lu\n", ret);
    CloseHandle(thread);
}

static void test_query_data_alignment(void)
{
    SYSTEM_CPU_INFORMATION sci;
    ULONG len;
    NTSTATUS status;
    DWORD value;

    value = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemRecommendedSharedDataAlignment, &value, sizeof(value), &len);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok(sizeof(value) == len, "Inconsistent length %lu\n", len);

    pRtlGetNativeSystemInformation(SystemCpuInformation, &sci, sizeof(sci), &len);
    switch (sci.ProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_ARM:
        ok(value == 32, "Expected 32, got %lu\n", value);
        break;
    case PROCESSOR_ARCHITECTURE_ARM64:
        ok(value == 128, "Expected 128, got %lu\n", value);
        break;
    default:
        ok(value == 64, "Expected 64, got %lu\n", value);
        break;
    }
}

static void test_thread_lookup(void)
{
    OBJECT_BASIC_INFORMATION obj_info;
    THREAD_BASIC_INFORMATION info;
    OBJECT_ATTRIBUTES attr;
    CLIENT_ID cid;
    HANDLE handle;
    NTSTATUS status;

    InitializeObjectAttributes( &attr, NULL, 0, NULL, NULL );
    cid.UniqueProcess = ULongToHandle(GetCurrentProcessId());
    cid.UniqueThread = ULongToHandle(GetCurrentThreadId());
    status = pNtOpenThread(&handle, THREAD_QUERY_INFORMATION, &attr, &cid);
    ok(!status, "NtOpenThread returned %#lx\n", status);
    status = pNtOpenThread((HANDLE *)0xdeadbee0, THREAD_QUERY_INFORMATION, &attr, &cid);
    ok( status == STATUS_ACCESS_VIOLATION, "NtOpenThread returned %#lx\n", status);

    status = pNtQueryObject(handle, ObjectBasicInformation, &obj_info, sizeof(obj_info), NULL);
    ok(!status, "NtQueryObject returned: %#lx\n", status);
    ok(obj_info.GrantedAccess == (THREAD_QUERY_LIMITED_INFORMATION | THREAD_QUERY_INFORMATION)
       || broken(obj_info.GrantedAccess == THREAD_QUERY_INFORMATION), /* winxp */
       "GrantedAccess = %lx\n", obj_info.GrantedAccess);

    status = pNtQueryInformationThread(handle, ThreadBasicInformation, &info, sizeof(info), NULL);
    ok(!status, "NtQueryInformationThread returned %#lx\n", status);
    ok(info.ClientId.UniqueProcess == ULongToHandle(GetCurrentProcessId()),
       "UniqueProcess = %p expected %lx\n", info.ClientId.UniqueProcess, GetCurrentProcessId());
    ok(info.ClientId.UniqueThread == ULongToHandle(GetCurrentThreadId()),
       "UniqueThread = %p expected %lx\n", info.ClientId.UniqueThread, GetCurrentThreadId());
    pNtClose(handle);

    cid.UniqueProcess = 0;
    cid.UniqueThread = ULongToHandle(GetCurrentThreadId());
    status = pNtOpenThread(&handle, THREAD_QUERY_INFORMATION, &attr, &cid);
    ok(!status, "NtOpenThread returned %#lx\n", status);
    status = pNtQueryInformationThread(handle, ThreadBasicInformation, &info, sizeof(info), NULL);
    ok(!status, "NtQueryInformationThread returned %#lx\n", status);
    ok(info.ClientId.UniqueProcess == ULongToHandle(GetCurrentProcessId()),
       "UniqueProcess = %p expected %lx\n", info.ClientId.UniqueProcess, GetCurrentProcessId());
    ok(info.ClientId.UniqueThread == ULongToHandle(GetCurrentThreadId()),
       "UniqueThread = %p expected %lx\n", info.ClientId.UniqueThread, GetCurrentThreadId());
    pNtClose(handle);

    cid.UniqueProcess = ULongToHandle(0xdeadbeef);
    cid.UniqueThread = ULongToHandle(GetCurrentThreadId());
    handle = (HANDLE)0xdeadbeef;
    status = NtOpenThread(&handle, THREAD_QUERY_INFORMATION, &attr, &cid);
    todo_wine
    ok(status == STATUS_INVALID_CID, "NtOpenThread returned %#lx\n", status);
    todo_wine
    ok( !handle || broken(handle == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", handle );
    if (!status) pNtClose(handle);

    cid.UniqueProcess = 0;
    cid.UniqueThread = ULongToHandle(0xdeadbeef);
    handle = (HANDLE)0xdeadbeef;
    status = pNtOpenThread(&handle, THREAD_QUERY_INFORMATION, &attr, &cid);
    ok(status == STATUS_INVALID_CID || broken(status == STATUS_INVALID_PARAMETER) /* winxp */,
       "NtOpenThread returned %#lx\n", status);
    ok( !handle || broken(handle == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", handle );
}

static void test_thread_ideal_processor(void)
{
    ULONG number, len;
    PROCESSOR_NUMBER processor;
    NTSTATUS status;

    number = 0;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadIdealProcessor, &number, sizeof(number) );
    ok(NT_SUCCESS(status), "Unexpected status %#lx.\n", status);

    number = 64 + 1;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadIdealProcessor, &number, sizeof(number) );
    ok(status == STATUS_INVALID_PARAMETER, "Unexpected status %#lx.\n", status);

    number = 0;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadIdealProcessor, &number, sizeof(number) );
    ok(!status, "Unexpected status %#lx.\n", status);

    status = pNtQueryInformationThread( GetCurrentThread(), ThreadIdealProcessor, &number, sizeof(number), &len );
    ok(status == STATUS_INVALID_INFO_CLASS, "Unexpected status %#lx.\n", status);

    status = pNtQueryInformationThread( GetCurrentThread(), ThreadIdealProcessorEx, &processor, sizeof(processor) + 1, &len );
#ifdef __REACTOS__
    ok(status == (GetNTVersion() >= _WIN32_WINNT_WIN7 ? STATUS_INFO_LENGTH_MISMATCH : STATUS_INVALID_INFO_CLASS), "Unexpected status %#lx.\n", status);
#else
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Unexpected status %#lx.\n", status);
#endif

    status = pNtQueryInformationThread( GetCurrentThread(), ThreadIdealProcessorEx, &processor, sizeof(processor), &len );
#ifdef __REACTOS__
    ok(status == (GetNTVersion() >= _WIN32_WINNT_WIN7 ? STATUS_SUCCESS : STATUS_INVALID_INFO_CLASS), "Unexpected status %#lx.\n", status);
#else
    ok(status == STATUS_SUCCESS, "Unexpected status %#lx.\n", status);
#endif
}

static void test_thread_info(void)
{
    NTSTATUS status;
    ULONG len, data;

    len = 0xdeadbeef;
    data = 0xcccccccc;
    status = pNtQueryInformationThread( GetCurrentThread(), ThreadAmILastThread,
                                        &data, sizeof(data), &len );
    ok( !status, "failed %lx\n", status );
    ok( data == 0 || data == 1, "wrong data %lx\n", data );
    ok( len == sizeof(data), "wrong len %lu\n", len );

    len = 0xdeadbeef;
    data = 0xcccccccc;
    status = pNtQueryInformationThread( GetCurrentThread(), ThreadAmILastThread,
                                        &data, sizeof(data) - 1, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "failed %lx\n", status );
    ok( data == 0xcccccccc, "wrong data %lx\n", data );
    ok( len == 0xdeadbeef, "wrong len %lu\n", len );

    len = 0xdeadbeef;
    data = 0xcccccccc;
    status = pNtQueryInformationThread( GetCurrentThread(), ThreadAmILastThread,
                                        &data, sizeof(data) + 1, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "failed %lx\n", status );
    ok( data == 0xcccccccc, "wrong data %lx\n", data );
    ok( len == 0xdeadbeef, "wrong len %lu\n", len );
}

static void test_debug_object(void)
{
    NTSTATUS status;
    HANDLE handle;
    OBJECT_ATTRIBUTES attr = { sizeof(attr) };
    ULONG len, flag = 0;
    DBGUI_WAIT_STATE_CHANGE state;
    DEBUG_EVENT event;

    status = pNtCreateDebugObject( &handle, DEBUG_ALL_ACCESS, &attr, 0 );
    ok( !status, "NtCreateDebugObject failed %lx\n", status );
    status = pNtSetInformationDebugObject( handle, 0, &flag, sizeof(ULONG), &len );
    ok( status == STATUS_INVALID_PARAMETER, "NtSetInformationDebugObject failed %lx\n", status );
    status = pNtSetInformationDebugObject( handle, 2, &flag, sizeof(ULONG), &len );
    ok( status == STATUS_INVALID_PARAMETER, "NtSetInformationDebugObject failed %lx\n", status );
    status = pNtSetInformationDebugObject( (HANDLE)0xdead, DebugObjectKillProcessOnExitInformation,
                                           &flag, sizeof(ULONG), &len );
    ok( status == STATUS_INVALID_HANDLE, "NtSetInformationDebugObject failed %lx\n", status );

    len = 0xdead;
    status = pNtSetInformationDebugObject( handle, DebugObjectKillProcessOnExitInformation,
                                           &flag, sizeof(ULONG) + 1, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtSetInformationDebugObject failed %lx\n", status );
    ok( len == sizeof(ULONG), "wrong len %lu\n", len );

    len = 0xdead;
    status = pNtSetInformationDebugObject( handle, DebugObjectKillProcessOnExitInformation,
                                           &flag, sizeof(ULONG) - 1, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtSetInformationDebugObject failed %lx\n", status );
    ok( len == sizeof(ULONG), "wrong len %lu\n", len );

    len = 0xdead;
    status = pNtSetInformationDebugObject( handle, DebugObjectKillProcessOnExitInformation,
                                           &flag, sizeof(ULONG), &len );
    ok( !status, "NtSetInformationDebugObject failed %lx\n", status );
    ok( !len, "wrong len %lu\n", len );

    flag = DEBUG_KILL_ON_CLOSE;
    status = pNtSetInformationDebugObject( handle, DebugObjectKillProcessOnExitInformation,
                                           &flag, sizeof(ULONG), &len );
    ok( !status, "NtSetInformationDebugObject failed %lx\n", status );
    ok( !len, "wrong len %lu\n", len );

    for (flag = 2; flag; flag <<= 1)
    {
        status = pNtSetInformationDebugObject( handle, DebugObjectKillProcessOnExitInformation,
                                               &flag, sizeof(ULONG), &len );
        ok( status == STATUS_INVALID_PARAMETER, "NtSetInformationDebugObject failed %lx\n", status );
    }

    pNtClose( handle );

    memset( &state, 0xdd, sizeof(state) );
    state.NewState = DbgIdle;
    memset( &event, 0xcc, sizeof(event) );
    status = pDbgUiConvertStateChangeStructure( &state, &event );
    ok( status == STATUS_UNSUCCESSFUL, "DbgUiConvertStateChangeStructure failed %lx\n", status );
    ok( event.dwProcessId == 0xdddddddd, "event not updated %lx\n", event.dwProcessId );
    ok( event.dwThreadId == 0xdddddddd, "event not updated %lx\n", event.dwThreadId );

    state.NewState = DbgReplyPending;
    memset( &event, 0xcc, sizeof(event) );
    status = pDbgUiConvertStateChangeStructure( &state, &event );
    ok( status == STATUS_UNSUCCESSFUL, "DbgUiConvertStateChangeStructure failed %lx\n", status );
    ok( event.dwProcessId == 0xdddddddd, "event not updated %lx\n", event.dwProcessId );
    ok( event.dwThreadId == 0xdddddddd, "event not updated %lx\n", event.dwThreadId );

    state.NewState = 11;
    memset( &event, 0xcc, sizeof(event) );
    status = pDbgUiConvertStateChangeStructure( &state, &event );
    ok( status == STATUS_UNSUCCESSFUL, "DbgUiConvertStateChangeStructure failed %lx\n", status );
    ok( event.dwProcessId == 0xdddddddd, "event not updated %lx\n", event.dwProcessId );
    ok( event.dwThreadId == 0xdddddddd, "event not updated %lx\n", event.dwThreadId );

    state.NewState = DbgExitProcessStateChange;
    state.StateInfo.ExitProcess.ExitStatus = 0x123456;
    status = pDbgUiConvertStateChangeStructure( &state, &event );
    ok( !status, "DbgUiConvertStateChangeStructure failed %lx\n", status );
    ok( event.dwProcessId == 0xdddddddd, "event not updated %lx\n", event.dwProcessId );
    ok( event.dwThreadId == 0xdddddddd, "event not updated %lx\n", event.dwThreadId );
    ok( event.u.ExitProcess.dwExitCode == 0x123456, "event not updated %lx\n", event.u.ExitProcess.dwExitCode );

    memset( &state, 0xdd, sizeof(state) );
    state.NewState = DbgCreateProcessStateChange;
    status = pDbgUiConvertStateChangeStructure( &state, &event );
    ok( !status, "DbgUiConvertStateChangeStructure failed %lx\n", status );
    ok( event.dwProcessId == 0xdddddddd, "event not updated %lx\n", event.dwProcessId );
    ok( event.dwThreadId == 0xdddddddd, "event not updated %lx\n", event.dwThreadId );
    ok( event.u.CreateProcessInfo.nDebugInfoSize == 0xdddddddd, "event not updated %lx\n", event.u.CreateProcessInfo.nDebugInfoSize );
    ok( event.u.CreateProcessInfo.lpThreadLocalBase == NULL, "event not updated %p\n", event.u.CreateProcessInfo.lpThreadLocalBase );
    ok( event.u.CreateProcessInfo.lpImageName == NULL, "event not updated %p\n", event.u.CreateProcessInfo.lpImageName );
    ok( event.u.CreateProcessInfo.fUnicode == TRUE, "event not updated %x\n", event.u.CreateProcessInfo.fUnicode );

    memset( &state, 0xdd, sizeof(state) );
    state.NewState = DbgLoadDllStateChange;
    status = pDbgUiConvertStateChangeStructure( &state, &event );
    ok( !status, "DbgUiConvertStateChangeStructure failed %lx\n", status );
    ok( event.dwProcessId == 0xdddddddd, "event not updated %lx\n", event.dwProcessId );
    ok( event.dwThreadId == 0xdddddddd, "event not updated %lx\n", event.dwThreadId );
    ok( event.u.LoadDll.nDebugInfoSize == 0xdddddddd, "event not updated %lx\n", event.u.LoadDll.nDebugInfoSize );
    ok( PtrToUlong(event.u.LoadDll.lpImageName) == 0xdddddddd, "event not updated %p\n", event.u.LoadDll.lpImageName );
    ok( event.u.LoadDll.fUnicode == TRUE, "event not updated %x\n", event.u.LoadDll.fUnicode );
}

static void test_process_instrumentation_callback(void)
{
    PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION info;
    NTSTATUS status;

    status = NtSetInformationProcess( GetCurrentProcess(), ProcessInstrumentationCallback, NULL, 0 );
    ok( status == STATUS_INFO_LENGTH_MISMATCH /* Win10 */ || status == STATUS_INVALID_INFO_CLASS
            || status == STATUS_NOT_SUPPORTED, "Got unexpected status %#lx.\n", status );
    if (status != STATUS_INFO_LENGTH_MISMATCH)
    {
        win_skip( "ProcessInstrumentationCallback is not supported.\n" );
        return;
    }

    memset(&info, 0, sizeof(info));
    status = NtSetInformationProcess( GetCurrentProcess(), ProcessInstrumentationCallback, &info, sizeof(info) );
    ok( status == STATUS_SUCCESS /* Win 10 */ || broken( status == STATUS_PRIVILEGE_NOT_HELD )
            || broken( status == STATUS_INFO_LENGTH_MISMATCH ), "Got unexpected status %#lx.\n", status );

    memset(&info, 0, sizeof(info));
    status = NtSetInformationProcess( GetCurrentProcess(), ProcessInstrumentationCallback, &info, 2 * sizeof(info) );
    ok( status == STATUS_SUCCESS || status == STATUS_INFO_LENGTH_MISMATCH
            || broken( status == STATUS_PRIVILEGE_NOT_HELD ) /* some versions and machines before Win10 */,
            "Got unexpected status %#lx.\n", status );

    if (status)
    {
        win_skip( "NtSetInformationProcess failed, skipping further tests.\n" );
        return;
    }

    status = NtSetInformationProcess( GetCurrentProcess(), ProcessInstrumentationCallback,
                                      &info.Callback, sizeof(info.Callback) );
    ok( status == STATUS_SUCCESS, "got %#lx.\n", status );
    status = NtSetInformationProcess( GetCurrentProcess(), ProcessInstrumentationCallback,
                                      &info.Callback, sizeof(info.Callback) + 4 );
    ok( status == STATUS_SUCCESS, "got %#lx.\n", status );
    status = NtSetInformationProcess( GetCurrentProcess(), ProcessInstrumentationCallback,
                                      &info.Callback, sizeof(info.Callback) / 2 );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %#lx.\n", status );
}

static void test_debuggee_dbgport(int argc, char **argv)
{
    NTSTATUS status, expect_status;
    DWORD_PTR debug_port = 0xdeadbeef;
    DWORD debug_flags = 0xdeadbeef;
    HANDLE handle;
    ACCESS_MASK access;

    if (argc < 2)
    {
        ok(0, "insufficient arguments for child process\n");
        return;
    }

    access = strtoul(argv[1], NULL, 0);
    winetest_push_context("debug object access %08lx", access);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessDebugPort,
                                         &debug_port, sizeof(debug_port), NULL );
    ok( !status, "NtQueryInformationProcess ProcessDebugPort failed, status %#lx.\n", status );
    ok( debug_port == ~(DWORD_PTR)0, "Expected port %#Ix, got %#Ix.\n", ~(DWORD_PTR)0, debug_port );

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessDebugFlags,
                                         &debug_flags, sizeof(debug_flags), NULL );
    ok( !status, "NtQueryInformationProcess ProcessDebugFlags failed, status %#lx.\n", status );

    expect_status = access ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessDebugObjectHandle,
                                         &handle, sizeof(handle), NULL );
    ok( status == expect_status, "NtQueryInformationProcess ProcessDebugObjectHandle expected status %#lx, actual %#lx.\n", expect_status, status );
    if (SUCCEEDED( status )) NtClose( handle );

    winetest_pop_context();
}

static DWORD WINAPI test_ThreadIsTerminated_thread( void *stop_event )
{
    WaitForSingleObject( stop_event, INFINITE );
    return STATUS_PENDING;
}

static void test_ThreadIsTerminated(void)
{
    HANDLE thread, stop_event;
    ULONG terminated;
    NTSTATUS status;

    stop_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    thread = CreateThread( NULL, 0, test_ThreadIsTerminated_thread, stop_event, 0, NULL );
    ok( thread != INVALID_HANDLE_VALUE, "failed, error %ld\n", GetLastError() );

    status = pNtQueryInformationThread( thread, ThreadIsTerminated, &terminated, sizeof(terminated) * 2, NULL );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %#lx.\n", status );

    terminated = 0xdeadbeef;
    status = pNtQueryInformationThread( thread, ThreadIsTerminated, &terminated, sizeof(terminated), NULL );
    ok( !status, "got %#lx.\n", status );
    ok( !terminated, "got %lu.\n", terminated );

    SetEvent( stop_event );
    WaitForSingleObject( thread, INFINITE );

    status = pNtQueryInformationThread( thread, ThreadIsTerminated, &terminated, sizeof(terminated), NULL );
    ok( !status, "got %#lx.\n", status );
    ok( terminated == 1, "got %lu.\n", terminated );

    CloseHandle(stop_event);
    CloseHandle(thread);

    status = pNtQueryInformationThread( thread, ThreadIsTerminated, &terminated, sizeof(terminated), NULL );
    ok( status == STATUS_INVALID_HANDLE, "got %#lx.\n", status );
}

static void test_system_debug_control(void)
{
    NTSTATUS status;
    int class;

    for (class = 0; class < SysDbgMaxInfoClass; ++class)
    {
#ifdef __REACTOS__
        if ((class == SysDbgBreakPoint) && is_reactos()) continue; // Don't break into the kernel debugger!
#endif
        status = pNtSystemDebugControl( class, NULL, 0, NULL, 0, NULL );
        if (is_wow64)
        {
            /* Most of the calls return STATUS_NOT_IMPLEMENTED on wow64. */
            ok( status == STATUS_DEBUGGER_INACTIVE || status == STATUS_NOT_IMPLEMENTED || status == STATUS_INFO_LENGTH_MISMATCH,
                    "class %d, got %#lx.\n", class, status );
        }
        else
        {
            ok( status == STATUS_DEBUGGER_INACTIVE || status == STATUS_ACCESS_DENIED || status == STATUS_INFO_LENGTH_MISMATCH || broken(/* __REACTOS__ Win 2003: */ status == STATUS_NOT_IMPLEMENTED),
                "class %d, got %#lx.\n", class, status );
        }
    }
}

static void test_process_token(int argc, char **argv)
{
    STARTUPINFOA si = {.cb = sizeof(si)};
    PROCESS_ACCESS_TOKEN token_info = {0};
    TOKEN_STATISTICS stats1, stats2;
    HANDLE token, their_token;
    PROCESS_INFORMATION pi;
    char cmdline[MAX_PATH];
    NTSTATUS status;
    DWORD size;
    BOOL ret;

    token_info.Thread = (HANDLE)0xdeadbeef;

    sprintf( cmdline, "%s %s dummy", argv[0], argv[1] );

    ret = CreateProcessA( NULL, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi );
    ok( ret, "got error %lu\n", GetLastError() );

    status = pNtSetInformationProcess( pi.hProcess, ProcessAccessToken, &token_info, sizeof(token_info) - 1 );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %#lx\n", status );

    status = pNtSetInformationProcess( pi.hProcess, ProcessAccessToken, &token_info, sizeof(token_info) );
    ok( status == STATUS_INVALID_HANDLE, "got %#lx\n", status );

    ret = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY | READ_CONTROL | TOKEN_DUPLICATE
            | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_DEFAULT, &token );
    ok( ret, "got error %lu\n", GetLastError() );

    token_info.Token = token;
    status = pNtSetInformationProcess( pi.hProcess, ProcessAccessToken, &token_info, sizeof(token_info) );
    todo_wine ok( status == STATUS_TOKEN_ALREADY_IN_USE, "got %#lx\n", status );

    ret = DuplicateTokenEx( token, TOKEN_ALL_ACCESS, NULL, SecurityAnonymous, TokenImpersonation, &token_info.Token );
    ok( ret, "got error %lu\n", GetLastError() );
    status = pNtSetInformationProcess( pi.hProcess, ProcessAccessToken, &token_info, sizeof(token_info) );
#ifdef __REACTOS__
    if (GetNTVersion() < _WIN32_WINNT_WIN7)
        todo_wine ok( status == STATUS_BAD_TOKEN_TYPE, "got %#lx\n", status );
    else
#endif
    todo_wine ok( status == STATUS_BAD_IMPERSONATION_LEVEL, "got %#lx\n", status );
    CloseHandle( token_info.Token );

    ret = DuplicateTokenEx( token, TOKEN_QUERY, NULL, SecurityAnonymous, TokenPrimary, &token_info.Token );
    ok( ret, "got error %lu\n", GetLastError() );
    status = pNtSetInformationProcess( pi.hProcess, ProcessAccessToken, &token_info, sizeof(token_info) );
    ok( status == STATUS_ACCESS_DENIED, "got %#lx\n", status );
    CloseHandle( token_info.Token );

    ret = DuplicateTokenEx( token, TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY, NULL, SecurityAnonymous, TokenPrimary, &token_info.Token );
    ok(ret, "got error %lu\n", GetLastError());
    status = pNtSetInformationProcess( pi.hProcess, ProcessAccessToken, &token_info, sizeof(token_info) );
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );

    ret = OpenProcessToken( pi.hProcess, TOKEN_QUERY, &their_token );
    ok( ret, "got error %lu\n", GetLastError() );

    /* The tokens should be the same. */
    ret = GetTokenInformation( token_info.Token, TokenStatistics, &stats1, sizeof(stats1), &size );
    ok( ret, "got error %lu\n", GetLastError() );
    ret = GetTokenInformation( their_token, TokenStatistics, &stats2, sizeof(stats2), &size );
    ok( ret, "got error %lu\n", GetLastError() );
    ok( !memcmp( &stats1.TokenId, &stats2.TokenId, sizeof(LUID) ), "expected same IDs\n" );

    CloseHandle( token_info.Token );
    CloseHandle( their_token );

    ResumeThread( pi.hThread );
    ret = WaitForSingleObject( pi.hProcess, 1000 );
    ok( !ret, "got %d\n", ret );

    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    CloseHandle( token );
}

static void test_process_id(void)
{
    char image_name_buffer[1024 * sizeof(WCHAR)];
    UNICODE_STRING *image_name = (UNICODE_STRING *)image_name_buffer;
    SYSTEM_PROCESS_ID_INFORMATION info;
    unsigned int i, length;
    DWORD pids[2048];
    WCHAR name[2048];
    NTSTATUS status;
    HANDLE process;
    ULONG len;
    BOOL bret;

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessImageFileName, image_name,
                                        sizeof(image_name_buffer), NULL );
    ok( !status, "got %#lx.\n", status );
    length = image_name->Length;
    image_name->Buffer[length] = 0;

    len = 0xdeadbeef;
    status = pNtQuerySystemInformation( SystemProcessIdInformation, NULL, 0, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH || (is_wow64 && status == STATUS_ACCESS_VIOLATION), "got %#lx.\n", status );
    ok( len == sizeof(info) || (is_wow64 && len == 0xdeadbeef), "got %#lx.\n", len );

    info.ProcessId = 0xdeadbeef;
    info.ImageName.Length = info.ImageName.MaximumLength = 0;
    info.ImageName.Buffer = NULL;
    status = pNtQuerySystemInformation( SystemProcessIdInformation, &info, sizeof(info), &len );
    ok( status == STATUS_INVALID_CID, "got %#lx.\n", status );
    ok( !info.ImageName.Length, "got %#x.\n", info.ImageName.Length );
    ok( !info.ImageName.MaximumLength, "got %#x.\n", info.ImageName.MaximumLength );

    info.ProcessId = GetCurrentProcessId();
    status = pNtQuerySystemInformation( SystemProcessIdInformation, &info, sizeof(info), &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %#lx.\n", status );
    ok( len == sizeof(info), "got %#lx.\n", len );
    ok( !info.ImageName.Length, "got %#x.\n", info.ImageName.Length );
    ok( info.ImageName.MaximumLength == length + 2 || (is_wow64 && !info.ImageName.MaximumLength),
        "got %#x.\n", info.ImageName.MaximumLength );

    info.ImageName.MaximumLength = sizeof(name);
    len = 0xdeadbeef;
    status = pNtQuerySystemInformation( SystemProcessIdInformation, &info, sizeof(info), &len );
    ok( status == STATUS_ACCESS_VIOLATION, "got %#lx.\n", status );
    ok( len == sizeof(info), "got %#lx.\n", len );
    ok( info.ImageName.Length == length || (is_wow64 && !info.ImageName.Length),
        "got %u.\n", info.ImageName.Length );
    ok( info.ImageName.MaximumLength == length + 2 || (is_wow64 && !info.ImageName.Length),
        "got %#x.\n", info.ImageName.MaximumLength );

    info.ProcessId = 0xdeadbeef;
    info.ImageName.MaximumLength = sizeof(name);
    info.ImageName.Buffer = name;
    info.ImageName.Length = 0;
    status = pNtQuerySystemInformation( SystemProcessIdInformation, &info, sizeof(info), &len );
    ok( status == STATUS_INVALID_CID, "got %#lx.\n", status );
    ok( !info.ImageName.Length, "got %#x.\n", info.ImageName.Length );
    ok( info.ImageName.MaximumLength == sizeof(name), "got %#x.\n", info.ImageName.MaximumLength );
    ok( info.ImageName.Buffer == name, "got %p, %p.\n", info.ImageName.Buffer, name );

    info.ProcessId = 0;
    info.ImageName.MaximumLength = sizeof(name);
    info.ImageName.Buffer = name;
    info.ImageName.Length = 0;
    status = pNtQuerySystemInformation( SystemProcessIdInformation, &info, sizeof(info), &len );
    ok( status == STATUS_INVALID_CID, "got %#lx.\n", status );
    ok( !info.ImageName.Length, "got %#x.\n", info.ImageName.Length );
    ok( info.ImageName.MaximumLength == sizeof(name), "got %#x.\n", info.ImageName.MaximumLength );
    ok( info.ImageName.Buffer == name, "got non NULL.\n" );

    info.ProcessId = 0;
    info.ImageName.MaximumLength = sizeof(name);
    info.ImageName.Buffer = name;
    info.ImageName.Length = 4;
    status = pNtQuerySystemInformation( SystemProcessIdInformation, &info, sizeof(info), &len );
    ok( status == STATUS_INVALID_PARAMETER, "got %#lx.\n", status );
    ok( info.ImageName.Length == 4, "got %#x.\n", info.ImageName.Length );
    ok( info.ImageName.MaximumLength == sizeof(name), "got %#x.\n", info.ImageName.MaximumLength );
    ok( info.ImageName.Buffer == name, "got non NULL.\n" );

    info.ProcessId = GetCurrentProcessId();
    info.ImageName.MaximumLength = sizeof(name);
    info.ImageName.Buffer = name;
    info.ImageName.Length = 4;
    status = pNtQuerySystemInformation( SystemProcessIdInformation, &info, sizeof(info), NULL );
    ok( status == STATUS_INVALID_PARAMETER, "got %#lx.\n", status );
    ok( info.ImageName.Length == 4, "got %#x.\n", info.ImageName.Length );
    ok( info.ImageName.MaximumLength == sizeof(name), "got %#x.\n", info.ImageName.MaximumLength );

    info.ImageName.Length = 0;
    memset( name, 0xcc, sizeof(name) );
    status = pNtQuerySystemInformation( SystemProcessIdInformation, &info, sizeof(info), &len );
    ok( !status, "got %#lx.\n", status );
    ok( info.ImageName.Length == length, "got %#x.\n", info.ImageName.Length );
    ok( len == sizeof(info), "got %#lx.\n", len );
    ok( info.ImageName.MaximumLength == info.ImageName.Length + 2, "got %#x.\n", info.ImageName.MaximumLength );
    ok( !name[info.ImageName.Length / 2], "got %#x.\n", name[info.ImageName.Length / 2] );

    ok( info.ImageName.Length == image_name->Length, "got %#x, %#x.\n", info.ImageName.Length, image_name->Length );
    ok( !wcscmp( name, image_name->Buffer ), "got %s, %s.\n", debugstr_w(name), debugstr_w(image_name->Buffer) );

#if !defined (__REACTOS__) || (DLL_EXPORT_VERSION >= 0x601)
    bret = EnumProcesses( pids, sizeof(pids), &len );
    ok( bret, "got error %lu.\n", GetLastError() );
    for (i = 0; i < len / sizeof(*pids); ++i)
    {
        process = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pids[i] );
        if (pids[i] && !process && GetLastError() != ERROR_ACCESS_DENIED)
        {
            /* process is gone already. */
            continue;
        }
        info.ProcessId = pids[i];
        info.ImageName.Length = 0;
        info.ImageName.MaximumLength = sizeof(name);
        info.ImageName.Buffer = name;
        status = pNtQuerySystemInformation( SystemProcessIdInformation, &info, sizeof(info), &len );
        ok( info.ImageName.Buffer == name || (!info.ImageName.MaximumLength && !info.ImageName.Length),
            "got %p, %p, pid %lu, lengh %u / %u.\n", info.ImageName.Buffer, name, pids[i],
            info.ImageName.Length, info.ImageName.MaximumLength );
        if (pids[i])
            ok( !status, "got %#lx, pid %lu.\n", status, pids[i] );
        else
            ok( status == STATUS_INVALID_CID, "got %#lx, pid %lu.\n", status, pids[i] );
        if (process) CloseHandle( process );
    }
#endif
}

static void test_processor_idle_cycle_time(void)
{
    unsigned int cpu_count = NtCurrentTeb()->Peb->NumberOfProcessors;
    ULONG64 buffer[64];
    NTSTATUS status;
    USHORT group_id;
    ULONG size;

    size = 0xdeadbeef;
    status = pNtQuerySystemInformation( SystemProcessorIdleCycleTimeInformation, NULL, 0, &size );
    ok( status == STATUS_BUFFER_TOO_SMALL, "got %#lx.\n", status );
    ok( size == cpu_count * sizeof(*buffer), "got %#lx.\n", size );

    size = 0xdeadbeef;
    status = pNtQuerySystemInformation( SystemProcessorIdleCycleTimeInformation, buffer, 7, &size );
    ok( status == STATUS_BUFFER_TOO_SMALL, "got %#lx.\n", status );
    ok( size == cpu_count * sizeof(*buffer), "got %#lx.\n", size );

    size = 0xdeadbeef;
    status = pNtQuerySystemInformation( SystemProcessorIdleCycleTimeInformation, NULL, sizeof(buffer), &size );
    ok( status == STATUS_ACCESS_VIOLATION, "got %#lx.\n", status );
    ok( size == 0xdeadbeef, "got %#lx.\n", size );

    size = 0xdeadbeef;
    status = pNtQuerySystemInformation( SystemProcessorIdleCycleTimeInformation, buffer, sizeof(buffer), &size );
    ok( !status, "got %#lx.\n", status );
    ok( size == cpu_count * sizeof(*buffer), "got %#lx.\n", size );

#ifdef __REACTOS__
    if (pNtQuerySystemInformationEx == NULL)
    {
        win_skip("NtQuerySystemInformationEx is not available.\n");
        return;
    }
#endif

    memset( buffer, 0xcc, sizeof(buffer) );
    size = 0xdeadbeef;
    status = pNtQuerySystemInformationEx( SystemProcessorIdleCycleTimeInformation, NULL, 0, buffer, sizeof(buffer), &size );
    ok( status == STATUS_INVALID_PARAMETER, "got %#lx.\n", status );
    ok( size == 0xdeadbeef, "got %#lx.\n", size );
    group_id = 50;
    size = 0xdeadbeef;
    status = pNtQuerySystemInformationEx( SystemProcessorIdleCycleTimeInformation, &group_id, sizeof(group_id), buffer, sizeof(buffer), &size );
    ok( status == STATUS_INVALID_PARAMETER, "got %#lx.\n", status );
    ok( size == 0xdeadbeef, "got %#lx.\n", size );
    group_id = 0;
    size = 0xdeadbeef;
    status = pNtQuerySystemInformationEx( SystemProcessorIdleCycleTimeInformation, &group_id, sizeof(group_id), buffer, sizeof(buffer), &size );
    ok( status == STATUS_SUCCESS, "got %#lx.\n", status );
    ok( size == cpu_count * sizeof(*buffer), "got %#lx.\n", size );
}

START_TEST(info)
{
    char **argv;
    int argc;

    InitFunctionPtrs();

    argc = winetest_get_mainargs(&argv);
    if (argc >= 3)
    {
        if (strcmp(argv[2], "debuggee:dbgport") == 0) test_debuggee_dbgport(argc - 2, argv + 2);
        return; /* Child */
    }

    /* NtQuerySystemInformation */
    test_query_basic();
    test_query_cpu();
    test_query_performance();
    test_query_timeofday();
    test_query_process( TRUE );
    test_query_process( FALSE );
    test_query_procperf();
    test_query_module();
    test_query_handle();
    test_query_handle_ex();
    test_query_cache();
    test_query_interrupt();
    test_time_adjustment();
    test_query_kerndebug();
    test_query_regquota();
    test_query_logicalproc();
    test_query_logicalprocex();
    test_query_cpusetinfo();
    test_query_firmware();
    test_query_data_alignment();

    /* NtPowerInformation */
    test_query_battery();
    test_query_processor_power_info();

    /* NtQueryInformationProcess */
    test_query_process_basic();
    test_query_process_io();
    test_query_process_vm();
    test_query_process_times();
    test_query_process_debug_port(argc, argv);
    test_query_process_debug_port_custom_dacl(argc, argv);
    test_query_process_priority();
    test_query_process_handlecount();
    test_query_process_wow64();
    test_query_process_image_file_name();
    test_query_process_debug_object_handle(argc, argv);
    test_query_process_debug_flags(argc, argv);
    test_query_process_image_info();
    test_query_process_quota_limits();
    test_mapprotection();
    test_threadstack();

    /* NtQueryInformationThread */
    test_thread_info();
    test_HideFromDebugger();
    test_thread_start_address();
    test_thread_lookup();
    test_thread_ideal_processor();
    test_ThreadIsTerminated();

    test_affinity();
    test_debug_object();

    /* belongs to its own file */
    test_readvirtualmemory();
    test_queryvirtualmemory();
    test_NtGetCurrentProcessorNumber();

    test_ThreadEnableAlignmentFaultFixup();
    test_process_instrumentation_callback();
    test_system_debug_control();
    test_process_token(argc, argv);
    test_process_id();
    test_processor_idle_cycle_time();
}
