/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Folder implementation
 * COPYRIGHT:   Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


CFolder::CFolder()
{
}

CFolder::~CFolder()
{
}

HRESULT CFolder::Initialize(LPITEMIDLIST idlist)
{
    m_idlist.Attach(ILClone(idlist));
    return CShellDispatch_Constructor(IID_PPV_ARG(IShellDispatch, &m_Application));
}

HRESULT CFolder::GetShellFolder(CComPtr<IShellFolder>& psfCurrent)
{
    CComPtr<IShellFolder> psfDesktop;

    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return psfDesktop->BindToObject(m_idlist, NULL, IID_PPV_ARG(IShellFolder, &psfCurrent));
}

// *** Folder methods ***
HRESULT STDMETHODCALLTYPE CFolder::get_Title(BSTR *pbs)
{
    if (!pbs)
        return E_POINTER;

    WCHAR path[MAX_PATH+2] = {0};
    HRESULT hr = ILGetDisplayNameExW(NULL, m_idlist, path, ILGDN_INFOLDER);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    *pbs = SysAllocString(path);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFolder::get_Application(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);

    if (!ppid)
        return E_INVALIDARG;

    *ppid = m_Application;
    (*ppid)->AddRef();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFolder::get_Parent(IDispatch **ppid)
{
    TRACE("(%p %p)\n", this, ppid);

    if (ppid)
        *ppid = NULL;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::get_ParentFolder(Folder **ppsf)
{
    TRACE("(%p, %p)\n", this);

    *ppsf = NULL;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::Items(FolderItems **ppid)
{
    /* FolderItems_Constructor */
    return ShellObjectCreatorInit<CFolderItems>(static_cast<LPITEMIDLIST>(m_idlist), this, IID_PPV_ARG(FolderItems, ppid));
}

HRESULT STDMETHODCALLTYPE CFolder::ParseName(BSTR bName, FolderItem **ppid)
{
    TRACE("(%p, %s, %p)\n", this, wine_dbgstr_w(bName), ppid);
    if (!ppid)
        return E_POINTER;
    *ppid = NULL;

    CComPtr<IShellFolder> psfCurrent;
    HRESULT hr = GetShellFolder(psfCurrent);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComHeapPtr<ITEMIDLIST_RELATIVE> relativePidl;
    hr = psfCurrent->ParseDisplayName(NULL, NULL, bName, NULL, &relativePidl, NULL);
    if (!SUCCEEDED(hr))
        return S_FALSE;

    CComHeapPtr<ITEMIDLIST> combined;
    combined.Attach(ILCombine(m_idlist, relativePidl));

    return ShellObjectCreatorInit<CFolderItem>(this, static_cast<LPITEMIDLIST>(combined), IID_PPV_ARG(FolderItem, ppid));
}

HRESULT STDMETHODCALLTYPE CFolder::NewFolder(BSTR bName, VARIANT vOptions)
{
    TRACE("(%p, %s, %s)\n", this, wine_dbgstr_w(bName), wine_dbgstr_variant(&vOptions));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::MoveHere(VARIANT vItem, VARIANT vOptions)
{
    TRACE("(%p, %s, %s)\n", this, wine_dbgstr_variant(&vItem), wine_dbgstr_variant(&vOptions));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::CopyHere(VARIANT vItem, VARIANT vOptions)
{
    TRACE("(%p, %s, %s)\n", this, wine_dbgstr_variant(&vItem), wine_dbgstr_variant(&vOptions));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::GetDetailsOf(VARIANT vItem, int iColumn, BSTR *pbs)
{
    TRACE("(%p, %s, %i, %p)\n", this, wine_dbgstr_variant(&vItem), iColumn, pbs);
    return E_NOTIMPL;
}


// *** Folder2 methods ***
HRESULT STDMETHODCALLTYPE CFolder::get_Self(FolderItem **ppfi)
{
    TRACE("(%p, %p)\n", this, ppfi);
    if (!ppfi)
        return E_POINTER;

    return ShellObjectCreatorInit<CFolderItem>(this, static_cast<LPITEMIDLIST>(m_idlist), IID_PPV_ARG(FolderItem, ppfi));
}

HRESULT STDMETHODCALLTYPE CFolder::get_OfflineStatus(LONG *pul)
{
    TRACE("(%p, %p)\n", this, pul);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::Synchronize()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::get_HaveToShowWebViewBarricade(VARIANT_BOOL *pbHaveToShowWebViewBarricade)
{
    TRACE("(%p, %p)\n", this, pbHaveToShowWebViewBarricade);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::DismissedWebViewBarricade()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}


