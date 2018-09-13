/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    pathmisc.c

Abstract:

    Win32 misceleneous path functions

Author:

    Mark Lucovsky (markl) 16-Oct-1990

Revision History:

--*/

#include "basedll.h"
#include "apcompat.h"

BOOL
IsThisARootDirectory(
    HANDLE RootHandle,
    PUNICODE_STRING FileName OPTIONAL
    )
{
    PFILE_NAME_INFORMATION FileNameInfo;
    WCHAR Buffer[MAX_PATH+sizeof(FileNameInfo)];
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    BOOL rv;

    OBJECT_ATTRIBUTES Attributes;
    HANDLE LinkHandle;
    WCHAR LinkValueBuffer[2*MAX_PATH];
    UNICODE_STRING LinkValue;
    ULONG ReturnedLength;

    rv = FALSE;

    FileNameInfo = (PFILE_NAME_INFORMATION)Buffer;
    Status = NtQueryInformationFile(
                RootHandle,
                &IoStatusBlock,
                FileNameInfo,
                sizeof(Buffer),
                FileNameInformation
                );
    if ( NT_SUCCESS(Status) ) {
            if ( FileNameInfo->FileName[(FileNameInfo->FileNameLength>>1)-1] == (WCHAR)'\\' ) {
            rv = TRUE;
            }
        }

    if ( !rv ) {

        //
        // See if this is a dos substed drive (or) redirected net drive
        //

        if ( ARGUMENT_PRESENT(FileName) ) {

            FileName->Length = FileName->Length - sizeof((WCHAR)'\\');

            InitializeObjectAttributes( &Attributes,
                                        FileName,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL
                                      );
            Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                               SYMBOLIC_LINK_QUERY,
                                               &Attributes
                                             );
            FileName->Length = FileName->Length + sizeof((WCHAR)'\\');
            if ( NT_SUCCESS(Status) ) {

                //
                // Now query the link and see if there is a redirection
                //

                LinkValue.Buffer = LinkValueBuffer;
                LinkValue.Length = 0;
                LinkValue.MaximumLength = (USHORT)(sizeof(LinkValueBuffer));
                ReturnedLength = 0;
                Status = NtQuerySymbolicLinkObject( LinkHandle,
                                                    &LinkValue,
                                                    &ReturnedLength
                                                  );
                NtClose( LinkHandle );

                if ( NT_SUCCESS(Status) ) {
                    rv = TRUE;
                    }
                }

            }
        }
    return rv;
}


UINT
APIENTRY
GetSystemDirectoryA(
    LPSTR lpBuffer,
    UINT uSize
    )

/*++

Routine Description:

    ANSI thunk to GetSystemDirectoryW

--*/

{
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    ULONG cbAnsiString;
    PUNICODE_STRING WindowsSystemDirectory = &BaseWindowsSystemDirectory;

#ifdef WX86
    if (NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll) {
        NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = FALSE;
        WindowsSystemDirectory = &BaseWindowsSys32x86Directory;
        }
#endif



    // BaseWindowsSystemDirectory.Length contains the byte
    // count of unicode string.
    // Original code does "UnicodeLength / sizeof(WCHAR)" to
    // get the size of corresponding ansi string.
    // This is correct in SBCS environment. However in DBCS
    // environment, it's definitely WRONG.

    Status = RtlUnicodeToMultiByteSize(&cbAnsiString,
                                       WindowsSystemDirectory->Buffer,
                                       WindowsSystemDirectory->MaximumLength
                                       );
    if ( !NT_SUCCESS(Status) ) {
        return 0;
        }

    if ( (USHORT)uSize < (USHORT)cbAnsiString ) {
        return cbAnsiString;
        }

    AnsiString.MaximumLength = (USHORT)(uSize);
    AnsiString.Buffer = lpBuffer;

    Status = BasepUnicodeStringTo8BitString(
                &AnsiString,
                WindowsSystemDirectory,
                FALSE
                );
    if ( !NT_SUCCESS(Status) ) {
        return 0;
        }
    return AnsiString.Length;
}


UINT
APIENTRY
GetSystemDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )

/*++

Routine Description:

    This function obtains the pathname of the Windows system
    subdirectory.  The system subdirectory contains such files as
    Windows libraries, drivers, and font files.

    The pathname retrieved by this function does not end with a
    backslash unless the system directory is the root directory.  For
    example, if the system directory is named WINDOWS\SYSTEM on drive
    C:, the pathname of the system subdirectory retrieved by this
    function is C:\WINDOWS\SYSTEM.

Arguments:

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the pathname.

    uSize - Specifies the maximum size (in bytes) of the buffer.  This
        value should be set to at least MAX_PATH to allow sufficient room in
        the buffer for the pathname.

Return Value:

    The return value is the length of the string copied to lpBuffer, not
    including the terminating null character.  If the return value is
    greater than uSize, the return value is the size of the buffer
    required to hold the pathname.  The return value is zero if the
    function failed.

--*/

{
    PUNICODE_STRING WindowsSystemDirectory = &BaseWindowsSystemDirectory;

#ifdef WX86
    if (NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll) {
        NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = FALSE;
        WindowsSystemDirectory = &BaseWindowsSys32x86Directory;
        }
#endif

    if ( uSize*2 < WindowsSystemDirectory->MaximumLength ) {
        return WindowsSystemDirectory->MaximumLength/2;
        }
    RtlMoveMemory(
        lpBuffer,
        WindowsSystemDirectory->Buffer,
        WindowsSystemDirectory->Length
        );
    lpBuffer[(WindowsSystemDirectory->Length>>1)] = UNICODE_NULL;
    return WindowsSystemDirectory->Length/2;
}

UINT
APIENTRY
GetSystemWindowsDirectoryA(
    LPSTR lpBuffer,
    UINT uSize
    )

/*++

Routine Description:

    ANSI thunk to GetSystemWindowsDirectoryW

--*/

{
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    ULONG cbAnsiString;

    // BaseWindowsDirectory.Length contains the byte
    // count of unicode string.
    // Original code does "UnicodeLength / sizeof(WCHAR)" to
    // get the size of corresponding ansi string.
    // This is correct in SBCS environment. However in DBCS
    // environment, it's definitely WRONG.

    Status = RtlUnicodeToMultiByteSize( &cbAnsiString,
                               BaseWindowsDirectory.Buffer,
                               BaseWindowsDirectory.MaximumLength);
    if ( !NT_SUCCESS(Status) ) {
        return 0;
        }

    if ( (USHORT)uSize < (USHORT)cbAnsiString ) {
        return cbAnsiString;
        }

    AnsiString.MaximumLength = (USHORT)(uSize);
    AnsiString.Buffer = lpBuffer;

    Status = BasepUnicodeStringTo8BitString(
                &AnsiString,
                &BaseWindowsDirectory,
                FALSE
                );
    if ( !NT_SUCCESS(Status) ) {
        return 0;
        }
    return AnsiString.Length;
}

UINT
APIENTRY
GetSystemWindowsDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )

/*++

Routine Description:

    This function obtains the pathname of the system Windows directory.  

Arguments:

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the pathname.

    uSize - Specifies the maximum size (in bytes) of the buffer.  This
        value should be set to at least MAX_PATH to allow sufficient room in
        the buffer for the pathname.

Return Value:

    The return value is the length of the string copied to lpBuffer, not
    including the terminating null character.  If the return value is
    greater than uSize, the return value is the size of the buffer
    required to hold the pathname.  The return value is zero if the
    function failed.

--*/

{

    if ( uSize*2 < BaseWindowsDirectory.MaximumLength ) {
        return BaseWindowsDirectory.MaximumLength/2;
        }
    RtlMoveMemory(
        lpBuffer,
        BaseWindowsDirectory.Buffer,
        BaseWindowsDirectory.Length
        );
    lpBuffer[(BaseWindowsDirectory.Length>>1)] = UNICODE_NULL;
    return BaseWindowsDirectory.Length/2;
}



UINT
APIENTRY
GetWindowsDirectoryA(
    LPSTR lpBuffer,
    UINT uSize
    )

/*++

Routine Description:


--*/

{

    if (gpTermsrvGetWindowsDirectoryA) {

        // 
        //  If Terminal Server get the Per User Windows Directory
        //

        UINT retval;
        if (retval = gpTermsrvGetWindowsDirectoryA(lpBuffer, uSize)) {
            return retval;
        }
    }


    return GetSystemWindowsDirectoryA(lpBuffer,uSize);
}

UINT
APIENTRY
GetWindowsDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )

/*++

Routine Description:

    This function obtains the pathname of the Windows directory.  The
    Windows directory contains such files as Windows applications,
    initialization files, and help files.
    425
    The pathname retrieved by this function does not end with a
    backslash unless the Windows directory is the root directory.  For
    example, if the Windows directory is named WINDOWS on drive C:, the
    pathname of the Windows directory retrieved by this function is
    C:\WINDOWS If Windows was installed in the root directory of drive
    C:, the pathname retrieved by this function is C:\

Arguments:

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the pathname.

    uSize - Specifies the maximum size (in bytes) of the buffer.  This
        value should be set to at least MAX_PATH to allow sufficient room in
        the buffer for the pathname.

Return Value:

    The return value is the length of the string copied to lpBuffer, not
    including the terminating null character.  If the return value is
    greater than uSize, the return value is the size of the buffer
    required to hold the pathname.  The return value is zero if the
    function failed.

--*/

{
    if (gpTermsrvGetWindowsDirectoryW) {
        // 
        //  If Terminal Server get the Per User Windows Directory
        //

        UINT retval;
        if (retval = gpTermsrvGetWindowsDirectoryW(lpBuffer, uSize)) {
            return retval;
        }
    }

    return GetSystemWindowsDirectoryW(lpBuffer,uSize);

}



UINT
APIENTRY
GetDriveTypeA(
    LPCSTR lpRootPathName
    )

/*++

Routine Description:

    ANSI thunk to GetDriveTypeW

--*/

{
    PUNICODE_STRING Unicode;
    LPCWSTR lpRootPathName_U;

    if (ARGUMENT_PRESENT(lpRootPathName)) {
        Unicode = Basep8BitStringToStaticUnicodeString( lpRootPathName );
        if (Unicode == NULL) {
            return 1;
        }

        lpRootPathName_U = (LPCWSTR)Unicode->Buffer;
        }
    else {
        lpRootPathName_U = NULL;
        }

    return GetDriveTypeW(lpRootPathName_U);
}

UINT
APIENTRY
GetDriveTypeW(
    LPCWSTR lpRootPathName
    )

/*++

Routine Description:

    This function determines whether a disk drive is removeable, fixed,
    remote, CD ROM, or a RAM disk.

    The return value is zero if the function cannot determine the drive
    type, or 1 if the specified root directory does not exist.

Arguments:

    lpRootPathName - An optional parameter, that if specified, supplies
        the root directory of the disk whose drive type is to be
        determined.  If this parameter is not specified, then the root
        of the current directory is used.

Return Value:

    The return value specifies the type of drive.  It can be one of the
    following values:

    DRIVE_UNKNOWN - The drive type can not be determined.

    DRIVE_NO_ROOT_DIR - The root directory does not exist.

    DRIVE_REMOVABLE - Disk can be removed from the drive.

    DRIVE_FIXED - Disk cannot be removed from the drive.

    DRIVE_REMOTE - Drive is a remote (network) drive.

    DRIVE_CDROM - Drive is a CD rom drive.

    DRIVE_RAMDISK - Drive is a RAM disk.

--*/

