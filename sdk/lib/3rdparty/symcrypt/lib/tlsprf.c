//
// tlsprf.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module contains the routines to implement the two PRF
// functions for the TLS protocols 1.1 and 1.2. These are used in
// the protocol's key derivation function.
//
//

#include "precomp.h"

//
// TLS PRF Constants
//
#define SYMCRYPT_TLS_MAX_LABEL_AND_SEED_SIZE     (SYMCRYPT_TLS_MAX_LABEL_SIZE + SYMCRYPT_TLS_MAX_SEED_SIZE)

// This **MUST** be a common multiple of MD5
// output size and SHA1 output size.
#define SYMCRYPT_TLS_1_1_CHUNK_SIZE              80

//
// SymCryptTlsPrf1_1ExpandKey is the key expansion function for versions 1.0
// and 1.1 of the TLS protocol. It takes as inputs a pointer to the expanded TLSPRF1.1
// key, and the key material in pbKey. Regarding the treatment of the key
// material (the "secret"), the following is defined in RFCs 2246 and 4346:
//
//      TLS's PRF is created by splitting the secret into two halves and
//      using one half to generate data with P_MD5 and the other half to
//      generate data with P_SHA - 1, then exclusive - or'ing the outputs of
//      these two expansion functions together.
//
//      S1 and S2 are the two halves of the secret and each is the same
//      length. S1 is taken from the first half of the secret, S2 from the
//      second half. Their length is created by rounding up the length of the
//      overall secret divided by two; thus, if the original secret is an odd
//      number of bytes long, the last byte of S1 will be the same as the
//      first byte of S2.
//
//          L_S = length in bytes of secret;
//          L_S1 = L_S2 = ceil(L_S / 2);
//
//      The secret is partitioned into two halves (with the possibility of
//      one shared byte) as described above, S1 taking the first L_S1 bytes
//      and S2 the last L_S2 bytes.
//
//  Note:   In pre-RS1 Windows if the length of the key material of each half
//          exceeded HMAC_K_PADSIZE = 64, we truncated the key. This does not comply
//          with RFC 2014 (HMAC). However, as of April 2016 several cipher suites
//          used keys (pre-master secret) longer than 128 bytes. To achieve interop
//          with servers complying to the RFC we use the entire key for the HMAC calculation.
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsPrf1_1ExpandKey(
    _Out_               PSYMCRYPT_TLSPRF1_1_EXPANDED_KEY    pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                              pbKey,
                        SIZE_T                              cbKey)
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    SIZE_T cbKeySize;
    SIZE_T cbHalfSecret;
    SIZE_T cbOdd;

    // Calculating the two halves
    cbHalfSecret = cbKey / 2;
    cbOdd = cbKey % 2;
    cbKeySize = cbHalfSecret + cbOdd;

    //
    //     The bytes of the key material are split as following:
    //     cbOdd == 0   =>   cbKeySize == cbHalfSecret
    //
    //          ********************************************
    //          <----cbHalfSecret----><----cbHalfSecret---->
    //          <----cbKeySize-------><----cbKeySize------->
    //
    //
    //     cbOdd == 1   =>   cbKeySize == cbHalfSecret + 1
    //
    //          **********************$**********************
    //          <----cbHalfSecret----> <----cbHalfSecret---->
    //          <----cbKeySize-------->
    //                                <----cbKeySize-------->
    //
    //      Note that the middle byte of the key input might be
    //      read twice (when the key length is odd). This violates
    //      the standard rule that input data should only be read
    //      once. In this case, we do this for the following reasons:
    //      -   Avoiding the dual-read is difficult; we'd have to buffer
    //          an arbitrary-size input, and SymCrypt avoids memory
    //          allocations for symmetric algorithms.
    //      -   The dual-reading of inputs is a problem when the
    //          memory is double-mapped to a different (less trusted)
    //          security context. (E.g. a kernel-mode operation on
    //          memory that is also mapped into a user address space.)
    //          This PRF is used by TLS in LSA where that situation
    //          does not occur.
    //      -   This is used for TLS 1.0 and TLS 1.1, both of which
    //          are on the deprecation path.
    //      -   In the dual-read attack, the input is typically provided
    //          by the attacker, and then changed whilst the code is
    //          accessing it. But if the attacker is providing the input,
    //          she could just as well have provided an even-length key
    //          input that provides full freedom for choosing both HMAC
    //          keys; there is simply no reason to try and perform the
    //          dual-read attack.
    //      -   Even if the dual-read problem were to occur, it does not
    //          seem to help an attacker in any way.

    // MD5 Key Expansion
    scError = SymCryptHmacMd5ExpandKey(&pExpandedKey->macMd5Key, pbKey, cbKeySize);
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // SHA1 Key Expansion
    scError = SymCryptHmacSha1ExpandKey(&pExpandedKey->macSha1Key, pbKey + cbHalfSecret, cbKeySize);
    if (scError != SYMCRYPT_NO_ERROR)
    {
        SymCryptWipeKnownSize(&pExpandedKey->macMd5Key, sizeof(pExpandedKey->macMd5Key));

        goto cleanup;
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsPrf1_2ExpandKey(
    _Out_               PSYMCRYPT_TLSPRF1_2_EXPANDED_KEY    pExpandedKey,
    _In_                PCSYMCRYPT_MAC                      macAlgorithm,
    _In_reads_(cbKey)   PCBYTE                              pbKey,
                        SIZE_T                              cbKey )
{
    SYMCRYPT_ASSERT( macAlgorithm->expandedKeySize <= sizeof( pExpandedKey->macKey ) );

    pExpandedKey->macAlg = macAlgorithm;
    return macAlgorithm->expandKeyFunc( &pExpandedKey->macKey, pbKey, cbKey );
}

//
// SymCryptTlsPrfMac uses the expanded key and hashes the concatenated
// inputs pbAi and pbSeed. It is used by all the TLS versions per
// RFCs 2246, 4346, and 5246.
// Remark:
//      - cbSeed can be 0 and pbSeed NULL.
//      - pbResult should be of size at least pMacAlgorithm->resultSize
//

VOID
SYMCRYPT_CALL
SymCryptTlsPrfMac(
    _In_                    PCSYMCRYPT_MAC              pMacAlgorithm,
    _In_                    PCSYMCRYPT_MAC_EXPANDED_KEY pMacExpandedKey,
    _In_reads_(cbAi)        PCBYTE                      pbAi,
    _In_                    SIZE_T                      cbAi,
    _In_reads_opt_(cbSeed)  PCBYTE                      pbSeed,
    _In_                    SIZE_T                      cbSeed,
    _Out_                   PBYTE                       pbResult)
{
    SYMCRYPT_MAC_STATE macState;

    pMacAlgorithm->initFunc( &macState, pMacExpandedKey );
    pMacAlgorithm->appendFunc(&macState, pbAi, cbAi);

    if (cbSeed > 0)
    {
        pMacAlgorithm->appendFunc( &macState, pbSeed, cbSeed );
    }

    pMacAlgorithm->resultFunc( &macState, pbResult );

    // No need to wipe the state. The resultFunc wipes it.
}

//
// SymCryptTlsPrfPHash is defined in RFCs 2246, 4346,
// and 5246 as follows:
//
//      First, we define a data expansion function, P_hash(secret, data)
//      which uses a single hash function to expand a secret and seed into
//      an arbitrary quantity of output:
//
//          P_hash(secret, seed) = HMAC_hash(secret, A(1) + seed) +
//                                 HMAC_hash(secret, A(2) + seed) +
//                                 HMAC_hash(secret, A(3) + seed) + ...
//
//      Where + indicates concatenation.
//      A() is defined as:
//          A(0) = seed
//          A(i) = HMAC_hash(secret, A(i-1))
//

VOID
SYMCRYPT_CALL
SymCryptTlsPrfPHash(
    _In_                        PCSYMCRYPT_MAC              pMacAlgorithm,
    _In_                        PCSYMCRYPT_MAC_EXPANDED_KEY pMacExpandedKey,
    _In_reads_(cbSeed)          PCBYTE                      pbSeed,
    _In_                        SIZE_T                      cbSeed,
    _In_reads_opt_(cbAiIn)      PCBYTE                      pbAiIn,         // Buffer for the previous Ai (used in 1.1)
    _In_                        SIZE_T                      cbAiIn,
    _Out_writes_(cbResult)      PBYTE                       pbResult,
                                SIZE_T                      cbResult,
    _Out_writes_opt_(cbAiOut)   PBYTE                       pbAiOut,        // Buffer for the next Ai (only with AiIn)
                                SIZE_T                      cbAiOut)
{
    SYMCRYPT_ALIGN BYTE    rbAi[SYMCRYPT_MAC_MAX_RESULT_SIZE];
    SYMCRYPT_ALIGN BYTE    rbPartialResult[SYMCRYPT_MAC_MAX_RESULT_SIZE];
                   BYTE *  pbTmp = pbResult;

    SIZE_T  cbMacResultSize = pMacAlgorithm->resultSize;
    SIZE_T  cbBytesToWrite = cbResult;

    if (cbAiIn == 0)
    {
        // Build A(1)
        SymCryptTlsPrfMac(
            pMacAlgorithm,
            pMacExpandedKey,
            pbSeed,         // This is A(0)
            cbSeed,
            NULL,           // No "seed" part for A(i)'s
            0,
            rbAi);
    }
    else
    {
        // Get the previous Ai
        memcpy(rbAi, pbAiIn, SYMCRYPT_MIN(SYMCRYPT_MAC_MAX_RESULT_SIZE, cbAiIn));
    }

    while (cbBytesToWrite > 0)
    {
        // Build HMAC( secret, A(i) + seed)
        SymCryptTlsPrfMac(
            pMacAlgorithm,
            pMacExpandedKey,
            rbAi,          // this is A(i)
            cbMacResultSize,
            pbSeed,             // the "seed" part
            cbSeed,
            rbPartialResult);

        // Store it in the output buffer
        memcpy(pbTmp, rbPartialResult, SYMCRYPT_MIN(cbBytesToWrite, cbMacResultSize));

        // Build A(i+1)
        SymCryptTlsPrfMac(
            pMacAlgorithm,
            pMacExpandedKey,
            rbAi,         // This is A(i)
            cbMacResultSize,
            NULL,              // No "seed" part for A(i)'s
            0,
            rbAi);

        if (cbBytesToWrite <= cbMacResultSize)
        {
            break;
        }

        pbTmp += cbMacResultSize;
        cbBytesToWrite -= cbMacResultSize;
    }

    // Store the next A(i) if needed
    if (cbAiOut > 0)
    {
        memcpy(pbAiOut, rbAi, SYMCRYPT_MIN(cbAiOut,cbMacResultSize));
    }

    SymCryptWipeKnownSize(rbAi, sizeof(rbAi));
    SymCryptWipeKnownSize(rbPartialResult, sizeof(rbPartialResult));
}


//
// The following PRF is defined in RFC 2246 and 4346:
//
//      The PRF is then defined as the result of mixing the two pseudorandom
//      streams by exclusive - or'ing them together.
//
//          PRF(secret, label, seed) =  P_MD5(S1, label + seed) XOR
//                                      P_SHA-1(S2, label + seed);
//
// Remark: We will do the do the two P_hash computations in parallel
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsPrf1_1Derive(
    _In_                    PCSYMCRYPT_TLSPRF1_1_EXPANDED_KEY   pExpandedKey,
    _In_reads_opt_(cbLabel) PCBYTE                              pbLabel,
    _In_                    SIZE_T                              cbLabel,
    _In_reads_(cbSeed)      PCBYTE                              pbSeed,
    _In_                    SIZE_T                              cbSeed,
    _Out_writes_(cbResult)  PBYTE                               pbResult,
                            SIZE_T                              cbResult)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SYMCRYPT_ALIGN BYTE    rbLabelAndSeed[SYMCRYPT_TLS_MAX_LABEL_AND_SEED_SIZE];
                   SIZE_T  cbLabelAndSeed = 0;

    SYMCRYPT_ALIGN BYTE    rbAiMd5[SYMCRYPT_HMAC_MD5_RESULT_SIZE];
    SYMCRYPT_ALIGN BYTE    rbPartialResultMd5[SYMCRYPT_TLS_1_1_CHUNK_SIZE];

    SYMCRYPT_ALIGN BYTE    rbAiSha1[SYMCRYPT_HMAC_SHA1_RESULT_SIZE];
    SYMCRYPT_ALIGN BYTE    rbPartialResultSha1[SYMCRYPT_TLS_1_1_CHUNK_SIZE];

                   BYTE *  pbTmp = pbResult;
                   SIZE_T  cbBytesToWrite = cbResult;

    // Size checks
    if ((cbLabel > SYMCRYPT_TLS_MAX_LABEL_SIZE) || (cbSeed > SYMCRYPT_TLS_MAX_SEED_SIZE))
    {
        scError = SYMCRYPT_WRONG_DATA_SIZE;
        goto cleanup;
    }

    // Concatenating the label and the seed
    pbTmp = rbLabelAndSeed;
    if( cbLabel > 0 )
    {
        memcpy(pbTmp, pbLabel, cbLabel);
        pbTmp += cbLabel;
    }
    memcpy(pbTmp, pbSeed, cbSeed);
    cbLabelAndSeed = cbLabel + cbSeed;

    // Build A(1)'s
    SymCryptTlsPrfMac(
        SymCryptHmacMd5Algorithm,
        (PCSYMCRYPT_MAC_EXPANDED_KEY)&pExpandedKey->macMd5Key,
        rbLabelAndSeed,         // This is A(0)
        cbLabelAndSeed,
        NULL,                   // No "seed" part for A(i)'s
        0,
        rbAiMd5);

    SymCryptTlsPrfMac(
        SymCryptHmacSha1Algorithm,
        (PCSYMCRYPT_MAC_EXPANDED_KEY)&pExpandedKey->macSha1Key,
        rbLabelAndSeed,         // This is A(0)
        cbLabelAndSeed,
        NULL,                   // No "seed" part for A(i)'s
        0,
        rbAiSha1);

    // Calculate the output
    pbTmp = pbResult;
    while (cbBytesToWrite > 0)
    {
        // Calculate the two P_Hashes up to SYMCRYPT_TLS_1_1_CHUNK_SIZE bytes

        // P_MD5
        SymCryptTlsPrfPHash(
            SymCryptHmacMd5Algorithm,
            (PCSYMCRYPT_MAC_EXPANDED_KEY)&pExpandedKey->macMd5Key,
            rbLabelAndSeed,
            cbLabelAndSeed,
            rbAiMd5,
            SYMCRYPT_HMAC_MD5_RESULT_SIZE,
            rbPartialResultMd5,
            SYMCRYPT_MIN(cbBytesToWrite, SYMCRYPT_TLS_1_1_CHUNK_SIZE),
            rbAiMd5,
            SYMCRYPT_HMAC_MD5_RESULT_SIZE);

        // P_SHA1
        SymCryptTlsPrfPHash(
            SymCryptHmacSha1Algorithm,
            (PCSYMCRYPT_MAC_EXPANDED_KEY)&pExpandedKey->macSha1Key,
            rbLabelAndSeed,
            cbLabelAndSeed,
            rbAiSha1,
            SYMCRYPT_HMAC_SHA1_RESULT_SIZE,
            rbPartialResultSha1,
            SYMCRYPT_MIN(cbBytesToWrite, SYMCRYPT_TLS_1_1_CHUNK_SIZE),
            rbAiSha1,
            SYMCRYPT_HMAC_SHA1_RESULT_SIZE);

        // XOR the two into the output
        SymCryptXorBytes(
            rbPartialResultMd5,
            rbPartialResultSha1,
            pbTmp,
            SYMCRYPT_MIN(cbBytesToWrite, SYMCRYPT_TLS_1_1_CHUNK_SIZE));

        if (cbBytesToWrite <= SYMCRYPT_TLS_1_1_CHUNK_SIZE)
        {
            break;
        }

        cbBytesToWrite -= SYMCRYPT_TLS_1_1_CHUNK_SIZE;
        pbTmp += SYMCRYPT_TLS_1_1_CHUNK_SIZE;

    }

cleanup:
    SymCryptWipeKnownSize(rbLabelAndSeed, sizeof(rbLabelAndSeed));
    SymCryptWipeKnownSize(rbAiMd5, sizeof(rbAiMd5));
    SymCryptWipeKnownSize(rbPartialResultMd5, sizeof(rbPartialResultMd5));
    SymCryptWipeKnownSize(rbAiSha1, sizeof(rbAiSha1));
    SymCryptWipeKnownSize(rbPartialResultSha1, sizeof(rbPartialResultSha1));

    return scError;
}

