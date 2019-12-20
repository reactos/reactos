/*
 * PROJECT:     ReactOS Search Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Search UI
 * COPYRIGHT:   Copyright 2019 Brock Mammen
 */

#include "CSearchBar.h"
#include <psdk/wingdi.h>
#include <commoncontrols.h>
#include <../browseui.h>
#include <shellapi.h>
#include <exdispid.h>

WINE_DEFAULT_DEBUG_CHANNEL(shellfind);

#if 1
#undef UNIMPLEMENTED

#define UNIMPLEMENTED DbgPrint("%s is UNIMPLEMENTED!\n", __FUNCTION__)
#endif

CSearchBar::CSearchBar() :
    m_pSite(NULL),
    m_bVisible(FALSE)
{
}

CSearchBar::~CSearchBar()
{
}

LRESULT CSearchBar::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HKEY hkey;
    DWORD dwType;
    DWORD size = sizeof(DWORD);
    DWORD result;
    DWORD SearchHiddenValue = 0;

    result = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", 0, KEY_QUERY_VALUE, &hkey);
    if (result == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkey, L"SearchHidden", NULL, &dwType, (LPBYTE)&SearchHiddenValue, &size) == ERROR_SUCCESS)
        {
            if ((dwType != REG_DWORD) || (size != sizeof(DWORD)))
            {
                ERR("RegQueryKey for \"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SearchHidden\" returned error(s).\n");
                SearchHiddenValue = 1;
            }
            else
            {
                TRACE("SearchHidden is '%d'.\n", SearchHiddenValue);
            }
        }
        else
        {
            ERR("RegQueryKey for \"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SearchHidden\" Failed.\n");
        }
        RegCloseKey(hkey);
    }
    else
        ERR("RegOpenKey for \"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\" Failed.\n");

    if (SearchHiddenValue != 0)
        CheckDlgButton(IDC_SEARCH_HIDDEN, BST_CHECKED);
    else
        CheckDlgButton(IDC_SEARCH_HIDDEN, BST_UNCHECKED);

    SetSearchInProgress(FALSE);

    HWND hCombobox = GetDlgItem(IDC_SEARCH_COMBOBOX);
    CComPtr<IImageList> pImageList;
    HRESULT hResult = SHGetImageList(SHIL_SMALL, IID_PPV_ARG(IImageList, &pImageList));
    SendMessage(hCombobox, CBEM_SETIMAGELIST, 0, FAILED_UNEXPECTEDLY(hResult) ? 0 : reinterpret_cast<LPARAM>(pImageList.p));

    SendMessage(hCombobox, CBEM_SETEXTENDEDSTYLE,
        CBES_EX_CASESENSITIVE | CBES_EX_NOSIZELIMIT, CBES_EX_CASESENSITIVE | CBES_EX_NOSIZELIMIT);
    HWND hEditControl = reinterpret_cast<HWND>(SendMessage(hCombobox, CBEM_GETEDITCONTROL, 0, 0));
    hResult = CAddressEditBox_CreateInstance(IID_PPV_ARG(IAddressEditBox, &m_AddressEditBox));
    if (FAILED_UNEXPECTEDLY(hResult))
        return FALSE;

    hResult = m_AddressEditBox->Init(hCombobox, hEditControl, 0, m_pSite);
    if (FAILED_UNEXPECTEDLY(hResult))
        return FALSE;

    // Subscribe to navigation events
    CComPtr<IShellBrowser> pShellBrowser;
    hResult = IUnknown_QueryService(m_pSite, SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, &pShellBrowser));
    DWORD dwAdviseCookie;
    if (SUCCEEDED(hResult))
        AtlAdvise(pShellBrowser, static_cast<IDispatch *>(this), DIID_DWebBrowserEvents, &dwAdviseCookie);

    // Invoke the navigate event in case a search results folder is already open
    DISPPARAMS params = {0};
    Invoke(DISPID_NAVIGATECOMPLETE2, GUID_NULL, 0, DISPATCH_METHOD, &params, NULL, NULL, NULL);

    return TRUE;
}