{
    WCHAR wch;
    ULONG n, DriveNumber;
    WCHAR DefaultPath[MAX_PATH];
    PWSTR RootPathName;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    PVOID FreeBuffer;
    DWORD ReturnValue;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;

    if (!ARGUMENT_PRESENT(lpRootPathName)) {
        n = RtlGetCurrentDirectory_U(sizeof(DefaultPath), DefaultPath);
        RootPathName = DefaultPath;
        if (n > (3 * sizeof(WCHAR))) {
            RootPathName[3]=UNICODE_NULL;
            }
        }
    else
    if (lpRootPathName == (PWSTR)IntToPtr(0xFFFFFFFF)) {
        //
        // Hack to be compatible with undocumented feature of old
        // implementation.
        //

        return 0;
        }
    else {
        //
        // If input string is just C: then convert to C:\ so it does
        // not default to current directory which may or may not be
        // at the root.
        //
        RootPathName = (PWSTR)lpRootPathName;
        if (wcslen( RootPathName ) == 2) {
            wch = RtlUpcaseUnicodeChar( *RootPathName );
            if (wch >= (WCHAR)'A' &&
                wch <= (WCHAR)'Z' &&
                RootPathName[1] == (WCHAR)':'
               ) {
                RootPathName = wcscpy(DefaultPath, lpRootPathName);
                RootPathName[2] = (WCHAR)'\\';
                RootPathName[3] = UNICODE_NULL;
                }
            }
        }

    //
    // If input string is of the form C:\ then look in the drive letter
    // cache maintained by the kernel to see if the drive type is already
    // known.
    //
    wch = RtlUpcaseUnicodeChar( *RootPathName );
    if (wch >= (WCHAR)'A' &&
        wch <= (WCHAR)'Z' &&
        RootPathName[1]==(WCHAR)':' &&
        RootPathName[2]==(WCHAR)'\\' &&
        RootPathName[3]==UNICODE_NULL
       ) {
        Status = NtQueryInformationProcess( NtCurrentProcess(),
                                            ProcessDeviceMap,
                                            &ProcessDeviceMapInfo.Query,
                                            sizeof( ProcessDeviceMapInfo.Query ),
                                            NULL
                                          );
        if (!NT_SUCCESS( Status )) {
            RtlZeroMemory( &ProcessDeviceMapInfo, sizeof( ProcessDeviceMapInfo ) );
            }

        DriveNumber = wch - (WCHAR)'A';
        if (ProcessDeviceMapInfo.Query.DriveMap & (1 << DriveNumber)) {
            switch ( ProcessDeviceMapInfo.Query.DriveType[ DriveNumber ] ) {
                case DOSDEVICE_DRIVE_UNKNOWN:
                    return DRIVE_UNKNOWN;

                case DOSDEVICE_DRIVE_REMOVABLE:
                    return DRIVE_REMOVABLE;

                case DOSDEVICE_DRIVE_FIXED:
                    return DRIVE_FIXED;

                case DOSDEVICE_DRIVE_REMOTE:
                    return DRIVE_REMOTE;

                case DOSDEVICE_DRIVE_CDROM:
                    return DRIVE_CDROM;

                case DOSDEVICE_DRIVE_RAMDISK:
                    return DRIVE_RAMDISK;
                }
            }
        }


    //
    // Either not C:\ or kernel does not know the drive type, so try to
    // calculate the drive type by opening the root directory and doing
    // a query volume information.
    //


    //
    // If curdir is a UNC connection, and default path is used,
    // the RtlGetCurrentDirectory logic is wrong, so throw it away.
    //

    if (!ARGUMENT_PRESENT(lpRootPathName)) {
        RootPathName = L"\\";
        }

    TranslationStatus = RtlDosPathNameToNtPathName_U( RootPathName,
                                                      &FileName,
                                                      NULL,
                                                      NULL
                                                    );
    if (!TranslationStatus) {
        return DRIVE_NO_ROOT_DIR;
        }
    FreeBuffer = FileName.Buffer;

    //
    // Check to make sure a root was specified
    //

    if (FileName.Buffer[(FileName.Length >> 1)-1] != '\\') {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        return DRIVE_NO_ROOT_DIR;
        }

    FileName.Length -= 2;
    InitializeObjectAttributes( &Obja,
                                &FileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    //
    // Open the file
    //
    Status = NtOpenFile( &Handle,
                         (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                       );

    //
    //
    // substd drives are really directories, so if we are dealing with one
    // of them, bypass this
    //

    if ( Status == STATUS_FILE_IS_A_DIRECTORY ) {
        Status = NtOpenFile(
                    &Handle,
                    (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_SYNCHRONOUS_IO_NONALERT
                    );
        }
    else {

        //
        // check for substed drives another way just in case
        //

        FileName.Length = FileName.Length + sizeof((WCHAR)'\\');
        if (!IsThisARootDirectory(NULL,&FileName) ) {
            FileName.Length = FileName.Length - sizeof((WCHAR)'\\');
            if (NT_SUCCESS(Status)) {
                NtClose(Handle);
                }
            Status = NtOpenFile(
                        &Handle,
                        (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &Obja,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT
                        );
            }
        }
    RtlFreeHeap( RtlProcessHeap(), 0, FreeBuffer );
    if (!NT_SUCCESS( Status )) {
        return DRIVE_NO_ROOT_DIR;
        }

    //
    // Determine if this is a network or disk file system. If it
    // is a disk file system determine if this is removable or not
    //

    Status = NtQueryVolumeInformationFile( Handle,
                                           &IoStatusBlock,
                                           &DeviceInfo,
                                           sizeof(DeviceInfo),
                                           FileFsDeviceInformation
                                         );
    if (!NT_SUCCESS( Status )) {
        ReturnValue = DRIVE_UNKNOWN;
        }
    else
    if (DeviceInfo.Characteristics & FILE_REMOTE_DEVICE) {
        ReturnValue = DRIVE_REMOTE;
        }
    else {
        switch (DeviceInfo.DeviceType) {

            case FILE_DEVICE_NETWORK:
            case FILE_DEVICE_NETWORK_FILE_SYSTEM:
                ReturnValue = DRIVE_REMOTE;
                break;

            case FILE_DEVICE_CD_ROM:
            case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
                ReturnValue = DRIVE_CDROM;
                break;

            case FILE_DEVICE_VIRTUAL_DISK:
                ReturnValue = DRIVE_RAMDISK;
                break;

            case FILE_DEVICE_DISK:
            case FILE_DEVICE_DISK_FILE_SYSTEM:

                if ( DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA ) {
                    ReturnValue = DRIVE_REMOVABLE;
                    }
                else {
                    ReturnValue = DRIVE_FIXED;
                    }
                break;

            default:
                ReturnValue = DRIVE_UNKNOWN;
                break;
            }
        }

    NtClose( Handle );
    return ReturnValue;
}

DWORD
APIENTRY
SearchPathA(
    LPCSTR lpPath,
    LPCSTR lpFileName,
    LPCSTR lpExtension,
    DWORD nBufferLength,
    LPSTR lpBuffer,
    LPSTR *lpFilePart
    )

/*++

Routine Description:

    ANSI thunk to SearchPathW

--*/

{

    UNICODE_STRING xlpPath;
    PUNICODE_STRING Unicode;
    UNICODE_STRING xlpExtension;
    PWSTR xlpBuffer;
    DWORD ReturnValue;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    PWSTR FilePart;
    PWSTR *FilePartPtr;

    if ( ARGUMENT_PRESENT(lpFilePart) ) {
        FilePartPtr = &FilePart;
        }
    else {
        FilePartPtr = NULL;
        }

    Unicode = Basep8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return 0;
    }

    if ( ARGUMENT_PRESENT(lpExtension) ) {

        if (!Basep8BitStringToDynamicUnicodeString( &xlpExtension, lpExtension )) {
            return 0;
        }

    } else {
        xlpExtension.Buffer = NULL;
    }

    if ( ARGUMENT_PRESENT(lpPath) ) {

        if (!Basep8BitStringToDynamicUnicodeString( &xlpPath, lpPath )) {
            if ( ARGUMENT_PRESENT(lpExtension) ) {
                RtlFreeUnicodeString(&xlpExtension);
            }
            return 0;
        }
    } else {
        xlpPath.Buffer = NULL;
    }

    xlpBuffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), nBufferLength<<1);
    if ( !xlpBuffer ) {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        ReturnValue = 0;
        goto bail0;
        }
    ReturnValue = SearchPathW(
                    xlpPath.Buffer,
                    Unicode->Buffer,
                    xlpExtension.Buffer,
                    nBufferLength,
                    xlpBuffer,
                    FilePartPtr
                    );
    //
    // === DBCS modification note [takaok] ===
    //
    // SearchPathW retruns:
    //
    //   buffer size needed(including null terminator) if buffer size is too small.
    //   number of characters( not including null terminator) if buffer size is enougth
    //
    // This means SearchPathW never returns value which is equal to nBufferLength.
    //

    if ( ReturnValue > nBufferLength ) {
        //
        // To know the ansi buffer size needed, we should get all of
        // unicode string.
        //
        RtlFreeHeap(RtlProcessHeap(), 0,xlpBuffer);
        xlpBuffer = RtlAllocateHeap(RtlProcessHeap(),
                                    MAKE_TAG( TMP_TAG ),
                                    ReturnValue * sizeof(WCHAR));
        if ( !xlpBuffer ) {
            BaseSetLastNTError(STATUS_NO_MEMORY);
            goto bail0;
        }
        ReturnValue = SearchPathW(
                        xlpPath.Buffer,
                        Unicode->Buffer,
                        xlpExtension.Buffer,
                        ReturnValue,
                        xlpBuffer,
                        FilePartPtr
                        );
        if ( ReturnValue > 0 ) {
            //
            // We called SearchPathW with the enough size of buffer.
            // So, ReturnValue is the size of the path not including the
            // terminating null character.
            //
            Status = RtlUnicodeToMultiByteSize( &ReturnValue,
                                       xlpBuffer,
                                       ReturnValue * sizeof(WCHAR));
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                ReturnValue = 0;
            }
            else {
                ReturnValue += 1;
            }
        }
    } else if ( ReturnValue > 0 ) {

        INT AnsiByteCount;

        //
        // We have unicode string. We need to compute the ansi byte count
        // of the string.
        //
        // ReturnValue   : unicode character count not including null terminator
        // AnsiByteCount : ansi byte count not including null terminator
        //
        Status = RtlUnicodeToMultiByteSize( &AnsiByteCount,
                                   xlpBuffer,
                                   ReturnValue * sizeof(WCHAR) );

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            ReturnValue = 0;
            }
        else {
            if ( AnsiByteCount < (INT)nBufferLength ) {
            //
            // The string (including null terminator) fits to the buffer
            //
                Status = RtlUnicodeToMultiByteN ( lpBuffer,
                                                  nBufferLength - 1,
                                                  &AnsiByteCount,
                                                  xlpBuffer,
                                                  ReturnValue * sizeof(WCHAR)
                                                );
                if ( !NT_SUCCESS(Status) ) {
                    BaseSetLastNTError(Status);
                    ReturnValue = 0;
                }
                else {

                    lpBuffer[ AnsiByteCount ] = '\0';

                    //
                    // The return value is the byte count copied to the buffer
                    // not including the terminating null character.
                    //
                    ReturnValue = AnsiByteCount;


                    if ( ARGUMENT_PRESENT(lpFilePart) ) {
                        if ( FilePart == NULL ) {
                            *lpFilePart = NULL;
                        } else {

                            INT PrefixLength;

                            PrefixLength = (INT)(FilePart - xlpBuffer);
                            Status = RtlUnicodeToMultiByteSize( &PrefixLength,
                                                       xlpBuffer,
                                                       PrefixLength * sizeof(WCHAR));
                            if ( !NT_SUCCESS(Status) ) {
                                BaseSetLastNTError(Status);
                                ReturnValue = 0;
                            }
                            else {
                                *lpFilePart = lpBuffer + PrefixLength;
                            }
                        }
                    }
                }

            } else {
            //
            // We should return the size of the buffer required to
            // hold the path. The size should include the
            // terminating null character.
            //
                ReturnValue = AnsiByteCount + 1;

            }
        }
    }

    RtlFreeHeap(RtlProcessHeap(), 0,xlpBuffer);
