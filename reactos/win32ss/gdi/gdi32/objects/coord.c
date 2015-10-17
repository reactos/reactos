/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            win32ss/gdi/gdi32/objects/coord.c
 * PURPOSE:         Functions for coordinate transformation
 * PROGRAMMER:
 */
#include <precomp.h>

/* Currently we use a MATRIX inside the DC_ATTR containing the
   coordinate transformations, while we deal with XFORM structures
   internally. If we move all coordinate transformation to gdi32,
   we might as well have an XFORM structure in the DC_ATTR. */
void
MatrixToXForm(XFORM *pxform, const MATRIX *pmx)
{
    XFORML *pxforml = (XFORML*)pxform;
    pxforml->eM11 = FOtoF(&pmx->efM11);
    pxforml->eM12 = FOtoF(&pmx->efM12);
    pxforml->eM21 = FOtoF(&pmx->efM21);
    pxforml->eM22 = FOtoF(&pmx->efM22);
    pxforml->eDx = FOtoF(&pmx->efDx);
    pxforml->eDy = FOtoF(&pmx->efDy);
}

void
GdiTransformPoints2(
    _In_ XFORM *pxform,
    _Out_writes_(nCount) PPOINT pptOut,
    _In_reads_(nCount) PPOINT pptIn,
    _In_ ULONG nCount)
{
    ULONG i;
    FLOAT x, y;

    for (i = 0; i < nCount; i++)
    {
        x = pptIn[i].x * pxform->eM11 + pptIn[i].y * pxform->eM12 + pxform->eDx;
        pptOut[i].x = _lrintf(x);
        y = pptIn[i].x * pxform->eM21 + pptIn[i].y * pxform->eM22 + pxform->eDy;
        pptOut[i].y = _lrintf(y);
    }
}

FORCEINLINE
void
GdiTransformPoints(
    _In_ MATRIX *pmx,
    _Out_writes_(nCount) PPOINT pptOut,
    _In_reads_(nCount) PPOINT pptIn,
    _In_ ULONG nCount)
{
    XFORM xform;

    MatrixToXForm(&xform, pmx);
    GdiTransformPoints2(&xform, pptOut, pptIn, nCount);
}

#define MAX_OFFSET 4294967041.0
#define _fmul(x,y) (((x) == 0) ? 0 : (x) * (y))

BOOL
WINAPI
CombineTransform(
    _Out_ LPXFORM pxfResult,
    _In_ const XFORM *pxf1,
    _In_ const XFORM *pxf2)
{
    XFORM xformTmp;

    /* Check paramters */
    if (!pxfResult || !pxf1 || !pxf2) return FALSE;

    /* Do matrix multiplication, start with scaling elements */
    xformTmp.eM11 = (pxf1->eM11 * pxf2->eM11) + (pxf1->eM12 * pxf2->eM21);
    xformTmp.eM22 = (pxf1->eM21 * pxf2->eM12) + (pxf1->eM22 * pxf2->eM22);

    /* Calculate shear/rotate elements only of they are present */
    if ((pxf1->eM12 != 0.) || (pxf1->eM21 != 0.) ||
        (pxf2->eM12 != 0.) || (pxf2->eM21 != 0.))
    {
        xformTmp.eM12 = (pxf1->eM11 * pxf2->eM12) + (pxf1->eM12 * pxf2->eM22);
        xformTmp.eM21 = (pxf1->eM21 * pxf2->eM11) + (pxf1->eM22 * pxf2->eM21);
    }
    else
    {
        xformTmp.eM12 = 0.;
        xformTmp.eM21 = 0.;
    }

    /* Calculate the offset */
    xformTmp.eDx = _fmul(pxf1->eDx, pxf2->eM11) + _fmul(pxf1->eDy, pxf2->eM21) + pxf2->eDx;
    xformTmp.eDy = _fmul(pxf1->eDx, pxf2->eM12) + _fmul(pxf1->eDy, pxf2->eM22) + pxf2->eDy;

    /* Check for invalid offset ranges */
    if ((xformTmp.eDx > MAX_OFFSET) || (xformTmp.eDx < -MAX_OFFSET) ||
        (xformTmp.eDy > MAX_OFFSET) || (xformTmp.eDy < -MAX_OFFSET))
    {
        return FALSE;
    }

    /* All is ok, return the calculated values */
    *pxfResult = xformTmp;
    return TRUE;
}


