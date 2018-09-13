//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996                    **
//*********************************************************************

//
// ADVANCED.C - "Advanced" Property Sheet
//

// HISTORY:
//
// 6/22/96  t-gpease    created
// 5/27/97  t-ashlm     rewrote
//

#include "inetcplp.h"

#include <mluisupp.h>

//
// Private Calls and structures
//
TCHAR g_szUnderline[3][64];

// Reads a STRING and determines BOOL value: "yes" = TRUE | "no" = FALSE
BOOL RegGetBooleanString(HUSKEY huskey, LPTSTR RegValue, BOOL Value);

// Writes a STRING depending on the BOOL: TRUE = "yes" | FALSE = "no"
BOOL RegSetBooleanString(HUSKEY huskey, LPTSTR RegValue, BOOL Value);

// Reads a STRING of R,G,B values and returns a COLOREF.
COLORREF RegGetColorRefString( HUSKEY huskey, LPTSTR RegValue, COLORREF Value);

// Writes a STRING of R,G,B comma separated values.
COLORREF RegSetColorRefString( HUSKEY huskey, LPTSTR RegValue, COLORREF Value);

BOOL _AorW_GetFileNameFromBrowse(HWND hDlg,
                                 LPWSTR pszFilename,
                                 UINT cchFilename,
                                 LPCWSTR pszWorkingDir,
                                 LPCWSTR pszExt,
                                 LPCWSTR pszFilter,
                                 LPCWSTR pszTitle);

//
// Reg keys
//
#define REGSTR_PATH_ADVANCEDLIST REGSTR_PATH_IEXPLORER TEXT("\\AdvancedOptions")


typedef struct {
    HWND hDlg;              // handle of our dialog
    HWND hwndTree;          // handle to the treeview

    IRegTreeOptions *pTO;   // pointer to RegTreeOptions interface
    BOOL fChanged;
    BOOL fShowIEOnDesktop;
} ADVANCEDPAGE, *LPADVANCEDPAGE;


BOOL IsShowIEOnDesktopEnabled()
{
    HKEY hk;
    if (SUCCEEDED(SHRegGetCLSIDKey(CLSID_Internet, TEXT("ShellFolder"), TRUE, FALSE, &hk)))
    {
        DWORD dwValue = 0, cbSize = SIZEOF(dwValue);
        SHGetValueW(hk, NULL, TEXT("Attributes"), NULL, (BYTE *)&dwValue, &cbSize);
        RegCloseKey(hk);

        return (dwValue & SFGAO_NONENUMERATED) != SFGAO_NONENUMERATED;;
    }
    return TRUE;
}


STDAPI_(UINT) GetUIVersion()
{
    static UINT s_uiShell32 = 0;
    if (s_uiShell32 == 0)
    {
        HINSTANCE hinst = GetModuleHandle(TEXT("SHELL32.DLL"));
        if (hinst)
        {
            DLLGETVERSIONPROC pfnGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinst, "DllGetVersion");
            DLLVERSIONINFO dllinfo;

            dllinfo.cbSize = sizeof(DLLVERSIONINFO);
            if (pfnGetVersion && pfnGetVersion(&dllinfo) == NOERROR)
                s_uiShell32 = dllinfo.dwMajorVersion;
            else
                s_uiShell32 = 3;
        }
    }
    return s_uiShell32;
}

#define IE_DESKTOP_NAMESPACE_KEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\Namespace\\{FBF23B42-E3F0-101B-8488-00AA003E56F8}")

void ShowIEOnDesktop(BOOL fShow)
{
    switch (GetUIVersion())
    {
    case 3:
        //  win95 shell
        if (fShow)
        {
            TCHAR szTheInternet[MAX_PATH];

            int cch = MLLoadString(IDS_THE_INTERNET, szTheInternet, ARRAYSIZE(szTheInternet));
            SHSetValue(HKEY_LOCAL_MACHINE, IE_DESKTOP_NAMESPACE_KEY, NULL, REG_SZ, 
                       szTheInternet, (cch + 1) * sizeof(TCHAR));
        }
        else
        {
            SHDeleteKey(HKEY_LOCAL_MACHINE, IE_DESKTOP_NAMESPACE_KEY); 
        }
        break;

    case 4:
        //  IE4 integrated shell
        //  doesnt have peruser, so we need to just 
        //  delete it by marking it NONENUMERATED
        {
            HKEY hk;
            if (SUCCEEDED(SHRegGetCLSIDKey(CLSID_Internet, TEXT("ShellFolder"), FALSE, FALSE, &hk)))
            {
                DWORD dwValue = 0, cbSize = SIZEOF(dwValue);
                SHGetValue(hk, NULL, TEXT("Attributes"), NULL, (BYTE *)&dwValue, &cbSize);

                dwValue = (dwValue & ~SFGAO_NONENUMERATED) | (fShow ? 0 : SFGAO_NONENUMERATED);

                SHSetValueW(hk, NULL, TEXT("Attributes"), REG_DWORD, (BYTE *)&dwValue, SIZEOF(dwValue));
                RegCloseKey(hk);
            }
        }       
        break;

    default:
        //  do nothing since just changing the settings 
        //  in the right place sets up the PER-USER
        //  stuff correctly
        break;
    }
        
}

