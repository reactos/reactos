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

static NTSTATUS (WINAPI * pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI * pNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

static HMODULE hntdll = 0;

/* one_before_last_pid is used to be able to compare values of a still running process
   with the output of the test_query_process_times and test_query_process_handlecount tests.
*/
static DWORD one_before_last_pid = 0;

#define NTDLL_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hntdll, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
      FreeLibrary(hntdll); \
      return FALSE; \
    }

static BOOL InitFunctionPtrs(void)
{
    hntdll = LoadLibraryA("ntdll.dll");
    if(!hntdll) {
      trace("Could not load ntdll.dll\n");
      return FALSE;
    }
    if (hntdll)
    {
      NTDLL_GET_PROC(NtQuerySystemInformation)
      NTDLL_GET_PROC(NtQueryInformationProcess)
    }
    return TRUE;
}

static void test_query_basic(void)
{
    DWORD status;
    ULONG ReturnLength;
    SYSTEM_BASIC_INFORMATION sbi;

    /* This test also covers some basic parameter testing that should be the same for 
     * every information class
    */

    /* Use a nonexistent info class */
    trace("Check nonexistent info class\n");
    status = pNtQuerySystemInformation(-1, NULL, 0, NULL);
    ok( status == STATUS_INVALID_INFO_CLASS, "Expected STATUS_INVALID_INFO_CLASS, got %08lx\n", status);

    /* Use an existing class but with a zero-length buffer */
    trace("Check zero-length buffer\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, NULL, 0, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Use an existing class, correct length but no SystemInformation buffer */
    trace("Check no SystemInformation buffer\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, NULL, sizeof(sbi), NULL);
    ok( status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", status);

    /* Use a existing class, correct length, a pointer to a buffer but no ReturnLength pointer */
    trace("Check no ReturnLength pointer\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Check a too large buffer size */
    trace("Check a too large buffer size\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Finally some correct calls */
    trace("Check with correct parameters\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(sbi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(sbi), ReturnLength);

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
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(sci) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(sci), ReturnLength);

    /* Check if we have some return values */
    trace("Processor FeatureSet : %08lx\n", sci.FeatureSet);
    ok( sci.FeatureSet != 0, "Expected some features for this processor, got %08lx\n", sci.FeatureSet);
}

static void test_query_performance(void)
{
    DWORD status;
    ULONG ReturnLength;
    SYSTEM_PERFORMANCE_INFORMATION spi;

    status = pNtQuerySystemInformation(SystemPerformanceInformation, &spi, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = pNtQuerySystemInformation(SystemPerformanceInformation, &spi, sizeof(spi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(spi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(spi), ReturnLength);

    status = pNtQuerySystemInformation(SystemPerformanceInformation, &spi, sizeof(spi) + 2, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(spi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(spi), ReturnLength);

    /* Not return values yet, as struct members are unknown */
}

static void test_query_timeofday(void)
{
    DWORD status;
    ULONG ReturnLength;

    /* Copy of our winternl.h structure turned into a private one */
    typedef struct _SYSTEM_TIMEOFDAY_INFORMATION_PRIVATE {
        LARGE_INTEGER liKeBootTime;
        LARGE_INTEGER liKeSystemTime;
        LARGE_INTEGER liExpTimeZoneBias;
        ULONG uCurrentTimeZoneId;
        DWORD dwUnknown1[5];
    } SYSTEM_TIMEOFDAY_INFORMATION_PRIVATE, *PSYSTEM_TIMEOFDAY_INFORMATION_PRIVATE;

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
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
        ok( 0 == ReturnLength, "ReturnLength should be 0, it is (%ld)\n", ReturnLength);

        sti.uCurrentTimeZoneId = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 28, &ReturnLength);
        ok( 0xdeadbeef == sti.uCurrentTimeZoneId, "This part of the buffer should not have been filled\n");

        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 32, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( 32 == ReturnLength, "ReturnLength should be 0, it is (%ld)\n", ReturnLength);
    }
    else
    {
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 0, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( 0 == ReturnLength, "ReturnLength should be 0, it is (%ld)\n", ReturnLength);

        sti.uCurrentTimeZoneId = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 24, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( 24 == ReturnLength, "ReturnLength should be 24, it is (%ld)\n", ReturnLength);
        ok( 0xdeadbeef == sti.uCurrentTimeZoneId, "This part of the buffer should not have been filled\n");
    
        sti.uCurrentTimeZoneId = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 32, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( 32 == ReturnLength, "ReturnLength should be 32, it is (%ld)\n", ReturnLength);
        ok( 0xdeadbeef != sti.uCurrentTimeZoneId, "Buffer should have been partially filled\n");
    
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 49, &ReturnLength);
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
        ok( 0 == ReturnLength, "ReturnLength should be 0, it is (%ld)\n", ReturnLength);
    
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, sizeof(sti), &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( sizeof(sti) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(sti), ReturnLength);
    }

    /* Check if we have some return values */
    trace("uCurrentTimeZoneId : (%ld)\n", sti.uCurrentTimeZoneId);
}

static void test_query_process(void)
{
    DWORD status;
    DWORD last_pid;
    ULONG ReturnLength;
    int i = 0, k = 0;
    int is_nt = 0;
    SYSTEM_BASIC_INFORMATION sbi;

    /* Copy of our winternl.h structure turned into a private one */
    typedef struct _SYSTEM_PROCESS_INFORMATION_PRIVATE {
        DWORD dwOffset;
        DWORD dwThreadCount;
        DWORD dwUnknown1[6];
        FILETIME ftCreationTime;
        FILETIME ftUserTime;
        FILETIME ftKernelTime;
        UNICODE_STRING ProcessName;
        DWORD dwBasePriority;
        DWORD dwProcessID;
        DWORD dwParentProcessID;
        DWORD dwHandleCount;
        DWORD dwUnknown3;
        DWORD dwUnknown4;
        VM_COUNTERS vmCounters;
        IO_COUNTERS ioCounters;
        SYSTEM_THREAD_INFORMATION ti[1];
    } SYSTEM_PROCESS_INFORMATION_PRIVATE, *PSYSTEM_PROCESS_INFORMATION_PRIVATE;

    ULONG SystemInformationLength = sizeof(SYSTEM_PROCESS_INFORMATION_PRIVATE);
    SYSTEM_PROCESS_INFORMATION_PRIVATE* spi = HeapAlloc(GetProcessHeap(), 0, SystemInformationLength);

    /* Only W2K3 returns the needed length, the rest returns 0, so we have to loop */

    for (;;)
    {
        status = pNtQuerySystemInformation(SystemProcessInformation, spi, SystemInformationLength, &ReturnLength);

        if (status != STATUS_INFO_LENGTH_MISMATCH) break;
        
        spi = HeapReAlloc(GetProcessHeap(), 0, spi , SystemInformationLength *= 2);
    }

    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Get the first dwOffset, from this we can deduce the OS version we're running
     *
     * W2K/WinXP/W2K3:
     *   dwOffset for a process is 184 + (no. of threads) * sizeof(SYSTEM_THREAD_INFORMATION)
     * NT:
     *   dwOffset for a process is 136 + (no. of threads) * sizeof(SYSTEM_THREAD_INFORMATION)
     * Wine (with every windows version):
     *   dwOffset for a process is 0 if just this test is running
     *   dwOffset for a process is 184 + (no. of threads) * sizeof(SYSTEM_THREAD_INFORMATION) +
     *                             ProcessName.MaximumLength
     *     if more wine processes are running
     *
     * Note : On windows the first process is in fact the Idle 'process' with a thread for every processor
    */

    pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength);

    is_nt = ( spi->dwOffset - (sbi.NumberOfProcessors * sizeof(SYSTEM_THREAD_INFORMATION)) == 136);

    if (is_nt) trace("Windows version is NT, we will skip thread tests\n");

    /* Check if we have some return values
     * 
     * On windows there will be several processes running (Including the always present Idle and System)
     * On wine we only have one (if this test is the only wine process running)
    */
    
    /* Loop through the processes */

    for (;;)
    {
        i++;

        last_pid = spi->dwProcessID;

        ok( spi->dwThreadCount > 0, "Expected some threads for this process, got 0\n");

        /* Loop through the threads, skip NT4 for now */
        
        if (!is_nt)
        {
            DWORD j;
            for ( j = 0; j < spi->dwThreadCount; j++) 
            {
                k++;
                ok ( spi->ti[j].dwOwningPID == spi->dwProcessID, 
                     "The owning pid of the thread (%ld) doesn't equal the pid (%ld) of the process\n",
                     spi->ti[j].dwOwningPID, spi->dwProcessID);
            }
        }

        if (!spi->dwOffset) break;

        one_before_last_pid = last_pid;

        spi = (SYSTEM_PROCESS_INFORMATION_PRIVATE*)((char*)spi + spi->dwOffset);
    }
    trace("Total number of running processes : %d\n", i);
    if (!is_nt) trace("Total number of running threads   : %d\n", k);

    if (one_before_last_pid == 0) one_before_last_pid = last_pid;

    HeapFree( GetProcessHeap(), 0, spi);
}

static void test_query_procperf(void)
{
    DWORD status;
    ULONG ReturnLength;
    ULONG NeededLength;
    SYSTEM_BASIC_INFORMATION sbi;
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* sppi;

    /* Find out the number of processors */
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength);
    NeededLength = sbi.NumberOfProcessors * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

    sppi = HeapAlloc(GetProcessHeap(), 0, NeededLength);

    status = pNtQuerySystemInformation(SystemProcessorPerformanceInformation, sppi, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Try it for 1 processor */
    status = pNtQuerySystemInformation(SystemProcessorPerformanceInformation, sppi,
                                       sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) == ReturnLength,
        "Inconsistent length (%d) <-> (%ld)\n", sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION), ReturnLength);
 
    /* Try it for all processors */
    status = pNtQuerySystemInformation(SystemProcessorPerformanceInformation, sppi, NeededLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( NeededLength == ReturnLength, "Inconsistent length (%ld) <-> (%ld)\n", NeededLength, ReturnLength);

    /* A too large given buffer size */
    sppi = HeapReAlloc(GetProcessHeap(), 0, sppi , NeededLength + 2);
    status = pNtQuerySystemInformation(SystemProcessorPerformanceInformation, sppi, NeededLength + 2, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( NeededLength == ReturnLength, "Inconsistent length (%ld) <-> (%ld)\n", NeededLength, ReturnLength);

    HeapFree( GetProcessHeap(), 0, sppi);
}

static void test_query_module(void)
{
    DWORD status;
    ULONG ReturnLength;
    ULONG ModuleCount, i;

    ULONG SystemInformationLength = sizeof(SYSTEM_MODULE_INFORMATION);
    SYSTEM_MODULE_INFORMATION* smi = HeapAlloc(GetProcessHeap(), 0, SystemInformationLength); 
    SYSTEM_MODULE* sm;

    /* Request the needed length */
    status = pNtQuerySystemInformation(SystemModuleInformation, smi, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( ReturnLength > 0, "Expected a ReturnLength to show the needed length\n");

    SystemInformationLength = ReturnLength;
    smi = HeapReAlloc(GetProcessHeap(), 0, smi , SystemInformationLength);
    status = pNtQuerySystemInformation(SystemModuleInformation, smi, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    ModuleCount = smi->ModulesCount;
    sm = &smi->Modules[0];
    todo_wine{
        /* our implementation is a stub for now */
        ok( ModuleCount > 0, "Expected some modules to be loaded\n");
    }

    /* Loop through all the modules/drivers, Wine doesn't get here (yet) */
    for (i = 0; i < ModuleCount ; i++)
    {
        ok( i == sm->Id, "Id (%d) should have matched %lu\n", sm->Id, i);
        sm++;
    }

    HeapFree( GetProcessHeap(), 0, smi);
}

static void test_query_handle(void)
{
    DWORD status;
    ULONG ReturnLength;
    ULONG SystemInformationLength = sizeof(SYSTEM_HANDLE_INFORMATION);
    SYSTEM_HANDLE_INFORMATION* shi = HeapAlloc(GetProcessHeap(), 0, SystemInformationLength);

    /* Request the needed length : a SystemInformationLength greater than one struct sets ReturnLength */
    status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);

    /* The following check assumes more than one handle on any given system */
    todo_wine
    {
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    }
    ok( ReturnLength > 0, "Expected ReturnLength to be > 0, it was %ld\n", ReturnLength);

    SystemInformationLength = ReturnLength;
    shi = HeapReAlloc(GetProcessHeap(), 0, shi , SystemInformationLength);
    status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Check if we have some return values */
    trace("Number of Handles : %ld\n", shi->Count);
    todo_wine
    {
        /* our implementation is a stub for now */
        ok( shi->Count > 1, "Expected more than 1 handles, got (%ld)\n", shi->Count);
    }

    HeapFree( GetProcessHeap(), 0, shi);
}

static void test_query_cache(void)
{
    DWORD status;
    ULONG ReturnLength;
    SYSTEM_CACHE_INFORMATION sci;

    status = pNtQuerySystemInformation(SystemCacheInformation, &sci, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = pNtQuerySystemInformation(SystemCacheInformation, &sci, sizeof(sci), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(sci) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(sci), ReturnLength);

    status = pNtQuerySystemInformation(SystemCacheInformation, &sci, sizeof(sci) + 2, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(sci) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(sci), ReturnLength);
}

static void test_query_interrupt(void)
{
    DWORD status;
    ULONG ReturnLength;
    ULONG NeededLength;
    SYSTEM_BASIC_INFORMATION sbi;
    SYSTEM_INTERRUPT_INFORMATION* sii;

    /* Find out the number of processors */
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength);
    NeededLength = sbi.NumberOfProcessors * sizeof(SYSTEM_INTERRUPT_INFORMATION);

    sii = HeapAlloc(GetProcessHeap(), 0, NeededLength);

    status = pNtQuerySystemInformation(SystemInterruptInformation, sii, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Try it for all processors */
    status = pNtQuerySystemInformation(SystemInterruptInformation, sii, NeededLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Windows XP and W2K3 (and others?) always return 0 for the ReturnLength
     * No test added for this as it's highly unlikely that an app depends on this
    */

    HeapFree( GetProcessHeap(), 0, sii);
}

static void test_query_kerndebug(void)
{
    DWORD status;
    ULONG ReturnLength;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION skdi;

    status = pNtQuerySystemInformation(SystemKernelDebuggerInformation, &skdi, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = pNtQuerySystemInformation(SystemKernelDebuggerInformation, &skdi, sizeof(skdi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(skdi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(skdi), ReturnLength);

    status = pNtQuerySystemInformation(SystemKernelDebuggerInformation, &skdi, sizeof(skdi) + 2, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(skdi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(skdi), ReturnLength);
}

static void test_query_regquota(void)
{
    DWORD status;
    ULONG ReturnLength;
    SYSTEM_REGISTRY_QUOTA_INFORMATION srqi;

    status = pNtQuerySystemInformation(SystemRegistryQuotaInformation, &srqi, 0, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = pNtQuerySystemInformation(SystemRegistryQuotaInformation, &srqi, sizeof(srqi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(srqi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(srqi), ReturnLength);

    status = pNtQuerySystemInformation(SystemRegistryQuotaInformation, &srqi, sizeof(srqi) + 2, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(srqi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(srqi), ReturnLength);
}

static void test_query_process_basic(void)
{
    DWORD status;
    ULONG ReturnLength;

    typedef struct _PROCESS_BASIC_INFORMATION_PRIVATE {
        DWORD ExitStatus;
        DWORD PebBaseAddress;
        DWORD AffinityMask;
        DWORD BasePriority;
        ULONG UniqueProcessId;
        ULONG InheritedFromUniqueProcessId;
    } PROCESS_BASIC_INFORMATION_PRIVATE, *PPROCESS_BASIC_INFORMATION_PRIVATE;

    PROCESS_BASIC_INFORMATION_PRIVATE pbi;

    /* This test also covers some basic parameter testing that should be the same for
     * every information class
    */

    /* Use a nonexistent info class */
    trace("Check nonexistent info class\n");
    status = pNtQueryInformationProcess(NULL, -1, NULL, 0, NULL);
    ok( status == STATUS_INVALID_INFO_CLASS, "Expected STATUS_INVALID_INFO_CLASS, got %08lx\n", status);

    /* Do not give a handle and buffer */
    trace("Check NULL handle and buffer and zero-length buffersize\n");
    status = pNtQueryInformationProcess(NULL, ProcessBasicInformation, NULL, 0, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Use a correct info class and buffer size, but still no handle and buffer */
    trace("Check NULL handle and buffer\n");
    status = pNtQueryInformationProcess(NULL, ProcessBasicInformation, NULL, sizeof(pbi), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08lx\n", status);

    /* Use a correct info class and buffer size, but still no handle */
    trace("Check NULL handle\n");
    status = pNtQueryInformationProcess(NULL, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    /* Use a greater buffer size */
    trace("Check NULL handle and too large buffersize\n");
    status = pNtQueryInformationProcess(NULL, ProcessBasicInformation, &pbi, sizeof(pbi) * 2, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Use no ReturnLength */
    trace("Check NULL ReturnLength\n");
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Finally some correct calls */
    trace("Check with correct parameters\n");
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(pbi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(pbi), ReturnLength);

    /* Everything is correct except a too large buffersize */
    trace("Too large buffersize\n");
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( sizeof(pbi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(pbi), ReturnLength);
                                                                                                                                               
    /* Check if we have some return values */
    trace("ProcessID : %ld\n", pbi.UniqueProcessId);
    ok( pbi.UniqueProcessId > 0, "Expected a ProcessID > 0, got 0\n");
}

static void test_query_process_vm(void)
{
    DWORD status;
    ULONG ReturnLength;
    VM_COUNTERS pvi;

    status = pNtQueryInformationProcess(NULL, ProcessVmCounters, NULL, sizeof(pvi), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08lx\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessVmCounters, &pvi, sizeof(pvi), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    /* Windows XP and W2K3 will report success for a size of 44 AND 48 !
       Windows W2K will only report success for 44.
       For now we only care for 44, which is sizeof(VM_COUNTERS)
       If an app depends on it, we have to implement this in ntdll/process.c
    */

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessVmCounters, &pvi, 24, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessVmCounters, &pvi, sizeof(pvi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(pvi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(pvi), ReturnLength);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessVmCounters, &pvi, 46, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( sizeof(pvi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(pvi), ReturnLength);

    /* Check if we have some return values */
    trace("WorkingSetSize : %ld\n", pvi.WorkingSetSize);
    todo_wine
    {
        ok( pvi.WorkingSetSize > 0, "Expected a WorkingSetSize > 0\n");
    }
}

static void test_query_process_io(void)
{
    DWORD status;
    ULONG ReturnLength;
    IO_COUNTERS pii;

    /* NT4 doesn't support this information class, so check for it */
    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessIoCounters, &pii, sizeof(pii), &ReturnLength);
    if (status == STATUS_NOT_SUPPORTED)
    {
        trace("ProcessIoCounters information class not supported, skipping tests\n");
        return;
    }
 
    status = pNtQueryInformationProcess(NULL, ProcessIoCounters, NULL, sizeof(pii), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08lx\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessIoCounters, &pii, sizeof(pii), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessIoCounters, &pii, 24, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessIoCounters, &pii, sizeof(pii), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(pii) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(pii), ReturnLength);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessIoCounters, &pii, sizeof(pii) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( sizeof(pii) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(pii), ReturnLength);

    /* Check if we have some return values */
    trace("OtherOperationCount : %lld\n", pii.OtherOperationCount);
    todo_wine
    {
        ok( pii.OtherOperationCount > 0, "Expected an OtherOperationCount > 0\n");
    }
}

static void test_query_process_times(void)
{
    DWORD status;
    ULONG ReturnLength;
    HANDLE process;
    SYSTEMTIME UTC, Local;
    KERNEL_USER_TIMES spti;

    status = pNtQueryInformationProcess(NULL, ProcessTimes, NULL, sizeof(spti), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08lx\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessTimes, &spti, sizeof(spti), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessTimes, &spti, 24, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, one_before_last_pid);
    if (!process)
    {
        trace("Could not open process with ID : %ld, error : %08lx. Going to use current one.\n", one_before_last_pid, GetLastError());
        process = GetCurrentProcess();
        trace("ProcessTimes for current process\n");
    }
    else
        trace("ProcessTimes for process with ID : %ld\n", one_before_last_pid);

    status = pNtQueryInformationProcess( process, ProcessTimes, &spti, sizeof(spti), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(spti) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(spti), ReturnLength);
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
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( sizeof(spti) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(spti), ReturnLength);
}

static void test_query_process_handlecount(void)
{
    DWORD status;
    ULONG ReturnLength;
    DWORD handlecount;
    HANDLE process;

    status = pNtQueryInformationProcess(NULL, ProcessHandleCount, NULL, sizeof(handlecount), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08lx\n", status);

    status = pNtQueryInformationProcess(NULL, ProcessHandleCount, &handlecount, sizeof(handlecount), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessHandleCount, &handlecount, 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, one_before_last_pid);
    if (!process)
    {
        trace("Could not open process with ID : %ld, error : %08lx. Going to use current one.\n", one_before_last_pid, GetLastError());
        process = GetCurrentProcess();
        trace("ProcessHandleCount for current process\n");
    }
    else
        trace("ProcessHandleCount for process with ID : %ld\n", one_before_last_pid);

    status = pNtQueryInformationProcess( process, ProcessHandleCount, &handlecount, sizeof(handlecount), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(handlecount) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(handlecount), ReturnLength);
    CloseHandle(process);

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessHandleCount, &handlecount, sizeof(handlecount) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    ok( sizeof(handlecount) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(handlecount), ReturnLength);

    /* Check if we have some return values */
    trace("HandleCount : %ld\n", handlecount);
    todo_wine
    {
        ok( handlecount > 0, "Expected some handles, got 0\n");
    }
}

START_TEST(info)
{
    if(!InitFunctionPtrs())
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

    /* 0x14 ProcessHandleCount */
    trace("Starting test_query_process_handlecount()\n");
    test_query_process_handlecount();

    FreeLibrary(hntdll);
}
