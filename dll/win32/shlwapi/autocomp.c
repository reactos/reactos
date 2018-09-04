/*
 * SHAutoComplete
 *
 * Copyright 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include "config.h"
#include "wine/port.h"

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "shlobj.h"
#include "shlwapi.h"
#include <assert.h>
#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define NUM_GROW 64

typedef struct AC_EnumStringVtbl
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
} AC_EnumStringVtbl;

typedef struct AC_EnumString
{
    AC_EnumStringVtbl*  lpVtbl;
    LONG                m_cRefs;
    ULONG               m_istr;
    SIZE_T              m_cstrs;
    SIZE_T              m_capacity;
    BSTR *              m_pstrs;
    HWND                m_hwndEdit;
    DWORD               m_dwSHACF_;
} AC_EnumString;

static void
AC_EnumString_ResetContent(AC_EnumString *this)
{
    SIZE_T count = this->m_cstrs;
    BSTR *pstrs = this->m_pstrs;
    while (count-- > 0)
    {
        SysFreeString(pstrs[count]);
    }
    CoTaskMemFree(this->m_pstrs);
    this->m_pstrs = NULL;
    this->m_cstrs = this->m_capacity = 0;
    this->m_istr = 0;
}

static void
AC_EnumString_Destruct(AC_EnumString *this)
{
    AC_EnumString_ResetContent(this);
    CoTaskMemFree(this->lpVtbl);
    CoTaskMemFree(this);
}

static HRESULT STDMETHODCALLTYPE
AC_EnumString_QueryInterface(
    IEnumString* This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IEnumString) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = This;
        IUnknown_AddRef(This);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
AC_EnumString_AddRef(IEnumString* This)
{
    AC_EnumString *this = (AC_EnumString *)This;
    return InterlockedIncrement(&this->m_cRefs);
}

static ULONG STDMETHODCALLTYPE
AC_EnumString_Release(IEnumString* This)
{
    AC_EnumString *this = (AC_EnumString *)This;
    LONG ret = InterlockedDecrement(&this->m_cRefs);
    if (!ret)
    {
        AC_EnumString_Destruct(this);
    }
    return ret;
}

static inline BOOL
AC_EnumString_AddBStrNoGrow(AC_EnumString *this, BSTR bstr)
{
    UINT cch = SysStringLen(bstr);
    bstr = SysAllocStringLen(bstr, cch);
    if (!bstr)
        return FALSE;

    assert(this->m_cstrs + 1 < this->m_capacity);
    this->m_pstrs[this->m_cstrs++] = bstr;
    return TRUE;
}

static BOOL
AC_EnumString_AddString(AC_EnumString *this, LPCWSTR str)
{
    SIZE_T new_capacity;
    BSTR bstr, *pstrs;

    bstr = SysAllocString(str);
    if (!bstr)
        return FALSE;

    if (this->m_cstrs + 1 >= this->m_capacity)
    {
        new_capacity = this->m_capacity + NUM_GROW;
        pstrs = (BSTR *)CoTaskMemAlloc(new_capacity * sizeof(BSTR));
        if (!pstrs)
        {
            SysFreeString(bstr);
            return FALSE;
        }

        CopyMemory(pstrs, this->m_pstrs, this->m_cstrs * sizeof(BSTR));
        CoTaskMemFree(this->m_pstrs);
        this->m_pstrs = pstrs;
        this->m_capacity = new_capacity;
    }

    this->m_pstrs[this->m_cstrs++] = bstr;
    return TRUE;
}

static HRESULT STDMETHODCALLTYPE
AC_EnumString_Next(
    IEnumString* This,
    ULONG celt,
    LPOLESTR *rgelt,
    ULONG *pceltFetched)
{
    SIZE_T ielt;
    AC_EnumString *this = (AC_EnumString *)This;

    if (!rgelt || !pceltFetched)
        return E_POINTER;

    *pceltFetched = 0;
    *rgelt = NULL;

    if (this->m_istr >= this->m_cstrs)
        return S_FALSE;

    ielt = 0;
    for (; ielt < celt && this->m_istr < this->m_cstrs; ++ielt, ++this->m_istr)
    {
        SIZE_T cch = (wcslen(this->m_pstrs[this->m_istr]) + 1);
        SIZE_T cb = cch * sizeof(WCHAR);

        rgelt[ielt] = (LPWSTR)CoTaskMemAlloc(cb);
        if (rgelt[ielt])
        {
            CopyMemory(rgelt[ielt], this->m_pstrs[this->m_istr], cb);
        }
    }

    *pceltFetched = ielt;

    if (ielt == celt)
        return S_OK;

    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE
AC_EnumString_Skip(IEnumString* This, ULONG celt)
{
    AC_EnumString *this = (AC_EnumString *)This;
    if (this->m_istr + celt >= this->m_cstrs || 0 == this->m_cstrs)
        return S_FALSE;

    this->m_istr += celt;
    return S_OK;
}

static AC_EnumString *AC_EnumString_Construct(SIZE_T capacity);

static HRESULT STDMETHODCALLTYPE
AC_EnumString_Clone(IEnumString* This, IEnumString **ppenum)
{
    AC_EnumString *this, *cloned;
    SIZE_T i, count;
    BSTR *pstrs;

    if (!ppenum)
        return E_POINTER;

    this = (AC_EnumString *)This;
    cloned = AC_EnumString_Construct(this->m_cstrs);
    if (!cloned)
        return E_OUTOFMEMORY;

    count = this->m_cstrs;
    pstrs = this->m_pstrs;
    for (i = 0; i < count; ++i)
    {
        if (!AC_EnumString_AddBStrNoGrow(cloned, pstrs[i]))
        {
            AC_EnumString_Destruct(cloned);
            return E_FAIL;
        }
    }
    cloned->m_capacity = cloned->m_cstrs = count;
    cloned->m_hwndEdit = this->m_hwndEdit;

    *ppenum = (IEnumString *)cloned;
    return S_OK;
}

#define IS_IGNORED_DOTS(sz) ( \
    sz[0] == L'.' && (sz[1] == 0 || (sz[1] == L'.' && sz[2] == 0)) \
)

/* directories only */
static void
AC_DoDir0(AC_EnumString *pES, LPCWSTR pszDir)
{
    LPWSTR pch;
    WCHAR szPath[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATAW find;

    StringCbCopyW(szPath, sizeof(szPath), pszDir);
    PathAddBackslashW(szPath);

    StringCbCatW(szPath, sizeof(szPath), L"*");
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
            if (!(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;

            *pch = UNICODE_NULL;
            PathAddBackslashW(szPath);
            StringCbCatW(szPath, sizeof(szPath), find.cFileName);

            AC_EnumString_AddString(pES, szPath);
        } while (FindNextFileW(hFind, &find));
    }
}

/* all filesystem objects */
static void
AC_DoDir1(AC_EnumString *pES, LPCWSTR pszDir)
{
    LPWSTR pch;
    WCHAR szPath[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATAW find;

    StringCbCopyW(szPath, sizeof(szPath), pszDir);
    PathAddBackslashW(szPath);

    StringCbCatW(szPath, sizeof(szPath), L"*");
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

            *pch = UNICODE_NULL;
            PathAddBackslashW(szPath);
            StringCbCatW(szPath, sizeof(szPath), find.cFileName);

            AC_EnumString_AddString(pES, szPath);
        } while (FindNextFileW(hFind, &find));
    }
}

