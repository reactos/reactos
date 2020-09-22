/*
 * PROJECT:     ReactOS shlwapi
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement SHAutoComplete
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include <windef.h>
#include <winreg.h>
#include <shldisp.h>
#include <shlguid.h>
#define NO_SHLWAPI_STREAM
#include <shlwapi.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include <atlsimpcoll.h>
#include <strsafe.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define MAX_ITEMS 50

class CAutoCompleteEnumString :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumString
{
public:
    CAutoCompleteEnumString()
        : m_iItem(0)
        , m_hwndEdit(NULL)
        , m_dwSHACF(0)
    {
    }

    void Initialize(IAutoComplete2 *pAC2, DWORD dwSHACF, HWND hwndEdit);

    BOOL AddString(LPCWSTR psz)
    {
        if (m_items.GetSize() >= MAX_ITEMS)
            return FALSE;
        m_items.Add(psz);
        return (m_items.GetSize() < MAX_ITEMS);
    }

    void ResetContent()
    {
        m_items.RemoveAll();
        m_iItem = 0;
    }

    void DoAll();
    void DoFileSystem(LPCWSTR pszQuery);
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
    INT m_iItem;
    HWND m_hwndEdit;
    DWORD m_dwSHACF;
    CSimpleArray<CStringW> m_items;
};

void CAutoCompleteEnumString::Initialize(IAutoComplete2 *pAC2, DWORD dwSHACF, HWND hwndEdit)
{
    m_iItem = 0;
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
                CoTaskMemFree(rgelt[ielt]);
            return S_FALSE;
        }
        CopyMemory(rgelt[ielt], (LPCWSTR)m_items[m_iItem], cb);
    }
    *pceltFetched = ielt; // The number of elements we got

    return (ielt == celt) ? S_OK : S_FALSE;
}

STDMETHODIMP CAutoCompleteEnumString::Skip(ULONG celt)
{
    if (m_items.GetSize() <= INT(m_iItem + celt))
        return S_FALSE; // Out of bound

    m_iItem += celt;
    return S_OK;
}

void CAutoCompleteEnumString::DoFileSystem(LPCWSTR pszQuery)
{
    // Check the drive
    INT nDriveNumber = PathGetDriveNumberW(pszQuery);
    if (nDriveNumber != -1)
    {
        WCHAR szRoot[] = L"C:\\";
        szRoot[0] = WCHAR('A' + nDriveNumber);
        switch (GetDriveTypeW(szRoot))
        {
            case DRIVE_REMOTE: case DRIVE_RAMDISK: case DRIVE_FIXED:
                break;
            default:
                return; // Don't scan slow drives
        }
    }

    // Is it directory-only?
    BOOL bDirOnly = !!(m_dwSHACF & SHACF_FILESYS_DIRS);

    DWORD attrs = GetFileAttributesW(pszQuery);
    if (attrs != INVALID_FILE_ATTRIBUTES) 
    {
        // File or folder does exist
        if (attrs & FILE_ATTRIBUTE_DIRECTORY)
            DoDir(pszQuery, bDirOnly); // Scan the directory
        else
            AddString(pszQuery); // A normal file
    }
    else if (pszQuery[0] && wcschr(pszQuery, L'\\') != NULL)
    {
        // Non-existent but can be a partial path
        WCHAR szPath[MAX_PATH];
        StringCbCopyW(szPath, sizeof(szPath), pszQuery);
        PathRemoveFileSpecW(szPath); // Remove the file title part

        DoDir(szPath, bDirOnly); // Scan the directory
    }
    else
    {
        // Scan drives for an empty query
        DoDrives(bDirOnly);
    }
}

void CAutoCompleteEnumString::DoAll()
{
    // Clear all the items
    ResetContent();

    // Check whether m_hwndEdit is valid
    if (!IsWindow(m_hwndEdit))
    {
        TRACE("m_hwndEdit was invalid\n");
        return;
    }

    // Get text from the EDIT control
    WCHAR szText[MAX_PATH];
    GetWindowTextW(m_hwndEdit, szText, _countof(szText));

    // Populate the items
    if (m_dwSHACF & (SHACF_FILESYS_ONLY | SHACF_FILESYSTEM | SHACF_FILESYS_DIRS))
        DoFileSystem(szText);

    if (!(m_dwSHACF & (SHACF_FILESYS_ONLY)))
    {
        if (m_dwSHACF & SHACF_URLHISTORY)
            DoURLHistory();

        if (m_dwSHACF & SHACF_URLMRU)
            DoURLMRU();
    }
}

STDMETHODIMP CAutoCompleteEnumString::Reset()
{
    DoAll();
    return S_OK;
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
    pES->m_hwndEdit = NULL;
    pES->m_dwSHACF = m_dwSHACF;
    pES->m_items = m_items;
    *ppenum = pES;
    return S_OK;
}

/* "." or ".." ? */
#define IS_DOTS(sz) (sz[0] == L'.' && (sz[1] == 0 || (sz[1] == L'.' && sz[2] == 0)))

void CAutoCompleteEnumString::DoDir(LPCWSTR pszDir, BOOL bDirOnly)
{
    if (m_items.GetSize() >= MAX_ITEMS)
        return;

    // Build a path with wildcard
    WCHAR szPath[MAX_PATH];
    StringCbCopyW(szPath, sizeof(szPath), pszDir);
    if (!PathAppendW(szPath, L"*"))
        return;

    // Start the enumeration
    WIN32_FIND_DATAW find;
    HANDLE hFind = FindFirstFileW(szPath, &find);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    LPWSTR pchFileTitle = PathFindFileNameW(szPath); // The file title part

    do
    {
        if (IS_DOTS(find.cFileName) || (find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            continue; // "." and ".." and hidden files are invisible
        if (bDirOnly && !(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue; // Directory-only and not a directory

        *pchFileTitle = 0; // Truncate the path
        if (PathAppendW(szPath, find.cFileName)) // Build a path
        {
            if (!AddString(szPath))
                break;
        }
    } while (FindNextFileW(hFind, &find));

    FindClose(hFind); // End the enumeration
}

void CAutoCompleteEnumString::DoDrives(BOOL bDirOnly)
{
    WCHAR sz[4] = L"C:\\";
    for (DWORD i = 0, dwBits = GetLogicalDrives(); i <= L'Z' - L'A'; ++i)
    {
        if ((dwBits & (1 << i)) == 0)
            continue; // The drive doesn't exist

        sz[0] = (WCHAR)(L'A' + i); // Build a root path of the drive
        if (!AddString(sz))
            break;

        switch (GetDriveTypeW(sz))
        {
            case DRIVE_REMOTE: case DRIVE_RAMDISK: case DRIVE_FIXED:
                DoDir(sz, bDirOnly);
                break;
            default:
                break; // Don't scan slow drives
        }
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
        // Build a registry value name
        StringCbPrintfW(szName, sizeof(szName), L"url%lu", i);

        // Read a registry value
        DWORD cbValue = sizeof(szValue), dwType;
        result = RegQueryValueExW(hKey, szName, NULL, &dwType, (LPBYTE)szValue, &cbValue);
        if (result == ERROR_SUCCESS && dwType == REG_SZ) // Could I read it?
        {
            if (UrlIsW(szValue, URLIS_URL)) // Is it a URL?
            {
                if (!AddString(szValue))
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
            if (!AddString(szValue))
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
