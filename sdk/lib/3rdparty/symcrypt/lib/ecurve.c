//
// ecurve.c   Ecurve functions
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//

#include "precomp.h"

// Approximate number of consecutive operations with the modulus and the
// (sub)group order of the curve. These numbers can trigger special optimizations
// on the underlying code, e.g. use of Montgomery multiplication or not.
#define SYMCRYPT_INTERNAL_ECURVE_MODULUS_NUMOF_OPERATIONS( _bitsize )      ( 100 * (_bitsize) )
#define SYMCRYPT_INTERNAL_ECURVE_GROUP_ORDER_NUMOF_OPERATIONS              ( 1 )

// We limit the max size of the elliptic curve to avoid denial-of-service attacks when
// an attacker sends a curve specification.
// Elliptic curve operations are O(n^3) in the curve size. Theoretically SymCrypt supports
// values up to 2^20 bits at the moment, so that is 2^12 times more than a typical curve size
// of 256 bits. Operations are then 2^36 times slower, and a single operation could take months.
// Our largest curve is 521 bits, and we won't see curves > 1024 bits for a while yet.
#define SYMCRYPT_INTERNAL_MAX_ECURVE_SIZE   (1024)

// Private struct which records the sizes of various different parts of the elliptic curve
// structure.
typedef struct _SYMCRYPT_ECURVE_SIZES {
    UINT32                          nDigitsFieldLength;
    UINT32                          nDigitsSubgroupOrder;
    UINT32                          nDigitsCoFactor;
    UINT32                          cbAlloc; // Length of the whole curve buffer
    UINT32                          cbModulus;
    UINT32                          cbModElement;
    UINT32                          cbEcpoint;
    UINT32                          cbSubgroupOrder;
    UINT32                          cbCoFactor;
    UINT32                          cbScratch;
    SYMCRYPT_ECPOINT_COORDINATES    eCoordinates;
} SYMCRYPT_ECURVE_SIZES, *PSYMCRYPT_ECURVE_SIZES;
typedef const SYMCRYPT_ECURVE_SIZES * PCSYMCRYPT_ECURVE_SIZES;

