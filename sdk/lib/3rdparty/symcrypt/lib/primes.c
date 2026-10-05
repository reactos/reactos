//
// primes.c
// Primality tests and prime number generation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

UINT32
SYMCRYPT_CALL
SymCryptIntMillerRabinPrimalityTest(
    _In_                            PCSYMCRYPT_INT      piSrc,
                                    UINT32              nBitsSrc,
                                    UINT32              nIterations,
                                    UINT32              flags,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch )
{
    BOOLEAN     innerLoop = TRUE;
    UINT32      borrow = 0;

    UINT32      nDigitsSrc = 0;

    UINT32                  R = 1;
    PSYMCRYPT_INT           piD = NULL;
    UINT32                  cbD = 0;
    PSYMCRYPT_MODULUS       pmModulus = NULL;
    UINT32                  cbModulus = 0;
    PSYMCRYPT_MODELEMENT    peX = NULL;
    UINT32                  cbX = 0;

    PSYMCRYPT_MODELEMENT    peOne = NULL;
    PSYMCRYPT_MODELEMENT    peMinOne = NULL;

    nDigitsSrc = SymCryptIntDigitsizeOfObject( piSrc );
    cbD = SymCryptSizeofIntFromDigits( nDigitsSrc );
    cbModulus = SymCryptSizeofModulusFromDigits( nDigitsSrc );

    SYMCRYPT_ASSERT( nBitsSrc >= SymCryptIntBitsizeOfValue( piSrc ) );

    SYMCRYPT_ASSERT( cbScratch >= cbModulus + SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_MODULUS(nDigitsSrc) );

    // Allocate the modulus
    pmModulus = SymCryptModulusCreate( pbScratch, cbModulus, nDigitsSrc );
    SYMCRYPT_ASSERT( pmModulus != NULL );
    pbScratch += cbModulus;
    cbScratch -= cbModulus;

    // Set the modulus
    SymCryptIntToModulus(
            piSrc,
            pmModulus,
            nBitsSrc,       // Average number of expected operations
            SYMCRYPT_FLAG_MODULUS_PARITY_PUBLIC,
            pbScratch,
            cbScratch );

    // Modelement size
    cbX = SymCryptSizeofModElementFromModulus( pmModulus );

    SYMCRYPT_ASSERT( cbScratch >=   3*cbX + cbD +
                                    SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigitsSrc ) );

    peX = SymCryptModElementCreate( pbScratch, cbX, pmModulus );
    SYMCRYPT_ASSERT( peX != NULL );
    pbScratch += cbX;
    cbScratch -= cbX;

    peOne = SymCryptModElementCreate( pbScratch, cbX, pmModulus );
    SYMCRYPT_ASSERT( peOne != NULL );
    pbScratch += cbX;
    cbScratch -= cbX;

    peMinOne = SymCryptModElementCreate( pbScratch, cbX, pmModulus );
    SYMCRYPT_ASSERT( peMinOne != NULL );
    pbScratch += cbX;
    cbScratch -= cbX;

    // Allocate D
    piD = SymCryptIntCreate( pbScratch, cbD, nDigitsSrc );
    SYMCRYPT_ASSERT( piD != NULL );
    pbScratch += cbD;
    cbScratch -= cbD;

    // Calculate (piSrc - 1)
    // Note: We should never get a borrow here because the requirement
    // is that Src > 3.
    SymCryptIntCopy( piSrc, piD );
    borrow = SymCryptIntSubUint32( piD, 1, piD );
    SYMCRYPT_ASSERT( borrow==0 );

    SYMCRYPT_ASSERT( SymCryptIntGetBit( piD, 0 ) == 0 );

    // Check the 3 mod 4 requirement when side-channel safe
    SYMCRYPT_ASSERT(
            ((flags & SYMCRYPT_FLAG_DATA_PUBLIC) != 0) ||
            (SymCryptIntGetBit( piD, 1 )!=0) );
    UNREFERENCED_PARAMETER( flags );

    // Calculate R and D such that Src - 1 = D*2^R
    //      Notice that the loop executes only if
    //      the SYMCRYPT_FLAG_DATA_PUBLIC is
    //      specified (and Src != 3 mod 4)
    R = 1;
    while( SymCryptIntGetBit( piD, R )==0 )
    {
        R++;
    }
    SymCryptIntDivPow2( piD, R, piD );

    // Set peOne and peMinOne
    SymCryptModElementSetValueUint32( 1, pmModulus, peOne, pbScratch, cbScratch );
    SymCryptModElementSetValueNegUint32( 1, pmModulus, peMinOne, pbScratch, cbScratch );

    for (UINT32 i=0; i<nIterations; i++)
    {
        // Pick a random X in [2, piSrc-2]
        // Therefore the flags parameter is 0 (default: not allowed 0, 1, -1 when modulus > 3)
        SymCryptModSetRandom( pmModulus, peX, 0, pbScratch, cbScratch );

        // X^D mod piSrc
        // Notice that nBitsSrc is public in the call of SymCryptModExp
        SymCryptModExp( pmModulus, peX, piD, nBitsSrc, 0, peX, pbScratch, cbScratch );

        // Check for 1 or -1
        if ( SymCryptModElementIsEqual( pmModulus, peX, peOne ) |
             SymCryptModElementIsEqual( pmModulus, peX, peMinOne ) )
        {
            continue;
        }

        // repeat R-1 times
        //      Notice that the inner loop executes only if
        //      the SYMCRYPT_FLAG_DATA_PUBLIC is
        //      specified (and Src != 3 mod 4)
        innerLoop = TRUE;
        for (UINT32 j=0; (j<R-1)&&(innerLoop); j++)
        {
            // Square X
            SymCryptModSquare( pmModulus, peX, peX, pbScratch, cbScratch );

            // Check if it is 1
            if (SymCryptModElementIsEqual( pmModulus, peX, peOne ))
            {
                return 0x0;
            }

            // Check if it is -1
            if (SymCryptModElementIsEqual( pmModulus, peX, peMinOne ))
            {
                innerLoop = FALSE;
                break;
            }
        }

        if (innerLoop)
        {
            return 0x0;
        }
    }

    return  0xffffffff;     // Prime
}

