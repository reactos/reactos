#include "stdafx.h"
//#pragma hdrstop
//#include <shellids.h>
//#include "resource.h"
//#include "deskhtml.h"
//#include "deskstat.h"
//#include "dcomp.h"
//#include "dsubscri.h"
//#include "dutil.h"
//#include "options.h"
//#include "webcheck.h"
//#include <shlwapip.h>

#include <mluisupp.h>

#define _BROWSEUI_      // Make functions exported from browseui as stdapi (as they are delay loaded)
//#include "iethread.h"
//#include "browseui.h"

#ifdef POSTSPLIT

static void EmptyListview(HWND hwndLV, BOOL fDeleteComponents);
static void GetSubscriptionList(HDSA* hdsaSubscriptions);
static void ReplaceSubscriptions(HDSA hdsaOldSubscriptions, HDSA hdsaNewSubscriptions, HWND hwnd);
static void DSA_DestroyWithMembers(HDSA hdsa);

#define DXA_GROWTH_CONST 10
 
#define COMP_CHECKED    0x00002000
#define COMP_UNCHECKED  0x00001000

#define GALRET_NO       0x00000001
#define GALRET_NEVER    0x00000002

#define THISCLASS CCompPropSheetPage

#define c_szHelpFile    TEXT("Update.hlp")
const static DWORD aCompHelpIDs[] = {  // Context Help IDs
    IDC_COMP_CHANNELS,  IDH_LIST_CHANNELS,
    IDC_COMP_ENABLEAD,  IDH_VIEW_AS_WEB_PAGE,
    IDC_COMP_FOLDEROPT, IDH_FOLDER_OPTIONS,
    IDC_COMP_LIST,      IDH_LIST_CHANNELS,
    IDC_COMP_NEW,       IDH_NEW_CHANNEL,
    IDC_COMP_DELETE,    IDH_DELETE_CHANNEL,
    IDC_COMP_PROPERTIES,IDH_CHANNEL_PROPERTIES,
    IDC_COMP_RESET,     IDH_RESET_ALL,
    IDC_COMP_PREVIEW,   IDH_DISPLAY_CHANNELS,
    0, 0
};

typedef struct
{
    WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
    SUBSCRIPTIONINFO si;
} BACKUPSUBSCRIPTION;

static HDSA g_hdsaBackupSubscriptions = NULL;

THISCLASS::CCompPropSheetPage(void)
{
    dwSize = sizeof(THISCLASS);
    dwFlags = PSP_DEFAULT | PSP_USECALLBACK;
    hInstance = HINST_THISDLL;
    pszTemplate = MAKEINTRESOURCE(IDD_COMPONENTS);
    // hIcon = NULL; // unused (PSP_USEICON is not set)
    // pszTitle = NULL; // unused (PSP_USETITLE is not set)
    pfnDlgProc = (DLGPROC)_DlgProc;
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
        EmptyListview(_hwndLV, FALSE);
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

    //
    // Put checkboxes in correct state.
    //
    COMPONENTSOPT co;
    co.dwSize = sizeof(COMPONENTSOPT);
    g_pActiveDesk->GetDesktopItemOptions(&co, 0);
    CheckDlgButton(_hwnd, IDC_COMP_ENABLEAD, co.fActiveDesktop ? BST_CHECKED : BST_UNCHECKED);

    //
    // Reenable redraws.
    //
    SendMessage(_hwndLV, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(_hwndLV, NULL, TRUE);
}

void THISCLASS::_EnableControls(void)
{
    COMPONENTSOPT co;
    co.dwSize = sizeof(COMPONENTSOPT);
    g_pActiveDesk->GetDesktopItemOptions(&co, 0);

    if (co.fActiveDesktop)
    {
        BOOL fEnable;
        COMPONENT comp = { SIZEOF(comp) };
        BOOL fHaveSelection = FALSE;

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

            if (SUCCEEDED(g_pActiveDesk->GetDesktopItemByID(lvi.lParam, &comp, 0)))
            {
                fHaveSelection = TRUE;
            }
        }

        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_NEW), _fAllowAdd);

        //
        // Delete button only enabled when an item is selected.
        //
        fEnable = _fAllowDel && fHaveSelection;
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
                LPTSTR  pszSource;
#ifdef UNICODE
                pszSource = (LPTSTR)comp.wszSource;
#else
                TCHAR   szCompSource[INTERNET_MAX_URL_LENGTH];
                SHUnicodeToAnsi(comp.wszSource, szCompSource, ARRAYSIZE(szCompSource));
                pszSource = szCompSource;
