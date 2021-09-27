/*
 *  ReactOS GDI lib
 *  Copyright (C) 2003 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * PROJECT:         ReactOS gdi32.dll
 * FILE:            win32ss/gdi/gdi32/misc/misc.c
 * PURPOSE:         Miscellaneous functions
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      2004/09/04  Created
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>

PGDI_TABLE_ENTRY GdiHandleTable = NULL;
PGDI_SHARED_HANDLE_TABLE GdiSharedHandleTable = NULL;
HANDLE CurrentProcessId = NULL;
DWORD GDI_BatchLimit = 1;
extern PGDIHANDLECACHE GdiHandleCache;

/*
 * @implemented
 */
BOOL
WINAPI
GdiFlush(VOID)
{
    NtGdiFlush();
    return TRUE;
}

/*
 * @unimplemented
 */
INT
WINAPI
Escape(
    _In_ HDC hdc,
    _In_ INT nEscape,
    _In_ INT cbInput,
    _In_ LPCSTR lpvInData,
    _Out_ LPVOID lpvOutData)
{
    INT retValue = SP_ERROR;
    ULONG ulObjType;

    ulObjType = GDI_HANDLE_GET_TYPE(hdc);

    if (ulObjType == GDILoObjType_LO_METADC16_TYPE)
    {
        return METADC_ExtEscape(hdc, nEscape, cbInput, lpvInData, 0, lpvOutData);
    }

    switch (nEscape)
    {
        case ABORTDOC:
            /* Note: Windows checks if the handle has any user data for the ABORTDOC command
             * ReactOS copies this behavior to be compatible with windows 2003
             */
            if (GdiGetDcAttr(hdc) == NULL)
            {
                GdiSetLastError(ERROR_INVALID_HANDLE);
                retValue = FALSE;
            }
            else
            {
                retValue = AbortDoc(hdc);
            }
            break;

        case DRAFTMODE:
        case FLUSHOUTPUT:
        case SETCOLORTABLE:
            /* Note 1: DRAFTMODE, FLUSHOUTPUT, SETCOLORTABLE are outdated */
            /* Note 2: Windows checks if the handle has any user data for the DRAFTMODE, FLUSHOUTPUT, SETCOLORTABLE commands
             * ReactOS copies this behavior to be compatible with windows 2003
             */
            if (GdiGetDcAttr(hdc) == NULL)
            {
                GdiSetLastError(ERROR_INVALID_HANDLE);
            }
            retValue = FALSE;
            break;

        case SETABORTPROC:
            /* Note: Windows checks if the handle has any user data for the SETABORTPROC command
             * ReactOS copies this behavior to be compatible with windows 2003
             */
            if (GdiGetDcAttr(hdc) == NULL)
            {
                GdiSetLastError(ERROR_INVALID_HANDLE);
                retValue = FALSE;
            }
            retValue = SetAbortProc(hdc, (ABORTPROC)lpvInData);
            break;

        case GETCOLORTABLE:
            retValue = GetSystemPaletteEntries(hdc, (UINT)*lpvInData, 1, (LPPALETTEENTRY)lpvOutData);
            if (!retValue)
            {
                retValue = SP_ERROR;
            }
            break;

        case ENDDOC:
            /* Note: Windows checks if the handle has any user data for the ENDDOC command
             * ReactOS copies this behavior to be compatible with windows 2003
             */
            if (GdiGetDcAttr(hdc) == NULL)
            {
                GdiSetLastError(ERROR_INVALID_HANDLE);
                retValue = FALSE;
            }
            retValue = EndDoc(hdc);
            break;

        case GETSCALINGFACTOR:
            /* Note GETSCALINGFACTOR is outdated have been replace by GetDeviceCaps */
            if (ulObjType == GDI_OBJECT_TYPE_DC)
            {
                if (lpvOutData)
                {
                    PPOINT ptr = (PPOINT)lpvOutData;
                    ptr->x = 0;
                    ptr->y = 0;
                }
            }
            retValue = FALSE;
            break;

        case GETEXTENDEDTEXTMETRICS:
            retValue = GetETM(hdc, (EXTTEXTMETRIC *)lpvOutData) != 0;
            break;

        case STARTDOC:
        {
            DOCINFOA di;

            /* Note: Windows checks if the handle has any user data for the STARTDOC command
             * ReactOS copies this behavior to be compatible with windows 2003
             */
            if (GdiGetDcAttr(hdc) == NULL)
            {
                GdiSetLastError(ERROR_INVALID_HANDLE);
                retValue = FALSE;
            }

            di.cbSize = sizeof(DOCINFOA);
            di.lpszOutput = 0;
            di.lpszDatatype = 0;
            di.fwType = 0;
            di.lpszDocName = lpvInData;

            /* NOTE : doc for StartDocA/W at msdn http://msdn2.microsoft.com/en-us/library/ms535793(VS.85).aspx */
            retValue = StartDocA(hdc, &di);

            /* Check if StartDocA failed */
            if (retValue < 0)
            {
                {
                    retValue = GetLastError();

                    /* Translate StartDocA error code to STARTDOC error code
                     * see msdn http://msdn2.microsoft.com/en-us/library/ms535472.aspx
                     */
                    switch(retValue)
                    {
                    case ERROR_NOT_ENOUGH_MEMORY:
                        retValue = SP_OUTOFMEMORY;
                        break;

                    case ERROR_PRINT_CANCELLED:
                        retValue = SP_USERABORT;
                        break;

                    case ERROR_DISK_FULL:
                        retValue = SP_OUTOFDISK;
                        break;

                    default:
                        retValue = SP_ERROR;
                        break;
                    }
                }
            }
        }
        break;

        default:
            UNIMPLEMENTED;
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }

    return retValue;
}

