/*
 * COPYRIGHT:        GNU GPL, See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Coordinate systems
 * FILE:             win32ss/gdi/ntgdi/coord.c
 * PROGRAMER:        Timo Kreuzer (timo.kreuzer@rectos.org)
 */

/* Coordinate translation overview
 * -------------------------------
 *
 * Windows uses 3 different coordinate systems, referred to as world space,
 * page space and device space.
 *
 * Device space:
 * This is the coordinate system of the physical device that displays the
 * graphics. One unit matches one pixel of the surface. The coordinate system
 * is always orthogonal.
 *
 * Page space:
 * This is the coordinate system on the screen or on the paper layout for
 * printer devices. The coordinate system is also orthogonal but one unit
 * does not necessarily match one pixel. Instead there are different mapping
 * modes that can be set using SetMapMode() that specify how page space units
 * are transformed into device space units. These mapping modes are:
 * - MM_TEXT: One unit matches one unit in device space (one pixel)
 * - MM_TWIPS One unit matches 1/20 point (1/1440 inch)
 * - MM_LOMETRIC: One unit matches 0.1 millimeter
 * - MM_HIMETRIC: One unit matches 0.01 millimeter
 * - MM_LOENGLISH: One unit matches 0.01 inch
 * - MM_HIENGLISH: One unit matches 0.001 inch
 * - MM_ISOTROPIC:
 * - MM_ANISOTROPIC:
 * If the mapping mode is either MM_ISOTROPIC or MM_ANISOTROPIC, the actual
 * transformation is calculated from the window and viewport extension.
 * The window extension can be set using SetWindowExtEx() and describes the
 * extents of an arbitrary window (not to confuse with the gui element!) in
 * page space coordinates.
 * The viewport extension can be set using SetViewportExtEx() and describes
 * the extent of the same window in device space coordinates. If the mapping
 * mode is MM_ISOTROPIC one of the viewport extensions can be adjusted by GDI
 * to make sure the mapping stays isotropic, i.e. that it has the same x/y
 * ratio as the window extension.
 *
 * World space:
 * World space is the coordinate system that is used for all GDI drawing
 * operations. The metrics of this coordinate system depend on the DCs
 * graphics mode, which can be set using SetGraphicsMode().
 * If the graphics mode is GM_COMPATIBLE, world space is identical to page
 * space and no additional transformation is applied.
 * If the graphics mode is GM_ADVANCED, an arbitrary coordinate transformation
 * can be set using SetWorldTransform(), which is applied to transform world
 * space coordinates into page space coordinates.
 *
 * User mode data:
 * All coordinate translation data is stored in the DC attribute, so the values
 * might be invalid. This has to be taken into account. Values might also be
 * zero, so when a division is made, the value has to be read first and then
 * checked! This is true for both integer and floating point values, even if
 * we cannot get floating point exceptions on x86, we can get them on all other
 * architectures that use the FPU directly instead of emulation.
 * The result of all operations might be completely random and invalid, if it was
 * messed with in an illegal way in user mode. This is not a problem, since the
 * result of coordinate transformations are never expected to be "valid" values.
 * In the worst case, the drawing operation draws rubbish into the DC.
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>
C_ASSERT(sizeof(XFORML) == sizeof(XFORM));


/* GLOBALS *******************************************************************/

const MATRIX gmxIdentity =
{
    FLOATOBJ_1, FLOATOBJ_0,
    FLOATOBJ_0, FLOATOBJ_1,
    FLOATOBJ_0, FLOATOBJ_0,
    0, 0, XFORM_NO_TRANSLATION|XFORM_FORMAT_LTOL|XFORM_UNITY|XFORM_SCALE
};


/* FUNCTIONS *****************************************************************/

VOID
FASTCALL
DC_vFixIsotropicMapping(PDC pdc)
{
    PDC_ATTR pdcattr;
    LONG64 fx, fy;
    LONG s;
    SIZEL szlWindowExt, szlViewportExt;
    ASSERT(pdc->pdcattr->iMapMode == MM_ISOTROPIC);

    /* Get a pointer to the DC_ATTR */
    pdcattr = pdc->pdcattr;

    /* Read the extents, we rely on non-null values */
    szlWindowExt = pdcattr->szlWindowExt;
    szlViewportExt = pdcattr->szlViewportExt;

    /* Check if all values are valid */
    if ((szlWindowExt.cx == 0) || (szlWindowExt.cy == 0) ||
        (szlViewportExt.cx == 0) || (szlViewportExt.cy == 0))
    {
        /* Someone put rubbish into the fields, just ignore it. */
        return;
    }

    fx = abs((LONG64)szlWindowExt.cx * szlViewportExt.cy);
    fy = abs((LONG64)szlWindowExt.cy * szlViewportExt.cx);

    if (fx < fy)
    {
        s = (szlWindowExt.cy ^ szlViewportExt.cx) > 0 ? 1 : -1;
        pdcattr->szlViewportExt.cx = (LONG)(fx * s / szlWindowExt.cy);
    }
    else if (fx > fy)
    {
        s = (szlWindowExt.cx ^ szlViewportExt.cy) > 0 ? 1 : -1;
        pdcattr->szlViewportExt.cy = (LONG)(fy * s / szlWindowExt.cx);
    }

    /* Reset the flag */
    pdc->pdcattr->flXform &= ~PAGE_EXTENTS_CHANGED;
}

