//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996                    **
//*********************************************************************

//
// ADVANCED.C - "Advanced" Property Sheet
//

//
//  This is a registry driven UI that walk through the part of the
//  registry tree and convert it into a tree view list.
//  Note that registry values can be localized, so internation versions
//  have to localize the INF file so that setup will set the registry
//  with the right text.
//
//  Here is how the registry key looks like,
//
//  HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\AdvancedOptions\
//      GroupKey1\
//          Type="group"
//          Text="text to display"
//          DefaultImage="somedll.dll,icon_index", (optional).
//
//          RadioKey1.1\
//              Type="radio"
//              Text="text to display"
//   (optional) HKeyRoot=any one of the pre-defined root HKEY in DWORD,
//                  ; default to HKEY_CURRENT_USER.
//   (optional) RegPath="path to the key where the setting value is stored."
//                  ; default to Software\Microsoft\Windows\CurrentVersion\Explorer\AdvancedOptions
//              ValueName="reg value name."
//              CheckedValue=...    ; value for this radio button,
//                  ; if the value is platform dependent, use one of the following two instead
//              CheckedValueNT=...  ; if the value is platform dependent
//              CheckedValueW95=... ; if the value is platform dependent
//              DefaultValue=...
//   (optional) SPIAction=uiAction param to SystemParametersInfo in DWORD
//   (optional) SPIParamON=uiParam param to SystemParametersInfo for this radio button in DWORD
//   (optional) Mask=...          ; mask for the bitfield 1's for the bits we want to be able to set/clear
//              
//          RadioKey1.2\
//              Type="radio"
//              Text="text to display"
//   (optional) HKeyRoot=any one of the pre-defined root HKEY in DWORD,
//                  ; default to HKEY_CURRENT_USER.
//   (optional) RegPath="path to the key where the setting value is stored."
//                  ; default to Software\Microsoft\Windows\CurrentVersion\Explorer\AdvancedOptions
//              ValueName="reg value name."
//              CheckedValue=...    ; value for this checkbox if it's checked
//                  ; if the value is platform dependent, use one of the following two instead
//              CheckedValueNT=...  ; if the value is platform dependent
//              CheckedValueW95=... ; if the value is platform dependent
//              UncheckedValue=...  ; value for this checkbox if it's unchecked
//              DefaultValue=...
//   (optional) SPIAction=uiAction param to SystemParametersInfo in DWORD
//   (optional) SPIParamON=uiParam param to SystemParametersInfo if checked in DWORD
//   (optional) SPIParamOFF=uiParam param to SystemParametersInfo if checked in DWORD
//   (optional) Mask=...          ; mask for the bitfield 1's for the bits we want to be able to set/clear
//
//          CheckBoxKey1.1\
//              ...
//          GroupKey1.1\
//              ...
//      GroupKey2\
//          ...
//          CheckBoxKey2.1\
//              ...
//          CheckBoxKey2.2\
//              ...
//
//
//  Notes:
//      1. All the settings are store in HKCU\...\Explorer\Settings key,
//          code can be added to support any random absolute reg path for
//          the settings.
//

#include "stdafx.h"
#pragma hdrstop

void Cabinet_StateChanged(void);
void Cabinet_RefreshAll(void);

//
// Private Calls and structures
//
//

typedef struct {
    HWND hDlg;              // handle of our dialog
    HWND hwndTree;          // handle to the treeview
    IRegTreeOptions *pTO;    // pointer to RegTreeOptions interface
    CFolderOptionsPsx *ppsx;    // to talk to our propsheet sibling
    BOOL    fDirty;          // Dirty bit to detect if anything changed.
} ADVANCEDPAGE, *LPADVANCEDPAGE;


#define ENABLEAPPLY(hDlg) SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L)

// local special keys
#define REGSTR_PATH_ADVANCEDLIST    REGSTR_PATH_EXPLORER TEXT("\\Advanced")

