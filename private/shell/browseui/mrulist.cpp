/**************************************************************\
    FILE: mrulist.cpp

    DESCRIPTION:
        CMRUList implements the Shell Name Space List or DriveList.
    This will store a pidl and be able to populate the AddressBand
    combobox with the shell name space that includes that PIDL.
\**************************************************************/

#include "priv.h"
#include "dbgmem.h"
#include "addrlist.h"
#include "itbar.h"
#include "itbdrop.h"
#include "util.h"
#include "autocomp.h"
#include <urlhist.h>
#include <winbase.h>
#include <wininet.h>

#define SUPERCLASS CAddressList



///////////////////////////////////////////////////////////////////
// #DEFINEs
#define MRU_LIST_MAX_CONST            25

///////////////////////////////////////////////////////////////////
// Data Structures
typedef struct tagSIZESTRCOMBO
{
    DWORD dwStringSize; // Size in Characters (not bytes)
    LPTSTR lpszString;
    int iImage;
    int iSelectedImage;
} SIZESTRCOMBO;

///////////////////////////////////////////////////////////////////
// Prototypes

/**************************************************************\
    CLASS: CMRUList

    DESCRIPTION:
        The MRU List will contain the type MRU history for the
    browser.  This is an IAddressList used by the Address Band/Bar.
\**************************************************************/

class CMRUList  : public CAddressList
                , public IMRU
                , public IPersistStream
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////

    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IAddressList methods ***
    virtual STDMETHODIMP Connect(BOOL fConnect, HWND hwnd, IBrowserService * pbs, IBandProxy * pbp, IAutoComplete * pac);
    virtual STDMETHODIMP Refresh(DWORD dwType);
    virtual STDMETHODIMP Save(void);

  // *** IPersistStream methods ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID){ *pClassID = CLSID_MRUList; return S_OK; }
    virtual STDMETHODIMP Load(IStream *pStm) { return S_OK; }
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);
    virtual STDMETHODIMP IsDirty(void) { return S_FALSE; }
    virtual STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize) { return E_NOTIMPL; }

    // *** IMRU methods ***
    virtual STDMETHODIMP AddEntry(LPCWSTR pszEntry);


    // IWinEventHandler
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);

protected:
    //////////////////////////////////////////////////////
    // Private Member Functions
    //////////////////////////////////////////////////////

    // Constructor / Destructor
    CMRUList();
    ~CMRUList(void);        // This is now an OLE Object and cannot be used as a normal Class.

    // Address List Modification Functions
    HRESULT _UpdateMRUEntry(LPCTSTR szNewValue, int nIndex, int iImage , int iISelectedImage);
    HRESULT _LoadList(void);
    HKEY _GetRegKey(BOOL fCreate);
    HRESULT _UpdateMRU(void);
    int _FindInMRU(LPCTSTR szURL);
    HRESULT _MRUMerge(HKEY kKey);
    BOOL _MoveAddressToTopOfMRU(int nMRUIndex);
    HRESULT _PopulateOneItem(void);
    HRESULT _Populate(void);
    void _InitCombobox(void);
    HRESULT _SetTopItem(void);

    // Friend Functions
    friend IAddressList * CMRUList_Create(void);

    //////////////////////////////////////////////////////
    //  Private Member Variables
    //////////////////////////////////////////////////////
    BOOL                _fDropDownPopulated;// Have we populated the drop down yet?
    BOOL                _fListLoaded;       // Have we loaded the Type-in MRU?
    BOOL                _fMRUUptodate;      // Is it necessary to update the MRU?
    BOOL                _fNeedToSave;      // Do we need to update the registry?
    SIZESTRCOMBO        _szMRU[MRU_LIST_MAX_CONST];  // MRU list.
    int                 _nMRUSize;          // Number of entries in MRU used.
};




//=================================================================
// Implementation of CMRUList
//=================================================================


