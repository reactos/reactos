//
// mldsa_primitives.c   ML-DSA low-level primitive implementations
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// Q^-1 mod 2^32 - used in Montgomery reduction
//
#define SYMCRYPT_MLDSA_Q_INV (58728449)

//
// Inverse NTT fixup times R = (256^-1 << 32) mod Q
//
#define SYMCRYPT_MLDSA_INTT_FIXUP_TIMES_R (16382)

//
// R^2 mod Q - used for multiplying a factor of R into a polynomial in NTT form via
// Montgomery multiplication
//
#define SYMCRYPT_MLDSA_RSQR (2365951)

//
// Size of the expanded public seed used in SymCryptMlDsaRejNttPoly
// Defined in FIPS 204 to be 272 bits (256 bit public seed rho || 8 bit index s || 8 bit index r)
//
#define SYMCRYPT_MLDSA_REJNTTPOLY_SEED_SIZE (34)

//
// Size of the expanded private seed used in SymCryptMlDsaRejBoundedPoly
// Defined in FIPS 204 to be 528 bits (512 bit private vector seed rho' || 16 bit index)
//
#define SYMCRYPT_MLDSA_REJBOUNDEDPOLY_SEED_SIZE (66)

//
// Number of low-order bits dropped by Power2Round. Defined as d in FIPS 204
//
#define SYMCRYPT_POWER2ROUND_LOW_ORDER_BITS (13)

//
// Zeta tables.
// For ML-DSA, zeta = 1753, which is a 512th root of unity modulo Q
//
// In ML-DSA we use powers of zeta to convert to and from NTT form
// and to perform multiplication between polynomials in NTT form
//

// This table is a lookup for (Zeta^(BitRev(index)) * R) mod Q
// Used in NTT and Inverse NTT
// i.e. element 1 is Zeta^(BitRev(1)) * (2^32) mod Q == (1753^128)*(2^32) mod 8380417 == 25847
//
// MLDSA_ZETA_BITREV_TIMES_R = [ (pow(1753, bitRev(i), 8380417) << 32) % 8380417 for i in range(256) ]
//
const UINT32 MLDSA_ZETA_BITREV_TIMES_R[SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS] = {
    4193792, 25847, 5771523, 7861508, 237124, 7602457, 7504169, 466468,
    1826347, 2353451, 8021166, 6288512, 3119733, 5495562, 3111497, 2680103,
    2725464, 1024112, 7300517, 3585928, 7830929, 7260833, 2619752, 6271868,
    6262231, 4520680, 6980856, 5102745, 1757237, 8360995, 4010497, 280005,
    2706023, 95776, 3077325, 3530437, 6718724, 4788269, 5842901, 3915439,
    4519302, 5336701, 3574422, 5512770, 3539968, 8079950, 2348700, 7841118,
    6681150, 6736599, 3505694, 4558682, 3507263, 6239768, 6779997, 3699596,
    811944, 531354, 954230, 3881043, 3900724, 5823537, 2071892, 5582638,
    4450022, 6851714, 4702672, 5339162, 6927966, 3475950, 2176455, 6795196,
    7122806, 1939314, 4296819, 7380215, 5190273, 5223087, 4747489, 126922,
    3412210, 7396998, 2147896, 2715295, 5412772, 4686924, 7969390, 5903370,
    7709315, 7151892, 8357436, 7072248, 7998430, 1349076, 1852771, 6949987,
    5037034, 264944, 508951, 3097992, 44288, 7280319, 904516, 3958618,
    4656075, 8371839, 1653064, 5130689, 2389356, 8169440, 759969, 7063561,
    189548, 4827145, 3159746, 6529015, 5971092, 8202977, 1315589, 1341330,
    1285669, 6795489, 7567685, 6940675, 5361315, 4499357, 4751448, 3839961,
    2091667, 3407706, 2316500, 3817976, 5037939, 2244091, 5933984, 4817955,
    266997, 2434439, 7144689, 3513181, 4860065, 4621053, 7183191, 5187039,
    900702, 1859098, 909542, 819034, 495491, 6767243, 8337157, 7857917,
    7725090, 5257975, 2031748, 3207046, 4823422, 7855319, 7611795, 4784579,
    342297, 286988, 5942594, 4108315, 3437287, 5038140, 1735879, 203044,
    2842341, 2691481, 5790267, 1265009, 4055324, 1247620, 2486353, 1595974,
    4613401, 1250494, 2635921, 4832145, 5386378, 1869119, 1903435, 7329447,
    7047359, 1237275, 5062207, 6950192, 7929317, 1312455, 3306115, 6417775,
    7100756, 1917081, 5834105, 7005614, 1500165, 777191, 2235880, 3406031,
    7838005, 5548557, 6709241, 6533464, 5796124, 4656147, 594136, 4603424,
    6366809, 2432395, 2454455, 8215696, 1957272, 3369112, 185531, 7173032,
    5196991, 162844, 1616392, 3014001, 810149, 1652634, 4686184, 6581310,
    5341501, 3523897, 3866901, 269760, 2213111, 7404533, 1717735, 472078,
    7953734, 1723600, 6577327, 1910376, 6712985, 7276084, 8119771, 4546524,
    5441381, 6144432, 7959518, 6094090, 183443, 7403526, 1612842, 4834730,
    7826001, 3919660, 8332111, 7018208, 3937738, 1400424, 7534263, 1976782
};

const UINT32 MLDSA_NEGATIVE_ZETA_BITREV_TIMES_R[SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS] = {
    4186625, 8354570, 2608894, 518909, 8143293, 777960, 876248, 7913949,
    6554070, 6026966, 359251, 2091905, 5260684, 2884855, 5268920, 5700314,
    5654953, 7356305, 1079900, 4794489, 549488, 1119584, 5760665, 2108549,
    2118186, 3859737, 1399561, 3277672, 6623180, 19422, 4369920, 8100412,
    5674394, 8284641, 5303092, 4849980, 1661693, 3592148, 2537516, 4464978,
    3861115, 3043716, 4805995, 2867647, 4840449, 300467, 6031717, 539299,
    1699267, 1643818, 4874723, 3821735, 4873154, 2140649, 1600420, 4680821,
    7568473, 7849063, 7426187, 4499374, 4479693, 2556880, 6308525, 2797779,
    3930395, 1528703, 3677745, 3041255, 1452451, 4904467, 6203962, 1585221,
    1257611, 6441103, 4083598, 1000202, 3190144, 3157330, 3632928, 8253495,
    4968207, 983419, 6232521, 5665122, 2967645, 3693493, 411027, 2477047,
    671102, 1228525, 22981, 1308169, 381987, 7031341, 6527646, 1430430,
    3343383, 8115473, 7871466, 5282425, 8336129, 1100098, 7475901, 4421799,
    3724342, 8578, 6727353, 3249728, 5991061, 210977, 7620448, 1316856,
    8190869, 3553272, 5220671, 1851402, 2409325, 177440, 7064828, 7039087,
    7094748, 1584928, 812732, 1439742, 3019102, 3881060, 3628969, 4540456,
    6288750, 4972711, 6063917, 4562441, 3342478, 6136326, 2446433, 3562462,
    8113420, 5945978, 1235728, 4867236, 3520352, 3759364, 1197226, 3193378,
    7479715, 6521319, 7470875, 7561383, 7884926, 1613174, 43260, 522500,
    655327, 3122442, 6348669, 5173371, 3556995, 525098, 768622, 3595838,
    8038120, 8093429, 2437823, 4272102, 4943130, 3342277, 6644538, 8177373,
    5538076, 5688936, 2590150, 7115408, 4325093, 7132797, 5894064, 6784443,
    3767016, 7129923, 5744496, 3548272, 2994039, 6511298, 6476982, 1050970,
    1333058, 7143142, 3318210, 1430225, 451100, 7067962, 5074302, 1962642,
    1279661, 6463336, 2546312, 1374803, 6880252, 7603226, 6144537, 4974386,
    542412, 2831860, 1671176, 1846953, 2584293, 3724270, 7786281, 3776993,
    2013608, 5948022, 5925962, 164721, 6423145, 5011305, 8194886, 1207385,
    3183426, 8217573, 6764025, 5366416, 7570268, 6727783, 3694233, 1799107,
    3038916, 4856520, 4513516, 8110657, 6167306, 975884, 6662682, 7908339,
    426683, 6656817, 1803090, 6470041, 1667432, 1104333, 260646, 3833893,
    2939036, 2235985, 420899, 2286327, 8196974, 976891, 6767575, 3545687,
    554416, 4460757, 48306, 1362209, 4442679, 6979993, 846154, 6403635
};

const SYMCRYPT_MLDSA_INTERNAL_PARAMS SymCryptMlDsaInternalParams44 =
{
    .params = SYMCRYPT_MLDSA_PARAMS_MLDSA44,
    .cbPolyElement = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT,
    .cbRowVector = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR(4),
    .cbColVector = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR(4),
    .cbMatrix = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_MATRIX(4, 4),
    .nRows = 4,
    .nCols = 4,
    .privateKeyRange = 2,
    .encodedCoefficientBitLength = 3,
    .nChallengeNonZeroCoeffs = 39,
    .nHintNonZeroCoeffs = 80,
    .maskCoefficientRangeLog2 = 17,
    .commitmentModulus = 44,
    .decomposeR1Factor = 11275,
    .commitmentRoundingRange = 95232,
    .w1EncodeCoefficientBitLength = 6, // [0, 43]
    .cbCommitmentHash = 32,
    .cbEncodedPrivateKey = 2560,
    .cbEncodedPublicKey = 1312,
    .cbEncodedSignature = SYMCRYPT_MLDSA_SIGNATURE_SIZE_MLDSA44
};

const SYMCRYPT_MLDSA_INTERNAL_PARAMS SymCryptMlDsaInternalParams65 =
{
    .params = SYMCRYPT_MLDSA_PARAMS_MLDSA65,
    .cbPolyElement = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT,
    .cbRowVector = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR(6),
    .cbColVector = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR(5),
    .cbMatrix = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_MATRIX(6, 5),
    .nRows = 6,
    .nCols = 5,
    .privateKeyRange = 4,
    .encodedCoefficientBitLength = 4,
    .nChallengeNonZeroCoeffs = 49,
    .nHintNonZeroCoeffs = 55,
    .maskCoefficientRangeLog2 = 19,
    .commitmentModulus = 16,
    .decomposeR1Factor = 4100,
    .commitmentRoundingRange = 261888,
    .w1EncodeCoefficientBitLength = 4, // [0, 15]
    .cbCommitmentHash = 48,
    .cbEncodedPrivateKey = 4032,
    .cbEncodedPublicKey = 1952,
    .cbEncodedSignature = SYMCRYPT_MLDSA_SIGNATURE_SIZE_MLDSA65
};

const SYMCRYPT_MLDSA_INTERNAL_PARAMS SymCryptMlDsaInternalParams87 =
{
    .params = SYMCRYPT_MLDSA_PARAMS_MLDSA87,
    .cbPolyElement = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT,
    .cbRowVector = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR(8),
    .cbColVector = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR(7),
    .cbMatrix = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_MATRIX(8, 7),
    .nRows = 8,
    .nCols = 7,
    .privateKeyRange = 2,
    .encodedCoefficientBitLength = 3,
    .nChallengeNonZeroCoeffs = 60,
    .nHintNonZeroCoeffs = 75,
    .maskCoefficientRangeLog2 = 19,
    .commitmentModulus = 16,
    .decomposeR1Factor = 4100,
    .commitmentRoundingRange = 261888,
    .w1EncodeCoefficientBitLength = 4, // [0, 15]
    .cbCommitmentHash = 64,
    .cbEncodedPrivateKey = 4896,
    .cbEncodedPublicKey = 2592,
    .cbEncodedSignature = SYMCRYPT_MLDSA_SIGNATURE_SIZE_MLDSA87
};

