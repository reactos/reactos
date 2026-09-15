//
// aes-default.c   code for AES implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// This is the interface for the default AES implementation.
// On each platform, this is the fastest AES implementation irrespective of code size.
// It uses assembler, XMM, or any other trick.
//


#include "precomp.h"

//
// Virtual table for generic functions
// This allows us to default to generic implementations for some modes without pulling in all the
// dedicated functions.
// We use this when we cannot use the optimized implementations for some reason.
//
const SYMCRYPT_BLOCKCIPHER SymCryptAesBlockCipherNoOpt = {
    &SymCryptAesExpandKey,
#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM
    &SymCryptAesEncryptAsm,
    &SymCryptAesDecryptAsm,
#else
    &SymCryptAesEncryptC,
    &SymCryptAesDecryptC,
#endif
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    SYMCRYPT_AES_BLOCK_SIZE,
    sizeof( SYMCRYPT_AES_EXPANDED_KEY ),
};

VOID
SYMCRYPT_CALL
SymCryptAes4Sbox( _In_reads_(4) PCBYTE pIn, _Out_writes_(4) PBYTE pOut, BOOL UseSimd )
{
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    if( UseSimd )
    {
        SymCryptAes4SboxXmm( pIn, pOut );
    } else {
        SymCryptAes4SboxC( pIn, pOut );
    }
#elif SYMCRYPT_CPU_ARM64
    if( UseSimd )
    {
        SymCryptAes4SboxNeon( pIn, pOut );
    } else {
        SymCryptAes4SboxC( pIn, pOut );
    }
#else
    UNREFERENCED_PARAMETER( UseSimd );
    SymCryptAes4SboxC( pIn, pOut );         // never use XMM on SaveXmm arch, save/restore overhead is too large.
#endif
}

VOID
SYMCRYPT_CALL
SymCryptAesCreateDecryptionRoundKey(
    _In_reads_(16)      PCBYTE  pEncryptionRoundKey,
    _Out_writes_(16)    PBYTE   pDecryptionRoundKey,
                        BOOL    UseSimd )
{
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    if( UseSimd )
    {
        SymCryptAesCreateDecryptionRoundKeyXmm( pEncryptionRoundKey, pDecryptionRoundKey );
    } else {
        SymCryptAesCreateDecryptionRoundKeyC( pEncryptionRoundKey, pDecryptionRoundKey );
    }
#elif SYMCRYPT_CPU_ARM64
    if( UseSimd )
    {
        SymCryptAesCreateDecryptionRoundKeyNeon( pEncryptionRoundKey, pDecryptionRoundKey );
    } else {
        SymCryptAesCreateDecryptionRoundKeyC( pEncryptionRoundKey, pDecryptionRoundKey );
    }
#else
    UNREFERENCED_PARAMETER( UseSimd );
    SymCryptAesCreateDecryptionRoundKeyC( pEncryptionRoundKey, pDecryptionRoundKey );   // never use XMM on SaveXmm arch, save/restore overhead is too large.
#endif
}

VOID
SYMCRYPT_CALL
SymCryptAesEncrypt(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_(SYMCRYPT_AES_BLOCK_SIZE)     PCBYTE                      pbSrc,
    _Out_writes_(SYMCRYPT_AES_BLOCK_SIZE)   PBYTE                       pbDst )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) )
    {
        SymCryptAesEncryptXmm( pExpandedKey, pbSrc, pbDst );
    } else {
        SymCryptAesEncryptAsm( pExpandedKey, pbSrc, pbDst );
    }
#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptAesEncryptXmm( pExpandedKey, pbSrc, pbDst );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptAesEncryptAsm( pExpandedKey, pbSrc, pbDst );
    }
#elif SYMCRYPT_CPU_ARM
    SymCryptAesEncryptAsm( pExpandedKey, pbSrc, pbDst );
#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES ) )
    {
        SymCryptAesEncryptNeon( pExpandedKey, pbSrc, pbDst );
    } else {
        SymCryptAesEncryptC( pExpandedKey, pbSrc, pbDst );
    }
#else
    SymCryptAesEncryptC( pExpandedKey, pbSrc, pbDst );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptAesDecrypt(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_(SYMCRYPT_AES_BLOCK_SIZE)     PCBYTE                      pbSrc,
    _Out_writes_(SYMCRYPT_AES_BLOCK_SIZE)   PBYTE                       pbDst )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) )
    {
        SymCryptAesDecryptXmm( pExpandedKey, pbSrc, pbDst );
    } else {
        SymCryptAesDecryptAsm( pExpandedKey, pbSrc, pbDst );
    }