/*
 * @implemented
 *
 */
int
WINAPI
GetMapMode(
    _In_ HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Return the map mode */
    return pdcattr->iMapMode;
}

/*
 * @implemented
 */
INT
WINAPI
SetMapMode(
    _In_ HDC hdc,
    _In_ INT iMode)
{
    PDC_ATTR pdcattr;

    /* Handle METADC16 here, since we don't have a DCATTR. */
    if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE) \
    {
        return GetAndSetDCDWord(hdc, GdiGetSetMapMode, iMode, 0, 0, 0 );
    }

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Force change if Isotropic is set for recompute. */
    if ((iMode != pdcattr->iMapMode) || (iMode == MM_ISOTROPIC))
    {
        pdcattr->ulDirty_ &= ~SLOW_WIDTHS;
        return GetAndSetDCDWord(hdc, GdiGetSetMapMode, iMode, 0, 0, 0 );
    }

    return pdcattr->iMapMode;
}


BOOL
WINAPI
DPtoLP(
    _In_ HDC hdc,
    _Inout_updates_(nCount) LPPOINT lpPoints,
    _In_ INT nCount)
{
#if 0
    INT i;
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (!pdcattr)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pdcattr->flXform & ANY_XFORM_CHANGES)
    {
        GdiFixupTransforms(pdcattr);
    }

    // FIXME: can this fail on Windows?
    GdiTransformPoints(&pdcattr->mxDeviceToWorld, lpPoints, lpPoints, nCount);

    return TRUE;
#endif
    return NtGdiTransformPoints(hdc, lpPoints, lpPoints, nCount, GdiDpToLp);
}

BOOL
WINAPI
LPtoDP(
    _In_ HDC hdc,
    _Inout_updates_(nCount) LPPOINT lpPoints,
    _In_ INT nCount)
{
#if 0
    INT i;
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (!pdcattr)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pdcattr->flXform & ANY_XFORM_CHANGES)
    {
        GdiFixupTransforms(pdcattr);
    }

    // FIXME: can this fail on Windows?
    GdiTransformPoints(&pdcattr->mxWorldToDevice, lpPoints, lpPoints, nCount);

    return TRUE;
#endif
    return NtGdiTransformPoints(hdc, lpPoints, lpPoints, nCount, GdiLpToDp);
}

/*
 * @implemented
 *
 */
BOOL
WINAPI
GetCurrentPositionEx(
    _In_ HDC hdc,
    _Out_ LPPOINT lpPoint)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if ((pdcattr == NULL) || (lpPoint == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pdcattr->ulDirty_ & DIRTY_PTLCURRENT) // have a hit!
    {
        lpPoint->x = pdcattr->ptfxCurrent.x;
        lpPoint->y = pdcattr->ptfxCurrent.y;
        DPtoLP(hdc, lpPoint, 1);          // reconvert back.
        pdcattr->ptlCurrent.x = lpPoint->x; // save it
        pdcattr->ptlCurrent.y = lpPoint->y;
        pdcattr->ulDirty_ &= ~DIRTY_PTLCURRENT; // clear bit
    }
    else
    {
        lpPoint->x = pdcattr->ptlCurrent.x;
        lpPoint->y = pdcattr->ptlCurrent.y;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetWorldTransform(
    _In_ HDC hdc,
    _Out_ LPXFORM pxform)
{
#if 0
    PDC_ATTR pdcattr;

    pdcattr = GdiGetDcAttr(hdc);
    if (!pdcattr)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pdcattr->flXform & ANY_XFORM_INVALID)
    {
        GdiFixupTransforms(pdcattr);
    }

    MatrixToXForm(pxform, &pdcattr->mxWorldToDevice);
#endif
    return NtGdiGetTransform(hdc, GdiWorldSpaceToPageSpace, pxform);
}


BOOL
WINAPI
SetWorldTransform(
    _In_ HDC hdc,
    _Out_ CONST XFORM *pxform)
{
    /* FIXME  shall we add undoc #define MWT_SETXFORM 4 ?? */
    return ModifyWorldTransform(hdc, pxform, MWT_MAX+1);
}


BOOL
WINAPI
ModifyWorldTransform(
    _In_ HDC hdc,
    _In_opt_ CONST XFORM *pxform,
    _In_ DWORD dwMode)
{
    PDC_ATTR pdcattr;

    if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE)
        return FALSE;

    HANDLE_METADC(BOOL, ModifyWorldTransform, FALSE, hdc, pxform, dwMode);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Check that graphics mode is GM_ADVANCED */
    if (pdcattr->iGraphicsMode != GM_ADVANCED)
        return FALSE;

    /* Call win32k to do the work */
    return NtGdiModifyWorldTransform(hdc, (LPXFORM)pxform, dwMode);
}

