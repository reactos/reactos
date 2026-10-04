//
// tlsCbcVerify.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// This code needs to process data in words, and we'd like to use 32-bit words on 32-bit
// architectures and 64-bit words on 64-bit architectures. So we use NATIVE_UINT & friends.
//
// We don't want to use 64-bit words on 32-bit architectures because the 64-bit shift/rotate
// code might not be constant-time, and it puts further register pressure on the x86 that can only
// use 6 registers in C code.
//

#if NATIVE_BYTES == 8
#define NATIVE_01   (0x0101010101010101)
#elif NATIVE_BYTES == 4
#define NATIVE_01   (0x01010101)
#else
#error Unexpected NATIVE_BYTES value
#endif

//
// MASK32 macros return UINT32 values based on conditions of the inputs
//

// MASK32_LT returns a UINT32 that is -1 if _a < _b, 0 otherwise.
#define MASK32_LT( _a, _b )     ((UINT32)( ( (INT32)((_a)-(_b)) ) >> 31 ) )

// MASK32_EQ returns a UINT32 that is -1 if _a == _b, 0 otherwise.
#define MASK32_EQ( _a, _b )     (~(UINT32)(-(INT32)((_a) ^ (_b)) >> 31))


//
// Native Byte mask generation is done in inlined functions, as that makes them much more readable
// These are mask values that are computed per byte in the word.
//

// Relevant bits to look at when determining whether an index is in the word
// Difference of index & word start must be in 0..NATIVE_BYTES - 1
// This mask defines the relevant bits we look at.
// We avoid using the highest bits as we use fact that the result after the mask
// is positive. This works as all our positions are < 2^16.
#define MASKNB_INWORD_RELEVANTBITS (~(NATIVE_BYTES - 1) & 0x0fffffff)

#define MASKNB_BROADCAST( _b )     ((NATIVE_UINT)(_b) * NATIVE_01)

FORCEINLINE
NATIVE_UINT
SymCryptNMaskGe( UINT32 wordStart, UINT32 boundary )
// Return a word starting at byte wordStart from an array with a[i] = 0xff if i>=boundary, 0 otherwise
{
    INT32 diff32;
    NATIVE_INT anySet;
    UINT32 shift;

    // Mask that is -1 if boundary < wordStart + 8
    anySet = ((NATIVE_INT) boundary - (NATIVE_INT) wordStart - NATIVE_BYTES) >> (NATIVE_BITS - 1);

    // Compute the index of boundary into the word, possibly negative
    diff32 = (INT32)boundary - (INT32)wordStart;
    // Compute the necessary shift when the result will be partially set
    shift = 8 * (diff32 & (NATIVE_BYTES - 1));

    // Mask the shift to 0 if the word is to be all set as boundary < wordStart
    shift &= (INT32)~diff32 >> 31;

    return (NATIVE_UINT) anySet << shift;
}

FORCEINLINE
NATIVE_UINT
SymCryptNMaskEq( UINT32 wordStart, UINT32 boundary )
// Return a word starting at byte wordStart from an array with a[i] = (i == boundary) ? 0xff : 0
{
    INT32 diff32;
    NATIVE_UINT inWord;

    // 32-bit signed difference
    diff32 = (INT32)boundary - (INT32)wordStart;

    // inWord = (-1) if boundary is within the word, 0 otherwise
    // Cast to NATIVE_UINT is free on AMD64, as is subsequent cast to NATIVE_INT
    // A direct cast from INT32 to NATIVE_INT requires a sign extension instruction, so this is faster.
    inWord = ~ ((-(NATIVE_INT)(NATIVE_UINT)(diff32  & MASKNB_INWORD_RELEVANTBITS)) >> (NATIVE_BITS -1));

    return inWord & ((NATIVE_UINT)0xff << 8 * (diff32 & (NATIVE_BYTES - 1)) );
}

