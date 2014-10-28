/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/gdi32/objects/coord.c
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
    XFORM *pxform,
    PPOINT pptOut,
    PPOINT pptIn,
    ULONG nCount)
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
    MATRIX *pmx,
    PPOINT pptOut,
    PPOINT pptIn,
    ULONG nCount)
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
    LPXFORM pxfResult,
    const XFORM *pxf1,
    const XFORM *pxf2)
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
GetMapMode(HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

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

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

#if 0
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetMapMode(hdc, iMode);
        else
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return 0;
        }
    }
#endif
    /* Force change if Isotropic is set for recompute. */
    if ((iMode != pdcattr->iMapMode) || (iMode == MM_ISOTROPIC))
    {
        pdcattr->ulDirty_ &= ~SLOW_WIDTHS;
        return GetAndSetDCDWord( hdc, GdiGetSetMapMode, iMode, 0, 0, 0 );
    }

    return pdcattr->iMapMode;
}


BOOL
WINAPI
DPtoLP(HDC hdc, LPPOINT lpPoints, INT nCount)
{
#if 0
    INT i;
    PDC_ATTR pdcattr;

    pdcattr = GdiGetDcAttr(hdc);
    if (!pdcattr)
    {
        SetLastError(ERROR_INVALID_HANDLE);
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
LPtoDP(HDC hdc, LPPOINT lpPoints, INT nCount)
{
#if 0
    INT i;
    PDC_ATTR pdcattr;

    pdcattr = GdiGetDcAttr(hdc);
    if (!pdcattr)
    {
        SetLastError(ERROR_INVALID_HANDLE);
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
GetCurrentPositionEx(HDC hdc,
                     LPPOINT lpPoint)
{
    PDC_ATTR Dc_Attr;

    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;

    if ( lpPoint )
    {
        if ( Dc_Attr->ulDirty_ & DIRTY_PTLCURRENT ) // have a hit!
        {
            lpPoint->x = Dc_Attr->ptfxCurrent.x;
            lpPoint->y = Dc_Attr->ptfxCurrent.y;
            DPtoLP ( hdc, lpPoint, 1);          // reconvert back.
            Dc_Attr->ptlCurrent.x = lpPoint->x; // save it
            Dc_Attr->ptlCurrent.y = lpPoint->y;
            Dc_Attr->ulDirty_ &= ~DIRTY_PTLCURRENT; // clear bit
        }
        else
        {
            lpPoint->x = Dc_Attr->ptlCurrent.x;
            lpPoint->y = Dc_Attr->ptlCurrent.y;
        }
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetWorldTransform(HDC hDC, LPXFORM lpXform)
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

    MatrixToXForm(lpXform, &pdcattr->mxWorldToDevice);
#endif
    return NtGdiGetTransform(hDC, GdiWorldSpaceToPageSpace, lpXform);
}


BOOL
WINAPI
SetWorldTransform( HDC hDC, CONST XFORM *Xform )
{
    /* FIXME  shall we add undoc #define MWT_SETXFORM 4 ?? */
    return ModifyWorldTransform( hDC, Xform, MWT_MAX+1);
}


BOOL
WINAPI
ModifyWorldTransform(
    HDC hDC,
    CONST XFORM *Xform,
    DWORD iMode
)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
            return FALSE;
        else
        {
            PLDC pLDC = GdiGetLDC(hDC);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                if (iMode ==  MWT_MAX+1)
                    if (!EMFDRV_SetWorldTransform( hDC, Xform) ) return FALSE;
                return EMFDRV_ModifyWorldTransform( hDC, Xform, iMode); // Ported from wine.
            }
            return FALSE;
        }
    }
#endif
    PDC_ATTR Dc_Attr;

    if (!GdiGetHandleUserData((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;

    /* Check that graphics mode is GM_ADVANCED */
    if ( Dc_Attr->iGraphicsMode != GM_ADVANCED ) return FALSE;

    return NtGdiModifyWorldTransform(hDC, (CONST LPXFORM) Xform, iMode);
}

BOOL
WINAPI
GetViewportExtEx(
    HDC hdc,
    LPSIZE lpSize
)
{
    PDC_ATTR Dc_Attr;

    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;

    if ((Dc_Attr->flXform & PAGE_EXTENTS_CHANGED) && (Dc_Attr->iMapMode == MM_ISOTROPIC))
        // Something was updated, go to kernel.
        return NtGdiGetDCPoint( hdc, GdiGetViewPortExt, (PPOINTL) lpSize );
    else
    {
        lpSize->cx = Dc_Attr->szlViewportExt.cx;
        lpSize->cy = Dc_Attr->szlViewportExt.cy;
    }
    return TRUE;
}


BOOL
WINAPI
GetViewportOrgEx(
    HDC hdc,
    LPPOINT lpPoint
)
{
    PDC_ATTR Dc_Attr;

    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;
    lpPoint->x = Dc_Attr->ptlViewportOrg.x;
    lpPoint->y = Dc_Attr->ptlViewportOrg.y;
    if (Dc_Attr->dwLayout & LAYOUT_RTL) lpPoint->x = -lpPoint->x;
    return TRUE;
    // return NtGdiGetDCPoint( hdc, GdiGetViewPortOrg, lpPoint );
}


BOOL
WINAPI
GetWindowExtEx(
    HDC hdc,
    LPSIZE lpSize
)
{
    PDC_ATTR Dc_Attr;

    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;
    lpSize->cx = Dc_Attr->szlWindowExt.cx;
    lpSize->cy = Dc_Attr->szlWindowExt.cy;
    if (Dc_Attr->dwLayout & LAYOUT_RTL) lpSize->cx = -lpSize->cx;
    return TRUE;
    // return NtGdiGetDCPoint( hdc, GdiGetWindowExt, (LPPOINT) lpSize );
}


BOOL
WINAPI
GetWindowOrgEx(
    HDC hdc,
    LPPOINT lpPoint
)
{
    PDC_ATTR Dc_Attr;

    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;
    lpPoint->x = Dc_Attr->ptlWindowOrg.x;
    lpPoint->y = Dc_Attr->ptlWindowOrg.y;
    return TRUE;
    //return NtGdiGetDCPoint( hdc, GdiGetWindowOrg, lpPoint );
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetViewportExtEx(HDC hdc,
                 int nXExtent,
                 int nYExtent,
                 LPSIZE lpSize)
{
    PDC_ATTR Dc_Attr;
#if 0
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetViewportExtEx();
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_SetViewportExtEx();
            }
        }
    }
#endif
    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr))
    {
        return FALSE;
    }

    if (lpSize)
    {
        lpSize->cx = Dc_Attr->szlViewportExt.cx;
        lpSize->cy = Dc_Attr->szlViewportExt.cy;
    }

    if ((Dc_Attr->szlViewportExt.cx == nXExtent) && (Dc_Attr->szlViewportExt.cy == nYExtent))
        return TRUE;

    if ((Dc_Attr->iMapMode == MM_ISOTROPIC) || (Dc_Attr->iMapMode == MM_ANISOTROPIC))
    {
        if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
        {
            if (Dc_Attr->ulDirty_ & DC_FONTTEXT_DIRTY)
            {
                NtGdiFlush(); // Sync up Dc_Attr from Kernel space.
                Dc_Attr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
            }
        }
        Dc_Attr->szlViewportExt.cx = nXExtent;
        Dc_Attr->szlViewportExt.cy = nYExtent;
        if (Dc_Attr->dwLayout & LAYOUT_RTL) NtGdiMirrorWindowOrg(hdc);
        Dc_Attr->flXform |= (PAGE_EXTENTS_CHANGED|INVALIDATE_ATTRIBUTES|DEVICE_TO_WORLD_INVALID);
    }
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetWindowOrgEx(HDC hdc,
               int X,
               int Y,
               LPPOINT lpPoint)
{
#if 0
    PDC_ATTR Dc_Attr;
#if 0
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetWindowOrgEx();
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_SetWindowOrgEx();
            }
        }
    }
