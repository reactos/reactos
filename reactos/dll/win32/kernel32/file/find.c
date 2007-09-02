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
#include <debug.h>


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

static HANDLE
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

    return FIND_DEVICE_HANDLE;
}

static HANDLE
InternalCopyDeviceFindDataA(LPWIN32_FIND_DATAA lpFindFileData,
                            PUNICODE_STRING FileName,
                            ULONG DeviceNameInfo)
{
    UNICODE_STRING DeviceName;
    ANSI_STRING BufferA;
    CHAR Buffer[MAX_PATH];

    DeviceName.Length = DeviceName.MaximumLength = (USHORT)(DeviceNameInfo & 0xFFFF);
    DeviceName.Buffer = (LPWSTR)((ULONG_PTR)FileName->Buffer + (DeviceNameInfo >> 16));

    BufferA.MaximumLength = sizeof(Buffer) - sizeof(Buffer[0]);
    BufferA.Buffer = Buffer;
    if (bIsFileApiAnsi)
        RtlUnicodeStringToAnsiString (&BufferA, &DeviceName, FALSE);
    else
        RtlUnicodeStringToOemString (&BufferA, &DeviceName, FALSE);

    /* Return the data */
    RtlZeroMemory(lpFindFileData,
                  sizeof(*lpFindFileData));
    lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
    RtlCopyMemory(lpFindFileData->cFileName,
                  BufferA.Buffer,
                  BufferA.Length);

    return FIND_DEVICE_HANDLE;
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

static VOID
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
    PUNICODE_STRING SearchPattern,
    PVOID lpFindFileData,
    BOOL bUnicode
    )
{
    PKERNEL32_FIND_DATA_HEADER IHeader;
    PKERNEL32_FIND_FILE_DATA IData;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN Locked = FALSE;
    PFILE_BOTH_DIR_INFORMATION Buffer, FoundFile = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("InternalFindNextFile(%lx, %wZ)\n", hFindFile, SearchPattern);

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
            _SEH_TRY
            {
                if (bUnicode)
                {
                    InternalCopyFindDataW(lpFindFileData,
                                          FoundFile);
                }
                else
                {
                    InternalCopyFindDataA(lpFindFileData,
                                          FoundFile);
                }
            }
            _SEH_HANDLE
            {
            }
            _SEH_END;
        }

        if (Locked)
            RtlLeaveCriticalSection(&IData->Lock);
    }

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus (Status);
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
STDCALL
InternalFindFirstFile (
    LPCWSTR	lpFileName,
    BOOLEAN DirectoryOnly,
    PVOID lpFindFileData,
    BOOL bUnicode
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
	CURDIR DirInfo;
	ULONG DeviceNameInfo;
	HANDLE hDirectory = NULL;

	DPRINT("FindFirstFileW(lpFileName %S)\n",
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

	if (DirInfo.DosPath.Length != 0 && DirInfo.DosPath.Buffer != PathFileName.Buffer)
	{
	    if (PathFileName.Buffer != NULL)
	    {
	        /* This is a relative path to DirInfo.Handle, adjust NtPathU! */
	        NtPathU.Length = NtPathU.MaximumLength =
	            (USHORT)((ULONG_PTR)PathFileName.Buffer - (ULONG_PTR)DirInfo.DosPath.Buffer);
	        NtPathU.Buffer = DirInfo.DosPath.Buffer;
	    }
	}
	else
	{
	    /* This is an absolute path, NtPathU receives the full path */
	    DirInfo.Handle = NULL;
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

	DPRINT("lpFileName: \"%ws\"\n", lpFileName);
	DPRINT("NtPathU: \"%wZ\"\n", &NtPathU);
	DPRINT("PathFileName: \"%wZ\"\n", &PathFileName);
	DPRINT("RelativeTo: 0x%p\n", DirInfo.Handle);

	InitializeObjectAttributes (&ObjectAttributes,
	                            &NtPathU,
	                            OBJ_CASE_INSENSITIVE,
	                            DirInfo.Handle,
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
	   RtlFreeHeap (hProcessHeap,
	                0,
	                NtPathBuffer);

	   /* See if the application tries to look for a DOS device */
	   DeviceNameInfo = RtlIsDosDeviceName_U((PWSTR)((ULONG_PTR)lpFileName));
	   if (DeviceNameInfo != 0)
	   {
	       if (bUnicode)
	       {
	           InternalCopyDeviceFindDataW(lpFindFileData,
	                                       lpFileName,
	                                       DeviceNameInfo);
	       }
	       else
	       {
	           InternalCopyDeviceFindDataA(lpFindFileData,
	                                       &FileName,
	                                       DeviceNameInfo);
	       }

	       return FIND_DEVICE_HANDLE;
	   }

	   SetLastErrorByStatus (Status);
	   return INVALID_HANDLE_VALUE;
	}

	if (PathFileName.Length == 0)
	{
	    /* No file part?! */
	    NtClose(hDirectory);
	    RtlFreeHeap (hProcessHeap,
	                 0,
	                 NtPathBuffer);
	    SetLastError(ERROR_FILE_NOT_FOUND);
	    return INVALID_HANDLE_VALUE;
	}

	IHeader = RtlAllocateHeap (hProcessHeap,
	                           HEAP_ZERO_MEMORY,
	                               sizeof(KERNEL32_FIND_DATA_HEADER) +
	                               sizeof(KERNEL32_FIND_FILE_DATA) + FIND_DATA_SIZE);
	if (NULL == IHeader)
	{
	    RtlFreeHeap (hProcessHeap,
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
	                               lpFindFileData,
	                               bUnicode);

	RtlFreeHeap (hProcessHeap,
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
STDCALL
FindFirstFileA (
	LPCSTR			lpFileName,
	LPWIN32_FIND_DATAA	lpFindFileData
	)
{
	return FindFirstFileExA (lpFileName,
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
FindNextFileA (
	HANDLE hFindFile,
	LPWIN32_FIND_DATAA lpFindFileData)
{
	return InternalFindNextFile (hFindFile,
	                             NULL,
	                             lpFindFileData,
	                             FALSE);
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
	return InternalFindNextFile (hFindFile,
	                             NULL,
	                             lpFindFileData,
	                             TRUE);
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
                                      lpFindFileData,
                                      TRUE);
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
    UNICODE_STRING FileNameU;
    ANSI_STRING FileNameA;
    HANDLE Handle;

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

        Handle = InternalFindFirstFile (FileNameU.Buffer,
                                        fSearchOp == FindExSearchLimitToDirectories,
                                        lpFindFileData,
                                        FALSE);

        RtlFreeUnicodeString (&FileNameU);
        return Handle;
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