// *** ATL event handlers ***
LRESULT CSearchBar::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    IUnknown_OnFocusChangeIS(m_pSite, static_cast<IDeskBand *>(this), TRUE);
    bHandled = FALSE;
    return TRUE;
}

HRESULT CSearchBar::GetSearchResultsFolder(IShellBrowser **ppShellBrowser, HWND *pHwnd, IShellFolder **ppShellFolder)
{
    HRESULT hr;
    CComPtr<IShellBrowser> pShellBrowser;
    if (!ppShellBrowser)
        ppShellBrowser = &pShellBrowser;
    if (!*ppShellBrowser)
    {
        hr = IUnknown_QueryService(m_pSite, SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, ppShellBrowser));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    CComPtr<IShellView> pShellView;
    hr = (*ppShellBrowser)->QueryActiveShellView(&pShellView);
    if (FAILED(hr) || !pShellView)
        return hr;

    CComPtr<IFolderView> pFolderView;
    hr = pShellView->QueryInterface(IID_PPV_ARG(IFolderView, &pFolderView));
    if (FAILED(hr) || !pFolderView)
        return hr;

    CComPtr<IShellFolder> pShellFolder;
    if (!ppShellFolder)
        ppShellFolder = &pShellFolder;
    hr = pFolderView->GetFolder(IID_PPV_ARG(IShellFolder, ppShellFolder));
    if (FAILED(hr) || !pShellFolder)
        return hr;

    CLSID clsid;
    hr = IUnknown_GetClassID(*ppShellFolder, &clsid);
    if (FAILED(hr))
        return hr;
    if (clsid != CLSID_FindFolder)
        return E_FAIL;

    if (pHwnd)
    {
        hr = pShellView->GetWindow(pHwnd);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    return S_OK;
}

LRESULT CSearchBar::OnSearchButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    size_t len = 0;
    WCHAR endchar;
    WCHAR startchar;

    CComHeapPtr<SearchStart> pSearchStart(static_cast<SearchStart *>(CoTaskMemAlloc(sizeof(SearchStart))));
    GetDlgItemText(IDC_SEARCH_FILENAME, pSearchStart->szFileName, _countof(pSearchStart->szFileName));
    GetDlgItemText(IDC_SEARCH_QUERY, pSearchStart->szQuery, _countof(pSearchStart->szQuery));

    pSearchStart->SearchHidden = IsDlgButtonChecked(IDC_SEARCH_HIDDEN);

    if (!GetAddressEditBoxPath(pSearchStart->szPath))
    {
        ShellMessageBoxW(_AtlBaseModule.GetResourceInstance(), m_hWnd, MAKEINTRESOURCEW(IDS_SEARCHINVALID), MAKEINTRESOURCEW(IDS_SEARCHLABEL), MB_OK | MB_ICONERROR, pSearchStart->szPath);
        return 0;
    }

    // See if we have an szFileName by testing for its entry lenth > 0 and our searched FileName does not contain
    // an asterisk or a question mark. If so, then prepend and append an asterisk to the searched FileName.
    // (i.e. it's equivalent to searching for *<the_file_name>* )
    if (FAILED (StringCchLengthW (pSearchStart->szFileName, MAX_PATH, &len))) return 0;
    if ((len > 0) && !wcspbrk(pSearchStart->szFileName, L"*?"))
    {
        endchar = pSearchStart->szFileName[len - 1];
        startchar = pSearchStart->szFileName[0];
        if ((len < MAX_PATH - 1) && (startchar != L'*'))
        {
            memmove(&pSearchStart->szFileName[1], &pSearchStart->szFileName[0],
                   len * sizeof(WCHAR) + sizeof(WCHAR));
            len = len + 1;
            pSearchStart->szFileName[0] = L'*';
        }

        // See if our last character is an asterisk and if not and we have room then add one
        if ((len < MAX_PATH - 1) && (endchar != L'*'))
            StringCchCatW(pSearchStart->szFileName, MAX_PATH, L"*");
    }

    // Print our final search string for szFileName
    TRACE("Searched szFileName is '%S'.\n", pSearchStart->szFileName);

    CComPtr<IShellBrowser> pShellBrowser;
    HRESULT hr = IUnknown_QueryService(m_pSite, SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, &pShellBrowser));
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    HWND hwnd;
    if (FAILED(GetSearchResultsFolder(&pShellBrowser, &hwnd, NULL)))
    {
        // Open a new search results folder
        WCHAR szShellGuid[MAX_PATH];
        const WCHAR shellGuidPrefix[] = L"shell:::";
        memcpy(szShellGuid, shellGuidPrefix, sizeof(shellGuidPrefix));
        hr = StringFromGUID2(CLSID_FindFolder, szShellGuid + _countof(shellGuidPrefix) - 1,
                             _countof(szShellGuid) - _countof(shellGuidPrefix));
        if (FAILED_UNEXPECTEDLY(hr))
            return 0;

        CComHeapPtr<ITEMIDLIST> findFolderPidl;
        hr = SHParseDisplayName(szShellGuid, NULL, &findFolderPidl, 0, NULL);
        if (FAILED_UNEXPECTEDLY(hr))
            return 0;

        hr = pShellBrowser->BrowseObject(findFolderPidl, 0);
        if (FAILED_UNEXPECTEDLY(hr))
            return 0;

        hr = GetSearchResultsFolder(&pShellBrowser, &hwnd, NULL);
        if (FAILED_UNEXPECTEDLY(hr))
            return 0;
    }

    ::PostMessageW(hwnd, WM_SEARCH_START, 0, (LPARAM) pSearchStart.Detach());

    SetSearchInProgress(TRUE);

    return 0;
}