// AdvancedDlgInit()
//
// Initializes Advanced property sheet
//
// History:
//
// 6/13/96  t-gpease   created
// 5/27/96  t-ashlm    rewrote
//
BOOL AdvancedDlgInit(HWND hDlg)
{
    LPADVANCEDPAGE  pAdv;
    HTREEITEM htvi;
    HRESULT hr;

    pAdv = (LPADVANCEDPAGE)LocalAlloc(LPTR, sizeof(*pAdv));
    if (!pAdv)
    {
        EndDialog(hDlg, 0);
        return FALSE;   // no memory?
    }

    TraceMsg(TF_GENERAL, "\nInitializing Advanced Tab\n");

    pAdv->fShowIEOnDesktop = IsShowIEOnDesktopEnabled();

    InitCommonControls();

    // tell dialog where to get info
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pAdv);

    // save dialog handle
    pAdv->hDlg = hDlg;
    pAdv->hwndTree = GetDlgItem( pAdv->hDlg, IDC_ADVANCEDTREE );

    CoInitialize(0);
    hr = CoCreateInstance(CLSID_CRegTreeOptions, NULL, CLSCTX_INPROC_SERVER,
                          IID_IRegTreeOptions, (LPVOID *)&(pAdv->pTO));


    if (SUCCEEDED(hr))
    {
#ifdef UNICODE  // InitTree takes Ansi string
        char szRegPath[REGSTR_MAX_VALUE_LENGTH];
        SHTCharToAnsi(REGSTR_PATH_ADVANCEDLIST, szRegPath, ARRAYSIZE(szRegPath));
        hr = pAdv->pTO->InitTree(pAdv->hwndTree, HKEY_LOCAL_MACHINE, szRegPath, NULL);
#else
        hr = pAdv->pTO->InitTree(pAdv->hwndTree, HKEY_LOCAL_MACHINE, REGSTR_PATH_ADVANCEDLIST, NULL);
#endif
    }

        // find the first root and make sure that it is visible
    htvi = TreeView_GetRoot( pAdv->hwndTree );
    TreeView_EnsureVisible( pAdv->hwndTree, htvi );

    if (g_restrict.fAdvanced)
    {
        EnableDlgItem(hDlg, IDC_RESTORE_DEFAULT, FALSE);
    }

    return SUCCEEDED(hr) ? TRUE : FALSE;
}

#define REGKEY_DECLINED_IOD   TEXT("Software\\Microsoft\\Active Setup\\Declined Install On Demand IEv5.PP2")
#define REGKEY_DECLINED_COMPONENTS     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Declined Components IE5.pp2")
//
// AdvancedDlgOnCommand
//
// Handles Advanced property sheet window commands
//
// History:
// 6/13/96  t-gpease   created
// 5/27/97  t-ashlm    rewrote
//
void AdvancedDlgOnCommand(LPADVANCEDPAGE pAdv, UINT id, UINT nCmd)
{
    switch (id)
    {
        case IDC_RESTORE_DEFAULT:
            if (nCmd == BN_CLICKED)
            {
                // forget all Install On Demands that the user requested we
                // never ask again
                // Warning : if you ever have subkeys - these will fail on NT
                RegDeleteKey(HKEY_CURRENT_USER, REGKEY_DECLINED_IOD);
                // forget all code downloads that user said No to
                RegDeleteKey(HKEY_CURRENT_USER, REGKEY_DECLINED_COMPONENTS);

                pAdv->pTO->WalkTree(WALK_TREE_RESTORE);
                pAdv->fChanged = TRUE;
                ENABLEAPPLY(pAdv->hDlg);
            }
            break;
    }
}

//
// AdvancedDlgOnNotify()
//
// Handles Advanced property sheets WM_NOTIFY messages
//
// History:
//
// 6/13/96  t-gpease   created
//
void AdvancedDlgOnNotify(LPADVANCEDPAGE pAdv, LPNMHDR psn)
{
    SetWindowLongPtr( pAdv->hDlg, DWLP_MSGRESULT, (LONG_PTR)0); // handled

    switch (psn->code) {
        case TVN_KEYDOWN:
        {
            TV_KEYDOWN *pnm = (TV_KEYDOWN*)psn;
            if (pnm->wVKey == VK_SPACE)
            {
                if (!g_restrict.fAdvanced)
                {
                    pAdv->pTO->ToggleItem((HTREEITEM)SendMessage(pAdv->hwndTree, TVM_GETNEXTITEM, TVGN_CARET, NULL));
                    ENABLEAPPLY(pAdv->hDlg);
                    pAdv->fChanged = TRUE;

                    // Return TRUE so that the treeview swallows the space key.  Otherwise
                    // it tries to search for an element starting with a space and beeps.
                    SetWindowLongPtr(pAdv->hDlg, DWLP_MSGRESULT, TRUE);
                }
            }
            break;
        }

        case NM_CLICK:
        case NM_DBLCLK:
        {   // is this click in our tree?
            if ( psn->idFrom == IDC_ADVANCEDTREE )
            {   // yes...
                TV_HITTESTINFO ht;

                GetCursorPos( &ht.pt );                         // get where we were hit
                ScreenToClient( pAdv->hwndTree, &ht.pt );       // translate it to our window

                // retrieve the item hit
                if (!g_restrict.fAdvanced)
                {
                    pAdv->pTO->ToggleItem(TreeView_HitTest( pAdv->hwndTree, &ht));
                    ENABLEAPPLY(pAdv->hDlg);
                    pAdv->fChanged = TRUE;
                }
            }
        }
        break;

        case PSN_QUERYCANCEL:
        case PSN_KILLACTIVE:
        case PSN_RESET:
            SetWindowLongPtr(pAdv->hDlg, DWLP_MSGRESULT, FALSE);
            break;

        case PSN_APPLY:
        {
            if (pAdv->fChanged)
            {
                pAdv->pTO->WalkTree( WALK_TREE_SAVE );

                //  Now see if the user changed the "Show Internet Explorer on the Desktop"
                //  setting.
                if (pAdv->fShowIEOnDesktop != IsShowIEOnDesktopEnabled())
                {
                    pAdv->fShowIEOnDesktop = !pAdv->fShowIEOnDesktop;
                    
                    //  They did, so now see if it's integrated shell or not.
                    ShowIEOnDesktop(pAdv->fShowIEOnDesktop);

                    //  Now refresh the desktop
                    SHITEMID mkid = {0};
                    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, &mkid, NULL);
                }

                InternetSetOption( NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
                UpdateAllWindows();
                pAdv->fChanged = FALSE;
            }
        }
        break;

    }
}

