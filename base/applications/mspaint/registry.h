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
