#include "shellprv.h"
#include "combooc.h"

#include "unicpp\sdspatch.h"

#define DROPDOWN_DEFAULTSIZE    300 // in pixels

CComboBoxExOC::CComboBoxExOC(LPCTSTR pszRegKey, int csidlDefaultPidl)
{
    INITCOMMONCONTROLSEX icex;
    DllAddRef();

    // This allocator should have zero inited the memory, so assert the member variables are empty.
    ASSERT(!_hwndComboBox);
    ASSERT(!_hwndEdit);
    ASSERT(!_pszInitialString);
    ASSERT(!_pszPersistString);
    ASSERT(!_fInRecursion);

    _fEnabled = VARIANT_TRUE;        // Default to enabled.
    _pszRegKey = pszRegKey;
    _csidlDefaultPidl = csidlDefaultPidl;

    // We do this so comctl32.dll won't fail to create the ComboBoxEx
    // control.
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_USEREX_CLASSES;
    InitCommonControlsEx(&icex);

    _dwDropDownSize = DROPDOWN_DEFAULTSIZE;
}

CComboBoxExOC::~CComboBoxExOC()
{
    if (_hwndComboBox)
        ::DestroyWindow(_hwndComboBox);

    Str_SetPtrA((LPSTR *)&_pszInitialString, NULL);
    Str_SetPtr((LPTSTR *)&_pszPersistString, NULL);
    ASSERT(!_fInRecursion);     // We are in a bad state.
    DllRelease();
}



LRESULT CComboBoxExOC::cb_ForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
#ifdef UNICODE
    LRESULT lResult = ::SendMessageW(_hwndComboBox, uMsg, wParam, lParam);
#else // UNICODE
    LRESULT lResult = ::SendMessageA(_hwndComboBox, uMsg, wParam, lParam);
#endif // UNICODE

    //TraceMsg(TF_CUSTOM1, "in CComboBoxExOC::_ForwardMessage() forward msg=%d to real window. (lParam=%d, wParam=%d)", uMsg, lParam, wParam);
    bHandled = (lResult ? TRUE : FALSE);
    return lResult;
}


LRESULT CComboBoxExOC::cb_OnDropDownMessage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // DOH! The user wants to see the full combo contents.
    // Better go fill it in now.
    _Populate();
    SetCursor(hCursorOld);

   //TraceMsg(TF_CUSTOM1, "in CComboBoxExOC::_ForwardMessage() forward msg=%d to real window. (lParam=%d, wParam=%d)", uMsg, lParam, wParam);
    bHandled = FALSE;
    return 1;
}

#define STYLE_EX_MASK (CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE | CBES_EX_NOEDITIMAGE | CBES_EX_NOEDITIMAGEINDENT | CBES_EX_PATHWORDBREAKPROC)