#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptAesDecryptXmm( pExpandedKey, pbSrc, pbDst );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptAesDecryptAsm( pExpandedKey, pbSrc, pbDst );
    }
#elif SYMCRYPT_CPU_ARM
    SymCryptAesDecryptAsm( pExpandedKey, pbSrc, pbDst );
#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES ) )
    {
        SymCryptAesDecryptNeon( pExpandedKey, pbSrc, pbDst );
    } else {
        SymCryptAesDecryptC( pExpandedKey, pbSrc, pbDst );
    }
#else
    SymCryptAesDecryptC( pExpandedKey, pbSrc, pbDst );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptAesCbcEncrypt(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) )
    {
        SymCryptAesCbcEncryptXmm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    } else {
        SymCryptAesCbcEncryptAsm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    }
#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptAesCbcEncryptXmm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptAesCbcEncryptAsm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    }
#elif SYMCRYPT_CPU_ARM
    SymCryptAesCbcEncryptAsm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES ) )
    {
        SymCryptAesCbcEncryptNeon( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    } else {
        SymCryptCbcEncrypt( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    }
#else
    SymCryptCbcEncrypt( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptAesCbcDecrypt(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) )
    {
        SymCryptAesCbcDecryptXmm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    } else {
        SymCryptAesCbcDecryptAsm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    }
#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptAesCbcDecryptXmm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptAesCbcDecryptAsm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    }
#elif SYMCRYPT_CPU_ARM
    SymCryptAesCbcDecryptAsm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES ) )
    {
        SymCryptAesCbcDecryptNeon( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    } else {
        SymCryptCbcDecrypt( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    }
#else
    SymCryptCbcDecrypt( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptAesEcbEncrypt(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) )
    {
        SymCryptAesEcbEncryptXmm( pExpandedKey, pbSrc, pbDst, cbData );
    } else {
        SymCryptAesEcbEncryptAsm( pExpandedKey, pbSrc, pbDst, cbData );
    }
#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptAesEcbEncryptXmm( pExpandedKey, pbSrc, pbDst, cbData );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptAesEcbEncryptAsm( pExpandedKey, pbSrc, pbDst, cbData );
    }
#elif SYMCRYPT_CPU_ARM
    SymCryptAesEcbEncryptAsm( pExpandedKey, pbSrc, pbDst, cbData );
#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES ) )
    {
        SymCryptAesEcbEncryptNeon( pExpandedKey, pbSrc, pbDst, cbData );
    } else {
        SymCryptAesEcbEncryptC( pExpandedKey, pbSrc, pbDst, cbData );
    }
#else
    SymCryptAesEcbEncryptC( pExpandedKey, pbSrc, pbDst, cbData );
#endif
}

//
// NOTE: There is no reason that SymCryptAesEcbDecrypt could not have unrolled versions similar to
// SymCryptAesEcbEncrypt if a real use case requiring large scale Ecb decryption is found.
// For now just decrypt 1 block at a time to reduce code size.
//
VOID
SYMCRYPT_CALL
SymCryptAesEcbDecrypt(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    while( cbData >= SYMCRYPT_AES_BLOCK_SIZE )
    {
        SymCryptAesDecrypt( pExpandedKey, pbSrc, pbDst );
        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        pbDst += SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= SYMCRYPT_AES_BLOCK_SIZE;
    }
}

VOID
SYMCRYPT_CALL
SymCryptAesCbcMac(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbData,
                                                SIZE_T                      cbData )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) )
    {
        SymCryptAesCbcMacXmm( pExpandedKey, pbChainingValue, pbData, cbData );
    } else {
        SYMCRYPT_ASSERT( SymCryptAesBlockCipherNoOpt.blockSize == SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptCbcMac( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbData, cbData );
    }
#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptAesCbcMacXmm( pExpandedKey, pbChainingValue, pbData, cbData );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SYMCRYPT_ASSERT( SymCryptAesBlockCipherNoOpt.blockSize == SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptCbcMac( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbData, cbData );
    }
