/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Keep track of palette data, notify listeners
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

PaletteModel paletteModel;

/* FUNCTIONS ********************************************************/

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
        0x000000, 0x090909, 0x131313, 0x1C1C1C, 0x262626, 0x2F2F2F, 0x393939,
        0x424242, 0x4C4C4C, 0x555555, 0x5E5E5E, 0x686868, 0x717171, 0x7B7B7B,
        0xFFFFFF, 0xF6F6F6, 0xECECEC, 0xE3E3E3, 0xD9D9D9, 0xD0D0D0, 0xC6C6C6,
        0xBDBDBD, 0xB3B3B3, 0xAAAAAA, 0xA1A1A1, 0x979797, 0x8E8E8E, 0x848484
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

HBRUSH PaletteModel::CreateDitherBrush(COLORREF color)
{
    // 8x8 Bayer ordered dithering matrix (0 to 63)
    static const BYTE s_bayerMatrix[8][8] =
    {
        {  0, 32,  8, 40,  2, 34, 10, 42 },
        { 48, 16, 56, 24, 50, 18, 58, 26 },
        { 12, 44,  4, 36, 14, 46,  6, 38 },
        { 60, 28, 52, 20, 62, 30, 54, 22 },
        {  3, 35, 11, 43,  1, 33,  9, 41 },
        { 51, 19, 59, 27, 49, 17, 57, 25 },
        { 15, 47,  7, 39, 13, 45,  5, 37 },
        { 63, 31, 55, 23, 61, 29, 53, 21 },
    };
    static const COLORREF rgbBlack = RGB(0, 0, 0);
    static const COLORREF rgbWhite = RGB(255, 255, 255);
    INT sum = GetRValue(color) + GetGValue(color) + GetBValue(color);
    INT brightness = sum / 3;
    if (brightness < 0)
        brightness = 0;
    if (brightness >= 255)
        brightness = 256;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth    = 8;
    bmi.bmiHeader.biHeight   = -8; // Top-down
    bmi.bmiHeader.biPlanes   = 1;
    bmi.bmiHeader.biBitCount = 24;
    BYTE pixels[8 * 8 * 3];

    for (INT y = 0; y < 8; ++y)
    {
        for (INT x = 0; x < 8; ++x)
        {
            INT threshold = s_bayerMatrix[y][x] * 255 / 63;

            INT idx = (y * 8 + x) * 3;
            if (brightness > threshold)
            {
                pixels[idx + 0] = GetBValue(rgbWhite);  // Blue
                pixels[idx + 1] = GetGValue(rgbWhite);  // Green
                pixels[idx + 2] = GetRValue(rgbWhite);  // Red
            }
            else
            {
                pixels[idx + 0] = GetBValue(rgbBlack);  // Blue
                pixels[idx + 1] = GetGValue(rgbBlack);  // Green
                pixels[idx + 2] = GetRValue(rgbBlack);  // Red
            }
        }
    }

    HDC hdc = GetDC(NULL);
    HBITMAP hBitmap = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, pixels, &bmi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    if (!hBitmap)
        return NULL;

    HBRUSH hBrush = CreatePatternBrush(hBitmap);
    DeleteObject(hBitmap);

    return hBrush;
}

HBRUSH PaletteModel::CreateColorBrush(COLORREF color)
{
    if (m_nSelectedPalette == PAL_MONOCHROME)
        return CreateDitherBrush(color);
    else
        return CreateSolidBrush(color);
}