static inline void
AC_DoDir(AC_EnumString *pES, LPCWSTR pszDir, BOOL bDirOnly)
{
    if (bDirOnly)
        AC_DoDir0(pES, pszDir);
    else
        AC_DoDir1(pES, pszDir);
}

static void
AC_DoFiles(AC_EnumString *pES, BOOL bDirOnly)
{
    WCHAR sz[4];
    UINT uType;
    DWORD i, dwBits = GetLogicalDrives();
    for (i = 0; i < sizeof(DWORD) * 8; ++i)
    {
        if (dwBits & (1 << i))
        {
            sz[0] = (WCHAR)(L'A' + i);
            sz[1] = L':';
            sz[2] = L'\\';
            sz[3] = 0;
            AC_EnumString_AddString(pES, sz);

            uType = GetDriveTypeW(sz);
            switch (uType)
            {
                case DRIVE_UNKNOWN:
                case DRIVE_NO_ROOT_DIR:
                case DRIVE_REMOVABLE:
                case DRIVE_REMOTE:
                case DRIVE_CDROM:
                case DRIVE_RAMDISK:
                    break;
                case DRIVE_FIXED:
                    AC_DoDir(pES, sz, bDirOnly);
                    break;
            }
        }
    }
}

static void
AC_DoURLHistory(AC_EnumString *pES)
{
    static const LPCWSTR
    pszTypedURLs = L"Software\\Microsoft\\Internet Explorer\\TypedURLs";
    HKEY hKey;
    LONG result;
    DWORD i, cbValue;
    WCHAR szName[32], szValue[MAX_PATH + 32];

    result = RegOpenKeyExW(HKEY_CURRENT_USER, pszTypedURLs, 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS)
    {
        for (i = 1; i <= 50; ++i)
        {
            StringCbPrintfW(szName, sizeof(szName), L"url%lu", i);

            szValue[0] = 0;
            cbValue = sizeof(szValue);
            result = RegQueryValueExW(hKey, szName, NULL, NULL, (LPBYTE)szValue, &cbValue);
            if (result != ERROR_SUCCESS)
                continue;

            if (UrlIsW(szValue, URLIS_URL))
            {
                AC_EnumString_AddString(pES, szValue);
            }
        }

        RegCloseKey(hKey);
    }
}

