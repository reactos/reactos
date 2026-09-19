//
// ScsTable.c
// Side-channel safe table
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//
// These functions implement an table of large elements.
// Reading an element from the table is done in a way that does not reveal the
// element accessed through memory side channels.
// Basically, the whole table is read by the CPU, and the required data is selected
// using boolean operations.
//

#include "precomp.h"

//
// Items are multiple of SYMCRYPT_DIGIT_SIZE long.
//
// Format:
// The memory format is parameterized for optimal implementations on several
// different architectures.
//
// The following parameters define the format:
//  - group_size
//  - interleave_size
//
// Let nElements be the number of elements in the table.
// If necessary, the size of each element in the table is rounded up to a multiple of interleave_size.
// Each whole group of group_size elements is interleaved with each other.
// The last (nElements % group_size) elements are simply stored consecutively.
// (For now we simply require that nElements is a multiple of group_size.)
// Within each group of group_size, the data for the elements are interleaved in natural order
// using chunks of interleave_size bytes.
//
// The choice of group_size and interleave_size depends on the CPU architecture, CPU features,
// and even the element size. (E.g. 1024-bit elements might interleave @ 64 bytes on an AVX512
// capable CPU, but 256-bit elements would have to interleave at 16 or 32 bytes on that same CPU.)
//

// Currently these are constants as that allows easier optimizations...
#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64
#define SYMCRYPT_SCSTABLE_USE64             1
#define SYMCRYPT_SCSTABLE_INTERLEAVE_SIZE   32
#define SYMCRYPT_SCSTABLE_GROUP_SIZE        4
typedef UINT64 SYMCRYPT_SCSTABLE_TYPE;
#else
#define SYMCRYPT_SCSTABLE_USE64             0
#define SYMCRYPT_SCSTABLE_INTERLEAVE_SIZE   16
#define SYMCRYPT_SCSTABLE_GROUP_SIZE        4
typedef UINT32 SYMCRYPT_SCSTABLE_TYPE;
#endif

UINT32
SYMCRYPT_CALL
SymCryptScsTableInit(
    _Out_   PSYMCRYPT_SCSTABLE  pScsTable,
            UINT32              nElements,
            UINT32              elementSize )
{
    UINT32  groupSize;
    UINT32  interleaveSize;
    UINT32  cbBuffer;

    SYMCRYPT_ASSERT( nElements > 0 );

#pragma warning( suppress: 4127 )       // conditional expression is constant
    if( SYMCRYPT_CPU_AMD64 && elementSize == 128 )
    {
        // Highly optimized assembler mode for 1024-bit entries for RSA-2048...
        interleaveSize = 128;
        groupSize = 1;
    } else {
        // Standard C implementation
        interleaveSize = SYMCRYPT_SCSTABLE_INTERLEAVE_SIZE;
        groupSize = SYMCRYPT_SCSTABLE_GROUP_SIZE;
    }

    // Right now, we limit ourselves to element sizes that are a multiple of the interleaveSize and
    // # elements that are a multiple of the group size.
    // We also limit ourselves to sensible input sizes
    SYMCRYPT_ASSERT( elementSize % interleaveSize == 0 && nElements % groupSize == 0 && (elementSize | nElements) < (1 << 16) && elementSize > 0 );

    cbBuffer = elementSize * nElements; // Each factor is < 2^16, so there is no overflow in the mul

    pScsTable->groupSize = groupSize;
    pScsTable->interleaveSize = interleaveSize;
    pScsTable->nElements = nElements;
    pScsTable->elementSize = elementSize;
    pScsTable->cbTableData = cbBuffer;
    pScsTable->pbTableData = NULL;

    return cbBuffer;
}

VOID
SYMCRYPT_CALL
SymCryptScsTableSetBuffer(
    _Inout_                             PSYMCRYPT_SCSTABLE  pScsTable,
    _Inout_updates_bytes_( cbBuffer )   PBYTE               pbBuffer,
                                        UINT32              cbBuffer )
{
    SYMCRYPT_ASSERT(cbBuffer >= pScsTable->cbTableData);
    UNREFERENCED_PARAMETER( cbBuffer );

    pScsTable->pbTableData = pbBuffer;
}


