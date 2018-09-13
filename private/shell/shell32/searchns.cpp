#include "shellprv.h"
#include "searchns.h"

#include "unicpp\sdspatch.h"
#include "exdispid.h"
#include <winuser.h>

#define SZ_REGKEY_CTL_SEARCHNAMESPACE  TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Controls\\SearchNameSpace")
#define CSIDL_DEFAULT_PIDL             CSIDL_RECENT
#define IDS_SEARCH_DEFAULT_LOC         IDS_CSIDL_RECENT
#define COMBO_SUBCLASS_COOKIE          2895
#define EDIT_SUBCLASS_COOKIE           52895
#define LIST_SIZE                      20

// Some stuff added to do timings of populating the list...
#ifdef DEBUG
//#define TIME_POPULATE
#endif

CSearchNameSpaceOC::CSearchNameSpaceOC() : CComboBoxExOC(SZ_REGKEY_CTL_SEARCHNAMESPACE, CSIDL_DEFAULT_PIDL),
        _iDeferPathList(CB_ERR), _iLocalDisk(CB_ERR)
{
    DllAddRef();

    // This allocator should have zero inited the memory, so assert the member variables are empty.
    ASSERT(!_fPopulated);
    ASSERT(!_pidlStart);
    _fFileSysAutoComp = TRUE;

    // ATL needs these to make our window resize automatically.
    m_bWindowOnly = TRUE;
    m_bEnabled = TRUE;
    m_bRecomposeOnResize = TRUE;
    m_bResizeNatural = TRUE;
}

CSearchNameSpaceOC::~CSearchNameSpaceOC()
{
    ILFree(_pidlStart); // should long be gone, but...

    // Get rid of connection point as well...
    if (_dwCookie) {
        _pcpBrowser->Unadvise(_dwCookie);
        ATOMICRELEASE(_pcpBrowser);
        _dwCookie = 0;
    }

    DllRelease();
}


LRESULT CSearchNameSpaceOC::_OnDeleteItemMessage(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    // HACKHACK: combobox (comctl32\comboex.c) will pass a LPNMHDR, but it's really
    //	         a PNMCOMBOBOXEX (which has a first element of LPNMHDR).  This function
    //	         can use this type cast iff it's guaranteed that this will only come from
    //           a function that behaves in this perverse way.
    LRESULT lRet = _DeleteNamespaceComboItem( pnmh ) ;
    bHandled = FALSE;
    return lRet ;
}

LRESULT CSearchNameSpaceOC::_OnFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    // CComControlBase::InPlaceActivate() will call us back recursively, so ignore the second call. 
    TraceMsg(TF_CUSTOM1, "in CSearchNameSpaceOC::_OnFocus(), _fInRecursion=%d", _fInRecursion);
    if (_fInRecursion)
        return 1;       // We already did the set focus.

    _fInRecursion = TRUE;
    _PrivateActivate();

    LRESULT lResult = ::SendMessage(_hwndComboBox, uMsg, wParam, lParam);
    bHandled = FALSE;
    _fInRecursion = FALSE;
    return 1;
}


HRESULT CSearchNameSpaceOC::_PrivateActivate(void)
{
    TraceMsg(TF_CUSTOM1, "in CSearchNameSpaceOC::_PrivateActivate(), _fInRecursion=%d", _fInRecursion);

    if (!m_bUIActive)
        CComControlBase::InPlaceActivate(OLEIVERB_UIACTIVATE);

    return S_OK;
}


// in unicpp\cpymovto.cpp
extern int BrowseCallback(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData);


HRESULT CSearchNameSpaceOC::_BrowseForDirectory(LPTSTR pszPath, DWORD cchSize)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlRoot = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, TRUE);

    if (EVAL(pidlRoot))
    {
        LPITEMIDLIST pidlDefault = SHCloneSpecialIDList(NULL, CSIDL_PERSONAL, TRUE);
        
        if (EVAL(pidlDefault))
        {
            TCHAR szTitle[MAX_PATH];

            if (EVAL(LoadString(HINST_THISDLL, IDS_SNS_BROWSERFORDIR_TITLE, szTitle, ARRAYSIZE(szTitle))))
            {
                // Yes, so display the dialog and replace the display text with the path
                // if something was selected.
                BROWSEINFO bi = {0};
                LPITEMIDLIST pidl;

                bi.hwndOwner        = _hwndComboBox;
                // bi.pszDisplayName   = pszPath; - // If we want to display a friendly name then set this value
                bi.pidlRoot         = pidlRoot;
                bi.lpszTitle        = szTitle;
                bi.ulFlags          = (BIF_EDITBOX | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
                bi.lpfn             = BrowseCallback;
                bi.lParam           = (LPARAM) &pidlDefault;

                ASSERT(MAX_PATH <= cchSize);    // bi.pszDisplayName requires this

                // Display Dialog and see if they cancelled or not.
                hr = S_FALSE;
                pidl = SHBrowseForFolder(&bi);
                if (pidl)
                {
                    SHGetPathFromIDList(pidl, pszPath);
                    ILFree(pidl);
                    hr = S_OK;
                }
            }

            ILFree(pidlDefault);
        }

        ILFree(pidlRoot);
    }

    return hr;
}

