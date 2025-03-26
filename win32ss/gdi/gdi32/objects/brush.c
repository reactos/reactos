#include <precomp.h>

#define NDEBUG
#include <debug.h>



/*
 * @implemented
 */
HPEN
APIENTRY
ExtCreatePen(DWORD dwPenStyle,
             DWORD dwWidth,
             CONST LOGBRUSH *lplb,
             DWORD dwStyleCount,
             CONST DWORD *lpStyle)
{
    PVOID lpPackedDIB = NULL;
    HPEN hPen = NULL;
    PBITMAPINFO pConvertedInfo = NULL;
    UINT ConvertedInfoSize = 0, lbStyle;
    BOOL Hit = FALSE;

    if ((dwPenStyle & PS_STYLE_MASK) == PS_USERSTYLE)
    {
        if(!lpStyle)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
    } // This is an enhancement and prevents a call to kernel space.
    else if ((dwPenStyle & PS_STYLE_MASK) == PS_INSIDEFRAME &&
             (dwPenStyle & PS_TYPE_MASK) != PS_GEOMETRIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    else if ((dwPenStyle & PS_STYLE_MASK) == PS_ALTERNATE &&
             (dwPenStyle & PS_TYPE_MASK) != PS_COSMETIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    else
    {
        if (dwStyleCount || lpStyle)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
    }

    lbStyle = lplb->lbStyle;

    if (lplb->lbStyle > BS_HATCHED)
    {
        if (lplb->lbStyle == BS_PATTERN)
        {
            pConvertedInfo = (PBITMAPINFO)lplb->lbHatch;
            if (!pConvertedInfo) return 0;
        }
        else
        {
            if ((lplb->lbStyle == BS_DIBPATTERN) || (lplb->lbStyle == BS_DIBPATTERNPT))
            {
                if (lplb->lbStyle == BS_DIBPATTERN)
                {
                    lbStyle = BS_DIBPATTERNPT;
                    lpPackedDIB = GlobalLock((HGLOBAL)lplb->lbHatch);
                    if (lpPackedDIB == NULL) return 0;
                }
                pConvertedInfo = ConvertBitmapInfo((PBITMAPINFO)lpPackedDIB,
                                                   lplb->lbColor,
                                                   &ConvertedInfoSize,
                                                   TRUE);
                Hit = TRUE; // We converted DIB.
            }
            else
                pConvertedInfo = (PBITMAPINFO)lpStyle;
        }
    }
    else
        pConvertedInfo = (PBITMAPINFO)lplb->lbHatch;


    hPen = NtGdiExtCreatePen(dwPenStyle,
                             dwWidth,
                             lbStyle,
                             lplb->lbColor,
                             lplb->lbHatch,
                             (ULONG_PTR)pConvertedInfo,
                             dwStyleCount,
                             (PULONG)lpStyle,
                             ConvertedInfoSize,
                             FALSE,
                             NULL);


    if (lplb->lbStyle == BS_DIBPATTERN) GlobalUnlock((HGLOBAL)lplb->lbHatch);

    if (Hit)
    {
        if ((PBITMAPINFO)lpPackedDIB != pConvertedInfo)
            RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);
    }
    return hPen;
}

/*
 * @implemented
 */
HBRUSH WINAPI
CreateDIBPatternBrush(
    HGLOBAL hglbDIBPacked,
    UINT fuColorSpec)
{
    PVOID lpPackedDIB;
    HBRUSH hBrush = NULL;
    PBITMAPINFO pConvertedInfo;
    UINT ConvertedInfoSize;

    lpPackedDIB = GlobalLock(hglbDIBPacked);
    if (lpPackedDIB == NULL)
        return 0;

    pConvertedInfo = ConvertBitmapInfo((PBITMAPINFO)lpPackedDIB, fuColorSpec,
                                       &ConvertedInfoSize, TRUE);
    if (pConvertedInfo)
    {
        hBrush = NtGdiCreateDIBBrush(pConvertedInfo, fuColorSpec,
                                     ConvertedInfoSize, FALSE, FALSE, lpPackedDIB);
        if ((PBITMAPINFO)lpPackedDIB != pConvertedInfo)
            RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);
    }

    GlobalUnlock(hglbDIBPacked);

    return hBrush;
}

/*
 * @implemented
 */
HBRUSH WINAPI
CreateDIBPatternBrushPt(
    CONST VOID *lpPackedDIB,
    UINT fuColorSpec)
{
    HBRUSH hBrush = NULL;
    PBITMAPINFO pConvertedInfo;
    UINT ConvertedInfoSize;

    if (lpPackedDIB == NULL)
        return 0;

    pConvertedInfo = ConvertBitmapInfo((PBITMAPINFO)lpPackedDIB, fuColorSpec,
                                       &ConvertedInfoSize, TRUE);
    if (pConvertedInfo)
    {
        hBrush = NtGdiCreateDIBBrush(pConvertedInfo, fuColorSpec,
                                     ConvertedInfoSize, FALSE, FALSE, (PVOID)lpPackedDIB);
        if ((PBITMAPINFO)lpPackedDIB != pConvertedInfo)
            RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);
    }

    return hBrush;
}

/*
 * @implemented
 */
HBRUSH
WINAPI
CreateHatchBrush(INT fnStyle,
                 COLORREF clrref)
{
    return NtGdiCreateHatchBrushInternal(fnStyle, clrref, FALSE);
}

/*
 * @implemented
 */
HBRUSH
WINAPI
CreatePatternBrush(HBITMAP hbmp)
{
    return NtGdiCreatePatternBrushInternal(hbmp, FALSE, FALSE);
}

/*
 * @implemented
 */
