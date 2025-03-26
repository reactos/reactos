/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Utility functions
 * COPYRIGHT:   Copyright 2023-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmserver.h>
#include <secext.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static PCSTR StrEndNA(_In_ PCSTR psz, _In_ INT_PTR cch)
{
    PCSTR pch, pchEnd = &psz[cch];
    for (pch = psz; *pch && pch < pchEnd; pch = CharNextA(pch))
        ;
    if (pchEnd < pch) // A double-byte character detected at last?
        pch -= 2; // The width of a double-byte character is 2
    return pch;
}

static PCWSTR StrEndNW(_In_ PCWSTR psz, _In_ INT_PTR cch)
{
    PCWSTR pch, pchEnd = &psz[cch];
    for (pch = psz; *pch && pch < pchEnd; ++pch)
        ;
    return pch;
}

/*************************************************************************
 *  StrRStrA [SHELL32.389]
 */
EXTERN_C
PSTR WINAPI
StrRStrA(
    _In_ PCSTR pszSrc,
    _In_opt_ PCSTR pszLast,
    _In_ PCSTR pszSearch)
{
    INT cchSearch = lstrlenA(pszSearch);

    PCSTR pchEnd = pszLast ? pszLast : &pszSrc[lstrlenA(pszSrc)];
    if (pchEnd == pszSrc)
        return NULL;

    INT_PTR cchEnd = pchEnd - pszSrc;
    for (;;)
    {
        --pchEnd;
        --cchEnd;
        if (!pchEnd)
            break;
        if (!StrCmpNA(pchEnd, pszSearch, cchSearch) && pchEnd == StrEndNA(pszSrc, cchEnd))
            break;
        if (pchEnd == pszSrc)
            return NULL;
    }

    return const_cast<PSTR>(pchEnd);
}

/*************************************************************************
 *  StrRStrW [SHELL32.392]
 */
EXTERN_C
PWSTR WINAPI
StrRStrW(
    _In_ PCWSTR pszSrc,
    _In_opt_ PCWSTR pszLast,
    _In_ PCWSTR pszSearch)
{
    INT cchSearch = lstrlenW(pszSearch);

    PCWSTR pchEnd = pszLast ? pszLast : &pszSrc[lstrlenW(pszSrc)];
    if (pchEnd == pszSrc)
        return NULL;

    INT_PTR cchEnd = pchEnd - pszSrc;
    for (;;)
    {
        --pchEnd;
        --cchEnd;
        if (!pchEnd)
            break;
        if (!StrCmpNW(pchEnd, pszSearch, cchSearch) && pchEnd == StrEndNW(pszSrc, cchEnd))
            break;
        if (pchEnd == pszSrc)
            return NULL;
    }

    return const_cast<PWSTR>(pchEnd);
}

HWND
CStubWindow32::FindStubWindow(UINT Type, LPCWSTR Path)
{
    for (HWND hWnd, hWndAfter = NULL;;)
    {
        hWnd = hWndAfter = FindWindowExW(NULL, hWndAfter, CSTUBWINDOW32_CLASSNAME, Path);
        if (!hWnd || !Path)
            return NULL;
        if (GetPropW(hWnd, GetTypePropName()) == ULongToHandle(Type))
            return hWnd;
    }
}

HRESULT
CStubWindow32::CreateStub(UINT Type, LPCWSTR Path, const POINT *pPt)
{
    if (HWND hWnd = FindStubWindow(Type, Path))
    {
        ::SwitchToThisWindow(::GetLastActivePopup(hWnd), TRUE);
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    }
    RECT rcPosition = { pPt ? pPt->x : CW_USEDEFAULT, pPt ? pPt->y : CW_USEDEFAULT, 0, 0 };
    DWORD Style = WS_DISABLED | WS_CLIPSIBLINGS | WS_CAPTION;
    DWORD ExStyle = WS_EX_WINDOWEDGE | WS_EX_APPWINDOW;
    if (!Create(NULL, rcPosition, Path, Style, ExStyle))
    {
        ERR("StubWindow32 creation failed\n");
        return E_FAIL;
    }
    ::SetPropW(*this, GetTypePropName(), ULongToHandle(Type));
    return S_OK;
}

HRESULT
SHILClone(
    _In_opt_ LPCITEMIDLIST pidl,
    _Outptr_ LPITEMIDLIST *ppidl)
{
    if (!pidl)
    {
        *ppidl = NULL;
        return S_OK;
    }
    *ppidl = ILClone(pidl);
    return (*ppidl ? S_OK : E_OUTOFMEMORY);
}

BOOL PathIsDotOrDotDotW(_In_ LPCWSTR pszPath)
{
    if (pszPath[0] != L'.')
        return FALSE;
    return !pszPath[1] || (pszPath[1] == L'.' && !pszPath[2]);
}

#define PATH_VALID_ELEMENT ( \
    PATH_CHAR_CLASS_DOT | PATH_CHAR_CLASS_SEMICOLON | PATH_CHAR_CLASS_COMMA | \
    PATH_CHAR_CLASS_SPACE | PATH_CHAR_CLASS_OTHER_VALID \
)

BOOL PathIsValidElement(_In_ LPCWSTR pszPath)
{
    if (!*pszPath || PathIsDotOrDotDotW(pszPath))
        return FALSE;

    for (LPCWSTR pch = pszPath; *pch; ++pch)
    {
        if (!PathIsValidCharW(*pch, PATH_VALID_ELEMENT))
            return FALSE;
    }

    return TRUE;
}

BOOL PathIsDosDevice(_In_ LPCWSTR pszName)
{
    WCHAR szPath[MAX_PATH];
    StringCchCopyW(szPath, _countof(szPath), pszName);
    PathRemoveExtensionW(szPath);

    if (lstrcmpiW(szPath, L"NUL") == 0 || lstrcmpiW(szPath, L"PRN") == 0 ||
        lstrcmpiW(szPath, L"CON") == 0 || lstrcmpiW(szPath, L"AUX") == 0)
    {
        return TRUE;
    }

    if (_wcsnicmp(szPath, L"LPT", 3) == 0 || _wcsnicmp(szPath, L"COM", 3) == 0)
    {
        if ((L'0' <= szPath[3] && szPath[3] <= L'9') && szPath[4] == UNICODE_NULL)
            return TRUE;
    }

    return FALSE;
}

HRESULT SHILAppend(_Inout_ LPITEMIDLIST pidl, _Inout_ LPITEMIDLIST *ppidl)
{
    LPITEMIDLIST pidlOld = *ppidl;
    if (!pidlOld)
    {
        *ppidl = pidl;
        return S_OK;
    }

    HRESULT hr = SHILCombine(*ppidl, pidl, ppidl);
    ILFree(pidlOld);
    ILFree(pidl);
    return hr;
}

/*************************************************************************
 *  SHShouldShowWizards [SHELL32.237]
 *
 * Used by printer and network features.
 * @see https://undoc.airesoft.co.uk/shell32.dll/SHShouldShowWizards.php
 */
EXTERN_C
HRESULT WINAPI
SHShouldShowWizards(_In_ IUnknown *pUnknown)
{
    HRESULT hr;
    IShellBrowser *pBrowser;

    hr = IUnknown_QueryService(pUnknown, SID_STopWindow, IID_PPV_ARG(IShellBrowser, &pBrowser));
    if (FAILED(hr))
        return hr;

    SHELLSTATE state;
    SHGetSetSettings(&state, SSF_WEBVIEW, FALSE);
    if (state.fWebView &&
        !SHRegGetBoolUSValueW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                              L"ShowWizardsTEST", FALSE, FALSE))
    {
        hr = S_FALSE;
    }

    pBrowser->Release();
    return hr;
}

