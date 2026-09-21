//
// aesCtrDrbg.c   code for SP 800-90 AES-CTR-DRBG implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This code is derived from the implementation already in use in CNG.
//

#include "precomp.h"

#define SYMCRYPT_RNG_AES_KEY_SIZE                   (32)
#define SYMCRYPT_RNG_AES_KEY_AND_V_SIZE             (32 + 16)
#define SYMCRYPT_RNG_AES_MAX_REQUEST_SIZE           (1<<16)
#define SYMCRYPT_RNG_AES_MAX_REQUESTS_PER_RESEED    ((UINT64)1<<48)

VOID
SYMCRYPT_CALL
SymCryptRngAesBcc(
    _In_                                    PSYMCRYPT_AES_EXPANDED_KEY  pKey,
    _In_reads_( cbData )                   PCBYTE                      pcbData,
    _In_                                    SIZE_T                      cbData,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbResult )
{
    //
    //Length of input should always be multiple of the AES block size
    //
    SYMCRYPT_ASSERT(cbData % SYMCRYPT_AES_BLOCK_SIZE == 0);

    SymCryptWipe( pbResult, SYMCRYPT_AES_BLOCK_SIZE );

    SymCryptAesCbcMac( pKey, pbResult, pcbData, cbData );
}


VOID
SYMCRYPT_CALL
SymCryptRngAesDf(
    _In_reads_(cbData)                                 PCBYTE  pcbData,
    _In_                                                SIZE_T  cbData,
    _Out_writes_(SYMCRYPT_RNG_AES_INTERNAL_SEED_SIZE)   PBYTE   pbSeed )
{
    //maximal input length + IV + padding
    SYMCRYPT_ALIGN BYTE         buf[SYMCRYPT_RNG_AES_MAX_SEED_SIZE + 3 * SYMCRYPT_AES_BLOCK_SIZE];
    PBYTE                       pb;
    SIZE_T                      lenIvS;

    SYMCRYPT_ALIGN BYTE         temp[SYMCRYPT_RNG_AES_KEY_AND_V_SIZE];
    SYMCRYPT_AES_EXPANDED_KEY   aesKey;
    PBYTE                       pX;

    SIZE_T                      i;

    C_ASSERT( sizeof( temp ) % SYMCRYPT_AES_BLOCK_SIZE == 0 );

    //
    // See SP800-90 section 10.4.2
    //
    // Our buf contains the following data:
    // - 16 bytes IV
    // - 4 bytes L
    // - 4 bytes N
    // - up to SEEDLEN bytes input data
    // - 1 byte 0x80
    // - zeroes to fill to a multiple of 16
    //

    SYMCRYPT_ASSERT(    cbData >= SYMCRYPT_RNG_AES_MIN_RESEED_SIZE &&
                        cbData <= SYMCRYPT_RNG_AES_MAX_SEED_SIZE );

    //
    // Initialize the entire buf to zero
    //
    SymCryptWipeKnownSize( buf, sizeof( buf ) );

    //
    // build the string S in buf[16...]
    //
    pb = &buf[ SYMCRYPT_AES_BLOCK_SIZE ];

    //
    // Set L; SP800-90 isn't clear, but we'll use MSB first as that is what is used elsewhere.
    //
    SYMCRYPT_STORE_MSBFIRST32( pb, (UINT32) cbData );
    pb += 4;

    //
    // Set N
    //
    SYMCRYPT_STORE_MSBFIRST32( pb, SYMCRYPT_RNG_AES_INTERNAL_SEED_SIZE );
    pb += 4;

    //
    // Set input_string
    //

    memcpy( pb, pcbData, cbData );
    pb += cbData;

    //
    // set padding
    //
    *pb++ = 0x80;

    while( (pb - buf) % SYMCRYPT_AES_BLOCK_SIZE != 0 )
    {
#pragma prefast( suppress: 26015, "Logic why this doesn't overflow the buf[] array is too complicated for prefast" )
        *pb++ = 0;
    }

    lenIvS = pb - buf;      // Length of IV & S together

    //
    // Set up the inital key
    //

    for( i = 0; i < SYMCRYPT_RNG_AES_KEY_SIZE; i++ )
    {
        temp[i] = (BYTE) i;
    }
    SymCryptAesExpandKeyEncryptOnly( &aesKey, temp, SYMCRYPT_RNG_AES_KEY_SIZE );


    //
    // Produce the 'temp' intermediate result.
    //

    for( i=0; i< SYMCRYPT_RNG_AES_KEY_AND_V_SIZE / SYMCRYPT_AES_BLOCK_SIZE; i++ )
    {
        //
        // Update the IV with the right i value.
        // i is only 0-2, so we only have to set a single byte
        //
        buf[3] = (BYTE) i;

        //
        // Now we perform the BCC function, which is just CbcMac
        // BCC(K,(IV||S))
        SymCryptRngAesBcc( &aesKey, buf, lenIvS, &temp[ i * SYMCRYPT_AES_BLOCK_SIZE ] );
     }

    //
    // Second phase, produce the actual output
    //
    SymCryptAesExpandKeyEncryptOnly( &aesKey, temp, SYMCRYPT_RNG_AES_KEY_SIZE );
    pX = &temp[SYMCRYPT_RNG_AES_KEY_SIZE];

    for( i=0; i < SYMCRYPT_RNG_AES_INTERNAL_SEED_SIZE; i += SYMCRYPT_AES_BLOCK_SIZE )
    {
        SymCryptAesEncrypt( &aesKey, pX, pX );
        memcpy( &pbSeed[ i ], pX, SYMCRYPT_AES_BLOCK_SIZE );
    }

    SymCryptWipeKnownSize( buf, sizeof( buf ) );
    SymCryptWipeKnownSize( temp, sizeof( temp ) );
    SymCryptWipeKnownSize( &aesKey, sizeof( aesKey ) );
}