//
// AdvancedDlgProc
//
// SubDialogs:
//    Temporary Internet Files (cache)
//
// History:
//
// 6/12/96    t-gpease    created
// 5/27/97    t-ashlm     rewrote
//
INT_PTR CALLBACK AdvancedDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPADVANCEDPAGE pAdv;

    if (uMsg == WM_INITDIALOG)
        return AdvancedDlgInit(hDlg);
    else
         pAdv = (LPADVANCEDPAGE)GetWindowLongPtr(hDlg, DWLP_USER);

    if (!pAdv)
        return FALSE;

    switch (uMsg)
    {

        case WM_NOTIFY:
            AdvancedDlgOnNotify(pAdv, (LPNMHDR)lParam);
            return TRUE;
            break;

        case WM_COMMAND:
            AdvancedDlgOnCommand(pAdv, LOWORD(wParam), HIWORD(wParam));
            return TRUE;
            break;

        case WM_HELP:                   // F1
        {
            LPHELPINFO lphelpinfo;
            lphelpinfo = (LPHELPINFO)lParam;

            if (lphelpinfo->iCtrlId != IDC_ADVANCEDTREE)
            {
                ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                             HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);

            }
            else
            {
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
                    ht.pt =((LPHELPINFO)lParam)->MousePos;
                    ScreenToClient(pAdv->hwndTree, &ht.pt); // Translate it to our window
                    hItem = TreeView_HitTest(pAdv->hwndTree, &ht);
                }

                if (FAILED(pAdv->pTO->ShowHelp(hItem, HELP_WM_HELP)))
                {
                    ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                                HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
                }
            }
            break;
        }

        case WM_CONTEXTMENU:        // right mouse click
        {
            TV_HITTESTINFO ht;

            GetCursorPos( &ht.pt );                         // get where we were hit
            ScreenToClient( pAdv->hwndTree, &ht.pt );       // translate it to our window

            // retrieve the item hit
            if (FAILED(pAdv->pTO->ShowHelp(TreeView_HitTest( pAdv->hwndTree, &ht),HELP_CONTEXTMENU)))
            {
                ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                            HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            }
            break;
        }
        case WM_DESTROY:
            // destroying this deliberately flushes its update (see WM_DESTROY in the UpdateWndProc);
#ifndef UNIX
            // Should only be destroyed in Process Detach.
            if (g_hwndUpdate)
                DestroyWindow(g_hwndUpdate);
#endif

            // free the tree
            if (pAdv->pTO)
            {
                pAdv->pTO->WalkTree( WALK_TREE_DELETE );
                pAdv->pTO->Release();
                pAdv->pTO=NULL;
            }

            // free local memory
            ASSERT(pAdv);
            LocalFree(pAdv);

            // make sure we don't re-enter
            SetWindowLongPtr( hDlg, DWLP_USER, (LONG)NULL );
            CoUninitialize();
            break;

    } // switch

    return FALSE; // not handled

} // AdvancedDlgProc


//////////////////////////////////////////////
//
// Buttons on bottom
//
////////////////////////////////////////////////////////////////////////////////////////////

typedef struct tagCOLORSINFO {
    HWND     hDlg;
    BOOL     fUseWindowsDefaults;
    COLORREF colorWindowText;
    COLORREF colorWindowBackground;
    COLORREF colorLinkViewed;
    COLORREF colorLinkNotViewed;
    COLORREF colorLinkHover;
    BOOL     fUseHoverColor;
} COLORSINFO, *LPCOLORSINFO;

