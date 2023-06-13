/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/history.cpp
 * PURPOSE:     Undo and redo functionality
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

#include "precomp.h"

ImageModel imageModel;

/* FUNCTIONS ********************************************************/

void ImageModel::NotifyImageChanged()
{
    if (canvasWindow.IsWindow())
        canvasWindow.Invalidate(FALSE);
}

ImageModel::ImageModel()
    : hDrawingDC(::CreateCompatibleDC(NULL))
    , currInd(0)
    , undoSteps(0)
    , redoSteps(0)
{
    ZeroMemory(hBms, sizeof(hBms));

    hBms[0] = CreateDIBWithProperties(1, 1);
    ::SelectObject(hDrawingDC, hBms[0]);

    imageSaved = TRUE;
}

ImageModel::~ImageModel()
{
    ::DeleteDC(hDrawingDC);

    for (size_t i = 0; i < HISTORYSIZE; ++i)
    {
        if (hBms[i])
            ::DeleteObject(hBms[i]);
    }
}

void ImageModel::Undo(BOOL bClearRedo)
{
    ATLTRACE("%s: %d\n", __FUNCTION__, undoSteps);
    if (!CanUndo())
        return;

    selectionModel.m_bShow = FALSE;

    // Select previous item
    currInd = (currInd + HISTORYSIZE - 1) % HISTORYSIZE;
    ::SelectObject(hDrawingDC, hBms[currInd]);

    undoSteps--;
    if (bClearRedo)
        redoSteps = 0;
    else if (redoSteps < HISTORYSIZE - 1)
        redoSteps++;

    NotifyImageChanged();
}

void ImageModel::Redo()
{
    ATLTRACE("%s: %d\n", __FUNCTION__, redoSteps);
    if (!CanRedo())
        return;

    selectionModel.m_bShow = FALSE;

    // Select next item
    currInd = (currInd + 1) % HISTORYSIZE;
    ::SelectObject(hDrawingDC, hBms[currInd]);

    redoSteps--;
    if (undoSteps < HISTORYSIZE - 1)
        undoSteps++;

    NotifyImageChanged();
}

void ImageModel::ResetToPrevious()
{
    ATLTRACE("%s: %d\n", __FUNCTION__, currInd);

    // Revert current item with previous item
    ::DeleteObject(hBms[currInd]);
    hBms[currInd] = CopyDIBImage(hBms[(currInd + HISTORYSIZE - 1) % HISTORYSIZE]);
    ::SelectObject(hDrawingDC, hBms[currInd]);

    NotifyImageChanged();
}

void ImageModel::ClearHistory()
{
    undoSteps = 0;
    redoSteps = 0;
}

void ImageModel::PushImageForUndo(HBITMAP hbm)
{
    ATLTRACE("%s: %d\n", __FUNCTION__, currInd);

    // Go to the next item with an HBITMAP or current item
    ::DeleteObject(hBms[(currInd + 1) % HISTORYSIZE]);
    hBms[(currInd + 1) % HISTORYSIZE] = (hbm ? hbm : CopyDIBImage(hBms[currInd]));
    currInd = (currInd + 1) % HISTORYSIZE;
    ::SelectObject(hDrawingDC, hBms[currInd]);

    if (undoSteps < HISTORYSIZE - 1)
        undoSteps++;
    redoSteps = 0;

    imageSaved = FALSE;
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
    HDC hdcMem = ::CreateCompatibleDC(hDrawingDC);
    HGDIOBJ hbmOld = ::SelectObject(hdcMem, hbmCropped);

    // Fill background of the HBITMAP
    RECT rcBack = { 0, 0, nWidth, nHeight };
    HBRUSH hbrBack = ::CreateSolidBrush(paletteModel.GetBgColor());
    ::FillRect(hdcMem, &rcBack, hbrBack);
    ::DeleteObject(hbrBack);

    // Copy the old content
    ::BitBlt(hdcMem, -nOffsetX, -nOffsetY, GetWidth(), GetHeight(), hDrawingDC, 0, 0, SRCCOPY);

    // Clean up
    ::SelectObject(hdcMem, hbmOld);
    ::DeleteDC(hdcMem);

    // Push it
    PushImageForUndo(hbmCropped);

    NotifyImageChanged();
}

void ImageModel::SaveImage(LPCTSTR lpFileName)
{
    SaveDIBToFile(hBms[currInd], lpFileName, hDrawingDC);
}

BOOL ImageModel::IsImageSaved() const
{
    return imageSaved;
}

void ImageModel::StretchSkew(int nStretchPercentX, int nStretchPercentY, int nSkewDegX, int nSkewDegY)
{
    int oldWidth = GetWidth();
    int oldHeight = GetHeight();
    INT newWidth = oldWidth * nStretchPercentX / 100;
    INT newHeight = oldHeight * nStretchPercentY / 100;
    if (oldWidth != newWidth || oldHeight != newHeight)
    {
        HBITMAP hbm0 = CopyDIBImage(hBms[currInd], newWidth, newHeight);
        PushImageForUndo(hbm0);
    }
    if (nSkewDegX)
    {
        HBITMAP hbm1 = SkewDIB(hDrawingDC, hBms[currInd], nSkewDegX, FALSE);
        PushImageForUndo(hbm1);
    }
    if (nSkewDegY)
    {
        HBITMAP hbm2 = SkewDIB(hDrawingDC, hBms[currInd], nSkewDegY, TRUE);
        PushImageForUndo(hbm2);
    }
    NotifyImageChanged();
}

int ImageModel::GetWidth() const
{
    return GetDIBWidth(hBms[currInd]);
}

int ImageModel::GetHeight() const
{
    return GetDIBHeight(hBms[currInd]);
}

void ImageModel::InvertColors()
{
    RECT rect = {0, 0, GetWidth(), GetHeight()};
    PushImageForUndo();
    InvertRect(hDrawingDC, &rect);
    NotifyImageChanged();
}

HDC ImageModel::GetDC()
{
    return hDrawingDC;
}

void ImageModel::FlipHorizontally()
{
    PushImageForUndo();
    StretchBlt(hDrawingDC, GetWidth() - 1, 0, -GetWidth(), GetHeight(), GetDC(), 0, 0,
               GetWidth(), GetHeight(), SRCCOPY);
    NotifyImageChanged();
}

void ImageModel::FlipVertically()
{
    PushImageForUndo();
    StretchBlt(hDrawingDC, 0, GetHeight() - 1, GetWidth(), -GetHeight(), GetDC(), 0, 0,
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
            HBITMAP hbm = Rotate90DegreeBlt(hDrawingDC, GetWidth(), GetHeight(), iN == 1, FALSE);
            if (hbm)
                PushImageForUndo(hbm);
            break;
        }
        case 2:
        {
            PushImageForUndo();
            StretchBlt(hDrawingDC, GetWidth() - 1, GetHeight() - 1, -GetWidth(), -GetHeight(),
                       hDrawingDC, 0, 0, GetWidth(), GetHeight(), SRCCOPY);
            break;
        }
    }
    NotifyImageChanged();
}

void ImageModel::DeleteSelection()
{
    if (!selectionModel.m_bShow)
        return;

    selectionModel.TakeOff();
    selectionModel.m_bShow = FALSE;
    selectionModel.ClearColor();
    selectionModel.ClearMask();
    NotifyImageChanged();
}

void ImageModel::Bound(POINT& pt)
{
    pt.x = max(0, min(pt.x, GetWidth()));
    pt.y = max(0, min(pt.y, GetHeight()));
}
