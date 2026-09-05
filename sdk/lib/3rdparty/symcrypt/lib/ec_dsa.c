//
// ec_dsa.c   ECDSA functions
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//

#include "precomp.h"

/*
    Sections 7.2.7 and 7.2.8 of the 29 August 2000
    IEEE Standard Specifications for Public-Key Cryptography,
    IEEE Std 1363-2000, list DSA versions of the elliptic
    curve signature and verification primitives.
    This file has draft interfaces,

    7.2.7  ECSP_DSA (pages 35-36)

       Inputs:
          E -- An elliptic curve.
          G (generator) -- A point on E of prime order r.
          r -- See G.
          s -- A secret exponent, 1 <= s < r (Private key)
          msghash -- Hash of the message being signed.

        Outputs:
           c, d --  Two integers in the interval [1, r-1]

        Algorithm:
           1) Generate random exponent k, 1 <= k < r,
              to be kept from adversary.
              Compute KG = k*G in E.
              Note KG <> (point at infinity).
           2) Convert x(KG) (an element of GF(q))
              to an integer FE2IP(x(KG)).
              Let c = FE2IP(x(KG)) (mod r).
           3) Compute d = (msghash + s*c)/k (mod r).
           4) If c == 0 or d == 0, return to 1).
           5) Output c and d as integers.

    7.2.8  ECVP_DSA

        Inputs:
            E, G, r, msghash -- Same as in ECSP_DSA.
            W -- The signer's public key.  Equal to
                s*G where s was passed to ECSP_DSA.
            c, d  -- A signature to be checked.

        Output:
            TRUE if signature OK, else FALSE.

        Algorithm:
            1) If c or d is not in [1, r-1], return FALSE.
            2) Compute h1 = msghash/d (mod r)
               and h2 = c/d (mod r).
            3) Compute P = h1*G + h2*W.
               If P == (point at infinity), return FALSE.
            4) If c == FE2IP(x(P)) mod r, return TRUE.
               Otherwise return FALSE.

FE2IP is a P1363 function that casts a field element to an
integer (MSB_FIRST).  See Section 5.5.5 of P1363.
*/