static BOOL
OpenEffectiveToken(
    _In_ DWORD DesiredAccess,
    _Out_ HANDLE *phToken)
{
    BOOL ret;

    if (phToken == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phToken = NULL;

    ret = OpenThreadToken(GetCurrentThread(), DesiredAccess, FALSE, phToken);
    if (!ret && GetLastError() == ERROR_NO_TOKEN)
        ret = OpenProcessToken(GetCurrentProcess(), DesiredAccess, phToken);

    return ret;
}

HRESULT
Shell_TranslateIDListAlias(
    _In_ LPCITEMIDLIST pidl,
    _In_ HANDLE hToken,
    _Out_ LPITEMIDLIST *ppidlAlias,
    _In_ DWORD dwFlags)
{
    return E_FAIL; //FIXME
}

BOOL BindCtx_ContainsObject(_In_ IBindCtx *pBindCtx, _In_ LPCWSTR pszName)
{
    CComPtr<IUnknown> punk;
    if (!pBindCtx || FAILED(pBindCtx->GetObjectParam(const_cast<LPWSTR>(pszName), &punk)))
        return FALSE;
    return TRUE;
}

DWORD BindCtx_GetMode(_In_ IBindCtx *pbc, _In_ DWORD dwDefault)
{
    if (!pbc)
        return dwDefault;

    BIND_OPTS BindOpts = { sizeof(BindOpts) };
    HRESULT hr = pbc->GetBindOptions(&BindOpts);
    if (FAILED(hr))
        return dwDefault;

    return BindOpts.grfMode;
}

BOOL SHSkipJunctionBinding(_In_ IBindCtx *pbc, _In_ CLSID *pclsid)
{
    if (!pbc)
        return FALSE;

    BIND_OPTS BindOps = { sizeof(BindOps) };
    if (SUCCEEDED(pbc->GetBindOptions(&BindOps)) && BindOps.grfFlags == OLECONTF_LINKS)
        return TRUE;

    return pclsid && SHSkipJunction(pbc, pclsid);
}

HRESULT SHIsFileSysBindCtx(_In_ IBindCtx *pBindCtx, _Out_opt_ WIN32_FIND_DATAW *pFindData)
{
    CComPtr<IUnknown> punk;
    CComPtr<IFileSystemBindData> pBindData;

    if (!pBindCtx || FAILED(pBindCtx->GetObjectParam((LPWSTR)STR_FILE_SYS_BIND_DATA, &punk)))
        return S_FALSE;

    if (FAILED(punk->QueryInterface(IID_PPV_ARG(IFileSystemBindData, &pBindData))))
        return S_FALSE;

    if (pFindData)
        pBindData->GetFindData(pFindData);

    return S_OK;
}

BOOL Shell_FailForceReturn(_In_ HRESULT hr)
{
    DWORD code = HRESULT_CODE(hr);

    switch (code)
    {
        case ERROR_BAD_NETPATH:
        case ERROR_BAD_NET_NAME:
        case ERROR_CANCELLED:
            return TRUE;

        default:
            return (ERROR_FILE_NOT_FOUND <= code && code <= ERROR_PATH_NOT_FOUND);
    }
}

HRESULT
SHBindToObjectEx(
    _In_opt_ IShellFolder *pShellFolder,
    _In_ LPCITEMIDLIST pidl,
    _In_opt_ IBindCtx *pBindCtx,
    _In_ REFIID riid,
    _Out_ void **ppvObj)
{
    CComPtr<IShellFolder> psfDesktop;

    *ppvObj = NULL;

    if (!pShellFolder)
    {
        SHGetDesktopFolder(&psfDesktop);
        if (!psfDesktop)
            return E_FAIL;

        pShellFolder = psfDesktop;
    }

    HRESULT hr;
    if (_ILIsDesktop(pidl))
        hr = pShellFolder->QueryInterface(riid, ppvObj);
    else
        hr = pShellFolder->BindToObject(pidl, pBindCtx, riid, ppvObj);

    if (SUCCEEDED(hr) && !*ppvObj)
        hr = E_FAIL;

    return hr;
}

EXTERN_C
HRESULT SHBindToObject(
    _In_opt_ IShellFolder *psf,
    _In_ LPCITEMIDLIST pidl,
    _In_ REFIID riid,
    _Out_ void **ppvObj)
{
    return SHBindToObjectEx(psf, pidl, NULL, riid, ppvObj);
}

EXTERN_C HRESULT
SHELL_GetUIObjectOfAbsoluteItem(
    _In_opt_ HWND hWnd,
    _In_ PCIDLIST_ABSOLUTE pidl,
    _In_ REFIID riid, _Out_ void **ppvObj)
{
    if (!ppvObj)
        return E_INVALIDARG;
    *ppvObj = NULL;
    IShellFolder *psf;
    PCUITEMID_CHILD pidlChild;
    HRESULT hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
    if (SUCCEEDED(hr))
    {
        hr = psf->GetUIObjectOf(hWnd, 1, &pidlChild, riid, NULL, ppvObj);
        psf->Release();
        if (SUCCEEDED(hr))
        {
            if (*ppvObj)
                return hr;
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT
Shell_DisplayNameOf(
    _In_ IShellFolder *psf,
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwFlags,
    _Out_ LPWSTR pszBuf,
    _In_ UINT cchBuf)
{
    *pszBuf = UNICODE_NULL;
    STRRET sr;
    HRESULT hr = psf->GetDisplayNameOf(pidl, dwFlags, &sr);
    if (FAILED(hr))
        return hr;
    return StrRetToBufW(&sr, pidl, pszBuf, cchBuf);
}

DWORD
SHGetAttributes(_In_ IShellFolder *psf, _In_ LPCITEMIDLIST pidl, _In_ DWORD dwAttributes)
{
    LPCITEMIDLIST pidlLast = pidl;
    IShellFolder *release = NULL;

    if (!psf)
    {
        SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
        if (!psf)
            return 0;
        release = psf;
    }

    DWORD oldAttrs = dwAttributes;
    if (FAILED(psf->GetAttributesOf(1, &pidlLast, &dwAttributes)))
        dwAttributes = 0;
    else
        dwAttributes &= oldAttrs;

    if ((dwAttributes & SFGAO_FOLDER) &&
        (dwAttributes & SFGAO_STREAM) &&
        !(dwAttributes & SFGAO_STORAGEANCESTOR) &&
        (oldAttrs & SFGAO_STORAGEANCESTOR) &&
        (SHGetObjectCompatFlags(psf, NULL) & 0x200))
    {
        dwAttributes &= ~(SFGAO_STREAM | SFGAO_STORAGEANCESTOR);
        dwAttributes |= SFGAO_STORAGEANCESTOR;
    }

    if (release)
        release->Release();
    return dwAttributes;
}

HRESULT SHELL_GetIDListTarget(_In_ LPCITEMIDLIST pidl, _Out_ PIDLIST_ABSOLUTE *ppidl)
{
    IShellLink *pSL;
    HRESULT hr = SHBindToObject(NULL, pidl, IID_PPV_ARG(IShellLink, &pSL));
    if (SUCCEEDED(hr))
    {
        hr = pSL->GetIDList(ppidl); // Note: Returns S_FALSE if no target pidl
        pSL->Release();
    }
    return hr;
}

HRESULT SHCoInitializeAnyApartment(VOID)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
        hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE);
    return hr;
}

HRESULT
SHGetNameAndFlagsW(
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwFlags,
    _Out_opt_ LPWSTR pszText,
    _In_ UINT cchBuf,
    _Inout_opt_ DWORD *pdwAttributes)
{
    if (pszText)
        *pszText = UNICODE_NULL;

    HRESULT hrCoInit = SHCoInitializeAnyApartment();

    CComPtr<IShellFolder> psfFolder;
    LPCITEMIDLIST ppidlLast;
    HRESULT hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psfFolder), &ppidlLast);
    if (SUCCEEDED(hr))
    {
        if (pszText)
            hr = Shell_DisplayNameOf(psfFolder, ppidlLast, dwFlags, pszText, cchBuf);

        if (SUCCEEDED(hr))
        {
            if (pdwAttributes)
                *pdwAttributes = SHGetAttributes(psfFolder, ppidlLast, *pdwAttributes);
        }
    }

    if (SUCCEEDED(hrCoInit))
        CoUninitialize();

    return hr;
}

EXTERN_C HWND
BindCtx_GetUIWindow(_In_ IBindCtx *pBindCtx)
{
    HWND hWnd = NULL;

    CComPtr<IUnknown> punk;
    if (pBindCtx && SUCCEEDED(pBindCtx->GetObjectParam((LPWSTR)L"UI During Binding", &punk)))
        IUnknown_GetWindow(punk, &hWnd);

    return hWnd;
}

