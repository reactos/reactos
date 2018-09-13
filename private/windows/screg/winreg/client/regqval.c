/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regqval.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    query value APIs.  That is:

        - RegQueryValueA
        - RegQueryValueW
        - RegQueryValueExA
        - RegQueryValueExW

Author:

    David J. Gilman (davegi) 18-Mar-1992

Notes:

    See the notes in server\regqval.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"

LONG
RegQueryValueA (
    HKEY hKey,
    LPCSTR lpSubKey,
    LPSTR lpData,
    PLONG lpcbData
    )

/*++

Routine Description:


    Win 3.1 ANSI RPC wrapper for querying a value.

--*/

{
    HKEY            ChildKey;
    LONG            Error;
    DWORD           ValueType;
    LONG            InitialCbData;
    HKEY            TempHandle = NULL;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // If the sub-key is NULL or points to an empty string then the value is
    // to be queried from this key (i.e.  hKey) otherwise the sub-key needs
    // to be opened.
    //

    if(( lpSubKey == NULL ) || ( *lpSubKey == '\0' )) {

        ChildKey = hKey;

    } else {

        //
        // The sub-key was supplied so impersonate the
        // client and attempt to open it.
        //

        Error = RegOpenKeyExA(
                    hKey,
                    lpSubKey,
                    0,
                    KEY_QUERY_VALUE,
                    &ChildKey
                    );

        if( Error != ERROR_SUCCESS ) {
            goto ExitCleanup;
        }
    }

    InitialCbData = ARGUMENT_PRESENT(lpcbData) ? (*lpcbData) : 0;

    //
    // ChildKey contains an HKEY which may be the one supplied (hKey) or
    // returned from RegOpenKeyExA. Query the value using the special value
    // name NULL.
    //

    Error = RegQueryValueExA(
                ChildKey,
                NULL,
                NULL,
                &ValueType,
                lpData,
                lpcbData
                );
    //
    // If the sub key was opened, close it.
    //

    if( ChildKey != hKey ) {

        if( IsLocalHandle( ChildKey )) {

            LocalBaseRegCloseKey( &ChildKey );

        } else {

            ChildKey = DereferenceRemoteHandle( ChildKey );
            BaseRegCloseKey( &ChildKey );
        }
    }

    //
    // If the type of the value is not a null terminate string, then return
    // an error. (Win 3.1 compatibility)
    //

    if (!Error && ((ValueType != REG_SZ) && (ValueType != REG_EXPAND_SZ))) {
        Error = ERROR_INVALID_DATA;
    }

    //
    // If value doesn't exist, return ERROR_SUCCESS and an empty string.
    // (Win 3.1 compatibility)
    //
    if( Error == ERROR_FILE_NOT_FOUND ) {
        if( ARGUMENT_PRESENT( lpcbData ) ) {
            *lpcbData = sizeof( CHAR );
        }
        if( ARGUMENT_PRESENT( lpData ) ) {
            *lpData = '\0';
        }
        Error = ERROR_SUCCESS;
    }

    //
    // Expand if necessary (VB compatibility)
    //

    if (!Error && (ValueType == REG_EXPAND_SZ)) {
        if ( (!ARGUMENT_PRESENT(lpcbData)) || (!ARGUMENT_PRESENT(lpData)) ) {
            Error = ERROR_INVALID_DATA;
        } else {
            LPSTR ExpandBuffer;
            LONG ExpandedSize;
            LONG BufferSize = (InitialCbData>*lpcbData)?InitialCbData:*lpcbData;
            //
            // if InitialCbData was 0, allocate a buffer of the real size
            //
            ExpandBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, BufferSize);
            if (ExpandBuffer == NULL) {
                Error = ERROR_NOT_ENOUGH_MEMORY;
            } else {
                RtlCopyMemory(ExpandBuffer, lpData, *lpcbData);
                ExpandedSize = ExpandEnvironmentStringsA(ExpandBuffer, lpData, BufferSize);
                if (ExpandedSize > InitialCbData) {
                    Error = ERROR_MORE_DATA;
                }
                *lpcbData = ExpandedSize;
                RtlFreeHeap( RtlProcessHeap(), 0, ExpandBuffer );
            }
        }
    }

    //
    // Return the results of querying the value.
    //

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

