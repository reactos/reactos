/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
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
NtCreateEventPair(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
}

NTSTATUS
STDCALL
ZwCreateEventPair(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
}


NTSTATUS
STDCALL
NtSetHighEventPair(
	IN HANDLE EventPair
	)
{
}

NTSTATUS
STDCALL
ZwSetHighEventPair(
	IN HANDLE EventPair
	)
{
}

NTSTATUS
STDCALL
NtSetHighWaitLowEventPair(
	IN HANDLE EventPair
	)
{
}

NTSTATUS
STDCALL
ZwSetHighWaitLowEventPair(
	IN HANDLE EventPair
	)
{
}


NTSTATUS
STDCALL
NtSetLowEventPair(
	HANDLE EventPair
	)
{
}

NTSTATUS
STDCALL
ZwSetLowEventPair(
	HANDLE EventPair
	)
{
}


NTSTATUS
STDCALL
NtSetLowWaitHighEventPair(
	HANDLE EventPair
	)
{
}

NTSTATUS
STDCALL
ZwSetLowWaitHighEventPair(
	HANDLE EventPair
	)
{
}

NTSTATUS
STDCALL
NtWaitLowEventPair(
	IN HANDLE EventHandle
	)
{
}

NTSTATUS
STDCALL
ZwWaitLowEventPair(
	IN HANDLE EventHandle
	)
{
}

NTSTATUS STDCALL NtOpenEventPair(VOID)
{
}

NTSTATUS STDCALL NtWaitHighEventPair(VOID)
{
}
