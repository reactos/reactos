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

#include "ntdll_test.h"
#include <winnls.h>
#include <stdio.h>

static NTSTATUS (WINAPI * pRtlDowncaseUnicodeString)(UNICODE_STRING *, const UNICODE_STRING *, BOOLEAN);
static NTSTATUS (WINAPI * pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI * pNtQuerySystemInformationEx)(SYSTEM_INFORMATION_CLASS, void*, ULONG, void*, ULONG, ULONG*);
static NTSTATUS (WINAPI * pNtPowerInformation)(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG);
static NTSTATUS (WINAPI * pNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI * pNtQueryInformationThread)(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI * pNtSetInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG);
static NTSTATUS (WINAPI * pNtSetInformationThread)(HANDLE, THREADINFOCLASS, PVOID, ULONG);
static NTSTATUS (WINAPI * pNtReadVirtualMemory)(HANDLE, const void*, void*, SIZE_T, SIZE_T*);
static NTSTATUS (WINAPI * pNtQueryVirtualMemory)(HANDLE, LPCVOID, MEMORY_INFORMATION_CLASS , PVOID , SIZE_T , SIZE_T *);
static NTSTATUS (WINAPI * pNtCreateSection)(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*,const LARGE_INTEGER*,ULONG,ULONG,HANDLE);
static NTSTATUS (WINAPI * pNtMapViewOfSection)(HANDLE,HANDLE,PVOID*,ULONG,SIZE_T,const LARGE_INTEGER*,SIZE_T*,SECTION_INHERIT,ULONG,ULONG);
static NTSTATUS (WINAPI * pNtUnmapViewOfSection)(HANDLE,PVOID);
static NTSTATUS (WINAPI * pNtClose)(HANDLE);
static ULONG    (WINAPI * pNtGetCurrentProcessorNumber)(void);
static BOOL     (WINAPI * pIsWow64Process)(HANDLE, PBOOL);
static BOOL     (WINAPI * pGetLogicalProcessorInformationEx)(LOGICAL_PROCESSOR_RELATIONSHIP,SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*,DWORD*);

static BOOL is_wow64;

/* one_before_last_pid is used to be able to compare values of a still running process
   with the output of the test_query_process_times and test_query_process_handlecount tests.
*/
static DWORD one_before_last_pid = 0;

#define NTDLL_GET_PROC(func) do {                     \
    p ## func = (void*)GetProcAddress(hntdll, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
      return FALSE; \
    } \
  } while(0)

static BOOL InitFunctionPtrs(void)
{
    /* All needed functions are NT based, so using GetModuleHandle is a good check */
    HMODULE hntdll = GetModuleHandleA("ntdll");
    HMODULE hkernel32 = GetModuleHandleA("kernel32");

    if (!hntdll)
    {
        win_skip("Not running on NT\n");
        return FALSE;
    }

    NTDLL_GET_PROC(RtlDowncaseUnicodeString);
    NTDLL_GET_PROC(NtQuerySystemInformation);
    NTDLL_GET_PROC(NtPowerInformation);
    NTDLL_GET_PROC(NtQueryInformationProcess);
    NTDLL_GET_PROC(NtQueryInformationThread);
    NTDLL_GET_PROC(NtSetInformationProcess);
    NTDLL_GET_PROC(NtSetInformationThread);
    NTDLL_GET_PROC(NtReadVirtualMemory);
    NTDLL_GET_PROC(NtQueryVirtualMemory);
    NTDLL_GET_PROC(NtClose);
    NTDLL_GET_PROC(NtCreateSection);
    NTDLL_GET_PROC(NtMapViewOfSection);
    NTDLL_GET_PROC(NtUnmapViewOfSection);

    /* not present before XP */
    pNtGetCurrentProcessorNumber = (void *) GetProcAddress(hntdll, "NtGetCurrentProcessorNumber");

    pIsWow64Process = (void *)GetProcAddress(hkernel32, "IsWow64Process");
    if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

    /* starting with Win7 */
    pNtQuerySystemInformationEx = (void *) GetProcAddress(hntdll, "NtQuerySystemInformationEx");
    if (!pNtQuerySystemInformationEx)
        win_skip("NtQuerySystemInformationEx() is not supported, some tests will be skipped.\n");

    pGetLogicalProcessorInformationEx = (void *) GetProcAddress(hkernel32, "GetLogicalProcessorInformationEx");

    return TRUE;
}