C_ASSERT( SYMCRYPT_SCSTABLE_INTERLEAVE_SIZE == 16 || SYMCRYPT_SCSTABLE_INTERLEAVE_SIZE == 32 );
// check that an interleave size is exactly 4 words
C_ASSERT( SYMCRYPT_SCSTABLE_INTERLEAVE_SIZE == 4 * sizeof( SYMCRYPT_SCSTABLE_TYPE ) );

VOID
SYMCRYPT_CALL
SymCryptScsTableStoreC(
    _Inout_                     PSYMCRYPT_SCSTABLE  pScsTable,
                                UINT32              iIndex,
    _In_reads_bytes_( cbData )  PCBYTE              pbData,
                                UINT32              cbData )
{
    UINT32 groupSize = SYMCRYPT_SCSTABLE_GROUP_SIZE;
    UINT32 interleaveSize = SYMCRYPT_SCSTABLE_INTERLEAVE_SIZE;
    UINT32 elementSize = pScsTable->elementSize;
    UINT32 groupOffset;

    SYMCRYPT_ASSERT( groupSize ==  pScsTable->groupSize );
    SYMCRYPT_ASSERT( interleaveSize == pScsTable->interleaveSize );

    SYMCRYPT_ASSERT( cbData == elementSize );
    UNREFERENCED_PARAMETER( cbData );

    SYMCRYPT_ASSERT(iIndex < pScsTable->nElements);

    groupOffset = iIndex % groupSize;

	// dcl - document why this can't be an integer overflow
    SYMCRYPT_SCSTABLE_TYPE * pDst = (SYMCRYPT_SCSTABLE_TYPE *) (pScsTable->pbTableData + (iIndex - groupOffset) * elementSize + groupOffset * interleaveSize);
    SYMCRYPT_SCSTABLE_TYPE * pSrc = (SYMCRYPT_SCSTABLE_TYPE *) pbData;

    UINT32 nInterleaves = elementSize / interleaveSize;

    do
    {
        pDst[0] = pSrc[0];
        pDst[1] = pSrc[1];
        pDst[2] = pSrc[2];
        pDst[3] = pSrc[3];

        pDst += interleaveSize * groupSize / sizeof( *pDst );
        pSrc += interleaveSize / sizeof( *pSrc );
        nInterleaves--;
    } while( nInterleaves > 0 );

}

#if SYMCRYPT_CPU_AMD64
VOID
SYMCRYPT_CALL
SymCryptScsTableStore128Xmm(
    _Inout_                     PSYMCRYPT_SCSTABLE  pScsTable,
                                UINT32              iIndex,
    _In_reads_bytes_( cbData )  PCBYTE              pbData,
                                UINT32              cbData )
{
    __m128i * pDst = (__m128i *) (pScsTable->pbTableData + iIndex * 128);
    __m128i * pSrc = (__m128i *) pbData;

    SYMCRYPT_ASSERT( cbData == 128 && pScsTable->elementSize == 128 && iIndex < pScsTable->nElements && pScsTable->groupSize == 1 );
    UNREFERENCED_PARAMETER( cbData );

    pDst[0] = pSrc[0];
    pDst[1] = pSrc[1];
    pDst[2] = pSrc[2];
    pDst[3] = pSrc[3];
    pDst[4] = pSrc[4];
    pDst[5] = pSrc[5];
    pDst[6] = pSrc[6];
    pDst[7] = pSrc[7];
}
#endif // AMD64

