//
// scsTools.c   Support tools for writing side-channel safe code
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// This code needs to process data in words, and we'd like to use 32-bit words on 32-bit
// architectures and 64-bit words on 64-bit architectures. So we use NATIVE_UINT & friends.
//

// Buffer limits for SymCryptScsRotateBuffer
#define MIN_BUFFER_SIZE (32)

//
// Masking functions
// Masking functions can be more efficient if the inputs are restricted to values that can
// be represented in the signed data types.
// This is why we have some functions that take 31-bit inputs.
//

// 31-bit inputs

UINT32
SYMCRYPT_CALL
SymCryptMask32IsNonzeroU31( UINT32 v )
{
    SYMCRYPT_ASSERT( v < (1UL<<31) );
    return (-(INT32) v) >> 31;
}

UINT32
SYMCRYPT_CALL
SymCryptMask32IsZeroU31( UINT32 v )
{
    return ~SymCryptMask32IsNonzeroU31( v );
}

UINT32
SYMCRYPT_CALL
SymCryptMask32NeqU31( UINT32 a, UINT32 b )
{
    SYMCRYPT_ASSERT( a < (1UL<<31) );
    SYMCRYPT_ASSERT( b < (1UL<<31) );

    return SymCryptMask32IsNonzeroU31( a ^ b );
}

UINT32
SYMCRYPT_CALL
SymCryptMask32LtU31( UINT32 a, UINT32 b )
{
    SYMCRYPT_ASSERT( a < (1UL<<31) );
    SYMCRYPT_ASSERT( b < (1UL<<31) );

    // Casting to INT32 is defined as a and b are < 2^31
    return ((INT32) a - (INT32) b) >> 31;
}


// 32-bit inputs

UINT32
SYMCRYPT_CALL
SymCryptMask32EqU32( UINT32 a, UINT32 b )
{
    return ~(UINT32) ( (-(INT64)(a^b)) >> 32);
}


// Other helper functions
SIZE_T
SYMCRYPT_CALL
SymCryptRoundUpPow2Sizet( SIZE_T v )
{
    SIZE_T res;

    SYMCRYPT_ASSERT( v <= (SIZE_T_MAX / 2) + 1);
    // If v is very large, then the result res might overflow.
    // As SIZE_T is an unsigned type, the overflow is defined to
    // be modulo 2^n for some n, and therefore we'll get res==0
    // which will terminate the loop.

    res = 1;
    while( res < v )
    {
        res += res;

        // Catch any overflows; should never happen but break to avoid infinite loop
        if( res == 0 )
        {
            break;
        }
    }

    return res;
}


//
// Copy data
//

VOID
SYMCRYPT_CALL
SymCryptScsCopy(
    _In_reads_( cbDst )     PCBYTE  pbSrc,
                            SIZE_T  cbSrc,
    _Out_writes_( cbDst )   PBYTE   pbDst,
                            SIZE_T  cbDst )
// Copy cbSrc bytes of pbSrc into pbDst without revealing cbSrc
// through side channels.
// - pbSrc/cbSrc: buffer to copy data from
// - pbDst/cbDst: buffer that receives the data
// Equivalent to:
//      n = min( cbSrc, cbDst )
//      pbDst[ 0.. n-1 ] = pbSrc[ 0 .. n - 1 ]
// cbSrc is protected from side-channels; cbDst is public.
// Note that pbSrc must be cbDst bytes long, not cbSrc bytes.
{
    UINT32 i;

    SYMCRYPT_ASSERT( cbSrc <= (1UL << 31) && cbDst <= (1UL << 31) );

    // Loop over the destination buffer and update each byte with the source data (if appropriate)
    // We round-robin loop over the source buffer
    for( i = 0; i < cbDst; i++ )
    {
        pbDst[ i ] ^= (pbSrc[ i ] ^ pbDst[ i ]) & SymCryptMask32LtU31( i, (UINT32) cbSrc );
    }
}


