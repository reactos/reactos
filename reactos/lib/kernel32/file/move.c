/* $Id: move.c,v 1.9 2003/01/15 21:24:34 chorns Exp $
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

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


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
	return MoveFileWithProgressA (lpExistingFileName,
	                              lpNewFileName,
	                              NULL,
	                              NULL,
	                              dwFlags);
}


WINBOOL
STDCALL
MoveFileWithProgressA (
	LPCSTR			lpExistingFileName,
	LPCSTR			lpNewFileName,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	DWORD			dwFlags
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

	Result = MoveFileWithProgressW (ExistingFileNameU.Buffer,
	                                NewFileNameU.Buffer,
	                                lpProgressRoutine,
	                                lpData,
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
	return MoveFileWithProgressW (lpExistingFileName,
	                              lpNewFileName,
	                              NULL,
	                              NULL,
	                              dwFlags);
}


static WINBOOL
AdjustFileAttributes (
	LPCWSTR ExistingFileName,
	LPCWSTR NewFileName
	)
{
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_BASIC_INFORMATION ExistingInfo,
		NewInfo;
	HANDLE hFile;
	DWORD Attributes;
	NTSTATUS errCode;
	WINBOOL Result = FALSE;

	hFile = CreateFileW (ExistingFileName,
	                     FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
	                     FILE_SHARE_READ,
	                     NULL,
	                     OPEN_EXISTING,
	                     FILE_ATTRIBUTE_NORMAL,
	                     NULL);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		errCode = NtQueryInformationFile (hFile,
		                                  &IoStatusBlock,
		                                  &ExistingInfo,
		                                  sizeof(FILE_BASIC_INFORMATION),
		                                  FileBasicInformation);
		if (NT_SUCCESS (errCode))
		{
			if (0 != (ExistingInfo.FileAttributes & FILE_ATTRIBUTE_READONLY))
			{
				Attributes = ExistingInfo.FileAttributes;
				ExistingInfo.FileAttributes &= ~ FILE_ATTRIBUTE_READONLY;
				if (0 == (ExistingInfo.FileAttributes &
				          (FILE_ATTRIBUTE_HIDDEN |
				           FILE_ATTRIBUTE_SYSTEM |
				           FILE_ATTRIBUTE_ARCHIVE)))
				{
					ExistingInfo.FileAttributes |= FILE_ATTRIBUTE_NORMAL;
				}
				errCode = NtSetInformationFile (hFile,
				                                &IoStatusBlock,
				                                &ExistingInfo,
				                                sizeof(FILE_BASIC_INFORMATION),
				                                FileBasicInformation);
				if (!NT_SUCCESS(errCode))
				{
					DPRINT("Removing READONLY attribute from source failed with status 0x%08x\n", errCode);
				}
				ExistingInfo.FileAttributes = Attributes;
			}
			CloseHandle(hFile);

			if (NT_SUCCESS(errCode))
			{
				hFile = CreateFileW (NewFileName,
				                     FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
				                     FILE_SHARE_READ,
				                     NULL,
				                     OPEN_EXISTING,
			        	             FILE_ATTRIBUTE_NORMAL,
				                     NULL);
				if (INVALID_HANDLE_VALUE != hFile)
				{
					errCode = NtQueryInformationFile(hFile,
					                                 &IoStatusBlock,
					                                 &NewInfo,
					                                 sizeof(FILE_BASIC_INFORMATION),
					                                 FileBasicInformation);
					if (NT_SUCCESS(errCode))
					{
						NewInfo.FileAttributes = (NewInfo.FileAttributes &
						                          ~ (FILE_ATTRIBUTE_HIDDEN |
						                             FILE_ATTRIBUTE_SYSTEM |
						                             FILE_ATTRIBUTE_READONLY |
						                             FILE_ATTRIBUTE_NORMAL)) |
					                                 (ExistingInfo.FileAttributes &
					                                  (FILE_ATTRIBUTE_HIDDEN |
						                           FILE_ATTRIBUTE_SYSTEM |
						                           FILE_ATTRIBUTE_READONLY |
						                           FILE_ATTRIBUTE_NORMAL)) |
						                         FILE_ATTRIBUTE_ARCHIVE;
						NewInfo.CreationTime = ExistingInfo.CreationTime;
						NewInfo.LastAccessTime = ExistingInfo.LastAccessTime;
						NewInfo.LastWriteTime = ExistingInfo.LastWriteTime;
						errCode = NtSetInformationFile (hFile,
						                                &IoStatusBlock,
						                                &NewInfo,
						                                sizeof(FILE_BASIC_INFORMATION),
						                                FileBasicInformation);
						if (NT_SUCCESS(errCode))
						{
							Result = TRUE;
						}
						else
						{
							DPRINT("Setting attributes on dest file failed with status 0x%08x\n", errCode);
						}
					}
					else
					{
						DPRINT("Obtaining attributes from dest file failed with status 0x%08x\n", errCode);
					}
					CloseHandle(hFile);
				}
				else
				{
					DPRINT("Opening dest file to set attributes failed with code %d\n", GetLastError());
				}
			}
		}
		else
		{
			DPRINT("Obtaining attributes from source file failed with status 0x%08x\n", errCode);
			CloseHandle(hFile);
		}
	}
	else
	{
		DPRINT("Opening source file to obtain attributes failed with code %d\n", GetLastError());
	}

	return Result;
}


WINBOOL
STDCALL
MoveFileWithProgressW (
	LPCWSTR			lpExistingFileName,
	LPCWSTR			lpNewFileName,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	DWORD			dwFlags
	)
{
	HANDLE hFile = NULL;
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_RENAME_INFORMATION *FileRename;
	USHORT Buffer[FILE_RENAME_SIZE];
	NTSTATUS errCode;
	DWORD err;
	WINBOOL Result;

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
	CloseHandle(hFile);
	if (NT_SUCCESS(errCode))
	{
		Result = TRUE;
	}
	/* FIXME file rename not yet implemented in all FSDs so it will always
	 * fail, even when the move is to the same device
	 */
#if 0
	else if (STATUS_NOT_SAME_DEVICE == errCode &&
		 MOVEFILE_COPY_ALLOWED == (dwFlags & MOVEFILE_COPY_ALLOWED))
#else
	else
#endif
	{
		Result = CopyFileExW (lpExistingFileName,
		                      lpNewFileName,
		                      lpProgressRoutine,
		                      lpData,
		                      NULL,
		                      FileRename->Replace ? 0 : COPY_FILE_FAIL_IF_EXISTS) &&
		         AdjustFileAttributes(lpExistingFileName, lpNewFileName) &&
		         DeleteFileW (lpExistingFileName);
		if (! Result)
		{
			/* Delete of the existing file failed so the
			 * existing file is still there. Clean up the
			 * new file (if possible)
			 */
			err = GetLastError();
			if (! SetFileAttributesW (lpNewFileName, FILE_ATTRIBUTE_NORMAL))
			{
				DPRINT("Removing possible READONLY attrib from new file failed with code %d\n", GetLastError());
			}
			if (! DeleteFileW (lpNewFileName))
			{
				DPRINT("Deleting new file during cleanup failed with code %d\n", GetLastError());
			}
			SetLastError (err);
		}
	}
	/* See FIXME above */
#if 0
	else
	{
		SetLastErrorByStatus (errCode);
		Result = FALSE;
	}
#endif

	return Result;
}

/* EOF */
