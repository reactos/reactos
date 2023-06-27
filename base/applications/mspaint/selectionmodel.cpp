/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Keep track of selection parameters, notify listeners
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

SelectionModel selectionModel;

/* FUNCTIONS ********************************************************/

SelectionModel::SelectionModel()
    : m_hbmColor(NULL)
    , m_hbmMask(NULL)
    , m_ptStack(NULL)
    , m_iPtSP(0)
    , m_rgbBack(RGB(255, 255, 255))
    , m_bShow(FALSE)
    , m_bContentChanged(FALSE)
{
    ::SetRectEmpty(&m_rc);
    ::SetRectEmpty(&m_rcOld);
    m_ptHit.x = m_ptHit.y = -1;
}

SelectionModel::~SelectionModel()
{
    ClearColor();
    ClearMask();
    ResetPtStack();
}

void SelectionModel::ResetPtStack()
{
    if (m_ptStack)
    {
        free(m_ptStack);
        m_ptStack = NULL;
    }
    m_iPtSP = 0;
}

void SelectionModel::PushToPtStack(POINT pt)
{
#define GROW_COUNT 256
    if (m_iPtSP % GROW_COUNT == 0)
    {
        INT nNewCount = m_iPtSP + GROW_COUNT;
        LPPOINT pptNew = (LPPOINT)realloc(m_ptStack, sizeof(POINT) * nNewCount);
        if (pptNew == NULL)
            return;
        m_ptStack = pptNew;
    }
    m_ptStack[m_iPtSP] = pt;
    m_iPtSP++;
#undef GROW_COUNT
}

void SelectionModel::ShiftPtStack(INT dx, INT dy)
{
    for (INT i = 0; i < m_iPtSP; ++i)
    {
        POINT& pt = m_ptStack[i];
        pt.x += dx;
        pt.y += dy;
    }
}

void SelectionModel::BuildMaskFromPtStack()
{
    CRect rc = { MAXLONG, MAXLONG, 0, 0 };
    for (INT i = 0; i < m_iPtSP; ++i)
    {
        POINT& pt = m_ptStack[i];
        rc.left = min(pt.x, rc.left);
        rc.top = min(pt.y, rc.top);
        rc.right = max(pt.x, rc.right);
        rc.bottom = max(pt.y, rc.bottom);
    }
    rc.right += 1;
    rc.bottom += 1;

    m_rc = m_rcOld = rc;

    ClearMask();

    ShiftPtStack(-m_rcOld.left, -m_rcOld.top);

    HDC hdcMem = ::CreateCompatibleDC(NULL);
    m_hbmMask = ::CreateBitmap(rc.Width(), rc.Height(), 1, 1, NULL);
    HGDIOBJ hbmOld = ::SelectObject(hdcMem, m_hbmMask);
    ::FillRect(hdcMem, &rc, (HBRUSH)::GetStockObject(BLACK_BRUSH));
    HGDIOBJ hPenOld = ::SelectObject(hdcMem, GetStockObject(NULL_PEN));
    HGDIOBJ hbrOld = ::SelectObject(hdcMem, GetStockObject(WHITE_BRUSH));
    ::Polygon(hdcMem, m_ptStack, m_iPtSP);
    ::SelectObject(hdcMem, hbrOld);
    ::SelectObject(hdcMem, hPenOld);
    ::SelectObject(hdcMem, hbmOld);
    ::DeleteDC(hdcMem);

    ShiftPtStack(+m_rcOld.left, +m_rcOld.top);
}

void SelectionModel::DrawBackgroundPoly(HDC hDCImage, COLORREF crBg)
{
    if (::IsRectEmpty(&m_rcOld))
        return;

    HGDIOBJ hPenOld = ::SelectObject(hDCImage, ::GetStockObject(NULL_PEN));
    HGDIOBJ hbrOld = ::SelectObject(hDCImage, ::CreateSolidBrush(crBg));
    ::Polygon(hDCImage, m_ptStack, m_iPtSP);
    ::DeleteObject(::SelectObject(hDCImage, hbrOld));
    ::SelectObject(hDCImage, hPenOld);
}

void SelectionModel::DrawBackgroundRect(HDC hDCImage, COLORREF crBg)
{
    if (::IsRectEmpty(&m_rcOld))
        return;

    Rect(hDCImage, m_rcOld.left, m_rcOld.top, m_rcOld.right, m_rcOld.bottom, crBg, crBg, 0, 1);
}

void SelectionModel::DrawBackground(HDC hDCImage)
{
    if (toolsModel.GetActiveTool() == TOOL_FREESEL)
        DrawBackgroundPoly(hDCImage, paletteModel.GetBgColor());
    else
        DrawBackgroundRect(hDCImage, paletteModel.GetBgColor());
}