//
// The following PRF is defined in RFC 5246:
//
//      TLS's PRF is created by applying P_hash to the secret as:
//
//          PRF(secret, label, seed) = P_<hash>(secret, label + seed)
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsPrf1_2Derive(
    _In_                    PCSYMCRYPT_TLSPRF1_2_EXPANDED_KEY   pExpandedKey,
    _In_reads_opt_(cbLabel) PCBYTE                              pbLabel,
    _In_                    SIZE_T                              cbLabel,
    _In_reads_(cbSeed)      PCBYTE                              pbSeed,
    _In_                    SIZE_T                              cbSeed,
    _Out_writes_(cbResult)  PBYTE                               pbResult,
                            SIZE_T                              cbResult)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SYMCRYPT_ALIGN BYTE    rbLabelAndSeed[SYMCRYPT_TLS_MAX_LABEL_AND_SEED_SIZE];
                   BYTE *  pbTmp;

    // Size checks
    if ((cbLabel > SYMCRYPT_TLS_MAX_LABEL_SIZE) || (cbSeed > SYMCRYPT_TLS_MAX_SEED_SIZE))
    {
        scError = SYMCRYPT_WRONG_DATA_SIZE;
        goto cleanup;
    }

    // Concatenating the label and the seed
    pbTmp = rbLabelAndSeed;
    if( cbLabel > 0 )
    {
        memcpy(pbTmp, pbLabel, cbLabel);
        pbTmp += cbLabel;
    }
    memcpy(pbTmp, pbSeed, cbSeed);

    //
    // According to RFC 2104 (HMAC),  hash the secret if its length
    // exceeds the basic compression block length. This is taken
    // care by the specific HMAC inside SymCryptTlsPrfPHash.
    //
    SymCryptTlsPrfPHash(
        pExpandedKey->macAlg,
        &pExpandedKey->macKey,
        rbLabelAndSeed,
        cbLabel + cbSeed,
        NULL,
        0,
        pbResult,
        cbResult,
        NULL,
        0);

