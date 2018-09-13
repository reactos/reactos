#include "priv.h"

// forwarders will be needed to write the named exports to browseui
#include <fsmenu.h>
#include <mstask.h>
#include "favorite.h"
#include "iehelpid.h"
#ifdef UNIX
#include "subsmgr.h"
#else
#include "webcheck.h"
#endif
#include "chanmgr.h"
#include "chanmgrp.h"
#include "resource.h"
#include <platform.h>
#include <mobsync.h>
#include <mobsyncp.h>

#include <mluisupp.h>

#ifdef UNIX
#include "unixstuff.h"
#include "shalias.h"
#endif

UINT IE_ErrorMsgBox(IShellBrowser* psb,
                    HWND hwndOwner, HRESULT hrError, LPCWSTR szError, LPCTSTR pszURLparam,
                    UINT idResource, UINT wFlags);
void ReplaceTransplacedControls (HWND hDlgMaster, HWND hDlgTemplate);

///////////////////////////////////////////////////////////////////////
// helper function for DoOrganizeFavDlgEx
// the org favs dialog returns a list of null terminated strings containing
//   all the urls to update.
void OrgFavSynchronize(HWND hwnd, VARIANT *pvarUrlsToSynch)
{
#ifndef DISABLE_SUBSCRIPTIONS

    ASSERT(pvarUrlsToSynch);
    
    //if there are no urls to update, it's an empty string so bail
    if ( (pvarUrlsToSynch->vt == VT_BSTR) && (pvarUrlsToSynch->bstrVal) &&
         *(pvarUrlsToSynch->bstrVal) )
    {
        PWSTR pwzUrls = pvarUrlsToSynch->bstrVal;

        ISubscriptionMgr *psm;

        if (SUCCEEDED(JITCoCreateInstance(CLSID_SubscriptionMgr, NULL,
                              CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr,
                              (void**)&psm, hwnd, FIEF_FLAG_FORCE_JITUI)))
        {
            //SysStringLen doesn't look at the string contents, just the cb of the alloc
            while (pwzUrls < (pvarUrlsToSynch->bstrVal + SysStringLen(pvarUrlsToSynch->bstrVal)))
            {
                psm->UpdateSubscription(pwzUrls);
                pwzUrls += lstrlenW(pwzUrls) + 1;
            }

            psm->Release();
        }
    }
#endif /* !DISABLE_SUBSCRIPTIONS */
}


/*
 * DoOrganizeFavDlgEx
 *
 * HWND hwnd             Owner window for the dialog.
 * LPWSTR pszInitDir     Dir to use as root. if null, the user's favorites dir is used.
 *
 * Returns:
 *  BOOL.  TRUE if succeeds. FALSE otherwise.
 *
 */

BOOL WINAPI DoOrganizeFavDlgEx(HWND hwnd, LPWSTR pszInitDir)
{
    // The easy answer would be to add an about:OrganizeFavorites that
    // gets registered in our selfreg.inx file.  Unfortunately, multilanguage
    // support requires us to generate the URL on the fly.

    WCHAR wszUrl[6 + MAX_PATH + 11 + 1]; // "res://MAX_PATH/orgfav.dlg"

    StrCpyNW(wszUrl, L"res://", 7);

#ifndef UNIX
    GetModuleFileNameWrapW(MLGetHinst(), wszUrl+6, MAX_PATH);
#else
    // IEUNIX : GetModuleFilename returns /vobs/...../libbrowseui.so
    // We need actual dllname here.
    StrCpyNW(wszUrl + 6, L"shdocvw.dll" , 12);
#endif

    StrCatW(wszUrl, L"/orgfav.dlg");

    IMoniker *pmk;
    if (SUCCEEDED(CreateURLMoniker(NULL, wszUrl, &pmk)))
    {
        ASSERT(pmk);
        VARIANT varUrlsToSynch, varInitialDir;
        BSTR    bstrInitDir;

        VariantInit(&varUrlsToSynch);
        VariantInit(&varInitialDir);

        if (pszInitDir)
        {
            bstrInitDir = SysAllocString(pszInitDir);
            if (bstrInitDir)
            {
                varInitialDir.vt = VT_BSTR;
                varInitialDir.bstrVal = bstrInitDir;
            }
        }
        
        ShowHTMLDialog(hwnd, pmk, &varInitialDir, L"Resizable=1", &varUrlsToSynch);
        OrgFavSynchronize(hwnd, &varUrlsToSynch);

        if (pszInitDir && bstrInitDir)
            SysFreeString(bstrInitDir);
        VariantClear(&varUrlsToSynch);
        pmk->Release();
        return TRUE;
    }
    else
        return FALSE;
}



/*
 * DoOrganizeFavDlg
 *
 * This API is exported so that it may be called by explorer and mshtml in
 * addition to being called internally by shdocvw.
 *
 * HWND   hwndOwner       Owner window for the dialog.
 * LPWSTR pszInitDir      Dir to use as root. if null, the user's favorites dir is used.
 *
 * Returns:
 *  BOOL.  TRUE if succeeds. FALSE otherwise.
 *
 */

BOOL WINAPI DoOrganizeFavDlg(HWND hwnd, LPSTR pszInitDir)
{
    BOOL fRet;
    WCHAR szInitDir[MAX_PATH];

    if (pszInitDir)
        SHAnsiToUnicode(pszInitDir, szInitDir, ARRAYSIZE(szInitDir));

    fRet = DoOrganizeFavDlgEx(hwnd, szInitDir);

    return fRet;
}

BOOL WINAPI DoOrganizeFavDlgW(HWND hwnd, LPWSTR pszInitDir)
{
    return DoOrganizeFavDlgEx(hwnd, pszInitDir);
}


#define ADDTOFAVPROP TEXT("SHDOC_ATFPROP")

typedef enum { ATF_FAVORITE,
               ATF_CHANNEL,
               ATF_CHANNEL_MODIFY,
               ATF_CHANNEL_SOFTDIST
} FAVDLGTYPE;

typedef struct _ADDTOFAV
{
    PTSTR pszInitDir;
    UINT cchInitDir;
    PTSTR pszFile;
    UINT cchFile;
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlSelected;
    LPCITEMIDLIST pidlFavorite;
    FAVDLGTYPE iDlgType;
    SUBSCRIPTIONINFO siSubsInProg;
    SUBSCRIPTIONTYPE subsType;
    BOOL bIsSoftdist;
    BOOL bStartSubscribed;
    BOOL bSubscribed;
} ADDTOFAV;

BOOL IsSubscribed(ADDTOFAV *patf);

typedef struct _BFFFavSubStruct
{
    WNDPROC lpfnOldWndProc;
    HWND hwndNew;
    HWND hwndTV;
    HWND hwndSave;
    HWND hTemplateWnd;
    ADDTOFAV * patf;
    RECT rcRestored;
} BFFFAVSUBSTRUCT;

BOOL CALLBACK NewFavDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        HWND hwnd;
        ASSERT(lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
        // cross-lang platform support
        SHSetDefaultDialogFont(hDlg, IDD_NAME);
        hwnd = GetDlgItem(hDlg, IDD_NAME);
#ifndef UNIX
        SendMessage(hwnd, EM_LIMITTEXT, MAX_PATH - 1, 0);
#else
        // IEUNIX : file/dir name on unix is limited to _MAX_FNAME.
        SendMessage(hwnd, EM_LIMITTEXT, (WPARAM)(_MAX_FNAME - 1), (LPARAM)0);
#endif
        EnableOKButtonFromID(hDlg, IDD_NAME);
        break;
    }    
    case WM_DESTROY:
        SHRemoveDefaultDialogFont(hDlg);
        return FALSE;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDD_NAME:
            {
                if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_UPDATE)
                {
                    LPTSTR lpstrName = (LPTSTR) GetWindowLongPtr(hDlg, DWLP_USER);
                    EnableOKButtonFromID(hDlg, IDD_NAME);
                    GetDlgItemText(hDlg, IDD_NAME, lpstrName, MAX_PATH);
                }
                break;
            }

        case IDOK:
        {
            TCHAR  szTmp[MAX_PATH];
            StrCpyN(szTmp, (LPTSTR)GetWindowLongPtr(hDlg, DWLP_USER), ARRAYSIZE(szTmp));
            if (PathCleanupSpec(NULL,szTmp))
            {
               HWND hwnd;

             MLShellMessageBox(
                             hDlg,
                             MAKEINTRESOURCE(IDS_FAVS_INVALIDFN),
                             MAKEINTRESOURCE(IDS_FAVS_ADDTOFAVORITES),
                             MB_OK | MB_ICONHAND);
             hwnd = GetDlgItem(hDlg, IDD_NAME);
             SetWindowText(hwnd, TEXT('\0'));
             EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
             SetFocus(hwnd);
             break;
            }
        }
        // fall through

        case IDCANCEL:
            EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

// BOGUS - these id's stolen from SHBrowseForFolder implementation
#define IDD_FOLDERLIST 0x3741
#define IDD_BROWSETITLE 0x3742
#define IDD_BROWSESTATUS 0x3743

const static DWORD aAddToFavHelpIDs[] = {  // Context Help IDs
    IDC_FAVORITE_DESC,          NO_HELP,
    IDD_BROWSETITLE,            NO_HELP,
    IDD_BROWSESTATUS,           NO_HELP,
    IDC_FAVORITE_ICON,          NO_HELP,
    IDC_NAMESTATIC,             IDH_NAMEEDIT,
    IDC_FOLDERLISTSTATIC,       IDH_BROWSELIST,
    IDC_SUBSCRIBE_FOLDERLIST_PLACEHOLDER,     IDH_BROWSELIST,
    IDC_FAVORITE_NEWFOLDER,     IDH_CREATEIN,
    IDC_SUBSCRIBE_CUSTOMIZE,    IDH_CHANNEL_SUBSCR_CUST_BUTTON,
    IDC_FAVORITE_CREATEIN,      IDH_NEWFOLDER,
    IDC_FAVORITE_NAME,          IDH_NAMEEDIT,
    IDC_MAKE_OFFLINE,           IDH_MAKE_AVAIL_OFFLINE,
    0, 0
};

