//
// rsa_enc.c   RSA related algorithms
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// Helper functions for RSA raw encrypt/decrypt (they do NOT allocate scratch space)
//

UINT32
SYMCRYPT_CALL
SymCryptRsaCoreEncScratchSpace( _In_ PCSYMCRYPT_RSAKEY pkRsakey)
{
	// Bounded by 2^19 + 2^24 ~ 2^24 (see symcrypt_internal.h)
    return SymCryptSizeofModElementFromModulus( pkRsakey->pmModulus ) +
           SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pkRsakey->nDigitsOfModulus ),
                SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( pkRsakey->nDigitsOfModulus ) );
}

SYMCRYPT_ERROR
SymCryptRsaCoreVerifyInput(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_NUMBER_FORMAT      numFormat,
                                SIZE_T                      cbDst,
    _Out_writes_bytes_( cbScratch )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbScratch)
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;
    PSYMCRYPT_INT           piTmpInteger = NULL;
    UINT32                  cbTmpInteger = 0;

    UNREFERENCED_PARAMETER( cbScratch );

    if ( cbSrc > SymCryptRsakeySizeofModulus(pkRsakey) ||
         cbDst < SymCryptRsakeySizeofModulus(pkRsakey) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // It is an error of value(pbSrc) >= modulus
    // We already know that cbSrc <= sizeof( modulus ) so we only have to run this check
    // if cbSrc == sizeof( modulus )
    // No side channel issues here: we are only comparing the input to the public part of the key.
    if (cbSrc == SymCryptRsakeySizeofModulus(pkRsakey))
    {
        cbTmpInteger = SymCryptSizeofIntFromDigits( pkRsakey->nDigitsOfModulus );
        SYMCRYPT_ASSERT( cbScratch >= cbTmpInteger );
        piTmpInteger = SymCryptIntCreate( pbScratch, cbTmpInteger, pkRsakey->nDigitsOfModulus );

        scError = SymCryptIntSetValue( pbSrc, cbSrc, numFormat, piTmpInteger );
        if (scError != SYMCRYPT_NO_ERROR)
        {
           goto cleanup;
        }

        if (!SymCryptIntIsLessThan(piTmpInteger, SymCryptIntFromModulus(pkRsakey->pmModulus)))
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }
    }

cleanup:
   return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaCoreEnc(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_NUMBER_FORMAT      numFormat,
                                UINT32                      flags,
    _Out_writes_( cbDst )       PBYTE                       pbDst,
                                SIZE_T                      cbDst,
    _Out_writes_bytes_( cbScratch )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbScratch )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    PSYMCRYPT_MODELEMENT peRes = NULL;
    UINT32 cbModElement = 0;

    PBYTE   pbFnScratch = NULL;
    SIZE_T  cbFnScratch = 0;

    BYTE    abExpIntBuffer[ SYMCRYPT_SIZEOF_INT_FROM_BITS( 64 ) + SYMCRYPT_ASYM_ALIGN_VALUE];
    PSYMCRYPT_INT piExp = NULL;

    UNREFERENCED_PARAMETER( flags );

    scError = SymCryptRsaCoreVerifyInput(pkRsakey, pbSrc, cbSrc, numFormat, cbDst, pbScratch, cbScratch);
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    cbModElement = SymCryptSizeofModElementFromModulus( pkRsakey->pmModulus );

    UNREFERENCED_PARAMETER( cbScratch );
    SYMCRYPT_ASSERT( cbScratch >= cbModElement +
                                  SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pkRsakey->nDigitsOfModulus ),
                                       SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( pkRsakey->nDigitsOfModulus ) ));

    pbFnScratch = pbScratch;
    cbFnScratch = cbScratch;

    peRes = SymCryptModElementCreate( pbScratch, cbModElement, pkRsakey->pmModulus );
    SYMCRYPT_ASSERT( peRes != NULL );
    pbFnScratch += cbModElement;
    cbFnScratch -= cbModElement;

    // Set the original value
    scError = SymCryptModElementSetValue( pbSrc, cbSrc, numFormat, pkRsakey->pmModulus, peRes, pbFnScratch, cbFnScratch );
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Convert the public exponent to an Int
    // Future: we can optimize the ModExp to take an UINT64
    piExp = SymCryptIntCreate( SYMCRYPT_ASYM_ALIGN_UP(abExpIntBuffer), sizeof( abExpIntBuffer) - SYMCRYPT_ASYM_ALIGN_VALUE, 1 );
    if( piExp == NULL )
    {
        scError = SYMCRYPT_HARDWARE_FAILURE;
        goto cleanup;
    }
    SymCryptIntSetValueUint64( pkRsakey->au64PubExp[0], piExp );

    // Modular Exponentiation
    SymCryptModExp(
            pkRsakey->pmModulus,
            peRes,
            piExp,
            SymCryptIntBitsizeOfValue( piExp ),   // This is a public value
            SYMCRYPT_FLAG_DATA_PUBLIC,
            peRes,
            pbFnScratch,
            cbFnScratch );

    // Output the value
    scError = SymCryptModElementGetValue( pkRsakey->pmModulus, peRes, pbDst, cbDst, numFormat, pbFnScratch, cbFnScratch );
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

