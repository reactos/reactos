/*++

Copyright (c) 2003 Microsoft Corporation

Abstract:

    Helper functions for HIDPI/Landscape support.

--*/

#include "ms_ui_helper.h"

HIDPI_ENABLE;

BOOL HIDPI_StretchBitmap(
    HBITMAP* phbm,
    int cxDstImg,
    int cyDstImg,
    int cImagesX,
    int cImagesY
    )
{
    BOOL fRet = FALSE;
    HBITMAP hbmNew;
    BITMAP  bm;
    HDC hdcSrc, hdcDst, hdcScreen;
    HBITMAP hbmOldSrc, hbmOldDst;
    int  cxSrcImg, cySrcImg;
    int  i, j, xDest, yDest, xBmp, yBmp;

    if (!phbm || !*phbm || (cxDstImg == 0 && cyDstImg == 0) || (cImagesX == 0 || cImagesY == 0))
        goto donestretch;

    if ((sizeof(bm) != GetObject(*phbm, sizeof(bm), &bm)))
        goto donestretch;

    // If you hit this ASSERT, that mean your passed in image count in row and
    //   the column number of images is not correct.
    // ASSERT(((bm.bmWidth % cImagesX) == 0) && ((bm.bmHeight % cImagesY) == 0));

    cxSrcImg = bm.bmWidth / cImagesX;
    cySrcImg = bm.bmHeight / cImagesY;

    if (cxSrcImg == cxDstImg && cySrcImg == cyDstImg)
    {
        fRet = TRUE;
        goto donestretch;
    }

    if (cxDstImg == 0)
        cxDstImg = HIDPIMulDiv(cyDstImg, cxSrcImg, cySrcImg);
    else if (cyDstImg == 0)
        cyDstImg = HIDPIMulDiv(cxDstImg, cySrcImg, cxSrcImg);

    hdcSrc = CreateCompatibleDC(NULL);
    hdcDst = CreateCompatibleDC(NULL);
    hdcScreen = GetDC(NULL);
    hbmOldSrc = (HBITMAP)SelectObject(hdcSrc, *phbm);
    hbmNew = CreateCompatibleBitmap(hdcScreen, cxDstImg * cImagesX, cyDstImg * cImagesY);
    hbmOldDst = (HBITMAP)SelectObject(hdcDst, hbmNew);
    ReleaseDC(NULL, hdcScreen);

    // BLAST!
    for (j = 0, yDest = 0, yBmp = 0; j < cImagesY; j++, yDest += cyDstImg, yBmp += cySrcImg)
    {
        for (i = 0, xDest = 0, xBmp = 0; i < cImagesX; i++, xDest += cxDstImg, xBmp += cxSrcImg)
        {
            StretchBlt(hdcDst, xDest, yDest, cxDstImg, cyDstImg,
                       hdcSrc, xBmp, yBmp, cxSrcImg, cySrcImg,
                       SRCCOPY);
        }
    }

    // Free allocated memory
    SelectObject(hdcSrc, hbmOldSrc);
    SelectObject(hdcDst, hbmOldDst);
    DeleteDC(hdcSrc);
    DeleteDC(hdcDst);

    // Delete the passed in bitmap
    DeleteObject(*phbm);
    *phbm = hbmNew;

    fRet = TRUE;

donestretch:
    return fRet;
}

static BOOL HIDPI_StretchIcon_Internal(
    HICON hiconIn,
    HICON* phiconOut,
    int cxIcon,
    int cyIcon
)
{
    ICONINFO iconinfo;

    HDC hdc;
    HBITMAP hbmImage, hbmMask;
    HBITMAP hbmOld;
    BOOL fDrawMaskOK;
    BOOL fDrawImageOK;

    *phiconOut = NULL;
    hdc = CreateCompatibleDC(NULL);

    hbmMask = CreateCompatibleBitmap(hdc, cxIcon, cyIcon);
    hbmOld = (HBITMAP)SelectObject(hdc, hbmMask);
    fDrawMaskOK = DrawIconEx(hdc, 0, 0, hiconIn, cxIcon, cyIcon, 0, NULL, DI_MASK);
    SelectObject(hdc, hbmOld);

    hbmImage = CreateBitmap(cxIcon, cyIcon, 1, GetDeviceCaps(hdc, BITSPIXEL), NULL);
    hbmOld = (HBITMAP)SelectObject(hdc, hbmImage);
    fDrawImageOK = DrawIconEx(hdc, 0, 0, hiconIn, cxIcon, cyIcon, 0, NULL, DI_IMAGE);
    SelectObject(hdc, hbmOld);

    if (fDrawImageOK && fDrawMaskOK)
    {
        iconinfo.fIcon = TRUE;
        iconinfo.hbmColor = hbmImage;
        iconinfo.hbmMask = hbmMask;
        *phiconOut = CreateIconIndirect(&iconinfo);
    }

    DeleteObject(hbmImage);
    DeleteObject(hbmMask);

    DeleteDC(hdc);

    return (fDrawImageOK && fDrawMaskOK && *phiconOut != NULL) ? TRUE : FALSE;
}

