/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/atom.c
 * PURPOSE:         Atom managment
 * PROGRAMMER:      Nobody
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtAddAtom (
	OUT	ATOM		* Atom,
	IN	PUNICODE_STRING	AtomString
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtDeleteAtom (
	IN	ATOM	Atom
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtFindAtom (
	OUT	ATOM		* Atom,
	IN	PUNICODE_STRING	AtomString
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtQueryInformationAtom (
	IN	HANDLE	AtomHandle,
	IN	CINT	AtomInformationClass,
	OUT	PVOID	AtomInformation,
	IN	ULONG	AtomInformationLength,
	OUT	PULONG	ReturnLength
	)
{
	UNIMPLEMENTED;
}