LRESULT CSearchNameSpaceOC::_OnSelectChangeMessage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LONG nSelected = (LONG) ::SendMessage(_hwndComboBox, CB_GETCURSEL, 0, 0);
    LPCVOID pvData = (LPCVOID)::SendMessage(_hwndComboBox, CB_GETITEMDATA, nSelected, 0);

    // Was this the "Browse..." item?
    if ((-1 != nSelected) && (INVALID_HANDLE_VALUE == pvData))
    {
        TCHAR szPath[MAX_PATH];

        if (S_OK == _BrowseForDirectory(szPath, ARRAYSIZE(szPath)))
        {
            // They didn't cancel so grab the directory path.
            ::SetWindowText(_hwndComboBox, szPath);
        }
        else
            ::SetWindowText(_hwndComboBox, TEXT(""));   // Do this so we don't leave "Browse..."
    }

    bHandled = FALSE;
    return 1;
}


LRESULT CSearchNameSpaceOC::SubClassWndProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam, WPARAM uIdSubclass, ULONG_PTR dwRefData)
{
    if ((COMBO_SUBCLASS_COOKIE == uIdSubclass) ||
        (EDIT_SUBCLASS_COOKIE == uIdSubclass))
    {
        switch (uMessage)
        {
        case WM_SETFOCUS:
            {
                CSearchNameSpaceOC * pdtp = (CSearchNameSpaceOC *) dwRefData;

                if (EVAL(pdtp))
                    pdtp->_PrivateActivate();
            }
            break;
        case WM_DESTROY:
            RemoveWindowSubclass(hwnd, CSearchNameSpaceOC::SubClassWndProc, uIdSubclass);
            break;
        }
    }

    return DefSubclassProc(hwnd, uMessage, wParam, lParam);
}


HWND CSearchNameSpaceOC::Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT_PTR nID)
{
    m_hWnd = _CreateComboBoxEx(m_spClientSite, m_hWnd, hWndParent, rcPos, pszWindowName, dwStyle, dwExStyle, nID);
    if (EVAL(_hwndComboBox))
    {
        BOOL fSucceeded;
        HWND hwndComboBox = (HWND)::SendMessage(_hwndComboBox, CBEM_GETCOMBOCONTROL, 0, 0);

        if (EVAL(hwndComboBox))
        {
            fSucceeded = SetWindowSubclass(hwndComboBox, CSearchNameSpaceOC::SubClassWndProc, COMBO_SUBCLASS_COOKIE, (ULONG_PTR) this);
            ASSERT(fSucceeded);
        }

        if (_hwndEdit)  // User may not want combobox to be editable.
        {
            fSucceeded = SetWindowSubclass(_hwndEdit, CSearchNameSpaceOC::SubClassWndProc, EDIT_SUBCLASS_COOKIE, (ULONG_PTR) this);
            ASSERT(fSucceeded);
        }
    }

    if (EVAL(_hwndEdit) && _fFileSysAutoComp)
        SHAutoComplete(_hwndEdit, NULL);

    return m_hWnd;
}


/*
LRESULT CSearchNameSpaceOC::_DisableIconMessage(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
//    ::SendMessage(_hwndComboBox, CBEM_SETEXTENDEDSTYLE, CBES_EX_NOEDITIMAGE, 0);    // Disable Icon
    bHandled = FALSE;
    return 1;
}

LRESULT CSearchNameSpaceOC::_EnableIconMessage(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
//    ::SendMessage(_hwndComboBox, CBEM_SETEXTENDEDSTYLE, CBES_EX_NOEDITIMAGE, CBES_EX_NOEDITIMAGE);    // Enable Icon
    bHandled = FALSE;  // BUGBUG
    return 1;
}
*/