//
//
//
//
// AdvancedDlgInit()
//
// Initializes Advanced property sheet
//
// History:
//
void Install_AdvancedShellSettings(SHELLSTATE * pss);
BOOL AdvancedDlgInit(HWND hDlg, LPARAM lParam)
{
    HRESULT hr;
    LPADVANCEDPAGE  pAdv = (LPADVANCEDPAGE)LocalAlloc(LPTR, sizeof(*pAdv));
    if (!pAdv)
    {
        EndDialog(hDlg, 0);
        return FALSE;   // no memory?
    }

    // We expose some of SHGetSetSettings in the advanced settings section,
    // so we need to migrate these PER-USER settings from the SHELLSTATE
    // structure into the registry tree structure once per user.  Since we
    // don't have a per-user install section, do it here on demand.
    // We don't have to do it every time, but that's easier...
    Install_AdvancedShellSettings(NULL);
    
    // tell dialog where to get info
    SetWindowPtr(hDlg, DWLP_USER, pAdv);

    PROPSHEETPAGE *pps = (PROPSHEETPAGE *)lParam;

    pAdv->ppsx = (CFolderOptionsPsx *)pps->lParam;

    //Check if we have pointer to Shell Browser
    if (!pAdv->ppsx->HasBrowserService())
    {
        //Hey ! we dont have pointer to Shell Browser so we must have been
        //invoked through a global Folder Options . In this case it doesn't 
        //make sense to  say "You can make all your folders look the same Like Current Folder"
        //because we dont have a current folder so let just disable the button "Like Current Folder"
        EnableWindow(GetDlgItem(hDlg, IDC_ADVO_USECURRENTFOLDER), FALSE);
    }

    // save dialog handle
    pAdv->hDlg = hDlg;
    pAdv->hwndTree = GetDlgItem( pAdv->hDlg, IDC_ADVO_ADVANCEDTREE );
    DWORD dwServerType = CLSCTX_INPROC_SERVER;

    hr = CoCreateInstance(CLSID_CRegTreeOptions, NULL, dwServerType,
                          IID_IRegTreeOptions, (LPVOID *)&(pAdv->pTO));
    if (SUCCEEDED(hr))
    {
        // HACK - IRegTreeOptions is ANSI, so we temporarily turn off UNICODE
        #undef TEXT
        #define TEXT(s) s
        hr = pAdv->pTO->InitTree(pAdv->hwndTree, HKEY_LOCAL_MACHINE, REGSTR_PATH_ADVANCEDLIST, NULL);
        #undef TEXT
        #define TEXT(s) __TEXT(s)
    }
    
    // find the first root and make sure that it is visible
    TreeView_EnsureVisible(pAdv->hwndTree, TreeView_GetRoot( pAdv->hwndTree ));

    return SUCCEEDED(hr) ? TRUE : FALSE;
}



//
//
//
void Tree_OnNotify( LPADVANCEDPAGE pAdv )
{
    TV_HITTESTINFO ht;

    // BUGBUG REVIEW: shouldn't this be GetMessagePos?
    GetCursorPos( &ht.pt );                         // get where we were hit
    ScreenToClient( pAdv->hwndTree, &ht.pt );       // translate it to our window

    // retrieve the item hit
    pAdv->pTO->ToggleItem(TreeView_HitTest( pAdv->hwndTree, &ht));
    ENABLEAPPLY(pAdv->hDlg);
    pAdv->fDirty = TRUE;

} // Tree_OnNotify


#if 0
void AnimateToTray(HWND hwnd)
{
    HWND hwndTray = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hwndTray)
    {
        HWND hwndIcons = FindWindowEx(hwndTray, NULL, TEXT("TrayNotifyWnd"), NULL);
        if (hwndIcons)
        {
            RECT rcSource, rcDest;
            GetWindowRect(hwnd, &rcSource);
            GetWindowRect(hwndIcons, &rcDest);

            DrawAnimatedRects(hwnd, IDANI_CAPTION, &rcSource, &rcDest);
        }
    }
}
#endif

