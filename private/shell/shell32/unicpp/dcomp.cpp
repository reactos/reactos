#include "stdafx.h"
#pragma hdrstop

#define _BROWSEUI_      // Make functions exported from browseui as stdapi (as they are delay loaded)
#include "iethread.h"
#include "browseui.h"

#ifdef POSTSPLIT

static void EmptyListview(HWND hwndLV);

#define DXA_GROWTH_CONST 10
 
#define COMP_CHECKED    0x00002000
#define COMP_UNCHECKED  0x00001000

#define GALRET_NO       0x00000001
#define GALRET_NEVER    0x00000002

#define THISCLASS CCompPropSheetPage

#define c_szHelpFile    TEXT("Display.hlp")
const static DWORD aCompHelpIDs[] = {  // Context Help IDs
    IDC_COMP_ENABLEAD,  IDH_DISPLAY_WEB_SHOWWEB_CHECKBOX,
    IDC_COMP_LIST,      IDH_DISPLAY_WEB_ACTIVEDESKTOP_LIST,
    IDC_COMP_NEW,       IDH_DISPLAY_WEB_NEW_BUTTON,
    IDC_COMP_DELETE,    IDH_DISPLAY_WEB_DELETE_BUTTON,
    IDC_COMP_PROPERTIES,IDH_DISPLAY_WEB_PROPERTIES_BUTTON,
    IDC_COMP_PREVIEW,   IDH_DISPLAY_WEB_GRAPHIC,
    0, 0
};

typedef struct
{
    WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
    SUBSCRIPTIONINFO si;
} BACKUPSUBSCRIPTION;

THISCLASS::CCompPropSheetPage(void) : _iPreviousSelection(-1)
{
    dwSize = sizeof(THISCLASS);
    dwFlags = PSP_DEFAULT | PSP_USECALLBACK;
    hInstance = HINST_THISDLL;
    pszTemplate = MAKEINTRESOURCE(IDD_COMPONENTS);
    // hIcon = NULL; // unused (PSP_USEICON is not set)
    // pszTitle = NULL; // unused (PSP_USETITLE is not set)
    pfnDlgProc = _DlgProc;
    // lParam   = 0;     // unused
    pfnCallback = NULL;
    // pcRefParent = NULL;
}

void THISCLASS::_ConstructLVString(COMPONENTA *pcomp, LPTSTR pszBuf, DWORD cchBuf)
{
    //
    // Use the friendly name if it exists.
    // Otherwise use the source name.
    //
    if (pcomp->szFriendlyName[0])
    {
        lstrcpyn(pszBuf, pcomp->szFriendlyName, cchBuf);
    }
    else
    {
        lstrcpyn(pszBuf, pcomp->szSource, cchBuf);
    }
}

void THISCLASS::_AddComponentToLV(COMPONENTA *pcomp)
{
    TCHAR szBuf[INTERNET_MAX_URL_LENGTH + 40];
    _ConstructLVString(pcomp, szBuf, ARRAYSIZE(szBuf));

    //
    // Construct the listview item.
    //
    LV_ITEM lvi = {0};
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = 0x7FFFFFFF;
    lvi.pszText = szBuf;
    lvi.lParam = pcomp->dwID;

    int index = ListView_InsertItem(_hwndLV, &lvi);
    if (index != -1)
    {
        ListView_SetItemState(_hwndLV, index, pcomp->fChecked ? COMP_CHECKED : COMP_UNCHECKED, LVIS_STATEIMAGEMASK);
        ListView_SetColumnWidth(_hwndLV, 0, LVSCW_AUTOSIZE);
    }
}