HWND CComboBoxExOC::_CreateComboBoxEx(IOleClientSite * pClientSite, HWND hWnd, HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT_PTR nID)
{
    HWND hwndParent = NULL;
    HRESULT hr = S_OK;
    CComQIPtr<IOleWindow, &IID_IOleWindow> spOleWindow(pClientSite);

    if (EVAL(spOleWindow))
        spOleWindow.p->GetWindow(&hwndParent);

    // Please call BryanSt if this happens.
    ASSERT(hwndParent);     // If this happens we need to reparent our selves later.
    if (!hWnd)
    {
        DWORD dwStyle = (WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);
        DWORD dwWindowStyle = _GetWindowStyles();
        UINT_PTR nID = (UINT_PTR)this;

        if (!_fEnableEdit)
            dwWindowStyle |= CBS_DROPDOWNLIST;  // Disable editing in the editbox.

        rcPos.bottom += _dwDropDownSize;

        // This is the wrapper hwnd because ATL doesn't allow the normal winproc to get messages this
        // control doesn't handle.
        hWnd = _Create(hWndParent, rcPos, pszWindowName, dwStyle, dwExStyle, nID);
        if (EVAL(hWnd))
        {   
            _hwndComboBox = CreateWindowEx(0, WC_COMBOBOXEX, TEXT("Shell Name Space ComboBoxEx"), dwWindowStyle,
                0, 0, (rcPos.right - rcPos.left), (rcPos.bottom - rcPos.top), hWnd, (HMENU) FCIDM_VIEWADDRESS, HINST_THISDLL, NULL);
            if (EVAL(_hwndComboBox))
            {
                ::EnableWindow(_hwndComboBox, ((VARIANT_TRUE == _fEnabled) ? TRUE : FALSE));     // Set the gray property in case we needed to cache it until the window was created.

                // Initial combobox parameters.
                ::SendMessage(_hwndComboBox, CBEM_SETEXTENDEDSTYLE,
                        CBES_EX_NOEDITIMAGE |
                        CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE,
                        CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE);

                // _hwndEdit will be NULL if we are a rooted explorer and we
                // turned off the edit attribute of the ComboBox by
                // setting the CBS_DROPDOWNLIST flag when creating the window
                // at the top of this function.
                _hwndEdit = (HWND)::SendMessage(_hwndComboBox, CBEM_GETEDITCONTROL, 0, 0L);
                if (_pszInitialString)
                    ::SetWindowTextA(_hwndComboBox, _pszInitialString);

                // TODO: Add AutoComplete here if we want it.
                ::ShowWindow(_hwndComboBox, SW_SHOW);

                HIMAGELIST  hil = _GetSystemImageListSmallIcons() ;
                ASSERT( hil ) ;
 
                ::SendMessage(_hwndComboBox, CBEM_SETIMAGELIST, 0, (LPARAM)hil);
                ::SendMessage(_hwndComboBox, CBEM_SETEXSTYLE, 0, 0);
                _PopulateTopItem();
            }
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());

        TraceMsg(TF_CUSTOM1, "in CComboBoxExOC::_CreateComboBoxEx() new window created.");
    }
    else
        TraceMsg(TF_CUSTOM1, "in CComboBoxExOC::_CreateComboBoxEx() window already exists.");

    return hWnd;
}

HRESULT CComboBoxExOC::_AddCsidlIcon(LPCTSTR pszDisplayName, LPVOID pvData, int nCsidlIcon, INT_PTR nPos, int iIndent)
{
    CBXITEM item ;
    HRESULT hr = _MakeCsidlIconCbxItem( &item, pszDisplayName, pvData, nCsidlIcon, nPos, iIndent ) ;

    if( SUCCEEDED( hr ) )
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, NULL ) ;
    return hr;
}

HRESULT CComboBoxExOC::_AddFileType(LPCTSTR pszDisplayName, LPCTSTR pszExt, LPCITEMIDLIST pidlIcon, INT_PTR nPos, int iIndent)
{
    HRESULT hr = S_OK;

    CBXITEM item ;
    if( SUCCEEDED( (hr = _MakeFileTypeCbxItem( &item, pszDisplayName, pszExt, pidlIcon, nPos, iIndent )) ) )
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, NULL ) ;
    return hr;
}

HRESULT CComboBoxExOC::_AddResourceAndCsidlStr(UINT idString, int nCsidlItem, int nCsidlIcon, INT_PTR nPos, int iIndent)
{
    CBXITEM item ;
    HRESULT hr = _MakeResourceAndCsidlStrCbxItem( &item, idString, nCsidlItem, nCsidlIcon, nPos, iIndent ) ;

    if( SUCCEEDED( hr ) )
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, NULL ) ;
    return hr ;
}

HRESULT CComboBoxExOC::_AddResourceItem(int idString, DWORD dwData, int nCsidlIcon, INT_PTR nPos, int iIndent)
{
    CBXITEM item ;
    HRESULT hr = _MakeResourceCbxItem( &item, idString, dwData, nCsidlIcon, nPos, iIndent ) ;

    if( SUCCEEDED( hr ) )
    {
        hr = _CustomizeName( idString, item.szText, ARRAYSIZE(item.szText) ) ;
        if( SUCCEEDED( hr ) )
            hr = _AddCbxItemToComboBox( _hwndComboBox, &item, NULL ) ;
    }
    return hr;
}

HRESULT CComboBoxExOC::_AddCsidlItemStr(int nCsidlItem, int nCsidlIcon, INT_PTR nPos, int iIndent)
{
    CBXITEM item ;
    HRESULT hr = _MakeCsidlItemStrCbxItem( &item, nCsidlItem, nCsidlIcon, nPos, iIndent ) ;

    if( SUCCEEDED( hr ) )
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, NULL ) ;
    return hr;
}

