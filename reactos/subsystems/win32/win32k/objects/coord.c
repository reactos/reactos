/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Coordinate systems
 * FILE:             subsys/win32k/objects/coord.c
 * PROGRAMER:        Unknown
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

void FASTCALL
IntFixIsotropicMapping(PDC dc)
{
    PDC_ATTR pdcattr;
    LONG fx, fy, s;

    /* Get a pointer to the DC_ATTR */
    pdcattr = dc->pdcattr;

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

BOOL FASTCALL
IntGdiCombineTransform(LPXFORM XFormResult,
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

BOOL APIENTRY NtGdiCombineTransform(LPXFORM  UnsafeXFormResult,
                                    LPXFORM  Unsafexform1,
                                    LPXFORM  Unsafexform2)
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


int
FASTCALL
IntGetGraphicsMode(PDC dc)
{
    PDC_ATTR pdcattr;
    ASSERT(dc);
    pdcattr = dc->pdcattr;
    return pdcattr->iGraphicsMode;
}

BOOL
FASTCALL
IntGdiModifyWorldTransform(PDC pDc,
                           CONST LPXFORM lpXForm,
                           DWORD Mode)
{
    ASSERT(pDc);
    XFORM xformWorld2Wnd;

    switch (Mode)
    {
        case MWT_IDENTITY:
            xformWorld2Wnd.eM11 = 1.0f;
            xformWorld2Wnd.eM12 = 0.0f;
            xformWorld2Wnd.eM21 = 0.0f;
            xformWorld2Wnd.eM22 = 1.0f;
            xformWorld2Wnd.eDx  = 0.0f;
            xformWorld2Wnd.eDy  = 0.0f;
            XForm2MatrixS(&pDc->DcLevel.mxWorldToPage, &xformWorld2Wnd);
            break;

        case MWT_LEFTMULTIPLY:
            MatrixS2XForm(&xformWorld2Wnd, &pDc->DcLevel.mxWorldToPage);
            IntGdiCombineTransform(&xformWorld2Wnd, lpXForm, &xformWorld2Wnd);
            XForm2MatrixS(&pDc->DcLevel.mxWorldToPage, &xformWorld2Wnd);
            break;

        case MWT_RIGHTMULTIPLY:
            MatrixS2XForm(&xformWorld2Wnd, &pDc->DcLevel.mxWorldToPage);
            IntGdiCombineTransform(&xformWorld2Wnd, &xformWorld2Wnd, lpXForm);
            XForm2MatrixS(&pDc->DcLevel.mxWorldToPage, &xformWorld2Wnd);
            break;

        case MWT_MAX+1: // Must be MWT_SET????
            XForm2MatrixS(&pDc->DcLevel.mxWorldToPage, lpXForm); // Do it like Wine.
            break;

        default:
            return FALSE;
    }
    DC_UpdateXforms(pDc);
    return TRUE;
}

BOOL
APIENTRY
NtGdiGetTransform(HDC  hDC,
                  DWORD iXform,
                  LPXFORM  XForm)
{
    PDC  dc;
    NTSTATUS Status = STATUS_SUCCESS;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!XForm)
    {
        DC_UnlockDc(dc);
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    _SEH2_TRY
    {
        ProbeForWrite(XForm, sizeof(XFORM), 1);
        switch (iXform)
        {
            case GdiWorldSpaceToPageSpace:
                MatrixS2XForm(XForm, &dc->DcLevel.mxWorldToPage);
                break;
            default:
                break;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    DC_UnlockDc(dc);
    return NT_SUCCESS(Status);
}


/*!
 * Converts points from logical coordinates into device coordinates. Conversion depends on the mapping mode,
 * world transfrom, viewport origin settings for the given device context.
 * \param	hDC		device context.
 * \param	Points	an array of POINT structures (in/out).
 * \param	Count	number of elements in the array of POINT structures.
 * \return  TRUE 	if success.
*/
BOOL
APIENTRY
NtGdiTransformPoints(HDC hDC,
                     PPOINT UnsafePtsIn,
                     PPOINT UnsafePtOut,
                     INT Count,
                     INT iMode)
{
    PDC dc;
    NTSTATUS Status = STATUS_SUCCESS;
    LPPOINT Points;
    ULONG Size;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!UnsafePtsIn || !UnsafePtOut || Count <= 0)
    {
        DC_UnlockDc(dc);
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Size = Count * sizeof(POINT);

    Points = (LPPOINT)ExAllocatePoolWithTag(PagedPool, Size, TAG_COORD);
    if (!Points)
    {
        DC_UnlockDc(dc);
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
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
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        DC_UnlockDc(dc);
        ExFreePoolWithTag(Points, TAG_COORD);
        SetLastNtError(Status);
        return FALSE;
    }

    switch (iMode)
    {
        case GdiDpToLp:
            IntDPtoLP(dc, Points, Count);
            break;
        case GdiLpToDp:
            IntLPtoDP(dc, Points, Count);
            break;
        case 2: // Not supported yet. Need testing.
        default:
        {
            DC_UnlockDc(dc);
            ExFreePoolWithTag(Points, TAG_COORD);
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    _SEH2_TRY
    {
        /* pointer was already probed! */
        RtlCopyMemory(UnsafePtOut, Points, Size);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        DC_UnlockDc(dc);
        ExFreePoolWithTag(Points, TAG_COORD);
        SetLastNtError(Status);
        return FALSE;
    }
//
// If we are getting called that means User XForms is a mess!
//
    DC_UnlockDc(dc);
    ExFreePoolWithTag(Points, TAG_COORD);
    return TRUE;
}

BOOL
APIENTRY
NtGdiModifyWorldTransform(HDC hDC,
                          LPXFORM  UnsafeXForm,
                          DWORD Mode)
{
    PDC dc;
    XFORM SafeXForm;
    BOOL Ret = TRUE;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
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
NtGdiOffsetViewportOrgEx(HDC hDC,
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
        SetLastWin32Error(ERROR_INVALID_HANDLE);
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
NtGdiOffsetWindowOrgEx(HDC  hDC,
                       int  XOffset,
                       int  YOffset,
                       LPPOINT  Point)
{
    PDC dc;
    PDC_ATTR pdcattr;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
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
NtGdiScaleViewportExtEx(HDC  hDC,
                        int  Xnum,
                        int  Xdenom,
                        int  Ynum,
                        int  Ydenom,
                        LPSIZE pSize)
{
    PDC pDC;
    PDC_ATTR pdcattr;
    BOOL Ret = FALSE;
    LONG X, Y;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = pDC->pdcattr;

    if (pSize)
    {
        NTSTATUS Status = STATUS_SUCCESS;

        _SEH2_TRY
        {
            ProbeForWrite(pSize, sizeof(LPSIZE), 1);

            pSize->cx = pdcattr->szlViewportExt.cx;
            pSize->cy = pdcattr->szlViewportExt.cy;
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

    DC_UnlockDc(pDC);
    return Ret;
}

BOOL
APIENTRY
NtGdiScaleWindowExtEx(HDC  hDC,
                      int  Xnum,
                      int  Xdenom,
                      int  Ynum,
                      int  Ydenom,
                      LPSIZE pSize)
{
    PDC pDC;
    PDC_ATTR pdcattr;
    BOOL Ret = FALSE;
    LONG X, Y;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = pDC->pdcattr;

    if (pSize)
    {
        NTSTATUS Status = STATUS_SUCCESS;

        _SEH2_TRY
        {
            ProbeForWrite(pSize, sizeof(LPSIZE), 1);

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
IntGdiSetMapMode(PDC  dc,
                 int  MapMode)
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
            pdcattr->szlWindowExt.cx = 3600;
            pdcattr->szlWindowExt.cy = 2700;
            pdcattr->szlViewportExt.cx = ((PGDIDEVICE)dc->ppdev)->GDIInfo.ulHorzRes;
            pdcattr->szlViewportExt.cy = -((PGDIDEVICE)dc->ppdev)->GDIInfo.ulVertRes;
            break;

        case MM_HIMETRIC:
            pdcattr->szlWindowExt.cx = 36000;
            pdcattr->szlWindowExt.cy = 27000;
            pdcattr->szlViewportExt.cx = ((PGDIDEVICE)dc->ppdev)->GDIInfo.ulHorzRes;
            pdcattr->szlViewportExt.cy = -((PGDIDEVICE)dc->ppdev)->GDIInfo.ulVertRes;
            break;

        case MM_LOENGLISH:
            pdcattr->szlWindowExt.cx = 1417;
            pdcattr->szlWindowExt.cy = 1063;
            pdcattr->szlViewportExt.cx = ((PGDIDEVICE)dc->ppdev)->GDIInfo.ulHorzRes;
            pdcattr->szlViewportExt.cy = -((PGDIDEVICE)dc->ppdev)->GDIInfo.ulVertRes;
            break;

        case MM_HIENGLISH:
            pdcattr->szlWindowExt.cx = 14173;
            pdcattr->szlWindowExt.cy = 10630;
            pdcattr->szlViewportExt.cx = ((PGDIDEVICE)dc->ppdev)->GDIInfo.ulHorzRes;
            pdcattr->szlViewportExt.cy = -((PGDIDEVICE)dc->ppdev)->GDIInfo.ulVertRes;
            break;

        case MM_TWIPS:
            pdcattr->szlWindowExt.cx = 20409;
            pdcattr->szlWindowExt.cy = 15307;
            pdcattr->szlViewportExt.cx = ((PGDIDEVICE)dc->ppdev)->GDIInfo.ulHorzRes;
            pdcattr->szlViewportExt.cy = -((PGDIDEVICE)dc->ppdev)->GDIInfo.ulVertRes;
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
NtGdiSetViewportOrgEx(HDC  hDC,
                      int  X,
                      int  Y,
                      LPPOINT  Point)
{
    PDC dc;
    PDC_ATTR pdcattr;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
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
NtGdiSetWindowOrgEx(HDC  hDC,
                    int  X,
                    int  Y,
                    LPPOINT  Point)
{
    PDC dc;
    PDC_ATTR pdcattr;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
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
    PDC dc;
    PDC_ATTR pdcattr;
    DWORD oLayout;

    dc = DC_LockDc(hdc);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }
    pdcattr = dc->pdcattr;

    pdcattr->dwLayout = dwLayout;
    oLayout = pdcattr->dwLayout;

    if (!(dwLayout & LAYOUT_ORIENTATIONMASK))
    {
        DC_UnlockDc(dc);
        return oLayout;
    }

    if (dwLayout & LAYOUT_RTL)
    {
        pdcattr->iMapMode = MM_ANISOTROPIC;
    }

    pdcattr->szlWindowExt.cy = -pdcattr->szlWindowExt.cy;
    pdcattr->ptlWindowOrg.x  = -pdcattr->ptlWindowOrg.x;

    if (wox == -1)
        IntMirrorWindowOrg(dc);
    else
        pdcattr->ptlWindowOrg.x = wox - pdcattr->ptlWindowOrg.x;

    if (!(pdcattr->flTextAlign & TA_CENTER)) pdcattr->flTextAlign |= TA_RIGHT;

    if (dc->DcLevel.flPath & DCPATH_CLOCKWISE)
        dc->DcLevel.flPath &= ~DCPATH_CLOCKWISE;
    else
        dc->DcLevel.flPath |= DCPATH_CLOCKWISE;

    pdcattr->flXform |= (PAGE_EXTENTS_CHANGED |
                         INVALIDATE_ATTRIBUTES |
                         DEVICE_TO_WORLD_INVALID);

//  DC_UpdateXforms(dc);
    DC_UnlockDc(dc);
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
        SetLastWin32Error(ERROR_INVALID_HANDLE);
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
        SetLastWin32Error(ERROR_INVALID_HANDLE);
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

    // Need test types for zeros and non zeros

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

/* EOF */