VOID
FASTCALL
DC_vGetPageToDevice(PDC pdc, MATRIX *pmx)
{
    PDC_ATTR pdcattr = pdc->pdcattr;
    PSIZEL pszlViewPortExt;
    SIZEL szlWindowExt;

    /* Get the viewport extension */
    pszlViewPortExt = DC_pszlViewportExt(pdc);

    /* Copy the window extension, so no one can mess with it */
    szlWindowExt = pdcattr->szlWindowExt;

    /* No shearing / rotation */
    FLOATOBJ_SetLong(&pmx->efM12, 0);
    FLOATOBJ_SetLong(&pmx->efM21, 0);

    /* Calculate scaling */
    if (szlWindowExt.cx != 0)
    {
        FLOATOBJ_SetLong(&pmx->efM11, pszlViewPortExt->cx);
        FLOATOBJ_DivLong(&pmx->efM11, szlWindowExt.cx);
    }
    else
        FLOATOBJ_SetLong(&pmx->efM11, 1);

    if (szlWindowExt.cy != 0)
    {
        FLOATOBJ_SetLong(&pmx->efM22, pszlViewPortExt->cy);
        FLOATOBJ_DivLong(&pmx->efM22, szlWindowExt.cy);
    }
    else
        FLOATOBJ_SetLong(&pmx->efM22, 1);

    /* Calculate x offset */
    FLOATOBJ_SetLong(&pmx->efDx, -pdcattr->ptlWindowOrg.x);
    FLOATOBJ_Mul(&pmx->efDx, &pmx->efM11);
    FLOATOBJ_AddLong(&pmx->efDx, pdcattr->ptlViewportOrg.x);

    /* Calculate y offset */
    FLOATOBJ_SetLong(&pmx->efDy, -pdcattr->ptlWindowOrg.y);
    FLOATOBJ_Mul(&pmx->efDy, &pmx->efM22);
    FLOATOBJ_AddLong(&pmx->efDy, pdcattr->ptlViewportOrg.y);
}

VOID
FASTCALL
DC_vUpdateWorldToDevice(PDC pdc)
{
    XFORMOBJ xoPageToDevice, xoWorldToPage, xoWorldToDevice;
    MATRIX mxPageToDevice;

    // FIXME: make sure world-to-page is valid!

    /* Construct a transformation to do the page-to-device conversion */
    DC_vGetPageToDevice(pdc, &mxPageToDevice);
    XFORMOBJ_vInit(&xoPageToDevice, &mxPageToDevice);

    /* Recalculate the world-to-device xform */
    XFORMOBJ_vInit(&xoWorldToPage, &pdc->pdcattr->mxWorldToPage);
    XFORMOBJ_vInit(&xoWorldToDevice, &pdc->pdcattr->mxWorldToDevice);
    XFORMOBJ_iCombine(&xoWorldToDevice, &xoWorldToPage, &xoPageToDevice);

    /* Reset the flags */
    pdc->pdcattr->flXform &= ~(PAGE_XLATE_CHANGED|PAGE_EXTENTS_CHANGED|WORLD_XFORM_CHANGED);
}

VOID
FASTCALL
DC_vUpdateDeviceToWorld(PDC pdc)
{
    XFORMOBJ xoWorldToDevice, xoDeviceToWorld;
    PMATRIX pmxWorldToDevice;

    /* Get the world-to-device translation */
    pmxWorldToDevice = DC_pmxWorldToDevice(pdc);
    XFORMOBJ_vInit(&xoWorldToDevice, pmxWorldToDevice);

    /* Create inverse of world-to-device transformation */
    XFORMOBJ_vInit(&xoDeviceToWorld, &pdc->pdcattr->mxDeviceToWorld);
    if (XFORMOBJ_iInverse(&xoDeviceToWorld, &xoWorldToDevice) == DDI_ERROR)
    {
        // FIXME: do we need to reset anything?
        return;
    }

    /* Reset the flag */
    pdc->pdcattr->flXform &= ~DEVICE_TO_WORLD_INVALID;
}

