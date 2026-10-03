//
// IEEE802_11SaeCustom.c  Implementation of the custom crypto of IEEE 802.11 SAE
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//

#include "precomp.h"

// Used in SAE Hunting and Pecking methods where NIST P256 is hardcoded
#define PRIME_LENGTH_BITS 256

//
// This data structure is used to store the associated elliptic curve and the z value corresponding to
// each IANA group mappings for each elliptic
// curve defined in IEEE Std 802.11 SAE method.
//
typedef struct _SYMCRYPT_SAE_GROUP_DATA {
    SYMCRYPT_802_11_SAE_GROUP       group;
    const PCSYMCRYPT_ECURVE_PARAMS *pCurveParams;
    const PCSYMCRYPT_MAC           *macAlgorithm;
    INT32                           z;
} SYMCRYPT_SAE_GROUP_DATA, *PSYMCRYPT_SAE_GROUP_DATA;

typedef const SYMCRYPT_SAE_GROUP_DATA* PCSYMCRYPT_SAE_GROUP_DATA;

//
// Data based on IEEE Std 802.11-2020
// Table 12.1 - Hash algorithm based on length of prime
// Table 12.2 - Unique curve parameter
//
const SYMCRYPT_SAE_GROUP_DATA g_ianaData[] = {
    { SYMCRYPT_SAE_GROUP_19, &SymCryptEcurveParamsNistP256, &SymCryptHmacSha256Algorithm, -10},
    { SYMCRYPT_SAE_GROUP_20, &SymCryptEcurveParamsNistP384, &SymCryptHmacSha384Algorithm, -12},
};

//
// Helper function that finds the associated IANA group data entry for a given group number
// Searches the global variable g_ianaData where the data for supported groups are stored
//
PCSYMCRYPT_SAE_GROUP_DATA SymCryptSaeFindGroupData(SYMCRYPT_802_11_SAE_GROUP ianaGroup)
{
    for (UINT32 index = 0; index < SYMCRYPT_ARRAY_SIZE(g_ianaData); index++ )
    {
        if ( g_ianaData[index].group == ianaGroup )
        {
            return &g_ianaData[index];
        }
    }

    return NULL;
}

//
// Helper function that returns the sizes of the field elements and elliptic curve points in bytes
// for a given IANA group number. Both output parameters are optional.
//
VOID SymCrypt802_11SaeGetGroupSizes(
                                SYMCRYPT_802_11_SAE_GROUP       group,
                    _Out_opt_   SIZE_T*                         pcbScalar,
                    _Out_opt_   SIZE_T*                         pcbPoint )
{
    PCSYMCRYPT_SAE_GROUP_DATA pGroupData = NULL;
    SIZE_T cbScalar = 0;
    SIZE_T cbPoint = 0;

    pGroupData = SymCryptSaeFindGroupData( group );

    if ( pGroupData != NULL )
    {
        cbScalar = ( *( pGroupData->pCurveParams ) )->cbFieldLength;
        cbPoint = 2 * cbScalar;
    }

    if ( pcbScalar != NULL )
    {
        *pcbScalar = cbScalar;
    }

    if ( pcbPoint != NULL )
    {
        *pcbPoint = cbPoint;
    }
}

