/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/selectionmodel.cpp
 * PURPOSE:     Keep track of selection parameters, notify listeners
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

SelectionModel::SelectionModel()
    : m_hDC(CreateCompatibleDC(NULL))
    , m_hBm(NULL)
    , m_hMask(NULL)
    , m_ptStack(NULL)
    , m_iPtSP(0)
{
    SetRectEmpty(&m_rcSrc);
    SetRectEmpty(&m_rcDest);
}

SelectionModel::~SelectionModel()
{
    DeleteDC(m_hDC);
    ResetPtStack();
    if (m_hBm)
    {
        DeleteObject(m_hBm);
    }
    if (m_hMask)
    {
        DeleteObject(m_hMask);
    }
}

void SelectionModel::ResetPtStack()
{
    if (m_ptStack != NULL)
        HeapFree(GetProcessHeap(), 0, m_ptStack);
    m_ptStack = NULL;
    m_iPtSP = 0;
}

void SelectionModel::PushToPtStack(LONG x, LONG y)
{
    if (m_iPtSP % 1024 == 0)
    {
        if (m_ptStack)
            m_ptStack = (POINT*) HeapReAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, m_ptStack, sizeof(POINT) * (m_iPtSP + 1024));
        else
            m_ptStack = (POINT*) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(POINT) * 1024);
    }
    m_ptStack[m_iPtSP].x = x;
    m_ptStack[m_iPtSP].y = y;
    m_iPtSP++;
}

void SelectionModel::CalculateBoundingBoxAndContents(HDC hDCImage)
{
    int i;
    m_rcSrc.left = m_rcSrc.top = MAXLONG;
    m_rcSrc.right = m_rcSrc.bottom = 0;
    for (i = 0; i < m_iPtSP; i++)
    {
        if (m_ptStack[i].x < m_rcSrc.left)
            m_rcSrc.left = m_ptStack[i].x;
        if (m_ptStack[i].y < m_rcSrc.top)
            m_rcSrc.top = m_ptStack[i].y;
        if (m_ptStack[i].x > m_rcSrc.right)
            m_rcSrc.right = m_ptStack[i].x;
        if (m_ptStack[i].y > m_rcSrc.bottom)
            m_rcSrc.bottom = m_ptStack[i].y;
    }
    m_rcSrc.right  += 1;
    m_rcSrc.bottom += 1;
    m_rcDest.left   = m_rcSrc.left;
    m_rcDest.top    = m_rcSrc.top;
    m_rcDest.right  = m_rcSrc.right;
    m_rcDest.bottom = m_rcSrc.bottom;

    if (m_iPtSP > 1)
    {
        DeleteObject(m_hMask);
        m_hMask = CreateBitmap(RECT_WIDTH(m_rcSrc), RECT_HEIGHT(m_rcSrc), 1, 1, NULL);
        DeleteObject(SelectObject(m_hDC, m_hMask));
        POINT *m_ptStackCopy = (POINT*) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(POINT) * m_iPtSP);
        for (i = 0; i < m_iPtSP; i++)
        {
            m_ptStackCopy[i].x = m_ptStack[i].x - m_rcSrc.left;
            m_ptStackCopy[i].y = m_ptStack[i].y - m_rcSrc.top;
        }
        Poly(m_hDC, m_ptStackCopy, m_iPtSP, 0x00ffffff, 0x00ffffff, 1, 2, TRUE, FALSE);
        HeapFree(GetProcessHeap(), 0, m_ptStackCopy);
        SelectObject(m_hDC, m_hBm = CreateDIBWithProperties(RECT_WIDTH(m_rcSrc), RECT_HEIGHT(m_rcSrc)));
        imageModel.ResetToPrevious();
        MaskBlt(m_hDC, 0, 0, RECT_WIDTH(m_rcSrc), RECT_HEIGHT(m_rcSrc), hDCImage, m_rcSrc.left,
                m_rcSrc.top, m_hMask, 0, 0, MAKEROP4(SRCCOPY, WHITENESS));
    }
}

