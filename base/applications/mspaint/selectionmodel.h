/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/selectionmodel.h
 * PURPOSE:     Keep track of selection parameters, notify listeners
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

#pragma once

/* DEFINES **********************************************************/

#define ACTION_MOVE                 0
#define ACTION_RESIZE_TOP_LEFT      1
#define ACTION_RESIZE_TOP           2
#define ACTION_RESIZE_TOP_RIGHT     3
#define ACTION_RESIZE_LEFT          4
#define ACTION_RESIZE_RIGHT         5
#define ACTION_RESIZE_BOTTOM_LEFT   6
#define ACTION_RESIZE_BOTTOM        7
#define ACTION_RESIZE_BOTTOM_RIGHT  8

/* CLASSES **********************************************************/

class SelectionModel
{
private:
    HDC m_hDC;
    RECT m_rcSrc;
    RECT m_rcDest;
    HBITMAP m_hBm;
    HBITMAP m_hMask;
    POINT *m_ptStack;
    int m_iPtSP;

//     void NotifySelectionChanging();
//     void NotifySelectionChanged();
    void NotifyRefreshNeeded();

public:
    SelectionModel();
    ~SelectionModel();
    void ResetPtStack();
    void PushToPtStack(LONG x, LONG y);
    void CalculateBoundingBoxAndContents(HDC hDCImage);
    void CalculateContents(HDC hDCImage);
    void DrawBackgroundPoly(HDC hDCImage, COLORREF crBg);
    void DrawBackgroundRect(HDC hDCImage, COLORREF crBg);
    void DrawSelection(HDC hDCImage, COLORREF crBg = 0, BOOL bBgTransparent = FALSE);
    void DrawSelectionStretched(HDC hDCImage);
    void ScaleContentsToFit();
    void InsertFromHBITMAP(HBITMAP hBm);
    void FlipHorizontally();
    void FlipVertically();
    void RotateNTimes90Degrees(int iN);
    HBITMAP GetBitmap() const;
    int PtStackSize() const;
    void DrawFramePoly(HDC hDCImage);
    void SetSrcAndDestRectFromPoints(const POINT& ptFrom, const POINT& ptTo);
    void SetSrcRectSizeToZero();
    BOOL IsSrcRectSizeNonzero() const;
    void ModifyDestRect(POINT& ptDelta, int iAction);
    LONG GetDestRectWidth() const;
    LONG GetDestRectHeight() const;
    LONG GetDestRectLeft() const;
    LONG GetDestRectTop() const;
    void GetRect(LPRECT prc) const;
    void DrawTextToolText(HDC hDCImage, COLORREF crFg, COLORREF crBg, BOOL bBgTransparent = FALSE);

private:
    SelectionModel(const SelectionModel&);
    SelectionModel& operator=(const SelectionModel&);
};