// Helper function which validates curve parameters and computes various buffer sizes.
static
BOOLEAN
SymCryptEcurveValidateAndComputeSizes(
    _In_    PCSYMCRYPT_ECURVE_PARAMS    pParams,
    _Out_   PSYMCRYPT_ECURVE_SIZES      pSizes )
{
    BOOLEAN fSuccess = FALSE;

    // Check that the parameters are well formatted
    SYMCRYPT_ASSERT( pParams != NULL );
    SYMCRYPT_ASSERT( (pParams->version == 1) || (pParams->version == 2) );
    SYMCRYPT_ASSERT( pParams->cbFieldLength != 0 );
    SYMCRYPT_ASSERT( pParams->cbSubgroupOrder != 0 );
    SYMCRYPT_ASSERT( pParams->cbCofactor != 0 );
    SYMCRYPT_ASSERT( (pParams->type == SYMCRYPT_ECURVE_TYPE_SHORT_WEIERSTRASS) ||
                     (pParams->type == SYMCRYPT_ECURVE_TYPE_TWISTED_EDWARDS) ||
                     (pParams->type == SYMCRYPT_ECURVE_TYPE_MONTGOMERY) );

    // Reject inputs that are wildly big to avoid denial-of-service attacks.
    if ( pParams->cbFieldLength > SYMCRYPT_INTERNAL_MAX_ECURVE_SIZE/8 ||
         pParams->cbSubgroupOrder > SYMCRYPT_INTERNAL_MAX_ECURVE_SIZE / 8 + 1 ||     // subgroup can be > field prime
         pParams->cbCofactor > 2 ||                                                  // We support co-factor = 256
         pParams->cbSeed > 256 )
    {
        goto cleanup;
    }

    // Getting the # of digits of the various parameters
    pSizes->nDigitsFieldLength = SymCryptDigitsFromBits( pParams->cbFieldLength * 8 );
    pSizes->nDigitsSubgroupOrder = SymCryptDigitsFromBits( pParams->cbSubgroupOrder * 8 );
    pSizes->nDigitsCoFactor = SymCryptDigitsFromBits( pParams->cbCofactor * 8 );

    // -----------------------------------------------
    // Getting the byte sizes of different objects
    // -----------------------------------------------
    pSizes->cbModulus = SymCryptSizeofModulusFromDigits( pSizes->nDigitsFieldLength );
    pSizes->cbSubgroupOrder = SymCryptSizeofModulusFromDigits( pSizes->nDigitsSubgroupOrder );
    pSizes->cbCoFactor =  SymCryptSizeofIntFromDigits( pSizes->nDigitsCoFactor );

    pSizes->cbModElement = SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( pParams->cbFieldLength * 8 );

    // EcPoint: The curve is not initialized yet, we call the helper function.
    // It depends on the default format of each curve type
    switch (pParams->type)
    {
    case (SYMCRYPT_ECURVE_TYPE_SHORT_WEIERSTRASS):
        pSizes->eCoordinates = SYMCRYPT_ECPOINT_COORDINATES_JACOBIAN;
        break;
    case (SYMCRYPT_ECURVE_TYPE_TWISTED_EDWARDS):
        pSizes->eCoordinates = SYMCRYPT_ECPOINT_COORDINATES_EXTENDED_PROJECTIVE;
        break;
    case (SYMCRYPT_ECURVE_TYPE_MONTGOMERY):
        pSizes->eCoordinates = SYMCRYPT_ECPOINT_COORDINATES_SINGLE_PROJECTIVE;
        break;
    default:
        goto cleanup;
    }

    pSizes->cbEcpoint = SymCryptSizeofEcpointEx( pSizes->cbModElement, SYMCRYPT_INTERNAL_NUMOF_COORDINATES( pSizes->eCoordinates ) );
    // -----------------------------------------------

    // Compute memory needed for the curve
    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    // Thus the following calculation does not overflow cbAlloc.
    //
    pSizes->cbAlloc =   sizeof( SYMCRYPT_ECURVE ) +
                        pSizes->cbModulus +
                        2 * pSizes->cbModElement +
                        pSizes->cbSubgroupOrder +
                        pSizes->cbCoFactor;

    if ( (pParams->type == SYMCRYPT_ECURVE_TYPE_SHORT_WEIERSTRASS) ||
         (pParams->type == SYMCRYPT_ECURVE_TYPE_TWISTED_EDWARDS) )
    {
        // If the curve's type is short Weierstrass allocate space for 2^(w-2) ECPOINTs
        // at the end of the curve's structure, where w is the width of the window.
        //
        // Note: The window width is fixed now. In later versions we can pass it in as a parameter.
        // SYMCRYPT_ASSERT( (1 << (SYMCRYPT_ECURVE_SW_DEF_WINDOW-2)) <= SYMCRYPT_ECURVE_SW_MAX_NPRECOMP_POINTS );
        pSizes->cbAlloc += (1 << (SYMCRYPT_ECURVE_SW_DEF_WINDOW-2))*pSizes->cbEcpoint;
    }
    else
    {
        // Otherwise just allocate space for just the distinguished point
        pSizes->cbAlloc += pSizes->cbEcpoint;
    }

    // Compute memory needed for internal scratch space
    // EcpointSetValue and SymCryptOfflinePrecomputation

    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    //      - SymCryptSizeofEcpointEx is bounded by 2^20
    // Thus the following calculation does not overflow cbScratch.
    //
    pSizes->cbScratch = SymCryptSizeofEcpointEx( pSizes->cbModElement, SYMCRYPT_ECPOINT_FORMAT_MAX_LENGTH ) +
                        8 * pSizes->cbModElement +
                        SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pSizes->nDigitsFieldLength ),
                                      SYMCRYPT_SCRATCH_BYTES_FOR_MODINV( pSizes->nDigitsFieldLength ) );
    // IntToModulus( FMod and GOrd )
    pSizes->cbScratch = SYMCRYPT_MAX( pSizes->cbScratch,
                            SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_MODULUS( SYMCRYPT_MAX(pSizes->nDigitsFieldLength, pSizes->nDigitsSubgroupOrder) ) );
    // ModElementSetValue( FMod )
    pSizes->cbScratch = SYMCRYPT_MAX( pSizes->cbScratch,
                            SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pSizes->nDigitsFieldLength ) );

    fSuccess = TRUE;

cleanup:
    return fSuccess;
}

