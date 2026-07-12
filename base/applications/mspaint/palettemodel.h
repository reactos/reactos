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
    PAL_MONOCHROME = 4,
};

/* CLASSES **********************************************************/

class PaletteModel
{
private:
    COLORREF m_colors[NUM_COLORS];
    PAL_TYPE m_nSelectedPalette;
    COLORREF m_fgColor;
    COLORREF m_bgColor;
    UINT m_bpp = 24;
    COLORREF m_primaryColor = RGB(0, 0, 0);
    COLORREF m_secondaryColor = RGB(255, 255, 255);

    void NotifyColorChanged();
    void NotifyPaletteChanged();
    void SetColorTable(UINT bpp, UINT cColors, RGBQUAD* colors);

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
    UINT GetBpp() const;
    void SetColorInfo(HBITMAP hbm);
    void SetPrimaryColors(COLORREF color0, COLORREF color1);
    COLORREF GetPrimaryColor() const { return m_primaryColor; }
    COLORREF GetSecondaryColor() const { return m_secondaryColor; }
};
