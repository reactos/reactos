/* $Id: find.c,v 1.24 2000/03/15 12:25:47 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/find.c
 * PURPOSE:         Find functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>
#include <wchar.h>
#include <string.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* TYPES ********************************************************************/

typedef struct _KERNEL32_FIND_FILE_DATA
{
   HANDLE DirectoryHandle;
   FILE_BOTH_DIRECTORY_INFORMATION FileInfo;
   WCHAR FileNameExtra[MAX_PATH];
   UNICODE_STRING PatternStr;
} KERNEL32_FIND_FILE_DATA, *PKERNEL32_FIND_FILE_DATA;

typedef struct _WIN32_FIND_DATA_UNICODE {
  DWORD dwFileAttributes; 
  FILETIME ftCreationTime; 
  FILETIME ftLastAccessTime; 
  FILETIME ftLastWriteTime; 
  DWORD    nFileSizeHigh; 
  DWORD    nFileSizeLow; 
  DWORD    dwReserved0; 
  DWORD    dwReserved1; 
  WCHAR    cFileName[ MAX_PATH ]; 
  WCHAR    cAlternateFileName[ 14 ]; 
} WIN32_FIND_DATA_UNICODE, *PWIN32_FIND_DATA_UNICODE; 

typedef struct _WIN32_FIND_DATA_ASCII { 
  DWORD dwFileAttributes; 
  FILETIME ftCreationTime; 
  FILETIME ftLastAccessTime; 
  FILETIME ftLastWriteTime; 
  DWORD    nFileSizeHigh; 
  DWORD    nFileSizeLow; 
  DWORD    dwReserved0; 
  DWORD    dwReserved1; 
  CHAR    cFileName[ MAX_PATH ]; 
  CHAR    cAlternateFileName[ 14 ]; 
} WIN32_FIND_DATA_ASCII, *PWIN32_FIND_DATA_ASCII; 

/* FUNCTIONS *****************************************************************/


static void
FileDataToWin32Data (
	LPWIN32_FIND_DATA		lpFindFileData,
	PKERNEL32_FIND_FILE_DATA	IData
	)
{
	lpFindFileData->dwFileAttributes = IData->FileInfo.FileAttributes;
	memcpy (&lpFindFileData->ftCreationTime,
	        &IData->FileInfo.CreationTime,
	        sizeof(FILETIME));
	memcpy (&lpFindFileData->ftLastAccessTime,
	        &IData->FileInfo.LastAccessTime,
	        sizeof(FILETIME));
	memcpy (&lpFindFileData->ftLastWriteTime,
	        &IData->FileInfo.LastWriteTime,
	        sizeof(FILETIME));
	lpFindFileData->nFileSizeHigh = IData->FileInfo.EndOfFile.u.HighPart;
	lpFindFileData->nFileSizeLow = IData->FileInfo.EndOfFile.u.LowPart;
}


WINBOOL
STDCALL
InternalFindNextFile (
	HANDLE			hFindFile,
	LPWIN32_FIND_DATA	lpFindFileData
	)
{
	PKERNEL32_FIND_FILE_DATA IData;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;

	Status = NtQueryDirectoryFile (IData->DirectoryHandle,
	                               NULL,
	                               NULL,
	                               NULL,
	                               &IoStatusBlock,
	                               (PVOID)&IData->FileInfo,
	                               sizeof(IData->FileInfo) +
	                               sizeof(IData->FileNameExtra),
	                               FileBothDirectoryInformation,
	                               TRUE,
	                               &(IData->PatternStr),
	                               FALSE);
	DPRINT("Found %S\n",IData->FileInfo.FileName);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	FileDataToWin32Data (lpFindFileData, IData);

	return TRUE;
}