const static DWORD aAddToChanHelpIDs[] = {  // Context Help IDs
    IDC_FAVORITE_DESC,          NO_HELP,
    IDD_BROWSETITLE,            NO_HELP,
    IDD_BROWSESTATUS,           NO_HELP,
    IDC_FAVORITE_ICON,          NO_HELP,
    IDC_NAMESTATIC,             IDH_NAMEEDIT,
    IDC_FOLDERLISTSTATIC,       IDH_BROWSELIST,
    IDC_SUBSCRIBE_FOLDERLIST_PLACEHOLDER,     IDH_BROWSELIST,
    IDC_FAVORITE_NEWFOLDER,     IDH_CREATEIN,
    IDC_SUBSCRIBE_CUSTOMIZE,    IDH_CHANNEL_SUBSCR_CUST_BUTTON,
    IDC_FAVORITE_CREATEIN,      IDH_NEWFOLDER,
    IDC_FAVORITE_NAME,          IDH_NAMEEDIT,
    IDC_MAKE_OFFLINE,           IDH_MAKE_AVAIL_OFFLINE,
    0, 0
};

/*
 * Makes sure the item being added to favorites doesn't already exist.  If it does,
 * puts up a message box to have the user confirm whether they want to overwrite
 * the old favorite or not.  
*/
BOOL ConfirmAddToFavorites(HWND hwndOwner, ADDTOFAV * patf)
{
    BOOL fRet = FALSE;
    BOOL fExists;
    int iPromptString = 0;

    if (patf->subsType == SUBSTYPE_CHANNEL)
    {
        //patf->pszInitDir now contains the path with a .url on the end; the channel
        //will be stored in a directory of that name without .url.  Strip it.
        TCHAR szPath[MAX_PATH];
        StrCpyN(szPath, patf->pszInitDir, ARRAYSIZE(szPath));
        PathRemoveExtension (szPath);
        fExists = PathFileExists(szPath);

        iPromptString = IDS_CHANNELS_FILEEXISTS;

    }
    else
    {
        fExists = PathFileExists(patf->pszInitDir);
        iPromptString = IDS_FAVS_FILEEXISTS;

    }

    fRet = ! fExists ||
        (MLShellMessageBox(
                         hwndOwner,
                         MAKEINTRESOURCE(iPromptString),
                         NULL,    //use owner's title
                         MB_ICONQUESTION | MB_YESNO) == IDYES);
    return fRet;
}

//
// Get the localized date and time
//

typedef HRESULT (*PFVARIANTTIMETOSYSTEMTIME)(DOUBLE, LPSYSTEMTIME);


//
// Subscribe to the current site.
//

HRESULT SubscribeToSite(HWND hwnd, LPCTSTR pszFile, LPCITEMIDLIST pidl, DWORD dwFlags,
                        SUBSCRIPTIONINFO* pSubs, SUBSCRIPTIONTYPE subsType)
{
#ifndef DISABLE_SUBSCRIPTIONS

    TCHAR szURL[MAX_URL_STRING];
    ISubscriptionMgr *pISubscriptionMgr;

    //
    // Get a displayable URL.
    //

    IEGetDisplayName(pidl, szURL, SHGDN_FORPARSING);

    //
    // Get a pointer to the subscription manager.
    //

    HRESULT hr = JITCoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER,
                          IID_ISubscriptionMgr,
                          (void**)&pISubscriptionMgr, hwnd, FIEF_FLAG_FORCE_JITUI);

    if (SUCCEEDED(hr)) 
    {
        //
        // Create a default subscription.
        //
        BSTR bstrURL = SysAllocStringT(szURL);
        if (bstrURL) 
        {
            BSTR bstrName = SysAllocStringT(pszFile);
            if (bstrName) 
            {
                hr = pISubscriptionMgr->CreateSubscription(hwnd, 
                    bstrURL, bstrName, dwFlags, subsType,  pSubs);
                SysFreeString(bstrName);
            }
            SysFreeString(bstrURL);
        }

        //
        // Clean up.
        //

        pISubscriptionMgr->Release();
    }

    return hr;
#else  /* !DISABLE_SUBSCRIPTIONS */

    return E_FAIL;

#endif /* !DISABLE_SUBSCRIPTIONS */
}


//
// Create in-memory subscription, but only optionally save it to subscription manager
//

BOOL StartSiteSubscription (HWND hwnd, ADDTOFAV* patf, BOOL bFinalize)
{
#ifndef DISABLE_SUBCRIPTIONS

    //update the changes-only flag (radio buttons here are, effectively, direct access to this flag)
    if (patf->subsType == SUBSTYPE_CHANNEL || patf->subsType == SUBSTYPE_DESKTOPCHANNEL)
    {
        //if set, leave alone; otherwise, put to full download
        if (!(patf->siSubsInProg.fChannelFlags & CHANNEL_AGENT_PRECACHE_SOME))
            patf->siSubsInProg.fChannelFlags |= CHANNEL_AGENT_PRECACHE_ALL;

        patf->siSubsInProg.fUpdateFlags |= SUBSINFO_CHANNELFLAGS | SUBSINFO_SCHEDULE;
    }

    if (S_OK != SubscribeToSite(hwnd, patf->pszFile, patf->pidlFavorite,
                                   bFinalize ? CREATESUBS_NOUI | CREATESUBS_FROMFAVORITES : CREATESUBS_NOSAVE,
                                   &patf->siSubsInProg, patf->subsType))
    {
        return FALSE;
    }

    return TRUE;

#else  /* !DISABLE_SUBSCRIPTIONS */

    return FALSE;

#endif /* !DISABLE_SUBSCRIPTIONS */
}

/*
   Combines the path and the filename of the favorite
   and puts it into patf->pszInitDir, so that it has the fully qualified pathname.
*/
#define SZ_URLEXT    TEXT(".url")
#define CCH_URLEXT   SIZECHARS(SZ_URLEXT)

BOOL QualifyFileName(ADDTOFAV *patf)
{
    TCHAR szTemp[MAX_PATH];
    BOOL fRet = FALSE;
    LPTSTR  pstr;

    // Can we safely add the extension to this?
    if (lstrlen(patf->pszFile) < (int)(patf->cchFile - CCH_URLEXT))
    {
        //Add extension .url if its not already there
        //This is to prevent strings like "com"  in "www.microsoft.com" from being interpreted as extensions

        pstr = PathFindExtension(patf->pszFile);
        if (!pstr || (pstr && StrCmpI(pstr, SZ_URLEXT)))// && StrCmpI(pstr, SZ_CDFEXT)))
            StrCatBuff(patf->pszFile, SZ_URLEXT, patf->cchFile);
            
        // Is there a folder associated with the filename?
        if (patf->pidlSelected && SHGetPathFromIDList(patf->pidlSelected, szTemp)) 
        {
            // Yes
            if (PathCombine(szTemp, szTemp, patf->pszFile))
            {
                if ((UINT)lstrlen(szTemp) < patf->cchInitDir)
                {
                    StrCpyN(patf->pszInitDir, szTemp, patf->cchInitDir);
                    fRet = TRUE;
                }
            }
        }
    }

    return fRet;
}


BOOL SubscriptionFailsChannelAuthentication (HWND hDlg, SUBSCRIPTIONINFO* psi)
{
#ifndef DISABLE_SUBSCRIPTIONS  

    if (psi->bNeedPassword && !(psi->bstrPassword && psi->bstrPassword[0]
                             && psi->bstrUserName && psi->bstrUserName[0]))
    {   //password would be required
        if (IsDlgButtonChecked (hDlg, IDC_MAKE_OFFLINE))
        {   //they're trying to subscribe...  WRONG!
            MLShellMessageBox(
                hDlg,
                MAKEINTRESOURCE(IDS_NEED_CHANNEL_PASSWORD),
                NULL,
                MB_ICONINFORMATION | MB_OK);
            return TRUE;
        }
    }

    return FALSE;

#else  /* !DISABLE_SUBSCRIPTIONS */

    return FALSE;

#endif /* !DISABLE_SUBSCRIPTIONS */
}


LRESULT CALLBACK BFFFavSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BFFFAVSUBSTRUCT * pbffFS = (BFFFAVSUBSTRUCT *)GetProp(hwnd, ADDTOFAVPROP);
    WNDPROC lpfnOldWndProc = pbffFS->lpfnOldWndProc;
    RECT rc;

    switch (uMsg) {
        case WM_COMMAND:
            // Intercept the command for the New Folder button we hacked into
            // the SHBrowseForFolder dialog.
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
            case IDC_FAVORITE_NAME:
            {
                HWND hwndedit;
                if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE) 
                    EnableOKButtonFromID(hwnd, IDC_FAVORITE_NAME);
                hwndedit = GetDlgItem(hwnd, IDC_FAVORITE_NAME);
                SendMessage(hwndedit, EM_LIMITTEXT, MAX_PATH - 1, 0);
                break;
            }    

#ifndef UNIX
// IEUNIX (OFFLINE) : No offline ability

            case IDC_MAKE_OFFLINE:
                EnableWindow(GetDlgItem(hwnd, IDC_SUBSCRIBE_CUSTOMIZE), 
                             IsDlgButtonChecked(hwnd, IDC_MAKE_OFFLINE));
                break;

            case IDC_SUBSCRIBE_CUSTOMIZE:
                //need to create -- but not store -- subscription
                if (StartSiteSubscription (hwnd, pbffFS->patf, FALSE))
                    SendMessage (hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDOK), TRUE);
                break;