//
// Buffer rotation
// To recover a message from an encoding with variable data position we have to do a copy from a
// variable memory location. But our memory access pattern cannot depend on the secret location.
// This code rotates a given buffer by a variable # bytes without revealing the shift amount.
//
// For efficiency we do this using NATIVE_UINT values so that we get the best performance on each platform.
//
// The first step is to rotate the array between 0 and NATIVE_BYTES-1 bytes to get the proper word alignment.
// After that we only have to rotate the words.
// We do this using a sequence of swaps.
// Notation:
//  W[i]    array of words, 0 <= i < n where n is the # words, a power of 2.
//  s       Rotation amount (to the left). The value in W[s] at the start should appear in W[0] at the end.
//
// We use masked swaps as they seem to be more efficient then masked multiplexers.
// We can split this problem down recursively
//
// Function Rotate( W, n, s)
//  - Rotate W[0..n/2-1] by s mod n/2
//  - Rotate W[n/2..n-1] by s mod n/2
// for i in 0..n/2-1:
//      swap W[i] and W[i+n/2] if (i+s) % n >= n/2
//
// After the two half-sized rotates, each word is in the right position modulo n/2, so all that needs to be
// done in possibly swap (W[i],W[i+n/2]) pairs.
// Let W' be the array after the half-sized rotates. We have
// W'[i] = W[ (i + s) % (n/2) ] for i in 0..n/2
// In the final array W'' we should have W''[i] = W[ (i+s)%n ]
// So W''[i] = W'[i] when (i+s) % n = (i+s) % n/2 which is equivalent to (i+s)%n < n/2.
//
// We turn this into a non-recursive algorithm.
// First we do rotations on 2 words,
// then the fixups to make it 4-word rotations,
// then on to 8-words, etc.
// At each level we compute the masks for the swaps once, and re-use them for each copy
// As a further optimization, we merge the 1st and 2nd pass into one to reduce the # read/writes
//
// We avoid using / and % throughout to avoid any time-dependent instructions.
//

