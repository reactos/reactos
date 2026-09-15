//
// xmss.c XMSS and XMSS^MT implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//


// Maximum size of the domain separator prefix used in PRFs
#define SYMCRYPT_XMSS_MAX_PREFIX_SIZE       SYMCRYPT_HASH_MAX_RESULT_SIZE

// PRF domain separators
#define SYMCRYPT_XMSS_F             0x00
#define SYMCRYPT_XMSS_H             0x01
#define SYMCRYPT_XMSS_H_MSG         0x02
#define SYMCRYPT_XMSS_PRF           0x03
#define SYMCRYPT_XMSS_PRF_KEYGEN    0x04


static const PCSYMCRYPT_HASH* XmssHashArray[] = {
    &SymCryptSha256Algorithm,       //  0
    &SymCryptSha512Algorithm,       //  1
    &SymCryptShake128HashAlgorithm, //  2
    &SymCryptShake256HashAlgorithm, //  3
};


typedef enum _SYMCRYPT_XMSS_WOTSP_ALGID
{
    //                                                  Hash Fn.    RFC-8391    SP800-208
    SYMCRYPT_XMSS_WOTSP_SHA2_256        = 0x00000001,   // SHA-256      X           X
    SYMCRYPT_XMSS_WOTSP_SHA2_512        = 0x00000002,   // SHA-512      X
    SYMCRYPT_XMSS_WOTSP_SHAKE_256       = 0x00000003,   // SHAKE128     X
    SYMCRYPT_XMSS_WOTSP_SHAKE_512       = 0x00000004,   // SHAKE256     X
    SYMCRYPT_XMSS_WOTSP_SHA2_192        = 0x00000005,   // SHA-256                  X
    SYMCRYPT_XMSS_WOTSP_SHAKE256_256    = 0x00000006,   // SHAKE256                 X
    SYMCRYPT_XMSS_WOTSP_SHAKE256_192    = 0x00000007,   // SHAKE256                 X

} SYMCRYPT_XMSS_WOTSP_ALGID, *PSYMCRYPT_XMSS_WOTSP_ALGID;


typedef struct _SYMCRYPT_XMSS_WOTSP_PARAMS
{
    SYMCRYPT_XMSS_WOTSP_ALGID   wotspId;
    UINT8                       hashIndex;
    UINT8                       n;
    UINT8                       w;
    UINT8                       cbPrefix;

} SYMCRYPT_XMSS_WOTSP_PARAMS, *PSYMCRYPT_XMSS_WOTSP_PARAMS;


static const SYMCRYPT_XMSS_WOTSP_PARAMS XmssWotspParams[] =
{
    //  wotspId                             hashIndex   n       w   cbPrefix
    {   SYMCRYPT_XMSS_WOTSP_SHA2_256,       0,          32,     4,  32  },  // SHA-256
    {   SYMCRYPT_XMSS_WOTSP_SHA2_512,       1,          64,     4,  64  },  // SHA-512
    {   SYMCRYPT_XMSS_WOTSP_SHAKE_256,      2,          32,     4,  32  },  // SHAKE128
    {   SYMCRYPT_XMSS_WOTSP_SHAKE_512,      3,          64,     4,  64  },  // SHAKE256
    {   SYMCRYPT_XMSS_WOTSP_SHA2_192,       0,          24,     4,  4   },  // SHA-256
    {   SYMCRYPT_XMSS_WOTSP_SHAKE256_256,   3,          32,     4,  32  },  // SHAKE256
    {   SYMCRYPT_XMSS_WOTSP_SHAKE256_192,   3,          24,     4,  4   },  // SHAKE256
};

typedef struct _SYMCRYPT_XMSS_PARAMETER_PREDEFINED
{
    UINT32                      idAlg;

    SYMCRYPT_XMSS_WOTSP_ALGID   idWotsp;

    // total tree height (each level has height h/d)
    UINT8                       h;

    // number of layers (for single tree, d=1)
    UINT8                       d;

} SYMCRYPT_XMSS_PARAMETER_PREDEFINED;

typedef SYMCRYPT_XMSS_PARAMETER_PREDEFINED* PSYMCRYPT_XMSS_PARAMETER_PREDEFINED;
typedef const SYMCRYPT_XMSS_PARAMETER_PREDEFINED* PCSYMCRYPT_XMSS_PARAMETER_PREDEFINED;


static const SYMCRYPT_XMSS_PARAMETER_PREDEFINED XmssParametersPredefined[] = {

    //  algId                               wotspId/wotspIndex                  h   d
    {   SYMCRYPT_XMSS_SHA2_10_256,          SYMCRYPT_XMSS_WOTSP_SHA2_256,       10, 1   },
    {   SYMCRYPT_XMSS_SHA2_16_256,          SYMCRYPT_XMSS_WOTSP_SHA2_256,       16, 1   },
    {   SYMCRYPT_XMSS_SHA2_20_256,          SYMCRYPT_XMSS_WOTSP_SHA2_256,       20, 1   },
    {   SYMCRYPT_XMSS_SHA2_10_512,          SYMCRYPT_XMSS_WOTSP_SHA2_512,       10, 1   },
    {   SYMCRYPT_XMSS_SHA2_16_512,          SYMCRYPT_XMSS_WOTSP_SHA2_512,       16, 1   },
    {   SYMCRYPT_XMSS_SHA2_20_512,          SYMCRYPT_XMSS_WOTSP_SHA2_512,       20, 1   },
    {   SYMCRYPT_XMSS_SHAKE_10_256,         SYMCRYPT_XMSS_WOTSP_SHAKE_256,      10, 1   },
    {   SYMCRYPT_XMSS_SHAKE_16_256,         SYMCRYPT_XMSS_WOTSP_SHAKE_256,      16, 1   },
    {   SYMCRYPT_XMSS_SHAKE_20_256,         SYMCRYPT_XMSS_WOTSP_SHAKE_256,      20, 1   },
    {   SYMCRYPT_XMSS_SHAKE_10_512,         SYMCRYPT_XMSS_WOTSP_SHAKE_512,      10, 1   },
    {   SYMCRYPT_XMSS_SHAKE_16_512,         SYMCRYPT_XMSS_WOTSP_SHAKE_512,      16, 1   },
    {   SYMCRYPT_XMSS_SHAKE_20_512,         SYMCRYPT_XMSS_WOTSP_SHAKE_512,      20, 1   },
    {   SYMCRYPT_XMSS_SHA2_10_192,          SYMCRYPT_XMSS_WOTSP_SHA2_192,       10, 1   },
    {   SYMCRYPT_XMSS_SHA2_16_192,          SYMCRYPT_XMSS_WOTSP_SHA2_192,       16, 1   },
    {   SYMCRYPT_XMSS_SHA2_20_192,          SYMCRYPT_XMSS_WOTSP_SHA2_192,       20, 1   },
    {   SYMCRYPT_XMSS_SHAKE256_10_256,      SYMCRYPT_XMSS_WOTSP_SHAKE256_256,   10, 1   },
    {   SYMCRYPT_XMSS_SHAKE256_16_256,      SYMCRYPT_XMSS_WOTSP_SHAKE256_256,   16, 1   },
    {   SYMCRYPT_XMSS_SHAKE256_20_256,      SYMCRYPT_XMSS_WOTSP_SHAKE256_256,   20, 1   },
    {   SYMCRYPT_XMSS_SHAKE256_10_192,      SYMCRYPT_XMSS_WOTSP_SHAKE256_192,   10, 1   },
    {   SYMCRYPT_XMSS_SHAKE256_16_192,      SYMCRYPT_XMSS_WOTSP_SHAKE256_192,   16, 1   },
    {   SYMCRYPT_XMSS_SHAKE256_20_192,      SYMCRYPT_XMSS_WOTSP_SHAKE256_192,   20, 1   },
};


static const SYMCRYPT_XMSS_PARAMETER_PREDEFINED XmssMtParametersPredefined[] = {

    //  algId                               wotspId/wotspIndex                  h   d
    {   SYMCRYPT_XMSSMT_SHA2_20_2_256,      SYMCRYPT_XMSS_WOTSP_SHA2_256,       20, 2   },
    {   SYMCRYPT_XMSSMT_SHA2_20_4_256,      SYMCRYPT_XMSS_WOTSP_SHA2_256,       20, 4   },
    {   SYMCRYPT_XMSSMT_SHA2_40_2_256,      SYMCRYPT_XMSS_WOTSP_SHA2_256,       40, 2   },
    {   SYMCRYPT_XMSSMT_SHA2_40_4_256,      SYMCRYPT_XMSS_WOTSP_SHA2_256,       40, 4   },
    {   SYMCRYPT_XMSSMT_SHA2_40_8_256,      SYMCRYPT_XMSS_WOTSP_SHA2_256,       40, 8   },
    {   SYMCRYPT_XMSSMT_SHA2_60_3_256,      SYMCRYPT_XMSS_WOTSP_SHA2_256,       60, 3   },
    {   SYMCRYPT_XMSSMT_SHA2_60_6_256,      SYMCRYPT_XMSS_WOTSP_SHA2_256,       60, 6   },
    {   SYMCRYPT_XMSSMT_SHA2_60_12_256,     SYMCRYPT_XMSS_WOTSP_SHA2_256,       60, 12  },

    {   SYMCRYPT_XMSSMT_SHA2_20_2_512,      SYMCRYPT_XMSS_WOTSP_SHA2_512,       20, 2   },
    {   SYMCRYPT_XMSSMT_SHA2_20_4_512,      SYMCRYPT_XMSS_WOTSP_SHA2_512,       20, 4   },
    {   SYMCRYPT_XMSSMT_SHA2_40_2_512,      SYMCRYPT_XMSS_WOTSP_SHA2_512,       40, 2   },
    {   SYMCRYPT_XMSSMT_SHA2_40_4_512,      SYMCRYPT_XMSS_WOTSP_SHA2_512,       40, 4   },
    {   SYMCRYPT_XMSSMT_SHA2_40_8_512,      SYMCRYPT_XMSS_WOTSP_SHA2_512,       40, 8   },
    {   SYMCRYPT_XMSSMT_SHA2_60_3_512,      SYMCRYPT_XMSS_WOTSP_SHA2_512,       60, 3   },
    {   SYMCRYPT_XMSSMT_SHA2_60_6_512,      SYMCRYPT_XMSS_WOTSP_SHA2_512,       60, 6   },
    {   SYMCRYPT_XMSSMT_SHA2_60_12_512,     SYMCRYPT_XMSS_WOTSP_SHA2_512,       60, 12  },

    {   SYMCRYPT_XMSSMT_SHAKE_20_2_256,     SYMCRYPT_XMSS_WOTSP_SHAKE_256,      20, 2   },
    {   SYMCRYPT_XMSSMT_SHAKE_20_4_256,     SYMCRYPT_XMSS_WOTSP_SHAKE_256,      20, 4   },
    {   SYMCRYPT_XMSSMT_SHAKE_40_2_256,     SYMCRYPT_XMSS_WOTSP_SHAKE_256,      40, 2   },
    {   SYMCRYPT_XMSSMT_SHAKE_40_4_256,     SYMCRYPT_XMSS_WOTSP_SHAKE_256,      40, 4   },
    {   SYMCRYPT_XMSSMT_SHAKE_40_8_256,     SYMCRYPT_XMSS_WOTSP_SHAKE_256,      40, 8   },
    {   SYMCRYPT_XMSSMT_SHAKE_60_3_256,     SYMCRYPT_XMSS_WOTSP_SHAKE_256,      60, 3   },
    {   SYMCRYPT_XMSSMT_SHAKE_60_6_256,     SYMCRYPT_XMSS_WOTSP_SHAKE_256,      60, 6   },
    {   SYMCRYPT_XMSSMT_SHAKE_60_12_256,    SYMCRYPT_XMSS_WOTSP_SHAKE_256,      60, 12  },

    {   SYMCRYPT_XMSSMT_SHAKE_20_2_512,     SYMCRYPT_XMSS_WOTSP_SHAKE_512,      20, 2   },
    {   SYMCRYPT_XMSSMT_SHAKE_20_4_512,     SYMCRYPT_XMSS_WOTSP_SHAKE_512,      20, 4   },
    {   SYMCRYPT_XMSSMT_SHAKE_40_2_512,     SYMCRYPT_XMSS_WOTSP_SHAKE_512,      40, 2   },
    {   SYMCRYPT_XMSSMT_SHAKE_40_4_512,     SYMCRYPT_XMSS_WOTSP_SHAKE_512,      40, 4   },
    {   SYMCRYPT_XMSSMT_SHAKE_40_8_512,     SYMCRYPT_XMSS_WOTSP_SHAKE_512,      40, 8   },
    {   SYMCRYPT_XMSSMT_SHAKE_60_3_512,     SYMCRYPT_XMSS_WOTSP_SHAKE_512,      60, 3   },
    {   SYMCRYPT_XMSSMT_SHAKE_60_6_512,     SYMCRYPT_XMSS_WOTSP_SHAKE_512,      60, 6   },
    {   SYMCRYPT_XMSSMT_SHAKE_60_12_512,    SYMCRYPT_XMSS_WOTSP_SHAKE_512,      60, 12  },

    {   SYMCRYPT_XMSSMT_SHA2_20_2_192,      SYMCRYPT_XMSS_WOTSP_SHA2_192,       20, 2   },
    {   SYMCRYPT_XMSSMT_SHA2_20_4_192,      SYMCRYPT_XMSS_WOTSP_SHA2_192,       20, 4   },
    {   SYMCRYPT_XMSSMT_SHA2_40_2_192,      SYMCRYPT_XMSS_WOTSP_SHA2_192,       40, 2   },
    {   SYMCRYPT_XMSSMT_SHA2_40_4_192,      SYMCRYPT_XMSS_WOTSP_SHA2_192,       40, 4   },
    {   SYMCRYPT_XMSSMT_SHA2_40_8_192,      SYMCRYPT_XMSS_WOTSP_SHA2_192,       40, 8   },
    {   SYMCRYPT_XMSSMT_SHA2_60_3_192,      SYMCRYPT_XMSS_WOTSP_SHA2_192,       60, 3   },
    {   SYMCRYPT_XMSSMT_SHA2_60_6_192,      SYMCRYPT_XMSS_WOTSP_SHA2_192,       60, 6   },
    {   SYMCRYPT_XMSSMT_SHA2_60_12_192,     SYMCRYPT_XMSS_WOTSP_SHA2_192,       60, 12  },

    {   SYMCRYPT_XMSSMT_SHAKE256_20_2_256,  SYMCRYPT_XMSS_WOTSP_SHAKE256_256,   20, 2   },
    {   SYMCRYPT_XMSSMT_SHAKE256_20_4_256,  SYMCRYPT_XMSS_WOTSP_SHAKE256_256,   20, 4   },
    {   SYMCRYPT_XMSSMT_SHAKE256_40_2_256,  SYMCRYPT_XMSS_WOTSP_SHAKE256_256,   40, 2   },
    {   SYMCRYPT_XMSSMT_SHAKE256_40_4_256,  SYMCRYPT_XMSS_WOTSP_SHAKE256_256,   40, 4   },
    {   SYMCRYPT_XMSSMT_SHAKE256_40_8_256,  SYMCRYPT_XMSS_WOTSP_SHAKE256_256,   40, 8   },
    {   SYMCRYPT_XMSSMT_SHAKE256_60_3_256,  SYMCRYPT_XMSS_WOTSP_SHAKE256_256,   60, 3   },
    {   SYMCRYPT_XMSSMT_SHAKE256_60_6_256,  SYMCRYPT_XMSS_WOTSP_SHAKE256_256,   60, 6   },
    {   SYMCRYPT_XMSSMT_SHAKE256_60_12_256, SYMCRYPT_XMSS_WOTSP_SHAKE256_256,   60, 12  },

    {   SYMCRYPT_XMSSMT_SHAKE256_20_2_192,  SYMCRYPT_XMSS_WOTSP_SHAKE256_192,   20, 2   },
    {   SYMCRYPT_XMSSMT_SHAKE256_20_4_192,  SYMCRYPT_XMSS_WOTSP_SHAKE256_192,   20, 4   },
    {   SYMCRYPT_XMSSMT_SHAKE256_40_2_192,  SYMCRYPT_XMSS_WOTSP_SHAKE256_192,   40, 2   },
    {   SYMCRYPT_XMSSMT_SHAKE256_40_4_192,  SYMCRYPT_XMSS_WOTSP_SHAKE256_192,   40, 4   },
    {   SYMCRYPT_XMSSMT_SHAKE256_40_8_192,  SYMCRYPT_XMSS_WOTSP_SHAKE256_192,   40, 8   },
    {   SYMCRYPT_XMSSMT_SHAKE256_60_3_192,  SYMCRYPT_XMSS_WOTSP_SHAKE256_192,   60, 3   },
    {   SYMCRYPT_XMSSMT_SHAKE256_60_6_192,  SYMCRYPT_XMSS_WOTSP_SHAKE256_192,   60, 6   },
    {   SYMCRYPT_XMSSMT_SHAKE256_60_12_192, SYMCRYPT_XMSS_WOTSP_SHAKE256_192,   60, 12  },
};

//
// Compute the number of chains for an n-byte input and its checksum
// for Winternitz parameter w (i.e., using w-bit blocks) in an OTS scheme
//
VOID
SYMCRYPT_CALL
SymCryptHbsGetWinternitzLengths(
            UINT32  n,
            UINT32  w,
    _Out_   PUINT32 puLen1,
    _Out_   PUINT32 puLen2
    )
{
    UINT32  len1;
    UINT32  len2;
    UINT32  maxChecksum;
    UINT32  msb;

    SYMCRYPT_ASSERT(n > 0);
    SYMCRYPT_ASSERT(w >= 1 && w <= 8);

    // number of w-bit digits in an n-byte input
    len1 = (8 * n + (w - 1)) / w;

    // maximum value the checksum can take (each w-bit digit can have value at most 2^w-1)
    maxChecksum = len1 * ((1 << w) - 1);

    msb = 31 - SymCryptCountLeadingZeros32(maxChecksum);

    // msb + 1 bits are required to store the maxChecksum,
    // calculate the number of w-bit blocks to represent that
    len2 = (msb + 1 + (w - 1)) / w;

    *puLen1 = len1;
    *puLen2 = len2;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssGetWotspParams(
            SYMCRYPT_XMSS_WOTSP_ALGID   id,
    _Out_   PSYMCRYPT_XMSS_PARAMS       pParams )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    for (UINT32 i = 0; i < SYMCRYPT_ARRAY_SIZE(XmssWotspParams); i++)
    {
        if (XmssWotspParams[i].wotspId == id)
        {
            SYMCRYPT_ASSERT(XmssWotspParams[i].hashIndex < SYMCRYPT_ARRAY_SIZE(XmssHashArray));
            pParams->hash = *XmssHashArray[XmssWotspParams[i].hashIndex];
            pParams->cbHashOutput = XmssWotspParams[i].n;
            pParams->nWinternitzWidth = XmssWotspParams[i].w;
            pParams->cbPrefix = XmssWotspParams[i].cbPrefix;
            goto cleanup;
        }
    }

    scError = SYMCRYPT_INVALID_ARGUMENT;

cleanup:

    return scError;
}

//
// Derive XMSS parameters that can be computed from others
//
// SYMCRYPT_XMSS_PARAMS structure must be initialized with either predefined
// or user defined parameters before this function is called.
//
VOID
SYMCRYPT_CALL
SymCryptXmssDeriveParams(
    _Inout_ PSYMCRYPT_XMSS_PARAMS pParams )
{
    SymCryptHbsGetWinternitzLengths(
        pParams->cbHashOutput,
        pParams->nWinternitzWidth,
        &pParams->len1,
        &pParams->len2);

    pParams->len = pParams->len1 + pParams->len2;

    UINT32 nChecksumBits = pParams->len2 * pParams->nWinternitzWidth;
    SYMCRYPT_ASSERT(nChecksumBits <= 32);
    pParams->nLeftShift32 = (UINT8)(32 - nChecksumBits);

    if (pParams->nLayers == 1)
    {
        // single trees have a 32-bit Idx value
        pParams->cbIdx = 4;
    }
    else
    {
        // number of bytes to store h-bits for Idx
        pParams->cbIdx = (pParams->nTotalTreeHeight + 7) / 8;
    }

    pParams->nLayerHeight = pParams->nTotalTreeHeight / pParams->nLayers;
}


