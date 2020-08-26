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

enum SETTINGS_FIELDTYPE
{
    SettingsFieldInt,
    SettingsFieldBool,
    SettingsFieldString,
};

typedef struct _SettingsRegEntry
{
    size_t Offset;  // Offset of the field in SETTINGS_INFO struct
    SETTINGS_FIELDTYPE FieldType;    // Type of the field.
    ULONG cchStrlen; // string length. only used when FieldType == SettingsFieldString
    const WCHAR *RegKeyName; // The key name of this field in registery.
}SETTINGS_REG_ENTRY;

BOOL LoadSettings(PSETTINGS_INFO pSettingsInfo);
VOID SaveSettings(HWND hwnd, PSETTINGS_INFO pSettingsInfo);
VOID FillDefaultSettings(PSETTINGS_INFO pSettingsInfo);

extern SETTINGS_INFO SettingsInfo;
