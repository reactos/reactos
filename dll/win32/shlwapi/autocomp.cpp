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
        : m_istr(0)
        , m_hwndEdit(NULL)
        , m_dwSHACF(0)
    {
    }

    void Initialize(IAutoComplete2 *pAC2, DWORD dwSHACF, HWND hwndEdit);

    BOOL AddString(LPCWSTR psz)
    {
        if (m_strs.GetSize() >= MAX_ITEMS)
            return FALSE;
        m_strs.Add(psz);
        return (m_strs.GetSize() < MAX_ITEMS);
    }

    void ResetContent()
    {
        m_strs.RemoveAll();
        m_istr = 0;
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
    INT m_istr;
    HWND m_hwndEdit;
    DWORD m_dwSHACF;
    CSimpleArray<CStringW> m_strs;
};

void CAutoCompleteEnumString::Initialize(IAutoComplete2 *pAC2, DWORD dwSHACF, HWND hwndEdit)
{
    m_istr = 0;
    m_hwndEdit = hwndEdit;
    m_dwSHACF = dwSHACF;
    Reset();
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

    if (m_istr >= m_strs.GetSize())
        return S_FALSE; // No more elements

    // Get elements
    ULONG ielt;
    for (ielt = 0; ielt < celt && m_istr < m_strs.GetSize(); ++ielt, ++m_istr)
    {
        size_t cb = (wcslen(m_strs[m_istr]) + 1) * sizeof(WCHAR);
        rgelt[ielt] = (LPWSTR)CoTaskMemAlloc(cb);
        if (!rgelt[ielt])
        {
            while (ielt-- > 0)
                CoTaskMemFree(rgelt[ielt]);
            return S_FALSE;
        }
        CopyMemory(rgelt[ielt], (LPCWSTR)m_strs[m_istr], cb);
    }
    *pceltFetched = ielt;
    return (ielt == celt) ? S_OK : S_FALSE;
}

STDMETHODIMP CAutoCompleteEnumString::Skip(ULONG celt)
{
    if (m_strs.GetSize() == 0 || m_strs.GetSize() <= INT(m_istr + celt))
        return S_FALSE;

    m_istr += celt;
    return S_OK;
}

void CAutoCompleteEnumString::DoFileSystem(LPCWSTR pszQuery)
{
    // Check drive number
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

    BOOL bDirOnly = !!(m_dwSHACF & SHACF_FILESYS_DIRS);

    DWORD attrs = GetFileAttributesW(pszQuery);
    if (attrs != INVALID_FILE_ATTRIBUTES) 
    {
        // File or folder doesn't exist
        if (attrs & FILE_ATTRIBUTE_DIRECTORY)
            DoDir(pszQuery, bDirOnly);
        else
            AddString(pszQuery);
    }
    else if (pszQuery[0] && wcschr(pszQuery, L'\\') != NULL)
    {
        // Non-existent but can be a partial path
        WCHAR szPath[MAX_PATH];
        StringCbCopyW(szPath, sizeof(szPath), pszQuery);
        PathRemoveFileSpecW(szPath);

        DoDir(szPath, bDirOnly);
    }
    else
    {
        // An empty query
        DoDrives(bDirOnly);
    }
}

void CAutoCompleteEnumString::DoAll()
{
    // Check whether m_hwndEdit is valid
    if (!IsWindow(m_hwndEdit))
    {
        ResetContent();
        return;
    }

    // Get text from EDIT control
    WCHAR szText[MAX_PATH];
    GetWindowTextW(m_hwndEdit, szText, _countof(szText));

    // Clear
    ResetContent();

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
    m_istr = 0;

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
    pES->m_istr = m_istr;
    pES->m_hwndEdit = NULL;
    pES->m_dwSHACF = m_dwSHACF;
    pES->m_strs = m_strs;

    *ppenum = pES;
    return S_OK;
}

/* "." or ".." ? */
#define IS_DOTS(sz) (sz[0] == L'.' && (sz[1] == 0 || (sz[1] == L'.' && sz[2] == 0)))

void CAutoCompleteEnumString::DoDir(LPCWSTR pszDir, BOOL bDirOnly)
{
    WCHAR szPath[MAX_PATH];
    StringCbCopyW(szPath, sizeof(szPath), pszDir);
    if (!PathAppendW(szPath, L"*") || m_strs.GetSize() >= MAX_ITEMS)
        return;

    WIN32_FIND_DATAW find;
    HANDLE hFind = FindFirstFileW(szPath, &find);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    LPWSTR pch = PathFindFileNameW(szPath);
    do
    {
        if (IS_DOTS(find.cFileName) || (find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            continue;
        if (bDirOnly && !(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;

        *pch = UNICODE_NULL;
        if (PathAppendW(szPath, find.cFileName))
        {
            if (!AddString(szPath))
                break;
        }
    } while (FindNextFileW(hFind, &find));

    FindClose(hFind);
}

void CAutoCompleteEnumString::DoDrives(BOOL bDirOnly)
{
    WCHAR sz[4] = L"C:\\";
    for (DWORD i = 0, dwBits = GetLogicalDrives(); i <= L'Z' - L'A'; ++i)
    {
        if ((dwBits & (1 << i)) == 0)
            continue;

        sz[0] = (WCHAR)(L'A' + i);
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

    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, pszTypedURLs, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS)
    {
        TRACE("Opening TypedURLs failed: 0x%lX\n", result);
        return;
    }

    for (DWORD i = 1; i <= MAX_ITEMS; ++i)
    {
        StringCbPrintfW(szName, sizeof(szName), L"url%lu", i);

        DWORD cbValue = sizeof(szValue), dwType;
        result = RegQueryValueExW(hKey, szName, NULL, &dwType, (LPBYTE)szValue, &cbValue);
        if (result == ERROR_SUCCESS && dwType == REG_SZ && UrlIsW(szValue, URLIS_URL))
        {
            if (!AddString(szValue))
                break;
        }
    }

    RegCloseKey(hKey);
}

void CAutoCompleteEnumString::DoURLMRU()
{
    static const LPCWSTR
        pszRunMRU = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU";
    WCHAR szName[2], szMRUList[64], szValue[MAX_PATH + 32];

    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, pszRunMRU, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS)
    {
        TRACE("Opening RunMRU failed: 0x%lX\n", result);
        return;
    }

    DWORD cbValue = sizeof(szMRUList), dwType;
    result = RegQueryValueExW(hKey, L"MRUList", NULL, &dwType, (LPBYTE)szMRUList, &cbValue);
    if (result == ERROR_SUCCESS && dwType == REG_SZ)
    {
        szName[1] = 0;
        for (DWORD i = 0; i <= L'z' - L'a' && szMRUList[i]; ++i)
        {
            szName[0] = szMRUList[i];
            cbValue = sizeof(szValue);
            result = RegQueryValueExW(hKey, szName, NULL, &dwType, (LPBYTE)szValue, &cbValue);
            if (result != ERROR_SUCCESS || dwType != REG_SZ)
                continue;

            size_t cch = wcslen(szValue);
            if (cch >= 2 && wcscmp(&szValue[cch - 2], L"\\1") == 0)
                szValue[cch - 2] = 0;

            if (UrlIsW(szValue, URLIS_URL))
            {
                if (!AddString(szValue))
                    break;
            }
        }
    }

    RegCloseKey(hKey);
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