bail0:
    if ( ARGUMENT_PRESENT(lpExtension) ) {
        RtlFreeUnicodeString(&xlpExtension);
        }

    if ( ARGUMENT_PRESENT(lpPath) ) {
        RtlFreeUnicodeString(&xlpPath);
        }
    return ReturnValue;
}



#ifdef WX86

ULONG
GetFullPathNameWithWx86Override(
    PCWSTR lpFileName,
    ULONG nBufferLength,
    PWSTR lpBuffer,
    PWSTR *lpFilePart
    )
{
    UNICODE_STRING FullPathName, PathUnicode, Wx86PathName;
    PUNICODE_STRING FoundFileName;
    RTL_PATH_TYPE PathType;
    PWSTR FilePart;
    ULONG Length, LengthPath;
    ULONG  PathNameLength;

    FullPathName.Buffer = NULL;
    Wx86PathName.Buffer = NULL;

    if (lpFilePart) {
        *lpFilePart = NULL;
        }

    FullPathName.MaximumLength = (USHORT)(MAX_PATH * sizeof(WCHAR)) + sizeof(WCHAR);
    FullPathName.Length = 0;
    FullPathName.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                          MAKE_TAG( TMP_TAG ),
                                          FullPathName.MaximumLength
                                          );
    if (!FullPathName.Buffer) {
        PathNameLength = 0;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto WDOExitCleanup;
        }

    FoundFileName = &FullPathName;
    PathNameLength = RtlGetFullPathName_U(lpFileName,
                                          FullPathName.MaximumLength,
                                          FullPathName.Buffer,
                                          &FilePart
                                          );

    if (!PathNameLength || PathNameLength >= FullPathName.MaximumLength) {
        PathNameLength = 0;
        goto WDOExitCleanup;
        }

    FullPathName.Length = (USHORT)PathNameLength;


    PathUnicode = FullPathName;
    PathUnicode.Length = (USHORT)((ULONG_PTR)FilePart -
                                  (ULONG_PTR)FullPathName.Buffer);

    PathUnicode.Length -= sizeof(WCHAR);
    if (!RtlEqualUnicodeString(&PathUnicode, &BaseWindowsSystemDirectory, TRUE)) {
        goto WDOExitCleanup;
        }


    Wx86PathName.MaximumLength = BaseWindowsSys32x86Directory.Length +
                                 FullPathName.Length - PathUnicode.Length +
                                 2*sizeof(WCHAR);
    Wx86PathName.Length = 0;
    Wx86PathName.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                          MAKE_TAG( TMP_TAG ),
                                          Wx86PathName.MaximumLength
                                          );

    if (!Wx86PathName.Buffer) {
        goto WDOExitCleanup;
        }

    RtlCopyUnicodeString(&Wx86PathName, &BaseWindowsSys32x86Directory);
    Length = Wx86PathName.Length + sizeof(WCHAR);
    RtlAppendUnicodeToString (&Wx86PathName, FilePart - 1);
    if (RtlDoesFileExists_U(Wx86PathName.Buffer)) {
        FoundFileName = &Wx86PathName;
        FilePart = Wx86PathName.Buffer + Length/sizeof(WCHAR);
        }



WDOExitCleanup:

    if (PathNameLength) {
        if (FoundFileName->Length >= nBufferLength) {
            PathNameLength = FoundFileName->Length + sizeof(WCHAR);
            }
        else {
            RtlMoveMemory(lpBuffer,
                          FoundFileName->Buffer,
                          FoundFileName->Length + sizeof(WCHAR)
                          );

            PathNameLength = FoundFileName->Length;
            Length = (ULONG)(FilePart - FoundFileName->Buffer);

            if (lpFilePart) {
                *lpFilePart = lpBuffer + Length/sizeof(WCHAR);
                }
            }
        }


    if (FullPathName.Buffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, FullPathName.Buffer);
        }

    if (Wx86PathName.Buffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, Wx86PathName.Buffer);
        }

    return PathNameLength;

}
#endif










DWORD
APIENTRY
SearchPathW(
    LPCWSTR lpPath,
    LPCWSTR lpFileName,
    LPCWSTR lpExtension,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    )

/*++

Routine Description:

    This function is used to search for a file specifying a search path
    and a filename.  It returns with a fully qualified pathname of the
    found file.

    This function is used to locate a file using the specified path.  If
    the file is found, its fully qualified pathname is returned.  In
    addition to this, it calculates the address of the file name portion
    of the fully qualified pathname.

Arguments:

    lpPath - An optional parameter, that if specified, supplies the
        search path to be used when locating the file.  If this
        parameter is not specified, the default windows search path is
        used.  The default path is:

          - The current directory

          - The windows directory

          - The windows system directory

          - The directories listed in the path environment variable

    lpFileName - Supplies the file name of the file to search for.

    lpExtension - An optional parameter, that if specified, supplies an
        extension to be added to the filename when doing the search.
        The extension is only added if the specified filename does not
        end with an extension.

    nBufferLength - Supplies the length in characters of the buffer that
        is to receive the fully qualified path.

    lpBuffer - Returns the fully qualified pathname corresponding to the
        file that was found.

    lpFilePart - Returns the address of the last component of the fully
        qualified pathname.

Return Value:

    The return value is the length of the string copied to lpBuffer, not
    including the terminating null character.  If the return value is
    greater than nBufferLength, the return value is the size of the buffer
    required to hold the pathname.  The return value is zero if the
    function failed.


--*/

{

    LPWSTR ComputedFileName;
    ULONG ExtensionLength;
    ULONG PathLength;
    ULONG FileLength;
    LPCWSTR p;
    LPWSTR p1;
    LPWSTR AllocatedPath;
    ULONG ComputedLength;
    UNICODE_STRING Scratch;
    UNICODE_STRING AllocatedPathString;
    RTL_PATH_TYPE PathType;
    WCHAR wch;

#ifdef WX86
    BOOL UseKnownWx86Dll;

    UseKnownWx86Dll = NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll;
    NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = FALSE;
#endif


    //
    // if the file name is not a relative name, then
    // check to see if the file exists.
    //

    nBufferLength *= 2;
    PathType = RtlDetermineDosPathNameType_U(lpFileName);
    if ( PathType == RtlPathTypeRelative ) {

        //
        // for .\ and ..\ names don't search path
        //

        if ( lpFileName[0] == (WCHAR)'.') {
            if ( lpFileName[1] == (WCHAR)'\\' || lpFileName[1] == (WCHAR)'/') {
                PathType = RtlPathTypeUnknown;
                }
            else {
                if ( lpFileName[1] == (WCHAR)'.'  &&
                    (lpFileName[2] == (WCHAR)'\\' || lpFileName[2] == (WCHAR)'/') ) {
                    PathType = RtlPathTypeUnknown;
                    }
                }
            }
        }

    if ( PathType != RtlPathTypeRelative ) {
        if (RtlDoesFileExists_U(lpFileName) ) {
            ExtensionLength = RtlGetFullPathName_U(
                                lpFileName,
                                nBufferLength,
                                lpBuffer,
                                lpFilePart
                                );
            return ExtensionLength/2;
            }
        else {

            //
            // if the name does not exist, then see if adding the extension
            // helps any
            //

            if ( ARGUMENT_PRESENT(lpExtension) ) {
                RtlInitUnicodeString(&Scratch,lpExtension);
                ExtensionLength = Scratch.Length;
                RtlInitUnicodeString(&Scratch,lpFileName);
                AllocatedPathString.Length = (USHORT)ExtensionLength + Scratch.Length;
                AllocatedPathString.MaximumLength = AllocatedPathString.Length + (USHORT)sizeof(UNICODE_NULL);
                AllocatedPathString.Buffer = RtlAllocateHeap(
                                                RtlProcessHeap(),
                                                MAKE_TAG( TMP_TAG ),
                                                AllocatedPathString.MaximumLength
                                                );
                if ( !AllocatedPathString.Buffer ) {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return 0;
                    }
                RtlCopyMemory(AllocatedPathString.Buffer,lpFileName,Scratch.Length);
                RtlCopyMemory(AllocatedPathString.Buffer+(Scratch.Length>>1),lpExtension,ExtensionLength+sizeof(UNICODE_NULL));
                if (RtlDoesFileExists_U(AllocatedPathString.Buffer) ) {
                    ExtensionLength = RtlGetFullPathName_U(
                                        AllocatedPathString.Buffer,
                                        nBufferLength,
                                        lpBuffer,
                                        lpFilePart
                                        );
                    ExtensionLength /= 2;
                    }
                else {
                    SetLastError(ERROR_FILE_NOT_FOUND);
                    ExtensionLength = 0;
                    }
                RtlFreeHeap(RtlProcessHeap(), 0, AllocatedPathString.Buffer);
                return ExtensionLength;
                }
            else {
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
                }
            }
        }

    //
    // Determine if the file name contains an extension
    //

    ExtensionLength = 1;
    p = lpFileName;
    while (*p) {
        if ( *p == (WCHAR)'.' ) {
            ExtensionLength = 0;
            break;
            }
        p++;
        }

    //
    // If no extension was found, then determine the extension length
    // that should be used to search for the file
    //

    if ( ExtensionLength ) {
        if ( ARGUMENT_PRESENT(lpExtension) ) {
            RtlInitUnicodeString(&Scratch,lpExtension);
            ExtensionLength = Scratch.Length;
            }
        else {
            ExtensionLength = 0;
            }
        }
    else {
        ExtensionLength = 0;
        }

    //
    // If the lpPath parameter is not specified, then use the
    // default windows search.
    //

    if ( !ARGUMENT_PRESENT(lpPath) ) {
        AllocatedPath = BaseComputeProcessDllPath(NULL, GetEnvironmentStringsW());
        if ( !AllocatedPath ) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
            }
        RtlInitUnicodeString(&Scratch,AllocatedPath);
        PathLength = Scratch.Length;
        lpPath = AllocatedPath;
        }
    else {
        RtlInitUnicodeString(&Scratch,lpPath);
        PathLength = Scratch.Length;
        AllocatedPath = NULL;
        }

    //
    // Compute the file name length and the path length;
    //

    RtlInitUnicodeString(&Scratch,lpFileName);
    FileLength = Scratch.Length;

    //
    // trim trailing spaces, and then check for a real filelength
    // if length is 0 (NULL, "", or " ") passed in then abort the
    // search
    //

    if ( FileLength ) {
        wch = Scratch.Buffer[(FileLength>>1) - 1];
        while ( FileLength && wch == (WCHAR)' ' ) {
            FileLength -= sizeof(WCHAR);
            wch = Scratch.Buffer[(FileLength>>1) - 1];
            }
        if ( !FileLength ) {
            if ( AllocatedPath ) {
                RtlFreeHeap(RtlProcessHeap(), 0, AllocatedPath);
                }
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
            }
        else {
            FileLength = Scratch.Length;
            }
        }
    else {
        if ( AllocatedPath ) {
            RtlFreeHeap(RtlProcessHeap(), 0, AllocatedPath);
            }
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
        }

    ComputedLength = PathLength + FileLength + ExtensionLength + 3*sizeof(UNICODE_NULL);
    ComputedFileName = RtlAllocateHeap(
                            RtlProcessHeap(), MAKE_TAG( TMP_TAG ),
                            ComputedLength
                            );
    if ( !ComputedFileName ) {
        if ( AllocatedPath ) {
            RtlFreeHeap(RtlProcessHeap(), 0, AllocatedPath);
            }
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return 0;
        }

    //
    // find ; 's in path and copy path component to computed file name
    //

    do {
        p1 = ComputedFileName;
        while (*lpPath) {
            if (*lpPath == (WCHAR)';') {
                lpPath++;
                break;
                }

            *p1++ = *lpPath++;
            }

        if (p1 != ComputedFileName &&
            p1 [ -1 ] != (WCHAR)'\\' ) {
            *p1++ = (WCHAR)'\\';
            }

        if (*lpPath == UNICODE_NULL) {
            lpPath = NULL;
            }
        RtlMoveMemory(p1,lpFileName,FileLength);
        if ( ExtensionLength ) {
            RtlMoveMemory((PUCHAR)p1+FileLength,lpExtension,ExtensionLength+sizeof(UNICODE_NULL));
            }
        else {
            *(LPWSTR)(((PUCHAR)p1+FileLength)) = UNICODE_NULL;
            }
        if (RtlDoesFileExists_U(ComputedFileName) ) {

#ifdef WX86
            if (UseKnownWx86Dll) {
                ExtensionLength = GetFullPathNameWithWx86Override(
                                     ComputedFileName,
                                     nBufferLength,
                                     lpBuffer,
                                     lpFilePart
                                     );
                }
            else {
                ExtensionLength = RtlGetFullPathName_U(
                                    ComputedFileName,
                                    nBufferLength,
                                    lpBuffer,
                                    lpFilePart
                                    );

                }

#else
            ExtensionLength = RtlGetFullPathName_U(
                                ComputedFileName,
                                nBufferLength,
                                lpBuffer,
                                lpFilePart
                                );
#endif

            if ( AllocatedPath ) {
                RtlFreeHeap(RtlProcessHeap(), 0, AllocatedPath);
                }
            RtlFreeHeap(RtlProcessHeap(), 0, ComputedFileName);
            return ExtensionLength/2;
            }
        }
    while ( lpPath );

    if ( AllocatedPath ) {
        RtlFreeHeap(RtlProcessHeap(), 0, AllocatedPath);
        }
    RtlFreeHeap(RtlProcessHeap(), 0, ComputedFileName);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
}


