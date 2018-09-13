#include "stdafx.h"
#pragma hdrstop
//#include "shellp.h"
//#include "deskstat.h"
//#include "dutil.h"
//#include "ids.h"
//#include "resource.h"
//#include "shellids.h"
//#include "htmlhelp.h"
//#include "advanced.h"
//#include "options.h"
//#include "browseui.h"

#include <mluisupp.h>

void Cabinet_StateChanged(CABINETSTATE *pcs);
void Cabinet_RefreshAll(void);
BOOL UpdateDefFolderSettings(DEFFOLDERSETTINGS* pdfsNew, DEFFOLDERSETTINGS* pdfsOld);

#define SZ_FOLDEROPTSTUBCLASS TEXT("MSGlobalFolderOptionsStub")

const TCHAR c_szHtmlHelp[] = TEXT("update.chm > iedefault");
const TCHAR c_szHtmlTopic[] = TEXT("ie4_use_mouse_over.htm");

//---------------------------------------------------------------------------
const static DWORD aFolderOptsHelpIDs[] = {  // Context Help IDs
    IDC_FOPT_PREVIEW,            IDH_SAMPLE_GRAPHIC,
    IDC_FOPT_INTEGRATION_GROUPBOX,IDH_GROUPBOX,
    IDC_FOPT_RADIO_WEB,          IDH_WEB_VIEW,
    IDC_FOPT_WEBTEXTDESCRIPTION, IDH_WEB_VIEW,
    IDC_FOPT_RADIO_CLASSIC,      IDH_CLASSIC_STYLE,
    IDC_FOPT_CLASSICTEXT,        IDH_CLASSIC_STYLE,
    IDC_FOPT_RADIO_CUSTOM,       IDH_CUSTOM,
    IDC_FCUS_SAME_WINDOW,        IDH_BROWSE_SAME_WINDOW,
    IDC_FCUS_SEPARATE_WINDOWS,   IDH_BROWSE_SEPARATE_WINDOWS,
    IDC_FCUS_WHENEVER_POSSIBLE,  IDH_SHOW_WEB_WHEN_POSSIBLE,
    IDC_FCUS_WHEN_CHOOSE,        IDH_SHOW_WEB_WHEN_CHOOSE,
    IDC_FCUS_SINGLECLICK,        IDH_SINGLE_CLICK_MODE,
    IDC_FCUS_DOUBLECLICK,        IDH_DOUBLE_CLICK_MODE,
    IDC_FCUS_ICON_IE,            IDH_TITLES_LIKE_LINKS,
    IDC_FCUS_ICON_HOVER,         IDH_TITLES_WHEN_POINT,
    IDC_FOPT_BUTTON_SETTINGS,    IDH_CUSTOM_SETTINGS,
    IDC_FCUS_WEB,                IDH_ENABLE_WEB_CONTENT,
    IDC_FCUS_CLASSIC,            IDH_USE_WINDOWS_CLASSIC,
    IDC_FCUS_CUSTOMIZE,          IDH_CUSTOMIZE_ACTIVE_DESKTOP,
    0, 0
};

typedef struct
{
    BOOL fLaunchWebTab;   //Should I Launch the Web Tab at the exit ?
    CABINETSTATE cs;      // Cached "current" CabState.
    CFolderOptionsPsx *ppsx;    // to talk to our propsheet sibling
    DEFFOLDERSETTINGS dfs;  // The folder settings that drive our UI
                            // not guaranteed up-to-date; always double-check
                            // before relying on it for something important
} FOLDEROPTDATA;

typedef struct
{
    HBITMAP *phbm;
    BOOL fLaunchWebTab : 1;
    BOOL fOKCancelToClose : 1;
    FOLDEROPTDATA *pfod; // Pointer to parent dialog
} CUSTOMDATA;

BOOL CALLBACK FolderCustomOptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

//This functions gets/sets the current Active Desktop value

// if 'set' is true then it takes the value in *pActDesk and sets the ActiveDesktop
// accordingly (TRUE-Active Desktop is On, FALSE-Active Desktop is off)

// if 'set' is false then it gets the ActiveDesktop state  and assigns that value to the
// *pActDesk 
void GetSetActiveDesktop(BOOL *pActDesk, BOOL fset)
{
    IActiveDesktop* pad;
    HRESULT hres;
    
    //Get a pointer to the IActiveDesktop interface
    hres = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER,
        IID_IActiveDesktop, (void**)&pad);

    if(SUCCEEDED(hres))
    {
        COMPONENTSOPT co;
        co.dwSize = sizeof(COMPONENTSOPT);
        
        //Get the Current Options
        pad->GetDesktopItemOptions(&co, NULL);

        //Do we have to set new value ?
        if (fset)
        {
            if (co.fActiveDesktop != *pActDesk)
            {
                //Yes? Set the new value
                co.fActiveDesktop = *pActDesk;
                pad->SetDesktopItemOptions(&co, NULL);

                //Apply the change 
                pad->ApplyChanges(AD_APPLY_REFRESH);
            }
        }
        else
        {
              //No, Get the current state  and return
            *pActDesk  = co.fActiveDesktop;         
        }

        pad->Release();
    }

}