static void test_query_basic(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    SYSTEM_BASIC_INFORMATION sbi;

    /* This test also covers some basic parameter testing that should be the same for 
     * every information class
    */

    /* Use a nonexistent info class */
    trace("Check nonexistent info class\n");
    status = pNtQuerySystemInformation(-1, NULL, 0, NULL);
    ok( status == STATUS_INVALID_INFO_CLASS || status == STATUS_NOT_IMPLEMENTED /* vista */,
        "Expected STATUS_INVALID_INFO_CLASS or STATUS_NOT_IMPLEMENTED, got %08x\n", status);

    /* Use an existing class but with a zero-length buffer */
    trace("Check zero-length buffer\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, NULL, 0, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    /* Use an existing class, correct length but no SystemInformation buffer */
    trace("Check no SystemInformation buffer\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, NULL, sizeof(sbi), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER /* vista */,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER, got %08x\n", status);

    /* Use an existing class, correct length, a pointer to a buffer but no ReturnLength pointer */
    trace("Check no ReturnLength pointer\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);

    /* Check a too large buffer size */
    trace("Check a too large buffer size\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    /* Finally some correct calls */
    trace("Check with correct parameters\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( sizeof(sbi) == ReturnLength, "Inconsistent length %d\n", ReturnLength);

    /* Check if we have some return values */
    trace("Number of Processors : %d\n", sbi.NumberOfProcessors);
    ok( sbi.NumberOfProcessors > 0, "Expected more than 0 processors, got %d\n", sbi.NumberOfProcessors);
}

static void test_query_cpu(void)
{
    DWORD status;
    ULONG ReturnLength;
    SYSTEM_CPU_INFORMATION sci;

    status = pNtQuerySystemInformation(SystemCpuInformation, &sci, sizeof(sci), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( sizeof(sci) == ReturnLength, "Inconsistent length %d\n", ReturnLength);

    /* Check if we have some return values */
    trace("Processor FeatureSet : %08x\n", sci.ProcessorFeatureBits);
    ok( sci.ProcessorFeatureBits != 0, "Expected some features for this processor, got %08x\n", sci.ProcessorFeatureBits);
}

static void test_query_performance(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    ULONGLONG buffer[sizeof(SYSTEM_PERFORMANCE_INFORMATION)/sizeof(ULONGLONG) + 5];
    DWORD size = sizeof(SYSTEM_PERFORMANCE_INFORMATION);

    status = pNtQuerySystemInformation(SystemPerformanceInformation, buffer, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    status = pNtQuerySystemInformation(SystemPerformanceInformation, buffer, size, &ReturnLength);
    if (status == STATUS_INFO_LENGTH_MISMATCH && is_wow64)
    {
        /* size is larger on wow64 under w2k8/win7 */
        size += 16;
        status = pNtQuerySystemInformation(SystemPerformanceInformation, buffer, size, &ReturnLength);
    }
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( ReturnLength == size, "Inconsistent length %d\n", ReturnLength);

    status = pNtQuerySystemInformation(SystemPerformanceInformation, buffer, size + 2, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( ReturnLength == size || ReturnLength == size + 2,
        "Inconsistent length %d\n", ReturnLength);

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
  
    /*  The struct size for NT (32 bytes) and Win2K/XP (48 bytes) differ.
     *
     *  Windows 2000 and XP return STATUS_INFO_LENGTH_MISMATCH if the given buffer size is greater
     *  then 48 and 0 otherwise
     *  Windows NT returns STATUS_INFO_LENGTH_MISMATCH when the given buffer size is not correct
     *  and 0 otherwise
     *
     *  Windows 2000 and XP copy the given buffer size into the provided buffer, if the return code is STATUS_SUCCESS
     *  NT only fills the buffer if the return code is STATUS_SUCCESS
     *
    */

    status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, sizeof(sti), &ReturnLength);

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        trace("Windows version is NT, we have to cater for differences with W2K/WinXP\n");
 
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 0, &ReturnLength);
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
        ok( 0 == ReturnLength, "ReturnLength should be 0, it is (%d)\n", ReturnLength);

        sti.uCurrentTimeZoneId = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 28, &ReturnLength);
        ok(status == STATUS_SUCCESS || broken(status == STATUS_INFO_LENGTH_MISMATCH /* NT4 */), "Expected STATUS_SUCCESS, got %08x\n", status);
        ok( 0xdeadbeef == sti.uCurrentTimeZoneId, "This part of the buffer should not have been filled\n");

        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 32, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
        ok( 32 == ReturnLength, "ReturnLength should be 0, it is (%d)\n", ReturnLength);
    }
    else
    {
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 0, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
        ok( 0 == ReturnLength, "ReturnLength should be 0, it is (%d)\n", ReturnLength);

        sti.uCurrentTimeZoneId = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 24, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
        ok( 24 == ReturnLength, "ReturnLength should be 24, it is (%d)\n", ReturnLength);
        ok( 0xdeadbeef == sti.uCurrentTimeZoneId, "This part of the buffer should not have been filled\n");
    
        sti.uCurrentTimeZoneId = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 32, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
        ok( 32 == ReturnLength, "ReturnLength should be 32, it is (%d)\n", ReturnLength);
        ok( 0xdeadbeef != sti.uCurrentTimeZoneId, "Buffer should have been partially filled\n");
    
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 49, &ReturnLength);
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
        ok( ReturnLength == 0 || ReturnLength == sizeof(sti) /* vista */,
            "ReturnLength should be 0, it is (%d)\n", ReturnLength);
    
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, sizeof(sti), &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
        ok( sizeof(sti) == ReturnLength, "Inconsistent length %d\n", ReturnLength);
    }

    /* Check if we have some return values */
    trace("uCurrentTimeZoneId : (%d)\n", sti.uCurrentTimeZoneId);
}

static void test_query_process(void)
{
    NTSTATUS status;
    DWORD last_pid;
    ULONG ReturnLength;
    int i = 0, k = 0;
    BOOL is_nt = FALSE;
    SYSTEM_BASIC_INFORMATION sbi;

    /* Copy of our winternl.h structure turned into a private one */
    typedef struct _SYSTEM_PROCESS_INFORMATION_PRIVATE {
        ULONG NextEntryOffset;
        DWORD dwThreadCount;
        DWORD dwUnknown1[6];
        FILETIME ftCreationTime;
        FILETIME ftUserTime;
        FILETIME ftKernelTime;
        UNICODE_STRING ProcessName;
        DWORD dwBasePriority;
        HANDLE UniqueProcessId;
        HANDLE ParentProcessId;
        ULONG HandleCount;
        DWORD dwUnknown3;
        DWORD dwUnknown4;
        VM_COUNTERS vmCounters;
        IO_COUNTERS ioCounters;
        SYSTEM_THREAD_INFORMATION ti[1];
    } SYSTEM_PROCESS_INFORMATION_PRIVATE;

    ULONG SystemInformationLength = sizeof(SYSTEM_PROCESS_INFORMATION_PRIVATE);
    SYSTEM_PROCESS_INFORMATION_PRIVATE *spi, *spi_buf = HeapAlloc(GetProcessHeap(), 0, SystemInformationLength);

    /* test ReturnLength */
    ReturnLength = 0;
    status = pNtQuerySystemInformation(SystemProcessInformation, NULL, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH got %08x\n", status);
    ok( ReturnLength > 0 || broken(ReturnLength == 0) /* NT4, Win2K */,
        "Expected a ReturnLength to show the needed length\n");

    /* W2K3 and later returns the needed length, the rest returns 0, so we have to loop */
    for (;;)
    {
        status = pNtQuerySystemInformation(SystemProcessInformation, spi_buf, SystemInformationLength, &ReturnLength);

        if (status != STATUS_INFO_LENGTH_MISMATCH) break;
        
        spi_buf = HeapReAlloc(GetProcessHeap(), 0, spi_buf , SystemInformationLength *= 2);
    }
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    spi = spi_buf;

    /* Get the first NextEntryOffset, from this we can deduce the OS version we're running
     *
     * W2K/WinXP/W2K3:
     *   NextEntryOffset for a process is 184 + (no. of threads) * sizeof(SYSTEM_THREAD_INFORMATION)
     * NT:
     *   NextEntryOffset for a process is 136 + (no. of threads) * sizeof(SYSTEM_THREAD_INFORMATION)
     * Wine (with every windows version):
     *   NextEntryOffset for a process is 0 if just this test is running
     *   NextEntryOffset for a process is 184 + (no. of threads) * sizeof(SYSTEM_THREAD_INFORMATION) +
     *                             ProcessName.MaximumLength
     *     if more wine processes are running
     *
     * Note : On windows the first process is in fact the Idle 'process' with a thread for every processor
    */

    pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength);

    is_nt = ( spi->NextEntryOffset - (sbi.NumberOfProcessors * sizeof(SYSTEM_THREAD_INFORMATION)) == 136);

    if (is_nt) win_skip("Windows version is NT, we will skip thread tests\n");

    /* Check if we have some return values
     * 
     * On windows there will be several processes running (Including the always present Idle and System)
     * On wine we only have one (if this test is the only wine process running)
    */
    
    /* Loop through the processes */

    for (;;)
    {
        i++;

        last_pid = (DWORD_PTR)spi->UniqueProcessId;

        disable_success_count
        ok( spi->dwThreadCount > 0, "Expected some threads for this process, got 0\n");

        /* Loop through the threads, skip NT4 for now */
        
        if (!is_nt)
        {
            DWORD j;
            for ( j = 0; j < spi->dwThreadCount; j++) 
            {
                k++;
                disable_success_count
                ok ( spi->ti[j].ClientId.UniqueProcess == spi->UniqueProcessId,
                     "The owning pid of the thread (%p) doesn't equal the pid (%p) of the process\n",
                     spi->ti[j].ClientId.UniqueProcess, spi->UniqueProcessId);
            }
        }

        if (!spi->NextEntryOffset) break;

        one_before_last_pid = last_pid;

        spi = (SYSTEM_PROCESS_INFORMATION_PRIVATE*)((char*)spi + spi->NextEntryOffset);
    }
    trace("Total number of running processes : %d\n", i);
    if (!is_nt) trace("Total number of running threads   : %d\n", k);

    if (one_before_last_pid == 0) one_before_last_pid = last_pid;

    HeapFree( GetProcessHeap(), 0, spi_buf);
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
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    NeededLength = sbi.NumberOfProcessors * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

    sppi = HeapAlloc(GetProcessHeap(), 0, NeededLength);

    status = pNtQuerySystemInformation(SystemProcessorPerformanceInformation, sppi, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    /* Try it for 1 processor */
    sppi->KernelTime.QuadPart = 0xdeaddead;
    sppi->UserTime.QuadPart = 0xdeaddead;
    sppi->IdleTime.QuadPart = 0xdeaddead;
    status = pNtQuerySystemInformation(SystemProcessorPerformanceInformation, sppi,
                                       sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) == ReturnLength,
        "Inconsistent length %d\n", ReturnLength);
    ok (sppi->KernelTime.QuadPart != 0xdeaddead, "KernelTime unchanged\n");
    ok (sppi->UserTime.QuadPart != 0xdeaddead, "UserTime unchanged\n");
    ok (sppi->IdleTime.QuadPart != 0xdeaddead, "IdleTime unchanged\n");

    /* Try it for all processors */
    sppi->KernelTime.QuadPart = 0xdeaddead;
    sppi->UserTime.QuadPart = 0xdeaddead;
    sppi->IdleTime.QuadPart = 0xdeaddead;
    status = pNtQuerySystemInformation(SystemProcessorPerformanceInformation, sppi, NeededLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( NeededLength == ReturnLength, "Inconsistent length (%d) <-> (%d)\n", NeededLength, ReturnLength);
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
        "Expected STATUS_SUCCESS or STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
    ok( NeededLength == ReturnLength, "Inconsistent length (%d) <-> (%d)\n", NeededLength, ReturnLength);
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
    NTSTATUS status;
    ULONG ReturnLength;
    ULONG ModuleCount, i;

    ULONG SystemInformationLength = sizeof(RTL_PROCESS_MODULES);
    RTL_PROCESS_MODULES* smi = HeapAlloc(GetProcessHeap(), 0, SystemInformationLength); 
    RTL_PROCESS_MODULE_INFORMATION* sm;

    /* Request the needed length */
    status = pNtQuerySystemInformation(SystemModuleInformation, smi, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
    ok( ReturnLength > 0, "Expected a ReturnLength to show the needed length\n");

    SystemInformationLength = ReturnLength;
    smi = HeapReAlloc(GetProcessHeap(), 0, smi , SystemInformationLength);
    status = pNtQuerySystemInformation(SystemModuleInformation, smi, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);

    ModuleCount = smi->ModulesCount;
    sm = &smi->Modules[0];
    /* our implementation is a stub for now */
    ok( ModuleCount > 0, "Expected some modules to be loaded\n");

    /* Loop through all the modules/drivers, Wine doesn't get here (yet) */
    for (i = 0; i < ModuleCount ; i++)
    {
        ok( i == sm->LoadOrderIndex, "LoadOrderIndex (%d) should have matched %u\n", sm->LoadOrderIndex, i);
        sm++;
    }

    HeapFree( GetProcessHeap(), 0, smi);
}

static void test_query_handle(void)
{
    NTSTATUS status;
    ULONG ExpectedLength, ReturnLength;
    ULONG SystemInformationLength = sizeof(SYSTEM_HANDLE_INFORMATION);
    SYSTEM_HANDLE_INFORMATION* shi = HeapAlloc(GetProcessHeap(), 0, SystemInformationLength);
    HANDLE EventHandle;
    BOOL found;
    INT i;

    EventHandle = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok( EventHandle != NULL, "CreateEventA failed %u\n", GetLastError() );

    /* Request the needed length : a SystemInformationLength greater than one struct sets ReturnLength */
    ReturnLength = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
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
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status );
    ExpectedLength = FIELD_OFFSET(SYSTEM_HANDLE_INFORMATION, Handle[shi->Count]);
    ok( ReturnLength == ExpectedLength || broken(ReturnLength == ExpectedLength - sizeof(DWORD)), /* Vista / 2008 */
        "Expected length %u, got %u\n", ExpectedLength, ReturnLength );
    ok( shi->Count > 1, "Expected more than 1 handle, got %u\n", shi->Count );
    ok( shi->Handle[1].HandleValue != 0x5555 || broken( shi->Handle[1].HandleValue == 0x5555 ), /* Vista / 2008 */
        "Uninitialized second handle\n" );
    if (shi->Handle[1].HandleValue == 0x5555)
    {
        win_skip("Skipping broken SYSTEM_HANDLE_INFORMATION\n");
        CloseHandle(EventHandle);
        goto done;
    }

    for (i = 0, found = FALSE; i < shi->Count && !found; i++)
        found = (shi->Handle[i].OwnerPid == GetCurrentProcessId()) &&
                ((HANDLE)(ULONG_PTR)shi->Handle[i].HandleValue == EventHandle);
    ok( found, "Expected to find event handle %p (pid %x) in handle list\n", EventHandle, GetCurrentProcessId() );

    if (!found)
        for (i = 0; i < shi->Count; i++)
            trace( "%d: handle %x pid %x\n", i, shi->Handle[i].HandleValue, shi->Handle[i].OwnerPid );

    CloseHandle(EventHandle);

    ReturnLength = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);
    while (status == STATUS_INFO_LENGTH_MISMATCH) /* Vista / 2008 */
    {
        SystemInformationLength *= 2;
        shi = HeapReAlloc(GetProcessHeap(), 0, shi, SystemInformationLength);
        status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);
    }
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status );
    for (i = 0, found = FALSE; i < shi->Count && !found; i++)
        found = (shi->Handle[i].OwnerPid == GetCurrentProcessId()) &&
                ((HANDLE)(ULONG_PTR)shi->Handle[i].HandleValue == EventHandle);
    ok( !found, "Unexpectedly found event handle in handle list\n" );

    status = pNtQuerySystemInformation(SystemHandleInformation, NULL, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %08x\n", status );

done:
    HeapFree( GetProcessHeap(), 0, shi);
}

static void test_query_handle_ex(void)
{
    NTSTATUS status;
    ULONG ExpectedLength, ReturnLength;
    ULONG SystemInformationLength = sizeof(SYSTEM_HANDLE_INFORMATION_EX);
    SYSTEM_HANDLE_INFORMATION_EX* shi = HeapAlloc(GetProcessHeap(), 0, SystemInformationLength);
    HANDLE EventHandle;
    BOOL found;
    INT i;

    EventHandle = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok( EventHandle != NULL, "CreateEventA failed %u\n", GetLastError() );

    ReturnLength = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemExtendedHandleInformation, shi, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
    ok( ReturnLength != 0xdeadbeef, "Expected valid ReturnLength\n" );

    SystemInformationLength = ReturnLength;
    shi = HeapReAlloc(GetProcessHeap(), 0, shi , SystemInformationLength);
    memset(shi, 0x55, SystemInformationLength);

    ReturnLength = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemExtendedHandleInformation, shi, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status );
    ExpectedLength = FIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[shi->NumberOfHandles]);
    ok( ReturnLength == ExpectedLength, "Expected length %u, got %u\n", ExpectedLength, ReturnLength );
    ok( shi->NumberOfHandles > 1, "Expected more than 1 handle, got %u\n", (DWORD)shi->NumberOfHandles );

    for (i = 0, found = FALSE; i < shi->NumberOfHandles && !found; i++)
        found = (shi->Handles[i].UniqueProcessId == GetCurrentProcessId()) &&
                ((HANDLE)(ULONG_PTR)shi->Handles[i].HandleValue == EventHandle);
    ok( found, "Expected to find event handle %p (pid %x) in handle list\n", EventHandle, GetCurrentProcessId() );

    if (!found)
    {
        for (i = 0; i < shi->NumberOfHandles; i++)
            trace( "%d: handle %x pid %x\n", i, (DWORD)shi->Handles[i].HandleValue, (DWORD)shi->Handles[i].UniqueProcessId );
    }

    CloseHandle(EventHandle);

    ReturnLength = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemExtendedHandleInformation, shi, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status );
    for (i = 0, found = FALSE; i < shi->NumberOfHandles && !found; i++)
        found = (shi->Handles[i].UniqueProcessId == GetCurrentProcessId()) &&
                ((HANDLE)(ULONG_PTR)shi->Handles[i].HandleValue == EventHandle);
    ok( !found, "Unexpectedly found event handle in handle list\n" );

    status = pNtQuerySystemInformation(SystemExtendedHandleInformation, NULL, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %08x\n", status );

    HeapFree( GetProcessHeap(), 0, shi);
}

