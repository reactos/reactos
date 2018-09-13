/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    filemisc.c

Abstract:

    Misc file operations for Win32

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include <basedll.h>


DWORD
BasepGetComputerNameFromNtPath (
    PUNICODE_STRING NtPathName,
    HANDLE hFile,
    LPWSTR lpBuffer,
    LPDWORD nSize
    );

NTSTATUS
BasepMoveFileDelayed(
    IN PUNICODE_STRING OldFileName,
    IN PUNICODE_STRING NewFileName,
    IN ULONG Index,
    IN BOOL OkayToCreateNewValue
    );

BOOL
APIENTRY
SetFileAttributesA(
    LPCSTR lpFileName,
    DWORD dwFileAttributes
    )

/*++

Routine Description:

    ANSI thunk to SetFileAttributesW

--*/

{
    PUNICODE_STRING Unicode;

    Unicode = Basep8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return FALSE;
    }

    return ( SetFileAttributesW(
                (LPCWSTR)Unicode->Buffer,
                dwFileAttributes
                )
            );
}


BOOL
APIENTRY
SetFileAttributesW(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    )

/*++

Routine Description:

    The attributes of a file can be set using SetFileAttributes.

    This API provides the same functionality as DOS (int 21h, function
    43H with AL=1), and provides a subset of OS/2's DosSetFileInfo.

Arguments:

    lpFileName - Supplies the file name of the file whose attributes are to
        be set.

    dwFileAttributes - Specifies the file attributes to be set for the
        file.  Any combination of flags is acceptable except that all
        other flags override the normal file attribute,
        FILE_ATTRIBUTE_NORMAL.

        FileAttributes Flags:

        FILE_ATTRIBUTE_NORMAL - A normal file should be created.

        FILE_ATTRIBUTE_READONLY - A read-only file should be created.

        FILE_ATTRIBUTE_HIDDEN - A hidden file should be created.

        FILE_ATTRIBUTE_SYSTEM - A system file should be created.

        FILE_ATTRIBUTE_ARCHIVE - The file should be marked so that it
            will be archived.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicInfo;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
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
    // Open the file inhibiting the reparse behavior.
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
                );

    if ( !NT_SUCCESS(Status) ) {
        //
        // Back level file systems may not support reparse points.
        // We infer this is the case when the Status is STATUS_INVALID_PARAMETER.
        //

        if ( Status == STATUS_INVALID_PARAMETER ) {
            //
            // Open the file without inhibiting the reparse behavior.
            //

            Status = NtOpenFile(
                        &Handle,
                        (ACCESS_MASK)FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                        &Obja,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                        );

            if ( !NT_SUCCESS(Status) ) {
                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                BaseSetLastNTError(Status);
                return FALSE;
                }
            }
        else {
            RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

    //
    // Set the attributes
    //

    RtlZeroMemory(&BasicInfo,sizeof(BasicInfo));
    BasicInfo.FileAttributes = (dwFileAttributes & FILE_ATTRIBUTE_VALID_SET_FLAGS) | FILE_ATTRIBUTE_NORMAL;

    Status = NtSetInformationFile(
                Handle,
                &IoStatusBlock,
                &BasicInfo,
                sizeof(BasicInfo),
                FileBasicInformation
                );

    NtClose(Handle);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}



DWORD
APIENTRY
GetFileAttributesA(
    LPCSTR lpFileName
    )

/*++

Routine Description:

    ANSI thunk to GetFileAttributesW

--*/

{

    PUNICODE_STRING Unicode;

    Unicode = Basep8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return (DWORD)-1;
    }

    return ( GetFileAttributesW((LPCWSTR)Unicode->Buffer) );
}

DWORD
APIENTRY
GetFileAttributesW(
    LPCWSTR lpFileName
    )

/*++

Routine Description:

    The attributes of a file can be obtained using GetFileAttributes.

    This API provides the same functionality as DOS (int 21h, function
    43H with AL=0), and provides a subset of OS/2's DosQueryFileInfo.

Arguments:

    lpFileName - Supplies the file name of the file whose attributes are to
        be set.

Return Value:

    Not -1 - Returns the attributes of the specified file.  Valid
        returned attributes are:

        FILE_ATTRIBUTE_NORMAL - The file is a normal file.

        FILE_ATTRIBUTE_READONLY - The file is marked read-only.

        FILE_ATTRIBUTE_HIDDEN - The file is marked as hidden.

        FILE_ATTRIBUTE_SYSTEM - The file is marked as a system file.

        FILE_ATTRIBUTE_ARCHIVE - The file is marked for archive.

        FILE_ATTRIBUTE_DIRECTORY - The file is marked as a directory.

        FILE_ATTRIBUTE_REPARSE_POINT - The file is marked as a reparse point.

        FILE_ATTRIBUTE_VOLUME_LABEL - The file is marked as a volume lable.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    FILE_BASIC_INFORMATION BasicInfo;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return (DWORD)-1;
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

    Status = NtQueryAttributesFile(
                 &Obja,
                 &BasicInfo
                 );
    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    if ( NT_SUCCESS(Status) ) {
        return BasicInfo.FileAttributes;
        }
    else {

        //
        // Check for a device name.
        //

        if ( RtlIsDosDeviceName_U((PWSTR)lpFileName) ) {
            return FILE_ATTRIBUTE_ARCHIVE;
            }
        BaseSetLastNTError(Status);
        return (DWORD)-1;
        }
}

BOOL
APIENTRY
GetFileAttributesExA(
    LPCSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation
    )

/*++

Routine Description:

    ANSI thunk to GetFileAttributesExW

--*/

{

    PUNICODE_STRING Unicode;

    Unicode = Basep8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return FALSE;
    }

    return ( GetFileAttributesExW((LPCWSTR)Unicode->Buffer,fInfoLevelId,lpFileInformation) );
}

BOOL
APIENTRY
GetFileAttributesExW(
    LPCWSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation
    )

/*++

Routine Description:

    The main attributes of a file can be obtained using GetFileAttributesEx.

Arguments:

    lpFileName - Supplies the file name of the file whose attributes are to
        be set.

    fInfoLevelId - Supplies the info level indicating the information to be
        returned about the file.

    lpFileInformation - Supplies a buffer to receive the specified information
        about the file.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    FILE_NETWORK_OPEN_INFORMATION NetworkInfo;
    LPWIN32_FILE_ATTRIBUTE_DATA AttributeData;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    //
    // Check the parameters.  Note that for now there is only one info level,
    // so there's no special code here to determine what to do.
    //

    if ( fInfoLevelId >= GetFileExMaxInfoLevel || fInfoLevelId < GetFileExInfoStandard ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        }

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
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
    // Query the information about the file using the path-based NT service.
    //

    Status = NtQueryFullAttributesFile( &Obja, &NetworkInfo );
    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    if ( NT_SUCCESS(Status) ) {
        AttributeData = (LPWIN32_FILE_ATTRIBUTE_DATA)lpFileInformation;
        AttributeData->dwFileAttributes = NetworkInfo.FileAttributes;
        AttributeData->ftCreationTime = *(PFILETIME)&NetworkInfo.CreationTime;
        AttributeData->ftLastAccessTime = *(PFILETIME)&NetworkInfo.LastAccessTime;
        AttributeData->ftLastWriteTime = *(PFILETIME)&NetworkInfo.LastWriteTime;
        AttributeData->nFileSizeHigh = NetworkInfo.EndOfFile.HighPart;
        AttributeData->nFileSizeLow = (DWORD)NetworkInfo.EndOfFile.LowPart;
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
APIENTRY
DeleteFileA(
    LPCSTR lpFileName
    )

/*++

Routine Description:

    ANSI thunk to DeleteFileW

--*/