/****************************************************\
    FUNCTION: CMRUList_Create

    DESCRIPTION:
        This function will create an instance of the
    CMRUList COM object.
\****************************************************/
IAddressList * CMRUList_Create(void)
{
    CMRUList *p = new CMRUList();

    return p;
}


/****************************************************\

    Address Band Constructor

\****************************************************/
CMRUList::CMRUList()
{
}


/****************************************************\

    Address Band destructor

\****************************************************/
CMRUList::~CMRUList()
{
    // loop through every potential saved URL in registry.
    if (_fListLoaded)
    {
        for (int nIndex = 0; (nIndex < MRU_LIST_MAX_CONST) && (_szMRU[nIndex].lpszString); nIndex++)
        {
            TrcLocalFree(_szMRU[nIndex].lpszString);
        }
    }
}


//===========================
// *** IUnknown Interface ***
HRESULT CMRUList::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IMRU))
    {
        *ppvObj = SAFECAST(this, IMRU*);
    }
    else if (IsEqualIID(riid, IID_IPersistStream))
    {
        *ppvObj = SAFECAST(this, IPersistStream*);
    }
    else
    {
        return SUPERCLASS::QueryInterface(riid,ppvObj);
    }

    AddRef();
    return S_OK;
}

ULONG CMRUList::AddRef()
{
    return SUPERCLASS::AddRef();
}

ULONG CMRUList::Release()
{
    return SUPERCLASS::Release();
}


//================================
// *** IAddressList Interface ***

/****************************************************\
    FUNCTION: Connect

    DESCRIPTION:
        We are either becoming the selected list for
    the AddressBand's combobox, or lossing this status.
    We need to populate or unpopulate the combobox
    as appropriate.
\****************************************************/

HRESULT CMRUList::Connect(BOOL fConnect, HWND hwnd, IBrowserService * pbs, IBandProxy * pbp, IAutoComplete * pac)
{
    HRESULT hr = S_OK;

    _fVisible = fConnect;
    if (!_hwnd)
        _hwnd = hwnd;
    ASSERT(_hwnd);

    if (fConnect)
    {
        // This needs to come before because it setups up
        // pointers that we need.
        SUPERCLASS::Connect(fConnect, hwnd, pbs, pbp, pac);
        //
        // Initial combobox parameters.
        //
        ASSERT(_pbp);
        if (_pbp->IsConnected() == S_FALSE) {

            // Do these tasks only the first time and only if
            // we are not in a browser window (because it will come
            // from the navigation complete).
            _PopulateOneItem();
        }
    }
    else
    {
        _UpdateMRU();    // Save contents of MRU because the ComboBox will be purged.
        _fDropDownPopulated = FALSE;

        // This call needs to come after _UpdateMRU() because
        // it releases pointers that we need.
        SUPERCLASS::Connect(fConnect, hwnd, pbs, pbp, pac);
    }
    return hr;
}


/****************************************************\
    FUNCTION: _SetTopItem

    DESCRIPTION:
        TODO.
\****************************************************/
HRESULT CMRUList::_SetTopItem(void)
{
    COMBOBOXEXITEM cbexItem = {0};
    LPCTSTR pszData = _szMRU[0].lpszString;

    if (pszData) {
        cbexItem.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
        cbexItem.iItem = -1;
        cbexItem.pszText =(LPTSTR)pszData;
        cbexItem.cchTextMax = lstrlen(pszData);
        if(_szMRU[0].iImage == -1 ||  _szMRU[0].iSelectedImage == -1) {
            _GetUrlUI(NULL,pszData, &(_szMRU[0].iImage),\
                                      &(_szMRU[0].iSelectedImage));
        }

        cbexItem.iImage =_szMRU[0].iImage;
        cbexItem.iSelectedImage = _szMRU[0].iSelectedImage;

        SendMessage(_hwnd, CBEM_SETITEM, (WPARAM)0, (LPARAM)(LPVOID)&cbexItem);
    }
    return S_OK;
}


