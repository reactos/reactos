/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/section.c
 * PURPOSE:         Implements section objects
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS ZwOpenSection(PHANDLE SectionHandle,
		       ACCESS_MASK DesiredAccess,
		       POBJECT_ATTRIBUTES ObjectAttributes)
{
   UNIMPLEMENTED;
}

NTSTATUS ZwMapViewOfSection(HANDLE SectionHandle,
			    HANDLE ProcessHandle,
			    PVOID* BaseAddress,
			    ULONG ZeroBits,
			    ULONG CommitSize,
			    PLARGE_INTEGER SectionOffset,
			    PULONG ViewSize,
			    SECTION_INHERIT InheritDisposition,
			    ULONG AllocationType,
			    ULONG Protect)
{
   UNIMPLEMENTED;
}

NTSTATUS ZwUnmapViewOfSection(HANDLE ProcessHandle, PVOID BaseAddress)
{
   UNIMPLEMENTED;
}
