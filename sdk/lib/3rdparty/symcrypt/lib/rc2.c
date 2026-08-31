//
// Rc2.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module contains the routines to implement RC2 from RFC 2268
//
// This is a new implementation, based on the RFC specification
// and NOT based on the existing one in RSA32.lib,	which is the one from RSA data security.
//


#include "precomp.h"

const SYMCRYPT_BLOCKCIPHER SymCryptRc2BlockCipher_default = {
    SymCryptRc2ExpandKey,   // PSYMCRYPT_BLOCKCIPHER_EXPAND_KEY    expandKeyFunc;
    SymCryptRc2Encrypt,     // PSYMCRYPT_BLOCKCIPHER_CRYPT         encryptFunc;
    SymCryptRc2Decrypt,     // PSYMCRYPT_BLOCKCIPHER_CRYPT         decryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_ECB     ecbEncryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_ECB     ecbDecryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    cbcEncryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    cbcDecryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_MAC_MODE      cbcMacFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    ctrMsbFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_AEADPART_MODE gcmEncryptPartFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_AEADPART_MODE gcmDecryptPartFunc;
    8,                      // SIZE_T                              blockSize;
    sizeof( SYMCRYPT_RC2_EXPANDED_KEY ), // SIZE_T  expandedKeySize;    // = sizeof( SYMCRYPT_XXX_EXPANDED_KEY )
};

const PCSYMCRYPT_BLOCKCIPHER SymCryptRc2BlockCipher = &SymCryptRc2BlockCipher_default;

/*
 *
 *  constants
 *
 */
static  const BYTE    PITABLE[] =
{
    0xd9, 0x78, 0xf9, 0xc4, 0x19, 0xdd, 0xb5, 0xed, 0x28, 0xe9, 0xfd, 0x79, 0x4a, 0xa0, 0xd8, 0x9d,
    0xc6, 0x7e, 0x37, 0x83, 0x2b, 0x76, 0x53, 0x8e, 0x62, 0x4c, 0x64, 0x88, 0x44, 0x8b, 0xfb, 0xa2,
    0x17, 0x9a, 0x59, 0xf5, 0x87, 0xb3, 0x4f, 0x13, 0x61, 0x45, 0x6d, 0x8d, 0x09, 0x81, 0x7d, 0x32,
    0xbd, 0x8f, 0x40, 0xeb, 0x86, 0xb7, 0x7b, 0x0b, 0xf0, 0x95, 0x21, 0x22, 0x5c, 0x6b, 0x4e, 0x82,
    0x54, 0xd6, 0x65, 0x93, 0xce, 0x60, 0xb2, 0x1c, 0x73, 0x56, 0xc0, 0x14, 0xa7, 0x8c, 0xf1, 0xdc,
    0x12, 0x75, 0xca, 0x1f, 0x3b, 0xbe, 0xe4, 0xd1, 0x42, 0x3d, 0xd4, 0x30, 0xa3, 0x3c, 0xb6, 0x26,
    0x6f, 0xbf, 0x0e, 0xda, 0x46, 0x69, 0x07, 0x57, 0x27, 0xf2, 0x1d, 0x9b, 0xbc, 0x94, 0x43, 0x03,
    0xf8, 0x11, 0xc7, 0xf6, 0x90, 0xef, 0x3e, 0xe7, 0x06, 0xc3, 0xd5, 0x2f, 0xc8, 0x66, 0x1e, 0xd7,
    0x08, 0xe8, 0xea, 0xde, 0x80, 0x52, 0xee, 0xf7, 0x84, 0xaa, 0x72, 0xac, 0x35, 0x4d, 0x6a, 0x2a,
    0x96, 0x1a, 0xd2, 0x71, 0x5a, 0x15, 0x49, 0x74, 0x4b, 0x9f, 0xd0, 0x5e, 0x04, 0x18, 0xa4, 0xec,
    0xc2, 0xe0, 0x41, 0x6e, 0x0f, 0x51, 0xcb, 0xcc, 0x24, 0x91, 0xaf, 0x50, 0xa1, 0xf4, 0x70, 0x39,
    0x99, 0x7c, 0x3a, 0x85, 0x23, 0xb8, 0xb4, 0x7a, 0xfc, 0x02, 0x36, 0x5b, 0x25, 0x55, 0x97, 0x31,
    0x2d, 0x5d, 0xfa, 0x98, 0xe3, 0x8a, 0x92, 0xae, 0x05, 0xdf, 0x29, 0x10, 0x67, 0x6c, 0xba, 0xc9,
    0xd3, 0x00, 0xe6, 0xcf, 0xe1, 0x9e, 0xa8, 0x2c, 0x63, 0x16, 0x01, 0x3f, 0x58, 0xe2, 0x89, 0xa9,
    0x0d, 0x38, 0x34, 0x1b, 0xab, 0x33, 0xff, 0xb0, 0xbb, 0x48, 0x0c, 0x5f, 0xb9, 0xb1, 0xcd, 0x2e,
    0xc5, 0xf3, 0xdb, 0x47, 0xe5, 0xa5, 0x9c, 0x77, 0x0a, 0xa6, 0x20, 0x68, 0xfe, 0x7f, 0xc1, 0xad
};