class CDummyOleWindow : public IOleWindow
{
protected:
    LONG m_cRefs;
    HWND m_hWnd;

public:
    CDummyOleWindow() : m_cRefs(1), m_hWnd(NULL) { }
    virtual ~CDummyOleWindow() { }

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) override
    {
        static const QITAB c_tab[] =
        {
            QITABENT(CDummyOleWindow, IOleWindow),
            { NULL }
        };
        return ::QISearch(this, c_tab, riid, ppvObj);
    }
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return ++m_cRefs;
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        if (--m_cRefs == 0)
        {
            delete this;
            return 0;
        }
        return m_cRefs;
    }

    // IOleWindow methods
    STDMETHODIMP GetWindow(HWND *phWnd) override
    {
        *phWnd = m_hWnd;
        if (!m_hWnd)
            return E_NOTIMPL;
        return S_OK;
    }
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) override
    {
        return E_NOTIMPL;
    }
};

EXTERN_C HRESULT
BindCtx_RegisterObjectParam(
    _In_ IBindCtx *pBindCtx,
    _In_ LPOLESTR pszKey,
    _In_opt_ IUnknown *punk,
    _Out_ LPBC *ppbc)
{
    HRESULT hr = S_OK;
    CDummyOleWindow *pUnknown = NULL;

    *ppbc = pBindCtx;

    if (pBindCtx)
    {
        pBindCtx->AddRef();
    }
    else
    {
        hr = CreateBindCtx(0, ppbc);
        if (FAILED(hr))
            return hr;
    }

    if (!punk)
        punk = pUnknown = new CDummyOleWindow();

    hr = (*ppbc)->RegisterObjectParam(pszKey, punk);

    if (pUnknown)
        pUnknown->Release();

    if (FAILED(hr))
    {
        (*ppbc)->Release();
        *ppbc = NULL;
    }

    return hr;
}

/*************************************************************************
 *                SHSetFolderPathA (SHELL32.231)
 *
 * @see https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shsetfolderpatha
 */
EXTERN_C
HRESULT WINAPI
SHSetFolderPathA(
    _In_ INT csidl,
    _In_ HANDLE hToken,
    _In_ DWORD dwFlags,
    _In_ LPCSTR pszPath)
{
    TRACE("(%d, %p, 0x%X, %s)\n", csidl, hToken, dwFlags, debugstr_a(pszPath));
    CStringW strPathW(pszPath);
    return SHSetFolderPathW(csidl, hToken, dwFlags, strPathW);
}

/*************************************************************************
 *                PathIsSlowA (SHELL32.240)
 *
 * @see https://learn.microsoft.com/en-us/windows/win32/api/shlobj/nf-shlobj-pathisslowa
 */
EXTERN_C
BOOL WINAPI
PathIsSlowA(
    _In_ LPCSTR pszFile,
    _In_ DWORD dwAttr)
{
    TRACE("(%s, 0x%X)\n", debugstr_a(pszFile), dwAttr);
    CStringW strFileW(pszFile);
    return PathIsSlowW(strFileW, dwAttr);
}

/*************************************************************************
 *                ExtractIconResInfoA (SHELL32.221)
 */
EXTERN_C
WORD WINAPI
ExtractIconResInfoA(
    _In_ HANDLE hHandle,
    _In_ LPCSTR lpFileName,
    _In_ WORD wIndex,
    _Out_ LPWORD lpSize,
    _Out_ LPHANDLE lpIcon)
{
    TRACE("(%p, %s, %u, %p, %p)\n", hHandle, debugstr_a(lpFileName), wIndex, lpSize, lpIcon);

    if (!lpFileName)
        return 0;

    CStringW strFileNameW(lpFileName);
    return ExtractIconResInfoW(hHandle, strFileNameW, wIndex, lpSize, lpIcon);
}

/*************************************************************************
 *                ShortSizeFormatW (SHELL32.204)
 */
EXTERN_C
LPWSTR WINAPI
ShortSizeFormatW(
    _In_ DWORD dwNumber,
    _Out_writes_(0x8FFF) LPWSTR pszBuffer)
{
    TRACE("(%lu, %p)\n", dwNumber, pszBuffer);
    return StrFormatByteSizeW(dwNumber, pszBuffer, 0x8FFF);
}

/*************************************************************************
 *                SHOpenEffectiveToken (SHELL32.235)
 */
EXTERN_C BOOL WINAPI SHOpenEffectiveToken(_Out_ LPHANDLE phToken)
{
    TRACE("%p\n", phToken);
    return OpenEffectiveToken(TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, phToken);
}

/*************************************************************************
 *                SHGetUserSessionId (SHELL32.248)
 */
EXTERN_C DWORD WINAPI SHGetUserSessionId(_In_opt_ HANDLE hToken)
{
    DWORD dwSessionId, dwLength;
    BOOL bOpenToken = FALSE;

    TRACE("%p\n", hToken);

    if (!hToken)
        bOpenToken = SHOpenEffectiveToken(&hToken);

    if (!hToken ||
        !GetTokenInformation(hToken, TokenSessionId, &dwSessionId, sizeof(dwSessionId), &dwLength))
    {
        dwSessionId = 0;
    }

    if (bOpenToken)
        CloseHandle(hToken);

    return dwSessionId;
}

/*************************************************************************
 *                SHInvokePrivilegedFunctionW (SHELL32.246)
 */
EXTERN_C
HRESULT WINAPI
SHInvokePrivilegedFunctionW(
    _In_ LPCWSTR pszName,
    _In_ PRIVILEGED_FUNCTION fn,
    _In_opt_ LPARAM lParam)
{
    TRACE("(%s %p %p)\n", debugstr_w(pszName), fn, lParam);

    if (!pszName || !fn)
        return E_INVALIDARG;

    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES NewPriv, PrevPriv;
    BOOL bAdjusted = FALSE;

    if (SHOpenEffectiveToken(&hToken) &&
        ::LookupPrivilegeValueW(NULL, pszName, &NewPriv.Privileges[0].Luid))
    {
        NewPriv.PrivilegeCount = 1;
        NewPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        DWORD dwReturnSize;
        bAdjusted = ::AdjustTokenPrivileges(hToken, FALSE, &NewPriv,
                                            sizeof(PrevPriv), &PrevPriv, &dwReturnSize);
    }

    HRESULT hr = fn(lParam);

    if (bAdjusted)
        ::AdjustTokenPrivileges(hToken, FALSE, &PrevPriv, 0, NULL, NULL);

    if (hToken)
        ::CloseHandle(hToken);

    return hr;
}

/*************************************************************************
 *                SHTestTokenPrivilegeW (SHELL32.236)
 *
 * @see http://undoc.airesoft.co.uk/shell32.dll/SHTestTokenPrivilegeW.php
 */
EXTERN_C
BOOL WINAPI
SHTestTokenPrivilegeW(
    _In_opt_ HANDLE hToken,
    _In_ LPCWSTR lpName)
{
    LUID Luid;
    DWORD dwLength;
    PTOKEN_PRIVILEGES pTokenPriv;
    HANDLE hNewToken = NULL;
    BOOL ret = FALSE;

    TRACE("(%p, %s)\n", hToken, debugstr_w(lpName));

    if (!lpName)
        return FALSE;

    if (!hToken)
    {
        if (!SHOpenEffectiveToken(&hNewToken))
            goto Quit;

        if (!hNewToken)
            return FALSE;

        hToken = hNewToken;
    }

    if (!LookupPrivilegeValueW(NULL, lpName, &Luid))
        return FALSE;

    dwLength = 0;
    if (!GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwLength))
        goto Quit;

    pTokenPriv = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, dwLength);
    if (!pTokenPriv)
        goto Quit;

    if (GetTokenInformation(hToken, TokenPrivileges, pTokenPriv, dwLength, &dwLength))
    {
        UINT iPriv, cPrivs;
        cPrivs = pTokenPriv->PrivilegeCount;
        for (iPriv = 0; !ret && iPriv < cPrivs; ++iPriv)
        {
            ret = RtlEqualLuid(&Luid, &pTokenPriv->Privileges[iPriv].Luid);
        }
    }

    LocalFree(pTokenPriv);

Quit:
    if (hToken == hNewToken)
        CloseHandle(hNewToken);

    return ret;
}

BOOL IsShutdownAllowed(VOID)
{
    return SHTestTokenPrivilegeW(NULL, SE_SHUTDOWN_NAME);
}

/*************************************************************************
 *                IsSuspendAllowed (SHELL32.53)
 */
BOOL WINAPI IsSuspendAllowed(VOID)
{
    TRACE("()\n");
    return IsShutdownAllowed() && IsPwrSuspendAllowed();
}

/*************************************************************************
 *                SHGetShellStyleHInstance (SHELL32.749)
 */
