/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/efi.c
 * PURPOSE:         EFI Unimplemented Function Calls
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 16/07/04
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtAddBootEntry(
	IN PUNICODE_STRING EntryName,
	IN PUNICODE_STRING EntryValue
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtDeleteBootEntry(
	IN PUNICODE_STRING EntryName,
	IN PUNICODE_STRING EntryValue
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtEnumerateBootEntries(
	IN ULONG Unknown1,
	IN ULONG Unknown2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtQueryBootEntryOrder(
	IN ULONG Unknown1,
	IN ULONG Unknown2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtQueryBootOptions(
	IN ULONG Unknown1,
	IN ULONG Unknown2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSetBootEntryOrder(
	IN ULONG Unknown1,
	IN ULONG Unknown2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtSetBootOptions(
	ULONG Unknown1, 
	ULONG Unknown2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtTranslateFilePath(
	ULONG Unknown1, 
	ULONG Unknown2,
	ULONG Unknown3
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/* EOF */