#endif
    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;

    if (lpPoint)
    {
        lpPoint->x = Dc_Attr->ptlWindowOrg.x;
        lpPoint->y = Dc_Attr->ptlWindowOrg.y;
    }

    if ((Dc_Attr->ptlWindowOrg.x == X) && (Dc_Attr->ptlWindowOrg.y == Y))
        return TRUE;

    if (NtCurrentTeb()->GdiTebBatch.HDC == (ULONG)hdc)
    {
        if (Dc_Attr->ulDirty_ & DC_FONTTEXT_DIRTY)
        {
            NtGdiFlush(); // Sync up Dc_Attr from Kernel space.
            Dc_Attr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
        }
    }

    Dc_Attr->ptlWindowOrg.x = X;
    Dc_Attr->lWindowOrgx    = X;
    Dc_Attr->ptlWindowOrg.y = Y;
    if (Dc_Attr->dwLayout & LAYOUT_RTL) NtGdiMirrorWindowOrg(hdc);
    Dc_Attr->flXform |= (PAGE_XLATE_CHANGED|DEVICE_TO_WORLD_INVALID);
    return TRUE;
#endif
    return NtGdiSetWindowOrgEx(hdc,X,Y,lpPoint);
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
    ULONG ulType;

    /* Check what type of DC that is */
    ulType = GDI_HANDLE_GET_TYPE(hdc);
    switch (ulType)
    {
        case GDILoObjType_LO_DC_TYPE:
            /* Handle this in the path below */
            break;
#if 0// FIXME: we don't support this
        case GDILoObjType_LO_METADC16_TYPE:
            return MFDRV_SetWindowExtEx(hdc, nXExtent, nYExtent, lpSize);

        case GDILoObjType_LO_METAFILE_TYPE:
            return EMFDRV_SetWindowExtEx(hdc, nXExtent, nYExtent, lpSize);
#endif
        default:
            /* Other types are not allowed */
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
    }

    /* Get the DC attr */
    pdcattr = GdiGetDcAttr(hdc);
    if (!pdcattr)
    {
        /* Set the error value and return failure */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (lpSize)
    {
        lpSize->cx = pdcattr->szlWindowExt.cx;
        lpSize->cy = pdcattr->szlWindowExt.cy;
        if (pdcattr->dwLayout & LAYOUT_RTL) lpSize->cx = -lpSize->cx;
    }

    if (pdcattr->dwLayout & LAYOUT_RTL)
    {
        NtGdiMirrorWindowOrg(hdc);
        pdcattr->flXform |= (PAGE_EXTENTS_CHANGED|INVALIDATE_ATTRIBUTES|DEVICE_TO_WORLD_INVALID);
    }
    else if ((pdcattr->iMapMode == MM_ISOTROPIC) || (pdcattr->iMapMode == MM_ANISOTROPIC))
    {
        if ((pdcattr->szlWindowExt.cx == nXExtent) && (pdcattr->szlWindowExt.cy == nYExtent))
            return TRUE;

        if ((!nXExtent) || (!nYExtent)) return FALSE;

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
        if (pdcattr->dwLayout & LAYOUT_RTL) NtGdiMirrorWindowOrg(hdc);
        pdcattr->flXform |= (PAGE_EXTENTS_CHANGED|INVALIDATE_ATTRIBUTES|DEVICE_TO_WORLD_INVALID);
    }

    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetViewportOrgEx(HDC hdc,
                 int X,
                 int Y,
                 LPPOINT lpPoint)
{
#if 0
    PDC_ATTR Dc_Attr;
#if 0
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetViewportOrgEx();
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_SetViewportOrgEx();
            }
        }
    }