{
    PUNICODE_STRING Unicode;

    Unicode = Basep8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return FALSE;
    }

    return ( DeleteFileW((LPCWSTR)Unicode->Buffer) );
}

BOOL
APIENTRY
DeleteFileW(
    LPCWSTR lpFileName
    )

/*++

    Routine Description:

    An existing file can be deleted using DeleteFile.

    This API provides the same functionality as DOS (int 21h, function 41H)
    and OS/2's DosDelete.

Arguments:

    lpFileName - Supplies the file name of the file to be deleted.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_DISPOSITION_INFORMATION Disposition;
    FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    BOOLEAN fIsSymbolicLink = FALSE;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
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
    // Open the file for delete access.
    // Inhibit the reparse behavior using FILE_OPEN_REPARSE_POINT.
    //

    Status = NtOpenFile(
                 &Handle,
                 (ACCESS_MASK)DELETE | FILE_READ_ATTRIBUTES,
                 &Obja,
                 &IoStatusBlock,
                 FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
                 );
    if ( !NT_SUCCESS(Status) ) {
        //
        // Back level file systems may not support reparse points and thus not
        // support symbolic links.
        // We infer this is the case when the Status is STATUS_INVALID_PARAMETER.
        //

        if ( Status == STATUS_INVALID_PARAMETER ) {
            //
            // Open without inhibiting the reparse behavior and not needing to
            // read the attributes.
            //

            Status = NtOpenFile(
                         &Handle,
                         (ACCESS_MASK)DELETE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                         );
            if ( !NT_SUCCESS(Status) ) {
                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                BaseSetLastNTError(Status);
                return FALSE;
                }
            }
        else {
            //
            // A second case of interest is when the caller does not have rights 
            // to read attributes yet it does have rights to delete the file.
            // In this case Status is to be STATUS_ACCESS_DENIED.
            //
            
            if ( Status != STATUS_ACCESS_DENIED ) {
                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                BaseSetLastNTError(Status);
                return FALSE;
                }
            
            // 
            // Re-open inhibiting reparse point and not requiring read attributes.
            //

            Status = NtOpenFile(
                         &Handle,
                         (ACCESS_MASK)DELETE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
                         );
            if ( !NT_SUCCESS(Status) ) {
                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                BaseSetLastNTError(Status);
                return FALSE;
                }

            //
            // If we are here, Handle is valid.
            //
            // Moreover, Handle is to a file for which the caller has DELETE right yet
            // does not have FILE_READ_ATTRIBUTES rights. 
            //
            // The underlying file may or not be a reparse point. 
            // As the caller does not have rights to read the attributes this code
            // will delete this file without giving the opportunity to the 
            // appropriate manager of these reparse points to clean-up its internal 
            // state at this time.
            //
            }
        }
    else {
        //
        // If we found a reparse point that is not a symbolic link, we re-open
        // without inhibiting the reparse behavior.
        //

        Status = NtQueryInformationFile(
                     Handle,
                     &IoStatusBlock,
                     (PVOID) &FileTagInformation,
                     sizeof(FileTagInformation),
                     FileAttributeTagInformation
                     );
        if ( !NT_SUCCESS(Status) ) {
            //
            // Not all File Systems implement all information classes.
            // The value STATUS_INVALID_PARAMETER is returned when a non-supported
            // information class is requested to a back-level File System. As all the
            // parameters to NtQueryInformationFile are correct, we can infer that
            // we found a back-level system.
            //
            // If FileAttributeTagInformation is not implemented, we assume that
            // the file at hand is not a reparse point.
            //

            if ( (Status != STATUS_NOT_IMPLEMENTED) &&
                 (Status != STATUS_INVALID_PARAMETER) ) {
                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                NtClose(Handle);
                BaseSetLastNTError(Status);
                return FALSE;
                }
            }

        if ( NT_SUCCESS(Status) &&
             (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ) {
            if ( FileTagInformation.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT ) {
                fIsSymbolicLink = TRUE;
                }
            }

        if ( NT_SUCCESS(Status) &&
             (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
             !fIsSymbolicLink) {
            //
            // Re-open without inhibiting the reparse behavior and not needing to
            // read the attributes.
            //

            NtClose(Handle);
            Status = NtOpenFile(
                         &Handle,
                         (ACCESS_MASK)DELETE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                         );

            if ( !NT_SUCCESS(Status) ) {
                //
                // When the FS Filter is absent, delete it any way.
                //

                if ( Status == STATUS_IO_REPARSE_TAG_NOT_HANDLED ) {
                    //
                    // We re-open (possible 3rd open) for delete access inhibiting the reparse behavior.
                    //

                    Status = NtOpenFile(
                                 &Handle,
                                 (ACCESS_MASK)DELETE,
                                 &Obja,
                                 &IoStatusBlock,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
                                 );
                    }

                if ( !NT_SUCCESS(Status) ) {
                    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                    BaseSetLastNTError(Status);
                    return FALSE;
                    }
                }
            }
        }

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

    //
    // Delete the file
    //
#undef DeleteFile
    Disposition.DeleteFile = TRUE;

    Status = NtSetInformationFile(
                 Handle,
                 &IoStatusBlock,
                 &Disposition,
                 sizeof(Disposition),
                 FileDispositionInformation
                 );

    NtClose(Handle);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}


//
//  Ascii versions that thunk to the common code
//

BOOL
APIENTRY
MoveFileA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    )
{
    return MoveFileWithProgressA( lpExistingFileName,
                                  lpNewFileName,
                                  (LPPROGRESS_ROUTINE)NULL,
                                  NULL,
                                  MOVEFILE_COPY_ALLOWED );
}

BOOL
APIENTRY
MoveFileExA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    DWORD dwFlags
    )

{
    return MoveFileWithProgressA( lpExistingFileName,
                                  lpNewFileName,
                                  (LPPROGRESS_ROUTINE)NULL,
                                  NULL,
                                  dwFlags );
}


BOOL
APIENTRY
MoveFileWithProgressA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID lpData OPTIONAL,
    DWORD dwFlags
    )

/*++

Routine Description:

    ANSI thunk to MoveFileWithProgressW

--*/