//
// Truncating function according to the standard or
// the original CNG implementation:
//
// Initially both implementations truncate the last **bytes**
// of the hash that are over the group byte length. Then if
// the bit length of the hash is still bigger than the bit
// length of the group order, ...
//
// 1. According to the X9.62 standard, we do an appropriate right shift to the entire hash.
//      An example of this is a 160-bit hash, but a 113-bit subgroup order.  For this case:
//      a. We would truncate cbHash to (113 + 7) / 8 = 15 bytes.
//      b. Since 15*8 = 120 > 113 we need to right-shift by 7 bits.
// 2. According to the original CNG implementation, we mask an appropriate number of the
//    topmost bits of the hash.
//      In the same example as before we would zero out the top 7 bits.
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcDsaTruncateHash(
    _In_                            PCSYMCRYPT_ECURVE       pCurve,
    _In_reads_bytes_( cbHashValue ) PCBYTE                  pbHashValue,
                                    SIZE_T                  cbHashValue,
                                    UINT32                  flags,
    _Out_                           PSYMCRYPT_MODELEMENT    peMsghash,
    _Out_                           PSYMCRYPT_INT           piTmp,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 uiBitsizeOfTmp = 0;
    UINT32 uiBitsizeOfGroup = 0;

    // Make sure that only the correct flags are set
    if ( (flags & ~SYMCRYPT_FLAG_ECDSA_NO_TRUNCATION) != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Get the bitsize of the group order
    uiBitsizeOfGroup = SymCryptEcurveBitsizeofGroupOrder( pCurve );

    // Truncate the last bytes of the hash
    if (cbHashValue*8 > uiBitsizeOfGroup)
    {
        cbHashValue = (uiBitsizeOfGroup + 7)/8;
    }

    // Get the value of msghash
    scError = SymCryptIntSetValue( pbHashValue, cbHashValue, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piTmp );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Get the bit size of the hash
    uiBitsizeOfTmp = (UINT32)cbHashValue * 8;

    // If SYMCRYPT_FLAG_ECDSA_NO_TRUNCATION is set, we don't do hash truncation.
    // The caller can do their own truncation before calling into Symcrypt.
    if ( ( flags & SYMCRYPT_FLAG_ECDSA_NO_TRUNCATION ) == 0)
    {
        // ******** Standard truncation **************
        // Shift right if needed
        if ( uiBitsizeOfTmp > uiBitsizeOfGroup )
        {
            SymCryptIntDivPow2( piTmp, uiBitsizeOfTmp - uiBitsizeOfGroup, piTmp );
        }
    }

    SymCryptIntToModElement( piTmp, pCurve->GOrd, peMsghash, pbScratch, cbScratch );        // msghash mod r

cleanup:
    return scError;
}

#define SYMCRYPT_MAX_ECDSA_SIGNATURE_COUNT (100)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcDsaSignEx(
    _In_                                PCSYMCRYPT_ECKEY        pKey,
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
    SIZE_T  cbScratchInternal = 0;
    PBYTE   pCurr = NULL;

    PCSYMCRYPT_ECURVE       pCurve = pKey->pCurve;

    PSYMCRYPT_INT           piTmp = NULL;
    PSYMCRYPT_INT           piMul = NULL;
    PSYMCRYPT_ECPOINT       poKG = NULL;

    PSYMCRYPT_MODELEMENT    peMsghash = NULL;
    PSYMCRYPT_MODELEMENT    peSigC = NULL;
    PSYMCRYPT_MODELEMENT    peSigD = NULL;
    PSYMCRYPT_MODELEMENT    peTmp = NULL;

    PBYTE                   pbX = NULL;

    UINT32  nDigitsInt = 0;
    UINT32  nDigitsMul = 0;

    UINT32  cbInt = 0;
    UINT32  cbMul = 0;
    UINT32  cbKG = 0;
    UINT32  cbRs = 0;
    UINT32  cbX  = 0;

    UINT32 signatureCount = 0;
    UINT32 allowedFlags = SYMCRYPT_FLAG_ECDSA_NO_TRUNCATION | SYMCRYPT_FLAG_DATA_PUBLIC;
    UINT32 publicFlag = flags & SYMCRYPT_FLAG_DATA_PUBLIC;
    UINT32 truncationFlag = flags & SYMCRYPT_FLAG_ECDSA_NO_TRUNCATION;

    // Make sure that the key may be used in ECDSA
    if ( ((pKey->fAlgorithmInfo & SYMCRYPT_FLAG_ECKEY_ECDSA) == 0)  )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Make sure only allowed flags are specified and
    // there is a private key
    if ( ((flags & ~(allowedFlags)) != 0) ||
         (!pKey->hasPrivateKey) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Calculating the digits for the temporary integers
    nDigitsInt = pCurve->GOrdDigits;

    nDigitsMul = SymCryptEcurveDigitsofScalarMultiplier(pCurve);

    // Objects and scratch space size calculation
    cbInt = SymCryptSizeofIntFromDigits( nDigitsInt );
    cbMul = SymCryptSizeofIntFromDigits( nDigitsMul );
    cbKG = SymCryptSizeofEcpointFromCurve( pCurve );
    cbRs = SymCryptSizeofModElementFromModulus( pCurve->GOrd );
    cbX  = SymCryptEcurveSizeofFieldElement( pCurve );

    cbScratchInternal = SYMCRYPT_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS( pCurve );
    cbScratchInternal = SYMCRYPT_MAX( cbScratchInternal, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->GOrdDigits ) );
    cbScratchInternal = SYMCRYPT_MAX( cbScratchInternal, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ) );
    cbScratchInternal = SYMCRYPT_MAX( cbScratchInternal, SYMCRYPT_SCRATCH_BYTES_FOR_MODINV( pCurve->GOrdDigits ) );
    cbScratchInternal = SYMCRYPT_MAX( cbScratchInternal, SYMCRYPT_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( pCurve ) );

    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    // Thus the following calculation does not overflow cbScratch.
    //
    cbScratch = cbScratchInternal + cbInt + cbMul + cbKG + 4*cbRs + cbX;

    // Scratch space allocation
    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if ( pbScratch == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Creating temporaries
    pCurr = pbScratch + cbScratchInternal;
    piTmp = SymCryptIntCreate( pCurr, cbInt, nDigitsInt );
    pCurr += cbInt;
    piMul = SymCryptIntCreate( pCurr, cbMul, nDigitsMul );
    pCurr += cbMul;
    poKG = SymCryptEcpointCreate( pCurr, cbKG, pCurve );
    pCurr += cbKG;
    peMsghash = SymCryptModElementCreate( pCurr, cbRs, pCurve->GOrd );
    pCurr += cbRs;
    peSigC = SymCryptModElementCreate( pCurr, cbRs, pCurve->GOrd );
    pCurr += cbRs;
    peSigD = SymCryptModElementCreate( pCurr, cbRs, pCurve->GOrd );
    pCurr += cbRs;
    peTmp = SymCryptModElementCreate( pCurr, cbRs, pCurve->GOrd );
    pCurr += cbRs;
    pbX = pCurr;

    SYMCRYPT_ASSERT( piTmp != NULL);
    SYMCRYPT_ASSERT( piMul != NULL);
    SYMCRYPT_ASSERT( poKG != NULL);
    SYMCRYPT_ASSERT( peMsghash != NULL);
    SYMCRYPT_ASSERT( peSigC != NULL);
    SYMCRYPT_ASSERT( peSigD != NULL);
    SYMCRYPT_ASSERT( peTmp != NULL);

    // Truncate the message according to the flags
    scError = SymCryptEcDsaTruncateHash(
                    pCurve,
                    pbHashValue,
                    cbHashValue,
                    truncationFlag,
                    peMsghash,
                    piTmp,
                    pbScratch,
                    cbScratchInternal );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    //
    // Main loop: Stop when both c and d are not zero (unless a specific k is provided)
    //
    while( TRUE )
    {
        if ( piK == NULL )
        {
            SymCryptEcpointSetRandom( pCurve, piMul, poKG, pbScratch, cbScratchInternal );          // Generate k and k*G
            SymCryptIntToModElement( piMul, pCurve->GOrd, peTmp, pbScratch, cbScratchInternal );
        }
        else
        {
            // Ensure that piK is in the range [1, GOrd-1]
            if( SymCryptIntIsEqualUint32( piK, 0 ) ||
                !SymCryptIntIsLessThan( piK, SymCryptIntFromModulus( pCurve->GOrd ) ) )
            {
                scError = SYMCRYPT_INVALID_ARGUMENT;
                goto cleanup;
            }

            SymCryptIntCopy( piK, piMul );
            SymCryptIntToModElement( piMul, pCurve->GOrd, peTmp, pbScratch, cbScratchInternal );

            scError = SymCryptEcpointScalarMul( pCurve, piMul, NULL, 0, poKG, pbScratch, cbScratchInternal );  // Generate k*G
            if ( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }
        }

        scError = SymCryptModInv( pCurve->GOrd, peTmp, peTmp, publicFlag, pbScratch, cbScratchInternal );              // Invert k
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        // Get the x coordinates from KG
        scError = SymCryptEcpointGetValue(
                        pCurve,
                        poKG,
                        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                        SYMCRYPT_ECPOINT_FORMAT_X,
                        pbX,
                        cbX,
                        publicFlag,
                        pbScratch,
                        cbScratchInternal );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        // Store c = x(KG) as an integer
        scError = SymCryptModElementSetValue( pbX, cbX, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pCurve->GOrd, peSigC, pbScratch, cbScratch );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        // Move the private key into peSigD
        SymCryptIntToModElement( pKey->piPrivateKey, pCurve->GOrd, peSigD, pbScratch, cbScratchInternal );

        // Multiply the private key by h since its internal format is "DivH"
        for (UINT32 i=0; i<pCurve->coFactorPower; i++)
        {
            SymCryptModAdd( pCurve->GOrd, peSigD, peSigD, peSigD, pbScratch, cbScratchInternal );
        }

        SymCryptModMul( pCurve->GOrd, peSigC, peSigD, peSigD, pbScratch, cbScratchInternal );                   // s * c
        SymCryptModAdd( pCurve->GOrd, peMsghash, peSigD, peSigD, pbScratch, cbScratchInternal );                // msghash + s*c
        SymCryptModMul( pCurve->GOrd, peSigD, peTmp, peSigD, pbScratch, cbScratchInternal );                    // ( msghash + s*c ) / k

        if ( !( SymCryptModElementIsZero( pCurve->GOrd, peSigC ) |
                SymCryptModElementIsZero( pCurve->GOrd, peSigD ) ) )
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
        if ( signatureCount >= SYMCRYPT_MAX_ECDSA_SIGNATURE_COUNT )
        {
            // We have not generated a non-zero signature after SYMCRYPT_MAX_ECDSA_SIGNATURE_COUNT attempts;
            // Something is wrong with the group setup
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }
    }

    // Output c
    scError = SymCryptModElementGetValue( pCurve->GOrd, peSigC, pbSignature, cbSignature / 2, format, pbScratch, cbScratchInternal );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Output d
    scError = SymCryptModElementGetValue( pCurve->GOrd, peSigD, pbSignature + cbSignature / 2, cbSignature / 2, format, pbScratch, cbScratchInternal );
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
SymCryptEcDsaSign(
    _In_                                PCSYMCRYPT_ECKEY        pKey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                  pbHashValue,
                                        SIZE_T                  cbHashValue,
                                        SYMCRYPT_NUMBER_FORMAT  format,
                                        UINT32                  flags,
    _Out_writes_bytes_( cbSignature )   PBYTE                   pbSignature,
                                        SIZE_T                  cbSignature )
{
    // Make sure that only the correct flags are set
    if ( (flags & ~SYMCRYPT_FLAG_ECDSA_NO_TRUNCATION) != 0 )
    {
        return SYMCRYPT_INVALID_ARGUMENT;
    }

    // We must have a private key to perform PCT or signature
    if( !pKey->hasPrivateKey || !(pKey->fAlgorithmInfo & SYMCRYPT_FLAG_ECKEY_ECDSA) )
    {
        return SYMCRYPT_INVALID_ARGUMENT;
    }

    // If the key was generated in SymCrypt and has not yet had a PCT performed - perform PCT before first use
    SYMCRYPT_RUN_KEY_GEN_PCT(
        SymCryptEcDsaPct,
        pKey,
        SYMCRYPT_PCT_ECDSA );

    return SymCryptEcDsaSignEx( pKey, pbHashValue, cbHashValue, NULL, format, flags, pbSignature, cbSignature );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcDsaVerify(
    _In_                                PCSYMCRYPT_ECKEY        pKey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                  pbHashValue,
                                        SIZE_T                  cbHashValue,
    _In_reads_bytes_( cbSignature )     PCBYTE                  pbSignature,
                                        SIZE_T                  cbSignature,
                                        SYMCRYPT_NUMBER_FORMAT  format,
                                        UINT32                  flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;
    SIZE_T  cbScratchInternal = 0;
    PBYTE   pCurr = NULL;
    BOOLEAN fValidSignature = FALSE;

    PCSYMCRYPT_ECURVE       pCurve = pKey->pCurve;

    PSYMCRYPT_INT           piTmp = NULL;
    PSYMCRYPT_INT           piMul1 = NULL;
    PSYMCRYPT_INT           piMul2 = NULL;
    PSYMCRYPT_ECPOINT       poQ1 = NULL;
    PSYMCRYPT_ECPOINT       poQ2 = NULL;

    PSYMCRYPT_MODELEMENT    peMsghash = NULL;
    PSYMCRYPT_MODELEMENT    peSigC = NULL;
    PSYMCRYPT_MODELEMENT    peSigD = NULL;
    PSYMCRYPT_MODELEMENT    peTmp = NULL;

    PBYTE                   pbX = NULL;
    PCSYMCRYPT_ECPOINT      poTable[2] = { 0 };
    PCSYMCRYPT_INT          piTable[2] = { 0 };

    UINT32  nDigitsInt = 0;
    UINT32  nDigitsMul = 0;

    UINT32  cbInt = 0;
    UINT32  cbMul = 0;
    UINT32  cbKG = 0;
    UINT32  cbRs = 0;
    UINT32  cbX  = 0;

    // Make sure that the key may be used in ECDSA
    if ( ((pKey->fAlgorithmInfo & SYMCRYPT_FLAG_ECKEY_ECDSA) == 0)  )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Make sure that only the correct flags are set
    if ( (flags & ~SYMCRYPT_FLAG_ECDSA_NO_TRUNCATION) != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Calculating the digits for the temporary integer
    nDigitsInt = SYMCRYPT_MAX( pCurve->FModDigits, pCurve->GOrdDigits );
    nDigitsInt = SYMCRYPT_MAX( nDigitsInt, SymCryptDigitsFromBits( (UINT32)cbSignature * 4 ) );  // pbSignature contains (c,d)

    nDigitsMul = SymCryptEcurveDigitsofScalarMultiplier(pCurve);

    // Objects and scratch space size calculation
    cbInt = SymCryptSizeofIntFromDigits( nDigitsInt );
    cbMul = SymCryptSizeofIntFromDigits( nDigitsMul );
    cbKG = SymCryptSizeofEcpointFromCurve( pCurve );
    cbRs = SymCryptSizeofModElementFromModulus( pCurve->GOrd );
    cbX  = SymCryptEcurveSizeofFieldElement( pCurve );

    cbScratchInternal = SYMCRYPT_SCRATCH_BYTES_FOR_MULTI_SCALAR_ECURVE_OPERATIONS( pCurve, 2 );
    cbScratchInternal = SYMCRYPT_MAX( cbScratchInternal, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->GOrdDigits ) );
    cbScratchInternal = SYMCRYPT_MAX( cbScratchInternal, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ) );
    cbScratchInternal = SYMCRYPT_MAX( cbScratchInternal, SYMCRYPT_SCRATCH_BYTES_FOR_MODINV( pCurve->GOrdDigits ) );
    cbScratchInternal = SYMCRYPT_MAX( cbScratchInternal, SYMCRYPT_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( pCurve ) );
    cbScratchInternal = SYMCRYPT_MAX( cbScratchInternal, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ) );

    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    // Thus the following calculation does not overflow cbScratch.
    //
    cbScratch = cbScratchInternal + cbInt + 2*cbMul + 2*cbKG + 4*cbRs + cbX;

    // Scratch space allocation
    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if ( pbScratch == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Creating temporaries
    pCurr = pbScratch + cbScratchInternal;
    piTmp = SymCryptIntCreate( pCurr, cbInt, nDigitsInt );
    pCurr += cbInt;
    piMul1 = SymCryptIntCreate( pCurr, cbMul, nDigitsMul );
    pCurr += cbMul;
    piMul2 = SymCryptIntCreate( pCurr, cbMul, nDigitsMul );
    pCurr += cbMul;
    poQ1 = SymCryptEcpointCreate( pCurr, cbKG, pCurve );
    pCurr += cbKG;
    poQ2 = SymCryptEcpointCreate( pCurr, cbKG, pCurve );
    pCurr += cbKG;
    peMsghash = SymCryptModElementCreate( pCurr, cbRs, pCurve->GOrd );
    pCurr += cbRs;
    peSigC = SymCryptModElementCreate( pCurr, cbRs, pCurve->GOrd );
    pCurr += cbRs;
    peSigD = SymCryptModElementCreate( pCurr, cbRs, pCurve->GOrd );
    pCurr += cbRs;
    peTmp = SymCryptModElementCreate( pCurr, cbRs, pCurve->GOrd );
    pCurr += cbRs;
    pbX = pCurr;

    SYMCRYPT_ASSERT( piTmp != NULL);
    SYMCRYPT_ASSERT( piMul1 != NULL);
    SYMCRYPT_ASSERT( piMul2 != NULL);
    SYMCRYPT_ASSERT( poQ1 != NULL);
    SYMCRYPT_ASSERT( poQ2 != NULL);
    SYMCRYPT_ASSERT( peMsghash != NULL);
    SYMCRYPT_ASSERT( peSigC != NULL);
    SYMCRYPT_ASSERT( peSigD != NULL);
    SYMCRYPT_ASSERT( peTmp != NULL);

    // Get c
    scError = SymCryptIntSetValue( pbSignature, cbSignature / 2, format, piTmp );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Check if c is less than r
    if ( !SymCryptIntIsLessThan( piTmp, SymCryptIntFromModulus( pCurve->GOrd ) ) )
    {
        goto cleanup;
    }

    // c mod r
    SymCryptIntToModElement( piTmp, pCurve->GOrd, peSigC, pbScratch, cbScratchInternal );

    // Check if c is zero
    if (SymCryptModElementIsZero( pCurve->GOrd, peSigC ))
    {
        goto cleanup;
    }

    // Get d
    scError = SymCryptIntSetValue( pbSignature + cbSignature / 2, cbSignature / 2, format, piTmp );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Check if d is less than r
    if ( !SymCryptIntIsLessThan( piTmp, SymCryptIntFromModulus( pCurve->GOrd ) ) )
    {
        goto cleanup;
    }

    // d mod r
    SymCryptIntToModElement( piTmp, pCurve->GOrd, peSigD, pbScratch, cbScratchInternal );

    // Check if d is zero
    if (SymCryptModElementIsZero( pCurve->GOrd, peSigD ))
    {
        goto cleanup;
    }

    // Calculate 1/d mod r
    // The D value is not secret; it is part of the signature.
    // We mark it public to avoid the use of random blinding, which would require a source of randomness
    // just to verify an ECDSA signature.
    scError = SymCryptModInv( pCurve->GOrd, peSigD, peSigD, SYMCRYPT_FLAG_DATA_PUBLIC, pbScratch, cbScratchInternal );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Truncate the message according to the flags
    scError = SymCryptEcDsaTruncateHash(
                    pCurve,
                    pbHashValue,
                    cbHashValue,
                    flags,
                    peMsghash,
                    piTmp,
                    pbScratch,
                    cbScratchInternal );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    SymCryptModMul( pCurve->GOrd, peMsghash, peSigD, peMsghash, pbScratch, cbScratchInternal );     // msghash / d = h1
    SymCryptModMul( pCurve->GOrd, peSigC, peSigD, peTmp, pbScratch, cbScratchInternal );            // c / d = h2

    SymCryptModElementToInt( pCurve->GOrd, peMsghash, piMul1, pbScratch, cbScratchInternal );
    SymCryptModElementToInt( pCurve->GOrd, peTmp, piMul2, pbScratch, cbScratchInternal );

    // h1*G + h2*W
    piTable[0] = piMul1;
    piTable[1] = piMul2;

    poTable[0] = NULL;  // The first base point is the generator G of the group
    poTable[1] = pKey->poPublicKey;

    scError = SymCryptEcpointMultiScalarMul( pCurve, piTable, poTable, 2, SYMCRYPT_FLAG_DATA_PUBLIC, poQ1, pbScratch, cbScratchInternal );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Check for point at infinity
    if ( SymCryptEcpointIsZero( pCurve, poQ1, pbScratch, cbScratchInternal ) )
    {
        goto cleanup;
    }

    // Get the x from poQ1
    scError = SymCryptEcpointGetValue( pCurve, poQ1, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, SYMCRYPT_ECPOINT_FORMAT_X, pbX, cbX, SYMCRYPT_FLAG_DATA_PUBLIC, pbScratch, cbScratchInternal);
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Store it in a big enough INT
    scError = SymCryptIntSetValue( pbX, cbX, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piTmp );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    SymCryptIntToModElement( piTmp, pCurve->GOrd, peTmp, pbScratch, cbScratchInternal );    // x mod r

    // Comparison c = x
    if (SymCryptModElementIsEqual( pCurve->GOrd, peSigC, peTmp ))
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