void THISCLASS::_SetUIFromDeskState(BOOL fEmpty)
{
    //
    // Disable redraws while we mess repeatedly with the listview contents.
    //
    SendMessage(_hwndLV, WM_SETREDRAW, FALSE, 0);

    if (fEmpty)
    {
        EmptyListview(_hwndLV);
    }

    //
    // Add each component to the listview.
    //
    int cComp;
    g_pActiveDesk->GetDesktopItemCount(&cComp, 0);
    for (int i=0; i<cComp; i++)
    {
        COMPONENT comp;
        comp.dwSize = SIZEOF(comp);

        if (SUCCEEDED(g_pActiveDesk->GetDesktopItem(i, &comp, 0)))
        {
            COMPONENTA compA;
            compA.dwSize = sizeof(compA);
            WideCompToMultiComp(&comp, &compA);
            _AddComponentToLV(&compA);
        }
    }
    // Put checkboxes in correct state.
    //
    COMPONENTSOPT co;
    co.dwSize = sizeof(COMPONENTSOPT);
    g_pActiveDesk->GetDesktopItemOptions(&co, 0);
    CheckDlgButton(_hwnd, IDC_COMP_ENABLEAD, co.fActiveDesktop ? BST_CHECKED : BST_UNCHECKED);
    //If active desktop is forced on, disable this control!
    EnableWindow(GetDlgItem(_hwnd, IDC_COMP_ENABLEAD), !_fForceAD);
    
    //
    // Reenable redraws.
    //
    SendMessage(_hwndLV, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(_hwndLV, NULL, TRUE);
    InvalidateRect(GetDlgItem(_hwnd, IDC_COMP_PREVIEW), NULL, FALSE);
}

void THISCLASS::_EnableControls(void)
{
    COMPONENTSOPT co;
    co.dwSize = sizeof(COMPONENTSOPT);
    g_pActiveDesk->GetDesktopItemOptions(&co, 0);

    if(co.fActiveDesktop)
    {
        BOOL fEnable;
        COMPONENT comp = { SIZEOF(comp) };
        BOOL fHaveSelection = FALSE;
        BOOL fSpecialComp = FALSE;  //Is this a special component that can't be deleted?
        LPTSTR  pszSource = NULL;
#ifndef UNICODE
        TCHAR   szCompSource[INTERNET_MAX_URL_LENGTH];
#endif //UNICODE

        //
        // Read in the information about the selected component (if any).
        //
        int iIndex = ListView_GetNextItem(_hwndLV, -1, LVNI_SELECTED);
        if (iIndex > -1)
        {
            LV_ITEM lvi = {0};
            lvi.mask = LVIF_PARAM;
            lvi.iItem = iIndex;
            ListView_GetItem(_hwndLV, &lvi);

            if (SUCCEEDED(g_pActiveDesk->GetDesktopItemByID( lvi.lParam, &comp, 0)))
            {
                fHaveSelection = TRUE;
                //Check if this is a special component.
#ifdef UNICODE
                pszSource = (LPTSTR)comp.wszSource;
#else
                SHUnicodeToAnsi(comp.wszSource, szCompSource, ARRAYSIZE(szCompSource));
                pszSource = szCompSource;
#endif
                fSpecialComp = !lstrcmpi(pszSource, MY_HOMEPAGE_SOURCE);
            }
        }

//  98/08/19 vtan #142332: If there was a previously selected item
//  then reselect it and mark that there is now no previously selected
//  item.

        else if (_iPreviousSelection > -1)
        {
            ListView_SetItemState(_hwndLV, _iPreviousSelection, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            _iPreviousSelection = -1;
            // The above ListView_SetItemState results in LVN_ITEMCHANGED notification to _onNotify
            // function which inturn calls this _EnableControls again (recursively) and that call
            // enables/disables the buttons properly because now an item is selected. Nothing more
            // to do and hence this return.
            // This is done to fix Bug #276568.
            return;
        }

        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_NEW), _fAllowAdd);

        //
        // Delete button only enabled when an item is selected AND if it is NOT a special comp.
        //
        fEnable = _fAllowDel && fHaveSelection && !fSpecialComp;
        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_DELETE), fEnable);

        //
        // Properties button only enabled on URL based pictures
        // and websites.
        //
        fEnable = FALSE;
        if (_fAllowEdit && fHaveSelection)
        {
            switch (comp.iComponentType)
            {
                case COMP_TYPE_PICTURE:
                case COMP_TYPE_WEBSITE:
                    //pszSource is already initialized if fHaveSelection is TRUE.
                    if (PathIsURL(pszSource))
                    {
                        fEnable = TRUE;
                    }
                    break;
            }
        }
        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_PROPERTIES), fEnable);
        
        EnableWindow(_hwndLV, TRUE);
        
    }
    else
    {
        EnableWindow(_hwndLV, FALSE);
        int iSel = ListView_GetNextItem(_hwndLV, -1, LVNI_SELECTED);
        if (iSel > -1)
        {
            ListView_SetItemState(_hwndLV, iSel, 0, LVIS_SELECTED | LVIS_FOCUSED);

//  98/08/19 vtan #142332: If there is a currently selected item
//  at the time of disabling then save it so that it can be
//  reselected on enabling.

            _iPreviousSelection = iSel;
        }
        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_NEW), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_DELETE), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_PROPERTIES), FALSE);
    }
}