//
//  Fill a SYMCRYPT_XMSS_PARAMS structure from either an XMSS algorithm ID or
//  XMSS^MT algorithm ID from predefined parameter sets.
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssParamsFromAlgIdCommon(
            UINT32                  id,
            BOOLEAN                 isMultiTree,
    _Out_   PSYMCRYPT_XMSS_PARAMS   pParams )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_INVALID_ARGUMENT;
    PCSYMCRYPT_XMSS_PARAMETER_PREDEFINED pParameters = NULL;
    SIZE_T uParameterCount;

    SymCryptWipeKnownSize(pParams, sizeof(*pParams));

    if (isMultiTree)
    {
        pParameters = XmssMtParametersPredefined;
        uParameterCount = SYMCRYPT_ARRAY_SIZE(XmssMtParametersPredefined);
    }
    else
    {
        pParameters = XmssParametersPredefined;
        uParameterCount = SYMCRYPT_ARRAY_SIZE(XmssParametersPredefined);
    }

    for (UINT32 i = 0; i < uParameterCount; i++)
    {
        if (pParameters[i].idAlg == id)
        {
            scError = SymCryptXmssGetWotspParams(pParameters[i].idWotsp, pParams);

            if (scError == SYMCRYPT_NO_ERROR)
            {
                SYMCRYPT_ASSERT(pParams->cbHashOutput <= SYMCRYPT_HASH_MAX_RESULT_SIZE);

                pParams->id = id;
                pParams->nTotalTreeHeight = pParameters[i].h;
                pParams->nLayers = pParameters[i].d;
                SymCryptXmssDeriveParams(pParams);
            }

            break;
        }
    }

    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssParamsFromAlgId(
            SYMCRYPT_XMSS_ALGID     id,
    _Out_   PSYMCRYPT_XMSS_PARAMS   pParams )
{
    return SymCryptXmssParamsFromAlgIdCommon(id, FALSE, pParams);
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssMtParamsFromAlgId(
            SYMCRYPT_XMSSMT_ALGID   id,
    _Out_   PSYMCRYPT_XMSS_PARAMS   pParams)
{
    return SymCryptXmssParamsFromAlgIdCommon(id, TRUE, pParams);
}


//
//  Set custom XMSS/XMSS^MT parameters
//
//  This function can be used to initialize SYMCRYPT_XMSS_PARAMS with
//  custom parameters that are not defined by the standards.
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssSetParams(
    _Out_   PSYMCRYPT_XMSS_PARAMS   pParams,
            UINT32                  id,
    _In_    PCSYMCRYPT_HASH         pHash,              // hash algorithm
            UINT32                  cbHashOutput,       // hash output size
            UINT32                  nWinternitzWidth,   // Winternitz parameter
            UINT32                  nTotalTreeHeight,   // total tree height
            UINT32                  nLayers,            // number of layers
            UINT32                  cbPrefix            // domain separator prefix length
    )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    if (pParams == NULL)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    SymCryptWipeKnownSize(pParams, sizeof(*pParams));

    if (pHash == NULL)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Output size n can at most be equal to the hash output size
    if (cbHashOutput == 0 ||
        cbHashOutput > pHash->resultSize ||
        cbHashOutput > SYMCRYPT_HASH_MAX_RESULT_SIZE)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Winternitz parameter must be one of 1, 2, 4, or 8
    if (nWinternitzWidth == 0 ||
        nWinternitzWidth > 8 ||
        (nWinternitzWidth & (nWinternitzWidth - 1)) != 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // nTotalTreeHeight and nLayers must both be positive and
    // nLayers must divide nTotalTreeHeight
    if (nTotalTreeHeight == 0 ||
        nLayers == 0 ||
        (nTotalTreeHeight % nLayers) != 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Layer height (tree height of one layer) can be at most 31
    if ((nTotalTreeHeight / nLayers) > 31)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Total tree height can be at most 63
    if (nTotalTreeHeight > 63)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (cbPrefix == 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pParams->id = id;
    pParams->hash = pHash;
    pParams->cbHashOutput = cbHashOutput;
    pParams->nWinternitzWidth = nWinternitzWidth;
    pParams->nTotalTreeHeight = nTotalTreeHeight;
    pParams->nLayers = nLayers;
    SymCryptXmssDeriveParams(pParams);

    pParams->cbPrefix = cbPrefix;

cleanup:

    return scError;
}


//
//  Updates the type field in ADRS structure and clears the
//  subsequent fields.
//
//  Does not modify the first two fields (Layer and Tree) of
//  the ADRS structure.
//
VOID
SYMCRYPT_CALL
SymCryptXmssSetAdrsType(
    _Out_   PXMSS_ADRS  adrs,
            UINT32      type )
{
    SYMCRYPT_STORE_MSBFIRST32(adrs->en32Type, type);
    SymCryptWipeKnownSize(&adrs->u, sizeof(adrs->u));
    SYMCRYPT_STORE_MSBFIRST32(adrs->en32KeyAndMask, 0);
}


SIZE_T
SYMCRYPT_CALL
SymCryptXmssSizeofSignatureFromParams(
    _In_ PCSYMCRYPT_XMSS_PARAMS pParams )
{
    SYMCRYPT_ASSERT(pParams->nLayers != 0);
    SYMCRYPT_ASSERT((pParams->nTotalTreeHeight % pParams->nLayers) == 0);
    SYMCRYPT_ASSERT(pParams->nLayerHeight > 0);

    SIZE_T size = 0;
    size += pParams->cbIdx;         // idx
    size += pParams->cbHashOutput;  // randomness

    // WOTSP signature + authentication path for each layer
    size += pParams->nLayers * ( pParams->cbHashOutput * (pParams->len + pParams->nLayerHeight) );

    return size;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssSizeofKeyBlobFromParams(
    _In_    PCSYMCRYPT_XMSS_PARAMS  pParams,
            SYMCRYPT_XMSSKEY_TYPE   keyType,
    _Out_   SIZE_T*                 pcbKey )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SIZE_T cbPublicKey = 0;
    SIZE_T cbPrivateKey = 0;

    // Public Key
    cbPublicKey += sizeof(UINT32);             // Alg ID
    cbPublicKey += 2 * pParams->cbHashOutput;  // Root and Seed

    // Private Key (on top of the public key)
    cbPrivateKey = cbPublicKey;
    cbPrivateKey += sizeof(UINT64);             // Idx
    cbPrivateKey += 2 * pParams->cbHashOutput;  // SK_XMSS and SK_PRF

    switch (keyType)
    {
        case SYMCRYPT_XMSSKEY_TYPE_PUBLIC:
            *pcbKey = cbPublicKey;
            break;

        case SYMCRYPT_XMSSKEY_TYPE_PRIVATE:
            *pcbKey = cbPrivateKey;
            break;

        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            break;
    }

    return scError;
}

PSYMCRYPT_XMSS_KEY
SYMCRYPT_CALL
SymCryptXmsskeyAllocate(
    _In_    PCSYMCRYPT_XMSS_PARAMS  pParams,
            UINT32                  flags )
{
    PSYMCRYPT_XMSS_KEY pKey = NULL;

    // No flags allowed
    if (flags != 0)
    {
        goto cleanup;
    }

    SIZE_T cbSize = sizeof(SYMCRYPT_XMSS_KEY);

    pKey = SymCryptCallbackAlloc(cbSize);

    if (pKey == NULL)
    {
        goto cleanup;
    }

    SymCryptWipe(pKey, cbSize);
    pKey->version = 1;
    pKey->keyType = SYMCRYPT_XMSSKEY_TYPE_NONE;
    pKey->params = *pParams;

    SYMCRYPT_SET_MAGIC(pKey);

 cleanup:

    return pKey;
}


VOID
SYMCRYPT_CALL
SymCryptXmsskeyFree(
    _Inout_ PSYMCRYPT_XMSS_KEY pKey )
{
    SYMCRYPT_CHECK_MAGIC(pKey);
    SymCryptWipeKnownSize(pKey, sizeof(*pKey));
    SymCryptCallbackFree(pKey);
}


PSYMCRYPT_INCREMENTAL_TREEHASH
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashInit(
    UINT32  nLeaves,
    PBYTE   pbBuffer,
    SIZE_T  cbBuffer,
    UINT32  cbHashResult,
    PSYMCRYPT_INCREMENTAL_TREEHASH_FUNC funcCompressNodes,
    PSYMCRYPT_XMSS_INCREMENTAL_TREEHASH_CONTEXT pContext )
{
    UNREFERENCED_PARAMETER(cbBuffer);

    SYMCRYPT_ASSERT(cbBuffer >= SymCryptHbsSizeofScratchBytesForIncrementalTreehash(cbHashResult, nLeaves));

    PSYMCRYPT_INCREMENTAL_TREEHASH pIncHash = (PSYMCRYPT_INCREMENTAL_TREEHASH)pbBuffer;

    pIncHash->cbNode = 2 * sizeof(UINT32) + cbHashResult;
    pIncHash->nSize = 0;
    pIncHash->nCapacity = SymCryptHbsIncrementalTreehashStackDepth(nLeaves);
    pIncHash->nLastLeafIndex = 0;
    pIncHash->funcCompressNodes = funcCompressNodes;
    pIncHash->pContext = pContext;

    return pIncHash;
}


PSYMCRYPT_TREEHASH_NODE
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashGetNode(
    _In_ PSYMCRYPT_INCREMENTAL_TREEHASH pIncHash,
         SIZE_T                         index )
{
    PBYTE pNode = (PBYTE)pIncHash->arrNodes;

    pNode += index * pIncHash->cbNode;

    return (PSYMCRYPT_TREEHASH_NODE)pNode;
}


PSYMCRYPT_TREEHASH_NODE
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashAllocNode(
    _Inout_ PSYMCRYPT_INCREMENTAL_TREEHASH  pIncHash,
            UINT32                          nLeafIndex )
{
    SYMCRYPT_ASSERT(pIncHash->nSize < pIncHash->nCapacity);

    PSYMCRYPT_TREEHASH_NODE pNode = SymCryptHbsIncrementalTreehashGetNode(pIncHash, pIncHash->nSize);

    pNode->height = 0;
    pNode->index = nLeafIndex;

    pIncHash->nSize++;

    return pNode;
}


VOID
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashGetTopNodes(
    _Inout_ PSYMCRYPT_INCREMENTAL_TREEHASH  pIncHash,
    _Out_   PSYMCRYPT_TREEHASH_NODE         *ppNodeLeft,
    _Out_   PSYMCRYPT_TREEHASH_NODE         *ppNodeRight )
{
    *ppNodeRight = (pIncHash->nSize < 1) ? NULL : SymCryptHbsIncrementalTreehashGetNode(pIncHash, pIncHash->nSize - 1);

    *ppNodeLeft = (pIncHash->nSize < 2) ? NULL : SymCryptHbsIncrementalTreehashGetNode(pIncHash, pIncHash->nSize - 2);
}


PSYMCRYPT_TREEHASH_NODE
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashProcessCommon(
    _Inout_ PSYMCRYPT_INCREMENTAL_TREEHASH  pIncHash,
            BOOLEAN                         fFinal )
{
    PSYMCRYPT_TREEHASH_NODE pNodeLeft = NULL;
    PSYMCRYPT_TREEHASH_NODE pNodeRight = NULL;

    SYMCRYPT_ASSERT(pIncHash->nSize > 0);

    SymCryptHbsIncrementalTreehashGetTopNodes(pIncHash, &pNodeLeft, &pNodeRight);

    while ( pNodeLeft &&
            (fFinal || (pNodeLeft->height == pNodeRight->height)) )
    {
        pIncHash->funcCompressNodes(
            pNodeLeft,
            pNodeRight,
            pNodeLeft,
            pIncHash->pContext);

        pIncHash->nSize--;

        SymCryptHbsIncrementalTreehashGetTopNodes(pIncHash, &pNodeLeft, &pNodeRight);
    }

    return pNodeRight;
}


PSYMCRYPT_TREEHASH_NODE
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashProcess(
    _Inout_ PSYMCRYPT_INCREMENTAL_TREEHASH pIncHash)
{
    return SymCryptHbsIncrementalTreehashProcessCommon(pIncHash, FALSE);
}


PSYMCRYPT_TREEHASH_NODE
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashFinalize(
    _Inout_ PSYMCRYPT_INCREMENTAL_TREEHASH  pIncHash )
{
    return SymCryptHbsIncrementalTreehashProcessCommon(pIncHash, TRUE);
}


UINT32
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashStackDepth(
    UINT32 nLeaves)
{
    UINT32 h;

    // Minimum height binary tree that contains nLeaves many leaves is h+1
    h = 31 - SymCryptCountLeadingZeros32(nLeaves);

    // Tree root computation will require a stack of depth equal to tree height plus 1
    return (h + 2);
}


SIZE_T
SYMCRYPT_CALL
SymCryptHbsSizeofScratchBytesForIncrementalTreehash(
    UINT32  cbNode,
    UINT32  nLeaves)
{
    SIZE_T nodeSize = cbNode + 2 * sizeof(UINT32);
    SIZE_T result = (sizeof(SYMCRYPT_INCREMENTAL_TREEHASH) - sizeof(SYMCRYPT_TREEHASH_NODE));

    result += nodeSize * SymCryptHbsIncrementalTreehashStackDepth(nLeaves);
    return result;
}


VOID
SYMCRYPT_CALL
SymCryptXmssPrfInit(
    _In_    PCSYMCRYPT_HASH         hash,
            BYTE                    PrfType,
            SIZE_T                  prefixLength,
    _Out_   PSYMCRYPT_HASH_STATE    state )
{
    BYTE prefix[SYMCRYPT_XMSS_MAX_PREFIX_SIZE];

    SYMCRYPT_ASSERT(prefixLength <= SYMCRYPT_XMSS_MAX_PREFIX_SIZE);

    SymCryptWipe(prefix, prefixLength);
    prefix[prefixLength - 1] = PrfType;

    SymCryptHashInit(hash, state);
    SymCryptHashAppend(hash, state, prefix, prefixLength);
}


VOID
SYMCRYPT_CALL
SymCryptXmssPrfKey(
    _In_                        PCSYMCRYPT_XMSS_PARAMS  pParams,
    _In_reads_bytes_( cbKey )   PCBYTE                  pbKey,
                                SIZE_T                  cbKey,
    _Out_                       SYMCRYPT_HASH_STATE     *pState )
{
    SymCryptXmssPrfInit(pParams->hash, SYMCRYPT_XMSS_PRF, pParams->cbPrefix, pState);
    SymCryptHashAppend(pParams->hash, pState, pbKey, cbKey);
}

VOID
SYMCRYPT_CALL
SymCryptXmssPrf(
    _In_                                        PCSYMCRYPT_XMSS_PARAMS  pParams,
                                                BYTE                    PrfType,
    _In_reads_bytes_( cbKey )                   PCBYTE                  pbKey,
                                                SIZE_T                  cbKey,
    _In_reads_bytes_( cbMsg )                   PCBYTE                  pbMsg,
                                                SIZE_T                  cbMsg,
    _Out_writes_bytes_( pParams->cbHashOutput ) PBYTE                   pbOutput )
{
    SYMCRYPT_HASH_STATE state;

    SymCryptXmssPrfInit(pParams->hash, PrfType, pParams->cbPrefix, &state);
    SymCryptHashAppend(pParams->hash, &state, pbKey, cbKey);
    SymCryptHashAppend(pParams->hash, &state, pbMsg, cbMsg);
    SymCryptHashResult(pParams->hash, &state, pbOutput, pParams->cbHashOutput);
}


VOID
SYMCRYPT_CALL
SymCryptXmssRandHash(
    _In_                                        PCSYMCRYPT_XMSS_PARAMS  pParams,
    _Inout_                                     XMSS_ADRS              *adrs,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbSeed,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbLeft,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbRight,
    _Out_writes_bytes_( pParams->cbHashOutput ) PBYTE                   pbOutput )
{
    BYTE key[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    BYTE bitmask[2 * SYMCRYPT_HASH_MAX_RESULT_SIZE];
    SYMCRYPT_HASH_STATE stateKeyed;
    SYMCRYPT_HASH_STATE stateMask;

    SYMCRYPT_ASSERT(pParams->cbHashOutput <= SYMCRYPT_HASH_MAX_RESULT_SIZE);

    SymCryptXmssPrfKey(pParams, pbSeed, pParams->cbHashOutput, &stateKeyed);

    SYMCRYPT_STORE_MSBFIRST32(adrs->en32KeyAndMask, 1);
    SymCryptHashStateCopy(pParams->hash, &stateKeyed, &stateMask);
    SymCryptHashAppend(pParams->hash, &stateMask, (PCBYTE)adrs, sizeof(*adrs));
    SymCryptHashResult(pParams->hash, &stateMask, &bitmask[0], pParams->cbHashOutput);

    SYMCRYPT_STORE_MSBFIRST32(adrs->en32KeyAndMask, 2);
    SymCryptHashStateCopy(pParams->hash, &stateKeyed, &stateMask);
    SymCryptHashAppend(pParams->hash, &stateMask, (PCBYTE)adrs, sizeof(*adrs));
    SymCryptHashResult(pParams->hash, &stateMask, &bitmask[pParams->cbHashOutput], pParams->cbHashOutput);

    SYMCRYPT_STORE_MSBFIRST32(adrs->en32KeyAndMask, 0);
    SymCryptHashAppend(pParams->hash, &stateKeyed, (PCBYTE)adrs, sizeof(*adrs));
    SymCryptHashResult(pParams->hash, &stateKeyed, key, pParams->cbHashOutput);

    SymCryptXorBytes(&bitmask[0], pbLeft, &bitmask[0], pParams->cbHashOutput);
    SymCryptXorBytes(&bitmask[pParams->cbHashOutput], pbRight, &bitmask[pParams->cbHashOutput], pParams->cbHashOutput);

    SymCryptXmssPrf(pParams, SYMCRYPT_XMSS_H, key, pParams->cbHashOutput, bitmask, 2 * pParams->cbHashOutput, pbOutput);
}


VOID
SYMCRYPT_CALL
SymCryptXmssTreeNodeCompress(
    _In_    PSYMCRYPT_TREEHASH_NODE                     pNodeLeft,
    _In_    PSYMCRYPT_TREEHASH_NODE                     pNodeRight,
    _Out_   PSYMCRYPT_TREEHASH_NODE                     pNodeOut,
    _Inout_ PSYMCRYPT_XMSS_INCREMENTAL_TREEHASH_CONTEXT pCtxIncHash )
{
    SYMCRYPT_STORE_MSBFIRST32(pCtxIncHash->adrs.u.hashtree.en32Height, pNodeLeft->height);
    SYMCRYPT_STORE_MSBFIRST32(pCtxIncHash->adrs.u.hashtree.en32Index, pNodeLeft->index / 2);

    SymCryptXmssRandHash(
        pCtxIncHash->pParams,
        &pCtxIncHash->adrs,
        pCtxIncHash->pbSeed,
        pNodeLeft->value,
        pNodeRight->value,
        pNodeOut->value);

    pNodeOut->index = pNodeLeft->index / 2;
    pNodeOut->height = pNodeLeft->height + 1;
}

VOID
SYMCRYPT_CALL
SymCryptXmssLtreeNodeCompress(
    _In_    PSYMCRYPT_TREEHASH_NODE                     pNodeLeft,
    _In_    PSYMCRYPT_TREEHASH_NODE                     pNodeRight,
    _Out_   PSYMCRYPT_TREEHASH_NODE                     pNodeOut,
    _Inout_ PSYMCRYPT_XMSS_INCREMENTAL_TREEHASH_CONTEXT pCtxIncHash )
{
    SYMCRYPT_STORE_MSBFIRST32(pCtxIncHash->adrs.u.ltree.en32Height, pNodeLeft->height);
    SYMCRYPT_STORE_MSBFIRST32(pCtxIncHash->adrs.u.ltree.en32Index, pNodeLeft->index / 2);

    SymCryptXmssRandHash(
        pCtxIncHash->pParams,
        &pCtxIncHash->adrs,
        pCtxIncHash->pbSeed,
        pNodeLeft->value,
        pNodeRight->value,
        pNodeOut->value);

    pNodeOut->index = pNodeLeft->index / 2;
    pNodeOut->height = pNodeLeft->height + 1;
}

VOID
SYMCRYPT_CALL
SymCryptXmssCreateWotspSecret(
    _In_                                        PCSYMCRYPT_XMSS_PARAMS  pParams,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbSkXmss,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbSeed,
    _Inout_                                     XMSS_ADRS               *adrs,
    _Out_writes_bytes_( pParams->cbHashOutput ) PBYTE                   pbOutput )
{
    SYMCRYPT_HASH_STATE state;

    SymCryptXmssPrfInit(pParams->hash, SYMCRYPT_XMSS_PRF_KEYGEN, pParams->cbPrefix, &state);
    SymCryptHashAppend(pParams->hash, &state, pbSkXmss, pParams->cbHashOutput);
    SymCryptHashAppend(pParams->hash, &state, pbSeed, pParams->cbHashOutput);
    SymCryptHashAppend(pParams->hash, &state, (PCBYTE)adrs, sizeof(*adrs));
    SymCryptHashResult(pParams->hash, &state, pbOutput, pParams->cbHashOutput);
}

VOID
SYMCRYPT_CALL
SymCryptXmssChain(
    _In_                                        PCSYMCRYPT_XMSS_PARAMS  pParams,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbInput,
                                                UINT32                  startIndex,
                                                UINT32                  steps,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbSeed,
    _Inout_                                     XMSS_ADRS              *adrs,
    _Out_writes_bytes_( pParams->cbHashOutput ) PBYTE                   pbOutput )
{
    BYTE tmp[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    BYTE key[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    BYTE bm[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    SYMCRYPT_HASH_STATE stateKey;
    SYMCRYPT_HASH_STATE stateMask;

    memcpy(tmp, pbInput, pParams->cbHashOutput);

    for (UINT32 i = startIndex; i < startIndex + steps; i++)
    {
        SYMCRYPT_STORE_MSBFIRST32(adrs->u.ots.en32Hash, i);

        SymCryptXmssPrfKey(pParams, pbSeed, pParams->cbHashOutput, &stateKey);
        SymCryptHashStateCopy(pParams->hash, &stateKey, &stateMask);

        SYMCRYPT_STORE_MSBFIRST32(adrs->en32KeyAndMask, 0);
        SymCryptHashAppend(pParams->hash, &stateKey, (PCBYTE)adrs, sizeof(*adrs));
        SymCryptHashResult(pParams->hash, &stateKey, key, pParams->cbHashOutput);

        SYMCRYPT_STORE_MSBFIRST32(adrs->en32KeyAndMask, 1);
        SymCryptHashAppend(pParams->hash, &stateMask, (PCBYTE)adrs, sizeof(*adrs));
        SymCryptHashResult(pParams->hash, &stateMask, bm, pParams->cbHashOutput);

        SymCryptXorBytes(tmp, bm, tmp, pParams->cbHashOutput);

        SymCryptXmssPrf(pParams, SYMCRYPT_XMSS_F, key, pParams->cbHashOutput, tmp, pParams->cbHashOutput, tmp);
    }

    // reset used ADRS fields
    SYMCRYPT_STORE_MSBFIRST32(adrs->u.ots.en32Hash, 0);
    SYMCRYPT_STORE_MSBFIRST32(adrs->en32KeyAndMask, 0);

    memcpy(pbOutput, tmp, pParams->cbHashOutput);
}


VOID
SYMCRYPT_CALL
SymCryptXmssCreateWotspPublickey(
    _In_                                        PCSYMCRYPT_XMSS_PARAMS  pParams,
    _Inout_                                     XMSS_ADRS               *adrs,
                                                UINT32                  uLeaf,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbSkXmss,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbSeed,
    _Out_writes_bytes_opt_( cbScratch )         PBYTE                   pbScratch,
                                                SIZE_T                  cbScratch,
    _Out_writes_bytes_( pParams->cbHashOutput ) PBYTE                   pbOutput )
{
    PSYMCRYPT_INCREMENTAL_TREEHASH pIncHash = NULL;
    PSYMCRYPT_TREEHASH_NODE pNode = NULL;
    SYMCRYPT_XMSS_INCREMENTAL_TREEHASH_CONTEXT ctxIncHash;

    SYMCRYPT_ASSERT(cbScratch >= SymCryptHbsSizeofScratchBytesForIncrementalTreehash(pParams->cbHashOutput, pParams->len));

    SymCryptXmssSetAdrsType(adrs, XMSS_ADRS_TYPE_LTREE);
    SYMCRYPT_STORE_MSBFIRST32(adrs->u.ltree.en32Leaf, uLeaf);

    ctxIncHash.adrs = *adrs;
    ctxIncHash.pParams = pParams;
    ctxIncHash.pbSeed = pbSeed;

    pIncHash = SymCryptHbsIncrementalTreehashInit(
                    pParams->len,
                    pbScratch,
                    cbScratch,
                    pParams->cbHashOutput,
                    SymCryptXmssLtreeNodeCompress,
                    &ctxIncHash);

    for (UINT32 i = 0; i < pParams->len; i++)
    {
        pNode = SymCryptHbsIncrementalTreehashAllocNode(pIncHash, i);

        SymCryptXmssSetAdrsType(adrs, XMSS_ADRS_TYPE_OTS);
        SYMCRYPT_STORE_MSBFIRST32(adrs->u.ots.en32Leaf, uLeaf);
        SYMCRYPT_STORE_MSBFIRST32(adrs->u.ots.en32Chain, i);

        SymCryptXmssCreateWotspSecret(
            pParams,
            pbSkXmss,
            pbSeed,
            adrs,
            pNode->value);

        SymCryptXmssChain(
            pParams,
            pNode->value,
            0,
            (1 << pParams->nWinternitzWidth) - 1,
            pbSeed,
            adrs,
            pNode->value);

        SymCryptHbsIncrementalTreehashProcess(pIncHash);

    }

    pNode = SymCryptHbsIncrementalTreehashFinalize(pIncHash);

    memcpy(pbOutput, pNode->value, pParams->cbHashOutput);
}


VOID
SYMCRYPT_CALL
SymCryptXmssComputeSubtreeRoot(
    _In_                                        PCSYMCRYPT_XMSS_PARAMS  pParams,
    _In_                                        XMSS_ADRS               *adrs,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbSkXmss,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbSeed,
                                                UINT32                  uLeaf,
                                                UINT32                  uHeight,
    _Out_writes_bytes_opt_( cbScratch )         PBYTE                   pbScratch,
                                                SIZE_T                  cbScratch,
    _Out_writes_bytes_( pParams->cbHashOutput ) PBYTE                   pbRoot )
{
    UNREFERENCED_PARAMETER(cbScratch);

    PSYMCRYPT_INCREMENTAL_TREEHASH pIncHash = NULL;
    PSYMCRYPT_TREEHASH_NODE pNode = NULL;
    SYMCRYPT_XMSS_INCREMENTAL_TREEHASH_CONTEXT ctxIncHash;

    SYMCRYPT_ASSERT((uLeaf & ((1UL << uHeight) - 1)) == 0); // uLeaf must be a multiple of 2^uHeight
    SYMCRYPT_ASSERT(pParams->nLayerHeight < 32); // Ensure nLeaves fits in 32 bits

    SIZE_T cbScratchTree = SymCryptHbsSizeofScratchBytesForIncrementalTreehash(pParams->cbHashOutput, 1ULL << pParams->nLayerHeight);
    SIZE_T cbScratchLtree = SymCryptHbsSizeofScratchBytesForIncrementalTreehash(pParams->cbHashOutput, pParams->len);

    SYMCRYPT_ASSERT(cbScratch >= (cbScratchTree + cbScratchLtree));

    PBYTE pbScratchTree = pbScratch;
    PBYTE pbScratchLtree = pbScratch + cbScratchTree;

    SymCryptXmssSetAdrsType(adrs, XMSS_ADRS_TYPE_HASH_TREE);

    ctxIncHash.adrs = *adrs;
    ctxIncHash.pParams = pParams;
    ctxIncHash.pbSeed = pbSeed;

    pIncHash = SymCryptHbsIncrementalTreehashInit(
        1ULL << uHeight,
        pbScratchTree,
        cbScratchTree,
        pParams->cbHashOutput,
        SymCryptXmssTreeNodeCompress,
        &ctxIncHash);

    for (UINT32 nLeafIndex = uLeaf; nLeafIndex < uLeaf + (1UL << uHeight); nLeafIndex++)
    {
        pNode = SymCryptHbsIncrementalTreehashAllocNode(pIncHash, nLeafIndex);

        SymCryptXmssCreateWotspPublickey(pParams,
                                    adrs,
                                    nLeafIndex,
                                    pbSkXmss,
                                    pbSeed,
                                    pbScratchLtree,
                                    cbScratchLtree,
                                    pNode->value );

        SymCryptHbsIncrementalTreehashProcess(pIncHash);
    }

    pNode = SymCryptHbsIncrementalTreehashFinalize(pIncHash);

    memcpy(pbRoot, pNode->value, pParams->cbHashOutput);
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssComputePublicRoot(
    _In_                            PCSYMCRYPT_XMSS_PARAMS  pParams,
    _In_reads_bytes_( cbSeed )      PCBYTE                  pbSeed,
                                    SIZE_T                  cbSeed,
    _In_reads_bytes_( cbSkXmss )    PCBYTE                  pbSkXmss,
                                    SIZE_T                  cbSkXmss,
    _Out_writes_bytes_( cbRoot )    PBYTE                   pbRoot,
                                    SIZE_T                  cbRoot )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE pbScratch = NULL;
    SIZE_T cbScratch = 0;
    XMSS_ADRS adrs;

    SYMCRYPT_ASSERT(pParams->nLayerHeight < 32); // Ensure nLeaves fits in 32 bits

    if (pbRoot == NULL || cbRoot != pParams->cbHashOutput ||
        pbSeed == NULL || cbSeed != pParams->cbHashOutput ||
        pbSkXmss == NULL || cbSkXmss != pParams->cbHashOutput)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbScratch += SymCryptHbsSizeofScratchBytesForIncrementalTreehash(pParams->cbHashOutput, 1ULL << pParams->nLayerHeight);
    cbScratch += SymCryptHbsSizeofScratchBytesForIncrementalTreehash(pParams->cbHashOutput, pParams->len);

    SYMCRYPT_ASSERT(cbScratch > 0);
    pbScratch = SymCryptCallbackAlloc(cbScratch);

    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    SymCryptWipeKnownSize(&adrs, sizeof(XMSS_ADRS));
    SYMCRYPT_STORE_MSBFIRST32(adrs.en32Layer, pParams->nLayers - 1);

    SymCryptXmssComputeSubtreeRoot(
        pParams,
        &adrs,
        pbSkXmss,
        pbSeed,
        0,
        pParams->nLayerHeight,
        pbScratch,
        cbScratch,
        pbRoot );

cleanup:

    if (pbScratch != NULL)
    {
        SymCryptWipe(pbScratch, cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmsskeyVerifyRoot(
    _In_    PCSYMCRYPT_XMSS_KEY pKey)
{
    BYTE Root[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SYMCRYPT_CHECK_MAGIC(pKey);

    // key to be verified has to be a private key
    if (pKey->keyType != SYMCRYPT_XMSSKEY_TYPE_PRIVATE)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    SymCryptWipeKnownSize(Root, sizeof(Root));

    scError = SymCryptXmssComputePublicRoot(
        &pKey->params,
        pKey->Seed,
        pKey->params.cbHashOutput,
        pKey->SkXmss,
        pKey->params.cbHashOutput,
        Root,
        pKey->params.cbHashOutput);

    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    if (!SymCryptEqual(Root, pKey->Root, pKey->params.cbHashOutput))
    {
        scError = SYMCRYPT_HBS_PUBLIC_ROOT_MISMATCH;
    }

cleanup:

    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmsskeyGenerate(
    _Inout_ PSYMCRYPT_XMSS_KEY  pKey,
            UINT32              flags)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SYMCRYPT_CHECK_MAGIC(pKey);

    if (flags != 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Wipe key material
    SymCryptWipeKnownSize(pKey->Root, sizeof(pKey->Root));
    SymCryptWipeKnownSize(pKey->Seed, sizeof(pKey->Seed));
    SymCryptWipeKnownSize(pKey->SkPrf, sizeof(pKey->SkPrf));
    SymCryptWipeKnownSize(pKey->SkXmss, sizeof(pKey->SkXmss));
    pKey->Idx = 0;

    scError = SymCryptCallbackRandom(pKey->SkPrf, pKey->params.cbHashOutput);

    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SymCryptCallbackRandom(pKey->SkXmss, pKey->params.cbHashOutput);

    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SymCryptCallbackRandom(pKey->Seed, pKey->params.cbHashOutput);

    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Compute public root from the private key
    scError = SymCryptXmssComputePublicRoot(
        &pKey->params,
        pKey->Seed,
        pKey->params.cbHashOutput,
        pKey->SkXmss,
        pKey->params.cbHashOutput,
        pKey->Root,
        pKey->params.cbHashOutput);

    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    pKey->keyType = SYMCRYPT_XMSSKEY_TYPE_PRIVATE;

cleanup:

    if (scError != SYMCRYPT_NO_ERROR)
    {
        SymCryptWipeKnownSize(pKey->SkPrf, sizeof(pKey->SkPrf));
        SymCryptWipeKnownSize(pKey->SkXmss, sizeof(pKey->SkXmss));
        pKey->keyType = SYMCRYPT_XMSSKEY_TYPE_NONE;
    }

    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmsskeySetValue(
    _In_reads_bytes_( cbInput ) PCBYTE                  pbInput,
                                SIZE_T                  cbInput,
                                SYMCRYPT_XMSSKEY_TYPE   keyType,
                                UINT32                  flags,
    _Inout_                     PSYMCRYPT_XMSS_KEY      pKey )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 uAlgId;
    SIZE_T cbKey;

    SYMCRYPT_ASSERT(keyType == SYMCRYPT_XMSSKEY_TYPE_PUBLIC || keyType == SYMCRYPT_XMSSKEY_TYPE_PRIVATE);

    SYMCRYPT_CHECK_MAGIC(pKey);

    if ((flags & (~SYMCRYPT_FLAG_XMSSKEY_VERIFY_ROOT)) != 0 ||
        (keyType != SYMCRYPT_XMSSKEY_TYPE_PUBLIC && keyType != SYMCRYPT_XMSSKEY_TYPE_PRIVATE))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Public root validation can only be performed for private keys
    if ((flags & SYMCRYPT_FLAG_XMSSKEY_VERIFY_ROOT) != 0 &&
        keyType != SYMCRYPT_XMSSKEY_TYPE_PRIVATE)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    SymCryptXmssSizeofKeyBlobFromParams(&pKey->params, keyType, &cbKey);

    if (cbInput != cbKey)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    uAlgId = SYMCRYPT_LOAD_MSBFIRST32(pbInput);
    pbInput += sizeof(UINT32);

    if (uAlgId != pKey->params.id)
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    // Wipe private key material
    pKey->Idx = 0;
    SymCryptWipeKnownSize(pKey->SkPrf, sizeof(pKey->SkPrf));
    SymCryptWipeKnownSize(pKey->SkXmss, sizeof(pKey->SkXmss));

    pKey->keyType = keyType;

    memcpy(pKey->Root, pbInput, pKey->params.cbHashOutput);
    pbInput += pKey->params.cbHashOutput;

    memcpy(pKey->Seed, pbInput, pKey->params.cbHashOutput);
    pbInput += pKey->params.cbHashOutput;

    if (keyType == SYMCRYPT_XMSSKEY_TYPE_PRIVATE)
    {
        pKey->Idx = SYMCRYPT_LOAD_MSBFIRST64(pbInput);
        pbInput += sizeof(UINT64);

        memcpy(pKey->SkXmss, pbInput, pKey->params.cbHashOutput);
        pbInput += pKey->params.cbHashOutput;

        memcpy(pKey->SkPrf, pbInput, pKey->params.cbHashOutput);
        pbInput += pKey->params.cbHashOutput;

        if ((flags & SYMCRYPT_FLAG_XMSSKEY_VERIFY_ROOT) != 0)
        {
            // pKey has been initialized by now
            scError = SymCryptXmsskeyVerifyRoot(pKey);

            if (scError != SYMCRYPT_NO_ERROR)
            {
                goto cleanup;
            }
        }
    }

cleanup:

    if (scError != SYMCRYPT_NO_ERROR)
    {
        SymCryptWipeKnownSize(pKey->SkPrf, sizeof(pKey->SkPrf));
        SymCryptWipeKnownSize(pKey->SkXmss, sizeof(pKey->SkXmss));
        pKey->keyType = SYMCRYPT_XMSSKEY_TYPE_NONE;
    }

    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmsskeyGetValue(
    _In_                            PCSYMCRYPT_XMSS_KEY     pKey,
                                    SYMCRYPT_XMSSKEY_TYPE   keyType,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbOutput )  PBYTE                   pbOutput,
                                    SIZE_T                  cbOutput)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SIZE_T cbKey;

    SYMCRYPT_ASSERT(keyType == SYMCRYPT_XMSSKEY_TYPE_PUBLIC || keyType == SYMCRYPT_XMSSKEY_TYPE_PRIVATE);

    SYMCRYPT_CHECK_MAGIC(pKey);

    if (flags != 0 ||
        (keyType != SYMCRYPT_XMSSKEY_TYPE_PUBLIC && keyType != SYMCRYPT_XMSSKEY_TYPE_PRIVATE) ||
        pKey->keyType == SYMCRYPT_XMSSKEY_TYPE_NONE)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Cannot export private key from a public key object
    if (keyType == SYMCRYPT_XMSSKEY_TYPE_PRIVATE && pKey->keyType != SYMCRYPT_XMSSKEY_TYPE_PRIVATE)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    SymCryptXmssSizeofKeyBlobFromParams(&pKey->params, keyType, &cbKey);

    if (cbOutput != cbKey)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    //
    // Public Key
    //

    // Alg Id
    SYMCRYPT_STORE_MSBFIRST32(pbOutput, pKey->params.id);
    pbOutput += sizeof(UINT32);

    // Root
    memcpy(pbOutput, pKey->Root, pKey->params.cbHashOutput);
    pbOutput += pKey->params.cbHashOutput;

    // Seed
    memcpy(pbOutput, pKey->Seed, pKey->params.cbHashOutput);
    pbOutput += pKey->params.cbHashOutput;

    if (keyType == SYMCRYPT_XMSSKEY_TYPE_PRIVATE)
    {
        //
        // Private Key
        //

        // Idx
        SYMCRYPT_STORE_MSBFIRST64(pbOutput, pKey->Idx);
        pbOutput += sizeof(pKey->Idx);

        // SK_XMSS
        memcpy(pbOutput, pKey->SkXmss, pKey->params.cbHashOutput);
        pbOutput += pKey->params.cbHashOutput;

        // SK_PRF
        memcpy(pbOutput, pKey->SkPrf, pKey->params.cbHashOutput);
        pbOutput += pKey->params.cbHashOutput;
    }

cleanup:

    return scError;
}


UINT32
SYMCRYPT_CALL
SymCryptHbsGetDigit(
            UINT32 width,
    _In_    PCBYTE pbBuffer,
            SIZE_T cbBuffer,
            UINT32 index )
{
    UNREFERENCED_PARAMETER(cbBuffer);

    SYMCRYPT_ASSERT(width == 1 || width == 2 || width == 4 || width == 8);
    SYMCRYPT_ASSERT(index < ((cbBuffer * 8) / width));

    UINT32 digitsPerByte = 8 / width;

    BYTE value = pbBuffer[index / digitsPerByte];

    value >>= width * (digitsPerByte - 1 - (index % digitsPerByte));

    value &= (1 << width) - 1;

    return value;
}


VOID
SYMCRYPT_CALL
SymCryptXmssTreeRootFromAuthenticationPath(
    _In_    PCSYMCRYPT_XMSS_PARAMS  pParams,
    _Inout_ XMSS_ADRS              *adrs,
            UINT32                  uLeaf,
    _In_reads_bytes_( pParams->cbHashOutput )
            PCBYTE                  pbStartingNode,
    _In_reads_bytes_( pParams->cbHashOutput * pParams->nLayerHeight )
            PCBYTE                  pbAuthNodes,
    _In_reads_bytes_( pParams->cbHashOutput )
            PCBYTE                  pbSeed,
    _Out_writes_bytes_( pParams->cbHashOutput )
            PBYTE                   pbOutput )
{
    BYTE node[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    BYTE tmp[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    UINT32 uNodeIndex = uLeaf;

    memcpy(node, pbStartingNode, pParams->cbHashOutput);

    SymCryptXmssSetAdrsType(adrs, XMSS_ADRS_TYPE_HASH_TREE);
    SYMCRYPT_STORE_MSBFIRST32(adrs->u.hashtree.en32Index, uNodeIndex);

    for (UINT32 i = 0; i < pParams->nLayerHeight; i++)
    {
        SYMCRYPT_STORE_MSBFIRST32(adrs->u.hashtree.en32Height, i);

        if ( ((uLeaf >> i) & 1) == 0 )
        {
            uNodeIndex = uNodeIndex / 2;
            SYMCRYPT_STORE_MSBFIRST32(adrs->u.hashtree.en32Index, uNodeIndex);
            SymCryptXmssRandHash(pParams, adrs, pbSeed, node, &pbAuthNodes[pParams->cbHashOutput * i], tmp);
        }
        else
        {
            uNodeIndex = (uNodeIndex - 1) / 2;
            SYMCRYPT_STORE_MSBFIRST32(adrs->u.hashtree.en32Index, uNodeIndex);
            SymCryptXmssRandHash(pParams, adrs, pbSeed, &pbAuthNodes[pParams->cbHashOutput * i], node, tmp);
        }

        memcpy(node, tmp, pParams->cbHashOutput);
    }

    memcpy(pbOutput, node, pParams->cbHashOutput);
}

VOID
SYMCRYPT_CALL
SymCryptXmssRandomizedHash(
    _In_                                        PCSYMCRYPT_XMSS_PARAMS  pParams,
                                                UINT64                  Idx,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbRandomizer,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbRoot,
    _In_reads_bytes_( pParams->cbHashOutput )   PCBYTE                  pbMsg,
                                                SIZE_T                  cbMsg,
    _Out_writes_bytes_( pParams->cbHashOutput ) PBYTE                   pbOutput )
{
    SYMCRYPT_HASH_STATE state;
    BYTE idxBuf[SYMCRYPT_HASH_MAX_RESULT_SIZE];

    SymCryptWipe(idxBuf, pParams->cbHashOutput);
    SYMCRYPT_STORE_MSBFIRST64(&idxBuf[pParams->cbHashOutput - sizeof(Idx)], Idx);

    SymCryptXmssPrfInit(pParams->hash, SYMCRYPT_XMSS_H_MSG, pParams->cbPrefix, &state);
    SymCryptHashAppend(pParams->hash, &state, pbRandomizer, pParams->cbHashOutput);
    SymCryptHashAppend(pParams->hash, &state, pbRoot, pParams->cbHashOutput);
    SymCryptHashAppend(pParams->hash, &state, idxBuf, pParams->cbHashOutput);
    SymCryptHashAppend(pParams->hash, &state, pbMsg, cbMsg);
    SymCryptHashResult(pParams->hash, &state, pbOutput, pParams->cbHashOutput);
}


VOID
SYMCRYPT_CALL
SymCryptXmssWotspPublickeyFromSignature(
    _In_    PCSYMCRYPT_XMSS_PARAMS  pParams,
    _Inout_ XMSS_ADRS              *adrs,
            UINT32                  idx,
    _In_reads_bytes_( pParams->cbHashOutput )
            PCBYTE                  pbMsg,
    _In_reads_bytes_( pParams->cbHashOutput )
            PCBYTE                  pbSeed,
    _In_reads_bytes_( pParams->cbHashOutput * pParams->len )
            PCBYTE  pbSignature,
    _Out_writes_bytes_( cbScratch )
            PBYTE                   pbScratch,
            SIZE_T                  cbScratch,
   _Out_writes_bytes_( pParams->cbHashOutput )
            PBYTE                   pbOutput )
{

    UINT32 digit;
    UINT32 checksum = 0;
    BYTE en32Checksum[4];
    const UINT32 maxChainIndex = (1 << pParams->nWinternitzWidth) - 1;
    PSYMCRYPT_INCREMENTAL_TREEHASH pIncHash = NULL;
    PSYMCRYPT_TREEHASH_NODE pNode = NULL;
    SYMCRYPT_XMSS_INCREMENTAL_TREEHASH_CONTEXT ctxIncHash;

    SymCryptXmssSetAdrsType(adrs, XMSS_ADRS_TYPE_LTREE);
    SYMCRYPT_STORE_MSBFIRST32(adrs->u.ltree.en32Leaf, idx);

    ctxIncHash.adrs = *adrs;
    ctxIncHash.pParams = pParams;
    ctxIncHash.pbSeed = pbSeed;

    pIncHash = SymCryptHbsIncrementalTreehashInit(
                    pParams->len,
                    pbScratch,
                    cbScratch,
                    pParams->cbHashOutput,
                    SymCryptXmssLtreeNodeCompress,
                    &ctxIncHash);

    for (UINT32 i = 0; i < pParams->len; i++)
    {
        if (i < pParams->len1)
        {
            digit = SymCryptHbsGetDigit(pParams->nWinternitzWidth, pbMsg, pParams->cbHashOutput, i);

            checksum += maxChainIndex - digit;
        }
        else
        {
            if (i == pParams->len1)
            {
                checksum <<= pParams->nLeftShift32;
                SYMCRYPT_STORE_MSBFIRST32(en32Checksum, checksum);
            }

            digit = SymCryptHbsGetDigit(pParams->nWinternitzWidth, en32Checksum, sizeof(en32Checksum), i - pParams->len1);
        }

        pNode = SymCryptHbsIncrementalTreehashAllocNode(pIncHash, i);

        SymCryptXmssSetAdrsType(adrs, XMSS_ADRS_TYPE_OTS);
        SYMCRYPT_STORE_MSBFIRST32(adrs->u.ots.en32Leaf, idx);
        SYMCRYPT_STORE_MSBFIRST32(adrs->u.ots.en32Chain, i);

        SymCryptXmssChain(
            pParams,
            &pbSignature[pParams->cbHashOutput * i],
            digit,
            maxChainIndex - digit,
            pbSeed,
            adrs,
            pNode->value);

        SymCryptHbsIncrementalTreehashProcess(pIncHash);
    }

    pNode = SymCryptHbsIncrementalTreehashFinalize(pIncHash);

    memcpy(pbOutput, pNode->value, pParams->cbHashOutput);
}


VOID
SYMCRYPT_CALL
SymCryptXmssTreeRootFromSignature(
    _In_    PCSYMCRYPT_XMSS_PARAMS  pParams,
    _Inout_ XMSS_ADRS               *adrs,
    _In_reads_bytes_( pParams->cbHashOutput )
            PCBYTE                  pbSeed,
    _In_reads_bytes_( pParams->cbHashOutput )
            PCBYTE                  pbHash,
            UINT32                  uLeaf,
    _In_reads_bytes_( pParams->cbHashOutput * pParams->len )
            PCBYTE                  pbWotspSig,
    _In_reads_bytes_( pParams->cbHashOutput* pParams->nLayerHeight )
            PCBYTE                  pbAuthNodes,
    _Out_writes_bytes_( pParams->cbHashOutput )
            PBYTE                   pbOutput,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE                   pbScratch,
            SIZE_T                  cbScratch )
{
    BYTE WotspPublickey[SYMCRYPT_HASH_MAX_RESULT_SIZE];

    SymCryptXmssWotspPublickeyFromSignature(
        pParams,
        adrs,
        uLeaf,
        pbHash,
        pbSeed,
        pbWotspSig,
        pbScratch,
        cbScratch,
        WotspPublickey);

    SymCryptXmssTreeRootFromAuthenticationPath(
        pParams,
        adrs,
        uLeaf,
        WotspPublickey,
        pbAuthNodes,
        pbSeed,
        pbOutput);
}

SIZE_T
SYMCRYPT_CALL
SymCryptXmssSizeofWotspSignature(_In_ PCSYMCRYPT_XMSS_PARAMS pParams)
{
    // WOTSP signature size is len = len1 + len2 many hash outputs
    return pParams->cbHashOutput * pParams->len;
}

SIZE_T
SYMCRYPT_CALL
SymCryptXmssSizeofAuthNodes(
    _In_ PCSYMCRYPT_XMSS_PARAMS pParams)
{
    // size of authentication nodes for single tree
    return pParams->cbHashOutput * pParams->nLayerHeight;
}

UINT64
SYMCRYPT_CALL
SymCryptXmssSignatureGetIdx(
    _In_ PCSYMCRYPT_XMSS_PARAMS pParams,
    _In_ PCBYTE                 pbSig )
{
    UINT64 Idx = 0;

    for (UINT8 i = 0; i < pParams->cbIdx; i++)
    {
        Idx <<= 8;
        Idx |= (UINT64)pbSig[i];
    }

    return Idx;
}

PBYTE
SYMCRYPT_CALL
SymCryptXmssSignatureGetRandomness(
    _In_ PCSYMCRYPT_XMSS_PARAMS pParams,
    _In_ PCBYTE                 pbSig )
{
    PBYTE pb = (PBYTE)pbSig;

    // randomness comes after idx
    pb += pParams->cbIdx;

    return pb;
}

PBYTE
SYMCRYPT_CALL
SymCryptXmssSignatureGetWotspSig(
    _In_    PCSYMCRYPT_XMSS_PARAMS  pParams,
    _In_    PCBYTE                  pbSig,
            UINT32                  uLayer )
{
    PBYTE pb = SymCryptXmssSignatureGetRandomness(pParams, pbSig);

    // skip randomness
    pb += pParams->cbHashOutput;

    // each layer contains WOTSP signature and AuthNodes
    pb += uLayer * (SymCryptXmssSizeofWotspSignature(pParams) + SymCryptXmssSizeofAuthNodes(pParams));

    return pb;
}

PBYTE
SYMCRYPT_CALL
SymCryptXmssSignatureGetAuthNodes(
    _In_    PCSYMCRYPT_XMSS_PARAMS  pParams,
    _In_    PCBYTE                  pbSig,
            UINT32                  uLayer )
{
    PBYTE pb = SymCryptXmssSignatureGetWotspSig(pParams, pbSig, uLayer);

    // AuthNodes follow WOTSP signature
    pb += SymCryptXmssSizeofWotspSignature(pParams);

    return pb;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssVerifyInternal(
    _Inout_                         PSYMCRYPT_XMSS_KEY  pKey,
    _In_reads_bytes_( cbMessage )   PCBYTE              pbMessage,
                                    SIZE_T              cbMessage,
                                    UINT32              flags,
    _In_reads_bytes_( cbSignature ) PCBYTE              pbSignature,
                                    SIZE_T              cbSignature )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE pbScratch = NULL;
    SIZE_T cbScratch = 0;
    PCSYMCRYPT_XMSS_PARAMS pParams = &pKey->params;
    BYTE RandomizedHash[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    BYTE ComputedRoot[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    XMSS_ADRS adrs;
    UINT32 uLayer;
    UINT64 uTree;
    UINT32 uLeaf;
    const UINT64 LeafMask = (1ULL << pParams->nLayerHeight) - 1;

    SYMCRYPT_CHECK_MAGIC(pKey);

    SYMCRYPT_ASSERT(pParams->nLayerHeight < 32); // Ensure nLeaves fits in 32 bits

    if (flags != 0 ||
        pbSignature == NULL ||
        cbSignature != SymCryptXmssSizeofSignatureFromParams(pParams) ||
        pKey->keyType == SYMCRYPT_XMSSKEY_TYPE_NONE )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbScratch += SymCryptHbsSizeofScratchBytesForIncrementalTreehash(pParams->cbHashOutput, pParams->len);
    cbScratch += SymCryptHbsSizeofScratchBytesForIncrementalTreehash(pParams->cbHashOutput, 1ULL << pParams->nLayerHeight);

    pbScratch = (PBYTE)SymCryptCallbackAlloc(cbScratch);

    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    PBYTE pbRandomness = SymCryptXmssSignatureGetRandomness(pParams, pbSignature);
    UINT64 Idx = SymCryptXmssSignatureGetIdx(pParams, pbSignature);

    SymCryptXmssRandomizedHash(
        pParams,
        Idx,
        pbRandomness,
        pKey->Root,
        pbMessage,
        cbMessage,
        RandomizedHash);

    SymCryptWipeKnownSize(&adrs, sizeof(XMSS_ADRS));

    for (uLayer = 0; uLayer < pParams->nLayers; uLayer++)
    {
        uTree = Idx >> pParams->nLayerHeight;
        uLeaf = (UINT32)(Idx & LeafMask);

        SYMCRYPT_STORE_MSBFIRST32(adrs.en32Layer, uLayer);
        SYMCRYPT_STORE_MSBFIRST64(adrs.en64Tree, uTree);
        SymCryptXmssTreeRootFromSignature(
            pParams,
            &adrs,
            pKey->Seed,
            uLayer == 0 ? RandomizedHash : ComputedRoot,
            uLeaf,
            SymCryptXmssSignatureGetWotspSig(pParams, pbSignature, uLayer),
            SymCryptXmssSignatureGetAuthNodes(pParams, pbSignature, uLayer),
            ComputedRoot,
            pbScratch,
            cbScratch);

        Idx >>= pParams->nLayerHeight;
    }

    if (!SymCryptEqual(ComputedRoot, pKey->Root, pParams->cbHashOutput))
    {
        scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
        goto cleanup;
    }

cleanup:

    if (pbScratch)
    {
        SymCryptWipe(pbScratch, cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssVerify(
    _Inout_                         PSYMCRYPT_XMSS_KEY  pKey,
    _In_reads_bytes_( cbMessage )   PCBYTE              pbMessage,
                                    SIZE_T              cbMessage,
                                    UINT32              flags,
    _In_reads_bytes_( cbSignature ) PCBYTE              pbSignature,
                                    SIZE_T              cbSignature )
{
    SYMCRYPT_RUN_SELFTEST_ONCE(
        SymCryptXmssSelftest,
        SYMCRYPT_SELFTEST_ALGORITHM_XMSS);

    return SymCryptXmssVerifyInternal(
        pKey,
        pbMessage,
        cbMessage,
        flags,
        pbSignature,
        cbSignature);
}

VOID
SYMCRYPT_CALL
SymCryptXmssWotspSign(
    _In_                                                        PCSYMCRYPT_XMSS_PARAMS  pParams,
    _Inout_                                                     XMSS_ADRS*              adrs,
    _In_reads_bytes_( pParams->cbHashOutput )                   PCBYTE                  pbInput,
                                                                UINT32                  uLeaf,
    _In_reads_bytes_( pParams->cbHashOutput )                   PCBYTE                  pbSeed,
    _In_reads_bytes_( pParams->cbHashOutput )                   PCBYTE                  pbSkXmss,
    _Out_writes_bytes_( pParams->cbHashOutput * pParams->len )  PBYTE                   pbOutput )
{
    BYTE node[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    UINT32 nChecksum = 0;
    BYTE en32Checksum[sizeof(UINT32)];
    UINT32 digit;
    const UINT32 maxChainIndex = (1UL << pParams->nWinternitzWidth) - 1;


    SymCryptXmssSetAdrsType(adrs, XMSS_ADRS_TYPE_OTS);
    SYMCRYPT_STORE_MSBFIRST32(adrs->u.ots.en32Leaf, uLeaf);

    for (UINT32 i = 0; i < pParams->len; i++)
    {
        SYMCRYPT_STORE_MSBFIRST32(adrs->u.ots.en32Chain, i);
        SymCryptXmssCreateWotspSecret(
            pParams,
            pbSkXmss,
            pbSeed,
            adrs,
            node);

        if (i < pParams->len1)
        {
            digit = SymCryptHbsGetDigit(pParams->nWinternitzWidth, pbInput, pParams->cbHashOutput, i);

            nChecksum += maxChainIndex - digit;
        }
        else
        {
            if (i == pParams->len1)
            {
                nChecksum <<= pParams->nLeftShift32;
                SYMCRYPT_STORE_MSBFIRST32(en32Checksum, nChecksum);
            }

            digit = SymCryptHbsGetDigit(pParams->nWinternitzWidth, en32Checksum, sizeof(en32Checksum), i - pParams->len1);
        }

        SymCryptXmssChain(
            pParams,
            node,
            0,
            digit,
            pbSeed,
            adrs,
            &pbOutput[i * pParams->cbHashOutput]);
    }

    SymCryptWipeKnownSize(node, sizeof(node));
}


VOID
SYMCRYPT_CALL
SymCryptXmssTreeSignHash(
    _In_    PCSYMCRYPT_XMSS_PARAMS  pParams,
    _Inout_ XMSS_ADRS               *adrs,
    _In_reads_bytes_( pParams->cbHashOutput )
            PCBYTE                  pbSkXmss,
    _In_reads_bytes_( pParams->cbHashOutput )
            PCBYTE                  pbSeed,
    _In_reads_bytes_( pParams->cbHashOutput )
            PCBYTE                  pbHash,
            UINT32                  Idx,
    _Out_writes_bytes_( pParams->cbHashOutput * pParams->len )
            PBYTE                   pbWotspSig,
    _Out_writes_bytes_( pParams->cbHashOutput * pParams->nLayerHeight )
            PBYTE                   pbAuthNodes,
    _Out_writes_bytes_opt_( pParams->cbHashOutput )
            PBYTE                   pbRoot,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE                   pbScratch,
            SIZE_T                  cbScratch )
{
    BYTE WotspPublicKey[SYMCRYPT_HASH_MAX_RESULT_SIZE];

    SymCryptXmssWotspSign(
        pParams,
        adrs,
        pbHash,
        Idx,
        pbSeed,
        pbSkXmss,
        pbWotspSig );

    // Generate authentication path
    for (UINT32 h = 0; h < pParams->nLayerHeight; h++)
    {
        UINT32 uLeaf = ((Idx >> h) ^ 1UL) << h;
        SymCryptXmssComputeSubtreeRoot(
            pParams,
            adrs,
            pbSkXmss,
            pbSeed,
            uLeaf,
            h,
            pbScratch,
            cbScratch,
            &pbAuthNodes[h * pParams->cbHashOutput]);
    }

    //
    // Calculate tree root if requested by the caller
    //
    // This is used to return the tree root to be signed with the upper
    // layer in XMSS^MT.
    if (pbRoot)
    {
        SymCryptXmssCreateWotspPublickey(
            pParams,
            adrs,
            Idx,
            pbSkXmss,
            pbSeed,
            pbScratch,
            cbScratch,
            WotspPublicKey);

        SymCryptXmssTreeRootFromAuthenticationPath(
            pParams,
            adrs,
            Idx,
            WotspPublicKey,
            pbAuthNodes,
            pbSeed,
            pbRoot);
    }
}

//
// Compute randomness for randomized hashing
//
VOID
SYMCRYPT_CALL
SymCryptXmssComputeRandomness(
    _In_                                            PCSYMCRYPT_XMSS_KEY pKey,
                                                    UINT64              Idx,
    _Out_writes_bytes_( pKey->params.cbHashOutput ) PBYTE               pbRandomness )
{
    BYTE IdxBuffer[32];

    SymCryptWipeKnownSize(IdxBuffer, sizeof(IdxBuffer));
    SYMCRYPT_STORE_MSBFIRST64(IdxBuffer + sizeof(IdxBuffer) - sizeof(Idx), Idx);

    SymCryptXmssPrf(
        &pKey->params,
        SYMCRYPT_XMSS_PRF,
        pKey->SkPrf,
        pKey->params.cbHashOutput,
        IdxBuffer,
        sizeof(IdxBuffer),
        pbRandomness);
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssSign(
    _Inout_                             PSYMCRYPT_XMSS_KEY  pKey,
    _In_reads_bytes_( cbMessage )       PCBYTE              pbMessage,
                                        SIZE_T              cbMessage,
                                        UINT32              flags,
    _Out_writes_bytes_( cbSignature )   PBYTE               pbSignature,
                                        SIZE_T              cbSignature )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE pbScratch = NULL;
    SIZE_T cbScratch = 0;
    PSYMCRYPT_XMSS_PARAMS pParams = &pKey->params;
    UINT64 Idx;
    BYTE en64Idx[sizeof(UINT64)];
    BYTE Randomness[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    BYTE RandomizedHash[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    BYTE TreeRoot[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    XMSS_ADRS adrs;
    UINT32 uLayer;
    UINT64 uTree;
    UINT32 uLeaf;
    const UINT64 LeafMask = (1ULL << pParams->nLayerHeight) - 1;

    SYMCRYPT_CHECK_MAGIC(pKey);

    SYMCRYPT_ASSERT(pParams->nLayerHeight < 32); // Ensure nLeaves fits in 32 bits

    if (flags != 0 ||
        pbSignature == NULL ||
        cbSignature != SymCryptXmssSizeofSignatureFromParams(pParams) ||
        pKey->keyType != SYMCRYPT_XMSSKEY_TYPE_PRIVATE )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbScratch += SymCryptHbsSizeofScratchBytesForIncrementalTreehash(pParams->cbHashOutput, pParams->len);        // Ltree hashing
    cbScratch += SymCryptHbsSizeofScratchBytesForIncrementalTreehash(pParams->cbHashOutput, 1ULL << pParams->nLayerHeight);  // Merkle-tree hashing

    pbScratch = (PBYTE)SymCryptCallbackAlloc(cbScratch);

    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    Idx = SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(&pKey->Idx, 1) - 1;
    if (Idx >= (1ULL << pParams->nTotalTreeHeight))
    {
        // Set Idx to first unusable value
        pKey->Idx = (1ULL << pParams->nTotalTreeHeight);

        scError = SYMCRYPT_HBS_NO_OTS_KEYS_LEFT;
        goto cleanup;
    }

    SYMCRYPT_STORE_MSBFIRST64(en64Idx, Idx);
    memcpy(pbSignature, &en64Idx[sizeof(en64Idx) - pParams->cbIdx], pParams->cbIdx);

    SymCryptXmssComputeRandomness(pKey, Idx, Randomness);
    memcpy(SymCryptXmssSignatureGetRandomness(pParams, pbSignature), Randomness, pKey->params.cbHashOutput);

    SymCryptXmssRandomizedHash(&pKey->params, Idx, Randomness, pKey->Root, pbMessage, cbMessage, RandomizedHash);

    SymCryptWipeKnownSize(&adrs, sizeof(XMSS_ADRS));

    for (uLayer = 0; uLayer < pParams->nLayers; uLayer++)
    {
        uTree = Idx >> pParams->nLayerHeight;
        uLeaf = (UINT32)(Idx & LeafMask);

        SYMCRYPT_STORE_MSBFIRST32(adrs.en32Layer, uLayer);
        SYMCRYPT_STORE_MSBFIRST64(adrs.en64Tree, uTree);

        SymCryptXmssTreeSignHash(
            &pKey->params,
            &adrs,
            pKey->SkXmss,
            pKey->Seed,
            uLayer == 0 ? RandomizedHash : TreeRoot,
            uLeaf,
            SymCryptXmssSignatureGetWotspSig(pParams, pbSignature, uLayer),
            SymCryptXmssSignatureGetAuthNodes(pParams, pbSignature, uLayer),
            uLayer == (UINT32)(pParams->nLayers - 1) ? NULL : TreeRoot, // No need to compute the root for the top layer tree
            pbScratch,
            cbScratch);

        Idx >>= pParams->nLayerHeight;
    }

    if (scError != SYMCRYPT_NO_ERROR)
    {
        SymCryptWipe(pbSignature, cbSignature);
        goto cleanup;
    }

cleanup:

    if (pbScratch)
    {
        SymCryptWipe(pbScratch, cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}
