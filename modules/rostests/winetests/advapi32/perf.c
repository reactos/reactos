/*
 * Unit tests for Perflib functions
 *
 * Copyright (c) 2021 Paul Gofman for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "perflib.h"
#include "winperf.h"
#include "winternl.h"

#include "wine/test.h"

#include "initguid.h"

ULONG (WINAPI *pPerfCloseQueryHandle)(HANDLE);
ULONG (WINAPI *pPerfOpenQueryHandle)(const WCHAR*, HANDLE*);
ULONG (WINAPI *pPerfAddCounters)(HANDLE, PERF_COUNTER_IDENTIFIER*, DWORD);
ULONG (WINAPI *pPerfQueryCounterData)(HANDLE, PERF_DATA_HEADER*, DWORD, DWORD*);

static void init_functions(void)
{
    HANDLE hadvapi = GetModuleHandleA("advapi32.dll");

#define GET_FUNCTION(name) p##name = (void *)GetProcAddress(hadvapi, #name)
    GET_FUNCTION(PerfCloseQueryHandle);
    GET_FUNCTION(PerfOpenQueryHandle);
    GET_FUNCTION(PerfAddCounters);
    GET_FUNCTION(PerfQueryCounterData);
#undef GET_FUNCTION
}

static ULONG WINAPI test_provider_callback(ULONG code, void *buffer, ULONG size)
{
    ok(0, "Provider callback called.\n");
    return ERROR_SUCCESS;
}

void test_provider_init(void)
{
#ifdef __REACTOS__
    skip("test_provider_init() can't be built until ReactOS has implementations for Perf* functions.\n");
#else
    static GUID test_set_guid = {0xdeadbeef, 0x0002, 0x0003, {0x0f, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00 ,0x0a}};
    static GUID test_set_guid2 = {0xdeadbeef, 0x0003, 0x0003, {0x0f, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00 ,0x0a}};
    static GUID test_guid = {0xdeadbeef, 0x0001, 0x0002, {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00 ,0x0a}};
    static struct
    {
        PERF_COUNTERSET_INFO counterset;
        PERF_COUNTER_INFO counter[2];
    }
    pc_template =
    {
        {{0}},
        {
            {1, PERF_COUNTER_COUNTER, PERF_ATTRIB_BY_REFERENCE, sizeof(PERF_COUNTER_INFO),
                    PERF_DETAIL_NOVICE, 0, 0xdeadbeef},
            {2, PERF_COUNTER_COUNTER, PERF_ATTRIB_BY_REFERENCE, sizeof(PERF_COUNTER_INFO),
                    PERF_DETAIL_NOVICE, 0, 0xdeadbeef},
        },
    };

    PERF_COUNTERSET_INSTANCE *instance;
    PERF_PROVIDER_CONTEXT prov_context;
    UINT64 counter1, counter2;
    HANDLE prov, prov2;
    ULONG ret, size;
    BOOL bret;

    prov = (HANDLE)0xdeadbeef;
    ret = PerfStartProvider(NULL, test_provider_callback, &prov);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);
    ok(prov == (HANDLE)0xdeadbeef, "Got unexpected prov %p.\n", prov);

    prov = (HANDLE)0xdeadbeef;
    ret = PerfStartProvider(&test_guid, test_provider_callback, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);
    ok(prov == (HANDLE)0xdeadbeef, "Got unexpected prov %p.\n", prov);

    prov = (HANDLE)0xdeadbeef;
    ret = PerfStartProvider(&test_guid, test_provider_callback, &prov);
    ok(!ret, "Got unexpected ret %lu.\n", ret);
    ok(prov != (HANDLE)0xdeadbeef, "Provider handle is not set.\n");

    prov2 = prov;
    ret = PerfStartProvider(&test_guid, test_provider_callback, &prov2);
    ok(!ret, "Got unexpected ret %lu.\n", ret);
    ok(prov2 != prov, "Got the same provider handle.\n");

    ret = PerfStopProvider(prov2);
    ok(!ret, "Got unexpected ret %lu.\n", ret);

    if (0)
    {
        /* Access violation on Windows. */
        PerfStopProvider(prov2);
    }

    /* Provider handle is a pointer and not a kernel object handle. */
    bret = DuplicateHandle(GetCurrentProcess(), prov, GetCurrentProcess(), &prov2, 0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(!bret && GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected bret %d, err %lu.\n", bret, GetLastError());
    bret = IsBadWritePtr(prov, 8);
    ok(!bret, "Handle does not point to the data.\n");

    pc_template.counterset.CounterSetGuid = test_set_guid;
    pc_template.counterset.ProviderGuid = test_guid;
    pc_template.counterset.NumCounters = 0;
    pc_template.counterset.InstanceType = PERF_COUNTERSET_SINGLE_INSTANCE;
    ret = PerfSetCounterSetInfo(prov, &pc_template.counterset, sizeof(pc_template.counterset));
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);

    pc_template.counterset.CounterSetGuid = test_set_guid;
    pc_template.counterset.ProviderGuid = test_guid;
    pc_template.counterset.NumCounters = 2;
    pc_template.counterset.InstanceType = PERF_COUNTERSET_SINGLE_INSTANCE;
    ret = PerfSetCounterSetInfo(prov, &pc_template.counterset, sizeof(pc_template));
    ok(!ret, "Got unexpected ret %lu.\n", ret);

    pc_template.counterset.CounterSetGuid = test_set_guid2;
    /* Looks like ProviderGuid doesn't need to match provider. */
    pc_template.counterset.ProviderGuid = test_set_guid;
    pc_template.counterset.NumCounters = 1;
    pc_template.counterset.InstanceType = PERF_COUNTERSET_SINGLE_INSTANCE;
    ret = PerfSetCounterSetInfo(prov, &pc_template.counterset, sizeof(pc_template));
    ok(!ret, "Got unexpected ret %lu.\n", ret);

    ret = PerfSetCounterSetInfo(prov, &pc_template.counterset, sizeof(pc_template));
    ok(ret == ERROR_ALREADY_EXISTS, "Got unexpected ret %lu.\n", ret);

    SetLastError(0xdeadbeef);
    instance = PerfCreateInstance(prov, NULL, L"1", 1);
    ok(!instance, "Got unexpected instance %p.\n", instance);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %lu.\n", GetLastError());

    SetLastError(0xdeadbeef);
    instance = PerfCreateInstance(prov, &test_guid, L"1", 1);
    ok(!instance, "Got unexpected instance %p.\n", instance);
    ok(GetLastError() == ERROR_NOT_FOUND, "Got unexpected error %lu.\n", GetLastError());

    SetLastError(0xdeadbeef);
    instance = PerfCreateInstance(prov, &test_guid, NULL, 1);
    ok(!instance, "Got unexpected instance %p.\n", instance);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %lu.\n", GetLastError());

    SetLastError(0xdeadbeef);
    instance = PerfCreateInstance(prov, &test_set_guid, L"11", 1);
    ok(!!instance, "Got NULL instance.\n");
    ok(GetLastError() == 0xdeadbeef, "Got unexpected error %lu.\n", GetLastError());
    ok(instance->InstanceId == 1, "Got unexpected InstanceId %lu.\n", instance->InstanceId);
    ok(instance->InstanceNameSize == 6, "Got unexpected InstanceNameSize %lu.\n", instance->InstanceNameSize);
    ok(IsEqualGUID(&instance->CounterSetGuid, &test_set_guid), "Got unexpected guid %s.\n",
            debugstr_guid(&instance->CounterSetGuid));

    ok(instance->InstanceNameOffset == sizeof(*instance) + sizeof(UINT64) * 2,
            "Got unexpected InstanceNameOffset %lu.\n", instance->InstanceNameOffset);
    ok(!lstrcmpW((WCHAR *)((BYTE *)instance + instance->InstanceNameOffset), L"11"),
            "Got unexpected instance name %s.\n",
            debugstr_w((WCHAR *)((BYTE *)instance + instance->InstanceNameOffset)));
    size = ((sizeof(*instance) + sizeof(UINT64) * 2 + instance->InstanceNameSize) + 7) & ~7;
    ok(size == instance->dwSize, "Got unexpected size %lu, instance->dwSize %lu.\n", size, instance->dwSize);

    ret = PerfSetCounterRefValue(prov, instance, 1, &counter1);
    ok(!ret, "Got unexpected ret %lu.\n", ret);
    ret = PerfSetCounterRefValue(prov, instance, 2, &counter2);
    ok(!ret, "Got unexpected ret %lu.\n", ret);

    ret = PerfSetCounterRefValue(prov, instance, 0, &counter2);
    ok(ret == ERROR_NOT_FOUND, "Got unexpected ret %lu.\n", ret);

    ok(*(void **)(instance + 1) == &counter1, "Got unexpected counter value %p.\n",
            *(void **)(instance + 1));
    ok(*(void **)((BYTE *)instance + sizeof(*instance) + sizeof(UINT64)) == &counter2,
            "Got unexpected counter value %p.\n", *(void **)(instance + 1));

    ret = PerfDeleteInstance(prov, instance);
    ok(!ret, "Got unexpected ret %lu.\n", ret);

    ret = PerfStopProvider(prov);
    ok(!ret, "Got unexpected ret %lu.\n", ret);

    memset( &prov_context, 0, sizeof(prov_context) );
    prov = (HANDLE)0xdeadbeef;
    ret = PerfStartProviderEx( &test_guid, &prov_context, &prov );
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);
    ok(prov == (HANDLE)0xdeadbeef, "Got unexpected prov %p.\n", prov);

    prov_context.ContextSize = sizeof(prov_context) + 1;
    ret = PerfStartProviderEx( &test_guid, &prov_context, &prov );
    ok(!ret, "Got unexpected ret %lu.\n", ret);
    ok(prov != (HANDLE)0xdeadbeef, "Provider handle is not set.\n");

    ret = PerfStopProvider(prov);
    ok(!ret, "Got unexpected ret %lu.\n", ret);