//================================
// *** IMRU Interface ***
/*******************************************************************
    FUNCTION: AddEntry

    DESCRIPTION:
        Adds the specified URL to the top of the address bar
    combo box.  Limits the number of URLs in combo box to
    MRU_LIST_MAX_CONST.
********************************************************************/
HRESULT CMRUList::AddEntry(LPCWSTR pszEntry)
{
    HRESULT hr = S_OK;

    _fNeedToSave = TRUE;
    if (_fDropDownPopulated)
    {
        _ComboBoxInsertURL(pszEntry, MAX_URL_STRING, MRU_LIST_MAX_CONST);
        _fMRUUptodate = FALSE;
    }
    else
    {
        int nMRUIndex;

        if (!_fListLoaded)
            _LoadList();

        // Since we don't own the ComboBox, we need to add it to
        // the end of our MRU data.
        nMRUIndex = _FindInMRU(pszEntry);  // Now make it the top most.

        if (-1 != nMRUIndex)
        {
            // We already have this entry in our list, so all we need
            // to do is move it to the top.
            _MoveAddressToTopOfMRU(nMRUIndex);
            return hr;
        }


        for (nMRUIndex = 0; nMRUIndex < MRU_LIST_MAX_CONST; nMRUIndex++)
        {
            if (!_szMRU[nMRUIndex].lpszString)
            {   // We found an empty spot.
                _UpdateMRUEntry(pszEntry, nMRUIndex, -1, -1);
                break;  // We are done.
            }
        }



        if (MRU_LIST_MAX_CONST == nMRUIndex)
        {
            // The MRU is full so we will replace the last entry.
            _UpdateMRUEntry(pszEntry, --nMRUIndex, -1, -1);
        }

        _MoveAddressToTopOfMRU(nMRUIndex);  // Now make it the top most.
    }
    TraceMsg(TF_BAND|TF_GENERAL, "CMRUList: AddEntry(), URL=%s", pszEntry);
    return hr;
}


//================================
// *** IPersistStream Interface ***

/****************************************************\
    FUNCTION: Save

    DESCRIPTION:
        TODO.
\****************************************************/

HRESULT CMRUList::Save(IStream *pstm, BOOL fClearDirty)
{
    // BUGBUG: There currently is a bug in the shell that it leaks
    //         an object.  This causes CAddressBand to never be
    //         destructed when hosted in the TaskBar.
    //         Since we normally call IAddressList::SaveList() in the
    //         destructor, we will now need to call it
    //         from here in that case.
    if (_pbp && _pbp->IsConnected() == S_FALSE)
    {
        Save();
    }

    return S_OK;
}


/****************************************************\
    FUNCTION: _InitCombobox

    DESCRIPTION:
        Prepare the combo box for this list.  This normally
    means that the indenting and icon are either turned
    on or off.
\****************************************************/

void CMRUList::_InitCombobox()
{
    HIMAGELIST himlSysSmall;
    Shell_GetImageLists(NULL, &himlSysSmall);

    SendMessage(_hwnd, CBEM_SETIMAGELIST, 0, (LPARAM)himlSysSmall);
    SendMessage(_hwnd, CBEM_SETEXTENDEDSTYLE, 0,0);
    SUPERCLASS::_InitCombobox();
}

HKEY CMRUList::_GetRegKey(BOOL fCreate)
{
    BOOL fIsConnected = FALSE;
    HKEY hKey;
    DWORD result;
    LPCTSTR pszKey;

    if (_pbp)
        fIsConnected = (_pbp->IsConnected() == S_OK);
    if (fIsConnected)
        pszKey = SZ_REGKEY_TYPEDURLMRU;
    else
        pszKey = SZ_REGKEY_TYPEDCMDMRU;

    if (fCreate)
        result = RegCreateKey(HKEY_CURRENT_USER, pszKey, &hKey);
    else
        result = RegOpenKey(HKEY_CURRENT_USER, pszKey, &hKey);

    if (result != ERROR_SUCCESS)
        return NULL;

    return hKey;
}

/****************************************************\
    FUNCTION: _LoadList

    DESCRIPTION:
        When the ComboBox is switched to this MRU
    AddressList, the contents need to be populated.  Before
    that happens, we copy the data to the combobox
    from the registry.
\****************************************************/

