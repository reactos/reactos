#pragma once

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

static inline UINT
ErrorFromHResult(HRESULT hr)
{
    // Attempt to extract the original Win32 error code from the HRESULT
    if (HIWORD(hr) == HIWORD(HRESULT_FROM_WIN32(!0)))
        return LOWORD(hr);
    else
        return hr >= 0 ? ERROR_SUCCESS : hr;
}

VOID
CopyTextToClipboard(LPCWSTR lpszText);
VOID
ShowPopupMenuEx(HWND hwnd, HWND hwndOwner, UINT MenuID, UINT DefaultItem, POINT *Point = NULL);
VOID
EmulateDialogReposition(HWND hwnd);
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

typedef struct
{
    const CStringW &ItemPath;
    UINT64 UncompressedSize;
    UINT FileAttributes;
} EXTRACTCALLBACKINFO;
typedef BOOL (CALLBACK*EXTRACTCALLBACK)(const EXTRACTCALLBACKINFO &Info, void *Cookie);

static inline BOOL
NotifyFileExtractCallback(const CStringW &ItemPath, UINT64 UncompressedSize, UINT FileAttributes,
                          EXTRACTCALLBACK Callback, void *Cookie)
{
    EXTRACTCALLBACKINFO eci = { ItemPath, UncompressedSize, FileAttributes };
    return Callback ? Callback(eci, Cookie) : TRUE;
}

BOOL
ExtractFilesFromCab(const CStringW &szCabName, const CStringW &szCabDir, const CStringW &szOutputDir,
                    EXTRACTCALLBACK Callback = NULL, void *Cookie = NULL);
BOOL
ExtractFilesFromCab(LPCWSTR FullCabPath, const CStringW &szOutputDir,
                    EXTRACTCALLBACK Callback = NULL, void *Cookie = NULL);

BOOL
IsSystem64Bit();

INT
GetSystemColorDepth();

void
UnixTimeToFileTime(DWORD dwUnixTime, LPFILETIME pFileTime);

BOOL
SearchPatternMatch(LPCWSTR szHaystack, LPCWSTR szNeedle);

HRESULT
RegKeyHasValues(HKEY hKey, LPCWSTR Path, REGSAM wowsam = 0);
LPCWSTR
GetRegString(CRegKey &Key, LPCWSTR Name, CStringW &Value);

bool
ExpandEnvStrings(CStringW &Str);

template <class T> static CStringW
BuildPath(const T &Base, LPCWSTR Append)
{
    CStringW path = Base;
    SIZE_T len = path.GetLength();
    if (len && path[len - 1] != L'\\' && path[len - 1] != L'/')
        path += L'\\';
    while (*Append == L'\\' || *Append == L'/')
        ++Append;
    return path + Append;
}

CStringW
SplitFileAndDirectory(LPCWSTR FullPath, CStringW *pDir = NULL);
BOOL
DeleteDirectoryTree(LPCWSTR Dir, HWND hwnd = NULL);
UINT
CreateDirectoryTree(LPCWSTR Dir);
HRESULT
GetSpecialPath(UINT csidl, CStringW &Path, HWND hwnd = NULL);
HRESULT
GetKnownPath(REFKNOWNFOLDERID kfid, CStringW &Path, DWORD Flags = KF_FLAG_CREATE);
HRESULT
GetProgramFilesPath(CStringW &Path, BOOL PerUser, HWND hwnd = NULL);

template <class T> class CLocalPtr : public CHeapPtr<T, CLocalAllocator>
{
};

struct CScopedMutex
{
    HANDLE m_hMutex;

    CScopedMutex(LPCWSTR Name, UINT Timeout = INFINITE, BOOL InitialOwner = FALSE)
    {
        m_hMutex = CreateMutexW(NULL, InitialOwner, Name);
        if (m_hMutex && !InitialOwner)
        {
            DWORD wait = WaitForSingleObject(m_hMutex, Timeout);
            if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED)
            {
                CloseHandle(m_hMutex);
                m_hMutex = NULL;
            }
        }
    }
    ~CScopedMutex()
    {
        if (m_hMutex)
        {
            ReleaseMutex(m_hMutex);
            CloseHandle(m_hMutex);
        }
    }

    bool Acquired() const { return m_hMutex != NULL; }
};
