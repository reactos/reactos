//
// aeskw.c    Implementation of the AES-KW(P) block cipher modes
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// The KW and KWP modes have inherently terrible performance characteristics from how they are
// defined. Notably, they require a serial chain of AES block operations 12x longer than an
// equivalent AES-CBC encryption (which is already not a favored mode just because of the serial
// nature).
// Additionally the intermediate state of AES-KW and AES-KWP must be of a size proportional to
// the plaintext / ciphertext, rather than fitting into some constant-sized state.
//
// The current strategy for intermediate state handling is to allocate an internal buffer for
// the state. We expect that the caller does not care too much about performance if they are using
// these modes, so the overhead of an allocation per operation should not be a problem.
//
// While it is possible to expose an API surface which uses the destination buffer as a scratch
// buffer to store intermediate state, this would break the read-once/write-once rule, making the
// API surface brittle to misuse if the caller is encrypting to memory that may be in a different
// security domain (i.e. kernel caller encrypting a secret directly into memory which is mapped to
// user mode).
// If we need to expose a non-allocating version, we can introduce a lower-level API where the
// caller provides an appropriately sized scratch buffer, but we will cross that bridge if it is
// required.
//

#include "precomp.h"

const UINT64 SymCryptAesKwDefaultICV    = 0xA6A6A6A6A6A6A6A6;
const UINT32 SymCryptAesKwpDefaultICV   = 0xA65959A6;
#define SYMCRYPT_AES_SEMIBLOCK_SIZE (SYMCRYPT_AES_BLOCK_SIZE / 2)

const SIZE_T SymCryptAesKWMinPlaintextLen   = 16u;          // 2*SYMCRYPT_AES_SEMIBLOCK_SIZE
const SIZE_T SymCryptAesKWMaxPlaintextLen   = (1u<<31)-8;
const SIZE_T SymCryptAesKWMinCiphertextLen  = 24u;          // 3*SYMCRYPT_AES_SEMIBLOCK_SIZE
const SIZE_T SymCryptAesKWMaxCiphertextLen  = (1u<<31);

const SIZE_T SymCryptAesKWPMinPlaintextLen  = 1u;
const SIZE_T SymCryptAesKWPMaxPlaintextLen  = (1u<<31)-8;
const SIZE_T SymCryptAesKWPMinCiphertextLen = 16u;
const SIZE_T SymCryptAesKWPMaxCiphertextLen = (1u<<31);

//
// This function corresponds to algorithm W(S) from section 6.1 of SP 800-38F
//
// We perform this algorithm destructively in place, reading and writing to the same location
// multiple times
//
static
VOID
SYMCRYPT_CALL
SymCryptAesKwxInternalWrap(
    _In_                            PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_bytes_(cbBuf)    PBYTE                       pbBuf,
                                    UINT32                      cbBuf )
{
    SYMCRYPT_ALIGN BYTE activeBlock[SYMCRYPT_AES_BLOCK_SIZE];
    const UINT32 nSemiBlocks = cbBuf / SYMCRYPT_AES_SEMIBLOCK_SIZE; // n per SP 800-38F
    UINT64 encryptionIdx = 1;                                       // t per SP 800-38F
    UINT64 lowHalfTemp = 0;

    SYMCRYPT_ASSERT((cbBuf & (SYMCRYPT_AES_SEMIBLOCK_SIZE-1)) == 0);
    SYMCRYPT_ASSERT(cbBuf >= SymCryptAesKWMinCiphertextLen);
    SYMCRYPT_ASSERT(cbBuf <= SymCryptAesKWMaxCiphertextLen);

    // Special case for first encryption
    // Initialize the low half of active block with the first semi-block of input
    memcpy( activeBlock, pbBuf, SYMCRYPT_AES_SEMIBLOCK_SIZE);

    for( UINT32 outerLoopCnt = 0; outerLoopCnt < 6; outerLoopCnt++ )
    {
        for( UINT32 innerLoopCnt = 1; innerLoopCnt < nSemiBlocks; innerLoopCnt++ )
        {
            SIZE_T bufOffset = innerLoopCnt*SYMCRYPT_AES_SEMIBLOCK_SIZE;

            // Initialize the high half of active block to semi-block from buf
            memcpy( activeBlock+SYMCRYPT_AES_SEMIBLOCK_SIZE, pbBuf+bufOffset, SYMCRYPT_AES_SEMIBLOCK_SIZE);

            // Encrypt activeBlock in place
            SymCryptAesEncrypt( pExpandedKey, activeBlock, activeBlock );

            // Store the high half of result back to semi-block from buf
            memcpy( pbBuf+bufOffset, activeBlock+SYMCRYPT_AES_SEMIBLOCK_SIZE, SYMCRYPT_AES_SEMIBLOCK_SIZE );

            // Use the low half of the result and the next encryptionIdx to
            // initialize the low half of the next encryption
            lowHalfTemp = SYMCRYPT_LOAD_LSBFIRST64( activeBlock );
            lowHalfTemp ^= SYMCRYPT_BSWAP64( encryptionIdx );
            SYMCRYPT_STORE_LSBFIRST64( activeBlock, lowHalfTemp );

            // Update encryptionIdx
            encryptionIdx++;
        }
    }

    SYMCRYPT_ASSERT( (encryptionIdx-1) == (nSemiBlocks-1)*6 );

    // Special case for last encryption
    // Store the final low half of encryption as the first semi-block of output
    SYMCRYPT_STORE_LSBFIRST64( pbBuf, lowHalfTemp );

    SymCryptWipeKnownSize( activeBlock, sizeof(activeBlock) );
}