cleanup:
    SymCryptWipeKnownSize(rbLabelAndSeed, sizeof(rbLabelAndSeed));

    return scError;
}

//
// The full TLS 1.0/1.1 Key Derivation Function
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsPrf1_1(
    _In_reads_(cbKey)       PCBYTE   pbKey,
    _In_                    SIZE_T   cbKey,
    _In_reads_opt_(cbLabel) PCBYTE   pbLabel,
    _In_                    SIZE_T   cbLabel,
    _In_reads_(cbSeed)      PCBYTE   pbSeed,
    _In_                    SIZE_T   cbSeed,
    _Out_writes_(cbResult)  PBYTE    pbResult,
                            SIZE_T   cbResult)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_TLSPRF1_1_EXPANDED_KEY key;

    // Create the expanded key
    scError = SymCryptTlsPrf1_1ExpandKey(&key, pbKey, cbKey);
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Derive the key
    scError = SymCryptTlsPrf1_1Derive(
        &key,
        pbLabel,
        cbLabel,
        pbSeed,
        cbSeed,
        pbResult,
        cbResult);
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

cleanup:
    SymCryptWipeKnownSize(&key, sizeof(key));

    return scError;
}


//
// The full TLS 1.2 Key Derivation Function
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsPrf1_2(
    _In_                    PCSYMCRYPT_MAC  pMacAlgorithm,
    _In_reads_(cbKey)       PCBYTE          pbKey,
    _In_                    SIZE_T          cbKey,
    _In_reads_opt_(cbLabel) PCBYTE          pbLabel,
    _In_                    SIZE_T          cbLabel,
    _In_reads_(cbSeed)      PCBYTE          pbSeed,
    _In_                    SIZE_T          cbSeed,
    _Out_writes_(cbResult)  PBYTE           pbResult,
                            SIZE_T          cbResult)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_TLSPRF1_2_EXPANDED_KEY key;

    // Create the expanded key
    scError = SymCryptTlsPrf1_2ExpandKey(&key, pMacAlgorithm, pbKey, cbKey);
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Derive the key
    scError = SymCryptTlsPrf1_2Derive(
        &key,
        pbLabel,
        cbLabel,
        pbSeed,
        cbSeed,
        pbResult,
        cbResult);
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

cleanup:
    SymCryptWipeKnownSize(&key, sizeof(key));

    return scError;
}