BOOL
WINAPI
GetViewportExtEx(
    _In_ HDC hdc,
    _Out_ LPSIZE lpSize)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Do not set LastError here! */
        return FALSE;
    }

    /* Check if we need to update values */
    if ((pdcattr->flXform & PAGE_EXTENTS_CHANGED) &&
        (pdcattr->iMapMode == MM_ISOTROPIC))
    {
        /* Call win32k to do the work */
        return NtGdiGetDCPoint(hdc, GdiGetViewPortExt, (PPOINTL)lpSize);
    }

    /* Nothing to calculate, return the current extension */
    lpSize->cx = pdcattr->szlViewportExt.cx;
    lpSize->cy = pdcattr->szlViewportExt.cy;

    return TRUE;
}


BOOL
WINAPI
GetViewportOrgEx(
    _In_ HDC hdc,
    _Out_ LPPOINT lpPoint)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Do not set LastError here! */
        return FALSE;
    }

    /* Get the current viewport org */
    lpPoint->x = pdcattr->ptlViewportOrg.x;
    lpPoint->y = pdcattr->ptlViewportOrg.y;

    /* Handle right-to-left layout */
    if (pdcattr->dwLayout & LAYOUT_RTL)
        lpPoint->x = -lpPoint->x;

    return TRUE;
}


BOOL
WINAPI
GetWindowExtEx(
    _In_ HDC hdc,
    _Out_ LPSIZE lpSize)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Do not set LastError here! */
        return FALSE;
    }

    /* Get the current window extension */
    lpSize->cx = pdcattr->szlWindowExt.cx;
    lpSize->cy = pdcattr->szlWindowExt.cy;

    /* Handle right-to-left layout */
    if (pdcattr->dwLayout & LAYOUT_RTL)
        lpSize->cx = -lpSize->cx;

    return TRUE;
}


