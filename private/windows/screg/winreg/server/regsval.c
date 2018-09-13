/*++


Copyright (c) 1991  Microsoft Corporation

Module Name:

    Regsval.c

Abstract:

    This module contains the server side implementation for the Win32
    Registry set value API.  That is:

        - BaseRegSetValue
Author:

    David J. Gilman (davegi) 27-Nov-1991

Notes:

    See the Notes in Regkey.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"
#ifdef LOCAL
#include "tsappcmp.h"
#include "regclass.h"
#endif


error_status_t
BaseRegSetValue(
    HKEY hKey,
    PUNICODE_STRING lpValueName,
    DWORD dwType,
    LPBYTE lpData,
    DWORD cbData
    )

/*++

Routine Description:

    Set the type and value of an open key.  Changes are not committed
    until the key is flushed.  By "committed" we mean written to disk.
    Changes will be seen by subsequent queries as soon as this call
    returns.

Arguments:

    hKey - Supplies a handle to the open key.  Any of the predefined
        reserved handles or a previously opened key handle may be used for
        hKey.

    lpValueName - Supplies the name of the value to set.  If the ValueName
        is not present, it is added to the key.

    dwType - Supplies the type of information to be stored: REG_SZ, etc.

    lpData - supplies a pointer to a buffer containing the data to set for
        the value entry.

    cbData - Supplies the length (in bytes) of the information to be stored.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

Notes:

    A set may fail due to memory limits - any config entry must fit in
    main memory.  If successful, RegSetValue will set the type, contents,
    and length of the information stored at the specified key.
    KEY_SET_VALUE access is required.

--*/

{
    NTSTATUS   Status;    
    HKEY  hkSet;

#ifdef LOCAL
    PVOID     PreSetData  = NULL;

    HKEY                            hkUserClasses;
    HKEY                            hkMachineClasses;

    hkUserClasses = NULL;
    hkMachineClasses = NULL;

#endif


    hkSet = hKey;

    //
    //  Subtract the NULL from the Length. This was added on the
    //  client side so that RPC would transmit it.
    //

    if ( lpValueName->Length > 0 ) {
        lpValueName->Length -= sizeof( UNICODE_NULL );
    }
    if ((hKey == HKEY_PERFORMANCE_DATA) ||
        (hKey==HKEY_PERFORMANCE_TEXT) ||
        (hKey==HKEY_PERFORMANCE_NLSTEXT)) {
        return(PerfRegSetValue(hKey,
                               lpValueName->Buffer,
                               0,
                               dwType,
                               lpData,
                               cbData));
    }

#ifdef LOCAL

    if (gpfnTermsrvSetValueKey) {

        //
        // Find any pre-set values
        //
        
        Status = gpfnTermsrvGetPreSetValue( hKey,
                                            lpValueName,
                                            dwType,
                                            &PreSetData
                                            );
        
        //
        // Use the pre-set values if they exists
        //
        
        if ( NT_SUCCESS(Status) ) {
            lpData = (( PKEY_VALUE_PARTIAL_INFORMATION ) PreSetData )->Data;
            cbData = (( PKEY_VALUE_PARTIAL_INFORMATION ) PreSetData )->DataLength;
        }
        else {
            PreSetData = NULL;
        }
        
        //
        // Save the Master Copy
        //
        gpfnTermsrvSetValueKey(hKey,
                             lpValueName,
                             0,
                             dwType,
                             lpData,
                             cbData);
            
    }

    if ( PreSetData ) {

        //
        //  Set the value and free any data
        //

        Status = NtSetValueKey(
                       hKey,
                       lpValueName,
                       0,
                       dwType,
                       lpData,
                       cbData
                 );

        RtlFreeHeap( RtlProcessHeap( ), 0, PreSetData );

        return (error_status_t)RtlNtStatusToDosError( Status );
    }
    else
        //
        // No pre-set values, just do original code
        //

#endif

    //
    // Call the Nt API to set the value, map the NTSTATUS code to a
    // Win32 Registry error code and return.
    //

#ifdef LOCAL
    if (REG_CLASS_IS_SPECIAL_KEY(hKey)) {

        Status = BaseRegGetUserAndMachineClass(
            NULL,
            hkSet,
            MAXIMUM_ALLOWED,
            &hkMachineClasses,
            &hkUserClasses);

        if (!NT_SUCCESS(Status)) {
            return (error_status_t)RtlNtStatusToDosError(Status);
        }
    }

    if (hkUserClasses && hkMachineClasses) {
        hkSet = hkUserClasses;
    }
#endif

    Status = NtSetValueKey(
        hkSet,
        lpValueName,
        0,
        dwType,
        lpData,
        cbData
        );

#ifdef LOCAL
    if (hkUserClasses && hkMachineClasses) {
        if (hkUserClasses != hKey) {
            NtClose(hkUserClasses);
        } else {
            NtClose(hkMachineClasses);
        }
    }
#endif // LOCAL

    return (error_status_t) RtlNtStatusToDosError(Status);

}

