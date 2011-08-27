#pragma once

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

FORCEINLINE
void
DC_vXformDeviceToWorld(
    IN PDC pdc,
    IN ULONG cNumPoints,
    IN PPOINTL pptlDest,
    IN PPOINTL pptlSource)
{
    XFORMOBJ xo;

    XFORMOBJ_vInit(&xo, &pdc->dclevel.mxDeviceToWorld);
    XFORMOBJ_bApplyXform(&xo, XF_LTOL, cNumPoints, pptlDest, pptlSource);
}

FORCEINLINE
void
DC_vXformWorldToDevice(
    IN PDC pdc,
    IN ULONG cNumPoints,
    IN PPOINTL pptlDest,
    IN PPOINTL pptlSource)
{
    XFORMOBJ xo;

    XFORMOBJ_vInit(&xo, &pdc->dclevel.mxWorldToDevice);
    XFORMOBJ_bApplyXform(&xo, XF_LTOL, cNumPoints, pptlDest, pptlSource);
}

int APIENTRY IntGdiSetMapMode(PDC, int);

BOOL
FASTCALL
IntGdiModifyWorldTransform(PDC pDc,
                           CONST LPXFORM lpXForm,
                           DWORD Mode);

VOID FASTCALL IntMirrorWindowOrg(PDC);
void FASTCALL IntFixIsotropicMapping(PDC);
LONG FASTCALL IntCalcFillOrigin(PDC);
PPOINTL FASTCALL IntptlBrushOrigin(PDC pdc,LONG,LONG);
