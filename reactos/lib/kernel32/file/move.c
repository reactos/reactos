/* $Id: move.c,v 1.4 2000/06/29 23:35:24 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <kernel32/kernel32.h>


#define FILE_RENAME_SIZE  MAX_PATH +sizeof(FILE_RENAME_INFORMATION)


/* FUNCTIONS ****************************************************************/

WINBOOL
STDCALL
MoveFileA (
	LPCSTR	lpExistingFileName,
	LPCSTR	lpNewFileName
	)
{
	return MoveFileExA (lpExistingFileName,
	                    lpNewFileName,
	                    MOVEFILE_COPY_ALLOWED);
}


WINBOOL
STDCALL
MoveFileExA (
	LPCSTR	lpExistingFileName,
	LPCSTR	lpNewFileName,
	DWORD	dwFlags
	)
{
	UNICODE_STRING ExistingFileNameU;
	UNICODE_STRING NewFileNameU;
	ANSI_STRING ExistingFileName;
	ANSI_STRING NewFileName;
	WINBOOL Result;

	RtlInitAnsiString (&ExistingFileName,
	                   (LPSTR)lpExistingFileName);

	RtlInitAnsiString (&NewFileName,
	                   (LPSTR)lpNewFileName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
	{
		RtlAnsiStringToUnicodeString (&ExistingFileNameU,
		                              &ExistingFileName,
		                              TRUE);
		RtlAnsiStringToUnicodeString (&NewFileNameU,
		                              &NewFileName,
		                              TRUE);
	}
	else
	{
		RtlOemStringToUnicodeString (&ExistingFileNameU,
		                             &ExistingFileName,
		                             TRUE);
		RtlOemStringToUnicodeString (&NewFileNameU,
		                             &NewFileName,
		                             TRUE);
	}

	Result = MoveFileExW (ExistingFileNameU.Buffer,
	                      NewFileNameU.Buffer,
	                      dwFlags);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             ExistingFileNameU.Buffer);
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             NewFileNameU.Buffer);

	return Result;
}


WINBOOL
STDCALL
MoveFileW (
	LPCWSTR	lpExistingFileName,
	LPCWSTR	lpNewFileName
	)
{
	return MoveFileExW (lpExistingFileName,
	                    lpNewFileName,
	                    MOVEFILE_COPY_ALLOWED);
}


WINBOOL
STDCALL
MoveFileExW (
	LPCWSTR	lpExistingFileName,
	LPCWSTR	lpNewFileName,
	DWORD	dwFlags
	)
{
	HANDLE hFile = NULL;
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_RENAME_INFORMATION *FileRename;
	USHORT Buffer[FILE_RENAME_SIZE];
	NTSTATUS errCode;	

	hFile = CreateFileW (lpExistingFileName,
	                     GENERIC_ALL,
	                     FILE_SHARE_WRITE|FILE_SHARE_READ,
	                     NULL,
	                     OPEN_EXISTING,
	                     FILE_ATTRIBUTE_NORMAL,
	                     NULL);

	FileRename = (FILE_RENAME_INFORMATION *)Buffer;
	if ((dwFlags & MOVEFILE_REPLACE_EXISTING) == MOVEFILE_REPLACE_EXISTING)
		FileRename->Replace = TRUE;
	else
		FileRename->Replace = FALSE;

	FileRename->FileNameLength = wcslen (lpNewFileName);
	memcpy (FileRename->FileName,
	        lpNewFileName,
	        min(FileRename->FileNameLength, MAX_PATH));
	
	errCode = NtSetInformationFile (hFile,
	                                &IoStatusBlock,
	                                FileRename,
	                                FILE_RENAME_SIZE,
	                                FileRenameInformation);
	if (!NT_SUCCESS(errCode))
	{
		if (CopyFileW (lpExistingFileName,
		               lpNewFileName,
		               FileRename->Replace))
			DeleteFileW (lpExistingFileName);
	}

	CloseHandle(hFile);
	return TRUE;
}

/* EOF */