cleanup:

    if( piExp != NULL )
    {
        SymCryptIntWipe( piExp );
    }

    return scError;
}

UINT32
SYMCRYPT_CALL
SymCryptRsaCoreDecCrtScratchSpace( _In_ PCSYMCRYPT_RSAKEY pkRsakey)
{
    UINT32 cbModElementTotal = 0;
    UINT32 nPrimes = pkRsakey->nPrimes;

    SYMCRYPT_ASSERT( nPrimes <= SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES );
    // clamp nPrimes to SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES for scratch memory allocation purposes
    // SymCryptRsaCoreDecCrt will fail with invalid argument if there are too many primes later
    nPrimes = SYMCRYPT_MIN( nPrimes, SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES );

    for (UINT32 i=0; i<pkRsakey->nPrimes; i++)
    {
        cbModElementTotal += SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( pkRsakey->nBitsOfPrimes[i]);
    }

    // Bounded by 5*2^19 + 2^24 ~ 2^24 (see symcrypt_internal.h)
    return 3*SymCryptSizeofIntFromDigits( pkRsakey->nDigitsOfModulus ) +
           SymCryptSizeofIntFromDigits( pkRsakey->nMaxDigitsOfPrimes ) +
           cbModElementTotal +
           SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( pkRsakey->nBitsOfModulus) +
           SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pkRsakey->nDigitsOfModulus ),
           SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( pkRsakey->nDigitsOfModulus ),
           SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( pkRsakey->nDigitsOfModulus, pkRsakey->nMaxDigitsOfPrimes ),
                SYMCRYPT_SCRATCH_BYTES_FOR_CRT_SOLUTION( pkRsakey->nMaxDigitsOfPrimes ) )));
}

