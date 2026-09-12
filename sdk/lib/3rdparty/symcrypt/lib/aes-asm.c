//
// aes-asm.c   code for AES implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//


#include "precomp.h"

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM

VOID
SYMCRYPT_CALL
SymCryptAesEcbEncryptAsm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    while( cbData >= SYMCRYPT_AES_BLOCK_SIZE )
    {
        SymCryptAesEncryptAsm( pExpandedKey, pbSrc, pbDst );
        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        pbDst += SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= SYMCRYPT_AES_BLOCK_SIZE;
    }
}

VOID
SYMCRYPT_CALL
SymCryptAesEcbDecryptAsm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    while( cbData >= SYMCRYPT_AES_BLOCK_SIZE )
    {
        SymCryptAesDecryptAsm( pExpandedKey, pbSrc, pbDst );
        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        pbDst += SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= SYMCRYPT_AES_BLOCK_SIZE;
    }
}

#endif
