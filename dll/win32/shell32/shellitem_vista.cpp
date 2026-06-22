

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

EXTERN_C HRESULT WINAPI SHGetKnownFolderIDList(
    REFKNOWNFOLDERID rfid,
    DWORD flags,
    HANDLE token,
    PIDLIST_ABSOLUTE *pidl);

EXTERN_C HRESULT WINAPI SHCreateShellItem(
    _In_opt_ PCIDLIST_ABSOLUTE pidlParent,
    _In_opt_ IShellFolder *psfParent,
    _In_ PCUITEMID_CHILD pidl,
    _Out_ IShellItem **ppsi);

/***********************************************************************
 *   SHCreateItemFromIDList [SHELL32.@]
 */

EXTERN_C HRESULT WINAPI
SHCreateItemFromIDList(_In_ PCIDLIST_ABSOLUTE pidl, _In_ REFIID riid, _Out_ void **ppv)
{
    CComPtr<IShellItem> item;
    CComPtr<IPersistIDList> persist;
    HRESULT hr;

    TRACE("(%p,%s,%p)\n", pidl, debugstr_guid(&riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (!pidl)
        return E_INVALIDARG;

    hr = CShellItem::_CreatorClass::CreateInstance(NULL, IID_PPV_ARG(IShellItem, &item));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = item->QueryInterface(IID_PPV_ARG(IPersistIDList, &persist));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = persist->SetIDList(pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return item->QueryInterface(riid, ppv);
}

/***********************************************************************
 *   SHCreateItemFromParsingName [SHELL32.@]
 */

EXTERN_C HRESULT WINAPI
SHCreateItemFromParsingName(
    _In_ PCWSTR pszPath,
    _In_opt_ IBindCtx *pbc,
    _In_ REFIID riid,
    _Out_ void **ppv)
{
    CComHeapPtr<ITEMIDLIST> pidl;
    HRESULT hr;

    TRACE("(%s,%p,%s,%p)\n", debugstr_w(pszPath), pbc, debugstr_guid(&riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    hr = SHParseDisplayName(pszPath, pbc, &pidl, 0, NULL);
    if (SUCCEEDED(hr))
        hr = SHCreateItemFromIDList(pidl, riid, ppv);

    return hr;
}

/***********************************************************************
 *   SHCreateItemFromRelativeName [SHELL32.@]
 */

EXTERN_C HRESULT WINAPI
SHCreateItemFromRelativeName(
    _In_ IShellItem *parent,
    _In_ PCWSTR pszName,
    _In_opt_ IBindCtx *pbc,
    _In_ REFIID riid,
    _Out_ void **ppv)
{
    CComPtr<IShellFolder> desktop;
    CComPtr<IShellFolder> folder;
    CComHeapPtr<ITEMIDLIST> pidlFolder;
    CComHeapPtr<ITEMIDLIST> pidlChild;
    HRESULT hr;

    TRACE("(%p,%s,%p,%s,%p)\n", parent, debugstr_w(pszName), pbc, debugstr_guid(&riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (!pszName)
        return E_INVALIDARG;

    hr = SHGetIDListFromObject(parent, &pidlFolder);
    if (hr != S_OK)
        return hr;

    hr = SHGetDesktopFolder(&desktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (!_ILIsDesktop(pidlFolder))
    {
        hr = desktop->BindToObject(pidlFolder, NULL, IID_PPV_ARG(IShellFolder, &folder));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    {
        IShellFolder *psfBind = folder.p ? folder.p : desktop.p;
        hr = psfBind->ParseDisplayName(NULL, pbc, const_cast<LPWSTR>(pszName), NULL, &pidlChild, NULL);
    }
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return SHCreateItemFromIDList(pidlChild, riid, ppv);
}

/***********************************************************************
 *   SHCreateItemInKnownFolder [SHELL32.@]
 */

EXTERN_C HRESULT WINAPI
SHCreateItemInKnownFolder(
    _In_ REFKNOWNFOLDERID kfid,
    _In_ DWORD dwFlags,
    _In_opt_ PCWSTR pszFileName,
    _In_ REFIID riid,
    _Out_ void **ppv)
{
    TRACE("(%s,%lx,%s,%s,%p)\n", debugstr_guid(&kfid), dwFlags, debugstr_w(pszFileName),
          debugstr_guid(&riid), ppv);
    return E_NOTIMPL;
    //TODO: Import SHGetKnownFolderIDList from wine 
#if 0
    CComPtr<IShellItem> parent;
    CComHeapPtr<ITEMIDLIST> pidl;
    HRESULT hr;

    TRACE("(%s,%lx,%s,%s,%p)\n", debugstr_guid(&kfid), dwFlags, debugstr_w(pszFileName),
          debugstr_guid(&riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    hr = SHGetKnownFolderIDList(kfid, dwFlags, NULL, &pidl);
    if (hr != S_OK)
        return hr;

    hr = SHCreateItemFromIDList(pidl, IID_PPV_ARG(IShellItem, &parent));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (pszFileName)
        hr = SHCreateItemFromRelativeName(parent, pszFileName, NULL, riid, ppv);
    else
        hr = parent->QueryInterface(riid, ppv);

    return hr;
#endif
}

/***********************************************************************
 *   SHCreateShellItemArray [SHELL32.@]
 */

EXTERN_C HRESULT WINAPI
SHCreateShellItemArray(
    _In_opt_ PCIDLIST_ABSOLUTE pidlParent,
    _In_opt_ IShellFolder *psf,
    _In_ UINT cidl,
    _In_reads_opt_(cidl) PCUITEMID_CHILD_ARRAY ppidl,
    _Out_ IShellItemArray **ppsiItemArray)
{
    CComHeapPtr<IShellItem *> array;
    HRESULT hr = E_FAIL;

    TRACE("(%p,%p,%u,%p,%p)\n", pidlParent, psf, cidl, ppidl, ppsiItemArray);

    if (!ppsiItemArray)
        return E_POINTER;
    *ppsiItemArray = NULL;

    if (!pidlParent && !psf)
        return E_POINTER;

    if (!ppidl)
        return E_INVALIDARG;

    array.Allocate(cidl);
    if (!array)
        return E_OUTOFMEMORY;

    for (UINT i = 0; i < cidl; ++i)
        array[i] = NULL;

    for (UINT i = 0; i < cidl; ++i)
    {
        hr = SHCreateShellItem(pidlParent, psf, ppidl[i], &array[i]);
        if (FAILED_UNEXPECTEDLY(hr))
            break;
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateShellItemArrayFromItems(array, cidl, IID_PPV_ARG(IShellItemArray, ppsiItemArray));
    }

    for (UINT i = 0; i < cidl; ++i)
    {
        if (array[i])
            array[i]->Release();
    }

    return hr;
}

/***********************************************************************
 *   SHCreateShellItemArrayFromIDLists [SHELL32.@]
 */

EXTERN_C HRESULT WINAPI
SHCreateShellItemArrayFromIDLists(
    _In_ UINT cidl,
    _In_reads_(cidl) PCIDLIST_ABSOLUTE_ARRAY pidlArray,
    _Out_ IShellItemArray **ppsiItemArray)
{
    CComHeapPtr<IShellItem *> array;
    HRESULT hr;

    TRACE("(%u,%p,%p)\n", cidl, pidlArray, ppsiItemArray);

    if (!ppsiItemArray)
        return E_POINTER;
    *ppsiItemArray = NULL;

    if (cidl == 0 || !pidlArray)
        return E_INVALIDARG;

    array.Allocate(cidl);
    if (!array)
        return E_OUTOFMEMORY;

    for (UINT i = 0; i < cidl; ++i)
        array[i] = NULL;

    for (UINT i = 0; i < cidl; ++i)
    {
        hr = SHCreateShellItem(NULL, NULL, pidlArray[i], &array[i]);
        if (FAILED_UNEXPECTEDLY(hr))
            break;
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateShellItemArrayFromItems(array, cidl, IID_PPV_ARG(IShellItemArray, ppsiItemArray));
    }

    for (UINT i = 0; i < cidl; ++i)
    {
        if (array[i])
            array[i]->Release();
    }

    return hr;
}

/***********************************************************************
 *   SHCreateShellItemArrayFromShellItem [SHELL32.@]
 */

EXTERN_C HRESULT WINAPI
SHCreateShellItemArrayFromShellItem(_In_ IShellItem *psi, _In_ REFIID riid, _Out_ void **ppv)
{
    IShellItem *items[1] = { psi };

    TRACE("(%p,%s,%p)\n", psi, debugstr_guid(&riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (!psi)
        return E_INVALIDARG;

    return CreateShellItemArrayFromItems(items, 1, riid, ppv);
}

/***********************************************************************
 *   SHCreateShellItemArrayFromDataObject [SHELL32.@]
 */

EXTERN_C HRESULT WINAPI
SHCreateShellItemArrayFromDataObject(_In_ IDataObject *pdo, _In_ REFIID riid, _Out_ void **ppv)
{
    return ShellObjectCreatorInit<CShellItemArray>(pdo, riid, ppv);
}

/***********************************************************************
 *   SHGetItemFromObject [SHELL32.@]
 */

EXTERN_C HRESULT WINAPI
SHGetItemFromObject(_In_ IUnknown *punk, _In_ REFIID riid, _Out_ void **ppv)
{
    CComHeapPtr<ITEMIDLIST> pidl;
    HRESULT hr;

    TRACE("(%p,%s,%p)\n", punk, debugstr_guid(&riid), ppv);

    hr = SHGetIDListFromObject(punk, &pidl);
    if (SUCCEEDED(hr))
        hr = SHCreateItemFromIDList(pidl, riid, ppv);
    return hr;
}

/***********************************************************************
 *   SHGetItemFromDataObject [SHELL32.@]
 */

EXTERN_C HRESULT WINAPI
SHGetItemFromDataObject(
    _In_ IDataObject *pdtobj,
    _In_ DATAOBJ_GET_ITEM_FLAGS dwFlags,
    _In_ REFIID riid,
    _Out_ void **ppv)
{
    FORMATETC fmt;
    STGMEDIUM medium = { 0 };
    HRESULT hr;

    TRACE("(%p,%x,%s,%p)\n", pdtobj, dwFlags, debugstr_guid(&riid), ppv);

    if (!pdtobj)
        return E_INVALIDARG;

    fmt.cfFormat = (CLIPFORMAT)RegisterClipboardFormatW(CFSTR_SHELLIDLISTW);
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;

    hr = pdtobj->GetData(&fmt, &medium);
    if (SUCCEEDED(hr))
    {
        LPIDA pida = (LPIDA)GlobalLock(medium.hGlobal);
        if (pida && pida->cidl >= 1 &&
            ((pida->cidl > 1 && !(dwFlags & DOGIF_ONLY_IF_ONE)) || pida->cidl == 1))
        {
            LPITEMIDLIST pidl = ILCombine(HIDA_GetPIDLFolder(pida),
                                          HIDA_GetPIDLItem(pida, 0));
            if (pidl)
            {
                hr = SHCreateItemFromIDList(pidl, riid, ppv);
                ILFree(pidl);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            TRACE("Failed to create item, cidl=%u, dwFlags=%#x\n", pida ? pida->cidl : 0, dwFlags);
            hr = E_FAIL;
        }

        if (medium.hGlobal)
        {
            GlobalUnlock(medium.hGlobal);
            ReleaseStgMedium(&medium);
        }
    }

    if (FAILED(hr) && !(dwFlags & DOGIF_NO_HDROP))
    {
        TRACE("Attempting to fall back on CF_HDROP.\n");

        fmt.cfFormat = CF_HDROP;
        fmt.ptd = NULL;
        fmt.dwAspect = DVASPECT_CONTENT;
        fmt.lindex = -1;
        fmt.tymed = TYMED_HGLOBAL;
        ZeroMemory(&medium, sizeof(medium));

        hr = pdtobj->GetData(&fmt, &medium);
        if (SUCCEEDED(hr))
        {
            DROPFILES *df = (DROPFILES *)GlobalLock(medium.hGlobal);
            hr = E_FAIL;
            if (df)
            {
                LPBYTE files = (LPBYTE)df + df->pFiles;
                if (!df->fWide)
                {
                    PCSTR first = (PCSTR)files;
                    BOOL multi = *(first + lstrlenA(first) + 1) != 0;
                    if (!(multi && (dwFlags & DOGIF_ONLY_IF_ONE)))
                    {
                        WCHAR filename[MAX_PATH];
                        MultiByteToWideChar(CP_ACP, 0, first, -1, filename, _countof(filename));
                        hr = SHCreateItemFromParsingName(filename, NULL, riid, ppv);
                    }
                }
                else
                {
                    PCWSTR first = (PCWSTR)files;
                    BOOL multi = *(first + lstrlenW(first) + 1) != 0;
                    if (!(multi && (dwFlags & DOGIF_ONLY_IF_ONE)))
                        hr = SHCreateItemFromParsingName(first, NULL, riid, ppv);
                }
                GlobalUnlock(medium.hGlobal);
            }
            ReleaseStgMedium(&medium);
        }
    }

    if (FAILED(hr) && !(dwFlags & DOGIF_NO_URL))
        FIXME("Failed to create item, should try CF_URL.\n");

    return hr;
}

/***********************************************************************
 *   SHGetPropertyStoreFromParsingName [SHELL32.@]
 */

EXTERN_C HRESULT WINAPI
SHGetPropertyStoreFromParsingName(
    _In_ PCWSTR pszPath,
    _In_opt_ IBindCtx *pbc,
    _In_ GETPROPERTYSTOREFLAGS flags,
    _In_ REFIID riid,
    _Out_ void **ppv)
{
    CComPtr<IShellItem2> item;
    HRESULT hr;

    TRACE("(%s,%p,%#x,%s,%p)\n", debugstr_w(pszPath), pbc, flags, debugstr_guid(&riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    hr = SHCreateItemFromParsingName(pszPath, pbc, IID_PPV_ARG(IShellItem2, &item));
    if (SUCCEEDED(hr))
        hr = item->GetPropertyStore(flags, riid, ppv);

    return hr;
}
