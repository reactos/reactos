/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/palettemodel.h
 * PURPOSE:     Keep track of palette data, notify listeners
 * PROGRAMMERS: Benedikt Freisen
 */

#pragma once

#define NUM_COLORS 28

/* CLASSES **********************************************************/

class PaletteModel
{
private:
    COLORREF m_colors[NUM_COLORS];
    int m_nSelectedPalette;
    int m_fgColor;
    int m_bgColor;

    void NotifyColorChanged();
    void NotifyPaletteChanged();

public:
    PaletteModel();
    int SelectedPalette();
    void SelectPalette(int nPalette);
    COLORREF GetColor(int nIndex) const;
    void SetColor(int nIndex, COLORREF newColor);
    COLORREF GetFgColor() const;
    void SetFgColor(COLORREF newColor);
    COLORREF GetBgColor() const;
    void SetBgColor(COLORREF newColor);
};
