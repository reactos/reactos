/*
 * IShellItem implementation
 *
 * Copyright 2008 Vincent Povirk for CodeWeavers
 * Copyright 2009 Andrew Hill
 * Copyright 2013 Katayama Hirofumi MZ
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

CShellItem::CShellItem() :
    m_pidl(NULL)
{
}

CShellItem::~CShellItem()
{
    ILFree(m_pidl);
}

HRESULT CShellItem::get_parent_pidl(LPITEMIDLIST *parent_pidl)
{
    *parent_pidl = ILClone(m_pidl);
    if (*parent_pidl)
    {
        if (ILRemoveLastID(*parent_pidl))
            return S_OK;
        else
        {
            ILFree(*parent_pidl);
            *parent_pidl = NULL;
            return E_INVALIDARG;
        }
    }
    else
    {
        *parent_pidl = NULL;
        return E_OUTOFMEMORY;
    }
}

HRESULT CShellItem::get_parent_shellfolder(IShellFolder **ppsf)
{
    HRESULT hr;
    LPITEMIDLIST parent_pidl;
    CComPtr<IShellFolder>        desktop;

    hr = get_parent_pidl(&parent_pidl);
    if (SUCCEEDED(hr))
    {
        hr = SHGetDesktopFolder(&desktop);
        if (SUCCEEDED(hr))
            hr = desktop->BindToObject(parent_pidl, NULL, IID_PPV_ARG(IShellFolder, ppsf));
        ILFree(parent_pidl);
    }

    return hr;
}

HRESULT CShellItem::get_shellfolder(IBindCtx *pbc, REFIID riid, void **ppvOut)
{
    CComPtr<IShellFolder> psf;
    CComPtr<IShellFolder> psfDesktop;
    HRESULT ret;

    ret = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(ret))
        return ret;

    if (_ILIsDesktop(m_pidl))
        psf = psfDesktop;
    else
    {
        ret = psfDesktop->BindToObject(m_pidl, pbc, IID_PPV_ARG(IShellFolder, &psf));
        if (FAILED_UNEXPECTEDLY(ret))
            return ret;
    }

    return psf->QueryInterface(riid, ppvOut);
}

HRESULT WINAPI CShellItem::BindToHandler(IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppvOut)
{
    HRESULT ret;
    TRACE("(%p, %p,%s,%p,%p)\n", this, pbc, shdebugstr_guid(&rbhid), riid, ppvOut);

    *ppvOut = NULL;
    if (IsEqualGUID(rbhid, BHID_SFObject))
    {
        return get_shellfolder(pbc, riid, ppvOut);
    }
    else if (IsEqualGUID(rbhid, BHID_SFUIObject))
    {
        CComPtr<IShellFolder> psf_parent;
        if (_ILIsDesktop(m_pidl))
            ret = SHGetDesktopFolder(&psf_parent);
        else
            ret = get_parent_shellfolder(&psf_parent);
        if (FAILED_UNEXPECTEDLY(ret))
            return ret;

        LPCITEMIDLIST pidl = ILFindLastID(m_pidl);
        return psf_parent->GetUIObjectOf(NULL, 1, &pidl, riid, NULL, ppvOut);
    }
    else if (IsEqualGUID(rbhid, BHID_DataObject))
    {
        return BindToHandler(pbc, BHID_SFUIObject, IID_IDataObject, ppvOut);
    }
    else if (IsEqualGUID(rbhid, BHID_SFViewObject))
    {
        CComPtr<IShellFolder> psf;
        ret = get_shellfolder(NULL, IID_PPV_ARG(IShellFolder, &psf));
        if (FAILED_UNEXPECTEDLY(ret))
            return ret;

        return psf->CreateViewObject(NULL, riid, ppvOut);
    }

    FIXME("Unsupported BHID %s.\n", debugstr_guid(&rbhid));

    return MK_E_NOOBJECT;
}

HRESULT WINAPI CShellItem::GetParent(IShellItem **ppsi)
{
    HRESULT hr;
    LPITEMIDLIST parent_pidl;

    TRACE("(%p,%p)\n", this, ppsi);

    hr = get_parent_pidl(&parent_pidl);
    if (SUCCEEDED(hr))
    {
        hr = SHCreateShellItem(NULL, NULL, parent_pidl, ppsi);
        ILFree(parent_pidl);
    }

    return hr;
}

HRESULT WINAPI CShellItem::GetDisplayName(SIGDN sigdnName, LPWSTR *ppszName)
{
    return SHGetNameFromIDList(m_pidl, sigdnName, ppszName);
}

HRESULT WINAPI CShellItem::GetAttributes(SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs)
{
    CComPtr<IShellFolder>        parent_folder;
    LPCITEMIDLIST child_pidl;
    HRESULT hr;

    TRACE("(%p,%x,%p)\n", this, sfgaoMask, psfgaoAttribs);

    if (_ILIsDesktop(m_pidl))
        hr = SHGetDesktopFolder(&parent_folder);
    else
        hr = get_parent_shellfolder(&parent_folder);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    child_pidl = ILFindLastID(m_pidl);
    *psfgaoAttribs = sfgaoMask;
    hr = parent_folder->GetAttributesOf(1, &child_pidl, psfgaoAttribs);
    *psfgaoAttribs &= sfgaoMask;

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return (sfgaoMask == *psfgaoAttribs) ? S_OK : S_FALSE;
}

HRESULT WINAPI CShellItem::Compare(IShellItem *oth, SICHINTF hint, int *piOrder)
{
    HRESULT hr;
    CComPtr<IPersistIDList>      pIDList;
    CComPtr<IShellFolder>        psfDesktop;
    LPITEMIDLIST pidl;

    TRACE("(%p,%p,%x,%p)\n", this, oth, hint, piOrder);

    if (piOrder == NULL || oth == NULL)
        return E_POINTER;

    hr = oth->QueryInterface(IID_PPV_ARG(IPersistIDList, &pIDList));
    if (SUCCEEDED(hr))
    {
        hr = pIDList->GetIDList(&pidl);
        if (SUCCEEDED(hr))
        {
            hr = SHGetDesktopFolder(&psfDesktop);
            if (SUCCEEDED(hr))
            {
                hr = psfDesktop->CompareIDs(hint, m_pidl, pidl);
                *piOrder = (int)(short)SCODE_CODE(hr);
            }
            ILFree(pidl);
        }
    }

    if(FAILED(hr))
        return hr;

    if(*piOrder)
        return S_FALSE;
    else
        return S_OK;
}

HRESULT WINAPI CShellItem::GetClassID(CLSID *pClassID)
{
    TRACE("(%p,%p)\n", this, pClassID);

    *pClassID = CLSID_ShellItem;
    return S_OK;
}

HRESULT WINAPI CShellItem::SetIDList(PCIDLIST_ABSOLUTE pidlx)
{
    LPITEMIDLIST new_pidl;

    TRACE("(%p,%p)\n", this, pidlx);

    new_pidl = ILClone(pidlx);
    if (new_pidl)
    {
        ILFree(m_pidl);
        m_pidl = new_pidl;
        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}

HRESULT WINAPI CShellItem::GetIDList(PIDLIST_ABSOLUTE *ppidl)
{
    TRACE("(%p,%p)\n", this, ppidl);

    *ppidl = ILClone(m_pidl);
    if (*ppidl)
        return S_OK;
    else
        return E_OUTOFMEMORY;
}

HRESULT WINAPI SHCreateShellItem(PCIDLIST_ABSOLUTE pidlParent,
    IShellFolder *psfParent, PCUITEMID_CHILD pidl, IShellItem **ppsi)
{
    HRESULT hr;
    CComPtr<IShellItem> newShellItem;
    LPITEMIDLIST new_pidl;
    CComPtr<IPersistIDList>            newPersistIDList;

    TRACE("(%p,%p,%p,%p)\n", pidlParent, psfParent, pidl, ppsi);

    *ppsi = NULL;

    if (!pidl)
        return E_INVALIDARG;

    if (pidlParent || psfParent)
    {
        LPITEMIDLIST temp_parent = NULL;
        if (!pidlParent)
        {
            CComPtr<IPersistFolder2>    ppf2Parent;

            if (FAILED(psfParent->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2Parent))))
            {
                FIXME("couldn't get IPersistFolder2 interface of parent\n");
                return E_NOINTERFACE;
            }

            if (FAILED(ppf2Parent->GetCurFolder(&temp_parent)))
            {
                FIXME("couldn't get parent PIDL\n");
                return E_NOINTERFACE;
            }

            pidlParent = temp_parent;
        }

        new_pidl = ILCombine(pidlParent, pidl);
        ILFree(temp_parent);

        if (!new_pidl)
            return E_OUTOFMEMORY;
    }
    else
    {
        new_pidl = ILClone(pidl);
        if (!new_pidl)
            return E_OUTOFMEMORY;
    }

    hr = CShellItem::_CreatorClass::CreateInstance(NULL, IID_PPV_ARG(IShellItem, &newShellItem));
    if (FAILED(hr))
    {
        ILFree(new_pidl);
        return hr;
    }
    hr = newShellItem->QueryInterface(IID_PPV_ARG(IPersistIDList, &newPersistIDList));
    if (FAILED(hr))
    {
        ILFree(new_pidl);
        return hr;
    }
    hr = newPersistIDList->SetIDList(new_pidl);
    if (FAILED(hr))
    {
        ILFree(new_pidl);
        return hr;
    }
    ILFree(new_pidl);

    *ppsi = newShellItem.Detach();

    return hr;
}

/***********************************************************************
 *   SHCreateItemFromIDList [SHELL32.@]
 */
EXTERN_C HRESULT WINAPI
SHCreateItemFromIDList(_In_ PCIDLIST_ABSOLUTE pidl, _In_ REFIID riid, _Out_ void **ppv)
{
    CComPtr<IShellItem> item;
    HRESULT hr;

    TRACE("(%p,%s,%p)\n", pidl, debugstr_guid(&riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (!pidl)
        return E_INVALIDARG;

    hr = SHCreateShellItem(NULL, NULL, pidl, &item);
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

#if DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA

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
EXTERN_C HRESULT WINAPI SHGetKnownFolderIDList(
    REFKNOWNFOLDERID rfid,
    DWORD flags,
    HANDLE token,
    PIDLIST_ABSOLUTE *pidl);

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

#endif /* DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA */
