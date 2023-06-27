/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Undo and redo functionality
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#pragma once

/* HISTORYSIZE = number of possible undo-steps + 1 */
#define HISTORYSIZE 11

class ImageModel
{
public:
    ImageModel();
    virtual ~ImageModel();

    HDC GetDC();
    BOOL CanUndo() const { return m_undoSteps > 0; }
    BOOL CanRedo() const { return m_redoSteps > 0; }
    void PushImageForUndo(HBITMAP hbm = NULL);
    void ResetToPrevious(void);
    void Undo(BOOL bClearRedo = FALSE);
    void Redo(void);
    void ClearHistory(void);
    void Crop(int nWidth, int nHeight, int nOffsetX = 0, int nOffsetY = 0);
    void SaveImage(LPCTSTR lpFileName);
    BOOL IsImageSaved() const;
    void StretchSkew(int nStretchPercentX, int nStretchPercentY, int nSkewDegX = 0, int nSkewDegY = 0);
    int GetWidth() const;
    int GetHeight() const;
    HBITMAP CopyBitmap();
    void InvertColors();
    void FlipHorizontally();
    void FlipVertically();
    void RotateNTimes90Degrees(int iN);
    void Clamp(POINT& pt) const;
    void NotifyImageChanged();

protected:
    HDC m_hDrawingDC; // The device context for this class
    int m_currInd; // The current index in m_hBms
    int m_undoSteps; // The undo-able count
    int m_redoSteps; // The redo-able count
    HBITMAP m_hBms[HISTORYSIZE]; // A rotation buffer of HBITMAPs
    HGDIOBJ m_hbmOld;
};