/*
 *
 *  macros
 *
 */


/*
 * These are the original macros we derived directly from the RFC.
 * To improve the perf we changed to using R0, R1, R2, R3 variables rather
 * than an array.
 */

/*
#define MIX(R, K, i, j, S)    {\
            R[i] = R[i] + K[j] + (R[(i-1)&3] & R[(i-2)&3]) + ((~R[(i-1)&3]) & R[(i-3)&3]);\
            j = j + 1;\
            R[i] = ROL16(R[i], S);\
    }

#define MIXROUND(R, K, j) {\
            MIX(R, K, 0, j, 1);\
            MIX(R, K, 1, j, 2);\
            MIX(R, K, 2, j, 3);\
            MIX(R, K, 3, j, 5);\
    }

#define MASH(R, K, i) \
            R[i] = R[i] + K[R[(i-1)&3]&63];

#define MASHROUND(R, K) {\
            MASH(R, K, 0);\
            MASH(R, K, 1);\
            MASH(R, K, 2);\
            MASH(R, K, 3);\
    }

//
//  decrypt macros
//


#define RMIX(R, K, i, j, S)   {\
            R[i] = ROR16( R[i], S );\
            R[i] = R[i] - K[j] - (R[(i-1)&3] & R[(i-2)&3]) - ((~R[(i-1)&3]) & R[(i-3)&3]);\
            j = j - 1;\
    }

#define RMIXROUND(R, K, j)   {\
            RMIX(R, K, 3, j, 5);\
            RMIX(R, K, 2, j, 3);\
            RMIX(R, K, 1, j, 2);\
            RMIX(R, K, 0, j, 1);\
    }

#define RMASH(R, K, i) \
            R[i] = R[i] - K[R[(i-1)&3] & 63];

#define RMASHROUND(R, K)    {\
            RMASH(R, K, 3);\
            RMASH(R, K, 2);\
            RMASH(R, K, 1);\
            RMASH(R, K, 0);\
    }
*/


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRc2ExpandKeyEx(
    _Out_               PSYMCRYPT_RC2_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbKey)  PCBYTE                      pbKey,
                        SIZE_T                      cbKey,
                        UINT32                      effectiveKeySizeInBits )
{
    SYMCRYPT_ALIGN BYTE   L[128];
    UINT32  T;
    UINT32  T1;
    UINT32  T8;
    UINT32  TM;
    int     i;

    SYMCRYPT_SET_MAGIC( pExpandedKey );

    //
    // According to RFC 2268 any key size in 1..128 is allowed.
    //
    // The effective key size cannot be 0 as the RFC specs would lead to a buffer overflow
    // in the key expansion.
    //
    // If the effective key size <= 8 then T8=1 and the key expansion backward recursion
    // drops into a fixed point because L[i+1] xor L[i+T8] is zero.
    // Therefore, we require an effective key size of at least 9.
    //
    if( cbKey < 1 || cbKey > 128 || effectiveKeySizeInBits < 9 || effectiveKeySizeInBits > 8*128 )
    {
        return SYMCRYPT_WRONG_KEY_SIZE;
    }

    T = (UINT32)cbKey;              // 1 <= T1 <= 128

    T1 = effectiveKeySizeInBits;    // 9 <= T1 <= 1024
    T8 = (T1+7)/8;                  //  2 <= T8 <= 128

    TM = 255 & ((1 << (8 + (UINT32)T1 - 8*T8))-1);

    //
    // To be endian-agnostic our expanded key is stored as an array of UINT16s. We do the key
    // expansion in a local buffer and copy the values into the expanded key using the proper conversion.
    //
    memcpy(L, pbKey, T);

    for(i = T; i <= 127; i++)
    {
        L[i] = PITABLE[(L[i-1]+L[i-T]) & 0xff];
        //
        // If the key size T=1 then we lose one bit of key space in the key expansion because
        // L[i-1] == L[i-T] which makes the index to PITABLE even. So L[1..128] depend only on
        // 7 bits.
        //
    }

    L[128-T8] = PITABLE[L[128-T8] & TM];

    for( i = 127-T8; i >=0; i--)
    {
        L[i] = PITABLE[L[i+1] ^ L[i+T8]];
    }

    //
    // Now we copy the result into the UINT16 array in our expanded key.
    // This is a memcpy for little-endian platforms, but this code works on all CPUs.
    //
    for( i=0; i<64; i++ )
    {
        pExpandedKey->K[i] = SYMCRYPT_LOAD_LSBFIRST16( &L[2*i] );
    }

    SymCryptWipeKnownSize( L, sizeof( L ) );

    return SYMCRYPT_NO_ERROR;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRc2ExpandKey(
    _Out_               PSYMCRYPT_RC2_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                      pbKey,
                        SIZE_T                      cbKey )
{
    return SymCryptRc2ExpandKeyEx( pExpandedKey, pbKey, cbKey, (UINT32) (8*cbKey) );
}


#define MIXROUND( n ) {\
            R0 = R0 + K[4*n] + (R3 & R2) + (~R3 & R1); \
            R0 = ROL16( R0, 1 ); \
            R1 = R1 + K[4*n+1] + (R0 & R3) + (~R0 & R2 ); \
            R1 = ROL16( R1, 2 ); \
            R2 = R2 + K[4*n+2] + (R1 & R0) + (~R1 & R3); \
            R2 = ROL16( R2, 3 ); \
            R3 = R3 + K[4*n+3] + (R2 & R1) + (~R2 & R0 ); \
            R3 = ROL16( R3, 5 ); \
    }