LRESULT CSearchBar::OnStopButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HWND hwnd;
    HRESULT hr = GetSearchResultsFolder(NULL, &hwnd, NULL);
    if (SUCCEEDED(hr))
        ::PostMessageW(hwnd, WM_SEARCH_STOP, 0, 0);

    return 0;
}

BOOL CSearchBar::GetAddressEditBoxPath(WCHAR *szPath)
{
    HWND hComboboxEx = GetDlgItem(IDC_SEARCH_COMBOBOX);
    ::GetWindowTextW(hComboboxEx, szPath, MAX_PATH);
    INT iSelectedIndex = SendMessageW(hComboboxEx, CB_GETCURSEL, 0, 0);
    if (iSelectedIndex != CB_ERR)
    {
        WCHAR szItemText[MAX_PATH];
        COMBOBOXEXITEMW item = {0};
        item.mask = CBEIF_LPARAM | CBEIF_TEXT;
        item.iItem = iSelectedIndex;
        item.pszText = szItemText;
        item.cchTextMax = _countof(szItemText);
        SendMessageW(hComboboxEx, CBEM_GETITEMW, 0, (LPARAM)&item);

        if (!wcscmp(szItemText, szPath) && SHGetPathFromIDListW((LPCITEMIDLIST)item.lParam, szItemText))
        {
            StringCbCopyW(szPath, MAX_PATH * sizeof(WCHAR), szItemText);
            return TRUE;
        }
    }

    DWORD dwAttributes = GetFileAttributesW(szPath);
    return dwAttributes != INVALID_FILE_ATTRIBUTES
        && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

LRESULT CSearchBar::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    INT iWidth = LOWORD(lParam);
    INT iPadding = 10;

    ((CWindow)GetDlgItem(IDC_SEARCH_LABEL)).SetWindowPos(NULL, 0, 0, iWidth - iPadding, 40, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);

    int inputs[] = { IDC_SEARCH_FILENAME, IDC_SEARCH_QUERY, IDC_SEARCH_COMBOBOX, IDC_SEARCH_BUTTON, IDC_SEARCH_STOP_BUTTON, IDC_PROGRESS_BAR };
    HDWP hdwp = BeginDeferWindowPos(_countof(inputs));
    for (SIZE_T i = 0; i < _countof(inputs); i++)
    {
        CWindow wnd = (CWindow) GetDlgItem(inputs[i]);
        RECT rect;
        wnd.GetWindowRect(&rect);
        POINT pt = { rect.left, rect.top };
        ScreenToClient(&pt);
        hdwp = wnd.DeferWindowPos(hdwp,
                                  HWND_TOP,
                                  iPadding,
                                  pt.y,
                                  iWidth - iPadding * 2,
                                  rect.bottom - rect.top,
                                  SWP_NOZORDER | SWP_NOACTIVATE);
    }
    EndDeferWindowPos(hdwp);

    return 0;
}


