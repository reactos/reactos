/**************************************************************\
    FILE: snslist.cpp

    DESCRIPTION:
        SNSList implements the Shell Name Space List or DriveList.
    This will store a pidl and be able to populate the AddressBand
    combobox with the shell name space that includes that PIDL.
\**************************************************************/

#include "priv.h"

#ifndef UNIX

#include "dbgmem.h"
#include "addrlist.h"
#include "itbar.h"
#include "itbdrop.h"
#include "util.h"
#include "autocomp.h"
#include <urlhist.h>
#include <winbase.h>
#include <wininet.h>



///////////////////////////////////////////////////////////////////
// Data Structures
typedef struct {
    LPITEMIDLIST pidl;          // the pidl
    TCHAR szName[MAX_URL_STRING];     // pidl's display name
    int iImage;                 // pidl's icon
    int iSelectedImage;         // pidl's selected icon
} PIDLCACHE, *PPIDLCACHE;


/**************************************************************\
    CLASS: CSNSList

    DESCRIPTION:
        This object supports IAddressList and can populate
    the Address Band/Bar with the Shell Name Space (DriveList)
    heirarchy.
\**************************************************************/
class CSNSList  : public CAddressList
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IAddressList methods ***
    virtual STDMETHODIMP Connect(BOOL fConnect, HWND hwnd, IBrowserService * pbs, IBandProxy * pbp, IAutoComplete * pac);
    virtual STDMETHODIMP NavigationComplete(LPVOID pvCShellUrl);
    virtual STDMETHODIMP Refresh(DWORD dwType);
    virtual STDMETHODIMP SetToListIndex(int nIndex, LPVOID pvShelLUrl);
    virtual STDMETHODIMP FileSysChangeAL(DWORD dw, LPCITEMIDLIST* ppidl);

protected:
    //////////////////////////////////////////////////////
    // Private Member Functions
    //////////////////////////////////////////////////////

    // Constructor / Destructor
    CSNSList();
    ~CSNSList(void);        // This is now an OLE Object and cannot be used as a normal Class.


    // Address Band Specific Functions
    LRESULT _OnNotify(LPNMHDR pnm);
    LRESULT _OnCommand(WPARAM wParam, LPARAM lParam);

    // Address List Modification Functions
    void _AddItem(LPITEMIDLIST pidl, int iInsert, int iIndent);
    LPITEMIDLIST _GetFullIDList(int iItem);
    int _GetIndent(int iItem);
    void _FillOneLevel(int iItem, int iIndent, int iDepth);
    void _ExpandMyComputer(int iDepth);
    LPITEMIDLIST _GetSelectedPidl(void);
    int _FindItem(LPITEMIDLIST pidl);
    BOOL _SetCachedPidl(LPCITEMIDLIST pidl);
    BOOL _GetPidlUI(LPCITEMIDLIST pidl, LPTSTR pszName, int cchName, int *piImage, int *piSelectedImage, DWORD dwFlags, BOOL fIgnoreCache);
    BOOL _GetPidlImage(LPCITEMIDLIST pidl, int *piImage, int *piSelectedImage);
    LRESULT _OnGetDispInfoA(PNMCOMBOBOXEXA pnmce);
    LRESULT _OnGetDispInfoW(PNMCOMBOBOXEXW pnmce);
    void _PurgeComboBox();
    void _PurgeAndResetComboBox();

    LPITEMIDLIST CSNSList::_GetDragDropPidl(LPNMCBEDRAGBEGINW pnmcbe);
    HRESULT _GetURLToolTip(LPTSTR pszUrl, DWORD dwStrSize);        
    HRESULT _GetPIDL(LPITEMIDLIST* ppidl);
    BOOL _IsSelectionValid(void);
    HRESULT _PopulateOneItem(BOOL fIgnoreCache = FALSE);
    HRESULT _Populate(void);
    void _InitCombobox(void);
    // Friend Functions
    friend IAddressList * CSNSList_Create(void);

    //////////////////////////////////////////////////////
    //  Private Member Variables 
    //////////////////////////////////////////////////////
    PIDLCACHE           _cache;             // cache of pidl UI information

    BOOL                _fFullListValid:1;  // TRUE when the full combo is correctly populated
    BOOL                _fPurgePending:1;   // TRUE if we should purge when the combo closes up
};



//================================================================= 
// Implementation of CSNSList
//=================================================================


/****************************************************\
    FUNCTION: CSNSList_Create
  
    DESCRIPTION:
        This function will create an instance of the
    CSNSList COM object.
\****************************************************/
IAddressList * CSNSList_Create(void)
{
    CSNSList * p = new CSNSList();
    return p;
}


/****************************************************\
  
    Address Band Constructor
  
\****************************************************/
CSNSList::CSNSList()
{
    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!_cache.pidl);
}


/****************************************************\
  
    Address Band destructor
  
\****************************************************/
CSNSList::~CSNSList()
{
    if (_cache.pidl)
        ILFree(_cache.pidl);

    _PurgeComboBox();

    TraceMsg(TF_SHDLIFE, "dtor CSNSList %x", this);
}


