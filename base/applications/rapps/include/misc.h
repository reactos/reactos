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

VOID
CopyTextToClipboard(LPCWSTR lpszText);
VOID
ShowPopupMenuEx(HWND hwnd, HWND hwndOwner, UINT MenuID, UINT DefaultItem);
BOOL
StartProcess(const CStringW &Path, BOOL Wait);
BOOL
GetStorageDirectory(CStringW &lpDirectory);

VOID
InitLogs();
VOID
FreeLogs();
BOOL
WriteLogMessage(WORD wType, DWORD dwEventID, LPCWSTR lpMsg);
BOOL
GetInstalledVersion(CStringW *pszVersion, const CStringW &szRegName);

BOOL
ExtractFilesFromCab(const CStringW &szCabName, const CStringW &szCabDir, const CStringW &szOutputDir);

BOOL
IsSystem64Bit();

INT
GetSystemColorDepth();

void
UnixTimeToFileTime(DWORD dwUnixTime, LPFILETIME pFileTime);

BOOL
SearchPatternMatch(LPCWSTR szHaystack, LPCWSTR szNeedle);

template <class T> class CLocalPtr : public CHeapPtr<T, CLocalAllocator>
{
};