VOID Color_DrawButton(HWND hDlg, LPDRAWITEMSTRUCT lpdis, COLORREF the_color )
{
    SIZE thin   = { GetSystemMetrics(SM_CXBORDER), GetSystemMetrics(SM_CYBORDER) };
    RECT rc     = lpdis->rcItem;
    HDC hdc     = lpdis->hDC;
    BOOL bFocus = ((lpdis->itemState & ODS_FOCUS) && !(lpdis->itemState & ODS_DISABLED));

    if (!thin.cx) thin.cx = 1;
    if (!thin.cy) thin.cy = 1;

    FillRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));

    // Draw any caption
    TCHAR szCaption[80];
    int cxButton = 23*(rc.bottom - rc.top)/12;

    if (GetWindowText(lpdis->hwndItem, szCaption, ARRAYSIZE(szCaption)))
    {
        COLORREF crText;

        RECT rcText = rc;
        rcText.right -= cxButton;

        int nOldMode = SetBkMode(hdc, TRANSPARENT);

        if (lpdis->itemState & ODS_DISABLED)
        {
            // Draw disabled text using the embossed look
            crText = SetTextColor(hdc, GetSysColor(COLOR_BTNHIGHLIGHT));
            RECT rcOffset = rcText;
            OffsetRect(&rcOffset, 1, 1);
            DrawText(hdc, szCaption, -1, &rcOffset, DT_VCENTER|DT_SINGLELINE);
            SetTextColor(hdc, GetSysColor(COLOR_BTNSHADOW));
        }
        else
        {
            crText = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        }
        DrawText(hdc, szCaption, -1, &rcText, DT_VCENTER|DT_SINGLELINE);
        SetTextColor(hdc, crText);
        SetBkMode(hdc, nOldMode);
    }
    
    // Draw the button portion
    rc.left = rc.right - cxButton;

    if (lpdis->itemState & ODS_SELECTED)
    {
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
        OffsetRect(&rc, 1, 1);
    }
    else
    {
        DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT | BF_ADJUST);
    }

    if (bFocus)
    {
        InflateRect(&rc, -thin.cx, -thin.cy);
        DrawFocusRect(hdc, &rc);
        InflateRect(&rc, thin.cx, thin.cy);
    }

    // color sample
    if ( !(lpdis->itemState & ODS_DISABLED) )
    {
        HBRUSH hBrush;

        InflateRect(&rc, -2 * thin.cx, -2 * thin.cy);
        FrameRect(hdc, &rc, GetSysColorBrush(COLOR_BTNTEXT));
        InflateRect(&rc, -thin.cx, -thin.cy);

        hBrush = CreateSolidBrush( the_color );
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
    }
}

COLORREF g_CustomColors[16] = { 0 };

// ChooseColorW is yet implemented in comdlg32.dll
BOOL UseColorPicker( HWND hWnd,  COLORREF *the_color, int extra_flags )
{
    // Make a local copy of the custom colors so they are not saved if the 
    // color picker dialog is cancelled
    COLORREF customColors[16];
    memcpy(customColors, g_CustomColors, sizeof(customColors));

    CHOOSECOLORA cc;

    cc.lStructSize      = sizeof(cc);
    cc.hwndOwner        = hWnd;
    cc.hInstance        = NULL;
    cc.rgbResult        = (DWORD) *the_color;
    cc.lpCustColors     = customColors;
    cc.Flags            = CC_RGBINIT | extra_flags;
    cc.lCustData        = (DWORD) NULL;
    cc.lpfnHook         = NULL;
    cc.lpTemplateName   = NULL;

    if (ChooseColorA(&cc))
    {
        *the_color = cc.rgbResult;
        memcpy(g_CustomColors, customColors, sizeof(g_CustomColors));

        InvalidateRect( hWnd, NULL, FALSE );
        return TRUE;
    }
    TraceMsg(TF_GENERAL, "\nChooseColor() return 0\n");

    return FALSE;
}


VOID AppearanceDimFields( HWND hDlg )
{
    // reverse the function of the check.... if CHECKED turn off the color
    // selectors.
    BOOL setting = !IsDlgButtonChecked( hDlg, IDC_GENERAL_APPEARANCE_USE_CUSTOM_COLORS_CHECKBOX ) && !g_restrict.fColors;

    EnableWindow(GetDlgItem(hDlg, IDC_GENERAL_APPEARANCE_COLOR_TEXT), setting);
    EnableWindow(GetDlgItem(hDlg, IDC_GENERAL_APPEARANCE_COLOR_TEXT_LABEL), setting);
    EnableWindow(GetDlgItem(hDlg, IDC_GENERAL_APPEARANCE_COLOR_BACKGROUND), setting);
    EnableWindow(GetDlgItem(hDlg, IDC_GENERAL_APPEARANCE_COLOR_BACKGROUND_LABEL), setting);
    EnableWindow(GetDlgItem(hDlg, IDC_GENERAL_APPEARANCE_COLOR_HOVER),
                 IsDlgButtonChecked(hDlg, IDC_GENERAL_APPEARANCE_USE_HOVER_COLOR_CHECKBOX) && !g_restrict.fLinks);
}

BOOL General_DrawItem(HWND hDlg, WPARAM wParam, LPARAM lParam, LPCOLORSINFO pci)
{
    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
        case IDC_GENERAL_APPEARANCE_COLOR_TEXT:
            Color_DrawButton(hDlg, (LPDRAWITEMSTRUCT)lParam, pci->colorWindowText);
            return TRUE;

        case IDC_GENERAL_APPEARANCE_COLOR_BACKGROUND:
            Color_DrawButton(hDlg, (LPDRAWITEMSTRUCT)lParam, pci->colorWindowBackground);
            return TRUE;

        case IDC_GENERAL_APPEARANCE_COLOR_LINKS:
            Color_DrawButton(hDlg, (LPDRAWITEMSTRUCT)lParam, pci->colorLinkNotViewed);
            return TRUE;

        case IDC_GENERAL_APPEARANCE_COLOR_VISITED_LINKS:
            Color_DrawButton(hDlg, (LPDRAWITEMSTRUCT)lParam, pci->colorLinkViewed);
            return TRUE;

        case IDC_GENERAL_APPEARANCE_COLOR_HOVER:
            Color_DrawButton(hDlg, (LPDRAWITEMSTRUCT)lParam, pci->colorLinkHover);
            return TRUE;
    }
    return FALSE;
}


