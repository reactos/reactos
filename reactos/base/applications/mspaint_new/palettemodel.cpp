/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/palettemodel.cpp
 * PURPOSE:     Keep track of palette data, notify listeners
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

PaletteModel::PaletteModel()
{
    m_fgColor = 0x00000000;
    m_bgColor = 0x00ffffff;
    SelectPalette(1);
}

int PaletteModel::SelectedPalette()
{
    return m_nSelectedPalette;
}

void PaletteModel::SelectPalette(int nPalette)
{
    int modernColors[28] = {
        0x000000, 0x464646, 0x787878, 0x300099, 0x241ced, 0x0078ff, 0x0ec2ff,
        0x00f2ff, 0x1de6a8, 0x4cb122, 0xefb700, 0xf36d4d, 0x99362f, 0x98316f,
        0xffffff, 0xdcdcdc, 0xb4b4b4, 0x3c5a9c, 0xb1a3ff, 0x7aaae5, 0x9ce4f5,
        0xbdf9ff, 0xbcf9d3, 0x61bb9d, 0xead999, 0xd19a70, 0x8e6d54, 0xd5a5b5
    };
    int oldColors[28] = {
        0x000000, 0x808080, 0x000080, 0x008080, 0x008000, 0x808000, 0x800000,
        0x800080, 0x408080, 0x404000, 0xff8000, 0x804000, 0xff0040, 0x004080,
        0xffffff, 0xc0c0c0, 0x0000ff, 0x00ffff, 0x00ff00, 0xffff00, 0xff0000,
        0xff00ff, 0x80ffff, 0x80ff00, 0xffff80, 0xff8080, 0x8000ff, 0x4080ff
    };
    if (nPalette == 1)
        CopyMemory(m_colors, modernColors, sizeof(m_colors));
    else if (nPalette == 2)
        CopyMemory(m_colors, oldColors, sizeof(m_colors));
    else
        return;
    m_nSelectedPalette = nPalette;
}

int PaletteModel::GetColor(int nIndex)
{
    if (nIndex < 28)
        return m_colors[nIndex];
    else
        return 0;
}

void PaletteModel::SetColor(int nIndex, int newColor)
{
    if (nIndex < 28)
        m_colors[nIndex] = newColor;
}

int PaletteModel::GetFgColor()
{
    return m_fgColor;
}

void PaletteModel::SetFgColor(int newColor)
{
    m_fgColor = newColor;
}

int PaletteModel::GetBgColor()
{
    return m_bgColor;
}

void PaletteModel::SetBgColor(int newColor)
{
    m_bgColor = newColor;
}