//================================
// *** IAddressList Interface ***


void CSNSList::_PurgeComboBox()
{
    if (_hwnd)
    {
        // Deleting items from the combobox trashes the edit button if something was
        // previously selected from the combobox.  So we want to restore the editbox
        // when we are done
        WCHAR szBuf[MAX_URL_STRING];
        *szBuf = NULL;
        GetWindowText(_hwnd, szBuf, ARRAYSIZE(szBuf));
        SendMessage(_hwnd, WM_SETREDRAW, FALSE, 0);

        // Delete the PIDL of every item and then free the item
        INT iMax = (int)SendMessage(_hwnd, CB_GETCOUNT, 0, 0);
        
        while(iMax > 0)
        {
            // Each call to DeleteItem results in a callback
            // which frees the corresponding PIDL
            // if you simply use CB_RESETCONTENT - you don't get the callback
            iMax = (int)SendMessage(_hwnd, CBEM_DELETEITEM, (WPARAM)0, (LPARAM)0);
        }

        // Restore the contents of the editbox
        SetWindowText(_hwnd, szBuf);
        SendMessage(_hwnd, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(_hwnd, NULL, FALSE);
    }
    _fFullListValid = FALSE;
}

void CSNSList::_PurgeAndResetComboBox()
{
    _PurgeComboBox();
    if (_hwnd)
    {
        SendMessage(_hwnd, CB_RESETCONTENT, 0, 0L);
    }
}

/****************************************************\
    DESCRIPTION:
        We are either becoming the selected list for
    the AddressBand's combobox, or lossing this status.
    We need to populate or unpopulate the combobox
    as appropriate.
\****************************************************/
HRESULT CSNSList::Connect(BOOL fConnect, HWND hwnd, IBrowserService * pbs, IBandProxy * pbp, IAutoComplete * pac)
{
    _PurgeComboBox();

    HRESULT hr = CAddressList::Connect(fConnect, hwnd, pbs, pbp, pac);
    
    if (fConnect)
    {
        _PopulateOneItem();
    }
    else
    {
        // Get the pidl of the currently displayed item and destroy it
        COMBOBOXEXITEM cbexItem = {0};
        cbexItem.iItem = -1;
        cbexItem.mask = CBEIF_LPARAM;
        SendMessage(_hwnd, CBEM_GETITEM, 0, (LPARAM)&cbexItem);
        LPITEMIDLIST pidlPrev = (LPITEMIDLIST)cbexItem.lParam;
        if (pidlPrev)
        {
            ILFree(pidlPrev);
            cbexItem.lParam = NULL;
            SendMessage(_hwnd, CBEM_SETITEM, 0, (LPARAM)&cbexItem);
        }
    }

    return hr;
}



/****************************************************\
    FUNCTION: _InitCombobox
    
    DESCRIPTION:
        Prepare the combo box for this list.  This normally
    means that the indenting and icon are either turned
    on or off.
\****************************************************/
void CSNSList::_InitCombobox()
{
    HIMAGELIST himlSysSmall;
    Shell_GetImageLists(NULL, &himlSysSmall);

    SendMessage(_hwnd, CBEM_SETIMAGELIST, 0, (LPARAM)himlSysSmall);
    SendMessage(_hwnd, CBEM_SETEXSTYLE, 0, 0);
    CAddressList::_InitCombobox();    
}


/****************************************************\
  
    FUNCTION: _IsSelectionValid
  
    DESCRIPTION:
        Is the current selection valid?
\****************************************************/
BOOL CSNSList::_IsSelectionValid(void)
{
    LPITEMIDLIST pidlCur, pidlSel;
    BOOL fValid = S_OK;

    _GetPIDL(&pidlCur);
    pidlSel = _GetSelectedPidl();

    if (pidlCur == pidlSel)
    {
        fValid = TRUE;
    }
    else if ((pidlCur == NULL) || (pidlSel == NULL))
    {
        fValid = FALSE;
    }
    else
    {
        //
        // ILIsEqual faults on NULL pidls, sigh
        //
        fValid = ILIsEqual(pidlCur, pidlSel);
    }
    ILFree(pidlCur);

    return fValid;
}


/****************************************************\
    FUNCTION: NavigationComplete
  
    DESCRIPTION:
        Update the URL in the Top of the list.
\****************************************************/
HRESULT CSNSList::NavigationComplete(LPVOID pvCShellUrl)
{
    CShellUrl * psu = (CShellUrl *) pvCShellUrl;
    ASSERT(pvCShellUrl);
    LPITEMIDLIST pidl;
    HRESULT hr = psu->GetPidl(&pidl);
    if (SUCCEEDED(hr))
    {
        // Update current PIDL.
        if (_SetCachedPidl(pidl))
            hr = _PopulateOneItem();

        ILFree(pidl);
    }

    return hr;
}


/****************************************************\
    FUNCTION: Refresh
  
    DESCRIPTION:
        This call will invalidate the contents of the
    contents of the drop down as well as refresh the
    Top Most icon and URL.
\****************************************************/
HRESULT CSNSList::Refresh(DWORD dwType)
{
    if (!_hwnd)
        return S_OK;    // Don't need to do any work.

    // Full refresh (ignore the cache) because the full path
    // style bit may have changed
    return _PopulateOneItem(TRUE);
}


/****************************************************\
  
    DESCRIPTION:
        Puts the current pidl into the combobox.
    This is a sneaky perf win.  Since most of the time users
    don't drop down the combo, we only fill it in with the
    (visible) current selection.
    We need to destroy the PIDL of the currently displayed item
    first though
  
\****************************************************/
HRESULT CSNSList::_PopulateOneItem(BOOL fIgnoreCache)
{
    LPITEMIDLIST pidlCur;
    HRESULT hr = S_OK;

    _fFullListValid = FALSE;
    //
    // First easy out - if there is no current pidl,
    // do nothing.
    //
    if (SUCCEEDED(_GetPIDL(&pidlCur)) && pidlCur)
    {
        DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
        TraceMsg(TF_BAND|TF_GENERAL, "CSNSList: _PopulateOneItem(), and Pidl not in ComboBox. PIDL=>%s<", Dbg_PidlStr(pidlCur, szDbgBuffer, SIZECHARS(szDbgBuffer)));
        ASSERT(_hwnd);
        TCHAR szURL[MAX_URL_STRING];

        COMBOBOXEXITEM cbexItem = {0};
        // Get the pidl of the currently displayed item and destroy it
        cbexItem.iItem = -1;
        cbexItem.mask = CBEIF_LPARAM;
        SendMessage(_hwnd, CBEM_GETITEM, 0, (LPARAM)&cbexItem);
        // we only free pidlPrev if we can sucessfully set the new item in...
        LPITEMIDLIST pidlPrev = (LPITEMIDLIST)cbexItem.lParam;
        
        // Done - so go insert the new item
        cbexItem.iItem = -1;
        cbexItem.pszText = szURL;
        cbexItem.cchTextMax = ARRAYSIZE(szURL);
        cbexItem.iIndent = 0;
        cbexItem.lParam = (LPARAM)ILClone(pidlCur);
        cbexItem.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_INDENT | CBEIF_LPARAM;

        _GetPidlUI(pidlCur, szURL, cbexItem.cchTextMax, &cbexItem.iImage,
                   &cbexItem.iSelectedImage, SHGDN_FORPARSING, fIgnoreCache);
        if (!*szURL)
        {
            // Navigating to a net unc in browser-only doesn't work so try again without cache and FORPARSING
            _GetPidlUI(pidlCur, szURL, cbexItem.cchTextMax, &cbexItem.iImage,
                   &cbexItem.iSelectedImage, SHGDN_NORMAL, TRUE);
        }

        TraceMsg(TF_BAND|TF_GENERAL, "CSNSList::_PopulateOneItem(), Name=>%s<", cbexItem.pszText);
        LRESULT lRes = SendMessage(_hwnd, CBEM_SETITEM, 0, (LPARAM)&cbexItem);
        if ((CB_ERR == lRes) || (0 == lRes))
        {
            // Since we didn't insert the item, free the cloned pidl
            ILFree((LPITEMIDLIST) cbexItem.lParam);
        }
        else
        {
            // since we inserted the item, free the previous one
            if (pidlPrev)
            {
                ILFree(pidlPrev);
            }
        }

        ILFree(pidlCur);
    }
    return hr;
}

/****************************************************\
  
    DESCRIPTION:
        fills in the entire combo.
  
    WARNING!!!!!!!!:
    *** This is expensive, don't do it unless absolutely necessary! ***
  
\****************************************************/
HRESULT CSNSList::_Populate(void)
{
    LPITEMIDLIST pidl = NULL;
    int iIndent, iDepth;
    HRESULT hr = S_OK;

    if (_fFullListValid)
        return S_OK;  // Not needed, the drop down is already up todate.

    ASSERT(_hwnd);
    _PurgeAndResetComboBox();

    //
    // Fill in the current pidl and all it's parents.
    //
    hr = _GetPIDL(&pidl);

    iDepth = 0;
    iIndent = 0;

    if (pidl)
    {
        //
        // Compute the relative depth of pidl from the root.
        //
        LPITEMIDLIST pidlChild = pidl;
        if (ILIsRooted(pidl))
            pidlChild = ILGetNext(pidl);

        ASSERT(pidlChild);

        if (pidlChild)
        {
            //
            // Compute the maximum indentation level.
            //
            while (!ILIsEmpty(pidlChild))
            {
                pidlChild = _ILNext(pidlChild);
                iIndent++;
            }

            //
            // Save the maximum level.
            //
            iDepth = iIndent;
            
            //
            // Insert all those pidls.
            //
            LPITEMIDLIST pidlTemp = ILClone(pidl);

            do
            {
                _AddItem(pidlTemp, 0, iIndent);

                ILRemoveLastID(pidlTemp);
                iIndent--;
            } while (iIndent >= 0);
            ILFree(pidlTemp);
        }
        
        // Expand the root item.
        _FillOneLevel(0, 1, iDepth);

        // If this is not a rooted explorer, we expand MyComputer as well.
        // This is where we get our name "the drives dropdown".
        if (!ILIsRooted(pidl))
            _ExpandMyComputer(iDepth);
    }

    ILFree(pidl);
    _fFullListValid = TRUE;
    return hr;
} 


//================================
// *** Internal/Private Methods ***

//=================================================================
// General Band Functions
//=================================================================


/****************************************************\
    FUNCTION: _OnNotify
  
    DESCRIPTION:
        This function will handle WM_NOTIFY messages.
\****************************************************/
LRESULT CSNSList::_OnNotify(LPNMHDR pnm)
{
    LRESULT lReturn = 0;
    // HACKHACK: combobox (comctl32\comboex.c) will pass a LPNMHDR, but it's really
    // a PNMCOMBOBOXEX (which has a first element of LPNMHDR).  This function
    // can use this type cast iff it's guaranteed that this will only come from
    // a function that behaves in this perverse way.
    PNMCOMBOBOXEX pnmce = (PNMCOMBOBOXEX)pnm;

    ASSERT(pnm);
    switch (pnm->code)
    {
        case TTN_NEEDTEXT:
        {
            LPTOOLTIPTEXT pnmTT = (LPTOOLTIPTEXT)pnm;
            _GetURLToolTip(pnmTT->szText, ARRAYSIZE(pnmTT->szText));
            break;
        }

        case CBEN_DRAGBEGINA:
        {
            LPNMCBEDRAGBEGINA pnmbd = (LPNMCBEDRAGBEGINA)pnm;
            _OnDragBeginA(pnmbd);
            break;
        }

        case CBEN_DRAGBEGINW:

        {
            LPNMCBEDRAGBEGINW pnmbd = (LPNMCBEDRAGBEGINW)pnm;
            _OnDragBeginW(pnmbd);
            break;
        }

        case CBEN_GETDISPINFOW:
            _OnGetDispInfoW((PNMCOMBOBOXEXW)pnmce);
            break;

        case CBEN_GETDISPINFOA:
            _OnGetDispInfoA((PNMCOMBOBOXEXA) pnmce);
            break;

        case CBEN_DELETEITEM:
            if (pnmce->ceItem.lParam)
                ILFree((LPITEMIDLIST)pnmce->ceItem.lParam);
            break;

        default:
            lReturn = CAddressList::_OnNotify(pnm);
            break;
    }

    return lReturn;
}

/****************************************************\
    FUNCTION: _OnCommand
  
    DESCRIPTION:
        This function will handle WM_COMMAND messages.
\****************************************************/
LRESULT CSNSList::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch (GET_WM_COMMAND_CMD(wParam, lParam))
    {
    case CBN_CLOSEUP:
        if (_fPurgePending)
        {
            _fPurgePending = FALSE;
            _PurgeAndResetComboBox();
        }
        break;
    }

    return CAddressList::_OnCommand(wParam, lParam);
}

