//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       General control utility functions
//
//-------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_CTRLUTIL_HXX_
#define X_CTRLUTIL_HXX_
#include "ctrlutil.hxx"
#endif

DWORD
FormsDrawDiagonalLine(HDC hdc, LPRECT lprc, int iDirection, int iThickness, UINT flags)
{
    RECT    rc;
    LPINT   py;
    int     cx;
    int     cy;
    int     dx;
    int     dy;
    LPINT   pc;
    int     cxBorder = GetSystemMetrics(SM_CXBORDER);
    int     cyBorder = GetSystemMetrics(SM_CYBORDER);

    if (IsRectEmpty(lprc))
        return(0L);

    CopyRect(&rc, lprc);

    //
    // We draw slopes < 1 by varying y instead of x.
    //
    --iThickness;

    // HACK HACK HACK. REMOVE THIS ONCE MARLETT IS AROUND
    cy = rc.bottom - rc.top;
    cx = rc.right - rc.left;

    if (!flags && (cy != cx))
        cy -= iThickness * cyBorder;

    if (cy >= cx)
    {
        // "slope" is >= 1, so vary x by 1
        cy /= cx;
        pc = &cy;

        cx = cxBorder;
    }
    else
    {
        // "slope" is < 1, so vary y by 1
        cx /= cy;
        pc = &cx;

        cy = cyBorder;
    }

    dx = cx;
    dy = iDirection * cy;

    *pc = (*pc + iThickness) * cyBorder;

    rc.right -= cx;
    rc.bottom -= cy;

    // For negative slopes, start from opposite side.
    if (iDirection < 0)
        py = (int *)&rc.top;
    else
        py = (int *)&rc.bottom;

    while ((rc.left <= rc.right) && (rc.top <= rc.bottom))
    {
        if (!(flags & BF_MIDDLE))
            PatBlt(hdc, rc.left, *py, cx, cy, PATCOPY);
        else
        {
            // Fill interior.  We can determine vertex in interior
            // by vector define.

            if (cy > cyBorder)
            {
                if (flags & BF_LEFT)
                    PatBlt(hdc, rc.left, lprc->top, cx, *py - lprc->top + cy, PATCOPY);
                else
                    PatBlt(hdc, rc.left, *py, cx, lprc->bottom - *py, PATCOPY);
            }
            else
            {
                if (flags & BF_TOP)
                    PatBlt(hdc, rc.left, *py, lprc->right - rc.left, cy, PATCOPY);
                else
                    PatBlt(hdc, lprc->left, *py, rc.left - lprc->left + cx, cy, PATCOPY);
            }
        }

        rc.left += dx;
        *py -= dy;
    }

    return(MAKELONG(cx, cy));
}


BOOL
FormsDrawCheckMark(HDC hdc, LPRECT lprc, int iThickness)
{
    int i = (lprc->right - lprc->left + 2) / 3;
    int saveRight = lprc->right;

    lprc->top += ((lprc->bottom - lprc->top - (i * 2)) / 2) + i - 1;
    lprc->bottom = lprc->top + i + (iThickness - 1);

    lprc->left += (lprc->right - lprc->left - ((i * 3) - 1)) / 2;

    lprc->right = lprc->left + i - 1;
    FormsDrawDiagonalLine(hdc, lprc, -1, iThickness, 0);
    lprc->top -= i;
    lprc->left = lprc->right;
    lprc->right = saveRight;
    FormsDrawDiagonalLine(hdc, lprc, 1, iThickness, 0);
    return(TRUE);
}


const int HAIRLINE_IN_HIMETRICS = 26;

BOOL
FormsDrawGlyph(CDrawInfo * pDI, LPGDIRECT prc, UINT wType, UINT wState)
{
    COLORREF crSaveBkColor=0;           // save background color
    COLORREF crSaveFrColor=0;           // save foreground color
    INT nTechnology     = GetDeviceCaps(pDI->_hdc, TECHNOLOGY);
    INT nNumColors      = GetDeviceCaps(pDI->_hdc, NUMCOLORS);
    DWORD dwDCObjType   = GetObjectType(pDI->_hdc);
    HDC     hdcMem      = NULL;
    HBITMAP hbmMem      = NULL;
    HBITMAP oldhbmMem   = NULL;

    int nHeight, nWidth;

    // this function only handles buttons...
    Assert (wType == DFC_BUTTON);


    nHeight = prc->bottom - prc->top;
    nWidth = prc->right - prc->left;

    switch (nTechnology)
    {
// WINCEREVIEW: no support for  DT_METAFILE:
#ifndef WINCE
    case DT_METAFILE:
        Assert(dwDCObjType == OBJ_ENHMETADC || dwDCObjType == OBJ_METADC);
#endif // WINCE
    case DT_RASPRINTER:
    case DT_PLOTTER:
        if (nNumColors == 2)                            // if this is a black and white printer
        {
            crSaveBkColor = GetBkColor(pDI->_hdc);      // save the back color
            crSaveFrColor = GetTextColor(pDI->_hdc);    // save the fore color
            SetBkColor(pDI->_hdc, RGB(255, 255, 255));
            SetTextColor(pDI->_hdc, RGB(0, 0, 0));
        }

        switch(dwDCObjType)
        {
        case    OBJ_ENHMETADC:
// WINCEREVIEW: no support for  DT_METAFILE:
#ifndef WINCE
        case    OBJ_METADC:
#endif // WINCE
            if (nNumColors == 2)                        // if this is a black and white printer
            {
                hdcMem = CreateCompatibleDC(TLS(hdcDesktop));
                if (hdcMem)
                {
                    hbmMem = CreateCompatibleBitmap(TLS(hdcDesktop), nWidth, nHeight);
                    if (hbmMem)
                    {
                        GDIRECT rcMem;

                        rcMem.top = rcMem.left = 0;
                        rcMem.right = nWidth;
                        rcMem.bottom = nHeight;

                        oldhbmMem = (HBITMAP)SelectObject(hdcMem, hbmMem);
                        BitBlt(hdcMem, 0, 0, nWidth, nHeight, hdcMem, 0, 0, WHITENESS);
                        BitBlt(pDI->_hdc, prc->left, prc->top,
                                            nWidth, nHeight, pDI->_hdc, 0, 0, WHITENESS);
                        DrawFrameControl(hdcMem, &rcMem, wType, wState);

                        BitBlt(pDI->_hdc, prc->left, prc->top,
                                            nWidth, nHeight, hdcMem, 0, 0, SRCCOPY);
                        SelectObject(hdcMem, oldhbmMem);
                        DeleteDC(hdcMem);
                        DeleteObject(hbmMem);
                        DeleteObject(oldhbmMem);
                        break;
                    }
                    DeleteDC(hdcMem);
                }
            }

            // if something wrong, fall through
        default:
            DrawFrameControl(pDI->_hdc, prc, wType, wState);
        }
        if (nNumColors == 2)                            // if this is a black and white printer
        {
          SetBkColor(pDI->_hdc, crSaveBkColor);       // restore the back color
          SetTextColor(pDI->_hdc, crSaveFrColor);     // restore the fore color
        }
        break;
    default:
        DrawFrameControl(pDI->_hdc, prc, wType, wState);
    } // end of swtich

    return TRUE;
}
