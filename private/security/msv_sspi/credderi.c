/*++

Copyright (c) 1987-1998  Microsoft Corporation

Module Name:

    credderi.c

Abstract:

    Interface to credential derivation facility.

Author:

    Scott Field (sfield)    14-Jan-1998

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#include "msp.h"
#include "nlp.h"

#include <sha.h>

#define HMAC_K_PADSIZE      (64)


//
// Prototype for credential derivation routines.
//

VOID
DeriveWithHMAC_SHA1(
    IN      PBYTE   pbKeyMaterial,
    IN      DWORD   cbKeyMaterial,
    IN      PBYTE   pbData,
    IN      DWORD   cbData,
    IN OUT  BYTE    rgbHMAC[A_SHA_DIGEST_LEN]   // output buffer
    );



NTSTATUS
MspNtDeriveCredential(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of MsV1_0DeriveCredential.

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.


--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PMSV1_0_DERIVECRED_REQUEST DeriveCredRequest;
    PMSV1_0_DERIVECRED_RESPONSE DeriveCredResponse;
    CLIENT_BUFFER_DESC ClientBufferDesc;

    PMSV1_0_PRIMARY_CREDENTIAL Credential = NULL;

    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );
    *ProtocolStatus = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ClientBufferBase);

    //
    // Ensure the specified Submit Buffer is of reasonable size and
    // relocate all of the pointers to be relative to the LSA allocated
    // buffer.
    //

    if ( SubmitBufferSize < sizeof(MSV1_0_DERIVECRED_REQUEST) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    DeriveCredRequest = (PMSV1_0_DERIVECRED_REQUEST) ProtocolSubmitBuffer;

    //
    // validate supported derive types.
    //

    if( DeriveCredRequest->DeriveCredType != MSV1_0_DERIVECRED_TYPE_SHA1 ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // caller must pass in mixing bits into submit buffer.
    //

    if( DeriveCredRequest->DeriveCredInfoLength == 0 ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Make sure the buffer fits in the supplied size
    //

    if ( (DeriveCredRequest->DeriveCredInfoLength + sizeof(MSV1_0_DERIVECRED_REQUEST))
            > SubmitBufferSize )
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Get the OWF password for this session.
    //

    Status = NlpGetPrimaryCredential( &DeriveCredRequest->LogonId, &Credential, NULL );

    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    //
    // Allocate a buffer to return to the caller.
    //

    *ReturnBufferSize = sizeof(MSV1_0_DERIVECRED_RESPONSE) +
                        A_SHA_DIGEST_LEN;

    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      *ReturnBufferSize,
                                      *ReturnBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    ZeroMemory( ClientBufferDesc.MsvBuffer, *ReturnBufferSize );
    DeriveCredResponse = (PMSV1_0_DERIVECRED_RESPONSE) ClientBufferDesc.MsvBuffer;

    //
    // Fill in the return buffer.
    //

    DeriveCredResponse->MessageType = MsV1_0DeriveCredential;
    DeriveCredResponse->DeriveCredInfoLength = A_SHA_DIGEST_LEN;

    //
    // derive credential from HMAC_SHA1 crypto primitive
    // (the only supported crypto primitive at the moment)
    //

    DeriveWithHMAC_SHA1(
            (PBYTE) &(Credential->NtOwfPassword),   // key material is NT OWF
            sizeof( NT_OWF_PASSWORD ),
            DeriveCredRequest->DeriveCredSubmitBuffer,
            DeriveCredRequest->DeriveCredInfoLength,
            DeriveCredResponse->DeriveCredReturnBuffer
            );


    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   ProtocolReturnBuffer );


Cleanup:

    if ( Credential != NULL ) {
        ZeroMemory( &(Credential->NtOwfPassword), sizeof(NT_OWF_PASSWORD) );
        ZeroMemory( &(Credential->LmOwfPassword), sizeof(LM_OWF_PASSWORD) );
        (*Lsa.FreeLsaHeap)( Credential );
    }

    if ( !NT_SUCCESS(Status)) {
        NlpFreeClientBuffer( &ClientBufferDesc );
    }

    return(Status);
}

VOID
DeriveWithHMAC_SHA1(
    IN      PBYTE   pbKeyMaterial,              // input key material
    IN      DWORD   cbKeyMaterial,
    IN      PBYTE   pbData,                     // input mixing data
    IN      DWORD   cbData,
    IN OUT  BYTE    rgbHMAC[A_SHA_DIGEST_LEN]   // output buffer
    )
{
    BYTE rgbKipad[HMAC_K_PADSIZE];
    BYTE rgbKopad[HMAC_K_PADSIZE];
    A_SHA_CTX sSHAHash;
    DWORD dwBlock;

    // truncate
    if( cbKeyMaterial > HMAC_K_PADSIZE )
        cbKeyMaterial = HMAC_K_PADSIZE;

    ZeroMemory(rgbKipad, sizeof(rgbKipad));
    CopyMemory(rgbKipad, pbKeyMaterial, cbKeyMaterial);

    ZeroMemory(rgbKopad, sizeof(rgbKopad));
    CopyMemory(rgbKopad, pbKeyMaterial, cbKeyMaterial);

    // Kipad, Kopad are padded sMacKey. Now XOR across...
    for( dwBlock = 0; dwBlock < HMAC_K_PADSIZE/sizeof(unsigned __int64) ; dwBlock++ )
    {
        ((unsigned __int64*)rgbKipad)[dwBlock] ^= 0x3636363636363636;
        ((unsigned __int64*)rgbKopad)[dwBlock] ^= 0x5C5C5C5C5C5C5C5C;
    }

    // prepend Kipad to data, Hash to get H1
    A_SHAInit(&sSHAHash);
    A_SHAUpdate(&sSHAHash, rgbKipad, sizeof(rgbKipad));
    A_SHAUpdate(&sSHAHash, pbData, cbData);


    // Finish off the hash
    A_SHAFinal(&sSHAHash, rgbHMAC);

    // prepend Kopad to H1, hash to get HMAC
    // note: done in place to avoid buffer copies

    // final hash: output value into passed-in buffer
    A_SHAInit(&sSHAHash);
    A_SHAUpdate(&sSHAHash, rgbKopad, sizeof(rgbKopad));
    A_SHAUpdate(&sSHAHash, rgbHMAC, A_SHA_DIGEST_LEN);
    A_SHAFinal(&sSHAHash, rgbHMAC);


    ZeroMemory( rgbKipad, sizeof(rgbKipad) );
    ZeroMemory( rgbKopad, sizeof(rgbKopad) );
    ZeroMemory( &sSHAHash, sizeof(sSHAHash) );

    return;
}
