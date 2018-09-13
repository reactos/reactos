/*****************************************************************************\
    FILE: ftpdhlp.cpp
    
    DESCRIPTION:
        Replace strings in a dialog template with attributes about an FTP
    item (ftp server, ftp dir, or ftp file).
\*****************************************************************************/

#include "priv.h"
#include "ftpurl.h"
#include "ftpdhlp.h"

#define SZ_WSPRINTFSTR_S            TEXT("%s")
#define SZ_WSPRINTFSTR_U            TEXT("%u")



class CSizeHolder
{
public:
    BOOL IsAllFolders(void) {return m_fAllFolders;};
    void FoundNonFolder(void) {m_fAllFolders = FALSE;};

    HRESULT GetError(void) {return m_hr;};
    void SetError(HRESULT hr) {m_hr = hr;};

    void AddSize(ULONGLONG ullSizeToAdd) { m_ullTotalSize += ullSizeToAdd;};
    ULONGLONG GetTotalSize(void) {return m_ullTotalSize;};

    CSizeHolder() {m_ullTotalSize = 0; m_fAllFolders = TRUE; m_hr = S_OK;};
    ~CSizeHolder() {};

private:
    BOOL    m_fAllFolders;
    HRESULT m_hr;
    ULONGLONG   m_ullTotalSize;
};



HRESULT CFtpDialogTemplate::_ReinsertDlgText(HWND hwnd, LPCVOID pv, LPCTSTR ptszFormat)
{
    TCHAR szDlgTemplate[256];
    TCHAR szFinalString[1024];            // wnsprintf maxes at 1024
    
    GetWindowText(hwnd, szDlgTemplate, ARRAYSIZE(szDlgTemplate));
    wnsprintf(szFinalString, ARRAYSIZE(szFinalString), szDlgTemplate, pv);
    
    // Are they the same?
    if (!StrCmp(szDlgTemplate, szFinalString))
        wnsprintf(szFinalString, ARRAYSIZE(szFinalString), ptszFormat, pv); // Yes
    
    SetWindowText(hwnd, szFinalString);
    return S_OK;
}


/*****************************************************************************\
    FUNCTION: _ReplaceIcon
    
    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpDialogTemplate::_ReplaceIcon(HWND hwnd, HICON hicon)
{
    if (hicon)
    {
        hicon = (HICON)SendMessage(hwnd, STM_SETICON, (WPARAM)hicon, 0L);
        if (hicon)
            DestroyIcon(hicon);
    }
    return S_OK;
}

/*****************************************************************************\
    FUNCTION: _InitIcon
    
    DESCRIPTION:
        _HACKHACK_  We go straight to CFtpIcon to get the pxi
    instead of going through CFtpFolder.  Same effect, but
    saves some memory allocations.  What's more important,
    we don't necessarily have a psf to play with, so we really
    have no choice.

    Yes, it's gross.
\*****************************************************************************/
HRESULT CFtpDialogTemplate::_InitIcon(HWND hwnd, CFtpFolder * pff, CFtpPidlList * pflHfpl)
{
    IExtractIcon * pxi;
    HRESULT hr;
    
    if (pflHfpl && pflHfpl->GetCount() == 1)
    {
        SHFILEINFO sfi;
        hr = FtpPidl_GetFileInfo(pflHfpl->GetPidl(0), &sfi, SHGFI_ICON | SHGFI_LARGEICON);
        if (SUCCEEDED(hr))
            hr = _ReplaceIcon(hwnd, sfi.hIcon);
    }
    else
    {
        hr = CFtpIcon_Create(pff, pflHfpl, IID_IExtractIcon, (LPVOID *)&pxi);
        if (EVAL(SUCCEEDED(hr)))
        {
            TCHAR szPath[MAX_PATH];
            int i;
            UINT ui;
            
            hr = pxi->GetIconLocation(0, szPath, ARRAYSIZE(szPath), &i, &ui);
            if (EVAL(SUCCEEDED(hr)))
            {
                CHAR szPathAnsi[MAX_PATH];
                
                SHTCharToAnsi(szPath, szPathAnsi, ARRAYSIZE(szPathAnsi));
                hr = _ReplaceIcon(hwnd, ExtractIconA(g_hinst, szPathAnsi, i));
            }
            
            ASSERT(pxi);
            pxi->Release();
        }
    }
    
    return hr;
}