{

    UNICODE_STRING UnicodeOldFileName;
    UNICODE_STRING UnicodeNewFileName;
    BOOL ReturnValue;

    if ( !Basep8BitStringToDynamicUnicodeString(&UnicodeOldFileName, lpExistingFileName) ) {
        return FALSE;
        }

    if ( ARGUMENT_PRESENT(lpNewFileName) ) {
        if ( !Basep8BitStringToDynamicUnicodeString(&UnicodeNewFileName, lpNewFileName) ) {
            RtlFreeUnicodeString(&UnicodeOldFileName);
            return FALSE;
            }
        }
    else {
        UnicodeNewFileName.Buffer = NULL;
        }

    ReturnValue =
        MoveFileWithProgressW( (LPCWSTR)UnicodeOldFileName.Buffer,
                               (LPCWSTR)UnicodeNewFileName.Buffer,
                               lpProgressRoutine,
                               lpData,
                               dwFlags
                               );

    RtlFreeUnicodeString(&UnicodeOldFileName);
    RtlFreeUnicodeString(&UnicodeNewFileName);

    return ReturnValue;
}

typedef struct _HELPER_CONTEXT {
    DWORD dwFlags;
    LPPROGRESS_ROUTINE lpProgressRoutine;
    LPVOID lpData;
} HELPER_CONTEXT, *PHELPER_CONTEXT;

DWORD
APIENTRY
BasepMoveFileCopyProgress(
    LARGE_INTEGER TotalFileSize,
    LARGE_INTEGER TotalBytesTransferred,
    LARGE_INTEGER StreamSize,
    LARGE_INTEGER StreamBytesTransferred,
    DWORD dwStreamNumber,
    DWORD dwCallbackReason,
    HANDLE SourceFile,
    HANDLE DestinationFile,
    LPVOID lpData OPTIONAL
    )
/*++

Routine Description:

    Perform special actions when doing move-by-copy.

Arguments:

    TotalFileSize - total number of bytes being transferred

    TotalBytesTransferred - current progress through the file

    StreamSize - total number of bytes being transferred in this stream

    StreamBytesTransferred - current progress through this stream

    dwStreamNumber - ordinal number of stream

    dwCallbackReason - CopyFile's reason for calling us

    SourceFile - source handle of transfer

    DestinationFile - destination handle of transfer

    lpData - pointer to HELPER_CONTEXT constructed by MoveFileWithProgressW.


Return Value:

    PROGRESS_CONTINUE if no progress routine was specified, otherwise
        the return value from the progress routine specified to
        MoveFileWithProgress

--*/

{
    PHELPER_CONTEXT Context = (PHELPER_CONTEXT)lpData;

    //
    //  If we are finished with a stream and the caller
    //  specified WRITE_THROUGH then we make sure the file buffers
    //  actually made it out to disk.
    //

    if ((Context->dwFlags & MOVEFILE_WRITE_THROUGH) != 0
        && dwCallbackReason == CALLBACK_CHUNK_FINISHED
        && StreamBytesTransferred.QuadPart == StreamSize.QuadPart ) {

        FlushFileBuffers(DestinationFile);

    }


    //
    //  If a callback routine was specified, call through him
    //

    if (Context->lpProgressRoutine == NULL) {
        return PROGRESS_CONTINUE;
    }

    return (Context->lpProgressRoutine) (
                TotalFileSize,
                TotalBytesTransferred,
                StreamSize,
                StreamBytesTransferred,
                dwStreamNumber,
                dwCallbackReason,
                SourceFile,
                DestinationFile,
                Context->lpData );
}



NTSTATUS
BasepNotifyTrackingService( PHANDLE SourceFile,
                            POBJECT_ATTRIBUTES SourceFileObjAttributes,
                            HANDLE DestFile,
                            PUNICODE_STRING NewFileName
                            )
{
    NTSTATUS Status = STATUS_SUCCESS;
    FILE_BASIC_INFORMATION BasicInformation;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG FileAttributes;
    ULONG cchComputerName;
    WCHAR ComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD dwError;

    BYTE FTIBuffer[ sizeof(FILE_TRACKING_INFORMATION) + MAX_COMPUTERNAME_LENGTH + 1 ];
    PFILE_TRACKING_INFORMATION pfti = (PFILE_TRACKING_INFORMATION) &FTIBuffer[0];

    try
    {
        cchComputerName = MAX_COMPUTERNAME_LENGTH + 1;
        dwError = BasepGetComputerNameFromNtPath( NewFileName,
                                                  DestFile,
                                                  ComputerName,
                                                  &cchComputerName );

        if (ERROR_SUCCESS != dwError) {
            pfti->ObjectInformationLength = 0;
        } else {
            
            CHAR ComputerNameOemBuffer[ MAX_PATH ];
            OEM_STRING ComputerNameOemString = { 0,
                                                 sizeof(ComputerNameOemBuffer),
                                                 ComputerNameOemBuffer };
            UNICODE_STRING ComputerNameUnicodeString;

            RtlInitUnicodeString( &ComputerNameUnicodeString,
                                  ComputerName );


            Status = RtlUnicodeStringToOemString( &ComputerNameOemString,
                                                  &ComputerNameUnicodeString,
                                                  FALSE );  // Don't allocate
            if( !NT_SUCCESS(Status) ) {
                leave;
            }

            memcpy( pfti->ObjectInformation,
                    ComputerNameOemString.Buffer,
                    ComputerNameOemString.Length );
            pfti->ObjectInformation[ ComputerNameOemString.Length ] = '\0';
                
            // Fill in the rest of the fti buffer, and set the file information

            pfti->ObjectInformationLength = ComputerNameOemString.Length + 1;
        }

        pfti->DestinationFile = DestFile;

        Status = NtSetInformationFile(
                                     *SourceFile,
                                     &IoStatusBlock,
                                     pfti,
                                     sizeof( FTIBuffer ),
                                     FileTrackingInformation );

        //
        // Check to see if tracking failed because
        // the source has a read-only attribute set.
        //

        if (Status != STATUS_ACCESS_DENIED) {
            leave;
        }

        //
        // reopen the source file and reset the read-only attribute
        // so that we'll be able to open for write access.
        //

        CloseHandle(*SourceFile);

        Status = NtOpenFile(
                           SourceFile,
                           SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                           SourceFileObjAttributes,
                           &IoStatusBlock,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_SYNCHRONOUS_IO_NONALERT
                           );

        if (!NT_SUCCESS(Status)) {
            *SourceFile = INVALID_HANDLE_VALUE;
            leave;
        }


        Status = NtQueryInformationFile(
                                       *SourceFile,
                                       &IoStatusBlock,
                                       &BasicInformation,
                                       sizeof(BasicInformation),
                                       FileBasicInformation );

        if (!NT_SUCCESS(Status)) {
            leave;
        }

        //
        // Reset the r/o bit and write the attributes back.
        //

        FileAttributes = BasicInformation.FileAttributes;
        RtlZeroMemory(&BasicInformation, sizeof(BasicInformation));
        BasicInformation.FileAttributes = FileAttributes & ~FILE_ATTRIBUTE_READONLY;

        Status = NtSetInformationFile(
                                     *SourceFile,
                                     &IoStatusBlock,
                                     &BasicInformation,
                                     sizeof(BasicInformation),
                                     FileBasicInformation);

        if (!NT_SUCCESS(Status)) {

            //
            // If this fails, we can't track the file.
            //

            leave;
        }

        //
        // Now that the r/o bit is reset, reopen for write access and
        // retry the tracking notification.
        //

        else {
            HANDLE hSourceRw;

            Status = NtOpenFile(
                               &hSourceRw,
                               SYNCHRONIZE | GENERIC_WRITE,
                               SourceFileObjAttributes,
                               &IoStatusBlock,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               FILE_SYNCHRONOUS_IO_NONALERT
                               );

            if (NT_SUCCESS(Status)) {
                NtClose(*SourceFile);
                *SourceFile = hSourceRw;

                //
                // Send the source machine a tracking notification.
                //

                Status = NtSetInformationFile( *SourceFile,
                                               &IoStatusBlock,
                                               pfti,
                                               sizeof( FTIBuffer ),
                                               FileTrackingInformation );
            }
        }


        if (!NT_SUCCESS(Status)) {

            //
            // Try to put back the r/o bit -- don't assign Status here
            // because we want to fail. BUGBUG: If we crash here, we may leave
            // the r/o attribute clear when it should be set.
            //

            BasicInformation.FileAttributes |= FILE_ATTRIBUTE_READONLY;
            NtSetInformationFile(
                                *SourceFile,
                                &IoStatusBlock,
                                &BasicInformation,
                                sizeof(BasicInformation),
                                FileBasicInformation);
        }
    }
    finally
    {
    }


    return( Status );

}