/****************************************************\
    PARAMETERS:
        LPSTR pszUrl - String Buffer that will contain the
                      URL as output.
        DWORD dwStrSize - Size of String buffer in characters.
  
    DESCRIPTION:
        Get the current URL.
\****************************************************/
HRESULT CSNSList::_GetURLToolTip(LPTSTR pszUrl, DWORD dwStrSize)
{
    ASSERT(pszUrl);
    if (!pszUrl)
        return E_INVALIDARG;

    LPITEMIDLIST pidlCur;
    HRESULT hr = _GetPIDL(&pidlCur); 
    if (S_OK == hr)
    {
        TCHAR szPidlName[MAX_URL_STRING];
        _GetPidlUI(pidlCur, szPidlName, ARRAYSIZE(szPidlName), NULL, NULL, SHGDN_FORPARSING, FALSE);
        lstrcpyn(pszUrl, szPidlName, dwStrSize);
        ILFree(pidlCur);
    }
    else
        pszUrl[0] = 0;

    return hr; 
}


/****************************************************\
    FUNCTION: _GetPIDL
  
    DESCRIPTION:
        This function returns a pointer to the current
    PIDL.  The caller will need to free the PIDL when
    it's no longer needed.  S_FALSE will be returned
    if there isn't a current PIDL.
\****************************************************/
HRESULT CSNSList::_GetPIDL(LPITEMIDLIST * ppidl)
{
    TraceMsg(TF_BAND|TF_GENERAL, "CSNSList: _GetPIDL() Begin");
    ASSERT(ppidl);
    if (!ppidl)
        return E_INVALIDARG;

    *ppidl = NULL;

    if (!_pbs)
    {
        DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
        TraceMsg(TF_BAND|TF_GENERAL, "CSNSList: _GetPIDL(), _cache.pidl=>%s<", Dbg_PidlStr(_cache.pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));
        if (_cache.pidl)
            *ppidl = ILClone(_cache.pidl);
    }
    else
    {
        _pbs->GetPidl(ppidl);

        DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
        TraceMsg(TF_BAND|TF_GENERAL, "CSNSList: _GetPIDL(), Current Pidl in TravelLog. PIDL=>%s<", Dbg_PidlStr(*ppidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));
    }

    if (*ppidl)
        return S_OK;

    TraceMsg(TF_BAND|TF_GENERAL, "CSNSList: _GetPIDL() End");
    return S_FALSE;
}




