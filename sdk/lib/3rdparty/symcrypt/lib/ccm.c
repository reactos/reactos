//
// CCM.c    implementation of the CCM block cipher mode
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define CCM_MIN_NONCE_SIZE  (7)
#define CCM_MAX_NONCE_SIZE  (13)
#define CCM_MIN_TAG_SIZE    (4)
#define CCM_MAX_TAG_SIZE    (16)

#define CCM_MAX_COUNTER_SIZE    (SYMCRYPT_CCM_BLOCK_SIZE - 1 - CCM_MIN_NONCE_SIZE)

#define AUTHDATA_16BIT_LIMIT    ((1<<16) - (1<<8))
#define AUTHDATA_32BIT_LIMIT    (1ull << 32)

// Compile time BOOL statically determines if we need to check cbAuthData < AUTHDATA_32BIT_LIMIT
// Used to suppress MSVC C4127 and clang Wtautological-constant-out-of-range-compare on 32b platforms
const BOOL fcbAuthDataLt32bitLimitStatic = SIZE_T_MAX < AUTHDATA_32BIT_LIMIT;

#define CCM_BLOCK_MOD_MASK      (SYMCRYPT_CCM_BLOCK_SIZE - 1)
#define CCM_BLOCK_ROUND_MASK    (~CCM_BLOCK_MOD_MASK)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCcmValidateParameters(
    _In_    PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_    SIZE_T                  cbNonce,
    _In_    SIZE_T                  cbAssociatedData,
    _In_    UINT64                  cbData,
    _In_    SIZE_T                  cbTag
   )
{
    SIZE_T cbCounter;

    UNREFERENCED_PARAMETER( cbAssociatedData );

    if( pBlockCipher->blockSize != SYMCRYPT_CCM_BLOCK_SIZE )
    {
        return SYMCRYPT_WRONG_BLOCK_SIZE;
    }

    //
    // Test against limits in SP800-38C appendix A
    //
    if( cbNonce < CCM_MIN_NONCE_SIZE || cbNonce > CCM_MAX_NONCE_SIZE )
    {
        return SYMCRYPT_WRONG_NONCE_SIZE;
    }

    //
    // cbAssociatedData is limited to <2^64
    // We don't test for this. None of our platforms has a SIZE_T that is
    // large enough to violate this condition. And the test
    // is of a form that the compiler cannot optimize away.
    //

    //
    // The counter block consists of a single flag byte, the nonce, and the counter field.
    //
    cbCounter = SYMCRYPT_CCM_BLOCK_SIZE - cbNonce - 1;

    //
    // per SP800-38C cbData is limited to 2^{8*cbCounter}
    // There is no way to do this test in a single comparison.
    // We don't have to worry about side-channels in the && because
    // cbCounter depends only on the length of the nonce, and we do not
    // try to hide any lengths.
    //
    if( cbCounter < sizeof( UINT64 ) &&
        cbData >= ((UINT64)1 << (8*cbCounter)) )
    {
        return SYMCRYPT_WRONG_DATA_SIZE;
    }

    if( cbTag < CCM_MIN_TAG_SIZE ||
        cbTag > CCM_MAX_TAG_SIZE ||
        (cbTag & 1) == 1        // valid tag lengths are [4, 6, 8, ..., 16]
        )
    {
        return SYMCRYPT_WRONG_TAG_SIZE;
    }

    return SYMCRYPT_NO_ERROR;
}



VOID
SYMCRYPT_CALL
SymCryptCcmEncryptDecryptPart(
    _Inout_                     PSYMCRYPT_CCM_STATE pState,
    _In_reads_( cbData )        PCBYTE              pbSrc,
    _Out_writes_( cbData )      PBYTE               pbDst,
                                SIZE_T              cbData )