HBITMAP _GetCustomBitmap(BOOL fActiveDesktop,BOOL fSingleWindow, BOOL fWebView, BOOL fSingleClick)
{
    
    /*  Naming Convention:
        The naming is based on the following scheme
        A - Active Desktop   0 means Off                        1 Means on
        W - Window           1 means Single Window              2 Means Multiple Window
        V - View             0 means Classic View               1 means Web View
        U - Underline/Click  0 means Double Click               1 means single Click

        Example :
            A1W2V1U0

        Bitmap corresponding to the case where Active Desktop is turned on, folders are opened in
        multiple window , we have Web Style View and Double Click is necessary to invoke the item
    */

    //Start with First ID
    UINT id = IDB_A1W1V1U1;
#if 0 // BUGBUG SHDOC401_UI should I do this?
    if (g_bRunOnMemphis)
        id = IDB_MA1W1V1U1;
#endif

    if (!fActiveDesktop)
    {
        //Active Desktop is not so the bitmap corresponding to this case must 
        //be 8 bitmaps away from the current

        id += 8;
    }


    if (!fSingleWindow)
    {
        //if its single window then the bitmap corresponding to this case must be 4 bitmaps
        // away from the current

        id += 4;
    }

    if (!fWebView)
    {
        //if its not web view then the bitmap corresponding to this case must be 2 bitmaps
        // away from the current
        id += 2;        

    }

    if (!fSingleClick)
    {
        //if its not single click then the bitmap corresponding to this case must be 1 bitmap
        // away from the current

        id += 1;

    }

    return (HBITMAP)LoadImage(g_hinst, MAKEINTRESOURCE(id),IMAGE_BITMAP,0,0,
                                          LR_LOADMAP3DCOLORS);
}

BOOL CALLBACK CheckSCDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        CheckRadioButton(hDlg, IDC_SC_YES, IDC_SC_NO, IDC_SC_YES);
        
        break;
    }
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
        {
            SHELLSTATE ss = {0};
            
            SHGetSetSettings(&ss, SSF_DOUBLECLICKINWEBVIEW, FALSE);

            if (BOOLIFY(ss.fDoubleClickInWebView) != BOOLIFY(IsDlgButtonChecked(hDlg, IDC_SC_NO)))
            {
                ss.fDoubleClickInWebView = IsDlgButtonChecked(hDlg, IDC_SC_NO);
                SHGetSetSettings(&ss, SSF_DOUBLECLICKINWEBVIEW, TRUE);
                
                Cabinet_RefreshAll();
            }

            EndDialog(hDlg, TRUE);            
            break;
        }    

        case IDC_SC_MOREINFO:
           MLHtmlHelpWrap(hDlg, c_szHtmlHelp, HH_DISPLAY_TOPIC, (ULONG_PTR)c_szHtmlTopic, ML_CROSSCODEPAGE);
           break;

        default:
            return FALSE;
        }
        break;
    }
           
    return FALSE;
}

BOOL CheckSingleClickDialog(HWND hwndParent)
{
    BOOL fRet = FALSE;
    HKEY hkey;
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\MICROSOFT\\WINDOWS\\CURRENTVERSION\\EXPLORER\\ADVANCED"),
                0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL))
    {
        DWORD dwType, dwData;
        DWORD cbData = SIZEOF(dwData);
        if (ERROR_SUCCESS != RegQueryValueEx(hkey, TEXT("CheckedSCDlg"), NULL,
            &dwType, (LPBYTE)&dwData, &cbData) || !dwData)
        {
            dwData = 1;
            RegSetValueEx(hkey, TEXT("CheckedSCDlg"), 0, REG_DWORD, (LPBYTE)&dwData, SIZEOF(dwData));

            DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_CHECKSINGLECLICK), hwndParent, CheckSCDlgProc);
            fRet = TRUE;
        }
        
        RegCloseKey(hkey);
    }

    return fRet;
}