BOOLEAN
SYMCRYPT_CALL
SymCryptEcurveBufferSizesFromParams(
    _In_    PCSYMCRYPT_ECURVE_PARAMS        pParams,
    _Out_   SIZE_T *                        pcbCurve,
    _Out_   SIZE_T *                        pcbScratch )
{
    BOOLEAN fSuccess = FALSE;
    SYMCRYPT_ECURVE_SIZES sizes;

    if ( !SymCryptEcurveValidateAndComputeSizes( pParams, &sizes ))
    {
        goto cleanup;
    }

    *pcbCurve = sizes.cbAlloc;
    *pcbScratch = sizes.cbScratch;

    fSuccess = TRUE;

cleanup:
    return fSuccess;
}

// Internal function which actually computes and writes curve into the given buffer.
//
// This is called internally by both SymCryptEcurveCreate() and SymCryptEcurveAllocate().
static
PSYMCRYPT_ECURVE
SymCryptEcurveInitialize(
    _In_                                    PCSYMCRYPT_ECURVE_PARAMS    pParams,
    _In_                                    UINT32                      flags,
    _In_                                    PCSYMCRYPT_ECURVE_SIZES     pSizes,
    _Out_writes_bytes_( pSizes->cbAlloc )   PBYTE                       pbCurve,
    _Out_writes_bytes_( pSizes->cbScratch)  PBYTE                       pbScratch )
{
    BOOLEAN         fSuccess = FALSE;
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    PSYMCRYPT_ECURVE pCurve = (PSYMCRYPT_ECURVE)pbCurve;
    PBYTE            pDst = NULL;   // Destination pointer
    PBYTE            pSrc = NULL;   // Source pointer

    PBYTE            pSrcGenerator = NULL;  // We have to set the generator point
                                            // only after we have fully initialized the curve

    PSYMCRYPT_INT   pTempInt = 0;

    PSYMCRYPT_MODELEMENT  peTemp = NULL;

    PCSYMCRYPT_ECURVE_PARAMS_V2_EXTENSION   pcParamsV2Ext = NULL;

    UNREFERENCED_PARAMETER( flags );

    // -----------------------------------------------
    // Populating the fields of the curve object
    // -----------------------------------------------

    // Version of curve structure
    pCurve->version = SYMCRYPT_INTERNAL_ECURVE_VERSION_LATEST;

    // Type of curve
    pCurve->type = (int) pParams->type;

    // Curve point format
    pCurve->eCoordinates = pSizes->eCoordinates;

    // Number of digits of the field modulus
    pCurve->FModDigits = pSizes->nDigitsFieldLength;

    // Number of digits of the group order
    pCurve->GOrdDigits = pSizes->nDigitsSubgroupOrder;

    // Byte size of field elements
    pCurve->FModBytesize = (UINT32)pParams->cbFieldLength;

    // Byte size of group elements
    SYMCRYPT_ASSERT( pParams->cbSubgroupOrder < UINT32_MAX );
    pCurve->GOrdBytesize = (UINT32)pParams->cbSubgroupOrder;

    // Byte size of mod elements
    pCurve->cbModElement = pSizes->cbModElement;

    // Total bytesize of the curve (used to free the curve object)
    pCurve->cbAlloc = pSizes->cbAlloc;

    // Set destination and source pointers
    pDst = ((PBYTE) pCurve) + sizeof( SYMCRYPT_ECURVE );
    pSrc = ((PBYTE) pParams) + sizeof( SYMCRYPT_ECURVE_PARAMS );

    // Field Modulus
    pCurve->FMod = SymCryptModulusCreate( pDst, pSizes->cbModulus, pSizes->nDigitsFieldLength );
    if ( pCurve->FMod == NULL )
    {
        goto cleanup;
    }

    pTempInt = SymCryptIntFromModulus( pCurve->FMod );
    if ( pTempInt == NULL)
    {
        goto cleanup;
    }

    scError = SymCryptIntSetValue( pSrc, pParams->cbFieldLength, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pTempInt );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Field Modulus Bitsize
    pCurve->FModBitsize = SymCryptIntBitsizeOfValue( pTempInt );
    if (pCurve->FModBitsize < SYMCRYPT_ECURVE_MIN_BITSIZE_FMOD)
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    if( (SymCryptIntGetValueLsbits32( pTempInt ) & 1) == 0 )
    {
        // 'Prime' must be odd to avoid errors in conversion to modulus
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // IntToModulus requirement:
    //      FModBitsize >= SYMCRYPT_ECURVE_MIN_BITSIZE_FMOD --> pTempInt > 0
    SymCryptIntToModulus(
                    pTempInt,
                    pCurve->FMod,
                    SYMCRYPT_INTERNAL_ECURVE_MODULUS_NUMOF_OPERATIONS( 8 * pParams->cbFieldLength ),
                    SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
                    pbScratch,
                    pSizes->cbScratch );

    pDst += pSizes->cbModulus;
    pSrc += pParams->cbFieldLength;

    // A constant
    pCurve->A = SymCryptModElementCreate( pDst, pSizes->cbModElement, pCurve->FMod );
    if ( pCurve->A == NULL )
    {
        goto cleanup;
    }
    scError = SymCryptModElementSetValue(
                    pSrc,
                    pParams->cbFieldLength,
                    SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                    pCurve->FMod,
                    pCurve->A,
                    pbScratch,
                    pSizes->cbScratch );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }
    pDst += pSizes->cbModElement;
    pSrc += pParams->cbFieldLength;

    // B constant
    pCurve->B = SymCryptModElementCreate( pDst, pSizes->cbModElement, pCurve->FMod );
    if ( pCurve->B == NULL )
    {
        goto cleanup;
    }

    // Detect Short-Weierstrass curves with A == -3 (NIST prime curves are all of this form)
    // Use B's ModElement space for check
    if( pParams->type == SYMCRYPT_ECURVE_TYPE_SHORT_WEIERSTRASS )
    {
        SymCryptModElementSetValueNegUint32(
            3,
            pCurve->FMod,
            pCurve->B,
            pbScratch,
            pSizes->cbScratch );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
        if( SymCryptModElementIsEqual( pCurve->FMod, pCurve->A, pCurve->B ) )
        {
            pCurve->type = SYMCRYPT_INTERNAL_ECURVE_TYPE_SHORT_WEIERSTRASS_AM3;
        }
    }

    // Set B to the correct value
    scError = SymCryptModElementSetValue(
                    pSrc,
                    pParams->cbFieldLength,
                    SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                    pCurve->FMod,
                    pCurve->B,
                    pbScratch,
                    pSizes->cbScratch );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }
    pDst += pSizes->cbModElement;
    pSrc += pParams->cbFieldLength;

    // Skip over the distinguished point until we fix all the parameters and scratch space sizes
    pSrcGenerator = pSrc;
    pSrc += pParams->cbFieldLength * 2;

    // Subgroup Order
    pCurve->GOrd = SymCryptModulusCreate( pDst, pSizes->cbSubgroupOrder, pSizes->nDigitsSubgroupOrder );
    if ( pCurve->GOrd == NULL )
    {
        goto cleanup;
    }

    pTempInt = SymCryptIntFromModulus( pCurve->GOrd );
    if ( pTempInt == NULL)
    {
        goto cleanup;
    }

    scError = SymCryptIntSetValue( pSrc, pParams->cbSubgroupOrder, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, pTempInt );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Subgroup Order Bitsize
    pCurve->GOrdBitsize = SymCryptIntBitsizeOfValue( pTempInt );
    if (pCurve->GOrdBitsize < SYMCRYPT_ECURVE_MIN_BITSIZE_GORD)
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    if( (SymCryptIntGetValueLsbits32( pTempInt ) & 1) == 0 )
    {
        // 'Prime' must be odd to avoid errors in conversion to modulus
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // IntToModulus requirement:
    //      GOrdBitsize >= SYMCRYPT_ECURVE_MIN_BITSIZE_GORD --> pTempInt > 0
    SymCryptIntToModulus(
            pTempInt,
            pCurve->GOrd,
            SYMCRYPT_INTERNAL_ECURVE_GROUP_ORDER_NUMOF_OPERATIONS,
            SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
            pbScratch,
            pSizes->cbScratch );

    pDst += pSizes->cbSubgroupOrder;
    pSrc += pParams->cbSubgroupOrder;

    // Cofactor
    pCurve->H = SymCryptIntCreate( pDst, pSizes->cbCoFactor, pSizes->nDigitsCoFactor );
    if ( pCurve->H == NULL )
    {
        goto cleanup;
    }
    scError = SymCryptIntSetValue(
                    pSrc,
                    pParams->cbCofactor,
                    SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                    pCurve->H );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }


    // Make sure that the cofactor is not zero or too big
    pCurve->coFactorPower = SymCryptIntBitsizeOfValue( pCurve->H ) - 1;
    if (pCurve->coFactorPower == (UINT32)-1 || pCurve->coFactorPower > SYMCRYPT_ECURVE_MAX_COFACTOR_POWER)
    {
        goto cleanup;
    }

    // Validate that the cofactor is a power of two
    if (!SymCryptIntIsEqualUint32( pCurve->H, 1<<(pCurve->coFactorPower) ))
    {
        goto cleanup;
    }

    pDst += pSizes->cbCoFactor;
    pSrc += pParams->cbCofactor;

    // Calculate scratch spaces' sizes
    if (pParams->type == SYMCRYPT_ECURVE_TYPE_SHORT_WEIERSTRASS)
    {
        pCurve->info.sw.window = SYMCRYPT_ECURVE_SW_DEF_WINDOW;
        pCurve->info.sw.nPrecompPoints = (1 << (SYMCRYPT_ECURVE_SW_DEF_WINDOW-2));
        pCurve->info.sw.nRecodedDigits = pCurve->GOrdBitsize + 1;               // This is the maximum - used by the wNAF Interleaving method
    }
    else if ( pParams->type == SYMCRYPT_ECURVE_TYPE_TWISTED_EDWARDS )
    {
        pCurve->info.sw.window = SYMCRYPT_ECURVE_SW_DEF_WINDOW;
        pCurve->info.sw.nPrecompPoints = (1 << (SYMCRYPT_ECURVE_SW_DEF_WINDOW-2));
        pCurve->info.sw.nRecodedDigits = pCurve->GOrdBitsize + 1;               // This is the maximum - used by the wNAF Interleaving method
    }

    SymCryptEcurveFillScratchSpaces(pCurve);

    // Now set the distinguished point
    pCurve->G = SymCryptEcpointCreate( pDst, pSizes->cbEcpoint, pCurve );
    if ( pCurve->G == NULL )
    {
        goto cleanup;
    }
    scError = SymCryptEcpointSetValue(
                    pCurve,
                    pSrcGenerator,
                    pParams->cbFieldLength * 2,
                    SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                    SYMCRYPT_ECPOINT_FORMAT_XY,
                    pCurve->G,
                    SYMCRYPT_FLAG_DATA_PUBLIC,
                    pbScratch,
                    pSizes->cbScratch );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }
    pDst += pSizes->cbEcpoint;

    // Fill the precomputed table
    if ( (pParams->type == SYMCRYPT_ECURVE_TYPE_SHORT_WEIERSTRASS) ||
         (pParams->type == SYMCRYPT_ECURVE_TYPE_TWISTED_EDWARDS) )
    {
        // The first point of the table is the generator
        pCurve->info.sw.poPrecompPoints[0] = pCurve->G;

        for (UINT32 i=1; i<pCurve->info.sw.nPrecompPoints; i++)
        {
            pCurve->info.sw.poPrecompPoints[i] = SymCryptEcpointCreate( pDst, pSizes->cbEcpoint, pCurve );
            if ( pCurve->info.sw.poPrecompPoints[i] == NULL )
            {
                goto cleanup;
            }
            pDst += pSizes->cbEcpoint;
        }

        SymCryptOfflinePrecomputation( pCurve, pbScratch, pSizes->cbScratch );
    }

    // For Montgomery curve, we calculate A = (A + 2) / 4
    if (pParams->type == SYMCRYPT_ECURVE_TYPE_MONTGOMERY)
    {
        peTemp = SymCryptModElementCreate( pbScratch, pSizes->cbModElement, pCurve->FMod );

        // SetValueUint32 requirements:
        //  FMod > 2 since it has more than SYMCRYPT_ECURVE_MIN_BITSIZE_FMOD bits
        SymCryptModElementSetValueUint32( 2, pCurve->FMod, peTemp, pbScratch + pSizes->cbModElement, pSizes->cbScratch - pSizes->cbModElement );
        SymCryptModAdd (pCurve->FMod, pCurve->A, peTemp, pCurve->A, pbScratch + pSizes->cbModElement, pSizes->cbScratch - pSizes->cbModElement );   // A = A + 2;
        SymCryptModDivPow2( pCurve->FMod, pCurve->A, 2, pCurve->A, pbScratch + pSizes->cbModElement, pSizes->cbScratch - pSizes->cbModElement );    // A = (A + 2) / 4
    }

    // Set the default curve policy for parameters of version 2
    if (pParams->version == 2)
    {
        // Skip over the seed (if any)
        pSrc += pParams->cbSeed;

        // Copy the extension info (it can be unaligned)
        pcParamsV2Ext = (PCSYMCRYPT_ECURVE_PARAMS_V2_EXTENSION) pSrc;
    }
    else
    {
        // Set the defaults for version 1
        if (pParams->type == SYMCRYPT_ECURVE_TYPE_SHORT_WEIERSTRASS)
        {
            pcParamsV2Ext = SymCryptEcurveParamsV2ExtensionShortWeierstrass;
        }
        else if ( pParams->type == SYMCRYPT_ECURVE_TYPE_TWISTED_EDWARDS )
        {
            pcParamsV2Ext = SymCryptEcurveParamsV2ExtensionTwistedEdwards;
        }
        else if ( pParams->type == SYMCRYPT_ECURVE_TYPE_MONTGOMERY )
        {
            pcParamsV2Ext = SymCryptEcurveParamsV2ExtensionMontgomery;
        }
    }

    pCurve->PrivateKeyDefaultFormat = pcParamsV2Ext->PrivateKeyDefaultFormat;
    pCurve->HighBitRestrictionNumOfBits = pcParamsV2Ext->HighBitRestrictionNumOfBits;
    pCurve->HighBitRestrictionPosition = pcParamsV2Ext->HighBitRestrictionPosition;
    pCurve->HighBitRestrictionValue = pcParamsV2Ext->HighBitRestrictionValue;

    // Make sure that the HighBitRestrictions make sense
    // (see SymCryptIntGet/SetBits)
    if ( (pCurve->HighBitRestrictionNumOfBits>32) ||
         ((pCurve->HighBitRestrictionNumOfBits>0) &&
          (pCurve->HighBitRestrictionPosition + pCurve->HighBitRestrictionNumOfBits > pCurve->GOrdBitsize + pCurve->coFactorPower)) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Setting the magic
    SYMCRYPT_SET_MAGIC( pCurve );

    fSuccess = TRUE;

cleanup:
    if (!fSuccess)
    {
        SymCryptWipe( pbCurve, pSizes->cbAlloc );
        pCurve = NULL;
    }

    return pCurve;
}

