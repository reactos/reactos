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

#define FIND_DATA_SIZE	(16*1024)

typedef struct _KERNEL32_FIND_FILE_DATA
{
   HANDLE DirectoryHandle;
   BOOLEAN DirectoryOnly;
   PFILE_BOTH_DIR_INFORMATION pFileInfo;
} KERNEL32_FIND_FILE_DATA, *PKERNEL32_FIND_FILE_DATA;

typedef struct _KERNEL32_FIND_STREAM_DATA
{
    STREAM_INFO_LEVELS InfoLevel;
    PFILE_STREAM_INFORMATION pFileStreamInfo;
    PFILE_STREAM_INFORMATION pCurrent;
} KERNEL32_FIND_STREAM_DATA, *PKERNEL32_FIND_STREAM_DATA;

typedef enum _KERNEL32_FIND_DATA_TYPE
{
    FileFind,
    StreamFind
} KERNEL32_FIND_DATA_TYPE;

typedef struct _KERNEL32_FIND_DATA_HEADER
{
    KERNEL32_FIND_DATA_TYPE Type;
} KERNEL32_FIND_DATA_HEADER, *PKERNEL32_FIND_DATA_HEADER;


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

    FileNameU.Length = FileNameU.MaximumLength = (USHORT)lpFileInfo->FileNameLength;
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
	PKERNEL32_FIND_DATA_HEADER IHeader;
	PKERNEL32_FIND_FILE_DATA IData;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	DPRINT("InternalFindNextFile(%lx)\n", hFindFile);

        IHeader = (PKERNEL32_FIND_DATA_HEADER)hFindFile;
        if (hFindFile == NULL || hFindFile == INVALID_HANDLE_VALUE ||
            IHeader->Type != FileFind)
        {
            SetLastError (ERROR_INVALID_HANDLE);
            return FALSE;
        }

        IData = (PKERNEL32_FIND_FILE_DATA)(IHeader + 1);

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
	PKERNEL32_FIND_DATA_HEADER IHeader;
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
	BOOL bResult;

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

	bResult = RtlDosPathNameToNtPathName_U (SearchPath,
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
           SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	   return NULL;
	}

	DPRINT("NtPathU \'%S\'\n", NtPathU.Buffer);

	IHeader = RtlAllocateHeap (hProcessHeap,
	                           HEAP_ZERO_MEMORY,
                                   sizeof(KERNEL32_FIND_DATA_HEADER) +
	                               sizeof(KERNEL32_FIND_FILE_DATA) + FIND_DATA_SIZE);
	if (NULL == IHeader)
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

 	IHeader->Type = FileFind;
 	IData = (PKERNEL32_FIND_FILE_DATA)(IHeader + 1);

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
	   RtlFreeHeap (hProcessHeap, 0, IHeader);
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

        bResult = InternalFindNextFile((HANDLE)IHeader, &PatternStr);
	if (NULL != SlashlessFileName)
	{
	   RtlFreeHeap(hProcessHeap,
	               0,
	               SlashlessFileName);
	}

        if (!bResult)
        {
            FindClose((HANDLE)IHeader);
            return NULL;
        }

	return (HANDLE)IHeader;
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
	PKERNEL32_FIND_DATA_HEADER IHeader;
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

	IHeader = InternalFindFirstFile (FileNameU.Buffer, FALSE);

	RtlFreeUnicodeString (&FileNameU);

	if (IHeader == NULL)
	{
		DPRINT("Failing request\n");
		return INVALID_HANDLE_VALUE;
	}

	IData = (PKERNEL32_FIND_FILE_DATA)(IHeader + 1);

	DPRINT("IData->pFileInfo->FileNameLength %d\n",
	       IData->pFileInfo->FileNameLength);

	/* copy data into WIN32_FIND_DATA structure */
        InternalCopyFindDataA(lpFindFileData, IData->pFileInfo);


	return (HANDLE)IHeader;
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

	if (!InternalFindNextFile (hFindFile, NULL))
	{
		DPRINT("InternalFindNextFile() failed\n");
		return FALSE;
	}

	IData = (PKERNEL32_FIND_FILE_DATA)((PKERNEL32_FIND_DATA_HEADER)hFindFile + 1);

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
	PKERNEL32_FIND_DATA_HEADER IHeader;

	DPRINT("FindClose(hFindFile %x)\n",hFindFile);

	if (!hFindFile || hFindFile == INVALID_HANDLE_VALUE)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		return FALSE;
	}

	IHeader = (PKERNEL32_FIND_DATA_HEADER)hFindFile;

	switch (IHeader->Type)
	{
		case FileFind:
		{
			PKERNEL32_FIND_FILE_DATA IData = (PKERNEL32_FIND_FILE_DATA)(IHeader + 1);
			CloseHandle (IData->DirectoryHandle);
			break;
		}

		case StreamFind:
		{
			PKERNEL32_FIND_STREAM_DATA IData = (PKERNEL32_FIND_STREAM_DATA)(IHeader + 1);
			if (IData->pFileStreamInfo != NULL)
			{
				RtlFreeHeap (hProcessHeap, 0, IData->pFileStreamInfo);
			}
			break;
		}

		default:
			SetLastError (ERROR_INVALID_HANDLE);
			return FALSE;
	}

	RtlFreeHeap (hProcessHeap, 0, IHeader);

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

	if (!InternalFindNextFile(hFindFile, NULL))
	{
		DPRINT("Failing request\n");
		return FALSE;
	}

	IData = (PKERNEL32_FIND_FILE_DATA)((PKERNEL32_FIND_DATA_HEADER)hFindFile + 1);

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
    PKERNEL32_FIND_DATA_HEADER IHeader;
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

        IHeader = InternalFindFirstFile (lpFileName, fSearchOp == FindExSearchLimitToDirectories ? TRUE : FALSE);
	if (IHeader == NULL)
	{
		DPRINT("Failing request\n");
		return INVALID_HANDLE_VALUE;
	}

        IData = (PKERNEL32_FIND_FILE_DATA)(IHeader + 1);

        /* copy data into WIN32_FIND_DATA structure */
        InternalCopyFindDataW((LPWIN32_FIND_DATAW)lpFindFileData, IData->pFileInfo);

	return (HANDLE)IHeader;
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
    PKERNEL32_FIND_DATA_HEADER IHeader;
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

	IHeader = InternalFindFirstFile (FileNameU.Buffer, FALSE);

	RtlFreeUnicodeString (&FileNameU);

	if (IHeader == NULL)
	{
		DPRINT("Failing request\n");
		return INVALID_HANDLE_VALUE;
	}

        IData = (PKERNEL32_FIND_FILE_DATA)(IHeader + 1);

	/* copy data into WIN32_FIND_DATA structure */
        InternalCopyFindDataA(lpFindFileData, IData->pFileInfo);

        return (HANDLE)IHeader;
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return INVALID_HANDLE_VALUE;
}