UINT32
SYMCRYPT_CALL
SymCryptRsaCoreDecScratchSpace( _In_ PCSYMCRYPT_RSAKEY pkRsakey)
{
	// Bounded by 2^19 + 2^24 ~ 2^24 (see symcrypt_internal.h)
    return SymCryptSizeofModElementFromModulus( pkRsakey->pmModulus ) +
           SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pkRsakey->nDigitsOfModulus ),
                SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( pkRsakey->nDigitsOfModulus ) );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaCoreDecCrt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_NUMBER_FORMAT      numFormat,
                                UINT32                      flags,
    _Out_writes_( cbDst )       PBYTE                       pbDst,
                                SIZE_T                      cbDst,
    _Out_writes_bytes_( cbScratch )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbScratch )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    PSYMCRYPT_INT piCiphertext = NULL;
    PSYMCRYPT_INT piPlaintext = NULL;
    UINT32 cbInt = 0;

    PSYMCRYPT_INT piTmp = NULL;
    UINT32 cbTmp = 0;

    PSYMCRYPT_MODELEMENT peCrtElements[SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES] = { 0 };
    UINT32 cbModElements[SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES] = { 0 };
    UINT32 cbModElementTotal = 0;

    // Used to verify decryption
    PSYMCRYPT_INT piVerify = NULL;  // Size equal to cbInt
    PSYMCRYPT_MODELEMENT peVerify = NULL;
    UINT32 cbModElementVerify = 0;

    PBYTE   pbFnScratch = NULL;
    SIZE_T  cbFnScratch = 0;

    UNREFERENCED_PARAMETER( flags );

    // Make sure that the key has a private key
    if (!pkRsakey->hasPrivateKey)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    scError = SymCryptRsaCoreVerifyInput(pkRsakey, pbSrc, cbSrc, numFormat, cbDst, pbScratch, cbScratch);
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Verify that the number of primes does not cause a stack overflow
    if (pkRsakey->nPrimes > SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbInt = SymCryptSizeofIntFromDigits( pkRsakey->nDigitsOfModulus );
    cbTmp = SymCryptSizeofIntFromDigits( pkRsakey->nMaxDigitsOfPrimes );
    for (UINT32 i=0; i<pkRsakey->nPrimes; i++)
    {
        cbModElements[i] = SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( pkRsakey->nBitsOfPrimes[i]);
        cbModElementTotal += cbModElements[i];
    }

    cbModElementVerify = SymCryptSizeofModElementFromModulus( pkRsakey->pmModulus );

    UNREFERENCED_PARAMETER( cbScratch );
    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    //      - nPrimes is at most SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES = 2
    // Thus the following calculation does not overflow cbScratch.
    //
    SYMCRYPT_ASSERT( cbScratch >=
                3*cbInt + cbTmp + cbModElementTotal + cbModElementVerify +
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pkRsakey->nDigitsOfModulus ),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( pkRsakey->nDigitsOfModulus ),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( pkRsakey->nDigitsOfModulus, pkRsakey->nMaxDigitsOfPrimes ),
                     SYMCRYPT_SCRATCH_BYTES_FOR_CRT_SOLUTION( pkRsakey->nMaxDigitsOfPrimes ) ))) );

    pbFnScratch = pbScratch;
    cbFnScratch = cbScratch;

    piPlaintext = SymCryptIntCreate( pbFnScratch, cbFnScratch, pkRsakey->nDigitsOfModulus );
    SYMCRYPT_ASSERT( piPlaintext != NULL );
    pbFnScratch += cbInt;
    cbFnScratch -= cbInt;

    piCiphertext = SymCryptIntCreate( pbFnScratch, cbFnScratch, pkRsakey->nDigitsOfModulus );
    SYMCRYPT_ASSERT( piCiphertext != NULL );
    pbFnScratch += cbInt;
    cbFnScratch -= cbInt;

    piTmp = SymCryptIntCreate( pbFnScratch, cbFnScratch, pkRsakey->nMaxDigitsOfPrimes );
    SYMCRYPT_ASSERT( piTmp != NULL );
    pbFnScratch += cbTmp;
    cbFnScratch -= cbTmp;

    SYMCRYPT_ASSERT( pkRsakey->nPrimes <= SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES );
    for (UINT32 i=0; i<pkRsakey->nPrimes; i++)
    {
        peCrtElements[i] = SymCryptModElementCreate( pbFnScratch, cbFnScratch, pkRsakey->pmPrimes[i] );
        SYMCRYPT_ASSERT( peCrtElements[i] != NULL );
        pbFnScratch += cbModElements[i];
        cbFnScratch -= cbModElements[i];
    }

    piVerify = SymCryptIntCreate( pbFnScratch, cbFnScratch, pkRsakey->nDigitsOfModulus );
    SYMCRYPT_ASSERT( piVerify != NULL );
    pbFnScratch += cbInt;
    cbFnScratch -= cbInt;

    peVerify = SymCryptModElementCreate( pbFnScratch, cbFnScratch, pkRsakey->pmModulus );
    SYMCRYPT_ASSERT( peVerify != NULL );
    pbFnScratch += cbModElementVerify;
    cbFnScratch -= cbModElementVerify;

    // Set the ciphertext
    scError = SymCryptIntSetValue( pbSrc, cbSrc, numFormat, piCiphertext );
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Modular exponentiations
    for (UINT32 i=0; i<pkRsakey->nPrimes; i++)
    {
        // c mod the prime
        // Note: For two equally sized primes we can use straight the faster SymCryptIntToModElement function
        // but for now this is the general case.
        SymCryptIntDivMod(
                piCiphertext,
                SymCryptDivisorFromModulus(pkRsakey->pmPrimes[i]),
                NULL,
                piTmp,
                pbFnScratch,
                cbFnScratch );

        SymCryptIntToModElement( piTmp, pkRsakey->pmPrimes[i], peCrtElements[i], pbFnScratch, cbFnScratch );

        // Modular Exponentiation
        SymCryptModExp(
                pkRsakey->pmPrimes[i],
                peCrtElements[i],
                pkRsakey->piCrtPrivExps[i],    // For now only the first exponent is allowed
                pkRsakey->nBitsOfPrimes[i],    // This is a public value
                0,                             // Side-channel safe modexp
                peCrtElements[i],
                pbFnScratch,
                cbFnScratch );
    }

    // Solve the crt equations
    scError = SymCryptCrtSolve(
            pkRsakey->nPrimes,
            (PCSYMCRYPT_MODULUS *) pkRsakey->pmPrimes,
            (PSYMCRYPT_MODELEMENT *) pkRsakey->peCrtInverses,
            peCrtElements,
            0,
            piPlaintext,
            pbFnScratch,
            cbFnScratch );
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    /*
        A hardware error during RSA decryption can leak the
        prime factors.  For example, suppose the message
        is M and you try to sign it with
        M^d for some decryption exponent d.
        Using the CRT, you compute M^d mod p correctly but
        M^d mod q incorrectly.  Your supposed M^d (mod p*q) is
        then raised to an encryption exponent e
        by the verifier, detects an invalid signature.
        The verifier can also find p via a GCD and factor the modulus.

        To avoid this problem, re-encrypt the supposed M^d
        and verify our own signature.
   */

    // Don't call the full encryption function just the modular exponentiation

    SymCryptIntToModElement( piPlaintext, pkRsakey->pmModulus, peVerify, pbFnScratch, cbFnScratch );

    SymCryptIntSetValueUint64( pkRsakey->au64PubExp[0], piTmp );

    // Modular Exponentiation (Not side-channel safe)
    SymCryptModExp(
            pkRsakey->pmModulus,
            peVerify,
            piTmp,
            SymCryptIntBitsizeOfValue( piTmp ),
            SYMCRYPT_FLAG_DATA_PUBLIC,          // Exponent is public
            peVerify,
            pbFnScratch,
            cbFnScratch );

    SymCryptModElementToInt( pkRsakey->pmModulus, peVerify, piVerify, pbFnScratch, cbFnScratch );

    if (!SymCryptIntIsEqual( piCiphertext, piVerify ))
    {
        scError = SYMCRYPT_HARDWARE_FAILURE;
        goto cleanup;
    }

    // Output the result
    scError = SymCryptIntGetValue( piPlaintext, pbDst, cbDst, numFormat );
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

