//
// fdef_int.c   INT functions for default number format
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

PSYMCRYPT_MODULUS
SYMCRYPT_CALL
SymCryptFdefModulusAllocate( UINT32 nDigits )
{
    PVOID               p = NULL;
    UINT32              cb;
    PSYMCRYPT_MODULUS   res = NULL;

    //
    // The nDigits requirements are enforced by SymCryptFdefSizeofModulusFromDigits. Thus
    // the result does not overflow and is upper bounded by 2^19.
    //
    cb = SymCryptFdefSizeofModulusFromDigits( nDigits );

    if( cb != 0 )
    {
        p = SymCryptCallbackAlloc( cb );
    }

    if( p == NULL )
    {
        goto cleanup;
    }

    res = SymCryptFdefModulusCreate( p, cb, nDigits );

cleanup:
    return res;
}

VOID
SYMCRYPT_CALL
SymCryptFdefModulusFree( _Out_ PSYMCRYPT_MODULUS pmObj )
{
    SymCryptModulusWipe( pmObj );
    SymCryptCallbackFree( pmObj );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefSizeofModulusFromDigits( UINT32 nDigits )
{
    SYMCRYPT_ASSERT( nDigits != 0 );
    SYMCRYPT_ASSERT( nDigits <= SYMCRYPT_FDEF_UPB_DIGITS );

    // Ensure we do not overflow the following calculation when provided with invalid inputs
    if( nDigits == 0 || nDigits > SYMCRYPT_FDEF_UPB_DIGITS )
    {
        return 0;
    }

    // Room for the Modulus structure, the Divisor, the negated divisor, and the R^2 Montgomery factor
    //
    return SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, Divisor ) + SymCryptFdefSizeofDivisorFromDigits( nDigits ) + (2 * nDigits * SYMCRYPT_FDEF_DIGIT_SIZE);
}

PSYMCRYPT_MODULUS
SYMCRYPT_CALL
SymCryptFdefModulusCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer,
                                    UINT32  nDigits )
{
    PSYMCRYPT_MODULUS pmMod = NULL;
    UINT32 cb = SymCryptFdefSizeofModulusFromDigits( nDigits );

    const UINT32 offset = SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, Divisor );

    SYMCRYPT_ASSERT( cb >= sizeof(SYMCRYPT_MODULUS) );
    SYMCRYPT_ASSERT( cbBuffer >= cb );
    if( (cb == 0) || (cbBuffer < cb) )
    {
        goto cleanup; // return NULL
    }

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbBuffer );
    pmMod = (PSYMCRYPT_MODULUS) pbBuffer;

    pmMod->type = 'gM' << 16;
    pmMod->nDigits = nDigits;

    //
    // The nDigits requirements are enforced by SymCryptFdefSizeofModulusFromDigits. Thus
    // the result does not overflow and is upper bounded by 2^19.
    //
    pmMod->cbSize = cb;
    pmMod->flags = 0;

    // The following is bounded by 2^17
    pmMod->cbModElement = nDigits * SYMCRYPT_FDEF_DIGIT_SIZE;

    SymCryptFdefDivisorCreate( pbBuffer + offset, cbBuffer - offset, nDigits );

    // We don't have a modulus value yet, so we don't create/initialize any implementation-specific things.

    SYMCRYPT_SET_MAGIC( pmMod );

cleanup:
    return pmMod;
}

VOID
SYMCRYPT_CALL
SymCryptFdefModulusInitGeneric(
    _Inout_                         PSYMCRYPT_MODULUS       pmMod,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UNREFERENCED_PARAMETER( pmMod );
    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );
}


VOID
SymCryptFdefModulusCopy(
    _In_    PCSYMCRYPT_MODULUS  pmSrc,
    _Out_   PSYMCRYPT_MODULUS   pmDst )
{
    SYMCRYPT_ASSERT( pmSrc->nDigits == pmDst->nDigits );

    if( pmSrc != pmDst )
    {
        memcpy( pmDst, pmSrc, pmDst->cbSize );

        SymCryptFdefDivisorCopyFixup( &pmSrc->Divisor, &pmDst->Divisor );

        // Copy the type-specific fields
        SYMCRYPT_MOD_CALL( pmSrc ) modulusCopyFixup( pmSrc, pmDst );

        SYMCRYPT_SET_MAGIC( pmDst );
    }
}

VOID
SYMCRYPT_CALL
SymCryptFdefModulusCopyFixupGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmSrc,
    _Out_                           PSYMCRYPT_MODULUS       pmDst )
{
    // Only have to handle the type-specific fields, which we don't have any of.
    UNREFERENCED_PARAMETER( pmSrc );
    UNREFERENCED_PARAMETER( pmDst );
}


PSYMCRYPT_MODELEMENT
SYMCRYPT_CALL
SymCryptFdefModElementAllocate( _In_ PCSYMCRYPT_MODULUS pmMod )
{
    PVOID                   p;
    UINT32                  cb;
    PSYMCRYPT_MODELEMENT    res = NULL;

    //
    // The nDigits requirements are enforced by the modulus object. Thus
    // the result does not overflow and is upper bounded by 2^17.
    //
    cb = SymCryptFdefSizeofModElementFromModulus( pmMod );

    p = SymCryptCallbackAlloc( cb );

    if( p == NULL )
    {
        goto cleanup;
    }

    res = SymCryptFdefModElementCreate( p, cb, pmMod );

cleanup:
    return res;
}

VOID
SYMCRYPT_CALL
SymCryptFdefModElementFree(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _Out_   PSYMCRYPT_MODELEMENT    peObj )
{
    SymCryptFdefModElementWipe( pmMod, peObj );
    SymCryptCallbackFree( peObj );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefSizeofModElementFromModulus( PCSYMCRYPT_MODULUS pmMod )
{
    // Upper bounded by 2^17 since the modulus is up to SYMCRYPT_INT_MAXBITS = 2^20 bits.
    return pmMod->cbModElement;
}

PSYMCRYPT_MODELEMENT
SYMCRYPT_CALL
SymCryptFdefModElementCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE               pbBuffer,
                                    SIZE_T              cbBuffer,
                                    PCSYMCRYPT_MODULUS  pmMod )
{
    PSYMCRYPT_MODELEMENT pDst = (PSYMCRYPT_MODELEMENT) pbBuffer;

    UNREFERENCED_PARAMETER( pmMod );
    UNREFERENCED_PARAMETER( cbBuffer );

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbBuffer );
    SYMCRYPT_ASSERT( cbBuffer >= SymCryptFdefSizeofModElementFromModulus( pmMod ) );
    SYMCRYPT_ASSERT( cbBuffer >= pmMod->nDigits*SYMCRYPT_FDEF_DIGIT_SIZE );

    //
    // We have various optimizations where we use only part of the last digit
    // Simple and fast solution: always wipe the last digit
    //