void THISCLASS::_OnInitDialog(HWND hwnd)
{
    _hwnd = hwnd;
    _hwndLV = GetDlgItem(_hwnd, IDC_COMP_LIST);
    _fLaunchGallery = FALSE;
    HWND hWndComp = GetDlgItem(hwnd, IDC_COMP_PREVIEW);
    if (hWndComp) {
        // Turn off mirroring for this control.
        SetWindowBits(hWndComp, GWL_EXSTYLE, RTL_MIRRORED_WINDOW, 0);
        
    }

    if (!g_pActiveDesk)
    {
        HRESULT  hres;
        IActiveDesktopP * piadp;

        if (SUCCEEDED(hres = CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&piadp, IID_IActiveDesktopP)))
        {
            WCHAR wszScheme[MAX_PATH];
            DWORD dwcch = ARRAYSIZE(wszScheme);

            // Get the global "edit" scheme and set ourselves us to read from and edit that scheme
            if (SUCCEEDED(piadp->GetScheme(wszScheme, &dwcch, SCHEME_GLOBAL | SCHEME_EDIT)))
            {
                piadp->SetScheme(wszScheme, SCHEME_LOCAL);
                
            }
            hres = piadp->QueryInterface(IID_IActiveDesktop, (LPVOID *)&g_pActiveDesk);
            piadp->Release();
        }

        if (FAILED(hres))
        {
            return;
        }
    }
    else
    {
        g_pActiveDesk->AddRef();
    }

    //
    // Read in the restrictions.
    //
    _fAllowAdd = !SHRestricted(REST_NOADDDESKCOMP);
    _fAllowDel = !SHRestricted(REST_NODELDESKCOMP);
    _fAllowEdit = !SHRestricted(REST_NOEDITDESKCOMP);
    _fAllowClose = !SHRestricted(REST_NOCLOSEDESKCOMP);
    _fAllowReset = _fAllowAdd && _fAllowDel && _fAllowEdit &&
                    _fAllowClose && !SHRestricted(REST_NOCHANGINGWALLPAPER);
    _fForceAD = SHRestricted(REST_FORCEACTIVEDESKTOPON);

    EnableWindow(GetDlgItem(_hwnd, IDC_COMP_NEW), _fAllowAdd);
    EnableWindow(GetDlgItem(_hwnd, IDC_COMP_DELETE), _fAllowDel);
    EnableWindow(GetDlgItem(_hwnd, IDC_COMP_PROPERTIES), _fAllowEdit);
    if (_fAllowClose)
    {
        ListView_SetExtendedListViewStyle(_hwndLV, LVS_EX_CHECKBOXES);
    }

    //
    // Add the single column that we want.
    //
    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.iSubItem = 0;
    ListView_InsertColumn(_hwndLV, 0, &lvc);

    //
    // Now make the UI match the g_pActiveDesk object.
    //
    _SetUIFromDeskState(FALSE);

    //
    // Select the first item, if it exists.
    //
    int cComp;
    g_pActiveDesk->GetDesktopItemCount(&cComp, 0);
    if (cComp)
    {
        ListView_SetItemState(_hwndLV, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    _EnableControls();
}

void THISCLASS::_OnNotify(LPNMHDR lpnm)
{
    switch (lpnm->code)
    {
    case PSN_SETACTIVE:
        //
        // Make sure the Disable Active Desktop button is in correct state.
        //
        COMPONENTSOPT co;
        co.dwSize = sizeof(COMPONENTSOPT);
        g_pActiveDesk->GetDesktopItemOptions(&co, 0);
        CheckDlgButton(_hwnd, IDC_COMP_ENABLEAD, co.fActiveDesktop ? BST_CHECKED : BST_UNCHECKED);
        break;

    case PSN_APPLY:
        IActiveDesktopP * piadp;
        DWORD dwApplyFlags;

        dwApplyFlags = AD_APPLY_ALL | AD_APPLY_DYNAMICREFRESH;

        if (SUCCEEDED(g_pActiveDesk->QueryInterface(IID_IActiveDesktopP, (LPVOID *)&piadp)))
        {
            WCHAR wszEdit[MAX_PATH], wszDisplay[MAX_PATH];
            DWORD dwcch = ARRAYSIZE(wszEdit);

            // If the edit scheme and display scheme are different, then we need to make
            // sure we force an update.
            if (SUCCEEDED(piadp->GetScheme(wszEdit, &dwcch, SCHEME_GLOBAL | SCHEME_EDIT)) &&
                (dwcch = ARRAYSIZE(wszDisplay)) &&
                SUCCEEDED(piadp->GetScheme(wszDisplay, &dwcch, SCHEME_GLOBAL | SCHEME_DISPLAY)))
            {
                if (StrCmpW(wszDisplay, wszEdit))
                    dwApplyFlags |= AD_APPLY_FORCE;
                
            }
            piadp->Release();
        }

        // PSN_APPLY notification message is sent to background prop. tab also. 
        // Inside this function we check to dirty bit to avoid calling this twice.
        EnableADifHtmlWallpaper(_hwnd);
        SetSafeMode(SSM_CLEAR);
        g_pActiveDesk->ApplyChanges(dwApplyFlags);
        SetWindowLongPtr(_hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
        _SetUIFromDeskState(TRUE);
        _EnableControls();
        break;

    case LVN_ITEMCHANGED:
        NM_LISTVIEW *pnmlv = (NM_LISTVIEW *)lpnm;

        if ((pnmlv->uChanged & LVIF_STATE) &&
            ((pnmlv->uNewState ^ pnmlv->uOldState) & COMP_CHECKED))
        {
            LV_ITEM lvi = {0};
            lvi.iItem = pnmlv->iItem;
            lvi.mask = LVIF_PARAM;
            ListView_GetItem(_hwndLV, &lvi);

            COMPONENT comp;
            comp.dwSize = sizeof(COMPONENT);
            if (SUCCEEDED(g_pActiveDesk->GetDesktopItemByID(lvi.lParam, &comp, 0)))
            {
                comp.fChecked = (pnmlv->uNewState & COMP_CHECKED) != 0;
                g_pActiveDesk->ModifyDesktopItem(&comp, COMP_ELEM_CHECKED);
            }
            InvalidateRect(GetDlgItem(_hwnd, IDC_COMP_PREVIEW), NULL, FALSE);
            EnableApplyButton(_hwnd);
        }

        if ((pnmlv->uChanged & LVIF_STATE) &&
            ((pnmlv->uNewState ^ pnmlv->uOldState) & LVIS_SELECTED))
        {
            InvalidateRect(GetDlgItem(_hwnd, IDC_COMP_PREVIEW), NULL, FALSE);
            _EnableControls();
        }
        break;
    }
}

//
// Returns TRUE if the string looks like a candidate for
// getting qualified as "file:".
//
BOOL LooksLikeFile(LPCTSTR psz)
{
    BOOL fRet = FALSE;

    if (psz[0] &&
        psz[1] &&
#ifndef UNICODE
        !IsDBCSLeadByte(psz[0]) &&
        !IsDBCSLeadByte(psz[1]) &&
#endif
        ((psz[0] == TEXT('\\')) ||
         (psz[1] == TEXT(':')) ||
         (psz[1] == TEXT('|'))))
    {
        fRet = TRUE;
    }

    return fRet;
}


#define GOTO_GALLERY    (-2)

BOOL_PTR CALLBACK AddComponentDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPTSTR pszSource = (LPTSTR)GetWindowLongPtr(hdlg, DWLP_USER);
    TCHAR szBuf[INTERNET_MAX_URL_LENGTH];

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pszSource = (LPTSTR)lParam;
        SetWindowLongPtr(hdlg, DWLP_USER, (LONG_PTR)pszSource);

        SetDlgItemText(hdlg, IDC_CPROP_SOURCE, c_szNULL);
        EnableWindow(GetDlgItem(hdlg, IDOK), FALSE);
        SHAutoComplete(GetDlgItem(hdlg, IDC_CPROP_SOURCE), 0);
        return TRUE;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_CPROP_BROWSE:
            {
                GetDlgItemText(hdlg, IDC_CPROP_SOURCE, szBuf, ARRAYSIZE(szBuf));
                if (!LooksLikeFile(szBuf))
                {
                    //
                    // Open the favorites folder when we aren't
                    // looking at a specific file.
                    //
                    SHGetSpecialFolderPath(hdlg, szBuf, CSIDL_FAVORITES, FALSE);

                    //
                    // Append a slash because GetFileName breaks the
                    // string into a file & dir, and we want to make sure
                    // the entire favorites path is treated as a dir.
                    //
                    lstrcat(szBuf, TEXT("\\"));
                }
                DWORD   adwFlags[] = {   
                                        GFN_ALL,            
                                        GFN_PICTURE,       
                                        (GFN_LOCALHTM | GFN_LOCALMHTML | GFN_CDF | GFN_URL), 
                                        0
                                    };
                int     aiTypes[]  = {   
                                        IDS_COMP_FILETYPES, 
                                        IDS_ALL_PICTURES,  
                                        IDS_ALL_HTML, 
                                        0
                                    };
                if (GetFileName(hdlg, szBuf, ARRAYSIZE(szBuf), aiTypes, adwFlags))
                {
                    CheckAndResolveLocalUrlFile(szBuf, ARRAYSIZE(szBuf));
                    SetDlgItemText(hdlg, IDC_CPROP_SOURCE, szBuf);
                }
            }
            break;

        case IDC_CPROP_SOURCE:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
            {
                EnableWindow(GetDlgItem(hdlg, IDOK), GetWindowTextLength(GetDlgItem(hdlg, IDC_CPROP_SOURCE)) > 0);
            }
            break;

        case IDOK:
            GetDlgItemText(hdlg, IDC_CPROP_SOURCE, pszSource, INTERNET_MAX_URL_LENGTH);
            ASSERT(pszSource[0]);
            if (ValidateFileName(hdlg, pszSource, IDS_COMP_TYPE1))
            {
                CheckAndResolveLocalUrlFile(pszSource, INTERNET_MAX_URL_LENGTH);

                //
                // Qualify non file-protocol strings.
                //
                if (!LooksLikeFile(pszSource))
                {
                    DWORD cchSize = INTERNET_MAX_URL_LENGTH;

                    PathRemoveBlanks(pszSource);
                    ParseURLFromOutsideSource(pszSource, pszSource, &cchSize, NULL);
                }

                EndDialog(hdlg, 0);
            }
            break;

        case IDCANCEL:
            EndDialog(hdlg, -1);
            break;

        case IDC_GOTO_GALLERY:
            EndDialog(hdlg, GOTO_GALLERY);
            break;
        }
        break;
    }

    return FALSE;
}