BOOL
APIENTRY
MoveFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    )
{
    return MoveFileWithProgressW( lpExistingFileName,
                                  lpNewFileName,
                                  (LPPROGRESS_ROUTINE)NULL,
                                  NULL,
                                  MOVEFILE_COPY_ALLOWED );
}

BOOL
APIENTRY
MoveFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags
    )

{
    return MoveFileWithProgressW( lpExistingFileName,
                                  lpNewFileName,
                                  (LPPROGRESS_ROUTINE)NULL,
                                  NULL,
                                  dwFlags );
}

BOOL
APIENTRY
MoveFileWithProgressW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    LPVOID lpData OPTIONAL,
    DWORD dwFlags
    )

/*++

Routine Description:

    An existing file can be renamed using MoveFileWithProgressW.

Arguments:

    lpExistingFileName - Supplies the name of an existing file that is to be
        renamed.

    lpNewFileName - Supplies the new name for the existing file.  The new
        name must reside in the same file system/drive as the existing
        file and must not already exist.

    lpProgressRoutine - Supplies a callback routine that is notified.

    lpData - Supplies context data passed to the progress routine.

    dwFlags - Supplies optional flag bits to control the behavior of the
        rename.  The following bits are currently defined:

        MOVEFILE_REPLACE_EXISTING - if the new file name exists, replace
            it by renaming the old file name on top of the new file name.

        MOVEFILE_COPY_ALLOWED - if the new file name is on a different
            volume than the old file name, and causes the rename operation
            to fail, then setting this flag allows the MoveFileEx API
            call to simulate the rename with a call to CopyFile followed
            by a call to DeleteFile to the delete the old file if the
            CopyFile was successful.

        MOVEFILE_DELAY_UNTIL_REBOOT - dont actually do the rename now, but
            instead queue the rename so that it will happen the next time
            the system boots.  If this flag is set, then the lpNewFileName
            parameter may be NULL, in which case a delay DeleteFile of
            the old file name will occur the next time the system is
            booted.

            The delay rename/delete operations occur immediately after
            AUTOCHK is run, but prior to creating any paging files, so
            it can be used to delete paging files from previous boots
            before they are reused.

        MOVEFILE_WRITE_THROUGH - perform the rename operation in such a
            way that the file has actually been moved on the disk before
            the API returns to the caller.  Note that this flag causes a
            flush at the end of a copy operation (if one were allowed and
            necessary), and has no effect if the rename operation is
            delayed until the next reboot.

        MOVEFILE_CREATE_HARDLINK - create a hard link from the new file name to
            the existing file name.  May not be specified with
            MOVEFILE_DELAY_UNTIL_REBOOT

        MOVEFILE_FAIL_IF_NOT_TRACKABLE - fail the move request if the file cannot
            be tracked.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    BOOLEAN ReplaceIfExists;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle = INVALID_HANDLE_VALUE;
    UNICODE_STRING OldFileName;
    UNICODE_STRING NewFileName;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_RENAME_INFORMATION NewName;
    FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    UNICODE_STRING RelativeOldName;
    ULONG OpenFlags;
    BOOLEAN b = FALSE;
    HELPER_CONTEXT Context;

    NewFileName.Buffer = NULL;
    OldFileName.Buffer = NULL;

    try {

        //
        // if the target is a device, do not allow the rename !
        //

        if ( lpNewFileName ) {
            if ( RtlIsDosDeviceName_U((PWSTR)lpNewFileName) ) {
                BaseSetLastNTError( STATUS_OBJECT_NAME_COLLISION );
                leave;
            }
        }

        ReplaceIfExists = (dwFlags & MOVEFILE_REPLACE_EXISTING) != 0;

        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                lpExistingFileName,
                                &OldFileName,
                                NULL,
                                &RelativeName
                                );

        if ( !TranslationStatus ) {
            BaseSetLastNTError( STATUS_OBJECT_PATH_NOT_FOUND );
            leave;
        }

        //
        //  Cannot perform delayed-move-by-creating-hardlink
        //

        if ((dwFlags & MOVEFILE_DELAY_UNTIL_REBOOT) != 0 &&
            (dwFlags & MOVEFILE_CREATE_HARDLINK) != 0) {
            BaseSetLastNTError( STATUS_INVALID_PARAMETER );
            leave;
        }

        //
        //  Get a handle to the source of the move.  We do this even for
        //  the delayed move in order to validate that we have delete
        //  access to the file.
        //

        if ( RelativeName.RelativeName.Length ) {
            RelativeOldName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        } else {
            RelativeOldName = OldFileName;
            RelativeName.ContainingDirectory = NULL;
        }

        InitializeObjectAttributes(
                                  &Obja,
                                  &RelativeOldName,
                                  OBJ_CASE_INSENSITIVE,
                                  RelativeName.ContainingDirectory,
                                  NULL
                                  );

        //
        //  Establish whether we are renaming a symbolic link or not by:
        //      (1) obtaining a handle to the local entity, and
        //      (2) finding whether a symbolic link was found.
        //
        //  Open the file for delete access inhibiting the reparse
        //  point behavior.
        //

        OpenFlags = FILE_SYNCHRONOUS_IO_NONALERT |
                    FILE_OPEN_FOR_BACKUP_INTENT  |
                    ((dwFlags & MOVEFILE_WRITE_THROUGH) ? FILE_WRITE_THROUGH : 0);

        Status = NtOpenFile( &Handle,
                             FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
                             &Obja,
                             &IoStatusBlock,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             FILE_OPEN_REPARSE_POINT | OpenFlags
                             );


        if (!NT_SUCCESS( Status )) {

            //
            //  The Open may fail for a number of reasons.  If we're
            //  delaying the operation until reboot, it doesn't matter
            //  if we get a sharing violation or a non-existent file
            //  or a non-existent path.
            //

            if (((dwFlags & MOVEFILE_DELAY_UNTIL_REBOOT) != 0)
                && (Status == STATUS_SHARING_VIOLATION
                    || Status == STATUS_OBJECT_NAME_NOT_FOUND
                    || Status == STATUS_OBJECT_PATH_NOT_FOUND)) {

                Handle = INVALID_HANDLE_VALUE;

            } else {

                //
                //  Back level file systems may not support reparse points and thus not
                //  support symbolic links.
                //
                //  We infer this is the case when the Status is STATUS_INVALID_PARAMETER.
                //

                if ( Status == STATUS_INVALID_PARAMETER ) {

                    //
                    //  Retry the open without reparse behaviour.  This should be compatible
                    //  with older file systems.
                    //

                    Status = NtOpenFile(
                                       &Handle,
                                       DELETE | SYNCHRONIZE,
                                       &Obja,
                                       &IoStatusBlock,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       OpenFlags
                                       );
                }

                if ( !NT_SUCCESS( Status ) ) {
                    BaseSetLastNTError( Status );
                    leave;
                }
            }
        } else {

            //
            //  The open succeeded. If we do not find a symbolic link or a mount point,
            //  re-open without inhibiting the reparse behavior.
            //

            Status = NtQueryInformationFile(
                                           Handle,
                                           &IoStatusBlock,
                                           (PVOID) &FileTagInformation,
                                           sizeof(FileTagInformation),
                                           FileAttributeTagInformation
                                           );

            if ( !NT_SUCCESS( Status ) ) {

                //
                //  Not all File Systems implement all information classes.
                //  The value STATUS_INVALID_PARAMETER is returned when a non-supported
                //  information class is requested to a back-level File System. As all the
                //  parameters to NtQueryInformationFile are correct, we can infer that
                //  we found a back-level system.
                //
                //  If FileAttributeTagInformation is not implemented, we assume that
                //  the file at hand is not a reparse point.
                //

                if ( (Status != STATUS_NOT_IMPLEMENTED) &&
                     (Status != STATUS_INVALID_PARAMETER) ) {
                    BaseSetLastNTError( Status );
                    leave;
                }
            }

            if ( NT_SUCCESS(Status) &&
                 (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
                 FileTagInformation.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT ) {

                //
                //  Open without inhibiting the reparse behavior and not needing to
                //  read the attributes.
                //

                NtClose( Handle );
                Handle = INVALID_HANDLE_VALUE;

                Status = NtOpenFile(
                                   &Handle,
                                   DELETE | SYNCHRONIZE,
                                   &Obja,
                                   &IoStatusBlock,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   OpenFlags
                                   );

                if ( !NT_SUCCESS( Status ) ) {
                    BaseSetLastNTError( Status );
                    leave;
                }
            }
        }

        if (!(dwFlags & MOVEFILE_DELAY_UNTIL_REBOOT) ||
            (lpNewFileName != NULL)) {
            TranslationStatus = RtlDosPathNameToNtPathName_U(
                                                            lpNewFileName,
                                                            &NewFileName,
                                                            NULL,
                                                            NULL
                                                            );

            if ( !TranslationStatus ) {
                BaseSetLastNTError( STATUS_OBJECT_PATH_NOT_FOUND );
                leave;
            }

        } else {
            RtlInitUnicodeString( &NewFileName, NULL );
        }

        if (dwFlags & MOVEFILE_DELAY_UNTIL_REBOOT) {

            //
            // (typical stevewo hack, preserved for sentimental value)
            //
            // If ReplaceIfExists is TRUE, prepend an exclamation point
            // to the new filename in order to pass this bit of data
            // along to the session manager.
            //

            if (ReplaceIfExists && NewFileName.Length != 0) {
                PWSTR NewBuffer;

                NewBuffer = RtlAllocateHeap( RtlProcessHeap(),
                                             MAKE_TAG( TMP_TAG ),
                                             NewFileName.Length + sizeof(WCHAR) );
                if (NewBuffer == NULL) {
                    BaseSetLastNTError( STATUS_NO_MEMORY );
                    leave;
                }

                NewBuffer[0] = L'!';
                CopyMemory(&NewBuffer[1], NewFileName.Buffer, NewFileName.Length);
                NewFileName.Length += sizeof(WCHAR);
                NewFileName.MaximumLength += sizeof(WCHAR);
                RtlFreeHeap(RtlProcessHeap(), 0, NewFileName.Buffer);
                NewFileName.Buffer = NewBuffer;
            }

            //
            // Check to see if the existing file is on a remote share. If it
            // is, flag the error rather than let the operation silently fail
            // because the delayed operations are done before the net is
            // available. Rather than open the file and do a hard core file type,
            // we just check for UNC in the file name. This isn't perfect, but it is
            // pretty good. Chances are we can not open and manipulate the file. That is
            // why the caller is using the delay until reboot option !
            //

            if ( RtlDetermineDosPathNameType_U(lpExistingFileName) == RtlPathTypeUncAbsolute ) {
                Status = STATUS_INVALID_PARAMETER;
            }

            //
            // copy allowed is not permitted on delayed renames
            //

            else if ( dwFlags & MOVEFILE_COPY_ALLOWED ) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
	        Status = BasepMoveFileDelayed( &OldFileName,
					       &NewFileName,
					       2,
					       FALSE );
		if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
		    Status = BasepMoveFileDelayed( &OldFileName,
						   &NewFileName,
						   1,
						   TRUE );
		    if (Status == STATUS_INSUFFICIENT_RESOURCES) {
                        Status = BasepMoveFileDelayed( &OldFileName,
						       &NewFileName,
						       2,
						       TRUE );
		    }
		}
            }

            if (!NT_SUCCESS( Status )) {
                BaseSetLastNTError( Status );
                leave;
            }

            b = TRUE;
            leave;
        }

        //
        //  We must to the real move now.
        //

        NewName = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), NewFileName.Length+sizeof(*NewName));

        if (NewName == NULL) {
            BaseSetLastNTError( STATUS_NO_MEMORY );
            leave;
        }

        RtlMoveMemory( NewName->FileName, NewFileName.Buffer, NewFileName.Length );

        NewName->ReplaceIfExists = ReplaceIfExists;
        NewName->RootDirectory = NULL;
        NewName->FileNameLength = NewFileName.Length;

        Status = NtSetInformationFile(
                    Handle,
                    &IoStatusBlock,
                    NewName,
                    NewFileName.Length+sizeof(*NewName),
                    (dwFlags & MOVEFILE_CREATE_HARDLINK) ?
                        FileLinkInformation :
                        FileRenameInformation
                    );

        RtlFreeHeap(RtlProcessHeap(), 0, NewName);

        if (NT_SUCCESS( Status )) {
            b = TRUE;
            leave;
        }

        if (Status != STATUS_NOT_SAME_DEVICE || (dwFlags & MOVEFILE_COPY_ALLOWED) == 0) {
            BaseSetLastNTError( Status );
            leave;
        }

        NtClose( Handle );
        Handle = INVALID_HANDLE_VALUE;

        //
        //  Perform a copy/delete.  Handle link tracking.
        //

        {
            HANDLE hSource = INVALID_HANDLE_VALUE;
            HANDLE hDest = INVALID_HANDLE_VALUE;

            Context.dwFlags = dwFlags;
            Context.lpProgressRoutine = lpProgressRoutine;
            Context.lpData = lpData;

            b = (BOOLEAN)BasepCopyFileExW(
                            lpExistingFileName,
                            lpNewFileName,
                            BasepMoveFileCopyProgress,
                            &Context,
                            NULL,
                            (ReplaceIfExists ? 0 : COPY_FILE_FAIL_IF_EXISTS) | COPY_FILE_OPEN_SOURCE_FOR_WRITE,
                            0, // PrivCopyFile flags
                            &hSource,
                            &hDest
                            );

            if ( b && hSource != INVALID_HANDLE_VALUE && hDest != INVALID_HANDLE_VALUE) {

                //
                // attempt to do tracking
                //

                Status = BasepNotifyTrackingService( &hSource,
                                                     &Obja,
                                                     hDest,
                                                     &NewFileName );


                if ( !NT_SUCCESS(Status) &&
                    (dwFlags & MOVEFILE_FAIL_IF_NOT_TRACKABLE)) {

                    if (hDest != INVALID_HANDLE_VALUE)
                        CloseHandle( hDest );

                    hDest = INVALID_HANDLE_VALUE;
                    DeleteFileW( lpNewFileName );
                    b = FALSE;

                    BaseSetLastNTError( Status );

                }
            }

            if (hSource != INVALID_HANDLE_VALUE) {
                CloseHandle(hSource);
                hSource = INVALID_HANDLE_VALUE;
            }

            if (hDest != INVALID_HANDLE_VALUE) {
                CloseHandle(hDest);
                hDest = INVALID_HANDLE_VALUE;
            }

            //
            // the copy worked... Delete the source of the rename
            // if it fails, try a set attributes and then a delete
            //

            if (b && !DeleteFileW( lpExistingFileName ) ) {

                //
                // If the delete fails, we will return true, but possibly
                // leave the source dangling
                //

                SetFileAttributesW(lpExistingFileName,FILE_ATTRIBUTE_NORMAL);
                DeleteFileW( lpExistingFileName );
            }
        }

    } finally {
        if (Handle != INVALID_HANDLE_VALUE) {
            NtClose( Handle );
        }
        RtlFreeHeap( RtlProcessHeap(), 0, OldFileName.Buffer );
        RtlFreeHeap( RtlProcessHeap(), 0, NewFileName.Buffer );
    }

    return b;
}


NTSTATUS
BasepMoveFileDelayed(
    IN PUNICODE_STRING OldFileName,
    IN PUNICODE_STRING NewFileName,
    IN ULONG Index,
    IN BOOL OkayToCreateNewValue
    )

/*++

Routine Description:

    Appends the given delayed move file operation to the registry
    value that contains the list of move file operations to be
    performed on the next boot.

Arguments:

    OldFileName - Supplies the old file name

    NewFileName - Supplies the new file name

Return Value:

    NTSTATUS

--*/

