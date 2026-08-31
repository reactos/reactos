//
// equal.c  Memory comparison routine that is safe against side channels.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

BOOLEAN
SYMCRYPT_CALL
SymCryptEqual(  _In_reads_( cbBytes )  PCBYTE pbSrc1,
                _In_reads_( cbBytes )  PCBYTE pbSrc2,
                                       SIZE_T cbBytes )
{
    UINT32 neq = 0;
    BYTE b;
    volatile BYTE * p1 = (volatile BYTE *) pbSrc1;
    volatile BYTE * p2 = (volatile BYTE *) pbSrc2;

    //
    // We use forced-access memory reads to ensure that the compiler doesn't get
    // smart and implement an early-out solution.
    //

    while( cbBytes >= 4 )
    {
        neq |= SYMCRYPT_FORCE_READ32( (volatile UINT32 *) p1 ) ^ SYMCRYPT_FORCE_READ32( (volatile UINT32 *) p2 );
        p1 += 4;
        p2 += 4;
        cbBytes -= 4;
    }

    // We have to deal with the remaining bytes using a separate accumulator to work around an issue in the ARM64 compiler.
    if( cbBytes > 0 )
    {
        b = 0;
        while( cbBytes > 0 )
        {
            b |= SYMCRYPT_FORCE_READ8( p1 ) ^ SYMCRYPT_FORCE_READ8( p2 );
            p1++;
            p2++;
            cbBytes--;
        }
        neq |= b;
    }

    return neq == 0;
}
