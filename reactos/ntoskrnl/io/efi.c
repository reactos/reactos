/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/efi.c
 * PURPOSE:         EFI Unimplemented Function Calls
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

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

NTSTATUS
STDCALL
NtEnumerateBootEntries(
    IN PVOID Buffer,
    IN PULONG BufferLength
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
NtQueryBootEntryOrder(
    IN PULONG Ids,
    IN PULONG Count
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
NtQueryBootOptions(
    IN PBOOT_OPTIONS BootOptions,
    IN PULONG BootOptionsLength
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
NtSetBootEntryOrder(
    IN PULONG Ids,
    IN PULONG Count
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
NtSetBootOptions(
    IN PBOOT_OPTIONS BootOptions,
    IN ULONG FieldsToChange
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
NtTranslateFilePath(
    PFILE_PATH InputFilePath,
    ULONG OutputType,
    PFILE_PATH OutputFilePath,
    ULONG OutputFilePathLength
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