void SelectionModel::CalculateContents(HDC hDCImage)
{
    DeleteObject(m_hMask);
    m_hMask = CreateBitmap(RECT_WIDTH(m_rcSrc), RECT_HEIGHT(m_rcSrc), 1, 1, NULL);
    DeleteObject(SelectObject(m_hDC, m_hMask));
    Rect(m_hDC, 0, 0, RECT_WIDTH(m_rcSrc), RECT_HEIGHT(m_rcSrc), 0x00ffffff, 0x00ffffff, 1, 2);
    SelectObject(m_hDC, m_hBm = CreateDIBWithProperties(RECT_WIDTH(m_rcSrc), RECT_HEIGHT(m_rcSrc)));
    BitBlt(m_hDC, 0, 0, RECT_WIDTH(m_rcSrc), RECT_HEIGHT(m_rcSrc), hDCImage, m_rcSrc.left,
           m_rcSrc.top, SRCCOPY);
}

void SelectionModel::DrawBackgroundPoly(HDC hDCImage, COLORREF crBg)
{
    Poly(hDCImage, m_ptStack, m_iPtSP, crBg, crBg, 1, 2, TRUE, FALSE);
}

void SelectionModel::DrawBackgroundRect(HDC hDCImage, COLORREF crBg)
{
    Rect(hDCImage, m_rcSrc.left, m_rcSrc.top, m_rcSrc.right, m_rcSrc.bottom, crBg, crBg, 0, 1);
}

extern BOOL
ColorKeyedMaskBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, HBITMAP hbmMask, int xMask, int yMask, DWORD dwRop, COLORREF keyColor);

void SelectionModel::DrawSelection(HDC hDCImage, COLORREF crBg, BOOL bBgTransparent)
{
    if (!bBgTransparent)
        MaskBlt(hDCImage, m_rcDest.left, m_rcDest.top, RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest),
                m_hDC, 0, 0, m_hMask, 0, 0, MAKEROP4(SRCCOPY, SRCAND));
    else
        ColorKeyedMaskBlt(hDCImage, m_rcDest.left, m_rcDest.top, RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest),
                          m_hDC, 0, 0, m_hMask, 0, 0, MAKEROP4(SRCCOPY, SRCAND), crBg);
}

void SelectionModel::DrawSelectionStretched(HDC hDCImage)
{
    StretchBlt(hDCImage, m_rcDest.left, m_rcDest.top, RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), m_hDC, 0, 0, GetDIBWidth(m_hBm), GetDIBHeight(m_hBm), SRCCOPY);
}

void SelectionModel::ScaleContentsToFit()
{
    HDC hTempDC;
    HBITMAP hTempBm;
    hTempDC = CreateCompatibleDC(m_hDC);
    hTempBm = CreateDIBWithProperties(RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest));
    SelectObject(hTempDC, hTempBm);
    SelectObject(m_hDC, m_hBm);
    StretchBlt(hTempDC, 0, 0, RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), m_hDC, 0, 0,
               GetDIBWidth(m_hBm), GetDIBHeight(m_hBm), SRCCOPY);
    DeleteObject(m_hBm);
    m_hBm = hTempBm;
    hTempBm = CreateBitmap(RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), 1, 1, NULL);
    SelectObject(hTempDC, hTempBm);
    SelectObject(m_hDC, m_hMask);
    StretchBlt(hTempDC, 0, 0, RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), m_hDC, 0, 0,
               GetDIBWidth(m_hMask), GetDIBHeight(m_hMask), SRCCOPY);
    DeleteObject(m_hMask);
    m_hMask = hTempBm;
    SelectObject(m_hDC, m_hBm);
    DeleteDC(hTempDC);
}

void SelectionModel::InsertFromHBITMAP(HBITMAP hBm)
{
    HDC hTempDC;
    HBITMAP hTempMask;

    DeleteObject(SelectObject(m_hDC, m_hBm = (HBITMAP) CopyImage(hBm,
                                                                 IMAGE_BITMAP, 0, 0,
                                                                 LR_COPYRETURNORG)));

    SetRectEmpty(&m_rcSrc);
    m_rcDest.left = m_rcDest.top = 0;
    m_rcDest.right = m_rcDest.left + GetDIBWidth(m_hBm);
    m_rcDest.bottom = m_rcDest.top + GetDIBHeight(m_hBm);

    hTempDC = CreateCompatibleDC(m_hDC);
    hTempMask = CreateBitmap(RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), 1, 1, NULL);
    SelectObject(hTempDC, hTempMask);
    Rect(hTempDC, m_rcDest.left, m_rcDest.top, m_rcDest.right, m_rcDest.bottom, 0x00ffffff, 0x00ffffff, 1, 1);
    DeleteObject(m_hMask);
    m_hMask = hTempMask;
    DeleteDC(hTempDC);
}

