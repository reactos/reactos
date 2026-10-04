//
// fdef_general.c   General functions of the default format.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//
//

#include "precomp.h"

#include "smallPrimes32.h"      // For SymCryptTestTrialdivisionMaxSmallPrime

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64

#define SYMCRYPT_TRIALDIVISION_DIGIT_REDUCTION_CYCLES   (16)        // Measured on amd64
#define SYMCRYPT_TRIALDIVISION_DIVIDE_TEST_CYCLES       (2)         // Measured on amd64
#define SYMCRYPT_RABINMILLER_DIGIT_CYCLES               (43000)     // Measured on amd64

#elif SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM

#define SYMCRYPT_TRIALDIVISION_DIGIT_REDUCTION_CYCLES   (18)        // Measured on x86
#define SYMCRYPT_TRIALDIVISION_DIVIDE_TEST_CYCLES       (16)        // Measured on x86
#define SYMCRYPT_RABINMILLER_DIGIT_CYCLES               (25300)     // Measured on x86

#else

#define SYMCRYPT_TRIALDIVISION_DIGIT_REDUCTION_CYCLES   (18)        // Measured on x86
#define SYMCRYPT_TRIALDIVISION_DIVIDE_TEST_CYCLES       (16)        // Measured on x86
#define SYMCRYPT_RABINMILLER_DIGIT_CYCLES               (25300)     // Measured on x86

#endif


#define SYMCRYPT_TRIALDIVISION_MAX_SMALL_PRIME          (1<<22)     // Some large limit to bound memory usage
C_ASSERT( SYMCRYPT_TRIALDIVISION_MAX_SMALL_PRIME <= UINT32_MAX );
C_ASSERT( SYMCRYPT_TRIALDIVISION_MAX_SMALL_PRIME == ((UINT32) SYMCRYPT_TRIALDIVISION_MAX_SMALL_PRIME) );

VOID
SYMCRYPT_CALL
SymCryptFdefMaskedCopyC(
    _In_reads_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )        PCBYTE      pbSrc,
    _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbDst,
                                                                UINT32      nDigits,
                                                                UINT32      mask )
	/*
	This function is dangerous, and would create a buffer overflow if nDigits > nDigits for pbDst
	It also appears that it is never called. Consider removing it if it is not needed.
	*/
{
    UINT64 m64 = (UINT64)0 - (mask & 1);
    PUINT64 pSrc = (PUINT64) pbSrc; // should be a const pointer to match pSrc
    PUINT64 pDst = (PUINT64) pbDst;
    SIZE_T i;

	// This allows 0xffffffff and 0, is that what you wanted?
	// If so, ( mask == 0xffffffff || mask == 0 )
	// would be more readable. It is also odd that 1 is not valid, but it results in exactly the
	// same code flow as ~0.
    SYMCRYPT_ASSERT( (mask + 1) < 2 );      // Check that mask is valid

	// This - nDigits * SYMCRYPT_FDEF_DIGIT_SIZE / sizeof( UINT64 )
	// seems to occur often. Consider a macro with a name that explains what you are doing
	// A comment on the macro which explains why this multiplication is never a problem would be
	// helpful - I'm fairly sure it is not a problem.
    for( i=0; i< nDigits * SYMCRYPT_FDEF_DIGIT_SIZE / sizeof( UINT64 ); i += 2 )
    {
        pDst[i  ] = (pSrc[i  ] & m64) | (pDst[i  ] & ~m64 );
        pDst[i+1] = (pSrc[i+1] & m64) | (pDst[i+1] & ~m64 );
    }
}

VOID
SYMCRYPT_CALL
SymCryptFdefMaskedCopy(
    _In_reads_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )        PCBYTE      pbSrc,
    _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbDst,
                                                                UINT32      nDigits,
                                                                UINT32      mask )
{
#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM64 | SYMCRYPT_CPU_ARM
    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbSrc );
    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbDst );
    SymCryptFdefMaskedCopyAsm( pbSrc, pbDst, nDigits, mask );
#else
    SymCryptFdefMaskedCopyC( pbSrc, pbDst, nDigits, mask );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptFdefConditionalSwapC(
    _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbSrc1,
    _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbSrc2,
                                                                UINT32      nDigits,
                                                                UINT32      cond )
{
	/*
	Some documentation as to what the cond argument means would be helpful.
	*/
    UINT64  m64 = (UINT64)0 - (cond & 1);
    PUINT64 pSrc1 = (PUINT64) pbSrc1;
    PUINT64 pSrc2 = (PUINT64) pbSrc2;
    UINT64  tmp1 = 0;
    UINT64  tmp2 = 0;
    SIZE_T  i;

	// Unlike the previous function, this only allows 0 and 1 why?
    SYMCRYPT_ASSERT( cond < 2 );      // Check that the condition is valid

    for( i=0; i< nDigits * SYMCRYPT_FDEF_DIGIT_SIZE / sizeof( UINT64 ); i += 2 )
    {
        tmp1 = (pSrc1[i  ] ^ pSrc2[i  ]) & m64;
        tmp2 = (pSrc1[i+1] ^ pSrc2[i+1]) & m64;

        pSrc1[i  ] ^= tmp1;    pSrc2[i  ] ^= tmp1;
        pSrc1[i+1] ^= tmp2;    pSrc2[i+1] ^= tmp2;
    }
}

VOID
SYMCRYPT_CALL
SymCryptFdefConditionalSwap(
    _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbSrc1,
    _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbSrc2,
                                                                UINT32      nDigits,
                                                                UINT32      cond )
{
    SymCryptFdefConditionalSwapC( pbSrc1, pbSrc2, nDigits, cond );
}


UINT32
SymCryptFdefDigitsFromBits( UINT32 nBits )
{
    UINT32  res;

    if( nBits == 0 )
    {
        res = 1;
    }
    else
    {
        SYMCRYPT_ASSERT( nBits <= SYMCRYPT_INT_MAX_BITS );

        // Callers with integers larger than SYMCRYPT_INT_MAX_BITS should not occur in real use cases
        // To avoid overflow issues, return the 0 digits to indicate an error which can be handled by
        // callers, or flow through into object allocation which will in turn recognize the invalid
        // digit count.
        if( nBits > SYMCRYPT_INT_MAX_BITS )
        {
            res = 0;
        } else {
            res = SYMCRYPT_FDEF_DIGITS_FROM_BITS( nBits );
        }
    }

    return res;
}

// Let's limit max bits to the number of bits we actually test
C_ASSERT( SYMCRYPT_INT_MAX_BITS < (1 << 30) );      // Larger values can cause overflows and sign confusion

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptFdefIntAllocate( UINT32 nDigits )
{
    PVOID               p = NULL;
    UINT32              cb;
    PSYMCRYPT_INT       res = NULL;

    //
    // The nDigits requirements are enforced by SymCryptFdefSizeofIntFromDigits. Thus
    // the result does not overflow and is upper bounded by 2^18.
    //
    cb = SymCryptFdefSizeofIntFromDigits( nDigits );

    if( cb != 0 )
    {
        p = SymCryptCallbackAlloc( cb );
    }

    if( p == NULL )
    {
        goto cleanup;
    }

    res = SymCryptIntCreate( p, cb, nDigits );

cleanup:
    return res;
}


