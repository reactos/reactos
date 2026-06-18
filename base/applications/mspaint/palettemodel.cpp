/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Keep track of palette data, notify listeners
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2021-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

PaletteModel paletteModel;

/* FUNCTIONS ********************************************************/

ColorBrush::~ColorBrush()
{
    if (m_hBrush)
        DeleteObject(m_hBrush);
}

HBRUSH ColorBrush::GetColorBrush(PAL_TYPE palette, COLORREF rgbColor)
{
    if (m_hBrush &&
        m_palette == palette &&
        m_rgbColor == rgbColor)
    {
        return m_hBrush;
    }

    if (m_hBrush)
        ::DeleteObject(m_hBrush);

    if (palette == PAL_MONOCHROME)
        m_hBrush = CreateDitherBrush(rgbColor, RGB(0, 0, 0), RGB(255, 255, 255));
    else
        m_hBrush = CreateSolidBrush(rgbColor);

    m_palette = palette;
    m_rgbColor = rgbColor;
    return m_hBrush;
}

PaletteModel::PaletteModel()
{
    m_fgColor = RGB(0, 0, 0);
    m_bgColor = RGB(255, 255, 255);
    SelectPalette(PAL_MODERN);
}

PAL_TYPE PaletteModel::SelectedPalette()
{
    return m_nSelectedPalette;
}

void PaletteModel::SelectPalette(PAL_TYPE nPalette)
{
    static const COLORREF modernColors[NUM_COLORS] =
    {
        0x000000, 0x464646, 0x787878, 0x300099, 0x241ced, 0x0078ff, 0x0ec2ff,
        0x00f2ff, 0x1de6a8, 0x4cb122, 0xefb700, 0xf36d4d, 0x99362f, 0x98316f,
        0xffffff, 0xdcdcdc, 0xb4b4b4, 0x3c5a9c, 0xb1a3ff, 0x7aaae5, 0x9ce4f5,
        0xbdf9ff, 0xbcf9d3, 0x61bb9d, 0xead999, 0xd19a70, 0x8e6d54, 0xd5a5b5
    };
    static const COLORREF oldColors[NUM_COLORS] =
    {
        0x000000, 0x808080, 0x000080, 0x008080, 0x008000, 0x808000, 0x800000,
        0x800080, 0x408080, 0x404000, 0xff8000, 0x804000, 0xff0040, 0x004080,
        0xffffff, 0xc0c0c0, 0x0000ff, 0x00ffff, 0x00ff00, 0xffff00, 0xff0000,
        0xff00ff, 0x80ffff, 0x80ff00, 0xffff80, 0xff8080, 0x8000ff, 0x4080ff
    };
    static const COLORREF grayscale[NUM_COLORS] =
    {
        0x000000, 0x090909, 0x121212, 0x1C1C1C, 0x252525, 0x2F2F2F, 0x383838,
        0x424242, 0x4B4B4B, 0x555555, 0x5E5E5E, 0x676767, 0x717171, 0x7B7B7B,
        0xFFFFFF, 0xF6F6F6, 0xEDEDED, 0xE3E3E3, 0xD9D9D9, 0xD0D0D0, 0xC7C7C7,
        0xBDBDBD, 0xB4B4B4, 0xAAAAAA, 0xA1A1A1, 0x979797, 0x8E8E8E, 0x848484
    };
    switch (nPalette)
    {
        case PAL_MODERN:
            CopyMemory(m_colors, modernColors, sizeof(m_colors));
            break;
        case PAL_OLDTYPE:
            CopyMemory(m_colors, oldColors, sizeof(m_colors));
            break;
        case PAL_GRAYSCALE:
        case PAL_MONOCHROME:
            CopyMemory(m_colors, grayscale, sizeof(m_colors));
            break;
    }
    m_nSelectedPalette = nPalette;
    m_fgColor = RGB(0, 0, 0);
    m_bgColor = RGB(255, 255, 255);
    NotifyPaletteChanged();
}

COLORREF PaletteModel::GetColor(UINT nIndex) const
{
    if (nIndex < NUM_COLORS)
        return m_colors[nIndex];
    else
        return 0;
}

void PaletteModel::SetColor(UINT nIndex, COLORREF newColor)
{
    if (nIndex < NUM_COLORS)
    {
        m_colors[nIndex] = newColor;
        NotifyPaletteChanged();
    }
}

COLORREF PaletteModel::GetFgColor() const
{
    return m_fgColor;
}

void PaletteModel::SetFgColor(COLORREF newColor)
{
    m_fgColor = newColor;
    NotifyColorChanged();
}

COLORREF PaletteModel::GetBgColor() const
{
    return m_bgColor;
}

void PaletteModel::SetBgColor(COLORREF newColor)
{
    m_bgColor = newColor;
    NotifyColorChanged();
}

void PaletteModel::NotifyColorChanged()
{
    if (paletteWindow.IsWindow())
        paletteWindow.SendMessage(WM_PALETTEMODELCOLORCHANGED);
    if (canvasWindow.IsWindow())
        canvasWindow.SendMessage(WM_PALETTEMODELCOLORCHANGED);
    if (textEditWindow.IsWindow())
        textEditWindow.SendMessage(WM_PALETTEMODELCOLORCHANGED);
}

void PaletteModel::NotifyPaletteChanged()
{
    if (paletteWindow.IsWindow())
        paletteWindow.Invalidate(FALSE);
}
