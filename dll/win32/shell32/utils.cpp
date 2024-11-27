/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Utility functions
 * COPYRIGHT:   Copyright 2023-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

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
 *  SHOpenPropSheetA [SHELL32.707]
 *
 * @see https://learn.microsoft.com/en-us/windows/win32/api/shlobj/nf-shlobj-shopenpropsheeta
 */
EXTERN_C
BOOL WINAPI
SHOpenPropSheetA(
    _In_opt_ LPCSTR pszCaption,
    _In_opt_ HKEY *ahKeys,
    _In_ UINT cKeys,
    _In_ const CLSID *pclsidDefault,
    _In_ IDataObject *pDataObject,
    _In_opt_ IShellBrowser *pShellBrowser,
    _In_opt_ LPCSTR pszStartPage)
{
    CStringW strStartPageW, strCaptionW;
    LPCWSTR pszCaptionW = NULL, pszStartPageW = NULL;

    TRACE("(%s, %p, %u, %p, %p, %p, %s)", debugstr_a(pszCaption), ahKeys, cKeys, pclsidDefault,
          pDataObject, pShellBrowser, debugstr_a(pszStartPage));

    if (pszCaption)
    {
        strStartPageW = pszCaption;
        pszCaptionW = strCaptionW;
    }

    if (pszStartPage)
    {
        strStartPageW = pszStartPage;
        pszStartPageW = strStartPageW;
    }

    return SHOpenPropSheetW(pszCaptionW, ahKeys, cKeys, pclsidDefault,
                            pDataObject, pShellBrowser, pszStartPageW);
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