BOOL HIDPI_StretchIcon(
    HICON* phic,
    int cxIcon,
    int cyIcon
)
{
    HICON hiconOut;

    if (HIDPI_StretchIcon_Internal(*phic, &hiconOut, cxIcon, cyIcon))
    {
        DestroyIcon(*phic);
        *phic = hiconOut;
        return TRUE;
    }

    return FALSE;
}


BOOL HIDPI_GetBitmapLogPixels(
    HINSTANCE hinst,
    LPCTSTR lpbmp,
    int* pnLogPixelsX,
    int* pnLogPixelsY
    )
{
    BOOL fRet = FALSE;
    HRSRC hResource;
    HGLOBAL hResourceBitmap = NULL;
    BITMAPINFO* pBitmapInfo;
    int PelsPerMeterX, PelsPerMeterY;

    *pnLogPixelsX = 0;
    *pnLogPixelsY = 0;

    hResource = FindResource(hinst, lpbmp, RT_BITMAP);
    if (!hResource)
    {
        goto error;
    }
    hResourceBitmap = LoadResource(hinst, hResource);
    if (!hResourceBitmap)
    {
        goto error;
    }
    pBitmapInfo = (BITMAPINFO*)LockResource(hResourceBitmap);
    if (!pBitmapInfo)
    {
        goto error;
    }

    // There are at least three kind value of PslsPerMeter used for 96 DPI bitmap:
    //   0    - the bitmap just simply doesn't set this value
    //   2834 - 72 DPI
    //   3780 - 96 DPI
    // So any value of PslsPerMeter under 3780 should be treated as 96 DPI bitmap.
    PelsPerMeterX = (pBitmapInfo->bmiHeader.biXPelsPerMeter < 3780) ? 3780 : pBitmapInfo->bmiHeader.biXPelsPerMeter;
    PelsPerMeterY = (pBitmapInfo->bmiHeader.biYPelsPerMeter < 3780) ? 3780 : pBitmapInfo->bmiHeader.biYPelsPerMeter;

    // The formula for converting PelsPerMeter to LogPixels(DPI) is:
    //   LogPixels = PelsPerMeter / 39.37
    //   ( PelsPerMeter : Pixels per meter )
    //   ( LogPixels    : Pixels per inch  )
    // Note: We need to round up.
    *pnLogPixelsX = (int)((PelsPerMeterX * 100 + 1968) / 3937);
    *pnLogPixelsY = (int)((PelsPerMeterY * 100 + 1968) / 3937);

    fRet = TRUE;

error:
    return fRet;
}


