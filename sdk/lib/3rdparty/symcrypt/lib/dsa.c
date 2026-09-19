//
// dsa.c   DSA functions
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//

#include "precomp.h"

// Truncating function according to the FIPS 186-4 standard
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDsaTruncateHash(
    _In_                            PCSYMCRYPT_DLGROUP      pDlgroup,
    _In_reads_bytes_( cbHashValue ) PCBYTE                  pbHashValue,
                                    SIZE_T                  cbHashValue,
                                    UINT32                  flags,
    _Out_                           PSYMCRYPT_MODELEMENT    peMsghash,
    _Out_                           PSYMCRYPT_INT           piIntLarge,
    _Out_                           PSYMCRYPT_INT           piIntQ,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    UNREFERENCED_PARAMETER( flags );

    // Get the value of msghash into piIntLarge
    scError = SymCryptIntSetValue( pbHashValue, cbHashValue, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piIntLarge );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Truncate the rightmost bits if the value exceeds the size of the modulus Q
    if (SymCryptIntBitsizeOfValue(piIntLarge) > pDlgroup->nBitsOfQ)
    {
        SymCryptIntDivPow2( piIntLarge, SymCryptIntBitsizeOfValue(piIntLarge) - pDlgroup->nBitsOfQ, piIntLarge );
    }

    scError = SymCryptIntCopyMixedSize( piIntLarge, piIntQ );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        // This should never fail here as we truncated the IntLarge
        goto cleanup;
    }

    // Now we can call IntToModElement as they have the same digit size
    SymCryptIntToModElement( piIntQ, pDlgroup->pmQ, peMsghash, pbScratch, cbScratch );        // msghash mod Q

cleanup:
    return scError;
}

