/*
 * ntpoapi.h
 *
 * APIs for power management.
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __NTPOAPI_H
#define __NTPOAPI_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"
#include "batclass.h"

#define POWER_PERF_SCALE                  100
#define PERF_LEVEL_TO_PERCENT(x)          (((x) * 1000) / (POWER_PERF_SCALE * 10))
#define PERCENT_TO_PERF_LEVEL(x)          (((x) * POWER_PERF_SCALE * 10) / 1000)

typedef struct _PROCESSOR_IDLE_TIMES {
	ULONGLONG  StartTime;
	ULONGLONG  EndTime;
	ULONG  IdleHandlerReserved[4];
} PROCESSOR_IDLE_TIMES, *PPROCESSOR_IDLE_TIMES;

typedef BOOLEAN DDKFASTAPI
(*PPROCESSOR_IDLE_HANDLER)(
  IN OUT PPROCESSOR_IDLE_TIMES IdleTimes);

typedef struct _PROCESSOR_IDLE_HANDLER_INFO {
  ULONG  HardwareLatency;
  PPROCESSOR_IDLE_HANDLER  Handler;
} PROCESSOR_IDLE_HANDLER_INFO, *PPROCESSOR_IDLE_HANDLER_INFO;

typedef VOID DDKFASTAPI
(*PSET_PROCESSOR_THROTTLE)(
  IN UCHAR  Throttle);

typedef NTSTATUS DDKFASTAPI
(*PSET_PROCESSOR_THROTTLE2)(
  IN UCHAR  Throttle);

#define MAX_IDLE_HANDLERS                 3

typedef struct _PROCESSOR_STATE_HANDLER {
	UCHAR  ThrottleScale;
	BOOLEAN  ThrottleOnIdle;
	PSET_PROCESSOR_THROTTLE  SetThrottle;
	ULONG  NumIdleHandlers;
	PROCESSOR_IDLE_HANDLER_INFO  IdleHandler[MAX_IDLE_HANDLERS];
} PROCESSOR_STATE_HANDLER, *PPROCESSOR_STATE_HANDLER;

typedef enum _POWER_STATE_HANDLER_TYPE {
	PowerStateSleeping1,
	PowerStateSleeping2,
	PowerStateSleeping3,
	PowerStateSleeping4,
	PowerStateSleeping4Firmware,
	PowerStateShutdownReset,
	PowerStateShutdownOff,
	PowerStateMaximum
} POWER_STATE_HANDLER_TYPE, *PPOWER_STATE_HANDLER_TYPE;

typedef NTSTATUS DDKAPI
(*PENTER_STATE_SYSTEM_HANDLER)(
  IN PVOID  SystemContext);

typedef NTSTATUS DDKAPI
(*PENTER_STATE_HANDLER)(
  IN PVOID  Context,
  IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler  OPTIONAL,
  IN PVOID  SystemContext,
  IN LONG  NumberProcessors,
  IN VOLATILE PLONG  Number);

typedef struct _POWER_STATE_HANDLER {
	POWER_STATE_HANDLER_TYPE  Type;
	BOOLEAN  RtcWake;
	UCHAR  Spare[3];
	PENTER_STATE_HANDLER  Handler;
	PVOID  Context;
} POWER_STATE_HANDLER, *PPOWER_STATE_HANDLER;

typedef NTSTATUS STDCALL
(*PENTER_STATE_NOTIFY_HANDLER)(
  IN POWER_STATE_HANDLER_TYPE  State,
  IN PVOID  Context,
  IN BOOLEAN  Entering);

typedef struct _POWER_STATE_NOTIFY_HANDLER {
	PENTER_STATE_NOTIFY_HANDLER  Handler;
	PVOID  Context;
} POWER_STATE_NOTIFY_HANDLER, *PPOWER_STATE_NOTIFY_HANDLER;

NTOSAPI
NTSTATUS
DDKAPI
NtPowerInformation(
  IN POWER_INFORMATION_LEVEL  InformationLevel,
  IN PVOID  InputBuffer OPTIONAL,
  IN ULONG  InputBufferLength,
  OUT PVOID  OutputBuffer OPTIONAL,
  IN ULONG  OutputBufferLength);

#define PROCESSOR_STATE_TYPE_PERFORMANCE  1
#define PROCESSOR_STATE_TYPE_THROTTLE     2

typedef struct _PROCESSOR_PERF_LEVEL {
  UCHAR  PercentFrequency;
  UCHAR  Reserved;
  USHORT  Flags;
} PROCESSOR_PERF_LEVEL, *PPROCESSOR_PERF_LEVEL;

typedef struct _PROCESSOR_PERF_STATE {
  UCHAR  PercentFrequency;
  UCHAR  MinCapacity;
  USHORT  Power;
  UCHAR  IncreaseLevel;
  UCHAR  DecreaseLevel;
  USHORT  Flags;
  ULONG  IncreaseTime;
  ULONG  DecreaseTime;
  ULONG  IncreaseCount;
  ULONG  DecreaseCount;
  ULONGLONG  PerformanceTime;
} PROCESSOR_PERF_STATE, *PPROCESSOR_PERF_STATE;

typedef struct _PROCESSOR_STATE_HANDLER2 {
	ULONG  NumIdleHandlers;
	PROCESSOR_IDLE_HANDLER_INFO  IdleHandler[MAX_IDLE_HANDLERS];
	PSET_PROCESSOR_THROTTLE2  SetPerfLevel;
	ULONG  HardwareLatency;
	UCHAR  NumPerfStates;
	PROCESSOR_PERF_LEVEL  PerfLevel[1];
} PROCESSOR_STATE_HANDLER2, *PPROCESSOR_STATE_HANDLER2;


NTOSAPI
NTSTATUS
DDKAPI
NtSetThreadExecutionState(
  IN EXECUTION_STATE  esFlags,
  OUT EXECUTION_STATE  *PreviousFlags);

NTOSAPI
NTSTATUS
DDKAPI
NtRequestWakeupLatency(
  IN LATENCY_TIME  latency);

NTOSAPI
NTSTATUS
DDKAPI
NtInitiatePowerAction(
  IN POWER_ACTION  SystemAction,
  IN SYSTEM_POWER_STATE  MinSystemState,
  IN ULONG  Flags,
  IN BOOLEAN  Asynchronous);

NTOSAPI
NTSTATUS
DDKAPI
NtSetSystemPowerState(
  IN POWER_ACTION SystemAction,
  IN SYSTEM_POWER_STATE MinSystemState,
  IN ULONG Flags);

NTOSAPI
NTSTATUS
DDKAPI
NtGetDevicePowerState(
  IN HANDLE  Device,
  OUT DEVICE_POWER_STATE  *State);

NTOSAPI
NTSTATUS
DDKAPI
NtCancelDeviceWakeupRequest(
  IN HANDLE  Device);

NTOSAPI
BOOLEAN
DDKAPI
NtIsSystemResumeAutomatic(
  VOID);

NTOSAPI
NTSTATUS
DDKAPI
NtRequestDeviceWakeup(
  IN HANDLE  Device);

#define WINLOGON_LOCK_ON_SLEEP            0x00000001

typedef struct _PROCESSOR_POWER_INFORMATION {
  ULONG  Number;
  ULONG  MaxMhz;
  ULONG  CurrentMhz;
  ULONG  MhzLimit;
  ULONG  MaxIdleState;
  ULONG  CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

#ifdef __cplusplus
}
#endif

#endif /* __NTPOAPI_H */