#endif

            case IDC_FAVORITE_NEWFOLDER:
                TCHAR szPath[MAX_PATH];
                TCHAR szName[MAX_PATH];
                HWND hwndTV;
                TV_ITEM tv_item;

                // Bring up the Create New Folder dialog
                if ((DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(DLG_NEWFOLDER), hwnd,
                    (DLGPROC)NewFavDlgProc, (LPARAM)szName) == IDOK) &&
                    (SHGetPathFromIDList(pbffFS->patf->pidlSelected, szPath)) &&
                    ((lstrlen(szPath) + lstrlen(szName) + 1) < MAX_PATH))
                {
                    PathCombine(szPath, szPath, szName);

                    BOOL bSuccess = FALSE;

#ifdef CREATEFOLDERSINCHANNELSDIR
                    if (pbffFS->patf->subsType == SUBSTYPE_CHANNEL)
                    {
                        ASSERT(0);  //should not be possible in this release
                                    //(I removed this button in the .rc dialogs for channels)

                        //Note: to make this work in a future release, reenable this code -- it's
                        //functional.  But the folders created here show up ugly in the channel bar
                        //(just a default folder icon) and if you click on them, you get a shell
                        //Explorer window instead of a theater-mode browser window.  The reason
                        //for this second happening is that the desktop.ini file created in the new
                        //folder has no URL=.  To remedy this: AddCategory() has to be fixed so it
                        //doesn't interpret the pszURL argument as a UNC name (I was using a resouce moniker
                        //pointing into cdfview.dll for the html target), and the OC hosted by the default
                        //html pages has to learn how to be hosted from a html page without a path -- or we
                        //actually have to create an html page in the new directory, which is messy.
                        IChannelMgr* pChanMgr;
                        HRESULT hr;

                        hr = JITCoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER,
                                              IID_IChannelMgr, (void**)&pChanMgr, 
                                              hwnd, FIEF_FLAG_FORCE_JITUI);

                        if (SUCCEEDED(hr))
                        {
                            IChannelMgrPriv* pChanMgrPriv;
                            hr = pChanMgr->QueryInterface (IID_IChannelMgrPriv, (void**)&pChanMgrPriv);
                            if (SUCCEEDED(hr))
                            {
                                char szCFPath[MAX_PATH];
                                WCHAR wszFolder[MAX_PATH];
                                IChannelMgrPriv::CHANNELFOLDERLOCATION cflLocation =
                                    (pbffFS->patf->iDlgType == ATF_CHANNEL_SOFTDIST ?
                                        IChannelMgrPriv::CF_SOFTWAREUPDATE :
                                        IChannelMgrPriv::CF_CHANNEL);
                                hr = pChanMgrPriv->GetChannelFolderPath (szCFPath, ARRAYSIZE(szCFPath), cflLocation);

                                int cchCommon = PathCommonPrefix (szPath, szCFPath, NULL);
                                AnsiToUnicode (szPath + cchCommon, wszFolder, ARRAYSIZE(wszFolder));

                                CHANNELCATEGORYINFO info = {0};
                                info.cbSize = sizeof(info);
                                info.pszTitle = wszFolder;
                                bSuccess = SUCCEEDED (pChanMgr->AddCategory (&info));

                                pChanMgrPriv->Release();
                            }

                            pChanMgr->Release();
                        }
                    }
                    else
#endif
                    {
                        bSuccess = CreateDirectory(szPath, NULL);
                    }

                    if (bSuccess)
                    {
                        // This code assumes the layout of SHBrowseForFolder!

                        // directory successfully created, must notify registered shell components.
                        SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, szPath, NULL);
                        // Get the TreeView control
                        hwndTV = GetDlgItem(hwnd, IDD_FOLDERLIST);
                        if (hwndTV) {
                            HTREEITEM hti = TreeView_GetSelection(hwndTV);
                            // Take the selected item and reset it, then reexpand it so
                            // that it shows the new directory we just created.
                            tv_item.mask = TVIF_CHILDREN;
                            tv_item.hItem = hti;
                            tv_item.cChildren = -1;
                            TreeView_SetItem(hwndTV, &tv_item);
                            TreeView_Expand(hwndTV, hti, TVE_COLLAPSE | TVE_COLLAPSERESET);
                            TreeView_Expand(hwndTV, hti, TVE_EXPAND);

                            // Find the new directory we just created and select it by
                            // walking the tree from the selected item down.
                            if (hti = TreeView_GetChild(hwndTV, hti)) {
                                tv_item.mask = TVIF_TEXT;
                                tv_item.pszText = szPath;
                                tv_item.cchTextMax = MAX_PATH;
                                do {
                                    tv_item.hItem = hti;
                                    TreeView_GetItem(hwndTV, &tv_item);
                                    if (StrCmp(szName, szPath) == 0) {
                                        TreeView_Select(hwndTV, hti, TVGN_CARET);
                                        break;
                                    }
                                } while (hti = TreeView_GetNextSibling(hwndTV, hti));
                            }
                            SetFocus(hwndTV);
                        }
                    } else {
                        
                        LPVOID lpMsgBuf;

                        FormatMessage( 
                            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            GetLastError(),
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                            (LPTSTR) &lpMsgBuf,
                            0,
                            NULL 
                        );
                        MLShellMessageBox(
                                        hwnd,
                                        (LPCTSTR)lpMsgBuf,
                                        MAKEINTRESOURCE(IDS_FAVS_ADDTOFAVORITES),
                                        MB_ICONINFORMATION | MB_OK);
                        
                        // Free the buffer.
                        LocalFree( lpMsgBuf );
                        
                    }
                }
                break;

            case IDOK:
                // first, make sure they're not trying to subscribe to an authenticated
                // channel without entering a password.
                if (SubscriptionFailsChannelAuthentication (hwnd, &pbffFS->patf->siSubsInProg))
                    return FALSE;

                // Retrieve the text from the Name edit control.
                GetDlgItemText(hwnd, IDC_FAVORITE_NAME, pbffFS->patf->pszFile, pbffFS->patf->cchFile);
                { // Just  a block to declare variables
                    BOOL fTooBig = TRUE;        // assume failure
                    TCHAR  szTmp[MAX_PATH];
                                       
                    if (lstrlen(pbffFS->patf->pszFile) < MAX_PATH)
                    {
                        StrCpyN(szTmp, pbffFS->patf->pszFile, ARRAYSIZE(szTmp));

                        // PathCleanupSpec deals with MAX_PATH buffers, so we should be fine
                        if (PathCleanupSpec(NULL, szTmp))
                        {
                            MLShellMessageBox(
                                            hwnd,
                                            MAKEINTRESOURCE(IDS_FAVS_INVALIDFN),
                                            MAKEINTRESOURCE(IDS_FAVS_ADDTOFAVORITES),
                                            MB_OK | MB_ICONHAND);
                            return FALSE;
                        }
                                       
                        // Make sure the name is unique and if not, that the user has
                        // specified that it is OK to override.
                        if (QualifyFileName(pbffFS->patf))
                        {
                            if (!ConfirmAddToFavorites(hwnd, pbffFS->patf))
                                return FALSE;

                            //  BUGBUG: Bogus hack since the ATF stuff is only half done
                            //  Depending on which dlg is shown, look for the appropriate
                            //  check.
                            if (IsDlgButtonChecked (hwnd, IDC_MAKE_OFFLINE))
                            {
                               //they want to subscribe!  save subscription we already have in memory
                                //trouble is, pbffFS->patf->pszFile ends in a bogus .url
                                TCHAR* pszTemp = pbffFS->patf->pszFile;
                                TCHAR szNoExt[MAX_PATH];
                                StrCpyN(szNoExt, pbffFS->patf->pszFile, ARRAYSIZE(szNoExt));
                                pbffFS->patf->pszFile = szNoExt;
                                PathRemoveExtension (szNoExt);
                                pbffFS->patf->bSubscribed = 
                                    StartSiteSubscription (hwnd, pbffFS->patf, TRUE);
                                pbffFS->patf->pszFile = pszTemp;
                            }
                            else if (pbffFS->patf->bStartSubscribed)
                            {
                                //  If we started subscribed and they unchecked make available
                                //  offline, then delete the subscription.

                                ISubscriptionMgr* pSubsMgr;
                                if (SUCCEEDED (CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER,
                                                                IID_ISubscriptionMgr, (void**)&pSubsMgr)))
                                {
                                    //url is in patf->pidlFavorite
                                    WCHAR wszURL[MAX_URL_STRING];
                                    IEGetDisplayName(pbffFS->patf->pidlFavorite, wszURL, SHGDN_FORPARSING);

                                    pSubsMgr->DeleteSubscription(wszURL, NULL);
                                    pSubsMgr->Release();
                                }
                            }

                            // Enable and set focus to the tree view so that it is sure to
                            // be selected so that SHBrowseForFolder will return a pidl.
                            EnableWindow(pbffFS->hwndTV, TRUE);
                            SetFocus(pbffFS->hwndTV);
                            fTooBig = FALSE;
                        }
                    }

#ifdef UNIX_FEATURE_ALIAS
                    if( !fTooBig )
                    {
                        TCHAR alias[MAX_ALIAS_LENGTH];
                        TCHAR szThisURL[MAX_URL_STRING];
                        HDPA  aliasList = GetGlobalAliasList();

                        if( aliasList )
                        {
#ifdef UNICODE
                            // TODO :
#else
                            // Retrieve the text from the Alias edit control.
                            GetDlgItemText(hwnd, IDC_ALIAS_NAME, alias, MAX_ALIAS_LENGTH-1);
                            IEGetDisplayName(pbffFS->patf->pidlFavorite, 
                                szThisURL, SHGDN_FORPARSING);
                            if(AddAliasToListA( aliasList, alias, szThisURL, hwnd ))
                                SaveAliases(aliasList);
#endif
                        }
                    }
