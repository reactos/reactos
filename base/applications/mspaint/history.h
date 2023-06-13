/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/history.h
 * PURPOSE:     Undo and redo functionality
 * PROGRAMMERS: Benedikt Freisen
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
    BOOL CanUndo() const { return undoSteps > 0; }
    BOOL CanRedo() const { return redoSteps > 0; }
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
    void InvertColors();
    void FlipHorizontally();
    void FlipVertically();
    void RotateNTimes90Degrees(int iN);
    void DeleteSelection();
    void Bound(POINT& pt);

protected:
    HDC hDrawingDC; // The device context for this class
    int currInd; // The current index
    int undoSteps; // The undo-able count
    int redoSteps; // The redo-able count
    HBITMAP hBms[HISTORYSIZE]; // A rotation buffer of HBITMAPs

    void NotifyImageChanged();
};
