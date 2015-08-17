/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/palettemodel.h
 * PURPOSE:     Keep track of palette data, notify listeners
 * PROGRAMMERS: Benedikt Freisen
 */

/* CLASSES **********************************************************/

class PaletteModel
{
private:
    int m_colors[28];
    int m_nSelectedPalette;
    int m_fgColor;
    int m_bgColor;

    void NotifyColorChanged();
    void NotifyPaletteChanged();

public:
    PaletteModel();
    int SelectedPalette();
    void SelectPalette(int nPalette);
    int GetColor(int nIndex);
    void SetColor(int nIndex, int newColor);
    int GetFgColor();
    void SetFgColor(int newColor);
    int GetBgColor();
    void SetBgColor(int newColor);
};
