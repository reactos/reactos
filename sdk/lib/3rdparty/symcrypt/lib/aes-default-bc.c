//
// aes-default-bc.c   code for AES implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// The SymCrypt API allows callers to use the generic block cipher mode functions and pass
// a pointer to a structure that describes the block cipher.
// This structure contains pointers to all the optimized implementations of the various modes.
// This pulls in all the mode-specific code, which in some cases we don't want.
//
// We isolate the SymCryptAesBlockCipher structure into this separate C file so that it only gets
// pulled in when the application uses this structure.
//

//
// The virtual table for the AES block cipher.
//
// All pointers must point to specialized functions. The general
// block cipher mode functions will call these pointers if they are non-NULL
// so if they point back to an implementation that calls the generic
// mode functions we get an infinite recursion.
//
// NOTE: the compile-time conditions in this file should track the actual implementations in
// aes-default.c.
//

const SYMCRYPT_BLOCKCIPHER SymCryptAesBlockCipher_Fast = {
    &SymCryptAesExpandKey,
    &SymCryptAesEncrypt,
    &SymCryptAesDecrypt,

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64
    &SymCryptAesEcbEncrypt,
#else
    NULL,
#endif

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    &SymCryptAesEcbDecrypt,
#else
    NULL,
#endif

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64
    &SymCryptAesCbcEncrypt,
#else
    NULL,
#endif

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64
    &SymCryptAesCbcDecrypt,
#else
    NULL,
#endif

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64
    &SymCryptAesCbcMac,
#else
    NULL,
#endif

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64
    &SymCryptAesCtrMsb64,
#else
    NULL,
#endif

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64
    &SymCryptAesGcmEncryptPart,
#else
    NULL,
#endif

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64
    &SymCryptAesGcmDecryptPart,
#else
    NULL,
#endif

    SYMCRYPT_AES_BLOCK_SIZE,
    sizeof( SYMCRYPT_AES_EXPANDED_KEY ),
};

//
// This indirection makes it easier to switch implementations in a binary without
// changing the calling code.
//
const PCSYMCRYPT_BLOCKCIPHER SymCryptAesBlockCipher = &SymCryptAesBlockCipher_Fast;
