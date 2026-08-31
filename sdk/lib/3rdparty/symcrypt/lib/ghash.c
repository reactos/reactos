//
// GHASH.c
//
// Implementation of the NIST SP800-38D GHASH function which is the
// core authentication function for the GCM and GMAC modes.
//
// This implementation was done by Niels Ferguson for the RSA32.lib library in 2008,
// and adapted to the SymCrypt library in 2009.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"
#include "ghash_definitions.h"

//////////////////////////////////////////////////////////////////////////////
// Platform-independent code
//

//
// GHashExpandKeyC
// Generic GHash key expansion routine, works on all platforms.
// This function computes a table of H, Hx, Hx^2, Hx^3, ..., Hx^127
//
VOID
SYMCRYPT_CALL
SymCryptGHashExpandKeyC(
    _Out_writes_( SYMCRYPT_GF128_FIELD_SIZE )   PSYMCRYPT_GF128_ELEMENT expandedKey,
    _In_reads_( SYMCRYPT_GF128_BLOCK_SIZE )    PCBYTE                  pH )
{
    UINT64 H0, H1, t;
    UINT32 i;

    //
    // (H1, H0) form a 128-bit integer, H1 is the upper part, H0 the lower part.
    // Convert pH[] to (H1, H0) using MSByte first convention.
    //
    H1 = SYMCRYPT_LOAD_MSBFIRST64( &pH[0] );
    H0 = SYMCRYPT_LOAD_MSBFIRST64( &pH[8] );

    for( i=0; i<SYMCRYPT_GF128_FIELD_SIZE; i++ )
    {
        expandedKey[i].ull[0] = H0;
        expandedKey[i].ull[1] = H1;
        //
        // Multiply (H1,H0) by x in the GF(2^128) field using the field encoding from SP800-38D
        //
        t =  UINT64_NEG(H0 & 1) & ((UINT64)GF128_FIELD_R_BYTE << (8 * ( sizeof( UINT64 ) - 1 )) ) ;
        H0 = (H0 >> 1) | (H1 << 63);
        H1 = (H1 >> 1) ^ t;
    }
}


//
// GHashAppendDataC
// Generic GHash routine, works on all platforms.
//
VOID
SYMCRYPT_CALL
SymCryptGHashAppendDataC(
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE )  PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                  PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                     PCBYTE                      pbData,
                                             SIZE_T                      cbData )
{
    UINT64 R0, R1;
    UINT64 mask;
    SYMCRYPT_ALIGN UINT32 state32[4];
    UINT32 t;
    int i,j;
    while( cbData >= SYMCRYPT_GF128_BLOCK_SIZE )
    {
        R0 = R1 = 0;

        //
        // We have two nested loops so that we can do most of our operations
        // on 32-bit words. 64-bit rotates/shifts can be really slow on a 32-bit CPU.
        // On AMD64 we use the XMM version which is much faster.
        //
        state32[0] = (UINT32)pState->ull[0];
        state32[1] = (UINT32)(pState->ull[0] >> 32);
        state32[2] = (UINT32)pState->ull[1];
        state32[3] = (UINT32)(pState->ull[1] >> 32);
        for( i=0; i<4; i++ )
        {
            t = SYMCRYPT_LOAD_MSBFIRST32( &pbData[4*i] ) ^ state32[3-i];
            for( j=31; j>=0; j-- )
            {
                mask = (UINT64)( -(INT64)(t & 1 ));
                R0 ^= expandedKeyTable[32*i+j].ull[0] & mask;
                R1 ^= expandedKeyTable[32*i+j].ull[1] & mask;
                t >>= 1;
            }
        }
        pState->ull[0] = R0;
        pState->ull[1] = R1;
        pbData += SYMCRYPT_GF128_BLOCK_SIZE;
        cbData -= SYMCRYPT_GF128_BLOCK_SIZE;
    }

    SymCryptWipeKnownSize( state32, sizeof( state32 ) );
}


VOID
SYMCRYPT_CALL
SymCryptGHashResult(
    _In_                                        PCSYMCRYPT_GF128_ELEMENT    pState,
    _Out_writes_( SYMCRYPT_GF128_BLOCK_SIZE )   PBYTE                       pbResult )
{
    SYMCRYPT_STORE_MSBFIRST64( pbResult    , pState->ull[1] );
    SYMCRYPT_STORE_MSBFIRST64( pbResult + 8, pState->ull[0] );
}

////////////////////////////////////////////////////////////////////////////////////////////
// XMM code
//

