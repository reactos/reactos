/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/selectionmodel.h
 * PURPOSE:     Keep track of selection parameters, notify listeners
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

#pragma once

class SelectionModel
{
private:
    HBITMAP m_hbmColor;
    HBITMAP m_hbmMask;
    POINT *m_ptStack;
    int m_iPtSP;

public:
    BOOL m_bShow;
    CRect m_rc;    // in image pixel coordinates
    POINT m_ptHit; // in image pixel coordinates

    SelectionModel();
    ~SelectionModel();

    void ResetPtStack();
    void PushToPtStack(POINT pt);
    int PtStackSize() const;
    void SetRectFromPoints(const POINT& ptFrom, const POINT& ptTo);
    void BuildMaskFromPtStack();

    BOOL TakeOff();
    void Landing();

    HBITMAP GetBitmap();
    void GetSelectionContents(HDC hDCImage);
    void DrawFramePoly(HDC hDCImage);
    void DrawBackgroundPoly(HDC hDCImage, COLORREF crBg);
    void DrawBackgroundRect(HDC hDCImage, COLORREF crBg);
    void DrawSelection(HDC hDCImage, LPCRECT prc, COLORREF crBg = 0, BOOL bBgTransparent = FALSE);
    void InsertFromHBITMAP(HBITMAP hBm, INT x = 0, INT y = 0);

    // operation
    void FlipHorizontally();
    void FlipVertically();
    void RotateNTimes90Degrees(int iN);
    void StretchSkew(int nStretchPercentX, int nStretchPercentY, int nSkewDegX, int nSkewDegY);

    void CancelSelection();
    void NotifyRefreshNeeded();
    void Dragging(CANVAS_HITTEST hit, POINT pt);
    void ClearMask();
    void ClearColor();

private:
    SelectionModel(const SelectionModel&);
    SelectionModel& operator=(const SelectionModel&);

    void ShiftPtStack(BOOL bPlus);
};
