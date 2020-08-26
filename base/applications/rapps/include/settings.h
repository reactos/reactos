#pragma once
#include <windef.h>
#include <wininet.h>

struct SETTINGS_INFO
{
    BOOL bSaveWndPos;
    BOOL bUpdateAtStart;
    BOOL bLogEnabled;
    WCHAR szDownloadDir[MAX_PATH];
    BOOL bDelInstaller;
    /* Window Pos */
    BOOL Maximized;
    INT Left;
    INT Top;
    INT Width;
    INT Height;
    /* Proxy settings */
    INT Proxy;
    WCHAR szProxyServer[MAX_PATH];
    WCHAR szNoProxyFor[MAX_PATH];
    /* Software source settings */
    BOOL bUseSource;
    WCHAR szSourceURL[INTERNET_MAX_URL_LENGTH];
};

typedef SETTINGS_INFO *PSETTINGS_INFO;

BOOL LoadSettings();
VOID SaveSettings(HWND hwnd);
VOID FillDefaultSettings(PSETTINGS_INFO pSettingsInfo);

extern SETTINGS_INFO SettingsInfo;