VOID
SYMCRYPT_CALL
SymCryptGHashExpandKeyXmm(
    _Out_writes_( SYMCRYPT_GF128_FIELD_SIZE )   PSYMCRYPT_GF128_ELEMENT expandedKey,
    _In_reads_( SYMCRYPT_GF128_BLOCK_SIZE )    PCBYTE                  pH )
{
    //
    // We use the same layout for XMM code as we did for C code, so we can use the same key
    // expansion code.
    // Improvement: we can add an expansion routine that uses the XMM registers for speed.
    //

    SymCryptGHashExpandKeyC( expandedKey, pH );
}

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("sse2"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("sse2")
#endif

//
// The XMM-based GHash append data function, only on AMD64 & X86
//
VOID
SYMCRYPT_CALL
SymCryptGHashAppendDataXmm(
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbData,
                                            SIZE_T                      cbData )
{
    __m128i R;
    __m128i cmpValue;
    __m128i mask;
    __m128i T;
    __m128i tmp;

    PCSYMCRYPT_GF128_ELEMENT   p;
    PCSYMCRYPT_GF128_ELEMENT   pLimit;
    UINT32 t;
    int i;

    cmpValue = _mm_setzero_si128();             // cmpValue = 0

    while( cbData >= SYMCRYPT_GF128_BLOCK_SIZE )
    {
        R = _mm_setzero_si128();

        //
        // The amd64 compiler can't optimize array indices in a loop where
        // you use _mm intrinsics,
        // so we do all the pointer arithmetic for the compiler.
        //
        p = &expandedKeyTable[0];
        pLimit = &expandedKeyTable[32];

        for( i=0; i<4; i++ )
        {
            //
            // Set up our XMM register with 4 identical 32-bit integers so that
            // we can generate the mask from the individual bits of the 32-bit value.
            // Note the use of tmp; if we assign directly to the fields of T the
            // compiler no longer caches T in an XMM register, which is bad.
            //
            // There are XMM instructions where we can do the duplication in the XMM
            // registers, but they require SSE3 support, and this code only requires
            // SSE2. As the inner loop consumes most of the time, it isn't worth
            // using the SSE3 instructions.
            //
            // Note that accessing the state as an array of UINT32s depends on the
            // endianness of the CPU, but this is XMM code that only runs on
            // little endian machines.
            //
            t = SYMCRYPT_LOAD_MSBFIRST32( &pbData[4*i] ) ^ pState->ul[3-i];
            tmp = _mm_set_epi32(t, t, t, t);

            T = tmp;
            while( p < pLimit )
            {
                //
                // p and plimit are always at indexes that are multiples of 4 from
                // the start of the array.
                // We need to explain to prefast that this means that p <= pLimit - 4
                //
                SYMCRYPT_ASSERT( p <= pLimit - 4 );

                mask = _mm_cmpgt_epi32( cmpValue, T );
                T = _mm_add_epi32( T, T );
                mask = _mm_and_si128( mask, p[0].m128i );
                R = _mm_xor_si128( R, mask );

                mask = _mm_cmpgt_epi32( cmpValue, T );
                T = _mm_add_epi32( T, T );
                mask = _mm_and_si128( mask, p[1].m128i );
                R = _mm_xor_si128( R, mask );

                mask = _mm_cmpgt_epi32( cmpValue, T );
                T = _mm_add_epi32( T, T );
                mask = _mm_and_si128( mask, p[2].m128i );
                R = _mm_xor_si128( R, mask );

                mask = _mm_cmpgt_epi32( cmpValue, T );
                T = _mm_add_epi32( T, T );
                mask = _mm_and_si128( mask, p[3].m128i );
                R = _mm_xor_si128( R, mask );

                p += 4;
            }
            pLimit += 32;
        }

        pState->m128i = R;
        pbData += SYMCRYPT_GF128_BLOCK_SIZE;
        cbData -= SYMCRYPT_GF128_BLOCK_SIZE;
    }
}

#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif

#endif

#if SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64
//
// The NEON-based GHash append data function, only on ARM & ARM64
//
VOID
SYMCRYPT_CALL
SymCryptGHashAppendDataNeon(
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE )     PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                     PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                        PCBYTE                      pbData,
                                                SIZE_T                      cbData )
{
    // Room for improvement: replace non-crypto NEON code below, based on a bit by bit lookup with
    // pmull on 8b elements - 8x(8bx8b) -> 8x(16b) pmull is NEON instruction since Armv7
    //
    // When properly unrolled:
    // 1 (64bx64b -> 128b) pmull instruction and 1 eor instruction can be replaced by
    // 8 (8x(8bx8b) -> 8x(16b)) pmull instructions and 8 eor instructions
    // so each 128b of data could be processed by less than 64 instructions (using karatsuba)
    // rather than ~512 instructions (bit by bit)
    //
    // Not a priority, expect that AES-GCM performance will be dominated by AES on these platforms

    __n128 R;
    __n128 cmpValue;
    __n128 mask;
    __n128 T;

    PCSYMCRYPT_GF128_ELEMENT   p;
    PCSYMCRYPT_GF128_ELEMENT   pLimit;
    UINT32 t;
    int i;

    cmpValue = vdupq_n_u32(0);             // cmpValue = 0

    while( cbData >= SYMCRYPT_GF128_BLOCK_SIZE )
    {
        R = cmpValue;

        //
        // Do all the pointer arithmetic for the compiler.
        //
        p = &expandedKeyTable[0];
        pLimit = &expandedKeyTable[32];

        for( i=0; i<4; i++ )
        {
            //
            // Set up our XMM register with 4 identical 32-bit integers so that
            // we can generate the mask from the individual bits of the 32-bit value.
            // Note the use of tmp; if we assign directly to the fields of T the
            // compiler no longer caches T in an XMM register, which is bad.
            //
            // Note that accessing the state as an array of UINT32s depends on the
            // endianness of the CPU, but Arm code is always expected to execute in
            // little endian mode.
            //
            t = SYMCRYPT_LOAD_MSBFIRST32( &pbData[4*i] ) ^ pState->ul[3-i];
            T = vdupq_n_u32( t );

            while( p < pLimit )
            {
                //
                // p and plimit are always at indexes that are multiples of 4 from
                // the start of the array.
                // We need to explain to prefast that this means that p <= pLimit - 4
                //
                SYMCRYPT_ASSERT( p <= pLimit - 4 );

                mask = vcgtq_s32( cmpValue, T );
                T = vaddq_u32( T, T );
                mask = vandq_u32( mask, p[0].n128 );
                R = veorq_u32( R, mask );

                mask = vcgtq_s32( cmpValue, T );
                T = vaddq_u32( T, T );
                mask = vandq_u32( mask, p[1].n128 );
                R = veorq_u32( R, mask );

                mask = vcgtq_s32( cmpValue, T );
                T = vaddq_u32( T, T );
                mask = vandq_u32( mask, p[2].n128 );
                R = veorq_u32( R, mask );

                mask = vcgtq_s32( cmpValue, T );
                T = vaddq_u32( T, T );
                mask = vandq_u32( mask, p[3].n128 );
                R = veorq_u32( R, mask );

                p += 4;
            }
            pLimit += 32;
        }

        pState->n128 = R;
        pbData += SYMCRYPT_GF128_BLOCK_SIZE;
        cbData -= SYMCRYPT_GF128_BLOCK_SIZE;
    }
}
#endif


