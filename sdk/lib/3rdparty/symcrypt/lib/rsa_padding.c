//
// rsa_padding.c   RSA padding algorithms
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define ASN1_SEQUENCE_BYTE      (0x30)
#define ASN1_OCTET_STRING_BYTE  (0x04)

#define PKCS_BLOCKTYPE_1        (0x01)      // This is not used, added here for completeness
#define PKCS_BLOCKTYPE_2        (0x02)

//
// Note: we could optimize these OID lists by using the same byte sequence for
// the long and short versions.
//
const SYMCRYPT_OID SymCryptMd5OidList[] =
{
    {12, (BYTE *)"\x06\x08\x2a\x86\x48\x86\xf7\x0d\x02\x05\x05\x00"},
    {10, (BYTE *)"\x06\x08\x2a\x86\x48\x86\xf7\x0d\x02\x05"},
};

const SYMCRYPT_OID SymCryptSha1OidList[] =
{
    {9, (BYTE *)"\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00"},
    {7, (BYTE *)"\x06\x05\x2b\x0e\x03\x02\x1a"}
};

const SYMCRYPT_OID SymCryptSha224OidList[] =
{
    {13, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x04\x05\x00"},
    {11, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x04"}
};

const SYMCRYPT_OID SymCryptSha256OidList[] =
{
    {13, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x01\x05\x00"},
    {11, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x01"}
};

const SYMCRYPT_OID SymCryptSha384OidList[] =
{
    {13, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x02\x05\x00"},
    {11, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x02"}
};

const SYMCRYPT_OID SymCryptSha512OidList[] =
{
    {13, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x03\x05\x00"},
    {11, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x03"}
};

const SYMCRYPT_OID SymCryptSha512_224OidList[] =
{
    {13, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x05\x05\x00"},
    {11, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x05"}
};

const SYMCRYPT_OID SymCryptSha512_256OidList[] =
{
    {13, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x06\x05\x00"},
    {11, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x06"}
};

const SYMCRYPT_OID SymCryptSha3_224OidList[] =
{
    {13, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x07\x05\x00"},
    {11, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x07"}
};

const SYMCRYPT_OID SymCryptSha3_256OidList[] =
{
    {13, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x08\x05\x00"},
    {11, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x08"}
};

const SYMCRYPT_OID SymCryptSha3_384OidList[] =
{
    {13, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x09\x05\x00"},
    {11, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x09"}
};

const SYMCRYPT_OID SymCryptSha3_512OidList[] =
{
    {13, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x0a\x05\x00"},
    {11, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x0a"}
};

const SYMCRYPT_OID SymCryptShake128OidList[] =
{
    {13, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x0b\x05\x00"},
    {11, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x0b"}
};

const SYMCRYPT_OID SymCryptShake256OidList[] =
{
    {13, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x0c\x05\x00"},
    {11, (BYTE *)"\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x0c"}
};

VOID
SYMCRYPT_CALL
SymCryptRsaPaddingMaskGeneration(
    _In_                        PCSYMCRYPT_HASH             hashAlgorithm,
    _In_                        PVOID                       pHashState,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
    _Out_writes_bytes_( cbDst ) PBYTE                       pbDst,
                                SIZE_T                      cbDst )
{
    SIZE_T              cIterations = 0;

    BYTE                rgbHash[SYMCRYPT_HASH_MAX_RESULT_SIZE] = { 0 };
    BYTE                rgbCount[sizeof(UINT32)] = { 0 };
    PBYTE               pbCount = NULL;
    SIZE_T              cbMaskRemaining = cbDst;
    PBYTE               pbMaskIndex = pbDst;

    BOOLEAN             fAvoidDWORDReverse = FALSE;

    SIZE_T              cbHashAlg = SymCryptHashResultSize( hashAlgorithm );

    cIterations = (cbDst + (cbHashAlg - 1)) / cbHashAlg;
    if (cIterations < 256)
    {
        fAvoidDWORDReverse = TRUE;
    }

    for (UINT32 i = 0; i < cIterations; i++)
    {
        SymCryptHashInit( hashAlgorithm, pHashState );

        // hash the seed
        SymCryptHashAppend( hashAlgorithm, pHashState, pbSrc, cbSrc );

        // Reverse the count bytes
        pbCount = (BYTE*)&i;
        if (fAvoidDWORDReverse)
        {
            rgbCount[3] = pbCount[0];
        }
        else
        {
            for (UINT32 j = 0; j < sizeof(UINT32); j++)
            {
                rgbCount[j] = pbCount[sizeof(UINT32) - j - 1];
            }
        }

        // hash the count
        SymCryptHashAppend( hashAlgorithm, pHashState, rgbCount, sizeof(UINT32) );

        // copy the bytes from this hash into the mask buffer
        if (cbMaskRemaining >= cbHashAlg)
        {
            SymCryptHashResult( hashAlgorithm, pHashState, pbMaskIndex, cbHashAlg );

            cbMaskRemaining -= cbHashAlg;
            pbMaskIndex += cbHashAlg;
        }
        else
        {
            SymCryptHashResult( hashAlgorithm, pHashState, rgbHash, cbHashAlg );

            memcpy( pbMaskIndex, rgbHash, cbMaskRemaining);
            break;
        }
    }
}

//
// PKCS1 Encryption Format:
//      0x00 || 0x02 || PS || 0x00 || M
//
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1ApplyEncryptionPadding(
    _In_reads_bytes_( cbPlaintext )     PCBYTE      pbPlaintext,
                                        SIZE_T      cbPlaintext,
    _Out_writes_bytes_( cbPkcs1Format ) PBYTE       pbPkcs1Format,
                                        SIZE_T      cbPkcs1Format )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    // Format: 00 02 <PS> 00 <M>
    // <PS> 8 or more padding bytes, random, all nonzero
    // <M> message, length between 0 and cbPKCS1Format - 11.
    // See RFC 3447 for more details.

    SIZE_T cbPS;
    SIZE_T i;

    // ensure output buffer is big enough (padding has 11 bytes overhead)
    if( cbPkcs1Format < (cbPlaintext + 11) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbPS = cbPkcs1Format - (cbPlaintext + 3);

    pbPkcs1Format[0] = 0x00;
    pbPkcs1Format[1] = PKCS_BLOCKTYPE_2;

    scError = SymCryptCallbackRandom( &pbPkcs1Format[2], cbPS );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Make sure that none of the bytes in PS is zero (as per specs)
    for( i = 0; i < cbPS; i++ )
    {
        while( pbPkcs1Format[2 + i] == 0x00 )
        {
            scError = SymCryptCallbackRandom( &pbPkcs1Format[2+i], 1 );
            if( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }
        }
    }

    pbPkcs1Format[2 + cbPS] = 0x00;

    memcpy(pbPkcs1Format + 3 + cbPS, pbPlaintext, cbPlaintext);

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1RemoveEncryptionPadding(
    _Inout_updates_bytes_( cbPkcs1Buffer )  PBYTE       pbPkcs1Format,
                                            SIZE_T      cbPkcs1Format,
                                            SIZE_T      cbPkcs1Buffer,
    _Out_writes_bytes_opt_( cbPlaintext )   PBYTE       pbPlaintext,
                                            SIZE_T      cbPlaintext,
    _Out_                                   SIZE_T     *pcbPlaintext )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 mPaddingError = 0;
    UINT32 mBufferSizeError = 0;

    UINT32 cbPlaintextResult = 0;
    UINT32 i;
    UINT32 mByteIsZero;
    UINT32 mLengthFound;
    UINT32 iFirstZero;
    UINT32 cbPlaintextTruncated;

    SYMCRYPT_ASSERT( cbPkcs1Buffer >= cbPkcs1Format );
    SYMCRYPT_ASSERT( cbPkcs1Buffer >= 32 );                         // Requirements for SymcryptScsRotateBuffer
    SYMCRYPT_ASSERT( (cbPkcs1Buffer & (cbPkcs1Buffer - 1)) == 0 );  // must be a power of 2
    SYMCRYPT_ASSERT( cbPkcs1Buffer <= (1 << 30 ));                  // Ensure we can use 31-bit masking operations

    // Format: 00 02 <PS> 00 <M>
    // <PS> 8 or more padding bytes, random, all nonzero
    // <M> message, length between 0 and cbPKCS1Format - 11.
    // See RFC 3447 for more details.
    // We do not reveal the buffer contents through side-channels to avoid Bleichenbacher-style attacks
    // This includes the plaintext length, which is determined by the location of the 00 byte

    if ( cbPkcs1Format < 11 )
    {
        // cbPKCS1Format is public, so the if() is safe. 11 is the total overhead
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }
    // this also implies that cbPkcs1Buffer >= 16

    // Check the leading bytes
    mPaddingError |= SymCryptMask32IsNonzeroU31( pbPkcs1Format[0] ); // First byte must be = 0
    mPaddingError |= SymCryptMask32NeqU31( pbPkcs1Format[1], PKCS_BLOCKTYPE_2 ); // Second byte must be = 2

    iFirstZero = 0;
    mLengthFound = 0;
    for (i = 2; i < cbPkcs1Format; i++)
    {
        mByteIsZero = SymCryptMask32IsZeroU31( pbPkcs1Format[i] );

        // remember the index of the first zero byte
        iFirstZero |= i & mByteIsZero & ~mLengthFound;
        mLengthFound |= mByteIsZero;
    }
    mPaddingError |= ~mLengthFound;

    // At this point:
    // - iFirstZero points to the first zero byte, or is 0 if there is no zero byte
    // - mPaddingError is set if no zero byte was found

    // It is an error if the first zero is at index < 10 as <PS> needs to be at least 8 bytes
    mPaddingError |= SymCryptMask32LtU31( iFirstZero, 10 );

    // Compute the # bytes of the message; 0 if there was a padding error
    cbPlaintextResult = ~mPaddingError & ((UINT32)(cbPkcs1Format - iFirstZero - 1));

    // We're done if the caller didn't want the actual message, but only the size.
    // We do that before checking the size of the plaintext buffer so that callers who
    // only want the size do not get an error.
    if( pbPlaintext == NULL )
    {
        // Condition is public.
        goto cleanup;
    }

    // Checking that the output buffer is large enough is a bit tricky as we have a SIZE_T as
    // buffer size, but we like to work on 31-bit integers as they have better mask algorithm perf.
    // We can truncate the SIZE_T and check for equality, which is side-channel safe.
    cbPlaintextTruncated = ((UINT32) cbPlaintext) & 0x7fffffff;                 // Truncate to 31 bits
    if( cbPlaintextTruncated == cbPlaintext )
    {
        // Condition is public as we write the whole plaintext buffer anyway.
        mBufferSizeError = SymCryptMask32LtU31( cbPlaintextTruncated, cbPlaintextResult );
    }

    // The message starts at iFirstZero + 1, which is a variable location so we can't just memcpy it without
    // revealing information through side channels.
    // Instead we rotate the buffer left (side-channel safe) so that the message appears at the front.
    // Rotation constant is such that the message appears at the start.
    SymCryptScsRotateBuffer( pbPkcs1Format, cbPkcs1Buffer, (iFirstZero + 1) & (cbPkcs1Buffer - 1) );

    // The ScsCopy function can copy the data to the destination buffer, but the input buffer must be
    // as long as the output buffer. We can't just use cbPlaintext as the output buffer size, as it is
    // unbounded. But we can limit it to cbPkcs1Format as that is the public key size and is public.
    SymCryptScsCopy( pbPkcs1Format, cbPlaintextResult, pbPlaintext, SYMCRYPT_MIN( cbPlaintext, cbPkcs1Format ) );

cleanup:
    // Update scError with the two error masks. Padding error given highest priority.
    scError ^= mBufferSizeError & (scError ^ SYMCRYPT_BUFFER_TOO_SMALL);
    scError ^= mPaddingError & (scError ^ SYMCRYPT_INVALID_ARGUMENT);

    *pcbPlaintext = cbPlaintextResult;
    return scError;
}

//
// OAEP Encryption Format:
//                             +----------+---------+-------+
//                        DB = |  lHash   |    PS   |   M   |
//                             +----------+---------+-------+
//                                            |
//                  +----------+              V
//                  |   seed   |--> MGF ---> xor
//                  +----------+              |
//                        |                   |
//               +--+     V                   |
//               |00|    xor <----- MGF <-----|
//               +--+     |                   |
//                 |      |                   |
//                 V      V                   V
//               +--+----------+----------------------------+
//         EM =  |00|maskedSeed|          maskedDB          |
//               +--+----------+----------------------------+
//
// PS = zero or more bytes 0x00 || 0x01
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaOaepApplyEncryptionPadding(
    _In_reads_bytes_( cbPlaintext )     PCBYTE          pbPlaintext,
                                        SIZE_T          cbPlaintext,
    _In_                                PCSYMCRYPT_HASH hashAlgorithm,
    _In_reads_bytes_( cbLabel )         PCBYTE          pbLabel,
                                        SIZE_T          cbLabel,
    _In_reads_bytes_opt_( cbSeed )      PCBYTE          pbSeed,
                                        SIZE_T          cbSeed,
    _Out_writes_bytes_( cbOaepFormat )  PBYTE           pbOaepFormat,
                                        SIZE_T          cbOaepFormat,
    _Out_writes_bytes_( cbScratch )     PBYTE           pbScratch,
                                        SIZE_T          cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PVOID   pHashState;

    PBYTE   pbSeedInternal;
    PBYTE   pbSeedMask;
    PBYTE   pbDB;
    PBYTE   pbDBMask;

    SIZE_T  cbDB;
    SIZE_T  cbPS;

    SIZE_T  cbHash = SymCryptHashResultSize( hashAlgorithm );
    SIZE_T  cbHashState = SymCryptHashStateSize( hashAlgorithm );

    UNREFERENCED_PARAMETER( cbScratch );

    // OAEP overhead is 2 + 2 * size of hash result
    if(  cbOaepFormat < (cbPlaintext + (cbHash * 2) + 2) ||
         ((pbSeed!=NULL) && (cbSeed>cbHash)) ||
         ((pbSeed==NULL) && (cbSeed!=0)) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbPS = cbOaepFormat - (cbPlaintext + (cbHash * 2) + 2);
    cbDB = cbOaepFormat - (cbHash + 1);

    SYMCRYPT_ASSERT( cbScratch >= cbHashState + (cbHash * 2) + (cbDB * 2) );

    pHashState = (PVOID) pbScratch;
    pbSeedInternal = pbScratch + cbHashState;
    pbSeedMask = pbSeedInternal + cbHash;
    pbDB = pbSeedMask + cbHash;
    pbDBMask = pbDB + cbDB;

    // hash the label
    SymCryptHash( hashAlgorithm, pbLabel, cbLabel, pbDB, cbHash );

    SymCryptWipe(pbDB + cbHash, cbPS);
    pbDB[cbHash + cbPS] = 0x01;

	// dcl - are we quite sure that none of these numbers are under attacker control?
    memcpy(pbDB + cbHash + cbPS + 1, pbPlaintext, cbPlaintext);

    if (NULL == pbSeed)
    {
        // generate the random seed (same length as the hash result)
        scError = SymCryptCallbackRandom( pbSeedInternal, cbHash );
        if (scError != SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }
    }
    else
    {
        SymCryptWipe( pbSeedInternal, cbHash );
        memcpy(pbSeedInternal, pbSeed, cbSeed);
    }

    // MGF(seed)
    SymCryptRsaPaddingMaskGeneration(
                            hashAlgorithm,
                            pHashState,
                            pbSeedInternal,
                            cbHash,
                            pbDBMask,
                            cbDB);

    // set the most significant byte to 0x00
    pbOaepFormat[0] = 0x00;

    // XOR the DB and the mask MGF(seed)
    for (UINT32 i = 0; i < cbDB; i++)
    {
        pbOaepFormat[cbHash + 1 + i] = pbDB[i] ^ pbDBMask[i];
    }

    // MGF(masked DB)
    SymCryptRsaPaddingMaskGeneration(
                            hashAlgorithm,
                            pHashState,
                            pbOaepFormat + cbHash + 1,
                            cbDB,
                            pbSeedMask,
                            cbHash);

    // XOR the seed and the seed mask MGF(masked DB)
    for (UINT32 i = 0; i < cbHash; i++)
    {
        pbOaepFormat[1 + i] = pbSeedInternal[i] ^ pbSeedMask[i];
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaOaepRemoveEncryptionPadding(
    _In_reads_bytes_( cbOAEPFormat )
                                PCBYTE                      pbOAEPFormat,
                                SIZE_T                      cbOAEPFormat,
    _In_                        PCSYMCRYPT_HASH             hashAlgorithm,
    _In_reads_bytes_( cbLabel ) PCBYTE                      pbLabel,
                                SIZE_T                      cbLabel,
                                UINT32                      flags,
    _Out_writes_bytes_( cbPlaintext )
                                PBYTE                       pbPlaintext,
                                SIZE_T                      cbPlaintext,
    _Out_                       SIZE_T                      *pcbPlaintext,
    _Out_writes_bytes_( cbScratch )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PVOID   pHashState;

    PBYTE   pbSeedMask;
    PBYTE   pbSeed;
    PBYTE   pbDBMask;
    PBYTE   pbDB;
    PBYTE   pbLabelHash;
    UINT32  mPaddingError;

    SIZE_T  cbDB;

    SIZE_T  cnt = 0;

    SIZE_T  cbHashAlg = SymCryptHashResultSize( hashAlgorithm );
    SIZE_T  cbHashState = SymCryptHashStateSize( hashAlgorithm );

    UNREFERENCED_PARAMETER( cbScratch );

    if (flags != 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // check if the most significant byte is set to 0x00
    mPaddingError = SymCryptMask32IsNonzeroU31( pbOAEPFormat[0] );

    // Padding overhead is 2 hash values plus 2 bytes
    if( cbOAEPFormat < (2*cbHashAlg + 2) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbDB = cbOAEPFormat - (cbHashAlg + 1);

    SYMCRYPT_ASSERT( cbScratch >= cbHashState + (cbHashAlg * 3) + (cbDB * 2) );

    pHashState = (PVOID) pbScratch;
    pbSeedMask = pbScratch + cbHashState;
    pbSeed = pbSeedMask + cbHashAlg;
    pbDBMask = pbSeed + cbHashAlg;
    pbDB = pbDBMask + cbDB;
    pbLabelHash = pbDB + cbDB;

    // MGF(masked DB)
    SymCryptRsaPaddingMaskGeneration(
                            hashAlgorithm,
                            pHashState,
                            pbOAEPFormat + cbHashAlg + 1,
                            cbDB,
                            pbSeedMask,
                            cbHashAlg);

    // XOR the masked seed and the seed mask MGF(masked DB)
    for (UINT32 i = 0; i < cbHashAlg; i++)
    {
        pbSeed[i] = pbOAEPFormat[1 + i] ^ pbSeedMask[i];
    }

    // MGF(seed)
    SymCryptRsaPaddingMaskGeneration(
                            hashAlgorithm,
                            pHashState,
                            pbSeed,
                            cbHashAlg,
                            pbDBMask,
                            cbDB);

    // XOR the masked DB and the mask MGF(seed)
    for (UINT32 i = 0; i < cbDB; i++)
    {
        pbDB[i] = pbOAEPFormat[cbHashAlg + 1 + i] ^ pbDBMask[i];
    }

    // hash the label
    SymCryptHash( hashAlgorithm, pbLabel, cbLabel, pbLabelHash, cbHashAlg );

    // check the label hash
    mPaddingError |= SymCryptMask32IsZeroU31( SymCryptEqual( pbLabelHash, pbDB, cbHashAlg ) );

    //
    // At this point we have verified the leading 0 byte and the label hash, with any
    // errors in mPaddingError. We could continue to make the entire padding removal
    // side-channel safe like we do in the PKCS1 padding case, but that is not necessary.
    // The side-channel only leaks data if the attacker can trigger two different behaviours
    // and derive information from the difference.
    // This is relatively easy to do with something like a match on 1 or 2 bytes because the
    // chance of satisfying the check on a random input is still useful. But here we have
    // matched 33 bytes (assuming a 32-byte hash) and the Bleichenbacher style attacks don't
    // work beyond this point. Basically, these attacks produce ciphertexts without knowing
    // the corresponding plaintext, and the chance of the label hash matching is something
    // like 2^{-256}. So these ciphertexts will always fail right here, and there is no
    // difference of behaviour that leaks data to the attacker.
    // Thus, we can switch back to normal processing of the errors here.
    //

    if( mPaddingError != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // check the PS
    for (cnt = cbHashAlg; cnt < cbDB; cnt++)
    {
        if (pbDB[cnt] == 0x01)
        {
            cnt++;
            break;
        }
        else if (pbDB[cnt] != 0x00)
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }
    }

    if (pbDB[cnt - 1] != 0x01)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // the rest is data
    *pcbPlaintext = cbDB - cnt;

    if(NULL == pbPlaintext)
    {
        scError = SYMCRYPT_NO_ERROR;
        goto cleanup;
    }

    if (cbPlaintext < *pcbPlaintext)
    {
        scError = SYMCRYPT_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    memcpy(pbPlaintext, pbDB + cnt, *pcbPlaintext);

    scError = SYMCRYPT_NO_ERROR;

cleanup:

    return scError;
}

//
// PKCS1 Signature Format:
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1ApplySignaturePadding(
    _In_reads_bytes_( cbHash )  PCBYTE                      pbHash,
                                SIZE_T                      cbHash,
    _In_reads_bytes_( cbHashOid )
                                PCBYTE                      pbHashOid,
                                SIZE_T                      cbHashOid,
                                UINT32                      flags,
    _Out_writes_bytes_( cbPKCS1Format )
                                PBYTE                       pbPKCS1Format,
                                SIZE_T                      cbPKCS1Format )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SIZE_T cbEncoding;
    SIZE_T cbPadding;
    SIZE_T cbOidOffset;

    BOOLEAN fInsertASN1 = TRUE;

    if ((flags & ~SYMCRYPT_FLAG_RSA_PKCS1_NO_ASN1) != 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Simple check to avoid funky behavior if cbHash is close to SIZE_MAX
    if (cbHash >= cbPKCS1Format)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    fInsertASN1 = ((flags & SYMCRYPT_FLAG_RSA_PKCS1_NO_ASN1) == 0);

    if (fInsertASN1)
    {
        if ( (pbHashOid!=NULL) && (cbHashOid>0) )
        {
            // determine the length of the ASN1 Encoding
            // 2 sequence bytes, 1 id byte and 3 length bytes
            cbEncoding = 6 + cbHashOid + cbHash;
        }
        else
        {
            if (cbHashOid > 0)
            {
                // The caller has passed a NULL hash and a non 0 size for it.
                // We can't guess the intent, hence we fail
                scError = SYMCRYPT_INVALID_ARGUMENT;
                goto cleanup;
            }

            // special case for MD5 hash without OID
            cbEncoding = 2 + cbHash;
        }

        // we don't support encodings longer than 128 bytes,
        // with this check we know that the length of the OID as
        // well as the length of the hash value will each fit in
        // one byte
        if (cbEncoding > 0x80)
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }
    }
    else
    {
        cbEncoding = cbHash;
    }

    // In a few scenarios (involving small RSA keys), the new large SHA
    // hashes are too big to be signed by the specified key.
    // There must be at least 8 bytes of 0xff.
    if (3 + 8 + cbEncoding > cbPKCS1Format)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbPadding = cbPKCS1Format - 3 - cbEncoding;


    // insert the block type and delimiters
    pbPKCS1Format[0] = 0x00;
    pbPKCS1Format[1] = 0x01;
    pbPKCS1Format[2 + cbPadding] = 0x00;

    // insert the type 1 padding
    memset(pbPKCS1Format + 2, 0xff, cbPadding);

    if (fInsertASN1)
    {
        cbOidOffset = 1;
        if ( (pbHashOid!=NULL) && (cbHashOid>0) )
        {
            // insert the algorithm encoding
            pbPKCS1Format[2 + cbPadding + 1] = ASN1_SEQUENCE_BYTE;
            pbPKCS1Format[2 + cbPadding + 2] = (BYTE)cbEncoding - 2;

            // insert the sequence string byte, length of the hash and the hash value
            pbPKCS1Format[2 + cbPadding + 3] = ASN1_SEQUENCE_BYTE;
            pbPKCS1Format[2 + cbPadding + 4] = (BYTE)cbHashOid;
            cbOidOffset += 4;
            memcpy(pbPKCS1Format + 2 + cbPadding + cbOidOffset, pbHashOid, cbHashOid);
        }

        // insert the octet string byte, length of the hash and the hash value
        pbPKCS1Format[2 + cbPadding + cbOidOffset + cbHashOid] = ASN1_OCTET_STRING_BYTE;
        pbPKCS1Format[2 + cbPadding + cbOidOffset + cbHashOid + 1] = (BYTE)cbHash;
        memcpy(pbPKCS1Format + 2 + cbPadding + cbOidOffset + cbHashOid + 2, pbHash, cbHash);
    }
    else
    {
        memcpy(pbPKCS1Format + 3 + cbPadding, pbHash, cbHash);
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:

    return scError;
}

//
// Check if a PKCS1 padding is valid with regard to a hash oid
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1CheckSignaturePadding(
    _In_reads_bytes_( cbHash )  PCBYTE                      pbHash,
                                SIZE_T                      cbHash,
    _In_reads_bytes_( cbHashOid )
                                PCBYTE                      pbHashOid,
                                SIZE_T                      cbHashOid,
    _In_reads_bytes_( cbPKCS1Format )
                                PCBYTE                      pbPKCS1Format,
                                UINT32                      flags,
    _Out_writes_bytes_( cbPKCS1Format )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbPKCS1Format)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SymCryptWipe(pbScratch, cbPKCS1Format);

    scError = SymCryptRsaPkcs1ApplySignaturePadding(
                      pbHash,
                      cbHash,
                      pbHashOid,
                      cbHashOid,
                      flags,
                      pbScratch,
                      cbPKCS1Format );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    if ( SymCryptEqual(pbScratch, pbPKCS1Format, cbPKCS1Format) )
    {
        scError = SYMCRYPT_NO_ERROR;
    }
    else
    {
        scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
    }

cleanup:

    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1VerifySignaturePadding(
    _In_reads_bytes_( cbHash )  PCBYTE                      pbHash,
                                SIZE_T                      cbHash,
    _In_reads_( nOIDCount )     PCSYMCRYPT_OID              pHashOIDs,
    _In_                        SIZE_T                      nOIDCount,
    _In_reads_bytes_( cbPKCS1Format )
                                PCBYTE                      pbPKCS1Format,
                                SIZE_T                      cbPKCS1Format,
                                UINT32                      flags,
    _Out_writes_bytes_( cbScratch )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 i = 0;

    UNREFERENCED_PARAMETER( cbScratch );
    SYMCRYPT_ASSERT( cbScratch >= cbPKCS1Format );

    if ((flags & ~SYMCRYPT_FLAG_RSA_PKCS1_OPTIONAL_HASH_OID) != 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    //
    // Verify padding and the hash value
    //
    if (pHashOIDs)
    {
        for (i = 0; i < nOIDCount; i++)
        {
            scError = SymCryptRsaPkcs1CheckSignaturePadding(
                              pbHash,
                              cbHash,
                              pHashOIDs[i].pbOID,
                              pHashOIDs[i].cbOID,
                              pbPKCS1Format,
                              0,
                              pbScratch,
                              cbPKCS1Format );
            if (scError == SYMCRYPT_NO_ERROR)
            {
                break;
            }
        }
    }

    if  ((pHashOIDs == NULL ) ||
         (scError != SYMCRYPT_NO_ERROR &&
          flags & SYMCRYPT_FLAG_RSA_PKCS1_OPTIONAL_HASH_OID))
    {
        // if no OID is passed in, or
        // OID is passed in but failed verification, but OID is optional
        scError = SymCryptRsaPkcs1CheckSignaturePadding(
                          pbHash,
                          cbHash,
                          NULL,
                          0,
                          pbPKCS1Format,
                          SYMCRYPT_FLAG_RSA_PKCS1_NO_ASN1,
                          pbScratch,
                          cbPKCS1Format );
    }

cleanup:

    return scError;
}

//
// PSS Signature Format:
//                        +--------+----------+----------+
//                   M' = |Padding1|  Hash M  |   salt   |
//                        +--------+----------+----------+
//                                       |
//             +--------+----------+     V
//       DB =  |Padding2|   salt   |   Hash
//             +--------+----------+     |
//                       |               |
//                       V               |    +--+
//                      xor <--- MGF <---|    |bc|
//                       |               |    +--+
//                       |               |      |
//                       V               V      V
//             +-------------------+----------+--+
//       EM =  |    maskedDB       |     H    |bc|
//             +-------------------+----------+--+
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPssApplySignaturePadding(
    _In_reads_bytes_( cbHash )  PCBYTE                      pbHash,
                                SIZE_T                      cbHash,
    _In_                        PCSYMCRYPT_HASH             hashAlgorithm,
    _In_reads_bytes_opt_( cbSalt )
                                PCBYTE                      pbSalt,
    _In_range_(0, cbPSSFormat)  SIZE_T                      cbSalt,
                                UINT32                      nBitsOfModulus,
                                UINT32                      flags,
    _Out_writes_bytes_( cbPSSFormat )
                                PBYTE                       pbPSSFormat,
                                SIZE_T                      cbPSSFormat,
    _Out_writes_bytes_( cbScratch )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbScratch )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    PVOID   pHashState;

    PBYTE pbMPrime;
    PBYTE pbDB;
    PBYTE pbDBMask;

    SIZE_T cbDB;
    SIZE_T cbMPrime;
    SIZE_T cbPadding2;

    SIZE_T dwZeroBits  = 0; // Number of bits of the leftmost bit to be zeroed

    SIZE_T cbHashAlg = SymCryptHashResultSize( hashAlgorithm );
    SIZE_T cbHashState = SymCryptHashStateSize( hashAlgorithm );

    UNREFERENCED_PARAMETER( cbScratch );

    if ((cbPSSFormat == 0) || (pbPSSFormat == NULL))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Corner case of RFC 3447 for PSS:
    //  If nBitsOfModulus == 1 mod 8, then emBits = nBitsOfModulus - 1 == 0 mod 8
    //  Thus the size of the input buffer in bytes is emLen = ceil(emBits /8),
    //  one smaller than the size of the modulus. Fix this here by setting the
    //  leftmost byte of the output equal to 0.
    if (nBitsOfModulus%8 == 1)
    {
        pbPSSFormat[0] = 0;
        pbPSSFormat++;
        cbPSSFormat--;
    }

    if ((flags!=0) ||
        (cbPSSFormat < (cbHashAlg + cbSalt + 2)) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbDB = cbPSSFormat - (cbHashAlg + 1);
    cbPadding2 = cbDB - cbSalt - 1;
    cbMPrime = 8 + cbHash + cbSalt;

    SYMCRYPT_ASSERT( cbScratch >= cbHashState + cbMPrime + (cbDB * 2) );

    pHashState = (PVOID) pbScratch;
    pbMPrime = pbScratch + cbHashState;
    pbDB = pbMPrime + cbMPrime;
    pbDBMask = pbDB + cbDB;

    // set up the M Prime
    SymCryptWipe(pbMPrime, 8);
    memcpy(pbMPrime + 8, pbHash, cbHash);

    if (NULL == pbSalt)
    {
        // generate the random salt
        scError = SymCryptCallbackRandom(
                            pbMPrime + 8 + cbHash,
                            cbSalt);
        if (scError != SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }
    }
    else
    {
        // copy the salt passed
        memcpy(pbMPrime + 8 + cbHash, pbSalt, cbSalt);
    }

    // hash the MPrime
    SymCryptHash( hashAlgorithm, pbMPrime, cbMPrime, pbPSSFormat + cbDB, cbHashAlg );

    // copy the same salt into the DB
    SymCryptWipe(pbDB, cbPadding2);
    pbDB[cbPadding2] = 0x01;
    memcpy(pbDB + cbPadding2 + 1, pbMPrime + 8 + cbHash, cbSalt);

    // MGF(Hash of MPrime)
    SymCryptRsaPaddingMaskGeneration(
                            hashAlgorithm,
                            pHashState,
                            pbPSSFormat + cbDB,
                            cbHashAlg,
                            pbDBMask,
                            cbDB);

    // XOR the DB and the mask MGF(seed)
    for (UINT32 i = 0; i < cbDB; i++)
    {
        pbPSSFormat[i] = pbDB[i] ^ pbDBMask[i];
    }

    // calculate the number of bits to be zeroed
    dwZeroBits = 8*cbPSSFormat + 1 - nBitsOfModulus;

    // mask off dwZeroBits worth of the encoded message
    pbPSSFormat[0] &= (BYTE)(0xff >> dwZeroBits);

    // set the least significant byte of pbPSSFormat to bc
    pbPSSFormat[cbPSSFormat - 1] = 0xbc;

    scError = SYMCRYPT_NO_ERROR;

cleanup:

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPssVerifySignaturePadding(
    _In_reads_bytes_( cbHash )  PCBYTE                      pbHash,
                                SIZE_T                      cbHash,
    _In_                        PCSYMCRYPT_HASH             hashAlgorithm,
    _In_range_(0, cbPSSFormat)  SIZE_T                      cbSalt,
    _In_reads_bytes_( cbPSSFormat )
                                PCBYTE                      pbPSSFormat,
                                SIZE_T                      cbPSSFormat,
                                UINT32                      nBitsOfModulus,
                                UINT32                      flags,
    _Out_writes_bytes_( cbScratch )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbScratch )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    PVOID   pHashState;

    PBYTE pbDBMask;
    PBYTE pbMPrime;
    PBYTE pbMPrimeHash;
    PCBYTE pbHashOfMPrimeIndex;

    SIZE_T cbDB;
    SIZE_T cbMPrime;
    SIZE_T cbPadding2;
    SIZE_T cbSaltObserved;

    SIZE_T dwZeroBits  = 0; // Number of bits of the leftmost bit to be zeroed

    SIZE_T cbHashAlg = SymCryptHashResultSize( hashAlgorithm );
    SIZE_T cbHashState = SymCryptHashStateSize( hashAlgorithm );

    UNREFERENCED_PARAMETER( cbScratch );

    if (((flags & ~SYMCRYPT_FLAG_RSA_PSS_VERIFY_WITH_MINIMUM_SALT) != 0) ||
        (cbPSSFormat == 0) ||
        (pbPSSFormat == NULL))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Corner case of RFC 3447 for PSS:
    //  If nBitsOfModulus == 1 mod 8, then emBits = nBitsOfModulus - 1 == 0 mod 8
    //  Thus the size of the input buffer in bytes is emLen = ceil(emBits /8),
    //  one smaller than the size of the modulus. Fix this here by checking that the
    //  leftmost byte of the input equals 0.
    if (nBitsOfModulus%8 == 1)
    {
        if (pbPSSFormat[0] != 0)
        {
            scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
            goto cleanup;
        }
        pbPSSFormat++;
        cbPSSFormat--;
    }

    // calculate the number of bits to be zeroed
    dwZeroBits = 8*cbPSSFormat + 1 - nBitsOfModulus;

    // check the most significant dwZeroBits bits to ensure they're zero and
    // check the least significant byte
    if( (cbPSSFormat < (cbHashAlg + cbSalt + 2)) ||
        (pbPSSFormat[0] & (BYTE)(0xff << (8 - dwZeroBits))) != 0 ||
        pbPSSFormat[cbPSSFormat - 1] != 0xbc
        )
    {
        scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
        goto cleanup;
    }

    cbDB = cbPSSFormat - (cbHashAlg + 1);

    pHashState = (PVOID) pbScratch;
    pbDBMask = pbScratch + cbHashState;

    // index to hash of M Prime
    pbHashOfMPrimeIndex = pbPSSFormat + (cbPSSFormat - (cbHashAlg + 1));

    // MGF(masked DB)
    SymCryptRsaPaddingMaskGeneration(
            hashAlgorithm,
            pHashState,
            pbHashOfMPrimeIndex,
            cbHashAlg,
            pbDBMask,
            cbDB);

    // XOR the DB and the DB mask and store the result in pbDBMask (not needed after this)
    for (UINT32 i = 0; i < cbDB; i++)
    {
        pbDBMask[i] = pbPSSFormat[i] ^ pbDBMask[i];
    }

    // mask off the first dwZeroBits
    pbDBMask[0] &= (BYTE)(0xff >> dwZeroBits);

    // find the length of the all-zeroes padding2 in pbDBMask
    // padding2 must be terminated by a 0x01 byte
    for (cbPadding2 = 0; cbPadding2 < (cbDB - cbSalt); cbPadding2++)
    {
        if (pbDBMask[cbPadding2] == 0x01)
        {
            // we have reached the end of padding2
            break;
        }

        if (pbDBMask[cbPadding2] != 0x00)
        {
            // non-zero byte in what should be padding2
            scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
            goto cleanup;
        }
    }

    // Here we have either:
    // cbPadding2 == cbDB - cbSalt, which means the padding is too long
    // or
    // cbPadding2 <= cbDB - cbSalt - 1, and we have broken out of the loop when we found the 0x01 byte
    if( cbPadding2 == cbDB - cbSalt )
    {
        scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
        goto cleanup;
    }

    cbSaltObserved = cbDB - cbPadding2 - 1;
    // cbSalt <= cbDB - cbPadding2 - 1 = cbSaltObserved
    // so cbSaltObserved is acceptable value for signature verification
    // with SYMCRYPT_FLAG_RSA_PSS_VERIFY_WITH_MINIMUM_SALT

    if( ((flags & SYMCRYPT_FLAG_RSA_PSS_VERIFY_WITH_MINIMUM_SALT) == 0) &&
        cbSaltObserved != cbSalt )
    {
        // When SYMCRYPT_FLAG_RSA_PSS_VERIFY_WITH_MINIMUM_SALT not specified,
        // we require salt length observed to exactly match the caller provided salt length
        scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
        goto cleanup;
    }

    pbMPrime = pbDBMask + cbDB;
    cbMPrime = 8 + cbHash + cbSaltObserved;
    pbMPrimeHash = pbMPrime + cbMPrime;

    SYMCRYPT_ASSERT( cbScratch >= cbHashState + cbDB + cbMPrime + cbHashAlg );

    // create the M Prime
    SymCryptWipe(pbMPrime, 8);
    memcpy(pbMPrime + 8, pbHash, cbHash);
    memcpy(pbMPrime + 8 + cbHash,
           pbDBMask + (cbDB - cbSaltObserved),
           cbSaltObserved);

    // hash the M Prime
    SymCryptHash( hashAlgorithm, pbMPrime, cbMPrime, pbMPrimeHash, cbHashAlg );

    if ( !SymCryptEqual(pbPSSFormat + cbDB, pbMPrimeHash, cbHashAlg) )
    {
        scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
        goto cleanup;
    }

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    return scError;
}
