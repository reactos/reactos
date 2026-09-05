//
// 3des.c Routines for DES and 3DES
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// This is an updated implementation that is carefully reviewed to be fully copyrighted by
// Microsoft. Our previous implementation was partially based on a very old public domain
// implementation.
// According to Andrew Tucker (atucker) there was a claim many years ago about the copyright of
// our DES code. Working with LCA (legal) it was determined that the DES implementation in RSA32.lib
// was a Microsoft derivative of a public-domain implementation, and therefore is clear of
// any IP issues. To avoid any further claims we have now scrubbed this implementation from all
// copyrightable elements derived from outside sources.
//
// We take non-copyrightable items from the old implementations such as
// - lookup tables
// - algorithm for various bit permutations
// - Any variable & function names that are already MS-generated.
// - Other MS-generated code elements (e.g. SymCrypt integration)
// Some of the considerations we made are:
// Most of the functionality of DES is required by the FIPS standard and there is not much
// choice on how to code it; elements required by the standard are not copyright protected.
// The lookup tables themselves are not copyrightable as they have no artistic expression.
// The format of the lookup tables is almost completely determined by the standard and the algorithm
// used to access them. Any further layout and structure are all standard C conventions.
// Algorithm tricks such as Hoey's IP implementation are not copyrightable but are patentable.
// Fortunately all the techniques we use have been around long enough that any patents have expired.
//

//
// Feb 2018, Niels Ferguson
//

#include "precomp.h"

//
// Tables to describe the DES and 3DES block ciphers so that the generic
// chaining mode functions can use them.
// We have no optimized mode-specific code as the DES block is so slow that there is
// very little to be gained.
//

const SYMCRYPT_BLOCKCIPHER SymCrypt3DesBlockCipher_default = {
    SymCrypt3DesExpandKey,  // PSYMCRYPT_BLOCKCIPHER_EXPAND_KEY    expandKeyFunc;
    SymCrypt3DesEncrypt,    // PSYMCRYPT_BLOCKCIPHER_CRYPT         encryptFunc;
    SymCrypt3DesDecrypt,    // PSYMCRYPT_BLOCKCIPHER_CRYPT         decryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_ECB     ecbEncryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_ECB     ecbDecryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    cbcEncryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    cbcDecryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_MAC_MODE      cbcMacFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    ctrMsbFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_AEADPART_MODE gcmEncryptPartFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_AEADPART_MODE gcmDecryptPartFunc;
    8,                      // SIZE_T                              blockSize;
    sizeof( SYMCRYPT_3DES_EXPANDED_KEY ), // SIZE_T  expandedKeySize;    // = sizeof( SYMCRYPT_XXX_EXPANDED_KEY )
};

const SYMCRYPT_BLOCKCIPHER SymCryptDesBlockCipher_default = {
    SymCryptDesExpandKey,   // PSYMCRYPT_BLOCKCIPHER_EXPAND_KEY    expandKeyFunc;
    SymCryptDesEncrypt,     // PSYMCRYPT_BLOCKCIPHER_CRYPT         encryptFunc;
    SymCryptDesDecrypt,     // PSYMCRYPT_BLOCKCIPHER_CRYPT         decryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_ECB     ecbEncryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_ECB     ecbDecryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    cbcEncryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    cbcDecryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_MAC_MODE      cbcMacFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    ctrMsbFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_AEADPART_MODE gcmEncryptPartFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_AEADPART_MODE gcmDecryptPartFunc;
    8,                      // SIZE_T                              blockSize;
    sizeof( SYMCRYPT_DES_EXPANDED_KEY ), // SIZE_T  expandedKeySize;    // = sizeof( SYMCRYPT_XXX_EXPANDED_KEY )
};

const PCSYMCRYPT_BLOCKCIPHER SymCrypt3DesBlockCipher = &SymCrypt3DesBlockCipher_default;
const PCSYMCRYPT_BLOCKCIPHER SymCryptDesBlockCipher = &SymCryptDesBlockCipher_default;

extern SYMCRYPT_ALIGN_AT(256) const UINT32 SymCryptDesSpbox[8][64];      // Combined S and P tables
extern SYMCRYPT_ALIGN_AT(256) const UINT32 SymCryptDesKeySelect[8][64];