#endif
                if (PathIsURL(pszSource))
                {
                    fEnable = TRUE;
                }
                break;
            }
        }
        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_PROPERTIES), fEnable);

        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_RESET), _fAllowReset);

        EnableWindow(_hwndLV, TRUE);
    }
    else
    {
        EnableWindow(_hwndLV, FALSE);
        int iSel = ListView_GetNextItem(_hwndLV, -1, LVNI_SELECTED);
        if (iSel > -1)
        {
            ListView_SetItemState(_hwndLV, iSel, 0, LVIS_SELECTED | LVIS_FOCUSED);
        }

        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_NEW), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_DELETE), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_PROPERTIES), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_COMP_RESET), FALSE);
    }
}

void THISCLASS::_OnInitDialog(HWND hwnd)
{
    _hwnd = hwnd;
    _hwndLV = GetDlgItem(_hwnd, IDC_COMP_LIST);
    _fLaunchGallery = FALSE;
    _fLaunchFolderOpt = FALSE;
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

    // Save the subscription settings so that we can restore them if Cancel is chosen.
    GetSubscriptionList(&g_hdsaBackupSubscriptions);

    //
    // Read in the restrictions.
    //
    _fAllowAdd = !SHRestricted(REST_NOADDDESKCOMP);
    _fAllowDel = !SHRestricted(REST_NODELDESKCOMP);
    _fAllowEdit = !SHRestricted(REST_NOEDITDESKCOMP);
    _fAllowClose = !SHRestricted(REST_NOCLOSEDESKCOMP);
    _fAllowReset = _fAllowAdd && _fAllowDel && _fAllowEdit &&
                    _fAllowClose && !SHRestricted(REST_NOCHANGINGWALLPAPER);

    EnableWindow(GetDlgItem(_hwnd, IDC_COMP_NEW), _fAllowAdd);
    EnableWindow(GetDlgItem(_hwnd, IDC_COMP_DELETE), _fAllowDel);
    EnableWindow(GetDlgItem(_hwnd, IDC_COMP_PROPERTIES), _fAllowEdit);
    EnableWindow(GetDlgItem(_hwnd, IDC_COMP_RESET), _fAllowReset);
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

    case PSN_QUERYCANCEL:
        //Just creating a local block to localise the definition of hdsaCurrentSubscriptions
        {
            // Restores subscriptions from the DSA g_hdsaBackupSubscriptions.
            HDSA hdsaCurrentSubscriptions = NULL;

            GetSubscriptionList(&hdsaCurrentSubscriptions);
            ReplaceSubscriptions(hdsaCurrentSubscriptions, g_hdsaBackupSubscriptions, _hwnd);
            DSA_DestroyWithMembers(hdsaCurrentSubscriptions);
        }
        break;

    case PSN_APPLY:
        IActiveDesktopP * piadp;
        DWORD dwApplyFlags;

        dwApplyFlags = AD_APPLY_ALL;

        // Backup subscriptions again
        DSA_DestroyWithMembers(g_hdsaBackupSubscriptions);
        g_hdsaBackupSubscriptions = NULL;
        GetSubscriptionList(&g_hdsaBackupSubscriptions);

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
        g_pActiveDesk->ApplyChanges(dwApplyFlags);
        SetSafeMode(SSM_CLEAR);
        SetWindowLong(_hwnd, DWL_MSGRESULT, PSNRET_NOERROR);
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


#ifndef SHDOC401_DLL_UI
#define GOTO_GALLERY    (-2)
#endif

#define SetDefaultDialogFont SHSetDefaultDialogFont
#define RemoveDefaultDialogFont SHRemoveDefaultDialogFont

BOOL CALLBACK AddComponentDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPTSTR pszSource = (LPTSTR)GetWindowLong(hdlg, DWL_USER);
    TCHAR szBuf[INTERNET_MAX_URL_LENGTH];

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pszSource = (LPTSTR)lParam;
        SetWindowLong(hdlg, DWL_USER, (LONG)pszSource);

        SetDefaultDialogFont(hdlg, IDC_CPROP_SOURCE);
        SetDlgItemText(hdlg, IDC_CPROP_SOURCE, c_szNULL);
        EnableWindow(GetDlgItem(hdlg, IDOK), FALSE);
        SHAutoComplete(GetDlgItem(hdlg, IDC_CPROP_SOURCE), 0);
        return TRUE;

    case WM_DESTROY:
        RemoveDefaultDialogFont(hdlg);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_CPROP_BROWSE:
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
            if (GetFileName(hdlg, szBuf, ARRAYSIZE(szBuf), IDS_COMP_FILETYPES, GFN_ALL))
            {
                CheckAndResolveLocalUrlFile(szBuf, ARRAYSIZE(szBuf));
                SetDlgItemText(hdlg, IDC_CPROP_SOURCE, szBuf);
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

#ifndef SHDOC401_DLL_UI
        case IDC_GOTO_GALLERY:
            EndDialog(hdlg, GOTO_GALLERY);
            break;
#endif
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

    PositionComponent(&pcomp->cpPos, pcomp->iComponentType);
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

void EmptyListview(HWND hwndLV, BOOL fDeleteComponents)
{
    if (fDeleteComponents)
    {
        SendMessage(hwndLV, WM_SETREDRAW, FALSE, 0);
    }

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

        if (fDeleteComponents)
        {
            g_pActiveDesk->GetDesktopItem(0, &comp, 0);
            g_pActiveDesk->RemoveDesktopItem(&comp, 0);
            LPTSTR  pszSource;
#ifndef UNICODE
            TCHAR   szCompURL[INTERNET_MAX_URL_LENGTH];

            SHUnicodeToAnsi(comp.wszSubscribedURL, szCompURL, ARRAYSIZE(szCompURL));
            pszSource = szCompURL;
#else
            pszSource = comp.wszSubscribedURL;
#endif
            DeleteFromSubscriptionList(pszSource);
        }
    }

    if (fDeleteComponents)
    {
        SendMessage(hwndLV, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hwndLV, NULL, TRUE);
    }
}


BOOL THISCLASS::_VisitGallery(void)
{
    TCHAR szTitle[80], szText[1024];

    MLLoadString(IDS_VISITGALLERY_TITLE, szTitle, ARRAYSIZE(szTitle));
    MLLoadString(IDS_VISITGALLERY_TEXT, szText, ARRAYSIZE(szText));

    return (IDYES == SHMessageBoxCheck(_hwnd, szText, szTitle, MB_YESNO | MB_ICONINFORMATION, IDNO, REG_VAL_GENERAL_VISITGALLERY));
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

void THISCLASS::_NewComponent(void)
{
#ifdef SHDOC401_DLL_UI // old behavior
  //
  // See if the user wants to launch the gallery instead.
  //
  if (_VisitGallery())
  {
      _fLaunchGallery = TRUE;
      PropSheet_PressButton(GetParent(_hwnd), PSBTN_OK);
  }
  else
  {
#endif
    //
    // Get the component name.
    //
    TCHAR szSource[INTERNET_MAX_URL_LENGTH];
    INT_PTR iChoice = DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_ADDCOMPONENT), _hwnd, AddComponentDlgProc, (LPARAM)szSource);

#ifndef SHDOC401_DLL_UI
    if (iChoice == GOTO_GALLERY)   // the user wants to launch the gallery
    {
        _fLaunchGallery = TRUE;
        PropSheet_PressButton(GetParent(_hwnd), PSBTN_OK);
    }
    else
#endif
    if (iChoice >= 0)
    {   // the user has entered a URL address
        BOOL fOkay = TRUE;
        COMPONENT   comp;
        WCHAR szSourceW[INTERNET_MAX_URL_LENGTH];
        
        comp.dwSize = SIZEOF(comp);
        SHTCharToUnicode(szSource, szSourceW, ARRAYSIZE(szSourceW));
        fOkay = SUCCEEDED(g_pActiveDesk->AddUrl(_hwnd, szSourceW, &comp, 0));
     
        if (fOkay)
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

            //
            // Enable Apply button and any necessary controls.
            //
            _EnableControls();
            EnableApplyButton(_hwnd);
        }
    }
#ifdef SHDOC401_DLL_UI
  }