VOID
SYMCRYPT_CALL
SymCryptScsTableLoadC(
    _In_                        PSYMCRYPT_SCSTABLE  pScsTable,
                                UINT32              iIndex,
    _Out_writes_bytes_(cbData)  PBYTE               pbData,
                                UINT32              cbData )
{
    UINT32 groupSize = SYMCRYPT_SCSTABLE_GROUP_SIZE;
    UINT32 interleaveSize = SYMCRYPT_SCSTABLE_INTERLEAVE_SIZE;
    UINT32 elementSize = pScsTable->elementSize;

    SYMCRYPT_SCSTABLE_TYPE mask0, mask1, mask2, mask3;
    UINT32 i;
    UINT32 j;
    UINT32 nElements = pScsTable->nElements;

    const SYMCRYPT_SCSTABLE_TYPE * pSrc = (SYMCRYPT_SCSTABLE_TYPE *) pScsTable->pbTableData;
    SYMCRYPT_SCSTABLE_TYPE * pDst = (SYMCRYPT_SCSTABLE_TYPE *) pbData;
    SYMCRYPT_SCSTABLE_TYPE * pD;

    UINT32 nInterleaves = elementSize / interleaveSize;


    SYMCRYPT_ASSERT( groupSize ==  pScsTable->groupSize );
    SYMCRYPT_ASSERT( interleaveSize == pScsTable->interleaveSize );

    SYMCRYPT_ASSERT( cbData >= sizeof( SYMCRYPT_SCSTABLE_TYPE ) * SYMCRYPT_SCSTABLE_GROUP_SIZE );
    SYMCRYPT_ASSERT( cbData == pScsTable->elementSize );
    UNREFERENCED_PARAMETER( cbData );

#if SYMCRYPT_SCSTABLE_USE64
#define SCS_MASK_EQUAL32( _a, _b )  ( ~(UINT64) ((INT64) ((UINT64)0 - (_a ^ _b)) >> 32 ) )
#else
#define SCS_MASK_EQUAL32( _a, _b )  (SYMCRYPT_MASK32_EQ( _a, _b ))
#endif

    i = 0;

    mask0 = SCS_MASK_EQUAL32( i+0, iIndex );
    mask1 = SCS_MASK_EQUAL32( i+1, iIndex );
    mask2 = SCS_MASK_EQUAL32( i+2, iIndex );
    mask3 = SCS_MASK_EQUAL32( i+3, iIndex );

    j = nInterleaves;
    pD = pDst;

    do {
        pD[0] = (mask0 & pSrc[0]) | (mask1 & pSrc[4]) | (mask2 & pSrc[ 8]) | (mask3 & pSrc[12]);
        pD[1] = (mask0 & pSrc[1]) | (mask1 & pSrc[5]) | (mask2 & pSrc[ 9]) | (mask3 & pSrc[13]);
        pD[2] = (mask0 & pSrc[2]) | (mask1 & pSrc[6]) | (mask2 & pSrc[10]) | (mask3 & pSrc[14]);
        pD[3] = (mask0 & pSrc[3]) | (mask1 & pSrc[7]) | (mask2 & pSrc[11]) | (mask3 & pSrc[15]);
        pD += interleaveSize / sizeof( *pD );
        pSrc += interleaveSize * groupSize / sizeof( *pSrc );
        j--;
    } while( j > 0 );

    i += groupSize;

    while (i + groupSize <= nElements)
    {

        mask0 = SCS_MASK_EQUAL32( i+0, iIndex );
        mask1 = SCS_MASK_EQUAL32( i+1, iIndex );
        mask2 = SCS_MASK_EQUAL32( i+2, iIndex );
        mask3 = SCS_MASK_EQUAL32( i+3, iIndex );

        j = nInterleaves;
        pD = pDst;

        do {
            pD[0] |= (mask0 & pSrc[0]) | (mask1 & pSrc[4]) | (mask2 & pSrc[ 8]) | (mask3 & pSrc[12]);
            pD[1] |= (mask0 & pSrc[1]) | (mask1 & pSrc[5]) | (mask2 & pSrc[ 9]) | (mask3 & pSrc[13]);
            pD[2] |= (mask0 & pSrc[2]) | (mask1 & pSrc[6]) | (mask2 & pSrc[10]) | (mask3 & pSrc[14]);
            pD[3] |= (mask0 & pSrc[3]) | (mask1 & pSrc[7]) | (mask2 & pSrc[11]) | (mask3 & pSrc[15]);
            pD += interleaveSize / sizeof( *pD );
            pSrc += interleaveSize * groupSize / sizeof( *pSrc );
            j--;
        } while( j > 0 );

        i += groupSize;
    }
}

