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
private:
    void NotifyDimensionsChanged();
    void NotifyImageChanged();
    HDC hDrawingDC;
public:
    HBITMAP hBms[HISTORYSIZE];
private:
    int currInd;
    int undoSteps;
    int redoSteps;
public:
    ImageModel();
    void CopyPrevious(void);
    void Undo(void);
    void Redo(void);
    void ResetToPrevious(void);
    void ClearHistory(void);
    void Insert(HBITMAP hbm);
    void Crop(int nWidth, int nHeight, int nOffsetX = 0, int nOffsetY = 0);
    void SaveImage(LPTSTR lpFileName);
    BOOL IsImageSaved() const;
    BOOL HasUndoSteps() const;
    BOOL HasRedoSteps() const;
    void StretchSkew(int nStretchPercentX, int nStretchPercentY, int nSkewDegX = 0, int nSkewDegY = 0);
    int GetWidth() const;
    int GetHeight() const;
    void InvertColors();
    void Clear(COLORREF color = 0x00ffffff);
    HDC GetDC();
    void FlipHorizontally();
    void FlipVertically();
    void RotateNTimes90Degrees(int iN);
};
