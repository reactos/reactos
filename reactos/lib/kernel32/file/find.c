/* $Id: find.c,v 1.36 2003/04/26 08:56:50 gvg Exp $
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

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* TYPES ********************************************************************/

#ifndef offsetof
#define	offsetof(TYPE, MEMBER)	((size_t) &( ((TYPE *) 0)->MEMBER ))
#endif

#define FIND_DATA_SIZE	(16*1024)

typedef struct _KERNEL32_FIND_FILE_DATA
{
   HANDLE DirectoryHandle;
   PFILE_BOTH_DIRECTORY_INFORMATION pFileInfo;
} KERNEL32_FIND_FILE_DATA, *PKERNEL32_FIND_FILE_DATA;


/* FUNCTIONS ****************************************************************/


WINBOOL
STDCALL
InternalFindNextFile (
	HANDLE	hFindFile
	)
{
	PKERNEL32_FIND_FILE_DATA IData;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;

	if (IData->pFileInfo->NextEntryOffset != 0)
	{
	   IData->pFileInfo = (PVOID)IData->pFileInfo + IData->pFileInfo->NextEntryOffset;
	   DPRINT("Found %.*S\n",IData->pFileInfo->FileNameLength, IData->pFileInfo->FileName);
	   return TRUE;
	}
	IData->pFileInfo = (PVOID)IData + sizeof(KERNEL32_FIND_FILE_DATA);
	IData->pFileInfo->FileIndex = 0;
	Status = NtQueryDirectoryFile (IData->DirectoryHandle,
	                               NULL,
	                               NULL,
	                               NULL,
	                               &IoStatusBlock,
	                               (PVOID)IData->pFileInfo,
	                               FIND_DATA_SIZE,
	                               FileBothDirectoryInformation,
	                               FALSE,
	                               NULL,
	                               FALSE);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return FALSE;
	}
	DPRINT("Found %.*S\n",IData->pFileInfo->FileNameLength, IData->pFileInfo->FileName);
	return TRUE;
}


