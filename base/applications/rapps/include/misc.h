#pragma once

#include <windef.h>
#include <atlstr.h>

INT GetWindowWidth(HWND hwnd);
INT GetWindowHeight(HWND hwnd);
INT GetClientWindowWidth(HWND hwnd);
INT GetClientWindowHeight(HWND hwnd);

VOID CopyTextToClipboard(LPCWSTR lpszText);
VOID SetWelcomeText();
VOID ShowPopupMenu(HWND hwnd, UINT MenuID, UINT DefaultItem);
BOOL StartProcess(ATL::CStringW &Path, BOOL Wait);
BOOL StartProcess(LPWSTR lpPath, BOOL Wait);
BOOL GetStorageDirectory(ATL::CStringW &lpDirectory);
BOOL ExtractFilesFromCab(LPCWSTR lpCabName, LPCWSTR lpOutputPath);
VOID InitLogs();
VOID FreeLogs();
BOOL WriteLogMessage(WORD wType, DWORD dwEventID, LPCWSTR lpMsg);
BOOL GetInstalledVersion(ATL::CStringW *pszVersion, const ATL::CStringW &szRegName);

class CConfigParser
{
    // Locale names cache
    const static INT m_cchLocaleSize = 5;

    ATL::CStringW m_szLocaleID;
    ATL::CStringW m_szCachedINISectionLocale;
    ATL::CStringW m_szCachedINISectionLocaleNeutral;

    const ATL::CStringW szConfigPath;

    ATL::CStringW GetINIFullPath(const ATL::CStringW& FileName);
    VOID CacheINILocale();

public:
    CConfigParser(const ATL::CStringW& FileName = "");

    UINT GetString(const ATL::CStringW& KeyName, ATL::CStringW& ResultString);
    UINT GetInt(const ATL::CStringW& KeyName);
};