BOOL IsUrlPicture(LPCTSTR pszUrl)
{
    BOOL fRet = FALSE;

    if(pszUrl[0] == TEXT('\0'))
    {
        fRet = TRUE;
    }
    else
    {
        LPTSTR pszExt = PathFindExtension(pszUrl);

        if ((lstrcmpi(pszExt, TEXT(".BMP"))  == 0) ||
            (lstrcmpi(pszExt, TEXT(".GIF"))  == 0) ||
            (lstrcmpi(pszExt, TEXT(".JPG"))  == 0) ||
            (lstrcmpi(pszExt, TEXT(".JPE"))  == 0) ||
            (lstrcmpi(pszExt, TEXT(".JPEG")) == 0) ||
            (lstrcmpi(pszExt, TEXT(".DIB"))  == 0) ||
            (lstrcmpi(pszExt, TEXT(".PNG"))  == 0))
        {
            fRet = TRUE;
        }
    }

    return(fRet);
}

int GetComponentType(LPCTSTR pszUrl)
{
    return IsUrlPicture(pszUrl) ? COMP_TYPE_PICTURE : COMP_TYPE_WEBSITE;
}

void CreateComponent(COMPONENTA *pcomp, LPCTSTR pszUrl)
{
    pcomp->dwSize = SIZEOF(*pcomp);
    pcomp->dwID = (DWORD)-1;
    pcomp->iComponentType = GetComponentType(pszUrl);
    pcomp->fChecked = TRUE;
    pcomp->fDirty = FALSE;
    pcomp->fNoScroll = FALSE;
    pcomp->cpPos.dwSize = sizeof(pcomp->cpPos);
    pcomp->cpPos.iLeft = COMPONENT_DEFAULT_LEFT;
    pcomp->cpPos.iTop = COMPONENT_DEFAULT_TOP;
    pcomp->cpPos.dwWidth = COMPONENT_DEFAULT_WIDTH;
    pcomp->cpPos.dwHeight = COMPONENT_DEFAULT_HEIGHT;
    pcomp->cpPos.izIndex = COMPONENT_TOP;
    pcomp->cpPos.fCanResize = TRUE;
    pcomp->cpPos.fCanResizeX = pcomp->cpPos.fCanResizeY = TRUE;
    pcomp->cpPos.iPreferredLeftPercent = pcomp->cpPos.iPreferredTopPercent = 0;
    lstrcpyn(pcomp->szSource, pszUrl, ARRAYSIZE(pcomp->szSource));
    lstrcpyn(pcomp->szSubscribedURL, pszUrl, ARRAYSIZE(pcomp->szSubscribedURL));
    pcomp->szFriendlyName[0] = TEXT('\0');
}

