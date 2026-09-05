//
// Poly1305.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

VOID
SYMCRYPT_CALL
SymCryptPoly1305ProcessBlocks(
    _Inout_                 PSYMCRYPT_POLY1305_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptPoly1305(
    _In_reads_( SYMCRYPT_POLY1305_KEY_SIZE )        PCBYTE  pbKey,
    _In_reads_( cbData )                            PCBYTE  pbData,
                                                    SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_POLY1305_RESULT_SIZE )   PBYTE   pbResult )
{
    SYMCRYPT_POLY1305_STATE state;

    SymCryptPoly1305Init( &state, pbKey );
    SymCryptPoly1305Append( &state, pbData, cbData );
    SymCryptPoly1305Result( &state, pbResult );
}

VOID
SYMCRYPT_CALL
SymCryptPoly1305Init(
    _Out_                                       PSYMCRYPT_POLY1305_STATE    pState,
    _In_reads_( SYMCRYPT_POLY1305_KEY_SIZE )    PCBYTE                      pbKey )
{
    pState->r[0] = SYMCRYPT_LOAD_LSBFIRST32( pbKey +  0 ) & 0x0fffffff;
    pState->r[1] = SYMCRYPT_LOAD_LSBFIRST32( pbKey +  4 ) & 0x0ffffffc;
    pState->r[2] = SYMCRYPT_LOAD_LSBFIRST32( pbKey +  8 ) & 0x0ffffffc;
    pState->r[3] = SYMCRYPT_LOAD_LSBFIRST32( pbKey + 12 ) & 0x0ffffffc;

    pState->s[0] = SYMCRYPT_LOAD_LSBFIRST32( pbKey + 16 );
    pState->s[1] = SYMCRYPT_LOAD_LSBFIRST32( pbKey + 20 );
    pState->s[2] = SYMCRYPT_LOAD_LSBFIRST32( pbKey + 24 );
    pState->s[3] = SYMCRYPT_LOAD_LSBFIRST32( pbKey + 28 );

    // Set accumulator to zero
    SymCryptWipeKnownSize( &pState->a[0], sizeof( pState->a ) );

    pState->bytesInBuffer = 0;
}

VOID
SYMCRYPT_CALL
SymCryptPoly1305Append(
    _Inout_                 PSYMCRYPT_POLY1305_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData )
{
    SIZE_T nBytes;
    SIZE_T bytesInBuffer;

    bytesInBuffer = pState->bytesInBuffer;
    if( bytesInBuffer != 0 )
    {
        // We have a partial block in the buffer, keep filling the block

        SYMCRYPT_ASSERT( bytesInBuffer < 16 );
        nBytes = 16 - bytesInBuffer;
        if( nBytes > cbData )
        {
            nBytes = cbData;
        }

        memcpy( &pState->buf[bytesInBuffer], pbData, nBytes );
        pbData += nBytes;
        cbData -= nBytes;
        bytesInBuffer += nBytes;

        if( bytesInBuffer == 16 )
        {
            // Buffer is full, process it and empty the buffer
            SymCryptPoly1305ProcessBlocks( pState, pState->buf, bytesInBuffer );
            bytesInBuffer = 0;
        }
        pState->bytesInBuffer = bytesInBuffer;
    }

    if( cbData >= 16 )
    {
        // There are whole blocks to process
        SymCryptPoly1305ProcessBlocks( pState, pbData, cbData & ~0xf );
        pbData += cbData;
        cbData &= 0xf;
        pbData -= cbData;
    }

    if( cbData > 0 )
    {
        // Copy remaining data to buffer
        SYMCRYPT_ASSERT( cbData < 16 );
        memcpy( &pState->buf[0], pbData, cbData );
        pState->bytesInBuffer = cbData;
    }
}

