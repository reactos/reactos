/* $Id$
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
#include "../include/debug.h"


/* TYPES ********************************************************************/

#ifndef offsetof
#define	offsetof(TYPE, MEMBER)	((size_t) &( ((TYPE *) 0)->MEMBER ))
#endif

#define FIND_DATA_SIZE	(16*1024)

typedef struct _KERNEL32_FIND_FILE_DATA
{
   HANDLE DirectoryHandle;
   BOOLEAN DirectoryOnly;
   PFILE_BOTH_DIR_INFORMATION pFileInfo;
} KERNEL32_FIND_FILE_DATA, *PKERNEL32_FIND_FILE_DATA;


/* FUNCTIONS ****************************************************************/

VOID
InternalCopyFindDataW(LPWIN32_FIND_DATAW            lpFindFileData,
                      PFILE_BOTH_DIR_INFORMATION    lpFileInfo)
{
    lpFindFileData->dwFileAttributes = lpFileInfo->FileAttributes;

    lpFindFileData->ftCreationTime.dwHighDateTime = lpFileInfo->CreationTime.u.HighPart;
    lpFindFileData->ftCreationTime.dwLowDateTime = lpFileInfo->CreationTime.u.LowPart;

    lpFindFileData->ftLastAccessTime.dwHighDateTime = lpFileInfo->LastAccessTime.u.HighPart;
    lpFindFileData->ftLastAccessTime.dwLowDateTime = lpFileInfo->LastAccessTime.u.LowPart;

    lpFindFileData->ftLastWriteTime.dwHighDateTime = lpFileInfo->LastWriteTime.u.HighPart;
    lpFindFileData->ftLastWriteTime.dwLowDateTime = lpFileInfo->LastWriteTime.u.LowPart;

    lpFindFileData->nFileSizeHigh = lpFileInfo->EndOfFile.u.HighPart;
    lpFindFileData->nFileSizeLow = lpFileInfo->EndOfFile.u.LowPart;

    memcpy (lpFindFileData->cFileName, lpFileInfo->FileName, lpFileInfo->FileNameLength);
    lpFindFileData->cFileName[lpFileInfo->FileNameLength / sizeof(WCHAR)] = 0;

    memcpy (lpFindFileData->cAlternateFileName, lpFileInfo->ShortName, lpFileInfo->ShortNameLength);
    lpFindFileData->cAlternateFileName[lpFileInfo->ShortNameLength / sizeof(WCHAR)] = 0;
}

VOID
InternalCopyFindDataA(LPWIN32_FIND_DATAA            lpFindFileData,
                      PFILE_BOTH_DIR_INFORMATION    lpFileInfo)
{
    UNICODE_STRING FileNameU;
    ANSI_STRING FileNameA;

    lpFindFileData->dwFileAttributes = lpFileInfo->FileAttributes;

    lpFindFileData->ftCreationTime.dwHighDateTime = lpFileInfo->CreationTime.u.HighPart;
    lpFindFileData->ftCreationTime.dwLowDateTime = lpFileInfo->CreationTime.u.LowPart;

    lpFindFileData->ftLastAccessTime.dwHighDateTime = lpFileInfo->LastAccessTime.u.HighPart;
    lpFindFileData->ftLastAccessTime.dwLowDateTime = lpFileInfo->LastAccessTime.u.LowPart;

    lpFindFileData->ftLastWriteTime.dwHighDateTime = lpFileInfo->LastWriteTime.u.HighPart;
    lpFindFileData->ftLastWriteTime.dwLowDateTime = lpFileInfo->LastWriteTime.u.LowPart;

    lpFindFileData->nFileSizeHigh = lpFileInfo->EndOfFile.u.HighPart;
    lpFindFileData->nFileSizeLow = lpFileInfo->EndOfFile.u.LowPart;

    FileNameU.Length = FileNameU.MaximumLength = lpFileInfo->FileNameLength;
    FileNameU.Buffer = lpFileInfo->FileName;

    FileNameA.MaximumLength = sizeof(lpFindFileData->cFileName) - sizeof(CHAR);
    FileNameA.Buffer = lpFindFileData->cFileName;

    /* convert unicode string to ansi (or oem) */
    if (bIsFileApiAnsi)
    	RtlUnicodeStringToAnsiString (&FileNameA, &FileNameU, FALSE);
    else
        RtlUnicodeStringToOemString (&FileNameA, &FileNameU, FALSE);

    FileNameA.Buffer[FileNameA.Length] = 0;

    DPRINT("lpFileInfo->ShortNameLength %d\n", lpFileInfo->ShortNameLength);

    FileNameU.Length = FileNameU.MaximumLength = lpFileInfo->ShortNameLength;
    FileNameU.Buffer = lpFileInfo->ShortName;

    FileNameA.MaximumLength = sizeof(lpFindFileData->cAlternateFileName) - sizeof(CHAR);
    FileNameA.Buffer = lpFindFileData->cAlternateFileName;

    /* convert unicode string to ansi (or oem) */
    if (bIsFileApiAnsi)
	RtlUnicodeStringToAnsiString (&FileNameA, &FileNameU, FALSE);
    else
    	RtlUnicodeStringToOemString (&FileNameA, &FileNameU, FALSE);

    FileNameA.Buffer[FileNameA.Length] = 0;
}

