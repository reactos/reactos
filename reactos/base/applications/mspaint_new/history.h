/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/history.h
 * PURPOSE:     Undo and redo functionality
 * PROGRAMMERS: Benedikt Freisen
 */

class ImageModel
{
private:
    void NotifyDimensionsChanged();
    void NotifyImageChanged();
public:
    HBITMAP hBms[HISTORYSIZE];
    int currInd;
    int undoSteps;
    int redoSteps;
    BOOL imageSaved;

    void CopyPrevious(void);
    void Undo(void);
    void Redo(void);
    void ResetToPrevious(void);
    void ClearHistory(void);
    void Insert(HBITMAP hbm);
    void Crop(int nWidth, int nHeight, int nOffsetX = 0, int nOffsetY = 0);
    void SaveImage(LPTSTR lpFileName);
    BOOL IsImageSaved();
    BOOL HasUndoSteps();
    BOOL HasRedoSteps();
    void StretchSkew(int nStretchPercentX, int nStretchPercentY, int nSkewDegX = 0, int nSkewDegY = 0);
    int GetWidth();
    int GetHeight();
    void InvertColors();
    void Clear(COLORREF color = 0x00ffffff);
};