BOOL FindComponent(LPCTSTR pszUrl)
{
    BOOL    fRet = FALSE;
    int     i, ccomp;
    LPWSTR  pwszUrl;

#ifndef UNICODE
    WCHAR   wszUrl[INTERNET_MAX_URL_LENGTH];

    SHAnsiToUnicode(pszUrl, wszUrl, ARRAYSIZE(wszUrl));
    pwszUrl = wszUrl;
#else
    pwszUrl = (LPWSTR)pszUrl;
#endif

    g_pActiveDesk->GetDesktopItemCount(&ccomp, 0);
    for (i=0; i<ccomp; i++)
    {
        COMPONENT comp;
        comp.dwSize = sizeof(COMPONENT);
        if (SUCCEEDED(g_pActiveDesk->GetDesktopItem(i, &comp, 0)))
        {
            if (StrCmpIW(pwszUrl, comp.wszSource) == 0)
            {
                fRet = TRUE;
                break;
            }
        }
    }

    return fRet;
}

void EmptyListview(HWND hwndLV)
{
    //
    // Delete all the old components.
    //
    int cComp;
    g_pActiveDesk->GetDesktopItemCount(&cComp, 0);
    int i;
    COMPONENT comp;
    comp.dwSize = sizeof(COMPONENT);
    for (i=0; i<cComp; i++)
    {
        ListView_DeleteItem(hwndLV, 0);
    }
}