HIMAGELIST HIDPI_ImageList_LoadImage(
    HINSTANCE hinst,
    LPCTSTR lpbmp,
    int cx,
    int cGrow,
    COLORREF crMask,
    UINT uType,
    UINT uFlags
    )
{
    HBITMAP hbmImage = NULL;
    HIMAGELIST piml = NULL;
    BITMAP bm;
    int cImages, cxImage, cy;
    int BmpLogPixelsX, BmpLogPixelsY;
    UINT flags;

    if ((uType != IMAGE_BITMAP) ||  // Image type is not IMAGE_BITMAP
        (cx == 0))                  // Caller doesn't care about the dimensions of the image - assumes the ones in the file
    {
        piml = ImageList_LoadImage(hinst, lpbmp, cx, cGrow, crMask, uType, uFlags);
        goto cleanup;
    }

    if (!HIDPI_GetBitmapLogPixels(hinst, lpbmp, &BmpLogPixelsX, &BmpLogPixelsY))
    {
        goto cleanup;
    }

    hbmImage = (HBITMAP)LoadImage(hinst, lpbmp, uType, 0, 0, uFlags);
    if (!hbmImage || (sizeof(bm) != GetObject(hbmImage, sizeof(bm), &bm)))
    {
        goto cleanup;
    }

    // do we need to scale this image?
    if (BmpLogPixelsX == g_HIDPI_LogPixelsX)
    {
        piml = ImageList_LoadImage(hinst, lpbmp, cx, cGrow, crMask, uType, uFlags);
        goto cleanup;
    }

    cxImage = HIDPIMulDiv(cx, BmpLogPixelsX, g_HIDPI_LogPixelsX);

    // Bitmap width should be multiple integral of image width.
    // If not, that means either your bitmap is wrong or passed in cx is wrong.
    // ASSERT((bm.bmWidth % cxImage) == 0);

    cImages = bm.bmWidth / cxImage;

    cy = HIDPIMulDiv(bm.bmHeight, g_HIDPI_LogPixelsY, BmpLogPixelsY);

    if ((g_HIDPI_LogPixelsX % BmpLogPixelsX) == 0)
    {
        HIDPI_StretchBitmap(&hbmImage, cx * cImages, cy, 1, 1);
    }
    else
    {
        // Here means the DPI is not integral multiple of standard DPI (96DPI).
        // So if we stretch entire bitmap together, we are not sure each indivisual
        //   image will be stretch to right place. It is controled by StretchBlt().
        //   (for example, a 16 pixel icon, the first one might be stretch to 22 pixels
        //    and next one might be stretched to 20 pixels)
        // What we have to do here is stretching indivisual image separately to make sure
        //   every one is stretched properly.
        HIDPI_StretchBitmap(&hbmImage, cx, cy, cImages, 1);
    }

    flags = 0;
    // ILC_MASK is important for supporting CLR_DEFAULT
    if (crMask != CLR_NONE)
    {
        flags |= ILC_MASK;
    }
    // ILC_COLORMASK bits are important if we ever want to Merge ImageLists
    if (bm.bmBits)
    {
        flags |= (bm.bmBitsPixel & ILC_COLORMASK);
    }

    // bitmap MUST be de-selected from the DC
    // create the image list of the size asked for.
    piml = ImageList_Create(cx, cy, flags, cImages, cGrow);

    if (piml)
    {
        int added;

        if (crMask == CLR_NONE)
        {
            added = ImageList_Add(piml, hbmImage, NULL);
        }
        else
        {
            added = ImageList_AddMasked(piml, hbmImage, crMask);
        }

        if (added < 0)
        {
            ImageList_Destroy(piml);
            piml = NULL;
        }
    }

cleanup:
    DeleteObject(hbmImage);
    return piml;
}

int HIDPI_ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon)
{
    int iRet;
    int cxIcon, cyIcon;
    HICON hiconStretched;

    ImageList_GetIconSize(himl, &cxIcon, &cyIcon);
    HIDPI_StretchIcon_Internal(hicon, &hiconStretched, cxIcon, cyIcon);
    if (hiconStretched != NULL)
    {
        iRet = ImageList_ReplaceIcon(himl, i, hiconStretched);
        DestroyIcon(hiconStretched);
    }
    else
    {
        iRet = ImageList_ReplaceIcon(himl, i, hicon);
    }

    return iRet;
}

BOOL HIDPI_RectangleInternal(HDC hdc, int nLeft, int nTop, int nRight, int nBottom, int nThickness)
{
    int nOff = nThickness/2;

    nLeft   += nOff;
    nTop    += nOff;
    nRight  -= nOff;
    nBottom -= nOff;

    return Rectangle(hdc, nLeft, nTop, nRight, nBottom);
}

#define BORDERX_PEN 32

BOOL HIDPI_BorderRectangle(HDC hdc, int nLeft, int nTop, int nRight, int nBottom)
{
    HPEN hpenOld;
    BOOL bRet;

    hpenOld = (HPEN)SelectObject(hdc, (HPEN) GetStockObject(BORDERX_PEN));
    bRet = HIDPI_RectangleInternal(hdc, nLeft, nTop, nRight, nBottom, GetSystemMetrics(SM_CXBORDER));
    SelectObject(hdc, hpenOld);

    return bRet;
}

BOOL HIDPI_Rectangle(HDC hdc, int nLeft, int nTop, int nRight, int nBottom)
{
    LOGPEN lpenSel;
    HPEN hpenSel;

    // Obtain current pen thickness
    hpenSel = (HPEN)GetCurrentObject(hdc, OBJ_PEN);
    GetObject(hpenSel, sizeof(lpenSel), &lpenSel);

    return HIDPI_RectangleInternal(hdc, nLeft, nTop, nRight, nBottom, lpenSel.lopnWidth.x);
}