//////////////////////////////////////////////////////////////////////////////////////
// Pclmulqdq implementation
//

/*
GHASH GF(2^128) multiplication using PCLMULQDQ

The GF(2^128) field used in GHASH is GF(2)[x]/p(x) where p(x) is the primitive polynomial
    x^128 + x^7 + x^2 + x + 1

Notation: We use the standard mathematical notation '+' for the addition in the field,
which corresponds to a xor of the bits.

Multiplication:
Given two field elements A and B (represented as 128-bit values),
we first compute the polynomial product
    (C,D) := A * B
where C and D are also 128-bit values.

The PCLMULQDQ instruction performs a 64 x 64 -> 128 bit carryless multiplication.
To multiply 128-bit values we write A = (A1, A0) and B = (B1, B0) in two 64-bit halves.

The schoolbook multiplication is computed by
    (C, D) = (A1 * B1)x^128 + (A1 * B0 + A0 * B1)x^64 + (A0 * B0)
This require four PCLMULQDQ instructions. The middle 128-bit result has to be shifted
left and right, and each half added to the upper and lower 128-bit result to get (C,D).

Alternatively, the middle 128-bit intermediate result be computed using Karatsuba:
    (A1*B0 + A0*B1) = (A1 + A0) * (B1 + B0) + (A1*B1) + (A0*B0)
This requires only one PCLMULQDQ instruction to multiply (A1 + A0) by (B1 + B0)
as the other two products are already computed.
Whether this is faster depends on the relative speed of shift/xor verses PCLMULQDQ.

Both multiplication algorithms produce three 128-bit intermediate results (R1, Rmid, R0),
with the full result defined by R1 x^128 + Rmid x^64 + R0.
If we do Multiply-Accumulate then we can accumulate the three 128-bit intermediate results
directly. As there are no carries, there is no overflow, and the combining of the three
intermediate results into a 256-bit result can be shared amongst all multiplications.


Modulo reduction:
We use << and >> to denote shifts on 128-bit values.
The modulo reduction can now be done as follows:
given a 256-bit value (C,D) representing C x^128 + D we compute
    (T1,T0) := C + C*x + C * x^2 + C * x^7
    R := D + T0 + T1 + (T1 << 1) + (T1 << 2) + (T1 << 7)

(T1,T0) is just the value C x^128 reduced one step modulo p(x).The value T1 is at most 7 bits,
so in the next step the reduction, which computes the result R, is easy. The
expression T1 + (T1 << 1) + (T1 << 2) + (T1 << 7) is just T1 * x^128 reduced modulo p(x).

Let's first get rid of the polynomial arithmetic and write this completely using shifts on
128-bit values.

T0 := C + (C << 1) + (C << 2) + (C << 7)
T1 := (C >> 127) + (C >> 126) + (C >> 121)
R := D + T0 + T1  + (T1 << 1) + (T1 << 2) + (T1 << 7)

We can optimize this by rewriting the equations

T2 := T1 + C
    = C + (C>>127) + (C>>126) + (C>>121)
R   = D + T0 + T1  + (T1 << 1) + (T1 << 2) + (T1 << 7)
    = D + C + (C << 1) + (C << 2) + (C << 7) + T1  + (T1 << 1) + (T1 << 2) + (T1 << 7)
    = D + T2 + (T2 << 1) + (T2 << 2) + (T2 << 7)

Thus
T2  = C + (C>>127) + (C>>126) + (C>>121)
R   = D + T2 + (T2 << 1) + (T2 << 2) + (T2 << 7)

Gets the right result and uses only 6 shifts.

The SSE instruction set does not implement bit-shifts of 128-bit values. Instead, we will
use bit-shifts of the 32-bit subvalues, and byte shifts (shifts by a multiple of 8 bits)
on the full 128-bit values.
We use the <<<< and >>>> operators to denote shifts on 32-bit subwords.

We can now do the modulo reduction by

t1 := (C >> 127) = (C >>>> 31) >> 96
t2 := (C >> 126) = (C >>>> 30) >> 96
t3 := (C >> 121) = (C >>>> 25) >> 96
T2 = C + t1 + t2 + t3

left-shifts in the computation of R are a bit more involved as we have to move bits from
one subword to the next

u1 := (T2 << 1) = (T2 <<<< 1) + ((T2 >>>> 31) << 32)
u2 := (T2 << 2) = (T2 <<<< 2) + ((T2 >>>> 30) << 32)
u3 := (T2 << 7) = (T2 <<<< 7) + ((T2 >>>> 25) << 32)
R = D + T2 + u1 + u2 + u3

We can eliminate some common subexpressions. For any k we have
(T2 >>>> k) = ((C + r) >>>> k)
where r is a 7-bit value. If k>7 then this is equal to (C >>>> k). This means that
the value (T2 >>>> 31) is equal to (C >>>> 31) so we don't have to compute it again.

So we can rewrite our formulas as
t4 := (C >>>> 31)
t5 := (C >>>> 30)
t6 := (C >>>> 25)
ts = t4 + t5 + t6
T2 = C + (ts >> 96)

Note that ts = (C >>>> 31) + (C >>>> 30) + (C >>>> 25)
which is equal to (T2 >>>> 31) + (T2 >>>> 30) + (T2 >>>> 25)

R = D + T2 + u1 + u2 + u3
  = D + T2 + (T2 <<<< 1) + (T2 <<<< 2) + (T2 <<<< 7) + (ts << 32)

All together, we can do the modulo reduction using the following formulas

ts := (C >>>> 31) + (C >>>> 30) + (C >>>> 25)
T2 := C + (ts >> 96)
R = D + T2 + (T2 <<<< 1) + (T2 <<<< 2) + (T2 <<<< 7) + (ts << 32)

Using a total of 16 operations. (6 subword shifts, 2 byte shifts, and 8 additions)

Reversed bit order:
There is one more complication. GHASH uses the bits in the reverse order from normal representation.
The bits b_0, b_1, ..., b_127 represent the polynomial b_0 + b_1 * x + ... + b_127 * x^127.
This means that the most significant bit in each byte is actually the least significant bit in the
polynomial.

SSE CPUs use the LSBFirst convention. This means that the bits b_0, b_1, ..., b_127 of the polynomial
end up at positions 7, 6, 5, ..., 1, 0, 15, 14, ..., 9, 8, 23, 22, ... of our XMM register.
This is obviously not a useful representation to do arithmetic in.
The first step is to BSWAP the value so that the bits appear in pure reverse order.
That is at least algebraically useful.

To compute the multiplication we use the fact that GF(2)[x] multiplication has no carries and
thus no preference for bit order. After the BSWAP we don't have the values A and B, but rather
rev(A) and rev(B) where rev() is a function that reverses the bit order. We can now compute

  rev(A) * rev(B) = rev( A*B ) >> 1

where the shift operator is on the 256-bit product.

The modulo reduction remains the same, except that we change all the shifts to be the other direction.

This gives us finally the outline of our multiplication:

- Apply BSWAP to all values loaded from memory.
    A := BSWAP( Abytes )
    B := BSWAP( Bbytes )
- Compute the 256-bit product, possibly using Karatsuba.
    (P1, P0) := A * B   // 128x128 carryless multiplication
- Shift the result left one bit.
    (Q1, Q0) := (P1, P0) << 1
    which is computed as
        Q0 = (P0 <<<< 1) + (P0 >>>> 31) << 32
        Q1 = (P1 <<<< 1) + (P1 >>>> 31) << 32 + (P0 >>>> 31) >> 96
- Perform the modulo reduction, with reversed bit order
    ts := (Q0 <<<< 31) + (Q0 <<<< 30) + (Q0 <<<< 25)
    T2 := Q0 + (ts << 96)
    R = Q1 + T2 + (T2 >>>> 1) + (T2 >>>> 2) + (T2 >>>> 7) + (ts >> 32)

Future work:
It might be possible to construct a faster solution by merging the leftshift of (P1,P0)
with the modulo reduction.

*/

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("ssse3,pclmul"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("ssse3,pclmul")
#endif

VOID
SYMCRYPT_CALL
SymCryptGHashExpandKeyPclmulqdq(
    _Out_writes_( SYMCRYPT_GF128_FIELD_SIZE )   PSYMCRYPT_GF128_ELEMENT expandedKey,
    _In_reads_( SYMCRYPT_GF128_BLOCK_SIZE )     PCBYTE                  pH )
{
    int i;
    __m128i H, Hx, H2, H2x;
    __m128i t0, t1, t2, t3, t4, t5;
    __m128i Hi_even, Hix_even, Hi_odd, Hix_odd;
    __m128i BYTE_REVERSE_ORDER = _mm_set_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 );
    __m128i vMultiplicationConstant = _mm_set_epi32( 0, 0, 0xc2000000, 0 );

    //
    // Our expanded key consists of a list of N=SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS
    // powers of H. The first entry is H^N, the next H^(N-1), then H^(N-2), ...
    //
    // For each power we store two 128-bit values. The first is H^i (Hi) and the second
    // contains the two halves of H^i xorred with each other in the lower 64 bits (Hix).
    //
    // We keep all of the Hi entries together in the first half of the expanded key
    // table, and all of the Hix entries together in the second half of the table.
    //
    // This ordering allow for efficient vectorization with arbitrary vector width, as
    // many multiplication constants can be loaded into wider vectors with the correct
    // alignment. Not maintaining different layouts for different vector lengths does
    // leave a small amount of performance on the table, but experimentally it seems to
    // <1% difference, and using a single layout reduces complexity significantly.
    //
    C_ASSERT( 2*SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS <= SYMCRYPT_GF128_FIELD_SIZE );

    H = _mm_loadu_si128((__m128i *) pH );
    H = _mm_shuffle_epi8( H, BYTE_REVERSE_ORDER );
    Hx = _mm_xor_si128( H, _mm_srli_si128( H, 8 ) );

    _mm_store_si128( &GHASH_H_POWER(expandedKey, 1), H );
    _mm_store_si128( &GHASH_Hx_POWER(expandedKey, 1), Hx );

    CLMUL_X_3( H, Hx, H, Hx, t0, t1, t2 );
    CLMUL_3_POST( t0, t1, t2 );
    MODREDUCE( vMultiplicationConstant, t0, t1, t2, H2 );
    H2x = _mm_xor_si128( H2, _mm_srli_si128( H2, 8 ) );
    _mm_store_si128( &GHASH_H_POWER(expandedKey, 2), H2 );
    _mm_store_si128( &GHASH_Hx_POWER(expandedKey, 2), H2x );

    Hi_even = H2;
    Hix_even = H2x;

    for( i=2; i<SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS; i+=2 )
    {
        CLMUL_X_3( H, Hx, Hi_even, Hix_even, t0, t1, t2 );
        CLMUL_3_POST( t0, t1, t2 );
        CLMUL_X_3( H2, H2x, Hi_even, Hix_even, t3, t4, t5 );
        CLMUL_3_POST( t3, t4, t5 );
        MODREDUCE( vMultiplicationConstant, t0, t1, t2, Hi_odd );
        MODREDUCE( vMultiplicationConstant, t3, t4, t5, Hi_even );
        Hix_odd  = _mm_xor_si128( Hi_odd, _mm_srli_si128( Hi_odd, 8 ) );
        Hix_even = _mm_xor_si128( Hi_even, _mm_srli_si128( Hi_even, 8 ) );

        _mm_store_si128( &GHASH_H_POWER(expandedKey, i + 1), Hi_odd );
        _mm_store_si128( &GHASH_H_POWER(expandedKey, i + 2), Hi_even );
        _mm_store_si128( &GHASH_Hx_POWER(expandedKey, i + 1), Hix_odd );
        _mm_store_si128( &GHASH_Hx_POWER(expandedKey, i + 2), Hix_even );
    }
}