INT
WINAPI
ExtEscape(HDC hDC,
          int nEscape,
          int cbInput,
          LPCSTR lpszInData,
          int cbOutput,
          LPSTR lpszOutData)
{
    return NtGdiExtEscape(hDC, NULL, 0, nEscape, cbInput, (LPSTR)lpszInData, cbOutput, lpszOutData);
}

INT
WINAPI
NamedEscape(HDC hdc,
            PWCHAR pDriver,
            INT iEsc,
            INT cjIn,
            LPSTR pjIn,
            INT cjOut,
            LPSTR pjOut)
{
    /* FIXME metadc, metadc are done most in user mode, and we do not support it
     * Windows 2000/XP/Vista ignore the current hdc, that are being pass and always set hdc to NULL
     * when it calls to NtGdiExtEscape from NamedEscape
     */
    return NtGdiExtEscape(NULL,pDriver,wcslen(pDriver),iEsc,cjIn,pjIn,cjOut,pjOut);
}

/*
 * @implemented
 */
int
WINAPI
DrawEscape(HDC  hDC,
           INT nEscape,
           INT cbInput,
           LPCSTR lpszInData)
{
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_DC)
        return NtGdiDrawEscape(hDC, nEscape, cbInput, (LPSTR) lpszInData);

    if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_METADC)
    {
        PLDC pLDC = GdiGetLDC(hDC);
        if ( pLDC )
        {
            if (pLDC->Flags & LDC_META_PRINT)
            {
//           if (nEscape != QUERYESCSUPPORT)
//              return EMFDRV_WriteEscape(hDC, nEscape, cbInput, lpszInData, EMR_DRAWESCAPE);

                return NtGdiDrawEscape(hDC, nEscape, cbInput, (LPSTR) lpszInData);
            }
        }
        SetLastError(ERROR_INVALID_HANDLE);
    }
    return 0;
}

#define ALPHABLEND_NONE             0
#define ALPHABLEND_BINARY           1
#define ALPHABLEND_FULL             2

typedef struct _MARGINS {
    int cxLeftWidth;
    int cxRightWidth;
    int cyTopHeight;
    int cyBottomHeight;
} MARGINS, *PMARGINS;

enum SIZINGTYPE {
    ST_TRUESIZE = 0,
    ST_STRETCH = 1,
    ST_TILE = 2,
};

#define TransparentBlt GdiTransparentBlt
#define AlphaBlend GdiAlphaBlend

/***********************************************************************
 *      UXTHEME_StretchBlt
 *
 * Pseudo TransparentBlt/StretchBlt
 */
