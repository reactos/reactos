/*
 * PROJECT:     ReactOS shlwapi
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement SHAutoComplete
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include <windef.h>
#include <winreg.h>
#include <winioctl.h>
#include <winternl.h>
#include <shldisp.h>
#include <shlguid.h>
#define NO_SHLWAPI_STREAM
#include <shlwapi.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include <atlsimpcoll.h>
#include <strsafe.h>
#include <assert.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define MAX_ITEMS 50 // Maximum number of items we can get
#define MAX_TICKS 500 // We wait upto 500 milliseconds

// IEnumString interface for SHAutoComplete
class CAutoCompleteEnumString :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumString
{
public:
    CAutoCompleteEnumString() : m_iItem(0), m_hwndEdit(NULL)
    {
    }

    void Initialize(IAutoComplete2 *pAC2, DWORD dwSHACF, HWND hwndEdit);

    BOOL CanAddItem() const
    {
        if (m_items.GetSize() >= MAX_ITEMS)
            return FALSE;
        if (GetTickCount() - m_dwOldTicks >= MAX_TICKS)
            return FALSE; // Timeout
        return TRUE;
    }

    BOOL AddItem(LPCWSTR psz)
    {
        if (!CanAddItem())
            return FALSE;
        m_items.Add(psz);
        return m_items.GetSize() < MAX_ITEMS;
    }

    void ResetContent()
    {
        m_items.RemoveAll();
        m_iItem = 0;
    }

    void DoAll();
    BOOL DoFileSystem(LPCWSTR pszQuery);
    void DoTypedPaths(LPCWSTR pszQuery);
    void DoDir(LPCWSTR pszDir, BOOL bDirOnly);
    void DoDrives(BOOL bDirOnly);
    void DoURLHistory();
    void DoURLMRU();

    /* IEnumString interface */
    STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumString **ppenum);

    BEGIN_COM_MAP(CAutoCompleteEnumString)
        COM_INTERFACE_ENTRY_IID(IID_IEnumString, IEnumString)
    END_COM_MAP()

protected:
    INT m_iItem; // The position for Next and Skip
    HWND m_hwndEdit; // The EDIT control
    DWORD m_dwSHACF; // The SHACF_* flags
    CSimpleArray<CStringW> m_items; // The items we got
    DWORD m_dwOldTicks; // For checking time span
};

void CAutoCompleteEnumString::Initialize(IAutoComplete2 *pAC2, DWORD dwSHACF, HWND hwndEdit)
{
    m_hwndEdit = hwndEdit;
    m_dwSHACF = dwSHACF;
    Reset(); // Populate the items
}

STDMETHODIMP
CAutoCompleteEnumString::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    if (!rgelt || !pceltFetched)
    {
        ERR("E_POINTER\n");
        return E_POINTER;
    }

    // Initialize
    *pceltFetched = 0;
    *rgelt = NULL;

    if (m_iItem >= m_items.GetSize())
        return S_FALSE; // No more elements

    // Get the elements
    ULONG ielt;
    for (ielt = 0; ielt < celt && m_iItem < m_items.GetSize(); ++ielt, ++m_iItem)
    {
        size_t cb = (wcslen(m_items[m_iItem]) + 1) * sizeof(WCHAR);
        rgelt[ielt] = (LPWSTR)CoTaskMemAlloc(cb);
        if (!rgelt[ielt]) // Failed
        {
            ERR("Out of memory\n");
            // Clean up
            while (ielt-- > 0)
            {
                CoTaskMemFree(rgelt[ielt]);
                rgelt[ielt] = NULL;
            }
            return S_FALSE; // Failure
        }
        CopyMemory(rgelt[ielt], (LPCWSTR)m_items[m_iItem], cb);
    }
    *pceltFetched = ielt; // The number of elements we got
    return (ielt == celt) ? S_OK : S_FALSE; // Success or failure
}

STDMETHODIMP CAutoCompleteEnumString::Skip(ULONG celt)
{
    if (m_items.GetSize() < INT(m_iItem + celt))
        return S_FALSE; // Out of bound

    m_iItem += celt;
    return S_OK; // Success
}

