/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    filefind.c

Abstract:

    This module implements Win32 FindFirst/FindNext

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include "basedll.h"

#define FIND_BUFFER_SIZE 4096

PFINDFILE_HANDLE
BasepInitializeFindFileHandle(
    IN HANDLE DirectoryHandle
    )
{
    PFINDFILE_HANDLE FindFileHandle;

    FindFileHandle = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( FIND_TAG ), sizeof(*FindFileHandle));
    if ( FindFileHandle ) {
        FindFileHandle->DirectoryHandle = DirectoryHandle;
        FindFileHandle->FindBufferBase = NULL;
        FindFileHandle->FindBufferNext = NULL;
        FindFileHandle->FindBufferLength = 0;
        FindFileHandle->FindBufferValidLength = 0;
        if ( !NT_SUCCESS(RtlInitializeCriticalSection(&FindFileHandle->FindBufferLock)) ){
            RtlFreeHeap(RtlProcessHeap(), 0,FindFileHandle);
            FindFileHandle = NULL;
            }
        }
    return FindFileHandle;
}

HANDLE
APIENTRY
FindFirstFileA(
    LPCSTR lpFileName,
    LPWIN32_FIND_DATAA lpFindFileData
    )

/*++

Routine Description:

    ANSI thunk to FindFirstFileW

--*/

{
    HANDLE ReturnValue;
    PUNICODE_STRING Unicode;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    WIN32_FIND_DATAW FindFileData;
    ANSI_STRING AnsiString;

    Unicode = Basep8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return INVALID_HANDLE_VALUE;
    }
        
    ReturnValue = FindFirstFileExW(
                    (LPCWSTR)Unicode->Buffer,
                    FindExInfoStandard,
                    &FindFileData,
                    FindExSearchNameMatch,
                    NULL,
                    0
                    );

    if ( ReturnValue == INVALID_HANDLE_VALUE ) {
        return ReturnValue;
        }
    RtlMoveMemory(
        lpFindFileData,
        &FindFileData,
        (ULONG_PTR)&FindFileData.cFileName[0] - (ULONG_PTR)&FindFileData
        );
    RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cFileName);
    AnsiString.Buffer = lpFindFileData->cFileName;
    AnsiString.MaximumLength = MAX_PATH;
    Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cAlternateFileName);
        AnsiString.Buffer = lpFindFileData->cAlternateFileName;
        AnsiString.MaximumLength = 14;
        Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    }
    if ( !NT_SUCCESS(Status) ) {
        FindClose(ReturnValue);
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }
    return ReturnValue;
}

HANDLE
APIENTRY
FindFirstFileW(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData
    )

/*++

Routine Description:

    A directory can be searched for the first entry whose name and
    attributes match the specified name using FindFirstFile.

    This API is provided to open a find file handle and return
    information about the first file whose name match the specified
    pattern.  Once established, the find file handle can be used to
    search for other files that match the same pattern.  When the find
    file handle is no longer needed, it should be closed.

    Note that while this interface only returns information for a single
    file, an implementation is free to buffer several matching files
    that can be used to satisfy subsequent calls to FindNextFile.  Also
    not that matches are done by name only.  This API does not do
    attribute based matching.

    This API is similar to DOS (int 21h, function 4Eh), and OS/2's
    DosFindFirst.  For portability reasons, its data structures and
    parameter passing is somewhat different.

Arguments:

    lpFileName - Supplies the file name of the file to find.  The file name
        may contain the DOS wild card characters '*' and '?'.

    lpFindFileData - On a successful find, this parameter returns information
        about the located file:

        WIN32_FIND_DATA Structure:

        DWORD dwFileAttributes - Returns the file attributes of the found
            file.

        FILETIME ftCreationTime - Returns the time that the file was created.
            A value of 0,0 specifies that the file system containing the
            file does not support this time field.

        FILETIME ftLastAccessTime - Returns the time that the file was last
            accessed.  A value of 0,0 specifies that the file system
            containing the file does not support this time field.

        FILETIME ftLastWriteTime - Returns the time that the file was last
            written.  A file systems support this time field.

        DWORD nFileSizeHigh - Returns the high order 32 bits of the
            file's size.

        DWORD nFileSizeLow - Returns the low order 32-bits of the file's
            size in bytes.

        UCHAR cFileName[MAX_PATH] - Returns the null terminated name of
            the file.

Return Value:

    Not -1 - Returns a find first handle
        that can be used in a subsequent call to FindNextFile or FindClose.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    return FindFirstFileExW(
                lpFileName,
                FindExInfoStandard,
                lpFindFileData,
                FindExSearchNameMatch,
                NULL,
                0
                );
}



BOOL
APIENTRY
FindNextFileA(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAA lpFindFileData
    )

/*++

Routine Description:

    ANSI thunk to FindFileDataW

--*/

{

    BOOL ReturnValue;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    WIN32_FIND_DATAW FindFileData;

    ReturnValue = FindNextFileW(hFindFile,&FindFileData);
    if ( !ReturnValue ) {
        return ReturnValue;
        }
    RtlMoveMemory(
        lpFindFileData,
        &FindFileData,
        (ULONG_PTR)&FindFileData.cFileName[0] - (ULONG_PTR)&FindFileData
        );
    RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cFileName);
    AnsiString.Buffer = lpFindFileData->cFileName;
    AnsiString.MaximumLength = MAX_PATH;
    Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cAlternateFileName);
        AnsiString.Buffer = lpFindFileData->cAlternateFileName;
        AnsiString.MaximumLength = 14;
        Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    }
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return ReturnValue;
}