/*
 * @implemented
 */
BOOL
STDCALL
InternalFindNextFile (
	HANDLE	hFindFile,
        PUNICODE_STRING SearchPattern
	)
{
	PKERNEL32_FIND_FILE_DATA IData;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	DPRINT("InternalFindNextFile(%lx)\n", hFindFile);

	IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;

        while (1)
        {
            if (IData->pFileInfo->NextEntryOffset != 0)
	    {
	        IData->pFileInfo = (PVOID)((ULONG_PTR)IData->pFileInfo + IData->pFileInfo->NextEntryOffset);
            }
            else
            {
                IData->pFileInfo = (PVOID)((ULONG_PTR)IData + sizeof(KERNEL32_FIND_FILE_DATA));
	        IData->pFileInfo->FileIndex = 0;
	        Status = NtQueryDirectoryFile (IData->DirectoryHandle,
	                                       NULL,
                                               NULL,
                                               NULL,
                                               &IoStatusBlock,
                                               (PVOID)IData->pFileInfo,
                                               FIND_DATA_SIZE,
                                               FileBothDirectoryInformation,
                                               SearchPattern ? TRUE : FALSE,
                                               SearchPattern,
                                               SearchPattern ? TRUE : FALSE);
                SearchPattern = NULL;
                if (!NT_SUCCESS(Status))
                {
                    SetLastErrorByStatus (Status);
                    return FALSE;
	        }
            }
            if (!IData->DirectoryOnly || IData->pFileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
	        DPRINT("Found %.*S\n",IData->pFileInfo->FileNameLength/sizeof(WCHAR), IData->pFileInfo->FileName);
	        return TRUE;
            }
        }
}


/*
 * @implemented
 */