//
// AdvancedDlgOnNotify()
//
// Handles Advanced property sheets WM_NOTIFY messages
//
//
void AdvancedDlgOnNotify(LPADVANCEDPAGE pAdv, LPNMHDR psn)
{
    SetWindowLongPtr( pAdv->hDlg, DWLP_MSGRESULT, 0); // handled

    switch (psn->code)
    {
    case TVN_KEYDOWN:
    {
        TV_KEYDOWN *pnm = (TV_KEYDOWN*)psn;
        if (pnm->wVKey == VK_SPACE)
        {
            pAdv->pTO->ToggleItem((HTREEITEM)SendMessage(pAdv->hwndTree, TVM_GETNEXTITEM, TVGN_CARET, NULL));
            ENABLEAPPLY(pAdv->hDlg);
            pAdv->fDirty = TRUE;
            // Specify that we handled the key, so we don't beep
            SetWindowLongPtr(pAdv->hDlg, DWLP_MSGRESULT, TRUE);
        }
        break;
    }

    case NM_CLICK:
    case NM_DBLCLK:
        // is this click in our tree?
        if ( psn->idFrom == IDC_ADVO_ADVANCEDTREE )
        {
            // yes...
            Tree_OnNotify( pAdv );
        }

        break;

    case PSN_APPLY:
    {
#if 0
        if (ShellMessageBox(HINST_THISDLL, pAdv->hDlg,
                            MAKEINTRESOURCE(IDS_SUPERHIDDENWARNING),
                            MAKEINTRESOURCE(IDS_FOLDERVIEWS),
                            MB_YESNO | MB_ICONINFORMATION) == IDYES)
        {

        }
#endif

        if(pAdv->fDirty) //We will save these only if something changed.
        {
            pAdv->pTO->WalkTree(WALK_TREE_SAVE);

            CABINETSTATE cs;
            ReadCabinetState(&cs, sizeof(cs));   // refresh the global CABINETSTATE
            WriteCabinetState(&cs);              // and make sure we commit to the bitfield.
        
            // We should only notify if needed.

            SHRefreshSettings();        // refresh the SHELLSTATE structure

            SHSettingsChanged(0, 0);    // invalidate the restrictions

            // Let everybody know about the new settings
            SendNotifyMessage(HWND_BROADCAST, WM_WININICHANGE, 0, 0);

            // Okay now tell all Cabinets to refresh
            Cabinet_RefreshAll();
        }

        break; // PSN_APPLY
    }

    } // switch

}


//---------------------------------------------------------------------------
const static DWORD aAdvOptsHelpIDs[] = {  // Context Help IDs
    IDC_ADVO_GROUPBOX,               IDH_COMM_GROUPBOX,
    IDC_ADVO_ADVANCEDTEXT,       IDH_GROUPBOX,
    IDC_ADVO_ADV_RESTORE_DEF,    IDH_RESTORE_DEFAULT,
    IDC_ADVO_RESETTOORIGINAL,    IDH_RESET_TO_ORIGINAL,
    IDC_ADVO_USECURRENTFOLDER,   IDH_USE_CURRENT_FOLDER,
    IDC_ADVO_IMAGEFOLDER,       -1,        //Suppress help for this item.
    IDC_ADVO_STATICTEXT,        -1,        //Suppress help for this item.
    0, 0
};


