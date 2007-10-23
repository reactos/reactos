/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Registry namespace extension
 * FILE:            dll/win32/shell32/extracticon.c
 * PURPOSE:         Icon extraction
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#define COBJMACROS
#define CONST_VTABLE
#include <shlobj.h>
#include <debug.h>

struct IconLocation
{
    LPWSTR file;
    UINT index;
};

struct IconExtraction
{
    ULONG ref;
    IDefaultExtractIconInit defaultExtractIconInitImpl;
    IExtractIconW extractIconWImpl;
    IExtractIconA extractIconAImpl;
    IPersistFile persistFileImpl;

    UINT flags;
    struct IconLocation defaultIcon;
    struct IconLocation normalIcon;
    struct IconLocation openIcon;
    struct IconLocation shortcutIcon;
};

static VOID
DuplicateString(
    LPCWSTR Source,
    LPWSTR *Destination)
{
    SIZE_T cb;

    if (*Destination)
        CoTaskMemFree(*Destination);

    cb = (wcslen(Source) + 1) * sizeof(WCHAR);
    *Destination = CoTaskMemAlloc(cb);
    if (!*Destination)
        return;
    CopyMemory(*Destination, Source, cb);
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_DefaultExtractIconInit_QueryInterface(
    IDefaultExtractIconInit *This,
    REFIID riid,
    void **ppvObject)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, defaultExtractIconInitImpl);

    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppvObject = &s->defaultExtractIconInitImpl;
    else if (IsEqualIID(riid, &IID_IDefaultExtractIconInit))
        *ppvObject = &s->defaultExtractIconInitImpl;
    else if (IsEqualIID(riid, &IID_IExtractIconW))
        *ppvObject = &s->extractIconWImpl;
    else if (IsEqualIID(riid, &IID_IExtractIconA))
        *ppvObject = &s->extractIconAImpl;
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppvObject = &s->persistFileImpl;
    else if (IsEqualIID(riid, &IID_IPersistFile))
        *ppvObject = &s->persistFileImpl;
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    
    IUnknown_AddRef(This);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE
IconExtraction_DefaultExtractIconInit_AddRef(
    IDefaultExtractIconInit *This)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, defaultExtractIconInitImpl);
    ULONG refCount = InterlockedIncrement((PLONG)&s->ref);
    return refCount;
}

static ULONG STDMETHODCALLTYPE
IconExtraction_DefaultExtractIconInit_Release(
    IDefaultExtractIconInit *This)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, defaultExtractIconInitImpl);
    ULONG refCount;
    
    if (!This)
        return E_POINTER;

    refCount = InterlockedDecrement((PLONG)&s->ref);
    if (refCount == 0)
    {
        if (s->defaultIcon.file) CoTaskMemFree(s->defaultIcon.file);
        if (s->normalIcon.file) CoTaskMemFree(s->normalIcon.file);
        if (s->openIcon.file) CoTaskMemFree(s->openIcon.file);
        if (s->shortcutIcon.file) CoTaskMemFree(s->shortcutIcon.file);
        CoTaskMemFree(s);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_DefaultExtractIconInit_SetDefaultIcon(
    IDefaultExtractIconInit *This,
    LPCWSTR pszFile,
    int iIcon)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, defaultExtractIconInitImpl);
    DuplicateString(pszFile, &s->defaultIcon.file);
    if (!s->defaultIcon.file)
        return E_OUTOFMEMORY;
    s->defaultIcon.index = iIcon;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_DefaultExtractIconInit_SetFlags(
    IDefaultExtractIconInit *This,
    UINT uFlags)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, defaultExtractIconInitImpl);
    s->flags = uFlags;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_DefaultExtractIconInit_SetKey(
    IDefaultExtractIconInit *This,
    HKEY hkey)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_DefaultExtractIconInit_SetNormalIcon(
    IDefaultExtractIconInit *This,
    LPCWSTR pszFile,
    int iIcon)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, defaultExtractIconInitImpl);
    DuplicateString(pszFile, &s->normalIcon.file);
    if (!s->normalIcon.file)
        return E_OUTOFMEMORY;
    s->normalIcon.index = iIcon;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_DefaultExtractIconInit_SetOpenIcon(
    IDefaultExtractIconInit *This,
    LPCWSTR pszFile,
    int iIcon)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, defaultExtractIconInitImpl);
    DuplicateString(pszFile, &s->openIcon.file);
    if (!s->openIcon.file)
        return E_OUTOFMEMORY;
    s->openIcon.index = iIcon;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_DefaultExtractIconInit_SetShortcutIcon(
    IDefaultExtractIconInit *This,
    LPCWSTR pszFile,
    int iIcon)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, defaultExtractIconInitImpl);
    DuplicateString(pszFile, &s->shortcutIcon.file);
    if (!s->shortcutIcon.file)
        return E_OUTOFMEMORY;
    s->shortcutIcon.index = iIcon;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_ExtractIconW_QueryInterface(
    IExtractIconW *This,
    REFIID riid,
    void **ppvObject)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, extractIconWImpl);
    return IconExtraction_DefaultExtractIconInit_QueryInterface(&s->defaultExtractIconInitImpl, riid, ppvObject);
}