HANDLE
STDCALL
InternalFindFirstFile (
	LPCWSTR			lpFileName,
	LPWIN32_FIND_DATA	lpFindFileData
	)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	PKERNEL32_FIND_FILE_DATA IData;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING NtPathU;
	NTSTATUS Status;
	PWSTR End;

	DPRINT("FindFirstFileW(lpFileName %S, lpFindFileData %x)\n",
	       lpFileName, lpFindFileData);

	if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFileName,
	                                   &NtPathU,
	                                   &End,
	                                   NULL))
		return FALSE;

	DPRINT("NtPathU \'%S\' End \'%S\'\n", NtPathU.Buffer, End);

	IData = RtlAllocateHeap (RtlGetProcessHeap (),
	                         HEAP_ZERO_MEMORY,
	                         sizeof(KERNEL32_FIND_FILE_DATA));

	/* move seach pattern to separate string */
	RtlCreateUnicodeString (&IData->PatternStr,
	                        End);
	*End = 0;
	NtPathU.Length = wcslen(NtPathU.Buffer)*sizeof(WCHAR);

	/* change pattern: "*.*" --> "*" */
	if (!wcscmp (IData->PatternStr.Buffer, L"*.*"))
	{
		IData->PatternStr.Buffer[1] = 0;
		IData->PatternStr.Length = sizeof(WCHAR);
	}

	DPRINT("NtPathU \'%S\' Pattern \'%S\'\n",
	       NtPathU.Buffer, IData->PatternStr.Buffer);

	InitializeObjectAttributes (&ObjectAttributes,
	                            &NtPathU,
	                            0,
	                            NULL,
	                            NULL);

	Status = NtOpenFile (&IData->DirectoryHandle,
	                     FILE_LIST_DIRECTORY,
	                     &ObjectAttributes,
	                     &IoStatusBlock,
	                     FILE_OPEN_IF,
	                     OPEN_EXISTING);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             NtPathU.Buffer);

	if (!NT_SUCCESS(Status))
	{
		RtlFreeHeap (RtlGetProcessHeap (), 0, IData->PatternStr.Buffer);
		RtlFreeHeap (RtlGetProcessHeap (), 0, IData);
		SetLastError (RtlNtStatusToDosError (Status));
		return(NULL);
	}

	Status = NtQueryDirectoryFile (IData->DirectoryHandle,
	                               NULL,
	                               NULL,
	                               NULL,
	                               &IoStatusBlock,
	                               (PVOID)&IData->FileInfo,
	                               sizeof(IData->FileInfo) +
	                               sizeof(IData->FileNameExtra),
	                               FileBothDirectoryInformation,
	                               TRUE,
	                               &(IData->PatternStr),
	                               FALSE);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Status %lx\n", Status);
		RtlFreeHeap (RtlGetProcessHeap (), 0, IData->PatternStr.Buffer);
		RtlFreeHeap (RtlGetProcessHeap (), 0, IData);
		SetLastError (RtlNtStatusToDosError (Status));
		return NULL;
	}
	DPRINT("Found %S\n",IData->FileInfo.FileName);

	FileDataToWin32Data(lpFindFileData, IData);

	return IData;
}


HANDLE
STDCALL
FindFirstFileA (
	LPCTSTR			lpFileName,
	LPWIN32_FIND_DATA	lpFindFileData
	)
{
	PKERNEL32_FIND_FILE_DATA IData;
	PWIN32_FIND_DATA_ASCII Ret;
	UNICODE_STRING FileNameU;
	ANSI_STRING FileName;

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

	IData = InternalFindFirstFile (FileNameU.Buffer, lpFindFileData);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             FileNameU.Buffer);

	if (IData == NULL)
	{
		DPRINT("Failing request\n");
		return INVALID_HANDLE_VALUE;
	}

	Ret = (PWIN32_FIND_DATA_ASCII)lpFindFileData;

	DPRINT("IData->FileInfo.FileNameLength %d\n",
	       IData->FileInfo.FileNameLength);

	FileNameU.Length = IData->FileInfo.FileNameLength * sizeof(WCHAR);
	FileNameU.MaximumLength = FileNameU.Length + sizeof(WCHAR);
	FileNameU.Buffer = IData->FileInfo.FileName;

	FileName.Length = 0;
	FileName.MaximumLength = MAX_PATH;
	FileName.Buffer = Ret->cFileName;

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
		RtlUnicodeStringToAnsiString (&FileName,
		                              &FileNameU,
		                              FALSE);
	else
		RtlUnicodeStringToOemString (&FileName,
		                             &FileNameU,
		                             FALSE);

	DPRINT("IData->FileInfo.ShortNameLength %d\n",
	       IData->FileInfo.ShortNameLength);

	FileNameU.Length = IData->FileInfo.ShortNameLength * sizeof(WCHAR);
	FileNameU.MaximumLength = FileNameU.Length + sizeof(WCHAR);
	FileNameU.Buffer = IData->FileInfo.FileName;

	FileName.Length = 0;
	FileName.MaximumLength = 14;
	FileName.Buffer = Ret->cAlternateFileName;

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
		RtlUnicodeStringToAnsiString (&FileName,
		                              &FileNameU,
		                              FALSE);
	else
		RtlUnicodeStringToOemString (&FileName,
		                             &FileNameU,
		                             FALSE);

	return IData;
}