EXTERN_C HINSTANCE
WINAPI
SHGetShellStyleHInstance(VOID)
{
    HINSTANCE hInst = NULL;
    WCHAR szPath[MAX_PATH], szColorName[100];
    HRESULT hr;
    CStringW strShellStyle;

    TRACE("SHGetShellStyleHInstance called\n");

    /* First, attempt to load the shellstyle dll from the current active theme */
    hr = GetCurrentThemeName(szPath, _countof(szPath), szColorName, _countof(szColorName), NULL, 0);
    if (FAILED(hr))
        goto DoDefault;

    /* Strip the theme filename */
    PathRemoveFileSpecW(szPath);

    strShellStyle = szPath;
    strShellStyle += L"\\Shell\\";
    strShellStyle += szColorName;
    strShellStyle += L"\\ShellStyle.dll";

    hInst = LoadLibraryExW(strShellStyle, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hInst)
        return hInst;

    /* Otherwise, use the version stored in the System32 directory */
DoDefault:
    if (!ExpandEnvironmentStringsW(L"%SystemRoot%\\System32\\ShellStyle.dll",
                                   szPath, _countof(szPath)))
    {
        ERR("Expand failed\n");
        return NULL;
    }
    return LoadLibraryExW(szPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
}

/*************************************************************************
 *                SHCreatePropertyBag (SHELL32.715)
 */
EXTERN_C HRESULT
WINAPI
SHCreatePropertyBag(_In_ REFIID riid, _Out_ void **ppvObj)
{
    return SHCreatePropertyBagOnMemory(STGM_READWRITE, riid, ppvObj);
}

// The helper function for SHGetUnreadMailCountW
static DWORD
SHELL_ReadSingleUnreadMailCount(
    _In_ HKEY hKey,
    _Out_opt_ PDWORD pdwCount,
    _Out_opt_ PFILETIME pFileTime,
    _Out_writes_opt_(cchShellExecuteCommand) LPWSTR pszShellExecuteCommand,
    _In_ INT cchShellExecuteCommand)
{
    DWORD dwType, dwCount, cbSize = sizeof(dwCount);
    DWORD error = SHQueryValueExW(hKey, L"MessageCount", 0, &dwType, &dwCount, &cbSize);
    if (error)
        return error;
    if (pdwCount && dwType == REG_DWORD)
        *pdwCount = dwCount;

    FILETIME FileTime;
    cbSize = sizeof(FileTime);
    error = SHQueryValueExW(hKey, L"TimeStamp", 0, &dwType, &FileTime, &cbSize);
    if (error)
        return error;
    if (pFileTime && dwType == REG_BINARY)
        *pFileTime = FileTime;

    WCHAR szName[2 * MAX_PATH];
    cbSize = sizeof(szName);
    error = SHQueryValueExW(hKey, L"Application", 0, &dwType, szName, &cbSize);
    if (error)
        return error;

    if (pszShellExecuteCommand && dwType == REG_SZ &&
        FAILED(StringCchCopyW(pszShellExecuteCommand, cchShellExecuteCommand, szName)))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    return ERROR_SUCCESS;
}

/*************************************************************************
 *  SHGetUnreadMailCountW [SHELL32.320]
 *
 * @see https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shgetunreadmailcountw
 */
EXTERN_C
HRESULT WINAPI
SHGetUnreadMailCountW(
    _In_opt_ HKEY hKeyUser,
    _In_opt_ PCWSTR pszMailAddress,
    _Out_opt_ PDWORD pdwCount,
    _Inout_opt_ PFILETIME pFileTime,
    _Out_writes_opt_(cchShellExecuteCommand) PWSTR pszShellExecuteCommand,
    _In_ INT cchShellExecuteCommand)
{
    LSTATUS error;
    HKEY hKey;

    if (!hKeyUser)
        hKeyUser = HKEY_CURRENT_USER;

    if (pszMailAddress)
    {
        CStringW strKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail";
        strKey += L'\\';
        strKey += pszMailAddress;

        error = RegOpenKeyExW(hKeyUser, strKey, 0, KEY_QUERY_VALUE, &hKey);
        if (error)
            return HRESULT_FROM_WIN32(error);

        error = SHELL_ReadSingleUnreadMailCount(hKey, pdwCount, pFileTime,
                                                pszShellExecuteCommand, cchShellExecuteCommand);
    }
    else
    {
        if (pszShellExecuteCommand || cchShellExecuteCommand)
            return E_INVALIDARG;

        *pdwCount = 0;

        error = RegOpenKeyExW(hKeyUser, L"Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail",
                              0, KEY_ENUMERATE_SUB_KEYS, &hKey);
        if (error)
            return HRESULT_FROM_WIN32(error);

        for (DWORD dwIndex = 0; !error; ++dwIndex)
        {
            WCHAR Name[2 * MAX_PATH];
            DWORD cchName = _countof(Name);
            FILETIME LastWritten;
            error = RegEnumKeyExW(hKey, dwIndex, Name, &cchName, NULL, NULL, NULL, &LastWritten);
            if (error)
                break;

            HKEY hSubKey;
            error = RegOpenKeyExW(hKey, Name, 0, KEY_QUERY_VALUE, &hSubKey);
            if (error)
                break;

            FILETIME FileTime;
            DWORD dwCount;
            error = SHELL_ReadSingleUnreadMailCount(hSubKey, &dwCount, &FileTime, NULL, 0);
            if (!error && (!pFileTime || CompareFileTime(&FileTime, pFileTime) >= 0))
                *pdwCount += dwCount;

            RegCloseKey(hSubKey);
        }

        if (error == ERROR_NO_MORE_ITEMS)
            error = ERROR_SUCCESS;
    }

    RegCloseKey(hKey);

    return error ? HRESULT_FROM_WIN32(error) : S_OK;
}

/*************************************************************************
 *  SHSetUnreadMailCountW [SHELL32.336]
 *
 * @see https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shsetunreadmailcountw
 */
EXTERN_C
HRESULT WINAPI
SHSetUnreadMailCountW(
    _In_ PCWSTR pszMailAddress,
    _In_ DWORD dwCount,
    _In_ PCWSTR pszShellExecuteCommand)
{
    CString strKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail\\";
    strKey += pszMailAddress;

    HKEY hKey;
    DWORD dwDisposition;
    LSTATUS error = RegCreateKeyExW(HKEY_CURRENT_USER, strKey, 0, NULL, 0, KEY_SET_VALUE, NULL,
                                    &hKey, &dwDisposition);
    if (error)
        return HRESULT_FROM_WIN32(error);

    error = RegSetValueExW(hKey, L"MessageCount", 0, REG_DWORD, (PBYTE)&dwCount, sizeof(dwCount));
    if (error)
    {
        RegCloseKey(hKey);
        return HRESULT_FROM_WIN32(error);
    }

    FILETIME FileTime;
    GetSystemTimeAsFileTime(&FileTime);

    error = RegSetValueExW(hKey, L"TimeStamp", 0, REG_BINARY, (PBYTE)&FileTime, sizeof(FileTime));
    if (error)
    {
        RegCloseKey(hKey);
        return HRESULT_FROM_WIN32(error);
    }

    WCHAR szBuff[2 * MAX_PATH];
    if (!PathUnExpandEnvStringsW(pszShellExecuteCommand, szBuff, _countof(szBuff)))
    {
        HRESULT hr = StringCchCopyW(szBuff, _countof(szBuff), pszShellExecuteCommand);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            RegCloseKey(hKey);
            return hr;
        }
    }

    DWORD cbValue = (lstrlenW(szBuff) + 1) * sizeof(WCHAR);
    error = RegSetValueExW(hKey, L"Application", 0, REG_SZ, (PBYTE)szBuff, cbValue);

    RegCloseKey(hKey);
    return (error ? HRESULT_FROM_WIN32(error) : S_OK);
}

/*************************************************************************
 *                SheRemoveQuotesA (SHELL32.@)
 */
EXTERN_C LPSTR
WINAPI
SheRemoveQuotesA(LPSTR psz)
{
    PCHAR pch;

    if (*psz == '"')
    {
        for (pch = psz + 1; *pch && *pch != '"'; ++pch)
        {
            *(pch - 1) = *pch;
        }

        if (*pch == '"')
            *(pch - 1) = ANSI_NULL;
    }

    return psz;
}

/*************************************************************************
 *                SheRemoveQuotesW (SHELL32.@)
 *
 * ExtractAssociatedIconExW uses this function.
 */