BOOL
APIENTRY
FindNextFileW(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAW lpFindFileData
    )

/*++

Routine Description:

    Once a successful call has been made to FindFirstFile, subsequent
    matching files can be located using FindNextFile.

    This API is used to continue a file search from a previous call to
    FindFirstFile.  This API returns successfully with the next file
    that matches the search pattern established in the original
    FindFirstFile call.  If no file match can be found NO_MORE_FILES is
    returned.

    Note that while this interface only returns information for a single
    file, an implementation is free to buffer several matching files
    that can be used to satisfy subsequent calls to FindNextFile.  Also
    not that matches are done by name only.  This API does not do
    attribute based matching.

    This API is similar to DOS (int 21h, function 4Fh), and OS/2's
    DosFindNext.  For portability reasons, its data structures and
    parameter passing is somewhat different.

Arguments:

    hFindFile - Supplies a find file handle returned in a previous call
        to FindFirstFile.

    lpFindFileData - On a successful find, this parameter returns information
        about the located file.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PFINDFILE_HANDLE FindFileHandle;
    BOOL ReturnValue;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;

    if ( hFindFile == BASE_FIND_FIRST_DEVICE_HANDLE ) {
        BaseSetLastNTError(STATUS_NO_MORE_FILES);
        return FALSE;
        }

    if ( hFindFile == INVALID_HANDLE_VALUE ) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
        }

    ReturnValue = TRUE;
    FindFileHandle = (PFINDFILE_HANDLE)hFindFile;
    RtlEnterCriticalSection(&FindFileHandle->FindBufferLock);
    try {

        //
        // If we haven't called find next yet, then
        // allocate the find buffer.
        //

        if ( !FindFileHandle->FindBufferBase ) {
            FindFileHandle->FindBufferBase = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( FIND_TAG ), FIND_BUFFER_SIZE);
            if (FindFileHandle->FindBufferBase) {
                FindFileHandle->FindBufferNext = FindFileHandle->FindBufferBase;
                FindFileHandle->FindBufferLength = FIND_BUFFER_SIZE;
                FindFileHandle->FindBufferValidLength = 0;
                }
            else {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                ReturnValue = FALSE;
                goto leavefinally;
                }
            }

        //
        // Test to see if there is no data in the find file buffer
        //

        DirectoryInfo = (PFILE_BOTH_DIR_INFORMATION)FindFileHandle->FindBufferNext;
        if ( FindFileHandle->FindBufferBase == (PVOID)DirectoryInfo ) {

            Status = NtQueryDirectoryFile(
                        FindFileHandle->DirectoryHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        DirectoryInfo,
                        FindFileHandle->FindBufferLength,
                        FileBothDirectoryInformation,
                        FALSE,
                        NULL,
                        FALSE
                        );

            //
            //  ***** Do a kludge hack fix for now *****
            //
            //  Forget about the last, partial, entry.
            //

            if ( Status == STATUS_BUFFER_OVERFLOW ) {

                PULONG Ptr;
                PULONG PriorPtr;

                Ptr = (PULONG)DirectoryInfo;
                PriorPtr = NULL;

                while ( *Ptr != 0 ) {

                    PriorPtr = Ptr;
                    Ptr += (*Ptr / sizeof(ULONG));
                }

                if (PriorPtr != NULL) { *PriorPtr = 0; }
                Status = STATUS_SUCCESS;
            }

            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                ReturnValue = FALSE;
                goto leavefinally;
                }
            }

        if ( DirectoryInfo->NextEntryOffset ) {
            FindFileHandle->FindBufferNext = (PVOID)(
                (PUCHAR)DirectoryInfo + DirectoryInfo->NextEntryOffset);
            }
        else {
            FindFileHandle->FindBufferNext = FindFileHandle->FindBufferBase;
            }

        //
        // Attributes are composed of the attributes returned by NT.
        //

        lpFindFileData->dwFileAttributes = DirectoryInfo->FileAttributes;
        lpFindFileData->ftCreationTime = *(LPFILETIME)&DirectoryInfo->CreationTime;
        lpFindFileData->ftLastAccessTime = *(LPFILETIME)&DirectoryInfo->LastAccessTime;
        lpFindFileData->ftLastWriteTime = *(LPFILETIME)&DirectoryInfo->LastWriteTime;
        lpFindFileData->nFileSizeHigh = DirectoryInfo->EndOfFile.HighPart;
        lpFindFileData->nFileSizeLow = DirectoryInfo->EndOfFile.LowPart;

        RtlMoveMemory( lpFindFileData->cFileName,
                       DirectoryInfo->FileName,
                       DirectoryInfo->FileNameLength );

        lpFindFileData->cFileName[DirectoryInfo->FileNameLength >> 1] = UNICODE_NULL;

        RtlMoveMemory( lpFindFileData->cAlternateFileName,
                       DirectoryInfo->ShortName,
                       DirectoryInfo->ShortNameLength );

        lpFindFileData->cAlternateFileName[DirectoryInfo->ShortNameLength >> 1] = UNICODE_NULL;

        //
        // For NTFS reparse points we return the reparse point data tag in dwReserved0.
        //

        if ( DirectoryInfo->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) {
            lpFindFileData->dwReserved0 = DirectoryInfo->EaSize;
            }

leavefinally:;
        }
    finally{
        RtlLeaveCriticalSection(&FindFileHandle->FindBufferLock);
        }
    return ReturnValue;
}

BOOL
FindClose(
    HANDLE hFindFile
    )

/*++

Routine Description:

    A find file context created by FindFirstFile can be closed using
    FindClose.

    This API is used to inform the system that a find file handle
    created by FindFirstFile is no longer needed.  On systems that
    maintain internal state for each find file context, this API informs
    the system that this state no longer needs to be maintained.

    Once this call has been made, the hFindFile may not be used in a
    subsequent call to either FindNextFile or FindClose.

    This API has no DOS counterpart, but is similar to OS/2's
    DosFindClose.

Arguments:

    hFindFile - Supplies a find file handle returned in a previous call
        to FindFirstFile that is no longer needed.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    PFINDFILE_HANDLE FindFileHandle;
    HANDLE DirectoryHandle;

    if ( hFindFile == BASE_FIND_FIRST_DEVICE_HANDLE ) {
        return TRUE;
        }

    if ( hFindFile == INVALID_HANDLE_VALUE ) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
        }

    try {
        FindFileHandle = (PFINDFILE_HANDLE)hFindFile;
        RtlEnterCriticalSection(&FindFileHandle->FindBufferLock);
        DirectoryHandle = FindFileHandle->DirectoryHandle;

        Status = NtClose(DirectoryHandle);
        if ( NT_SUCCESS(Status) ) {
            if (FindFileHandle->FindBufferBase) {
                RtlFreeHeap(RtlProcessHeap(), 0,FindFileHandle->FindBufferBase);
                }
            RtlDeleteCriticalSection(&FindFileHandle->FindBufferLock);
            RtlFreeHeap(RtlProcessHeap(), 0,FindFileHandle);
            return TRUE;
            }
        else {
            RtlLeaveCriticalSection(&FindFileHandle->FindBufferLock);
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
    except ( EXCEPTION_EXECUTE_HANDLER ) {
        BaseSetLastNTError(GetExceptionCode());
        return FALSE;
        }
    return FALSE;
}

HANDLE
WINAPI
FindFirstFileExA(
    LPCSTR lpFileName,
    FINDEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFindFileData,
    FINDEX_SEARCH_OPS fSearchOp,
    LPVOID lpSearchFilter,
    DWORD dwAdditionalFlags
    )
{
    HANDLE ReturnValue;
    PUNICODE_STRING Unicode;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    WIN32_FIND_DATAW FindFileData;
    LPWIN32_FIND_DATAA lpFindFileDataA;
    ANSI_STRING AnsiString;

    //
    // this code assumes that only FindExInfoStandard is supperted by ExW version
    // when more info levels are added, the W->A translation code needs to be modified
    //

    lpFindFileDataA = (LPWIN32_FIND_DATAA)lpFindFileData;
    
    Unicode = Basep8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return INVALID_HANDLE_VALUE;
    }
        
    ReturnValue = FindFirstFileExW(
                    (LPCWSTR)Unicode->Buffer,
                    fInfoLevelId,
                    (LPVOID)&FindFileData,
                    fSearchOp,
                    lpSearchFilter,
                    dwAdditionalFlags
                    );

    if ( ReturnValue == INVALID_HANDLE_VALUE ) {
        return ReturnValue;
        }
    RtlMoveMemory(
        lpFindFileData,
        &FindFileData,
        (ULONG_PTR)&FindFileData.cFileName[0] - (ULONG_PTR)&FindFileData
        );
    RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cFileName);
    AnsiString.Buffer = lpFindFileDataA->cFileName;
    AnsiString.MaximumLength = MAX_PATH;
    Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cAlternateFileName);
        AnsiString.Buffer = lpFindFileDataA->cAlternateFileName;
        AnsiString.MaximumLength = 14;
        Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    }
    if ( !NT_SUCCESS(Status) ) {
        FindClose(ReturnValue);
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }
    return ReturnValue;

}

HANDLE
WINAPI
FindFirstFileExW(
    LPCWSTR lpFileName,
    FINDEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFindFileData,
    FINDEX_SEARCH_OPS fSearchOp,
    LPVOID lpSearchFilter,
    DWORD dwAdditionalFlags
    )

/*++

Routine Description:

    A directory can be searched for the first entry whose name and
    attributes match the specified name using FindFirstFileEx.

    This API is provided to open a find file handle and return
    information about the first file whose name matchs the specified
    pattern.  If the fSearchOp is FindExSearchNameMatch, then that is
    the extent of the filtering, and lpSearchFilter MUST be NULL.
    Otherwise, additional subfiltering is done depending on this value.

        FindExSearchLimitToDirectories - If this search op is specified,
            then lpSearchFilter MUST be NULL.  For each file that
            matches the specified filename, and that is a directory, and
            entry for that file is returned.

            If the underlying file/io system does not support this type
            of filtering, the API will fail with ERROR_NOT_SUPPORTED,
            and the application will have to perform its own filtering
            by calling this API with FindExSearchNameMatch.

        FindExSearchLimitToDevices - If this search op is specified, the
            lpFileName MUST be *, and FIND_FIRST_EX_CASE_SENSITIVE
            must NOT be specified.  Only device names are returned.
            Device names are generally accessible through
            \\.\name-of-device naming.

    The data returned by this API is dependent on the fInfoLevelId.

        FindExInfoStandard - The lpFindFileData pointer is the standard
            LPWIN32_FIND_DATA structure.

        At this time, no other information levels are supported


    Once established, the find file handle can be used to search for
    other files that match the same pattern with the same filtering
    being performed.  When the find file handle is no longer needed, it
    should be closed.

    Note that while this interface only returns information for a single
    file, an implementation is free to buffer several matching files
    that can be used to satisfy subsequent calls to FindNextFileEx.

    This API is a complete superset of existing FindFirstFile. FindFirstFile
    could be coded as the following macro:

#define FindFirstFile(a,b)
    FindFirstFileEx((a),FindExInfoStandard,(b),FindExSearchNameMatch,NULL,0);


Arguments:

    lpFileName - Supplies the file name of the file to find.  The file name
        may contain the DOS wild card characters '*' and '?'.

    fInfoLevelId - Supplies the info level of the returned data.

    lpFindFileData - Supplies a pointer whose type is dependent on the value
        of fInfoLevelId. This buffer returns the appropriate file data.

    fSearchOp - Specified the type of filtering to perform above and
        beyond simple wildcard matching.

    lpSearchFilter - If the specified fSearchOp needs structured search
        information, this pointer points to the search criteria.  At
        this point in time, both search ops do not require extended
        search information, so this pointer is NULL.

    dwAdditionalFlags - Supplies additional flag values that control the
        search.  A flag value of FIND_FIRST_EX_CASE_SENSITIVE can be
        used to cause case sensitive searches to occur.  The default is
        case insensitive.

Return Value:

    Not -1 - Returns a find first handle that can be used in a
        subsequent call to FindNextFileEx or FindClose.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

#define FIND_FIRST_EX_INVALID_FLAGS (~FIND_FIRST_EX_CASE_SENSITIVE)
    HANDLE hFindFile;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    UNICODE_STRING PathName;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;
    struct SEARCH_BUFFER {
        FILE_BOTH_DIR_INFORMATION DirInfo;
        WCHAR Names[MAX_PATH];
        } Buffer;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    UNICODE_STRING UnicodeInput;
    PFINDFILE_HANDLE FindFileHandle;
    BOOLEAN EndsInDot;
    LPWIN32_FIND_DATAW FindFileData;
    BOOLEAN StrippedTrailingSlash;

    //
    // check parameters
    //

    if ( fInfoLevelId >= FindExInfoMaxInfoLevel ||
         fSearchOp >= FindExSearchLimitToDevices ||
        dwAdditionalFlags & FIND_FIRST_EX_INVALID_FLAGS ) {
        SetLastError(fSearchOp == FindExSearchLimitToDevices ? ERROR_NOT_SUPPORTED : ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
        }

    FindFileData = (LPWIN32_FIND_DATAW)lpFindFileData;

    RtlInitUnicodeString(&UnicodeInput,lpFileName);

    //
    // Bogus code to workaround ~* problem
    //

    if ( UnicodeInput.Buffer[(UnicodeInput.Length>>1)-1] == (WCHAR)'.' ) {
        EndsInDot = TRUE;
        }
    else {
        EndsInDot = FALSE;
        }


    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &PathName,
                            &FileName.Buffer,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
        }

    FreeBuffer = PathName.Buffer;

    //
    //  If there is a a file portion of this name, determine the length
    //  of the name for a subsequent call to NtQueryDirectoryFile.
    //

    if (FileName.Buffer) {
        FileName.Length =
            PathName.Length - (USHORT)((ULONG_PTR)FileName.Buffer - (ULONG_PTR)PathName.Buffer);
    } else {
        FileName.Length = 0;
        }

    FileName.MaximumLength = FileName.Length;
    if ( RelativeName.RelativeName.Length &&
         RelativeName.RelativeName.Buffer != (PUCHAR)FileName.Buffer ) {

        if (FileName.Buffer) {
            PathName.Length = (USHORT)((ULONG_PTR)FileName.Buffer - (ULONG_PTR)RelativeName.RelativeName.Buffer);
            PathName.MaximumLength = PathName.Length;
            PathName.Buffer = (PWSTR)RelativeName.RelativeName.Buffer;
            }

        }
    else {
        RelativeName.ContainingDirectory = NULL;

        if (FileName.Buffer) {
            PathName.Length = (USHORT)((ULONG_PTR)FileName.Buffer - (ULONG_PTR)PathName.Buffer);
            PathName.MaximumLength = PathName.Length;
            }
        }
    if ( PathName.Buffer[(PathName.Length>>1)-2] != (WCHAR)':' &&
         PathName.Buffer[(PathName.Length>>1)-1] != (WCHAR)'\\'   ) {

        PathName.Length -= sizeof(UNICODE_NULL);
        StrippedTrailingSlash = TRUE;
        }
    else {
        StrippedTrailingSlash = FALSE;
        }

    InitializeObjectAttributes(
        &Obja,
        &PathName,
        dwAdditionalFlags & FIND_FIRST_EX_CASE_SENSITIVE ? 0 : OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // Open the directory for list access
    //

    Status = NtOpenFile(
                &hFindFile,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                );

    if ( (Status == STATUS_INVALID_PARAMETER ||
          Status == STATUS_NOT_A_DIRECTORY) && StrippedTrailingSlash ) {
        //
        // open of a pnp style path failed, so try putting back the trailing slash
        //
        PathName.Length += sizeof(UNICODE_NULL);
        Status = NtOpenFile(
                    &hFindFile,
                    FILE_LIST_DIRECTORY | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                    );
        PathName.Length -= sizeof(UNICODE_NULL);
        }

    if ( !NT_SUCCESS(Status) ) {
        ULONG DeviceNameData;
        UNICODE_STRING DeviceName;

        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

        //
        // The full path does not refer to a directory. This could
        // be a device. Check for a device name.
        //

        if ( DeviceNameData = RtlIsDosDeviceName_U(UnicodeInput.Buffer) ) {
            DeviceName.Length = (USHORT)(DeviceNameData & 0xffff);
            DeviceName.MaximumLength = (USHORT)(DeviceNameData & 0xffff);
            DeviceName.Buffer = (PWSTR)
                ((PUCHAR)UnicodeInput.Buffer + (DeviceNameData >> 16));
            return BaseFindFirstDevice(&DeviceName,FindFileData);
            }

        if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            }
        if ( Status == STATUS_OBJECT_TYPE_MISMATCH ) {
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            }
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }

    //
    // Get an entry
    //

    //
    // If there is no file part, but we are not looking at a device,
    // then bail.
    //

    if ( !FileName.Length ) {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        NtClose(hFindFile);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
        }

    DirectoryInfo = &Buffer.DirInfo;

    //
    //  Special case *.* to * since it is so common.  Otherwise transmogrify
    //  the input name according to the following rules:
    //
    //  - Change all ? to DOS_QM
    //  - Change all . followed by ? or * to DOS_DOT
    //  - Change all * followed by a . into DOS_STAR
    //
    //  These transmogrifications are all done in place.
    //

    if ( (FileName.Length == 6) &&
         (RtlCompareMemory(FileName.Buffer, L"*.*", 6) == 6) ) {

        FileName.Length = 2;

    } else {

        ULONG Index;
        WCHAR *NameChar;

        for ( Index = 0, NameChar = FileName.Buffer;
              Index < FileName.Length/sizeof(WCHAR);
              Index += 1, NameChar += 1) {

            if (Index && (*NameChar == L'.') && (*(NameChar - 1) == L'*')) {

                *(NameChar - 1) = DOS_STAR;
            }

            if ((*NameChar == L'?') || (*NameChar == L'*')) {

                if (*NameChar == L'?') { *NameChar = DOS_QM; }

                if (Index && *(NameChar-1) == L'.') { *(NameChar-1) = DOS_DOT; }
            }
        }

        if (EndsInDot && *(NameChar - 1) == L'*') { *(NameChar-1) = DOS_STAR; }
    }

    Status = NtQueryDirectoryFile(
                hFindFile,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                DirectoryInfo,
                sizeof(Buffer),
                FileBothDirectoryInformation,
                TRUE,
                &FileName,
                FALSE
                );

    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    if ( !NT_SUCCESS(Status) ) {
        NtClose(hFindFile);
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }

    //
    // Attributes are composed of the attributes returned by NT.
    //

    FindFileData->dwFileAttributes = DirectoryInfo->FileAttributes;
    FindFileData->ftCreationTime = *(LPFILETIME)&DirectoryInfo->CreationTime;
    FindFileData->ftLastAccessTime = *(LPFILETIME)&DirectoryInfo->LastAccessTime;
    FindFileData->ftLastWriteTime = *(LPFILETIME)&DirectoryInfo->LastWriteTime;
    FindFileData->nFileSizeHigh = DirectoryInfo->EndOfFile.HighPart;
    FindFileData->nFileSizeLow = DirectoryInfo->EndOfFile.LowPart;

    RtlMoveMemory( FindFileData->cFileName,
                   DirectoryInfo->FileName,
                   DirectoryInfo->FileNameLength );

    FindFileData->cFileName[DirectoryInfo->FileNameLength >> 1] = UNICODE_NULL;

    RtlMoveMemory( FindFileData->cAlternateFileName,
                   DirectoryInfo->ShortName,
                   DirectoryInfo->ShortNameLength );

    FindFileData->cAlternateFileName[DirectoryInfo->ShortNameLength >> 1] = UNICODE_NULL;

    //
    // For NTFS reparse points we return the reparse point data tag in dwReserved0.
    //

    if ( DirectoryInfo->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) {
        FindFileData->dwReserved0 = DirectoryInfo->EaSize;
        }

    FindFileHandle = BasepInitializeFindFileHandle(hFindFile);
    if ( !FindFileHandle ) {
        NtClose(hFindFile);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
        }

    return (HANDLE)FindFileHandle;

}

HANDLE
BaseFindFirstDevice(
    PUNICODE_STRING FileName,
    LPWIN32_FIND_DATAW lpFindFileData
    )

/*++

Routine Description:

    This function is called when find first file encounters a device
    name. This function returns a successful psuedo file handle and
    fills in the find file data with all zeros and the devic name.

Arguments:

    FileName - Supplies the device name of the file to find.

    lpFindFileData - On a successful find, this parameter returns information
        about the located file.

Return Value:

    Always returns a static find file handle value of
    BASE_FIND_FIRST_DEVICE_HANDLE

--*/