void SelectionModel::DrawSelection(HDC hDCImage, COLORREF crBg, BOOL bBgTransparent)
{
    CRect rc = m_rc;
    if (::IsRectEmpty(&rc))
        return;

    BITMAP bm;
    if (!GetObject(m_hbmColor, sizeof(BITMAP), &bm))
        return;

    COLORREF keyColor = (bBgTransparent ? crBg : CLR_INVALID);

    HDC hMemDC = CreateCompatibleDC(hDCImage);
    HGDIOBJ hbmOld = SelectObject(hMemDC, m_hbmColor);
    ColorKeyedMaskBlt(hDCImage, rc.left, rc.top, rc.Width(), rc.Height(),
                      hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, m_hbmMask, keyColor);
    SelectObject(hMemDC, hbmOld);
    DeleteDC(hMemDC);
}

void SelectionModel::GetSelectionContents(HDC hDCImage)
{
    ClearColor();

    HDC hMemDC = ::CreateCompatibleDC(NULL);
    m_hbmColor = CreateColorDIB(m_rc.Width(), m_rc.Height(), RGB(255, 255, 255));
    HGDIOBJ hbmOld = ::SelectObject(hMemDC, m_hbmColor);
    ::BitBlt(hMemDC, 0, 0, m_rc.Width(), m_rc.Height(), hDCImage, m_rc.left, m_rc.top, SRCCOPY);
    ::SelectObject(hMemDC, hbmOld);
    ::DeleteDC(hMemDC);
}

BOOL SelectionModel::IsLanded() const
{
    return !m_hbmColor;
}

BOOL SelectionModel::TakeOff()
{
    if (!IsLanded() || ::IsRectEmpty(&m_rc))
        return FALSE;

    m_rgbBack = paletteModel.GetBgColor();
    GetSelectionContents(imageModel.GetDC());

    if (toolsModel.GetActiveTool() == TOOL_RECTSEL)
        ClearMask();

    m_rcOld = m_rc;

    imageModel.NotifyImageChanged();
    return TRUE;
}

void SelectionModel::Landing()
{
    if (IsLanded() && !m_bShow)
    {
        imageModel.NotifyImageChanged();
        return;
    }

    m_bShow = FALSE;

    if (m_bContentChanged ||
        (!::EqualRect(m_rc, m_rcOld) && !::IsRectEmpty(m_rc) && !::IsRectEmpty(m_rcOld)))
    {
        imageModel.PushImageForUndo();

        canvasWindow.m_drawing = FALSE;
        toolsModel.OnDrawOverlayOnImage(imageModel.GetDC());
    }

    HideSelection();
}

void SelectionModel::InsertFromHBITMAP(HBITMAP hbmColor, INT x, INT y, HBITMAP hbmMask)
{
    ::DeleteObject(m_hbmColor);
    m_hbmColor = hbmColor;

    m_rc.left = x;
    m_rc.top = y;
    m_rc.right = x + GetDIBWidth(hbmColor);
    m_rc.bottom = y + GetDIBHeight(hbmColor);

    if (hbmMask)
    {
        ::DeleteObject(m_hbmMask);
        m_hbmMask = hbmMask;
    }
    else
    {
        ClearMask();
    }

    NotifyContentChanged();
}

void SelectionModel::FlipHorizontally()
{
    TakeOff();

    HDC hdcMem = ::CreateCompatibleDC(NULL);
    if (m_hbmMask)
    {
        ::SelectObject(hdcMem, m_hbmMask);
        ::StretchBlt(hdcMem, m_rc.Width() - 1, 0, -m_rc.Width(), m_rc.Height(),
                     hdcMem, 0, 0, m_rc.Width(), m_rc.Height(), SRCCOPY);
    }
    if (m_hbmColor)
    {
        ::SelectObject(hdcMem, m_hbmColor);
        ::StretchBlt(hdcMem, m_rc.Width() - 1, 0, -m_rc.Width(), m_rc.Height(),
                     hdcMem, 0, 0, m_rc.Width(), m_rc.Height(), SRCCOPY);
    }
    ::DeleteDC(hdcMem);

    NotifyContentChanged();
}

void SelectionModel::FlipVertically()
{
    TakeOff();

    HDC hdcMem = ::CreateCompatibleDC(NULL);
    if (m_hbmMask)
    {
        ::SelectObject(hdcMem, m_hbmMask);
        ::StretchBlt(hdcMem, 0, m_rc.Height() - 1, m_rc.Width(), -m_rc.Height(),
                     hdcMem, 0, 0, m_rc.Width(), m_rc.Height(), SRCCOPY);
    }
    if (m_hbmColor)
    {
        ::SelectObject(hdcMem, m_hbmColor);
        ::StretchBlt(hdcMem, 0, m_rc.Height() - 1, m_rc.Width(), -m_rc.Height(),
                     hdcMem, 0, 0, m_rc.Width(), m_rc.Height(), SRCCOPY);
    }
    ::DeleteDC(hdcMem);

    NotifyContentChanged();
}