// *** IOleWindow methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::GetWindow(HWND *lphwnd)
{
    if (!lphwnd)
        return E_INVALIDARG;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSearchBar::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IDockingWindow methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::CloseDW(DWORD dwReserved)
{
    // We do nothing, we don't have anything to save yet
    TRACE("CloseDW called\n");
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSearchBar::ResizeBorderDW(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    /* Must return E_NOTIMPL according to MSDN */
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::ShowDW(BOOL fShow)
{
    m_bVisible = fShow;
    ShowWindow(fShow);
    return S_OK;
}


// *** IDeskBand methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
    if (!pdbi)
    {
        return E_INVALIDARG;
    }

    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        pdbi->ptMinSize.x = 200;
        pdbi->ptMinSize.y = 30;
    }

    if (pdbi->dwMask & DBIM_MAXSIZE)
    {
        pdbi->ptMaxSize.y = -1;
    }

    if (pdbi->dwMask & DBIM_INTEGRAL)
    {
        pdbi->ptIntegral.y = 1;
    }

    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        pdbi->ptActual.x = 200;
        pdbi->ptActual.y = 30;
    }

    if (pdbi->dwMask & DBIM_TITLE)
    {
        if (!LoadStringW(_AtlBaseModule.GetResourceInstance(), IDS_SEARCHLABEL, pdbi->wszTitle, _countof(pdbi->wszTitle)))
            return HRESULT_FROM_WIN32(GetLastError());
    }

    if (pdbi->dwMask & DBIM_MODEFLAGS)
    {
        pdbi->dwModeFlags = DBIMF_NORMAL | DBIMF_VARIABLEHEIGHT;
    }

    if (pdbi->dwMask & DBIM_BKCOLOR)
    {
        pdbi->dwMask &= ~DBIM_BKCOLOR;
    }
    return S_OK;
}