#endif /* UNIX_FEATURE_ALIAS */

                    if (fTooBig)
                    {
                        MLShellMessageBox(
                                        hwnd,
                                        MAKEINTRESOURCE(IDS_FAVS_FNTOOLONG),
                                        MAKEINTRESOURCE(IDS_FAVS_ADDTOFAVORITES),
                                        MB_OK | MB_ICONHAND);
                        return FALSE;
                    }
                }
                break;

            case IDC_FAVORITE_CREATEIN:
                // The advanced button has been clicked.  Enable/disable the tree view
                // and New button, set focus to the tree view or ok button, disable the advanced
                // button and then resize the dialog.
            {
                BOOL fExpanding = !IsWindowEnabled(GetDlgItem(hwnd, IDC_SUBSCRIBE_FOLDERLIST_PLACEHOLDER)); //random control that gets enabled when dialog expanded
                TCHAR szBuffer[100];

                EnableWindow(pbffFS->hwndTV, fExpanding);
                //don't show New Folder button for channels in the channels folder,
                // see code for case IDC_FAVORITE_NEWFOLDER for why
                if (fExpanding && pbffFS->patf->subsType == SUBSTYPE_CHANNEL)
                {
                    LPITEMIDLIST pidlFavs = NULL;
                    TCHAR tzFavsPath[MAX_PATH];
                
                    if (SUCCEEDED(SHGetSpecialFolderLocation(hwnd, CSIDL_FAVORITES, &pidlFavs)) 
                    &&  SUCCEEDED(SHGetNameAndFlags(pidlFavs, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, tzFavsPath, SIZECHARS(tzFavsPath), NULL))
                    &&  StrCmpNI(tzFavsPath, pbffFS->patf->pszInitDir, ARRAYSIZE(tzFavsPath))==0)
                    {
                        EnableWindow(pbffFS->hwndNew, TRUE);
                    }
                    if(pidlFavs)
                        ILFree(pidlFavs);
                }
                else
                    EnableWindow(pbffFS->hwndNew, fExpanding);

                GetWindowRect(hwnd, &rc);
                if (fExpanding)
                {
                    int lRet = MLLoadString(IDS_FAVS_ADVANCED_COLLAPSE, szBuffer, ARRAYSIZE(szBuffer));
                    ASSERT(lRet);
                    
                    SetFocus(pbffFS->hwndTV);

                    MoveWindow(hwnd, rc.left, rc.top,
                        pbffFS->rcRestored.right - pbffFS->rcRestored.left,
                        pbffFS->rcRestored.bottom - pbffFS->rcRestored.top, TRUE);
                }
                else
                {
                    int lRet = MLLoadString(IDS_FAVS_ADVANCED_EXPAND, szBuffer, ARRAYSIZE(szBuffer));
                    ASSERT(lRet);
                    
                    SetFocus(GetDlgItem(hwnd, IDC_FAVORITE_NAME));

                    MoveWindow(hwnd, rc.left, rc.top,
                        pbffFS->rcRestored.right - pbffFS->rcRestored.left,
                        pbffFS->rcRestored.bottom - pbffFS->rcRestored.top, TRUE);

                    // hide the bottom part of the dialog
                    int cx, cy;
                    RECT rc;
                    GetWindowRect (GetDlgItem (hwnd, IDC_SUBSCRIBE_FOLDERLIST_PLACEHOLDER), &rc);
                    cy = rc.top;
                    GetWindowRect (hwnd, &rc);
                    cx = rc.right - rc.left;
                    cy = cy /*top of ctrl*/ - rc.top; /*top of window*/
                    SetWindowPos (hwnd, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER);
                }
                SetWindowText(GetDlgItem(hwnd, IDC_FAVORITE_CREATEIN), szBuffer);

                break;
            }
            }
            break;

        case WM_DESTROY:
        {
            DWORD dwValue = IsWindowEnabled(GetDlgItem(hwnd, IDC_FAVORITE_NEWFOLDER)); //random control that gets enabled when dialog expanded

            SHRegSetUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"), TEXT("AddToFavoritesExpanded"),
                REG_DWORD, &dwValue, 4, SHREGSET_HKCU | SHREGSET_FORCE_HKCU);
            ReplaceTransplacedControls (hwnd, pbffFS->hTemplateWnd);
            DestroyWindow (pbffFS->hTemplateWnd);
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) lpfnOldWndProc);
            RemoveProp(hwnd, ADDTOFAVPROP);
            SHRemoveDefaultDialogFont(hwnd);
            ILFree(pbffFS->patf->pidlSelected);
            LocalFree((HLOCAL)pbffFS);
            break;
        }
        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
                HELP_WM_HELP, (DWORD_PTR)(LPTSTR) (pbffFS->patf->iDlgType == ATF_FAVORITE
                                ? aAddToFavHelpIDs : aAddToChanHelpIDs));
            return TRUE;
            break;

        case WM_CONTEXTMENU:
            SHWinHelpOnDemandWrap((HWND) wParam, c_szHelpFile, HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) (pbffFS->patf->iDlgType == ATF_FAVORITE
                             ? aAddToFavHelpIDs : aAddToChanHelpIDs));
            return TRUE;
            break;

    }

    return CallWindowProc(lpfnOldWndProc, hwnd, uMsg, wParam, lParam);
}


static const TCHAR szTransplacedProp[] = TEXT("tp");
void ReplaceTransplacedControls (HWND hDlgMaster, HWND hDlgTemplate)
{
    /*
     * This function moves the controls that we moved from our temporary
     * dialog over to SHBrowseForFolder's dialog, back to their original
     * home, before they get destroyed.  This is because otherwise we have
     * problems when destroying the template dialog -- specifically, we get
     * a GP fault in user.exe when destroying the edit control, because it
     * looks to its parent window to figure out where its data segment is.
     *
     * Solution: (for safety) -- put everything back where it came from.
     * Other possibilities: just move the edit control (by ID) back, or
     *       move all edit controls back, or use DS_LOCALEDIT for edit controls
     *       (but this is documented only for use with multiline edits.)
     *       Or modify SHBrowseForFolder to allow other dialog templates...
     *       but that's over in shell32.
     */
    HWND hCtrl = GetWindow (hDlgMaster, GW_CHILD);
    while (hCtrl)
    {
        HWND hNext = GetWindow (hCtrl, GW_HWNDNEXT);

        if (GetProp (hCtrl, szTransplacedProp) != NULL)
        {
            RemoveProp (hCtrl, szTransplacedProp);
            SetParent (hCtrl, hDlgTemplate);
        }

        hCtrl = hNext;
    }
}

#define szOriginalWND TEXT("WorkaroundOrigWndProc")
INT_PTR CALLBACK MergeFavoritesDialogControls(HWND hDlgTemplate, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        HWND hDlgMaster = (HWND)lParam;
        ASSERT (IsWindow (hDlgMaster));

        TCHAR szT[200];
        RECT rc;

        //resize master like us
        GetWindowText (hDlgTemplate, szT, ARRAYSIZE(szT));
        SetWindowText (hDlgMaster, szT);
        GetClientRect (hDlgTemplate, &rc);
        AdjustWindowRect (&rc, GetWindowLong (hDlgMaster, GWL_STYLE), FALSE);
        SetWindowPos (hDlgMaster, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
            SWP_NOMOVE | SWP_NOZORDER);
        // a-msadek; BUGBUG: if the owned window is mirrored, a dialog with specifed 
        // coordinates, the dialog get moved to the worng direction
        HWND hWndOwner = GetWindow(hDlgMaster, GW_OWNER);

        if(IS_WINDOW_RTL_MIRRORED(hWndOwner))
            {
                RECT rcOwner, rcDlg;            
                GetWindowRect(hWndOwner, &rcOwner);
                GetWindowRect(hDlgMaster, &rcDlg);
                SetWindowPos(hDlgMaster, NULL, rcDlg.left - (rcDlg.right - rcOwner.right), rcDlg.top, 0 ,0,
                SWP_NOSIZE | SWP_NOZORDER);
            }
            

#if 0   //now we do this as part of the "move controls from template to master" process,
        //if we notice that a ctrl with that id already exists.  This way we pick up the
        //tab order too.  If someone decides my hack (SetParent) to change tab order is
        //broken, then that code can be nuked and this reenabled.

        //position already-existing controls in master like us
        int ID_PREEXIST_CTRLS[] = { IDOK_PLACEHOLDER, IDCANCEL_PLACEHOLDER,
            IDC_SUBSCRIBE_FOLDERLIST_PLACEHOLDER };

        for (int iCtrl = 0; iCtrl < ARRAYSIZE(ID_PREEXIST_CTRLS); iCtrl++)
        {
            GetWindowRect (GetDlgItem (hDlgTemplate, ID_PREEXIST_CTRLS[iCtrl]), &rc);
            MapWindowPoints (NULL, hDlgTemplate, (LPPOINT)&rc, 2);
            MoveWindow (GetDlgItem (hDlgMaster, ID_PREEXIST_CTRLS[iCtrl]),
                rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

            DestroyWindow (GetDlgItem (hDlgTemplate, ID_PREEXIST_CTRLS[iCtrl]));
        }
#endif

        //copy other controls from us to master
        //find last child
        HWND hCtrlTemplate = NULL;
        HWND hNextCtrl = GetWindow (hDlgTemplate, GW_CHILD);
        if (hNextCtrl)      //can't see how this would fail, but...
            hCtrlTemplate = GetWindow (hNextCtrl, GW_HWNDLAST);

        //have last window in hCtrlTemplate
        //now move controls over in reverse order -- they'll end up stacking up in original order from template
        while (hCtrlTemplate)
        {
            hNextCtrl = GetWindow (hCtrlTemplate, GW_HWNDPREV);
            DWORD id = GetWindowLong (hCtrlTemplate, GWL_ID);
            HWND hCtrlExisting;
            if (id != (USHORT)IDC_STATIC && NULL != (hCtrlExisting = GetDlgItem (hDlgMaster, id)))
                //it's one of the controls pre-created by SHBrowseForFolder
            {   //so don't move this one over -- adjust existing control for size, position, tab order
                RECT rc;
                GetWindowRect (hCtrlTemplate, &rc);
                MapWindowPoints (NULL, hDlgTemplate, (LPPOINT)&rc, 2);
                SetWindowPos (hCtrlExisting, NULL, rc.left, rc.top,
                    rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOZORDER);
                DestroyWindow (hCtrlTemplate);
                //REVIEW
                //hack -- send control to end of tab order
                SetParent (hCtrlExisting, hDlgTemplate);
                SetParent (hCtrlExisting, hDlgMaster);
            }
            else    //we should move this control from template to master
            {
                SetProp (hCtrlTemplate, szTransplacedProp, (HANDLE)TRUE);  //anything -- it's the existence of the prop that we check for
                SetParent (hCtrlTemplate, hDlgMaster);          //to know to move this control back later
            }
            hCtrlTemplate = hNextCtrl;
        }
        // Let Template know about the child so that it can forward WM_COMMAND notifications to it
        // to work around the fact that edit controls cache their parent pointers and ignore SetParents
        // when it comes to sending parent notifications
        SetProp(hDlgTemplate, szOriginalWND, hDlgMaster);
    }

        break;
    case WM_COMMAND:
        // Workaround for above bug
        SendMessage((HWND)GetProp(hDlgTemplate, szOriginalWND), uMsg, wParam, lParam);
        break;
    }

    return FALSE;
}