EXTERN_C LPWSTR
WINAPI
SheRemoveQuotesW(LPWSTR psz)
{
    PWCHAR pch;

    if (*psz == L'"')
    {
        for (pch = psz + 1; *pch && *pch != L'"'; ++pch)
        {
            *(pch - 1) = *pch;
        }

        if (*pch == L'"')
            *(pch - 1) = UNICODE_NULL;
    }

    return psz;
}

/*************************************************************************
 *  SHEnumerateUnreadMailAccountsW [SHELL32.287]
 *
 * @see https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shenumerateunreadmailaccountsw
 */
EXTERN_C
HRESULT WINAPI
SHEnumerateUnreadMailAccountsW(
    _In_opt_ HKEY hKeyUser,
    _In_ DWORD dwIndex,
    _Out_writes_(cchMailAddress) PWSTR pszMailAddress,
    _In_ INT cchMailAddress)
{
    if (!hKeyUser)
        hKeyUser = HKEY_CURRENT_USER;

    HKEY hKey;
    LSTATUS error = RegOpenKeyExW(hKeyUser,
                                  L"Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail",
                                  0, KEY_ENUMERATE_SUB_KEYS, &hKey);
    if (error)
        return HRESULT_FROM_WIN32(error);

    FILETIME FileTime;
    error = RegEnumKeyExW(hKey, dwIndex, pszMailAddress, (PDWORD)&cchMailAddress, NULL, NULL,
                          NULL, &FileTime);
    if (error)
        *pszMailAddress = UNICODE_NULL;

    RegCloseKey(hKey);
    return error ? HRESULT_FROM_WIN32(error) : S_OK;
}

/*************************************************************************
 *  SHFindComputer [SHELL32.91]
 *
 * Invokes the shell search in My Computer. Used in SHFindFiles.
 * Two parameters are ignored.
 */
EXTERN_C BOOL
WINAPI
SHFindComputer(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlSavedSearch)
{
    UNREFERENCED_PARAMETER(pidlRoot);
    UNREFERENCED_PARAMETER(pidlSavedSearch);

    TRACE("%p %p\n", pidlRoot, pidlSavedSearch);

    IContextMenu *pCM;
    HRESULT hr = CoCreateInstance(CLSID_ShellSearchExt, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IContextMenu, (void **)&pCM);
    if (FAILED(hr))
    {
        ERR("0x%08X\n", hr);
        return hr;
    }

    CMINVOKECOMMANDINFO InvokeInfo = { sizeof(InvokeInfo) };
    InvokeInfo.lpParameters = "{996E1EB1-B524-11D1-9120-00A0C98BA67D}";
    InvokeInfo.nShow = SW_SHOWNORMAL;
    hr = pCM->InvokeCommand(&InvokeInfo);
    pCM->Release();

    return SUCCEEDED(hr);
}

static HRESULT
Int64ToStr(
    _In_ LONGLONG llValue,
    _Out_writes_(cchValue) LPWSTR pszValue,
    _In_ UINT cchValue)
{
    WCHAR szBuff[40];
    UINT ich = 0, ichValue;
#if (WINVER >= _WIN32_WINNT_VISTA)
    BOOL bMinus = (llValue < 0);

    if (bMinus)
        llValue = -llValue;
#endif

    if (cchValue <= 0)
        return E_FAIL;

    do
    {
        szBuff[ich++] = (WCHAR)(L'0' + (llValue % 10));
        llValue /= 10;
    } while (llValue != 0 && ich < _countof(szBuff) - 1);

#if (WINVER >= _WIN32_WINNT_VISTA)
    if (bMinus && ich < _countof(szBuff))
        szBuff[ich++] = '-';
#endif

    for (ichValue = 0; ich > 0 && ichValue < cchValue; ++ichValue)
    {
        --ich;
        pszValue[ichValue] = szBuff[ich];
    }

    if (ichValue >= cchValue)
    {
        pszValue[cchValue - 1] = UNICODE_NULL;
        return E_FAIL;
    }

    pszValue[ichValue] = UNICODE_NULL;
    return S_OK;
}

static VOID
Int64GetNumFormat(
    _Out_ NUMBERFMTW *pDest,
    _In_opt_ const NUMBERFMTW *pSrc,
    _In_ DWORD dwNumberFlags,
    _Out_writes_(cchDecimal) LPWSTR pszDecimal,
    _In_ INT cchDecimal,
    _Out_writes_(cchThousand) LPWSTR pszThousand,
    _In_ INT cchThousand)
{
    WCHAR szBuff[20];

    if (pSrc)
        *pDest = *pSrc;
    else
        dwNumberFlags = 0;

    if (!(dwNumberFlags & FMT_USE_NUMDIGITS))
    {
        GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, szBuff, _countof(szBuff));
        pDest->NumDigits = StrToIntW(szBuff);
    }

    if (!(dwNumberFlags & FMT_USE_LEADZERO))
    {
        GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ILZERO, szBuff, _countof(szBuff));
        pDest->LeadingZero = StrToIntW(szBuff);
    }

    if (!(dwNumberFlags & FMT_USE_GROUPING))
    {
        GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szBuff, _countof(szBuff));
        pDest->Grouping = StrToIntW(szBuff);
    }

    if (!(dwNumberFlags & FMT_USE_DECIMAL))
    {
        GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, pszDecimal, cchDecimal);
        pDest->lpDecimalSep = pszDecimal;
    }

    if (!(dwNumberFlags & FMT_USE_THOUSAND))
    {
        GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, pszThousand, cchThousand);
        pDest->lpThousandSep = pszThousand;
    }

    if (!(dwNumberFlags & FMT_USE_NEGNUMBER))
    {
        GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, szBuff, _countof(szBuff));
        pDest->NegativeOrder = StrToIntW(szBuff);
    }
}

/*************************************************************************
 *  Int64ToString [SHELL32.209]
 *
 * @see http://undoc.airesoft.co.uk/shell32.dll/Int64ToString.php
 */
EXTERN_C
INT WINAPI
Int64ToString(
    _In_ LONGLONG llValue,
    _Out_writes_(cchOut) LPWSTR pszOut,
    _In_ UINT cchOut,
    _In_ BOOL bUseFormat,
    _In_opt_ const NUMBERFMTW *pNumberFormat,
    _In_ DWORD dwNumberFlags)
{
    INT ret;
    NUMBERFMTW NumFormat;
    WCHAR szValue[80], szDecimalSep[6], szThousandSep[6];

    Int64ToStr(llValue, szValue, _countof(szValue));

    if (bUseFormat)
    {
        Int64GetNumFormat(&NumFormat, pNumberFormat, dwNumberFlags,
                          szDecimalSep, _countof(szDecimalSep),
                          szThousandSep, _countof(szThousandSep));
        ret = GetNumberFormatW(LOCALE_USER_DEFAULT, 0, szValue, &NumFormat, pszOut, cchOut);
        if (ret)
            --ret;
        return ret;
    }

    if (FAILED(StringCchCopyW(pszOut, cchOut, szValue)))
        return 0;

    return lstrlenW(pszOut);
}

/*************************************************************************
 *  LargeIntegerToString [SHELL32.210]
 *
 * @see http://undoc.airesoft.co.uk/shell32.dll/LargeIntegerToString.php
 */
EXTERN_C
INT WINAPI
LargeIntegerToString(
    _In_ const LARGE_INTEGER *pLargeInt,
    _Out_writes_(cchOut) LPWSTR pszOut,
    _In_ UINT cchOut,
    _In_ BOOL bUseFormat,
    _In_opt_ const NUMBERFMTW *pNumberFormat,
    _In_ DWORD dwNumberFlags)
{
    return Int64ToString(pLargeInt->QuadPart, pszOut, cchOut, bUseFormat,
                         pNumberFormat, dwNumberFlags);
}

/*************************************************************************
 *  CopyStreamUI [SHELL32.726]
 *
 * Copy a stream to another stream with optional progress display.
 */