// https://stackoverflow.com/questions/3098696/get-information-about-disk-drives-result-on-windows7-32-bit-system/3100268#3100268
static BOOL
GetDriveCharacteristics(HANDLE hDevice, ULONG *pCharacteristics)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;

    Status = NtQueryVolumeInformationFile(hDevice, &IoStatusBlock,
                                          &DeviceInfo, sizeof(DeviceInfo),
                                          FileFsDeviceInformation);
    if (Status == NO_ERROR)
    {
        *pCharacteristics = DeviceInfo.Characteristics;
        return TRUE;
    }
    return FALSE;
}

static DWORD GetDriveFlags(LPCWSTR pszRoot)
{
    WCHAR szDevice[16];
    HANDLE hDevice;
    ULONG ret = 0;

    StringCbCopyW(szDevice, sizeof(szDevice), L"\\\\.\\");
    szDevice[4] = pszRoot[0];
    szDevice[5] = L':';
    szDevice[6] = 0;
    hDevice = CreateFileW(szDevice, FILE_READ_ATTRIBUTES,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
        return ret;

    GetDriveCharacteristics(hDevice, &ret);
    CloseHandle(hDevice);
    return ret;
}

static BOOL IsSlowDrive(LPCWSTR pszRoot)
{
    DWORD ret;
    switch (GetDriveTypeW(pszRoot))
    {
        case DRIVE_REMOVABLE:
            ret = GetDriveFlags(pszRoot);
            if (!(ret & FILE_FLOPPY_DISKETTE) || (ret & FILE_VIRTUAL_VOLUME))
                break;
            return TRUE; // Floppy and non-virtual

        case DRIVE_CDROM:
            ret = GetDriveFlags(pszRoot);
            if (ret & FILE_VIRTUAL_VOLUME)
                break;
            return TRUE; // CD/DVD and non-virtual
    }
    return FALSE; // Not slow
}

BOOL CAutoCompleteEnumString::DoFileSystem(LPCWSTR pszQuery)
{
    // Check the drive
    INT nDriveNumber = PathGetDriveNumberW(pszQuery);
    if (nDriveNumber != -1)
    {
        WCHAR szRoot[] = L"C:\\";
        szRoot[0] = WCHAR('A' + nDriveNumber);
        assert(PathIsRootW(szRoot));
        if (IsSlowDrive(szRoot))
            return FALSE; // Don't scan slow drives
    }

    // Is it directory-only?
    BOOL bDirOnly = !!(m_dwSHACF & SHACF_FILESYS_DIRS);

    DWORD attrs = GetFileAttributesW(pszQuery);
    if (attrs != INVALID_FILE_ATTRIBUTES) // File or folder does exist
    {
        if (attrs & FILE_ATTRIBUTE_DIRECTORY) // Is it a directory?
        {
            size_t cch = wcslen(pszQuery);
            if (cch > 0 && pszQuery[cch - 1] == L'\\') // The last character is '\\'
            {
                DoDir(pszQuery, bDirOnly); // Scan the directory
                return FALSE; // Not exact match
            }
        }
        return TRUE; // Exact match should hide the list
    }
    else if (pszQuery[0] && wcschr(pszQuery, L'\\') != NULL)
    {
        // Non-existent but can be a partial path
        CStringW strPath = pszQuery;

        // Remove the file title part
        INT ich = strPath.ReverseFind(L'\\');
        if (ich >= 0)
            strPath = strPath.Left(ich);

        DoDir(strPath, bDirOnly); // Scan the directory
    }
    else
    {
        DoDrives(bDirOnly); // Scan drives
    }

    return FALSE; // Not exact match
}

void CAutoCompleteEnumString::DoTypedPaths(LPCWSTR pszQuery)
{
    static const LPCWSTR
        pszTypedPaths = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\TypedPaths";
    WCHAR szName[32], szValue[MAX_PATH + 32];

    INT cch = (INT)wcslen(pszQuery); // The length of pszQuery
    if (cch <= 0)
    {
        TRACE("cch <= 0\n");
        return;
    }

    // Open the registry key
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, pszTypedPaths, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS)
    {
        TRACE("Opening TypedPaths failed: 0x%lX\n", result);
        return;
    }

    for (DWORD i = 1; i <= MAX_ITEMS; ++i) // For all the URL entries
    {
        if (!CanAddItem())
            break;

        // Build a registry value name
        StringCbPrintfW(szName, sizeof(szName), L"url%lu", i);

        // Read a registry value
        DWORD cbValue = sizeof(szValue), dwType;
        result = RegQueryValueExW(hKey, szName, NULL, &dwType, (LPBYTE)szValue, &cbValue);
        if (result == ERROR_SUCCESS && dwType == REG_SZ) // Could I read it?
        {
            if (!PathFileExistsW(szValue))
                continue; // File or folder doesn't exist
            if ((m_dwSHACF & SHACF_FILESYS_DIRS) && !PathIsDirectoryW(szValue))
                continue; // Directory-only and not a directory

            CStringW strPath = szValue;
            strPath = strPath.Left(cch);
            if (_wcsicmp(pszQuery, strPath) == 0) // Matched
            {
                if (!AddItem(szValue))
                    break;
            }
        }
    }

    RegCloseKey(hKey); // Close the registry key
}

