/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Undo and redo functionality
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

ImageModel imageModel;

/* FUNCTIONS ********************************************************/

void IMAGE_PART::clear()
{
    ::DeleteObject(m_hbmImage);
    m_hbmImage = NULL;
    m_rcPart.SetRectEmpty();
    m_bPartial = FALSE;
}

void ImageModel::NotifyImageChanged()
{
    if (canvasWindow.IsWindow())
    {
        canvasWindow.updateScrollRange();
        canvasWindow.Invalidate();
    }

    if (miniature.IsWindow())
        miniature.Invalidate();
}

ImageModel::ImageModel()
    : m_hDrawingDC(::CreateCompatibleDC(NULL))
    , m_currInd(0)
    , m_undoSteps(0)
    , m_redoSteps(0)
{
    ZeroMemory(m_historyItems, sizeof(m_historyItems));

    m_hbmMaster = CreateColorDIB(1, 1, RGB(255, 255, 255));
    m_hbmOld = ::SelectObject(m_hDrawingDC, m_hbmMaster);

    g_imageSaved = TRUE;
}

ImageModel::~ImageModel()
{
    ::SelectObject(m_hDrawingDC, m_hbmOld); // De-select
    ::DeleteDC(m_hDrawingDC);
    ::DeleteObject(m_hbmMaster);
    ClearHistory();
}

void ImageModel::SwapPart()
{
    IMAGE_PART& part = m_historyItems[m_currInd];
    if (!part.m_bPartial)
    {
        Swap(m_hbmMaster, part.m_hbmImage);
        return;
    }

    HBITMAP hbmMaster = LockBitmap();
    HBITMAP hbmPart = getSubImage(hbmMaster, part.m_rcPart);
    putSubImage(hbmMaster, part.m_rcPart, part.m_hbmImage);
    ::DeleteObject(part.m_hbmImage);
    part.m_hbmImage = hbmPart;
    UnlockBitmap(hbmMaster);
}

void ImageModel::Undo(BOOL bClearRedo)
{
    ATLTRACE("%s: %d\n", __FUNCTION__, m_undoSteps);
    if (!CanUndo())
        return;

    selectionModel.HideSelection();

    m_currInd = (m_currInd + HISTORYSIZE - 1) % HISTORYSIZE; // Go previous
    ATLASSERT(m_hbmMaster != NULL);
    SwapPart();
    ::SelectObject(m_hDrawingDC, m_hbmMaster); // Re-select

    m_undoSteps--;
    if (bClearRedo)
        m_redoSteps = 0;
    else if (m_redoSteps < HISTORYSIZE - 1)
        m_redoSteps++;

    NotifyImageChanged();
}

void ImageModel::Redo()
{
    ATLTRACE("%s: %d\n", __FUNCTION__, m_redoSteps);
    if (!CanRedo())
        return;

    selectionModel.HideSelection();

    ATLASSERT(m_hbmMaster != NULL);
    SwapPart();
    m_currInd = (m_currInd + 1) % HISTORYSIZE; // Go next
    ::SelectObject(m_hDrawingDC, m_hbmMaster); // Re-select

    m_redoSteps--;
    if (m_undoSteps < HISTORYSIZE - 1)
        m_undoSteps++;

    NotifyImageChanged();
}

void ImageModel::ClearHistory()
{
    for (int i = 0; i < HISTORYSIZE; ++i)
    {
        m_historyItems[i].clear();
    }

    m_undoSteps = 0;
    m_redoSteps = 0;
}

void ImageModel::PushImageForUndo()
{
    HBITMAP hbm = CopyBitmap();
    if (hbm == NULL)
    {
        ShowOutOfMemory();
        return;
    }

    PushImageForUndo(hbm);
}

void ImageModel::PushImageForUndo(HBITMAP hbm)
{
    ATLTRACE("%s: %d\n", __FUNCTION__, m_currInd);

    if (hbm == NULL)
    {
        ShowOutOfMemory();
        return;
    }

    IMAGE_PART& part = m_historyItems[m_currInd];
    part.clear();
    part.m_hbmImage = m_hbmMaster;
    m_hbmMaster = hbm;
    ::SelectObject(m_hDrawingDC, m_hbmMaster); // Re-select

    PushDone();
}

void ImageModel::PushImageForUndo(const RECT& rcPartial)
{
    ATLTRACE("%s: %d\n", __FUNCTION__, m_currInd);

    IMAGE_PART& part = m_historyItems[m_currInd];
    part.clear();
    part.m_bPartial = TRUE;
    part.m_rcPart = rcPartial;

    CRect rcImage = { 0, 0, GetWidth(), GetHeight() };
    CRect& rc = part.m_rcPart;
    if (!rc.IntersectRect(rc, rcImage))
        rc.SetRect(-1, -1, 0, 0);

    HBITMAP hbmMaster = LockBitmap();
    part.m_hbmImage = getSubImage(hbmMaster, rc);
    UnlockBitmap(hbmMaster);

    PushDone();
}

void ImageModel::PushDone()
{
    m_currInd = (m_currInd + 1) % HISTORYSIZE; // Go next

    if (m_undoSteps < HISTORYSIZE - 1)
        m_undoSteps++;
    m_redoSteps = 0;

    g_imageSaved = FALSE;
    NotifyImageChanged();
}