static inline BOOL UXTHEME_StretchBlt(HDC hdcDst, int nXOriginDst, int nYOriginDst, int nWidthDst, int nHeightDst,
                                      HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc,
                                      INT transparent, COLORREF transcolor)
{
    static const BLENDFUNCTION blendFunc =
    {
      AC_SRC_OVER, /* BlendOp */
      0,           /* BlendFlag */
      255,         /* SourceConstantAlpha */
      AC_SRC_ALPHA /* AlphaFormat */
    };

    BOOL ret = TRUE;
    int old_stretch_mode;
    POINT old_brush_org;

    old_stretch_mode = SetStretchBltMode(hdcDst, HALFTONE);
    SetBrushOrgEx(hdcDst, nXOriginDst, nYOriginDst, &old_brush_org);

    if (transparent == ALPHABLEND_BINARY) {
        /* Ensure we don't pass any negative values to TransparentBlt */
        ret = TransparentBlt(hdcDst, nXOriginDst, nYOriginDst, abs(nWidthDst), abs(nHeightDst),
                              hdcSrc, nXOriginSrc, nYOriginSrc, abs(nWidthSrc), abs(nHeightSrc),
                              transcolor);
    } else if ((transparent == ALPHABLEND_NONE) ||
        !AlphaBlend(hdcDst, nXOriginDst, nYOriginDst, nWidthDst, nHeightDst,
                    hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc,
                    blendFunc))
    {
        ret = StretchBlt(hdcDst, nXOriginDst, nYOriginDst, nWidthDst, nHeightDst,
                          hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc,
                          SRCCOPY);
    }

    SetBrushOrgEx(hdcDst, old_brush_org.x, old_brush_org.y, NULL);
    SetStretchBltMode(hdcDst, old_stretch_mode);

    return ret;
}

/***********************************************************************
 *      UXTHEME_Blt
 *
 * Simplify sending same width/height for both source and dest
 */
static inline BOOL UXTHEME_Blt(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest,
                               HDC hdcSrc, int nXOriginSrc, int nYOriginSrc,
                               INT transparent, COLORREF transcolor)
{
    return UXTHEME_StretchBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest,
                              hdcSrc, nXOriginSrc, nYOriginSrc, nWidthDest, nHeightDest,
                              transparent, transcolor);
}

/***********************************************************************
 *      UXTHEME_SizedBlt
 *
 * Stretches or tiles, depending on sizingtype.
 */
static inline BOOL UXTHEME_SizedBlt (HDC hdcDst, int nXOriginDst, int nYOriginDst,
                                     int nWidthDst, int nHeightDst,
                                     HDC hdcSrc, int nXOriginSrc, int nYOriginSrc,
                                     int nWidthSrc, int nHeightSrc,
                                     int sizingtype,
                                     INT transparent, COLORREF transcolor)
{
    if (sizingtype == ST_TILE)
    {
        HDC hdcTemp;
        BOOL result = FALSE;

        if (!nWidthSrc || !nHeightSrc) return TRUE;

        /* For destination width/height less than or equal to source
           width/height, do not bother with memory bitmap optimization */
        if (nWidthSrc >= nWidthDst && nHeightSrc >= nHeightDst)
        {
            int bltWidth = min (nWidthDst, nWidthSrc);
            int bltHeight = min (nHeightDst, nHeightSrc);

            return UXTHEME_Blt (hdcDst, nXOriginDst, nYOriginDst, bltWidth, bltHeight,
                                hdcSrc, nXOriginSrc, nYOriginSrc,
                                transparent, transcolor);
        }

        /* Create a DC with a bitmap consisting of a tiling of the source
           bitmap, with standard GDI functions. This is faster than an
           iteration with UXTHEME_Blt(). */
        hdcTemp = CreateCompatibleDC(hdcSrc);
        if (hdcTemp != 0)
        {
            HBITMAP bitmapTemp;
            HBITMAP bitmapOrig;
            int nWidthTemp, nHeightTemp;
            int xOfs, xRemaining;
            int yOfs, yRemaining;
            int growSize;

            /* Calculate temp dimensions of integer multiples of source dimensions */
            nWidthTemp = ((nWidthDst + nWidthSrc - 1) / nWidthSrc) * nWidthSrc;
            nHeightTemp = ((nHeightDst + nHeightSrc - 1) / nHeightSrc) * nHeightSrc;
            bitmapTemp = CreateCompatibleBitmap(hdcSrc, nWidthTemp, nHeightTemp);
            bitmapOrig = SelectObject(hdcTemp, bitmapTemp);

            /* Initial copy of bitmap */
            BitBlt(hdcTemp, 0, 0, nWidthSrc, nHeightSrc, hdcSrc, nXOriginSrc, nYOriginSrc, SRCCOPY);

            /* Extend bitmap in the X direction. Growth of width is exponential */
            xOfs = nWidthSrc;
            xRemaining = nWidthTemp - nWidthSrc;
            growSize = nWidthSrc;
            while (xRemaining > 0)
            {
                growSize = min(growSize, xRemaining);
                BitBlt(hdcTemp, xOfs, 0, growSize, nHeightSrc, hdcTemp, 0, 0, SRCCOPY);
                xOfs += growSize;
                xRemaining -= growSize;
                growSize *= 2;
            }

            /* Extend bitmap in the Y direction. Growth of height is exponential */
            yOfs = nHeightSrc;
            yRemaining = nHeightTemp - nHeightSrc;
            growSize = nHeightSrc;
            while (yRemaining > 0)
            {
                growSize = min(growSize, yRemaining);
                BitBlt(hdcTemp, 0, yOfs, nWidthTemp, growSize, hdcTemp, 0, 0, SRCCOPY);
                yOfs += growSize;
                yRemaining -= growSize;
                growSize *= 2;
            }

            /* Use temporary hdc for source */
            result = UXTHEME_Blt (hdcDst, nXOriginDst, nYOriginDst, nWidthDst, nHeightDst,
                          hdcTemp, 0, 0,
                          transparent, transcolor);

            SelectObject(hdcTemp, bitmapOrig);
            DeleteObject(bitmapTemp);
        }
        DeleteDC(hdcTemp);
        return result;
    }
    else
    {
        return UXTHEME_StretchBlt (hdcDst, nXOriginDst, nYOriginDst, nWidthDst, nHeightDst,
                                   hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc,
                                   transparent, transcolor);
    }
}