#if (SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64)
    UINT32 nDigits = pmMod->nDigits;

    SymCryptWipeKnownSize( pbBuffer + (nDigits-1) * SYMCRYPT_FDEF_DIGIT_SIZE, SYMCRYPT_FDEF_DIGIT_SIZE );
#endif

    // There is nothing to initialize...

    return pDst;
}

VOID
SYMCRYPT_CALL
SymCryptFdefModElementWipe(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _Out_   PSYMCRYPT_MODELEMENT    peDst )
{
    SymCryptWipe( peDst, pmMod->cbModElement );
}

VOID
SymCryptFdefModElementCopy(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _In_    PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_   PSYMCRYPT_MODELEMENT    peDst )
{
    if( peSrc != peDst )
    {
        memcpy( peDst, peSrc, pmMod->cbModElement );
    }
}

VOID
SymCryptFdefModElementMaskedCopy(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _In_    PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_   PSYMCRYPT_MODELEMENT    peDst,
            UINT32                  mask )
{
    SymCryptFdefMaskedCopy( (PCBYTE) peSrc, (PBYTE) peDst, pmMod->nDigits, mask );
}


PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptFdefDivisorFromModulus( _In_ PSYMCRYPT_MODULUS pmSrc )
{
    return &pmSrc->Divisor;
}

VOID
SymCryptFdefModElementConditionalSwap(
    _In_       PCSYMCRYPT_MODULUS    pmMod,
    _Inout_    PSYMCRYPT_MODELEMENT  peData1,
    _Inout_    PSYMCRYPT_MODELEMENT  peData2,
    _In_       UINT32                cond )
{
    SymCryptFdefConditionalSwap( (PBYTE) &peData1->d.uint32[0], (PBYTE) &peData2->d.uint32[0], pmMod->nDigits, cond );
}

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptFdefIntFromModulus( _In_ PSYMCRYPT_MODULUS pmSrc )
{

    return SymCryptFdefIntFromDivisor( &pmSrc->Divisor );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefDecideModulusType( PCSYMCRYPT_INT piSrc, UINT32 nDigits, UINT32 averageOperations, UINT32 flags )
{
    UINT32 res = 0;
    BOOLEAN disableMontgomery = 0;
    BYTE tempBuf[64];
    PCSYMCRYPT_MODULUS_TYPE_SELECTION_ENTRY pEntry;

    UINT32 nBitsizeOfValue = SymCryptIntBitsizeOfValue( piSrc );
    UINT32 modulusFeatures = 0;

    if( !disableMontgomery &&
        ( flags & (SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PARITY_PUBLIC)) != 0 &&
        (SymCryptIntGetValueLsbits32( piSrc ) & 1) == 1 &&
        averageOperations >= 10 )
    {
        modulusFeatures |= SYMCRYPT_MODULUS_FEATURE_MONTGOMERY;

        // Specific modulus value detection
        if( (flags & SYMCRYPT_FLAG_DATA_PUBLIC) != 0 )
        {
            // Detect if modulus value is the P384 field modulus (convert piSrc to big endian and do comparison with known value of P384 modulus)
            if( nBitsizeOfValue == 384 &&
                SymCryptFdefRawGetValue(SYMCRYPT_FDEF_INT_PUINT32(piSrc), SYMCRYPT_FDEF_DIGITS_FROM_BITS(384), tempBuf, 64, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST) == SYMCRYPT_NO_ERROR )
            {
                // First 16 bytes are guaranteed to be zero because nBitsizeOfValue is 384
                if( memcmp(tempBuf+16, ((PBYTE)SymCryptEcurveParamsNistP384) + sizeof(SYMCRYPT_ECURVE_PARAMS), 48) == 0 )
                {
                    modulusFeatures |= SYMCRYPT_MODULUS_FEATURE_NISTP384;
                }
            }

            // Detect if modulus value is the P256 field modulus (not currently used)
            // if( nBitsizeOfValue == 256 &&
            //     SymCryptFdefRawGetValue(SYMCRYPT_FDEF_INT_PUINT32(piSrc), SYMCRYPT_FDEF_DIGITS_FROM_BITS(256), tempBuf, 64, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST) == SYMCRYPT_NO_ERROR )
            // {
            //     // First 32 bytes are guaranteed to be zero because nBitsizeOfValue is 256
            //     if( memcmp(tempBuf+32, ((PBYTE)SymCryptEcurveParamsNistP256) + sizeof(SYMCRYPT_ECURVE_PARAMS), 32) == 0 )
            //     {
            //         modulusFeatures |= SYMCRYPT_MODULUS_FEATURE_NISTP256;
            //     }
            // }
        }
    }

    pEntry = SymCryptModulusTypeSelections;

    for(;;)
    {
        if( SYMCRYPT_CPU_FEATURES_PRESENT( pEntry->cpuFeatures ) &&
            (pEntry->maxBits == 0 || (nDigits <= SymCryptDigitsFromBits( pEntry->maxBits ) && nBitsizeOfValue <= pEntry->maxBits )) &&
            (pEntry->modulusFeatures & ~modulusFeatures) == 0
            )
        {
            res = pEntry->type;
            break;
        }
        pEntry++;
    }

    return res;
}

VOID
SYMCRYPT_CALL
SymCryptFdefModSetPostGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PSYMCRYPT_MODELEMENT    peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UNREFERENCED_PARAMETER( pmMod );
    UNREFERENCED_PARAMETER( peObj );
    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );
}

PCUINT32
SYMCRYPT_CALL
SymCryptFdefModPreGetGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UNREFERENCED_PARAMETER( pmMod );
    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );

    return &peObj->d.uint32[0];
}