VOID
SYMCRYPT_CALL
SymCryptGHashAppendDataPclmulqdq(
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbData,
                                            SIZE_T                      cbData )
{
    __m128i state;
    __m128i data;
    __m128i a0, a1, a2;
    __m128i Hi, Hix;
    SIZE_T i;
    SIZE_T nBlocks = cbData / SYMCRYPT_GF128_BLOCK_SIZE;
    SIZE_T todo;

    //
    // To do a BSWAP we need an __m128i value with the bytes
    //

    __m128i BYTE_REVERSE_ORDER = _mm_set_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 );
    __m128i vMultiplicationConstant = _mm_set_epi32( 0, 0, 0xc2000000, 0 );

    state = _mm_loadu_si128( (__m128i *) pState );

    while( nBlocks > 0 )
    {
        //
        // We process the data in blocks of up to SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS blocks
        //
        todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS );

        //
        // The first block is xorred with the state before multiplying it with a power of H
        //
        data = _mm_loadu_si128( (__m128i *) pbData );
        data = _mm_shuffle_epi8( data, BYTE_REVERSE_ORDER );
        pbData += SYMCRYPT_GF128_BLOCK_SIZE;

        state = _mm_xor_si128( state, data );
        CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );

        //
        // Then we just do an improduct
        //
        for( i=1; i<todo; i++ )
        {
            data = _mm_loadu_si128( (__m128i *) pbData );
            data = _mm_shuffle_epi8( data, BYTE_REVERSE_ORDER );
            pbData += SYMCRYPT_GF128_BLOCK_SIZE;

            Hi  = _mm_load_si128( &GHASH_H_POWER(expandedKeyTable, todo - i) );
            Hix = _mm_load_si128( &GHASH_Hx_POWER(expandedKeyTable, todo - i) );
            CLMUL_ACC_3( data, Hi, Hix, a0, a1, a2 );
        }

        CLMUL_3_POST( a0, a1, a2 );
        MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );
        nBlocks -= todo;
    }

    _mm_storeu_si128((__m128i *)pState, state );
}

