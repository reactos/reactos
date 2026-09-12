//
// aes.c   code for AES implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// The actual encryption and decryption routines here are not nearly as fast as the
// assembler ones. They are used on platforms that don't have assembler implementations
// and for various testing purposes.
//
// This code derives from the orignal fast AES code that Niels Ferguson wrote
// for BitLocker in Windows Vista.
// The C code is derived from the AES that was already in the RSA32 library,
// the assembler code was created new at that time.
//


#include "precomp.h"


///////////////////////////////////////////////////////////////////////////////
// Key expansion uses two functions, a 4-byte S-box lookup and one
// to create a decryption round key from an encryption round key.
// These are the C implementations of these functions
//


static BYTE g_SymCryptAesRoundConstant[11] =
{
    0, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36,
};

SYMCRYPT_NOINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesExpandKeyInternal(
    _Out_               PSYMCRYPT_AES_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                      pbKey,
                        SIZE_T                      cbKey,
                        BOOLEAN                     fCreateDecryptionKeys )
{
    UINT32  nRounds;
    BYTE *  p;
    BYTE *  q;
    UINT32  i;
    UINT32  t;

    BOOL            UseSimd = FALSE;
    SYMCRYPT_ERROR  status = SYMCRYPT_NO_ERROR;

#if SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) )
    {
        if( SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
        {
            UseSimd = TRUE;
        }
    }
#elif SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE ) )
    {
        UseSimd = TRUE;
    }
#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_AES ) )
    {
        UseSimd = TRUE;
    }
#endif

    SYMCRYPT_SET_MAGIC( pExpandedKey );

    //
    // Separate code for each key size, this is significantly faster.
    // We have a number of applications that do frequent key expansions.
    //
    switch( cbKey )
    {
    case 16:
        nRounds = 10;
        pExpandedKey->lastEncRoundKey = &pExpandedKey->RoundKey[nRounds];
        pExpandedKey->lastDecRoundKey = &pExpandedKey->RoundKey[2*nRounds];

        memcpy( &pExpandedKey->RoundKey[0], pbKey, 16 );

        p = (BYTE *)&pExpandedKey->RoundKey[1];

        for( i=1; i<=nRounds; i++ )
        {
            SymCryptAes4Sbox( &p[-4], p, UseSimd );
            t = ROR32(SYMCRYPT_LOAD_LSBFIRST32(p), 8) ^ SYMCRYPT_LOAD_LSBFIRST32(p - 16) ^ g_SymCryptAesRoundConstant[i];
            SYMCRYPT_STORE_LSBFIRST32( p, t );     // this is a macro that re-evaluates its arguments

            *(UINT32 *)(p+4)  = *(UINT32 *) p    ^ *(UINT32 *)(p - 12);
            *(UINT32 *)(p+8)  = *(UINT32 *)(p+4) ^ *(UINT32 *)(p -  8);
            *(UINT32 *)(p+12) = *(UINT32 *)(p+8) ^ *(UINT32 *)(p -  4);

            p += 16;
        }

        break;

    case 24:
        nRounds = 12;
        pExpandedKey->lastEncRoundKey = &pExpandedKey->RoundKey[nRounds];
        pExpandedKey->lastDecRoundKey = &pExpandedKey->RoundKey[2*nRounds];

        memcpy( &pExpandedKey->RoundKey[0], pbKey, 24 );

        p = (BYTE *)&pExpandedKey->RoundKey[0] + 24;

        //
        // We have 12 rounds, 13 round keys, and 13*16 = 208 bytes of encryption key to generate.
        // We have 24 already, so we need 184 more.
        // Each iteration produces 24 bytes, so we need to loop 8 times.
        //
        for( i=1; i<=8; i++ )
        {
            SymCryptAes4Sbox( &p[-4], p, UseSimd );
            t = ROR32(SYMCRYPT_LOAD_LSBFIRST32(p), 8) ^ SYMCRYPT_LOAD_LSBFIRST32(p - 24) ^ g_SymCryptAesRoundConstant[i];
            SYMCRYPT_STORE_LSBFIRST32( p, t );

            *(UINT32 *)(p+4)  = *(UINT32 *) p     ^ *(UINT32 *)(p - 20);
            *(UINT32 *)(p+8)  = *(UINT32 *)(p+ 4) ^ *(UINT32 *)(p - 16);
            *(UINT32 *)(p+12) = *(UINT32 *)(p+ 8) ^ *(UINT32 *)(p - 12);
            *(UINT32 *)(p+16) = *(UINT32 *)(p+12) ^ *(UINT32 *)(p -  8);
            *(UINT32 *)(p+20) = *(UINT32 *)(p+16) ^ *(UINT32 *)(p -  4);

            p += 24;
        }

        break;

    case 32:
        nRounds = 14;
        pExpandedKey->lastEncRoundKey = &pExpandedKey->RoundKey[nRounds];
        pExpandedKey->lastDecRoundKey = &pExpandedKey->RoundKey[2*nRounds];

        memcpy( &pExpandedKey->RoundKey[0], pbKey, 32 );

        p = (BYTE *)&pExpandedKey->RoundKey[0] + 32;

        //
        // We have 14 rounds, 15 round keys, and 15*16 = 240 bytes of encryption key to generate.
        // We have 32 already, so we need 208 more.
        // Each iteration produces 32 bytes, so we need to loop 6.5 times.
        //
        for( i=1; i<=6; i++ )
        {
            SymCryptAes4Sbox( &p[-4], p, UseSimd );
            t = ROR32(SYMCRYPT_LOAD_LSBFIRST32(p), 8) ^ SYMCRYPT_LOAD_LSBFIRST32(p - 32) ^ g_SymCryptAesRoundConstant[i];
            SYMCRYPT_STORE_LSBFIRST32( p, t );

            *(UINT32 *)(p+4)  = *(UINT32 *) p       ^ *(UINT32 *)(p - 28);
            *(UINT32 *)(p+8)  = *(UINT32 *)(p +  4) ^ *(UINT32 *)(p - 24);
            *(UINT32 *)(p+12) = *(UINT32 *)(p +  8) ^ *(UINT32 *)(p - 20);

            SymCryptAes4Sbox( &p[12], &p[16], UseSimd );
            *(UINT32 *)(p+16) = *(UINT32 *)(p + 16) ^ *(UINT32 *)(p - 16);

            *(UINT32 *)(p+20) = *(UINT32 *)(p + 16) ^ *(UINT32 *)(p - 12);
            *(UINT32 *)(p+24) = *(UINT32 *)(p + 20) ^ *(UINT32 *)(p -  8);
            *(UINT32 *)(p+28) = *(UINT32 *)(p + 24) ^ *(UINT32 *)(p -  4);

            p += 32;
        }

        // We looped 6 times, so here is the half-loop

        SymCryptAes4Sbox( &p[-4], p, UseSimd );
        t = ROR32(SYMCRYPT_LOAD_LSBFIRST32(p), 8) ^ SYMCRYPT_LOAD_LSBFIRST32(p - 32) ^ g_SymCryptAesRoundConstant[i];
        SYMCRYPT_STORE_LSBFIRST32( p, t );

        *(UINT32 *)(p+4)  = *(UINT32 *) p       ^ *(UINT32 *)(p - 28);
        *(UINT32 *)(p+8)  = *(UINT32 *)(p +  4) ^ *(UINT32 *)(p - 24);
        *(UINT32 *)(p+12) = *(UINT32 *)(p +  8) ^ *(UINT32 *)(p - 20);

        break;

    default:
        status = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }


    if( fCreateDecryptionKeys )
    {
        p = &pExpandedKey->RoundKey[0][0][0];
        q = (PBYTE)(pExpandedKey->lastDecRoundKey);

        // The first encryption round key is the last decryption round key
        memcpy( q, p, SYMCRYPT_AES_BLOCK_SIZE );
        p += 16;
        q -= 16;

        while( p < (PBYTE) pExpandedKey->lastEncRoundKey )
        {
            SymCryptAesCreateDecryptionRoundKey( p, q, UseSimd );
            q -= 16;
            p += 16;
        }
    }

cleanup:

#if SYMCRYPT_CPU_X86
    if( UseSimd )
    {
        SymCryptRestoreXmm( &SaveData );
    }
#endif

    return status;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesExpandKey(
    _Out_               PSYMCRYPT_AES_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                      pbKey,
                        SIZE_T                      cbKey )

{
    return SymCryptAesExpandKeyInternal( pExpandedKey, pbKey, cbKey, TRUE );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesExpandKeyEncryptOnly(
    _Out_               PSYMCRYPT_AES_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                      pbKey,
                        SIZE_T                      cbKey )
{
    return SymCryptAesExpandKeyInternal( pExpandedKey, pbKey, cbKey, FALSE );
}

VOID
SYMCRYPT_CALL
SymCryptAesKeyCopy( _In_    PCSYMCRYPT_AES_EXPANDED_KEY pSrc,
                    _Out_   PSYMCRYPT_AES_EXPANDED_KEY  pDst )
{
    SYMCRYPT_CHECK_MAGIC( pSrc );

    *pDst = *pSrc;
    pDst->lastEncRoundKey = &pDst->RoundKey[0] + (pSrc->lastEncRoundKey - &pSrc->RoundKey[0]);
    pDst->lastDecRoundKey = &pDst->RoundKey[0] + (pSrc->lastDecRoundKey - &pSrc->RoundKey[0]);

    SYMCRYPT_SET_MAGIC( pDst );
}

//
// Self test code
//


const BYTE SymCryptAesNistTestVector128Ciphertext[16] = {
    0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
    0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a,
};



/****************************************************************
 * OLD CODE
 *
 * Old code to generate the AES tables dynamically.
 * Kept for future reference.
 *


//
// Prototype; on some platforms this function is in assembler.
//
VOID
SYMCRYPT_CALL
SymCryptAesCreateRotatedTables( BYTE MatrixMult[4][256][4] );

VOID
SYMCRYPT_CALL
SymCryptAesCreateRotatedTables( _Inout_ BYTE MatrixMult[4][256][4] )
{
    int i,j,k;

    //
    // We do this byte-by-byte, which is easiest.
    // It would be faster to use UINT32 operations,
    // but that is endian-specific, and therefore platform-specific.
    // Endian-agnostic UINT32-based code would be a lot more complicated.
    // All this is extremely easy to do in assembler, which we do on those
    // platforms that have assembler implementations.
    //
    for( j=1; j<4; j++ ) {
        for( i=0; i<256; i++ ) {
            for( k=0; k<4; k++ ) {
                MatrixMult[j][i][k] = MatrixMult[0][i][(k-j)&3];
            }
        }
    }
}



//
// SymCryptAesInitMatrixMultiplyTable
//
// Initialize a matrix multiplication table.
// Each matrix multiplication table consists of 4 tables of 256 entries of 4 bytes each.
// The four tables are rotated copies of each other.
// This function generates the first of those four tables from the init
// value.
//
// After this call:
//    At index i the table contains the four bytes
//        i * init[0], i * init[1], i * init[2], i * init[3]
//    where multiplication is in GF(2^8).
//
// We do not do a GF(2^8) multiplication for each entry, but rather use the
// relationship (a xor b) * init[.] = a * init[.] xor b * init[.]
// And only compute i*init[.] for i = 1,2,4,8,...,128. This can be done
// using repeated multiplication by x in the finite field.
//
// It is safe to call this function on two separate threads for the same table.
// All invocations will write the same data to the table, and within a tread each entry is written
// before it is read. Doing parallel initializations of the same table can be very inefficient
// as multiple cores will be fighting over the cache lines, but the result will be correct.
// We use this property to initialize the tables lazily.
//
static
VOID
SYMCRYPT_CALL
SymCryptAesInitMatrixMultiplyTable( _Out_   SYMCRYPT_ALIGN BYTE MatrixMult[256][4],
                                    _In_    SYMCRYPT_ALIGN BYTE init[4]
                                    )
{
    int i,j;
    SYMCRYPT_ALIGN BYTE initCopy[4];
    UINT32 initCopyAsUint32;

    //
    // We copy the init value so that we can modify it without worrying about multi-threading
    // issues.
    //
    *(UINT32 *)initCopy = *(UINT32 *)init;

    *(UINT32 *)MatrixMult[0] = 0;
    for( i=1; i<256; i<<=1 )
    {
        initCopyAsUint32 = *(UINT32 *)initCopy;
        for( j=0; j<i; j++ )
        {
            *(UINT32 *)MatrixMult[i+j] = *(UINT32 *)MatrixMult[j] ^ initCopyAsUint32;
        }
        for( j=0; j<4; j++ )
        {
            initCopy[j] = MULT_BY_X( initCopy[j] );
        }
    }
}


//
// SymCryptAesInitialize
//
// Initialize the static tables for the AES implementation.
// This function is called by the key expansion function if it finds the
// tables not initialized.
//
// This leads to an interesting case where multiple threads running on multiple
// CPUs run this initialization code at the same time.
// This code is carefully structured to allow that. When global data is written it is
// always with the final value, and we never read uninitialized global data.
// Thus, even if two CPUs run this code at the same time, they will both initialize each
// memory location to the same correct value and the end result will be correct.
// (Performance will suffer due to the fact that cache lines will be bounced back and force
// between the two CPUs, but that is not a significant concern as this code is used only once.)
//
// At the end of the initialization the flag is set to indicate that further
// key expansion invocations do not need to re-run the initialization.
// We use memory barriers to keep this multi-thread safe.
//
static
VOID
SYMCRYPT_CALL
SymCryptAesInitialize(void)
{
    int i,j;
    BYTE S;
    BYTE Stimes2;

    //
    // We force alignment of these arrays as we sometimes treat them as a UINT32
    //
    SYMCRYPT_ALIGN BYTE InvMatrixEntry[4] = {0xe, 0x9, 0xd, 0xb};
    SYMCRYPT_ALIGN BYTE MatrixEntry[4] = {2, 1, 1, 3};
    SYMCRYPT_ALIGN BYTE MatrixScratch[256][4];

    // Generate the forward MDS multiplication table in the scratch space
    SymCryptAesInitMatrixMultiplyTable( MatrixScratch, MatrixEntry );

    // Initialize first table of SymCryptAesInvMatrixMult
    SymCryptAesInitMatrixMultiplyTable( SymCryptAesInvMatrixMult[0], InvMatrixEntry );

    //
    // Build the InvSbox table and the first table of SymCryptAesSboxMatrixMult and
    // SymCryptAesInvSboxMatrixMult
    //
    for( i=0; i<256; i++ ) {
        S = SymCryptAesSbox[i];
        SymCryptAesInvSbox[S] = (BYTE) i;
        *(UINT32 *)SymCryptAesSboxMatrixMult[0][i] = *(UINT32 *)MatrixScratch[S];
        *(UINT32 *)SymCryptAesInvSboxMatrixMult[0][S] = *(UINT32 *)SymCryptAesInvMatrixMult[0][i];
    }

    //
    // Now we generate the byte rotations of the tables
    //
    SymCryptAesCreateRotatedTables( SymCryptAesSboxMatrixMult );
    SymCryptAesCreateRotatedTables( SymCryptAesInvSboxMatrixMult );
    SymCryptAesCreateRotatedTables( SymCryptAesInvMatrixMult );

    //
    // This is a memory barrier. It ensures that all the memory writes we do before the barrier
    // are globally visible to other CPUs before the memory writes we do after the fence.
    // In this particular case, it ensures that every CPU sees the completed tables before
    // it sees the flag as set.
    //
    MemoryBarrier();

    //
    // Set the flag to signal that the tables are initialized.
    //
    SymCryptAesTablesInitialized = TRUE;
}


*/