static ULONG STDMETHODCALLTYPE
IconExtraction_ExtractIconW_AddRef(
    IExtractIconW *This)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, extractIconWImpl);
    return IconExtraction_DefaultExtractIconInit_AddRef(&s->defaultExtractIconInitImpl);
}

static ULONG STDMETHODCALLTYPE
IconExtraction_ExtractIconW_Release(
    IExtractIconW *This)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, extractIconWImpl);
    return IconExtraction_DefaultExtractIconInit_Release(&s->defaultExtractIconInitImpl);
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_ExtractIconW_GetIconLocation(
    IExtractIconW *This,
    UINT uFlags,
    LPWSTR szIconFile,
    UINT cchMax,
    int *piIndex,
    UINT *pwFlags)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, extractIconWImpl);
    const struct IconLocation *icon = NULL;
    SIZE_T cb;

    if (uFlags & GIL_DEFAULTICON)
        icon = s->defaultIcon.file ? &s->defaultIcon : &s->normalIcon;
    else if (uFlags & GIL_FORSHORTCUT)
        icon = s->shortcutIcon.file ? &s->shortcutIcon : &s->normalIcon;
    else if (uFlags & GIL_OPENICON)
        icon = s->openIcon.file ? &s->openIcon : &s->normalIcon;
    else
        icon = &s->normalIcon;

    if (!icon->file)
        return E_FAIL;

    cb = wcslen(icon->file) + 1;
    if (cchMax < (UINT)cb)
        return E_FAIL;
    CopyMemory(szIconFile, icon->file, cb * sizeof(WCHAR));
    *piIndex = icon->index;
    *pwFlags = s->flags;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_ExtractIconW_Extract(
    IExtractIconW *This,
    LPCWSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize)
{
    /* Nothing to do, ExtractIconW::GetIconLocation should be enough */
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_ExtractIconA_QueryInterface(
    IExtractIconA *This,
    REFIID riid,
    void **ppvObject)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, extractIconAImpl);
    return IconExtraction_DefaultExtractIconInit_QueryInterface(&s->defaultExtractIconInitImpl, riid, ppvObject);
}

static ULONG STDMETHODCALLTYPE
IconExtraction_ExtractIconA_AddRef(
    IExtractIconA *This)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, extractIconAImpl);
    return IconExtraction_DefaultExtractIconInit_AddRef(&s->defaultExtractIconInitImpl);
}

static ULONG STDMETHODCALLTYPE
IconExtraction_ExtractIconA_Release(
    IExtractIconA *This)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, extractIconAImpl);
    return IconExtraction_DefaultExtractIconInit_Release(&s->defaultExtractIconInitImpl);
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_ExtractIconA_GetIconLocation(
    IExtractIconA *This,
    UINT uFlags,
    LPSTR szIconFile,
    UINT cchMax,
    int *piIndex,
    UINT *pwFlags)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, extractIconAImpl);
    LPWSTR szIconFileW = NULL;
    HRESULT hr;

    if (cchMax > 0)
    {
        szIconFileW = CoTaskMemAlloc(cchMax * sizeof(WCHAR));
        if (!szIconFileW)
            return E_OUTOFMEMORY;
    }
    
    hr = IconExtraction_ExtractIconW_GetIconLocation(
        &s->extractIconWImpl, uFlags, szIconFileW, cchMax, piIndex, pwFlags);
    if (SUCCEEDED(hr) && cchMax > 0)
        if (0 == WideCharToMultiByte(CP_ACP, 0, szIconFileW, cchMax, szIconFile, cchMax, NULL, NULL))
            hr = E_FAIL;

    if (szIconFileW)
        CoTaskMemFree(szIconFileW);
    return hr;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_ExtractIconA_Extract(
    IExtractIconA *This,
    LPCSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, extractIconAImpl);
    LPWSTR pszFileW = NULL;
    int nLength;
    HRESULT hr;

    if (pszFile)
    {
        nLength = MultiByteToWideChar(CP_ACP, 0, pszFile, -1, NULL, 0);
        if (nLength == 0)
            return E_FAIL;
        pszFileW = CoTaskMemAlloc(nLength * sizeof(WCHAR));
        if (!pszFileW)
            return E_OUTOFMEMORY;
        if (!MultiByteToWideChar(CP_ACP, 0, pszFile, nLength, pszFileW, nLength))
        {
            CoTaskMemFree(pszFileW);
            return E_FAIL;
        }
    }
    
    hr = IconExtraction_ExtractIconW_Extract(
        &s->extractIconWImpl, pszFileW, nIconIndex, phiconLarge, phiconSmall, nIconSize);

    if (pszFileW)
        CoTaskMemFree(pszFileW);
    return hr;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_PersistFile_QueryInterface(
    IPersistFile *This,
    REFIID riid,
    void **ppvObject)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, persistFileImpl);
    return IconExtraction_DefaultExtractIconInit_QueryInterface(&s->defaultExtractIconInitImpl, riid, ppvObject);
}