VOID
SYMCRYPT_CALL
SymCryptFdefIntToModulus(
    _In_                            PCSYMCRYPT_INT      piSrc,
    _Out_                           PSYMCRYPT_MODULUS   pmDst,
                                    UINT32              averageOperations,
                                    UINT32              flags,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch )
{
    pmDst->flags = flags;
    SymCryptIntToDivisor( piSrc, &pmDst->Divisor, averageOperations, flags & SYMCRYPT_FLAG_DATA_PUBLIC, pbScratch, cbScratch );

    pmDst->type = SymCryptFdefDecideModulusType( piSrc, pmDst->nDigits, averageOperations, flags );

    // Set inv64 - note the value is only valid if the modulus is odd, but the computation
    // is constant time regardless of the parity, so we can safely compute it in all cases
    pmDst->inv64 = 0 - SymCryptInverseMod2e64( SymCryptIntGetValueLsbits64(piSrc) );

    SYMCRYPT_MOD_CALL( pmDst ) modulusInit( pmDst, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptFdefIntToModElement(
    _In_                            PCSYMCRYPT_INT          piSrc,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SymCryptFdefRawDivMod(
        SYMCRYPT_FDEF_INT_PUINT32( piSrc ),
        piSrc->nDigits,
        &pmMod->Divisor,
        NULL,                   // throw away the quotient
        &peDst->d.uint32[0],
        pbScratch,
        cbScratch );

    SYMCRYPT_MOD_CALL( pmMod ) modSetPost( pmMod, peDst, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptFdefModElementToIntGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_reads_bytes_( pmMod->nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )
                                    PCUINT32                pSrc,
    _Out_                           PSYMCRYPT_INT           piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    memcpy( SYMCRYPT_FDEF_INT_PUINT32( piDst ), pSrc, pmMod->nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );

    SymCryptWipe( &SYMCRYPT_FDEF_INT_PUINT32( piDst )[pmMod->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32], (piDst->nDigits - pmMod->nDigits) * SYMCRYPT_FDEF_DIGIT_SIZE );

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pmMod->nDigits ) );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefModElementSetValueGeneric(
    _In_reads_bytes_( cbSrc )       PCBYTE                  pbSrc,
                                    SIZE_T                  cbSrc,
                                    SYMCRYPT_NUMBER_FORMAT  format,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR scError;
    UINT32  nDigits = pmMod->nDigits;

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );

    SYMCRYPT_ASSERT( cbSrc <= nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );

    scError = SymCryptFdefRawSetValue( pbSrc, cbSrc, format, &peDst->d.uint32[0], nDigits );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    SymCryptFdefRawDivMod(
        &peDst->d.uint32[0],
        nDigits,
        &pmMod->Divisor,
        NULL,
        &peDst->d.uint32[0],
        pbScratch,
        cbScratch );

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefModElementGetValue(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_writes_bytes_( cbDst )     PBYTE                   pbDst,
                                    SIZE_T                  cbDst,
                                    SYMCRYPT_NUMBER_FORMAT  format,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR scError;
    PCUINT32 pUint32;
    UINT32  nDigits = pmMod->nDigits;


    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );

    SYMCRYPT_ASSERT( cbDst <= nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );

    pUint32 = SYMCRYPT_MOD_CALL( pmMod ) modPreGet( pmMod, peSrc, pbScratch, cbScratch );

    scError = SymCryptFdefRawGetValue( pUint32, nDigits, pbDst, cbDst, format );

    return scError;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefModElementIsEqual(
    _In_    PCSYMCRYPT_MODULUS     pmMod,
    _In_    PCSYMCRYPT_MODELEMENT  peSrc1,
    _In_    PCSYMCRYPT_MODELEMENT  peSrc2 )
{
    UINT32 d;
    UINT32 i;

    d = 0;
    for( i=0; i < pmMod->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32 ; i++ )
    {
        d |= peSrc1->d.uint32[i] ^ peSrc2->d.uint32[i];
    }

    return SYMCRYPT_MASK32_ZERO( d );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefModElementIsZero(
    _In_    PCSYMCRYPT_MODULUS     pmMod,
    _In_    PCSYMCRYPT_MODELEMENT  peSrc )
{
    UINT32 d;
    UINT32 i;

    d = 0;
    for( i=0; i < pmMod->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32 ; i++ )
    {
        d |= peSrc->d.uint32[i];        // Check that all bits are zero
    }

    return SYMCRYPT_MASK32_ZERO( d );
}

VOID
SYMCRYPT_CALL
SymCryptFdefModAddGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 c;
    UINT32 d;
    UINT32 nDigits = pmMod->nDigits;

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );
    SYMCRYPT_ASSERT( cbScratch >= nDigits*SYMCRYPT_FDEF_DIGIT_SIZE );

    //
    // Doing add/cmp/sub might be faster or not.
    // Masked add is hard because the mask operations destroy the carry flag.
    //

	// dcl - cleanup?

//    c = SymCryptFdefRawAdd( &pSrc1->uint32[0], &pSrc2->uint32[0], &pDst->uint32[0], nDigits);
//    d = SymCryptFdefRawSub( &pDst->uint32[0], &pMod->Divisor.Int.uint32[0], &pDst->uint32[0], nDigits );
//    e = SymCryptFdefRawMaskedAdd( &pDst->uint32[0], &pMod->Divisor.Int.uint32[0], 0 - (c^d), nDigits );

    c = SymCryptFdefRawAdd( &peSrc1->d.uint32[0], &peSrc2->d.uint32[0], &peDst->d.uint32[0], nDigits );
    d = SymCryptFdefRawSub( &peDst->d.uint32[0], SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int ), (PUINT32) pbScratch, nDigits );
    SymCryptFdefMaskedCopy( pbScratch, (PBYTE) &peDst->d.uint32[0], nDigits, (c^d) - 1 );

    // We can't have a carry in the first addition, and no carry in the subtraction.
    SYMCRYPT_ASSERT( !( c == 1 && d == 0 ) );
}

VOID
SYMCRYPT_CALL
SymCryptFdefModSubGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 c;
    UINT32 d;
    UINT32 nDigits = pmMod->nDigits;

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );
    SYMCRYPT_ASSERT( cbScratch >= nDigits*SYMCRYPT_FDEF_DIGIT_SIZE );

    c = SymCryptFdefRawSub( &peSrc1->d.uint32[0], &peSrc2->d.uint32[0], &peDst->d.uint32[0], nDigits );
    d = SymCryptFdefRawAdd( &peDst->d.uint32[0], SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int ), (PUINT32) pbScratch, nDigits );
    SymCryptFdefMaskedCopy( pbScratch, (PBYTE) &peDst->d.uint32[0], nDigits, 0 - c );

    SYMCRYPT_ASSERT( !(c == 1 && d == 0) );
}


VOID
SYMCRYPT_CALL
SymCryptFdefModNegGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    UINT32 isZero;
    UINT32 i;

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );

    //
    // We have to be careful to handle the value 0 properly as it does NOT map to Modulus - Value.
    //
    isZero = SymCryptFdefRawIsEqualUint32( &peSrc->d.uint32[0], nDigits , 0 );
    SymCryptFdefRawSub( SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int ), &peSrc->d.uint32[0], &peDst->d.uint32[0], nDigits );

    // Now we set the result to zero if the input was zero
    for( i=0; i< nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32; i++ )
    {
        peDst->d.uint32[i] &= ~isZero;
    }
}

VOID
SYMCRYPT_CALL
SymCryptFdefModElementSetValueUint32Generic(
                                    UINT32                  value,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );

    if( pmMod->Divisor.nBits <= 32 && value >= SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int )[0] )
    {
        // The value is >=  the modulus; this is not supported

        // For now do a possibly non-sidechannel safe, but mathematically correct modulo operation
        value %= SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int )[0];
    }

    peDst->d.uint32[0] = value;

    SymCryptWipe( &peDst->d.uint32[1], nDigits * SYMCRYPT_FDEF_DIGIT_SIZE - sizeof( UINT32 ) );
}

