/* $Id$
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

#include <k32.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32file);

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
STDCALL
DeleteFileA (
	LPCSTR	lpFileName
	)
{
	PWCHAR FileNameW;

   if (!(FileNameW = FilenameA2W(lpFileName, FALSE)))
      return FALSE;

	return DeleteFileW (FileNameW);
}


/*
 * @implemented
 */
BOOL
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

	TRACE("DeleteFileW (lpFileName %S)\n",lpFileName);

	if (!RtlDosPathNameToNtPathName_U (lpFileName,
	                                   &NtPathU,
	                                   NULL,
	                                   NULL))
   {
      SetLastError(ERROR_PATH_NOT_FOUND);
		return FALSE;
   }

	TRACE("NtPathU \'%wZ\'\n", &NtPathU);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &NtPathU,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

	Status = NtCreateFile (&FileHandle,
	                       DELETE,
	                       &ObjectAttributes,
	                       &IoStatusBlock,
	                       NULL,
	                       FILE_ATTRIBUTE_NORMAL,
	                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	                       FILE_OPEN,
                               FILE_NON_DIRECTORY_FILE,
	                       NULL,
	                       0);

	RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    NtPathU.Buffer);

	if (!NT_SUCCESS(Status))
	{
		WARN("Status 0x%08x\n", Status);
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	FileDispInfo.DeleteFile = TRUE;

	Status = NtSetInformationFile (FileHandle,
	                               &IoStatusBlock,
	                               &FileDispInfo,
	                               sizeof(FILE_DISPOSITION_INFORMATION),
	                               FileDispositionInformation);
	if (!NT_SUCCESS(Status))
	{
		WARN("Status 0x%08x\n", Status);
		NtClose (FileHandle);
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	Status = NtClose (FileHandle);
	if (!NT_SUCCESS (Status))
	{
		WARN("Status 0x%08x\n", Status);
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	return TRUE;
}

/* EOF */