// IPersistPropertyBag
HRESULT CSearchNameSpaceOC::Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog)
{
    HRESULT hr = S_FALSE;
    VARIANT var;

    TraceMsg(TF_CUSTOM1, "in CSearchNameSpaceOC::Load()");
    // TODO: Load in SearchNameSpace specific settings.

    var.vt = VT_UI4;
    var.ulVal = NULL;
    hr = pPropBag->Read(L"AutoComplete In File System", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
        _fFileSysAutoComp = var.ulVal;

    hr = CComboBoxExOC::_Load(pPropBag, pErrorLog);
    return S_OK;
}


HRESULT CSearchNameSpaceOC::_GetSelectText(LPTSTR pszSelectText, DWORD cchSize, BOOL fDisplayName)
{
    LPITEMIDLIST pidl = SHCloneSpecialIDList(NULL, CSIDL_DEFAULT_PIDL, TRUE);

    if (EVAL(pidl))
    {
        UINT shgdnf = (fDisplayName ? SHGDN_NORMAL : SHGDN_FORPARSING | SHGDN_FORADDRESSBAR);
        SHGetNameAndFlags(pidl, shgdnf, pszSelectText, cchSize, NULL);
        ILFree(pidl);
    }

    return S_OK;
}


HRESULT CSearchNameSpaceOC::IOleInPlaceObject_InPlaceDeactivate(void)
{
    // Take this time to persist out selection.
    TCHAR szRegValue[MAX_PATH];

    // sortof gross, but save room for ! at start to say that this is user input...
    TCHAR szDisplayName[MAX_URL_STRING+1];
    LPTSTR pszDisplayName = &szDisplayName[1];
    LPCTSTR pszPath = NULL;

    LRESULT nIndex = _GetCurItemTextAndIndex( FALSE, pszDisplayName, ARRAYSIZE(szDisplayName)-1);

    TraceMsg(TF_CUSTOM1, "in CSearchNameSpaceOC::CSearchNameSpaceOC::IOleInPlaceObject_InPlaceDeactivate(), _fInRecursion=%d", _fInRecursion);

    _fPopulated = FALSE;    // This call will destroy the window.

    if (nIndex == -1)
    {
        // we are adding a speed hack to the control that allows us to know the next time we load this control
        // that this was user input data.  We detect if it had an ! at the start (not a valid start for a path) and if so
        // simply restore the path.
        szDisplayName[0] = TEXT('!');
        pszDisplayName = szDisplayName;
    }
    else
        pszPath = (LPCTSTR)::SendMessage(_hwndComboBox, CB_GETITEMDATA, (WPARAM)nIndex, (LPARAM)0);

    SHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_CTL_SEARCHNAMESPACE, 
            (_pszPersistString ? _pszPersistString : SZ_REGVALUE_LAST_SELECTION), REG_SZ, 
            (LPVOID)pszDisplayName, (lstrlen(pszDisplayName) + 1) * sizeof(TCHAR));


    wnsprintf(szRegValue, ARRAYSIZE(szRegValue), TEXT("%s_Path"), (_pszPersistString ? _pszPersistString : SZ_REGVALUE_LAST_SELECTION));
    SHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_CTL_SEARCHNAMESPACE, szRegValue, REG_SZ, 
            (LPVOID)(pszPath ? pszPath : pszDisplayName), 
            (lstrlen(pszPath ? pszPath : pszDisplayName) + 1) * sizeof(TCHAR));
    return CComControlBase::IOleInPlaceObject_InPlaceDeactivate();
}