HRESULT CMRUList::_LoadList(void)
{
    HRESULT hr = S_OK;
    HKEY hKey;
    DWORD dwCount;
    TCHAR szAddress[MAX_URL_STRING+1];

    ASSERT(!_fListLoaded);
    ASSERT(_hwnd);

    hKey = _GetRegKey(TRUE);
    ASSERT(hKey);
    if (!hKey)
        return E_FAIL;


    for (dwCount = 0; dwCount < MRU_LIST_MAX_CONST ; dwCount++)
    {
        hr = GetMRUEntry(hKey, dwCount, szAddress, SIZECHARS(szAddress), NULL);

        if (SUCCEEDED(hr))
            _UpdateMRUEntry(szAddress, (int)dwCount, -1, -1);
        else
        {
            _szMRU[dwCount].lpszString = NULL;
            _szMRU[dwCount].iImage = -1;
            _szMRU[dwCount].iSelectedImage = -1;
        }
    }


    RegCloseKey(hKey);
    _fListLoaded = TRUE;

    return hr;
}


/****************************************************\
    FUNCTION: Save

    DESCRIPTION:
        When this object is closed, we save the contents
    to the registry.
\****************************************************/

HRESULT CMRUList::Save(void)
{
    HRESULT hr = S_OK;
    HKEY hKey;
    DWORD result;
    TCHAR szValueName[10];   // big enough for "url99"
    int nCount;
    int nItems = (_fDropDownPopulated) ? ComboBox_GetCount(_hwnd) : _nMRUSize;

    if (!_fListLoaded || !_fNeedToSave) // Don't save the registry if we don't need to.
        return S_OK;

    if (!_fMRUUptodate)
        hr = _UpdateMRU();

    hKey = _GetRegKey(TRUE);
    ASSERT(hKey);
    if (!hKey)
        return E_FAIL;

    hr = _MRUMerge(hKey);  // Merge if the list has been modified.

    // loop through every potential saved URL in registry.
    for (nCount = 0; nCount < MRU_LIST_MAX_CONST; nCount++)
    {
        // make a value name a la "url1" (1-based for historical reasons)
        wnsprintf(szValueName, ARRAYSIZE(szValueName), SZ_REGVAL_MRUENTRY, nCount+1);

        // for every combo box item we have, get the corresponding
        // text and save it in the registry
        if (nCount < nItems && _szMRU[nCount].lpszString)
        {
            // store it in registry and off to the next one.
            result = SHSetValue(hKey, NULL, szValueName, REG_SZ, (CONST BYTE *) _szMRU[nCount].lpszString,
                    _szMRU[nCount].dwStringSize*SIZEOF(TCHAR));
        }
        else
        {
            // if we get here, we've run out of combo box items (or
            // failed to retrieve text for one of them).  Delete any
            // extra items that may be lingering in the registry.
            SHDeleteValue(hKey, NULL, szValueName);
        }
    }
    _fNeedToSave = FALSE;

    RegCloseKey(hKey);
    return hr;
}