int CALLBACK BFFFavCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch (uMsg)
    {
        case BFFM_INITIALIZED:
        {
            ADDTOFAV* patf = (ADDTOFAV*)lpData;
            ASSERT (patf);

            HWND hDlgTemp = CreateDialogParam(MLGetHinst(), 
                                MAKEINTRESOURCE(IDD_ADDTOFAVORITES_TEMPLATE),
                                NULL, MergeFavoritesDialogControls, (LPARAM)hwnd);
            //this results in all the controls being copied over
            
            //if successful, make our other modifications
            BFFFAVSUBSTRUCT * pbffFavSubStruct;
            if ((IsWindow(GetDlgItem(hwnd, IDC_SUBSCRIBE_CUSTOMIZE)))   //verify existence of randomly-selected control
                && (pbffFavSubStruct = (BFFFAVSUBSTRUCT *) LocalAlloc(LPTR, sizeof(BFFFAVSUBSTRUCT))))
            {
                //done with template, but don't destroy it:
                //see MSKB Q84190, owner/owned vs parent/child -- the children
                // of template are now children of master, but still OWNED
                // by template, and are destroyed when template is destroyed...
                // this kind of sucks, but we'll just keep template around
                // invisibly.
                //we'll take care of it when we go away

                //BUGBUG do we need to do SetDefaultDialogFont stuff for localization still, since it all comes from the .rc?

                //set up window stuff for subclass:
                // Get the TreeView control so we can muck with the style bits and move it down
                HWND hwndT;
                if (hwndT = GetDlgItem(hwnd, IDD_FOLDERLIST))
                {
                    DWORD dwStyle = GetWindowLong(hwndT, GWL_STYLE);
                    dwStyle |= TVS_SHOWSELALWAYS;
                    dwStyle &= ~TVS_LINESATROOT;
                    SetWindowLong(hwndT, GWL_STYLE, dwStyle);
                }

                // don't allow subscriptions if the URL is not "http:" protocol, or if already subscribed
                TCHAR szURL[MAX_URL_STRING];

                if (!patf->pidlFavorite ||
                    FAILED(IEGetDisplayName(patf->pidlFavorite, szURL, SHGDN_FORPARSING)) ||
                    SHRestricted2(REST_NoAddingSubscriptions, szURL, 0) ||
                    !IsSubscribable(szURL) ||
                    !IsFeaturePotentiallyAvailable(CLSID_SubscriptionMgr) ||
                    !IsBrowserFrameOptionsPidlSet(patf->pidlFavorite, BFO_USE_IE_OFFLINE_SUPPORT))
                {
                    CheckDlgButton(hwnd, IDC_MAKE_OFFLINE, 0);
                    EnableWindow(GetDlgItem (hwnd, IDC_MAKE_OFFLINE), FALSE);
                    EnableWindow(GetDlgItem (hwnd, IDC_SUBSCRIBE_CUSTOMIZE), FALSE);
                }
                else if (IsSubscribed(patf))
                {
                    patf->bStartSubscribed = TRUE;
                    CheckDlgButton(hwnd, IDC_MAKE_OFFLINE, 1);
                }
                else if (patf->bIsSoftdist)
                {
                    CheckDlgButton(hwnd, IDC_MAKE_OFFLINE, 1);
                }
                EnableWindow(GetDlgItem(hwnd, IDC_SUBSCRIBE_CUSTOMIZE),
                             IsDlgButtonChecked(hwnd, IDC_MAKE_OFFLINE));

                //set the name
                Edit_LimitText(GetDlgItem(hwnd, IDC_FAVORITE_NAME), MAX_PATH - 1);

                // Use URL if title string is not displayable
                if (SHIsDisplayable(patf->pszFile, g_fRunOnFE, g_bRunOnNT5))
                {  
                    SetDlgItemText(hwnd, IDC_FAVORITE_NAME, patf->pszFile);
                }
                else
                {
                    TCHAR szUrlTemp[MAX_URL_STRING];
                    IEGetDisplayName(patf->pidlFavorite, szUrlTemp, SHGDN_FORPARSING);
                    SetDlgItemText(hwnd, IDC_FAVORITE_NAME, szUrlTemp);
                }

                EnableOKButtonFromID(hwnd, IDC_FAVORITE_NAME);


                // hide the (empty) SHBrowseForFolder prompt control
                ShowWindow(GetDlgItem (hwnd, IDD_BROWSETITLE), SW_HIDE);

                // Fill out the structure and set it as a property so that our subclass
                // proc can get to this data.
                pbffFavSubStruct->lpfnOldWndProc = (WNDPROC) SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)BFFFavSubclass);
                pbffFavSubStruct->hwndNew = GetDlgItem(hwnd, IDC_FAVORITE_NEWFOLDER);
                pbffFavSubStruct->patf = patf;
                pbffFavSubStruct->hwndTV = GetDlgItem(hwnd, IDC_SUBSCRIBE_FOLDERLIST_PLACEHOLDER);
                pbffFavSubStruct->hwndSave = GetDlgItem(hwnd, IDC_FAVORITE_CREATEIN);
                pbffFavSubStruct->hTemplateWnd = hDlgTemp;  //save for explicit destruction later
                GetWindowRect(hwnd, &(pbffFavSubStruct->rcRestored));

                SetProp(hwnd, ADDTOFAVPROP, (HANDLE)pbffFavSubStruct);

                patf->pidlSelected = ILClone(patf->pidl);

                DWORD dwType, dwValue = 0, dwcData = sizeof(dwValue);
                TCHAR szBuffer[100];
                
                SHRegGetUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"), TEXT("AddToFavoritesExpanded"),
                        &dwType, &dwValue, &dwcData, 0, NULL, sizeof(dwValue));

                if (dwValue == 0)
                {
                    int lRet = MLLoadString(IDS_FAVS_ADVANCED_EXPAND, szBuffer, ARRAYSIZE(szBuffer));
                    ASSERT(lRet);

                    // Disable the tree view and new button so that we can't tab to them.
                    EnableWindow(GetDlgItem (hwnd, IDC_SUBSCRIBE_FOLDERLIST_PLACEHOLDER), FALSE);
                    EnableWindow(GetDlgItem (hwnd, IDC_FAVORITE_NEWFOLDER), FALSE);

                    // hide the bottom part of the dialog
                    int cx, cy;
                    RECT rc;
                    GetWindowRect (GetDlgItem (hwnd, IDC_SUBSCRIBE_FOLDERLIST_PLACEHOLDER), &rc);
                    cy = rc.top;
                    GetWindowRect (hwnd, &rc);
                    cx = rc.right - rc.left;
                    cy = cy /*top of ctrl*/ - rc.top; /*top of window*/
                    SetWindowPos (hwnd, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER);
                }
                else
                {
                    //don't show New Folder button for channels in the channels folder,
                    // see code for case IDC_FAVORITE_NEWFOLDER for why
                    if (patf->subsType == SUBSTYPE_CHANNEL)
                    {
                        LPITEMIDLIST pidlFavs = NULL;
                        TCHAR tzFavsPath[MAX_PATH];
                    
                        if (SUCCEEDED(SHGetSpecialFolderLocation(hwnd, CSIDL_FAVORITES, &pidlFavs)) 
                        && SUCCEEDED(SHGetNameAndFlags(pidlFavs, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, tzFavsPath, SIZECHARS(tzFavsPath), NULL))
                        && 0 == StrCmpNI(tzFavsPath, patf->pszInitDir, ARRAYSIZE(tzFavsPath)))
                        {
                            EnableWindow(pbffFavSubStruct->hwndNew, TRUE);
                        }
                        else
                            EnableWindow(pbffFavSubStruct->hwndNew, FALSE);

                        if(pidlFavs)
                            ILFree(pidlFavs);
                    }
                    else
                        EnableWindow(pbffFavSubStruct->hwndNew, TRUE);
                    
                    int lRet = MLLoadString(IDS_FAVS_ADVANCED_COLLAPSE, szBuffer, ARRAYSIZE(szBuffer));
                    ASSERT(lRet);
                }
                SetWindowText(GetDlgItem(hwnd, IDC_FAVORITE_CREATEIN), szBuffer);
                
            }
            else
            {
                EndDialog(hwnd, IDCANCEL);
            }
            break;
        }
        case BFFM_SELCHANGED:
        {
            //the first of these comes during BFFM_INITIALIZED, so ignore it
            if (((ADDTOFAV *)lpData)->pidlSelected != NULL)
            {
                ILFree(((ADDTOFAV *)lpData)->pidlSelected);
                ((ADDTOFAV *)lpData)->pidlSelected = ILClone((LPITEMIDLIST)lParam);
            }
            break;
        }
    }

    return 0;
}


// This API is not exported.  See below (DoAddToFavDlg) for the exported version
//
// hwnd        parent window for the dialog.
// pszInitDir  input: initial path
//             output: fully qualified path and filename
// chInitDir   Length of pszInitDir buffer
// pszFile     initial (default) filename for shortcut
// cchFile     Length of pszFile buffer
// pidlBrowse  associated with pszInitDir
//
// Returns:
//  TRUE if a directory and filename were selected by user, and no error
//  occurs.  In this case pszInitDir contains the new destination directory
//  and filename, pszFile contains the new file name.
//
//  FALSE if an error occurs or the user selects CANCEL.