{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    PWSTR ValueData, s;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    ULONG ValueLength = 1024;
    ULONG ReturnedLength;
    WCHAR ValueNameBuf[64];
    NTSTATUS Status;


    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager" );

    if (Index == 1) {
        RtlInitUnicodeString( &ValueName, L"PendingFileRenameOperations" );
    } else {
        swprintf(ValueNameBuf,L"PendingFileRenameOperations%d",Index);
        RtlInitUnicodeString( &ValueName, ValueNameBuf );
    }

    InitializeObjectAttributes(
        &Obja,
        &KeyName,
        OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtCreateKey( &KeyHandle,
                          GENERIC_READ | GENERIC_WRITE,
                          &Obja,
                          0,
                          NULL,
                          0,
                          NULL
                        );
    if ( Status == STATUS_ACCESS_DENIED ) {
        Status = NtCreateKey( &KeyHandle,
                              GENERIC_READ | GENERIC_WRITE,
                              &Obja,
                              0,
                              NULL,
                              REG_OPTION_BACKUP_RESTORE,
                              NULL
                            );
    }

    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    while (TRUE) {
        ValueInfo = RtlAllocateHeap(RtlProcessHeap(),
                                    MAKE_TAG(TMP_TAG),
                                    ValueLength + OldFileName->Length + sizeof(WCHAR) +
                                                  NewFileName->Length + 2*sizeof(WCHAR));

        if (ValueInfo == NULL) {
            NtClose(KeyHandle);
            return(STATUS_NO_MEMORY);
        }

        //
        // File rename operations are stored in the registry in a
        // single MULTI_SZ value. This allows the renames to be
        // performed in the same order that they were originally
        // requested. Each rename operation consists of a pair of
        // NULL-terminated strings.
        //

        Status = NtQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 ValueInfo,
                                 ValueLength,
                                 &ReturnedLength);

        if (Status != STATUS_BUFFER_OVERFLOW) {
            break;
        }

        //
        // The existing value is too large for our buffer.
        // Retry with a larger buffer.
        //
        ValueLength = ReturnedLength;
        RtlFreeHeap(RtlProcessHeap(), 0, ValueInfo);
    }

    if ((Status == STATUS_OBJECT_NAME_NOT_FOUND)
	&& OkayToCreateNewValue) {
        //
        // The value does not currently exist. Create the
        // value with our data.
        //
        s = ValueData = (PWSTR)ValueInfo;
    } else if (NT_SUCCESS(Status)) {
        //
        // A value already exists, append our two strings to the
        // MULTI_SZ.
        //
        ValueData = (PWSTR)(&ValueInfo->Data);
        s = (PWSTR)((PCHAR)ValueData + ValueInfo->DataLength) - 1;
    } else {
        NtClose(KeyHandle);
        RtlFreeHeap(RtlProcessHeap(), 0, ValueInfo);
        return(Status);
    }

    CopyMemory(s, OldFileName->Buffer, OldFileName->Length);
    s += (OldFileName->Length/sizeof(WCHAR));
    *s++ = L'\0';

    CopyMemory(s, NewFileName->Buffer, NewFileName->Length);
    s += (NewFileName->Length/sizeof(WCHAR));
    *s++ = L'\0';
    *s++ = L'\0';

    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_MULTI_SZ,
                           ValueData,
                           (ULONG)((s-ValueData)*sizeof(WCHAR)));
    NtClose(KeyHandle);
    RtlFreeHeap(RtlProcessHeap(), 0, ValueInfo);

    return(Status);
}