/****************************************************\
    FUNCTION: _MRUMerge

    DESCRIPTION:
        This function will merge the current contents
    of the saved MRU.  This means that if the Address
    Band is being closed, it will load the MRU again
    because it could have been saved by a AddressBar
    that was recently closed down.  The merge happens
    like this: If the MRU is not full, items
    in the registry will be appended to the end
    of the MRU if they don't currently exist in the
    MRU.
\****************************************************/
HRESULT CMRUList::_MRUMerge(HKEY hKey)
{
    HRESULT hr = S_OK;
    UINT nCount;
    UINT nNextFreeSlot = _nMRUSize;
    long lResult;
    TCHAR szValueName[10];   // big enough for "url99"
    TCHAR szAddress[MAX_URL_STRING+1];
    DWORD dwAddress;

    ASSERT(_fListLoaded);
    ASSERT(hKey);


    for (nCount = 0; (nCount < MRU_LIST_MAX_CONST) && (nNextFreeSlot < MRU_LIST_MAX_CONST); nCount++)
    {
        // make a value name a la "url1" (1-based for historical reasons)
        wnsprintf(szValueName, ARRAYSIZE(szValueName), SZ_REGVAL_MRUENTRY, nCount+1);

        dwAddress = SIZEOF(szAddress);

        lResult = SHQueryValueEx(hKey, szValueName, NULL, NULL, (LPBYTE) szAddress, &dwAddress);
        if (ERROR_SUCCESS == lResult)
        {
            if (-1 == _FindInMRU(szAddress))
            {
                // We found a unique item.  Add it to our free slot.
                _UpdateMRUEntry(szAddress, nNextFreeSlot++, -1, -1);
            }
        }
        else
            break;
    }


    // BUGBUG: Because the AddressBand is always closed after all
    //         AddressBars when the shell is shut down, anything
    //         new in the AddressBars will be ignored if the
    //         MRU in the AddressBand is full.  Revisit.

    return hr;
}


/****************************************************\
    FUNCTION: _UpdateMRU

    DESCRIPTION:
        Save the contents of the Combobox because
    it will be purged for the next AddressList.
\****************************************************/
HRESULT CMRUList::_UpdateMRU(void)
{
    HRESULT hr = S_OK;
    TCHAR szAddress[MAX_URL_STRING+1];
    // get the number of items in combo box
    int nItems;
    int nCount;

    if (!_hwnd)
        return S_OK;

    if (!_fDropDownPopulated)
        return S_OK;        // Nothing to update.
    nItems = ComboBox_GetCount(_hwnd);

    ASSERT(_hwnd);

    // loop through every potential saved URL in registry.
    for (nCount = 0; nCount < MRU_LIST_MAX_CONST; nCount++)
    {
        // for every combo box item we have, get the corresponding
        // text and save it in our local array.
        if (nCount < nItems)
        {
            COMBOBOXEXITEM cbexItem = {0};

            cbexItem.mask = CBEIF_TEXT|CBEIF_IMAGE|CBEIF_SELECTEDIMAGE;
            cbexItem.pszText = szAddress;
            cbexItem.cchTextMax = ARRAYSIZE(szAddress);
            cbexItem.iItem = nCount;

            if (SendMessage(_hwnd, CBEM_GETITEM, 0, (LPARAM) &cbexItem))
            {
                hr = _UpdateMRUEntry(szAddress, nCount, cbexItem.iImage, cbexItem.iSelectedImage);
            }
        }
        else
        {
            if (_szMRU[nCount].lpszString)
            {
                // Free this array entry because it's not being used.
                TrcLocalFree(_szMRU[nCount].lpszString);
                _szMRU[nCount].lpszString = NULL;
                _szMRU[nCount].iImage = -1;
                _szMRU[nCount].iSelectedImage = -1;

                _nMRUSize--;
            }
        }
    }
    _fMRUUptodate = TRUE;

    TraceMsg(TF_BAND|TF_GENERAL, "CMRUList: _UpdateMRU().");
    return hr;
}