//
// The SWAP_BITS_WITHIN_UINT32 macro swaps bits within a UINT32 value
// SWAP_BITS_WITHIN_UINT32( _value, _shift, _mask )
// swaps each bit in _value selected by _mask with the bit _shift positions to the left
// Thus it swaps (_value & _mask) with (_value & (_mask << _shift))
//

#define SWAP_BITS_WITHIN_UINT32( _value, _shift, _mask ) \
{ \
    UINT32  _tmp; \
    _tmp = ((_value) ^ ((_value) >> (_shift))) & (_mask ); \
    _value = (_value) ^ _tmp ^ (_tmp << (_shift)); \
}

//
// The SWAP_BITS_BETWEEN_UINT32 macro swaps bits between two UINT32 values
// SWAP_BITS_BETWEEN_UINT32( _v1, _v2, _shift, _mask )
// swaps bits in _v1 selected by _mask with bits in _v2 selected by _mask << _shift
//

#define SWAP_BITS_BETWEEN_UINT32( _v1, _v2, _shift, _mask ) \
{ \
    UINT32  _tmp; \
    _tmp = ((_v1) ^ ((_v2) >> (_shift))) & (_mask); \
    _v1 ^= _tmp; \
    _v2 ^= (_tmp << (_shift )); \
}

//
// For each round, a bit that states whether the key schedule shift registers are clocked twice
// The data is straight from the standard.
//
static const BYTE SymCryptDesDoubleShift[16]={0,0,1,1,1,1,1,1,0,1,1,1,1,1,1,0};

//////////////////////////
//  DES
// We just implement DES as 3DES.
// People using DES have bigger problems than bad performance.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDesExpandKey(
    _Out_               PSYMCRYPT_DES_EXPANDED_KEY  pExpandedKey,
    _In_reads_( cbKey ) PCBYTE                      pbKey,
                        SIZE_T                      cbKey )
{
    if( cbKey != 8 )
    {
        //
        // cbKey should be a compile-time constant in most cases,
        // so this should be optimized away
        //
        return SYMCRYPT_WRONG_KEY_SIZE;
    }
    return SymCrypt3DesExpandKey( &pExpandedKey->threeDes, pbKey, cbKey );
}

VOID
SYMCRYPT_CALL
SymCryptDesEncrypt(
    _In_                                    PCSYMCRYPT_DES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_DES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_DES_BLOCK_SIZE ) PBYTE                       pbDst )
{
    SymCrypt3DesEncrypt( &pExpandedKey->threeDes, pbSrc, pbDst );
}

VOID
SYMCRYPT_CALL
SymCryptDesDecrypt(
    _In_                                    PCSYMCRYPT_DES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_DES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_DES_BLOCK_SIZE ) PBYTE                       pbDst )
{
    SymCrypt3DesDecrypt( &pExpandedKey->threeDes, pbSrc, pbDst );
}

//
// The 3DesCbcEncrypt/Decrypt functions are used to make converting code from
// older libraries to SymCrypt easier.
//

