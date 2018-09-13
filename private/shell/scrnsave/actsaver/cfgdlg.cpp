/////////////////////////////////////////////////////////////////////////////
// CFGDLG.CPP
//
// Implementation of IScreenSaverCfgDialog
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     08/26/96    Created
// jaym     01/12/97    Added DisplaySubscriptions
// jaym     01/20/97    Added Advanced settings page
// jaym     07/05/97    Removed Advanced settings page
// jaym     08/01/97    Added context help.
// jaym     08/11/97    Updates for channel enumeration
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "saver.h"
#include "regini.h"
#include "shellids.h"   // for Help IDs
#include "subsmgr.h"
#include "cfgdlg.h"

/////////////////////////////////////////////////////////////////////////////
// External variables
/////////////////////////////////////////////////////////////////////////////
extern TCHAR        g_szRegSubKey[];            // From ACTSAVER.CPP
extern const WCHAR  g_szChannelFlags[];         // from PIDLLIST.CPP
extern const WCHAR  g_szPropScreenSaverURL[];   // from PIDLLIST.CPP

/////////////////////////////////////////////////////////////////////////////
// Module variables
/////////////////////////////////////////////////////////////////////////////
#pragma data_seg(DATASEG_READONLY)
static const TCHAR s_szDontAskCreateSubs[] = TEXT("DontAskCreateSubs");
    // Registry load/save settings keys
#pragma data_seg()

#define NUM_SETTINGS_PAGES  1
const int s_rgnPageIDs[NUM_SETTINGS_PAGES] =
{
    IDD_PAGE1_DIALOG,
};

const DLGPROC s_rgpfnPageFuncs[NUM_SETTINGS_PAGES] =
{
    PSGeneralPageDlgProc,
};

const TCHAR c_szHelpFile[] = TEXT("CHNSCSVR.HLP");
DWORD aCustomDlgHelpIDs[] =
{
    IDC_SUBSCRIPTION_LIST,  IDH_CHANNELS_LIST,
    IDC_CHANNEL_TIME,       IDH_SET_LENGTH,
    IDC_CHANNEL_TIME_SPIN,  IDH_SET_LENGTH,
    IDC_PLAY_SOUNDS,        IDH_PLAY_SOUNDS,
    IDC_NAVIGATE_GROUP_BOX, IDH_CLOSE_SCREENSAVER,
    IDC_NAVIGATE_CLICK,     IDH_CLOSE_SCREENSAVER,
    IDC_NAVIGATE_ALTCLICK,  IDH_CLOSE_SCREENSAVER,
};

/////////////////////////////////////////////////////////////////////////////
// Design constants
/////////////////////////////////////////////////////////////////////////////
#define IMGLIST_NUM_IMAGES      2
#define IMGLIST_IMAGE_WIDTH     16
#define IMGLIST_IMAGE_HEIGHT    16

