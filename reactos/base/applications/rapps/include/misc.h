#pragma once

#include <windef.h>
#include <atlstr.h>

INT GetWindowWidth(HWND hwnd);
INT GetWindowHeight(HWND hwnd);
INT GetClientWindowWidth(HWND hwnd);
INT GetClientWindowHeight(HWND hwnd);

VOID CopyTextToClipboard(LPCWSTR lpszText);
VOID SetWelcomeText(VOID);
VOID ShowPopupMenu(HWND hwnd, UINT MenuID, UINT DefaultItem);
BOOL StartProcess(ATL::CStringW &Path, BOOL Wait);
BOOL StartProcess(LPWSTR lpPath, BOOL Wait);
BOOL GetStorageDirectory(ATL::CStringW &lpDirectory);
BOOL ExtractFilesFromCab(LPCWSTR lpCabName, LPCWSTR lpOutputPath);
VOID InitLogs(VOID);
VOID FreeLogs(VOID);
BOOL WriteLogMessage(WORD wType, DWORD dwEventID, LPCWSTR lpMsg);
BOOL GetInstalledVersion(ATL::CStringW *pszVersion, const ATL::CStringW &szRegName);

class CConfigParser
{
    // Locale names cache
    const static INT m_cchLocaleSize = 5;

    static ATL::CStringW m_szLocaleID;
    static ATL::CStringW m_szCachedINISectionLocale;
    static ATL::CStringW m_szCachedINISectionLocaleNeutral;

    const ATL::CStringW szConfigPath;

    static ATL::CStringW GetINIFullPath(const ATL::CStringW& FileName);
    static VOID CacheINILocaleLazy();

public:
    static const ATL::CStringW& GetLocale();
    static INT CConfigParser::GetLocaleSize();

    CConfigParser(const ATL::CStringW& FileName);

    UINT GetString(const ATL::CStringW& KeyName, ATL::CStringW& ResultString);
    UINT GetInt(const ATL::CStringW& KeyName);
};