NTSTATUS
BasepOpenFileForMove( IN     LPCWSTR lpFileName,
                      OUT    PUNICODE_STRING FileName,
                      OUT    PVOID *FileNameFreeBuffer,
                      OUT    PHANDLE Handle,
                      OUT    POBJECT_ATTRIBUTES Obja,
                      IN     ULONG DesiredAccess,
                      IN     ULONG ShareAccess,
                      IN     ULONG OpenOptions )
/*++

Routine Description:

    Opens a file such that it may be used in MoveFile or MoveFileIdentity.

Arguments:

    lpFileName - the file to open

    FileName - lpFileName translated to an NT path

    FileNameFreeBuffer - a buffer which needs to be freed when FileName
        is no longer in use

    Handle - Location in which to put the handle for the opened file.

    Obja - Object attributes used to open the file

    DesiredAccess - Access flags which must be set, in addition to
        FILE_READ_ATTRIBUTES and SYNCHRONIZE which may also be set.

    ShareAccess - Sharing flags which must be set, though additional
        flags may also be set.

    OpenOptions - FILE_OPEN_ flags which must be set, though
        FILE_OPEN_REPARSE_POINT, FILE_SYNCHRONOUS_IO_NONALERT, and
        FILE_OPEN_FOR_BACKUP_INTENT may also be set.


Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    IO_STATUS_BLOCK IoStatusBlock;

    try
    {

        FileName->Length = FileName->MaximumLength = 0;
        FileName->Buffer = NULL;
        *FileNameFreeBuffer = NULL;

        //
        //  Initialize the object attributes
        //

        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                lpFileName,
                                FileName,
                                NULL,
                                &RelativeName
                                );

        if ( !TranslationStatus ) {
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            leave;
        }
        *FileNameFreeBuffer = FileName->Buffer;


        if ( RelativeName.RelativeName.Length ) {
            *FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        } else {
            RelativeName.ContainingDirectory = NULL;
        }

        InitializeObjectAttributes(
                                  Obja,
                                  FileName,
                                  OBJ_CASE_INSENSITIVE,
                                  RelativeName.ContainingDirectory,
                                  NULL
                                  );

        //
        //  Establish whether we are handling a symbolic link or not by:
        //      (1) obtaining a handle to the local entity, and
        //      (2) finding whether a symbolic link was found.
        //
        //  Open the file for delete access inhibiting the reparse
        //  point behavior.
        //

        OpenOptions |= (FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);

        Status = NtOpenFile( Handle,
                             FILE_READ_ATTRIBUTES | SYNCHRONIZE | DesiredAccess,
                             Obja,
                             &IoStatusBlock,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | ShareAccess,
                             FILE_OPEN_REPARSE_POINT | OpenOptions
                             );


        if (!NT_SUCCESS( Status )) {

            //
            //  Back level file systems may not support reparse points and thus not
            //  support symbolic links.
            //
            //  We infer this is the case when the Status is STATUS_INVALID_PARAMETER.
            //

            if ( Status == STATUS_INVALID_PARAMETER ) {

                //
                //  Retry the open without reparse behaviour.  This should be compatible
                //  with older file systems.
                //

                Status = NtOpenFile(
                                   Handle,
                                   SYNCHRONIZE | DesiredAccess,
                                   Obja,
                                   &IoStatusBlock,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | ShareAccess,
                                   OpenOptions
                                   );
            }

            if ( !NT_SUCCESS( Status ) ) {

                leave;
            }

        } else {

            FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;

            //
            //  The open succeeded. If we do not find a symbolic link or a mount point,
            //  re-open without inhibiting the reparse behavior.
            //

            Status = NtQueryInformationFile(
                                           *Handle,
                                           &IoStatusBlock,
                                           (PVOID) &FileTagInformation,
                                           sizeof(FileTagInformation),
                                           FileAttributeTagInformation
                                           );

            if ( !NT_SUCCESS( Status ) ) {

                //
                //  Not all File Systems implement all information classes.
                //  The value STATUS_INVALID_PARAMETER is returned when a non-supported
                //  information class is requested to a back-level File System. As all the
                //  parameters to NtQueryInformationFile are correct, we can infer that
                //  we found a back-level system.
                //
                //  If FileAttributeTagInformation is not implemented, we assume that
                //  the file at hand is not a reparse point.
                //

                if ( (Status != STATUS_NOT_IMPLEMENTED) &&
                     (Status != STATUS_INVALID_PARAMETER) ) {

                    leave;
                }
            }

            if ( NT_SUCCESS(Status) &&
                 (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
                 FileTagInformation.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT ) {

                //
                //  Open without inhibiting the reparse behavior and not needing to
                //  read the attributes.
                //

                NtClose( *Handle );
                *Handle = INVALID_HANDLE_VALUE;

                Status = NtOpenFile(
                                   Handle,
                                   SYNCHRONIZE | DesiredAccess,
                                   Obja,
                                   &IoStatusBlock,
                                   FILE_SHARE_DELETE | FILE_SHARE_READ | ShareAccess,
                                   OpenOptions
                                   );

                if ( !NT_SUCCESS( Status ) ) {

                    leave;
                }
            }
        }
    }
    finally
    {
    }

    return( Status );

}



BOOL
APIENTRY
PrivMoveFileIdentityW(
    LPCWSTR lpOldFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags
    )

/*++

Routine Description:

    Moves an identity from one file to another.  The identity is composed
    of the file's create date, and its object ID.  The Object ID isn't
    necessarily copied straight across; it's handled as if the actual
    file were being moved by MoveFileWithProgressW.

Arguments:

    lpOldFileName - Supplies the old file name

    lpNewFileName - Supplies the new file name

Return Value:

    TRUE if successful.  Otherwise the error can be found by calling GetLastError().

--*/