STDAPI_(BOOL) DoAddToFavDlgEx(HWND hwnd, 
                            TCHAR *pszInitDir, UINT cchInitDir,
                            TCHAR *pszFile, UINT cchFile, 
                            LPITEMIDLIST pidlBrowse,
                            LPCITEMIDLIST pidlFavorite,
                            FAVDLGTYPE atfDlgType,
                            SUBSCRIPTIONINFO* pInfo)
{
    ADDTOFAV atf = {pszInitDir, cchInitDir - 1, pszFile, cchFile - 1, pidlBrowse, NULL,
                    pidlFavorite, atfDlgType, {sizeof(SUBSCRIPTIONINFO), 0}, SUBSTYPE_URL };
    TCHAR szTemp[1];    //NOTE: we're not using SHBrowseForFolder's prompt string (see below)
    TCHAR szDisplayName[MAX_PATH];
    BROWSEINFO bi = {
            hwnd,
            pidlBrowse,
            szDisplayName,
            szTemp,
            BIF_RETURNONLYFSDIRS,
            // (BFFCALLBACK)
            BFFFavCallback,
            (LPARAM)&atf,
            0
    };
    LPITEMIDLIST pidl;

    if (pInfo)
        atf.siSubsInProg = *pInfo;

    switch (atfDlgType)
    {
        case ATF_CHANNEL_SOFTDIST:
            atf.bIsSoftdist = TRUE;
            //  fall through
        case ATF_CHANNEL:
            atf.subsType = SUBSTYPE_CHANNEL;
            break;

        //default:
        //  set in initialize to SUBSTYPE_URL
    }

    //MLLoadString(IDS_FAVS_BROWSETEXT, szTemp, ARRAYSIZE(szTemp));
    //this string is now in the template dialog in the .rc
    //REVIEW -- do we want to do it this way (we're hiding SHBrowse...'s control)? then the template dialog looks more like the finished product...
    szTemp[0] = 0;
 
    //init native font control, otherwise dialog may fail to initialize
    {
        INITCOMMONCONTROLSEX icc;

        icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icc.dwICC = ICC_NATIVEFNTCTL_CLASS;
        InitCommonControlsEx(&icc);
    }
    
    pidl = SHBrowseForFolder(&bi);

    if (pidl)
    {
        ILFree(pidl);
    }

    //  If the user created a new subscription, start a download.
    if (atf.bSubscribed && !atf.bStartSubscribed)
    {
        ISubscriptionMgr* pSubsMgr;
        if (SUCCEEDED (CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER,
                                        IID_ISubscriptionMgr, (void**)&pSubsMgr)))
        {
            WCHAR wszURL[MAX_URL_STRING];

            IEGetDisplayName(atf.pidlFavorite, wszURL, SHGDN_FORPARSING);

            pSubsMgr->UpdateSubscription(wszURL);
            pSubsMgr->Release();
        }
    }

    return (pidl != NULL);
}

STDAPI_(BOOL) DoSafeAddToFavDlgEx(HWND hwnd, 
                            TCHAR *pszInitDir, UINT cchInitDir,
                            TCHAR *pszFile, UINT cchFile, 
                            LPITEMIDLIST pidlBrowse,
                            LPCITEMIDLIST pidlFavorite,
                            FAVDLGTYPE atfDlgType,
                            SUBSCRIPTIONINFO* pInfo)
{
    BOOL fRet;

    if (IEIsLinkSafe(hwnd, pidlFavorite, ILS_ADDTOFAV))
    {
        fRet = DoAddToFavDlgEx(hwnd, pszInitDir, cchInitDir, pszFile, cchFile,
                               pidlBrowse, pidlFavorite, atfDlgType, pInfo);
    }
    else
    {
        fRet = FALSE;
    }

    return fRet;
}


// This API is exported so that it may be called by explorer and mshtml (and MSNVIEWR.EXE)
// in addition to being called internally by shdocvw.
// THEREFORE YOU MUST NOT CHANGE THE SIGNATURE OF THIS API
//

STDAPI_(BOOL) DoAddToFavDlg(HWND hwnd, 
                            CHAR *pszInitDir, UINT cchInitDir,
                            CHAR *pszFile, UINT cchFile, 
                            LPITEMIDLIST pidlBrowse)
{
    BOOL fRet;

    WCHAR szInitDir[MAX_PATH];
    WCHAR szFile[MAX_PATH];

    SHAnsiToUnicode(pszInitDir, szInitDir, ARRAYSIZE(szInitDir));
    SHAnsiToUnicode(pszFile, szFile, ARRAYSIZE(szFile));

    fRet = DoSafeAddToFavDlgEx(hwnd, szInitDir, ARRAYSIZE(szInitDir), szFile, ARRAYSIZE(szFile), pidlBrowse, NULL, ATF_FAVORITE, NULL);

    SHUnicodeToAnsi(szInitDir, pszInitDir, cchInitDir);
    SHUnicodeToAnsi(szFile, pszFile, cchFile);

    return fRet;
}


STDAPI_(BOOL) DoAddToFavDlgW(HWND hwnd, 
                             WCHAR *pszInitDir, UINT cchInitDir,
                             WCHAR *pszFile, UINT cchFile, 
                             LPITEMIDLIST pidlBrowse)
{
    return DoSafeAddToFavDlgEx(hwnd, pszInitDir, cchInitDir, pszFile, cchFile, pidlBrowse, NULL, ATF_FAVORITE, NULL);
}


STDAPI AddToFavoritesEx(HWND hwnd, LPCITEMIDLIST pidlCur, LPCTSTR pszTitle, DWORD dwFlags,
                        SUBSCRIPTIONINFO *pInfo, IOleCommandTarget *pCommandTarget, IHTMLDocument2 *pDoc);
STDAPI AddToChannelsEx (HWND hwnd, LPCITEMIDLIST pidlUrl, LPTSTR pszName, LPCWSTR pwszURL,
                        DWORD dwFlags, SUBSCRIPTIONINFO* pInfo);
STDAPI SubscribeFromFavorites (HWND hwnd, LPCITEMIDLIST pidlUrl, LPTSTR pszName, DWORD dwFlags,
                               SUBSCRIPTIONTYPE subsType, SUBSCRIPTIONINFO *pInfo);


// This API is exported privately, and is called by ISubscriptionMgr::CreateSubscription.
// shuioc uses it too.

STDAPI SHAddSubscribeFavoriteEx (
        HWND hwnd, 
        LPCWSTR pwszURL, 
        LPCWSTR pwszName, 
        DWORD dwFlags,
        SUBSCRIPTIONTYPE subsType, 
        SUBSCRIPTIONINFO* pInfo, 
        IOleCommandTarget *pcmdt,
        IHTMLDocument2 *pDoc)
{
    TCHAR           szName[MAX_PATH];
    LPITEMIDLIST    pidl = NULL;
    HRESULT         hr;
    
    if (pwszURL==NULL || pwszName == NULL)
        return E_INVALIDARG;
    //
    // Need to put pwszName into a buffer because it comes in const
    // but gets modified in SubscribeFromFavorites.
    //
    StrCpyN(szName, pwszName, ARRAYSIZE(szName));

    hr = IECreateFromPath(pwszURL, &pidl);
    if (SUCCEEDED(hr))
    {
        ASSERT (pidl);

        if (dwFlags & CREATESUBS_FROMFAVORITES)
        {
            if (subsType != SUBSTYPE_URL && subsType != SUBSTYPE_CHANNEL)
            {
                ASSERT(0);
                hr = E_INVALIDARG;
            }
            else
            {
                hr = SubscribeFromFavorites (hwnd, pidl, szName, dwFlags, subsType, pInfo);
            }
        }
        else
        {
            if (subsType == SUBSTYPE_URL)
            {
                hr = AddToFavoritesEx (hwnd, pidl, szName, dwFlags, pInfo, pcmdt, pDoc);
            }
            else if (subsType == SUBSTYPE_CHANNEL && !SHIsRestricted2W(hwnd, REST_NoChannelUI, NULL, 0))
            {
                hr = AddToChannelsEx (hwnd, pidl, szName, pwszURL, dwFlags, pInfo);
            }
            else
            {
                ASSERT (0);
                hr = E_INVALIDARG;
            }
        }

        ILFree(pidl);
    }
    return hr;    
}

STDAPI SHAddSubscribeFavorite (HWND hwnd, LPCWSTR pwszURL, LPCWSTR pwszName, DWORD dwFlags,
                               SUBSCRIPTIONTYPE subsType, SUBSCRIPTIONINFO* pInfo)
{
    return SHAddSubscribeFavoriteEx ( hwnd, pwszURL, pwszName, dwFlags,
                                subsType, pInfo, NULL, NULL);
}

// this API is also exported via the .def
// Use for backward compatibility only -- note that it is only for URL's (not channels)
// and doesn't know how to subscribe.
STDAPI AddUrlToFavorites(HWND hwnd, LPWSTR pszUrlW, LPWSTR pszTitleW, BOOL fDisplayUI)
{
    return SHAddSubscribeFavorite (hwnd, pszUrlW, pszTitleW,
        fDisplayUI ? CREATESUBS_NOUI : 0, SUBSTYPE_URL, NULL);
}


// this API is in the .h and is used elsewhere in shdocvw, but is not exported
// Backward compatibility only -- only for URL's (not channels) and can subscribe, but can't
// pass in subscriptioninfo starter.
STDAPI AddToFavorites(
    HWND hwnd, 
    LPCITEMIDLIST pidlCur, 
    LPCTSTR pszTitle, 
    BOOL fDisplayUI, 
    IOleCommandTarget *pCommandTarget,
    IHTMLDocument2 *pDoc)
{
    return AddToFavoritesEx (hwnd, pidlCur, pszTitle,
        fDisplayUI ? 0 : CREATESUBS_NOUI, NULL, pCommandTarget, pDoc);
}


