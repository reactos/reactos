/*
 * PROJECT:     ReactOS shlwapi
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement SHAutoComplete
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#define COBJMACROS
#include <windef.h>
#include <winreg.h>
#include <shldisp.h>
#include <shlguid.h>
#define NO_SHLWAPI_STREAM
#include <shlwapi.h>
#include <atlbase.h>
#include <atlwin.h>
#include <atlcom.h>
#include <atlstr.h>
#include <atlsimpcoll.h>
#include <strsafe.h>
#include <assert.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

class CAutoCompleteEnumString :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumString
{
public:
    CAutoCompleteEnumString(DWORD dwSHACF, HWND hwndEdit);
    CAutoCompleteEnumString(const CAutoCompleteEnumString& another);
    ~CAutoCompleteEnumString();

    void AddString(LPCWSTR psz);
    void DoDir(LPCWSTR pszDir, BOOL bDirOnly);
    void DoDrives(BOOL bDirOnly);
    void DoURLHistory();
    void DoURLMRU();
    void ResetContent();

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

CAutoCompleteEnumString::CAutoCompleteEnumString(DWORD dwSHACF, HWND hwndEdit)
    : m_istr(0)
    , m_hwndEdit(hwndEdit)
    , m_dwSHACF(dwSHACF)
{
    Reset();
}

CAutoCompleteEnumString::CAutoCompleteEnumString(const CAutoCompleteEnumString& another)
    : m_istr(another.m_istr)
    , m_hwndEdit(another.m_hwndEdit)
    , m_dwSHACF(another.m_dwSHACF)
    , m_strs(another.m_strs)
{
}

CAutoCompleteEnumString::~CAutoCompleteEnumString()
{
}

STDMETHODIMP
CAutoCompleteEnumString::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    SIZE_T ielt, cch, cb;

    if (!rgelt || !pceltFetched)
    {
        ERR("E_POINTER\n");
        return E_POINTER;
    }

    *pceltFetched = 0;
    *rgelt = NULL;

    if (m_istr >= m_strs.GetSize())
        return S_FALSE;

    for (ielt = 0;
         ielt < celt && m_istr < m_strs.GetSize();
         ++ielt, ++m_istr)
    {
        cch = lstrlenW(m_strs[m_istr]) + 1;
        cb = cch * sizeof(WCHAR);

        rgelt[ielt] = (LPWSTR)CoTaskMemAlloc(cb);
        if (!rgelt[ielt])
        {
            while (ielt-- > 0)
            {
                CoTaskMemFree(rgelt[ielt]);
            }
            return S_FALSE;
        }

        CopyMemory(rgelt[ielt], (LPCWSTR)m_strs[m_istr], cb);
    }

    *pceltFetched = ielt;

    if (ielt == celt)
        return S_OK;

    return S_FALSE;
}

STDMETHODIMP CAutoCompleteEnumString::Skip(ULONG celt)
{
    if (INT(m_istr + celt) >= m_strs.GetSize() || m_strs.GetSize() == 0)
        return S_FALSE;

    m_istr += celt;
    return S_OK;
}

STDMETHODIMP CAutoCompleteEnumString::Reset()
{
    DWORD attrs;
    WCHAR szText[MAX_PATH];
    DWORD dwSHACF = m_dwSHACF;

    ResetContent();

    GetWindowTextW(m_hwndEdit, szText, _countof(szText));
    attrs = GetFileAttributesW(szText);

    if (dwSHACF & (SHACF_FILESYS_ONLY | SHACF_FILESYSTEM | SHACF_FILESYS_DIRS))
    {
        BOOL bDirOnly = !!(dwSHACF & SHACF_FILESYS_DIRS);
        if (attrs != INVALID_FILE_ATTRIBUTES)
        {
            if (attrs & FILE_ATTRIBUTE_DIRECTORY)
            {
                DoDir(szText, bDirOnly);
            }
        }
        else if (szText[0])
        {
            PathRemoveFileSpecW(szText);
            DoDir(szText, bDirOnly);
        }
        else
        {
            DoDrives(bDirOnly);
        }
    }

    if (!(dwSHACF & (SHACF_FILESYS_ONLY)))
    {
        if (dwSHACF & SHACF_URLHISTORY)
        {
            DoURLHistory();
        }
        if (dwSHACF & SHACF_URLMRU)
        {
            DoURLMRU();
        }
    }

    m_istr = 0;
    return S_OK;
}

STDMETHODIMP CAutoCompleteEnumString::Clone(IEnumString **ppenum)
{
    if (!ppenum)
    {
        ERR("E_POINTER\n");
        return E_POINTER;
    }

    *ppenum = new CAutoCompleteEnumString(*this);
    return S_OK;
}

void CAutoCompleteEnumString::ResetContent()
{
    m_strs.RemoveAll();
    m_istr = 0;
}

void CAutoCompleteEnumString::AddString(LPCWSTR psz)
{
    m_strs.Add(psz);
}

/* "." or ".." ? */
#define IS_IGNORABLE_DOTS(sz) \
    ( sz[0] == L'.' && (sz[1] == 0 || (sz[1] == L'.' && sz[2] == 0)) )

