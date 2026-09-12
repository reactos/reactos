//
// aes-c.c   code for AES implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// The fast-ish C implementation of the core AES functions
//
// Separate C file because at some point we want to be able to switch this out with a compact-C implementation
// that is smaller.
//

#include "precomp.h"

//
// Static vs. dynamically generated tables.
//
// AES uses about 13 kB of tables; it turns out that most of these tables can be generated
// algorithmically much faster than they can be read off the disk.
// This implementation does not do so.
// The reason is that generated tables live in the modifyable data segment, which means
// that they are not shared between different instances of a DLL.
// Static tables are shared. Especially for applications that have a very large number
// of processes (e.g. Terminal Servers) the extra cost of generating and storing a
// per-process copy of these tables is higher then the cost of loading it a few times
// from disk.
// Earlier versions of this implementation did generate the tables dynamically and ran into
// this very problem.
//
// Our tables are aligned to eliminate side-channels from TLB lookups if the TLB page size
// is big enough. For example, the SboxMatrixMult table is 1024-aligned. Each use of that
// table consists of 4 lookups, and each lookup is within its own 1kB aligned subtable.
// The side-channels from cache lines still remains, of course.
//

//extern BYTE SymCryptAesSbox[256];                   // Basic S-box, not used
extern SYMCRYPT_ALIGN_AT( 256) BYTE SymCryptAesInvSbox[256];                // For final round in decryption
extern SYMCRYPT_ALIGN_AT(1024) BYTE SymCryptAesSboxMatrixMult[4][256][4];   // Main encryption tables
extern SYMCRYPT_ALIGN_AT(1024) BYTE SymCryptAesInvSboxMatrixMult[4][256][4];// Main decryption tables
extern SYMCRYPT_ALIGN_AT(1024) BYTE SymCryptAesInvMatrixMult[4][256][4];    // For computing decryption round keys

//
// Throughout this implementation we use UINT32s to access byte arrays. The AES
// algorithm almost requires this; without it the performance would be abysmal.
// All data elements are SYMCRYPT_ALIGNed, which must be at least 4.
//

//
// Macro to check for alignment to support platforms that need alignment fix-ups.
//
#define IS_UINT32_ALIGNED( __p )   ((((intptr_t)__p) & 3) == 0)

//
// Only need to enforce alignment on platforms that are not x86 or x64
// Future improvement: should switch to using unaligned pointer accesses
// on some platforms.
//
#define NEED_ALIGN (!(SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64))


VOID
SYMCRYPT_CALL
SymCryptAes4SboxC(
    _In_reads_(4)   PCBYTE  pIn,
    _Out_writes_(4) PBYTE   pOut )
//
// Perform 4 S-box lookups.
// This is a separate function as it can be done side-channel safe using
// AES-NI.
// Key expansion can actually be improved a lot more with AES-NI, but that
// requires major code changes for which we don't have time right now.
//
{
    pOut[0] = SymCryptAesSboxMatrixMult[0][pIn[0]][1];
    pOut[1] = SymCryptAesSboxMatrixMult[0][pIn[1]][1];
    pOut[2] = SymCryptAesSboxMatrixMult[0][pIn[2]][1];
    pOut[3] = SymCryptAesSboxMatrixMult[0][pIn[3]][1];
}

VOID
SYMCRYPT_CALL
SymCryptAesCreateDecryptionRoundKeyC(
    _In_reads_(16)     PCBYTE  pEncryptionRoundKey,
    _Out_writes_(16)    PBYTE   pDecryptionRoundKey )
//
// Convert an encryption round key to a decryption round key by applying the inverse
// mixcolumn function to each 4-byte subword.
// This is a separate function as with AES-NI there is an assembler version of this
// function that is side-channel safe.
//
{
    int i;
    PBYTE p = pDecryptionRoundKey;
    PCBYTE q = pEncryptionRoundKey;

    for( i=0; i<4; i++ ) {
        *(UINT32 *)p =
            *(UINT32 *)SymCryptAesInvMatrixMult[0][q[0]] ^
            *(UINT32 *)SymCryptAesInvMatrixMult[1][q[1]] ^
            *(UINT32 *)SymCryptAesInvMatrixMult[2][q[2]] ^
            *(UINT32 *)SymCryptAesInvMatrixMult[3][q[3]];
        p += 4;
        q += 4;
    }

}