// sets current selection + sets a bitmap and enables the settings button if necessary
UINT SetCurrentSelection(HWND hDlg)
{
    SHELLSTATE  ss = {0};
    BOOL        fActiveDesktop;
    UINT        id      = IDC_FOPT_RADIO_CUSTOM;
    HBITMAP     hbitmap = NULL;
    HBITMAP     hbitmapOld;
    DWORD       dwIconUnderline = ICON_IE;
    DWORD       cb              = SIZEOF(dwIconUnderline);

    FOLDEROPTDATA *pfod = (FOLDEROPTDATA *)GetWindowLongPtr(hDlg, DWL_USER);
    ASSERT(pfod);

    pfod->ppsx->SetNeedRefresh(FALSE);

    //Get the current settings
    SHGetSetSettings(&ss, SSF_WIN95CLASSIC | SSF_DOUBLECLICKINWEBVIEW, FALSE);
    ReadCabinetState(&pfod->cs, SIZEOF(pfod->cs));
        
    SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                    TEXT("IconUnderline"),
                    NULL,
                    &dwIconUnderline,
                    &cb,
                    FALSE,
                    &dwIconUnderline,
                    cb);

    //Get the state of Active Desktop
    GetSetActiveDesktop(&fActiveDesktop, FALSE);

    // Get the current default folder settings
    EVAL(SUCCEEDED(pfod->ppsx->GetDefFolderSettings(
                           &pfod->dfs, sizeof(pfod->dfs))));

    //Find the Current Selection
    if (SHRestricted(REST_CLASSICSHELL) ||
                     ((pfod->dfs.bUseVID == FALSE || pfod->dfs.vid != VID_WebView) &&
                     (pfod->cs.fNewWindowMode) &&
                     (ss.fDoubleClickInWebView) &&
                     (!fActiveDesktop)))
    {
        id = IDC_FOPT_RADIO_CLASSIC;
        hbitmap = _GetCustomBitmap(FALSE, FALSE, FALSE, FALSE);
    }
    else if ((!ss.fWin95Classic)         &&
             (pfod->dfs.bUseVID) && (pfod->dfs.vid == VID_WebView) &&
             (!ss.fDoubleClickInWebView) && (dwIconUnderline == ICON_IE) &&
             (!pfod->cs.fNewWindowMode) &&
             (fActiveDesktop))
    {
        id = IDC_FOPT_RADIO_WEB;
        hbitmap = _GetCustomBitmap(TRUE, TRUE, TRUE, TRUE);
    }
    else
    {
        hbitmap = _GetCustomBitmap( fActiveDesktop,
                                    pfod->cs.fNewWindowMode ? FALSE:TRUE,
                                    (pfod->dfs.bUseVID && (pfod->dfs.vid == VID_WebView)),
                                    ss.fDoubleClickInWebView ? FALSE : TRUE);
    }

    //Set  the current selection and bitmap corresponding to it
    CheckRadioButton(hDlg, IDC_FOPT_RADIO_WEB, IDC_FOPT_RADIO_CUSTOM, id);
    hbitmapOld = (HBITMAP)SendMessage(GetDlgItem(hDlg, IDC_FOPT_PREVIEW), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbitmap);
    if (hbitmapOld)
    {
        DeleteObject(hbitmapOld);
    }
    EnableWindow(GetDlgItem(hDlg, IDC_FOPT_BUTTON_SETTINGS),id==IDC_FOPT_RADIO_CUSTOM);

    return id;
}


