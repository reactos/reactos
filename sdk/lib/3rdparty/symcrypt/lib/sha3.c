//
// Sha3.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//


//
// Keccak state
//
// Keccak-f[1600] state consists of 25 64-bit words. We represent this state as a single
// dimensional array of 25 elements (Wi being the i^th element of the array for i=0..24)
// with the following mapping to two dimensional coordinates. Note that in FIPS 202 Figure 2,
// the element W0 at (x,y)=(0,0) is depicted in the middle of the 5x5 array. We set W0
// to be the first element so that the rate part of the permutation maps to the beginning
// of the state.
//
//       x=0  x=1  x=2  x=3  x=4
//       -----------------------
// y=0    W0   W1   W2   W3   W4
// y=1    W5   W6   W7   W8   W9
// y=2   W10  W11  W12  W13  W14
// y=3   W15  W16  W17  W18  W19
// y=4   W20  W21  W22  W23  W24



// Rotation constants for Keccak Rho transformation
static const UINT8 KeccakRhoK[25] = {
     0,  1, 62, 28, 27,     // y = 0
    36, 44,  6, 55, 20,     // y = 1
     3, 10, 43, 25, 39,     // y = 2
    41, 45, 15, 21,  8,     // y = 3
    18,  2, 61, 56, 14,     // y = 4
};

// Keccak round constants
static UINT64 KeccakIotaK[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
    0x000000000000808bULL, 0x0000000080000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008aULL, 0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL, 0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
};

// XOR sum of column c of the state
#define KECCAK_COLUMN_SUM(state, c) \
    (state[0 + (c)] ^ state[5 + (c)] ^ state[10 + (c)] ^ state[15 + (c)] ^ state[20 + (c)])

// XOR w to all the lanes in column c of the state
//
// Note: The expression to be XORed is copied to a temporary variable to avoid reevaluation
#define KECCAK_COLUMN_UPDATE(state, c, w) { \
    UINT64 t = (w); \
    state[ 0 + (c)] ^= t; \
    state[ 5 + (c)] ^= t; \
    state[10 + (c)] ^= t; \
    state[15 + (c)] ^= t; \
    state[20 + (c)] ^= t; \
}

// Apply Theta transformation to the state
#define KECCAK_THETA(state) { \
    UINT64 colSum[5]; \
    colSum[0] = KECCAK_COLUMN_SUM(state, 0); \
    colSum[1] = KECCAK_COLUMN_SUM(state, 1); \
    colSum[2] = KECCAK_COLUMN_SUM(state, 2); \
    colSum[3] = KECCAK_COLUMN_SUM(state, 3); \
    colSum[4] = KECCAK_COLUMN_SUM(state, 4); \
    KECCAK_COLUMN_UPDATE(state, 0, colSum[4] ^ ROL64(colSum[1], 1)); \
    KECCAK_COLUMN_UPDATE(state, 1, colSum[0] ^ ROL64(colSum[2], 1)); \
    KECCAK_COLUMN_UPDATE(state, 2, colSum[1] ^ ROL64(colSum[3], 1)); \
    KECCAK_COLUMN_UPDATE(state, 3, colSum[2] ^ ROL64(colSum[4], 1)); \
    KECCAK_COLUMN_UPDATE(state, 4, colSum[3] ^ ROL64(colSum[0], 1)); \
}

// Apply Rho transformation to row r of the state
#define KECCAK_RHO_ROW(state, r) { \
    state[5 * (r) + 0] = ROL64(state[5 * (r) + 0], KeccakRhoK[5 * (r) + 0]); \
    state[5 * (r) + 1] = ROL64(state[5 * (r) + 1], KeccakRhoK[5 * (r) + 1]); \
    state[5 * (r) + 2] = ROL64(state[5 * (r) + 2], KeccakRhoK[5 * (r) + 2]); \
    state[5 * (r) + 3] = ROL64(state[5 * (r) + 3], KeccakRhoK[5 * (r) + 3]); \
    state[5 * (r) + 4] = ROL64(state[5 * (r) + 4], KeccakRhoK[5 * (r) + 4]); \
}