PSYMCRYPT_ECURVE
SYMCRYPT_CALL
SymCryptEcurveCreate(
    _In_                                PSYMCRYPT_ECURVE_PARAMS pParams,
    _In_                                UINT32                  flags,
    _Out_writes_bytes_( cbCurve )       PBYTE                   pbCurve,
                                        SIZE_T                  cbCurve,
    _Out_writes_bytes_( cbScratch )     PBYTE                   pbScratch,
                                        SIZE_T                  cbScratch)
{
    SYMCRYPT_ECURVE_SIZES sizes;

    PSYMCRYPT_ECURVE pCurve = NULL;

    if ( !SymCryptEcurveValidateAndComputeSizes(pParams, &sizes) )
    {
        goto cleanup;
    }

    if ( cbCurve < sizes.cbAlloc )
    {
        goto cleanup;
    }

    if ( cbScratch < sizes.cbScratch )
    {
        goto cleanup;
    }

    pCurve = SymCryptEcurveInitialize( pParams, flags, &sizes, pbCurve, pbScratch );

cleanup:
    return pCurve;
}

PSYMCRYPT_ECURVE
SYMCRYPT_CALL
SymCryptEcurveAllocate(
    _In_    PCSYMCRYPT_ECURVE_PARAMS    pParams,
    _In_    UINT32                      flags )
{
    SYMCRYPT_ECURVE_SIZES sizes;

    PBYTE pbCurve = NULL;
    PBYTE pbScratch = NULL;

    PSYMCRYPT_ECURVE pCurve = NULL;

    if ( !SymCryptEcurveValidateAndComputeSizes(pParams, &sizes) )
    {
        goto cleanup;
    }

    pbCurve = SymCryptCallbackAlloc( sizes.cbAlloc );
    if ( pbCurve == NULL )
    {
        goto cleanup;
    }

    pbScratch = SymCryptCallbackAlloc( sizes.cbScratch );
    if ( pbScratch == NULL )
    {
        goto cleanup;
    }

    pCurve = SymCryptEcurveInitialize( pParams, flags, &sizes, pbCurve, pbScratch );
    if ( pCurve != NULL )
    {
        pbCurve = NULL;
    }

cleanup:
    if ( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, sizes.cbScratch );
        SymCryptCallbackFree( pbScratch );
    }

    if ( pbCurve != NULL )
    {
        SymCryptCallbackFree( pbCurve );
    }

    return pCurve;
}