//
// This function corresponds to algorithm W^-1(S) from section 6.1 of SP 800-38F
//
// We perform this algorithm destructively in place, reading and writing to the same location
// multiple times
//
static
VOID
SYMCRYPT_CALL
SymCryptAesKwxInternalUnwrap(
    _In_                            PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_bytes_(cbBuf)    PBYTE                       pbBuf,
                                    UINT32                      cbBuf )
{
    SYMCRYPT_ALIGN BYTE activeBlock[SYMCRYPT_AES_BLOCK_SIZE];
    const UINT32 nSemiBlocks = cbBuf / SYMCRYPT_AES_SEMIBLOCK_SIZE; // n per SP 800-38F
    UINT64 decryptionIdx = 6*(nSemiBlocks-1);                       // t per SP 800-38F
    UINT64 lowHalfTemp = 0;

    SYMCRYPT_ASSERT((cbBuf & (SYMCRYPT_AES_SEMIBLOCK_SIZE-1)) == 0);
    SYMCRYPT_ASSERT(cbBuf >= SymCryptAesKWMinCiphertextLen);
    SYMCRYPT_ASSERT(cbBuf <= SymCryptAesKWMaxCiphertextLen);

    // Special case for first decryption
    // Initialize the low half temporary with the first semi-block of input
    lowHalfTemp = SYMCRYPT_LOAD_LSBFIRST64( pbBuf );

    for( UINT32 outerLoopCnt = 0; outerLoopCnt < 6; outerLoopCnt++ )
    {
        for( UINT32 innerLoopCnt = nSemiBlocks-1; innerLoopCnt > 0; innerLoopCnt-- )
        {
            SIZE_T bufOffset = innerLoopCnt*SYMCRYPT_AES_SEMIBLOCK_SIZE;

            // Update low half with decryptionIdx and store to low half of active block
            lowHalfTemp ^= SYMCRYPT_BSWAP64( decryptionIdx );
            SYMCRYPT_STORE_LSBFIRST64( activeBlock, lowHalfTemp );

            // Initialize the high half of active block to semi-block from buf
            memcpy( activeBlock+SYMCRYPT_AES_SEMIBLOCK_SIZE, pbBuf+bufOffset, SYMCRYPT_AES_SEMIBLOCK_SIZE);

            // Decrypt activeBlock in place
            SymCryptAesDecrypt( pExpandedKey, activeBlock, activeBlock );

            // Store the high half of result back to semi-block from buf
            memcpy( pbBuf+bufOffset, activeBlock+SYMCRYPT_AES_SEMIBLOCK_SIZE, SYMCRYPT_AES_SEMIBLOCK_SIZE );

            // Update decryptionIdx
            decryptionIdx--;

            // Use the low half of the result to set the low half temporary
            lowHalfTemp = SYMCRYPT_LOAD_LSBFIRST64( activeBlock );
        }
    }

    SYMCRYPT_ASSERT( decryptionIdx == 0 );

    // Special case for last decryption
    // Store the final low half of decryption as the first semi-block of output
    SYMCRYPT_STORE_LSBFIRST64( pbBuf, lowHalfTemp );

    SymCryptWipeKnownSize( activeBlock, sizeof(activeBlock) );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesKwEncrypt(
    _In_                                PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbSrc)                   PCBYTE                      pbSrc,
                                        SIZE_T                      cbSrc,
    _Out_writes_to_(cbDst, *pcbResult)  PBYTE                       pbDst,
                                        SIZE_T                      cbDst,
    _Out_                               SIZE_T*                     pcbResult )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE  pbScratch = NULL;
    UINT32 cbScratch = 0;

    if( (cbSrc < SymCryptAesKWMinPlaintextLen) ||
        (cbSrc > SymCryptAesKWMaxPlaintextLen) ||
        ((cbSrc & (SYMCRYPT_AES_SEMIBLOCK_SIZE-1)) != 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbScratch = ((UINT32) cbSrc)+SYMCRYPT_AES_SEMIBLOCK_SIZE;
    if( cbDst < cbScratch )
    {
        scError = SYMCRYPT_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if( pbScratch == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // set up input buffer as ICV1 || P
    SYMCRYPT_STORE_LSBFIRST64( pbScratch, SymCryptAesKwDefaultICV );
    memcpy( pbScratch+8, pbSrc, cbSrc );

    // encrypt input buffer in place
    SymCryptAesKwxInternalWrap( pExpandedKey, pbScratch, cbScratch );

    // copy encrypted buffer to output
    memcpy( pbDst, pbScratch, cbScratch );
    *pcbResult = cbScratch;

cleanup:
    if( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesKwDecrypt(
    _In_                                PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbSrc)                   PCBYTE                      pbSrc,
                                        SIZE_T                      cbSrc,
    _Out_writes_to_(cbDst, *pcbResult)  PBYTE                       pbDst,
                                        SIZE_T                      cbDst,
    _Out_                               SIZE_T*                     pcbResult )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE  pbScratch = NULL;
    UINT32 cbScratch = 0;

    if( (cbSrc < SymCryptAesKWMinCiphertextLen) ||
        (cbSrc > SymCryptAesKWMaxCiphertextLen) ||
        ((cbSrc & (SYMCRYPT_AES_SEMIBLOCK_SIZE-1)) != 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbScratch = (UINT32) cbSrc;
    if( cbDst < cbScratch-SYMCRYPT_AES_SEMIBLOCK_SIZE )
    {
        scError = SYMCRYPT_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if( pbScratch == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // set up input buffer as C
    memcpy( pbScratch, pbSrc, cbSrc );

    // decrypt input buffer in place
    SymCryptAesKwxInternalUnwrap( pExpandedKey, pbScratch, cbScratch );

    // check first semi-block has the expected value
    if( SYMCRYPT_LOAD_LSBFIRST64( pbScratch ) != SymCryptAesKwDefaultICV )
    {
        scError = SYMCRYPT_AUTHENTICATION_FAILURE;
        goto cleanup;
    }

    // copy decrypted buffer to output
    memcpy( pbDst, pbScratch+SYMCRYPT_AES_SEMIBLOCK_SIZE, cbScratch-SYMCRYPT_AES_SEMIBLOCK_SIZE );
    *pcbResult = cbScratch-SYMCRYPT_AES_SEMIBLOCK_SIZE;

cleanup:
    if( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }
    return scError;

}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesKwpEncrypt(
    _In_                                PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbSrc)                   PCBYTE                      pbSrc,
                                        SIZE_T                      cbSrc,
    _Out_writes_to_(cbDst, *pcbResult)  PBYTE                       pbDst,
                                        SIZE_T                      cbDst,
    _Out_                               SIZE_T*                     pcbResult )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE  pbScratch = NULL;
    UINT32 cbScratch = 0;
    UINT32 cbPad = 0;

    if( (cbSrc < SymCryptAesKWPMinPlaintextLen) ||
        (cbSrc > SymCryptAesKWPMaxPlaintextLen) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbPad = SYMCRYPT_AES_SEMIBLOCK_SIZE - ((UINT32) cbSrc & (SYMCRYPT_AES_SEMIBLOCK_SIZE-1));
    if( cbPad == SYMCRYPT_AES_SEMIBLOCK_SIZE )
    {
        cbPad = 0;
    }

    cbScratch = (UINT32) cbSrc + SYMCRYPT_AES_SEMIBLOCK_SIZE + cbPad;
    if( cbDst < cbScratch )
    {
        scError = SYMCRYPT_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    SYMCRYPT_ASSERT( cbScratch >= 16 );

    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if( pbScratch == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // set up input buffer as ICV2 || len(P) || P || PAD
    SYMCRYPT_STORE_LSBFIRST32( pbScratch,   SymCryptAesKwpDefaultICV );
    SYMCRYPT_STORE_MSBFIRST32( pbScratch+4, (UINT32) cbSrc );
    // pad by unconditionally setting the last 8 bytes to 0
    // then overwrite some or all of the padding bytes with plaintext
    SYMCRYPT_STORE_LSBFIRST64( pbScratch+cbScratch-SYMCRYPT_AES_SEMIBLOCK_SIZE, 0u );
    memcpy( pbScratch+8, pbSrc, cbSrc );

    // encrypt input buffer in place
    if( cbScratch == SYMCRYPT_AES_BLOCK_SIZE )
    {
        // special case for AES-KWP with small plaintext
        SymCryptAesEncrypt( pExpandedKey, pbScratch, pbScratch );
    } else {
        SymCryptAesKwxInternalWrap( pExpandedKey, pbScratch, cbScratch );
    }

    // copy encrypted buffer to output
    memcpy( pbDst, pbScratch, cbScratch );
    *pcbResult = cbScratch;

cleanup:
    if( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesKwpDecrypt(
    _In_                                PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbSrc)                   PCBYTE                      pbSrc,
                                        SIZE_T                      cbSrc,
    _Out_writes_to_(cbDst, *pcbResult)  PBYTE                       pbDst,
                                        SIZE_T                      cbDst,
    _Out_                               SIZE_T*                     pcbResult )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE  pbScratch = NULL;
    UINT32 cbScratch = 0;
    UINT32 cbPlaintext = 0;
    UINT32 cbPad = 0;
    UINT32 mVerificationError = 0;  // Mask indicating whether the decrypted buffer is malformed
    UINT32 mIsPlaintext = 0;        // Mask for plaintext bytes in the final semi-block

    if( (cbSrc < SymCryptAesKWPMinCiphertextLen) ||
        (cbSrc > SymCryptAesKWPMaxCiphertextLen) ||
        ((cbSrc & (SYMCRYPT_AES_SEMIBLOCK_SIZE-1)) != 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbScratch = (UINT32) cbSrc;
    if( cbDst < cbScratch-SYMCRYPT_AES_SEMIBLOCK_SIZE-7 )
    {
        scError = SYMCRYPT_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if( pbScratch == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // set up input buffer as C
    memcpy( pbScratch, pbSrc, cbSrc );

    // decrypt input buffer in place
    if( cbScratch == SYMCRYPT_AES_BLOCK_SIZE )
    {
        // special case for AES-KWP with small ciphertext
        SymCryptAesDecrypt( pExpandedKey, pbScratch, pbScratch );
    } else {
        SymCryptAesKwxInternalUnwrap( pExpandedKey, pbScratch, cbScratch );
    }

    // Check if the decrypted buffer is of an expected form
    // check bytes [0..3] are expected ICV
    mVerificationError |= SYMCRYPT_LOAD_LSBFIRST32( pbScratch ) ^ SymCryptAesKwpDefaultICV;

    // check bytes [4..7] are a valid plaintext length (i.e. computed cbPad in range [0,7])
    cbPlaintext = SYMCRYPT_LOAD_MSBFIRST32( pbScratch+4 );
    cbPad = (UINT32) cbSrc - cbPlaintext - SYMCRYPT_AES_SEMIBLOCK_SIZE;
    mVerificationError |= (cbPad & 0xfffffff8);

    // check that padding is all 0s
    for( UINT32 i = 1; i<SYMCRYPT_AES_SEMIBLOCK_SIZE; i++ )
    {
        mIsPlaintext = SymCryptMask32LtU31(i, SYMCRYPT_AES_SEMIBLOCK_SIZE-(cbPad&7));
        mVerificationError |= ((UINT32) pbScratch[ cbScratch-SYMCRYPT_AES_SEMIBLOCK_SIZE+i ]) & ~mIsPlaintext;
    }

    // Now if there was any verification error, we fail
    if( mVerificationError != 0 )
    {
        scError = SYMCRYPT_AUTHENTICATION_FAILURE;
        goto cleanup;
    }

    // We are variable time w.r.t. the plaintext length on success
    if( cbDst < cbPlaintext )
    {
        scError = SYMCRYPT_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    // copy decrypted buffer to output
    memcpy( pbDst, pbScratch+SYMCRYPT_AES_SEMIBLOCK_SIZE, cbPlaintext );
    *pcbResult = cbPlaintext;

cleanup:
    if( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }
    return scError;

}
