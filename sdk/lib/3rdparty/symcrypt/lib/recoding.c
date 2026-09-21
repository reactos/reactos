//
// recoding.c   Algorithms for recoding the factors / exponents in various implementations
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//

#include "precomp.h"

//
// The following is an adaptation of algorithm 6: "Protected
// odd-only recoding algorithm for the fixed-window representation"
// from the paper
//      "Selecting Elliptic Curves for Cryptography: An Efficiency and
//       Security Analysis" by Bos, Costello, Longa, and Naehrig
//
// Input:   odd integer k \in [1,GOrd], window width w>=2, and
//          t = ceil( GOrdBitsize / w-1 )
//
// Output:  (k_t, ... , k_0) where k_i \in {+-1, +-3, ..., +-(2^(w-1) -1)}
//
// Algorithm:
//          for i=0 to (t-1) do
//              k_i = (k mod 2^w) - 2^(w-1)
//              k = (k-k_i)/2^(w-1)
//          k_t = k mod 2^(w-1)
//          return (k_t, ..., k_0)
//
// Remarks:
//          1. An invariant of the main loop is that (k > 0 and k odd). This means
//             that all k_i's are odd and that k_t > 0.
//          2. We will store the values of k_i's as absolute values and signs in
//             absofKIs and sigofKIs arrays, resp. The sigofKIs[i] is 0xffffffff if
//             k_i < 0, otherwise it is 0.
//          3. In the multiplication algorithm we always access the precomputed point
//             P[(|k_i|-1)/2]. Therefore here we just shift the |k_i| value left by
//             one bit before storing it in absofKIs table.
//          4. Caller should check k in range [1,GOrd] to ensure use of recoding will
//             give correct results. This algorithm always recodes the t * (w-1) least
//             significant bits of the provided k, interpreted as an unsigned integer.
//
VOID
SYMCRYPT_CALL
SymCryptFixedWindowRecoding(
            UINT32          W,
    _Inout_ PSYMCRYPT_INT   piK,
    _Inout_ PSYMCRYPT_INT   piTmp,
    _Out_writes_( nRecodedDigits )
            PUINT32         absofKIs,
    _Out_writes_( nRecodedDigits )
            PUINT32         sigofKIs,
            UINT32          nRecodedDigits )
{
    UINT32 T1 = 0;
    UINT32 T2 = 0;
    UINT32 mask = ~(0xffffffff << W);       // Window mask = 2^w - 1 (e.g. 0x0000003f for w = 6)
    UINT32 smask = 0x1 << (W-1);            // Sign mask   = 2^(w-1) (e.g. 0x00000020 for w = 6)

    SYMCRYPT_ASSERT( W < 32 );

    for (UINT32 i=0; i < nRecodedDigits - 1; i++)
    {
        T1 = SymCryptIntGetValueLsbits32( piK ) & mask;         // T1 = k mod 2^W

        // At this point if the w-th bit of T1 is 1 then we know that T1 > 2^(w-1)
        // (Since k = odd is a loop invariant).
        //
        // In this case, (case A), T1 & ~smask is equal to (k mod 2^w) - 2^(w-1) = k_i = |k_i|.
        //
        // Otherwise, (case B), we know that T1 < 2^(w-1). Therefore 2^(w-1) - T1 = |k_i|.

        sigofKIs[i] = SYMCRYPT_MASK32_ZERO( T1 & smask );       // If the sign of k_i is - this mask is set to 0xffffffff. (Case B)

        T2 = T1 & ~smask;                                       // |k_i| in case A
        T1 = smask - T1;                                        // |k_i| in case B

        absofKIs[i] = ((T1 & sigofKIs[i]) | (T2 & ~sigofKIs[i])) >> 1; // Setting (masked) the absolute value of k_i in absofKIs (divided by 2)

        SymCryptIntSubUint32( piK, T2, piTmp );                 // This gives k - k_i in case (A)
        SymCryptIntAddUint32( piK, T1, piK );                   // This gives k - k_i in case (B)

        SymCryptIntMaskedCopy( piTmp, piK, ~sigofKIs[i] );      // Copy the result to piK in case (B)

        SymCryptIntDivPow2( piK, W-1, piK );                    // k := k / 2^(w-1)
    }

    // The last sign is positive given k < GOrd => k_t < 2^w
    sigofKIs[nRecodedDigits - 1] = 0;
    // Belts and braces, select only the bottom w-1 bits (ensure all absofKIs represent odd values in range [1,2^(w-1)-1])
    absofKIs[nRecodedDigits - 1] = (SymCryptIntGetValueLsbits32( piK ) & mask & ~smask) >> 1;
}