BOOL HIDPI_PolylineInternal(HDC hdc, const POINT *lppt, int cPoints, int nStyle, int nThickness)
{
    int i;
    int nHOff = 0, nVOff = 0;
    BOOL bRet = TRUE;
    POINT pts[2];

    if (! (nStyle & PS_BIAS_MASK))
    {
        // No drawing bias. Draw normally
        return Polyline(hdc, lppt, cPoints);
    }

    // Make sure caller didn't try to get both a left and a right bias or both a down and an up bias
    // ASSERT(!(nStyle & PS_LEFTBIAS) || !(nStyle & PS_RIGHTBIAS));
    // ASSERT(!(nStyle & PS_UPBIAS) || !(nStyle & PS_DOWNBIAS));

    if (nStyle & PS_LEFTBIAS)
    {
        nHOff = -((nThickness-1)/2);
    }

    if (nStyle & PS_RIGHTBIAS)
    {
        nHOff = nThickness/2;
    }

    if (nStyle & PS_UPBIAS)
    {
        nVOff = -((nThickness-1)/2);
    }

    if (nStyle & PS_DOWNBIAS)
    {
        nVOff = nThickness/2;
    }

    for (i = 1; i < cPoints; i++)
    {
        // Use the two points that specify current line segment
        memcpy(pts, &lppt[i-1], 2*sizeof(POINT));
        if (abs(lppt[i].x - lppt[i-1].x) <= abs(lppt[i].y - lppt[i-1].y))
        {
            // Shift current line segment horizontally if abs(slope) >= 1
            pts[0].x += nHOff;
            pts[1].x += nHOff;
        }
        else
        {
            // Shift current line segment vertically if abs(slope) < 1
            pts[0].y += nVOff;
            pts[1].y += nVOff;
        }
        bRet = bRet && Polyline(hdc, pts, 2);
        if (!bRet)
        {
            goto Error;
        }
    }

Error:
    return bRet;
}

BOOL HIDPI_BorderPolyline(HDC hdc, const POINT *lppt, int cPoints, int nStyle)
{
    HPEN hpenOld;
    BOOL bRet;

    hpenOld = (HPEN)SelectObject(hdc, (HPEN) GetStockObject(BORDERX_PEN));
    bRet = HIDPI_PolylineInternal(hdc, lppt, cPoints, nStyle,  GetSystemMetrics(SM_CXBORDER));
    SelectObject(hdc, hpenOld);

    return bRet;
}

BOOL HIDPI_Polyline(HDC hdc, const POINT *lppt, int cPoints, int nStyle)
{
    LOGPEN lpenSel;
    HPEN hpenSel;

    // Obtain current pen thickness
    hpenSel = (HPEN)GetCurrentObject(hdc, OBJ_PEN);
    GetObject(hpenSel, sizeof(lpenSel), &lpenSel);

    return HIDPI_PolylineInternal(hdc, lppt, cPoints, nStyle, lpenSel.lopnWidth.x);
}

//
// Called by RelayoutDialog to advance to the next item in the dialog template.
//
static LPBYTE WalkDialogData(LPBYTE lpData)
{
    LPWORD lpWord = (LPWORD)lpData;
    if (*lpWord == 0xFFFF)
    {
        return (LPBYTE)(lpWord + 2);
    }
    while (*lpWord != 0x0000)
    {
        lpWord++;
    }
    return (LPBYTE)(lpWord + 1);
}