DWORD
APIENTRY
GetTempPathA(
    DWORD nBufferLength,
    LPSTR lpBuffer
    )

/*++

Routine Description:

    ANSI thunk to GetTempPathW

--*/

{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    ULONG  cbAnsiString;

    UnicodeString.MaximumLength = (USHORT)((nBufferLength<<1)+sizeof(UNICODE_NULL));
    UnicodeString.Buffer = RtlAllocateHeap(
                                RtlProcessHeap(), MAKE_TAG( TMP_TAG ),
                                UnicodeString.MaximumLength
                                );
    if ( !UnicodeString.Buffer ) {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return 0;
        }
    UnicodeString.Length = (USHORT)GetTempPathW(
                                        (DWORD)(UnicodeString.MaximumLength-sizeof(UNICODE_NULL))/2,
                                        UnicodeString.Buffer
                                        )*2;
    if ( UnicodeString.Length > (USHORT)(UnicodeString.MaximumLength-sizeof(UNICODE_NULL)) ) {
        RtlFreeHeap(RtlProcessHeap(), 0,UnicodeString.Buffer);

        //
        // given buffer size is too small.
        // allocate enough size of buffer and try again
        //
        // we need to get entire unicode temporary path
        // otherwise we can't figure out the exact length
        // of corresponding ansi string (cbAnsiString).

        UnicodeString.Buffer = RtlAllocateHeap ( RtlProcessHeap(),
                                                 MAKE_TAG( TMP_TAG ),
                                                 UnicodeString.Length+ sizeof(UNICODE_NULL));
        if ( !UnicodeString.Buffer ) {
            BaseSetLastNTError(STATUS_NO_MEMORY);
            return 0;
            }

        UnicodeString.Length = (USHORT)GetTempPathW(
                                     (DWORD)(UnicodeString.MaximumLength-sizeof(UNICODE_NULL))/2,
                                     UnicodeString.Buffer) * 2;
        Status = RtlUnicodeToMultiByteSize( &cbAnsiString,
                                            UnicodeString.Buffer,
                                            UnicodeString.Length );
        if ( !NT_SUCCESS(Status) ) {
            RtlFreeHeap(RtlProcessHeap(), 0, UnicodeString.Buffer);
            BaseSetLastNTError(Status);
            return 0;
            }
        else if ( nBufferLength < cbAnsiString ) {
            RtlFreeHeap(RtlProcessHeap(), 0, UnicodeString.Buffer);
            return cbAnsiString;
            }
        }
    AnsiString.Buffer = lpBuffer;
    AnsiString.MaximumLength = (USHORT)(nBufferLength+1);
    Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    RtlFreeHeap(RtlProcessHeap(), 0,UnicodeString.Buffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return 0;
        }
    return AnsiString.Length;
}

DWORD
APIENTRY
GetTempPathW(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    )

/*++

Routine Description:

    This function is used to return the pathname of the directory that
    should be used to create temporary files.

Arguments:

    nBufferLength - Supplies the length in bytes of the buffer that is
        to receive the temporary file path.

    lpBuffer - Returns the pathname of the directory that should be used
        to create temporary files in.

Return Value:

    The return value is the length of the string copied to lpBuffer, not
    including the terminating null character.  If the return value is
    greater than nSize, the return value is the size of the buffer
    required to hold the pathname.  The return value is zero if the
    function failed.

--*/

{

    DWORD Length;
    BOOLEAN AddTrailingSlash;
    PUNICODE_STRING Unicode;
    UNICODE_STRING EnvironmentValue;
    NTSTATUS Status;
    LPWSTR Name;
    ULONG Position;

    
    //
    // Some apps don't work with the new long path for the temp directory
    //

    if ( NtCurrentPeb()->AppCompatInfo &&
        (((PAPP_COMPAT_INFO)(NtCurrentPeb()->AppCompatInfo))->CompatibilityFlags.QuadPart & KACF_GETTEMPPATH ) ) {

        #define OLD_TEMP_PATH       L"c:\\temp\\"
        #define OLD_TEMP_PATH_SIZE  (sizeof(OLD_TEMP_PATH) / sizeof(WCHAR))
        
        BOOL bRet;
        
        //
        // If there isn't enough space provided in the buffer return
        // the desired size.
        //

        if (nBufferLength < OLD_TEMP_PATH_SIZE) {
            return OLD_TEMP_PATH_SIZE;
        }
        
        wcscpy(lpBuffer, OLD_TEMP_PATH);

        //
        // Use the correct drive letter
        //

        lpBuffer[0] = BaseWindowsDirectory.Buffer[0];

        bRet = CreateDirectoryW(lpBuffer, NULL);

        if (!bRet) {
            
            if (GetLastError() != ERROR_ALREADY_EXISTS)
                return 0;
        }
        
        return OLD_TEMP_PATH_SIZE - 1;
    }
    
    nBufferLength *= 2;
    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    EnvironmentValue = *Unicode;

    AddTrailingSlash = FALSE;

    Status = RtlQueryEnvironmentVariable_U(NULL,&BaseTmpVariableName,&EnvironmentValue);
    if ( !NT_SUCCESS(Status) ) {
        Status = RtlQueryEnvironmentVariable_U(NULL,&BaseTempVariableName,&EnvironmentValue);
        if ( !NT_SUCCESS(Status) ) {
            Status = RtlQueryEnvironmentVariable_U(NULL,&BaseUserProfileVariableName,&EnvironmentValue);
            }
        }

    if ( NT_SUCCESS(Status) ) {
        Name = EnvironmentValue.Buffer;
        if ( Name[(EnvironmentValue.Length>>1)-1] != (WCHAR)'\\' ) {
            AddTrailingSlash = TRUE;
            }
        }
    else {
        Name = BaseWindowsDirectory.Buffer;
        if ( Name[(BaseWindowsDirectory.Length>>1)-1] != (WCHAR)'\\' ) {
            AddTrailingSlash = TRUE;
            }
        }

    Length = RtlGetFullPathName_U(
                Name,
                nBufferLength,
                lpBuffer,
                NULL
                );
    Position = Length>>1;

    //
    // Make sure there is room for a trailing back slash
    //

    if ( Length && Length < nBufferLength ) {
        if ( lpBuffer[Position-1] != '\\' ) {
            if ( Length+sizeof((WCHAR)'\\') < nBufferLength ) {
                lpBuffer[Position] = (WCHAR)'\\';
                lpBuffer[Position+1] = UNICODE_NULL;
                return (Length+sizeof((WCHAR)'\\'))/2;
                }
            else {
                return (Length+sizeof((WCHAR)'\\')+sizeof(UNICODE_NULL))/2;
                }
            }
        else {
            return Length/2;
            }
        }
    else {
        if ( AddTrailingSlash ) {
            Length += sizeof((WCHAR)'\\');
            }
        return Length/2;
        }
}

UINT
APIENTRY
GetTempFileNameA(
    LPCSTR lpPathName,
    LPCSTR lpPrefixString,
    UINT uUnique,
    LPSTR lpTempFileName
    )

/*++

Routine Description:

    ANSI thunk to GetTempFileNameW

--*/

{
    PUNICODE_STRING Unicode;
    UNICODE_STRING UnicodePrefix;
    NTSTATUS Status;
    UINT ReturnValue;
    UNICODE_STRING UnicodeResult;

    Unicode = Basep8BitStringToStaticUnicodeString( lpPathName );
    if (Unicode == NULL) {
        return 0;
    }

    if (!Basep8BitStringToDynamicUnicodeString( &UnicodePrefix, lpPrefixString )) {
        return 0;
    }

    UnicodeResult.MaximumLength = (USHORT)((MAX_PATH<<1)+sizeof(UNICODE_NULL));
    UnicodeResult.Buffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), UnicodeResult.MaximumLength);
    if ( !UnicodeResult.Buffer ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        RtlFreeUnicodeString(&UnicodePrefix);
        return 0;
        }

    ReturnValue = GetTempFileNameW(
                    Unicode->Buffer,
                    UnicodePrefix.Buffer,
                    uUnique,
                    UnicodeResult.Buffer
                    );
    if ( ReturnValue ) {
        ANSI_STRING AnsiString;

        RtlInitUnicodeString(&UnicodeResult,UnicodeResult.Buffer);
        AnsiString.Buffer = lpTempFileName;
        AnsiString.MaximumLength = MAX_PATH+1;
        Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeResult,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            ReturnValue = 0;
            }
        }
    RtlFreeUnicodeString(&UnicodePrefix);
    RtlFreeHeap(RtlProcessHeap(), 0,UnicodeResult.Buffer);

    return ReturnValue;
}