static void
AC_DoURLMRU(AC_EnumString *pES)
{
    static const LPCWSTR
    pszRunMRU = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU";
    HKEY hKey;
    LONG result;
    DWORD i, cbValue;
    WCHAR szName[2], szMRUList[64], szValue[MAX_PATH + 32];
    INT cch;

    result = RegOpenKeyExW(HKEY_CURRENT_USER, pszRunMRU, 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS)
    {
        szMRUList[0] = 0;
        cbValue = sizeof(szMRUList);
        result = RegQueryValueExW(hKey, L"MRUList", NULL, NULL, (LPBYTE)szMRUList, &cbValue);
        if (result == ERROR_SUCCESS)
        {
            for (i = 0; i < 26; ++i)
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
                    AC_EnumString_AddString(pES, szValue);
                }
            }
        }

        RegCloseKey(hKey);
    }
}

static HRESULT STDMETHODCALLTYPE
AC_EnumString_Reset(IEnumString* This)
{
    WCHAR szText[MAX_PATH];
    DWORD attrs;
    AC_EnumString *this = (AC_EnumString *)This;
    DWORD dwSHACF_ = this->m_dwSHACF_;

    GetWindowTextW(this->m_hwndEdit, szText, ARRAYSIZE(szText));
    attrs = GetFileAttributesW(szText);

    AC_EnumString_ResetContent(this);

    if (dwSHACF_ & (SHACF_FILESYS_ONLY | SHACF_FILESYSTEM | SHACF_FILESYS_DIRS))
    {
        if (dwSHACF_ & SHACF_FILESYS_DIRS)
        {
            if (attrs != INVALID_FILE_ATTRIBUTES)
            {
                if (attrs & FILE_ATTRIBUTE_DIRECTORY)
                {
                    AC_DoDir(this, szText, TRUE);
                }
            }
            else if (szText[0])
            {
                PathRemoveFileSpecW(szText);
                AC_DoDir(this, szText, TRUE);
            }
            else
            {
                AC_DoFiles(this, TRUE);
            }
        }
        else
        {
            if (attrs != INVALID_FILE_ATTRIBUTES)
            {
                if (attrs & FILE_ATTRIBUTE_DIRECTORY)
                {
                    AC_DoDir(this, szText, FALSE);
                }
                else
                {
                    PathRemoveFileSpecW(szText);
                    AC_DoDir(this, szText, FALSE);
                }
            }
            else
            {
                AC_DoFiles(this, FALSE);
            }
        }
    }

    if (!(dwSHACF_ & (SHACF_FILESYS_ONLY)))
    {
        if (dwSHACF_ & SHACF_URLHISTORY)
        {
            AC_DoURLHistory(this);
        }
        if (dwSHACF_ & SHACF_URLMRU)
        {
            AC_DoURLMRU(this);
        }
    }

    this->m_istr = 0;
    return S_OK;
}