VOID
SYMCRYPT_CALL
SymCryptRngAesGenerateBlocks(
    _In_                                            PSYMCRYPT_AES_EXPANDED_KEY  pAesKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )      PBYTE                       pV,
    _Out_writes_(cbRandom)                          PBYTE                       pbRandom,
    _In_                                            SIZE_T                      cbRandom )
//
// Internal function to generate output blocks from the state.
// cbRandom must be a multiple of the block size.
//
{
    UINT64  v;
    SIZE_T  cBlocks;
    SIZE_T  blocksToDo;
    SIZE_T  bytesToDo;

//
// The roll-over of the counter is hard to test, especially since our
// NIST test vectors only cover small outputs.
// We have an option to test the output against a simpler (older) implementation
// to validate the proper working of the code.
//
#define TEST_AGAINST_OLD_CODE   0
#if TEST_AGAINST_OLD_CODE
    BYTE    Vcopy[16];
    BYTE    buf[16];
    PCBYTE  pbCheck = pbRandom;
    SIZE_T  cbCheck = cbRandom;

    memcpy( Vcopy, pV, 16 );
#endif

    //
    // cbRandom must be a multiple of BLOCK_LEN and > 0.
    //
    SYMCRYPT_ASSERT( (cbRandom & (SYMCRYPT_AES_BLOCK_SIZE-1)) == 0 );

    cBlocks = cbRandom / SYMCRYPT_AES_BLOCK_SIZE;

    //
    // We violate the write-once rule here by wiping the output buffer and then
    // filling it with the CTR-mode encryption.
    // This is safe because the caller only learns the proper output anyway.
    //
    SymCryptWipe( pbRandom, cbRandom );

    //
    // This loop is a little complicated because we need to pre-increment the 128-bit value V
    // and the SymCryptAesCtrMsb64 function does a 64-bit post-increment.
    //
    while( cBlocks != 0 )
    {
        // Increment V
        v = SYMCRYPT_LOAD_MSBFIRST64( &pV[8] ) + 1;
        SYMCRYPT_STORE_MSBFIRST64( &pV[8], v );
        SYMCRYPT_STORE_MSBFIRST64( &pV[0], SYMCRYPT_LOAD_MSBFIRST64( &pV[0] ) + (v == 0) );

        //
        // The SymCryptAesCtrMsb64 routine will increment the last 64 bits of the V value,
        // but not handle the carry to the first 64 bits.
        // We limit how many block we do so that we never cross this boundary.
        // SymCryptAesCtrMsb64 does a post-increment, so it may increment the last 64 bits
        // to zero as long as we don't rely on the V value afterwards.
        // As one-in-2^64 code is not testable, we terminate the Msb64 call earlier, and
        // much earlier on CHKed builds.
        //
#if SYMCRYPT_DEBUG
#define MAX_CTRMSB64_BLOCKS (1 << 3)        // very small; overflow will be triggered by any reasonable test
#else
#define MAX_CTRMSB64_BLOCKS (1 << 10)         // increase when we have this well-tested
#endif
        //
        // 1 + (~v & mask) is the value you can add to v so that the mask bits of the sum
        // end up to be zero. It is in the range 1 .. mask+1
        //
        blocksToDo = SYMCRYPT_MIN( cBlocks, 1 + ( (~v) & (MAX_CTRMSB64_BLOCKS - 1) ) );

        bytesToDo = blocksToDo * SYMCRYPT_AES_BLOCK_SIZE;
        SYMCRYPT_ASSERT( bytesToDo <= cbRandom );
        SymCryptAesCtrMsb64( pAesKey, &pV[0], pbRandom, pbRandom, bytesToDo );
        pbRandom += bytesToDo;
        cbRandom -= bytesToDo;          // only used for prefast assertions; optimized away in shipping code
        cBlocks -= blocksToDo;

        //
        // Post-decrement the V block to compensate for the post-increment of the Msb64 function
        //
        v += blocksToDo - 1;
        SYMCRYPT_ASSERT( v != 0 );

        SYMCRYPT_STORE_MSBFIRST64( &pV[8], v );
        // No need to carry to the first half of V here, it cannot happen
    }

#if TEST_AGAINST_OLD_CODE
    //
    // We tried to use the CtrMsb64 mode to generate the blocks, but that leads to
    // a number of complications.
    // The lack of carry means we end up with code paths that run once per 2^64 blocks
    // or so, and that is very hard to test.
    // Furthermore, CtrMsb64 uses post-increment, whereas AES-CTR_DRBG uses pre-increment.
    // That adds sufficient extra complications and testing problems that we went back
    // to the solution below.
    //

    while( cbCheck != 0 )
    {
        SYMCRYPT_ASSERT( cbCheck >= SYMCRYPT_AES_BLOCK_SIZE );     // Keep prefast happy
        //
        // Increment the 128-bit block V MSByte first.
        //
        v = SYMCRYPT_LOAD_MSBFIRST64( &Vcopy[8] ) + 1;
        SYMCRYPT_STORE_MSBFIRST64( &Vcopy[8], v );
        if( v == 0 )
        {
            //
            // This almost never happens.
            // Using an if() is not side-channel safe, but in this case
            // the side channel does not reveal anything that actually hurts the
            // security of the algorithm.
            //
            SYMCRYPT_STORE_MSBFIRST64( Vcopy, 1 + LOAD_MSBFIRST64( Vcopy ) );
        }

        SymCryptAesEncrypt( pAesKey, Vcopy, buf );
        if( memcmp( buf, pbCheck, 16 ) != 0 )
        {
            SymCryptFatal( 'OLD?' );
        }
        pbCheck += SYMCRYPT_AES_BLOCK_SIZE;
        cbCheck -= SYMCRYPT_AES_BLOCK_SIZE;
    }
#endif
}