cleanup:

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaCoreDec(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_NUMBER_FORMAT      numFormat,
                                UINT32                      flags,
    _Out_writes_( cbDst )       PBYTE                       pbDst,
                                SIZE_T                      cbDst,
    _Out_writes_bytes_( cbScratch )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbScratch )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    PSYMCRYPT_MODELEMENT peRes = NULL;
    UINT32 cbModElement = 0;

    PBYTE   pbFnScratch = NULL;
    SIZE_T  cbFnScratch = 0;

    UNREFERENCED_PARAMETER( flags );

    // Make sure that the key has a private key
    if ((cbSrc>SymCryptRsakeySizeofModulus(pkRsakey)) ||
        (!pkRsakey->hasPrivateKey) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbModElement = SymCryptSizeofModElementFromModulus( pkRsakey->pmModulus );

    UNREFERENCED_PARAMETER( cbScratch );
    SYMCRYPT_ASSERT( cbScratch >= cbModElement +
                     SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pkRsakey->nDigitsOfModulus ),
                          SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( pkRsakey->nDigitsOfModulus ) ) );

    pbFnScratch = pbScratch;
    cbFnScratch = cbScratch;

    peRes = SymCryptModElementCreate( pbScratch, cbModElement, pkRsakey->pmModulus );
    SYMCRYPT_ASSERT( peRes != NULL );
    pbFnScratch += cbModElement;
    cbFnScratch -= cbModElement;

    // Set the ciphertext
    scError = SymCryptModElementSetValue( pbSrc, cbSrc, numFormat, pkRsakey->pmModulus, peRes, pbFnScratch, cbFnScratch );
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Modular Exponentiation
    SymCryptModExp(
            pkRsakey->pmModulus,
            peRes,
            pkRsakey->piPrivExps[0],    // For now only the first exponent is allowed
            pkRsakey->nBitsOfModulus,   // This is a public value
            0,                          // Side-channel safe modexp
            peRes,
            pbFnScratch,
            cbFnScratch );

    // Output the value
    scError = SymCryptModElementGetValue( pkRsakey->pmModulus, peRes, pbDst, cbDst, numFormat, pbFnScratch, cbFnScratch );
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

cleanup:

    return scError;
}


