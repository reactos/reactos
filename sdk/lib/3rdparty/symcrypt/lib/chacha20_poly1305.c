//
// ChaCha20_Poly1305.c
//
// Copyright (c) Microsoft Corporation.
//

#include "precomp.h"

#define CHACHA20_POLY1305_MAX_DATA_SIZE    (((1ull << 32) - 1) * 64)

// Compile time BOOL statically determines if we need to check cbData > CHACHA20_POLY1305_MAX_DATA_SIZE
// Used to suppress MSVC C4127 and clang Wtautological-constant-out-of-range-compare on 32b platforms
const BOOL fcbDataLteMaxDataSizeStatic = SIZE_T_MAX <= CHACHA20_POLY1305_MAX_DATA_SIZE;

VOID
SYMCRYPT_CALL
SymCryptChaCha20Poly1305ComputeTag(
    _Inout_                                          PSYMCRYPT_POLY1305_STATE    pState,
    _In_reads_opt_( cbAuthData )                     PCBYTE                      pbAuthData,
                                                     SIZE_T                      cbAuthData,
    _In_reads_( cbData )                             PCBYTE                      pbData,
                                                     SIZE_T                      cbData,
    _Out_writes_( SYMCRYPT_POLY1305_RESULT_SIZE )    PBYTE                       pbTag )
{
    SYMCRYPT_ALIGN BYTE buf[SYMCRYPT_POLY1305_BLOCK_SIZE];
    BYTE partialBlockSize;

    SymCryptWipeKnownSize( buf, SYMCRYPT_POLY1305_BLOCK_SIZE );

    // Add additional authentication data if needed.
    if ( cbAuthData > 0 )
    {
        SymCryptPoly1305Append( pState, pbAuthData, cbAuthData );

        // Append zeros to make a complete Poly1305 block.
        partialBlockSize = cbAuthData % SYMCRYPT_POLY1305_BLOCK_SIZE;
        if ( partialBlockSize > 0 )
        {
            SymCryptPoly1305Append( pState, buf, SYMCRYPT_POLY1305_BLOCK_SIZE - partialBlockSize );
        }
    }

    // Add ciphertext if needed.
    if ( cbData > 0 )
    {
        SymCryptPoly1305Append( pState, pbData, cbData );

        // Append zeros to make a complete Poly1305 block.
        partialBlockSize = cbData % SYMCRYPT_POLY1305_BLOCK_SIZE;
        if ( partialBlockSize > 0 )
        {
            SymCryptPoly1305Append( pState, buf, SYMCRYPT_POLY1305_BLOCK_SIZE - partialBlockSize );
        }
    }

    // Add length of additional authentication data and ciphertext.
    SYMCRYPT_STORE_LSBFIRST64( &buf[0], cbAuthData );
    SYMCRYPT_STORE_LSBFIRST64( &buf[8], cbData );
    SymCryptPoly1305Append( pState, buf, SYMCRYPT_POLY1305_BLOCK_SIZE );
    SymCryptWipeKnownSize( buf, SYMCRYPT_POLY1305_BLOCK_SIZE );

    SymCryptPoly1305Result( pState, pbTag );
}