EXTERN_C
HRESULT WINAPI
CopyStreamUI(
    _In_ IStream *pSrc,
    _Out_ IStream *pDst,
    _Inout_opt_ IProgressDialog *pProgress,
    _In_opt_ DWORDLONG dwlSize)
{
    HRESULT hr = E_FAIL;
    DWORD cbBuff, cbRead, dwSizeToWrite;
    DWORDLONG cbDone;
    LPVOID pBuff;
    CComHeapPtr<BYTE> pHeapPtr;
    STATSTG Stat;
    BYTE abBuff[1024];

    TRACE("(%p, %p, %p, %I64u)\n", pSrc, pDst, pProgress, dwlSize);

    if (dwlSize == 0) // Invalid size?
    {
        // Get the stream size
        ZeroMemory(&Stat, sizeof(Stat));
        if (FAILED(pSrc->Stat(&Stat, STATFLAG_NONAME)))
            pProgress = NULL; // No size info. Disable progress
        else
            dwlSize = Stat.cbSize.QuadPart;
    }

    if (!pProgress) // Progress is disabled?
    {
        ULARGE_INTEGER uliSize;

        if (dwlSize > 0)
            uliSize.QuadPart = dwlSize;
        else
            uliSize.HighPart = uliSize.LowPart = INVALID_FILE_SIZE;

        return pSrc->CopyTo(pDst, uliSize, NULL, NULL); // One punch
    }

    // Allocate the buffer if necessary
    if (dwlSize > 0 && dwlSize <= sizeof(abBuff))
    {
        cbBuff = sizeof(abBuff);
        pBuff = abBuff;
    }
    else
    {
#define COPY_STREAM_DEFAULT_BUFFER_SIZE 0x4000
        cbBuff = COPY_STREAM_DEFAULT_BUFFER_SIZE;
        if (pHeapPtr.AllocateBytes(cbBuff))
        {
            pBuff = pHeapPtr;
        }
        else // Low memory?
        {
            cbBuff = sizeof(abBuff);
            pBuff = abBuff;
        }
#undef COPY_STREAM_DEFAULT_BUFFER_SIZE
    }

    // Start reading
    LARGE_INTEGER zero;
    zero.QuadPart = 0;
    pSrc->Seek(zero, 0, NULL);
    pDst->Seek(zero, 0, NULL);
    cbDone = 0;
    pProgress->SetProgress64(cbDone, dwlSize);

    // Repeat reading and writing until goal
    for (;;)
    {
        hr = pSrc->Read(pBuff, cbBuff, &cbRead);
        if (FAILED(hr))
            break;

        // Calculate the size to write
        if (dwlSize > 0)
            dwSizeToWrite = (DWORD)min((DWORDLONG)(dwlSize - cbDone), (DWORDLONG)cbRead);
        else
            dwSizeToWrite = cbRead;

        if (dwSizeToWrite == 0) // No need to write?
        {
            hr = S_OK;
            break;
        }

        hr = pDst->Write(pBuff, dwSizeToWrite, NULL);
        if (hr != S_OK)
            break;

        cbDone += dwSizeToWrite;

        if (pProgress->HasUserCancelled()) // Cancelled?
        {
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            break;
        }
        pProgress->SetProgress64(cbDone, dwlSize);

        if (dwlSize > 0 && cbDone >= dwlSize) // Reached the goal?
        {
            hr = S_OK;
            break;
        }
    }

    return hr;
}

/*************************************************************************
 *  Activate_RunDLL [SHELL32.105]
 *
 * Unlocks the foreground window and allows the shell window to become the
 * foreground window. Every parameter is unused.
 */
EXTERN_C
BOOL WINAPI
Activate_RunDLL(
    _In_ HWND hwnd,
    _In_ HINSTANCE hinst,
    _In_ LPCWSTR cmdline,
    _In_ INT cmdshow)
{
    DWORD dwProcessID;

    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(hinst);
    UNREFERENCED_PARAMETER(cmdline);
    UNREFERENCED_PARAMETER(cmdshow);

    TRACE("(%p, %p, %s, %d)\n", hwnd, hinst, debugstr_w(cmdline), cmdline);

    GetWindowThreadProcessId(GetShellWindow(), &dwProcessID);
    return AllowSetForegroundWindow(dwProcessID);
}

/*************************************************************************
 *                SHStartNetConnectionDialogA (SHELL32.12)
 *
 * @see https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shstartnetconnectiondialoga
 */
EXTERN_C
HRESULT WINAPI
SHStartNetConnectionDialogA(
    _In_ HWND hwnd,
    _In_ LPCSTR pszRemoteName,
    _In_ DWORD dwType)
{
    LPCWSTR pszRemoteNameW = NULL;
    CStringW strRemoteNameW;

    TRACE("(%p, %s, %lu)\n", hwnd, debugstr_a(pszRemoteName), dwType);

    if (pszRemoteName)
    {
        strRemoteNameW = pszRemoteName;
        pszRemoteNameW = strRemoteNameW;
    }

    return SHStartNetConnectionDialogW(hwnd, pszRemoteNameW, dwType);
}

/*************************************************************************
 * Helper functions for PathIsEqualOrSubFolder
 */

static INT
DynamicPathCommonPrefixW(
    _In_ LPCWSTR lpszPath1,
    _In_ LPCWSTR lpszPath2,
    _Out_ CStringW& strPath)
{
    SIZE_T cchPath1 = wcslen(lpszPath1);
    SIZE_T cchPath2 = wcslen(lpszPath2);
    LPWSTR lpszPath = strPath.GetBuffer((INT)max(cchPath1, cchPath2) + 16);
    INT ret = PathCommonPrefixW(lpszPath1, lpszPath2, lpszPath);
    strPath.ReleaseBuffer();
    return ret;
}

EXTERN_C HRESULT WINAPI
SHGetPathCchFromIDListW(LPCITEMIDLIST pidl, LPWSTR pszPath, SIZE_T cchPathMax);

static HRESULT
DynamicSHGetPathFromIDListW(
    _In_ LPCITEMIDLIST pidl,
    _Out_ CStringW& strPath)
{
    HRESULT hr;

    for (UINT cchPath = MAX_PATH;; cchPath *= 2)
    {
        LPWSTR lpszPath = strPath.GetBuffer(cchPath);
        if (!lpszPath)
            return E_OUTOFMEMORY;

        hr = SHGetPathCchFromIDListW(pidl, lpszPath, cchPath);
        strPath.ReleaseBuffer();

        if (hr != E_NOT_SUFFICIENT_BUFFER)
            break;

        if (cchPath >= MAXUINT / 2)
        {
            hr = E_FAIL;
            break;
        }
    }

    if (FAILED(hr))
        strPath.Empty();

    return hr;
}

static HRESULT
DynamicSHGetSpecialFolderPathW(
    _In_ HWND hwndOwner,
    _Out_ CStringW& strPath,
    _In_ INT nCSIDL,
    _In_ BOOL bCreate)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetSpecialFolderLocation(hwndOwner, nCSIDL, &pidl);
    if (SUCCEEDED(hr))
    {
        hr = DynamicSHGetPathFromIDListW(pidl, strPath);
        CoTaskMemFree(pidl);
    }

    if (FAILED(hr))
        strPath.Empty();
    else if (bCreate)
        CreateDirectoryW(strPath, NULL);

    return hr;
}

static VOID
DynamicPathRemoveBackslashW(
    _Out_ CStringW& strPath)
{
    INT nLength = strPath.GetLength();
    if (nLength > 0 && strPath[nLength - 1] == L'\\')
        strPath = strPath.Left(nLength - 1);
}

/*************************************************************************
 *                PathIsEqualOrSubFolder (SHELL32.755)
 */
EXTERN_C
BOOL WINAPI
PathIsEqualOrSubFolder(
    _In_ LPCWSTR pszPath1OrCSIDL,
    _In_ LPCWSTR pszPath2)
{
    CStringW strCommon, strPath1;

    TRACE("(%s %s)\n", debugstr_w(pszPath1OrCSIDL), debugstr_w(pszPath2));

    if (IS_INTRESOURCE(pszPath1OrCSIDL))
    {
        DynamicSHGetSpecialFolderPathW(
            NULL, strPath1, LOWORD(pszPath1OrCSIDL) | CSIDL_FLAG_DONT_VERIFY, FALSE);
    }
    else
    {
        strPath1 = pszPath1OrCSIDL;
    }

    DynamicPathRemoveBackslashW(strPath1);

    if (!DynamicPathCommonPrefixW(strPath1, pszPath2, strCommon))
        return FALSE;

    return strPath1.CompareNoCase(strCommon) == 0;
}

/*************************************************************************
 *  SHGetRealIDL [SHELL32.98]
 */
