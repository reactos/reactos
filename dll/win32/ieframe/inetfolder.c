/*
 * PROJECT:     ReactOS ieframe
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Internet IShellFolder implementation
 * COPYRIGHT:   Copyright 2025 Whindmar Saksit <whindsaks@proton.me>
 */

#define NONAMELESSUNION

#include "ieframe.h"

#include "shlobj.h"
#include "shobjidl.h"
#include "shellapi.h"
#include "shlwapi.h"
#include "shlguid.h"
#include "intshcut.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ieframe);

extern int WINAPI SHAnsiToUnicodeCP(UINT CodePage, LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf);

#define MAX_URL_LENGTH 1024

#define PT_INTERNET_URL 0x61
#define IFUIF_UNICODE 0x80
typedef struct _IFURLITEM
{
    WORD cb;
    BYTE Type; // PT_INTERNET_URL
    BYTE Flags; // IFUIF_*
    UINT Unknown;
    WCHAR Url[ANYSIZE_ARRAY];
} IFURLITEM;

static int GetSchemeCharType(WCHAR c)
{
    if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
        return 0;
    return c == '+' || c == '-' || c == '.' ? 1 : -1;
}

static unsigned int GetSchemeLength(PCWSTR s)
{
    if (GetSchemeCharType(s[0]) != 0) // The first character MUST be A-Z, a-z.
        return 0;
    for (unsigned int i = 0;;)
    {
        if (s[++i] == ':')
            return ++i;
        if (GetSchemeCharType(s[i]) < 0)
            return 0;
    }
}

static LPITEMIDLIST CreateUrlItem(PCWSTR pszUrl)
{
    UINT cch = lstrlenW(pszUrl) + 1;
    UINT cb = FIELD_OFFSET(IFURLITEM, Url[cch]);
    IFURLITEM *pidl = SHAlloc(cb + sizeof(WORD));
    if (!pidl)
        return (LPITEMIDLIST)pidl;
    pidl->cb = cb;
    pidl->Type = PT_INTERNET_URL;
    pidl->Flags = IFUIF_UNICODE;
    pidl->Unknown = 0;
    CopyMemory(pidl->Url, pszUrl, cch * sizeof(*pszUrl));
    ILGetNext((LPITEMIDLIST)pidl)->mkid.cb = 0;
    return (LPITEMIDLIST)pidl;
}

static IFURLITEM* IsUrlItem(LPCITEMIDLIST pidl)
{
    IFURLITEM *p = (IFURLITEM*)pidl;
    if (p && p->cb > FIELD_OFFSET(IFURLITEM, Url) && p->Type == PT_INTERNET_URL)
        return p;
    return NULL;
}

static PWSTR GetUrl(IFURLITEM *pUrl, PWSTR Buffer)
{
    if (pUrl->Flags & IFUIF_UNICODE)
        return pUrl->Url;
    SHAnsiToUnicodeCP(CP_ACP, (PCSTR)pUrl->Url, Buffer, MAX_URL_LENGTH);
    return Buffer;
}

static HRESULT CreateUrlShortcut(PCWSTR Url, IUniformResourceLocatorW **ppv)
{
    HRESULT hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                                  &IID_IUniformResourceLocatorW, (void**)ppv);
    if (SUCCEEDED(hr))
        hr = (*ppv)->lpVtbl->SetURL(*ppv, Url, 0);
    return hr;
}

typedef struct _CInternetFolder
{
    IShellFolder IShellFolder_iface;
    IPersistFolder IPersistFolder_iface;
    LONG refCount;
} CInternetFolder;

static HRESULT Unknown_QueryInterface(CInternetFolder *This, REFIID riid, PVOID *ppvObject)
{
    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppvObject);
    *ppvObject = NULL;
    if (IsEqualGUID(&IID_IUnknown, riid))
        *ppvObject = &This->IShellFolder_iface;
    else if (IsEqualGUID(&IID_IShellFolder, riid))
        *ppvObject = &This->IShellFolder_iface;
    else if (IsEqualGUID(&IID_IPersist, riid) || IsEqualGUID(&IID_IPersistFolder, riid))
        *ppvObject = &This->IPersistFolder_iface;
    else
        return E_NOINTERFACE;
    IUnknown_AddRef((IUnknown*)*ppvObject);
    return S_OK;
}

