/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/history.cpp
 * PURPOSE:     Undo and redo functionality
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

void ImageModel::NotifyDimensionsChanged()
{
    imageArea.SendMessage(WM_IMAGEMODELDIMENSIONSCHANGED);
}

void ImageModel::NotifyImageChanged()
{
    imageArea.SendMessage(WM_IMAGEMODELIMAGECHANGED);
}

ImageModel::ImageModel()
{
    currInd = 0;
    undoSteps = 0;
    redoSteps = 0;
    imageSaved = TRUE;

    // prepare a minimal usable bitmap
    int imgXRes = 1;
    int imgYRes = 1;

    hDrawingDC = CreateCompatibleDC(NULL);
    SelectObject(hDrawingDC, CreatePen(PS_SOLID, 0, paletteModel.GetFgColor()));
    SelectObject(hDrawingDC, CreateSolidBrush(paletteModel.GetBgColor()));

    hBms[0] = CreateDIBWithProperties(imgXRes, imgYRes);
    SelectObject(hDrawingDC, hBms[0]);
    Rectangle(hDrawingDC, 0 - 1, 0 - 1, imgXRes + 1, imgYRes + 1);
}

void ImageModel::CopyPrevious()
{
    DPRINT("%s: %d\n", __FUNCTION__, currInd);
    DeleteObject(hBms[(currInd + 1) % HISTORYSIZE]);
    hBms[(currInd + 1) % HISTORYSIZE] = CopyDIBImage(hBms[currInd]);
    currInd = (currInd + 1) % HISTORYSIZE;
    if (undoSteps < HISTORYSIZE - 1)
        undoSteps++;
    redoSteps = 0;
    SelectObject(hDrawingDC, hBms[currInd]);
    imageSaved = FALSE;
}

void ImageModel::Undo()
{
    DPRINT("%s: %d\n", __FUNCTION__, undoSteps);
    if (undoSteps > 0)
    {
        int oldWidth = GetWidth();
        int oldHeight = GetHeight();
        selectionWindow.ShowWindow(SW_HIDE);
        currInd = (currInd + HISTORYSIZE - 1) % HISTORYSIZE;
        SelectObject(hDrawingDC, hBms[currInd]);
        undoSteps--;
        if (redoSteps < HISTORYSIZE - 1)
            redoSteps++;
        if (GetWidth() != oldWidth || GetHeight() != oldHeight)
            NotifyDimensionsChanged();
        NotifyImageChanged();
    }
}

void ImageModel::Redo()
{
    DPRINT("%s: %d\n", __FUNCTION__, redoSteps);
    if (redoSteps > 0)
    {
        int oldWidth = GetWidth();
        int oldHeight = GetHeight();
        selectionWindow.ShowWindow(SW_HIDE);
        currInd = (currInd + 1) % HISTORYSIZE;
        SelectObject(hDrawingDC, hBms[currInd]);
        redoSteps--;
        if (undoSteps < HISTORYSIZE - 1)
            undoSteps++;
        if (GetWidth() != oldWidth || GetHeight() != oldHeight)
            NotifyDimensionsChanged();
        NotifyImageChanged();
    }
}

void ImageModel::ResetToPrevious()
{
    DPRINT("%s: %d\n", __FUNCTION__, currInd);
    DeleteObject(hBms[currInd]);
    hBms[currInd] = CopyDIBImage(hBms[(currInd + HISTORYSIZE - 1) % HISTORYSIZE]);
    SelectObject(hDrawingDC, hBms[currInd]);
    NotifyImageChanged();
}

void ImageModel::ClearHistory()
{
    undoSteps = 0;
    redoSteps = 0;
}

void ImageModel::Insert(HBITMAP hbm)
{
    int oldWidth = GetWidth();
    int oldHeight = GetHeight();
    DeleteObject(hBms[(currInd + 1) % HISTORYSIZE]);
    hBms[(currInd + 1) % HISTORYSIZE] = hbm;
    currInd = (currInd + 1) % HISTORYSIZE;
    if (undoSteps < HISTORYSIZE - 1)
        undoSteps++;
    redoSteps = 0;
    SelectObject(hDrawingDC, hBms[currInd]);
    if (GetWidth() != oldWidth || GetHeight() != oldHeight)
        NotifyDimensionsChanged();
    NotifyImageChanged();
}

