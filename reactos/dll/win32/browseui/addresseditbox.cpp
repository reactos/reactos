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
    Add auto completion support
    Subclass windows in Init method
    Connect to browser connection point
    Handle navigation complete messages to set edit box text
    Handle listbox dropdown message and fill contents
    Add drag and drop of icon in edit box
    Handle enter in edit box to browse to typed path
    Handle change notifies to update appropriately
    Add handling of enter in edit box
    Fix so selection in combo listbox navigates
    Fix so editing text and typing enter navigates
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

HRESULT STDMETHODCALLTYPE CAddressEditBox::SetOwner(IUnknown *)
{
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

    CComPtr<IShellBrowser> pisb;
    hr = IUnknown_QueryService(fSite, SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, &pisb));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = IUnknown_GetWindow(pisb, &topLevelWindow);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    LPWSTR input;
    int inputLength = ::GetWindowTextLength(fCombobox.m_hWnd) + 2;

    input = new WCHAR[inputLength];
    ::GetWindowText(fCombobox.m_hWnd, input, inputLength);

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

    CComPtr<IShellFolder> psfDesktop;
    hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
    {
        hr = psfDesktop->ParseDisplayName(topLevelWindow, NULL, address, &eaten, &pidlLastParsed, &attributes);
    }

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
    if (pDispParams == NULL)
        return E_INVALIDARG;

    switch (dispIdMember)
    {
    case DISPID_NAVIGATECOMPLETE2:
    case DISPID_DOCUMENTCOMPLETE:
        pidlLastParsed = NULL;
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

HRESULT CreateAddressEditBox(REFIID riid, void **ppv)
{
    return ShellObjectCreator<CAddressEditBox>(riid, ppv);
}