#endif
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

            MLLoadString(IDS_COMP_CONFIRMDEL, szMsg, ARRAYSIZE(szMsg));
            MLLoadString(IDS_COMP_TITLE, szTitle, ARRAYSIZE(szTitle));

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
                _EnableControls();
                EnableApplyButton(_hwnd);
            }
        }
    }
}

BOOL THISCLASS::_VerifyFolderOptions(void)
{
    TCHAR szTitle[80], szText[1024];

    MLLoadString(IDS_FOLDEROPT_TITLE, szTitle, ARRAYSIZE(szTitle));
    MLLoadString(IDS_FOLDEROPT_TEXT, szText, ARRAYSIZE(szText));

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
        fFocusToList = TRUE;
        break;

    case IDC_COMP_PROPERTIES:
        _EditComponent();
        fFocusToList = TRUE;
        break;

    case IDC_COMP_DELETE:
        _DeleteComponent();
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

    case IDC_COMP_FOLDEROPT:
        if (_VerifyFolderOptions())
        {
            _fLaunchFolderOpt = TRUE;
            PropSheet_PressButton(GetParent(_hwnd), PSBTN_OK);
        }
        break;

    case IDC_COMP_RESET:
        TCHAR szMsg[1024];
        TCHAR szTitle[MAX_PATH];

        MLLoadString(IDS_COMP_TITLE, szTitle, ARRAYSIZE(szTitle));
        MLLoadString(IDS_COMP_CONFIRMRESET, szMsg, ARRAYSIZE(szTitle));
        if (MessageBox(_hwnd, szMsg, szTitle, MB_YESNO | MB_ICONQUESTION) != IDYES)
        {
            break;
        }

        //
        // Reset all the check boxes.
        //
        co.dwSize = sizeof(COMPONENTSOPT);
        g_pActiveDesk->GetDesktopItemOptions(&co, 0);
        co.fEnableComponents = TRUE;
        co.fActiveDesktop = TRUE;
        g_pActiveDesk->SetDesktopItemOptions(&co, 0);
        
        //
        // Delete all the old components.
        //
        EmptyListview(_hwndLV, TRUE);

        COMPONENTA compA;
        COMPONENT comp;

        compA.dwSize = sizeof(compA);
        comp.dwSize = sizeof(comp);  //This is needed for MultiCompToWide to work properly.

        //
        // Add the default components.
        //
#ifdef WANT_ORANGE_EGG
        compA.dwID = -1;
        compA.iComponentType = COMP_TYPE_HTMLDOC;
        compA.fChecked = TRUE;
        compA.fDirty = FALSE;
        compA.fNoScroll = FALSE;
        compA.cpPos.dwSize = sizeof(COMPPOS);

        // Find the virtual dimensions.
        EnumMonitorsArea ema;
        GetMonitorSettings(&ema);

        // Position it in the primary monitor.
        compA.cpPos.iLeft = EGG_LEFT + (ema.rcMonitor[0].left - ema.rcVirtualMonitor.left);
        compA.cpPos.iTop = EGG_TOP + (ema.rcMonitor[0].top - ema.rcVirtualMonitor.top);
        compA.cpPos.dwWidth = EGG_WIDTH;
        compA.cpPos.dwHeight = EGG_HEIGHT;
        compA.cpPos.izIndex = COMPONENT_TOP;
        compA.cpPos.fCanResize = compA.cpPos.fCanResizeX = compA.cpPos.fCanResizeY = TRUE;
        compA.cpPos.iPreferredLeftPercent = compA.cpPos.iPreferredTopPercent = 0;

        MLLoadString(IDS_SAMPLE_COMPONENT, compA.szFriendlyName, ARRAYSIZE(compA.szFriendlyName));

        GetWindowsDirectory(compA.szSource, ARRAYSIZE(compA.szSource));
        lstrcat(compA.szSource, COMPON_FILENAME);
        lstrcpy(compA.szSubscribedURL, compA.szSource);
        PositionComponent(&compA.cpPos, compA.iComponentType);
        MultiCompToWideComp(&compA, &comp);
        g_pActiveDesk->AddComponent(&comp);
#endif

        //
        // Don't restore the channel bar on memphis OSR1+.
        //
        if (!IsOS(OS_MEMPHIS) || IsOS(OS_MEMPHIS_GOLD))
        {
            compA.dwID = -1;
            compA.dwSize = SIZEOF(compA);
            compA.iComponentType = COMP_TYPE_CONTROL;
            compA.fChecked = !g_fRunningOnNT;  //On WinNT, the channelbar is turned OFF by default!
            compA.fDirty = FALSE;
            compA.fNoScroll = FALSE;
            compA.cpPos.dwSize = SIZEOF(compA.cpPos);
            GetCBarStartPos(&compA.cpPos.iLeft, &compA.cpPos.iTop, &compA.cpPos.dwWidth, &compA.cpPos.dwHeight);
            compA.cpPos.izIndex = COMPONENT_TOP;
            compA.cpPos.fCanResize = compA.cpPos.fCanResizeX = compA.cpPos.fCanResizeY = TRUE;
            compA.cpPos.iPreferredLeftPercent = compA.cpPos.iPreferredTopPercent = 0;
            MLLoadString(IDS_CHANNEL_BAR, compA.szFriendlyName, ARRAYSIZE(compA.szFriendlyName));
            lstrcpy(compA.szSource, CBAR_SOURCE);
            lstrcpy(compA.szSubscribedURL, CBAR_SOURCE);
            PositionComponent(&compA.cpPos, compA.iComponentType);
            MultiCompToWideComp(&compA, &comp);
            g_pActiveDesk->AddDesktopItem(&comp, 0);
        }

        //
        // Reset the wallpaper settings.
        //
        WALLPAPEROPT wpo;

        wpo.dwSize = sizeof(WALLPAPEROPT);
        g_pActiveDesk->GetWallpaperOptions(&wpo, 0);
        wpo.dwStyle = WPSTYLE_CENTER;
        g_pActiveDesk->SetWallpaperOptions(&wpo, 0);

        //Set the default wallpaper
        TCHAR szWallpaper[INTERNET_MAX_URL_LENGTH];
        WCHAR *pwszWallpaper;

        GetWallpaperDirName(szWallpaper, ARRAYSIZE(szWallpaper));
        lstrcat(szWallpaper, TEXT("\\"));
        TCHAR szWP[INTERNET_MAX_URL_LENGTH];
        GetDefaultWallpaper(szWP);
        lstrcat(szWallpaper, szWP);
#ifndef UNICODE
        WCHAR   wszWallpaper[INTERNET_MAX_URL_LENGTH];
        SHAnsiToUnicode(szWallpaper, wszWallpaper, ARRAYSIZE(wszWallpaper));
        pwszWallpaper = wszWallpaper;
#else
        pwszWallpaper = szWallpaper;
#endif
        g_pActiveDesk->SetWallpaper(pwszWallpaper, 0);

        //
        // Get the UI to match the new desk state.
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
        EnableApplyButton(_hwnd);
        fFocusToList = TRUE;

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

    DSA_DestroyWithMembers(g_hdsaBackupSubscriptions);
    g_hdsaBackupSubscriptions = NULL;

    if (_fLaunchGallery)
    {
        WCHAR szGalleryUrl[INTERNET_MAX_URL_LENGTH];

        if (SUCCEEDED(URLSubLoadString(MLGetHinst(), IDS_VISIT_URL, szGalleryUrl, ARRAYSIZE(szGalleryUrl), URLSUB_ALL)))
        {
            NavToUrlUsingIEW(szGalleryUrl, TRUE);
        }
    }

    if (_fLaunchFolderOpt)
    {
        DWORD dwPid;
        if (GetWindowThreadProcessId(GetShellWindow(), &dwPid)) {
            AllowSetForegroundWindow(dwPid);
        }
        PostMessage(GetShellWindow(), CWM_SHOWFOLDEROPT, 0, 0);
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

BOOL CALLBACK THISCLASS::_DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CCompPropSheetPage *pcpsp = (CCompPropSheetPage *)GetWindowLong(hdlg, DWL_USER);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pcpsp = (CCompPropSheetPage *)lParam;
        SetWindowLong(hdlg, DWL_USER, (LPARAM)pcpsp);

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
        SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
                HELP_WM_HELP, (DWORD)aCompHelpIDs);
        break;

    case WM_CONTEXTMENU:
        SHWinHelpOnDemandWrap((HWND) wParam, c_szHelpFile, HELP_CONTEXTMENU,
                (DWORD)(LPVOID) aCompHelpIDs);
        break;
    }

    return FALSE;
}