{
    SIZE_T  cbToDo = cbData;
    SIZE_T  bytesToProcess;

    //
    // Use any left-over key stream
    //
    while( (pState->bytesProcessed & CCM_BLOCK_MOD_MASK) != 0 && cbToDo > 0 )
    {
        *pbDst = *pbSrc ^ pState->keystreamBlock[ pState->bytesProcessed & CCM_BLOCK_MOD_MASK ];
        pbDst++;
        pbSrc++;
        cbToDo--;
        pState->bytesProcessed++;
    }

    //
    // Bulk process the main part of the input and output
    //
    if( cbToDo >= SYMCRYPT_CCM_BLOCK_SIZE )
    {
        bytesToProcess = cbToDo & CCM_BLOCK_ROUND_MASK;
        SYMCRYPT_ASSERT( bytesToProcess <= cbToDo );

        SYMCRYPT_ASSERT( pState->pBlockCipher->blockSize == SYMCRYPT_CCM_BLOCK_SIZE );
        SymCryptCtrMsb64(   pState->pBlockCipher,
                            pState->pExpandedKey,
                            &pState->counterBlock[0],
                            pbSrc,
                            pbDst,
                            bytesToProcess );
        pbSrc += bytesToProcess;
        pbDst += bytesToProcess;
        pState->bytesProcessed += bytesToProcess;
        cbToDo -= bytesToProcess;
    }

    if( cbToDo > 0 )
    {
        //
        // Encrypt an all-zero key stream block to get the key stream.
        //
        SymCryptWipeKnownSize( &pState->keystreamBlock[0], SYMCRYPT_CCM_BLOCK_SIZE );

        SYMCRYPT_ASSERT( pState->pBlockCipher->blockSize == SYMCRYPT_CCM_BLOCK_SIZE );
        SymCryptCtrMsb64(   pState->pBlockCipher,
                            pState->pExpandedKey,
                            &pState->counterBlock[0],
                            &pState->keystreamBlock[0],
                            &pState->keystreamBlock[0],
                            SYMCRYPT_CCM_BLOCK_SIZE );
        while( cbToDo > 0 )
        {
            *pbDst = *pbSrc ^ pState->keystreamBlock[ pState->bytesProcessed & CCM_BLOCK_MOD_MASK ];
            pbDst++;
            pbSrc++;
            cbToDo--;
            pState->bytesProcessed++;
        }
    }
}


VOID
SYMCRYPT_CALL
SymCryptCcmAddMacData(
    _Inout_                 PSYMCRYPT_CCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbData,
                            SIZE_T              cbData )
{
    SIZE_T bytesToProcess;
    if( pState->bytesInMacBlock > 0 )
    {
        bytesToProcess = SYMCRYPT_MIN( cbData, SYMCRYPT_CCM_BLOCK_SIZE - pState->bytesInMacBlock );
        SymCryptXorBytes( &pState->macBlock[pState->bytesInMacBlock], pbData, &pState->macBlock[pState->bytesInMacBlock], bytesToProcess );
        pbData += bytesToProcess;
        cbData -= bytesToProcess;
        pState->bytesInMacBlock += bytesToProcess;

        if( pState->bytesInMacBlock == SYMCRYPT_CCM_BLOCK_SIZE )
        {
            pState->pBlockCipher->encryptFunc( pState->pExpandedKey, &pState->macBlock[0], &pState->macBlock[0] );
            pState->bytesInMacBlock = 0;
        }
    }

    if( cbData >= SYMCRYPT_CCM_BLOCK_SIZE )
    {
        bytesToProcess = cbData & CCM_BLOCK_ROUND_MASK;
        SYMCRYPT_ASSERT( pState->pBlockCipher->blockSize == SYMCRYPT_CCM_BLOCK_SIZE );

        SymCryptCbcMac( pState->pBlockCipher,
                        pState->pExpandedKey,
                        &pState->macBlock[0],
                        pbData,
                        bytesToProcess );

        pbData += bytesToProcess;
        cbData -= bytesToProcess;
    }

    if( cbData > 0 )
    {
        SymCryptXorBytes( &pState->macBlock[0], pbData, &pState->macBlock[0], cbData );
        pState->bytesInMacBlock = cbData;
    }
}

