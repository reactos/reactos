/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Keep track of selection parameters, notify listeners
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2019-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
    COLORREF m_rgbBack;
    BOOL m_bShow;
    BOOL m_bContentChanged;
    CRect m_rc;    // in image pixel coordinates
    POINT m_ptHit; // in image pixel coordinates
    CRect m_rcOld; // in image pixel coordinates

    SelectionModel();
    ~SelectionModel();

    void ResetPtStack();
    void PushToPtStack(POINT pt);
    int PtStackSize() const;
    void SetRectFromPoints(const POINT& ptFrom, const POINT& ptTo);
    void BuildMaskFromPtStack();

    BOOL TakeOff();
    void Landing();
    BOOL IsLanded() const;
    void HideSelection();
    void DeleteSelection();

    HBITMAP CopyBitmap();
    void GetSelectionContents(HDC hDCImage);
    void DrawFramePoly(HDC hDCImage);
    void DrawBackground(HDC hDCImage);
    void DrawBackgroundPoly(HDC hDCImage, COLORREF crBg);
    void DrawBackgroundRect(HDC hDCImage, COLORREF crBg);
    void DrawSelection(HDC hDCImage, COLORREF crBg = 0, BOOL bBgTransparent = FALSE);
    void InsertFromHBITMAP(HBITMAP hbmColor, INT x = 0, INT y = 0, HBITMAP hbmMask = NULL);

    // operation
    void FlipHorizontally();
    void FlipVertically();
    void RotateNTimes90Degrees(int iN);
    void StretchSkew(int nStretchPercentX, int nStretchPercentY, int nSkewDegX, int nSkewDegY);
    void InvertSelection();

    void Dragging(HITTEST hit, POINT pt);
    void ClearMask();
    void ClearColor();
    void NotifyContentChanged();

private:
    SelectionModel(const SelectionModel&);
    SelectionModel& operator=(const SelectionModel&);

    void ShiftPtStack(INT dx, INT dy);
    void SwapWidthAndHeight();
};