/****************************************************\
    FUNCTION: _UpdateMRUEntry

    DESCRIPTION:
        When the ComboBox is switched to this MRU
    AddressList, the contents need to be populated.  Before
    that happens, we copy the data to the combobox
    from the registry.
\****************************************************/
HRESULT CMRUList::_UpdateMRUEntry(LPCTSTR szNewValue, int nIndex, int iImage , int iSelectedImage)
{
    DWORD dwStrSize = lstrlen(szNewValue);

    if (!szNewValue)
    {
        // The caller wants us to free the string.
        if (_szMRU[nIndex].lpszString)
        {
            // We have a string that needs freeing.
            TrcLocalFree(_szMRU[nIndex].lpszString);
            _szMRU[nIndex].lpszString = NULL;
            _nMRUSize--;
        }
        return S_OK;
    }

    if (!(_szMRU[nIndex].lpszString))
    {
        // We need to create the string buffer
        _szMRU[nIndex].dwStringSize = dwStrSize+1;
        _szMRU[nIndex].lpszString = (LPTSTR) TrcLocalAlloc(LPTR, _szMRU[nIndex].dwStringSize*SIZEOF(TCHAR));
        if (!(_szMRU[nIndex].lpszString))
            return E_FAIL;
        _nMRUSize++;
    }

    if (dwStrSize + 1 > _szMRU[nIndex].dwStringSize)
    {
        // We need to increase the size of the buffer.
        TrcLocalFree(_szMRU[nIndex].lpszString);
        _szMRU[nIndex].dwStringSize = dwStrSize+1;
        _szMRU[nIndex].lpszString = (LPTSTR) TrcLocalAlloc(LPTR, _szMRU[nIndex].dwStringSize*SIZEOF(TCHAR));
        if (!(_szMRU[nIndex].lpszString))
            return E_FAIL;
    }

    lstrcpyn(_szMRU[nIndex].lpszString, szNewValue, _szMRU[nIndex].dwStringSize);
    _szMRU[nIndex].iImage = iImage;
    _szMRU[nIndex].iSelectedImage = iSelectedImage;


    return S_OK;
}


/****************************************************\
    FUNCTION: _Populate

    DESCRIPTION:
        fills in the entire combo.

    WARNING!!!!!!!!:
        This is expensive, don't do it unless absolutely
    necessary!
\****************************************************/
HRESULT CMRUList::_Populate(void)
{
    HRESULT hr = S_OK;
    CShellUrl *psu;

    if (!_fListLoaded)
        hr = _LoadList();  // Load Data

    if (_fDropDownPopulated)
        return S_OK;    // We are already populated.

    psu = new CShellUrl();

    if (psu)
    {
        // Give it a parent for displaying message boxes
        psu->SetMessageBoxParent(_hwnd);
            
        // read values from registry and put them in combo box
        COMBOBOXEXITEM cbexItem = {0};
        cbexItem.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;

        for (cbexItem.iItem = 0; cbexItem.iItem < MRU_LIST_MAX_CONST ; cbexItem.iItem++)
        {
            if (_szMRU[cbexItem.iItem].lpszString)
            {
                cbexItem.pszText = _szMRU[cbexItem.iItem].lpszString;
                cbexItem.cchTextMax = _szMRU[cbexItem.iItem].dwStringSize;

                //Do Image creation when we actually populate
                if(_szMRU[cbexItem.iItem].iImage == -1 ||   _szMRU[cbexItem.iItem].iSelectedImage == -1) {
                    _GetUrlUI(psu,_szMRU[cbexItem.iItem].lpszString, &(_szMRU[cbexItem.iItem].iImage),\
                                                               &(_szMRU[cbexItem.iItem].iSelectedImage));
                }

                // initialize the image indexes
                cbexItem.iImage = _szMRU[cbexItem.iItem].iImage;
                cbexItem.iSelectedImage = _szMRU[cbexItem.iItem].iSelectedImage;

                SendMessage(_hwnd, CBEM_INSERTITEM, (WPARAM)0, (LPARAM)(LPVOID)&cbexItem);
            }
            else
                break;  // Stop populating when we hit the max.
        }
        _fDropDownPopulated = TRUE;

        //Delete the shell url object
        delete psu;
    } else {
        // low mem
        hr = E_OUTOFMEMORY;
    }

    TraceMsg(TF_BAND|TF_GENERAL, "CMRUList: _Populate(). This is a VERY EXPENSIVE operation.");
    return hr;
}


/****************************************************\
    FUNCTION: _PopulateOneItem

    DESCRIPTION:
        This just files the ComboBox's edit control.
    We do this when we want to postpone populating
    the entire drop down list.
\****************************************************/
HRESULT CMRUList::_PopulateOneItem(void)
{
    HRESULT hr = S_OK;

    if (!_fListLoaded)
        hr = _LoadList();  // Load Data

    if (_fDropDownPopulated)
        return S_OK;    // We are already populated.

    hr = _SetTopItem();
    return hr;
}