FORCEINLINE
int
SymCryptRngAesAreBlocksIdentical(
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE ) PCBYTE  pSrc1,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE ) PCBYTE  pSrc2 )
//
// return 1 if the blocks are identical, 0 if they are different.
//
{
    SYMCRYPT_UNALIGNED const SIZE_T * p1 = (SYMCRYPT_UNALIGNED const SIZE_T *) pSrc1;
    SYMCRYPT_UNALIGNED const SIZE_T * p2 = (SYMCRYPT_UNALIGNED const SIZE_T *) pSrc2;

    SIZE_T tmp;

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM

    C_ASSERT( sizeof( SIZE_T ) == 4 );
    tmp = (p1[0] ^ p2[0]) | (p1[1] ^ p2[1]) | (p1[2] ^ p2[2]) | (p1[3] ^ p2[3]);

#elif SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64

    C_ASSERT( sizeof( SIZE_T ) == 8 );
    tmp = (p1[0] ^ p2[0]) | (p1[1] ^ p2[1]);

#else

    SIZE_T i;

    C_ASSERT( 16 % sizeof( SIZE_T ) == 0 );

    tmp = 0;
    for( i=0; i < 16/sizeof( SIZE_T ); i ++ )
    {
        tmp |= p1[i] ^ p2[i];
    }

#endif

    return tmp == 0;
}


VOID
SYMCRYPT_CALL
SymCryptRngAesCheckBlocksNotIdentical(
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE   pbPreviousBlock,
    _In_reads_( cbData )                        PCBYTE  pcbData,
                                                SIZE_T  cbData )
{
    SIZE_T identical;
    SIZE_T i;

    SYMCRYPT_ASSERT( ((cbData & 15) == 0) && cbData > 0 );

    identical = SymCryptRngAesAreBlocksIdentical( pbPreviousBlock, pcbData );

    for( i = SYMCRYPT_AES_BLOCK_SIZE; i < cbData; i += SYMCRYPT_AES_BLOCK_SIZE )
    {
        SYMCRYPT_ASSERT( cbData >= i + SYMCRYPT_AES_BLOCK_SIZE );
        identical |= SymCryptRngAesAreBlocksIdentical( &pcbData[i-SYMCRYPT_AES_BLOCK_SIZE], &pcbData[ i ] );
    }

    memcpy( pbPreviousBlock, &pcbData[cbData - SYMCRYPT_AES_BLOCK_SIZE], SYMCRYPT_AES_BLOCK_SIZE );

    //
    // The structure of AES-CTR-DRBG makes it impossible for two consecutive blocks of a single request
    // to be equal. The only way this could happen is if the first block of one request is the same as
    // the last block of the previous request. But the probability of this happening is 2^{-128}.
    // This never happens, so the whole check is technically useless.
    // Nevertheless, it is required by FIPS 140-2, so we have to implement it,
    // but we don't have to handle the error usefully in any way.
    // (Trying to handle this error sensibly is far too complicated, and adds far more danger from code
    // bugs than it is worth. It is much better to just treat it as a fatal occurrence.)
    //

    if( identical )
    {
        SymCryptFatal( 'acdi' );
    }
}