UINT
APIENTRY
GetTempFileNameW(
    LPCWSTR lpPathName,
    LPCWSTR lpPrefixString,
    UINT uUnique,
    LPWSTR lpTempFileName
    )

/*++

Routine Description:

    This function creates a temporary filename of the following form:

        drive:\path\prefixuuuu.tmp

    In this syntax line, drive:\path\ is the path specified by the
    lpPathName parameter; prefix is all the letters (up to the first
    three) of the string pointed to by the lpPrefixString parameter; and
    uuuu is the hexadecimal value of the number specified by the
    uUnique parameter.

    To avoid problems resulting from converting OEM character an string
    to an ANSI string, an application should call the CreateFile
    function to create the temporary file.

    If the uUnique parameter is zero, GetTempFileName attempts to form a
    unique number based on the current system time.  If a file with the
    resulting filename exists, the number is increased by one and the
    test for existence is repeated.  This continues until a unique
    filename is found; GetTempFileName then creates a file by that name
    and closes it.  No attempt is made to create and open the file when
    uUnique is nonzero.

Arguments:

    lpPathName - Specifies the null terminated pathname of the directory
        to create the temporary file within.

    lpPrefixString - Points to a null-terminated character string to be
        used as the temporary filename prefix.  This string must consist
        of characters in the OEM-defined character set.

    uUnique - Specifies an unsigned integer.

    lpTempFileName - Points to the buffer that is to receive the
        temporary filename.  This string consists of characters in the
        OEM-defined character set.  This buffer should be at least MAX_PATH
        characters in length to allow sufficient room for the pathname.

Return Value:

    The return value specifies a unique numeric value used in the
    temporary filename.  If a nonzero value was given for the uUnique
    parameter, the return value specifies this same number.

--*/

{
#if !defined(BUILD_WOW6432)
    BASE_API_MSG m;
    PBASE_GETTEMPFILE_MSG a = (PBASE_GETTEMPFILE_MSG)&m.u.GetTempFile;
#endif
    LPWSTR p,savedp;
    ULONG Length;
    HANDLE FileHandle;
    ULONG PassCount;
    DWORD LastError;
    UNICODE_STRING UnicodePath, UnicodePrefix;
    CHAR UniqueAsAnsi[8];
    CHAR *c;
    ULONG i;

#if defined(BUILD_WOW6432)
    UINT uNewUnique;
#endif

    PassCount = 0;
    RtlInitUnicodeString(&UnicodePath,lpPathName);
    Length = UnicodePath.Length;

    RtlMoveMemory(lpTempFileName,lpPathName,UnicodePath.MaximumLength);
    if ( lpTempFileName[(Length>>1)-1] != (WCHAR)'\\' ) {
        lpTempFileName[Length>>1] = (WCHAR)'\\';
        Length += sizeof(UNICODE_NULL);
        }

    lpTempFileName[(Length>>1)-1] = UNICODE_NULL;
    i = GetFileAttributesW(lpTempFileName);
    if ( (i == 0xFFFFFFFF) ||
         !(i & FILE_ATTRIBUTE_DIRECTORY) ) {
        SetLastError(ERROR_DIRECTORY);
        return FALSE;
        }
    lpTempFileName[(Length>>1)-1] = (WCHAR)'\\';

    RtlInitUnicodeString(&UnicodePrefix,lpPrefixString);
    if ( UnicodePrefix.Length > (USHORT)6 ) {
        UnicodePrefix.Length = (USHORT)6;
        }
    p = &lpTempFileName[Length>>1];
    Length = UnicodePrefix.Length;
    RtlMoveMemory(p,lpPrefixString,Length);
    p += (Length>>1);
    savedp = p;
    //
    // If uUnique is not specified, then get one
    //

    uUnique = uUnique & 0x0000ffff;

try_again:
    p = savedp;
    if ( !uUnique ) {

#if defined(BUILD_WOW6432)
        uNewUnique = CsrBasepGetTempFile();
        if ( uNewUnique == 0 ) {
#else
        CsrClientCallServer( (PCSR_API_MSG)&m,
                             NULL,
                             CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                                  BasepGetTempFile
                                                ),
                             sizeof( *a )
                           );
        a->uUnique = (UINT)m.ReturnValue;
        if ( m.ReturnValue == 0 ) {
#endif

            PassCount++;
            if ( PassCount & 0xffff0000 ) {
                return 0;
                }
            goto try_again;
            }
        }
    else {
#if defined(BUILD_WOW6432)
        uNewUnique = uUnique;
#else
        a->uUnique = uUnique;
#endif
        }

    //
    // Convert the unique value to a 4 byte character string
    //

#if defined(BUILD_WOW6432)
    RtlIntegerToChar ((ULONG) uNewUnique,16,5,UniqueAsAnsi);
#else
    RtlIntegerToChar ((ULONG) a->uUnique,16,5,UniqueAsAnsi);
#endif
    c = UniqueAsAnsi;
    for(i=0;i<4;i++){
        *p = RtlAnsiCharToUnicodeChar(&c);
        if ( *p == UNICODE_NULL ) {
            break;
            }
        p++;
        }
    RtlMoveMemory(p,BaseDotTmpSuffixName.Buffer,BaseDotTmpSuffixName.MaximumLength);

    if ( !uUnique ) {

        //
        // test for resulting name being a device (prefix com, uUnique 1-9...
        //

        if ( RtlIsDosDeviceName_U(lpTempFileName) ) {
            PassCount++;
            if ( PassCount & 0xffff0000 ) {
                SetLastError(ERROR_INVALID_NAME);
                return 0;
                }
            goto try_again;
            }

        FileHandle = CreateFileW(
                        lpTempFileName,
                        GENERIC_READ,
                        0,
                        NULL,
                        CREATE_NEW,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );
        //
        // If the create worked, then we are ok. Just close the file.
        // Otherwise, try again.
        //

        if ( FileHandle != INVALID_HANDLE_VALUE ) {
            NtClose(FileHandle);
            }
        else {
            
            //
            // FIXFIX This test should be inverted when time permits
            // sufficient testing to nail down the error codes that
            // would indicate is is reasonable to continue the loop
            // as opposed to stop the loop. All it currently takes is
            // CreateFile coming back with an error we don't know about
            // to make us spin here for a long time.
            //

            LastError = GetLastError();
            switch (LastError) {
                case ERROR_INVALID_PARAMETER     :
                case ERROR_WRITE_PROTECT         :
                case ERROR_FILE_NOT_FOUND        :
                case ERROR_BAD_PATHNAME          :
                case ERROR_INVALID_NAME          :
                case ERROR_PATH_NOT_FOUND        :
                case ERROR_NETWORK_ACCESS_DENIED :
                case ERROR_DISK_CORRUPT          :
                case ERROR_FILE_CORRUPT          :
                case ERROR_DISK_FULL             :
                    return 0;
                case ERROR_ACCESS_DENIED         :
                    // It's possible for us to hit this if there's a
                    // directory with the name we're trying; in that
                    // case, we can usefully continue.
                    // CreateFile() uses BaseSetLastNTError() to set
                    // LastStatusValue to the actual NT error in the
                    // TEB; we just need to check it, and only abort
                    // if it's not a directory.
                    // This was bug #397477.
                    if (NtCurrentTeb()->LastStatusValue
                        != STATUS_FILE_IS_A_DIRECTORY)
                        return 0;
                }

            PassCount++;
            if ( PassCount & 0xffff0000 ) {
                return 0;
                }
            goto try_again;
            }
        }
#if defined(BUILD_WOW6432)
    return uNewUnique;
#else
    return a->uUnique;
#endif
}

BOOL
APIENTRY
GetDiskFreeSpaceA(
    LPCSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )

/*++

Routine Description:

    ANSI thunk to GetDiskFreeSpaceW

--*/

{
    PUNICODE_STRING Unicode;

    if (!ARGUMENT_PRESENT( lpRootPathName )) {
        lpRootPathName = "\\";
    }

    Unicode = Basep8BitStringToStaticUnicodeString( lpRootPathName );
    if (Unicode == NULL) {
        return FALSE;
    }

    return ( GetDiskFreeSpaceW(
                (LPCWSTR)Unicode->Buffer,
                lpSectorsPerCluster,
                lpBytesPerSector,
                lpNumberOfFreeClusters,
                lpTotalNumberOfClusters
                )
            );
}

BOOL
APIENTRY
GetDiskFreeSpaceW(
    LPCWSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )

#define MAKE2GFRIENDLY(lpOut, dwSize)                                           \
                                                                                \
    if (!bAppHack) {                                                            \
        *lpOut =  dwSize;                                                       \
    } else {                                                                    \
        dwTemp = SizeInfo.SectorsPerAllocationUnit * SizeInfo.BytesPerSector;   \
                                                                                \
        if (0x7FFFFFFF / dwTemp < dwSize) {                                     \
                                                                                \
            *lpOut = 0x7FFFFFFF / dwTemp;                                       \
        } else {                                                                \
            *lpOut =  dwSize;                                                   \
        }                                                                       \
    }



