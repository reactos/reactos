/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    fileopcr.c

Abstract:

    This module implements File open and Create APIs for Win32

Author:

    Mark Lucovsky (markl) 25-Sep-1990

Revision History:

--*/


#include "basedll.h"
#include "mountmgr.h"
#include "aclapi.h"

WCHAR BasepPropertySetAttributeType[] = PROPERTY_SET_ATTRIBUTE_NAME;
WCHAR BasepDataAttributeType[] =        DATA_ATTRIBUTE_NAME;

typedef BOOL (WINAPI *ENCRYPTFILEWPTR)(LPCWSTR);
typedef BOOL (WINAPI *DECRYPTFILEWPTR)(LPCWSTR, DWORD);

#define BASE_OF_SHARE_MASK 0x00000070
#define TWO56K ( 256 * 1024 )
ULONG
BasepOfShareToWin32Share(
    IN ULONG OfShare
    )
{
    DWORD ShareMode;

    if ( (OfShare & BASE_OF_SHARE_MASK) == OF_SHARE_DENY_READ ) {
        ShareMode = FILE_SHARE_WRITE;
        }
    else if ( (OfShare & BASE_OF_SHARE_MASK) == OF_SHARE_DENY_WRITE ) {
        ShareMode = FILE_SHARE_READ;
        }
    else if ( (OfShare & BASE_OF_SHARE_MASK) == OF_SHARE_DENY_NONE ) {
        ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
        }
    else if ( (OfShare & BASE_OF_SHARE_MASK) == OF_SHARE_EXCLUSIVE ) {
        ShareMode = 0;
        }
    else {
        ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;;
        }
    return ShareMode;
}


PUNICODE_STRING
BaseIsThisAConsoleName(
    PUNICODE_STRING FileNameString,
    DWORD dwDesiredAccess
    )
{
    PUNICODE_STRING FoundConsoleName;
    ULONG DeviceNameLength;
    ULONG DeviceNameOffset;
    UNICODE_STRING ConString;
    WCHAR sch,ech;

    FoundConsoleName = NULL;
    if ( FileNameString->Length ) {
        sch = FileNameString->Buffer[0];
        ech = FileNameString->Buffer[(FileNameString->Length-1)>>1];

        //
        // if CON, CONOUT$, CONIN$, \\.\CON...
        //
        //

        if ( sch == (WCHAR)'c' || sch == (WCHAR)'C' || sch == (WCHAR)'\\' ||
             ech == (WCHAR)'n' || ech == (WCHAR)'N' || ech == (WCHAR)':' || ech == (WCHAR)'$' ) {


            ConString = *FileNameString;

            DeviceNameLength = RtlIsDosDeviceName_U(ConString.Buffer);
            if ( DeviceNameLength ) {
                DeviceNameOffset = DeviceNameLength >> 16;
                DeviceNameLength &= 0x0000ffff;

                ConString.Buffer = (PWSTR)((PSZ)ConString.Buffer + DeviceNameOffset);
                ConString.Length = (USHORT)DeviceNameLength;
                ConString.MaximumLength = (USHORT)(DeviceNameLength + sizeof(UNICODE_NULL));
                }

            FoundConsoleName = NULL;
            try {

                if (RtlEqualUnicodeString(&ConString,&BaseConsoleInput,TRUE) ) {
                    FoundConsoleName = &BaseConsoleInput;
                    }
                else if (RtlEqualUnicodeString(&ConString,&BaseConsoleOutput,TRUE) ) {
                    FoundConsoleName = &BaseConsoleOutput;
                    }
                else if (RtlEqualUnicodeString(&ConString,&BaseConsoleGeneric,TRUE) ) {
                    if ((dwDesiredAccess & (GENERIC_READ|GENERIC_WRITE)) == GENERIC_READ) {
                        FoundConsoleName = &BaseConsoleInput;
                        }
                    else if ((dwDesiredAccess & (GENERIC_READ|GENERIC_WRITE)) == GENERIC_WRITE){
                        FoundConsoleName = &BaseConsoleOutput;
                        }
                    }
                }
            except (EXCEPTION_EXECUTE_HANDLER) {
                return NULL;
                }
            }
        }
    return FoundConsoleName;
}


DWORD
WINAPI
CopyReparsePoint(
    HANDLE hSourceFile,
    HANDLE hDestinationFile
    )

/*++

Routine Description:

    This is an internal routine that copies a reparse point.

Arguments:

    hSourceFile - Provides a handle to the source file.

    hDestinationFile - Provides a handle to the destination file.

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
   NTSTATUS Status;
   IO_STATUS_BLOCK IoStatusBlock;
   PUCHAR ReparseBuffer;
   PREPARSE_DATA_BUFFER ReparseBufferHeader;

   //
   //  Allocate the buffer to set the reparse point.
   //

   ReparseBuffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
   if ( ReparseBuffer == NULL ) {
       BaseSetLastNTError(STATUS_NO_MEMORY);
       return FALSE;
   }

   //
   // Get the reparse point.
   //

   Status = NtFsControlFile(
                hSourceFile,
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
       BaseSetLastNTError(Status);
       return FALSE;
       }

   //
   // Decode the reparse point buffer.
   //

   ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;

   //
   // Set the reparse point.
   //

   Status = NtFsControlFile(
                hDestinationFile,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                FSCTL_SET_REPARSE_POINT,
                ReparseBuffer,
                FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + ReparseBufferHeader->ReparseDataLength,
                NULL,
                0
                );

   RtlFreeHeap(RtlProcessHeap(), 0, ReparseBuffer);

   if ( !NT_SUCCESS( Status ) ) {
       BaseSetLastNTError(Status);
       return FALSE;
       }

   return TRUE;
}


DWORD
WINAPI
CopyNameGraftNow(
    HANDLE hSourceFile,
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    ULONG CreateOptions,
    LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    LPVOID lpData OPTIONAL,
    LPBOOL pbCancel OPTIONAL,
    LPDWORD lpCopyFlags
    )

/*++

Routine Description:

    This is an internal routine that copies a name grafting file/directory preserving
    its characteristics.

Arguments:

    hSourceFile - Provides a handle to the source file.

    lpExistingFileName - Provides the name of the existing, source file.

    lpNewFileName - Provides a name for the target file/stream. This must not
        be a UNC path name.

    lpProgressRoutine - Optionally supplies the address of a callback routine
        to be called as the copy operation progresses.

    lpData - Optionally supplies a context to be passed to the progress callback
        routine.

    pbCancel - Optionally supplies the address of a boolean to be set to TRUE
        if the caller would like the copy to abort.

    lpCopyFlags - Provides flags that modify how the copy is to proceed.  See
        CopyFileEx for details.

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{   // CopyNameGraftNow

    NTSTATUS Status;
    DWORD ReturnValue = FALSE;
    HANDLE DestFile = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK IoStatusBlock;
    PREPARSE_DATA_BUFFER ReparseBufferHeader;
    PUCHAR ReparseBuffer = NULL;
    FILE_BASIC_INFORMATION BasicInformation;
    FILE_STANDARD_INFORMATION StandardInformation;
    COPYFILE_CONTEXT CfContext;
    UNICODE_STRING      SourceFileName;
    UNICODE_STRING      DestFileName;
    PVOID               SourceFileNameBuffer = NULL;
    PVOID               DestFileNameBuffer = NULL;
    RTL_RELATIVE_NAME   DestRelativeName;
    BOOL TranslationStatus;
    BOOL b;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatus;

    //
    // Set up the context if appropriate.
    //

    RtlZeroMemory(&StandardInformation, sizeof(StandardInformation));
    if ( ARGUMENT_PRESENT(lpProgressRoutine) || ARGUMENT_PRESENT(pbCancel) ) {

        CfContext.TotalFileSize = StandardInformation.EndOfFile;
        CfContext.TotalBytesTransferred.QuadPart = 0;
        CfContext.dwStreamNumber = 0;
        CfContext.lpCancel = pbCancel;
        CfContext.lpData = lpData;
        CfContext.lpProgressRoutine = lpProgressRoutine;
    }

    //
    // Allocate the buffer to set the reparse point.
    //

    ReparseBuffer = RtlAllocateHeap(
                        RtlProcessHeap(),
                        MAKE_TAG( TMP_TAG ),
                        MAXIMUM_REPARSE_DATA_BUFFER_SIZE
                        );
    if ( ReparseBuffer == NULL) {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return FALSE;
        }

    try
    {
        //
        // Translate both names.
        //

        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                lpExistingFileName,
                                &SourceFileName,
                                NULL,
                                &DestRelativeName
                                );

        if ( !TranslationStatus ) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            DestFile = INVALID_HANDLE_VALUE;
            leave;
        }
        SourceFileNameBuffer = SourceFileName.Buffer;

        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                lpNewFileName,
                                &DestFileName,
                                NULL,
                                &DestRelativeName
                                );

        if ( !TranslationStatus ) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            DestFile = INVALID_HANDLE_VALUE;
            leave;
        }
        DestFileNameBuffer = DestFileName.Buffer;

        //
        // Verify that the source and target are different.
        //

        if ( RtlEqualUnicodeString(&SourceFileName, &DestFileName, TRUE) ) {
            //
            // Do nothing. Source and target are the same.
            //

            DestFile = INVALID_HANDLE_VALUE;
            leave;
        }

        //
        // Open the destination.
        //

        /*
        DestFile = CreateFileW(
                       lpNewFileName,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       (*lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS) ? CREATE_NEW : OPEN_ALWAYS,
                       FILE_FLAG_OPEN_REPARSE_POINT,
                       NULL
                       );
        */

        if ( DestRelativeName.RelativeName.Length ) {
            DestFileName = *(PUNICODE_STRING)&DestRelativeName.RelativeName;
        }
        else {
            DestRelativeName.ContainingDirectory = NULL;
        }

        InitializeObjectAttributes(
            &Obja,
            &DestFileName,
            OBJ_CASE_INSENSITIVE,
            DestRelativeName.ContainingDirectory,
            NULL
            );

        Status = NtCreateFile( &DestFile,
                               GENERIC_READ | GENERIC_WRITE,
                               &Obja,
                               &IoStatus,
                               NULL,
                               0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               (*lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS) ? FILE_CREATE : FILE_OPEN_IF,
                               FILE_OPEN_REPARSE_POINT | CreateOptions,
                               NULL,
                               0 );
        if( !NT_SUCCESS(Status) ) {
            DestFile = INVALID_HANDLE_VALUE;
            BaseSetLastNTError(Status);
            leave;
        }

        //
        // We now have the handle to the destination.
        // We get and set the corresponding reparse point.
        //

        Status = NtFsControlFile(
                     hSourceFile,
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
            BaseSetLastNTError(Status);
            leave;
        }

        //
        // Defensive sanity check. The reparse buffer should be name grafting.
        //

        ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;
        if ( ReparseBufferHeader->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT ) {
            BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
            leave;
        }

        //
        // Determine whether the sourse is a volume mount point.
        //

        if ( MOUNTMGR_IS_VOLUME_NAME(&SourceFileName) ) {
            //
            // Set the volume mount point and be done.
            //

            b = SetVolumeMountPointW(
                    lpNewFileName,
                    ReparseBufferHeader->MountPointReparseBuffer.PathBuffer
                    );
            if ( !b ) {
                leave;
                }
            }
        else {
            //
            // Set the reparse point of type name junction.
            //

            Status = NtFsControlFile(
                         DestFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         FSCTL_SET_REPARSE_POINT,
                         ReparseBuffer,
                         FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + ReparseBufferHeader->ReparseDataLength,
                         NULL,
                         0
                         );
            }

        if ( !(*lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS) &&
             ((Status == STATUS_EAS_NOT_SUPPORTED) ||
              (Status == STATUS_IO_REPARSE_TAG_MISMATCH)) ) {
            //
            // In either of these error conditions, the correct behavior is to
            // first delete the destination file and then copy the name graft.
            //
            // Re-open the destination for the deletion without inhibiting the
            // reparse behavior.
            //

            BOOL DeleteStatus = FALSE;

            CloseHandle(DestFile);
            DestFile = INVALID_HANDLE_VALUE;

            DeleteStatus = DeleteFileW(
                               lpNewFileName
                               );

            if ( !DeleteStatus ) {
                leave;
                }

            //
            // Create the destination name graft.
            // Notice that either a file or a directory may be created.
            //

            /*
            DestFile = CreateFileW(
                           lpNewFileName,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           CREATE_NEW,
                           FILE_FLAG_OPEN_REPARSE_POINT,
                           NULL
                           );

            if ( DestFile == INVALID_HANDLE_VALUE ) {
                RtlFreeHeap(RtlProcessHeap(), 0, ReparseBuffer);
                return FALSE;
                }
            */

            Status = NtCreateFile( &DestFile,
                                   GENERIC_READ | GENERIC_WRITE,
                                   &Obja,
                                   &IoStatus,
                                   NULL,
                                   0,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   FILE_CREATE,
                                   FILE_OPEN_REPARSE_POINT | CreateOptions,
                                   NULL,
                                   0 );
            if( !NT_SUCCESS( Status )) {
                BaseSetLastNTError( Status );
                leave;
            }

            //
            // Set the reparse point.
            //

            Status = NtFsControlFile(
                         DestFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         FSCTL_SET_REPARSE_POINT,
                         ReparseBuffer,
                         FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + ReparseBufferHeader->ReparseDataLength,
                         NULL,
                         0
                         );
        }   // if ( !(*lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS) ...

        //
        // Close the destination file and return appropriatelly.
        //

        if ( !NT_SUCCESS( Status ) ) {
            BaseSetLastNTError(Status);
            leave;
            }

        //
        // The name graft was copied. Set the last write time for the file
        // so that it matches the input file.
        //

        Status = NtQueryInformationFile(
                     hSourceFile,
                     &IoStatusBlock,
                     (PVOID) &BasicInformation,
                     sizeof(BasicInformation),
                     FileBasicInformation
                     );

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            leave;
        }

        BasicInformation.CreationTime.QuadPart = 0;
        BasicInformation.LastAccessTime.QuadPart = 0;
        BasicInformation.FileAttributes = 0;

        //
        // If the time cannot be set for whatever reason, we still return
        // TRUE.
        //

        Status = NtSetInformationFile(
                     DestFile,
                     &IoStatusBlock,
                     &BasicInformation,
                     sizeof(BasicInformation),
                     FileBasicInformation
                     );

        if ( Status == STATUS_SHARING_VIOLATION ) {

            //
            // IBM PC Lan Program (and other MS-NET servers) return
            // STATUS_SHARING_VIOLATION if an application attempts to perform
            // an NtSetInformationFile on a file handle opened for GENERIC_READ
            // or GENERIC_WRITE.
            //
            // If we get a STATUS_SHARING_VIOLATION on this API we want to:
            //
            //   1) Close the handle to the destination
            //   2) Re-open the file for FILE_WRITE_ATTRIBUTES
            //   3) Re-try the operation.
            //

            CloseHandle(DestFile);

            //
            // Re-Open the destination file inhibiting the reparse behavior as
            // we know that it is a symbolic link.  Please note that we do this
            // using the CreateFileW API.  The CreateFileW API allows you to
            // pass NT native desired access flags, even though it is not
            // documented to work in this manner.
            //

            /*
            DestFile = CreateFileW(
                           lpNewFileName,
                           FILE_WRITE_ATTRIBUTES,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
                           NULL
                           );
            */
            Status = NtCreateFile( &DestFile,
                                   FILE_WRITE_ATTRIBUTES,
                                   &Obja,
                                   &IoStatus,
                                   NULL,
                                   0,
                                   0,
                                   FILE_OPEN,
                                   FILE_OPEN_REPARSE_POINT | CreateOptions,
                                   NULL,
                                   0 );

            if ( NT_SUCCESS( Status )) {

                //
                // If the open succeeded, we update the file information on
                // the new file.
                //
                // Note that we ignore any errors from this point on.
                //

                Status = NtSetInformationFile(
                             DestFile,
                             &IoStatusBlock,
                             &BasicInformation,
                             sizeof(BasicInformation),
                             FileBasicInformation
                             );

            }
        }

        ReturnValue = TRUE;

    }
    finally
    {
        if( INVALID_HANDLE_VALUE != DestFile )
            CloseHandle( DestFile );
        if( SourceFileNameBuffer )
            RtlFreeHeap( RtlProcessHeap(), 0, SourceFileNameBuffer );
        if( DestFileNameBuffer )
            RtlFreeHeap( RtlProcessHeap(), 0, DestFileNameBuffer );
        if( ReparseBuffer )
            RtlFreeHeap(RtlProcessHeap(), 0, ReparseBuffer);
    }

    return ReturnValue;
}


BOOL
WINAPI
CopyFileA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    BOOL bFailIfExists
    )

/*++

Routine Description:

    ANSI thunk to CopyFileW

--*/

{
    PUNICODE_STRING StaticUnicode;
    UNICODE_STRING DynamicUnicode;
    BOOL b;

    StaticUnicode = Basep8BitStringToStaticUnicodeString( lpExistingFileName );
    if (StaticUnicode == NULL) {
        return FALSE;
    }

    if (!Basep8BitStringToDynamicUnicodeString( &DynamicUnicode, lpNewFileName )) {
        return FALSE;
    }

    b = CopyFileExW(
            (LPCWSTR)StaticUnicode->Buffer,
            (LPCWSTR)DynamicUnicode.Buffer,
            (LPPROGRESS_ROUTINE)NULL,
            (LPVOID)NULL,
            (LPBOOL)NULL,
            bFailIfExists ? COPY_FILE_FAIL_IF_EXISTS : 0
            );

    RtlFreeUnicodeString(&DynamicUnicode);

    return b;
}

BOOL
WINAPI
CopyFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists
    )

/*++

Routine Description:

    A file, its extended attributes, alternate data streams, and any other
    attributes can be copied using CopyFile.

Arguments:

    lpExistingFileName - Supplies the name of an existing file that is to be
        copied.

    lpNewFileName - Supplies the name where a copy of the existing
        files data and attributes are to be stored.

    bFailIfExists - Supplies a flag that indicates how this operation is
        to proceed if the specified new file already exists.  A value of
        TRUE specifies that this call is to fail.  A value of FALSE
        causes the call to the function to succeed whether or not the
        specified new file exists.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    BOOL b;

    b = CopyFileExW(
            lpExistingFileName,
            lpNewFileName,
            (LPPROGRESS_ROUTINE)NULL,
            (LPVOID)NULL,
            (LPBOOL)NULL,
            bFailIfExists ? COPY_FILE_FAIL_IF_EXISTS : 0
            );

    return b;
}

BOOL
WINAPI
CopyFileExA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    LPVOID lpData OPTIONAL,
    LPBOOL pbCancel OPTIONAL,
    DWORD dwCopyFlags
    )

/*++

Routine Description:

    ANSI thunk to CopyFileExW

--*/

{
    PUNICODE_STRING StaticUnicode;
    UNICODE_STRING DynamicUnicode;
    BOOL b;

    StaticUnicode = Basep8BitStringToStaticUnicodeString( lpExistingFileName );
    if (StaticUnicode == NULL) {
        return FALSE;
    }

    if (!Basep8BitStringToDynamicUnicodeString( &DynamicUnicode, lpNewFileName )) {
        return FALSE;
    }

    b = CopyFileExW(
            (LPCWSTR)StaticUnicode->Buffer,
            (LPCWSTR)DynamicUnicode.Buffer,
            lpProgressRoutine,
            lpData,
            pbCancel,
            dwCopyFlags
            );

    RtlFreeUnicodeString(&DynamicUnicode);

    return b;
}





#define COPY_FILE_VALID_FLAGS (COPY_FILE_FAIL_IF_EXISTS | COPY_FILE_RESTARTABLE | COPY_FILE_OPEN_SOURCE_FOR_WRITE)




NTSTATUS
BasepProcessNameGrafting( HANDLE SourceFile,
                          PBOOL IsNameGrafting,
                          PBOOL bCopyRawSourceFile,
                          PBOOL bOpenFilesAsReparsePoint,
                          PBOOL bIsNssFile,
                          PFILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation )
/*++

Routine Description:

    During CopyFile, check to see if the source is a symlink which
    requires special processing during copy.

Arguments:

    SourceFile - Handle for the source of the copy.

    IsNameGrafting - If true on return, the source file is grafted.

    bCopyRawSourceFile - If true on return, the source file needn't be
        reopened.  If false, the file should be reopened without the
        FILE_OPEN_REPARSE_POINT flag.

    bOpenFilesAsReparsePoint - If true on return, source/dest named
        streams should be opened/created with FILE_OPEN_REPARSE_POINT
        specified.

    bIsNssFile - If true on return, the source file is a Native
        Structured Storage file.

    FileTagInformation - Pointer to location to hold the results of
        NtQueryInformationFile(FileAttributeTagInformation).

Return Value:

    NTSTATUS

--*/


{
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = NtQueryInformationFile(
                SourceFile,
                &IoStatus,
                (PVOID) FileTagInformation,
                sizeof(*FileTagInformation),
                FileAttributeTagInformation
                );

    if ( !NT_SUCCESS(Status) ) {
        //
        //  Not all File Systems implement all information classes.
        //  The value STATUS_INVALID_PARAMETER is returned when a non-supported
        //  information class is requested to a back-level File System. As all
        //  the parameters to NtQueryInformationFile are correct, we can infer
        //  in this case that we found a back-level system.
        //

        if ( (Status != STATUS_INVALID_PARAMETER) &&
             (Status != STATUS_NOT_IMPLEMENTED) ) {
            return( Status );
        }
        Status = STATUS_SUCCESS;

        //
        //  If FileAttributeTagInformation is not supported, we assume that
        //  the file at hand is not a reparse point nor a symbolic link.
        //  The copy of these files is the same as the raw copy of a file.
        //  The target file is opened without inhibiting the reparse point
        //  behavior.
        //

        *bCopyRawSourceFile = TRUE;
    } else {
       //
       //  The source file is opened and the file system supports the
       //  FileAttributeTagInformation information class.
       //

       if ( FileTagInformation->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) {
           //
           //  We have a reparse point at hand.
           //

           if ( FileTagInformation->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT ) {
               //
               //  We found a name grafting operation.
               //

               *IsNameGrafting = TRUE;
           }

           if (( FileTagInformation->ReparseTag == IO_REPARSE_TAG_NSS ) ||
               ( FileTagInformation->ReparseTag == IO_REPARSE_TAG_NSSRECOVER)) {
               //
               //  It is safe to copy the reparse point raw. Whenever possible
               //  inhibit the reparse behavior at the source and at the
               //  destination file systems.
               //

               *bCopyRawSourceFile = TRUE;
               *bOpenFilesAsReparsePoint = TRUE;
               *bIsNssFile = TRUE;
           }
       } else {
           //
           //  We have a valid handle.
           //  The underlying file system supports reparse points.
           //  The source file is not a reparse point.
           //  This is the case of a normal file in NT 5.0.
           //  The SourceFile handle can be used for the copy. The copy of
           //  these files is the same as the raw copy of a reparse point.
           //  The target file is opened without inhibiting the reparse
           //  point behavior.
           //

           *bCopyRawSourceFile = TRUE;
       }
    }

    return( Status );
}



BOOL
BasepCopySecurityInformation( LPCWSTR lpExistingFileName,
                              HANDLE SourceFile,
                              ACCESS_MASK SourceFileAccess,
                              LPCWSTR lpNewFileName,
                              HANDLE DestFile,
                              ACCESS_MASK DestFileAccess,
                              SECURITY_INFORMATION SecurityInformation,
                              LPCOPYFILE_CONTEXT Context,
                              DWORD DestFileFsAttributes,
                              PBOOL Canceled );

BOOL
BasepCopyFileCallback( BOOL ContinueByDefault,
                       DWORD Win32ErrorOnStopOrCancel,
                       LPCOPYFILE_CONTEXT Context,
                       PLARGE_INTEGER StreamBytesCopied OPTIONAL,
                       DWORD CallbackReason,
                       HANDLE SourceFile,
                       HANDLE DestFile,
                       OPTIONAL PBOOL Canceled );



