/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Offering functions dealing with registry values
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#pragma once

#define MAX_RECENT_FILES 4

class RegistrySettings
{
private:
    void LoadPresets(INT nCmdShow);

public:
    DWORD BMPHeight;
    DWORD BMPWidth;
    DWORD GridExtent;
    DWORD NoStretching;
    DWORD ShowThumbnail;
    DWORD SnapToGrid;
    DWORD ThumbHeight;
    DWORD ThumbWidth;
    DWORD ThumbXPos;
    DWORD ThumbYPos;
    DWORD UnitSetting;
    WINDOWPLACEMENT WindowPlacement;

    CStringW strFiles[MAX_RECENT_FILES];

    CStringW strFontName;
    DWORD PointSize;
    DWORD Bold;
    DWORD Italic;
    DWORD Underline;
    DWORD CharSet;
    DWORD FontsPositionX;
    DWORD FontsPositionY;
    DWORD ShowTextTool;
    DWORD ShowStatusBar;
    DWORD ShowPalette;
    DWORD ShowToolBox;
    DWORD Bar1ID;
    DWORD Bar2ID;

// Values for Bar1ID.
// I think these values are Win2k3 mspaint compatible but sometimes not working...
#define BAR1ID_TOP    0x0000e81b
#define BAR1ID_BOTTOM 0x0000e81e

// Values for Bar2ID.
// I think these values are Win2k3 mspaint compatible but sometimes not working...
#define BAR2ID_LEFT   0x0000e81c
#define BAR2ID_RIGHT  0x0000e81d

    enum WallpaperStyle {
        TILED,
        CENTERED,
        STRETCHED
    };

    static void SetWallpaper(LPCWSTR szFileName, WallpaperStyle style);

    void Load(INT nCmdShow);
    void Store();
    void SetMostRecentFile(LPCWSTR szPathName);
};
