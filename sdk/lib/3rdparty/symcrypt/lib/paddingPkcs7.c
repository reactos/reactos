//
// paddingPkcs7.c   Add/Remove PKCS7 padding
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"


VOID
SYMCRYPT_CALL
SymCryptPaddingPkcs7Add(
                                            SIZE_T  cbBlockSize,
    _In_reads_(cbSrc)                       PCBYTE  pbSrc,
                                            SIZE_T  cbSrc,
    _Out_writes_to_(cbDst, *pcbResult)      PBYTE   pbDst,
                                            SIZE_T  cbDst,
                                            SIZE_T* pcbResult)
{
    SIZE_T          cbPadVal;               // PadVal is the number of bytes to pad.
    SIZE_T          cbDataLastBlock;        // dwDataLastBlock is the number of bytes of data at the final block.
    SIZE_T          cbResult = 0;           // This variable must always have a valid value when we finish the function.

    SYMCRYPT_ASSERT(cbBlockSize < 256);                         // cbBlockSize must be < 256
    SYMCRYPT_ASSERT((cbBlockSize & (cbBlockSize - 1)) == 0);    // cbBlockSize must be a power of 2

    //
    // Compute the padding parameters.
    //

    cbDataLastBlock = (cbSrc & (cbBlockSize - 1));

    cbResult = (cbSrc - cbDataLastBlock + cbBlockSize);

    SYMCRYPT_ASSERT(cbDst >= cbResult);     // cbDst >= cbSrc - cbSrc % cbBlockSize + cbBlockSize

    if (cbResult > cbDst)
    {
        goto cleanup;
    }

    cbPadVal = (cbBlockSize - cbDataLastBlock);

    //
    //  perform the padding
    //

    // cbSrc must be greater than zero. memcpy(pbDst, NULL, 0) is not defined!
    if (pbDst != pbSrc && cbSrc > 0)
    {
        memcpy(pbDst, pbSrc, cbSrc);
    }

    memset(pbDst + cbSrc, (int)cbPadVal, cbPadVal);

cleanup:
    *pcbResult = cbResult;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptPaddingPkcs7Remove(
                                            SIZE_T  cbBlockSize,
    _In_reads_(cbSrc)                       PCBYTE  pbSrc,
                                            SIZE_T  cbSrc,
    _Out_writes_to_(cbDst, *pcbResult)      PBYTE   pbDst,
                                            SIZE_T  cbDst,
                                            SIZE_T* pcbResult)
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    UINT32  mPaddingError       = 0;    // Indicates whether there is an error in padding or not.
    UINT32  mBufferSizeError    = 0;    // Indicates whether pbDst is large enough to contain the entire message.
    UINT32  mask                = 0;    // Mask for message bytes at the final block.
    UINT32  cbPadVal;                   // PadVal is the number of padded bytes.
    UINT32  cbSrc32;
    UINT32  cbDst32;
    UINT32  cbMsg32;

    SIZE_T  cbBulk = 0;
    SIZE_T  cbResult;           // This variable must always have a valid value when we finish the function.


    SYMCRYPT_ASSERT(cbBlockSize < 256);                         // cbBlockSize must be < 256
    SYMCRYPT_ASSERT((cbBlockSize & (cbBlockSize - 1)) == 0);    // cbBlockSize must be a power of 2
    SYMCRYPT_ASSERT((cbSrc & (cbBlockSize - 1)) == 0);          // cbSrc is a multiple of cbBlockSize
    SYMCRYPT_ASSERT(cbSrc > 0);                                 // cbSrc is greaten than zero

    cbPadVal = (UINT32)pbSrc[cbSrc - 1];

    // check the Padding to make sure it is valid.
    mPaddingError |= SymCryptMask32IsZeroU31(cbPadVal) | SymCryptMask32LtU31((UINT32)cbBlockSize, cbPadVal);

    // If cbPadVal is greater than cbSrc, SYMCRYPT_INVALID_ARGUMENT will be returned
    // and cbResult will not be the right value.
    cbResult = cbSrc - cbPadVal;

    //
    // Bulk processing
    //

    cbDst = SYMCRYPT_MIN(cbDst, cbSrc);

    cbBulk = cbSrc - cbBlockSize;

    // cbSrc, cbDst, and blockSize are not secrets.
    // This condition can be checked in a non-side channel safe way.
    if (cbDst < cbBulk)
    {
        scError = SYMCRYPT_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    if (pbDst != pbSrc)
    {
        memcpy(pbDst, pbSrc, cbBulk);
    }

    // Updating parameters
    pbSrc += cbBulk; cbSrc -= cbBulk;
    pbDst += cbBulk; cbDst -= cbBulk;

    cbSrc32 = (UINT32)cbSrc;
    cbDst32 = (UINT32)cbDst;

    //
    // Validating padding
    //
    // If cbPadVal is greater than cbBlockSize,
    // we have to limit cbPadVal to be at most equal to cbBlockSize.
    cbPadVal = 1 + ((cbPadVal - 1) & (cbBlockSize - 1));
    cbMsg32 = (UINT32)(cbBlockSize - cbPadVal);

    //check Dst buffer length to make sure it is possible copy the whole message (not including the padding).
    mBufferSizeError |= SymCryptMask32LtU31(cbDst32, cbMsg32);

    //
    // Final Block processing
    //

    // Updating only the bytes of the message and leaving the other bytes in pbDst unchanged.
    // Validating the value of the padded bytes.

    for (UINT32 i = 0; i < cbBlockSize; ++i)  // cbDst <= cbSrc == cbBlockSize
    {
        mask = SymCryptMask32LtU31(i, cbMsg32);

        mPaddingError |= (SymCryptMask32IsNonzeroU31((UINT32)pbSrc[i] ^ cbPadVal) & ~mask);

        if (i < cbDst)
        {
            pbDst[i] ^= (pbDst[i] ^ pbSrc[i]) & mask;
        }
    }

cleanup:

    *pcbResult = cbResult;

    // Update scError with the two error masks.
    // SYMCRYPT_INVALID_ARGUMENT gets precedence over SYMCRYPT_BUFFER_TOO_SMALL
    scError ^= mBufferSizeError & (scError ^ SYMCRYPT_BUFFER_TOO_SMALL);
    scError ^= mPaddingError & (scError ^ SYMCRYPT_INVALID_ARGUMENT);

    return scError;
}
