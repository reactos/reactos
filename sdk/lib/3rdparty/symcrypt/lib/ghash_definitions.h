//
// ghash_definitions.h
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//////////////////////////////////////////////////////////////////////////////
// Constants & globals
//

#define GF128_FIELD_R_BYTE   (0xe1)
#define UINT64_NEG(x) ((UINT64)-(INT64)(x))



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

#define SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS    32

#define GHASH_H_POWER( ghashTable, ind )  ( (ghashTable)[  SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS - (ind)].m128i )
#define GHASH_Hx_POWER( ghashTable, ind ) ( (ghashTable)[2*SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS - (ind)].m128i )

//
// We define a few macros
//

//
// CLMUL_4 multiplies two operands into three intermediate results using 4 pclmulqdq instructions
//
#define CLMUL_4( opA, opB, resl, resm, resh ) \
{ \
    resl = _mm_clmulepi64_si128( opA, opB, 0x00 ); \
    resm = _mm_xor_si128( _mm_clmulepi64_si128( opA, opB, 0x01 ), _mm_clmulepi64_si128( opA, opB, 0x10 ) ); \
    resh = _mm_clmulepi64_si128( opA, opB, 0x11 ); \
};

//
// CLMUL_3 multiplies two operands into three intermediate results using 3 pclmulqdq instructions.
// The second operand has a pre-computed difference of the two halves.
// This uses Karatsuba, but we delay xorring the high and low piece into the middle piece.
//
#define CLMUL_3( opA, opB, opBx, resl, resm, resh ) \
{ \
    __m128i _tmpA; \
    resl = _mm_clmulepi64_si128( opA, opB, 0x00 ); \
    resh = _mm_clmulepi64_si128( opA, opB, 0x11 ); \
    _tmpA = _mm_xor_si128( opA, _mm_srli_si128( opA, 8 ) ); \
    resm = _mm_clmulepi64_si128( _tmpA, opBx, 0x00 ); \
};
//
// CLMUL_X_3 is as CLMUL_3 only it takes precomputed differences of both multiplicands.
//
#define CLMUL_X_3( opA, opAx, opB, opBx, resl, resm, resh ) \
{ \
    resl = _mm_clmulepi64_si128( opA, opB, 0x00 ); \
    resh = _mm_clmulepi64_si128( opA, opB, 0x11 ); \
    resm = _mm_clmulepi64_si128( opAx, opBx, 0x00 ); \
};

//
// Post-process the CLMUL_3 result to be compatible with the CLMUL_4
//
#define CLMUL_3_POST( resl, resm, resh ) \
    resm = _mm_xor_si128( resm, _mm_xor_si128( resl, resh ) );

//
// Multiply-accumulate using CLMUL_4
//
#define CLMUL_ACC_4( opA, opB, resl, resm, resh ) \
{\
    __m128i _tmpl, _tmpm, _tmph;\
    CLMUL_4( opA, opB, _tmpl, _tmpm, _tmph );\
    resl = _mm_xor_si128( resl, _tmpl ); \
    resm = _mm_xor_si128( resm, _tmpm ); \
    resh = _mm_xor_si128( resh, _tmph ); \
};

//
// Multiply-accumulate using CLMUL_3
//
#define CLMUL_ACC_3( opA, opB, opBx, resl, resm, resh ) \
{\
    __m128i _tmpl, _tmpm, _tmph;\
    CLMUL_3( opA, opB, opBx, _tmpl, _tmpm, _tmph );\
    resl = _mm_xor_si128( resl, _tmpl ); \
    resm = _mm_xor_si128( resm, _tmpm ); \
    resh = _mm_xor_si128( resh, _tmph ); \
};
#define CLMUL_ACC_3_Ymm( opA, opB, opBx, resl, resm, resh ) \
{\
    __m256i _tmpl, _tmpm, _tmph;\
    __m256i _tmpA; \
    _tmpl = _mm256_clmulepi64_epi128( opA, opB, 0x00 ); \
    _tmph = _mm256_clmulepi64_epi128( opA, opB, 0x11 ); \
    _tmpA = _mm256_xor_si256( opA, _mm256_srli_si256( opA, 8 ) ); \
    _tmpm = _mm256_clmulepi64_epi128( _tmpA, opBx, 0x00 ); \
    resl = _mm256_xor_si256( resl, _tmpl ); \
    resm = _mm256_xor_si256( resm, _tmpm ); \
    resh = _mm256_xor_si256( resh, _tmph ); \
};