static void test_query_cache(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    BYTE buffer[128];
    SYSTEM_CACHE_INFORMATION *sci = (SYSTEM_CACHE_INFORMATION *) buffer;
    ULONG expected;
    INT i;

    /* the large SYSTEM_CACHE_INFORMATION on WIN64 is not documented */
    expected = sizeof(SYSTEM_CACHE_INFORMATION);
    for (i = sizeof(buffer); i>= expected; i--)
    {
        ReturnLength = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemFileCacheInformation, sci, i, &ReturnLength);
        ok(!status && (ReturnLength == expected),
            "%d: got 0x%x and %u (expected STATUS_SUCCESS and %u)\n", i, status, ReturnLength, expected);
    }

    /* buffer too small for the full result.
       Up to win7, the function succeeds with a partial result. */
    status = pNtQuerySystemInformation(SystemFileCacheInformation, sci, i, &ReturnLength);
    if (!status)
    {
        expected = offsetof(SYSTEM_CACHE_INFORMATION, MinimumWorkingSet);
        for (; i>= expected; i--)
        {
            ReturnLength = 0xdeadbeef;
            status = pNtQuerySystemInformation(SystemFileCacheInformation, sci, i, &ReturnLength);
            ok(!status && (ReturnLength == expected),
                "%d: got 0x%x and %u (expected STATUS_SUCCESS and %u)\n", i, status, ReturnLength, expected);
        }
    }

    /* buffer too small for the result, this call will always fail */
    ReturnLength = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemFileCacheInformation, sci, i, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH &&
        ((ReturnLength == expected) || broken(!ReturnLength) || broken(ReturnLength == 0xfffffff0)),
        "%d: got 0x%x and %u (expected STATUS_INFO_LENGTH_MISMATCH and %u)\n", i, status, ReturnLength, expected);

    if (0) {
        /* this crashes on some vista / win7 machines */
        ReturnLength = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemFileCacheInformation, sci, 0, &ReturnLength);
        ok( status == STATUS_INFO_LENGTH_MISMATCH &&
            ((ReturnLength == expected) || broken(!ReturnLength) || broken(ReturnLength == 0xfffffff0)),
            "0: got 0x%x and %u (expected STATUS_INFO_LENGTH_MISMATCH and %u)\n", status, ReturnLength, expected);
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
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    NeededLength = sbi.NumberOfProcessors * sizeof(SYSTEM_INTERRUPT_INFORMATION);

    sii = HeapAlloc(GetProcessHeap(), 0, NeededLength);

    status = pNtQuerySystemInformation(SystemInterruptInformation, sii, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    /* Try it for all processors */
    status = pNtQuerySystemInformation(SystemInterruptInformation, sii, NeededLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);

    /* Windows XP and W2K3 (and others?) always return 0 for the ReturnLength
     * No test added for this as it's highly unlikely that an app depends on this
    */

    HeapFree( GetProcessHeap(), 0, sii);
}

static void test_query_kerndebug(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION skdi;

    status = pNtQuerySystemInformation(SystemKernelDebuggerInformation, &skdi, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    status = pNtQuerySystemInformation(SystemKernelDebuggerInformation, &skdi, sizeof(skdi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( sizeof(skdi) == ReturnLength, "Inconsistent length %d\n", ReturnLength);

    status = pNtQuerySystemInformation(SystemKernelDebuggerInformation, &skdi, sizeof(skdi) + 2, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( sizeof(skdi) == ReturnLength, "Inconsistent length %d\n", ReturnLength);
}

static void test_query_regquota(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    SYSTEM_REGISTRY_QUOTA_INFORMATION srqi;

    status = pNtQuerySystemInformation(SystemRegistryQuotaInformation, &srqi, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    status = pNtQuerySystemInformation(SystemRegistryQuotaInformation, &srqi, sizeof(srqi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( sizeof(srqi) == ReturnLength, "Inconsistent length %d\n", ReturnLength);

    status = pNtQuerySystemInformation(SystemRegistryQuotaInformation, &srqi, sizeof(srqi) + 2, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( sizeof(srqi) == ReturnLength, "Inconsistent length %d\n", ReturnLength);
}

static void test_query_logicalproc(void)
{
    NTSTATUS status;
    ULONG len, i, proc_no;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *slpi;
    SYSTEM_INFO si;

    GetSystemInfo(&si);

    status = pNtQuerySystemInformation(SystemLogicalProcessorInformation, NULL, 0, &len);
    if(status == STATUS_INVALID_INFO_CLASS)
    {
        win_skip("SystemLogicalProcessorInformation is not supported\n");
        return;
    }
    if(status == STATUS_NOT_IMPLEMENTED)
    {
        todo_wine ok(0, "SystemLogicalProcessorInformation is not implemented\n");
        return;
    }
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
    ok(len%sizeof(*slpi) == 0, "Incorrect length %d\n", len);

    slpi = HeapAlloc(GetProcessHeap(), 0, len);
    status = pNtQuerySystemInformation(SystemLogicalProcessorInformation, slpi, len, &len);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);

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
        ok(proc_no == si.dwNumberOfProcessors, "Incorrect number of logical processors: %d, expected %d\n",
                proc_no, si.dwNumberOfProcessors);

    HeapFree(GetProcessHeap(), 0, slpi);
}

static void test_query_logicalprocex(void)
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *infoex, *infoex2;
    DWORD relationship, len2, len;
    NTSTATUS status;
    BOOL ret;

    if (!pNtQuerySystemInformationEx)
        return;

    len = 0;
    relationship = RelationProcessorCore;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), NULL, 0, &len);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got 0x%08x\n", status);
    ok(len > 0, "got %u\n", len);

    len = 0;
    relationship = RelationAll;
    status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), NULL, 0, &len);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got 0x%08x\n", status);
    ok(len > 0, "got %u\n", len);

    len2 = 0;
    ret = pGetLogicalProcessorInformationEx(RelationAll, NULL, &len2);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %d, error %d\n", ret, GetLastError());
    ok(len == len2, "got %u, expected %u\n", len2, len);

    if (len && len == len2) {
        int j, i;

        infoex = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
        infoex2 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);

        status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship), infoex, len, &len);
        ok(status == STATUS_SUCCESS, "got 0x%08x\n", status);

        ret = pGetLogicalProcessorInformationEx(RelationAll, infoex2, &len2);
        ok(ret, "got %d, error %d\n", ret, GetLastError());
        ok(!memcmp(infoex, infoex2, len), "returned info data mismatch\n");

        for(i = 0; status == STATUS_SUCCESS && i < len; ){
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *ex = (void*)(((char *)infoex) + i);

            ok(ex->Relationship >= RelationProcessorCore && ex->Relationship <= RelationGroup,
                    "Got invalid relationship value: 0x%x\n", ex->Relationship);
            if (!ex->Size)
            {
                ok(0, "got infoex[%u].Size=0\n", i);
                break;
            }

            trace("infoex[%u].Size: %u\n", i, ex->Size);
            switch(ex->Relationship){
            case RelationProcessorCore:
            case RelationProcessorPackage:
                trace("infoex[%u].Relationship: 0x%x (Core == 0x0 or Package == 0x3)\n", i, ex->Relationship);
                trace("infoex[%u].Processor.Flags: 0x%x\n", i, ex->Processor.Flags);
#ifndef __REACTOS__
                trace("infoex[%u].Processor.EfficiencyClass: 0x%x\n", i, ex->Processor.EfficiencyClass);
#endif
                trace("infoex[%u].Processor.GroupCount: 0x%x\n", i, ex->Processor.GroupCount);
                for(j = 0; j < ex->Processor.GroupCount; ++j){
                    trace("infoex[%u].Processor.GroupMask[%u].Mask: 0x%lx\n", i, j, ex->Processor.GroupMask[j].Mask);
                    trace("infoex[%u].Processor.GroupMask[%u].Group: 0x%x\n", i, j, ex->Processor.GroupMask[j].Group);
                }
                break;
            case RelationNumaNode:
                trace("infoex[%u].Relationship: 0x%x (NumaNode)\n", i, ex->Relationship);
                trace("infoex[%u].NumaNode.NodeNumber: 0x%x\n", i, ex->NumaNode.NodeNumber);
                trace("infoex[%u].NumaNode.GroupMask.Mask: 0x%lx\n", i, ex->NumaNode.GroupMask.Mask);
                trace("infoex[%u].NumaNode.GroupMask.Group: 0x%x\n", i, ex->NumaNode.GroupMask.Group);
                break;
            case RelationCache:
                trace("infoex[%u].Relationship: 0x%x (Cache)\n", i, ex->Relationship);
                trace("infoex[%u].Cache.Level: 0x%x\n", i, ex->Cache.Level);
                trace("infoex[%u].Cache.Associativity: 0x%x\n", i, ex->Cache.Associativity);
                trace("infoex[%u].Cache.LineSize: 0x%x\n", i, ex->Cache.LineSize);
                trace("infoex[%u].Cache.CacheSize: 0x%x\n", i, ex->Cache.CacheSize);
                trace("infoex[%u].Cache.Type: 0x%x\n", i, ex->Cache.Type);
                trace("infoex[%u].Cache.GroupMask.Mask: 0x%lx\n", i, ex->Cache.GroupMask.Mask);
                trace("infoex[%u].Cache.GroupMask.Group: 0x%x\n", i, ex->Cache.GroupMask.Group);
                break;
            case RelationGroup:
                trace("infoex[%u].Relationship: 0x%x (Group)\n", i, ex->Relationship);
                trace("infoex[%u].Group.MaximumGroupCount: 0x%x\n", i, ex->Group.MaximumGroupCount);
                trace("infoex[%u].Group.ActiveGroupCount: 0x%x\n", i, ex->Group.ActiveGroupCount);
                for(j = 0; j < ex->Group.ActiveGroupCount; ++j){
                    trace("infoex[%u].Group.GroupInfo[%u].MaximumProcessorCount: 0x%x\n", i, j, ex->Group.GroupInfo[j].MaximumProcessorCount);
                    trace("infoex[%u].Group.GroupInfo[%u].ActiveProcessorCount: 0x%x\n", i, j, ex->Group.GroupInfo[j].ActiveProcessorCount);
                    trace("infoex[%u].Group.GroupInfo[%u].ActiveProcessorMask: 0x%lx\n", i, j, ex->Group.GroupInfo[j].ActiveProcessorMask);
                }
                break;
            default:
                break;
            }

            i += ex->Size;
        }

        HeapFree(GetProcessHeap(), 0, infoex);
        HeapFree(GetProcessHeap(), 0, infoex2);
    }
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
            ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);

            for(i = 0; i < si.dwNumberOfProcessors; i++)
                ppi[i].Number = 0xDEADBEEF;
            status = pNtPowerInformation(ProcessorInformation, 0, 0, ppi, sizeof(PROCESSOR_POWER_INFORMATION) - 1);
            ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
            for(i = 0; i < si.dwNumberOfProcessors; i++)
                if (ppi[i].Number != 0xDEADBEEF) break;
            ok( i == si.dwNumberOfProcessors, "Expected untouched buffer\n");
        }
        else
        {
            /* picky version found on newer Windows like Win7 */
            ok( ppi[1].Number == 0xDEADBEEF, "Expected untouched buffer.\n");
            ok( status == STATUS_BUFFER_TOO_SMALL, "Expected STATUS_BUFFER_TOO_SMALL, got %08x\n", status);

            status = pNtPowerInformation(ProcessorInformation, 0, 0, 0, size);
            ok( status == STATUS_SUCCESS || status == STATUS_INVALID_PARAMETER, "Got %08x\n", status);

            status = pNtPowerInformation(ProcessorInformation, 0, 0, ppi, 0);
            ok( status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INVALID_PARAMETER, "Got %08x\n", status);
        }
    }
    else
    {
        skip("Test needs more than one processor.\n");
    }

    status = pNtPowerInformation(ProcessorInformation, 0, 0, ppi, size);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);

    HeapFree(GetProcessHeap(), 0, ppi);
}

