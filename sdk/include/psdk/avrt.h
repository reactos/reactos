/*
 * avrt definitions
 *
 * Copyright 2009 Maarten Lankhorst
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

#ifndef _AVRT_
#define _AVRT_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _AVRT_PRIORITY
{
    AVRT_PRIORITY_VERYLOW = -2,
    AVRT_PRIORITY_LOW,
    AVRT_PRIORITY_NORMAL,
    AVRT_PRIORITY_HIGH,
    AVRT_PRIORITY_CRITICAL
} AVRT_PRIORITY, *PAVRT_PRIORITY;

/* Windows compiler is more lax */
#define THREAD_ORDER_GROUP_INFINITE_TIMEOUT ((void*)((LONG_PTR)-1))

HANDLE WINAPI AvSetMmThreadCharacteristicsA(LPCSTR TaskName, LPDWORD TaskIndex);
HANDLE WINAPI AvSetMmThreadCharacteristicsW(LPCWSTR TaskName, LPDWORD TaskIndex);
#define AvSetMmThreadCharacteristics WINELIB_NAME_AW(AvSetMmThreadCharacteristics)

HANDLE WINAPI AvSetMmMaxThreadCharacteristicsA(LPCSTR FirstTask, LPCSTR SecondTask, LPDWORD TaskIndex);
HANDLE WINAPI AvSetMmMaxThreadCharacteristicsW(LPCWSTR FirstTask, LPCWSTR SecondTask, LPDWORD TaskIndex);
#define AvSetMmMaxThreadCharacteristics WINELIB_NAME_AW(AvSetMmMaxThreadCharacteristics)

BOOL WINAPI AvRevertMmThreadCharacteristics(HANDLE AvrtHandle);
BOOL WINAPI AvSetMmThreadPriority(HANDLE AvrtHandle, AVRT_PRIORITY Priority);
BOOL WINAPI AvRtCreateThreadOrderingGroup(
    PHANDLE Context, PLARGE_INTEGER Period,
    GUID *ThreadOrderingGuid, PLARGE_INTEGER Timeout);

BOOL WINAPI AvRtCreateThreadOrderingGroupExA(
    PHANDLE Context, PLARGE_INTEGER Period,
    GUID *ThreadOrderingGuid, PLARGE_INTEGER Timeout,
    LPCSTR TaskName);
BOOL WINAPI AvRtCreateThreadOrderingGroupExW(
    PHANDLE Context, PLARGE_INTEGER Period,
    GUID *ThreadOrderingGuid, PLARGE_INTEGER Timeout,
    LPCSTR TaskName);
#define AvRtCreateThreadOrderingGroupEx WINELIB_NAME_AW(AvRtCreateThreadOrderingGroupEx)

BOOL WINAPI AvRtJoinThreadOrderingGroup(PHANDLE Context, GUID *ThreadOrderingGuid, BOOL Before);
BOOL WINAPI AvRtWaitOnThreadOrderingGroup(HANDLE Context);
BOOL WINAPI AvRtLeaveThreadOrderingGroup(HANDLE Context);
BOOL WINAPI AvRtDeleteThreadOrderingGroup(HANDLE Context);
BOOL WINAPI AvQuerySystemResponsiveness(HANDLE AvrtHandle, PULONG SystemResponsivenessValue);

#ifdef __cplusplus
}
#endif

#endif /*_AVRT_*/
