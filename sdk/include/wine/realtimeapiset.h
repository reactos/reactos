/*
 * Copyright (C) 2023 Mohamad Al-Jaf
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

#ifndef _REALTIMEAPISET_H_
#define _REALTIMEAPISET_H_

#ifdef __cplusplus
extern "C" {
#endif

WINBASEAPI HRESULT WINAPI ConvertAuxiliaryCounterToPerformanceCounter(ULONGLONG,ULONGLONG*,ULONGLONG*);
WINBASEAPI HRESULT WINAPI ConvertPerformanceCounterToAuxiliaryCounter(ULONGLONG,ULONGLONG*,ULONGLONG*);
WINBASEAPI HRESULT WINAPI QueryAuxiliaryCounterFrequency(ULONGLONG*);
WINBASEAPI BOOL    WINAPI QueryIdleProcessorCycleTime(ULONG*,ULONG64*);
WINBASEAPI BOOL    WINAPI QueryIdleProcessorCycleTimeEx(USHORT,ULONG*,ULONG64*);
WINBASEAPI VOID    WINAPI QueryInterruptTime(ULONGLONG*);
WINBASEAPI VOID    WINAPI QueryInterruptTimePrecise(ULONGLONG*);
WINBASEAPI BOOL    WINAPI QueryProcessCycleTime(HANDLE,ULONG64*);
WINBASEAPI BOOL    WINAPI QueryThreadCycleTime(HANDLE,ULONG64*);
WINBASEAPI BOOL    WINAPI QueryUnbiasedInterruptTime(ULONGLONG*);
WINBASEAPI VOID    WINAPI QueryUnbiasedInterruptTimePrecise(ULONGLONG*);

#ifdef __cplusplus
}
#endif

#endif /* _REALTIMEAPISET_H_ */