BOOL
WINAPI
GetWindowOrgEx(
    _In_ HDC hdc,
    _Out_ LPPOINT lpPoint)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Do not set LastError here! */
        return FALSE;
    }

    /* Get the current window origin */
    lpPoint->x = pdcattr->ptlWindowOrg.x;
    lpPoint->y = pdcattr->ptlWindowOrg.y;

    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetViewportExtEx(
    _In_ HDC hdc,
    _In_ int nXExtent,
    _In_ int nYExtent,
    _Out_opt_ LPSIZE lpSize)
{
    PDC_ATTR pdcattr;

    HANDLE_METADC(BOOL, SetViewportExtEx, FALSE, hdc, nXExtent, nYExtent, lpSize);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Check if the caller wants the old extension */
    if (lpSize)
    {
        /* Return the current viewport extension */
        lpSize->cx = pdcattr->szlViewportExt.cx;
        lpSize->cy = pdcattr->szlViewportExt.cy;
    }

    /* Check for trivial case */
    if ((pdcattr->szlViewportExt.cx == nXExtent) &&
        (pdcattr->szlViewportExt.cy == nYExtent))
        return TRUE;

    /* Only change viewport extension if we are in iso or aniso mode */
    if ((pdcattr->iMapMode == MM_ISOTROPIC) ||
        (pdcattr->iMapMode == MM_ANISOTROPIC))
    {
        if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
        {
            if (pdcattr->ulDirty_ & DC_FONTTEXT_DIRTY)
            {
                NtGdiFlush(); // Sync up pdcattr from Kernel space.
                pdcattr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
            }
        }

        /* Set the new viewport extension */
        pdcattr->szlViewportExt.cx = nXExtent;
        pdcattr->szlViewportExt.cy = nYExtent;

        /* Handle right-to-left layout */
        if (pdcattr->dwLayout & LAYOUT_RTL)
            NtGdiMirrorWindowOrg(hdc);

        /* Update xform flags */
        pdcattr->flXform |= (PAGE_EXTENTS_CHANGED|INVALIDATE_ATTRIBUTES|DEVICE_TO_WORLD_INVALID);
    }

    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetWindowOrgEx(
    _In_ HDC hdc,
    _In_ int X,
    _In_ int Y,
    _Out_opt_ LPPOINT lpPoint)
{
    PDC_ATTR pdcattr;

    HANDLE_METADC(BOOL, SetWindowOrgEx, FALSE, hdc, X, Y, lpPoint);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Do not set LastError here! */
        return FALSE;
    }
#if 0
    if (lpPoint)
    {
        lpPoint->x = pdcattr->ptlWindowOrg.x;
        lpPoint->y = pdcattr->ptlWindowOrg.y;
    }

    if ((pdcattr->ptlWindowOrg.x == X) && (pdcattr->ptlWindowOrg.y == Y))
        return TRUE;

    if (NtCurrentTeb()->GdiTebBatch.HDC == (ULONG)hdc)
    {
        if (pdcattr->ulDirty_ & DC_FONTTEXT_DIRTY)
        {
            NtGdiFlush(); // Sync up pdcattr from Kernel space.
            pdcattr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
        }
    }

    pdcattr->ptlWindowOrg.x = X;
    pdcattr->lWindowOrgx    = X;
    pdcattr->ptlWindowOrg.y = Y;
    if (pdcattr->dwLayout & LAYOUT_RTL) NtGdiMirrorWindowOrg(hdc);
    pdcattr->flXform |= (PAGE_XLATE_CHANGED|DEVICE_TO_WORLD_INVALID);
    return TRUE;
#endif
    return NtGdiSetWindowOrgEx(hdc, X, Y, lpPoint);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetWindowExtEx(
    _In_ HDC hdc,
    _In_ INT nXExtent,
    _In_ INT nYExtent,
    _Out_opt_ LPSIZE lpSize)
{
    PDC_ATTR pdcattr;

    HANDLE_METADC(BOOL, SetWindowExtEx, FALSE, hdc, nXExtent, nYExtent, lpSize);

    /* Get the DC attr */
    pdcattr = GdiGetDcAttr(hdc);
    if (!pdcattr)
    {
        /* Set the error value and return failure */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Check if the caller wants the old extension */
    if (lpSize)
    {
        /* Return the current window extension */
        lpSize->cx = pdcattr->szlWindowExt.cx;
        lpSize->cy = pdcattr->szlWindowExt.cy;

        /* Handle right-to-left layout */
        if (pdcattr->dwLayout & LAYOUT_RTL)
            lpSize->cx = -lpSize->cx;
    }

    if (pdcattr->dwLayout & LAYOUT_RTL)
    {
        NtGdiMirrorWindowOrg(hdc);
        pdcattr->flXform |= (PAGE_EXTENTS_CHANGED|INVALIDATE_ATTRIBUTES|DEVICE_TO_WORLD_INVALID);
    }
    else if ((pdcattr->iMapMode == MM_ISOTROPIC) ||
             (pdcattr->iMapMode == MM_ANISOTROPIC))
    {
        if ((pdcattr->szlWindowExt.cx == nXExtent) &&
            (pdcattr->szlWindowExt.cy == nYExtent))
            return TRUE;

        if ((!nXExtent) || (!nYExtent))
            return FALSE;

        if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
        {
            if (pdcattr->ulDirty_ & DC_FONTTEXT_DIRTY)
            {
                NtGdiFlush(); // Sync up Dc_Attr from Kernel space.
                pdcattr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
            }
        }

        pdcattr->szlWindowExt.cx = nXExtent;
        pdcattr->szlWindowExt.cy = nYExtent;
        if (pdcattr->dwLayout & LAYOUT_RTL)
            NtGdiMirrorWindowOrg(hdc);

        pdcattr->flXform |= (PAGE_EXTENTS_CHANGED|INVALIDATE_ATTRIBUTES|DEVICE_TO_WORLD_INVALID);
    }

    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetViewportOrgEx(
    _In_ HDC hdc,
    _In_ int X,
    _In_ int Y,
    _Out_opt_ LPPOINT lpPoint)
{
    PDC_ATTR pdcattr;

    HANDLE_METADC(BOOL, SetViewportOrgEx, FALSE, hdc, X, Y, lpPoint);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (!pdcattr)
    {
        /* Do not set LastError here! */
        return FALSE;
    }

#if 0
    if (lpPoint)
    {
        lpPoint->x = pdcattr->ptlViewportOrg.x;
        lpPoint->y = pdcattr->ptlViewportOrg.y;
        if (pdcattr->dwLayout & LAYOUT_RTL) lpPoint->x = -lpPoint->x;
    }
    pdcattr->flXform |= (PAGE_XLATE_CHANGED|DEVICE_TO_WORLD_INVALID);
    if (pdcattr->dwLayout & LAYOUT_RTL) X = -X;
    pdcattr->ptlViewportOrg.x = X;
    pdcattr->ptlViewportOrg.y = Y;
    return TRUE;
#endif
    return NtGdiSetViewportOrgEx(hdc,X,Y,lpPoint);
}

/*
 * @implemented
 */
BOOL
WINAPI
ScaleViewportExtEx(
    _In_ HDC hdc,
    _In_ INT xNum,
    _In_ INT xDenom,
    _In_ INT yNum,
    _In_ INT yDenom,
    _Out_ LPSIZE lpSize)
{
    HANDLE_METADC(BOOL, ScaleViewportExtEx, FALSE, hdc, xNum, xDenom, yNum, yDenom, lpSize);

    if (!GdiGetDcAttr(hdc))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return NtGdiScaleViewportExtEx(hdc, xNum, xDenom, yNum, yDenom, lpSize);
}

/*
 * @implemented
 */
BOOL
WINAPI
ScaleWindowExtEx(
    _In_ HDC hdc,
    _In_ INT xNum,
    _In_ INT xDenom,
    _In_ INT yNum,
    _In_ INT yDenom,
    _Out_ LPSIZE lpSize)
{
    HANDLE_METADC(BOOL, ScaleWindowExtEx, FALSE, hdc, xNum, xDenom, yNum, yDenom, lpSize);

    if (!GdiGetDcAttr(hdc))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return NtGdiScaleWindowExtEx(hdc, xNum, xDenom, yNum, yDenom, lpSize);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetLayout(
    _In_ HDC hdc)
{
    PDC_ATTR pdcattr;

    /* METADC16 is not supported in this API */
    if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE)
    {
        return GDI_ERROR;
    }

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (!pdcattr)
    {
        /* Set the error value and return failure */
        SetLastError(ERROR_INVALID_PARAMETER);
        return GDI_ERROR;
    }

    /* Return the layout */
    return pdcattr->dwLayout;
}


/*
 * @implemented
 */
DWORD
WINAPI
SetLayout(
    _In_ HDC hdc,
    _In_ DWORD dwLayout)
{
    HANDLE_METADC(DWORD, SetLayout, GDI_ERROR, hdc, dwLayout);

    if (!GdiGetDcAttr(hdc))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return GDI_ERROR;
    }

    return NtGdiSetLayout(hdc, -1, dwLayout);
}

/*
 * @implemented
 */
DWORD
WINAPI
SetLayoutWidth(
    _In_ HDC hdc,
    _In_ LONG wox,
    _In_ DWORD dwLayout)
{
    /* Only normal DCs are handled here */
    if (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_DC_TYPE)
    {
        return GDI_ERROR;
    }

    if (!GdiGetDcAttr(hdc))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return GDI_ERROR;
    }

    return NtGdiSetLayout(hdc, wox, dwLayout);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDCOrgEx(
    _In_ HDC hdc,
    _Out_ LPPOINT lpPoint)
{
    return NtGdiGetDCPoint(hdc, GdiGetDCOrg, (PPOINTL)lpPoint);
}


/*
 * @implemented
 */
LONG
WINAPI
GetDCOrg(
    _In_ HDC hdc)
{
    POINT pt;

    /* Call the new API */
    if (!GetDCOrgEx(hdc, &pt))
        return 0;

    /* Return the point in the old way */
    return(MAKELONG(pt.x, pt.y));
}


/*
 * @implemented
 *
 */
BOOL
WINAPI
OffsetViewportOrgEx(
    _In_ HDC hdc,
    _In_ int nXOffset,
    _In_ int nYOffset,
    _Out_opt_ LPPOINT lpPoint)
{
    //PDC_ATTR pdcattr;

    HANDLE_METADC(BOOL, OffsetViewportOrgEx, FALSE, hdc, nXOffset, nYOffset, lpPoint);
#if 0

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (!pdcattr)
    {
        /* Do not set LastError here! */
        return FALSE;
    }

    if (lpPoint)
    {
        *lpPoint = (POINT)pdcattr->ptlViewportOrg;
        if ( pdcattr->dwLayout & LAYOUT_RTL) lpPoint->x = -lpPoint->x;
    }

    if ( nXOffset || nYOffset != nXOffset )
    {
        if (NtCurrentTeb()->GdiTebBatch.HDC == (ULONG)hdc)
        {
            if (pdcattr->ulDirty_ & DC_MODE_DIRTY)
            {
                NtGdiFlush();
                pdcattr->ulDirty_ &= ~DC_MODE_DIRTY;
            }
        }

        pdcattr->flXform |= (PAGE_XLATE_CHANGED|DEVICE_TO_WORLD_INVALID);
        if (pdcattr->dwLayout & LAYOUT_RTL) nXOffset = -nXOffset;
        pdcattr->ptlViewportOrg.x += nXOffset;
        pdcattr->ptlViewportOrg.y += nYOffset;
    }
    return TRUE;
#endif
    return  NtGdiOffsetViewportOrgEx(hdc, nXOffset, nYOffset, lpPoint);
}

/*
 * @implemented
 *
 */
BOOL
WINAPI
OffsetWindowOrgEx(
    _In_ HDC hdc,
    _In_ int nXOffset,
    _In_ int nYOffset,
    _Out_opt_ LPPOINT lpPoint)
{
    //PDC_ATTR pdcattr;

    HANDLE_METADC(BOOL, OffsetWindowOrgEx, FALSE, hdc, nXOffset, nYOffset, lpPoint);

#if 0
    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (!pdcattr)
    {
        /* Do not set LastError here! */
        return FALSE;
    }

    if ( lpPoint )
    {
        *lpPoint   = (POINT)pdcattr->ptlWindowOrg;
        lpPoint->x = pdcattr->lWindowOrgx;
    }

    if ( nXOffset || nYOffset != nXOffset )
    {
        if (NtCurrentTeb()->GdiTebBatch.HDC == (ULONG)hdc)
        {
            if (pdcattr->ulDirty_ & DC_MODE_DIRTY)
            {
                NtGdiFlush();
                pdcattr->ulDirty_ &= ~DC_MODE_DIRTY;
            }
        }

        pdcattr->flXform |= (PAGE_XLATE_CHANGED|DEVICE_TO_WORLD_INVALID);
        pdcattr->ptlWindowOrg.x += nXOffset;
        pdcattr->ptlWindowOrg.y += nYOffset;
        pdcattr->lWindowOrgx += nXOffset;
    }
    return TRUE;
#endif
    return NtGdiOffsetWindowOrgEx(hdc, nXOffset, nYOffset, lpPoint);
}