BOOL
BasepCopyFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    LPVOID lpData OPTIONAL,
    LPBOOL pbCancel OPTIONAL,
    DWORD dwCopyFlags,
    DWORD dwPrivCopyFlags,  // From PrivCopyFileExW
    LPHANDLE phSource,
    LPHANDLE phDest
    )
{
    HANDLE SourceFile = INVALID_HANDLE_VALUE;
    HANDLE DestFile = INVALID_HANDLE_VALUE;
    DWORD b = FALSE;
    BOOL IsNameGrafting = FALSE;
    BOOL bCopyRawSourceFile = FALSE;
    BOOL bOpenFilesAsReparsePoint = FALSE;
    BOOL bIsNssFile = FALSE;
    ULONG CopySize;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    FILE_STANDARD_INFORMATION FileInformation;
    FILE_BASIC_INFORMATION BasicInformation;
    PFILE_STREAM_INFORMATION StreamInfo;
    PFILE_STREAM_INFORMATION StreamInfoBase = NULL;
    UNICODE_STRING StreamName;
    WCHAR FileName[MAXIMUM_FILENAME_LENGTH+1];
    HANDLE OutputStream;
    HANDLE StreamHandle;
    ULONG StreamInfoSize;
    COPYFILE_CONTEXT CfContext;
    LPCOPYFILE_CONTEXT CopyFileContext = NULL;
    RESTART_STATE RestartState;
    DWORD SourceFileAttributes = 0;
    DWORD FlagsAndAttributes = 0;
    DWORD FileFlagBackupSemantics = 0;
    DWORD DestFileFsAttributes = 0;
    DWORD SourceFileAccessDefault;
    DWORD SourceFileAccess = 0;
    DWORD SourceFileFlagsAndAttributes = 0;
    DWORD SourceFileSharing = 0;
    DWORD SourceFileSharingDefault = 0;
    BOOL  CheckedForNameGrafting = FALSE;
    FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;

    //
    // Ensure that only valid flags were passed.
    //

    if ( dwCopyFlags & ~COPY_FILE_VALID_FLAGS ) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    if ( dwPrivCopyFlags & ~PRIVCOPY_FILE_VALID_FLAGS ) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    dwCopyFlags |= dwPrivCopyFlags;

    try {

        //
        //  We first establish whether we are copying a reparse point:
        //  (1) obtain a handle inhibiting the reparse point behavior
        //  (2) establish whether a symbolic link was found
        //  (3) establish whether a reparse point that is not a symbolic link
        //      is to be copied in raw format or re-enabling the reparse point
        //      behavior
        //

        // Determine if backup-intent should be set.
        if( (PRIVCOPY_FILE_DIRECTORY|PRIVCOPY_FILE_BACKUP_SEMANTICS) & dwCopyFlags ) {
            FileFlagBackupSemantics = FILE_FLAG_BACKUP_SEMANTICS;
        }

        SourceFileAccessDefault = GENERIC_READ;
        SourceFileAccessDefault |= (dwCopyFlags & COPY_FILE_OPEN_SOURCE_FOR_WRITE) ? GENERIC_WRITE : 0;
        SourceFileAccessDefault |= (dwCopyFlags & PRIVCOPY_FILE_SACL) ? ACCESS_SYSTEM_SECURITY : 0;

        SourceFileFlagsAndAttributes = FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN | FileFlagBackupSemantics;
        CheckedForNameGrafting = FALSE;
        SourceFileSharingDefault = FILE_SHARE_READ;

retry_open_SourceFile:

        SourceFileAccess = SourceFileAccessDefault;
        SourceFileSharing = SourceFileSharingDefault;

        while( TRUE ) {

            SourceFile = CreateFileW(
                            lpExistingFileName,
                            SourceFileAccess,
                            SourceFileSharing,
                            NULL,
                            OPEN_EXISTING,
                            SourceFileFlagsAndAttributes,
                            NULL
                            );

            if ( SourceFile == INVALID_HANDLE_VALUE ) {

                // If we tried to get ACCESS_SYSTEM_SECURITY access, that
                // might cause an access or privilege error.
                if( ( GetLastError() == ERROR_PRIVILEGE_NOT_HELD
                      ||
                      GetLastError() == ERROR_ACCESS_DENIED
                    )
                    &&
                    (SourceFileAccess & ACCESS_SYSTEM_SECURITY) ) {

                    // Turn it off
                    SourceFileAccess &= ~ACCESS_SYSTEM_SECURITY;
                }


                // Maybe we should stop requesting write access (done for
                // COPYFILE_OPEN_SOURCE_FOR_WRITE
                else if( GetLastError() == ERROR_ACCESS_DENIED &&
                         (GENERIC_WRITE & SourceFileAccess) ) {

                    // Turn it off, but if originally requested,
                    // turn access_system_security back on.
                    SourceFileAccess &= ~GENERIC_WRITE;

                    if( SourceFileAccessDefault & ACCESS_SYSTEM_SECURITY ) {
                        SourceFileAccess |= ACCESS_SYSTEM_SECURITY;
                    }
                }

                // Try sharing for writing.
                else if( !(FILE_SHARE_WRITE & SourceFileSharing) ) {
                    // Add write-sharing
                    SourceFileSharing |= FILE_SHARE_WRITE;

                    // Start back over wrt the access flags
                    SourceFileAccess = SourceFileAccessDefault;
                }

                //
                //  There is the case when we still do not get the file opened and we
                //  do want to proceed with the copy. Pre NT 5.0 systems do not support
                //  FILE_FLAG_OPEN_REPARSE_POINT. If this happens, by initialization we
                //  have that:
                //        IsNameGrafting            is FALSE  and
                //        bCopyRawSourceFile        is FALSE  and
                //        bOpenFilesAsReparsePoint  is FALSE
                //

                else if( FILE_FLAG_OPEN_REPARSE_POINT & SourceFileFlagsAndAttributes ) {
                    // Turn off open-reparse
                    SourceFileFlagsAndAttributes &= ~FILE_FLAG_OPEN_REPARSE_POINT;

                    // Reset the access & sharing back to default
                    SourceFileAccess = SourceFileAccessDefault;
                    SourceFileSharing = SourceFileSharingDefault;
                }


                // Otherwise there's nothing more we can try.
                else {
                    leave;
                }


            }   // if ( SourceFile == INVALID_HANDLE_VALUE )

            // We've opened the source file.  If we haven't yet checked for
            // name grafting (symbolic links), do so now.

            else if( !CheckedForNameGrafting ) {

                CheckedForNameGrafting = TRUE;

                //
                //  Find out whether the file is a symbolic link and whether a reparse
                //  point can be copied with the reparse behavior inhibited.
                //

                Status = BasepProcessNameGrafting( SourceFile,
                                                   &IsNameGrafting,
                                                   &bCopyRawSourceFile,
                                                   &bOpenFilesAsReparsePoint,
                                                   &bIsNssFile,
                                                   &FileTagInformation );
                if( !NT_SUCCESS(Status) ) {
                    CloseHandle( SourceFile );
                    SourceFile = INVALID_HANDLE_VALUE;
                    BaseSetLastNTError(Status);
                    leave;
                }

                if ( IsNameGrafting ) {
                    //
                    //  Do now the copy of a name grafting file/directory.
                    //

                    Status = CopyNameGraftNow(
                                 SourceFile,
                                 lpExistingFileName,
                                 lpNewFileName,
                                 (PRIVCOPY_FILE_DIRECTORY & dwPrivCopyFlags)
                                     ? (FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT)
                                     : 0,
                                 lpProgressRoutine,
                                 lpData,
                                 pbCancel,
                                 &dwCopyFlags
                                 );

                    CloseHandle(SourceFile);
                    SourceFile = INVALID_HANDLE_VALUE;

                    if( !Status ) {
                        return FALSE;
                    }

                    return TRUE;

                }

                // If we're doing a raw copy, we can start doing the copy with this
                // SourceFile handle.

                if ( bCopyRawSourceFile ) {
                    break; // while( TRUE )
                }

                // Otherwise, we need to reopen without FILE_FLAG_OPEN_REPARSE_POINT;
                else {
                    // Turn off open-as-reparse
                    SourceFileFlagsAndAttributes &= ~FILE_FLAG_OPEN_REPARSE_POINT;

                    CloseHandle( SourceFile );
                    SourceFile = INVALID_HANDLE_VALUE;

                    // Since SourceFileAccess & SourceFileSharing are already set,
                    // the next CreateFile attempt should succeed.
                }

            }   // else if( !CheckedForNameGrafting )

            // Otherwise, we have the file open, and we're done checking for grafting
            else {
                break;
            }

        }   // while( TRUE )


        //
        //  Size the source file to determine how much data is to be copied
        //

        Status = NtQueryInformationFile(
                    SourceFile,
                    &IoStatus,
                    (PVOID) &FileInformation,
                    sizeof(FileInformation),
                    FileStandardInformation
                    );

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            leave;
        }

        //
        //  Get the timestamp info as well.
        //

        Status = NtQueryInformationFile(
                    SourceFile,
                    &IoStatus,
                    (PVOID) &BasicInformation,
                    sizeof(BasicInformation),
                    FileBasicInformation
                    );

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            leave;
        }

        SourceFileAttributes = BasicInformation.FileAttributes; // Cache for later use

        //
        // Set up the context if appropriate.
        //

        if ( ARGUMENT_PRESENT(lpProgressRoutine) || ARGUMENT_PRESENT(pbCancel) ) {

            CfContext.TotalFileSize = FileInformation.EndOfFile;
            CfContext.TotalBytesTransferred.QuadPart = 0;
            CfContext.dwStreamNumber = 0;
            CfContext.lpCancel = pbCancel;
            CfContext.lpData = lpData;
            CfContext.lpProgressRoutine = lpProgressRoutine;
            CopyFileContext = &CfContext;
        }

        //
        //  Obtain the full set of streams we have to copy.  Since the Io subsystem does
        //  not provide us a way to find out how much space this information will take,
        //  we must iterate the call, doubling the buffer size upon each failure.
        //
        //  If the underlying file system does not support stream enumeration, we end up
        //  with a NULL buffer.  This is acceptable since we have at least a default
        //  data stream,
        //

        StreamInfoSize = 4096;
        do {
            StreamInfoBase = RtlAllocateHeap( RtlProcessHeap(),
                                              MAKE_TAG( TMP_TAG ),
                                              StreamInfoSize );

            if ( !StreamInfoBase ) {
                BaseSetLastNTError( STATUS_NO_MEMORY );
                leave;
            }

            Status = NtQueryInformationFile(
                        SourceFile,
                        &IoStatus,
                        (PVOID) StreamInfoBase,
                        StreamInfoSize,
                        FileStreamInformation
                        );

            if ( !NT_SUCCESS(Status) ) {
                //
                //  We failed the call.  Free up the previous buffer and set up
                //  for another pass with a buffer twice as large
                //

                RtlFreeHeap(RtlProcessHeap(), 0, StreamInfoBase);
                StreamInfoBase = NULL;
                StreamInfoSize *= 2;
            }
            else if( IoStatus.Information == 0 ) {
                //
                // There are no streams (SourceFile must be a directory).
                //
                RtlFreeHeap(RtlProcessHeap(), 0, StreamInfoBase);
                StreamInfoBase = NULL;
            }

        } while ( Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL );

        if (bIsNssFile && bOpenFilesAsReparsePoint && !NT_SUCCESS(Status))
        {
            //An NSS file should always have named streams.  If we can't
            //access them for some reason (most likely because there is
            //more than 64k of stream data and the source is across the
            //network), try to downgrade to docfile.
            if (SourceFile != INVALID_HANDLE_VALUE) {
                CloseHandle( SourceFile );
                SourceFile = INVALID_HANDLE_VALUE;
            }

            if (StreamInfoBase != NULL) {
                RtlFreeHeap (RtlProcessHeap(), 0, StreamInfoBase );
                StreamInfoBase = NULL;
            }

            bOpenFilesAsReparsePoint = FALSE;

            // Clear the open-reparse bit from the flags&attributes.  The
            // first CreateFile attempt will succeed, because we've already
            // set the correct SourceFileAccess/Sharing values.
            SourceFileFlagsAndAttributes &= ~FILE_FLAG_OPEN_REPARSE_POINT;

            goto retry_open_SourceFile;
        }

        //
        //  If a progress routine or a restartable copy was requested, obtain the
        //  full size of the entire file, including its alternate data streams, etc.
        //

        if ( ARGUMENT_PRESENT(lpProgressRoutine) ||
             (dwCopyFlags & COPY_FILE_RESTARTABLE) ) {

            if ( dwCopyFlags & COPY_FILE_RESTARTABLE ) {

                RestartState.Type = 0x7a9b;
                RestartState.Size = sizeof( RESTART_STATE );
                RestartState.CreationTime = BasicInformation.CreationTime;
                RestartState.WriteTime = BasicInformation.LastWriteTime;
                RestartState.EndOfFile = FileInformation.EndOfFile;
                RestartState.FileSize = FileInformation.EndOfFile;
                RestartState.NumberOfStreams = 0;
                RestartState.CurrentStream = 0;
                RestartState.LastKnownGoodOffset.QuadPart = 0;
            }

            if ( StreamInfoBase != NULL ) {

                ULONGLONG TotalSize = 0;

                StreamInfo = StreamInfoBase;
                while (TRUE) {
                    //
                    // Account for the size of this stream in the overall size of
                    // the file.
                    //

                    TotalSize += StreamInfo->StreamSize.QuadPart;
                    RestartState.NumberOfStreams++;

                    if (StreamInfo->NextEntryOffset == 0) {
                        break;
                    }
                    StreamInfo = (PFILE_STREAM_INFORMATION)((PCHAR) StreamInfo + StreamInfo->NextEntryOffset);
                }

                RestartState.FileSize.QuadPart =
                    CfContext.TotalFileSize.QuadPart = TotalSize;
                RestartState.NumberOfStreams--;
            }
        }

        //
        //  Set the Basic Info to change only the WriteTime
        //
        BasicInformation.CreationTime.QuadPart = 0;
        BasicInformation.LastAccessTime.QuadPart = 0;
        BasicInformation.FileAttributes = 0;

        //
        // Determine whether or not the copy operation should really be restartable
        // based on the actual, total file size.
        //

        if ( (dwCopyFlags & COPY_FILE_RESTARTABLE) &&
            ( RestartState.FileSize.QuadPart < (512 * 1024) ||
              (SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )) {
            dwCopyFlags &= ~COPY_FILE_RESTARTABLE;
        }

        //
        // Copy the default data stream, EAs, etc. to the output file
        //

        b = BaseCopyStream(
                lpExistingFileName,
                SourceFile,
                SourceFileAccess,
                lpNewFileName,
                NULL,
                &FileInformation.EndOfFile,
                &dwCopyFlags,
                &DestFile,
                &CopySize,
                &CopyFileContext,
                &RestartState,
                bOpenFilesAsReparsePoint,
                FileTagInformation.ReparseTag,
                &DestFileFsAttributes   // In: 0, Out: Correct value
                );

        if ( bOpenFilesAsReparsePoint &&
             !b &&
             !((GetLastError() == ERROR_FILE_EXISTS) && (dwCopyFlags & COPY_FILE_FAIL_IF_EXISTS)) ) {

            //
            //  Clean up.
            //

            if (!(SourceFileAttributes & FILE_ATTRIBUTE_READONLY)) {
                BaseMarkFileForDelete(DestFile, FILE_ATTRIBUTE_NORMAL);
            }

            if (DestFile != INVALID_HANDLE_VALUE) {
                CloseHandle( DestFile );
                DestFile = INVALID_HANDLE_VALUE;
            }

            if (SourceFileAttributes & FILE_ATTRIBUTE_READONLY) {

                //  Delete the destination file before retry
                //  Some servers (like Win9x) won't let us set file attributes
                //  on the handle we already have opened.  SetFileAttributesW
                //  can do the job nicely, so we'll call that to make sure that
                //  the read-only bit isn't set.
                //  We had to close DestFile before doing this because it was
                //  possibly opened share-exclusive.
                SetFileAttributesW(lpNewFileName, FILE_ATTRIBUTE_NORMAL);
                (void) DeleteFileW(lpNewFileName);
            }

            if (SourceFile != INVALID_HANDLE_VALUE) {
                CloseHandle( SourceFile );
                SourceFile = INVALID_HANDLE_VALUE;
            }

            if (StreamInfoBase != NULL) {
                RtlFreeHeap( RtlProcessHeap(), 0, StreamInfoBase );
                StreamInfoBase = NULL ;
            }

            //
            //  Try again the copy operation without inhibiting the reparse
            //  behavior for the source.
            //

            SourceFileFlagsAndAttributes &= ~FILE_FLAG_OPEN_REPARSE_POINT;
            bOpenFilesAsReparsePoint = FALSE;

            //
            //  Go to re-open the source file without inhibiting the reparse
            //  point behavior and try the copy again.
            //

            goto retry_open_SourceFile;
        }

        if ( b ) {

            //
            // Attempt to determine whether or not this file has any alternate
            // data streams associated with it.  If it does, attempt to copy each
            // to the output file.  Note that the stream information may have
            // already been obtained if a progress routine was requested.
            //

            if ( StreamInfoBase != NULL ) {
                DWORD StreamCount = 0;
                BOOLEAN CheckedForStreamCapable = FALSE;
                BOOLEAN IsStreamCapable = FALSE;
                StreamInfo = StreamInfoBase;

                while (TRUE) {

                    FILE_STREAM_INFORMATION DestStreamInformation;
                    Status = STATUS_SUCCESS;

                    //
                    //  Skip the default data stream since we've already copied
                    //  it.  Alas, this code is NTFS specific and documented
                    //  nowhere in the Io spec.
                    //

                    if (StreamInfo->StreamNameLength <= sizeof(WCHAR) ||
                        StreamInfo->StreamName[1] == ':') {
                        if (StreamInfo->NextEntryOffset == 0)
                            break;      // Done with streams
                        StreamInfo = (PFILE_STREAM_INFORMATION)((PCHAR) StreamInfo +
                                                                StreamInfo->NextEntryOffset);
                        continue;   // Move on to the next stream
                    }

                    StreamCount++;

                    if ( b == SUCCESS_RETURNED_STATE && CopyFileContext ) {
                        if ( StreamCount < RestartState.CurrentStream ) {
                            CopyFileContext->TotalBytesTransferred.QuadPart += StreamInfo->StreamSize.QuadPart;
                            }
                        else {
                            CopyFileContext->TotalBytesTransferred.QuadPart += RestartState.LastKnownGoodOffset.QuadPart;
                            }
                        }

                    //
                    // If we haven't already, verify that both the source and destination
                    // are really stream capable.
                    //

                    if( !CheckedForStreamCapable ) {

                        struct {
                            FILE_FS_ATTRIBUTE_INFORMATION Info;
                            WCHAR Buffer[ MAX_PATH ];
                        } FileFsAttrInfoBuffer;

                        CheckedForStreamCapable = TRUE;

                        // Check for the supports-streams bit in the dest filesystem.

                        Status = NtQueryVolumeInformationFile( DestFile,
                                                               &IoStatus,
                                                               &FileFsAttrInfoBuffer.Info,
                                                               sizeof(FileFsAttrInfoBuffer),
                                                               FileFsAttributeInformation );
                        if( NT_SUCCESS(Status) &&
                            (FileFsAttrInfoBuffer.Info.FileSystemAttributes & FILE_NAMED_STREAMS) ) {

                            // It seems redundant to check to see if the source is stream capable,
                            // since we already got back a successful stream enumeration, but some
                            // SMB servers (SCO VisionFS) return success but don't really support
                            // streams.

                            Status = NtQueryVolumeInformationFile( SourceFile,
                                                                   &IoStatus,
                                                                   &FileFsAttrInfoBuffer.Info,
                                                                   sizeof(FileFsAttrInfoBuffer),
                                                                   FileFsAttributeInformation );
                        }


                        if( !NT_SUCCESS(Status) ||
                            !(FileFsAttrInfoBuffer.Info.FileSystemAttributes & FILE_NAMED_STREAMS) ) {

                            if( NT_SUCCESS(Status) ) {
                                Status = STATUS_NOT_SUPPORTED;
                            }

                            if( dwCopyFlags & PRIVCOPY_FILE_VALID_FLAGS ) {
                                if( !BasepCopyFileCallback( TRUE,    // Continue by default
                                                            RtlNtStatusToDosError(Status),
                                                            CopyFileContext,
                                                            NULL,
                                                            PRIVCALLBACK_STREAMS_NOT_SUPPORTED,
                                                            SourceFile,
                                                            DestFile,
                                                            NULL )) {

                                    // LastError has been set, but we need it in Status
                                    // for compatibility with the rest of this routine.
                                    PTEB Teb = NtCurrentTeb();
                                    if ( Teb ) {
                                        Status = Teb->LastStatusValue;
                                    } else {
                                        Status = STATUS_INVALID_PARAMETER;
                                    }

                                    b = FALSE;
                                } else {
                                    // Ignore the named stream loss
                                    Status = STATUS_SUCCESS;
                                }
                            } else {
                                // Ignore the named stream loss.  We'll still try to copy the
                                // streams, though, since the target might be NT4 which didn't support
                                // the FILE_NAMED_STREAMS bit.  But since IsStreamCapable is FALSE,
                                // if there's an error, we'll ignore it.

                                Status = STATUS_SUCCESS;
                            }
                        }
                        else {
                            Status = STATUS_SUCCESS;
                            IsStreamCapable = TRUE;
                        }
                    }   // if( !CheckedForStreamCapable )


                    if ( b == TRUE ||
                        (b == SUCCESS_RETURNED_STATE &&
                         RestartState.CurrentStream == StreamCount) ) {

                        if ( b != SUCCESS_RETURNED_STATE ) {
                            RestartState.CurrentStream = StreamCount;
                            RestartState.LastKnownGoodOffset.QuadPart = 0;
                            }

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
                            &ObjectAttributes,
                            &StreamName,
                            0,
                            SourceFile,
                            NULL
                            );

                        //
                        // Inhibit reparse behavior when appropriate.
                        //

                        FlagsAndAttributes = FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY;
                        if ( bOpenFilesAsReparsePoint ) {
                            FlagsAndAttributes |= FILE_OPEN_REPARSE_POINT;
                        }

                        Status = NtCreateFile(
                                    &StreamHandle,
                                    GENERIC_READ | SYNCHRONIZE,
                                    &ObjectAttributes,
                                    &IoStatus,
                                    NULL,
                                    0,
                                    FILE_SHARE_READ,
                                    FILE_OPEN,
                                    FlagsAndAttributes,
                                    NULL,
                                    0
                                    );

                        //If we got a share violation, try again with
                        // FILE_SHARE_WRITE.
                        if ( Status == STATUS_SHARING_VIOLATION ) {
                            DWORD dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;

                            if (bIsNssFile) {
                                //For NSS files, also specify
                                //FILE_SHARE_DELETE
                                dwShare |= FILE_SHARE_DELETE;
                            }

                            Status = NtCreateFile(
                                        &StreamHandle,
                                        GENERIC_READ | SYNCHRONIZE,
                                        &ObjectAttributes,
                                        &IoStatus,
                                        NULL,
                                        0,
                                        dwShare,
                                        FILE_OPEN,
                                        FlagsAndAttributes,
                                        NULL,
                                        0
                                        );
                        }


                        if ( NT_SUCCESS(Status) ) {
                            DWORD dwCopyFlagsNamedStreams;

                            RtlCopyMemory( FileName, StreamName.Buffer, StreamName.Length );
                            FileName[StreamName.Length / sizeof( WCHAR )] = L'\0';
                            OutputStream = (HANDLE)NULL;

                            //
                            // For named streams, ignore the fail-if-exists flag.  If the dest
                            // file already existed at the time the copy started, then
                            // we would have failed on the copy of the unnamed stream.  So if
                            // a named stream exists, that means that it was created by some
                            // other process while we were copying the unnamed stream.  The
                            // assumption is that such a stream should be overwritten (this
                            // scenario can occur with SFM).
                            //

                            dwCopyFlagsNamedStreams = dwCopyFlags & ~COPY_FILE_FAIL_IF_EXISTS;

                            b = BaseCopyStream(
                                    lpExistingFileName,
                                    StreamHandle,
                                    SourceFileAccess,
                                    FileName,
                                    DestFile,
                                    &StreamInfo->StreamSize,
                                    &dwCopyFlagsNamedStreams,
                                    &OutputStream,
                                    &CopySize,
                                    &CopyFileContext,
                                    &RestartState,
                                    bOpenFilesAsReparsePoint,
                                    FileTagInformation.ReparseTag,
                                    &DestFileFsAttributes   // Set by first call to BaseCopyStream
                                    );
                            NtClose(StreamHandle);
                            if ( OutputStream ) {

                                //
                                //  We set the last write time on all streams
                                //  since there is a problem with RDR caching
                                //  open handles and closing them out of order.
                                //

                                if ( b ) {
                                    Status = NtSetInformationFile(
                                                OutputStream,
                                                &IoStatus,
                                                &BasicInformation,
                                                sizeof(BasicInformation),
                                                FileBasicInformation
                                                );
                                }
                                NtClose(OutputStream);
                            }

                        }   // Status = NtCreateFile; if( NT_SUCCESS(Status) )
                    }   // if ( b == TRUE || ...

                    if ( !NT_SUCCESS(Status) ) {
                        b = FALSE;
                        BaseSetLastNTError(Status);
                    }

                    if ( !b ) {

                        // If the target is known to be capable of multi-stream files,
                        // then this is a fatal error.  Otherwise we'll ignore it.

                        if( IsStreamCapable ) {
                            BaseMarkFileForDelete(DestFile,0);
                            break;  // while( TRUE )
                        } else {
                            Status = STATUS_SUCCESS;
                            b = TRUE;
                        }
                    }

                    if (StreamInfo->NextEntryOffset == 0) {
                        break;
                    }

                    StreamInfo =
                        (PFILE_STREAM_INFORMATION)((PCHAR) StreamInfo +
                                                   StreamInfo->NextEntryOffset);
                }   // while (TRUE)
            }   // if ( StreamInfoBase != NULL )
        }   // b = BaseCopyStream; if ( b ) ...


        //
        //  If the copy operation was successful, and it was restartable, and the
        //  output file was large enough that it was actually copied in a
        //  restartable manner, then copy the initial part of the file to its
        //  output.
        //
        //  Restartability is accomplished by placing a restart header at the
        //  head of the default data stream.  When the copy is complete, we
        //  overwite this header with the real user data.
        //

        if ( b && (dwCopyFlags & COPY_FILE_RESTARTABLE) ) {

            DWORD BytesToRead, BytesRead;
            DWORD BytesWritten;
            FILE_END_OF_FILE_INFORMATION EofInformation;

            SetFilePointer( SourceFile, 0, NULL, FILE_BEGIN );
            SetFilePointer( DestFile, 0, NULL, FILE_BEGIN );

            BytesToRead = sizeof(RESTART_STATE);
            if ( FileInformation.EndOfFile.QuadPart < sizeof(RESTART_STATE) ) {
                BytesToRead = FileInformation.EndOfFile.LowPart;
            }

            //
            //  Grab true data from the source stream
            //

            b = ReadFile(
                    SourceFile,
                    &RestartState,
                    BytesToRead,
                    &BytesRead,
                    NULL
                    );

            if ( b && (BytesRead == BytesToRead) ) {

                //
                //  Overwrite the restart header in the destination.
                //  After this point, the copy is no longer restartable
                //

                b = WriteFile(
                        DestFile,
                        &RestartState,
                        BytesRead,
                        &BytesWritten,
                        NULL
                        );

                if ( b && (BytesRead == BytesWritten) ) {
                    if ( BytesRead < sizeof(RESTART_STATE) ) {
                        EofInformation.EndOfFile.QuadPart = BytesWritten;
                        Status = NtSetInformationFile(
                                    DestFile,
                                    &IoStatus,
                                    &EofInformation,
                                    sizeof(EofInformation),
                                    FileEndOfFileInformation
                                    );
                        if ( !NT_SUCCESS(Status) ) {
                            BaseMarkFileForDelete(DestFile,0);
                            b = FALSE;
                        }
                    }
                } else {
                    BaseMarkFileForDelete(DestFile,0);
                    b = FALSE;
                }
            } else {
                BaseMarkFileForDelete(DestFile,0);
                b = FALSE;
            }
        }

        //
        // If the copy operation was successful, set the last write time for the
        // default steam so that it matches the input file.
        //

        if ( b ) {
            Status = NtSetInformationFile(
                DestFile,
                &IoStatus,
                &BasicInformation,
                sizeof(BasicInformation),
                FileBasicInformation
                );

            if ( Status == STATUS_SHARING_VIOLATION ) {

                //
                // IBM PC Lan Program (and other MS-NET servers) return
                // STATUS_SHARING_VIOLATION if an application attempts to perform
                // an NtSetInformationFile on a file handle opened for GENERIC_READ
                // or GENERIC_WRITE.
                //
                // If we get a STATUS_SHARING_VIOLATION on this API we want to:
                //
                //   1) Close the handle to the destination
                //   2) Re-open the file for FILE_WRITE_ATTRIBUTES
                //   3) Re-try the operation.
                //

                CloseHandle(DestFile);
                DestFile = INVALID_HANDLE_VALUE;

                //
                //  Re-Open the destination file.  Please note that we do this
                //  using the CreateFileW API.  The CreateFileW API allows you to
                //  pass NT native desired access flags, even though it is not
                //  documented to work in this manner.
                //
                //  Inhibit reparse behavior when appropriate.
                //

                FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
                if ( bOpenFilesAsReparsePoint ) {
                    FlagsAndAttributes |= FILE_FLAG_OPEN_REPARSE_POINT;
                }

                DestFile = CreateFileW(
                            lpNewFileName,
                            FILE_WRITE_ATTRIBUTES,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FlagsAndAttributes | FileFlagBackupSemantics,
                            NULL
                            );

                if ( DestFile != INVALID_HANDLE_VALUE ) {

                    //
                    //  If the open succeeded, we update the file information on
                    //  the new file.
                    //
                    //  Note that we ignore any errors from this point on.
                    //

                    NtSetInformationFile(
                        DestFile,
                        &IoStatus,
                        &BasicInformation,
                        sizeof(BasicInformation),
                        FileBasicInformation
                        );

                }
            }
        }

    } finally {

        *phSource = SourceFile;
        *phDest = DestFile;

        if (StreamInfoBase != NULL) {
            RtlFreeHeap( RtlProcessHeap(), 0, StreamInfoBase );
        }
    }

    return b;
}

BOOL
CopyFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    LPVOID lpData OPTIONAL,
    LPBOOL pbCancel OPTIONAL,
    DWORD dwCopyFlags
    )

/*

Routine Description:

    A file, its extended attributes, alternate data streams, and any other
    attributes can be copied using CopyFileEx.  CopyFileEx also provides
    callbacks and cancellability.

Arguments:

    lpExistingFileName - Supplies the name of an existing file that is to be
        copied.

    lpNewFileName - Supplies the name where a copy of the existing
        files data and attributes are to be stored.

    lpProgressRoutine - Optionally supplies the address of a callback routine
        to be called as the copy operation progresses.

    lpData - Optionally supplies a context to be passed to the progress callback
        routine.

    lpCancel - Optionally supplies the address of a boolean to be set to TRUE
        if the caller would like the copy to abort.

    dwCopyFlags - Specifies flags that modify how the file is to be copied:

        COPY_FILE_FAIL_IF_EXISTS - Indicates that the copy operation should
            fail immediately if the target file already exists.

        COPY_FILE_RESTARTABLE - Indicates that the file should be copied in
            restartable mode; i.e., progress of the copy should be tracked in
            the target file in case the copy fails for some reason.  It can
            then be restarted at a later date.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

*/

{
    HANDLE DestFile = INVALID_HANDLE_VALUE;
    HANDLE SourceFile = INVALID_HANDLE_VALUE;
    BOOL b;

    try
    {
        b = BasepCopyFileExW(
                lpExistingFileName,
                lpNewFileName,
                lpProgressRoutine OPTIONAL,
                lpData OPTIONAL,
                pbCancel OPTIONAL,
                dwCopyFlags,
                0,  // PrivCopyFile flags
                &DestFile,
                &SourceFile
                );

    }
    finally
    {
        if (DestFile != INVALID_HANDLE_VALUE) {
            CloseHandle( DestFile );
        }

        if (SourceFile != INVALID_HANDLE_VALUE) {
            CloseHandle( SourceFile );
        }
    }

    return(b);
}