#define MASHROUND() { \
        R0 = R0 + K[R3 & 63]; \
        R1 = R1 + K[R0 & 63]; \
        R2 = R2 + K[R1 & 63]; \
        R3 = R3 + K[R2 & 63]; \
    }

VOID
SYMCRYPT_CALL
SymCryptRc2Encrypt(
    _In_                                    PCSYMCRYPT_RC2_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_RC2_BLOCK_SIZE )  PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_RC2_BLOCK_SIZE ) PBYTE                       pbDst )
{
    UINT16 R0, R1, R2, R3;
    PCUINT16 K;

    SYMCRYPT_CHECK_MAGIC( pExpandedKey );

    //
    //  1. Initialize words R[0], ..., R[3] to contain the 64-bit plaintext value.
    //
    R0 = SYMCRYPT_LOAD_LSBFIRST16( &pbSrc[0] );
    R1 = SYMCRYPT_LOAD_LSBFIRST16( &pbSrc[2] );
    R2 = SYMCRYPT_LOAD_LSBFIRST16( &pbSrc[4] );
    R3 = SYMCRYPT_LOAD_LSBFIRST16( &pbSrc[6] );

    //
    //  2. Expand the key, so that words K[0], ..., K[63] become defined.
    //  (In our case the key was previously expanded, so we just grab the pointer to it.)
    //
    K = pExpandedKey->K;

    //
    //  3. Initialize j to zero.
    //

    //
    //  4. Perform five mixing rounds.
    //
    MIXROUND(0);
    MIXROUND(1);
    MIXROUND(2);
    MIXROUND(3);
    MIXROUND(4);

    //
    //  5. Perform one mashing round.
    //
    MASHROUND();

    //
    //  6. Perform six mixing rounds.
    //
    MIXROUND(5);
    MIXROUND(6);
    MIXROUND(7);
    MIXROUND(8);
    MIXROUND(9);
    MIXROUND(10);

    //
    //  7. Perform one mashing round.
    //
    MASHROUND();

    //
    //  8. Perform five mixing rounds.
    //
    MIXROUND(11);
    MIXROUND(12);
    MIXROUND(13);
    MIXROUND(14);
    MIXROUND(15);

    SYMCRYPT_STORE_LSBFIRST16( &pbDst[0], R0 );
    SYMCRYPT_STORE_LSBFIRST16( &pbDst[2], R1 );
    SYMCRYPT_STORE_LSBFIRST16( &pbDst[4], R2 );
    SYMCRYPT_STORE_LSBFIRST16( &pbDst[6], R3 );

}