INT_PTR CALLBACK ColorsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    LPCOLORSINFO pci = (LPCOLORSINFO) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            DWORD cb = sizeof(DWORD);
            HUSKEY huskey;

            pci = (LPCOLORSINFO)LocalAlloc(LPTR, sizeof(COLORSINFO));
            if (!pci)
            {
                EndDialog(hDlg, IDCANCEL);
                return FALSE;
            }

            // tell dialog where to get info
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pci);

            // save the handle to the page
            pci->hDlg = hDlg;

            // set default values
            pci->fUseWindowsDefaults       = TRUE;
            pci->colorWindowText           = RGB(0,0,0);
            pci->colorWindowBackground     = RGB(192,192,192);
            pci->colorLinkViewed           = RGB(0, 128, 128);
            pci->colorLinkNotViewed        = RGB(0, 0, 255);
            pci->colorLinkHover            = RGB(255, 0, 0);
            pci->fUseHoverColor            = TRUE;

            if (SHRegOpenUSKey(REGSTR_PATH_IEXPLORER,
                               KEY_READ|KEY_WRITE,    // samDesired
                               NULL,    // hUSKeyRelative
                               &huskey,
                               FALSE) == ERROR_SUCCESS)
            {
                HUSKEY huskeySub;

                if (SHRegOpenUSKey(REGSTR_KEY_MAIN,
                                   KEY_READ|KEY_WRITE,
                                   huskey,
                                   &huskeySub,
                                   FALSE) == ERROR_SUCCESS)
                {
                    pci->fUseWindowsDefaults       = RegGetBooleanString(huskeySub,
                        REGSTR_VAL_USEDLGCOLORS, pci->fUseWindowsDefaults);

                    SHRegCloseUSKey(huskeySub);
                }

                if (SHRegOpenUSKey(REGSTR_KEY_IE_SETTINGS,
                                   KEY_READ|KEY_WRITE,
                                   huskey,
                                   &huskeySub,
                                   FALSE) == ERROR_SUCCESS)
                {
                    pci->colorWindowText           = RegGetColorRefString(huskeySub,
                        REGSTR_VAL_TEXTCOLOR, pci->colorWindowText);

                    pci->colorWindowBackground     = RegGetColorRefString(huskeySub,
                        REGSTR_VAL_BACKGROUNDCOLOR, pci->colorWindowBackground);

                    pci->colorLinkViewed           = RegGetColorRefString(huskeySub,
                        REGSTR_VAL_ANCHORCOLORVISITED, pci->colorLinkViewed);

                    pci->colorLinkNotViewed        = RegGetColorRefString(huskeySub,
                        REGSTR_VAL_ANCHORCOLOR, pci->colorLinkNotViewed);

                    pci->colorLinkHover            = RegGetColorRefString(huskeySub,
                        REGSTR_VAL_ANCHORCOLORHOVER, pci->colorLinkHover);

                    pci->fUseHoverColor            = RegGetBooleanString(huskeySub,
                        REGSTR_VAL_USEHOVERCOLOR, pci->fUseHoverColor);

                    SHRegCloseUSKey(huskeySub);
                }
                SHRegCloseUSKey(huskey);
            }

            cb = sizeof(g_CustomColors);
            SHRegGetUSValue(REGSTR_PATH_IE_SETTINGS, REGSTR_VAL_IE_CUSTOMCOLORS, NULL, (LPBYTE)&g_CustomColors,
                            &cb, FALSE, NULL, NULL);

            //
            // select appropriate dropdown item here for underline links
            //

            CheckDlgButton(hDlg, IDC_GENERAL_APPEARANCE_USE_CUSTOM_COLORS_CHECKBOX, pci->fUseWindowsDefaults);
            CheckDlgButton(hDlg, IDC_GENERAL_APPEARANCE_USE_HOVER_COLOR_CHECKBOX, pci->fUseHoverColor);

            AppearanceDimFields(hDlg);

            if (g_restrict.fLinks)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_GENERAL_APPEARANCE_COLOR_LINKS), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_GENERAL_APPEARANCE_COLOR_VISITED_LINKS), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_GENERAL_APPEARANCE_USE_HOVER_COLOR_CHECKBOX), FALSE);
            }

            if (g_restrict.fColors)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_GENERAL_APPEARANCE_COLOR_TEXT), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_GENERAL_APPEARANCE_COLOR_BACKGROUND), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_GENERAL_APPEARANCE_USE_CUSTOM_COLORS_CHECKBOX), FALSE);
            }

            return TRUE;
        }

        case WM_DRAWITEM:
            return General_DrawItem(hDlg, wParam, lParam, pci);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    HUSKEY huskey;
                    if (SHRegOpenUSKey(REGSTR_PATH_IEXPLORER,
                                       KEY_WRITE,    // samDesired
                                       NULL,    // hUSKeyRelative
                                       &huskey,
                                       FALSE) == ERROR_SUCCESS)
                    {
                        HUSKEY huskeySub;

                        if (SHRegOpenUSKey(REGSTR_KEY_MAIN,
                                           KEY_WRITE,
                                           huskey,
                                           &huskeySub,
                                           FALSE) == ERROR_SUCCESS)
                        {
                            pci->fUseWindowsDefaults = RegSetBooleanString(huskeySub,
                                REGSTR_VAL_USEDLGCOLORS, pci->fUseWindowsDefaults);

                            SHRegCloseUSKey(huskeySub);
                        }

                        if (SHRegOpenUSKey(REGSTR_KEY_IE_SETTINGS,
                                           KEY_WRITE,
                                           huskey,
                                           &huskeySub,
                                           FALSE) == ERROR_SUCCESS)
                        {
                            pci->colorWindowText           = RegSetColorRefString(huskeySub,
                                REGSTR_VAL_TEXTCOLOR, pci->colorWindowText);

                            pci->colorWindowBackground     = RegSetColorRefString(huskeySub,
                                REGSTR_VAL_BACKGROUNDCOLOR, pci->colorWindowBackground);

                            pci->colorLinkViewed           = RegSetColorRefString(huskeySub,
                                REGSTR_VAL_ANCHORCOLORVISITED, pci->colorLinkViewed);

                            pci->colorLinkNotViewed        = RegSetColorRefString(huskeySub,
                                REGSTR_VAL_ANCHORCOLOR, pci->colorLinkNotViewed);

                            pci->colorLinkHover            = RegSetColorRefString(huskeySub,
                                REGSTR_VAL_ANCHORCOLORHOVER, pci->colorLinkHover);

                            pci->fUseHoverColor            = RegSetBooleanString(huskeySub,
                                REGSTR_VAL_USEHOVERCOLOR, pci->fUseHoverColor);

                            SHRegCloseUSKey(huskeySub);
                        }
                        SHRegCloseUSKey(huskey);
                    }


                    // custom colors
                    SHRegSetUSValue(REGSTR_PATH_IE_SETTINGS, REGSTR_VAL_IE_CUSTOMCOLORS, REGSTR_VAL_IE_CUSTOMCOLORS_TYPE, (LPBYTE)&g_CustomColors,
                                    sizeof(g_CustomColors), SHREGSET_FORCE_HKCU);

                    // refresh the browser
                    UpdateAllWindows();

                    EndDialog(hDlg, IDOK);
                    break;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;

                case IDC_GENERAL_APPEARANCE_USE_CUSTOM_COLORS_CHECKBOX:
                    if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                    {
                        pci->fUseWindowsDefaults =
                            IsDlgButtonChecked(hDlg, IDC_GENERAL_APPEARANCE_USE_CUSTOM_COLORS_CHECKBOX);
                        AppearanceDimFields(hDlg);
                    }
                    break;


                case IDC_GENERAL_APPEARANCE_USE_HOVER_COLOR_CHECKBOX:
                    if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                    {
                        pci->fUseHoverColor =
                            IsDlgButtonChecked(hDlg, IDC_GENERAL_APPEARANCE_USE_HOVER_COLOR_CHECKBOX);
                        AppearanceDimFields(hDlg);
                    }
                    break;

                case IDC_GENERAL_APPEARANCE_COLOR_TEXT:
                    if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                    {
                        UseColorPicker( hDlg, &pci->colorWindowText, CC_SOLIDCOLOR);
                    }
                    break;

                case IDC_GENERAL_APPEARANCE_COLOR_BACKGROUND:
                    if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                    {
                        UseColorPicker( hDlg, &pci->colorWindowBackground, CC_SOLIDCOLOR);
                    }
                    break;

                case IDC_GENERAL_APPEARANCE_COLOR_LINKS:
                    if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                    {
                        UseColorPicker( hDlg, &pci->colorLinkNotViewed, CC_SOLIDCOLOR);
                    }
                    break;

                case IDC_GENERAL_APPEARANCE_COLOR_VISITED_LINKS:
                    if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                    {
                        UseColorPicker( hDlg, &pci->colorLinkViewed, CC_SOLIDCOLOR);
                    }
                    break;

                case IDC_GENERAL_APPEARANCE_COLOR_HOVER:
                    if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                    {
                        UseColorPicker( hDlg, &pci->colorLinkHover, CC_SOLIDCOLOR);
                    }
                    break;

                default:
                    return FALSE;
            }
            return TRUE;
            break;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
            ASSERT(pci);
            if (pci)
            {
                LocalFree(pci);
            }

            break;
    }
    return FALSE;
}