static AC_EnumString *
AC_EnumString_Construct(SIZE_T capacity)
{
    AC_EnumString *ret = CoTaskMemAlloc(sizeof(AC_EnumString));
    if (!ret)
        return ret;

    ret->lpVtbl = CoTaskMemAlloc(sizeof(AC_EnumStringVtbl));
    if (!ret->lpVtbl)
    {
        CoTaskMemFree(ret);
        return NULL;
    }

    ret->lpVtbl->QueryInterface = AC_EnumString_QueryInterface;
    ret->lpVtbl->AddRef = AC_EnumString_AddRef;
    ret->lpVtbl->Release = AC_EnumString_Release;
    ret->lpVtbl->Next = AC_EnumString_Next;
    ret->lpVtbl->Skip = AC_EnumString_Skip;
    ret->lpVtbl->Reset = AC_EnumString_Reset;
    ret->lpVtbl->Clone = AC_EnumString_Clone;

    ret->m_pstrs = (BSTR *)CoTaskMemAlloc(capacity * sizeof(BSTR));;
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
AC_AdaptFlags(HWND hwndEdit, LPDWORD pdwACO_, LPDWORD pdwSHACF_)
{
    static const LPCWSTR s_pszAutoComplete =
        L"Software\\Microsoft\\Internet Explorer\\AutoComplete";

    DWORD dwType, cbValue, dwACO_, dwSHACF_;
    WCHAR szValue[8];

    dwSHACF_ = *pdwSHACF_;

    dwACO_ = 0;
    if (dwSHACF_ == SHACF_DEFAULT)
        dwSHACF_ = SHACF_FILESYSTEM | SHACF_URLALL;

    if (dwSHACF_ & SHACF_AUTOAPPEND_FORCE_OFF)
    {
        ;
    }
    else if (dwSHACF_ & SHACF_AUTOAPPEND_FORCE_ON)
    {
        dwACO_ |= ACO_AUTOAPPEND;
    }
    else
    {
        cbValue = sizeof(szValue);
        if (ERROR_SUCCESS != SHGetValueW(HKEY_CURRENT_USER, s_pszAutoComplete, L"Append Completion",
                                         &dwType, szValue, &cbValue) &&
            _wcsicmp(szValue, L"no") != 0)
        {
            dwACO_ |= ACO_AUTOSUGGEST;
        }
    }

    if (dwSHACF_ & SHACF_AUTOSUGGEST_FORCE_OFF)
    {
        ;
    }
    else if (dwSHACF_ & SHACF_AUTOSUGGEST_FORCE_ON)
    {
        dwACO_ |= ACO_AUTOSUGGEST;
    }
    else
    {
        cbValue = sizeof(szValue);
        if (ERROR_SUCCESS != SHGetValueW(HKEY_CURRENT_USER, s_pszAutoComplete,
                                         L"AutoSuggest", &dwType, szValue, &cbValue) &&
            _wcsicmp(szValue, L"no") != 0)
        {
            dwACO_ |= ACO_AUTOSUGGEST;
        }
    }

    if (dwSHACF_ & SHACF_USETAB)
        dwACO_ |= ACO_USETAB;

    if (GetWindowLongPtr(hwndEdit, GWL_EXSTYLE) & WS_EX_RTLREADING)
    {
        dwACO_ |= ACO_RTLREADING;
    }

    if (!(dwACO_ & (ACO_AUTOSUGGEST | ACO_AUTOAPPEND)))
        return FALSE;

    *pdwACO_ = dwACO_;
    *pdwSHACF_ = dwSHACF_;

    return TRUE;
}

static IEnumString *
AC_CreateEnumString(IAutoComplete2 *pAC2, HWND hwndEdit, DWORD dwSHACF_)
{
    IEnumString *ret;
    AC_EnumString *this = AC_EnumString_Construct(256);
    if (!this)
        return NULL;

    this->m_dwSHACF_ = dwSHACF_;
    this->m_hwndEdit = hwndEdit;

    ret = (IEnumString *)this;
    AC_EnumString_Reset(ret);

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
    DWORD dwACO_;
    DWORD dwClsCtx = CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER;

    if (!AC_AdaptFlags(hwndEdit, &dwACO_, &dwFlags))
    {
        return S_OK;
    }

    hr = CoCreateInstance(&CLSID_AutoComplete, NULL, dwClsCtx,
                          &IID_IAutoComplete, (LPVOID *)&pAC2);
    if (FAILED(hr))
    {
        return hr;
    }

    pES = AC_CreateEnumString(pAC2, hwndEdit, dwFlags);
    if (!pES)
    {
        IUnknown_Release(pAC2);
        return E_FAIL;
    }

    hr = IAutoComplete2_Init(pAC2, hwndEdit, (LPUNKNOWN)pES, NULL, L"www.%s.com");
    if (SUCCEEDED(hr))
    {
        IAutoComplete2_SetOptions(pAC2, dwACO_);
    }

    IUnknown_Release(pAC2);
    IUnknown_Release(pES);

    return hr;
}