static void test_query_process_wow64(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    ULONG_PTR pbi[2], dummy;

    memset(&dummy, 0xcc, sizeof(dummy));

    /* Do not give a handle and buffer */
    status = pNtQueryInformationProcess(NULL, ProcessWow64Information, NULL, 0, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    /* Use a correct info class and buffer size, but still no handle and buffer */
    status = pNtQueryInformationProcess(NULL, ProcessWow64Information, NULL, sizeof(ULONG_PTR), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE, got %08x\n", status);

    /* Use a correct info class, buffer size and handle, but no buffer */
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, NULL, sizeof(ULONG_PTR), NULL);
    ok( status == STATUS_ACCESS_VIOLATION , "Expected STATUS_ACCESS_VIOLATION, got %08x\n", status);

    /* Use a correct info class, buffer and buffer size, but no handle */
    pbi[0] = pbi[1] = dummy;
    status = pNtQueryInformationProcess(NULL, ProcessWow64Information, pbi, sizeof(ULONG_PTR), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08x\n", status);
    ok( pbi[0] == dummy, "pbi[0] changed to %lx\n", pbi[0]);
    ok( pbi[1] == dummy, "pbi[1] changed to %lx\n", pbi[1]);

    /* Use a greater buffer size */
    pbi[0] = pbi[1] = dummy;
    status = pNtQueryInformationProcess(NULL, ProcessWow64Information, pbi, sizeof(ULONG_PTR) + 1, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
    ok( pbi[0] == dummy, "pbi[0] changed to %lx\n", pbi[0]);
    ok( pbi[1] == dummy, "pbi[1] changed to %lx\n", pbi[1]);

    /* Use no ReturnLength */
    pbi[0] = pbi[1] = dummy;
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, pbi, sizeof(ULONG_PTR), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    trace( "Platform is_wow64 %d, ProcessInformation of ProcessWow64Information %lx\n", is_wow64, pbi[0]);
    ok( is_wow64 == (pbi[0] != 0), "is_wow64 %x, pbi[0] %lx\n", is_wow64, pbi[0]);
    ok( pbi[0] != dummy, "pbi[0] %lx\n", pbi[0]);
    ok( pbi[1] == dummy, "pbi[1] changed to %lx\n", pbi[1]);
    /* Test written size on 64 bit by checking high 32 bit buffer */
    if (sizeof(ULONG_PTR) > sizeof(DWORD))
    {
        DWORD *ptr = (DWORD *)pbi;
        ok( ptr[1] != (DWORD)dummy, "ptr[1] unchanged!\n");
    }

    /* Finally some correct calls */
    pbi[0] = pbi[1] = dummy;
    ReturnLength = 0xdeadbeef;
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, pbi, sizeof(ULONG_PTR), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( is_wow64 == (pbi[0] != 0), "is_wow64 %x, pbi[0] %lx\n", is_wow64, pbi[0]);
    ok( pbi[1] == dummy, "pbi[1] changed to %lx\n", pbi[1]);
    ok( ReturnLength == sizeof(ULONG_PTR), "Inconsistent length %d\n", ReturnLength);

    /* Everything is correct except a too small buffer size */
    pbi[0] = pbi[1] = dummy;
    ReturnLength = 0xdeadbeef;
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, pbi, sizeof(ULONG_PTR) - 1, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
    ok( pbi[0] == dummy, "pbi[0] changed to %lx\n", pbi[0]);
    ok( pbi[1] == dummy, "pbi[1] changed to %lx\n", pbi[1]);
    todo_wine ok( ReturnLength == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", ReturnLength);

    /* Everything is correct except a too large buffer size */
    pbi[0] = pbi[1] = dummy;
    ReturnLength = 0xdeadbeef;
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, pbi, sizeof(ULONG_PTR) + 1, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
    ok( pbi[0] == dummy, "pbi[0] changed to %lx\n", pbi[0]);
    ok( pbi[1] == dummy, "pbi[1] changed to %lx\n", pbi[1]);
    todo_wine ok( ReturnLength == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", ReturnLength);
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

    /* Use a nonexistent info class */
    trace("Check nonexistent info class\n");
    status = pNtQueryInformationProcess(NULL, -1, NULL, 0, NULL);
    ok( status == STATUS_INVALID_INFO_CLASS || status == STATUS_NOT_IMPLEMENTED /* vista */,
        "Expected STATUS_INVALID_INFO_CLASS or STATUS_NOT_IMPLEMENTED, got %08x\n", status);

    /* Do not give a handle and buffer */
    trace("Check NULL handle and buffer and zero-length buffersize\n");
    status = pNtQueryInformationProcess(NULL, ProcessBasicInformation, NULL, 0, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    /* Use a correct info class and buffer size, but still no handle and buffer */
    trace("Check NULL handle and buffer\n");
    status = pNtQueryInformationProcess(NULL, ProcessBasicInformation, NULL, sizeof(pbi), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08x\n", status);

    /* Use a correct info class and buffer size, but still no handle */
    trace("Check NULL handle\n");
    status = pNtQueryInformationProcess(NULL, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08x\n", status);

    /* Use a greater buffer size */
    trace("Check NULL handle and too large buffersize\n");
    status = pNtQueryInformationProcess(NULL, ProcessBasicInformation, &pbi, sizeof(pbi) * 2, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    /* Use no ReturnLength */
    trace("Check NULL ReturnLength\n");
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);

    /* Finally some correct calls */
    trace("Check with correct parameters\n");
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( sizeof(pbi) == ReturnLength, "Inconsistent length %d\n", ReturnLength);

    /* Everything is correct except a too large buffersize */
    trace("Too large buffersize\n");
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
    ok( sizeof(pbi) == ReturnLength, "Inconsistent length %d\n", ReturnLength);
                                                                                                                                               
    /* Check if we have some return values */
    trace("ProcessID : %lx\n", pbi.UniqueProcessId);
    ok( pbi.UniqueProcessId > 0, "Expected a ProcessID > 0, got 0\n");
}

static void dump_vm_counters(const char *header, const VM_COUNTERS *pvi)
{
    trace("%s:\n", header);
    trace("PeakVirtualSize           : %lu\n", pvi->PeakVirtualSize);
    trace("VirtualSize               : %lu\n", pvi->VirtualSize);
    trace("PageFaultCount            : %u\n",  pvi->PageFaultCount);
    trace("PeakWorkingSetSize        : %lu\n", pvi->PeakWorkingSetSize);
    trace("WorkingSetSize            : %lu\n", pvi->WorkingSetSize);
    trace("QuotaPeakPagedPoolUsage   : %lu\n", pvi->QuotaPeakPagedPoolUsage);
    trace("QuotaPagedPoolUsage       : %lu\n", pvi->QuotaPagedPoolUsage);
    trace("QuotaPeakNonPagePoolUsage : %lu\n", pvi->QuotaPeakNonPagedPoolUsage);
    trace("QuotaNonPagePoolUsage     : %lu\n", pvi->QuotaNonPagedPoolUsage);
    trace("PagefileUsage             : %lu\n", pvi->PagefileUsage);
    trace("PeakPagefileUsage         : %lu\n", pvi->PeakPagefileUsage);
}

static void test_query_process_vm(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    VM_COUNTERS pvi;
#ifndef __REACTOS__
    ULONG old_size = FIELD_OFFSET(VM_COUNTERS,PrivatePageCount);
#endif
    HANDLE process;
    SIZE_T prev_size;
    const SIZE_T alloc_size = 16 * 1024 * 1024;
    void *ptr;

    status = pNtQueryInformationProcess(NULL, ProcessVmCounters, NULL, sizeof(pvi), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08x\n", status);

#ifndef __REACTOS__
    status = pNtQueryInformationProcess(NULL, ProcessVmCounters, &pvi, old_size, NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08x\n", status);
#endif

    /* Windows XP and W2K3 will report success for a size of 44 AND 48 !
       Windows W2K will only report success for 44.
       For now we only care for 44, which is FIELD_OFFSET(VM_COUNTERS,PrivatePageCount))
    */

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessVmCounters, &pvi, 24, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

#ifndef __REACTOS__
    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessVmCounters, &pvi, old_size, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( old_size == ReturnLength, "Inconsistent length %d\n", ReturnLength);
#endif

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessVmCounters, &pvi, 46, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
#ifndef __REACTOS__
    ok( ReturnLength == old_size || ReturnLength == sizeof(pvi), "Inconsistent length %d\n", ReturnLength);
#endif

    /* Check if we have some return values */
    dump_vm_counters("VM counters for GetCurrentProcess", &pvi);
    ok( pvi.WorkingSetSize > 0, "Expected a WorkingSetSize > 0\n");
    ok( pvi.PagefileUsage > 0, "Expected a PagefileUsage > 0\n");

    process = OpenProcess(PROCESS_VM_READ, FALSE, GetCurrentProcessId());
    status = pNtQueryInformationProcess(process, ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_ACCESS_DENIED, "Expected STATUS_ACCESS_DENIED, got %08x\n", status);
    CloseHandle(process);

    process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, GetCurrentProcessId());
    status = pNtQueryInformationProcess(process, ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS || broken(!process) /* XP */, "Expected STATUS_SUCCESS, got %08x\n", status);
    CloseHandle(process);

    memset(&pvi, 0, sizeof(pvi));
    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
    status = pNtQueryInformationProcess(process, ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);

    /* Check if we have some return values */
    dump_vm_counters("VM counters for GetCurrentProcessId", &pvi);
    ok( pvi.WorkingSetSize > 0, "Expected a WorkingSetSize > 0\n");
    ok( pvi.PagefileUsage > 0, "Expected a PagefileUsage > 0\n");

    CloseHandle(process);

    /* Check if we have real counters */
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    prev_size = pvi.VirtualSize;
    if (winetest_debug > 1)
        dump_vm_counters("VM counters before VirtualAlloc", &pvi);
    ptr = VirtualAlloc(NULL, alloc_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    ok( ptr != NULL, "VirtualAlloc failed, err %u\n", GetLastError());
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    if (winetest_debug > 1)
        dump_vm_counters("VM counters after VirtualAlloc", &pvi);
    todo_wine ok( pvi.VirtualSize >= prev_size + alloc_size,
        "Expected to be greater than %lu, got %lu\n", prev_size + alloc_size, pvi.VirtualSize);
    VirtualFree( ptr, 0, MEM_RELEASE);

    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    prev_size = pvi.VirtualSize;
    if (winetest_debug > 1)
        dump_vm_counters("VM counters before VirtualAlloc", &pvi);
    ptr = VirtualAlloc(NULL, alloc_size, MEM_RESERVE, PAGE_READWRITE);
    ok( ptr != NULL, "VirtualAlloc failed, err %u\n", GetLastError());
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    if (winetest_debug > 1)
        dump_vm_counters("VM counters after VirtualAlloc(MEM_RESERVE)", &pvi);
    todo_wine ok( pvi.VirtualSize >= prev_size + alloc_size,
        "Expected to be greater than %lu, got %lu\n", prev_size + alloc_size, pvi.VirtualSize);
    prev_size = pvi.VirtualSize;

    ptr = VirtualAlloc(ptr, alloc_size, MEM_COMMIT, PAGE_READWRITE);
    ok( ptr != NULL, "VirtualAlloc failed, err %u\n", GetLastError());
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    if (winetest_debug > 1)
        dump_vm_counters("VM counters after VirtualAlloc(MEM_COMMIT)", &pvi);
    ok( pvi.VirtualSize == prev_size,
        "Expected to equal to %lu, got %lu\n", prev_size, pvi.VirtualSize);
    VirtualFree( ptr, 0, MEM_RELEASE);
}

static void test_query_process_io(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    IO_COUNTERS pii;

    /* NT4 doesn't support this information class, so check for it */
    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessIoCounters, &pii, sizeof(pii), &ReturnLength);
    if (status == STATUS_NOT_SUPPORTED)
    {
        win_skip("ProcessIoCounters information class is not supported\n");
        return;
    }
 
    status = pNtQueryInformationProcess(NULL, ProcessIoCounters, NULL, sizeof(pii), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08x\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessIoCounters, &pii, sizeof(pii), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08x\n", status);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessIoCounters, &pii, 24, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessIoCounters, &pii, sizeof(pii), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( sizeof(pii) == ReturnLength, "Inconsistent length %d\n", ReturnLength);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessIoCounters, &pii, sizeof(pii) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
    ok( sizeof(pii) == ReturnLength, "Inconsistent length %d\n", ReturnLength);

    /* Check if we have some return values */
    trace("OtherOperationCount : 0x%s\n", wine_dbgstr_longlong(pii.OtherOperationCount));
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

    status = pNtQueryInformationProcess(NULL, ProcessTimes, NULL, sizeof(spti), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08x\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessTimes, &spti, sizeof(spti), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08x\n", status);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessTimes, &spti, 24, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, one_before_last_pid);
    if (!process)
    {
        trace("Could not open process with ID : %d, error : %u. Going to use current one.\n", one_before_last_pid, GetLastError());
        process = GetCurrentProcess();
        trace("ProcessTimes for current process\n");
    }
    else
        trace("ProcessTimes for process with ID : %d\n", one_before_last_pid);

    status = pNtQueryInformationProcess( process, ProcessTimes, &spti, sizeof(spti), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( sizeof(spti) == ReturnLength, "Inconsistent length %d\n", ReturnLength);
    CloseHandle(process);

    FileTimeToSystemTime((const FILETIME *)&spti.CreateTime, &UTC);
    SystemTimeToTzSpecificLocalTime(NULL, &UTC, &Local);
    trace("CreateTime : %02d/%02d/%04d %02d:%02d:%02d\n", Local.wMonth, Local.wDay, Local.wYear,
           Local.wHour, Local.wMinute, Local.wSecond);

    FileTimeToSystemTime((const FILETIME *)&spti.ExitTime, &UTC);
    SystemTimeToTzSpecificLocalTime(NULL, &UTC, &Local);
    trace("ExitTime   : %02d/%02d/%04d %02d:%02d:%02d\n", Local.wMonth, Local.wDay, Local.wYear,
           Local.wHour, Local.wMinute, Local.wSecond);

    FileTimeToSystemTime((const FILETIME *)&spti.KernelTime, &Local);
    trace("KernelTime : %02d:%02d:%02d.%03d\n", Local.wHour, Local.wMinute, Local.wSecond, Local.wMilliseconds);

    FileTimeToSystemTime((const FILETIME *)&spti.UserTime, &Local);
    trace("UserTime   : %02d:%02d:%02d.%03d\n", Local.wHour, Local.wMinute, Local.wSecond, Local.wMilliseconds);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessTimes, &spti, sizeof(spti) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
    ok( sizeof(spti) == ReturnLength ||
        ReturnLength == 0 /* vista */ ||
        broken(is_wow64),  /* returns garbage on wow64 */
        "Inconsistent length %d\n", ReturnLength);
}

static void test_query_process_debug_port(int argc, char **argv)
{
    DWORD_PTR debug_port = 0xdeadbeef;
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    NTSTATUS status;
    BOOL ret;

    sprintf(cmdline, "%s %s %s", argv[0], argv[1], "debuggee");

    si.cb = sizeof(si);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed, last error %#x.\n", GetLastError());
    if (!ret) return;

    status = pNtQueryInformationProcess(NULL, ProcessDebugPort,
            NULL, 0, NULL);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %#x.\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessDebugPort,
            NULL, sizeof(debug_port), NULL);
    ok(status == STATUS_INVALID_HANDLE || status == STATUS_ACCESS_VIOLATION,
            "Expected STATUS_INVALID_HANDLE, got %#x.\n", status);

    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort,
            NULL, sizeof(debug_port), NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %#x.\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessDebugPort,
            &debug_port, sizeof(debug_port), NULL);
    ok(status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %#x.\n", status);

    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort,
            &debug_port, sizeof(debug_port) - 1, NULL);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %#x.\n", status);

    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort,
            &debug_port, sizeof(debug_port) + 1, NULL);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %#x.\n", status);

    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort,
            &debug_port, sizeof(debug_port), NULL);
    ok(!status, "NtQueryInformationProcess failed, status %#x.\n", status);
    ok(debug_port == 0, "Expected port 0, got %#lx.\n", debug_port);

    status = pNtQueryInformationProcess(pi.hProcess, ProcessDebugPort,
            &debug_port, sizeof(debug_port), NULL);
    ok(!status, "NtQueryInformationProcess failed, status %#x.\n", status);
    ok(debug_port == ~(DWORD_PTR)0, "Expected port %#lx, got %#lx.\n", ~(DWORD_PTR)0, debug_port);

    for (;;)
    {
        DEBUG_EVENT ev;

        ret = WaitForDebugEvent(&ev, INFINITE);
        ok(ret, "WaitForDebugEvent failed, last error %#x.\n", GetLastError());
        if (!ret) break;

        if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) break;

        ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
        ok(ret, "ContinueDebugEvent failed, last error %#x.\n", GetLastError());
        if (!ret) break;
    }

    ret = CloseHandle(pi.hThread);
    ok(ret, "CloseHandle failed, last error %#x.\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "CloseHandle failed, last error %#x.\n", GetLastError());
}

static void test_query_process_priority(void)
{
    PROCESS_PRIORITY_CLASS priority[2];
    ULONG ReturnLength;
    DWORD orig_priority;
    NTSTATUS status;
    BOOL ret;

    status = pNtQueryInformationProcess(NULL, ProcessPriorityClass, NULL, sizeof(priority[0]), NULL);
    ok(status == STATUS_ACCESS_VIOLATION || broken(status == STATUS_INVALID_HANDLE) /* w2k3 */,
       "Expected STATUS_ACCESS_VIOLATION, got %08x\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessPriorityClass, &priority, sizeof(priority[0]), NULL);
    ok(status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08x\n", status);

    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessPriorityClass, &priority, 1, &ReturnLength);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessPriorityClass, &priority, sizeof(priority), &ReturnLength);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    orig_priority = GetPriorityClass(GetCurrentProcess());
    ret = SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
    ok(ret, "Failed to set priority class: %u\n", GetLastError());

    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessPriorityClass, &priority, sizeof(priority[0]), &ReturnLength);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok(priority[0].PriorityClass == PROCESS_PRIOCLASS_BELOW_NORMAL,
       "Expected PROCESS_PRIOCLASS_BELOW_NORMAL, got %u\n", priority[0].PriorityClass);

    ret = SetPriorityClass(GetCurrentProcess(), orig_priority);
    ok(ret, "Failed to reset priority class: %u\n", GetLastError());
}

static void test_query_process_handlecount(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    DWORD handlecount;
    BYTE buffer[2 * sizeof(DWORD)];
    HANDLE process;

    status = pNtQueryInformationProcess(NULL, ProcessHandleCount, NULL, sizeof(handlecount), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08x\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessHandleCount, &handlecount, sizeof(handlecount), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08x\n", status);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessHandleCount, &handlecount, 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, one_before_last_pid);
    if (!process)
    {
        trace("Could not open process with ID : %d, error : %u. Going to use current one.\n", one_before_last_pid, GetLastError());
        process = GetCurrentProcess();
        trace("ProcessHandleCount for current process\n");
    }
    else
        trace("ProcessHandleCount for process with ID : %d\n", one_before_last_pid);

    status = pNtQueryInformationProcess( process, ProcessHandleCount, &handlecount, sizeof(handlecount), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( sizeof(handlecount) == ReturnLength, "Inconsistent length %d\n", ReturnLength);
    CloseHandle(process);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessHandleCount, buffer, sizeof(buffer), &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_SUCCESS,
        "Expected STATUS_INFO_LENGTH_MISMATCH or STATUS_SUCCESS, got %08x\n", status);
    ok( sizeof(handlecount) == ReturnLength, "Inconsistent length %d\n", ReturnLength);

    /* Check if we have some return values */
    trace("HandleCount : %d\n", handlecount);
    todo_wine
    {
        ok( handlecount > 0, "Expected some handles, got 0\n");
    }
}

static void test_query_process_image_file_name(void)
{
    NTSTATUS status;
    ULONG ReturnLength;
    UNICODE_STRING image_file_name;
    void *buffer;
    char *file_nameA;
    INT len;

    status = pNtQueryInformationProcess(NULL, ProcessImageFileName, &image_file_name, sizeof(image_file_name), NULL);
    if (status == STATUS_INVALID_INFO_CLASS)
    {
        win_skip("ProcessImageFileName is not supported\n");
        return;
    }
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08x\n", status);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessImageFileName, &image_file_name, 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessImageFileName, &image_file_name, sizeof(image_file_name), &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);

    buffer = HeapAlloc(GetProcessHeap(), 0, ReturnLength);
    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessImageFileName, buffer, ReturnLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    memcpy(&image_file_name, buffer, sizeof(image_file_name));
    len = WideCharToMultiByte(CP_ACP, 0, image_file_name.Buffer, image_file_name.Length/sizeof(WCHAR), NULL, 0, NULL, NULL);
    file_nameA = HeapAlloc(GetProcessHeap(), 0, len + 1);
    WideCharToMultiByte(CP_ACP, 0, image_file_name.Buffer, image_file_name.Length/sizeof(WCHAR), file_nameA, len, NULL, NULL);
    file_nameA[len] = '\0';
    HeapFree(GetProcessHeap(), 0, buffer);
    trace("process image file name: %s\n", file_nameA);
    todo_wine ok(strncmp(file_nameA, "\\Device\\", 8) == 0, "Process image name should be an NT path beginning with \\Device\\ (is %s)\n", file_nameA);
    HeapFree(GetProcessHeap(), 0, file_nameA);
}

static void test_query_process_debug_object_handle(int argc, char **argv)
{
    char cmdline[MAX_PATH];
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi;
    BOOL ret;
    HANDLE debug_object;
    NTSTATUS status;

    sprintf(cmdline, "%s %s %s", argv[0], argv[1], "debuggee");

    si.cb = sizeof(si);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL,
                        NULL, &si, &pi);
    ok(ret, "CreateProcess failed with last error %u\n", GetLastError());
    if (!ret) return;

    status = pNtQueryInformationProcess(NULL, ProcessDebugObjectHandle, NULL,
            0, NULL);
    if (status == STATUS_INVALID_INFO_CLASS || status == STATUS_NOT_IMPLEMENTED)
    {
        win_skip("ProcessDebugObjectHandle is not supported\n");
        return;
    }
    ok(status == STATUS_INFO_LENGTH_MISMATCH,
       "Expected NtQueryInformationProcess to return STATUS_INFO_LENGTH_MISMATCH, got 0x%08x\n",
       status);

    status = pNtQueryInformationProcess(NULL, ProcessDebugObjectHandle, NULL,
            sizeof(debug_object), NULL);
    ok(status == STATUS_INVALID_HANDLE ||
       status == STATUS_ACCESS_VIOLATION, /* XP */
       "Expected NtQueryInformationProcess to return STATUS_INVALID_HANDLE, got 0x%08x\n", status);

    status = pNtQueryInformationProcess(GetCurrentProcess(),
            ProcessDebugObjectHandle, NULL, sizeof(debug_object), NULL);
    ok(status == STATUS_ACCESS_VIOLATION,
       "Expected NtQueryInformationProcess to return STATUS_ACCESS_VIOLATION, got 0x%08x\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessDebugObjectHandle,
            &debug_object, sizeof(debug_object), NULL);
    ok(status == STATUS_INVALID_HANDLE,
       "Expected NtQueryInformationProcess to return STATUS_ACCESS_VIOLATION, got 0x%08x\n", status);

    status = pNtQueryInformationProcess(GetCurrentProcess(),
            ProcessDebugObjectHandle, &debug_object,
            sizeof(debug_object) - 1, NULL);
    ok(status == STATUS_INFO_LENGTH_MISMATCH,
       "Expected NtQueryInformationProcess to return STATUS_INFO_LENGTH_MISMATCH, got 0x%08x\n", status);

    status = pNtQueryInformationProcess(GetCurrentProcess(),
            ProcessDebugObjectHandle, &debug_object,
            sizeof(debug_object) + 1, NULL);
    ok(status == STATUS_INFO_LENGTH_MISMATCH,
       "Expected NtQueryInformationProcess to return STATUS_INFO_LENGTH_MISMATCH, got 0x%08x\n", status);

    debug_object = (HANDLE)0xdeadbeef;
    status = pNtQueryInformationProcess(GetCurrentProcess(),
            ProcessDebugObjectHandle, &debug_object,
            sizeof(debug_object), NULL);
    ok(status == STATUS_PORT_NOT_SET,
       "Expected NtQueryInformationProcess to return STATUS_PORT_NOT_SET, got 0x%08x\n", status);
    ok(debug_object == NULL ||
       broken(debug_object == (HANDLE)0xdeadbeef), /* Wow64 */
       "Expected debug object handle to be NULL, got %p\n", debug_object);

    debug_object = (HANDLE)0xdeadbeef;
    status = pNtQueryInformationProcess(pi.hProcess, ProcessDebugObjectHandle,
            &debug_object, sizeof(debug_object), NULL);
#ifndef __REACTOS__
    todo_wine
#endif
    ok(status == STATUS_SUCCESS,
       "Expected NtQueryInformationProcess to return STATUS_SUCCESS, got 0x%08x\n", status);
#ifndef __REACTOS__
    todo_wine
#endif
    ok(debug_object != NULL,
       "Expected debug object handle to be non-NULL, got %p\n", debug_object);
#ifdef __REACTOS__
    status = NtClose( debug_object );
    ok( !status, "NtClose failed %x\n", status );
#endif

    for (;;)
    {
        DEBUG_EVENT ev;

        ret = WaitForDebugEvent(&ev, INFINITE);
        ok(ret, "WaitForDebugEvent failed with last error %u\n", GetLastError());
        if (!ret) break;

        if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) break;

        ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
        ok(ret, "ContinueDebugEvent failed with last error %u\n", GetLastError());
        if (!ret) break;
    }

    ret = CloseHandle(pi.hThread);
    ok(ret, "CloseHandle failed with last error %u\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "CloseHandle failed with last error %u\n", GetLastError());
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
    status = pNtQueryInformationProcess(NULL, ProcessDebugFlags, NULL, 0, NULL);
    ok(status == STATUS_INFO_LENGTH_MISMATCH || broken(status == STATUS_INVALID_INFO_CLASS) /* WOW64 */,
            "Expected STATUS_INFO_LENGTH_MISMATCH, got %#x.\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessDebugFlags, NULL, sizeof(debug_flags), NULL);
    ok(status == STATUS_INVALID_HANDLE || status == STATUS_ACCESS_VIOLATION || broken(status == STATUS_INVALID_INFO_CLASS) /* WOW64 */,
            "Expected STATUS_INVALID_HANDLE, got %#x.\n", status);

    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessDebugFlags,
            NULL, sizeof(debug_flags), NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %#x.\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessDebugFlags,
            &debug_flags, sizeof(debug_flags), NULL);
    ok(status == STATUS_INVALID_HANDLE || broken(status == STATUS_INVALID_INFO_CLASS) /* WOW64 */,
            "Expected STATUS_INVALID_HANDLE, got %#x.\n", status);

    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessDebugFlags,
            &debug_flags, sizeof(debug_flags) - 1, NULL);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %#x.\n", status);

    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessDebugFlags,
            &debug_flags, sizeof(debug_flags) + 1, NULL);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %#x.\n", status);

    /* test ProcessDebugFlags of current process */
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessDebugFlags,
            &debug_flags, sizeof(debug_flags), NULL);
    ok(!status, "NtQueryInformationProcess failed, status %#x.\n", status);
    ok(debug_flags == TRUE, "Expected flag TRUE, got %x.\n", debug_flags);

    for (i = 0; i < sizeof(test_flags)/sizeof(test_flags[0]); i++)
    {
        DWORD expected_flags = !(test_flags[i] & DEBUG_ONLY_THIS_PROCESS);
        sprintf(cmdline, "%s %s %s", argv[0], argv[1], "debuggee");

        si.cb = sizeof(si);
        ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, test_flags[i], NULL, NULL, &si, &pi);
        ok(ret, "CreateProcess failed, last error %#x.\n", GetLastError());

        if (!(test_flags[i] & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)))
        {
            /* test ProcessDebugFlags before attaching with debugger */
            status = pNtQueryInformationProcess(pi.hProcess, ProcessDebugFlags,
                    &debug_flags, sizeof(debug_flags), NULL);
            ok(!status, "NtQueryInformationProcess failed, status %#x.\n", status);
            ok(debug_flags == TRUE, "Expected flag TRUE, got %x.\n", debug_flags);

            ret = DebugActiveProcess(pi.dwProcessId);
            ok(ret, "DebugActiveProcess failed, last error %#x.\n", GetLastError());
            expected_flags = FALSE;
        }

        /* test ProcessDebugFlags after attaching with debugger */
        status = pNtQueryInformationProcess(pi.hProcess, ProcessDebugFlags,
                &debug_flags, sizeof(debug_flags), NULL);
        ok(!status, "NtQueryInformationProcess failed, status %#x.\n", status);
        ok(debug_flags == expected_flags, "Expected flag %x, got %x.\n", expected_flags, debug_flags);

        if (!(test_flags[i] & CREATE_SUSPENDED))
        {
            /* Continue a couple of times to make sure the process is fully initialized,
             * otherwise Windows XP deadlocks in the following DebugActiveProcess(). */
            for (;;)
            {
                ret = WaitForDebugEvent(&ev, 1000);
                disable_success_count
                ok(ret, "WaitForDebugEvent failed, last error %#x.\n", GetLastError());
                if (!ret) break;

                if (ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) break;

                ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
                disable_success_count
                ok(ret, "ContinueDebugEvent failed, last error %#x.\n", GetLastError());
                if (!ret) break;
            }

            result = SuspendThread(pi.hThread);
            ok(result == 0, "Expected 0, got %u.\n", result);
        }

        ret = DebugActiveProcessStop(pi.dwProcessId);
        ok(ret, "DebugActiveProcessStop failed, last error %#x.\n", GetLastError());

        /* test ProcessDebugFlags after detaching debugger */
        status = pNtQueryInformationProcess(pi.hProcess, ProcessDebugFlags,
                &debug_flags, sizeof(debug_flags), NULL);
        ok(!status, "NtQueryInformationProcess failed, status %#x.\n", status);
        ok(debug_flags == expected_flags, "Expected flag %x, got %x.\n", expected_flags, debug_flags);

        ret = DebugActiveProcess(pi.dwProcessId);
        ok(ret, "DebugActiveProcess failed, last error %#x.\n", GetLastError());

        /* test ProcessDebugFlags after re-attaching debugger */
        status = pNtQueryInformationProcess(pi.hProcess, ProcessDebugFlags,
                &debug_flags, sizeof(debug_flags), NULL);
        ok(!status, "NtQueryInformationProcess failed, status %#x.\n", status);
        ok(debug_flags == FALSE, "Expected flag FALSE, got %x.\n", debug_flags);

        result = ResumeThread(pi.hThread);
        todo_wine ok(result == 2, "Expected 2, got %u.\n", result);

        /* Wait until the process is terminated. On Windows XP the process randomly
         * gets stuck in a non-continuable exception, so stop after 100 iterations.
         * On Windows 2003, the debugged process disappears (or stops?) without
         * any EXIT_PROCESS_DEBUG_EVENT after a couple of events. */
        for (j = 0; j < 100; j++)
        {
            ret = WaitForDebugEvent(&ev, 1000);
            disable_success_count
            ok(ret || broken(GetLastError() == ERROR_SEM_TIMEOUT),
                "WaitForDebugEvent failed, last error %#x.\n", GetLastError());
            if (!ret) break;

            if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) break;

            ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
            disable_success_count
            ok(ret, "ContinueDebugEvent failed, last error %#x.\n", GetLastError());
            if (!ret) break;
        }
        ok(j < 100 || broken(j >= 100) /* Win XP */, "Expected less than 100 debug events.\n");

        /* test ProcessDebugFlags after process has terminated */
        status = pNtQueryInformationProcess(pi.hProcess, ProcessDebugFlags,
                &debug_flags, sizeof(debug_flags), NULL);
        ok(!status, "NtQueryInformationProcess failed, status %#x.\n", status);
        ok(debug_flags == FALSE, "Expected flag FALSE, got %x.\n", debug_flags);

        ret = CloseHandle(pi.hThread);
        ok(ret, "CloseHandle failed, last error %#x.\n", GetLastError());
        ret = CloseHandle(pi.hProcess);
        ok(ret, "CloseHandle failed, last error %#x.\n", GetLastError());
    }
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
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( readcount == 12, "Expected to read 12 bytes, got %ld\n",readcount);
    ok( strcmp(teststring, buffer) == 0, "Expected read memory to be the same as original memory\n");

    /* no number of bytes */
    memset(buffer, 0, 12);
    status = pNtReadVirtualMemory(process, teststring, buffer, 12, NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( strcmp(teststring, buffer) == 0, "Expected read memory to be the same as original memory\n");

    /* illegal remote address */
    todo_wine{
    status = pNtReadVirtualMemory(process, (void *) 0x1234, buffer, 12, &readcount);
    ok( status == STATUS_PARTIAL_COPY || broken(status == STATUS_ACCESS_VIOLATION), "Expected STATUS_PARTIAL_COPY, got %08x\n", status);
    if (status == STATUS_PARTIAL_COPY)
        ok( readcount == 0, "Expected to read 0 bytes, got %ld\n",readcount);
    }

    /* 0 handle */
    status = pNtReadVirtualMemory(0, teststring, buffer, 12, &readcount);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08x\n", status);
    ok( readcount == 0, "Expected to read 0 bytes, got %ld\n",readcount);

    /* pseudo handle for current process*/
    memset(buffer, 0, 12);
    status = pNtReadVirtualMemory((HANDLE)-1, teststring, buffer, 12, &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( readcount == 12, "Expected to read 12 bytes, got %ld\n",readcount);
    ok( strcmp(teststring, buffer) == 0, "Expected read memory to be the same as original memory\n");

    /* illegal local address */
    status = pNtReadVirtualMemory(process, teststring, (void *)0x1234, 12, &readcount);
    ok( status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %08x\n", status);
    ok( readcount == 0, "Expected to read 0 bytes, got %ld\n",readcount);

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

    if (!pNtClose) {
        skip("No NtClose ... Win98\n");
        return;
    }
    /* Switch to being a noexec unaware process */
    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &oldflags, sizeof (oldflags), &flagsize);
    if (status == STATUS_INVALID_PARAMETER) {
        skip("Invalid Parameter on ProcessExecuteFlags query?\n");
        return;
    }
    ok( (status == STATUS_SUCCESS) || (status == STATUS_INVALID_INFO_CLASS), "Expected STATUS_SUCCESS, got %08x\n", status);
    status = pNtSetInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &flags, sizeof(flags) );
    ok( (status == STATUS_SUCCESS) || (status == STATUS_INVALID_INFO_CLASS), "Expected STATUS_SUCCESS, got %08x\n", status);

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
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);

    offset.u.LowPart  = 0;
    offset.u.HighPart = 0;
    count = 0x2000;
    addr = NULL;
    status = pNtMapViewOfSection ( h, GetCurrentProcess(), &addr, 0, 0, &offset, &count, ViewShare, 0, PAGE_READWRITE);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);

#if defined(__x86_64__) || defined(__i386__)
    *(unsigned char*)addr = 0xc3;       /* lret ... in both i386 and x86_64 */
#elif defined(__arm__)
    *(unsigned long*)addr = 0xe12fff1e; /* bx lr */
#elif defined(__aarch64__)
    *(unsigned long*)addr = 0xd65f03c0; /* ret */
#else
    ok(0, "Add a return opcode for your architecture or expect a crash in this test\n");
#endif
    trace("trying to execute code in the readwrite only mapped anon file...\n");
    f = addr;f();
    trace("...done.\n");

    status = pNtQueryVirtualMemory( GetCurrentProcess(), addr, MemoryBasicInformation, &info, sizeof(info), &retlen );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( retlen == sizeof(info), "Expected STATUS_SUCCESS, got %08x\n", status);
    ok((info.Protect & ~PAGE_NOCACHE) == PAGE_READWRITE, "addr.Protect is not PAGE_READWRITE, but 0x%x\n", info.Protect);

    status = pNtUnmapViewOfSection( GetCurrentProcess(), (char *)addr + 0x1050 );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    pNtClose (h);

    /* Switch back */
    pNtSetInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &oldflags, sizeof(oldflags) );
}

