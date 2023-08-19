/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
 * Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
    Add drag and drop of icon in edit box
    Handle change notifies to update appropriately
*/

CAddressEditBox::CAddressEditBox() :
    fCombobox(WC_COMBOBOXEXW, this),
    fEditWindow(WC_EDITW, this),
    fSite(NULL),
    pidlLastParsed(NULL)
{
}

CAddressEditBox::~CAddressEditBox()
{
    if (pidlLastParsed)
        ILFree(pidlLastParsed);
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
    hComboBoxEx = comboboxEx;

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

BOOL CAddressEditBox::GetComboBoxText(CComHeapPtr<WCHAR>& pszText)
{
    pszText.Free();
    INT cchMax = fCombobox.GetWindowTextLength() + sizeof(UNICODE_NULL);
    if (!pszText.Allocate(cchMax))
        return FALSE;
    return fCombobox.GetWindowText(pszText, cchMax);
}

HRESULT CAddressEditBox::RefreshAddress()
{
    /* Get the current pidl of the browser */
    CComHeapPtr<ITEMIDLIST> absolutePIDL;
    HRESULT hr = GetAbsolutePidl(&absolutePIDL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    /* Fill the combobox */
    ATLASSERT(absolutePIDL != NULL);
    PopulateComboBox(absolutePIDL);

    /* Get pShellFolder and pidlChild */
    CComPtr<IShellFolder> pShellFolder;
    PCITEMID_CHILD pidlChild;
    hr = SHBindToParent(absolutePIDL, IID_PPV_ARG(IShellFolder, &pShellFolder), &pidlChild);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    /* Get ready to set the displayed item */
    COMBOBOXEXITEMW item = { CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_TEXT | CBEIF_LPARAM };
    item.iItem = -1; /* -1 to specify the displayed item */
    item.iImage = SHMapPIDLToSystemImageListIndex(pShellFolder, pidlChild, &item.iSelectedImage);

    /* Set the path if filesystem; otherwise use the name */
    WCHAR szPathOrName[MAX_PATH];
    SHGDNF flags = SHGDN_FORADDRESSBAR;
    if (gCabinetState.fFullPathAddress)
        flags |= SHGDN_FORPARSING;

    if (SUCCEEDED(IEGetNameAndFlags(absolutePIDL, flags, szPathOrName, _countof(szPathOrName), NULL)))
        item.pszText = szPathOrName;

    /* Ownership of absolutePIDL will be moved to fCombobox. See CBEN_DELETEITEM */
    item.lParam = reinterpret_cast<LPARAM>(absolutePIDL.Detach());

    fCombobox.SendMessage(CBEM_SETITEM, 0, reinterpret_cast<LPARAM>(&item)); /* Set it! */
    return S_OK;
}

HRESULT CAddressEditBox::GetAbsolutePidl(PIDLIST_ABSOLUTE *pAbsolutePIDL)
{
    CComPtr<IBrowserService> isb;
    HRESULT hr = IUnknown_QueryService(fSite, SID_STopLevelBrowser, IID_PPV_ARG(IBrowserService, &isb));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = isb->GetPidl(pAbsolutePIDL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

/* Execute command line from address bar */
BOOL CAddressEditBox::ExecuteCommandLine()
{
    /* Get command line */
    CComHeapPtr<WCHAR> pszCmdLine;
    if (!GetComboBoxText(pszCmdLine))
        return FALSE;

    /* Split 1st parameter from trailing arguments */
    PWCHAR args = PathGetArgsW(pszCmdLine);
    PathRemoveArgsW(pszCmdLine);

    PathUnquoteSpacesW(pszCmdLine); /* Unquote the 1st parameter */

    /* Get ready for execution */
    SHELLEXECUTEINFOW info = { sizeof(info), SEE_MASK_FLAG_NO_UI, m_hWnd };
    info.lpFile = pszCmdLine;
    info.lpParameters = args;
    info.nShow = SW_SHOWNORMAL;

    /* Set current directory */
    WCHAR dir[MAX_PATH] = L"";
    CComHeapPtr<ITEMIDLIST> pidl;
    if (SUCCEEDED(GetAbsolutePidl(&pidl)))
    {
        if (SHGetPathFromIDListW(pidl, dir) && PathIsDirectoryW(dir))
            info.lpDirectory = dir;
    }

    if (!::ShellExecuteExW(&info)) /* Execute! */
        return FALSE;

    RefreshAddress();
    return TRUE;
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
    CComHeapPtr<WCHAR> input, address;
    if (!GetComboBoxText(input))
        return E_FAIL;

    INT addressLength = (wcschr(input, L'%') ? ::SHExpandEnvironmentStringsW(input, NULL, 0) : 0);
    if (addressLength <= 0 ||
        !address.Allocate(addressLength + 1) ||
        !::SHExpandEnvironmentStringsW(input, address, addressLength))
    {
        address.Free();
        address.Attach(input.Detach());
    }

    /* Try to parse a relative path and if it fails, try to browse an absolute path */
    CComPtr<IShellFolder> psfDesktop;
    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        goto cleanup;

    hr = pbs->GetPidl(&pidlCurrent);
    if (FAILED_UNEXPECTEDLY(hr))
        goto parseabsolute;

    hr = psfDesktop->BindToObject(pidlCurrent, NULL, IID_PPV_ARG(IShellFolder, &psfCurrent));
    if (FAILED_UNEXPECTEDLY(hr))
        goto parseabsolute;

    hr = psfCurrent->ParseDisplayName(topLevelWindow, NULL, address, &eaten,  &pidlRelative, &attributes);
    if (SUCCEEDED(hr))
    {
        pidlLastParsed = ILCombine(pidlCurrent, pidlRelative);
        ILFree(pidlRelative);
        goto cleanup;
    }

parseabsolute:
    /* We couldn't parse a relative path, attempt to parse an absolute path */
    hr = psfDesktop->ParseDisplayName(topLevelWindow, NULL, address, &eaten, &pidlLastParsed, &attributes);

cleanup:
    if (pidlCurrent)
        ILFree(pidlCurrent);
    return hr;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::ShowFileNotFoundError(HRESULT hRet)
{
    CComHeapPtr<WCHAR> input;
    if (!GetComboBoxText(input))
        return E_FAIL;

    ShellMessageBoxW(_AtlBaseModule.GetResourceInstance(), fCombobox.m_hWnd, MAKEINTRESOURCEW(IDS_PARSE_ADDR_ERR_TEXT), MAKEINTRESOURCEW(IDS_PARSE_ADDR_ERR_TITLE), MB_OK | MB_ICONERROR, input.m_pData);

    return hRet;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Execute(long paramC)
{
    HRESULT hr;

    /*
     * Parse the path if it wasn't parsed
     */
    if (!pidlLastParsed)
    {
        hr = ParseNow(0);

        /* If the destination path doesn't exist then display an error message */
        if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            if (ExecuteCommandLine())
                return S_OK;

            return ShowFileNotFoundError(hr);
        }

        if (!pidlLastParsed)
            return E_FAIL;
    }

    /*
     * Get the IShellBrowser and IBrowserService interfaces of the shell browser
     */
    CComPtr<IShellBrowser> pisb;
    hr = IUnknown_QueryService(fSite, SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, &pisb));
    if (FAILED(hr))
        return hr;

    /*
     * Get the current pidl of the shellbrowser and check if it is the same with the parsed one
     */
    PIDLIST_ABSOLUTE pidl;
    hr = GetAbsolutePidl(&pidl);
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

    if (theResult)
        *theResult = 0;

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                UINT selectedIndex = SendMessageW((HWND)lParam, CB_GETCURSEL, 0, 0);
                pidlLastParsed = ILClone((LPITEMIDLIST)SendMessageW((HWND)lParam, CB_GETITEMDATA, selectedIndex, 0));
                Execute(0);
            }
            break;
        }
        case WM_NOTIFY:
        {
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
                    RefreshAddress();
                }
            }
            else if (hdr->code == CBEN_DELETEITEM)
            {
                PNMCOMBOBOXEX pCBEx = (PNMCOMBOBOXEX) lParam;
                LPITEMIDLIST itemPidl = (LPITEMIDLIST)pCBEx->ceItem.lParam;
                if (itemPidl)
                {
                    ILFree(itemPidl);
                }
            }
            break;
        }
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
        if (pidlLastParsed)
        {
            ILFree(pidlLastParsed);
            pidlLastParsed = NULL;
        }

        RefreshAddress();
        break;
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

void CAddressEditBox::PopulateComboBox(LPITEMIDLIST pidlCurrent)
{
    HRESULT hr;
    LPITEMIDLIST pidl;
    int indent = 0;
    int index;

    index = SendMessageW(hComboBoxEx, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < index; i++)
        SendMessageW(hComboBoxEx, CBEM_DELETEITEM, i, 0);
    SendMessageW(hComboBoxEx, CB_RESETCONTENT, 0, 0);

    /* Calculate the indent level. No need to clone the pidl */
    pidl = pidlCurrent;
    do
    {
        if(!pidl->mkid.cb)
            break;
        pidl = ILGetNext(pidl);
        indent++;
    } while (pidl);
    index = indent;

    /* Add every id from the pidl in the combo box */
    pidl = ILClone(pidlCurrent);
    do
    {
        AddComboBoxItem(pidl, 0, index);
        ILRemoveLastID(pidl);
        index--;
    } while (index >= 0);
    ILFree(pidl);

    /* Add the items of the desktop */
    FillOneLevel(0, 1, indent);

    /* Add the items of My Computer */
    hr = SHGetSpecialFolderLocation(0, CSIDL_DRIVES, &pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    for(LPITEMIDLIST i = GetItemData(0); i; i = GetItemData(index))
    {
        if (ILIsEqual(i, pidl))
        {
            FillOneLevel(index, 2, indent);
            break;
        }
        index++;
    }
    ILFree(pidl);
}

void CAddressEditBox::AddComboBoxItem(LPITEMIDLIST pidl, int index, int indent)
{
    HRESULT hr;
    WCHAR buf[4096];

    LPCITEMIDLIST pidlChild;
    CComPtr<IShellFolder> sf;
    hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &sf), &pidlChild);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    STRRET strret;
    hr = sf->GetDisplayNameOf(pidlChild, SHGDN_FORADDRESSBAR, &strret);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    hr = StrRetToBufW(&strret, pidlChild, buf, 4095);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    COMBOBOXEXITEMW item = {0};
    item.mask = CBEIF_LPARAM | CBEIF_INDENT | CBEIF_SELECTEDIMAGE | CBEIF_IMAGE | CBEIF_TEXT;
    item.iImage = SHMapPIDLToSystemImageListIndex(sf, pidlChild, &item.iSelectedImage);
    item.pszText = buf;
    item.lParam = (LPARAM)(ILClone(pidl));
    item.iIndent = indent;
    item.iItem = index;
    SendMessageW(hComboBoxEx, CBEM_INSERTITEMW, 0, (LPARAM)&item);
}