WINBOOL
STDCALL
FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
	PWIN32_FIND_DATA_ASCII Ret;
	PKERNEL32_FIND_FILE_DATA IData;
	UNICODE_STRING FileNameU;
	ANSI_STRING FileName;

	IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;
	if (IData == NULL)
	{
		return FALSE;
	}

	if (!InternalFindNextFile (hFindFile, lpFindFileData))
	{
		DPRINT("InternalFindNextFile() failed\n");
		return FALSE;
	}

	Ret = (PWIN32_FIND_DATA_ASCII)lpFindFileData;

	DPRINT("IData->FileInfo.FileNameLength %d\n",
	       IData->FileInfo.FileNameLength);

	FileNameU.Length = IData->FileInfo.FileNameLength * sizeof(WCHAR);
	FileNameU.MaximumLength = FileNameU.Length + sizeof(WCHAR);
	FileNameU.Buffer = IData->FileInfo.FileName;

	FileName.Length = 0;
	FileName.MaximumLength = MAX_PATH;
	FileName.Buffer = Ret->cFileName;

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
		RtlUnicodeStringToAnsiString (&FileName,
		                              &FileNameU,
		                              FALSE);
	else
		RtlUnicodeStringToOemString (&FileName,
		                             &FileNameU,
		                             FALSE);

	DPRINT("IData->FileInfo.ShortNameLength %d\n",
	       IData->FileInfo.ShortNameLength);

	FileNameU.Length = IData->FileInfo.ShortNameLength * sizeof(WCHAR);
	FileNameU.MaximumLength = FileNameU.Length + sizeof(WCHAR);
	FileNameU.Buffer = IData->FileInfo.FileName;

	FileName.Length = 0;
	FileName.MaximumLength = 14;
	FileName.Buffer = Ret->cAlternateFileName;

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
		RtlUnicodeStringToAnsiString (&FileName,
		                              &FileNameU,
		                              FALSE);
	else
		RtlUnicodeStringToOemString (&FileName,
		                             &FileNameU,
		                             FALSE);

	return TRUE;
}


BOOL
STDCALL
FindClose (
	HANDLE	hFindFile
	)
{
	PKERNEL32_FIND_FILE_DATA IData;

	DPRINT("FindClose(hFindFile %x)\n",hFindFile);

	if (hFindFile || hFindFile == INVALID_HANDLE_VALUE)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		return FALSE;
	}

	IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;

	CloseHandle (IData->DirectoryHandle);
	if (IData->PatternStr.Buffer)
		RtlFreeHeap (RtlGetProcessHeap (), 0, IData->PatternStr.Buffer);
	RtlFreeHeap (RtlGetProcessHeap (), 0, IData);

	return TRUE;
}


HANDLE
STDCALL
FindFirstFileW (
	LPCWSTR			lpFileName,
	LPWIN32_FIND_DATA	lpFindFileData
	)
{
	PWIN32_FIND_DATA_UNICODE Ret;
	PKERNEL32_FIND_FILE_DATA IData;

	IData = InternalFindFirstFile (lpFileName, lpFindFileData);
	if (IData == NULL)
	{
		DPRINT("Failing request\n");
		return INVALID_HANDLE_VALUE;
	}

	Ret = (PWIN32_FIND_DATA_UNICODE)lpFindFileData;

	memcpy (Ret->cFileName,
	        IData->FileInfo.FileName,
	        IData->FileInfo.FileNameLength);
	memcpy (Ret->cAlternateFileName,
	        IData->FileInfo.ShortName,
	        IData->FileInfo.ShortNameLength);

	return IData;
}


WINBOOL
STDCALL
FindNextFileW (
	HANDLE			hFindFile,
	LPWIN32_FIND_DATA	lpFindFileData
	)
{
	PWIN32_FIND_DATA_UNICODE Ret;
	PKERNEL32_FIND_FILE_DATA IData;

	IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;
	if (!InternalFindNextFile(hFindFile, lpFindFileData))
	{
		DPRINT("Failing request\n");
		return FALSE;
	}

	Ret = (PWIN32_FIND_DATA_UNICODE)lpFindFileData;

	memcpy (Ret->cFileName,
	        IData->FileInfo.FileName,
	        IData->FileInfo.FileNameLength);

	memcpy (Ret->cAlternateFileName,
	        IData->FileInfo.ShortName,
	        IData->FileInfo.ShortNameLength);

	return TRUE;
}

/* EOF */