VOID
SYMCRYPT_CALL
SymCryptFdefModElementSetValueNegUint32(
                                    UINT32                  value,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );

    if( pmMod->Divisor.nBits <= 32 && value >= SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int )[0] )
    {
        // The value is >=  the modulus; this is not supported.

        // For now do a possibly non-sidechannel safe, but mathematically correct modulo operation
        value %= SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int )[0];
    }

    if( value == 0 )
    {
        SymCryptWipe( &peDst->d.uint32[0], nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );
    } else {
        SymCryptFdefRawSubUint32( SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int ), value, &peDst->d.uint32[0], nDigits );
    }

    //
    // Possible future optimization: we can optimize the value==0 and value==1 cases on a per-type basis
    //
    SYMCRYPT_MOD_CALL( pmMod ) modSetPost( pmMod, peDst, pbScratch, cbScratch );
}

// In the worst case there is a 1 in 8 chance of successfully generating a value
// This is when the modulus is 4 (nBits of modulus is 3), and 0, 1, and -1 are disallowed.
// In this case, having 1000 retries, there is a ~ 2^-193 chance of failure unless SymCryptCallbackRandom
// is completely broken. This passes the bar of being reasonable to Fatal.
#define FDEF_MOD_SET_RANDOM_GENERIC_LIMIT   (1000)

VOID
SYMCRYPT_CALL
SymCryptFdefModSetRandomGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 offset;
    UINT32 ulimit;
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;
    UINT32 nUsedBytes;
    UINT32 mask;
    UINT32 c;
    UINT32 cntr;
    PUINT32 pDst = &peDst->d.uint32[0];
    PCUINT32 pMod = SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int );

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );

    if( (flags & SYMCRYPT_FLAG_MODRANDOM_ALLOW_ZERO) != 0 )
    {
        // SYMCRYPT_FLAG_MODRANDOM_ALLOW_ZERO => SYMCRYPT_FLAG_MODRANDOM_ALLOW_ONE
        offset = 0;
    } else if( (flags & SYMCRYPT_FLAG_MODRANDOM_ALLOW_ONE) != 0 )
    {
        offset = 1;
    } else
    {
        offset = 2;
    }

    if( (flags & SYMCRYPT_FLAG_MODRANDOM_ALLOW_MINUSONE ) )
    {
        ulimit = 0;
    } else {
        ulimit = 1;
    }

    //
    // Special case for small divisors:
    //  When the divisor is 1, 2, or 3 we always allow returning -1
    //  We may also allow returning 1 or 0 depending on the flags specified
    if ( pmMod->Divisor.nBits < 3 )
    {
        // At a minimum, allow -1
        offset = SYMCRYPT_MIN(offset, pMod[0] - 1);
        ulimit = 0;
    }

    // Set pTmp to pMod-(offset+ulimit)
    SYMCRYPT_ASSERT( nDigits * SYMCRYPT_FDEF_DIGIT_SIZE <= cbScratch );
    c = SymCryptFdefRawSubUint32( pMod, offset + ulimit, pTmp, nDigits );
    SYMCRYPT_ASSERT( c == 0 );

    nUsedBytes = (pmMod->Divisor.nBits + 7)/8;
    mask = 0x100 >> ( (8-pmMod->Divisor.nBits) & 7);
    mask -= 1;

    // Wipe any bytes we won't fill with random
    SymCryptWipe( (PBYTE)pDst + nUsedBytes, (nDigits * SYMCRYPT_FDEF_DIGIT_SIZE) - nUsedBytes );

    for(cntr=0; cntr<FDEF_MOD_SET_RANDOM_GENERIC_LIMIT; cntr++)
    {
        // Try random values until we get one we like
        SymCryptCallbackRandom( (PBYTE)pDst, nUsedBytes );
        ((PBYTE)pDst)[nUsedBytes-1] &= (BYTE) mask;

        // Compare value to pMod-(offset+ulimit)
        if( SymCryptFdefRawIsLessThan( pDst, pTmp, nDigits ) )
        {
            // The value is within required range [0, Divisor-offset-ulimit)
            break;
        }
    }

    // Wipe all the digits in pTmp
    SymCryptWipe( pTmp, nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );

    if (cntr >= FDEF_MOD_SET_RANDOM_GENERIC_LIMIT)
    {
        SymCryptFatal( 'rndc');
    }

    // Add the offset which allows us to avoid 0 and/or 1 if required.
    // Now result is in range [offset, Divisor-ulimit)
    c = SymCryptFdefRawAddUint32( pDst, offset, pDst, nDigits );
    SYMCRYPT_ASSERT( c == 0 );
}

VOID
SYMCRYPT_CALL
SymCryptFdefModDivSmallPow2Generic(
    _In_                        PCSYMCRYPT_MODULUS      pmMod,
    _In_                        PCSYMCRYPT_MODELEMENT   peSrc,
    _In_range_(1, NATIVE_BITS)  UINT32                  exp,
    _Out_                       PSYMCRYPT_MODELEMENT    peDst)
{
    UINT32 nDigits = pmMod->nDigits;
    UINT32 mask;
    UINT64 t;
    UINT64 u;
    UINT32 i;
    PCUINT32 pMod = SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int );

    // mod must be odd
    SYMCRYPT_ASSERT( (pMod[0] & 1) != 0 );
    SYMCRYPT_ASSERT( (exp >= 1) && (exp <= NATIVE_BITS) );

    do
    {
        mask = (UINT32)0 - (peSrc->d.uint32[0] & 1);

        t = (UINT64) peSrc->d.uint32[0] + (pMod[0] & mask);
        u = (UINT32) t;
        t >>= 32;

        for( i = 1; i < nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32; i++ )
        {
            t += pMod[i] & mask;
            t += peSrc->d.uint32[i];

            u |= t << 32;

            peDst->d.uint32[i-1] = (UINT32)(u >> 1);
            t >>= 32;
            u >>= 32;
        }
        u |= t << 32;
        peDst->d.uint32[i-1] = (UINT32)( u >> 1 );

        exp -= 1;

        // First iteration reads from peSrc and writes to peDst
        // subsequent iterations must read from and write to peDst
        peSrc = peDst;
    } while (exp > 0);
}