/****************************************************\
  
    _AddItem - Adds one pidl to the address window
  
    Input:
        pidl - the pidl to add
        iInsert - where to insert
        iIndent - indentation level of pidl
  
\****************************************************/
void CSNSList::_AddItem(LPITEMIDLIST pidl, int iInsert, int iIndent)
{
    COMBOBOXEXITEM cei;
    DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
    TraceMsg(TF_BAND|TF_GENERAL, "CSNSList: _AddItem(). PIDL=>%s<", Dbg_PidlStr(pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));

    cei.pszText = LPSTR_TEXTCALLBACK;
    cei.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_INDENT | CBEIF_LPARAM;
    cei.lParam = (LPARAM)ILClone(pidl);
    cei.iIndent = iIndent;
    cei.iItem = iInsert;
    cei.iImage = I_IMAGECALLBACK;
    cei.iSelectedImage = I_IMAGECALLBACK;
    ASSERT(_hwnd);
    SendMessage(_hwnd, CBEM_INSERTITEM, 0, (LPARAM)&cei);
}


/****************************************************\
  
    _GetFullIDList - Get the pidl associated with a combo index
  
    Input:
        iItem - the item to retrieve
  
    Return:
        The pidl at that index.
        NULL on error.
  
\****************************************************/
LPITEMIDLIST CSNSList::_GetFullIDList(int iItem)
{
    LPITEMIDLIST pidl;
    
    ASSERT(_hwnd);
    pidl = (LPITEMIDLIST)SendMessage(_hwnd, CB_GETITEMDATA, iItem, 0);
    if (pidl == (LPITEMIDLIST)CB_ERR)
    {
        pidl = NULL;
    }
    
    return pidl;
}