void THISCLASS::_SelectComponent(LPWSTR pwszUrl)
{
    //
    // Look for the component with our URL.
    //
    int cComp;
    COMPONENT comp = { SIZEOF(comp) };
    g_pActiveDesk->GetDesktopItemCount(&cComp, 0);
    for (int i=0; i<cComp; i++)
    {
        if (SUCCEEDED(g_pActiveDesk->GetDesktopItem(i, &comp, 0)))
        {
            if (StrCmpW(pwszUrl, comp.wszSource) == 0)
            {
                break;
            }
        }
    }

    //
    // Find the matching listview entry (search for dwID).
    //
    if (i != cComp)
    {
        int nItems = ListView_GetItemCount(_hwndLV);

        for (i=0; i<nItems; i++)
        {
            LV_ITEM lvi = {0};

            lvi.iItem = i;
            lvi.mask = LVIF_PARAM;
            ListView_GetItem(_hwndLV, &lvi);
            if (lvi.lParam == (LPARAM)comp.dwID)
            {
                //
                // Found it, select it and exit.
                //
                ListView_SetItemState(_hwndLV, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                ListView_EnsureVisible(_hwndLV, i, FALSE);
                break;
            }
        }
    }
}

INT_PTR NewComponent(HWND hwndOwner, IActiveDesktop * pad, BOOL fDeferGallery, COMPONENT * pcomp)
{
    //
    // Get the component name.
    //
    TCHAR szSource[INTERNET_MAX_URL_LENGTH];
    COMPONENT comp;
    INT_PTR iChoice = DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(IDD_ADDCOMPONENT), hwndOwner, AddComponentDlgProc, (LPARAM)szSource);

    if (!pcomp)
    {
        pcomp = &comp;
        pcomp->dwSize = SIZEOF(comp);
        pcomp->dwCurItemState = IS_NORMAL;
    }
    
    if (iChoice == GOTO_GALLERY)   // the user wants to launch the gallery
    {
        if (!fDeferGallery)
        {
            WCHAR szGalleryUrl[INTERNET_MAX_URL_LENGTH];
    
            if (SUCCEEDED(URLSubLoadString(HINST_THISDLL, IDS_VISIT_URL, szGalleryUrl, ARRAYSIZE(szGalleryUrl), URLSUB_ALL)))
            {
                NavToUrlUsingIEW(szGalleryUrl, TRUE);
            }
        }
    }
    else if (iChoice >= 0)
    {   // the user has entered a URL address
        WCHAR szSourceW[INTERNET_MAX_URL_LENGTH];
        
        SHTCharToUnicode(szSource, szSourceW, ARRAYSIZE(szSourceW));

        if (!SUCCEEDED(pad->AddUrl(hwndOwner, szSourceW, pcomp, 0)))
            iChoice = -1;
    }

    return iChoice;
}

void THISCLASS::_NewComponent(void)
{
    COMPONENT comp;
    comp.dwSize = SIZEOF(comp);
    comp.dwCurItemState = IS_NORMAL;
    INT_PTR iChoice = NewComponent(_hwnd, g_pActiveDesk, TRUE, &comp);
    
    if (iChoice == GOTO_GALLERY)   // the user wants to launch the gallery
    {
        _fLaunchGallery = TRUE;
        PropSheet_PressButton(GetParent(_hwnd), PSBTN_OK);
    }
    else
    {
        if (iChoice >= 0) // the user has entered a URL address
        {
            // Add component to listview.
            //
            // Need to reload the entire listview so that it is shown in
            // the correct zorder.
            //
            _SetUIFromDeskState(TRUE);

            //
            // Select the newly added component.
            //
            _SelectComponent(comp.wszSource);

            //
            // Redraw preview window.
            //
            InvalidateRect(GetDlgItem(_hwnd, IDC_COMP_PREVIEW), NULL, FALSE);
        }

//  98/08/19 vtan #152418: Disable the "New" button in the property
//  sheet so that when enabled by _EnableControls() and the focus is
//  shifted the border that indicates this is the default butotn is
//  moved when the component list is set to be the focus.

        else
        {
            (BOOL)EnableWindow(GetDlgItem(_hwnd, IDC_COMP_NEW), false);
        }
        //
        // Enable Apply button and any necessary controls.
        //
        _EnableControls();
        EnableApplyButton(_hwnd);
    }
}

