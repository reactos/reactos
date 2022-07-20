/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/registry.h
 * PURPOSE:     Offering functions dealing with registry values
 * PROGRAMMERS: Benedikt Freisen
 */

#pragma once

class RegistrySettings
{
private:
    void LoadPresets();

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

    CString strFile1;
    CString strFile2;
    CString strFile3;
    CString strFile4;

    CString strFontName;
    DWORD PointSize;
    DWORD Bold;
    DWORD Italic;
    DWORD Underline;
    DWORD CharSet;
    DWORD FontsPositionX;
    DWORD FontsPositionY;
    DWORD ShowTextTool;

    enum WallpaperStyle {
        TILED,
        CENTERED,
        STRETCHED
    };

    static void SetWallpaper(LPCTSTR szFileName, WallpaperStyle style);

    void Load();
    void Store();
    void SetMostRecentFile(LPCTSTR szPathName);
};
