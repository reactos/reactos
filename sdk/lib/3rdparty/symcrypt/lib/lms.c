//
// lms.c LMS implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//
static const PCSYMCRYPT_HASH* LmsHashObjects[] = {
    &SymCryptSha256Algorithm,       //  0
    &SymCryptShake256HashAlgorithm, //  1
};

typedef struct _SYMCRYPT_LMS_PARAMETER_PREDEFINED
{
    SYMCRYPT_LMS_ALGID  lmsAlgId;

    // output length
    UINT8               cbHashOutput;

    // total tree height
    UINT8               nTreeHeight;

    // hash function index
    UINT8               nHashIdx;

} SYMCRYPT_LMS_PARAMETER_PREDEFINED, * PSYMCRYPT_LMS_PARAMETER_PREDEFINED;

typedef const SYMCRYPT_LMS_PARAMETER_PREDEFINED* PCSYMCRYPT_LMS_PARAMETER_PREDEFINED;


static const SYMCRYPT_LMS_PARAMETER_PREDEFINED LmsParametersPredefined[] = {

    //  algId                             m   h    HIdx
    {   SYMCRYPT_LMS_SHA256_M32_H5,       32, 5 ,    0    },
    {   SYMCRYPT_LMS_SHA256_M32_H10,      32, 10,    0    },
    {   SYMCRYPT_LMS_SHA256_M32_H15,      32, 15,    0    },
    {   SYMCRYPT_LMS_SHA256_M32_H20,      32, 20,    0    },
    {   SYMCRYPT_LMS_SHA256_M32_H25,      32, 25,    0    },
    {   SYMCRYPT_LMS_SHAKE_M32_H5,        32, 5 ,    1    },
    {   SYMCRYPT_LMS_SHAKE_M32_H10,       32, 10,    1    },
    {   SYMCRYPT_LMS_SHAKE_M32_H15,       32, 15,    1    },
    {   SYMCRYPT_LMS_SHAKE_M32_H20,       32, 20,    1    },
    {   SYMCRYPT_LMS_SHAKE_M32_H25,       32, 25,    1    },
    {   SYMCRYPT_LMS_SHA256_M24_H5,       24, 5 ,    0    },
    {   SYMCRYPT_LMS_SHA256_M24_H10,      24, 10,    0    },
    {   SYMCRYPT_LMS_SHA256_M24_H15,      24, 15,    0    },
    {   SYMCRYPT_LMS_SHA256_M24_H20,      24, 20,    0    },
    {   SYMCRYPT_LMS_SHA256_M24_H25,      24, 25,    0    },
    {   SYMCRYPT_LMS_SHAKE_M24_H5,        24, 5 ,    1    },
    {   SYMCRYPT_LMS_SHAKE_M24_H10,       24, 10,    1    },
    {   SYMCRYPT_LMS_SHAKE_M24_H15,       24, 15,    1    },
    {   SYMCRYPT_LMS_SHAKE_M24_H20,       24, 20,    1    },
    {   SYMCRYPT_LMS_SHAKE_M24_H25,       24, 25,    1    },
};

typedef struct _SYMCRYPT_LMS_OTS_PARAMETER_PREDEFINED
{
    SYMCRYPT_LMS_OTS_ALGID  lmsOtsAlgId;

    // output length
    UINT8                   cbHashOutput;

    // Winternitz width
    UINT8                   nWidth;

    // hash function index
    UINT8                   nHashIdx;

} SYMCRYPT_LMS_OTS_PARAMETER_PREDEFINED, * PSYMCRYPT_LMS_OTS_PARAMETER_PREDEFINED;
typedef const SYMCRYPT_LMS_OTS_PARAMETER_PREDEFINED* PCSYMCRYPT_LMS_OTS_PARAMETER_PREDEFINED;

static const SYMCRYPT_LMS_OTS_PARAMETER_PREDEFINED LmsOtsParametersPredefined[] = {

    //  algId                             n   w  HIdx
    {   SYMCRYPT_LMS_OTS_SHA256_N32_W1,   32, 1,  0   },
    {   SYMCRYPT_LMS_OTS_SHA256_N32_W2,   32, 2,  0   },
    {   SYMCRYPT_LMS_OTS_SHA256_N32_W4,   32, 4,  0   },
    {   SYMCRYPT_LMS_OTS_SHA256_N32_W8,   32, 8,  0   },
    {   SYMCRYPT_LMS_OTS_SHAKE_N32_W1,    32, 1,  1   },
    {   SYMCRYPT_LMS_OTS_SHAKE_N32_W2,    32, 2,  1   },
    {   SYMCRYPT_LMS_OTS_SHAKE_N32_W4,    32, 4,  1   },
    {   SYMCRYPT_LMS_OTS_SHAKE_N32_W8,    32, 8,  1   },
    {   SYMCRYPT_LMS_OTS_SHA256_N24_W1,   24, 1,  0   },
    {   SYMCRYPT_LMS_OTS_SHA256_N24_W2,   24, 2,  0   },
    {   SYMCRYPT_LMS_OTS_SHA256_N24_W4,   24, 4,  0   },
    {   SYMCRYPT_LMS_OTS_SHA256_N24_W8,   24, 8,  0   },
    {   SYMCRYPT_LMS_OTS_SHAKE_N24_W1,    24, 1,  1   },
    {   SYMCRYPT_LMS_OTS_SHAKE_N24_W2,    24, 2,  1   },
    {   SYMCRYPT_LMS_OTS_SHAKE_N24_W4,    24, 4,  1   },
    {   SYMCRYPT_LMS_OTS_SHAKE_N24_W8,    24, 8,  1   },
};
static const BYTE SYMCRYPT_LMS_D_PBLC[] = { 0x80, 0x80 };
static const BYTE SYMCRYPT_LMS_D_MESG[] = { 0x81, 0x81 };
static const BYTE SYMCRYPT_LMS_D_LEAF[] = { 0x82, 0x82 };
static const BYTE SYMCRYPT_LMS_D_INTR[] = { 0x83, 0x83 };