VOID
SYMCRYPT_CALL
SymCryptRngAesUpdate(
    _Inout_                                                 PSYMCRYPT_RNG_AES_STATE     pState,
    _In_reads_opt_( SYMCRYPT_RNG_AES_INTERNAL_SEED_SIZE )   PCBYTE                      pbProvidedData,
    _In_opt_                                                PSYMCRYPT_AES_EXPANDED_KEY  pAesKey)
//
// Implement the CTR_DRBG Update function.
// pbProvidedData is optional, but if provided must always be exactly seedlen bits.
// pAesKey is the already expanded key of the RngState. This is optional, and only has
// to be provided if the caller already has it.
//
{
    SYMCRYPT_AES_EXPANDED_KEY   aesKey;
    PSYMCRYPT_AES_EXPANDED_KEY  pKey;
    SYMCRYPT_ALIGN  BYTE        buf[SYMCRYPT_AES_BLOCK_SIZE];

    if(NULL == pAesKey)
    {
        SymCryptAesExpandKeyEncryptOnly( &aesKey, pState->keyAndV, SYMCRYPT_RNG_AES_KEY_SIZE );
        pKey = &aesKey;
    }
    else
    {
        pKey = pAesKey;
    }

    //
    // Copy the V value so that we can overwrite it safely.
    //

    memcpy( buf, &pState->keyAndV[SYMCRYPT_RNG_AES_KEY_SIZE], sizeof( buf ) );

    SymCryptRngAesGenerateBlocks(
            pKey,
            buf,                            // pV
            pState->keyAndV,                // pbRandom
            sizeof( pState->keyAndV) );     // cbRandom

    if( pbProvidedData != NULL )
    {
        // XOR provided data in
        SymCryptXorBytes( pState->keyAndV, pbProvidedData, pState->keyAndV, SYMCRYPT_RNG_AES_INTERNAL_SEED_SIZE );
    }

    SymCryptWipeKnownSize( buf, sizeof( buf ) );

    //
    // Only wipe the key if necessary.
    //
    if( pKey == &aesKey )
    {
        SymCryptWipeKnownSize( &aesKey, sizeof( aesKey ));
    }
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRngAesGenerateSmall(
    _Inout_                             PSYMCRYPT_RNG_AES_STATE pRngState,
    _Out_writes_( cbRandom )            PBYTE                   pbRandom,
                                        SIZE_T                  cbRandom,
    _In_reads_opt_( cbAdditionalInput ) PCBYTE                  pbAdditionalInput,
                                        SIZE_T                  cbAdditionalInput )
//
// This is the Generate function of our SP800-90 compliant implementation.
// It follows the method specified in SP800-90A 10.2.1.5.2
//
{
    SYMCRYPT_AES_EXPANDED_KEY   aesKey;
    SYMCRYPT_ALIGN BYTE         buf[SYMCRYPT_AES_BLOCK_SIZE];
    SYMCRYPT_ALIGN BYTE         abSeed[SYMCRYPT_RNG_AES_INTERNAL_SEED_SIZE];

    //
    // SP 800-90 9.3.1 requires a check on the length of the request.
    //
    if( cbRandom > SYMCRYPT_RNG_AES_MAX_REQUEST_SIZE )
    {
        return SYMCRYPT_WRONG_DATA_SIZE;
    }
    //
    // The requestCounter test is useless as it can never happen. (It would require
    // 2^48 calls to this function to trigger this error.)
    // Unfortunately, SP800-90 section 11 requires a test of this error, so we have
    // to implement the error.
    //
    if( pRngState->requestCounter > SYMCRYPT_RNG_AES_MAX_REQUESTS_PER_RESEED )
    {
        return SYMCRYPT_FIPS_FAILURE;
    }

    if( pbAdditionalInput != NULL )
    {
        // Update additional input using Derivation function
        SymCryptRngAesDf( pbAdditionalInput, cbAdditionalInput, abSeed );
        pbAdditionalInput = &abSeed[0];

        // Update state with modified additional input
        SymCryptRngAesUpdate( pRngState, pbAdditionalInput, NULL );
    }

    SymCryptAesExpandKeyEncryptOnly( &aesKey, pRngState->keyAndV, SYMCRYPT_RNG_AES_KEY_SIZE );

    if( cbRandom >= SYMCRYPT_AES_BLOCK_SIZE )
    {
        SIZE_T wholeBlocks = cbRandom & ~(SYMCRYPT_AES_BLOCK_SIZE - 1);
        SymCryptRngAesGenerateBlocks(   &aesKey,
                                        &pRngState->keyAndV[ SYMCRYPT_RNG_AES_KEY_SIZE],
                                        pbRandom,
                                        wholeBlocks );
        if( pRngState->fips140_2Check )
        {
            SymCryptRngAesCheckBlocksNotIdentical( pRngState->previousBlock, pbRandom, wholeBlocks );
        }
        pbRandom += wholeBlocks;
        cbRandom -= wholeBlocks;
    }

    if( cbRandom > 0 )
    {
        SYMCRYPT_ASSERT( cbRandom < SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptRngAesGenerateBlocks(   &aesKey,
                                        &pRngState->keyAndV[ SYMCRYPT_RNG_AES_KEY_SIZE],
                                        buf,
                                        sizeof( buf ) );
        if( pRngState->fips140_2Check )
        {
            SymCryptRngAesCheckBlocksNotIdentical( pRngState->previousBlock, buf, sizeof( buf ) );
        }

        memcpy( pbRandom, buf, cbRandom );
        SymCryptWipeKnownSize( buf, sizeof( buf ) );
    }

    SymCryptRngAesUpdate( pRngState, pbAdditionalInput, &aesKey );

    ++pRngState->requestCounter;

    SymCryptWipeKnownSize( &aesKey, sizeof( aesKey ) );
    SymCryptWipeKnownSize( abSeed, sizeof( abSeed ) );

    return SYMCRYPT_NO_ERROR;
}


_Use_decl_annotations_
SYMCRYPT_NOINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRngAesInstantiate(  PSYMCRYPT_RNG_AES_STATE pRngState,
                            PCBYTE                  pcbSeedMaterial,
                            SIZE_T                  cbSeedMaterial )
//
// This function creates a new SP 800-90 AES_CTR_DRBG instance.
// Our code is structured differently from what SP 800-90 assumes.
// At this point in time, the entropy has already been collected and it is
// passed to this function. Thus, there is no check for failing to get
// the entropy. If entropy collection fails, the caller of this function
// will generate an error. (Actually, we only choose to instantiate a FIPS-compliant
// SP 800-90 DRBG when we do have good entropy available, so there is never an
// error that we don't have the required entropy.)
//
{
    if( cbSeedMaterial < SYMCRYPT_RNG_AES_MIN_INSTANTIATE_SIZE )
    {
        return SYMCRYPT_EXTERNAL_FAILURE;
    }

    //
    // Instantiation of a new state is identical to setting the state to zero
    // and then performing a reseed with the same seed material.
    //
    // See SP 800-90 10.2.1.3.2 & 10.2.1.4.2
    //
    SymCryptWipeKnownSize( pRngState, sizeof( *pRngState ) );

    SYMCRYPT_SET_MAGIC( pRngState );

    return SymCryptRngAesReseed( pRngState, pcbSeedMaterial, cbSeedMaterial );
}

_Use_decl_annotations_
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptRngAesGenerate( PSYMCRYPT_RNG_AES_STATE pRngState,
                        PBYTE                   pbRandom,
                        SIZE_T                  cbRandom )
//
// For FIPS compliance purposes, this is NOT the generate function of the DRBG.
// The generate function is SymCryptRngAesGenerateSmall.
// This is a wrapper around the generate function that supports larger output
// sizes, and handles any errors by making them fatal.
//
{
    SYMCRYPT_ERROR scError;

    SYMCRYPT_CHECK_MAGIC( pRngState );

    while( cbRandom > SYMCRYPT_RNG_AES_MAX_REQUEST_SIZE )
    {

        scError = SymCryptRngAesGenerateSmall( pRngState, pbRandom, SYMCRYPT_RNG_AES_MAX_REQUEST_SIZE, NULL, 0 );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            SymCryptFatal( 'acdx' );
        }
        pbRandom += SYMCRYPT_RNG_AES_MAX_REQUEST_SIZE;
        cbRandom -= SYMCRYPT_RNG_AES_MAX_REQUEST_SIZE;
    }

    if( cbRandom > 0 )
    {
        scError = SymCryptRngAesGenerateSmall( pRngState, pbRandom, cbRandom, NULL, 0 );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            SymCryptFatal( 'acdx' );
        }
    }
}

