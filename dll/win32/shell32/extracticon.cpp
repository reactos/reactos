/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Registry namespace extension
 * FILE:            dll/win32/shell32/extracticon.c
 * PURPOSE:         Icon extraction
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

struct IconLocation
{
    LPWSTR file;
    UINT index;
};

class IconExtraction :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDefaultExtractIconInit,
    public IExtractIconW,
    public IExtractIconA,
    public IPersistFile
{
private:
    UINT flags;
    struct IconLocation defaultIcon;
    struct IconLocation normalIcon;
    struct IconLocation openIcon;
    struct IconLocation shortcutIcon;
public:
    IconExtraction();
    ~IconExtraction();

    // IDefaultExtractIconInit
    virtual HRESULT STDMETHODCALLTYPE SetDefaultIcon(LPCWSTR pszFile, int iIcon);
    virtual HRESULT STDMETHODCALLTYPE SetFlags(UINT uFlags);
    virtual HRESULT STDMETHODCALLTYPE SetKey(HKEY hkey);
    virtual HRESULT STDMETHODCALLTYPE SetNormalIcon(LPCWSTR pszFile, int iIcon);
    virtual HRESULT STDMETHODCALLTYPE SetOpenIcon(LPCWSTR pszFile, int iIcon);
    virtual HRESULT STDMETHODCALLTYPE SetShortcutIcon(LPCWSTR pszFile, int iIcon);

    // IExtractIconW
    virtual HRESULT STDMETHODCALLTYPE GetIconLocation(UINT uFlags, LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
    virtual HRESULT STDMETHODCALLTYPE Extract(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

    // IExtractIconA
    virtual HRESULT STDMETHODCALLTYPE GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
    virtual HRESULT STDMETHODCALLTYPE Extract(LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

    // IPersist
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);
    virtual HRESULT STDMETHODCALLTYPE IsDirty();

    // IPersistFile
    virtual HRESULT STDMETHODCALLTYPE Load(LPCOLESTR pszFileName, DWORD dwMode);
    virtual HRESULT STDMETHODCALLTYPE Save(LPCOLESTR pszFileName, BOOL fRemember);
    virtual HRESULT STDMETHODCALLTYPE SaveCompleted(LPCOLESTR pszFileName);
    virtual HRESULT STDMETHODCALLTYPE GetCurFile(LPOLESTR *ppszFileName);

BEGIN_COM_MAP(IconExtraction)
    COM_INTERFACE_ENTRY_IID(IID_IDefaultExtractIconInit, IDefaultExtractIconInit)
    COM_INTERFACE_ENTRY_IID(IID_IExtractIconW, IExtractIconW)
    COM_INTERFACE_ENTRY_IID(IID_IExtractIconA, IExtractIconA)
    COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
    COM_INTERFACE_ENTRY_IID(IID_IPersistFile, IPersistFile)
END_COM_MAP()
};

VOID DuplicateString(
    LPCWSTR Source,
    LPWSTR *Destination)
{
    SIZE_T cb;

    if (*Destination)
        CoTaskMemFree(*Destination);

    cb = (wcslen(Source) + 1) * sizeof(WCHAR);
    *Destination = (LPWSTR)CoTaskMemAlloc(cb);
    if (!*Destination)
        return;
    CopyMemory(*Destination, Source, cb);
}

IconExtraction::IconExtraction()
{
    flags = 0;
    memset(&defaultIcon, 0, sizeof(defaultIcon));
    memset(&normalIcon, 0, sizeof(normalIcon));
    memset(&openIcon, 0, sizeof(openIcon));
    memset(&shortcutIcon, 0, sizeof(shortcutIcon));
}

IconExtraction::~IconExtraction()
{
    if (defaultIcon.file) CoTaskMemFree(defaultIcon.file);
    if (normalIcon.file) CoTaskMemFree(normalIcon.file);
    if (openIcon.file) CoTaskMemFree(openIcon.file);
    if (shortcutIcon.file) CoTaskMemFree(shortcutIcon.file);
}

HRESULT STDMETHODCALLTYPE IconExtraction::SetDefaultIcon(
    LPCWSTR pszFile,
    int iIcon)
{
    TRACE("(%p, %s, %d)\n", this, debugstr_w(pszFile), iIcon);

    DuplicateString(pszFile, &defaultIcon.file);
    if (!defaultIcon.file)
        return E_OUTOFMEMORY;
    defaultIcon.index = iIcon;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IconExtraction::SetFlags(
    UINT uFlags)
{
    TRACE("(%p, 0x%x)\n", this, uFlags);

    flags = uFlags;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IconExtraction::SetKey(
    HKEY hkey)
{
    FIXME("(%p, %p)\n", this, hkey);
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IconExtraction::SetNormalIcon(
    LPCWSTR pszFile,
    int iIcon)
{
    TRACE("(%p, %s, %d)\n", this, debugstr_w(pszFile), iIcon);

    DuplicateString(pszFile, &normalIcon.file);
    if (!normalIcon.file)
        return E_OUTOFMEMORY;
    normalIcon.index = iIcon;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IconExtraction::SetOpenIcon(
    LPCWSTR pszFile,
    int iIcon)
{
    TRACE("(%p, %s, %d)\n", this, debugstr_w(pszFile), iIcon);

    DuplicateString(pszFile, &openIcon.file);
    if (!openIcon.file)
        return E_OUTOFMEMORY;
    openIcon.index = iIcon;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IconExtraction::SetShortcutIcon(
    LPCWSTR pszFile,
    int iIcon)
{
    TRACE("(%p, %s, %d)\n", this, debugstr_w(pszFile), iIcon);

    DuplicateString(pszFile, &shortcutIcon.file);
    if (!shortcutIcon.file)
        return E_OUTOFMEMORY;
    shortcutIcon.index = iIcon;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IconExtraction::GetIconLocation(
    UINT uFlags,
    LPWSTR szIconFile,
    UINT cchMax,
    int *piIndex,
    UINT *pwFlags)
{
    const struct IconLocation *icon = NULL;
    SIZE_T cb;

    TRACE("(%p, 0x%x, %s, 0x%x, %p, %p)\n", this, uFlags, debugstr_w(szIconFile), cchMax, piIndex, pwFlags);

    if (!piIndex || !pwFlags)
        return E_POINTER;

    if (uFlags & GIL_DEFAULTICON)
        icon = defaultIcon.file ? &defaultIcon : &normalIcon;
    else if (uFlags & GIL_FORSHORTCUT)
        icon = shortcutIcon.file ? &shortcutIcon : &normalIcon;
    else if (uFlags & GIL_OPENICON)
        icon = openIcon.file ? &openIcon : &normalIcon;
    else
        icon = &normalIcon;

    if (!icon->file)
        return E_FAIL;

    cb = wcslen(icon->file) + 1;
    if (cchMax < (UINT)cb)
        return E_FAIL;
    CopyMemory(szIconFile, icon->file, cb * sizeof(WCHAR));
    *piIndex = icon->index;
    *pwFlags = flags;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IconExtraction::Extract(
    LPCWSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize)
{
    TRACE("(%p, %s, %u, %p, %p, %u)\n", this, debugstr_w(pszFile), nIconIndex, phiconLarge, phiconSmall, nIconSize);

    /* Nothing to do, ExtractIconW::GetIconLocation should be enough */
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE IconExtraction::GetIconLocation(
    UINT uFlags,
    LPSTR szIconFile,
    UINT cchMax,
    int *piIndex,
    UINT *pwFlags)
{
    LPWSTR szIconFileW = NULL;
    HRESULT hr;

    if (cchMax > 0)
    {
        szIconFileW = (LPWSTR)CoTaskMemAlloc(cchMax * sizeof(WCHAR));
        if (!szIconFileW)
            return E_OUTOFMEMORY;
    }

    hr = GetIconLocation(
        uFlags, szIconFileW, cchMax, piIndex, pwFlags);
    if (SUCCEEDED(hr) && cchMax > 0)
        if (0 == WideCharToMultiByte(CP_ACP, 0, szIconFileW, cchMax, szIconFile, cchMax, NULL, NULL))
            hr = E_FAIL;

    if (szIconFileW)
        CoTaskMemFree(szIconFileW);
    return hr;
}

HRESULT STDMETHODCALLTYPE IconExtraction::Extract(
    LPCSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize)
{
    LPWSTR pszFileW = NULL;
    HRESULT hr;

    if (pszFile)
    {
        int nLength;

        nLength = MultiByteToWideChar(CP_ACP, 0, pszFile, -1, NULL, 0);
        if (nLength == 0)
            return E_FAIL;
        pszFileW = (LPWSTR)CoTaskMemAlloc(nLength * sizeof(WCHAR));
        if (!pszFileW)
            return E_OUTOFMEMORY;
        if (!MultiByteToWideChar(CP_ACP, 0, pszFile, nLength, pszFileW, nLength))
        {
            CoTaskMemFree(pszFileW);
            return E_FAIL;
        }
    }

    hr = Extract(pszFileW, nIconIndex, phiconLarge, phiconSmall, nIconSize);

    if (pszFileW)
        CoTaskMemFree(pszFileW);
    return hr;
}

HRESULT STDMETHODCALLTYPE IconExtraction::GetClassID(
    CLSID *pClassID)
{
    TRACE("(%p, %p)\n", this, pClassID);

    if (!pClassID)
        return E_POINTER;

    *pClassID = GUID_NULL;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IconExtraction::IsDirty()
{
    FIXME("(%p)\n", this);
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IconExtraction::Load(
    LPCOLESTR pszFileName,
    DWORD dwMode)
{
    FIXME("(%p, %s, %u)\n", this, debugstr_w(pszFileName), dwMode);
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IconExtraction::Save(
    LPCOLESTR pszFileName,
    BOOL fRemember)
{
    FIXME("(%p, %s, %d)\n", this, debugstr_w(pszFileName), fRemember);
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IconExtraction::SaveCompleted(
    LPCOLESTR pszFileName)
{
    FIXME("(%p, %s)\n", this, debugstr_w(pszFileName));
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IconExtraction::GetCurFile(
    LPOLESTR *ppszFileName)
{
    FIXME("(%p, %p)\n", this, ppszFileName);
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI SHCreateDefaultExtractIcon(REFIID riid, void **ppv)
{
    CComObject<IconExtraction>                *theExtractor;
    CComPtr<IUnknown>                        result;
    HRESULT                                    hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY (theExtractor = new CComObject<IconExtraction>);
    if (theExtractor == NULL)
        return E_OUTOFMEMORY;
    hResult = theExtractor->QueryInterface (riid, (void **)&result);
    if (FAILED (hResult))
    {
        delete theExtractor;
        return hResult;
    }
    *ppv = result.Detach ();
    return S_OK;
}
