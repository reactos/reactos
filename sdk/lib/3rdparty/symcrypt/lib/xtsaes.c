//
// xtsaes.c   code for XTS-AES implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXtsAesExpandKey(
    _Out_               PSYMCRYPT_XTS_AES_EXPANDED_KEY  pExpandedKey,
    _In_reads_( cbKey ) PCBYTE                          pbKey,
                        SIZE_T                          cbKey )
{
    SYMCRYPT_ERROR  scError;
    SIZE_T          halfKeySize = cbKey / 2;

    scError = SymCryptAesExpandKey( &pExpandedKey->key1, pbKey, halfKeySize );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    //
    // Pass the 'rest' of the key to the second one. This catches errors such as
    // an attempt to pass a 33 byte key.
    // halfKeySize = 16, which is valid, but this expansion gets a 17-byte key which will fail.
    // Key2 is only used for tweak encryption, so we can use the EncryptOnly key expansion.
    //
    scError = SymCryptAesExpandKeyEncryptOnly( &pExpandedKey->key2, pbKey + halfKeySize, cbKey - halfKeySize );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXtsAesExpandKeyEx(
    _Out_               PSYMCRYPT_XTS_AES_EXPANDED_KEY  pExpandedKey,
    _In_reads_( cbKey ) PCBYTE                          pbKey,
                        SIZE_T                          cbKey,
                        UINT32                          flags )
{
    if( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 )
    {
        // FIPS IG C.I enforces that the two AES keys internally used in XTS-AES are non-equal
        if( cbKey > 64 )
        {
            return SYMCRYPT_WRONG_KEY_SIZE;
        }
        if( SymCryptEqual( pbKey, pbKey+(cbKey/2), (cbKey/2) ) )
        {
            return SYMCRYPT_FIPS_FAILURE;
        }
    }

    return SymCryptXtsAesExpandKey( pExpandedKey, pbKey, cbKey );
}


VOID
SYMCRYPT_CALL
SymCryptXtsAesKeyCopy(
    _In_    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_XTS_AES_EXPANDED_KEY  pDst )
{
    SymCryptAesKeyCopy( &pSrc->key1, &pDst->key1 );
    SymCryptAesKeyCopy( &pSrc->key2, &pDst->key2 );
}

#define N_PARALLEL_TWEAKS   16

#define SYMCRYPT_XTS_AES_LOCALSCRATCH_DEFN \
    SYMCRYPT_ALIGN BYTE localScratch[N_PARALLEL_TWEAKS * SYMCRYPT_AES_BLOCK_SIZE];

#define SYMCRYPT_AesEcbEncryptXxx SymCryptAesEcbEncryptC

#define SYMCRYPT_XtsAesXxx SymCryptXtsAesEncryptInternalC
#define SYMCRYPT_XTSAESDATAUNIT_INVOKE \
    SymCryptXtsAesEncryptDataUnitC( &pExpandedKey->key1, &tweakBuf[i], pbSrc, pbDst, cbDataUnit );
#include "xtsaes_pattern.c"
#undef SYMCRYPT_XtsAesXxx
#undef SYMCRYPT_XTSAESDATAUNIT_INVOKE

#define SYMCRYPT_XtsAesXxx SymCryptXtsAesDecryptInternalC
#define SYMCRYPT_XTSAESDATAUNIT_INVOKE \
    SymCryptXtsAesDecryptDataUnitC( &pExpandedKey->key1, &tweakBuf[i], pbSrc, pbDst, cbDataUnit );
#include "xtsaes_pattern.c"
#undef SYMCRYPT_XtsAesXxx
#undef SYMCRYPT_XTSAESDATAUNIT_INVOKE

#undef SYMCRYPT_AesEcbEncryptXxx


#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM
#define SYMCRYPT_AesEcbEncryptXxx SymCryptAesEcbEncryptAsm

#define SYMCRYPT_XtsAesXxx SymCryptXtsAesEncryptInternalAsm
#define SYMCRYPT_XTSAESDATAUNIT_INVOKE \
    SymCryptXtsAesEncryptDataUnitAsm( &pExpandedKey->key1, &tweakBuf[i], pbSrc, pbDst, cbDataUnit );
#include "xtsaes_pattern.c"
#undef SYMCRYPT_XtsAesXxx
#undef SYMCRYPT_XTSAESDATAUNIT_INVOKE

#define SYMCRYPT_XtsAesXxx SymCryptXtsAesDecryptInternalAsm
#define SYMCRYPT_XTSAESDATAUNIT_INVOKE \
    SymCryptXtsAesDecryptDataUnitAsm( &pExpandedKey->key1, &tweakBuf[i], pbSrc, pbDst, cbDataUnit );
#include "xtsaes_pattern.c"
#undef SYMCRYPT_XtsAesXxx
#undef SYMCRYPT_XTSAESDATAUNIT_INVOKE

#undef SYMCRYPT_AesEcbEncryptXxx
#endif

#if SYMCRYPT_CPU_ARM64
#define SYMCRYPT_AesEcbEncryptXxx SymCryptAesEcbEncryptNeon

#define SYMCRYPT_XtsAesXxx SymCryptXtsAesEncryptInternalNeon
#define SYMCRYPT_XTSAESDATAUNIT_INVOKE \
    SymCryptXtsAesEncryptDataUnitNeon( &pExpandedKey->key1, &tweakBuf[i], pbSrc, pbDst, cbDataUnit );
#include "xtsaes_pattern.c"
#undef SYMCRYPT_XtsAesXxx
#undef SYMCRYPT_XTSAESDATAUNIT_INVOKE

#define SYMCRYPT_XtsAesXxx SymCryptXtsAesDecryptInternalNeon
#define SYMCRYPT_XTSAESDATAUNIT_INVOKE \
    SymCryptXtsAesDecryptDataUnitNeon( &pExpandedKey->key1, &tweakBuf[i], pbSrc, pbDst, cbDataUnit );
#include "xtsaes_pattern.c"
#undef SYMCRYPT_XtsAesXxx
#undef SYMCRYPT_XTSAESDATAUNIT_INVOKE

#undef SYMCRYPT_AesEcbEncryptXxx
#endif

#undef SYMCRYPT_XTS_AES_LOCALSCRATCH_DEFN


#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

#define SYMCRYPT_XTS_AES_LOCALSCRATCH_DEFN \
    /* Defining localScratch as a buffer of __m128is ensures there is required 16B alignment on x86 */ \
    __m128i localScratch[ N_PARALLEL_TWEAKS + 16 ];
#define SYMCRYPT_AesEcbEncryptXxx SymCryptAesEcbEncryptXmm

#define SYMCRYPT_XtsAesXxx SymCryptXtsAesEncryptInternalXmm
#define SYMCRYPT_XTSAESDATAUNIT_INVOKE \
    SymCryptXtsAesEncryptDataUnitXmm( &pExpandedKey->key1, &tweakBuf[i], (PBYTE) &localScratch[N_PARALLEL_TWEAKS], pbSrc, pbDst, cbDataUnit );
#include "xtsaes_pattern.c"
#undef SYMCRYPT_XtsAesXxx
#undef SYMCRYPT_XTSAESDATAUNIT_INVOKE

#define SYMCRYPT_XtsAesXxx SymCryptXtsAesDecryptInternalXmm
#define SYMCRYPT_XTSAESDATAUNIT_INVOKE \
    SymCryptXtsAesDecryptDataUnitXmm( &pExpandedKey->key1, &tweakBuf[i], (PBYTE) &localScratch[N_PARALLEL_TWEAKS], pbSrc, pbDst, cbDataUnit );
#include "xtsaes_pattern.c"
#undef SYMCRYPT_XtsAesXxx
#undef SYMCRYPT_XTSAESDATAUNIT_INVOKE

#define SYMCRYPT_XtsAesXxx SymCryptXtsAesEncryptInternalYmm
#define SYMCRYPT_XTSAESDATAUNIT_INVOKE \
    SymCryptXtsAesEncryptDataUnitYmm_2048( &pExpandedKey->key1, &tweakBuf[i], (PBYTE) &localScratch[N_PARALLEL_TWEAKS], pbSrc, pbDst, cbDataUnit );
#include "xtsaes_pattern.c"
#undef SYMCRYPT_XtsAesXxx
#undef SYMCRYPT_XTSAESDATAUNIT_INVOKE

#define SYMCRYPT_XtsAesXxx SymCryptXtsAesDecryptInternalYmm
#define SYMCRYPT_XTSAESDATAUNIT_INVOKE \
    SymCryptXtsAesDecryptDataUnitYmm_2048( &pExpandedKey->key1, &tweakBuf[i], (PBYTE) &localScratch[N_PARALLEL_TWEAKS], pbSrc, pbDst, cbDataUnit );
#include "xtsaes_pattern.c"
#undef SYMCRYPT_XtsAesXxx
#undef SYMCRYPT_XTSAESDATAUNIT_INVOKE

#if 0 //do not compile Zmm code for now

#define SYMCRYPT_XtsAesXxx SymCryptXtsAesEncryptInternalZmm
#define SYMCRYPT_XTSAESDATAUNIT_INVOKE \
    SymCryptXtsAesEncryptDataUnitZmm_2048( &pExpandedKey->key1, &tweakBuf[i], (PBYTE) &localScratch[N_PARALLEL_TWEAKS], pbSrc, pbDst, cbDataUnit );
#include "xtsaes_pattern.c"
#undef SYMCRYPT_XtsAesXxx
#undef SYMCRYPT_XTSAESDATAUNIT_INVOKE

#define SYMCRYPT_XtsAesXxx SymCryptXtsAesDecryptInternalZmm
#define SYMCRYPT_XTSAESDATAUNIT_INVOKE \
    SymCryptXtsAesDecryptDataUnitZmm_2048( &pExpandedKey->key1, &tweakBuf[i], (PBYTE) &localScratch[N_PARALLEL_TWEAKS], pbSrc, pbDst, cbDataUnit );
#include "xtsaes_pattern.c"
#undef SYMCRYPT_XtsAesXxx
#undef SYMCRYPT_XTSAESDATAUNIT_INVOKE

#endif

#undef SYMCRYPT_XTS_AES_LOCALSCRATCH_DEFN
#undef SYMCRYPT_AesEcbEncryptXxx

#endif

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptInternal(
    _In_                                    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pExpandedKey,
                                            SIZE_T                          cbDataUnit,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                          pbTweak,
    _In_reads_( cbData )                    PCBYTE                          pbSrc,
    _Out_writes_( cbData )                  PBYTE                           pbDst,
                                            SIZE_T                          cbData,
                                            BOOLEAN                         bOverflow )
{
#if SYMCRYPT_CPU_AMD64
    SYMCRYPT_EXTENDED_SAVE_DATA SaveData;
    /* if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_VAES_512_CODE ) ) {
        SymCryptXtsAesEncryptInternalZmm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
    } else */
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_VAES_256_CODE ) &&
        SymCryptSaveYmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptXtsAesEncryptInternalYmm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
        SymCryptRestoreYmm( &SaveData );
    } else if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) ) {
        SymCryptXtsAesEncryptInternalXmm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
    } else {
        SymCryptXtsAesEncryptInternalAsm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
    }