#endif
}

DEFINE_GUID(TestCounterGUID, 0x12345678, 0x1234, 0x5678, 0x12, 0x34, 0x11, 0x11, 0x22, 0x22, 0x33, 0x33);

static ULONG64 trunc_nttime_ms(ULONG64 t)
{
    return (t / 10000) * 10000;
}

static void test_perf_counters(void)
{
    LARGE_INTEGER freq, qpc1, qpc2, nttime1, nttime2, systime;
    char buffer[sizeof(PERF_COUNTER_IDENTIFIER) + 8];
    PERF_COUNTER_IDENTIFIER *counter_id;
    PERF_DATA_HEADER dh;
    HANDLE query;
    DWORD size;
    ULONG ret;

    if (!pPerfOpenQueryHandle)
    {
        win_skip("PerfOpenQueryHandle not found.\n");
        return;
    }

    ret = pPerfOpenQueryHandle(NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got ret %lu.\n", ret);
    ret = pPerfOpenQueryHandle(NULL, &query);
    ok(!ret, "got ret %lu.\n", ret);

    counter_id = (PERF_COUNTER_IDENTIFIER *)buffer;
    memset(buffer, 0, sizeof(buffer));

    counter_id->CounterSetGuid = TestCounterGUID;
    counter_id->CounterId = PERF_WILDCARD_COUNTER;
    counter_id->InstanceId = PERF_WILDCARD_COUNTER;

    ret = pPerfAddCounters(query, counter_id, sizeof(*counter_id));
    ok(ret == ERROR_INVALID_PARAMETER, "got ret %lu.\n", ret);

    counter_id->Size = sizeof(*counter_id);
    ret = pPerfAddCounters(query, counter_id, 8);
    ok(ret == ERROR_INVALID_PARAMETER, "got ret %lu.\n", ret);
    ret = pPerfAddCounters(query, counter_id, sizeof(*counter_id));
    ok(!ret, "got ret %lu.\n", ret);
    ok(counter_id->Status == ERROR_WMI_GUID_NOT_FOUND, "got Status %#lx.\n", counter_id->Status);

    ret = pPerfQueryCounterData(query, NULL, 0, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got ret %lu.\n", ret);

    size = 0xdeadbeef;
    ret = pPerfQueryCounterData(query, NULL, 0, &size);
    ok(ret == ERROR_NOT_ENOUGH_MEMORY, "got ret %lu.\n", ret);
    ok(size == sizeof(dh), "got size %lu.\n", size);

    ret = pPerfQueryCounterData(query, &dh, sizeof(dh), NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got ret %lu.\n", ret);

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&qpc1);
    NtQuerySystemTime(&nttime1);

    size = 0xdeadbeef;
    ret = pPerfQueryCounterData(query, &dh, sizeof(dh), &size);
    QueryPerformanceCounter(&qpc2);
    NtQuerySystemTime(&nttime2);
    SystemTimeToFileTime(&dh.SystemTime, (FILETIME *)&systime);
    ok(!ret, "got ret %lu.\n", ret);
    ok(size == sizeof(dh), "got size %lu.\n", size);
    ok(dh.dwTotalSize == sizeof(dh), "got dwTotalSize %lu.\n", dh.dwTotalSize);
    ok(!dh.dwNumCounters, "got dwNumCounters %lu.\n", dh.dwNumCounters);
    ok(dh.PerfFreq == freq.QuadPart, "got PerfFreq %I64u.\n", dh.PerfFreq);
    ok(dh.PerfTimeStamp >= qpc1.QuadPart && dh.PerfTimeStamp <= qpc2.QuadPart,
            "got PerfTimeStamp %I64u, qpc1 %I64u, qpc2 %I64u.\n",
            dh.PerfTimeStamp, qpc1.QuadPart, qpc2.QuadPart);
    ok(dh.PerfTime100NSec >= nttime1.QuadPart && dh.PerfTime100NSec <= nttime2.QuadPart,
            "got PerfTime100NSec %I64u, nttime1 %I64u, nttime2 %I64u.\n",
            dh.PerfTime100NSec, nttime1.QuadPart, nttime2.QuadPart);
    ok(systime.QuadPart >= trunc_nttime_ms(nttime1.QuadPart) && systime.QuadPart <= trunc_nttime_ms(nttime2.QuadPart),
            "got systime %I64u, nttime1 %I64u, nttime2 %I64u, %d.\n",
            systime.QuadPart, nttime1.QuadPart, nttime2.QuadPart, dh.SystemTime.wMilliseconds);

    ret = pPerfCloseQueryHandle(query);
    ok(!ret, "got ret %lu.\n", ret);
}

START_TEST(perf)
{
    init_functions();

    test_provider_init();
    test_perf_counters();
}