#define SYMCRYPT_PRIME_GENERATION_MR_ITERATIONS (64)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptIntGenerateRandomPrime(
    _In_                            PCSYMCRYPT_INT      piLow,
    _In_                            PCSYMCRYPT_INT      piHigh,
    _In_reads_opt_( nPubExp )       PCUINT64            pu64PubExp,
                                    UINT32              nPubExp,
                                    UINT32              nTries,
                                    UINT32              flags,
    _Inout_                         PSYMCRYPT_INT       piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_EXTERNAL_FAILURE;
    PSYMCRYPT_DIVISOR pdPubExp[ SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS ];
    PSYMCRYPT_INT piTmp;

    UINT32 cnt = 0;
    UINT32 e;
    BOOLEAN reject;
    SIZE_T cbObj;

    UINT32 nBits = SymCryptIntBitsizeOfObject(piDst);
    UINT32 nBytes = (nBits + 7)/8;

    UINT32 nBitsHigh = SymCryptIntBitsizeOfValue( piHigh );

    PCSYMCRYPT_TRIALDIVISION_CONTEXT pTrialDivisionContext = SymCryptCreateTrialDivisionContext( SymCryptIntDigitsizeOfObject( piHigh ) );

    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_INT_PRIME_GEN( SymCryptIntDigitsizeOfObject( piDst ) ) );
    SYMCRYPT_ASSERT( nPubExp <= SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS );
    SYMCRYPT_ASSERT( SymCryptDigitsFromBits( 64 ) == 1 );

    UNREFERENCED_PARAMETER( flags );

    // Allocate divisor objects for each public exponent & initialize them
    cbObj = SymCryptSizeofDivisorFromDigits( 1 );
    for( e = 0; e < nPubExp; e++ )
    {
        SYMCRYPT_ASSERT( cbScratch >= cbObj );
        if( pu64PubExp[e] == 0 )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto exit;
        }

        pdPubExp[e] = SymCryptDivisorCreate( pbScratch, cbObj, 1 );
        pbScratch += cbObj;
        cbScratch -= cbObj;

        SymCryptIntSetValueUint64( pu64PubExp[e], SymCryptIntFromDivisor( pdPubExp[e] ) );
        SymCryptIntToDivisor( SymCryptIntFromDivisor( pdPubExp[e] ), pdPubExp[e], 1000, SYMCRYPT_FLAG_DATA_PUBLIC, pbScratch, cbScratch );
    }

    cbObj = SymCryptSizeofIntFromDigits( 1 );
    SYMCRYPT_ASSERT( cbScratch >= cbObj + nBytes );
    piTmp = SymCryptIntCreate( pbScratch, cbObj, 1 );
    pbScratch += cbObj;
    cbScratch -= cbObj;

    do
    {
        cnt++;

        scError = SymCryptCallbackRandom( pbScratch, nBytes );
        if (scError != SYMCRYPT_NO_ERROR)
        {
            goto exit;
        }

        scError = SymCryptIntSetValue( pbScratch, nBytes, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piDst );
        if (scError != SYMCRYPT_NO_ERROR)
        {
            goto exit;
        }

        // Set the integer to 3 mod 4
        SymCryptIntSetBits( piDst, 3, 0, 2 );

        // Zero out the top bits above the upper limit
        SymCryptIntModPow2( piDst, nBitsHigh, piDst );

        // Check if it is in the correct range
        if ( (SymCryptIntIsLessThan( piDst, piLow )) ||
             (!SymCryptIntIsLessThan( piDst, piHigh )) )
        {
            continue;
        }

        // Fast compositeness check
        if( SymCryptIntFindSmallDivisor( pTrialDivisionContext, piDst, NULL, 0 ) != 0 )
        {
            // We found a small divisor; it is not a prime
            continue;
        }

        // Check for compatibility with public exponents (if provided)
        reject = FALSE;
        for( e = 0; e < nPubExp; e++ )
        {
            SymCryptIntDivMod( piDst, pdPubExp[e], NULL, piTmp, pbScratch, cbScratch );

            // Check that e has a modular inverse mod P-1
            // If e and P-1 are coprime, or GCD( P-1, e ) == 1, then e^-1 exists
            // We have (P mod e) in piTmp.
            // If piTmp == 0 then P is divisible by e, and will fail primality test - we don't care about the result of the GCD
            // Otherwise, GCD( (P mod e)-1, e ) == GCD( P-1 mod e, e ) == GCD( P-1, e )
            //
            // Note that if P-1 is a multiple of e then (P mod e)-1 == 0, and GCD( 0, e ) == e
            if( SymCryptUint64Gcd( pu64PubExp[e], SymCryptIntGetValueLsbits64( piTmp ) - 1, SYMCRYPT_FLAG_GCD_INPUTS_NOT_BOTH_EVEN ) != 1 )
            {
                // We can't continue the big loop from here :-(
                reject = TRUE;
                break;
            }
        }
        if( reject )
        {
            continue;
        }

        // Primality check
        if (SymCryptIntMillerRabinPrimalityTest( piDst, nBitsHigh, SYMCRYPT_PRIME_GENERATION_MR_ITERATIONS, 0, pbScratch, cbScratch ))
        {
            scError = SYMCRYPT_NO_ERROR;
            break;
        }
    }
    while (cnt<nTries);

    if (cnt>=nTries)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
    }

exit:
    SymCryptFreeTrialDivisionContext( pTrialDivisionContext );
    return scError;
}