void SelectionModel::RotateNTimes90Degrees(int iN)
{
    HBITMAP hbm;
    HGDIOBJ hbmOld;
    HDC hdcMem = ::CreateCompatibleDC(NULL);

    switch (iN)
    {
        case 1: /* rotate 90 degrees */
        case 3: /* rotate 270 degrees */
            TakeOff();

            if (m_hbmColor)
            {
                hbmOld = ::SelectObject(hdcMem, m_hbmColor);
                hbm = Rotate90DegreeBlt(hdcMem, m_rc.Width(), m_rc.Height(), iN == 1, FALSE);
                ::SelectObject(hdcMem, hbmOld);
                ::DeleteObject(m_hbmColor);
                m_hbmColor = hbm;
            }
            if (m_hbmMask)
            {
                hbmOld = ::SelectObject(hdcMem, m_hbmMask);
                hbm = Rotate90DegreeBlt(hdcMem, m_rc.Width(), m_rc.Height(), iN == 1, TRUE);
                ::SelectObject(hdcMem, hbmOld);
                ::DeleteObject(m_hbmMask);
                m_hbmMask = hbm;
            }

            SwapWidthAndHeight();
            break;

        case 2: /* rotate 180 degrees */
            TakeOff();

            if (m_hbmColor)
            {
                hbmOld = ::SelectObject(hdcMem, m_hbmColor);
                ::StretchBlt(hdcMem, m_rc.Width() - 1, m_rc.Height() - 1, -m_rc.Width(), -m_rc.Height(),
                             hdcMem, 0, 0, m_rc.Width(), m_rc.Height(), SRCCOPY);
                ::SelectObject(hdcMem, hbmOld);
            }
            if (m_hbmMask)
            {
                hbmOld = ::SelectObject(hdcMem, m_hbmMask);
                ::StretchBlt(hdcMem, m_rc.Width() - 1, m_rc.Height() - 1, -m_rc.Width(), -m_rc.Height(),
                             hdcMem, 0, 0, m_rc.Width(), m_rc.Height(), SRCCOPY);
                ::SelectObject(hdcMem, hbmOld);
            }
            break;
    }

    ::DeleteDC(hdcMem);
    NotifyContentChanged();
}

static void AttachHBITMAP(HBITMAP *phbm, HBITMAP hbmNew)
{
    if (hbmNew == NULL)
        return;
    ::DeleteObject(*phbm);
    *phbm = hbmNew;
}

void SelectionModel::StretchSkew(int nStretchPercentX, int nStretchPercentY, int nSkewDegX, int nSkewDegY)
{
    if (nStretchPercentX == 100 && nStretchPercentY == 100 && nSkewDegX == 0 && nSkewDegY == 0)
        return;

    TakeOff();

    INT oldWidth = m_rc.Width(), oldHeight = m_rc.Height();
    INT newWidth = oldWidth * nStretchPercentX / 100;
    INT newHeight = oldHeight * nStretchPercentY / 100;

    HBITMAP hbmColor = m_hbmColor, hbmMask = m_hbmMask;

    if (hbmMask == NULL)
        hbmMask = CreateMonoBitmap(oldWidth, oldHeight, TRUE);

    if (oldWidth != newWidth || oldHeight != newHeight)
    {
        AttachHBITMAP(&hbmColor, CopyDIBImage(hbmColor, newWidth, newHeight));
        AttachHBITMAP(&hbmMask, CopyMonoImage(hbmMask, newWidth, newHeight));
    }

    HGDIOBJ hbmOld;
    HDC hDC = ::CreateCompatibleDC(NULL);

    if (nSkewDegX)
    {
        hbmOld = ::SelectObject(hDC, hbmColor);
        AttachHBITMAP(&hbmColor, SkewDIB(hDC, hbmColor, nSkewDegX, FALSE));
        ::SelectObject(hDC, hbmMask);
        AttachHBITMAP(&hbmMask, SkewDIB(hDC, hbmMask, nSkewDegX, FALSE, TRUE));
        ::SelectObject(hDC, hbmOld);
    }

    if (nSkewDegY)
    {
        hbmOld = ::SelectObject(hDC, hbmColor);
        AttachHBITMAP(&hbmColor, SkewDIB(hDC, hbmColor, nSkewDegY, TRUE));
        ::SelectObject(hDC, hbmMask);
        AttachHBITMAP(&hbmMask, SkewDIB(hDC, hbmMask, nSkewDegY, TRUE, TRUE));
        ::SelectObject(hDC, hbmOld);
    }

    ::DeleteDC(hDC);

    InsertFromHBITMAP(hbmColor, m_rc.left, m_rc.top, hbmMask);

    m_bShow = TRUE;
    NotifyContentChanged();
}