BOOL CALLBACK FolderOptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL fCheckedSingleClickDialog = FALSE;
    static BOOL fCheckedWebStyle = FALSE;

    INSTRUMENT_WNDPROC(SHCNFI_FOLDEROPTIONS_DLGPROC, hDlg, uMsg, wParam, lParam);

    FOLDEROPTDATA *pfod = (FOLDEROPTDATA *)GetWindowLongPtr(hDlg, DWL_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
    {
        pfod = (FOLDEROPTDATA *)LocalAlloc(LPTR, SIZEOF(*pfod));
        if (pfod)
        {               
            //Set the Folder Options data
            SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pfod);

            PROPSHEETPAGE *pps = (PROPSHEETPAGE *)lParam;
            pfod->ppsx = (CFolderOptionsPsx *)pps->lParam;

            SetCurrentSelection(hDlg);

            if (SHRestricted(REST_CLASSICSHELL))
            {
                // If in classic shell restriction, disable the unavailable variations.
                EnableWindow(GetDlgItem(hDlg, IDC_FOPT_RADIO_CUSTOM), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_FOPT_RADIO_WEB), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_FOPT_WEBTEXTDESCRIPTION), FALSE);
            }
        }
        else
        {
            // Can't use EndDialog because we weren't created by DialogBox()
            DestroyWindow(hDlg);
        }
        return TRUE;
    }
    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        case PSN_APPLY:
        {
            BOOL fOldValue = BOOLIFY(pfod->cs.fNewWindowMode);
            BOOL fActiveDesktop;
            SHELLSTATE oldss = {0};
            SHELLSTATE ss = { 0 };

            //Get the Current Settings             
            SHGetSetSettings(&oldss, SSF_WIN95CLASSIC | SSF_DOUBLECLICKINWEBVIEW, FALSE);
            GetSetActiveDesktop(&fActiveDesktop, FALSE);
            EVAL(SUCCEEDED(pfod->ppsx->GetDefFolderSettings(
                                    &pfod->dfs, sizeof(pfod->dfs))));

            if (pfod->ppsx->NeedRefresh())
            {
                SetCurrentSelection(hDlg);
            }

            //Now check the user selection and gather information
            if (IsDlgButtonChecked(hDlg, IDC_FOPT_RADIO_CLASSIC))
            {
                ss.fDoubleClickInWebView  = TRUE;
                ss.fWin95Classic          = TRUE;
                pfod->cs.fNewWindowMode   = TRUE;
                pfod->dfs.bUseVID         = FALSE;
                fActiveDesktop            = FALSE;
            }
            else if (IsDlgButtonChecked(hDlg, IDC_FOPT_RADIO_WEB))
            {
                ss.fDoubleClickInWebView    = FALSE;
                ss.fWin95Classic            = FALSE;
                pfod->cs.fNewWindowMode     = FALSE;
                pfod->dfs.bUseVID           = TRUE;
                pfod->dfs.vid               = VID_WebView;
                fActiveDesktop              = TRUE;

                DWORD dwIconUnderline = ICON_IE;
                
                SHRegSetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                                TEXT("IconUnderline"),
                                NULL,
                                &dwIconUnderline,
                                SIZEOF(dwIconUnderline),
                                SHREGSET_DEFAULT);

                SendMessage(HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM)TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\IconUnderline"));
                
            }
            else    // custom is set, we don't have anything to do.
                return TRUE;


            //Change the state according to the information collected
            
            // force the save; do this before notifying cabinets to refresh
            // IE4 forced SETASDEFAULT even though it clobbered fs settings
            EVAL(SUCCEEDED(pfod->ppsx->SetDefFolderSettings(
                        &pfod->dfs, sizeof(pfod->dfs), GFSS_SETASDEFAULT)));

            // Set the Active Desktop
            GetSetActiveDesktop(&fActiveDesktop, TRUE);      //TRUE means set the value

            
            if ((ss.fWin95Classic         != oldss.fWin95Classic)   ||
                (ss.fDoubleClickInWebView != oldss.fDoubleClickInWebView))
            {
                SHGetSetSettings(&ss, SSF_WIN95CLASSIC | SSF_DOUBLECLICKINWEBVIEW, TRUE);
                Cabinet_RefreshAll();
            }

            // notify if there was a change
            if (fOldValue != (pfod->cs.fNewWindowMode ? 1 : 0))
                Cabinet_StateChanged( &pfod->cs );

            if (!fCheckedSingleClickDialog && fCheckedWebStyle)
            {
                fCheckedSingleClickDialog = TRUE;
                if(CheckSingleClickDialog(hDlg))
                {
                   //We popped up the Single Click Dialog . User might have said no to single click so 
                   //Check for that condition and change the setting accordingly.
                   SetCurrentSelection(hDlg);                   
                }
            }

            return TRUE;
        }

        case PSN_KILLACTIVE:
            // validate here
            // SetWindowLong(hDlg, DWL_MSGRESULT, !ValidateLink());   // don't allow close
            return TRUE;

        case PSN_SETACTIVE:
            if (pfod->ppsx->NeedRefresh())
            {
                SetCurrentSelection(hDlg);
            }
            return TRUE;
        }
        break;

    case WM_HELP:
        SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, TEXT("update.hlp"),
           HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aFolderOptsHelpIDs);
        break;

    case WM_CONTEXTMENU:
        SHWinHelpOnDemandWrap((HWND) wParam, TEXT("update.hlp"), HELP_CONTEXTMENU,
            (ULONG_PTR)(LPVOID)aFolderOptsHelpIDs);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_FOPT_RADIO_WEB:
            case IDC_FOPT_RADIO_CLASSIC:
            case IDC_FOPT_RADIO_CUSTOM:
            {
                fCheckedWebStyle = (GET_WM_COMMAND_ID(wParam, lParam) == IDC_FOPT_RADIO_WEB);
                if (GET_WM_COMMAND_CMD(wParam,lParam) == BN_CLICKED)
                {
                    HBITMAP hbitmap;
                    EnableWindow(GetDlgItem(hDlg, IDC_FOPT_BUTTON_SETTINGS),GET_WM_COMMAND_ID(wParam,lParam)==IDC_FOPT_RADIO_CUSTOM);

                    if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_FOPT_RADIO_WEB)
                    {
                        hbitmap = _GetCustomBitmap(TRUE, TRUE, TRUE, TRUE);
                    }
                    else if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_FOPT_RADIO_CLASSIC)
                    {
                        hbitmap = _GetCustomBitmap(FALSE, FALSE, FALSE, FALSE);
                    }
                    else if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_FOPT_RADIO_CUSTOM)
                    {
                        SHELLSTATE ss = {0};
                        BOOL  fActiveDesktop;
                        GetSetActiveDesktop(&fActiveDesktop, FALSE);  // FALSE means  get

                        SHGetSetSettings(&ss,SSF_DOUBLECLICKINWEBVIEW,FALSE);
                        hbitmap = _GetCustomBitmap(fActiveDesktop,
                                                   pfod->cs.fNewWindowMode ? FALSE:TRUE,
                                                   (pfod->dfs.bUseVID && (pfod->dfs.vid == VID_WebView)),
                                                   ss.fDoubleClickInWebView ? FALSE : TRUE);
                    }

                    DeleteObject((LPVOID)SendMessage(GetDlgItem(hDlg, IDC_FOPT_PREVIEW),STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbitmap));

                    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                }
                break;
            }
            case IDC_FOPT_BUTTON_SETTINGS:
            {
                SHELLSTATE ss={0};
                HBITMAP hbm=NULL;
                CUSTOMDATA cd = { &hbm, FALSE, FALSE, pfod };

                SHGetSetSettings(&ss, SSF_DOUBLECLICKINWEBVIEW, FALSE);

                DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_FOLDERCUSTOM), hDlg, FolderCustomOptionsDlgProc, (LPARAM)&cd);
                SetCurrentSelection(hDlg);
                if (cd.fOKCancelToClose)
                {
                    // user clicked on OK so the settings are saved and OK & Cancel buttons
                    // don't make sense anymore
                    PropSheet_CancelToClose(GetParent(hDlg));
                }
                else if (cd.fLaunchWebTab)
                {
                    pfod->fLaunchWebTab = TRUE;
                    PropSheet_PressButton(GetParent(hDlg), PSBTN_OK);
                }
            }
            break;
        }
        break;
        
    case WM_DESTROY:
        if (pfod)
        {
            if (pfod->fLaunchWebTab)
            {
                TCHAR szWebTab[MAX_PATH];
                MLLoadString(IDS_DESKTOPWEBSETTINGS, szWebTab, ARRAYSIZE(szWebTab));
                SHRunControlPanel(szWebTab, NULL);
            }
            SetWindowLongPtr(hDlg, DWL_USER, 0);
            LocalFree((HANDLE)pfod);
        }
        
        DeleteObject((LPVOID)SendMessage(GetDlgItem(hDlg, IDC_FOPT_PREVIEW),STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)NULL));

        break;
    }
           
    return FALSE;
}

