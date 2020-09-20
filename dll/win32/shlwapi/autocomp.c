/*
 * PROJECT:     ReactOS shlwapi
 * LICENSE:     LGPL-3.0-or-later (https://spdx.org/licenses/LGPL-3.0-or-later)
 * PURPOSE:     Implement SHAutoComplete
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#define COBJMACROS
#include "config.h"
#include "windef.h"
#include "winreg.h"
#include "shldisp.h"
#include "shlguid.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "strsafe.h"
#include "assert.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define INITIAL_CAPACITY    256
#define NUM_GROW            64
#define MAX_TYPED_URLS      50

typedef struct AutoComplete_EnumStringVtbl
{
    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IEnumString* This,
        REFIID riid,
        void **ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        IEnumString* This);

    ULONG (STDMETHODCALLTYPE *Release)(
        IEnumString* This);

    /*** IEnumString methods ***/
    HRESULT (STDMETHODCALLTYPE *Next)(
        IEnumString* This,
        ULONG celt,
        LPOLESTR *rgelt,
        ULONG *pceltFetched);

    HRESULT (STDMETHODCALLTYPE *Skip)(
        IEnumString* This,
        ULONG celt);

    HRESULT (STDMETHODCALLTYPE *Reset)(
        IEnumString* This);

    HRESULT (STDMETHODCALLTYPE *Clone)(
        IEnumString* This,
        IEnumString **ppenum);
} AutoComplete_EnumStringVtbl;

typedef struct AutoComplete_EnumString
{
    AutoComplete_EnumStringVtbl * lpVtbl;
    LONG                m_cRefs;
    ULONG               m_istr;
    SIZE_T              m_cstrs;
    SIZE_T              m_capacity;
    BSTR *              m_pstrs;
    HWND                m_hwndEdit;
    DWORD               m_dwSHACF;
} AutoComplete_EnumString;

static void
AutoComplete_EnumString_ResetContent(AutoComplete_EnumString *this_)
{
    SIZE_T count = this_->m_cstrs;
    BSTR *pstrs = this_->m_pstrs;
    while (count-- > 0)
    {
        SysFreeString(pstrs[count]);
    }
    CoTaskMemFree(pstrs);
    this_->m_pstrs = NULL;
    this_->m_cstrs = this_->m_capacity = 0;
    this_->m_istr = 0;
}

static void
AutoComplete_EnumString_Destruct(AutoComplete_EnumString *this_)
{
    AutoComplete_EnumString_ResetContent(this_);
    CoTaskMemFree(this_->lpVtbl);
    CoTaskMemFree(this_);
}