// Apply Rho transformation to row 0 of the state
//
// The first row contains a rotation by 0 on the first lane that uses a shift
// by 64 which we want to avoid. Rho operation below omits the rotation on the first lane.
#define KECCAK_RHO_ROW0(state) { \
    state[1] = ROL64(state[1], KeccakRhoK[1]); \
    state[2] = ROL64(state[2], KeccakRhoK[2]); \
    state[3] = ROL64(state[3], KeccakRhoK[3]); \
    state[4] = ROL64(state[4], KeccakRhoK[4]); \
}

// Apply Rho transformation to the state
#define KECCAK_RHO(state) { \
    KECCAK_RHO_ROW0(state); \
    KECCAK_RHO_ROW(state, 1); \
    KECCAK_RHO_ROW(state, 2); \
    KECCAK_RHO_ROW(state, 3); \
    KECCAK_RHO_ROW(state, 4); \
}

// Apply Pi transformation to the state
#define KECCAK_PI(state) { \
    UINT64 t  = state[ 1]; state[ 1] = state[ 6]; state[ 6] = state[ 9]; state[ 9] = state[22]; state[22] = state[14]; \
    state[14] = state[20]; state[20] = state[ 2]; state[ 2] = state[12]; state[12] = state[13]; state[13] = state[19]; \
    state[19] = state[23]; state[23] = state[15]; state[15] = state[ 4]; state[ 4] = state[24]; state[24] = state[21]; \
    state[21] = state[ 8]; state[ 8] = state[16]; state[16] = state[ 5]; state[ 5] = state[ 3]; state[ 3] = state[18]; \
    state[18] = state[17]; state[17] = state[11]; state[11] = state[ 7]; state[ 7] = state[10]; state[10] = t; \
}

// Apply Chi transformation on row r of state
#define KECCAK_CHI_ROW(state, r) { \
    UINT64 t1 = state[5 * (r) + 0] ^ (~state[5 * (r) + 1] & state[5 * (r) + 2]); \
    UINT64 t2 = state[5 * (r) + 1] ^ (~state[5 * (r) + 2] & state[5 * (r) + 3]); \
    state[5 * (r) + 2] = state[5 * (r) + 2] ^ (~state[5 * (r) + 3] & state[5 * (r) + 4]); \
    state[5 * (r) + 3] = state[5 * (r) + 3] ^ (~state[5 * (r) + 4] & state[5 * (r) + 0]); \
    state[5 * (r) + 4] = state[5 * (r) + 4] ^ (~state[5 * (r) + 0] & state[5 * (r) + 1]); \
    state[5 * (r) + 0] = t1; \
    state[5 * (r) + 1] = t2; \
}

// Apply Chi transformation to state
#define KECCAK_CHI(state) { \
    KECCAK_CHI_ROW(state, 0); \
    KECCAK_CHI_ROW(state, 1); \
    KECCAK_CHI_ROW(state, 2); \
    KECCAK_CHI_ROW(state, 3); \
    KECCAK_CHI_ROW(state, 4); \
}

// Add round constant to state
#define KECCAK_IOTA(state, rnd) state[0] ^= KeccakIotaK[rnd]

// Perform one round of Keccak permutation on state
#define KECCAK_PERM_ROUND(state, rnd) { \
    KECCAK_THETA(state); \
    KECCAK_RHO(state); \
    KECCAK_PI(state); \
    KECCAK_CHI(state); \
    KECCAK_IOTA(state, rnd); \
}


//
// SymCryptKeccakPermute
//
VOID
SYMCRYPT_CALL
SymCryptKeccakPermute(_Inout_updates_(25) UINT64* pState)
{
    for (int r = 0; r < 24; r++)
    {
        KECCAK_PERM_ROUND(pState, r);
    }
}


//
// SymCryptKeccakInit
//
VOID
SYMCRYPT_CALL
SymCryptKeccakInit(_Out_ PSYMCRYPT_KECCAK_STATE pState, UINT32 inputBlockSize, UINT8 paddingValue)
{
    pState->inputBlockSize = inputBlockSize;
    pState->paddingValue = paddingValue;

    // Initialize the Keccak permutation state and set mutable state variables
    // to their default values.
    SymCryptKeccakReset(pState);
}

