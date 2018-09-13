/*****************************************************************************\
    FILE: view.cpp

    DESCRIPTION:
        This is our ShellView which implements FTP specific behavior.  We get
    the default DefView implementation and then use IShellFolderViewCB to 
    override behavior specific to us.
\*****************************************************************************/

#include "priv.h"
#include "view.h"
#include "ftpobj.h"
#include "statusbr.h"
#include "dialogs.h"
#include <inetcpl.h>
#include <htmlhelp.h>
#include "newmenu.h"


extern ULONG g_cRef_CFtpView;

// {FBDB45F0-DBF8-11d2-BB9B-006097DF5BD4}   Private to msieftp.dll, NEVER EVER use outside of this DLL
const GUID IID_CFtpViewPrivThis = { 0xfbdb45f0, 0xdbf8, 0x11d2, { 0xbb, 0x9b, 0x0, 0x60, 0x97, 0xdf, 0x5b, 0xd4 } };


/*****************************************************************************
 *
 *      COLINFO, c_rgci
 *
 *      Column information for DVM_GETDETAILSOF.
 *
 *****************************************************************************/

const struct COLINFO {
    UINT cchCol;
    UINT uiFmt;
} c_rgci[] = {
    {   30, LVCFMT_LEFT },
    {   10, LVCFMT_RIGHT },
    {   20, LVCFMT_LEFT },
    {   20, LVCFMT_LEFT },
};


BOOL CFtpView::IsForegroundThread(void)
{
    return (GetCurrentThreadId() == m_nThreadID);
}


/*****************************************************************************\
    FUNCTION: _MOTDDialogProc

    DESCRIPTION:
\*****************************************************************************/
INT_PTR CALLBACK CFtpView::_MOTDDialogProc(HWND hDlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = FALSE;

    switch (wm)
    {
    case WM_INITDIALOG:
        {
            CFtpView * pThis = (CFtpView *) lParam;
            CFtpGlob * pfg = pThis->m_pff->GetSiteMotd();

            if (EVAL(pfg))
            {
                // TODO: NT #250018. Format the message and make it look pretty.
                //       so it doesn't have the FTP status numbers.  We may also
                //       want to filter only the message that comes thru with
                //       status numbers 230
                EVAL(SetWindowText(GetDlgItem(hDlg, IDC_MOTDDLG_MESSAGE), pfg->GetHGlobAsTCHAR()));
                pfg->Release();
            }

        }
        break;

    case WM_COMMAND:
        if ((IDOK == GET_WM_COMMAND_ID(wParam, lParam)) ||
            (IDCANCEL == GET_WM_COMMAND_ID(wParam, lParam)))
            EndDialog(hDlg, TRUE);
        break;
    }

    return lResult;
}


/*****************************************************************************
 *
 *      _ShowMotdPsf
 *
 *      Show the motd for a particular psf.
 *
 *****************************************************************************/
void CFtpView::_ShowMotdPsf(HWND hwndOwner)
{
    DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(IDD_MOTDDLG), hwndOwner, _MOTDDialogProc, (LPARAM)this);
}

/*****************************************************************************
 *
 *      _ShowMotd
 *
 *      When Explorer finally goes idle, this procedure will be called,
 *      and we will show the FTP site's (new) motd.
 *
 *****************************************************************************/
void CFtpView::_ShowMotd(void)
{
    m_hgtiWelcome = 0;

    if (EVAL(m_pff))
        _ShowMotdPsf(m_hwndOwner);
    else
    {
        // We got cancelled prematurely
    }
}

/*****************************************************************************
 *
 *      _OnGetDetailsOf
 *
 *      ici     - column for which information is requested
 *      pdi     -> DETAILSINFO
 *
 *      If pdi->pidl is 0, then we are asking for information about
 *      what columns to display.  If pdi->pidl is nonzero, then we
 *      are asking for particular information about the specified pidl.
 *
 *      _UNDOCUMENTED_: This callback and the DETAILSINFO structure
 *      are not documented.  Nor is the quirk about pdi->pidl as
 *      noted above.
 *
 *****************************************************************************/
#define MAX_SIZE_STR        30

HRESULT CFtpView::_OnGetDetailsOf(UINT ici, PDETAILSINFO pdi)
{
    HRESULT hr;

    if (ici < COL_MAX)
    {
        pdi->str.uType = STRRET_CSTR;
        pdi->str.cStr[0] = '\0';

        if (pdi->pidl)
        {
            switch (ici)
            {
            case COL_NAME:
                {
                    WCHAR wzDisplayName[MAX_PATH];
                    hr = FtpItemID_GetDisplayName(pdi->pidl, wzDisplayName, ARRAYSIZE(wzDisplayName));
                    if (EVAL(SUCCEEDED(hr)))
                        StringToStrRetW(wzDisplayName, &pdi->str);
                }
                break;

            case COL_SIZE:
                //  (Directories don't get a size.  Shell rules.)
                if (!FtpPidl_IsDirectory(pdi->pidl, TRUE))
                {
                    LONGLONG llSize = (LONGLONG) FtpItemID_GetFileSize(pdi->pidl);
                    WCHAR wzSizeStr[MAX_SIZE_STR];

                    if (StrFormatByteSizeW(llSize, wzSizeStr, ARRAYSIZE(wzSizeStr)))
                        SHUnicodeToAnsi(wzSizeStr, pdi->str.cStr, ARRAYSIZE(pdi->str.cStr));
                    else
                        StrFormatByteSizeA(FtpItemID_GetFileSizeLo(pdi->pidl), pdi->str.cStr, ARRAYSIZE(pdi->str.cStr));
                }
                hr = S_OK;
            break;

            case COL_TYPE:
                hr = FtpPidl_GetFileTypeStrRet(pdi->pidl, &pdi->str);
                break;

            case COL_MODIFIED:
                {
                    TCHAR szDateTime[MAX_PATH];
                    FILETIME ftLastModified = FtpPidl_GetFTPFileTime(pdi->pidl);
                    DWORD dwFlags = FDTF_SHORTDATE | FDTF_SHORTTIME;
                    switch (pdi->fmt)
                    {
                        case LVCFMT_LEFT_TO_RIGHT :
                            dwFlags |= FDTF_LTRDATE;
                        break;

                        case LVCFMT_RIGHT_TO_LEFT :
                            dwFlags |= FDTF_RTLDATE;
                        break;
                    }
                    Misc_StringFromFileTime(szDateTime, ARRAYSIZE(szDateTime), &ftLastModified, dwFlags);
                    hr = StringToStrRetW(szDateTime, &pdi->str);
                }
                break;
            }

        }
        else
        {
            WCHAR wzColumnLable[MAX_PATH];

            pdi->fmt = c_rgci[ici].uiFmt;
            pdi->cxChar = c_rgci[ici].cchCol;

            EVAL(LoadStringW(HINST_THISDLL, IDS_HEADER_NAME(ici), wzColumnLable, ARRAYSIZE(wzColumnLable)));
            hr = StringToStrRetW(wzColumnLable, &pdi->str);
        }
    }
    else
        hr = E_NOTIMPL;

    return hr;
}


