#pragma once

#include <windef.h>
#include <atlstr.h>

#define EPOCH_DIFF 116444736000000000 //FILETIME starts from 1601-01-01 UTC, UnixTime starts from 1970-01-01
#define RATE_DIFF 10000000

#ifdef _M_IX86
#define CurrentArchitecture L"x86"
#elif defined(_M_AMD64)
#define CurrentArchitecture L"amd64"
#elif defined(_M_ARM)
#define CurrentArchitecture L"arm"
#elif defined(_M_ARM64)
#define CurrentArchitecture L"arm64"
#elif defined(_M_IA64)
#define CurrentArchitecture L"ia64"
#elif defined(_M_PPC)
#define CurrentArchitecture L"ppc"
#endif

INT GetWindowWidth(HWND hwnd);
INT GetWindowHeight(HWND hwnd);
INT GetClientWindowWidth(HWND hwnd);
INT GetClientWindowHeight(HWND hwnd);

VOID CopyTextToClipboard(LPCWSTR lpszText);
VOID ShowPopupMenuEx(HWND hwnd, HWND hwndOwner, UINT MenuID, UINT DefaultItem);
VOID ShowPopupMenu(HWND hwnd, UINT MenuID, UINT DefaultItem);
BOOL StartProcess(ATL::CStringW &Path, BOOL Wait);
BOOL StartProcess(LPWSTR lpPath, BOOL Wait);
BOOL GetStorageDirectory(ATL::CStringW &lpDirectory);

VOID InitLogs();
VOID FreeLogs();
BOOL WriteLogMessage(WORD wType, DWORD dwEventID, LPCWSTR lpMsg);
BOOL GetInstalledVersion(ATL::CStringW *pszVersion, const ATL::CStringW &szRegName);

BOOL ExtractFilesFromCab(const ATL::CStringW& szCabName,
                         const ATL::CStringW& szCabDir,
                         const ATL::CStringW& szOutputDir);

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
    BOOL GetStringWorker(const ATL::CStringW& KeyName, PCWSTR Suffix, ATL::CStringW& ResultString);

public:
    CConfigParser(const ATL::CStringW& FileName = "");

    BOOL GetString(const ATL::CStringW& KeyName, ATL::CStringW& ResultString);
    BOOL GetInt(const ATL::CStringW& KeyName, INT& iResult);
};

BOOL PathAppendNoDirEscapeW(LPWSTR pszPath, LPCWSTR pszMore);

BOOL IsSystem64Bit();

INT GetSystemColorDepth();

void UnixTimeToFileTime(DWORD dwUnixTime, LPFILETIME pFileTime);

BOOL SearchPatternMatch(LPCWSTR szHaystack, LPCWSTR szNeedle);

template<class T>
class CLocalPtr : public CHeapPtr<T, CLocalAllocator>
{
};