static void test_queryvirtualmemory(void)
{
    NTSTATUS status;
    SIZE_T readcount;
    static const WCHAR windowsW[] = {'w','i','n','d','o','w','s'};
    static const char teststring[] = "test string";
    static char datatestbuf[42] = "abc";
    static char rwtestbuf[42];
    MEMORY_BASIC_INFORMATION mbi;
    char stackbuf[42];
    HMODULE module;
    char buffer_name[sizeof(MEMORY_SECTION_NAME) + MAX_PATH * sizeof(WCHAR)];
#ifndef __REACTOS__
    MEMORY_SECTION_NAME *msn = (MEMORY_SECTION_NAME *)buffer_name;
#endif
    BOOL found;
    int i;
#ifdef __REACTOS__
    MEMORY_SECTION_NAME *msn = HeapAlloc(GetProcessHeap(), 0, sizeof(buffer_name));
#endif

    module = GetModuleHandleA( "ntdll.dll" );
    trace("Check flags of the PE header of NTDLL.DLL at %p\n", module);
    status = pNtQueryVirtualMemory(NtCurrentProcess(), module, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %ld\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok (mbi.AllocationBase == module, "mbi.AllocationBase is 0x%p, expected 0x%p\n", mbi.AllocationBase, module);
    ok (mbi.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "mbi.AllocationProtect is 0x%x, expected 0x%x\n", mbi.AllocationProtect, PAGE_EXECUTE_WRITECOPY);
    ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%x, expected 0x%x\n", mbi.State, MEM_COMMIT);
    ok (mbi.Protect == PAGE_READONLY, "mbi.Protect is 0x%x, expected 0x%x\n", mbi.Protect, PAGE_READONLY);
    ok (mbi.Type == MEM_IMAGE, "mbi.Type is 0x%x, expected 0x%x\n", mbi.Type, MEM_IMAGE);

    trace("Check flags of a function entry in NTDLL.DLL at %p\n", pNtQueryVirtualMemory);
    module = GetModuleHandleA( "ntdll.dll" );
    status = pNtQueryVirtualMemory(NtCurrentProcess(), pNtQueryVirtualMemory, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %ld\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok (mbi.AllocationBase == module, "mbi.AllocationBase is 0x%p, expected 0x%p\n", mbi.AllocationBase, module);
    ok (mbi.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "mbi.AllocationProtect is 0x%x, expected 0x%x\n", mbi.AllocationProtect, PAGE_EXECUTE_WRITECOPY);
    ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%x, expected 0x%x\n", mbi.State, MEM_COMMIT);
    ok (mbi.Protect == PAGE_EXECUTE_READ, "mbi.Protect is 0x%x, expected 0x%x\n", mbi.Protect, PAGE_EXECUTE_READ);

    trace("Check flags of heap at %p\n", GetProcessHeap());
    status = pNtQueryVirtualMemory(NtCurrentProcess(), GetProcessHeap(), MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %ld\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok (mbi.AllocationProtect == PAGE_READWRITE || mbi.AllocationProtect == PAGE_EXECUTE_READWRITE,
        "mbi.AllocationProtect is 0x%x\n", mbi.AllocationProtect);
    ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%x, expected 0x%x\n", mbi.State, MEM_COMMIT);
    ok (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READWRITE,
        "mbi.Protect is 0x%x\n", mbi.Protect);

    trace("Check flags of stack at %p\n", stackbuf);
    status = pNtQueryVirtualMemory(NtCurrentProcess(), stackbuf, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %ld\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok (mbi.AllocationProtect == PAGE_READWRITE, "mbi.AllocationProtect is 0x%x, expected 0x%x\n", mbi.AllocationProtect, PAGE_READWRITE);
    ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%x, expected 0x%x\n", mbi.State, MEM_COMMIT);
    ok (mbi.Protect == PAGE_READWRITE, "mbi.Protect is 0x%x, expected 0x%x\n", mbi.Protect, PAGE_READWRITE);

    trace("Check flags of read-only data at %p\n", teststring);
    module = GetModuleHandleA( NULL );
    status = pNtQueryVirtualMemory(NtCurrentProcess(), teststring, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %ld\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok (mbi.AllocationBase == module, "mbi.AllocationBase is 0x%p, expected 0x%p\n", mbi.AllocationBase, module);
    ok (mbi.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "mbi.AllocationProtect is 0x%x, expected 0x%x\n", mbi.AllocationProtect, PAGE_EXECUTE_WRITECOPY);
    ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%x, expected 0x%X\n", mbi.State, MEM_COMMIT);
    if (mbi.Protect != PAGE_READONLY)
        todo_wine ok( mbi.Protect == PAGE_READONLY, "mbi.Protect is 0x%x, expected 0x%X\n", mbi.Protect, PAGE_READONLY);

    trace("Check flags of read-write data at %p\n", datatestbuf);
    status = pNtQueryVirtualMemory(NtCurrentProcess(), datatestbuf, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %ld\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    ok (mbi.AllocationBase == module, "mbi.AllocationBase is 0x%p, expected 0x%p\n", mbi.AllocationBase, module);
    ok (mbi.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "mbi.AllocationProtect is 0x%x, expected 0x%x\n", mbi.AllocationProtect, PAGE_EXECUTE_WRITECOPY);
    ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%x, expected 0x%X\n", mbi.State, MEM_COMMIT);
    ok (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_WRITECOPY,
        "mbi.Protect is 0x%x\n", mbi.Protect);

    trace("Check flags of read-write uninitialized data (.bss) at %p\n", rwtestbuf);
    status = pNtQueryVirtualMemory(NtCurrentProcess(), rwtestbuf, MemoryBasicInformation, &mbi, sizeof(MEMORY_BASIC_INFORMATION), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( readcount == sizeof(MEMORY_BASIC_INFORMATION), "Expected to read %d bytes, got %ld\n",(int)sizeof(MEMORY_BASIC_INFORMATION),readcount);
    if (mbi.AllocationBase == module)
    {
        ok (mbi.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "mbi.AllocationProtect is 0x%x, expected 0x%x\n", mbi.AllocationProtect, PAGE_EXECUTE_WRITECOPY);
        ok (mbi.State == MEM_COMMIT, "mbi.State is 0x%x, expected 0x%X\n", mbi.State, MEM_COMMIT);
        ok (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_WRITECOPY,
            "mbi.Protect is 0x%x\n", mbi.Protect);
    }
    else skip( "bss is outside of module\n" );  /* this can happen on Mac OS */

    /* check error code when addr is higher than working set limit */
    status = pNtQueryVirtualMemory(NtCurrentProcess(), (void *)~0, MemoryBasicInformation, &mbi, sizeof(mbi), &readcount);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %08x\n", status);

    trace("Check section name of NTDLL.DLL with invalid size\n");
    module = GetModuleHandleA( "ntdll.dll" );
    memset(msn, 0, sizeof(*msn));
    readcount = 0;
    status = pNtQueryVirtualMemory(NtCurrentProcess(), module, MemoryMappedFilenameInformation, msn, sizeof(*msn), &readcount);
    ok( status == STATUS_BUFFER_OVERFLOW, "Expected STATUS_BUFFER_OVERFLOW, got %08x\n", status);
    ok( readcount > 0, "Expected readcount to be > 0\n");

    trace("Check section name of NTDLL.DLL with invalid size\n");
    module = GetModuleHandleA( "ntdll.dll" );
    memset(msn, 0, sizeof(*msn));
    readcount = 0;
    status = pNtQueryVirtualMemory(NtCurrentProcess(), module, MemoryMappedFilenameInformation, msn, sizeof(*msn) - 1, &readcount);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
    ok( readcount > 0, "Expected readcount to be > 0\n");

    trace("Check section name of NTDLL.DLL\n");
    module = GetModuleHandleA( "ntdll.dll" );
    memset(msn, 0x55, sizeof(*msn));
    memset(buffer_name, 0x77, sizeof(buffer_name));
    readcount = 0;
    status = pNtQueryVirtualMemory(NtCurrentProcess(), module, MemoryMappedFilenameInformation, msn, sizeof(buffer_name), &readcount);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( readcount > 0, "Expected readcount to be > 0\n");
    trace ("Section Name: %s\n", wine_dbgstr_w(msn->SectionFileName.Buffer));
    pRtlDowncaseUnicodeString( &msn->SectionFileName, &msn->SectionFileName, FALSE );
    for (found = FALSE, i = (msn->SectionFileName.Length - sizeof(windowsW)) / sizeof(WCHAR); i >= 0; i--)
        found |= !memcmp( &msn->SectionFileName.Buffer[i], windowsW, sizeof(windowsW) );
    ok( found, "Section name does not contain \"Windows\"\n");

    trace("Check section name of non mapped memory\n");
    memset(msn, 0, sizeof(buffer_name));
    readcount = 0;
    status = pNtQueryVirtualMemory(NtCurrentProcess(), &buffer_name, MemoryMappedFilenameInformation, msn, sizeof(buffer_name), &readcount);
    ok( status == STATUS_INVALID_ADDRESS, "Expected STATUS_INVALID_ADDRESS, got %08x\n", status);
    ok( readcount == 0 || broken(readcount != 0) /* wow64 */, "Expected readcount to be 0\n");

#ifdef __REACTOS__
    HeapFree(GetProcessHeap(), 0, msn);
#endif
}

static void test_affinity(void)
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION pbi;
    DWORD_PTR proc_affinity, thread_affinity;
    THREAD_BASIC_INFORMATION tbi;
    SYSTEM_INFO si;

    GetSystemInfo(&si);
    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), NULL );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    proc_affinity = pbi.AffinityMask;
    ok( proc_affinity == (1 << si.dwNumberOfProcessors) - 1, "Unexpected process affinity\n" );
    proc_affinity = 1 << si.dwNumberOfProcessors;
    status = pNtSetInformationProcess( GetCurrentProcess(), ProcessAffinityMask, &proc_affinity, sizeof(proc_affinity) );
    ok( status == STATUS_INVALID_PARAMETER,
        "Expected STATUS_INVALID_PARAMETER, got %08x\n", status);

    proc_affinity = 0;
    status = pNtSetInformationProcess( GetCurrentProcess(), ProcessAffinityMask, &proc_affinity, sizeof(proc_affinity) );
    ok( status == STATUS_INVALID_PARAMETER,
        "Expected STATUS_INVALID_PARAMETER, got %08x\n", status);

    status = pNtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( tbi.AffinityMask == (1 << si.dwNumberOfProcessors) - 1, "Unexpected thread affinity\n" );
    thread_affinity = 1 << si.dwNumberOfProcessors;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadAffinityMask, &thread_affinity, sizeof(thread_affinity) );
    ok( status == STATUS_INVALID_PARAMETER,
        "Expected STATUS_INVALID_PARAMETER, got %08x\n", status);
    thread_affinity = 0;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadAffinityMask, &thread_affinity, sizeof(thread_affinity) );
    ok( status == STATUS_INVALID_PARAMETER,
        "Expected STATUS_INVALID_PARAMETER, got %08x\n", status);

    thread_affinity = 1;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadAffinityMask, &thread_affinity, sizeof(thread_affinity) );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    status = pNtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( tbi.AffinityMask == 1, "Unexpected thread affinity\n" );

    /* NOTE: Pre-Vista does not recognize the "all processors" flag (all bits set) */
    thread_affinity = ~(DWORD_PTR)0;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadAffinityMask, &thread_affinity, sizeof(thread_affinity) );
    ok( broken(status == STATUS_INVALID_PARAMETER) || status == STATUS_SUCCESS,
        "Expected STATUS_SUCCESS, got %08x\n", status);

    if (si.dwNumberOfProcessors <= 1)
    {
        skip("only one processor, skipping affinity testing\n");
        return;
    }

    /* Test thread affinity mask resulting from "all processors" flag */
    if (status == STATUS_SUCCESS)
    {
        status = pNtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
        ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
        ok( broken(tbi.AffinityMask == 1) || tbi.AffinityMask == (1 << si.dwNumberOfProcessors) - 1,
            "Unexpected thread affinity\n" );
    }
    else
        skip("Cannot test thread affinity mask for 'all processors' flag\n");

    proc_affinity = 2;
    status = pNtSetInformationProcess( GetCurrentProcess(), ProcessAffinityMask, &proc_affinity, sizeof(proc_affinity) );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), NULL );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    proc_affinity = pbi.AffinityMask;
    ok( proc_affinity == 2, "Unexpected process affinity\n" );
    /* Setting the process affinity changes the thread affinity to match */
    status = pNtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( tbi.AffinityMask == 2, "Unexpected thread affinity\n" );
    /* The thread affinity is restricted to the process affinity */
    thread_affinity = 1;
    status = pNtSetInformationThread( GetCurrentThread(), ThreadAffinityMask, &thread_affinity, sizeof(thread_affinity) );
    ok( status == STATUS_INVALID_PARAMETER,
        "Expected STATUS_INVALID_PARAMETER, got %08x\n", status);

    proc_affinity = (1 << si.dwNumberOfProcessors) - 1;
    status = pNtSetInformationProcess( GetCurrentProcess(), ProcessAffinityMask, &proc_affinity, sizeof(proc_affinity) );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    /* Resetting the process affinity also resets the thread affinity */
    status = pNtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok( tbi.AffinityMask == (1 << si.dwNumberOfProcessors) - 1,
        "Unexpected thread affinity\n" );
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
    trace("dwNumberOfProcessors: %d, current processor: %d\n", si.dwNumberOfProcessors, current_cpu);

    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    old_process_mask = pbi.AffinityMask;
    ok(status == STATUS_SUCCESS, "got 0x%x (expected STATUS_SUCCESS)\n", status);

    status = pNtQueryInformationThread(GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL);
    old_thread_mask = tbi.AffinityMask;
    ok(status == STATUS_SUCCESS, "got 0x%x (expected STATUS_SUCCESS)\n", status);

    /* allow the test to run on all processors */
    new_mask = (1 << si.dwNumberOfProcessors) - 1;
    status = pNtSetInformationProcess(GetCurrentProcess(), ProcessAffinityMask, &new_mask, sizeof(new_mask));
    ok(status == STATUS_SUCCESS, "got 0x%x (expected STATUS_SUCCESS)\n", status);

    for (i = 0; i < si.dwNumberOfProcessors; i++)
    {
        new_mask = 1 << i;
        status = pNtSetInformationThread(GetCurrentThread(), ThreadAffinityMask, &new_mask, sizeof(new_mask));
        ok(status == STATUS_SUCCESS, "%d: got 0x%x (expected STATUS_SUCCESS)\n", i, status);

        status = pNtQueryInformationThread(GetCurrentThread(), ThreadBasicInformation, &tbi, sizeof(tbi), NULL);
        ok(status == STATUS_SUCCESS, "%d: got 0x%x (expected STATUS_SUCCESS)\n", i, status);

        current_cpu = pNtGetCurrentProcessorNumber();
        ok((current_cpu == i), "%d (new_mask 0x%lx): running on processor %d (AffinityMask: 0x%lx)\n",
                                i, new_mask, current_cpu, tbi.AffinityMask);
    }

    /* restore old values */
    status = pNtSetInformationProcess(GetCurrentProcess(), ProcessAffinityMask, &old_process_mask, sizeof(old_process_mask));
    ok(status == STATUS_SUCCESS, "got 0x%x (expected STATUS_SUCCESS)\n", status);

    status = pNtSetInformationThread(GetCurrentThread(), ThreadAffinityMask, &old_thread_mask, sizeof(old_thread_mask));
    ok(status == STATUS_SUCCESS, "got 0x%x (expected STATUS_SUCCESS)\n", status);
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
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);
    ok(ret == sizeof(entry), "NtQueryInformationThread returned %u bytes\n", ret);
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
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);
    ok(ret == sizeof(entry), "NtQueryInformationThread returned %u bytes\n", ret);
    expected_entry = (void *)((char *)module + nt->OptionalHeader.AddressOfEntryPoint);
    ok(entry == expected_entry, "expected %p, got %p\n", expected_entry, entry);

    entry = (void *)0xdeadbeef;
    status = pNtSetInformationThread(GetCurrentThread(), ThreadQuerySetWin32StartAddress,
                                     &entry, sizeof(entry));
    ok(status == STATUS_SUCCESS || status == STATUS_INVALID_PARAMETER, /* >= Vista */
       "expected STATUS_SUCCESS or STATUS_INVALID_PARAMETER, got %08x\n", status);

    if (status == STATUS_SUCCESS)
    {
        entry = NULL;
        ret = 0xdeadbeef;
        status = pNtQueryInformationThread(GetCurrentThread(), ThreadQuerySetWin32StartAddress,
                                           &entry, sizeof(entry), &ret);
        ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);
        ok(ret == sizeof(entry), "NtQueryInformationThread returned %u bytes\n", ret);
        ok(entry == (void *)0xdeadbeef, "expected 0xdeadbeef, got %p\n", entry);
    }

    thread = CreateThread(NULL, 0, start_address_thread, NULL, 0, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "CreateThread failed with %d\n", GetLastError());
    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %u\n", ret);
    CloseHandle(thread);
}