#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES ) )
    {
        SymCryptAesCbcMacNeon( pExpandedKey, pbChainingValue, pbData, cbData );
    } else {
        SYMCRYPT_ASSERT( SymCryptAesBlockCipherNoOpt.blockSize == SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptCbcMac( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbData, cbData );
    }
#else
    SYMCRYPT_ASSERT( SymCryptAesBlockCipherNoOpt.blockSize == SYMCRYPT_AES_BLOCK_SIZE );
    SymCryptCbcMac( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbData, cbData );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptAesCtrMsb32(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) )
    {
        SymCryptAesCtrMsb32Xmm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    } else {
        SYMCRYPT_ASSERT( SymCryptAesBlockCipherNoOpt.blockSize == SYMCRYPT_AES_BLOCK_SIZE ); // keep Prefast happy
        SymCryptCtrMsb32( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    }

#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptAesCtrMsb32Xmm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SYMCRYPT_ASSERT( SymCryptAesBlockCipherNoOpt.blockSize == SYMCRYPT_AES_BLOCK_SIZE ); // keep Prefast happy
        SymCryptCtrMsb32( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    }

#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES ) )
    {
        SymCryptAesCtrMsb32Neon( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    } else {
        SymCryptCtrMsb32( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    }

#else
    SYMCRYPT_ASSERT( SymCryptAesBlockCipherNoOpt.blockSize == SYMCRYPT_AES_BLOCK_SIZE ); // keep Prefast happy
    SymCryptCtrMsb32( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptAesCtrMsb64(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) )
    {
        SymCryptAesCtrMsb64Xmm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    } else {
        SymCryptAesCtrMsb64Asm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    }

#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptAesCtrMsb64Xmm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptAesCtrMsb64Asm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    }

#elif SYMCRYPT_CPU_ARM
    SymCryptAesCtrMsb64Asm( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );

#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES ) )
    {
        SymCryptAesCtrMsb64Neon( pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    } else {
        SymCryptCtrMsb64( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
    }

#else
    SYMCRYPT_ASSERT( SymCryptAesBlockCipherNoOpt.blockSize == SYMCRYPT_AES_BLOCK_SIZE );        // keep Prefast happy
    SymCryptCtrMsb64( &SymCryptAesBlockCipherNoOpt, pExpandedKey, pbChainingValue, pbSrc, pbDst, cbData );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptAesGcmEncryptPartOnePass(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData )
{
    SIZE_T      bytesToProcess;
#if SYMCRYPT_CPU_AMD64
    SYMCRYPT_EXTENDED_SAVE_DATA SaveData;
#endif

    //
    // We have entered the encrypt phase, the AAD has been padded to be a multiple of block size
    // We know that the bytes still to use in the key stream buffer and the bytes left to fill the
    // macBlock will be the same in the context of this function
    //
    SYMCRYPT_ASSERT( (pState->cbData & SYMCRYPT_GCM_BLOCK_MOD_MASK) == pState->bytesInMacBlock );

    //
    // We update pState->cbData once before we modify cbData.
    // pState->cbData is not used in the rest of this function
    //
    SYMCRYPT_ASSERT( pState->cbData + cbData <= SYMCRYPT_GCM_MAX_DATA_SIZE );
    pState->cbData += cbData;

    if( pState->bytesInMacBlock > 0 )
    {
        bytesToProcess = SYMCRYPT_MIN( cbData, SYMCRYPT_GCM_BLOCK_SIZE - pState->bytesInMacBlock );
        SymCryptXorBytes(
            pbSrc,
            &pState->keystreamBlock[pState->bytesInMacBlock],
            &pState->macBlock[pState->bytesInMacBlock],
            bytesToProcess );
        memcpy( pbDst, &pState->macBlock[pState->bytesInMacBlock], bytesToProcess );
        pbSrc += bytesToProcess;
        pbDst += bytesToProcess;
        cbData -= bytesToProcess;
        pState->bytesInMacBlock += bytesToProcess;

        if( pState->bytesInMacBlock == SYMCRYPT_GCM_BLOCK_SIZE )
        {
            SymCryptGHashAppendData(    &pState->pKey->ghashKey,
                                        &pState->ghashState,
                                        &pState->macBlock[0],
                                        SYMCRYPT_GCM_BLOCK_SIZE );
            pState->bytesInMacBlock = 0;
        }

        //
        // If there are bytes left in the key stream buffer, then cbData == 0 and we're done.
        // If we used up all the bytes, then we are fine, no need to compute the next key stream block
        //
    }

    if( cbData >= SYMCRYPT_GCM_BLOCK_SIZE )
    {
        bytesToProcess = cbData & SYMCRYPT_GCM_BLOCK_ROUND_MASK;

        //
        // We use a Gcm function that increments the CTR by 64 bits, rather than the 32 bits that GCM requires.
        // As we only support 12-byte nonces, the 32-bit counter never overflows, and we can safely use
        // the 64-bit incrementing primitive.
        // If we ever support other nonce sizes this is going to be a big problem.
        // You can't fake a 32-bit counter using a 64-bit counter function without side-channels that expose
        // information about the current counter value.
        // With other nonce sizes the actual counter value itself is not public, so we can't expose that.
        // We can do two things:
        // - create SymCryptAesGcmEncryptXXX32
        // - Accept that we leak information about the counter value; after all it is not treated as a
        //   secret when the nonce is 12 bytes.
        //
        SYMCRYPT_ASSERT( pState->pKey->pBlockCipher->blockSize == SYMCRYPT_GCM_BLOCK_SIZE );

#if SYMCRYPT_CPU_AMD64
        if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_VAES_256_CODE ) &&
            (bytesToProcess >= GCM_YMM_MINBLOCKS * SYMCRYPT_GCM_BLOCK_SIZE) &&
            SymCryptSaveYmm( &SaveData ) == SYMCRYPT_NO_ERROR )
        {
            SymCryptAesGcmEncryptStitchedYmm_2048(
                &pState->pKey->blockcipherKey.aes,
                &pState->counterBlock[0],
                &pState->pKey->ghashKey.table[0],
                &pState->ghashState,
                pbSrc,
                pbDst,
                bytesToProcess );

            SymCryptRestoreYmm( &SaveData );
        } else {
            SymCryptAesGcmEncryptStitchedXmm(
                &pState->pKey->blockcipherKey.aes,
                &pState->counterBlock[0],
                &pState->pKey->ghashKey.table[0],
                &pState->ghashState,
                pbSrc,
                pbDst,
                bytesToProcess );
        }

#elif SYMCRYPT_CPU_X86
        SymCryptAesGcmEncryptStitchedXmm(
            &pState->pKey->blockcipherKey.aes,
            &pState->counterBlock[0],
            (PSYMCRYPT_GF128_ELEMENT)&pState->pKey->ghashKey.tableSpace[pState->pKey->ghashKey.tableOffset],
            &pState->ghashState,
            pbSrc,
            pbDst,
            bytesToProcess );

#elif SYMCRYPT_CPU_ARM64
        SymCryptAesGcmEncryptStitchedNeon(
            &pState->pKey->blockcipherKey.aes,
            &pState->counterBlock[0],
            &pState->pKey->ghashKey.table[0],
            &pState->ghashState,
            pbSrc,
            pbDst,
            bytesToProcess );

#else
        SymCryptAesCtrMsb32(&pState->pKey->blockcipherKey.aes,
                            &pState->counterBlock[0],
                            pbSrc,
                            pbDst,
                            cbData );
        //
        // We break the read-once/write once rule here by reading the pbDst data back.
        // In this particular situation this is safe, and avoiding it is expensive as it
        // requires an extra copy and an extra memory buffer.
        // The first write exposes the GCM key stream, independent of the underlying data that
        // we are processing. From an attacking point of view we can think of this as literally
        // handing over the key stream. So encryption consists of two steps:
        // - hand over the key stream
        // - MAC some ciphertext
        // In this view (which has equivalent security properties to GCM) is obviously doesn't
        // matter that we read pbDst back.
        //
        SymCryptGHashAppendData(&pState->pKey->ghashKey,
                                &pState->ghashState,
                                pbDst,
                                cbData );

#endif

        pbSrc += bytesToProcess;
        pbDst += bytesToProcess;
        cbData -= bytesToProcess;
    }

    if( cbData > 0 )
    {
        SymCryptWipeKnownSize( &pState->keystreamBlock[0], SYMCRYPT_GCM_BLOCK_SIZE );

        SYMCRYPT_ASSERT( pState->pKey->pBlockCipher->blockSize == SYMCRYPT_GCM_BLOCK_SIZE );
        SymCryptAesCtrMsb32(&pState->pKey->blockcipherKey.aes,
                            &pState->counterBlock[0],
                            &pState->keystreamBlock[0],
                            &pState->keystreamBlock[0],
                            SYMCRYPT_GCM_BLOCK_SIZE );

        SymCryptXorBytes( &pState->keystreamBlock[0], pbSrc, &pState->macBlock[0], cbData );
        memcpy( pbDst, &pState->macBlock[0], cbData );
        pState->bytesInMacBlock = cbData;

        //
        // pState->cbData contains the data length after this call already, so it knows how many
        // bytes are left in the keystream block
        //
    }
}

VOID
SYMCRYPT_CALL
SymCryptAesGcmDecryptPartOnePass(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData )
{
    SIZE_T      bytesToProcess;
#if SYMCRYPT_CPU_AMD64
    SYMCRYPT_EXTENDED_SAVE_DATA SaveData;
#endif

    //
    // We have entered the decrypt phase, the AAD has been padded to be a multiple of block size
    // We know that the bytes still to use in the key stream buffer and the bytes left to fill the
    // macBlock will be the same in the context of this function
    //
    SYMCRYPT_ASSERT( (pState->cbData & SYMCRYPT_GCM_BLOCK_MOD_MASK) == pState->bytesInMacBlock );

    //
    // We update pState->cbData once before we modify cbData.
    // pState->cbData is not used in the rest of this function
    //
    SYMCRYPT_ASSERT( pState->cbData + cbData <= SYMCRYPT_GCM_MAX_DATA_SIZE );
    pState->cbData += cbData;

    if( pState->bytesInMacBlock > 0 )
    {
        bytesToProcess = SYMCRYPT_MIN( cbData, SYMCRYPT_GCM_BLOCK_SIZE - pState->bytesInMacBlock );
        memcpy( &pState->macBlock[pState->bytesInMacBlock], pbSrc, bytesToProcess );
        SymCryptXorBytes(
            &pState->keystreamBlock[pState->bytesInMacBlock],
            &pState->macBlock[pState->bytesInMacBlock],
            pbDst,
            bytesToProcess );

        pbSrc += bytesToProcess;
        pbDst += bytesToProcess;
        cbData -= bytesToProcess;
        pState->bytesInMacBlock += bytesToProcess;

        if( pState->bytesInMacBlock == SYMCRYPT_GCM_BLOCK_SIZE )
        {
            SymCryptGHashAppendData(    &pState->pKey->ghashKey,
                                        &pState->ghashState,
                                        &pState->macBlock[0],
                                        SYMCRYPT_GCM_BLOCK_SIZE );
            pState->bytesInMacBlock = 0;
        }

        //
        // If there are bytes left in the key stream buffer, then cbData == 0 and we're done.
        // If we used up all the bytes, then we are fine, no need to compute the next key stream block
        //
    }

    if( cbData >= SYMCRYPT_GCM_BLOCK_SIZE )
    {
        bytesToProcess = cbData & SYMCRYPT_GCM_BLOCK_ROUND_MASK;

        //
        // We use a Gcm function that increments the CTR by 64 bits, rather than the 32 bits that GCM requires.
        // As we only support 12-byte nonces, the 32-bit counter never overflows, and we can safely use
        // the 64-bit incrementing primitive.
        // If we ever support other nonce sizes this is going to be a big problem.
        // You can't fake a 32-bit counter using a 64-bit counter function without side-channels that expose
        // information about the current counter value.
        // With other nonce sizes the actual counter value itself is not public, so we can't expose that.
        // We can do two things:
        // - create SymCryptAesGcmDecryptXXX32
        // - Accept that we leak information about the counter value; after all it is not treated as a
        //   secret when the nonce is 12 bytes.
        //
        SYMCRYPT_ASSERT( pState->pKey->pBlockCipher->blockSize == SYMCRYPT_GCM_BLOCK_SIZE );

#if SYMCRYPT_CPU_AMD64
        if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_VAES_256_CODE ) &&
            (bytesToProcess >= GCM_YMM_MINBLOCKS * SYMCRYPT_GCM_BLOCK_SIZE) &&
            SymCryptSaveYmm( &SaveData ) == SYMCRYPT_NO_ERROR )
        {
            SymCryptAesGcmDecryptStitchedYmm_2048(
                &pState->pKey->blockcipherKey.aes,
                &pState->counterBlock[0],
                &pState->pKey->ghashKey.table[0],
                &pState->ghashState,
                pbSrc,
                pbDst,
                bytesToProcess );

            SymCryptRestoreYmm( &SaveData );
        } else {
            SymCryptAesGcmDecryptStitchedXmm(
                &pState->pKey->blockcipherKey.aes,
                &pState->counterBlock[0],
                &pState->pKey->ghashKey.table[0],
                &pState->ghashState,
                pbSrc,
                pbDst,
                bytesToProcess );
        }

#elif SYMCRYPT_CPU_X86
        SymCryptAesGcmDecryptStitchedXmm(
            &pState->pKey->blockcipherKey.aes,
            &pState->counterBlock[0],
            (PSYMCRYPT_GF128_ELEMENT)&pState->pKey->ghashKey.tableSpace[pState->pKey->ghashKey.tableOffset],
            &pState->ghashState,
            pbSrc,
            pbDst,
            bytesToProcess );

#elif SYMCRYPT_CPU_ARM64
        SymCryptAesGcmDecryptStitchedNeon(
            &pState->pKey->blockcipherKey.aes,
            &pState->counterBlock[0],
            &pState->pKey->ghashKey.table[0],
            &pState->ghashState,
            pbSrc,
            pbDst,
            bytesToProcess );

#else
        SymCryptGHashAppendData(&pState->pKey->ghashKey,
                                &pState->ghashState,
                                pbSrc,
                                cbData );
        //
        // Do the actual decryption
        // This violates the read-once rule, but it is safe for the same reasons as above
        // in the encryption case.
        //
        SymCryptAesCtrMsb32(&pState->pKey->blockcipherKey.aes,
                            &pState->counterBlock[0],
                            pbSrc,
                            pbDst,
                            cbData );

#endif
        pbSrc += bytesToProcess;
        pbDst += bytesToProcess;
        cbData -= bytesToProcess;
    }

    if( cbData > 0 )
    {
        SymCryptWipeKnownSize( &pState->keystreamBlock[0], SYMCRYPT_GCM_BLOCK_SIZE );

        SYMCRYPT_ASSERT( pState->pKey->pBlockCipher->blockSize == SYMCRYPT_GCM_BLOCK_SIZE );
        SymCryptAesCtrMsb32(&pState->pKey->blockcipherKey.aes,
                            &pState->counterBlock[0],
                            &pState->keystreamBlock[0],
                            &pState->keystreamBlock[0],
                            SYMCRYPT_GCM_BLOCK_SIZE );

        memcpy( &pState->macBlock[0], pbSrc, cbData );
        SymCryptXorBytes(
            &pState->keystreamBlock[0],
            &pState->macBlock[0],
            pbDst,
            cbData );

        pState->bytesInMacBlock = cbData;

        //
        // pState->cbData contains the data length after this call already, so it knows how many
        // bytes are left in the keystream block
        //
    }
}

VOID
SYMCRYPT_CALL
SymCryptAesGcmEncryptPart(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_PCLMULQDQ_CODE ) )
    {
        SymCryptAesGcmEncryptPartOnePass( pState, pbSrc, pbDst, cbData );
    } else {
        SymCryptGcmEncryptPartTwoPass( pState, pbSrc, pbDst, cbData );
    }