LONG
RegQueryValueW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    LPWSTR lpData,
    PLONG  lpcbData
    )

/*++

Routine Description:

    Win 3.1 Unicode RPC wrapper for querying a value.

--*/

{
    HKEY        ChildKey;
    LONG        Error;
    DWORD       ValueType;
    LONG            InitialCbData;
    HKEY            TempHandle = NULL;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }


    //
    // If the sub-key is NULL or points to an empty string then the value is
    // to be queried from this key (i.e.  hKey) otherwise the sub-key needs
    // to be opened.
    //

    if(( lpSubKey == NULL ) || ( *lpSubKey == '\0' )) {

        ChildKey = hKey;

    } else {

        //
        // The sub-key was supplied so attempt to open it.
        //

        Error = RegOpenKeyExW(
                    hKey,
                    lpSubKey,
                    0,
                    KEY_QUERY_VALUE,
                    &ChildKey
                    );

        if( Error != ERROR_SUCCESS ) {
            goto ExitCleanup;
        }
    }

    InitialCbData = ARGUMENT_PRESENT(lpcbData) ? (*lpcbData) : 0;

    //
    // ChildKey contains an HKEY which may be the one supplied (hKey) or
    // returned from RegOpenKeyExA. Query the value using the special value
    // name NULL.
    //

    Error = RegQueryValueExW(
                ChildKey,
                NULL,
                NULL,
                &ValueType,
                ( LPBYTE )lpData,
                lpcbData
                );
    //
    // If the sub key was opened, close it.
    //

    if( ChildKey != hKey ) {

        if( IsLocalHandle( ChildKey )) {

            LocalBaseRegCloseKey( &ChildKey );

        } else {

            ChildKey = DereferenceRemoteHandle( ChildKey );
            BaseRegCloseKey( &ChildKey );
        }
    }

    //
    // If the type of the value is not a null terminate string, then return
    // an error. (Win 3.1 compatibility)
    //

    if (!Error && ((ValueType != REG_SZ) && (ValueType != REG_EXPAND_SZ))) {
        Error = ERROR_INVALID_DATA;
    }

    //
    // If value doesn't exist, return ERROR_SUCCESS and an empty string.
    // (Win 3.1 compatibility)
    //
    if( Error == ERROR_FILE_NOT_FOUND ) {
        if( ARGUMENT_PRESENT( lpcbData ) ) {
            *lpcbData = sizeof( WCHAR );
        }
        if( ARGUMENT_PRESENT( lpData ) ) {
            *lpData = ( WCHAR )'\0';
        }
        Error = ERROR_SUCCESS;
    }

    //
    // Expand if necessary (VB compatibility)
    //

    if (!Error && (ValueType == REG_EXPAND_SZ)) {
        if ( (!ARGUMENT_PRESENT(lpcbData)) || (!ARGUMENT_PRESENT(lpData)) ) {
            Error = ERROR_INVALID_DATA;
        } else {
            LPWSTR ExpandBuffer;
            LONG ExpandedSize;
            LONG BufferSize = (InitialCbData>*lpcbData)?InitialCbData:*lpcbData;
            //
            // if InitialCbData was 0, allocate a buffer of the real size
            //
            ExpandBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, BufferSize);
            if (ExpandBuffer == NULL) {
                Error = ERROR_NOT_ENOUGH_MEMORY;
            } else {
                
                RtlCopyMemory(ExpandBuffer, lpData, *lpcbData);
                ExpandedSize = ExpandEnvironmentStringsW(ExpandBuffer, lpData, BufferSize / sizeof(WCHAR));
                if (ExpandedSize > (LONG)(InitialCbData / sizeof(WCHAR))) {
                    Error = ERROR_MORE_DATA;
                }
                *lpcbData = ExpandedSize;
                RtlFreeHeap( RtlProcessHeap(), 0, ExpandBuffer );
            }
        }
    }

    //
    // Return the results of querying the value.
    //

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}


