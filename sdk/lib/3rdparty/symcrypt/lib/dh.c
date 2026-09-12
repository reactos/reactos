//
// dh.c   DH functions
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDhSecretAgreement(
    _In_                            PCSYMCRYPT_DLKEY        pkPrivate,
    _In_                            PCSYMCRYPT_DLKEY        pkPublic,
                                    SYMCRYPT_NUMBER_FORMAT  format,
                                    UINT32                  flags,
    _Out_writes_( cbAgreedSecret )  PBYTE                   pbAgreedSecret,
                                    SIZE_T                  cbAgreedSecret )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;
    PBYTE   pbScratchInternal = NULL;
    SIZE_T  cbScratchInternal = 0;

    PCSYMCRYPT_DLGROUP  pDlgroup = NULL;

    PSYMCRYPT_MODELEMENT peRes = NULL;
    UINT32 cbModelement = 0;

    UINT32 nBitsOfExp = 0;

    // Make sure that the keys may be used in DH
    if ( ((pkPrivate->fAlgorithmInfo & SYMCRYPT_FLAG_DLKEY_DH) == 0) ||
         ((pkPublic->fAlgorithmInfo & SYMCRYPT_FLAG_DLKEY_DH) == 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Make sure we only specify the correct flags and that
    // there is a private key
    if ( (flags != 0) || (!pkPrivate->fHasPrivateKey) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Check that the group is the same for both keys
    if ( SymCryptDlgroupIsSame( pkPrivate->pDlgroup, pkPublic->pDlgroup ) )
    {
        pDlgroup = pkPrivate->pDlgroup;
    }
    else
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Check the output buffer has the correct size
    if (cbAgreedSecret != SymCryptDlkeySizeofPublicKey( pkPrivate ))
    {
        scError = SYMCRYPT_WRONG_BLOCK_SIZE;
        goto cleanup;
    }

    // Objects and scratch space size calculation
    cbModelement = SymCryptSizeofModElementFromModulus( pDlgroup->pmP );
    cbScratch = cbModelement +
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( pDlgroup->nDigitsOfP ),
                     SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pDlgroup->nDigitsOfP ));

    // Scratch space allocation
    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if ( pbScratch == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Creating temporary
    pbScratchInternal = pbScratch;
    cbScratchInternal = cbScratch;
    peRes = SymCryptModElementCreate( pbScratchInternal, cbModelement, pDlgroup->pmP );
    pbScratchInternal += cbModelement;
    cbScratchInternal -= cbModelement;

    SYMCRYPT_ASSERT( peRes != NULL);

    // Fix the bits of the exponent (the private key might be either mod Q, mod 2^nBitsPriv, or mod P)
    if (pkPrivate->fPrivateModQ)
    {
        nBitsOfExp = pkPrivate->nBitsPriv;
    }
    else
    {
        nBitsOfExp = pDlgroup->nBitsOfP;
    }

    // Calculate the secret
    SymCryptModExp(
            pDlgroup->pmP,
            pkPublic->pePublicKey,
            pkPrivate->piPrivateKey,
            nBitsOfExp,
            0,              // SC safe
            peRes,
            pbScratchInternal,
            cbScratchInternal );

    // Check if the result is zero
    if ( SymCryptModElementIsZero( pDlgroup->pmP, peRes ) )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    // Output the result
    scError = SymCryptModElementGetValue(
            pDlgroup->pmP,
            peRes,
            pbAgreedSecret,
            cbAgreedSecret,
            format,
            pbScratchInternal,
            cbScratchInternal );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    if ( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }

    return scError;
}
