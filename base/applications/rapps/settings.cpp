/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Functions to load / save settings from reg.
 * COPYRIGHT:   Copyright 2020 He Yang (1160386205@qq.com)
 */

#include "rapps.h"
#include "settings.h"

#define SETTINGSSUBKEY L"Software\\ReactOS\\" RAPPS_NAME

class SettingsField
{
  public:
    virtual ~SettingsField()
    {
        ;
    }
    virtual BOOL
    Save(CRegKey &key) = 0;
    virtual BOOL
    Load(CRegKey &key) = 0;
};

class SettingsFieldBool : public SettingsField
{
  public:
    SettingsFieldBool(BOOL *pValue, LPCWSTR szRegName) : m_pValueStore(pValue), m_RegName(szRegName)
    {
    }

    virtual BOOL
    Save(CRegKey &key) override
    {
        return key.SetDWORDValue(m_RegName, (DWORD)(*m_pValueStore)) == ERROR_SUCCESS;
    }
    virtual BOOL
    Load(CRegKey &key) override
    {
        DWORD dwField;
        LONG lResult = key.QueryDWORDValue(m_RegName, dwField);
        if (lResult != ERROR_SUCCESS)
        {
            return FALSE;
        }
        *m_pValueStore = (BOOL)dwField;
        return TRUE;
    }

  private:
    BOOL *m_pValueStore; // where to read/store the value
    LPCWSTR m_RegName;   // key name in registery
};

class SettingsFieldInt : public SettingsField
{
  public:
    SettingsFieldInt(INT *pValue, LPCWSTR szRegName) : m_pValueStore(pValue), m_RegName(szRegName)
    {
    }

    virtual BOOL
    Save(CRegKey &key) override
    {
        return key.SetDWORDValue(m_RegName, (DWORD)(*m_pValueStore)) == ERROR_SUCCESS;
    }
    virtual BOOL
    Load(CRegKey &key) override
    {
        DWORD dwField;
        LONG lResult = key.QueryDWORDValue(m_RegName, dwField);
        if (lResult != ERROR_SUCCESS)
        {
            return FALSE;
        }
        *m_pValueStore = (INT)dwField;
        return TRUE;
    }

  private:
    INT *m_pValueStore; // where to read/store the value
    LPCWSTR m_RegName;  // key name in registery
};

class SettingsFieldString : public SettingsField
{
  public:
    SettingsFieldString(WCHAR *pString, ULONG cchLen, LPCWSTR szRegName)
        : m_pStringStore(pString), m_StringLen(cchLen), m_RegName(szRegName)
    {
    }

    virtual BOOL
    Save(CRegKey &key) override
    {
        return key.SetStringValue(m_RegName, m_pStringStore) == ERROR_SUCCESS;
    }
    virtual BOOL
    Load(CRegKey &key) override
    {
        ULONG nChar = m_StringLen - 1; // make sure the terminating L'\0'
        LONG lResult = key.QueryStringValue(m_RegName, m_pStringStore, &nChar);
        return lResult == ERROR_SUCCESS;
    }

  private:
    WCHAR *m_pStringStore; // where to read/store the value
    ULONG m_StringLen;     // string length, in chars
    LPCWSTR m_RegName;     // key name in registery
};

static void
AddInfoFields(ATL::CAtlList<SettingsField *> &infoFields, SETTINGS_INFO &settings)
{
    infoFields.AddTail(new SettingsFieldBool(&(settings.bSaveWndPos), L"bSaveWndPos"));
    infoFields.AddTail(new SettingsFieldBool(&(settings.bUpdateAtStart), L"bUpdateAtStart"));
    infoFields.AddTail(new SettingsFieldBool(&(settings.bLogEnabled), L"bLogEnabled"));
    infoFields.AddTail(new SettingsFieldString(settings.szDownloadDir, MAX_PATH, L"szDownloadDir"));
    infoFields.AddTail(new SettingsFieldBool(&(settings.bDelInstaller), L"bDelInstaller"));
    infoFields.AddTail(new SettingsFieldBool(&(settings.Maximized), L"WindowPosMaximized"));
    infoFields.AddTail(new SettingsFieldInt(&(settings.Left), L"WindowPosLeft"));
    infoFields.AddTail(new SettingsFieldInt(&(settings.Top), L"WindowPosTop"));
    infoFields.AddTail(new SettingsFieldInt(&(settings.Width), L"WindowPosWidth"));
    infoFields.AddTail(new SettingsFieldInt(&(settings.Height), L"WindowPosHeight"));
    infoFields.AddTail(new SettingsFieldInt(&(settings.Proxy), L"ProxyMode"));
    infoFields.AddTail(new SettingsFieldString((settings.szProxyServer), MAX_PATH, L"ProxyServer"));
    infoFields.AddTail(new SettingsFieldString((settings.szNoProxyFor), MAX_PATH, L"NoProxyFor"));
    infoFields.AddTail(new SettingsFieldBool(&(settings.bUseSource), L"bUseSource"));
    infoFields.AddTail(new SettingsFieldString((settings.szSourceURL), INTERNET_MAX_URL_LENGTH, L"SourceURL"));
}