void ImageModel::Crop(int nWidth, int nHeight, int nOffsetX, int nOffsetY)
{
    HDC hdc;
    HPEN oldPen;
    HBRUSH oldBrush;
    int oldWidth = GetWidth();
    int oldHeight = GetHeight();

    if (nWidth <= 0)
        nWidth = 1;
    if (nHeight <= 0)
        nHeight = 1;

    SelectObject(hDrawingDC, hBms[currInd]);
    DeleteObject(hBms[(currInd + 1) % HISTORYSIZE]);
    hBms[(currInd + 1) % HISTORYSIZE] = CreateDIBWithProperties(nWidth, nHeight);
    currInd = (currInd + 1) % HISTORYSIZE;
    if (undoSteps < HISTORYSIZE - 1)
        undoSteps++;
    redoSteps = 0;

    hdc = CreateCompatibleDC(hDrawingDC);
    SelectObject(hdc, hBms[currInd]);

    oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, 1, paletteModel.GetBgColor()));
    oldBrush = (HBRUSH) SelectObject(hdc, CreateSolidBrush(paletteModel.GetBgColor()));
    Rectangle(hdc, 0, 0, nWidth, nHeight);
    BitBlt(hdc, -nOffsetX, -nOffsetY, GetWidth(), GetHeight(), hDrawingDC, 0, 0, SRCCOPY);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
    DeleteDC(hdc);
    SelectObject(hDrawingDC, hBms[currInd]);

    if (GetWidth() != oldWidth || GetHeight() != oldHeight)
        NotifyDimensionsChanged();
    NotifyImageChanged();
}

void ImageModel::SaveImage(LPTSTR lpFileName)
{
    SaveDIBToFile(hBms[currInd], lpFileName, hDrawingDC);
}

BOOL ImageModel::IsImageSaved() const
{
    return imageSaved;
}

BOOL ImageModel::HasUndoSteps() const
{
    return undoSteps > 0;
}

BOOL ImageModel::HasRedoSteps() const
{
    return redoSteps > 0;
}

void ImageModel::StretchSkew(int nStretchPercentX, int nStretchPercentY, int nSkewDegX, int nSkewDegY)
{
    int oldWidth = GetWidth();
    int oldHeight = GetHeight();
    INT newWidth = oldWidth * nStretchPercentX / 100;
    INT newHeight = oldHeight * nStretchPercentY / 100;
    Insert(CopyDIBImage(hBms[currInd], newWidth, newHeight));
    if (GetWidth() != oldWidth || GetHeight() != oldHeight)
        NotifyDimensionsChanged();
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
    CopyPrevious();
    InvertRect(hDrawingDC, &rect);
    NotifyImageChanged();
}

void ImageModel::Clear(COLORREF color)
{
    Rectangle(hDrawingDC, 0 - 1, 0 - 1, GetWidth() + 1, GetHeight() + 1);
    NotifyImageChanged();
}

HDC ImageModel::GetDC()
{
    return hDrawingDC;
}

void ImageModel::FlipHorizontally()
{
    CopyPrevious();
    StretchBlt(hDrawingDC, GetWidth() - 1, 0, -GetWidth(), GetHeight(), GetDC(), 0, 0,
               GetWidth(), GetHeight(), SRCCOPY);
    NotifyImageChanged();
}

void ImageModel::FlipVertically()
{
    CopyPrevious();
    StretchBlt(hDrawingDC, 0, GetHeight() - 1, GetWidth(), -GetHeight(), GetDC(), 0, 0,
               GetWidth(), GetHeight(), SRCCOPY);
    NotifyImageChanged();
}

void ImageModel::RotateNTimes90Degrees(int iN)
{
    if (iN == 2)
    {
        CopyPrevious();
        StretchBlt(hDrawingDC, GetWidth() - 1, GetHeight() - 1, -GetWidth(), -GetHeight(), GetDC(),
                   0, 0, GetWidth(), GetHeight(), SRCCOPY);
    }
    NotifyImageChanged();
}