typedef struct tagACCESSIBILITYINFO
{
    HWND hDlg;
    BOOL fMyColors;
    BOOL fMyFontStyle;
    BOOL fMyFontSize;
    BOOL fMyStyleSheet;
    TCHAR szStyleSheetPath[MAX_PATH];
} ACCESSIBILITYINFO, *LPACCESSIBILITYINFO;

INT_PTR CALLBACK AccessibilityDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    LPACCESSIBILITYINFO pai = (LPACCESSIBILITYINFO) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HKEY hkey;
            DWORD cb;

            pai = (LPACCESSIBILITYINFO)LocalAlloc(LPTR, sizeof(ACCESSIBILITYINFO));
            if (!pai)
            {
                EndDialog(hDlg, IDCANCEL);
                return FALSE;
            }

            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pai);
            pai->hDlg = hDlg;

            if (RegCreateKeyEx(HKEY_CURRENT_USER,
                               TEXT("Software\\Microsoft\\Internet Explorer\\Settings"),
                               0, NULL, 0,
                               KEY_READ,
                               NULL,
                               &hkey,
                               NULL) == ERROR_SUCCESS)
            {

                cb = sizeof(pai->fMyColors);
                RegQueryValueEx(hkey, TEXT("Always Use My Colors"), NULL, NULL, (LPBYTE)&(pai->fMyColors), &cb);

                cb = sizeof(pai->fMyFontStyle);
                RegQueryValueEx(hkey, TEXT("Always Use My Font Face"), NULL, NULL, (LPBYTE)&(pai->fMyFontStyle),&cb);

                cb = sizeof(pai->fMyFontSize);
                RegQueryValueEx(hkey, TEXT("Always Use My Font Size"), NULL, NULL, (LPBYTE)&(pai->fMyFontSize),&cb);

                RegCloseKey(hkey);

            }
            if (RegCreateKeyEx(HKEY_CURRENT_USER,
                             TEXT("Software\\Microsoft\\Internet Explorer\\Styles"),
                             0, NULL, 0,
                             KEY_READ,
                             NULL,
                             &hkey,
                             NULL) == ERROR_SUCCESS)
            {
                cb = sizeof(pai->fMyStyleSheet);
                RegQueryValueEx(hkey, TEXT("Use My Stylesheet"), NULL, NULL, (LPBYTE)&(pai->fMyStyleSheet),&cb);

                cb = sizeof(pai->szStyleSheetPath);
                RegQueryValueEx(hkey, TEXT("User Stylesheet"), NULL, NULL, (LPBYTE)&(pai->szStyleSheetPath), &cb);
                RegCloseKey(hkey);
            }

            CheckDlgButton(hDlg, IDC_CHECK_COLOR, pai->fMyColors);
            CheckDlgButton(hDlg, IDC_CHECK_FONT_STYLE, pai->fMyFontStyle);
            CheckDlgButton(hDlg, IDC_CHECK_FONT_SIZE, pai->fMyFontSize);
            CheckDlgButton(hDlg, IDC_CHECK_USE_MY_STYLESHEET, pai->fMyStyleSheet);
            SetDlgItemText(hDlg, IDC_EDIT_STYLESHEET, pai->szStyleSheetPath);
            SHAutoComplete(GetDlgItem(hDlg, IDC_EDIT_STYLESHEET), SHACF_DEFAULT);

            if (!pai->fMyStyleSheet || g_restrict.fAccessibility)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_STATIC_STYLESHEET), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_STYLESHEET), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_STYLESHEET_BROWSE), FALSE);
            }

            if (g_restrict.fAccessibility)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_COLOR), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_FONT_STYLE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_FONT_SIZE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_USE_MY_STYLESHEET), FALSE);
            }
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    HKEY hkey;

                    GetDlgItemText(hDlg, IDC_EDIT_STYLESHEET, pai->szStyleSheetPath, sizeof(pai->szStyleSheetPath));
                    if (!PathFileExists(pai->szStyleSheetPath) && IsDlgButtonChecked(hDlg, IDC_CHECK_USE_MY_STYLESHEET))
                    {
                        MLShellMessageBox(hDlg, MAKEINTRESOURCEW(IDS_FILENOTFOUND), NULL, MB_ICONHAND|MB_OK);
                        break;
                    }

                    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Settings"),0, KEY_WRITE, &hkey) == ERROR_SUCCESS)
                    {
                        DWORD cb;

                        cb = sizeof(pai->fMyColors);
                        pai->fMyColors = IsDlgButtonChecked(hDlg, IDC_CHECK_COLOR);
                        RegSetValueEx(hkey, TEXT("Always Use My Colors"), NULL, REG_DWORD, (LPBYTE)&(pai->fMyColors), cb);

                        cb = sizeof(pai->fMyFontStyle);
                        pai->fMyFontStyle = IsDlgButtonChecked(hDlg, IDC_CHECK_FONT_STYLE);
                        RegSetValueEx(hkey, TEXT("Always Use My Font Face"), NULL, REG_DWORD, (LPBYTE)&(pai->fMyFontStyle), cb);

                        cb = sizeof(pai->fMyFontSize);
                        pai->fMyFontSize = IsDlgButtonChecked(hDlg, IDC_CHECK_FONT_SIZE);
                        RegSetValueEx(hkey, TEXT("Always Use My Font Size"), NULL, REG_DWORD, (LPBYTE)&(pai->fMyFontSize),cb);

                        RegCloseKey(hkey);
                    }

                    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Styles"), 0, KEY_WRITE, &hkey) == ERROR_SUCCESS)
                    {
                        DWORD cb;

#ifndef UNIX
                        cb = sizeof(pai->szStyleSheetPath);
#else
                        // We don't know if this is exactly what we need to do, so we ifdef it.
                        cb = (_tcslen(pai->szStyleSheetPath) + 1) * sizeof(TCHAR);
#endif
                        RegSetValueEx(hkey, TEXT("User Stylesheet"), NULL, REG_SZ, (LPBYTE)&(pai->szStyleSheetPath),cb);

                        cb = sizeof(pai->fMyStyleSheet);
                        pai->fMyStyleSheet = IsDlgButtonChecked(hDlg, IDC_CHECK_USE_MY_STYLESHEET);
                        RegSetValueEx(hkey, TEXT("Use My Stylesheet"), NULL, REG_DWORD, (LPBYTE)&(pai->fMyStyleSheet),cb);

                        RegCloseKey(hkey);
                    }

                    UpdateAllWindows();     // refresh the browser

                    EndDialog(hDlg, IDOK);
                    break;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;

                case IDC_CHECK_USE_MY_STYLESHEET:
                {
                    DWORD fChecked;

                    fChecked = IsDlgButtonChecked(hDlg, IDC_CHECK_USE_MY_STYLESHEET);
                    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_STYLESHEET), fChecked);
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_STYLESHEET), fChecked);
                    EnableWindow(GetDlgItem(hDlg, IDC_STYLESHEET_BROWSE), fChecked);
                    EnableWindow(GetDlgItem(hDlg,IDOK), IsDlgButtonChecked(hDlg, IDC_CHECK_USE_MY_STYLESHEET) ? (GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_STYLESHEET)) ? TRUE:FALSE) : TRUE);
                    break;
                }

                case IDC_EDIT_STYLESHEET:
                    switch(HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            EnableWindow(GetDlgItem(hDlg,IDOK), IsDlgButtonChecked(hDlg, IDC_CHECK_USE_MY_STYLESHEET) ? (GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_STYLESHEET)) ? TRUE:FALSE) : TRUE);
                            break;
                    }
                    break;


                case IDC_STYLESHEET_BROWSE:
                {
                    TCHAR szFilenameBrowse[MAX_PATH];
                    int ret;
                    TCHAR szExt[MAX_PATH];
                    TCHAR szFilter[MAX_PATH];

                    szFilenameBrowse[0] = 0;
                    // BUGBUG why is IDS_STYLESHEET_EXT in shdoclc.rc?
                    MLLoadString(IDS_STYLESHEET_EXT, szExt, ARRAYSIZE(szExt));
                    int cchFilter = MLLoadShellLangString(IDS_STYLESHEET_FILTER, szFilter, ARRAYSIZE(szFilter)-1);

                    // Make sure we have a double null termination on the filter
                    szFilter[cchFilter + 1] = 0;

                    ret = _AorW_GetFileNameFromBrowse(hDlg, szFilenameBrowse, ARRAYSIZE(szFilenameBrowse), NULL, szExt,
                        szFilter, NULL);

                    if (ret > 0)
                    {
                        SetDlgItemText(hDlg, IDC_EDIT_STYLESHEET, szFilenameBrowse);
                    }
                    break;
                }

                default:
                    return FALSE;
            }
            return TRUE;
            break;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
            ASSERT(pai);
            if (pai)
            {
                LocalFree(pai);
            }
            break;
    }
    return FALSE;
}