BOOL CALLBACK FolderCustomOptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // INSTRUMENT_WNDPROC(SHCNFI_FOLDEROPTIONS_DLGPROC, hDlg, uMsg, wParam, lParam);

    CUSTOMDATA *pcd = (CUSTOMDATA *)GetWindowLongPtr(hDlg, DWL_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
    {
        SHELLSTATE ss={0};
        BOOL  fActiveDesktop;

        // save our custom data
        SetWindowLongPtr(hDlg, DWL_USER, lParam);
        pcd = (CUSTOMDATA *)lParam;

        //Active Desktop
        GetSetActiveDesktop(&fActiveDesktop, FALSE); // False means get
        CheckRadioButton(hDlg, IDC_FCUS_WEB, IDC_FCUS_CLASSIC, fActiveDesktop ? IDC_FCUS_WEB:IDC_FCUS_CLASSIC);
                
        SHGetSetSettings(&ss, SSF_DOUBLECLICKINWEBVIEW | SSF_WIN95CLASSIC, FALSE);

        // browse folder options
        CheckRadioButton(hDlg,
                         IDC_FCUS_SAME_WINDOW,
                         IDC_FCUS_SEPARATE_WINDOWS,
                         pcd->pfod->cs.fNewWindowMode ? IDC_FCUS_SEPARATE_WINDOWS:IDC_FCUS_SAME_WINDOW);

        // show folders as web pages      
        CheckRadioButton(hDlg,
                         IDC_FCUS_WHENEVER_POSSIBLE,
                         IDC_FCUS_WHEN_CHOOSE,
                         ((pcd->pfod->dfs.bUseVID) && (pcd->pfod->dfs.vid == VID_WebView)) ? IDC_FCUS_WHENEVER_POSSIBLE : IDC_FCUS_WHEN_CHOOSE);

        // single/double click
        CheckRadioButton(hDlg,
                         IDC_FCUS_SINGLECLICK,IDC_FCUS_DOUBLECLICK,
                         !ss.fWin95Classic ? (ss.fDoubleClickInWebView ? IDC_FCUS_DOUBLECLICK:IDC_FCUS_SINGLECLICK) : IDC_FCUS_DOUBLECLICK);

        // gray out icon underline behvaior when not in single click mode
        BOOL fChecked = IsDlgButtonChecked(hDlg, IDC_FCUS_SINGLECLICK);
        EnableWindow(GetDlgItem(hDlg, IDC_FCUS_ICON_IE),    fChecked);
        EnableWindow(GetDlgItem(hDlg, IDC_FCUS_ICON_HOVER), fChecked);

        DWORD dwIconUnderline = ICON_IE;
        DWORD cb = sizeof(dwIconUnderline);
        
        SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                        TEXT("IconUnderline"),
                        NULL,
                        &dwIconUnderline,
                        &cb,
                        FALSE,
                        &dwIconUnderline,
                        cb);
        
        CheckRadioButton(hDlg,
                         IDC_FCUS_ICON_IE,IDC_FCUS_ICON_HOVER,
                         dwIconUnderline == ICON_IE ? IDC_FCUS_ICON_IE : IDC_FCUS_ICON_HOVER);

        return TRUE;
    }
    case WM_HELP:
        SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, TEXT("update.hlp"),
           HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aFolderOptsHelpIDs);
        break;

    case WM_CONTEXTMENU:
        SHWinHelpOnDemandWrap((HWND) wParam, TEXT("update.hlp"), HELP_CONTEXTMENU,
            (ULONG_PTR)(LPVOID)aFolderOptsHelpIDs);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDOK:
            {
                SHELLSTATE oldss={0};
                SHELLSTATE ss = { 0 };

                //Active Desktop
                BOOL fActiveDesktop = IsDlgButtonChecked(hDlg, IDC_FCUS_WEB);
                GetSetActiveDesktop(&fActiveDesktop, TRUE); //True means set
                EVAL(SUCCEEDED(pcd->pfod->ppsx->GetDefFolderSettings(
                                &pcd->pfod->dfs, sizeof(pcd->pfod->dfs))));

                SHGetSetSettings(&oldss, SSF_DOUBLECLICKINWEBVIEW | SSF_WIN95CLASSIC, FALSE);

                BOOL fOldValue = pcd->pfod->cs.fNewWindowMode ? 1 : 0;
                if (IsDlgButtonChecked(hDlg, IDC_FCUS_SAME_WINDOW))
                    pcd->pfod->cs.fNewWindowMode = FALSE;
                else
                    pcd->pfod->cs.fNewWindowMode = TRUE;
                if (fOldValue != (pcd->pfod->cs.fNewWindowMode ? 1 : 0))
                    Cabinet_StateChanged( &pcd->pfod->cs );

                DEFFOLDERSETTINGS dfsOld = pcd->pfod->dfs;
                if (IsDlgButtonChecked(hDlg, IDC_FCUS_WHENEVER_POSSIBLE))
                {
                    // ss.fWin95Classic = FALSE;
                    pcd->pfod->dfs.bUseVID = TRUE;
                    pcd->pfod->dfs.vid = VID_WebView;
                }
                else
                {
                    pcd->pfod->dfs.bUseVID = FALSE;
                }
                // force the save
                // IE4 forced SETASDEFAULT even though it clobbered fs settings
                EVAL(SUCCEEDED(pcd->pfod->ppsx->SetDefFolderSettings(
                            &pcd->pfod->dfs, sizeof(pcd->pfod->dfs), GFSS_SETASDEFAULT)));

                UpdateDefFolderSettings(&pcd->pfod->dfs, &dfsOld);

                if (IsDlgButtonChecked(hDlg, IDC_FCUS_SINGLECLICK))
                {
                    ss.fDoubleClickInWebView = FALSE;
                    ss.fWin95Classic = FALSE;
                }
                else
                {
                    ss.fDoubleClickInWebView = TRUE;
                    ss.fWin95Classic = FALSE;
                }

                DWORD dwIconUnderline;
                DWORD cb = sizeof(dwIconUnderline);

                if (IsDlgButtonChecked(hDlg, IDC_FCUS_ICON_IE))
                    dwIconUnderline = ICON_IE;
                else
                    dwIconUnderline = ICON_HOVER;
                
                SHRegSetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                                TEXT("IconUnderline"),
                                NULL,
                                &dwIconUnderline,
                                cb,
                                SHREGSET_DEFAULT);

                SendMessage(HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM)TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\IconUnderline"));
                    
                if (ss.fWin95Classic != oldss.fWin95Classic
                    || ss.fDoubleClickInWebView != oldss.fDoubleClickInWebView)
                {
                    SHGetSetSettings(&ss, SSF_WIN95CLASSIC | SSF_DOUBLECLICKINWEBVIEW, TRUE);
                    Cabinet_RefreshAll();
                }

                // if we clicked on customize button we don't want buttons 
                // to change: OK->Close, Cancel->gray out
                // so don't set fOKCancelToClose
                if (!pcd->fLaunchWebTab)
                    pcd->fOKCancelToClose = TRUE;
            }

            // fall through...
            
            case IDCANCEL:
                EndDialog(hDlg,GET_WM_COMMAND_ID(wParam,lParam));
                break;

            case IDC_FCUS_SINGLECLICK:
            case IDC_FCUS_DOUBLECLICK:
                if (GET_WM_COMMAND_CMD(wParam,lParam) == BN_CLICKED)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_FCUS_ICON_IE),    GET_WM_COMMAND_ID(wParam,lParam) == IDC_FCUS_SINGLECLICK);
                    EnableWindow(GetDlgItem(hDlg, IDC_FCUS_ICON_HOVER), GET_WM_COMMAND_ID(wParam,lParam) == IDC_FCUS_SINGLECLICK);
                }
                break;

            case IDC_FCUS_CUSTOMIZE:
                TCHAR szText[1024], szTitle[MAX_PATH];

                MLLoadString(IDS_CUSTOMDESK_TITLE, szTitle, ARRAYSIZE(szTitle));
                MLLoadString(IDS_CUSTOMDESK_TEXT, szText, ARRAYSIZE(szText));

                if (MessageBox(hDlg, szText, szTitle, MB_YESNO | MB_ICONINFORMATION) == IDYES)
                {
                    pcd->fLaunchWebTab = TRUE;
                    PostMessage(hDlg, WM_COMMAND, MAKELPARAM(IDOK, 0), 0);
                }
                break;
        }
        break;
        
    case WM_DESTROY:
        break;
    }
           
    return FALSE;
}