/***********************************************************************
 *      UXTHEME_DrawImageBackground
 *
 * Draw an imagefile background
 */
static HRESULT UXTHEME_DrawImageBackground(HDC hdc, HBITMAP bmpSrc, RECT *prcSrc, INT transparent,
                                    COLORREF transparentcolor, BOOL borderonly, int sizingtype, MARGINS *psm, RECT *pRect)
{
    HRESULT hr = S_OK;
    HBITMAP bmpSrcResized = NULL;
    HGDIOBJ oldSrc;
    HDC hdcSrc, hdcOrigSrc = NULL;
    RECT rcDst;
    POINT dstSize;
    POINT srcSize;
    RECT rcSrc;
    MARGINS sm;

    rcDst = *pRect;
    rcSrc = *prcSrc;
    sm = *psm;

    hdcSrc = CreateCompatibleDC(hdc);
    if(!hdcSrc) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    dstSize.x = rcDst.right-rcDst.left;
    dstSize.y = rcDst.bottom-rcDst.top;
    srcSize.x = rcSrc.right-rcSrc.left;
    srcSize.y = rcSrc.bottom-rcSrc.top;

    if(sizingtype == ST_TRUESIZE) {
        if(!UXTHEME_StretchBlt(hdc, rcDst.left, rcDst.top, dstSize.x, dstSize.y,
                                hdcSrc, rcSrc.left, rcSrc.top, srcSize.x, srcSize.y,
                                transparent, transparentcolor))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else {
        HDC hdcDst = NULL;
        POINT org;

        dstSize.x = abs(dstSize.x);
        dstSize.y = abs(dstSize.y);

        /* Resize source image if destination smaller than margins */
#ifndef __REACTOS__
        /* Revert Wine Commit 2b650fa as it breaks themed Explorer Toolbar Separators
           FIXME: Revisit this when the bug is fixed. CORE-9636 and Wine Bug #38538 */
        if (sm.cyTopHeight + sm.cyBottomHeight > dstSize.y || sm.cxLeftWidth + sm.cxRightWidth > dstSize.x) {
            if (sm.cyTopHeight + sm.cyBottomHeight > dstSize.y) {
                sm.cyTopHeight = MulDiv(sm.cyTopHeight, dstSize.y, srcSize.y);
                sm.cyBottomHeight = dstSize.y - sm.cyTopHeight;
                srcSize.y = dstSize.y;
            }

            if (sm.cxLeftWidth + sm.cxRightWidth > dstSize.x) {
                sm.cxLeftWidth = MulDiv(sm.cxLeftWidth, dstSize.x, srcSize.x);
                sm.cxRightWidth = dstSize.x - sm.cxLeftWidth;
                srcSize.x = dstSize.x;
            }

            hdcOrigSrc = hdcSrc;
            hdcSrc = CreateCompatibleDC(NULL);
            bmpSrcResized = CreateBitmap(srcSize.x, srcSize.y, 1, 32, NULL);
            SelectObject(hdcSrc, bmpSrcResized);

            UXTHEME_StretchBlt(hdcSrc, 0, 0, srcSize.x, srcSize.y, hdcOrigSrc, rcSrc.left, rcSrc.top,
                               rcSrc.right - rcSrc.left, rcSrc.bottom - rcSrc.top, transparent, transparentcolor);

            rcSrc.left = 0;
            rcSrc.top = 0;
            rcSrc.right = srcSize.x;
            rcSrc.bottom = srcSize.y;
        }
#endif /* __REACTOS__ */

        hdcDst = hdc;
        OffsetViewportOrgEx(hdcDst, rcDst.left, rcDst.top, &org);

        /* Upper left corner */
        if(!UXTHEME_Blt(hdcDst, 0, 0, sm.cxLeftWidth, sm.cyTopHeight,
                        hdcSrc, rcSrc.left, rcSrc.top,
                        transparent, transparentcolor)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error;
        }
        /* Upper right corner */
        if(!UXTHEME_Blt (hdcDst, dstSize.x-sm.cxRightWidth, 0,
                         sm.cxRightWidth, sm.cyTopHeight,
                         hdcSrc, rcSrc.right-sm.cxRightWidth, rcSrc.top,
                         transparent, transparentcolor)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error;
        }
        /* Lower left corner */
        if(!UXTHEME_Blt (hdcDst, 0, dstSize.y-sm.cyBottomHeight,
                         sm.cxLeftWidth, sm.cyBottomHeight,
                         hdcSrc, rcSrc.left, rcSrc.bottom-sm.cyBottomHeight,
                         transparent, transparentcolor)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error;
        }
        /* Lower right corner */
        if(!UXTHEME_Blt (hdcDst, dstSize.x-sm.cxRightWidth, dstSize.y-sm.cyBottomHeight,
                         sm.cxRightWidth, sm.cyBottomHeight,
                         hdcSrc, rcSrc.right-sm.cxRightWidth, rcSrc.bottom-sm.cyBottomHeight,
                         transparent, transparentcolor)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error;
        }

        if ((sizingtype == ST_STRETCH) || (sizingtype == ST_TILE)) {
            int destCenterWidth  = dstSize.x - (sm.cxLeftWidth + sm.cxRightWidth);
            int srcCenterWidth   = srcSize.x - (sm.cxLeftWidth + sm.cxRightWidth);
            int destCenterHeight = dstSize.y - (sm.cyTopHeight + sm.cyBottomHeight);
            int srcCenterHeight  = srcSize.y - (sm.cyTopHeight + sm.cyBottomHeight);

            if(destCenterWidth > 0) {
                /* Center top */
                if(!UXTHEME_SizedBlt (hdcDst, sm.cxLeftWidth, 0,
                                      destCenterWidth, sm.cyTopHeight,
                                      hdcSrc, rcSrc.left+sm.cxLeftWidth, rcSrc.top,
                                      srcCenterWidth, sm.cyTopHeight,
                                      sizingtype, transparent, transparentcolor)) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto draw_error;
                }
                /* Center bottom */
                if(!UXTHEME_SizedBlt (hdcDst, sm.cxLeftWidth, dstSize.y-sm.cyBottomHeight,
                                      destCenterWidth, sm.cyBottomHeight,
                                      hdcSrc, rcSrc.left+sm.cxLeftWidth, rcSrc.bottom-sm.cyBottomHeight,
                                      srcCenterWidth, sm.cyBottomHeight,
                                      sizingtype, transparent, transparentcolor)) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto draw_error;
                }
            }
            if(destCenterHeight > 0) {
                /* Left center */
                if(!UXTHEME_SizedBlt (hdcDst, 0, sm.cyTopHeight,
                                      sm.cxLeftWidth, destCenterHeight,
                                      hdcSrc, rcSrc.left, rcSrc.top+sm.cyTopHeight,
                                      sm.cxLeftWidth, srcCenterHeight,
                                      sizingtype,
                                      transparent, transparentcolor)) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto draw_error;
                }
                /* Right center */
                if(!UXTHEME_SizedBlt (hdcDst, dstSize.x-sm.cxRightWidth, sm.cyTopHeight,
                                      sm.cxRightWidth, destCenterHeight,
                                      hdcSrc, rcSrc.right-sm.cxRightWidth, rcSrc.top+sm.cyTopHeight,
                                      sm.cxRightWidth, srcCenterHeight,
                                      sizingtype, transparent, transparentcolor)) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto draw_error;
                }
            }
            if(destCenterHeight > 0 && destCenterWidth > 0) {
                if(!borderonly) {
                    /* Center */
                    if(!UXTHEME_SizedBlt (hdcDst, sm.cxLeftWidth, sm.cyTopHeight,
                                          destCenterWidth, destCenterHeight,
                                          hdcSrc, rcSrc.left+sm.cxLeftWidth, rcSrc.top+sm.cyTopHeight,
                                          srcCenterWidth, srcCenterHeight,
                                          sizingtype, transparent, transparentcolor)) {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        goto draw_error;
                    }
                }
            }
        }