VOID
SYMCRYPT_CALL
SymCryptFdefModDivSmallPow2(
    _In_                        PCSYMCRYPT_MODULUS      pmMod,
    _In_                        PCSYMCRYPT_MODELEMENT   peSrc,
    _In_range_(1, NATIVE_BITS)  UINT32                  exp,
    _Out_                       PSYMCRYPT_MODELEMENT    peDst )
{

#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_MULX ) )
    {
        SymCryptFdefModDivSmallPow2Mulx( pmMod, peSrc, exp, peDst );
    }
    else
    {
        // Currently SymCryptAsm does not support AMD64 functions with shl/shr/shrd
        // by a variable count, as this needs special handling of the rcx (cl) register
        // For now we just fallback to the generic implementation on machines without MULX
        SymCryptFdefModDivSmallPow2Generic( pmMod, peSrc, exp, peDst );
    }
#elif SYMCRYPT_CPU_ARM64
    SymCryptFdefModDivSmallPow2Asm( pmMod, peSrc, exp, peDst );
#else
    SymCryptFdefModDivSmallPow2Generic( pmMod, peSrc, exp, peDst );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptFdefModDivPow2(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
                                    UINT32                  exp,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 shiftAmount;

    UNREFERENCED_PARAMETER(pbScratch);
    UNREFERENCED_PARAMETER(cbScratch);

    // mod must be odd
    SYMCRYPT_ASSERT( (SYMCRYPT_FDEF_INT_PUINT32(&pmMod->Divisor.Int)[0] & 1) != 0 );

    if( exp == 0 )
    {
        // If exp is 0 we just need to copy peSrc to peDst
        SymCryptFdefModElementCopy( pmMod, peSrc, peDst );
        return;
    }

    do
    {
        shiftAmount = SYMCRYPT_MIN(NATIVE_BITS, exp);
        SymCryptFdefModDivSmallPow2( pmMod, peSrc, shiftAmount, peDst );
        exp -= shiftAmount;

        // First iteration reads from peSrc and writes to peDst
        // subsequent iterations must read from and write to peDst
        peSrc = peDst;
    } while( exp > 0 );
}

VOID
SYMCRYPT_CALL
SymCryptFdefModMulGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;
    UINT32  scratchOffset = 2 * nDigits * SYMCRYPT_FDEF_DIGIT_SIZE;

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );
    SYMCRYPT_ASSERT( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) >= scratchOffset + SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_DIVMOD( 2 * nDigits, nDigits ) );
    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbScratch );

    // Tmp space is enough for the product plus the DivMod scratch

    SymCryptFdefRawMul( &peSrc1->d.uint32[0], nDigits, &peSrc2->d.uint32[0], nDigits, pTmp );

    SymCryptFdefRawDivMod( pTmp, 2*nDigits, &pmMod->Divisor, NULL, &peDst->d.uint32[0], pbScratch + scratchOffset, cbScratch - scratchOffset );
}

VOID
SYMCRYPT_CALL
SymCryptFdefModSquareGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;
    UINT32  scratchOffset = 2 * nDigits * SYMCRYPT_FDEF_DIGIT_SIZE;

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );
    SYMCRYPT_ASSERT( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) >= scratchOffset + SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_DIVMOD( 2 * nDigits, nDigits ) );
    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbScratch );

    // Tmp space is enough for the product plus the DivMod scratch

    SymCryptFdefRawSquare( &peSrc->d.uint32[0], nDigits, pTmp );

    SymCryptFdefRawDivMod( pTmp, 2*nDigits, &pmMod->Divisor, NULL, &peDst->d.uint32[0], pbScratch + scratchOffset, cbScratch - scratchOffset );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefModInvGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 nDigits = pmMod->nDigits;
    UINT32 nBytes;
    UINT32 c;
    UINT32 leastSignificantUint32;
    UINT32 trailingZeros;

    //
    // This function is called on Montgomery moduli so we can't directly call specifically optimized modular operations from here.
    //
    // For now we use dispatch functions with pmMod to perform potentially optimized modular operations.
    // This approach makes sense when on average the cost of dispatch is less than the benefit using an optimized operation.
    // The alternative is to make specialized ModInv routines for different types of moduli, but we do not yet do this to
    // reduce code duplication / code size.
    //

    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_MODINV( nDigits ) );

    if( (pmMod->flags & (SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME )) != (SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME ) )
    {
        // Inversion over non-public or non-prime moduli currently not supported.
        // Our blinding below only works for prime moduli.
        // As the modulus cannot be blinded, it requires a fully side-channel safe algorithm which is much more complicated and
        // slower.
        // When this is necessary, we will add a second ModInv implementation for those cases.
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    //
    // Algorithm:
    // R = random nonzero value mod Mod
    // X := Src * R (mod Mod)
    // A = X
    // B = Mod
    // Va = 1
    // Vb = 0
    // invariant: A = Va*X (mod Mod), B = Vb*X (mod Mod),
    //
    // if( A == 0 ): error
    //
    // verify (A | B) is odd
    // if B even: swap (A,B), swap( Va, Vb)
    //
    //  repeat:
    //      while( A even ):
    //          A /= 2; Va /= 2 (mod Mod)
    //      if( A == 1 ): break1
    //      (A, Va, B, Vb) = (B-A, Vb - Va, A, Va)
    //      if( A == 0 ): error (not co-prime)

    nBytes = SymCryptSizeofModElementFromModulus( pmMod );

    SYMCRYPT_ASSERT( cbScratch >= 4*nBytes );
    PSYMCRYPT_MODELEMENT peR = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
    pbScratch += nBytes;
    PSYMCRYPT_MODELEMENT peX = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
    pbScratch += nBytes;
    PSYMCRYPT_MODELEMENT peVa = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
    pbScratch += nBytes;
    PSYMCRYPT_MODELEMENT peVb = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
    pbScratch += nBytes;
    cbScratch -= 4*nBytes;

    PSYMCRYPT_MODELEMENT peVtmpPtr;

    nBytes = SymCryptSizeofIntFromDigits( nDigits );
    SYMCRYPT_ASSERT( cbScratch >= 3 * nBytes );
    PSYMCRYPT_INT piA = SymCryptIntCreate( pbScratch, nBytes, nDigits );
    pbScratch += nBytes;
    PSYMCRYPT_INT piB = SymCryptIntCreate( pbScratch, nBytes, nDigits );
    pbScratch += nBytes;
    PSYMCRYPT_INT piT = SymCryptIntCreate( pbScratch, nBytes, nDigits );
    pbScratch += nBytes;
    cbScratch -= 3*nBytes;

    PSYMCRYPT_INT piTmpPtr;

    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );

    // If the data is not public, multiply by a random blinding factor; otherwise copy the value
    if( (flags & SYMCRYPT_FLAG_DATA_PUBLIC) == 0 )
    {
        SymCryptModSetRandom( pmMod, peR, SYMCRYPT_FLAG_MODRANDOM_ALLOW_ONE | SYMCRYPT_FLAG_MODRANDOM_ALLOW_MINUSONE, pbScratch, cbScratch );   //R = random
        SymCryptModMul( pmMod, peR, peSrc, peX, pbScratch, cbScratch );     // X = R * Src
    } else
    {
        SymCryptModElementCopy( pmMod, peSrc, peX );
    }

    // Set up piA and piB
    SymCryptFdefModElementToIntGeneric( pmMod, &peX->d.uint32[0], piA, pbScratch, cbScratch );   // A = X
    SymCryptIntCopy( SymCryptIntFromModulus( (PSYMCRYPT_MODULUS) pmMod ), piB );          // B = Mod

    // Reject if A = 0, B = 0, or A and B both even
    if( SymCryptIntIsEqualUint32( piA, 0 ) |
        SymCryptIntIsEqualUint32( piB, 0 ) |
        (((SymCryptIntGetValueLsbits32( piA ) | SymCryptIntGetValueLsbits32( piB )) & 1) ^ 1) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if( SymCryptIntIsEqualUint32( piB, 2 ) )
    {
        // Mod = 2 is a valid input. Luckily, modular inversion is easy.
        // The rest of the code assumes that Mod is odd. Other even values are not prime.
        SymCryptModElementCopy( pmMod, peSrc, peDst);
        goto cleanup;
    }

    SymCryptFdefModElementSetValueUint32Generic( 1, pmMod, peVa, pbScratch, cbScratch );               // Va = 1
    SymCryptFdefModElementSetValueUint32Generic( 0, pmMod, peVb, pbScratch, cbScratch );               // Vb = 0

    for(;;)
    {
        // invariant: A = Va*X (mod Mod), B = Vb*X (mod Mod), A != 0, B > 1.
        // Remove factors of 2 from A. This loop terminates because A != 0
        leastSignificantUint32 = SymCryptIntGetValueLsbits32(piA);
        while( (leastSignificantUint32 & 1) == 0 )
        {
            trailingZeros = SymCryptCountTrailingZeros32( leastSignificantUint32 );
            SymCryptIntDivPow2( piA, trailingZeros, piA );
            SymCryptFdefModDivSmallPow2( pmMod, peVa, trailingZeros, peVa );
            leastSignificantUint32 = SymCryptIntGetValueLsbits32(piA);
        }

        if( SymCryptIntIsEqualUint32( piA, 1 ) )
        {
            // A = 1 = Va * X (mod Mod), so Va is the inverse of X
            break;
        }

        c = SymCryptIntSubSameSize( piB, piA, piT );

        // If A != 1 and A=B, then A is the GCD of the original inputs, and there is no inverse
        if( SymCryptIntIsEqualUint32( piT, 0 ) )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        if( c == 0 )
        {
            // B > A, we set B to B-A and swap (B,A)
            // that way we continue our halving on B-A

            SymCryptIntCopy( piT, piB );
            SymCryptModSub( pmMod, peVb, peVa, peVb, pbScratch, cbScratch );

            piTmpPtr  = piB;  piB  = piA;  piA  = piTmpPtr;
            peVtmpPtr = peVb; peVb = peVa; peVa = peVtmpPtr;
        } else {
            // B < A, Set A to A-B and continue halving A
            SymCryptIntNeg( piT, piA );
            SymCryptModSub( pmMod, peVa, peVb, peVa, pbScratch, cbScratch );
        }
    }

    // 1 = A = Va * X (mod Mod), so Va is the inverse of X
    // Check computation that we can test in the debugger
    SymCryptModMul( pmMod, peVa, peX, peVb, pbScratch, cbScratch );

    // Actual answer

    // If the data is not public, multiply by the random blinding factor; otherwise copy the value
    if( (flags & SYMCRYPT_FLAG_DATA_PUBLIC) == 0 )
    {
        SymCryptModMul( pmMod, peVa, peR, peDst, pbScratch, cbScratch );
    } else
    {
        SymCryptModElementCopy( pmMod, peVa, peDst );
    }

cleanup:
    return scError;
}