VOID
SYMCRYPT_CALL
SymCryptKeccakReset(_Out_ PSYMCRYPT_KECCAK_STATE pState)
{
    //
    // Wipe & re-initialize
    //
    // Wipe the Keccak permutation state and set the mutable state variables to their
    // default values. Non-mutable state variables retain their values. State becomes
    // re-initialized after this call.
    SymCryptWipeKnownSize(pState->state, sizeof(pState->state));
    pState->stateIndex = 0;
    pState->squeezeMode = FALSE;
}

//
// SymCryptKeccakAppendByte
//
FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptKeccakAppendByte(_Inout_ PSYMCRYPT_KECCAK_STATE  pState, BYTE val)
{
    SYMCRYPT_ASSERT(!pState->squeezeMode);
    SYMCRYPT_ASSERT(pState->stateIndex < pState->inputBlockSize);

    pState->state[pState->stateIndex / sizeof(UINT64)] ^= ((UINT64)val << (8 * (pState->stateIndex % 8)));
    pState->stateIndex++;
}

//
// SymCryptKeccakAppendBytes
//
FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptKeccakAppendBytes(_Inout_ PSYMCRYPT_KECCAK_STATE  pState, PCBYTE pbBuffer, SIZE_T cbBuffer)
{
    SYMCRYPT_ASSERT(!pState->squeezeMode);
    SYMCRYPT_ASSERT((pState->stateIndex + cbBuffer) <= pState->inputBlockSize);

    for (SIZE_T i = 0; i < cbBuffer; i++)
    {
        pState->state[(pState->stateIndex + i) / sizeof(UINT64)] ^= ((UINT64)pbBuffer[i] << (8 * ((pState->stateIndex + i) % 8)));
    }

    pState->stateIndex += (UINT32)cbBuffer;
}


//
// SymCryptKeccakAppendLanes
//
VOID
SYMCRYPT_CALL
SymCryptKeccakAppendLanes(
    _Inout_                                 PSYMCRYPT_KECCAK_STATE  pState,
    _In_reads_(uLaneCount * sizeof(UINT64)) PCBYTE                  pbData,
                                            SIZE_T                  uLaneCount)
{
    SYMCRYPT_ASSERT(!pState->squeezeMode);
    SYMCRYPT_ASSERT((pState->inputBlockSize & 0x7) == 0);
    SYMCRYPT_ASSERT((pState->stateIndex & 0x7) == 0);
    SYMCRYPT_ASSERT(pState->stateIndex != pState->inputBlockSize);

    // Locate the lane in the state for next append.
    // Currently, pState->stateIndex/sizeof(UINT64) of the lanes are used.
    UINT32 uLaneIndex = pState->stateIndex / sizeof(UINT64);

    for (SIZE_T i = 0; i < uLaneCount; i++)
    {
        pState->state[uLaneIndex] ^= SYMCRYPT_LOAD_LSBFIRST64(pbData + i * sizeof(UINT64));
        pState->stateIndex += sizeof(UINT64);
        uLaneIndex++;

        if (pState->stateIndex == pState->inputBlockSize)
        {
            SymCryptKeccakPermute(pState->state);
            pState->stateIndex = 0;
            uLaneIndex = 0;
        }
    }
}

//
// SymCryptKeccakZeroAppendBlock
//
VOID
SYMCRYPT_CALL
SymCryptKeccakZeroAppendBlock(_Inout_ PSYMCRYPT_KECCAK_STATE  pState)
{
    SYMCRYPT_ASSERT(!pState->squeezeMode);
    SymCryptKeccakPermute(pState->state);
    pState->stateIndex = 0;
}

