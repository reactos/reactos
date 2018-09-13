/**************************************************************\
    FILE: addrlist.cpp

    DESCRIPTION:
        This is a class that all Address Lists can inherite
    from.  This will give them the IAddressList interface
    so they can work in the AddressBand/Bar.
\**************************************************************/

#include "priv.h"
#include "dbgmem.h"
#include "util.h"
#include "itbdrop.h"
#include "autocomp.h"
#include "addrlist.h"
#include "apithk.h"
#include "shui.h"
#include "shlguid.h"

CAddressList::CAddressList() : _cRef(1)
{
    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!_pbp);
}

CAddressList::~CAddressList()
{
    if (_pbp)
        _pbp->Release();
    if (_pbs)
        _pbs->Release();
    if (_pshuUrl)
        delete _pshuUrl;
}

ULONG CAddressList::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CAddressList::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CAddressList::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IWinEventHandler) ||
        IsEqualIID(riid, IID_IAddressList))
    {
        *ppvObj = SAFECAST(this, IAddressList*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


//================================
//  ** IWinEventHandler Interface ***

/****************************************************\
    FUNCTION: OnWinEvent

    DESCRIPTION:
        This function will give receive events from
    the parent ShellToolbar.
\****************************************************/
HRESULT CAddressList::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    LRESULT lres = 0;

    switch (uMsg)
    {
    case WM_COMMAND:
        lres = _OnCommand(wParam, lParam);
        break;

    case WM_NOTIFY:
        lres = _OnNotify((LPNMHDR)lParam);
        break;
    }

    if (plres)
        *plres = lres;

    return S_OK;
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
HRESULT CAddressList::Connect(BOOL fConnect, HWND hwnd, IBrowserService * pbs, IBandProxy * pbp, IAutoComplete * pac)
{
    HRESULT hr = S_OK;

    ASSERT(hwnd);
    _fVisible = fConnect;
    _hwnd = hwnd;

    // Copy the (IBandProxy *) parameter
    if (_pbp)
        _pbp->Release();
    _pbp = pbp;
    if (_pbp)
        _pbp->AddRef();

    if (_pbs)
        _pbs->Release();
    _pbs = pbs;
    if (_pbs)
        _pbs->AddRef();

    if (fConnect)
    {
        //
        // Initial combobox parameters.
        //
        _InitCombobox();
    }
    else
    {
        // Remove contents of the List
        _PurgeComboBox();
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
void CAddressList::_InitCombobox()
{
     SendMessage(_hwnd, CB_SETDROPPEDWIDTH, 200, 0L);
     SendMessage(_hwnd, CB_SETEXTENDEDUI, TRUE, 0L);
     SendMessage(_hwnd, CBEM_SETEXTENDEDSTYLE, CBES_EX_NOSIZELIMIT, CBES_EX_NOSIZELIMIT);
}


/****************************************************\
    FUNCTION: _PurgeComboBox

    DESCRIPTION:
        Removes all items from the combobox.
\****************************************************/
void CAddressList::_PurgeComboBox()
{
    if (_hwnd)
    {
        SendMessage(_hwnd, CB_RESETCONTENT, 0, 0L);
    }
}

/****************************************************\
    FUNCTION: _OnCommand

    DESCRIPTION:
        This function will handle WM_COMMAND messages.
\****************************************************/
LRESULT CAddressList::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT uCmd = GET_WM_COMMAND_CMD(wParam, lParam);

    switch (uCmd)
    {
        case CBN_DROPDOWN:
            {
                HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

                //
                // DOH! The user wants to see the full combo contents.
                // Better go fill it in now.
                //
                _Populate();
                SetCursor(hCursorOld);
            }
            break;
    }
    return 0;
}

/****************************************************\
    FUNCTION: NavigationComplete

    DESCRIPTION:
        Update the URL in the Top of the list.
\****************************************************/
HRESULT CAddressList::NavigationComplete(LPVOID pvCShellUrl)
{
    HRESULT hr = S_OK;
    TCHAR szDisplayName[MAX_URL_STRING];
    CShellUrl * psu = (CShellUrl *) pvCShellUrl;
    LPITEMIDLIST pidl;

    ASSERT(pvCShellUrl);
    hr = psu->GetDisplayName(szDisplayName, SIZECHARS(szDisplayName));
    ASSERT(SUCCEEDED(hr));

    //
    // Don't display the url to internal error pages.  The url that should get
    // displayed is appended after the #.
    //
    // All error urls start with res:// so do a quick check first.
    //
    BOOL fChangeURL = TRUE;
    if (TEXT('r') == szDisplayName[0] && TEXT('e') == szDisplayName[1])
    {
        if (IsErrorUrl(szDisplayName))
        {
            TCHAR* pszUrl = StrChr(szDisplayName, TEXT('#'));
            if (pszUrl)
            {
                pszUrl += 1;

                DWORD dwScheme = GetUrlScheme(pszUrl);
                fChangeURL = ((URL_SCHEME_HTTP == dwScheme) ||
                              (URL_SCHEME_HTTPS == dwScheme) ||
                              (URL_SCHEME_FTP == dwScheme) ||
                              (URL_SCHEME_GOPHER == dwScheme));

                // Don't blast in the stuff after the # into address bar
                // unless it is a 'safe' url.  If it's not safe leave the
                // addressbar alone to preserve what the user typed in.
                //
                // The issue here is that a web page could navigate to our internal
                // error page with "format c:" after the '#'.  The error page
                // suggests that the user refreshed the page which would be very bad!
                if(fChangeURL)
                {
                    StrCpyN(szDisplayName, pszUrl, ARRAYSIZE(szDisplayName));
                }
            }
        }
    }

    if (fChangeURL)
    {
        SHRemoveURLTurd(szDisplayName);

        hr = psu->GetPidl(&pidl);
        if (SUCCEEDED(hr))
        {
            COMBOBOXEXITEM cbexItem = {0};
            cbexItem.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
            cbexItem.iItem = -1;
            cbexItem.pszText = szDisplayName;
            cbexItem.cchTextMax = ARRAYSIZE(szDisplayName);

            hr = _GetPidlIcon(pidl, &(cbexItem.iImage), &(cbexItem.iSelectedImage));
            SendMessage(_hwnd, CBEM_SETITEM, (WPARAM)0, (LPARAM)(LPVOID)&cbexItem);

            ILFree(pidl);
        }
    }

    TraceMsg(TF_BAND|TF_GENERAL, "CAddressList: NavigationComplete(), URL=%s", szDisplayName);
    return hr;
}

/*******************************************************************
    FUNCTION: _MoveAddressToTopOfList

    PARAMETERS:
        iSel - index of item in combo box to move

    DESCRIPTION:
        Moves the specified selection in the combo box
    to be the first item in the combo box
********************************************************************/
BOOL CAddressList::_MoveAddressToTopOfList(int iSel)
{
    BOOL fRet = FALSE;

    ASSERT(iSel >= 0);   // must have valid index

    COMBOBOXEXITEM cbexItem = {0};
    TCHAR szAddress[MAX_URL_STRING+1];

    cbexItem.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
    cbexItem.pszText = szAddress;
    cbexItem.cchTextMax = ARRAYSIZE(szAddress);
    cbexItem.iItem = iSel;

    
    // get the specified item from combo box
    if (SendMessage(_hwnd,CBEM_GETITEM,0,(LPARAM) &cbexItem)) {

        SendMessage(_hwnd, CBEM_DELETEITEM, (WPARAM)iSel, (LPARAM)0);

        // re-insert it at index 0 to put it at the top
        cbexItem.iItem = 0;

        // sending CBEM_INSERTITEM should return the index we specified
                // (0) if successful
        fRet = (SendMessage(_hwnd, CBEM_INSERTITEM, (WPARAM)0,
            (LPARAM)(LPVOID)&cbexItem) == 0);
    }

    return fRet;
}



/*******************************************************************
    FUNCTION: _ComboBoxInsertURL

    DESCRIPTION:
        Adds the specified URL to the top of the address bar
    combo box.  Limits the number of URLs in combo box to
    nMaxComboBoxSize.
********************************************************************/
void CAddressList::_ComboBoxInsertURL(LPCTSTR pszURL, int cchStrSize, int nMaxComboBoxSize)
{
    // Since we own it and it's populated,
    // we will add it directly to the ComboBox.
    int iPrevInstance;

    int iImage, iSelectedImage ;

    COMBOBOXEXITEM cbexItem = {0};
    cbexItem.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
    cbexItem.iItem = 0;

    cbexItem.cchTextMax = cchStrSize;
    cbexItem.pszText = (LPTSTR)pszURL;

    _GetUrlUI(NULL, (LPTSTR)pszURL, &iImage, &iSelectedImage);

    cbexItem.iImage = iImage;
    cbexItem.iSelectedImage = iSelectedImage;


    iPrevInstance = (int)SendMessage(_hwnd, CB_FINDSTRINGEXACT, (WPARAM)-1,  (LPARAM)pszURL);
    if (iPrevInstance != CB_ERR) {
        _MoveAddressToTopOfList(iPrevInstance);
        return;
    }

    // insert the URL as the first item in combo box
    SendMessage(_hwnd, CBEM_INSERTITEM, (WPARAM)0, (LPARAM)(LPVOID)&cbexItem);

    // limit number of items in combo box to nMaxComboBoxSize
    if (ComboBox_GetCount(_hwnd) > nMaxComboBoxSize)
    {
        // if we're ever over the limit, we should only be over the limit
        // by exactly one item
        ASSERT(ComboBox_GetCount(_hwnd) == nMaxComboBoxSize+1);

        // if over the limit, delete the least recently used
        // (the one with the highest index)

        SendMessage(_hwnd, CBEM_DELETEITEM, (WPARAM)nMaxComboBoxSize, (LPARAM)0);

    }
}


/*******************************************************************
    FUNCTION: SetToListIndex

    DESCRIPTION:
        This function will set the CShellUrl parameter to the item
    in the Drop Down list that is indexed by nIndex.
********************************************************************/
HRESULT CAddressList::SetToListIndex(int nIndex, LPVOID pvShelLUrl)
{
    HRESULT hr = S_OK;
    TCHAR szBuffer[MAX_URL_STRING];
    CShellUrl * psuURL = (CShellUrl *) pvShelLUrl;

    GetCBListIndex(_hwnd, nIndex, szBuffer, SIZECHARS(szBuffer));
    hr = psuURL->ParseFromOutsideSource(szBuffer, SHURL_FLAGS_NOUI);
    ASSERT(SUCCEEDED(hr));  // We should not have added it to the Drop Down list if it's invalid.

    return hr;
}

HRESULT CAddressList::FileSysChangeAL(DWORD dw, LPCITEMIDLIST *ppidl)
{
    return S_OK;
}


/****************************************************\
    FUNCTION: GetCBListIndex

    DESCRIPTION:
        This function will get the text of a specified
    element in the combobox.
\****************************************************/
HRESULT GetCBListIndex(HWND hwnd, int iItem, LPTSTR szAddress, int cchAddressSize)
{
    HRESULT hr = E_FAIL;
    COMBOBOXEXITEM cbexItem = {0};

    cbexItem.mask = CBEIF_TEXT;
    cbexItem.pszText = szAddress;
    cbexItem.cchTextMax = cchAddressSize;
    cbexItem.iItem = iItem;

    if (SendMessage(hwnd, CBEM_GETITEM, 0, (LPARAM) &cbexItem))
        hr = S_OK;

    return hr;
}


// Helper Function
// We need to really becareful of perf in this function.
HRESULT CAddressList::_GetUrlUI(CShellUrl *psu, LPCTSTR szUrl, int *piImage, int *piSelectedImage)
{
    CShellUrl * psuUrl;
    HRESULT hr = E_FAIL;
    int iDrive;

    if (psu)
        psuUrl = psu;
    else
    {
        psuUrl = new CShellUrl();
        if (EVAL(psuUrl))
        {
            // Set the parent for error messageboxes.  Note that this could end up disabing the taskbar.
            // If this is deemed to be a problem we can first check to see where the addressbar is hosted.
            psuUrl->SetMessageBoxParent(_hwnd);

            SetDefaultShellPath(psuUrl);
        }
    }

    //Initialize the values to 0
    *piImage = 0;
    *piSelectedImage = 0;

    //if object is not created return with default value
    if (!psuUrl)
        return E_OUTOFMEMORY;

    // See if we have a drive specified in the path
    if((iDrive = PathGetDriveNumber(szUrl)) >= 0)
    {
        // See if the drive is removable ?
        if(DriveType(iDrive) == DRIVE_REMOVABLE)
            hr = S_OK;    //Drive is removable so pass the default icons
    }

    // Do we still need to get the icons?
    if (FAILED(hr))
    {
        // Yes, so try the fast way first.
        hr = _GetFastPathIcons(szUrl, piImage, piSelectedImage);
        if (FAILED(hr))
        {
            LPITEMIDLIST pidl = NULL;

            // If that failed because it the string probably uses advanced parsing, 
            // let CShellUrl do it the slower but more thurough way.
            hr = psuUrl->ParseFromOutsideSource(szUrl, SHURL_FLAGS_NOUI);
            if(SUCCEEDED(hr))
                hr = psuUrl->GetPidl(&pidl);

            if(SUCCEEDED(hr))
            {
                hr = _GetPidlIcon(pidl, piImage, piSelectedImage);
                ILFree(pidl);
            }
        }
    }

    if (psu != psuUrl)
        delete psuUrl;

    return hr;
}


// IECreateFromPath() and CShellUrl::ParseFromOutsideSource() both
// touch the disk which causes unconnected network cases to be really
// slow.  This will create icons for file system paths w/o hitting
// the disk.
HRESULT CAddressList::_GetFastPathIcons(LPCTSTR pszPath, int *piImage, int *piSelectedImage)
{
    SHFILEINFO shFileInfo = {0};

    // SHGetFileInfo() with those flags will be fast because it's won't filter out
    // garbage passed to it.  So it will think URLs are actually relative paths
    // and accept them.  We will fall back to the slow advanced parser which is still
    // fast with URLs.
    if (PathIsRelative(pszPath))
        return E_FAIL;

    HIMAGELIST himl = (HIMAGELIST) SHGetFileInfo(pszPath, FILE_ATTRIBUTE_DIRECTORY, &shFileInfo, sizeof(shFileInfo), (SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES));
    if (!himl || !shFileInfo.iIcon)
        return E_FAIL;

    *piImage = shFileInfo.iIcon;
    *piSelectedImage = shFileInfo.iIcon;
    // I don't need to free himl.

    return S_OK;
}


HRESULT CAddressList::_GetPidlIcon(LPCITEMIDLIST pidl, int *piImage, int *piSelectedImage)
{
    IShellFolder *psfParent;
    LPCITEMIDLIST pidlChild;
    HRESULT hr = IEBindToParentFolder(pidl, &psfParent, &pidlChild);
    if (SUCCEEDED(hr))
    {
        *piImage = IEMapPIDLToSystemImageListIndex(psfParent, pidlChild, piSelectedImage);
        psfParent->Release();
    }
    return hr;
}

LPITEMIDLIST CAddressList::_GetDragDropPidl(LPNMCBEDRAGBEGINW pnmcbe)
{
    LPITEMIDLIST pidl = NULL;
    CShellUrl *psuUrl = new CShellUrl();
    if (psuUrl)
    {
        // Set the parent for error messageboxes.  Note that this could end up disabing the taskbar.
        // If this is deemed to be a problem we can first check to see where the addressbar is hosted.
        psuUrl->SetMessageBoxParent(_hwnd);

        HRESULT hr = SetDefaultShellPath(psuUrl);
        if (SUCCEEDED(hr))
        {
            hr = psuUrl->ParseFromOutsideSource(pnmcbe->szText, SHURL_FLAGS_NOUI, NULL);
            if (SUCCEEDED(hr))
            {
                hr = psuUrl->GetPidl(&pidl);
            }
        }

        delete psuUrl;
    }
    return pidl;
}

LRESULT CAddressList::_OnDragBeginA(LPNMCBEDRAGBEGINA pnmcbe)
{
    NMCBEDRAGBEGINW  nmcbew;

    nmcbew.hdr = pnmcbe->hdr;
    nmcbew.iItemid = pnmcbe->iItemid;
    SHAnsiToUnicode(pnmcbe->szText, nmcbew.szText, SIZECHARS(nmcbew.szText));

    return _OnDragBeginW(&nmcbew);
}

#ifdef UNIX
extern "C" LPITEMIDLIST UnixUrlToPidl(UINT uiCP, LPCTSTR pszUrl, LPCWSTR pszLocation);
#endif

LRESULT CAddressList::_OnDragBeginW(LPNMCBEDRAGBEGINW pnmcbe)
{
    LPITEMIDLIST pidl = _GetDragDropPidl(pnmcbe);
#ifdef UNIX
    // for UNIX we fake a URL pidl so we create a .url instead of a .lnk
    if (!IsURLChild(pidl, TRUE))
    {
        LPITEMIDLIST pidl1;
        TCHAR szPath[MAX_PATH];
        StrCpyN(szPath, TEXT("file://"), 8);
        SHGetPathFromIDList(pidl, &szPath[7]);
        pidl1 = UnixUrlToPidl(CP_ACP, szPath, NULL);
        if (pidl1)
    {
            ILFree(pidl);
            pidl = pidl1;
        }
    }
#endif
    if (pidl) 
    {
        IOleCommandTarget *pcmdt = NULL;

        IUnknown *punk;
        if (SUCCEEDED(_pbp->GetBrowserWindow(&punk)))
        {
            punk->QueryInterface(IID_IOleCommandTarget, (void **)&pcmdt);
            punk->Release(); 
        }

        DoDragDropWithInternetShortcut(pcmdt, pidl, _hwnd);

        if (pcmdt)
            pcmdt->Release();

        ILFree(pidl);
    }

    return 0;
}

// handle WM_NOTIFY messages.
LRESULT CAddressList::_OnNotify(LPNMHDR pnm)
{
    LRESULT lReturn = 0;

    switch (pnm->code)
    {
    case NM_SETCURSOR:
        if (!SendMessage(_hwnd, CBEM_GETEXTENDEDSTYLE, 0, 0) & CBES_EX_NOEDITIMAGE)
        {
            RECT rc;
            POINT pt;
            int cx, cy;
            GetCursorPos(&pt);
            GetClientRect(_hwnd, &rc);
            MapWindowRect(_hwnd, HWND_DESKTOP, &rc);
            ImageList_GetIconSize((HIMAGELIST)SendMessage(_hwnd, CBEM_GETIMAGELIST, 0, 0), &cx, &cy);

            rc.right = rc.left + cx + GetSystemMetrics(SM_CXEDGE);
            if (PtInRect(&rc, pt)) 
            {
                // this means there's an image, which means we can drag
                SetCursor(LoadHandCursor(0));
                return 1;
            }
        }
        break;

        case CBEN_DRAGBEGINA:
            lReturn = _OnDragBeginA((LPNMCBEDRAGBEGINA)pnm);
            break;

        case CBEN_DRAGBEGINW:
            lReturn = _OnDragBeginW((LPNMCBEDRAGBEGINW)pnm);
            break;
    }

    return lReturn;
}