void THISCLASS::_EditComponent(void)
{
    int iIndex = ListView_GetNextItem(_hwndLV, -1, LVNI_SELECTED);
    if (iIndex > -1)
    {
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iIndex;
        ListView_GetItem(_hwndLV, &lvi);

        COMPONENT comp = { SIZEOF(comp) };
        if (SUCCEEDED(g_pActiveDesk->GetDesktopItemByID(lvi.lParam, &comp, 0)))
        {
            LPTSTR  pszSubscribedURL;
#ifndef UNICODE
            TCHAR   szSubscribedURL[INTERNET_MAX_URL_LENGTH];

            SHUnicodeToAnsi(comp.wszSubscribedURL, szSubscribedURL, ARRAYSIZE(szSubscribedURL));
            pszSubscribedURL = szSubscribedURL;
#else
            pszSubscribedURL = (LPTSTR)comp.wszSubscribedURL;
#endif
            if (SUCCEEDED(ShowSubscriptionProperties(pszSubscribedURL, _hwnd)))
            {
                EnableApplyButton(_hwnd);
            }
        }
    }
}

void THISCLASS::_DeleteComponent(void)
{
    int iIndex = ListView_GetNextItem(_hwndLV, -1, LVNI_ALL | LVNI_SELECTED);
    if (iIndex > -1)
    {
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iIndex;
        ListView_GetItem(_hwndLV, &lvi);

        COMPONENT comp;
        comp.dwSize = sizeof(COMPONENT);
        if (SUCCEEDED(g_pActiveDesk->GetDesktopItemByID(lvi.lParam, &comp, 0)))
        {
            TCHAR szMsg[1024];
            TCHAR szTitle[MAX_PATH];

            LoadString(HINST_THISDLL, IDS_COMP_CONFIRMDEL, szMsg, ARRAYSIZE(szMsg));
            LoadString(HINST_THISDLL, IDS_COMP_TITLE, szTitle, ARRAYSIZE(szTitle));

            if (MessageBox(_hwnd, szMsg, szTitle, MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
                g_pActiveDesk->RemoveDesktopItem(&comp, 0);

                ListView_DeleteItem(_hwndLV, iIndex);
                int cComp = ListView_GetItemCount(_hwndLV);
                if (cComp == 0)
                {
                    SendMessage(_hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(_hwnd, IDC_COMP_NEW), TRUE);
                }
                else
                {
                    int iSel = (iIndex > cComp - 1 ? cComp - 1 : iIndex);

                    ListView_SetItemState(_hwndLV, iSel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                }

                LPTSTR  pszSubscribedURL;
#ifndef UNICODE
                TCHAR   szSubscribedURL[INTERNET_MAX_URL_LENGTH];

                SHUnicodeToAnsi(comp.wszSubscribedURL, szSubscribedURL, ARRAYSIZE(szSubscribedURL));
                pszSubscribedURL = szSubscribedURL;
#else
                pszSubscribedURL = comp.wszSubscribedURL;
#endif
                DeleteFromSubscriptionList(pszSubscribedURL);

                InvalidateRect(GetDlgItem(_hwnd, IDC_COMP_PREVIEW), NULL, FALSE);
            }

//  98/08/19 vtan #152418: See _NewComponent for an explanation.

            else
            {
                (BOOL)EnableWindow(GetDlgItem(_hwnd, IDC_COMP_DELETE), false);
            }
            _EnableControls();
            EnableApplyButton(_hwnd);
        }
    }
}

BOOL THISCLASS::_VerifyFolderOptions(void)
{
    TCHAR szTitle[80], szText[1024];

    LoadString(HINST_THISDLL, IDS_FOLDEROPT_TITLE, szTitle, ARRAYSIZE(szTitle));
    LoadString(HINST_THISDLL, IDS_FOLDEROPT_TEXT, szText, ARRAYSIZE(szText));

    return MessageBox(_hwnd, szText, szTitle, MB_YESNO | MB_ICONINFORMATION) == IDYES;
}

void THISCLASS::_OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
    BOOL fFocusToList = FALSE;
    COMPONENTSOPT co;

    switch (wID)
    {
    case IDC_COMP_NEW:
        _NewComponent();

//  98/08/19 vtan #152418: Set the default border to "New". This
//  will be changed when the focus is changed to the component list
//  but this allows the dialog handling code to draw the default
//  border correctly.

        (BOOL)SendMessage(_hwnd, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(GetDlgItem(_hwnd, IDC_COMP_NEW)), static_cast<BOOL>(true));
        fFocusToList = TRUE;
        break;

    case IDC_COMP_PROPERTIES:
        _EditComponent();

//  98/08/19 vtan #152418: Same as above.

        (BOOL)EnableWindow(GetDlgItem(_hwnd, IDC_COMP_PROPERTIES), false);
        _EnableControls();
        (BOOL)SendMessage(_hwnd, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(GetDlgItem(_hwnd, IDC_COMP_PROPERTIES)), static_cast<BOOL>(true));
        fFocusToList = TRUE;
        break;

    case IDC_COMP_DELETE:
        _DeleteComponent();

//  98/08/19 vtan #152418: Same as above.

        (BOOL)SendMessage(_hwnd, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(GetDlgItem(_hwnd, IDC_COMP_DELETE)), static_cast<BOOL>(true));
        fFocusToList = TRUE;
        break;
        
    case IDC_COMP_ENABLEAD:
        co.dwSize = sizeof(COMPONENTSOPT);
        g_pActiveDesk->GetDesktopItemOptions(&co, 0);
        co.fActiveDesktop = IsDlgButtonChecked(_hwnd, wID) == BST_CHECKED;
        g_pActiveDesk->SetDesktopItemOptions(&co, 0);
        _EnableControls();
        InvalidateRect(GetDlgItem(_hwnd, IDC_COMP_PREVIEW), NULL, FALSE);
        EnableApplyButton(_hwnd);
        break;

    }

    //Set the focus back to the components list, if necessary
    if (fFocusToList)
    {
        int iIndex = ListView_GetNextItem(_hwndLV, -1, LVNI_SELECTED);
        if (iIndex > -1)
        {
            SetFocus(GetDlgItem(_hwnd, IDC_COMP_LIST));
        }
    }
}

void THISCLASS::_OnDestroy(void)
{
    if (g_pActiveDesk->Release() == 0)
    {
        g_pActiveDesk = NULL;
    }

    if (_fLaunchGallery)
    {
        WCHAR szGalleryUrl[INTERNET_MAX_URL_LENGTH];

        if (SUCCEEDED(URLSubLoadString(HINST_THISDLL, IDS_VISIT_URL, szGalleryUrl, ARRAYSIZE(szGalleryUrl), URLSUB_ALL)))
        {
            NavToUrlUsingIEW(szGalleryUrl, TRUE);
        }
    }
}

void THISCLASS::_OnGetCurSel(int *piIndex)
{
    if (_hwndLV)
    {
        *piIndex = ListView_GetNextItem(_hwndLV, -1, LVNI_ALL | LVNI_SELECTED);
    }
    else
    {
        *piIndex = -1;
    }
}

BOOL_PTR CALLBACK THISCLASS::_DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CCompPropSheetPage *pcpsp = (CCompPropSheetPage *)GetWindowLongPtr(hdlg, DWLP_USER);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pcpsp = (CCompPropSheetPage *)lParam;
        SetWindowLongPtr(hdlg, DWLP_USER, (LPARAM)pcpsp);

        pcpsp->_OnInitDialog(hdlg);
        break;

    case WM_NOTIFY:
        pcpsp->_OnNotify((LPNMHDR)lParam);
        break;

    case WM_COMMAND:
        pcpsp->_OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
        break;

    case WM_SYSCOLORCHANGE:
    case WM_SETTINGCHANGE:
    case WM_DISPLAYCHANGE:
        SHPropagateMessage(hdlg, uMsg, wParam, lParam, TRUE);
        break;

    case WM_DESTROY:
        if (pcpsp)
        {
            pcpsp->_OnDestroy();
        }
        break;

    case WM_COMP_GETCURSEL:
        pcpsp->_OnGetCurSel((int *)lParam);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
                HELP_WM_HELP, (ULONG_PTR)(LPVOID)aCompHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, c_szHelpFile, HELP_CONTEXTMENU,
                (ULONG_PTR)(LPVOID) aCompHelpIDs);
        break;
    }

    return FALSE;
}
#endif