#define RMIXROUND( n ) {\
            R3 = ROR16( R3, 5 ); \
            R3 = R3 - K[4*n+3] - (R2 & R1) - (~R2 & R0 ); \
            R2 = ROR16( R2, 3 ); \
            R2 = R2 - K[4*n+2] - (R1 & R0) - (~R1 & R3); \
            R1 = ROR16( R1, 2 ); \
            R1 = R1 - K[4*n+1] - (R0 & R3) - (~R0 & R2 ); \
            R0 = ROR16( R0, 1 ); \
            R0 = R0 - K[4*n  ] - (R3 & R2) - (~R3 & R1); \
        }

#define RMASHROUND() { \
        R3 = R3 - K[R2 & 63]; \
        R2 = R2 - K[R1 & 63]; \
        R1 = R1 - K[R0 & 63]; \
        R0 = R0 - K[R3 & 63]; \
    }



VOID
SYMCRYPT_CALL
SymCryptRc2Decrypt(
    _In_                                    PCSYMCRYPT_RC2_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_RC2_BLOCK_SIZE )  PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_RC2_BLOCK_SIZE ) PBYTE                       pbDst )
{
    UINT16 R0, R1, R2, R3;
    PCUINT16 K;

    SYMCRYPT_CHECK_MAGIC( pExpandedKey );

    //
    //  1. Initialize words R[0], ..., R[3] to contain the 64-bit plaintext value.
    //
    R0 = SYMCRYPT_LOAD_LSBFIRST16( &pbSrc[0] );
    R1 = SYMCRYPT_LOAD_LSBFIRST16( &pbSrc[2] );
    R2 = SYMCRYPT_LOAD_LSBFIRST16( &pbSrc[4] );
    R3 = SYMCRYPT_LOAD_LSBFIRST16( &pbSrc[6] );

    //
    //  2. Expand the key, so that words K[0], ..., K[63] become defined.
    //  (In our case the key was previously expanded, so we just grab the pointer to it.)
    //
    K = pExpandedKey->K;

    //
    //  3. Initialize j to 63.
    //

    //
    //  4. Perform five r-mixing rounds.
    //
    RMIXROUND(15);
    RMIXROUND(14);
    RMIXROUND(13);
    RMIXROUND(12);
    RMIXROUND(11);

    //
    //  5. Perform one r-mashing round.
    //
    RMASHROUND();

    //
    //  6. Perform six r-mixing rounds.
    //
    RMIXROUND(10);
    RMIXROUND(9);
    RMIXROUND(8);
    RMIXROUND(7);
    RMIXROUND(6);
    RMIXROUND(5);

    //
    //  7. Perform one r-mashing round.
    //
    RMASHROUND();

    //
    //  8. Perform five r-mixing rounds.
    //
    RMIXROUND(4);
    RMIXROUND(3);
    RMIXROUND(2);
    RMIXROUND(1);
    RMIXROUND(0);

    SYMCRYPT_STORE_LSBFIRST16( &pbDst[0], R0 );
    SYMCRYPT_STORE_LSBFIRST16( &pbDst[2], R1 );
    SYMCRYPT_STORE_LSBFIRST16( &pbDst[4], R2 );
    SYMCRYPT_STORE_LSBFIRST16( &pbDst[6], R3 );

}

static const BYTE testPlaintext[8] = { 'P', 'l', 'a', 'i', 'n', 't', 'x', 't', };
static const BYTE testCiphertext[8] = {
    0x89, 0xe8, 0x5d, 0x1a, 0x98, 0xcd, 0xe5, 0x52,
};

VOID
SYMCRYPT_CALL
SymCryptRc2Selftest(void)
{
    SYMCRYPT_RC2_EXPANDED_KEY key;
    BYTE buf[SYMCRYPT_RC2_BLOCK_SIZE];

    if( SymCryptRc2ExpandKeyEx( &key, SymCryptTestKey32, 16, 87) != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'rc21' );
    }

    SymCryptRc2Encrypt( &key, testPlaintext, buf );

    SymCryptInjectError( buf, SYMCRYPT_RC2_BLOCK_SIZE );

    if( memcmp( buf, testCiphertext, SYMCRYPT_RC2_BLOCK_SIZE ) != 0 )
    {
        SymCryptFatal( 'rc22' );
    }

    SymCryptRc2Decrypt( &key, testCiphertext, buf );

    SymCryptInjectError( buf, SYMCRYPT_RC2_BLOCK_SIZE );

    if( memcmp( buf, testPlaintext, SYMCRYPT_RC2_BLOCK_SIZE ) != 0 )
    {
        SymCryptFatal( 'rc23' );
    }

}
