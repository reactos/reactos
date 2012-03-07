/*
 * COPYRIGHT:        GNU GPL, See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Coordinate systems
 * FILE:             subsystems/win32/win32k/objects/coord.c
 * PROGRAMER:        Unknown
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

void FASTCALL
IntFixIsotropicMapping(PDC pdc)
{
    PDC_ATTR pdcattr;
    LONG fx, fy, s;

    /* Get a pointer to the DC_ATTR */
    pdcattr = pdc->pdcattr;

    /* Check if all values are valid */
    if (pdcattr->szlWindowExt.cx == 0 || pdcattr->szlWindowExt.cy == 0 ||
        pdcattr->szlViewportExt.cx == 0 || pdcattr->szlViewportExt.cy == 0)
    {
        /* Don't recalculate */
        return;
    }

    fx = abs(pdcattr->szlWindowExt.cx * pdcattr->szlViewportExt.cy);
    fy = abs(pdcattr->szlWindowExt.cy * pdcattr->szlViewportExt.cx);

    if (fy > fx)
    {
        s = pdcattr->szlWindowExt.cy * pdcattr->szlViewportExt.cx > 0 ? 1 : -1;
        pdcattr->szlViewportExt.cx = s * fx / pdcattr->szlWindowExt.cy;
    }
    else if (fx > fy)
    {
        s = pdcattr->szlWindowExt.cx * pdcattr->szlViewportExt.cy > 0 ? 1 : -1;
        pdcattr->szlViewportExt.cy = s * fy / pdcattr->szlWindowExt.cx;
    }
}

// FIXME: Don't use floating point in the kernel!
void IntWindowToViewPort(PDC_ATTR pdcattr, LPXFORM xformWnd2Vport)
{
    FLOAT scaleX, scaleY;

    scaleX = (pdcattr->szlWindowExt.cx ? (FLOAT)pdcattr->szlViewportExt.cx / (FLOAT)pdcattr->szlWindowExt.cx : 0.0f);
    scaleY = (pdcattr->szlWindowExt.cy ? (FLOAT)pdcattr->szlViewportExt.cy / (FLOAT)pdcattr->szlWindowExt.cy : 0.0f);
    xformWnd2Vport->eM11 = scaleX;
    xformWnd2Vport->eM12 = 0.0;
    xformWnd2Vport->eM21 = 0.0;
    xformWnd2Vport->eM22 = scaleY;
    xformWnd2Vport->eDx  = (FLOAT)pdcattr->ptlViewportOrg.x - scaleX * (FLOAT)pdcattr->ptlWindowOrg.x;
    xformWnd2Vport->eDy  = (FLOAT)pdcattr->ptlViewportOrg.y - scaleY * (FLOAT)pdcattr->ptlWindowOrg.y;
}

// FIXME: Use XFORMOBJECT!
VOID FASTCALL
DC_UpdateXforms(PDC dc)
{
    XFORM  xformWnd2Vport;
    PDC_ATTR pdcattr = dc->pdcattr;
    XFORM xformWorld2Vport, xformWorld2Wnd, xformVport2World;

    /* Construct a transformation to do the window-to-viewport conversion */
    IntWindowToViewPort(pdcattr, &xformWnd2Vport);

    /* Combine with the world transformation */
    MatrixS2XForm(&xformWorld2Vport, &dc->dclevel.mxWorldToDevice);
    MatrixS2XForm(&xformWorld2Wnd, &dc->dclevel.mxWorldToPage);
    IntGdiCombineTransform(&xformWorld2Vport, &xformWorld2Wnd, &xformWnd2Vport);

    /* Create inverse of world-to-viewport transformation */
    MatrixS2XForm(&xformVport2World, &dc->dclevel.mxDeviceToWorld);
    if (DC_InvertXform(&xformWorld2Vport, &xformVport2World))
    {
        pdcattr->flXform &= ~DEVICE_TO_WORLD_INVALID;
    }
    else
    {
        pdcattr->flXform |= DEVICE_TO_WORLD_INVALID;
    }

    /* Update transformation matrices */
    XForm2MatrixS(&dc->dclevel.mxWorldToDevice, &xformWorld2Vport);
    XForm2MatrixS(&dc->dclevel.mxDeviceToWorld, &xformVport2World);
}