HANDLE
STDCALL
InternalFindFirstFile (
	LPCWSTR	lpFileName
	)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	PKERNEL32_FIND_FILE_DATA IData;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING NtPathU;
	UNICODE_STRING PatternStr;
	NTSTATUS Status;
	PWSTR e1, e2;
	WCHAR CurrentDir[256];
	PWSTR SearchPath;
	PWCHAR SearchPattern;
	ULONG Length;
	BOOLEAN bResult;

	DPRINT("FindFirstFileW(lpFileName %S)\n",
	       lpFileName);

	e1 = wcsrchr(lpFileName, L'/');
	e2 = wcsrchr(lpFileName, L'\\');
	SearchPattern = max(e1, e2);
	SearchPath = CurrentDir;

	if (NULL == SearchPattern)
	{
	   CHECKPOINT;
	   SearchPattern = (PWCHAR)lpFileName;
           Length = GetCurrentDirectoryW(sizeof(CurrentDir) / sizeof(WCHAR), SearchPath);
	   if (0 == Length)
	   {
	      return NULL;
	   }
	   if (Length > sizeof(CurrentDir) / sizeof(WCHAR))
	   {
	      SearchPath = RtlAllocateHeap(hProcessHeap,
	                                   HEAP_ZERO_MEMORY,
					   Length * sizeof(WCHAR));
	      if (NULL == SearchPath)
	      {
	         SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	         return NULL;
	      }
	      GetCurrentDirectoryW(Length, SearchPath);
	   }
	}
	else
	{
	   CHECKPOINT;
	   SearchPattern++;
	   Length = SearchPattern - lpFileName;
	   if (Length + 1 > sizeof(CurrentDir) / sizeof(WCHAR))
	   {
              SearchPath = RtlAllocateHeap(hProcessHeap,
	                                   HEAP_ZERO_MEMORY,
					   (Length + 1) * sizeof(WCHAR));
	      if (NULL == SearchPath)
	      {
	         SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	         return NULL;
	      }
	   }
           memcpy(SearchPath, lpFileName, Length * sizeof(WCHAR));
	   SearchPath[Length] = 0;
	}

	bResult = RtlDosPathNameToNtPathName_U ((LPWSTR)SearchPath,
	                                        &NtPathU,
	                                        NULL,
	                                        NULL);
        if (SearchPath != CurrentDir)
	{
	   RtlFreeHeap(hProcessHeap,
	               0,
		       SearchPath);
	}
	if (FALSE == bResult)
	{
	   return NULL;
	}

	DPRINT("NtPathU \'%S\'\n", NtPathU.Buffer);

	IData = RtlAllocateHeap (hProcessHeap,
	                         HEAP_ZERO_MEMORY,
	                         sizeof(KERNEL32_FIND_FILE_DATA) + FIND_DATA_SIZE);
	if (NULL == IData)
	{
	   RtlFreeHeap (hProcessHeap,
	                0,
	                NtPathU.Buffer);
	   SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	   return NULL;
	}

	/* change pattern: "*.*" --> "*" */
	if (!wcscmp (SearchPattern, L"*.*"))
	{
	    RtlInitUnicodeStringFromLiteral(&PatternStr, L"*");
	}
	else
	{
	    RtlInitUnicodeString(&PatternStr, SearchPattern);
	}

	DPRINT("NtPathU \'%S\' Pattern \'%S\'\n",
	       NtPathU.Buffer, PatternStr.Buffer);

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

	RtlFreeHeap (hProcessHeap,
	             0,
	             NtPathU.Buffer);

	if (!NT_SUCCESS(Status))
	{
		RtlFreeHeap (hProcessHeap, 0, IData);
		SetLastErrorByStatus (Status);
		return(NULL);
	}
	IData->pFileInfo = (PVOID)IData + sizeof(KERNEL32_FIND_FILE_DATA);
	IData->pFileInfo->FileIndex = 0;

	Status = NtQueryDirectoryFile (IData->DirectoryHandle,
	                               NULL,
	                               NULL,
	                               NULL,
	                               &IoStatusBlock,
	                               (PVOID)IData->pFileInfo,
	                               FIND_DATA_SIZE,
	                               FileBothDirectoryInformation,
	                               TRUE,
	                               &PatternStr,
	                               TRUE);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Status %lx\n", Status);
		RtlFreeHeap (hProcessHeap, 0, IData);
		SetLastErrorByStatus (Status);
		return NULL;
	}
	DPRINT("Found %.*S\n",IData->pFileInfo->FileNameLength, IData->pFileInfo->FileName);

	return IData;
}


HANDLE
STDCALL
FindFirstFileA (
	LPCTSTR			lpFileName,
	LPWIN32_FIND_DATAA	lpFindFileData
	)
{
	PKERNEL32_FIND_FILE_DATA IData;
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

	IData = InternalFindFirstFile (FileNameU.Buffer);

	RtlFreeUnicodeString (&FileNameU);

	if (IData == NULL)
	{
		DPRINT("Failing request\n");
		return INVALID_HANDLE_VALUE;
	}

	DPRINT("IData->pFileInfo->FileNameLength %d\n",
	       IData->pFileInfo->FileNameLength);

	/* copy data into WIN32_FIND_DATA structure */
	lpFindFileData->dwFileAttributes = IData->pFileInfo->FileAttributes;
	memcpy (&lpFindFileData->ftCreationTime,
	        &IData->pFileInfo->CreationTime,
	        sizeof(FILETIME));
	memcpy (&lpFindFileData->ftLastAccessTime,
	        &IData->pFileInfo->LastAccessTime,
	        sizeof(FILETIME));
	memcpy (&lpFindFileData->ftLastWriteTime,
	        &IData->pFileInfo->LastWriteTime,
	        sizeof(FILETIME));
	lpFindFileData->nFileSizeHigh = IData->pFileInfo->EndOfFile.u.HighPart;
	lpFindFileData->nFileSizeLow = IData->pFileInfo->EndOfFile.u.LowPart;

	FileNameU.Length = IData->pFileInfo->FileNameLength;
	FileNameU.MaximumLength = FileNameU.Length + sizeof(WCHAR);
	FileNameU.Buffer = IData->pFileInfo->FileName;

	FileName.Length = 0;
	FileName.MaximumLength = MAX_PATH;
	FileName.Buffer = lpFindFileData->cFileName;

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
		RtlUnicodeStringToAnsiString (&FileName,
		                              &FileNameU,
		                              FALSE);
	else
		RtlUnicodeStringToOemString (&FileName,
		                             &FileNameU,
		                             FALSE);

	DPRINT("IData->pFileInfo->ShortNameLength %d\n",
	       IData->pFileInfo->ShortNameLength);

	FileNameU.Length = IData->pFileInfo->ShortNameLength;
	FileNameU.MaximumLength = FileNameU.Length + sizeof(WCHAR);
	FileNameU.Buffer = IData->pFileInfo->ShortName;

	FileName.Length = 0;
	FileName.MaximumLength = 14;
	FileName.Buffer = lpFindFileData->cAlternateFileName;

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
		RtlUnicodeStringToAnsiString (&FileName,
		                              &FileNameU,
		                              FALSE);
	else
		RtlUnicodeStringToOemString (&FileName,
		                             &FileNameU,
		                             FALSE);

	return (HANDLE)IData;
}