//=============================
// Montgomery representation

VOID
SYMCRYPT_CALL
SymCryptFdefModulusInitMontgomeryInternal(
    _Inout_                         PSYMCRYPT_MODULUS       pmMod,
                                    UINT32                  nUint32Used,           // R = 2^{32 * this parameter}
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    // Scratch space is big enough for an nDigit+1 byte value + sufficient divmod scratch
    PUINT32 pR2;
    UINT32  cbR2;
    UINT32 nDigits;

    PUINT32 modR2;
    PUINT32 negDivisor;

    nDigits = pmMod->nDigits;
    modR2 = (PUINT32)((PBYTE)&pmMod->Divisor + SymCryptFdefSizeofDivisorFromDigits( nDigits ));

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbScratch );

    pmMod->tm.montgomery.Rsqr = modR2;
    negDivisor = (PUINT32)((PBYTE)modR2 + (nDigits * SYMCRYPT_FDEF_DIGIT_SIZE));

    // We pre-compute R^2 mod M

    pR2 = (PUINT32) pbScratch;
    cbR2 = (2*nDigits + 1) * SYMCRYPT_FDEF_DIGIT_SIZE;
    SYMCRYPT_ASSERT( cbScratch >= cbR2 );
    SYMCRYPT_ASSERT( cbScratch >= 2 * nUint32Used * sizeof(UINT32) );

    // Set it to R^2
    SymCryptWipe( pR2, cbR2 );
    pR2[ 2 * nUint32Used ] = 1;
    SymCryptFdefRawDivMod( pR2, 2*nDigits + 1, &pmMod->Divisor, NULL, modR2, pbScratch + cbR2, cbScratch - cbR2 );

    SymCryptFdefRawNeg( SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int ), 0, negDivisor, nDigits );
}

VOID
SYMCRYPT_CALL
SymCryptFdefModulusInitMontgomery(
    _Inout_                         PSYMCRYPT_MODULUS       pmMod,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SymCryptFdefModulusInitMontgomeryInternal( pmMod, pmMod->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32, pbScratch, cbScratch );
}

VOID
SymCryptFdefMontgomeryReduceC(
    _In_                                                                PCSYMCRYPT_MODULUS  pmMod,
    _Inout_updates_( 2 * pmMod->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32 ) PUINT32             pSrc,
    _Out_writes_( pmMod->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32 )        PUINT32             pDst )
{
    UINT32 nDigits = pmMod->nDigits;
    UINT32 nWords = nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32;
    PCUINT32 pMod = SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int );

    UINT32 hc = 0;
    for( UINT32 i=0; i<nWords; i++ )
    {
        UINT32 m = (UINT32)pmMod->inv64 * pSrc[0];
        UINT64 c = 0;
        for( UINT32 j = 0; j < nWords; j++ )
        {
            // Invariant: c < 2^32
            c += SYMCRYPT_MUL32x32TO64( pMod[j], m );
            c += pSrc[j];
            // There is no overflow on C because the max value is
            // (2^32 - 1) * (2^32 - 1) + 2^32 - 1 + 2^32 - 1 = 2^64 - 1.
            pSrc[j] = (UINT32) c;
            c >>= 32;
        }
        c = c + pSrc[nWords] + hc;
        pSrc[nWords] = (UINT32) c;
        hc = c >> 32;
        pSrc++;
    }
    SYMCRYPT_ASSERT( hc < 2 );

    UINT32 d = SymCryptFdefRawSub( pSrc, pMod, pDst, nDigits );

    SYMCRYPT_ASSERT( hc <= d );     // if hc = 1, then d = 1 is mandatory

    SymCryptFdefMaskedCopy( (PCBYTE) pSrc, (PBYTE) pDst, nDigits, hc - (hc | d) );  // copy only if hc=0, d=1
}