#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif

#endif  // CPU_X86 || CPU_AMD64

#if SYMCRYPT_CPU_ARM64

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("aes"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("aes")
#endif

VOID
SYMCRYPT_CALL
SymCryptGHashExpandKeyPmull(
    _Out_writes_( SYMCRYPT_GF128_FIELD_SIZE )   PSYMCRYPT_GF128_ELEMENT expandedKey,
    _In_reads_( SYMCRYPT_GF128_BLOCK_SIZE )    PCBYTE                  pH )
{
    int i;
    __n128 H, Hx, H2, H2x;
    __n128 t0, t1, t2, t3, t4, t5;
    __n128 Hi_even, Hix_even, Hi_odd, Hix_odd;
    const __n64 vMultiplicationConstant = SYMCRYPT_SET_N64_U64(0xc200000000000000);
    //
    // Our expanded key consists of a list of N=SYMCRYPT_GHASH_PMULL_HPOWERS
    // powers of H. The first entry is H^N, the next H^(N-1), then H^(N-2), ...
    //
    // For each power we store two 128-bit values. The first is H^i (Hi) and the second
    // contains the two halves of H^i xorred with each other in the lower 64 bits (Hix).
    //
    // We keep all of the Hi entries together in the first half of the expanded key
    // table, and all of the Hix entries together in the second half of the table.
    //
    // This ordering allow for efficient vectorization with arbitrary vector width, as
    // many multiplication constants can be loaded into wider vectors with the correct
    // alignment. Not maintaining different layouts for different vector lengths does
    // leave a small amount of performance on the table, but experimentally it seems to
    // <1% difference, and using a single layout reduces complexity significantly.
    //
    C_ASSERT( 2*SYMCRYPT_GHASH_PMULL_HPOWERS <= SYMCRYPT_GF128_FIELD_SIZE );

    H = *(__n128 *) pH;
    Hx = vrev64q_u8( H );
    H = vextq_u8( Hx, Hx, 8 );
    Hx = veorq_u8( H, Hx );

    GHASH_H_POWER(expandedKey, 1) = H;
    GHASH_Hx_POWER(expandedKey, 1) = Hx;

    CLMUL_X_3( H, Hx, H, Hx, t0, t1, t2 );
    CLMUL_3_POST( t0, t1, t2 );
    MODREDUCE( vMultiplicationConstant, t0, t1, t2, H2 );
    H2x = veorq_u8( H2, vextq_u8( H2, H2, 8 ) );
    GHASH_H_POWER(expandedKey, 2) = H2;
    GHASH_Hx_POWER(expandedKey, 2) = H2x;

    Hi_even = H2;
    Hix_even = H2x;

    for( i=2; i<SYMCRYPT_GHASH_PMULL_HPOWERS; i+=2 )
    {
        CLMUL_X_3( H, Hx, Hi_even, Hix_even, t0, t1, t2 );
        CLMUL_3_POST( t0, t1, t2 );
        CLMUL_X_3( H2, H2x, Hi_even, Hix_even, t3, t4, t5 );
        CLMUL_3_POST( t3, t4, t5 );
        MODREDUCE( vMultiplicationConstant, t0, t1, t2, Hi_odd );
        MODREDUCE( vMultiplicationConstant, t3, t4, t5, Hi_even );
        Hix_odd = veorq_u8( Hi_odd, vextq_u8( Hi_odd, Hi_odd, 8 ) );
        Hix_even = veorq_u8( Hi_even, vextq_u8( Hi_even, Hi_even, 8 ) );

        GHASH_H_POWER(expandedKey, i + 1) = Hi_odd;
        GHASH_H_POWER(expandedKey, i + 2) = Hi_even;
        GHASH_Hx_POWER(expandedKey, i + 1) = Hix_odd;
        GHASH_Hx_POWER(expandedKey, i + 2) = Hix_even;
    }
}

VOID
SYMCRYPT_CALL
SymCryptGHashAppendDataPmull(
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbData,
                                            SIZE_T                      cbData )
{
    __n128 state;
    __n128 data, datax;
    __n128 a0, a1, a2;
    __n128 Hi, Hix;
    const __n64 vMultiplicationConstant = SYMCRYPT_SET_N64_U64(0xc200000000000000);
    SIZE_T i;
    SIZE_T nBlocks = cbData / SYMCRYPT_GF128_BLOCK_SIZE;
    SIZE_T todo;

    state = *(__n128 *) pState;

    while( nBlocks > 0 )
    {
        //
        // We process the data in blocks of up to SYMCRYPT_GHASH_PMULL_HPOWERS blocks
        //
        todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PMULL_HPOWERS );

        //
        // The first block is xorred with the state before multiplying it with a power of H
        //
        data = *(__n128 *)pbData;
        REVERSE_BYTES( data, data );
        pbData += SYMCRYPT_GF128_BLOCK_SIZE;

        state = veorq_u8( state, data );
        CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );

        //
        // Then we just do an improduct
        //
        for( i=1; i<todo; i++ )
        {
            // we can avoid an EXT here by precomputing datax for CLMUL_ACCX_3
            datax = vrev64q_u8( *(__n128 *)pbData );
            data = vextq_u8( datax, datax, 8 );
            datax = veorq_u8( data, datax );
            pbData += SYMCRYPT_GF128_BLOCK_SIZE;

            Hi  = GHASH_H_POWER(expandedKeyTable, todo - i);
            Hix = GHASH_Hx_POWER(expandedKeyTable, todo - i);
            CLMUL_ACCX_3( data, datax, Hi, Hix, a0, a1, a2 );
        }

        CLMUL_3_POST( a0, a1, a2 );
        MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );
        nBlocks -= todo;
    }

    *(__n128 *) pState = state;
}

