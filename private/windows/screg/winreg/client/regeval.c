/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regeval.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    enumerate value APIs.  That is:

        - RegEnumValueExA
        - RegEnumValueExW

Author:

    David J. Gilman (davegi) 18-Mar-1992

Notes:

    See the notes in server\regeval.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"



LONG
RegEnumValueA (
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD  lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )

/*++

Routine Description:

    Win32 ANSI RPC wrapper for enumerating values.

--*/

{
    PUNICODE_STRING     Name;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    LONG                Error = ERROR_SUCCESS;
    DWORD               ValueType;
    DWORD               ValueLength;
    DWORD               InputLength;
    PWSTR               UnicodeValueBuffer;
    ULONG               UnicodeValueLength;
    PSTR                AnsiValueBuffer;
    ULONG               AnsiValueLength;
    ULONG               Index;
    BOOLEAN             Win95Server = FALSE;
    ULONG               cbAnsi = 0;
    HKEY                TempHandle = NULL;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif


    //
    // Validate dependency between lpData and lpcbData parameters.
    //

    if( ARGUMENT_PRESENT( lpReserved ) ||
        (ARGUMENT_PRESENT( lpData ) && ( ! ARGUMENT_PRESENT( lpcbData )))) {
        return ERROR_INVALID_PARAMETER;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error =  ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Use the static Unicode string in the TEB as a temporary for the
    // value's name.
    //

    Name = &NtCurrentTeb( )->StaticUnicodeString;
    ASSERT( Name != NULL );
    Name->Length = 0;

    //
    // Call the Base API passing it a pointer to the counted Unicode
    // strings for the value name. Note that zero bytes are transmitted (i.e.
    // InputLength = 0) for the data.
    //

    if (ARGUMENT_PRESENT( lpcbData )) {
        ValueLength = *lpcbData;
        }
    else {
        ValueLength = 0;
        }

    InputLength = 0;

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegEnumValue (
                            hKey,
                            dwIndex,
                            Name,
                            &ValueType,
                            lpData,
                            &ValueLength,
                            &InputLength
                            );

        ASSERT( (&NtCurrentTeb( )->StaticUnicodeString)->Buffer );

    } else {
        DWORD dwVersion;

        //
        // Check for a downlevel Win95 server, which requires
        // us to work around their BaseRegEnumValue bugs.
        // The returned ValueLength is one WCHAR too large AND
        // they trash two bytes beyond the end of the buffer
        // for REG_SZ, REG_MULTI_SZ, and REG_EXPAND_SZ
        //
        Win95Server = IsWin95Server(DereferenceRemoteHandle(hKey),dwVersion);

        if (Win95Server) {
            LPBYTE lpWin95Data;
            //
            // This is a Win95 server.
            // Allocate a new buffer that is two bytes larger than
            // the old one so they can trash the last two bytes.
            //
            lpWin95Data = RtlAllocateHeap(RtlProcessHeap(),
                                          0,
                                          ValueLength+sizeof(WCHAR));
            if (lpWin95Data == NULL) {
                Error = ERROR_NOT_ENOUGH_MEMORY;
            } else {
                Error = (LONG)BaseRegEnumValue (DereferenceRemoteHandle( hKey ),
                                                dwIndex,
                                                Name,
                                                &ValueType,
                                                lpWin95Data,
                                                &ValueLength,
                                                &InputLength);
                if (Error == ERROR_SUCCESS) {
                    if ((ValueType == REG_SZ) ||
                        (ValueType == REG_MULTI_SZ) ||
                        (ValueType == REG_EXPAND_SZ)) {
                        //
                        // The returned length is one WCHAR too large
                        // and the last two bytes of the buffer are trashed.
                        //
                        ValueLength -= sizeof(WCHAR);
                    }
                    CopyMemory(lpData, lpWin95Data, ValueLength);
                }
                RtlFreeHeap(RtlProcessHeap(),0,lpWin95Data);
            }

        } else {
            Error = (LONG)BaseRegEnumValue (DereferenceRemoteHandle( hKey ),
                                            dwIndex,
                                            Name,
                                            &ValueType,
                                            lpData,
                                            &ValueLength,
                                            &InputLength);
        }

    }


    //
    // If no error or callers buffer too small, and type is one of the null
    // terminated string types, then do the UNICODE to ANSI translation.
    // We handle the buffer too small case, because the callers buffer may
    // be big enough for the ANSI representation, but not the UNICODE one.
    // In this case, we need to allocate a buffer big enough, do the query
    // again and then the translation into the callers buffer.
    //

    if ((Error == ERROR_SUCCESS || Error == ERROR_MORE_DATA) &&
        ARGUMENT_PRESENT( lpcbData ) &&
        (ValueType == REG_SZ ||
         ValueType == REG_EXPAND_SZ ||
         ValueType == REG_MULTI_SZ)
       ) {

        UnicodeValueLength         = ValueLength;

        AnsiValueBuffer        = lpData;
        AnsiValueLength        = ARGUMENT_PRESENT( lpcbData )? *lpcbData : 0;


        //
        // Allocate a buffer for the UNICODE value and reissue the query.
        //
        UnicodeValueBuffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                          UnicodeValueLength
                                        );
        if (UnicodeValueBuffer == NULL) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            InputLength = 0;

            if( IsLocalHandle( hKey )) {


                Error = (LONG)LocalBaseRegEnumValue (
                                    hKey,
                                    dwIndex,
                                    Name,
                                    &ValueType,
                                    (LPBYTE)UnicodeValueBuffer,
                                    &ValueLength,
                                    &InputLength
                                    );
                //
                //  Make sure that the local side didn't destroy the
                //  Buffer in the StaticUnicodeString
                //

                ASSERT((&NtCurrentTeb()->StaticUnicodeString)->Buffer);

            } else {
                if (Win95Server) {
                    LPBYTE lpWin95Data;
                    //
                    // This is a Win95 server.
                    // Allocate a new buffer that is two bytes larger than
                    // the old one so they can trash the last two bytes.
                    //
                    lpWin95Data = RtlAllocateHeap(RtlProcessHeap(),
                                                  0,
                                                  ValueLength+sizeof(WCHAR));
                    if (lpWin95Data == NULL) {
                        Error = ERROR_NOT_ENOUGH_MEMORY;
                    } else {
                        Error = (LONG)BaseRegEnumValue (DereferenceRemoteHandle( hKey ),
                                                        dwIndex,
                                                        Name,
                                                        &ValueType,
                                                        lpWin95Data,
                                                        &ValueLength,
                                                        &InputLength);
                        if (Error == ERROR_SUCCESS) {
                            if ((ValueType == REG_SZ) ||
                                (ValueType == REG_MULTI_SZ) ||
                                (ValueType == REG_EXPAND_SZ)) {
                                //
                                // The returned length is one WCHAR too large
                                // and the last two bytes of the buffer are trashed.
                                //
                                ValueLength -= sizeof(WCHAR);
                            }
                            CopyMemory(UnicodeValueBuffer, lpWin95Data, ValueLength);
                        }
                        RtlFreeHeap(RtlProcessHeap(),0,lpWin95Data);
                    }

                } else {
                    Error = (LONG)BaseRegEnumValue (DereferenceRemoteHandle( hKey ),
                                                    dwIndex,
                                                    Name,
                                                    &ValueType,
                                                    (LPBYTE)UnicodeValueBuffer,
                                                    &ValueLength,
                                                    &InputLength);
                }
            }
            // Compute needed buffer size , cbAnsi will keeps the byte
            // counts to keep MBCS string after following step.

            RtlUnicodeToMultiByteSize( &cbAnsi ,
                                       UnicodeValueBuffer ,
                                       ValueLength );

            // If we could not store all MBCS string to buffer that
            // Apps gives me. We set ERROR_MORE_DATA to Error

            if( ARGUMENT_PRESENT( lpcbData ) ) {
                if( cbAnsi > *lpcbData && lpData != NULL ) {
                    Error = ERROR_MORE_DATA;
                }
            }
        }

        if ((Error == ERROR_SUCCESS) && (AnsiValueBuffer != NULL)) {

            //
            // We have a UNICODE value, so translate it to ANSI in the callers
            // buffer.  In the case where the caller's buffer was big enough
            // for the UNICODE version, we do the conversion in place, which
            // works since the ANSI version is smaller than the UNICODE version.
            //


            Index = 0;
            Status = RtlUnicodeToMultiByteN( AnsiValueBuffer,
                                             AnsiValueLength,
                                             &Index,
                                             UnicodeValueBuffer,
                                             UnicodeValueLength
                                           );

            if (!NT_SUCCESS( Status )) {
                Error = RtlNtStatusToDosError( Status );
            }
            cbAnsi = Index;
        }

        //
        // Free the unicode buffer if it was successfully allocated
        //
        if (UnicodeValueBuffer != NULL) {
            RtlFreeHeap( RtlProcessHeap(), 0, UnicodeValueBuffer );
        }

        //
        // Return the length of the ANSI version to the caller.
        //

        ValueLength = cbAnsi;

        //
        // Special hack to help out all the idiots who
        // believe the length of a NULL terminated string is
        // strlen(foo) instead of strlen(foo) + 1.
        // If the last character of the buffer is not a NULL
        // and there is enough space left in the caller's buffer,
        // slap a NULL in there to prevent him from going nuts
        // trying to do a strlen().
        //
        if (ARGUMENT_PRESENT( lpData ) &&
            (*lpcbData > ValueLength)  &&
            (ValueLength > 0) &&
            (lpData[ValueLength-1] != '\0')) {

            lpData[ValueLength] = '\0';
        }
    }

    //
    // Return the value type and data length if requested and we have it.
    //

    if (Error == ERROR_SUCCESS || Error == ERROR_MORE_DATA) {

        if (lpcbData != NULL) {
            *lpcbData = ValueLength;
        }

        if (lpType != NULL) {
            *lpType = ValueType;
        }
    }

    //
    // If the information was not succesfully queried return the error.
    //

    if( Error != ERROR_SUCCESS ) {
        goto ExitCleanup;
    }


    //
    //  Subtract the NULL from the Length. This was added by the server
    //  so that RPC would transmit it.
    //

    if ( Name->Length > 0 ) {
        Name->Length -= sizeof( UNICODE_NULL );
    }

    //
    // Convert the name to ANSI.
    //

    AnsiString.MaximumLength    = ( USHORT ) *lpcbValueName;
    AnsiString.Buffer           = lpValueName;

    Status = RtlUnicodeStringToAnsiString(
                &AnsiString,
                Name,
                FALSE
                );


    //
    // If the name conversion failed, map and return the error.
    //

    if( ! NT_SUCCESS( Status )) {


        Error = RtlNtStatusToDosError( Status );
        goto ExitCleanup;
    }

    //
    // Update the name length return parameter.
    //

    *lpcbValueName = AnsiString.Length;