HRESULT CSearchNameSpaceOC::_Populate(void)
{
//    if (_fPopulated)
//        return S_OK;        // Done.
#ifdef TIME_POPULATE
#define ENDTIME(x) DWORD x = GetTickCount()
    DWORD dwStart = GetTickCount();
#else 
#define ENDTIME(x)
#endif

    _fPopulated = TRUE;
    ASSERT(_hwndComboBox);
    ::SendMessage(_hwndComboBox, CB_RESETCONTENT, 0, 0L);
    _iLocalDisk = CB_ERR;   // reset to default value...
    _iDeferPathList = CB_ERR;

    // Document Folders
    _AddResourceAndCsidlStr(IDS_CSIDL_PERSONAL, CSIDL_PERSONAL, CSIDL_PERSONAL, LISTINSERT_LAST, NO_ITEM_INDEX);       //  My Documents (IDS_CSIDL_PERSONAL)
    _AddCsidlItemStr(CSIDL_DESKTOPDIRECTORY, CSIDL_DESKTOP, LISTINSERT_LAST, NO_ITEM_INDEX);                                    //  Desktop
    ENDTIME(dwPer);

    // My Computer
    _AddMyComputer();                                                                                               // My Computer Drives
    ENDTIME(dwMyComp);
    _AddLocalHardDrives();                                                                                          //  Local HardDrives (C:\, D:\, E:\)
    ENDTIME(dwLocDrives);
    _AddDrives();                                                                                                   //  each drive
    ENDTIME(dwDrives);

    // My Network Places
    _AddMyNetworkPlaces();                                                                                          // My Network Places
    ENDTIME(dwMyNet);
    _AddMyNetworkPlacesItems();                                                                                     //  Nethood and maybe PubPlaces
    ENDTIME(dwMyNetI);

    // Recent Folder
    _AddRecentFolderAndEntries(TRUE);                                                                               // Recent Folder
    ENDTIME(dwRecent);

    // Browse...
    _AddBrowseItem();                                                                                             // Recent Folder

    _SetDefaultSelect();
#ifdef TIME_POPULATE
    TraceMsg(TF_GENERAL, "Populate Total Time=%d", (GetTickCount()-dwStart));
DWORD dwLast = dwStart;
#define PDELTA(x, y)  TraceMsg(TF_GENERAL, "        %s=%d", TEXT(x), (y-dwLast)); dwLast = y;
    PDELTA("My Computer", dwMyComp);
    PDELTA("Local Drives", dwLocDrives);
    PDELTA("All Drives", dwDrives);
    PDELTA("My Network", dwMyNet);
    PDELTA("My Network Items", dwMyNetI);
    PDELTA("Recent and Items", dwRecent);
#endif

    return S_OK;
}

HRESULT CSearchNameSpaceOC::_AddMyComputer(void)
{
    CBXITEM item ;
    HRESULT hr = _MakeMyComputerCbxItem( &item ) ;

    if( SUCCEEDED( hr ) )
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, NULL ) ;
    return hr;
}

HRESULT CSearchNameSpaceOC::_AddLocalHardDrives(void)
{
    CBXITEM item ;
    HRESULT hr = _MakeLocalHardDrivesCbxItem( &item ) ;

    if( SUCCEEDED( hr ) )
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, &_iLocalDisk ) ;
    return hr;
}

HRESULT CSearchNameSpaceOC::_CustomizeName(UINT idString, LPTSTR szDisplayName, DWORD cchSize)
{
    if (IDS_SNS_LOCALHARDDRIVES == idString)
    {
        TCHAR szDriveList[MAX_PATH];    // Needs to be 'Z'-'A' (26) * 3 + 1 = 79.
        TCHAR szTemp[MAX_PATH];
        TCHAR szDrive[3] = TEXT("A:");
        int nSlot = 0;
        TCHAR chDriveLetter;

        for (chDriveLetter = TEXT('A'); chDriveLetter <= TEXT('Z'); chDriveLetter++)
        {
            szDrive[0] = chDriveLetter;
            if (DRIVE_FIXED == GetDriveType(szDrive))
            {
                if (nSlot) // Do we need to add a separator before we add the next item?
                    szDriveList[nSlot++] = TEXT(','); // terminate list.  

                szDriveList[nSlot++] = chDriveLetter; // terminate list.  
                szDriveList[nSlot++] = TEXT(':'); // terminate list.  
            }
        }
        szDriveList[nSlot] = 0; // terminate list.  

        wsprintf(szTemp, szDisplayName, szDriveList);
        StrCpyN(szDisplayName, szTemp, cchwSize);     // Put back into the final location.
    }

    return S_OK;
}

HRESULT CSearchNameSpaceOC::_AddMappedDrives(LPITEMIDLIST pidl)
{
    CBXITEM item ;

    HRESULT hr = _MakeMappedDrivesCbxItem( &item, pidl) ;

    if( SUCCEEDED( hr ) )
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, NULL ) ;
    return hr ;
}


HRESULT CSearchNameSpaceOC::_AddNethoodDirs(LPITEMIDLIST pidl)
{
    CBXITEM item ;
    HRESULT hr = _MakeNethoodDirsCbxItem( &item, pidl ) ;

    if( SUCCEEDED( hr ) )
    {
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, NULL ) ;
        if( SUCCEEDED( hr ) )
            EVAL(_AddPath( item.szText )) ;  // TODO: Use something larger than MAX_PATH.
    }
    return hr ;
}

