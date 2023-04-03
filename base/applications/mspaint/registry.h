/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/registry.h
 * PURPOSE:     Offering functions dealing with registry values
 * PROGRAMMERS: Benedikt Freisen
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

    CString strFiles[MAX_RECENT_FILES];

    CString strFontName;
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

    static void SetWallpaper(LPCTSTR szFileName, WallpaperStyle style);

    void Load(INT nCmdShow);
    void Store();
    void SetMostRecentFile(LPCTSTR szPathName);
};
