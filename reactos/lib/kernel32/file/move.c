/* $Id$
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
#include "../include/debug.h"

/* GLOBALS *****************************************************************/

/* FUNCTIONS ****************************************************************/

static BOOL
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
	BOOL Result = FALSE;

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


/*
 * @implemented
 */
BOOL
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
	PFILE_RENAME_INFORMATION FileRename;
	NTSTATUS errCode;
	BOOL Result;
	UNICODE_STRING DstPathU;

	DPRINT("MoveFileWithProgressW()\n");

	hFile = CreateFileW (lpExistingFileName,
	                     GENERIC_ALL,
	                     FILE_SHARE_WRITE|FILE_SHARE_READ,
	                     NULL,
	                     OPEN_EXISTING,
	                     FILE_ATTRIBUTE_NORMAL,
	                     NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
           return FALSE;
	}

        /* validate & translate the filename */
        if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpNewFileName,
				           &DstPathU,
				           NULL,
				           NULL))
        {
           DPRINT("Invalid destination path\n");
	   CloseHandle(hFile);
           SetLastError(ERROR_PATH_NOT_FOUND);
           return FALSE;
        }

	FileRename = alloca(sizeof(FILE_RENAME_INFORMATION) + DstPathU.Length);
	if ((dwFlags & MOVEFILE_REPLACE_EXISTING) == MOVEFILE_REPLACE_EXISTING)
		FileRename->ReplaceIfExists = TRUE;
	else
		FileRename->ReplaceIfExists = FALSE;

	memcpy(FileRename->FileName, DstPathU.Buffer, DstPathU.Length);
        RtlFreeHeap (RtlGetProcessHeap (),
		     0,
		     DstPathU.Buffer);
	/*
	 * FIXME:
	 *   Is the length the count of characters or the length of the buffer?
	 */
	FileRename->FileNameLength = DstPathU.Length / sizeof(WCHAR);
	errCode = NtSetInformationFile (hFile,
	                                &IoStatusBlock,
	                                FileRename,
	                                sizeof(FILE_RENAME_INFORMATION) + DstPathU.Length,
	                                FileRenameInformation);
	CloseHandle(hFile);
	if (NT_SUCCESS(errCode))
	{
		Result = TRUE;
	}
	else if (STATUS_NOT_SAME_DEVICE == errCode &&
		 MOVEFILE_COPY_ALLOWED == (dwFlags & MOVEFILE_COPY_ALLOWED))
	{
		Result = CopyFileExW (lpExistingFileName,
		                      lpNewFileName,
		                      lpProgressRoutine,
		                      lpData,
		                      NULL,
		                      FileRename->ReplaceIfExists ? 0 : COPY_FILE_FAIL_IF_EXISTS);
		if (Result)
		{
			/* Cleanup the source file */
			AdjustFileAttributes(lpExistingFileName, lpNewFileName);
	                Result = DeleteFileW (lpExistingFileName);
		}
	}
#if 1
	/* FIXME file rename not yet implemented in all FSDs so it will always
	 * fail, even when the move is to the same device
	 */
	else if (STATUS_NOT_IMPLEMENTED == errCode)
	{

		UNICODE_STRING SrcPathU;

		SrcPathU.Buffer = alloca(sizeof(WCHAR) * MAX_PATH);
		SrcPathU.MaximumLength = MAX_PATH * sizeof(WCHAR);
		SrcPathU.Length = GetFullPathNameW(lpExistingFileName, MAX_PATH, SrcPathU.Buffer, NULL);
		if (SrcPathU.Length >= MAX_PATH)
		{
		    SetLastError(ERROR_FILENAME_EXCED_RANGE);
		    return FALSE;
		}
		SrcPathU.Length *= sizeof(WCHAR);

		DstPathU.Buffer = alloca(sizeof(WCHAR) * MAX_PATH);
		DstPathU.MaximumLength = MAX_PATH * sizeof(WCHAR);
		DstPathU.Length = GetFullPathNameW(lpNewFileName, MAX_PATH, DstPathU.Buffer, NULL);
		if (DstPathU.Length >= MAX_PATH)
		{
		    SetLastError(ERROR_FILENAME_EXCED_RANGE);
		    return FALSE;
		}
		DstPathU.Length *= sizeof(WCHAR);

		if (0 == RtlCompareUnicodeString(&SrcPathU, &DstPathU, TRUE))
		{
		   /* Source and destination file are the same, nothing to do */
		   return TRUE;
		}

		Result = CopyFileExW (lpExistingFileName,
		                      lpNewFileName,
		                      lpProgressRoutine,
		                      lpData,
		                      NULL,
		                      FileRename->ReplaceIfExists ? 0 : COPY_FILE_FAIL_IF_EXISTS);
		if (Result)
		{
		    /* Cleanup the source file */
                    AdjustFileAttributes(lpExistingFileName, lpNewFileName);
		    Result = DeleteFileW (lpExistingFileName);
		}
	}
#endif
	else
	{
		SetLastErrorByStatus (errCode);
		Result = FALSE;
	}
	return Result;
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileWithProgressA (
	LPCSTR			lpExistingFileName,
	LPCSTR			lpNewFileName,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	DWORD			dwFlags
	)
{
	PWCHAR ExistingFileNameW;
   PWCHAR NewFileNameW;
	BOOL ret;

   if (!(ExistingFileNameW = FilenameA2W(lpExistingFileName, FALSE)))
      return FALSE;

   if (!(NewFileNameW= FilenameA2W(lpNewFileName, TRUE)))
      return FALSE;

   ret = MoveFileWithProgressW (ExistingFileNameW ,
                                   NewFileNameW,
	                                lpProgressRoutine,
	                                lpData,
	                                dwFlags);

   RtlFreeHeap (RtlGetProcessHeap (), 0, NewFileNameW);

	return ret;
}


/*
 * @implemented
 */
BOOL
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


/*
 * @implemented
 */
BOOL
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


/*
 * @implemented
 */
BOOL
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


/*
 * @implemented
 */
BOOL
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

/* EOF */