HBRUSH
WINAPI
CreateSolidBrush(IN COLORREF crColor)
{
    /* Call Server-Side API */
    return NtGdiCreateSolidBrush(crColor, NULL);
}

/*
 * @implemented
 */
HBRUSH WINAPI
CreateBrushIndirect(
    CONST LOGBRUSH *LogBrush)
{
    HBRUSH hBrush;

    switch (LogBrush->lbStyle)
    {
    case BS_DIBPATTERN:
        hBrush = CreateDIBPatternBrush((HGLOBAL)LogBrush->lbHatch,
                                       LogBrush->lbColor);
        break;

    case BS_DIBPATTERNPT:
        hBrush = CreateDIBPatternBrushPt((PVOID)LogBrush->lbHatch,
                                         LogBrush->lbColor);
        break;

    case BS_PATTERN:
        hBrush = NtGdiCreatePatternBrushInternal((HBITMAP)LogBrush->lbHatch,
                 FALSE,
                 FALSE);
        break;

    case BS_PATTERN8X8:
        hBrush = NtGdiCreatePatternBrushInternal((HBITMAP)LogBrush->lbHatch,
                 FALSE,
                 TRUE);
        break;

    case BS_SOLID:
/*        hBrush = hGetPEBHandle(hctBrushHandle, LogBrush->lbColor);
        if (!hBrush)*/
        hBrush = NtGdiCreateSolidBrush(LogBrush->lbColor, 0);
        break;

    case BS_HATCHED:
        hBrush = NtGdiCreateHatchBrushInternal(LogBrush->lbHatch,
                                               LogBrush->lbColor,
                                               FALSE);
        break;

    case BS_NULL:
        hBrush = NtGdiGetStockObject(NULL_BRUSH);
        break;

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        hBrush = NULL;
        break;
    }

    return hBrush;
}

/*
 * @implemented
 *
 */
BOOL
WINAPI
GetBrushOrgEx(HDC hdc,LPPOINT pt)
{
    PDC_ATTR Dc_Attr;

    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;
    if (pt)
    {
        pt->x = Dc_Attr->ptlBrushOrigin.x;
        pt->y = Dc_Attr->ptlBrushOrigin.y;
    }
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetBrushOrgEx(HDC hdc,
              int nXOrg,
              int nYOrg,
              LPPOINT lppt)
{
    PDC_ATTR Dc_Attr;
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        PLDC pLDC = GdiGetLDC(hdc);
        if ( (pLDC == NULL) || (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC))
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
        if (pLDC->iType == LDC_EMFLDC)
        {
            return EMFDRV_SetBrushOrg(hdc, nXOrg, nYOrg); // ReactOS only.
        }
        return FALSE;
    }
#endif
    if (GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID)&Dc_Attr))
    {
        PGDIBSSETBRHORG pgSBO;

        /* Does the caller want the current brush origin to be returned? */
        if (lppt)
        {
            lppt->x = Dc_Attr->ptlBrushOrigin.x;
            lppt->y = Dc_Attr->ptlBrushOrigin.y;
        }

        /* Check if we have nothing to do */
        if ((nXOrg == Dc_Attr->ptlBrushOrigin.x) &&
            (nYOrg == Dc_Attr->ptlBrushOrigin.y))
            return TRUE;

        /* Allocate a batch command buffer */
        pgSBO = GdiAllocBatchCommand(hdc, GdiBCSetBrushOrg);
        if (pgSBO != NULL)
        {
            /* Set current brush origin in the DC attribute */
            Dc_Attr->ptlBrushOrigin.x = nXOrg;
            Dc_Attr->ptlBrushOrigin.y = nYOrg;

            /* Setup the GDI batch command */
            pgSBO->ptlBrushOrigin = Dc_Attr->ptlBrushOrigin;

            return TRUE;
        }
    }

    /* Fall back to the slower kernel path */
    return NtGdiSetBrushOrg(hdc, nXOrg, nYOrg, lppt);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetBrushAttributes(HBRUSH hbr)
{
    if ( GDI_HANDLE_IS_STOCKOBJ(hbr) )
    {
        return SC_BB_STOCKOBJ;
    }
    return 0;
}

/*
 * @implemented
 */
HBRUSH
WINAPI
SetBrushAttributes(HBRUSH hbm, DWORD dwFlags)
{
    if ( dwFlags & ~SC_BB_STOCKOBJ )
    {
        return NULL;
    }
    return NtGdiSetBrushAttributes(hbm, dwFlags);
}

/*
 * @implemented
 */
HBRUSH
WINAPI
ClearBrushAttributes(HBRUSH hbm, DWORD dwFlags)
{
    if ( dwFlags & ~SC_BB_STOCKOBJ )
    {
        return NULL;
    }
    return NtGdiClearBrushAttributes(hbm, dwFlags);
}

/*
 * @implemented
 */
BOOL
WINAPI
UnrealizeObject(HGDIOBJ  hgdiobj)
{
    BOOL retValue = TRUE;
    /*
       Win 2k Graphics API, Black Book. by coriolis.com
       Page 62, Note that Steps 3, 5, and 6 are not required for Windows NT(tm)
       and Windows 2000(tm).

       Step 5. UnrealizeObject(hTrackBrush);
     */
    /*
        msdn.microsoft.com,
        "Windows 2000/XP: If hgdiobj is a brush, UnrealizeObject does nothing,
        and the function returns TRUE. Use SetBrushOrgEx to set the origin of
        a brush."
     */
    if (GDI_HANDLE_GET_TYPE(hgdiobj) != GDI_OBJECT_TYPE_BRUSH)
    {
        retValue = NtGdiUnrealizeObject(hgdiobj);
    }

    return retValue;
}
