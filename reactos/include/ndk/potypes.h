/* $Id: potypes.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
 *
 *  ReactOS Headers
 *  Copyright (C) 1998-2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/potypes.h
 * PURPOSE:         Defintions for Power Manager Types not documented in DDK/IFS.
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _POTYPES_H
#define _POTYPES_H

typedef struct _PROCESSOR_IDLE_TIMES {
	ULONGLONG StartTime;
	ULONGLONG EndTime;
	ULONG IdleHandlerReserved[4];
} PROCESSOR_IDLE_TIMES, *PPROCESSOR_IDLE_TIMES;

typedef struct _PROCESSOR_PERF_STATE {
	UCHAR PercentFrequency;
	UCHAR MinCapacity;
	USHORT Power;
	UCHAR IncreaseLevel;
	UCHAR DecreaseLevel;
	USHORT Flags;
	ULONG IncreaseTime;
	ULONG DecreaseTime;
	ULONG IncreaseCount;
	ULONG DecreaseCount;
	ULONGLONG PerformanceTime;
} PROCESSOR_PERF_STATE, *PPROCESSOR_PERF_STATE;

typedef struct _PROCESSOR_POWER_STATE {
	PVOID IdleFunction;
	ULONG Idle0KernelTimeLimit;
	ULONG Idle0LastTime;
	PVOID IdleHandlers;
	PVOID IdleState;
	ULONG IdleHandlersCount;
	ULONGLONG LastCheck;
	PROCESSOR_IDLE_TIMES IdleTimes;
	ULONG IdleTime1;
	ULONG PromotionCheck;
	ULONG IdleTime2;
	UCHAR CurrentThrottle;
	UCHAR ThermalThrottleLimit;
	UCHAR CurrentThrottleIndex;
	UCHAR ThermalThrottleIndex;
	ULONG LastKernelUserTime;
	ULONG PerfIdleTime;
	ULONG DebugDelta;
	ULONG DebugCount;
	ULONG LastSysTime;
	ULONG TotalIdleStateTime[3];
	ULONG TotalIdleTransitions[3];
	ULONGLONG PreviousC3StateTime;
	UCHAR KneeThrottleIndex;
	UCHAR ThrottleLimitIndex;
	UCHAR PerfStatesCount;
	UCHAR ProcessorMinThrottle;
	UCHAR ProcessorMaxThrottle;
	UCHAR LastBusyPercentage;
	UCHAR LastC3Percentage;
	UCHAR LastAdjustedBusyPercentage;
	ULONG PromotionCount;
	ULONG DemotionCount;
	ULONG ErrorCount;
	ULONG RetryCount;
	ULONG Flags;
	LARGE_INTEGER PerfCounterFrequency;
	ULONG PerfTickCount;
	KTIMER PerfTimer;
	KDPC PerfDpc;
	PROCESSOR_PERF_STATE *PerfStates;
	PVOID PerfSetThrottle;
	ULONG LastC3KernelUserTime;
	ULONG Spare1[1];
} PROCESSOR_POWER_STATE, *PPROCESSOR_POWER_STATE;

#endif