{   // MOVE_FILEIDentityW

    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS StatusIgnored = STATUS_SUCCESS;
    HANDLE SourceFile = INVALID_HANDLE_VALUE;
    HANDLE DestFile = INVALID_HANDLE_VALUE;
    UNICODE_STRING SourceFileName = { 0, 0, NULL };
    PVOID SourceFileNameFreeBuffer = NULL;
    UNICODE_STRING DestFileName = { 0, 0, NULL };
    PVOID DestFileNameFreeBuffer = NULL;
    BOOL TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    OBJECT_ATTRIBUTES SourceObja;
    OBJECT_ATTRIBUTES DestObja;
    ULONG OpenFlags;
    FILE_DISPOSITION_INFORMATION DispositionInformation = { TRUE };
    IO_STATUS_BLOCK IoStatus;
    FILE_BASIC_INFORMATION SourceBasicInfo;
    FILE_BASIC_INFORMATION DestBasicInfo;
    DWORD SourceFileAccess;
    DWORD DestFileAccess;

    try {

        // Open the source file.  It must be opened for write or the
        // FileTrackingInformation call will fail.

        SourceFileAccess = FILE_WRITE_DATA | FILE_READ_ATTRIBUTES;
        if( dwFlags & PRIVMOVE_FILEID_DELETE_OLD_FILE ) {
            SourceFileAccess |= DELETE;
        }

        while( TRUE ) {

            Status = BasepOpenFileForMove( lpOldFileName,
                                           &SourceFileName,
                                           &SourceFileNameFreeBuffer,
                                           &SourceFile,
                                           &SourceObja,
                                           SourceFileAccess,
                                           FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           FILE_OPEN_NO_RECALL );
            if( NT_SUCCESS(Status) ) {
                break;
            } else {

                // We might be able to try again without requesting write access
                if( (SourceFileAccess & FILE_WRITE_DATA) &&
                    (dwFlags & PRIVMOVE_FILEID_IGNORE_ID_ERRORS) ) {

                    // Try again without write access
                    SourceFileAccess &= ~FILE_WRITE_DATA;

                    if( NT_SUCCESS(StatusIgnored) ) {
                        StatusIgnored = Status;
                    }
                    Status = STATUS_SUCCESS;
                } else {
                    // Nothing we can do.
                    break;
                }
            }
        }

        if( !NT_SUCCESS(Status) ) {
            leave;
        }

        // Open the destination file

        DestFileAccess = FILE_WRITE_ATTRIBUTES;
        if( SourceFileAccess & FILE_WRITE_DATA )
            DestFileAccess |= FILE_WRITE_DATA;

        while( TRUE ) {

            Status = BasepOpenFileForMove( lpNewFileName,
                                           &DestFileName,
                                           &DestFileNameFreeBuffer,
                                           &DestFile,
                                           &DestObja,
                                           (SourceFileAccess & FILE_WRITE_DATA)
                                                ? FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA
                                                : FILE_WRITE_ATTRIBUTES,
                                           FILE_SHARE_DELETE | FILE_SHARE_WRITE | FILE_SHARE_READ,
                                           FILE_OPEN_NO_RECALL );

            if( NT_SUCCESS(Status) ) {
                break;
            } else {

                // We might be able to try again without requesting write access
                if( (DestFileAccess & FILE_WRITE_DATA) &&
                    (dwFlags & PRIVMOVE_FILEID_IGNORE_ID_ERRORS) ) {

                    // Try again
                    DestFileAccess &= ~FILE_WRITE_DATA;

                    if( NT_SUCCESS(StatusIgnored) ) {
                        StatusIgnored = Status;
                    }
                    Status = STATUS_SUCCESS;

                } else {
                    // Nothing we can do.
                    break;
                }
            }
        }

        if( !NT_SUCCESS(Status) ) {
            leave;
        }

        // Copy the create date to the dest file

        Status = NtQueryInformationFile( SourceFile,
                                         &IoStatus,
                                         &SourceBasicInfo,
                                         sizeof(SourceBasicInfo),
                                         FileBasicInformation );
        if( NT_SUCCESS(Status) ) {

            RtlZeroMemory( &DestBasicInfo, sizeof(DestBasicInfo) );
            DestBasicInfo.CreationTime = SourceBasicInfo.CreationTime;

            Status = NtSetInformationFile( DestFile,
                                           &IoStatus,
                                           &DestBasicInfo,
                                           sizeof(DestBasicInfo),
                                           FileBasicInformation );
        }

        // If we had an error and can't ignore it, abort.
        if( !NT_SUCCESS(Status) ) {

            if( dwFlags & PRIVMOVE_FILEID_IGNORE_ID_ERRORS ) {
                if( NT_SUCCESS(StatusIgnored) ) {
                    StatusIgnored = Status;
                }
                Status = STATUS_SUCCESS;
            }
            else {
                leave;
            }
        }

        // Transfer the tracking information to the dest file, but only if we
        // were able to get write access to both files.

        if( (DestFileAccess & FILE_WRITE_DATA) &&
            (SourceFileAccess & FILE_WRITE_DATA) ) {

            Status = BasepNotifyTrackingService( &SourceFile,
                                                 &SourceObja,
                                                 DestFile,
                                                 &DestFileName );
            if( !NT_SUCCESS(Status) ) {
                if( dwFlags & PRIVMOVE_FILEID_IGNORE_ID_ERRORS ) {
                    if( NT_SUCCESS(StatusIgnored) ) {
                        StatusIgnored = Status;
                    }
                    Status = STATUS_SUCCESS;
                }
                else {
                    leave;
                }
            }
        }

    }
    finally
    {
        if( SourceFileNameFreeBuffer != NULL )
            RtlFreeHeap( RtlProcessHeap(), 0, SourceFileNameFreeBuffer );

        if( DestFileNameFreeBuffer != NULL )
            RtlFreeHeap( RtlProcessHeap(), 0, DestFileNameFreeBuffer );

    }

    // If requested, delete the source file.  DispositionInformation.DeleteFile
    // has already been initialized to TRUE.

    if( NT_SUCCESS(Status) && (dwFlags & PRIVMOVE_FILEID_DELETE_OLD_FILE) ) {

        Status = NtSetInformationFile(
            SourceFile,
            &IoStatus,
            &DispositionInformation,
            sizeof(DispositionInformation),
            FileDispositionInformation
            );
    }

    if( DestFile != INVALID_HANDLE_VALUE )
        NtClose( DestFile );

    if( SourceFile != INVALID_HANDLE_VALUE )
        NtClose( SourceFile );

    if( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
    }
    else if( !NT_SUCCESS(StatusIgnored) ) {
        BaseSetLastNTError(StatusIgnored);
    }

    return( NT_SUCCESS(Status) );

}