void CAddressEditBox::FillOneLevel(int index, int levelIndent, int indent)
{
    HRESULT hr;
    ULONG numObj;
    int count;
    LPITEMIDLIST pidl, pidl2, pidl3, pidl4;

    count = index + 1;
    pidl = GetItemData(index);
    pidl2 = GetItemData(count);
    if(pidl)
    {
        CComPtr<IShellFolder> psfDesktop;
        CComPtr<IShellFolder> psfItem;

        hr = SHGetDesktopFolder(&psfDesktop);
        if (FAILED_UNEXPECTEDLY(hr))
            return;

        if (!pidl->mkid.cb)
        {
            psfItem = psfDesktop;
        }
        else
        {
            hr = psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfItem));
            if (FAILED_UNEXPECTEDLY(hr))
                return;
        }

        CComPtr<IEnumIDList> pEnumIDList;
        hr = psfItem->EnumObjects(0, SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN, &pEnumIDList);
        if (FAILED_UNEXPECTEDLY(hr))
            return;

        do
        {
            hr = pEnumIDList->Next(1, &pidl3, &numObj);
            if(hr != S_OK || !numObj)
                break;

            pidl4 = ILCombine(pidl, pidl3);
            if (pidl2 && ILIsEqual(pidl4, pidl2))
                count += (indent - levelIndent);
            else
                AddComboBoxItem(pidl4, count, levelIndent);
            count++;
            ILFree(pidl3);
            ILFree(pidl4);
        } while (true);
    }
}

LPITEMIDLIST CAddressEditBox::GetItemData(int index)
{
    COMBOBOXEXITEMW item;

    memset(&item, 0, sizeof(COMBOBOXEXITEMW));
    item.mask = CBEIF_LPARAM;
    item.iItem = index;
    SendMessageW(hComboBoxEx, CBEM_GETITEMW, 0, (LPARAM)&item);
    return (LPITEMIDLIST)item.lParam;
}

LRESULT CAddressEditBox::OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    RefreshAddress();
    return NO_ERROR;
}