//
// Calculate sqrt(peVal) if it exists. If so, *puIsQuadraticResidue is set to 0xFFFF`FFFF.
// Otherwise, *puIsQuadraticResidue is set to 0.
// WARNING: *peSqrtArg is set even if the square root doesn't exist. Use masked copy functions
// with *puIsQuadraticResidue so as to use the value of *peSqrtArg only if the square root exists.
//
// - pmMod: Modulus of the curve. Must equal 3 mod 4, which holds for all NIST Prime curves except P224
// - peVal: Value to calculate the square root of
// - puIsQuadraticResidue: mask value, true if sqrt(peVal) exists, false otherwise
// - peSqrtArg: optional out argument for square root value
// - pbScratch, cbScratch: scratch space >= SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( pmMod->nDigits )
//
SYMCRYPT_ERROR
SymCryptModSqrt(
    _In_                            PSYMCRYPT_MODULUS    pmMod,
    _In_                            PSYMCRYPT_MODELEMENT peVal,
    _Out_                           PUINT32              puIsQuadraticResidue,
    _Out_opt_                       PSYMCRYPT_MODELEMENT peSqrtArg,
    _Out_writes_bytes_( cbScratch ) PBYTE                pbScratch,
                                    SIZE_T               cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PSYMCRYPT_INT piTmp = SymCryptIntAllocate( SymCryptDigitsFromBits( pmMod->Divisor.nBits ) );
    PSYMCRYPT_MODELEMENT peSqrt = SymCryptModElementAllocate( pmMod );
    PSYMCRYPT_MODELEMENT peTmp = SymCryptModElementAllocate( pmMod );

    if( piTmp == NULL || peSqrt == NULL || peTmp == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Sqrt( v ) = v^{(P+1)/4} mod P when P = 3 mod 4 as it is here
    SymCryptIntCopy( SymCryptIntFromModulus( pmMod ), piTmp );
    SymCryptIntAddUint32( piTmp, 1, piTmp );      // No overflow as our prime is not 2^256 - 1

    SYMCRYPT_ASSERT( (SymCryptIntGetValueLsbits32(piTmp) & 3) == 0);
    SymCryptIntDivPow2(piTmp, 2, piTmp);
    // iX = (P+1)/4

    // Compute Sqrt( v ) if it exists
    SymCryptModExp( pmMod, peVal, piTmp, pmMod->Divisor.nBits - 2, 0, peSqrt, pbScratch, cbScratch );

    SymCryptModSquare( pmMod, peSqrt, peTmp, pbScratch, cbScratch );
    *puIsQuadraticResidue = SymCryptModElementIsEqual( pmMod, peTmp, peVal );

    if( peSqrtArg != NULL )
    {
        SymCryptModElementCopy( pmMod, peSqrt, peSqrtArg );
    }

cleanup:

    if( piTmp != NULL )
    {
        SymCryptIntFree( piTmp );
        piTmp = NULL;
    }

    if( peSqrt != NULL )
    {
        SymCryptModElementFree( pmMod, peSqrt );
        peSqrt = NULL;
    }

    if( peTmp != NULL )
    {
        SymCryptModElementFree( pmMod, peTmp );
        peTmp = NULL;
    }

    return scError;

}

//
// Calculates SSWU( u ) as described in 12.4.4.2.3
//
// - pCurve: The curve object to use.
// - z: z value used in the SSWU calculation. Currently we assume this value to be negative.
// - peU: Value to calculate SSWU of.
// - popP: point on the curve found by SSWU.
// - pbScratch, cbScratch: scratch space >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve )
//
SYMCRYPT_ERROR
SymCryptSswu(
    _In_                            PSYMCRYPT_ECURVE     pCurve,
    _In_                            INT32                z,
    _In_                            PSYMCRYPT_MODELEMENT peU,
    _Out_                           PSYMCRYPT_ECPOINT    poP,
    _Out_writes_bytes_( cbScratch ) PBYTE                pbScratch,
                                    SIZE_T               cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    UINT32 selectionMask = 0; // Mask variable for masked copy operations. "l" in the spec

    PSYMCRYPT_INT        piTmp = NULL;
    PSYMCRYPT_MODELEMENT peTmp = NULL;

    PSYMCRYPT_MODELEMENT peZ = NULL;
    PSYMCRYPT_MODELEMENT peM = NULL;
    PSYMCRYPT_MODELEMENT peT = NULL;
    PSYMCRYPT_MODELEMENT peX1 = NULL;
    PSYMCRYPT_MODELEMENT peX2 = NULL;
    PSYMCRYPT_MODELEMENT peGX1 = NULL;
    PSYMCRYPT_MODELEMENT peGX2 = NULL;

    BYTE pointBuf[SYMCRYPT_SAE_MAX_EC_POINT_SIZE_BYTES] = { 0 };

    SYMCRYPT_ASSERT( z < 0 );

    piTmp = SymCryptIntAllocate( SymCryptDigitsFromBits( pCurve->FModBitsize ) );

    peTmp = SymCryptModElementAllocate( pCurve->FMod );
    peZ = SymCryptModElementAllocate( pCurve->FMod );
    peM = SymCryptModElementAllocate( pCurve->FMod );
    peT = SymCryptModElementAllocate( pCurve->FMod );
    peX1 = SymCryptModElementAllocate( pCurve->FMod );
    peX2 = SymCryptModElementAllocate( pCurve->FMod );
    peGX1 = SymCryptModElementAllocate( pCurve->FMod );
    peGX2 = SymCryptModElementAllocate( pCurve->FMod );

    if( piTmp == NULL|| peTmp == NULL || peZ == NULL || peM == NULL || peT == NULL ||
        peX1 == NULL || peX2 == NULL || peGX1 == NULL || peGX2 == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Convert z to mod element
    // Currently we avoid a branching based on the sign of z to make the assignment and assume it will
    // be negative which holds for the set of possible values as of now (NIST P256 and NIST P384).
    // There is no direct function to create a SYMCRYPT_INT from a signed INT32, so when z is negative
    // we change its sign and call SymCryptModElementSetValueNegUInt32
    SymCryptModElementSetValueNegUint32(-z, pCurve->FMod, peZ, pbScratch, cbScratch);

    // Set peTmp to 1 for convenience later
    SymCryptModElementSetValueUint32( 1, pCurve->FMod, peTmp, pbScratch, cbScratch );

    // m = ( z^2 * u^4 + z * u^2 ) = (z * u^2)(z * u^2 + 1) modulo p
    SymCryptModSquare( pCurve->FMod, peU, peM, pbScratch, cbScratch ); // M = u^2
    SymCryptModMul( pCurve->FMod, peM, peZ, peM, pbScratch, cbScratch ); // M = z * u^2
    SymCryptModAdd( pCurve->FMod, peM, peTmp, peTmp, pbScratch, cbScratch ); // tmp = (z * u^2 + 1)
    SymCryptModMul( pCurve->FMod, peM, peTmp, peM, pbScratch, cbScratch ); // M = M * tmp = (z * u^2)(z * u^2 + 1)

    // l = CEQ( m, 0 )
    selectionMask = SymCryptModElementIsZero( pCurve->FMod, peM );

    // t = inverse( m ) where inverse ( m ) = m^( p-2 ) modulo p
    SymCryptIntSubUint32( SymCryptIntFromModulus( pCurve->FMod ), 2, piTmp );
    SymCryptModExp( pCurve->FMod, peM, piTmp, pCurve->FModBitsize, 0, peT, pbScratch, cbScratch );

    //x1 = CSEL( l, ( b / ( z * a ) modulo p ), ( ( - b / a ) * ( 1 + t ) ) modulo p )
    // where CSEL(x,y,z) operates in constant time and returns y if x is true and z otherwise.
    SymCryptModMul( pCurve->FMod, peZ, pCurve->A, peTmp, pbScratch, cbScratch ); // tmp = z * a
    SymCryptModInv( pCurve->FMod, peTmp, peTmp, SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME, pbScratch, cbScratch ); // tmp = 1/(z * a)
    SymCryptModMul( pCurve->FMod, pCurve->B, peTmp, peX1, pbScratch, cbScratch ); // x1A = B * 1/(z * a)

    SymCryptModInv( pCurve->FMod, pCurve->A, peTmp, SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME, pbScratch, cbScratch ); // tmp = 1/a
    SymCryptModMul( pCurve->FMod, pCurve->B, peTmp, peTmp, pbScratch, cbScratch ); // tmp = b * 1/a
    SymCryptModNeg( pCurve->FMod, peTmp, peTmp, pbScratch, cbScratch ); // tmp = -(b * 1/a)

    // NB: in this block we're using X2 as the second candidate for CSEL. This allows us to choose the
    // correct X1 by copying X2 to X1 if l is false
    SymCryptIntSetValueUint32( 1, piTmp );
    SymCryptIntToModElement( piTmp, pCurve->FMod, peX2, pbScratch, cbScratch ); // X1B = 1
    SymCryptModAdd( pCurve->FMod, peX2, peT, peX2, pbScratch, cbScratch ); // X1B = 1 + t
    SymCryptModMul( pCurve->FMod, peX2, peTmp, peX2, pbScratch, cbScratch ); // X1B = -(b * 1/a)(1 + t)

    // Note: we need the binary complement of l since MaskedCopy copies only if the mask is 0xFFFFFFFF,
    // and we want the second X1 candidate iff l is false
    SymCryptModElementMaskedCopy( pCurve->FMod, peX2, peX1, ~selectionMask );

    // gx1 = ( x1^3 + a * x1 + b ) = (x1^2 + a)*x1 + b modulo p
    SymCryptModSquare( pCurve->FMod, peX1, peGX1, pbScratch, cbScratch ); // gx1 = x1^2
    SymCryptModAdd( pCurve->FMod, peGX1, pCurve->A, peGX1, pbScratch, cbScratch ); // gx1 = x1^2 + a
    SymCryptModMul( pCurve->FMod, peGX1, peX1, peGX1, pbScratch, cbScratch ); // gx1 = (x1^2 + a)*x1
    SymCryptModAdd( pCurve->FMod, peGX1, pCurve->B, peGX1, pbScratch, cbScratch ); // gx1 = (x1^2 + a)*x1 + b

    //x2 = ( z * u^2 * x1 ) modulo p
    SymCryptModSquare( pCurve->FMod, peU, peX2, pbScratch, cbScratch ); // x2 = u^2
    SymCryptModMul( pCurve->FMod, peX2, peZ, peX2, pbScratch, cbScratch ); // x2 = u^2 * z
    SymCryptModMul( pCurve->FMod, peX2, peX1, peX2, pbScratch, cbScratch ); // x2 = u^2 * z * x1

    //gx2 = ( x2^3 + a * x2 + b ) = (x2^2 + a)*x2 + b modulo p
    SymCryptModSquare( pCurve->FMod, peX2, peGX2, pbScratch, cbScratch ); // gx2 = x2^2
    SymCryptModAdd( pCurve->FMod, peGX2, pCurve->A, peGX2, pbScratch, cbScratch ); // gx2 = x2^2 + a
    SymCryptModMul( pCurve->FMod, peGX2, peX2, peGX2, pbScratch, cbScratch ); // gx2 = (x2^2 + a)*x2
    SymCryptModAdd( pCurve->FMod, peGX2, pCurve->B, peGX2, pbScratch, cbScratch ); // gx2 = (x2^2 + a)*x2 + b

    //l = gx1 is a quadratic residue modulo p
    scError = SymCryptModSqrt( pCurve->FMod, peGX1, &selectionMask, NULL, pbScratch, cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // v = CSEL( l, gx1, gx2 )
    // (Using gx1 as a temporary for v)
    SymCryptModElementMaskedCopy( pCurve->FMod, peGX2, peGX1, ~selectionMask );

    // x = CSEL( l, x1, x2 )
    // (Using x1 as a temporary for x)
    SymCryptModElementMaskedCopy( pCurve->FMod, peX2, peX1, ~selectionMask );

    // y = sqrt( v ) = v^{(P+1)/4}
    // (Using gx1 as a temporary for y)
    scError = SymCryptModSqrt( pCurve->FMod, peGX1, &selectionMask, peGX1, pbScratch, cbScratch );

    // l = CEQ( LSB( u ), LSB( y ) )
    // LSB returns the least significant *BIT* of its argument
    SymCryptModElementToInt( pCurve->FMod, peU, piTmp, pbScratch, cbScratch );
    UINT32 u = SymCryptIntGetValueLsbits32( piTmp );

    SymCryptModElementToInt( pCurve->FMod, peGX1, piTmp, pbScratch, cbScratch );
    UINT32 y = SymCryptIntGetValueLsbits32( piTmp );

    selectionMask = SYMCRYPT_MASK32_EQ( u & 1, y & 1 );

    // P = CSEL( l, ( x, y ), ( x, p - y ) )
    // equivalently, y = CSEL( l, y, p - y )
    // (p - y) mod p is equivalent to -y mod p, so we end up with
    // y = CSEL(l, y, -y)
    // We use gx1 for y
    SymCryptModNeg( pCurve->FMod, peGX1, peTmp, pbScratch, cbScratch );
    SymCryptModElementMaskedCopy( pCurve->FMod, peTmp, peGX1, ~selectionMask );

    SymCryptModElementGetValue( pCurve->FMod, peX1, &pointBuf[0], pCurve->FModBytesize, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pbScratch, cbScratch );
    SymCryptModElementGetValue( pCurve->FMod, peGX1, &pointBuf[pCurve->FModBytesize], pCurve->FModBytesize, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pbScratch, cbScratch );

    scError = SymCryptEcpointSetValue( pCurve,
        pointBuf,
        2 * pCurve->FModBytesize,
        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
        SYMCRYPT_ECPOINT_FORMAT_XY,
        poP,
        0,
        pbScratch,
        cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:

    if( peGX2 != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peGX2 );
        peGX2 = NULL;
    }

    if( peGX1 != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peGX1 );
        peGX1 = NULL;
    }

    if( peX2 != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peX2 );
        peX2 = NULL;
    }

    if( peX1 != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peX1 );
        peX1 = NULL;
    }

    if( peT != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peT );
        peT = NULL;
    }

    if( peM != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peM );
        peM = NULL;
    }

    if( peZ != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peZ );
        peZ = NULL;
    }

    if( peTmp != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peTmp );
        peTmp = NULL;
    }

    if( piTmp != NULL )
    {
        SymCryptIntFree( piTmp );
        piTmp = NULL;
    }

    return scError;
}

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomSetRandMask(
    _Inout_                             PSYMCRYPT_802_11_SAE_CUSTOM_STATE   pState,
    _Inout_updates_opt_( cbRand )       PBYTE                               pbRand,
                                        SIZE_T                              cbRand,
    _Inout_updates_opt_( cbMask)        PBYTE                               pbMask,
                                        SIZE_T                              cbMask,
    _Out_writes_bytes_( cbScratch )     PBYTE                               pbScratch,
                                        SIZE_T                              cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PCSYMCRYPT_ECURVE pcCurve = pState->pCurve;

    SymCryptModElementSetValueUint32( 0, pcCurve->GOrd, pState->peRand, pbScratch, cbScratch );
    if( pbRand != NULL )
    {
        scError = SymCryptModElementSetValue( pbRand, cbRand, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pcCurve->GOrd, pState->peRand, pbScratch, cbScratch );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

    if( SymCryptModElementIsZero( pcCurve->GOrd, pState->peRand ) )
    {
        SymCryptModSetRandom( pcCurve->GOrd, pState->peRand, SYMCRYPT_FLAG_MODRANDOM_ALLOW_MINUSONE, pbScratch, cbScratch );
    }

    if( pbRand != NULL )
    {
        scError = SymCryptModElementGetValue( pcCurve->GOrd, pState->peRand, pbRand, cbRand, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pbScratch, cbScratch );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

    SymCryptModElementSetValueUint32( 0, pcCurve->GOrd, pState->peMask, pbScratch, cbScratch );
    if( pbMask != NULL )
    {
        scError = SymCryptModElementSetValue( pbMask, cbMask, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pcCurve->GOrd, pState->peMask, pbScratch, cbScratch );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

    if( SymCryptModElementIsZero( pcCurve->GOrd, pState->peMask ) )
    {
        SymCryptModSetRandom( pcCurve->GOrd, pState->peMask, SYMCRYPT_FLAG_MODRANDOM_ALLOW_MINUSONE, pbScratch, cbScratch );
    }

    if( pbMask != NULL )
    {
        scError = SymCryptModElementGetValue( pcCurve->GOrd, pState->peMask, pbMask, cbMask, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pbScratch, cbScratch );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

    //
    // The standard calls for checking that peRand and peMask are not 0 or 1, and peRand + peMask is not 0 or 1.
    // When the caller specifies the values we don't want to do any checking as they might be helpful in test vectors.
    // When this code generates the random values, we avoid 0 or 1 (by not passing the flags allowing 0 and 1).
    // We don't check that peRand + peMask > 1 because the probability of that occurring randomly is about 2^{-254} so the
    // risk of this happening on any machine ever in the world is much smaller than the risk associated with adding several lines of code.
    //

cleanup:

    return scError;
}

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomInit(
    _Out_                       PSYMCRYPT_802_11_SAE_CUSTOM_STATE   pState,
    _In_reads_( 6 )             PCBYTE                              pbMac1,
    _In_reads_( 6 )             PCBYTE                              pbMac2,
    _In_reads_( cbPassword )    PCBYTE                              pbPassword,
                                SIZE_T                              cbPassword,
    _Out_opt_                   PBYTE                               pbCounter,
    _Inout_updates_opt_( 32 )   PBYTE                               pbRand,
    _Inout_updates_opt_( 32 )   PBYTE                               pbMask )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    BYTE counter;
    UINT32 notFoundMask;
    UINT32 solutionMask;
    UINT32 negMask;
    BYTE abSeed[SYMCRYPT_HMAC_SHA256_RESULT_SIZE];
    BYTE abValue[SYMCRYPT_HMAC_SHA256_RESULT_SIZE];
    BYTE abSeedKey[16];     // Need only 12, but the extra bytes make the code easier.
    SYMCRYPT_HMAC_SHA256_EXPANDED_KEY hmacSeedKey;
    SYMCRYPT_HMAC_SHA256_EXPANDED_KEY hmacValueKey;
    SYMCRYPT_HMAC_SHA256_STATE hmacState;
    BYTE abTmp[2];
    BYTE pointBuf[ 64 ];
    PBYTE pbScratch = NULL;
    SIZE_T cbScratch = 0;
    UINT64 minMac;
    UINT64 maxMac;

    UINT32  nDigits;
    PSYMCRYPT_ECURVE        pCurve;             // Only a cache, pState->pCurve owns the allocation
    PSYMCRYPT_INT           piTmp = NULL;
    PSYMCRYPT_MODELEMENT    peX = NULL;
    PSYMCRYPT_MODELEMENT    peY = NULL;
    PSYMCRYPT_MODELEMENT    peCubic = NULL;
    PSYMCRYPT_MODELEMENT    peTmp = NULL;
    PSYMCRYPT_ECPOINT       poPWECandidate = NULL;

    // Set state to 0 so that our pointers have valid values.
    SymCryptWipe( pState, sizeof( *pState ) );

    // Per IEEE 802.11-2016 section 12.4.4.1 the mandatory-to-implement curve is
    // number 19 from the IANA Group description for RFC 2409 (IKE)
    // The IANA website maps this to a 256-bit Random ECP group in RFC 5903.
    // RFC 5903 specifies this group to be identical to the NIST P256 curve.
    pCurve = SymCryptEcurveAllocate( SymCryptEcurveParamsNistP256, 0 );
    pState->pCurve = pCurve;
    if( pCurve == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pState->macAlgorithm = SymCryptHmacSha256Algorithm;

    pState->peRand = SymCryptModElementAllocate( pCurve->GOrd );
    if( pState->peRand == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pState->peMask = SymCryptModElementAllocate( pCurve->GOrd );
    if( pState->peMask == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pState->poPWE = SymCryptEcpointAllocate( pCurve );
    if( pState->poPWE == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    nDigits = SymCryptDigitsFromBits( PRIME_LENGTH_BITS );

    cbScratch = SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( nDigits ),
                     SYMCRYPT_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( pCurve ) ) );
    pbScratch = SymCryptCallbackAlloc( cbScratch );

    piTmp = SymCryptIntAllocate( nDigits );
    peX = SymCryptModElementAllocate( pCurve->FMod );
    peY = SymCryptModElementAllocate( pCurve->FMod );
    peCubic = SymCryptModElementAllocate( pCurve->FMod );
    peTmp = SymCryptModElementAllocate( pCurve->FMod );
    poPWECandidate = SymCryptEcpointAllocate( pCurve );

    if( pbScratch == NULL || piTmp == NULL || peX == NULL || peY == NULL || peCubic == NULL || peTmp == NULL || poPWECandidate == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    SymCryptWipeKnownSize( abSeedKey, sizeof( abSeedKey ) );
    memcpy( &abSeedKey[0], pbMac1, 6 );
    minMac = SYMCRYPT_LOAD_MSBFIRST64( abSeedKey );
    memcpy( &abSeedKey[0], pbMac2, 6 );
    maxMac = SYMCRYPT_LOAD_MSBFIRST64( abSeedKey );

    if( minMac > maxMac )
    {
        // MAC values are public, no side-channel issues with this if()
        // Swap the two values
        minMac ^= maxMac;
        maxMac ^= minMac;
        minMac ^= maxMac;
    }

    // Now we write the two MACs into the buffer.
    // Note the slight overlap, and the use of 14 bytes rather than 12
    SYMCRYPT_STORE_MSBFIRST64( &abSeedKey[0], maxMac );
    SYMCRYPT_STORE_MSBFIRST64( &abSeedKey[6], minMac );         // This writes up to abSeedKey[14]

    SymCryptHmacSha256ExpandKey( &hmacSeedKey, abSeedKey, 12 );
    SymCryptWipeKnownSize( abSeedKey, sizeof( abSeedKey ) );    // Not strictly speaking a secret, but good general hygiene

    notFoundMask = (UINT32)-1;
    counter = 0;

    // We exit the loop only after 40 or more iterations
    // This greatly reduces the side-channel of how often we run this loop.
    while( notFoundMask != 0 || counter < 40 )
    {
        counter += 1;
        if( counter == 0 )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        // pwd-seed = Hmac-sha256( MacA || MacB , Password || counter )
        SymCryptHmacSha256Init( &hmacState, &hmacSeedKey );
        SymCryptHmacSha256Append( &hmacState, pbPassword, cbPassword );
        SymCryptHmacSha256Append( &hmacState, &counter, 1 );
        SymCryptHmacSha256Result( &hmacState, abSeed );

        // pwd-value
        SymCryptHmacSha256ExpandKey( &hmacValueKey, abSeed, sizeof( abSeed ) );
        SymCryptHmacSha256Init( &hmacState, &hmacValueKey );

        SYMCRYPT_STORE_LSBFIRST16( abTmp, 1 );
        SymCryptHmacSha256Append( &hmacState, abTmp, 2 );   // i value = 1
        // Spec is unclear on whether there should be a terminating 0 on the context
        // There are 23 characters in the string, so using len=24 gives us a zero
        SymCryptHmacSha256Append( &hmacState, (PCBYTE) "SAE Hunting and Pecking", 23 );

        // Pick up the byte representation of p from the parameters
        SymCryptHmacSha256Append( &hmacState, (BYTE *)(SymCryptEcurveParamsNistP256 + 1), 32 );

        SYMCRYPT_STORE_LSBFIRST16( abTmp, 256 );
        SymCryptHmacSha256Append( &hmacState, abTmp, 2 );   // Length value = 256
        SymCryptHmacSha256Result( &hmacState, abValue );

        // Get the pwd-value into an integer
        scError = SymCryptIntSetValue( abValue, sizeof( abValue ), SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piTmp );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        // Check that it is less than P
        if( !SymCryptIntIsLessThan( piTmp, SymCryptIntFromModulus( pCurve->FMod ) ) )
        {
            // This is a slight side-channel, but our prime P starts with FFFFFFFF so the probability of
            // hitting this case is < 2^-32.
            continue;
        }

        // Compute x^3 + A*x + B
        SymCryptIntToModElement( piTmp, pCurve->FMod, peX, pbScratch, cbScratch );
        SymCryptModSquare( pCurve->FMod, peX, peCubic, pbScratch, cbScratch );
        SymCryptModAdd( pCurve->FMod, peCubic, pCurve->A, peCubic, pbScratch, cbScratch );
        SymCryptModMul( pCurve->FMod, peCubic, peX, peCubic, pbScratch, cbScratch );
        SymCryptModAdd( pCurve->FMod, peCubic, pCurve->B, peCubic, pbScratch, cbScratch );

        // Get the quadratic residue of (x^3 + A*x + B) modulo P if it exists
        scError = SymCryptModSqrt( pCurve->FMod, peCubic, &solutionMask, peY, pbScratch, cbScratch );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        solutionMask &= notFoundMask;

        // Pick Y or -Y according to the LSbits
        SymCryptModElementToInt( pCurve->FMod, peY, piTmp, pbScratch, cbScratch );
        SymCryptModNeg( pCurve->FMod, peY, peTmp, pbScratch, cbScratch );

        negMask = 0 - ((abSeed[ sizeof( abSeed ) - 1 ] ^ SymCryptIntGetValueLsbits32( piTmp ) ) & 1);
        SymCryptModElementMaskedCopy( pCurve->FMod, peTmp, peY, negMask );

        SymCryptModElementGetValue( pCurve->FMod, peX, &pointBuf[ 0], 32, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pbScratch, cbScratch );
        SymCryptModElementGetValue( pCurve->FMod, peY, &pointBuf[32], 32, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pbScratch, cbScratch );
        scError = SymCryptEcpointSetValue(  pCurve,
                                            pointBuf,
                                            sizeof( pointBuf ),
                                            SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                                            SYMCRYPT_ECPOINT_FORMAT_XY,
                                            poPWECandidate,
                                            0,
                                            pbScratch,
                                            cbScratch );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        SymCryptEcpointMaskedCopy( pCurve, poPWECandidate, pState->poPWE, solutionMask );
        pState->counter |= (BYTE)(counter & solutionMask);

        notFoundMask &= ~solutionMask;
    }

    scError = SymCrypt802_11SaeCustomSetRandMask( pState, pbRand, 32, pbMask, 32, pbScratch, cbScratch );
    if( scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    if( pbCounter != NULL )
    {
        *pbCounter = pState->counter;
    }

cleanup:

    SymCryptWipe( &hmacSeedKey, sizeof( hmacSeedKey ) );
    SymCryptWipe( &hmacValueKey, sizeof( hmacValueKey ) );
    SymCryptWipe( abSeed, sizeof( abSeed ) );
    SymCryptWipe( abValue, sizeof( abValue ) );
    SymCryptWipe( pointBuf, sizeof( pointBuf ) );

    if( piTmp != NULL )
    {
        SymCryptIntFree( piTmp );
        piTmp = NULL;
    }

    if( peX != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peX );
        peX = NULL;
    }

    if( peY != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peY );
        peY = NULL;
    }

    if( peCubic != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peCubic );
        peCubic = NULL;
    }

    if( peTmp != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peTmp );
        peTmp = NULL;
    }

    if( poPWECandidate != NULL )
    {
        SymCryptEcpointFree( pCurve, poPWECandidate );
        poPWECandidate = NULL;
    }

    if( scError != SYMCRYPT_NO_ERROR )
    {
        SymCrypt802_11SaeCustomDestroy( pState );
    }

    if( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
        pbScratch = NULL;
    }

    return scError;
}


SYMCRYPT_ERROR
SymCrypt802_11SaeCustomCreatePTGeneric(
                                                            SYMCRYPT_802_11_SAE_GROUP           group,
    _In_reads_( cbSsid )                                    PCBYTE                              pbSsid,
                                                            SIZE_T                              cbSsid,
    _In_reads_( cbPassword )                                PCBYTE                              pbPassword,
                                                            SIZE_T                              cbPassword,
    _In_reads_opt_( cbPasswordIdentifier )                  PCBYTE                              pbPasswordIdentifier,
                                                            SIZE_T                              cbPasswordIdentifier,
    _Out_writes_( cbPT )                                    PBYTE                               pbPT,
                                                            SIZE_T                              cbPT)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SIZE_T cbIkm = 0;
    SIZE_T cbScratch = 0;

    PBYTE pbPwdValue = NULL;
    UINT32 cbPwdValue = 0;
    PBYTE pbScratch = NULL;
    SYMCRYPT_HKDF_EXPANDED_KEY hkdfKey;

    PSYMCRYPT_ECURVE          pCurve = NULL;
    PCSYMCRYPT_MAC            pMacAlgorithm = NULL;
    PSYMCRYPT_INT             piU1 = NULL;
    PSYMCRYPT_INT             piU2 = NULL;
    PSYMCRYPT_MODELEMENT      peU1 = NULL;
    PSYMCRYPT_MODELEMENT      peU2 = NULL;

    PSYMCRYPT_ECPOINT         poP1 = NULL;
    PSYMCRYPT_ECPOINT         poP2 = NULL;
    PSYMCRYPT_ECPOINT         poPT = NULL;

    PCSYMCRYPT_SAE_GROUP_DATA pGroupData = NULL;


    pGroupData = SymCryptSaeFindGroupData( group );

    // Provided IANA group number must match one of the supported groups
    if ( pGroupData == NULL)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Construct the objects associated with the IANA group number
    pCurve = SymCryptEcurveAllocate( *( pGroupData->pCurveParams), 0 );
    if( pCurve == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pMacAlgorithm = *( pGroupData->macAlgorithm );

    const UINT32 nDigits = SymCryptEcurveDigitsofFieldElement( pCurve );

    cbIkm = cbPassword + cbPasswordIdentifier;
    cbScratch =  SYMCRYPT_MAX( cbIkm,
                      SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ),
                          SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( nDigits ),
                              SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( pCurve ),
                                  SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ) ) ) ) );
    pbScratch = SymCryptCallbackAlloc( cbScratch );

    // len = olen( p ) + floor( olen( p ) / 2 )
    cbPwdValue = SYMCRYPT_BYTES_FROM_BITS(pCurve->FModBitsize) + SYMCRYPT_BYTES_FROM_BITS(pCurve->FModBitsize) / 2;

    pbPwdValue = SymCryptCallbackAlloc( cbPwdValue );

    piU1 = SymCryptIntAllocate( SymCryptDigitsFromBits( cbPwdValue * 8 ) );
    piU2 = SymCryptIntAllocate( SymCryptDigitsFromBits( cbPwdValue * 8 ) );
    peU1 = SymCryptModElementAllocate( pCurve->FMod );
    peU2 = SymCryptModElementAllocate( pCurve->FMod );

    poP1 = SymCryptEcpointAllocate( pCurve );
    poP2 = SymCryptEcpointAllocate( pCurve );
    poPT = SymCryptEcpointAllocate( pCurve );

    if( pbScratch == NULL || pbPwdValue == NULL || piU1 == NULL || piU2 == NULL ||
        peU1 == NULL || peU2 == NULL || poP1 == NULL || poP2 == NULL || poPT == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // pwd-seed = HKDF-Extract( ssid, password [|| identifier] )
    // Note that SymCryptHkdfExpandKey corresponds to HKDF-Extract
    memcpy( pbScratch, pbPassword, cbPassword );
    if( pbPasswordIdentifier )
    {
        memcpy( pbScratch + cbPassword, pbPasswordIdentifier, cbPasswordIdentifier );
    }

    scError = SymCryptHkdfExpandKey( &hkdfKey, pMacAlgorithm, pbScratch, cbIkm, pbSsid, cbSsid );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // pwd-value = HKDF-Expand( pwd-seed, "SAE Hash to Element u1 P1", len )
    // Note that SymCryptHkdf derive corresponds to HKDF-Expand
    // Salt does not include a null terminator, so the length is 25 chars
    scError = SymCryptHkdfDerive( &hkdfKey, (PCBYTE) "SAE Hash to Element u1 P1", 25, pbPwdValue, cbPwdValue );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // u1 = pwd-value modulo p
    scError = SymCryptIntSetValue( pbPwdValue, cbPwdValue, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piU1 );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    SymCryptIntToModElement( piU1, pCurve->FMod, peU1, pbScratch, cbScratch );

    // P1 = SSWU( u1 )
    SymCryptSswu( pCurve, pGroupData->z, peU1, poP1, pbScratch, cbScratch );

    // pwd-value = HKDF-Expand( pwd-seed, "SAE Hash to Element u2 P2", len )
    scError = SymCryptHkdfDerive( &hkdfKey, (PCBYTE) "SAE Hash to Element u2 P2", 25, pbPwdValue, cbPwdValue );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // u2 = pwd-value modulo p
    scError = SymCryptIntSetValue( pbPwdValue, cbPwdValue, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piU2 );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    SymCryptIntToModElement( piU2, pCurve->FMod, peU2, pbScratch, cbScratch );

    // P2 = SSWU( u2 )
    scError = SymCryptSswu( pCurve, pGroupData->z, peU2, poP2, pbScratch, cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // PT = P1 + P2
    SymCryptEcpointAdd( pCurve, poP1, poP2, poPT, 0, pbScratch, cbScratch );

    scError = SymCryptEcpointGetValue( pCurve,
                                       poPT,
                                       SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                                       SYMCRYPT_ECPOINT_FORMAT_XY,
                                       pbPT,
                                       cbPT,
                                       0,
                                       pbScratch,
                                       cbScratch );
    SYMCRYPT_ASSERT( scError == SYMCRYPT_NO_ERROR );

cleanup:

    if( poP2 != NULL )
    {
        SymCryptEcpointFree( pCurve, poP2 );
        poP2 = NULL;
    }

    if( poP1 != NULL )
    {
        SymCryptEcpointFree( pCurve, poP1 );
        poP1 = NULL;
    }

    if( poPT != NULL )
    {
        SymCryptEcpointFree( pCurve, poPT );
        poPT = NULL;
    }

    if( peU2 != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peU2 );
        peU2 = NULL;
    }

    if( peU1 != NULL )
    {
        SymCryptModElementFree( pCurve->FMod, peU1 );
        peU1 = NULL;
    }

    if( piU2 != NULL )
    {
        SymCryptIntFree( piU2 );
        piU2 = NULL;
    }

    if( piU1 != NULL )
    {
        SymCryptIntFree( piU1 );
        piU1 = NULL;
    }

    if( pbPwdValue != NULL )
    {
        SymCryptWipe( pbPwdValue, cbPwdValue );
        SymCryptCallbackFree( pbPwdValue );
        pbPwdValue = NULL;
    }

    if( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
        pbScratch = NULL;
    }

    if ( pCurve != NULL )
    {
        SymCryptEcurveFree( pCurve );
        pCurve = NULL;
    }

    return scError;
}