VOID
SYMCRYPT_CALL
SymCrypt3DesCbcEncrypt(
    _In_                                        PCSYMCRYPT_3DES_EXPANDED_KEY    pExpandedKey,
    _Inout_updates_( SYMCRYPT_3DES_BLOCK_SIZE ) PBYTE                           pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                          pbSrc,
    _Out_writes_( cbData )                      PBYTE                           pbDst,
                                                SIZE_T                          cbData )
{
    SYMCRYPT_ASSERT( SymCrypt3DesBlockCipher->blockSize == SYMCRYPT_3DES_BLOCK_SIZE );
    SymCryptCbcEncrypt( SymCrypt3DesBlockCipher, pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
}

VOID
SYMCRYPT_CALL
SymCrypt3DesCbcDecrypt(
    _In_                                        PCSYMCRYPT_3DES_EXPANDED_KEY    pExpandedKey,
    _Inout_updates_( SYMCRYPT_3DES_BLOCK_SIZE ) PBYTE                           pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                          pbSrc,
    _Out_writes_( cbData )                      PBYTE                           pbDst,
                                                SIZE_T                          cbData )
{
    SYMCRYPT_ASSERT( SymCrypt3DesBlockCipher->blockSize == SYMCRYPT_3DES_BLOCK_SIZE );
    SymCryptCbcDecrypt( SymCrypt3DesBlockCipher, pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
}



VOID
SYMCRYPT_CALL
SymCryptDesExpandSingleKey(
        _Out_writes_bytes_(128) UINT32  expandedKeyTable[16][2],
        _In_reads_(8)           PCBYTE  pKey )
{
    UINT32 Cr, Dr;      // The C_r D_r values of FIPS 43 for round value r
    UINT32 r;           // round
    UINT32 K1, K2;      // round keys after the permuted choice 2
    UINT32 tmp;

    //
    // We follow the FIPS 43 flow quite closely and have not optimized the key expansion much.
    // Key expansion is not performance-critical.
    //

    // Load the key
    Cr = SYMCRYPT_LOAD_LSBFIRST32( pKey );
    Dr = SYMCRYPT_LOAD_LSBFIRST32( pKey + 4 );

    //
    // The Permuted Choice 1 can be done mostly with a sequence of bit swaps.
    // The algorithm we use is derived from our earlier implementation and might potentially
    // derive from an external source.
    // But the algorithm cannot be copyrighted, only patented, and if there were any patents
    // they have expired by now.
    // The expression of the algorithm in code is purely MS generated, and so not encumbered
    // by external copyrights.
    // This algorithm is really just a transposition of the bits when viewed as an 8x8 matrix
    // with an additional permutation on the output side.
    //
    SWAP_BITS_BETWEEN_UINT32( Cr, Dr, 4, 0x0f0f0f0f );
    SWAP_BITS_WITHIN_UINT32( Dr, 18, 0x00003333 );
    SWAP_BITS_WITHIN_UINT32( Cr, 18, 0x00003333 );
    SWAP_BITS_BETWEEN_UINT32( Cr, Dr, 1, 0x55555555 );
    SWAP_BITS_BETWEEN_UINT32( Dr, Cr, 8, 0x00ff00ff );
    SWAP_BITS_BETWEEN_UINT32( Cr, Dr, 1, 0x55555555 );
    SWAP_BITS_WITHIN_UINT32( Dr, 16, 0xff );

    // Have to re-arrange C and D a tiny bit so that each contains 28 bits and we throw away 8 bits
    Dr = (Dr & 0x00ffffff) | ((Cr & 0xf0000000 ) >> 4 );
    Cr = (Cr & 0x0fffffff);

    for( r = 0; r < 16; r++)
    {
        //
        // Cr and Dr are the two key shift registers, they are rotated once or twice for each round.
        //

        if( SymCryptDesDoubleShift[ r ] ) {
            Cr = ((Cr >> 2) | (Cr << 26));
            Dr = ((Dr >> 2) | (Dr << 26));
        } else {
            Cr = ((Cr >> 1) | (Cr << 27));
            Dr = ((Dr >> 1) | (Dr << 27));
        }

        Cr &= 0x0fffffff;
        Dr &= 0x0fffffff;

        //
        // The Permuted Choice 2 is done using table lookups
        // Not all bits of C and D are used, so we cut those out using shifts and masks,
        // and then index 6 bits at a time into lookup tables that implement the bit relocation.
        //

        K1 = SymCryptDesKeySelect[0][ (Cr      )&0x3f                     ] |
             SymCryptDesKeySelect[1][((Cr >>  6)&0x03) | ((Cr >>  7)&0x3c)] |
             SymCryptDesKeySelect[2][((Cr >> 13)&0x0f) | ((Cr >> 14)&0x30)] |
             SymCryptDesKeySelect[3][((Cr >> 20)&0x01) | ((Cr >> 21)&0x06) | ((Cr >> 22)&0x38)];

        K2 = SymCryptDesKeySelect[4][ (Dr      )&0x3f                     ] |
             SymCryptDesKeySelect[5][((Dr >>  7)&0x03) | ((Dr >>  8)&0x3c)] |
             SymCryptDesKeySelect[6][ (Dr >> 15)&0x3f                     ] |
             SymCryptDesKeySelect[7][((Dr >> 21)&0x0f) | ((Dr >> 22)&0x30)];

        //
        // After this we still have to swap the halves of K1 and K2, that is done below
        // as part of the formatting of the round key
        //

        //
        // So far we have recreated the round keys per the standard.
        // The round keys are stored rotated by 2 as the encrypt/decrypt code finds that easier.
        // We could update the tables to do this, but key expansion is not used that frequently,
        // and it is not worth the effort to update the tables.
        //
        // We don't worry about extraneous bits in unused positions as the F function masks out unused bits.
        //

        tmp = ((K2 << 16) | (K1 & 0x0000ffff)) ;
        expandedKeyTable[r][0] = ROL32(tmp, 2);

        tmp = ((K1 >> 16) | (K2 & 0xffff0000));
        expandedKeyTable[r][1] = ROL32(tmp, 6);
    }
}


SYMCRYPT_NOINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCrypt3DesExpandKey(
    _Out_               PSYMCRYPT_3DES_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                      pbKey,
                        SIZE_T                      cbKey )
{
    SIZE_T keyIndex = 0;
    int i;

    if( cbKey != 8 && cbKey != 16 && cbKey != 24 )
    {
        return SYMCRYPT_WRONG_KEY_SIZE;
    }

    //
    // A loop that goes over the provided key as a circular buffer provides
    // the right result with the least complexity.
    // This is inefficient for the cases cbKey=8 and cbKey=16, but those should
    // not be used anyway.
    //
    for( i=0; i<3; i++ )
    {
        SYMCRYPT_ASSERT( keyIndex <= cbKey - 8 );       // help PreFast
        SymCryptDesExpandSingleKey( pExpandedKey->roundKey[i], pbKey + keyIndex );
        keyIndex = (keyIndex + 8) % cbKey;
    }

    SYMCRYPT_SET_MAGIC( pExpandedKey );

    return SYMCRYPT_NO_ERROR;
}


//
// The DES round function
// This is straight from the standard.
// Ta and Tb each contain 4 sets of 6 bits that are S-box inputs
// We interleave the use of Ta and Tb to provide better CPU scheduling on weak compilers.
// We ensure that the input bits appear in bits 2-7 of the index to avoid a scaled index
// which can be slower on some CPUs.
//
#define F(L, R, keyptr) { \
    Ta = keyptr[0] ^ R; \
    Tb = keyptr[1] ^ R; \
    Tb = ROR32(Tb, 4); \
    L ^= *(UINT32 *)((PBYTE)SymCryptDesSpbox[0] + ( Ta     & 0xfc)); \
    L ^= *(UINT32 *)((PBYTE)SymCryptDesSpbox[1] + ( Tb     & 0xfc)); \
    L ^= *(UINT32 *)((PBYTE)SymCryptDesSpbox[2] + ((Ta>> 8)& 0xfc)); \
    L ^= *(UINT32 *)((PBYTE)SymCryptDesSpbox[3] + ((Tb>> 8)& 0xfc)); \
    L ^= *(UINT32 *)((PBYTE)SymCryptDesSpbox[4] + ((Ta>>16)& 0xfc)); \
    L ^= *(UINT32 *)((PBYTE)SymCryptDesSpbox[5] + ((Tb>>16)& 0xfc)); \
    L ^= *(UINT32 *)((PBYTE)SymCryptDesSpbox[6] + ((Ta>>24)& 0xfc)); \
    L ^= *(UINT32 *)((PBYTE)SymCryptDesSpbox[7] + ((Tb>>24)& 0xfc)); }



//
// Block encryption.
// The noinline stops the compiler from inlining the code and creating additional
// implementations which would require separate FIPS selftests.
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCrypt3DesEncrypt(
    _In_                                        PCSYMCRYPT_3DES_EXPANDED_KEY    pExpandedKey,
    _In_reads_( SYMCRYPT_3DES_BLOCK_SIZE )      PCBYTE                          pbSrc,
    _Out_writes_( SYMCRYPT_3DES_BLOCK_SIZE )    PBYTE                           pbDst )
{
    UINT32   L, R, Ta, Tb;
    int r;

    SYMCRYPT_CHECK_MAGIC( pExpandedKey );

    R = SYMCRYPT_LOAD_LSBFIRST32( pbSrc );
    L = SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 4 );

    //
    // Hoey's wonderful initial permutation algorithm, from Outerbridge
    // (see Schneier p 478)
    //
    // The algorithm we use is derived (through several intermediate forms) from the mentioned source.
    // But the algorithm cannot be copyrighted, only patented, and if there were any patents
    // they have expired by now.
    // The expression of the algorithm in code is purely MS generated,
    // within the confines of implementing the algorithm in the best way such that even a simple
    // compiler will create good code.
    //

    R = ROL32(R, 4);
    Ta = (L ^ R) & 0xf0f0f0f0;
    L ^= Ta;
    R ^= Ta;

    L = ROL32(L, 20);
    Ta = (L ^ R) & 0xfff0000f;
    R ^= Ta;
    L ^= Ta;

    L = ROL32(L,14);
    Ta = (L ^ R) & 0x33333333;
    R ^= Ta;
    L ^= Ta;

    R = ROL32(R, 22);
    Ta = (L ^ R) & 0x03fc03fc;
    R ^= Ta;
    L ^= Ta;

    R = ROL32(R, 9);
    Ta = (L ^ R) & 0xaaaaaaaa;
    R ^= Ta;
    L ^= Ta;

    L = ROL32(L, 1);

    //
    // First: encryption
    //
    for( r=0; r<16; r += 2 )
    {
        F( L, R, pExpandedKey->roundKey[0][r  ] );
        F( R, L, pExpandedKey->roundKey[0][r+1] );
    }

    //
    // Second: decryption
    // Note that L and R are swapped here, and the round counter counts down.
    //
    for( r=14; r>=0; r -= 2 )
    {
        F( R, L, pExpandedKey->roundKey[1][r+1] );
        F( L, R, pExpandedKey->roundKey[1][r  ] );
    }

    //
    // Third: encryption
    //
    for( r=0; r<16; r += 2 )
    {
        F( L, R, pExpandedKey->roundKey[2][r  ] );
        F( R, L, pExpandedKey->roundKey[2][r+1] );
    }

    R = ROR32(R, 1);
    Ta = (L ^ R) & 0xaaaaaaaa;
    R ^= Ta;
    L ^= Ta;

    L = ROR32(L, 9);
    Ta = (L ^ R) & 0x03fc03fc;
    R ^= Ta;
    L ^= Ta;

    L = ROR32(L, 22);
    Ta = (L ^ R) & 0x33333333;
    R ^= Ta;
    L ^= Ta;

    R = ROR32(R, 14);
    Ta = (L ^ R) & 0xfff0000f;
    R ^= Ta;
    L ^= Ta;

    R = ROR32(R, 20);
    Ta = (L ^ R) & 0xf0f0f0f0;
    R ^= Ta;
    L ^= Ta;

    L = ROR32(L, 4);

    SYMCRYPT_STORE_LSBFIRST32( pbDst, L );
    SYMCRYPT_STORE_LSBFIRST32( pbDst + 4, R );
}


