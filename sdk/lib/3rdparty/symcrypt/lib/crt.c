//
// crt.c   Chinese Remainder Theorem Algorithms
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCrtGenerateForTwoCoprimes(
    _In_    PCSYMCRYPT_MODULUS   pmP,
    _In_    PCSYMCRYPT_MODULUS   pmQ,
            UINT32               flags,
    _Out_   PSYMCRYPT_MODELEMENT peInvQModP,
    _Out_   PSYMCRYPT_MODELEMENT peInvPModQ,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PCSYMCRYPT_INT  piSrc1 = NULL;
    PCSYMCRYPT_INT  piSrc2 = NULL;

    PSYMCRYPT_INT   piInvSrc1ModSrc2 = NULL;
    PSYMCRYPT_INT   piInvSrc2ModSrc1 = NULL;

    UINT32 nDigits = 0;
    UINT32 cbInt = 0;

    BOOLEAN oddP = FALSE;

    SYMCRYPT_ASSERT( pmP != NULL );
    SYMCRYPT_ASSERT( pmQ != NULL );

    nDigits = SYMCRYPT_MAX( SymCryptModulusDigitsizeOfObject( pmP ), SymCryptModulusDigitsizeOfObject( pmQ ));

    // Create two temporary integers
    cbInt = SymCryptSizeofIntFromDigits( nDigits );

    SYMCRYPT_ASSERT( cbScratch >= 2*cbInt + SYMCRYPT_SCRATCH_BYTES_FOR_EXTENDED_GCD( nDigits ));

    piInvSrc1ModSrc2 = SymCryptIntCreate( pbScratch, cbInt, nDigits ); pbScratch += cbInt; cbScratch -= cbInt;
    piInvSrc2ModSrc1 = SymCryptIntCreate( pbScratch, cbInt, nDigits ); pbScratch += cbInt; cbScratch -= cbInt;

    oddP = ((SymCryptIntGetValueLsbits32(SymCryptIntFromModulus( (PSYMCRYPT_MODULUS) pmP )) & 1) == 1);
    if (oddP)
    {
        piSrc1 = SymCryptIntFromModulus( (PSYMCRYPT_MODULUS) pmQ );
        piSrc2 = SymCryptIntFromModulus( (PSYMCRYPT_MODULUS) pmP );
    }
    else
    {
        piSrc1 = SymCryptIntFromModulus( (PSYMCRYPT_MODULUS) pmP );
        piSrc2 = SymCryptIntFromModulus( (PSYMCRYPT_MODULUS) pmQ );
    }

    // IntExtendedGcd requirements:
    //      - First argument > 0
    //      - Second argument odd
    if( SymCryptIntIsEqualUint32(piSrc1, 0) ||
        ((SymCryptIntGetValueLsbits32(piSrc2) & 1) != 1) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Extended GCD
    SymCryptIntExtendedGcd( piSrc1, piSrc2, flags, NULL, NULL, piInvSrc1ModSrc2, piInvSrc2ModSrc1, pbScratch, cbScratch );

    if (oddP)
    {
        SymCryptIntToModElement( piInvSrc2ModSrc1, pmQ, peInvPModQ, pbScratch, cbScratch );
        SymCryptIntToModElement( piInvSrc1ModSrc2, pmP, peInvQModP, pbScratch, cbScratch );
    }
    else
    {
        SymCryptIntToModElement( piInvSrc2ModSrc1, pmP, peInvQModP, pbScratch, cbScratch );
        SymCryptIntToModElement( piInvSrc1ModSrc2, pmQ, peInvPModQ, pbScratch, cbScratch );
    }

cleanup:
    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCrtGenerateInverses(
                                    UINT32                  nCoprimes,
    _In_reads_( nCoprimes )         PCSYMCRYPT_MODULUS *    ppmCoprimes,
                                    UINT32                  flags,
    _Out_writes_( nCoprimes )       PSYMCRYPT_MODELEMENT *  ppeCrtInverses,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    if (nCoprimes == 2)
    {
        SymCryptCrtGenerateForTwoCoprimes(
                ppmCoprimes[0],
                ppmCoprimes[1],
                flags,
                ppeCrtInverses[0],
                ppeCrtInverses[1],
                pbScratch,
                cbScratch );
    }
    else
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCrtSolve(
                                    UINT32                  nCoprimes,
    _In_reads_( nCoprimes )         PCSYMCRYPT_MODULUS *    ppmCoprimes,
    _In_reads_( nCoprimes )         PCSYMCRYPT_MODELEMENT * ppeCrtInverses,
    _In_reads_( nCoprimes )         PCSYMCRYPT_MODELEMENT * ppeCrtRemainders,
                                    UINT32                  flags,
    _Out_                           PSYMCRYPT_INT           piSolution,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SYMCRYPT_ASSERT( nCoprimes >= 2 );

    PSYMCRYPT_INT  piTmp = NULL;
    PSYMCRYPT_MODELEMENT peTmp = NULL;

    PSYMCRYPT_INT  piDouble = NULL;

    UINT32 nDigitsMax = 0;

    UINT32 cbInt = 0;
    UINT32 cbModElement = 0;
    UINT32 cbDouble = 0;

    UINT32 carry = 0;

    UNREFERENCED_PARAMETER( flags );

    nDigitsMax = SYMCRYPT_MAX( SymCryptModulusDigitsizeOfObject( ppmCoprimes[0] ), SymCryptModulusDigitsizeOfObject( ppmCoprimes[1] ) );

    cbInt = SymCryptSizeofIntFromDigits( nDigitsMax );
    cbModElement = SymCryptSizeofModElementFromModulus( ppmCoprimes[0] );
    cbDouble = SymCryptSizeofIntFromDigits( 2*nDigitsMax );

    if( cbDouble == 0 )
    {
        // It is possible that cbDouble would not fit within the maximum integer
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    SYMCRYPT_ASSERT( cbScratch >= cbInt + cbModElement + cbDouble +
                                  SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigitsMax ),
                                       SYMCRYPT_SCRATCH_BYTES_FOR_INT_MUL( 2*nDigitsMax ) )
                    );

    // Create temporaries
    piTmp = SymCryptIntCreate( pbScratch, cbInt, nDigitsMax ); pbScratch += cbInt; cbScratch -= cbInt;

    peTmp = SymCryptModElementCreate( pbScratch, cbModElement, ppmCoprimes[0] ); pbScratch += cbModElement; cbScratch -= cbModElement;

    piDouble = SymCryptIntCreate( pbScratch, cbDouble, 2*nDigitsMax ); pbScratch += cbDouble; cbScratch -= cbDouble;

    if (nCoprimes == 2)
    {
        //
        // Let r0 and r1 be the two remainders modulo p and q respectively
        // Then we calculate (q^{-1}(r0 - r1) mod p)*q + r1
        //
        SymCryptModElementToInt( ppmCoprimes[1], ppeCrtRemainders[1], piTmp, pbScratch, cbScratch );               // Convert r1 to Int
        SymCryptIntToModElement( piTmp, ppmCoprimes[0], peTmp, pbScratch, cbScratch );                             // Convert it to r1 mod p

        SymCryptModSub( ppmCoprimes[0], ppeCrtRemainders[0], peTmp, peTmp, pbScratch, cbScratch );                 // (r0 - r1) mod p
        SymCryptModMul( ppmCoprimes[0], ppeCrtInverses[0], peTmp, peTmp, pbScratch, cbScratch );                   // q^{-1}*(r0 - r1) mod p
        SymCryptModElementToInt( ppmCoprimes[0], peTmp, piTmp, pbScratch, cbScratch );                             // Convert it to integer

        SymCryptIntMulMixedSize( piTmp, SymCryptIntFromModulus((PSYMCRYPT_MODULUS)ppmCoprimes[1]), piDouble, pbScratch, cbScratch );    // Multiply by q
        scError = SymCryptIntCopyMixedSize( piDouble, piSolution );                                                 // Copy it into the solution
        if (scError != SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        SymCryptModElementToInt( ppmCoprimes[1], ppeCrtRemainders[1], piTmp, pbScratch, cbScratch );                // Convert r1 to integer

        carry = SymCryptIntAddMixedSize( piTmp, piSolution, piSolution );                                           // Add it to the solution

        if (carry>0)
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }
    }
    else
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

cleanup:
    return scError;
}