EXTERN_C
HRESULT WINAPI
SHGetRealIDL(
    _In_ IShellFolder *psf,
    _In_ PCUITEMID_CHILD pidlSimple,
    _Outptr_ PITEMID_CHILD *ppidlReal)
{
    HRESULT hr;
    STRRET strret;
    WCHAR szPath[MAX_PATH];
    SFGAOF attrs;

    *ppidlReal = NULL;

    hr = IShellFolder_GetDisplayNameOf(psf, pidlSimple, SHGDN_INFOLDER | SHGDN_FORPARSING,
                                       &strret, 0);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = StrRetToBufW(&strret, pidlSimple, szPath, _countof(szPath));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    attrs = SFGAO_FILESYSTEM;
    hr = psf->GetAttributesOf(1, &pidlSimple, &attrs);
    if (SUCCEEDED(hr) && !(attrs & SFGAO_FILESYSTEM))
        return SHILClone(pidlSimple, ppidlReal);

    hr = IShellFolder_ParseDisplayName(psf, NULL, NULL, szPath, NULL, ppidlReal, NULL);
    if (hr == E_INVALIDARG || hr == E_NOTIMPL)
        return SHILClone(pidlSimple, ppidlReal);

    return hr;
}

EXTERN_C HRESULT
IUnknown_InitializeCommand(
    _In_ IUnknown *pUnk,
    _In_ PCWSTR pszCommandName,
    _In_opt_ IPropertyBag *pPB)
{
    HRESULT hr;
    CComPtr<IInitializeCommand> pIC;
    if (SUCCEEDED(hr = pUnk->QueryInterface(IID_PPV_ARG(IInitializeCommand, &pIC))))
        hr = pIC->Initialize(pszCommandName, pPB);
    return hr;
}

EXTERN_C HRESULT
InvokeIExecuteCommand(
    _In_ IExecuteCommand *pEC,
    _In_ PCWSTR pszCommandName,
    _In_opt_ IPropertyBag *pPB,
    _In_opt_ IShellItemArray *pSIA,
    _In_opt_ LPCMINVOKECOMMANDINFOEX pICI,
    _In_opt_ IUnknown *pSite)
{
    if (!pEC)
        return E_INVALIDARG;

    if (pSite)
        IUnknown_SetSite(pEC, pSite);
    IUnknown_InitializeCommand(pEC, pszCommandName, pPB);

    CComPtr<IObjectWithSelection> pOWS;
    if (pSIA && SUCCEEDED(pEC->QueryInterface(IID_PPV_ARG(IObjectWithSelection, &pOWS))))
        pOWS->SetSelection(pSIA);

    DWORD dwKeyState = 0, fMask = pICI ? pICI->fMask : 0;
    pEC->SetNoShowUI((fMask & CMIC_MASK_FLAG_NO_UI) != 0);
    pEC->SetShowWindow(pICI ? pICI->nShow : SW_SHOW);
    if (fMask & CMIC_MASK_SHIFT_DOWN)
        dwKeyState |= MK_SHIFT;
    if (fMask & CMIC_MASK_CONTROL_DOWN)
        dwKeyState |= MK_CONTROL;
    pEC->SetKeyState(dwKeyState);
    if ((fMask & CMIC_MASK_UNICODE) && pICI->lpDirectoryW)
        pEC->SetDirectory(pICI->lpDirectoryW);
    if ((fMask & CMIC_MASK_UNICODE) && pICI->lpParametersW)
        pEC->SetParameters(pICI->lpParametersW);
    if (fMask & CMIC_MASK_PTINVOKE)
        pEC->SetPosition(pICI->ptInvoke);

    HRESULT hr = pEC->Execute();
    if (pSite)
        IUnknown_SetSite(pEC, NULL);
    return hr;
}

EXTERN_C HRESULT
InvokeIExecuteCommandWithDataObject(
    _In_ IExecuteCommand *pEC,
    _In_ PCWSTR pszCommandName,
    _In_opt_ IPropertyBag *pPB,
    _In_ IDataObject *pDO,
    _In_opt_ LPCMINVOKECOMMANDINFOEX pICI,
    _In_opt_ IUnknown *pSite)
{
    CComPtr<IShellItemArray> pSIA;
    HRESULT hr = SHCreateShellItemArrayFromDataObject(pDO, IID_PPV_ARG(IShellItemArray, &pSIA));
    return SUCCEEDED(hr) ? InvokeIExecuteCommand(pEC, pszCommandName, pPB, pSIA, pICI, pSite) : hr;
}

static HRESULT
GetCommandStringA(_In_ IContextMenu *pCM, _In_ UINT_PTR Id, _In_ UINT GCS, _Out_writes_(cchMax) LPSTR Buf, _In_ UINT cchMax)
{
    HRESULT hr = pCM->GetCommandString(Id, GCS & ~GCS_UNICODE, NULL, Buf, cchMax);
    if (FAILED(hr))
    {
        WCHAR buf[MAX_PATH];
        hr = pCM->GetCommandString(Id, GCS | GCS_UNICODE, NULL, (LPSTR)buf, _countof(buf));
        if (SUCCEEDED(hr))
            hr = SHUnicodeToAnsi(buf, Buf, cchMax) > 0 ? S_OK : E_FAIL;
    }
    return hr;
}

UINT
GetDfmCmd(_In_ IContextMenu *pCM, _In_ LPCSTR verba)
{
    CHAR buf[MAX_PATH];
    if (IS_INTRESOURCE(verba))
    {
        if (FAILED(GetCommandStringA(pCM, LOWORD(verba), GCS_VERB, buf, _countof(buf))))
            return 0;
        verba = buf;
    }
    return MapVerbToDfmCmd(verba); // Returns DFM_CMD_* or 0
}

HRESULT
SHELL_MapContextMenuVerbToCmdId(LPCMINVOKECOMMANDINFO pICI, const CMVERBMAP *pMap)
{
    LPCSTR pVerbA = pICI->lpVerb;
    CHAR buf[MAX_PATH];
    LPCMINVOKECOMMANDINFOEX pICIX = (LPCMINVOKECOMMANDINFOEX)pICI;
    if (IsUnicode(*pICIX) && !IS_INTRESOURCE(pICIX->lpVerbW))
    {
        if (SHUnicodeToAnsi(pICIX->lpVerbW, buf, _countof(buf)))
            pVerbA = buf;
    }

    if (IS_INTRESOURCE(pVerbA))
        return LOWORD(pVerbA);
    for (SIZE_T i = 0; pMap[i].Verb; ++i)
    {
        assert(SUCCEEDED((int)(pMap[i].CmdId))); // The id must be >= 0 and ideally in the 0..0x7fff range
        if (!lstrcmpiA(pMap[i].Verb, pVerbA) && pVerbA[0])
            return pMap[i].CmdId;
    }
    return E_FAIL;
}

static const CMVERBMAP*
FindVerbMapEntry(UINT_PTR CmdId, const CMVERBMAP *pMap)
{
    for (SIZE_T i = 0; pMap[i].Verb; ++i)
    {
        if (pMap[i].CmdId == CmdId)
            return &pMap[i];
    }
    return NULL;
}

HRESULT
SHELL_GetCommandStringImpl(SIZE_T CmdId, UINT uFlags, LPSTR Buf, UINT cchBuf, const CMVERBMAP *pMap)
{
    const CMVERBMAP* pEntry;
    switch (uFlags | GCS_UNICODE)
    {
        case GCS_VALIDATEW:
        case GCS_VERBW:
            pEntry = FindVerbMapEntry(CmdId, pMap);
            if ((uFlags | GCS_UNICODE) == GCS_VERBW)
            {
                if (!pEntry)
                    return E_INVALIDARG;
                else if (uFlags & GCS_UNICODE)
                    return SHAnsiToUnicode(pEntry->Verb, (LPWSTR)Buf, cchBuf) ? S_OK : E_FAIL;
                else
                    return StringCchCopyA(Buf, cchBuf, pEntry->Verb);
            }
            return pEntry ? S_OK : S_FALSE; // GCS_VALIDATE
    }
    return E_NOTIMPL;
}

HRESULT
SHELL_CreateShell32DefaultExtractIcon(int IconIndex, REFIID riid, LPVOID *ppvOut)
{
    CComPtr<IDefaultExtractIconInit> initIcon;
    HRESULT hr = SHCreateDefaultExtractIcon(IID_PPV_ARG(IDefaultExtractIconInit, &initIcon));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    initIcon->SetNormalIcon(swShell32Name, IconIndex);
    return initIcon->QueryInterface(riid, ppvOut);
}

