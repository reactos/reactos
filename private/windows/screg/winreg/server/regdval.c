/*++


Copyright (c) 1991  Microsoft Corporation

Module Name:

    Regdval.c

Abstract:

    This module contains the server side implementation for the Win32
    Registry API to delete values from keys. That is:

        - BaseRegDeleteValue

Author:

    David J. Gilman (davegi) 15-Nov-1991

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
BaseRegDeleteValue (
    HKEY hKey,
    PUNICODE_STRING lpValueName
    )

/*++

Routine Description:

    Remove the named value from the specified key.

Arguments:

    hKey - Supplies a handle to the open key.

    lpValueName - Supplies the name of the value to remove.  lpValueName
        may be NULL.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

Notes:

    hKey must have been opened for KEY_SET_VALUE.

--*/

{
    HKEY                            hkDelete;
    NTSTATUS                        Status;
#ifdef LOCAL

    HKEY                            hkUserClasses;
    HKEY                            hkMachineClasses;

    hkUserClasses = NULL;
    hkMachineClasses = NULL;

#endif

    hkDelete = hKey;

    //
    //  Subtract the NULL from the string length. This was added
    //  by the client so that RPC would transmit the whole thing.
    //
    lpValueName->Length -= sizeof( UNICODE_NULL );

    //
    // Call the Nt Api to delete the value, map the NTSTATUS code to a
    // Win32 Registry error code and return.
    //

#ifdef LOCAL
    if (gpfnTermsrvDeleteValue) {
        //
        // Remove the value from the Terminal Server registry tracking database
        //
        gpfnTermsrvDeleteValue(hKey, lpValueName);
    }
#endif

#ifdef LOCAL
    if (REG_CLASS_IS_SPECIAL_KEY(hKey)) {

        Status = BaseRegGetUserAndMachineClass(
            NULL,
            hkDelete,
            MAXIMUM_ALLOWED,
            &hkMachineClasses,
            &hkUserClasses);

        if (!NT_SUCCESS(Status)) {
            return (error_status_t)RtlNtStatusToDosError(Status);
        }
    }

    if (hkUserClasses && hkMachineClasses) {
        hkDelete = hkUserClasses;
    }
#endif

    Status = NtDeleteValueKey(
        hkDelete,
        lpValueName
        );

#ifdef LOCAL
    
    //
    // For class keys, try again with machine if there were
    // two keys of the same name
    //
    if (REG_CLASS_IS_SPECIAL_KEY(hKey)) {
        
        if ((STATUS_OBJECT_NAME_NOT_FOUND == Status) && 
            (hkUserClasses && hkMachineClasses)) {

            Status = NtDeleteValueKey(
                hkMachineClasses,
                lpValueName
                );
        }
    }

    if (hkUserClasses && hkMachineClasses) {
        if (hkUserClasses != hKey) {
            NtClose(hkUserClasses);
        } else {
            NtClose(hkMachineClasses);
        }
    }
#endif // LOCAL

    return (error_status_t)RtlNtStatusToDosError(Status);

}