#define TEMP_SMALL_BUF_SZ  256
inline BOOL IsNotResource(LPCWSTR pszItem)
{
    return (HIWORD(pszItem) != 0);
}

BOOL WINAPI _AorW_GetFileNameFromBrowse
(
    HWND hwnd,
    LPWSTR pszFilePath,     // IN OUT
    UINT cchFilePath,
    LPCWSTR pszWorkingDir,  //IN OPTIONAL
    LPCWSTR pszDefExt,      //IN OPTIONAL
    LPCWSTR pszFilters,     //IN OPTIONAL
    LPCWSTR pszTitle        //IN OPTIONAL
)
{
    BOOL bResult;

#ifndef UNIX
    // Determine which version of NT or Windows we're running on
    OSVERSIONINFOA osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExA(&osvi);

    BOOL fRunningOnNT = (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId);

    if (fRunningOnNT)
    {
#endif
        bResult = GetFileNameFromBrowse(hwnd,
                                    pszFilePath,
                                    cchFilePath,
                                    pszWorkingDir,
                                    pszDefExt,
                                    pszFilters,
                                    pszTitle);
#ifndef UNIX
    }
    else
    {
        // Thunk to ansi
        CHAR szFilters[TEMP_SMALL_BUF_SZ*2];
        CHAR szPath[MAX_PATH];
        CHAR szDir[MAX_PATH];
        CHAR szExt[TEMP_SMALL_BUF_SZ];
        CHAR szTitle[TEMP_SMALL_BUF_SZ];
 
        // Always move pszFilePath stuff to szPath buffer. Should never be a resourceid.
        SHUnicodeToAnsi(pszFilePath, szPath, ARRAYSIZE(szPath));

        if (IsNotResource(pszWorkingDir)) 
        {
            SHUnicodeToAnsi(pszWorkingDir, szDir, ARRAYSIZE(szDir));
            pszWorkingDir = (LPCWSTR)szDir;
        }
        if (IsNotResource(pszDefExt))
        {
            SHUnicodeToAnsi(pszDefExt, szExt, ARRAYSIZE(szExt));
            pszDefExt = (LPCWSTR)szExt;
        }
        if (IsNotResource(pszFilters))
        {
            int nIndex = 1;

            // Find the double terminator
            while (pszFilters[nIndex] || pszFilters[nIndex-1])
                nIndex++;

            // nIndex+1 looks like bunk unless it goes past the terminator
            WideCharToMultiByte(CP_ACP, 0, (LPCTSTR)pszFilters, nIndex+1, szFilters, ARRAYSIZE(szFilters), NULL, NULL);
            pszFilters = (LPCWSTR)szFilters;
        }
        if (IsNotResource(pszTitle))
        {
            SHUnicodeToAnsi(pszTitle, szTitle, ARRAYSIZE(szTitle));
            pszTitle = (LPCWSTR)szTitle;
        }

        bResult = GetFileNameFromBrowse(hwnd, (LPWSTR)szPath, ARRAYSIZE(szPath), pszWorkingDir, pszDefExt, pszFilters, pszTitle);

        SHAnsiToUnicode(szPath, pszFilePath, cchFilePath);
    }
#endif

    return bResult;
}