void ImageModel::Crop(int nWidth, int nHeight, int nOffsetX, int nOffsetY)
{
    // We cannot create bitmaps of size zero
    if (nWidth <= 0)
        nWidth = 1;
    if (nHeight <= 0)
        nHeight = 1;

    // Create a white HBITMAP
    HBITMAP hbmNew = CreateColorDIB(nWidth, nHeight, RGB(255, 255, 255));
    if (!hbmNew)
    {
        ShowOutOfMemory();
        return;
    }

    // Put the master image as a sub-image
    RECT rcPart = { -nOffsetX, -nOffsetY, GetWidth() - nOffsetX, GetHeight() - nOffsetY };
    HBITMAP hbmOld = imageModel.LockBitmap();
    putSubImage(hbmNew, rcPart, hbmOld);
    imageModel.UnlockBitmap(hbmOld);

    // Push it
    PushImageForUndo(hbmNew);

    NotifyImageChanged();
}

void ImageModel::SaveImage(LPCWSTR lpFileName)
{
    SaveDIBToFile(m_hbmMaster, lpFileName, TRUE);
}

BOOL ImageModel::IsImageSaved() const
{
    return g_imageSaved;
}

void ImageModel::StretchSkew(int nStretchPercentX, int nStretchPercentY, int nSkewDegX, int nSkewDegY)
{
    int oldWidth = GetWidth();
    int oldHeight = GetHeight();
    INT newWidth = oldWidth * nStretchPercentX / 100;
    INT newHeight = oldHeight * nStretchPercentY / 100;
    if (oldWidth != newWidth || oldHeight != newHeight)
    {
        HBITMAP hbm0 = CopyDIBImage(m_hbmMaster, newWidth, newHeight);
        PushImageForUndo(hbm0);
    }
    if (nSkewDegX)
    {
        HBITMAP hbm1 = SkewDIB(m_hDrawingDC, m_hbmMaster, nSkewDegX, FALSE);
        PushImageForUndo(hbm1);
    }
    if (nSkewDegY)
    {
        HBITMAP hbm2 = SkewDIB(m_hDrawingDC, m_hbmMaster, nSkewDegY, TRUE);
        PushImageForUndo(hbm2);
    }
    NotifyImageChanged();
}

int ImageModel::GetWidth() const
{
    return GetDIBWidth(m_hbmMaster);
}

int ImageModel::GetHeight() const
{
    return GetDIBHeight(m_hbmMaster);
}

void ImageModel::InvertColors()
{
    RECT rect = {0, 0, GetWidth(), GetHeight()};
    PushImageForUndo();
    InvertRect(m_hDrawingDC, &rect);
    NotifyImageChanged();
}

HDC ImageModel::GetDC()
{
    return m_hDrawingDC;
}

void ImageModel::FlipHorizontally()
{
    PushImageForUndo();
    StretchBlt(m_hDrawingDC, GetWidth() - 1, 0, -GetWidth(), GetHeight(), GetDC(), 0, 0,
               GetWidth(), GetHeight(), SRCCOPY);
    NotifyImageChanged();
}

void ImageModel::FlipVertically()
{
    PushImageForUndo();
    StretchBlt(m_hDrawingDC, 0, GetHeight() - 1, GetWidth(), -GetHeight(), GetDC(), 0, 0,
               GetWidth(), GetHeight(), SRCCOPY);
    NotifyImageChanged();
}

void ImageModel::RotateNTimes90Degrees(int iN)
{
    switch (iN)
    {
        case 1:
        case 3:
        {
            HBITMAP hbm = Rotate90DegreeBlt(m_hDrawingDC, GetWidth(), GetHeight(), iN == 1, FALSE);
            PushImageForUndo(hbm);
            break;
        }
        case 2:
        {
            PushImageForUndo();
            ::StretchBlt(m_hDrawingDC, GetWidth() - 1, GetHeight() - 1, -GetWidth(), -GetHeight(),
                         m_hDrawingDC, 0, 0, GetWidth(), GetHeight(), SRCCOPY);
            break;
        }
    }
    NotifyImageChanged();
}

void ImageModel::Clamp(POINT& pt) const
{
    pt.x = max(0, min(pt.x, GetWidth()));
    pt.y = max(0, min(pt.y, GetHeight()));
}

HBITMAP ImageModel::CopyBitmap()
{
    HBITMAP hBitmap = LockBitmap();
    HBITMAP ret = CopyDIBImage(hBitmap);
    UnlockBitmap(hBitmap);
    return ret;
}

BOOL ImageModel::IsBlackAndWhite()
{
    HBITMAP hBitmap = LockBitmap();
    BOOL bBlackAndWhite = IsBitmapBlackAndWhite(hBitmap);
    UnlockBitmap(hBitmap);
    return bBlackAndWhite;
}

void ImageModel::PushBlackAndWhite()
{
    HBITMAP hBitmap = LockBitmap();
    HBITMAP hNewBitmap = ConvertToBlackAndWhite(hBitmap);
    UnlockBitmap(hBitmap);

    PushImageForUndo(hNewBitmap);
}

HBITMAP ImageModel::LockBitmap()
{
    // NOTE: An app cannot select a bitmap into more than one device context at a time.
    ::SelectObject(m_hDrawingDC, m_hbmOld); // De-select
    HBITMAP hbmLocked = m_hbmMaster;
    m_hbmMaster = NULL;
    return hbmLocked;
}

void ImageModel::UnlockBitmap(HBITMAP hbmLocked)
{
    m_hbmMaster = hbmLocked;
    m_hbmOld = ::SelectObject(m_hDrawingDC, m_hbmMaster); // Re-select
}