HBITMAP SelectionModel::CopyBitmap()
{
    if (m_hbmColor == NULL)
        GetSelectionContents(imageModel.GetDC());
    return CopyDIBImage(m_hbmColor);
}

int SelectionModel::PtStackSize() const
{
    return m_iPtSP;
}

void SelectionModel::DrawFramePoly(HDC hDCImage)
{
    /* draw the freehand selection inverted/xored */
    Poly(hDCImage, m_ptStack, m_iPtSP, 0, 0, 2, 0, FALSE, TRUE);
}

void SelectionModel::SetRectFromPoints(const POINT& ptFrom, const POINT& ptTo)
{
    m_rc.left = min(ptFrom.x, ptTo.x);
    m_rc.top = min(ptFrom.y, ptTo.y);
    m_rc.right = max(ptFrom.x, ptTo.x);
    m_rc.bottom = max(ptFrom.y, ptTo.y);
}

void SelectionModel::Dragging(HITTEST hit, POINT pt)
{
    switch (hit)
    {
        case HIT_NONE:
            break;
        case HIT_UPPER_LEFT:
            m_rc.left += pt.x - m_ptHit.x;
            m_rc.top += pt.y - m_ptHit.y;
            break;
        case HIT_UPPER_CENTER:
            m_rc.top += pt.y - m_ptHit.y;
            break;
        case HIT_UPPER_RIGHT:
            m_rc.right += pt.x - m_ptHit.x;
            m_rc.top += pt.y - m_ptHit.y;
            break;
        case HIT_MIDDLE_LEFT:
            m_rc.left += pt.x - m_ptHit.x;
            break;
        case HIT_MIDDLE_RIGHT:
            m_rc.right += pt.x - m_ptHit.x;
            break;
        case HIT_LOWER_LEFT:
            m_rc.left += pt.x - m_ptHit.x;
            m_rc.bottom += pt.y - m_ptHit.y;
            break;
        case HIT_LOWER_CENTER:
            m_rc.bottom += pt.y - m_ptHit.y;
            break;
        case HIT_LOWER_RIGHT:
            m_rc.right += pt.x - m_ptHit.x;
            m_rc.bottom += pt.y - m_ptHit.y;
            break;
        case HIT_BORDER:
        case HIT_INNER:
            OffsetRect(&m_rc, pt.x - m_ptHit.x, pt.y - m_ptHit.y);
            break;
    }
    m_ptHit = pt;
}

void SelectionModel::ClearMask()
{
    if (m_hbmMask)
    {
        ::DeleteObject(m_hbmMask);
        m_hbmMask = NULL;
    }
}

void SelectionModel::ClearColor()
{
    if (m_hbmColor)
    {
        ::DeleteObject(m_hbmColor);
        m_hbmColor = NULL;
    }
}

void SelectionModel::HideSelection()
{
    m_bShow = m_bContentChanged = FALSE;
    ClearColor();
    ClearMask();
    ::SetRectEmpty(&m_rc);
    ::SetRectEmpty(&m_rcOld);
    imageModel.NotifyImageChanged();
}

void SelectionModel::DeleteSelection()
{
    if (!m_bShow)
        return;

    TakeOff();
    imageModel.PushImageForUndo();
    DrawBackground(imageModel.GetDC());

    HideSelection();
}

void SelectionModel::InvertSelection()
{
    TakeOff();

    BITMAP bm;
    ::GetObject(m_hbmColor, sizeof(bm), &bm);

    HDC hdc = ::CreateCompatibleDC(NULL);
    HGDIOBJ hbmOld = ::SelectObject(hdc, m_hbmColor);
    RECT rc = { 0, 0, bm.bmWidth, bm.bmHeight };
    ::InvertRect(hdc, &rc);
    ::SelectObject(hdc, hbmOld);
    ::DeleteDC(hdc);

    NotifyContentChanged();
}

void SelectionModel::NotifyContentChanged()
{
    m_bContentChanged = TRUE;
    imageModel.NotifyImageChanged();
}

void SelectionModel::SwapWidthAndHeight()
{
    INT cx = m_rc.Width();
    INT cy = m_rc.Height();
    m_rc.right = m_rc.left + cy;
    m_rc.bottom = m_rc.top + cx;
}
