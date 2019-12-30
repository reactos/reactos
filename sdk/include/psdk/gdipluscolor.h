/*
 * Copyright (C) 2008 Google (Lei Zhang)
 *               2015 Benedikt Freisen
 *               2019 Katayama Hirofumi MZ
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _GDIPLUSCOLOR_H
#define _GDIPLUSCOLOR_H

typedef enum ColorChannelFlags
{
    ColorChannelFlagsC,
    ColorChannelFlagsM,
    ColorChannelFlagsY,
    ColorChannelFlagsK,
    ColorChannelFlagsLast
} ColorChannelFlags;

#ifdef __cplusplus

class Color
{
  public:
    Color() : Argb(0xff000000)
    {
    }

    Color(ARGB argb) : Argb(argb)
    {
    }

    Color(BYTE r, BYTE g, BYTE b) : Argb(MakeARGB(0xFF, r, g, b))
    {
    }

    Color(BYTE a, BYTE r, BYTE g, BYTE b) : Argb(MakeARGB(a, r, g, b))
    {
    }

    BYTE
    GetA() const
    {
        return (BYTE)(Argb >> 24);
    }

    BYTE
    GetAlpha() const
    {
        return (BYTE)(Argb >> 24);
    }

    BYTE
    GetB() const
    {
        return (BYTE)Argb;
    }

    BYTE
    GetBlue() const
    {
        return (BYTE)Argb;
    }

    BYTE
    GetG() const
    {
        return (BYTE)(Argb >> 8);
    }

    BYTE
    GetGreen() const
    {
        return (BYTE)(Argb >> 8);
    }

    BYTE
    GetR() const
    {
        return (BYTE)(Argb >> 16);
    }

    BYTE
    GetRed() const
    {
        return (BYTE)(Argb >> 16);
    }

    ARGB
    GetValue() const
    {
        return Argb;
    }

    static ARGB
    MakeARGB(BYTE a, BYTE r, BYTE g, BYTE b)
    {
        ARGB a0 = a, r0 = r, g0 = g, b0 = b;
        return (a0 << AlphaShift) | (r0 << RedShift) | (g0 << GreenShift) | (b0 << BlueShift);
    }

    VOID
    SetFromCOLORREF(COLORREF rgb)
    {
        Argb = MakeARGB(0xFF, GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));
    }

    VOID
    SetValue(ARGB argb)
    {
        Argb = argb;
    }

    COLORREF
    ToCOLORREF() const
    {
        return RGB(GetRed(), GetGreen(), GetBlue());
    }

    enum
    {
        AlphaShift = 24,
        RedShift = 16,
        GreenShift = 8,
        BlueShift = 0
    };

    enum
    {
        AlphaMask = 0xFF000000,
        RedMask = 0x00FF0000,
        GreenMask = 0x0000FF00,
        BlueMask = 0x000000FF
    };

    enum
    {
        AliceBlue = 0xFFF0F8FF,
        AntiqueWhite = 0xFFFAEBD7,
        Aqua = 0xFF00FFFF,
        Aquamarine = 0xFF7FFFD4,
        Azure = 0xFFF0FFFF,
        Beige = 0xFFF5F5DC,
        Bisque = 0xFFFFE4C4,
        Black = 0xFF000000,
        BlanchedAlmond = 0xFFFFEBCD,
        Blue = 0xFF0000FF,
        BlueViolet = 0xFF8A2BE2,
        Brown = 0xFFA52A2A,
        BurlyWood = 0xFFDEB887,
        CadetBlue = 0xFF5F9EA0,
        Chartreuse = 0xFF7FFF00,
        Chocolate = 0xFFD2691E,
        Coral = 0xFFFF7F50,
        CornflowerBlue = 0xFF6495ED,
        Cornsilk = 0xFFFFF8DC,
        Crimson = 0xFFDC143C,
        Cyan = 0xFF00FFFF,
        DarkBlue = 0xFF00008B,
        DarkCyan = 0xFF008B8B,
        DarkGoldenrod = 0xFFB8860B,
        DarkGray = 0xFFA9A9A9,
        DarkGreen = 0xFF006400,
        DarkKhaki = 0xFFBDB76B,
        DarkMagenta = 0xFF8B008B,
        DarkOliveGreen = 0xFF556B2F,
        DarkOrange = 0xFFFF8C00,
        DarkOrchid = 0xFF9932CC,
        DarkRed = 0xFF8B0000,
        DarkSalmon = 0xFFE9967A,
        DarkSeaGreen = 0xFF8FBC8F,
        DarkSlateBlue = 0xFF483D8B,
        DarkSlateGray = 0xFF2F4F4F,
        DarkTurquoise = 0xFF00CED1,
        DarkViolet = 0xFF9400D3,
        DeepPink = 0xFFFF1493,
        DeepSkyBlue = 0xFF00BFFF,
        DimGray = 0xFF696969,
        DodgerBlue = 0xFF1E90FF,
        Firebrick = 0xFFB22222,
        FloralWhite = 0xFFFFFAF0,
        ForestGreen = 0xFF228B22,
        Fuchsia = 0xFFFF00FF,
        Gainsboro = 0xFFDCDCDC,
        GhostWhite = 0xFFF8F8FF,
        Gold = 0xFFFFD700,
        Goldenrod = 0xFFDAA520,
        Gray = 0xFF808080,
        Green = 0xFF008000,
        GreenYellow = 0xFFADFF2F,
        Honeydew = 0xFFF0FFF0,
        HotPink = 0xFFFF69B4,
        IndianRed = 0xFFCD5C5C,
        Indigo = 0xFF4B0082,
        Ivory = 0xFFFFFFF0,
        Khaki = 0xFFF0E68C,
        Lavender = 0xFFE6E6FA,
        LavenderBlush = 0xFFFFF0F5,
        LawnGreen = 0xFF7CFC00,
        LemonChiffon = 0xFFFFFACD,
        LightBlue = 0xFFADD8E6,
        LightCoral = 0xFFF08080,
        LightCyan = 0xFFE0FFFF,
        LightGoldenrodYellow = 0xFFFAFAD2,
        LightGray = 0xFFD3D3D3,
        LightGreen = 0xFF90EE90,
        LightPink = 0xFFFFB6C1,
        LightSalmon = 0xFFFFA07A,
        LightSeaGreen = 0xFF20B2AA,
        LightSkyBlue = 0xFF87CEFA,
        LightSlateGray = 0xFF778899,
        LightSteelBlue = 0xFFB0C4DE,
        LightYellow = 0xFFFFFFE0,
        Lime = 0xFF00FF00,
        LimeGreen = 0xFF32CD32,
        Linen = 0xFFFAF0E6,
        Magenta = 0xFFFF00FF,
        Maroon = 0xFF800000,
        MediumAquamarine = 0xFF66CDAA,
        MediumBlue = 0xFF0000CD,
        MediumOrchid = 0xFFBA55D3,
        MediumPurple = 0xFF9370DB,
        MediumSeaGreen = 0xFF3CB371,
        MediumSlateBlue = 0xFF7B68EE,
        MediumSpringGreen = 0xFF00FA9A,
        MediumTurquoise = 0xFF48D1CC,
        MediumVioletRed = 0xFFC71585,
        MidnightBlue = 0xFF191970,
        MintCream = 0xFFF5FFFA,
        MistyRose = 0xFFFFE4E1,
        Moccasin = 0xFFFFE4B5,
        NavajoWhite = 0xFFFFDEAD,
        Navy = 0xFF000080,
        OldLace = 0xFFFDF5E6,
        Olive = 0xFF808000,
        OliveDrab = 0xFF6B8E23,
        Orange = 0xFFFFA500,
        OrangeRed = 0xFFFF4500,
        Orchid = 0xFFDA70D6,
        PaleGoldenrod = 0xFFEEE8AA,
        PaleGreen = 0xFF98FB98,
        PaleTurquoise = 0xFFAFEEEE,
        PaleVioletRed = 0xFFDB7093,
        PapayaWhip = 0xFFFFEFD5,
        PeachPuff = 0xFFFFDAB9,
        Peru = 0xFFCD853F,
        Pink = 0xFFFFC0CB,
        Plum = 0xFFDDA0DD,
        PowderBlue = 0xFFB0E0E6,
        Purple = 0xFF800080,
        Red = 0xFFFF0000,
        RosyBrown = 0xFFBC8F8F,
        RoyalBlue = 0xFF4169E1,
        SaddleBrown = 0xFF8B4513,
        Salmon = 0xFFFA8072,
        SandyBrown = 0xFFF4A460,
        SeaGreen = 0xFF2E8B57,
        SeaShell = 0xFFFFF5EE,
        Sienna = 0xFFA0522D,
        Silver = 0xFFC0C0C0,
        SkyBlue = 0xFF87CEEB,
        SlateBlue = 0xFF6A5ACD,
        SlateGray = 0xFF708090,
        Snow = 0xFFFFFAFA,
        SpringGreen = 0xFF00FF7F,
        SteelBlue = 0xFF4682B4,
        Tan = 0xFFD2B48C,
        Teal = 0xFF008080,
        Thistle = 0xFFD8BFD8,
        Tomato = 0xFFFF6347,
        Transparent = 0x00FFFFFF,
        Turquoise = 0xFF40E0D0,
        Violet = 0xFFEE82EE,
        Wheat = 0xFFF5DEB3,
        White = 0xFFFFFFFF,
        WhiteSmoke = 0xFFF5F5F5,
        Yellow = 0xFFFFFF00,
        YellowGreen = 0xFF9ACD32
    };

  protected:
    ARGB Argb;
};

#else /* end of c++ typedefs */

typedef struct Color
{
    ARGB Argb;
} Color;

#endif /* end of c typedefs */

#endif /* _GDIPLUSCOLOR_H */
