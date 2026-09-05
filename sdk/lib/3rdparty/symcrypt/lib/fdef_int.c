//
// fdef_int.c   INT functions for default number format
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// Default big-number format:
// INT objects are stored in two parts:
//  a SYMCRYPT_FDEF_INT structure
//  an array of UINT32; the # elements in the array is a multiple of SYMCRYPT_FDEF_DIGIT_SIZE/4.
//
// The pointer passed points to the start of the UINT32 array, just after the SYMCRYPT_FDEF_INT structure.
//
// The generic implementation accesses the digits as an array of UINT32, but on 64-bit CPUs
// the code can also view it as an array of UINT64.
//

UINT32
SYMCRYPT_CALL
SymCryptFdefRawAddC(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
                                                            UINT32      nDigits )
{
    UINT32 i;
    UINT64 t;

    t = 0;
    for( i=0; i<nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32; i++ )
    {
        t = t + pSrc1[i] + pSrc2[i];
        pDst[i] = (UINT32) t;
        t >>= 32;
    }

    return (UINT32) t;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawAdd(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
                                                            UINT32      nDigits )
{
#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM64 | SYMCRYPT_CPU_ARM
    return SymCryptFdefRawAddAsm( pSrc1, pSrc2, pDst, nDigits );
#else
    return SymCryptFdefRawAddC( pSrc1, pSrc2, pDst, nDigits );
#endif
}


UINT32
SYMCRYPT_CALL
SymCryptFdefRawAddUint32(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src1,
                                                            UINT32      Src2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     Dst,
                                                            UINT32      nDigits )
{
    UINT32 i;
    UINT64 t;

    t = Src2;
    for( i=0; i<nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32; i++ )
    {
        t = t + Src1[i];
        Dst[i] = (UINT32) t;
        t >>= 32;
    }

    return (UINT32) t;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntAddUint32(
    _In_    PCSYMCRYPT_INT  piSrc1,
            UINT32          u32Src2,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SYMCRYPT_CHECK_MAGIC( piSrc1 );
    SYMCRYPT_CHECK_MAGIC( piDst );

    SYMCRYPT_ASSERT( piSrc1->nDigits == piDst->nDigits );

    return SymCryptFdefRawAddUint32( SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ), u32Src2, SYMCRYPT_FDEF_INT_PUINT32( piDst ), piDst->nDigits );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntAddSameSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst )
{
    SYMCRYPT_ASSERT( piSrc1->nDigits == piSrc2->nDigits && piSrc2->nDigits == piDst->nDigits );

    return SymCryptFdefRawAdd(  SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ),
                                SYMCRYPT_FDEF_INT_PUINT32( piSrc2 ),
                                SYMCRYPT_FDEF_INT_PUINT32( piDst ),
                                piDst->nDigits );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntAddMixedSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst )
{
    UINT32      nS1 = piSrc1->nDigits;
    UINT32      nS2 = piSrc2->nDigits;
    UINT32      nD  = piDst->nDigits;
    UINT32      c;
    UINT32      nW;

    SYMCRYPT_ASSERT( nD >= nS1 && nD >= nS2 );

    if( nS1 < nS2 )
    {
        c = SymCryptFdefRawAdd( SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ), SYMCRYPT_FDEF_INT_PUINT32( piSrc2 ), SYMCRYPT_FDEF_INT_PUINT32( piDst ), nS1 );
        c = SymCryptFdefRawAddUint32( &SYMCRYPT_FDEF_INT_PUINT32( piSrc2 )[nS1 * SYMCRYPT_FDEF_DIGIT_NUINT32], c, &SYMCRYPT_FDEF_INT_PUINT32( piDst )[nS1 * SYMCRYPT_FDEF_DIGIT_NUINT32], nS2 - nS1 );
        nW = nS2;
    } else {
        // nS2 < nS1
        c = SymCryptFdefRawAdd( SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ), SYMCRYPT_FDEF_INT_PUINT32( piSrc2 ), SYMCRYPT_FDEF_INT_PUINT32( piDst ), nS2 );
        c = SymCryptFdefRawAddUint32( &SYMCRYPT_FDEF_INT_PUINT32( piSrc1 )[nS2 * SYMCRYPT_FDEF_DIGIT_NUINT32], c, &SYMCRYPT_FDEF_INT_PUINT32( piDst )[nS2 * SYMCRYPT_FDEF_DIGIT_NUINT32], nS1 - nS2 );
        nW = nS1;
    }

    if( nW < nD )
    {
        SymCryptWipe( &SYMCRYPT_FDEF_INT_PUINT32( piDst )[nW * SYMCRYPT_FDEF_DIGIT_NUINT32], (nD - nW) * SYMCRYPT_FDEF_DIGIT_SIZE );
        SYMCRYPT_FDEF_INT_PUINT32( piDst )[nW * SYMCRYPT_FDEF_DIGIT_NUINT32] = c;
        c = 0;
    }

    return c;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawSubC(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
                                                            UINT32      nDigits )
{
    UINT32 i;
    UINT64 t;
    UINT32 c;

    c = 0;
    for( i=0; i<nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32; i++ )
    {
        // c == 1 for carry, 0 for no carry
        t = (UINT64) pSrc1[i] - pSrc2[i] - c;
        pDst[i] = (UINT32) t;
        c = (UINT32)(t >> 32) & 1;
    }

    return c;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawSub(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
                                                            UINT32      nDigits )
{
#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM64 | SYMCRYPT_CPU_ARM
    return SymCryptFdefRawSubAsm( pSrc1, pSrc2, pDst, nDigits );
#else
    return SymCryptFdefRawSubC( pSrc1, pSrc2, pDst, nDigits );
#endif
}


UINT32
SYMCRYPT_CALL
SymCryptFdefRawSubUint32(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
                                                            UINT32      Src2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
                                                            UINT32      nDigits )
{
    UINT32 i;
    UINT64 t;
    UINT32 c;

    c = Src2;
    for( i=0; i<nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32; i++ )
    {
        t = (UINT64)pSrc1[i] - c;
        pDst[i] = (UINT32) t;
        c = (UINT32)(t >> 32) & 1;
    }

    return c;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawNeg(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
                                                            UINT32      carryIn,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
                                                            UINT32      nDigits )
{
    UINT32 i;
    UINT64 t;
    UINT32 c;

    c = carryIn;
    for( i=0; i<nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32; i++ )
    {
        t = (UINT64)0 - pSrc1[i] - c;
        pDst[i] = (UINT32) t;
        c = (UINT32)(t >> 32) & 1;
    }

    return c;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntSubUint32(
    _In_    PCSYMCRYPT_INT  piSrc1,
            UINT32          u32Src2,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SYMCRYPT_ASSERT( piSrc1->nDigits == piDst->nDigits );

    return SymCryptFdefRawSubUint32( SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ), u32Src2, SYMCRYPT_FDEF_INT_PUINT32( piDst ), piDst->nDigits );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntSubSameSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst )
{
    SYMCRYPT_ASSERT( piSrc1->nDigits == piSrc2->nDigits && piSrc1->nDigits == piDst->nDigits );

    return SymCryptFdefRawSub( SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ), SYMCRYPT_FDEF_INT_PUINT32( piSrc2 ), SYMCRYPT_FDEF_INT_PUINT32( piDst ), piDst->nDigits );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntSubMixedSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst )
{
    UINT32      nS1 = piSrc1->nDigits;
    UINT32      nS2 = piSrc2->nDigits;
    UINT32      nD  = piDst->nDigits;
    UINT32      c;
    UINT32      n;

    SYMCRYPT_ASSERT( nD >= nS1 && nD >= nS2 );

    if( nS1 < nS2 )
    {
        c = SymCryptFdefRawSub( SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ), SYMCRYPT_FDEF_INT_PUINT32( piSrc2 ), SYMCRYPT_FDEF_INT_PUINT32( piDst ), nS1 );
        c = SymCryptFdefRawNeg( &SYMCRYPT_FDEF_INT_PUINT32( piSrc2 )[nS1 * SYMCRYPT_FDEF_DIGIT_NUINT32], c, &SYMCRYPT_FDEF_INT_PUINT32( piDst )[nS1 * SYMCRYPT_FDEF_DIGIT_NUINT32], nS2 - nS1 );
        n = nS2 * SYMCRYPT_FDEF_DIGIT_NUINT32;
    } else {
        // nS2 < nS1
        c = SymCryptFdefRawSub( SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ), SYMCRYPT_FDEF_INT_PUINT32( piSrc2 ), SYMCRYPT_FDEF_INT_PUINT32( piDst ), nS2 );
        c = SymCryptFdefRawSubUint32( &SYMCRYPT_FDEF_INT_PUINT32( piSrc1 )[nS2 * SYMCRYPT_FDEF_DIGIT_NUINT32], c, &SYMCRYPT_FDEF_INT_PUINT32( piDst )[nS2 * SYMCRYPT_FDEF_DIGIT_NUINT32], nS1 - nS2 );
        n = nS1 * SYMCRYPT_FDEF_DIGIT_NUINT32;
    }

    //
    // Set the rest of the result to 0s or 1s
    //
    while( n < nD * SYMCRYPT_FDEF_DIGIT_NUINT32 )
    {
        SYMCRYPT_FDEF_INT_PUINT32( piDst )[n++] = 0 - c;
    }

    return c;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawIsLessThanC(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
                                                            UINT32      nDigits )
{
    UINT32 i;
    UINT64 t;
    UINT32 c;

    // We just do a subtraction without writing and return the carry
    c = 0;
    for( i=0; i<nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32; i++ )
    {
        // c == 1 for carry, 0 for no carry
        t = (UINT64) pSrc1[i] - pSrc2[i] - c;
        c = (UINT32)(t >> 32) & 1;
    }

    // All booleans are returned as masks
    return 0 - c;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawIsLessThan(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
                                                            UINT32      nDigits )
{
#if 0 & SYMCRYPT_CPU_AMD64
//    return SymCryptFdefRawIsLessThanAsm( pSrc1, pSrc2, nDigits );
#else
    return SymCryptFdefRawIsLessThanC( pSrc1, pSrc2, nDigits );
#endif
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawIsZeroC(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
                                                            UINT32      nDigits )
{
    UINT32 i;
    UINT32 c;

    c = 0;
    for( i=0; i<nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32; i++ )
    {
        c |= pSrc1[i];
    }

    // All booleans are returned as masks
    return SYMCRYPT_MASK32_ZERO( c );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawIsZero(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
                                                            UINT32      nDigits )
{
#if 0 & SYMCRYPT_CPU_AMD64
//    return SymCryptFdefRawIsZeroAsm( pSrc1, nDigits );
#else
    return SymCryptFdefRawIsZeroC( pSrc1, nDigits );
#endif
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntIsLessThan(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    PCSYMCRYPT_INT  piSrc2 )
{
    UINT32  nD1 = piSrc1->nDigits;
    UINT32  nD2 = piSrc2->nDigits;

    UINT32 res;

    if( nD1 == nD2 )
    {
        res = SymCryptFdefRawIsLessThan( SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ), SYMCRYPT_FDEF_INT_PUINT32( piSrc2 ), nD1 );
    } else if( nD1 < nD2 ) {
        res =  SymCryptFdefRawIsLessThan( SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ), SYMCRYPT_FDEF_INT_PUINT32( piSrc2 ), nD1 );
        res |= ~SymCryptFdefRawIsZero( &SYMCRYPT_FDEF_INT_PUINT32( piSrc2 )[ nD1 * SYMCRYPT_FDEF_DIGIT_NUINT32 ], nD2 - nD1 );
    } else {
        res =  SymCryptFdefRawIsLessThan( SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ), SYMCRYPT_FDEF_INT_PUINT32( piSrc2 ), nD2 );
        res &= SymCryptFdefRawIsZero( &SYMCRYPT_FDEF_INT_PUINT32( piSrc1 )[ nD2 * SYMCRYPT_FDEF_DIGIT_NUINT32 ], nD1 - nD2 );
    }

    return res;
}


VOID
SYMCRYPT_CALL
SymCryptFdefIntNeg(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst )
{
    UINT32 nDigits = piDst->nDigits;
    SYMCRYPT_ASSERT( piSrc->nDigits == nDigits );

    SymCryptFdefRawNeg( SYMCRYPT_FDEF_INT_PUINT32( piSrc ), 0, SYMCRYPT_FDEF_INT_PUINT32( piDst ), nDigits );
}


VOID
SYMCRYPT_CALL
SymCryptFdefIntMulPow2(
    _In_    PCSYMCRYPT_INT  piSrc,
            SIZE_T          Exp,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SYMCRYPT_ASSERT( piSrc->nDigits == piDst->nDigits );

    SIZE_T  shiftWords = Exp / (8 * sizeof( UINT32 ) );
    SIZE_T  shiftBits  = Exp % (8 * sizeof( UINT32 ) );

    UINT32  nWords = piDst->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32;

    if( shiftWords >= nWords )
    {
        SymCryptWipe( SYMCRYPT_FDEF_INT_PUINT32( piDst ), nWords * sizeof( UINT32 ) );
        goto cleanup;
    }

    SIZE_T i = nWords;
    while( i > shiftWords )
    {
        i--;
        UINT64 t = (UINT64)SYMCRYPT_FDEF_INT_PUINT32( piSrc )[i - shiftWords] << 32;
        if( i > shiftWords )
        {
            t |= SYMCRYPT_FDEF_INT_PUINT32( piSrc )[i - shiftWords - 1];
        }
        SYMCRYPT_FDEF_INT_PUINT32( piDst )[i] = (UINT32)(t >> (32 - shiftBits));
    }

    while( i > 0 )
    {
        i--;
        SYMCRYPT_FDEF_INT_PUINT32( piDst )[i] = 0;
    }

cleanup:
    ;
}

// In shift-based operations which we have no assembly for, and we'd like to use 32-bit words
// on 32-bit architectures and 64-bit words on 64-bit architectures. So we use NATIVE_UINT &
// friends.

// Note that accessing the FDEF uint32 array as an array of NATIVE_UINTs relies on
// the little-endianness of the target if NATIVE_UINT is larger than 32 bits.
// AMD64 is little endian and ARM64 code is always expected to execute in little
// endian mode, but this is not true in general for an arbitrary 64 bit platform.
//
// If we need to support a 64 bit big endian platform, we need to either
// restrict its NATIVE_UINT to 32 bits, or introduce load and store macros.
#define SYMCRYPT_FDEF_INT_PNATIVE_UINT(p) ((NATIVE_UINT*) SYMCRYPT_FDEF_INT_PUINT32( p ))
// Ensure that sizeof(NATIVE_UINT) > 4 only when compiling for known little endian target
C_ASSERT( (NATIVE_BYTES <= 4) || SYMCRYPT_CPU_AMD64 || SYMCRYPT_CPU_ARM64 );

#define SYMCRYPT_FDEF_DIGIT_NNATIVE_UINT  ((NATIVE_UINT)(SYMCRYPT_FDEF_DIGIT_SIZE / NATIVE_BYTES))

// Ensure that digit is divisible by native word size!
C_ASSERT(SYMCRYPT_FDEF_DIGIT_NNATIVE_UINT * NATIVE_BYTES == SYMCRYPT_FDEF_DIGIT_SIZE);

VOID
SYMCRYPT_CALL
SymCryptFdefIntDivPow2(
    _In_    PCSYMCRYPT_INT  piSrc,
            SIZE_T          exp,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SIZE_T  shiftWords = exp / NATIVE_BITS;
    SIZE_T  shiftRightBits  = exp % NATIVE_BITS;
    SIZE_T  shiftLeftBits   = (NATIVE_BITS-1) - shiftRightBits;
    NATIVE_UINT lowWord, highWord, highPart;
    SIZE_T i = 0;

    NATIVE_UINT nWords = piDst->nDigits * SYMCRYPT_FDEF_DIGIT_NNATIVE_UINT;

    SYMCRYPT_ASSERT( piSrc->nDigits == piDst->nDigits );

    shiftWords = SYMCRYPT_MIN(shiftWords, nWords);
    if( shiftWords < nWords )
    {
        lowWord = SYMCRYPT_FDEF_INT_PNATIVE_UINT(piSrc)[shiftWords];
        while( i+shiftWords+1 < nWords )
        {
            highWord = SYMCRYPT_FDEF_INT_PNATIVE_UINT(piSrc)[i+shiftWords+1];

            // We always shift highWord left by 1 to keep variable shiftLeftBits in range [0,NATIVE_BITS-1]
            highPart = (highWord << shiftLeftBits)<<1;

            SYMCRYPT_FDEF_INT_PNATIVE_UINT(piDst)[i] = (lowWord >> shiftRightBits) | highPart;

            lowWord = highWord;
            i++;
        }
        SYMCRYPT_FDEF_INT_PNATIVE_UINT(piDst)[i] = (lowWord >> shiftRightBits);
        i++;
    }

    SYMCRYPT_ASSERT(i + shiftWords == nWords);

    SymCryptWipe( &SYMCRYPT_FDEF_INT_PNATIVE_UINT( piDst )[nWords-shiftWords], shiftWords * NATIVE_BYTES );
}

VOID
SYMCRYPT_CALL
SymCryptFdefIntShr1(
            UINT32          highestBit,
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst )
{
    UINT32  nWords = piDst->nDigits * SYMCRYPT_FDEF_DIGIT_NNATIVE_UINT;

    SYMCRYPT_ASSERT( piSrc->nDigits == piDst->nDigits );
    SYMCRYPT_ASSERT( highestBit < 2 );

    SIZE_T i = 0;
    NATIVE_UINT lowWord = SYMCRYPT_FDEF_INT_PNATIVE_UINT(piSrc)[0];
    NATIVE_UINT highWord = 0;
    while( i+1 < nWords )
    {
        highWord = SYMCRYPT_FDEF_INT_PNATIVE_UINT(piSrc)[i+1];

        SYMCRYPT_FDEF_INT_PNATIVE_UINT(piDst)[i] = (lowWord >> 1) | (highWord << (NATIVE_BITS - 1));

        lowWord = highWord;
        i++;
    }

    SYMCRYPT_FDEF_INT_PNATIVE_UINT(piDst)[i] = (lowWord >> 1) | ((NATIVE_UINT)highestBit) << (NATIVE_BITS - 1);
}

VOID
SYMCRYPT_CALL
SymCryptFdefIntModPow2(
    _In_    PCSYMCRYPT_INT  piSrc,
            SIZE_T          exp,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SIZE_T  expWords = exp / 32;        // index of word with the partial mask
    SIZE_T  expBits  = exp % 32;        // # bits to leave in that word

    UINT32  nWords = piDst->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32;

    SYMCRYPT_ASSERT( piSrc->nDigits == piDst->nDigits );

    if( piSrc != piDst )
    {
        memcpy( SYMCRYPT_FDEF_INT_PUINT32( piDst ), SYMCRYPT_FDEF_INT_PUINT32( piSrc ), nWords * sizeof( UINT32 ) );
    }

    if( expWords >= nWords )
    {
        // exp is so large that Dst = Src is sufficient.
        goto cleanup;
    }

    for( SIZE_T i=expWords + 1; i < nWords; i++ )
    {
        SYMCRYPT_FDEF_INT_PUINT32( piDst )[i] = 0;
    }

    if( expBits != 0 )
    {
        SYMCRYPT_FDEF_INT_PUINT32( piDst )[expWords] &= ((UINT32) -1) >> (32 - expBits );
    } else {
        SYMCRYPT_FDEF_INT_PUINT32( piDst )[expWords] = 0;
    }

cleanup:
    ;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntGetBit(
    _In_    PCSYMCRYPT_INT  piSrc,
            UINT32          iBit )
{
    SYMCRYPT_ASSERT( iBit < piSrc->nDigits * SYMCRYPT_FDEF_DIGIT_BITS );

    return (((SYMCRYPT_FDEF_INT_PUINT32( piSrc)[iBit / 32]) >> (iBit % 32)) & 1);
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntGetBits(
    _In_    PCSYMCRYPT_INT  piSrc,
            UINT32          iBit,
            UINT32          nBits )
{
    UINT32 mainMask = 0;
    UINT32 result = 0;

    SYMCRYPT_ASSERT( (nBits > 0) &&
                     (nBits < 33) &&
                     (iBit < piSrc->nDigits * SYMCRYPT_FDEF_DIGIT_BITS) &&
                     (iBit + nBits <= piSrc->nDigits * SYMCRYPT_FDEF_DIGIT_BITS) );

    mainMask = (UINT32)(-1) >> (32-nBits);

    // Get the lower word first (it exists since iBit is smaller than the max bit)
    result = SYMCRYPT_FDEF_INT_PUINT32(piSrc)[iBit/32];

    // Shift to the right accordingly
    result >>= (iBit%32);

    // Get the upper word (if we need it)
    // Note: the iBit and nBits values are public
    if ((iBit%32!=0) && ( iBit/32 + 1 < piSrc->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32 ))
    {
        result |= ( SYMCRYPT_FDEF_INT_PUINT32(piSrc)[iBit/32+1] << (32 - iBit%32) );
    }

    // Mask out the top bits
    result &= mainMask;

    return result;
}

VOID
SYMCRYPT_CALL
SymCryptFdefIntSetBits(
    _In_    PSYMCRYPT_INT   piDst,
            UINT32          value,
            UINT32          iBit,
            UINT32          nBits )
{
    UINT32 mainMask = 0;

    UINT32 alignedVal = 0;
    UINT32 alignedMask = 0;

    SYMCRYPT_ASSERT( (nBits > 0) &&
                     (nBits < 33) &&
                     (iBit < piDst->nDigits * SYMCRYPT_FDEF_DIGIT_BITS) &&
                     (iBit + nBits <= piDst->nDigits * SYMCRYPT_FDEF_DIGIT_BITS) );

    // Zero out the not needed bits of the value
    mainMask = (UINT32)(-1) >> (32-nBits);
    value &= mainMask;

    //
    // Lower word
    //

    // Create the needed mask
    alignedMask = mainMask << (iBit%32);

    // Align the value
    alignedVal = value << (iBit%32);

    // Set the lower word first (it exists since iBit is smaller than the max bit)
    SYMCRYPT_FDEF_INT_PUINT32(piDst)[iBit/32] = (SYMCRYPT_FDEF_INT_PUINT32(piDst)[iBit/32] & ~alignedMask) | alignedVal;

    //
    // Upper word
    //

    if ((iBit%32!=0) && ( iBit/32 + 1 < piDst->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32 ))
    {
        // Create the needed mask
        alignedMask = mainMask >> (32 - iBit%32);

        // Align the value
        alignedVal = value >> (32 - iBit%32);

        // Set the upper word
        SYMCRYPT_FDEF_INT_PUINT32(piDst)[iBit/32 + 1] = (SYMCRYPT_FDEF_INT_PUINT32(piDst)[iBit/32 + 1] & ~alignedMask) | alignedVal;
    }

}


UINT32
SYMCRYPT_CALL
SymCryptFdefIntMulUint32(
    _In_                            PCSYMCRYPT_INT  piSrc1,
                                    UINT32          Src2,
    _Out_                           PSYMCRYPT_INT   piDst )
{
    UINT32  nWords = piDst->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32;

    SYMCRYPT_ASSERT( piSrc1->nDigits == piDst->nDigits );

    UINT64 c = 0;
    for( UINT32 i=0; i<nWords; i++ )
    {
        c += SYMCRYPT_MUL32x32TO64( SYMCRYPT_FDEF_INT_PUINT32( piSrc1 )[i], Src2 );
        SYMCRYPT_FDEF_INT_PUINT32( piDst )[i] = (UINT32) c;
        c >>= 32;
    }

    return (UINT32) c;
}


VOID
SYMCRYPT_CALL
SymCryptFdefIntMulSameSize(
    _In_                            PCSYMCRYPT_INT  piSrc1,
    _In_                            PCSYMCRYPT_INT  piSrc2,
    _Out_                           PSYMCRYPT_INT   piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch )
{
    SymCryptFdefIntMulMixedSize( piSrc1, piSrc2, piDst, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptFdefIntSquare(
    _In_                            PCSYMCRYPT_INT  piSrc,
    _Out_                           PSYMCRYPT_INT   piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch )
{
    UINT32      nS = piSrc->nDigits;
    UINT32      nD = piDst->nDigits;

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_MUL( piDst->nDigits ) );

    SYMCRYPT_ASSERT( 2*nS <= nD );

    SymCryptFdefRawSquare( SYMCRYPT_FDEF_INT_PUINT32( piSrc ), nS, SYMCRYPT_FDEF_INT_PUINT32( piDst ) );

    if( 2*nS < nD )
    {
        SymCryptWipe( &SYMCRYPT_FDEF_INT_PUINT32( piDst )[2 * nS * SYMCRYPT_FDEF_DIGIT_NUINT32], (nD - 2*nS) * SYMCRYPT_FDEF_DIGIT_SIZE );
    }
}


VOID
SYMCRYPT_CALL
SymCryptFdefRawMulC(
    _In_reads_(nDigits1 * SYMCRYPT_FDEF_DIGIT_NUINT32)              PCUINT32    pSrc1,
                                                                    UINT32      nDigits1,
    _In_reads_(nDigits2 * SYMCRYPT_FDEF_DIGIT_NUINT32)              PCUINT32    pSrc2,
                                                                    UINT32      nDigits2,
    _Out_writes_((nDigits1+nDigits2)*SYMCRYPT_FDEF_DIGIT_NUINT32)   PUINT32     pDst )
{
    UINT32 nWords1 = nDigits1 * SYMCRYPT_FDEF_DIGIT_NUINT32;
    UINT32 nWords2 = nDigits2 * SYMCRYPT_FDEF_DIGIT_NUINT32;

    // Set Dst to zero
    SymCryptWipe( pDst, (nDigits1+nDigits2) * SYMCRYPT_FDEF_DIGIT_SIZE );

    for( UINT32 i = 0; i < nWords1; i++ )
    {
        UINT32 m = pSrc1[i];
        UINT64 c = 0;
        for( UINT32 j = 0; j < nWords2; j++ )
        {
            // Invariant: c < 2^32
            c += SYMCRYPT_MUL32x32TO64( pSrc2[j], m );
            c += pDst[i+j];
            // There is no overflow on C because the max value is
            // (2^32 - 1) * (2^32 - 1) + 2^32 - 1 + 2^32 - 1 = 2^64 - 1.
            pDst[i+j] = (UINT32) c;
            c >>= 32;
        }
        pDst[i + nWords2] = (UINT32) c;
    }
}

VOID
SYMCRYPT_CALL
SymCryptFdefRawMul(
    _In_reads_(nDigits1*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc1,
                                                                    UINT32      nDigits1,
    _In_reads_(nDigits2*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc2,
                                                                    UINT32      nDigits2,
    _Out_writes_((nDigits1+nDigits2)*SYMCRYPT_FDEF_DIGIT_NUINT32)   PUINT32     pDst )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_MULX ) )
    {
        SymCryptFdefRawMulMulx( pSrc1, nDigits1, pSrc2, nDigits2, pDst );
    } else {
        SymCryptFdefRawMulAsm( pSrc1, nDigits1, pSrc2, nDigits2, pDst );
    }
#elif SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM64 | SYMCRYPT_CPU_ARM
    SymCryptFdefRawMulAsm( pSrc1, nDigits1, pSrc2, nDigits2, pDst );
#else
    SymCryptFdefRawMulC( pSrc1, nDigits1, pSrc2, nDigits2, pDst );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptFdefRawSquareC(
    _In_reads_(nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32)   PCUINT32    pSrc,
                                                        UINT32      nDigits,
    _Out_writes_(2*nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32) PUINT32     pDst )
{
    UINT32 nWords = nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32;

    UINT32 m = 0;
    UINT64 c = 0;

    // Set Dst to zero
    SymCryptWipe( pDst, (2*nDigits) * SYMCRYPT_FDEF_DIGIT_SIZE );

    // First Pass - Addition of the cross products x_i*x_j with i!=j
    for( UINT32 i = 0; i < nWords; i++ )
    {
        m = pSrc[i];
        c = 0;
        for( UINT32 j = i+1; j < nWords; j++ )
        {
            // Invariant: c < 2^32
            c += SYMCRYPT_MUL32x32TO64( pSrc[j], m );
            c += pDst[i+j];
            // There is no overflow on C because the max value is
            // (2^32 - 1) * (2^32 - 1) + 2^32 - 1 + 2^32 - 1 = 2^64 - 1.
            pDst[i+j] = (UINT32) c;
            c >>= 32;
        }
        pDst[i + nWords] = (UINT32) c;
    }

    // Second Pass - Shifting all results 1 bit left
    c = 0;
    for( UINT32 i = 1; i < 2*nWords; i++ )
    {
        c |= (((UINT64)pDst[i])<<1);
        pDst[i] = (UINT32)c;
        c >>= 32;
    }

    // Third Pass - Adding the squares on the even columns and propagating the sum
    c = 0;
    for( UINT32 i = 0; i < nWords; i++ )
    {
        //
        // Even column
        //
        m = pSrc[i];
        c += SYMCRYPT_MUL32x32TO64( m, m );
        c += pDst[2*i];
        // There is no overflow on C because the max value is
        // (2^32 - 1) * (2^32 - 1) + 2^32 - 1 + 2^32 - 1 = 2^64 - 1

        pDst[2*i] = (UINT32) c;
        c >>= 32;

        //
        // Odd column
        //
        c += pDst[2*i+1];
        // There is no overflow on C because the max value is
        // 2^32 - 1 + 2^32 - 1 = 2^33 - 2

        pDst[2*i+1] = (UINT32) c;
        c >>= 32;
    }
}

VOID
SYMCRYPT_CALL
SymCryptFdefRawSquare(
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)         PCUINT32    pSrc,
                                                            UINT32      nDigits,
    _Out_writes_(2*nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PUINT32     pDst )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_MULX ) )
    {
        SymCryptFdefRawSquareMulx( pSrc, nDigits, pDst );
    } else {
        SymCryptFdefRawSquareAsm( pSrc, nDigits, pDst );
    }
#elif SYMCRYPT_CPU_ARM64 | SYMCRYPT_CPU_ARM
    SymCryptFdefRawSquareAsm( pSrc, nDigits, pDst );
#elif SYMCRYPT_CPU_X86
    SymCryptFdefRawMulAsm( pSrc, nDigits, pSrc, nDigits, pDst );
#else
    SymCryptFdefRawSquareC( pSrc, nDigits, pDst );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptFdefIntMulMixedSize(
    _In_                            PCSYMCRYPT_INT  piSrc1,
    _In_                            PCSYMCRYPT_INT  piSrc2,
    _Out_                           PSYMCRYPT_INT   piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch )
{
    UINT32      nS1 = piSrc1->nDigits;
    UINT32      nS2 = piSrc2->nDigits;
    UINT32      nD  = piDst ->nDigits;

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_MUL( piDst->nDigits ) );

    SYMCRYPT_ASSERT( nS1 + nS2 <= nD );

    SymCryptFdefRawMul( SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ), nS1, SYMCRYPT_FDEF_INT_PUINT32( piSrc2 ), nS2, SYMCRYPT_FDEF_INT_PUINT32( piDst ) );

    if( nS1 + nS2 < nD )
    {
        SymCryptWipe( &SYMCRYPT_FDEF_INT_PUINT32( piDst )[(nS1 + nS2) * SYMCRYPT_FDEF_DIGIT_NUINT32], (nD - (nS1 + nS2)) * SYMCRYPT_FDEF_DIGIT_SIZE );
    }
}


PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptFdefIntFromDivisor( _In_ PSYMCRYPT_DIVISOR pdSrc )
{
    return &pdSrc->Int;
}

VOID
SYMCRYPT_CALL
SymCryptFdefIntToDivisor(
    _In_                            PCSYMCRYPT_INT      piSrc,
    _Out_                           PSYMCRYPT_DIVISOR   pdDst,
                                    UINT32              totalOperations,
                                    UINT32              flags,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch )
{
    UINT32      W;
    UINT32      nBits;
    UINT32      nWords;
    UINT32      bitToTest;
    UINT64      P;

    UNREFERENCED_PARAMETER( totalOperations );
    UNREFERENCED_PARAMETER( flags );

    SYMCRYPT_CHECK_MAGIC( piSrc );
    SYMCRYPT_CHECK_MAGIC( pdDst );

    SYMCRYPT_ASSERT( piSrc->nDigits == pdDst->nDigits );

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_TO_DIVISOR( piSrc->nDigits ) );

    //
    // Copy the Int.
    //
    SymCryptFdefIntCopy( piSrc, &pdDst->Int );

    //
    // For an N-bit divisor M, and D-bit divisor digit size,
    // the value W is defined as
    //     floor( (2^{N+D} - 1) / M } - 2^D
    // which is the largest W such that (W * M + 2^D * M )< 2^{N+D}
    // To compute W we use a binary search.
    // This can be optimized, but this is the simplest side-channel safe solution.
    // We can compute the upper bits of W * M + 2^D * M in a simple loop.
    //
    // For now we only compute a 32-bit W for a 32-bit digit divisor size.
    //

    nBits = SymCryptIntBitsizeOfValue( &pdDst->Int );

    SYMCRYPT_ASSERT( nBits != 0 );
    if( nBits == 0 )
    {
        // Can't create a divisor from a Int whose value is 0

        // We really should not have any callers which get here (it is a requirement that Src != 0)
        // We assert in CHKed builds
        // In release set the divisor to 1 instead
        SymCryptIntSetValueUint32( 1, &pdDst->Int );
    }

    pdDst->nBits = nBits;

    nWords = (nBits + 31)/32;
    bitToTest = (UINT32)1 << 31;
    W = 0;
    while( bitToTest > 0 )
    {
        W |= bitToTest;
        // Do the multiplication
        P = 0;
        for( UINT32 i=0; i<nWords; i++ )
        {
            // Invariant:
            // P <= 2^{2D} - 2 which ensures the mul-add doesn't generate an overflow
            // P  = floor( (W + 2^32)*M[0..i-1] / 2^{32*i} )
            P += SYMCRYPT_MUL32x32TO64( W, SYMCRYPT_FDEF_INT_PUINT32( &pdDst->Int )[i] );
            P >>= 32;
            P += SYMCRYPT_FDEF_INT_PUINT32( &pdDst->Int )[i];
        }
        // We are interested in bit N+D, and P[0] is bit nWords*D, this shift brings the relevant bit to position 0
        P >>= ((nBits+31) % 32) + 1;
        // If the bit is 1, W*M is too large and we reset the corresponding bit in W.
        W ^= bitToTest & (0 - ((UINT32)P & 1));
        bitToTest >>= 1;
    }
    pdDst->td.fdef.W = W;

    SYMCRYPT_SET_MAGIC( pdDst );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawMultSubUint32(
    _Inout_updates_( nUint32 + 1 )  PUINT32     pAcc,
    _In_reads_( nUint32 )           PCUINT32    pSrc1,
                                    UINT32      Src2,
                                    UINT32      nUint32 )
{
    //
    // pAcc -= pSrc1 * Src2
    // BEWARE: this is only used by the DivMod routine, and works in Words rather than Digits
    // making optimizations hard.
    //

    UINT32 i;
    UINT64 tmul;
    UINT64 tsub;
    UINT32 c;

    tmul = 0;
    c = 0;
    for( i=0; i<nUint32; i++ )
    {
        tmul += SYMCRYPT_MUL32x32TO64( pSrc1[i], Src2 );
        tsub  = (UINT64)pAcc[i] - (UINT32) tmul - c;
        pAcc[i] = (UINT32) tsub;
        c = (tsub >> 32) & 1;
        tmul >>= 32;
    }

    // Writing the last word is strictly speaking not necessary, but a really good check that things are going right.
    // We can remove the write, but still need the computation of c so it gains very little.

    tsub = (UINT64) pAcc[i] - (UINT32) tmul - c;
    pAcc[i] = (UINT32) tsub;
    c = (tsub >> 32) & 1;

    return c;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawMaskedAddSubdigit(
    _Inout_updates_( nUint32 )  PUINT32     pAcc,
    _In_reads_( nUint32 )       PCUINT32    pSrc,
                                UINT32      mask,
                                UINT32      nUint32 )
{
    UINT32 i;
    UINT64 t;

    t = 0;
    for( i=0; i<nUint32; i++ )
    {
        t = t + pAcc[i] + (mask & pSrc[i]);
        pAcc[i] = (UINT32) t;
        t >>= 32;
    }

    return (UINT32) t;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawMaskedAdd(
    _Inout_updates_( nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32 )  PUINT32     pAcc,
    _In_reads_( nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32 )       PCUINT32    pSrc,
                                                            UINT32      mask,
                                                            UINT32      nDigits )
{
    return SymCryptFdefRawMaskedAddSubdigit( pAcc, pSrc, mask, nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32 );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawMaskedSub(
    _Inout_updates_( nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32 )  PUINT32     pAcc,
    _In_reads_( nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32 )       PCUINT32    pSrc,
                                                            UINT32      mask,
                                                            UINT32      nDigits )
{
    UINT32 i;
    UINT64 t;
    UINT32 c;

    c = 0;
    for( i=0; i<nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32; i++ )
    {
        t = (UINT64) pAcc[i] - (mask & pSrc[i]) - c;
        pAcc[i] = (UINT32) t;
        c = (UINT32)(t >>= 32) & 1;
    }

    return c;
}



VOID
SYMCRYPT_CALL
SymCryptFdefRawDivMod(
    _In_reads_(nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32)           PCUINT32                pNum,
                                                                UINT32                  nDigits,
    _In_                                                        PCSYMCRYPT_DIVISOR      pdDivisor,
    _Out_writes_opt_(nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32)     PUINT32                 pQuotient,
    _Out_writes_opt_(SYMCRYPT_OBJ_NUINT32(pdDivisor))           PUINT32                 pRemainder,
    _Out_writes_bytes_( cbScratch )                             PBYTE                   pbScratch,
                                                                SIZE_T                  cbScratch )
{
    UINT32 nWords = nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32;
    UINT32 activeDivWords = (pdDivisor->nBits + 8 * sizeof(UINT32) - 1) / (8 * sizeof( UINT32 ) );
    UINT32 remainderWords = SYMCRYPT_OBJ_NUINT32( pdDivisor );

    UINT32 cbScratchNeeded = (nWords+4) * sizeof( UINT32 );
    PUINT32 pTmp = (PUINT32) pbScratch;
    UINT32 Qest;
    UINT32 Q;
    UINT32 c;
    UINT32 d;
    UINT32 shift;
    UINT32 X0, X1;
    UINT32 W;
    UINT64 T;
    UINT32 nQ;

    SYMCRYPT_ASSERT( cbScratch >= cbScratchNeeded );
    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbScratch );

    if( nWords < activeDivWords )
    {
        //
        // input is smaller in size than the significant size of the divisor, no division to do.
        // Note that both values in the if() statement are public, so this does not create a side channel.
        //

        // Set quotient to zero, and the remainder to the input value
        if( pQuotient != NULL )
        {
            SymCryptWipe( pQuotient, nDigits * SYMCRYPT_FDEF_DIGIT_SIZE );
        }

        if( pRemainder != NULL )
        {
            SYMCRYPT_ASSERT( remainderWords >= nWords );
            memcpy( pRemainder, pNum, nWords * sizeof( UINT32 ) );
            SymCryptWipe( &pRemainder[nWords], (remainderWords - nWords) * sizeof( UINT32 ) );        // clear the rest of the remainder words
        }

        SymCryptFdefClaimScratch( pbScratch, cbScratch, cbScratchNeeded );
        goto cleanup;
    }

    //
    // We have two zero words in front and two zero words behind the tmp value to allow unrestricted accesses.
    // We keep the explicit offset of 2 rather than adjust the pTmp pointer to avoid negative indexes which appear
    // to be buffer overflows, and cause trouble with unsigned computations of negative index values that overflow
    // to 2^32 - 1 on a 64-bit CPU.
    //
    pTmp[0] = pTmp[1] = 0;
    memcpy( &pTmp[2], pNum, nWords * sizeof( UINT32 ) );
    pTmp[nWords + 2] = pTmp[nWords + 3] = 0;
    shift = (0 - pdDivisor->nBits) & 31;   // # bits we have to shift top words to the left to align with the W value

    // We generate the quotient words one at a time, starting at the most significant position
    // The top (divWords - 1) words are always zero

    if( pQuotient != NULL )
    {
        SymCryptWipe( &pQuotient[nWords - activeDivWords + 1], (activeDivWords - 1) * sizeof( UINT32 ) );
    }

    nQ = nWords - activeDivWords + 1;

    // There is always at least one word of Q to be computed, so we can use a do-while loop which
    // also avoids the UINT32 underflow.
    do
    {
        nQ--;
        X0 = ( ((UINT64) pTmp[nQ + activeDivWords + 2] << 32) + pTmp[nQ + activeDivWords + 1] ) >> (32 - shift);
        X1 = ( ((UINT64) pTmp[nQ + activeDivWords + 1] << 32) + pTmp[nQ + activeDivWords + 0] ) >> (32 - shift);

        W = (UINT32) pdDivisor->td.fdef.W;
        T = SYMCRYPT_MUL32x32TO64( W, X0 ) + (((UINT64)X0) << 32) + X1 + ((W>>1) & ((UINT32)0 - (X1 >> 31)));
        Qest = (UINT32)(T >> 32);
        // At this point the estimator is correct or one too small, add one but don't overflow
        Qest += 1;
        Qest += SYMCRYPT_MASK32_ZERO( Qest );

        c = SymCryptFdefRawMultSubUint32( &pTmp[nQ+2], SYMCRYPT_FDEF_INT_PUINT32( &pdDivisor->Int ), Qest, activeDivWords );
        Q = Qest - c;
        d = SymCryptFdefRawMaskedAddSubdigit( &pTmp[nQ+2], SYMCRYPT_FDEF_INT_PUINT32( &pdDivisor->Int ), (0-c), activeDivWords );
        SYMCRYPT_ASSERT( c == d );
        SYMCRYPT_ASSERT( pTmp[nQ + activeDivWords+2] == (0 - c) );

        if( pQuotient != NULL )
        {
            pQuotient[nQ] = Q;
        }
    } while( nQ > 0 );

    if( pRemainder != NULL )
    {
        memcpy( pRemainder, pTmp+2, activeDivWords * sizeof( UINT32 ) );
        SymCryptWipe( &pRemainder[activeDivWords], (remainderWords - activeDivWords) * sizeof( UINT32 ) );
    }

cleanup:
    return;         // label needs a statement to follow it...
}


VOID
SYMCRYPT_CALL
SymCryptFdefIntDivMod(
    _In_                            PCSYMCRYPT_INT      piSrc,
    _In_                            PCSYMCRYPT_DIVISOR  pdDivisor,
    _Out_opt_                       PSYMCRYPT_INT       piQuotient,
    _Out_opt_                       PSYMCRYPT_INT       piRemainder,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch )
{
    UINT32  nDigits = SYMCRYPT_OBJ_NDIGITS( piSrc );

    SYMCRYPT_ASSERT( piQuotient  == NULL || piQuotient->nDigits >= piSrc->nDigits );
    SYMCRYPT_ASSERT( piRemainder == NULL || piRemainder->nDigits >= pdDivisor->nDigits );

    SymCryptFdefRawDivMod(
        SYMCRYPT_FDEF_INT_PUINT32( piSrc ),
        nDigits,
        pdDivisor,
        piQuotient  == NULL ? NULL : SYMCRYPT_FDEF_INT_PUINT32( piQuotient ),
        piRemainder == NULL ? NULL : SYMCRYPT_FDEF_INT_PUINT32( piRemainder ),
        pbScratch,
        cbScratch
        );

    if ((piQuotient != NULL) && (piQuotient->nDigits > piSrc->nDigits))
    {
        SymCryptWipe( &SYMCRYPT_FDEF_INT_PUINT32( piQuotient )[piSrc->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32], (piQuotient->nDigits - piSrc->nDigits) * SYMCRYPT_FDEF_DIGIT_SIZE );
    }

    if ((piRemainder != NULL) && (piRemainder->nDigits > pdDivisor->nDigits))
    {
        SymCryptWipe( &SYMCRYPT_FDEF_INT_PUINT32( piRemainder )[pdDivisor->nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32], (piRemainder->nDigits - pdDivisor->nDigits) * SYMCRYPT_FDEF_DIGIT_SIZE );
    }
}