VOID
SymCryptFdefMontgomeryReduce(
    _In_                                                                PCSYMCRYPT_MODULUS  pmMod,
    _Inout_updates_( 2 * pmMod->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32 ) PUINT32             pSrc,
    _Out_writes_( pmMod->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32 )        PUINT32             pDst )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_MULX ) )
    {
        SymCryptFdefMontgomeryReduceMulx( pmMod, pSrc, pDst );
    } else {
        SymCryptFdefMontgomeryReduceAsm( pmMod, pSrc, pDst );
    }
#elif SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM64 | SYMCRYPT_CPU_ARM
    SymCryptFdefMontgomeryReduceAsm( pmMod, pSrc, pDst );
#else
    SymCryptFdefMontgomeryReduceC( pmMod, pSrc, pDst );
#endif
}


VOID
SYMCRYPT_CALL
SymCryptFdefModSetPostMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PSYMCRYPT_MODELEMENT    peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    // Montgomery representation for X is R*X mod M where R = 2^<nDigits * bits-per-digit>
    // Montgomery reduction performs an implicit division by R
    // This function converts to the internal representation by multiplying by R^2 mod M and then performing a Montgomery reduction
    UINT32 nDigits = pmMod->nDigits;

	// dcl - this should not incur significant cost, consider checking always
    SYMCRYPT_ASSERT( cbScratch >= nDigits * 2 * SYMCRYPT_FDEF_DIGIT_SIZE );
    UNREFERENCED_PARAMETER( cbScratch );

    SymCryptFdefRawMul( &peObj->d.uint32[0], nDigits, pmMod->tm.montgomery.Rsqr, nDigits, (PUINT32) pbScratch );
    SymCryptFdefMontgomeryReduce( pmMod, (PUINT32) pbScratch, &peObj->d.uint32[0] );
}

PCUINT32
SYMCRYPT_CALL
SymCryptFdefModPreGetMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    PUINT32 pTmp = (PUINT32) pbScratch;
    UINT32 nDigits = pmMod->nDigits;

	// dcl - this should not incur significant cost, consider checking always
    SYMCRYPT_ASSERT( cbScratch >= nDigits * 2 * SYMCRYPT_FDEF_DIGIT_SIZE );
    UNREFERENCED_PARAMETER( cbScratch );

    memcpy( pTmp, &peObj->d.uint32[0], nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );
    SymCryptWipe( pTmp + nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32, nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );
    SymCryptFdefMontgomeryReduce( pmMod, pTmp, pTmp );

    return pTmp;
}

VOID
SYMCRYPT_CALL
SymCryptFdefModulusCopyFixupMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmSrc,
    _Out_                           PSYMCRYPT_MODULUS       pmDst )
{
    // We only have to fix up the Montgomery-specific stuff here
	// dcl - not sure I understand why you pass pmSrc here
    UNREFERENCED_PARAMETER( pmSrc );
    pmDst->tm.montgomery.Rsqr = (PUINT32)((PBYTE)&pmDst->Divisor + SymCryptFdefSizeofDivisorFromDigits( pmDst->nDigits ));
}

VOID
SYMCRYPT_CALL
SymCryptFdefModMulMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;

	// dcl - missing assert?
    UNREFERENCED_PARAMETER( cbScratch );
    SYMCRYPT_ASSERT( cbScratch >= 2 * nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );

    SymCryptFdefRawMul( &peSrc1->d.uint32[0], nDigits, &peSrc2->d.uint32[0], nDigits, pTmp );
    SymCryptFdefMontgomeryReduce( pmMod, pTmp, &peDst->d.uint32[0] );
}

#if SYMCRYPT_CPU_AMD64
VOID
SYMCRYPT_CALL
SymCryptFdefModMulMontgomeryMulx(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;

    UNREFERENCED_PARAMETER( cbScratch );
    SYMCRYPT_ASSERT( cbScratch >= 2 * nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );

    SymCryptFdefRawMulMulx( &peSrc1->d.uint32[0], nDigits, &peSrc2->d.uint32[0], nDigits, pTmp );
    SymCryptFdefMontgomeryReduceMulx( pmMod, pTmp, &peDst->d.uint32[0] );
}

VOID
SYMCRYPT_CALL
SymCryptFdefModMulMontgomeryMulx1024(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;

    UNREFERENCED_PARAMETER( cbScratch );
    SYMCRYPT_ASSERT( cbScratch >= 2 * nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );

    SymCryptFdefRawMulMulx1024( &peSrc1->d.uint32[0], &peSrc2->d.uint32[0], nDigits, pTmp );
    SymCryptFdefMontgomeryReduceMulx1024( pmMod, pTmp, &peDst->d.uint32[0] );
}
#endif


VOID
SYMCRYPT_CALL
SymCryptFdefModSquareMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;

    UNREFERENCED_PARAMETER( cbScratch );
    SYMCRYPT_ASSERT( cbScratch >= 2 * nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );

    SymCryptFdefRawSquare( &peSrc->d.uint32[0], nDigits, pTmp );
    SymCryptFdefMontgomeryReduce( pmMod, pTmp, &peDst->d.uint32[0] );
}


#if SYMCRYPT_CPU_AMD64
VOID
SYMCRYPT_CALL
SymCryptFdefModSquareMontgomeryMulx(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;

    UNREFERENCED_PARAMETER( cbScratch );
    SYMCRYPT_ASSERT( cbScratch >= 2 * nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );

    SymCryptFdefRawSquareMulx( &peSrc->d.uint32[0], nDigits, pTmp );
    SymCryptFdefMontgomeryReduceMulx( pmMod, pTmp, &peDst->d.uint32[0] );
}

VOID
SYMCRYPT_CALL
SymCryptFdefModSquareMontgomeryMulx1024(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;

    UNREFERENCED_PARAMETER( cbScratch );
    SYMCRYPT_ASSERT( cbScratch >= 2 * nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );

    SymCryptFdefRawSquareMulx1024( &peSrc->d.uint32[0], nDigits, pTmp );
    SymCryptFdefMontgomeryReduceMulx1024( pmMod, pTmp, &peDst->d.uint32[0] );
}
#endif

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefModInvMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 nDigits = pmMod->nDigits;
    UINT32 nBytes = nDigits * SYMCRYPT_FDEF_DIGIT_SIZE;
    PUINT32 pTmp = (PUINT32) pbScratch;

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pTmp );

    //
    // We have R*X; we first apply the montgomery reduction twice to get X/R, and then invert that
    // using the generic inversion to get R/X.
    //
	SYMCRYPT_ASSERT( cbScratch >= 2 * nBytes );
    memcpy( pTmp, &peSrc->d.uint32[0], nBytes );

    SymCryptWipe( (PBYTE)pTmp + nBytes, nBytes );
    SymCryptFdefMontgomeryReduce( pmMod, pTmp, pTmp );

    SymCryptWipe( (PBYTE)pTmp + nBytes, nBytes );
    SymCryptFdefMontgomeryReduce( pmMod, pTmp, &peDst->d.uint32[0] );

    scError = SymCryptFdefModInvGeneric( pmMod, peDst, peDst, flags, pbScratch, cbScratch );

    return scError;
}