#ifdef SHDOC401_DLL

HWND CreateGlobalFolderOptionsStubWindow(void)
{
    WNDCLASS wc;
    DWORD dwExStyle = WS_EX_TOOLWINDOW;
    if (!GetClassInfo(HINST_THISDLL, SZ_FOLDEROPTSTUBCLASS, &wc))
    {
        wc.style         = 0;
        wc.lpfnWndProc   = DefWindowProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = HINST_THISDLL;
        wc.hIcon         = NULL;
        wc.hCursor       = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = SZ_FOLDEROPTSTUBCLASS;

        if (!RegisterClass(&wc))
            return NULL;
    }
    if (IS_BIDI_LOCALIZED_SYSTEM()) {
        dwExStyle |= dwExStyleRTLMirrorWnd;
    }    

    return CreateWindowEx(dwExStyle, SZ_FOLDEROPTSTUBCLASS, c_szNULL, WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, HINST_THISDLL, NULL);
}

BOOL CALLBACK AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    if (ppsh->nPages < MAXPROPPAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }
    return FALSE;
}

IShellPropSheetExt *
AddPropSheetCLSID(REFCLSID clsid, PROPSHEETHEADER *ppsh)
{
    HRESULT hres;
    IShellPropSheetExt *psx;

    hres = SHCoCreateInstance(NULL, &clsid, NULL,
                              IID_IShellPropSheetExt, (LPVOID *)&psx);
    if (SUCCEEDED(hres)) {
        IShellExtInit *psxi;
        hres = psx->QueryInterface(IID_IShellExtInit, (LPVOID *)&psxi);
        if (SUCCEEDED(hres)) {
            hres = psxi->Initialize(NULL, NULL, 0);
            psxi->Release();
        } else {
            // The File Types extension doesn't support IShellExtInit.. Grr..
            // This means that the absence of IShellExtInit is not an error.
            hres = S_OK;
        }
        if (SUCCEEDED(hres)) {
            psx->AddPages(AddPropSheetPage, (LPARAM)ppsh);
        }
    }
    if (FAILED(hres)) {
        ATOMICRELEASE(psx);
    }
    return psx;
}