BOOL
PrivCopyFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    LPVOID lpData OPTIONAL,
    LPBOOL pbCancel OPTIONAL,
    DWORD dwCopyFlags
    )

/*

Routine Description:

    A file, its extended attributes, alternate data streams, and any other
    attributes can be copied using CopyFileEx.  CopyFileEx also provides
    callbacks and cancellability.

Arguments:

    lpExistingFileName - Supplies the name of an existing file that is to be
        copied.

    lpNewFileName - Supplies the name where a copy of the existing
        files data and attributes are to be stored.

    lpProgressRoutine - Optionally supplies the address of a callback routine
        to be called as the copy operation progresses.

    lpData - Optionally supplies a context to be passed to the progress callback
        routine.

    lpCancel - Optionally supplies the address of a boolean to be set to TRUE
        if the caller would like the copy to abort.

    dwCopyFlags - Specifies flags that modify how the file is to be copied:

        COPY_FILE_FAIL_IF_EXISTS - Indicates that the copy operation should
            fail immediately if the target file already exists.

        COPY_FILE_RESTARTABLE - Indicates that the file should be copied in
            restartable mode; i.e., progress of the copy should be tracked in
            the target file in case the copy fails for some reason.  It can
            then be restarted at a later date.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

*/

{
    HANDLE DestFile = INVALID_HANDLE_VALUE;
    HANDLE SourceFile = INVALID_HANDLE_VALUE;
    BOOL b;

    if( (dwCopyFlags & COPY_FILE_FAIL_IF_EXISTS) &&
        (dwCopyFlags & PRIVCOPY_FILE_SUPERSEDE) ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );
    }

    try
    {
        b = BasepCopyFileExW(
                lpExistingFileName,
                lpNewFileName,
                lpProgressRoutine OPTIONAL,
                lpData OPTIONAL,
                pbCancel OPTIONAL,
                dwCopyFlags & COPY_FILE_VALID_FLAGS,    // Copy flags
                dwCopyFlags & ~COPY_FILE_VALID_FLAGS,   // Priv copy flags
                &DestFile,
                &SourceFile
                );

    }
    finally
    {
        if (DestFile != INVALID_HANDLE_VALUE) {
            CloseHandle( DestFile );
        }

        if (SourceFile != INVALID_HANDLE_VALUE) {
            CloseHandle( SourceFile );
        }
    }

    return(b);
}







DWORD
BasepChecksum(
    PUSHORT Source,
    ULONG Length
    )

/*++

Routine Description:

    Compute a partial checksum on a structure.

Arguments:

    Source - Supplies a pointer to the array of words for which the
        checksum is computed.

    Length - Supplies the length of the array in words.

Return Value:

    The computed checksum value is returned as the function value.

--*/

{

    ULONG PartialSum = 0;

    //
    // Compute the word wise checksum allowing carries to occur into the
    // high order half of the checksum longword.
    //

    while (Length--) {
        PartialSum += *Source++;
        PartialSum = (PartialSum >> 16) + (PartialSum & 0xffff);
    }

    //
    // Fold final carry into a single word result and return the resultant
    // value.
    //

    return (((PartialSum >> 16) + PartialSum) & 0xffff);
}

BOOL
BasepRemoteFile(
    HANDLE SourceFile,
    HANDLE DestinationFile
    )

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    FILE_FS_DEVICE_INFORMATION DeviceInformation;

    DeviceInformation.Characteristics = 0;
    Status = NtQueryVolumeInformationFile(
                SourceFile,
                &IoStatus,
                &DeviceInformation,
                sizeof(DeviceInformation),
                FileFsDeviceInformation
                );

    if ( NT_SUCCESS(Status) &&
         (DeviceInformation.Characteristics & FILE_REMOTE_DEVICE) ) {

        return TRUE;

    }

    Status = NtQueryVolumeInformationFile(
                    DestinationFile,
                    &IoStatus,
                    &DeviceInformation,
                    sizeof(DeviceInformation),
                    FileFsDeviceInformation
                    );
    if ( NT_SUCCESS(Status) &&
         DeviceInformation.Characteristics & FILE_REMOTE_DEVICE ) {

        return TRUE;
    }

    return FALSE;
}



DWORD
WINAPI
BasepOpenRestartableFile(
            HANDLE hSourceFile,
            LPCWSTR lpNewFileName,
            PHANDLE DestFile,
            DWORD CopyFlags,
            LPRESTART_STATE lpRestartState,
            LARGE_INTEGER *lpFileSize,
            LPCOPYFILE_CONTEXT *lpCopyFileContext,
            DWORD FlagsAndAttributes,
            BOOL OpenAsReparsePoint )

{   // BasepRestartCopyFile

    LPCOPYFILE_CONTEXT Context = *lpCopyFileContext;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    HANDLE OverwriteHandle;
    IO_STATUS_BLOCK IoStatus;
    RESTART_STATE RestartState;
    DWORD b = TRUE;
    ULONG BytesRead = 0;


    try {

        //
        // Note that setting the sequential scan flag is an optimization
        // here that works because of the way that the Cache Manager on
        // the target works vis-a-vis unmapping segments of the file
        // behind write operations.  This eventually allows the restart
        // section and the end of the file to both be mapped, which is
        // the desired result.
        //
        // Inhibit reparse behavior when appropriate.
        //

        FlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;

        if ( OpenAsReparsePoint ) {
            //
            // The target has to be opened as reparse point. If
            // this fails the source is to be closed and re-opened
            // without inhibiting the reparse point behavior.
            //

            FlagsAndAttributes |= FILE_FLAG_OPEN_REPARSE_POINT;
        }

        *DestFile = CreateFileW(
                       lpNewFileName,
                       GENERIC_READ | GENERIC_WRITE | DELETE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FlagsAndAttributes,
                       hSourceFile
                       );

        if( *DestFile == INVALID_HANDLE_VALUE ) {

            // Caller should attempt to create/overwrite the dest file
            b = TRUE;
            leave;
        }

        //
        //  The target file already exists, so determine whether
        //  a restartable copy was already proceeding.  If so,
        //  then continue;  else, check to see whether or not
        //  the target file can be replaced.  If not, bail with
        //  an error, otherwise simply overwrite the output file.
        //

        b = ReadFile(
                *DestFile,
                &RestartState,
                sizeof(RESTART_STATE),
                &BytesRead,
                NULL
                );
        if ( !b || BytesRead != sizeof(RESTART_STATE) ) {

            //
            // The file could not be read, or there were not
            // enough bytes to contain a restart record.  In
            // either case, if the output file cannot be
            // replaced, simply return an error now.
            //

            if ( CopyFlags & COPY_FILE_FAIL_IF_EXISTS ) {
                SetLastError( ERROR_ALREADY_EXISTS );
                b = FALSE;  // Fatal error
                leave;
            }

            // The caller should create/overwrite the dest file.
            b = TRUE;
            CloseHandle( *DestFile );
            *DestFile = INVALID_HANDLE_VALUE;
            leave;

        }

        //
        // Check the contents of the restart state just
        // read against the known contents of what would
        // be there if this were the same copy operation.
        //

        if ( RestartState.Type != 0x7a9b ||
             RestartState.Size != sizeof(RESTART_STATE) ||
             RestartState.FileSize.QuadPart != lpRestartState->FileSize.QuadPart ||
             RestartState.EndOfFile.QuadPart != lpRestartState->EndOfFile.QuadPart ||
             RestartState.NumberOfStreams != lpRestartState->NumberOfStreams ||
             RestartState.CreationTime.QuadPart != lpRestartState->CreationTime.QuadPart ||
             RestartState.WriteTime.QuadPart != lpRestartState->WriteTime.QuadPart ||
             RestartState.Checksum != BasepChecksum((PUSHORT)&RestartState,FIELD_OFFSET(RESTART_STATE,Checksum) >> 1) ) {

            if ( CopyFlags & COPY_FILE_FAIL_IF_EXISTS ) {
                b = FALSE;  // Fatal error
                SetLastError( ERROR_ALREADY_EXISTS );
                leave;
            }

            // The caller should create/overwrite the dest file.
            b = TRUE;
            CloseHandle( *DestFile );
            *DestFile = INVALID_HANDLE_VALUE;
            leave;

        }

        //
        // A valid restart state has been found.  Copy
        // the appropriate values into the internal
        // restart state so the operation can continue
        // from there.
        //

        lpRestartState->CurrentStream = RestartState.CurrentStream;
        lpRestartState->LastKnownGoodOffset.QuadPart = RestartState.LastKnownGoodOffset.QuadPart;
        if ( !RestartState.CurrentStream ) {

            // We were in the middle of copying the unnamed data stream.

            if ( Context ) {
                Context->TotalBytesTransferred.QuadPart = RestartState.LastKnownGoodOffset.QuadPart;
            }

            // We'll leave the handle in *DestFile, and the caller and pick up the
            // copy of this stream.

            b = TRUE;

        } else {

            // We were in the middle of copying a named data stream.

            if ( Context ) {
                ULONG ReturnCode;

                Context->TotalBytesTransferred.QuadPart = lpFileSize->QuadPart;
                Context->dwStreamNumber = RestartState.CurrentStream;

                if ( Context->lpProgressRoutine ) {
                    ReturnCode = Context->lpProgressRoutine(
                                    Context->TotalFileSize,
                                    Context->TotalBytesTransferred,
                                    *lpFileSize,
                                    Context->TotalBytesTransferred,
                                    1,
                                    CALLBACK_STREAM_SWITCH,
                                    hSourceFile,
                                    *DestFile,
                                    Context->lpData
                                    );
                } else {
                    ReturnCode = PROGRESS_CONTINUE;
                }

                if ( ReturnCode == PROGRESS_CANCEL ||
                    (Context->lpCancel && *Context->lpCancel) ) {
                    BaseMarkFileForDelete(
                        *DestFile,
                        0
                        );
                    BaseSetLastNTError(STATUS_REQUEST_ABORTED);
                    b = FALSE;
                    leave;
                }

                if ( ReturnCode == PROGRESS_STOP ) {
                    BaseSetLastNTError(STATUS_REQUEST_ABORTED);
                    b = FALSE;
                    leave;
                }

                if ( ReturnCode == PROGRESS_QUIET ) {
                    Context = NULL;
                    *lpCopyFileContext = NULL;
                }
            }

            b = SUCCESS_RETURNED_STATE;

        }   // if ( !RestartState.CurrentStream ) ... else
    }
    finally {

        if( b == FALSE &&
            *DestFile != INVALID_HANDLE_VALUE ) {
            CloseHandle( *DestFile );
            *DestFile = INVALID_HANDLE_VALUE;
        }


    }

    return( b );

}





BOOL
WINAPI
BasepCopyCompression( HANDLE hSourceFile,
                      HANDLE DestFile,
                      DWORD SourceFileAttributes,
                      DWORD DestFileAttributes,
                      DWORD DestFileFsAttributes,
                      DWORD CopyFlags,
                      LPCOPYFILE_CONTEXT *lpCopyFileContext )
/*++

Routine Description:

    This is an internal routine that copies the compression state during
    a copyfile.  If the source is compressed, that same compression
    algorithm is copied to the dest.  If that fails, an attempt is made
    to set the default compression.  Depending on the copy flags, it
    may alternatively be necessary to decompress the destination.


Arguments:

    hSourceFile - Provides a handle to the source file.

    DestFile - Provides a handle to the destination file.

    SourceFileAttributes - FileBasicInformation attributes queried from the
        source file.

    DestFileAttributes - FileBasicInformation attributes for the current
        state of the destination file.

    DestFileFsAttributes - FileFsAttributeInformation.FileSystemAttributes
        for the file system of the dest file.

    CopyFlags - Provides flags that modify how the copy is to proceed.  See
        CopyFileEx for details.

    lpCopyFileContext - Provides a pointer to a pointer to the context
        information to track callbacks, file sizes, etc. across streams during
        the copy operation.


Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.  The DestFile has already been marked
        for delete.

--*/

{   // BasepCopyCompression

    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status = STATUS_SUCCESS;
    LPCOPYFILE_CONTEXT Context = *lpCopyFileContext;
    BOOL SuccessReturn = FALSE;
    BOOL Canceled = FALSE;

    try
    {
        if( !(SourceFileAttributes & FILE_ATTRIBUTE_COMPRESSED) ) {

            // The source file is not compressed.  If necessary, decompress
            // the target.

            if( (DestFileAttributes & FILE_ATTRIBUTE_COMPRESSED) &&
                (CopyFlags & PRIVCOPY_FILE_SUPERSEDE) ) {

                // The source isn't compressed, but the dest is, and we don't
                // want to acquire attributes from the dest.  So we need to manually
                // decompress it.

                ULONG CompressionType = COMPRESSION_FORMAT_NONE;

                Status = NtFsControlFile(
                             DestFile,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatus,
                             FSCTL_SET_COMPRESSION,
                             &CompressionType,                    //  Input buffer
                             sizeof(CompressionType),             //  Input buffer length
                             NULL,                                //  Output buffer
                             0                                    //  Output buffer length
                             );
                if( !NT_SUCCESS(Status) ) {
                    // See if it's OK to ignore the error
                    if( !BasepCopyFileCallback( TRUE,    // Continue by default
                                                RtlNtStatusToDosError(Status),
                                                Context,
                                                NULL,
                                                PRIVCALLBACK_COMPRESSION_NOT_SUPPORTED,
                                                hSourceFile,
                                                DestFile,
                                                &Canceled )) {


                        BaseMarkFileForDelete( DestFile, 0 );
                        BaseSetLastNTError( Status );
                        leave;
                    } else {
                        Status = STATUS_SUCCESS;
                    }
                }

            }

        }   // if( !(SourceFileAttributes & FILE_ATTRIBUTE_COMPRESSED) )

        else {

            // The source file is compressed.  Does the target filesystem
            // even support compression?

            if( !(FILE_FILE_COMPRESSION & DestFileFsAttributes) ) {

                // No, it won't be compressable.  See if it's OK to continue.

                if( !BasepCopyFileCallback( TRUE,    // Continue by default
                                            ERROR_NOT_SUPPORTED,
                                            Context,
                                            NULL,
                                            PRIVCALLBACK_COMPRESSION_NOT_SUPPORTED,
                                            hSourceFile,
                                            DestFile,
                                            &Canceled )) {

                    if( Canceled ) {
                        BaseMarkFileForDelete(
                            DestFile,
                            0 );
                    }
                    leave;
                }
            }   // if( !(FILE_FILE_COMPRESSION & *DestFileFsAttributes) )

            else {

                // Target volume supports compression.  Compress the target file if
                // it's not already.

                if( !(DestFileAttributes & FILE_ATTRIBUTE_COMPRESSED) ) {

                    USHORT CompressionType;

                    // Get the source file's compression type

                    Status = NtFsControlFile(
                                 hSourceFile,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatus,
                                 FSCTL_GET_COMPRESSION,
                                 NULL,                                //  Input buffer
                                 0,                                   //  Input buffer length
                                 &CompressionType,                    //  Output buffer
                                 sizeof(CompressionType)              //  Output buffer length
                                 );
                    if( NT_SUCCESS(Status) ) {

                        // Set the compression type on the target

                        Status = NtFsControlFile(
                                     DestFile,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatus,
                                     FSCTL_SET_COMPRESSION,
                                     &CompressionType,                    //  Input buffer
                                     sizeof(CompressionType),             //  Input buffer length
                                     NULL,                                //  Output buffer
                                     0                                    //  Output buffer length
                                     );

                        // If that didn't work, try the default compression
                        // format (maybe we're copying from uplevel to downlevel).

                        if( !NT_SUCCESS(Status) &&
                            COMPRESSION_FORMAT_DEFAULT != CompressionType ) {

                            CompressionType = COMPRESSION_FORMAT_DEFAULT;
                            Status = NtFsControlFile(
                                         DestFile,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &IoStatus,
                                         FSCTL_SET_COMPRESSION,
                                         &CompressionType,                    //  Input buffer
                                         sizeof(CompressionType),             //  Input buffer length
                                         NULL,                                //  Output buffer
                                         0                                    //  Output buffer length
                                         );
                        }
                    }   // FSCTL_GET_COMPRESSION ... if( NT_SUCCESS(Status) )

                    // If something went wrong and we couldn't compress it, there's a good
                    // chance that the caller doesn't want this to be fatal.  Ask and find
                    // out.

                    if( !NT_SUCCESS(Status) ) {
                        BOOL Canceled = FALSE;

                        if( !BasepCopyFileCallback( TRUE,    // Continue by default
                                                    RtlNtStatusToDosError(Status),
                                                    Context,
                                                    NULL,
                                                    PRIVCALLBACK_COMPRESSION_FAILED,
                                                    hSourceFile,
                                                    DestFile,
                                                    &Canceled )) {
                            if( Canceled ) {
                                BaseMarkFileForDelete(
                                    DestFile,
                                    0 );
                            }
                            leave;
                        }
                    }
                }   // if( !(DestFileAttributes & FILE_FILE_COMPRESSION) )
            }   // if( !(FILE_FILE_COMPRESSION & *DestFileFsAttributes) )
        }   // if( !(SourceFileAttributes & FILE_ATTRIBUTE_COMPRESSED) ) ... else

        SuccessReturn = TRUE;
    }
    finally
    {
    }

    return( SuccessReturn );
}



typedef BOOL (WINAPI *ENCRYPTFILEWPTR)(LPCWSTR);
typedef BOOL (WINAPI *DECRYPTFILEWPTR)(LPCWSTR, DWORD);

BOOL
WINAPI
BasepCopyEncryption( HANDLE hSourceFile,
                     LPCWSTR lpNewFileName,
                     PHANDLE DestFile,
                     POBJECT_ATTRIBUTES Obja,
                     DWORD DestFileAccess,
                     DWORD DestFileSharing,
                     DWORD CreateDisposition,
                     DWORD CreateOptions,
                     DWORD SourceFileAttributes,
                     DWORD SourceFileAttributesMask,
                     PDWORD DestFileAttributes,
                     DWORD DestFileFsAttributes,
                     DWORD CopyFlags,
                     LPCOPYFILE_CONTEXT *lpCopyFileContext )
/*++

Routine Description:

    This is an internal routine that copies the encryption state during
    a copyfile.  Depending on the copy flags, it may be necessary to
    decompress the destination.  To encrypt/decrypt a file it is necessary
    to close the current handle, encrypt/decrypt, and reopen.

Arguments:

    hSourceFile - Provides a handle to the source file.

    lpNewFileName - Provides a name for the target file/stream.

    Obja - ObjectAttributes structure for the destination file.

    DestFileAccess - ACCESS_MASK to use when opening the dest.

    DestFileSharing - Sharing options to use when openting the dest.

    CreateDisposition - Creation/disposition options for opening the dest.

    SourceFileAttributes - FileBasicInformation attributes queried from the
        source file.

    SourceFileAttributesMask - the attributes from the source that are intended
        to be set on the dest.

    DestFileAttributes - FileBasicInformation attributes for the current
        state of the destination file.  This value is updated to reflect
        changes to the encryption state of the dest file.

    DestFileFsAttributes - FileFsAttributeInformation.FileSystemAttributes
        for the file system of the dest file.

    CopyFlags - Provides flags that modify how the copy is to proceed.  See
        CopyFileEx for details.

    lpCopyFileContext - Provides a pointer to a pointer to the context
        information to track callbacks, file sizes, etc. across streams during
        the copy operation.


Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.  The DestFile has already been marked
        for delete.

--*/