#endif
    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;

    if (lpPoint)
    {
        lpPoint->x = Dc_Attr->ptlViewportOrg.x;
        lpPoint->y = Dc_Attr->ptlViewportOrg.y;
        if (Dc_Attr->dwLayout & LAYOUT_RTL) lpPoint->x = -lpPoint->x;
    }
    Dc_Attr->flXform |= (PAGE_XLATE_CHANGED|DEVICE_TO_WORLD_INVALID);
    if (Dc_Attr->dwLayout & LAYOUT_RTL) X = -X;
    Dc_Attr->ptlViewportOrg.x = X;
    Dc_Attr->ptlViewportOrg.y = Y;
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
    HDC	a0,
    int	a1,
    int	a2,
    int	a3,
    int	a4,
    LPSIZE	a5
)
{
#if 0
    if (GDI_HANDLE_GET_TYPE(a0) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(a0) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_;
        else
        {
            PLDC pLDC = GdiGetLDC(a0);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_;
            }
        }
    }
#endif
    if (!GdiIsHandleValid((HGDIOBJ) a0) ||
            (GDI_HANDLE_GET_TYPE(a0) != GDI_OBJECT_TYPE_DC)) return FALSE;

    return NtGdiScaleViewportExtEx(a0, a1, a2, a3, a4, a5);
}

/*
 * @implemented
 */
BOOL
WINAPI
ScaleWindowExtEx(
    HDC	a0,
    int	a1,
    int	a2,
    int	a3,
    int	a4,
    LPSIZE	a5
)
{
#if 0
    if (GDI_HANDLE_GET_TYPE(a0) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(a0) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_;
        else
        {
            PLDC pLDC = GdiGetLDC(a0);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_;
            }
        }
    }
#endif
    if (!GdiIsHandleValid((HGDIOBJ) a0) ||
            (GDI_HANDLE_GET_TYPE(a0) != GDI_OBJECT_TYPE_DC)) return FALSE;

    return NtGdiScaleWindowExtEx(a0, a1, a2, a3, a4, a5);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetLayout(HDC hdc
         )
{
    PDC_ATTR Dc_Attr;
    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return GDI_ERROR;
    return Dc_Attr->dwLayout;
}