draw_error:
        SetViewportOrgEx (hdcDst, org.x, org.y, NULL);
    }
    SelectObject(hdcSrc, oldSrc);
    DeleteDC(hdcSrc);
    if (bmpSrcResized) DeleteObject(bmpSrcResized);
    if (hdcOrigSrc) DeleteDC(hdcOrigSrc);
    *pRect = rcDst;
    return hr;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiDrawStream(HDC dc, ULONG l, PGDI_DRAW_STREAM pDS)
{
    if (!pDS || l != sizeof(*pDS))
    {
        DPRINT1("GdiDrawStream: Invalid params\n");
        return 0;
    }

    if (pDS->signature != 0x44727753 ||
        pDS->reserved != 0 ||
        pDS->unknown1 != 1 ||
        pDS->unknown2 != 9)
    {
        DPRINT1("GdiDrawStream: Got unknown pDS data\n");
        return 0;
    }

    {
        MARGINS sm = {pDS->leftSizingMargin, pDS->rightSizingMargin, pDS->topSizingMargin, pDS->bottomSizingMargin};
        INT transparent = 0;
        int sizingtype;

        if (pDS->drawOption & DS_TRANSPARENTALPHA)
            transparent = ALPHABLEND_FULL;
        else if (pDS->drawOption & DS_TRANSPARENTCLR)
            transparent = ALPHABLEND_BINARY;
        else
            transparent = ALPHABLEND_NONE;

        if (pDS->drawOption & DS_TILE)
            sizingtype = ST_TILE;
        else if (pDS->drawOption & DS_TRUESIZE)
            sizingtype = ST_TRUESIZE;
        else
            sizingtype = ST_STRETCH;

        if (pDS->rcDest.right < pDS->rcDest.left || pDS->rcDest.bottom < pDS->rcDest.top)
            return 0;

        if (sm.cxLeftWidth + sm.cxRightWidth > pDS->rcDest.right - pDS->rcDest.left)
        {
            sm.cxLeftWidth = sm.cxRightWidth = 0;
        }

        if (sm.cyTopHeight + sm.cyBottomHeight > pDS->rcDest.bottom - pDS->rcDest.top)
        {
            sm.cyTopHeight = sm.cyBottomHeight = 0;
        }

        UXTHEME_DrawImageBackground(pDS->hDC,
                                    pDS->hImage,
                                    &pDS->rcSrc,
                                    transparent,
                                    pDS->crTransparent,
                                    FALSE,
                                    sizingtype,
                                    &sm,
                                    &pDS->rcDest);
    }
    return 0;
}