//
// Post-processing step for each dialog item.
//    Static controls and buttons: change text and bitmaps.
//    Listboxes and combo boxes: ensures that the selected item is visible.
//
static void FixupDialogItem(
    HINSTANCE hInst,
    HWND hDlg,
    LPDLGITEMTEMPLATE lpDlgItem,
    LPWORD lpClass,
    LPWORD lpData)
{
    if (lpClass[0] == 0xFFFF)
    {
        switch (lpClass[1])
        {
            case 0x0080: // button
            case 0x0082: // static
            {
                if (lpData[0] == 0xFFFF)
                {
                    if (lpDlgItem->style & SS_ICON)
                    {
                        HICON hOld = (HICON)SendDlgItemMessageW(hDlg, lpDlgItem->id, STM_GETIMAGE, IMAGE_ICON, 0);
                        HICON hNew = LoadIcon(hInst, MAKEINTRESOURCE(lpData[1]));
                        SendDlgItemMessageW(hDlg, lpDlgItem->id, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hNew);
                        DestroyIcon(hOld);
                    }
                    else if (lpDlgItem->style & SS_BITMAP)
                    {
                        HBITMAP hOld = (HBITMAP)SendDlgItemMessageW(hDlg, lpDlgItem->id, STM_GETIMAGE, IMAGE_BITMAP, 0);
                        HBITMAP hNew = LoadBitmap(hInst, MAKEINTRESOURCE(lpData[1]));
                        SendDlgItemMessageW(hDlg, lpDlgItem->id, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hNew);
                        DeleteObject(hOld);
                    }
                }
                else // lpData[0] is not 0xFFFF (it's text).
                {
                    SetDlgItemTextW(hDlg, lpDlgItem->id, (LPCTSTR)lpData);
                }
            }
            break;

            case 0x0083: // list box
            {
                INT nSel = SendDlgItemMessageW(hDlg, lpDlgItem->id, LB_GETCURSEL, 0, 0);
                if (nSel != LB_ERR)
                {
                    SendDlgItemMessageW(hDlg, lpDlgItem->id, LB_SETCURSEL, nSel, 0);
                }
            }
            break;

            case 0x0085: // combo box
            {
                INT nSel = SendDlgItemMessageW(hDlg, lpDlgItem->id, CB_GETCURSEL, 0, 0);
                if (nSel != CB_ERR)
                {
                    SendDlgItemMessageW(hDlg, lpDlgItem->id, CB_SETCURSEL, nSel, 0);
                }
            }
            break;
        }
    }
}

BOOL RelayoutDialog(HINSTANCE hInst, HWND hDlg, LPCWSTR iddTemplate)
{
    HRSRC hRsrc = FindResource((HMODULE)hInst, iddTemplate, RT_DIALOG);
    INT nStatics = 0;
    HGLOBAL hGlobal;
    LPBYTE lpData;
    LPDLGTEMPLATE lpTemplate;
    HDWP hDWP;
    int i;
    LPDLGITEMTEMPLATE lpDlgItem;
    HWND hwndCtl;
    LPWORD lpClass;
    WORD cbExtra;

    if (hRsrc == NULL)
    {
        return FALSE;
    }

    hGlobal = LoadResource((HMODULE)hInst, hRsrc);
    if (hGlobal == NULL)
    {
        return FALSE;
    }

    lpData = (LPBYTE)LockResource(hGlobal);
    lpTemplate = (LPDLGTEMPLATE)lpData;
    hDWP = BeginDeferWindowPos(lpTemplate->cdit);

    //
    // For more information about the data structures that we are walking,
    // consult the DLGTEMPLATE and DLGITEMTEMPLATE documentation on MSDN.
    //
    lpData += sizeof(DLGTEMPLATE);
    lpData = WalkDialogData(lpData);     // menu
    lpData = WalkDialogData(lpData);     // class
    lpData = WalkDialogData(lpData);     // title

    if (lpTemplate->style & DS_SETFONT)
    {
        lpData += sizeof(WORD);          // font size.
        lpData = WalkDialogData(lpData); // font face.
    }

    for (i = 0; i < lpTemplate->cdit; i++)
    {
        lpData = (LPBYTE) (((INT)lpData + 3) & ~3);  // force to DWORD boundary.
        lpDlgItem = (LPDLGITEMTEMPLATE)lpData;
        hwndCtl = GetDlgItem(hDlg, lpDlgItem->id);

        if (lpDlgItem->id == 0xFFFF)
        {
            nStatics++;
        }

        //
        // Move the item around.
        //
        {
            RECT r;
            r.left   = lpDlgItem->x;
            r.top    = lpDlgItem->y;
            r.right  = lpDlgItem->x + lpDlgItem->cx;
            r.bottom = lpDlgItem->y + lpDlgItem->cy;
            MapDialogRect(hDlg, &r);
            DeferWindowPos(hDWP, hwndCtl, NULL,
                r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER);
        }

        lpData += sizeof(DLGITEMTEMPLATE);
        lpClass = (LPWORD)lpData;
        lpData = WalkDialogData(lpData);  // class

        //
        // Do some special handling for each dialog item (changing text,
        // bitmaps, ensuring visible, etc.
        //
        FixupDialogItem(hInst, hDlg, lpDlgItem, lpClass, (LPWORD)lpData);

        lpData = WalkDialogData(lpData);  // title
        cbExtra = *((LPWORD)lpData); // extra class data.
        lpData += (cbExtra ? cbExtra : sizeof(WORD));
    }

    EndDeferWindowPos(hDWP);
    return nStatics < 2 ? TRUE : FALSE;
}
