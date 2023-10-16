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
    ZeroMemory(m_hBms, sizeof(m_hBms));

    m_hBms[0] = CreateColorDIB(1, 1, RGB(255, 255, 255));
    m_hbmOld = ::SelectObject(m_hDrawingDC, m_hBms[0]);

    g_imageSaved = TRUE;
}

ImageModel::~ImageModel()
{
    ::DeleteDC(m_hDrawingDC);

    for (size_t i = 0; i < HISTORYSIZE; ++i)
    {
        if (m_hBms[i])
            ::DeleteObject(m_hBms[i]);
    }
}

void ImageModel::Undo(BOOL bClearRedo)
{
    ATLTRACE("%s: %d\n", __FUNCTION__, m_undoSteps);
    if (!CanUndo())
        return;

    selectionModel.HideSelection();

    // Select previous item
    m_currInd = (m_currInd + HISTORYSIZE - 1) % HISTORYSIZE;
    ::SelectObject(m_hDrawingDC, m_hBms[m_currInd]);

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

    // Select next item
    m_currInd = (m_currInd + 1) % HISTORYSIZE;
    ::SelectObject(m_hDrawingDC, m_hBms[m_currInd]);

    m_redoSteps--;
    if (m_undoSteps < HISTORYSIZE - 1)
        m_undoSteps++;

    NotifyImageChanged();
}

void ImageModel::ResetToPrevious()
{
    ATLTRACE("%s: %d\n", __FUNCTION__, m_currInd);

    // Revert current item with previous item
    ::DeleteObject(m_hBms[m_currInd]);
    m_hBms[m_currInd] = CopyDIBImage(m_hBms[(m_currInd + HISTORYSIZE - 1) % HISTORYSIZE]);
    ::SelectObject(m_hDrawingDC, m_hBms[m_currInd]);

    NotifyImageChanged();
}

void ImageModel::ClearHistory()
{
    for (int i = 0; i < HISTORYSIZE; ++i)
    {
        if (m_hBms[i] && i != m_currInd)
        {
            ::DeleteObject(m_hBms[i]);
            m_hBms[i] = NULL;
        }
    }

    m_undoSteps = 0;
    m_redoSteps = 0;
}

void ImageModel::PushImageForUndo(HBITMAP hbm)
{
    ATLTRACE("%s: %d\n", __FUNCTION__, m_currInd);

    // Go to the next item with an HBITMAP or current item
    ::DeleteObject(m_hBms[(m_currInd + 1) % HISTORYSIZE]);
    m_hBms[(m_currInd + 1) % HISTORYSIZE] = (hbm ? hbm : CopyDIBImage(m_hBms[m_currInd]));
    m_currInd = (m_currInd + 1) % HISTORYSIZE;
    ::SelectObject(m_hDrawingDC, m_hBms[m_currInd]);

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

    // Create an HBITMAP
    HBITMAP hbmCropped = CreateDIBWithProperties(nWidth, nHeight);
    if (!hbmCropped)
        return;

    // Select the HBITMAP by memory DC
    HDC hdcMem = ::CreateCompatibleDC(m_hDrawingDC);
    HGDIOBJ hbmOld = ::SelectObject(hdcMem, hbmCropped);

    // Fill background of the HBITMAP
    RECT rcBack = { 0, 0, nWidth, nHeight };
    HBRUSH hbrBack = ::CreateSolidBrush(paletteModel.GetBgColor());
    ::FillRect(hdcMem, &rcBack, hbrBack);
    ::DeleteObject(hbrBack);

    // Copy the old content
    ::BitBlt(hdcMem, -nOffsetX, -nOffsetY, GetWidth(), GetHeight(), m_hDrawingDC, 0, 0, SRCCOPY);

    // Clean up
    ::SelectObject(hdcMem, hbmOld);
    ::DeleteDC(hdcMem);

    // Push it
    PushImageForUndo(hbmCropped);

    NotifyImageChanged();
}

void ImageModel::SaveImage(LPCTSTR lpFileName)
{
    SaveDIBToFile(m_hBms[m_currInd], lpFileName, TRUE);
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
        HBITMAP hbm0 = CopyDIBImage(m_hBms[m_currInd], newWidth, newHeight);
        PushImageForUndo(hbm0);
    }
    if (nSkewDegX)
    {
        HBITMAP hbm1 = SkewDIB(m_hDrawingDC, m_hBms[m_currInd], nSkewDegX, FALSE);
        PushImageForUndo(hbm1);
    }
    if (nSkewDegY)
    {
        HBITMAP hbm2 = SkewDIB(m_hDrawingDC, m_hBms[m_currInd], nSkewDegY, TRUE);
        PushImageForUndo(hbm2);
    }
    NotifyImageChanged();
}

int ImageModel::GetWidth() const
{
    return GetDIBWidth(m_hBms[m_currInd]);
}

int ImageModel::GetHeight() const
{
    return GetDIBHeight(m_hBms[m_currInd]);
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
            if (hbm)
                PushImageForUndo(hbm);
            break;
        }
        case 2:
        {
            PushImageForUndo();
            StretchBlt(m_hDrawingDC, GetWidth() - 1, GetHeight() - 1, -GetWidth(), -GetHeight(),
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

    if (hNewBitmap)
        PushImageForUndo(hNewBitmap);
}

HBITMAP ImageModel::LockBitmap()
{
    // NOTE: An app cannot select a bitmap into more than one device context at a time.
    ::SelectObject(m_hDrawingDC, m_hbmOld); // De-select
    HBITMAP hbmLocked = m_hBms[m_currInd];
    m_hBms[m_currInd] = NULL;
    return hbmLocked;
}

void ImageModel::UnlockBitmap(HBITMAP hbmLocked)
{
    m_hBms[m_currInd] = hbmLocked;
    m_hbmOld = ::SelectObject(m_hDrawingDC, hbmLocked); // Re-select
}

void ImageModel::SelectionClone(BOOL bUndoable)
{
    if (!selectionModel.m_bShow || ::IsRectEmpty(&selectionModel.m_rc))
        return;

    if (bUndoable)
        PushImageForUndo(CopyBitmap());

    selectionModel.DrawSelection(m_hDrawingDC, paletteModel.GetBgColor(),
                                 toolsModel.IsBackgroundTransparent());
    NotifyImageChanged();
}
