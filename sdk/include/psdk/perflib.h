/*
 * Copyright (C) 2017 Austin English
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

#ifndef _PERFLIB_H_
#define _PERFLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void * (__WINE_ALLOC_SIZE(1) CDECL * PERF_MEM_ALLOC)(SIZE_T,void*);
typedef void (CDECL * PERF_MEM_FREE)(LPVOID,LPVOID);
typedef ULONG (WINAPI * PERFLIBREQUEST)(ULONG,PVOID,ULONG);

typedef struct _PERF_COUNTERSET_INFO {
    GUID CounterSetGuid;
    GUID ProviderGuid;
    ULONG NumCounters;
    ULONG InstanceType;
} PERF_COUNTERSET_INFO, * PPERF_COUNTERSET_INFO;

/* PERF_COUNTERSET_INFO InstanceType values. */
#define PERF_COUNTERSET_FLAG_MULTIPLE  0x00000002
#define PERF_COUNTERSET_FLAG_AGGREGATE 0x00000004
#define PERF_COUNTERSET_FLAG_HISTORY   0x00000008
#define PERF_COUNTERSET_FLAG_INSTANCE  0x00000010

#define PERF_COUNTERSET_SINGLE_INSTANCE          0
#define PERF_COUNTERSET_MULTI_INSTANCES          PERF_COUNTERSET_FLAG_MULTIPLE
#define PERF_COUNTERSET_SINGLE_AGGREGATE         PERF_COUNTERSET_FLAG_AGGREGATE
#define PERF_COUNTERSET_MULTI_AGGREGATE          (PERF_COUNTERSET_FLAG_AGGREGATE | PERF_COUNTERSET_FLAG_MULTIPLE)
#define PERF_COUNTERSET_SINGLE_AGGREGATE_HISTORY (PERF_COUNTERSET_FLAG_HISTORY | PERF_COUNTERSET_SINGLE_AGGREGATE)
#define PERF_COUNTERSET_INSTANCE_AGGREGATE       (PERF_COUNTERSET_MULTI_AGGREGATE | PERF_COUNTERSET_FLAG_INSTANCE)

typedef struct _PERF_COUNTERSET_INSTANCE {
    GUID CounterSetGuid;
    ULONG dwSize;
    ULONG InstanceId;
    ULONG InstanceNameOffset;
    ULONG InstanceNameSize;
} PERF_COUNTERSET_INSTANCE, * PPERF_COUNTERSET_INSTANCE;

typedef struct _PERF_COUNTER_INFO {
    ULONG CounterId;
    ULONG Type;
    ULONGLONG Attrib;
    ULONG Size;
    ULONG DetailLevel;
    LONG Scale;
    ULONG Offset;
} PERF_COUNTER_INFO, *PPERF_COUNTER_INFO;

/* PERF_COUNTER_INFO Attrib flags. */
#define PERF_ATTRIB_BY_REFERENCE       0x00000001
#define PERF_ATTRIB_NO_DISPLAYABLE     0x00000002
#define PERF_ATTRIB_NO_GROUP_SEPARATOR 0x00000004
#define PERF_ATTRIB_DISPLAY_AS_REAL    0x00000008
#define PERF_ATTRIB_DISPLAY_AS_HEX     0x00000010

typedef struct _PROVIDER_CONTEXT {
    DWORD ContextSize;
    DWORD Reserved;
    PERFLIBREQUEST ControlCallback;
    PERF_MEM_ALLOC MemAllocRoutine;
    PERF_MEM_FREE MemFreeRoutine;
    LPVOID pMemContext;
} PERF_PROVIDER_CONTEXT, * PPERF_PROVIDER_CONTEXT;

typedef struct _PERF_COUNTER_IDENTIFIER {
    GUID CounterSetGuid;
    ULONG Status;
    ULONG Size;
    ULONG CounterId;
    ULONG InstanceId;
    ULONG Index;
    ULONG Reserved;
} PERF_COUNTER_IDENTIFIER, *PPERF_COUNTER_IDENTIFIER;

#define PERF_WILDCARD_COUNTER  0xFFFFFFFF
#define PERF_WILDCARD_INSTANCE L"*"

typedef struct _PERF_DATA_HEADER {
    ULONG dwTotalSize;
    ULONG dwNumCounters;
    LONGLONG PerfTimeStamp;
    LONGLONG PerfTime100NSec;
    LONGLONG PerfFreq;
    SYSTEMTIME SystemTime;
} PERF_DATA_HEADER, *PPERF_DATA_HEADER;

PERF_COUNTERSET_INSTANCE WINAPI *PerfCreateInstance(HANDLE, const GUID *, const WCHAR *, ULONG);
ULONG WINAPI PerfDeleteInstance(HANDLE, PERF_COUNTERSET_INSTANCE *);
ULONG WINAPI PerfSetCounterRefValue(HANDLE, PERF_COUNTERSET_INSTANCE *, ULONG, void *);
ULONG WINAPI PerfSetCounterSetInfo(HANDLE, PERF_COUNTERSET_INFO *, ULONG);
ULONG WINAPI PerfStartProvider(GUID *, PERFLIBREQUEST, HANDLE *);
ULONG WINAPI PerfStartProviderEx(GUID *, PERF_PROVIDER_CONTEXT *, HANDLE *);
ULONG WINAPI PerfStopProvider(HANDLE);

ULONG WINAPI PerfAddCounters(HANDLE, PERF_COUNTER_IDENTIFIER *, DWORD);
ULONG WINAPI PerfCloseQueryHandle(HANDLE);
ULONG WINAPI PerfOpenQueryHandle(const WCHAR *, HANDLE *);
ULONG WINAPI PerfQueryCounterData(HANDLE, PERF_DATA_HEADER *, DWORD, DWORD *);

#ifdef __cplusplus
}       /* extern "C" */
#endif

#endif /* _PERFLIB_H_ */