HRESULT CComboBoxExOC::_AddCsidlItem(int nCsidlItem, int nCsidlIcon, INT_PTR nPos, int iIndent)
{
    CBXITEM item ;
    HRESULT hr = _MakeCsidlCbxItem( &item, nCsidlItem, nCsidlIcon, nPos, iIndent ) ;

    if( SUCCEEDED( hr ) )
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, NULL ) ;
    return hr;
}

HRESULT CComboBoxExOC::_AddPidl(LPITEMIDLIST pidl, LPITEMIDLIST pidlIcon, INT_PTR nPos, int iIndent)
{
    CBXITEM item ;
    HRESULT hr = _MakePidlCbxItem( &item, pidl, pidlIcon, nPos, iIndent ) ;

    if( SUCCEEDED( hr ) )
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, NULL ) ;

    return hr ;
}


HRESULT CComboBoxExOC::_AddToComboBox(LPCTSTR pszDisplayName, LPVOID pvData, LPCITEMIDLIST pidlIcon, INT_PTR nPos, int iIndent,
                                      INT_PTR *pnPosAdded)
{
    HRESULT hr ;
    CBXITEM item ;
    if( SUCCEEDED( (hr = _MakeCbxItem( &item, pszDisplayName, pvData, pidlIcon, nPos, iIndent )) ) )
    {
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, pnPosAdded ) ;
    }
    return hr ;
}

HRESULT CComboBoxExOC::_AddToComboBoxKnownImage(LPCTSTR pszDisplayName, LPVOID pvData, int iImage, int iSelectedImage, INT_PTR nPos, int iIndent, INT_PTR *pnPosAdded)
{
    CBXITEM item ;
    HRESULT hr ;

    if( SUCCEEDED( (hr = _MakeCbxItemKnownImage( &item, pszDisplayName, pvData, iImage, iSelectedImage, nPos, iIndent )) ) )
        return _AddCbxItemToComboBox( _hwndComboBox, &item, pnPosAdded ) ;

    return hr ;
}

HRESULT CComboBoxExOC::_LoadAndConvertString(UINT idString, LPTSTR pszDisplayName, DWORD chSize)
{
    // After loading the string, we may need to make a conversion on it.
    if (EVAL(LoadString(HINST_THISDLL, idString, pszDisplayName, chSize)))
        return _CustomizeName(idString, pszDisplayName, chSize);

    return E_FAIL;
}

