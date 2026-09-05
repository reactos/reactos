//
// ec_dh.c   ECDH function
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcDhSecretAgreement(
    _In_                            PCSYMCRYPT_ECKEY        pkPrivate,
    _In_                            PCSYMCRYPT_ECKEY        pkPublic,
                                    SYMCRYPT_NUMBER_FORMAT  format,
                                    UINT32                  flags,
    _Out_writes_( cbAgreedSecret )  PBYTE                   pbAgreedSecret,
                                    SIZE_T                  cbAgreedSecret )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;
    SIZE_T  cbScratchInternal = 0;
    PBYTE   pCurr = NULL;

    PCSYMCRYPT_ECURVE   pCurve = NULL;

    PSYMCRYPT_ECPOINT   poQ = NULL;
    PBYTE               pbX = NULL;

    UINT32              cbQ = 0;
    UINT32              cbX = 0;

    // Make sure that the keys may be used in ECDH
    if ( ((pkPrivate->fAlgorithmInfo & SYMCRYPT_FLAG_ECKEY_ECDH) == 0) ||
         ((pkPublic->fAlgorithmInfo & SYMCRYPT_FLAG_ECKEY_ECDH) == 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Make sure we only specify the correct flags and that
    // there is a private key
    if ( (flags != 0) ||
         (!pkPrivate->hasPrivateKey) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Check that the curve is the same for both keys
    if ( SymCryptEcurveIsSame( pkPrivate->pCurve, pkPublic->pCurve ) )
    {
        pCurve = pkPrivate->pCurve;
    }
    else
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Objects and scratch space size calculation
    cbQ = SymCryptSizeofEcpointFromCurve( pCurve );
    cbX  = SymCryptEcurveSizeofFieldElement( pCurve );

    // Check the output buffer has the correct size
    if (cbAgreedSecret != cbX)
    {
        scError = SYMCRYPT_WRONG_BLOCK_SIZE;
        goto cleanup;
    }

    cbScratchInternal = SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS(pCurve),
                        SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS( pCurve ),
                             SYMCRYPT_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( pCurve ) ));

    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    // Thus the following calculation does not overflow cbScratch.
    //
    cbScratch = cbScratchInternal + cbQ + cbX;

    // Scratch space allocation
    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if ( pbScratch == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Creating temporaries
    pCurr = pbScratch + cbScratchInternal;
    poQ = SymCryptEcpointCreate( pCurr, cbQ, pCurve );
    pCurr += cbQ;
    pbX = pCurr;

    SYMCRYPT_ASSERT( poQ != NULL);

    // Make sure that the public key is not the zero point
    // No need to check that the point is on the curve; that check is done when the
    // public key is created.
    if (SymCryptEcpointIsZero(pCurve, pkPublic->poPublicKey, pbScratch, cbScratchInternal))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Calculate the secret
    // Always do low order clearing by multiplying by the cofactor.
    //  Note:   the internal format of piPrivateKey is "DivH", so we
    //          get the correct result.
    scError = SymCryptEcpointScalarMul(
                pCurve,
                pkPrivate->piPrivateKey,
                pkPublic->poPublicKey,
                SYMCRYPT_FLAG_ECC_LL_COFACTOR_MUL,
                poQ,
                pbScratch,
                cbScratchInternal );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Check if the result is the identity point
    if ( SymCryptEcpointIsZero(
                pCurve,
                poQ,
                pbScratch,
                cbScratchInternal ) )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    // Get the x from poQ
    scError = SymCryptEcpointGetValue( pCurve, poQ, format, SYMCRYPT_ECPOINT_FORMAT_X, pbX, cbX, 0, pbScratch, cbScratchInternal);
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Store it in the destination
    memcpy( pbAgreedSecret, pbX, cbX);

cleanup:
    if ( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }

    return scError;
}