DWORD
WINAPI
GetCompressedFileSizeA(
    LPCSTR lpFileName,
    LPDWORD lpFileSizeHigh
    )
{

    PUNICODE_STRING Unicode;

    Unicode = Basep8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return (DWORD)-1;
    }

    return ( GetCompressedFileSizeW((LPCWSTR)Unicode->Buffer,lpFileSizeHigh) );
}

DWORD
WINAPI
GetCompressedFileSizeW(
    LPCWSTR lpFileName,
    LPDWORD lpFileSizeHigh
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_COMPRESSION_INFORMATION CompressionInfo;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    DWORD FileSizeLow;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return (DWORD)-1;
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
                FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN_FOR_BACKUP_INTENT
                );
    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return (DWORD)-1;
        }

    //
    // Get the compressed file size.
    //

    Status = NtQueryInformationFile(
                Handle,
                &IoStatusBlock,
                &CompressionInfo,
                sizeof(CompressionInfo),
                FileCompressionInformation
                );

    if ( !NT_SUCCESS(Status) ) {
        FileSizeLow = GetFileSize(Handle,lpFileSizeHigh);
        NtClose(Handle);
        return FileSizeLow;
        }


    NtClose(Handle);
    if ( ARGUMENT_PRESENT(lpFileSizeHigh) ) {
        *lpFileSizeHigh = (DWORD)CompressionInfo.CompressedFileSize.HighPart;
        }
    if (CompressionInfo.CompressedFileSize.LowPart == -1 ) {
        SetLastError(0);
        }
    return CompressionInfo.CompressedFileSize.LowPart;
}