VOID
SYMCRYPT_CALL
SymCryptPoly1305Result(
    _Inout_                                         PSYMCRYPT_POLY1305_STATE    pState,
    _Out_writes_( SYMCRYPT_POLY1305_RESULT_SIZE )   PBYTE                       pbResult )
{
    SIZE_T bytesInBuffer;
    UINT64 t;
    UINT32 a4, a3, a2, a1, a0;
    UINT32 maskOld, maskNew;

    bytesInBuffer = pState->bytesInBuffer;
    if( bytesInBuffer > 0 )
    {
        // Add trailing '1' byte and pad with zeroes
        // Wipe function deals with 0-length wipes properly
        pState->buf[bytesInBuffer++] = 1;
        SymCryptWipe( &pState->buf[bytesInBuffer], 16 - bytesInBuffer );

        // Now we have to process the block, but the block function adds a trailing
        // 1 byte to each 16-byte block. We compensate for that by decrementing
        // the highest word of the accumulator first; the 1 byte added by the block
        // processing function has the effect of incrementing the highest accumulator
        // word so those two operations cancel each other out.
        pState->a[4] -= 1;
        SymCryptPoly1305ProcessBlocks( pState, pState->buf, 16 );
    }

    // We have to fully reduce the accumulator first
    // We have a[4]<6 at this point
    a0 = pState->a[0];
    a1 = pState->a[1];
    a2 = pState->a[2];
    a3 = pState->a[3];
    a4 = pState->a[4];

    SYMCRYPT_ASSERT( a4 < 6 );
    // Because a4 < 6, we have to subtract either 0*P or 1*P
    // we subtract P and them mux-choose between the new and old value
    // Subtracting P is the same as subtracting 2^130 and adding 5
    t = 5;

    t += a0;
    a0 = (UINT32) t;
    t >>= 32;

    t += a1;
    a1 = (UINT32) t;
    t >>= 32;

    t += a2;
    a2 = (UINT32) t;
    t >>= 32;

    t += a3;
    a3 = (UINT32) t;
    t >>= 32;

    t += a4;
    t -= 4;
    a4 = (UINT32) t;
    t >>= 32;

    // If this subtraction produced a carry, then t = 0xffffffff, otherwise it is 0
    maskOld = (UINT32) t;       // ffffffff if the old value is correct, 0 otherwise
    maskNew = ~maskOld;         // ffffffff if the new value is correct, 0 otherwise

    a0 = (maskNew & a0) | (maskOld & pState->a[0]);
    a1 = (maskNew & a1) | (maskOld & pState->a[1]);
    a2 = (maskNew & a2) | (maskOld & pState->a[2]);
    a3 = (maskNew & a3) | (maskOld & pState->a[3]);
 // a4 = (maskNew & a4) | (maskOld & pState->a[4]); // We don't need a4...

    // Now we add S and return the data
    t  = a0;
    t += pState->s[0];
    SYMCRYPT_STORE_LSBFIRST32( pbResult +  0, (UINT32) t );
    t >>= 32;

    t += a1;
    t += pState->s[1];
    SYMCRYPT_STORE_LSBFIRST32( pbResult +  4, (UINT32) t );
    t >>= 32;

    t += a2;
    t += pState->s[2];
    SYMCRYPT_STORE_LSBFIRST32( pbResult +  8, (UINT32) t );
    t >>= 32;

    t += a3;
    t += pState->s[3];
    SYMCRYPT_STORE_LSBFIRST32( pbResult + 12, (UINT32) t );

    SymCryptWipeKnownSize( (PBYTE) pState, sizeof( *pState ) );
}