void SelectionModel::FlipHorizontally()
{
    SelectObject(m_hDC, m_hMask);
    StretchBlt(m_hDC, RECT_WIDTH(m_rcDest) - 1, 0, -RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), m_hDC,
               0, 0, RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), SRCCOPY);
    SelectObject(m_hDC, m_hBm);
    StretchBlt(m_hDC, RECT_WIDTH(m_rcDest) - 1, 0, -RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), m_hDC,
               0, 0, RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), SRCCOPY);
    NotifyRefreshNeeded();
}

void SelectionModel::FlipVertically()
{
    SelectObject(m_hDC, m_hMask);
    StretchBlt(m_hDC, 0, RECT_HEIGHT(m_rcDest) - 1, RECT_WIDTH(m_rcDest), -RECT_HEIGHT(m_rcDest), m_hDC,
               0, 0, RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), SRCCOPY);
    SelectObject(m_hDC, m_hBm);
    StretchBlt(m_hDC, 0, RECT_HEIGHT(m_rcDest) - 1, RECT_WIDTH(m_rcDest), -RECT_HEIGHT(m_rcDest), m_hDC,
               0, 0, RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), SRCCOPY);
    NotifyRefreshNeeded();
}

void SelectionModel::RotateNTimes90Degrees(int iN)
{
    if (iN == 2)
    {
        SelectObject(m_hDC, m_hMask);
        StretchBlt(m_hDC, RECT_WIDTH(m_rcDest) - 1, RECT_HEIGHT(m_rcDest) - 1, -RECT_WIDTH(m_rcDest), -RECT_HEIGHT(m_rcDest), m_hDC,
                   0, 0, RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), SRCCOPY);
        SelectObject(m_hDC, m_hBm);
        StretchBlt(m_hDC, RECT_WIDTH(m_rcDest) - 1, RECT_HEIGHT(m_rcDest) - 1, -RECT_WIDTH(m_rcDest), -RECT_HEIGHT(m_rcDest), m_hDC,
                   0, 0, RECT_WIDTH(m_rcDest), RECT_HEIGHT(m_rcDest), SRCCOPY);
    }
    NotifyRefreshNeeded();
}

HBITMAP SelectionModel::GetBitmap() const
{
    return m_hBm;
}

int SelectionModel::PtStackSize() const
{
    return m_iPtSP;
}

void SelectionModel::DrawFramePoly(HDC hDCImage)
{
    Poly(hDCImage, m_ptStack, m_iPtSP, 0, 0, 2, 0, FALSE, TRUE); /* draw the freehand selection inverted/xored */
}

void SelectionModel::SetSrcAndDestRectFromPoints(const POINT& ptFrom, const POINT& ptTo)
{
    m_rcDest.left = m_rcSrc.left = min(ptFrom.x, ptTo.x);
    m_rcDest.top = m_rcSrc.top = min(ptFrom.y, ptTo.y);
    m_rcDest.right = m_rcSrc.right = max(ptFrom.x, ptTo.x);
    m_rcDest.bottom = m_rcSrc.bottom = max(ptFrom.y, ptTo.y);
}

void SelectionModel::SetSrcRectSizeToZero()
{
    m_rcSrc.right = m_rcSrc.left;
    m_rcSrc.bottom = m_rcSrc.top;
}

BOOL SelectionModel::IsSrcRectSizeNonzero() const
{
    return (RECT_WIDTH(m_rcSrc) != 0) && (RECT_HEIGHT(m_rcSrc) != 0);
}

