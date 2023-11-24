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
    , m_rgbBack(RGB(255, 255, 255))
    , m_bShow(FALSE)
    , m_bContentChanged(FALSE)
{
    m_rc.SetRectEmpty();
    m_rcOld.SetRectEmpty();
    m_ptHit = { -1, -1 };
}

SelectionModel::~SelectionModel()
{
    ClearColorImage();
    ClearMaskImage();
}

void SelectionModel::DrawBackgroundPoly(HDC hDCImage, COLORREF crBg)
{
    if (m_rcOld.IsRectEmpty())
        return;

    HGDIOBJ hbrOld = ::SelectObject(hDCImage, ::GetStockObject(DC_BRUSH));
    ::SetDCBrushColor(hDCImage, crBg);
    ::MaskBlt(hDCImage, m_rcOld.left, m_rcOld.top, m_rcOld.Width(), m_rcOld.Height(),
              hDCImage, m_rcOld.left, m_rcOld.top, m_hbmMask, 0, 0, MAKEROP4(PATCOPY, SRCCOPY));
    ::SelectObject(hDCImage, hbrOld);
}

void SelectionModel::DrawBackgroundRect(HDC hDCImage, COLORREF crBg)
{
    if (m_rcOld.IsRectEmpty())
        return;

    Rect(hDCImage, m_rcOld.left, m_rcOld.top, m_rcOld.right, m_rcOld.bottom, crBg, crBg, 0, 1);
}

void SelectionModel::DrawBackground(HDC hDCImage, COLORREF crBg)
{
    if (toolsModel.GetActiveTool() == TOOL_FREESEL)
        DrawBackgroundPoly(hDCImage, crBg);
    else
        DrawBackgroundRect(hDCImage, crBg);
}

void SelectionModel::DrawSelection(HDC hDCImage, COLORREF crBg, BOOL bBgTransparent)
{
    CRect rc = m_rc;
    if (rc.IsRectEmpty())
        return;

    BITMAP bm;
    if (!GetObjectW(m_hbmColor, sizeof(BITMAP), &bm))
        return;

    COLORREF keyColor = (bBgTransparent ? crBg : CLR_INVALID);

    HDC hMemDC = CreateCompatibleDC(hDCImage);
    HGDIOBJ hbmOld = SelectObject(hMemDC, m_hbmColor);
    ColorKeyedMaskBlt(hDCImage, rc.left, rc.top, rc.Width(), rc.Height(),
                      hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, m_hbmMask, keyColor);
    SelectObject(hMemDC, hbmOld);
    DeleteDC(hMemDC);
}

void SelectionModel::setMask(const CRect& rc, HBITMAP hbmMask)
{
    if (m_hbmMask)
        ::DeleteObject(m_hbmMask);

    m_hbmMask = hbmMask;
    m_rc = m_rcOld = rc;
}

HBITMAP SelectionModel::GetSelectionContents()
{
    if (m_hbmColor)
        return CopyDIBImage(m_hbmColor, m_rc.Width(), m_rc.Height());

    HBITMAP hbmWhole = imageModel.LockBitmap();
    HBITMAP hbmPart = getSubImage(hbmWhole, m_rc);
    imageModel.UnlockBitmap(hbmWhole);
    return hbmPart;
}

BOOL SelectionModel::IsLanded() const
{
    return !m_hbmColor;
}

BOOL SelectionModel::TakeOff()
{
    if (!IsLanded() || m_rc.IsRectEmpty())
        return FALSE;

    // The background color is needed for transparency of selection
    m_rgbBack = paletteModel.GetBgColor();

    // Get the contents of the selection area
    ClearColorImage();
    m_hbmColor = GetSelectionContents();

    // RectSel doesn't need the mask image
    if (toolsModel.GetActiveTool() == TOOL_RECTSEL)
        ClearMaskImage();

    // Save the selection area
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

    if (m_bContentChanged ||
        (!m_rc.EqualRect(m_rcOld) && !m_rc.IsRectEmpty() && !m_rcOld.IsRectEmpty()))
    {
        CRect rc;
        rc.UnionRect(m_rc, m_rcOld);
        imageModel.PushImageForUndo(rc);

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
        ClearMaskImage();
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

void SelectionModel::SetRectFromPoints(const POINT& ptFrom, const POINT& ptTo)
{
    m_rc = CRect(ptFrom, ptTo);
    m_rc.NormalizeRect();
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
            m_rc.OffsetRect(pt.x - m_ptHit.x, pt.y - m_ptHit.y);
            break;
    }
    m_ptHit = pt;
}

void SelectionModel::ClearMaskImage()
{
    if (m_hbmMask)
    {
        ::DeleteObject(m_hbmMask);
        m_hbmMask = NULL;
    }
}

void SelectionModel::ClearColorImage()
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
    ClearColorImage();
    ClearMaskImage();
    m_rc.SetRectEmpty();
    m_rcOld.SetRectEmpty();
    imageModel.NotifyImageChanged();
}

void SelectionModel::DeleteSelection()
{
    if (!m_bShow)
        return;

    TakeOff();
    imageModel.PushImageForUndo();
    DrawBackground(imageModel.GetDC(), paletteModel.GetBgColor());

    HideSelection();
}

void SelectionModel::InvertSelection()
{
    TakeOff();

    BITMAP bm;
    ::GetObjectW(m_hbmColor, sizeof(bm), &bm);

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
    INT cx = m_rc.Width(), cy = m_rc.Height();
    m_rc.right = m_rc.left + cy;
    m_rc.bottom = m_rc.top + cx;
}

HITTEST SelectionModel::hitTest(POINT ptCanvas)
{
    if (!m_bShow)
        return HIT_NONE;

    CRect rcSelection = m_rc;
    canvasWindow.ImageToCanvas(rcSelection);
    rcSelection.InflateRect(GRIP_SIZE, GRIP_SIZE);
    return getSizeBoxHitTest(ptCanvas, &rcSelection);
}

void SelectionModel::drawFrameOnCanvas(HDC hCanvasDC)
{
    if (!m_bShow)
        return;

    CRect rcSelection = m_rc;
    canvasWindow.ImageToCanvas(rcSelection);
    rcSelection.InflateRect(GRIP_SIZE, GRIP_SIZE);
    drawSizeBoxes(hCanvasDC, &rcSelection, TRUE);
}

void SelectionModel::moveSelection(INT xDelta, INT yDelta)
{
    if (!m_bShow)
        return;

    TakeOff();
    m_rc.OffsetRect(xDelta, yDelta);
    canvasWindow.Invalidate();
}

void SelectionModel::StretchSelection(BOOL bShrink)
{
    if (!m_bShow)
        return;

    TakeOff();

    INT cx = m_rc.Width(), cy = m_rc.Height();

    if (bShrink)
        m_rc.InflateRect(-cx / 4, -cy / 4);
    else
        m_rc.InflateRect(+cx / 2, +cy / 2);

    // The selection area must exist there
    if (m_rc.Width() <= 0)
        m_rc.right = m_rc.left + 1;
    if (m_rc.Height() <= 0)
        m_rc.bottom = m_rc.top + 1;

    imageModel.NotifyImageChanged();
}