#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif

#endif  // CPU_ARM64



//////////////////////////////////////////////////////////////
// Stuff around the core algorithm implementation functions
//


VOID
SYMCRYPT_CALL
SymCryptGHashExpandKey(
    _Out_                                       PSYMCRYPT_GHASH_EXPANDED_KEY    expandedKey,
    _In_reads_( SYMCRYPT_GF128_BLOCK_SIZE )     PCBYTE                          pH )
{
#if  SYMCRYPT_CPU_X86
    PSYMCRYPT_GF128_ELEMENT pExpandedKeyTable;
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    //
    // Initialize offset into table space for 16-alignment.
    //
    expandedKey->tableOffset = (0 -((UINT_PTR) &expandedKey->tableSpace[0])) % sizeof(SYMCRYPT_GF128_ELEMENT);

    pExpandedKeyTable = (PSYMCRYPT_GF128_ELEMENT)&expandedKey->tableSpace[expandedKey->tableOffset];

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_PCLMULQDQ_CODE ) )
    {
        //
        // We can only use the PCLMULQDQ data representation if the SaveXmm never fails.
        // This is one of the CPU features required.
        // We check anyway...
        //
        if( SymCryptSaveXmm( &SaveData ) != SYMCRYPT_NO_ERROR )
        {
            SymCryptFatal( 'pclm' );
        }
        SymCryptGHashExpandKeyPclmulqdq( pExpandedKeyTable, pH );
        SymCryptRestoreXmm( &SaveData );
    } else if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_SSE2 ) && SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptGHashExpandKeyXmm( pExpandedKeyTable, pH );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptGHashExpandKeyC( pExpandedKeyTable, pH );
    }