/****************************************************\
  
    _GetIndent - Get the indentation level of a combo index
  
    Input:
        iItem - the item to retrieve
  
    Return:
        The indentation level.
        -1 on error.
  
\****************************************************/
int CSNSList::_GetIndent(int iItem)
{
    int iIndent;
    COMBOBOXEXITEM cbexItem;

    cbexItem.mask = CBEIF_INDENT;
    cbexItem.iItem = iItem;
    ASSERT(_hwnd);
    if (SendMessage(_hwnd, CBEM_GETITEM, 0, (LPARAM)&cbexItem))
    {
        iIndent = cbexItem.iIndent;
    }
    else
    {
        iIndent = -1;
    }
    
    return iIndent;
}


/****************************************************\
    FUNCTION: _ExpandMyComputer
  
    DESCRIPTION:
        Find the "My Computer" entry in the drop down
    list and expand it.
\****************************************************/
void CSNSList::_ExpandMyComputer(int iDepth)
{
    LPITEMIDLIST pidlMyComputer = NULL;
    
    SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlMyComputer);
    if (pidlMyComputer)
    {
        LPITEMIDLIST pidl = NULL;
        BOOL fFound = FALSE;
        int nIndex = 0;

        while (pidl = _GetFullIDList(nIndex))
        {
            if (ILIsEqual(pidl, pidlMyComputer))
            {
                fFound = TRUE;
                break;
            }
            nIndex++;
        }
    
        if (fFound)
        {
            _FillOneLevel(nIndex, 2, iDepth);
        }

        ILFree(pidlMyComputer);
    }
}