void CAutoCompleteEnumString::DoAll()
{
    ResetContent(); // Clear all the items

    if (!IsWindow(m_hwndEdit)) // Check whether m_hwndEdit is valid
    {
        if (m_hwndEdit)
        {
            TRACE("m_hwndEdit was invalid\n");
            m_hwndEdit = NULL;
        }
        return;
    }

    // Get text from the EDIT control
    INT cchMax = GetWindowTextLengthW(m_hwndEdit) + 1;
    CString strText;
    LPWSTR pszText = strText.GetBuffer(cchMax);
    GetWindowTextW(m_hwndEdit, pszText, cchMax);
    strText.ReleaseBuffer();

    // Populate the items for filesystem and typed paths
    if (m_dwSHACF & (SHACF_FILESYS_ONLY | SHACF_FILESYSTEM | SHACF_FILESYS_DIRS))
    {
        if (!DoFileSystem(strText) && CanAddItem())
            DoTypedPaths(strText);
    }

    // Populate the items for URLs
    if (!(m_dwSHACF & SHACF_FILESYS_ONLY)) // Not filesystem-only
    {
        if (CanAddItem() && (m_dwSHACF & SHACF_URLHISTORY)) // History URLs
            DoURLHistory();
        if (CanAddItem() && (m_dwSHACF & SHACF_URLMRU)) // Most-Recently-Used URLs
            DoURLMRU();
    }
}

STDMETHODIMP CAutoCompleteEnumString::Reset()
{
    m_dwOldTicks = GetTickCount();
    DoAll();
    return S_OK; // Always return S_OK
}

STDMETHODIMP CAutoCompleteEnumString::Clone(IEnumString **ppenum)
{
    if (!ppenum)
    {
        ERR("E_POINTER\n");
        return E_POINTER;
    }

    CAutoCompleteEnumString *pES = new CComObject<CAutoCompleteEnumString>();
    pES->AddRef();
    pES->m_iItem = m_iItem;
    pES->m_dwSHACF = m_dwSHACF;
    pES->m_items = m_items;
    *ppenum = pES;
    return S_OK;
}

/* "." or ".." ? */
#define IS_DOTS(sz) (sz[0] == L'.' && (sz[1] == 0 || (sz[1] == L'.' && sz[2] == 0)))