void SelectionModel::ModifyDestRect(POINT& ptDelta, int iAction)
{
    POINT ptDeltaUsed;

    switch (iAction)
    {
        case ACTION_MOVE:                /* move selection */
            ptDeltaUsed.x = ptDelta.x;
            ptDeltaUsed.y = ptDelta.y;
            OffsetRect(&m_rcDest, ptDeltaUsed.x, ptDeltaUsed.y);
            break;
        case ACTION_RESIZE_TOP_LEFT:     /* resize at upper left corner */
            ptDeltaUsed.x = min(ptDelta.x, RECT_WIDTH(m_rcDest) - 1);
            ptDeltaUsed.y = min(ptDelta.y, RECT_HEIGHT(m_rcDest) - 1);
            m_rcDest.left += ptDeltaUsed.x;
            m_rcDest.top  += ptDeltaUsed.y;
            break;
        case ACTION_RESIZE_TOP:          /* resize at top edge */
            ptDeltaUsed.x = ptDelta.x;
            ptDeltaUsed.y = min(ptDelta.y, RECT_HEIGHT(m_rcDest) - 1);
            m_rcDest.top += ptDeltaUsed.y;
            break;
        case ACTION_RESIZE_TOP_RIGHT:    /* resize at upper right corner */
            ptDeltaUsed.x = max(ptDelta.x, -(RECT_WIDTH(m_rcDest) - 1));
            ptDeltaUsed.y = min(ptDelta.y, RECT_HEIGHT(m_rcDest) - 1);
            m_rcDest.top   += ptDeltaUsed.y;
            m_rcDest.right += ptDeltaUsed.x;
            break;
        case ACTION_RESIZE_LEFT:         /* resize at left edge */
            ptDeltaUsed.x = min(ptDelta.x, RECT_WIDTH(m_rcDest) - 1);
            ptDeltaUsed.y = ptDelta.y;
            m_rcDest.left += ptDeltaUsed.x;
            break;
        case ACTION_RESIZE_RIGHT:        /* resize at right edge */
            ptDeltaUsed.x = max(ptDelta.x, -(RECT_WIDTH(m_rcDest) - 1));
            ptDeltaUsed.y = ptDelta.y;
            m_rcDest.right += ptDeltaUsed.x;
            break;
        case ACTION_RESIZE_BOTTOM_LEFT:  /* resize at lower left corner */
            ptDeltaUsed.x = min(ptDelta.x, RECT_WIDTH(m_rcDest) - 1);
            ptDeltaUsed.y = max(ptDelta.y, -(RECT_HEIGHT(m_rcDest) - 1));
            m_rcDest.left   += ptDeltaUsed.x;
            m_rcDest.bottom += ptDeltaUsed.y;
            break;
        case ACTION_RESIZE_BOTTOM:       /* resize at bottom edge */
            ptDeltaUsed.x = ptDelta.x;
            ptDeltaUsed.y = max(ptDelta.y, -(RECT_HEIGHT(m_rcDest) - 1));
            m_rcDest.bottom += ptDeltaUsed.y;
            break;
        case ACTION_RESIZE_BOTTOM_RIGHT: /* resize at lower right corner */
            ptDeltaUsed.x = max(ptDelta.x, -(RECT_WIDTH(m_rcDest) - 1));
            ptDeltaUsed.y = max(ptDelta.y, -(RECT_HEIGHT(m_rcDest) - 1));
            m_rcDest.right  += ptDeltaUsed.x;
            m_rcDest.bottom += ptDeltaUsed.y;
            break;
    }
    ptDelta.x -= ptDeltaUsed.x;
    ptDelta.y -= ptDeltaUsed.y;
}

LONG SelectionModel::GetDestRectWidth() const
{
    return m_rcDest.right - m_rcDest.left;
}

LONG SelectionModel::GetDestRectHeight() const
{
    return m_rcDest.bottom - m_rcDest.top;
}

LONG SelectionModel::GetDestRectLeft() const
{
    return m_rcDest.left;
}

LONG SelectionModel::GetDestRectTop() const
{
    return m_rcDest.top;
}

void SelectionModel::DrawTextToolText(HDC hDCImage, COLORREF crFg, COLORREF crBg, BOOL bBgTransparent)
{
    Text(hDCImage, m_rcDest.left, m_rcDest.top, m_rcDest.right, m_rcDest.bottom, crFg, crBg, textToolText, hfontTextFont, bBgTransparent);
}

void SelectionModel::NotifyRefreshNeeded()
{
    selectionWindow.SendMessage(WM_SELECTIONMODELREFRESHNEEDED);
}

void SelectionModel::GetRect(LPRECT prc) const
{
    *prc = m_rcDest;
}