VOID
SYMCRYPT_CALL
SymCryptEcurveFree( _Out_ PSYMCRYPT_ECURVE pCurve )
{
    SYMCRYPT_CHECK_MAGIC( pCurve );

    SymCryptWipe( (PBYTE) pCurve, pCurve->cbAlloc );

    SymCryptCallbackFree( pCurve );
}

UINT32
SYMCRYPT_CALL
SymCryptEcurveBitsizeofFieldModulus( _In_ PCSYMCRYPT_ECURVE pCurve )
{
    return pCurve->FModBitsize;
}

UINT32
SYMCRYPT_CALL
SymCryptEcurveBitsizeofGroupOrder( _In_ PCSYMCRYPT_ECURVE pCurve )
{
    return pCurve->GOrdBitsize;
}

UINT32
SYMCRYPT_CALL
SymCryptEcurveDigitsofFieldElement( _In_ PCSYMCRYPT_ECURVE pCurve )
{
    return pCurve->FModDigits;
}

UINT32
SYMCRYPT_CALL
SymCryptEcurveSizeofFieldElement( _In_ PCSYMCRYPT_ECURVE pCurve )
{
    return pCurve->FModBytesize;
}

UINT32
SYMCRYPT_CALL
SymCryptEcurveSizeofScalarMultiplier( _In_ PCSYMCRYPT_ECURVE pCurve )
{
    return pCurve->GOrdBytesize;
}