//
// Block decrypt
// The noinline stops the compiler from inlining the code and creating additional
// implementations which would require separate FIPS selftests.
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCrypt3DesDecrypt(
    _In_                                        PCSYMCRYPT_3DES_EXPANDED_KEY    pExpandedKey,
    _In_reads_( SYMCRYPT_3DES_BLOCK_SIZE )      PCBYTE                          pbSrc,
    _Out_writes_( SYMCRYPT_3DES_BLOCK_SIZE )    PBYTE                           pbDst )
{
    UINT32   L, R, Ta, Tb;
    int r;

    SYMCRYPT_CHECK_MAGIC( pExpandedKey );

    R = SYMCRYPT_LOAD_LSBFIRST32( pbSrc );
    L = SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 4 );

    R = ROL32(R, 4);
    Ta = (L ^ R) & 0xf0f0f0f0;
    L ^= Ta;
    R ^= Ta;

    L = ROL32(L, 20);
    Ta = (L ^ R) & 0xfff0000f;
    L ^= Ta;
    R ^= Ta;

    L = ROL32(L, 14);
    Ta = (L ^ R) & 0x33333333;
    L ^= Ta;
    R ^= Ta;

    R = ROL32(R, 22);
    Ta = (L ^ R) & 0x03fc03fc;
    L ^= Ta;
    R ^= Ta;

    R = ROL32(R, 9);
    Ta = (L ^ R) & 0xaaaaaaaa;
    L ^= Ta;
    R ^= Ta;

    L = ROL32(L, 1);


    // Decrypt with key 2
    for( r=14; r>=0; r -= 2 )
    {
        F( L, R, pExpandedKey->roundKey[2][r+1] );
        F( R, L, pExpandedKey->roundKey[2][r  ] );
    }

    // Encrypt with key 1
    for( r=0; r<16; r += 2 )
    {
        F( R, L, pExpandedKey->roundKey[1][r  ] );
        F( L, R, pExpandedKey->roundKey[1][r+1] );
    }

    // Decrypt with key 0
    for( r=14; r>=0; r -= 2 )
    {
        F( L, R, pExpandedKey->roundKey[0][r+1] );
        F( R, L, pExpandedKey->roundKey[0][r  ] );
    }

    /* Inverse permutation, also from Hoey via Outerbridge and Schneier */

    R = ROR32(R, 1);
    Ta = (L ^ R) & 0xaaaaaaaa;
    L ^= Ta;
    R ^= Ta;

    L = ROR32(L, 9);
    Ta = (L ^ R) & 0x03fc03fc;
    L ^= Ta;
    R ^= Ta;

    L = ROR32(L, 22);
    Ta = (L ^ R) & 0x33333333;
    L ^= Ta;
    R ^= Ta;

    R = ROR32(R, 14);
    Ta = (L ^ R) & 0xfff0000f;
    L ^= Ta;
    R ^= Ta;

    R = ROR32(R, 20);
    Ta = (L ^ R) & 0xf0f0f0f0;
    L ^= Ta;
    R ^= Ta;

    L = ROR32(L, 4);

    SYMCRYPT_STORE_LSBFIRST32( pbDst, L );
    SYMCRYPT_STORE_LSBFIRST32( pbDst + 4, R );
}