ExitCleanup:
    
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}



LONG
RegEnumValueW (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )

/*++

Routine Description:

    Win32 Unicode RPC wrapper for enumerating values.

--*/

{
    UNICODE_STRING      Name;
    LONG                Error;
    DWORD               InputLength;
    DWORD               ValueLength;
    DWORD               ValueType;
    HKEY                TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Validate dependency between lpData and lpcbData parameters.
    //

    if( ARGUMENT_PRESENT( lpReserved ) ||
        (ARGUMENT_PRESENT( lpData ) && ( ! ARGUMENT_PRESENT( lpcbData )))) {
        return ERROR_INVALID_PARAMETER;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    Name.Length           = 0;
    Name.MaximumLength    = ( USHORT )( *lpcbValueName << 1 );
    Name.Buffer           = lpValueName;

    //
    // Call the Base API passing it a pointer to the counted Unicode
    // string for the name and return the results. Note that zero bytes
    // are transmitted (i.e.InputLength = 0) for the data.
    //

    InputLength = 0;
    ValueLength = ( ARGUMENT_PRESENT( lpcbData ) )? *lpcbData : 0;

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegEnumValue (
                            hKey,
                            dwIndex,
                            &Name,
                            &ValueType,
                            lpData,
                            &ValueLength,
                            &InputLength
                            );
    } else {
        DWORD dwVersion;

        if (IsWin95Server(DereferenceRemoteHandle(hKey),dwVersion)) {
            LPBYTE lpWin95Data;
            //
            // This is a Win95 server.
            // Allocate a new buffer that is two bytes larger than
            // the old one so they can trash the last two bytes.
            //
            lpWin95Data = RtlAllocateHeap(RtlProcessHeap(),
                                          0,
                                          ValueLength+sizeof(WCHAR));
            if (lpWin95Data == NULL) {
                Error = ERROR_NOT_ENOUGH_MEMORY;
            } else {
                Error = (LONG)BaseRegEnumValue (DereferenceRemoteHandle( hKey ),
                                                dwIndex,
                                                &Name,
                                                &ValueType,
                                                lpWin95Data,
                                                &ValueLength,
                                                &InputLength);
                if (Error == ERROR_SUCCESS) {
                    if ((ValueType == REG_SZ) ||
                        (ValueType == REG_MULTI_SZ) ||
                        (ValueType == REG_EXPAND_SZ)) {
                        //
                        // The returned length is one WCHAR too large
                        // and the last two bytes of the buffer are trashed.
                        //
                        ValueLength -= sizeof(WCHAR);
                    }
                    CopyMemory(lpData, lpWin95Data, ValueLength);
                }
                RtlFreeHeap(RtlProcessHeap(),0,lpWin95Data);
            }

        } else {
            Error = (LONG)BaseRegEnumValue (DereferenceRemoteHandle( hKey ),
                                            dwIndex,
                                            &Name,
                                            &ValueType,
                                            lpData,
                                            &ValueLength,
                                            &InputLength);
        }
    }
    //
    // Special hack to help out all the idiots who
    // believe the length of a NULL terminated string is
    // strlen(foo) instead of strlen(foo) + 1.
    // If the last character of the buffer is not a NULL
    // and there is enough space left in the caller's buffer,
    // slap a NULL in there to prevent him from going nuts
    // trying to do a strlen().
    //
    if ( (Error == ERROR_SUCCESS) &&
         ARGUMENT_PRESENT( lpData ) &&
         ( (ValueType == REG_SZ) ||
           (ValueType == REG_EXPAND_SZ) ||
           (ValueType == REG_MULTI_SZ)) &&
         ( ValueLength > sizeof(WCHAR))) {

        UNALIGNED WCHAR *String = (UNALIGNED WCHAR *)lpData;
        DWORD Length = ValueLength/sizeof(WCHAR);

        if ((String[Length-1] != UNICODE_NULL) &&
            (ValueLength+sizeof(WCHAR) <= *lpcbData)) {
            String[Length] = UNICODE_NULL;
        }
    }

    //
    // Don't count the NUL.
    //
    if( Name.Length != 0 ) {
        *lpcbValueName = ( Name.Length >> 1 ) - 1;
    }

    if( ARGUMENT_PRESENT( lpcbData ) ) {
        *lpcbData = ValueLength;
    }
    if ( ARGUMENT_PRESENT( lpType )) {
        *lpType = ValueType;
    }

ExitCleanup:
    
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}