/*****************************************************************************\
    FUNCTION: _OnColumnClick

    DESCRIPTION:
      _UNDOCUMENTED_: This callback and its parameters are not documented.
      _UNDOCUMENTED_: ShellFolderView_ReArrange is not documented.

    PARAMETERS:
      hwnd    - view window
      ici     - column that was clicked
\*****************************************************************************/
HRESULT CFtpView::_OnColumnClick(UINT ici)
{
    ShellFolderView_ReArrange(m_hwndOwner, ici);

    return S_OK;
}


HRESULT CFtpView::_OnAddPropertyPages(SFVM_PROPPAGE_DATA * pData)
{
    return AddFTPPropertyPages(pData->pfn, pData->lParam, &m_hinstInetCpl, m_psfv);
}


/*****************************************************************************\
    FUNCTION: _OnInitMenuPopup

    DESCRIPTION:
        We use IContextMenu::QueryContectMenu() to merge background items into
    the File menu.  This doesn't work on browser only because it's not supported
    so we would like to see if this works.

    PARAMETERS:
\*****************************************************************************/
HRESULT CFtpView::_OnInitMenuPopup(HMENU hmenu, UINT idCmdFirst, UINT nIndex)
{
    return S_OK;
}


/*****************************************************************************\
    FUNCTION: _OnMergeMenu

    DESCRIPTION:
      _UNDOCUMENTED_: This callback and its parameters are not documented.
      _UNDOCUMENTED_: Nothing about menu merging is documented.

    PARAMETERS:
      pqcm    - QueryContextMenu info
\*****************************************************************************/
HRESULT CFtpView::_OnMergeMenu(LPQCMINFO pqcm)
{
    HRESULT hr;
    HMENU hmenu = LoadMenu(HINST_THISDLL, MAKEINTRESOURCE(IDM_FTPMERGE));

    if (SHELL_VERSION_W95NT4 != GetShellVersion())
    {
        // We prefer to add "New" and "Login As" via
        // IContextMenu::QueryContextMenu() but it wasn't implemented
        // in browser only.  The IDM_FTPMERGE menu contains a second
        // copy for the browser only case so we need to remove them
        // if it's not browser only.
        EVAL(DeleteMenu(hmenu, FCIDM_MENU_FILE, MF_BYCOMMAND));
    }

    if (SHELL_VERSION_IE4 < GetShellVersion())
    {
        // Remove "Help.FTP Help" because we will have that work done
        // in "Help.Help Topics" on NT5 and after.  We don't do this for
        // earlier versions of shell32 because shell32 in NT5 is the 
        // first version to support "HtmlHelp" over WinHelp.  This is
        // needed because FTP's help is stored in IE's HTML Help files.
        EVAL(DeleteMenu(hmenu, IDC_ITEM_FTPHELP, MF_BYCOMMAND));
    }

    if (hmenu)
    {
        MergeMenuHierarchy(pqcm->hmenu, hmenu, pqcm->idCmdFirst, pqcm->idCmdLast);
        m_idMergedMenus = pqcm->idCmdFirst;
        m_nMenuItemsAdded = GetMenuItemCount(hmenu);
        DestroyMenu(hmenu);

        // Remove duplicate items. (Browser Only)
        _SHPrettyMenu(pqcm->hmenu);
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    // NT #267081, some other people (IE) will reformat the StatusBar during the
    // asynch navigation.  I take this event (MergeMenus) and reformat the
    // status bar if necessary.
    _InitStatusBar();

    return hr;
}


/*****************************************************************************\
    FUNCTION: UnMergeMenu

    DESCRIPTION:

    PARAMETERS:
\*****************************************************************************/
HRESULT UnMergeMenu(HMENU hMenu, UINT idOffset, HMENU hMenuTemplate)
{
    HRESULT hr = S_OK;
    UINT nIndex;
    UINT nEnd = GetMenuItemCount(hMenuTemplate);

    for (nIndex = 0; nIndex < nEnd; nIndex++)
    {
        UINT idToDelete = GetMenuItemID(hMenuTemplate, nIndex);

        if (-1 != idToDelete)
            DeleteMenu(hMenu, (idToDelete + idOffset), MF_BYPOSITION);
        else
        {
            // It may be a submenu, so we may need to recurse.
            MENUITEMINFO mii;

            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_SUBMENU;
            mii.cch = 0;     // just in case

            if (GetMenuItemInfo(hMenuTemplate, nIndex, TRUE, &mii) && mii.hSubMenu)
            {
                // It is a sub menu, so delete those items also.
                hr = UnMergeMenu(hMenu, idOffset, mii.hSubMenu);
            }
        }
    }

    return hr;
}


HRESULT CFtpView::_OnUnMergeMenu(HMENU hMenu)
{
    HRESULT hr = S_OK;

    // Did I merge anything?
    if (m_idMergedMenus && m_nMenuItemsAdded)
    {
        HMENU hMenuFTP = LoadMenu(HINST_THISDLL, MAKEINTRESOURCE(IDM_FTPMERGE));

        if (hMenuFTP)
        {
            hr = UnMergeMenu(hMenu, m_idMergedMenus, hMenuFTP);
            DestroyMenu(hMenuFTP);
        }

        m_idMergedMenus = 0;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _OnInvokeLoginAs

    DESCRIPTION:

    PARAMETERS:
\*****************************************************************************/
HRESULT CFtpView::_OnInvokeLoginAs(HWND hwndOwner)
{
    ASSERT(m_pff);
    return LoginAsViaFolder(hwndOwner, m_pff, m_psfv);
}


/*****************************************************************************\
    FUNCTION: _OnInvokeNewFolder

    DESCRIPTION:

    PARAMETERS:
\*****************************************************************************/
HRESULT CFtpView::_OnInvokeNewFolder(HWND hwndOwner)
{
    POINT pt = {0,0};

    return CreateNewFolder(hwndOwner, m_pff, NULL, m_psfv, FALSE, pt);
}


/*****************************************************************************\
    FUNCTION: _OnInvokeCommand

    DESCRIPTION:
    _UNDOCUMENTED_: This callback and its parameters are not documented.
    _UNDOCUMENTED_: ShellFolderView_ReArrange is not documented.

    PARAMETERS:
    idc     - Command being invoked
\*****************************************************************************/
HRESULT CFtpView::_OnInvokeCommand(UINT idc)
{
    HRESULT hr = S_OK;

    switch (idc)
    {
    case IDM_SORTBYNAME:
    case IDM_SORTBYSIZE:
    case IDM_SORTBYTYPE:
    case IDM_SORTBYDATE:
        ShellFolderView_ReArrange(m_hwndOwner, CONVERT_IDMID_TO_COLNAME(idc));
        break;

    case IDC_ITEM_ABOUTSITE:
        _ShowMotdPsf(m_hwndOwner);
        break;

    case IDC_ITEM_FTPHELP:
        _OnInvokeFtpHelp(m_hwndOwner);
        break;

    case IDC_LOGIN_AS:
        _OnInvokeLoginAs(m_hwndOwner);
        break;

    case IDC_ITEM_NEWFOLDER:
        _OnInvokeNewFolder(m_hwndOwner);
        break;

#ifdef ADD_ABOUTBOX
    case IDC_ITEM_ABOUTFTP:
        hr = DisplayAboutBox(m_hwndOwner);
        break;
#endif // ADD_ABOUTBOX

    default:
        ASSERT(0);
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _OnGetHelpText

    DESCRIPTION:
        The shell want's the Help Text but they want it in their format (Ansi
    vs. Unicode).
\*****************************************************************************/
HRESULT CFtpView::_OnGetHelpText(LPARAM lParam, WPARAM wParam)
{
    HRESULT hres = E_FAIL;
    UINT uiID = IDS_ITEM_HELP(LOWORD(wParam));
    TCHAR szHelpText[MAX_PATH];
    LPWSTR pwzHelpTextOut = (LPWSTR) lParam;    // Only one of these is correct and fUnicodeShell indicates which one.
    LPSTR pszHelpTextOut = (LPSTR) lParam;

    pwzHelpTextOut[0] = L'\0';   // Terminate string. (Ok if it's ANSI)

    szHelpText[0] = TEXT('\0');
    // This will fail for some items that the shell will provide for us.
    // These include View.ArrangeIcon.AutoArrange.
    // BUGBUG: This currently doesn't work for everything in the View.ArrangeIcon
    //         menu except AutoArrange because uiID is 30-33, 
    //         not 40-43 (IDS_HEADER_HELP(COL_NAME) - IDS_HEADER_HELP(COL_MODIFIED)).
    //         This will require changing the resource IDs but that will screw up
    //         the localizers and require changing IDS_HEADER_NAME().
    if (LoadString(HINST_THISDLL, uiID, szHelpText, ARRAYSIZE(szHelpText)))
    {
        HMODULE hMod = GetModuleHandle(TEXT("shell32.dll"));

        if (hMod)
        {
            BOOL fUnicodeShell = (NULL != GetProcAddress(hMod, "WOWShellExecute"));

            // NOTE: This sucks, but DVM_GETHELPTEXT will want a UNICODE string if we are running
            //       on NT and an Ansi string if we are running on Win95.  Let's thunk it to what
            //       they want.

            if (fUnicodeShell)
                SHTCharToUnicode(szHelpText, pwzHelpTextOut, HIWORD(wParam));
            else
                SHTCharToAnsi(szHelpText, pszHelpTextOut, HIWORD(wParam));

            hres = S_OK;
        }
    }

    return hres;
}


#define         SZ_HELPTOPIC_FILEA        "iexplore.chm > iedefault"
#define         SZ_HELPTOPIC_FTPSECTIONA  "ftp_over.htm"
#define         SZ_HELPTOPIC_FILEW         L"iexplore.chm"
#define         SZ_HELPTOPIC_FTPSECTIONW   L"ftp_over.htm"

/*****************************************************************************\
    FUNCTION: _OnInvokeFtpHelp

    DESCRIPTION:
        The wants Help specific to FTP.
\*****************************************************************************/
HRESULT CFtpView::_OnInvokeFtpHelp(HWND hwnd)
{
    HRESULT hr = E_INVALIDARG;
    uCLSSPEC ucs;
    QUERYCONTEXT qc = { 0 };

    ucs.tyspec = TYSPEC_CLSID;
    ucs.tagged_union.clsid = CLSID_IEHelp;

//    ASSERT(m_hwndOwner && m_psfv);        // Not available on browser only
    IUnknown_EnableModless((IUnknown *)m_psfv, FALSE);
    hr = FaultInIEFeature(m_hwndOwner, &ucs, &qc, FIEF_FLAG_FORCE_JITUI);
    IUnknown_EnableModless((IUnknown *)m_psfv, TRUE);

    HtmlHelpA(NULL, SZ_HELPTOPIC_FILEA, HH_HELP_FINDER, (DWORD_PTR) SZ_HELPTOPIC_FTPSECTIONA);
    return hr;
}


/*****************************************************************************\
    FUNCTION: _OnGetHelpTopic

    DESCRIPTION:
        Remove "Help.FTP Help" because we will have that work done
    in "Help.Help Topics" on NT5 and after.  We don't do this for
    earlier versions of shell32 because shell32 in NT5 is the 
    first version to support "HtmlHelp" over WinHelp.  This is
    needed because FTP's help is stored in IE's HTML Help files.
\*****************************************************************************/
HRESULT CFtpView::_OnGetHelpTopic(SFVM_HELPTOPIC_DATA * phtd)
{
    HRESULT hr = E_NOTIMPL;

    // Remove "Help.FTP Help" because we will have that work done
    // in "Help.Help Topics" on NT5 and after.  We don't do this for
    // earlier versions of shell32 because shell32 in NT5 is the 
    // first version to support "HtmlHelp" over WinHelp.  This is
    // needed because FTP's help is stored in IE's HTML Help files.
    if (SHELL_VERSION_IE4 < GetShellVersion())
    {
        StrCpyNW(phtd->wszHelpFile, SZ_HELPTOPIC_FILEW, ARRAYSIZE(phtd->wszHelpFile));
        StrCpyNW(phtd->wszHelpTopic, SZ_HELPTOPIC_FTPSECTIONW, ARRAYSIZE(phtd->wszHelpTopic));
        hr = S_OK;
    }

    return hr;
}

/*****************************************************************************\
    FUNCTION: _OnGetZone

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::_OnGetZone(DWORD * pdwZone, WPARAM wParam)
{
    HRESULT hr = E_INVALIDARG;
    DWORD dwZone = URLZONE_INTERNET;    // Default
    LPCITEMIDLIST pidl = m_pff->GetPrivatePidlReference();
    
    if (pidl)
    {
        WCHAR wzUrl[MAX_URL_STRING];

        // NT #277100: This may fail if TweakUI is installed because
        //             they abuse us.
        hr = UrlCreateFromPidlW(pidl, SHGDN_FORPARSING, wzUrl, ARRAYSIZE(wzUrl), ICU_ESCAPE | ICU_USERNAME, FALSE);
        if (SUCCEEDED(hr))
        {
            IInternetSecurityManager * pism;

            if (EVAL(SUCCEEDED(CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER, 
                                IID_IInternetSecurityManager, (void **) &pism))))
            {
                pism->MapUrlToZone(wzUrl, &dwZone, 0);
                pism->Release();
            }
        }
    }
    
    if (pdwZone)
    {
        *pdwZone = dwZone;
        hr = S_OK;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _OnGetPane

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::_OnGetPane(DWORD dwPaneID, DWORD * pdwPane)
{
    HRESULT hr = E_INVALIDARG;
    DWORD dwPane = PANE_NONE;    // Default unknown

    switch (dwPaneID)
    {
        case PANE_NAVIGATION:
            dwPane = STATUS_PANE_STATUS;
            break;
        case PANE_ZONE:
            dwPane = STATUS_PANE_ZONE;
            break;
        default:
            break;
    }

    if (pdwPane)
    {
        *pdwPane = dwPane;
        hr = S_OK;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _OnRefresh

    DESCRIPTION:
        We need to purge the cache and force our selves to hit the server again.
\*****************************************************************************/
HRESULT CFtpView::_OnRefresh(BOOL fReload)
{
    if (EVAL(m_pff) && fReload)
        m_pff->InvalidateCache();

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: _OnBackGroundEnumDone

    DESCRIPTION:
        Our enum happens on the background.  Sometimes we decide that we want
    to do a redirect during the enumeration because the UserName/Password
    didn't allow access to the server but the user provided a pair that does.
    Since we can't access the ComDlgBrowser's IShellBrowser::BrowseObject()
    on the background, we need to call it on the forground.  In order to do
    that, we need an event that happens on the forground.  Well this is that
    even baby.
\*****************************************************************************/
HRESULT CFtpView::_OnBackGroundEnumDone(void)
{
    HRESULT hr = S_OK;

    if (m_pidlRedirect)
    {
        LPITEMIDLIST pidlRedirect = NULL;

        ENTERCRITICAL;
        if (m_pidlRedirect)
        {
            pidlRedirect = m_pidlRedirect;
            m_pidlRedirect = NULL;
        }
        LEAVECRITICAL;

        if (pidlRedirect)
        {
            IShellBrowser * psb;
            hr = IUnknown_QueryService(_punkSite, SID_SCommDlgBrowser, IID_IShellBrowser, (LPVOID *) &psb);
            if (SUCCEEDED(hr))
            {
                hr = psb->BrowseObject(pidlRedirect, 0);
            
                AssertMsg(SUCCEEDED(hr), TEXT("CFtpView::_OnBackGroundEnumDone() defview needs to support QS(SID_ShellFolderViewCB) on all platforms that hit this point"));
                psb->Release();
            }

            ILFree(pidlRedirect);
        }
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _OnGetNotify

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::_OnGetNotify(LPITEMIDLIST * ppidl, LONG * lEvents)
{
    if (EVAL(lEvents))
        *lEvents = FTP_SHCNE_EVENTS;

    if (EVAL(ppidl))
    {
        // Normally I would use pidlRoot to get ChangeNotify messages but since
        // that doesn't work, it's necessary to broadcast ChangeNotify messages
        // using pidlTarget and receive them using pidlTarget. This is the later
        // case.
        if (EVAL(m_pff))
            *ppidl = (LPITEMIDLIST) m_pff->GetPublicTargetPidlReference();
        else
            *ppidl = NULL;
    }

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: _OnSize

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::_OnSize(LONG x, LONG y)
{
    RECT rcCurrent;
    HRESULT hr = S_OK;

    ASSERT(m_hwndOwner);
    GetWindowRect(m_hwndOwner, &rcCurrent);

    // Has the size really changed?
    if ((m_rcPrev.bottom != rcCurrent.bottom) ||
        (m_rcPrev.top != rcCurrent.top) ||
        (m_rcPrev.left != rcCurrent.left) ||
        (m_rcPrev.right != rcCurrent.right))
    {
        // yes, so update the StatusBar.
        if (m_psb)
            hr = m_psb->Resize(x, y);
        m_rcPrev = rcCurrent;
    }
    else
    {
        // No, so ignore it because we may stomp on some other
        // active view. (Because we get this message even after
        // another view took over the brower).

        // I don't care about resizing to zero.
        // I don't think the user will ever need it and it casues
        // bug #198695 where the addressband goes blank.  This is because
        // defview will call us thru each of the two places:
        // 1) CFtpFolder::CreateViewObject() (Old URL)
        // 2) CDefView::CreateViewWindow2()->CFtpView::_OnSize() (Old URL)
        // 3) DV_UpdateStatusBar()->CFtpView::_OnUpdateStatusBar() (New URL)
        // 4) ReleaseWindowLV()->WndSize()->CFtpView::_OnSize() (Old URL)
        // #4 makes us update the URL and replace #3 which is valid.
    }
    
    return hr;
}


/*****************************************************************************\
    FUNCTION: _OnThisIDList

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::_OnThisIDList(LPITEMIDLIST * ppidl)
{
    HRESULT hr = S_FALSE;

    if (EVAL(ppidl))
    {
        *ppidl = ILClone(m_pff->GetPublicRootPidlReference());
        hr = S_OK;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _OnUpdateStatusBar

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::_OnUpdateStatusBar(void)
{
    HRESULT hr = S_FALSE;
    LPCITEMIDLIST pidl = m_pff->GetPrivatePidlReference();

    if (EVAL(pidl))
    {
        TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
        BOOL fAnnonymousLogin = TRUE;

        hr = FtpPidl_GetUserName(pidl, szUserName, ARRAYSIZE(szUserName));
        if (SUCCEEDED(hr) && szUserName[0])
            fAnnonymousLogin = FALSE;

        if (m_psb)
        {
            // Even if the above call fails, we set the user name to clear out
            // any old invalid values.
            m_psb->SetUserName(szUserName, fAnnonymousLogin);
        }

        EVAL(SUCCEEDED(_SetStatusBarZone(m_psb, m_pff->m_pfs)));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: SetRedirectPidl

    DESCRIPTION:
        See the comments in _OnBackGroundEnumDone().
\*****************************************************************************/
HRESULT CFtpView::SetRedirectPidl(LPCITEMIDLIST pidlRedirect)
{
    ENTERCRITICAL;
    Pidl_Set(&m_pidlRedirect, pidlRedirect);
    LEAVECRITICAL;
    return S_OK;
}


/*****************************************************************************\
    FUNCTION: DummyHintCallback

    DESCRIPTION:
        Doesn't do anything; simply forces the connection to be established
    and the motd to be obtained.
\*****************************************************************************/
HRESULT CFtpView::DummyHintCallback(HWND hwnd, CFtpFolder * pff, HINTERNET hint, LPVOID pv1, LPVOID pv2)
{
    return S_OK;
}


/*****************************************************************************\
    FUNCTION: _InitStatusBar

    DESCRIPTION:
        Obtains and initializes the status bar window.
    It is not an error if the viewer does not provide a status bar.
\*****************************************************************************/
void CFtpView::_InitStatusBar(void)
{
    if (m_psb)
        m_psb->SetStatusMessage(IDS_EMPTY, 0);
}


/*****************************************************************************\
    FUNCTION: _OnWindowCreated (from shell32.IShellView)

    DESCRIPTION:
        When the window is created, we get the motd.  Very soon thereafter,
    DefView is going to ask for the IEnumIDList, which will now be
    in the cache.  (GROSS!  Screws up background enumeration!)

    Do this only if we don't already have a motd.
\*****************************************************************************/
HRESULT CFtpView::_OnWindowCreated(void)
{
    HRESULT hr = S_FALSE;

#ifdef _SOMEDAY_FIGURE_OUT_MOTD
/** Currently Turned off
    CFtpDir * pfd = m_pff->GetFtpDir();

    if (EVAL(pfd))
    {
        if (!CFtpDir_IsRoot(pfd))
        {
            CFtpSite * pfs = pfd->GetFtpSite();

            ASSERT(pfs);
            if (!pfs->QueryMotd())
            {
#ifdef HACKHACK_WHAT_THE_HELL       // Gotta clean this
                // This forcdes a connexn to see if there is a motd
                pfd->WithHint(&m_psb, hwndOwner, DummyHintCallback, NULL);
#endif
            }
            if (pfs->QueryNewMotd())
            {
                //  If we can't set the timeout, tough.  All that
                //  happens is you don't get to see the motd.  Boo hoo.
#pragma message("BUGBUG -- This is busted!")
                SetDelayedAction(ShowMotd, pfv, &pfv->m_hgtiWelcome);
            }
            hr = S_FALSE;
        }
        pfd->Release();
    }
***/
#endif /* SOMEDAY_FIGURE_OUT_MOTD */

    return hr;
}


/*****************************************************************************\
    FUNCTION: _OnDefItemCount (from shell32.IShellView)

    DESCRIPTION:
        _UNDOCUMENTED_: This callback and its parameters are not documented.

    Called to advise the browser of how many items we might have.  This
    allows preliminary UI to appear while we are busy enumerating
    the contents.
\*****************************************************************************/
HRESULT CFtpView::_OnDefItemCount(LPINT pi)
{
    *pi = 20;
    return S_OK;
}


/*****************************************************************************\
    FUNCTION: _OnDidDragDrop

    DESCRIPTION:
        Called to advise the browser that somebody did a drag/drop operation
    on objects in the folder.  If the effect was DROPEFFECT_MOVE, then
    we delete the source, if we aren't still doing the copy asynch on a
    background thread.

    RETURN VALUES:
        S_OK: We take responsibility of deleting the files which we can
              do here in the synch case, or in IAsynchOperation::EndOperation()
              in the asynch case.
        S_FALSE: We didn't do the delete but it's OK for the caller to do it.
                 so the caller needs to display UI and then delete via
                 IContextMenu->InvokeCommand(-delete-).
\*****************************************************************************/
HRESULT CFtpView::_OnDidDragDrop(DROPEFFECT de, IDataObject * pdo)
{
    HRESULT hr = S_OK;

    if (DROPEFFECT_MOVE == de)
    {
        IAsyncOperation * pao;

        hr = pdo->QueryInterface(IID_IAsyncOperation, (void **) &pao);
        if (SUCCEEDED(hr))
        {
            BOOL fInAsyncOp = TRUE;

            hr = pao->InOperation(&fInAsyncOp);
            hr = S_OK;  // Don't have caller do the delete.
            if (FALSE == fInAsyncOp)
            {
#ifdef FEATURE_CUT_MOVE
                CLSID clsid;
                BOOL fDoDelete = TRUE;
                CFtpObj * pfo = (CFtpObj *) pdo;

                // Is the destination the recycle bin?
                if (SUCCEEDED(DataObj_GetDropTarget(pdo, &clsid)) &&
                    IsEqualCLSID(clsid, CLSID_RecycleBin))
                {
                    // Yes, so we need to first inform the user that drops to the
                    // Recycle bin are perminate deletes and the user can't undo
                    // the operation.
                    if (IDYES != SHMessageBox(m_hwndOwner, NULL, IDS_RECYCLE_IS_PERM_WARNING, IDS_FTPERR_TITLE, (MB_ICONQUESTION | MB_YESNO)))
                        fDoDelete = FALSE;
                }

                // We didn't do the operation aynch so we need to DELETE the
                // files now to complete the MOVE operation (MOVE=Copy + Delete).
                if (fDoDelete)
                {
                    Misc_DeleteHfpl(m_pff, m_hwndOwner, pfo->GetHfpl());    // Will fail on permission denied.
                }

#else // FEATURE_CUT_MOVE
                hr = S_FALSE;   // Have parent do the delete.
#endif //FEATURE_CUT_MOVE
            }

            pao->Release();
        }
        else
            hr = S_OK;  // Don't have caller delete.  IAsyncOperation::EndOperation() will.
    }

    return hr;
}



//===========================
// *** IFtpWebView Interface ***
//===========================

/*****************************************************************************\
    FUNCTION: IFtpWebView::get_MessageOfTheDay

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::get_MessageOfTheDay(BSTR * pbstr)
{
    HRESULT hr = S_FALSE;

    if (EVAL(pbstr))
    {
        *pbstr = NULL;

        if (EVAL(m_pff))
        {
            TCHAR szDefault[MAX_PATH];
            LPCTSTR pszMOTD = szDefault;
            CFtpGlob * pfg = m_pff->GetSiteMotd();

            szDefault[0] = 0;
            if (pfg)
                pszMOTD = pfg->GetHGlobAsTCHAR();

            // if we were not able to get the message of the day
            // from CFtpFolder or it was empty, display "None"
            if ((pszMOTD == szDefault) || (!pszMOTD[0]))
            {
                pszMOTD = szDefault;
                LoadString(HINST_THISDLL, IDS_NO_MESSAGEOFTHEDAY, szDefault, ARRAYSIZE(szDefault));
            }

            *pbstr = TCharSysAllocString(pszMOTD);

            if (pfg)
                pfg->Release();

            hr = S_OK;
        }
    }
    else
        hr = E_INVALIDARG;

    ASSERT_POINTER_MATCHES_HRESULT(*pbstr, hr);
    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_Server

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::get_Server(BSTR * pbstr)
{
    HRESULT hr = S_FALSE;

    if (EVAL(pbstr))
    {
        *pbstr = NULL;

        if (EVAL(m_pff))
        {
            TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];

            if (SUCCEEDED(FtpPidl_GetServer(m_pff->GetPrivatePidlReference(), szServer, ARRAYSIZE(szServer))))
            {
                *pbstr = TCharSysAllocString(szServer);
                if (*pbstr)
                    hr = S_OK;
            }
        }
    }
    else
        hr = E_INVALIDARG;

//    ASSERT_POINTER_MATCHES_HRESULT(*pbstr, hr);
    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_Directory

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::get_Directory(BSTR * pbstr)
{
    HRESULT hr = S_FALSE;

    if (EVAL(pbstr))
    {
        *pbstr = NULL;

        if (EVAL(m_pff))
        {
            TCHAR szUrlPath[INTERNET_MAX_PATH_LENGTH];

            if (EVAL(SUCCEEDED(GetDisplayPathFromPidl(m_pff->GetPrivatePidlReference(), szUrlPath, ARRAYSIZE(szUrlPath), FALSE))))
            {
                *pbstr = TCharSysAllocString(szUrlPath);
                if (*pbstr)
                    hr = S_OK;
            }
        }
    }
    else
        hr = E_INVALIDARG;

    ASSERT_POINTER_MATCHES_HRESULT(*pbstr, hr);
    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_UserName

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::get_UserName(BSTR * pbstr)
{
    HRESULT hr = S_FALSE;

    if (EVAL(pbstr))
    {
        *pbstr = NULL;

        if (EVAL(m_pff))
        {
            TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];

            if (EVAL(SUCCEEDED(FtpPidl_GetUserName(m_pff->GetPrivatePidlReference(), szUserName, ARRAYSIZE(szUserName)))))
            {
                *pbstr = TCharSysAllocString((0 != szUserName[0]) ? szUserName : SZ_ANONYMOUS);
                if (*pbstr)
                    hr = S_OK;
            }
        }
    }
    else
        hr = E_INVALIDARG;

    ASSERT_POINTER_MATCHES_HRESULT(*pbstr, hr);
    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_PasswordLength

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::get_PasswordLength(long * plLength)
{
    HRESULT hr = S_FALSE;

    if (EVAL(plLength))
    {
        TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];

        *plLength = 0;
        if (SUCCEEDED(FtpPidl_GetPassword(m_pff->GetPrivatePidlReference(), szPassword, ARRAYSIZE(szPassword), FALSE)))
        {
            *plLength = lstrlen(szPassword);
            hr = S_OK;
        }
    }
    else
        hr = E_INVALIDARG;

    ASSERT_POINTER_MATCHES_HRESULT(*plLength, hr);
    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_EmailAddress

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::get_EmailAddress(BSTR * pbstr)
{
    HRESULT hr = S_OK;

    if (EVAL(pbstr))
    {
        TCHAR szEmailName[MAX_PATH];
        DWORD dwType = REG_SZ;
        DWORD cbSize = sizeof(szEmailName);

        if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_INTERNET_SETTINGS, SZ_REGKEY_EMAIL_NAME, &dwType, szEmailName, &cbSize))
            *pbstr = TCharSysAllocString(szEmailName);
        else
        {
            hr = S_FALSE;
            *pbstr = NULL;
        }
    }
    else
        hr = E_INVALIDARG;

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::put_EmailAddress

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::put_EmailAddress(BSTR bstr)
{
    HRESULT hr = S_OK;

    if (EVAL(bstr))
    {
        TCHAR szEmailName[MAX_PATH];

        SHUnicodeToTChar(bstr, szEmailName, ARRAYSIZE(szEmailName));
        if (ERROR_SUCCESS != SHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_INTERNET_SETTINGS, SZ_REGKEY_EMAIL_NAME, REG_SZ, szEmailName, sizeof(szEmailName)))
            hr = S_FALSE;
    }
    else
        hr = E_INVALIDARG;

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_CurrentLoginAnonymous

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::get_CurrentLoginAnonymous(VARIANT_BOOL * pfAnonymousLogin)
{
    HRESULT hr = S_OK;

    if (EVAL(pfAnonymousLogin))
    {
        TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];

        if (EVAL(m_pff) &&
            SUCCEEDED(FtpPidl_GetUserName(m_pff->GetPrivatePidlReference(), szUserName, ARRAYSIZE(szUserName))) &&
            szUserName[0] && (0 != StrCmpI(szUserName, TEXT("anonymous"))))
        {
            *pfAnonymousLogin = VARIANT_FALSE;
        }
        else
            *pfAnonymousLogin = VARIANT_TRUE;
    }
    else
        hr = E_INVALIDARG;

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::LoginAnonymously

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::LoginAnonymously(void)
{
    return _LoginWithPassword(NULL, NULL);
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::LoginWithPassword

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::LoginWithPassword(BSTR bUserName, BSTR bPassword)
{
    HRESULT hr = S_OK;
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
    TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];

    SHUnicodeToTChar(bUserName, szUserName, ARRAYSIZE(szUserName));
    SHUnicodeToTChar(bPassword, szPassword, ARRAYSIZE(szPassword));
    return _LoginWithPassword(szUserName, szPassword);
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::LoginWithoutPassword

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpView::LoginWithoutPassword(BSTR bUserName)
{
    HRESULT hr = S_FALSE;
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
    TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];

    SHUnicodeToTChar(bUserName, szUserName, ARRAYSIZE(szUserName));
    if (SUCCEEDED(FtpPidl_GetPassword(m_pff->GetPrivatePidlReference(), szPassword, ARRAYSIZE(szPassword), TRUE)))
        hr = _LoginWithPassword(szUserName, szPassword);

    return hr;
    
}


HRESULT CFtpView::_LoginWithPassword(LPCTSTR pszUserName, LPCTSTR pszPassword)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlUser;

    hr = PidlReplaceUserPassword(m_pff->GetPrivatePidlReference(), &pidlUser, m_pff->GetItemAllocatorDirect(), pszUserName, pszPassword);
    if (EVAL(SUCCEEDED(hr)))
    {
        LPITEMIDLIST pidlFull = m_pff->CreateFullPublicPidl(pidlUser);
        if (pidlFull)
        {
            hr = IUnknown_PidlNavigate(m_psfv, pidlFull, TRUE);
            ASSERT(SUCCEEDED(hr));
            ILFree(pidlFull);
        }

        ILFree(pidlUser);
    }

    if (FAILED(hr))
        hr = S_FALSE;   // Automation interfaces don't like failure returns.

    return hr;
}


//===========================
// *** IDispatch Interface ***
//===========================

// BUGBUG: Cane we nuke this?

STDMETHODIMP CFtpView::GetTypeInfoCount(UINT * pctinfo)
{ 
    return CImpIDispatch::GetTypeInfoCount(pctinfo); 
}

STDMETHODIMP CFtpView::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo * * pptinfo)
{ 
    return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); 
}

STDMETHODIMP CFtpView::GetIDsOfNames(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{ 
    return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); 
}

STDMETHODIMP CFtpView::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}


/*****************************************************************************
 *
 *	CFtpView_Create
 *
 *	Creates a brand new enumerator based on an ftp site.
 *
 *****************************************************************************/
HRESULT CFtpView_Create(CFtpFolder * pff, HWND hwndOwner, REFIID riid, LPVOID * ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CFtpView * pfv = new CFtpView(pff, hwndOwner);

    if (EVAL(pfv))
    {
        hr = pfv->QueryInterface(riid, ppv);
        pfv->Release();
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppv, hr);
    return hr;
}



/****************************************************\
    Constructor
\****************************************************/
CFtpView::CFtpView(CFtpFolder * pff, HWND hwndOwner) : CImpIDispatch(&LIBID_MSIEFTPLib)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_hwndOwner);
    ASSERT(!m_hwndStatusBar);
    ASSERT(!m_pff);
    ASSERT(!m_hgtiWelcome);
    
    m_nThreadID = GetCurrentThreadId();
    if (hwndOwner)
    {
        m_hwndOwner = hwndOwner;
        m_hwndStatusBar = Misc_FindStatusBar(hwndOwner);
        m_psb = CStatusBar_Create(m_hwndStatusBar);
        _InitStatusBar();
    }

    m_rcPrev.top = m_rcPrev.bottom = m_rcPrev.right = m_rcPrev.left = -1;
    IUnknown_Set(&m_pff, pff);

    LEAK_ADDREF(LEAK_CFtpView);
    g_cRef_CFtpView++;  // Needed to determine when to purge cache.
}


/****************************************************\
    Destructor
\****************************************************/
/*****************************************************************************
 *
 *      FtpView_OnRelease (from shell32.IShellView)
 *
 *      When the view is released, clean up various stuff.
 *
 *      BUGBUG -- (Note that there is a race here, because this->hwndOwner
 *      doesn't get zero'd out on the OnWindowDestroy because the shell
 *      doesn't give us a pdvsci...)
 *
 *      We release the psf before triggering the timeout, which is a
 *      signal to the trigger not to do anything.
 *
 *      _UNDOCUMENTED_: This callback and its parameters are not documented.
 *
 *****************************************************************************/
CFtpView::~CFtpView()
{
    IUnknown_Set(&m_pff, NULL);

    // BUGBUG -- should be a cancel, not a trigger
    TriggerDelayedAction(&m_hgtiWelcome);   /* Kick out the old one */

    SetRedirectPidl(NULL);
    if (m_psb)
        delete m_psb;

    if (m_hinstInetCpl)
        FreeLibrary(m_hinstInetCpl);

    DllRelease();
    LEAK_DELREF(LEAK_CFtpView);
    g_cRef_CFtpView--;  // Needed to determine when to purge cache.
}


//===========================
// *** IUnknown Interface ***
//===========================

HRESULT CFtpView::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = SAFECAST(this, IDispatch *);
    }
    else if (IsEqualIID(riid, IID_IFtpWebView))
    {
        *ppvObj = SAFECAST(this, IFtpWebView *);
    }
    else if (IsEqualIID(riid, IID_CFtpViewPrivThis))
    {
        *ppvObj = (void *)this;
    }
    else
        return CBaseFolderViewCB::QueryInterface(riid, ppvObj);

    AddRef();
    return S_OK;
}


CFtpView * GetCFtpViewFromDefViewSite(IUnknown * punkSite)
{
    CFtpView * pfv = NULL;
    IShellFolderViewCB * psfvcb = NULL;

    // This fails on Browser Only
    IUnknown_QueryService(punkSite, SID_ShellFolderViewCB, IID_IShellFolderViewCB, (LPVOID *) &psfvcb);
    if (psfvcb)
    {
        psfvcb->QueryInterface(IID_CFtpViewPrivThis, (void **) &pfv);
        psfvcb->Release();
    }

    return pfv;
}


CStatusBar * GetCStatusBarFromDefViewSite(IUnknown * punkSite)
{
    CStatusBar * psb = NULL;
    CFtpView * pfv = GetCFtpViewFromDefViewSite(punkSite);

    if (pfv)
    {
        psb = pfv->GetStatusBar();
        pfv->Release();
    }

    return psb;
}


