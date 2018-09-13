/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    winlink.c

Abstract:

    This module implements Win32 CreateHardLink

Author:

    Felipe Cabrera (cabrera) 28-Feb-1997

Revision History:

--*/

#include "basedll.h"

BOOL
APIENTRY
CreateHardLinkA(
    LPCSTR lpLinkName,
    LPCSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    ANSI thunk to CreateHardLinkW

--*/

{
    PUNICODE_STRING Unicode;
    UNICODE_STRING UnicodeExistingFileName;
    BOOL ReturnValue;

    Unicode = Basep8BitStringToStaticUnicodeString( lpLinkName );
    if (Unicode == NULL) {
        return FALSE;
    }
    
    if ( ARGUMENT_PRESENT(lpExistingFileName) ) {
        if (!Basep8BitStringToDynamicUnicodeString( &UnicodeExistingFileName, lpExistingFileName )) {
            return FALSE;
            }
        }
    else {
        UnicodeExistingFileName.Buffer = NULL;
        }

    ReturnValue = CreateHardLinkW((LPCWSTR)Unicode->Buffer, (LPCWSTR)UnicodeExistingFileName.Buffer, lpSecurityAttributes);

    RtlFreeUnicodeString(&UnicodeExistingFileName);

    return ReturnValue;
}


BOOL
APIENTRY
CreateHardLinkW(
    LPCWSTR lpLinkName,
    LPCWSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    A file can be made to be a hard link to an existing file.
    The existing file can be a reparse point or not.

Arguments:

    lpLinkName - Supplies the name of a file that is to be to be made a hard link. As
        this is to be a new hard link, there should be no file or directory present
        with this name.

    lpExistingFileName - Supplies the name of an existing file that is the target for
        the hard link.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    BOOLEAN TranslationStatus;
    UNICODE_STRING OldFileName;
    UNICODE_STRING NewFileName;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG OpenFlags;
    PFILE_LINK_INFORMATION NewName;
    HANDLE FileHandle;
    DWORD FullPathLength;
    PWSTR FullPath, FilePart;

    //
    // Check to see that both names are present.
    //

    if ( !ARGUMENT_PRESENT(lpLinkName) ||
         !ARGUMENT_PRESENT(lpExistingFileName) ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Check to see if any of the names is on a remote share. We fail the
    // request if we find any UNC name.
    //

    if ( (RtlDetermineDosPathNameType_U(lpLinkName) == RtlPathTypeUncAbsolute) ||
         (RtlDetermineDosPathNameType_U(lpExistingFileName) == RtlPathTypeUncAbsolute) ) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
        }

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpExistingFileName,
                            &OldFileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    FreeBuffer = OldFileName.Buffer;

    //
    // Initialize the object name.
    //

    if ( RelativeName.RelativeName.Length ) {
        OldFileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    } else {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        &OldFileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );
                                   
    //
    // Determine whether the drive letter of the existing name is a mapped drive.
    // Fail the request if it is.
    //

    FullPathLength = GetFullPathNameW(lpExistingFileName, 0, NULL, NULL);
    if ( !FullPathLength ) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }
    
    FullPathLength += 2;
    FullPath = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG), FullPathLength*sizeof(WCHAR));
    if ( !FullPath ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if ( !GetFullPathNameW(lpExistingFileName, FullPathLength, FullPath, &FilePart) ) {
        RtlFreeHeap(RtlProcessHeap(), 0, FullPath);
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    //
    // Note that with GetFullPathName if the drive letter is  H:  then the prefix
    // has the form  H:..., thus the drive letter is in offset 0 of the unicode 
    // buffer.
    //

    if ( FullPathLength > 4 ) {

        NTSTATUS           status;
        UNICODE_STRING     linkValue, drivePath, fullName;
        OBJECT_ATTRIBUTES  objectAttributes;
        HANDLE             linkHandle;
        PWCHAR             linkValueBuffer = NULL;   //  MAX_PATH is 260
        WCHAR              pathNameValue[sizeof(L"\\??\\C:\0")];

        RtlCopyMemory( &pathNameValue, L"\\??\\C:\0", sizeof(L"\\??\\C:\0") );

        RtlInitUnicodeString( &drivePath, pathNameValue );
        RtlInitUnicodeString( &fullName, FullPath );

        //
        // Place the appropriate drive letter in the buffer overwriting offset 4.
        //

        drivePath.Buffer[4] = fullName.Buffer[0];

        InitializeObjectAttributes( 
            &objectAttributes,
            &drivePath,
            OBJ_CASE_INSENSITIVE,
            (HANDLE) NULL,
            (PSECURITY_DESCRIPTOR) NULL 
            );

        status = NtOpenSymbolicLinkObject( 
                     &linkHandle,
                     SYMBOLIC_LINK_QUERY,
                     &objectAttributes 
                     );

        if ( NT_SUCCESS( status ) ) {

            //
            // Now query the link and see if there is a redirection.
            //

            linkValueBuffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), 2 * 260);

            if ( !linkValueBuffer ) {

                //
                // Insufficient resources. Return FALSE.
                //

                NtClose( linkHandle );

                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                RtlFreeHeap(RtlProcessHeap(), 0, FullPath);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }

            linkValue.Buffer = linkValueBuffer;
            linkValue.Length = 0;
            linkValue.MaximumLength = (USHORT)(2 * 260);

            status = NtQuerySymbolicLinkObject( 
                         linkHandle,
                         &linkValue,
                         NULL 
                         );

            NtClose( linkHandle );
            
            if ( NT_SUCCESS( status ) ) {

                //
                // The link is a re-directed drive only when it has the prefix
                // \Device\LanmanRedirector\
                //

                if ( (linkValue.Buffer[ 0] == L'\\') &&
                     (linkValue.Buffer[ 1] == L'D') &&
                     (linkValue.Buffer[ 2] == L'e') &&
                     (linkValue.Buffer[ 3] == L'v') &&
                     (linkValue.Buffer[ 4] == L'i') &&
                     (linkValue.Buffer[ 5] == L'c') &&
                     (linkValue.Buffer[ 6] == L'e') &&
                     (linkValue.Buffer[ 7] == L'\\') &&
                     (linkValue.Buffer[ 8] == L'L') &&
                     (linkValue.Buffer[ 9] == L'a') &&
                     (linkValue.Buffer[10] == L'n') &&
                     (linkValue.Buffer[14] == L'R') &&
                     (linkValue.Buffer[15] == L'e') &&
                     (linkValue.Buffer[16] == L'd') &&
                     (linkValue.Buffer[17] == L'i') &&
                     (linkValue.Buffer[18] == L'r') &&
                     (linkValue.Buffer[23] == L'r') &
                     (linkValue.Buffer[24] == L'\\') ) {

                    //
                    // Free all the buffers.
                    //

                    RtlFreeHeap(RtlProcessHeap(), 0, linkValueBuffer);
                    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                    RtlFreeHeap(RtlProcessHeap(), 0, FullPath);
                    SetLastError(ERROR_INVALID_NAME);
                    return FALSE;
                }
            }
            
            //
            // Free the link buffer.
            //

            RtlFreeHeap(RtlProcessHeap(), 0, linkValueBuffer);
        }
    }

    RtlFreeHeap(RtlProcessHeap(), 0, FullPath);

    //
    // Account the inheritance of the security descriptor.
    //

    if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
        ObjectAttributes.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        }

    //
    // Notice that FILE_OPEN_REPARSE_POINT inhibits the reparse behavior.
    // Thus, the hard link is established to the local entity, be it a reparse
    // point or not.
    //

    OpenFlags = FILE_FLAG_OPEN_REPARSE_POINT |
                FILE_SYNCHRONOUS_IO_NONALERT |
                FILE_OPEN_FOR_BACKUP_INTENT;

    Status = NtOpenFile(
                 &FileHandle,
                 (ACCESS_MASK)DELETE | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 OpenFlags
                 );

    if ( !NT_SUCCESS(Status) ) {
        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
        BaseSetLastNTError( Status );
        return FALSE;
        }

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpLinkName,
                            &NewFileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    FreeBuffer = NewFileName.Buffer;

    NewName = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), NewFileName.Length+sizeof(*NewName));

    if ( NewName != NULL ) {
        RtlMoveMemory(NewName->FileName, NewFileName.Buffer, NewFileName.Length);
        NewName->ReplaceIfExists = FALSE;
        NewName->RootDirectory = NULL;
        NewName->FileNameLength = NewFileName.Length;

        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

        Status = NtSetInformationFile(
                     FileHandle,
                     &IoStatusBlock,
                     NewName,
                     NewFileName.Length+sizeof(*NewName),
                     FileLinkInformation
                     );

        RtlFreeHeap(RtlProcessHeap(), 0, NewName);
        NtClose(FileHandle);
    } else {
        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
        NtClose(FileHandle);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError( Status );
        return FALSE;
    }

    return TRUE;
}