#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_PCLMULQDQ_CODE ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptAesGcmEncryptPartOnePass( pState, pbSrc, pbDst, cbData );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptGcmEncryptPartTwoPass( pState, pbSrc, pbDst, cbData );
    }

#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES | SYMCRYPT_CPU_FEATURE_NEON_PMULL ) )
    {
        SymCryptAesGcmEncryptPartOnePass( pState, pbSrc, pbDst, cbData );
    } else {
        SymCryptGcmEncryptPartTwoPass( pState, pbSrc, pbDst, cbData );
    }

#else
    SymCryptGcmEncryptPartTwoPass( pState, pbSrc, pbDst, cbData );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptAesGcmDecryptPart(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData )
{
#if SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_PCLMULQDQ_CODE ) )
    {
        SymCryptAesGcmDecryptPartOnePass( pState, pbSrc, pbDst, cbData );
    } else {
        SymCryptGcmDecryptPartTwoPass( pState, pbSrc, pbDst, cbData );
    }

#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_PCLMULQDQ_CODE ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptAesGcmDecryptPartOnePass( pState, pbSrc, pbDst, cbData );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptGcmDecryptPartTwoPass( pState, pbSrc, pbDst, cbData );
    }

#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES | SYMCRYPT_CPU_FEATURE_NEON_PMULL ) )
    {
        SymCryptAesGcmDecryptPartOnePass( pState, pbSrc, pbDst, cbData );
    } else {
        SymCryptGcmDecryptPartTwoPass( pState, pbSrc, pbDst, cbData );
    }

#else
    SymCryptGcmDecryptPartTwoPass( pState, pbSrc, pbDst, cbData );
#endif
}
