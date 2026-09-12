//
// BlockCipherModes.c   generic implementation of all block cipher modes
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

VOID
SYMCRYPT_CALL
SymCryptEcbEncrypt(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData )
{
    SIZE_T i;
    SIZE_T cbToDo = cbData & ~(pBlockCipher->blockSize - 1);

    if( pBlockCipher->ecbEncryptFunc != NULL )
    {
        //
        // Use optimized implementation if available
        //
        (*pBlockCipher->ecbEncryptFunc)( pExpandedKey, pbSrc, pbDst, cbData );
        return;
    }

    //
    // To avoid buffer overruns we truncate the work to an integral number of blocks.
    //

    for( i=0; i<cbToDo; i+= pBlockCipher->blockSize )
    {
        (*pBlockCipher->encryptFunc)( pExpandedKey, pbSrc + i, pbDst + i );
    }
}

VOID
SYMCRYPT_CALL
SymCryptEcbDecrypt(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _In_reads_( cbData )       PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData )
{
    SIZE_T i;
    SIZE_T cbToDo = cbData & ~(pBlockCipher->blockSize - 1);

    if( pBlockCipher->ecbDecryptFunc != NULL )
    {
        //
        // Use optimized implementation if available
        //
        (*pBlockCipher->ecbDecryptFunc)( pExpandedKey, pbSrc, pbDst, cbData );
        return;
    }

    for( i=0; i<cbToDo; i+= pBlockCipher->blockSize )
    {
        (*pBlockCipher->decryptFunc)( pExpandedKey, pbSrc + i, pbDst + i );
    }
}


//
// SymCryptCbcEncrypt
//
// Generic CBC encryption routine for block ciphers.
// The following restrictions must be obeyed:
//  - blockSize <= 32 and must be a power of 2
//  - cbData must be a multiple of the block size
//
VOID
SYMCRYPT_CALL
SymCryptCbcEncrypt(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData )
{
    SYMCRYPT_ALIGN BYTE    buf[SYMCRYPT_MAX_BLOCK_SIZE];
    SIZE_T blockSize;
    PCBYTE pbSrcEnd;
    PCBYTE pSrc = pbSrc;
    PBYTE  pDst = pbDst;

    if( pBlockCipher->cbcEncryptFunc != NULL )
    {
        //
        // Use optimized implementation if available
        //
        (*pBlockCipher->cbcEncryptFunc)( pExpandedKey, pbChainingValue, pSrc, pDst, cbData );
        return;
    }

    blockSize = pBlockCipher->blockSize;

    SYMCRYPT_ASSERT( blockSize <= SYMCRYPT_MAX_BLOCK_SIZE );


    //
    // Compute the end of the data, rounding the size down to a multiple of the block size.
    //
    pbSrcEnd = &pbSrc[ cbData & ~(blockSize - 1) ];

    //
    // We keep the chaining state in a local buffer to enforce the read-once write-once rule.
    //
    memcpy( buf, pbChainingValue, blockSize );
    while( pSrc < pbSrcEnd )
    {
        SYMCRYPT_ASSERT( pSrc <= pbSrc + cbData - blockSize );  // help PreFast
        SYMCRYPT_ASSERT( pDst <= pbDst + cbData - blockSize );  // help PreFast
        SYMCRYPT_ASSERT( blockSize <= cbData );                 // help PreFast
        SymCryptXorBytes( pSrc, buf, buf, blockSize );
        (*pBlockCipher->encryptFunc)( pExpandedKey, buf, buf );
        memcpy( pDst, buf, blockSize );
        pSrc += blockSize;
        pDst += blockSize;
    }

    memcpy( pbChainingValue, buf, blockSize );

    SymCryptWipeKnownSize( buf, sizeof( buf ));
}

