/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Keep track of palette data, notify listeners
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#pragma once

#define NUM_COLORS 28

enum PAL_TYPE
{
    PAL_MODERN = 1,
    PAL_OLDTYPE = 2,
    PAL_GRAYSCALE = 3,
    PAL_BLACKANDWHITE = 4,
};

/* CLASSES **********************************************************/

class PaletteModel
{
private:
    COLORREF m_colors[NUM_COLORS];
    PAL_TYPE m_nSelectedPalette;
    COLORREF m_fgColor;
    COLORREF m_bgColor;
    INT m_iBgColor;
    INT m_iFgColor;

    void NotifyColorChanged();
    void NotifyPaletteChanged();
    static HBRUSH CreateDitherBrush(COLORREF color);

public:
    PaletteModel();
    PAL_TYPE SelectedPalette();
    void SelectPalette(PAL_TYPE nPalette);
    COLORREF GetColor(UINT nIndex) const;
    void SetColor(UINT nIndex, COLORREF newColor);
    COLORREF GetFgColor() const;
    void SetFgColor(COLORREF newColor);
    COLORREF GetBgColor() const;
    void SetBgColor(COLORREF newColor);
    HBRUSH CreateColorBrush(UINT nIndex);
    HBRUSH CreateFgBrush() { return CreateColorBrush(m_iFgColor); }
    HBRUSH CreateBgBrush() { return CreateColorBrush(m_iBgColor); }
    INT GetFgIndex() const { return m_iFgColor; }
    INT GetBgIndex() const { return m_iBgColor; }
    void SetFgIndex(INT nIndex);
    void SetBgIndex(INT nIndex);
};
