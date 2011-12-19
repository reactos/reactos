/*
 * IShellItem implementation
 *
 * Copyright 2008 Vincent Povirk for CodeWeavers
 * Copyright 2009 Andrew Hill
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

EXTERN_C HRESULT WINAPI SHCreateShellItem(LPCITEMIDLIST pidlParent,
    IShellFolder *psfParent, LPCITEMIDLIST pidl, IShellItem **ppsi);

CShellItem::CShellItem()
{
    pidl = NULL;
}

CShellItem::~CShellItem()
{
    ILFree(pidl);
}

HRESULT CShellItem::get_parent_pidl(LPITEMIDLIST *parent_pidl)
{
    *parent_pidl = ILClone(pidl);
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
    LPITEMIDLIST parent_pidl;
    CComPtr<IShellFolder>        desktop;
    HRESULT ret;

    ret = get_parent_pidl(&parent_pidl);
    if (SUCCEEDED(ret))
    {
        ret = SHGetDesktopFolder(&desktop);
        if (SUCCEEDED(ret))
            ret = desktop->BindToObject(parent_pidl, NULL, IID_IShellFolder, (void**)ppsf);
        ILFree(parent_pidl);
    }

    return ret;
}

HRESULT WINAPI CShellItem::BindToHandler(IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppvOut)
{
    FIXME("(%p,%p,%s,%p,%p)\n", this, pbc, shdebugstr_guid(&rbhid), riid, ppvOut);

    *ppvOut = NULL;

    return E_NOTIMPL;
}

HRESULT WINAPI CShellItem::GetParent(IShellItem **ppsi)
{
    LPITEMIDLIST parent_pidl;
    HRESULT ret;

    TRACE("(%p,%p)\n", this, ppsi);

    ret = get_parent_pidl(&parent_pidl);
    if (SUCCEEDED(ret))
    {
        ret = SHCreateShellItem(NULL, NULL, parent_pidl, ppsi);
        ILFree(parent_pidl);
    }

    return ret;
}

HRESULT WINAPI CShellItem::GetDisplayName(SIGDN sigdnName, LPWSTR *ppszName)
{
    FIXME("(%p,%x,%p)\n", this, sigdnName, ppszName);

    *ppszName = NULL;

    return E_NOTIMPL;
}

HRESULT WINAPI CShellItem::GetAttributes(SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs)
{
    CComPtr<IShellFolder>        parent_folder;
    LPITEMIDLIST child_pidl;
    HRESULT ret;

    TRACE("(%p,%x,%p)\n", this, sfgaoMask, psfgaoAttribs);

    ret = get_parent_shellfolder(&parent_folder);
    if (SUCCEEDED(ret))
    {
        child_pidl = ILFindLastID(pidl);
        *psfgaoAttribs = sfgaoMask;
        ret = parent_folder->GetAttributesOf(1, (LPCITEMIDLIST*)&child_pidl, psfgaoAttribs);
    }

    return ret;
}

HRESULT WINAPI CShellItem::Compare(IShellItem *oth, SICHINTF hint, int *piOrder)
{
    FIXME("(%p,%p,%x,%p)\n", this, oth, hint, piOrder);

    return E_NOTIMPL;
}

HRESULT WINAPI CShellItem::GetClassID(CLSID *pClassID)
{
    TRACE("(%p,%p)\n", this, pClassID);

    *pClassID = CLSID_ShellItem;
    return S_OK;
}


HRESULT WINAPI CShellItem::SetIDList(LPCITEMIDLIST pidlx)
{
    LPITEMIDLIST new_pidl;

    TRACE("(%p,%p)\n", this, pidlx);

    new_pidl = ILClone(pidlx);

    if (new_pidl)
    {
        ILFree(pidl);
        pidl = new_pidl;
        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}

HRESULT WINAPI CShellItem::GetIDList(LPITEMIDLIST *ppidl)
{
    TRACE("(%p,%p)\n", this, ppidl);

    *ppidl = ILClone(pidl);
    if (*ppidl)
        return S_OK;
    else
        return E_OUTOFMEMORY;
}

HRESULT WINAPI SHCreateShellItem(LPCITEMIDLIST pidlParent,
    IShellFolder *psfParent, LPCITEMIDLIST pidl, IShellItem **ppsi)
{
    IShellItem *newShellItem;
    LPITEMIDLIST new_pidl;
    CComPtr<IPersistIDList>            newPersistIDList;
    HRESULT ret;

    TRACE("(%p,%p,%p,%p)\n", pidlParent, psfParent, pidl, ppsi);

    if (!pidl)
    {
        return E_INVALIDARG;
    }
    else if (pidlParent || psfParent)
    {
        LPITEMIDLIST temp_parent=NULL;
        if (!pidlParent)
        {
            CComPtr<IPersistFolder2>    ppf2Parent;

            if (FAILED(psfParent->QueryInterface(IID_IPersistFolder2, (void**)&ppf2Parent)))
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

    ret = CShellItem::_CreatorClass::CreateInstance(NULL, IID_IShellItem, (void**)&newShellItem);
    if (FAILED(ret))
    {
        *ppsi = NULL;
        ILFree(new_pidl);
        return ret;
    }
    ret = newShellItem->QueryInterface(IID_IPersistIDList, (void **)&newPersistIDList);
    if (FAILED(ret))
    {
        ILFree(new_pidl);
        return ret;
    }
    ret = newPersistIDList->SetIDList(new_pidl);
    if (FAILED(ret))
    {
        ILFree(new_pidl);
        return ret;
    }
    ILFree(new_pidl);
    *ppsi = newShellItem;
    return ret;
}