static HRESULT STDMETHODCALLTYPE
AutoComplete_EnumString_QueryInterface(
    IEnumString* This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject)
    {
        ERR("ppvObject is NULL\n");
        return E_POINTER;
    }

    if (IsEqualIID(riid, &IID_IEnumString) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = This;
        IUnknown_AddRef(This);
        return S_OK;
    }

    ERR("E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
AutoComplete_EnumString_AddRef(IEnumString* This)
{
    AutoComplete_EnumString *this_ = (AutoComplete_EnumString *)This;
    return InterlockedIncrement(&this_->m_cRefs);
}

static ULONG STDMETHODCALLTYPE
AutoComplete_EnumString_Release(IEnumString* This)
{
    AutoComplete_EnumString *this_ = (AutoComplete_EnumString *)This;
    LONG ret = InterlockedDecrement(&this_->m_cRefs);
    if (!ret)
        AutoComplete_EnumString_Destruct(this_);
    return ret;
}

static inline BOOL
AutoComplete_EnumString_AddBStrNoGrow(AutoComplete_EnumString *this_, BSTR bstr)
{
    UINT cch = SysStringLen(bstr);
    bstr = SysAllocStringLen(bstr, cch);
    if (!bstr)
    {
        ERR("Out of memory\n");
        return FALSE;
    }

    assert(this_->m_cstrs + 1 < this_->m_capacity);
    this_->m_pstrs[this_->m_cstrs++] = bstr;
    return TRUE;
}

static BOOL
AutoComplete_EnumString_AddString(AutoComplete_EnumString *this_, LPCWSTR str)
{
    SIZE_T new_capacity;
    BSTR bstr, *pstrs;

    bstr = SysAllocString(str);
    if (!bstr)
    {
        ERR("Out of memory\n");
        return FALSE;
    }

    if (this_->m_cstrs + 1 >= this_->m_capacity)
    {
        new_capacity = this_->m_capacity + NUM_GROW;
        pstrs = (BSTR *)CoTaskMemAlloc(new_capacity * sizeof(BSTR));
        if (!pstrs)
        {
            ERR("Out of memory\n");
            SysFreeString(bstr);
            return FALSE;
        }

        CopyMemory(pstrs, this_->m_pstrs, this_->m_cstrs * sizeof(BSTR));
        CoTaskMemFree(this_->m_pstrs);
        this_->m_pstrs = pstrs;
        this_->m_capacity = new_capacity;
    }

    this_->m_pstrs[this_->m_cstrs++] = bstr;
    return TRUE;
}

static HRESULT STDMETHODCALLTYPE
AutoComplete_EnumString_Next(
    IEnumString* This,
    ULONG celt,
    LPOLESTR *rgelt,
    ULONG *pceltFetched)
{
    SIZE_T ielt, cch, cb;
    BSTR *pstrs;
    AutoComplete_EnumString *this_ = (AutoComplete_EnumString *)This;

    if (!rgelt || !pceltFetched)
    {
        ERR("E_POINTER\n");
        return E_POINTER;
    }

    *pceltFetched = 0;
    *rgelt = NULL;

    if (this_->m_istr >= this_->m_cstrs)
        return S_FALSE;

    pstrs = this_->m_pstrs;
    for (ielt = 0;
         ielt < celt && this_->m_istr < this_->m_cstrs;
         ++ielt, ++this_->m_istr)
    {
        cch = (SysStringLen(pstrs[this_->m_istr]) + 1);
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
        CopyMemory(rgelt[ielt], pstrs[this_->m_istr], cb);
    }

    *pceltFetched = ielt;

    if (ielt == celt)
        return S_OK;

    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE
AutoComplete_EnumString_Skip(IEnumString* This, ULONG celt)
{
    AutoComplete_EnumString *this_ = (AutoComplete_EnumString *)This;
    if (this_->m_istr + celt >= this_->m_cstrs || 0 == this_->m_cstrs)
        return S_FALSE;

    this_->m_istr += celt;
    return S_OK;
}

static AutoComplete_EnumString *AutoComplete_EnumString_Construct(SIZE_T capacity);

static HRESULT STDMETHODCALLTYPE
AutoComplete_EnumString_Clone(IEnumString* This, IEnumString **ppenum)
{
    AutoComplete_EnumString *this_, *cloned;
    SIZE_T i, count;
    BSTR *pstrs;

    if (!ppenum)
    {
        ERR("E_POINTER\n");
        return E_POINTER;
    }

    this_ = (AutoComplete_EnumString *)This;
    cloned = AutoComplete_EnumString_Construct(this_->m_cstrs);
    if (!cloned)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    count = this_->m_cstrs;
    pstrs = this_->m_pstrs;
    for (i = 0; i < count; ++i)
    {
        if (!AutoComplete_EnumString_AddBStrNoGrow(cloned, pstrs[i]))
        {
            AutoComplete_EnumString_Destruct(cloned);
            return E_FAIL;
        }
    }

    cloned->m_capacity = cloned->m_cstrs = count;
    cloned->m_hwndEdit = this_->m_hwndEdit;
    cloned->m_dwSHACF = this_->m_dwSHACF;

    *ppenum = (IEnumString *)cloned;
    return S_OK;
}

#define IS_IGNORED_DOTS(sz) \
    ( sz[0] == L'.' && (sz[1] == 0 || (sz[1] == L'.' && sz[2] == 0)) )

static inline void
AutoComplete_DoDir(AutoComplete_EnumString *pES, LPCWSTR pszDir, BOOL bDirOnly)
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
            if (IS_IGNORED_DOTS(find.cFileName))
                continue;
            if (find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
                continue;
            if (bDirOnly && !(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;

            *pch = UNICODE_NULL;
            if (PathAppendW(szPath, find.cFileName))
                AutoComplete_EnumString_AddString(pES, szPath);
        } while (FindNextFileW(hFind, &find));

        FindClose(hFind);
    }
}

static void
AutoComplete_DoDrives(AutoComplete_EnumString *pES, BOOL bDirOnly)
{
    WCHAR sz[4];
    UINT uType;
    DWORD i, dwBits = GetLogicalDrives();

    for (i = 0; i <= L'Z' - L'A'; ++i)
    {
        if (dwBits & (1 << i))
        {
            sz[0] = (WCHAR)(L'A' + i);
            sz[1] = L':';
            sz[2] = L'\\';
            sz[3] = 0;
            AutoComplete_EnumString_AddString(pES, sz);

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
                    AutoComplete_DoDir(pES, sz, bDirOnly);
                    break;
            }
        }
    }
}

static void
AutoComplete_DoURLHistory(AutoComplete_EnumString *pES)
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
            AutoComplete_EnumString_AddString(pES, szValue);
        }
    }

    RegCloseKey(hKey);
}

static void
AutoComplete_DoURLMRU(AutoComplete_EnumString *pES)
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
                AutoComplete_EnumString_AddString(pES, szValue);
            }
        }
    }

    RegCloseKey(hKey);
}

