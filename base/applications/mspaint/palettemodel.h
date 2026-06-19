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

class ColorBrush
{
protected:
    COLORREF m_rgbColor = RGB(0, 0, 0);
    PAL_TYPE m_palette = PAL_MODERN;
    HBRUSH m_hBrush = NULL;

public:
    ~ColorBrush();

    HBRUSH GetColorBrush(PAL_TYPE palette, COLORREF rgbColor);
};

class PaletteModel
{
private:
    COLORREF m_colors[NUM_COLORS];
    PAL_TYPE m_nSelectedPalette;
    COLORREF m_fgColor;
    COLORREF m_bgColor;

    void NotifyColorChanged();
    void NotifyPaletteChanged();

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
};