#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptXtsAesEncryptInternalXmm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptXtsAesEncryptInternalAsm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
    }
#elif SYMCRYPT_CPU_ARM
    SymCryptXtsAesEncryptInternalAsm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES ) )
    {
        SymCryptXtsAesEncryptInternalNeon( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
    } else {
        SymCryptXtsAesEncryptInternalC( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
    }
#else
    SymCryptXtsAesEncryptInternalC( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncrypt(
    _In_                    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pExpandedKey,
                            SIZE_T                          cbDataUnit,
                            UINT64                          tweak,
    _In_reads_( cbData )    PCBYTE                          pbSrc,
    _Out_writes_( cbData )  PBYTE                           pbDst,
                            SIZE_T                          cbData )
{
    SYMCRYPT_ALIGN BYTE fullTweak[SYMCRYPT_AES_BLOCK_SIZE];

    SYMCRYPT_ASSERT( cbData % cbDataUnit == 0 );

    if( cbDataUnit < SYMCRYPT_AES_BLOCK_SIZE )
    {
        // Invalid data unit size
        // Return early to avoid repeated checks deeper in the code
        return;
    }

    SYMCRYPT_STORE_LSBFIRST64(&fullTweak[0], tweak);
    SYMCRYPT_STORE_LSBFIRST64(&fullTweak[8], 0);

    SymCryptXtsAesEncryptInternal( pExpandedKey, cbDataUnit, &fullTweak[0], pbSrc, pbDst, cbData, FALSE );
}

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptWith128bTweak(
    _In_                                    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pExpandedKey,
                                            SIZE_T                          cbDataUnit,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                          pbTweak,
    _In_reads_( cbData )                    PCBYTE                          pbSrc,
    _Out_writes_( cbData )                  PBYTE                           pbDst,
                                            SIZE_T                          cbData )
{
    if( cbDataUnit < SYMCRYPT_AES_BLOCK_SIZE )
    {
        // Invalid data unit size
        // Return early to avoid repeated checks deeper in the code
        return;
    }

    SymCryptXtsAesEncryptInternal( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, TRUE );
}


VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptInternal(
    _In_                                    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pExpandedKey,
                                            SIZE_T                          cbDataUnit,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                          pbTweak,
    _In_reads_( cbData )                    PCBYTE                          pbSrc,
    _Out_writes_( cbData )                  PBYTE                           pbDst,
                                            SIZE_T                          cbData,
                                            BOOLEAN                         bOverflow )
{
#if SYMCRYPT_CPU_AMD64
    SYMCRYPT_EXTENDED_SAVE_DATA SaveData;
    /* if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_VAES_512_CODE ) ) {
        SymCryptXtsAesDecryptInternalZmm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
    } else */
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_VAES_256_CODE ) &&
        SymCryptSaveYmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptXtsAesDecryptInternalYmm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
        SymCryptRestoreYmm( &SaveData );
    } else if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) ) {
        SymCryptXtsAesDecryptInternalXmm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
    } else {
        SymCryptXtsAesDecryptInternalAsm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
    }
