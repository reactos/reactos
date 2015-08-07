#pragma once

/* Maximum extend of coordinate space */
#define MIN_COORD (INT_MIN / 16)
#define MAX_COORD (INT_MAX / 16)

#define IntLPtoDP(pdc, ppt, count) DC_vXformWorldToDevice(pdc, count, (PPOINTL)(ppt), (PPOINTL)(ppt));
#define CoordLPtoDP(pdc, ppt) DC_vXformWorldToDevice(pdc, 1,  (PPOINTL)(ppt), (PPOINTL)(ppt));
#define IntDPtoLP(pdc, ppt, count) DC_vXformDeviceToWorld(pdc, count, (PPOINTL)(ppt), (PPOINTL)(ppt));
#define CoordDPtoLP(pdc, ppt) DC_vXformDeviceToWorld(pdc, 1, (PPOINTL)(ppt), (PPOINTL)(ppt));

#define XForm2MatrixS(m, x) XFormToMatrix(m, (XFORML*)x)
#define MatrixS2XForm(x, m) MatrixToXForm((XFORML*)x, m)

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

FORCEINLINE
VOID
DC_vXformDeviceToWorld(
    IN PDC pdc,
    IN ULONG cNumPoints,
    IN PPOINTL pptlDest,
    IN PPOINTL pptlSource)
{
    XFORMOBJ xo;
    PMATRIX pmx;

    pmx = DC_pmxDeviceToWorld(pdc);
    XFORMOBJ_vInit(&xo, pmx);
    XFORMOBJ_bApplyXform(&xo, XF_LTOL, cNumPoints, pptlDest, pptlSource);
}

FORCEINLINE
VOID
DC_vXformWorldToDevice(
    IN PDC pdc,
    IN ULONG cNumPoints,
    IN PPOINTL pptlDest,
    IN PPOINTL pptlSource)
{
    XFORMOBJ xo;
    PMATRIX pmx;

    pmx = DC_pmxWorldToDevice(pdc);
    XFORMOBJ_vInit(&xo, pmx);
    XFORMOBJ_bApplyXform(&xo, XF_LTOL, cNumPoints, pptlDest, pptlSource);
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
