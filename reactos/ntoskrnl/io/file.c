/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/file.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS ZwQueryInformationFile(HANDLE FileHandle,
				PIO_STATUS_BLOCK IoStatusBlock,
				PVOID FileInformation,
				ULONG Length,
				FILE_INFORMATION_CLASS FileInformationClass)
{
   UNIMPLEMENTED;
}

NTSTATUS NtQueryInformationFile(HANDLE FileHandle,
				PIO_STATUS_BLOCK IoStatusBlock,
				PVOID FileInformation,
				ULONG Length,
				FILE_INFORMATION_CLASS FileInformationClass)
{
   UNIMPLEMENTED;
}

NTSTATUS ZwSetInformationFile(HANDLE FileHandle,
			      PIO_STATUS_BLOCK IoStatusBlock,
			      PVOID FileInformation,
			      ULONG Length,
			      FILE_INFORMATION_CLASS FileInformationClass)
{
   UNIMPLEMENTED;
}

NTSTATUS NtSetInformationFile(HANDLE FileHandle,
			      PIO_STATUS_BLOCK IoStatusBlock,
			      PVOID FileInformation,
			      ULONG Length,
			      FILE_INFORMATION_CLASS FileInformationClass)
{
   UNIMPLEMENTED;
}

PGENERIC_MAPPING IoGetFileObjectGenericMapping()
{
   UNIMPLEMENTED;
}

NTSTATUS 
STDCALL 
NtQueryAttributesFile(
	IN HANDLE FileHandle,
	IN PVOID Buffer
	)
{
}

NTSTATUS 
STDCALL 
ZwQueryAttributesFile(
	IN HANDLE FileHandle,
	IN PVOID Buffer
	)
{
}

NTSTATUS
STDCALL 
NtQueryFullAttributesFile(
	IN HANDLE FileHandle,
	IN PVOID Attributes
	)
{
}

NTSTATUS
STDCALL 
ZwQueryFullAttributesFile(
	IN HANDLE FileHandle,
	IN PVOID Attributes
	)
{
}

NTSTATUS
STDCALL
NtQueryEaFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID Buffer,
	IN ULONG Length,
	IN BOOLEAN ReturnSingleEntry,
	IN PVOID EaList OPTIONAL,
	IN ULONG EaListLength,
	IN PULONG EaIndex OPTIONAL,
	IN BOOLEAN RestartScan
	)
{
}

NTSTATUS
STDCALL
NtSetEaFile(
	IN HANDLE FileHandle,
	IN PIO_STATUS_BLOCK IoStatusBlock,	
	PVOID EaBuffer, 
	ULONG EaBufferSize
	)
{
}

NTSTATUS
STDCALL
ZwSetEaFile(
	IN HANDLE FileHandle,
	IN PIO_STATUS_BLOCK IoStatusBlock,	
	PVOID EaBuffer, 
	ULONG EaBufferSize
	)
{
}