/*
 * @implemented
 */
DWORD
WINAPI
SetLayout(HDC hdc,
          DWORD dwLayout)
{
#if 0
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetLayout( hdc, dwLayout);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return 0;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_SetLayout( hdc, dwLayout);
            }
        }
    }
#endif
    if (!GdiIsHandleValid((HGDIOBJ) hdc) ||
            (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)) return GDI_ERROR;
    return NtGdiSetLayout( hdc, -1, dwLayout);
}

/*
 * @implemented
 */
DWORD
WINAPI
SetLayoutWidth(HDC hdc,LONG wox,DWORD dwLayout)
{
    if (!GdiIsHandleValid((HGDIOBJ) hdc) ||
            (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)) return GDI_ERROR;
    return NtGdiSetLayout( hdc, wox, dwLayout);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDCOrgEx(
    HDC hdc,
    LPPOINT lpPoint)
{
    return NtGdiGetDCPoint( hdc, GdiGetDCOrg, (PPOINTL)lpPoint );
}


/*
 * @implemented
 */
LONG
WINAPI
GetDCOrg(
    HDC hdc)
{
    // Officially obsolete by Microsoft
    POINT Pt;
    if (!GetDCOrgEx(hdc, &Pt))
        return 0;
    return(MAKELONG(Pt.x, Pt.y));
}


/*
 * @implemented
 *
 */
BOOL
WINAPI
OffsetViewportOrgEx(HDC hdc,
                    int nXOffset,
                    int nYOffset,
                    LPPOINT lpPoint)
{
#if 0
    PDC_ATTR Dc_Attr;
#if 0
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_OffsetViewportOrgEx(hdc, nXOffset, nYOffset, lpPoint);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_OffsetWindowOrgEx(hdc, nXOffset, nYOffset, lpPoint);
            }
        }
    }
#endif
    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;

    if ( lpPoint )
    {
        *lpPoint = (POINT)Dc_Attr->ptlViewportOrg;
        if ( Dc_Attr->dwLayout & LAYOUT_RTL) lpPoint->x = -lpPoint->x;
    }

    if ( nXOffset || nYOffset != nXOffset )
    {
        if (NtCurrentTeb()->GdiTebBatch.HDC == (ULONG)hdc)
        {
            if (Dc_Attr->ulDirty_ & DC_MODE_DIRTY)
            {
                NtGdiFlush();
                Dc_Attr->ulDirty_ &= ~DC_MODE_DIRTY;
            }
        }
        Dc_Attr->flXform |= (PAGE_XLATE_CHANGED|DEVICE_TO_WORLD_INVALID);
        if ( Dc_Attr->dwLayout & LAYOUT_RTL) nXOffset = -nXOffset;
        Dc_Attr->ptlViewportOrg.x += nXOffset;
        Dc_Attr->ptlViewportOrg.y += nYOffset;
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
OffsetWindowOrgEx(HDC hdc,
                  int nXOffset,
                  int nYOffset,
                  LPPOINT lpPoint)
{
#if 0
    PDC_ATTR Dc_Attr;
#if 0
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_OffsetWindowOrgEx(hdc, nXOffset, nYOffset, lpPoint);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_OffsetWindowOrgEx(hdc, nXOffset, nYOffset, lpPoint);
            }
        }
    }
#endif
    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;

    if ( lpPoint )
    {
        *lpPoint   = (POINT)Dc_Attr->ptlWindowOrg;
        lpPoint->x = Dc_Attr->lWindowOrgx;
    }

    if ( nXOffset || nYOffset != nXOffset )
    {
        if (NtCurrentTeb()->GdiTebBatch.HDC == (ULONG)hdc)
        {
            if (Dc_Attr->ulDirty_ & DC_MODE_DIRTY)
            {
                NtGdiFlush();
                Dc_Attr->ulDirty_ &= ~DC_MODE_DIRTY;
            }
        }
        Dc_Attr->flXform |= (PAGE_XLATE_CHANGED|DEVICE_TO_WORLD_INVALID);
        Dc_Attr->ptlWindowOrg.x += nXOffset;
        Dc_Attr->ptlWindowOrg.y += nYOffset;
        Dc_Attr->lWindowOrgx += nXOffset;
    }
    return TRUE;
#endif
    return NtGdiOffsetWindowOrgEx(hdc, nXOffset, nYOffset, lpPoint);
}