static HRESULT STDMETHODCALLTYPE
AutoComplete_EnumString_Reset(IEnumString* This)
{
    DWORD attrs;
    WCHAR szText[MAX_PATH];
    AutoComplete_EnumString *this_ = (AutoComplete_EnumString *)This;
    DWORD dwSHACF = this_->m_dwSHACF;

    GetWindowTextW(this_->m_hwndEdit, szText, ARRAYSIZE(szText));
    attrs = GetFileAttributesW(szText);

    AutoComplete_EnumString_ResetContent(this_);

    if (dwSHACF & (SHACF_FILESYS_ONLY | SHACF_FILESYSTEM | SHACF_FILESYS_DIRS))
    {
        BOOL bDirOnly = !!(dwSHACF & SHACF_FILESYS_DIRS);
        if (attrs != INVALID_FILE_ATTRIBUTES)
        {
            if (attrs & FILE_ATTRIBUTE_DIRECTORY)
            {
                AutoComplete_DoDir(this_, szText, bDirOnly);
            }
        }
        else if (szText[0])
        {
            PathRemoveFileSpecW(szText);
            AutoComplete_DoDir(this_, szText, bDirOnly);
        }
        else
        {
            AutoComplete_DoDrives(this_, bDirOnly);
        }
    }

    if (!(dwSHACF & (SHACF_FILESYS_ONLY)))
    {
        if (dwSHACF & SHACF_URLHISTORY)
        {
            AutoComplete_DoURLHistory(this_);
        }
        if (dwSHACF & SHACF_URLMRU)
        {
            AutoComplete_DoURLMRU(this_);
        }
    }

    this_->m_istr = 0;
    return S_OK;
}

static AutoComplete_EnumString *
AutoComplete_EnumString_Construct(SIZE_T capacity)
{
    AutoComplete_EnumStringVtbl *lpVtbl;
    AutoComplete_EnumString *ret = CoTaskMemAlloc(sizeof(AutoComplete_EnumString));
    if (!ret)
        return ret;

    lpVtbl = CoTaskMemAlloc(sizeof(AutoComplete_EnumStringVtbl));
    if (!lpVtbl)
    {
        CoTaskMemFree(ret);
        return NULL;
    }

    lpVtbl->QueryInterface = AutoComplete_EnumString_QueryInterface;
    lpVtbl->AddRef = AutoComplete_EnumString_AddRef;
    lpVtbl->Release = AutoComplete_EnumString_Release;
    lpVtbl->Next = AutoComplete_EnumString_Next;
    lpVtbl->Skip = AutoComplete_EnumString_Skip;
    lpVtbl->Reset = AutoComplete_EnumString_Reset;
    lpVtbl->Clone = AutoComplete_EnumString_Clone;
    ret->lpVtbl = lpVtbl;

    ret->m_pstrs = (BSTR *)CoTaskMemAlloc(capacity * sizeof(BSTR));
    if (ret->m_pstrs)
    {
        ret->m_cRefs = 1;
        ret->m_istr = 0;
        ret->m_cstrs = 0;
        ret->m_capacity = capacity;
        ret->m_hwndEdit = NULL;
        return ret;
    }

    CoTaskMemFree(ret->lpVtbl);
    CoTaskMemFree(ret);
    return NULL;
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

static IEnumString *
AutoComplete_CreateEnumString(IAutoComplete2 *pAC2, HWND hwndEdit, DWORD dwSHACF)
{
    IEnumString *ret;
    AutoComplete_EnumString *this_ = AutoComplete_EnumString_Construct(INITIAL_CAPACITY);
    if (!this_)
        return NULL;

    this_->m_dwSHACF = dwSHACF;
    this_->m_hwndEdit = hwndEdit;

    ret = (IEnumString *)this_;
    AutoComplete_EnumString_Reset(ret);

    return ret;
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
    LPAUTOCOMPLETE2 pAC2 = NULL;
    LPENUMSTRING pES;
    DWORD dwACO;
    DWORD dwClsCtx = CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER;

    TRACE("SHAutoComplete(%p, 0x%lX)\n", hwndEdit, dwFlags);

    if (!AutoComplete_AdaptFlags(hwndEdit, &dwACO, &dwFlags))
        return S_OK;

    hr = CoCreateInstance(&CLSID_AutoComplete, NULL, dwClsCtx,
                          &IID_IAutoComplete, (LPVOID *)&pAC2);
    if (FAILED(hr))
    {
        ERR("CoCreateInstance(CLSID_AutoComplete) failed: 0x%lX\n", hr);
        return hr;
    }

    pES = AutoComplete_CreateEnumString(pAC2, hwndEdit, dwFlags);
    if (!pES)
    {
        ERR("Creating IEnumString failed.\n");
        IUnknown_Release(pAC2);
        return E_FAIL;
    }

    hr = IAutoComplete2_Init(pAC2, hwndEdit, (LPUNKNOWN)pES, NULL, L"www.%s.com");
    if (SUCCEEDED(hr))
    {
        IAutoComplete2_SetOptions(pAC2, dwACO);
    }
    else
    {
        ERR("IAutoComplete2::Init failed: 0x%lX\n", hr);
    }

    IUnknown_Release(pAC2);
    IUnknown_Release(pES);
    return hr;
}