HRESULT CSearchNameSpaceOC::_AddRecentFolderAndEntries(BOOL fAddEntries)
{
    CBXITEM item ;
    HRESULT hr = _MakeRecentFolderCbxItem( &item ) ;

    if( SUCCEEDED( hr ) )
    {
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, &_iDeferPathList ) ;
    
        if (fAddEntries)
            _ReallyEnumRecentFolderAndEntries(fAddEntries);
    }
    return S_OK;
}

// Ok we now need to do the work to get the paths contained in recent...
HRESULT CSearchNameSpaceOC::_ReallyEnumRecentFolderAndEntries(BOOL fAddEntries)
{
        LPTSTR pszPath = NULL;


        // Now build the list and optional the entries
        _EnumRecentAndGeneratePath( fAddEntries, _EnumRecentAndGenPathCB, this );

        if (!_pszPathList || !*_pszPathList)
        {
            // Recent is empty, for now assume local hard disks...
            if (_iLocalDisk != CB_ERR)
            {
                LPTSTR pszString;
                pszString = (LPTSTR)::SendMessage(_hwndComboBox, CB_GETITEMDATA, _iLocalDisk, 0);
                Str_SetPtr(&_pszPathList, pszString);
            }
            else
            {
                // handle case where we may not have enum the list...
                TCHAR szPath[MAX_PATH];
                _BuildDrivesList(DRIVE_FIXED, TEXT(";"), TEXT(":\\"), szPath, ARRAYSIZE(szPath));
                Str_SetPtr(&_pszPathList, szPath);
            }
        }

        // Now update the path for the item to the calculated path.
        _UpdateDeferredPathFromPathList();

    return S_OK;
}


HRESULT CSearchNameSpaceOC::_EnumRecentAndGenPathCB( LPCTSTR pszPath, BOOL fAddEntries, LPVOID pvParam )
{
    CSearchNameSpaceOC* pThis = (CSearchNameSpaceOC*)pvParam ;
    ASSERT( pThis ) ;

    EVAL(pThis->_AddPath(pszPath));
    if (fAddEntries)
    {
        LPTSTR psz = NULL;
        Str_SetPtr(&psz, pszPath);

        CBXITEM item ;
        if( SUCCEEDED( _MakeCbxItemKnownImage( &item, pszPath, (LPVOID)psz,
                                               3, 3, LISTINSERT_LAST, ITEM_INDEX ) ) )
            _AddCbxItemToComboBox( pThis->_hwndComboBox, &item, NULL ) ;
    }
    return S_OK ;
}

HRESULT CSearchNameSpaceOC::_AddBrowseItem(void)
{
    CBXITEM item ;
    HRESULT hr = _MakeBrowseForCbxItem( &item ) ;

    if( SUCCEEDED( hr ) )
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, NULL ) ;
    return hr;
}

BOOL CSearchNameSpaceOC::_AddPath(LPCTSTR pszPath)
{
    BOOL fSuccess;
    TCHAR szPathList[MAX_PATH * LIST_SIZE];

    szPathList[0] = 0;
    if (_pszPathList)
        StrCpyN(szPathList, _pszPathList, ARRAYSIZE(szPathList));

    fSuccess = SafePathListAppend(szPathList, ARRAYSIZE(szPathList), pszPath);
    Str_SetPtr(&_pszPathList, szPathList);

    return fSuccess;
}

HRESULT CSearchNameSpaceOC::_AddMyNetworkPlaces(void)
{
    CBXITEM item ;
    LPTSTR pszPath = NULL;

    Str_SetPtr(&pszPath, (_pszPathList ? _pszPathList : TEXT("")));
    HRESULT hr = _MakeNetworkPlacesCbxItem( &item, pszPath ) ;

    if( SUCCEEDED( hr ) )
    {
        hr = _AddCbxItemToComboBox( _hwndComboBox, &item, &_iDeferPathList ) ;
        if( SUCCEEDED( hr ) )
            Str_SetPtr(&_pszPathList, NULL);
    }
    return hr ;
}

HRESULT CSearchNameSpaceOC::_AddMyNetworkPlacesItems(void) 
{ 
    HRESULT hr = _EnumSpecialItemIDs(CSIDL_NETHOOD, (SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN), 
            _AddNethoodDirsCB, this); 
    if (SUCCEEDED(hr))
        _UpdateDeferredPathFromPathList();
    return hr;
};


void CSearchNameSpaceOC::_UpdateDeferredPathFromPathList(void) 
{ 
    if (_iDeferPathList != CB_ERR)
    {
        LPTSTR pszString = (LPTSTR)::SendMessage(_hwndComboBox, CB_GETITEMDATA, _iDeferPathList, 0);
        Str_SetPtr(&pszString, NULL);
        pszString = (LPTSTR)::SendMessage(_hwndComboBox, CB_SETITEMDATA, _iDeferPathList, (LPARAM)_pszPathList);
        _pszPathList = NULL;
        _iDeferPathList = CB_ERR;
    }
};