//
// Convert the 3 intermediate results to a 256-bit result,
// and do the modulo reduction.
#define MODREDUCE( vMultiplicationConstant, rl, rm, rh, res ) \
{\
    __m128i _T0, _T1; \
\
    /* multiply rl by constant which is (rev(0x87) << 1) - we'll eor the lost high bit in manually */ \
    _T0 = _mm_clmulepi64_si128( rl, vMultiplicationConstant, 0x00 ); \
\
    /* we want the high 64b of rl to align with the low 64b of rm, because we haven't merged rm into rl and rh */ \
    /* we want the low 64b of rl to align with the high 64b of rm, because we lost the high bit in the previous pmull */ \
    rl = _mm_shuffle_epi32( rl, _MM_SHUFFLE( 1, 0, 3, 2 ) ); \
\
    rm = _mm_xor_si128( rm, _T0 ); \
    rm = _mm_xor_si128( rm, rl ); \
\
    /* almost same again to fold rm into rh, but bit 63 needs no more multiplication and the result ultimately needs shifting left by 1 */ \
    /* pre-shift bottom of rm left by 1 and accumulate the result when the other parts are aligned */ \
    _T0 = _mm_clmulepi64_si128( _mm_slli_epi64( rm, 1 ), vMultiplicationConstant, 0x00 ); \
\
    rm = _mm_shuffle_epi32( rm, _MM_SHUFFLE( 1, 0, 3, 2 ) ); \
    res = _mm_xor_si128( rh, rm ); \
\
    /* rotate res left by 1 and accumulate the aligned parts */ \
    _T1 = _mm_slli_epi32( res, 1 ); \
    res = _mm_srli_epi32( res, 31 ); \
\
    _T0 = _mm_xor_si128( _T0, _T1 ); \
    res = _mm_shuffle_epi32( res, _MM_SHUFFLE( 2, 1, 0, 3 ) ); \
\
    res = _mm_xor_si128( res, _T0 ); \
};

//
// See the large comment above on how this is done.
// When we want to do MODREDUCE in parallel with other work, making use of pclmuldq to reduce
// total instruction count (and register pressure) is beneficial. When testing on Haswell,
// using the newer approach is beneficial. Keeping the old approach around in case we have significant
// regression on older platforms.
//
#define MODREDUCE_OLD( rl, rm, rh, res ) \
{\
    __m128i _T0, _T1, _T2, _Q0, _Q1; \
    rl = _mm_xor_si128( rl, _mm_slli_si128( rm, 8 ) ); \
    rh = _mm_xor_si128( rh, _mm_srli_si128( rm, 8 ) ); \
\
    _Q0 = _mm_slli_epi32( rl, 1 ); \
    _Q1 = _mm_slli_epi32( rh, 1 ); \
\
    _T0 = _mm_srli_epi32( rl, 31 ); \
    _T1 = _mm_srli_epi32( rh, 31 ); \
\
    _T1 = _mm_alignr_epi8( _T1, _T0, 12 ); \
    _T0 = _mm_slli_si128( _T0, 4 ); \
\
    _Q0 = _mm_xor_si128( _Q0, _T0 ); \
    _Q1 = _mm_xor_si128( _Q1, _T1 ); \
\
    _T0 = _mm_slli_epi32( _Q0, 31 ); \
    _T1 = _mm_slli_epi32( _Q0, 30 ); \
    _T2 = _mm_slli_epi32( _Q0, 25 ); \
    _T0 = _mm_xor_si128( _T0, _T1 ); \
    _T0 = _mm_xor_si128( _T0, _T2 ); \
\
    _T1 = _mm_slli_si128( _T0, 12 ); \
\
    _T2 = _mm_xor_si128( _Q0, _T1 ); \
\
    res = _mm_xor_si128( _Q1, _T2 ); \
    _T1 = _mm_srli_si128( _T0, 4 ); \
    res = _mm_xor_si128(  res, _T1 ); \
\
    _T0 = _mm_srli_epi32( _T2, 1 ); \
    _T1 = _mm_srli_epi32( _T2, 2 ); \
    _T2 = _mm_srli_epi32( _T2, 7 ); \
\
    _T1 = _mm_xor_si128( _T0, _T1 ); \
    res = _mm_xor_si128( res, _T2 ); \
    res = _mm_xor_si128( res, _T1 ); \
};

#endif  // CPU_X86 || CPU_AMD64

#if SYMCRYPT_CPU_ARM64

#define SYMCRYPT_GHASH_PMULL_HPOWERS    32

#define GHASH_H_POWER( ghashTable, ind )  ( (ghashTable)[  SYMCRYPT_GHASH_PMULL_HPOWERS - (ind)].n128 )
#define GHASH_Hx_POWER( ghashTable, ind ) ( (ghashTable)[2*SYMCRYPT_GHASH_PMULL_HPOWERS - (ind)].n128 )