#elif SYMCRYPT_CPU_AMD64
    PSYMCRYPT_GF128_ELEMENT pExpandedKeyTable;
    pExpandedKeyTable = &expandedKey->table[0];

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_PCLMULQDQ_CODE ) )
    {
        SymCryptGHashExpandKeyPclmulqdq( pExpandedKeyTable, pH );
    } else if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_SSE2 ) )
    {
        SymCryptGHashExpandKeyXmm( pExpandedKeyTable, pH );
    } else {
        SymCryptGHashExpandKeyC( pExpandedKeyTable, pH );
    }

#elif SYMCRYPT_CPU_ARM64
    PSYMCRYPT_GF128_ELEMENT pExpandedKeyTable;
    pExpandedKeyTable = &expandedKey->table[0];

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_PMULL ) )
    {
        SymCryptGHashExpandKeyPmull( pExpandedKeyTable, pH );
    } else {
        SymCryptGHashExpandKeyC( pExpandedKeyTable, pH );
    }

#else
    SymCryptGHashExpandKeyC( &expandedKey->table[0], pH );      // Default expansion (does not need alignment)
#endif
}

VOID
SYMCRYPT_CALL
SymCryptGHashAppendData(
    _In_                              PCSYMCRYPT_GHASH_EXPANDED_KEY   expandedKey,
    _Inout_                           PSYMCRYPT_GF128_ELEMENT         pState,
    _In_reads_( cbData )              PCBYTE                          pbData,
                                      SIZE_T                          cbData )
{
#if SYMCRYPT_CPU_X86
    PCSYMCRYPT_GF128_ELEMENT pExpandedKeyTable;
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    pExpandedKeyTable = (PSYMCRYPT_GF128_ELEMENT)&expandedKey->tableSpace[expandedKey->tableOffset];

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_PCLMULQDQ_CODE ) )
    {
        if( SymCryptSaveXmm( &SaveData ) != SYMCRYPT_NO_ERROR )
        {
            SymCryptFatal( 'pclm' );
        }
        SymCryptGHashAppendDataPclmulqdq( pExpandedKeyTable, pState, pbData, cbData );
        SymCryptRestoreXmm( &SaveData );
    } else if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_SSE2 ) && SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptGHashAppendDataXmm( pExpandedKeyTable, pState, pbData, cbData );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptGHashAppendDataC( pExpandedKeyTable, pState, pbData, cbData );
    }