{   // BasepCopyEncryption

    NTSTATUS Status = 0;
    BOOL SuccessReturn = FALSE;
    BOOL EncryptFile = FALSE;
    BOOL DecryptFile = FALSE;
    HANDLE Advapi32;
    ENCRYPTFILEWPTR EncryptFileWPtr = NULL;
    DECRYPTFILEWPTR DecryptFileWPtr = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    LPCOPYFILE_CONTEXT Context = *lpCopyFileContext;


    try
    {
        if( (SourceFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) &&
            (SourceFileAttributesMask & FILE_ATTRIBUTE_ENCRYPTED) &&
            !(*DestFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ) {

            // We tried to copy over encryption, but it didn't stick:
            // *  This may be a system file, encryption is not supported on
            //    system files.
            // *  If this is a non-directory file, then encryption is not
            //    supported on the target file system.
            // *  If this is a directory file, then we must try to encrypt
            //    it manually (since we opened it, rather than creating it).
            //    It still may not be possible but we'll have to try to
            //    find out.

            if( (SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                &&
                !(*DestFileAttributes & FILE_ATTRIBUTE_SYSTEM) ) {
                EncryptFile = TRUE;
            }

        } else if( !(SourceFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) &&
                   (*DestFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) &&
                   (CopyFlags & PRIVCOPY_FILE_SUPERSEDE) ) {

            // The source is decrypted, the destination was encrypted, and the
            // caller specified that the source should be copied as-is.  So
            // we must manually decrypt the destination.  This can happen if
            // the dest file already existed and was encrypted.

            DecryptFile = TRUE;
        }


        if( DecryptFile || EncryptFile ) {

            NtClose( *DestFile );
            *DestFile = INVALID_HANDLE_VALUE;

            Advapi32 = LoadLibrary("advapi32.dll");
            if( Advapi32 == NULL ) {
                leave;
            }

            if( EncryptFile ) {
                EncryptFileWPtr = (ENCRYPTFILEWPTR)GetProcAddress(Advapi32, "EncryptFileW");
                if( EncryptFileWPtr == NULL ) {
                    leave;
                }

                if( EncryptFileWPtr(lpNewFileName) )
                    *DestFileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
            } else {
                DecryptFileWPtr = (DECRYPTFILEWPTR)GetProcAddress(Advapi32, "DecryptFileW");
                if( DecryptFileWPtr == NULL ) {
                    leave;
                }

                if( DecryptFileWPtr(lpNewFileName, 0) )
                    *DestFileAttributes &= ~FILE_ATTRIBUTE_ENCRYPTED;
            }
            FreeLibrary( Advapi32 );

            Status = NtCreateFile(
                        DestFile,
                        DestFileAccess,
                        Obja,
                        &IoStatusBlock,
                        NULL,
                        SourceFileAttributes & FILE_ATTRIBUTE_VALID_FLAGS & SourceFileAttributesMask,
                        DestFileSharing,
                        CreateDisposition,
                        CreateOptions,
                        NULL,
                        0
                        );
            if( !NT_SUCCESS(Status) ) {
                *DestFile = INVALID_HANDLE_VALUE;
                BaseSetLastNTError(Status);
                leave;
            }
        }   // if( DecryptFile || EncryptFile )

        // If it's still not encrypted and we're allowed to call back the caller,
        // see if it's OK to drop encryption.

        if( (SourceFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) &&
            !(*DestFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ) {

            BOOL Canceled = FALSE;
            DWORD dwCallbackReason = 0;
            LONG lError = ERROR_NOT_SUPPORTED;

            // Either there was an encryption problem (e.g. no keys available)
            // or the target just doesn't support encryption.  See if it's OK
            // to continue with the copy.

            if( DestFileFsAttributes & FILE_SUPPORTS_ENCRYPTION ) {

                if( !(SourceFileAttributesMask & FILE_ATTRIBUTE_ENCRYPTED) ) {
                    // We opened the file with encryption turned off, so we must
                    // have gotten an access-denied on the first try.

                    lError = ERROR_ACCESS_DENIED;
                    dwCallbackReason = PRIVCALLBACK_ENCRYPTION_FAILED;
                }

                else if( *DestFileAttributes & FILE_ATTRIBUTE_SYSTEM )
                    dwCallbackReason = PRIVCALLBACK_CANT_ENCRYPT_SYSTEM_FILE;
                else
                    dwCallbackReason = PRIVCALLBACK_ENCRYPTION_FAILED;
            }
            else
                dwCallbackReason = PRIVCALLBACK_ENCRYPTION_NOT_SUPPORTED;

            if( !BasepCopyFileCallback( TRUE,    // Continue by default
                                       lError,
                                       Context,
                                       NULL,
                                       dwCallbackReason,
                                       hSourceFile,
                                       *DestFile,
                                       &Canceled )) {
                if( Canceled ) {
                    BaseMarkFileForDelete(
                        DestFile,
                        0 );
                }
                leave;
            }
        }

        SuccessReturn = TRUE;

    }
    finally
    {
    }

    return( SuccessReturn );

}




DWORD
WINAPI
BaseCopyStream(
    OPTIONAL LPCWSTR lpExistingFileName,
    HANDLE hSourceFile,
    ACCESS_MASK SourceFileAccess OPTIONAL,
    LPCWSTR lpNewFileName,
    HANDLE hTargetFile OPTIONAL,
    LARGE_INTEGER *lpFileSize,
    LPDWORD lpCopyFlags,
    LPHANDLE lpDestFile,
    LPDWORD lpCopySize,
    LPCOPYFILE_CONTEXT *lpCopyFileContext,
    LPRESTART_STATE lpRestartState OPTIONAL,
    BOOL OpenFileAsReparsePoint,
    DWORD dwReparseTag,
    PDWORD DestFileFsAttributes
    )

/*++

Routine Description:

    This is an internal routine that copies an entire file (default data stream
    only), or a single stream of a file.  If the hTargetFile parameter is
    present, then only a single stream of the output file is copied.  Otherwise,
    the entire file is copied.

Arguments:

    hSourceFile - Provides a handle to the source file.

    SourceFileAccess - The ACCESS_MASK bits used to open the source file handle.
        This variable is only used with the PRIVCOPY_FILE_* flags.

    lpNewFileName - Provides a name for the target file/stream.

    hTargetFile - Optionally provides a handle to the target file.  If the
        stream being copied is an alternate data stream, then this handle must
        be provided.

    lpFileSize - Provides the size of the input stream.

    lpCopyFlags - Provides flags that modify how the copy is to proceed.  See
        CopyFileEx for details.

    lpDestFile - Provides a variable to store the handle to the target file.

    lpCopySize - Provides variable to store size of copy chunks to be used in
        copying the streams.  This is set for the file, and then reused on
        alternate streams.

    lpCopyFileContext - Provides a pointer to a pointer to the context
        information to track callbacks, file sizes, etc. across streams during
        the copy operation.

    lpRestartState - Optionally provides storage to maintain restart state
        during the copy operation.  This pointer is only valid if the caller
        has specified the COPY_FILE_RESTARTABLE flag in the lpCopyFlags word.

    OpenFileAsReparsePoint - Flag to indicate whether the target file is to
        be opened as a reparse point or not.

    DestFileFsAttributes - If hTargetFile is present, provides a location to
        store the destination file's filesystem attributes.  If hTargetFile
        is not present, provides those attributes to this routine.

Return Value:

    TRUE - The operation was successful.

    SUCCESS_RETURNED_STATE - The operation was successful, but extended
        information was returned in the restart state structure.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{   // BaseCopyStream

    HANDLE DestFile = INVALID_HANDLE_VALUE;
    HANDLE Section;
    NTSTATUS Status;
    PVOID SourceBase, IoDestBase;
    PCHAR SourceBuffer;
    LARGE_INTEGER SectionOffset;
    LARGE_INTEGER BytesWritten;
    SIZE_T BigViewSize;
    ULONG ViewSize;
    ULONG BytesToWrite;
    ULONG BytesRead;
    FILE_BASIC_INFORMATION FileBasicInformationData;
    FILE_END_OF_FILE_INFORMATION EndOfFileInformation;
    IO_STATUS_BLOCK IoStatus;
    LPCOPYFILE_CONTEXT Context = *lpCopyFileContext;
    DWORD ReturnCode;
    DWORD b;
    BOOL Restartable;
    DWORD ReturnValue = FALSE;
    DWORD WriteCount = 0;
    DWORD FlagsAndAttributes;
    DWORD DesiredAccess;
    DWORD DestFileAccess;
    DWORD DestFileSharing;
    DWORD DesiredCreateDisposition;
    DWORD CreateDisposition;
    BOOL Canceled = FALSE;
    BOOL fIsNssFile = (dwReparseTag == IO_REPARSE_TAG_NSS ||
                       dwReparseTag == IO_REPARSE_TAG_NSSRECOVER);

    DWORD SourceFileAttributes;
    DWORD SourceFileAttributesMask;
    BOOL fSparseSource;
    DWORD BlockSize;
    BOOL fSkipBlock;
    UNICODE_STRING DestFileName;
    PVOID DestFileNameBuffer = NULL;
    RTL_RELATIVE_NAME DestRelativeName;
    OBJECT_ATTRIBUTES Obja;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    FILE_EA_INFORMATION EaInfo;
    PFILE_FULL_EA_INFORMATION EaBuffer = NULL;
    ULONG EaSize = 0;
    BOOL EasDropped = FALSE;
    IO_STATUS_BLOCK IoStatusBlock;

    // Default the size of copy chunks
    *lpCopySize = BASE_COPY_FILE_CHUNK;

    //
    //  Get times and attributes for the file if the entire file is being
    //  copied
    //

    Status = NtQueryInformationFile(
                hSourceFile,
                &IoStatus,
                (PVOID) &FileBasicInformationData,
                sizeof(FileBasicInformationData),
                FileBasicInformation
                );

    SourceFileAttributes = NT_SUCCESS(Status) ?
                             FileBasicInformationData.FileAttributes :
                             0;

    if ( !ARGUMENT_PRESENT(hTargetFile) ) {

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return FALSE;
        }
    } else {

        //
        //  A zero in the file's attributes informs latter DeleteFile that
        //  this code does not know what the actual file attributes are so
        //  that this code does not actually have to retrieve them for each
        //  stream, nor does it have to remember them across streams.  The
        //  error path will simply get them if needed.
        //

        FileBasicInformationData.FileAttributes = 0;
    }

    //
    // For now, only maintain sparse information on NSS files that are not
    // being downgraded.
    //
    fSparseSource = fIsNssFile && OpenFileAsReparsePoint;

    //
    // We don't allow restartable copies of directory files, because the
    // unnamed data stream is used to store restart context, and directory files
    // don't have an unnamed data stream.
    //

    Restartable = (*lpCopyFlags & COPY_FILE_RESTARTABLE) != 0;
    if( Restartable && SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
        Restartable = FALSE;
        *lpCopyFlags &= ~COPY_FILE_RESTARTABLE;
    }


    try {

        //
        // Create the destination file or alternate data stream
        //

        SourceBase = NULL;
        IoDestBase = NULL;
        Section = NULL;

        if ( !ARGUMENT_PRESENT(hTargetFile) ) {

            ULONG CreateOptions = 0, DesiredCreateOptions = 0;
            BOOL TranslationStatus = FALSE;
            PFILE_FULL_EA_INFORMATION EaBufferToUse = NULL;
            DWORD SourceFileFsAttributes = 0;
            ULONG EaSizeToUse = 0;

            // We're being called to copy the unnamed stream of the file, and
            // we need to create the file itself.

            DWORD DestFileAttributes = 0;
            struct {
                FILE_FS_ATTRIBUTE_INFORMATION Info;
                WCHAR Buffer[ MAX_PATH ];
            } FileFsAttrInfoBuffer;

            //
            // Begin by determining how the target file is to be opened based
            // on whether or not the copy operation is to be restartable.
            //

            if ( Restartable ) {

                b = BasepOpenRestartableFile( hSourceFile,
                                              lpNewFileName,
                                              &DestFile,
                                              *lpCopyFlags,
                                              lpRestartState,
                                              lpFileSize,
                                              lpCopyFileContext,
                                              FileBasicInformationData.FileAttributes,
                                              OpenFileAsReparsePoint );

                if( b == SUCCESS_RETURNED_STATE ) {
                    // We've picked up in the middle of a restartable copy.
                    // The destination file handle is in DestFile, which will
                    // be given back to our caller below in the finally.

                    if ( BasepRemoteFile(hSourceFile,DestFile) ) {
                        *lpCopySize = BASE_COPY_FILE_CHUNK - 4096;
                    }
                    ReturnValue = b;
                    leave;
                } else if( b == FALSE ) {
                    // There was a fatal error.
                    leave;
                }

                // Otherwise we should copy the first stream.  If we are to restart copying
                // in that stream, DestFile will be valid.

            }

            //
            // If the dest file is not already opened (the restart case), open it now.
            //

            if( DestFile == INVALID_HANDLE_VALUE ) {

                BOOL EndsInSlash = FALSE;
                UNICODE_STRING Win32NewFileName;
                PUNICODE_STRING lpConsoleName = NULL;
                FILE_BASIC_INFORMATION DestBasicInformation;

                //
                // Determine the create options
                //

                CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;

                if( SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                    CreateOptions |= FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT;
                else
                    CreateOptions |= FILE_NON_DIRECTORY_FILE  | FILE_SEQUENTIAL_ONLY;

                if( *lpCopyFlags & (PRIVCOPY_FILE_BACKUP_SEMANTICS|PRIVCOPY_FILE_OWNER_GROUP) )
                    CreateOptions |= FILE_OPEN_FOR_BACKUP_INTENT;


                //
                // Determine the create disposition
                //
                // Directory files are copied with merge semantics.  The rationale
                // is that copying of a directory tree has merge semantics wrt the
                // contained files, so copying of a directory file should also have
                // merge semantics.
                //

                if( SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                    CreateDisposition = (*lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS) ? FILE_CREATE : FILE_OPEN_IF;
                else
                    CreateDisposition = (*lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS) ? FILE_CREATE : FILE_OVERWRITE_IF;


                //
                // Determine what access is necessary based on what is being copied
                //

                DesiredAccess = SYNCHRONIZE | FILE_READ_ATTRIBUTES | GENERIC_WRITE | DELETE;

                if( SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                    // We may or may not be able to get FILE_WRITE_DATA access, necessary for
                    // setting compression.
                    DesiredAccess &= ~GENERIC_WRITE;
                    DesiredAccess |= FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_LIST_DIRECTORY;
                }


                if( *lpCopyFlags & PRIVCOPY_FILE_METADATA ) {
                    // We need read access for compression, write_dac for the DACL
                    DesiredAccess |= GENERIC_READ | WRITE_DAC;
                }

                if( *lpCopyFlags & PRIVCOPY_FILE_OWNER_GROUP ) {
                    DesiredAccess |= WRITE_OWNER;
                }

                if( (*lpCopyFlags & PRIVCOPY_FILE_SACL)
                    &&
                    (SourceFileAccess & ACCESS_SYSTEM_SECURITY) ) {
                    // Don't bother trying to get access_system_security unless it was
                    // successfully obtained on the source (requires SeSecurityPrivilege)
                    DesiredAccess |= ACCESS_SYSTEM_SECURITY;
                }

                SourceFileAttributesMask = ~0;

                if ( OpenFileAsReparsePoint && !fIsNssFile ) {
                    //
                    // The target has to be opened as reparse point. If the open
                    // below fails, the source is to be closed and re-opened
                    // without inhibiting the reparse point behavior.
                    //
                    // For NSS files, we don't want to include
                    // FILE_FLAG_OPEN_REPARSE_POINT, and we don't want to
                    // pass OPEN_ALWAYS, since doing so leaves us with extra
                    // streams that we don't need.

                    CreateOptions |= FILE_OPEN_REPARSE_POINT;
                    DesiredAccess = (DesiredAccess & ~DELETE) | GENERIC_READ;
                    CreateDisposition = (*lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS) ? FILE_CREATE : FILE_OPEN_IF;
                }

                if ( fIsNssFile && !OpenFileAsReparsePoint ) {
                    //Copying an NSS file across the network or to a
                    //local downlevel file system.  Try to pass
                    //FILE_COPY_STRUCTURED_STORAGE, we may retry later
                    //without it.
                    //We need read access to do the fsctl.
                    DesiredAccess |= GENERIC_READ;
                    CreateOptions |= FILE_COPY_STRUCTURED_STORAGE;
                }

                DesiredCreateOptions = CreateOptions;
                DesiredCreateDisposition = CreateDisposition;

                //
                // Get the Win32 path in a unicode_string, and get the NT path
                //

                RtlInitUnicodeString( &Win32NewFileName, lpNewFileName );

                if ( lpNewFileName[(Win32NewFileName.Length >> 1)-1] == (WCHAR)'\\' ) {
                    EndsInSlash = TRUE;
                }
                else {
                    EndsInSlash = FALSE;
                }

                TranslationStatus = RtlDosPathNameToNtPathName_U(
                                        lpNewFileName,
                                        &DestFileName,
                                        NULL,
                                        &DestRelativeName
                                        );

                if ( !TranslationStatus ) {
                    SetLastError(ERROR_PATH_NOT_FOUND);
                    DestFile = INVALID_HANDLE_VALUE;
                    leave;
                    }
                DestFileNameBuffer = DestFileName.Buffer;

                if ( DestRelativeName.RelativeName.Length ) {
                    DestFileName = *(PUNICODE_STRING)&DestRelativeName.RelativeName;
                }
                else {
                    DestRelativeName.ContainingDirectory = NULL;
                }

                InitializeObjectAttributes(
                    &Obja,
                    &DestFileName,
                    OBJ_CASE_INSENSITIVE,
                    DestRelativeName.ContainingDirectory,
                    NULL
                    );

                SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
                SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
                SecurityQualityOfService.EffectiveOnly = TRUE;
                SecurityQualityOfService.Length = sizeof( SECURITY_QUALITY_OF_SERVICE );

                Obja.SecurityQualityOfService = &SecurityQualityOfService;

                //
                //  Get the EAs
                //

                EaBuffer = NULL;
                EaSize = 0;

                Status = NtQueryInformationFile(
                            hSourceFile,
                            &IoStatusBlock,
                            &EaInfo,
                            sizeof(EaInfo),
                            FileEaInformation
                            );
                if ( NT_SUCCESS(Status) && EaInfo.EaSize ) {

                    EaSize = EaInfo.EaSize;

                    do {

                        EaSize *= 2;
                        EaBuffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), EaSize);
                        if ( !EaBuffer ) {
                            BaseSetLastNTError(STATUS_NO_MEMORY);
                            leave;
                        }

                        Status = NtQueryEaFile(
                                    hSourceFile,
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
                            RtlFreeHeap(RtlProcessHeap(), 0,EaBuffer);
                            EaBuffer = NULL;
                            IoStatusBlock.Information = 0;
                        }

                    } while ( Status == STATUS_BUFFER_OVERFLOW ||
                              Status == STATUS_BUFFER_TOO_SMALL );


                    EaSize = (ULONG)IoStatusBlock.Information;

                }   // if ( NT_SUCCESS(Status) && EaInfo.EaSize )


                //
                // Open the destination file.  If the destination is a console name,
                // open as such, otherwise loop until we find a way to open it with
                // NtCreateFile.
                //

                DestFileAccess = DesiredAccess;
                DestFileSharing = 0;
                EaBufferToUse = EaBuffer;
                EaSizeToUse = EaSize;

                if( (lpConsoleName = BaseIsThisAConsoleName( &Win32NewFileName, GENERIC_WRITE )) ) {

                    DestFileAccess = DesiredAccess = GENERIC_WRITE;
                    DestFileSharing = FILE_SHARE_READ | FILE_SHARE_WRITE;

                    if( EaBuffer != NULL )
                        EasDropped = TRUE;  // We're not copying the EAs

                    DestFile= OpenConsoleW( lpConsoleName->Buffer,
                                            DestFileAccess,
                                            FALSE,  // Not inheritable
                                            DestFileSharing
                                           );

                    if ( DestFile == INVALID_HANDLE_VALUE ) {
                        BaseSetLastNTError(STATUS_ACCESS_DENIED);
                        NtClose( DestFile );
                        DestFile = INVALID_HANDLE_VALUE;
                        leave;
                    }

                }

                //
                // Turn off FILE_ATTRIBUTE_OFFLINE for destination
                //
                SourceFileAttributes &= ~FILE_ATTRIBUTE_OFFLINE;

                while( DestFile == NULL || DestFile == INVALID_HANDLE_VALUE ) {

                    // Attempt to create the destination

                    Status = NtCreateFile(
                                &DestFile,
                                DestFileAccess,
                                &Obja,
                                &IoStatusBlock,
                                NULL,
                                SourceFileAttributes & FILE_ATTRIBUTE_VALID_FLAGS & SourceFileAttributesMask,
                                DestFileSharing,
                                CreateDisposition,
                                CreateOptions,
                                EaBufferToUse,
                                EaSizeToUse
                                );

                    // If this was successful, then break out of this while loop.
                    // The remaining code in this loop attempt to recover from the problem,
                    // then it loops back to the top and attempt the NtCreateFile again.

                    if( NT_SUCCESS(Status) ) {

                        if ( fIsNssFile && !OpenFileAsReparsePoint &&
                             (CreateOptions & FILE_COPY_STRUCTURED_STORAGE) ) {
                            //Check to see if the destination has CNSS
                            //loaded.  If it doesn't, we want to go back
                            //through and do this create with
                            //FILE_COPY_STRUCTURED_STORAGE.  We need
                            //this check because passing
                            //FILE_COPY_STRUCTURED_STORAGE to NT4 or
                            //Win95 results in a directory being
                            //created.
                            NSS_CONTROL nssc;
                            nssc.code = NSS_CONTROL_ISNSSFILE;
                            nssc.param=0;
                            Status = NtFsControlFile(
                                DestFile,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_NSS_RCONTROL,
                                &nssc,
                                sizeof(NSS_CONTROL),
                                NULL,
                                0);

                            if ( !NT_SUCCESS(Status) ) {
                                CreateOptions &= ~FILE_COPY_STRUCTURED_STORAGE;
                                //Get rid of the file we just created.
                                BaseMarkFileForDelete(DestFile, 0);
                                NtClose(DestFile);
                                DestFile = INVALID_HANDLE_VALUE;
                                continue;
                            }
                        }
                        else if( (SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                                 CreateDisposition == FILE_OPEN &&
                                 (DestFileAccess & FILE_WRITE_DATA) == FILE_WRITE_DATA &&
                                 (CreateOptions & FILE_DIRECTORY_FILE) == FILE_DIRECTORY_FILE ) {

                            //
                            // If we're copying to NT4, a previous iteration through this
                            // large while loop switched the CreateDisposition from
                            // FILE_OPENIF to FILE_OPEN; otherwise, NT4 fails the open
                            // (when passing FILE_OPENIF and FILE_WRITE_DATA to a directory
                            // file that already exists).  The open worked, but the problem
                            // is that now if we need to set compression on the target, we'll
                            // get status_invalid_parameter because the FILE_DIRECTORY_FILE
                            // CreateOption was set.  So, to allow compression to work, and
                            // since at this point we already know the target is a directory
                            // file, we can re-open it without that create option.
                            //

                            CreateOptions &= ~FILE_DIRECTORY_FILE;

                            NtClose( DestFile );
                            Status = NtCreateFile(
                                        &DestFile,
                                        DestFileAccess,
                                        &Obja,
                                        &IoStatusBlock,
                                        NULL,
                                        SourceFileAttributes & FILE_ATTRIBUTE_VALID_FLAGS & SourceFileAttributesMask,
                                        DestFileSharing,
                                        CreateDisposition,
                                        CreateOptions,
                                        EaBufferToUse,
                                        EaSizeToUse
                                        );
                            if( !NT_SUCCESS(Status) ) {

                                // But if that didn't work, go back to the combination that
                                // did (this happens on Samba servers).

                                CreateOptions |= FILE_DIRECTORY_FILE;
                                Status = NtCreateFile(
                                            &DestFile,
                                            DestFileAccess,
                                            &Obja,
                                            &IoStatusBlock,
                                            NULL,
                                            SourceFileAttributes & FILE_ATTRIBUTE_VALID_FLAGS & SourceFileAttributesMask,
                                            DestFileSharing,
                                            CreateDisposition,
                                            CreateOptions,
                                            EaBufferToUse,
                                            EaSizeToUse
                                            );

                                if( !NT_SUCCESS(Status) ) {
                                    DestFile = INVALID_HANDLE_VALUE;
                                    BaseSetLastNTError( Status );
                                    leave;
                                }
                            }
                        }
                        else if( (SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                                 CreateDisposition == FILE_OPEN_IF &&
                                 lpConsoleName == NULL ) {

                            //
                            // Compatibility hack:  We successfully created the target, but
                            // some servers (SCO VisionFS) get confused by the FILE_OPEN_IF
                            // flag and create a non-directory file instead.  Check to see if
                            // this hapenned, and if so deleted it and re-create with FILE_CREATE
                            // instead.  This is a perf hit that we have to query the file attributes,
                            // but at least it is not a net round-trip because the rdr caches the
                            // file attributes in Create&X.
                            //


                            FILE_BASIC_INFORMATION NewDestInfo;

                            Status = NtQueryInformationFile( DestFile,
                                                             &IoStatus,
                                                             &NewDestInfo,
                                                             sizeof(NewDestInfo),
                                                             FileBasicInformation );
                            if( !NT_SUCCESS(Status) ) {
                                BaseMarkFileForDelete( DestFile, 0 );
                                BaseSetLastNTError(Status);
                                leave;
                            }

                            if( !(NewDestInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {

                                // Yes, a non-directory file got created.  Delete it, then
                                // try again without FILE_OPEN_IF.

                                BaseMarkFileForDelete( DestFile,
                                                       NewDestInfo.FileAttributes );
                                NtClose( DestFile );
                                DestFile = INVALID_HANDLE_VALUE;

                                CreateDisposition = FILE_CREATE;

                                // Also, if we request FILE_WRITE_DATA access, the
                                // directory gets created but the NtCreateFile call
                                // returns status_object_name_collision.  Since this
                                // is a very VisionFS-specific workaround, we'll just
                                // turn off that bit

                                DestFileAccess &= ~FILE_WRITE_DATA;

                                continue;
                            }
                        }

                        break;  // while( TRUE )
                    }   // NtCreateFile ... if( NT_SUCCESS(Status) )

                    else {
                        BaseSetLastNTError( Status );
                    }

                    //
                    // If a file/directory already exists and we can't overwrite it,
                    // abort now.
                    //

                    if ( (*lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS) &&
                         (STATUS_OBJECT_NAME_COLLISION == Status) ) {

                        // Not allowed to overwrite an existing file.
                        SetLastError( ERROR_FILE_EXISTS );
                        DestFile = INVALID_HANDLE_VALUE;
                        leave;

                    } else if ( Status == STATUS_FILE_IS_A_DIRECTORY ) {

                        // Not allowed to overwrite a directory with a file.
                        if ( EndsInSlash ) {
                            SetLastError(ERROR_PATH_NOT_FOUND);
                        }
                        else {
                            SetLastError(ERROR_ACCESS_DENIED);
                        }
                        DestFile = INVALID_HANDLE_VALUE;
                        leave;
                    }

                    //
                    // If we're trying to create a directory, and a non-directory
                    // file already exists by that name, we need to manually delete
                    // it (FILE_OVERWRITE isn't valid for a directory file).
                    //

                    if( (*lpCopyFlags & PRIVCOPY_FILE_DIRECTORY) &&
                        Status == STATUS_NOT_A_DIRECTORY &&
                        !(*lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS) ) {

                        Status = NtCreateFile(
                                    &DestFile,
                                    DELETE|SYNCHRONIZE,
                                    &Obja,
                                    &IoStatusBlock,
                                    NULL,
                                    FILE_ATTRIBUTE_NORMAL,
                                    0,
                                    FILE_OPEN,
                                    FILE_DELETE_ON_CLOSE | FILE_SYNCHRONOUS_IO_NONALERT,
                                    NULL,
                                    0
                                    );
                        if( !NT_SUCCESS(Status) ) {
                            BaseSetLastNTError(Status);
                            DestFile = INVALID_HANDLE_VALUE;
                            leave;
                        }

                        NtClose( DestFile );
                        DestFile = INVALID_HANDLE_VALUE;

                        continue;
                    }


                    //
                    // If the create failed because of a sharing violation or because access
                    // was denied, attempt to open the file and allow other readers and
                    // writers.
                    //

                    if( GetLastError() == ERROR_SHARING_VIOLATION ||
                        GetLastError() == ERROR_ACCESS_DENIED ) {

                        DestFileSharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
                        Status = NtCreateFile(
                                    &DestFile,
                                    DestFileAccess,
                                    &Obja,
                                    &IoStatusBlock,
                                    NULL,
                                    SourceFileAttributes & FILE_ATTRIBUTE_VALID_FLAGS & SourceFileAttributesMask,
                                    DestFileSharing,
                                    CreateDisposition,
                                    CreateOptions,
                                    EaBufferToUse,
                                    EaSizeToUse
                                    );
                        //
                        // If this failed as well, then attempt to open w/o specifying
                        // delete access.  It is probably not necessary to have delete
                        // access to the file anyway, since it will not be able to clean
                        // it up because it's probably open.  However, this is not
                        // necessarily the case.
                        //

                        if ( !NT_SUCCESS(Status) &&
                             (DestFileAccess & DELETE) &&
                             (Status == STATUS_SHARING_VIOLATION ||
                              Status == STATUS_ACCESS_DENIED ) ) {

                            DestFileAccess &= ~DELETE;
                            Status = NtCreateFile(
                                        &DestFile,
                                        DestFileAccess,
                                        &Obja,
                                        &IoStatusBlock,
                                        NULL,
                                        SourceFileAttributes & FILE_ATTRIBUTE_VALID_FLAGS & SourceFileAttributesMask,
                                        DestFileSharing,
                                        CreateDisposition,
                                        CreateOptions,
                                        EaBufferToUse,
                                        EaSizeToUse
                                        );
                        }
                    }

                    if( NT_SUCCESS(Status) ) {
                        break;  // while( TRUE )
                    } else {
                        BaseSetLastNTError(Status);
                    }


                    //
                    // If the destination has not been successfully created/opened, see
                    // if it's because EAs aren't supported
                    //

                    if( EaBufferToUse != NULL
                        &&
                        GetLastError() == ERROR_EAS_NOT_SUPPORTED ) {

                        // Attempt the create again, but don't use the EAs

                        EasDropped = TRUE;
                        EaBufferToUse = NULL;
                        EaSizeToUse = 0;
                        DestFileAccess = DesiredAccess;
                        DestFileSharing = 0;
                        continue;

                    }   // if( EaBufferToUse != NULL ...

                    // If we still have an access-denied problem, try dropping
                    // the WRITE_DAC or WRITE_OWNER access

                    if(( GetLastError() == ERROR_ACCESS_DENIED  ) &&
                            (DestFileAccess & (WRITE_DAC | WRITE_OWNER)) ) {

                        // If WRITE_DAC is set, try turning it off.

                        if( DestFileAccess & WRITE_DAC ) {
                            DestFileAccess &= ~WRITE_DAC;
                        }

                        // Or, if WRITE_OWNER is set, try turning it off.  We'll
                        // turn WRITE_DAC back on if it was previously turned off.  Then,
                        // if this still doesn't work, then the next iteration will turn
                        // WRITE_DAC back off, thus covering both scenarios.

                        else if( DestFileAccess & WRITE_OWNER ) {
                            DestFileAccess &= ~WRITE_OWNER;
                            DestFileAccess |= (DesiredAccess & WRITE_DAC);
                        }

                        DestFileSharing = 0;
                        continue;
                    }


                    //
                    // If we're copying an NSS file without the reparse
                    // point and we failed, try again without
                    // FILE_COPY_STRUCTURED_STORAGE

                    if ( (CreateOptions & FILE_COPY_STRUCTURED_STORAGE) == FILE_COPY_STRUCTURED_STORAGE ) {
                        CreateOptions &= ~FILE_COPY_STRUCTURED_STORAGE;
                        DestFileAccess = DesiredAccess;
                        DestFileSharing = 0;
                        continue;
                    }


                    //
                    // We might have an access denied problem because we're copying
                    // an encrypted file to a machine that's not trusted for delegation.
                    // If possible, try without encryption (only do this if we can
                    // call the caller back and get permission to copy the data
                    // into an unencrypted file).
                    //

                    if(( GetLastError() == ERROR_ACCESS_DENIED  ) &&
                            (SourceFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) &&
                            (SourceFileAttributesMask & FILE_ATTRIBUTE_ENCRYPTED) &&
                            (*lpCopyFlags & PRIVCOPY_FILE_VALID_FLAGS) &&
                            Context != NULL &&
                            Context->lpProgressRoutine != NULL ) {

                        SourceFileAttributesMask &= ~FILE_ATTRIBUTE_ENCRYPTED;
                        CreateOptions = DesiredCreateOptions;
                        DestFileAccess = DesiredAccess;
                        DestFileSharing = 0;
                        continue;
                    }


                    //
                    // NT4 returns invalid-parameter error on an attempt to open
                    // a directory file with both FILE_WRITE_DATA and FILE_OPEN_IF.
                    // Samba 2.x returns ERROR_ALREADY_EXISTS, even though
                    // the semantics of FILE_OPEN_IF says that it should open the
                    // existing directory.
                    // For both cases, we'll try it with FILE_OPEN.
                    //

                    if( ( GetLastError() == ERROR_INVALID_PARAMETER  ||
                          GetLastError() == ERROR_ALREADY_EXISTS ) &&
                        (SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                        CreateDisposition == FILE_OPEN_IF )  {

                        CreateDisposition = FILE_OPEN;

                        SourceFileAttributesMask = ~0;
                        CreateOptions = DesiredCreateOptions;
                        DestFileAccess = DesiredAccess;
                        DestFileSharing = 0;
                        continue;
                    }

                    //
                    // Some downlevel servers don't allow a directory to be opened for write_data
                    // access.  We need write_data in order to set compression, but the
                    // downlevel server likely won't support that anyway.  (This happens on
                    // NTFS4 if the target directory file doesn't already exist.  In this
                    // case the compression will get copied over anyway as part of the create.)
                    //

                    if( (SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                        (DestFileAccess & FILE_WRITE_DATA) ) {

                        DestFileAccess = DesiredAccess & ~FILE_WRITE_DATA;

                        CreateDisposition = DesiredCreateDisposition;
                        CreateOptions = DesiredCreateOptions;
                        DestFileSharing = 0;
                        continue;
                    }

                    // If we reach this point, we've run out of options and must give up.
                    DestFile = INVALID_HANDLE_VALUE;
                    leave;

                }   // while( DestFile == INVALID_HANDLE_VALUE )
                // If we reach this point, we've successfully opened the dest file.

                //
                // If we lost the EAs, check to see if that's OK before carrying on.
                //

                if( EasDropped && (*lpCopyFlags & PRIVCOPY_FILE_METADATA) ) {

                    // Check to see if it's OK that we skip the EAs.

                    if( !BasepCopyFileCallback( TRUE,    // Continue by default
                                                ERROR_EAS_NOT_SUPPORTED,
                                                Context,
                                                NULL,
                                                PRIVCALLBACK_EAS_NOT_SUPPORTED,
                                                hSourceFile,
                                                INVALID_HANDLE_VALUE,
                                                &Canceled
                                                ) ) {
                        // Not OK.  The last error has already been set.
                        if( Canceled ) {
                            BaseMarkFileForDelete(
                                DestFile,
                                0
                                );
                        }
                        NtClose( DestFile );
                        DestFile = INVALID_HANDLE_VALUE;
                        leave;
                    }
                }

                //
                // When appropriate, copy the reparse point.
                //

                if ( OpenFileAsReparsePoint &&
                     (DestFile != INVALID_HANDLE_VALUE)) {
                    DWORD CopyResult = FALSE;

                    CopyResult = CopyReparsePoint(
                                     hSourceFile,
                                     DestFile
                                     );

                    if ( !CopyResult ) {
                        //
                        // Note that when OpenFileAsReparsePoint is TRUE, by
                        // exiting at this point the effect is that the caller
                        // will re-start the copy without inhibiting the reparse
                        // behavior.
                        //

                        //If we fail here, we may be leaving a newly created
                        // file around at the destination.  If
                        // COPY_FILE_FAIL_IF_EXISTS has been specified,
                        // further retries will fail.  Therefore we need to
                        // try to delete the new file here.
                        if (*lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS)
                        {
                            FILE_DISPOSITION_INFORMATION Disposition = {TRUE};

                            Status = NtSetInformationFile(
                                DestFile,
                                &IoStatus,
                                &Disposition,
                                sizeof(Disposition),
                                FileDispositionInformation
                                );
                            //Ignore an error if there is one.

                        }
                        *lpDestFile = DestFile;
                        leave;
                    }
                }   // if ( OpenFileAsReparsePoint &&(DestFile != INVALID_HANDLE_VALUE))


                //
                // Get the File & FileSys attributes for the target volume, plus
                // the FileSys attributes for the source volume.
                //

                *DestFileFsAttributes = 0;
                SourceFileFsAttributes = 0;
                DestFileAttributes = 0;

                if( *lpCopyFlags & PRIVCOPY_FILE_VALID_FLAGS ) {

                    Status = NtQueryVolumeInformationFile( DestFile,
                                                           &IoStatus,
                                                           &FileFsAttrInfoBuffer.Info,
                                                           sizeof(FileFsAttrInfoBuffer),
                                                           FileFsAttributeInformation );

                    if( NT_SUCCESS(Status) ) {
                        *DestFileFsAttributes = FileFsAttrInfoBuffer.Info.FileSystemAttributes;
                    } else {
                        BaseMarkFileForDelete( DestFile, 0 );
                        BaseSetLastNTError(Status);
                        leave;
                    }

                    if( lpConsoleName == NULL ) {
                        Status = NtQueryInformationFile( DestFile,
                                                         &IoStatus,
                                                         &DestBasicInformation,
                                                         sizeof(DestBasicInformation),
                                                         FileBasicInformation );
                        if( NT_SUCCESS(Status) ) {
                            DestFileAttributes = DestBasicInformation.FileAttributes;
                        } else {
                            BaseMarkFileForDelete( DestFile, 0 );
                            BaseSetLastNTError(Status);
                            leave;
                        }
                    }

                    Status = NtQueryVolumeInformationFile( hSourceFile,
                                                           &IoStatus,
                                                           &FileFsAttrInfoBuffer.Info,
                                                           sizeof(FileFsAttrInfoBuffer),
                                                           FileFsAttributeInformation );
                    if( NT_SUCCESS(Status) ) {
                        SourceFileFsAttributes = FileFsAttrInfoBuffer.Info.FileSystemAttributes;
                    } else {
                        BaseMarkFileForDelete( DestFile, 0 );
                        BaseSetLastNTError(Status);
                        leave;
                    }
                }   // if( *lpCopyFlags & PRIVCOPY_FILE_VALID_FLAGS )

                //
                // If requested and applicable, copy one or more of the the DACL, SACL, owner, and group.
                // If the source doesn't support persistent ACLs, assume that that means that
                // it doesn't support any of DACL, SACL, and owner/group.
                //

                if( (SourceFileFsAttributes & FILE_PERSISTENT_ACLS)
                    &&
                    (*lpCopyFlags & (PRIVCOPY_FILE_METADATA | PRIVCOPY_FILE_SACL | PRIVCOPY_FILE_OWNER_GROUP)) ) {

                    SECURITY_INFORMATION SecurityInformation = 0;

                    if( *lpCopyFlags & PRIVCOPY_FILE_METADATA )
                        SecurityInformation |= DACL_SECURITY_INFORMATION;

                    if( *lpCopyFlags & PRIVCOPY_FILE_SACL )
                        SecurityInformation |= SACL_SECURITY_INFORMATION;

                    if( *lpCopyFlags & PRIVCOPY_FILE_OWNER_GROUP )
                        SecurityInformation |= OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION;

                    if( !BasepCopySecurityInformation( lpExistingFileName,
                                                       hSourceFile,
                                                       SourceFileAccess,
                                                       lpNewFileName,
                                                       DestFile,
                                                       DestFileAccess,
                                                       SecurityInformation,
                                                       Context,
                                                       *DestFileFsAttributes,
                                                       &Canceled )) {

                        if( Canceled ) {
                            BaseMarkFileForDelete(
                                DestFile,
                                0
                                );
                        }
                        leave;
                    }
                }

                //
                // Copy compression and encryption
                //

                if( (*lpCopyFlags & PRIVCOPY_FILE_METADATA) ) {

                    BOOL DoCompression = FALSE;
                    int i = 0;

                    // Compression and encryption must be handled in the proper
                    // order, since a file can't be both at once.  For example,
                    // if copying (with supersede) a compressed/unencrypted file over an
                    // uncompressed/encrypted file, we must decrypt the dest
                    // before attempting to compress it.

                    if( DestFileAttributes & FILE_ATTRIBUTE_COMPRESSED ) {
                        // Handle compression first
                        DoCompression = TRUE;
                    }

                    for( i = 0; i < 2; i++ ) {

                        if( DoCompression ) {

                            DoCompression = FALSE;
                            b = BasepCopyCompression( hSourceFile,
                                                      DestFile,
                                                      SourceFileAttributes,
                                                      DestFileAttributes,
                                                      *DestFileFsAttributes,
                                                      *lpCopyFlags,
                                                      &Context );

                        } else {

                            DoCompression = TRUE;
                            b = BasepCopyEncryption( hSourceFile,
                                                     lpNewFileName,
                                                     &DestFile,
                                                     &Obja,
                                                     DestFileAccess,
                                                     DestFileSharing,
                                                     CreateDisposition,
                                                     CreateOptions,
                                                     SourceFileAttributes,
                                                     SourceFileAttributesMask,
                                                     &DestFileAttributes,
                                                     *DestFileFsAttributes,
                                                     *lpCopyFlags,
                                                     &Context );
                        }

                        if( !b ) {
                            // The dest file is already marked for delete and
                            // last error has been set.
                            leave;
                        }
                    }   // for( i = 0; i < 2; i++ )

                }   // if( (*lpCopyFlags & PRIVCOPY_FILE_METADATA) )


                //
                // If copying a directory file, handle the supersede case and
                // attributes.
                //

                if( SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

                    //
                    // In the supersede case, the target's named streams
                    // should be removed.
                    //

                    if( *lpCopyFlags & PRIVCOPY_FILE_SUPERSEDE ) {

                        ULONG StreamInfoSize;
                        PFILE_STREAM_INFORMATION StreamInfo;
                        PFILE_STREAM_INFORMATION StreamInfoBase = NULL;

                        // Get the dest file's streams

                        StreamInfoSize = 4096;
                        do {
                            StreamInfoBase = RtlAllocateHeap( RtlProcessHeap(),
                                                              MAKE_TAG( TMP_TAG ),
                                                              StreamInfoSize );

                            if ( !StreamInfoBase ) {
                                BaseSetLastNTError( STATUS_NO_MEMORY );
                                leave;
                            }

                            Status = NtQueryInformationFile(
                                        DestFile,
                                        &IoStatus,
                                        (PVOID) StreamInfoBase,
                                        StreamInfoSize,
                                        FileStreamInformation
                                        );

                            if ( !NT_SUCCESS(Status) ) {
                                //
                                //  We failed the call.  Free up the previous buffer and set up
                                //  for another pass with a buffer twice as large
                                //

                                RtlFreeHeap(RtlProcessHeap(), 0, StreamInfoBase);
                                StreamInfoBase = NULL;
                                StreamInfoSize *= 2;
                            }
                            else if( IoStatus.Information == 0 ) {
                                // There are no streams
                                RtlFreeHeap(RtlProcessHeap(), 0, StreamInfoBase);
                                StreamInfoBase = NULL;
                            }

                        } while ( Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL );

                        // If there were any streams, delete them.

                        if( StreamInfoBase != NULL ) {
                            StreamInfo = StreamInfoBase;
                            while (TRUE) {

                                OBJECT_ATTRIBUTES Obja;
                                UNICODE_STRING StreamName;
                                HANDLE DestStream;

                                StreamName.Length = (USHORT) StreamInfo->StreamNameLength;
                                StreamName.MaximumLength = (USHORT) StreamName.Length;
                                StreamName.Buffer = StreamInfo->StreamName;

                                InitializeObjectAttributes(
                                    &Obja,
                                    &StreamName,
                                    OBJ_CASE_INSENSITIVE,
                                    DestFile,
                                    NULL
                                    );

                                // Relative-open the stream to be deleted.

                                Status = NtCreateFile(
                                            &DestStream,
                                            DELETE|SYNCHRONIZE,
                                            &Obja,
                                            &IoStatusBlock,
                                            NULL,
                                            0,
                                            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                            FILE_OPEN,
                                            FILE_DELETE_ON_CLOSE | FILE_SYNCHRONOUS_IO_NONALERT,
                                            NULL,
                                            0
                                            );
                                if( !NT_SUCCESS(Status) ) {
                                    RtlFreeHeap(RtlProcessHeap(), 0, StreamInfoBase);
                                    BaseMarkFileForDelete( DestFile, 0 );
                                    BaseSetLastNTError( Status );
                                    leave;
                                }

                                // Delete the stream
                                NtClose( DestStream );

                                if (StreamInfo->NextEntryOffset == 0) {
                                    break;
                                }
                                StreamInfo = (PFILE_STREAM_INFORMATION)((PCHAR) StreamInfo + StreamInfo->NextEntryOffset);
                            }   // while (TRUE)

                            RtlFreeHeap(RtlProcessHeap(), 0, StreamInfoBase);
                        }   // if( StreamInfoBase != NULL )
                    }   // if( *lpCopyFlags & PRIVCOPY_FILE_SUPERSEDE )

                    //
                    // Also for a directory file, see if any attributes need to be
                    // added.  For non-directory files, this is handled in the NtCreateFile since
                    // either FILE_CREATE or FILE_OVERWRITE_IF is specified.  We do this last
                    // because if we set a read-only bit we won't be able to do much else afterwards.
                    //


                    if( SourceFileAttributes != DestFileAttributes ) {

                        DestFileAttributes |= SourceFileAttributes;

                        RtlZeroMemory( &DestBasicInformation, sizeof(DestBasicInformation) );
                        DestBasicInformation.FileAttributes = DestFileAttributes;
                        Status = NtSetInformationFile( DestFile,
                                                       &IoStatus,
                                                       &DestBasicInformation,
                                                       sizeof(DestBasicInformation),
                                                       FileBasicInformation );
                        if( !NT_SUCCESS(Status) ) {
                            BaseMarkFileForDelete( DestFile, 0 );
                            BaseSetLastNTError(Status);
                            leave;
                        }

                        DestFileAttributes = 0;
                        Status = NtQueryInformationFile( DestFile,
                                                         &IoStatus,
                                                         &DestBasicInformation,
                                                         sizeof(DestBasicInformation),
                                                         FileBasicInformation );
                        if( NT_SUCCESS(Status) ) {
                            DestFileAttributes = DestBasicInformation.FileAttributes;
                        } else {
                            BaseMarkFileForDelete( DestFile, 0 );
                            BaseSetLastNTError(Status);
                            leave;
                        }
                    }



                }   // if( SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY )


            }   // if( DestFile != INVALID_HANDLE_VALUE )

            //
            // If this is a directory file, there is nothing left to copy
            //

            if( SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                BOOL Canceled = FALSE;

                if( !BasepCopyFileCallback( TRUE,   // ContinueByDefault
                                            RtlNtStatusToDosError(STATUS_REQUEST_ABORTED),
                                            Context,
                                            NULL,
                                            CALLBACK_STREAM_SWITCH,
                                            hSourceFile,
                                            DestFile,
                                            &Canceled ) ) {
                    ReturnValue = FALSE;
                    if( Canceled ) {
                        BaseMarkFileForDelete(
                            DestFile,
                            0
                            );
                    }
                } else {
                    ReturnValue = TRUE;
                }
                leave;

            }


        } else {    // if ( !ARGUMENT_PRESENT(hTargetFile) )

            // We're copying a named stream.

            OBJECT_ATTRIBUTES ObjectAttributes;
            UNICODE_STRING StreamName;
            IO_STATUS_BLOCK IoStatus;
            ULONG Disposition;

            //
            // Create the output stream relative to the file specified by the
            // hTargetFile file handle.
            //

            RtlInitUnicodeString(&StreamName, lpNewFileName);
            InitializeObjectAttributes(
                &ObjectAttributes,
                &StreamName,
                0,
                hTargetFile,
                (PSECURITY_DESCRIPTOR)NULL
                );

            //
            // Determine the disposition type.
            //

            if ( *lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS ) {
                Disposition = FILE_CREATE;
            } else {
                Disposition = FILE_OVERWRITE_IF;
            }

            if ( Restartable ) {
                if ( lpRestartState->LastKnownGoodOffset.QuadPart ) {
                    Disposition = FILE_OPEN;
                } else {
                    Disposition = FILE_OVERWRITE_IF;
                }
            }

            //
            // Inhibit reparse behavior when appropriate.
            //

            FlagsAndAttributes = FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY;
            DesiredAccess = GENERIC_WRITE | SYNCHRONIZE;
            if ( OpenFileAsReparsePoint ) {
                //
                // The target has to be opened as reparse point. If
                // this fails the source is to be closed and re-opened
                // without inhibiting the reparse point behavior.
                //

                FlagsAndAttributes |= FILE_OPEN_REPARSE_POINT;
                DesiredAccess |= GENERIC_READ;
                if ( !(*lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS) ||
                     !(Restartable && (lpRestartState->LastKnownGoodOffset.QuadPart)) ) {
                    Disposition = FILE_OPEN_IF;
                }
            }

            Status = NtCreateFile(
                        &DestFile,
                        DesiredAccess,
                        &ObjectAttributes,
                        &IoStatus,
                        lpFileSize,
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        Disposition,
                        FlagsAndAttributes,
                        (PVOID)NULL,
                        0);

            if ( !NT_SUCCESS(Status) ) {

                // If we failed the create with an invalid name error, it might be becuase
                // we tried to copy an NTFS5 property set to pre-NTFS5 (and pre-NTFS4/SP4)
                // To detect this, we first check the error, and that the prefix character
                // of the stream name is a reserved ole character.

                if( Status == STATUS_OBJECT_NAME_INVALID
                    &&
                    StreamName.Buffer[1] <= 0x1f
                    &&
                    StreamName.Buffer[1] >= 1 ) {

                    // Now we check to see if we're copying to pre-NTFS5.
                    // If so, we'll assume that the leading ole character is
                    // the cause of the problem, and will silently fail the
                    // copy of this stream just as NT4 did.

                    NTSTATUS StatusT = STATUS_SUCCESS;
                    IO_STATUS_BLOCK Iosb;
                    FILE_FS_ATTRIBUTE_INFORMATION FsAttrInfo;

                    StatusT = NtQueryVolumeInformationFile( hTargetFile, &Iosb,
                                                            &FsAttrInfo,
                                                            sizeof(FsAttrInfo),
                                                            FileFsAttributeInformation );


                    // We should always get a buffer-overflow error here, because we don't
                    // provide enough buffer for the file system name, but that's OK because
                    // we don't need it (status_buffer_overflow is just a warning, so the rest
                    // of the data is good).

                    if( !NT_SUCCESS(StatusT) && STATUS_BUFFER_OVERFLOW != StatusT) {
                        Status = StatusT;
                        BaseSetLastNTError(Status);
                        leave;
                    }

                    // If this is pre-NTFS5, then silently ignore the error.
                    if( !(FILE_SUPPORTS_OBJECT_IDS & FsAttrInfo.FileSystemAttributes) ) {

                        Status = STATUS_SUCCESS;
                        ReturnValue = TRUE;
                        leave;
                    }
                }


                if ( Status != STATUS_ACCESS_DENIED ) {
                    BaseSetLastNTError(Status);
                    leave;
                }

                //
                // Determine whether or not this failed because the file
                // is a readonly file.  If so, change it to read/write,
                // re-attempt the open, and set it back to readonly again.
                //

                Status = NtQueryInformationFile(
                            hTargetFile,
                            &IoStatus,
                            (PVOID) &FileBasicInformationData,
                            sizeof(FileBasicInformationData),
                            FileBasicInformation
                            );

                if ( !NT_SUCCESS(Status) ) {
                    leave;
                }

                if ( FileBasicInformationData.FileAttributes & FILE_ATTRIBUTE_READONLY ) {
                    ULONG attributes = FileBasicInformationData.FileAttributes;

                    RtlZeroMemory( &FileBasicInformationData,
                                   sizeof(FileBasicInformationData)
                                );
                    FileBasicInformationData.FileAttributes = FILE_ATTRIBUTE_NORMAL;
                    (VOID) NtSetInformationFile(
                              hTargetFile,
                              &IoStatus,
                              &FileBasicInformationData,
                              sizeof(FileBasicInformationData),
                              FileBasicInformation
                              );
                    Status = NtCreateFile(
                                &DestFile,
                                DesiredAccess,
                                &ObjectAttributes,
                                &IoStatus,
                                lpFileSize,
                                0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                Disposition,
                                FlagsAndAttributes,
                                (PVOID)NULL,
                                0);
                    FileBasicInformationData.FileAttributes = attributes;
                    (VOID) NtSetInformationFile(
                                hTargetFile,
                                &IoStatus,
                                &FileBasicInformationData,
                                sizeof(FileBasicInformationData),
                                FileBasicInformation
                                );
                    if ( !NT_SUCCESS(Status) ) {
                        leave;
                    }
                } else {
                    leave;
                }
            }

            //
            // Adjust the file length in the case of a destination open with the
            // reparse behavior inhibited. This is needed because of the incompatibility
            // between FILE_OPEN_REPARSE_POINT and FILE_OVERWRITE_IF.
            //

            if ( OpenFileAsReparsePoint ) {
               if ( !(*lpCopyFlags & COPY_FILE_FAIL_IF_EXISTS) ||
                    !(Restartable && (lpRestartState->LastKnownGoodOffset.QuadPart)) ) {
                   SetFilePointer(DestFile,0,NULL,FILE_BEGIN);
               }
            }

        }   // if ( !ARGUMENT_PRESENT(hTargetFile) ) ... else

        //
        // Adjust the notion of restartability and chunk size based on whether
        // or not one of the files is remote.
        //

        if ( Restartable || lpFileSize->QuadPart >= BASE_COPY_FILE_CHUNK ) {
            if ( BasepRemoteFile(hSourceFile,DestFile) ) {
                *lpCopySize = BASE_COPY_FILE_CHUNK - 4096;
            } else if ( Restartable ) {
                *lpCopyFlags &= ~COPY_FILE_RESTARTABLE;
                Restartable = FALSE;
            }
        }

        //
        // Handle sparse streams
        //

        if (fSparseSource) {

            //Set the destination stream sparse
            Status = NtFsControlFile(DestFile,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatus,
                                     FSCTL_SET_SPARSE,
                                     NULL,
                                     0,
                                     NULL,
                                     0);

            // If we couldn't make the target sparse, see if it's OK to carry on.

            if( !NT_SUCCESS(Status) &&
                (*lpCopyFlags & PRIVCOPY_FILE_VALID_FLAGS) ) {

                if( !BasepCopyFileCallback( TRUE,    // Continue (ignore the error) by default
                                            RtlNtStatusToDosError(Status),
                                            Context,
                                            NULL,
                                            (STATUS_INVALID_DEVICE_REQUEST != Status)
                                               ? PRIVCALLBACK_SPARSE_FAILED
                                               : PRIVCALLBACK_SPARSE_NOT_SUPPORTED,
                                            hSourceFile,
                                            DestFile,
                                            &Canceled )) {

                    if( Canceled ) {
                        BaseMarkFileForDelete(
                            DestFile,
                            0 );
                    }
                    CloseHandle(DestFile);
                    DestFile = INVALID_HANDLE_VALUE;
                    leave;
                }
            }
        }   // if (fSparseSource)

        //
        // Preallocate the size of this file/stream so that extends do not
        // occur.
        //

        if ( !(Restartable && lpRestartState->LastKnownGoodOffset.QuadPart) &&
            lpFileSize->QuadPart) {

            EndOfFileInformation.EndOfFile = *lpFileSize;
            Status = NtSetInformationFile(
                        DestFile,
                        &IoStatus,
                        &EndOfFileInformation,
                        sizeof(EndOfFileInformation),
                        FileEndOfFileInformation
                        );
            if ( Status == STATUS_DISK_FULL ) {
                BaseSetLastNTError(Status);
                BaseMarkFileForDelete(
                    DestFile,
                    FileBasicInformationData.FileAttributes
                    );
                CloseHandle(DestFile);
                DestFile = INVALID_HANDLE_VALUE;
                leave;
            }
        }

        //
        // If the caller has a progress routine, invoke it and indicate that the
        // output file or alternate data stream has been created.  Note that a
        // stream number of 1 means that the file itself has been created.
        //

        BytesWritten.QuadPart = 0;
        if ( Context ) {
            if ( Context->lpProgressRoutine ) {
                Context->dwStreamNumber += 1;
                ReturnCode = Context->lpProgressRoutine(
                                Context->TotalFileSize,
                                Context->TotalBytesTransferred,
                                *lpFileSize,
                                BytesWritten,
                                Context->dwStreamNumber,
                                CALLBACK_STREAM_SWITCH,
                                hSourceFile,
                                DestFile,
                                Context->lpData
                                );
            } else {
                ReturnCode = PROGRESS_CONTINUE;
            }

            if ( ReturnCode == PROGRESS_CANCEL ||
                (Context->lpCancel && *Context->lpCancel) ) {
                BaseMarkFileForDelete(
                    hTargetFile ? hTargetFile : DestFile,
                    FileBasicInformationData.FileAttributes
                    );
                BaseSetLastNTError(STATUS_REQUEST_ABORTED);
                leave;
            }

            if ( ReturnCode == PROGRESS_STOP ) {
                BaseSetLastNTError(STATUS_REQUEST_ABORTED);
                leave;
            }

            if ( ReturnCode == PROGRESS_QUIET ) {
                Context = NULL;
                *lpCopyFileContext = NULL;
            }
        }


        if (!Restartable && !fSparseSource) {

            while (!lpFileSize->HighPart && (lpFileSize->LowPart < TWO56K)) {

                // If there's nothing to copy, then we're done (this happens when
                // copying directory files, as there's no unnamed data stream).

                if( lpFileSize->LowPart == 0 ) {
                    ReturnValue = TRUE;
                    leave;
                }

                //
                // Create a section and map the source file.  If anything fails,
                // then drop into an I/O system copy mode.
                //

                Status = NtCreateSection(
                    &Section,
                        SECTION_ALL_ACCESS,
                        NULL,
                        NULL,
                        PAGE_READONLY,
                        SEC_COMMIT,
                        hSourceFile
                    );
                if ( !NT_SUCCESS(Status) ) {
                    break;
                }

                SectionOffset.LowPart = 0;
                SectionOffset.HighPart = 0;
                ViewSize = 0;
                BigViewSize = 0;

                Status = NtMapViewOfSection(
                    Section,
                    NtCurrentProcess(),
                    &SourceBase,
                    0L,
                    0L,
                    &SectionOffset,
                    &BigViewSize,
                    ViewShare,
                    0L,
                    PAGE_READONLY
                    );
                NtClose(Section);
                Section = NULL;
                if ( !NT_SUCCESS(Status) ) {
                    break;
                }

                //
                // note that this is OK since ViewSize will never be > 256k in this path
                //

                ViewSize = (ULONG)BigViewSize;

                //
                // Everything is mapped, so copy the stream
                //

                SourceBuffer = SourceBase;
                BytesToWrite = lpFileSize->LowPart;

                //
                //  Since we are playing with user memory here, the user
                //  may decommit or unmap it on us.  We wrap the access
                //  in try/except to clean up if anything goes wrong
                //
                //  We set ReturnCode inside the try/except so that we
                //  can detect failure and leave from the enclosing try/finally.
                //

                ReturnCode = TRUE;

                try {

                    while (BytesToWrite) {
                        if (BytesToWrite > *lpCopySize) {
                            ViewSize = *lpCopySize;
                        } else {
                            ViewSize = BytesToWrite;
                        }

                        if ( !WriteFile(DestFile,SourceBuffer,ViewSize, &ViewSize, NULL) ) {
                            if ( !ARGUMENT_PRESENT(hTargetFile) &&
                                GetLastError() != ERROR_NO_MEDIA_IN_DRIVE ) {

                                BaseMarkFileForDelete(
                                    DestFile,
                                    FileBasicInformationData.FileAttributes
                                    );
                            }
                            ReturnCode = PROGRESS_STOP;
                            leave;
                        }

                        BytesToWrite -= ViewSize;
                        SourceBuffer += ViewSize;

                        //
                        // If the caller has a progress routine, invoke it for this
                        // chunk's completion.
                        //

                        if ( Context ) {
                            if ( Context->lpProgressRoutine ) {
                                BytesWritten.QuadPart += ViewSize;
                                Context->TotalBytesTransferred.QuadPart += ViewSize;
                                ReturnCode = Context->lpProgressRoutine(
                                    Context->TotalFileSize,
                                    Context->TotalBytesTransferred,
                                    *lpFileSize,
                                    BytesWritten,
                                    Context->dwStreamNumber,
                                    CALLBACK_CHUNK_FINISHED,
                                    hSourceFile,
                                    DestFile,
                                    Context->lpData
                                    );
                            } else {
                                ReturnCode = PROGRESS_CONTINUE;
                            }

                            if ( ReturnCode == PROGRESS_CANCEL ||
                                 (Context->lpCancel && *Context->lpCancel) ) {
                                if ( !ARGUMENT_PRESENT(hTargetFile) ) {
                                    BaseMarkFileForDelete(
                                        hTargetFile ? hTargetFile : DestFile,
                                        FileBasicInformationData.FileAttributes
                                        );
                                    BaseSetLastNTError(STATUS_REQUEST_ABORTED);
                                }
                                ReturnCode = PROGRESS_STOP;
                                leave;
                            }

                            if ( ReturnCode == PROGRESS_STOP ) {
                                BaseSetLastNTError(STATUS_REQUEST_ABORTED);
                                ReturnCode = PROGRESS_STOP;
                                leave;
                            }

                            if ( ReturnCode == PROGRESS_QUIET ) {
                                Context = NULL;
                                *lpCopyFileContext = NULL;
                            }
                        }
                    }   // while (BytesToWrite)

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    if ( !ARGUMENT_PRESENT(hTargetFile) ) {
                        BaseMarkFileForDelete(
                            DestFile,
                            FileBasicInformationData.FileAttributes
                            );
                    }
                    BaseSetLastNTError(GetExceptionCode());
                    ReturnCode = PROGRESS_STOP;
                }

                if (ReturnCode != PROGRESS_STOP) {
                    ReturnValue = TRUE;
                }

                leave;

            }   // while (!lpFileSize->HighPart && (lpFileSize->LowPart < TWO56K)
        }   // if (!Restartable && !fSparseSource)

        if ( Restartable ) {

            //
            // A restartable operation is being performed.  Reset the state
            // of the copy to the last known good offset that was written
            // to the output file to continue the operation.
            //

            SetFilePointer(
                hSourceFile,
                lpRestartState->LastKnownGoodOffset.LowPart,
                &lpRestartState->LastKnownGoodOffset.HighPart,
                FILE_BEGIN
                );
            SetFilePointer(
                DestFile,
                lpRestartState->LastKnownGoodOffset.LowPart,
                &lpRestartState->LastKnownGoodOffset.HighPart,
                FILE_BEGIN
                );
            BytesWritten.QuadPart = lpRestartState->LastKnownGoodOffset.QuadPart;
        }

        IoDestBase = RtlAllocateHeap(
                        RtlProcessHeap(),
                        MAKE_TAG( TMP_TAG ),
                        *lpCopySize
                        );
        if ( !IoDestBase ) {
            if ( !ARGUMENT_PRESENT(hTargetFile) && !Restartable ) {
                BaseMarkFileForDelete(
                    DestFile,
                    FileBasicInformationData.FileAttributes
                    );
            }
            BaseSetLastNTError(STATUS_NO_MEMORY);
            leave;
        }



        do {

            BlockSize = *lpCopySize;
            fSkipBlock = FALSE;

            if (fSparseSource)
            {
                FILE_ALLOCATED_RANGE_BUFFER arbIn, arbOut;
                arbIn.FileOffset = BytesWritten;
                arbIn.Length.QuadPart = BlockSize;
                arbOut.FileOffset.QuadPart = arbOut.Length.QuadPart = 0;
                Status = NtFsControlFile(hSourceFile,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &IoStatus,
                                         FSCTL_QUERY_ALLOCATED_RANGES,
                                         &arbIn,
                                         sizeof(arbIn),
                                         &arbOut,
                                         sizeof(arbOut));
                if (NT_SUCCESS(Status) || (Status == STATUS_BUFFER_OVERFLOW))
                {
                    LARGE_INTEGER liBlockStart;
                    LARGE_INTEGER liBlockEnd;
                    LARGE_INTEGER liAllocatedStart;
                    LARGE_INTEGER liAllocatedEnd;

                    liBlockStart = BytesWritten;
                    liBlockEnd.QuadPart = BytesWritten.QuadPart + BlockSize - 1;
                    liAllocatedStart = arbOut.FileOffset;
                    liAllocatedEnd.QuadPart = arbOut.FileOffset.QuadPart +
                        arbOut.Length.QuadPart - 1;

                    //Determine if the current block is in an allocated
                    //range.  If it is not, then skip the block.
#if DBG == 1
                    if ((liBlockStart.QuadPart > liAllocatedStart.QuadPart) &&
                        (arbOut.Length.QuadPart != 0))
                    {
                        //This should never happen
                        OutputDebugString("\nBad start from FSCTL_QUERY_ALLOCATED_RANGES\n");
                    }
#endif
                    if (arbOut.Length.QuadPart == 0) {
                        //No more allocated blocks until the end of the file.
                        //Set fSkipBlock to TRUE here, and the ReadFile call
                        //will eventually return zero bytes, indicating we've
                        //hit the end of the file.
                        fSkipBlock = TRUE;
                    } else if ((liBlockStart.QuadPart <
                                 liAllocatedStart.QuadPart) &&
                                (liBlockEnd.QuadPart <
                                 liAllocatedStart.QuadPart)) {
                        //The entire block is zeroed, so skip it
                        fSkipBlock = TRUE;
                    }
                    else if (liBlockStart.QuadPart <
                             liAllocatedStart.QuadPart) {
                        //Only the tail end of the block is valid.
                        //Skip forward to that and copy to the end of
                        //the block.
                        if ((SetFilePointer(hSourceFile,
                                            liAllocatedStart.LowPart,
                                            &(liAllocatedStart.HighPart),
                                           FILE_BEGIN) != 0xFFFFFFFF) &&
                            (SetFilePointer(DestFile,
                                           liAllocatedStart.LowPart,
                                           &(liAllocatedStart.HighPart),
                                           FILE_BEGIN) != 0xFFFFFFFF)) {
                            //BUGBUG:  What if one of those SetFilePointers
                            //fails?
                            BytesWritten = liAllocatedStart;
                            BlockSize = (DWORD)((liBlockEnd.QuadPart -
                                                 liAllocatedStart.QuadPart));
                        }

                    }
                    else {
                        //The whole block is allocated, so do nothing
                    }
                }
            }

            if (!fSkipBlock)
            {
                b = ReadFile(hSourceFile,IoDestBase,BlockSize, &ViewSize, NULL);
            }
            else
            {
                LARGE_INTEGER BytesRead;
                BytesRead = BytesWritten;

                if (BytesRead.QuadPart > lpFileSize->QuadPart) {
                    BlockSize = 0;
                } else if (BytesRead.QuadPart + BlockSize >= lpFileSize->QuadPart) {
                    BlockSize = (ULONG)(lpFileSize->QuadPart - BytesRead.QuadPart);
                }

                BytesRead.QuadPart += BlockSize;
                if ( SetFilePointer(hSourceFile,
                                    BytesRead.LowPart,
                                    &BytesRead.HighPart,
                                    FILE_BEGIN) != 0xffffffff ) {
                }
                else
                {
                    if (GetLastError() != NO_ERROR)
                        b = FALSE;
                }
                ViewSize = BlockSize;
            }

            if (!b || !ViewSize)
                break;

            if (!fSkipBlock) {
                if ( !WriteFile(DestFile,IoDestBase,ViewSize, &ViewSize, NULL) ) {
                    if ( !ARGUMENT_PRESENT(hTargetFile) &&
                        GetLastError() != ERROR_NO_MEDIA_IN_DRIVE &&
                        !Restartable ) {

                        BaseMarkFileForDelete(
                            DestFile,
                            FileBasicInformationData.FileAttributes
                            );
                    }

                    leave;
                }
                BytesWritten.QuadPart += ViewSize;
            }
            else
            {
                BytesWritten.QuadPart += ViewSize;
                if (( SetFilePointer(DestFile,
                                    BytesWritten.LowPart,
                                    &BytesWritten.HighPart,
                                    FILE_BEGIN) == 0xffffffff ) &&
                    ( GetLastError() != NO_ERROR ))
                {
                    b = FALSE;
                    break;
                }
            }

            WriteCount++;

            if ( Restartable &&
                 (((WriteCount & 3) == 0 &&
                 BytesWritten.QuadPart ) ||
                 BytesWritten.QuadPart == lpFileSize->QuadPart) ) {

                LARGE_INTEGER SavedOffset;
                DWORD Bytes;
                HANDLE DestinationFile = hTargetFile ? hTargetFile : DestFile;

                //
                // Another 256kb has been written to the target file, or
                // this stream of the file has been completely copied, so
                // update the restart state in the output file accordingly.
                //

                NtFlushBuffersFile(DestinationFile,&IoStatus);
                SavedOffset.QuadPart = BytesWritten.QuadPart;
                SetFilePointer(DestinationFile,0,NULL,FILE_BEGIN);
                lpRestartState->LastKnownGoodOffset.QuadPart = BytesWritten.QuadPart;
                lpRestartState->Checksum = BasepChecksum((PUSHORT)lpRestartState,FIELD_OFFSET(RESTART_STATE,Checksum) >> 1);
                b = WriteFile(
                        DestinationFile,
                        lpRestartState,
                        sizeof(RESTART_STATE),
                        &Bytes,
                        NULL
                        );
                if ( !b || Bytes != sizeof(RESTART_STATE) ) {
                    leave;
                }
                NtFlushBuffersFile(DestinationFile,&IoStatus);
                SetFilePointer(
                    DestinationFile,
                    SavedOffset.LowPart,
                    &SavedOffset.HighPart,
                    FILE_BEGIN
                    );
            }

            //
            // If the caller has a progress routine, invoke it for this
            // chunk's completion.
            //

            if ( Context ) {
                if ( Context->lpProgressRoutine ) {
                    Context->TotalBytesTransferred.QuadPart += ViewSize;
                    ReturnCode = Context->lpProgressRoutine(
                                    Context->TotalFileSize,
                                    Context->TotalBytesTransferred,
                                    *lpFileSize,
                                    BytesWritten,
                                    Context->dwStreamNumber,
                                    CALLBACK_CHUNK_FINISHED,
                                    hSourceFile,
                                    DestFile,
                                    Context->lpData
                                    );
                } else {
                    ReturnCode = PROGRESS_CONTINUE;
                }
                if ( ReturnCode == PROGRESS_CANCEL ||
                    (Context->lpCancel && *Context->lpCancel) ) {
                    if ( !ARGUMENT_PRESENT(hTargetFile) ) {
                        BaseMarkFileForDelete(
                            hTargetFile ? hTargetFile : DestFile,
                            FileBasicInformationData.FileAttributes
                            );
                        BaseSetLastNTError(STATUS_REQUEST_ABORTED);
                        leave;
                    }
                }

                if ( ReturnCode == PROGRESS_STOP ) {
                    BaseSetLastNTError(STATUS_REQUEST_ABORTED);
                    leave;
                }

                if ( ReturnCode == PROGRESS_QUIET ) {
                    Context = NULL;
                    *lpCopyFileContext = NULL;
                }
            }
        } while (TRUE);

        if ( !b && !ARGUMENT_PRESENT(hTargetFile) ) {
            if ( !Restartable ) {
                BaseMarkFileForDelete(
                    DestFile,
                    FileBasicInformationData.FileAttributes
                    );
            }
            leave;
        }

        ReturnValue = TRUE;
    } finally {
        if ( DestFile != INVALID_HANDLE_VALUE ) {
            *lpDestFile = DestFile;
        }
        if ( Section ) {
            NtClose(Section);
        }
        if ( SourceBase ) {
            NtUnmapViewOfSection(NtCurrentProcess(),SourceBase);
        }
        if ( IoDestBase ) {
            RtlFreeHeap(RtlProcessHeap(), 0,IoDestBase);
        }
        if ( DestFileNameBuffer ) {
            RtlFreeHeap(RtlProcessHeap(), 0, DestFileNameBuffer );
        }
        if( EaBuffer ) {
            RtlFreeHeap(RtlProcessHeap(), 0, EaBuffer );
        }



    }

    return ReturnValue;
}

HANDLE
WINAPI
CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )

/*++

Routine Description:

    ANSI thunk to CreateFileW

--*/

{

    PUNICODE_STRING Unicode;

    Unicode = Basep8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return INVALID_HANDLE_VALUE;
    }

    return ( CreateFileW( Unicode->Buffer,
                          dwDesiredAccess,
                          dwShareMode,
                          lpSecurityAttributes,
                          dwCreationDisposition,
                          dwFlagsAndAttributes,
                          hTemplateFile
                        )
           );
}

HANDLE
WINAPI
CreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )

/*++

Routine Description:

    A file can be created, opened, or truncated, and a handle opened to
    access the new file using CreateFile.

    This API is used to create or open a file and obtain a handle to it
    that allows reading data, writing data, and moving the file pointer.

    This API allows the caller to specify the following creation
    dispositions:

      - Create a new file and fail if the file exists ( CREATE_NEW )

      - Create a new file and succeed if it exists ( CREATE_ALWAYS )

      - Open an existing file ( OPEN_EXISTING )

      - Open and existing file or create it if it does not exist (
        OPEN_ALWAYS )

      - Truncate and existing file ( TRUNCATE_EXISTING )

    If this call is successful, a handle is returned that has
    appropriate access to the specified file.

    If as a result of this call, a file is created,

      - The attributes of the file are determined by the value of the
        FileAttributes parameter or'd with the FILE_ATTRIBUTE_ARCHIVE bit.

      - The length of the file will be set to zero.

      - If the hTemplateFile parameter is specified, any extended
        attributes associated with the file are assigned to the new file.

    If a new file is not created, then the hTemplateFile is ignored as
    are any extended attributes.

    For DOS based systems running share.exe the file sharing semantics
    work as described above.  Without share.exe no share level
    protection exists.

    This call is logically equivalent to DOS (int 21h, function 5Bh), or
    DOS (int 21h, function 3Ch) depending on the value of the
    FailIfExists parameter.

Arguments:

    lpFileName - Supplies the file name of the file to open.  Depending on
        the value of the FailIfExists parameter, this name may or may
        not already exist.

    dwDesiredAccess - Supplies the caller's desired access to the file.

        DesiredAccess Flags:

        GENERIC_READ - Read access to the file is requested.  This
            allows data to be read from the file and the file pointer to
            be modified.

        GENERIC_WRITE - Write access to the file is requested.  This
            allows data to be written to the file and the file pointer to
            be modified.

    dwShareMode - Supplies a set of flags that indicates how this file is
        to be shared with other openers of the file.  A value of zero
        for this parameter indicates no sharing of the file, or
        exclusive access to the file is to occur.

        ShareMode Flags:

        FILE_SHARE_READ - Other open operations may be performed on the
            file for read access.

        FILE_SHARE_WRITE - Other open operations may be performed on the
            file for write access.

    lpSecurityAttributes - An optional parameter that, if present, and
        supported on the target file system supplies a security
        descriptor for the new file.

    dwCreationDisposition - Supplies a creation disposition that
        specifies how this call is to operate.  This parameter must be
        one of the following values.

        dwCreationDisposition Value:

        CREATE_NEW - Create a new file.  If the specified file already
            exists, then fail.  The attributes for the new file are what
            is specified in the dwFlagsAndAttributes parameter or'd with
            FILE_ATTRIBUTE_ARCHIVE.  If the hTemplateFile is specified,
            then any extended attributes associated with that file are
            propogated to the new file.

        CREATE_ALWAYS - Always create the file.  If the file already
            exists, then it is overwritten.  The attributes for the new
            file are what is specified in the dwFlagsAndAttributes
            parameter or'd with FILE_ATTRIBUTE_ARCHIVE.  If the
            hTemplateFile is specified, then any extended attributes
            associated with that file are propogated to the new file.

        OPEN_EXISTING - Open the file, but if it does not exist, then
            fail the call.

        OPEN_ALWAYS - Open the file if it exists.  If it does not exist,
            then create the file using the same rules as if the
            disposition were CREATE_NEW.

        TRUNCATE_EXISTING - Open the file, but if it does not exist,
            then fail the call.  Once opened, the file is truncated such
            that its size is zero bytes.  This disposition requires that
            the caller open the file with at least GENERIC_WRITE access.

    dwFlagsAndAttributes - Specifies flags and attributes for the file.
        The attributes are only used when the file is created (as
        opposed to opened or truncated).  Any combination of attribute
        flags is acceptable except that all other attribute flags
        override the normal file attribute, FILE_ATTRIBUTE_NORMAL.  The
        FILE_ATTRIBUTE_ARCHIVE flag is always implied.

        dwFlagsAndAttributes Flags:

        FILE_ATTRIBUTE_NORMAL - A normal file should be created.

        FILE_ATTRIBUTE_READONLY - A read-only file should be created.

        FILE_ATTRIBUTE_HIDDEN - A hidden file should be created.

        FILE_ATTRIBUTE_SYSTEM - A system file should be created.

        FILE_FLAG_WRITE_THROUGH - Indicates that the system should
            always write through any intermediate cache and go directly
            to the file.  The system may still cache writes, but may not
            lazily flush the writes.

        FILE_FLAG_OVERLAPPED - Indicates that the system should initialize
            the file so that ReadFile and WriteFile operations that may
            take a significant time to complete will return ERROR_IO_PENDING.
            An event will be set to the signalled state when the operation
            completes. When FILE_FLAG_OVERLAPPED is specified the system will
            not maintain the file pointer. The position to read/write from
            is passed to the system as part of the OVERLAPPED structure
            which is an optional parameter to ReadFile and WriteFile.

        FILE_FLAG_NO_BUFFERING - Indicates that the file is to be opened
            with no intermediate buffering or caching done by the
            system.  Reads and writes to the file must be done on sector
            boundries.  Buffer addresses for reads and writes must be
            aligned on at least disk sector boundries in memory.

        FILE_FLAG_RANDOM_ACCESS - Indicates that access to the file may
            be random. The system cache manager may use this to influence
            its caching strategy for this file.

        FILE_FLAG_SEQUENTIAL_SCAN - Indicates that access to the file
            may be sequential.  The system cache manager may use this to
            influence its caching strategy for this file.  The file may
            in fact be accessed randomly, but the cache manager may
            optimize its cacheing policy for sequential access.

        FILE_FLAG_DELETE_ON_CLOSE - Indicates that the file is to be
            automatically deleted when the last handle to it is closed.

        FILE_FLAG_BACKUP_SEMANTICS - Indicates that the file is being opened
            or created for the purposes of either a backup or a restore
            operation.  Thus, the system should make whatever checks are
            appropriate to ensure that the caller is able to override
            whatever security checks have been placed on the file to allow
            this to happen.

        FILE_FLAG_POSIX_SEMANTICS - Indicates that the file being opened
            should be accessed in a manner compatible with the rules used
            by POSIX.  This includes allowing multiple files with the same
            name, differing only in case.  WARNING:  Use of this flag may
            render it impossible for a DOS, WIN-16, or WIN-32 application
            to access the file.

        FILE_FLAG_OPEN_REPARSE_POINT - Indicates that the file being opened
            should be accessed as if it were a reparse point.  WARNING:  Use
            of this flag may inhibit the operation of file system filter drivers
            present in the I/O subsystem.

        FILE_FLAG_OPEN_NO_RECALL - Indicates that all the state of the file
            should be acessed without changing its storage location.  Thus,
            in the case of files that have parts of its state stored at a
            remote servicer, no permanent recall of data is to happen.

    Security Quality of Service information may also be specified in
        the dwFlagsAndAttributes parameter.  These bits are meaningful
        only if the file being opened is the client side of a Named
        Pipe.  Otherwise they are ignored.

        SECURITY_SQOS_PRESENT - Indicates that the Security Quality of
            Service bits contain valid values.

    Impersonation Levels:

        SECURITY_ANONYMOUS - Specifies that the client should be impersonated
            at Anonymous impersonation level.

        SECURITY_IDENTIFICAION - Specifies that the client should be impersonated
            at Identification impersonation level.

        SECURITY_IMPERSONATION - Specifies that the client should be impersonated
            at Impersonation impersonation level.

        SECURITY_DELEGATION - Specifies that the client should be impersonated
            at Delegation impersonation level.

    Context Tracking:

        SECURITY_CONTEXT_TRACKING - A boolean flag that when set,
            specifies that the Security Tracking Mode should be
            Dynamic, otherwise Static.

        SECURITY_EFFECTIVE_ONLY - A boolean flag indicating whether
            the entire security context of the client is to be made
            available to the server or only the effective aspects of
            the context.

    hTemplateFile - An optional parameter, then if specified, supplies a
        handle with GENERIC_READ access to a template file.  The
        template file is used to supply extended attributes for the file
        being created.  When the new file is created, the relevant attributes
        from the template file are used in creating the new file.

Return Value:

    Not -1 - Returns an open handle to the specified file.  Subsequent
        access to the file is controlled by the DesiredAccess parameter.

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
    ULONG CreateDisposition;
    ULONG CreateFlags;
    FILE_ALLOCATION_INFORMATION AllocationInfo;
    FILE_EA_INFORMATION EaInfo;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    ULONG EaSize;
    PUNICODE_STRING lpConsoleName;
    BOOL bInheritHandle;
    BOOL EndsInSlash;
    DWORD SQOSFlags;
    BOOLEAN ContextTrackingMode = FALSE;
    BOOLEAN EffectiveOnly = FALSE;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel = 0;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    switch ( dwCreationDisposition ) {
        case CREATE_NEW        :
            CreateDisposition = FILE_CREATE;
            break;
        case CREATE_ALWAYS     :
            CreateDisposition = FILE_OVERWRITE_IF;
            break;
        case OPEN_EXISTING     :
            CreateDisposition = FILE_OPEN;
            break;
        case OPEN_ALWAYS       :
            CreateDisposition = FILE_OPEN_IF;
            break;
        case TRUNCATE_EXISTING :
            CreateDisposition = FILE_OPEN;
            if ( !(dwDesiredAccess & GENERIC_WRITE) ) {
                BaseSetLastNTError(STATUS_INVALID_PARAMETER);
                return INVALID_HANDLE_VALUE;
                }
            break;
        default :
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }

    // temporary routing code

    RtlInitUnicodeString(&FileName,lpFileName);

    if ( FileName.Length > 1 && lpFileName[(FileName.Length >> 1)-1] == (WCHAR)'\\' ) {
        EndsInSlash = TRUE;
        }
    else {
        EndsInSlash = FALSE;
        }

    if ((lpConsoleName = BaseIsThisAConsoleName(&FileName,dwDesiredAccess)) ) {

        Handle = INVALID_HANDLE_VALUE;

        bInheritHandle = FALSE;
        if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
                bInheritHandle = lpSecurityAttributes->bInheritHandle;
            }

        Handle = OpenConsoleW(lpConsoleName->Buffer,
                           dwDesiredAccess,
                           bInheritHandle,
                           FILE_SHARE_READ | FILE_SHARE_WRITE //dwShareMode
                          );

        if ( Handle == INVALID_HANDLE_VALUE ) {
            BaseSetLastNTError(STATUS_ACCESS_DENIED);
            return INVALID_HANDLE_VALUE;
            }
        else {
            SetLastError(0);
             return Handle;
            }
        }
    // end temporary code

    CreateFlags = 0;


    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
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
        dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS ? 0 : OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    SQOSFlags = dwFlagsAndAttributes & SECURITY_VALID_SQOS_FLAGS;

    if ( SQOSFlags & SECURITY_SQOS_PRESENT ) {

        SQOSFlags &= ~SECURITY_SQOS_PRESENT;

        if (SQOSFlags & SECURITY_CONTEXT_TRACKING) {

            SecurityQualityOfService.ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE) TRUE;
            SQOSFlags &= ~SECURITY_CONTEXT_TRACKING;

        } else {

            SecurityQualityOfService.ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE) FALSE;
        }

        if (SQOSFlags & SECURITY_EFFECTIVE_ONLY) {

            SecurityQualityOfService.EffectiveOnly = TRUE;
            SQOSFlags &= ~SECURITY_EFFECTIVE_ONLY;

        } else {

            SecurityQualityOfService.EffectiveOnly = FALSE;
        }

        SecurityQualityOfService.ImpersonationLevel = SQOSFlags >> 16;


    } else {

        SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
        SecurityQualityOfService.EffectiveOnly = TRUE;
    }

    SecurityQualityOfService.Length = sizeof( SECURITY_QUALITY_OF_SERVICE );
    Obja.SecurityQualityOfService = &SecurityQualityOfService;

    if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
        Obja.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        if ( lpSecurityAttributes->bInheritHandle ) {
            Obja.Attributes |= OBJ_INHERIT;
            }
        }

    EaBuffer = NULL;
    EaSize = 0;

    if ( ARGUMENT_PRESENT(hTemplateFile) ) {
        Status = NtQueryInformationFile(
                    hTemplateFile,
                    &IoStatusBlock,
                    &EaInfo,
                    sizeof(EaInfo),
                    FileEaInformation
                    );
        if ( NT_SUCCESS(Status) && EaInfo.EaSize ) {
            EaSize = EaInfo.EaSize;
            do {
                EaSize *= 2;
                EaBuffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), EaSize);
                if ( !EaBuffer ) {
                    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                    BaseSetLastNTError(STATUS_NO_MEMORY);
                    return INVALID_HANDLE_VALUE;
                    }
                Status = NtQueryEaFile(
                            hTemplateFile,
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
                    RtlFreeHeap(RtlProcessHeap(), 0,EaBuffer);
                    EaBuffer = NULL;
                    IoStatusBlock.Information = 0;
                    }
                } while ( Status == STATUS_BUFFER_OVERFLOW ||
                          Status == STATUS_BUFFER_TOO_SMALL );
            EaSize = (ULONG)IoStatusBlock.Information;
            }
        }

    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING ? FILE_NO_INTERMEDIATE_BUFFERING : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH ? FILE_WRITE_THROUGH : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED ? 0 : FILE_SYNCHRONOUS_IO_NONALERT );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_SEQUENTIAL_SCAN ? FILE_SEQUENTIAL_ONLY : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_RANDOM_ACCESS ? FILE_RANDOM_ACCESS : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS ? FILE_OPEN_FOR_BACKUP_INTENT : 0 );

    if ( dwFlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE ) {
        CreateFlags |= FILE_DELETE_ON_CLOSE;
        dwDesiredAccess |= DELETE;
        }

    if ( dwFlagsAndAttributes & FILE_FLAG_OPEN_REPARSE_POINT ) {
        CreateFlags |= FILE_OPEN_REPARSE_POINT;
        }

    if ( dwFlagsAndAttributes & FILE_FLAG_OPEN_NO_RECALL ) {
        CreateFlags |= FILE_OPEN_NO_RECALL;
        }

    //
    // Backup semantics allow directories to be opened
    //

    if ( !(dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS) ) {
        CreateFlags |= FILE_NON_DIRECTORY_FILE;
        }
    else {

        //
        // Backup intent was specified... Now look to see if we are to allow
        // directory creation
        //

        if ( (dwFlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY  ) &&
             (dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS ) &&
             (CreateDisposition == FILE_CREATE) ) {
             CreateFlags |= FILE_DIRECTORY_FILE;
             }
        }

    Status = NtCreateFile(
                &Handle,
                (ACCESS_MASK)dwDesiredAccess | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                NULL,
                dwFlagsAndAttributes & (FILE_ATTRIBUTE_VALID_FLAGS & ~FILE_ATTRIBUTE_DIRECTORY),
                dwShareMode,
                CreateDisposition,
                CreateFlags,
                EaBuffer,
                EaSize
                );

    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

    if ( EaBuffer ) {
        RtlFreeHeap(RtlProcessHeap(), 0, EaBuffer);
        }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        if ( Status == STATUS_OBJECT_NAME_COLLISION ) {
            SetLastError(ERROR_FILE_EXISTS);
            }
        else if ( Status == STATUS_FILE_IS_A_DIRECTORY ) {
            if ( EndsInSlash ) {
                SetLastError(ERROR_PATH_NOT_FOUND);
                }
            else {
                SetLastError(ERROR_ACCESS_DENIED);
                }
            }
        return INVALID_HANDLE_VALUE;
        }

    //
    // if NT returns supersede/overwritten, it means that a create_always, openalways
    // found an existing copy of the file. In this case ERROR_ALREADY_EXISTS is returned
    //

    if ( (dwCreationDisposition == CREATE_ALWAYS && IoStatusBlock.Information == FILE_OVERWRITTEN) ||
         (dwCreationDisposition == OPEN_ALWAYS && IoStatusBlock.Information == FILE_OPENED) ){
        SetLastError(ERROR_ALREADY_EXISTS);
        }
    else {
        SetLastError(0);
        }

    //
    // Truncate the file if required
    //

    if ( dwCreationDisposition == TRUNCATE_EXISTING) {

        AllocationInfo.AllocationSize.QuadPart = 0;
        Status = NtSetInformationFile(
                    Handle,
                    &IoStatusBlock,
                    &AllocationInfo,
                    sizeof(AllocationInfo),
                    FileAllocationInformation
                    );
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            NtClose(Handle);
            Handle = INVALID_HANDLE_VALUE;
            }
        }

    //
    // Deal with hTemplateFile
    //

    return Handle;
}

UINT
GetErrorMode();

HFILE
WINAPI
OpenFile(
    LPCSTR lpFileName,
    LPOFSTRUCT lpReOpenBuff,
    UINT uStyle
    )
{

    BOOL b;
    FILETIME LastWriteTime;
    HANDLE hFile;
    DWORD DesiredAccess;
    DWORD ShareMode;
    DWORD CreateDisposition;
    DWORD PathLength;
    LPSTR FilePart;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    NTSTATUS Status;
    OFSTRUCT OriginalReOpenBuff;
    BOOL SearchFailed;

    SearchFailed = FALSE;
    OriginalReOpenBuff = *lpReOpenBuff;
    hFile = (HANDLE)-1;
    try {
        SetLastError(0);

        if ( uStyle & OF_PARSE ) {
            PathLength = GetFullPathName(lpFileName,(OFS_MAXPATHNAME - 1),lpReOpenBuff->szPathName,&FilePart);
            if ( PathLength > (OFS_MAXPATHNAME - 1) ) {
                SetLastError(ERROR_INVALID_DATA);
                hFile = (HANDLE)-1;
                goto finally_exit;
                }
            lpReOpenBuff->cBytes = sizeof(*lpReOpenBuff);
            lpReOpenBuff->fFixedDisk = 1;
            lpReOpenBuff->nErrCode = 0;
            lpReOpenBuff->Reserved1 = 0;
            lpReOpenBuff->Reserved2 = 0;
            hFile = (HANDLE)0;
            goto finally_exit;
            }
        //
        // Compute Desired Access
        //

        if ( uStyle & OF_WRITE ) {
            DesiredAccess = GENERIC_WRITE;
            }
        else {
            DesiredAccess = GENERIC_READ;
            }
        if ( uStyle & OF_READWRITE ) {
            DesiredAccess |= (GENERIC_READ | GENERIC_WRITE);
            }

        //
        // Compute ShareMode
        //

        ShareMode = BasepOfShareToWin32Share(uStyle);

        //
        // Compute Create Disposition
        //

        CreateDisposition = OPEN_EXISTING;
        if ( uStyle & OF_CREATE ) {
            CreateDisposition = CREATE_ALWAYS;
            DesiredAccess = (GENERIC_READ | GENERIC_WRITE);
            }

        //
        // if this is anything other than a re-open, fill the re-open buffer
        // with the full pathname for the file
        //

        if ( !(uStyle & OF_REOPEN) ) {
            PathLength = SearchPath(NULL,lpFileName,NULL,OFS_MAXPATHNAME-1,lpReOpenBuff->szPathName,&FilePart);
            if ( PathLength > (OFS_MAXPATHNAME - 1) ) {
                SetLastError(ERROR_INVALID_DATA);
                hFile = (HANDLE)-1;
                goto finally_exit;
                }
            if ( PathLength == 0 ) {
                SearchFailed = TRUE;
                PathLength = GetFullPathName(lpFileName,(OFS_MAXPATHNAME - 1),lpReOpenBuff->szPathName,&FilePart);
                if ( !PathLength || PathLength > (OFS_MAXPATHNAME - 1) ) {
                    SetLastError(ERROR_INVALID_DATA);
                    hFile = (HANDLE)-1;
                    goto finally_exit;
                    }
                }
            }

        //
        // Special case, Delete, Exist, and Parse
        //

        if ( uStyle & OF_EXIST ) {
            if ( !(uStyle & OF_CREATE) ) {
                DWORD FileAttributes;

                if (SearchFailed) {
                    SetLastError(ERROR_FILE_NOT_FOUND);
                    hFile = (HANDLE)-1;
                    goto finally_exit;
                    }

                FileAttributes = GetFileAttributesA(lpReOpenBuff->szPathName);
                if ( FileAttributes == 0xffffffff ) {
                    SetLastError(ERROR_FILE_NOT_FOUND);
                    hFile = (HANDLE)-1;
                    goto finally_exit;
                    }
                if ( FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                    SetLastError(ERROR_ACCESS_DENIED);
                    hFile = (HANDLE)-1;
                    goto finally_exit;
                    }
                else {
                    hFile = (HANDLE)1;
                    lpReOpenBuff->cBytes = sizeof(*lpReOpenBuff);
                    goto finally_exit;
                    }
                }
            }

        if ( uStyle & OF_DELETE ) {
            if ( DeleteFile(lpReOpenBuff->szPathName) ) {
                lpReOpenBuff->nErrCode = 0;
                lpReOpenBuff->cBytes = sizeof(*lpReOpenBuff);
                hFile = (HANDLE)1;
                goto finally_exit;
                }
            else {
                lpReOpenBuff->nErrCode = ERROR_FILE_NOT_FOUND;
                hFile = (HANDLE)-1;
                goto finally_exit;
                }
            }


        //
        // Open the file
        //

retry_open:
        hFile = CreateFile(
                    lpReOpenBuff->szPathName,
                    DesiredAccess,
                    ShareMode,
                    NULL,
                    CreateDisposition,
                    0,
                    NULL
                    );

        if ( hFile == INVALID_HANDLE_VALUE ) {

            if ( uStyle & OF_PROMPT && !(GetErrorMode() & SEM_NOOPENFILEERRORBOX) ) {
                {
                    DWORD WinErrorStatus;
                    NTSTATUS st,HardErrorStatus;
                    ULONG_PTR ErrorParameter;
                    ULONG ErrorResponse;
                    ANSI_STRING AnsiString;
                    UNICODE_STRING UnicodeString;

                    WinErrorStatus = GetLastError();
                    if ( WinErrorStatus == ERROR_FILE_NOT_FOUND ) {
                        HardErrorStatus = STATUS_NO_SUCH_FILE;
                        }
                    else if ( WinErrorStatus == ERROR_PATH_NOT_FOUND ) {
                        HardErrorStatus = STATUS_OBJECT_PATH_NOT_FOUND;
                        }
                    else {
                        goto finally_exit;
                        }

                    //
                    // Hard error time
                    //

                    RtlInitAnsiString(&AnsiString,lpReOpenBuff->szPathName);
                    st = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
                    if ( !NT_SUCCESS(st) ) {
                        goto finally_exit;
                        }
                    ErrorParameter = (ULONG_PTR)&UnicodeString;

                    HardErrorStatus = NtRaiseHardError(
                                        HardErrorStatus | HARDERROR_OVERRIDE_ERRORMODE,
                                        1,
                                        1,
                                        &ErrorParameter,
                                        OptionRetryCancel,
                                        &ErrorResponse
                                        );
                    RtlFreeUnicodeString(&UnicodeString);
                    if ( NT_SUCCESS(HardErrorStatus) && ErrorResponse == ResponseRetry ) {
                        goto retry_open;
                        }
                    }
                }
            goto finally_exit;
            }

        if ( uStyle & OF_EXIST ) {
            CloseHandle(hFile);
            hFile = (HANDLE)1;
            lpReOpenBuff->cBytes = sizeof(*lpReOpenBuff);
            goto finally_exit;
            }

        //
        // Determine if this is a hard disk.
        //

        Status = NtQueryVolumeInformationFile(
                    hFile,
                    &IoStatusBlock,
                    &DeviceInfo,
                    sizeof(DeviceInfo),
                    FileFsDeviceInformation
                    );
        if ( !NT_SUCCESS(Status) ) {
            CloseHandle(hFile);
            BaseSetLastNTError(Status);
            hFile = (HANDLE)-1;
            goto finally_exit;
            }
        switch ( DeviceInfo.DeviceType ) {

            case FILE_DEVICE_DISK:
            case FILE_DEVICE_DISK_FILE_SYSTEM:
                if ( DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA ) {
                    lpReOpenBuff->fFixedDisk = 0;
                    }
                else {
                    lpReOpenBuff->fFixedDisk = 1;
                    }
                break;

            default:
                lpReOpenBuff->fFixedDisk = 0;
                break;
            }

        //
        // Capture the last write time and save in the open struct.
        //

        b = GetFileTime(hFile,NULL,NULL,&LastWriteTime);

        if ( !b ) {
            lpReOpenBuff->Reserved1 = 0;
            lpReOpenBuff->Reserved2 = 0;
            }
        else {
            b = FileTimeToDosDateTime(
                    &LastWriteTime,
                    &lpReOpenBuff->Reserved1,
                    &lpReOpenBuff->Reserved2
                    );
            if ( !b ) {
                lpReOpenBuff->Reserved1 = 0;
                lpReOpenBuff->Reserved2 = 0;
                }
            }

        lpReOpenBuff->cBytes = sizeof(*lpReOpenBuff);

        //
        // The re-open buffer is completely filled in. Now
        // see if we are quitting (parsing), verifying, or
        // just returning with the file opened.
        //

        if ( uStyle & OF_VERIFY ) {
            if ( OriginalReOpenBuff.Reserved1 == lpReOpenBuff->Reserved1 &&
                 OriginalReOpenBuff.Reserved2 == lpReOpenBuff->Reserved2 &&
                 !strcmp(OriginalReOpenBuff.szPathName,lpReOpenBuff->szPathName) ) {
                goto finally_exit;
                }
            else {
                *lpReOpenBuff = OriginalReOpenBuff;
                CloseHandle(hFile);
                hFile = (HANDLE)-1;
                goto finally_exit;
                }
            }
finally_exit:;
        }
    finally {
        lpReOpenBuff->nErrCode = (WORD)GetLastError();
        }
    return (HFILE)HandleToUlong(hFile);
}




typedef DWORD (WINAPI *GETNAMEDSECURITYINFOWPTR)(
    IN  LPCWSTR                pObjectName,
    IN  SE_OBJECT_TYPE         ObjectType,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSID                 * ppsidOwner,
    OUT PSID                 * ppsidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

typedef DWORD (WINAPI *SETNAMEDSECURITYINFOWPTR)(
    IN LPCWSTR               pObjectName,
    IN SE_OBJECT_TYPE        ObjectType,
    IN SECURITY_INFORMATION  SecurityInfo,
    IN PSID                  psidOwner,
    IN PSID                  psidGroup,
    IN PACL                  pDacl,
    IN PACL                  pSacl
    );

BOOL
BasepCopySecurityInformation( LPCWSTR lpExistingFileName,
                              HANDLE SourceFile,
                              ACCESS_MASK SourceFileAccess,
                              LPCWSTR lpNewFileName,
                              HANDLE DestFile,
                              ACCESS_MASK DestFileAccess,
                              SECURITY_INFORMATION SecurityInformation,
                              LPCOPYFILE_CONTEXT Context,
                              DWORD DestFileFsAttributes,
                              PBOOL DeleteDest )

/*++

Routine Description:

    This is an internal routine that copies one or more of the DACL,
    SACL, owner, and group from the source to the dest file.

Arguments:

    lpExistingFileName - Provides the name of the source file.

    SourceFile - Provides a handle to that source file.

    SourceFileAccess - The access flags that were used to open SourceFile.

    lpNewFileName - Provides the name of the destination file.

    DestFile - Provides a handle to that destination file.

    DestFileAccess - The access flags that were used to open DestFile.

    SecurityInformation - Specifies what security should be copied (bit
        flag of the *_SECURITY_INFORMATION defines).

    Context - All the information necessary to call the CopyFile callback routine.

    DestFileFsAttributes - Provides the FILE_FS_ATTRIBUTE_INFORMATION.FileSystemAttributes
        for the dest file's volume.

    DeleteDest - Contains a pointer to a value that will be set to TRUE if this the dest
        file should be deleted.  This is the case if there is an error or the user
        cancels the operation.  If the user stops the operation, this routine still
        returns an error, but the dest file is not deleted.


Return Value:

    TRUE - The operation was successful.

    FALSE- The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    BOOLEAN Succeeded = FALSE;

    PACL Dacl = NULL;
    PACL Sacl = NULL;
    PSID Owner = NULL;
    PSID Group = NULL;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    DWORD dwError = 0;

    HANDLE Advapi32 = NULL;
    GETNAMEDSECURITYINFOWPTR GetNamedSecurityInfoWPtr = NULL;
    SETNAMEDSECURITYINFOWPTR SetNamedSecurityInfoWPtr = NULL;

    // If the source file isn't identified, there's nothing we can do.

    if( lpExistingFileName == NULL || lpNewFileName == NULL ) {
        Succeeded = TRUE;
        goto Exit;
    }

    // If the destination doesn't support ACLs, assume it doesn't
    // support any such security information (i.e. owner/group).

    if( !(FILE_PERSISTENT_ACLS & DestFileFsAttributes ) ) {

        if( BasepCopyFileCallback( TRUE,   // Continue (ignore the problem) by default
                                   ERROR_NOT_SUPPORTED,
                                   Context,
                                   NULL,
                                   PRIVCALLBACK_SECURITY_INFORMATION_NOT_SUPPORTED,
                                   SourceFile,
                                   DestFile,
                                   DeleteDest )) {
            // The caller wants to coninue on despite this.
            Succeeded = TRUE;
        }

        goto Exit;
    }

    // Check that DACL is copy-able if necessary

    if( SecurityInformation & DACL_SECURITY_INFORMATION ) {

        // We're supposed to copy the DACL.  Do we have enough access?
        if( !( SourceFileAccess & GENERIC_READ ) ||
            !( DestFileAccess & WRITE_DAC ) ) {

            SecurityInformation &= ~DACL_SECURITY_INFORMATION;

            if( !BasepCopyFileCallback( TRUE,   // Continue (ignore the problem) by default
                                        ERROR_ACCESS_DENIED,
                                        Context,
                                        NULL,
                                        PRIVCALLBACK_DACL_ACCESS_DENIED,
                                        SourceFile,
                                        DestFile,
                                        DeleteDest )) {
                goto Exit;
            }


        }
    }

    // Check that owner & group is copy-able if necessary

    if( (SecurityInformation & OWNER_SECURITY_INFORMATION) ||
        (SecurityInformation & GROUP_SECURITY_INFORMATION) ) {

        // We're supposed to copy owner & group.  Do we have enough access?

        if( !( SourceFileAccess & GENERIC_READ ) ||
            !( DestFileAccess & WRITE_OWNER ) ) {

            SecurityInformation &= ~(OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION);

            if( !BasepCopyFileCallback( TRUE,   // Continue (ignore the problem) by default
                                        ERROR_ACCESS_DENIED,
                                        Context,
                                        NULL,
                                        PRIVCALLBACK_OWNER_GROUP_ACCESS_DENIED,
                                        SourceFile,
                                        DestFile,
                                        DeleteDest )) {
                goto Exit;
            }


        }
    }

    // Check that SACL is copy-able if necessary

    if( SecurityInformation & SACL_SECURITY_INFORMATION ) {

        // We're supposed to copy the SACL.  Do we have enough rights?

        if( !(SourceFileAccess & ACCESS_SYSTEM_SECURITY) ||
            !(DestFileAccess & ACCESS_SYSTEM_SECURITY) ) {

            SecurityInformation &= ~SACL_SECURITY_INFORMATION;

            if( !BasepCopyFileCallback( TRUE,   // Continue (ignore the problem) by default
                                        ERROR_PRIVILEGE_NOT_HELD,
                                        Context,
                                        NULL,
                                        PRIVCALLBACK_SACL_ACCESS_DENIED,
                                        SourceFile,
                                        DestFile,
                                        DeleteDest )) {
                goto Exit;
            }

        }
    }

    // If nothing was copyable (and all was ignorable), then we're done.

    if( SecurityInformation == 0 ) {
        Succeeded = TRUE;
        goto Exit;
    }

    // Get the advapi32 APIs.

    Advapi32 = LoadLibrary("advapi32.dll");
    if( NULL == Advapi32 ) {
        *DeleteDest = TRUE;
        goto Exit;
    }


    GetNamedSecurityInfoWPtr     = (GETNAMEDSECURITYINFOWPTR) GetProcAddress( Advapi32,
                                                                              "GetNamedSecurityInfoW" );
    SetNamedSecurityInfoWPtr     = (SETNAMEDSECURITYINFOWPTR) GetProcAddress( Advapi32,
                                                                              "SetNamedSecurityInfoW" );

    if( GetNamedSecurityInfoWPtr == NULL ||
        SetNamedSecurityInfoWPtr == NULL ) {

        SetLastError( ERROR_INVALID_DLL );
        *DeleteDest = TRUE;
        goto Exit;
    }


    // Read in the security information from the source files

    dwError = GetNamedSecurityInfoWPtr( lpExistingFileName,
                                        SE_FILE_OBJECT,
                                        SecurityInformation,
                                        &Owner,
                                        &Group,
                                        &Dacl,
                                        &Sacl,
                                        &SecurityDescriptor );
    if( dwError != ERROR_SUCCESS ) {
        SetLastError( dwError );
        *DeleteDest = TRUE;
        goto Exit;
    }


    // We may have requested a DACL or SACL from a file that didn't have one.  If so,
    // don't try to set it (because it will cause a parameter error).

    if( Dacl == NULL ) {
        SecurityInformation &= ~DACL_SECURITY_INFORMATION;
    }
    if( Sacl == NULL ) {
        SecurityInformation &= ~SACL_SECURITY_INFORMATION;
    }

    // Set the security on the dest file.  This loops because it may
    // have to back off on what it requests.

    while( TRUE && SecurityInformation != 0 ) {


        dwError = SetNamedSecurityInfoWPtr( lpNewFileName,
                                            SE_FILE_OBJECT,
                                            SecurityInformation,
                                            Owner,
                                            Group,
                                            Dacl,
                                            Sacl );

        // Even if we have WRITE_OWNER access, the SID we're setting might not
        // be valid.  If so, see if we can retry without them.

        if( dwError == ERROR_SUCCESS ) {
            break;
        } else {

            if( SecurityInformation & (OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION) ) {

                if( !BasepCopyFileCallback( TRUE,   // Continue by default
                                            dwError,
                                            Context,
                                            NULL,
                                            PRIVCALLBACK_OWNER_GROUP_FAILED,
                                            SourceFile,
                                            DestFile,
                                            DeleteDest )) {
                    goto Exit;
                }

                // It's OK to ignore the owner/group.  Try again with them turned off.
                SecurityInformation &= ~(OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION);

            } else {

                // Samba 2.x says that it supports ACLs, but returns not-supported.
                if( !BasepCopyFileCallback( TRUE,   // Continue by default
                                            dwError,
                                            Context,
                                            NULL,
                                            PRIVCALLBACK_SECURITY_INFORMATION_NOT_SUPPORTED,
                                            SourceFile,
                                            DestFile,
                                            DeleteDest )) {
                    goto Exit;
                }

                SecurityInformation = 0;
            }

        }
    }   // while( TRUE && SecurityInformation != 0 )

    Succeeded = TRUE;

Exit:

    if( SecurityDescriptor != NULL ) {
        LocalFree( SecurityDescriptor );
    }

    if( Advapi32 != NULL ) {
        FreeLibrary( Advapi32 );
    }

    return( Succeeded );

}



BOOL
BasepCopyFileCallback( BOOL ContinueByDefault,
                       DWORD Win32ErrorOnStopOrCancel,
                       LPCOPYFILE_CONTEXT Context,
                       PLARGE_INTEGER StreamBytesCopied OPTIONAL,
                       DWORD CallbackReason,
                       HANDLE SourceFile,
                       HANDLE DestFile,
                       OPTIONAL PBOOL Canceled )
/*++

Routine Description:

    During CopyFile, call the CopyFileProgressCallback routine.

Arguments:

    ContinueByDefault - Value to use as the return code of this
        function if there is no callback function or the callback
        returns PROGRESS_REASON_NOT_HANDLED.

    Win32ErrorOnStopOrCancel - If the callback returns PROGRESS_STOP
        or PROGRESS_CANCEL set this as the last error.

    Context - Structure with the information necessary to call
        the callback.

    StreamBytesCopied - If provided, passed to the callback.  If not
        provided, zero is passed.

    CallbackReason - Passed to the callback as the dwReasonCode.

    SourceFile - The source of the CopyFile.

    DestFile - The destination of the CopyFile.

    Canceled - Pointer to a bool that on return indicates that the copy operation
                has been canceled by the user.

Return Value:

    TRUE - The CopyFile should continue.

    FALSE - The CopyFile should be aborted.  The last error will be set
        before this routine returns.

--*/

{
    PLARGE_INTEGER StreamBytes;
    LARGE_INTEGER Zero;
    DWORD ReturnCode;
    BOOL Continue = ContinueByDefault;

    // If there's no callback context or it's been quieted, then
    // there's nothing to do.

    if( Context == NULL || Context->lpProgressRoutine == NULL )
        return( Continue );

    // If the caller didn't provide a StreamBytesCopied, use zero.

    if( StreamBytesCopied == NULL ) {
        StreamBytes = &Zero;
        StreamBytes->QuadPart = 0;
    } else {
        StreamBytes = StreamBytesCopied;
    }

    // Call the callback

    ReturnCode = Context->lpProgressRoutine(
                    Context->TotalFileSize,
                    Context->TotalBytesTransferred,
                    Context->TotalFileSize,
                    *StreamBytes,
                    Context->dwStreamNumber,
                    CallbackReason,
                    SourceFile,
                    DestFile,
                    Context->lpData
                    );

    if( Canceled ) {
        *Canceled = FALSE;
    }

    switch( ReturnCode )
    {
    case PROGRESS_QUIET:
        Context->lpProgressRoutine = NULL;
        Continue = TRUE;
        break;

    case PROGRESS_CANCEL:
        if( Canceled ) {
            *Canceled = TRUE;
        }
        // Fall through

    case PROGRESS_STOP:
        SetLastError( Win32ErrorOnStopOrCancel );
        Continue = FALSE;
        break;

    case PRIVPROGRESS_REASON_NOT_HANDLED:
    case PROGRESS_CONTINUE:
        Continue = TRUE;
        break;

    // default?
    }

    return( Continue );

}





BOOL
WINAPI
ReplaceFileA(
    LPCSTR  lpReplacedFileName,
    LPCSTR  lpReplacementFileName,
    LPCSTR  lpBackupFileName OPTIONAL,
    DWORD   dwReplaceFlags,
    LPVOID  lpExclude,
    LPVOID  lpReserved
    )

/*++

Routine Description:

    ANSI thunk to ReplaceFileW

--*/

{
    UNICODE_STRING DynamicUnicodeReplaced;
    UNICODE_STRING DynamicUnicodeReplacement;
    UNICODE_STRING DynamicUnicodeBackup;
    BOOL b;

    //
    // Parameter validation.
    //

    if(NULL == lpReplacedFileName || NULL == lpReplacementFileName ||
       NULL != lpExclude || NULL != lpReserved ||
       dwReplaceFlags & ~(REPLACEFILE_WRITE_THROUGH | REPLACEFILE_IGNORE_MERGE_ERRORS)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!Basep8BitStringToDynamicUnicodeString( &DynamicUnicodeReplaced, lpReplacedFileName )) {
        return FALSE;
    }

    if (!Basep8BitStringToDynamicUnicodeString( &DynamicUnicodeReplacement, lpReplacementFileName )) {
        return FALSE;
    }

    if (lpBackupFileName && !Basep8BitStringToDynamicUnicodeString( &DynamicUnicodeBackup, lpBackupFileName )) {
        return FALSE;
    }

    if(lpBackupFileName) {
        b = ReplaceFileW(
                (LPCWSTR)DynamicUnicodeReplaced.Buffer,
                (LPCWSTR)DynamicUnicodeReplacement.Buffer,
                (LPCWSTR)DynamicUnicodeBackup.Buffer,
                dwReplaceFlags,
                lpExclude,
                lpReserved
                );
    } else {
        b = ReplaceFileW(
                (LPCWSTR)DynamicUnicodeReplaced.Buffer,
                (LPCWSTR)DynamicUnicodeReplacement.Buffer,
                NULL,
                dwReplaceFlags,
                lpExclude,
                lpReserved
                );
    }

    RtlFreeUnicodeString(&DynamicUnicodeReplaced);
    RtlFreeUnicodeString(&DynamicUnicodeReplacement);
    if(lpBackupFileName) {
        RtlFreeUnicodeString(&DynamicUnicodeBackup);
    }

    return b;
}

BOOL
WINAPI
ReplaceFileW(
    LPCWSTR lpReplacedFileName,
    LPCWSTR lpReplacementFileName,
    LPCWSTR lpBackupFileName OPTIONAL,
    DWORD   dwReplaceFlags,
    LPVOID  lpExclude,
    LPVOID  lpReserved
    )

/*++

Routine Description:

    Replace a file with a new file. The original file's attributes, alternate
    data streams, oid, acl, compression/encryption are transfered to the new
    file. If a backup file name is supplied, the original file is left at the
    backup file specified. Object ID, Create time/date, and file shortnames are
    tunneled by the system.

Arguments:

    lpReplacementFileName - name of the new file.

    lpReplacedFileName - name of the file to be replaced.

    lpBackupFileName - optional. If not NULL, the original file can be found
        under this name.

    dwReplaceFlags - specifies how the file is to be replaced. Currently, the
        possible values are:
        REPLACEFILE_WRITE_THROUGH   Setting this flag guarantees that any
                                    tunneled information is flushed to disk
                                    before the function returns.
        REPLACEFILE_IGNORE_MERGE_ERRORS Setting this flag lets the routine
                                        continue on with the operation even
                                        when merge error occurs. If this flag
                                        is set, GetLastError will not return
                                        ERROR_UNABLE_TO_MERGE_DATA.

    lpExclude - Reserved for future use. Must be set to NULL.

    lpReserved - for future use. Must be set to NULL.

Return Value:

    TRUE - The operation was successful.
    FALSE - The operation failed. Extended error status is available
        using GetLastError.

Error Code:

    ERROR_UNABLE_TO_REMOVE_REPLACED     The replacement file has inherited the
                                        replaced file's attributes and streams.
                                        the replaced file is unchanged. Both
                                        files still exist under their original
                                        names. No backup file exists.

    ERROR_UNABLE_TO_MOVE_REPLACEMENT    Same as above. Except that backup file
                                        exists if requested.

    ERROR_UNABLE_TO_MOVE_REPLACEMENT_2  The replacement file has inherited the
                                        replaced file's attributes and streams.
                                        It's still under its original name.
                                        Replaced file exists under the name of
                                        the backup file.

    All other error codes               Both replacement file and replaced file
                                        exist under their original names. The
                                        replacement file may have inherited
                                        none of, or part of, or all of the
                                        replaced file's attributes and streams.
                                        No backup file exists.

--*/

{
    HANDLE                          advapi32LibHandle = INVALID_HANDLE_VALUE;
    ENCRYPTFILEWPTR                 EncryptFileWPtr = NULL;
    DECRYPTFILEWPTR                 DecryptFileWPtr = NULL;
    HANDLE                          ReplacedFile = INVALID_HANDLE_VALUE;
    HANDLE                          ReplacementFile = INVALID_HANDLE_VALUE;
    HANDLE                          StreamHandle = INVALID_HANDLE_VALUE;
    HANDLE                          OutputStreamHandle = INVALID_HANDLE_VALUE;
    UNICODE_STRING                  ReplacedFileNTName;
    UNICODE_STRING                  ReplacementFileNTName;
    UNICODE_STRING                  StreamNTName;
    UNICODE_STRING                  BackupNTFileName;
    WCHAR                           StreamNTNameStr[MAXIMUM_FILENAME_LENGTH+1];
    RTL_RELATIVE_NAME               ReplacedRelativeName;
    RTL_RELATIVE_NAME               ReplacementRelativeName;
    OBJECT_ATTRIBUTES               ReplacedObjAttr;
    OBJECT_ATTRIBUTES               ReplacementObjAttr;
    OBJECT_ATTRIBUTES               StreamObjAttr;
    IO_STATUS_BLOCK                 IoStatusBlock;
    NTSTATUS                        status;
    BOOL                            fSuccess = FALSE;
    BOOL                            fDoCopy;
    PVOID                           ReplacedFreeBuffer = NULL;
    PVOID                           ReplacementFreeBuffer = NULL;
    FILE_BASIC_INFORMATION          ReplacedBasicInfo;
    FILE_BASIC_INFORMATION          ReplacementBasicInfo;
    DWORD                           ReplacementFileAccess;
    DWORD                           ReplacedFileAccess;
    FILE_COMPRESSION_INFORMATION    ReplacedCompressionInfo;
    PSECURITY_DESCRIPTOR            ReplacedSecDescPtr = NULL;
    DWORD                           dwSizeNeeded;
    ULONG                           cInfo;
    PFILE_STREAM_INFORMATION        ReplacedStreamInfo = NULL;
    PFILE_STREAM_INFORMATION        ReplacementStreamInfo = NULL;
    PFILE_STREAM_INFORMATION        ScannerStreamInfoReplaced = NULL;
    PFILE_STREAM_INFORMATION        ScannerStreamInfoReplacement = NULL;
    DWORD                           dwCopyFlags = COPY_FILE_FAIL_IF_EXISTS;
    DWORD                           dwCopySize = 0;
    PFILE_RENAME_INFORMATION        BackupReplaceRenameInfo = NULL;
    PFILE_RENAME_INFORMATION        ReplaceRenameInfo = NULL;
    LPCOPYFILE_CONTEXT              context = NULL;
    BOOL                            fQueryReplacedFileFail = FALSE;
    BOOL                            fQueryReplacementFileFail = FALSE;
    BOOL                            fReplacedFileIsEncrypted = FALSE;
    BOOL                            fReplacedFileIsCompressed = FALSE;
    BOOL                            fReplacementFileIsEncrypted = FALSE;
    BOOL                            fReplacementFileIsCompressed = FALSE;
    DWORD                           DestFileFsAttributes = 0;
    struct {
        FILE_FS_ATTRIBUTE_INFORMATION   Info;
        WCHAR                           Buffer[MAX_PATH];
    } ReplacementFsAttrInfoBuffer;

    //
    // Initialization
    //

    RtlInitUnicodeString(&BackupNTFileName, NULL);

    //
    // Parameter validation.
    //

    if(NULL == lpReplacedFileName || NULL == lpReplacementFileName ||
       NULL != lpExclude || NULL != lpReserved ||
       dwReplaceFlags & ~(REPLACEFILE_WRITE_THROUGH | REPLACEFILE_IGNORE_MERGE_ERRORS)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    try
    {
        //
        // Open the to-be-replaced file
        //

        RtlInitUnicodeString(&ReplacedFileNTName, NULL);
        if(!RtlDosPathNameToNtPathName_U(lpReplacedFileName,
                                         &ReplacedFileNTName,
                                         NULL,
                                         &ReplacedRelativeName)) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            leave;
        }
        ReplacedFreeBuffer = ReplacedFileNTName.Buffer;
        if(ReplacedRelativeName.RelativeName.Length) {
            ReplacedFileNTName = *(PUNICODE_STRING)&ReplacedRelativeName.RelativeName;
        }
        else {
            ReplacedRelativeName.ContainingDirectory = NULL;
        }
        InitializeObjectAttributes(&ReplacedObjAttr,
                                   &ReplacedFileNTName,
                                   OBJ_CASE_INSENSITIVE,
                                   ReplacedRelativeName.ContainingDirectory,
                                   NULL);
        ReplacedFileAccess = GENERIC_READ | DELETE | SYNCHRONIZE;
        status = NtOpenFile(&ReplacedFile,
                            ReplacedFileAccess,
                            &ReplacedObjAttr,
                            &IoStatusBlock,
                            FILE_SHARE_READ |
                            FILE_SHARE_WRITE |
                            FILE_SHARE_DELETE,
                            FILE_NON_DIRECTORY_FILE |
                            FILE_SYNCHRONOUS_IO_NONALERT);
        if(!NT_SUCCESS(status))
        {
            BaseSetLastNTError(status);
            leave;
        }

        //
        // Open the replacement file
        //

        if(!RtlDosPathNameToNtPathName_U(lpReplacementFileName,
                                         &ReplacementFileNTName,
                                         NULL,
                                         &ReplacementRelativeName)) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            leave;
        }
        ReplacementFreeBuffer = ReplacementFileNTName.Buffer;
        if(ReplacementRelativeName.RelativeName.Length) {
            ReplacementFileNTName = *(PUNICODE_STRING)&ReplacementRelativeName.RelativeName;
        }
        else {
            ReplacementRelativeName.ContainingDirectory = NULL;
        }
        InitializeObjectAttributes(&ReplacementObjAttr,
                                   &ReplacementFileNTName,
                                   OBJ_CASE_INSENSITIVE,
                                   ReplacementRelativeName.ContainingDirectory,
                                   NULL);
        ReplacementFileAccess = SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE | DELETE | WRITE_DAC;
        status = NtOpenFile(&ReplacementFile,
                            ReplacementFileAccess,
                            &ReplacementObjAttr,
                            &IoStatusBlock,
                            0,
                            FILE_NON_DIRECTORY_FILE |
                            FILE_SYNCHRONOUS_IO_NONALERT);
        if(STATUS_ACCESS_DENIED == status &&
           dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS) {
            ReplacementFileAccess = SYNCHRONIZE | GENERIC_READ | DELETE | WRITE_DAC;
            status = NtOpenFile(&ReplacementFile,
                                ReplacementFileAccess,
                                &ReplacementObjAttr,
                                &IoStatusBlock,
                                0,
                                FILE_NON_DIRECTORY_FILE |
                                FILE_SYNCHRONOUS_IO_NONALERT);
        }

        // BUGBUG   If NtOpenFile fails again, try opening the file without
        // WRITE_DAC access.

        if(!NT_SUCCESS(status))
        {
            BaseSetLastNTError(status);
            leave;
        }

        //
        // Get the attributes of the to-be-replaced file and set them on the
        // replacement file. FILE_ATTRIBUTE_COMPRESSED and
        // FILE_ATTRIBUTE_ENCRYPTED can be obtained by NtQueryInformationFile,
        // but can't be set by NtSetInformationFile. Compression and
        // encryption will be handled later.
        //

        status = NtQueryInformationFile(ReplacedFile,
                                        &IoStatusBlock,
                                        &ReplacedBasicInfo,
                                        sizeof(ReplacedBasicInfo),
                                        FileBasicInformation);
        if(!NT_SUCCESS(status)) {
            if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                BaseSetLastNTError(status);
                leave;
            }
            fQueryReplacedFileFail = TRUE;
        }
        else {
            status = NtQueryInformationFile(ReplacementFile,
                                            &IoStatusBlock,
                                            &ReplacementBasicInfo,
                                            sizeof(ReplacementBasicInfo),
                                            FileBasicInformation);
            if(!NT_SUCCESS(status)) {
                if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                    BaseSetLastNTError(status);
                    leave;
                }
                fQueryReplacementFileFail = TRUE;
            }

            //
            // Creation time is the only time we want to preserve. So zero out
            // all the other times.
            //
            ReplacedBasicInfo.LastAccessTime.QuadPart = 0;
            ReplacedBasicInfo.LastWriteTime.QuadPart = 0;
            ReplacedBasicInfo.ChangeTime.QuadPart = 0;
            status = NtSetInformationFile(ReplacementFile,
                                          &IoStatusBlock,
                                          &ReplacedBasicInfo,
                                          sizeof(ReplacedBasicInfo),
                                          FileBasicInformation);
            if(!NT_SUCCESS(status) &&
               !(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                BaseSetLastNTError(status);
                leave;
            }
        }

        //
        // Transfer ACLs from the to-be-replaced file to the replacement file.
        //

        status = NtQueryVolumeInformationFile(ReplacementFile,
                                              &IoStatusBlock,
                                              &ReplacementFsAttrInfoBuffer.Info,
                                              sizeof(ReplacementFsAttrInfoBuffer),
                                              FileFsAttributeInformation);
        if(!NT_SUCCESS(status)) {
            if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                BaseSetLastNTError(status);
                leave;
            }
        }
        else
        {
            BOOL Delete = FALSE;
            if( !BasepCopySecurityInformation( lpReplacedFileName,
                                               ReplacedFile,
                                               ReplacedFileAccess,
                                               lpReplacementFileName,
                                               ReplacementFile,
                                               ReplacementFileAccess,
                                               DACL_SECURITY_INFORMATION,
                                               NULL,
                                               ReplacementFsAttrInfoBuffer.Info.FileSystemAttributes,
                                               &Delete )) {
                leave;

            }
        }


        //
        // If the to-be-replaced file has alternate data streams, and they do
        // not exist in the replacement file, copy them into the replacement
        // file.
        //

        cInfo = 4096;
        do {
            ReplacedStreamInfo = RtlAllocateHeap(RtlProcessHeap(),
                                                  MAKE_TAG(TMP_TAG),
                                                  cInfo);
            if (!ReplacedStreamInfo) {
                break;
            }
            status = NtQueryInformationFile(ReplacedFile,
                                            &IoStatusBlock,
                                            ReplacedStreamInfo,
                                            cInfo,
                                            FileStreamInformation);
            if (!NT_SUCCESS(status)) {
                RtlFreeHeap(RtlProcessHeap(), 0, ReplacedStreamInfo);
                ReplacedStreamInfo = NULL;
                cInfo *= 2;
            }
        } while(status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL);
        if(NULL == ReplacedStreamInfo) {
            if(status != STATUS_INVALID_PARAMETER &&
               status != STATUS_NOT_IMPLEMENTED) {
                if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                    BaseSetLastNTError(status);
                    leave;
                }
            }
        }
        else {
            if(!NT_SUCCESS(status)) {
                if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                    BaseSetLastNTError(status);
                    leave;
                }
            }
            else {
                // The outer loop enumerates streams in the to-be-replaced file.
                ScannerStreamInfoReplaced = ReplacedStreamInfo;
                while(TRUE) {
                    // Skip the default stream.
                    if(ScannerStreamInfoReplaced->StreamNameLength <= sizeof(WCHAR) ||
                        ScannerStreamInfoReplaced->StreamName[1] == ':') {
                        if(0 == ScannerStreamInfoReplaced->NextEntryOffset) {
                            break;
                        }
                        ScannerStreamInfoReplaced = (PFILE_STREAM_INFORMATION)((PCHAR)ScannerStreamInfoReplaced + ScannerStreamInfoReplaced->NextEntryOffset);
                        continue;
                    }

                    // Query replacement file stream information if we haven't done so.
                    // We wait until now to do this query because we don't want to do
                    // it unless it's absolutely necessary.
                    if(NULL == ReplacementStreamInfo) {
                        cInfo = 4096;
                        do {
                            ReplacementStreamInfo = RtlAllocateHeap(RtlProcessHeap(),
                                                                     MAKE_TAG(TMP_TAG),
                                                             cInfo);
                            if (!ReplacementStreamInfo) {
                                break;
                            }
                            status = NtQueryInformationFile(ReplacementFile,
                                                            &IoStatusBlock,
                                                            ReplacementStreamInfo,
                                                            cInfo,
                                                            FileStreamInformation);
                            if (!NT_SUCCESS(status)) {
                                RtlFreeHeap(RtlProcessHeap(), 0, ReplacementStreamInfo);
                                ReplacementStreamInfo = NULL;
                                cInfo *= 2;
                            }
                        } while(status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL);
                        if(NULL == ReplacementStreamInfo ||
                           !NT_SUCCESS(status)) {
                            if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                                BaseSetLastNTError(status);
                                leave;
                            }
                            break;
                        }
                    }

                    // The inner loop enumerates the replacement file streams.
                    ScannerStreamInfoReplacement = ReplacementStreamInfo;
                    fDoCopy = TRUE;
                    while(TRUE) {
                        if(ScannerStreamInfoReplaced->StreamNameLength == ScannerStreamInfoReplacement->StreamNameLength &&
                           _wcsnicmp(ScannerStreamInfoReplaced->StreamName, ScannerStreamInfoReplacement->StreamName, ScannerStreamInfoReplacement->StreamNameLength / sizeof(WCHAR)) == 0) {
                            // The stream already exists in the replacement file.
                            fDoCopy = FALSE;
                            break;
                        }
                        if(0 == ScannerStreamInfoReplacement->NextEntryOffset) {
                            // end of the stream information
                            break;
                        }
                        ScannerStreamInfoReplacement = (PFILE_STREAM_INFORMATION)((PCHAR)ScannerStreamInfoReplacement + ScannerStreamInfoReplacement->NextEntryOffset);
                    }

                    // We copy the stream if it doesn't exist in the replacement file.
                    if(TRUE == fDoCopy) {
                        StreamNTName.Buffer = &ScannerStreamInfoReplaced->StreamName[0];
                        StreamNTName.Length = (USHORT)ScannerStreamInfoReplaced->StreamNameLength;
                        StreamNTName.MaximumLength = StreamNTName.Length;

                        // Open the stream in the to-be-replaced file.
                        InitializeObjectAttributes(&StreamObjAttr,
                                                   &StreamNTName,
                                                   0,
                                                   ReplacedFile,
                                                   NULL);
                        status = NtOpenFile(&StreamHandle,
                                            SYNCHRONIZE |
                                            GENERIC_READ,
                                            &StreamObjAttr,
                                            &IoStatusBlock,
                                            FILE_SHARE_READ |
                                            FILE_SHARE_WRITE |
                                            FILE_SHARE_DELETE,
                                            FILE_SYNCHRONOUS_IO_NONALERT |
                                            FILE_SEQUENTIAL_ONLY);
                        if(!NT_SUCCESS(status)) {
                            if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                                BaseSetLastNTError(status);
                                leave;
                            }

                            if(0 == ScannerStreamInfoReplaced->NextEntryOffset) {
                                break;
                            }

                            ScannerStreamInfoReplaced = (PFILE_STREAM_INFORMATION)((PCHAR)ScannerStreamInfoReplaced + ScannerStreamInfoReplaced->NextEntryOffset);
                            continue;
                        }

                        // Copy the stream;
                        RtlCopyMemory(StreamNTNameStr, StreamNTName.Buffer, StreamNTName.Length);
                        StreamNTNameStr[StreamNTName.Length / sizeof(WCHAR)] = L'\0';
                        OutputStreamHandle = INVALID_HANDLE_VALUE;
                        if(!BaseCopyStream(NULL,
                                           StreamHandle,
                                           SYNCHRONIZE | GENERIC_READ,
                                           StreamNTNameStr,
                                           ReplacementFile,
                                           &ScannerStreamInfoReplaced->StreamSize,
                                           &dwCopyFlags,
                                           &OutputStreamHandle,
                                           &dwCopySize,
                                           &context,
                                           NULL,
                                           FALSE,
                                           0,
                                           &DestFileFsAttributes )) {
                            if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                                leave;
                            }
                        }
                        NtClose(StreamHandle);
                        StreamHandle = INVALID_HANDLE_VALUE;
                    } // copy stream

                    if(0 == ScannerStreamInfoReplaced->NextEntryOffset) {
                        break;
                    }

                    ScannerStreamInfoReplaced = (PFILE_STREAM_INFORMATION)((PCHAR)ScannerStreamInfoReplaced + ScannerStreamInfoReplaced->NextEntryOffset);
                } // outer loop
            }
        }

        //
        // Compression/Encryption.
        //

        // If we successfully read the to-be-replaced file's attributes, we
        // do the necessary compression/encryption. Otherwise we do nothing.
        // If we don't know the replacement files attributes
        // (fQueryReplacementFileFail is TRUE), to be on the safe side, we will
        // try to (un)compress/(un)encrypt it if the to-be-replaced file is
        // (un)compressed/(un)encrypted.
        if(!fQueryReplacedFileFail) {

            fReplacedFileIsEncrypted = ReplacedBasicInfo.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED;
            fReplacedFileIsCompressed = ReplacedBasicInfo.FileAttributes & FILE_ATTRIBUTE_COMPRESSED;
            if(!fQueryReplacementFileFail) {
                fReplacementFileIsEncrypted = ReplacementBasicInfo.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED;
                fReplacementFileIsCompressed = ReplacementBasicInfo.FileAttributes & FILE_ATTRIBUTE_COMPRESSED;
            }
            else {
                // If we don't know the file attributes of the replacement
                // file, we'll assume the replacement file has opposite
                // encryption/compression attributes as the replaced file
                // so that encryption/compression operations will be forced
                // on the replacement file.
                fReplacementFileIsEncrypted = !fReplacedFileIsEncrypted;
                fReplacementFileIsCompressed = !fReplacedFileIsCompressed;
            }

            //
            // Encryption.
            //

            // If the to-be-replaced file is encrypted and either the
            // replacement file is encrypted or we don't know it's encryption
            // status, we try to encrypt the replacement file.
            if(fReplacedFileIsEncrypted && !fReplacementFileIsEncrypted) {
                NtClose(ReplacementFile);
                ReplacementFile = INVALID_HANDLE_VALUE;
                // There's no way to encrypt a file based on its handle.  We
                // must use the Win32 API (which calls over to the EFS service).
                advapi32LibHandle = LoadLibrary("advapi32");
                if(NULL == advapi32LibHandle) {
                    if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                        leave;
                    }
                }
                else {
                    EncryptFileWPtr = (ENCRYPTFILEWPTR)GetProcAddress(advapi32LibHandle, "EncryptFileW");
                    if(NULL == EncryptFileWPtr) {
                        if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                            leave;
                        }
                    }
                    else {
                        if((EncryptFileWPtr)(lpReplacementFileName)) {
                            // Encryption operation automatically decompresses
                            // compressed files. We need this flag for the
                            // case when the replaced file is encrypted and
                            // the replacement file is compressed. At this
                            // point, the replacement file is encrypted.
                            // Because a file is automatically decompressed
                            // when it's encrypted, we don't want to
                            // decompress it again, otherwise we'll get an
                            // error.
                            fReplacementFileIsCompressed = FALSE;
                        } else if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                            leave;
                        }
                    }
                }
                status = NtOpenFile(&ReplacementFile,
                                    SYNCHRONIZE |
                                    GENERIC_READ |
                                    GENERIC_WRITE |
                                    WRITE_DAC |
                                    DELETE,
                                    &ReplacementObjAttr,
                                    &IoStatusBlock,
                                    0,
                                    FILE_NON_DIRECTORY_FILE |
                                    FILE_SYNCHRONOUS_IO_NONALERT);
                if(STATUS_ACCESS_DENIED == status &&
                   dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS) {
                    status = NtOpenFile(&ReplacementFile,
                                        SYNCHRONIZE |
                                        GENERIC_READ |
                                        DELETE |
                                        WRITE_DAC,
                                        &ReplacementObjAttr,
                                        &IoStatusBlock,
                                        0,
                                        FILE_NON_DIRECTORY_FILE |
                                        FILE_SYNCHRONOUS_IO_NONALERT);
                }

                // BUGBUG   If NtOpenFile fails again, try opening the file
                // without WRITE_DAC access.

                // We leave without attempt to rename the files because we know
                // we can't rename the replacement file without it being opened
                // first.
                if(!NT_SUCCESS(status)) {
                    BaseSetLastNTError(status);
                    leave;
                }
            }
            else if(!fReplacedFileIsEncrypted && fReplacementFileIsEncrypted) {
                // If the to-be-replaced file is not encrypted and the
                // replacement file is encrypted, decrypt the replacement file.
                NtClose(ReplacementFile);
                ReplacementFile = INVALID_HANDLE_VALUE;
                advapi32LibHandle = LoadLibrary("advapi32");
                if(NULL == advapi32LibHandle) {
                    if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                        leave;
                    }
                }
                else {
                    DecryptFileWPtr = (DECRYPTFILEWPTR)GetProcAddress(advapi32LibHandle, "DecryptFileW");
                    if(NULL == DecryptFileWPtr) {
                        if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                            leave;
                        }
                    }
                    else {
                        if((DecryptFileWPtr)(lpReplacementFileName, 0)) {
                            fReplacementFileIsEncrypted = FALSE;
                        } else if(!(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                            leave;
                        }
                    }
                }
                status = NtOpenFile(&ReplacementFile,
                                    SYNCHRONIZE |
                                    GENERIC_READ |
                                    GENERIC_WRITE |
                                    WRITE_DAC |
                                    DELETE,
                                    &ReplacementObjAttr,
                                    &IoStatusBlock,
                                    0,
                                    FILE_NON_DIRECTORY_FILE |
                                    FILE_SYNCHRONOUS_IO_NONALERT);
                // We leave without attempt to rename the files because we know
                // we can't rename the replacement file without it being opened
                // first.
                if(!NT_SUCCESS(status)) {
                    BaseSetLastNTError(status);
                    leave;
                }
            }

            //
            // Compression.
            //

            // If the to-be-replaced file is compressed, and the replacement
            // file is not, we compress the replacement file. In the case that
            // we don't know if the replacement file is compressed or not
            // (fQueryReplacementFileFail is TRUE), we will
            // try to compress it anyway and ignore the error if it's already
            // compressed.
            if(fReplacedFileIsCompressed && !fReplacementFileIsCompressed) {
                // Get the compression method mode.
                status = NtQueryInformationFile(ReplacedFile,
                                                &IoStatusBlock,
                                                &ReplacedCompressionInfo,
                                                sizeof(FILE_COMPRESSION_INFORMATION),
                                                FileCompressionInformation);
                if(!NT_SUCCESS(status)) {
                    // We couldn't get the compression method code. if the
                    // ignore merge error flag is on, we continue on to
                    // encryption. Otherwise, we set last error and leave.
                    if(!fQueryReplacementFileFail &&
                       !(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                        BaseSetLastNTError(status);
                        leave;
                    }
                }
                else {
                    // Do the compression. If we fail and ignore failure flag
                    // is not on, set error and leave. Otherwise continue on
                    // to encryption.
                    status = NtFsControlFile(ReplacementFile,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &IoStatusBlock,
                                             FSCTL_SET_COMPRESSION,
                                             &ReplacedCompressionInfo.CompressionFormat,
                                             sizeof(ReplacedCompressionInfo.CompressionFormat),
                                             NULL,
                                             0);
                    if(!fQueryReplacementFileFail && !NT_SUCCESS(status) &&
                       !(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                            BaseSetLastNTError(status);
                            leave;
                    }
                }
            }
            else if(!fReplacedFileIsCompressed && fReplacementFileIsCompressed && !fReplacementFileIsEncrypted) {
                // The replaced file is not compressed, the replacement file
                // is compressed (or that the query information for replacement
                // file failed and we don't know if it's compressed or not),
                // decompress the replacement file.
                USHORT      CompressionFormat = 0;
                status = NtFsControlFile(ReplacementFile,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &IoStatusBlock,
                                         FSCTL_SET_COMPRESSION,
                                         &CompressionFormat,
                                         sizeof(CompressionFormat),
                                         NULL,
                                         0);
                if(!fQueryReplacementFileFail && !NT_SUCCESS(status) &&
                   !(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS)) {
                        BaseSetLastNTError(status);
                        leave;
                }
            }
        } // if querying replaced file attribute failed.

        //
        // Do the renames.
        //

        // If backup file requested, rename the to-be-replaced file to backup.
        // Otherwise delete it.
        if(lpBackupFileName) {
            if(!RtlDosPathNameToNtPathName_U(lpBackupFileName,
                                             &BackupNTFileName,
                                             NULL,
                                             NULL)) {
                SetLastError(ERROR_PATH_NOT_FOUND);
                leave;
            }

            BackupReplaceRenameInfo = RtlAllocateHeap(RtlProcessHeap(),
                                              MAKE_TAG(TMP_TAG),
                                              BackupNTFileName.Length +
                                              sizeof(*BackupReplaceRenameInfo));
            if(NULL == BackupReplaceRenameInfo)
            {
                SetLastError(ERROR_UNABLE_TO_REMOVE_REPLACED);
                leave;
            }
            BackupReplaceRenameInfo->ReplaceIfExists = TRUE;
            BackupReplaceRenameInfo->RootDirectory = NULL;
            BackupReplaceRenameInfo->FileNameLength = BackupNTFileName.Length;
            RtlCopyMemory(BackupReplaceRenameInfo->FileName, BackupNTFileName.Buffer, BackupNTFileName.Length);
            status = NtSetInformationFile(ReplacedFile,
                                          &IoStatusBlock,
                                          BackupReplaceRenameInfo,
                                          BackupNTFileName.Length +
                                          sizeof(*BackupReplaceRenameInfo),
                                          FileRenameInformation);
            if(!NT_SUCCESS(status)) {
                SetLastError(ERROR_UNABLE_TO_REMOVE_REPLACED);
                leave;
            }
        }
        else {
            FILE_DISPOSITION_INFORMATION    fileDispositionInformation = {TRUE};

            status = NtSetInformationFile(ReplacedFile,
                                 &IoStatusBlock,
                                 &fileDispositionInformation,
                                 sizeof(fileDispositionInformation),
                                 FileDispositionInformation);
            if(!NT_SUCCESS(status) && !(dwReplaceFlags & REPLACEFILE_IGNORE_MERGE_ERRORS))
            {
                // We can't delete the replaced file, so object id will not be
                // tunnelled. If the REPLACEFILE_IGNORE_MERGE_ERRORS flag is
                // not on, we have to return error.
                BaseSetLastNTError(status);
                leave;
            }

            NtClose(ReplacedFile);
            ReplacedFile = INVALID_HANDLE_VALUE;
        }

        // Rename the replacement file to the replaced file.
        ReplaceRenameInfo = RtlAllocateHeap(RtlProcessHeap(),
                                      MAKE_TAG(TMP_TAG),
                                      ReplacedFileNTName.Length +
                                      sizeof(*ReplaceRenameInfo));
        if(NULL == ReplaceRenameInfo)
        {
            SetLastError(ERROR_UNABLE_TO_MOVE_REPLACEMENT);
            leave;
        }
        ReplaceRenameInfo->ReplaceIfExists = TRUE;
        ReplaceRenameInfo->RootDirectory = NULL;
        ReplaceRenameInfo->FileNameLength = ReplacedFileNTName.Length;
        RtlCopyMemory(ReplaceRenameInfo->FileName, ReplacedFileNTName.Buffer, ReplacedFileNTName.Length);
        status = NtSetInformationFile(ReplacementFile,
                                      &IoStatusBlock,
                                      ReplaceRenameInfo,
                                      ReplacedFileNTName.Length +
                                      sizeof(*ReplaceRenameInfo),
                                      FileRenameInformation);
        if(!NT_SUCCESS(status)) {
            // If we failed to rename the replacement file, and a backup file
            // for the original file exists, we try to restore the original
            // file from the backup file.
            if(lpBackupFileName) {
                status = NtSetInformationFile(ReplacedFile,
                                              &IoStatusBlock,
                                              ReplaceRenameInfo,
                                              ReplacedFileNTName.Length +
                                              sizeof(*ReplaceRenameInfo),
                                              FileRenameInformation);
                if(!NT_SUCCESS(status)) {
                    SetLastError(ERROR_UNABLE_TO_MOVE_REPLACEMENT_2);
                }
                else {
                    SetLastError(ERROR_UNABLE_TO_MOVE_REPLACEMENT);
                }
                leave;
            }
            else {
                SetLastError(ERROR_UNABLE_TO_MOVE_REPLACEMENT);
                leave;
            }
        }

        //
        // All is well. We set the return code to TRUE. And flush the files if
        // necessary.
        //

        if(dwReplaceFlags & REPLACEFILE_WRITE_THROUGH) {
            NtFlushBuffersFile(ReplacedFile, &IoStatusBlock);
        }

        fSuccess = TRUE;
    }
    finally {
        if(INVALID_HANDLE_VALUE != advapi32LibHandle && NULL != advapi32LibHandle) {
            FreeLibrary(advapi32LibHandle);
        }

        if(INVALID_HANDLE_VALUE != ReplacedFile) {
            NtClose(ReplacedFile);
        }
        if(INVALID_HANDLE_VALUE != ReplacementFile) {
            NtClose(ReplacementFile);
        }
        if(INVALID_HANDLE_VALUE != StreamHandle) {
            NtClose(StreamHandle);
        }
        if(INVALID_HANDLE_VALUE != OutputStreamHandle) {
            NtClose(OutputStreamHandle);
        }

        RtlFreeHeap(RtlProcessHeap(), 0, ReplacedFreeBuffer);
        RtlFreeHeap(RtlProcessHeap(), 0, ReplacementFreeBuffer);
        RtlFreeHeap(RtlProcessHeap(), 0, BackupNTFileName.Buffer);

        RtlFreeHeap(RtlProcessHeap(), 0, ReplacedStreamInfo);
        RtlFreeHeap(RtlProcessHeap(), 0, ReplacementStreamInfo);
        RtlFreeHeap(RtlProcessHeap(), 0, ReplaceRenameInfo);
        RtlFreeHeap(RtlProcessHeap(), 0, BackupReplaceRenameInfo);
    }

