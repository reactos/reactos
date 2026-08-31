//
// Rc4.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// This is a new implementation, NOT based on the existing ones in RSA32.lib.
// The algorithm specification is taken from "ARCFOUR Algorithm" internet
// draft dated July 1999, and from memory.
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRc4Init(
    _Out_                   PSYMCRYPT_RC4_STATE pState,
    _In_reads_( cbKey )     PCBYTE              pbKey,
    _In_                    SIZE_T              cbKey )
{
    SIZE_T i;
    SIZE_T j;
    BYTE keyBuf[256];
    SIZE_T keyIdx;

    SYMCRYPT_RC4_S_TYPE T;

    if( cbKey > 256 || cbKey == 0 )
    {
        return SYMCRYPT_WRONG_KEY_SIZE;
    }

    //
    // Make a copy of the key to obey the read-once rule.
    //  This is a case where it looks safe to break the read-once
    //  rule, but it isn't. RC4 with very long keys (e.g. 256 bytes)
    //  is actually very vulnerable against related-key attacks.
    //  One obvious precaution is to limit the length of the RC4 key,
    //  which one of the layers above us might do.
    //  Allowing the key bytes to change as we read them negates
    //  this countermeasure.
    //
    memcpy( keyBuf, pbKey, cbKey );

    for( i=0; i<256; i++ )
    {
        pState->S[i] = (SYMCRYPT_RC4_S_TYPE) i;
    }

    j = 0;
    keyIdx = 0;
    for( i=0; i<256; i++ )
    {

        T = pState->S[i];
        j = (j + T + keyBuf[keyIdx]) & 0xff;
        pState->S[i] = pState->S[j];
        pState->S[j] = T;
        keyIdx++;
        if( keyIdx == cbKey )
        {
            keyIdx = 0;
        }
    }

    //
    // We store the i value already incremented for the next byte.
    // This seems to allow better instruction sequencing interleaving in the actual en/decrypt loop
    //
    pState->i = 1;
    pState->j = 0;

    SYMCRYPT_SET_MAGIC( pState );

    SymCryptWipe( keyBuf, cbKey );

    return SYMCRYPT_NO_ERROR;
}


VOID
SYMCRYPT_CALL
SymCryptRc4Crypt(
    _Inout_                 PSYMCRYPT_RC4_STATE pState,
    _In_reads_( cbData )   PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
    _In_                    SIZE_T              cbData )
{
    SIZE_T i;
    SIZE_T j;
    SYMCRYPT_RC4_S_TYPE Ti;
    SYMCRYPT_RC4_S_TYPE Tj;
    PCBYTE pbSrcEnd = pbSrc + cbData;

    SYMCRYPT_CHECK_MAGIC( pState );

    i = pState->i;
    j = pState->j;

    //
    // I tried to unroll this loop 4x and use a single 32-bit operation to XOR the key
    // stream with the data. This actually makes the code slower by 1 c/B on a Core 2.
    // I suspect that that is because the instruction decoders are the bottleneck, and
    // a small loop can be run out of the uop queue which bypasses the instruction decoders.
    // A larger loop has to be decoded every time, and that slows things down.
    // The theoretical gain of unrolling the loop is less than 1 c/B,
    // and as Core 2 and derived CPUs are the most commonly used CPUs by our customers,
    // it is not worthwhile to persue this further.
    //
    //  - Niels Ferguson (niels) 2010-10-11
    //

    while( pbSrc < pbSrcEnd )
    {
        //
        // Our i value is already incremented
        //
        Ti = pState->S[i];
        j = (j + Ti ) & 0xff;
        Tj = pState->S[j];
        pState->S[i] = Tj;
        pState->S[j] = Ti;
        *pbDst = (BYTE) (*pbSrc ^ pState->S[(Ti + Tj) & 0xff]);

        i = (i + 1) & 0xff;

        pbSrc++;
        pbDst++;
    }

    pState->i = (BYTE) i;
    pState->j = (BYTE) j;
}


static const BYTE   rc4KatAnswer[ 3 ] = { 0x71, 0x46, 0x92 };


VOID
SYMCRYPT_CALL
SymCryptRc4Selftest(void)
{
    BYTE buf[3];
    SYMCRYPT_RC4_STATE  state;

    SymCryptRc4Init( &state, SymCryptTestKey32, sizeof( SymCryptTestKey32 ) );

    SymCryptRc4Crypt( &state, SymCryptTestMsg3, buf, sizeof( buf ) );

    SymCryptInjectError( buf, sizeof( buf ) );

    if( memcmp( buf, rc4KatAnswer, sizeof( buf )) != 0 )
    {
        SymCryptFatal( 'rc4 ' );
    }

}