WINBOOL
STDCALL
FindNextFileA (
	HANDLE hFindFile,
	LPWIN32_FIND_DATAA lpFindFileData)
{
	PKERNEL32_FIND_FILE_DATA IData;
	UNICODE_STRING FileNameU;
	ANSI_STRING FileName;

	IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;
	if (IData == NULL)
	{
		return FALSE;
	}

	if (!InternalFindNextFile (hFindFile))
	{
		DPRINT("InternalFindNextFile() failed\n");
		return FALSE;
	}

	DPRINT("IData->pFileInfo->FileNameLength %d\n",
	       IData->pFileInfo->FileNameLength);

	/* copy data into WIN32_FIND_DATA structure */
	lpFindFileData->dwFileAttributes = IData->pFileInfo->FileAttributes;
	memcpy (&lpFindFileData->ftCreationTime,
	        &IData->pFileInfo->CreationTime,
	        sizeof(FILETIME));
	memcpy (&lpFindFileData->ftLastAccessTime,
	        &IData->pFileInfo->LastAccessTime,
	        sizeof(FILETIME));
	memcpy (&lpFindFileData->ftLastWriteTime,
	        &IData->pFileInfo->LastWriteTime,
	        sizeof(FILETIME));
	lpFindFileData->nFileSizeHigh = IData->pFileInfo->EndOfFile.u.HighPart;
	lpFindFileData->nFileSizeLow = IData->pFileInfo->EndOfFile.u.LowPart;

	FileNameU.Length = IData->pFileInfo->FileNameLength;
	FileNameU.MaximumLength = FileNameU.Length + sizeof(WCHAR);
	FileNameU.Buffer = IData->pFileInfo->FileName;

	FileName.Length = 0;
	FileName.MaximumLength = MAX_PATH;
	FileName.Buffer = lpFindFileData->cFileName;

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
		RtlUnicodeStringToAnsiString (&FileName,
		                              &FileNameU,
		                              FALSE);
	else
		RtlUnicodeStringToOemString (&FileName,
		                             &FileNameU,
		                             FALSE);

	DPRINT("IData->pFileInfo->ShortNameLength %d\n",
	       IData->pFileInfo->ShortNameLength);

	FileNameU.Length = IData->pFileInfo->ShortNameLength;
	FileNameU.MaximumLength = FileNameU.Length + sizeof(WCHAR);
	FileNameU.Buffer = IData->pFileInfo->ShortName;

	FileName.Length = 0;
	FileName.MaximumLength = 14;
	FileName.Buffer = lpFindFileData->cAlternateFileName;

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

	if (!hFindFile || hFindFile == INVALID_HANDLE_VALUE)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		return FALSE;
	}

	IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;

	CloseHandle (IData->DirectoryHandle);
	RtlFreeHeap (hProcessHeap, 0, IData);

	return TRUE;
}