VOID
SYMCRYPT_CALL
SymCryptDesSetOddParity(
    _Inout_updates_( cbData )   PBYTE   pbData,
    _In_                        SIZE_T  cbData )
//
// For each byte, set bit 0 such that the byte parity is odd.
// This function is side-channel safe
//
{
    SIZE_T i;
    BYTE b, t;
    for( i=0; i<cbData; i++ )
    {
        // We obey the read-once write-once rule
        b = *pbData;

        t = b ^ (b>>4);     // parity(b) = parity( t & 0xf )
        t ^= t>>2;          //           = parity( t & 0x3 )
        t ^= t>>1;          //           = parity( t & 0x1 )
        *pbData++ = b ^ (t&1) ^ 1;
    }
}


//
// Test vectors for self test
//
static const BYTE SP800_67Key[24] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01,
    0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23,
};

static const BYTE des3KnownPlaintext[8] = {
    0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74,
};

static const BYTE des3KnownCiphertext[8] = {
    0x31, 0x4F, 0x83, 0x27, 0xFA, 0x7A, 0x09, 0xA8,
};

static const BYTE desKnownCiphertext[8] = {
    0x3F, 0xA4, 0x0E, 0x8A, 0x98, 0x4D, 0x48, 0x15,
};


VOID
SYMCRYPT_CALL
SymCryptDesSelftest(void)
{
    BYTE                        buf[SYMCRYPT_DES_BLOCK_SIZE];
    SYMCRYPT_DES_EXPANDED_KEY   key;

    if( SymCryptDesExpandKey( &key, SP800_67Key, 8 ) != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'desa' );
    }

    SymCryptDesEncrypt( &key, des3KnownPlaintext, buf );

    SymCryptInjectError( buf, SYMCRYPT_DES_BLOCK_SIZE );

    if( memcmp( buf, desKnownCiphertext, SYMCRYPT_DES_BLOCK_SIZE ) != 0 )
    {
        SymCryptFatal( 'desb' );
    }

    SymCryptDesDecrypt( &key, desKnownCiphertext, buf );

    SymCryptInjectError( buf, SYMCRYPT_DES_BLOCK_SIZE );

    if( memcmp( buf, des3KnownPlaintext, SYMCRYPT_DES_BLOCK_SIZE ) != 0 )
    {
        SymCryptFatal( 'desc' );
    }
}