typedef struct _SYMCRYPT_HASH_OID_MAPPING
{
    SYMCRYPT_PQDSA_HASH_ID  hashId;
    const PCSYMCRYPT_HASH   pHashAlgorithm;
    PCSYMCRYPT_OID          pOid;
    BOOLEAN                 fIsXof;
} SYMCRYPT_HASH_OID_MAPPING, *PSYMCRYPT_HASH_OID_MAPPING;

//
// Mapping of hash OIDs to SymCrypt hash algorithms. Currently this only contains the "short" hash
// OIDs, and only for those algorithms that are approved for use in ML-DSA. In the future, we might
// want to make this functionality more generic, but that requires more thought about the design.
// Ideally, the SYMCRYPT_HASH structures could contain pointers to their corresponding OIDs, but
// those structures are exposed externally, so extending them would be a breaking change.
//
const SYMCRYPT_HASH_OID_MAPPING g_hashOidMap[] =
{
    { SYMCRYPT_PQDSA_HASH_ID_SHA256,        &SymCryptSha256Algorithm_default,       &SymCryptSha256OidList[1],      FALSE },
    { SYMCRYPT_PQDSA_HASH_ID_SHA384,        &SymCryptSha384Algorithm_default,       &SymCryptSha384OidList[1],      FALSE },
    { SYMCRYPT_PQDSA_HASH_ID_SHA512,        &SymCryptSha512Algorithm_default,       &SymCryptSha512OidList[1],      FALSE },
    { SYMCRYPT_PQDSA_HASH_ID_SHA512_256,    &SymCryptSha512_256Algorithm_default,   &SymCryptSha512_256OidList[1],  FALSE },
    { SYMCRYPT_PQDSA_HASH_ID_SHA3_256,      &SymCryptSha3_256Algorithm_default,     &SymCryptSha3_256OidList[1],    FALSE },
    { SYMCRYPT_PQDSA_HASH_ID_SHA3_384,      &SymCryptSha3_384Algorithm_default,     &SymCryptSha3_384OidList[1],    FALSE },
    { SYMCRYPT_PQDSA_HASH_ID_SHA3_512,      &SymCryptSha3_512Algorithm_default,     &SymCryptSha3_512OidList[1],    FALSE },
    { SYMCRYPT_PQDSA_HASH_ID_SHAKE128,      &SymCryptShake128HashAlgorithm_default, &SymCryptShake128OidList[1],    TRUE  },
    { SYMCRYPT_PQDSA_HASH_ID_SHAKE256,      &SymCryptShake256HashAlgorithm_default, &SymCryptShake256OidList[1],    TRUE  }
};

//
// The table above relies on the OID lists having (at least) two entries, where the second one
// is the 11-byte encoding of the OID. If this ever changes, the table needs to be updated.
//
C_ASSERT( SYMCRYPT_SHA256_OID_COUNT == 2 );
C_ASSERT( SYMCRYPT_SHA384_OID_COUNT == 2 );
C_ASSERT( SYMCRYPT_SHA512_OID_COUNT == 2 );
C_ASSERT( SYMCRYPT_SHA512_256_OID_COUNT == 2 );
C_ASSERT( SYMCRYPT_SHA3_256_OID_COUNT == 2 );
C_ASSERT( SYMCRYPT_SHA3_384_OID_COUNT == 2 );
C_ASSERT( SYMCRYPT_SHA3_512_OID_COUNT == 2 );
C_ASSERT( SYMCRYPT_SHAKE128_OID_COUNT == 2 );
C_ASSERT( SYMCRYPT_SHAKE256_OID_COUNT == 2 );

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaGetInternalParamsFromParams(
    SYMCRYPT_MLDSA_PARAMS               params,
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS*   pInternalParams )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    switch( params )
    {
        case SYMCRYPT_MLDSA_PARAMS_MLDSA44:
            *pInternalParams = &SymCryptMlDsaInternalParams44;
            break;
        case SYMCRYPT_MLDSA_PARAMS_MLDSA65:
            *pInternalParams = &SymCryptMlDsaInternalParams65;
            break;
        case SYMCRYPT_MLDSA_PARAMS_MLDSA87:
            *pInternalParams = &SymCryptMlDsaInternalParams87;
            break;
        case SYMCRYPT_MLDSA_PARAMS_NULL:
            scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
            break;
        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            break;
    }

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSizeofKeyFormatFromParams(
    SYMCRYPT_MLDSA_PARAMS       params,
    SYMCRYPT_MLDSAKEY_FORMAT    mlDsakeyFormat,
    SIZE_T*                     pcbKeyFormat )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS pInternalParams = NULL;

    scError = SymCryptMlDsaGetInternalParamsFromParams( params, &pInternalParams );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    switch( mlDsakeyFormat )
    {
        case SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_SEED:
            *pcbKeyFormat = SYMCRYPT_MLDSA_ROOT_SEED_SIZE;
            break;
        case SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_KEY:
            *pcbKeyFormat = pInternalParams->cbEncodedPrivateKey;
            break;
        case SYMCRYPT_MLDSAKEY_FORMAT_PUBLIC_KEY:
            *pcbKeyFormat = pInternalParams->cbEncodedPublicKey;
            break;
        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            break;
    }

cleanup:
    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSizeofSignatureFromParams(
    SYMCRYPT_MLDSA_PARAMS       params,
    SIZE_T*                     pcbSignature )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS pInternalParams = NULL;

    scError = SymCryptMlDsaGetInternalParamsFromParams( params, &pInternalParams );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    *pcbSignature = pInternalParams->cbEncodedSignature;

cleanup:
    return scError;
}