void GetItemName(CFtpFolder * pff, CFtpPidlList * pflHfpl, LPWSTR pwzName, DWORD cchSize)
{
    // Are multiple items selected?
    if (1 < pflHfpl->GetCount())
        LoadString(HINST_THISDLL, IDS_SEVERAL_SELECTED, pwzName, cchSize);
    else
    {
        LPCITEMIDLIST pidl;
    
        if (0 == pflHfpl->GetCount())
            pidl = FtpID_GetLastIDReferense(pff->GetPrivatePidlReference());
        else
            pidl = FtpID_GetLastIDReferense(pflHfpl->GetPidl(0));

        if (EVAL(pidl))
            FtpPidl_GetDisplayName(pidl, pwzName, cchSize);
    }
}


BOOL CanEditName(CFtpFolder * pff, CFtpPidlList * pflHfpl)
{
    int nNumItems = pflHfpl->GetCount();
    BOOL fCanRename = TRUE;

    // we can edit except for multiply selected items
    if (2 <= nNumItems)
        fCanRename = FALSE;
    else
    {
        // If they chose the background properties for a server,
        // we won't let the change the server name.
        if (0 == nNumItems)
        {
            LPCITEMIDLIST pidlFolder = pff->GetPrivatePidlReference();

            if (pidlFolder && (ILIsEmpty(pidlFolder) || (ILIsEmpty(_ILNext(pidlFolder)))))
            {
                fCanRename = FALSE;
            }
        }
        else if (1 == nNumItems)
        {
            // Now I'm worried that pflHfpl->GetPidl(0) is a PIDL pointing to
            // an FTP Server.
            LPCITEMIDLIST pidl = pflHfpl->GetPidl(0);

            if (pidl && !ILIsEmpty(pidl) &&
                FtpID_IsServerItemID(pidl) && ILIsEmpty(_ILNext(pidl)))
            {
                fCanRename = FALSE;
            }
        }
    }

    return fCanRename;
}