/*
 * @implemented
 */
BOOL
WINAPI
GdiValidateHandle(HGDIOBJ hobj)
{
    PGDI_TABLE_ENTRY Entry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hobj);
    if ( (Entry->Type & GDI_ENTRY_BASETYPE_MASK) != 0 &&
            ( (Entry->Type << GDI_ENTRY_UPPER_SHIFT) & GDI_HANDLE_TYPE_MASK ) ==
            GDI_HANDLE_GET_TYPE(hobj) )
    {
        HANDLE pid = (HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1);
        if(pid == NULL || pid == CurrentProcessId)
        {
            return TRUE;
        }
    }
    return FALSE;

}

/*
 * @implemented
 */
HGDIOBJ
WINAPI
GdiFixUpHandle(HGDIOBJ hGdiObj)
{
    PGDI_TABLE_ENTRY Entry;

    if (((ULONG_PTR)(hGdiObj)) & GDI_HANDLE_UPPER_MASK )
    {
        return hGdiObj;
    }

    /* FIXME is this right ?? */

    Entry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hGdiObj);

    /* Rebuild handle for Object */
    return (HGDIOBJ)(((ULONG_PTR)(hGdiObj)) | (Entry->Type << GDI_ENTRY_UPPER_SHIFT));
}

/*
 * @implemented
 */
PVOID
WINAPI
GdiQueryTable(VOID)
{
    return (PVOID)GdiHandleTable;
}

