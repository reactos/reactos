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

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtCancelTimer(IN HANDLE TimerHandle,
			       OUT PBOOLEAN CurrentState OPTIONAL)
{
   return(ZwCancelTimer(TimerHandle,
			CurrentState));
}

NTSTATUS STDCALL ZwCancelTimer(IN HANDLE TimerHandle,
			       OUT ULONG ElapsedTime)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtCreateTimer(OUT PHANDLE TimerHandle,
			       IN ACCESS_MASK DesiredAccess,
			       IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
			       IN CINT TimerType)
{
   return(ZwCreateTimer(TimerHandle,
			DesiredAccess,
			ObjectAttributes,
			TimerType));
}

NTSTATUS STDCALL ZwCreateTimer(OUT PHANDLE TimerHandle,
			       IN ACCESS_MASK DesiredAccess,
			       IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
			       IN CINT TimerType)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtOpenTimer(OUT PHANDLE TimerHandle,
			     IN ACCESS_MASK DesiredAccess,
			     IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwOpenTimer(TimerHandle,
		      DesiredAccess,
		      ObjectAttributes));
}

NTSTATUS STDCALL ZwOpenTimer(OUT PHANDLE TimerHandle,
			     IN ACCESS_MASK DesiredAccess,
			     IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQueryTimer(IN HANDLE TimerHandle,
			      IN CINT TimerInformationClass,
			      OUT PVOID TimerInformation,
			      IN ULONG Length,
			      OUT PULONG ResultLength)
{
   return(ZwQueryTimer(TimerHandle,
		       TimerInformationClass,
		       TimerInformation,
		       Length,
		       ResultLength));
}

NTSTATUS STDCALL ZwQueryTimer(IN HANDLE TimerHandle,
			      IN CINT TimerInformationClass,
			      OUT PVOID TimerInformation,
			      IN ULONG Length,
			      OUT PULONG ResultLength)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtSetTimer(IN HANDLE TimerHandle,
			    IN PLARGE_INTEGER DueTime,
			    IN PTIMERAPCROUTINE TimerApcRoutine,
			    IN PVOID TimerContext,
			    IN BOOL WakeTimer,
			    IN ULONG Period OPTIONAL,
			    OUT PBOOLEAN PreviousState OPTIONAL)
{
   return(ZwSetTimer(TimerHandle,
		     DueTime,
		     TimerApcRoutine,
		     TimerContext,
		     WakeTimer,
		     Period,
		     PreviousState));
}

NTSTATUS STDCALL ZwSetTimer(IN HANDLE TimerHandle,
			    IN PLARGE_INTEGER DueTime,
			    IN PTIMERAPCROUTINE TimerApcRoutine,
			    IN PVOID TimerContext,
			    IN BOOL WakeTimer,
			    IN ULONG Period OPTIONAL,
			    OUT PBOOLEAN PreviousState OPTIONAL)
{
   UNIMPLEMENTED;
}