HRESULT CSearchNameSpaceOC::_IsDefaultSelection(LPCTSTR pszLastSelection)
{
    BOOL fIsDefault = FALSE;
    TCHAR szDefault[MAX_PATH];

    // if there is a pidl start say no here...
    if (_FGetStartingPidl())
        return FALSE;

    if (EVAL(LoadString(HINST_THISDLL, IDS_SEARCH_DEFAULT_LOC, szDefault, ARRAYSIZE(szDefault))) &&
        !StrCmp(szDefault, pszLastSelection))
    {
        fIsDefault = TRUE;
    }

    return fIsDefault;
}

BOOL CSearchNameSpaceOC::_SetupBrowserCP()
{
    HRESULT hr;
    if (!_dwCookie) {
        _cwbe.SetOwner(this);   // make sure our owner is set...

        IServiceProvider *pspTLB;
        IConnectionPointContainer *pcpc;

        // OK now lets register ourself with the Defview to get any events that they may generate...
        if (SUCCEEDED(hr = IUnknown_QueryService(m_spClientSite, SID_STopLevelBrowser, 
                IID_IServiceProvider, (void**)&pspTLB))) {
            if (SUCCEEDED(hr = pspTLB->QueryService(IID_IExpDispSupport, IID_IConnectionPointContainer, (void **)&pcpc))) {
                hr = ConnectToConnectionPoint(SAFECAST(&_cwbe,IDispatch*), DIID_DWebBrowserEvents2,
                                              TRUE, pcpc, &_dwCookie, &_pcpBrowser);
                pcpc->Release();
            }
            pspTLB->Release();
        }
    }
    if (_dwCookie) {
        _cwbe.SetWaiting(TRUE);
        return TRUE;
    }

    return FALSE;
}

HRESULT CSearchNameSpaceOC::_RestoreIfUserInputValue(LPCTSTR pszLastSelection)
{
    if (_pidlStart)
    {
        // BUGBUG only works if file system path...
        // Should check to see if get path fails and try to select item by different
        // way...
        TCHAR szPath[ MAX_PATH ];
        BOOL fOk = SHGetPathFromIDList(_pidlStart, szPath);
        ILFree(_pidlStart);
        _pidlStart = NULL;
        if (fOk)
        {
            ::SetWindowText(_hwndComboBox, szPath);
            return S_OK;
        }
    }

    if (*pszLastSelection == TEXT('!'))
    {
        // user input...
        ::SetWindowText(_hwndComboBox, pszLastSelection+1);
        return S_OK;
    }
    return S_FALSE; // say nope we did not handle this.
}

LRESULT CSearchNameSpaceOC::_GetCurItemTextAndIndex( BOOL fPath, LPTSTR psz, int cch)
{
    //  Devnote: Keep logic synched with external version
    //  _GetNamespaceComboItemText(), _GetNamespaceComboSelItemText()

    // Share code with get_String and the save user input code.
    TCHAR szItemName[MAX_PATH];
    LRESULT nSelected = ::SendMessage(_hwndComboBox, CB_GETCURSEL, 0, 0);

    // We don't trust the comboex to handle the edit text properly so try to compensate...
    ::GetWindowText(_hwndComboBox, psz, cch);
    if (-1 != nSelected)
    {
        if (::SendMessage(_hwndComboBox, CB_GETLBTEXT, (WPARAM)nSelected, (LPARAM)szItemName) == CB_ERR)
            szItemName[0]=0;

        LPCTSTR pszString = (LPCTSTR)::SendMessage(_hwndComboBox, CB_GETITEMDATA, nSelected, 0);
        // We doule check that the text of the control is equal to the text of the item.  If not the control
        // probably screwed up and we will use the text...  Also check for error conditions returned from GetItemData...
        // Why would this happen? (Did once during stress...)
        if (pszString && (lstrcmp(pszString, NAMESPACECOMBO_RECENT_PARAM) == 0))
        {
            // oops this is the default case and we have not calculated the default yet, do it now...
            _iDeferPathList = nSelected;
            _ReallyEnumRecentFolderAndEntries(FALSE);

            // Try again...
            pszString = (LPCTSTR)::SendMessage(_hwndComboBox, CB_GETITEMDATA, nSelected, 0);
        }
        
        if (EVAL(!pszString) || EVAL(pszString == (LPCTSTR)CB_ERR) || (lstrcmp(psz, szItemName) != 0))
            nSelected = -1;
        else
            StrCpyN(psz, fPath? pszString : szItemName, cch);
    }
    return nSelected;
}