//
// SymCryptKeccakAppend
//
VOID
SYMCRYPT_CALL
SymCryptKeccakAppend(
    _Inout_                 PSYMCRYPT_KECCAK_STATE  pState,
    _In_reads_(cbData)      PCBYTE                  pbData,
                            SIZE_T                  cbData)
{
    SYMCRYPT_ASSERT(pState->inputBlockSize % 8 == 0);

    // If we were in squeeze mode (Append is called after an Extract without wiping),
    // switch to absorb mode to start a new hash computation.
    if (pState->squeezeMode)
    {
        SymCryptKeccakReset(pState);
    }

    SYMCRYPT_ASSERT(pState->stateIndex < pState->inputBlockSize);

    // Make pState->stateIndex a multiple of 8.
    // Message block boundary will not be crossed, check
    // if permutation is needed after this part.
    while (cbData > 0 && (pState->stateIndex & 0x7))
    {
        SymCryptKeccakAppendByte(pState, *pbData);
        pbData++;
        cbData--;
    }

    // Permute if input message block is filled
    if (pState->stateIndex == pState->inputBlockSize)
    {
        SymCryptKeccakPermute(pState->state);
        pState->stateIndex = 0;
    }

    // Append full lanes
    SIZE_T uFullLanes = cbData / sizeof(UINT64);
    if (uFullLanes > 0)
    {
        SymCryptKeccakAppendLanes(pState, pbData, uFullLanes);
        pbData += uFullLanes * sizeof(UINT64);
        cbData -= uFullLanes * sizeof(UINT64);
    }

    SYMCRYPT_ASSERT(cbData < sizeof(UINT64));
    SymCryptKeccakAppendBytes(pState, pbData, cbData);

    SYMCRYPT_ASSERT(pState->stateIndex != pState->inputBlockSize);
}

//
// SymCryptKeccakApplyPadding
//
VOID
SYMCRYPT_CALL
SymCryptKeccakApplyPadding(_Inout_ PSYMCRYPT_KECCAK_STATE pState)
{
    SYMCRYPT_ASSERT(!pState->squeezeMode);

    // Locate the lane and byte position for the padding byte
    UINT32 uLanePos = pState->stateIndex / sizeof(UINT64);
    UINT32 uBytePos = pState->stateIndex % sizeof(UINT64);
    pState->state[uLanePos] ^= ((UINT64)pState->paddingValue << (8 * uBytePos));

    // Pad the final 1 bit to the msb of the last lane in the rate portion of the state
    pState->state[pState->inputBlockSize / sizeof(UINT64) - 1] ^= (1ULL << 63);

    // Process the padded block and switch to squeeze mode
    SymCryptKeccakPermute(pState->state);
    pState->stateIndex = 0;
    pState->squeezeMode = TRUE;
}

//
// SymCryptKeccakExtractByte
//
FORCEINLINE
BYTE
SYMCRYPT_CALL
SymCryptKeccakExtractByte(_Inout_ PSYMCRYPT_KECCAK_STATE  pState)
{
    SYMCRYPT_ASSERT(pState->squeezeMode);
    SYMCRYPT_ASSERT(pState->stateIndex < pState->inputBlockSize);

    BYTE ret = (BYTE)((pState->state[pState->stateIndex / sizeof(UINT64)] >> (8 * (pState->stateIndex % 8))) & 0xff);
    pState->stateIndex++;
    return ret;
}

//
// SymCryptKeccakExtractLanes
//
VOID
SYMCRYPT_CALL
SymCryptKeccakExtractLanes(
    _Inout_                                     PSYMCRYPT_KECCAK_STATE  pState,
    _Out_writes_(uLaneCount * sizeof(UINT64))   PBYTE                   pbResult,
                                                SIZE_T                  uLaneCount)
{
    SYMCRYPT_ASSERT(pState->squeezeMode);
    SYMCRYPT_ASSERT((pState->inputBlockSize & 0x7) == 0);
    SYMCRYPT_ASSERT((pState->stateIndex & 0x7) == 0);

    // Locate the lane in the state for next extraction
    UINT32 uLaneIndex = pState->stateIndex / sizeof(UINT64);

    for (SIZE_T i = 0; i < uLaneCount; i++)
    {
        SYMCRYPT_ASSERT(pState->stateIndex <= pState->inputBlockSize);

        if (pState->stateIndex == pState->inputBlockSize)
        {
            SymCryptKeccakPermute(pState->state);
            pState->stateIndex = 0;
            uLaneIndex = 0;
        }

        SYMCRYPT_STORE_LSBFIRST64(pbResult + i * sizeof(UINT64), pState->state[uLaneIndex]);
        pState->stateIndex += sizeof(UINT64);
        uLaneIndex++;
    }
}