/****************************************************\
  
    _FillOneLevel - find and add all of the children of one combo item
  
    Input:
        iItem - the item to expand
        iIndent - the indentation level of the children to add
        iDepth - the deepest indented item currently in the list
  
\****************************************************/
void CSNSList::_FillOneLevel(int iItem, int iIndent, int iDepth)
{
    LPITEMIDLIST pidl;
    
    pidl = _GetFullIDList(iItem);
    
    if (pidl)
    {
        HDPA hdpa;

        //
        // Fill hdps with all the children of this pidl.
        //
        hdpa = GetSortedIDList(pidl);
        if (hdpa)
        {
            int iCount, iInsert, i;
            LPITEMIDLIST pidlAlreadyThere;

            iCount = DPA_GetPtrCount(hdpa);

            //
            // The insert point starts right after parent.
            //
            iInsert = iItem + 1;

            //
            // Examine the next item.  If it is at the same level as
            // our soon-to-be-added children, remember it so we don't add
            // it twice.
            //
            pidlAlreadyThere = _GetFullIDList(iInsert);
            if (pidlAlreadyThere && (_GetIndent(iInsert) != iIndent))
            {
                pidlAlreadyThere = NULL;
            }

            //
            // Loop through each child.
            //
            for (i=0; i<iCount; i++, iInsert++)
            {
                LPITEMIDLIST pidlChild = (LPITEMIDLIST)DPA_GetPtr(hdpa, i);
                LPITEMIDLIST pidlInsert = ILClone(pidl);

                if (pidlInsert)
                {
                    ASSERT((LPVOID)pidlChild == (LPVOID)&pidlChild->mkid);
                    pidlInsert = ILAppendID(pidlInsert, &pidlChild->mkid, TRUE);
                    
                    //
                    // If this item was already added, we need to skip over it
                    // and all of its children that have been inserted.
                    // Because we know how this list was constructed,
                    // we know the number of items is iDepth-iIndent.
                    //
                    if (pidlAlreadyThere && ILIsEqual(pidlInsert, pidlAlreadyThere))
                    {
                        //
                        // Skip over this item (it's already been added) and
                        // its children.
                        //
                        iInsert += iDepth - iIndent;
                    }
                    else
                    {
                        _AddItem(pidlInsert, iInsert, iIndent);
                    }

                    ILFree(pidlInsert);
                }
            }
            
            FreeSortedIDList(hdpa);
        }
    }
}


/****************************************************\
  
    _GetSelectedPidl - return the pidl of the combo selection
  
    Return:
        The selected pidl.
        NULL on error.
  
\****************************************************/
LPITEMIDLIST CSNSList::_GetSelectedPidl(void)
{
    LPITEMIDLIST pidl = NULL;
    int iSel;
    
    ASSERT(_hwnd);
    iSel = ComboBox_GetCurSel(_hwnd);
    if (iSel >= 0)
    {
        pidl = _GetFullIDList(iSel);
    }

    DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
    TraceMsg(TF_BAND|TF_GENERAL, "CSNSList: _GetSelectedPidl(). PIDL=>%s<", Dbg_PidlStr(pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));
    return pidl;
}


/****************************************************\
    _FindItem - return the combo index associated with a pidl.
    Input:
        pidl - the pidl to find.
  
    Return:
        The combo index.
        -1 on error.
\****************************************************/
int CSNSList::_FindItem(LPITEMIDLIST pidl)
{
    LPITEMIDLIST pidlCombo;
    int i = 0;
    int iRet = -1;
    int iMax;

    if (!pidl)
        return iRet;    // Return -1 to show that nothing is selected.

    ASSERT(_hwnd);
    iMax = (int)SendMessage(_hwnd, CB_GETCOUNT, 0, 0);

    for (i=0; i<iMax; i++)
    {
        pidlCombo = (LPITEMIDLIST)SendMessage(_hwnd, CB_GETITEMDATA, i, 0);

        DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
        TraceMsg(TF_BAND|TF_GENERAL, "CSNSList: _FindItem(), ENUM ComboBox Pidls. PIDL=>%s<", Dbg_PidlStr(pidlCombo, szDbgBuffer, SIZECHARS(szDbgBuffer)));

        if (pidlCombo && IEILIsEqual(pidl, pidlCombo, TRUE))
        {
            iRet = i;
            break;
        }
    }

    return iRet;
}


/****************************************************\
    FUNCTION: _GetPidlImage
  
    PARAMETERS:
        pidl - the pidl to get the icon index.
        piImage - Pointer to location to store result. (OPTIONAL)
        piSelectedImage - Pointer to location to store result. (OPTIONAL)
  
    DESCRIPTION:
        This function will retrieve information about the
    icon index for the pidl.
\****************************************************/
BOOL CSNSList::_GetPidlImage(LPCITEMIDLIST pidl, int *piImage, int *piSelectedImage)
{
    int * piImagePriv = piImage;
    int * piSelectedImagePriv = piSelectedImage;
    int iNotUsed;
    BOOL fFound = FALSE;

    if (!piImagePriv)
        piImagePriv = &iNotUsed;

    if (!piSelectedImagePriv)
        piSelectedImagePriv = &iNotUsed;

    *piImagePriv = -1;
    *piSelectedImagePriv = -1;

    // PERF OPTIMIZATION: We will call directly to the browser window
    // which is a performance savings.  We can only do this in the
    // following situation:
    // 1. We are connected to a browser window.  (Bar only).
    // 2. The current pidl in the browser window is equal to the pidlParent.
    
    if (_pbp->IsConnected() == S_OK && _cache.pidl)
    {
        if (ILIsEqual(pidl, _cache.pidl))
        {
            IOleCommandTarget * pcmdt;

            if (SUCCEEDED(_pbs->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&pcmdt)))
            {
                VARIANT var = {0};
                HRESULT hresT = pcmdt->Exec(&CGID_ShellDocView, SHDVID_GETSYSIMAGEINDEX, 0, NULL, &var);
                if (SUCCEEDED(hresT)) 
                {
                    if (var.vt==VT_I4) 
                    {
                        *piImagePriv = var.lVal;
                        *piSelectedImagePriv = var.lVal;
                    } 
                    else 
                    {
                        ASSERT(0);
                        VariantClearLazy(&var);
                    }
                }
                pcmdt->Release();
            }
        }
    }

    if (-1 == *piImagePriv || -1 == *piSelectedImagePriv)
    {
        _GetPidlIcon(pidl, piImagePriv, piSelectedImagePriv) ;
    }
    return TRUE;
}

