/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/find.c
 * PURPOSE:         Find functions
 * PROGRAMMERS:     Ariadne (ariadne@xs4all.nl)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 *                  Hermes Belusca-Maito
 */

/* INCLUDES *******************************************************************/

#include <k32.h>
#include <ntstrsafe.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(kernel32file);


/* TYPES **********************************************************************/

#define FIND_DATA_SIZE      0x4000
#define FIND_DEVICE_HANDLE  ((HANDLE)0x1)

typedef enum _FIND_DATA_TYPE
{
    FindFile   = 1,
    FindStream = 2
} FIND_DATA_TYPE;

/*
 * FILE_FULL_DIR_INFORMATION and FILE_BOTH_DIR_INFORMATION structures layout.
 *
 *
 *  struct FILE_FULL_DIR_INFORMATION   |   struct FILE_BOTH_DIR_INFORMATION
 * ------------------------------------+---------------------------------------
 *     ULONG NextEntryOffset;          |       ULONG NextEntryOffset;
 *     ULONG FileIndex;                |       ULONG FileIndex;
 *     LARGE_INTEGER CreationTime;     |       LARGE_INTEGER CreationTime;
 *     LARGE_INTEGER LastAccessTime;   |       LARGE_INTEGER LastAccessTime;
 *     LARGE_INTEGER LastWriteTime;    |       LARGE_INTEGER LastWriteTime;
 *     LARGE_INTEGER ChangeTime;       |       LARGE_INTEGER ChangeTime;
 *     LARGE_INTEGER EndOfFile;        |       LARGE_INTEGER EndOfFile;
 *     LARGE_INTEGER AllocationSize;   |       LARGE_INTEGER AllocationSize;
 *     ULONG FileAttributes;           |       ULONG FileAttributes;
 *     ULONG FileNameLength;           |       ULONG FileNameLength;
 *     ULONG EaSize;                   |       ULONG EaSize;
 * ------------------------------------+---------------------------------------
 *     WCHAR FileName[1];              |       CCHAR ShortNameLength;
 *                                     |       WCHAR ShortName[12];
 *                                     |       WCHAR FileName[1];
 *
 * Therefore we can use pointers to FILE_FULL_DIR_INFORMATION when one doesn't
 * want to refer to the ShortName* fields and FileName (useful for implementing
 * the FindExInfoBasic functionality for FindFirstFileEx), however a cast to
 * FILE_BOTH_DIR_INFORMATION is required when one wants to use FileName and
 * ShortName* fields (needed for the FindExInfoStandard functionality).
 *
 */
typedef union _DIR_INFORMATION
{
    PVOID DirInfo;
    PFILE_FULL_DIR_INFORMATION FullDirInfo;
    PFILE_BOTH_DIR_INFORMATION BothDirInfo;
} DIR_INFORMATION;

typedef struct _FIND_FILE_DATA
{
    HANDLE Handle;
    FINDEX_INFO_LEVELS InfoLevel;
    FINDEX_SEARCH_OPS SearchOp;

    /*
     * For handling STATUS_BUFFER_OVERFLOW errors emitted by
     * NtQueryDirectoryFile in the FindNextFile function.
     */
    BOOLEAN HasMoreData;

    /*
     * "Pointer" to the next file info structure in the buffer.
     * The type is defined by the 'InfoLevel' parameter.
     */
    DIR_INFORMATION NextDirInfo;

    BYTE Buffer[FIND_DATA_SIZE];
} FIND_FILE_DATA, *PFIND_FILE_DATA;

typedef struct _FIND_STREAM_DATA
{
    STREAM_INFO_LEVELS InfoLevel;
    PFILE_STREAM_INFORMATION FileStreamInfo;
    PFILE_STREAM_INFORMATION CurrentInfo;
} FIND_STREAM_DATA, *PFIND_STREAM_DATA;

typedef struct _FIND_DATA_HANDLE
{
    FIND_DATA_TYPE Type;
    RTL_CRITICAL_SECTION Lock;

    /*
     * Pointer to the following finding data, located at
     * (this + 1). The type is defined by the 'Type' parameter.
     */
    union
    {
        PFIND_FILE_DATA FindFileData;
        PFIND_STREAM_DATA FindStreamData;
    } u;

} FIND_DATA_HANDLE, *PFIND_DATA_HANDLE;


