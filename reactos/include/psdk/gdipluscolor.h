/*
 * Copyright (C) 2008 Google (Lei Zhang)
 *               2015 Benedikt Freisen
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

enum ColorChannelFlags
{
    ColorChannelFlagsC,
    ColorChannelFlagsM,
    ColorChannelFlagsY,
    ColorChannelFlagsK,
    ColorChannelFlagsLast
};

#ifdef __cplusplus

class Color
{
public:
    Color(VOID)
    {
        Argb = 0xff000000;
    }

    Color(ARGB argb)
    {
        Argb = argb;
    }

    Color(BYTE r, BYTE g, BYTE b)
    {
        Argb = 0xff << 24 | r << 16 | g << 8 | b;
    }

    Color(BYTE a, BYTE r, BYTE g, BYTE b)
    {
        Argb = a << 24 | r << 16 | g << 8 | b;
    }

    BYTE GetA(VOID)
    {
        return (Argb >> 24) & 0xff;
    }

    BYTE GetAlpha(VOID)
    {
        return (Argb >> 24) & 0xff;
    }

    BYTE GetB(VOID)
    {
        return Argb & 0xff;
    }

    BYTE GetBlue(VOID)
    {
        return Argb & 0xff;
    }

    BYTE GetG(VOID)
    {
        return (Argb >> 8) & 0xff;
    }

    BYTE GetGreen(VOID)
    {
        return (Argb >> 8) & 0xff;
    }

    BYTE GetR(VOID)
    {
        return (Argb >> 16) & 0xff;
    }

    BYTE GetRed(VOID)
    {
        return (Argb >> 16) & 0xff;
    }

    ARGB GetValue(VOID)
    {
        return Argb;
    }

    static ARGB MakeARGB(BYTE a, BYTE r, BYTE g, BYTE b)
    {
        return a << 24 | r << 16 | g << 8 | b;
    }

    VOID SetFromCOLORREF(COLORREF rgb)
    {
        Argb = 0xff000000 | rgb & 0x000000ff << 16 | rgb & 0x0000ff00 | rgb & 0x00ff0000 >> 16;
    }

    VOID SetValue(ARGB argb)
    {
        Argb = argb;
    }

    COLORREF ToCOLORREF(VOID)
    {
        return Argb & 0x000000ff << 16 | Argb & 0x0000ff00 | Argb & 0x00ff0000 >> 16;
    }

protected:
    ARGB Argb;
};

#else /* end of c++ typedefs */

typedef struct Color
{
    ARGB Argb;
} Color;

typedef enum ColorChannelFlags ColorChannelFlags;

#endif  /* end of c typedefs */

#endif  /* _GDIPLUSCOLOR_H */