#define SYMCRYPT_MAX_DSA_SIGNATURE_COUNT (100)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDsaSignEx(
    _In_                                PCSYMCRYPT_DLKEY        pKey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                  pbHashValue,
                                        SIZE_T                  cbHashValue,
    _In_opt_                            PCSYMCRYPT_INT          piK,
                                        SYMCRYPT_NUMBER_FORMAT  format,
                                        UINT32                  flags,
    _Out_writes_bytes_( cbSignature )   PBYTE                   pbSignature,
                                        SIZE_T                  cbSignature )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;
    PBYTE   pbScratchInternal = NULL;
    SIZE_T  cbScratchInternal = 0;

    SIZE_T  cbScratchInputK = 0;        // Extra scratch space needed if the caller specified a K

    PCSYMCRYPT_DLGROUP pDlgroup = pKey->pDlgroup;
    UINT32 nDigitsOfP = pDlgroup->nDigitsOfP;
    UINT32 nDigitsOfQ = pDlgroup->nDigitsOfQ;

    UINT32 ndIntLarge = 0;

    UINT32 cbIntLarge = 0;
    UINT32 cbIntQ = 0;
    UINT32 cbIntP = 0;

    UINT32 cbModelementP = 0;
    UINT32 cbModelementQ = 0;

    // Helper Integers
    PSYMCRYPT_INT piIntLarge = NULL;    // Safe size for all caller specified sizes
    PSYMCRYPT_INT piIntP = NULL;        // Same number of digits as P
    PSYMCRYPT_INT piIntQ = NULL;        // Same number of digits as Q

    // Elements modulo P
    PSYMCRYPT_MODELEMENT peRmodP = NULL;

    // Elements modulo Q
    PSYMCRYPT_MODELEMENT peMsghash = NULL;
    PSYMCRYPT_MODELEMENT peRmodQ = NULL;
    PSYMCRYPT_MODELEMENT peK = NULL;
    PSYMCRYPT_MODELEMENT peS = NULL;

    UINT32 signatureCount = 0;

    UNREFERENCED_PARAMETER( flags );

    // Make sure that the key may be used in DSA
    if ( ((pKey->fAlgorithmInfo & SYMCRYPT_FLAG_DLKEY_DSA) == 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Make sure that the group and the key have all the
    // information for dsa, i.e. prime q and private key
    // modulo q, and we are not using a named DH safe-prime
    // group
    if ((!pDlgroup->fHasPrimeQ) ||
        (!pKey->fHasPrivateKey) ||
        (!pKey->fPrivateModQ) ||
        (pDlgroup->isSafePrimeGroup))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Calculate the digit size for the HashValue
    ndIntLarge = SymCryptDigitsFromBits( (UINT32)cbHashValue * 8 );

    // Calculate the sizes of temp objects
    cbIntLarge = SymCryptSizeofIntFromDigits(ndIntLarge);
    cbIntQ = SymCryptSizeofIntFromDigits(nDigitsOfQ);
    cbIntP = SymCryptSizeofIntFromDigits(nDigitsOfP);

    cbModelementP = SymCryptSizeofModElementFromModulus( pDlgroup-> pmP );
    cbModelementQ = SymCryptSizeofModElementFromModulus( pDlgroup-> pmQ );

    // Allocate scratch space
    cbScratchInputK = (piK==NULL)?0:SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD(SymCryptIntDigitsizeOfObject(piK),nDigitsOfQ);

    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    // Thus the following calculation does not overflow cbScratch.
    //
    cbScratch = cbIntLarge + cbIntQ + cbIntP + cbModelementP + 4*cbModelementQ +
                SYMCRYPT_MAX( cbScratchInputK,
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigitsOfQ ),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigitsOfP ),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( nDigitsOfP ),
                     SYMCRYPT_SCRATCH_BYTES_FOR_MODINV( nDigitsOfQ ) ))));
    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if (pbScratch==NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Create the objects
    pbScratchInternal = pbScratch;
    cbScratchInternal = cbScratch;

    piIntLarge = SymCryptIntCreate(pbScratchInternal, cbIntLarge, ndIntLarge); pbScratchInternal += cbIntLarge; cbScratchInternal -= cbIntLarge;
    piIntQ = SymCryptIntCreate(pbScratchInternal, cbIntQ, nDigitsOfQ); pbScratchInternal += cbIntQ; cbScratchInternal -= cbIntQ;
    piIntP = SymCryptIntCreate(pbScratchInternal, cbIntP, nDigitsOfP); pbScratchInternal += cbIntP; cbScratchInternal -= cbIntP;

    peRmodP = SymCryptModElementCreate(pbScratchInternal, cbModelementP, pDlgroup->pmP); pbScratchInternal += cbModelementP; cbScratchInternal -= cbModelementP;

    peMsghash = SymCryptModElementCreate(pbScratchInternal, cbModelementQ, pDlgroup->pmQ); pbScratchInternal += cbModelementQ; cbScratchInternal -= cbModelementQ;
    peRmodQ = SymCryptModElementCreate(pbScratchInternal, cbModelementQ, pDlgroup->pmQ); pbScratchInternal += cbModelementQ; cbScratchInternal -= cbModelementQ;
    peK = SymCryptModElementCreate(pbScratchInternal, cbModelementQ, pDlgroup->pmQ); pbScratchInternal += cbModelementQ; cbScratchInternal -= cbModelementQ;
    peS = SymCryptModElementCreate(pbScratchInternal, cbModelementQ, pDlgroup->pmQ); pbScratchInternal += cbModelementQ; cbScratchInternal -= cbModelementQ;

    // Get the message into a modelement
    scError = SymCryptDsaTruncateHash(
                    pDlgroup,
                    pbHashValue,
                    cbHashValue,
                    flags,
                    peMsghash,
                    piIntLarge,
                    piIntQ,
                    pbScratchInternal,
                    cbScratchInternal );
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    //
    // Main loop: Stop when both R and S are not zero (unless a specific k is provided)
    //
    while( TRUE )
    {
        if (piK==NULL)
        {
            SymCryptModSetRandom(
                pDlgroup->pmQ,
                peK,
                SYMCRYPT_FLAG_MODRANDOM_ALLOW_ONE|SYMCRYPT_FLAG_MODRANDOM_ALLOW_MINUSONE,
                pbScratchInternal,
                cbScratchInternal );

            SymCryptModElementToInt(
                pDlgroup->pmQ,
                peK,
                piIntQ,
                pbScratchInternal,
                cbScratchInternal );
        }
        else
        {
            SymCryptIntDivMod(
                piK,
                SymCryptDivisorFromModulus( pDlgroup->pmQ ),
                NULL,
                piIntQ,
                pbScratchInternal,
                cbScratchInternal );

            SymCryptIntToModElement(
                piIntQ,
                pDlgroup->pmQ,
                peK,
                pbScratchInternal,
                cbScratchInternal );

            // Make sure that the K passed in is not zero
            if (SymCryptModElementIsZero(pDlgroup->pmQ, peK))
            {
                scError = SYMCRYPT_INVALID_ARGUMENT;
                goto cleanup;
            }
        }

        // Here piIntQ and peK hold the random exponent K

        // G^K mod P
        SymCryptModExp(
                pDlgroup->pmP,
                pDlgroup->peG,
                piIntQ,
                pDlgroup->nBitsOfQ,
                0,      // Side-channel safe
                peRmodP,
                pbScratchInternal,
                cbScratchInternal );

        // Convert to integer
        SymCryptModElementToInt(
                pDlgroup->pmP,
                peRmodP,
                piIntP,
                pbScratchInternal,
                cbScratchInternal );

        // Convert to mod Q
        SymCryptIntDivMod(
                piIntP,
                SymCryptDivisorFromModulus( pDlgroup->pmQ ),
                NULL,
                piIntQ,
                pbScratchInternal,
                cbScratchInternal );

        // Convert to modelement
        SymCryptIntToModElement(
                piIntQ,
                pDlgroup->pmQ,
                peRmodQ,
                pbScratchInternal,
                cbScratchInternal );

        // Invert k mod q
        scError = SymCryptModInv(
                pDlgroup->pmQ,
                peK,
                peK,    // In place
                0,
                pbScratchInternal,
                cbScratchInternal );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        // Get the private key X to modelement
        // *** We are sure here that the digit
        // size of it is nDigitsOfQ
        SymCryptIntToModElement(
                pKey->piPrivateKey,
                pDlgroup->pmQ,
                peS,
                pbScratchInternal,
                cbScratchInternal );

        // X*R
        SymCryptModMul(
                pDlgroup->pmQ,
                peS,
                peRmodQ,
                peS,
                pbScratchInternal,
                cbScratchInternal );

        // H(m)+X*R
        SymCryptModAdd(
                pDlgroup->pmQ,
                peS,
                peMsghash,
                peS,
                pbScratchInternal,
                cbScratchInternal );

        // S = k^{-1}*(H(m)+X*R)
        SymCryptModMul(
                pDlgroup->pmQ,
                peK,
                peS,
                peS,
                pbScratchInternal,
                cbScratchInternal );

        if ( !( SymCryptModElementIsZero( pDlgroup->pmQ, peRmodQ ) |
                SymCryptModElementIsZero( pDlgroup->pmQ, peS ) ) )
        {
            break;
        }

        if (piK != NULL)
        {
            // piK resulted in 0 signature
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        signatureCount++;
        if ( signatureCount >= SYMCRYPT_MAX_DSA_SIGNATURE_COUNT )
        {
            // We have not generated a non-zero signature after SYMCRYPT_MAX_DSA_SIGNATURE_COUNT attempts;
            // Something is wrong with the group setup
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }
    }

    // Output R
    scError = SymCryptModElementGetValue(
                pDlgroup->pmQ,
                peRmodQ,
                pbSignature,
                cbSignature / 2,
                format,
                pbScratchInternal,
                cbScratchInternal );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Output S
    scError = SymCryptModElementGetValue(
                pDlgroup->pmQ,
                peS,
                pbSignature + cbSignature / 2,
                cbSignature / 2,
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

    if (scError != SYMCRYPT_NO_ERROR)
    {
        SymCryptWipe( pbSignature, cbSignature );
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDsaSign(
    _In_                                PCSYMCRYPT_DLKEY        pKey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                  pbHashValue,
                                        SIZE_T                  cbHashValue,
                                        SYMCRYPT_NUMBER_FORMAT  format,
                                        UINT32                  flags,
    _Out_writes_bytes_( cbSignature )   PBYTE                   pbSignature,
                                        SIZE_T                  cbSignature )
{
    return SymCryptDsaSignEx( pKey, pbHashValue, cbHashValue, NULL, format, flags, pbSignature, cbSignature );
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDsaVerify(
    _In_                                PCSYMCRYPT_DLKEY        pKey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                  pbHashValue,
                                        SIZE_T                  cbHashValue,
    _In_reads_bytes_( cbSignature )     PCBYTE                  pbSignature,
                                        SIZE_T                  cbSignature,
                                        SYMCRYPT_NUMBER_FORMAT  format,
                                        UINT32                  flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    BOOLEAN fValidSignature = FALSE;

    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;
    PBYTE   pbScratchInternal = NULL;
    SIZE_T  cbScratchInternal = 0;

    PCSYMCRYPT_DLGROUP pDlgroup = pKey->pDlgroup;
    UINT32 nDigitsOfP = pDlgroup->nDigitsOfP;
    UINT32 nDigitsOfQ = pDlgroup->nDigitsOfQ;

    UINT32 ndIntLarge = 0;

    UINT32 cbIntLarge = 0;
    UINT32 cbIntQ = 0;
    UINT32 cbIntP = 0;

    UINT32 cbModelementP = 0;
    UINT32 cbModelementQ = 0;

    PSYMCRYPT_MODELEMENT peBases[2] = { NULL, NULL }; // Array with pointers to base points

    // Helper Integers
    PSYMCRYPT_INT piIntLarge = NULL;            // Safe size for all caller specified sizes
    PSYMCRYPT_INT piIntP = NULL;                // Same number of digits as P
    PSYMCRYPT_INT piIntQ[2] = { NULL, NULL };   // Same number of digits as Q

    // Elements modulo P
    PSYMCRYPT_MODELEMENT peResP = NULL;

    // Elements modulo Q
    PSYMCRYPT_MODELEMENT peR = NULL;
    PSYMCRYPT_MODELEMENT peS = NULL;
    PSYMCRYPT_MODELEMENT peT = NULL;    // Temp

    UNREFERENCED_PARAMETER( flags );

    // Make sure that the key may be used in DSA
    if ( ((pKey->fAlgorithmInfo & SYMCRYPT_FLAG_DLKEY_DSA) == 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Make sure that the group has a prime q, and we are not using a named DH safe-prime group
    if (!pDlgroup->fHasPrimeQ || pDlgroup->isSafePrimeGroup)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Calculate the digit sizes
    ndIntLarge = SymCryptDigitsFromBits( (UINT32)cbHashValue * 8 );
    ndIntLarge = SYMCRYPT_MAX( ndIntLarge, SymCryptDigitsFromBits( (UINT32)cbSignature * 4 ) );  // pbSignature contains (R,S)

    // Calculate the sizes of temp objects
    cbIntLarge = SymCryptSizeofIntFromDigits(ndIntLarge);
    cbIntQ = SymCryptSizeofIntFromDigits(nDigitsOfQ);
    cbIntP = SymCryptSizeofIntFromDigits(nDigitsOfP);

    cbModelementP = SymCryptSizeofModElementFromModulus( pDlgroup-> pmP );
    cbModelementQ = SymCryptSizeofModElementFromModulus( pDlgroup-> pmQ );

    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    // Thus the following calculation does not overflow cbScratch.
    //
    cbScratch = cbIntLarge + cbIntP + 2*cbIntQ + cbModelementP + 3*cbModelementQ +
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD(nDigitsOfP,nDigitsOfQ),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigitsOfQ ),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigitsOfP ),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_MODMULTIEXP( SymCryptModulusDigitsizeOfObject(pDlgroup->pmP), 2, pDlgroup->nBitsOfQ ),
                     SYMCRYPT_SCRATCH_BYTES_FOR_MODINV( nDigitsOfQ ) ))));
    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if (pbScratch==NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Create the objects
    pbScratchInternal = pbScratch;
    cbScratchInternal = cbScratch;

    piIntLarge = SymCryptIntCreate(pbScratchInternal, cbIntLarge, ndIntLarge); pbScratchInternal += cbIntLarge; cbScratchInternal -= cbIntLarge;
    piIntP = SymCryptIntCreate(pbScratchInternal, cbIntP, nDigitsOfP); pbScratchInternal += cbIntP; cbScratchInternal -= cbIntP;
    piIntQ[0] = SymCryptIntCreate(pbScratchInternal, cbIntQ, nDigitsOfQ); pbScratchInternal += cbIntQ; cbScratchInternal -= cbIntQ;
    piIntQ[1] = SymCryptIntCreate(pbScratchInternal, cbIntQ, nDigitsOfQ); pbScratchInternal += cbIntQ; cbScratchInternal -= cbIntQ;

    peResP = SymCryptModElementCreate(pbScratchInternal, cbModelementP, pDlgroup->pmP); pbScratchInternal += cbModelementP; cbScratchInternal -= cbModelementP;

    peR = SymCryptModElementCreate(pbScratchInternal, cbModelementQ, pDlgroup->pmQ); pbScratchInternal += cbModelementQ; cbScratchInternal -= cbModelementQ;
    peS = SymCryptModElementCreate(pbScratchInternal, cbModelementQ, pDlgroup->pmQ); pbScratchInternal += cbModelementQ; cbScratchInternal -= cbModelementQ;
    peT = SymCryptModElementCreate(pbScratchInternal, cbModelementQ, pDlgroup->pmQ); pbScratchInternal += cbModelementQ; cbScratchInternal -= cbModelementQ;

    // Get R
    scError = SymCryptIntSetValue( pbSignature, cbSignature / 2, format, piIntLarge );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Check if R is less than Q
    if ( !SymCryptIntIsLessThan( piIntLarge, SymCryptIntFromModulus( pDlgroup->pmQ ) ) )
    {
        goto cleanup;
    }

    // R mod Q (use piIntQ[0] as temp space)
    scError = SymCryptIntCopyMixedSize( piIntLarge, piIntQ[0] );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        // This should never fail here as we verified that IntLarge is less than Q
        goto cleanup;
    }
    SymCryptIntToModElement( piIntQ[0], pDlgroup->pmQ, peR, pbScratchInternal, cbScratchInternal );

    // Check if R is zero
    if (SymCryptModElementIsZero( pDlgroup->pmQ, peR ))
    {
        goto cleanup;
    }

    // Get S
    scError = SymCryptIntSetValue( pbSignature + cbSignature / 2, cbSignature / 2, format, piIntLarge );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Check if S is less than Q
    if ( !SymCryptIntIsLessThan( piIntLarge, SymCryptIntFromModulus( pDlgroup->pmQ ) ) )
    {
        goto cleanup;
    }

    // S mod Q (use piIntQ[0] as temp space)
    scError = SymCryptIntCopyMixedSize( piIntLarge, piIntQ[0] );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        // This should never fail here as we verified that IntLarge is less than Q
        goto cleanup;
    }
    SymCryptIntToModElement( piIntQ[0], pDlgroup->pmQ, peS, pbScratchInternal, cbScratchInternal );

    // Check if S is zero
    if (SymCryptModElementIsZero( pDlgroup->pmQ, peS ))
    {
        goto cleanup;
    }

    // Calculate 1/S mod Q
    // S is part of the signature and therefore not a secret.
    // We mark it public to avoid the use of random blinding, which would require a source of randomness
    // just to verify a DSA signature.
    scError = SymCryptModInv( pDlgroup->pmQ, peS, peS, SYMCRYPT_FLAG_DATA_PUBLIC, pbScratchInternal, cbScratchInternal );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Get the message into a modelement
    scError = SymCryptDsaTruncateHash(
                    pDlgroup,
                    pbHashValue,
                    cbHashValue,
                    flags,
                    peT,
                    piIntLarge,
                    piIntQ[0],
                    pbScratchInternal,
                    cbScratchInternal );
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Calculate U1 = Hash(M)/S modQ
    SymCryptModMul(
            pDlgroup->pmQ,
            peT,
            peS,
            peT,
            pbScratchInternal,
            cbScratchInternal );

    // Convert U1 to integer
    SymCryptModElementToInt(
                pDlgroup->pmQ,
                peT,
                piIntQ[0],
                pbScratchInternal,
                cbScratchInternal );

    // Calculate U2 = R/S modQ
    SymCryptModMul(
            pDlgroup->pmQ,
            peR,
            peS,
            peT,
            pbScratchInternal,
            cbScratchInternal );

    // Convert U2 to integer
    SymCryptModElementToInt(
                pDlgroup->pmQ,
                peT,
                piIntQ[1],
                pbScratchInternal,
                cbScratchInternal );

    // Arrange the pointers for v = G^U1 * Y^U2
    peBases[0] = pDlgroup->peG;
    peBases[1] = pKey->pePublicKey;

    // v = G^U1 * Y^U2
    scError = SymCryptModMultiExp(
                pDlgroup->pmP,
                peBases,
                piIntQ,
                2,
                pDlgroup->nBitsOfQ,
                SYMCRYPT_FLAG_DATA_PUBLIC,
                peResP,
                pbScratchInternal,
                cbScratchInternal );
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Convert V to a modelement modulo Q
    SymCryptModElementToInt(
                pDlgroup->pmP,
                peResP,
                piIntP,
                pbScratchInternal,
                cbScratchInternal );
    SymCryptIntDivMod(
                piIntP,
                SymCryptDivisorFromModulus( pDlgroup->pmQ ),
                NULL,
                piIntQ[0],
                pbScratchInternal,
                cbScratchInternal );
    SymCryptIntToModElement(
                piIntQ[0],
                pDlgroup->pmQ,
                peT,
                pbScratchInternal,
                cbScratchInternal );

    // Comparison V = R
    if (SymCryptModElementIsEqual( pDlgroup->pmQ, peT, peR ))
    {
        fValidSignature = TRUE;
    }


cleanup:

    if (!fValidSignature)
    {
        scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
    }

    if ( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }

    return scError;
}