DWORD CALLBACK GlobalFolderOptPropSheetThreadProc(LPVOID)
{
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE rPages[MAXPROPPAGES];
    IShellPropSheetExt *psx1, *psx2;
    HRESULT hrInit;

    hrInit = SHCoInitialize();

    //
    // Create stub parent window.
    //
    HWND hwndStub = CreateGlobalFolderOptionsStubWindow();

    //
    // Property sheet stuff.
    //
    psh.dwSize = SIZEOF(psh);
    psh.dwFlags = PSH_DEFAULT;
    psh.hInstance = MLGetHinst();
    psh.hwndParent = hwndStub;
    psh.pszCaption = MAKEINTRESOURCE(IDS_FOLDEROPT_TITLE);
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = rPages;

    //
    // Add the Folder Options tabs.
    //
    psx1 = AddPropSheetCLSID(CLSID_ShellFldSetExt, &psh);

    //
    // Add the File Types tab.
    //
    psx2 = AddPropSheetCLSID(CLSID_FileTypes, &psh);

    //
    // Display the property sheet.
    //
    // BUGBUG this should be a call to MLPropertySheet
    PropertySheet(&psh);

    //
    // Clean up the propsheet extensions.  We must do this *after*
    // the property sheet is destroyed.
    //
    ATOMICRELEASE(psx1);
    ATOMICRELEASE(psx2);

    //
    // Clean up stub window.
    //
    DestroyWindow(hwndStub);

    SHCoUninitialize(hrInit);

    return 0;
}