//
// The following is an algorithm for computing the width-w NAF of a positive integer.
//
// Input:   integer k \in [1,GOrd), window width w>=2, and nRecodedDigits = GOrdBitsize + 1
//
// Output:  (k_(nRecodedDigits-1), ... , k_0) where k_i \in {0, +-1, +-3, ..., +-(2^(w-1) -1)}
//
// Algorithm:
//          for i = 0 to (nRecodedDigits-1)
//              if (k is odd)
//                  k_i = (k mods 2^w)
//                  k = k - k_i
//              else
//                  k_i = 0
//              k = k/2
//          return (k_(nRecodedDigits-1), ..., k_0)
//
// Note: k mods 2^w is the integer u with (u == k mod 2^w) and (-2^(w-1) <= u < 2^(w-1) ).
//
// Remarks:
//          1. The above algorithm and the implementation are NOT SIDE-CHANNEL SAFE.
//             Therefore, it should only be used when the SYMCRYPT_FLAG_DATA_PUBLIC is
//             specified.
//          2. The multiplication algorithm uses |k_i|/2 as indexes. Therefore we will shift left
//             the absolute value of k_i by 1 bit and store only |k_i|/2.
//          3. Since now the k_i's can be zero we will store the following in sigofKIs:
//                  sigofKIs[i] = 0x00000001    if k_i > 0
//                  sigofKIs[i] = 0x00000000    if k_i = 0
//                  sigofKIs[i] = 0xffffffff    if k_i < 0
//
VOID
SYMCRYPT_CALL
SymCryptWidthNafRecoding(
            UINT32          W,
    _Inout_ PSYMCRYPT_INT   piK,
    _Out_writes_( nRecodedDigits )
            PUINT32         absofKIs,
    _Out_writes_( nRecodedDigits )
            PUINT32         sigofKIs,
            UINT32          nRecodedDigits )
{
    UINT32 T1 = 0;
    UINT32 mask = ~(0xffffffff << W);       // Window mask = 2^w - 1 (e.g. 0x0000003f for w = 6)
    UINT32 modulus = mask + 1;              // 2^w
    UINT32 smask = 0x1 << (W-1);            // Sign mask   = 2^(w-1) (e.g. 0x00000020 for w = 6)

    SYMCRYPT_ASSERT( W < 32 );

    for (UINT32 i=0; i < nRecodedDigits; i++)
    {
        T1 = SymCryptIntGetValueLsbits32( piK ) & mask;         // T1 = k mod 2^W

        if (T1 & 0x1)
        {
            if (T1 > smask)
            {
                sigofKIs[i] = 0xffffffff;
                absofKIs[i] = modulus - T1;                         // 2^W - T1 = |T1 - 2^W|
                SymCryptIntAddUint32( piK, absofKIs[i], piK );      // k-k_i
            }
            else
            {
                // Here (k mod 2^W) is already in the specified range
                sigofKIs[i] = 0x00000001;
                absofKIs[i] = T1;
                SymCryptIntSubUint32( piK, absofKIs[i], piK );      // k-k_i
            }
        }
        else
        {
            absofKIs[i] = 0;
            sigofKIs[i] = 0;
        }

        SymCryptIntDivPow2( piK, 1, piK );                      // k := k / 2
    }
}

//
// The following is an algorithm similar to the above
// but the output is only non-negative (odd) digits.
//
// Requirements:
//      nRecodedDigits == nBitsExp
//
VOID
SYMCRYPT_CALL
SymCryptPositiveWidthNafRecoding(
            UINT32          W,
    _In_    PCSYMCRYPT_INT  piK,
            UINT32          nBitsExp,
    _Out_writes_( nRecodedDigits )
            PUINT32         absofKIs,
            UINT32          nRecodedDigits )
{
    UINT32 T1 = 0;
    UINT32 cntrZ = W;       // Counter that specifies when we filled the last non-zero NAF digit

    SYMCRYPT_ASSERT( nRecodedDigits <= SymCryptIntBitsizeOfObject( piK ) );

    for (UINT32 i=0; i < nRecodedDigits; i++)
    {
        T1 = SymCryptIntGetBits( piK, i, SYMCRYPT_MIN(W, nBitsExp-i) );   // Get a batch of W bits (but don't go over nBitsExp)

        if ((cntrZ>=W) && ((T1 & 0x01) > 0))    // Only store odd digits
        {
            absofKIs[i] = T1;
            cntrZ = 0;
        }
        else
        {
            absofKIs[i] = 0;
        }

        cntrZ++;        // Prepare the counter for the next iteration
    }
}