#if SYMCRYPT_CPU_AMD64

//=====================================
// 256-bit Montgomery modulus code
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefModInvMontgomery256(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 nBytes = 32;
    PUINT32 pTmp = (PUINT32) pbScratch;

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pTmp );

    //
    // We have R*X; we first apply the montgomery reduction twice to get X/R, and then invert that
    // using the generic inversion to get R/X.
    //
    SYMCRYPT_ASSERT( cbScratch >= 2 * nBytes );
    memcpy( pTmp, &peSrc->d.uint32[0], nBytes );

    SymCryptWipe( (PBYTE)pTmp + nBytes, nBytes );
    SymCryptFdefMontgomeryReduce256Asm( pmMod, pTmp, pTmp );

    SymCryptWipe( (PBYTE)pTmp + nBytes, nBytes );
    SymCryptFdefMontgomeryReduce256Asm( pmMod, pTmp, &peDst->d.uint32[0] );

    scError = SymCryptFdefModInvGeneric( pmMod, peDst, peDst, flags, pbScratch, cbScratch );

    return scError;
}

VOID
SYMCRYPT_CALL
SymCryptFdefModSetPostMontgomeryMulx256(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PSYMCRYPT_MODELEMENT    peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    // Montgomery representation for X is R*X mod M where R = 2^<nDigits * bits-per-digit>
    // Montgomery reduction performs an implicit division by R
    // This function converts to the internal representation by multiplying by R^2 mod M and then performing a Montgomery reduction
    UINT32 nDigits = pmMod->nDigits;

    SYMCRYPT_ASSERT( cbScratch >= nDigits * 2 * SYMCRYPT_FDEF_DIGIT_SIZE );
    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );
    UNREFERENCED_PARAMETER( nDigits );

    SymCryptFdefModMulMontgomeryMulx256Asm( pmMod, (PSYMCRYPT_MODELEMENT) pmMod->tm.montgomery.Rsqr, peObj, peObj );
}

PCUINT32
SYMCRYPT_CALL
SymCryptFdefModPreGetMontgomery256(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    PUINT32 pTmp = (PUINT32) pbScratch;
    UINT32 nDigits = 1;

    SYMCRYPT_ASSERT( cbScratch >= nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );
    UNREFERENCED_PARAMETER( cbScratch );

    memcpy( pTmp, &peObj->d.uint32[0], nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );
    SymCryptFdefMontgomeryReduce256Asm( pmMod, pTmp, pTmp );

    // This gives the right result, but relies on peObj having zeroed upper half
    // on AMD64 when digits are 512 bits. This should be true - check in a CHKed build.
    for( UINT32 i=8; i<16; ++i )
    {
        SYMCRYPT_ASSERT( pTmp[i] == 0 );
    }

    // Wipe the extra bytes
    // SymCryptWipeKnownSize( pTmp + (SYMCRYPT_FDEF_DIGIT_NUINT32 / 2), 32 );

    return pTmp;
}

VOID
SYMCRYPT_CALL
SymCryptFdefModulusInitMontgomery256(
    _Inout_                         PSYMCRYPT_MODULUS       pmMod,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SymCryptFdefModulusInitMontgomeryInternal( pmMod, 8, pbScratch, cbScratch );
}

//=====================================
// 384-bit Montgomery modulus code
//

VOID
SYMCRYPT_CALL
SymCryptFdefModSetPostMontgomeryMulxP384(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PSYMCRYPT_MODELEMENT    peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    // Montgomery representation for X is R*X mod M where R = 2^<nDigits * bits-per-digit>
    // Montgomery reduction performs an implicit division by R
    // This function converts to the internal representation by multiplying by R^2 mod M and then performing a Montgomery reduction
    UINT32 nDigits = pmMod->nDigits;

    SYMCRYPT_ASSERT( cbScratch >= nDigits * 2 * SYMCRYPT_FDEF_DIGIT_SIZE );
    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );
    UNREFERENCED_PARAMETER( nDigits );

    SymCryptFdefModMulMontgomeryMulxP384Asm( pmMod, (PSYMCRYPT_MODELEMENT) pmMod->tm.montgomery.Rsqr, peObj, peObj );
}

#if 0
//=====================================
// 512-bit Montgomery modulus code
//

VOID
SYMCRYPT_CALL
SymCryptFdefModMulMontgomery512(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;

    SYMCRYPT_ASSERT( cbScratch >= nDigits * 2 * SYMCRYPT_FDEF_DIGIT_SIZE );
    UNREFERENCED_PARAMETER( cbScratch );

    SymCryptFdefRawMul512Asm( &peSrc1->d.uint32[0], &peSrc2->d.uint32[0], nDigits, pTmp );
    SymCryptFdefMontgomeryReduce512Asm( pmMod, pTmp, &peDst->d.uint32[0] );
}

VOID
SYMCRYPT_CALL
SymCryptFdefModSquareMontgomery512(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;

    SYMCRYPT_ASSERT( cbScratch >= nDigits * 2 * SYMCRYPT_FDEF_DIGIT_SIZE );
    UNREFERENCED_PARAMETER( cbScratch );

    SymCryptFdefRawSquare512Asm( &peSrc->d.uint32[0], nDigits, pTmp );
    SymCryptFdefMontgomeryReduce512Asm( pmMod, pTmp, &peDst->d.uint32[0] );
}

//=====================================
// 1024-bit Montgomery modulus code
//

VOID
SYMCRYPT_CALL
SymCryptFdefModMulMontgomery1024(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;

    SYMCRYPT_ASSERT( cbScratch >= nDigits * 2 * SYMCRYPT_FDEF_DIGIT_SIZE );
    UNREFERENCED_PARAMETER( cbScratch );

    SymCryptFdefRawMul1024Asm( &peSrc1->d.uint32[0], &peSrc2->d.uint32[0], nDigits, pTmp );
    SymCryptFdefMontgomeryReduce1024Asm( pmMod, pTmp, &peDst->d.uint32[0] );
}

VOID
SYMCRYPT_CALL
SymCryptFdefModSquareMontgomery1024(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;

    SYMCRYPT_ASSERT( cbScratch >= nDigits * 2 * SYMCRYPT_FDEF_DIGIT_SIZE );
    UNREFERENCED_PARAMETER( cbScratch );

    SymCryptFdefRawSquare1024Asm( &peSrc->d.uint32[0], nDigits, pTmp );
    SymCryptFdefMontgomeryReduce1024Asm( pmMod, pTmp, &peDst->d.uint32[0] );
}
#endif

#endif