// Backs up subscriptions from the g_pActiveDesk structure.
void GetSubscriptionList(HDSA* hdsaSubscriptions)
{
    if (*hdsaSubscriptions != NULL)    // Just to be sure
    {
        TraceMsg(TF_WARNING, "BackupSubscriptions : *hdsaSubscriptions was not Destroyed.");
        DSA_DestroyWithMembers(*hdsaSubscriptions);
    }

    BACKUPSUBSCRIPTION bsi;
    *hdsaSubscriptions = DSA_Create(SIZEOF(bsi), DXA_GROWTH_CONST);

    ISubscriptionMgr *psm;
    if (SUCCEEDED(CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr, (void**)&psm)))
    {
        int cComp;
        g_pActiveDesk->GetDesktopItemCount(&cComp, 0);
        int i;
        COMPONENT comp;
        comp.dwSize = sizeof(COMPONENT);
        for (i=0; i<cComp; i++)
        {
            g_pActiveDesk->GetDesktopItem(i, &comp, 0);
            if(!comp.wszSource)
                continue;
            LPTSTR  pszSource;
#ifndef UNICODE
            TCHAR   szSource[INTERNET_MAX_URL_LENGTH];

            SHUnicodeToAnsi(comp.wszSource, szSource, ARRAYSIZE(szSource));
            pszSource = szSource;
#else
            pszSource = comp.wszSource;
#endif
            if(PathIsURL(pszSource) && !UrlIsFileUrl(pszSource))
            {
                LPTSTR  pszSubscribedURL;
#ifndef UNICODE
                TCHAR   szSubscribedURL[INTERNET_MAX_URL_LENGTH];

                SHUnicodeToAnsi(comp.wszSubscribedURL, szSubscribedURL, ARRAYSIZE(szSubscribedURL));
                pszSubscribedURL = szSubscribedURL;
#else
                pszSubscribedURL = comp.wszSubscribedURL;
#endif
                if(CheckForExistingSubscription(pszSubscribedURL))
                {
                    lstrcpynW(bsi.wszURL, comp.wszSubscribedURL, ARRAYSIZE(bsi.wszURL));
                    bsi.si.cbSize = sizeof(bsi.si);
                    bsi.si.fUpdateFlags = SUBSINFO_ALLFLAGS;
                    HRESULT hr = psm->GetSubscriptionInfo(bsi.wszURL, &bsi.si);
                    
                    if(SUCCEEDED(hr))
                    {
                        if (DSA_AppendItem(*hdsaSubscriptions, &bsi) == -1)
                        {
                            TraceMsg(TF_WARNING, "BackupSubscriptions : Unable to backup subscription info in DSA. Will have problems on clicking Cancel.");
                        }
                    }
                }
            }
        }
        psm->Release();
    }
    else
    {
        TraceMsg(TF_WARNING, "BackupSubscriptions : CoCreateInstance for CLSID_SubscriptionMgr failed.");
    }
}