PCSYMCRYPT_MODULUS
SYMCRYPT_CALL
SymCryptEcurveGroupOrder( _In_    PCSYMCRYPT_ECURVE   pCurve )
{
    return pCurve->GOrd;
}

UINT32
SYMCRYPT_CALL
SymCryptEcurveDigitsofScalarMultiplier( _In_    PCSYMCRYPT_ECURVE   pCurve )
{
    return SymCryptDigitsFromBits( pCurve->GOrdBitsize + pCurve->coFactorPower );
}

UINT32
SYMCRYPT_CALL
SymCryptEcurvePrivateKeyDefaultFormat( _In_ PCSYMCRYPT_ECURVE pCurve )
{
    return pCurve->PrivateKeyDefaultFormat;
}

UINT32
SYMCRYPT_CALL
SymCryptEcurveHighBitRestrictionNumOfBits( _In_ PCSYMCRYPT_ECURVE pCurve )
{
    return pCurve->HighBitRestrictionNumOfBits;
}

UINT32
SYMCRYPT_CALL
SymCryptEcurveHighBitRestrictionPosition( _In_ PCSYMCRYPT_ECURVE pCurve )
{
    return pCurve->HighBitRestrictionPosition;
}

UINT32
SYMCRYPT_CALL
SymCryptEcurveHighBitRestrictionValue( _In_ PCSYMCRYPT_ECURVE pCurve )
{
    return pCurve->HighBitRestrictionValue;
}

BOOLEAN
SYMCRYPT_CALL
SymCryptEcurveIsSame(
    _In_    PCSYMCRYPT_ECURVE  pCurve1,
    _In_    PCSYMCRYPT_ECURVE  pCurve2)
{
    BOOLEAN fIsSameCurve = FALSE;

    if ( pCurve1 == pCurve2 )
    {
        fIsSameCurve = TRUE;
        goto cleanup;
    }

    if ( (pCurve1->type != pCurve2->type) ||
         !SymCryptIntIsEqual (
              SymCryptIntFromModulus( pCurve1->FMod ),
              SymCryptIntFromModulus( pCurve2->FMod ) ) ||
         !SymCryptModElementIsEqual ( pCurve1->FMod, pCurve1->A, pCurve2->A ) ||
         !SymCryptModElementIsEqual ( pCurve1->FMod, pCurve1->B, pCurve2->B ) )
    {
        goto cleanup;
    }

    fIsSameCurve = TRUE;

cleanup:
    return fIsSameCurve;
}