static void test_query_data_alignment(void)
{
    ULONG ReturnLength;
    NTSTATUS status;
    DWORD value;

    value = 0xdeadbeef;
    status = pNtQuerySystemInformation(SystemRecommendedSharedDataAlignment, &value, sizeof(value), &ReturnLength);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08x\n", status);
    ok(sizeof(value) == ReturnLength, "Inconsistent length %u\n", ReturnLength);
    ok(value == 64, "Expected 64, got %u\n", value);
}

START_TEST(info)
{
    char **argv;
    int argc;

    argc = winetest_get_mainargs(&argv);
    if (argc >= 3) return; /* Child */

    if (!InitFunctionPtrs())
        return;

    /* NtQuerySystemInformation */

    /* 0x0 SystemBasicInformation */
    trace("Starting test_query_basic()\n");
    test_query_basic();

    /* 0x1 SystemCpuInformation */
    trace("Starting test_query_cpu()\n");
    test_query_cpu();

    /* 0x2 SystemPerformanceInformation */
    trace("Starting test_query_performance()\n");
    test_query_performance();

    /* 0x3 SystemTimeOfDayInformation */
    trace("Starting test_query_timeofday()\n");
    test_query_timeofday();

    /* 0x5 SystemProcessInformation */
    trace("Starting test_query_process()\n");
    test_query_process();

    /* 0x8 SystemProcessorPerformanceInformation */
    trace("Starting test_query_procperf()\n");
    test_query_procperf();

    /* 0xb SystemModuleInformation */
    trace("Starting test_query_module()\n");
    test_query_module();

    /* 0x10 SystemHandleInformation */
    trace("Starting test_query_handle()\n");
    test_query_handle();

    /* 0x40 SystemHandleInformation */
    trace("Starting test_query_handle_ex()\n");
    test_query_handle_ex();

    /* 0x15 SystemCacheInformation */
    trace("Starting test_query_cache()\n");
    test_query_cache();

    /* 0x17 SystemInterruptInformation */
    trace("Starting test_query_interrupt()\n");
    test_query_interrupt();

    /* 0x23 SystemKernelDebuggerInformation */
    trace("Starting test_query_kerndebug()\n");
    test_query_kerndebug();

    /* 0x25 SystemRegistryQuotaInformation */
    trace("Starting test_query_regquota()\n");
    test_query_regquota();

    /* 0x49 SystemLogicalProcessorInformation */
    trace("Starting test_query_logicalproc()\n");
    test_query_logicalproc();
    test_query_logicalprocex();

    /* NtPowerInformation */

    /* 0xb ProcessorInformation */
    trace("Starting test_query_processor_power_info()\n");
    test_query_processor_power_info();

    /* NtQueryInformationProcess */

    /* 0x0 ProcessBasicInformation */
    trace("Starting test_query_process_basic()\n");
    test_query_process_basic();

    /* 0x2 ProcessIoCounters */
    trace("Starting test_query_process_io()\n");
    test_query_process_io();

    /* 0x3 ProcessVmCounters */
    trace("Starting test_query_process_vm()\n");
    test_query_process_vm();

    /* 0x4 ProcessTimes */
    trace("Starting test_query_process_times()\n");
    test_query_process_times();

    /* 0x7 ProcessDebugPort */
    trace("Starting test_process_debug_port()\n");
    test_query_process_debug_port(argc, argv);

    /* 0x12 ProcessPriorityClass */
    trace("Starting test_query_process_priority()\n");
    test_query_process_priority();

    /* 0x14 ProcessHandleCount */
    trace("Starting test_query_process_handlecount()\n");
    test_query_process_handlecount();

    /* 0x1A ProcessWow64Information */
    trace("Starting test_query_process_wow64()\n");
    test_query_process_wow64();

    /* 0x1B ProcessImageFileName */
    trace("Starting test_query_process_image_file_name()\n");
    test_query_process_image_file_name();

    /* 0x1E ProcessDebugObjectHandle */
    trace("Starting test_query_process_debug_object_handle()\n");
    test_query_process_debug_object_handle(argc, argv);

    /* 0x1F ProcessDebugFlags */
    trace("Starting test_process_debug_flags()\n");
    test_query_process_debug_flags(argc, argv);

    /* belongs to its own file */
    trace("Starting test_readvirtualmemory()\n");
    test_readvirtualmemory();

    trace("Starting test_queryvirtualmemory()\n");
    test_queryvirtualmemory();

    trace("Starting test_mapprotection()\n");
    test_mapprotection();

    trace("Starting test_affinity()\n");
    test_affinity();

    trace("Starting test_NtGetCurrentProcessorNumber()\n");
    test_NtGetCurrentProcessorNumber();

    trace("Starting test_thread_start_address()\n");
    test_thread_start_address();

    trace("Starting test_query_data_alignment()\n");
    test_query_data_alignment();
}