//
// AdvancedDlgProc
//
// History:
//
//
BOOL_PTR CALLBACK AdvancedOptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam)
{
    LPADVANCEDPAGE pAdv = (LPADVANCEDPAGE)GetWindowPtr(hDlg, DWLP_USER);

    if ( uMsg == WM_INITDIALOG)
    {
        // Initialize Dialog controls
// Don't write to the registry randomly -raymondc
//      WriteCabinetState( &g_CabState );   // make sure reg is in sync, since we might modify
        return AdvancedDlgInit(hDlg, lParam);
    }

    if ( pAdv )
    {
        switch (uMsg)
        {

        case WM_NOTIFY:
            AdvancedDlgOnNotify(pAdv, (LPNMHDR)lParam);
            return TRUE;
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_ADVO_ADV_RESTORE_DEF:
                pAdv->pTO->WalkTree(WALK_TREE_RESTORE);
                ENABLEAPPLY(hDlg);
                pAdv->fDirty = TRUE;
                break;

            case IDC_ADVO_USECURRENTFOLDER:
                if (ShellMessageBox(HINST_THISDLL, hDlg,
                                    MAKEINTRESOURCE(IDS_LIKECURRENT_TEXT),
                                    MAKEINTRESOURCE(IDS_FOLDERVIEWS),
                                    MB_YESNO | MB_ICONINFORMATION) == IDYES)
                {
                    pAdv->ppsx->SetAsDefFolderSettings();
                    pAdv->ppsx->SetNeedRefresh(TRUE);
                    pAdv->fDirty = TRUE;
                }
                break;

            case IDC_ADVO_RESETTOORIGINAL:
                if (ShellMessageBox(HINST_THISDLL, hDlg,
                                    MAKEINTRESOURCE(IDS_RESETALL_TEXT),
                                    MAKEINTRESOURCE(IDS_FOLDERVIEWS),
                                    MB_YESNO | MB_ICONINFORMATION) == IDYES)
                {
                    pAdv->ppsx->ResetDefFolderSettings();
                    pAdv->ppsx->SetNeedRefresh(TRUE);
                    pAdv->fDirty = TRUE;
                }
            
                break;
            }
            break;

        case WM_HELP:                   // F1
        {
            LPHELPINFO lphelpinfo = (LPHELPINFO)lParam;

            //if the help is for one of the command buttons then call winhelp 
            if (lphelpinfo->iCtrlId != IDC_ADVO_ADVANCEDTREE)
            {
                WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, TEXT(SHELL_HLP),
                             HELP_WM_HELP, (DWORD_PTR)(LPSTR)aAdvOptsHelpIDs);
            }
            else 
            {
                //Help is for the tree item so we need to do some special processing
                
                HTREEITEM hItem;

                //Is this help invoked throught F1 key
                if (GetAsyncKeyState(VK_F1) < 0)                
                {
                    // Yes. WE need to give help for the currently selected item
                    hItem = TreeView_GetSelection(pAdv->hwndTree);
                }
                else 
                {
                    //No, We need to give help for the item at the cursor position 
                    TV_HITTESTINFO ht;
                    ht.pt =lphelpinfo->MousePos;
                    ScreenToClient(pAdv->hwndTree, &ht.pt); // Translate it to our window
                    hItem  = TreeView_HitTest(pAdv->hwndTree, &ht);
                }

                if (FAILED(pAdv->pTO->ShowHelp(hItem, HELP_WM_HELP)))
                {
                    WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, NULL,
                                HELP_WM_HELP, (DWORD_PTR)(LPSTR)aAdvOptsHelpIDs);
                }

            }
            break; 
        }

        case WM_CONTEXTMENU:        // right mouse click
        {
            HTREEITEM hti;

            // BUGBUG: even after picking off the keyboard context menu
            // key, winhelp is still putting the context menu where the
            // cursor is.  need to investigate...
            //
            if ((LPARAM)-1 == lParam)
            {
                hti = TreeView_GetSelection(pAdv->hwndTree);
            }
            else
            {
                TV_HITTESTINFO ht;

                ht.pt.x = GET_X_LPARAM(lParam);
                ht.pt.y = GET_Y_LPARAM(lParam);
                ScreenToClient(pAdv->hwndTree, &ht.pt);

                hti = TreeView_HitTest(pAdv->hwndTree, &ht);
            }

            // retrieve the item hit
            if (FAILED(pAdv->pTO->ShowHelp(hti, HELP_CONTEXTMENU)))
            {           
                WinHelp( (HWND) wParam, TEXT(SHELL_HLP),
                            HELP_CONTEXTMENU, (ULONG_PTR)(LPSTR)aAdvOptsHelpIDs);
            }
            break; 
        }

        case WM_DESTROY:
            // free the tree
            if (pAdv->pTO)
            {
                pAdv->pTO->WalkTree(WALK_TREE_DELETE);
                ATOMICRELEASE(pAdv->pTO);
            }

            // free local memory
            ASSERT(pAdv);
            LocalFree(pAdv);

            // make sure we don't re-enter
            SetWindowPtr( hDlg, DWLP_USER, NULL );
            break; // WM_DESTORY

        }

    }

    return FALSE; // not handled
}