//
// Encryption / decryption functions
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaRawEncrypt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_NUMBER_FORMAT      numFormat,
                                UINT32                      flags,
    _Out_writes_( cbDst )       PBYTE                       pbDst,
                                SIZE_T                      cbDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PBYTE   pbScratch = NULL;
    UINT32  cbScratch = 0;

    // Make sure that the key may be used in Encrypt/Decrypt
    if ( (pkRsakey->fAlgorithmInfo & SYMCRYPT_FLAG_RSAKEY_ENCRYPT) == 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbScratch = SymCryptRsaCoreEncScratchSpace( pkRsakey );

    pbScratch = (PBYTE)SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    scError = SymCryptRsaCoreEnc( pkRsakey, pbSrc, cbSrc, numFormat, flags, pbDst, cbDst, pbScratch, cbScratch );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaRawDecrypt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_NUMBER_FORMAT      numFormat,
                                UINT32                      flags,
    _Out_writes_( cbDst )       PBYTE                       pbDst,
                                SIZE_T                      cbDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PBYTE   pbScratch = NULL;
    UINT32  cbScratch = 0;

    // Make sure that the key may be used in Encrypt/Decrypt
    if ( (pkRsakey->fAlgorithmInfo & SYMCRYPT_FLAG_RSAKEY_ENCRYPT) == 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Make sure that the key has a private key
    if (!pkRsakey->hasPrivateKey)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

#define SYMCRYPT_CRT_DECRYPTION     (1)     // Set this to 0 to test the non-crt decryption

    // Scratch space
#if (SYMCRYPT_CRT_DECRYPTION)
    cbScratch = SymCryptRsaCoreDecCrtScratchSpace( pkRsakey );
#else
    cbScratch = SymCryptRsaCoreDecScratchSpace( pkRsakey );
#endif

    pbScratch = (PBYTE)SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

#if (SYMCRYPT_CRT_DECRYPTION)
    scError = SymCryptRsaCoreDecCrt( pkRsakey, pbSrc, cbSrc, numFormat, flags, pbDst, cbDst, pbScratch, cbScratch );
#else
    scError = SymCryptRsaCoreDec( pkRsakey, pbSrc, cbSrc, numFormat, flags, pbDst, cbDst, pbScratch, cbScratch );
#endif

    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1Encrypt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                UINT32                      flags,
                                SYMCRYPT_NUMBER_FORMAT      nfDst,
    _Out_writes_opt_( cbDst )   PBYTE                       pbDst,
                                SIZE_T                      cbDst,
    _Out_                       SIZE_T                      *pcbDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;

    PBYTE   pbTmp = NULL;
    SIZE_T  cbTmp = SymCryptRsakeySizeofModulus(pkRsakey);

    cbScratch = cbTmp + SymCryptRsaCoreEncScratchSpace( pkRsakey );

    UNREFERENCED_PARAMETER( flags );

    // Make sure that the key may be used in Encrypt/Decrypt
    if ( (pkRsakey->fAlgorithmInfo & SYMCRYPT_FLAG_RSAKEY_ENCRYPT) == 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    *pcbDst = cbTmp;

    // Check if only *pcbDst is needed
    if (pbDst == NULL)
    {
        scError = SYMCRYPT_NO_ERROR;
        goto cleanup;
    }

    pbScratch = (PBYTE)SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pbTmp = pbScratch + cbScratch - cbTmp;

    scError = SymCryptRsaPkcs1ApplyEncryptionPadding(
                    pbSrc,
                    cbSrc,
                    pbTmp,
                    cbTmp );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SymCryptRsaCoreEnc(
                    pkRsakey,
                    pbTmp,
                    cbTmp,
                    SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,   // Always MSB first for RSA OAEP
                    flags,
                    pbDst,
                    cbDst,
                    pbScratch,
                    cbScratch - cbTmp );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    if (nfDst == SYMCRYPT_NUMBER_FORMAT_LSB_FIRST)
    {
        // To implement this revert the buffer properly
        scError = SYMCRYPT_NOT_IMPLEMENTED;
        goto cleanup;
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}

// Ensure SymCryptRoundUpPow2Sizet below will not fail
C_ASSERT((UINT32) ((SYMCRYPT_RSAKEY_MAX_BITSIZE_MODULUS + 7) / 8) <= ((SIZE_T_MAX / 2) + 1));

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1Decrypt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_NUMBER_FORMAT      nfSrc,
                                UINT32                      flags,
    _Out_writes_opt_( cbDst )   PBYTE                       pbDst,
                                SIZE_T                      cbDst,
    _Out_                       SIZE_T                      *pcbDst )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;

    PBYTE   pbTmp = NULL;
    SIZE_T  cbModulus = SymCryptRsakeySizeofModulus(pkRsakey);
    SIZE_T  cbTmp = SymCryptRoundUpPow2Sizet( cbModulus );      // tmp buffer needs to be a power of 2

    // Make sure that the key may be used in Encrypt/Decrypt
    if ( (pkRsakey->fAlgorithmInfo & SYMCRYPT_FLAG_RSAKEY_ENCRYPT) == 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Make sure that the key has a private key
    if (!pkRsakey->hasPrivateKey)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

#if (SYMCRYPT_CRT_DECRYPTION)
    cbScratch = cbTmp + SymCryptRsaCoreDecCrtScratchSpace( pkRsakey );
#else
    cbScratch = cbTmp + SymCryptRsaCoreDecScratchSpace( pkRsakey );
#endif

    UNREFERENCED_PARAMETER( flags );

    pbScratch = (PBYTE)SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pbTmp = pbScratch + cbScratch - cbTmp;

    if (nfSrc == SYMCRYPT_NUMBER_FORMAT_LSB_FIRST)
    {
        // To implement this revert the buffer properly
        scError = SYMCRYPT_NOT_IMPLEMENTED;
        goto cleanup;
    }

#if (SYMCRYPT_CRT_DECRYPTION)
    scError = SymCryptRsaCoreDecCrt(
                pkRsakey,
                pbSrc,
                cbSrc,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                flags,
                pbTmp,
                cbModulus,
                pbScratch,
                cbScratch - cbTmp );
#else
        scError = SymCryptRsaCoreDec(
                pkRsakey,
                pbSrc,
                cbSrc,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                flags,
                pbTmp,
                cbModulus,
                pbScratch,
                cbScratch - cbTmp );
#endif
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SymCryptRsaPkcs1RemoveEncryptionPadding(
                pbTmp,
                cbModulus,
                cbTmp,
                pbDst,
                cbDst,
                pcbDst );
    // The error that is returned from the encryption padding is confidential data
    // due to Bleichenbacher-style attacks.
    // Make sure we don't create a side-channel leak for it.

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaOaepEncrypt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
    _In_                        PCSYMCRYPT_HASH             hashAlgorithm,
    _In_reads_bytes_( cbLabel ) PCBYTE                      pbLabel,
                                SIZE_T                      cbLabel,
                                UINT32                      flags,
                                SYMCRYPT_NUMBER_FORMAT      nfDst,
    _Out_writes_opt_( cbDst )   PBYTE                       pbDst,
                                SIZE_T                      cbDst,
    _Out_                       SIZE_T                      *pcbDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;

    PBYTE   pbTmp = NULL;
    SIZE_T  cbTmp = SymCryptRsakeySizeofModulus(pkRsakey);

    UNREFERENCED_PARAMETER( flags );

    // Make sure that the key may be used in Encrypt/Decrypt
    if ( (pkRsakey->fAlgorithmInfo & SYMCRYPT_FLAG_RSAKEY_ENCRYPT) == 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    *pcbDst = cbTmp;

    // Check if only *pcbDst is needed
    if (pbDst == NULL)
    {
        scError = SYMCRYPT_NO_ERROR;
        goto cleanup;
    }

    // The SYMCRYPT_SCRATCH_BYTES_FOR_RSA_OAEP macro does not
    // overflow cbScratch since cbTmp < 2^17.
    cbScratch = cbTmp + SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_RSA_OAEP( hashAlgorithm, cbTmp ), SymCryptRsaCoreEncScratchSpace( pkRsakey ) );
    pbScratch = (PBYTE)SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pbTmp = pbScratch + cbScratch - cbTmp;

    scError = SymCryptRsaOaepApplyEncryptionPadding(
                    pbSrc,
                    cbSrc,
                    hashAlgorithm,
                    pbLabel,
                    cbLabel,
                    NULL,               // Seed
                    0,                  // cbSeed
                    pbTmp,
                    cbTmp,
                    pbScratch,
                    cbScratch - cbTmp );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SymCryptRsaCoreEnc(
                    pkRsakey,
                    pbTmp,
                    cbTmp,
                    SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,   // Always MSB first for RSA OAEP
                    flags,
                    pbDst,
                    cbDst,
                    pbScratch,
                    cbScratch - cbTmp );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    if (nfDst == SYMCRYPT_NUMBER_FORMAT_LSB_FIRST)
    {
        // To implement this revert the buffer properly
        scError = SYMCRYPT_NOT_IMPLEMENTED;
        goto cleanup;
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaOaepDecrypt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_NUMBER_FORMAT      nfSrc,
    _In_                        PCSYMCRYPT_HASH             hashAlgorithm,
    _In_reads_bytes_( cbLabel ) PCBYTE                      pbLabel,
                                SIZE_T                      cbLabel,
                                UINT32                      flags,
    _Out_writes_opt_( cbDst )   PBYTE                       pbDst,
                                SIZE_T                      cbDst,
    _Out_                       SIZE_T                      *pcbDst )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;
    SIZE_T  cbDstResult = 0;        // We always return a value into *pcbDst

    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;

    PBYTE   pbTmp = NULL;
    SIZE_T  cbTmp = SymCryptRsakeySizeofModulus(pkRsakey);

    UNREFERENCED_PARAMETER( flags );

    // Make sure that the key may be used in Encrypt/Decrypt
    if ( (pkRsakey->fAlgorithmInfo & SYMCRYPT_FLAG_RSAKEY_ENCRYPT) == 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (cbSrc > cbTmp)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Make sure that the key has a private key
    if (!pkRsakey->hasPrivateKey)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // The SYMCRYPT_SCRATCH_BYTES_FOR_RSA_OAEP macro does not
    // overflow cbScratch since cbTmp < 2^17.
#if (SYMCRYPT_CRT_DECRYPTION)
    cbScratch = cbTmp + SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_RSA_OAEP( hashAlgorithm, cbSrc ), SymCryptRsaCoreDecCrtScratchSpace( pkRsakey ) );
#else
    cbScratch = cbTmp + SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_RSA_OAEP( hashAlgorithm, cbSrc ), SymCryptRsaCoreDecScratchSpace( pkRsakey ) );
#endif

    pbScratch = (PBYTE)SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pbTmp = pbScratch + cbScratch - cbTmp;

    if (nfSrc == SYMCRYPT_NUMBER_FORMAT_LSB_FIRST)
    {
        // To implement this revert the buffer properly
        scError = SYMCRYPT_NOT_IMPLEMENTED;
        goto cleanup;
    }

#if (SYMCRYPT_CRT_DECRYPTION)
    scError = SymCryptRsaCoreDecCrt(
                pkRsakey,
                pbSrc,
                cbSrc,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                flags,
                pbTmp,
                cbTmp,
                pbScratch,
                cbScratch - cbTmp );
#else
    scError = SymCryptRsaCoreDec(
                pkRsakey,
                pbSrc,
                cbSrc,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                flags,
                pbTmp,
                cbTmp,
                pbScratch,
                cbScratch - cbTmp );
#endif

    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SymCryptRsaOaepRemoveEncryptionPadding(
                    pbTmp,
                    cbTmp,
                    hashAlgorithm,
                    pbLabel,
                    cbLabel,
                    flags,
                    pbDst,
                    cbDst,
                    &cbDstResult,
                    pbScratch,
                    cbScratch - cbTmp );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    *pcbDst = cbDstResult;

    return scError;
}

//
// Signing / Verification functions
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1Sign(
    _In_                                PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                      pbHashValue,
                                        SIZE_T                      cbHashValue,
    _In_                                PCSYMCRYPT_OID              pHashOIDs,
    _In_                                SIZE_T                      nOIDCount,
                                        UINT32                      flags,
                                        SYMCRYPT_NUMBER_FORMAT      nfSignature,
    _Out_writes_opt_( cbSignature )     PBYTE                       pbSignature,
                                        SIZE_T                      cbSignature,
    _Out_                               SIZE_T                      *pcbSignature )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;

    PBYTE   pbTmp = NULL;
    SIZE_T  cbTmp = SymCryptRsakeySizeofModulus(pkRsakey);

    PCBYTE pbOID = NULL;
    SIZE_T cbOID = 0;

    UNREFERENCED_PARAMETER(nOIDCount);

    pbOID = pHashOIDs ? pHashOIDs->pbOID : NULL;
    cbOID = pHashOIDs ? pHashOIDs->cbOID : 0;

    // Make sure that the key may be used in Sign/Verify
    if ( (pkRsakey->fAlgorithmInfo & SYMCRYPT_FLAG_RSAKEY_SIGN) == 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Make sure that the key has a private key
    if (!pkRsakey->hasPrivateKey)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    *pcbSignature = cbTmp;

    // Check if only *pcbSignature is needed
    if (pbSignature == NULL)
    {
        scError = SYMCRYPT_NO_ERROR;
        goto cleanup;
    }

#if (SYMCRYPT_CRT_DECRYPTION)
    cbScratch = cbTmp + SymCryptRsaCoreDecCrtScratchSpace( pkRsakey );
#else
    cbScratch = cbTmp + SymCryptRsaCoreDecScratchSpace( pkRsakey );
#endif

    pbScratch = (PBYTE)SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pbTmp = pbScratch + cbScratch - cbTmp;

    scError = SymCryptRsaPkcs1ApplySignaturePadding(
                        pbHashValue,
                        cbHashValue,
                        pbOID,
                        cbOID,
                        flags,
                        pbTmp,
                        cbTmp );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

#if (SYMCRYPT_CRT_DECRYPTION)
    scError = SymCryptRsaCoreDecCrt(
                pkRsakey,
                pbTmp,
                cbTmp,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                flags,
                pbSignature,
                cbSignature,
                pbScratch,
                cbScratch - cbTmp );
#else
    scError = SymCryptRsaCoreDec(
                pkRsakey,
                pbTmp,
                cbTmp,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                flags,
                pbSignature,
                cbSignature,
                pbScratch,
                cbScratch - cbTmp );
#endif
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    if (nfSignature == SYMCRYPT_NUMBER_FORMAT_LSB_FIRST)
    {
        // To implement this revert the buffer properly
        scError = SYMCRYPT_NOT_IMPLEMENTED;
        goto cleanup;
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1Verify(
    _In_                                PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                      pbHashValue,
                                        SIZE_T                      cbHashValue,
    _In_reads_bytes_( cbSignature )     PCBYTE                      pbSignature,
                                        SIZE_T                      cbSignature,
                                        SYMCRYPT_NUMBER_FORMAT      nfSignature,
    _In_reads_opt_( nOIDCount )         PCSYMCRYPT_OID              pHashOIDs,
    _In_                                SIZE_T                      nOIDCount,
                                        UINT32                      flags )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;

    PBYTE   pbTmp = NULL;
    SIZE_T  cbTmp = SymCryptRsakeySizeofModulus(pkRsakey);

    // Make sure that the key may be used in Sign/Verify
    if ( (pkRsakey->fAlgorithmInfo & SYMCRYPT_FLAG_RSAKEY_SIGN) == 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (cbSignature > cbTmp)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (nfSignature == SYMCRYPT_NUMBER_FORMAT_LSB_FIRST)
    {
        // To implement this revert the buffer properly
        scError = SYMCRYPT_NOT_IMPLEMENTED;
        goto cleanup;
    }

    // The SYMCRYPT_SCRATCH_BYTES_FOR_RSA_PKCS1 macro does not
    // overflow cbScratch since cbTmp < 2^17.
    cbScratch = cbTmp +
                SYMCRYPT_MAX( SymCryptRsaCoreEncScratchSpace( pkRsakey ),
                     SYMCRYPT_SCRATCH_BYTES_FOR_RSA_PKCS1( cbTmp ) );

    pbScratch = (PBYTE)SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pbTmp = pbScratch + cbScratch - cbTmp;

    scError = SymCryptRsaCoreEnc(
                pkRsakey,
                pbSignature,
                cbSignature,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                flags,
                pbTmp,
                cbTmp,
                pbScratch,
                cbScratch - cbTmp );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SymCryptRsaPkcs1VerifySignaturePadding(
                    pbHashValue,
                    cbHashValue,
                    pHashOIDs,
                    nOIDCount,
                    pbTmp,
                    cbTmp,
                    flags,
                    pbScratch,
                    cbScratch - cbTmp );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPssSign(
    _In_                                PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                      pbHashValue,
                                        SIZE_T                      cbHashValue,
    _In_                                PCSYMCRYPT_HASH             hashAlgorithm,
                                        SIZE_T                      cbSalt,
                                        UINT32                      flags,
                                        SYMCRYPT_NUMBER_FORMAT      nfSignature,
    _Out_writes_opt_( cbSignature )     PBYTE                       pbSignature,
                                        SIZE_T                      cbSignature,
    _Out_                               SIZE_T                      *pcbSignature )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;

    PBYTE   pbTmp = NULL;
    SIZE_T  cbTmp = SymCryptRsakeySizeofModulus(pkRsakey);

    // Make sure that the key may be used in Sign/Verify
    if ( (pkRsakey->fAlgorithmInfo & SYMCRYPT_FLAG_RSAKEY_SIGN) == 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if ( (cbHashValue > cbTmp) ||
         (cbSalt > cbTmp) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Make sure that the key has a private key
    if (!pkRsakey->hasPrivateKey)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    *pcbSignature = cbTmp;

    // Check if only *pcbSignature is needed
    if (pbSignature == NULL)
    {
        scError = SYMCRYPT_NO_ERROR;
        goto cleanup;
    }

    // The SYMCRYPT_SCRATCH_BYTES_FOR_RSA_PSS macro does not
    // overflow cbScratch since cbTmp < 2^17.
#if (SYMCRYPT_CRT_DECRYPTION)
    cbScratch = cbTmp +
                SYMCRYPT_MAX( SymCryptRsaCoreDecCrtScratchSpace( pkRsakey ),
                     SYMCRYPT_SCRATCH_BYTES_FOR_RSA_PSS( hashAlgorithm, cbHashValue, cbTmp ) );
#else
    cbScratch = cbTmp +
                SYMCRYPT_MAX( SymCryptRsaCoreDecScratchSpace( pkRsakey ),
                     SYMCRYPT_SCRATCH_BYTES_FOR_RSA_PSS( hashAlgorithm, cbHashValue, cbTmp ) );
#endif

    pbScratch = (PBYTE)SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pbTmp = pbScratch + cbScratch - cbTmp;

    scError = SymCryptRsaPssApplySignaturePadding(
                        pbHashValue,
                        cbHashValue,
                        hashAlgorithm,
                        NULL,       // For now only random salt supported
                        cbSalt,
                        pkRsakey->nBitsOfModulus,
                        flags,
                        pbTmp,
                        cbTmp,
                        pbScratch,
                        cbScratch - cbTmp );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

#if (SYMCRYPT_CRT_DECRYPTION)
    scError = SymCryptRsaCoreDecCrt(
                pkRsakey,
                pbTmp,
                cbTmp,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                flags,
                pbSignature,
                cbSignature,
                pbScratch,
                cbScratch - cbTmp );
#else
    scError = SymCryptRsaCoreDec(
                pkRsakey,
                pbTmp,
                cbTmp,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                flags,
                pbSignature,
                cbSignature,
                pbScratch,
                cbScratch - cbTmp );
#endif
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    if (nfSignature == SYMCRYPT_NUMBER_FORMAT_LSB_FIRST)
    {
        // To implement this revert the buffer properly
        scError = SYMCRYPT_NOT_IMPLEMENTED;
        goto cleanup;
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPssVerify(
    _In_                                PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                      pbHashValue,
                                        SIZE_T                      cbHashValue,
    _In_reads_bytes_( cbSignature )     PCBYTE                      pbSignature,
                                        SIZE_T                      cbSignature,
                                        SYMCRYPT_NUMBER_FORMAT      nfSignature,
    _In_                                PCSYMCRYPT_HASH             hashAlgorithm,
                                        SIZE_T                      cbSalt,
                                        UINT32                      flags )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    PBYTE   pbScratch = NULL;
    SIZE_T  cbScratch = 0;

    PBYTE   pbTmp = NULL;
    SIZE_T  cbTmp = SymCryptRsakeySizeofModulus(pkRsakey);

    // Make sure that the key may be used in Sign/Verify
    if ( (pkRsakey->fAlgorithmInfo & SYMCRYPT_FLAG_RSAKEY_SIGN) == 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if ( (cbHashValue > cbTmp) ||
         (cbSalt > cbTmp) ||
         (cbSignature > cbTmp) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (nfSignature == SYMCRYPT_NUMBER_FORMAT_LSB_FIRST)
    {
        // To implement this revert the buffer properly
        scError = SYMCRYPT_NOT_IMPLEMENTED;
        goto cleanup;
    }

    // The SYMCRYPT_SCRATCH_BYTES_FOR_RSA_PSS macro does not
    // overflow cbScratch since cbTmp < 2^17.
    cbScratch = cbTmp +
                SYMCRYPT_MAX( SymCryptRsaCoreEncScratchSpace( pkRsakey ),
                     SYMCRYPT_SCRATCH_BYTES_FOR_RSA_PSS( hashAlgorithm, cbHashValue, cbTmp ) );

    pbScratch = (PBYTE)SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pbTmp = pbScratch + cbScratch - cbTmp;

    scError = SymCryptRsaCoreEnc(
                pkRsakey,
                pbSignature,
                cbSignature,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                flags,
                pbTmp,
                cbTmp,
                pbScratch,
                cbScratch - cbTmp );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SymCryptRsaPssVerifySignaturePadding(
                pbHashValue,
                cbHashValue,
                hashAlgorithm,
                cbSalt,
                pbTmp,
                cbTmp,
                pkRsakey->nBitsOfModulus,
                flags,
                pbScratch,
                cbScratch - cbTmp );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}