VOID
SYMCRYPT_CALL
SymCryptCcmPadMacData( _Inout_ PSYMCRYPT_CCM_STATE  pState )
{
    //
    // Pad the MAC data with zeroes until we hit the block size.
    // The data is xorred into macBlock, so we don't have to update that.
    // All we do is apply the block cipher if there was any data remaining in the macBlock.
    //
    if( pState->bytesInMacBlock > 0 )
    {
        pState->pBlockCipher->encryptFunc( pState->pExpandedKey, &pState->macBlock[0], &pState->macBlock[0] );
        pState->bytesInMacBlock = 0;
    }
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptCcmEncrypt(
    _In_                            PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                            PCVOID                  pExpandedKey,
    _In_reads_( cbNonce )           PCBYTE                  pbNonce,
                                    SIZE_T                  cbNonce,
    _In_reads_opt_( cbAuthData )    PCBYTE                  pbAuthData,
                                    SIZE_T                  cbAuthData,
    _In_reads_( cbData )            PCBYTE                  pbSrc,
    _Out_writes_( cbData )          PBYTE                   pbDst,
                                    SIZE_T                  cbData,
    _Out_writes_( cbTag )           PBYTE                   pbTag,
                                    SIZE_T                  cbTag )
{
    SYMCRYPT_CCM_STATE  state;

    SymCryptCcmInit(    &state,
                        pBlockCipher,
                        pExpandedKey,
                        pbNonce, cbNonce,
                        pbAuthData, cbAuthData,
                        cbData, cbTag );

    SymCryptCcmEncryptPart( &state, pbSrc, pbDst, cbData );

    SymCryptCcmEncryptFinal( &state, pbTag, cbTag );
}

SYMCRYPT_NOINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCcmDecrypt(
    _In_                            PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                            PCVOID                  pExpandedKey,
    _In_reads_( cbNonce )           PCBYTE                  pbNonce,
                                    SIZE_T                  cbNonce,
    _In_reads_opt_( cbAuthData )    PCBYTE                  pbAuthData,
                                    SIZE_T                  cbAuthData,
    _In_reads_( cbData )            PCBYTE                  pbSrc,
    _Out_writes_( cbData )          PBYTE                   pbDst,
                                    SIZE_T                  cbData,
    _In_reads_( cbTag )             PCBYTE                  pbTag,
                                    SIZE_T                  cbTag )
{
    SYMCRYPT_CCM_STATE  state;
    SYMCRYPT_ERROR      status;

    SymCryptCcmInit(    &state,
                        pBlockCipher,
                        pExpandedKey,
                        pbNonce, cbNonce,
                        pbAuthData, cbAuthData,
                        cbData, cbTag );


    SymCryptCcmDecryptPart( &state, pbSrc, pbDst, cbData );

    status = SymCryptCcmDecryptFinal( &state, pbTag, cbTag );

    //
    // If we failed for any reason we wipe our output buffer to avoid returning
    // decrypted but unauthenticated data.
    //
    if( status != SYMCRYPT_NO_ERROR )
    {
        SymCryptWipe( pbDst, cbData );
    }

    return status;
}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptCcmInit(
    _Out_                           PSYMCRYPT_CCM_STATE     pState,
    _In_                            PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                            PCVOID                  pExpandedKey,
    _In_reads_( cbNonce )           PCBYTE                  pbNonce,
                                    SIZE_T                  cbNonce,
    _In_reads_opt_( cbAuthData )    PCBYTE                  pbAuthData,
                                    SIZE_T                  cbAuthData,
                                    UINT64                  cbData,
                                    SIZE_T                  cbTag )
{
    BYTE            flags;
    BYTE            tmpBuf[ SYMCRYPT_CCM_BLOCK_SIZE ];
    SIZE_T          cbCounter;

    SYMCRYPT_SET_MAGIC( pState );

    //
    // Validate parameters in checked builds
    //
    SYMCRYPT_ASSERT( SymCryptCcmValidateParameters( pBlockCipher, cbNonce, cbAuthData, cbData, cbTag ) == SYMCRYPT_NO_ERROR );


    //
    // compute # bytes in the counter field
    // We limit cbNonce to 15 so that cbCounter + cbNonce = 15 will always hold
    // This is much cheaper than full parameter validation, and it is enough to
    // avoid any buffer overflows.
    //
    cbNonce &= SYMCRYPT_CCM_BLOCK_SIZE - 1;
    cbCounter = SYMCRYPT_CCM_BLOCK_SIZE - 1 - cbNonce;

    pState->pBlockCipher = pBlockCipher;
    pState->pExpandedKey = pExpandedKey;
    pState->cbNonce = cbNonce;
    pState->cbData = cbData;
    pState->cbTag = cbTag;
    pState->cbCounter = cbCounter;
    pState->bytesProcessed = 0;
    pState->bytesInMacBlock = 0;

    //
    // Build the initial blocks for authentication and en/decryption
    //
    // Per Sp800-38c the flag byte is made up of four fields:
    // Bits 0-2 are cbCounter - 1
    // Bits 3-5 are (cbTag-2)/2
    // Bit 6 is 1 if cbAuthData > 0
    // Bit 7 is reserved and set to 0.
    flags = (BYTE) (pState->cbCounter - 1);
    flags |= ((cbTag-2)/2) << 3;
    if( cbAuthData > 0 )
    {
        //
        // No side-channel concerns with this if statements as we don't try to hide the
        // data length or presence of authdata.
        //
        flags |= (1 << 6);
    }


    //
    // The MAC starting block consists of three fields:
    // the flag byte, the nonce, and cbData encoded into cbCounter bytes.
    //
    pState->macBlock[0] = flags;
    memcpy( &pState->macBlock[1], pbNonce, cbNonce );
    SYMCRYPT_STORE_MSBFIRST64( &tmpBuf[0], cbData );
    memcpy( &pState->macBlock[1+cbNonce], &tmpBuf[ 8 - cbCounter ], cbCounter );

    //
    // The counter block is similar in layout, but with two changes:
    // Bits 3-7 of the flag bytes are set to 0.
    // The counter field is set to one (first counter value used for data encryption).
    // Wiping the whole block first is probably faster, as the size is known and the
    // block is aligned.
    // We also copy the nonce from the mac block to follow the read-once rule.
    //
    SymCryptWipeKnownSize( &pState->counterBlock[0], SYMCRYPT_CCM_BLOCK_SIZE );
    pState->counterBlock[0] = (BYTE)(flags & 0x7);
    memcpy( &pState->counterBlock[1], &pState->macBlock[1], cbNonce );
    pState->counterBlock[ SYMCRYPT_CCM_BLOCK_SIZE - 1] = 1;

    //
    // Encrypt the current MAC block; our CBC convention is to do the encryption
    // as soon as we have enough data.
    //
    pBlockCipher->encryptFunc( pExpandedKey, &pState->macBlock[0], &pState->macBlock[0] );

    //
    // Next we process the associated data
    // See the CCM specs for the details
    //
    if( cbAuthData <= 0 )
    {
        //
        // cbAuthData == 0, nothing needs to be done.
        //
    } else if( cbAuthData < AUTHDATA_16BIT_LIMIT )
    {
        //
        // 16-bit length encoding.
        //
        SYMCRYPT_STORE_MSBFIRST16( &tmpBuf[0], (UINT16) cbAuthData );
        SymCryptCcmAddMacData( pState, &tmpBuf[0], 2 );
    } else if( fcbAuthDataLt32bitLimitStatic || cbAuthData < AUTHDATA_32BIT_LIMIT )
    {
        //
        // 32-bit length
        //
        tmpBuf[0] = 0xff;
        tmpBuf[1] = 0xfe;       // Magic prefix as per SP 800-38c
        SYMCRYPT_STORE_MSBFIRST32( &tmpBuf[2], (UINT32) cbAuthData );
        SymCryptCcmAddMacData( pState, &tmpBuf[0], 2 + sizeof( UINT32 ) );
    } else
    {
        //
        // 64-bit length
        //
        tmpBuf[0] = 0xff;
        tmpBuf[1] = 0xff;       // Magic prefix as per SP 800-38c
        SYMCRYPT_STORE_MSBFIRST64( &tmpBuf[2], cbAuthData );
        SymCryptCcmAddMacData( pState, &tmpBuf[0], 2 + sizeof( UINT64 ) );
    }

    SymCryptCcmAddMacData( pState, pbAuthData, cbAuthData );
    SymCryptCcmPadMacData( pState );        // Pad MAC data with zeroes until the next block size boundary

}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptCcmEncryptPart(
    _Inout_                   PSYMCRYPT_CCM_STATE pState,
    _In_reads_( cbData )      PCBYTE              pbSrc,
    _Out_writes_( cbData )    PBYTE               pbDst,
                              SIZE_T              cbData )
{
    UINT64 bytesProcessedAfterThisCall;

    SYMCRYPT_CHECK_MAGIC( pState );

    bytesProcessedAfterThisCall = cbData + pState->bytesProcessed;

    SYMCRYPT_ASSERT( bytesProcessedAfterThisCall >= cbData &&
                     bytesProcessedAfterThisCall <= pState->cbData );

    //
    // We are violating the read-once implementation rule here. We read the data twice:
    // once for MACing and once for encryption.
    // In this particular situation this is safe to do.
    // We consider the read for the MAC operation as reading the 'real' value.
    // The encryption code reads the data, but all it does is XOR the key stream into
    // it. (CCM encryption uses CTR mode for the encryption part.)
    // We don't care if the attacker modifies the data before the encryption.
    // We are revealing the key stream anyway (from the plaintext and ciphertext) and
    // the exact byte value that we xor the key stream into is irrelevant.
    //
    SymCryptCcmAddMacData( pState, pbSrc, cbData );

    SymCryptCcmEncryptDecryptPart( pState, pbSrc, pbDst, cbData );

}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptCcmEncryptFinal(
    _Inout_                 PSYMCRYPT_CCM_STATE pState,
    _Out_writes_( cbTag )   PBYTE               pbTag,
                            SIZE_T              cbTag )
{
    //
    // Check invariants in checked builds
    //
    SYMCRYPT_CHECK_MAGIC( pState );

    SYMCRYPT_ASSERT( cbTag  == pState->cbTag && pState->bytesProcessed == pState->cbData );


    SymCryptCcmPadMacData( pState );

    //
    // Set the counter value to zero to get the counter value that encrypts the tag,
    // and then encrypt the tag.
    // We reset bytesProcessed so that the partial encrypt/decrypt function will do the right thing
    //
    SymCryptWipe( &pState->counterBlock[1 + pState->cbNonce], pState->cbCounter );

    pState->bytesProcessed = 0;

    SymCryptCcmEncryptDecryptPart( pState, &pState->macBlock[0], &pState->macBlock[0], SYMCRYPT_CCM_BLOCK_SIZE );

    memcpy( pbTag, &pState->macBlock[0], cbTag );

    SymCryptWipeKnownSize( pState, sizeof( *pState ) );
    SYMCRYPT_ASSERT( pState->bytesInMacBlock == 0 );
}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptCcmDecryptPart(
    _Inout_                 PSYMCRYPT_CCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData )
{
    UINT64 bytesProcessedAfterThisCall;

    SYMCRYPT_CHECK_MAGIC( pState );

    bytesProcessedAfterThisCall = cbData + pState->bytesProcessed;

    SYMCRYPT_ASSERT( bytesProcessedAfterThisCall >= cbData &&
                     bytesProcessedAfterThisCall <= pState->cbData );


    //
    // We are violating the read-once/write-once implementation rule here.
    // We write the decrypted data and then read it back for the authentication function.
    // In this particular situation this is safe to do.
    //
    // Anyone who can access the memory space that contains the source and destination of this
    // function can recover the key stream used for this (key,nonce) combination.
    // We can think of the decryption function as merely exposing the key stream, and then the
    // caller picking the ciphertext (and by implication the plaintext) to be authenticated.
    // Thus the data we read during authentication is the 'real' plaintext, and the
    // decryption function merely made the key stream available.
    //
    // Note that this would not safe in general, it is only safe because CTR mode decryption already
    // reveals the key stream.
    //
    SymCryptCcmEncryptDecryptPart( pState, pbSrc, pbDst, cbData );
    SymCryptCcmAddMacData( pState, pbDst, cbData );

}

SYMCRYPT_NOINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCcmDecryptFinal(
    _Inout_             PSYMCRYPT_CCM_STATE pState,
    _In_reads_( cbTag ) PCBYTE              pbTag,
                        SIZE_T              cbTag )
{
    SYMCRYPT_ERROR status;

    //
    // Check invariants in checked builds
    //
    SYMCRYPT_CHECK_MAGIC( pState );

    SYMCRYPT_ASSERT( cbTag  == pState->cbTag && pState->bytesProcessed == pState->cbData );

    SymCryptCcmPadMacData( pState );

    //
    // Set the counter value to zero to get the counter value that encrypts the tag,
    // and then encrypt the tag
    // We reset bytesProcessed so that the partial encrypt/decrypt function will do the right thing
    //
    SymCryptWipe( &pState->counterBlock[1 + pState->cbNonce], pState->cbCounter );

    pState->bytesProcessed = 0;

    SymCryptCcmEncryptDecryptPart( pState, &pState->macBlock[0], &pState->macBlock[0], SYMCRYPT_CCM_BLOCK_SIZE );

    if( !SymCryptEqual( pbTag, &pState->macBlock[0], cbTag ) )
    {
        status = SYMCRYPT_AUTHENTICATION_FAILURE;
    }
    else
    {
        status = SYMCRYPT_NO_ERROR;
    }

    SymCryptWipeKnownSize( pState, sizeof( *pState ) );
    SYMCRYPT_ASSERT( pState->bytesInMacBlock == 0 );

    return status;
}


static const BYTE SymCryptCcmSelftestResult[3 + SYMCRYPT_AES_BLOCK_SIZE ] =
{
    0x42, 0xd7, 0xda,
    0x3d, 0x9e, 0x95, 0x82, 0x29, 0x3c, 0x10, 0x9c, 0xa3, 0x39, 0x31, 0x3f, 0x18, 0xf3, 0x10, 0xf6
};

VOID
SYMCRYPT_CALL
SymCryptCcmSelftest(void)
{
    BYTE    buf[ 3 + SYMCRYPT_AES_BLOCK_SIZE ];
    SYMCRYPT_AES_EXPANDED_KEY key;
    SYMCRYPT_ERROR err;

    if( SymCryptAesExpandKey( &key, SymCryptTestKey32, 16 ) != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'ccm0' );
    }

    SymCryptCcmEncrypt( SymCryptAesBlockCipher,
                        &key,
                        &SymCryptTestKey32[16], 12,
                        NULL, 0,
                        &SymCryptTestMsg3[0], buf, 3,
                        &buf[3], SYMCRYPT_AES_BLOCK_SIZE );

    SymCryptInjectError( buf, sizeof( buf ) );
    if( memcmp( buf, SymCryptCcmSelftestResult, sizeof( buf ) ) != 0 )
    {
        SymCryptFatal( 'ccm1' );
    }

    // inject error into the ciphertext or tag
    SymCryptInjectError( buf, sizeof( buf ) );

    err = SymCryptCcmDecrypt(   SymCryptAesBlockCipher,
                                &key,
                                &SymCryptTestKey32[16], 12,
                                NULL, 0,
                                buf, buf, 3,
                                &buf[3], SYMCRYPT_AES_BLOCK_SIZE );

    SymCryptInjectError( buf, 3 );

    if( err != SYMCRYPT_NO_ERROR || memcmp( buf, SymCryptTestMsg3, 3 ) != 0 )
    {
        SymCryptFatal( 'ccm2' );
    }

}
