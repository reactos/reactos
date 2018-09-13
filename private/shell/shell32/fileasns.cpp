#include "shellprv.h"
#include "fileasns.h"

#include "unicpp\sdspatch.h"
#include <winuser.h>
#include "pidl.h"
#include "fstreex.h"

#define SZ_REGKEY_CTL_FILEASSOCIATIONS          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Controls\\File Associations")
#define COMBO_SUBCLASS_COOKIE         8410
#define EDIT_SUBCLASS_COOKIE          25987

// forwards...
LPVOID  _getData( HWND hwndComboBox ) ;
DWORD   _getID( HWND hwndComboBox ) ;

CFileAssocNameSpaceOC::CFileAssocNameSpaceOC() : CComboBoxExOC(NULL, 0)
{
    DllAddRef();

    // This allocator should have zero inited the memory, so assert the member variables are empty.
    ASSERT(!_fPopulated);

    // ATL needs these to make our window resize automatically.
    m_bWindowOnly = TRUE;
    m_bEnabled = TRUE;
    m_bRecomposeOnResize = TRUE;
    m_bResizeNatural = TRUE;
}

CFileAssocNameSpaceOC::~CFileAssocNameSpaceOC()
{
    DllRelease();
}


LRESULT CFileAssocNameSpaceOC::_OnDeleteItemMessage(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    // HACKHACK: combobox (comctl32\comboex.c) will pass a LPNMHDR, but it's really
    //	         a PNMCOMBOBOXEX (which has a first element of LPNMHDR).  This function
    //	         can use this type cast iff it's guaranteed that this will only come from
    //           a function that behaves in this perverse way.
    bHandled = FALSE ;
    return _DeleteFileAssocComboItem( pnmh ) ;
}

LRESULT _DeleteFileAssocComboItem( IN LPNMHDR pnmh )
{
    PNMCOMBOBOXEX pnmce = (PNMCOMBOBOXEX)pnmh;
    if (pnmce->ceItem.lParam)
    {
        // Is this a pidl?
        if ((pnmce->ceItem.lParam) > FILEASSOCIATIONSID_MAX)
        {
            // Yes, so let's free it.
            Str_SetPtr((LPTSTR *)&pnmce->ceItem.lParam, NULL);
        }
    }
    return 1L ;
}


LRESULT CFileAssocNameSpaceOC::_OnFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    // CComControlBase::InPlaceActivate() will call us back recursively, so ignore the second call. 
    if (_fInRecursion)
        return 1;       // We already did the set focus.

    _fInRecursion = TRUE;
    if (!m_bUIActive)
        CComControlBase::InPlaceActivate(OLEIVERB_UIACTIVATE);

    LRESULT lResult = ::SendMessage(_hwndComboBox, uMsg, wParam, lParam);
    bHandled = FALSE;
    _fInRecursion = FALSE;
    return 1;
}


LRESULT CFileAssocNameSpaceOC::SubClassWndProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam, WPARAM uIdSubclass, ULONG_PTR dwRefData)
{
    if ((COMBO_SUBCLASS_COOKIE == uIdSubclass) ||
        (EDIT_SUBCLASS_COOKIE == uIdSubclass))
    {
        switch (uMessage)
        {
        case WM_SETFOCUS:
            {
                BOOL fHandled;
                CFileAssocNameSpaceOC * pdtp = (CFileAssocNameSpaceOC *) dwRefData;

                if (EVAL(pdtp))
                    pdtp->_OnFocus(uMessage, wParam, lParam, fHandled);
            }
            break;
        case WM_DESTROY:
            RemoveWindowSubclass(hwnd, CFileAssocNameSpaceOC::SubClassWndProc, uIdSubclass);
            break;
        }
    }

    return DefSubclassProc(hwnd, uMessage, wParam, lParam);
}