// *** IObjectWithSite methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::SetSite(IUnknown *pUnkSite)
{
    HRESULT hr;
    HWND parentWnd;

    if (pUnkSite == m_pSite)
        return S_OK;

    TRACE("SetSite called \n");
    if (!pUnkSite)
    {
        DestroyWindow();
        m_hWnd = NULL;
        return S_OK;
    }

    hr = IUnknown_GetWindow(pUnkSite, &parentWnd);
    if (!SUCCEEDED(hr))
    {
        ERR("Could not get parent's window ! Status: %08lx\n", hr);
        return E_INVALIDARG;
    }

    m_pSite = pUnkSite;

    if (m_hWnd)
    {
        // Change its parent
        SetParent(parentWnd);
    }
    else
    {
        CDialogImpl::Create(parentWnd);

    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSearchBar::GetSite(REFIID riid, void **ppvSite)
{
    if (!ppvSite)
        return E_POINTER;
    *ppvSite = m_pSite;
    return S_OK;
}


// *** IInputObject methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    if (fActivate)
    {
        //SetFocus();
        SetActiveWindow();
    }
    // TODO: handle message
    if(lpMsg)
    {
        TranslateMessage(lpMsg);
        DispatchMessage(lpMsg);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSearchBar::HasFocusIO()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSearchBar::TranslateAcceleratorIO(LPMSG lpMsg)
{
    if (IsDialogMessage(lpMsg))
        return S_OK;

    if ((lpMsg->hwnd == m_hWnd || IsChild(lpMsg->hwnd)))
    {
        TranslateMessage(lpMsg);
        DispatchMessage(lpMsg);
        return S_OK;
    }

    return S_FALSE;
}

// *** IPersist methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::GetClassID(CLSID *pClassID)
{
    if (!pClassID)
        return E_POINTER;
    *pClassID = CLSID_FileSearchBand;
    return S_OK;
}


// *** IPersistStream methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::IsDirty()
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::Load(IStream *pStm)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::Save(IStream *pStm, BOOL fClearDirty)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    // TODO: calculate max size
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IDispatch methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::GetTypeInfoCount(UINT *pctinfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

void CSearchBar::SetSearchInProgress(BOOL bInProgress)
{
    ::ShowWindow(GetDlgItem(IDC_SEARCH_BUTTON), bInProgress ? SW_HIDE : SW_SHOW);
    ::ShowWindow(GetDlgItem(IDC_SEARCH_STOP_BUTTON), bInProgress ? SW_SHOW : SW_HIDE);
    HWND hProgressBar = GetDlgItem(IDC_PROGRESS_BAR);
    ::ShowWindow(hProgressBar, bInProgress ? SW_SHOW : SW_HIDE);
    ::PostMessage(hProgressBar, PBM_SETMARQUEE, bInProgress, 0);
}

HRESULT CSearchBar::TrySubscribeToSearchEvents()
{
    CComPtr<IShellFolder> pShellFolder;
    HRESULT hr = GetSearchResultsFolder(NULL, NULL, &pShellFolder);
    if (FAILED(hr))
        return hr;

    DWORD fAdviseCookie;
    hr = AtlAdvise(pShellFolder, static_cast<IDispatch *>(this), DIID_DSearchCommandEvents, &fAdviseCookie);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSearchBar::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    switch (dispIdMember)
    {
    case DISPID_NAVIGATECOMPLETE2:
    case DISPID_DOCUMENTCOMPLETE:
    {
        TrySubscribeToSearchEvents();

        // Remove the search results folder from the address box
        CComPtr<IDispatch> pDispatch;
        HRESULT hResult = m_AddressEditBox->QueryInterface(IID_PPV_ARG(IDispatch, &pDispatch));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        pDispatch->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
        CComPtr<IShellService> pShellService;
        hResult = m_AddressEditBox->QueryInterface(IID_PPV_ARG(IShellService, &pShellService));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        hResult = pShellService->SetOwner(NULL);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        HWND hComboboxEx = GetDlgItem(IDC_SEARCH_COMBOBOX);
        int index = SendMessageW(hComboboxEx, CB_GETCOUNT, 0, 0);
        if (index <= 0)
            return S_OK;
        COMBOBOXEXITEMW item = {0};
        item.mask = CBEIF_LPARAM;
        item.iItem = index - 1;
        SendMessageW(hComboboxEx, CBEM_GETITEMW, 0, (LPARAM)&item);
        if (!item.lParam)
            return S_OK;
        CComPtr<IShellFolder> pDesktopFolder;
        hResult = SHGetDesktopFolder(&pDesktopFolder);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        CComPtr<IShellFolder> pShellFolder;
        hResult = pDesktopFolder->BindToObject((LPCITEMIDLIST)item.lParam, NULL, IID_PPV_ARG(IShellFolder, &pShellFolder));
        if (FAILED(hResult))
            return S_OK;
        CLSID clsid;
        hResult = IUnknown_GetClassID(pShellFolder, &clsid);
        if (SUCCEEDED(hResult) && clsid == CLSID_FindFolder)
        {
            SendMessageW(hComboboxEx, CBEM_DELETEITEM, item.iItem, 0);
            SendMessageW(hComboboxEx, CB_SETCURSEL, 0, 0);
        }
        return S_OK;
    }
    case DISPID_SEARCHCOMPLETE:
    case DISPID_SEARCHABORT:
        SetSearchInProgress(FALSE);
        return S_OK;
    default:
        return E_INVALIDARG;
    }
}