/*++

Routine Description:

    The free space on a disk and the size parameters can be returned
    using GetDiskFreeSpace.

Arguments:

    lpRootPathName - An optional parameter, that if specified, supplies
        the root directory of the disk whose free space is to be
        returned for.  If this parameter is not specified, then the root
        of the current directory is used.

    lpSectorsPerCluster - Returns the number of sectors per cluster
        where a cluster is the allocation granularity on the disk.

    lpBytesPerSector - Returns the number of bytes per sector.

    lpNumberOfFreeClusters - Returns the total number of free clusters
        on the disk.

    lpTotalNumberOfClusters - Returns the total number of clusters on
        the disk.

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
    PVOID FreeBuffer;
    FILE_FS_SIZE_INFORMATION SizeInfo;
    WCHAR DefaultPath[2];
    DWORD dwTemp;
    BOOL  bAppHack;

    DefaultPath[0] = (WCHAR)'\\';
    DefaultPath[1] = UNICODE_NULL;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            ARGUMENT_PRESENT(lpRootPathName) ? lpRootPathName : DefaultPath,
                            &FileName,
                            NULL,
                            NULL
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    //
    // Check to make sure a root was specified
    //

    if ( FileName.Buffer[(FileName.Length >> 1)-1] != (WCHAR)'\\' ) {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        return FALSE;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
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
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_FREE_SPACE_QUERY
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

        //
        // Prior releases of NT where these parameters were not optional
        // zeroed out this field even in the failure case.  Some applications
        // failed to check the return value from this function and instead
        // relied on this side effect.  I'm putting that back now so the apps
        // can still treat an unformatted volume as a zero size volume.
        //

        if (ARGUMENT_PRESENT( lpBytesPerSector )) {
            *lpBytesPerSector = 0;
            }
        return FALSE;
        }

    if ( !IsThisARootDirectory(Handle,&FileName) ) {
        NtClose(Handle);
        SetLastError(ERROR_DIR_NOT_ROOT);
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        return FALSE;
        }
    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

    //
    // Determine the size parameters of the volume.
    //

    Status = NtQueryVolumeInformationFile(
                Handle,
                &IoStatusBlock,
                &SizeInfo,
                sizeof(SizeInfo),
                FileFsSizeInformation
                );
    NtClose(Handle);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    //
    // See if the calling process needs hack to work with HDD > 2GB
    // 2GB is 0x80000000 bytes and some apps treat that as a signed LONG.
    //

    if ( NtCurrentPeb()->AppCompatInfo &&
        (((PAPP_COMPAT_INFO)(NtCurrentPeb()->AppCompatInfo))->CompatibilityFlags.QuadPart & KACF_GETDISKFREESPACE ) ) {

        bAppHack = TRUE;
    } else {
        bAppHack = FALSE;
    }
    
    //
    // Deal with 64 bit sizes
    //

    if ( SizeInfo.TotalAllocationUnits.HighPart ) {
        SizeInfo.TotalAllocationUnits.LowPart = (ULONG)-1;
        }
    if ( SizeInfo.AvailableAllocationUnits.HighPart ) {
        SizeInfo.AvailableAllocationUnits.LowPart = (ULONG)-1;
        }

    if (ARGUMENT_PRESENT( lpSectorsPerCluster )) {
        *lpSectorsPerCluster = SizeInfo.SectorsPerAllocationUnit;
        }
    if (ARGUMENT_PRESENT( lpBytesPerSector )) {
        *lpBytesPerSector = SizeInfo.BytesPerSector;
        }
    if (ARGUMENT_PRESENT( lpNumberOfFreeClusters )) {
        MAKE2GFRIENDLY(lpNumberOfFreeClusters, SizeInfo.AvailableAllocationUnits.LowPart);
        }
    if (ARGUMENT_PRESENT( lpTotalNumberOfClusters )) {
        MAKE2GFRIENDLY(lpTotalNumberOfClusters, SizeInfo.TotalAllocationUnits.LowPart);
        }

    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
GetDiskFreeSpaceExA(
    LPCSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    )
{
    PUNICODE_STRING Unicode;

    if (!ARGUMENT_PRESENT( lpDirectoryName )) {
        lpDirectoryName = "\\";
    }

    Unicode = Basep8BitStringToStaticUnicodeString( lpDirectoryName );
    if (Unicode == NULL) {
        return FALSE;
    }

    return ( GetDiskFreeSpaceExW(
                (LPCWSTR)Unicode->Buffer,
                lpFreeBytesAvailableToCaller,
                lpTotalNumberOfBytes,
                lpTotalNumberOfFreeBytes
                )
            );
}


WINBASEAPI
BOOL
WINAPI
GetDiskFreeSpaceExW(
    LPCWSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    PVOID FreeBuffer;
    union {
        FILE_FS_SIZE_INFORMATION Normal;
        FILE_FS_FULL_SIZE_INFORMATION Full;
    } SizeInfo;

    WCHAR DefaultPath[2];
    ULARGE_INTEGER BytesPerAllocationUnit;
    ULARGE_INTEGER FreeBytesAvailableToCaller;
    ULARGE_INTEGER TotalNumberOfBytes;

    DefaultPath[0] = (WCHAR)'\\';
    DefaultPath[1] = UNICODE_NULL;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            ARGUMENT_PRESENT(lpDirectoryName) ? lpDirectoryName : DefaultPath,
                            &FileName,
                            NULL,
                            NULL
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
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
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_FREE_SPACE_QUERY
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            }
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        return FALSE;
        }

    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

    //
    // If the caller wants the volume total then try to get a full
    // file size.
    //

    if ( ARGUMENT_PRESENT(lpTotalNumberOfFreeBytes) ) {

        Status = NtQueryVolumeInformationFile(
                    Handle,
                    &IoStatusBlock,
                    &SizeInfo,
                    sizeof(SizeInfo.Full),
                    FileFsFullSizeInformation
                    );

        if ( NT_SUCCESS(Status) ) {

            NtClose(Handle);

            BytesPerAllocationUnit.QuadPart =
                SizeInfo.Full.BytesPerSector * SizeInfo.Full.SectorsPerAllocationUnit;

            if ( ARGUMENT_PRESENT(lpFreeBytesAvailableToCaller) ) {
                lpFreeBytesAvailableToCaller->QuadPart =
                    BytesPerAllocationUnit.QuadPart *
                    SizeInfo.Full.CallerAvailableAllocationUnits.QuadPart;
                }
            if ( ARGUMENT_PRESENT(lpTotalNumberOfBytes) ) {
                lpTotalNumberOfBytes->QuadPart =
                    BytesPerAllocationUnit.QuadPart * SizeInfo.Full.TotalAllocationUnits.QuadPart;
                }
            lpTotalNumberOfFreeBytes->QuadPart =
                BytesPerAllocationUnit.QuadPart *
                SizeInfo.Full.ActualAvailableAllocationUnits.QuadPart;

            return TRUE;
        }
    }

    //
    // Determine the size parameters of the volume.
    //

    Status = NtQueryVolumeInformationFile(
                Handle,
                &IoStatusBlock,
                &SizeInfo,
                sizeof(SizeInfo.Normal),
                FileFsSizeInformation
                );
    NtClose(Handle);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    BytesPerAllocationUnit.QuadPart =
        SizeInfo.Normal.BytesPerSector * SizeInfo.Normal.SectorsPerAllocationUnit;

    FreeBytesAvailableToCaller.QuadPart =
        BytesPerAllocationUnit.QuadPart * SizeInfo.Normal.AvailableAllocationUnits.QuadPart;

    TotalNumberOfBytes.QuadPart =
        BytesPerAllocationUnit.QuadPart * SizeInfo.Normal.TotalAllocationUnits.QuadPart;

    if ( ARGUMENT_PRESENT(lpFreeBytesAvailableToCaller) ) {
        lpFreeBytesAvailableToCaller->QuadPart = FreeBytesAvailableToCaller.QuadPart;
        }
    if ( ARGUMENT_PRESENT(lpTotalNumberOfBytes) ) {
        lpTotalNumberOfBytes->QuadPart = TotalNumberOfBytes.QuadPart;
        }
    if ( ARGUMENT_PRESENT(lpTotalNumberOfFreeBytes) ) {
        lpTotalNumberOfFreeBytes->QuadPart = FreeBytesAvailableToCaller.QuadPart;
        }

    return TRUE;
}

BOOL
APIENTRY
GetVolumeInformationA(
    LPCSTR lpRootPathName,
    LPSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    LPSTR lpFileSystemNameBuffer,
    DWORD nFileSystemNameSize
    )

/*++

Routine Description:

    ANSI thunk to GetVolumeInformationW

--*/

{
    PUNICODE_STRING Unicode;
    NTSTATUS Status;
    UNICODE_STRING UnicodeVolumeName;
    UNICODE_STRING UnicodeFileSystemName;
    ANSI_STRING AnsiVolumeName;
    ANSI_STRING AnsiFileSystemName;
    BOOL ReturnValue;

    if (!ARGUMENT_PRESENT( lpRootPathName )) {
        lpRootPathName = "\\";
    }

    Unicode = Basep8BitStringToStaticUnicodeString( lpRootPathName );
    if (Unicode == NULL) {
        return FALSE;
    }

    UnicodeVolumeName.Buffer = NULL;
    UnicodeFileSystemName.Buffer = NULL;
    UnicodeVolumeName.MaximumLength = 0;
    UnicodeFileSystemName.MaximumLength = 0;
    AnsiVolumeName.Buffer = lpVolumeNameBuffer;
    AnsiVolumeName.MaximumLength = (USHORT)(nVolumeNameSize+1);
    AnsiFileSystemName.Buffer = lpFileSystemNameBuffer;
    AnsiFileSystemName.MaximumLength = (USHORT)(nFileSystemNameSize+1);

    try {
        if ( ARGUMENT_PRESENT(lpVolumeNameBuffer) ) {
            UnicodeVolumeName.MaximumLength = AnsiVolumeName.MaximumLength << 1;
            UnicodeVolumeName.Buffer = RtlAllocateHeap(
                                            RtlProcessHeap(), MAKE_TAG( TMP_TAG ),
                                            UnicodeVolumeName.MaximumLength
                                            );

            if ( !UnicodeVolumeName.Buffer ) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
                }
            }

        if ( ARGUMENT_PRESENT(lpFileSystemNameBuffer) ) {
            UnicodeFileSystemName.MaximumLength = AnsiFileSystemName.MaximumLength << 1;
            UnicodeFileSystemName.Buffer = RtlAllocateHeap(
                                                RtlProcessHeap(), MAKE_TAG( TMP_TAG ),
                                                UnicodeFileSystemName.MaximumLength
                                                );

            if ( !UnicodeFileSystemName.Buffer ) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
                }
            }

        ReturnValue = GetVolumeInformationW(
                            (LPCWSTR)Unicode->Buffer,
                            UnicodeVolumeName.Buffer,
                            nVolumeNameSize,
                            lpVolumeSerialNumber,
                            lpMaximumComponentLength,
                            lpFileSystemFlags,
                            UnicodeFileSystemName.Buffer,
                            nFileSystemNameSize
                            );

        if ( ReturnValue ) {

            if ( ARGUMENT_PRESENT(lpVolumeNameBuffer) ) {
                RtlInitUnicodeString(
                    &UnicodeVolumeName,
                    UnicodeVolumeName.Buffer
                    );

                Status = BasepUnicodeStringTo8BitString(
                            &AnsiVolumeName,
                            &UnicodeVolumeName,
                            FALSE
                            );

                if ( !NT_SUCCESS(Status) ) {
                    BaseSetLastNTError(Status);
                    return FALSE;
                    }
                }

            if ( ARGUMENT_PRESENT(lpFileSystemNameBuffer) ) {
                RtlInitUnicodeString(
                    &UnicodeFileSystemName,
                    UnicodeFileSystemName.Buffer
                    );

                Status = BasepUnicodeStringTo8BitString(
                            &AnsiFileSystemName,
                            &UnicodeFileSystemName,
                            FALSE
                            );

                if ( !NT_SUCCESS(Status) ) {
                    BaseSetLastNTError(Status);
                    return FALSE;
                    }
                }
            }
        }
    finally {
        if ( UnicodeVolumeName.Buffer ) {
            RtlFreeHeap(RtlProcessHeap(), 0,UnicodeVolumeName.Buffer);
            }
        if ( UnicodeFileSystemName.Buffer ) {
            RtlFreeHeap(RtlProcessHeap(), 0,UnicodeFileSystemName.Buffer);
            }
        }

    return ReturnValue;
}

BOOL
APIENTRY
GetVolumeInformationW(
    LPCWSTR lpRootPathName,
    LPWSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    LPWSTR lpFileSystemNameBuffer,
    DWORD nFileSystemNameSize
    )