HANDLE
STDCALL
FindFirstFileW (
	LPCWSTR			lpFileName,
	LPWIN32_FIND_DATAW	lpFindFileData
	)
{
	PKERNEL32_FIND_FILE_DATA IData;

	IData = InternalFindFirstFile (lpFileName);
	if (IData == NULL)
	{
		DPRINT("Failing request\n");
		return INVALID_HANDLE_VALUE;
	}

	/* copy data into WIN32_FIND_DATA structure */
	lpFindFileData->dwFileAttributes = IData->pFileInfo->FileAttributes;
	memcpy (&lpFindFileData->ftCreationTime,
	        &IData->pFileInfo->CreationTime,
	        sizeof(FILETIME));
	memcpy (&lpFindFileData->ftLastAccessTime,
	        &IData->pFileInfo->LastAccessTime,
	        sizeof(FILETIME));
	memcpy (&lpFindFileData->ftLastWriteTime,
	        &IData->pFileInfo->LastWriteTime,
	        sizeof(FILETIME));
	lpFindFileData->nFileSizeHigh = IData->pFileInfo->EndOfFile.u.HighPart;
	lpFindFileData->nFileSizeLow = IData->pFileInfo->EndOfFile.u.LowPart;
	memcpy (lpFindFileData->cFileName,
	        IData->pFileInfo->FileName,
	        IData->pFileInfo->FileNameLength);
	lpFindFileData->cFileName[IData->pFileInfo->FileNameLength / sizeof(WCHAR)] = 0;
	memcpy (lpFindFileData->cAlternateFileName,
	        IData->pFileInfo->ShortName,
	        IData->pFileInfo->ShortNameLength);
	lpFindFileData->cAlternateFileName[IData->pFileInfo->ShortNameLength / sizeof(WCHAR)] = 0;
	return IData;
}


WINBOOL
STDCALL
FindNextFileW (
	HANDLE			hFindFile,
	LPWIN32_FIND_DATAW	lpFindFileData
	)
{
	PKERNEL32_FIND_FILE_DATA IData;

	IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;
	if (!InternalFindNextFile(hFindFile))
	{
		DPRINT("Failing request\n");
		return FALSE;
	}

	/* copy data into WIN32_FIND_DATA structure */
	lpFindFileData->dwFileAttributes = IData->pFileInfo->FileAttributes;
	memcpy (&lpFindFileData->ftCreationTime,
	        &IData->pFileInfo->CreationTime,
	        sizeof(FILETIME));
	memcpy (&lpFindFileData->ftLastAccessTime,
	        &IData->pFileInfo->LastAccessTime,
	        sizeof(FILETIME));
	memcpy (&lpFindFileData->ftLastWriteTime,
	        &IData->pFileInfo->LastWriteTime,
	        sizeof(FILETIME));
	lpFindFileData->nFileSizeHigh = IData->pFileInfo->EndOfFile.u.HighPart;
	lpFindFileData->nFileSizeLow = IData->pFileInfo->EndOfFile.u.LowPart;
	memcpy (lpFindFileData->cFileName,
	        IData->pFileInfo->FileName,
	        IData->pFileInfo->FileNameLength);
	lpFindFileData->cFileName[IData->pFileInfo->FileNameLength / sizeof(WCHAR)] = 0;
	memcpy (lpFindFileData->cAlternateFileName,
	        IData->pFileInfo->ShortName,
	        IData->pFileInfo->ShortNameLength);
	lpFindFileData->cAlternateFileName[IData->pFileInfo->ShortNameLength / sizeof(WCHAR)] = 0;
	return TRUE;
}


HANDLE
STDCALL
FindFirstFileExW (
	LPCWSTR			lpFileName,
	FINDEX_INFO_LEVELS	fInfoLevelId,
	LPVOID			lpFindFileData,
	FINDEX_SEARCH_OPS	fSearchOp,
	LPVOID			lpSearchFilter,
	DWORD			dwAdditionalFlags
	)
{
	/* FIXME */
	return (HANDLE) 0;
}


HANDLE
STDCALL
FindFirstFileExA (
	LPCSTR			lpFileName,
	FINDEX_INFO_LEVELS	fInfoLevelId,
	LPVOID			lpFindFileData,
	FINDEX_SEARCH_OPS	fSearchOp,
	LPVOID			lpSearchFilter,
	DWORD			dwAdditionalFlags
	)
{
	/* FIXME */
	return (HANDLE) 0;
}


/* EOF */