HWND CFileAssocNameSpaceOC::Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT_PTR nID)
{
    m_hWnd = _CreateComboBoxEx(m_spClientSite, m_hWnd, hWndParent, rcPos, pszWindowName, dwStyle, dwExStyle, nID);
    if (EVAL(_hwndComboBox))
    {
        BOOL fSucceeded;
        HWND hwndComboBox = (HWND)::SendMessage(_hwndComboBox, CBEM_GETCOMBOCONTROL, 0, 0);

        if (EVAL(hwndComboBox))
        {
            fSucceeded = SetWindowSubclass(hwndComboBox, CFileAssocNameSpaceOC::SubClassWndProc, COMBO_SUBCLASS_COOKIE, (ULONG_PTR) this);
            ASSERT(fSucceeded);
        }

        if (_hwndEdit)  // User may not want combobox to be editable.
        {
            fSucceeded = SetWindowSubclass(_hwndEdit, CFileAssocNameSpaceOC::SubClassWndProc, EDIT_SUBCLASS_COOKIE, (ULONG_PTR) this);
            ASSERT(fSucceeded);
        }
    }

    return m_hWnd;
}


// IPersistPropertyBag
HRESULT CFileAssocNameSpaceOC::Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog)
{
    HRESULT hr = S_FALSE;

//    TraceMsg(TF_CUSTOM1, "in CUpDownOC::Load()");
    // TODO: Load in FILEASSOCIATIONS specific settings.

    hr = CComboBoxExOC::_Load(pPropBag, pErrorLog);
    return S_OK;
}

HRESULT CFileAssocNameSpaceOC::_IsDefaultSelection(LPCTSTR pszLastSelection)
{
    BOOL fIsDefault = FALSE;
    TCHAR szAllTypes[MAX_PATH];

    if (EVAL(LoadString(HINST_THISDLL, IDS_SNS_ALL_FILE_TYPES, szAllTypes, ARRAYSIZE(szAllTypes))) &&
        !StrCmp(szAllTypes, pszLastSelection))
    {
        fIsDefault = TRUE;
    }

    return fIsDefault;
}

HRESULT CFileAssocNameSpaceOC::_RestoreIfUserInputValue(LPCTSTR pszLastSelection)
{
    return S_FALSE; // say nope we did not handle this.
}


/****************************************************\
    FUNCTION: _PopulateTopItem

    DESCRIPTION:
        This function is optimizing for perf.  Since
    enumerating the drop down includes hitting the disk
    a bit, we want to optimize for the default case of
    "My Computer" being selected.
\****************************************************/
HRESULT CFileAssocNameSpaceOC::_PopulateTopItem(void)
{
    TCHAR szLastSelection[MAX_URL_STRING];
    DWORD cbSize = SIZEOF(szLastSelection);

    // Are we using the default setting?
    if ((ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_CTL_FILEASSOCIATIONS, (_pszPersistString ? _pszPersistString : SZ_REGVALUE_LAST_SELECTION), NULL, (LPVOID)szLastSelection, &cbSize)) ||
        _IsDefaultSelection(szLastSelection))
    {
        // Yes
        _AddResourceItem(IDS_SNS_ALL_FILE_TYPES, FILEASSOCIATIONSID_ALLFILETYPES, -1, LISTINSERT_LAST, ITEM_INDEX);         // All Files and Folders (*.*)
        ::SendMessage(_hwndComboBox, CB_SETCURSEL, (WPARAM)0, 0);     // Select "My Computer"
    }
    else
    {
        // No, so take the hit now.
        _Populate();
        _SetSelect(szLastSelection, szLastSelection);
    }

    return S_OK;
}


HRESULT CFileAssocNameSpaceOC::_GetSelectText(LPTSTR pszSelectText, DWORD cchSize, BOOL fDisplayName)
{
    LoadString(HINST_THISDLL, IDS_SNS_ALL_FILE_TYPES, pszSelectText, cchSize);
    return S_OK;
}

HRESULT CFileAssocNameSpaceOC::IOleInPlaceObject_InPlaceDeactivate(void)
{
    // Take this time to persist out selection.
    TCHAR szSelection[MAX_URL_STRING];

    _fPopulated = FALSE;    // This call will destroy the window.
    if (_fEnableEdit)
        ::GetWindowText(_hwndComboBox, szSelection, ARRAYSIZE(szSelection));
    else
        ::SendMessage(_hwndComboBox, CB_GETLBTEXT, (WPARAM)::SendMessage(_hwndComboBox, CB_GETCURSEL, 0, 0), (LPARAM)szSelection);

    SHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_CTL_FILEASSOCIATIONS, (_pszPersistString ? _pszPersistString : SZ_REGVALUE_LAST_SELECTION), REG_SZ, (LPVOID)szSelection, ARRAYSIZE(szSelection));
    return CComControlBase::IOleInPlaceObject_InPlaceDeactivate();
}