Exit:

    return fSuccess;
}


VOID
BaseMarkFileForDelete(
    HANDLE File,
    DWORD FileAttributes
    )

/*++

Routine Description:

    This routine marks a file for delete, so that when the supplied handle
    is closed, the file will actually be deleted.

Arguments:

    File - Supplies a handle to the file that is to be marked for delete.

    FileAttributes - Attributes for the file, if known.  Zero indicates they
        are unknown.

Return Value:

    None.

--*/

{
    #undef DeleteFile

    FILE_DISPOSITION_INFORMATION DispositionInformation;
    IO_STATUS_BLOCK IoStatus;
    FILE_BASIC_INFORMATION BasicInformation;

    if (!FileAttributes) {
        BasicInformation.FileAttributes = 0;
        NtQueryInformationFile(
            File,
            &IoStatus,
            &BasicInformation,
            sizeof(BasicInformation),
            FileBasicInformation
            );
        FileAttributes = BasicInformation.FileAttributes;
        }

    if (FileAttributes & FILE_ATTRIBUTE_READONLY) {
        RtlZeroMemory(&BasicInformation, sizeof(BasicInformation));
        BasicInformation.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        NtSetInformationFile(
            File,
            &IoStatus,
            &BasicInformation,
            sizeof(BasicInformation),
            FileBasicInformation
            );
        }

    DispositionInformation.DeleteFile = TRUE;
    NtSetInformationFile(
        File,
        &IoStatus,
        &DispositionInformation,
        sizeof(DispositionInformation),
        FileDispositionInformation
        );

}
