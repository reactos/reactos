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
	OUT PHANDLE EventPairHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
}

NTSTATUS
STDCALL
ZwCreateEventPair(
	OUT PHANDLE EventPairHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
}


NTSTATUS
STDCALL
NtSetHighEventPair(
	IN HANDLE EventPairHandle
	)
{
}

NTSTATUS
STDCALL
ZwSetHighEventPair(
	IN HANDLE EventPairHandle
	)
{
}

NTSTATUS
STDCALL
NtSetHighWaitLowEventPair(
	IN HANDLE EventPairHandle
	)
{
}

NTSTATUS
STDCALL
ZwSetHighWaitLowEventPair(
	IN HANDLE EventPairHandle
	)
{
}


NTSTATUS
STDCALL
NtSetLowEventPair(
	HANDLE EventPairHandle
	)
{
}

NTSTATUS
STDCALL
ZwSetLowEventPair(
	HANDLE EventPairHandle
	)
{
}


NTSTATUS
STDCALL
NtSetLowWaitHighEventPair(
	HANDLE EventPairHandle
	)
{
}

NTSTATUS
STDCALL
ZwSetLowWaitHighEventPair(
	HANDLE EventPairHandle
	)
{
}

NTSTATUS
STDCALL
NtWaitLowEventPair(
	IN HANDLE EventPairHandle
	)
{
}

NTSTATUS
STDCALL
ZwWaitLowEventPair(
	IN HANDLE EventPairHandle
	)
{
}

NTSTATUS
STDCALL
NtOpenEventPair(	
	OUT PHANDLE EventPairHandle,
        IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
}

NTSTATUS
STDCALL
ZwOpenEventPair(	
	OUT PHANDLE EventPairHandle,
        IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
}

NTSTATUS
STDCALL
NtWaitHighEventPair(
	IN HANDLE EventPairHandle
	)
{
}

NTSTATUS
STDCALL
ZwWaitHighEventPair(
	IN HANDLE EventPairHandle
	)
{
}