BOOL
NTAPI
GreCombineTransform(
    XFORML *pxformDest,
    XFORML *pxform1,
    XFORML *pxform2)
{
    MATRIX mxDest, mx1, mx2;
    XFORMOBJ xoDest, xo1, xo2;

    /* Check for illegal parameters */
    if (!pxformDest || !pxform1 || !pxform2) return FALSE;

    /* Initialize XFORMOBJs */
    XFORMOBJ_vInit(&xoDest, &mxDest);
    XFORMOBJ_vInit(&xo1, &mx1);
    XFORMOBJ_vInit(&xo2, &mx2);

    /* Convert the XFORMLs into XFORMOBJs */
    XFORMOBJ_iSetXform(&xo1, pxform1);
    XFORMOBJ_iSetXform(&xo2, pxform2);

    /* Combine them */
    XFORMOBJ_iCombine(&xoDest, &xo1, &xo2);

    /* Translate back into XFORML */
    XFORMOBJ_iGetXform(&xoDest, pxformDest);

    return TRUE;
}

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
        Ret = GreCombineTransform((XFORML*)UnsafeXFormResult,
                                  (XFORML*)Unsafexform1,
                                  (XFORML*)Unsafexform2);
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
    HDC hdc,
    DWORD iXform,
    LPXFORM pXForm)
{
    PDC pdc;
    BOOL ret = TRUE;
    MATRIX mxPageToDevice;
    XFORMOBJ xo;
    PMATRIX pmx;

    if (!pXForm)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    switch (iXform)
    {
        case GdiWorldSpaceToPageSpace:
            pmx = DC_pmxWorldToPage(pdc);
            break;

        case GdiWorldSpaceToDeviceSpace:
            pmx = DC_pmxWorldToDevice(pdc);
            break;

        case GdiDeviceSpaceToWorldSpace:
            pmx = DC_pmxDeviceToWorld(pdc);
            break;

        case GdiPageSpaceToDeviceSpace:
            DC_vGetPageToDevice(pdc, &mxPageToDevice);
            pmx = &mxPageToDevice;
            break;

        default:
            DPRINT1("Unknown transform %lu\n", iXform);
            ret = FALSE;
            goto leave;
    }

    /* Initialize an XFORMOBJ */
    XFORMOBJ_vInit(&xo, pmx);

    _SEH2_TRY
    {
        ProbeForWrite(pXForm, sizeof(XFORML), 1);
        XFORMOBJ_iGetXform(&xo, (XFORML*)pXForm);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = FALSE;
    }
    _SEH2_END;

leave:
    DC_UnlockDc(pdc);
    return ret;
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
            DC_vXformDeviceToWorld(pdc, Count, Points, Points);
            break;

        case GdiLpToDp:
            DC_vXformWorldToDevice(pdc, Count, Points, Points);
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
        ret = 0;
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

BOOL
NTAPI
GreModifyWorldTransform(
    PDC pdc,
    const XFORML *pxform,
    DWORD dwMode)
{
    MATRIX mxSrc;
    XFORMOBJ xoSrc, xoDC;

    switch (dwMode)
    {
        case MWT_IDENTITY:
            pdc->pdcattr->mxWorldToPage = gmxIdentity;
            break;

        case MWT_LEFTMULTIPLY:
            XFORMOBJ_vInit(&xoDC, &pdc->pdcattr->mxWorldToPage);
            XFORMOBJ_vInit(&xoSrc, &mxSrc);
            if (XFORMOBJ_iSetXform(&xoSrc, pxform) == DDI_ERROR)
                return FALSE;
            XFORMOBJ_iCombine(&xoDC, &xoSrc, &xoDC);
            break;

        case MWT_RIGHTMULTIPLY:
            XFORMOBJ_vInit(&xoDC, &pdc->pdcattr->mxWorldToPage);
            XFORMOBJ_vInit(&xoSrc, &mxSrc);
            if (XFORMOBJ_iSetXform(&xoSrc, pxform) == DDI_ERROR)
                return FALSE;
            XFORMOBJ_iCombine(&xoDC, &xoDC, &xoSrc);
            break;

        case MWT_MAX+1: // Must be MWT_SET????
            XFORMOBJ_vInit(&xoDC, &pdc->pdcattr->mxWorldToPage);
            if (XFORMOBJ_iSetXform(&xoDC, pxform) == DDI_ERROR)
                return FALSE;
            break;

        default:
            return FALSE;
    }

    /*Set invalidation flags */
    pdc->pdcattr->flXform |= WORLD_XFORM_CHANGED|DEVICE_TO_WORLD_INVALID;

    return TRUE;
}

BOOL
APIENTRY
NtGdiModifyWorldTransform(
    HDC hdc,
    LPXFORM pxformUnsafe,
    DWORD dwMode)
{
    PDC pdc;
    XFORML xformSafe;
    BOOL Ret = TRUE;

    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* The xform is permitted to be NULL for MWT_IDENTITY.
     * However, if it is not NULL, then it must be valid even
     * though it is not used. */
    if ((dwMode != MWT_IDENTITY) && (pxformUnsafe == NULL))
    {
        DC_UnlockDc(pdc);
        return FALSE;
    }

    if (pxformUnsafe != NULL)
    {
        _SEH2_TRY
        {
            ProbeForRead(pxformUnsafe, sizeof(XFORML), 1);
            RtlCopyMemory(&xformSafe, pxformUnsafe, sizeof(XFORML));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Ret = FALSE;
        }
        _SEH2_END;
    }

    /* Safe to handle kernel mode data. */
    if (Ret) Ret = GreModifyWorldTransform(pdc, &xformSafe, dwMode);
    DC_UnlockDc(pdc);
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
    pdcattr->flXform |= PAGE_XLATE_CHANGED;

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
    pdcattr->flXform |= PAGE_XLATE_CHANGED|DEVICE_TO_WORLD_INVALID;

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
            DC_pszlViewportExt(pDC);
            X = Xnum * pdcattr->szlViewportExt.cx / Xdenom;
            if (X)
            {
                Y = Ynum * pdcattr->szlViewportExt.cy / Ydenom;
                if (Y)
                {
                    pdcattr->szlViewportExt.cx = X;
                    pdcattr->szlViewportExt.cy = Y;
                    pdcattr->flXform |= PAGE_XLATE_CHANGED;

                    IntMirrorWindowOrg(pDC);

                    pdcattr->flXform |= (PAGE_EXTENTS_CHANGED |
                                         INVALIDATE_ATTRIBUTES |
                                         DEVICE_TO_WORLD_INVALID);

                    if (pdcattr->iMapMode == MM_ISOTROPIC)
                    {
                        DC_vFixIsotropicMapping(pDC);
                    }

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
    INT iPrevMapMode;
    FLONG flXform;
    PDC_ATTR pdcattr = dc->pdcattr;

    flXform = pdcattr->flXform & ~(ISO_OR_ANISO_MAP_MODE|PTOD_EFM22_NEGATIVE|
        PTOD_EFM11_NEGATIVE|POSITIVE_Y_IS_UP|PAGE_TO_DEVICE_SCALE_IDENTITY|
        PAGE_TO_DEVICE_IDENTITY);

    switch (MapMode)
    {
        case MM_TEXT:
            pdcattr->szlWindowExt.cx = 1;
            pdcattr->szlWindowExt.cy = 1;
            pdcattr->szlViewportExt.cx = 1;
            pdcattr->szlViewportExt.cy = 1;
            flXform |= PAGE_TO_DEVICE_SCALE_IDENTITY;
            break;

        case MM_ISOTROPIC:
            flXform |= ISO_OR_ANISO_MAP_MODE;
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
            flXform &= ~(PAGE_TO_DEVICE_IDENTITY|POSITIVE_Y_IS_UP);
            flXform |= ISO_OR_ANISO_MAP_MODE;
            break;

        default:
            return 0;
    }

    /* Save the old map mode and set the new one */
    iPrevMapMode = pdcattr->iMapMode;
    pdcattr->iMapMode = MapMode;

    /* Update xform flags */
    pdcattr->flXform = flXform | (PAGE_XLATE_CHANGED|PAGE_EXTENTS_CHANGED|
        INVALIDATE_ATTRIBUTES|DEVICE_TO_PAGE_INVALID|DEVICE_TO_WORLD_INVALID);

    return iPrevMapMode;
}

BOOL
FASTCALL
GreSetViewportOrgEx(
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
       Point->x = pdcattr->ptlViewportOrg.x;
       Point->y = pdcattr->ptlViewportOrg.y;
    }

    pdcattr->ptlViewportOrg.x = X;
    pdcattr->ptlViewportOrg.y = Y;
    pdcattr->flXform |= PAGE_XLATE_CHANGED;

    DC_UnlockDc(dc);
    return TRUE;
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
    pdcattr->flXform |= PAGE_XLATE_CHANGED;

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
    pdcattr->flXform |= PAGE_XLATE_CHANGED;

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
    LONG X, cx;

    pdcattr = dc->pdcattr;

    if (!(pdcattr->dwLayout & LAYOUT_RTL))
    {
        pdcattr->ptlWindowOrg.x = pdcattr->lWindowOrgx; // Flip it back.
        return;
    }

    /* Copy the window extension, so no one can mess with it */
    cx = pdcattr->szlViewportExt.cx;
    if (cx == 0) return;
    //
    // WOrgx = wox - (Width - 1) * WExtx / VExtx
    //
    X = (dc->erclWindow.right - dc->erclWindow.left) - 1; // Get device width - 1

    X = (X * pdcattr->szlWindowExt.cx) / cx;

    pdcattr->ptlWindowOrg.x = pdcattr->lWindowOrgx - X; // Now set the inverted win origion.
    pdcattr->flXform |= PAGE_XLATE_CHANGED;

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

    //pdcattr->szlWindowExt.cy = -pdcattr->szlWindowExt.cy;
    //pdcattr->ptlWindowOrg.x  = -pdcattr->ptlWindowOrg.x;

    //if (wox == -1)
    //    IntMirrorWindowOrg(pdc);
    //else
    //    pdcattr->ptlWindowOrg.x = wox - pdcattr->ptlWindowOrg.x;

    if (!(pdcattr->flTextAlign & TA_CENTER)) pdcattr->flTextAlign |= TA_RIGHT;

    if (pdc->dclevel.flPath & DCPATH_CLOCKWISE)
        pdc->dclevel.flPath &= ~DCPATH_CLOCKWISE;
    else
        pdc->dclevel.flPath |= DCPATH_CLOCKWISE;

    pdcattr->flXform |= (PAGE_EXTENTS_CHANGED |
                         INVALIDATE_ATTRIBUTES |
                         DEVICE_TO_WORLD_INVALID);
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
    DWORD dwOldLayout;

    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }

    dwOldLayout = pdc->pdcattr->dwLayout;
    DC_vSetLayout(pdc, wox, dwLayout);

    DC_UnlockDc(pdc);
    return dwOldLayout;
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

//    DC_vUpdateXforms(dc);
    DC_UnlockDc(dc);
    return TRUE;
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
GreGetDCPoint(
    HDC hDC,
    UINT iPoint,
    PPOINTL Point)
{
    BOOL Ret = TRUE;
    DC *pdc;
    SIZE Size;
    PSIZEL pszlViewportExt;

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
            pszlViewportExt = DC_pszlViewportExt(pdc);
            Point->x = pszlViewportExt->cx;
            Point->y = pszlViewportExt->cy;
            break;

        case GdiGetWindowExt:
            Point->x = pdc->pdcattr->szlWindowExt.cx;
            Point->y = pdc->pdcattr->szlWindowExt.cy;
            break;

        case GdiGetViewPortOrg:
            *Point = pdc->pdcattr->ptlViewportOrg;
            break;

        case GdiGetWindowOrg:
            *Point = pdc->pdcattr->ptlWindowOrg;
            break;

        case GdiGetDCOrg:
            *Point = pdc->ptlDCOrig;
            break;

        case GdiGetAspectRatioFilter:
            DC_vGetAspectRatioFilter(pdc, &Size);
            Point->x = Size.cx;
            Point->y = Size.cy;
            break;

        default:
            EngSetLastError(ERROR_INVALID_PARAMETER);
            Ret = FALSE;
            break;
    }

    DC_UnlockDc(pdc);
    return Ret;
}

BOOL
WINAPI
GreGetWindowExtEx(
    _In_ HDC hdc,
    _Out_ LPSIZE lpSize)
{
   return GreGetDCPoint(hdc, GdiGetWindowExt, (PPOINTL)lpSize);
}

BOOL
WINAPI
GreGetViewportExtEx(
    _In_ HDC hdc,
    _Out_ LPSIZE lpSize)
{
   return GreGetDCPoint(hdc, GdiGetViewPortExt, (PPOINTL)lpSize);
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
    PSIZEL pszlViewportExt;

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
            pszlViewportExt = DC_pszlViewportExt(pdc);
            SafePoint.x = pszlViewportExt->cx;
            SafePoint.y = pszlViewportExt->cy;
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
            Ret = FALSE;
        }
        _SEH2_END;
    }

    DC_UnlockDc(pdc);
    return Ret;
}

/* EOF */
