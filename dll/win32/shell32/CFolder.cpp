/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Folder implementation
 * COPYRIGHT:   Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"
#include "prop.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


EXTERN_C BOOL WINAPI Win32CreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec);

CFolder::CFolder()
{
}

CFolder::~CFolder()
{
}

HRESULT CFolder::Initialize(LPCITEMIDLIST idlist)
{
    m_idlist.Attach(ILClone(idlist));
    if (!m_idlist)
        return E_OUTOFMEMORY;
    return CShellDispatch_Constructor(IID_PPV_ARG(IShellDispatch, &m_Application));
}

HRESULT CFolder::GetShellFolder(CComPtr<IShellFolder>& psfCurrent)
{
    HRESULT hr = SHBindToObjectEx(NULL, GetAbsoluteIDList(), NULL, IID_PPV_ARG(IShellFolder, &psfCurrent));
    FAILED_UNEXPECTEDLY(hr);
    return hr;
}

// *** Folder methods ***
HRESULT STDMETHODCALLTYPE CFolder::get_Title(BSTR *pbs)
{
    if (!pbs)
        return E_POINTER;

    WCHAR path[MAX_PATH];
    HRESULT hr = ILGetDisplayNameExW(NULL, m_idlist, path, ILGDN_INFOLDER) ? S_OK : E_FAIL;
    path[_countof(path) - 1] = UNICODE_NULL;
    *pbs = SysAllocString(SUCCEEDED(hr) ? path : L"");
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
    TRACE("(%p, %p)\n", this, ppsf);

    *ppsf = NULL;
    LPCITEMIDLIST pidlAbsSelf = GetAbsoluteIDList();
    if (ILIsEmpty(pidlAbsSelf))
        return S_FALSE;

    CComHeapPtr<ITEMIDLIST> pidlParent(ILClone(pidlAbsSelf));
    if (!pidlParent)
        return E_OUTOFMEMORY;
    ILRemoveLastID(pidlParent);
    return ShellObjectCreatorInit<CFolder>(static_cast<LPITEMIDLIST>(pidlParent), IID_PPV_ARG(Folder, ppsf));
}

HRESULT STDMETHODCALLTYPE CFolder::Items(FolderItems **ppid)
{
    /* FolderItems_Constructor */
    return ShellObjectCreatorInit<CFolderItems>(GetAbsoluteIDList(), this, IID_PPV_ARG(FolderItems, ppid));
}

HRESULT STDMETHODCALLTYPE CFolder::ParseName(BSTR bName, FolderItem **ppid)
{
    TRACE("(%p, %s, %p)\n", this, wine_dbgstr_w(bName), ppid);
    if (!ppid)
        return E_POINTER;
    *ppid = NULL;

    LPCITEMIDLIST pidlAbsSelf = GetAbsoluteIDList();
    CComPtr<IShellFolder> psfCurrent;
    HRESULT hr = GetShellFolder(psfCurrent);
    if (FAILED(hr))
        return hr;

    CComHeapPtr<ITEMIDLIST_RELATIVE> relativePidl;
    hr = psfCurrent->ParseDisplayName(NULL, NULL, bName, NULL, &relativePidl, NULL);
    if (!SUCCEEDED(hr))
        return S_FALSE;

    CComHeapPtr<ITEMIDLIST> combined;
    combined.Attach(ILCombine(pidlAbsSelf, relativePidl));
    return ShellObjectCreatorInit<CFolderItem>(this, static_cast<LPITEMIDLIST>(combined), IID_PPV_ARG(FolderItem, ppid));
}

HRESULT STDMETHODCALLTYPE CFolder::NewFolder(BSTR bName, VARIANT vOptions)
{
    TRACE("(%p, %s, %s)\n", this, wine_dbgstr_w(bName), wine_dbgstr_variant(&vOptions));

    // Note: MSDN says vOptions "is not currently used"
    CComPtr<IStorage> pStg, pStgNew;
    HRESULT hr = SHBindToObjectEx(NULL, GetAbsoluteIDList(), NULL, IID_PPV_ARG(IStorage, &pStg));
    if (FAILED(hr))
        goto tryfs;//return hr;
    hr = pStg->CreateStorage(bName, STGM_FAILIFTHERE, 0, 0, &pStgNew);
    return (hr == STG_E_FILEALREADYEXISTS) ? S_OK : hr;

tryfs: // HACKFIX: Our CFSFolder does not yet support IStorage, try it directly
    LPCITEMIDLIST pidlAbsSelf = GetAbsoluteIDList();
    if (SHGetAttributes(NULL, pidlAbsSelf, SFGAO_FILESYSTEM) == SFGAO_FILESYSTEM)
    {
        CComHeapPtr<WCHAR> pszDirPath;
        if (SUCCEEDED(SHELL_DisplayNameOf(NULL, pidlAbsSelf, SHGDN_FORPARSING, &pszDirPath)))
        {
            DWORD cch = wcslen(pszDirPath) + 1 + wcslen(bName) + 1, attr;
            if (PWSTR pszPath = (PWSTR)SHAlloc(cch * sizeof(WCHAR)))
            {
                StringCchPrintfW(pszPath, cch, L"%s\\%s", &pszDirPath[0], bName);
                if (PathFileExistsAndAttributesW(pszPath, &attr))
                    hr = (attr & FILE_ATTRIBUTE_DIRECTORY) ? S_OK : S_FALSE;
                else
                    hr = Win32CreateDirectoryW(pszPath, NULL) ? S_OK : E_FAIL;
                SHFree(pszPath);
            }
        }
    }
    return hr;
}

