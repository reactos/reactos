#pragma once

#include <windef.h>
#include <atlstr.h>

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

VOID CopyTextToClipboard(LPCWSTR lpszText);
VOID ShowPopupMenuEx(HWND hwnd, HWND hwndOwner, UINT MenuID, UINT DefaultItem);
BOOL StartProcess(const ATL::CStringW &Path, BOOL Wait);
BOOL GetStorageDirectory(ATL::CStringW &lpDirectory);

VOID InitLogs();
VOID FreeLogs();
BOOL WriteLogMessage(WORD wType, DWORD dwEventID, LPCWSTR lpMsg);
BOOL GetInstalledVersion(ATL::CStringW *pszVersion, const ATL::CStringW &szRegName);

BOOL ExtractFilesFromCab(const ATL::CStringW& szCabName,
                         const ATL::CStringW& szCabDir,
                         const ATL::CStringW& szOutputDir);

BOOL PathAppendNoDirEscapeW(LPWSTR pszPath, LPCWSTR pszMore);

BOOL IsSystem64Bit();

INT GetSystemColorDepth();

void UnixTimeToFileTime(DWORD dwUnixTime, LPFILETIME pFileTime);

BOOL SearchPatternMatch(LPCWSTR szHaystack, LPCWSTR szNeedle);

template<class T>
class CLocalPtr : public CHeapPtr<T, CLocalAllocator>
{
};