BOOL GdiGetHandleUserData(HGDIOBJ hGdiObj, DWORD ObjectType, PVOID *UserData)
{
    PGDI_TABLE_ENTRY Entry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hGdiObj);

    /* Check if twe have the correct type */
    if (GDI_HANDLE_GET_TYPE(hGdiObj) != ObjectType ||
        ((Entry->Type << GDI_ENTRY_UPPER_SHIFT) & GDI_HANDLE_TYPE_MASK) != ObjectType ||
        (Entry->Type & GDI_ENTRY_BASETYPE_MASK) != (ObjectType & GDI_ENTRY_BASETYPE_MASK))
    {
        return FALSE;
    }

    /* Check if we are the owner */
    if ((HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1) != CurrentProcessId)
    {
        return FALSE;
    }

    *UserData = Entry->UserData;
    return TRUE;
}

PLDC
FASTCALL
GdiGetLDC(HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        return NULL;
    }

    /* Return the LDC pointer */
    return pdcattr->pvLDC;
}

BOOL
FASTCALL
GdiSetLDC(HDC hdc, PVOID pvLDC)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        return FALSE;
    }

    /* Set the LDC pointer */
    pdcattr->pvLDC = pvLDC;
    return TRUE;
}


VOID GdiSAPCallback(PLDC pldc)
{
    DWORD Time, NewTime = GetTickCount();

    Time = NewTime - pldc->CallBackTick;

    if ( Time < SAPCALLBACKDELAY) return;

    pldc->CallBackTick = NewTime;

    if ( !pldc->pAbortProc(pldc->hDC, 0) )
    {
        CancelDC(pldc->hDC);
        AbortDoc(pldc->hDC);
    }
}

/*
 * @implemented
 */
DWORD
WINAPI
GdiSetBatchLimit(DWORD	Limit)
{
    DWORD OldLimit = GDI_BatchLimit;

    if ( (!Limit) ||
            (Limit >= GDI_BATCH_LIMIT))
    {
        return Limit;
    }

    GdiFlush();
    GDI_BatchLimit = Limit;
    return OldLimit;
}


/*
 * @implemented
 */
DWORD
WINAPI
GdiGetBatchLimit(VOID)
{
    return GDI_BatchLimit;
}


/*
 * @implemented
 */
VOID
WINAPI
GdiSetLastError(DWORD dwErrCode)
{
    NtCurrentTeb()->LastErrorValue = (ULONG) dwErrCode;
}

