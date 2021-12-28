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
    int m_colors[NUM_COLORS];
    int m_nSelectedPalette;
    int m_fgColor;
    int m_bgColor;

    void NotifyColorChanged();
    void NotifyPaletteChanged();

public:
    PaletteModel();
    int SelectedPalette();
    void SelectPalette(int nPalette);
    int GetColor(int nIndex) const;
    void SetColor(int nIndex, int newColor);
    int GetFgColor() const;
    void SetFgColor(int newColor);
    int GetBgColor() const;
    void SetBgColor(int newColor);
};