_Use_decl_annotations_
SYMCRYPT_NOINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRngAesReseed(   PSYMCRYPT_RNG_AES_STATE pRngState,
                        PCBYTE                  pcbSeedMaterial,
                        SIZE_T                  cbSeedMaterial )
{
    SYMCRYPT_ALIGN BYTE    abSeed[SYMCRYPT_RNG_AES_INTERNAL_SEED_SIZE];

    SYMCRYPT_CHECK_MAGIC( pRngState );

    //
    // For a reseed, the minimum # bits is the security strength, or the key size.
    // We retain the same maximum as that protects our own internal buffers.
    //
    if (cbSeedMaterial < SYMCRYPT_RNG_AES_MIN_RESEED_SIZE ||
        cbSeedMaterial > SYMCRYPT_RNG_AES_MAX_SEED_SIZE )
    {
        return SYMCRYPT_EXTERNAL_FAILURE;     // bug is external to SymCrypt (i.e. the caller)
    }

    //
    // We do not perform the FIPS-required reseed self-test here.
    // Rather, we have a function that external callers can use to implement that test before
    // calling this reseed function.
    // This allows callers that are not interested in FIPS certification to skip the test.
    //

    SymCryptRngAesDf( pcbSeedMaterial, cbSeedMaterial, abSeed );

    SymCryptRngAesUpdate( pRngState, abSeed, NULL );

    pRngState->requestCounter = 1;

    SymCryptWipeKnownSize( abSeed, sizeof( abSeed ) );

    return SYMCRYPT_NO_ERROR;
}