#elif SYMCRYPT_CPU_AMD64
    PCSYMCRYPT_GF128_ELEMENT pExpandedKeyTable;

    pExpandedKeyTable = &expandedKey->table[0];
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_PCLMULQDQ_CODE ) )
    {
        SymCryptGHashAppendDataPclmulqdq( pExpandedKeyTable, pState, pbData, cbData );
    } else if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_SSE2 ) )
    {
        SymCryptGHashAppendDataXmm( pExpandedKeyTable, pState, pbData, cbData );
    } else {
        SymCryptGHashAppendDataC( pExpandedKeyTable, pState, pbData, cbData );
    }
#elif SYMCRYPT_CPU_ARM
    PCSYMCRYPT_GF128_ELEMENT pExpandedKeyTable;

    pExpandedKeyTable = &expandedKey->table[0];
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON ) )
    {
        SymCryptGHashAppendDataNeon( pExpandedKeyTable, pState, pbData, cbData );
    } else {
        SymCryptGHashAppendDataC( pExpandedKeyTable, pState, pbData, cbData );
    }
#elif SYMCRYPT_CPU_ARM64
    PCSYMCRYPT_GF128_ELEMENT pExpandedKeyTable;

    pExpandedKeyTable = &expandedKey->table[0];
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_PMULL ) )
    {
        SymCryptGHashAppendDataPmull( pExpandedKeyTable, pState, pbData, cbData );
    } else if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON ) )
    {
        SymCryptGHashAppendDataNeon( pExpandedKeyTable, pState, pbData, cbData );
    } else {
        SymCryptGHashAppendDataC( pExpandedKeyTable, pState, pbData, cbData );
    }
#else
    SymCryptGHashAppendDataC( &expandedKey->table[0], pState, pbData, cbData );
#endif
}