//
// SymCryptAesEncrypt
// NOINLINE prevents the compiler from creating additional implementations
// that have to be FIPS selftested.
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptAesEncryptC(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY  pExpandedKey,
    _In_reads_(SYMCRYPT_AES_BLOCK_SIZE)     PCBYTE      pbPlaintext,
    _Out_writes_(SYMCRYPT_AES_BLOCK_SIZE)   PBYTE       pbCiphertext )
{
    SYMCRYPT_ALIGN BYTE state[4][4] = { 0 };
    SYMCRYPT_ALIGN UINT32 state2[4] = { 0 };

    const BYTE (*keyPtr)[4][4];
    const BYTE (*keyLimit)[4][4];

#if NEED_ALIGN
    SYMCRYPT_ALIGN BYTE   alignBuffer[SYMCRYPT_AES_BLOCK_SIZE];
#endif

#if NEED_ALIGN

    //
    // Callers who don't have their buffers aligned don't care about speed,
    // so we do this in the simplest way.
    //
    if( !(IS_UINT32_ALIGNED( pbPlaintext ) & IS_UINT32_ALIGNED( pbCiphertext )) ) {
        memcpy( alignBuffer, pbPlaintext, SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptAesEncrypt( pExpandedKey, alignBuffer, alignBuffer );
        memcpy( pbCiphertext, alignBuffer, SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptWipeKnownSize( alignBuffer, sizeof( alignBuffer ) );
        return;
    }
#endif

    SYMCRYPT_CHECK_MAGIC( pExpandedKey );

    //
    // From this point on all our data is UINT32 aligned or better on those
    // platforms that have alignment restrictions.
    //

    keyPtr = &pExpandedKey->RoundKey[0];            // First round key
    keyLimit = &pExpandedKey->lastEncRoundKey[0];   // Last round key

    // Initial round (AddRoundKey)
    *((UINT32 *) &state[0][0]) = *(UINT32 *) (*keyPtr)[0] ^ *(UINT32 *) &pbPlaintext[0];
    *((UINT32 *) &state[1][0]) = *(UINT32 *) (*keyPtr)[1] ^ *(UINT32 *) &pbPlaintext[4];
    *((UINT32 *) &state[2][0]) = *(UINT32 *) (*keyPtr)[2] ^ *(UINT32 *) &pbPlaintext[8];
    *((UINT32 *) &state[3][0]) = *(UINT32 *) (*keyPtr)[3] ^ *(UINT32 *) &pbPlaintext[12];

    keyPtr += 1;

    // Main rounds
    while (keyPtr < keyLimit)
    {

        // SubBytes/ShiftRows/MixColumns for col. 0
        state2[0] = *((UINT32 *) &SymCryptAesSboxMatrixMult[0][ state[0][0] ]);
        state2[3] = *((UINT32 *) &SymCryptAesSboxMatrixMult[1][ state[0][1] ]);
        state2[2] = *((UINT32 *) &SymCryptAesSboxMatrixMult[2][ state[0][2] ]);
        state2[1] = *((UINT32 *) &SymCryptAesSboxMatrixMult[3][ state[0][3] ]);

        // SubBytes/ShiftRows/MixColumns for col. 1
        state2[1] ^= *((UINT32 *) &SymCryptAesSboxMatrixMult[0][ state[1][0] ]);
        state2[0] ^= *((UINT32 *) &SymCryptAesSboxMatrixMult[1][ state[1][1] ]);
        state2[3] ^= *((UINT32 *) &SymCryptAesSboxMatrixMult[2][ state[1][2] ]);
        state2[2] ^= *((UINT32 *) &SymCryptAesSboxMatrixMult[3][ state[1][3] ]);

        // SubBytes/ShiftRows/MixColumns for col. 2
        state2[2] ^= *((UINT32 *) &SymCryptAesSboxMatrixMult[0][ state[2][0] ]);
        state2[1] ^= *((UINT32 *) &SymCryptAesSboxMatrixMult[1][ state[2][1] ]);
        state2[0] ^= *((UINT32 *) &SymCryptAesSboxMatrixMult[2][ state[2][2] ]);
        state2[3] ^= *((UINT32 *) &SymCryptAesSboxMatrixMult[3][ state[2][3] ]);

        // SubBytes/ShiftRows/MixColumns for col. 3
        state2[3] ^= *((UINT32 *) &SymCryptAesSboxMatrixMult[0][ state[3][0] ]);
        state2[2] ^= *((UINT32 *) &SymCryptAesSboxMatrixMult[1][ state[3][1] ]);
        state2[1] ^= *((UINT32 *) &SymCryptAesSboxMatrixMult[2][ state[3][2] ]);
        state2[0] ^= *((UINT32 *) &SymCryptAesSboxMatrixMult[3][ state[3][3] ]);

        // AddRoundKey
        *((UINT32 *) &state[0][0]) = *(UINT32 *) (*keyPtr)[0] ^ state2[0];
        *((UINT32 *) &state[1][0]) = *(UINT32 *) (*keyPtr)[1] ^ state2[1];
        *((UINT32 *) &state[2][0]) = *(UINT32 *) (*keyPtr)[2] ^ state2[2];
        *((UINT32 *) &state[3][0]) = *(UINT32 *) (*keyPtr)[3] ^ state2[3];

        keyPtr += 1;
    }

    // Final round

    // SubBytes/ShiftRows for col. 0
    state2[0] = (UINT32) SymCryptAesSboxMatrixMult[0][ state[0][0] ][1];
    state2[3] = (UINT32) SymCryptAesSboxMatrixMult[0][ state[0][1] ][1] << 8;
    state2[2] = (UINT32) SymCryptAesSboxMatrixMult[0][ state[0][2] ][1] << 16;
    state2[1] = (UINT32) SymCryptAesSboxMatrixMult[0][ state[0][3] ][1] << 24;

    // SubBytes/ShiftRows for col. 1
    state2[1] |= (UINT32) SymCryptAesSboxMatrixMult[0][ state[1][0] ][1];
    state2[0] |= (UINT32) SymCryptAesSboxMatrixMult[0][ state[1][1] ][1] << 8;
    state2[3] |= (UINT32) SymCryptAesSboxMatrixMult[0][ state[1][2] ][1] << 16;
    state2[2] |= (UINT32) SymCryptAesSboxMatrixMult[0][ state[1][3] ][1] << 24;

    // SubBytes/ShiftRows for col. 2
    state2[2] |= (UINT32) SymCryptAesSboxMatrixMult[0][ state[2][0] ][1];
    state2[1] |= (UINT32) SymCryptAesSboxMatrixMult[0][ state[2][1] ][1] << 8;
    state2[0] |= (UINT32) SymCryptAesSboxMatrixMult[0][ state[2][2] ][1] << 16;
    state2[3] |= (UINT32) SymCryptAesSboxMatrixMult[0][ state[2][3] ][1] << 24;

    // SubBytes/ShiftRows for col. 3
    state2[3] |= (UINT32) SymCryptAesSboxMatrixMult[0][ state[3][0] ][1];
    state2[2] |= (UINT32) SymCryptAesSboxMatrixMult[0][ state[3][1] ][1] << 8;
    state2[1] |= (UINT32) SymCryptAesSboxMatrixMult[0][ state[3][2] ][1] << 16;
    state2[0] |= (UINT32) SymCryptAesSboxMatrixMult[0][ state[3][3] ][1] << 24;

    // AddRoundKey
    *((UINT32 *) &pbCiphertext[0 ]) = *(UINT32 *) (*keyPtr)[0] ^ state2[0];
    *((UINT32 *) &pbCiphertext[4 ]) = *(UINT32 *) (*keyPtr)[1] ^ state2[1];
    *((UINT32 *) &pbCiphertext[8 ]) = *(UINT32 *) (*keyPtr)[2] ^ state2[2];
    *((UINT32 *) &pbCiphertext[12]) = *(UINT32 *) (*keyPtr)[3] ^ state2[3];

    SymCryptWipeKnownSize( state, sizeof( state ) );
    SymCryptWipeKnownSize( state2, sizeof( state2 ) );

    return;
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptAesDecryptC(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_(SYMCRYPT_AES_BLOCK_SIZE)     PCBYTE                      pbCiphertext,
    _Out_writes_(SYMCRYPT_AES_BLOCK_SIZE)   PBYTE                       pbPlaintext )
{
    SYMCRYPT_ALIGN BYTE state[4][4] = { 0 };
    SYMCRYPT_ALIGN UINT32 state2[4] = { 0 };

    const BYTE (*keyPtr)[4][4];
    const BYTE (*keyLimit)[4][4];

#if NEED_ALIGN
    SYMCRYPT_ALIGN BYTE   alignBuffer[SYMCRYPT_AES_BLOCK_SIZE];
#endif

#if NEED_ALIGN
    //
    // Callers who don't have their buffers aligned don't care about speed,
    // so we do this in the simplest way.
    //
    if( !(IS_UINT32_ALIGNED( pbPlaintext ) & IS_UINT32_ALIGNED( pbCiphertext )) ) {
        memcpy( alignBuffer, pbCiphertext, SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptAesDecrypt( pExpandedKey, alignBuffer, alignBuffer );
        memcpy( pbPlaintext, alignBuffer, SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptWipeKnownSize( alignBuffer, sizeof( alignBuffer ) );
        return;
    }
#endif

    SYMCRYPT_CHECK_MAGIC( pExpandedKey );

    keyPtr = &pExpandedKey->lastEncRoundKey[0];     // First round key
    keyLimit = &pExpandedKey->lastDecRoundKey[0];   // Last round key

    // Initial round (AddRoundKey)
    *((UINT32 *) &state[0][0]) = *(UINT32 *) (*keyPtr)[0] ^ *(UINT32 *) &pbCiphertext[0];
    *((UINT32 *) &state[1][0]) = *(UINT32 *) (*keyPtr)[1] ^ *(UINT32 *) &pbCiphertext[4];
    *((UINT32 *) &state[2][0]) = *(UINT32 *) (*keyPtr)[2] ^ *(UINT32 *) &pbCiphertext[8];
    *((UINT32 *) &state[3][0]) = *(UINT32 *) (*keyPtr)[3] ^ *(UINT32 *) &pbCiphertext[12];

    keyPtr += 1;

    // Main rounds
    while (keyPtr < keyLimit)
    {

        // SubBytes/ShiftRows/MixColumns for col. 0
        state2[0] = *((UINT32 *) &SymCryptAesInvSboxMatrixMult[0][ state[0][0] ]);
        state2[1] = *((UINT32 *) &SymCryptAesInvSboxMatrixMult[1][ state[0][1] ]);
        state2[2] = *((UINT32 *) &SymCryptAesInvSboxMatrixMult[2][ state[0][2] ]);
        state2[3] = *((UINT32 *) &SymCryptAesInvSboxMatrixMult[3][ state[0][3] ]);

        // SubBytes/ShiftRows/MixColumns for col. 1
        state2[1] ^= *((UINT32 *) &SymCryptAesInvSboxMatrixMult[0][ state[1][0] ]);
        state2[2] ^= *((UINT32 *) &SymCryptAesInvSboxMatrixMult[1][ state[1][1] ]);
        state2[3] ^= *((UINT32 *) &SymCryptAesInvSboxMatrixMult[2][ state[1][2] ]);
        state2[0] ^= *((UINT32 *) &SymCryptAesInvSboxMatrixMult[3][ state[1][3] ]);

        // SubBytes/ShiftRows/MixColumns for col. 2
        state2[2] ^= *((UINT32 *) &SymCryptAesInvSboxMatrixMult[0][ state[2][0] ]);
        state2[3] ^= *((UINT32 *) &SymCryptAesInvSboxMatrixMult[1][ state[2][1] ]);
        state2[0] ^= *((UINT32 *) &SymCryptAesInvSboxMatrixMult[2][ state[2][2] ]);
        state2[1] ^= *((UINT32 *) &SymCryptAesInvSboxMatrixMult[3][ state[2][3] ]);

        // SubBytes/ShiftRows/MixColumns for col. 3
        state2[3] ^= *((UINT32 *) &SymCryptAesInvSboxMatrixMult[0][ state[3][0] ]);
        state2[0] ^= *((UINT32 *) &SymCryptAesInvSboxMatrixMult[1][ state[3][1] ]);
        state2[1] ^= *((UINT32 *) &SymCryptAesInvSboxMatrixMult[2][ state[3][2] ]);
        state2[2] ^= *((UINT32 *) &SymCryptAesInvSboxMatrixMult[3][ state[3][3] ]);

        // AddRoundKey
        *((UINT32 *) &state[0][0]) = *(UINT32 *) (*keyPtr)[0] ^ state2[0];
        *((UINT32 *) &state[1][0]) = *(UINT32 *) (*keyPtr)[1] ^ state2[1];
        *((UINT32 *) &state[2][0]) = *(UINT32 *) (*keyPtr)[2] ^ state2[2];
        *((UINT32 *) &state[3][0]) = *(UINT32 *) (*keyPtr)[3] ^ state2[3];

        keyPtr += 1;
    }

    // Final round

    // SubBytes/ShiftRows for col. 0
    state2[0] = (UINT32) SymCryptAesInvSbox[ state[0][0] ];
    state2[1] = (UINT32) SymCryptAesInvSbox[ state[0][1] ] << 8;
    state2[2] = (UINT32) SymCryptAesInvSbox[ state[0][2] ] << 16;
    state2[3] = (UINT32) SymCryptAesInvSbox[ state[0][3] ] << 24;

    // SubBytes/ShiftRows for col. 1
    state2[1] |= (UINT32) SymCryptAesInvSbox[ state[1][0] ];
    state2[2] |= (UINT32) SymCryptAesInvSbox[ state[1][1] ] << 8;
    state2[3] |= (UINT32) SymCryptAesInvSbox[ state[1][2] ] << 16;
    state2[0] |= (UINT32) SymCryptAesInvSbox[ state[1][3] ] << 24;

    // SubBytes/ShiftRows for col. 2
    state2[2] |= (UINT32) SymCryptAesInvSbox[ state[2][0] ];
    state2[3] |= (UINT32) SymCryptAesInvSbox[ state[2][1] ] << 8;
    state2[0] |= (UINT32) SymCryptAesInvSbox[ state[2][2] ] << 16;
    state2[1] |= (UINT32) SymCryptAesInvSbox[ state[2][3] ] << 24;

    // SubBytes/ShiftRows for col. 3
    state2[3] |= (UINT32) SymCryptAesInvSbox[ state[3][0] ];
    state2[0] |= (UINT32) SymCryptAesInvSbox[ state[3][1] ] << 8;
    state2[1] |= (UINT32) SymCryptAesInvSbox[ state[3][2] ] << 16;
    state2[2] |= (UINT32) SymCryptAesInvSbox[ state[3][3] ] << 24;

    // AddRoundKey
    *((UINT32 *) &pbPlaintext[0 ]) = *(UINT32 *) (*keyPtr)[0] ^ state2[0];
    *((UINT32 *) &pbPlaintext[4 ]) = *(UINT32 *) (*keyPtr)[1] ^ state2[1];
    *((UINT32 *) &pbPlaintext[8 ]) = *(UINT32 *) (*keyPtr)[2] ^ state2[2];
    *((UINT32 *) &pbPlaintext[12]) = *(UINT32 *) (*keyPtr)[3] ^ state2[3];

    SymCryptWipeKnownSize( state, sizeof( state ) );
    SymCryptWipeKnownSize( state2, sizeof( state2 ) );

    return;
}

VOID
SYMCRYPT_CALL
SymCryptAesEcbEncryptC(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    while( cbData >= SYMCRYPT_AES_BLOCK_SIZE )
    {
        SymCryptAesEncryptC( pExpandedKey, pbSrc, pbDst );
        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        pbDst += SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= SYMCRYPT_AES_BLOCK_SIZE;
    }
}

VOID
SYMCRYPT_CALL
SymCryptAesEcbDecryptC(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    while( cbData >= SYMCRYPT_AES_BLOCK_SIZE )
    {
        SymCryptAesDecryptC( pExpandedKey, pbSrc, pbDst );
        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        pbDst += SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= SYMCRYPT_AES_BLOCK_SIZE;
    }
}