void CAutoCompleteEnumString::DoDir(LPCWSTR pszDir, BOOL bDirOnly)
{
    // Add backslash
    CStringW strPath = pszDir;
    strPath += L'\\';

    // Build a path with wildcard
    CStringW strSpec = strPath + L"\\*";

    // Start the enumeration
    WIN32_FIND_DATAW find;
    HANDLE hFind = FindFirstFileW(strSpec, &find);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (IS_DOTS(find.cFileName) || (find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            continue; // "." and ".." and hidden files are invisible
        if (bDirOnly && !(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue; // Directory-only and not a directory

        // Build a path
        CStringW strNewPath = strPath;
        strNewPath += find.cFileName;

        if (!AddItem(strNewPath))
            break;
    } while (FindNextFileW(hFind, &find));

    FindClose(hFind); // End the enumeration
}

void CAutoCompleteEnumString::DoDrives(BOOL bDirOnly)
{
    // For all the drives
    WCHAR szRoot[4] = L"C:\\";
    for (DWORD i = 0, dwBits = GetLogicalDrives(); i <= L'Z' - L'A'; ++i)
    {
        if ((dwBits & (1 << i)) == 0)
            continue; // The drive doesn't exist

        szRoot[0] = (WCHAR)(L'A' + i); // Build a root path of the drive
        assert(PathIsRootW(szRoot));
        if (!AddItem(szRoot))
            break;
    }
}

void CAutoCompleteEnumString::DoURLHistory()
{
    static const LPCWSTR
        pszTypedURLs = L"Software\\Microsoft\\Internet Explorer\\TypedURLs";
    WCHAR szName[32], szValue[MAX_PATH + 32];

    // Open the registry key
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, pszTypedURLs, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS)
    {
        TRACE("Opening TypedURLs failed: 0x%lX\n", result);
        return;
    }

    for (DWORD i = 1; i <= MAX_ITEMS; ++i) // For all the URL entries
    {
        if (!CanAddItem())
            break;

        // Build a registry value name
        StringCbPrintfW(szName, sizeof(szName), L"url%lu", i);

        // Read a registry value
        DWORD cbValue = sizeof(szValue), dwType;
        result = RegQueryValueExW(hKey, szName, NULL, &dwType, (LPBYTE)szValue, &cbValue);
        if (result == ERROR_SUCCESS && dwType == REG_SZ) // Could I read it?
        {
            if (UrlIsW(szValue, URLIS_URL)) // Is it a URL?
            {
                if (!AddItem(szValue))
                    break;
            }
        }
    }

    RegCloseKey(hKey); // Close the registry key
}

void CAutoCompleteEnumString::DoURLMRU()
{
    static const LPCWSTR
        pszRunMRU = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU";
    WCHAR szName[2], szMRUList[64], szValue[MAX_PATH + 32];

    // Open the registry key
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, pszRunMRU, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS)
    {
        TRACE("Opening RunMRU failed: 0x%lX\n", result);
        return;
    }

    // Read the MRUList
    DWORD cbValue = sizeof(szMRUList), dwType;
    result = RegQueryValueExW(hKey, L"MRUList", NULL, &dwType, (LPBYTE)szMRUList, &cbValue);
    if (result != ERROR_SUCCESS || dwType != REG_SZ)
    {
        RegCloseKey(hKey); // Close the registry key
        return;
    }

    for (DWORD i = 0; i <= L'z' - L'a' && szMRUList[i]; ++i) // for all the MRU items
    {
        if (!CanAddItem())
            break;

        // Build a registry value name
        szName[0] = szMRUList[i];
        szName[1] = 0;

        // Read a registry value
        cbValue = sizeof(szValue);
        result = RegQueryValueExW(hKey, szName, NULL, &dwType, (LPBYTE)szValue, &cbValue);
        if (result != ERROR_SUCCESS || dwType != REG_SZ)
            continue;

        // Fix up for special case of "\\1"
        size_t cch = wcslen(szValue);
        if (cch >= 2 && wcscmp(&szValue[cch - 2], L"\\1") == 0)
            szValue[cch - 2] = 0;

        if (UrlIsW(szValue, URLIS_URL)) // Is it a URL?
        {
            if (!AddItem(szValue))
                break;
        }
    }

    RegCloseKey(hKey); // Close the registry key
}

static BOOL
AutoComplete_AdaptFlags(HWND hwndEdit, LPDWORD pdwACO, LPDWORD pdwSHACF)
{
    static const LPCWSTR s_pszAutoComplete =
        L"Software\\Microsoft\\Internet Explorer\\AutoComplete";
    WCHAR szValue[8];

    DWORD dwSHACF = *pdwSHACF, dwACO = 0;
    if (dwSHACF == SHACF_DEFAULT)
        dwSHACF = SHACF_FILESYSTEM | SHACF_URLALL;

    if (dwSHACF & SHACF_AUTOAPPEND_FORCE_OFF)
    {
        // do nothing
    }
    else if (dwSHACF & SHACF_AUTOAPPEND_FORCE_ON)
    {
        dwACO |= ACO_AUTOAPPEND;
    }
    else
    {
        DWORD dwType, cbValue = sizeof(szValue);
        if (SHGetValueW(HKEY_CURRENT_USER, s_pszAutoComplete, L"Append Completion",
                        &dwType, szValue, &cbValue) != ERROR_SUCCESS ||
            dwType != REG_SZ || _wcsicmp(szValue, L"no") != 0)
        {
            dwACO |= ACO_AUTOSUGGEST;
        }
    }

    if (dwSHACF & SHACF_AUTOSUGGEST_FORCE_OFF)
    {
        // do nothing
    }
    else if (dwSHACF & SHACF_AUTOSUGGEST_FORCE_ON)
    {
        dwACO |= ACO_AUTOSUGGEST;
    }
    else
    {
        DWORD dwType, cbValue = sizeof(szValue);
        if (SHGetValueW(HKEY_CURRENT_USER, s_pszAutoComplete, L"AutoSuggest",
                        &dwType, szValue, &cbValue) != ERROR_SUCCESS ||
            dwType != REG_SZ || _wcsicmp(szValue, L"no") != 0)
        {
            dwACO |= ACO_AUTOSUGGEST;
        }
    }

    if (dwSHACF & SHACF_USETAB)
        dwACO |= ACO_USETAB;

    if (GetWindowLongPtr(hwndEdit, GWL_EXSTYLE) & WS_EX_RTLREADING)
        dwACO |= ACO_RTLREADING;

    if (!(dwACO & (ACO_AUTOSUGGEST | ACO_AUTOAPPEND)))
    {
        ERR("dwACO: 0x%lX\n", dwACO);
        return FALSE;
    }

    *pdwACO = dwACO;
    *pdwSHACF = dwSHACF;
    return TRUE;
}

/*************************************************************************
 *      SHAutoComplete  	[SHLWAPI.@]
 *
 * Enable auto-completion for an edit control.
 *
 * PARAMS
 *  hwndEdit [I] Handle of control to enable auto-completion for
 *  dwFlags  [I] SHACF_ flags from "shlwapi.h"
 *
 * RETURNS
 *  Success: S_OK. Auto-completion is enabled for the control.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI SHAutoComplete(HWND hwndEdit, DWORD dwFlags)
{
    TRACE("SHAutoComplete(%p, 0x%lX)\n", hwndEdit, dwFlags);

    DWORD dwACO;
    if (!AutoComplete_AdaptFlags(hwndEdit, &dwACO, &dwFlags))
        return S_OK;

    CComPtr<IAutoComplete2> pAC2;
    const DWORD dwClsCtx = CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER;
    HRESULT hr = CoCreateInstance(CLSID_AutoComplete, NULL, dwClsCtx,
                                  IID_IAutoComplete, (LPVOID *)&pAC2);
    if (FAILED(hr))
    {
        ERR("CoCreateInstance(CLSID_AutoComplete) failed: 0x%lX\n", hr);
        return hr;
    }

    CComPtr<CAutoCompleteEnumString> pES(new CComObject<CAutoCompleteEnumString>());
    pES->Initialize(pAC2, dwFlags, hwndEdit);

    hr = pAC2->Init(hwndEdit, pES, NULL, L"www.%s.com");
    if (SUCCEEDED(hr))
        pAC2->SetOptions(dwACO);
    else
        ERR("IAutoComplete2::Init failed: 0x%lX\n", hr);
    return hr;
}