VOID
SYMCRYPT_CALL
SymCryptScsRotateBuffer(
    _Inout_updates_( cbBuffer ) PBYTE   pbBuffer,
                                SIZE_T  cbBuffer,
                                SIZE_T  lshift )
{
    NATIVE_UINT * pBuf;
    UINT32 n;
    UINT32 a;
    UINT32 b;
    UINT32 i;
    UINT32 j;
    UINT32 blockSize;
    UINT32 blockSizeLog;
    UINT32 blockSizeLimit;

    NATIVE_UINT V;
    NATIVE_UINT T;
    NATIVE_UINT A;
    NATIVE_UINT B;
    NATIVE_UINT C;
    NATIVE_UINT D;
    NATIVE_UINT M;
    NATIVE_UINT M0;
    NATIVE_UINT M1;

    NATIVE_UINT Mask[ 16 ];      // Size must be a power of 2

    SYMCRYPT_ASSERT( (cbBuffer & (cbBuffer - 1)) == 0 && cbBuffer >= MIN_BUFFER_SIZE );
    SYMCRYPT_ASSERT( lshift < cbBuffer );

    pBuf = (NATIVE_UINT *) pbBuffer;
    n = (UINT32)cbBuffer / NATIVE_BYTES;

    // First a rotate left by lshift % NATIVE_BYTES
    // This is more complex because shifting by NATIVE_BITS is not a defined operation, and behavior is different
    // on different CPUs.

    // Compute the shift amounts & mask
    // M = 0 if lshift % NATIVE_BYTES == 0, -1 otherwise
    a = 8 * (lshift & (NATIVE_BYTES-1));            // Core shift
    M = (-(NATIVE_INT)a) >> (NATIVE_BITS - 1);      // mask
    b = (NATIVE_BITS - a) & (UINT32) M;         // complementary shift, or 0 if it would be equal to NATIVE_BITS

    i = n;
    V = pBuf[0];
    do{
        // Loop invariant: i > 0 && v = pBuf[i] from before any changes;
        i--;
        T = pBuf[i];
        pBuf[i] = T >> a | ((V << b) & M);
        V = T;
    } while( i > 0 );

    // Now that the rotation is word-aligned, we can start our word rotation
    lshift >>= NATIVE_BYTES_LOG2;           // convert to # words to rotate.

    // We know we have at least 4 words, so we start with a pass do do 4-word rotations
    SYMCRYPT_ASSERT( n >= 4 );

    M = -(NATIVE_INT)(lshift & 1);
    M0 = -(NATIVE_INT)( ((lshift + 0) >> 1) & 1 );      // s + 0 mod 4 >= 2
    M1 = -(NATIVE_INT)( ((lshift + 1) >> 1) & 1 );      // s + 1 mod 4 >= 2

    for( i=0; i<n; i+=4 )
    {
        A = pBuf[i];
        B = pBuf[i+1];
        C = pBuf[i+2];
        D = pBuf[i+3];

        T = (A ^ B) & M;
        A ^= T;
        B ^= T;

        T = (C ^ D) & M;
        C ^= T;
        D ^= T;

        T = (A ^ C) & M0;
        A ^= T;
        C ^= T;

        T = (B ^ D) & M1;
        B ^= T;
        D ^= T;

        pBuf[i  ] = A;
        pBuf[i+1] = B;
        pBuf[i+2] = C;
        pBuf[i+3] = D;
    }

    // Do the swaps using the mask array
    blockSize = 4;  // size of rotated blocks
    blockSizeLog = 2;

    //
    // Using the mask array is beneficial as long as the array is used twice or more
    // Each swap loop processes 2 * blockSize of data, so the block size should never
    // be larger than n/4
    blockSizeLimit = SYMCRYPT_MIN( SYMCRYPT_ARRAY_SIZE( Mask ), n/4 );
    while( blockSize <= blockSizeLimit )
    {
        // Compute the masks for this level
        for( i=0; i<blockSize; i++ )
        {
            Mask[i] =-(NATIVE_INT)( ((i + lshift) >> blockSizeLog) & 1);
        }

        // Now swap the elements of pairs of blocks according to the masks
        for( i=0; i < n; i += 2 * blockSize )
        {
            for( j=0; j < blockSize; j++ )
            {
                A = pBuf[ i + j ];
                B = pBuf[ i + j + blockSize ];
                T = (A ^ B) & Mask[j];
                A ^= T;
                B ^= T;
                pBuf[ i + j ] = A;
                pBuf[ i + j + blockSize ] = B;
            }
        }
        blockSize *= 2;
        blockSizeLog += 1;
    }

    // Do the rest without using a mask array, either because we are only
    // going to use each mask value once, or because we don't have a large-enough
    // array
    while( blockSize < n  )
    {
        // Now swap the elements of pairs of blocks according to the masks
        for( i=0; i < n; i += 2 * blockSize )
        {
            for( j=0; j < blockSize; j++ )
            {
                M = -(NATIVE_INT)( ((j + lshift) >> blockSizeLog) & 1);
                A = pBuf[ i + j ];
                B = pBuf[ i + j + blockSize ];
                T = (A ^ B) & M;
                A ^= T;
                B ^= T;
                pBuf[ i + j ] = A;
                pBuf[ i + j + blockSize ] = B;
            }
        }
        blockSize *= 2;
        blockSizeLog += 1;
    }

}


//
// Map values in a side-channel safe way, typically used for mapping error codes.
//
// (pcMap, nMap) point to an array of nMap entries of type SYMCRYPT_UINT32_MAP;
// each entry specifies a single mapping. If u32Input matches the
// 'from' field, the return value will be the 'to' field value.
// If u32Input is not equal to any 'from' field values, the return value is u32Default.
// Both u32Input and the return value are treated as secrets w.r.t. side channels.
//
// If multiple map entries have the same 'from' field value, then the return value
// is one of the several 'to' field values; which one is not defined.
//
// This function is particularly useful when mapping error codes in situations where
// the actual error cannot be revealed through side channels.
//

UINT32
SYMCRYPT_CALL
SymCryptMapUint32(
                        UINT32                  u32Input,
                        UINT32                  u32Default,
    _In_reads_(nMap)    PCSYMCRYPT_UINT32_MAP   pcMap,
                        SIZE_T                  nMap)
{
    UINT32 mask;
    UINT32 u32Output    = u32Default;

    for (SIZE_T i = 0; i < nMap; ++i)
    {
        mask = SymCryptMask32EqU32(u32Input, pcMap[i].from);
        u32Output ^= (u32Output ^ pcMap[i].to) & mask;
    }

    return u32Output;
}