VOID
FASTCALL
DC_vUpdateViewportExt(PDC pdc)
{
    PDC_ATTR pdcattr;

    /* Get a pointer to the dc attribute */
    pdcattr = pdc->pdcattr;

    /* Check if we need to recalculate */
    if (pdcattr->flXform & PAGE_EXTENTS_CHANGED)
    {
        /* Check if we need to do isotropic fixup */
        if (pdcattr->iMapMode == MM_ISOTROPIC)
        {
            IntFixIsotropicMapping(pdc);
        }

        /* Update xforms, CHECKME: really done here? */
        DC_UpdateXforms(pdc);
    }
}

// FIXME: Don't use floating point in the kernel! use XFORMOBJ function
BOOL FASTCALL
IntGdiCombineTransform(
    LPXFORM XFormResult,
    LPXFORM xform1,
    LPXFORM xform2)
{
    XFORM xformTemp;

    /* Check for illegal parameters */
    if (!XFormResult || !xform1 || !xform2)
    {
        return  FALSE;
    }

    /* Create the result in a temporary XFORM, since xformResult may be
     * equal to xform1 or xform2 */
    xformTemp.eM11 = xform1->eM11 * xform2->eM11 + xform1->eM12 * xform2->eM21;
    xformTemp.eM12 = xform1->eM11 * xform2->eM12 + xform1->eM12 * xform2->eM22;
    xformTemp.eM21 = xform1->eM21 * xform2->eM11 + xform1->eM22 * xform2->eM21;
    xformTemp.eM22 = xform1->eM21 * xform2->eM12 + xform1->eM22 * xform2->eM22;
    xformTemp.eDx  = xform1->eDx  * xform2->eM11 + xform1->eDy  * xform2->eM21 + xform2->eDx;
    xformTemp.eDy  = xform1->eDx  * xform2->eM12 + xform1->eDy  * xform2->eM22 + xform2->eDy;
    *XFormResult = xformTemp;

    return TRUE;
}

// FIXME: Should be XFORML and use XFORMOBJ functions
BOOL
APIENTRY
NtGdiCombineTransform(
    LPXFORM UnsafeXFormResult,
    LPXFORM Unsafexform1,
    LPXFORM Unsafexform2)
{
    BOOL Ret;

    _SEH2_TRY
    {
        ProbeForWrite(UnsafeXFormResult, sizeof(XFORM), 1);
        ProbeForRead(Unsafexform1, sizeof(XFORM), 1);
        ProbeForRead(Unsafexform2, sizeof(XFORM), 1);
        Ret = IntGdiCombineTransform(UnsafeXFormResult,
                                     Unsafexform1,
                                     Unsafexform2);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = FALSE;
    }
    _SEH2_END;

    return Ret;
}