//
// SymCryptCbcDecrypt
//
// Generic CBC decryption routine for block ciphers.
// The following restrictions must be obeyed:
//  - blockSize <= 32 and must be a power of 2
//  - cbData must be a multiple of the block size
//
VOID
SYMCRYPT_CALL
SymCryptCbcDecrypt(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData )
{
    SYMCRYPT_ALIGN BYTE buf[3 * SYMCRYPT_MAX_BLOCK_SIZE];
    PBYTE chain = &buf[0];
    PBYTE ciphertext = &buf[SYMCRYPT_MAX_BLOCK_SIZE];
    PBYTE tmp = &buf[2*SYMCRYPT_MAX_BLOCK_SIZE];

    SIZE_T blockSize;
    PCBYTE pbSrcEnd;

    if( pBlockCipher->cbcDecryptFunc != NULL )
    {
        (*pBlockCipher->cbcDecryptFunc)( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
        return;
    }

    blockSize = pBlockCipher->blockSize;
    SYMCRYPT_ASSERT( blockSize <= SYMCRYPT_MAX_BLOCK_SIZE );

    //
    // Compute the end of the data, rounding the size down to a multiple of the block size.
    //
    pbSrcEnd = &pbSrc[ cbData & ~(blockSize-1) ];

#pragma warning(suppress: 22105)
    memcpy( chain, pbChainingValue, blockSize );

    //
    // Loop structured to obey the read-once/write-once rule
    //
    while( pbSrc < pbSrcEnd )
    {
        SYMCRYPT_ASSERT( pbSrc <= pbSrcEnd - blockSize );   // help PreFast
        memcpy( ciphertext, pbSrc, blockSize );
        (*pBlockCipher->decryptFunc)( pExpandedKey, ciphertext, tmp );
        SymCryptXorBytes( tmp, chain, pbDst, blockSize );
        memcpy( chain, ciphertext, blockSize );
        pbDst += blockSize;
        pbSrc += blockSize;
    }

    memcpy( pbChainingValue, chain, blockSize );

    SymCryptWipeKnownSize( buf, sizeof( buf ));
}

VOID
SYMCRYPT_CALL
SymCryptCbcMac(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
                                SIZE_T                  cbData )
{
    SYMCRYPT_ALIGN BYTE    buf[32];
    SIZE_T blockSize;
    PCBYTE pbSrcEnd;
    PCBYTE p;

    if( pBlockCipher->cbcMacFunc != NULL )
    {
        //
        // Use optimized implementation if available
        //
        (*pBlockCipher->cbcMacFunc)( pExpandedKey, pbChainingValue, pbSrc, cbData );
        return;
    }

    blockSize = pBlockCipher->blockSize;
    SYMCRYPT_ASSERT( blockSize <= SYMCRYPT_MAX_BLOCK_SIZE );

    //
    // Compute the end of the data, rounding the size down to a multiple of the block size.
    //
    pbSrcEnd = &pbSrc[ cbData & ~(blockSize - 1) ];

    //
    // We keep the chaining state in a local buffer to enforce the read-once write-once rule.
    // It also improves memory locality.
    //
    memcpy( buf, pbChainingValue, blockSize );
    p = pbSrc;
    while( p < pbSrcEnd )
    {
        SYMCRYPT_ASSERT( p <= pbSrc + cbData - blockSize );
        SymCryptXorBytes( p, buf, buf, blockSize );
        (*pBlockCipher->encryptFunc)( pExpandedKey, buf, buf );
        p += blockSize;
    }

    memcpy( pbChainingValue, buf, blockSize );

    SymCryptWipeKnownSize( buf, sizeof( buf ));
}

VOID
SYMCRYPT_CALL
SymCryptCtrMsb32(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData )
{
    SYMCRYPT_ALIGN BYTE buf[2 * SYMCRYPT_MAX_BLOCK_SIZE];
    PBYTE count = &buf[0];
    PBYTE keystream= &buf[SYMCRYPT_MAX_BLOCK_SIZE];
    SIZE_T blockSize;
    PCBYTE pbSrcEnd;

    blockSize = pBlockCipher->blockSize;
    SYMCRYPT_ASSERT( blockSize <= SYMCRYPT_MAX_BLOCK_SIZE );

    //
    // Compute the end of the data, rounding the size down to a multiple of the block size.
    //
    pbSrcEnd = &pbSrc[ cbData & ~(blockSize - 1) ];

    //
    // We keep the chaining state in a local buffer to enforce the read-once write-once rule.
    // It also improves memory locality.
    //
    #pragma warning(suppress: 22105)
    memcpy( count, pbChainingValue, blockSize );
    while( pbSrc < pbSrcEnd )
    {
        SYMCRYPT_ASSERT( pbSrc <= pbSrcEnd - blockSize );   // help PreFast
        (*pBlockCipher->encryptFunc)( pExpandedKey, count, keystream );
        SymCryptXorBytes( keystream, pbSrc, pbDst, blockSize );

        //
        // We only need to increment the last 32 bits of the counter value.
        //
        SYMCRYPT_STORE_MSBFIRST32( &count[ blockSize-4 ], 1 + SYMCRYPT_LOAD_MSBFIRST32( &count[ blockSize-4 ] ) );

        pbSrc += blockSize;
        pbDst += blockSize;
    }

    memcpy( pbChainingValue, count, blockSize );

    SymCryptWipeKnownSize( buf, sizeof( buf ));
}

VOID
SYMCRYPT_CALL
SymCryptCtrMsb64(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData )
{
    SYMCRYPT_ALIGN BYTE    buf[2 * SYMCRYPT_MAX_BLOCK_SIZE];
    PBYTE   count = &buf[0];
    PBYTE   keystream= &buf[SYMCRYPT_MAX_BLOCK_SIZE];
    SIZE_T blockSize;
    PCBYTE pbSrcEnd;

    if( pBlockCipher->ctrMsb64Func != NULL )
    {
        //
        // Use optimized implementation if available
        //
        (*pBlockCipher->ctrMsb64Func)( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
        return;
    }

    blockSize = pBlockCipher->blockSize;
    SYMCRYPT_ASSERT( blockSize <= SYMCRYPT_MAX_BLOCK_SIZE );

    //
    // Compute the end of the data, rounding the size down to a multiple of the block size.
    //
    pbSrcEnd = &pbSrc[ cbData & ~(blockSize - 1) ];

    //
    // We keep the chaining state in a local buffer to enforce the read-once write-once rule.
    // It also improves memory locality.
    //
    #pragma warning(suppress: 22105)
    memcpy( count, pbChainingValue, blockSize );
    while( pbSrc < pbSrcEnd )
    {
        SYMCRYPT_ASSERT( pbSrc <= pbSrcEnd - blockSize );   // help PreFast
        (*pBlockCipher->encryptFunc)( pExpandedKey, count, keystream );
        SymCryptXorBytes( keystream, pbSrc, pbDst, blockSize );

        //
        // We only need to increment the last 64 bits of the counter value.
        //
        SYMCRYPT_STORE_MSBFIRST64( &count[ blockSize-8 ], 1 + SYMCRYPT_LOAD_MSBFIRST64( &count[ blockSize-8 ] ) );

        pbSrc += blockSize;
        pbDst += blockSize;
    }

    memcpy( pbChainingValue, count, blockSize );

    SymCryptWipeKnownSize( buf, sizeof( buf ));
}

VOID
SYMCRYPT_CALL
SymCryptCfbEncrypt(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
                                SIZE_T                  cbShift,
    _In_                        PCVOID                  pExpandedKey,
        _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData )
//
// Encrypt a buffer using the CFB cipher mode.
//
// This implements the CFB mode using a 1-byte feedback shift.
// This requires a block cipher encryption call for each byte, which is very slow.
// Use of this cipher mode is not recommended.
//
// - pBlockCipher is a pointer to the block cipher description table.
//      Suitable description tables for all ciphers in this library have been pre-defined.
// - pExpandedKey points to the expanded key to use. This generic function uses PVOID so there
//      is no type safety to ensure that the expanded key and the encryption function match.
// - pbChainingValue points to the chaining value. On entry and exit it
//      contains the last blockSize ciphertext bytes.
// - pbSrc is the input data buffer that will be encrypted/decrypted.
// - cbData. Number of bytes to encrypt/decrypt. This must be a multiple of the block size.
// - pbDst is the output buffer that receives the encrypted/decrypted data. The input and output
//      buffers may be the same or non-overlapping, but may not partially overlap.
//
{
    SYMCRYPT_ALIGN BYTE buf[2*SYMCRYPT_MAX_BLOCK_SIZE];
    PBYTE   chain = &buf[0];
    PBYTE   tmp   = &buf[SYMCRYPT_MAX_BLOCK_SIZE];
    SIZE_T  blockSize;

    blockSize = pBlockCipher->blockSize;
    SYMCRYPT_ASSERT( blockSize <= SYMCRYPT_MAX_BLOCK_SIZE );

    // Force cbShift to either be 1 or blockSize
    if(cbShift != 1)
    {
        cbShift = blockSize;
    }

    memcpy( chain, pbChainingValue, blockSize );
    while( cbData >= cbShift )
    {
        (*pBlockCipher->encryptFunc)( pExpandedKey, chain, tmp );
        SymCryptXorBytes( pbSrc, tmp, tmp, cbShift );           // tmp[0..cbShift-1] ^= pbSrc[0..cbShift-1]
        memcpy( pbDst, tmp, cbShift );

        memmove( chain, chain + cbShift, blockSize - cbShift );
        memcpy( chain + blockSize - cbShift, tmp, cbShift );

        pbDst += cbShift;
        pbSrc += cbShift;
        cbData -= cbShift;
    }

    memcpy( pbChainingValue, chain, blockSize );
}


VOID
SYMCRYPT_CALL
SymCryptCfbDecrypt(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
                                SIZE_T                  cbShift,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData )
{
    SYMCRYPT_ALIGN BYTE buf[2*SYMCRYPT_MAX_BLOCK_SIZE];
    PBYTE   chain = &buf[0];
    PBYTE   tmp   = &buf[SYMCRYPT_MAX_BLOCK_SIZE];
    SIZE_T  blockSize;

    blockSize = pBlockCipher->blockSize;
    SYMCRYPT_ASSERT( blockSize <= SYMCRYPT_MAX_BLOCK_SIZE );

    // Force cbShift to either be 1 or blockSize
    if(cbShift != 1)
    {
        cbShift = blockSize;
    }

    memcpy( chain, pbChainingValue, blockSize );
    while( cbData >= cbShift )
    {
        (*pBlockCipher->encryptFunc)( pExpandedKey, chain, tmp );

        //
        // First we update the chain block
        //

        memmove( chain, chain + cbShift, blockSize - cbShift );
        memcpy( chain + blockSize - cbShift, pbSrc, cbShift );

        //
        // To obey the read-once rule, we take the ciphertext from the updated chain block.
        //
        SymCryptXorBytes( chain + blockSize - cbShift, tmp, pbDst, cbShift );

        pbDst += cbShift;
        pbSrc += cbShift;
        cbData -= cbShift;
    }

    memcpy( pbChainingValue, chain, blockSize );
}