//
// SymCryptKeccakExtract
//
VOID
SYMCRYPT_CALL
SymCryptKeccakExtract(
    _Inout_                 PSYMCRYPT_KECCAK_STATE  pState,
    _Out_writes_(cbResult)  PBYTE                   pbResult,
                            SIZE_T                  cbResult,
                            BOOLEAN                 bWipe)
{
    // Apply padding and switch to squeeze mode if this is the first call to Extract
    if (!pState->squeezeMode)
    {
        SymCryptKeccakApplyPadding(pState);
    }

    // Do the permutation if there are no bytes available in the state
    if ( (cbResult > 0) && (pState->stateIndex == pState->inputBlockSize) )
    {
        SymCryptKeccakPermute(pState->state);
        pState->stateIndex= 0;
    }

    // Make stateIndex a multiple of 8 so that the extraction can be performed in lanes.
    // We don't call the permutation as soon as the stateIndex reaches inputBlockSize,
    // cbResult must also be non-zero for that. This condition is checked
    // in ExtractLanes or in the 'remaining bytes' block that follows it.
    while (cbResult > 0 && (pState->stateIndex & 0x7))
    {
        *pbResult = SymCryptKeccakExtractByte(pState);
        pbResult++;
        cbResult--;
    }

    SYMCRYPT_ASSERT((cbResult == 0) || ((pState->stateIndex & 0x7) == 0));

    // Extract full lanes
    SIZE_T uFullLanes = cbResult / sizeof(UINT64);
    if (uFullLanes > 0)
    {
        SymCryptKeccakExtractLanes(pState, pbResult, uFullLanes);
        pbResult += uFullLanes * sizeof(UINT64);
        cbResult -= uFullLanes * sizeof(UINT64);
    }

    // Extract the remaining bytes
    SYMCRYPT_ASSERT(cbResult < sizeof(UINT64));
    while (cbResult > 0)
    {
        if (pState->stateIndex == pState->inputBlockSize)
        {
            SymCryptKeccakPermute(pState->state);
            pState->stateIndex = 0;
        }

        *pbResult = SymCryptKeccakExtractByte(pState);
        pbResult++;
        cbResult--;
    }

    if (bWipe)
    {
        // Wipe the Keccak state and make it ready for a new hash computation
        SymCryptKeccakReset(pState);
    }
}

//
// SymCryptKeccakStateExport
//
VOID
SYMCRYPT_CALL
SymCryptKeccakStateExport(
                                                            SYMCRYPT_BLOB_TYPE      type,
    _In_                                                    PCSYMCRYPT_KECCAK_STATE pState,
    _Out_writes_bytes_(SYMCRYPT_KECCAK_STATE_EXPORT_SIZE)   PBYTE                   pbBlob)
{

    SYMCRYPT_ALIGN SYMCRYPT_KECCAK_STATE_EXPORT_BLOB    blob;           // local copy to have proper alignment.
    C_ASSERT(sizeof(blob) == SYMCRYPT_KECCAK_STATE_EXPORT_SIZE);

    SymCryptWipeKnownSize(&blob, sizeof(blob)); // wipe to avoid any data leakage

    blob.header.magic = SYMCRYPT_BLOB_MAGIC;
    blob.header.size = SYMCRYPT_KECCAK_STATE_EXPORT_SIZE;
    blob.header.type = type;

    //
    // Copy the relevant data. Buffer will be 0-padded.
    //

    SymCryptUint64ToLsbFirst(&pState->state[0], &blob.state[0], 25);
    blob.stateIndex = pState->stateIndex;
    blob.paddingValue = pState->paddingValue;
    blob.squeezeMode = pState->squeezeMode;

    SYMCRYPT_ASSERT((PCBYTE)&blob + sizeof(blob) - sizeof(SYMCRYPT_BLOB_TRAILER) == (PCBYTE)&blob.trailer);
    SymCryptMarvin32(SymCryptMarvin32DefaultSeed, (PCBYTE)&blob, sizeof(blob) - sizeof(SYMCRYPT_BLOB_TRAILER), &blob.trailer.checksum[0]);

    memcpy(pbBlob, &blob, sizeof(blob));

    SymCryptWipeKnownSize(&blob, sizeof(blob));
    return;
}


