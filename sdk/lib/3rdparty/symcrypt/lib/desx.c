//
// DesX.c   DESX implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//


#include "precomp.h"


const SYMCRYPT_BLOCKCIPHER SymCryptDesxBlockCipher_default = {
    SymCryptDesxExpandKey,  // PSYMCRYPT_BLOCKCIPHER_EXPAND_KEY    expandKeyFunc;
    SymCryptDesxEncrypt,    // PSYMCRYPT_BLOCKCIPHER_CRYPT         encryptFunc;
    SymCryptDesxDecrypt,    // PSYMCRYPT_BLOCKCIPHER_CRYPT         decryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_ECB     ecbEncryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_ECB     ecbDecryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    cbcEncryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    cbcDecryptFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_MAC_MODE      cbcMacFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    ctrMsbFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_AEADPART_MODE gcmEncryptPartFunc;
    NULL,                   // PSYMCRYPT_BLOCKCIPHER_AEADPART_MODE gcmDecryptPartFunc;
    8,                      // SIZE_T                              blockSize;
    sizeof( SYMCRYPT_DESX_EXPANDED_KEY ), // SIZE_T  expandedKeySize;    // = sizeof( SYMCRYPT_XXX_EXPANDED_KEY )
};

const PCSYMCRYPT_BLOCKCIPHER SymCryptDesxBlockCipher = &SymCryptDesxBlockCipher_default;

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDesxExpandKey(   _Out_              PSYMCRYPT_DESX_EXPANDED_KEY  pExpandedKey,
                        _In_reads_(cbKey)  PCBYTE                      pbKey,
                                            SIZE_T                      cbKey )
{
    if( cbKey != 24 )
    {
        return SYMCRYPT_WRONG_KEY_SIZE;
    }

    SymCryptDesExpandKey( &pExpandedKey->desKey, pbKey, 8 );
    memcpy( pExpandedKey->inputWhitening, pbKey+8, 8 );
    memcpy( pExpandedKey->outputWhitening, pbKey+16, 8 );

    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptDesxEncrypt(
    _In_                                        PCSYMCRYPT_DESX_EXPANDED_KEY    pExpandedKey,
    _In_reads_( SYMCRYPT_DESX_BLOCK_SIZE )     PCBYTE                          pbSrc,
    _Out_writes_( SYMCRYPT_DESX_BLOCK_SIZE )    PBYTE                           pbDst )
{
    SYMCRYPT_ALIGN BYTE buf[8];

    //
    // We buffer the result locally to obey the read once/write once rule.
    //
    SymCryptXorBytes( pbSrc, pExpandedKey->inputWhitening, buf, 8 );
    SymCryptDesEncrypt( &pExpandedKey->desKey, buf, buf );
    SymCryptXorBytes( buf, pExpandedKey->outputWhitening, pbDst, 8 );

    SymCryptWipeKnownSize( buf, sizeof( buf ) );
}

VOID
SYMCRYPT_CALL
SymCryptDesxDecrypt(
    _In_                                        PCSYMCRYPT_DESX_EXPANDED_KEY    pExpandedKey,
    _In_reads_( SYMCRYPT_DESX_BLOCK_SIZE )     PCBYTE                          pbSrc,
    _Out_writes_( SYMCRYPT_DESX_BLOCK_SIZE )    PBYTE                           pbDst )
{
    SYMCRYPT_ALIGN BYTE buf[8];

    //
    // We buffer the result locally to obey the read once/write once rule.
    //
    SymCryptXorBytes( pbSrc, pExpandedKey->outputWhitening, buf, 8 );
    SymCryptDesDecrypt( &pExpandedKey->desKey, buf, buf );
    SymCryptXorBytes( buf, pExpandedKey->inputWhitening, pbDst, 8 );

    SymCryptWipeKnownSize( buf, sizeof( buf ) );
}


static const BYTE desxKnownKey[24] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    0x01, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18
};

static const BYTE desxKnownPlaintext[] = {
    0xd9, 0xb6, 0xa1, 0x4e, 0xe6, 0x71, 0x4e, 0x17
};

static const BYTE desxKnownCiphertext[] = {
    0x66, 0x77, 0x1f, 0x2a, 0x0c, 0x05, 0x01, 0xca
};


VOID
SYMCRYPT_CALL
SymCryptDesxSelftest(void)
{
    SYMCRYPT_DESX_EXPANDED_KEY key;
    BYTE buf[SYMCRYPT_DESX_BLOCK_SIZE];

    if( SymCryptDesxExpandKey( &key, desxKnownKey, sizeof( desxKnownKey )) != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'desx' );
    }

    SymCryptDesxEncrypt( &key, desxKnownPlaintext, buf );

    SymCryptInjectError( buf, SYMCRYPT_DESX_BLOCK_SIZE );

    if( memcmp( buf, desxKnownCiphertext, SYMCRYPT_DESX_BLOCK_SIZE ) != 0 )
    {
        SymCryptFatal( 'desy' );
    }

    SymCryptDesxDecrypt( &key, desxKnownCiphertext, buf );

    SymCryptInjectError( buf, SYMCRYPT_DESX_BLOCK_SIZE );

    if( memcmp( buf, desxKnownPlaintext, SYMCRYPT_DESX_BLOCK_SIZE ) != 0 )
    {
        SymCryptFatal( 'desz' );
    }

}