HANDLE
STDCALL
InternalFindFirstFile (
	LPCWSTR	lpFileName,
        BOOLEAN DirectoryOnly
	)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	PKERNEL32_FIND_FILE_DATA IData;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING NtPathU;
	UNICODE_STRING PatternStr = RTL_CONSTANT_STRING(L"*");
	NTSTATUS Status;
	PWSTR e1, e2;
	WCHAR CurrentDir[256];
	PWCHAR SlashlessFileName;
	PWSTR SearchPath;
	PWCHAR SearchPattern;
	ULONG Length;
	BOOLEAN bResult;

	DPRINT("FindFirstFileW(lpFileName %S)\n",
	       lpFileName);

	Length = wcslen(lpFileName);
	if (L'\\' == lpFileName[Length - 1])
	{
	    SlashlessFileName = RtlAllocateHeap(hProcessHeap,
	                                        0,
					        Length * sizeof(WCHAR));
	    if (NULL == SlashlessFileName)
	    {
	        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	        return NULL;
	    }
	    memcpy(SlashlessFileName, lpFileName, (Length - 1) * sizeof(WCHAR));
	    SlashlessFileName[Length - 1] = L'\0';
	    lpFileName = SlashlessFileName;
	}
	else
	{
	    SlashlessFileName = NULL;
	}

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
	      if (NULL != SlashlessFileName)
	      {
	         RtlFreeHeap(hProcessHeap,
	                     0,
	                     SlashlessFileName);
	      }
	      return NULL;
	   }
	   if (Length > sizeof(CurrentDir) / sizeof(WCHAR))
	   {
	      SearchPath = RtlAllocateHeap(hProcessHeap,
	                                   HEAP_ZERO_MEMORY,
					   Length * sizeof(WCHAR));
	      if (NULL == SearchPath)
	      {
	         if (NULL != SlashlessFileName)
	         {
	            RtlFreeHeap(hProcessHeap,
	                        0,
	                        SlashlessFileName);
	         }
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
	         if (NULL != SlashlessFileName)
	         {
	            RtlFreeHeap(hProcessHeap,
	                        0,
	                        SlashlessFileName);
	         }
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
	   if (NULL != SlashlessFileName)
	   {
	      RtlFreeHeap(hProcessHeap,
	                  0,
	                  SlashlessFileName);
	   }
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
	   if (NULL != SlashlessFileName)
	   {
	      RtlFreeHeap(hProcessHeap,
	                  0,
	                  SlashlessFileName);
	   }
	   SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	   return NULL;
	}

	/* change pattern: "*.*" --> "*" */
	if (wcscmp (SearchPattern, L"*.*"))
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
	                     FILE_SHARE_READ|FILE_SHARE_WRITE,
	                     FILE_DIRECTORY_FILE);

	RtlFreeHeap (hProcessHeap,
	             0,
	             NtPathU.Buffer);

	if (!NT_SUCCESS(Status))
	{
	   RtlFreeHeap (hProcessHeap, 0, IData);
	   if (NULL != SlashlessFileName)
	   {
	      RtlFreeHeap(hProcessHeap,
	                  0,
	                  SlashlessFileName);
	   }
	   SetLastErrorByStatus (Status);
	   return(NULL);
	}
	IData->pFileInfo = (PVOID)((ULONG_PTR)IData + sizeof(KERNEL32_FIND_FILE_DATA));
	IData->pFileInfo->FileIndex = 0;
        IData->DirectoryOnly = DirectoryOnly;

        bResult = InternalFindNextFile((HANDLE)IData, &PatternStr);
	if (NULL != SlashlessFileName)
	{
	   RtlFreeHeap(hProcessHeap,
	               0,
	               SlashlessFileName);
	}

        if (!bResult)
        {
            FindClose((HANDLE)IData);
            return NULL;
        }

	return IData;
}


/*
 * @implemented
 */
HANDLE
STDCALL
FindFirstFileA (
	LPCSTR			lpFileName,
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

	IData = InternalFindFirstFile (FileNameU.Buffer, FALSE);

	RtlFreeUnicodeString (&FileNameU);

	if (IData == NULL)
	{
		DPRINT("Failing request\n");
		return INVALID_HANDLE_VALUE;
	}

	DPRINT("IData->pFileInfo->FileNameLength %d\n",
	       IData->pFileInfo->FileNameLength);

	/* copy data into WIN32_FIND_DATA structure */
        InternalCopyFindDataA(lpFindFileData, IData->pFileInfo);


	return (HANDLE)IData;
}


/*
 * @implemented
 */
