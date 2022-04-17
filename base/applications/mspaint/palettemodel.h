/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/palettemodel.h
 * PURPOSE:     Keep track of palette data, notify listeners
 * PROGRAMMERS: Benedikt Freisen
 */

#pragma once

#define NUM_COLORS 28

enum PAL_TYPE
{
    PAL_MODERN = 1,
    PAL_OLDTYPE = 2,
};

/* CLASSES **********************************************************/

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
