/* $Id: delete.c,v 1.10 2002/04/27 19:13:41 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/delete.c
 * PURPOSE:         Deleting files
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


/* FUNCTIONS ****************************************************************/

WINBOOL
STDCALL
DeleteFileA (
	LPCSTR	lpFileName
	)
{
	UNICODE_STRING FileNameU;
	ANSI_STRING FileName;
	WINBOOL Result;

	RtlInitAnsiString (&FileName,
	                   (LPSTR)lpFileName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&FileNameU,
		                              &FileName,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&FileNameU,
		                             &FileName,
		                             TRUE);

	Result = DeleteFileW (FileNameU.Buffer);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             FileNameU.Buffer);

	return Result;
}


WINBOOL
STDCALL
DeleteFileW (
	LPCWSTR	lpFileName
	)
{
	FILE_DISPOSITION_INFORMATION FileDispInfo;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING NtPathU;
	HANDLE FileHandle;
	NTSTATUS Status;

	DPRINT("DeleteFileW (lpFileName %S)\n",lpFileName);

	if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFileName,
	                                   &NtPathU,
	                                   NULL,
	                                   NULL))
		return FALSE;

	DPRINT("NtPathU \'%wZ\'\n", &NtPathU);

	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
	ObjectAttributes.ObjectName = &NtPathU;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	Status = NtCreateFile (&FileHandle,
	                       FILE_WRITE_ATTRIBUTES,
	                       &ObjectAttributes,
	                       &IoStatusBlock,
	                       NULL,
	                       FILE_ATTRIBUTE_NORMAL,
	                       0,
	                       FILE_OPEN,
                               FILE_NON_DIRECTORY_FILE,
	                       NULL,
	                       0);

	RtlFreeUnicodeString(&NtPathU);

	if (!NT_SUCCESS(Status))
	{
		CHECKPOINT;
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	FileDispInfo.DoDeleteFile = TRUE;

	Status = NtSetInformationFile (FileHandle,
	                               &IoStatusBlock,
	                               &FileDispInfo,
	                               sizeof(FILE_DISPOSITION_INFORMATION),
	                               FileDispositionInformation);
	if (!NT_SUCCESS(Status))
	{
		CHECKPOINT;
		NtClose (FileHandle);
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	Status = NtClose (FileHandle);
	if (!NT_SUCCESS (Status))
	{
		CHECKPOINT;
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	return TRUE;
}

/* EOF */