//helper function to create one column in a ListView control, add one item to that column,
//size the column to the width of the control, and color the control like a static...
//basically, like SetWindowText for a ListView.  Because we use a lot of ListViews to display
//urls that would otherwise be truncated... the ListView gives us automatic ellipsis and ToolTip.
void SetListViewToString (HWND hLV, LPCTSTR pszString)
{
    ASSERT(hLV);
    
    LV_COLUMN   lvc = {0};
    RECT lvRect;
    GetClientRect (hLV, &lvRect);
    lvc.mask = LVCF_WIDTH;
    lvc.cx = lvRect.right - lvRect.left;
    if (-1 == ListView_InsertColumn(hLV, 0, &lvc))   {
        ASSERT(0);
    }

    SendMessage(hLV, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_INFOTIP, LVS_EX_INFOTIP);

    LV_ITEM lvi = {0};
    lvi.iSubItem = 0;
    lvi.pszText = (LPTSTR)pszString;
    lvi.mask = LVIF_TEXT;
    ListView_InsertItem(hLV, &lvi);
    ListView_EnsureVisible(hLV, 0, TRUE);
    
    ListView_SetBkColor(hLV, GetSysColor(COLOR_BTNFACE));
    ListView_SetTextBkColor(hLV, GetSysColor(COLOR_BTNFACE));
}


INT_PTR CALLBACK SubscribeFavoriteDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ADDTOFAV * patf = (ADDTOFAV*)GetProp(hDlg, ADDTOFAVPROP);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            TCHAR szURL[MAX_URL_STRING];

            patf = (ADDTOFAV*)lParam;
            SetProp(hDlg, ADDTOFAVPROP, (HANDLE)patf);

            //set up name and url displays
            SetDlgItemText (hDlg, IDC_CHANNEL_NAME, patf->pszFile);
            //url is in patf->pidlFavorite
            IEGetDisplayName(patf->pidlFavorite, szURL, SHGDN_FORPARSING);
            SetListViewToString (GetDlgItem (hDlg, IDC_CHANNEL_URL), szURL);

            //now the tricky part... this is for modifying the subscription associated with
            //an existing ChannelBar shortcut.  We need to find out if they are subscribed --
            //if so, load the existing subscription into memory so it can be modified in the
            //wizard.  If not, leave the information that was passed up because it's got the
            //schedule extracted from the CDF.  In either case, check the radio button that
            //corresponds to their current subscription level.
            ISubscriptionMgr* pSubsMgr;
            BOOL bSubs = FALSE;

            HRESULT hr = JITCoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER,
                                             IID_ISubscriptionMgr, (void**)&pSubsMgr, 
                                             hDlg, FIEF_FLAG_FORCE_JITUI | FIEF_FLAG_PEEK);

            if (SUCCEEDED(hr))
            {
                pSubsMgr->IsSubscribed(szURL, &bSubs);

                patf->bStartSubscribed = bSubs;

                pSubsMgr->Release();
            }
            else if ((E_ACCESSDENIED == hr) || !IsBrowserFrameOptionsPidlSet(patf->pidlFavorite, BFO_USE_IE_OFFLINE_SUPPORT))
            {
                EnableWindow(GetDlgItem(hDlg, IDC_MAKE_OFFLINE), FALSE);
            }

            if (!bSubs && patf->bIsSoftdist)
            {
                bSubs = TRUE;
            }

            CheckDlgButton(hDlg, IDC_MAKE_OFFLINE, bSubs ? 1 : 0);
            EnableWindow(GetDlgItem (hDlg, IDC_SUBSCRIBE_CUSTOMIZE), bSubs);
        }
        break;

    case WM_DESTROY:
        RemoveProp (hDlg, ADDTOFAVPROP);
        break;

    case WM_HELP:
        SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
            HELP_WM_HELP, (DWORD_PTR)(LPTSTR) (patf->iDlgType == ATF_FAVORITE
                            ? aAddToFavHelpIDs : aAddToChanHelpIDs));
        return TRUE;
        break;

    case WM_CONTEXTMENU:
        SHWinHelpOnDemandWrap((HWND) wParam, c_szHelpFile, HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID) (patf->iDlgType == ATF_FAVORITE
                         ? aAddToFavHelpIDs : aAddToChanHelpIDs));
        return TRUE;
        break;

    case WM_COMMAND:
        ASSERT (patf);
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;

        case IDOK:
            // first, make sure they're not trying to subscribe to an authenticated
            // channel without entering a password.
            if (SubscriptionFailsChannelAuthentication (hDlg, &patf->siSubsInProg))
                return FALSE;

            //find out whether they WERE subscribed, so if they click OK and they
            //were already subscribed, we delete that subscription -- and either leave it
            //deleted if "No subs" was the choice, or create the new one.
            ISubscriptionMgr* pSubsMgr;
            if (SUCCEEDED (JITCoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER,
                                            IID_ISubscriptionMgr, (void**)&pSubsMgr, 
                                            hDlg, FIEF_FLAG_FORCE_JITUI)))
            {
                //url is in patf->pidlFavorite
                TCHAR szURL[MAX_URL_STRING];
                IEGetDisplayName(patf->pidlFavorite, szURL, SHGDN_FORPARSING);

                BOOL bAlreadySubs;
                if (SUCCEEDED (pSubsMgr->IsSubscribed (szURL, &bAlreadySubs)) && bAlreadySubs)
                {
                    pSubsMgr->DeleteSubscription(szURL, NULL);
                }

                pSubsMgr->Release();
            }

            if (IsDlgButtonChecked (hDlg, IDC_MAKE_OFFLINE))
            {
               //they want to subscribe!  save subscription we already have in memory
                patf->bSubscribed = StartSiteSubscription (hDlg, patf, TRUE);
            }
            EndDialog(hDlg, IDOK);
            break;

        //BUGBUG common code with ATF dialog

        case IDC_SUBSCRIBE_CUSTOMIZE:
            //need to create -- but not store -- subscription
            //need to (temporarily) trash patf->pidlFavorite so that we can go through the
            //wizard without colliding with an existing subscription.  When we actually create
            //the subscription, we'll use the real name.
            LPCITEMIDLIST pidlSave = patf->pidlFavorite;
            TCHAR szUrlTemp[MAX_URL_STRING+1];
            IEGetDisplayName(patf->pidlFavorite, szUrlTemp, SHGDN_FORPARSING);
            StrCat (szUrlTemp, TEXT("."));   //just put something nearly invisible on the end
            if (SUCCEEDED (IECreateFromPath(szUrlTemp, (LPITEMIDLIST*)&patf->pidlFavorite)))
            {
                if (StartSiteSubscription (hDlg, patf, FALSE))
                    SendMessage (hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDOK), TRUE);
                ILFree ((LPITEMIDLIST)patf->pidlFavorite);
            }
            patf->pidlFavorite = pidlSave;
            break;
        }
        break;
    }

    return FALSE;
}


static const int CREATESUBS_ACTIVATE = 0x8000;      //hidden flag meaning channel is already on system

STDAPI SubscribeFromFavorites (HWND hwnd, LPCITEMIDLIST pidlUrl, LPTSTR pszName, DWORD dwFlags,
                               SUBSCRIPTIONTYPE subsType, SUBSCRIPTIONINFO *pInfo)
{
    //used to subscribe to a channel that's already in the Favorites
    //or a URL that's already a Favorite

    //flags are same as ISubscriptionMgr::CreateSubscription

    //display our part of the fav's dialog -- no need to go through SHBrowseForFolder
    //or any of that, just our radio buttons in a fixed-size dialog with our own DlgProc

    INT_PTR iDlgResult;
    HRESULT hr = S_OK;
    ADDTOFAV atf = {0};
    atf.pszFile = pszName;
    atf.siSubsInProg.cbSize = sizeof(SUBSCRIPTIONINFO);
    if (pInfo && pInfo->cbSize == sizeof(SUBSCRIPTIONINFO))
        atf.siSubsInProg = *pInfo;

    atf.subsType = subsType;

    //figure out what dialog to use
    atf.iDlgType = (subsType == SUBSTYPE_URL ? ATF_FAVORITE :
        (dwFlags & CREATESUBS_ACTIVATE ? ATF_CHANNEL_MODIFY : ATF_CHANNEL));
    //BUGBUG do we potentially need ANOTHER dialog type for softdist channels?

    if (dwFlags & CREATESUBS_SOFTWAREUPDATE)
    {
        atf.bIsSoftdist = TRUE;
    }

    atf.pidlFavorite = pidlUrl;

#ifdef OLD_FAVORITES
    int iTemplate;
    switch (atf.iDlgType)
    {
    case ATF_CHANNEL_SOFTDIST: //BUGBUG inappropriate, but it doesn't currently get used
    case ATF_CHANNEL:
        iTemplate = IDD_SUBSCRIBE_FAV_CHANNEL;
        break;
    case ATF_CHANNEL_MODIFY:
        iTemplate = IDD_ACTIVATE_PLATINUM_CHANNEL;
        break;
    case ATF_FAVORITE:
        iTemplate = IDD_SUBSCRIBE_FAVORITE;
        break;
    }

    iDlgResult = DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(iTemplate), hwnd,
            (DLGPROC)SubscribeFavoriteDlgProc, (LPARAM)&atf);

#endif

    iDlgResult = DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_ADDTOFAVORITES_TEMPLATE), hwnd,
            (DLGPROC)SubscribeFavoriteDlgProc, (LPARAM)&atf);


    switch (iDlgResult)
    {
    case -1:
        hr = E_FAIL;
        break;
    case IDCANCEL:
        hr = S_FALSE;
        break;
    default:
        if (pInfo && (pInfo->cbSize == sizeof(SUBSCRIPTIONINFO))
                  && (dwFlags & CREATESUBS_NOSAVE))
            *pInfo = atf.siSubsInProg;
        hr = S_OK;
        break;
    }

    return hr;
}


