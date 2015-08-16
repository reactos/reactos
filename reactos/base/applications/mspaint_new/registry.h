/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/registry.h
 * PURPOSE:     Offering functions dealing with registry values
 * PROGRAMMERS: Benedikt Freisen
 */

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

    static void SetWallpaper(TCHAR *szFileName, DWORD dwStyle, DWORD dwTile);

    void Load();
    void Store();
};
