//
// ec_dispatch.c   Dispatch file for elliptic curve crypto functions
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//

#include "precomp.h"

// Table with all the pointers to SYMCRYPT_ECURVE_FUNCTIONS
const SYMCRYPT_ECURVE_FUNCTIONS SymCryptEcurveDispatchTable[] =
{
    // NULL Type
    {
        NULL,       // SymCryptEcpointSetZeroNotImplemented,
        NULL,       // SymCryptEcpointSetDistinguishedPointNotImplemented,
        NULL,       // SymCryptEcpointSetRandomNotImplemented,
        NULL,       // SymCryptEcpointIsEqualNotImplemented,
        NULL,       // SymCryptEcpointIsZeroNotImplemented,
        NULL,       // SymCryptEcpointOnCurveNotImplemented,
        NULL,       // SymCryptEcpointAddNotImplemented,
        NULL,       // SymCryptEcpointAddDiffNonZeroNotImplemented,
        NULL,       // SymCryptEcpointDoubleNotImplemented,
        NULL,       // SymCryptEcpointNegateNotImplemented,
        NULL,       // SymCryptEcpointScalarMulNotImplemented,
        NULL,       // SymCryptEcpointMultiScalarMulNotImplemented,
        NULL,       // SymCryptEcurveFillScratchSpacesNotImplemented,
    },
    // Short Weierstrass
    {
        SymCryptShortWeierstrassSetZero,
        SymCryptShortWeierstrassSetDistinguished,
        SymCryptEcpointGenericSetRandom,
        SymCryptShortWeierstrassIsEqual,
        SymCryptShortWeierstrassIsZero,
        SymCryptShortWeierstrassOnCurve,
        SymCryptShortWeierstrassAdd,
        SymCryptShortWeierstrassAddDiffNonZero,
        SymCryptShortWeierstrassDouble,
        SymCryptShortWeierstrassNegate,
        SymCryptEcpointScalarMulFixedWindow,
        SymCryptEcpointMultiScalarMulWnafWithInterleaving,
        SymCryptShortWeierstrassFillScratchSpaces,
    },
    // Twisted Edwards
    {
        SymCryptTwistedEdwardsSetZero,
        SymCryptTwistedEdwardsSetDistinguished,
        SymCryptEcpointGenericSetRandom,
        SymCryptTwistedEdwardsIsEqual,
        SymCryptTwistedEdwardsIsZero,
        SymCryptTwistedEdwardsOnCurve,
        SymCryptTwistedEdwardsAdd,
        SymCryptTwistedEdwardsAddDiffNonZero,
        SymCryptTwistedEdwardsDouble,
        SymCryptTwistedEdwardsNegate,
        SymCryptEcpointScalarMulFixedWindow,
        SymCryptEcpointMultiScalarMulWnafWithInterleaving,
        SymCryptTwistedEdwardsFillScratchSpaces,
    },
    // Montgomery
    {
        NULL,       // SymCryptEcpointSetZeroNotImplemented,
        SymCryptMontgomerySetDistinguished,
        SymCryptEcpointGenericSetRandom,
        SymCryptMontgomeryIsEqual,
        SymCryptMontgomeryIsZero,
        NULL,       // SymCryptEcpointOnCurveNotImplemented,
        NULL,       // SymCryptEcpointAddNotImplemented,
        NULL,       // SymCryptEcpointAddDiffNonZeroNotImplemented,
        NULL,       // SymCryptEcpointDoubleNotImplemented,
        NULL,       // SymCryptEcpointNegateNotImplemented,
        SymCryptMontgomeryPointScalarMul,
        NULL,       // SymCryptEcpointMultiScalarMulNotImplemented,
        SymCryptMontgomeryFillScratchSpaces,
    },
    // Short Weierstrass with A==-3
    {
        SymCryptShortWeierstrassSetZero,
        SymCryptShortWeierstrassSetDistinguished,
        SymCryptEcpointGenericSetRandom,
        SymCryptShortWeierstrassIsEqual,
        SymCryptShortWeierstrassIsZero,
        SymCryptShortWeierstrassOnCurve,
        SymCryptShortWeierstrassAdd,
        SymCryptShortWeierstrassAddDiffNonZero,
        SymCryptShortWeierstrassDoubleSpecializedAm3,
        SymCryptShortWeierstrassNegate,
        SymCryptEcpointScalarMulFixedWindow,
        SymCryptEcpointMultiScalarMulWnafWithInterleaving,
        SymCryptShortWeierstrassFillScratchSpaces,
    },
    // Slack to make dispatch table size a power of 2
    {NULL,},
    {NULL,},
    {NULL,},
};