SYMCRYPT_ERROR
SymCrypt802_11SaeCustomCreatePT(
    _In_reads_( cbSsid )                                    PCBYTE                              pbSsid,
                                                            SIZE_T                              cbSsid,
    _In_reads_( cbPassword )                                PCBYTE                              pbPassword,
                                                            SIZE_T                              cbPassword,
    _In_reads_opt_( cbPasswordIdentifier )                  PCBYTE                              pbPasswordIdentifier,
                                                            SIZE_T                              cbPasswordIdentifier,
    _Out_writes_( 64 )                                      PBYTE                               pbPT )
{
    return SymCrypt802_11SaeCustomCreatePTGeneric(  SYMCRYPT_SAE_GROUP_19,
                                                    pbSsid,
                                                    cbSsid,
                                                    pbPassword,
                                                    cbPassword,
                                                    pbPasswordIdentifier,
                                                    cbPasswordIdentifier,
                                                    pbPT,
                                                    64 );
}


SYMCRYPT_ERROR
SymCrypt802_11SaeCustomInitH2EGeneric(
    _Out_                                                       PSYMCRYPT_802_11_SAE_CUSTOM_STATE   pState,
                                                                SYMCRYPT_802_11_SAE_GROUP           group,
    _In_reads_( cbPT )                                          PCBYTE                              pbPT,
                                                                SIZE_T                              cbPT,
    _In_reads_( 6 )                                             PCBYTE                              pbMacA,
    _In_reads_( 6 )                                             PCBYTE                              pbMacB,
    _Inout_updates_opt_( cbRand )                               PBYTE                               pbRand,
                                                                SIZE_T                              cbRand,
    _Inout_updates_opt_( cbMask )                               PBYTE                               pbMask,
                                                                SIZE_T                              cbMask)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    BYTE hmacKeyBytes[SYMCRYPT_SAE_MAX_HMAC_OUTPUT_SIZE_BYTES] = { 0 };
    BYTE valBytes[SYMCRYPT_SAE_MAX_HMAC_OUTPUT_SIZE_BYTES] = { 0 };
    BYTE macBuffer[16] = { 0 }; // Need only 12, but the extra bytes make the code easier.
    SYMCRYPT_MAC_EXPANDED_KEY hmacKey = { 0 };
    SYMCRYPT_MAC_STATE hmacState = { 0 };

    SIZE_T cbScratch = 0;
    PBYTE pbScratch = NULL;

    UINT64 minMac = 0;
    UINT64 maxMac = 0;

    UINT32 nDigits = 0;

    PSYMCRYPT_INT piTmp = NULL;
    PSYMCRYPT_MODULUS pmMod = NULL;
    PSYMCRYPT_MODELEMENT peVal = NULL;
    PSYMCRYPT_MODELEMENT peTmp = NULL;
    PSYMCRYPT_ECPOINT poPT = NULL;
    PCSYMCRYPT_SAE_GROUP_DATA pGroupData = NULL;
    PCSYMCRYPT_MAC pMacAlgorithm = NULL;

    // Set state to 0 so that our pointers have valid values.
    SymCryptWipeKnownSize( pState, sizeof( *pState ) );

    PSYMCRYPT_ECURVE pCurve = NULL; // Weak reference; curve is owned by pState

    pGroupData = SymCryptSaeFindGroupData( group );

    // Provided IANA group number must match one of the supported groups
    if ( pGroupData == NULL )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Construct the objects associated with the IANA group number
    pCurve = SymCryptEcurveAllocate( *( pGroupData->pCurveParams ), 0 );
    if ( pCurve == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pState->pCurve = pCurve;

    pMacAlgorithm = *( pGroupData->macAlgorithm );

    SIZE_T cbHMACOutputSize = pMacAlgorithm->resultSize;

    pState->peRand = SymCryptModElementAllocate( pCurve->GOrd );
    if( pState->peRand == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pState->peMask = SymCryptModElementAllocate( pCurve->GOrd );
    if( pState->peMask == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pState->poPWE = SymCryptEcpointAllocate( pCurve );
    if( pState->poPWE == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    nDigits = SymCryptDigitsFromBits( pCurve->GOrdBitsize );

    piTmp = SymCryptIntAllocate( nDigits );
    pmMod = SymCryptModulusAllocate( nDigits );
    poPT = SymCryptEcpointAllocate( pCurve );

    cbScratch = SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ),
        SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( pCurve ),
            SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ),
                 SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS ( pCurve, 1 ) ) ) );
    pbScratch = SymCryptCallbackAlloc( cbScratch );

    if( piTmp == NULL || pmMod == NULL || poPT == NULL || pbScratch == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    memcpy( &macBuffer[0], pbMacA, 6 );
    minMac = SYMCRYPT_LOAD_MSBFIRST64( macBuffer );
    memcpy( &macBuffer[0], pbMacB, 6 );
    maxMac = SYMCRYPT_LOAD_MSBFIRST64( macBuffer );

    if( minMac > maxMac )
    {
        // MAC values are public, no side-channel issues with this if()
        // Swap the two values
        minMac ^= maxMac;
        maxMac ^= minMac;
        minMac ^= maxMac;
    }

    // Now we write the two MACs into the buffer.
    // Note the slight overlap, and the use of 14 bytes rather than 12
    SYMCRYPT_STORE_MSBFIRST64( &macBuffer[0], maxMac );
    SYMCRYPT_STORE_MSBFIRST64( &macBuffer[6], minMac ); // This writes up to macBuffer[14]

    // val = hmac-sha256( 0^n, maxMac || minMac )
    // The HMAC key is is a buffer of all zeros whose length equals the length of the digest from the hash function
    pMacAlgorithm->expandKeyFunc(&hmacKey, hmacKeyBytes, cbHMACOutputSize);

    pMacAlgorithm->initFunc( &hmacState, &hmacKey );
    pMacAlgorithm->appendFunc( &hmacState, macBuffer, 12 );
    pMacAlgorithm->resultFunc( &hmacState, valBytes );

    // val = val (#4666)modulo (q - 1) + 1
    SymCryptIntSubUint32( SymCryptIntFromModulus(pCurve->GOrd), 1, piTmp );
    SymCryptIntToModulus( piTmp, pmMod, 1, SYMCRYPT_FLAG_DATA_PUBLIC, pbScratch, cbScratch );

    peVal = SymCryptModElementAllocate( pmMod );
    peTmp = SymCryptModElementAllocate( pmMod );

    if( peVal == NULL || peTmp == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    scError = SymCryptModElementSetValue( valBytes, cbHMACOutputSize, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pmMod, peVal, pbScratch, cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    SymCryptModElementSetValueUint32( 1, pmMod, peTmp, pbScratch, cbScratch );
    SymCryptModAdd( pmMod, peVal, peTmp, peVal, pbScratch, cbScratch );

    SymCryptModElementToInt( pmMod, peVal, piTmp, pbScratch, cbScratch );

    scError = SymCryptEcpointSetValue( pCurve,
                                       pbPT,
                                       cbPT,
                                       SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                                       SYMCRYPT_ECPOINT_FORMAT_XY,
                                       poPT,
                                       0,
                                       pbScratch,
                                       cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptEcpointScalarMul( pCurve, piTmp, poPT, 0, pState->poPWE, pbScratch, cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCrypt802_11SaeCustomSetRandMask( pState, pbRand, cbRand, pbMask, cbMask, pbScratch, cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:

    if( peTmp != NULL )
    {
        SymCryptModElementFree( pmMod, peTmp );
        peTmp = NULL;
    }

    if( peVal != NULL )
    {
        SymCryptModElementFree( pmMod, peVal );
        peVal = NULL;
    }

    if( poPT != NULL )
    {
        SymCryptEcpointFree( pCurve, poPT );
        poPT = NULL;
    }

    if( pmMod != NULL )
    {
        SymCryptModulusFree( pmMod );
        pmMod = NULL;
    }

    if( piTmp != NULL )
    {
        SymCryptIntFree( piTmp );
        piTmp = NULL;
    }

    if( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
        pbScratch = NULL;
    }

    if( scError != SYMCRYPT_NO_ERROR )
    {
        SymCrypt802_11SaeCustomDestroy( pState );
    }

    return scError;
}

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomInitH2E(
    _Out_                           PSYMCRYPT_802_11_SAE_CUSTOM_STATE   pState,
    _In_reads_( 64 )                PCBYTE                              pbPT,
    _In_reads_( 6 )                 PCBYTE                              pbMacA,
    _In_reads_( 6 )                 PCBYTE                              pbMacB,
    _Inout_updates_opt_( 32 )       PBYTE                               pbRand,
    _Inout_updates_opt_( 32 )       PBYTE                               pbMask )
{
    return SymCrypt802_11SaeCustomInitH2EGeneric(   pState,
                                                    SYMCRYPT_SAE_GROUP_19,
                                                    pbPT,
                                                    64,
                                                    pbMacA,
                                                    pbMacB,
                                                    pbRand,
                                                    32,
                                                    pbMask,
                                                    32 );
}


VOID
SymCrypt802_11SaeCustomDestroy(
    _Inout_   PSYMCRYPT_802_11_SAE_CUSTOM_STATE   pState )
{
    PSYMCRYPT_ECURVE pCurve = pState->pCurve;

    if( pState->poPWE != NULL )
    {
        SymCryptEcpointFree( pCurve, pState->poPWE );
    }

    if( pState->peMask != NULL )
    {
        SymCryptModElementFree( pCurve->GOrd, pState->peMask );
    }

    if( pState->peRand != NULL )
    {
        SymCryptModElementFree( pCurve->GOrd, pState->peRand );
    }

    if( pCurve != NULL )
    {
        SymCryptEcurveFree( pCurve );
    }

    SymCryptWipeKnownSize( pState, sizeof( *pState ) );
}

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomCommitCreateGeneric(
    _In_                                PCSYMCRYPT_802_11_SAE_CUSTOM_STATE  pState,
    _Out_writes_( cbCommitScalar )      PBYTE                               pbCommitScalar,
                                        SIZE_T                              cbCommitScalar,
    _Out_writes_( cbCommitElement )     PBYTE                               pbCommitElement,
                                        SIZE_T                              cbCommitElement)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PSYMCRYPT_MODELEMENT peTmp = NULL;
    PSYMCRYPT_INT piTmp = NULL;
    PSYMCRYPT_ECPOINT poPoint = NULL;
    PBYTE pbScratch = NULL;
    SIZE_T cbScratch;
    SIZE_T nDigits;

    PCSYMCRYPT_ECURVE pCurve = pState->pCurve;

    nDigits = SymCryptDigitsFromBits( pCurve->FModBitsize );
    cbScratch = SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS( pCurve ),
                     SYMCRYPT_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( pCurve ) ) );

    pbScratch = SymCryptCallbackAlloc( cbScratch );

    peTmp = SymCryptModElementAllocate( pCurve->GOrd );
    piTmp = SymCryptIntAllocate( SymCryptEcurveDigitsofScalarMultiplier( pCurve ) );
    poPoint = SymCryptEcpointAllocate( pCurve );

    if( peTmp == NULL || piTmp == NULL || poPoint == NULL || pbScratch == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    SymCryptModAdd( pCurve->GOrd, pState->peRand, pState->peMask, peTmp, pbScratch, cbScratch );
    scError = SymCryptModElementGetValue( pCurve->GOrd, peTmp, pbCommitScalar, cbCommitScalar, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pbScratch, cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    SymCryptModElementToInt( pCurve->GOrd, pState->peMask, piTmp, pbScratch, cbScratch );
    scError = SymCryptEcpointScalarMul( pCurve,
                                        piTmp,
                                        pState->poPWE,
                                        0,
                                        poPoint,
                                        pbScratch,
                                        cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Now we have mask * PWE, but we need the negative...
    SymCryptEcpointNegate( pCurve, poPoint, (UINT32)-1, pbScratch, cbScratch );

    scError = SymCryptEcpointGetValue(  pCurve,
                                        poPoint,
                                        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                                        SYMCRYPT_ECPOINT_FORMAT_XY,
                                        pbCommitElement,
                                        cbCommitElement,
                                        0,
                                        pbScratch,
                                        cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:

    if( piTmp != NULL )
    {
        SymCryptIntFree( piTmp );
        piTmp = NULL;
    }

    if( peTmp != NULL )
    {
        SymCryptModElementFree( pCurve->GOrd, peTmp );
        peTmp = NULL;
    }

    if( poPoint != NULL )
    {
        SymCryptEcpointFree( pCurve, poPoint );
        poPoint = NULL;
    }

    if( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
        pbScratch = NULL;
    }

    return scError;
}

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomCommitCreate(
    _In_                  PCSYMCRYPT_802_11_SAE_CUSTOM_STATE  pState,
    _Out_writes_( 32 )    PBYTE                               pbCommitScalar,
    _Out_writes_( 64 )    PBYTE                               pbCommitElement )
{
    return SymCrypt802_11SaeCustomCommitCreateGeneric(  pState,
                                                        pbCommitScalar,
                                                        32,
                                                        pbCommitElement,
                                                        64 );
}

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomCommitProcessGeneric(
    _In_                                    PCSYMCRYPT_802_11_SAE_CUSTOM_STATE  pState,
    _In_reads_( cbPeerCommitScalar )        PCBYTE                              pbPeerCommitScalar,
                                            SIZE_T                              cbPeerCommitScalar,
    _In_reads_( cbPeerCommitElement )       PCBYTE                              pbPeerCommitElement,
                                            SIZE_T                              cbPeerCommitElement,
    _Out_writes_( cbSharedSecret )          PBYTE                               pbSharedSecret,
                                            SIZE_T                              cbSharedSecret,
    _Out_writes_( cbScalarSum )             PBYTE                               pbScalarSum,
                                            SIZE_T                              cbScalarSum )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PSYMCRYPT_ECURVE        pCurve = pState->pCurve;
    PSYMCRYPT_MODELEMENT    peCommitScalarSum = NULL;
    PSYMCRYPT_ECPOINT       poPeerCommitElement = NULL;
    PSYMCRYPT_ECPOINT       poTmp = NULL;
    PSYMCRYPT_INT           piTmp = NULL;
    UINT32                  nDigits;

    PBYTE pbScratch = NULL;
    SIZE_T cbScratch;

    nDigits = SymCryptDigitsFromBits( pCurve->FModBitsize );
    cbScratch = SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS( pCurve ),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ),
                     SYMCRYPT_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( pCurve ) ) ) );
    pbScratch = SymCryptCallbackAlloc( cbScratch );

    peCommitScalarSum = SymCryptModElementAllocate( pCurve->GOrd );
    poPeerCommitElement = SymCryptEcpointAllocate( pCurve );
    poTmp = SymCryptEcpointAllocate( pCurve );
    piTmp = SymCryptIntAllocate( SymCryptEcurveDigitsofScalarMultiplier( pCurve ) );

    if( pbScratch == NULL || peCommitScalarSum == NULL || poPeerCommitElement == NULL || poTmp == NULL || piTmp == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // piTmp = peer commit value
    scError = SymCryptIntSetValue( pbPeerCommitScalar, cbPeerCommitScalar, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piTmp );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // The Standard requires a check that the Peer commit value must be 1 < peer-commit < r where r is the group order.
    if( !SymCryptIntIsLessThan( piTmp, SymCryptIntFromModulus( pCurve->GOrd ) ) ||
        SymCryptIntIsEqualUint32( piTmp, 0 ) ||
        SymCryptIntIsEqualUint32( piTmp, 1 ) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    SymCryptIntToModElement( piTmp, pCurve->GOrd, peCommitScalarSum, pbScratch, cbScratch );

    // Now compute the sum of the scalar commit values
    SymCryptModAdd( pCurve->GOrd, peCommitScalarSum, pState->peRand, peCommitScalarSum, pbScratch, cbScratch );
    SymCryptModAdd( pCurve->GOrd, peCommitScalarSum, pState->peMask, peCommitScalarSum, pbScratch, cbScratch );

    scError = SymCryptEcpointSetValue(  pCurve,
                                        pbPeerCommitElement,
                                        cbPeerCommitElement,
                                        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                                        SYMCRYPT_ECPOINT_FORMAT_XY,
                                        poPeerCommitElement,
                                        0,
                                        pbScratch,
                                        cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // The EcPointSetValue routine returns an error if either coordinate is >= P.
    // We need to check that the point is on the curve and not the zero point of the curve
    // (The zero point is sometimes called the 'point at infinity'.)
    if( !SymCryptEcpointOnCurve( pCurve, poPeerCommitElement, pbScratch, cbScratch ) ||
        SymCryptEcpointIsZero( pCurve, poPeerCommitElement, pbScratch, cbScratch ) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }


    scError = SymCryptEcpointScalarMul( pCurve,
                                        piTmp,
                                        pState->poPWE,
                                        0,
                                        poTmp,
                                        pbScratch,
                                        cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    SymCryptEcpointAdd( pCurve, poTmp, poPeerCommitElement, poTmp, 0, pbScratch, cbScratch );

    SymCryptModElementToInt( pCurve->GOrd, pState->peRand, piTmp, pbScratch, cbScratch );
    scError = SymCryptEcpointScalarMul( pCurve,
                                        piTmp,
                                        poTmp,
                                        0,
                                        poTmp,
                                        pbScratch,
                                        cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptEcpointGetValue(  pCurve,
                                        poTmp,
                                        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                                        SYMCRYPT_ECPOINT_FORMAT_X,
                                        pbSharedSecret,
                                        cbSharedSecret,
                                        0,
                                        pbScratch,
                                        cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptModElementGetValue( pCurve->GOrd, peCommitScalarSum, pbScalarSum, cbScalarSum, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pbScratch, cbScratch );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:

    if( peCommitScalarSum != NULL )
    {
        SymCryptModElementFree( pCurve->GOrd, peCommitScalarSum );
        peCommitScalarSum = NULL;
    }

    if( poPeerCommitElement != NULL )
    {
        SymCryptEcpointFree( pCurve, poPeerCommitElement );
        poPeerCommitElement = NULL;
    }

    if( poTmp != NULL )
    {
        SymCryptEcpointFree( pCurve, poTmp );
        poTmp = NULL;
    }

    if( piTmp != NULL )
    {
        SymCryptIntFree( piTmp );
        piTmp = NULL;
    }

    if( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
        pbScratch = NULL;
    }

    return scError;
}

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomCommitProcess(
    _In_                    PCSYMCRYPT_802_11_SAE_CUSTOM_STATE  pState,
    _In_reads_( 32 )        PCBYTE                              pbPeerCommitScalar,
    _In_reads_( 64 )        PCBYTE                              pbPeerCommitElement,
    _Out_writes_( 32 )      PBYTE                               pbSharedSecret,
    _Out_writes_( 32 )      PBYTE                               pbScalarSum )
{
    return SymCrypt802_11SaeCustomCommitProcessGeneric( pState,
                                                        pbPeerCommitScalar,
                                                        32,
                                                        pbPeerCommitElement,
                                                        64,
                                                        pbSharedSecret,
                                                        32,
                                                        pbScalarSum,
                                                        32 );
}