// IComboBoxExOC
HRESULT CSearchNameSpaceOC::get_String(OUT BSTR *pbs)
{
    if (EVAL(pbs && _hwndComboBox))
    {
        TCHAR szPath[MAX_PATH];
        _GetCurItemTextAndIndex( TRUE, szPath, ARRAYSIZE(szPath));

#ifdef UNICODE
        *pbs = SysAllocString(szPath);
#else // UNICODE
        WCHAR wzString[MAX_URL_STRING];

        SHAnsiToUnicode(szPath, wzString, ARRAYSIZE(wzString));
        *pbs = SysAllocString(wzString);
#endif // UNICODE
    }

    return S_OK;
}


HRESULT CSearchNameSpaceOC::put_String(IN BSTR bs)
{    
    HRESULT hr = S_FALSE;

    if (EVAL(bs))
    {
        CHAR szNewString[MAX_URL_STRING];

        SHUnicodeToAnsi(bs, szNewString, ARRAYSIZE(szNewString));
        hr = CComboBoxExOC::_putString(szNewString);
    }

    return hr;
}


// *** ISearchNameSpaceOC ***
HRESULT CSearchNameSpaceOC::get_IsValidSearch(OUT VARIANT_BOOL * pfValid)
{
    if (!pfValid)
        return S_FALSE;

    // we start by assuming the search is valid
    *pfValid = VARIANT_TRUE;

    // Get the selected item index;
    LRESULT nIndex = ::SendMessage(_hwndComboBox, CB_GETCURSEL, 0, 0);

    // If any item is selected then we are valid because we only have valid items in the combo box.
    if (-1 == nIndex)
    {
        // If no item is selected the user might have entered a path.  Any string is considered
        // valid so we simply need to check if the path is empty.  If the path is empty then
        // this is an invalid search.  Note that an empty path is a valid search if the index
        // does not equal -1.
        TCHAR szPath[3];
        ::GetWindowText(_hwndComboBox, szPath, ARRAYSIZE(szPath));
        if ( !szPath[0] )
        {
            // So basically the only invalid case is when nIndex == -1 and the path is NULL
            *pfValid = VARIANT_FALSE;
        }
    }

    return S_OK;
}

HRESULT CSearchNameSpaceOC::SetClientSite(IOleClientSite * pClientSite)
{
    if (!pClientSite && _dwCookie) 
    {
        _pcpBrowser->Unadvise(_dwCookie);
        ATOMICRELEASE(_pcpBrowser);
        _dwCookie = 0;
    }

    return CComControlBase::IOleObject_SetClientSite(pClientSite);
}


HRESULT CSearchNameSpaceOC::_SetDefaultSelect(void)
{
    TCHAR szDefaultDisplayName[MAX_URL_STRING];
    TCHAR szDefaultPath[MAX_URL_STRING];

    if (EVAL(SUCCEEDED(_GetSelectText(szDefaultDisplayName, ARRAYSIZE(szDefaultDisplayName), TRUE))) &&
        EVAL(SUCCEEDED(_GetSelectText(szDefaultPath, ARRAYSIZE(szDefaultPath), FALSE))))
    {
        _SetSelect(szDefaultDisplayName, szDefaultPath);
    }

    return S_OK;
}


#define SZ_WEB_SEARCHTEMPLATE_PATH      TEXT("Web\\fsearch.htm")

HRESULT CSearchNameSpaceOC::_GetSearchUrlW(LPWSTR pwzUrl, DWORD cchSize)
{
    TCHAR szUrlPath[MAX_PATH];

    if (EVAL(GetWindowsDirectory(szUrlPath, ARRAYSIZE(szUrlPath))))
    {
        DWORD cchTempSize;

        PathAppend(szUrlPath, SZ_WEB_SEARCHTEMPLATE_PATH);
        EVAL(SUCCEEDED(UrlCreateFromPath(szUrlPath, szUrlPath, &cchTempSize, 0)));
        SHTCharToUnicode(szUrlPath, pwzUrl, cchSize);
    }

    return S_OK;
}