#define SYMCRYPT_ECURVE_DISPATCH_TABLE_SIZE (sizeof( SymCryptEcurveDispatchTable ))

// Ensure the table size is a power of 2
C_ASSERT( (SYMCRYPT_ECURVE_DISPATCH_TABLE_SIZE & (SYMCRYPT_ECURVE_DISPATCH_TABLE_SIZE - 1)) == 0 );

// For now the ECurve type encodes the index into this dispatch table, so we just mask by the size of the table
//
// We could instead encode the absolute offset into the table in the type field (similar to the Modulus dispatch table),
// and this mask would be multiplied by SYMCRYPT_ECURVE_FUNCTIONS_SIZE
#define SYMCRYPT_ECURVE_DISPATCH_TABLE_MASK ((SYMCRYPT_ECURVE_DISPATCH_TABLE_SIZE / SYMCRYPT_ECURVE_FUNCTIONS_SIZE)-1)

// We mask to constrain the unpredictable behaviour in the case of memory corruption; we do not want to interpret some data
// beyond the end of the dispatch table as function pointers
#define SYMCRYPT_ECURVE_CALL(v) (SymCryptEcurveDispatchTable[SYMCRYPT_FORCE_READ32(&(v)->type) & SYMCRYPT_ECURVE_DISPATCH_TABLE_MASK]).

// We read the curve's internal type with a 32b read so it must be 4 bytes large
C_ASSERT(sizeof(((PCSYMCRYPT_ECURVE)0)->type) == 4);

// Main functions
SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptEcpointSetZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ECURVE_CALL( pCurve ) setZeroFunc( pCurve, poDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptEcpointSetDistinguishedPoint(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ECURVE_CALL( pCurve ) setDistinguishedFunc( pCurve, poDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptEcpointSetRandom(
    _In_    PCSYMCRYPT_ECURVE       pCurve,
    _Out_   PSYMCRYPT_INT           piScalar,
    _Out_   PSYMCRYPT_ECPOINT       poDst,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE                   pbScratch,
            SIZE_T                  cbScratch )
{
    SYMCRYPT_ECURVE_CALL( pCurve ) setRandomFunc( pCurve, piScalar, poDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
UINT32
SYMCRYPT_CALL
SymCryptEcpointIsEqual(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
            UINT32              flags,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    return SYMCRYPT_ECURVE_CALL( pCurve ) isEqualFunc( pCurve, poSrc1, poSrc2, flags, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
UINT32
SYMCRYPT_CALL
SymCryptEcpointIsZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    return SYMCRYPT_ECURVE_CALL( pCurve ) isZeroFunc( pCurve, poSrc, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
UINT32
SYMCRYPT_CALL
SymCryptEcpointOnCurve(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    return SYMCRYPT_ECURVE_CALL( pCurve ) onCurveFunc( pCurve, poSrc, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptEcpointAdd(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ECURVE_CALL( pCurve ) addFunc( pCurve, poSrc1, poSrc2, poDst, flags, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptEcpointAddDiffNonZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ECURVE_CALL( pCurve ) addDiffFunc( pCurve, poSrc1, poSrc2, poDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptEcpointDouble(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ECURVE_CALL( pCurve ) doubleFunc( pCurve, poSrc, poDst, flags, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptEcpointNegate(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Inout_ PSYMCRYPT_ECPOINT   poSrc,
            UINT32              mask,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ECURVE_CALL( pCurve ) negateFunc( pCurve, poSrc, mask, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcpointScalarMul(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_INT      piScalar,
    _In_opt_
            PCSYMCRYPT_ECPOINT  poSrc,
            UINT32              flags,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    return SYMCRYPT_ECURVE_CALL( pCurve ) scalarMulFunc( pCurve, piScalar, poSrc, flags, poDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcpointMultiScalarMul(
    _In_    PCSYMCRYPT_ECURVE       pCurve,
    _In_    PCSYMCRYPT_INT *        piSrcScalarArray,
    _In_    PCSYMCRYPT_ECPOINT *    poSrcEcpointArray,
            UINT32                  nPoints,
            UINT32                  flags,
    _Out_   PSYMCRYPT_ECPOINT       poDst,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE                   pbScratch,
            SIZE_T                  cbScratch )
{
    return SYMCRYPT_ECURVE_CALL( pCurve ) multiScalarMulFunc( pCurve, piSrcScalarArray, poSrcEcpointArray, nPoints, flags, poDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptEcurveFillScratchSpaces(
    _Inout_    PSYMCRYPT_ECURVE pCurve )
{
    SYMCRYPT_ECURVE_CALL( pCurve ) fillScratchSpacesFunc( pCurve );
}
