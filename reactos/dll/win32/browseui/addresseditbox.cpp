/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
This class handles the combo box of the address band.
*/

#include "precomp.h"

/*
TODO:
    Handle listbox dropdown message and fill contents
    Add drag and drop of icon in edit box
    Handle change notifies to update appropriately
    Fix so selection in combo listbox navigates
*/

CAddressEditBox::CAddressEditBox() :
    fCombobox(NULL, this, 1),
    fEditWindow(NULL, this, 1),
    fSite(NULL)
{
}

CAddressEditBox::~CAddressEditBox()
{
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::SetOwner(IUnknown *pOwner)
{
    if (!pOwner)
    {
        CComPtr<IBrowserService> browserService;
        HRESULT hResult = IUnknown_QueryService(fSite, SID_SShellBrowser, IID_PPV_ARG(IBrowserService, &browserService));
        if (SUCCEEDED(hResult))
            AtlUnadvise(browserService, DIID_DWebBrowserEvents, fAdviseCookie);
        fSite = NULL; 
    }
    // connect to browser connection point
    return 0;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::FileSysChange(long param8, long paramC)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Refresh(long param8)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Init(HWND comboboxEx, HWND editControl, long param14, IUnknown *param18)
{
    CComPtr<IBrowserService> browserService;

    fCombobox.SubclassWindow(comboboxEx);
    fEditWindow.SubclassWindow(editControl);
    fSite = param18;

    SHAutoComplete(fEditWindow.m_hWnd, SHACF_FILESYSTEM | SHACF_URLALL | SHACF_USETAB);

    // take advice to watch events
    HRESULT hResult = IUnknown_QueryService(param18, SID_SShellBrowser, IID_PPV_ARG(IBrowserService, &browserService));
    if (SUCCEEDED(hResult))
    {
        hResult = AtlAdvise(browserService, static_cast<IDispatch *>(this), DIID_DWebBrowserEvents, &fAdviseCookie);
    }

    return hResult;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::SetCurrentDir(long paramC)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::ParseNow(long paramC)
{
    ULONG eaten;
    ULONG attributes;
    HRESULT hr;
    HWND topLevelWindow;
    PIDLIST_ABSOLUTE pidlCurrent= NULL;
    PIDLIST_RELATIVE pidlRelative = NULL;
    CComPtr<IShellFolder> psfCurrent;

    CComPtr<IBrowserService> pbs;
    hr = IUnknown_QueryService(fSite, SID_SShellBrowser, IID_PPV_ARG(IBrowserService, &pbs));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = IUnknown_GetWindow(pbs, &topLevelWindow);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    /* Get the path to browse and expand it if needed */
    LPWSTR input;
    int inputLength = fCombobox.GetWindowTextLength() + 2;

    input = new WCHAR[inputLength];
    fCombobox.GetWindowText(input, inputLength);

    LPWSTR address;
    int addressLength = ExpandEnvironmentStrings(input, NULL, 0);

    if (addressLength <= 0)
    {
        address = input;
    }
    else
    {
        addressLength += 2;
        address = new WCHAR[addressLength];
        if (!ExpandEnvironmentStrings(input, address, addressLength))
        {
            delete[] address;
            address = input;
        }
    }

    /* Try to parse a relative path and if it fails, try to browse an absolute path */
    CComPtr<IShellFolder> psfDesktop;
    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        goto cleanup;

    hr = pbs->GetPidl(&pidlCurrent);
    if (FAILED_UNEXPECTEDLY(hr))
        goto cleanup;

    hr = psfDesktop->BindToObject(pidlCurrent, NULL, IID_PPV_ARG(IShellFolder, &psfCurrent));
    if (FAILED_UNEXPECTEDLY(hr))
        goto cleanup;

    hr = psfCurrent->ParseDisplayName(topLevelWindow, NULL, address, &eaten,  &pidlRelative, &attributes);
    if (SUCCEEDED(hr))
    {
        pidlLastParsed = ILCombine(pidlCurrent, pidlRelative);
        ILFree(pidlRelative);
        goto cleanup;
    }

    /* We couldn't parse a relative path, attempt to parse an absolute path */
    hr = psfDesktop->ParseDisplayName(topLevelWindow, NULL, address, &eaten, &pidlLastParsed, &attributes);

cleanup:
    if (pidlCurrent)
        ILFree(pidlCurrent);
    if (address != input)
        delete [] address;
    delete [] input;

    return hr;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Execute(long paramC)
{
    HRESULT hr;

    /* 
     * Parse the path is it wasn't parsed
     */
    if (!pidlLastParsed)
        ParseNow(0);

    if (!pidlLastParsed)
        return E_FAIL;

    /* 
     * Get the IShellBrowser and IBrowserService interfaces of the shell browser 
     */
    CComPtr<IShellBrowser> pisb;
    hr = IUnknown_QueryService(fSite, SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, &pisb));
    if (FAILED(hr))
        return hr;

    CComPtr<IBrowserService> pbs;
    pisb->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs));
    if (FAILED(hr))
        return hr;

    /*
     * Get the current pidl of the shellbrowser and check if it is the same with the parsed one
     */
    PIDLIST_ABSOLUTE pidl;
    hr = pbs->GetPidl(&pidl);
    if (FAILED(hr))
        return hr;

    CComPtr<IShellFolder> psf;
    hr = SHGetDesktopFolder(&psf);
    if (FAILED(hr))
        return hr;

    hr = psf->CompareIDs(0, pidl, pidlLastParsed);

    SHFree(pidl);
    if (hr == 0)
        return S_OK;

    /* 
     * Attempt to browse to the parsed pidl 
     */
    hr = pisb->BrowseObject(pidlLastParsed, 0);
    if (SUCCEEDED(hr))
        return hr;

    /* 
     * Browsing to the pidl failed so it's not a folder. So invoke its defaule command.
     */
    HWND topLevelWindow;
    hr = IUnknown_GetWindow(pisb, &topLevelWindow);
    if (FAILED(hr))
        return hr;

    LPCITEMIDLIST pidlChild;
    CComPtr<IShellFolder> sf;
    hr = SHBindToParent(pidlLastParsed, IID_PPV_ARG(IShellFolder, &sf), &pidlChild);
    if (FAILED(hr))
        return hr;

    hr = SHInvokeDefaultCommand(topLevelWindow, sf, pidlChild);
    if (FAILED(hr))
        return hr;

    return hr;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Save(long paramC)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::OnWinEvent(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    LPNMHDR hdr;

    *theResult = 0;

    switch (uMsg)
    {
    case WM_NOTIFY:
        hdr = (LPNMHDR) lParam;
        if (hdr->code == CBEN_ENDEDIT)
        {
            NMCBEENDEDITW *endEdit = (NMCBEENDEDITW*) lParam;
            if (endEdit->iWhy == CBENF_RETURN)
            {
                Execute(0);
            }
            else if (endEdit->iWhy == CBENF_ESCAPE)
            {
                /* Reset the contents of the combo box */
            }
        }
        break;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::IsWindowOwner(HWND hWnd)
{
    if (fCombobox.m_hWnd == hWnd)
        return S_OK;
    if (fEditWindow.m_hWnd == hWnd)
        return S_OK;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::QueryStatus(
    const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::GetIDsOfNames(
    REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    CComPtr<IBrowserService> isb;
    CComPtr<IShellFolder> sf;
    HRESULT hr;
    INT indexClosed, indexOpen, itemExists, oldIndex;
    DWORD result;
    COMBOBOXEXITEMW item;
    PIDLIST_ABSOLUTE absolutePIDL;
    LPCITEMIDLIST pidlChild;
    LPITEMIDLIST pidlPrevious;
    STRRET ret;
    WCHAR buf[4096];

    if (pDispParams == NULL)
        return E_INVALIDARG;

    switch (dispIdMember)
    {
    case DISPID_NAVIGATECOMPLETE2:
    case DISPID_DOCUMENTCOMPLETE:
        pidlLastParsed = NULL;

        oldIndex = fCombobox.SendMessage(CB_GETCURSEL, 0, 0);

        itemExists = FALSE;
        pidlPrevious = NULL;

        ZeroMemory(&item, sizeof(item));
        item.mask = CBEIF_LPARAM;
        item.iItem = 0;
        if (fCombobox.SendMessage(CBEM_GETITEM, 0, reinterpret_cast<LPARAM>(&item)))
        {
            pidlPrevious = reinterpret_cast<LPITEMIDLIST>(item.lParam);
            if (pidlPrevious)
                itemExists = TRUE;
        }

        hr = IUnknown_QueryService(fSite, SID_STopLevelBrowser, IID_PPV_ARG(IBrowserService, &isb));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = isb->GetPidl(&absolutePIDL);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = SHBindToParent(absolutePIDL, IID_PPV_ARG(IShellFolder, &sf), &pidlChild);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = sf->GetDisplayNameOf(pidlChild, SHGDN_FORADDRESSBAR | SHGDN_FORPARSING, &ret);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = StrRetToBufW(&ret, pidlChild, buf, 4095);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        indexClosed = SHMapPIDLToSystemImageListIndex(sf, pidlChild, &indexOpen);

        item.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_TEXT | CBEIF_LPARAM;
        item.iItem = 0;
        item.iImage = indexClosed;
        item.iSelectedImage = indexOpen;
        item.pszText = buf;
        item.lParam = reinterpret_cast<LPARAM>(absolutePIDL);

        if (itemExists)
        {
            result = fCombobox.SendMessage(CBEM_SETITEM, 0, reinterpret_cast<LPARAM>(&item));
            oldIndex = 0;

            if (result)
            {
                ILFree(pidlPrevious);
            }
        }
        else
        {
            oldIndex = fCombobox.SendMessage(CBEM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&item));

            if (oldIndex < 0)
                DbgPrint("ERROR %d\n", GetLastError());
        }

        fCombobox.SendMessage(CB_SETCURSEL, -1, 0);
        fCombobox.SendMessage(CB_SETCURSEL, oldIndex, 0);

        //fAddressEditBox->SetCurrentDir(index);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::GetClassID(CLSID *pClassID)
{
    if (pClassID == NULL)
        return E_POINTER;
    *pClassID = CLSID_AddressEditBox;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::IsDirty()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Load(IStream *pStm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Save(IStream *pStm, BOOL fClearDirty)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    return E_NOTIMPL;
}