HGDIOBJ
FASTCALL
hGetPEBHandle(HANDLECACHETYPE Type, COLORREF cr)
{
    int Number, Offset, MaxNum, GdiType;
    HANDLE Lock;
    HGDIOBJ Handle = NULL;

    Lock = InterlockedCompareExchangePointer( (PVOID*)&GdiHandleCache->ulLock,
            NtCurrentTeb(),
            NULL );

    if (Lock) return Handle;

    Number = GdiHandleCache->ulNumHandles[Type];

    if (Type == hctBrushHandle)
    {
       Offset = 0;
       MaxNum = CACHE_BRUSH_ENTRIES;
       GdiType = GDILoObjType_LO_BRUSH_TYPE;
    }
    else if (Type == hctPenHandle)
    {
       Offset = CACHE_BRUSH_ENTRIES;
       MaxNum = CACHE_PEN_ENTRIES;
       GdiType = GDILoObjType_LO_PEN_TYPE;
    }
    else if (Type == hctRegionHandle)
    {
       Offset = CACHE_BRUSH_ENTRIES+CACHE_PEN_ENTRIES;
       MaxNum = CACHE_REGION_ENTRIES;
       GdiType = GDILoObjType_LO_REGION_TYPE;
    }
    else // Font is not supported here.
    {
       return Handle;
    }

    if ( Number && Number <= MaxNum )
    {
       PBRUSH_ATTR pBrush_Attr;
       HGDIOBJ *hPtr;
       hPtr = GdiHandleCache->Handle + Offset;
       Handle = hPtr[Number - 1];

       if (GdiGetHandleUserData( Handle, GdiType, (PVOID) &pBrush_Attr))
       {
          if (pBrush_Attr->AttrFlags & ATTR_CACHED)
          {
             DPRINT("Get Handle! Type %d Count %lu PEB 0x%p\n", Type, GdiHandleCache->ulNumHandles[Type], NtCurrentTeb()->ProcessEnvironmentBlock);
             pBrush_Attr->AttrFlags &= ~ATTR_CACHED;
             hPtr[Number - 1] = NULL;
             GdiHandleCache->ulNumHandles[Type]--;
             if ( Type == hctBrushHandle ) // Handle only brush.
             {
                if ( pBrush_Attr->lbColor != cr )
                {
                   pBrush_Attr->lbColor = cr ;
                   pBrush_Attr->AttrFlags |= ATTR_NEW_COLOR;
                }
             }
          }
       }
       else
       {
          Handle = NULL;
       }
    }
    (void)InterlockedExchangePointer((PVOID*)&GdiHandleCache->ulLock, Lock);
    return Handle;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
bMakePathNameW(LPWSTR lpBuffer,LPCWSTR lpFileName,LPWSTR *lpFilePart,DWORD unknown)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 * Synchronized with WINE dlls/gdi32/driver.c
 */
DEVMODEW *
WINAPI
GdiConvertToDevmodeW(const DEVMODEA *dmA)
{
    DEVMODEW *dmW;
    WORD dmW_size, dmA_size;

    dmA_size = dmA->dmSize;

    /* this is the minimal dmSize that XP accepts */
    if (dmA_size < FIELD_OFFSET(DEVMODEA, dmFields))
        return NULL;

    if (dmA_size > sizeof(DEVMODEA))
        dmA_size = sizeof(DEVMODEA);

    dmW_size = dmA_size + CCHDEVICENAME;
    if (dmA_size >= FIELD_OFFSET(DEVMODEA, dmFormName) + CCHFORMNAME)
        dmW_size += CCHFORMNAME;

    dmW = HeapAlloc(GetProcessHeap(), 0, dmW_size + dmA->dmDriverExtra);
    if (!dmW) return NULL;

    MultiByteToWideChar(CP_ACP, 0, (const char*) dmA->dmDeviceName, -1,
                                   dmW->dmDeviceName, CCHDEVICENAME);
    /* copy slightly more, to avoid long computations */
    memcpy(&dmW->dmSpecVersion, &dmA->dmSpecVersion, dmA_size - CCHDEVICENAME);

    if (dmA_size >= FIELD_OFFSET(DEVMODEA, dmFormName) + CCHFORMNAME)
    {
        if (dmA->dmFields & DM_FORMNAME)
            MultiByteToWideChar(CP_ACP, 0, (const char*) dmA->dmFormName, -1,
                                       dmW->dmFormName, CCHFORMNAME);
        else
            dmW->dmFormName[0] = 0;

        if (dmA_size > FIELD_OFFSET(DEVMODEA, dmLogPixels))
            memcpy(&dmW->dmLogPixels, &dmA->dmLogPixels, dmA_size - FIELD_OFFSET(DEVMODEA, dmLogPixels));
    }

    if (dmA->dmDriverExtra)
        memcpy((char *)dmW + dmW_size, (const char *)dmA + dmA_size, dmA->dmDriverExtra);

    dmW->dmSize = dmW_size;

    return dmW;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiRealizationInfo(HDC hdc,
                   PREALIZATION_INFO pri)
{
    // ATM we do not support local font data and Language Pack.
    return NtGdiGetRealizationInfo(hdc, pri, (HFONT) NULL);
}


/*
 * @halfplemented
 */
VOID WINAPI GdiInitializeLanguagePack(DWORD InitParam)
{
    /* Lpk function pointers to be passed to user32 */
#if 0
    FARPROC hookfuncs[4];
#endif

#ifdef LANGPACK
    if (!LoadLPK(LPK_INIT)) // no lpk found!
#endif
        return;

     /* Call InitializeLpkHooks with 4 procedure addresses
        loaded from lpk.dll but currently only one of them is currently implemented.
        Then InitializeLpkHooks (in user32) uses these to replace certain internal functions
        and ORs a DWORD being used also by ClientThreadSetup and calls
        NtUserOneParam with parameter 54 which is ONEPARAM_ROUTINE_REGISTERLPK
        which most likely changes the value of dwLpkEntryPoints in the
        PROCESSINFO struct */

#if 0
        hookfuncs[0] = GetProcAddress(hLpk, "LpkPSMTextOut");
        InitializeLpkHooks(hookfuncs);
#endif

    gbLpk = TRUE;
}

BOOL
WINAPI
GdiAddGlsBounds(HDC hdc,LPRECT prc)
{
    return NtGdiSetBoundsRect(hdc, prc, DCB_WINDOWMGR|DCB_ACCUMULATE ) ? TRUE : FALSE;
}

BOOL
WINAPI
GetBoundsRectAlt(HDC hdc,LPRECT prc,UINT flags)
{
    return NtGdiGetBoundsRect(hdc, prc, flags);
}

BOOL
WINAPI
SetBoundsRectAlt(HDC hdc,LPRECT prc,UINT flags)
{
    return NtGdiSetBoundsRect(hdc, prc, flags );
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiAddGlsRecord(HDC hdc,
                DWORD unknown1,
                LPCSTR unknown2,
                LPRECT unknown3)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

