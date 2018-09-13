/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dosdev.c

Abstract:

    This file contains the implementation of the DefineDosDevice API

Author:

    Steve Wood (stevewo) 13-Dec-1992

Revision History:

--*/

#include "basedll.h"

BOOL
WINAPI
DefineDosDeviceA(
    DWORD dwFlags,
    LPCSTR lpDeviceName,
    LPCSTR lpTargetPath
    )
{
    NTSTATUS Status;
    BOOL Result;
    ANSI_STRING AnsiString;
    PUNICODE_STRING DeviceName;
    UNICODE_STRING TargetPath;
    PCWSTR lpDeviceNameW;
    PCWSTR lpTargetPathW;

    RtlInitAnsiString( &AnsiString, lpDeviceName );
    DeviceName = &NtCurrentTeb()->StaticUnicodeString;
    Status = RtlAnsiStringToUnicodeString( DeviceName, &AnsiString, FALSE );
    if (!NT_SUCCESS( Status )) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError( ERROR_FILENAME_EXCED_RANGE );
            }
        else {
            BaseSetLastNTError( Status );
            }
        return FALSE;
        }
    else {
        lpDeviceNameW = DeviceName->Buffer;
        }

    if (ARGUMENT_PRESENT( lpTargetPath )) {
        RtlInitAnsiString( &AnsiString, lpTargetPath );
        Status = RtlAnsiStringToUnicodeString( &TargetPath, &AnsiString, TRUE );
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            return FALSE;
            }
        else {
            lpTargetPathW = TargetPath.Buffer;
            }
        }
    else {
        lpTargetPathW = NULL;
        }

    Result = DefineDosDeviceW( dwFlags,
                               lpDeviceNameW,
                               lpTargetPathW
                             );

    if (lpTargetPathW != NULL) {
        RtlFreeUnicodeString( &TargetPath );
        }

    return Result;
}


typedef
long
(WINAPI *PBROADCASTSYSTEMMESSAGEW)( DWORD, LPDWORD, UINT, WPARAM, LPARAM );

HMODULE hUser32Dll;
PBROADCASTSYSTEMMESSAGEW pBroadCastSystemMessageW;

BOOL
WINAPI
DefineDosDeviceW(
    DWORD dwFlags,
    PCWSTR lpDeviceName,
    PCWSTR lpTargetPath
    )