// FIXME: Should be XFORML and use XFORMOBJ functions directly
BOOL
APIENTRY
NtGdiGetTransform(
    HDC  hDC,
    DWORD iXform,
    LPXFORM  XForm)
{
    PDC pdc;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!XForm)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    _SEH2_TRY
    {
        ProbeForWrite(XForm, sizeof(XFORM), 1);
        switch (iXform)
        {
            case GdiWorldSpaceToPageSpace:
                MatrixS2XForm(XForm, &pdc->dclevel.mxWorldToPage);
                break;

            case GdiWorldSpaceToDeviceSpace:
                MatrixS2XForm(XForm, &pdc->dclevel.mxWorldToDevice);
                break;

            case GdiPageSpaceToDeviceSpace:
                IntWindowToViewPort(pdc->pdcattr, XForm);
                break;

            case GdiDeviceSpaceToWorldSpace:
                MatrixS2XForm(XForm, &pdc->dclevel.mxDeviceToWorld);
                break;

            default:
                DPRINT1("Unknown transform %lu\n", iXform);
                Status = STATUS_INVALID_PARAMETER;
                break;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    DC_UnlockDc(pdc);
    return NT_SUCCESS(Status);
}


/*!
 * Converts points from logical coordinates into device coordinates.
 * Conversion depends on the mapping mode,
 * world transfrom, viewport origin settings for the given device context.
 * \param	hDC		device context.
 * \param	Points	an array of POINT structures (in/out).
 * \param	Count	number of elements in the array of POINT structures.
 * \return  TRUE if success, FALSE otherwise.
*/
BOOL
APIENTRY
NtGdiTransformPoints(
    HDC hDC,
    PPOINT UnsafePtsIn,
    PPOINT UnsafePtOut,
    INT Count,
    INT iMode)
{
    PDC pdc;
    LPPOINT Points;
    ULONG Size;
    BOOL ret = TRUE;

    if (Count <= 0)
        return TRUE;

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Size = Count * sizeof(POINT);

    // FIXME: It would be wise to have a small stack buffer as optimization
    Points = ExAllocatePoolWithTag(PagedPool, Size, GDITAG_TEMP);
    if (!Points)
    {
        DC_UnlockDc(pdc);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    _SEH2_TRY
    {
        ProbeForWrite(UnsafePtOut, Size, 1);
        ProbeForRead(UnsafePtsIn, Size, 1);
        RtlCopyMemory(Points, UnsafePtsIn, Size);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Do not set last error */
        _SEH2_YIELD(goto leave;)
    }
    _SEH2_END;

    switch (iMode)
    {
        case GdiDpToLp:
            IntDPtoLP(pdc, Points, Count);
            break;

        case GdiLpToDp:
            IntLPtoDP(pdc, Points, Count);
            break;

        case 2: // Not supported yet. Need testing.
        default:
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            ret = FALSE;
            goto leave;
        }
    }

    _SEH2_TRY
    {
        /* Pointer was already probed! */
        RtlCopyMemory(UnsafePtOut, Points, Size);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Do not set last error */
    }
    _SEH2_END;

//
// If we are getting called that means User XForms is a mess!
//
leave:
    DC_UnlockDc(pdc);
    ExFreePoolWithTag(Points, GDITAG_TEMP);
    return ret;
}

// FIXME: Don't use floating point in the kernel
BOOL
FASTCALL
IntGdiModifyWorldTransform(
    PDC pDc,
    CONST LPXFORM lpXForm,
    DWORD Mode)
{
    XFORM xformWorld2Wnd;
    ASSERT(pDc);

    switch (Mode)
    {
        case MWT_IDENTITY:
            xformWorld2Wnd.eM11 = 1.0f;
            xformWorld2Wnd.eM12 = 0.0f;
            xformWorld2Wnd.eM21 = 0.0f;
            xformWorld2Wnd.eM22 = 1.0f;
            xformWorld2Wnd.eDx  = 0.0f;
            xformWorld2Wnd.eDy  = 0.0f;
            XForm2MatrixS(&pDc->dclevel.mxWorldToPage, &xformWorld2Wnd);
            break;

        case MWT_LEFTMULTIPLY:
            MatrixS2XForm(&xformWorld2Wnd, &pDc->dclevel.mxWorldToPage);
            IntGdiCombineTransform(&xformWorld2Wnd, lpXForm, &xformWorld2Wnd);
            XForm2MatrixS(&pDc->dclevel.mxWorldToPage, &xformWorld2Wnd);
            break;

        case MWT_RIGHTMULTIPLY:
            MatrixS2XForm(&xformWorld2Wnd, &pDc->dclevel.mxWorldToPage);
            IntGdiCombineTransform(&xformWorld2Wnd, &xformWorld2Wnd, lpXForm);
            XForm2MatrixS(&pDc->dclevel.mxWorldToPage, &xformWorld2Wnd);
            break;

        case MWT_MAX+1: // Must be MWT_SET????
            XForm2MatrixS(&pDc->dclevel.mxWorldToPage, lpXForm); // Do it like Wine.
            break;

        default:
            return FALSE;
    }
    DC_UpdateXforms(pDc);
    return TRUE;
}

BOOL
APIENTRY
NtGdiModifyWorldTransform(
    HDC hDC,
    LPXFORM  UnsafeXForm,
    DWORD Mode)
{
    PDC dc;
    XFORM SafeXForm; // FIXME: Use XFORML
    BOOL Ret = TRUE;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    // The xform is permitted to be NULL for MWT_IDENTITY.
    // However, if it is not NULL, then it must be valid even though it is not used.
    if (UnsafeXForm != NULL || Mode != MWT_IDENTITY)
    {
        _SEH2_TRY
        {
            ProbeForRead(UnsafeXForm, sizeof(XFORM), 1);
            RtlCopyMemory(&SafeXForm, UnsafeXForm, sizeof(XFORM));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Ret = FALSE;
        }
        _SEH2_END;
    }

    // Safe to handle kernel mode data.
    if (Ret) Ret = IntGdiModifyWorldTransform(dc, &SafeXForm, Mode);
    DC_UnlockDc(dc);
    return Ret;
}

BOOL
APIENTRY
NtGdiOffsetViewportOrgEx(
    HDC hDC,
    int XOffset,
    int YOffset,
    LPPOINT UnsafePoint)
{
    PDC      dc;
    PDC_ATTR pdcattr;
    NTSTATUS Status = STATUS_SUCCESS;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;

    if (UnsafePoint)
    {
        _SEH2_TRY
        {
            ProbeForWrite(UnsafePoint, sizeof(POINT), 1);
            UnsafePoint->x = pdcattr->ptlViewportOrg.x;
            UnsafePoint->y = pdcattr->ptlViewportOrg.y;
            if (pdcattr->dwLayout & LAYOUT_RTL)
            {
                UnsafePoint->x = -UnsafePoint->x;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            DC_UnlockDc(dc);
            return FALSE;
        }
    }

    if (pdcattr->dwLayout & LAYOUT_RTL)
    {
        XOffset = -XOffset;
    }
    pdcattr->ptlViewportOrg.x += XOffset;
    pdcattr->ptlViewportOrg.y += YOffset;
    DC_UpdateXforms(dc);
    DC_UnlockDc(dc);

    return TRUE;
}

BOOL
APIENTRY
NtGdiOffsetWindowOrgEx(
    HDC hDC,
    int XOffset,
    int YOffset,
    LPPOINT Point)
{
    PDC dc;
    PDC_ATTR pdcattr;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;

    if (Point)
    {
        NTSTATUS Status = STATUS_SUCCESS;

        _SEH2_TRY
        {
            ProbeForWrite(Point, sizeof(POINT), 1);
            Point->x = pdcattr->ptlWindowOrg.x;
            Point->y = pdcattr->ptlWindowOrg.y;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            DC_UnlockDc(dc);
            return FALSE;
        }
    }

    pdcattr->ptlWindowOrg.x += XOffset;
    pdcattr->ptlWindowOrg.y += YOffset;

    DC_UpdateXforms(dc);
    DC_UnlockDc(dc);

    return TRUE;
}

BOOL
APIENTRY
NtGdiScaleViewportExtEx(
    HDC hDC,
    int Xnum,
    int Xdenom,
    int Ynum,
    int Ydenom,
    LPSIZE pSize)
{
    PDC pDC;
    PDC_ATTR pdcattr;
    BOOL Ret = FALSE;
    LONG X, Y;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = pDC->pdcattr;

    if (pdcattr->iMapMode > MM_TWIPS)
    {
        if (Xdenom && Ydenom)
        {
            X = Xnum * pdcattr->szlViewportExt.cx / Xdenom;
            if (X)
            {
                Y = Ynum * pdcattr->szlViewportExt.cy / Ydenom;
                if (Y)
                {
                    pdcattr->szlViewportExt.cx = X;
                    pdcattr->szlViewportExt.cy = Y;

                    IntMirrorWindowOrg(pDC);

                    pdcattr->flXform |= (PAGE_EXTENTS_CHANGED |
                                         INVALIDATE_ATTRIBUTES |
                                         DEVICE_TO_WORLD_INVALID);

                    if (pdcattr->iMapMode == MM_ISOTROPIC)
                    {
                        IntFixIsotropicMapping(pDC);
                    }
                    DC_UpdateXforms(pDC);

                    Ret = TRUE;
                }
            }
        }
    }
    else
        Ret = TRUE;

    if (pSize)
    {
        _SEH2_TRY
        {
            ProbeForWrite(pSize, sizeof(SIZE), 1);

            pSize->cx = pdcattr->szlViewportExt.cx;
            pSize->cy = pdcattr->szlViewportExt.cy;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
            Ret = FALSE;
        }
        _SEH2_END;
    }

    DC_UnlockDc(pDC);
    return Ret;
}

BOOL
APIENTRY
NtGdiScaleWindowExtEx(
    HDC hDC,
    int Xnum,
    int Xdenom,
    int Ynum,
    int Ydenom,
    LPSIZE pSize)
{
    PDC pDC;
    PDC_ATTR pdcattr;
    BOOL Ret = FALSE;
    LONG X, Y;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = pDC->pdcattr;

    if (pSize)
    {
        NTSTATUS Status = STATUS_SUCCESS;

        _SEH2_TRY
        {
            ProbeForWrite(pSize, sizeof(SIZE), 1);

            X = pdcattr->szlWindowExt.cx;
            if (pdcattr->dwLayout & LAYOUT_RTL) X = -X;
            pSize->cx = X;
            pSize->cy = pdcattr->szlWindowExt.cy;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            DC_UnlockDc(pDC);
            return FALSE;
        }
    }

    if (pdcattr->iMapMode > MM_TWIPS)
    {
        if (Xdenom && Ydenom)
        {
            X = Xnum * pdcattr->szlWindowExt.cx / Xdenom;
            if (X)
            {
                Y = Ynum * pdcattr->szlWindowExt.cy / Ydenom;
                if (Y)
                {
                    pdcattr->szlWindowExt.cx = X;
                    pdcattr->szlWindowExt.cy = Y;

                    IntMirrorWindowOrg(pDC);

                    pdcattr->flXform |= (PAGE_EXTENTS_CHANGED|INVALIDATE_ATTRIBUTES|DEVICE_TO_WORLD_INVALID);

                    if (pdcattr->iMapMode == MM_ISOTROPIC) IntFixIsotropicMapping(pDC);
                    DC_UpdateXforms(pDC);

                    Ret = TRUE;
                }
            }
        }
    }
    else
        Ret = TRUE;

    DC_UnlockDc(pDC);
    return Ret;
}

int
APIENTRY
IntGdiSetMapMode(
    PDC dc,
    int MapMode)
{
    int PrevMapMode;
    PDC_ATTR pdcattr = dc->pdcattr;

    PrevMapMode = pdcattr->iMapMode;

    pdcattr->iMapMode = MapMode;

    switch (MapMode)
    {
        case MM_TEXT:
            pdcattr->szlWindowExt.cx = 1;
            pdcattr->szlWindowExt.cy = 1;
            pdcattr->szlViewportExt.cx = 1;
            pdcattr->szlViewportExt.cy = 1;
            pdcattr->flXform &= ~(ISO_OR_ANISO_MAP_MODE|PTOD_EFM22_NEGATIVE|
                                  PTOD_EFM11_NEGATIVE|POSITIVE_Y_IS_UP);
            pdcattr->flXform |= (PAGE_XLATE_CHANGED|PAGE_TO_DEVICE_SCALE_IDENTITY|
                                 INVALIDATE_ATTRIBUTES|DEVICE_TO_WORLD_INVALID);
            break;

        case MM_ISOTROPIC:
            pdcattr->flXform |= ISO_OR_ANISO_MAP_MODE;
            /* Fall through */

        case MM_LOMETRIC:
            pdcattr->szlWindowExt.cx = pdcattr->szlVirtualDeviceMm.cx * 10;
            pdcattr->szlWindowExt.cy = pdcattr->szlVirtualDeviceMm.cy * 10;
            pdcattr->szlViewportExt.cx =  pdcattr->szlVirtualDevicePixel.cx;
            pdcattr->szlViewportExt.cy = -pdcattr->szlVirtualDevicePixel.cy;
            break;

        case MM_HIMETRIC:
            pdcattr->szlWindowExt.cx = pdcattr->szlVirtualDeviceMm.cx * 100;
            pdcattr->szlWindowExt.cy = pdcattr->szlVirtualDeviceMm.cy * 100;
            pdcattr->szlViewportExt.cx =  pdcattr->szlVirtualDevicePixel.cx;
            pdcattr->szlViewportExt.cy = -pdcattr->szlVirtualDevicePixel.cy;
            break;

        case MM_LOENGLISH:
            pdcattr->szlWindowExt.cx = MulDiv(1000, pdcattr->szlVirtualDeviceMm.cx, 254);
            pdcattr->szlWindowExt.cy = MulDiv(1000, pdcattr->szlVirtualDeviceMm.cy, 254);
            pdcattr->szlViewportExt.cx =  pdcattr->szlVirtualDevicePixel.cx;
            pdcattr->szlViewportExt.cy = -pdcattr->szlVirtualDevicePixel.cy;
            break;

        case MM_HIENGLISH:
            pdcattr->szlWindowExt.cx = MulDiv(10000, pdcattr->szlVirtualDeviceMm.cx, 254);
            pdcattr->szlWindowExt.cy = MulDiv(10000, pdcattr->szlVirtualDeviceMm.cy, 254);
            pdcattr->szlViewportExt.cx =  pdcattr->szlVirtualDevicePixel.cx;
            pdcattr->szlViewportExt.cy = -pdcattr->szlVirtualDevicePixel.cy;
            break;

        case MM_TWIPS:
            pdcattr->szlWindowExt.cx = MulDiv(14400, pdcattr->szlVirtualDeviceMm.cx, 254);
            pdcattr->szlWindowExt.cy = MulDiv(14400, pdcattr->szlVirtualDeviceMm.cy, 254);
            pdcattr->szlViewportExt.cx =  pdcattr->szlVirtualDevicePixel.cx;
            pdcattr->szlViewportExt.cy = -pdcattr->szlVirtualDevicePixel.cy;
            break;

        case MM_ANISOTROPIC:
            pdcattr->flXform &= ~(PAGE_TO_DEVICE_IDENTITY|POSITIVE_Y_IS_UP);
            pdcattr->flXform |= ISO_OR_ANISO_MAP_MODE;
            break;

        default:
            pdcattr->iMapMode = PrevMapMode;
            PrevMapMode = 0;
    }
    DC_UpdateXforms(dc);

    return PrevMapMode;
}


BOOL
APIENTRY
NtGdiSetViewportOrgEx(
    HDC hDC,
    int X,
    int Y,
    LPPOINT Point)
{
    PDC dc;
    PDC_ATTR pdcattr;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;

    if (Point)
    {
        NTSTATUS Status = STATUS_SUCCESS;

        _SEH2_TRY
        {
            ProbeForWrite(Point, sizeof(POINT), 1);
            Point->x = pdcattr->ptlViewportOrg.x;
            Point->y = pdcattr->ptlViewportOrg.y;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            DC_UnlockDc(dc);
            return FALSE;
        }
    }

    pdcattr->ptlViewportOrg.x = X;
    pdcattr->ptlViewportOrg.y = Y;

    DC_UpdateXforms(dc);
    DC_UnlockDc(dc);

    return TRUE;
}

BOOL
APIENTRY
NtGdiSetWindowOrgEx(
    HDC hDC,
    int X,
    int Y,
    LPPOINT Point)
{
    PDC dc;
    PDC_ATTR pdcattr;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;

    if (Point)
    {
        NTSTATUS Status = STATUS_SUCCESS;

        _SEH2_TRY
        {
            ProbeForWrite(Point, sizeof(POINT), 1);
            Point->x = pdcattr->ptlWindowOrg.x;
            Point->y = pdcattr->ptlWindowOrg.y;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            DC_UnlockDc(dc);
            return FALSE;
        }
    }

    pdcattr->ptlWindowOrg.x = X;
    pdcattr->ptlWindowOrg.y = Y;

    DC_UpdateXforms(dc);
    DC_UnlockDc(dc);

    return TRUE;
}

//
// Mirror Window function.
//
VOID
FASTCALL
IntMirrorWindowOrg(PDC dc)
{
    PDC_ATTR pdcattr;
    LONG X;

    pdcattr = dc->pdcattr;

    if (!(pdcattr->dwLayout & LAYOUT_RTL))
    {
        pdcattr->ptlWindowOrg.x = pdcattr->lWindowOrgx; // Flip it back.
        return;
    }
    if (!pdcattr->szlViewportExt.cx) return;
    //
    // WOrgx = wox - (Width - 1) * WExtx / VExtx
    //
    X = (dc->erclWindow.right - dc->erclWindow.left) - 1; // Get device width - 1

    X = (X * pdcattr->szlWindowExt.cx) / pdcattr->szlViewportExt.cx;

    pdcattr->ptlWindowOrg.x = pdcattr->lWindowOrgx - X; // Now set the inverted win origion.

    return;
}

VOID
NTAPI
DC_vSetLayout(
    IN PDC pdc,
    IN LONG wox,
    IN DWORD dwLayout)
{
    PDC_ATTR pdcattr = pdc->pdcattr;

    pdcattr->dwLayout = dwLayout;

    if (!(dwLayout & LAYOUT_ORIENTATIONMASK)) return;

    if (dwLayout & LAYOUT_RTL)
    {
        pdcattr->iMapMode = MM_ANISOTROPIC;
    }

    pdcattr->szlWindowExt.cy = -pdcattr->szlWindowExt.cy;
    pdcattr->ptlWindowOrg.x  = -pdcattr->ptlWindowOrg.x;

    if (wox == -1)
        IntMirrorWindowOrg(pdc);
    else
        pdcattr->ptlWindowOrg.x = wox - pdcattr->ptlWindowOrg.x;

    if (!(pdcattr->flTextAlign & TA_CENTER)) pdcattr->flTextAlign |= TA_RIGHT;

    if (pdc->dclevel.flPath & DCPATH_CLOCKWISE)
        pdc->dclevel.flPath &= ~DCPATH_CLOCKWISE;
    else
        pdc->dclevel.flPath |= DCPATH_CLOCKWISE;

    pdcattr->flXform |= (PAGE_EXTENTS_CHANGED |
                         INVALIDATE_ATTRIBUTES |
                         DEVICE_TO_WORLD_INVALID);

//  DC_UpdateXforms(pdc);
}

// NtGdiSetLayout
//
// The default is left to right. This function changes it to right to left, which
// is the standard in Arabic and Hebrew cultures.
//
/*
 * @implemented
 */
DWORD
APIENTRY
NtGdiSetLayout(
    IN HDC hdc,
    IN LONG wox,
    IN DWORD dwLayout)
{
    PDC pdc;
    PDC_ATTR pdcattr;
    DWORD oLayout;

    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }
    pdcattr = pdc->pdcattr;

    oLayout = pdcattr->dwLayout;
    DC_vSetLayout(pdc, wox, dwLayout);

    DC_UnlockDc(pdc);
    return oLayout;
}

/*
 * @implemented
 */
LONG
APIENTRY
NtGdiGetDeviceWidth(
    IN HDC hdc)
{
    PDC dc;
    LONG Ret;
    dc = DC_LockDc(hdc);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }
    Ret = dc->erclWindow.right - dc->erclWindow.left;
    DC_UnlockDc(dc);
    return Ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtGdiMirrorWindowOrg(
    IN HDC hdc)
{
    PDC dc;
    dc = DC_LockDc(hdc);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    IntMirrorWindowOrg(dc);
    DC_UnlockDc(dc);
    return TRUE;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtGdiSetSizeDevice(
    IN HDC hdc,
    IN INT cxVirtualDevice,
    IN INT cyVirtualDevice)
{
    PDC dc;
    PDC_ATTR pdcattr;

    if (!cxVirtualDevice || !cyVirtualDevice)
    {
        return FALSE;
    }

    dc = DC_LockDc(hdc);
    if (!dc) return FALSE;

    pdcattr = dc->pdcattr;

    pdcattr->szlVirtualDeviceSize.cx = cxVirtualDevice;
    pdcattr->szlVirtualDeviceSize.cy = cyVirtualDevice;

//    DC_UpdateXforms(dc);
    DC_UnlockDc(dc);

    return TRUE;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtGdiSetVirtualResolution(
    IN HDC hdc,
    IN INT cxVirtualDevicePixel,
    IN INT cyVirtualDevicePixel,
    IN INT cxVirtualDeviceMm,
    IN INT cyVirtualDeviceMm)
{
    PDC dc;
    PDC_ATTR pdcattr;

    /* Check parameters (all zeroes resets to real resolution) */
    if (cxVirtualDevicePixel == 0 && cyVirtualDevicePixel == 0 &&
        cxVirtualDeviceMm == 0 && cyVirtualDeviceMm == 0)
    {
        cxVirtualDevicePixel = NtGdiGetDeviceCaps(hdc, HORZRES);
        cyVirtualDevicePixel = NtGdiGetDeviceCaps(hdc, VERTRES);
        cxVirtualDeviceMm = NtGdiGetDeviceCaps(hdc, HORZSIZE);
        cyVirtualDeviceMm = NtGdiGetDeviceCaps(hdc, VERTSIZE);
    }
    else if (cxVirtualDevicePixel == 0 || cyVirtualDevicePixel == 0 ||
             cxVirtualDeviceMm == 0 || cyVirtualDeviceMm == 0)
    {
        return FALSE;
    }

    dc = DC_LockDc(hdc);
    if (!dc) return FALSE;

    pdcattr = dc->pdcattr;

    pdcattr->szlVirtualDevicePixel.cx = cxVirtualDevicePixel;
    pdcattr->szlVirtualDevicePixel.cy = cyVirtualDevicePixel;
    pdcattr->szlVirtualDeviceMm.cx = cxVirtualDeviceMm;
    pdcattr->szlVirtualDeviceMm.cy = cyVirtualDeviceMm;

//    DC_UpdateXforms(dc);
    DC_UnlockDc(dc);
    return TRUE;
}


// FIXME: Don't use floating point in the kernel!
BOOL FASTCALL
DC_InvertXform(const XFORM *xformSrc,
               XFORM *xformDest)
{
    FLOAT  determinant;

    determinant = xformSrc->eM11*xformSrc->eM22 - xformSrc->eM12*xformSrc->eM21;
    if (determinant > -1e-12 && determinant < 1e-12)
    {
        return  FALSE;
    }

    xformDest->eM11 =  xformSrc->eM22 / determinant;
    xformDest->eM12 = -xformSrc->eM12 / determinant;
    xformDest->eM21 = -xformSrc->eM21 / determinant;
    xformDest->eM22 =  xformSrc->eM11 / determinant;
    xformDest->eDx  = -xformSrc->eDx * xformDest->eM11 - xformSrc->eDy * xformDest->eM21;
    xformDest->eDy  = -xformSrc->eDx * xformDest->eM12 - xformSrc->eDy * xformDest->eM22;

    return  TRUE;
}

LONG FASTCALL
IntCalcFillOrigin(PDC pdc)
{
    pdc->ptlFillOrigin.x = pdc->dclevel.ptlBrushOrigin.x + pdc->ptlDCOrig.x;
    pdc->ptlFillOrigin.y = pdc->dclevel.ptlBrushOrigin.y + pdc->ptlDCOrig.y;

    return pdc->ptlFillOrigin.y;
}

PPOINTL
FASTCALL
IntptlBrushOrigin(PDC pdc, LONG x, LONG y )
{
    pdc->dclevel.ptlBrushOrigin.x = x;
    pdc->dclevel.ptlBrushOrigin.y = y;
    IntCalcFillOrigin(pdc);
    return &pdc->dclevel.ptlBrushOrigin;
}

static
VOID FASTCALL
DC_vGetAspectRatioFilter(PDC pDC, LPSIZE AspectRatio)
{
    if (pDC->pdcattr->flFontMapper & 1) // TRUE assume 1.
    {
        // "This specifies that Windows should only match fonts that have the
        // same aspect ratio as the display.", Programming Windows, Fifth Ed.
        AspectRatio->cx = pDC->ppdev->gdiinfo.ulLogPixelsX;
        AspectRatio->cy = pDC->ppdev->gdiinfo.ulLogPixelsY;
    }
    else
    {
        AspectRatio->cx = 0;
        AspectRatio->cy = 0;
    }
}

BOOL APIENTRY
NtGdiGetDCPoint(
    HDC hDC,
    UINT iPoint,
    PPOINTL Point)
{
    BOOL Ret = TRUE;
    DC *pdc;
    POINTL SafePoint;
    SIZE Size;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!Point)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    switch (iPoint)
    {
        case GdiGetViewPortExt:
            DC_vUpdateViewportExt(pdc);
            SafePoint.x = pdc->pdcattr->szlViewportExt.cx;
            SafePoint.y = pdc->pdcattr->szlViewportExt.cy;
            break;

        case GdiGetWindowExt:
            SafePoint.x = pdc->pdcattr->szlWindowExt.cx;
            SafePoint.y = pdc->pdcattr->szlWindowExt.cy;
            break;

        case GdiGetViewPortOrg:
            SafePoint = pdc->pdcattr->ptlViewportOrg;
            break;

        case GdiGetWindowOrg:
            SafePoint = pdc->pdcattr->ptlWindowOrg;
            break;

        case GdiGetDCOrg:
            SafePoint = pdc->ptlDCOrig;
            break;

        case GdiGetAspectRatioFilter:
            DC_vGetAspectRatioFilter(pdc, &Size);
            SafePoint.x = Size.cx;
            SafePoint.y = Size.cy;
            break;

        default:
            EngSetLastError(ERROR_INVALID_PARAMETER);
            Ret = FALSE;
            break;
    }

    if (Ret)
    {
        _SEH2_TRY
        {
            ProbeForWrite(Point, sizeof(POINT), 1);
            *Point = SafePoint;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            Ret = FALSE;
        }
    }

    DC_UnlockDc(pdc);
    return Ret;
}

/* EOF */
