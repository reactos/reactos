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

NTSTATUS STDCALL NtAddAtom(OUT ATOM *Atom,
			   IN PUNICODE_STRING AtomString)
{
}

NTSTATUS
STDCALL
ZwAddAtom(
	OUT ATOM *Atom,
	IN PUNICODE_STRING AtomString
	)
{
}

NTSTATUS
STDCALL
NtDeleteAtom(
	IN ATOM Atom
	)
{
}

NTSTATUS
STDCALL
ZwDeleteAtom(
	IN ATOM Atom
	)
{
}

NTSTATUS
STDCALL
NtFindAtom(
	OUT ATOM *Atom,
	IN PUNICODE_STRING AtomString
	)
{
}

NTSTATUS
STDCALL
ZwFindAtom(
	OUT ATOM *Atom,
	IN PUNICODE_STRING AtomString
	)
{
}


NTSTATUS
STDCALL
NtQueryInformationAtom(
	IN HANDLE AtomHandle,
	IN CINT AtomInformationClass,
	OUT PVOID AtomInformation,
	IN ULONG AtomInformationLength,
	OUT PULONG ReturnLength
	)
{
}