{
    RtlZeroMemory(lpFindFileData,sizeof(*lpFindFileData));
    lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
    RtlMoveMemory(
        &lpFindFileData->cFileName[0],
        FileName->Buffer,
        FileName->MaximumLength
        );
    return BASE_FIND_FIRST_DEVICE_HANDLE;
}

HANDLE
APIENTRY
FindFirstChangeNotificationA(
    LPCSTR lpPathName,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter
    )

/*++

Routine Description:

    ANSI thunk to FindFirstChangeNotificationW

--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString,lpPathName);
    Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }
    return ( FindFirstChangeNotificationW(
                (LPCWSTR)Unicode->Buffer,
                bWatchSubtree,
                dwNotifyFilter
                )
            );
}

//
// this is a hack... darrylh, please remove when NT supports null
// buffers to change notify
//

char staticchangebuff[sizeof(FILE_NOTIFY_INFORMATION) + 16];
IO_STATUS_BLOCK staticIoStatusBlock;

HANDLE
APIENTRY
FindFirstChangeNotificationW(
    LPCWSTR lpPathName,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter
    )

/*++

Routine Description:

    This API is used to create a change notification handle and to set
    up the initial change notification filter conditions.

    If successful, this API returns a waitable notification handle.  A
    wait on a notification handle is successful when a change matching
    the filter conditions occurs in the directory or subtree being
    watched.

    Once a change notification object is created and the initial filter
    conditions are set, the appropriate directory or subtree is
    monitored by the system for changes that match the specified filter
    conditions.  When one of these changes occurs, a change notification
    wait is satisfied.  If a change occurs without an outstanding change
    notification request, it is remembered by the system and will
    satisfy the next change notification wait.

    Note that this means that after a call to
    FindFirstChangeNotification is made, the application should wait on
    the notification handle before making another call to
    FindNextChangeNotification.

Arguments:

    lpPathName - Supplies the pathname of the directory to be watched.
        This path must specify the pathname of a directory.

    bWatchSubtree - Supplies a boolean value that if TRUE causes the
        system to monitor the directory tree rooted at the specified
        directory.  A value of FALSE causes the system to monitor only
        the specified directory.

    dwNotifyFilter - Supplies a set of flags that specify the filter
        conditions the system uses to satisfy a change notification
        wait.

        FILE_NOTIFY_CHANGE_FILENAME - Any file name changes that occur
            in a directory or subtree being watched will satisfy a
            change notification wait.  This includes renames, creations,
            and deletes.

        FILE_NOTIFY_CHANGE_DIRNAME - Any directory name changes that occur
            in a directory or subtree being watched will satisfy a
            change notification wait.  This includes directory creations
            and deletions.

        FILE_NOTIFY_CHANGE_ATTRIBUTES - Any attribute changes that occur
            in a directory or subtree being watched will satisfy a
            change notification wait.

        FILE_NOTIFY_CHANGE_SIZE - Any file size changes that occur in a
            directory or subtree being watched will satisfy a change
            notification wait.  File sizes only cause a change when the
            on disk structure is updated.  For systems with extensive
            caching this may only occur when the system cache is
            sufficiently flushed.

        FILE_NOTIFY_CHANGE_LAST_WRITE - Any last write time changes that
            occur in a directory or subtree being watched will satisfy a
            change notification wait.  Last write time change only cause
            a change when the on disk structure is updated.  For systems
            with extensive caching this may only occur when the system
            cache is sufficiently flushed.

        FILE_NOTIFY_CHANGE_SECURITY - Any security descriptor changes
            that occur in a directory or subtree being watched will
            satisfy a change notification wait.

Return Value:

    Not -1 - Returns a find change notification handle.  The handle is a
        waitable handle.  A wait is satisfied when one of the filter
        conditions occur in a directory or subtree being monitored.  The
        handle may also be used in a subsequent call to
        FindNextChangeNotify and in FindCloseChangeNotify.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpPathName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // Open the file
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                );

    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }

    //
    // call change notify
    //

    Status = NtNotifyChangeDirectoryFile(
                Handle,
                NULL,
                NULL,
                NULL,
                &staticIoStatusBlock,
                staticchangebuff,   // should be NULL
                sizeof(staticchangebuff),
                dwNotifyFilter,
                (BOOLEAN)bWatchSubtree
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        NtClose(Handle);
        Handle = INVALID_HANDLE_VALUE;
        }
    return Handle;
}

BOOL
APIENTRY
FindNextChangeNotification(
    HANDLE hChangeHandle
    )

/*++

Routine Description:

    This API is used to request that a change notification handle
    be signaled the next time the system dectects an appropriate
    change.

    If a change occurs prior to this call that would otherwise satisfy
    a change request, it is remembered by the system and will satisfy
    this request.

    Once a successful change notification request has been made, the
    application should wait on the change notification handle to
    pick up the change.

    If an application calls this API with a change request outstanding,

        .
        .
        FindNextChangeNotification(h);
        FindNextChangeNotification(h);
        WaitForSingleObject(h,-1);
        .
        .
    it may miss a change notification.

Arguments:

    hChangeHandle - Supplies a change notification handle created
        using FindFirstChangeNotification.

Return Value:

    TRUE - The change notification request was registered. A wait on the
        change handle should be issued to pick up the change notification.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    BOOL ReturnValue;

    ReturnValue = TRUE;
    //
    // call change notify
    //

    Status = NtNotifyChangeDirectoryFile(
                hChangeHandle,
                NULL,
                NULL,
                NULL,
                &staticIoStatusBlock,
                staticchangebuff,           // should be NULL
                sizeof(staticchangebuff),
                FILE_NOTIFY_CHANGE_NAME,    // not needed bug workaround
                TRUE                        // not needed bug workaround
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
        }
    return ReturnValue;
}




BOOL
APIENTRY
FindCloseChangeNotification(
    HANDLE hChangeHandle
    )

/*++

Routine Description:

    This API is used close a change notification handle and to tell the
    system to stop monitoring changes on the notification handle.

Arguments:

    hChangeHandle - Supplies a change notification handle created
        using FindFirstChangeNotification.

Return Value:

    TRUE - The change notification handle was closed.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    return CloseHandle(hChangeHandle);
}

VOID
WINAPI
BasepIoCompletion(
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    DWORD Reserved
    );

BOOL
WINAPI
ReadDirectoryChangesW(
    HANDLE hDirectory,
    LPVOID lpBuffer,
    DWORD nBufferLength,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )

/*++

Routine Description:

    This rountine allows you to read changes that occur in a directory
    or a tree rooted at the specified directory.  It is similar to the
    FindxxxChangeNotification family of APIs, but this API can return
    structured data describing the changes occuring within a directory.

    This API requires the caller to pass in an open directory handle to
    the directory that is to be read.  The handle must be opened with
    FILE_LIST_DIRECTORY acces.  GENERIC_READ includes this and may also
    be used.  The directory may be opened for overlapped access.  This
    technique should be used whenever you call this API asynchronously
    (by specifying and lpOverlapped value).  Opening a directory in
    Win32 is easy.  Use CreateFile, pass in the name of a directory, and
    make sure you specify FILE_FLAG_BACKUP_SEMANTICS.  This will allow
    you to open a directory.  This technique will not force a directory
    to be opened.  It simply allows you to open a directory.  Calling
    this API with a handle to a regular file will fail.

    The following code fragment illustrates how to open a directory using
    CreateFile.

        hDir = CreateFile(
                    DirName,
                    FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | (fASync ? FILE_FLAG_OVERLAPPED : 0),
                    NULL
                    );

    This API returns it's data in a structured format. The structure is defined by
    the FILE_NOTIFY_INFORMATION structure.

        typedef struct _FILE_NOTIFY_INFORMATION {
            DWORD NextEntryOffset;
            DWORD Action;
            DWORD FileNameLength;
            WCHAR FileName[1];
        } FILE_NOTIFY_INFORMATION, *PFILE_NOTIFY_INFORMATION;

    The lpBuffer/nBufferLength parameters are used to describe the
    callers buffer to the system.  This API fills in the buffer either
    syncronously or asynchronously depending on how the directory is
    opened and the presence of the lpOverlapped parameter.

    Upon successful I/O completion, a formated buffer, and number of
    bytes transfered into the buffer is available to the caller.  If the
    number of bytes transfered is 0, this means that the system was
    unable to provide detailed information on all of the changes that
    occured in the directory or tree.  The application should manually
    compute this information by enumerating the directory or tree.
    Otherwise, structured data is returned to the caller.

    Each record contains:

        NextEntryOffest - This is the number of bytes to be skipped to get
            to the next record. A value of 0 indicates that this is the last
            record.

        Action - This is used to describe the type of change that occured:

            FILE_ACTION_ADDED - The file was added to the directory

            FILE_ACTION_REMOVED - The file was removed from the
                directory

            FILE_ACTION_MODIFIED - The file was modified (time change,
                attribute change...)

            FILE_ACTION_RENAMED_OLD_NAME - The file was renamed and this
                is the old name.

            FILE_ACTION_RENAMED_NEW_NAME - The file was renamed and this
                is the new name.

        FileNameLength - This is the length in bytes of the file name portion
            of this record. Note that the file name is NOT null terminated. This
            length does not include a trailing NULL.

        FileName - This variable length portion of the recorn contains a file name
            relative to the directory handle. The name is in the UNICODE character
            format and is NOT NULL terminated.

    The caller of this API can specify a filter that describes to sort
    of changes that should trigger a read completion on thie directory.
    The first call to this API on a directory establishes the filter to
    be used for that call and all subsequent calls.

    The caller can also tell the system to watch for changes in the
    directory, or the entire subtree under the directory.  Again, the
    first call to this API establishes this condition.

    This call can complete either synchronously or asynchronously.

    For synchronous completion, the directory should be opened without
    the FILE_FLAG_OVERLAPPED flag.  The I/O will complete when the
    callers buffer either fills up or overflows.  When this condition
    occurs, the caller may parse the returned buffer.  If the
    *lpBytesReturned value is 0, this means that the buffer was too
    small to hold all of the changes, and the caller will have to
    manually enumerate the directory or tree.

    For asynchronous completion, the directory should be opened with the
    FILE_FLAG_OVERLAPPED flag, and an lpOverlapped parameter must be
    specified.  I/O completion is returned to the caller via
    GetOverlappedResult(), GetQueuedCompletionStatus(), or via an I/O
    completion callback.

    To receive notification via GetOverlappedResult(), DO NOT specify an
    lpCompletionRoutine.  Set the hEvent field of the overlapped
    structure to an hEvent unique to this I/O operation. Pick up your I/O completion
    using GetOverlappedResult().

    To receive notification via GetQueuedCompletionSTatus(), DO NOT
    specify an lpCompletionRoutine.  Associate the directory handle with
    a completion port using CreateIoCompletionPort().  Pick up your I/O
    completion using GetQueuedCompletionStatus().  To disable a
    completion packet from being used on an associated directory, set
    the low order bit of the hEvent in the lpOverlapped structure and
    use GetOverlappedResult().

    To receive notification via an I/O completion callback, DO NOT
    associate the directory with a completion port.  Specify an
    lpCompletionRoutine.  This function will be called whenever an
    outstanding I/O completes while you are in an alertable wait.  If an
    I/O completes, but you are not waiting, the I/O notification stays
    pending and will occur when you wait.  Only the thread that issues
    the I/O is notified. The hEvent field of the overlapped structure is not
    used by the system and may be used by the caller.

Arguments:

    hDirectory - SUpplies an open handle to a directory to be watched.
        The directory must be opened with FILE_LIST_DIRECTORY access.

    lpBuffer - Supplies the address of a buffer that will be used to return the
        results of the read. The format of this buffer is described above.

    nBufferLength - Supplies the length of the buffer.

    bWatchSubtree - Supplies a boolean value that if TRUE causes the
        system to monitor the directory tree rooted at the specified
        directory.  A value of FALSE causes the system to monitor only
        the specified directory.

    dwNotifyFilter - Supplies a set of flags that specify the filter
        conditions the system uses to satisfy a read.

        FILE_NOTIFY_CHANGE_FILENAME - Any file name changes that occur
            in a directory or subtree being watched will satisfy a read.
            This includes renames, creations, and deletes.

        FILE_NOTIFY_CHANGE_DIRNAME - Any directory name changes that
            occur in a directory or subtree being watched will satisfy a
            read.  This includes directory creations and deletions.

        FILE_NOTIFY_CHANGE_ATTRIBUTES - Any attribute changes that occur
            in a directory or subtree being watched will satisfy a
            read.

        FILE_NOTIFY_CHANGE_SIZE - Any file size changes that occur in a
            directory or subtree being watched will satisfy a read.
            File sizes only cause a change when the on disk structure is
            updated.  For systems with extensive caching this may only
            occur when the system cache is sufficiently flushed.

        FILE_NOTIFY_CHANGE_LAST_WRITE - Any last write time changes that
            occur in a directory or subtree being watched will satisfy a
            read.  Last write time change only cause a change when the
            on disk structure is updated.  For systems with extensive
            caching this may only occur when the system cache is
            sufficiently flushed.


        FILE_NOTIFY_CHANGE_LAST_ACCESS - Any last access time changes that
            occur in a directory or subtree being watched will satisfy a
            read.  Last access time change only cause a change when the
            on disk structure is updated.  For systems with extensive
            caching this may only occur when the system cache is
            sufficiently flushed.


        FILE_NOTIFY_CHANGE_CREATION - Any creation time changes that
            occur in a directory or subtree being watched will satisfy a
            read.  Last creation time change only cause a change when the
            on disk structure is updated.  For systems with extensive
            caching this may only occur when the system cache is
            sufficiently flushed.

        FILE_NOTIFY_CHANGE_SECURITY - Any security descriptor changes
            that occur in a directory or subtree being watched will
            satisfy a read.

    lpBytesReturned - For synchronous calls, this returns the number of
        bytes transfered into the buffer.  A successful call coupled
        with a value of 0 means that the buffer was too small, and the
        caller must manually enumerate the directory/tree.  For
        asynchronous calls, this value is undefined.  The system does
        not attempt to store anything here.  The caller must use an
        asynchronous notification technique to pick up I/O completion
        and number of bytes transfered.

    lpOverlapped - Supplies an overlapped structure to be used in
        conjunction with asynchronous I/O completion notification.  The
        offset fields of this structure are not used.  Using this on a
        directory that was not opened with FILE_FLAG_OVERLAPPED is
        undefined.


    lpCompletionRoutine - Supplies the address of a completion routine
        that is called when this I/O completes, AND the thread that
        issues the I/O enters an alertable wait.  The threads wait will
        be interrupted with a return code of WAIT_IO_COMPLETION, and
        this I/O completion routine will be called.  The routine is
        passed the error code of the operation, the number of bytes
        transfered, and the address of the lpOverlapped structure used
        in the call.  An error will occur if this parameter is specified
        on a directory handle that is associated with a completion port.

Return Value:

    TRUE - For synchronous calls, the operation succeeded.
        lpBytesReturned is the number of bytes transferred into your
        buffer.  A value of 0 means that your buffer was too small to
        hold all of the changes that occured and that you need to
        enumerate the directory yourself to see the changes.  For
        asyncronous calls, the operation was queued successfully.
        Results will be delivered using asynch I/O notification
        (GetOverlappedResult(), GetQueuedCompletionStatus(), or your
        completion callback routine).

    FALSE - An error occured. GetLastError() can be used to obtain detailed
        error status.

--*/