LONG
APIENTRY
RegQueryValueExA (
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpdwType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )

/*++

Routine Description:

    Win32 ANSI RPC wrapper for querying a value.

    RegQueryValueExA converts the lpValueName argument to a counted Unicode
    string and then calls BaseRegQueryValue.

--*/

{
    PUNICODE_STRING     ValueName;
    UNICODE_STRING      StubValueName;
    DWORD               ValueType;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    LONG                Error;
    DWORD               ValueLength;
    DWORD               InputLength;
    PWSTR               UnicodeValueBuffer;
    ULONG               UnicodeValueLength;

    PSTR                AnsiValueBuffer;
    ULONG               AnsiValueLength;
    ULONG               Index;
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

    ConvertKey(&hKey);
    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the value name to a counted Unicode string using the static
    // Unicode string in the TEB.
    //

    StubValueName.Buffer = NULL;
    ValueName = &NtCurrentTeb( )->StaticUnicodeString;
    ASSERT( ValueName != NULL );
    RtlInitAnsiString( &AnsiString, lpValueName );
    Status = RtlAnsiStringToUnicodeString(
                ValueName,
                &AnsiString,
                FALSE
                );

    if( ! NT_SUCCESS( Status )) {
        //
        // The StaticUnicodeString is not long enough; Try to allocate a bigger one
        //
        Status = RtlAnsiStringToUnicodeString(
                    &StubValueName,
                    &AnsiString,
                    TRUE
                    );
        if( ! NT_SUCCESS( Status )) {
            Error = RtlNtStatusToDosError( Status );
            goto ExitCleanup;
        }

        ValueName = &StubValueName;
    }

    //
    //  Add the terminating NULL to the Length so that RPC transmits
    //  it.
    //

    ValueName->Length += sizeof( UNICODE_NULL );

    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings. Note that zero bytes are transmitted (i.e.
    // InputLength = 0) for the data.
    //

    ValueLength = ARGUMENT_PRESENT( lpcbData )? *lpcbData : 0;
    InputLength = 0;

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegQueryValue (
                             hKey,
                             ValueName,
                             &ValueType,
                             lpData,
                             &ValueLength,
                             &InputLength
                             );
        //
        //  Make sure that the local side didn't destroy the Buffer in
        //  the StaticUnicodeString
        //
        ASSERT( ValueName->Buffer );


    } else {

        Error = (LONG)BaseRegQueryValue (
                             DereferenceRemoteHandle( hKey ),
                             ValueName,
                             &ValueType,
                             lpData,
                             &ValueLength,
                             &InputLength
                             );
    }

    //
    // If no error or callers buffer too small, and type is one of the null
    // terminated string types, then do the UNICODE to ANSI translation.
    // We handle the buffer too small case, because the callers buffer may
    // be big enough for the ANSI representation, but not the UNICODE one.
    // In this case, we need to allocate a buffer big enough, do the query
    // again and then the translation into the callers buffer.  We only do
    // this if the caller actually wants the value data (lpData != NULL)
    //

    if ((Error == ERROR_SUCCESS || Error == ERROR_MORE_DATA) &&
        (ARGUMENT_PRESENT( lpData ) || ARGUMENT_PRESENT( lpcbData ))&&
        (ValueType == REG_SZ ||
         ValueType == REG_EXPAND_SZ ||
         ValueType == REG_MULTI_SZ)
		 ) {
		UnicodeValueLength         = ValueLength;


		AnsiValueBuffer            = lpData;
		AnsiValueLength            = ARGUMENT_PRESENT( lpcbData )?
													 *lpcbData : 0;

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

				//
				//  Add the terminating NULL to the Length
				//  (remember that in the local case, ValueName->Length
				//  was decremented by sizeof( UNICODE_NULL ) in the first
				//  call to LocalBaseRegQueryValue).
				//  This won't happen in the remote case, since the
				//  server side will decrement ValueName->Length on
				//  the transmitted structure (a copy of ValueName), and
				//  the new Valuename->Length won't be transmitted back to
				//  the client.
				//

				ValueName->Length += sizeof( UNICODE_NULL );


				Error = (LONG)LocalBaseRegQueryValue (
									 hKey,
									 ValueName,
									 &ValueType,
									 (LPBYTE)UnicodeValueBuffer,
									 &ValueLength,
									 &InputLength
									 );
				//
				//  Make sure that the local side didn't destroy the
				//  Buffer in the StaticUnicodeString
				//

				ASSERT(ValueName->Buffer);


			} else {

				Error = (LONG)BaseRegQueryValue (
									 DereferenceRemoteHandle( hKey ),
									 ValueName,
									 &ValueType,
									 (LPBYTE)UnicodeValueBuffer,
									 &ValueLength,
									 &InputLength
									 );
			}
			// Compute needed buffer size , cbAnsi will keeps the byte
			// counts to keep MBCS string after following step.

			RtlUnicodeToMultiByteSize( &cbAnsi ,
									   UnicodeValueBuffer ,
									   ValueLength );

			// If we could not store all MBCS string to buffer that
			// Apps gives me.  We set ERROR_MORE_DATA to Error

			if( ARGUMENT_PRESENT( lpcbData ) ) {
				if( cbAnsi > *lpcbData && lpData != NULL ) {
					Error = ERROR_MORE_DATA;
				}
			}
		}

		if ((Error == ERROR_SUCCESS) && (AnsiValueBuffer != NULL) ) {
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

			// Now Index keeps Byte counts of MBCS string in AnsiValueBuffer
			cbAnsi = Index;
		}

		//
		// Free the buffer if it was successfully allocated
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
    // Stored the returned length in the caller specified location and
    // return the error code.
    //

    if (lpdwType != NULL) {
        *lpdwType = ValueType;
    }

    if( ARGUMENT_PRESENT( lpcbData ) ) {
        *lpcbData = ValueLength;
    }

    //
    // Free the temporary Unicode string stub allocated for the ValueName
    //
    RtlFreeUnicodeString(&StubValueName);

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}