SYMCRYPT_NOINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptChaCha20Poly1305Encrypt(
    _In_reads_( cbKey )             PCBYTE    pbKey,
                                    SIZE_T    cbKey,
    _In_reads_( cbNonce )           PCBYTE    pbNonce,
                                    SIZE_T    cbNonce,
    _In_reads_opt_( cbAuthData )    PCBYTE    pbAuthData,
                                    SIZE_T    cbAuthData,
    _In_reads_( cbData )            PCBYTE    pbSrc,
    _Out_writes_( cbData )          PBYTE     pbDst,
                                    SIZE_T    cbData,
    _Out_writes_( cbTag )           PBYTE     pbTag,
                                    SIZE_T    cbTag )
{
    SYMCRYPT_ERROR status = SYMCRYPT_NO_ERROR;
    SYMCRYPT_CHACHA20_STATE ChaCha20State;
    SYMCRYPT_POLY1305_STATE Poly1305State;
    SYMCRYPT_ALIGN BYTE     key[SYMCRYPT_POLY1305_KEY_SIZE];

    if ( !fcbDataLteMaxDataSizeStatic && cbData > CHACHA20_POLY1305_MAX_DATA_SIZE )
    {
        status = SYMCRYPT_WRONG_DATA_SIZE;
        goto cleanup;
    }

    if ( cbTag != SYMCRYPT_POLY1305_RESULT_SIZE )
    {
        status = SYMCRYPT_WRONG_TAG_SIZE;
        goto cleanup;
    }

    status = SymCryptChaCha20Init( &ChaCha20State, pbKey, cbKey, pbNonce, cbNonce, 0 );
    if ( status != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Generate the first 32 bytes of keystream.
    SymCryptWipeKnownSize( key, sizeof( key ) );
    SymCryptChaCha20Crypt( &ChaCha20State, key, key, sizeof ( key ) );

    // Create the Poly1305 key using the first 32 bytes of the ChaCha20 keystream.
    SymCryptPoly1305Init( &Poly1305State, key );
    SymCryptWipeKnownSize( key, sizeof( key ) );

    // Encrypt data if needed.
    if ( cbData > 0 )
    {
        // Advance the keystream to counter 1 (offset 64) for data encryption.
        SymCryptChaCha20SetOffset( &ChaCha20State, 64 );
        SymCryptChaCha20Crypt( &ChaCha20State, pbSrc, pbDst, cbData );
    }

    // We read the ciphertext back, violating the general rule not to rely on I/O buffers
    // as they can reside in a different security domain. For ChaCha20Poly1305, like GCM,
    // this read-back of data is not a problem. An attacker with access to the buffer
    // will get the ChaCha20 key stream plus the Poly1305 authenticator of a single value.
    // As Poly1305 is strong even with attacker-controlled data, this is harmless.
    SymCryptChaCha20Poly1305ComputeTag( &Poly1305State, pbAuthData, cbAuthData,
                                        pbDst, cbData, pbTag );
cleanup:

    SymCryptWipeKnownSize( &ChaCha20State, sizeof( ChaCha20State ) );
    SymCryptWipeKnownSize( &Poly1305State, sizeof( Poly1305State ) );

    return status;
}

SYMCRYPT_NOINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptChaCha20Poly1305Decrypt(
    _In_reads_( cbKey )             PCBYTE    pbKey,
                                    SIZE_T    cbKey,
    _In_reads_( cbNonce )           PCBYTE    pbNonce,
                                    SIZE_T    cbNonce,
    _In_reads_opt_( cbAuthData )    PCBYTE    pbAuthData,
                                    SIZE_T    cbAuthData,
    _In_reads_( cbData )            PCBYTE    pbSrc,
    _Out_writes_( cbData )          PBYTE     pbDst,
                                    SIZE_T    cbData,
    _In_reads_( cbTag )             PCBYTE    pbTag,
                                    SIZE_T    cbTag )
{
    SYMCRYPT_ERROR status = SYMCRYPT_NO_ERROR;
    SYMCRYPT_CHACHA20_STATE ChaCha20State;
    SYMCRYPT_POLY1305_STATE Poly1305State;
    SYMCRYPT_ALIGN BYTE     buf[SYMCRYPT_POLY1305_RESULT_SIZE];
    SYMCRYPT_ALIGN BYTE     key[SYMCRYPT_POLY1305_KEY_SIZE];

    if ( !fcbDataLteMaxDataSizeStatic && cbData > CHACHA20_POLY1305_MAX_DATA_SIZE )
    {
        status = SYMCRYPT_WRONG_DATA_SIZE;
        goto cleanup;
    }

    if ( cbTag != SYMCRYPT_POLY1305_RESULT_SIZE )
    {
        status = SYMCRYPT_WRONG_TAG_SIZE;
        goto cleanup;
    }

    status = SymCryptChaCha20Init( &ChaCha20State, pbKey, cbKey, pbNonce, cbNonce, 0 );
    if ( status != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Generate the first 32 bytes of keystream.
    SymCryptWipeKnownSize( key, sizeof( key ) );
    SymCryptChaCha20Crypt( &ChaCha20State, key, key, sizeof( key ) );

    // Create the Poly1305 key using the first 32 bytes of the ChaCha20 keystream.
    SymCryptPoly1305Init( &Poly1305State, key );
    SymCryptWipeKnownSize( key, sizeof( key ) );

    // We read the ciphertext back, violating the general rule not to rely on I/O buffers
    // as they can reside in a different security domain. For ChaCha20Poly1305, like GCM,
    // this read-back of data is not a problem. An attacker with access to the buffer
    // will get the ChaCha20 key stream plus the Poly1305 authenticator of a single value.
    // As Poly1305 is strong even with attacker-controlled data, this is harmless.
    SymCryptChaCha20Poly1305ComputeTag( &Poly1305State, pbAuthData, cbAuthData,
                                        pbSrc, cbData, buf );

    // Validate tag.
    if (!SymCryptEqual(pbTag, buf, cbTag))
    {
        status = SYMCRYPT_AUTHENTICATION_FAILURE;
        goto cleanup;
    }

    // Decrypt data if needed.
    if ( cbData > 0)
    {
        // Advance the keystream to counter 1 (offset 64) for data decryption.
        SymCryptChaCha20SetOffset( &ChaCha20State, 64 );
        SymCryptChaCha20Crypt( &ChaCha20State, pbSrc, pbDst, cbData );
    }

cleanup:

    SymCryptWipeKnownSize( &ChaCha20State, sizeof( ChaCha20State ) );
    SymCryptWipeKnownSize( &Poly1305State, sizeof( Poly1305State ) );

    return status;
}


static const BYTE SymCryptChaCha20Poly1305Result[3 + SYMCRYPT_POLY1305_RESULT_SIZE] =
{
    0x5d, 0xba, 0x7b,
    0x80, 0x10, 0xd2, 0x05, 0x4a, 0xad, 0x53, 0x1f, 0xa2, 0xce, 0x83, 0xc1, 0x66, 0x12, 0x85, 0x21
};

VOID
SYMCRYPT_CALL
SymCryptChaCha20Poly1305Selftest(void)
{
    BYTE    buf[3 + SYMCRYPT_POLY1305_RESULT_SIZE];
    SYMCRYPT_ERROR err;

    if ( SymCryptChaCha20Poly1305Encrypt( SymCryptTestKey32, sizeof( SymCryptTestKey32 ),
                                          SymCryptTestMsg16, 12,
                                          SymCryptTestMsg16, sizeof( SymCryptTestMsg16 ),
                                          &SymCryptTestMsg3[0], buf, 3,
                                          &buf[3], SYMCRYPT_POLY1305_RESULT_SIZE ) != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'ccp0' );
    }

    SymCryptInjectError( buf, sizeof( buf ) );
    if ( memcmp( buf, SymCryptChaCha20Poly1305Result, sizeof( buf ) ) != 0 )
    {
        SymCryptFatal( 'ccp1' );
    }

    // Inject error into the ciphertext or tag.
    SymCryptInjectError( buf, sizeof( buf ) );

    err = SymCryptChaCha20Poly1305Decrypt( SymCryptTestKey32, sizeof( SymCryptTestKey32 ),
                                           SymCryptTestMsg16, 12,
                                           SymCryptTestMsg16, sizeof( SymCryptTestMsg16 ),
                                           buf, buf, 3,
                                           &buf[3], SYMCRYPT_POLY1305_RESULT_SIZE );
    SymCryptInjectError( buf, 3 );

    if ( err != SYMCRYPT_NO_ERROR || memcmp( buf, SymCryptTestMsg3, 3 ) != 0 )
    {
        SymCryptFatal( 'ccp2' );
    }
}