STDAPI AddToChannelsEx (HWND hwnd, LPCITEMIDLIST pidlUrl, LPTSTR pszName, LPCWSTR pwszURL,
                        DWORD dwFlags, SUBSCRIPTIONINFO* pInfo)
{
    HRESULT hr = S_OK;
    IChannelMgrPriv* pIChannelMgrPriv;

    hr = JITCoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER,
                          IID_IChannelMgrPriv, (void**)&pIChannelMgrPriv, 
                          hwnd, FIEF_FLAG_FORCE_JITUI);

    if (SUCCEEDED(hr))
    {
        if (S_OK == pIChannelMgrPriv->IsChannelInstalled (pwszURL))
        {
            hr = SubscribeFromFavorites (hwnd, pidlUrl, pszName, dwFlags | CREATESUBS_ACTIVATE,
                SUBSTYPE_CHANNEL, pInfo);
        }
        else
        {
            LPITEMIDLIST pidlChannelFolder;
            TCHAR szPath[MAX_PATH];
            TCHAR szCFPath[MAX_PATH];

            ASSERT(pIChannelMgrPriv);

            IChannelMgrPriv::CHANNELFOLDERLOCATION cflLocation =
                ((dwFlags & CREATESUBS_SOFTWAREUPDATE) ?
                    IChannelMgrPriv::CF_SOFTWAREUPDATE :
                    IChannelMgrPriv::CF_CHANNEL);

            hr = pIChannelMgrPriv->GetChannelFolder(&pidlChannelFolder, cflLocation);
            if (SUCCEEDED (hr))
            {
                //
                // Change IChannelMgrPriv to unicode!  This has to get fixed to
                // support a unicode "Channels" name. (edwardp)
                //

                CHAR szBuff[MAX_PATH];

                hr = pIChannelMgrPriv->GetChannelFolderPath (szBuff, ARRAYSIZE(szBuff), cflLocation);

                if (SUCCEEDED(hr))
                    SHAnsiToUnicode(szBuff, szCFPath, ARRAYSIZE(szCFPath));
                

                if (SUCCEEDED (hr))
                {
                    TCHAR szDspName[MAX_URL_STRING];
                    DWORD cchDspName = ARRAYSIZE(szDspName);

                    StrCpyN(szPath, szCFPath, ARRAYSIZE(szPath));
            
                    // When we create a short cut for the URL, we have to make sure it's readable for
                    // the end user. PrepareURLForDisplay() will unescape the string if it's escaped.
                    if (!UrlIs(pszName, URLIS_URL) ||
                        !PrepareURLForDisplay(pszName, szDspName, &cchDspName))
                    {
                        // Unescaping wasn't wanted or didn't work.
                        StrCpyN(szDspName, pszName, ARRAYSIZE(szDspName));
                    }
                         
                    PathCleanupSpec(szPath, szDspName);

                    FAVDLGTYPE iDlgType = (dwFlags & CREATESUBS_SOFTWAREUPDATE ? ATF_CHANNEL_SOFTDIST : ATF_CHANNEL);

                    if ((dwFlags & CREATESUBS_NOUI) || 
                        DoSafeAddToFavDlgEx(hwnd, szPath, ARRAYSIZE(szPath), 
                                        szDspName, ARRAYSIZE(szDspName), pidlChannelFolder,
                                        pidlUrl, iDlgType, pInfo))
                    {
                        //we create the channelbar entry here, instead of cdfview, because here
                        //we know where in the channels folder the user wants it to go.
                        IChannelMgr* pChannelMgr = NULL;
                        hr = pIChannelMgrPriv->QueryInterface (IID_IChannelMgr, (void**)&pChannelMgr);
                        if (SUCCEEDED (hr))
                        {
                            //prepare strings
                            PathRemoveExtension(szPath);

                            //strip off absolute part of folder path, and convert to Unicode
                            int cchCommon = PathCommonPrefix (szPath, szCFPath, NULL);

                            //pack in the info we have
                            CHANNELSHORTCUTINFO csiChannel = {0};
                            csiChannel.cbSize = sizeof(csiChannel);
                            csiChannel.pszTitle = szPath + cchCommon;
                            csiChannel.pszURL = (LPWSTR)pwszURL;
                            csiChannel.bIsSoftware = (dwFlags & CREATESUBS_SOFTWAREUPDATE) ? TRUE : FALSE;
                            //and tell the channel mgr to add the channel
                            hr = pChannelMgr->AddChannelShortcut (&csiChannel);
                            pChannelMgr->Release();
                        }
                    }
                    else
                    {
                        hr = S_FALSE;       //no failure, but no add
                    }
                }

                ILFree (pidlChannelFolder);
            }
        }
        pIChannelMgrPriv->Release();
    }

    return hr;
}


STDAPI AddToFavoritesEx(
    HWND hwnd, 
    LPCITEMIDLIST pidlCur, 
    LPCTSTR pszTitle,
    DWORD dwFlags, 
    SUBSCRIPTIONINFO *pInfo, 
    IOleCommandTarget *pCommandTarget,
    IHTMLDocument2 *pDoc)
{
    HRESULT hres = S_FALSE;
    HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (pidlCur)
    {
        TCHAR szName[MAX_URL_STRING];
        TCHAR szPath[MAX_PATH];
        if (pszTitle)
        {
            StrCpyN(szName, pszTitle, ARRAYSIZE(szName));
        }
        else
        {
            szName[0] = 0;

            IEGetNameAndFlags(pidlCur, SHGDN_INFOLDER | SHGDN_NORMAL, szName, SIZECHARS(szName), NULL);
        }

        LPITEMIDLIST pidlFavorites;

        if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_FAVORITES, TRUE) &&
            (pidlFavorites = SHCloneSpecialIDList(NULL, CSIDL_FAVORITES, TRUE)))
        {
            TCHAR szDspName[MAX_PATH];
            DWORD cchDspName = ARRAYSIZE(szDspName);
            
            // When we create a short cut for the URL, we have to make sure it's readable for
            // the end user. PrepareURLForDisplay() will unescape the string if it's escaped.
            if (!UrlIs(szName, URLIS_URL) ||
                !PrepareURLForDisplay(szName, szDspName, &cchDspName))
            {
                // Unescaping wasn't wanted or didn't work.
                StrCpyN(szDspName, szName, ARRAYSIZE(szDspName));
            }

            PathCleanupSpec(szPath, szDspName);

            // if left with spaces only, use the filename friendly version of the url instead.
            StrTrim(szDspName, L" ");
            if (szDspName[0] == 0)
            {
                if (SUCCEEDED(IEGetNameAndFlags(pidlCur, SHGDN_FORPARSING, szDspName, ARRAYSIZE(szDspName), NULL)))
                    PathCleanupSpec(szPath, szDspName);
            }

            BOOL fDisplayUI = (dwFlags & CREATESUBS_NOUI) ? FALSE : TRUE;
            if (!fDisplayUI || 
                DoSafeAddToFavDlgEx(hwnd, szPath, ARRAYSIZE(szPath), 
                                    szDspName, ARRAYSIZE(szDspName), pidlFavorites,
                                    pidlCur, ATF_FAVORITE, NULL))
            {
                if (fDisplayUI)
                    PathRemoveFileSpec(szPath);
                    
                ISHCUT_PARAMS ShCutParams = {0};
                
                PathRemoveExtension(szDspName);
                
                ShCutParams.pidlTarget = pidlCur;
                ShCutParams.pszTitle = PathFindFileName(szDspName); 
                ShCutParams.pszDir = szPath; 
                ShCutParams.pszOut = NULL;
                ShCutParams.bUpdateProperties = FALSE;
                ShCutParams.bUniqueName = FALSE;
                ShCutParams.bUpdateIcon = TRUE;
                ShCutParams.pCommand = pCommandTarget;
                ShCutParams.pDoc = pDoc;
                hres = CreateShortcutInDirEx(&ShCutParams);
                if (fDisplayUI && FAILED(hres)) 
                {
                    IE_ErrorMsgBox(NULL, hwnd, GetLastError(), NULL, szDspName, IDS_FAV_UNABLETOCREATE, MB_OK| MB_ICONSTOP);
                }
            }
            else
            {
                hres = S_FALSE;
            }
            ILFree(pidlFavorites);
        }
    }

    SetCursor(hCursorOld);
    
    return hres;
}


BOOL IsSubscribed(ADDTOFAV *patf)
{
    BOOL bSubscribed = FALSE;

    TCHAR szURL[MAX_URL_STRING];
    if (SUCCEEDED(IEGetDisplayName(patf->pidlFavorite, szURL, SHGDN_FORPARSING)))
    {
        ISubscriptionMgr *pSubscriptionMgr;
        if (SUCCEEDED(CoCreateInstance(CLSID_SubscriptionMgr,
                                       NULL, CLSCTX_INPROC_SERVER,
                                       IID_ISubscriptionMgr,
                                       (void**)&pSubscriptionMgr)))
        {
            BSTR bstrURL = SysAllocStringT(szURL);
            if (bstrURL)
            {
                if (SUCCEEDED(pSubscriptionMgr->IsSubscribed(bstrURL, &bSubscribed)) &&
                    bSubscribed)
                {
                    patf->siSubsInProg.fUpdateFlags = SUBSINFO_ALLFLAGS;
                    pSubscriptionMgr->GetSubscriptionInfo(bstrURL, &patf->siSubsInProg);
                }
                SysFreeString(bstrURL);
            }
            pSubscriptionMgr->Release();
        }
    }

    return bSubscribed;
}

BOOL IsSubscribed(LPCITEMIDLIST pidl)
{
    BOOL bSubscribed = FALSE;

    TCHAR szURL[MAX_URL_STRING];
    if (FAILED(IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szURL, SIZEOF(szURL), NULL)))
        return FALSE;

    bSubscribed = IsSubscribed(szURL);    

    return bSubscribed;
}

BOOL IsSubscribed(LPWSTR pwzUrl)
{
#ifndef DISABLE_SUBSCRIPTIONS

    BOOL bSubscribed = FALSE;

    ISubscriptionMgr * pSubscriptionMgr;
    if (FAILED(CoCreateInstance(CLSID_SubscriptionMgr,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_ISubscriptionMgr,
                                (void**)&pSubscriptionMgr)))
    {
        return FALSE;
    }

    pSubscriptionMgr->IsSubscribed(pwzUrl, &bSubscribed);
    pSubscriptionMgr->Release();

    return bSubscribed;

#else  /* !DISABLE_SUBSCRIPTIONS */

    return FALSE;

#endif /* !DISABLE_SUBSCRIPTIONS */
}
