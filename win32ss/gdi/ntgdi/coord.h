#pragma once

/* Maximum extend of coordinate space */
#define MIN_COORD (INT_MIN / 16)
#define MAX_COORD (INT_MAX / 16)

/*
 * Applies matrix (which is made of FLOATOBJs) to the points array, which are made of integers.
 */
static
inline
BOOLEAN
INTERNAL_APPLY_MATRIX(PMATRIX matrix, LPPOINT points, UINT count)
{
    while (count--)
    {
        FLOATOBJ x, y;
        FLOATOBJ tmp;

        /* x = x * matrix->efM11 + y * matrix->efM21 + matrix->efDx; */
        FLOATOBJ_SetLong(&x, points[count].x);
        FLOATOBJ_Mul(&x, &matrix->efM11);
        tmp = matrix->efM21;
        FLOATOBJ_MulLong(&tmp, points[count].y);
        FLOATOBJ_Add(&x, &tmp);
        FLOATOBJ_Add(&x, &matrix->efDx);

        /* y = x * matrix->efM12 + y * matrix->efM22 + matrix->efDy; */
        FLOATOBJ_SetLong(&y, points[count].y);
        FLOATOBJ_Mul(&y, &matrix->efM22);
        tmp = matrix->efM12;
        FLOATOBJ_MulLong(&tmp, points[count].x);
        FLOATOBJ_Add(&y, &tmp);
        FLOATOBJ_Add(&y, &matrix->efDy);

        if (!FLOATOBJ_bConvertToLong(&x, &points[count].x))
            return FALSE;
        if (!FLOATOBJ_bConvertToLong(&y, &points[count].y))
            return FALSE;
    }
    return TRUE;
}

static
inline
BOOLEAN
INTERNAL_LPTODP(DC *dc, LPPOINT points, UINT count)
{
    return INTERNAL_APPLY_MATRIX(&dc->pdcattr->mxWorldToDevice, points, count);
}

static
inline
BOOLEAN
INTERNAL_DPTOLP(DC *dc, LPPOINT points, UINT count)
{
    return INTERNAL_APPLY_MATRIX(&dc->pdcattr->mxDeviceToWorld, points, count);
}

FORCEINLINE
void
XFormToMatrix(
    MATRIX *pmx,
    const XFORML *pxform)
{
    XFORMOBJ xo;
    XFORMOBJ_vInit(&xo, pmx);
    XFORMOBJ_iSetXform(&xo, pxform);
}

FORCEINLINE
void
MatrixToXForm(
    XFORML *pxform,
    const MATRIX *pmx)
{
    XFORMOBJ xo;
    XFORMOBJ_vInit(&xo, (MATRIX*)pmx);
    XFORMOBJ_iGetXform(&xo, pxform);
}

FORCEINLINE
void
InvertXform(
    XFORML *pxformDest,
    const XFORML *pxformSource)
{
    XFORMOBJ xo;
    MATRIX mx;

    XFORMOBJ_vInit(&xo, &mx);
    XFORMOBJ_iSetXform(&xo, pxformSource);
    XFORMOBJ_iInverse(&xo, &xo);
    XFORMOBJ_iGetXform(&xo, pxformDest);
}

VOID
FASTCALL
DC_vFixIsotropicMapping(PDC pdc);

VOID
FASTCALL
DC_vUpdateWorldToDevice(PDC pdc);

VOID
FASTCALL
DC_vUpdateDeviceToWorld(PDC pdc);

FORCEINLINE
PSIZEL
DC_pszlViewportExt(PDC pdc)
{
    PDC_ATTR pdcattr = pdc->pdcattr;

    /* Check if we need isotropic fixup */
    if ((pdcattr->flXform & PAGE_EXTENTS_CHANGED) &&
        (pdcattr->iMapMode == MM_ISOTROPIC))
    {
        /* Fixup viewport extension */
        DC_vFixIsotropicMapping(pdc);
    }

    return &pdcattr->szlViewportExt;
}

FORCEINLINE
PMATRIX
DC_pmxWorldToPage(PDC pdc)
{
    return &pdc->pdcattr->mxWorldToPage;
}

FORCEINLINE
PMATRIX
DC_pmxWorldToDevice(PDC pdc)
{
    /* Check if world or page xform was changed */
    if (pdc->pdcattr->flXform & (PAGE_XLATE_CHANGED|PAGE_EXTENTS_CHANGED|WORLD_XFORM_CHANGED))
    {
        /* Update the world-to-device xform */
         DC_vUpdateWorldToDevice(pdc);
    }

    return &pdc->pdcattr->mxWorldToDevice;
}

FORCEINLINE
PMATRIX
DC_pmxDeviceToWorld(PDC pdc)
{
    /* Check if the device-to-world xform is invalid */
    if (pdc->pdcattr->flXform & DEVICE_TO_WORLD_INVALID)
    {
        /* Update the world-to-device xform */
         DC_vUpdateDeviceToWorld(pdc);
    }

    return &pdc->pdcattr->mxDeviceToWorld;
}

BOOL
NTAPI
GreModifyWorldTransform(
    PDC pdc,
    const XFORML *pXForm,
    DWORD dwMode);

VOID FASTCALL IntMirrorWindowOrg(PDC);
int APIENTRY IntGdiSetMapMode(PDC, int);
BOOL FASTCALL GreLPtoDP(HDC, LPPOINT, INT);
BOOL FASTCALL GreDPtoLP(HDC, LPPOINT, INT);
BOOL APIENTRY GreGetDCPoint(HDC,UINT,PPOINTL);
BOOL WINAPI GreGetWindowExtEx( _In_ HDC hdc, _Out_ LPSIZE lpSize);
BOOL WINAPI GreGetViewportExtEx( _In_ HDC hdc, _Out_ LPSIZE lpSize);
BOOL FASTCALL GreSetViewportOrgEx(HDC,int,int,LPPOINT);
BOOL WINAPI GreGetDCOrgEx(_In_ HDC, _Out_ PPOINTL, _Out_ PRECTL);
BOOL WINAPI GreSetDCOrg(_In_  HDC, _In_ LONG, _In_ LONG, _In_opt_ PRECTL);

static
inline
BOOLEAN
IntLPtoDP(DC* pdc, PPOINTL ppt, UINT count)
{
    DC_vUpdateWorldToDevice(pdc);
    return INTERNAL_LPTODP(pdc, (LPPOINT)ppt, count);
}
#define CoordLPtoDP(pdc, ppt) INTERNAL_LPTODP(pdc, ppt, 1)

static
inline
BOOLEAN
IntDPtoLP(DC* pdc, PPOINTL ppt, UINT count)
{
    DC_vUpdateDeviceToWorld(pdc);
    return INTERNAL_DPTOLP(pdc, (LPPOINT)ppt, count);
}
#define CoordDPtoLP(pdc, ppt) INTERNAL_DPTOLP(pdc, ppt, 1)

#define XForm2MatrixS(m, x) XFormToMatrix(m, (XFORML*)x)
#define MatrixS2XForm(x, m) MatrixToXForm((XFORML*)x, m)
