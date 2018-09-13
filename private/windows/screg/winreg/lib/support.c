/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    Support.c

Abstract:

    This module contains support routines for the Win32 Registry API.

Author:

    David J. Gilman (davegi) 15-Nov-1991

--*/

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    error.c

Abstract:

    This module contains a routine for converting NT status codes
    to DOS/OS|2 error codes.

Author:

    David Treadwell (davidtr)   04-Apr-1991

Revision History:

--*/

#include <rpc.h>
#include "regrpc.h"
#include <stdio.h>



LONG
MapSAToRpcSA (
    IN LPSECURITY_ATTRIBUTES lpSA,
    OUT PRPC_SECURITY_ATTRIBUTES lpRpcSA
    )

/*++

Routine Description:

    Maps a SECURITY_ATTRIBUTES structure to a RPC_SECURITY_ATTRIBUTES
    structure by converting the SECURITY_DESCRIPTOR to a form where it can
    be marshalled/unmarshalled.

Arguments:

    lpSA - Supplies a pointer to the SECURITY_ATTRIBUTES structure to be
        converted.

    lpRpcSA - Supplies a pointer to the converted RPC_SECURITY_ATTRIBUTES
        structure.  The caller should free (using RtlFreeHeap) the field
        lpSecurityDescriptor when its finished using it.

Return Value:

    LONG - Returns ERROR_SUCCESS if the SECURITY_ATTRIBUTES is
        succesfully mapped.

--*/

{
    LONG    Error;

    ASSERT( lpSA != NULL );
    ASSERT( lpRpcSA != NULL );

    //
    // Map the SECURITY_DESCRIPTOR to a RPC_SECURITY_DESCRIPTOR.
    //
    lpRpcSA->RpcSecurityDescriptor.lpSecurityDescriptor = NULL;

    if( lpSA->lpSecurityDescriptor != NULL ) {
        Error = MapSDToRpcSD(
                    lpSA->lpSecurityDescriptor,
                    &lpRpcSA->RpcSecurityDescriptor
                    );
    } else {
        lpRpcSA->RpcSecurityDescriptor.cbInSecurityDescriptor = 0;
        lpRpcSA->RpcSecurityDescriptor.cbOutSecurityDescriptor = 0;
        Error = ERROR_SUCCESS;
    }

    if( Error == ERROR_SUCCESS ) {

        //
        //
        // The supplied SECURITY_DESCRIPTOR was successfully converted
        // to self relative format so assign the remaining fields.
        //

        lpRpcSA->nLength = lpSA->nLength;

        lpRpcSA->bInheritHandle = ( BOOLEAN ) lpSA->bInheritHandle;
    }

    return Error;
}

LONG
MapSDToRpcSD (
    IN  PSECURITY_DESCRIPTOR lpSD,
    IN OUT PRPC_SECURITY_DESCRIPTOR lpRpcSD
    )

/*++

Routine Description:

    Maps a SECURITY_DESCRIPTOR to a RPC_SECURITY_DESCRIPTOR by converting
    it to a form where it can be marshalled/unmarshalled.

Arguments:

    lpSD - Supplies a pointer to the SECURITY_DESCRIPTOR
        structure to be converted.

    lpRpcSD - Supplies a pointer to the converted RPC_SECURITY_DESCRIPTOR
        structure. Memory for the security descriptor is allocated if
        not provided. The caller must take care of freeing up the memory
        if necessary.

Return Value:

    LONG - Returns ERROR_SUCCESS if the SECURITY_DESCRIPTOR is
        succesfully mapped.

--*/

{
    DWORD   cbLen;


    ASSERT( lpSD != NULL );
    ASSERT( lpRpcSD != NULL );

    if( RtlValidSecurityDescriptor( lpSD )) {

        cbLen = RtlLengthSecurityDescriptor( lpSD );
        ASSERT( cbLen > 0 );

        //
        //  If we're not provided a buffer for the security descriptor,
        //  allocate it.
        //
        if ( !lpRpcSD->lpSecurityDescriptor ) {

            //
            // Allocate space for the converted SECURITY_DESCRIPTOR.
            //
            lpRpcSD->lpSecurityDescriptor =
                 ( PBYTE ) RtlAllocateHeap(
                                RtlProcessHeap( ), 0,
                                cbLen
                                );

            //
            // If the memory allocation failed, return.
            //
            if( lpRpcSD->lpSecurityDescriptor == NULL ) {
                return ERROR_OUTOFMEMORY;
            }

            lpRpcSD->cbInSecurityDescriptor = cbLen;

        } else {

            //
            //  Make sure that the buffer provided is big enough
            //
            if ( lpRpcSD->cbInSecurityDescriptor < cbLen ) {
                return ERROR_OUTOFMEMORY;
            }
        }

        //
        //  Set the size of the transmittable buffer
        //
        lpRpcSD->cbOutSecurityDescriptor = cbLen;

        //
        // Convert the supplied SECURITY_DESCRIPTOR to self relative form.
        //

        return RtlNtStatusToDosError(
            RtlMakeSelfRelativeSD(
                        lpSD,
                        lpRpcSD->lpSecurityDescriptor,
                        &lpRpcSD->cbInSecurityDescriptor
                        )
                    );
    } else {

        //
        // The supplied SECURITY_DESCRIPTOR is invalid.
        //

        return ERROR_INVALID_PARAMETER;
    }
}