static ULONG Unknown_AddRef(CInternetFolder *This)
{
    return InterlockedIncrement(&This->refCount);
}

static ULONG Unknown_Release(CInternetFolder *This)
{
    const ULONG count = InterlockedDecrement(&This->refCount);
    if (count == 0)
    {
        SHFree(This);
        unlock_module();
    }
    return count;
}

static HRESULT WINAPI ShellFolder_QueryInterface(IShellFolder *This, REFIID riid, PVOID *ppvObject)
{
    return Unknown_QueryInterface(CONTAINING_RECORD(This, CInternetFolder, IShellFolder_iface), riid, ppvObject);
}

static ULONG WINAPI ShellFolder_AddRef(IShellFolder *This)
{
    return Unknown_AddRef(CONTAINING_RECORD(This, CInternetFolder, IShellFolder_iface));
}

static ULONG WINAPI ShellFolder_Release(IShellFolder *This)
{
    return Unknown_Release(CONTAINING_RECORD(This, CInternetFolder, IShellFolder_iface));
}

static HRESULT WINAPI ParseDisplayName(IShellFolder *This, HWND hwnd, IBindCtx *pbc, LPWSTR pszDisplayName, ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
{
    UINT len;

    *ppidl = NULL;
    len = GetSchemeLength(pszDisplayName);
    if (len)
    {
        if (len + 1 == sizeof("shell:"))
        {
            WCHAR szBuf[100];
            lstrcpynW(szBuf, pszDisplayName, sizeof("shell:"));
            if (!lstrcmpiW(szBuf, L"shell:"))
                return E_FAIL;
        }

        if ((*ppidl = CreateUrlItem(pszDisplayName)) == NULL)
            return E_OUTOFMEMORY;
        if (pchEaten)
            *pchEaten = lstrlenW(pszDisplayName);
        if (pdwAttributes)
            IShellFolder_GetAttributesOf(This, 1, (PCUITEMID_CHILD_ARRAY)ppidl, pdwAttributes);
        return S_OK;
    }
    return E_FAIL;
}

static HRESULT WINAPI ShellFolder_EnumObjects(IShellFolder *This, HWND hwndOwner, SHCONTF grfFlags, IEnumIDList **ppenumIDList)
{
    *ppenumIDList = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI BindToObject(IShellFolder *This, PCUIDLIST_RELATIVE pidl, IBindCtx *pbc, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI BindToStorage(IShellFolder *This, PCUIDLIST_RELATIVE pidl, IBindCtx *pbc, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI CompareIDs(IShellFolder *This, LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    IFURLITEM *pUrl1 = IsUrlItem(pidl1), *pUrl2 = IsUrlItem(pidl2);
    if (pUrl1 && pUrl2)
    {
        WCHAR szUrl1[MAX_URL_LENGTH], *pszUrl1 = GetUrl(pUrl1, szUrl1);
        WCHAR szUrl2[MAX_URL_LENGTH], *pszUrl2 = GetUrl(pUrl2, szUrl2);
        int cmp = lstrcmpiW(pszUrl1, pszUrl2);
        return cmp < 0 ? 0xffff : cmp != 0;
    }
    return E_FAIL;
}

static HRESULT WINAPI CreateViewObject(IShellFolder *This, HWND hwndOwner,REFIID riid,void **ppv)
{
    *ppv = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI GetAttributesOf(IShellFolder *This, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, SFGAOF *rgfInOut)
{
    UINT i;

    if (cidl == 0)
    {
        *rgfInOut &= SFGAO_FOLDER | SFGAO_CANLINK | SFGAO_STREAM; // Folder attributes
        return S_OK;
    }

    for (i = 0; i < cidl; ++i)
    {
        IFURLITEM *pUrl = IsUrlItem(apidl[i]);
        if (!pUrl)
            return E_FAIL;
        *rgfInOut &= SFGAO_CANLINK | SFGAO_BROWSABLE | SFGAO_STREAM;
    }
    return S_OK;
}

static HRESULT WINAPI GetUIObjectOf(IShellFolder *This, HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT *rgfReserved, void **ppv)
{
    IFURLITEM *pUrl;
    if (cidl == 1 && (pUrl = IsUrlItem(apidl[0])) != NULL)
    {
        if (IsEqualIID(riid, &IID_IExtractIconW) || IsEqualIID(riid, &IID_IExtractIconA) ||
            IsEqualIID(riid, &IID_IContextMenu) ||
            IsEqualIID(riid, &IID_IDataObject) ||
            IsEqualIID(riid, &IID_IQueryInfo))
        {
            IUniformResourceLocatorW *pUrlLnk;
            WCHAR szUrl[MAX_URL_LENGTH], *pszUrl = GetUrl(pUrl, szUrl);
            HRESULT hr = CreateUrlShortcut(pszUrl, &pUrlLnk);
            if (SUCCEEDED(hr))
            {
                hr = IUnknown_QueryInterface(pUrlLnk, riid, ppv);
                IUnknown_Release(pUrlLnk);
            }
            return hr;
        }
    }
    return E_NOINTERFACE;
}

static HRESULT WINAPI GetDisplayNameOf(IShellFolder *This, PCUITEMID_CHILD pidl, SHGDNF uFlags, STRRET *pSR)
{
    IFURLITEM *pUrl = IsUrlItem(pidl);
    if (pUrl)
    {
        WCHAR szUrl[MAX_URL_LENGTH], *pszUrl = GetUrl(pUrl, szUrl);
        pSR->uType = STRRET_WSTR;
        return SHStrDupW(pszUrl, &pSR->u.pOleStr);
    }
    return E_FAIL;
}

static HRESULT WINAPI SetNameOf(IShellFolder *This, HWND hwndOwner, PCUITEMID_CHILD pidl, LPCWSTR pszName, SHGDNF uFlags, PITEMID_CHILD*ppidlOut)
{
    if (ppidlOut)
        *ppidlOut = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistFolder_QueryInterface(IPersistFolder *This, REFIID riid, PVOID *ppvObject)
{
    return Unknown_QueryInterface(CONTAINING_RECORD(This, CInternetFolder, IPersistFolder_iface), riid, ppvObject);
}

static ULONG WINAPI PersistFolder_AddRef(IPersistFolder *This)
{
    return Unknown_AddRef(CONTAINING_RECORD(This, CInternetFolder, IPersistFolder_iface));
}

static ULONG WINAPI PersistFolder_Release(IPersistFolder *This)
{
    return Unknown_Release(CONTAINING_RECORD(This, CInternetFolder, IPersistFolder_iface));
}

static HRESULT WINAPI PersistFolder_GetClassID(IPersistFolder *This, CLSID *pClassID)
{
    *pClassID = CLSID_Internet;
    return S_OK;
}

static HRESULT WINAPI PersistFolder_Initialize(IPersistFolder *This, PCIDLIST_ABSOLUTE pidl)
{
    return S_OK;
}

static const IShellFolderVtbl ShellFolderVtbl = {
    ShellFolder_QueryInterface,
    ShellFolder_AddRef,
    ShellFolder_Release,
    ParseDisplayName,
    ShellFolder_EnumObjects,
    BindToObject,
    BindToStorage,
    CompareIDs,
    CreateViewObject,
    GetAttributesOf,
    GetUIObjectOf,
    GetDisplayNameOf,
    SetNameOf
};

static const IPersistFolderVtbl PersistFolderVtbl = {
    PersistFolder_QueryInterface,
    PersistFolder_AddRef,
    PersistFolder_Release,
    PersistFolder_GetClassID,
    PersistFolder_Initialize
};

static CInternetFolder* CreateInstance(void)
{
    CInternetFolder *obj = SHAlloc(sizeof(CInternetFolder));
    if (obj)
    {
        obj->IShellFolder_iface.lpVtbl = &ShellFolderVtbl;
        obj->IPersistFolder_iface.lpVtbl = &PersistFolderVtbl;
        obj->refCount = 1;
        lock_module();
    }
    return obj;
}

HRESULT WINAPI CInternetFolder_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    CInternetFolder *pThis;

    *ppv = NULL;
    if (outer)
        return CLASS_E_NOAGGREGATION;

    if ((pThis = CreateInstance()) != NULL)
    {
        HRESULT hr = Unknown_QueryInterface(pThis, riid, ppv);
        Unknown_Release(pThis);
        return hr;
    }
    return E_OUTOFMEMORY;
}
