/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/nttimer.c
 * PURPOSE:         User-mode timers
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>


/* GLOBALS ******************************************************************/

#if 0
static GENERIC_MAPPING ExpTimerMapping = {
	STANDARD_RIGHTS_READ | TIMER_QUERY_STATE,
	STANDARD_RIGHTS_WRITE | TIMER_MODIFY_STATE,
	STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
	TIMER_ALL_ACCESS};
#endif


/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtCancelTimer (
	IN	HANDLE		TimerHandle,
	OUT	PBOOLEAN	CurrentState	OPTIONAL
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtCreateTimer (
	OUT	PHANDLE			TimerHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes	OPTIONAL,
	IN	TIMER_TYPE			TimerType
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtOpenTimer (
	OUT	PHANDLE			TimerHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtQueryTimer (
	IN	HANDLE	TimerHandle,
	IN	CINT	TimerInformationClass,
	OUT	PVOID	TimerInformation,
	IN	ULONG	Length,
	OUT	PULONG	ResultLength
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtSetTimer (
	IN	HANDLE			TimerHandle,
	IN	PLARGE_INTEGER		DueTime,
	IN	PTIMERAPCROUTINE	TimerApcRoutine,
	IN	PVOID			TimerContext,
	IN	BOOL			WakeTimer,
	IN	ULONG			Period		OPTIONAL,
	OUT	PBOOLEAN		PreviousState	OPTIONAL
	)
{
	UNIMPLEMENTED;
}