LONG
APIENTRY
RegQueryValueExW (
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpdwType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )

/*++

Routine Description:

    Win32 Unicode RPC wrapper for querying a value.

    RegQueryValueExW converts the lpValueName argument to a counted Unicode
    string and then calls BaseRegQueryValue.

--*/

{
    UNICODE_STRING      ValueName;
    DWORD               InputLength;
    DWORD               ValueLength;
    DWORD               ValueType;
    LONG                Error;
    UNALIGNED WCHAR     *String;
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

    ConvertKey(&hKey);
    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the value name to a counted Unicode string.
    //

    RtlInitUnicodeString( &ValueName, lpValueName );

    //
    //  Add the terminating NULL to the Length so that RPC transmits
    //  it.
    //

    ValueName.Length += sizeof( UNICODE_NULL );

    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings. Note that zero bytes are transmitted (i.e.
    // InputLength = 0) for the data.
    //
    InputLength = 0;
    ValueLength = ( ARGUMENT_PRESENT( lpcbData ) )? *lpcbData : 0;

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegQueryValue (
                            hKey,
                            &ValueName,
                            &ValueType,
                            lpData,
                            &ValueLength,
                            &InputLength
                            );
    } else {

        Error =  (LONG)BaseRegQueryValue (
                            DereferenceRemoteHandle( hKey ),
                            &ValueName,
                            &ValueType,
                            lpData,
                            &ValueLength,
                            &InputLength
                            );
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
           (ValueType==REG_MULTI_SZ)) &&
         ( ValueLength > sizeof(WCHAR))) {

        UNALIGNED WCHAR *String = (UNALIGNED WCHAR *)lpData;
        DWORD Length = ValueLength/sizeof(WCHAR);

        if ((String[Length-1] != UNICODE_NULL) &&
            (ValueLength+sizeof(WCHAR) <= *lpcbData)) {
            String[Length] = UNICODE_NULL;
        }
    }
    if( ARGUMENT_PRESENT( lpcbData ) ) {
        *lpcbData = ValueLength;
    }
    if ( ARGUMENT_PRESENT( lpdwType )) {
        *lpdwType = ValueType;
    }

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}