// NOTE: show full file system path if we're running with IE4's shell32
// (the Win95/NT4 shell and the Win2000 shell don't show the
//  full file system path in the address bar by default)

HRESULT _GetAddressBarText(LPCITEMIDLIST pidl, DWORD dwFlags, LPTSTR pszName, UINT cchName)
{
    HRESULT hr;
    *pszName = 0;

    if ((GetUIVersion() >= 5) &&
        ((dwFlags & (SHGDN_INFOLDER | SHGDN_FORPARSING)) == SHGDN_FORPARSING))
    {
        // NOTE: we are under GetUIVersion() >= 5 so we can use the "SH" versions of these API
        DWORD dwAttrib = SFGAO_FOLDER | SFGAO_LINK;
        SHGetAttributesOf(pidl, &dwAttrib);
        if (dwAttrib & SFGAO_FOLDER)
        {
            // folder objects respect the FullPathAddress flag, files (.htm) do not
            BOOL bFullTitle = FALSE;
            DWORD cbData = SIZEOF(bFullTitle);
            SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER TEXT( "\\CabinetState"), TEXT("FullPathAddress"), NULL, &bFullTitle, &cbData);
            if (!bFullTitle)
                dwFlags = SHGDN_INFOLDER;       // convert parsing name into normal name

            if ((dwFlags & SHGDN_FORPARSING) && (dwAttrib & SFGAO_LINK))
            {
                // folder shortcut special case
                IShellLinkA *psl;  // Use A version for W95.
                if (SUCCEEDED(SHGetUIObjectFromFullPIDL(pidl, NULL, IID_PPV_ARG(IShellLinkA, &psl))))
                {
                    LPITEMIDLIST pidlTarget;
                    if (SUCCEEDED(psl->GetIDList(&pidlTarget)) && pidlTarget)
                    {
                        hr = SHGetNameAndFlags(pidlTarget, dwFlags | SHGDN_FORADDRESSBAR, pszName, cchName, NULL);
                        ILFree(pidlTarget);
                    }
                }
            }
        }
    }

    if (0 == *pszName)
    {
        if (!ILIsRooted(pidl))
            dwFlags |= SHGDN_FORADDRESSBAR;
    
        hr = IEGetNameAndFlags(pidl, dwFlags, pszName, cchName, NULL);
        if (SUCCEEDED(hr))
            SHRemoveURLTurd(pszName);
    }
    return hr;
}

// This function will retrieve information about the
// pidl so the ComboBox item can be displayed.
//        pidl - the pidl to examine.
//        pszName - gets the name. (OPTIONAL)
//        cchName - size of pszName buffer. (OPTIONAL)
//        piImage - gets the icon index. (OPTIONAL)
//        dwFlags - SHGDN_ flags
//        piSelectedImage - gets selected icon index. (OPTIONAL)

BOOL CSNSList::_GetPidlUI(LPCITEMIDLIST pidl, LPTSTR pszName, int cchName, int *piImage, int *piSelectedImage, DWORD dwFlags, BOOL fIgnoreCache)
{
    ASSERT(pidl);
    if (pszName && cchName)
        *pszName = 0;

    if (!fIgnoreCache && _cache.pidl && (pidl == _cache.pidl || ILIsEqual(pidl, _cache.pidl)))
    {
        lstrcpyn(pszName, _cache.szName, cchName);
        if (piImage)
            *piImage = _cache.iImage;
        if (piSelectedImage)
            *piSelectedImage = _cache.iSelectedImage;
    }
    else 
    {
        if (pszName && cchName)
             _GetAddressBarText(pidl, dwFlags, pszName, cchName);

        if (piImage || piSelectedImage)
        {
            _GetPidlImage(pidl, piImage, piSelectedImage);
        }
    }
    return TRUE;
}