/* PRIVATE FUNCTIONS **********************************************************/

static VOID
CopyDeviceFindData(OUT LPWIN32_FIND_DATAW lpFindFileData,
                   IN LPCWSTR lpFileName,
                   IN ULONG DeviceNameInfo)
{
    LPCWSTR DeviceName;
    SIZE_T Length;

    _SEH2_TRY
    {
        /* DeviceNameInfo == { USHORT Offset; USHORT Length } */
        Length     =  (SIZE_T)(DeviceNameInfo & 0xFFFF);
        DeviceName = (LPCWSTR)((ULONG_PTR)lpFileName + ((DeviceNameInfo >> 16) & 0xFFFF));

        /* Return the data */
        RtlZeroMemory(lpFindFileData, sizeof(*lpFindFileData));
        lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
        RtlStringCbCopyNW(lpFindFileData->cFileName,
                          sizeof(lpFindFileData->cFileName),
                          DeviceName, Length);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return;
}

static VOID
CopyFindData(OUT LPWIN32_FIND_DATAW lpFindFileData,
             IN FINDEX_INFO_LEVELS fInfoLevelId,
             IN DIR_INFORMATION DirInfo)
{
#define ULARGE_INTEGER_2_FILETIME(ft, ul) \
do { \
    (ft).dwHighDateTime = (ul).u.HighPart; \
    (ft).dwLowDateTime  = (ul).u.LowPart ; \
} while(0)

    _SEH2_TRY
    {
        RtlZeroMemory(lpFindFileData, sizeof(*lpFindFileData));

        lpFindFileData->dwFileAttributes = DirInfo.FullDirInfo->FileAttributes;

        ULARGE_INTEGER_2_FILETIME(lpFindFileData->ftCreationTime, DirInfo.FullDirInfo->CreationTime);
        ULARGE_INTEGER_2_FILETIME(lpFindFileData->ftLastAccessTime, DirInfo.FullDirInfo->LastAccessTime);
        ULARGE_INTEGER_2_FILETIME(lpFindFileData->ftLastWriteTime, DirInfo.FullDirInfo->LastWriteTime);

        lpFindFileData->nFileSizeHigh = DirInfo.FullDirInfo->EndOfFile.u.HighPart;
        lpFindFileData->nFileSizeLow = DirInfo.FullDirInfo->EndOfFile.u.LowPart;

        /* dwReserved0 contains the NTFS reparse point tag, if any. */
        if (DirInfo.FullDirInfo->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            lpFindFileData->dwReserved0 = DirInfo.FullDirInfo->EaSize;
        else
            lpFindFileData->dwReserved0 = 0;

        /* Unused dwReserved1 field */
        lpFindFileData->dwReserved1 = 0;

        if (fInfoLevelId == FindExInfoStandard)
        {
            RtlStringCbCopyNW(lpFindFileData->cFileName,
                              sizeof(lpFindFileData->cFileName),
                              DirInfo.BothDirInfo->FileName,
                              DirInfo.BothDirInfo->FileNameLength);

            RtlStringCbCopyNW(lpFindFileData->cAlternateFileName,
                              sizeof(lpFindFileData->cAlternateFileName),
                              DirInfo.BothDirInfo->ShortName,
                              DirInfo.BothDirInfo->ShortNameLength);
        }
        else if (fInfoLevelId == FindExInfoBasic)
        {
            RtlStringCbCopyNW(lpFindFileData->cFileName,
                              sizeof(lpFindFileData->cFileName),
                              DirInfo.FullDirInfo->FileName,
                              DirInfo.FullDirInfo->FileNameLength);

            lpFindFileData->cAlternateFileName[0] = UNICODE_NULL;
        }
        else
        {
            /* Invalid InfoLevelId */
            ASSERT(FALSE);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return;
}

static VOID
CopyStreamData(IN OUT PFIND_STREAM_DATA FindStreamData,
               OUT PWIN32_FIND_STREAM_DATA lpFindStreamData)
{
    _SEH2_TRY
    {
        ASSERT(FindStreamData->CurrentInfo);

        switch (FindStreamData->InfoLevel)
        {
            case FindStreamInfoStandard:
            {
                ULONG StreamNameLen = min(FindStreamData->CurrentInfo->StreamNameLength,
                                          sizeof(lpFindStreamData->cStreamName) - sizeof(WCHAR));

                RtlZeroMemory(lpFindStreamData, sizeof(*lpFindStreamData));

                lpFindStreamData->StreamSize.QuadPart = FindStreamData->CurrentInfo->StreamSize.QuadPart;
                RtlCopyMemory(lpFindStreamData->cStreamName,
                              FindStreamData->CurrentInfo->StreamName,
                              StreamNameLen);
                lpFindStreamData->cStreamName[StreamNameLen / sizeof(WCHAR)] = UNICODE_NULL;

                break;
            }

            default:
            {
                /* Invalid InfoLevel */
                ASSERT(FALSE);
                break;
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return;
}


/* PUBLIC FUNCTIONS ***********************************************************/

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
    if (!lpFileNameW) return INVALID_HANDLE_VALUE;

    hSearch = FindFirstFileExW(lpFileNameW->Buffer,
                               FindExInfoStandard,
                               &FindFileDataW,
                               FindExSearchNameMatch,
                               NULL, 0);
    if (hSearch == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    RtlCopyMemory(lpFindFileData,
                  &FindFileDataW,
                  FIELD_OFFSET(WIN32_FIND_DATAA, cFileName));

    RtlInitUnicodeString(&UTF8, FindFileDataW.cFileName);
    Ansi.Buffer = lpFindFileData->cFileName;
    Ansi.Length = 0;
    Ansi.MaximumLength = sizeof(lpFindFileData->cFileName);
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
    Ansi.MaximumLength = sizeof(lpFindFileData->cAlternateFileName);
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
HANDLE
WINAPI
FindFirstFileW(IN LPCWSTR lpFileName,
               OUT LPWIN32_FIND_DATAW lpFindFileData)
{
    return FindFirstFileExW(lpFileName,
                            FindExInfoStandard,
                            lpFindFileData,
                            FindExSearchNameMatch,
                            NULL, 0);
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
        return FALSE;

    RtlCopyMemory(lpFindFileData,
                  &FindFileDataW,
                  FIELD_OFFSET(WIN32_FIND_DATAA, cFileName));

    RtlInitUnicodeString(&UTF8, FindFileDataW.cFileName);
    Ansi.Buffer = lpFindFileData->cFileName;
    Ansi.Length = 0;
    Ansi.MaximumLength = sizeof(lpFindFileData->cFileName);
    Status = BasepUnicodeStringTo8BitString(&Ansi, &UTF8, FALSE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    RtlInitUnicodeString(&UTF8, FindFileDataW.cAlternateFileName);
    Ansi.Buffer = lpFindFileData->cAlternateFileName;
    Ansi.Length = 0;
    Ansi.MaximumLength = sizeof(lpFindFileData->cAlternateFileName);
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
FindNextFileW(IN HANDLE hFindFile,
              OUT LPWIN32_FIND_DATAW lpFindFileData)
{
    NTSTATUS Status = STATUS_SUCCESS;
    DIR_INFORMATION FoundFile = {NULL};

    TRACE("FindNextFileW(%p, 0x%p)\n", hFindFile, lpFindFileData);

    if (hFindFile != FIND_DEVICE_HANDLE)
    {
        PFIND_DATA_HANDLE FindDataHandle = (PFIND_DATA_HANDLE)hFindFile;
        PFIND_FILE_DATA FindFileData;
        FINDEX_INFO_LEVELS InfoLevel;
        IO_STATUS_BLOCK IoStatusBlock;
        DIR_INFORMATION DirInfo = {NULL}, NextDirInfo = {NULL};

        if (hFindFile == NULL || hFindFile == INVALID_HANDLE_VALUE ||
            FindDataHandle->Type != FindFile)
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        RtlEnterCriticalSection(&FindDataHandle->Lock);

        FindFileData = FindDataHandle->u.FindFileData;
        InfoLevel = FindFileData->InfoLevel;

        do
        {
            if (FindFileData->NextDirInfo.DirInfo == NULL)
            {
                Status = NtQueryDirectoryFile(FindFileData->Handle,
                                              NULL, NULL, NULL,
                                              &IoStatusBlock,
                                              &FindFileData->Buffer,
                                              sizeof(FindFileData->Buffer),
                                              (InfoLevel == FindExInfoStandard
                                                          ? FileBothDirectoryInformation
                                                          : FileFullDirectoryInformation),
                                              FALSE,
                                              NULL, /* Use the file pattern from the first call */
                                              FALSE);
                if (Status == STATUS_BUFFER_OVERFLOW)
                {
                    FindFileData->HasMoreData = TRUE;
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    if (!NT_SUCCESS(Status)) break;
                    FindFileData->HasMoreData = FALSE;
                }

                FindFileData->NextDirInfo.DirInfo = &FindFileData->Buffer;
            }

            DirInfo = FindFileData->NextDirInfo;

            if (DirInfo.FullDirInfo->NextEntryOffset != 0)
            {
                ULONG_PTR BufferEnd = (ULONG_PTR)&FindFileData->Buffer + sizeof(FindFileData->Buffer);
                PWSTR pFileName;

                NextDirInfo.DirInfo = FindFileData->NextDirInfo.DirInfo =
                    (PVOID)((ULONG_PTR)DirInfo.DirInfo + DirInfo.FullDirInfo->NextEntryOffset);

                pFileName = (InfoLevel == FindExInfoStandard
                                        ? NextDirInfo.BothDirInfo->FileName
                                        : NextDirInfo.FullDirInfo->FileName);

                /* Be paranoid and make sure that the next entry is completely there */
                if (BufferEnd < (ULONG_PTR)NextDirInfo.DirInfo ||
                    BufferEnd < (ULONG_PTR)&NextDirInfo.FullDirInfo->FileNameLength + sizeof(NextDirInfo.FullDirInfo->FileNameLength) ||
                    BufferEnd <= (ULONG_PTR)((ULONG_PTR)pFileName + NextDirInfo.FullDirInfo->FileNameLength))
                {
                    FindFileData->NextDirInfo.DirInfo = NULL;
                }
            }
            else
            {
                FindFileData->NextDirInfo.DirInfo = NULL;
            }

            if ((FindFileData->SearchOp != FindExSearchLimitToDirectories) ||
                (DirInfo.FullDirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                FoundFile = DirInfo;
            }
        } while ( FoundFile.DirInfo == NULL && (FindFileData->NextDirInfo.DirInfo || FindFileData->HasMoreData) );

        if (FoundFile.DirInfo != NULL)
        {
            /* Return the information */
            CopyFindData(lpFindFileData, InfoLevel, FoundFile);
        }

        RtlLeaveCriticalSection(&FindDataHandle->Lock);
    }

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    else if (FoundFile.DirInfo == NULL)
    {
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
FindClose(HANDLE hFindFile)
{
    TRACE("FindClose(hFindFile %p)\n", hFindFile);

    if (hFindFile == FIND_DEVICE_HANDLE)
        return TRUE;

    if (!hFindFile || hFindFile == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Protect with SEH against closing attempts on invalid handles. */
    _SEH2_TRY
    {
        PFIND_DATA_HANDLE FindDataHandle = (PFIND_DATA_HANDLE)hFindFile;

        switch (FindDataHandle->Type)
        {
            case FindFile:
            {
                RtlEnterCriticalSection(&FindDataHandle->Lock);
                NtClose(FindDataHandle->u.FindFileData->Handle);
                RtlLeaveCriticalSection(&FindDataHandle->Lock);
                RtlDeleteCriticalSection(&FindDataHandle->Lock);
                break;
            }

            case FindStream:
            {
                RtlEnterCriticalSection(&FindDataHandle->Lock);
                if (FindDataHandle->u.FindStreamData->FileStreamInfo != NULL)
                {
                    RtlFreeHeap(RtlGetProcessHeap(), 0,
                                FindDataHandle->u.FindStreamData->FileStreamInfo);
                }
                RtlLeaveCriticalSection(&FindDataHandle->Lock);
                RtlDeleteCriticalSection(&FindDataHandle->Lock);
                break;
            }

            default:
            {
                SetLastError(ERROR_INVALID_HANDLE);
                _SEH2_YIELD(return FALSE);
            }
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, FindDataHandle);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        BaseSetLastNTError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    return TRUE;
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
    LPWIN32_FIND_DATAA lpFindFileDataA = (LPWIN32_FIND_DATAA)lpFindFileData;

    if ((fInfoLevelId != FindExInfoStandard && fInfoLevelId != FindExInfoBasic) ||
        fSearchOp == FindExSearchLimitToDevices ||
        dwAdditionalFlags & ~FIND_FIRST_EX_CASE_SENSITIVE /* only supported flag for now */)
    {
        SetLastError(fSearchOp == FindExSearchLimitToDevices
                                ? ERROR_NOT_SUPPORTED
                                : ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    lpFileNameW = Basep8BitStringToStaticUnicodeString(lpFileName);
    if (!lpFileNameW) return INVALID_HANDLE_VALUE;

    hSearch = FindFirstFileExW(lpFileNameW->Buffer,
                               fInfoLevelId,
                               &FindFileDataW,
                               fSearchOp,
                               lpSearchFilter,
                               dwAdditionalFlags);
    if (hSearch == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    RtlCopyMemory(lpFindFileDataA,
                  &FindFileDataW,
                  FIELD_OFFSET(WIN32_FIND_DATAA, cFileName));

    RtlInitUnicodeString(&UTF8, FindFileDataW.cFileName);
    Ansi.Buffer = lpFindFileDataA->cFileName;
    Ansi.Length = 0;
    Ansi.MaximumLength = sizeof(lpFindFileDataA->cFileName);
    Status = BasepUnicodeStringTo8BitString(&Ansi, &UTF8, FALSE);
    if (!NT_SUCCESS(Status))
    {
        FindClose(hSearch);
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
    }

    if (fInfoLevelId != FindExInfoBasic)
    {
        RtlInitUnicodeString(&UTF8, FindFileDataW.cAlternateFileName);
        Ansi.Buffer = lpFindFileDataA->cAlternateFileName;
        Ansi.Length = 0;
        Ansi.MaximumLength = sizeof(lpFindFileDataA->cAlternateFileName);
        Status = BasepUnicodeStringTo8BitString(&Ansi, &UTF8, FALSE);
        if (!NT_SUCCESS(Status))
        {
            FindClose(hSearch);
            BaseSetLastNTError(Status);
            return INVALID_HANDLE_VALUE;
        }
    }
    else
    {
        lpFindFileDataA->cAlternateFileName[0] = ANSI_NULL;
    }

    return hSearch;
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
    TRACE("FindFirstFileExW(lpFileName %S)\n", lpFileName);

    if ((fInfoLevelId != FindExInfoStandard && fInfoLevelId != FindExInfoBasic) ||
        fSearchOp == FindExSearchLimitToDevices ||
        dwAdditionalFlags & ~FIND_FIRST_EX_CASE_SENSITIVE /* only supported flag for now */)
    {
        SetLastError(fSearchOp == FindExSearchLimitToDevices
                                ? ERROR_NOT_SUPPORTED
                                : ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if (fSearchOp == FindExSearchNameMatch ||
        fSearchOp == FindExSearchLimitToDirectories)
    {
        LPWIN32_FIND_DATAW Win32FindData = (LPWIN32_FIND_DATAW)lpFindFileData;
        PFIND_DATA_HANDLE FindDataHandle;
        PFIND_FILE_DATA FindFileData;

        UNICODE_STRING NtPath, FilePattern, FileName;
        PWSTR NtPathBuffer;
        RTL_RELATIVE_NAME_U RelativePath;
        ULONG DeviceNameInfo = 0;

        NTSTATUS Status;
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK IoStatusBlock;
        HANDLE hDirectory = NULL;

        BOOLEAN HadADot = FALSE;

        /*
         * May represent many FILE_BOTH_DIR_INFORMATION
         * or many FILE_FULL_DIR_INFORMATION structures.
         * NOTE: NtQueryDirectoryFile requires the buffer to be ULONG-aligned
         */
        DECLSPEC_ALIGN(4) BYTE DirectoryInfo[FIND_DATA_SIZE];
        DIR_INFORMATION DirInfo = { .DirInfo = &DirectoryInfo };

        /* The search filter is always unused */
        if (lpSearchFilter)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }

        RtlInitUnicodeString(&FileName, lpFileName);
        if (FileName.Length != 0 && FileName.Buffer[FileName.Length / sizeof(WCHAR) - 1] == L'.')
        {
            HadADot = TRUE;
        }

        if (!RtlDosPathNameToNtPathName_U(lpFileName,
                                          &NtPath,
                                          (PCWSTR*)&FilePattern.Buffer,
                                          &RelativePath))
        {
            SetLastError(ERROR_PATH_NOT_FOUND);
            return INVALID_HANDLE_VALUE;
        }

        DPRINT("lpFileName = '%S'\n", lpFileName);
        DPRINT("FilePattern.Buffer = '%S'\n", FilePattern.Buffer);
        DPRINT("RelativePath.RelativeName = '%wZ'\n", &RelativePath.RelativeName);
        DPRINT("NtPath.Buffer = '%S'\n", NtPath.Buffer);
        DPRINT("NtPath - Before = '%wZ'\n", &NtPath);

        /* Save the buffer pointer for later, we need to free it! */
        NtPathBuffer = NtPath.Buffer;

        /*
         * Contrary to what Windows does, check NOW whether or not
         * lpFileName is a DOS driver. Therefore we don't have to
         * write broken code to check that.
         */
        if (!FilePattern.Buffer || !*FilePattern.Buffer)
        {
            /* No file pattern specified, or DOS device */

            DeviceNameInfo = RtlIsDosDeviceName_U(lpFileName);
            if (DeviceNameInfo != 0)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathBuffer);

                /* OK, it's really a DOS device */
                CopyDeviceFindData(Win32FindData, lpFileName, DeviceNameInfo);
                return FIND_DEVICE_HANDLE;
            }
        }

        /* A file pattern was specified, or it was not a DOS device */

        /* If there is a file pattern then determine its length */
        if (FilePattern.Buffer != NULL)
        {
            FilePattern.Length = NtPath.Length -
                (USHORT)((ULONG_PTR)FilePattern.Buffer - (ULONG_PTR)NtPath.Buffer);
        }
        else
        {
            FilePattern.Length = 0;
        }
        FilePattern.MaximumLength = FilePattern.Length;

        if (RelativePath.RelativeName.Length != 0 &&
            RelativePath.RelativeName.Buffer != FilePattern.Buffer)
        {
            if (FilePattern.Buffer != NULL)
            {
                /* This is a relative path to RelativePath.ContainingDirectory, adjust NtPath! */
                NtPath.Length = NtPath.MaximumLength =
                    (USHORT)((ULONG_PTR)FilePattern.Buffer - (ULONG_PTR)RelativePath.RelativeName.Buffer);
                NtPath.Buffer = RelativePath.RelativeName.Buffer;
            }
        }
        else
        {
            /* This is an absolute path, NtPath receives the full path */
            RelativePath.ContainingDirectory = NULL;
            if (FilePattern.Buffer != NULL)
            {
                NtPath.Length = NtPath.MaximumLength =
                    (USHORT)((ULONG_PTR)FilePattern.Buffer - (ULONG_PTR)NtPath.Buffer);
            }
        }

        DPRINT("NtPath - After = '%wZ'\n", &NtPath);
        DPRINT("FilePattern = '%wZ'\n", &FilePattern);
        DPRINT("RelativeTo = 0x%p\n", RelativePath.ContainingDirectory);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &NtPath,
                                   (dwAdditionalFlags & FIND_FIRST_EX_CASE_SENSITIVE) ? 0 : OBJ_CASE_INSENSITIVE,
                                   RelativePath.ContainingDirectory,
                                   NULL);

        Status = NtOpenFile(&hDirectory,
                            FILE_LIST_DIRECTORY | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

        if (!NT_SUCCESS(Status))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathBuffer);

            /* Adjust the last error codes */
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_OBJECT_PATH_NOT_FOUND;
            else if (Status == STATUS_OBJECT_TYPE_MISMATCH)
                Status = STATUS_OBJECT_PATH_NOT_FOUND;

            BaseSetLastNTError(Status);
            return INVALID_HANDLE_VALUE;
        }

        /*
         * Fail if there is not any file pattern,
         * since we are not looking for a device.
         */
        if (FilePattern.Length == 0)
        {
            NtClose(hDirectory);
            RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathBuffer);

            SetLastError(ERROR_FILE_NOT_FOUND);
            return INVALID_HANDLE_VALUE;
        }

        /* Change pattern: "*.*" --> "*" */
        if (FilePattern.Length == 6 &&
            RtlCompareMemory(FilePattern.Buffer, L"*.*", 6) == 6)
        {
            FilePattern.Length = 2;
        }
        else
        {
            /* Translate wildcard from "real" world to DOS world for lower interpretation */
            USHORT PatternIndex = 0;
            while (PatternIndex < FilePattern.Length / sizeof(WCHAR))
            {
                if (PatternIndex > 0)
                {
                    if (FilePattern.Buffer[PatternIndex] == L'.' &&
                        FilePattern.Buffer[PatternIndex - 1] == L'*')
                    {
                        FilePattern.Buffer[PatternIndex - 1] = L'<';
                    }
                }

                if (FilePattern.Buffer[PatternIndex] == L'?')
                {
                    FilePattern.Buffer[PatternIndex] = L'>';
                    if (PatternIndex > 0)
                    {
                        if (FilePattern.Buffer[PatternIndex - 1] == L'.')
                        {
                            FilePattern.Buffer[PatternIndex - 1] = L'\"';
                        }
                    }
                }
                else if (FilePattern.Buffer[PatternIndex] == L'*')
                {
                    if (PatternIndex > 0)
                    {
                        if (FilePattern.Buffer[PatternIndex - 1] == L'.')
                        {
                            FilePattern.Buffer[PatternIndex - 1] = L'\"';
                        }
                    }
                }

                PatternIndex++;
            }

            /* Handle partial wc if our last dot was eaten */
            if (HadADot)
            {
                if (FilePattern.Buffer[FilePattern.Length / sizeof(WCHAR) - 1] == L'*')
                {
                    FilePattern.Buffer[FilePattern.Length / sizeof(WCHAR) - 1] = L'<';
                }
            }
        }

        Status = NtQueryDirectoryFile(hDirectory,
                                      NULL, NULL, NULL,
                                      &IoStatusBlock,
                                      DirInfo.DirInfo, // == &DirectoryInfo
                                      sizeof(DirectoryInfo),
                                      (fInfoLevelId == FindExInfoStandard
                                                     ? FileBothDirectoryInformation
                                                     : FileFullDirectoryInformation),
                                      TRUE, /* Return a single entry */
                                      &FilePattern,
                                      TRUE);

        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathBuffer);

        if (!NT_SUCCESS(Status))
        {
            NtClose(hDirectory);
            BaseSetLastNTError(Status);
            return INVALID_HANDLE_VALUE;
        }

        ASSERT(DirInfo.FullDirInfo->NextEntryOffset == 0);

        /* Return the information */
        CopyFindData(Win32FindData, fInfoLevelId, DirInfo);

        /*
         * Initialization of the search handle.
         */
        FindDataHandle = RtlAllocateHeap(RtlGetProcessHeap(),
                                         HEAP_ZERO_MEMORY,
                                         sizeof(FIND_DATA_HANDLE) +
                                             sizeof(FIND_FILE_DATA));
        if (!FindDataHandle)
        {
            NtClose(hDirectory);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return INVALID_HANDLE_VALUE;
        }

        FindDataHandle->Type = FindFile;
        FindDataHandle->u.FindFileData = (PFIND_FILE_DATA)(FindDataHandle + 1);
        FindFileData = FindDataHandle->u.FindFileData;

        FindFileData->Handle = hDirectory;
        FindFileData->InfoLevel = fInfoLevelId;
        FindFileData->SearchOp = fSearchOp;
        FindFileData->HasMoreData = FALSE;
        FindFileData->NextDirInfo.DirInfo = NULL;

        /* The critical section must always be initialized */
        Status = RtlInitializeCriticalSection(&FindDataHandle->Lock);
        if (!NT_SUCCESS(Status))
        {
            NtClose(hDirectory);
            RtlFreeHeap(RtlGetProcessHeap(), 0, FindDataHandle);

            BaseSetLastNTError(Status);
            return INVALID_HANDLE_VALUE;
        }

        return (HANDLE)FindDataHandle;
    }
    else
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        return INVALID_HANDLE_VALUE;
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
    PFIND_DATA_HANDLE FindDataHandle = NULL;
    PFIND_STREAM_DATA FindStreamData;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING NtFilePath;
    HANDLE FileHandle = NULL;
    NTSTATUS Status;
    ULONG BufferSize = 0;

    if (dwFlags != 0 || InfoLevel != FindStreamInfoStandard ||
        lpFindStreamData == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    /* Validate and translate the filename */
    if (!RtlDosPathNameToNtPathName_U(lpFileName,
                                      &NtFilePath,
                                      NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }

    /* Open the file */
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtFilePath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandle,
                          0,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL, 0,
                          FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          0, NULL, 0);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /*
     * Initialization of the search handle.
     */
    FindDataHandle = RtlAllocateHeap(RtlGetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     sizeof(FIND_DATA_HANDLE) +
                                         sizeof(FIND_STREAM_DATA));
    if (!FindDataHandle)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    FindDataHandle->Type = FindStream;
    FindDataHandle->u.FindStreamData = (PFIND_STREAM_DATA)(FindDataHandle + 1);
    FindStreamData = FindDataHandle->u.FindStreamData;

    FindStreamData->InfoLevel = InfoLevel;
    FindStreamData->FileStreamInfo = NULL;
    FindStreamData->CurrentInfo = NULL;

    /* The critical section must always be initialized */
    Status = RtlInitializeCriticalSection(&FindDataHandle->Lock);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, FindDataHandle);
        goto Cleanup;
    }

    /* Capture all information about the streams */
    do
    {
        BufferSize += 0x1000;

        if (FindStreamData->FileStreamInfo == NULL)
        {
            FindStreamData->FileStreamInfo = RtlAllocateHeap(RtlGetProcessHeap(),
                                                              HEAP_ZERO_MEMORY,
                                                              BufferSize);
            if (FindStreamData->FileStreamInfo == NULL)
            {
                Status = STATUS_NO_MEMORY;
                break;
            }
        }
        else
        {
            PFILE_STREAM_INFORMATION pfsi;

            pfsi = RtlReAllocateHeap(RtlGetProcessHeap(),
                                     0, // HEAP_ZERO_MEMORY,
                                     FindStreamData->FileStreamInfo,
                                     BufferSize);
            if (pfsi == NULL)
            {
                Status = STATUS_NO_MEMORY;
                break;
            }

            FindStreamData->FileStreamInfo = pfsi;
        }

        Status = NtQueryInformationFile(FileHandle,
                                        &IoStatusBlock,
                                        FindStreamData->FileStreamInfo,
                                        BufferSize,
                                        FileStreamInformation);

    } while (Status == STATUS_BUFFER_TOO_SMALL);

    if (NT_SUCCESS(Status))
    {
        /* Select the first stream and return the information */
        FindStreamData->CurrentInfo = FindStreamData->FileStreamInfo;
        CopyStreamData(FindStreamData, lpFindStreamData);

        /* All done */
        Status = STATUS_SUCCESS;
    }
    else
    {
        if (FindStreamData->FileStreamInfo)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, FindStreamData->FileStreamInfo);
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, FindDataHandle);
    }

Cleanup:
    if (FileHandle) NtClose(FileHandle);

    RtlFreeHeap(RtlGetProcessHeap(), 0, NtFilePath.Buffer);

    if (NT_SUCCESS(Status))
    {
        return (HANDLE)FindDataHandle;
    }
    else
    {
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
    }
}


/*
 * @implemented
 */
BOOL
WINAPI
FindNextStreamW(IN HANDLE hFindStream,
                OUT LPVOID lpFindStreamData)
{
    PFIND_DATA_HANDLE FindDataHandle = (PFIND_DATA_HANDLE)hFindStream;
    PFIND_STREAM_DATA FindStreamData;

    if (hFindStream == NULL || hFindStream == INVALID_HANDLE_VALUE ||
        FindDataHandle->Type != FindStream)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    RtlEnterCriticalSection(&FindDataHandle->Lock);

    FindStreamData = FindDataHandle->u.FindStreamData;

    /* Select next stream if possible */
    if (FindStreamData->CurrentInfo->NextEntryOffset != 0)
    {
        FindStreamData->CurrentInfo = (PFILE_STREAM_INFORMATION)((ULONG_PTR)FindStreamData->CurrentInfo +
                                                                 FindStreamData->CurrentInfo->NextEntryOffset);

        /* Return the information */
        CopyStreamData(FindStreamData, lpFindStreamData);

        RtlLeaveCriticalSection(&FindDataHandle->Lock);
        return TRUE;
    }
    else
    {
        RtlLeaveCriticalSection(&FindDataHandle->Lock);

        SetLastError(ERROR_HANDLE_EOF);
        return FALSE;
    }
}

/* EOF */
