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
Implements the navigation band of the cabinet window
*/

#include "precomp.h"
#include <commoncontrols.h>
#include <shlwapi_undoc.h>
#include <shellapi.h>

HRESULT CreateAddressEditBox(REFIID riid, void **ppv);

extern "C"
HRESULT WINAPI SHGetImageList(
    _In_   int iImageList,
    _In_   REFIID riid,
    _Out_  void **ppv
    );

/*
TODO:
****Add command handler for show/hide Go button to OnWinEvent
****Add tooltip notify handler
  **Properly implement GetBandInfo
  **Add support for showing/hiding Go button
  **Fix so Go button will be shown/hidden properly on load
  **Add correct text to Go button
  **Implement TranslateAcceleratorIO
    Implement Exec
    Implement QueryService
    Implement Load
    Implement Save
*/

CAddressBand::CAddressBand()
{
    fEditControl = NULL;
    fGoButton = NULL;
    fComboBox = NULL;
    fGoButtonShown = false;
    fAdviseCookie = 0;
}

CAddressBand::~CAddressBand()
{
}

void CAddressBand::FocusChange(BOOL bFocus)
{
//    m_bFocus = bFocus;

    //Inform the input object site that the focus has changed.
    if (fSite)
    {
#if 0
        fSite->OnFocusChangeIS((IDockingWindow *)this, bFocus);
#endif
    }
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        pdbi->ptMinSize.x = 400;
        pdbi->ptMinSize.y = 22;
    }
    if (pdbi->dwMask & DBIM_MAXSIZE)
    {
        pdbi->ptMaxSize.x = 0;
        pdbi->ptMaxSize.y = 0;
    }
    if (pdbi->dwMask & DBIM_INTEGRAL)
    {
        pdbi->ptIntegral.x = 0;
        pdbi->ptIntegral.y = 0;
    }
    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        pdbi->ptActual.x = 400;
        pdbi->ptActual.y = 22;
    }
    if (pdbi->dwMask & DBIM_TITLE)
        wcscpy(pdbi->wszTitle, L"Address");
    if (pdbi->dwMask & DBIM_MODEFLAGS)
        pdbi->dwModeFlags = DBIMF_UNDELETEABLE;
    if (pdbi->dwMask & DBIM_BKCOLOR)
        pdbi->crBkgnd = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::SetSite(IUnknown *pUnkSite)
{
    CComPtr<IBrowserService>                browserService;
    CComPtr<IShellService>                  shellService;
    HWND                                    parentWindow;
    HWND                                    combobox;
    HRESULT                                 hResult;

    if (pUnkSite == NULL)
    {
        hResult = AtlUnadvise(fSite, DIID_DWebBrowserEvents, fAdviseCookie);
        fSite.Release();
        return S_OK;
    }

    fSite.Release();

    hResult = pUnkSite->QueryInterface(IID_PPV_ARG(IDockingWindowSite, &fSite));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    // get window handle of parent
    parentWindow = NULL;
    hResult = IUnknown_GetWindow(fSite, &parentWindow);

    if (!::IsWindow(parentWindow))
        return E_FAIL;

    // create combo box ex
    combobox = CreateWindowEx(WS_EX_TOOLWINDOW, WC_COMBOBOXEXW, NULL, WS_CHILD | WS_VISIBLE |
        WS_CLIPCHILDREN | WS_TABSTOP | CCS_NODIVIDER | CCS_NOMOVEY | CBS_OWNERDRAWFIXED,
                    0, 0, 500, 250, parentWindow, (HMENU)0xa205, _AtlBaseModule.GetModuleInstance(), 0);
    if (combobox == NULL)
        return E_FAIL;
    SubclassWindow(combobox);

    SendMessage(CBEM_SETEXTENDEDSTYLE,
        CBES_EX_CASESENSITIVE | CBES_EX_NOSIZELIMIT, CBES_EX_CASESENSITIVE | CBES_EX_NOSIZELIMIT);

    fEditControl = reinterpret_cast<HWND>(SendMessage(CBEM_GETEDITCONTROL, 0, 0));
    fComboBox = reinterpret_cast<HWND>(SendMessage(CBEM_GETCOMBOCONTROL, 0, 0));
#if 1
    hResult = CoCreateInstance(CLSID_AddressEditBox, NULL, CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(IAddressEditBox, &fAddressEditBox));
#else
    hResult = E_FAIL;
#endif
    if (FAILED_UNEXPECTEDLY(hResult))
    {
        // instantiate new version
        hResult = CreateAddressEditBox(IID_PPV_ARG(IAddressEditBox, &fAddressEditBox));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
    }

    hResult = fAddressEditBox->QueryInterface(IID_PPV_ARG(IShellService, &shellService));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = fAddressEditBox->Init(combobox, fEditControl, 8, fSite /*(IAddressBand *)this*/);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = shellService->SetOwner(fSite);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    // TODO: properly initialize this from registry
    fGoButtonShown = true;

    if (fGoButtonShown)
    {
        const TBBUTTON buttonInfo [] = { { 0, 1, TBSTATE_ENABLED, 0 } };
        HIMAGELIST            normalImagelist;
        HIMAGELIST            hotImageList;
        HINSTANCE             shellInstance;

        shellInstance = GetModuleHandle(_T("shell32.dll"));
        normalImagelist = ImageList_LoadImageW(shellInstance, MAKEINTRESOURCE(IDB_GOBUTTON_NORMAL),
            20, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);
        hotImageList = ImageList_LoadImageW(shellInstance, MAKEINTRESOURCE(IDB_GOBUTTON_HOT),
            20, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);

        fGoButton = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAMEW, 0, WS_CHILD | WS_CLIPSIBLINGS |
            WS_CLIPCHILDREN | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NODIVIDER |
            CCS_NOPARENTALIGN | CCS_NORESIZE,
            0, 0, 0, 0, m_hWnd, NULL, _AtlBaseModule.GetModuleInstance(), NULL);
        SendMessage(fGoButton, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        SendMessage(fGoButton, TB_SETMAXTEXTROWS, 1, 0);
        if (normalImagelist)
            SendMessage(fGoButton, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(normalImagelist));
        if (hotImageList)
            SendMessage(fGoButton, TB_SETHOTIMAGELIST, 0, reinterpret_cast<LPARAM>(hotImageList));
        SendMessage(fGoButton, TB_ADDSTRINGW,
            reinterpret_cast<WPARAM>(_AtlBaseModule.GetResourceInstance()), IDS_GOBUTTONLABEL);
        SendMessage(fGoButton, TB_ADDBUTTONSW, 1, (LPARAM) &buttonInfo);

        IImageList * piml;
        HRESULT hr = SHGetImageList(SHIL_SMALL, IID_PPV_ARG(IImageList, &piml));
        if (FAILED_UNEXPECTEDLY(hr))
        {
            SendMessageW(combobox, CBEM_SETIMAGELIST, 0, 0);
        }
        else
        {
            SendMessageW(combobox, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(piml));
        }
    }

    // take advice to watch events
    hResult = IUnknown_QueryService(pUnkSite, SID_SShellBrowser, IID_PPV_ARG(IBrowserService, &browserService));
    if (SUCCEEDED(hResult))
    {
        hResult = AtlAdvise(browserService, static_cast<IDispatch *>(this), DIID_DWebBrowserEvents, &fAdviseCookie);
    }

    return hResult;
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetSite(REFIID riid, void **ppvSite)
{
    if (fSite == NULL)
        return E_FAIL;
    return fSite->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetWindow(HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::CloseDW(DWORD dwReserved)
{
    ShowDW(FALSE);

    if (IsWindow())
        DestroyWindow();

    m_hWnd = NULL;

    IUnknown_SetSite(fAddressEditBox, NULL);

    if (fAddressEditBox) fAddressEditBox.Release();
    if (fSite) fSite.Release();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::ResizeBorderDW(
    const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::ShowDW(BOOL fShow)
{
    if (m_hWnd)
    {
        if (fShow)
            ShowWindow(SW_SHOW);
        else
            ShowWindow(SW_HIDE);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::QueryStatus(
    const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
    CComPtr<IOleCommandTarget>              oleCommandTarget;
    HRESULT                                 hResult;

    hResult = fAddressEditBox->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &oleCommandTarget));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return oleCommandTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
}

HRESULT STDMETHODCALLTYPE CAddressBand::Exec(const GUID *pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    // incomplete
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::HasFocusIO()
{
    if (GetFocus() == fEditControl || SendMessage(CB_GETDROPPEDSTATE, 0, 0))
        return S_OK;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CAddressBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    if (lpMsg->hwnd == fEditControl)
    {
        switch (lpMsg->message)
        {
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSCOMMAND:
        case WM_SYSDEADCHAR:
        case WM_SYSCHAR:
            return S_FALSE;
        }

        TranslateMessage(lpMsg);
        DispatchMessage(lpMsg);
        return S_OK;
    }
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CAddressBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    CComPtr<IInputObjectSite>               inputObjectSite;
    HRESULT                                 hResult;

    if (fActivate)
    {
        hResult = fSite->QueryInterface(IID_PPV_ARG(IInputObjectSite, &inputObjectSite));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        hResult = inputObjectSite->OnFocusChangeIS(static_cast<IDeskBand *>(this), fActivate);
        SetFocus();
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::OnWinEvent(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    CComPtr<IWinEventHandler>               winEventHandler;
    HRESULT                                 hResult;

    *theResult = 0;

    switch (uMsg)
    {
        case WM_WININICHANGE:
            break;
        case WM_COMMAND:
            if (wParam == IDM_TOOLBARS_GOBUTTON)
            {
                // toggle whether the Go button is displayed
                // setting is Yes or No, stored in key "Software\Microsoft\Internet Explorer\Main" in value ShowGoButton
                // broadcast change notification to all explorer windows
            }
            break;
    }
    hResult = fAddressEditBox->QueryInterface(IID_PPV_ARG(IWinEventHandler, &winEventHandler));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return winEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
}

HRESULT STDMETHODCALLTYPE CAddressBand::IsWindowOwner(HWND hWnd)
{
    CComPtr<IWinEventHandler>               winEventHandler;
    HRESULT                                 hResult;

    if (fAddressEditBox)
    {
        hResult = fAddressEditBox->QueryInterface(IID_PPV_ARG(IWinEventHandler, &winEventHandler));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        return winEventHandler->IsWindowOwner(hWnd);
    }
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CAddressBand::FileSysChange(long param8, long paramC)
{
    CComPtr<IAddressBand>                   addressBand;
    HRESULT                                 hResult;

    hResult = fAddressEditBox->QueryInterface(IID_PPV_ARG(IAddressBand, &addressBand));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return addressBand->FileSysChange(param8, paramC);
}

HRESULT STDMETHODCALLTYPE CAddressBand::Refresh(long param8)
{
    CComPtr<IAddressBand>                   addressBand;
    HRESULT                                 hResult;

    hResult = fAddressEditBox->QueryInterface(IID_PPV_ARG(IAddressBand, &addressBand));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return addressBand->Refresh(param8);
}

HRESULT STDMETHODCALLTYPE CAddressBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetClassID(CLSID *pClassID)
{
    if (pClassID == NULL)
        return E_POINTER;
    *pClassID = CLSID_SH_AddressBand;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::IsDirty()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::Load(IStream *pStm)
{
    // incomplete
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::Save(IStream *pStm, BOOL fClearDirty)
{
    // incomplete
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    // incomplete
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
    LCID lcid, DISPID *rgDispId)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
    DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
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

        oldIndex = SendMessage(m_hWnd, CB_GETCURSEL, 0, 0);

        itemExists = FALSE;
        pidlPrevious = NULL;

        ZeroMemory(&item, sizeof(item));
        item.mask = CBEIF_LPARAM;
        item.iItem = 0;
        if (SendMessage(m_hWnd, CBEM_GETITEM, 0, reinterpret_cast<LPARAM>(&item)))
        {
            pidlPrevious = reinterpret_cast<LPITEMIDLIST>(item.lParam);
            if (pidlPrevious)
                itemExists = TRUE;
        }

        hr = IUnknown_QueryService(fSite, SID_STopLevelBrowser, IID_PPV_ARG(IBrowserService, &isb));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
        isb->GetPidl(&absolutePIDL);

        SHBindToParent(absolutePIDL, IID_PPV_ARG(IShellFolder, &sf), &pidlChild);

        sf->GetDisplayNameOf(pidlChild, SHGDN_FORADDRESSBAR | SHGDN_FORPARSING, &ret);

        StrRetToBufW(&ret, pidlChild, buf, 4095);

        indexClosed = SHMapPIDLToSystemImageListIndex(sf, pidlChild, &indexOpen);

        item.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_TEXT | CBEIF_LPARAM;
        item.iItem = 0;
        item.iImage = indexClosed;
        item.iSelectedImage = indexOpen;
        item.pszText = buf;
        item.lParam = reinterpret_cast<LPARAM>(absolutePIDL);

        if (itemExists)
        {
            result = SendMessage(m_hWnd, CBEM_SETITEM, 0, reinterpret_cast<LPARAM>(&item));
            oldIndex = 0;

            if (result)
            {
                ILFree(pidlPrevious);
            }
        }
        else
        {
            oldIndex = SendMessage(m_hWnd, CBEM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&item));

            if (oldIndex < 0)
                DbgPrint("ERROR %d\n", GetLastError());
        }

        SendMessage(m_hWnd, CB_SETCURSEL, -1, 0);
        SendMessage(m_hWnd, CB_SETCURSEL, oldIndex, 0);

        //fAddressEditBox->SetCurrentDir(index);

        break;
    }
    return S_OK;
}

LRESULT CAddressBand::OnNotifyClick(WPARAM wParam, NMHDR *notifyHeader, BOOL &bHandled)
{
    if (notifyHeader->hwndFrom == fGoButton)
    {
        fAddressEditBox->Execute(0);
    }
    return 0;
}

LRESULT CAddressBand::OnTipText(UINT idControl, NMHDR *notifyHeader, BOOL &bHandled)
{
    if (notifyHeader->hwndFrom == fGoButton)
    {
        // TODO
        // Go to "destination path"
    }
    return 0;
}

LRESULT CAddressBand::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    POINT                                   pt;
    POINT                                   ptOrig;
    HWND                                    parentWindow;
    LRESULT                                 result;

    if (fGoButtonShown == false)
    {
        bHandled = FALSE;
        return 0;
    }
    pt.x = 0;
    pt.y = 0;
    parentWindow = GetParent();
    ::MapWindowPoints(m_hWnd, parentWindow, &pt, 1);
    OffsetWindowOrgEx(reinterpret_cast<HDC>(wParam), pt.x, pt.y, &ptOrig);
    result = SendMessage(parentWindow, WM_ERASEBKGND, wParam, 0);
    SetWindowOrgEx(reinterpret_cast<HDC>(wParam), ptOrig.x, ptOrig.y, NULL);
    if (result == 0)
    {
        bHandled = FALSE;
        return 0;
    }
    return result;
}

LRESULT CAddressBand::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    RECT                                    goButtonBounds;
    RECT                                    buttonBounds;
    long                                    buttonWidth;
    long                                    buttonHeight;
    RECT                                    comboBoxBounds;
    long                                    newHeight;
    long                                    newWidth;

    if (fGoButtonShown == false)
    {
        bHandled = FALSE;
        return 0;
    }

    newHeight = HIWORD(lParam);
    newWidth = LOWORD(lParam);

    SendMessage(fGoButton, TB_GETITEMRECT, 0, reinterpret_cast<LPARAM>(&buttonBounds));
    buttonWidth = buttonBounds.right - buttonBounds.left;
    buttonHeight = buttonBounds.bottom - buttonBounds.top;

    DefWindowProc(WM_SIZE, wParam, MAKELONG(newWidth - buttonWidth - 2, newHeight));
    ::GetWindowRect(fComboBox, &comboBoxBounds);
    ::SetWindowPos(fGoButton, NULL, newWidth - buttonWidth, (comboBoxBounds.bottom - comboBoxBounds.top - buttonHeight) / 2,
                    buttonWidth, buttonHeight, SWP_NOOWNERZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);

    goButtonBounds.left = newWidth - buttonWidth;
    goButtonBounds.top = 0;
    goButtonBounds.right = newWidth - buttonWidth;
    goButtonBounds.bottom = newHeight;
    InvalidateRect(&goButtonBounds, TRUE);

    SendMessage(fComboBox, CB_SETDROPPEDWIDTH, 200, 0);
    return 0;
}

LRESULT CAddressBand::OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    RECT                                    goButtonBounds;
    RECT                                    buttonBounds;
    long                                    buttonWidth;
    long                                    buttonHeight;
    RECT                                    comboBoxBounds;
    WINDOWPOS                               positionInfoCopy;
    long                                    newHeight;
    long                                    newWidth;

    if (!fGoButtonShown)
    {
        bHandled = FALSE;
        return 0;
    }

    positionInfoCopy = *reinterpret_cast<WINDOWPOS *>(lParam);
    newHeight = positionInfoCopy.cy;
    newWidth = positionInfoCopy.cx;
    SendMessage(fGoButton, TB_GETITEMRECT, 0, reinterpret_cast<LPARAM>(&buttonBounds));

    buttonWidth = buttonBounds.right - buttonBounds.left;
    buttonHeight = buttonBounds.bottom - buttonBounds.top;
    positionInfoCopy.cx = newWidth - 2 - buttonWidth;
    DefWindowProc(WM_WINDOWPOSCHANGING, wParam, reinterpret_cast<LPARAM>(&positionInfoCopy));
    ::GetWindowRect(fComboBox, &comboBoxBounds);
    ::SetWindowPos(fGoButton, NULL, newWidth - buttonWidth, (comboBoxBounds.bottom - comboBoxBounds.top - buttonHeight) / 2,
                    buttonWidth, buttonHeight, SWP_NOOWNERZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);

    goButtonBounds.left = newWidth - buttonWidth;
    goButtonBounds.top = 0;
    goButtonBounds.right = newWidth - buttonWidth;
    goButtonBounds.bottom = newHeight;
    InvalidateRect(&goButtonBounds, TRUE);

    SendMessage(fComboBox, CB_SETDROPPEDWIDTH, 200, 0);
    return 0;
}

HRESULT CreateAddressBand(REFIID riid, void **ppv)
{
    return ShellObjectCreator<CAddressBand>(riid, ppv);
}