VOID
SYMCRYPT_CALL
SymCrypt3DesSelftest(void)
{
    BYTE                        buf[SYMCRYPT_3DES_BLOCK_SIZE];
    SYMCRYPT_3DES_EXPANDED_KEY  key;

    if( SymCrypt3DesExpandKey( &key, SP800_67Key, 24 ) != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'des3' );
    }

    SymCrypt3DesEncrypt( &key, des3KnownPlaintext, buf );

    SymCryptInjectError( buf, SYMCRYPT_3DES_BLOCK_SIZE );

    if( memcmp( buf, des3KnownCiphertext, SYMCRYPT_3DES_BLOCK_SIZE ) != 0 )
    {
        SymCryptFatal( 'des4' );
    }

    SymCrypt3DesDecrypt( &key, des3KnownCiphertext, buf );

    SymCryptInjectError( buf, SYMCRYPT_3DES_BLOCK_SIZE );

    if( memcmp( buf, des3KnownPlaintext, SYMCRYPT_3DES_BLOCK_SIZE ) != 0 )
    {
        SymCryptFatal( 'des5' );
    }
}





#if 0
////////////////////////////////////////////
// Useful tables, kept for future reference.
//

// Tables defined in the Data Encryption Standard documents
// Three of these tables, the initial permutation, the final
// permutation and the expansion operator, are regular enough that
// for speed, we hard-code them. They're here for reference only.
// Also, the S and P boxes are used by a separate program, gensp.c,
// to build the combined SP box, Spbox[]. They're also here just
// for reference.
//
// initial permutation IP
static unsigned BYTE ip[] = {
    58, 50, 42, 34, 26, 18, 10,  2,
    60, 52, 44, 36, 28, 20, 12,  4,
    62, 54, 46, 38, 30, 22, 14,  6,
    64, 56, 48, 40, 32, 24, 16,  8,
    57, 49, 41, 33, 25, 17,  9,  1,
    59, 51, 43, 35, 27, 19, 11,  3,
    61, 53, 45, 37, 29, 21, 13,  5,
    63, 55, 47, 39, 31, 23, 15,  7
};