BOOL CSearchNameSpaceOC::_IsSecure(void)
{    
    BOOL fSecure = FALSE;

    if (EVAL(m_spClientSite))
    {
        if (S_OK == LocalZoneCheck((IUnknown *) m_spClientSite))
            fSecure = TRUE;
    }
    else
        TraceMsg(TF_CUSTOM1, "in CSearchNameSpaceOC::_GetEditboxBuddy() We don't have a parent yet.");

    return fSecure;
}

BOOL CSearchNameSpaceOC::_FGetStartingPidl(void)
{    

    if (EVAL(m_spClientSite))
    {
        IShellBrowser  *psb;
        IShellView     *psv;
        IWebBrowser2* pwb = NULL;
        if (SUCCEEDED(IUnknown_QueryService(m_spClientSite, SID_STopLevelBrowser, 
                IID_IShellBrowser, (void **) &psb)))
        {
            
            // Warning:: We check for shell view simply to see how the search pane was
            // loaded.  If this fails it is because we were loaded on the CoCreateInstance
            // of the browser window and as such it is a race condition to know if the
            // properties were set or not.  So in this case wait until we get a 
            // navigate complete.  This lets us know for sure if a save file was passed
            // in or not.
            if (SUCCEEDED(psb->QueryActiveShellView(&psv))) 
            {

                if (SUCCEEDED(IUnknown_QueryService(psb, SID_SWebBrowserApp, 
                        IID_IWebBrowser2, (void **) &pwb)))
                {
                    SA_BSTR bstr;
                    VARIANT var;       
                    VariantInit(&var);
                    lstrcpyW(bstr.wsz, L"Search_PidlFolder");
                    bstr.cb = lstrlenW(bstr.wsz) * sizeof(WCHAR);
                    pwb->GetProperty(bstr.wsz, &var);
        
                    if (var.vt != VT_EMPTY)
                    {
                        _pidlStart = VariantToIDList(&var);
                        VariantClear(&var);
                        pwb->PutProperty(bstr.wsz, var);  // clear it out so only use once
                    }
                    pwb->Release();
                }
                psv->Release();
            }
            else
            {
                // appears to be race condition to load
                if (!_fDeferPidlStartTried)
                {
                    TraceMsg(TF_WARNING, "CDFCommand::MaybeRestoreSearch - QueryActiveShellView failed...");
                    _fDeferPidlStart = TRUE;
                    if (!_SetupBrowserCP())
                        _fDeferPidlStart = FALSE;
                }
            }
            psb->Release();
        }
    }

    return _pidlStart != NULL;
}

HRESULT CSearchNameSpaceOC::_AddDefaultItem(void)
{    
    _AddRecentFolderAndEntries(FALSE);
    ::SendMessage(_hwndComboBox, CB_SETCURSEL, (WPARAM)0, 0);     // Select "Recent"

    return S_OK;
}


// Implemention of our IDispatch to hookup to the top level browsers connnection point...
ULONG CSearchNameSpaceOC::CWBEvents2::AddRef(void)        
{
    return (SAFECAST(_pcsns, IComboBoxExOC *))->AddRef();
}

ULONG CSearchNameSpaceOC::CWBEvents2::Release(void)        
{
    return (SAFECAST(_pcsns, IComboBoxExOC *))->Release();
}

STDMETHODIMP CSearchNameSpaceOC::CWBEvents2::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    if ( riid == IID_IUnknown || riid == IID_IDispatch || riid == DIID_DWebBrowserEvents2
         || riid == DIID_DWebBrowserEvents){
        *ppvObj = (LPVOID)this;
        AddRef();
    }
    else
    {
        return E_NOINTERFACE;
    }
    return NOERROR;
}

STDMETHODIMP CSearchNameSpaceOC::CWBEvents2::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    if (_fWaitingForNavigate) {
        TraceMsg(TF_WARNING, "CSearchNameSpaceOC::CWBEvents2::Invoke dispid=%d.",dispidMember);
        if ((dispidMember == DISPID_NAVIGATECOMPLETE) || (dispidMember == DISPID_DOCUMENTCOMPLETE)) {
            // Assume this is ours... Should maybe check parameters...
            _fWaitingForNavigate = FALSE;

            // Now see if it is a case where we are to restore the search...
            if (_pcsns->_fDeferPidlStart)
            {
                _pcsns->_fDeferPidlStart = FALSE;
                _pcsns->_fDeferPidlStartTried = TRUE;
                if (_pcsns->_FGetStartingPidl())
                {
                    // Call the restoreuser input function which will
                    // if possible update the combobox...
                    _pcsns->_RestoreIfUserInputValue(TEXT(""));
                }
            }
        }
    }
    return S_OK;
}