#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptXtsAesDecryptInternalXmm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptXtsAesDecryptInternalAsm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
    }
#elif SYMCRYPT_CPU_ARM
    SymCryptXtsAesDecryptInternalAsm( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES ) )
    {
        SymCryptXtsAesDecryptInternalNeon( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
    } else {
        SymCryptXtsAesDecryptInternalC( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
    }
#else
    SymCryptXtsAesDecryptInternalC( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, bOverflow );
#endif
}

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecrypt(
    _In_                    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pExpandedKey,
                            SIZE_T                          cbDataUnit,
                            UINT64                          tweak,
    _In_reads_( cbData )    PCBYTE                          pbSrc,
    _Out_writes_( cbData )  PBYTE                           pbDst,
                            SIZE_T                          cbData )
{
    SYMCRYPT_ALIGN BYTE fullTweak[SYMCRYPT_AES_BLOCK_SIZE];

    SYMCRYPT_ASSERT( cbData % cbDataUnit == 0 );

    if( cbDataUnit < SYMCRYPT_AES_BLOCK_SIZE )
    {
        // Invalid data unit size
        // Return early to avoid repeated checks deeper in the code
        return;
    }

    SYMCRYPT_STORE_LSBFIRST64(&fullTweak[0], tweak);
    SYMCRYPT_STORE_LSBFIRST64(&fullTweak[8], 0);

    SymCryptXtsAesDecryptInternal( pExpandedKey, cbDataUnit, &fullTweak[0], pbSrc, pbDst, cbData, FALSE );
}

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptWith128bTweak(
    _In_                                    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pExpandedKey,
                                            SIZE_T                          cbDataUnit,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                          pbTweak,
    _In_reads_( cbData )                    PCBYTE                          pbSrc,
    _Out_writes_( cbData )                  PBYTE                           pbDst,
                                            SIZE_T                          cbData )
{
    if( cbDataUnit < SYMCRYPT_AES_BLOCK_SIZE )
    {
        // Invalid data unit size
        // Return early to avoid repeated checks deeper in the code
        return;
    }

    SymCryptXtsAesDecryptInternal( pExpandedKey, cbDataUnit, pbTweak, pbSrc, pbDst, cbData, TRUE );
}

VOID
SYMCRYPT_CALL
SymCryptXtsUpdateTweak(
    _Inout_updates_(SYMCRYPT_AES_BLOCK_SIZE)    PBYTE   buf )
{
/*
    UINT32 b0 = LOAD_LSBFIRST32( buf      );
    UINT32 b1 = LOAD_LSBFIRST32( buf +  4 );
    UINT32 b2 = LOAD_LSBFIRST32( buf +  8 );
    UINT32 b3 = LOAD_LSBFIRST32( buf + 12 );
    UINT32 msbit = b3 >> 31;

    //
    // The STORE_* macros re-evaluate their arguments sometimes, so we
    // keep all computations in local variables.
    //
    UINT32 r0 = (b0 << 1) ^ (135 * msbit);
    UINT32 r1 = (b1 << 1) | (b0 >> 31);
    UINT32 r2 = (b2 << 1) | (b1 >> 31);
    UINT32 r3 = (b3 << 1) | (b2 >> 31);

    STORE_LSBFIRST32( buf     , r0 );
    STORE_LSBFIRST32( buf +  4, r1 );
    STORE_LSBFIRST32( buf +  8, r2 );
    STORE_LSBFIRST32( buf + 12, r3 );
*/
    UINT64 b0 = SYMCRYPT_LOAD_LSBFIRST64( buf     );
    UINT64 b1 = SYMCRYPT_LOAD_LSBFIRST64( buf + 8 );

    /*
    UINT32 msbit = (UINT32)(b1 >> 63);
    //UINT32 feedback = 135 * msbit;
    UINT32 feedback = (msbit << 7) + (msbit << 3) - msbit;
    */
    UINT32 feedback = (((INT64)b1) >> 63) & 135;

    UINT64 r0 = (b0 << 1) ^ feedback;
    UINT64 r1 = (b1 << 1) | (b0 >> 63);

    SYMCRYPT_STORE_LSBFIRST64( buf    , r0 );
    SYMCRYPT_STORE_LSBFIRST64( buf + 8, r1 );
}

VOID
SYMCRYPT_CALL
SymCryptXtsEncryptDataUnit(
    _In_                                        PCSYMCRYPT_BLOCKCIPHER      pBlockCipher,
    _In_                                        PCVOID                      pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    BYTE  buf[2*SYMCRYPT_AES_BLOCK_SIZE];

    while( cbData >= 2*SYMCRYPT_AES_BLOCK_SIZE )
    {
        SymCryptXorBytes( pbTweakBlock, pbSrc, buf, SYMCRYPT_AES_BLOCK_SIZE );
        (*pBlockCipher->encryptFunc)( pExpandedKey, buf, buf );
        SymCryptXorBytes( pbTweakBlock, buf, pbDst, SYMCRYPT_AES_BLOCK_SIZE );

        SYMCRYPT_ASSERT( pBlockCipher->blockSize == SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptXtsUpdateTweak( pbTweakBlock );

        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        pbDst += SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= SYMCRYPT_AES_BLOCK_SIZE;
    }

    if( cbData > SYMCRYPT_AES_BLOCK_SIZE )
    {
        // Ciphertext stealing encryption
        //
        //                      +--------------+
        //                      |              |
        //                      |              V
        // +-----------------+  |  +-----+-----------+
        // |      P_m-1      |  |  | P_m |++++CP+++++|
        // +-----------------+  |  +-----+-----------+
        //          |           |           |
        //       enc_m-1        |         enc_m
        //          |           |           |
        //          V           |           V
        // +-----+-----------+  |  +-----------------+
        // | C_m |++++CP+++++|--+  |      C_m-1      |
        // +-----+-----------+     +-----------------+
        //    |                   /
        //    +----------------  /  --+
        //                      /     |
        //                      |     V
        // +-----------------+  |  +-----+
        // |      C_m-1      |<-+  | C_m |
        // +-----------------+     +-----+

        // Encrypt penultimate plaintext block into buf
        SymCryptXorBytes( pbTweakBlock, pbSrc, buf, SYMCRYPT_AES_BLOCK_SIZE );
        (*pBlockCipher->encryptFunc)( pExpandedKey, buf, buf );
        SymCryptXorBytes( pbTweakBlock, buf, buf, SYMCRYPT_AES_BLOCK_SIZE );

        cbData -= SYMCRYPT_AES_BLOCK_SIZE;

        // Copy buf to buf[SYMCRYPT_AES_BLOCK_SIZE]
        memcpy( &buf[SYMCRYPT_AES_BLOCK_SIZE], buf, SYMCRYPT_AES_BLOCK_SIZE );
        // Copy final plaintext bytes to prefix of buf - we must read before writing to support in-place encryption
        memcpy( buf, pbSrc + SYMCRYPT_AES_BLOCK_SIZE, cbData );
        // Copy prefix of buf[SYMCRYPT_AES_BLOCK_SIZE] to the right place in the destination buffer
        memcpy( pbDst + SYMCRYPT_AES_BLOCK_SIZE, &buf[SYMCRYPT_AES_BLOCK_SIZE], cbData );

        // Do final tweak update
        SymCryptXtsUpdateTweak( pbTweakBlock );

        // Set pbSrc correctly to share code with non-ciphertext stealing case
        pbSrc = &buf[0];
    }

    // Final full block encryption
    SymCryptXorBytes( pbTweakBlock, pbSrc, buf, SYMCRYPT_AES_BLOCK_SIZE );
    (*pBlockCipher->encryptFunc)( pExpandedKey, buf, buf );
    SymCryptXorBytes( pbTweakBlock, buf, pbDst, SYMCRYPT_AES_BLOCK_SIZE );

    SymCryptWipeKnownSize( buf, sizeof(buf) );
}

VOID
SYMCRYPT_CALL
SymCryptXtsDecryptDataUnit(
    _In_                                        PCSYMCRYPT_BLOCKCIPHER      pBlockCipher,
    _In_                                        PCVOID                      pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    BYTE buf[2*SYMCRYPT_AES_BLOCK_SIZE];
    BYTE tweakBuf[SYMCRYPT_AES_BLOCK_SIZE];

    while( cbData >= 2*SYMCRYPT_AES_BLOCK_SIZE )
    {
        SymCryptXorBytes( pbTweakBlock, pbSrc, buf, SYMCRYPT_AES_BLOCK_SIZE );
        (*pBlockCipher->decryptFunc)( pExpandedKey, buf, buf );
        SymCryptXorBytes( pbTweakBlock, buf, pbDst, SYMCRYPT_AES_BLOCK_SIZE );

        SYMCRYPT_ASSERT( pBlockCipher->blockSize == SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptXtsUpdateTweak( pbTweakBlock );

        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        pbDst += SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= SYMCRYPT_AES_BLOCK_SIZE;
    }

    if( cbData > SYMCRYPT_AES_BLOCK_SIZE )
    {
        // Ciphertext stealing decryption
        //
        //                      +--------------+
        //                      |              |
        //                      |              V
        // +-----------------+  |  +-----+-----------+
        // |      C_m-1      |  |  | C_m |++++CP+++++|
        // +-----------------+  |  +-----+-----------+
        //          |           |           |
        //        dec_m         |        dec_m-1
        //          |           |           |
        //          V           |           V
        // +-----+-----------+  |  +-----------------+
        // | P_m |++++CP+++++|--+  |      P_m-1      |
        // +-----+-----------+     +-----------------+
        //    |                   /
        //    +----------------  /  --+
        //                      /     |
        //                      |     V
        // +-----------------+  |  +-----+
        // |      P_m-1      |<-+  | P_m |
        // +-----------------+     +-----+

        // Save penultimate value of tweak to tweakBuf
        memcpy( tweakBuf, pbTweakBlock, SYMCRYPT_AES_BLOCK_SIZE );

        // Do final tweak update
        SymCryptXtsUpdateTweak( pbTweakBlock );

        // Decrypt penultimate ciphertext block into buf
        SymCryptXorBytes( pbTweakBlock, pbSrc, buf, SYMCRYPT_AES_BLOCK_SIZE );
        (*pBlockCipher->decryptFunc)( pExpandedKey, buf, buf );
        SymCryptXorBytes( pbTweakBlock, buf, buf, SYMCRYPT_AES_BLOCK_SIZE );

        cbData -= SYMCRYPT_AES_BLOCK_SIZE;

        // Copy buf to buf[SYMCRYPT_AES_BLOCK_SIZE]
        memcpy( &buf[SYMCRYPT_AES_BLOCK_SIZE], buf, SYMCRYPT_AES_BLOCK_SIZE );
        // Copy final ciphertext bytes to prefix of buf - we must read before writing to support in-place decryption
        memcpy( buf, pbSrc + SYMCRYPT_AES_BLOCK_SIZE, cbData );
        // Copy prefix of buf[SYMCRYPT_AES_BLOCK_SIZE] to the right place in the destination buffer
        memcpy( pbDst + SYMCRYPT_AES_BLOCK_SIZE, &buf[SYMCRYPT_AES_BLOCK_SIZE], cbData );

        // Set pbSrc and pbTweakBlock correctly to share code with non-ciphertext stealing case
        pbSrc = &buf[0];
        pbTweakBlock = &tweakBuf[0];
    }

    // Final full block decryption
    SymCryptXorBytes( pbTweakBlock, pbSrc, buf, SYMCRYPT_AES_BLOCK_SIZE );
    (*pBlockCipher->decryptFunc)( pExpandedKey, buf, buf );
    SymCryptXorBytes( pbTweakBlock, buf, pbDst, SYMCRYPT_AES_BLOCK_SIZE );

    SymCryptWipeKnownSize( buf, sizeof(buf) );
    SymCryptWipeKnownSize( tweakBuf, sizeof(tweakBuf) );
}

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptDataUnitAsm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    SYMCRYPT_ASSERT( SymCryptAesBlockCipherNoOpt.blockSize == SYMCRYPT_AES_BLOCK_SIZE ); // keep Prefast happy
    SymCryptXtsEncryptDataUnit(
            &SymCryptAesBlockCipherNoOpt,
            pExpandedKey,
            pbTweakBlock,
            pbSrc,
            pbDst,
            cbData );
}

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptDataUnitAsm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    SYMCRYPT_ASSERT( SymCryptAesBlockCipherNoOpt.blockSize == SYMCRYPT_AES_BLOCK_SIZE ); // keep Prefast happy
    SymCryptXtsDecryptDataUnit(
            &SymCryptAesBlockCipherNoOpt,
            pExpandedKey,
            pbTweakBlock,
            pbSrc,
            pbDst,
            cbData );
}

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptDataUnitC(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    // No special optimizations...
    SYMCRYPT_ASSERT( SymCryptAesBlockCipherNoOpt.blockSize == SYMCRYPT_AES_BLOCK_SIZE ); // keep Prefast happy
    SymCryptXtsEncryptDataUnit(
        &SymCryptAesBlockCipherNoOpt,
        pExpandedKey,
        pbTweakBlock,
        pbSrc,
        pbDst,
        cbData );
}

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptDataUnitC(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    SYMCRYPT_ASSERT( SymCryptAesBlockCipherNoOpt.blockSize == SYMCRYPT_AES_BLOCK_SIZE ); // keep Prefast happy
    SymCryptXtsDecryptDataUnit(
        &SymCryptAesBlockCipherNoOpt,
        pExpandedKey,
        pbTweakBlock,
        pbSrc,
        pbDst,
        cbData );

}

static const BYTE SymCryptXtsAesCiphertext[32] = {
    0xef, 0xe5, 0x8b, 0x1a, 0x0b, 0xaf, 0xc1, 0x08,
    0xe9, 0xb7, 0x74, 0x1c, 0xcb, 0xdc, 0xf8, 0x53,
    0x4f, 0x90, 0x55, 0x32, 0x53, 0xf6, 0x18, 0xd2,
    0x34, 0xd5, 0xf2, 0x29, 0xf6, 0x4f, 0xd3, 0x8c,
};

VOID
SYMCRYPT_CALL
SymCryptXtsAesSelftest(void)
{
    SYMCRYPT_XTS_AES_EXPANDED_KEY key;
    BYTE buf[32];
    BYTE plaintext[sizeof( buf )];

    SymCryptWipeKnownSize( buf, sizeof( buf ) );
    buf[0] = 1;

    if( SymCryptXtsAesExpandKeyEx( &key, buf, sizeof( buf ), 0 ) != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'xtsa' );
    }

    SymCryptXtsAesEncrypt( &key, sizeof( buf ), 0, buf, buf, sizeof( buf ) );

    SymCryptInjectError( buf, sizeof( buf ) );
    if( memcmp( buf, SymCryptXtsAesCiphertext, sizeof( buf ) ) != 0 )
    {
        SymCryptFatal( 'xtsa' );
    }

    SymCryptXtsAesDecrypt( &key, sizeof( buf ), 0, buf, buf, sizeof( buf ) );

    SymCryptInjectError( buf, sizeof( buf ) );

    SymCryptWipeKnownSize( plaintext, sizeof( plaintext ) );
    plaintext[0] = 1;
    if( memcmp( buf, plaintext, sizeof( buf ) ) != 0 )
    {
        SymCryptFatal( 'xtsa' );
    }
}