BOOL CALLBACK FindFolderOptionsEnumProc(HWND hwnd, LPARAM lParam)
{
    BOOL fRet = TRUE;
    HWND *phwnd = (HWND *)lParam;
    TCHAR szClass[MAX_PATH];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));

    if (lstrcmp(szClass, SZ_FOLDEROPTSTUBCLASS) == 0)
    {
        *phwnd = hwnd;
        fRet = FALSE;
    }

    return fRet;
}

void DoGlobalFolderOptions(void)
{
    HWND hwnd = NULL;
    
    EnumWindows(FindFolderOptionsEnumProc, (LPARAM)&hwnd);

    if (hwnd)
    {
        hwnd = GetLastActivePopup(hwnd);
        if (hwnd && IsWindow(hwnd))
        {
            SetForegroundWindow(hwnd);
        }
    }
    else
    {
        HANDLE hThread;
        DWORD dwThread;

        hThread = CreateThread(NULL, 0, GlobalFolderOptPropSheetThreadProc, NULL, 
            THREAD_PRIORITY_NORMAL, &dwThread);

        if (hThread != NULL)
        {
            CloseHandle(hThread);
        }
    }
}

#endif // SHDOC401_DLL

// Moved from defview.cpp, which never used these functions

const TCHAR c_szExploreClass[]  = TEXT("ExploreWClass");
const TCHAR c_szIExploreClass[] = TEXT("IEFrame");
const TCHAR c_szCabinetClass[]  = 
#ifdef IE3CLASSNAME
    TEXT("IEFrame");
#else
    TEXT("CabinetWClass");
#endif


BOOL IsNamedWindow(HWND hwnd, LPCTSTR pszClass)
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    return lstrcmp(szClass, pszClass) == 0;
}

BOOL IsExplorerWindow(HWND hwnd)
{
    return IsNamedWindow(hwnd, c_szExploreClass);
}

BOOL IsTrayWindow(HWND hwnd)
{
    return IsNamedWindow(hwnd, TEXT(WNDCLASS_TRAYNOTIFY));
}

BOOL IsFolderWindow(HWND hwnd)
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    return (lstrcmp(szClass, c_szCabinetClass) == 0) || (lstrcmp(szClass, c_szIExploreClass) == 0);
}

BOOL CALLBACK Cabinet_RefreshEnum(HWND hwnd, LPARAM lParam)
{
    if (IsFolderWindow(hwnd) || IsExplorerWindow(hwnd))
    {
        PostMessage(hwnd, WM_COMMAND, FCIDM_REFRESH, 0L);
    }

    return(TRUE);
}

void Cabinet_RefreshAll(void)
{
    HWND hwndDesktop = FindWindowEx(NULL, NULL, TEXT(STR_DESKTOPCLASS), NULL);
    if (hwndDesktop)
        PostMessage(hwndDesktop, WM_COMMAND, FCIDM_REFRESH, 0L);

    EnumWindows(Cabinet_RefreshEnum, 0);
}


BOOL CALLBACK Folder_UpdateEnum(HWND hwnd, LPARAM lParam)
{
    if (IsFolderWindow(hwnd) || IsExplorerWindow(hwnd))
    {
        // A value of -1L for lParam will force a refresh by loading the View window
        // with the new VID as specified in the global DefFolderSettings.
        PostMessage(hwnd, WM_COMMAND, SFVIDM_MISC_SETWEBVIEW, lParam);
    }
    return(TRUE);
}

BOOL UpdateDefFolderSettings(DEFFOLDERSETTINGS* pdfsNew, DEFFOLDERSETTINGS* pdfsOld)
{
    BOOL bRet = FALSE;

    if (pdfsNew->bUseVID != pdfsOld->bUseVID
            || pdfsNew->vid != pdfsOld->vid)   // The setting has changed
    {
        LPARAM lParam;

        if (pdfsNew->bUseVID && IsEqualGUID(pdfsNew->vid, VID_WebView))
        {
            lParam = (LPARAM)TRUE;
        }
        else if (pdfsOld->bUseVID && IsEqualGUID(pdfsOld->vid, VID_WebView))
        {
            lParam = (LPARAM)FALSE;
        }
        EnumWindows(Folder_UpdateEnum, lParam);
        bRet = TRUE;
    }
    return bRet;
}


BOOL CALLBACK Cabinet_GlobalStateEnum(HWND hwnd, LPARAM lParam)
{
    if (IsFolderWindow(hwnd) || IsExplorerWindow(hwnd))
    {
        PostMessage(hwnd, CWM_GLOBALSTATECHANGE, 0, 0L);
    }

    return(TRUE);
}


void Cabinet_StateChanged(CABINETSTATE *pcs)
{
    // Save the new settings away...
    WriteCabinetState( pcs );

    EnumWindows(Cabinet_GlobalStateEnum, 0);
}