FORCEINLINE
NATIVE_UINT
SymCryptNMaskEq80( UINT32 wordStart, UINT32 boundary )
// Same as SymcryptNMaskEq except the 0xff is replaced with 0x80.
{
    INT32 diff32;
    NATIVE_UINT inWord;

    // 32-bit signed difference
    diff32 = (INT32)boundary - (INT32)wordStart;

    // inWord = (-1) if boundary is within the word, 0 otherwise
    inWord = ~ ((-(NATIVE_INT)(NATIVE_UINT)(diff32  & MASKNB_INWORD_RELEVANTBITS)) >> (NATIVE_BITS -1));

    return inWord & ((NATIVE_UINT)0x80 << 8 * (diff32 & (NATIVE_BYTES - 1)) );
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsCbcHmacVerifyCore(
    _In_                                        PCSYMCRYPT_HASH             pHash,
    _Inout_                                     PSYMCRYPT_COMMON_HASH_STATE pState,
    _In_reads_bytes_(cbData)                    PCBYTE                      pbData,
                                                SIZE_T                      cbData,
    _Inout_updates_( pHash->inputBlockSize / 2) PBYTE                       pbMacValue,
    _Inout_updates_( pHash->resultSize )        PBYTE                       pbHashResult,
    _Out_                                       PUINT32                     pu32PaddingError )
//
// The core of the constant-time TLS record validation.
// This appends the data part of the record to the hash state, returns the intermediate hash value,
// and extracts the MAC value out of the record and returns it.
//
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    UINT32 cbPad;
    UINT32 maxPadLength;
    UINT32 u32;
    UINT32 i;
    UINT32 iPaddingStart;       // using 'i' for index
    UINT32 iMacStart;
    NATIVE_UINT mInData;
    NATIVE_UINT mInMac;
    NATIVE_UINT mInPadding;
    NATIVE_UINT m;
    NATIVE_UINT nPaddingError = 0;     // nonzero if a padding byte value is wrong.
    UINT32 next;
    UINT32 cbHashPrefix;
    UINT32 cbExtendedData;
    UINT32 totalBytesHashed;
    UINT32 hashPaddingFinal;
    UINT32 resultHashBlockIndex;
    UINT32 lastHashBlockIndex;
    NATIVE_UINT w;
    NATIVE_UINT data;
    UINT32 backOffset;
    NATIVE_UINT * bufferLocation;
    NATIVE_UINT padBytes;
    UINT32 m32ResultBlock;
    NATIVE_UINT mResultBlock;
    SIZE_T tmp;
    const UINT32 cbMacValue = pHash->inputBlockSize / 2;

    SYMCRYPT_ASSERT( cbMacValue == SymCryptRoundUpPow2Sizet(pHash->resultSize) );

    // Process all the data up to the part where the MAC value might appear
    // The if() is safe as both cbData and u32 are public values.
    u32 = pHash->resultSize + 256;
    if( cbData > u32 )
    {
        (*pHash->appendFunc)(pState, pbData, cbData-u32 );
        pbData += cbData - u32;
        cbData = u32;
    }

    // Check that we have enough data for a valid record.
    // We need one MAC value plus one padding_length byte
    if (cbData < pHash->resultSize + 1)
    {
        scError = SYMCRYPT_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    // We OR our results into the result buffers, so we must init them to zero
    SymCryptWipe( pbMacValue, cbMacValue );
    SymCryptWipe( pbHashResult, pHash->resultSize );

    // Pick up the padding_length. Note that this is the value we have to keep secret from
    // side-channel attacks.
    cbPad = pbData[cbData - 1];

    // We reduce cbData so that the padding_length byte is no longer under consideration.
    cbData -= 1;

    // Bound the padding length to cbData - mac_length
    // This doesn't reveal data as we treat all cbPad values the same, but it makes our
    // further computations easier
    maxPadLength = (UINT32)cbData - pHash->resultSize;          // We checked this is >= 0
    u32 = MASK32_LT( maxPadLength, cbPad );             // mask: maxPadLength < cbPad
    cbPad = cbPad + ((maxPadLength - cbPad) & u32);
    nPaddingError |= u32;           // mark as padding error

    // From here on out we maintain indices into a conceptual extended buffer with length cbExtendedData,
    // and index 0 at the start of the hash computation.
    // This aligns us with the hash input block, and simplifies hash padding computations and word
    // accesses.
    // However, we must always subtract cbHashPrefix from indices before using them to access bytes
    // in pbData, as the HashPrefix was already hashed into the MAC state.

    cbHashPrefix = (UINT32)pState->dataLengthL;     // # bytes already hashed into the MAC state
    cbExtendedData = (UINT32)cbData + cbHashPrefix; // total # bytes that the conceptual extended buffer has
    next = cbHashPrefix;                            // next index we will consider for processing (start of pbData)

    totalBytesHashed = cbExtendedData - pHash->resultSize - cbPad;
    SYMCRYPT_STORE_MSBFIRST32( &hashPaddingFinal, totalBytesHashed * 8 );      // Length padding for result hash block

    // We need to figure out what the index is of the last hash input block in the computation
    // (including any phantom blocks after the actual hash is done) and the index of the result
    // hash block of the actual hash computation.
    // We've limited the max input for simplicity (everything fits in 32 bits)
    // We also avoid 64-bit operations as their implementation on 32-bit architectures might not
    // always be constant-time.
    // This computation works for SHA-1, SHA-256, and SHA-384 which is all we care about
    // We avoid using % as the runtime isn't constant and our inputs are secret.

    // First the actual # bytes in the real and phantom computation
    resultHashBlockIndex = totalBytesHashed + 1 + pHash->inputBlockSize / 8;    // 1 byte 0x80 + length padding in last block
    lastHashBlockIndex = resultHashBlockIndex + cbPad;                          // The furthest any hash could go

    // round up to a whole # blocks
    resultHashBlockIndex = (resultHashBlockIndex + pHash->inputBlockSize - 1) & ~(pHash->inputBlockSize - 1);
    lastHashBlockIndex   = (lastHashBlockIndex   + pHash->inputBlockSize - 1) & ~(pHash->inputBlockSize - 1);

    // Compute the indices where the MAC and padding start
    iPaddingStart = cbExtendedData - cbPad;
    iMacStart = iPaddingStart - pHash->resultSize;

    SYMCRYPT_ASSERT( iMacStart < cbExtendedData );    // Fail if the last computation underflowed.

    // Align our handling to the native word size so that we can safely use native words
    if( (next & (NATIVE_BYTES - 1)) != 0 )
    {
        backOffset = next & (NATIVE_BYTES - 1);

        // Process a partial word
        SYMCRYPT_ASSERT( ( (next ^ pState->bytesInBuffer) & (NATIVE_BYTES - 1) ) == 0 );

        // Read a word; as the MAC value is > 8 bytes this won't overflow the buffer
        w = *(NATIVE_UINT *) &pbData[0];
        m = SymCryptNMaskGe( next, iMacStart );
        mInData = ~m;
        mInMac = m;

        data = w & mInData;
        data |= SymCryptNMaskEq80( next, iMacStart );    // add 0x80 byte @ iMacStart

        // Now we put the data into the hash buffer
        bufferLocation = (NATIVE_UINT *)&pState->buffer[ pState->bytesInBuffer - backOffset ];
        *bufferLocation = (*bufferLocation & (((NATIVE_UINT)1 << 8*backOffset) - 1))  | (data << 8*backOffset);
        pState->bytesInBuffer += NATIVE_BYTES - backOffset;

        // And the MAC data in the mac buffer
        *(NATIVE_UINT *)&pbMacValue[(next - backOffset) & (cbMacValue - 1)] |= (w & mInMac) << 8*backOffset;

        if( pState->bytesInBuffer == pHash->inputBlockSize )
        {
            // Block is full. This can't be the result block as we didn't have room for the padding yet.
            (*pHash->appendBlockFunc)( (PBYTE)pState + pHash->chainOffset, &pState->buffer[0], pHash->inputBlockSize, &tmp );
            pState->bytesInBuffer = 0;
        }

        next += NATIVE_BYTES - backOffset;
    }

    padBytes = MASKNB_BROADCAST( cbPad );

    // Now we can loop over the data in whole words
    while( next <= cbExtendedData - NATIVE_BYTES )
    {
        w = *(NATIVE_UINT *) &pbData[next - cbHashPrefix];

        m = SymCryptNMaskGe( next, iMacStart );
        mInMac = m;
        mInData = ~m;

        m = SymCryptNMaskGe( next, iPaddingStart );
        mInPadding = m;
        mInMac &= ~m;

        data = w & mInData;
        data |= SymCryptNMaskEq80( next, iMacStart );    // add 0x80 byte @ iMacStart

        *(NATIVE_UINT *)(&pState->buffer[ pState->bytesInBuffer ]) = data;
        pState->bytesInBuffer += NATIVE_BYTES;

        if (pState->bytesInBuffer == pHash->inputBlockSize)
        {
            // Insert the length component of the hash padding (only in result block)
            m32ResultBlock = MASK32_EQ( next, resultHashBlockIndex - NATIVE_BYTES );
            *(UINT32*) &pState->buffer[ pHash->inputBlockSize - 4 ] |= hashPaddingFinal & m32ResultBlock;

            (*pHash->appendBlockFunc)( (PBYTE)pState + pHash->chainOffset, &pState->buffer[0], pHash->inputBlockSize, &tmp );
            SYMCRYPT_ASSERT( tmp == 0 );

            mResultBlock = (NATIVE_UINT)(NATIVE_INT)(INT32) m32ResultBlock;       // Convert 32-bit mask to native mask

            // Masked copy of result to result buffer
            // We do whole words, and then an optional UINT32 to handle the 20-byte SHA-1 result on AMD64.
            // The for() and if() are side-channel safe as the resultSize and NATIVE_BYTES values are public.
            for ( i = 0; i < pHash->resultSize / NATIVE_BYTES; i++)
            {
                ((NATIVE_UINT *)pbHashResult)[i] |= ((NATIVE_UINT *)((PBYTE)pState + pHash->chainOffset))[i] & mResultBlock;
            }
            if( (pHash->resultSize & (NATIVE_BYTES - 1)) != 0 )
            {
                *(UINT32 *) (&pbHashResult[ pHash->resultSize - 4 ]) |= *(UINT32 *) ((PBYTE) pState + pHash->chainOffset + pHash->resultSize - 4) & (UINT32) mResultBlock;
            }
            pState->bytesInBuffer = 0;
        }

        *(NATIVE_UINT *)&pbMacValue[next & (cbMacValue - 1)] |= w & mInMac;

        nPaddingError |= (w ^ padBytes) & mInPadding;

        next += NATIVE_BYTES;
    }

    if( next < cbExtendedData )
    {
        // Process the remaining bytes. This can't be data so we only do the MAC and padding...
        // The main difference is that we read the last full word in pbData and then align it
        // as if we read the next word starting at pbData[next - cbHashPrefix]
        w = *(NATIVE_UINT *) &pbData[ cbData - NATIVE_BYTES ];      // last word
        w >>= 8 * (next - cbExtendedData + NATIVE_BYTES );          // Shift to right location
        padBytes >>= 8 * (next - cbExtendedData + NATIVE_BYTES );   // Zero padBytes that are never read

        m = SymCryptNMaskGe( next, iPaddingStart );
        mInPadding = m;
        mInMac = ~m;

        *(NATIVE_UINT *)&pbMacValue[next & (cbMacValue - 1)] |= w & mInMac;

        nPaddingError |= (w ^ padBytes) & mInPadding;
        next = cbExtendedData;
    }

    // At this point we still have to potentially do one more hash block.
    // The data is all copied into the hash input buffer, as is the 0x80 padding byte.

    if (next < lastHashBlockIndex)
    {
        // there is still one more hash block to compute. This could either be the actual last block of the hash
        // computation, or a phantom block for side-channel hiding.
        // This IF depends only on the cbData, the # bytes hashed before this final pbData buffer, and the hash algorithm
        // properties.
        // We never need to compute more than 1 additional hash block as we are at least pHash->resultSize bytes beyond the
        // actual data.
        SymCryptWipe( &pState->buffer[ pState->bytesInBuffer], pHash->inputBlockSize - pState->bytesInBuffer );

        // Just put in the padding, no need to mask this
        *(UINT32*) &pState->buffer[ pHash->inputBlockSize - 4 ] = hashPaddingFinal;

        (*pHash->appendBlockFunc)( (PBYTE)pState + pHash->chainOffset, &pState->buffer[0], pHash->inputBlockSize, &tmp );
        SYMCRYPT_ASSERT( tmp == 0 );

        // Masked copy of the result
        mResultBlock = (NATIVE_UINT)(NATIVE_INT)(INT32) MASK32_EQ( lastHashBlockIndex, resultHashBlockIndex );

        // Masked copy of result to result buffer
        // We do whole words, and then an optional UINT32 to handle the 20-byte SHA-1 result on AMD64.
        for ( i = 0; i < pHash->resultSize / NATIVE_BYTES; i++)
        {
            ((NATIVE_UINT *)pbHashResult)[i] |= ((NATIVE_UINT *)((PBYTE)pState + pHash->chainOffset))[i] & mResultBlock;
        }
        if( (pHash->resultSize & (NATIVE_BYTES - 1)) != 0 )
        {
            *(UINT32 *) (&pbHashResult[ pHash->resultSize - 4 ]) |= *(UINT32 *) ((PBYTE) pState + pHash->chainOffset + pHash->resultSize - 4) & (UINT32) mResultBlock;
        }
        pState->bytesInBuffer = 0;
    }

    // Now we have the hash result, and the Mac value buffer is filled with a rotated copy of the Mac value.
    // We have to un-rotate the Mac value.

    // Check that we have the right hash result
    //for( SIZE_T t=0; t < cbMacValue; t++ )
    //{
    //    SYMCRYPT_ASSERT( pbMacValue[ (iMacStart + t) & (cbMacValue - 1 ) ] == (t >= pHash->resultSize ? 0 : pbData[iMacStart - cbHashPrefix + t] ));
    //}

    SymCryptScsRotateBuffer(  pbMacValue, cbMacValue, iMacStart & (cbMacValue - 1) );

    //for( SIZE_T t=0; t < cbMacValue; t++ )
    //{
    //    SYMCRYPT_ASSERT( pbMacValue[ t ] == (t >= pHash->resultSize ? 0 : pbData[iMacStart - cbHashPrefix + t] ));
    //}

cleanup:

    nPaddingError |= nPaddingError >> (NATIVE_BITS/2);      // Map possibly 64 bits down to 32

    *pu32PaddingError = (UINT32) nPaddingError;

    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsCbcHmacVerify(
    _In_                PCSYMCRYPT_MAC  pMacAlgorithm,
    _In_                PVOID           pExpandedKey,
    _Inout_             PVOID           pState,
    _In_reads_(cbData)  PCBYTE          pbData,
                        SIZE_T          cbData)
{
    BYTE abMacValue[64];
    BYTE abHashResult[48];
    UINT32 u32PaddingError;
    PSYMCRYPT_COMMON_HASH_STATE pHashState = (PSYMCRYPT_COMMON_HASH_STATE) pState;
    PCSYMCRYPT_HASH pHashAlgorithm = *(pMacAlgorithm->ppHashAlgorithm);
    UINT32 i;

    SYMCRYPT_ASSERT(pMacAlgorithm == SymCryptHmacSha1Algorithm ||
                    pMacAlgorithm == SymCryptHmacSha256Algorithm ||
                    pMacAlgorithm == SymCryptHmacSha384Algorithm );
    SYMCRYPT_ASSERT(((*(pMacAlgorithm->ppHashAlgorithm))->inputBlockSize)/2 <= 64);
    SYMCRYPT_ASSERT((*(pMacAlgorithm->ppHashAlgorithm))->resultSize <= 48);

    SymCryptTlsCbcHmacVerifyCore(
        pHashAlgorithm,
        pHashState,
        pbData,
        cbData,
        abMacValue,
        abHashResult,
        &u32PaddingError );

    // We have the hash value, convert it to a MAC value
    // First we set up the chaining value
    memcpy( ((PBYTE)pHashState + pHashAlgorithm->chainOffset),
            (PBYTE)pExpandedKey + pMacAlgorithm->outerChainingStateOffset,
            pHashAlgorithm->chainSize );
    // Then copy the data & set the length
    // The hash result wasn't BSWAPPED yet...
    if( pMacAlgorithm->resultSize <= 32 )
    {
        SymCryptUint32ToMsbFirst( (UINT32 *) abHashResult, pHashState->buffer, pHashAlgorithm->resultSize / 4 );
    } else {
        SymCryptUint64ToMsbFirst( (UINT64 *) abHashResult, pHashState->buffer, pHashAlgorithm->resultSize / 8 );
    }
    pHashState->bytesInBuffer = pHashAlgorithm->resultSize;
    pHashState->dataLengthL = pHashAlgorithm->resultSize + pHashAlgorithm->inputBlockSize;

    (*pHashAlgorithm->resultFunc)( pHashState, abHashResult );

    // Verify in 32-bit chunks to support SHA-1 without further problems
    for( i=0; i<pHashAlgorithm->resultSize / 4; i++ )
    {
        u32PaddingError |= *(PUINT32)&abHashResult[4*i] ^ *(PUINT32)&abMacValue[4*i];
    }

    // We may reveal the final error-or-no-error as that will be visible anyway
    return u32PaddingError == 0 ? SYMCRYPT_NO_ERROR : SYMCRYPT_AUTHENTICATION_FAILURE;
}