UINT32
SYMCRYPT_CALL
SymCryptFdefSizeofIntFromDigits( UINT32 nDigits )
{
    SYMCRYPT_ASSERT( nDigits != 0 );
    SYMCRYPT_ASSERT( nDigits <= SYMCRYPT_FDEF_UPB_DIGITS );

    // Ensure we do not overflow the following calculation when provided with invalid inputs
    if( nDigits == 0 || nDigits > SYMCRYPT_FDEF_UPB_DIGITS )
    {
        return 0;
    }

    // Note: ti stands for 'Type-Int' and it helps catch type errors when type-casting macros are used.
    return SYMCRYPT_FIELD_OFFSET( SYMCRYPT_INT, ti ) + nDigits * SYMCRYPT_FDEF_DIGIT_SIZE;
}

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptFdefIntCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer,
                                    UINT32  nDigits )
{
    PSYMCRYPT_INT pInt = NULL;
    UINT32 cb = SymCryptFdefSizeofIntFromDigits( nDigits );

    SYMCRYPT_ASSERT( cb >= sizeof(SYMCRYPT_INT) );
    SYMCRYPT_ASSERT( cbBuffer >= cb );
    if( (cb == 0) || (cbBuffer < cb) )
    {
        goto cleanup; // return NULL
    }

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbBuffer );
    pInt = (PSYMCRYPT_INT) pbBuffer;

    pInt->type = 'gI' << 16;
    pInt->nDigits = nDigits;

    //
    // The nDigits requirements are enforced by SymCryptFdefSizeofIntFromDigits. Thus
    // the result does not overflow and is upper bounded by 2^18.
    //
    pInt->cbSize = cb;

    SYMCRYPT_SET_MAGIC( pInt );

cleanup:
    return pInt;
}


VOID
SymCryptFdefIntCopyFixup(
    _In_    PCSYMCRYPT_INT pSrc,
    _Out_   PSYMCRYPT_INT  pDst )
{
    UNREFERENCED_PARAMETER( pSrc );
    UNREFERENCED_PARAMETER( pDst ); // not used in FRE builds...

    SYMCRYPT_SET_MAGIC( pDst );
}

VOID
SymCryptFdefIntCopy(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SYMCRYPT_CHECK_MAGIC( piSrc );
    SYMCRYPT_CHECK_MAGIC( piDst );

    SYMCRYPT_ASSERT( piSrc->nDigits == piDst->nDigits );

    //
    // in-place copy is somewhat common, and addresses are always public, so we can test for a no-op copy.
    //
    if( piSrc != piDst )
    {
        // This is normally considered a banned, unsafe function. A note about why it is safe in this use
        // would be good.
        memcpy( SYMCRYPT_FDEF_INT_PUINT32( piDst ), SYMCRYPT_FDEF_INT_PUINT32( piSrc ), SYMCRYPT_OBJ_NBYTES( piDst ));
    }
}

VOID
SymCryptFdefIntMaskedCopy(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Inout_ PSYMCRYPT_INT   piDst,
            UINT32          mask )
	/*
	Function notes would be helpful - what is mask, what does it do?
	*/
{
    SYMCRYPT_CHECK_MAGIC( piSrc );
    SYMCRYPT_CHECK_MAGIC( piDst );

    SYMCRYPT_ASSERT( piSrc->nDigits == piDst->nDigits );

    SymCryptFdefMaskedCopy( (PBYTE) SYMCRYPT_FDEF_INT_PUINT32( piSrc ), (PBYTE) SYMCRYPT_FDEF_INT_PUINT32( piDst ), piSrc->nDigits, mask );
}

VOID
SYMCRYPT_CALL
SymCryptFdefIntConditionalCopy(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Inout_ PSYMCRYPT_INT   piDst,
            UINT32          cond )
{
    SYMCRYPT_CHECK_MAGIC( piSrc );
    SYMCRYPT_CHECK_MAGIC( piDst );

    SYMCRYPT_ASSERT( piSrc->nDigits == piDst->nDigits );

    SymCryptFdefMaskedCopy( (PBYTE) SYMCRYPT_FDEF_INT_PUINT32( piSrc ), (PBYTE) SYMCRYPT_FDEF_INT_PUINT32( piDst ), piSrc->nDigits, SYMCRYPT_MASK32_NONZERO( cond ) );
}

