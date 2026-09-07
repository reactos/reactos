// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


struct ColorPalette
{
    UINT Flags;             // Palette flags
    UINT Count;             // Number of color entries
    ARGB Entries[1];        // Palette color entries
};

class BitmapData
{
public:
    UINT Width;
    UINT Height;
    INT Stride;
    MilPixelFormat::Enum PixelFormat;
    VOID* Scan0;
    UINT_PTR Reserved;
};