#if SYMCRYPT_CPU_AMD64
VOID
SYMCRYPT_CALL
SymCryptScsTableLoad128Xmm(
    _In_                        PSYMCRYPT_SCSTABLE  pScsTable,
                                UINT32              iIndex,
    _Out_writes_bytes_(cbData)  PBYTE               pbData,
                                UINT32              cbData )
{
    UINT32 nElements = pScsTable->nElements;

    __m128i R0, R1, R2, R3, R4, R5, R6, R7;
    __m128i T0, T1;

    __m128i Count = _mm_setzero_si128();
    __m128i Ones = _mm_set_epi32( 1, 1, 1, 1 );
    __m128i Entry = _mm_set_epi32( iIndex, iIndex, iIndex, iIndex );
    __m128i Mask;
    __m128i * pSrc = (__m128i *) pScsTable->pbTableData;
    __m128i * pDst = (__m128i *) pbData;

    SYMCRYPT_ASSERT( cbData == 128 && pScsTable->elementSize == 128 && iIndex < pScsTable->nElements && pScsTable->groupSize == 1 );
    UNREFERENCED_PARAMETER( cbData );

    Mask = _mm_cmpeq_epi32( Count, Entry );
    Count = _mm_add_epi32( Count, Ones );

    R0 = _mm_and_si128( Mask, pSrc[0] );
    R1 = _mm_and_si128( Mask, pSrc[1] );
    R2 = _mm_and_si128( Mask, pSrc[2] );
    R3 = _mm_and_si128( Mask, pSrc[3] );
    R4 = _mm_and_si128( Mask, pSrc[4] );
    R5 = _mm_and_si128( Mask, pSrc[5] );
    R6 = _mm_and_si128( Mask, pSrc[6] );
    R7 = _mm_and_si128( Mask, pSrc[7] );

    pSrc += 8;

    while( --nElements > 0 )
    {
        Mask = _mm_cmpeq_epi32( Count, Entry );
        Count = _mm_add_epi32( Count, Ones );

        T0 = _mm_and_si128( Mask, pSrc[0] );        R0 = _mm_or_si128( R0, T0 );
        T1 = _mm_and_si128( Mask, pSrc[1] );        R1 = _mm_or_si128( R1, T1 );
        T0 = _mm_and_si128( Mask, pSrc[2] );        R2 = _mm_or_si128( R2, T0 );
        T1 = _mm_and_si128( Mask, pSrc[3] );        R3 = _mm_or_si128( R3, T1 );
        T0 = _mm_and_si128( Mask, pSrc[4] );        R4 = _mm_or_si128( R4, T0 );
        T1 = _mm_and_si128( Mask, pSrc[5] );        R5 = _mm_or_si128( R5, T1 );
        T0 = _mm_and_si128( Mask, pSrc[6] );        R6 = _mm_or_si128( R6, T0 );
        T1 = _mm_and_si128( Mask, pSrc[7] );        R7 = _mm_or_si128( R7, T1 );
        pSrc += 8;
    }

    pDst[0] = R0;
    pDst[1] = R1;
    pDst[2] = R2;
    pDst[3] = R3;
    pDst[4] = R4;
    pDst[5] = R5;
    pDst[6] = R6;
    pDst[7] = R7;
}
#endif // AMD64

VOID
SYMCRYPT_CALL
SymCryptScsTableStore(
    _Inout_                     PSYMCRYPT_SCSTABLE  pScsTable,
                                UINT32              iIndex,
    _In_reads_bytes_( cbData )  PCBYTE              pbData,
                                UINT32              cbData )
{
#if SYMCRYPT_CPU_AMD64

    if( pScsTable->elementSize == 128 )
    {
        SymCryptScsTableStore128Xmm( pScsTable, iIndex, pbData, cbData );
    } else {
        SymCryptScsTableStoreC( pScsTable, iIndex, pbData, cbData );
    }

#else

    SymCryptScsTableStoreC( pScsTable, iIndex, pbData, cbData );

#endif
}

VOID
SYMCRYPT_CALL
SymCryptScsTableLoad(
    _In_                        PSYMCRYPT_SCSTABLE  pScsTable,
                                UINT32              iIndex,
    _Out_writes_bytes_(cbData)  PBYTE               pbData,
                                UINT32              cbData )
{
    // This is the side-channel safe routine

#if SYMCRYPT_CPU_AMD64

    if( pScsTable->elementSize == 128 )
    {
        SymCryptScsTableLoad128Xmm( pScsTable, iIndex, pbData, cbData );
    } else {
        SymCryptScsTableLoadC( pScsTable, iIndex, pbData, cbData );
    }

#else

    SymCryptScsTableLoadC( pScsTable, iIndex, pbData, cbData );

#endif
}

VOID
SYMCRYPT_CALL
SymCryptScsTableWipe(
    _Inout_ PSYMCRYPT_SCSTABLE pScsTable )
{
    SymCryptWipe( pScsTable->pbTableData, pScsTable->cbTableData );
}