/****************************************************\
    FUNCTION: Refresh

    DESCRIPTION:
        Update the URL in the Top of the list.
\****************************************************/
HRESULT CMRUList::Refresh(DWORD dwType)
{
    HRESULT hr = S_OK;

    if (OLECMD_REFRESH_ENTIRELIST == dwType)
    {
        // Force a refresh.  We don't move the contents
        // of the of the Combobox to the MRU because the
        // user wanted to refresh the Combobox because
        // it's contents may be tainted.
        SendMessage(_hwnd, CB_RESETCONTENT, 0, 0L);
        _fDropDownPopulated = FALSE;
    }

    return hr;
}


//================================
// *** Internal/Private Methods ***


/*******************************************************************
    FUNCTION: _MoveAddressToTopOfMRU

    PARAMETERS:
        nMRUIndex - index of be moved to top.

    DESCRIPTION:
        This function will move the specified index to the top of
    the list.
********************************************************************/
BOOL CMRUList::_MoveAddressToTopOfMRU(int nMRUIndex)
{
    int nCurrent;
    SIZESTRCOMBO sscNewTopItem;
    _fNeedToSave = TRUE;

    ASSERT(nMRUIndex < MRU_LIST_MAX_CONST);

    // Save off new top item info.
    sscNewTopItem.dwStringSize = _szMRU[nMRUIndex].dwStringSize;
    sscNewTopItem.lpszString = _szMRU[nMRUIndex].lpszString;
    sscNewTopItem.iImage  = _szMRU[nMRUIndex].iImage;
    sscNewTopItem.iSelectedImage = _szMRU[nMRUIndex].iSelectedImage;

    for (nCurrent = nMRUIndex; nCurrent > 0; nCurrent--)
    {
        // Move item down in list.
        _szMRU[nCurrent].dwStringSize = _szMRU[nCurrent-1].dwStringSize;
        _szMRU[nCurrent].lpszString = _szMRU[nCurrent-1].lpszString;
        _szMRU[nCurrent].iImage = _szMRU[nCurrent-1].iImage;
        _szMRU[nCurrent].iSelectedImage = _szMRU[nCurrent-1].iSelectedImage;
    }

    // Set new top item.
    _szMRU[0].dwStringSize    = sscNewTopItem.dwStringSize;
    _szMRU[0].lpszString      = sscNewTopItem.lpszString;
    _szMRU[0].iImage          = sscNewTopItem.iImage;
    _szMRU[0].iSelectedImage  = sscNewTopItem.iSelectedImage;

    return TRUE;
}


/*******************************************************************
    FUNCTION: _FindInMRU

    PARAMETERS:
        szURL - URL to see if it exists in list.

    DESCRIPTION:
        Search through MRU for URL.  Return value will be -1 if not
    found or index if found.
********************************************************************/
int CMRUList::_FindInMRU(LPCTSTR szURL)
{
    int nCurrent;

    for (nCurrent = 0; (nCurrent < MRU_LIST_MAX_CONST) && _szMRU[nCurrent].lpszString; nCurrent++)
    {
        if (0 == StrCmpN(_szMRU[nCurrent].lpszString, szURL, _szMRU[nCurrent].dwStringSize))
        {
            // We found it.
            return nCurrent;
        }
    }

    return -1;
}



HRESULT CMRUList::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    switch(uMsg) {
    case WM_WININICHANGE:
    {
        HKEY hkey = _GetRegKey(FALSE);
        if (hkey) {
            RegCloseKey(hkey);
        } else {

            // reset if the key is gone
            if (_fVisible) {
                SendMessage(_hwnd, CB_RESETCONTENT, 0, 0L);
            }
            _fDropDownPopulated = FALSE;
            _fListLoaded = FALSE;
        }
    }
        break;
    }

    return SUPERCLASS::OnWinEvent(hwnd, uMsg, wParam, lParam, plres);
}