_Use_decl_annotations_
PSYMCRYPT_MLDSAKEY
SYMCRYPT_CALL
SymCryptMlDsakeyInitialize(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS pInternalParams,
    PBYTE                            pbKey,
    UINT32                           cbKey )
{
    PSYMCRYPT_MLDSAKEY pkMlDsakey = (PSYMCRYPT_MLDSAKEY) pbKey;
    SYMCRYPT_ASSERT( pkMlDsakey != NULL );

    UINT8 nRows = pInternalParams->nRows;
    UINT8 nCols = pInternalParams->nCols;

    SYMCRYPT_ASSERT( cbKey == SYMCRYPT_INTERNAL_MLDSA_SIZEOF_KEY(nRows, nCols) );

    UINT32 cbMatrix = pInternalParams->cbMatrix; // A matrix
    UINT32 cbRowVector = pInternalParams->cbRowVector; // s2, t vectors
    UINT32 cbColVector = pInternalParams->cbColVector; // s1 vector

    SymCryptWipe( pbKey, cbKey );

    pkMlDsakey->pParams = pInternalParams;
    pkMlDsakey->cbTotalSize = cbKey;

    PBYTE pbCurrent = pbKey + sizeof(SYMCRYPT_MLDSAKEY);

    // Public components
    pkMlDsakey->pmA = SymCryptMlDsaMatrixCreate( pbCurrent, cbMatrix, nRows, nCols );
    pbCurrent += cbMatrix;

    pkMlDsakey->pvt1 = SymCryptMlDsaVectorCreate( pbCurrent, cbRowVector, nRows );
    pbCurrent += cbRowVector;

    // Private components
    pkMlDsakey->pvs1 = SymCryptMlDsaVectorCreate( pbCurrent, cbColVector, nCols );
    pbCurrent += cbColVector;

    pkMlDsakey->pvs2 = SymCryptMlDsaVectorCreate( pbCurrent, cbRowVector, nRows );
    pbCurrent += cbRowVector;

    pkMlDsakey->pvt0 = SymCryptMlDsaVectorCreate( pbCurrent, cbRowVector, nRows );
    pbCurrent += cbRowVector;

    SYMCRYPT_ASSERT( pbCurrent == pbKey + cbKey );

    SYMCRYPT_SET_MAGIC( pkMlDsakey );

    return pkMlDsakey;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsakeyComputeT(
    PCSYMCRYPT_MLDSA_MATRIX     pmA,
    PCSYMCRYPT_MLDSA_VECTOR     pvs1,
    PCSYMCRYPT_MLDSA_VECTOR     pvs2,
    PSYMCRYPT_MLDSA_VECTOR      pvt0,
    PSYMCRYPT_MLDSA_VECTOR      pvt1,
    PSYMCRYPT_MLDSA_VECTOR      pvTmp,
    PSYMCRYPT_MLDSA_POLYELEMENT peTmp )
{
    // T = InvNTT(NTT(A)*NTT(s1) + NTT(s2))
    // pvTmp := NTT(A)*NTT(s1)
    SymCryptMlDsaMatrixVectorMontMul(
        pmA,
        pvs1,
        pvTmp,
        peTmp );

    // TODO: should probably do multiplication by directly in the matrix multiplication function
    for(UINT8 i = 0; i < pvTmp->nElems; ++i)
    {
        SymCryptMlDsaPolyElementMulR(SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT(i, pvTmp));
    }

    // pvTmp := pvTmp + NTT(s2)
    SymCryptMlDsaVectorAdd( pvTmp, pvs2, pvTmp );

    // T = pvTmp := InvNTT(NTT(A)*NTT(s1) + NTT(s2))
    SymCryptMlDsaVectorINTT( pvTmp );

    SymCryptMlDsaVectorPower2Round( pvTmp, pvt1, pvt0 );
}

UINT32
SYMCRYPT_CALL
SymCryptMlDsaMontReduce( UINT64 a )
{
    UINT32 t = ((UINT32) a) * SYMCRYPT_MLDSA_Q_INV;
    UINT32 m = (((UINT64) t) * SYMCRYPT_MLDSA_Q) >> SYMCRYPT_MLDSA_R_LOG2;

    UINT64 res = (a >> SYMCRYPT_MLDSA_R_LOG2) - m;
    UINT32 additionMask = SYMCRYPT_MASK32_LT( res, 0 );

    res = res + (SYMCRYPT_MLDSA_Q & additionMask);
    SYMCRYPT_ASSERT( res < SYMCRYPT_MLDSA_Q );

    return (UINT32) res;
}

UINT32
SYMCRYPT_CALL
SymCryptMlDsaMontMul( UINT32 a, UINT32 b )
{
    SYMCRYPT_ASSERT( a < SYMCRYPT_MLDSA_Q );
    SYMCRYPT_ASSERT( b < SYMCRYPT_MLDSA_Q );

    return SymCryptMlDsaMontReduce((UINT64) a * b);
}

UINT32
SYMCRYPT_CALL
SymCryptMlDsaModAdd( UINT32 a, UINT32 b )
{
    SYMCRYPT_ASSERT( a < SYMCRYPT_MLDSA_Q );
    SYMCRYPT_ASSERT( b < SYMCRYPT_MLDSA_Q );

    UINT32 res = a + b;
    UINT32 subtractionMask = SYMCRYPT_MASK32_LT( SYMCRYPT_MLDSA_Q - 1, res );

    // If res >= Q, subtract Q
    res = res - (SYMCRYPT_MLDSA_Q & subtractionMask);

    return res;
}

UINT32
SYMCRYPT_CALL
SymCryptMlDsaModSub( UINT32 a, UINT32 b )
{
    SYMCRYPT_ASSERT( a < SYMCRYPT_MLDSA_Q );
    SYMCRYPT_ASSERT( b < SYMCRYPT_MLDSA_Q );

    UINT32 additionMask = SYMCRYPT_MASK32_LT( a, b );

    // If a < b, result is negative, so we add Q
    return (INT32) a - (INT32) b + (SYMCRYPT_MLDSA_Q & additionMask);
}

_Use_decl_annotations_
PSYMCRYPT_MLDSA_POLYELEMENT
SYMCRYPT_CALL
SymCryptMlDsaPolyElementCreate(
    PBYTE   pbBuffer,
    SIZE_T  cbBuffer )
{
    UNREFERENCED_PARAMETER( cbBuffer );
    SYMCRYPT_ASSERT( cbBuffer == SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT );

    PSYMCRYPT_MLDSA_POLYELEMENT peElement = (PSYMCRYPT_MLDSA_POLYELEMENT) pbBuffer;
    SYMCRYPT_ASSERT( peElement != NULL );

    return peElement;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementSetZero(
    PSYMCRYPT_MLDSA_POLYELEMENT peDst )
{
    for( UINT32 i = 0; i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; ++i )
    {
        peDst->coeffs[i] = 0;
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementNTT(
    PSYMCRYPT_MLDSA_POLYELEMENT peSrc )
{
    C_ASSERT( (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS & 1) == 0);

    UINT32 k = 0;

    for(UINT32 len = SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 2; len >= 1; len /= 2)
    {
        for(UINT32 start = 0; start < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; start += 2 * len)
        {
            k++;
            UINT32 twiddleFactor = MLDSA_ZETA_BITREV_TIMES_R[k];

            for(UINT32 j = start; j < start + len; j++)
            {
                //
                // Typically for Montgomery multiplication, both operands have a factor of R.
                // After multiplying, the product has a factor for R^2, and a reduction is then
                // performed which divides out a factor of R, resulting in the product again having
                // a factor of R^1, mod Q. In this case, the twiddleFactor is pre-multiplied by R,
                // but the coefficients are not expected to have a factor of R; thus, after
                // reduction, the result does not have a factor of R either.
                //
                UINT32 t = SymCryptMlDsaMontMul(twiddleFactor, peSrc->coeffs[j + len]);
                peSrc->coeffs[j + len] = SymCryptMlDsaModSub(peSrc->coeffs[j], t);
                peSrc->coeffs[j] = SymCryptMlDsaModAdd(peSrc->coeffs[j], t);
            }
        }
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementINTT(
    PSYMCRYPT_MLDSA_POLYELEMENT peSrc )
{
    C_ASSERT( (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS & 1) == 0);

    UINT32 k = SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS;

    for(UINT32 len = 1; len < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; len *= 2)
    {
        for(UINT32 start = 0; start < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; start += 2 * len)
        {
            k--;
            UINT32 twiddleFactor = MLDSA_NEGATIVE_ZETA_BITREV_TIMES_R[k];

            for(UINT32 j = start; j < start + len; j++)
            {
                //
                // As above, our twiddleFactor is pre-multiplied by R, but the coefficients are not,
                // so after the reduction, the result does not have a factor of R.
                //
                UINT32 t = peSrc->coeffs[j];
                peSrc->coeffs[j] = SymCryptMlDsaModAdd(t, peSrc->coeffs[j + len]);
                peSrc->coeffs[j + len] = SymCryptMlDsaModSub(t, peSrc->coeffs[j + len]);
                peSrc->coeffs[j + len] = SymCryptMlDsaMontMul(twiddleFactor, peSrc->coeffs[j + len]);
            }
        }
    }

    for(UINT32 i = 0; i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++)
    {
        peSrc->coeffs[i] = SymCryptMlDsaMontMul(SYMCRYPT_MLDSA_INTT_FIXUP_TIMES_R, peSrc->coeffs[i]);
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementMulR(
    PSYMCRYPT_MLDSA_POLYELEMENT peSrc )
{
    for(UINT32 i = 0; i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++)
    {
        peSrc->coeffs[i] = SymCryptMlDsaMontMul(SYMCRYPT_MLDSA_RSQR, peSrc->coeffs[i]);
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementMontMul(
    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc1,
    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc2,
    PSYMCRYPT_MLDSA_POLYELEMENT     peDst )
{
    for(UINT32 i = 0; i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++)
    {
        peDst->coeffs[i] = SymCryptMlDsaMontMul(peSrc1->coeffs[i], peSrc2->coeffs[i]);
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementAdd(
    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc1,
    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc2,
    PSYMCRYPT_MLDSA_POLYELEMENT     peDst )
{
    for(UINT32 i = 0; i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++)
    {
        peDst->coeffs[i] = SymCryptMlDsaModAdd(peSrc1->coeffs[i], peSrc2->coeffs[i]);
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementSub(
    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc1,
    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc2,
    PSYMCRYPT_MLDSA_POLYELEMENT     peDst )
{
    for(UINT32 i = 0; i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++)
    {
        peDst->coeffs[i] = SymCryptMlDsaModSub(peSrc1->coeffs[i], peSrc2->coeffs[i]);
    }
}

_Use_decl_annotations_
PSYMCRYPT_MLDSA_VECTOR
SYMCRYPT_CALL
SymCryptMlDsaVectorCreate(
    PBYTE   pbBuffer,
    UINT32  cbBuffer,
    UINT8   nElems )
{
    SYMCRYPT_ASSERT( nElems > 0);
    SYMCRYPT_ASSERT( nElems <= SYMCRYPT_MLDSA_VECTOR_MAX_LENGTH );
    SYMCRYPT_ASSERT( cbBuffer == SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR(nElems) );

    PSYMCRYPT_MLDSA_VECTOR pvVector = (PSYMCRYPT_MLDSA_VECTOR) pbBuffer;
    SYMCRYPT_ASSERT( pvVector != NULL );

    pvVector->nElems = nElems;
    pvVector->cbTotalSize = cbBuffer;

    PBYTE pbCurrent = pbBuffer + sizeof(SYMCRYPT_MLDSA_VECTOR);
    for( UINT32 i = 0; i < nElems; ++i )
    {
        SymCryptMlDsaPolyElementCreate( pbCurrent, SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT );
        pbCurrent += SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT;
    }

    return pvVector;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorCopy(
    PCSYMCRYPT_MLDSA_VECTOR pvSrc,
    PSYMCRYPT_MLDSA_VECTOR  pvDst )
{
    SYMCRYPT_ASSERT( pvSrc->nElems == pvDst->nElems );

    memcpy( pvDst, pvSrc, pvSrc->cbTotalSize );
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorSetZero(
    PSYMCRYPT_MLDSA_VECTOR pvDst )
{
    for( UINT32 i = 0; i < pvDst->nElems; ++i )
    {
        PSYMCRYPT_MLDSA_POLYELEMENT peDst = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvDst );
        SymCryptMlDsaPolyElementSetZero( peDst );
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorAdd(
    PCSYMCRYPT_MLDSA_VECTOR pvSrc1,
    PCSYMCRYPT_MLDSA_VECTOR pvSrc2,
    PSYMCRYPT_MLDSA_VECTOR  pvDst )
{
    SYMCRYPT_ASSERT( pvSrc1->nElems == pvSrc2->nElems );
    SYMCRYPT_ASSERT( pvSrc1->nElems == pvDst->nElems );

    PCSYMCRYPT_MLDSA_POLYELEMENT peSrc1, peSrc2;
    PSYMCRYPT_MLDSA_POLYELEMENT peDst;

    for( UINT32 i = 0; i < pvSrc1->nElems; ++i )
    {
        peSrc1 = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvSrc1 );
        peSrc2 = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvSrc2 );
        peDst = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvDst );

        SymCryptMlDsaPolyElementAdd( peSrc1, peSrc2, peDst );
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorSub(
    PCSYMCRYPT_MLDSA_VECTOR pvSrc1,
    PCSYMCRYPT_MLDSA_VECTOR pvSrc2,
    PSYMCRYPT_MLDSA_VECTOR  pvDst )
{
    SYMCRYPT_ASSERT( pvSrc1->nElems == pvSrc2->nElems );
    SYMCRYPT_ASSERT( pvSrc1->nElems == pvDst->nElems );

    PCSYMCRYPT_MLDSA_POLYELEMENT peSrc1, peSrc2;
    PSYMCRYPT_MLDSA_POLYELEMENT peDst;

    for( UINT32 i = 0; i < pvSrc1->nElems; ++i )
    {
        peSrc1 = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvSrc1 );
        peSrc2 = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvSrc2 );
        peDst = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvDst );

        SymCryptMlDsaPolyElementSub( peSrc1, peSrc2, peDst );
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorPolyElementMontMul(
    PCSYMCRYPT_MLDSA_VECTOR         pvSrc1,
    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc2,
    PSYMCRYPT_MLDSA_VECTOR          pvDst )
{
    SYMCRYPT_ASSERT( pvSrc1->nElems == pvDst->nElems );

    for( UINT32 i = 0; i < pvSrc1->nElems; ++i )
    {
        PSYMCRYPT_MLDSA_POLYELEMENT peSrc1 = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvSrc1 );
        PSYMCRYPT_MLDSA_POLYELEMENT peDst = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvDst );

        SymCryptMlDsaPolyElementMontMul( peSrc1, peSrc2, peDst );
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorNTT(
    PSYMCRYPT_MLDSA_VECTOR pvSrc )
{
    for( UINT32 i = 0; i < pvSrc->nElems; ++i )
    {
        SymCryptMlDsaPolyElementNTT( SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvSrc ) );
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorINTT(
    _Inout_ PSYMCRYPT_MLDSA_VECTOR pvSrc )
{
    for( UINT32 i = 0; i < pvSrc->nElems; ++i )
    {
        SymCryptMlDsaPolyElementINTT( SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvSrc ) );
    }
}

_Use_decl_annotations_
PSYMCRYPT_MLDSA_MATRIX
SYMCRYPT_CALL
SymCryptMlDsaMatrixCreate(
    PBYTE   pbBuffer,
    UINT32  cbBuffer,
    UINT8   nRows,
    UINT8   nCols )
{
    SYMCRYPT_ASSERT( nRows > 0);
    SYMCRYPT_ASSERT( nCols > 0);
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLDSA_MATRIX_MAX_NROWS );
    SYMCRYPT_ASSERT( nCols <= SYMCRYPT_MLDSA_MATRIX_MAX_NCOLS );
    SYMCRYPT_ASSERT( cbBuffer == SYMCRYPT_INTERNAL_MLDSA_SIZEOF_MATRIX(nRows, nCols) );

    PSYMCRYPT_MLDSA_MATRIX pMatrix = (PSYMCRYPT_MLDSA_MATRIX) pbBuffer;
    SYMCRYPT_ASSERT( pMatrix != NULL );

    pMatrix->nRows = nRows;
    pMatrix->nCols = nCols;
    pMatrix->cbTotalSize = cbBuffer;

    PBYTE pbCurrent = pbBuffer + sizeof(SYMCRYPT_MLDSA_MATRIX);
    for(UINT32 i = 0; i < (UINT32) nRows * nCols; ++i)
    {
        SymCryptMlDsaPolyElementCreate( pbCurrent, SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT );
        pbCurrent += SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT;
    }

    return pMatrix;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaMatrixVectorMontMul(
    PCSYMCRYPT_MLDSA_MATRIX     pmSrc1,
    PCSYMCRYPT_MLDSA_VECTOR     pvSrc2,
    PSYMCRYPT_MLDSA_VECTOR      pvDst,
    PSYMCRYPT_MLDSA_POLYELEMENT peTmp)
{
    SYMCRYPT_ASSERT( pmSrc1->nCols == pvSrc2->nElems );
    SYMCRYPT_ASSERT( pmSrc1->nRows == pvDst->nElems );

    PCSYMCRYPT_MLDSA_POLYELEMENT peSrc1, peSrc2;
    PSYMCRYPT_MLDSA_POLYELEMENT peDst;

    SymCryptMlDsaVectorSetZero( pvDst );

    _Analysis_assume_( pmSrc1->nRows > 0 );
    _Analysis_assume_( pmSrc1->nCols > 0 );

    for( UINT32 i = 0; i < pmSrc1->nRows; ++i )
    {
        // peDst = pvDst[i]
        peDst = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvDst );

        for( UINT32 j = 0; j < pmSrc1->nCols; ++j )
        {
            peSrc1 = SYMCRYPT_INTERNAL_MLDSA_MATRIX_ELEMENT( i, j, pmSrc1 );
            peSrc2 = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( j, pvSrc2 );

            SymCryptMlDsaPolyElementMontMul( peSrc1, peSrc2, peTmp );
            SymCryptMlDsaPolyElementAdd( peDst, peTmp, peDst );
        }
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaRejNttPoly(
    PCBYTE                      pbRejNttPolySeed,
    SIZE_T                      cbRejNttPolySeed,
    PSYMCRYPT_MLDSA_POLYELEMENT peDst )
{
    SYMCRYPT_ASSERT( cbRejNttPolySeed == SYMCRYPT_MLDSA_REJNTTPOLY_SEED_SIZE );

    SYMCRYPT_SHAKE128_STATE shakeState;
    SymCryptShake128Init( &shakeState );
    SymCryptShake128Append( &shakeState, pbRejNttPolySeed, cbRejNttPolySeed );

    UINT32 coeff = 0;
    BYTE shakeBytes[4]; // We only use 3 bytes, but using 4 allows converting to UINT32 more easily

    SymCryptWipeKnownSize( shakeBytes, sizeof(shakeBytes) );

    for(UINT32 i = 0; i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++)
    {
        // CoeffFromThreeBytes from FIPS 204
        do
        {
            SymCryptShake128Extract( &shakeState, shakeBytes, 3, FALSE );
            shakeBytes[2] &= 0x7F; // if b2 > 127, b2 -= 128
            coeff = SYMCRYPT_LOAD_LSBFIRST32( shakeBytes );
        } while (coeff >= SYMCRYPT_MLDSA_Q);

        peDst->coeffs[i] = coeff;
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaExpandA(
    PCBYTE                  pbPublicSeed,
    SIZE_T                  cbPublicSeed,
    PSYMCRYPT_MLDSA_MATRIX  pmA )
{
    SYMCRYPT_ASSERT( cbPublicSeed == SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE );
    C_ASSERT( SYMCRYPT_MLDSA_REJNTTPOLY_SEED_SIZE == SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE + 2 );

    // The expanded seed is the public seed concatenated with one byte each for the column and row
    // indices of the matrix element being expanded.
    BYTE rejNttSeed[SYMCRYPT_MLDSA_REJNTTPOLY_SEED_SIZE];
    memcpy( rejNttSeed, pbPublicSeed, cbPublicSeed );

    for( UINT8 i = 0; i < pmA->nRows; ++i )
    {
        for( UINT8 j = 0; j < pmA->nCols; ++j )
        {
            rejNttSeed[SYMCRYPT_MLDSA_REJNTTPOLY_SEED_SIZE - 2] = j;
            rejNttSeed[SYMCRYPT_MLDSA_REJNTTPOLY_SEED_SIZE - 1] = i;

            #pragma prefast( suppress: 6385, "False warning - reading invalid data from rejNttSeed" );
            SymCryptMlDsaRejNttPoly( rejNttSeed, sizeof(rejNttSeed), SYMCRYPT_INTERNAL_MLDSA_MATRIX_ELEMENT( i, j, pmA ) );

        }
    }
}

_Use_decl_annotations_
FORCEINLINE
INT8
SYMCRYPT_CALL
SymCryptMlDsaCoeffFromHalfByte(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    UINT8                               halfByte )
{
    SYMCRYPT_ASSERT( halfByte <= 15 );
    SYMCRYPT_ASSERT( pParams->privateKeyRange == 2 || pParams->privateKeyRange == 4 );

    if( pParams->privateKeyRange == 2 && halfByte < 15)
    {
        UINT8 halfByteDiv5 = (UINT8) ( ( halfByte * 13 ) >> 6 );
        UINT8 halfByteMod5 = halfByte - (5 * halfByteDiv5);
        return 2 - halfByteMod5;
    }
    else if( pParams->privateKeyRange == 4 && halfByte < 9 )
    {
        return 4 - halfByte;
    }

    return INT8_MIN;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaRejBoundedPoly(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    PCBYTE                              pbRejBoundedPolySeed,
    SIZE_T                              cbRejBoundedPolySeed,
    PSYMCRYPT_MLDSA_POLYELEMENT         peDst )
{
    SYMCRYPT_ASSERT( cbRejBoundedPolySeed == SYMCRYPT_MLDSA_REJBOUNDEDPOLY_SEED_SIZE );

    SYMCRYPT_SHAKE256_STATE shakeState;
    SymCryptShake256Init( &shakeState );
    SymCryptShake256Append( &shakeState, pbRejBoundedPolySeed, cbRejBoundedPolySeed );

    BYTE shakeByte;
    UINT32 i = 0;
    INT8 z0, z1;

    do
    {
        // Note on sidechannel safety: the rejection sampling here can leak which bytes of the SHAKE
        // output are used and which are rejected. However, bytes themselves are not leaked. This
        // may allow the attacker to more quickly eliminate incorrect seed values when doing an
        // exhaustive search, but given the size of the seed this should still not make the
        // exhaustive search computationally feasible.
        SymCryptShake256Extract( &shakeState, &shakeByte, sizeof(shakeByte), FALSE );
        z0 = SymCryptMlDsaCoeffFromHalfByte( pParams, shakeByte & 0x0F );
        z1 = SymCryptMlDsaCoeffFromHalfByte( pParams, shakeByte >> 4 );

        SYMCRYPT_ASSERT( z0 == INT8_MIN || (( z0 + pParams->privateKeyRange >= 0 ) && ( z0 + pParams->privateKeyRange <= 2 * pParams->privateKeyRange )) );
        SYMCRYPT_ASSERT( z1 == INT8_MIN || (( z1 + pParams->privateKeyRange >= 0 ) && ( z1 + pParams->privateKeyRange <= 2 * pParams->privateKeyRange )) );

        if(z0 != INT8_MIN)
        {
            peDst->coeffs[i] = SymCryptMlDsaSignedCoefficientModQ( z0 );
            i++;
        }

        if(z1 != INT8_MIN && i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS)
        {
            peDst->coeffs[i] = SymCryptMlDsaSignedCoefficientModQ( z1 );
            i++;
        }

    } while( i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS );

    SymCryptWipeKnownSize( &shakeState, sizeof(shakeState) );
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaExpandS(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    PCBYTE                              pbPrivateVectorSeed,
    SIZE_T                              cbPrivateVectorSeed,
    PSYMCRYPT_MLDSA_VECTOR              pvs1,
    PSYMCRYPT_MLDSA_VECTOR              pvs2 )
{
    SYMCRYPT_ASSERT( cbPrivateVectorSeed == SYMCRYPT_MLDSA_PRIVATE_VECTOR_SEED_SIZE );
    C_ASSERT( SYMCRYPT_MLDSA_REJBOUNDEDPOLY_SEED_SIZE == SYMCRYPT_MLDSA_PRIVATE_VECTOR_SEED_SIZE + 2 );

    UINT32 nRows = pParams->nRows;
    UINT32 nCols = pParams->nCols;

    // The expanded seed is the private vector seed concatenated with the (two-byte) row/column
    // index of the vector element being expanded.
    BYTE rejBoundedPolySeed[SYMCRYPT_MLDSA_REJBOUNDEDPOLY_SEED_SIZE];
    memcpy( rejBoundedPolySeed, pbPrivateVectorSeed, cbPrivateVectorSeed );

    for(UINT16 i = 0; i < nCols; ++i)
    {
        SYMCRYPT_STORE_LSBFIRST16( rejBoundedPolySeed + SYMCRYPT_MLDSA_REJBOUNDEDPOLY_SEED_SIZE - sizeof(UINT16), i );
        SymCryptMlDsaRejBoundedPoly( pParams, rejBoundedPolySeed, sizeof(rejBoundedPolySeed),
            SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvs1 ) );
    }

    for(UINT16 i = 0; i < nRows; ++i)
    {
        SYMCRYPT_STORE_LSBFIRST16( rejBoundedPolySeed + SYMCRYPT_MLDSA_REJBOUNDEDPOLY_SEED_SIZE - sizeof(UINT16), (UINT16) nCols + i );
        #pragma prefast( suppress: 6385, "False warning - reading invalid data from rejBoundedPolySeed" ); // Doesn't trigger in previous loop for some reason
        SymCryptMlDsaRejBoundedPoly( pParams, rejBoundedPolySeed, sizeof(rejBoundedPolySeed),
            SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvs2 ) );
    }

    SymCryptWipeKnownSize( rejBoundedPolySeed, sizeof(rejBoundedPolySeed) );
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaSampleInBall(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    PCBYTE                              pbCommitmentHash,
    SIZE_T                              cbCommitmentHash,
    PSYMCRYPT_MLDSA_POLYELEMENT         peChallenge )
{
    SymCryptMlDsaPolyElementSetZero( peChallenge );

    SYMCRYPT_SHAKE256_STATE shakeState;
    SymCryptShake256Init( &shakeState );
    SymCryptShake256Append( &shakeState, pbCommitmentHash, cbCommitmentHash );

    // The first 8 bytes are used as as powers of negative one when sampling the challenge
    // polynomial: c[j] = -1^(H(rho)[i + tau - 256])
    BYTE temp[8];
    SYMCRYPT_ASSERT( pParams->nChallengeNonZeroCoeffs <= 8 * sizeof(temp) );
    SymCryptShake256Extract( &shakeState, temp, sizeof(temp), FALSE );

    UINT64 powersOfNegativeOne = SYMCRYPT_LOAD_LSBFIRST64( temp );

    for(UINT32 i = SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS - pParams->nChallengeNonZeroCoeffs;
        i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS;
        ++i)
    {
        BYTE j = 0;
        do
        {
            SymCryptShake256Extract( &shakeState, &j, sizeof(j), FALSE );
        } while( j > i );

        peChallenge->coeffs[i] = peChallenge->coeffs[j];

        UINT32 negativeMask = SYMCRYPT_MASK32_NONZERO( powersOfNegativeOne & 1 );
        powersOfNegativeOne >>= 1;

        // Set the coefficient modulo Q
        peChallenge->coeffs[j] = ((SYMCRYPT_MLDSA_Q - 1) & negativeMask) | (1 & ~negativeMask);
    }

    SymCryptWipeKnownSize( &shakeState, sizeof(shakeState) );
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaExpandMask(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    PSYMCRYPT_SHAKE256_STATE            pShakeState,
    PCBYTE                              pbPrivateRandom,
    SIZE_T                              cbPrivateRandom,
    UINT16                              counter,
    PSYMCRYPT_MLDSA_VECTOR              pvMask )
{
    SYMCRYPT_ASSERT( pParams->nCols == pvMask->nElems );
    SYMCRYPT_ASSERT( cbPrivateRandom == SYMCRYPT_SHAKE256_RESULT_SIZE );

    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    BYTE seedSuffix[2];

    UINT32 cbShakeOutput = (pParams->maskCoefficientRangeLog2 + 1) * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8);
    BYTE shakeOutput[20 * 32]; // Maximum size of the SHAKE output
    SYMCRYPT_ASSERT(  cbShakeOutput <= sizeof(shakeOutput) );

    for(UINT16 i = 0; i < pvMask->nElems; ++i)
    {
        SYMCRYPT_STORE_LSBFIRST16( seedSuffix, counter + i );
        SymCryptShake256Append( pShakeState, pbPrivateRandom, cbPrivateRandom );
        SymCryptShake256Append( pShakeState, (PBYTE) &seedSuffix, sizeof(seedSuffix) );
        SymCryptShake256Extract( pShakeState, shakeOutput, cbShakeOutput, TRUE );

        scError = SymCryptMlDsaPolyElementDecode(
            shakeOutput,
            pParams->maskCoefficientRangeLog2 + 1,
            1 << pParams->maskCoefficientRangeLog2,
            SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvMask ) );
        SYMCRYPT_ASSERT( scError == SYMCRYPT_NO_ERROR );
    }

    SymCryptMlDsaVectorNTT( pvMask );
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaMakeHint(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    PSYMCRYPT_MLDSA_VECTOR              pvWMinusCs2,
    PSYMCRYPT_MLDSA_VECTOR              pvWMinusCs2PlusCt0,
    PSYMCRYPT_MLDSA_VECTOR              pvDst,
    UINT32*                             nBitsSet )
{
    SYMCRYPT_ASSERT( pvWMinusCs2->nElems == pvWMinusCs2PlusCt0->nElems );
    SYMCRYPT_ASSERT( pvWMinusCs2->nElems == pvDst->nElems );

    *nBitsSet = 0;

    SymCryptMlDsaVectorHighBits( pParams, pvWMinusCs2, pvWMinusCs2 );
    SymCryptMlDsaVectorHighBits( pParams, pvWMinusCs2PlusCt0, pvWMinusCs2PlusCt0 );

    for( UINT32 i = 0; i < pvDst->nElems; ++i )
    {
        PSYMCRYPT_MLDSA_POLYELEMENT peDst = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvDst );
        PSYMCRYPT_MLDSA_POLYELEMENT peVec0 = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvWMinusCs2 );
        PSYMCRYPT_MLDSA_POLYELEMENT peVec1 = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvWMinusCs2PlusCt0 );

        for( UINT32 j = 0; j < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; ++j )
        {
            peDst->coeffs[j] = 1 & ~SYMCRYPT_MASK32_EQ(peVec0->coeffs[j], peVec1->coeffs[j]);
            *nBitsSet += peDst->coeffs[j];
        }
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaUseHint(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    PCSYMCRYPT_MLDSA_VECTOR             pvHint,
    PSYMCRYPT_MLDSA_VECTOR              pvCommitment )
{
    SYMCRYPT_ASSERT( pvHint->nElems == pvCommitment->nElems );

    UINT32 r1, r0;
    UINT32 hintIsZeroMask;
    UINT32 tmpMask;
    UINT32 r0PrimeGtZeroMask;
    UINT32 positiveOffsetCoeff;
    UINT32 negativeOffsetCoeff;

    for(UINT32 i = 0; i < pvHint->nElems; ++i)
    {
        PSYMCRYPT_MLDSA_POLYELEMENT peHint = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvHint );
        PSYMCRYPT_MLDSA_POLYELEMENT peCommitment = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvCommitment );

        for(UINT32 j = 0; j < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; ++j)
        {
            SYMCRYPT_ASSERT( peHint->coeffs[j] == 0 || peHint->coeffs[j] == 1 );

            SymCryptMlDsaDecompose( pParams, peCommitment->coeffs[j], &r1, &r0 );

            //
            // FIPS-204 UseHint lines 3-5
            // r1 in range [0, commitmentModulus)
            // r0 in range [-commitmentRoundingRange, commitmentRoundingRange], encoded
            // as an unsigned integer modulo Q
            //
            // Let r0' := r0 mod+- 2*gamma_2
            // (This is just r0 in FIPS 204, which uses signed integers.)
            //
            // r0 - 1 < commitmentRoundingRange => r0' > 0
            // r0 - 1 >= commitmentRoundingRange => r0' <= 0
            //
            // There are three cases to consider:
            // 1. If the hint is zero, the coefficient is set to r1.
            // 2. Else if r0' > 0, the coefficient is (r1 + 1) mod commitmentModulus.
            // 3. Else (r0' <= 0), the coefficient is (r1 - 1) mod commitmentModulus.
            //
            // The hint is public so we don't have to implement this in a sidechannel-safe manner,
            // but avoiding branches will improve performance.
            //

            // Set up masks to determine which case we fall into
            hintIsZeroMask = peHint->coeffs[j] - 1; // 0 if hint is 1, 0xFFFFFFFF if hint is 0
            r0PrimeGtZeroMask = SYMCRYPT_MASK32_LT( r0 - 1, pParams->commitmentRoundingRange );

            // Case 2: r0' > 0, so the coefficient is (r1 + 1) mod commitmentModulus,
            // i.e. (r1 + 1) if r1 != commitmentModulus - 1, else 0
            tmpMask = ~SYMCRYPT_MASK32_EQ( r1, (UINT32) (pParams->commitmentModulus - 1) );
            positiveOffsetCoeff = tmpMask & (r1 + 1);

            // Case 3: r0' <= 0, so the coefficient is (r1 - 1) mod commitmentModulus,
            // i.e. (r1 - 1) if r1 != 0,  else commitmentModulus - 1
            tmpMask = SYMCRYPT_MASK32_EQ( r1, 0 );
            negativeOffsetCoeff = ( tmpMask & (pParams->commitmentModulus - 1) ) | ( ~tmpMask & ( r1 - 1 ) );

            // Mask out each possible coefficient based on which case we fall into
            r1 &= hintIsZeroMask;
            positiveOffsetCoeff &= ~hintIsZeroMask & r0PrimeGtZeroMask;
            negativeOffsetCoeff &= ~hintIsZeroMask & ~r0PrimeGtZeroMask;

            // Sanity check: no combination of masked values should be set simultaneously
            SYMCRYPT_ASSERT( (r1 & positiveOffsetCoeff) == 0 );
            SYMCRYPT_ASSERT( (r1 & negativeOffsetCoeff) == 0 );
            SYMCRYPT_ASSERT( (positiveOffsetCoeff & negativeOffsetCoeff) == 0 );

            // Finally, we can pick the correct coefficient using our masks
            peCommitment->coeffs[j] = r1 | positiveOffsetCoeff | negativeOffsetCoeff;
        }
    }
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaPkEncode(
    PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    PBYTE               pbDst,
    SIZE_T              cbDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS pParams = pkMlDsakey->pParams;
    UINT32 cbEncodedKey = pParams->cbEncodedPublicKey;
    PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES pTemps = NULL;

    if( cbDst != cbEncodedKey )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pTemps = SymCryptMlDsaTemporariesAllocateAndInitialize( pParams, 1, 0, 0, 0 );
    if( pTemps == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    {
        PSYMCRYPT_MLDSA_VECTOR pvT1InvNTT = pTemps->pvRowVectors[0];

        PBYTE pbCurr = pbDst;
        memcpy( pbCurr, pkMlDsakey->publicSeed, SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE );

        pbCurr += SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE;

        SymCryptMlDsaVectorCopy( pkMlDsakey->pvt1, pvT1InvNTT );
        SymCryptMlDsaVectorINTT( pvT1InvNTT );

        // Pack each coefficient of T1 into 10 bits. Coefficients are rounded by Power2Round so they're
        // guaranteed to be at most 10 bits long.
        SYMCRYPT_ASSERT( cbDst - SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE == SYMCRYPT_INTERNAL_MLDSA_SIZEOF_ENCODED_VECTOR( pvT1InvNTT, 10u ) );
        SymCryptMlDsaVectorEncode( pvT1InvNTT, 10, 0, pbCurr );
    }

cleanup:
    if( pTemps != NULL)
    {
        SymCryptMlDsaTemporariesFree( pTemps );
    }

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaPkDecode(
    PCBYTE              pbSrc,
    SIZE_T              cbSrc,
    UINT32              flags,
    PSYMCRYPT_MLDSAKEY  pkMlDsakey )
{
    UNREFERENCED_PARAMETER( flags );

    // Size of one encoded polynomial from t1: 256 coefficients * 10 bits per coefficient / 8 bits per byte
    const UINT32 cbEncodedPoly = SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS * 10 / 8;

    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCBYTE pbCurr = pbSrc;
    PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES pTemps = NULL;
    PSYMCRYPT_SHAKE256_STATE pShakeState = NULL;

    if( cbSrc != pkMlDsakey->pParams->cbEncodedPublicKey )
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    // Allocate space for an encoded polynomial so we can copy the input, decode it, and append it
    // to our SHAKE state. We copy it to a local buffer so we don't violate the read-once rule when
    // appending to the SHAKE state.
    pTemps = SymCryptMlDsaTemporariesAllocateAndInitialize( pkMlDsakey->pParams, 0, 0, 0, cbEncodedPoly );
    if( pTemps == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Reset the private key state in case this key object is being reused
    if( pkMlDsakey->hasRootSeed )
    {
        SymCryptWipeKnownSize( pkMlDsakey->rootSeed, SYMCRYPT_MLDSA_ROOT_SEED_SIZE );
        pkMlDsakey->hasRootSeed = FALSE;
    }

    if( pkMlDsakey->hasPrivateKey )
    {
        SymCryptWipeKnownSize( pkMlDsakey->privateSigningSeed, SYMCRYPT_MLDSA_PRIVATE_SIGNING_SEED_SIZE );
        SymCryptMlDsaVectorSetZero( pkMlDsakey->pvs1 );
        SymCryptMlDsaVectorSetZero( pkMlDsakey->pvs2 );
        SymCryptMlDsaVectorSetZero( pkMlDsakey->pvt0 );
        pkMlDsakey->hasPrivateKey = FALSE;
    }

    memcpy( pkMlDsakey->publicSeed, pbCurr, SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE );
    pbCurr += SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE;

    pShakeState = &(pTemps->shake256State);

    SymCryptShake256Init( pShakeState );
    SymCryptShake256Append( pShakeState, pkMlDsakey->publicSeed, SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE );

    for( UINT32 i = 0; i < pkMlDsakey->pvt1->nElems; ++i )
    {
        memcpy( pTemps->pbScratch, pbCurr, cbEncodedPoly );

        PSYMCRYPT_MLDSA_POLYELEMENT peElement = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pkMlDsakey->pvt1 );

        scError = SymCryptMlDsaPolyElementDecode(
            pTemps->pbScratch,
            10,
            0,
            peElement );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        SymCryptShake256Append( pShakeState, pTemps->pbScratch, cbEncodedPoly );

        pbCurr += cbEncodedPoly;
    }

    SYMCRYPT_ASSERT( pbCurr == pbSrc + cbSrc );

    SymCryptMlDsaVectorNTT( pkMlDsakey->pvt1 );

    SymCryptMlDsaExpandA(
        pkMlDsakey->publicSeed,
        SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE,
        pkMlDsakey->pmA );

    SymCryptShake256Result( pShakeState, pkMlDsakey->publicKeyHash );

cleanup:
    if( pTemps != NULL )
    {
        SymCryptMlDsaTemporariesFree( pTemps );
    }

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSkEncode(
    PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    PBYTE               pbDst,
    SIZE_T              cbDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS pParams = pkMlDsakey->pParams;
    UINT32 cbEncodedKey = pParams->cbEncodedPrivateKey;
    PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES pTemps = NULL;
    PBYTE pbCurr = pbDst;

    if( !pkMlDsakey->hasPrivateKey )
    {
        scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
        goto cleanup;
    }

    if( cbDst != cbEncodedKey )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pTemps = SymCryptMlDsaTemporariesAllocateAndInitialize( pParams, 1, 1, 0, 0 );
    if( pTemps == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    memcpy( pbCurr, pkMlDsakey->publicSeed, SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE );
    pbCurr += SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE;

    memcpy( pbCurr, pkMlDsakey->privateSigningSeed, SYMCRYPT_MLDSA_PRIVATE_SIGNING_SEED_SIZE );
    pbCurr += SYMCRYPT_MLDSA_PRIVATE_SIGNING_SEED_SIZE;

    memcpy( pbCurr, pkMlDsakey->publicKeyHash, SYMCRYPT_MLDSA_PUBLIC_KEY_HASH_SIZE );
    pbCurr += SYMCRYPT_MLDSA_PUBLIC_KEY_HASH_SIZE;

    {
        // Inverse NTT and encode s1
        PSYMCRYPT_MLDSA_VECTOR pvs1InvNTT = pTemps->pvColVectors[0];
        SymCryptMlDsaVectorCopy( pkMlDsakey->pvs1, pvs1InvNTT );
        SymCryptMlDsaVectorINTT( pvs1InvNTT );

        SymCryptMlDsaVectorEncode(
            pvs1InvNTT,
            pParams->encodedCoefficientBitLength,
            pParams->privateKeyRange,
            pbCurr );
        pbCurr += SYMCRYPT_INTERNAL_MLDSA_SIZEOF_ENCODED_VECTOR( pvs1InvNTT, pParams->encodedCoefficientBitLength );
    }

    {
        // Inverse NTT and encode s2
        PSYMCRYPT_MLDSA_VECTOR pvs2InvNTT = pTemps->pvRowVectors[0];
        SymCryptMlDsaVectorCopy( pkMlDsakey->pvs2, pvs2InvNTT );
        SymCryptMlDsaVectorINTT( pvs2InvNTT );

        SymCryptMlDsaVectorEncode(
            pvs2InvNTT,
            pParams->encodedCoefficientBitLength,
            pParams->privateKeyRange,
            pbCurr );
        pbCurr += SYMCRYPT_INTERNAL_MLDSA_SIZEOF_ENCODED_VECTOR( pvs2InvNTT, pParams->encodedCoefficientBitLength );
    }

    {
        // Inverse NTT and encode t0
        // Can re-use the previous temporary row vector as it's no longer needed
        PSYMCRYPT_MLDSA_VECTOR pvt0InvNTT = pTemps->pvRowVectors[0];
        SymCryptMlDsaVectorCopy( pkMlDsakey->pvt0, pvt0InvNTT );
        SymCryptMlDsaVectorINTT( pvt0InvNTT );

        SymCryptMlDsaVectorEncode(
            pvt0InvNTT,
            SYMCRYPT_POWER2ROUND_LOW_ORDER_BITS,
            1 << (SYMCRYPT_POWER2ROUND_LOW_ORDER_BITS - 1),
            pbCurr );
        pbCurr += SYMCRYPT_INTERNAL_MLDSA_SIZEOF_ENCODED_VECTOR( pvt0InvNTT, SYMCRYPT_POWER2ROUND_LOW_ORDER_BITS );
    }

    SYMCRYPT_ASSERT( pbCurr == pbDst + cbDst );

cleanup:
    if( pTemps != NULL)
    {
        SymCryptMlDsaTemporariesFree( pTemps );
    }

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSkDecode(
    PCBYTE              pbSrc,
    SIZE_T              cbSrc,
    UINT32              flags,
    PSYMCRYPT_MLDSAKEY  pkMlDsakey )
{
    UNREFERENCED_PARAMETER( flags );

    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS pParams = pkMlDsakey->pParams;
    PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES pTemps = NULL;
    PCBYTE pbCurr = pbSrc;
    BYTE pubKeyHashTmp[SYMCRYPT_MLDSA_PUBLIC_KEY_HASH_SIZE];

    if( cbSrc != pkMlDsakey->pParams->cbEncodedPrivateKey )
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    pTemps = SymCryptMlDsaTemporariesAllocateAndInitialize(
        pParams,
        2, // row vectors - t, t0
        0, // column vectors
        1, // poly elements - temporary space
        pkMlDsakey->pParams->cbEncodedPublicKey ); // scratch space
    if( pTemps == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // We use temporaries to recalculate t, t0, t1 and the public key hash from the import values
    // for A (derived from the public seed), s1 and s2. This is not strictly necessary for callers
    // who just want to import a private key and use it for signing, but it allows the private
    // key to also be used for verification, and more importantly, provides extra robustness by
    // ensuring that the derived values are consistent with the encoded values. If the perf of
    // importing a key becomes a concern, we can move this recalculation to the first time the key
    // is used for verification.
    PSYMCRYPT_MLDSA_VECTOR pvtTmp = pTemps->pvRowVectors[0];
    PSYMCRYPT_MLDSA_VECTOR pvt0Tmp = pTemps->pvRowVectors[1];
    PSYMCRYPT_MLDSA_POLYELEMENT peTmp = pTemps->pePolyElements[0];
    PBYTE pbEncodedPubKeyTmp = pTemps->pbScratch;

    // Reset the key state in case this key is being reused
    pkMlDsakey->hasRootSeed = FALSE;
    pkMlDsakey->hasPrivateKey = FALSE;
    SymCryptWipeKnownSize( pkMlDsakey->rootSeed, SYMCRYPT_MLDSA_ROOT_SEED_SIZE );

    memcpy( pkMlDsakey->publicSeed, pbCurr, SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE );
    pbCurr += SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE;

    memcpy( pkMlDsakey->privateSigningSeed, pbCurr, SYMCRYPT_MLDSA_PRIVATE_SIGNING_SEED_SIZE );
    pbCurr += SYMCRYPT_MLDSA_PRIVATE_SIGNING_SEED_SIZE;

    memcpy( pubKeyHashTmp, pbCurr, SYMCRYPT_MLDSA_PUBLIC_KEY_HASH_SIZE );
    pbCurr += SYMCRYPT_MLDSA_PUBLIC_KEY_HASH_SIZE;

    // Expand A matrix
    SymCryptMlDsaExpandA( pkMlDsakey->publicSeed, SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE, pkMlDsakey->pmA );

    scError = SymCryptMlDsaVectorDecode(
        pbCurr,
        pParams->encodedCoefficientBitLength,
        pParams->privateKeyRange,
        pkMlDsakey->pvs1 );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }
    pbCurr += SYMCRYPT_INTERNAL_MLDSA_SIZEOF_ENCODED_VECTOR( pkMlDsakey->pvs1, pParams->encodedCoefficientBitLength );


    scError = SymCryptMlDsaVectorDecode(
        pbCurr,
        pParams->encodedCoefficientBitLength,
        pParams->privateKeyRange,
        pkMlDsakey->pvs2 );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }
    pbCurr += SYMCRYPT_INTERNAL_MLDSA_SIZEOF_ENCODED_VECTOR( pkMlDsakey->pvs2, pParams->encodedCoefficientBitLength );

    scError = SymCryptMlDsaVectorDecode(
        pbCurr,
        SYMCRYPT_POWER2ROUND_LOW_ORDER_BITS,
        1 << (SYMCRYPT_POWER2ROUND_LOW_ORDER_BITS - 1),
        pvt0Tmp );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }
    pbCurr += SYMCRYPT_INTERNAL_MLDSA_SIZEOF_ENCODED_VECTOR( pkMlDsakey->pvt0, SYMCRYPT_POWER2ROUND_LOW_ORDER_BITS );

    SYMCRYPT_ASSERT( pbCurr == pbSrc + cbSrc );

    // Convert s1 and s2 to NTT form
    SymCryptMlDsaVectorNTT( pkMlDsakey->pvs1 );
    SymCryptMlDsaVectorNTT( pkMlDsakey->pvs2 );

    SymCryptMlDsakeyComputeT(
        pkMlDsakey->pmA,
        pkMlDsakey->pvs1,
        pkMlDsakey->pvs2,
        pkMlDsakey->pvt0,
        pkMlDsakey->pvt1,
        pvtTmp,
        peTmp );

    // If the recalculated t0 doesn't match, the imported key is invalid.
    // Note: SymCryptMlDsakeyComputeT sets t0 and t1 in NTT form.
    if( memcmp( pkMlDsakey->pvt0, pvt0Tmp, pvt0Tmp->cbTotalSize ) != 0 )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    // Convert t0 and t1 to NTT form
    SymCryptMlDsaVectorNTT( pkMlDsakey->pvt0 );
    SymCryptMlDsaVectorNTT( pkMlDsakey->pvt1 );

    scError = SymCryptMlDsaPkEncode( pkMlDsakey, pbEncodedPubKeyTmp, pParams->cbEncodedPublicKey );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // Recalculate the public key hash and compare it to the imported value. If they don't match,
    // the imported key is invalid.
    SymCryptShake256(
        pbEncodedPubKeyTmp,
        pParams->cbEncodedPublicKey,
        pkMlDsakey->publicKeyHash,
        SYMCRYPT_MLDSA_PUBLIC_KEY_HASH_SIZE );

    if( memcmp( pkMlDsakey->publicKeyHash, pubKeyHashTmp, SYMCRYPT_MLDSA_PUBLIC_KEY_HASH_SIZE ) != 0 )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    pkMlDsakey->hasPrivateKey = TRUE;

cleanup:
    if( pTemps != NULL )
    {
        SymCryptMlDsaTemporariesFree( pTemps );
    }

    // Wipe private state on error as defense-in-depth
    if( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptWipeKnownSize( pkMlDsakey->privateSigningSeed, SYMCRYPT_MLDSA_PRIVATE_SIGNING_SEED_SIZE );
        SymCryptMlDsaVectorSetZero( pkMlDsakey->pvs1 );
        SymCryptMlDsaVectorSetZero( pkMlDsakey->pvs2 );
        SymCryptMlDsaVectorSetZero( pkMlDsakey->pvt0 );
    }

    return scError;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaSigEncode(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    PBYTE                               pbCommitmentHash,
    SIZE_T                              cbCommitmentHash,
    PCSYMCRYPT_MLDSA_VECTOR             pvResponse,
    PCSYMCRYPT_MLDSA_VECTOR             pvHint,
    PBYTE                               pbDst,
    SIZE_T                              cbDst )
{
    SYMCRYPT_ASSERT( cbDst == pParams->cbEncodedSignature );
    UNREFERENCED_PARAMETER( cbDst );

    const SIZE_T cbEncodedHint = pParams->nHintNonZeroCoeffs + pvHint->nElems;

    PBYTE pbCurr = pbDst;

    memcpy( pbCurr, pbCommitmentHash, cbCommitmentHash );
    pbCurr += cbCommitmentHash;

    SymCryptMlDsaVectorEncode(
        pvResponse,
        pParams->maskCoefficientRangeLog2 + 1,
        1 << pParams->maskCoefficientRangeLog2,
        pbCurr );

    pbCurr += SYMCRYPT_INTERNAL_MLDSA_SIZEOF_ENCODED_VECTOR( pvResponse, pParams->maskCoefficientRangeLog2 + 1 );

    SymCryptMlDsaHintBitPack( pParams, pvHint, pbCurr );
    pbCurr += cbEncodedHint;

    SYMCRYPT_ASSERT( pbCurr == pbDst + cbDst );
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSigDecode(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    PCBYTE                              pbSig,
    SIZE_T                              cbSig,
    PBYTE                               pbCommitmentHash,
    SIZE_T                              cbCommitmentHash,
    PSYMCRYPT_MLDSA_VECTOR              pvResponse,
    PSYMCRYPT_MLDSA_VECTOR              pvHint )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    if( cbSig != pParams->cbEncodedSignature )
    {
        scError = SYMCRYPT_WRONG_DATA_SIZE;
        goto cleanup;
    }

    SYMCRYPT_ASSERT( cbCommitmentHash == pParams->cbCommitmentHash );

    PCBYTE pbCurr = pbSig;

    memcpy( pbCommitmentHash, pbSig, cbCommitmentHash );
    pbCurr += cbCommitmentHash;

    SymCryptMlDsaVectorDecode(
        pbCurr,
        pParams->maskCoefficientRangeLog2 + 1,
        1 << pParams->maskCoefficientRangeLog2,
        pvResponse );
    pbCurr += SYMCRYPT_INTERNAL_MLDSA_SIZEOF_ENCODED_VECTOR( pvResponse, pParams->maskCoefficientRangeLog2 + 1 );

    SYMCRYPT_ASSERT( cbSig - (pbCurr - pbSig) == (SIZE_T) pParams->nHintNonZeroCoeffs + pvHint->nElems );

    scError = SymCryptMlDsaHintBitUnpack( pParams, pbCurr, pvHint );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    return scError;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaHintBitPack(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    PCSYMCRYPT_MLDSA_VECTOR             pvSrc,
    PBYTE                               pbDst )
{
    SymCryptWipe( pbDst, pParams->nHintNonZeroCoeffs );

    UINT32 index = 0;
    for( UINT32 i = 0; i < pvSrc->nElems; ++i )
    {
        PSYMCRYPT_MLDSA_POLYELEMENT peElement = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvSrc );
        for( UINT32 j = 0; j < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; ++j )
        {
            // Side channel safety: the hint is public (part of the signature) so it's okay to
            // leak information here
            if( peElement->coeffs[j] != 0 )
            {
                // Each byte in the hint is the index of a non-zero coefficient
                pbDst[index] = (BYTE) j;
                index++;
            }
        }

        // The number of non-zero coefficients in polynomials 0..i is stored in the
        // (nHintNonZeroCoeffs + i)th byte. This allows us to determine which indices correspond
        // to which polynomials during decoding while still only using one byte per index.
        SYMCRYPT_ASSERT( index <= pParams->nHintNonZeroCoeffs );
        pbDst[pParams->nHintNonZeroCoeffs + i] = (BYTE) index;
    }
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaHintBitUnpack(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    PCBYTE                              pbSrc,
    PSYMCRYPT_MLDSA_VECTOR              pvDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 index = 0;
    UINT32 maxIndex = 0;
    UINT32 first = 0;

    for( UINT32 i = 0; i < pvDst->nElems; ++i )
    {
        PSYMCRYPT_MLDSA_POLYELEMENT peElement = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvDst );

        // Ensure pvDst is zeroed out before unpacking
        SymCryptMlDsaPolyElementSetZero( peElement );

        maxIndex = pbSrc[pParams->nHintNonZeroCoeffs + i];
        if( ( maxIndex < index) ||
            ( maxIndex > pParams->nHintNonZeroCoeffs) )
        {
            // Invalid input
            scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
            goto cleanup;
        }

        first = index;
        while( index < maxIndex )
        {
            if( index > first && pbSrc[index - 1] >= pbSrc[index])
            {
                // Invalid input
                scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
                goto cleanup;
            }

            peElement->coeffs[pbSrc[index]] = 1;
            index++;
        }
    }

    for(UINT32 leftover = index; leftover < pParams->nHintNonZeroCoeffs; ++leftover)
    {
        if( pbSrc[leftover] != 0 )
        {
            // Invalid input
            scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
            goto cleanup;
        }
    }

cleanup:
    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHashMlDsaValidateHashAlgAndGetOid(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    SYMCRYPT_PQDSA_HASH_ID              hashAlg,
    SIZE_T                              cbHash,
    PCSYMCRYPT_OID*                     ppOid )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_OID pHashOid = NULL;
    BOOLEAN fFound = FALSE;
    BOOLEAN fIsXof = FALSE;
    SIZE_T cbHashExpected = 0;

    for( UINT32 i = 0; i < SYMCRYPT_ARRAY_SIZE(g_hashOidMap); ++i )
    {
        if( g_hashOidMap[i].hashId == hashAlg )
        {
            fFound = TRUE;
            pHashOid = g_hashOidMap[i].pOid;
            fIsXof = g_hashOidMap[i].fIsXof;
            cbHashExpected = g_hashOidMap[i].pHashAlgorithm->resultSize;
            break;
        }
    }

    if( !fFound )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    SYMCRYPT_ASSERT( pHashOid->cbOID == SYMCRYPT_MLDSA_SUPPORTED_HASH_OID_SIZE );

    // For traditional hash algorithms (non-XOFs), the hash length must exactly match the expected
    // value. For XOFs, the output length is arbitrary, and any length is acceptable as long as it
    // meets the minimum collision strength specified by the parameter set (cbCommitmentHash)
    if( (!fIsXof && cbHash != cbHashExpected ) ||
        ( cbHash < pParams->cbCommitmentHash ) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    *ppOid = pHashOid;

cleanup:
    return scError;
}

FORCEINLINE
INT32
SYMCRYPT_CALL
SymCryptMlDsaModPlusMinus( UINT32 r, UINT32 modulus )
{
    SYMCRYPT_ASSERT( r < modulus );

    // In most cases this function is used with even moduli, e.g. with Power2Round.
    // However, it's okay if the modulus is odd. FIPS 204 specifies that the output is in the range
    // (-ceil(modulus/2), floor(modulus/2) ]
    // = ( -((modulus + 1) // 2), modulus // 2 ]
    // = [ -modulus // 2, modulus // 2 ]
    const INT32 halfModulus = modulus >> 1;

    // Mask for conditional subtraction: 0 if r <= (modulus/2), 0xFFFFFFFF otherwise
    UINT32 subtractionMask = SYMCRYPT_MASK32_LT( halfModulus, r );

    INT32 r0 = (INT32) r - (modulus & subtractionMask);
    SYMCRYPT_ASSERT( r0 > -halfModulus && r0 <= halfModulus);

    return r0;
}

_Use_decl_annotations_
FORCEINLINE
UINT32
SYMCRYPT_CALL
SymCryptMlDsaPolyElementInfinityNorm( PCSYMCRYPT_MLDSA_POLYELEMENT peSrc )
{
    UINT32 norm = 0;
    UINT32 mask;
    INT32 curr;
    for( UINT32 i = 0; i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++ )
    {
        // Convert the coefficient to signed form
        curr = SymCryptMlDsaModPlusMinus( peSrc->coeffs[i], SYMCRYPT_MLDSA_Q );

        // If the coefficient is less than 0, negate it
        mask = SYMCRYPT_MASK32_LT( curr, 0 );
        curr = (curr & ~mask) | ((curr * -1) & mask);

        // If the coefficient is greater than the current norm, update the norm
        mask = SYMCRYPT_MASK32_LT( norm, curr );
        norm = (norm & ~mask) | (curr & mask);
    }

    return norm;
}

_Use_decl_annotations_
UINT32
SYMCRYPT_CALL
SymCryptMlDsaVectorInfinityNorm( PCSYMCRYPT_MLDSA_VECTOR pvSrc )
{
    UINT32 norm = 0;
    UINT32 curr;
    INT32 mask;
    for( UINT32 i = 0; i < pvSrc->nElems; i++ )
    {
        curr = SymCryptMlDsaPolyElementInfinityNorm( SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT(i, pvSrc) );
        mask = SYMCRYPT_MASK32_LT( norm, curr );
        norm = (norm & ~mask) | (curr & mask);
    }

    return norm;
}

_Use_decl_annotations_
FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptMlDsaDecompose(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS pParams,
    UINT32                           r,
    UINT32                           *puR1,
    UINT32                           *puR0 )
{
    SYMCRYPT_ASSERT( r < SYMCRYPT_MLDSA_Q );
    SYMCRYPT_ASSERT( puR1 != NULL || puR0 != NULL );

    UINT32 r1 = 0;
    UINT32 mask = 0;
    INT32 r0 = 0;

    // Some tricks for calculating this are borrowed from the reference implementation
    // https://github.com/pq-crystals/dilithium/blob/master/ref/rounding.c
    //
    // The multiplication constants for calculating r1 are in the PCSYMCRYPT_MLDSA_INTERNAL_PARAMS
    // structure. They are calculated as follows.
    //
    // To keep intermediate values in the 32-bit range, instead of using r directly, we calculate
    // ceil( r/128 ). We likewise divide the commitment rounding range by 128.
    //
    // For ML-DSA 44:
    //     2*commmitmentRoundingRange = 2 * 95,232 = 190,464
    //     190464 // 128 = 1488
    //     1 / 1488 ~= floor(2^24 // 1488) * 2^24 = 11,275 // 2^24
    // For ML-DSA 65 and 87:
    //     2*commmitmentRoundingRange = 2*261888 = 523776
    //     523776 // 128 = 4092
    //     1 / 4092 ~= floor(2^22 // 4092) * 2^22 = 1025 // 2^22 = 4100 // 2^24
    //

    UINT32 rdiv128 = (r + 127) >> 7;
    r1 = ( rdiv128 * pParams->decomposeR1Factor + (1 << 23)) >> 24;

    // Handle corner case: if r1 is outside of the expected range, set it to 0
    mask = SYMCRYPT_MASK32_LT( r1, pParams->commitmentModulus );
    r1 &= mask;

    r0 = r - ( r1 * 2 * pParams->commitmentRoundingRange );

    // Handle corner case for r0
    r0 -= ((((SYMCRYPT_MLDSA_Q - 1) >> 1) - r0) >> 31) & SYMCRYPT_MLDSA_Q;
    r0 = SymCryptMlDsaSignedCoefficientModQ( r0 );

    if( puR1 != NULL )
    {
        *puR1 = r1;
    }

    if( puR0 != NULL )
    {
        *puR0 = r0;
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorHighBits(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    PCSYMCRYPT_MLDSA_VECTOR             pvSrc,
    PSYMCRYPT_MLDSA_VECTOR              pvDst )
{
    SYMCRYPT_ASSERT( pvSrc->nElems == pvDst->nElems );

    for(UINT32 i = 0; i < pvSrc->nElems; i++)
    {
        for(UINT32 j = 0; j < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; j++)
        {
            SymCryptMlDsaDecompose(
                pParams,
                SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvSrc )->coeffs[j],
                &(SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvDst )->coeffs[j]),
                NULL );
        }
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorLowBits(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    PCSYMCRYPT_MLDSA_VECTOR             pvSrc,
    PSYMCRYPT_MLDSA_VECTOR              pvDst )
{
    SYMCRYPT_ASSERT( pvSrc->nElems == pvDst->nElems );

    for(UINT32 i = 0; i < pvSrc->nElems; i++)
    {
        for(UINT32 j = 0; j < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; j++)
        {
            SymCryptMlDsaDecompose(
                pParams,
                SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvSrc )->coeffs[j],
                NULL,
                &(SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvDst )->coeffs[j]) );
        }
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaPower2Round(
    UINT32  r,
    UINT32* puR1,
    UINT32* puR0 )
{
    SYMCRYPT_ASSERT( r < SYMCRYPT_MLDSA_Q );

    UINT32 rPrime = r & ( (1 << SYMCRYPT_POWER2ROUND_LOW_ORDER_BITS) - 1 ); // r mod 2^d
    INT32 r0 = SymCryptMlDsaModPlusMinus( rPrime, 1 << SYMCRYPT_POWER2ROUND_LOW_ORDER_BITS );

    UINT32 r1 = (r - r0) >> SYMCRYPT_POWER2ROUND_LOW_ORDER_BITS;

    *puR1 = r1;
    *puR0 = SymCryptMlDsaSignedCoefficientModQ( r0 ); //, 4096 );
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementPower2Round(
    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc,
    PSYMCRYPT_MLDSA_POLYELEMENT     peDst1,
    PSYMCRYPT_MLDSA_POLYELEMENT     peDst0 )
{
    UINT32 r1, r0;

    for(UINT32 i = 0; i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++)
    {
        SymCryptMlDsaPower2Round( peSrc->coeffs[i], &r1, &r0 );
        peDst1->coeffs[i] = r1;
        peDst0->coeffs[i] = r0;
    }
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorPower2Round(
    PCSYMCRYPT_MLDSA_VECTOR pvSrc,
    PSYMCRYPT_MLDSA_VECTOR  pvDst1,
    PSYMCRYPT_MLDSA_VECTOR  pvDst0 )
{
    SYMCRYPT_ASSERT( pvSrc->nElems == pvDst1->nElems );
    SYMCRYPT_ASSERT( pvSrc->nElems == pvDst0->nElems );

    for(UINT32 i = 0; i < pvSrc->nElems; i++)
    {
        SymCryptMlDsaPolyElementPower2Round(
            SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvSrc ),
            SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvDst1 ),
            SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvDst0 ) );
    }
}

FORCEINLINE
UINT32
SYMCRYPT_CALL
SymCryptMlDsaSignedCoefficientModQ( INT32 coefficient )
{
    SYMCRYPT_ASSERT(coefficient > -1 * SYMCRYPT_MLDSA_Q && coefficient < SYMCRYPT_MLDSA_Q);

    UINT32 result;
    UINT32 negativeMask = SYMCRYPT_MASK32_LT( coefficient, 0 );

    result = coefficient + (SYMCRYPT_MLDSA_Q & negativeMask);
    SYMCRYPT_ASSERT( result < SYMCRYPT_MLDSA_Q );

    return result;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementEncode(
    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc,
    UINT32                          nBitsPerCoefficient,
    UINT32                          signedCoefficientBound,
    PBYTE                           pbDst )
{
    SYMCRYPT_ASSERT( nBitsPerCoefficient >  0  );
    SYMCRYPT_ASSERT( nBitsPerCoefficient <= 20 ); // Maximum number of bits per coefficient across all encodings

    INT32 coefficient;
    UINT32 nBitsInCoefficient;
    UINT32 bitsToEncode;
    UINT32 nBitsToEncode;
    UINT32 cbDstWritten = 0;
    UINT32 accumulator = 0;
    UINT32 nBitsInAccumulator = 0;

    for(UINT32 i = 0; i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++ )
    {
        nBitsInCoefficient = nBitsPerCoefficient;
        coefficient = peSrc->coeffs[i];

        SYMCRYPT_ASSERT( coefficient < SYMCRYPT_MLDSA_Q );

        // If the coefficient is greater than the signedCoefficientBound, that means it is
        // a negative value modulo Q, so we need to subtract Q to get the original value.
        coefficient -= ( SYMCRYPT_MLDSA_Q & SYMCRYPT_MASK32_LT( signedCoefficientBound, coefficient ) );

        // The coefficient is now in the range [-signedCoefficientBound, signedCoefficientBound],
        // we need to map it to the range [0, 2*signedCoefficientBound] for encoding.
        coefficient = SYMCRYPT_INTERNAL_MLDSA_SHORT_COEFFICIENT_ENCODE_DECODE( coefficient, signedCoefficientBound );

        // Some coefficients are always positive and so do not need any special encoding. In this
        // case, we revert to the original value from the source polynomial.
        coefficient = ( peSrc->coeffs[i] & SYMCRYPT_MASK32_ZERO( signedCoefficientBound ) ) |
            ( coefficient & SYMCRYPT_MASK32_NONZERO( signedCoefficientBound ) );

        SYMCRYPT_ASSERT( coefficient >= 0 );
        SYMCRYPT_ASSERT( signedCoefficientBound == 0 || (UINT32) coefficient <= signedCoefficientBound * 2 );
        SYMCRYPT_ASSERT( (UINT32) coefficient < (1ul << nBitsPerCoefficient) );

        // encode the coefficient
        // simple loop to add bits to accumulator and write accumulator to output
        do
        {
            nBitsToEncode = SYMCRYPT_MIN( nBitsInCoefficient, 32 - nBitsInAccumulator );

            bitsToEncode = coefficient & ( ( 1UL << nBitsToEncode ) - 1 );
            coefficient >>= nBitsToEncode;
            nBitsInCoefficient -= nBitsToEncode;

            accumulator |= ( bitsToEncode << nBitsInAccumulator );
            nBitsInAccumulator += nBitsToEncode;
            if(nBitsInAccumulator == 32)
            {
                SYMCRYPT_STORE_LSBFIRST32( pbDst + cbDstWritten, accumulator );
                cbDstWritten += 4;
                accumulator = 0;
                nBitsInAccumulator = 0;
            }
        } while( nBitsInCoefficient > 0 );
    }

    SYMCRYPT_ASSERT(nBitsInAccumulator == 0);
    SYMCRYPT_ASSERT(cbDstWritten == (nBitsPerCoefficient*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8)));
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaPolyElementDecode(
    PCBYTE                      pbSrc,
    UINT32                      nBitsPerCoefficient,
    UINT32                      signedCoefficientBound,
    PSYMCRYPT_MLDSA_POLYELEMENT peDst )
{
    SYMCRYPT_ASSERT( nBitsPerCoefficient >  0  );
    SYMCRYPT_ASSERT( nBitsPerCoefficient <= 20 ); // Maximum number of bits per coefficient across all encodings

    INT32 coefficient;
    UINT32 nBitsInCoefficient;
    UINT32 bitsToDecode;
    UINT32 nBitsToDecode;
    UINT32 cbSrcRead = 0;
    UINT32 accumulator = 0;
    UINT32 nBitsInAccumulator = 0;

    for(UINT32 i = 0; i < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++ )
    {
        coefficient = 0;
        nBitsInCoefficient = 0;

        // first gather and decode bits from pbSrc
        do
        {
            if(nBitsInAccumulator == 0)
            {
                accumulator = SYMCRYPT_LOAD_LSBFIRST32( pbSrc+cbSrcRead );
                cbSrcRead += 4;
                nBitsInAccumulator = 32;
            }

            nBitsToDecode = SYMCRYPT_MIN(nBitsPerCoefficient-nBitsInCoefficient, nBitsInAccumulator);
            SYMCRYPT_ASSERT(nBitsToDecode <= nBitsInAccumulator);

            bitsToDecode = accumulator & ((1UL<<nBitsToDecode)-1);
            accumulator >>= nBitsToDecode;
            nBitsInAccumulator -= nBitsToDecode;

            coefficient |= (bitsToDecode << nBitsInCoefficient);
            nBitsInCoefficient += nBitsToDecode;
        } while( nBitsPerCoefficient > nBitsInCoefficient );
        SYMCRYPT_ASSERT( nBitsInCoefficient == nBitsPerCoefficient );

        // Coefficient should always be positive at this point since it's encoded in <= 20 bits
        SYMCRYPT_ASSERT( coefficient >= 0 );

        if( ( signedCoefficientBound != 0 ) && ( (UINT32) coefficient > 2 * signedCoefficientBound ) )
        {
            // Most of the encoded components of ML-DSA keys and signatures cannot be outside the
            // valid range, because the number of bits in their encodings do not permit invalid
            // values. However, the private key components s1 and s2 have encodings that do allow
            // for invalid values (because their valid ranges are [-2, 2] or [-4, 4]). If any
            // coefficient is outside this range, the key is invalid and we return an error.
            // We do not need to do this check in constant time because we treat the validity of an
            // imported key as public information.
            return SYMCRYPT_INVALID_BLOB;
        }

        // If this coefficient is intended to be signed, we need to decode it into its original
        // signed form and then map it modulo Q.
        // Side-channel safety: signedCoefficientBound just indicates which component we are
        // decoding, so it's public information
        if( signedCoefficientBound != 0 )
        {
            coefficient = SYMCRYPT_INTERNAL_MLDSA_SHORT_COEFFICIENT_ENCODE_DECODE( coefficient, signedCoefficientBound );
            coefficient = SymCryptMlDsaSignedCoefficientModQ( coefficient );
        }

        peDst->coeffs[i] = coefficient;
    }

    SYMCRYPT_ASSERT(nBitsInAccumulator == 0);
    SYMCRYPT_ASSERT(cbSrcRead == (nBitsPerCoefficient*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8)));

    return SYMCRYPT_NO_ERROR;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorEncode(
    PCSYMCRYPT_MLDSA_VECTOR             pvSrc,
    UINT32                              nBitsPerCoefficient,
    UINT32                              signedCoefficientBound,
    PBYTE                               pbDst )
{
    PBYTE pbCurr = pbDst;
    const SIZE_T cbEncodedPoly = nBitsPerCoefficient * ( SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8 );

    for( UINT32 i = 0; i < pvSrc->nElems; ++i )
    {
        PCSYMCRYPT_MLDSA_POLYELEMENT peElement = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvSrc );

        SymCryptMlDsaPolyElementEncode(
            peElement,
            nBitsPerCoefficient,
            signedCoefficientBound,
            pbCurr );

        pbCurr += cbEncodedPoly;
    }

    SYMCRYPT_ASSERT( pbCurr == pbDst + ( pvSrc->nElems * cbEncodedPoly ) );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaVectorDecode(
    _In_reads_bytes_( pvDst->nElems * nBitsPerCoefficient * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8) )
            PCBYTE                              pbSrc,
            UINT32                              nBitsPerCoefficient,
            UINT32                              signedCoefficientBound,
    _Inout_ PSYMCRYPT_MLDSA_VECTOR              pvDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCBYTE pbCurr = pbSrc;
    const SIZE_T cbEncodedPoly = nBitsPerCoefficient * ( SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8 );

    for( UINT32 i = 0; i < pvDst->nElems; ++i )
    {
        PSYMCRYPT_MLDSA_POLYELEMENT peElement = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvDst );

        scError = SymCryptMlDsaPolyElementDecode(
            pbCurr,
            nBitsPerCoefficient,
            signedCoefficientBound,
            peElement );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            // Side-channel safety: the validity of an imported key is public information.
            // See comment in SymCryptMlDsaPolyElementDecode.
            goto cleanup;
        }

        pbCurr += cbEncodedPoly;
    }

    SYMCRYPT_ASSERT( pbCurr == pbSrc + ( pvDst->nElems * cbEncodedPoly ) );

cleanup:
    return scError;
}

_Use_decl_annotations_
PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES
SYMCRYPT_CALL
SymCryptMlDsaTemporariesAllocateAndInitialize(
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    UINT32                              nRowVectors,
    UINT32                              nColVectors,
    UINT32                              nPolyElements,
    UINT32                              cbScratch )
{
    // Round scratch space to nearest multiple of 8 for alignment
    cbScratch = (cbScratch + 7) & ~7;

    UINT32 cbTotalSize = sizeof( SYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES) +
        ( nRowVectors * sizeof( PSYMCRYPT_MLDSA_VECTOR ) ) +        // Row vector pointers
        ( nColVectors * sizeof( PSYMCRYPT_MLDSA_VECTOR ) ) +        // Col vector pointers
        ( nPolyElements * sizeof( PSYMCRYPT_MLDSA_POLYELEMENT ) ) + // Poly element pointers
        ( nRowVectors * pParams->cbRowVector ) +                    // Row vector buffer
        ( nColVectors * pParams->cbColVector ) +                    // Col vector buffer
        ( nPolyElements * pParams->cbPolyElement ) +                // Poly element buffer
        ( cbScratch );                                              // Scratch buffer

    PBYTE pbBuffer = SymCryptCallbackAlloc( cbTotalSize );
    if( pbBuffer == NULL )
    {
        return NULL;
    }

    SymCryptWipe( pbBuffer, cbTotalSize );

    PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES pTemporaries =
        (PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES) pbBuffer;

    pTemporaries->cbTotalSize = cbTotalSize;
    pTemporaries->nRowVectors = nRowVectors;
    pTemporaries->nColVectors = nColVectors;
    pTemporaries->nPolyElements = nPolyElements;
    pTemporaries->cbScratch = cbScratch;

    PBYTE pbCurrent = pbBuffer + sizeof( SYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES );

    if( nRowVectors > 0 )
    {
        pTemporaries->pvRowVectors = (PSYMCRYPT_MLDSA_VECTOR *) pbCurrent;
        pbCurrent += nRowVectors * sizeof( PSYMCRYPT_MLDSA_VECTOR );
    }

    if( nColVectors > 0 )
    {
        pTemporaries->pvColVectors = (PSYMCRYPT_MLDSA_VECTOR *) pbCurrent;
        pbCurrent += nColVectors * sizeof( PSYMCRYPT_MLDSA_VECTOR );
    }

    if( nPolyElements > 0 )
    {
        pTemporaries->pePolyElements = (PSYMCRYPT_MLDSA_POLYELEMENT *) pbCurrent;
        pbCurrent += nPolyElements * sizeof( PSYMCRYPT_MLDSA_POLYELEMENT );
    }

    for(UINT32 i = 0; i < nRowVectors; i++)
    {
        pTemporaries->pvRowVectors[i] = SymCryptMlDsaVectorCreate( pbCurrent, pParams->cbRowVector, pParams->nRows );
        pbCurrent += pParams->cbRowVector;
    }

    for(UINT32 i = 0; i < nColVectors; i++)
    {
        pTemporaries->pvColVectors[i] = SymCryptMlDsaVectorCreate( pbCurrent, pParams->cbColVector, pParams->nCols );
        pbCurrent += pParams->cbColVector;
    }

    for(UINT32 i = 0; i < nPolyElements; i++)
    {
        pTemporaries->pePolyElements[i] = SymCryptMlDsaPolyElementCreate( pbCurrent, pParams->cbPolyElement );
        pbCurrent += pParams->cbPolyElement;
    }

    if( cbScratch > 0 )
    {
        pTemporaries->pbScratch = pbCurrent;
        pbCurrent += cbScratch;
    }

    SYMCRYPT_ASSERT( pbCurrent == pbBuffer + cbTotalSize );

    SYMCRYPT_SET_MAGIC( pTemporaries );

    return pTemporaries;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsaTemporariesFree(
    PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES pTemporaries )
{
    SYMCRYPT_CHECK_MAGIC( pTemporaries );

    SymCryptWipe( pTemporaries, pTemporaries->cbTotalSize );
    SymCryptCallbackFree( pTemporaries );
}