static ULONG STDMETHODCALLTYPE
IconExtraction_PersistFile_AddRef(
    IPersistFile *This)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, persistFileImpl);
    return IconExtraction_DefaultExtractIconInit_AddRef(&s->defaultExtractIconInitImpl);
}

static ULONG STDMETHODCALLTYPE
IconExtraction_PersistFile_Release(
    IPersistFile *This)
{
    struct IconExtraction *s = CONTAINING_RECORD(This, struct IconExtraction, persistFileImpl);
    return IconExtraction_DefaultExtractIconInit_Release(&s->defaultExtractIconInitImpl);
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_PersistFile_GetClassID(
    IPersistFile *This,
    CLSID *pClassID)
{
    if (!pClassID)
        return E_POINTER;

    *pClassID = GUID_NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_PersistFile_IsDirty(
    IPersistFile *This)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_PersistFile_Load(
    IPersistFile *This,
    LPCOLESTR pszFileName,
    DWORD dwMode)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_PersistFile_Save(
    IPersistFile *This,
    LPCOLESTR pszFileName,
    BOOL fRemember)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_PersistFile_SaveCompleted(
    IPersistFile *This,
    LPCOLESTR pszFileName)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IconExtraction_PersistFile_GetCurFile(
    IPersistFile *This,
    LPOLESTR *ppszFileName)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

static const IDefaultExtractIconInitVtbl IconExtractionDefaultExtractIconInitVtbl =
{
    IconExtraction_DefaultExtractIconInit_QueryInterface,
    IconExtraction_DefaultExtractIconInit_AddRef,
    IconExtraction_DefaultExtractIconInit_Release,
    IconExtraction_DefaultExtractIconInit_SetDefaultIcon,
    IconExtraction_DefaultExtractIconInit_SetFlags,
    IconExtraction_DefaultExtractIconInit_SetKey,
    IconExtraction_DefaultExtractIconInit_SetNormalIcon,
    IconExtraction_DefaultExtractIconInit_SetOpenIcon,
    IconExtraction_DefaultExtractIconInit_SetShortcutIcon,
};

static const IExtractIconWVtbl IconExtractionExtractIconWVtbl =
{
    IconExtraction_ExtractIconW_QueryInterface,
    IconExtraction_ExtractIconW_AddRef,
    IconExtraction_ExtractIconW_Release,
    IconExtraction_ExtractIconW_GetIconLocation,
    IconExtraction_ExtractIconW_Extract,
};

static const IExtractIconAVtbl IconExtractionExtractIconAVtbl =
{
    IconExtraction_ExtractIconA_QueryInterface,
    IconExtraction_ExtractIconA_AddRef,
    IconExtraction_ExtractIconA_Release,
    IconExtraction_ExtractIconA_GetIconLocation,
    IconExtraction_ExtractIconA_Extract,
};

static const IPersistFileVtbl IconExtractionPersistFileVtbl =
{
    IconExtraction_PersistFile_QueryInterface,
    IconExtraction_PersistFile_AddRef,
    IconExtraction_PersistFile_Release,
    IconExtraction_PersistFile_GetClassID,
    IconExtraction_PersistFile_IsDirty,
    IconExtraction_PersistFile_Load,
    IconExtraction_PersistFile_Save,
    IconExtraction_PersistFile_SaveCompleted,
    IconExtraction_PersistFile_GetCurFile,
};

HRESULT WINAPI
SHCreateDefaultExtractIcon(
    REFIID riid,
    void **ppv)
{
    struct IconExtraction *s;

    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    s = CoTaskMemAlloc(sizeof(struct IconExtraction));
    if (!s)
        return E_OUTOFMEMORY;
    memset(s, 0, sizeof(struct IconExtraction));
    s->defaultExtractIconInitImpl.lpVtbl = &IconExtractionDefaultExtractIconInitVtbl;
    s->extractIconAImpl.lpVtbl = &IconExtractionExtractIconAVtbl;
    s->extractIconWImpl.lpVtbl = &IconExtractionExtractIconWVtbl;
    s->persistFileImpl.lpVtbl = &IconExtractionPersistFileVtbl;
    s->ref = 1;
    *ppv = &s->defaultExtractIconInitImpl;

    return S_OK;
}