_Use_decl_annotations_
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptRngAesUninstantiate(    PSYMCRYPT_RNG_AES_STATE pRngState )
{
    SymCryptWipeKnownSize( pRngState, sizeof( *pRngState ) );
}

////////////////////////////////////////////////////////////////////////////
// Self test

//
// The test vector is from the NIST DRBG Test Vectors file
//
static const BYTE g_abInstantiateEntropyInputPlusNonce[] =
{
    // Entropy input

    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
    0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,

    // Nonce
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
    0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,

};


static const BYTE g_abReseedEntropy[] =
{

   0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
   0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
   0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
   0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
   0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
   0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF
};

static const BYTE g_abOutput1[ 32 ] =
{
   0xD1,0xE9,0xC7,0x37,0xB6,0xEB,0xAE,0xD7,
   0x65,0xA0,0xD4,0xE4,0xC6,0xEA,0xEB,0xE2,
   0x67,0xF5,0xE9,0x19,0x36,0x80,0xFD,0xFF,
   0xA6,0x2F,0x48,0x65,0xB3,0xF0,0x09,0xEC,
};

static const BYTE g_expectedStateAfterInstantiate[ SYMCRYPT_RNG_AES_KEY_AND_V_SIZE ] =
{
    //key
    0x8C,0x10,0xB6,0x58,0x44,0x0C,0x71,0x35,
    0x64,0x9D,0xC7,0x7B,0xE6,0xE5,0x75,0xCE,
    0x87,0xE7,0x48,0x90,0x83,0x9B,0x89,0x59,
    0x14,0x17,0xAF,0xAD,0x14,0xB2,0x26,0xD5,
    //V
    0xB4,0x03,0x6B,0x1D,0xBA,0x04,0x3A,0xE6,
    0x55,0xAC,0xD6,0x46,0xEC,0x5A,0xD3,0x5C,
};

static const BYTE g_expectedStateAfterReseed[ SYMCRYPT_RNG_AES_KEY_AND_V_SIZE ] =
{
    //key
    0x17,0x98,0xC0,0xDF,0x09,0x69,0x6A,0x46,
    0x19,0x46,0xFE,0x6D,0x68,0x7D,0x8C,0xC8,
    0x3F,0xEE,0xF1,0x22,0xF3,0xBB,0xC5,0xF2,
    0x9D,0xAC,0x85,0x10,0xF3,0x4A,0xF0,0x15,
    //V
    0x0B,0xF3,0x34,0x4D,0xF5,0x29,0x27,0x6B,
    0x0D,0x5B,0xBC,0x83,0x9B,0xD3,0x65,0x6A,
};