//
// SymCryptKeccakStateImport
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptKeccakStateImport(
                                                        SYMCRYPT_BLOB_TYPE      type,
    _Out_                                               PSYMCRYPT_KECCAK_STATE  pState,
    _In_reads_bytes_(SYMCRYPT_KECCAK_STATE_EXPORT_SIZE) PCBYTE                  pbBlob)
{
    SYMCRYPT_ERROR                  scError = SYMCRYPT_NO_ERROR;

    SYMCRYPT_ALIGN SYMCRYPT_KECCAK_STATE_EXPORT_BLOB blob;                       // local copy to have proper alignment.
    BYTE checksum[8];

    C_ASSERT(sizeof(blob) == SYMCRYPT_KECCAK_STATE_EXPORT_SIZE);
    memcpy(&blob, pbBlob, sizeof(blob));

    if (blob.header.magic != SYMCRYPT_BLOB_MAGIC ||
        blob.header.size != SYMCRYPT_KECCAK_STATE_EXPORT_SIZE ||
        blob.header.type != (UINT32)type)
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    SymCryptMarvin32(SymCryptMarvin32DefaultSeed, (PCBYTE)&blob, sizeof(blob) - sizeof(SYMCRYPT_BLOB_TRAILER), checksum);
    if (memcmp(checksum, &blob.trailer.checksum[0], 8) != 0)
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    SymCryptLsbFirstToUint64(&blob.state[0], &pState->state[0], 25);
    pState->stateIndex = blob.stateIndex;
    pState->paddingValue = blob.paddingValue;
    pState->squeezeMode = blob.squeezeMode;

    //
    // Set state fields based on the blob type and do validation
    //

    // default values indicate error
    pState->inputBlockSize = 0;
    pState->paddingValue = 0;

    switch (blob.header.type)
    {
        case SymCryptBlobTypeSha3_224State:
            pState->inputBlockSize = SYMCRYPT_SHA3_224_INPUT_BLOCK_SIZE;
            if (blob.paddingValue == SYMCRYPT_SHA3_PADDING_VALUE)
            {
                pState->paddingValue = blob.paddingValue;
            }
            break;
        case SymCryptBlobTypeSha3_256State:
            pState->inputBlockSize = SYMCRYPT_SHA3_256_INPUT_BLOCK_SIZE;
            if (blob.paddingValue == SYMCRYPT_SHA3_PADDING_VALUE)
            {
                pState->paddingValue = blob.paddingValue;
            }
            break;

        case SymCryptBlobTypeSha3_384State:
            pState->inputBlockSize = SYMCRYPT_SHA3_384_INPUT_BLOCK_SIZE;
            if (blob.paddingValue == SYMCRYPT_SHA3_PADDING_VALUE)
            {
                pState->paddingValue = blob.paddingValue;
            }
            break;

        case SymCryptBlobTypeSha3_512State:
            pState->inputBlockSize = SYMCRYPT_SHA3_512_INPUT_BLOCK_SIZE;
            if (blob.paddingValue == SYMCRYPT_SHA3_PADDING_VALUE)
            {
                pState->paddingValue = blob.paddingValue;
            }
            break;
        default:
            scError = SYMCRYPT_INVALID_BLOB;
            goto cleanup;
    }

    if (pState->inputBlockSize == 0 || pState->paddingValue == 0)
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    if (pState->stateIndex > pState->inputBlockSize)
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    // Allow stateIndex = inputBlockSize only in squeeze mode
    if ((pState->stateIndex == pState->inputBlockSize) && !pState->squeezeMode)
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

cleanup:
    SymCryptWipeKnownSize(&blob, sizeof(blob));

    return scError;
}