/*++

Routine Description:

    This function provides the capability to define new DOS device names or
    redefine or delete existing DOS device names.  DOS Device names are stored
    as symbolic links in the NT object name space.  The code that converts
    a DOS path into a corresponding NT path uses these symbolic links to
    handle mapping of DOS devices and drive letters.  This API provides a
    mechanism for a Win32 Application to modify the symbolic links used
    to implement the DOS Device namespace.  Use the QueryDosDevice API
    to query the current mapping for a DOS device name.

Arguments:

    dwFlags - Supplies additional flags that control the creation
        of the DOS device.

        dwFlags Flags:

        DDD_PUSH_POP_DEFINITION - If lpTargetPath is not NULL, then push
            the new target path in front of any existing target path.
            If lpTargetPath is NULL, then delete the existing target path
            and pop the most recent one pushed.  If nothing left to pop
            then the device name will be deleted.

        DDD_RAW_TARGET_PATH - Do not convert the lpTargetPath string from
            a DOS path to an NT path, but take it as is.

    lpDeviceName - Points to the DOS device name being defined, redefined or deleted.
        It must NOT have a trailing colon unless it is a drive letter being defined,
        redefined or deleted.

    lpTargetPath - Points to the DOS path that will implement this device.  If the
        ADD_RAW_TARGET_PATH flag is specified, then this parameter points to an
        NT path string.  If this parameter is NULL, then the device name is being
        deleted or restored if the ADD_PUSH_POP_DEFINITION flag is specified.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
#if !defined(BUILD_WOW6432)
    BASE_API_MSG m;
    PBASE_DEFINEDOSDEVICE_MSG a = (PBASE_DEFINEDOSDEVICE_MSG)&m.u.DefineDosDeviceApi;
    PCSR_CAPTURE_HEADER p;
    ULONG PointerCount, n;
#endif
    UNICODE_STRING DeviceName;
    UNICODE_STRING TargetPath;
    DWORD iDrive;
    DEV_BROADCAST_VOLUME dbv;
    DWORD dwRec = BSM_APPLICATIONS;

#if defined(BUILD_WOW6432)
    NTSTATUS Status;
#endif

    if (dwFlags & ~(DDD_RAW_TARGET_PATH |
                    DDD_REMOVE_DEFINITION |
                    DDD_EXACT_MATCH_ON_REMOVE |
                    DDD_NO_BROADCAST_SYSTEM
                   ) ||
        (((!ARGUMENT_PRESENT( lpTargetPath ) || (dwFlags & DDD_EXACT_MATCH_ON_REMOVE))) &&
         !(dwFlags & DDD_REMOVE_DEFINITION)
        )
       ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
        }

    RtlInitUnicodeString( &DeviceName, lpDeviceName );
#if !defined(BUILD_WOW6432)
    PointerCount = 1;
    n = DeviceName.MaximumLength;
#endif
    if (ARGUMENT_PRESENT( lpTargetPath )) {
        if (!(dwFlags & DDD_RAW_TARGET_PATH)) {
            if (!RtlDosPathNameToNtPathName_U( lpTargetPath,
                                               &TargetPath,
                                               NULL,
                                               NULL
                                             )
               ) {
                BaseSetLastNTError( STATUS_OBJECT_NAME_INVALID );
                return FALSE;
                }
            }
        else {
            RtlInitUnicodeString( &TargetPath, lpTargetPath );
            }
#if !defined(BUILD_WOW6432)
        PointerCount += 1;
        n += TargetPath.MaximumLength;
#endif
        }
    else {
        RtlInitUnicodeString( &TargetPath, NULL );
        }

#if defined(BUILD_WOW6432)    
    Status = CsrBasepDefineDosDevice(dwFlags, &DeviceName, &TargetPath); 

    if (TargetPath.Length != 0 && !(dwFlags & DDD_RAW_TARGET_PATH)) {
    	RtlFreeUnicodeString( &TargetPath );
    }
#else
    p = CsrAllocateCaptureBuffer( PointerCount, n );
    if (p == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
        }

    a->Flags = dwFlags;
    a->DeviceName.MaximumLength =
        (USHORT)CsrAllocateMessagePointer( p,
                                           DeviceName.MaximumLength,
                                           (PVOID *)&a->DeviceName.Buffer
                                         );
    RtlUpcaseUnicodeString( &a->DeviceName, &DeviceName, FALSE );
    if (TargetPath.Length != 0) {
        a->TargetPath.MaximumLength =
            (USHORT)CsrAllocateMessagePointer( p,
                                               TargetPath.MaximumLength,
                                               (PVOID *)&a->TargetPath.Buffer
                                             );
        RtlCopyUnicodeString( &a->TargetPath, &TargetPath );
        if (!(dwFlags & DDD_RAW_TARGET_PATH)) {
            RtlFreeUnicodeString( &TargetPath );
            }
        }
    else {
        RtlInitUnicodeString( &a->TargetPath, NULL );
        }

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         p,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepDefineDosDevice
                                            ),
                         sizeof( *a )
                       );
    CsrFreeCaptureBuffer( p );
#endif

#if defined(BUILD_WOW6432)
    if (NT_SUCCESS( Status )) {
#else
    if (NT_SUCCESS( (NTSTATUS)m.ReturnValue )) {
#endif
        if (!(dwFlags & DDD_NO_BROADCAST_SYSTEM) &&
            DeviceName.Length == (2 * sizeof( WCHAR )) &&
            DeviceName.Buffer[ 1 ] == L':' &&
            (iDrive = RtlUpcaseUnicodeChar( DeviceName.Buffer[ 0 ] ) - L'A') < 26
           ) {
            dbv.dbcv_size       = sizeof( dbv );
            dbv.dbcv_devicetype = DBT_DEVTYP_VOLUME;
            dbv.dbcv_reserved   = 0;
            dbv.dbcv_unitmask   = (1 << iDrive);
            dbv.dbcv_flags      = DBTF_NET;

            if (hUser32Dll == NULL) {
                hUser32Dll = GetModuleHandleW( L"USER32.DLL" );
                if (hUser32Dll == NULL) {
                    hUser32Dll = LoadLibraryW( L"USER32.DLL" );
                    if (hUser32Dll == NULL) {
                        hUser32Dll = (HMODULE)LongToHandle(0xFFFFFFFF);
                        }
                    }

                if (hUser32Dll != NULL && hUser32Dll != (HMODULE)LongToHandle(0xFFFFFFFF)) {
                    pBroadCastSystemMessageW = (PBROADCASTSYSTEMMESSAGEW)
                        GetProcAddress( hUser32Dll, "BroadcastSystemMessageW" );
                    }
                }

            // broadcast to all windows!
            if (pBroadCastSystemMessageW != NULL) {
                (*pBroadCastSystemMessageW)( BSF_FORCEIFHUNG |
                                                BSF_NOHANG |
                                                BSF_NOTIMEOUTIFNOTHUNG,
                                             &dwRec,
                                             WM_DEVICECHANGE,
                                             (WPARAM)((dwFlags & DDD_REMOVE_DEFINITION) ?
                                                               DBT_DEVICEREMOVECOMPLETE :
                                                               DBT_DEVICEARRIVAL
                                                     ),
                                             (LPARAM)(DEV_BROADCAST_HDR *)&dbv
                                           );
                }
            }

        return TRUE;
        }
    else {
#if defined(BUILD_WOW6432)
        BaseSetLastNTError( Status );
#else
        BaseSetLastNTError( (NTSTATUS)m.ReturnValue );
#endif
        return FALSE;
        }
}


DWORD
WINAPI
QueryDosDeviceA(
    LPCSTR lpDeviceName,
    LPSTR lpTargetPath,
    DWORD ucchMax
    )
{
    NTSTATUS Status;
    DWORD Result;
    ANSI_STRING AnsiString;
    PUNICODE_STRING DeviceName;
    UNICODE_STRING TargetPath;
    PCWSTR lpDeviceNameW;
    PWSTR lpTargetPathW;

    if (ARGUMENT_PRESENT( lpDeviceName )) {
        RtlInitAnsiString( &AnsiString, lpDeviceName );
        DeviceName = &NtCurrentTeb()->StaticUnicodeString;
        Status = RtlAnsiStringToUnicodeString( DeviceName, &AnsiString, FALSE );
        if (!NT_SUCCESS( Status )) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError( ERROR_FILENAME_EXCED_RANGE );
                }
            else {
                BaseSetLastNTError( Status );
                }
            return FALSE;
            }
        else {
            lpDeviceNameW = DeviceName->Buffer;
            }
        }
    else {
        lpDeviceNameW = NULL;
        }

    lpTargetPathW = RtlAllocateHeap( RtlProcessHeap(),
                                     MAKE_TAG( TMP_TAG ),
                                     ucchMax * sizeof( WCHAR )
                                   );
    if (lpTargetPathW == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
        }

    Result = QueryDosDeviceW( lpDeviceNameW,
                              lpTargetPathW,
                              ucchMax
                            );

    if (Result != 0) {
        TargetPath.Buffer = lpTargetPathW;
        TargetPath.Length = (USHORT)(Result * sizeof( WCHAR ));
        TargetPath.MaximumLength = (USHORT)(TargetPath.Length + 1);

        AnsiString.Buffer = lpTargetPath;
        AnsiString.Length = 0;
        AnsiString.MaximumLength = (USHORT)ucchMax;

        Status = RtlUnicodeStringToAnsiString( &AnsiString,
                                               &TargetPath,
                                               FALSE
                                             );
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            Result = 0;
            }
        }

    RtlFreeHeap( RtlProcessHeap(), 0, lpTargetPathW );
    return Result;
}


DWORD
WINAPI
QueryDosDeviceW(
    PCWSTR lpDeviceName,
    PWSTR lpTargetPath,
    DWORD ucchMax
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DirectoryHandle;
    HANDLE LinkHandle;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    BOOLEAN RestartScan;
    UCHAR DirInfoBuffer[ 512 ];
    CLONG Count = 0;
    ULONG Context = 0;
    ULONG ReturnedLength;
    DWORD ucchName, ucchReturned;

    RtlInitUnicodeString( &UnicodeString, L"\\??" );
    InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_QUERY,
                                    &Attributes
                                  );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return 0;
        }

    ucchReturned = 0;
    try {
        if (ARGUMENT_PRESENT( lpDeviceName )) {
            RtlInitUnicodeString( &UnicodeString, lpDeviceName );
            InitializeObjectAttributes( &Attributes,
                                        &UnicodeString,
                                        OBJ_CASE_INSENSITIVE,
                                        DirectoryHandle,
                                        NULL
                                      );
            Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                               SYMBOLIC_LINK_QUERY,
                                               &Attributes
                                             );
            if (NT_SUCCESS( Status )) {
                UnicodeString.Buffer = lpTargetPath;
                UnicodeString.Length = 0;
                UnicodeString.MaximumLength = (USHORT)(ucchMax * sizeof( WCHAR ));
                ReturnedLength = 0;
                Status = NtQuerySymbolicLinkObject( LinkHandle,
                                                    &UnicodeString,
                                                    &ReturnedLength
                                                  );
                NtClose( LinkHandle );
                if (NT_SUCCESS( Status )) {
                    ucchReturned = ReturnedLength / sizeof( WCHAR );
		    if (ucchReturned < ucchMax) {
			lpTargetPath[ ucchReturned++ ] = UNICODE_NULL;
			}
		    else {
                        ucchReturned = 0;
			Status = STATUS_BUFFER_TOO_SMALL;
			}
                    }
                }
            }
        else {
            RestartScan = TRUE;
            DirInfo = (POBJECT_DIRECTORY_INFORMATION)&DirInfoBuffer;
            while (TRUE) {
                Status = NtQueryDirectoryObject( DirectoryHandle,
                                                 (PVOID)DirInfo,
                                                 sizeof( DirInfoBuffer ),
                                                 TRUE,
                                                 RestartScan,
                                                 &Context,
                                                 &ReturnedLength
                                               );

                //
                //  Check the status of the operation.
                //

                if (!NT_SUCCESS( Status )) {
                    if (Status == STATUS_NO_MORE_ENTRIES) {
                        Status = STATUS_SUCCESS;
                        }

                    break;
                    }

                if (!wcscmp( DirInfo->TypeName.Buffer, L"SymbolicLink" )) {
                    ucchName = DirInfo->Name.Length / sizeof( WCHAR );
		    if ((ucchReturned + ucchName + 1 + 1) > ucchMax) {
                        Status = STATUS_BUFFER_TOO_SMALL;
                        break;
                        }
                    RtlMoveMemory( lpTargetPath,
                                   DirInfo->Name.Buffer,
                                   DirInfo->Name.Length
                                 );
                    lpTargetPath += ucchName;
                    *lpTargetPath++ = UNICODE_NULL;
                    ucchReturned += ucchName + 1;
                    }

                RestartScan = FALSE;
                }

            if (NT_SUCCESS( Status )) {
		*lpTargetPath++ = UNICODE_NULL;
		ucchReturned++;
		}
            }
        }
    finally {
        NtClose( DirectoryHandle );
        }

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        }

    return ucchReturned;
}