HRESULT CFileAssocNameSpaceOC::_Populate(void)
{
    if (_fPopulated)
        return S_OK;        // Done.

    _fPopulated = TRUE;

    ASSERT(_hwndComboBox);
    ::SendMessage(_hwndComboBox, CB_RESETCONTENT, 0, 0L);

    // Recent Folder
    _AddFileTypes( _hwndComboBox, NULL /* no callback */, 0 /* no callback param */ ) ;                                                                                            // each recent folder
    // Now add this to the top of the list.
    _AddResourceItem(IDS_SNS_ALL_FILE_TYPES, FILEASSOCIATIONSID_ALLFILETYPES, -1, LISTINSERT_FIRST, NO_ITEM_NOICON_INDEX);        // All Files and Folders (*.*)

    TCHAR szSelection[MAX_URL_STRING];
    if (EVAL(SUCCEEDED(_GetSelectText(szSelection, ARRAYSIZE(szSelection), TRUE))))
        _SetSelect(szSelection, szSelection);

    return S_OK;
}


BOOL ShouldAddToRecentDocs(LPCITEMIDLIST pidl)
{
    ASSERT(pidl);
    HKEY hk;
    BOOL fRet = TRUE;  //  default to true

    if (SHGetClassKey((LPCIDFOLDER) pidl, &hk, NULL))
    {
        if (GetFileTypeAttributes(hk) & FTA_NoRecentDocs)
            fRet = FALSE;

        SHCloseClassKey(hk);
    }

    return fRet;
}

// IComboBoxExOC
HRESULT CFileAssocNameSpaceOC::get_String(OUT BSTR *pbs)
{    
    if (EVAL(pbs && _hwndComboBox))
    {
        TCHAR szString[MAX_URL_STRING];
        if( _GetFileAssocComboSelItemText( _hwndComboBox, szString, ARRAYSIZE(szString) ) >= 0 )
            *pbs = SysAllocStringT(szString);
    }
    return S_OK;
}

LONG _GetFileAssocComboSelItemText( IN HWND hwndComboBox, OUT LPTSTR pszText, IN int cchText )
{
    ASSERT( hwndComboBox ) ;
    ASSERT( pszText ) ;
    *pszText = 0 ;

    int nSel ;
    if( (nSel = (LONG)::SendMessage( hwndComboBox, CB_GETCURSEL, 0, 0L )) >= 0 )
    {
        DWORD dwID = _getID( hwndComboBox );
        if (dwID > FILEASSOCIATIONSID_FILE_PATH)
            StrCpyN(pszText, TEXT(".*"), cchText);
        else
            StrCpyN(pszText, (LPCTSTR)_getData( hwndComboBox ), cchText );
    }
    return nSel ;
}

HRESULT CFileAssocNameSpaceOC::put_String(IN BSTR bs)
{    
    ASSERT(0);    // We are read only.
    return S_FALSE;
}

HRESULT CFileAssocNameSpaceOC::Reset(void)
{
    // The first item in the list should be the All Files and folders
    ::SendMessage(_hwndComboBox, CB_SETCURSEL, (WPARAM)0, 0); 
    return S_OK;
}


LPVOID _getData( HWND hwndComboBox )
{
    LRESULT nSelected = ::SendMessage( hwndComboBox, CB_GETCURSEL, 0, 0);

    if (-1 == nSelected)
        return NULL;

    return (LPVOID) ::SendMessage( hwndComboBox, CB_GETITEMDATA, nSelected, 0);
}


DWORD _getID( HWND hwndComboBox )
{
    DWORD dwID = 0;
    LPVOID pvData = _getData( hwndComboBox );

    // Is this an ID?
    if (pvData && ((DWORD_PTR)pvData <= FILEASSOCIATIONSID_MAX))
    {
        // Yes, so let's get it.
        dwID = PtrToUlong(pvData);
    }

    return dwID;
}