/****************************************************\
    PARAMETERS:
        pidl - the pidl to examine.

    RETURN:
        TRUE if cached pidl is changed, FALSE o/w.
  
    DESCRIPTION:
        This function will set the cache to the pidl
    that was passed in.  The cached pidl will be freeded.
    The caller still needs to free the pidl that was passed
    in because it will be cloned.
\****************************************************/
BOOL CSNSList::_SetCachedPidl(LPCITEMIDLIST pidl)
{
    BOOL fCacheChanged = FALSE;
    
    if ((_cache.pidl == NULL) || !ILIsEqual(_cache.pidl, pidl))
    {
        fCacheChanged = TRUE;

        _GetPidlUI(pidl, _cache.szName, ARRAYSIZE(_cache.szName), 
            &_cache.iImage, &_cache.iSelectedImage, SHGDN_FORPARSING, FALSE);

        if (_cache.pidl)
            ILFree(_cache.pidl);

        _cache.pidl = ILClone(pidl);
    }

    return fCacheChanged;
}


/****************************************************\
    PARAMETER:
        pnmce - PNMCOMBOBOXEXA which will come from the ComboBoxEx
                when in AddressBand mode.  The AddressBar uses
                the ANSI version of this data structure.

    DESCRIPTION:
        Handle the WM_NOTIFY/CBEN_GETDISPINFO message.
    We will call into _OnGetDispInfoW() to handle the
    call and then thunk the Text back into ANSI on
    the way out.
  
    Return:
        Standard WM_NOTIFY result.
\****************************************************/
LRESULT CSNSList::_OnGetDispInfoA(PNMCOMBOBOXEXA pnmce)
{
    LRESULT lResult = 0;
    LPWSTR  pszUniTemp;
    LPSTR pszAnsiDest;

    if (pnmce->ceItem.mask & (CBEIF_TEXT))
    {
        pszUniTemp = (LPWSTR)LocalAlloc(LPTR, pnmce->ceItem.cchTextMax * SIZEOF(WCHAR));
        if (pszUniTemp)
        {
            pszAnsiDest = pnmce->ceItem.pszText;
            ((PNMCOMBOBOXEXW)pnmce)->ceItem.pszText = pszUniTemp;

            lResult = _OnGetDispInfoW((PNMCOMBOBOXEXW)pnmce);
            SHUnicodeToAnsi(pszUniTemp, pszAnsiDest, pnmce->ceItem.cchTextMax);
            pnmce->ceItem.pszText = pszAnsiDest;
            LocalFree((VOID*)pszUniTemp);
        }
    }

    return lResult;
}


/****************************************************\
    Handle the WM_NOTIFY/CBEN_GETDISPINFO message.
  
    Input:
        pnmce - the notify message.
  
    Return:
        Standard WM_NOTIFY result.
\****************************************************/
LRESULT CSNSList::_OnGetDispInfoW(PNMCOMBOBOXEXW pnmce)
{
    if (pnmce->ceItem.lParam &&
        pnmce->ceItem.mask & (CBEIF_SELECTEDIMAGE | CBEIF_IMAGE | CBEIF_TEXT))
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST)pnmce->ceItem.lParam;

        // Normal case - ask shell to give us icon and text of a pidl.
        if (_GetPidlUI(pidl, pnmce->ceItem.pszText, pnmce->ceItem.cchTextMax,
                             &pnmce->ceItem.iImage, &pnmce->ceItem.iSelectedImage, 
                             SHGDN_INFOLDER, TRUE))
        {
            pnmce->ceItem.mask = CBEIF_DI_SETITEM | CBEIF_SELECTEDIMAGE |
                                 CBEIF_IMAGE | CBEIF_TEXT;
        }
    }

    return 0;
}


/*******************************************************************
    DESCRIPTION:
        This function will set the CShellUrl parameter to the item
    in the Drop Down list that is indexed by nIndex.
********************************************************************/
HRESULT CSNSList::SetToListIndex(int nIndex, LPVOID pvShelLUrl)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl = _GetFullIDList(nIndex);
    CShellUrl * psuURL = (CShellUrl *) pvShelLUrl;

    if (pidl)
        hr = psuURL->SetPidl(pidl);
    ASSERT(SUCCEEDED(hr));  // Should Always Succeed.

    return hr;
}


LPITEMIDLIST CSNSList::_GetDragDropPidl(LPNMCBEDRAGBEGINW pnmcbe)
{
    LPITEMIDLIST pidl;
    
    if (pnmcbe->iItemid == -1) 
    {
        pidl = ILClone(_cache.pidl);
    }
    else 
    {
        pidl = ILClone(_GetFullIDList(pnmcbe->iItemid));
    }
    return pidl;
}

HRESULT CSNSList::FileSysChangeAL(DWORD dw, LPCITEMIDLIST *ppidl)
{
    switch (dw)
    {
    case SHCNE_UPDATEIMAGE:
        _PopulateOneItem();
        break;
    
    default:

        // Don't purge the combo box if it is dropped; that confuses
        // too many people.  For example, addrlist.cpp caches the
        // *index* of the current item, and purging causes all the indexes
        // to change...

        if (SendMessage(_hwnd, CB_GETDROPPEDSTATE, 0, 0)) {
            _fPurgePending = TRUE;
        } else {
            _PurgeAndResetComboBox();
        }
        break;
    }
    return S_OK;
}

#endif