static
VOID
LmsHashMessage(
    _In_                                                    PCSYMCRYPT_HASH pHash,
    _In_reads_bytes_(SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE) PCBYTE          pbId,
    _In_reads_bytes_(sizeof(UINT32))                        PCBYTE          pbLeafNumber,
    _In_reads_bytes_(cbRandomizer)                          PCBYTE          pbRandomizer,
                                                            SIZE_T          cbRandomizer,
    _In_reads_bytes_(cbMessage)                             PCBYTE          pbMessage,
                                                            SIZE_T          cbMessage,
    _Out_writes_bytes_(cbOut)                               PBYTE           pbOut,
                                                            SIZE_T          cbOut)
{
    SYMCRYPT_HASH_STATE state   = { 0 };

    SymCryptHashInit(pHash, &state);
    SymCryptHashAppend(pHash, &state, pbId, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
    SymCryptHashAppend(pHash, &state, pbLeafNumber, sizeof(UINT32));
    SymCryptHashAppend(pHash, &state, SYMCRYPT_LMS_D_MESG, sizeof(SYMCRYPT_LMS_D_MESG));
    SymCryptHashAppend(pHash, &state, pbRandomizer, cbRandomizer);
    SymCryptHashAppend(pHash, &state, pbMessage, cbMessage);
    SymCryptHashResult(pHash, &state, pbOut, cbOut);
}

static
VOID
SYMCRYPT_CALL
LmsOtskeyComputePrivate(
    _In_    PCSYMCRYPT_LMS_KEY  pKey,
    _In_    UINT32              nLeafNumber,
    _In_    UINT32              nPIdx,
    _Out_writes_bytes_(pKey->params.cbHashOutput)
            PBYTE               pbOtsPrivateKey)
{
    UINT32              cbHashOutput                = pKey->params.cbHashOutput;
    PCSYMCRYPT_HASH     pHash                       = pKey->params.pLmsHashFunction;
    SYMCRYPT_HASH_STATE state                       = { 0 };
    BYTE                abTemp[sizeof(UINT32) + 3]  = { 0 }; // sizeof(UINT32) for nLeafNumber, 2 bytes of nPIdx and 1 byte of 0xff

    SYMCRYPT_ASSERT(nLeafNumber <= (((UINT32)1 << pKey->params.nTreeHeight) - 1));

    SYMCRYPT_STORE_MSBFIRST32(abTemp, nLeafNumber);
    SYMCRYPT_STORE_MSBFIRST16(abTemp + sizeof(UINT32), (UINT16)nPIdx);
    abTemp[sizeof(UINT32) + 2] = 0xff;

    SymCryptHashInit(pHash, &state);
    SymCryptHashAppend(pHash, &state, pKey->abId, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
    SymCryptHashAppend(pHash, &state, abTemp, sizeof(abTemp));
    SymCryptHashAppend(pHash, &state, pKey->abSeed, cbHashOutput);
    SymCryptHashResult(pHash, &state, pbOtsPrivateKey, cbHashOutput);
}

static
VOID
SYMCRYPT_CALL
LmskeyWipe(
    _Inout_ PSYMCRYPT_LMS_KEY pKey)
{
    SYMCRYPT_CHECK_MAGIC(pKey);

    SymCryptWipeKnownSize(pKey->abSeed, sizeof(pKey->abSeed));
    SymCryptWipeKnownSize(pKey->abPublicRoot, sizeof(pKey->abPublicRoot));
    SymCryptWipeKnownSize(pKey->abId, sizeof(pKey->abId));
    pKey->nNextUnusedLeaf = 0;
    pKey->keyType = SYMCRYPT_LMSKEY_TYPE_NONE;
}

static
UINT16
LmsOtsCalculateChecksum(
    _In_reads_bytes_(cbString)  PCBYTE pbString,
                                UINT32 cbString,
                                UINT32 nWidth,
                                UINT32 nLeftShift)
{
    UINT32 sum = 0;
    UINT32 max = (1 << nWidth) - 1;
    SYMCRYPT_ASSERT(SYMCRYPT_IS_VALID_WINTERNITZ_WIDTH(nWidth));

    for (UINT32 i = 0; i < (cbString * 8 / nWidth); i = i + 1)
    {
        sum = sum + max - SymCryptHbsGetDigit(nWidth, pbString, cbString, i);
    }
    return (UINT16)(sum << nLeftShift);
}

static
SIZE_T
SYMCRYPT_CALL
LmsOtsSizeofSignatureFromParams(
    _In_  PCSYMCRYPT_LMS_PARAMS pParams)
{
    UINT32 n    = pParams->cbHashOutput;
    UINT32 p    = pParams->nByteStringCount;
    SIZE_T size = 0;

    size += sizeof(UINT32); // type
    size += n;              // randomizer
    size += p * n;          // y[0..p-1]

    return size;
}

static
VOID
SYMCRYPT_CALL
LmsOtskeySign(
    _In_                                        PSYMCRYPT_LMS_KEY   pKey,
                                                UINT64              nLeafNumber,
    _In_reads_bytes_(cbMessage)                 PCBYTE              pbMessage,
                                                SIZE_T              cbMessage,
    _In_reads_bytes_(pKey->params.cbHashOutput) PCBYTE              pbRandomizer,
    _Out_writes_bytes_(cbSignature)             PBYTE               pbSignature,
                                                SIZE_T              cbSignature)
{
    PCSYMCRYPT_HASH     pHash                                                   = pKey->params.pLmsHashFunction;
    SYMCRYPT_HASH_STATE state                                                   = { 0 };
    UINT32              nIndex                                                  = 0;
    UINT32              cbHashOutput                                            = pKey->params.cbHashOutput;
    UINT32              nWinternitzChainWidth                                   = pKey->params.nWinternitzChainWidth;
    SIZE_T              cbRemainingBytes                                        = cbSignature;
    UINT16              nChecksum                                                   = 0;
    BYTE                en32LeafNumber[sizeof(UINT32)]                          = {0};
    BYTE                en16Index[sizeof(UINT16)]                               = { 0 };
    BYTE                abOtsPrivateKey[SYMCRYPT_LMS_MAX_N]                     = { 0 };
    BYTE                abLmsHashedMessage[SYMCRYPT_LMS_MAX_N + sizeof(nChecksum)]  = { 0 };
    PBYTE               pbDest                                                  = pbSignature;

    SYMCRYPT_ASSERT(cbSignature == LmsOtsSizeofSignatureFromParams(&pKey->params));

    SYMCRYPT_STORE_MSBFIRST32(pbDest, pKey->params.lmsOtsAlgID);
    pbDest += sizeof(UINT32);
    cbRemainingBytes -= sizeof(UINT32);

    memcpy(pbDest, pbRandomizer, cbHashOutput);
    pbDest += cbHashOutput;
    cbRemainingBytes -= cbHashOutput;

    SYMCRYPT_STORE_MSBFIRST32(en32LeafNumber, (UINT32)nLeafNumber);
    LmsHashMessage(pHash, pKey->abId, en32LeafNumber, pbRandomizer, cbHashOutput, pbMessage, cbMessage, abLmsHashedMessage, cbHashOutput);

    nChecksum = LmsOtsCalculateChecksum(abLmsHashedMessage, cbHashOutput, nWinternitzChainWidth, pKey->params.nChecksumLShiftBits);
    SYMCRYPT_STORE_MSBFIRST16((UINT16*)&abLmsHashedMessage[cbHashOutput], nChecksum);

    SymCryptHashInit(pHash, &state);
    for (nIndex = 0; nIndex < pKey->params.nByteStringCount; nIndex++)
    {
        BYTE coeff = (BYTE)SymCryptHbsGetDigit(nWinternitzChainWidth, abLmsHashedMessage, cbHashOutput + sizeof(nChecksum), nIndex);
        LmsOtskeyComputePrivate(pKey, (UINT32)nLeafNumber, nIndex, abOtsPrivateKey);

        SYMCRYPT_STORE_MSBFIRST16(en16Index, (UINT16)nIndex);

        for (BYTE j = 0; j < coeff; j++)
        {
            SymCryptHashAppend(pHash, &state, pKey->abId, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
            SymCryptHashAppend(pHash, &state, en32LeafNumber, sizeof(UINT32));
            SymCryptHashAppend(pHash, &state, en16Index, sizeof(UINT16));
            SymCryptHashAppend(pHash, &state, &j, 1);
            SymCryptHashAppend(pHash, &state, abOtsPrivateKey, cbHashOutput);
            SymCryptHashResult(pHash, &state, abOtsPrivateKey, cbHashOutput);
        }
        memcpy(pbDest, abOtsPrivateKey, cbHashOutput);
        pbDest += cbHashOutput;
        cbRemainingBytes -= cbHashOutput;
    }
    SYMCRYPT_ASSERT(cbRemainingBytes == 0);

    return;
}

static
VOID
SYMCRYPT_CALL
LmsOtskeyComputePublic(
    _In_                                            PCSYMCRYPT_LMS_KEY  pKey,
                                                    UINT32              nNodeIdx,
    _Out_writes_bytes_(pKey->params.cbHashOutput)   PBYTE               pbK)
{
    UINT32              cbHashOutput                    = pKey->params.cbHashOutput;
    UINT32              maxJ                            = (1 << pKey->params.nWinternitzChainWidth) - 1;
    PCSYMCRYPT_HASH     pHash                           = pKey->params.pLmsHashFunction;
    SYMCRYPT_HASH_STATE statePriv                       = { 0 };
    SYMCRYPT_HASH_STATE statePub                        = { 0 };
    BYTE                en32LeafNumber[sizeof(UINT32)]  = { 0 };
    BYTE                en16Index[sizeof(UINT16)]       = { 0 };
    BYTE                abNode[SYMCRYPT_LMS_MAX_N]      = { 0 };

    SYMCRYPT_STORE_MSBFIRST32(en32LeafNumber, nNodeIdx);

    SymCryptHashInit(pHash, &statePub);
    SymCryptHashAppend(pHash, &statePub, pKey->abId, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
    SymCryptHashAppend(pHash, &statePub, en32LeafNumber, sizeof(UINT32));
    SymCryptHashAppend(pHash, &statePub, SYMCRYPT_LMS_D_PBLC, sizeof(SYMCRYPT_LMS_D_PBLC));

    SymCryptHashInit(pHash, &statePriv);
    for (UINT32 i = 0; i < pKey->params.nByteStringCount; i++)
    {
        LmsOtskeyComputePrivate(pKey, nNodeIdx, i, abNode);
        SYMCRYPT_STORE_MSBFIRST16(en16Index, (UINT16)i);

        for (BYTE j = 0; j < maxJ; j++)
        {
            SymCryptHashAppend(pHash, &statePriv, pKey->abId, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
            SymCryptHashAppend(pHash, &statePriv, en32LeafNumber, sizeof(UINT32));
            SymCryptHashAppend(pHash, &statePriv, en16Index, sizeof(UINT16));
            SymCryptHashAppend(pHash, &statePriv, &j, 1);
            SymCryptHashAppend(pHash, &statePriv, abNode, cbHashOutput);
            SymCryptHashResult(pHash, &statePriv, abNode, cbHashOutput);
        }
        SymCryptHashAppend(pHash, &statePub, abNode, cbHashOutput);
    }
    SymCryptHashResult(pHash, &statePub, pbK, cbHashOutput);
}

static
VOID
SYMCRYPT_CALL
LmsComputeNodeValue(
    _In_                                            PCSYMCRYPT_LMS_KEY  pKey,
                                                    UINT32              nIndex,
    _Out_writes_bytes_(pKey->params.cbHashOutput)   PBYTE               pbNodeValue,
                                                    SIZE_T              cbNodeValue)
{
    UNREFERENCED_PARAMETER(cbNodeValue);

    UINT32              cbHashOutput                    = pKey->params.cbHashOutput;
    UINT32              nInternalNodes                  = (UINT32)1 << pKey->params.nTreeHeight;
    PCSYMCRYPT_HASH     pHash                           = pKey->params.pLmsHashFunction;
    SYMCRYPT_HASH_STATE state                           = { 0 };
    BYTE                abTemp[SYMCRYPT_LMS_MAX_N]      = { 0 };
    BYTE                en32Index[sizeof(UINT32)]       = { 0 };
    BYTE                abOtsPubKey[SYMCRYPT_LMS_MAX_N] = { 0 };

    SYMCRYPT_ASSERT(nIndex > 0);
    SYMCRYPT_ASSERT(cbNodeValue == cbHashOutput);

    SYMCRYPT_STORE_MSBFIRST32(en32Index, nIndex);

    SymCryptHashInit(pHash, &state);
    SymCryptHashAppend(pHash, &state, pKey->abId, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
    SymCryptHashAppend(pHash, &state, en32Index, sizeof(UINT32));
    if (nIndex >= nInternalNodes)
    {
        LmsOtskeyComputePublic(pKey, nIndex - nInternalNodes, abOtsPubKey);

        SymCryptHashAppend(pHash, &state, SYMCRYPT_LMS_D_LEAF, sizeof(SYMCRYPT_LMS_D_LEAF));
        SymCryptHashAppend(pHash, &state, abOtsPubKey, cbHashOutput);
    }
    else
    {
        SymCryptHashAppend(pHash, &state, SYMCRYPT_LMS_D_INTR, sizeof(SYMCRYPT_LMS_D_INTR));

        LmsComputeNodeValue(pKey, 2 * nIndex, abTemp, cbHashOutput);
        SymCryptHashAppend(pHash, &state, abTemp, cbHashOutput);
        SymCryptWipeKnownSize(abTemp, SYMCRYPT_LMS_MAX_N);

        LmsComputeNodeValue(pKey, 2 * nIndex + 1, abTemp, cbHashOutput);
        SymCryptHashAppend(pHash, &state, abTemp, cbHashOutput);
    }
    SymCryptHashResult(pHash, &state, pbNodeValue, cbHashOutput);
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmsParamsFromAlgId(
            SYMCRYPT_LMS_ALGID      lmsAlgID,
            SYMCRYPT_LMS_OTS_ALGID  lmsOtsAlgID,
    _Out_   PSYMCRYPT_LMS_PARAMS    pParams)
{
    SYMCRYPT_ERROR                          scError                 = SYMCRYPT_NO_ERROR;
    SIZE_T                                  uLmsParametersCount     = 0;
    SIZE_T                                  uLmsOtsParametersCount  = 0;
    UINT32                                  u                       = 0;
    UINT32                                  v                       = 0;
    PCSYMCRYPT_LMS_PARAMETER_PREDEFINED     pLmsParameters          = NULL;
    PCSYMCRYPT_LMS_OTS_PARAMETER_PREDEFINED pLmsOtsParameters       = NULL;
    BOOL bFound = FALSE;

    SymCryptWipeKnownSize(pParams, sizeof(*pParams));
    pLmsOtsParameters = LmsOtsParametersPredefined;
    uLmsOtsParametersCount = SYMCRYPT_ARRAY_SIZE(LmsOtsParametersPredefined);
    pLmsParameters = LmsParametersPredefined;
    uLmsParametersCount = SYMCRYPT_ARRAY_SIZE(LmsParametersPredefined);

    for (UINT32 i = 0; i < uLmsParametersCount; i++)
    {
        if (pLmsParameters[i].lmsAlgId == lmsAlgID)
        {
            pParams->lmsAlgID = lmsAlgID;
            pParams->nTreeHeight = pLmsParameters[i].nTreeHeight;
            SYMCRYPT_ASSERT(pParams->nTreeHeight <= SYMCRYPT_LMS_MAX_CUSTOM_TREE_HEIGHT);

            pParams->cbHashOutput = pLmsParameters[i].cbHashOutput;

            SYMCRYPT_ASSERT(pLmsParameters[i].nHashIdx < SYMCRYPT_ARRAY_SIZE(LmsHashObjects));
            pParams->pLmsHashFunction = *LmsHashObjects[pLmsParameters[i].nHashIdx];
            bFound = TRUE;
            break;
        }
    }
    if (!bFound)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    bFound = FALSE;
    for (UINT32 i = 0; i < uLmsOtsParametersCount; i++)
    {
        if (pLmsOtsParameters[i].lmsOtsAlgId == lmsOtsAlgID)
        {
            SYMCRYPT_ASSERT(pLmsOtsParameters[i].nHashIdx < SYMCRYPT_ARRAY_SIZE(LmsHashObjects));

            if (pParams->pLmsHashFunction != *LmsHashObjects[pLmsOtsParameters[i].nHashIdx] ||
                pParams->cbHashOutput != pLmsOtsParameters[i].cbHashOutput)
            {
                scError = SYMCRYPT_INVALID_ARGUMENT;
                goto cleanup;
            }
            pParams->lmsOtsAlgID = lmsOtsAlgID;
            pParams->nWinternitzChainWidth = pLmsOtsParameters[i].nWidth;
            SymCryptHbsGetWinternitzLengths(
                pParams->cbHashOutput,
                pParams->nWinternitzChainWidth,
                &u,
                &v);
            SYMCRYPT_ASSERT((v * pParams->nWinternitzChainWidth) <= SYMCRYPT_LMS_CHECKSUM_SIZE);
            pParams->nChecksumLShiftBits = SYMCRYPT_LMS_CHECKSUM_SIZE - (v * pParams->nWinternitzChainWidth);
            pParams->nByteStringCount = u + v;
            bFound = TRUE;
            break;
        }
    }
    if (!bFound)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

cleanup:
    return scError;
}

SIZE_T
SYMCRYPT_CALL
SymCryptLmsSizeofSignatureFromParams(
    _In_ PCSYMCRYPT_LMS_PARAMS pParams)
{
    SIZE_T size = 0;

    size += sizeof(UINT32); // q
    size += LmsOtsSizeofSignatureFromParams(pParams); // LMS-OTS signature
    size += sizeof(UINT32); // type
    size += pParams->nTreeHeight * pParams->cbHashOutput; // path[0..h-1]
    return size;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmsSetParams(
    _Out_   PSYMCRYPT_LMS_PARAMS    pParams,
            UINT32                  lmsAlgID,
            UINT32                  lmsOtsAlgID,
    _In_    PCSYMCRYPT_HASH         pLmsHashFunction,
            UINT32                  cbHashOutput,
            UINT32                  nTreeHeight,
            UINT32                  nWinternitzChainWidth)
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;
    UINT32          u       = 0;
    UINT32          v       = 0;

    SymCryptWipeKnownSize(pParams, sizeof(*pParams));

    // nTreeHeight must be positive and maximum SYMCRYPT_LMS_MAX_CUSTOM_TREE_HEIGHT
    if (nTreeHeight == 0 || nTreeHeight > SYMCRYPT_LMS_MAX_CUSTOM_TREE_HEIGHT)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Output cbHashOutput cannot be larger than the hash output size or SYMCRYPT_LMS_MAX_N
    if (cbHashOutput == 0 || cbHashOutput > pLmsHashFunction->resultSize || cbHashOutput > SYMCRYPT_LMS_MAX_N)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Winternitz parameter must be one of 1, 2, 4, or 8
    if (!SYMCRYPT_IS_VALID_WINTERNITZ_WIDTH(nWinternitzChainWidth))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pParams->lmsAlgID = lmsAlgID;
    pParams->lmsOtsAlgID = lmsOtsAlgID;
    pParams->pLmsHashFunction = pLmsHashFunction;
    pParams->nTreeHeight = nTreeHeight;
    pParams->cbHashOutput = cbHashOutput;
    pParams->nWinternitzChainWidth = nWinternitzChainWidth;
    SymCryptHbsGetWinternitzLengths(
        pParams->cbHashOutput,
        pParams->nWinternitzChainWidth,
        &u,
        &v);
    pParams->nChecksumLShiftBits = SYMCRYPT_LMS_CHECKSUM_SIZE - (v * pParams->nWinternitzChainWidth);
    pParams->nByteStringCount = u + v;

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmsSizeofKeyBlobFromParams(
    _In_    PCSYMCRYPT_LMS_PARAMS   pParams,
            SYMCRYPT_LMSKEY_TYPE    keyType,
    _Out_   SIZE_T*                 pcbKey)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    switch (keyType)
    {
        case SYMCRYPT_LMSKEY_TYPE_PUBLIC:
            *pcbKey = SYMCRYPT_LMS_PUB_KEY_SIZE(pParams->cbHashOutput);
            break;

        case SYMCRYPT_LMSKEY_TYPE_PRIVATE:
            *pcbKey = SYMCRYPT_LMS_PRIV_KEY_SIZE(pParams->cbHashOutput);
            break;

        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            break;
    }

    return scError;
}

PSYMCRYPT_LMS_KEY
SYMCRYPT_CALL
SymCryptLmskeyAllocate(
    _In_    PCSYMCRYPT_LMS_PARAMS   pParams,
            UINT32                  flags)
{
    PSYMCRYPT_LMS_KEY pKey   = NULL;
    SIZE_T            cbSize = sizeof(SYMCRYPT_LMS_KEY);

    if (flags != 0)
    {
        goto cleanup;
    }

    pKey = SymCryptCallbackAlloc(cbSize);
    if (pKey == NULL)
    {
        goto cleanup;
    }

    SymCryptWipe(pKey, cbSize);
    pKey->cbSize = cbSize;

    memcpy(&pKey->params, pParams, sizeof(*pParams));
    SYMCRYPT_SET_MAGIC(pKey);

cleanup:
    return pKey;
}

VOID
SYMCRYPT_CALL
SymCryptLmskeyFree(
    _Inout_ PSYMCRYPT_LMS_KEY   pKey)
{
    SYMCRYPT_CHECK_MAGIC(pKey);

    SymCryptWipeKnownSize(pKey, sizeof(*pKey));
    SymCryptCallbackFree(pKey);
}

static
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmskeyVerifyRoot(
    _In_    PCSYMCRYPT_LMS_KEY pKey)
{
    BYTE            abPublicRoot[SYMCRYPT_LMS_MAX_N]  = { 0 };
    SYMCRYPT_ERROR  scError                           = SYMCRYPT_NO_ERROR;

    SYMCRYPT_CHECK_MAGIC(pKey);

    // key to be verified has to be a private key
    if (pKey->keyType != SYMCRYPT_LMSKEY_TYPE_PRIVATE)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // compute the public root from the private key, root node has index 1
    LmsComputeNodeValue(
        pKey,
        1,
        abPublicRoot,
        pKey->params.cbHashOutput);

    if (!SymCryptEqual(abPublicRoot, pKey->abPublicRoot, pKey->params.cbHashOutput))
    {
        scError = SYMCRYPT_HBS_PUBLIC_ROOT_MISMATCH;
    }

cleanup:

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmskeyGenerate(
    _Inout_ PSYMCRYPT_LMS_KEY   pKey,
            UINT32              flags)
{
    SYMCRYPT_ERROR   scError  = SYMCRYPT_NO_ERROR;

    SYMCRYPT_CHECK_MAGIC(pKey);

    if (flags != 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pKey->nNextUnusedLeaf = 0;
    // Set the LMS key identifier I
    scError = SymCryptCallbackRandom(pKey->abId, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Set the private key Seed value
    scError = SymCryptCallbackRandom(pKey->abSeed, pKey->params.cbHashOutput);
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // compute the public root from the private key
    LmsComputeNodeValue(
        pKey,
        1,
        pKey->abPublicRoot,
        pKey->params.cbHashOutput);

    pKey->keyType = SYMCRYPT_LMSKEY_TYPE_PRIVATE;

cleanup:
    if (scError != SYMCRYPT_NO_ERROR)
    {
        LmskeyWipe(pKey);
    }
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmskeyGetValue(
    _In_                        PCSYMCRYPT_LMS_KEY      pKey,
                                SYMCRYPT_LMSKEY_TYPE    keyType,
                                UINT32                  flags,
    _Out_writes_bytes_(cbBlob)  PBYTE                   pbBlob,
                                SIZE_T                  cbBlob)
{
    SYMCRYPT_ERROR   scError        = SYMCRYPT_NO_ERROR;
    SIZE_T           cbHashOutput   = pKey->params.cbHashOutput;
    SIZE_T           cbKey          = 0;

    SYMCRYPT_CHECK_MAGIC(pKey);

    if (flags != 0 ||
        (keyType != SYMCRYPT_LMSKEY_TYPE_PRIVATE &&
        keyType != SYMCRYPT_LMSKEY_TYPE_PUBLIC))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if ((keyType == SYMCRYPT_LMSKEY_TYPE_PRIVATE) && (pKey->keyType == SYMCRYPT_LMSKEY_TYPE_PUBLIC))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    SymCryptLmsSizeofKeyBlobFromParams(&pKey->params, keyType, &cbKey);
    if (cbBlob != cbKey)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }
    SYMCRYPT_STORE_MSBFIRST32(pbBlob, (UINT32)pKey->params.lmsAlgID);
    pbBlob += sizeof(UINT32);

    SYMCRYPT_STORE_MSBFIRST32(pbBlob, (UINT32)pKey->params.lmsOtsAlgID);
    pbBlob += sizeof(UINT32);

    memcpy(pbBlob, pKey->abId, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
    pbBlob += SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE;

    memcpy(pbBlob, pKey->abPublicRoot, cbHashOutput);
    pbBlob += cbHashOutput;

    if (keyType == SYMCRYPT_LMSKEY_TYPE_PRIVATE)
    {

        SYMCRYPT_ASSERT((pKey->nNextUnusedLeaf & 0xFFFFFFFF00000000) == 0);

        SYMCRYPT_STORE_MSBFIRST32(pbBlob, (UINT32)pKey->nNextUnusedLeaf);
        pbBlob += sizeof(UINT32);

        memcpy(pbBlob, pKey->abSeed, cbHashOutput);
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmskeySetValue(
    _In_reads_bytes_(cbBlob)    PCBYTE                  pbBlob,
                                SIZE_T                  cbBlob,
                                SYMCRYPT_LMSKEY_TYPE    keyType,
                                UINT32                  flags,
    _Inout_                     PSYMCRYPT_LMS_KEY       pKey)
{
    SYMCRYPT_ERROR  scError     = SYMCRYPT_NO_ERROR;
    UINT32          lmsAlgID    = 0;
    UINT32          lmsOtsAlgID = 0;
    SIZE_T          cbKey       = 0;

    SYMCRYPT_ASSERT(keyType == SYMCRYPT_LMSKEY_TYPE_PUBLIC || keyType == SYMCRYPT_LMSKEY_TYPE_PRIVATE);

    SYMCRYPT_CHECK_MAGIC(pKey);

    if (flags & (~SYMCRYPT_FLAG_LMSKEY_VERIFY_ROOT))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Public key validation can only be performed for private keys
    if ((flags & SYMCRYPT_FLAG_LMSKEY_VERIFY_ROOT) != 0 &&
        keyType != SYMCRYPT_LMSKEY_TYPE_PRIVATE)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    SymCryptLmsSizeofKeyBlobFromParams(&pKey->params, keyType, &cbKey);
    if (cbBlob != cbKey)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    lmsAlgID = SYMCRYPT_LOAD_MSBFIRST32(pbBlob);
    pbBlob += sizeof(UINT32);

    lmsOtsAlgID = SYMCRYPT_LOAD_MSBFIRST32(pbBlob);
    pbBlob += sizeof(UINT32);

    // check if the lmsAlgID and lmsOtsAlgID matches the ones in the key
    if (lmsAlgID != pKey->params.lmsAlgID || lmsOtsAlgID != pKey->params.lmsOtsAlgID)
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }
    SymCryptWipeKnownSize(pKey->abPublicRoot, sizeof(pKey->abPublicRoot));
    SymCryptWipeKnownSize(pKey->abId, sizeof(pKey->abId));

    pKey->keyType = keyType;

    memcpy(pKey->abId, pbBlob, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
    pbBlob += SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE;

    memcpy(pKey->abPublicRoot, pbBlob, pKey->params.cbHashOutput);
    pbBlob += pKey->params.cbHashOutput;

    if (keyType == SYMCRYPT_LMSKEY_TYPE_PRIVATE)
    {
        // Wipe private key material
        pKey->nNextUnusedLeaf = 0;
        SymCryptWipeKnownSize(pKey->abSeed, sizeof(pKey->abSeed));

        pKey->nNextUnusedLeaf = SYMCRYPT_LOAD_MSBFIRST32(pbBlob);
        pbBlob += sizeof(UINT32);

        memcpy(pKey->abSeed, pbBlob,pKey->params.cbHashOutput);

        if (flags & SYMCRYPT_FLAG_LMSKEY_VERIFY_ROOT)
        {
            scError = SymCryptLmskeyVerifyRoot(pKey);
            if (scError != SYMCRYPT_NO_ERROR)
            {
                goto cleanup;
            }
        }
    }

cleanup:
    if (scError != SYMCRYPT_NO_ERROR)
    {
        LmskeyWipe(pKey);
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmsSign(
    _Inout_                         PSYMCRYPT_LMS_KEY   pKey,
    _In_reads_bytes_(cbMessage)     PCBYTE              pbMessage,
                                    SIZE_T              cbMessage,
                                    UINT32              flags,
    _Out_writes_bytes_(cbSignature) PBYTE               pbSignature,
                                    SIZE_T              cbSignature)
{
    SYMCRYPT_ERROR  scError                             = SYMCRYPT_NO_ERROR;
    UINT32          nLeafNumber                         = (UINT32)pKey->nNextUnusedLeaf;
    UINT32          cbHashOutput                        = pKey->params.cbHashOutput;
    UINT32          nTreeHeight                         = pKey->params.nTreeHeight;
    SIZE_T          cbRemainingBytes                    = cbSignature;
    UINT32          nLeavesCount                        = ((UINT32)1 << nTreeHeight);
    UINT32          nNodeIndex                          = 0;
    UINT32          nTemp                               = 0;
    SIZE_T          cbOtsSignature                      = LmsOtsSizeofSignatureFromParams(&pKey->params);
    BYTE            abLMSRandomizer[SYMCRYPT_LMS_MAX_N] = { 0 };

    SYMCRYPT_CHECK_MAGIC(pKey);

    if (flags != 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (pKey->keyType != SYMCRYPT_LMSKEY_TYPE_PRIVATE)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (cbSignature != SymCryptLmsSizeofSignatureFromParams(&pKey->params))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    scError = SymCryptCallbackRandom(abLMSRandomizer, cbHashOutput);
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    nLeafNumber = (UINT32)SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(&pKey->nNextUnusedLeaf, 1) - 1;
    if (nLeafNumber >= (nLeavesCount))
    {
        scError = SYMCRYPT_HBS_NO_OTS_KEYS_LEFT;
        pKey->nNextUnusedLeaf = nLeavesCount;
        goto cleanup;
    }
    SYMCRYPT_STORE_MSBFIRST32(pbSignature, nLeafNumber);
    pbSignature += sizeof(UINT32);
    cbRemainingBytes -= sizeof(UINT32);

    LmsOtskeySign(
        pKey,
        nLeafNumber,
        pbMessage,
        cbMessage,
        abLMSRandomizer,
        pbSignature,
        cbOtsSignature);
    pbSignature += cbOtsSignature;
    cbRemainingBytes -= cbOtsSignature;

    SYMCRYPT_STORE_MSBFIRST32(pbSignature, pKey->params.lmsAlgID);
    pbSignature += sizeof(UINT32);
    cbRemainingBytes -= sizeof(UINT32);

    nNodeIndex = nLeavesCount + nLeafNumber;
	// write the path into the signature
    for (UINT32 nIndex = 0; nIndex < nTreeHeight; nIndex++)
    {
        nTemp = (nNodeIndex >> nIndex) ^ 1;
        LmsComputeNodeValue(
            pKey,
            nTemp,
            pbSignature,
            cbHashOutput);
        pbSignature += cbHashOutput;
        cbRemainingBytes -= cbHashOutput;
    }
    SYMCRYPT_ASSERT(cbRemainingBytes == 0);

cleanup:
    return scError;
}

static
SYMCRYPT_ERROR
SYMCRYPT_CALL
LmsComputeOtsPubKeyCandidate(
                                                            UINT32                  nLeafNumber,
    _In_reads_bytes_(cbMessage)                             PCBYTE                  pbMessage,
                                                            SIZE_T                  cbMessage,
    _In_reads_bytes_(cbOtsSignature)                        PCBYTE                  pbOtsSignature,
                                                            SIZE_T                  cbOtsSignature,
    _In_reads_bytes_(SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE) PCBYTE                  pbId,
    _In_                                                    PCSYMCRYPT_LMS_PARAMS   pSigParams,
    _Out_writes_bytes_(pSigParams->cbHashOutput)            PBYTE                   pbOtsPubKeyCandidate)
{
    SYMCRYPT_ERROR          scError                                 = SYMCRYPT_NO_ERROR;
    UINT32                  cbHashOutput                            = pSigParams->cbHashOutput;
    UINT32                  nWinternitzChainWidth                   = pSigParams->nWinternitzChainWidth;
    UINT32                  nByteStringCount                        = pSigParams->nByteStringCount;
    UINT32                  nSigType                                = 0;
    UINT32                  nMaxJ                                   = (1 << nWinternitzChainWidth) - 1;
    UINT16                  nCksm                                   = 0;
    PCSYMCRYPT_HASH         pHash                                   = pSigParams->pLmsHashFunction;
    SYMCRYPT_HASH_STATE     state                                   = { 0 };
    SYMCRYPT_HASH_STATE     stateKc                                  = { 0 };
    BYTE                    en32LeafNumber[sizeof(UINT32)]          = { 0 };
    BYTE                    en16Index[sizeof(UINT16)]                            = { 0 };
    BYTE                    abLmsHashedMsg[SYMCRYPT_LMS_MAX_N + sizeof(nCksm)]  = { 0 };
    BYTE                    abTmpRes[SYMCRYPT_LMS_MAX_N]            = { 0 };
    PCBYTE                  pbRandomizer                            = NULL;

    if (cbOtsSignature != LmsOtsSizeofSignatureFromParams(pSigParams))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    nSigType = SYMCRYPT_LOAD_MSBFIRST32(pbOtsSignature);
    pbOtsSignature += sizeof(UINT32);
    if (nSigType != pSigParams->lmsOtsAlgID)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pbRandomizer = pbOtsSignature;
    pbOtsSignature += cbHashOutput;

    SYMCRYPT_STORE_MSBFIRST32(en32LeafNumber, nLeafNumber);

    LmsHashMessage(pHash, pbId, en32LeafNumber, pbRandomizer, cbHashOutput, pbMessage, cbMessage, abLmsHashedMsg, cbHashOutput);
    nCksm = LmsOtsCalculateChecksum(abLmsHashedMsg, cbHashOutput, nWinternitzChainWidth, pSigParams->nChecksumLShiftBits);
    SYMCRYPT_STORE_MSBFIRST16((UINT16*)&abLmsHashedMsg[cbHashOutput], (UINT16)nCksm);

    SymCryptHashInit(pHash, &stateKc);
    SymCryptHashAppend(pHash, &stateKc, pbId, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
    SymCryptHashAppend(pHash, &stateKc, en32LeafNumber, sizeof(UINT32));
    SymCryptHashAppend(pHash, &stateKc, SYMCRYPT_LMS_D_PBLC, sizeof(SYMCRYPT_LMS_D_PBLC));

    SymCryptHashInit(pHash, &state);
    for (UINT32 i = 0; i < nByteStringCount; i++)
    {
        BYTE a = (BYTE)SymCryptHbsGetDigit(nWinternitzChainWidth, abLmsHashedMsg, cbHashOutput + sizeof(nCksm), i);
        PCBYTE tmp = pbOtsSignature + (i * cbHashOutput);

        SYMCRYPT_STORE_MSBFIRST16(en16Index, (UINT16)i);

        for (BYTE j = a; j < nMaxJ; j++)
        {
            SymCryptHashAppend(pHash, &state, pbId, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
            SymCryptHashAppend(pHash, &state, en32LeafNumber, sizeof(UINT32));
            SymCryptHashAppend(pHash, &state, en16Index, sizeof(UINT16));
            SymCryptHashAppend(pHash, &state, &j, 1);
            SymCryptHashAppend(pHash, &state, tmp, cbHashOutput);
            SymCryptHashResult(pHash, &state, abTmpRes, cbHashOutput);
            tmp = abTmpRes;
        }
        SymCryptHashAppend(pHash, &stateKc, tmp, cbHashOutput);
    }
    SymCryptHashResult(pHash, &stateKc, pbOtsPubKeyCandidate, cbHashOutput);

cleanup:
    return scError;
}

static
VOID
SYMCRYPT_CALL
LmsComputeRootCandidate(
                                                                    UINT32                  nLeafNumber,
    _In_                                                            PCSYMCRYPT_LMS_PARAMS   pParams,
    _In_reads_bytes_(SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE)         PCBYTE                  pbId,
    _In_reads_bytes_(pParams->nTreeHeight * pParams->cbHashOutput)  PCBYTE                  pbPath,
    _In_reads_bytes_(pParams->cbHashOutput)                         PCBYTE                  pbPubKeyCandidate,
    _Out_writes_bytes_(pParams->cbHashOutput)                       PBYTE                   pbRootCandidate
)
{
    PCSYMCRYPT_HASH     pHash                       = pParams->pLmsHashFunction;
    SYMCRYPT_HASH_STATE state                       = { 0 };
    UINT32              cbHashOutput                = pParams->cbHashOutput;
    UINT32              nIndex                      = 0;
    UINT32              nNodeNum                    = (1 << pParams->nTreeHeight) + nLeafNumber;
    PBYTE               pbTemp                      = pbRootCandidate;
    BYTE                en32NodeNum[sizeof(UINT32)] = { 0 };

    SYMCRYPT_STORE_MSBFIRST32(en32NodeNum, nNodeNum);
    SymCryptHashInit(pHash, &state);
    SymCryptHashAppend(pHash, &state, pbId, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
    SymCryptHashAppend(pHash, &state, en32NodeNum, sizeof(UINT32));
    SymCryptHashAppend(pHash, &state, SYMCRYPT_LMS_D_LEAF, sizeof(SYMCRYPT_LMS_D_LEAF));
    SymCryptHashAppend(pHash, &state, pbPubKeyCandidate, cbHashOutput);
    SymCryptHashResult(pHash, &state, pbTemp, cbHashOutput);

    for (nIndex = 0; nIndex  < pParams->nTreeHeight; nIndex ++)
    {
        SYMCRYPT_STORE_MSBFIRST32(en32NodeNum, nNodeNum / 2);
        SymCryptHashAppend(pHash, &state, pbId, SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE);
        SymCryptHashAppend(pHash, &state, en32NodeNum, sizeof(UINT32));
        SymCryptHashAppend(pHash, &state, SYMCRYPT_LMS_D_INTR, sizeof(SYMCRYPT_LMS_D_INTR));
        if (nNodeNum % 2)
        {
            SymCryptHashAppend(pHash, &state, pbPath + (cbHashOutput * nIndex), cbHashOutput);
            SymCryptHashAppend(pHash, &state, pbTemp, cbHashOutput);
        }
        else
        {
            SymCryptHashAppend(pHash, &state, pbTemp, cbHashOutput);
            SymCryptHashAppend(pHash, &state, pbPath + (cbHashOutput * nIndex), cbHashOutput);
        }
        SymCryptHashResult(pHash, &state, pbTemp, cbHashOutput);
        nNodeNum /= 2;
    }
    SYMCRYPT_ASSERT(nNodeNum <= 1);

    memcpy(pbRootCandidate, pbTemp, cbHashOutput);
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmsVerifyInternal(
    _In_                            PCSYMCRYPT_LMS_KEY  pKey,
    _In_reads_bytes_(cbMessage)     PCBYTE              pbMessage,
                                    SIZE_T              cbMessage,
                                    UINT32              flags,
    _In_reads_bytes_(cbSignature)   PCBYTE              pbSignature,
                                    SIZE_T              cbSignature)
{
    SYMCRYPT_ASSERT(pKey != NULL);
    SYMCRYPT_ASSERT(pKey->keyType != SYMCRYPT_LMSKEY_TYPE_NONE);
    SYMCRYPT_CHECK_MAGIC(pKey);

    SYMCRYPT_ERROR          scError                                     = SYMCRYPT_NO_ERROR;
    UINT32                  cbHashOutput                                = pKey->params.cbHashOutput;
    PCSYMCRYPT_LMS_PARAMS   pLmsKeyParams                               = &pKey->params;
    PCBYTE                  pbLocSignature                              = pbSignature;
    BYTE                    abRootCandidate[SYMCRYPT_LMS_MAX_N]         = { 0 };
    BYTE                    abOtsPubKeyCandidate[SYMCRYPT_LMS_MAX_N]    = { 0 };

    if (flags != 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (cbSignature != SymCryptLmsSizeofSignatureFromParams(&pKey->params))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    UINT32 nLeafNumber = SYMCRYPT_LOAD_MSBFIRST32(pbLocSignature);
    pbLocSignature += sizeof(UINT32);
    if (nLeafNumber >= ((UINT32)1 << pKey->params.nTreeHeight))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    UINT32 nOtsSigtype = SYMCRYPT_LOAD_MSBFIRST32(pbLocSignature);
    pbLocSignature += sizeof(UINT32);

    if (nOtsSigtype != pLmsKeyParams->lmsOtsAlgID)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pbLocSignature += cbHashOutput * (pKey->params.nByteStringCount + 1); // +1 is for the randomizer
    UINT32 nSigType = SYMCRYPT_LOAD_MSBFIRST32(pbLocSignature);
    pbLocSignature += sizeof(UINT32);

    if (nSigType != pLmsKeyParams->lmsAlgID)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    scError = LmsComputeOtsPubKeyCandidate(
        nLeafNumber,
        pbMessage,
        cbMessage,
        pbSignature + sizeof(UINT32), //the +sizeof(UINT32) is to skip the leaf number and reach the LMS-OTS signature
        LmsOtsSizeofSignatureFromParams(&pKey->params),
        pKey->abId,
        pLmsKeyParams,
        abOtsPubKeyCandidate);
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    LmsComputeRootCandidate(
        nLeafNumber,
        pLmsKeyParams,
        pKey->abId,
        pbLocSignature,
        abOtsPubKeyCandidate,
        abRootCandidate);
    if (!SymCryptEqual(abRootCandidate, pKey->abPublicRoot, cbHashOutput))
    {
        scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
        goto cleanup;
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmsVerify(
    _In_                            PCSYMCRYPT_LMS_KEY  pKey,
    _In_reads_bytes_(cbMessage)     PCBYTE              pbMessage,
                                    SIZE_T              cbMessage,
                                    UINT32              flags,
    _In_reads_bytes_(cbSignature)   PCBYTE              pbSignature,
                                    SIZE_T              cbSignature)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SYMCRYPT_RUN_SELFTEST_ONCE(
        SymCryptLmsSelftest,
        SYMCRYPT_SELFTEST_ALGORITHM_LMS);

    scError = SymCryptLmsVerifyInternal(
        pKey,
        pbMessage,
        cbMessage,
        flags,
        pbSignature,
        cbSignature);

    return scError;
}