static HRESULT GetUIObjectFromVariant(VARIANT &vItem, REFIID riid, void **ppv)
{
    CComHeapPtr<ITEMIDLIST> pidlOneItem;
    if (SUCCEEDED(VariantToIdlist(&vItem, &pidlOneItem)))
        return SHELL_GetUIObjectOfAbsoluteItem(NULL, pidlOneItem, riid, ppv);

    // TODO: CFolderItems array
    return E_NOTIMPL;
}

HRESULT CFolder::CopyMoveOperation(VARIANT &vItem, VARIANT vOptions, BOOL bCopy)
{
    CComPtr<IShellFolder> psfCurrent;
    HRESULT hr = GetShellFolder(psfCurrent);
    if (FAILED(hr))
        return hr;

    CComPtr<IDropTarget> pDT;
    if (FAILED_UNEXPECTEDLY(hr = psfCurrent->CreateViewObject(GetHwnd(), IID_PPV_ARG(IDropTarget, &pDT))))
        return hr;

    CComPtr<IDataObject> pDO;
    if (FAILED_UNEXPECTEDLY(hr = GetUIObjectFromVariant(vItem, IID_PPV_ARG(IDataObject, &pDO))))
        return hr;

    if (SUCCEEDED(VariantChangeType(&vOptions, &vOptions, 0, VT_I4)))
    {
        UINT flags = V_I4(&vOptions) & ~(FOF_MULTIDESTFILES | FOF_WANTMAPPINGHANDLE);
        if (flags)
        {
            UINT cfFOF = RegisterClipboardFormatW(L"FileOpFlags");
            DataObj_SetDWORD(pDO, cfFOF, flags);
        }
    }
    hr = SH32_SimulateDropWithSite(pDT, pDO, MK_LBUTTON | (bCopy ? MK_CONTROL : MK_SHIFT), NULL, NULL, GetSite());
    return SUCCEEDED(hr) ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CFolder::MoveHere(VARIANT vItem, VARIANT vOptions)
{
    TRACE("(%p, %s, %s)\n", this, wine_dbgstr_variant(&vItem), wine_dbgstr_variant(&vOptions));
    return CopyMoveOperation(vItem, vOptions, FALSE);
}

HRESULT STDMETHODCALLTYPE CFolder::CopyHere(VARIANT vItem, VARIANT vOptions)
{
    TRACE("(%p, %s, %s)\n", this, wine_dbgstr_variant(&vItem), wine_dbgstr_variant(&vOptions));
    return CopyMoveOperation(vItem, vOptions, TRUE);
}

HRESULT STDMETHODCALLTYPE CFolder::GetDetailsOf(VARIANT vItem, int iColumn, BSTR *pbs)
{
    TRACE("(%p, %s, %i, %p)\n", this, wine_dbgstr_variant(&vItem), iColumn, pbs);

    CComPtr<IShellFolder> psf;
    HRESULT hr = GetShellFolder(psf);
    if (FAILED(hr))
        return hr;

    PCUITEMID_CHILD pidlItem = CFolderItem::GetLeafPidlRef(&vItem);
    if (pidlItem && iColumn == -1)
    {
        PWSTR pszTip = NULL;
        if (SUCCEEDED(hr = SHELL_QueryInfoTipAlloc(psf, QITIPF_DEFAULT, pidlItem, &pszTip)))
        {
            hr = SHELL_SysAllocString(pszTip, pbs);
            SHFree(pszTip);
            return hr;
        }
    }
    else
    {
        VARIANT v;
        if (pbs && SUCCEEDED(SHELL_GetDetailsOfAsStringVariant(psf, pidlItem, iColumn, &v)))
        {
            *pbs = V_BSTR(&v);
            return S_OK;
        }
    }
    return SHELL_SysAllocString(L"", pbs);
}


// *** Folder2 methods ***
HRESULT STDMETHODCALLTYPE CFolder::get_Self(FolderItem **ppfi)
{
    TRACE("(%p, %p)\n", this, ppfi);
    if (!ppfi)
        return E_POINTER;

    return ShellObjectCreatorInit<CFolderItem>(this, GetAbsoluteIDList(), IID_PPV_ARG(FolderItem, ppfi));
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