VOID
SYMCRYPT_CALL
SymCryptFdefIntConditionalSwap(
    _Inout_ PSYMCRYPT_INT   piSrc1,
    _Inout_ PSYMCRYPT_INT   piSrc2,
            UINT32          cond )
{
    SYMCRYPT_CHECK_MAGIC( piSrc1 );
    SYMCRYPT_CHECK_MAGIC( piSrc2 );

    SYMCRYPT_ASSERT( piSrc1->nDigits == piSrc2->nDigits );

    SymCryptFdefConditionalSwap( (PBYTE) SYMCRYPT_FDEF_INT_PUINT32( piSrc1 ), (PBYTE) SYMCRYPT_FDEF_INT_PUINT32( piSrc2 ), piSrc1->nDigits, cond );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntBitsizeOfObject( _In_ PCSYMCRYPT_INT  piSrc )
{
    // This does not overflow since the nDigits field is
    // bounded by SYMCRYPT_FDEF_UPB_DIGITS.
    return SYMCRYPT_FDEF_DIGIT_BITS * piSrc->nDigits;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefNumberofDigitsFromInt( _In_ PCSYMCRYPT_INT piSrc )
{
    return piSrc->nDigits;
}

SYMCRYPT_ERROR
SymCryptFdefIntCopyMixedSize(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst )
{
    UINT32  n;
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SYMCRYPT_CHECK_MAGIC( piSrc );
    SYMCRYPT_CHECK_MAGIC( piDst );

    // in-place copy is somewhat common, and addresses are always public, so we can test for a no-op copy.
    if( piSrc == piDst )
    {
        goto cleanup;
    }

    //
    // Copy the digits that are available in both
    //
    n = SYMCRYPT_MIN( piSrc->nDigits, piDst->nDigits );
    memcpy( SYMCRYPT_FDEF_INT_PUINT32( piDst ), SYMCRYPT_FDEF_INT_PUINT32( piSrc ), n * SYMCRYPT_FDEF_DIGIT_SIZE );

    if( piDst->nDigits > n )
    {
        SymCryptWipe( &SYMCRYPT_FDEF_INT_PUINT32( piDst )[n * SYMCRYPT_FDEF_DIGIT_NUINT32], (piDst->nDigits - n) * SYMCRYPT_FDEF_DIGIT_SIZE );
    }

    if( piSrc->nDigits > n )
    {
        // Check that the rest of the source is zero
        PUINT64 p = (PUINT64) &SYMCRYPT_FDEF_INT_PUINT32( piSrc )[n * SYMCRYPT_FDEF_DIGIT_NUINT32];
        UINT64 v = 0;
        UINT32 i = (piSrc->nDigits - n) * SYMCRYPT_FDEF_DIGIT_SIZE / sizeof( UINT64 );
        while( i > 0 )
        {
            v |= *p++;
            i--;
        }

        //
        // If the Src doesn't fit, we are allowed to publish that fact, so we can use an IF.
        //
        if( v != 0 )
        {
            scError = SYMCRYPT_BUFFER_TOO_SMALL;
            goto cleanup;
        }
    }

cleanup:
    return scError;
}


UINT32
SYMCRYPT_CALL
SymCryptFdefBitsizeOfUint32( UINT32 v )
{
    UINT32 res;
    UINT32 mask;
    UINT32 vUpper;
    UINT32 vBit1;

    // This is tricky to do side-channel safe using only defined behaviour of the C language.

	// This is very difficult to make any sense of. A comment containing the C code that one would normally
	// write to do the same thing would be helpful. I will need to come back to this.
	// Also, there is no test coverage of this function. There should be a unit test to show that it does the same thing
	// as the code one would normally write.

    vUpper = v & 0xffff0000;
    mask = (UINT32) ( (0 -(UINT64)(vUpper)) >> 32 );      // mask = 0 or 0xffffffff
    res = mask & 16;                                      // Why do we want the 9th bit? Also, 0x10 would be better here
    v = ((v & 0xffff) & ~mask) | ((vUpper >> 16) & mask);

    vUpper = v & 0xff00;
    mask = (0 - vUpper) >> 16;                           // mask = 0 or 0xffff
    res |= mask & 8;
    v = ((v & 0xff) & ~mask) | ((v >> 8) & mask);

    vUpper = v & 0xf0;
    mask = (0 - vUpper) >> 16;
    res |= mask & 4;
    v = ((v & 0xf) & ~mask) | ((v >> 4) & mask );

    vUpper = v & 0xc;
    mask = (0 - vUpper) >> 16;
    res |= mask & 2;
    v = ((v & 0x3) & ~mask) | ((v >> 2) & mask);

    //
    // Only 2 bits left.
    //
    vBit1 = (v >> 1) & 1;
    res |= vBit1;

    //
    // Now we have the bit number of the MSbit set in res.
    // We need to increase this by one if v was nonzero, so that we
    // get 0 for v==0, and the # bits needed for v > 0
    //
    res += (v | vBit1) & 1;

    return res;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntBitsizeOfValue( _In_ PCSYMCRYPT_INT piSrc )
{
    UINT32  nUint32 = SYMCRYPT_OBJ_NUINT32( piSrc );

    UINT32  res = 0;
    UINT32  msNonzeroWord = 0;      // most significant nonzero digit
    UINT32  searchingMask = SYMCRYPT_MASK32_SET; // Set if still searching, 0 otherwise
    UINT32  d;
    UINT32  dIsNonzeroMask;
    UINT32  foundMask;

    SYMCRYPT_CHECK_MAGIC( piSrc );

	// This while loop reveals the value of nUint32, is that OK?
	// If so, document why
    while( nUint32 > 0 )
    {
        //
        // Invariant:
        // If no nonzero digit has been found, res = 0 and updateMask = -1.
        // If a nonzero digit has been found:
        //  msNonzeroDigit = most significant nonzero digit in Src
        //  res = index where most-significant nonzero digit was found
        //  updateMask = 0
        //

        nUint32--;
        d = SYMCRYPT_FDEF_INT_PUINT32( piSrc )[nUint32];

        dIsNonzeroMask = SYMCRYPT_MASK32_NONZERO( d );
        foundMask = dIsNonzeroMask & searchingMask;
        res |= nUint32 & foundMask;
        msNonzeroWord |= d & foundMask;
        searchingMask &= ~foundMask;
    }

    //
    // If all words are zero, then res == 0 and msNonzeroDigit == 0.
    //
    res = res * 8 * sizeof( UINT32 ) + SymCryptFdefBitsizeOfUint32( msNonzeroWord );

    return res;
}

VOID
SYMCRYPT_CALL
SymCryptFdefIntSetValueUint32(
            UINT32          u32Src,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SYMCRYPT_CHECK_MAGIC( piDst );

    SymCryptWipe( SYMCRYPT_FDEF_INT_PUINT32( piDst ), SYMCRYPT_OBJ_NBYTES( piDst ) );
    SYMCRYPT_FDEF_INT_PUINT32( piDst )[0] = u32Src;
}

C_ASSERT( SYMCRYPT_FDEF_DIGIT_SIZE >= 8 );      // Code below fails if this doesn't hold

VOID
SYMCRYPT_CALL
SymCryptFdefIntSetValueUint64(
            UINT64          u64Src,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SYMCRYPT_CHECK_MAGIC( piDst );

    SymCryptWipe( SYMCRYPT_FDEF_INT_PUINT32( piDst ), SYMCRYPT_OBJ_NBYTES( piDst ) );
    SYMCRYPT_FDEF_INT_PUINT32( piDst )[0] = (UINT32) u64Src;
    SYMCRYPT_FDEF_INT_PUINT32( piDst )[1] = (UINT32)(u64Src >> 32);
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefRawSetValue(
    _In_reads_bytes_(cbSrc)                             PCBYTE                  pbSrc,
                                                        SIZE_T                  cbSrc,
                                                        SYMCRYPT_NUMBER_FORMAT  format,
    _Out_writes_(nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32) PUINT32                 pDst,
                                                        UINT32                  nDigits )
{
    SYMCRYPT_ERROR scError;
    UINT32  b;
    INT32   step;
    UINT32  w;
    UINT32  windex;
    UINT32  i;
    UINT32  nWords = nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32;

    //
    // This is a very simple and slow generic implementation;
    // We'll create optimized versions for specific CPU platforms
    // (e.g. use of memcpy)
    //

    // I assume the number format is public?
    switch( format )
    {
    case SYMCRYPT_NUMBER_FORMAT_LSB_FIRST:
        step = 1;
        break;
    case SYMCRYPT_NUMBER_FORMAT_MSB_FIRST:
        step = -1;
        pbSrc += cbSrc; // avoid tripping pointer overflow sanitizer with cbSrc == 0
        pbSrc--;
        break;
    default:
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    for( windex = 0; windex < nWords; windex++ )
    {
        w = 0;
        for( i=0; i<4; i++ )
        {
            // read the next byte into b
            if( cbSrc > 0 )
            {
                b = *pbSrc;
                cbSrc -= 1;
                pbSrc += step;
                w |= b << 8*i;
            }
        }
        pDst[windex] = w;
    }

    // Inspect any remaining input bytes
    b = 0;
    while( cbSrc > 0 )
    {
        b |= *pbSrc;
        pbSrc += step;
        cbSrc -= 1;
    }

    if( b > 0 )
    {
        scError = SYMCRYPT_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefIntSetValue(
    _In_reads_bytes_(cbSrc)     PCBYTE                  pbSrc,
                                SIZE_T                  cbSrc,
                                SYMCRYPT_NUMBER_FORMAT  format,
    _Out_                       PSYMCRYPT_INT           piDst )
{
    SYMCRYPT_ERROR scError;

    SYMCRYPT_CHECK_MAGIC( piDst );

    scError = SymCryptFdefRawSetValue( pbSrc, cbSrc, format, SYMCRYPT_FDEF_INT_PUINT32( piDst ), piDst->nDigits );

    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefRawGetValue(
    _In_reads_(nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32)   PCUINT32                pSrc,
                                                        UINT32                  nDigits,
    _Out_writes_bytes_(cbDst)                           PBYTE                   pbDst,
                                                        SIZE_T                  cbDst,
                                                        SYMCRYPT_NUMBER_FORMAT  format )
{
    SYMCRYPT_ERROR scError;
    UINT32  b;
    INT32   step;
    UINT32  w;
    UINT32  windex;
    UINT32  i;
    UINT32  nWords = nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32;

    //
    // This is a very simple and slow generic implementation;
    // We'll create optimized versions for specific CPU platforms
    // (e.g. use of memcpy)
    //

    switch( format )
    {
    case SYMCRYPT_NUMBER_FORMAT_LSB_FIRST:
        step = 1;
        break;
    case SYMCRYPT_NUMBER_FORMAT_MSB_FIRST:
        step = -1;
        pbDst += cbDst; // avoid tripping pointer overflow sanitizer with cbSrc == 0
        pbDst--;
        break;
    default:
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    for( windex = 0; windex < nWords; windex++ )
    {
        w = pSrc[windex];
        for( i=0; i<4; i++ )
        {
            b = w & 0xff;
            w >>= 8;

            // write the next byte
            if( cbDst > 0 )
            {
                *pbDst = (BYTE)b;
                cbDst -= 1;
                pbDst += step;
            } else {
                if( b != 0 )
                {
                    scError = SYMCRYPT_BUFFER_TOO_SMALL;
                    goto cleanup;
                }
            }
        }
    }

    // Zero any remaining output bytes
    while( cbDst > 0 )
    {
        *pbDst = 0;
        pbDst += step;
        cbDst -= 1;
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefIntGetValue(
    _In_                        PCSYMCRYPT_INT          piSrc,
    _Out_writes_bytes_(cbDst)   PBYTE                   pbDst,
                                SIZE_T                  cbDst,
                                SYMCRYPT_NUMBER_FORMAT  format )
{
    SYMCRYPT_ERROR scError;

    SYMCRYPT_CHECK_MAGIC( piSrc );

    scError = SymCryptFdefRawGetValue( &SYMCRYPT_FDEF_INT_PUINT32( piSrc )[0], piSrc->nDigits, pbDst, cbDst, format );

    return scError;
}


UINT32
SYMCRYPT_CALL
SymCryptFdefIntGetValueLsbits32( _In_  PCSYMCRYPT_INT piSrc )
{
    // nDigits cannot be zero, so we don't have to test
    return SYMCRYPT_FDEF_INT_PUINT32( piSrc )[0];
}

UINT64
SYMCRYPT_CALL
SymCryptFdefIntGetValueLsbits64( _In_  PCSYMCRYPT_INT piSrc )
{
    // nDigits cannot be zero, so we don't have to test
    PCUINT32 p = SYMCRYPT_FDEF_INT_PUINT32( piSrc );
    return ((UINT64)(p[1]) << 32) | p[0];
}

UINT32
SYMCRYPT_CALL
SymCryptFdefRawIsEqualUint32(
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32) PCUINT32        pSrc1,
                                                    UINT32          nDigits,
    _In_                                            UINT32          u32Src2 )
{
    UINT32 d;
    UINT32 nWords = nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32;

    d = pSrc1[0] ^ u32Src2;
    for( UINT32 i=1; i<nWords; i++)
    {
        d |= pSrc1[i];
    }

    return SYMCRYPT_MASK32_ZERO( d );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntIsEqualUint32(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    UINT32          u32Src2 )
{
    return SymCryptFdefRawIsEqualUint32( &SYMCRYPT_FDEF_INT_PUINT32( piSrc1 )[0], piSrc1->nDigits, u32Src2 );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntIsEqual(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    PCSYMCRYPT_INT  piSrc2 )
{
    UINT32 d;
    UINT32  n1 = SYMCRYPT_OBJ_NUINT32( piSrc1 );
    UINT32  n2 = SYMCRYPT_OBJ_NUINT32( piSrc2 );
    UINT32  i;
    UINT32  n;
    PCUINT32 pSrc1 = SYMCRYPT_FDEF_INT_PUINT32( piSrc1 );
    PCUINT32 pSrc2 = SYMCRYPT_FDEF_INT_PUINT32( piSrc2 );

    n = SYMCRYPT_MIN( n1, n2 );
    d = 0;
    for( i=0; i < n ; i++ )
    {
        d |= pSrc1[i] ^ pSrc2[i];
    }

    // i == n1 or i == n2, so at most one of the 2 loops below is ever run

    while( i < n1 )
    {
        d |= pSrc1[i];
        i++;
    }

    while( i < n2 )
    {
        d |= pSrc2[i];
        i++;
    }

    return SYMCRYPT_MASK32_ZERO( d );
}

PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptFdefDivisorAllocate( UINT32 nDigits )
{
    PVOID               p = NULL;
    UINT32              cb;
    PSYMCRYPT_DIVISOR   res = NULL;

    //
    // The nDigits requirements are enforced by SymCryptFdefSizeofDivisorFromDigits. Thus
    // the result does not overflow and is upper bounded by 2^19.
    //
    cb = SymCryptFdefSizeofDivisorFromDigits( nDigits );

    if( cb != 0 )
    {
        p = SymCryptCallbackAlloc( cb );
    }

    if( p == NULL )
    {
        goto cleanup;
    }

    res = SymCryptFdefDivisorCreate( p, cb, nDigits );

cleanup:
    return res;
}

UINT32
SYMCRYPT_CALL
SymCryptFdefSizeofDivisorFromDigits( UINT32 nDigits )
{
    SYMCRYPT_ASSERT( nDigits != 0 );
    SYMCRYPT_ASSERT( nDigits <= SYMCRYPT_FDEF_UPB_DIGITS );

    // Ensure we do not overflow the following calculation when provided with invalid inputs
    if( nDigits == 0 || nDigits > SYMCRYPT_FDEF_UPB_DIGITS )
    {
        return 0;
    }

    return SYMCRYPT_FIELD_OFFSET( SYMCRYPT_DIVISOR, Int ) + SymCryptFdefSizeofIntFromDigits( nDigits );
}

PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptFdefDivisorCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer,
                                    UINT32  nDigits )
{
    PSYMCRYPT_DIVISOR  pdDiv = NULL;
    UINT32 cb = SymCryptSizeofDivisorFromDigits( nDigits );

    SYMCRYPT_ASSERT( cb >= sizeof(SYMCRYPT_DIVISOR) );
    SYMCRYPT_ASSERT( cbBuffer >= cb );
    if( (cb == 0) || (cbBuffer < cb) )
    {
        goto cleanup; // return NULL
    }

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbBuffer );
    pdDiv = (PSYMCRYPT_DIVISOR) pbBuffer;

    pdDiv->type = 'gD' << 16;
    pdDiv->nDigits = nDigits;

    //
    // The nDigits requirements are enforced by SymCryptFdefSizeofDivisorFromDigits. Thus
    // the result does not overflow and is upper bounded by 2^19.
    //
    pdDiv->cbSize = cb;

    SYMCRYPT_SET_MAGIC( pdDiv );

    SymCryptIntCreate( (PBYTE)&pdDiv->Int, cbBuffer - SYMCRYPT_FIELD_OFFSET( SYMCRYPT_DIVISOR, Int ), nDigits );

cleanup:
    return pdDiv;
}

VOID
SymCryptFdefDivisorCopyFixup(
    _In_    PCSYMCRYPT_DIVISOR pdSrc,
    _Out_   PSYMCRYPT_DIVISOR  pdDst )
{
    UNREFERENCED_PARAMETER( pdSrc );
    UNREFERENCED_PARAMETER( pdDst );

    SymCryptFdefIntCopyFixup( &pdSrc->Int, &pdDst->Int );

    SYMCRYPT_SET_MAGIC( pdDst );
}

VOID
SymCryptFdefDivisorCopy(
    _In_    PCSYMCRYPT_DIVISOR  pdSrc,
    _Out_   PSYMCRYPT_DIVISOR   pdDst )
{
    SYMCRYPT_CHECK_MAGIC( pdSrc );
    SYMCRYPT_CHECK_MAGIC( pdDst );

    SYMCRYPT_ASSERT( pdSrc->nDigits == pdDst->nDigits );

    // in-place copy is somewhat common, and addresses are always public, so we can test for a no-op copy.
    if( pdSrc != pdDst )
    {
        memcpy( pdDst, pdSrc, pdDst->cbSize );

        SymCryptFdefDivisorCopyFixup( pdSrc, pdDst );
    }
}


VOID
SYMCRYPT_CALL
SymCryptFdefClaimScratch( PBYTE pbScratch, SIZE_T cbScratch, SIZE_T cbMin )
{
#if SYMCRYPT_DEBUG
    SYMCRYPT_ASSERT( cbScratch >= cbMin );
    SymCryptWipe( pbScratch, cbMin );
#else
    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );
    UNREFERENCED_PARAMETER( cbMin );
#endif
}

UINT32
SymCryptTestTrialdivisionMaxSmallPrime(
    _In_                            PCSYMCRYPT_TRIALDIVISION_CONTEXT    pContext )
{
    return pContext->maxTrialPrime;
}

UINT64
SymCryptInverseMod2e64( UINT64 m )
{
    // Compute the inv64 value such that inv64 * m = 1 mod 2^64 for odd m.
    // If m is even, there exists no inverse, this function will return a
    // useless value in constant time.
    //
    // We use Newton's method to search for a zero of f(x) := x^-1 - m, working modulo 2^64
    // We get the iteration formula
    //  x_{i+1} = x_i - f(x_i)/f'(x_i)
    //          = x_i - (x_i^-1 - m)/(-x_i^-2)
    //          = x_i + x_i^2(1/x_i - m)
    //          = x_i + x_i - (x_i^2 * m)
    //          = x_i (2 - x_i*m)
    //
    // Let x_i = d + 2^n * e where d = inv64 = m^-1 mod 2^64, and 2^n * e is the error term that is zero in the n least
    // significant bits. We have
    //  x_{i+1} = (d + 2^n * e) (2 - (d + 2^n * e) * m)
    //          = (d + 2^n * e) (2 - d*m - 2^n * e * m)
    //          = (d + 2^n * e) (2 - 1 - 2^n * e * m)
    //          = (d + 2^n * e) (1 - 2^n * e * m)
    //          = d - (2^n * e * (d*m)) + (2^n * e) - (2^{2n} * e^2 * m)
    //          = d - (2^{2n} * e^2 * m)
    // In other words, the error has been squared and multiplied by m. In our case, working modulo 2^64, the number of correct bits
    // on the least significant side is doubled.
    //
    // To get a 4-bit correct estimate for m^-1 given odd m, we consider the least significant 4 bits of m and inv:
    // m   = ... m_3 m_2 m_1 m_0
    // inv = ... i_3 i_2 i_1 i_0
    // We want to directly compute i_[3..0] s.t. (m*inv) & 0xf == 1
    // working through some simple simultaneous equations it is easily shown that:
    // i_0 = m_0 = 1
    // i_1 = m_1
    // i_2 = m_2
    // i_3 = m_1 ^ m_2 ^ m_3
    // Once we have 4 correct bits, we can double that multiple times using Newton's method.
    //
    // We use 32-bit operations for most of the iterations for speed on 32-bit platforms.
    //
    UINT32 inv32;
    UINT64 inv64;
    UINT32 m32;

    m32 = (UINT32)m;

    inv32 = m32 ^ (((m32 - 1) * 0x6) & 0x8); // sets inv32 bits [3..0]
    SYMCRYPT_ASSERT( ((m&1) == 0) || (((inv32 * m32) & 0xf) == 1) );

    inv32 = inv32 * (2 - inv32 * m32 );
    SYMCRYPT_ASSERT( ((m&1) == 0) || (((inv32 * m32) & 0xff) == 1) );

    inv32 = inv32 * (2 - inv32 * m32 );
    SYMCRYPT_ASSERT( ((m&1) == 0) || (((inv32 * m32) & 0xffff) == 1) );

    inv32 = inv32 * (2 - inv32 * m32 );
    SYMCRYPT_ASSERT( ((m&1) == 0) || ((inv32 * m32) == 1) );

    inv64 = inv32;
    inv64 = inv64 * (2 - inv64 * m );
    SYMCRYPT_ASSERT( ((m&1) == 0) || ((inv64 * m) == 1) );

    return inv64;
}


VOID
SYMCRYPT_CALL
SymCryptFdefInitTrialdivisionPrime(
            UINT32                          prime,
    _Out_   PSYMCRYPT_TRIALDIVISION_PRIME   pPrime )
{
    // Compute the inverse of the prime mod 2^64
    pPrime->invMod2e64 = SymCryptInverseMod2e64( prime );
    pPrime->compareLimit = ((UINT64) -1) / prime;
}

FORCEINLINE
UINT32
SymCryptIsMultipleOfSmallPrime( UINT64 value, PCSYMCRYPT_TRIALDIVISION_PRIME pPrime )
{
    return (value * pPrime->invMod2e64) <= pPrime->compareLimit;
}

VOID
SYMCRYPT_CALL
SymCryptFdefInitTrialDivisionGroup( PSYMCRYPT_TRIALDIVISION_GROUP pGroup, UINT32 nPrimes, UINT32 primeProd )
{
    UINT32 f;
    UINT32 r;
    UINT32 i;

    pGroup->nPrimes = nPrimes;

    // These % operations are expensive; maybe we can optimize this further.
    // In assembler we can do the UINT64 % UINT32 -> UINT32
    // hopefully the compiler is smart enough...

    f = (UINT32) (((UINT64)1 << 32) % primeProd);
    pGroup->factor[0] = f;
    r = f;
    for( i=1; i<9; i++ )
    {
        r = (UINT32) (SYMCRYPT_MUL32x32TO64( r, f ) % primeProd);
        pGroup->factor[i] = r;
    }
}

UINT32
SYMCRYPT_CALL
SymCryptGenerateSmallPrimes( UINT32 maxPrime, PUINT32 * ppList )
{
    // returns a list of small primes, excluding 2, 3, 5, and 17.
    UINT32 nPrimes = 0;
    PUINT32 pList = NULL;

    // pSieve[i] corresponds to 2*i+1
    // value X is in location X/2
    UINT32 nSieve;
    PBYTE pSieve;

    UINT32 pi;
    UINT32 p;
    UINT32 si;
    UINT32 i;

    maxPrime = SYMCRYPT_MAX( maxPrime, 32 );         // simplify error handling by always producing primes at least up to 32
    maxPrime = SYMCRYPT_MIN( maxPrime, 1 << 24 );    // Limit prime list to something sane (sieve = 8 MB, list = 4 MB or so).

    // highest index is (maxPrime - 1)/2 which encodes maxPrime if odd, or maxPrime-1 if even
    nSieve = (maxPrime - 1) / 2 + 1;

    pSieve = SymCryptCallbackAlloc( nSieve );
    if( pSieve == NULL )
    {
        goto cleanup;
    }

    SymCryptWipe( pSieve, nSieve );


    pi = 1; // index of first prime 3
    p = 2*pi + 1;  // prime value
    for(;;)
    {
        si = 2*(pi*pi + pi);    // index of p^2
        if( si > nSieve )
        {
            break;  // We're done sieving
        }
        while( si < nSieve )
        {
            pSieve[si] = 1;
            si += p;
        }
        // Search for the next prime
        do {
            pi += 1;
        } while( pSieve[pi] != 0 );
        p = 2*pi + 1;
    }

    // Eliminate 3, 5, and 17
    pSieve[1] = 1;
    pSieve[2] = 1;
    pSieve[8] = 1;

    for( i=1; i<nSieve; i++ )
    {
        nPrimes += 1 - pSieve[i];
    }

	// dcl - I suspect that this is not a problem, but please document
	// why this multiplication cannot overflow. I assume there is a practical limit on nPrimes, but unsure
	// what that would be.
    pList = SymCryptCallbackAlloc( nPrimes * sizeof( UINT32 ) );
    if( pList == NULL )
    {
        goto cleanup;
    }

    pi = 0;
    for( i=1; i<nSieve; i++ )
    {
        if( pSieve[i] == 0 )
        {
            pList[pi++] = 2*i+1;
        }
    }

    SYMCRYPT_ASSERT( pi == nPrimes );

cleanup:
    if( pSieve != NULL )
    {
        SymCryptWipe( pSieve, nSieve );
        SymCryptCallbackFree( pSieve );
    }

    *ppList = pList;
    return nPrimes;
}


PCSYMCRYPT_TRIALDIVISION_CONTEXT
SYMCRYPT_CALL
SymCryptFdefCreateTrialDivisionContext( UINT32 nDigits )
{
    PSYMCRYPT_TRIALDIVISION_CONTEXT pRes = NULL;
    PBYTE pAlloc;
    UINT32 nBytes;
    UINT32 iPrime;
    UINT32 iGroup;
    UINT32 nPrimes;
    UINT32 nGroups;
    UINT32 M;
    UINT32 iGroupSpec;
    UINT32 i;
    UINT32 j;
    UINT64 cRabinMillerCost;
    UINT64 cPerPrimeCost;
    UINT64 tmp64;
    UINT32 maxPrime;
    UINT32 minPrime;
    UINT32 nSmallPrimes = 0;
    UINT32 n;
    UINT32 nP;
    UINT32 nG;
    PUINT32 pSmallPrimeList = NULL;

    // First we estimate the largest prime we will do trial division with
    // Inputs:
    // - cycles/digit of reduction per group of primes
    // - cycles/prime of divide test
    // - cycles per digit^3 for a Rabin-Miller test
    // We optimize in this model, which is pretty accurate for large inputs but underestimates the RM cost
    // for smaller sizes.

    // Compute the Rabin-Miller cost estimate. We reduce it by 20% because our cost model does not take
    // into account some of the trial-division cost such as memory footprint, cache pressure,
    // setup cost, etc. Reducing the Rabin-Miller cost leads us to do fewer trial divisions to approximately
    // balance the hidden costs.

    if( nDigits <= 1000 )
    {
        // nDigits is small enough to not have any overflows in this computation
        if( nDigits == 0 )
        {
            goto cleanup; // return NULL
        }

        cRabinMillerCost = (UINT64) nDigits * nDigits * nDigits * (SYMCRYPT_RABINMILLER_DIGIT_CYCLES * 8 / 10);
        i = 0;
        minPrime = 0;
        for(;;)
        {
            nPrimes = g_SymCryptSmallPrimeGroupsSpec[i].nPrimes;
            maxPrime = g_SymCryptSmallPrimeGroupsSpec[i].maxPrime;
            nGroups = g_SymCryptSmallPrimeGroupsSpec[i].nGroups;
            cPerPrimeCost = (UINT64) nDigits * SYMCRYPT_TRIALDIVISION_DIGIT_REDUCTION_CYCLES / nPrimes + SYMCRYPT_TRIALDIVISION_DIVIDE_TEST_CYCLES;

            // If the last group isn't worth it, we shouldn't go to even fewer primes
            if( nGroups == 0 || maxPrime * cPerPrimeCost >=  cRabinMillerCost)
            {
                break;
            }
            i++;
            minPrime = maxPrime;
        }

        // Now we know how many primes are in the last groups, let's find out how large the largest prime should be
        tmp64 = cRabinMillerCost / cPerPrimeCost;
        tmp64 = SYMCRYPT_MIN( tmp64, SYMCRYPT_TRIALDIVISION_MAX_SMALL_PRIME );
        maxPrime = (UINT32) tmp64;
        maxPrime = SYMCRYPT_MAX( maxPrime, minPrime );       // Make sure we don't fall into the previous group size that we don't want
    }
    else
    {
        maxPrime = SYMCRYPT_TRIALDIVISION_MAX_SMALL_PRIME;
    }

    nSmallPrimes = SymCryptGenerateSmallPrimes( maxPrime, &pSmallPrimeList );

    // Find out how many groups we'll have, and how many actual primes we'll use
    n = nSmallPrimes;
    nG = 0;
    nP = 0;
    i = 0;
    for(;;)
    {
        nPrimes = g_SymCryptSmallPrimeGroupsSpec[i].nPrimes;
        nGroups = g_SymCryptSmallPrimeGroupsSpec[i].nGroups;

        if( n < nPrimes * nGroups || nGroups == 0 )
        {
            // At the right nPrimes, compute exactly how many groups to add
            n = n / nPrimes;
            nG += n;
            nP += n * nPrimes;
            n = 0;      // No primes left
            break;
        }

        // Use up all the groups of this size...
        nG += nGroups;
        nP += nPrimes * nGroups;
        n -= nPrimes * nGroups;
        i++;
    }

	// dcl - Potential integer overflow
	// Need to document sizes, and limits of nG, nP, and confirm
	// an overflow is not possible, also recall that size_t varies in size, but nBytes is 32-bit
    nBytes = sizeof( SYMCRYPT_TRIALDIVISION_CONTEXT )
            + (nG + 1) * sizeof( SYMCRYPT_TRIALDIVISION_GROUP )     // + 1 for 0 sentinel
            + (nP + 1) * sizeof( SYMCRYPT_TRIALDIVISION_PRIME )     // + 1 for 0 sentinel
            + (nP + 1) * sizeof( UINT32 );                          // + 1 for 0 sentinel

    pAlloc = SymCryptCallbackAlloc( nBytes );
    if( pAlloc == NULL )
    {
        goto cleanup;
    }

    pRes = (PSYMCRYPT_TRIALDIVISION_CONTEXT) pAlloc;
    pAlloc += sizeof( *pRes );

    pRes->nBytesAlloc = nBytes;

    pRes->pGroupList = (PSYMCRYPT_TRIALDIVISION_GROUP)pAlloc;
    pAlloc += (nG + 1) * sizeof( SYMCRYPT_TRIALDIVISION_GROUP );

    pRes->pPrimeList = (PSYMCRYPT_TRIALDIVISION_PRIME) pAlloc;
    pAlloc += (nP + 1) * sizeof( SYMCRYPT_TRIALDIVISION_PRIME );

    pRes->pPrimes = (PUINT32) pAlloc;
    pAlloc += (nP + 1) * sizeof( UINT32 );

    SYMCRYPT_ASSERT( nBytes == (SIZE_T)(pAlloc - (PBYTE)pRes) );

    // Initialize the primes 3, 5, and 17
    SymCryptFdefInitTrialdivisionPrime(  3, &pRes->Primes3_5_17[0] );
    SymCryptFdefInitTrialdivisionPrime(  5, &pRes->Primes3_5_17[1] );
    SymCryptFdefInitTrialdivisionPrime( 17, &pRes->Primes3_5_17[2] );

    memcpy( pRes->pPrimes, pSmallPrimeList, nP * sizeof( UINT32 ) );
    pRes->pPrimes[nP] = 0;
    pRes->maxTrialPrime = pRes->pPrimes[nP-1];

    /*
    *** Old code to decrypt the nibble encoding. Keep in case we want it back later...
    // Generate the other primes from the difference table.
    // We initialize the prime structures, and a list of the primes that is used to compute the group specs

    pNibs = &g_SymCryptSmallPrimeDifferenceNibbles[0];

    smallPrime = 3;
    nPrimes = 0;
    while( smallPrime < SYMCRYPT_MAX_SMALL_PRIME )
    {
        b = *pNibs++;
        nib = b & 0xf;

        if( nib == 0 )
        {
            smallPrime += 30;
            // No check for termination here as we wouldn't encode a 0 if there wasn't another prime.
        } else {
            smallPrime += 2*nib;
            pRes->pPrimes[nPrimes] = smallPrime;
            SymCryptFdefInitTrialdivisionPrime( smallPrime, &pRes->pPrimeList[nPrimes] );
            nPrimes++;
            if( smallPrime >= SYMCRYPT_MAX_SMALL_PRIME )
            {
                break;
            }
        }
        nib = b >> 4;
        if( nib == 0 )
        {
            smallPrime += 30;
        } else {
            smallPrime += 2*nib;
            pRes->pPrimes[nPrimes] = smallPrime;
            SymCryptFdefInitTrialdivisionPrime( smallPrime, &pRes->pPrimeList[nPrimes] );
            nPrimes++;
        }
    }
    SYMCRYPT_ASSERT( smallPrime == SYMCRYPT_MAX_SMALL_PRIME && nPrimes == SYMCRYPT_N_SMALL_PRIMES_ENCODED );
    */

    for( iPrime = 0; iPrime < nP; iPrime++ )
    {
        SymCryptFdefInitTrialdivisionPrime( pRes->pPrimes[iPrime], &pRes->pPrimeList[iPrime] );
    }

    // Add the trailing 0s
    pRes->pPrimeList[nP].invMod2e64 = 0;
    pRes->pPrimeList[nP].compareLimit = 0;

    // Make sure we have the 32-bit tables, not the 64-bit ones.
	// dcl - warning suppression is not portable. Also, if it is a compile time constant, shouldn't it be a compile assert?
#pragma warning( suppress: 4127 )       // conditional expression is constant
    SYMCRYPT_ASSERT( SYMCRYPT_MAX_SMALL_PRIME_GROUP_PRODUCT <= (UINT32)-1 );

    iGroup = 0;
    iPrime = 0;
    iGroupSpec = 0;
    nPrimes = g_SymCryptSmallPrimeGroupsSpec[iGroupSpec].nPrimes;
    nGroups = g_SymCryptSmallPrimeGroupsSpec[iGroupSpec].nGroups;
    while( iPrime < nP )
    {
        if( nGroups == 0 )
        {
            iGroupSpec +=1 ;
            nPrimes = g_SymCryptSmallPrimeGroupsSpec[iGroupSpec].nPrimes;
            nGroups = g_SymCryptSmallPrimeGroupsSpec[iGroupSpec].nGroups;
            if( nGroups == 0 )
            {
                nGroups = nG - iGroup;
            }
        }

        SYMCRYPT_ASSERT( iPrime + nPrimes <= nP );
        M = pRes->pPrimes[iPrime++];
        for( j=1; j<nPrimes; j++ )
        {
            SYMCRYPT_ASSERT( M <= SYMCRYPT_MAX_SMALL_PRIME_GROUP_PRODUCT / pRes->pPrimes[iPrime] );
            M *= pRes->pPrimes[iPrime++];
        }
        SymCryptFdefInitTrialDivisionGroup( &pRes->pGroupList[iGroup], nPrimes, M );
        iGroup++;

        nGroups--;
    }

    SYMCRYPT_ASSERT( iPrime == nP && iGroup == nG );

    // Add the trailing sentinel group
    pRes->pGroupList[iGroup].nPrimes = 0;

cleanup:
    if( pSmallPrimeList != NULL )
    {
        SymCryptWipe( pSmallPrimeList, nSmallPrimes * sizeof( UINT32 ) );
        SymCryptCallbackFree( pSmallPrimeList );
        pSmallPrimeList = NULL;
    }
    return pRes;
}

VOID
SYMCRYPT_CALL
SymCryptFdefFreeTrialDivisionContext( PCSYMCRYPT_TRIALDIVISION_CONTEXT pContext )
{
    // No security reason to wipe it, but our test code verifies that we wipe everything...
    // Perf cost is minor
    SymCryptWipe( (PBYTE) pContext, pContext->nBytesAlloc );
    SymCryptCallbackFree( (PSYMCRYPT_TRIALDIVISION_CONTEXT) pContext );
}

UINT32
SYMCRYPT_CALL
SymCryptFdefIntFindSmallDivisor(
    _In_                            PCSYMCRYPT_TRIALDIVISION_CONTEXT    pContext,
    _In_                            PCSYMCRYPT_INT                      piSrc,
    _Out_writes_bytes_( cbScratch ) PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch )
{
    PCUINT32    pSrc = SYMCRYPT_FDEF_INT_PUINT32( piSrc );
    PCUINT32    p;
    UINT32      nDigits = piSrc->nDigits;
    UINT32      nUint32 = nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32;
    UINT64      Acc;
    PCSYMCRYPT_TRIALDIVISION_GROUP pGroup;
    PCSYMCRYPT_TRIALDIVISION_PRIME pPrime;
    UINT32      nPrimes;
    UINT32      res;

    // Check for 2. Not really needed for prime generation, but it makes the function easier to test/document/describe.
    if( (*pSrc & 1) == 0 )
    {
        res = 2;
        goto cleanup;
    }

    // Check the factors 3, 5, 17. These are special as they divide 2^32 - 1
    // (We could also do 257 and 65537 but that doesn't seem worth the added complexity.)
    Acc = 0;
    p = pSrc;
    do {
#if SYMCRYPT_FDEF_DIGIT_SIZE == 16
        Acc = Acc + p[0] + p[1] + p[2] + p[3];
        p += 4;
#elif (SYMCRYPT_FDEF_DIGIT_SIZE % 32) == 0
        Acc = Acc + p[0] + p[1] + p[2] + p[3] + p[4] + p[5] + p[6] + p[7];
        p += 8;
#else
		// dcl - ideally, #error would have a descriptive message so it is easily found in code if encountered, same below
#error ??
#endif
    } while( p < pSrc + nUint32 );

    if( SymCryptIsMultipleOfSmallPrime( Acc, &pContext->Primes3_5_17[0] ) )
    {
        res = 3;
        goto cleanup;
    }

    if( SymCryptIsMultipleOfSmallPrime( Acc, &pContext->Primes3_5_17[1] ) )
    {
        res = 5;
        goto cleanup;
    }

    if( SymCryptIsMultipleOfSmallPrime( Acc, &pContext->Primes3_5_17[2] ) )
    {
        res = 17;
        goto cleanup;
    }

    pGroup = pContext->pGroupList;
    pPrime = pContext->pPrimeList;
    while( (nPrimes = pGroup->nPrimes) != 0 )
    {
        // Reduce Src modulo the group product to a 64-bit value
        Acc = 0;
        p = pSrc + nUint32;

#if SYMCRYPT_FDEF_DIGIT_SIZE == 16
        if( (nUint32 & 4) != 0 )
        {
            // nUInt32 is 4 mod 8, process the top 4 words only
            p -= 4;
            Acc =
                p[0] +
                SYMCRYPT_MUL32x32TO64( p[1],  pGroup->factor[0] ) +
                SYMCRYPT_MUL32x32TO64( p[2],  pGroup->factor[1] ) +
                SYMCRYPT_MUL32x32TO64( p[3],  pGroup->factor[2] );
        } else {
            // Process 8 words to start
            p -= 8;
            Acc =
                p[0] +
                SYMCRYPT_MUL32x32TO64( p[1],  pGroup->factor[0] ) +
                SYMCRYPT_MUL32x32TO64( p[2],  pGroup->factor[1] ) +
                SYMCRYPT_MUL32x32TO64( p[3],  pGroup->factor[2] ) +
                SYMCRYPT_MUL32x32TO64( p[4],  pGroup->factor[3] ) +
                SYMCRYPT_MUL32x32TO64( p[5],  pGroup->factor[4] ) +
                SYMCRYPT_MUL32x32TO64( p[6],  pGroup->factor[5] ) +
                SYMCRYPT_MUL32x32TO64( p[7],  pGroup->factor[6] );
        }
#elif (SYMCRYPT_FDEF_DIGIT_SIZE % 32) == 0

        p -= 8;
        Acc =
            p[0] +
            SYMCRYPT_MUL32x32TO64( p[1],  pGroup->factor[0] ) +
            SYMCRYPT_MUL32x32TO64( p[2],  pGroup->factor[1] ) +
            SYMCRYPT_MUL32x32TO64( p[3],  pGroup->factor[2] ) +
            SYMCRYPT_MUL32x32TO64( p[4],  pGroup->factor[3] ) +
            SYMCRYPT_MUL32x32TO64( p[5],  pGroup->factor[4] ) +
            SYMCRYPT_MUL32x32TO64( p[6],  pGroup->factor[5] ) +
            SYMCRYPT_MUL32x32TO64( p[7],  pGroup->factor[6] );

#else
#error ??
#endif
        while( p > pSrc )
        {
            p -= 8;
            Acc =
                p[0] +
                SYMCRYPT_MUL32x32TO64( p[1],  pGroup->factor[0] ) +
                SYMCRYPT_MUL32x32TO64( p[2],  pGroup->factor[1] ) +
                SYMCRYPT_MUL32x32TO64( p[3],  pGroup->factor[2] ) +
                SYMCRYPT_MUL32x32TO64( p[4],  pGroup->factor[3] ) +
                SYMCRYPT_MUL32x32TO64( p[5],  pGroup->factor[4] ) +
                SYMCRYPT_MUL32x32TO64( p[6],  pGroup->factor[5] ) +
                SYMCRYPT_MUL32x32TO64( p[7],  pGroup->factor[6] ) +
                SYMCRYPT_MUL32x32TO64( (UINT32) Acc       , pGroup->factor[7] ) +
                SYMCRYPT_MUL32x32TO64( (UINT32)(Acc >> 32), pGroup->factor[8] );
        }

        // Now we check whether we have a multiple of one of the primes
        while( nPrimes > 0 )
        {
            if( SymCryptIsMultipleOfSmallPrime( Acc, pPrime ) )
            {
                res = pContext->pPrimes[ (pPrime - pContext->pPrimeList) ];    // pointer subtraction auto-divides by size...
                goto cleanup;
            }
            pPrime++;
            nPrimes--;
        }

        pGroup++;
    }

    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );

    // Did not find a small factor, return zero
    res = 0;

cleanup:
    return res;
}