BOOL
STDCALL
FindNextFileA (
	HANDLE hFindFile,
	LPWIN32_FIND_DATAA lpFindFileData)
{
	PKERNEL32_FIND_FILE_DATA IData;

	if (hFindFile == INVALID_HANDLE_VALUE)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		DPRINT("Failing request\n");
		return FALSE;
	}

	IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;
	if (!InternalFindNextFile (hFindFile, NULL))
	{
		DPRINT("InternalFindNextFile() failed\n");
		return FALSE;
	}

	DPRINT("IData->pFileInfo->FileNameLength %d\n",
	       IData->pFileInfo->FileNameLength);

	/* copy data into WIN32_FIND_DATA structure */
        InternalCopyFindDataA(lpFindFileData, IData->pFileInfo);

	return TRUE;
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
HANDLE
STDCALL
FindFirstFileW (
	LPCWSTR			lpFileName,
	LPWIN32_FIND_DATAW	lpFindFileData
	)
{

        return FindFirstFileExW (lpFileName,
                                 FindExInfoStandard,
                                 (LPVOID)lpFindFileData,
                                 FindExSearchNameMatch,
                                 NULL,
                                 0);
}

/*
 * @implemented
 */
BOOL
STDCALL
FindNextFileW (
	HANDLE			hFindFile,
	LPWIN32_FIND_DATAW	lpFindFileData
	)
{
	PKERNEL32_FIND_FILE_DATA IData;

	if (hFindFile == INVALID_HANDLE_VALUE)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		DPRINT("Failing request\n");
		return FALSE;
	}

	IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;
	if (!InternalFindNextFile(hFindFile, NULL))
	{
		DPRINT("Failing request\n");
		return FALSE;
	}

	/* copy data into WIN32_FIND_DATA structure */
        InternalCopyFindDataW(lpFindFileData, IData->pFileInfo);

	return TRUE;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstFileExW (LPCWSTR               lpFileName,
                  FINDEX_INFO_LEVELS    fInfoLevelId,
                  LPVOID                lpFindFileData,
                  FINDEX_SEARCH_OPS     fSearchOp,
                  LPVOID                lpSearchFilter,
                  DWORD                 dwAdditionalFlags)
{
    PKERNEL32_FIND_FILE_DATA IData;

    if (fInfoLevelId != FindExInfoStandard)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (fSearchOp == FindExSearchNameMatch || fSearchOp == FindExSearchLimitToDirectories)
    {
        if (lpSearchFilter)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }

        IData = InternalFindFirstFile (lpFileName, fSearchOp == FindExSearchLimitToDirectories ? TRUE : FALSE);
	if (IData == NULL)
	{
		DPRINT("Failing request\n");
		return INVALID_HANDLE_VALUE;
	}

	/* copy data into WIN32_FIND_DATA structure */
        InternalCopyFindDataW((LPWIN32_FIND_DATAW)lpFindFileData, IData->pFileInfo);

	return (HANDLE)IData;
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return INVALID_HANDLE_VALUE;
}

/*
 * @unimplemented
 */
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
    PKERNEL32_FIND_FILE_DATA IData;
    UNICODE_STRING FileNameU;
    ANSI_STRING FileNameA;

    if (fInfoLevelId != FindExInfoStandard)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (fSearchOp == FindExSearchNameMatch || fSearchOp == FindExSearchLimitToDirectories)
    {
        if (lpSearchFilter)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }
	
        RtlInitAnsiString (&FileNameA, (LPSTR)lpFileName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
            RtlAnsiStringToUnicodeString (&FileNameU, &FileNameA, TRUE);
	else
            RtlOemStringToUnicodeString (&FileNameU, &FileNameA, TRUE);

	IData = InternalFindFirstFile (FileNameU.Buffer, FALSE);

	RtlFreeUnicodeString (&FileNameU);

	if (IData == NULL)
	{
		DPRINT("Failing request\n");
		return INVALID_HANDLE_VALUE;
	}

	/* copy data into WIN32_FIND_DATA structure */
        InternalCopyFindDataA(lpFindFileData, IData->pFileInfo);
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return INVALID_HANDLE_VALUE;
}


/* EOF */