// Restores subscriptions.
void ReplaceSubscriptions(HDSA hdsaOldSubscriptions, HDSA hdsaNewSubscriptions, HWND hwnd)
{
    ISubscriptionMgr *psm;
    if (SUCCEEDED(CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr, (void**)&psm)))
    {
        // First, delete subscriptions.
        BACKUPSUBSCRIPTION bsi;
        int cSubscriptions = DSA_GetItemCount(hdsaOldSubscriptions);
        int i;
        for(i = 0; i < cSubscriptions; i++)
        {
            DSA_GetItem(hdsaOldSubscriptions, i, &bsi);
            psm->DeleteSubscription(bsi.wszURL, NULL);
        }

        // Now, restore the subscriptions from hdsaNewSubscriptions
        cSubscriptions = DSA_GetItemCount(hdsaNewSubscriptions);
        for(i = 0; i < cSubscriptions; i++)
        {
            DSA_GetItem(hdsaNewSubscriptions, i, &bsi);
            bsi.si.cbSize = sizeof(bsi.si);
            psm->CreateSubscription(hwnd, bsi.wszURL, bsi.si.bstrFriendlyName, CREATESUBS_NOUI, SUBSTYPE_DESKTOPURL, &bsi.si);
        }
        psm->Release();
    }
    else
    {
        TraceMsg(TF_WARNING, "RestoreSubscriptions : CoCreateInstance for CLSID_SubscriptionMgr failed.");
    }
}

void DSA_DestroyWithMembers(HDSA hdsa)
{
    BACKUPSUBSCRIPTION bsi;
    int cSubscriptions = DSA_GetItemCount(hdsa);
    int i;
    for(i = 0; i < cSubscriptions; i++)
    {
        DSA_GetItem(hdsa, i, &bsi);
        if(bsi.si.bstrUserName)
        {
            SysFreeString(bsi.si.bstrUserName);
        }
        if(bsi.si.bstrPassword)
        {
            SysFreeString(bsi.si.bstrPassword);
        }
        if(bsi.si.bstrFriendlyName)
        {
            SysFreeString(bsi.si.bstrFriendlyName);
        }
    }
    DSA_Destroy(hdsa);
}
#endif
