/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dir.c

Abstract:

    This module implements Win32 Directory functions.

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include "basedll.h"
#include "mountmgr.h"

BOOL
APIENTRY
CreateDirectoryA(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    ANSI thunk to CreateDirectoryW

--*/

{
    PUNICODE_STRING Unicode;

    Unicode = Basep8BitStringToStaticUnicodeString( lpPathName );
    if (Unicode == NULL) {
        return FALSE;
    }
        
    return ( CreateDirectoryW((LPCWSTR)Unicode->Buffer,lpSecurityAttributes) );
}

BOOL
APIENTRY
CreateDirectoryW(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    A directory can be created using CreateDirectory.

    This API causes a directory with the specified pathname to be
    created.  If the underlying file system supports security on files
    and directories, then the SecurityDescriptor argument is applied to
    the new directory.

    This call is similar to DOS (int 21h, function 39h) and OS/2's
    DosCreateDir.

Arguments:

    lpPathName - Supplies the pathname of the directory to be created.

    lpSecurityAttributes - An optional parameter that, if present, and
        supported on the target file system supplies a security
        descriptor for the new directory.

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

    //
    // dont create a directory unless there is room in the directory for
    // at least an 8.3 name. This way everyone will be able to delete all
    // files in the directory by using del *.* which expands to path+\*.*
    //

    if ( FileName.Length > ((MAX_PATH-12)<<1) ) {
        DWORD L;
        LPWSTR lp;

        if ( !(lpPathName[0] == '\\' && lpPathName[1] == '\\' &&
               lpPathName[2] == '?' && lpPathName[3] == '\\') ) {
            L = GetFullPathNameW(lpPathName,0,NULL,&lp);
            if ( !L || L+12 > MAX_PATH ) {
                RtlFreeHeap(RtlProcessHeap(), 0,FileName.Buffer);
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                return FALSE;
                }
            }
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

    if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
        Obja.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        }

    Status = NtCreateFile(
                &Handle,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_CREATE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                NULL,
                0L
                );

    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    if ( NT_SUCCESS(Status) ) {
        NtClose(Handle);
        return TRUE;
        }
    else {
        if ( RtlIsDosDeviceName_U((LPWSTR)lpPathName) ) {
            Status = STATUS_NOT_A_DIRECTORY;
            }
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
APIENTRY
CreateDirectoryExA(
    LPCSTR lpTemplateDirectory,
    LPCSTR lpNewDirectory,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    ANSI thunk to CreateDirectoryFromTemplateW

--*/

{
    PUNICODE_STRING StaticUnicode;
    UNICODE_STRING DynamicUnicode;
    BOOL b;

    StaticUnicode = Basep8BitStringToStaticUnicodeString( lpTemplateDirectory );
    if (StaticUnicode == NULL) {
        return FALSE;
    }
        
    if (!Basep8BitStringToDynamicUnicodeString( &DynamicUnicode, lpNewDirectory )) {
        return FALSE;
    }
    
    b = CreateDirectoryExW(
            (LPCWSTR)StaticUnicode->Buffer,
            (LPCWSTR)DynamicUnicode.Buffer,
            lpSecurityAttributes
            );
    
    RtlFreeUnicodeString(&DynamicUnicode);
    
    return b;
}

BOOL
APIENTRY
CreateDirectoryExW(
    LPCWSTR lpTemplateDirectory,
    LPCWSTR lpNewDirectory,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    A directory can be created using CreateDirectoryEx, retaining the
    attributes of the original directory file.

    This API causes a directory with the specified pathname to be
    created.  If the underlying file system supports security on files
    and directories, then the SecurityDescriptor argument is applied to
    the new directory.  The other attributes of the template directory are
    retained when creating the new directory.

    If the original directory is a volume mount point then the new directory
    is also a volume mount point to the same volume as the original one.

Arguments:

    lpTemplateDirectory - Supplies the pathname of the directory to be used as
        a template when creating the new directory.

    lpPathName - Supplies the pathname of the directory to be created.

    lpSecurityAttributes - An optional parameter that, if present, and
        supported on the target file system supplies a security
        descriptor for the new directory.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE SourceFile;
    HANDLE DestFile;
    UNICODE_STRING PathName;
    UNICODE_STRING TargetName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    BOOLEAN IsNameGrafting = FALSE;
    BOOLEAN IsVolumeMountPoint = FALSE;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreePathBuffer;
    PVOID FreeTargetBuffer;
    UNICODE_STRING StreamName;
    WCHAR FileName[MAXIMUM_FILENAME_LENGTH+1];
    HANDLE StreamHandle;
    HANDLE OutputStream;
    PFILE_STREAM_INFORMATION StreamInfo;
    FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;
    PFILE_STREAM_INFORMATION StreamInfoBase;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    FILE_EA_INFORMATION EaInfo;
    FILE_BASIC_INFORMATION BasicInfo;
    ULONG EaSize;
    ULONG StreamInfoSize;
    ULONG CopySize;
    ULONG i;
    ULONG DesiredAccess = 0;
    DWORD Options;
    DWORD b;
    LPCOPYFILE_CONTEXT CopyFileContext = NULL;

    //
    // Process the input template directory name and then open the directory
    // file, ensuring that it really is a directory.
    //

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpTemplateDirectory,
                            &PathName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreePathBuffer = PathName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        PathName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &PathName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // Inhibit the reparse behavior using FILE_OPEN_REPARSE_POINT.
    //

    Status = NtOpenFile(
                 &SourceFile,
                 FILE_LIST_DIRECTORY | FILE_READ_EA | FILE_READ_ATTRIBUTES,
                 &Obja,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
                 );

    if ( !NT_SUCCESS(Status) ) {
        //
        // Back level file systems may not support reparse points and thus not
        // support symbolic links.
        // We infer this is the case when the Status is STATUS_INVALID_PARAMETER.
        //

        if ( Status == STATUS_INVALID_PARAMETER ) {
           //   
           // Re-open not inhibiting the reparse behavior.
           //

           Status = NtOpenFile(
                        &SourceFile,
                        FILE_LIST_DIRECTORY | FILE_READ_EA | FILE_READ_ATTRIBUTES,
                        &Obja,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                        );

           if ( !NT_SUCCESS(Status) ) {
               RtlFreeHeap(RtlProcessHeap(), 0, FreePathBuffer);
               BaseSetLastNTError(Status);
               return FALSE;
               }
           }
        else {
           RtlFreeHeap(RtlProcessHeap(), 0, FreePathBuffer);
           BaseSetLastNTError(Status);
           return FALSE;
           }
        }
    else { 
        //
        // See whether we have a name grafting operation.
        //

        BasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;

        Status = NtQueryInformationFile(
                     SourceFile,
                     &IoStatusBlock,
                     (PVOID)&BasicInfo,
                     sizeof(BasicInfo),
                     FileBasicInformation
                     );

        if ( !NT_SUCCESS(Status) ) {
            RtlFreeHeap(RtlProcessHeap(), 0, FreePathBuffer);
            CloseHandle(SourceFile);
            BaseSetLastNTError(Status);
            return FALSE;
            }

        if ( BasicInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) {
            Status = NtQueryInformationFile(
                         SourceFile,
                         &IoStatusBlock,
                         (PVOID)&FileTagInformation,
                         sizeof(FileTagInformation),
                         FileAttributeTagInformation
                         );

            if ( !NT_SUCCESS(Status) ) {
                RtlFreeHeap(RtlProcessHeap(), 0, FreePathBuffer);
                CloseHandle(SourceFile);
                BaseSetLastNTError(Status);
                return FALSE;
                }

            if ( FileTagInformation.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT ) {
                //   
                // Close and re-open not inhibiting the reparse behavior.
                //

                CloseHandle(SourceFile);

                Status = NtOpenFile(
                             &SourceFile,
                             FILE_LIST_DIRECTORY | FILE_READ_EA | FILE_READ_ATTRIBUTES,
                             &Obja,
                             &IoStatusBlock,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                             );

                if ( !NT_SUCCESS(Status) ) {
                    RtlFreeHeap(RtlProcessHeap(), 0, FreePathBuffer);
                    BaseSetLastNTError(Status);
                    return FALSE;
                    }
                }
            else {
                IsNameGrafting = TRUE;

                //
                // Determine whether the sourse is a volume mount point.
                //

                if ( MOUNTMGR_IS_VOLUME_NAME(&PathName) ) {
                    IsVolumeMountPoint = TRUE;
                    }
                }
            }
        }


    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpNewDirectory,
                            &TargetName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        RtlFreeHeap(RtlProcessHeap(), 0, FreePathBuffer);
        NtClose(SourceFile);
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeTargetBuffer = TargetName.Buffer;

    //
    // Verify that the source and target are different.
    //

    if ( RtlEqualUnicodeString(&PathName, &TargetName, TRUE) ) {
        //
        // Do nothing. Source and target are the same.
        //

        RtlFreeHeap(RtlProcessHeap(), 0, FreePathBuffer);
        RtlFreeHeap(RtlProcessHeap(), 0, FreeTargetBuffer);
        NtClose(SourceFile);
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RtlFreeHeap(RtlProcessHeap(), 0, FreePathBuffer);

    //
    // Do not create a directory unless there is room in the directory for
    // at least an 8.3 name. This way everyone will be able to delete all
    // files in the directory by using del *.* which expands to path+\*.*
    //

    if ( PathName.Length > ((MAX_PATH-12)<<1) ) {
        DWORD L;
        LPWSTR lp;
        if ( !(lpNewDirectory[0] == '\\' && lpNewDirectory[1] == '\\' &&
               lpNewDirectory[2] == '?' && lpNewDirectory[3] == '\\') ) {
            L = GetFullPathNameW(lpNewDirectory,0,NULL,&lp);
            if ( !L || L+12 > MAX_PATH ) {
                RtlFreeHeap(RtlProcessHeap(), 0, FreeTargetBuffer);
                CloseHandle(SourceFile);
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                return FALSE;
                }
            }
        }

    if ( RelativeName.RelativeName.Length ) {
        TargetName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    EaBuffer = NULL;
    EaSize = 0;

    Status = NtQueryInformationFile(
                 SourceFile,
                 &IoStatusBlock,
                 &EaInfo,
                 sizeof(EaInfo),
                 FileEaInformation
                 );

    if ( !NT_SUCCESS(Status) ) {
        RtlFreeHeap(RtlProcessHeap(), 0, FreeTargetBuffer);
        CloseHandle(SourceFile);
        BaseSetLastNTError(Status);
        return FALSE;
        }

    if ( NT_SUCCESS(Status) && EaInfo.EaSize ) {
        EaSize = EaInfo.EaSize;
        do {
            EaSize *= 2;
            EaBuffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), EaSize);
            if ( !EaBuffer ) {
                RtlFreeHeap(RtlProcessHeap(), 0, FreeTargetBuffer);
                CloseHandle(SourceFile);
                BaseSetLastNTError(STATUS_NO_MEMORY);
                return FALSE;
                }

            Status = NtQueryEaFile(
                         SourceFile,
                         &IoStatusBlock,
                         EaBuffer,
                         EaSize,
                         FALSE,
                         (PVOID)NULL,
                         0,
                         (PULONG)NULL,
                         TRUE
                         );

            if ( !NT_SUCCESS(Status) ) {
                RtlFreeHeap(RtlProcessHeap(), 0, EaBuffer);
                EaBuffer = NULL;
                IoStatusBlock.Information = 0;
                }
            } while ( Status == STATUS_BUFFER_OVERFLOW ||
                      Status == STATUS_BUFFER_TOO_SMALL );
        EaSize = (ULONG)IoStatusBlock.Information;
        }

    //
    // Open/create the destination directory.
    //

    InitializeObjectAttributes(
        &Obja,
        &TargetName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
        Obja.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        }

    DesiredAccess = FILE_LIST_DIRECTORY | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES | SYNCHRONIZE;
    if ( BasicInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) {
        //
        // To set the reparse point at the target one needs FILE_WRITE_DATA.
        //

        DesiredAccess |= FILE_WRITE_DATA;
    }

    //
    // Clear the reparse attribute before creating the target. Only the
    // name grafting use of reparse points is preserved at this level.
    // Open first inhibiting the reparse behavior.
    //
    
    BasicInfo.FileAttributes &= ~FILE_ATTRIBUTE_REPARSE_POINT;

    Status = NtCreateFile(
                 &DestFile,
                 DesiredAccess,
                 &Obja,
                 &IoStatusBlock,
                 NULL,
                 BasicInfo.FileAttributes,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,
                 FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT,
                 EaBuffer,
                 EaSize
                 );

    if ( !NT_SUCCESS(Status) ) {    
        //
        // Back level file systems may not support reparse points.
        // We infer this is the case when the Status is STATUS_INVALID_PARAMETER.
        //

        if ( (Status == STATUS_INVALID_PARAMETER) ||
             (Status == STATUS_ACCESS_DENIED) ) {
            //
            // Either the FS does not support reparse points or we do not have enough
            // access to the target.
            //

            if ( IsNameGrafting ) {
                //
                // We need to return error, as the target cannot be opened correctly.
                //

                RtlFreeHeap(RtlProcessHeap(), 0, FreeTargetBuffer);
                if ( EaBuffer ) {
                    RtlFreeHeap(RtlProcessHeap(), 0, EaBuffer);
                    }
                BaseSetLastNTError(Status);
                return FALSE;
                }

            Status = NtCreateFile(
                         &DestFile,
                         FILE_LIST_DIRECTORY | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         &Obja,
                         &IoStatusBlock,
                         NULL,
                         BasicInfo.FileAttributes,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_CREATE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                         EaBuffer,
                         EaSize
                         );                        
            }
        }

    RtlFreeHeap(RtlProcessHeap(), 0, FreeTargetBuffer);

    if ( EaBuffer ) {
        RtlFreeHeap(RtlProcessHeap(), 0, EaBuffer);
        }

    if ( !NT_SUCCESS(Status) ) {
        NtClose(SourceFile);
        if ( RtlIsDosDeviceName_U((LPWSTR)lpNewDirectory) ) {
            Status = STATUS_NOT_A_DIRECTORY;
            }
        BaseSetLastNTError(Status);
        return FALSE;
        }

    else {
        if ( IsNameGrafting ) {
           
            PREPARSE_DATA_BUFFER ReparseBufferHeader = NULL;
            PUCHAR ReparseBuffer = NULL;

            //
            // Allocate the buffer to get/set the reparse point.
            //

            ReparseBuffer = RtlAllocateHeap(
                                RtlProcessHeap(), 
                                MAKE_TAG( TMP_TAG ), 
                                MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

            if ( ReparseBuffer == NULL) {
                NtClose(SourceFile);
                NtClose(DestFile);
                BaseSetLastNTError(STATUS_NO_MEMORY);
                //
                // Notice that we leave behind the target directory.
                //
                return FALSE;
            }

            //
            // Get the data in the reparse point.
            //

            Status = NtFsControlFile(
                         SourceFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         FSCTL_GET_REPARSE_POINT,
                         NULL,                                //  Input buffer
                         0,                                   //  Input buffer length
                         ReparseBuffer,                       //  Output buffer
                         MAXIMUM_REPARSE_DATA_BUFFER_SIZE     //  Output buffer length
                         );

            if ( !NT_SUCCESS( Status ) ) {
                RtlFreeHeap(RtlProcessHeap(), 0, ReparseBuffer);
                NtClose(SourceFile);
                NtClose(DestFile);
                BaseSetLastNTError(Status);
                return FALSE;
                }

            //
            // Defensive sanity check. The reparse buffer should be name grafting.
            //

            ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;

            if ( ReparseBufferHeader->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT ) {
                RtlFreeHeap(RtlProcessHeap(), 0, ReparseBuffer);
                NtClose(SourceFile);
                NtClose(DestFile);
                BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
                return FALSE;
                }

            //
            // Finish up the creation of the target directory.
            //

            if ( IsVolumeMountPoint ) {
                //
                // Set the volume mount point and be done.
                //

                b = SetVolumeMountPointW(
                        lpNewDirectory, 
                        ReparseBufferHeader->MountPointReparseBuffer.PathBuffer
                        );
                }
            else {
                //
                // Copy the directory junction and be done.
                //

                b = TRUE;
                Status = NtFsControlFile(
                             DestFile,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FSCTL_SET_REPARSE_POINT,
                             ReparseBuffer,
                             FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + ReparseBufferHeader->ReparseDataLength,
                             NULL,                //  Output buffer
                             0                    //  Output buffer length
                             );                  
                }

            // 
            // Free the buffer.
            //

            RtlFreeHeap(RtlProcessHeap(), 0, ReparseBuffer);

            //
            // Close all files and return appropriatelly.
            //

            NtClose(SourceFile);
            NtClose(DestFile);

            if ( !b ) {
                //
                // No need to set the last error as SetVolumeMountPointW has done it. 
                //
                return FALSE;
                }
            if ( !NT_SUCCESS( Status ) ) {
                BaseSetLastNTError(Status);
                return FALSE;
                }
            
            return TRUE;

            //
            // The source directory was a name grafting directory.
            // No data streams are copied.
            //
            }

        //
        // Attempt to determine whether or not this file has any alternate
        // data streams associated with it.  If it does, attempt to copy each
        // to the output file.  If any copy fails, simply drop the error on
        // the floor, and continue.
        //

        StreamInfoSize = 4096;
        CopySize = 4096;
        do {
            StreamInfoBase = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), StreamInfoSize);
            if ( !StreamInfoBase ) {
                BaseMarkFileForDelete(DestFile, BasicInfo.FileAttributes);
                BaseSetLastNTError(STATUS_NO_MEMORY);
                b = FALSE;
                break;
                }
            Status = NtQueryInformationFile(
                         SourceFile,
                         &IoStatusBlock,
                         (PVOID) StreamInfoBase,
                         StreamInfoSize,
                         FileStreamInformation
                         );
            if ( !NT_SUCCESS(Status) ) {
                RtlFreeHeap(RtlProcessHeap(), 0, StreamInfoBase);
                StreamInfoBase = NULL;
                StreamInfoSize *= 2;
                }
            } while ( Status == STATUS_BUFFER_OVERFLOW ||
                      Status == STATUS_BUFFER_TOO_SMALL );

        //
        // Directories do not always have a stream
        //

        if ( NT_SUCCESS(Status) && IoStatusBlock.Information ) {
            StreamInfo = StreamInfoBase;

            for (;;) {

                DWORD DestFileFsAttributes = 0;

                //
                // Build a string descriptor for the name of the stream.
                //

                StreamName.Buffer = &StreamInfo->StreamName[0];
                StreamName.Length = (USHORT) StreamInfo->StreamNameLength;
                StreamName.MaximumLength = StreamName.Length;

                //
                // Open the source stream.
                //

                InitializeObjectAttributes(
                    &Obja,
                    &StreamName,
                    0,
                    SourceFile,
                    NULL
                    );
                Status = NtCreateFile(
                             &StreamHandle,
                             GENERIC_READ | SYNCHRONIZE,
                             &Obja,
                             &IoStatusBlock,
                             NULL,
                             0,
                             FILE_SHARE_READ,
                             FILE_OPEN,
                             FILE_SYNCHRONOUS_IO_NONALERT,
                             NULL,
                             0
                             );
                if ( NT_SUCCESS(Status) ) {
                    for ( i = 0; i < (ULONG) StreamName.Length >> 1; i++ ) {
                        FileName[i] = StreamName.Buffer[i];
                        }
                    FileName[i] = L'\0';
                    OutputStream = (HANDLE)NULL;
                    Options = 0;
                    b = BaseCopyStream(
                            NULL,
                            StreamHandle,
                            GENERIC_READ | SYNCHRONIZE,
                            FileName,
                            DestFile,
                            &StreamInfo->StreamSize,
                            &Options,
                            &OutputStream,
                            &CopySize,
                            &CopyFileContext,
                            (LPRESTART_STATE)NULL,
                            (BOOL)FALSE,
                            (DWORD)0,
                            &DestFileFsAttributes
                            );
                    NtClose(StreamHandle);
                    if ( OutputStream ) {
                        NtClose(OutputStream);
                        }
                    }

                if ( StreamInfo->NextEntryOffset ) {
                    StreamInfo = (PFILE_STREAM_INFORMATION)((PCHAR) StreamInfo + StreamInfo->NextEntryOffset);
                    }
                else {
                    break;
                    }

                }
            }
        if ( StreamInfoBase ) {
            RtlFreeHeap(RtlProcessHeap(), 0, StreamInfoBase);
            }
        b = TRUE;
        }
    NtClose(SourceFile);
    if ( DestFile ) {
        NtClose(DestFile);
        }
    return b;
}

BOOL
APIENTRY
RemoveDirectoryA(
    LPCSTR lpPathName
    )

/*++

Routine Description:

    ANSI thunk to RemoveDirectoryW

--*/

{

    PUNICODE_STRING Unicode;

    Unicode = Basep8BitStringToStaticUnicodeString( lpPathName );
    if (Unicode == NULL) {
        return FALSE;
    }
        
    return ( RemoveDirectoryW((LPCWSTR)Unicode->Buffer) );
}

BOOL
APIENTRY
RemoveDirectoryW(
    LPCWSTR lpPathName
    )

/*++

Routine Description:

    An existing directory can be removed using RemoveDirectory.

    This API causes a directory with the specified pathname to be
    deleted.  The directory must be empty before this call can succeed.

    This call is similar to DOS (int 21h, function 3Ah) and OS/2's
    DosDeleteDir.

Arguments:

    lpPathName - Supplies the pathname of the directory to be removed.
        The path must specify an empty directory to which the caller has
        delete access.

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
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    BOOLEAN IsNameGrafting = FALSE;
    FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;
    PREPARSE_DATA_BUFFER reparse;
    BOOL b;
    DWORD bytes;
    UNICODE_STRING mountName;
    PWCHAR volumeMountPoint;


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
    // Open the directory for delete access.
    // Inhibit the reparse behavior using FILE_OPEN_REPARSE_POINT.
    //

    Status = NtOpenFile(
                 &Handle,
                 DELETE | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                 &Obja,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
                 );

    if ( !NT_SUCCESS(Status) ) {
        //
        // Back level file systems may not support reparse points and thus not
        // support symbolic links.
        // We infer this is the case when the Status is STATUS_INVALID_PARAMETER.
        //

        if ( Status == STATUS_INVALID_PARAMETER ) {
            //   
            // Re-open not inhibiting the reparse behavior and not needing to read the attributes.
            //

            Status = NtOpenFile(
                         &Handle,
                         DELETE | SYNCHRONIZE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
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
    else {
        //
        // If we found a reparse point that is not a name grafting operation,
        // either a symbolic link or a mount point, we re-open without 
        // inhibiting the reparse behavior.
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

                //
                // If this is a volume mount point then fail with
                // "directory not empty".
                //

                reparse = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                          MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
                if (!reparse) {
                    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                    NtClose(Handle);
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return FALSE;
                    }

                b = DeviceIoControl(Handle, FSCTL_GET_REPARSE_POINT, NULL, 0,
                                    reparse, MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
                                    &bytes, NULL);

                if (b) {

                    mountName.Length = mountName.MaximumLength =
                        reparse->MountPointReparseBuffer.SubstituteNameLength;
                    mountName.Buffer = (PWCHAR)
                        ((PCHAR) reparse->MountPointReparseBuffer.PathBuffer +
                         reparse->MountPointReparseBuffer.SubstituteNameOffset);

                    if (MOUNTMGR_IS_VOLUME_NAME(&mountName)) {

                        RtlInitUnicodeString(&mountName, lpPathName);
                        volumeMountPoint = RtlAllocateHeap(RtlProcessHeap(),
                                                           MAKE_TAG(TMP_TAG),
                                                           mountName.Length +
                                                           2*sizeof(WCHAR));
                        if (!volumeMountPoint) {
                            RtlFreeHeap(RtlProcessHeap(), 0, reparse);
                            RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                            NtClose(Handle);
                            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                            return FALSE;
                            }

                        RtlCopyMemory(volumeMountPoint, mountName.Buffer,
                                      mountName.Length);
                        volumeMountPoint[mountName.Length/sizeof(WCHAR)] = 0;

                        if (mountName.Buffer[mountName.Length/sizeof(WCHAR) - 1] != '\\') {
                            volumeMountPoint[mountName.Length/sizeof(WCHAR)] = '\\';
                            volumeMountPoint[mountName.Length/sizeof(WCHAR) + 1] = 0;
                        }

                        DeleteVolumeMountPointW(volumeMountPoint);

                        RtlFreeHeap(RtlProcessHeap(), 0, volumeMountPoint);
                        }
                    }

                RtlFreeHeap(RtlProcessHeap(), 0, reparse);
                IsNameGrafting = TRUE;
                }
            }
        
        if ( NT_SUCCESS(Status) &&
             (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
             !IsNameGrafting) {
            //
            // Re-open without inhibiting the reparse behavior and not needing to 
            // read the attributes.
            //
  
            NtClose(Handle);
            Status = NtOpenFile(
                         &Handle,
                         DELETE | SYNCHRONIZE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
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
                                 DELETE | SYNCHRONIZE,
                                 &Obja,
                                 &IoStatusBlock,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
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