/*
The heart of Poly1305 is a modular multiplication.
The modulus P := 2^130 - 5

One multiplicant is R which is part of the key. R is restricted to a subset of all possible
values ("clamped") to make the computation faster.
The other multiplicant is the accumulator A. The overall operation is

    A += <value derived from the data>
    A = (A*R) mod P

We write all values base 2^32:
b := 2^32
A = a4 b^4 + a3 b^3 + a2 b^2 + a1 b + a1
R =          r3 b^3 + r2 b^2 + r1 b + r0

Fully reduced we would have a4 <= 3 but we don't store A in fully-reduced form. Instead
we maintain a4 < L with L:=8.

The restrictions on R are:
    r3, r2, r1, r0 < 2^28
    r3, r2, r1 are multiples of 4

The core algorithm looks like this (explanations below)


                  a4   a3  a2   a1   a0
                       r3  r2   r1   r0 *
---------------------------------------
               a4r0 a3r0 a2r0 a1r0 a0r0
          a4r1 a3r1 a2r1 a1r1 a0r1
     a4r2 a3r2 a2r2 a1r1 a0r2
a4r3 a3r3 a2r3 a1r3 a0r3                 +
----------------------------------------
  S7   S6   S5   S4   S3   S2   S1   S0

  S7   S6   S5 T4+U   T3   S2   S1   S0

                      T3   S2   S1   S0
                      S7   S6   S5   T4
                    S7/4 S6/4 S5/4 T4/4 +
                    -------------------
                  U   V3   V2   V1   V0

At the top you see A and R with the 5*4 digit products arranged in columns.
The S values are the sums of the product columns without any carries.
Because the r values are <2^28 and a4 < L we have

    S0 <= 1*(2^32-1)(2^28-1)
    S1 <= 2*(2^32-1)(2^28-1)
    S2 <= 3*(2^32-1)(2^28-1)
    S3 <= 4*(2^32-1)(2^28-1)
    S4 <= 3*(2^32-1)(2^28-1) + (L-1)*(2^28-1)
    S5 <= 2*(2^32-1)(2^28-1) + (L-1)*(2^28-1), multiple of 4
    S6 <= 1*(2^32-1)(2^28-1) + (L-1)*(2^28-1), multiple of 4
    S7 <=                      (L-1)*(2^28-1), multiple of 4

The next line defines T4, U, and T3 by
T3 := S3 mod b          the lower word of S3
T := S4 + floor(S3/b)   add the upper word of S3 to S4
U := T mod 4
T4 := T - U             Split T into a small value U and a bigger T4 that is a multiple of 4

note that the digits (S7,S6, S5, S4, S3, S2, S1, S0) and (S7, S6, S5, T4+U, T3, S2, S1, S0)
encode the same number, namely the result of the multiplication.

We have bounds
    floor(S3/b) <= 2^2 * (2^32-1) * (2^28-1) / 2^32  <  2^30
    T  < 3*(2^32-1)(2^28-1) + (L-1)*(2^28-1) + 2^30
    U  < 4
    T4 < 3*(2^32-1)(2^28-1) + (L-1)*(2^28-1) + 2^30, multiple of 4

Now we are ready to perform the modulo reduction. Because P = 2^130 - 5 we have for any value X
    X*2^130 mod P = 5*X mod P
because 2^130 = 5 mod P
Or, if X is a multiple of 4 then
    X*2^128 = (X + X/4) mod P
(this is just the previous equation divided by 4)
We apply that to S7, S6, S5, and T4 and add them (column wise) to (T3, S2, S1, S0) to get

V0 := S0 + T4 + T4/4
V1 := S1 + S5 + S5/4
V2 := S2 + S6 + S6/4
V3 := T3 + S7 + S7/4

and note that (U, V3, V2, V1, V0) is equal to the result of the multiplication modulo P
We can derive some bounds on these values

 We assume L <= 8 (will get strict bound later)

    V0 < 1*(2^32-1)(2^28-1) + 3*(2^32-1)(2^28-1) + (L-1)*(2^28-1) + 2^30 + (3*(2^32-1)(2^28-1) + (L-1)*(2^28-1) + 2^30)/4
       = 4*1*(2^32-1)(2^28-1) + (L-1)*(2^28-1) + 2^30 + (3*(2^32-1)(2^28-1) + (L-1)*(2^28-1) + 2^30)/4
       < 2^2 * 2^32 * 2^28 + 2^31 + 2^30 + (2^4 * 2^32 * 2^28 + 2^31 + 2^30)/4
       = 2^62 + 2^31 + 2^30 + 2^60 + 2^29 + 2^30
       < 2^63

    V1 < 2*2^60 + 2*2^60 + 2^31 + (2*2^60 + 2^31)/4 < 2^63

    V2 < 3*2^60 + 2^60 + 2^31 + (2^60 + 2^31) < 2^63

    V3 < 2^32 + 2^31 + 2^29 < 2*2^32

    U < 4

So all the V values fit in 64 bits. A final carry propagation pass cleans this up to an array of 32-bit values which become
the new accumulator value. (During carry propagation the 32-bit carry from the lower digit can be added to the higher digit
because the V values are less than 2^63.)

V3 < 2*2^32 and after adding at most 2^32 from a carry it is < 3*2^32 so the carry from V3 to U is at most 2.
Thus the highest digit of the accumulator can be at most 3 + 2 = 5. This ensures a5<L for L>=6. We assumed L<=8 before, so
L=6 works and satisfies the earlier assumption.

To clarify the logic: IF a4<8 at the start of the multiplication THEN a4<6 after this function. Between multiplications we add
a value < 2^129 which could result in adding 2 to a4, but as a4<6 before the addition the a4<8 before the multiplication
is still satisfied.
*/

VOID
SYMCRYPT_CALL
SymCryptPoly1305ProcessBlocks(
    _Inout_                 PSYMCRYPT_POLY1305_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData )
// This is the portable C implementation, based on 32-bit operations.
// If necessary, we'll add assembler code for this function later.
{
    UINT32 a0, a1, a2, a3, a4;
    UINT32 r0, r1, r2, r3;
    UINT64 t64;
    UINT32 T3;
    UINT32 V0, V1, V2;
    UINT32 cy;
    UINT32 U;
    UINT32 t32;

    r0 = pState->r[0];
    r1 = pState->r[1];
    r2 = pState->r[2];
    r3 = pState->r[3];

    a0 = pState->a[0];
    a1 = pState->a[1];
    a2 = pState->a[2];
    a3 = pState->a[3];
    a4 = pState->a[4];

    // Here we have a4 < 6, but we sometimes decrement a4 to compensate for the
    // 2^128 this function always adds. So we test a4 + 1 < 7
    SYMCRYPT_ASSERT( a4 + 1 < 7 );

    while( cbData >= 16 )
    {
        // Acc += data[0..15] + 2^128
        t64 = (UINT64) a0 + SYMCRYPT_LOAD_LSBFIRST32( pbData +  0 );
        a0 = (UINT32) t64;
        t64 >>= 32;

        t64 += (UINT64) a1 + SYMCRYPT_LOAD_LSBFIRST32( pbData +  4 );
        a1 = (UINT32) t64;
        t64 >>= 32;

        t64 += (UINT64) a2 + SYMCRYPT_LOAD_LSBFIRST32( pbData +  8 );
        a2 = (UINT32) t64;
        t64 >>= 32;

        t64 += (UINT64) a3 + SYMCRYPT_LOAD_LSBFIRST32( pbData + 12 );
        a3 = (UINT32) t64;
        t64 >>= 32;

        a4 = (UINT32) t64 + a4 + 1;   // +1 is the padding '1' which we always apply
        SYMCRYPT_ASSERT( a4 < 8 );

        pbData += 16;
        cbData -=16;

        // Compute S3
        t64 =  SYMCRYPT_MUL32x32TO64( a3, r0 )
             + SYMCRYPT_MUL32x32TO64( a2, r1 )
             + SYMCRYPT_MUL32x32TO64( a1, r2 )
             + SYMCRYPT_MUL32x32TO64( a0, r3 );

        SYMCRYPT_ASSERT( t64 < (1ULL << 62) );

        T3 = (UINT32) t64;
        t64 >>= 32;

        // Compute T = S4 + floor(S3/2^32). We have the floor part in t64 already
        // now add S4 to it
        t64 +=   a4*r0                              // this fits in 32 bits as r0 < 2^28 and a4 < 8
                + SYMCRYPT_MUL32x32TO64( a3, r1 )
                + SYMCRYPT_MUL32x32TO64( a2, r2 )
                + SYMCRYPT_MUL32x32TO64( a1, r3 );

        U = (UINT32) t64 & 3;
        t64 &= ~3;                  // t64 = T4 here

        // Compute S0 + T4 + T4/4, and V0
        t64 += (t64 >> 2) + SYMCRYPT_MUL32x32TO64( a0, r0 );
        V0 = (UINT32)t64;
        cy = (UINT32)(t64 >> 32);     // the carry from S0 to S1

        // Compute S5
        t64 = a4 * r1 + SYMCRYPT_MUL32x32TO64( a3, r2 ) + SYMCRYPT_MUL32x32TO64( a2, r3 );
        t64 += t64 >> 2;    // = S5 + S5/4

        t64 += SYMCRYPT_MUL32x32TO64( a1, r0 ) + SYMCRYPT_MUL32x32TO64( a0, r1 );
        // t64 = S1 + S5 + S5/4

        t64 += cy;
        V1 = (UINT32) t64;
        cy = (UINT32)(t64 >> 32);     // the carry from S1 to S2

        // Compute S6
        t64 = a4 * r2 + SYMCRYPT_MUL32x32TO64( a3, r3 );
        t64 += t64 >> 2;    // S6 + S6/4

        // now add S2
        t64 += SYMCRYPT_MUL32x32TO64( a2, r0 ) + SYMCRYPT_MUL32x32TO64( a1, r1 ) + SYMCRYPT_MUL32x32TO64( a0, r2 );
        t64 += cy;
        V2 = (UINT32) t64;
        cy = (UINT32)(t64 >> 32);

        // Finally T3 + S7 + S7/4
        t32 = a4 * r3;      // =S7, a 32-bit value
        t32 += t32/4;
        t64 = (UINT64) T3 + t32;
        t64 += cy;

        a0 = V0;
        a1 = V1;
        a2 = V2;
        a3 = (UINT32) t64;
        a4 = U + (UINT32)(t64 >> 32);

        SYMCRYPT_ASSERT( a4 < 6 );
    }

    pState->a[0] = a0;
    pState->a[1] = a1;
    pState->a[2] = a2;
    pState->a[3] = a3;
    pState->a[4] = a4;
}


static const BYTE poly1305Kat[16] = {
    0xef, 0x9e, 0x73, 0x2a, 0x7f, 0x2d, 0xf1, 0x85, 0xa7, 0x11, 0x80, 0xae, 0x58, 0x3a, 0x0f, 0x93,
};


VOID
SYMCRYPT_CALL
SymCryptPoly1305Selftest(void)
{
    BYTE res[16];

    SymCryptPoly1305( SymCryptTestKey32, SymCryptTestMsg16, 16, res );

    SymCryptInjectError( res, sizeof( res ) );

    if( memcmp( res, poly1305Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'p135');
    }
}
