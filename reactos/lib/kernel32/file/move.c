/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* FIXME: the large integer manipulations in this file dont handle overflow  */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <wchar.h>
#include <string.h>

//#define NDEBUG
#include <kernel32/kernel32.h>

/* FUNCTIONS ****************************************************************/

WINBOOL
STDCALL
MoveFileA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    )
{
	return MoveFileExA(lpExistingFileName,lpNewFileName,MOVEFILE_COPY_ALLOWED);
}

WINBOOL
STDCALL
MoveFileExA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    DWORD dwFlags
    )
{
	ULONG i;
	WCHAR ExistingFileNameW[MAX_PATH];
	WCHAR NewFileNameW[MAX_PATH];

	

    	i = 0;
   	while ((*lpExistingFileName)!=0 && i < MAX_PATH)
     	{
		ExistingFileNameW[i] = *lpExistingFileName;
		lpExistingFileName++;
		i++;
     	}
   	ExistingFileNameW[i] = 0;

	i = 0;
   	while ((*lpNewFileName)!=0 && i < MAX_PATH)
     	{
		NewFileNameW[i] = *lpNewFileName;
		lpNewFileName++;
		i++;
     	}
   	NewFileNameW[i] = 0;

	return MoveFileExW(ExistingFileNameW,NewFileNameW,dwFlags);
	
}



WINBOOL
STDCALL
MoveFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    )
{
	return MoveFileExW(lpExistingFileName,lpNewFileName,MOVEFILE_COPY_ALLOWED);
}

#define FILE_RENAME_SIZE  MAX_PATH +sizeof(FILE_RENAME_INFORMATION)

WINBOOL
STDCALL
MoveFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags
    )
{
	HANDLE hFile = NULL;
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_RENAME_INFORMATION *FileRename;
	USHORT Buffer[FILE_RENAME_SIZE];
	NTSTATUS errCode;	

	hFile = CreateFileW(
  		lpExistingFileName,	
    		GENERIC_ALL,	
    		FILE_SHARE_WRITE|FILE_SHARE_READ,	
    		NULL,	
    		OPEN_EXISTING,	
    		FILE_ATTRIBUTE_NORMAL,	
    		NULL 
   	);



	FileRename = (FILE_RENAME_INFORMATION *)Buffer;
	if ( ( dwFlags & MOVEFILE_REPLACE_EXISTING ) == MOVEFILE_REPLACE_EXISTING )
		FileRename->Replace = TRUE;
	else
		FileRename->Replace = FALSE;

	FileRename->FileNameLength = lstrlenW(lpNewFileName);
	memcpy(FileRename->FileName,lpNewFileName,min(FileRename->FileNameLength,MAX_PATH));
	
	errCode = NtSetInformationFile(hFile,&IoStatusBlock,FileRename, FILE_RENAME_SIZE, FileRenameInformation);
	if ( !NT_SUCCESS(errCode) ) {
		if ( CopyFileW(lpExistingFileName,lpNewFileName,FileRename->Replace) )
			DeleteFileW(lpExistingFileName);
	}

	CloseHandle(hFile);
	return TRUE;
}