#if SYMCRYPT_MS_VC
#ifndef vshl_n_u64
#define vshl_n_u64(src1, src2) neon_shlis64(src1, src2)
#endif
#endif
//
// CLMUL_4 multiplies two operands into three intermediate results using 4 pmull instructions
//
#define CLMUL_4( opA, opB, resl, resm, resh ) \
{ \
    __n128 _tmp; \
    resl = vmullq_p64( opA, opB ); \
    _tmp = vextq_u8( opA, opA, 8 ); \
    resm = veorq_u8( vmullq_p64( opB, _tmp ), vmull_high_p64( opB, _tmp ) );\
    resh = vmull_high_p64( opA, opB ); \
};

//
// CLMUL_3 multiplies two operands into three intermediate results using 3 pmull instructions.
// The second operand has a pre-computed difference of the two halves.
// This uses Karatsuba, but we delay xorring the high and low piece into the middle piece.
//
#define CLMUL_3( opA, opB, opBx, resl, resm, resh ) \
{ \
    __n128 _tmpA; \
    resl = vmullq_p64( opA, opB ); \
    resh = vmull_high_p64( opA, opB ); \
    _tmpA = veorq_u8( opA, vextq_u8( opA, opA, 8 ) ); \
    resm = vmullq_p64( _tmpA, opBx ); \
};
//
// CLMUL_X_3 is as CLMUL_3 only it takes precomputed differences of both multiplicands
//
#define CLMUL_X_3( opA, opAx, opB, opBx, resl, resm, resh ) \
{ \
    resl = vmullq_p64( opA, opB ); \
    resh = vmull_high_p64( opA, opB ); \
    resm = vmullq_p64( opAx, opBx ); \
};

//
// Post-process the CLMUL_3 result to be compatible with the CLMUL_4
//
#define CLMUL_3_POST( resl, resm, resh ) \
    resm = veorq_u8( resm, veorq_u8( resl, resh ) );

//
// Multiply-accumulate using CLMUL_4
//
#define CLMUL_ACC_4( opA, opB, resl, resm, resh ) \
{\
    __n128 _tmpl, _tmpm, _tmph;\
    CLMUL_4( opA, opB, _tmpl, _tmpm, _tmph );\
    resl = veorq_u8( resl, _tmpl ); \
    resm = veorq_u8( resm, _tmpm ); \
    resh = veorq_u8( resh, _tmph ); \
};

//
// Multiply-accumulate two operands into 3 accumulators.
// Takes the multiplicands and the pre-computed differences of the two halves of both multiplicands.
//
#define CLMUL_ACCX_3( opA, opAx, opB, opBx, resl, resm, resh ) \
{\
    __n128 _tmpl, _tmpm, _tmph;\
    CLMUL_X_3( opA, opAx, opB, opBx, _tmpl, _tmpm, _tmph ); \
    resl = veorq_u8( resl, _tmpl ); \
    resm = veorq_u8( resm, _tmpm ); \
    resh = veorq_u8( resh, _tmph ); \
};


//
// Convert the 3 intermediate results to a 256-bit result,
// and do the modulo reduction.
// See the large comment above on how this is done.
//
#define MODREDUCE( vMultiplicationConstant, rl, rm, rh, res ) \
{\
    __n128 _T0, _T1; \
\
    /* multiply rl by constant which is (rev(0x87) << 1) - we'll eor the lost high bit in manually */ \
    _T0 = vmull_p64( vget_low_p64(rl), vMultiplicationConstant ); \
\
    /* we want the high 64b of rl to align with the low 64b of rm, because we haven't merged rm into rl and rh */ \
    /* we want the low 64b of rl to align with the high 64b of rm, because we lost the high bit in the previous pmull */ \
    rl = vextq_u8( rl, rl, 8 ); \
\
    rm = veorq_u8( rm, _T0 ); \
    rm = veorq_u8( rm, rl ); \
\
    /* almost same again to fold rm into rh, but bit 63 needs no more multiplication and the result ultimately needs shifting left by 1 */ \
    /* pre-shift bottom of rm left by 1 and accumulate the result when the other parts are aligned */ \
    _T0 = vmull_p64( vshl_n_u64(vget_low_p64(rm), 1), vMultiplicationConstant ); \
\
    rm = vextq_u8( rm, rm, 8 ); \
    res = veorq_u8( rh, rm ); \
\
    /* rotate res left by 1 and accumulate the aligned parts */ \
    _T1 = vshlq_n_u32( res, 1 ); \
    res = vshrq_n_u32( res, 31 ); \
\
    _T0 = veorq_u8( _T0, _T1 ); \
    res = vextq_u8( res, res, 12 ); \
\
    res = veorq_u8( res, _T0 ); \
};

#define REVERSE_BYTES( _in, _out )\
{\
    __n128 _t;\
    _t = vrev64q_u8( _in ); \
    _out = vextq_u8( _t, _t, 8 ); \
}

#endif  // CPU_ARM64