/*************************************************************************
 *  SHIsBadInterfacePtr [SHELL32.84]
 *
 * Retired in 6.0 from Windows Vista and higher.
 */
EXTERN_C
BOOL WINAPI
SHIsBadInterfacePtr(
    _In_ LPCVOID pv,
    _In_ UINT_PTR ucb)
{
    struct CUnknownVtbl
    {
        HRESULT (STDMETHODCALLTYPE *QueryInterface)(REFIID riid, LPVOID *ppvObj);
        ULONG (STDMETHODCALLTYPE *AddRef)();
        ULONG (STDMETHODCALLTYPE *Release)();
    };
    struct CUnknown { CUnknownVtbl *lpVtbl; };
    const CUnknown *punk = reinterpret_cast<const CUnknown *>(pv);
    return !punk || IsBadReadPtr(punk, sizeof(punk->lpVtbl)) ||
           IsBadReadPtr(punk->lpVtbl, ucb) ||
           IsBadCodePtr((FARPROC)punk->lpVtbl->Release);
}

/*************************************************************************
 *  SHGetUserDisplayName [SHELL32.241]
 *
 * @see https://undoc.airesoft.co.uk/shell32.dll/SHGetUserDisplayName.php
 */
EXTERN_C
HRESULT WINAPI
SHGetUserDisplayName(
    _Out_writes_to_(*puSize, *puSize) PWSTR pName,
    _Inout_ PULONG puSize)
{
    if (!pName || !puSize)
        return E_INVALIDARG;

    if (GetUserNameExW(NameDisplay, pName, puSize))
        return S_OK;

    LONG error = GetLastError(); // for ERROR_NONE_MAPPED
    HRESULT hr = HRESULT_FROM_WIN32(error);

    WCHAR UserName[MAX_PATH];
    DWORD cchUserName = _countof(UserName);
    if (!GetUserNameW(UserName, &cchUserName))
        return HRESULT_FROM_WIN32(GetLastError());

    // Was the user name not available in the specified format (NameDisplay)?
    if (error == ERROR_NONE_MAPPED)
    {
        // Try to get the user name by using Network API
        PUSER_INFO_2 UserInfo;
        DWORD NetError = NetUserGetInfo(NULL, UserName, 2, (PBYTE*)&UserInfo);
        if (NetError)
        {
            hr = HRESULT_FROM_WIN32(NetError);
        }
        else
        {
            if (UserInfo->usri2_full_name)
            {
                hr = StringCchCopyW(pName, *puSize, UserInfo->usri2_full_name);
                if (SUCCEEDED(hr))
                {
                    // Include the NUL-terminator
                    *puSize = lstrlenW(UserInfo->usri2_full_name) + 1;
                }
            }

            NetApiBufferFree(UserInfo);
        }
    }

    if (FAILED(hr))
    {
        hr = StringCchCopyW(pName, *puSize, UserName);
        if (SUCCEEDED(hr))
            *puSize = cchUserName;
    }

    return hr;
}

// Skip leading backslashes
static PCWSTR
SHELL_SkipServerSlashes(
    _In_ PCWSTR pszPath)
{
    PCWSTR pch;
    for (pch = pszPath; *pch == L'\\'; ++pch)
        ;
    return pch;
}

// The registry key for server computer descriptions cache
#define COMPUTER_DESCRIPTIONS_KEY \
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComputerDescriptions"

// Get server computer description from cache
static HRESULT
SHELL_GetCachedComputerDescription(
    _Out_writes_z_(cchDescMax) PWSTR pszDesc,
    _In_ DWORD cchDescMax,
    _In_ PCWSTR pszServerName)
{
    cchDescMax *= sizeof(WCHAR);
    DWORD error = SHGetValueW(HKEY_CURRENT_USER, COMPUTER_DESCRIPTIONS_KEY,
                              SHELL_SkipServerSlashes(pszServerName), NULL, pszDesc, &cchDescMax);
    return HRESULT_FROM_WIN32(error);
}

// Do cache a server computer description
static VOID
SHELL_CacheComputerDescription(
    _In_ PCWSTR pszServerName,
    _In_ PCWSTR pszDesc)
{
    if (!pszDesc)
        return;

    SIZE_T cbDesc = (wcslen(pszDesc) + 1) * sizeof(WCHAR);
    SHSetValueW(HKEY_CURRENT_USER, COMPUTER_DESCRIPTIONS_KEY,
                SHELL_SkipServerSlashes(pszServerName), REG_SZ, pszDesc, (DWORD)cbDesc);
}

// Get real server computer description
static HRESULT
SHELL_GetComputerDescription(
    _Out_writes_z_(cchDescMax) PWSTR pszDesc,
    _In_ SIZE_T cchDescMax,
    _In_ PWSTR pszServerName)
{
    PSERVER_INFO_101 bufptr;
    NET_API_STATUS error = NetServerGetInfo(pszServerName, 101, (PBYTE*)&bufptr);
    HRESULT hr = (error > 0) ? HRESULT_FROM_WIN32(error) : error;
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    PCWSTR comment = bufptr->sv101_comment;
    if (comment && comment[0])
        StringCchCopyW(pszDesc, cchDescMax, comment);
    else
        hr = E_FAIL;

    NetApiBufferFree(bufptr);
    return hr;
}

// Build computer display name
static HRESULT
SHELL_BuildDisplayMachineName(
    _Out_writes_z_(cchNameMax) PWSTR pszName,
    _In_ DWORD cchNameMax,
    _In_ PCWSTR pszServerName,
    _In_ PCWSTR pszDescription)
{
    if (!pszDescription || !*pszDescription)
        return E_FAIL;

    PCWSTR pszFormat = (SHRestricted(REST_ALLOWCOMMENTTOGGLE) ? L"%2 (%1)" : L"%1 (%2)");
    PCWSTR args[] = { pszDescription , SHELL_SkipServerSlashes(pszServerName) };
    return (FormatMessageW(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING,
                           pszFormat, 0, 0, pszName, cchNameMax, (va_list *)args) ? S_OK : E_FAIL);
}

/*************************************************************************
 *  SHGetComputerDisplayNameW [SHELL32.752]
 */
EXTERN_C
HRESULT WINAPI
SHGetComputerDisplayNameW(
    _In_opt_ PWSTR pszServerName,
    _In_ DWORD dwFlags,
    _Out_writes_z_(cchNameMax) PWSTR pszName,
    _In_ DWORD cchNameMax)
{
    WCHAR szDesc[256], szCompName[MAX_COMPUTERNAME_LENGTH + 1];

    // If no server name is specified, retrieve the local computer name
    if (!pszServerName)
    {
        // Use computer name as server name
        DWORD cchCompName = _countof(szCompName);
        if (!GetComputerNameW(szCompName, &cchCompName))
            return E_FAIL;
        pszServerName = szCompName;

        // Don't use the cache for the local machine
        dwFlags |= SHGCDN_NOCACHE;
    }

    // Get computer description from cache if necessary
    HRESULT hr = E_FAIL;
    if (!(dwFlags & SHGCDN_NOCACHE))
        hr = SHELL_GetCachedComputerDescription(szDesc, _countof(szDesc), pszServerName);

    // Actually retrieve the computer description if it is not in the cache
    if (FAILED(hr))
    {
        hr = SHELL_GetComputerDescription(szDesc, _countof(szDesc), pszServerName);
        if (FAILED(hr))
            szDesc[0] = UNICODE_NULL;

        // Cache the description if necessary
        if (!(dwFlags & SHGCDN_NOCACHE))
            SHELL_CacheComputerDescription(pszServerName, szDesc);
    }

    // If getting the computer description failed, store the server name only
    if (FAILED(hr) || !szDesc[0])
    {
        if (dwFlags & SHGCDN_NOSERVERNAME)
            return hr; // Bail out if no server name is requested

        StringCchCopyW(pszName, cchNameMax, SHELL_SkipServerSlashes(pszServerName));
        return S_OK;
    }

    // If no server name is requested, store the description only
    if (dwFlags & SHGCDN_NOSERVERNAME)
    {
        StringCchCopyW(pszName, cchNameMax, szDesc);
        return S_OK;
    }

    // Build a string like "Description (SERVERNAME)"
    return SHELL_BuildDisplayMachineName(pszName, cchNameMax, pszServerName, szDesc);
}