HRESULT CComboBoxExOC::_Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog)
{
    HRESULT hr = S_FALSE;
    VARIANT var;

    TraceMsg(TF_CUSTOM1, "in CUpDownOC::_Load()");

    var.vt = VT_BSTR;
    var.bstrVal = NULL;
    hr = pPropBag->Read(L"Initial String", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_BSTR)
    {
        CHAR szString[MAX_URL_STRING];

        SHUnicodeToAnsi(var.bstrVal, szString, ARRAYSIZE(szString));
        Str_SetPtrA((LPSTR *)&_pszInitialString, szString);
    }

    var.vt = VT_BSTR;
    var.bstrVal = NULL;
    hr = pPropBag->Read(L"Persist String", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_BSTR)
    {
        TCHAR szString[MAX_URL_STRING];

        SHUnicodeToTChar(var.bstrVal, szString, ARRAYSIZE(szString));
        Str_SetPtr((LPTSTR *)&_pszPersistString, szString);
    }

    var.vt = VT_UI4;
    var.ulVal = NULL;
    hr = pPropBag->Read(L"Drop Down Height", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
        _dwDropDownSize = var.ulVal;

    var.vt = VT_UI4;
    var.ulVal = NULL;
    hr = pPropBag->Read(L"Enable Edit", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
        _fEnableEdit = var.ulVal;

    var.vt = VT_UI4;
    var.ulVal = NULL;
    hr = pPropBag->Read(L"Enabled", &var, NULL);
    if (SUCCEEDED(hr) && (var.vt==VT_UI4) && (!var.ulVal))
        _fEnabled = VARIANT_FALSE;

    return S_OK;
}


HRESULT CComboBoxExOC::TranslateAcceleratorInternal(MSG *pMsg, IOleClientSite * pocs)
{
    HRESULT hr = E_NOTIMPL;

    if (WM_KEYDOWN == pMsg->message)
    {
        switch (pMsg->wParam)
        {
        case VK_RETURN:
            if (TranslateMessage(pMsg))
            {
                DispatchMessage(pMsg);
                Fire_EnterPressed();
                hr = S_OK;
            }
            break;
        default:
            break;
        }
    }

    return hr;
}


/****************************************************\
    FUNCTION: _PopulateTopItem

    DESCRIPTION:
        This function is optimizing for perf.  Since
    enumerating the drop down includes hitting the disk
    a bit, we want to optimize for the default case of
    "My Computer" being selected.
\****************************************************/
HRESULT CComboBoxExOC::_PopulateTopItem(void)
{
    TCHAR szLastSelection[MAX_URL_STRING];
    DWORD cbSize = SIZEOF(szLastSelection);
    BOOL fIsSecure = _IsSecure();

    szLastSelection[0] = 0;
    // Will we allow the page to use persistence.
    if (TRUE == fIsSecure)
        SHGetValue(HKEY_CURRENT_USER, _pszRegKey, (_pszPersistString ? _pszPersistString : SZ_REGVALUE_LAST_SELECTION), NULL, (LPVOID)szLastSelection, &cbSize);

    // We will use the default item if either:
    // 1. We are not secure so we can't read the registery - or -
    // 2. We read the registery and it's blank or equal to the default value.
    if (!fIsSecure ||
        (!szLastSelection[0] || _IsDefaultSelection(szLastSelection)))
    {
        // Yes
        _AddDefaultItem();
    }

    // Also give the subclass a way to handle it directly himself...
    else if (_RestoreIfUserInputValue(szLastSelection) != S_OK)
    {
        TCHAR szRegValue[MAX_PATH];
        TCHAR szLastPath[MAX_URL_STRING];

        szLastPath[0] = 0;
        cbSize = SIZEOF(szLastPath);
        wnsprintf(szRegValue, ARRAYSIZE(szRegValue), TEXT("%s_Path"), (_pszPersistString ? _pszPersistString : SZ_REGVALUE_LAST_SELECTION));
        SHGetValue(HKEY_CURRENT_USER, _pszRegKey, szRegValue, NULL, (LPVOID)szLastPath, &cbSize);

        // No, so take the hit now.
        _Populate();

        if (szLastSelection[0] && szLastPath[0])
            _SetSelect(szLastSelection, szLastPath);
    }

    return S_OK;
}


HRESULT CComboBoxExOC::_SetSelect(LPCTSTR pszDisplay, LPCTSTR pszReplace)
{
    HRESULT hr = E_FAIL;
    LRESULT nIndex = ::SendMessage(_hwndComboBox, CB_FINDSTRINGEXACT, (WPARAM)0, (LPARAM) pszDisplay);

    if (CB_ERR == nIndex)
    {
        // Not in the name space, meaning it's probably custom.
        ::SetWindowText(_hwndComboBox, pszReplace);
    }
    else
        ::SendMessage(_hwndComboBox, CB_SETCURSEL, (WPARAM)nIndex, 0);

    return hr;
}


HRESULT CComboBoxExOC::_putString(LPCSTR pszString)
{
    Str_SetPtrA((LPSTR *)&_pszInitialString, pszString);
    if (_hwndComboBox)
        ::SetWindowTextA(_hwndComboBox, _pszInitialString);

    return S_OK;
}


HRESULT CComboBoxExOC::get_Enabled(OUT VARIANT_BOOL * pfEnabled)
{
    if (!EVAL(pfEnabled))
        return S_FALSE;

    *pfEnabled = _fEnabled;
    return S_OK;
}

HRESULT CComboBoxExOC::put_Enabled(IN VARIANT_BOOL fEnabled)
{    
    if (_hwndComboBox)
    {
        _fEnabled = fEnabled;
        ::EnableWindow(_hwndComboBox, ((VARIANT_TRUE == _fEnabled) ? TRUE : FALSE));
    }
    else
        _fEnabled = fEnabled;

    return S_OK;
}



