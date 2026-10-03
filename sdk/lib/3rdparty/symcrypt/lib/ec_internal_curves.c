//
// ec_internal_curves.c   Internally allocated elliptic curves.
//
// These curves are lazy-initialized. Currently only used
// for composite algorithms to avoid per-key allocation overhead.
//

#include "precomp.h"

static PCSYMCRYPT_ECURVE rgpCachedCurves[SYMCRYPT_CACHED_ECURVE_ID_COUNT] = { 0 };

static
PCSYMCRYPT_ECURVE_PARAMS
SYMCRYPT_CALL
SymCryptGetCachedEcurveParams(
    SYMCRYPT_CACHED_ECURVE_ID   curveId )
{
    switch (curveId)
    {
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P256:
            return SymCryptEcurveParamsNistP256;
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P384:
            return SymCryptEcurveParamsNistP384;
        case SYMCRYPT_CACHED_ECURVE_ID_CURVE_25519:
            return SymCryptEcurveParamsCurve25519;
        default:
            return NULL;
    }
}

PCSYMCRYPT_ECURVE
SYMCRYPT_CALL
SymCryptGetCachedEcurve(
    SYMCRYPT_CACHED_ECURVE_ID   curveId )
{
    PCSYMCRYPT_ECURVE pCachedCurve = NULL;
    PSYMCRYPT_ECURVE pNewCurve = NULL;
    PSYMCRYPT_ECURVE pCurrCurve = NULL;
    PCSYMCRYPT_ECURVE_PARAMS pParams = NULL;

    if ( curveId < 0 || curveId >= SYMCRYPT_CACHED_ECURVE_ID_COUNT )
    {
        return NULL;
    }

    pCachedCurve = (PCSYMCRYPT_ECURVE) SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE( &rgpCachedCurves[curveId] );
    if ( pCachedCurve != NULL )
    {
        return pCachedCurve;
    }

    pParams = SymCryptGetCachedEcurveParams( curveId );
    if ( pParams == NULL )
    {
        return NULL;
    }

    pNewCurve = SymCryptEcurveAllocate( pParams, 0 );
    if ( pNewCurve == NULL )
    {
        return NULL;
    }

    pCurrCurve = SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE(
                    &rgpCachedCurves[curveId],
                    pNewCurve,
                    NULL);

    // Means the original curve was already filled
    // and that our new curve was not used. So we
    // free the new curve and return the existing one.
    if ( pCurrCurve != NULL )
    {
        SymCryptEcurveFree( pNewCurve );
        return pCurrCurve;
    }

    return pNewCurve;
}
