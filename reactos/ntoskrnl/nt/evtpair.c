/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/evtpair.c
 * PURPOSE:         Support for event pairs
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtCreateEventPair (
	OUT	PHANDLE			EventPairHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtSetHighEventPair (
	IN	HANDLE	EventPairHandle
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtSetHighWaitLowEventPair (
	IN	HANDLE	EventPairHandle
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtSetLowEventPair (
	HANDLE	EventPairHandle
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtSetLowWaitHighEventPair (
	HANDLE	EventPairHandle
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtWaitLowEventPair (
	IN	HANDLE	EventPairHandle
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtOpenEventPair (
	OUT	PHANDLE			EventPairHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtWaitHighEventPair (
	IN	HANDLE	EventPairHandle
	)
{
	UNIMPLEMENTED;
}

/* EOF */