static const BYTE g_expectedStateAfterGenerate[ SYMCRYPT_RNG_AES_KEY_AND_V_SIZE ] =
{
    //key
    0x28, 0xbc, 0x65, 0xa8, 0x6a, 0xb7, 0xc7, 0x4e, 0xdf, 0x4b, 0xb8, 0x72, 0x87, 0xd3, 0x4f, 0xbb,
    0x8d, 0x6f, 0x16, 0xd7, 0xb9, 0x1b, 0x6a, 0xbb, 0xee, 0x7b, 0x88, 0x86, 0x5b, 0x0f, 0xc7, 0xbd,
    //V
    0xb7, 0x46, 0x11, 0xf3, 0x92, 0x95, 0xa6, 0x25, 0x7c, 0x39, 0x98, 0x4c, 0x9c, 0x09, 0x9b, 0x30,
};


VOID
SYMCRYPT_CALL
SymCryptRngAesTestInstantiate( PSYMCRYPT_RNG_AES_STATE pRngState )
//
// Test the Instantiate function on the passed instance. Leave it
// in the initialized state for the test vector.
//
{
    SYMCRYPT_ERROR scError;
    //
    // First test the error handling
    //
#pragma prefast( suppress: 26060 6309 28020, "Deliberate test of invalid parameter");
    scError = SymCryptRngAesInstantiate( pRngState, NULL, 327 );
    if( scError == SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'aci1' );
    }


    scError = SymCryptRngAesInstantiate( pRngState,
                                         g_abInstantiateEntropyInputPlusNonce,
                                         sizeof( g_abInstantiateEntropyInputPlusNonce )
                                         );

    SymCryptInjectError( pRngState->keyAndV, SYMCRYPT_RNG_AES_KEY_AND_V_SIZE );

    if ( scError != SYMCRYPT_NO_ERROR ||
         0 != memcmp( pRngState->keyAndV,
                        g_expectedStateAfterInstantiate,
                        SYMCRYPT_RNG_AES_KEY_AND_V_SIZE ))
    {
        SymCryptFatal( 'aci2' );
    }
}

VOID
SYMCRYPT_CALL
SymCryptRngAesTestReseed( PSYMCRYPT_RNG_AES_STATE pRngState )
{
    SYMCRYPT_ERROR scError;

    //
    // Set the state to a known state
    //
    SYMCRYPT_SET_MAGIC( pRngState );
    memcpy( pRngState->keyAndV, g_expectedStateAfterInstantiate, SYMCRYPT_RNG_AES_KEY_AND_V_SIZE );
    pRngState->requestCounter = 7;
    pRngState->fips140_2Check = FALSE;

    //
    // Test error handling
    //
#pragma prefast(suppress: 26060 6309 28020, "Deliberate test of invalid parameter");
    scError = SymCryptRngAesReseed( pRngState, NULL, 597 );
    if( scError == SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'acr1' );
    }

    scError = SymCryptRngAesReseed( pRngState, g_abReseedEntropy, sizeof( g_abReseedEntropy ) );

    SymCryptInjectError( pRngState->keyAndV, SYMCRYPT_RNG_AES_KEY_AND_V_SIZE );

    if ( scError != SYMCRYPT_NO_ERROR ||
         0 != memcmp( pRngState->keyAndV,
                        g_expectedStateAfterReseed,
                        SYMCRYPT_RNG_AES_KEY_AND_V_SIZE ) )
    {
        SymCryptFatal( 'acr2' );
    }
}

VOID
SYMCRYPT_CALL
SymCryptRngAesTestGenerate( PSYMCRYPT_RNG_AES_STATE pRngState )
{
    BYTE            abOutput[2*SYMCRYPT_AES_BLOCK_SIZE];
    SYMCRYPT_ERROR  scError;

    //
    // Set the state to a known value
    //
    SYMCRYPT_SET_MAGIC( pRngState );
    memcpy( pRngState->keyAndV, g_expectedStateAfterReseed, SYMCRYPT_RNG_AES_KEY_AND_V_SIZE );
    pRngState->requestCounter = 7;
    pRngState->fips140_2Check = FALSE;

    //
    // Test the error handling
    // - Too many requests since last reseed
    // - Too many bytes in request
    //

    pRngState->requestCounter = SYMCRYPT_RNG_AES_MAX_REQUESTS_PER_RESEED + 1;
    scError = SymCryptRngAesGenerateSmall( pRngState, abOutput, sizeof( g_abOutput1 ), NULL, 0 );

    if( scError == SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'acg1' );
    }
    pRngState->requestCounter = 7;

