/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regsckey.c

Abstract:

    This module contains the server side implementation for the Win32
    Registry APIs to set and get the SECURITY_DESCRIPTOR for a key.  That
    is:

        - BaseRegGetKeySecurity
        - BaseRegSetKeySecurity

Author:

    David J. Gilman (davegi) 10-Feb-1992

Notes:

    See the Notes in Regkey.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"
#ifdef LOCAL
#include "tsappcmp.h"
#endif


error_status_t
BaseRegGetKeySecurity(
    HKEY hKey,
    SECURITY_INFORMATION RequestedInformation,
    PRPC_SECURITY_DESCRIPTOR pRpcSecurityDescriptor
    )

/*++

Routine Description:

    This API returns a copy of the security descriptor protecting a
    previously opened key.  Based on the caller's access rights and
    privileges, this API returns a security descriptor containing the
    requested security descriptor fields.  To read the supplied key's
    security descriptor the caller must be granted READ_CONTROL access or
    be the owner of the object.  In addition, the caller must have
    SeSecurityPrivilege privilege to read the system ACL.


Arguments:

    hKey - Supplies a handle to a previously opened key.

    SecurityInformation - Supplies the information needed to determine
        the type of security returned in the SECURITY_DESCRIPTOR.

    pSecurityDescriptor - Supplies a pointer to a buffer where the
        requested SECURITY_DESCRIPTOR will be written.

    lpcbSecurityDescriptor - Supplies a pointer to a DWORD which on input
        contains the size, in bytes, of the supplied SECURITY_DESCRIPTOR
        buffer. On output it contains the actual number of bytes required
        by the SECURITY_DESCRIPTOR.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

Notes:

    If the buffer size passed in is too small, the correct value will be
    returned through lpcbSecurityDescriptor and the API will return,
    ERROR_INVALID_PARAMETER.

--*/

{
    NTSTATUS                Status;
    PSECURITY_DESCRIPTOR    lpSD;
    DWORD                   cbLen;
    DWORD                   Error = ERROR_SUCCESS;
    HKEY                    hPerflibKey = 0;
    OBJECT_ATTRIBUTES       Obja;

    if (hKey == HKEY_PERFORMANCE_DATA ||
        hKey == HKEY_PERFORMANCE_TEXT ||
        hKey == HKEY_PERFORMANCE_NLSTEXT ) {
        //
        // For these special cases, get the hKey for Perflib
        // and return the Perflib's Security Info
        //
        UNICODE_STRING  PerflibSubKeyString;
        BOOL            bNeedSACL;

        bNeedSACL = RequestedInformation & SACL_SECURITY_INFORMATION;

        RtlInitUnicodeString (
            &PerflibSubKeyString,
            L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib");


        //
        // Initialize the OBJECT_ATTRIBUTES structure and open the key.
        //
        InitializeObjectAttributes(
            &Obja,
            &PerflibSubKeyString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );


        Status = NtOpenKey(
                &hPerflibKey,
                bNeedSACL ?
                    MAXIMUM_ALLOWED | ACCESS_SYSTEM_SECURITY :
                    MAXIMUM_ALLOWED,
                &Obja
                );

        if ( ! NT_SUCCESS( Status )) {

            Error = RtlNtStatusToDosError( Status );
            pRpcSecurityDescriptor->cbInSecurityDescriptor  = 0;
            pRpcSecurityDescriptor->cbOutSecurityDescriptor = 0;
            return (error_status_t)Error;
        }

        hKey = hPerflibKey;

    } else {
        ASSERT( IsPredefinedRegistryHandle( hKey ) == FALSE );
    }

    //
    //  Allocate space for the security descriptor
    //
    lpSD = (PSECURITY_DESCRIPTOR)
                RtlAllocateHeap(
                        RtlProcessHeap(), 0,
                        pRpcSecurityDescriptor->cbInSecurityDescriptor
                        );

    if ( !lpSD ) {

        Error = ERROR_OUTOFMEMORY;

    } else {

        Status = NtQuerySecurityObject(
                     hKey,
                     RequestedInformation,
                     lpSD,
                     pRpcSecurityDescriptor->cbInSecurityDescriptor,
                     &cbLen
                     );

        //
        // If the call fails, set the size of the buffer to zero so RPC
        // won't copy any data.
        //
        if( ! NT_SUCCESS( Status )) {

            Error = RtlNtStatusToDosError( Status );

        } else {

            //
            //  Convert the security descriptor to a Self-relative form
            //
            Error = MapSDToRpcSD (
                        lpSD,
                        pRpcSecurityDescriptor
                        );
        }

        if ( Error != ERROR_SUCCESS ) {
            pRpcSecurityDescriptor->cbInSecurityDescriptor  = cbLen;
            pRpcSecurityDescriptor->cbOutSecurityDescriptor = 0;
        }

        //
        //  Free the buffer that we allocated for the security descriptor
        //
        RtlFreeHeap(
                RtlProcessHeap(), 0,
                lpSD
                );
    }

    if (hPerflibKey) {
        // Close the Perflib that was created in the special cases
        NtClose(hPerflibKey);
    }

    return (error_status_t)Error;
}

error_status_t
BaseRegSetKeySecurity(
    HKEY hKey,
    SECURITY_INFORMATION SecurityInformation,
    PRPC_SECURITY_DESCRIPTOR pRpcSecurityDescriptor
    )

/*++

Routine Description:

    This API can be used to set the security of a previously opened key.
    This call is only successful if the following conditions are met:

    o If the key's owner or group is to be set, the caller must
      have WRITE_OWNER permission or have SeTakeOwnershipPrivilege.

    o If the key's DACL is to be set, the caller must have
      WRITE_DAC permission or be the object's owner.

    o If the key's SACL is to be set, the caller must have
      SeSecurityPrivilege.

Arguments:

    hKey - Supplies a handle to a previously opened key.

    SecurityInformation - Supplies a pointer to a SECURITY_INFORMATION
        structure that specifies the contents of the supplied
        SECURITY_DESCRIPTOR.

    pSecurityDescriptor - Supplies a pointer to the SECURITY_DESCRIPTOR
        to set on the supplied key.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    NTSTATUS    Status;

    if (hKey == HKEY_PERFORMANCE_DATA ||
        hKey == HKEY_PERFORMANCE_TEXT ||
        hKey == HKEY_PERFORMANCE_NLSTEXT ) {
        //
        // these keys get their security descriptor from
        // other "real" registry keys.
        //
        Status = STATUS_INVALID_HANDLE;
    } else {
        ASSERT( IsPredefinedRegistryHandle( hKey ) == FALSE );

        RPC_IMPERSONATE_CLIENT( NULL );

        Status = NtSetSecurityObject(
                    hKey,
                    SecurityInformation,
                    pRpcSecurityDescriptor->lpSecurityDescriptor
                    );

        RPC_REVERT_TO_SELF();
    }

#ifdef LOCAL
    if (NT_SUCCESS(Status) && gpfnTermsrvSetKeySecurity) {
        gpfnTermsrvSetKeySecurity(hKey,
                                  SecurityInformation,
                                  pRpcSecurityDescriptor->lpSecurityDescriptor);
    }
#endif

    return (error_status_t)RtlNtStatusToDosError( Status );
}