{
    NTSTATUS Status;
    BOOL ReturnValue;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Event;
    PIO_APC_ROUTINE ApcRoutine;
    PVOID ApcContext;

    ReturnValue = TRUE;

    if ( ARGUMENT_PRESENT(lpOverlapped) ) {

        if ( ARGUMENT_PRESENT(lpCompletionRoutine) ) {

            //
            // completion is via APC routine
            //

            Event = NULL;
            ApcRoutine = BasepIoCompletion;
            ApcContext = (PVOID) lpCompletionRoutine;

            }
        else {

            //
            // completion is via completion port or get overlapped result
            //

            Event = lpOverlapped->hEvent;
            ApcRoutine = NULL;
            ApcContext = (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped;

            }

        lpOverlapped->Internal = (DWORD)STATUS_PENDING;

        Status = NtNotifyChangeDirectoryFile(
                    hDirectory,
                    Event,
                    ApcRoutine,
                    ApcContext,
                    (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                    lpBuffer,
                    nBufferLength,
                    dwNotifyFilter,
                    (BOOLEAN)bWatchSubtree
                    );

        //
        // Anything other than an error means that I/O completion will
        // occur and caller only gets return data via completion mechanism
        //

        if ( NT_ERROR(Status) ) {
            BaseSetLastNTError(Status);
            ReturnValue = FALSE;
            }
        }
    else {
        Status = NtNotifyChangeDirectoryFile(
                    hDirectory,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    lpBuffer,
                    nBufferLength,
                    dwNotifyFilter,
                    (BOOLEAN)bWatchSubtree
                    );
        if ( Status == STATUS_PENDING) {

            //
            // Operation must complete before return & IoStatusBlock destroyed
            //

            Status = NtWaitForSingleObject( hDirectory, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {
                Status = IoStatusBlock.Status;
                }
            }
        if ( NT_SUCCESS(Status) ) {
            *lpBytesReturned = (DWORD)IoStatusBlock.Information;
            }
        else {
            BaseSetLastNTError(Status);
            ReturnValue = FALSE;
            }
        }

    return ReturnValue;

}