void CAutoCompleteEnumString::DoDir(LPCWSTR pszDir, BOOL bDirOnly)
{
    LPWSTR pch;
    WCHAR szPath[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATAW find;

    StringCbCopyW(szPath, sizeof(szPath), pszDir);
    if (!PathAppendW(szPath, L"*"))
        return;

    pch = PathFindFileNameW(szPath);
    assert(pch);

    hFind = FindFirstFileW(szPath, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (IS_IGNORABLE_DOTS(find.cFileName))
                continue;
            if (find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
                continue;
            if (bDirOnly && !(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;

            *pch = UNICODE_NULL;
            if (PathAppendW(szPath, find.cFileName))
                AddString(szPath);
        } while (FindNextFileW(hFind, &find));

        FindClose(hFind);
    }
}

void CAutoCompleteEnumString::DoDrives(BOOL bDirOnly)
{
    WCHAR sz[4] = L"C:\\";
    UINT uType;
    DWORD i, dwBits = GetLogicalDrives();

    for (i = 0; i <= L'Z' - L'A'; ++i)
    {
        if (dwBits & (1 << i))
        {
            sz[0] = (WCHAR)(L'A' + i);
            AddString(sz);

            uType = GetDriveTypeW(sz);
            switch (uType)
            {
                case DRIVE_UNKNOWN:
                case DRIVE_NO_ROOT_DIR:
                case DRIVE_REMOVABLE:
                case DRIVE_CDROM:
                    break;
                case DRIVE_REMOTE:
                case DRIVE_RAMDISK:
                case DRIVE_FIXED:
                    DoDir(sz, bDirOnly);
                    break;
            }
        }
    }
}

void CAutoCompleteEnumString::DoURLHistory()
{
    static const LPCWSTR
        pszTypedURLs = L"Software\\Microsoft\\Internet Explorer\\TypedURLs";
    HKEY hKey;
    LONG result;
    DWORD i, cbValue;
    WCHAR szName[32], szValue[MAX_PATH + 32];

    result = RegOpenKeyExW(HKEY_CURRENT_USER, pszTypedURLs, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS)
    {
        TRACE("Opening TypedURLs failed: 0x%lX\n", result);
        return;
    }

#define MAX_TYPED_URLS 50
    for (i = 1; i <= MAX_TYPED_URLS; ++i)
    {
        StringCbPrintfW(szName, sizeof(szName), L"url%lu", i);

        szValue[0] = 0;
        cbValue = sizeof(szValue);
        result = RegQueryValueExW(hKey, szName, NULL, NULL, (LPBYTE)szValue, &cbValue);
        if (result != ERROR_SUCCESS)
            continue;

        if (UrlIsW(szValue, URLIS_URL))
        {
            AddString(szValue);
        }
    }

    RegCloseKey(hKey);
}

void CAutoCompleteEnumString::DoURLMRU()
{
    static const LPCWSTR
        pszRunMRU = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU";
    HKEY hKey;
    LONG result;
    DWORD i, cbValue;
    WCHAR szName[2], szMRUList[64], szValue[MAX_PATH + 32];
    INT cch;

    result = RegOpenKeyExW(HKEY_CURRENT_USER, pszRunMRU, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS)
    {
        TRACE("Opening RunMRU failed: 0x%lX\n", result);
        return;
    }

    szMRUList[0] = 0;
    cbValue = sizeof(szMRUList);
    result = RegQueryValueExW(hKey, L"MRUList", NULL, NULL, (LPBYTE)szMRUList, &cbValue);
    if (result == ERROR_SUCCESS)
    {
        for (i = 0; i <= L'z' - L'a'; ++i)
        {
            if (szMRUList[i] == 0)
                break;

            szName[0] = szMRUList[i];
            szName[1] = 0;
            szValue[0] = 0;
            cbValue = sizeof(szValue);
            result = RegQueryValueExW(hKey, szName, NULL, NULL, (LPBYTE)szValue, &cbValue);
            if (result != ERROR_SUCCESS)
                continue;

            cch = wcslen(szValue);
            if (cch >= 2 && wcscmp(&szValue[cch - 2], L"\\1") == 0)
            {
                szValue[cch - 2] = 0;
            }

            if (UrlIsW(szValue, URLIS_URL))
            {
                AddString(szValue);
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

    DWORD dwType, cbValue, dwACO, dwSHACF;
    WCHAR szValue[8];

    dwSHACF = *pdwSHACF;

    dwACO = 0;
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
        cbValue = sizeof(szValue);
        if (SHGetValueW(HKEY_CURRENT_USER, s_pszAutoComplete, L"Append Completion",
                        &dwType, szValue, &cbValue) != ERROR_SUCCESS ||
            _wcsicmp(szValue, L"no") != 0)
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
        cbValue = sizeof(szValue);
        if (SHGetValueW(HKEY_CURRENT_USER, s_pszAutoComplete, L"AutoSuggest",
                        &dwType, szValue, &cbValue) != ERROR_SUCCESS ||
            _wcsicmp(szValue, L"no") != 0)
        {
            dwACO |= ACO_AUTOSUGGEST;
        }
    }

    if (dwSHACF & SHACF_USETAB)
        dwACO |= ACO_USETAB;

    if (GetWindowLongPtr(hwndEdit, GWL_EXSTYLE) & WS_EX_RTLREADING)
    {
        dwACO |= ACO_RTLREADING;
    }

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
    HRESULT hr;
    CComPtr<IAutoComplete2> pAC2;
    CComPtr<IEnumString> pES;
    DWORD dwACO;
    DWORD dwClsCtx = CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER;

    TRACE("SHAutoComplete(%p, 0x%lX)\n", hwndEdit, dwFlags);

    if (!AutoComplete_AdaptFlags(hwndEdit, &dwACO, &dwFlags))
        return S_OK;

    hr = CoCreateInstance(CLSID_AutoComplete, NULL, dwClsCtx,
                          IID_IAutoComplete, (LPVOID *)&pAC2);
    if (FAILED(hr))
    {
        ERR("CoCreateInstance(CLSID_AutoComplete) failed: 0x%lX\n", hr);
        return hr;
    }

    pES = new CAutoCompleteEnumString(dwSHACF, hwndEdit);
    if (!pES)
    {
        ERR("Creating IEnumString failed.\n");
        return E_FAIL;
    }

    hr = pAC2->Init(hwndEdit, (LPUNKNOWN)pES, NULL, L"www.%s.com");
    if (SUCCEEDED(hr))
    {
        pAC2->SetOptions(dwACO);
    }
    else
    {
        ERR("IAutoComplete2::Init failed: 0x%lX\n", hr);
    }

    return hr;
}