/*++

Routine Description:

    This function returns information about the file system whose root
    directory is specified.

Arguments:

    lpRootPathName - An optional parameter, that if specified, supplies
        the root directory of the file system that information is to be
        returned about.  If this parameter is not specified, then the
        root of the current directory is used.

    lpVolumeNameBuffer - An optional parameter that if specified returns
        the name of the specified volume.

    nVolumeNameSize - Supplies the length of the volume name buffer.
        This parameter is ignored if the volume name buffer is not
        supplied.

    lpVolumeSerialNumber - An optional parameter that if specified
        points to a DWORD.  The DWORD contains the 32-bit of the volume
        serial number.

    lpMaximumComponentLength - An optional parameter that if specified
        returns the maximum length of a filename component supported by
        the specified file system.  A filename component is that portion
        of a filename between pathname seperators.

    lpFileSystemFlags - An optional parameter that if specified returns
        flags associated with the specified file system.

        lpFileSystemFlags Flags:

            FS_CASE_IS_PRESERVED - Indicates that the case of file names
                is preserved when the name is placed on disk.

            FS_CASE_SENSITIVE - Indicates that the file system supports
                case sensitive file name lookup.

            FS_UNICODE_STORED_ON_DISK - Indicates that the file system
                supports unicode in file names as they appear on disk.

    lpFileSystemNameBuffer - An optional parameter that if specified returns
        the name for the specified file system (e.g. FAT, HPFS...).

    nFileSystemNameSize - Supplies the length of the file system name
        buffer.  This parameter is ignored if the file system name
        buffer is not supplied.

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
    PVOID FreeBuffer;
    PFILE_FS_ATTRIBUTE_INFORMATION AttributeInfo;
    PFILE_FS_VOLUME_INFORMATION VolumeInfo;
    ULONG AttributeInfoLength;
    ULONG VolumeInfoLength;
    WCHAR DefaultPath[2];
    BOOL rv;
    ULONG HardErrorValue;
    PTEB Teb;

    rv = FALSE;
    DefaultPath[0] = (WCHAR)'\\';
    DefaultPath[1] = UNICODE_NULL;

    nVolumeNameSize *= 2;
    nFileSystemNameSize *= 2;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            ARGUMENT_PRESENT(lpRootPathName) ? lpRootPathName : DefaultPath,
                            &FileName,
                            NULL,
                            NULL
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    //
    // Check to make sure a root was specified
    //

    if ( FileName.Buffer[(FileName.Length >> 1)-1] != '\\' ) {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        return FALSE;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    AttributeInfo = NULL;
    VolumeInfo = NULL;

    //
    // Open the file
    //
    Teb = NtCurrentTeb();
    HardErrorValue = Teb->HardErrorsAreDisabled;
    Teb->HardErrorsAreDisabled = 1;
    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                );
    Teb->HardErrorsAreDisabled = HardErrorValue;
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        return FALSE;
        }

    if ( !IsThisARootDirectory(Handle,&FileName) ) {
        NtClose(Handle);
        SetLastError(ERROR_DIR_NOT_ROOT);
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        return FALSE;
        }
    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

    if ( ARGUMENT_PRESENT(lpVolumeNameBuffer) ||
         ARGUMENT_PRESENT(lpVolumeSerialNumber) ) {
        if ( ARGUMENT_PRESENT(lpVolumeNameBuffer) ) {
            VolumeInfoLength = sizeof(*VolumeInfo)+nVolumeNameSize;
            }
        else {
            VolumeInfoLength = sizeof(*VolumeInfo)+MAX_PATH;
            }
        VolumeInfo = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), VolumeInfoLength);

        if ( !VolumeInfo ) {
            NtClose(Handle);
            BaseSetLastNTError(STATUS_NO_MEMORY);
            return FALSE;
            }
        }

    if ( ARGUMENT_PRESENT(lpFileSystemNameBuffer) ||
         ARGUMENT_PRESENT(lpMaximumComponentLength) ||
         ARGUMENT_PRESENT(lpFileSystemFlags) ) {
        if ( ARGUMENT_PRESENT(lpFileSystemNameBuffer) ) {
            AttributeInfoLength = sizeof(*AttributeInfo) + nFileSystemNameSize;
            }
        else {
            AttributeInfoLength = sizeof(*AttributeInfo) + MAX_PATH;
            }
        AttributeInfo = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), AttributeInfoLength);
        if ( !AttributeInfo ) {
            NtClose(Handle);
            if ( VolumeInfo ) {
                RtlFreeHeap(RtlProcessHeap(), 0,VolumeInfo);
                }
            BaseSetLastNTError(STATUS_NO_MEMORY);
            return FALSE;
            }
        }

    try {
        if ( VolumeInfo ) {
            Status = NtQueryVolumeInformationFile(
                        Handle,
                        &IoStatusBlock,
                        VolumeInfo,
                        VolumeInfoLength,
                        FileFsVolumeInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                rv = FALSE;
                goto finally_exit;
                }
            }

        if ( AttributeInfo ) {
            Status = NtQueryVolumeInformationFile(
                        Handle,
                        &IoStatusBlock,
                        AttributeInfo,
                        AttributeInfoLength,
                        FileFsAttributeInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                rv = FALSE;
                goto finally_exit;
                }
            }
        try {

            if ( ARGUMENT_PRESENT(lpVolumeNameBuffer) ) {
                if ( VolumeInfo->VolumeLabelLength >= nVolumeNameSize ) {
                    SetLastError(ERROR_BAD_LENGTH);
                    rv = FALSE;
                    goto finally_exit;
                    }
                else {
                    RtlMoveMemory( lpVolumeNameBuffer,
                                   VolumeInfo->VolumeLabel,
                                   VolumeInfo->VolumeLabelLength );

                    *(lpVolumeNameBuffer + (VolumeInfo->VolumeLabelLength >> 1)) = UNICODE_NULL;
                    }
                }

            if ( ARGUMENT_PRESENT(lpVolumeSerialNumber) ) {
                *lpVolumeSerialNumber = VolumeInfo->VolumeSerialNumber;
                }

            if ( ARGUMENT_PRESENT(lpFileSystemNameBuffer) ) {

                if ( AttributeInfo->FileSystemNameLength >= nFileSystemNameSize ) {
                    SetLastError(ERROR_BAD_LENGTH);
                    rv = FALSE;
                    goto finally_exit;
                    }
                else {
                    RtlMoveMemory( lpFileSystemNameBuffer,
                                   AttributeInfo->FileSystemName,
                                   AttributeInfo->FileSystemNameLength );

                    *(lpFileSystemNameBuffer + (AttributeInfo->FileSystemNameLength >> 1)) = UNICODE_NULL;
                    }
                }

            if ( ARGUMENT_PRESENT(lpMaximumComponentLength) ) {
                *lpMaximumComponentLength = AttributeInfo->MaximumComponentNameLength;
                }

            if ( ARGUMENT_PRESENT(lpFileSystemFlags) ) {
                *lpFileSystemFlags = AttributeInfo->FileSystemAttributes;
                }
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            BaseSetLastNTError(STATUS_ACCESS_VIOLATION);
            return FALSE;
            }
        rv = TRUE;
finally_exit:;
        }
    finally {
        NtClose(Handle);
        if ( VolumeInfo ) {
            RtlFreeHeap(RtlProcessHeap(), 0,VolumeInfo);
            }
        if ( AttributeInfo ) {
            RtlFreeHeap(RtlProcessHeap(), 0,AttributeInfo);
            }
        }
    return rv;
}

DWORD
APIENTRY
GetLogicalDriveStringsA(
    DWORD nBufferLength,
    LPSTR lpBuffer
    )
{
    ULONG DriveMap;
    ANSI_STRING RootName;
    int i;
    PUCHAR Dst;
    DWORD BytesLeft;
    DWORD BytesNeeded;
    BOOL WeFailed;
    CHAR szDrive[] = "A:\\";

    BytesNeeded = 0;
    BytesLeft = nBufferLength;
    Dst = (PUCHAR)lpBuffer;
    WeFailed = FALSE;

    RtlInitAnsiString(&RootName, szDrive);
    DriveMap = GetLogicalDrives();
    for ( i=0; i<26; i++ ) {
        RootName.Buffer[0] = (CHAR)((CHAR)i+'A');
        if (DriveMap & (1 << i) ) {

            BytesNeeded += RootName.MaximumLength;
            if ( BytesNeeded < (USHORT)BytesLeft ) {
                RtlMoveMemory(Dst,RootName.Buffer,RootName.MaximumLength);
                Dst += RootName.MaximumLength;
                *Dst = '\0';
                }
            else {
                WeFailed = TRUE;
                }
            }
        }

    if ( WeFailed ) {
        BytesNeeded++;
        }
    //
    // Need to handle network uses;
    //

    return( BytesNeeded );
}

DWORD
APIENTRY
GetLogicalDriveStringsW(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    )
{
    ULONG DriveMap;
    UNICODE_STRING RootName;
    int i;
    PUCHAR Dst;
    DWORD BytesLeft;
    DWORD BytesNeeded;
    BOOL WeFailed;
    WCHAR wszDrive[] = L"A:\\";

    nBufferLength = nBufferLength*2;
    BytesNeeded = 0;
    BytesLeft = nBufferLength;
    Dst = (PUCHAR)lpBuffer;
    WeFailed = FALSE;

    RtlInitUnicodeString(&RootName, wszDrive);

    DriveMap = GetLogicalDrives();
    for ( i=0; i<26; i++ ) {
        RootName.Buffer[0] = (WCHAR)((CHAR)i+'A');
        if (DriveMap & (1 << i) ) {

            BytesNeeded += RootName.MaximumLength;
            if ( BytesNeeded < (USHORT)BytesLeft ) {
                RtlMoveMemory(Dst,RootName.Buffer,RootName.MaximumLength);
                Dst += RootName.MaximumLength;
                *(PWSTR)Dst = UNICODE_NULL;
                }
            else {
                WeFailed = TRUE;
                }
            }
        }

    if ( WeFailed ) {
        BytesNeeded += 2;
        }

    //
    // Need to handle network uses;
    //

    return( BytesNeeded/2 );
}

BOOL
WINAPI
SetVolumeLabelA(
    LPCSTR lpRootPathName,
    LPCSTR lpVolumeName
    )
{
    PUNICODE_STRING Unicode;
    UNICODE_STRING UnicodeVolumeName;
    BOOL ReturnValue;

    if (!ARGUMENT_PRESENT( lpRootPathName )) {
        lpRootPathName = "\\";
    }

    Unicode = Basep8BitStringToStaticUnicodeString( lpRootPathName );

    if (Unicode == NULL) {
        return FALSE;
        }

    if ( ARGUMENT_PRESENT(lpVolumeName) ) {
        if (!Basep8BitStringToDynamicUnicodeString( &UnicodeVolumeName, lpVolumeName )) {
            return FALSE;
        }

    } else {
        UnicodeVolumeName.Buffer = NULL;
    }

    ReturnValue = SetVolumeLabelW((LPCWSTR)Unicode->Buffer,(LPCWSTR)UnicodeVolumeName.Buffer);

    RtlFreeUnicodeString(&UnicodeVolumeName);

    return ReturnValue;
}

BOOL
WINAPI
SetVolumeLabelW(
    LPCWSTR lpRootPathName,
    LPCWSTR lpVolumeName
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    UNICODE_STRING LabelName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    PVOID FreeBuffer;
    PFILE_FS_LABEL_INFORMATION LabelInformation;
    ULONG LabelInfoLength;
    WCHAR DefaultPath[2];
    BOOL rv;
    WCHAR volumeName[MAX_PATH];
    BOOL usingVolumeName;

    rv = FALSE;
    DefaultPath[0] = (WCHAR)'\\';
    DefaultPath[1] = UNICODE_NULL;

    if ( ARGUMENT_PRESENT(lpVolumeName) ) {
        RtlInitUnicodeString(&LabelName,lpVolumeName);
        }
    else {
        LabelName.Length = 0;
        LabelName.MaximumLength = 0;
        LabelName.Buffer = NULL;
        }

    if (ARGUMENT_PRESENT(lpRootPathName)) {
        if (GetVolumeNameForVolumeMountPointW(lpRootPathName, volumeName,
                                              MAX_PATH)) {

            usingVolumeName = TRUE;
        } else {
            usingVolumeName = FALSE;
        }
    } else {
        usingVolumeName = FALSE;
    }

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            usingVolumeName ? volumeName : (ARGUMENT_PRESENT(lpRootPathName) ? lpRootPathName : DefaultPath),
                            &FileName,
                            NULL,
                            NULL
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    //
    // Check to make sure a root was specified
    //

    if ( FileName.Buffer[(FileName.Length >> 1)-1] != '\\' ) {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        return FALSE;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open the file
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_WRITE_DATA | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT
                );
    if ( !NT_SUCCESS(Status) ) {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        BaseSetLastNTError(Status);
        return FALSE;
        }

    if ( !IsThisARootDirectory(Handle,NULL) ) {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        NtClose(Handle);
        SetLastError(ERROR_DIR_NOT_ROOT);
        return FALSE;
        }

    NtClose(Handle);

    //
    // Now open the volume DASD by ignoring the ending backslash
    //

    FileName.Length -= 2;

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open the volume
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_WRITE_DATA | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT
                );
    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    //
    // Set the volume label
    //

    LabelInformation = NULL;

    try {

        rv = TRUE;

        //
        // the label info buffer contains a single wchar that is the basis of
        // the label name. Subtract this out so the info length is the length
        // of the label and the structure (not including the extra wchar)
        //

        if ( LabelName.Length ) {
            LabelInfoLength = sizeof(*LabelInformation) + LabelName.Length - sizeof(WCHAR);
            }
        else {
            LabelInfoLength = sizeof(*LabelInformation);
            }

        LabelInformation = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), LabelInfoLength);
        if ( LabelInformation ) {
            RtlCopyMemory(
                LabelInformation->VolumeLabel,
                LabelName.Buffer,
                LabelName.Length
                );
            LabelInformation->VolumeLabelLength = LabelName.Length;
            Status = NtSetVolumeInformationFile(
                        Handle,
                        &IoStatusBlock,
                        (PVOID) LabelInformation,
                        LabelInfoLength,
                        FileFsLabelInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                rv = FALSE;
                BaseSetLastNTError(Status);
                }
            }
        else {
            rv = FALSE;
            BaseSetLastNTError(STATUS_NO_MEMORY);
            }
        }
    finally {
        NtClose(Handle);
        if ( LabelInformation ) {
            RtlFreeHeap(RtlProcessHeap(), 0,LabelInformation);
            }
        }
    return rv;
}



#if 0
//
// frankar, let me know if this is needed...
//
UINT
WINAPI
GetZawSysDirectoryA(
    LPSTR lpBuffer,
    UINT uSize
    )
{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    ULONG  cbAnsiString;

    UnicodeString.MaximumLength = (USHORT)((uSize<<1)+sizeof(UNICODE_NULL));
    UnicodeString.Buffer = RtlAllocateHeap(
                                RtlProcessHeap(), MAKE_TAG( TMP_TAG ),
                                UnicodeString.MaximumLength
                                );
    if ( !UnicodeString.Buffer ) {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return 0;
        }
    UnicodeString.Length = (USHORT)GetZawSysDirectoryW(
                                        UnicodeString.Buffer,
                                        (DWORD)(UnicodeString.MaximumLength-sizeof(UNICODE_NULL))/2
                                        )*2;
    if ( UnicodeString.Length > (USHORT)(UnicodeString.MaximumLength-sizeof(UNICODE_NULL)) ) {
        RtlFreeHeap(RtlProcessHeap(), 0,UnicodeString.Buffer);

        //
        // given buffer size is too small.
        // allocate enough size of buffer and try again
        //
        // we need to get entire unicode path
        // otherwise we can't figure out the exact length
        // of corresponding ansi string (cbAnsiString).

        UnicodeString.Buffer = RtlAllocateHeap ( RtlProcessHeap(),
                                                 MAKE_TAG( TMP_TAG ),
                                                 UnicodeString.Length+ sizeof(UNICODE_NULL));
        if ( !UnicodeString.Buffer ) {
             BaseSetLastNTError(STATUS_NO_MEMORY);
             return 0;
             }

        UnicodeString.Length = (USHORT)GetZawSysDirectoryW(
                                     UnicodeString.Buffer,
                                     (DWORD)(UnicodeString.MaximumLength-sizeof(UNICODE_NULL))/2,
                                     ) * 2;
        Status = RtlUnicodeToMultiByteSize( &cbAnsiString,
                                            UnicodeString.Buffer,
                                            UnicodeString.Length );
        if ( !NT_SUCCESS(Status) ) {
            RtlFreeHeap(RtlProcessHeap(), 0, UnicodeString.Buffer);
            BaseSetLastNTError(Status);
            return 0;
            }
        else if ( nBufferLength < cbAnsiString ) {
            RtlFreeHeap(RtlProcessHeap(), 0, UnicodeString.Buffer);
            return cbAnsiString;
            }
        }
    AnsiString.Buffer = lpBuffer;
    AnsiString.MaximumLength = (USHORT)(uSize+1);
    Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    RtlFreeHeap(RtlProcessHeap(), 0,UnicodeString.Buffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return 0;
        }
    return AnsiString.Length;
}

UINT
WINAPI
GetZawWindDirectoryA(
    LPSTR lpBuffer,
    UINT uSize
    )
{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    ULONG  cbAnsiString;

    UnicodeString.MaximumLength = (USHORT)((uSize<<1)+sizeof(UNICODE_NULL));
    UnicodeString.Buffer = RtlAllocateHeap(
                                RtlProcessHeap(), MAKE_TAG( TMP_TAG ),
                                UnicodeString.MaximumLength
                                );
    if ( !UnicodeString.Buffer ) {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return 0;
        }
    UnicodeString.Length = (USHORT)GetZawWindDirectoryW(
                                        UnicodeString.Buffer,
                                        (DWORD)(UnicodeString.MaximumLength-sizeof(UNICODE_NULL))/2
                                        )*2;
    if ( UnicodeString.Length > (USHORT)(UnicodeString.MaximumLength-sizeof(UNICODE_NULL)) ) {
        RtlFreeHeap(RtlProcessHeap(), 0,UnicodeString.Buffer);

        //
        // given buffer size is too small.
        // allocate enough size of buffer and try again
        //
        // we need to get entire unicode path
        // otherwise we can't figure out the exact length
        // of corresponding ansi string (cbAnsiString).

        UnicodeString.Buffer = RtlAllocateHeap ( RtlProcessHeap(),
                                                 MAKE_TAG( TMP_TAG ),
                                                 UnicodeString.Length+ sizeof(UNICODE_NULL));
        if ( !UnicodeString.Buffer ) {
            BaseSetLastNTError(STATUS_NO_MEMORY);
            return 0;
            }

        UnicodeString.Length = (USHORT)GetZawWindDirectoryW(
                                     UnicodeString.Buffer,
                                     (DWORD)(UnicodeString.MaximumLength-sizeof(UNICODE_NULL))/2
                                     ) * 2;
        Status = RtlUnicodeToMultiByteSize( &cbAnsiString,
                                            UnicodeString.Buffer,
                                            UnicodeString.Length );
        if ( !NT_SUCCESS(Status) ) {
            RtlFreeHeap(RtlProcessHeap(), 0, UnicodeString.Buffer);
            BaseSetLastNTError(Status);
            return 0;
            }
        else if ( nBufferLength < cbAnsiString ) {
            RtlFreeHeap(RtlProcessHeap(), 0, UnicodeString.Buffer);
            return cbAnsiString;
            }
        }
    AnsiString.Buffer = lpBuffer;
    AnsiString.MaximumLength = (USHORT)(uSize+1);
    Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    RtlFreeHeap(RtlProcessHeap(), 0,UnicodeString.Buffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return 0;
        }
    return AnsiString.Length;
}

UINT
WINAPI
GetZawSysDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )
{
    NTSTATUS Status;
    HANDLE CurrentUserKey;
    HANDLE DirKey;
    UNICODE_STRING KeyName;
    UNICODE_STRING KeyValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG DataLength;
    ULONG ValueInfoBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+MAX_PATH/2];
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;

    Status = RtlOpenCurrentUser(GENERIC_READ,&CurrentUserKey);

    if ( !NT_SUCCESS(Status) ) {
bail_gzsd:
        return GetSystemDirectoryW(lpBuffer,uSize);
        }

    RtlInitUnicodeString(&KeyName,L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ZAW");

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                CurrentUserKey,
                                NULL
                              );
    Status = NtOpenKey( &DirKey,
                        KEY_READ | KEY_NOTIFY | KEY_WRITE,
                        &ObjectAttributes
                      );

    NtClose(CurrentUserKey);
    if ( !NT_SUCCESS(Status) ) {
        goto bail_gzsd;
        }

    RtlInitUnicodeString(&KeyValueName,L"ZawSys");
    ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)&ValueInfoBuffer;
    Status = NtQueryValueKey( DirKey,
                              &KeyValueName,
                              KeyValuePartialInformation,
                              ValueInfo,
                              sizeof(ValueInfoBuffer),
                              &DataLength
                            );
    NtClose(DirKey);
    if ( !NT_SUCCESS(Status) ) {
        goto bail_gzsd;
        }
    if ( ValueInfo->DataLength > (uSize<<1) ) {
        goto bail_gzsd;
        }
    RtlCopyMemory(lpBuffer,ValueInfo->Data,ValueInfo->DataLength);
    return (ValueInfo->DataLength >> 1)-1;
}

UINT
WINAPI
GetZawWindDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )
{
    NTSTATUS Status;
    HANDLE CurrentUserKey;
    HANDLE DirKey;
    UNICODE_STRING KeyName;
    UNICODE_STRING KeyValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG DataLength;
    ULONG ValueInfoBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+MAX_PATH/2];
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;

    Status = RtlOpenCurrentUser(GENERIC_READ,&CurrentUserKey);

    if ( !NT_SUCCESS(Status) ) {
bail_gzwd:
        return GetWindowsDirectoryW(lpBuffer,uSize);
        }

    RtlInitUnicodeString(&KeyName,L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ZAW");

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                CurrentUserKey,
                                NULL
                              );
    Status = NtOpenKey( &DirKey,
                        KEY_READ | KEY_NOTIFY | KEY_WRITE,
                        &ObjectAttributes
                      );

    NtClose(CurrentUserKey);
    if ( !NT_SUCCESS(Status) ) {
        goto bail_gzwd;
        }

    RtlInitUnicodeString(&KeyValueName,L"ZawWind");
    ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)&ValueInfoBuffer;
    Status = NtQueryValueKey( DirKey,
                              &KeyValueName,
                              KeyValuePartialInformation,
                              ValueInfo,
                              sizeof(ValueInfoBuffer),
                              &DataLength
                            );
    NtClose(DirKey);
    if ( !NT_SUCCESS(Status) ) {
        goto bail_gzwd;
        }
    if ( ValueInfo->DataLength > (uSize<<1) ) {
        goto bail_gzwd;
        }
    RtlCopyMemory(lpBuffer,ValueInfo->Data,ValueInfo->DataLength);
    return (ValueInfo->DataLength >> 1)-1;
}
#endif