static VOID
InternalCopyStreamInfo(IN OUT PKERNEL32_FIND_STREAM_DATA IData,
                       OUT LPVOID lpFindStreamData)
{
    ASSERT(IData->pCurrent);

    switch (IData->InfoLevel)
    {
        case FindStreamInfoStandard:
        {
            ULONG StreamNameLen;
            WIN32_FIND_STREAM_DATA *StreamData = (WIN32_FIND_STREAM_DATA*)lpFindStreamData;

            StreamNameLen = IData->pCurrent->StreamNameLength;
            if (StreamNameLen > sizeof(StreamData->cStreamName) - sizeof(WCHAR))
                StreamNameLen = sizeof(StreamData->cStreamName) - sizeof(WCHAR);

            StreamData->StreamSize.QuadPart = IData->pCurrent->StreamSize.QuadPart;
            RtlCopyMemory(StreamData->cStreamName,
                          IData->pCurrent->StreamName,
                          StreamNameLen);
            StreamData->cStreamName[StreamNameLen / sizeof(WCHAR)] = L'\0';
            break;
        }

        default:
            ASSERT(FALSE);
            break;
    }
}


/*
 * @implemented
 */
HANDLE
WINAPI
FindFirstStreamW(IN LPCWSTR lpFileName,
                 IN STREAM_INFO_LEVELS InfoLevel,
                 OUT LPVOID lpFindStreamData,
                 IN DWORD dwFlags)
{
    PKERNEL32_FIND_DATA_HEADER IHeader = NULL;
    PKERNEL32_FIND_STREAM_DATA IData = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING NtPathU;
    HANDLE FileHandle = NULL;
    NTSTATUS Status;
    ULONG BufferSize = 0;

    if (dwFlags != 0 || InfoLevel != FindStreamInfoStandard ||
        lpFindStreamData == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    /* validate & translate the filename */
    if (!RtlDosPathNameToNtPathName_U(lpFileName,
                                      &NtPathU,
                                      NULL,
                                      NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }

    /* open the file */
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandle,
                          0,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          0,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* create the search context */
    IHeader = RtlAllocateHeap(hProcessHeap,
                              0,
                              sizeof(KERNEL32_FIND_DATA_HEADER) +
                                  sizeof(KERNEL32_FIND_STREAM_DATA));
    if (IHeader == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    IHeader->Type = StreamFind;
    IData = (PKERNEL32_FIND_STREAM_DATA)(IHeader + 1);

    /* capture all information about the streams */
    IData->InfoLevel = InfoLevel;
    IData->pCurrent = NULL;
    IData->pFileStreamInfo = NULL;

    do
    {
        BufferSize += 0x1000;

        if (IData->pFileStreamInfo == NULL)
        {
            IData->pFileStreamInfo = RtlAllocateHeap(hProcessHeap,
                                                     0,
                                                     BufferSize);
            if (IData->pFileStreamInfo == NULL)
            {
                Status = STATUS_NO_MEMORY;
                break;
            }
        }
        else
        {
            PFILE_STREAM_INFORMATION pfsi;

            pfsi = RtlReAllocateHeap(hProcessHeap,
                                     0,
                                     IData->pFileStreamInfo,
                                     BufferSize);
            if (pfsi == NULL)
            {
                Status = STATUS_NO_MEMORY;
                break;
            }

            IData->pFileStreamInfo = pfsi;
        }

        Status = NtQueryInformationFile(FileHandle,
                                        &IoStatusBlock,
                                        IData->pFileStreamInfo,
                                        BufferSize,
                                        FileStreamInformation);

    } while (Status == STATUS_BUFFER_TOO_SMALL);

    if (NT_SUCCESS(Status))
    {
        NtClose(FileHandle);
        FileHandle = NULL;

        /* select the first stream and return the information */
        IData->pCurrent = IData->pFileStreamInfo;
        InternalCopyStreamInfo(IData,
                               lpFindStreamData);

        /* all done */
        Status = STATUS_SUCCESS;
    }

Cleanup:
    if (FileHandle != NULL)
    {
        NtClose(FileHandle);
    }

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                NtPathU.Buffer);

    if (!NT_SUCCESS(Status))
    {
        if (IHeader != NULL)
        {
            if (IData->pFileStreamInfo != NULL)
            {
                RtlFreeHeap(hProcessHeap,
                            0,
                            IData->pFileStreamInfo);
            }

            RtlFreeHeap(hProcessHeap,
                        0,
                        IHeader);
        }

        SetLastErrorByStatus(Status);
        return INVALID_HANDLE_VALUE;
    }

    return (HANDLE)IHeader;
}


/*
 * @implemented
 */
BOOL
WINAPI
FindNextStreamW(IN HANDLE hFindStream,
                OUT LPVOID lpFindStreamData)
{
    PKERNEL32_FIND_DATA_HEADER IHeader;
    PKERNEL32_FIND_STREAM_DATA IData;

    IHeader = (PKERNEL32_FIND_DATA_HEADER)hFindStream;
    if (hFindStream == NULL || hFindStream == INVALID_HANDLE_VALUE ||
        IHeader->Type != StreamFind)
    {
        SetLastError (ERROR_INVALID_HANDLE);
        return FALSE;
    }

    IData = (PKERNEL32_FIND_STREAM_DATA)(IHeader + 1);

    /* select next stream if possible */
    if (IData->pCurrent->NextEntryOffset != 0)
    {
        IData->pCurrent = (PFILE_STREAM_INFORMATION)((ULONG_PTR)IData->pFileStreamInfo +
                                                     IData->pCurrent->NextEntryOffset);
    }
    else
    {
        SetLastError(ERROR_HANDLE_EOF);
        return FALSE;
    }

    /* return the information */
    InternalCopyStreamInfo(IData,
                           lpFindStreamData);

    return TRUE;
}


/* EOF */
