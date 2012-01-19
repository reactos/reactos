/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/find.c
 * PURPOSE:         Find functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(kernel32file);

/* TYPES ********************************************************************/

#define FIND_DATA_SIZE	0x4000

#define FIND_DEVICE_HANDLE ((HANDLE)0x1)

typedef struct _KERNEL32_FIND_FILE_DATA
{
   HANDLE DirectoryHandle;
   RTL_CRITICAL_SECTION Lock;
   PFILE_BOTH_DIR_INFORMATION pFileInfo;
   BOOLEAN DirectoryOnly;
   BOOLEAN HasMoreData;
   BOOLEAN HasData;
   BOOLEAN LockInitialized;
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

static VOID
InternalCopyDeviceFindDataW(LPWIN32_FIND_DATAW lpFindFileData,
                            LPCWSTR lpFileName,
                            ULONG DeviceNameInfo)
{
    UNICODE_STRING DeviceName;

    DeviceName.Length = DeviceName.MaximumLength = (USHORT)(DeviceNameInfo & 0xFFFF);
    DeviceName.Buffer = (LPWSTR)((ULONG_PTR)lpFileName + (DeviceNameInfo >> 16));

    /* Return the data */
    RtlZeroMemory(lpFindFileData,
                  sizeof(*lpFindFileData));
    lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
    RtlCopyMemory(lpFindFileData->cFileName,
                  DeviceName.Buffer,
                  DeviceName.Length);
}

static VOID
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


/*
 * @implemented
 */
BOOL
WINAPI
InternalFindNextFile (
    HANDLE	hFindFile,
    PUNICODE_STRING SearchPattern,
    PVOID lpFindFileData
    )
{
    PKERNEL32_FIND_DATA_HEADER IHeader;
    PKERNEL32_FIND_FILE_DATA IData;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN Locked = FALSE;
    PFILE_BOTH_DIR_INFORMATION Buffer, FoundFile = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("InternalFindNextFile(%lx, %wZ)\n", hFindFile, SearchPattern);

    if (hFindFile != FIND_DEVICE_HANDLE)
    {
        IHeader = (PKERNEL32_FIND_DATA_HEADER)hFindFile;
        if (hFindFile == NULL || hFindFile == INVALID_HANDLE_VALUE ||
            IHeader->Type != FileFind)
        {
            SetLastError (ERROR_INVALID_HANDLE);
            return FALSE;
        }

        IData = (PKERNEL32_FIND_FILE_DATA)(IHeader + 1);
        Buffer = (PFILE_BOTH_DIR_INFORMATION)((ULONG_PTR)IData + sizeof(KERNEL32_FIND_FILE_DATA));

        if (SearchPattern == NULL)
        {
            RtlEnterCriticalSection(&IData->Lock);
            Locked = TRUE;
        }

        do
        {
            if (IData->HasData)
            {
                if (!IData->DirectoryOnly || (IData->pFileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    FoundFile = IData->pFileInfo;
                }

                if (IData->pFileInfo->NextEntryOffset != 0)
                {
                    ULONG_PTR BufferEnd;

                    IData->pFileInfo = (PFILE_BOTH_DIR_INFORMATION)((ULONG_PTR)IData->pFileInfo + IData->pFileInfo->NextEntryOffset);

                    /* Be paranoid and make sure that the next entry is completely there */
                    BufferEnd = (ULONG_PTR)Buffer + FIND_DATA_SIZE;
                    if (BufferEnd < (ULONG_PTR)IData->pFileInfo ||
                        BufferEnd < (ULONG_PTR)&IData->pFileInfo->FileNameLength + sizeof(IData->pFileInfo->FileNameLength) ||
                        BufferEnd <= (ULONG_PTR)&IData->pFileInfo->FileName[IData->pFileInfo->FileNameLength])
                    {
                        goto NeedMoreData;
                    }
                }
                else
                {
NeedMoreData:
                    IData->HasData = FALSE;

                    if (!IData->HasMoreData)
                        break;
                }
            }
            else
            {
                IData->pFileInfo = Buffer;
                IData->pFileInfo->NextEntryOffset = 0;
                Status = NtQueryDirectoryFile (IData->DirectoryHandle,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &IoStatusBlock,
                                               (PVOID)IData->pFileInfo,
                                               FIND_DATA_SIZE,
                                               FileBothDirectoryInformation,
                                               FALSE,
                                               SearchPattern,
                                               SearchPattern != NULL);

                if (Status == STATUS_BUFFER_OVERFLOW)
                {
                    IData->HasMoreData = TRUE;
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    if (!NT_SUCCESS(Status))
                        break;

                    IData->HasMoreData = FALSE;
                }

                IData->HasData = TRUE;
                SearchPattern = NULL;
            }

        } while (FoundFile == NULL);

        if (FoundFile != NULL)
        {
            _SEH2_TRY
            {
                InternalCopyFindDataW(lpFindFileData,
                                      FoundFile);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
            }
            _SEH2_END;
        }

        if (Locked)
            RtlLeaveCriticalSection(&IData->Lock);
    }

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError (Status);
        return FALSE;
    }
    else if (FoundFile == NULL)
    {
        SetLastError (ERROR_NO_MORE_FILES);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
HANDLE
WINAPI
InternalFindFirstFile (
    LPCWSTR	lpFileName,
    BOOLEAN DirectoryOnly,
    PVOID lpFindFileData
	)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	PKERNEL32_FIND_DATA_HEADER IHeader;
	PKERNEL32_FIND_FILE_DATA IData;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING NtPathU, FileName, PathFileName;
	NTSTATUS Status;
	PWSTR NtPathBuffer;
	BOOLEAN RemovedLastChar = FALSE;
	BOOL bResult;
	RTL_RELATIVE_NAME_U DirInfo;
	ULONG DeviceNameInfo;
	HANDLE hDirectory = NULL;

	TRACE("FindFirstFileW(lpFileName %S)\n",
	       lpFileName);

	RtlZeroMemory(&PathFileName,
	              sizeof(PathFileName));
	RtlInitUnicodeString(&FileName,
	                     lpFileName);

	bResult = RtlDosPathNameToNtPathName_U (lpFileName,
	                                        &NtPathU,
	                                        (PCWSTR *)((ULONG_PTR)&PathFileName.Buffer),
	                                        &DirInfo);
	if (FALSE == bResult)
	{
	    SetLastError(ERROR_PATH_NOT_FOUND);
	    return INVALID_HANDLE_VALUE;
	}

	/* Save the buffer pointer for later, we need to free it! */
	NtPathBuffer = NtPathU.Buffer;

	/* If there is a file name/pattern then determine it's length */
	if (PathFileName.Buffer != NULL)
	{
	    PathFileName.Length = NtPathU.Length -
	        (USHORT)((ULONG_PTR)PathFileName.Buffer - (ULONG_PTR)NtPathU.Buffer);
	}
	PathFileName.MaximumLength = PathFileName.Length;

	if (DirInfo.RelativeName.Length != 0 && DirInfo.RelativeName.Buffer != PathFileName.Buffer)
	{
	    if (PathFileName.Buffer != NULL)
	    {
	        /* This is a relative path to DirInfo.ContainingDirectory, adjust NtPathU! */
	        NtPathU.Length = NtPathU.MaximumLength =
	            (USHORT)((ULONG_PTR)PathFileName.Buffer - (ULONG_PTR)DirInfo.RelativeName.Buffer);
	        NtPathU.Buffer = DirInfo.RelativeName.Buffer;
	    }
	}
	else
	{
	    /* This is an absolute path, NtPathU receives the full path */
	    DirInfo.ContainingDirectory = NULL;
	    if (PathFileName.Buffer != NULL)
	    {
	        NtPathU.Length = NtPathU.MaximumLength =
	            (USHORT)((ULONG_PTR)PathFileName.Buffer - (ULONG_PTR)NtPathU.Buffer);
	    }
	}

	/* Remove the last character of the path (Unless the path is a drive and
	   ends with ":\"). If the caller however supplies a path to a device, such
	   as "C:\NUL" then the last character gets cut off, which later results in
	   NtOpenFile to return STATUS_OBJECT_NAME_NOT_FOUND, which in turn triggers
	   a fake DOS device check with RtlIsDosDeviceName_U. However, if there is a
	   real device with a name eg. "NU" in the system, FindFirstFile will succeed,
	   rendering the fake DOS device check useless... Why would they invent such a
	   stupid and broken behavior?! */
	if (NtPathU.Length >= 2 * sizeof(WCHAR) &&
	    NtPathU.Buffer[(NtPathU.Length / sizeof(WCHAR)) - 1] != L'\\' &&
	    NtPathU.Buffer[(NtPathU.Length / sizeof(WCHAR)) - 2] != L':')
	{
	    NtPathU.Length -= sizeof(WCHAR);
	    RemovedLastChar = TRUE;
	}

	TRACE("lpFileName: \"%ws\"\n", lpFileName);
	TRACE("NtPathU: \"%wZ\"\n", &NtPathU);
	TRACE("PathFileName: \"%wZ\"\n", &PathFileName);
	TRACE("RelativeTo: 0x%p\n", DirInfo.ContainingDirectory);

	InitializeObjectAttributes (&ObjectAttributes,
	                            &NtPathU,
	                            OBJ_CASE_INSENSITIVE,
	                            DirInfo.ContainingDirectory,
	                            NULL);

	Status = NtOpenFile (&hDirectory,
	                     FILE_LIST_DIRECTORY | SYNCHRONIZE,
	                     &ObjectAttributes,
	                     &IoStatusBlock,
	                     FILE_SHARE_READ|FILE_SHARE_WRITE,
	                     FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

	if (Status == STATUS_NOT_A_DIRECTORY && RemovedLastChar)
	{
	    /* Try again, this time with the last character ... */
	    NtPathU.Length += sizeof(WCHAR);

	    Status = NtOpenFile (&hDirectory,
	                         FILE_LIST_DIRECTORY | SYNCHRONIZE,
	                         &ObjectAttributes,
	                         &IoStatusBlock,
	                         FILE_SHARE_READ|FILE_SHARE_WRITE,
	                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

	    NtPathU.Length += sizeof(WCHAR);
	}

	if (!NT_SUCCESS(Status))
	{
	   RtlFreeHeap (RtlGetProcessHeap(),
	                0,
	                NtPathBuffer);

	   /* See if the application tries to look for a DOS device */
	   DeviceNameInfo = RtlIsDosDeviceName_U((PWSTR)((ULONG_PTR)lpFileName));
	   if (DeviceNameInfo != 0)
	   {
	       InternalCopyDeviceFindDataW(lpFindFileData,
	                                   lpFileName,
	                                   DeviceNameInfo);

	       return FIND_DEVICE_HANDLE;
	   }

	   BaseSetLastNTError (Status);
	   return INVALID_HANDLE_VALUE;
	}

	if (PathFileName.Length == 0)
	{
	    /* No file part?! */
	    NtClose(hDirectory);
	    RtlFreeHeap (RtlGetProcessHeap(),
	                 0,
	                 NtPathBuffer);
	    SetLastError(ERROR_FILE_NOT_FOUND);
	    return INVALID_HANDLE_VALUE;
	}

	IHeader = RtlAllocateHeap (RtlGetProcessHeap(),
	                           HEAP_ZERO_MEMORY,
	                               sizeof(KERNEL32_FIND_DATA_HEADER) +
	                               sizeof(KERNEL32_FIND_FILE_DATA) + FIND_DATA_SIZE);
	if (NULL == IHeader)
	{
	    RtlFreeHeap (RtlGetProcessHeap(),
	                 0,
	                 NtPathBuffer);
	    NtClose(hDirectory);

	    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	    return INVALID_HANDLE_VALUE;
	}

	IHeader->Type = FileFind;
	IData = (PKERNEL32_FIND_FILE_DATA)(IHeader + 1);
	IData->DirectoryHandle = hDirectory;
	IData->HasMoreData = TRUE;

	/* change pattern: "*.*" --> "*" */
	if (PathFileName.Length == 6 &&
	    RtlCompareMemory(PathFileName.Buffer,
	                     L"*.*",
	                     6) == 6)
	{
	    PathFileName.Length = 2;
	}

	IData->pFileInfo = (PVOID)((ULONG_PTR)IData + sizeof(KERNEL32_FIND_FILE_DATA));
	IData->pFileInfo->FileIndex = 0;
	IData->DirectoryOnly = DirectoryOnly;

	bResult = InternalFindNextFile((HANDLE)IHeader,
	                               &PathFileName,
	                               lpFindFileData);

	RtlFreeHeap (RtlGetProcessHeap(),
	             0,
	             NtPathBuffer);

	if (!bResult)
	{
	    FindClose((HANDLE)IHeader);
	    return INVALID_HANDLE_VALUE;
	}

	RtlInitializeCriticalSection(&IData->Lock);
	IData->LockInitialized = TRUE;

	return (HANDLE)IHeader;
}


/*
 * @implemented
 */
HANDLE
WINAPI
FindFirstFileA(IN LPCSTR lpFileName,
               OUT LPWIN32_FIND_DATAA lpFindFileData)
{
    HANDLE hSearch;
    NTSTATUS Status;
    ANSI_STRING Ansi;
    UNICODE_STRING UTF8;
    PUNICODE_STRING lpFileNameW;
    WIN32_FIND_DATAW FindFileDataW;

    lpFileNameW = Basep8BitStringToStaticUnicodeString(lpFileName);
    if (!lpFileNameW)
    {
        return INVALID_HANDLE_VALUE;
    }

    hSearch = FindFirstFileExW(lpFileNameW->Buffer,
                               FindExInfoStandard,
                               &FindFileDataW,
                               FindExSearchNameMatch,
                               NULL, 0);
    if (hSearch == INVALID_HANDLE_VALUE)
    {
        return INVALID_HANDLE_VALUE;
    }

    memcpy(lpFindFileData, &FindFileDataW, FIELD_OFFSET(WIN32_FIND_DATA, cFileName));

    RtlInitUnicodeString(&UTF8, FindFileDataW.cFileName);
    Ansi.Buffer = lpFindFileData->cFileName;
    Ansi.Length = 0;
    Ansi.MaximumLength = MAX_PATH;
    Status = BasepUnicodeStringTo8BitString(&Ansi, &UTF8, FALSE);
    if (!NT_SUCCESS(Status))
    {
        FindClose(hSearch);
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
    }

    RtlInitUnicodeString(&UTF8, FindFileDataW.cAlternateFileName);
    Ansi.Buffer = lpFindFileData->cAlternateFileName;
    Ansi.Length = 0;
    Ansi.MaximumLength = 14;
    Status = BasepUnicodeStringTo8BitString(&Ansi, &UTF8, FALSE);
    if (!NT_SUCCESS(Status))
    {
        FindClose(hSearch);
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
    }

    return hSearch;
}


/*
 * @implemented
 */
BOOL
WINAPI
FindNextFileA(IN HANDLE hFindFile,
              OUT LPWIN32_FIND_DATAA lpFindFileData)
{
    NTSTATUS Status;
    ANSI_STRING Ansi;
    UNICODE_STRING UTF8;
    WIN32_FIND_DATAW FindFileDataW;

    if (!FindNextFileW(hFindFile, &FindFileDataW))
    {
        return FALSE;
    }

    memcpy(lpFindFileData, &FindFileDataW, FIELD_OFFSET(WIN32_FIND_DATA, cFileName));

    RtlInitUnicodeString(&UTF8, FindFileDataW.cFileName);
    Ansi.Buffer = lpFindFileData->cFileName;
    Ansi.Length = 0;
    Ansi.MaximumLength = MAX_PATH;
    Status = BasepUnicodeStringTo8BitString(&Ansi, &UTF8, FALSE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    RtlInitUnicodeString(&UTF8, FindFileDataW.cAlternateFileName);
    Ansi.Buffer = lpFindFileData->cAlternateFileName;
    Ansi.Length = 0;
    Ansi.MaximumLength = 14;
    Status = BasepUnicodeStringTo8BitString(&Ansi, &UTF8, FALSE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
FindClose (
	HANDLE	hFindFile
	)
{
	PKERNEL32_FIND_DATA_HEADER IHeader;

	TRACE("FindClose(hFindFile %x)\n",hFindFile);

	if (hFindFile == FIND_DEVICE_HANDLE)
		return TRUE;

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
			if (IData->LockInitialized)
				RtlDeleteCriticalSection(&IData->Lock);
			IData->LockInitialized = FALSE;
			break;
		}

		case StreamFind:
		{
			PKERNEL32_FIND_STREAM_DATA IData = (PKERNEL32_FIND_STREAM_DATA)(IHeader + 1);
			if (IData->pFileStreamInfo != NULL)
			{
				RtlFreeHeap (RtlGetProcessHeap(), 0, IData->pFileStreamInfo);
			}
			break;
		}

		default:
			SetLastError (ERROR_INVALID_HANDLE);
			return FALSE;
	}

	RtlFreeHeap (RtlGetProcessHeap(), 0, IHeader);

	return TRUE;
}


/*
 * @implemented
 */
HANDLE
WINAPI
FindFirstFileW(IN LPCWSTR lpFileName,
               OUT LPWIN32_FIND_DATAW lpFindFileData)
{
    return FindFirstFileExW(lpFileName,
                            FindExInfoStandard,
                            lpFindFileData,
                            FindExSearchNameMatch,
                            NULL,
                            0);
}

/*
 * @implemented
 */
BOOL
WINAPI
FindNextFileW(IN HANDLE hFindFile,
              OUT LPWIN32_FIND_DATAW lpFindFileData)
{
	return InternalFindNextFile(hFindFile,
	                            NULL,
	                            lpFindFileData);
}


/*
 * @unimplemented
 */
HANDLE
WINAPI
FindFirstFileExW(IN LPCWSTR lpFileName,
                 IN FINDEX_INFO_LEVELS fInfoLevelId,
                 OUT LPVOID lpFindFileData,
                 IN FINDEX_SEARCH_OPS fSearchOp,
                 LPVOID lpSearchFilter,
                 IN DWORD dwAdditionalFlags)
{
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

        return InternalFindFirstFile (lpFileName,
                                      fSearchOp == FindExSearchLimitToDirectories,
                                      lpFindFileData);
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return INVALID_HANDLE_VALUE;
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
FindFirstFileExA(IN LPCSTR lpFileName,
                 IN FINDEX_INFO_LEVELS fInfoLevelId,
                 OUT LPVOID lpFindFileData,
                 IN FINDEX_SEARCH_OPS fSearchOp,
                 LPVOID lpSearchFilter,
                 IN DWORD dwAdditionalFlags)
{
    HANDLE hSearch;
    NTSTATUS Status;
    ANSI_STRING Ansi;
    UNICODE_STRING UTF8;
    PUNICODE_STRING lpFileNameW;
    WIN32_FIND_DATAW FindFileDataW;

    lpFileNameW = Basep8BitStringToStaticUnicodeString(lpFileName);
    if (!lpFileNameW)
    {
        return INVALID_HANDLE_VALUE;
    }

    hSearch = FindFirstFileExW(lpFileNameW->Buffer,
                               fInfoLevelId,
                               &FindFileDataW,
                               fSearchOp,
                               lpSearchFilter,
                               dwAdditionalFlags);
    if (hSearch == INVALID_HANDLE_VALUE)
    {
        return INVALID_HANDLE_VALUE;
    }

    memcpy(lpFindFileData, &FindFileDataW, FIELD_OFFSET(WIN32_FIND_DATA, cFileName));

    RtlInitUnicodeString(&UTF8, FindFileDataW.cFileName);
    Ansi.Buffer = ((LPWIN32_FIND_DATAA)lpFindFileData)->cFileName;
    Ansi.Length = 0;
    Ansi.MaximumLength = MAX_PATH;
    Status = BasepUnicodeStringTo8BitString(&Ansi, &UTF8, FALSE);
    if (!NT_SUCCESS(Status))
    {
        FindClose(hSearch);
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
    }

    RtlInitUnicodeString(&UTF8, FindFileDataW.cAlternateFileName);
    Ansi.Buffer = ((LPWIN32_FIND_DATAA)lpFindFileData)->cAlternateFileName;
    Ansi.Length = 0;
    Ansi.MaximumLength = 14;
    Status = BasepUnicodeStringTo8BitString(&Ansi, &UTF8, FALSE);
    if (!NT_SUCCESS(Status))
    {
        FindClose(hSearch);
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
    }

    return hSearch;
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
    IHeader = RtlAllocateHeap(RtlGetProcessHeap(),
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
            IData->pFileStreamInfo = RtlAllocateHeap(RtlGetProcessHeap(),
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

            pfsi = RtlReAllocateHeap(RtlGetProcessHeap(),
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
                RtlFreeHeap(RtlGetProcessHeap(),
                            0,
                            IData->pFileStreamInfo);
            }

            RtlFreeHeap(RtlGetProcessHeap(),
                        0,
                        IHeader);
        }

        BaseSetLastNTError(Status);
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