static BOOL
SaveAllSettings(CRegKey &key, SETTINGS_INFO &settings)
{
    BOOL bAllSuccess = TRUE;
    ATL::CAtlList<SettingsField *> infoFields;

    AddInfoFields(infoFields, settings);

    POSITION InfoListPosition = infoFields.GetHeadPosition();
    while (InfoListPosition)
    {
        SettingsField *Info = infoFields.GetNext(InfoListPosition);
        if (!Info->Save(key))
        {
            bAllSuccess = FALSE;
            // TODO: error log
        }
        delete Info;
    }
    return bAllSuccess;
}

static BOOL
LoadAllSettings(CRegKey &key, SETTINGS_INFO &settings)
{
    BOOL bLoadedAny = FALSE;
    ATL::CAtlList<SettingsField *> infoFields;

    AddInfoFields(infoFields, settings);

    POSITION InfoListPosition = infoFields.GetHeadPosition();
    while (InfoListPosition)
    {
        SettingsField *Info = infoFields.GetNext(InfoListPosition);
        if (Info->Load(key))
            bLoadedAny = TRUE;
        //else
        //  TODO: error log
        delete Info;
    }
    return bLoadedAny;
}

static void
GetDefaultDownloadDirectory(CStringW &szDownloadDir)
{
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szDownloadDir.GetBuffer(MAX_PATH))))
    {
        szDownloadDir.ReleaseBuffer();
        if (!szDownloadDir.GetEnvironmentVariableW(L"SystemDrive"))
        {
            szDownloadDir = L"C:";
        }
    }
    else
    {
        szDownloadDir.ReleaseBuffer();
    }

    PathAppendW(szDownloadDir.GetBuffer(MAX_PATH), L"\\RAPPS Downloads");
    szDownloadDir.ReleaseBuffer();
}

static VOID
ValidateStringSettings(PSETTINGS_INFO pSettingsInfo)
{
    if (!pSettingsInfo->szDownloadDir[0])
    {
        CStringW szDownloadDir;
        GetDefaultDownloadDirectory(szDownloadDir);

        CStringW::CopyChars(pSettingsInfo->szDownloadDir, _countof(pSettingsInfo->szDownloadDir),
                            szDownloadDir.GetString(), szDownloadDir.GetLength() + 1);
    }
}

VOID
FillDefaultSettings(PSETTINGS_INFO pSettingsInfo)
{
    ZeroMemory(pSettingsInfo, sizeof(*pSettingsInfo));

    pSettingsInfo->bSaveWndPos = TRUE;
    pSettingsInfo->bUpdateAtStart = FALSE;
    pSettingsInfo->bLogEnabled = TRUE;
    pSettingsInfo->bUseSource = FALSE;
    pSettingsInfo->bDelInstaller = FALSE;
    pSettingsInfo->Maximized = FALSE;
    pSettingsInfo->Left = CW_USEDEFAULT;
    pSettingsInfo->Top = CW_USEDEFAULT;
    pSettingsInfo->Width = 680;
    pSettingsInfo->Height = 450;

    ValidateStringSettings(pSettingsInfo);
}

BOOL
LoadSettings(PSETTINGS_INFO pSettingsInfo)
{
    BOOL bLoadedAny = FALSE;

    FillDefaultSettings(pSettingsInfo);

    ATL::CRegKey RegKey;
    if (RegKey.Open(HKEY_CURRENT_USER, SETTINGSSUBKEY, KEY_READ) == ERROR_SUCCESS)
    {
        bLoadedAny = LoadAllSettings(RegKey, *pSettingsInfo);
    }

    ValidateStringSettings(pSettingsInfo); // Handles the case where a REG_SZ is present but empty

    if (!bLoadedAny)
    {
        // This the first launch, write at least one item so ParseCmdAndExecute() does not
        // trigger another DB update in another process instance between now and SaveSettings().
        ATL::CRegKey RegKey;
        if (RegKey.Create(HKEY_CURRENT_USER, SETTINGSSUBKEY, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_WRITE, NULL, NULL) == ERROR_SUCCESS)
        {
            SettingsFieldBool field(&(pSettingsInfo->bUpdateAtStart), L"bUpdateAtStart");
            field.Save(RegKey);
        }
    }
    return bLoadedAny;
}

BOOL
SaveSettings(HWND hwnd, PSETTINGS_INFO pSettingsInfo)
{
    WINDOWPLACEMENT wp;
    ATL::CRegKey RegKey;

    if (pSettingsInfo->bSaveWndPos)
    {
        wp.length = sizeof(wp);
        GetWindowPlacement(hwnd, &wp);

        pSettingsInfo->Left = wp.rcNormalPosition.left;
        pSettingsInfo->Top = wp.rcNormalPosition.top;
        pSettingsInfo->Width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
        pSettingsInfo->Height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
        pSettingsInfo->Maximized =
            (wp.showCmd == SW_MAXIMIZE || (wp.showCmd == SW_SHOWMINIMIZED && (wp.flags & WPF_RESTORETOMAXIMIZED)));
    }

    if (RegKey.Create(HKEY_CURRENT_USER, SETTINGSSUBKEY, NULL, REG_OPTION_NON_VOLATILE,
                      KEY_WRITE, NULL, NULL) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    return SaveAllSettings(RegKey, *pSettingsInfo);
}