#pragma prefast( suppress: 6202 26000, "buffer size of cbOutput is purposely incorrect");
    scError = SymCryptRngAesGenerateSmall( pRngState, abOutput, SYMCRYPT_RNG_AES_MAX_REQUEST_SIZE + 1, NULL, 0 );

    if( scError == SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'acg2' );
    }

    //
    // Now test for correct output data.
    //
    scError = SymCryptRngAesGenerateSmall( pRngState, abOutput, sizeof( g_abOutput1 ), NULL, 0 );

    SymCryptInjectError( abOutput, sizeof( abOutput ) );

    if( scError != SYMCRYPT_NO_ERROR || memcmp( abOutput, g_abOutput1, sizeof( g_abOutput1 ) ) != 0 )
    {
        SymCryptFatal( 'acg3' );
    }

    //
    // And test for the correct resulting state
    //
    SymCryptInjectError( pRngState->keyAndV, SYMCRYPT_RNG_AES_KEY_AND_V_SIZE );

    if ( 0 != memcmp( pRngState->keyAndV,
                        g_expectedStateAfterGenerate,
                        SYMCRYPT_RNG_AES_KEY_AND_V_SIZE ) )
    {
        SymCryptFatal( 'acg4' );
    }
}


VOID
SYMCRYPT_CALL
SymCryptRngAesTestUninstantiate( PSYMCRYPT_RNG_AES_STATE pRngState )
{
    const SIZE_T * p = (const SIZE_T *) pRngState;
    SIZE_T t;
    SIZE_T i;

    C_ASSERT( sizeof( *pRngState ) % sizeof( SIZE_T ) == 0 );   // This is true on all our platforms.

    SYMCRYPT_CHECK_MAGIC( pRngState );

    SymCryptRngAesUninstantiate( pRngState );

    t = 0;
    for( i=0; i< sizeof( *pRngState ) / sizeof( SIZE_T ); i ++ )
    {
        t |= p[i];
    }

    if( t != 0 )
    {
        SymCryptFatal( 'acdu' );
    }
}

VOID
SYMCRYPT_CALL
SymCryptRngAesInstantiateSelftest(void)
{
    SYMCRYPT_RNG_AES_STATE rng;

    SymCryptRngAesTestInstantiate( &rng );

    //
    // Uninstantiate has to be tested whenever another function is tested.
    //
    SymCryptRngAesTestUninstantiate( &rng );
}

VOID
SYMCRYPT_CALL
SymCryptRngAesReseedSelftest(void)
{
    SYMCRYPT_RNG_AES_STATE rng;

    SymCryptRngAesTestReseed( &rng );

    //
    // Uninstantiate has to be tested whenever another function is tested.
    //
    SymCryptRngAesTestUninstantiate( &rng );
}

VOID
SYMCRYPT_CALL
SymCryptRngAesGenerateSelftest(void)
{
    SYMCRYPT_RNG_AES_STATE rng;

    SymCryptRngAesTestGenerate( &rng );

    //
    // Uninstantiate has to be tested whenever another function is tested.
    //
    SymCryptRngAesTestUninstantiate( &rng );
}


///////////////////////////////////////////////////////////////////
// AES-CTR_DRGB with FIPS 140-2 continuous self-test
//


_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRngAesFips140_2Instantiate( PSYMCRYPT_RNG_AES_FIPS140_2_STATE   pRngState,
                                    PCBYTE                              pcbSeedMaterial,
                                    SIZE_T                              cbSeedMaterial )
{
    SYMCRYPT_ERROR scError;

    scError = SymCryptRngAesInstantiate( &pRngState->rng, pcbSeedMaterial, cbSeedMaterial );

    if( scError == SYMCRYPT_NO_ERROR )
    {
        //
        // Generate the first block of output and store it so that we can compare future blocks.
        //
        SymCryptRngAesGenerate( &pRngState->rng, pRngState->rng.previousBlock, sizeof( pRngState->rng.previousBlock ) );
        pRngState->rng.fips140_2Check = TRUE;
    }

    return scError;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptRngAesFips140_2Generate(    PSYMCRYPT_RNG_AES_FIPS140_2_STATE   pRngState,
                                    PBYTE                               pbRandom,
                                    SIZE_T                              cbRandom )
{
    SymCryptRngAesGenerate( &pRngState->rng, pbRandom, cbRandom );
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRngAesFips140_2Reseed(  PSYMCRYPT_RNG_AES_FIPS140_2_STATE   pRngState,
                                PCBYTE                              pcbSeedMaterial,
                                SIZE_T                              cbSeedMaterial )
{
    return SymCryptRngAesReseed( &pRngState->rng, pcbSeedMaterial, cbSeedMaterial );
}


_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptRngAesFips140_2Uninstantiate(   PSYMCRYPT_RNG_AES_FIPS140_2_STATE   pRngState )
{
    SymCryptRngAesUninstantiate( &pRngState->rng );
}