/*****************************************************************************\
    FUNCTION: _InitName
    
    DESCRIPTION:
        Get the name of the object in the pflHfpl.  If there is more than one
    thing, use ellipses.
\*****************************************************************************/
HRESULT CFtpDialogTemplate::_InitName(HWND hwnd, CFtpFolder * pff, CFtpPidlList * pflHfpl)
{
    HRESULT hr = S_OK;
    WCHAR wzName[MAX_PATH];

    GetItemName(pff, pflHfpl, wzName, ARRAYSIZE(wzName));
    hr = _ReinsertDlgText(hwnd, wzName, SZ_WSPRINTFSTR_S);
    // We only use the static filename when more than one item is selected
    // because that is the case that we can't do a rename.  Are there
    // multiple items selected?
    if (m_fEditable && CanEditName(pff, pflHfpl))
    {
        // Hide because we will use IDC_FILENAME_EDITABLE instead.
        ShowEnableWindow(hwnd, FALSE);
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _InitNameEditable
    
    DESCRIPTION:
        Get the name of the object in the pflHfpl.  If there is more than one
    thing, use ellipses.
\*****************************************************************************/
HRESULT CFtpDialogTemplate::_InitNameEditable(HWND hwnd, CFtpFolder * pff, CFtpPidlList * pflHfpl)
{
    HRESULT hr = S_OK;
    TCHAR szName[MAX_PATH];

    GetItemName(pff, pflHfpl, szName, ARRAYSIZE(szName));
    hr = _ReinsertDlgText(hwnd, szName, SZ_WSPRINTFSTR_S);

    // We only use the static filename when more than one item is selected
    // because that is the case that we can't do a rename.  Are there
    // multiple items selected?
    if (!m_fEditable || !CanEditName(pff, pflHfpl))
    {
        // Hide because we will use IDC_FILENAME_EDITABLE instead.
        ShowEnableWindow(hwnd, FALSE);
    }

    return hr;
}


void GetNameFromPidlList(CFtpFolder * pff, CFtpPidlList * pflHfpl, LPWSTR pwzName, DWORD cchSize)
{
    LPCITEMIDLIST pidl;
    
    if (0 == pflHfpl->GetCount())
        pidl = FtpID_GetLastIDReferense(pff->GetPrivatePidlReference());
    else
        pidl = FtpID_GetLastIDReferense(pflHfpl->GetPidl(0));

    if (EVAL(pidl))
        StrCpyNW(pwzName, FtpPidl_GetLastItemDisplayName(pidl), cchSize);
}


/*****************************************************************************\
    FUNCTION: _InitType
    
    DESCRIPTION:
        Get the type of the pidls identified by pflHfpl.
\*****************************************************************************/
HRESULT CFtpDialogTemplate::_InitType(HWND hwnd, CFtpFolder * pff, CFtpPidlList * pflHfpl)
{
    TCHAR szType[MAX_URL_STRING];
    
    szType[0] = 0;
    switch (pflHfpl->GetCount())
    {
    case 0:
        {
            // Find out if it's a folder or an ftp server root.
            LPCITEMIDLIST pidl = FtpID_GetLastIDReferense(pff->GetPrivatePidlReference());
            if (EVAL(pidl))
                LoadString(HINST_THISDLL, (FtpID_IsServerItemID(pidl) ? IDS_ITEMTYPE_SERVER : IDS_ITEMTYPE_FOLDER), szType, ARRAYSIZE(szType));
        }
        break;
        
    case 1:
        // Just one item is selected, so get it's type.
        FtpPidl_GetFileType(pflHfpl->GetPidl(0), szType, ARRAYSIZE(szType));
        break;
        
    default:
        // Display "Several Selected" because they can span 1 type.
        LoadString(HINST_THISDLL, IDS_SEVERAL_SELECTED, szType, ARRAYSIZE(szType));
        break;
    }
    
    return _ReinsertDlgText(hwnd, szType, SZ_WSPRINTFSTR_S);
}


/*****************************************************************************\
    FUNCTION: _InitLocation
    
    DESCRIPTION:
        Get the name of the folder identified by pidl.
\*****************************************************************************/
HRESULT CFtpDialogTemplate::_InitLocation(HWND hwnd, CFtpFolder * pff, CFtpPidlList * pidlList)
{
    HRESULT hr = E_FAIL;
    TCHAR szUrl[MAX_PATH];
    LPITEMIDLIST pidl = GetPidlFromFtpFolderAndPidlList(pff, pidlList);

    ASSERT(pidlList && pff);
    if (EVAL(pidl))
    {
        // If more than one items are selected, then we only want to
        // show the common location.
        if (1 < pidlList->GetCount())
            ILRemoveLastID(pidl);
        hr = UrlCreateFromPidl(pidl, SHGDN_FORADDRESSBAR, szUrl, ARRAYSIZE(szUrl), 0, TRUE);
        if (SUCCEEDED(hr))
        {
            hr = _ReinsertDlgText(hwnd, szUrl, SZ_WSPRINTFSTR_S);
        }
        ILFree(pidl);
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _InitSizeTally
    
    DESCRIPTION:
        Total up the size of each file referred to in the pidl.
\*****************************************************************************/
int CFtpDialogTemplate::_InitSizeTally(LPVOID pvPidl, LPVOID pvSizeHolder)
{
    BOOL fSuccess = TRUE;
    LPCITEMIDLIST pidl = (LPCITEMIDLIST) pvPidl;
    CSizeHolder * pSizeHolder = (CSizeHolder *) pvSizeHolder;

    // Did we get a valid size and is pSizeHolder still valid?
    if (SUCCEEDED(pSizeHolder->GetError()))
    {
        // Yes, so keep accumulating if it's a file.
        if (!FtpID_IsServerItemID(pidl) && !FtpItemID_IsDirectory(pidl, FALSE))
        {
            ULARGE_INTEGER uliPidlFileSize;
            uliPidlFileSize.QuadPart = FtpItemID_GetFileSize(pidl);

            pSizeHolder->AddSize(uliPidlFileSize.QuadPart);
            pSizeHolder->FoundNonFolder();  // Show that at least one was a file.
            if (!uliPidlFileSize.QuadPart)
                fSuccess = FALSE;
        }
    }
    else
    {
        pSizeHolder->SetError(E_FAIL);
        fSuccess = FALSE;
    }

    return fSuccess;
}

#define MAX_FILE_SIZE           64

HRESULT GetFileSizeFromULargeInteger(ULARGE_INTEGER uliSize, LPTSTR pszSizeStr, DWORD cchSize)
{
    WCHAR wzSizeStr[MAX_FILE_SIZE];
    LONGLONG llSize = (LONGLONG) uliSize.QuadPart;

    if (StrFormatByteSizeW(llSize, wzSizeStr, ARRAYSIZE(wzSizeStr)))
        SHUnicodeToTChar(wzSizeStr, pszSizeStr, cchSize);
    else
    {
        CHAR szStrStrA[MAX_FILE_SIZE];

        StrFormatByteSizeA(uliSize.LowPart, szStrStrA, ARRAYSIZE(szStrStrA));
        SHAnsiToTChar(szStrStrA, pszSizeStr, cchSize);
    }

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: _InitSize
    
    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpDialogTemplate::_InitSize(HWND hwnd, HWND hwndLabel, CFtpFolder * pff, CFtpPidlList * pflHfpl)
{
    HRESULT hr;
    TCHAR szSizeStr[MAX_FILE_SIZE];
    CSizeHolder sizeHolder;

    szSizeStr[0] = 0;
    // GetCount maybe 0 if we are doing the background folder.
    if (0 < pflHfpl->GetCount())
    {
        pflHfpl->Enum(CFtpDialogTemplate::_InitSizeTally, (LPVOID) &sizeHolder);
        if (EVAL(SUCCEEDED(sizeHolder.GetError())))
        {
            // Are there files sizes to display?
            if (!sizeHolder.IsAllFolders())
            {
                TCHAR szBytesStr[MAX_FILE_SIZE];
                TCHAR szBytesStrFormatted[MAX_FILE_SIZE];
                TCHAR szCondencedSizeStr[MAX_FILE_SIZE];
                ULARGE_INTEGER uliTotal;
                uliTotal.QuadPart = sizeHolder.GetTotalSize();

                NUMBERFMT numfmt = {0, 0, 3, TEXT(""), TEXT(","), 0};

                EVAL(SUCCEEDED(GetFileSizeFromULargeInteger(uliTotal, szCondencedSizeStr, ARRAYSIZE(szCondencedSizeStr))));
                // BUGBUG: How do we wsprintf 64 bits?
                wnsprintf(szBytesStr, ARRAYSIZE(szBytesStr), TEXT("%u"), (DWORD)uliTotal.LowPart);
                GetNumberFormat(LOCALE_USER_DEFAULT, 0, szBytesStr, &numfmt, szBytesStrFormatted, ARRAYSIZE(szBytesStrFormatted));
                wnsprintf(szSizeStr, ARRAYSIZE(szSizeStr), TEXT("%s (%s bytes)"), szCondencedSizeStr, szBytesStrFormatted);
            }
        }
    }

    if (szSizeStr[0])
    {
        hr = _ReinsertDlgText(hwnd, szSizeStr, SZ_WSPRINTFSTR_S);
    }
    else
    {
        // If more than one item was selected...
        // remove both the label and the value.
        ShowEnableWindow(hwnd, FALSE);
        if (hwndLabel)
            ShowEnableWindow(hwndLabel, FALSE);

        hr = S_OK;
    }

    return hr;
}

// WINVER 0x0500 definition
#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL     0x00400000L // Right to left mirroring
#endif


/*****************************************************************************\
    FUNCTION: _InitTime
    
    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpDialogTemplate::_InitTime(HWND hwnd, HWND hwndLabel, CFtpFolder * pff, CFtpPidlList * pflHfpl)
{
    TCHAR szDateTime[MAX_PATH];
    HRESULT hr = E_FAIL;
    DWORD dwFlags = FDTF_SHORTTIME | FDTF_LONGDATE;
    LCID locale = GetUserDefaultLCID();

    if ((PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC)
        || (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_HEBREW))
        {
            DWORD dwExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

            if ((BOOLIFY(dwExStyle & WS_EX_RTLREADING)) != (BOOLIFY(dwExStyle & WS_EX_LAYOUTRTL)))
                dwFlags |= FDTF_RTLDATE;
            else
                dwFlags |= FDTF_LTRDATE;
         }      
    
    switch (pflHfpl->GetCount())
    {
    // one item was selected so get the time for that item.
    case 1:
        if (!FtpID_IsServerItemID(pflHfpl->GetPidl(0)))
        {
            FILETIME ftLastModified = FtpPidl_GetFTPFileTime(pflHfpl->GetPidl(0));
            Misc_StringFromFileTime(szDateTime, ARRAYSIZE(szDateTime), &ftLastModified, dwFlags);
            hr = S_OK;
        }
        break;

    // zero items selected means get the properties for the background folder
    case 0:
    {
        LPCITEMIDLIST pidl = FtpID_GetLastIDReferense(pff->GetPrivatePidlReference());
    
        // The user will get 'N/A' for the 'Server' folder. (i.e. ftp://ohserv/)
        if (EVAL(pidl) && !FtpID_IsServerItemID(pidl))
        {
            FILETIME ftLastModified = FtpPidl_GetFTPFileTime(pidl);
            Misc_StringFromFileTime(szDateTime, ARRAYSIZE(szDateTime), &ftLastModified, dwFlags);
            hr = S_OK;
        }
        // Don't free pidl because we have a pointer to someone else's copy.
    }
    }

    if (SUCCEEDED(hr))
    {
        hr = _ReinsertDlgText(hwnd, szDateTime, SZ_WSPRINTFSTR_S);
    }
    else
    {
        // If more than one item was selected...
        // remove both the label and the value.
        ShowEnableWindow(hwnd, FALSE);
        if (hwndLabel)
            ShowEnableWindow(hwndLabel, FALSE);

        hr = S_OK;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _InitCount
    
    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpDialogTemplate::_InitCount(HWND hwnd, CFtpFolder * pff, CFtpPidlList * pflHfpl)
{
    return _ReinsertDlgText(hwnd, (LPVOID)pflHfpl->GetCount(), SZ_WSPRINTFSTR_U);
}


/*****************************************************************************\
    FUNCTION: InitDialog
    
    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpDialogTemplate::InitDialog(HWND hDlg, BOOL fEditable, UINT id, CFtpFolder * pff, CFtpPidlList * pPidlList)
{
    HRESULT hr = S_OK;
    int nDlgTemlItem;

    m_fEditable = fEditable;
    for (nDlgTemlItem = 0; nDlgTemlItem < DLGTEML_MAX; nDlgTemlItem++)
    {
        HRESULT hrTemp = S_OK;

        HWND hwnd = GetDlgItem(hDlg, id + nDlgTemlItem);
        HWND hwndLabel = GetDlgItem(hDlg, id + nDlgTemlItem + DLGTEML_LABEL);
        if (hwnd)
        {
            switch (nDlgTemlItem)
            {
            case DLGTEML_FILENAME:          hrTemp = _InitName(hwnd, pff, pPidlList); break;
            case DLGTEML_FILENAMEEDITABLE:  hrTemp = _InitNameEditable(hwnd, pff, pPidlList); break;
            case DLGTEML_FILEICON:          hrTemp = _InitIcon(hwnd, pff, pPidlList); break;
            case DLGTEML_FILESIZE:          hrTemp = _InitSize(hwnd, hwndLabel, pff, pPidlList); break;
            case DLGTEML_FILETIME:          hrTemp = _InitTime(hwnd, hwndLabel, pff, pPidlList); break;
            case DLGTEML_FILETYPE:          hrTemp = _InitType(hwnd, pff, pPidlList); break;
            case DLGTEML_LOCATION:          hrTemp = _InitLocation(hwnd, pff, pPidlList); break;
            case DLGTEML_COUNT:             hrTemp = _InitCount(hwnd, pff, pPidlList); break;
            default:
                ASSERT(0);  // What are you thinking?
                break;
            }
        }

        if (EVAL(SUCCEEDED(hr)))
            hr = hrTemp;        // Propogate out the worst error.
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: OnClose
    
    DESCRIPTION:
\*****************************************************************************/
BOOL CFtpDialogTemplate::OnClose(HWND hdlg, HWND hwndBrowser, CFtpFolder * pff, CFtpPidlList * pPidlList)
{
    BOOL fCanClose = TRUE;

    // If the IDC_FILENAME_EDITABLE field is showing, then the user may have done
    // a rename.  Check if that happened and if so, do it now.
    if (IsWindowVisible(GetDlgItem(hdlg, IDC_FILENAME_EDITABLE)))
    {
        WCHAR wzOldName[MAX_PATH];
        WCHAR wzNewName[MAX_PATH];

        GetNameFromPidlList(pff, pPidlList, wzOldName, ARRAYSIZE(wzOldName));
        EVAL(GetWindowTextW(GetDlgItem(hdlg, IDC_FILENAME_EDITABLE), wzNewName, ARRAYSIZE(wzNewName)));

        // Was the name changed?
        if (StrCmpW(wzOldName, wzNewName))
        {
            // Yes, so change it.
            IShellFolder * psfParent = NULL;
            CFtpFolder * pffParent = pff;
            LPCITEMIDLIST pidlItem;

            if (0 == pPidlList->GetCount())
            {
                // We use pidlTarget mainly because we want to assure that the
                // ChangeNotifies are fired with pidlTarget.
                LPITEMIDLIST pidlParent = pff->GetPublicTargetPidlClone();

                if (pidlParent)
                {
                    ILRemoveLastID(pidlParent);
                    pidlItem = FtpID_GetLastIDReferense(pff->GetPrivatePidlReference());
                    IEBindToObject(pidlParent, &psfParent); 
                    ILFree(pidlParent);
                }
            }
            else
            {
                pidlItem = FtpID_GetLastIDReferense(pPidlList->GetPidl(0));
                EVAL(SUCCEEDED(pff->QueryInterface(IID_IShellFolder, (void **) &psfParent)));
            }

            if (EVAL(psfParent))
            {
                if (EVAL(pidlItem))
                    fCanClose = ((S_OK == psfParent->SetNameOf(hwndBrowser, pidlItem, wzNewName, NULL, NULL)) ? TRUE : FALSE);

                psfParent->Release();
            }
        }
    }

    return fCanClose;
}


BOOL CFtpDialogTemplate::HasNameChanged(HWND hdlg, CFtpFolder * pff, CFtpPidlList * pPidlList)
{
    BOOL fNameChanged = FALSE;

    // If the IDC_FILENAME_EDITABLE field is showing, then the user may have done
    // a rename.  Check if that happened and if so, do it now.
    if (IsWindowVisible(GetDlgItem(hdlg, IDC_FILENAME_EDITABLE)))
    {
        TCHAR szOldName[MAX_PATH];
        TCHAR szNewName[MAX_PATH];

        GetNameFromPidlList(pff, pPidlList, szOldName, ARRAYSIZE(szOldName));
        EVAL(GetWindowText(GetDlgItem(hdlg, IDC_FILENAME_EDITABLE), szNewName, ARRAYSIZE(szNewName)));

        // Was the name changed?
        if (StrCmp(szOldName, szNewName))
        {
            // Yes, so change it.
            fNameChanged = TRUE;
        }
    }

    return fNameChanged;
}


HRESULT CFtpDialogTemplate::InitDialogWithFindData(HWND hDlg, UINT id, CFtpFolder * pff, const FTP_FIND_DATA * pwfd, LPCWIRESTR pwWirePath, LPCWSTR pwzDisplayPath)
{
    FTP_FIND_DATA wfd = *pwfd;
    LPITEMIDLIST pidl;
    HRESULT hr;
    
    ASSERT(pwfd);

    StrCpyNA(wfd.cFileName, pwWirePath, ARRAYSIZE(wfd.cFileName));
    hr = FtpItemID_CreateReal(&wfd, pwzDisplayPath, &pidl);
    if (EVAL(SUCCEEDED(hr)))
    {
        CFtpPidlList * pfpl = NULL;
        
        hr = CFtpPidlList_Create(1, (LPCITEMIDLIST *) &pidl, &pfpl);
        if (EVAL(SUCCEEDED(hr)))
        {
            hr = InitDialog(hDlg, FALSE, id, pff, pfpl);
            pfpl->Release();
        }

        ILFree(pidl);
    }
    
    return hr;
}