/////////////////////////////////////////////////////////////////////////////
// IScreenSaverCfgDialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::get_NavigateOnClick
/////////////////////////////////////////////////////////////////////////////
HRESULT CActiveScreenSaver::get_NavigateOnClick
(
    VARIANT_BOOL * pbNavOnClick
)
{
    *pbNavOnClick = m_pSSInfo->bNavigateOnClick;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::put_NavigateOnClick
/////////////////////////////////////////////////////////////////////////////
HRESULT CActiveScreenSaver::put_NavigateOnClick
(
    VARIANT_BOOL bNavOnClick
)
{
    m_pSSInfo->bNavigateOnClick = bNavOnClick;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::get_ChannelTime
/////////////////////////////////////////////////////////////////////////////
HRESULT CActiveScreenSaver::get_ChannelTime
(
    int * pnChannelTime
)
{
    *pnChannelTime = m_pSSInfo->dwChannelTime;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::put_ChannelTime
/////////////////////////////////////////////////////////////////////////////
HRESULT CActiveScreenSaver::put_ChannelTime
(
    int nChannelTime
)
{
    m_pSSInfo->dwChannelTime = nChannelTime;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::get_RestartTime
/////////////////////////////////////////////////////////////////////////////
HRESULT CActiveScreenSaver::get_RestartTime
(
    DWORD * pdwRestartTime
)
{
    *pdwRestartTime = m_pSSInfo->dwRestartTime;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::put_RestartTime
/////////////////////////////////////////////////////////////////////////////
HRESULT CActiveScreenSaver::put_RestartTime
(
    DWORD dwRestartTime
)
{
    m_pSSInfo->dwRestartTime = dwRestartTime;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::get_PlaySounds
/////////////////////////////////////////////////////////////////////////////
HRESULT CActiveScreenSaver::get_PlaySounds
(
    VARIANT_BOOL * pbPlaySounds
)
{
    *pbPlaySounds = m_pSSInfo->bPlaySounds;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::put_PlaySounds
/////////////////////////////////////////////////////////////////////////////
HRESULT CActiveScreenSaver::put_PlaySounds
(
    VARIANT_BOOL bPlaySounds
)
{
    m_pSSInfo->bPlaySounds = bPlaySounds;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::get_Features
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::get_Features
(
    DWORD * pdwFeatureFlags
)
{
    *pdwFeatureFlags = m_pSSInfo->dwFeatureFlags;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::put_Features
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::put_Features
(
    DWORD dwFeatureFlags
)
{
    m_pSSInfo->dwFeatureFlags = dwFeatureFlags;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::ShowDialog
/////////////////////////////////////////////////////////////////////////////
HRESULT CActiveScreenSaver::ShowDialog
(
    HWND hwndParent
)
{
    int             i;
    PROPSHEETHEADER psHeader;
    PROPSHEETPAGE * psPages;
    HINSTANCE       hInst = _pModule->GetResourceInstance();
    CString         strCaption;
    CString         strTab;
    HRESULT         hr = S_OK;

    // Allocate as many property sheet pages as required
    if ((psPages = (PROPSHEETPAGE *) new PROPSHEETPAGE [NUM_SETTINGS_PAGES]) == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Read the screen saver information from the registry.
    hr = ReadSSInfo();

    if (FAILED(hr))
        goto Cleanup;

    // Setup the default image list.
    EVAL(m_hImgList = LoadImageList());

    if (NULL == m_hImgList)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Obtain the property sheet title
    strCaption.LoadString(IDS_DLG_CAPTION);

    psHeader.dwSize     = sizeof(PROPSHEETHEADER);
    psHeader.dwFlags    = PSH_PROPSHEETPAGE
                            | PSH_PROPTITLE
                            | PSH_USEICONID
                            | PSH_NOAPPLYNOW;
    psHeader.hwndParent = hwndParent;
    psHeader.hInstance  = hInst;
    psHeader.pszIcon    = MAKEINTRESOURCE(ID_APP);
    psHeader.pszCaption = strCaption;
    psHeader.nPages     = NUM_SETTINGS_PAGES;
    psHeader.nStartPage = 0;
    psHeader.ppsp       = (LPCPROPSHEETPAGE)psPages;

    // Load the pages. Each page tab text MUST follow
    // the dialog caption in the string table.
    for (i = 0; i < NUM_SETTINGS_PAGES; i++)
    {
        strTab.LoadString(IDS_DLG_CAPTION+i+1);
        if (strTab.GetLength() > 0)
        {
            psPages[i].pszTitle = new char [strTab.GetLength()+1];

            if (NULL == psPages[i].pszTitle)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            lstrcpy((char *)psPages[i].pszTitle, strTab);
        }
        else
            psPages[i].pszTitle = NULL;

        psPages[i].dwSize       = sizeof(PROPSHEETPAGE);
        psPages[i].dwFlags      = PSP_USETITLE;
        psPages[i].hInstance    = hInst;
        psPages[i].pszTemplate  = MAKEINTRESOURCE(s_rgnPageIDs[i]);
        psPages[i].pszIcon      = NULL;
        psPages[i].pfnDlgProc   = s_rgpfnPageFuncs[i];
        psPages[i].lParam       = (LPARAM)(CActiveScreenSaver *)this;
    }

    // Display the settings information.
    PropertySheet(&psHeader);

Cleanup:

    // Remove all the allocated page titles
    for (i = 0; i < NUM_SETTINGS_PAGES; i++)
    {
        if (psPages[i].pszTitle != NULL)
        {
            delete [] (char *)psPages[i].pszTitle;
            psPages[i].pszTitle = NULL;
        }
    }

    if (psPages)
        delete [] psPages;

    if (m_hImgList)
    {
        EVAL(ImageList_Destroy(m_hImgList));
        m_hImgList = NULL;
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::Apply
/////////////////////////////////////////////////////////////////////////////
HRESULT CActiveScreenSaver::Apply
(
)
{
    // Save the settings and notify everyone that settings have changed.
    WriteSSInfo();

    // Save the settings and notify everyone that settings have changed.
    SendNotifyMessage(HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM) "Windows"); 

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Class helper functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// PSGeneralPageDlgProc
/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK PSGeneralPageDlgProc
(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    static PROPSHEETPAGE * s_pspPropSheet;

    switch(message)
    {
        case WM_INITDIALOG:
        {
            s_pspPropSheet = (PROPSHEETPAGE *)lParam;

#ifdef FEATURE_CUSTOMDRAWIMAGES
            CActiveScreenSaver * pScreenSaver = (CActiveScreenSaver *)s_pspPropSheet->lParam;
            ASSERT(pScreenSaver->m_pfnOldGeneralPSDlgProc == NULL);

            // Subclass the window so we can do custom drawing
            m_pfnOldGeneralPSDlgProc = (DLGPROC)GetWindowLong(hWnd, GWL_WNDPROC);
            if (pScreenSaver->m_pfnOldGeneralPSDlgProc != NULL)
                SetWindowLong(hWnd, GWL_WNDPROC, (LONG)PSGeneralPageWndProc);
#endif  // FEATURE_CUSTOMDRAWIMAGES

            return HANDLE_WM_INITDIALOG(hDlg,
                                        wParam,
                                        s_pspPropSheet->lParam,
                                        PSGeneralPage_OnInitDialog);
        }
        
        case WM_HELP:
        {
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle,
                    c_szHelpFile,
                    HELP_WM_HELP,
                    (DWORD_PTR)aCustomDlgHelpIDs);
            break;
        }

        case WM_CONTEXTMENU:
        {
            WinHelp((HWND)wParam,
                    c_szHelpFile,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)aCustomDlgHelpIDs);
            break;
        }

        case WM_COMMAND:
        {
            return HANDLE_WM_COMMAND(   hDlg,
                                        wParam,
                                        lParam,
                                        PSGeneralPage_OnCommand);
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR *)lParam)->code)
            {
                case TVN_KEYDOWN:
                {
                    if (((TV_KEYDOWN *)lParam)->wVKey == VK_SPACE)
                    {
                        HWND hwndTree = GetDlgItem(hDlg, IDC_SUBSCRIPTION_LIST);

                        ToggleItemCheck(hDlg,
                                        hwndTree,
                                        (HTREEITEM)SendMessage( hwndTree,
                                                                TVM_GETNEXTITEM,
                                                                TVGN_CARET,
                                                                NULL));
                    }

                    break;
                }

                case NM_CLICK:
                case NM_DBLCLK:
                {
                    TV_HITTESTINFO  ht;
                    HWND            hwndTree = GetDlgItem(hDlg, IDC_SUBSCRIPTION_LIST);

                    GetCursorPos(&ht.pt);
                    ScreenToClient(hwndTree, &ht.pt);
                    ToggleItemCheck(hDlg, hwndTree, TreeView_HitTest(hwndTree, &ht));

                    break;
                }

                case PSN_APPLY:
                {
                    PSGeneralPage_OnApply(hDlg, s_pspPropSheet->lParam);
                    break;
                }

                default:
                    break;
            }

            break;
        }

        case WM_DESTROY:
        {
#ifdef FEATURE_CUSTOMDRAWIMAGES
            CActiveScreenSaver * pScreenSaver = (CActiveScreenSaver *)s_pspPropSheet->lParam;

            ASSERT(pScreenSaver->m_pfnOldGeneralPSDlgProc != NULL);
            SetWindowLong(hWnd, GWL_WNDPROC, (LONG)m_pfnOldGeneralPSDlgProc);
#endif  // FEATURE_CUSTOMDRAWIMAGES

            // Cleanup param data.
            HWND hwndTree;
            if ((hwndTree = GetDlgItem(hDlg, IDC_SUBSCRIPTION_LIST)) != NULL)
            {
                TV_ITEM tvi = { 0 };

                tvi.hItem = TreeView_GetRoot(hwndTree);
                tvi.mask = TVIF_PARAM;

                while (tvi.hItem != NULL)
                {
                    if (TreeView_GetItem(hwndTree, &tvi))
                    {
                        TVCHANNELDATA * pChannelData = (TVCHANNELDATA *)tvi.lParam;

                        ASSERT(pChannelData != NULL);

                        if (pChannelData->pSubsItem != NULL)
                            pChannelData->pSubsItem->Release();

                        delete pChannelData;
                    }
                    else
                        ASSERT(FALSE);

                    tvi.hItem = TreeView_GetNextItem(hwndTree, tvi.hItem, TVGN_NEXT);
                }
            }
            else
                ASSERT(FALSE);

            break;
        }

        default:
            break;
    }

    return FALSE;
}

#ifdef FEATURE_CUSTOMDRAWIMAGES
/////////////////////////////////////////////////////////////////////////////
// PSGeneralPageWndProc
/////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK PSGeneralPageWndProc
(
    HWND    hWnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    switch (message)
    {
        case WM_NOTIFY:
        {
            switch (((NMHDR *)lParam)->code)
            {
                case NM_CUSTOMDRAW:
                {
                    NMLVCUSTOMDRAW * plvcd = (NMLVCUSTOMDRAW *)lParam;

                    switch (plvcd->nmcd.dwDrawStage)
                    {
                        case CDDS_PREPAINT:
                        {
                            return CDRF_NOTIFYITEMDRAW;
                        }

                        case CDDS_ITEMPREPAINT:
                        {
                            return CDRF_DODEFAULT;
                        }

                        default:
                            break;
                    }

                    break;
                }

                default:
                    break;
            }

            break;
        }


        default:
            break;
    }

    return CallWindowProc((WNDPROC)m_pfnOldGeneralPSDlgProc, hWnd, message, wParam, lParam);
}
#endif  // FEATURE_CUSTOMDRAWIMAGES

/////////////////////////////////////////////////////////////////////////////
// PSGeneralPage_OnInitDialog
/////////////////////////////////////////////////////////////////////////////
BOOL PSGeneralPage_OnInitDialog
(
    HWND    hWnd,
    HWND    hwndFocus,
    LPARAM  lParam
)
{
    CActiveScreenSaver * pScreenSaver = (CActiveScreenSaver *)lParam;
    IScreenSaverConfig * pSSCfg = SAFECAST(pScreenSaver, IScreenSaverConfig *);

    HWND hwndList = GetDlgItem(hWnd, IDC_SUBSCRIPTION_LIST);
    TreeView_SetImageList(hwndList, pScreenSaver->m_hImgList, 0);

    // Add the subscriptions to the list
    DWORD dwFeatureFlags;
    EVAL(SUCCEEDED(pSSCfg->get_Features(&dwFeatureFlags)));
    EnumChannels(   ChannelAddToTreeProc,
                    (dwFeatureFlags & FEATURE_USE_CDF_TOPLEVEL_URL),
                    (LPARAM)hwndList);

    // Channel Time
    int nChannelTime;
    EVAL(SUCCEEDED(pSSCfg->get_ChannelTime(&nChannelTime)));
    SetDlgItemInt(hWnd, IDC_CHANNEL_TIME, nChannelTime, TRUE);
    UpDown_SetRange(GetDlgItem(hWnd, IDC_CHANNEL_TIME_SPIN), MIN_CHANNEL_TIME_SECONDS, MAX_CHANNEL_TIME_SECONDS);

    // Set the Play Sounds flag
    VARIANT_BOOL bVal;
    EVAL(SUCCEEDED(pSSCfg->get_PlaySounds(&bVal)));
    CheckDlgButton(hWnd, IDC_PLAY_SOUNDS, (bVal == VARIANT_TRUE));

    // Navigate on Click
    EVAL(SUCCEEDED(pSSCfg->get_NavigateOnClick(&bVal)));
    HWND hCtrl = ((bVal == VARIANT_TRUE) ? GetDlgItem(hWnd, IDC_NAVIGATE_CLICK)
                                         : GetDlgItem(hWnd, IDC_NAVIGATE_ALTCLICK));
    Button_SetCheck(hCtrl, TRUE);

    // Indicate that we have not changed the property sheet yet.
    PropSheet_UnChanged(GetParent(hWnd), hWnd);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// PSGeneralPage_OnCommand
/////////////////////////////////////////////////////////////////////////////
BOOL PSGeneralPage_OnCommand
(
    HWND    hWnd,
    int     id,
    HWND    hwndCtl,
    UINT    codeNotify
)
{
    switch (id)
    {
        // For the following controls we only have to indicate that the sheet has changed.
        case IDC_CHANNEL_TIME:
        {
            switch (codeNotify)
            {
                case EN_CHANGE:
                {
                    // Indicate the sheet has changed
                    PropSheet_Changed(GetParent(hWnd), hWnd);
                    break;
                }

                case EN_KILLFOCUS:
                {
                    BOOL    bTranslated;
                    int     nCurrent;
                    int     nRestricted;

                    // Channel Time
                    nCurrent = (int)GetDlgItemInt(hWnd, IDC_CHANNEL_TIME, &bTranslated, TRUE);

                    if (bTranslated)
                    {
                        nRestricted = min(MAX_CHANNEL_TIME_SECONDS, max(MIN_CHANNEL_TIME_SECONDS, nCurrent));

                        if (nRestricted != nCurrent)
                            SetDlgItemInt(hWnd, IDC_CHANNEL_TIME, nRestricted, TRUE);
                    }
                    else
                        SetDlgItemInt(hWnd, IDC_CHANNEL_TIME, DEFAULT_CHANNEL_TIME, TRUE);

                    break;
                }

                default:
                    break;
            }

            break;
        }

#ifdef FEATURE_FONT_SETTINGS        
        case IDC_FONT_SETTINGS:
        {
            HINSTANCE hInstINetCPL = NULL;
            FONTSPROC pfnOpenFontsDialog;
    
            if  (
                ((hInstINetCPL = LoadLibrary(STR_INETCPL)) != NULL)
                &&
                ((pfnOpenFontsDialog = (FONTSPROC)GetProcAddress(   hInstINetCPL,
                                                                    TEXT("OpenFontsDialog"))) != NULL)
                )
            {
                pfnOpenFontsDialog(hWnd, NULL);
            }
            else
                MessageBeep(0);

            if (hInstINetCPL != NULL)
                FreeLibrary(hInstINetCPL);
            
            break;
        }
#endif  // FEATURE_FONT_SETTINGS

        default:
            break;
    }

    FORWARD_WM_COMMAND(hWnd, id, hwndCtl, codeNotify, DefWindowProc);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// PSGeneralPage_OnApply
/////////////////////////////////////////////////////////////////////////////
void PSGeneralPage_OnApply
(
    HWND    hDlg,
    LPARAM  lParam
)
{
    CActiveScreenSaver *    pScreenSaver = (CActiveScreenSaver *)lParam;
    IScreenSaverConfig *    pSSCfg = SAFECAST(pScreenSaver, IScreenSaverConfig *);
    ISubscriptionMgr *      pSubscriptionMgr = NULL;

    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Channel selections
    for (;;)
    {
        HWND hwndTree;
        if ((hwndTree = GetDlgItem(hDlg, IDC_SUBSCRIPTION_LIST)) == NULL)
        {
            ASSERT(FALSE);
            break;
        }

        EVAL(SUCCEEDED(CoCreateInstance(CLSID_SubscriptionMgr,
                                        NULL,
                                        CLSCTX_INPROC_SERVER,
                                        IID_ISubscriptionMgr,
                                        (void **)&pSubscriptionMgr)));

        TV_ITEM tvi = { 0 };

        tvi.hItem = TreeView_GetRoot(hwndTree);
        tvi.mask = TVIF_IMAGE | TVIF_PARAM;

        BOOL bAlreadyAsked = FALSE;
        BOOL bDontCreateNewSubs = FALSE;

        while (tvi.hItem != NULL)
        {
            if (TreeView_GetItem(hwndTree, &tvi))
            {
                TVCHANNELDATA * pChannelData = (TVCHANNELDATA *)tvi.lParam;
                VARIANT         vFlags = { 0 };
                BOOL            bUpdateSubs = FALSE;

                ASSERT(pChannelData != NULL);

                // See if the selection of the item has changed to 'selected'.
                if ((tvi.iImage == (IDB_CHECKED - IDB_IMGLIST_FIRST)))
                {
                    if (pChannelData->pSubsItem == NULL)
                    {
                        // No Subscription, must have changed.
                        if (!bDontCreateNewSubs)
                        {
                            if (!bAlreadyAsked)
                            {
                                // See if the user wants to be asked about subscribing
                                if (ReadRegDWORD(   HKEY_CURRENT_USER,
                                                    g_szRegSubKey,
                                                    s_szDontAskCreateSubs,
                                                    BST_UNCHECKED) != BST_CHECKED)
                                {
                                    // Ask user if they want to create subscriptions
                                    if (DialogBoxParam( _pModule->GetResourceInstance(),
                                                        MAKEINTRESOURCE(IDD_CREATESUBS_DIALOG),
                                                        hDlg,
                                                        CreateSubscriptionDlgProc,
                                                        NULL) != IDOK)
                                    {
                                        bDontCreateNewSubs = TRUE;
                                    }
                                }

                                bAlreadyAsked = TRUE;
                            }

                            if  (
                                !bDontCreateNewSubs
                                &&
                                SUCCEEDED(SubscribeToChannelForScreenSaver(hDlg, pChannelData))
                                )
                            {
                                // NOTE: Assume that SubscribeToChannelForScreenSaver
                                // sets CHANNEL_AGENT_PRECACHE_SCRNSAVER

                                // Update subscription.
                                bUpdateSubs = TRUE;
                            }
                        }
                    }
                    else
                    {
                        if  (
                            // Check to see of the screen saver flag changed.
                            SUCCEEDED(pChannelData->pSubsItem->ReadProperties(1, 
                                                        &g_pProps[PROP_CHANNEL_FLAGS], &vFlags))
                            &&
                            (V_VT(&vFlags) == VT_I4)
                            &&
                            !(V_I4(&vFlags) & CHANNEL_AGENT_PRECACHE_SCRNSAVER)
                            )
                        {
                            V_I4(&vFlags) |= CHANNEL_AGENT_PRECACHE_SCRNSAVER;
                            EVAL(SUCCEEDED(pChannelData->pSubsItem->WriteProperties(1, 
                                                        &g_pProps[PROP_CHANNEL_FLAGS], &vFlags)));

                            // Update subscription.
                            bUpdateSubs = TRUE;
                        }
                        VariantClear(&vFlags);
                    }
                }
                else    // Unchecked
                {
                    // Check to see of the screen saver flag changed.
                    if  (
                        (pChannelData->pSubsItem != NULL)
                        &&
                        SUCCEEDED(pChannelData->pSubsItem->ReadProperties(1, 
                                                    &g_pProps[PROP_CHANNEL_FLAGS], &vFlags))
                        &&
                        (V_VT(&vFlags) == VT_I4)
                        &&
                        (V_I4(&vFlags) & CHANNEL_AGENT_PRECACHE_SCRNSAVER)
                        )
                    {
                        V_I4(&vFlags) &= ~CHANNEL_AGENT_PRECACHE_SCRNSAVER;
                            EVAL(SUCCEEDED(pChannelData->pSubsItem->WriteProperties(1, 
                                                        &g_pProps[PROP_CHANNEL_FLAGS], &vFlags)));

                        // REVIEW: Remove subscription?
                    }
                    VariantClear(&vFlags);
                }

                if (bUpdateSubs)
                {
                    BSTR bstrURL = pChannelData->strURL.AllocSysString();
                    if (bstrURL)
                    {
                        EVAL(SUCCEEDED(pSubscriptionMgr->UpdateSubscription(bstrURL)));
                        SysFreeString(bstrURL);
                    }
                }

                VariantClear(&vFlags);
            }
            else
                TraceMsg(TF_ERROR, "TreeView_GetItem() FAILED!");

            tvi.hItem = TreeView_GetNextItem(hwndTree, tvi.hItem, TVGN_NEXT);
        }

        break;
    }

    if (pSubscriptionMgr != NULL)
        EVAL(pSubscriptionMgr->Release() == 0);

    // Channel Time
    BOOL bTranslated;
    int nChannelTime = (int)GetDlgItemInt(hDlg, IDC_CHANNEL_TIME, &bTranslated, TRUE);
    EVAL(SUCCEEDED(pSSCfg->put_ChannelTime((bTranslated ? min(MAX_CHANNEL_TIME_SECONDS, max(MIN_CHANNEL_TIME_SECONDS, nChannelTime))
                                                        : DEFAULT_CHANNEL_TIME))));

    // Play Sounds
    EVAL(SUCCEEDED(pSSCfg->put_PlaySounds((IsDlgButtonChecked(hDlg, IDC_PLAY_SOUNDS)
                                                ? VARIANT_TRUE
                                                : VARIANT_FALSE))));

    // Navigate on Click
    EVAL(SUCCEEDED(pSSCfg->put_NavigateOnClick((IsDlgButtonChecked(hDlg, IDC_NAVIGATE_CLICK)
                                                    ? VARIANT_TRUE
                                                    : VARIANT_FALSE))));

    pSSCfg->Apply();

    if (hOldCursor != NULL)
        SetCursor(hOldCursor);
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// ChannelAddToTreeProc
/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK ChannelAddToTreeProc
(
    ISubscriptionMgr2 *  pSubscriptionMgr2,
    CHANNELENUMINFO *   pci,
    int                 nItemNum,
    BOOL                bDefaultTopLevelURL,
    LPARAM              lParam
)
{
    CString         strCDF;
    TVCHANNELDATA * pChannelData = new TVCHANNELDATA;
    BOOL            bResult = FALSE;
    BOOL            bEnabled = FALSE;

    if (NULL == pChannelData)
        return FALSE;

    OLE2T(pci->pszURL, lpszURL);
    strCDF = lpszURL;

    TraceMsg(TF_ALWAYS, "CATTP: pszURL = %s", (LPCTSTR)strCDF);

    if (!pChannelData)
    {
        return FALSE;
    }

    for (;;)
    {
        OLE2T(pci->pszPath, pszPath);
        TCHAR szDesktopINI[MAX_PATH];
        TCHAR szScreenSaverURL[INTERNET_MAX_URL_LENGTH];

        PathCombine(szDesktopINI, pszPath, g_szDesktopINI);

        if (GetPrivateProfileString(g_szChannel, 
                                    g_szScreenSaverURL, 
                                    TEXT(""), 
                                    szScreenSaverURL,
                                    ARRAYSIZE(szScreenSaverURL),
                                    szDesktopINI) == 0)
        {
            break;
        }

        HRESULT hr = pSubscriptionMgr2->GetItemFromURL(pci->pszURL, &pChannelData->pSubsItem);

        ASSERT((SUCCEEDED(hr) && pChannelData->pSubsItem) ||
               (FAILED(hr) && !pChannelData->pSubsItem));

        VARIANT vFlags = { 0 };

        if (SUCCEEDED(hr) && 
            SUCCEEDED(pChannelData->pSubsItem->ReadProperties(1, 
                      &g_pProps[PROP_CHANNEL_FLAGS], &vFlags)) &&
            (V_VT(&vFlags) == VT_I4) &&
            (V_I4(&vFlags) & CHANNEL_AGENT_PRECACHE_SCRNSAVER))
        {
            bEnabled = TRUE;
        }
               
               
        // Make sure task trigger is empty.
        ZeroMemory(&(pChannelData->tt), SIZEOF(TASK_TRIGGER));

        CString strTitle;
        OLE2T(pci->pszTitle, pszTitle);
        strTitle = pszTitle;

        pChannelData->strURL            = strCDF;
        pChannelData->strTitle          = strTitle;

        TV_INSERTSTRUCT tvInsert = { 0 };
        tvInsert.hParent                = TVI_ROOT;
        tvInsert.hInsertAfter           = TVI_SORT;

        tvInsert.item.mask              = TVIF_TEXT
                                            | TVIF_STATE
                                            | TVIF_IMAGE
                                            | TVIF_SELECTEDIMAGE
                                            | TVIF_PARAM;

        tvInsert.item.hItem             = TVI_FIRST;
        tvInsert.item.pszText           = strTitle;
        tvInsert.item.cchTextMax        = strTitle.GetLength();
        // tvInsert.item.iImage            = 0;
        // tvInsert.item.iSelectedImage    = 0;

        tvInsert.item.lParam = (LPARAM)pChannelData;

        // Check to see if the user has this channel screen saver active.
        if  (!bEnabled)
        {
            tvInsert.item.iImage            = IDB_UNCHECKED-IDB_IMGLIST_FIRST;
            tvInsert.item.iSelectedImage    = IDB_UNCHECKED-IDB_IMGLIST_FIRST;
        }

        // Add the item.
        if (TreeView_InsertItem((HWND)lParam, &tvInsert) == NULL)
            break;

        bResult = TRUE;
        break;
    }

    if (!bResult)
        delete pChannelData;

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// LoadImageList
/////////////////////////////////////////////////////////////////////////////
HIMAGELIST LoadImageList
(
)
{
    HIMAGELIST  hImgList = NULL;
    BOOL        bResult = FALSE;

    if ((hImgList = ImageList_Create(   IMGLIST_IMAGE_WIDTH,
                                        IMGLIST_IMAGE_HEIGHT,
                                        ILC_COLOR,
                                        IMGLIST_NUM_IMAGES, 1)) != NULL)
    {
        bResult = TRUE;

        for (int i = 0; i < IMGLIST_NUM_IMAGES; i++)
        {
            HBITMAP hBitmap;

            if ((hBitmap = (HBITMAP)LoadImage(  _pModule->GetResourceInstance(),
                                                MAKEINTRESOURCE(IDB_IMGLIST_FIRST+i),
                                                IMAGE_BITMAP,
                                                IMGLIST_IMAGE_WIDTH,
                                                IMGLIST_IMAGE_HEIGHT,
                                                LR_LOADTRANSPARENT)) != NULL)
            {
                EVAL(ImageList_Add(hImgList, hBitmap, NULL) != -1);
                DeleteObject(hBitmap);
            }
            else
            {
                bResult = FALSE;
                break;
            }
        }
    }
    else
        bResult = FALSE;

    // Cleanup on error
    if (!bResult && (hImgList != NULL))
    {
        EVAL(ImageList_Destroy(hImgList));        
        hImgList = NULL;
    }

    return hImgList;
}

/////////////////////////////////////////////////////////////////////////////
// ToggleItemCheck
/////////////////////////////////////////////////////////////////////////////
void ToggleItemCheck
(
    HWND        hWnd,
    HWND        hwndTree,
    HTREEITEM   hti
)
{
    TV_ITEM tvi = { 0 };

    tvi.hItem   = hti;
    tvi.mask    = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

    TreeView_GetItem(hwndTree, &tvi);

    // By default we need to atleast change the image.
    tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;

    switch (tvi.iImage+IDB_IMGLIST_FIRST)
    {
        case IDB_CHECKED:
        {
            tvi.iImage         = IDB_UNCHECKED-IDB_IMGLIST_FIRST;
            tvi.iSelectedImage = IDB_UNCHECKED-IDB_IMGLIST_FIRST;

            break;
        }

        case IDB_UNCHECKED:
        {
            tvi.iImage         = IDB_CHECKED-IDB_IMGLIST_FIRST;
            tvi.iSelectedImage = IDB_CHECKED-IDB_IMGLIST_FIRST;

            break;
        }

        default:
        {
            ASSERT(FALSE);
            break;
        }
    }

    TreeView_SetItem(hwndTree, &tvi);
}

/////////////////////////////////////////////////////////////////////////////
// SubscribeToChannelForScreenSaver
/////////////////////////////////////////////////////////////////////////////
HRESULT SubscribeToChannelForScreenSaver
(
    HWND            hwnd,
    TVCHANNELDATA * pChannelData
)
{
    ISubscriptionMgr *  pSubscriptionMgr = NULL;
    BSTR                bstrURL = pChannelData->strURL.AllocSysString();
    BSTR                bstrTitle = pChannelData->strTitle.AllocSysString();
    HRESULT             hrResult;

    if (bstrURL && bstrTitle)
    {
        for (;;)
        {
            
            if (FAILED(hrResult = CoCreateInstance( CLSID_SubscriptionMgr,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_ISubscriptionMgr,
                (void **)&pSubscriptionMgr)))
            {
                break;
            }
            
            TASK_TRIGGER        tt = { 0 };
            SUBSCRIPTIONINFO    si = { 0 };
            si.cbSize = SIZEOF(si);
            
            if (pChannelData->tt.cbTriggerSize != 0)
                si.pTrigger = &(pChannelData->tt);
            
            // If we created, only precache screen saver content.
            si.fUpdateFlags         = SUBSINFO_SCHEDULE
                | SUBSINFO_CHANNELFLAGS;
            
            si.schedule             = SUBSSCHED_AUTO;
            
            si.fChannelFlags        = CHANNEL_AGENT_PRECACHE_SCRNSAVER
                | CHANNEL_AGENT_PRECACHE_SOME;
            // | CHANNEL_AGENT_DYNAMIC_SCHEDULE;
            
            if (FAILED(hrResult = pSubscriptionMgr->CreateSubscription( hwnd,
                bstrURL,
                bstrTitle,
                CREATESUBS_FROMFAVORITES
                | CREATESUBS_NOUI,
                SUBSTYPE_CHANNEL,
                &si)))
            {
                break;
            }
            
            break;
        }
    }
    else
        hrResult = E_OUTOFMEMORY;
        
    // Cleanup
    if (bstrURL != NULL)
        SysFreeString(bstrURL);

    if (bstrTitle != NULL)
        SysFreeString(bstrTitle);

    if (pSubscriptionMgr != NULL)
        EVAL(pSubscriptionMgr->Release() == 0);

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CreateSubscriptionDlgProc
/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK CreateSubscriptionDlgProc
(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    switch (uMsg)
    {
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                {
                    DWORD dwChecked = IsDlgButtonChecked(hDlg, IDC_DONTASKAGAIN);
                    WriteRegValue(  HKEY_CURRENT_USER,
                                    g_szRegSubKey,
                                    s_szDontAskCreateSubs,
                                    REG_DWORD,
                                    (BYTE *)&dwChecked,
                                    sizeof(DWORD));

                    EndDialog(hDlg, wParam);
                    break;
                }

                default:
                    return FALSE;
            }

            break;
        }

        default:
            return FALSE;
    }

    return TRUE;
}