// final permutation IP^-1
static unsigned BYTE fp[] = {
    40,  8, 48, 16, 56, 24, 64, 32,
    39,  7, 47, 15, 55, 23, 63, 31,
    38,  6, 46, 14, 54, 22, 62, 30,
    37,  5, 45, 13, 53, 21, 61, 29,
    36,  4, 44, 12, 52, 20, 60, 28,
    35,  3, 43, 11, 51, 19, 59, 27,
    34,  2, 42, 10, 50, 18, 58, 26,
    33,  1, 41,  9, 49, 17, 57, 25
};

// expansion operation matrix
static unsigned BYTE ei[] = {
    32,  1,  2,  3,  4,  5,
     4,  5,  6,  7,  8,  9,
     8,  9, 10, 11, 12, 13,
    12, 13, 14, 15, 16, 17,
    16, 17, 18, 19, 20, 21,
    20, 21, 22, 23, 24, 25,
    24, 25, 26, 27, 28, 29,
    28, 29, 30, 31, 32,  1
};

// The (in)famous S-boxes
static unsigned BYTE sbox[8][64] = {
    // S1
    14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
     0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
     4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
    15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13,

    // S2
    15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
     3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
     0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
    13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9,

    // S3
    10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
    13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
    13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
     1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12,

    // S4
     7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
    13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
    10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
     3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14,

    // S5
     2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
    14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
     4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
    11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3,

    // S6
    12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
    10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
     9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
     4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13,

    // S7
     4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
    13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
     1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
     6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12,

    // S8
    13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
     1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
     7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
     2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
};

// 32-bit permutation function P used on the output of the S-boxes
static unsigned BYTE p32i[] = {
    16,  7, 20, 21,
    29, 12, 28, 17,
     1, 15, 23, 26,
     5, 18, 31, 10,
     2,  8, 24, 14,
    32, 27,  3,  9,
    19, 13, 30,  6,
    22, 11,  4, 25
};

// permuted choice table (key)
static unsigned BYTE pc1[] = {
    57, 49, 41, 33, 25, 17,  9,
     1, 58, 50, 42, 34, 26, 18,
    10,  2, 59, 51, 43, 35, 27,
    19, 11,  3, 60, 52, 44, 36,

    63, 55, 47, 39, 31, 23, 15,
     7, 62, 54, 46, 38, 30, 22,
    14,  6, 61, 53, 45, 37, 29,
    21, 13,  5, 28, 20, 12,  4
};

// number left rotations of pc1
static unsigned BYTE totrot[] = {
    1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28
};

// permuted choice key (table)
static unsigned BYTE pc2[] = {
    14, 17, 11, 24,  1,  5,
     3, 28, 15,  6, 21, 10,
    23, 19, 12,  4, 26,  8,
    16,  7, 27, 20, 13,  2,
    41, 52, 31, 37, 47, 55,
    30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53,
    46, 42, 50, 36, 29, 32
};

#endif
