#include "ctlspriv.h"
#include <prsht.h>
#ifndef IEWIN31
#include <commctrl.h>           // For the hotkey hack.
#endif
#include <prshtp.h>
#include "..\inc\help.h" // Help IDs

#ifndef WIN31
#define g_cxSmIcon  GetSystemMetrics(SM_CXSMICON)
#define g_cySmIcon  GetSystemMetrics(SM_CYSMICON)
#endif

#define FLAG_CHANGED    0x0001

LPVOID WINAPI MapSLFix(HANDLE);
VOID WINAPI UnMapSLFixArray(int, HANDLE *);

// to avoid warnings....
#ifdef WIN32
#define HWNDTOLONG(hwnd) (LONG)(hwnd)
#else
#define HWNDTOLONG(hwnd) MAKELONG(hwnd,0)
#endif

#define RECTWIDTH(rc) (rc.right - rc.left)
#define RECTHEIGHT(rc) (rc.bottom - rc.top)

#ifdef WIN31
LRESULT CALLBACK Win31PropPageWndProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
LRESULT NEAR PASCAL Win31OnCtlColor(HWND hDlg, HDC hdcChild, HWND hwndChild, int nCtlType);
BOOL    NEAR PASCAL Win31MakeDlgNonBold(HWND hDlg);
BOOL    FAR  PASCAL Win31IsKeyMessage(HWND hwndDlg, LPMSG lpmsg);
VOID    NEAR PASCAL CenterDialog ( HWND hdlg );
void    NEAR PASCAL Win31RepositionDlg( HWND hDlg );
void    FAR  PASCAL RemoveDefaultButton(HWND hwndRoot, HWND hwndStart);

const char g_szNonBoldFont[] = "DS_3DLOOK";
const char g_szLoSubclass[] = "MS_LO_SUBCLASS";
const char g_szHiSubclass[] = "MS_HI_SUBCLASS";

#endif

#if  !defined(WIN32) && !defined(WIN31)
#ifdef FE_IME
typedef void *PVOID;
DWORD WINAPI GetCurrentThreadID(VOID);
BOOL WINAPI  CreateImmTLink(DWORD);
#endif
#endif

void    NEAR PASCAL ResetWizButtons(LPPROPDATA ppd);

typedef struct  // tie
{
    TC_ITEMHEADER   tci;
    HWND            hwndPage;
    UINT            state;
} TC_ITEMEXTRA;

#define CB_ITEMEXTRA (sizeof(TC_ITEMEXTRA) - sizeof(TC_ITEMHEADER))

void NEAR PASCAL PageChange(LPPROPDATA ppd, int iAutoAdj);

HWND WINAPI CreatePage(PSP FAR *hpage, HWND hwndParent);
BOOL WINAPI GetPageInfo(PSP FAR *hpage, LPSTR pszCaption, int cbCaption, LPPOINT ppt, HICON FAR *hIcon);;

//
// IMPORTANT:  The IDHELP ID should always be LAST since we just subtract
// 1 from the number of IDs if no help in the page.
// IDD_APPLYNOW should always be the FIRST ID for standard IDs since it
// is sometimes not displayed and we'll start with index 1.
//
#pragma data_seg(DATASEG_READONLY)
static int IDs[] = {IDD_APPLYNOW, IDOK, IDCANCEL, IDHELP};
static int WizIDs[] = {IDD_BACK, IDD_NEXT, IDD_FINISH, IDCANCEL, IDHELP};
#pragma data_seg()

void NEAR PASCAL _SetTitle(HWND hDlg, LPPROPDATA ppd)
{
    char szFormat[50];
    char szTitle[128];
    char szTemp[128 + 50];
    LPCSTR pCaption = ppd->psh.pszCaption;

    if (HIWORD(pCaption) == 0) {
    LoadString(ppd->psh.hInstance, (UINT)LOWORD(pCaption), szTitle, sizeof(szTitle));
    pCaption = (LPCSTR)szTitle;
    }

    if (ppd->psh.dwFlags & PSH_PROPTITLE) {
    LoadString(HINST_THISDLL, IDS_PROPERTIESFOR, szFormat, sizeof(szFormat));
    if ((lstrlen(pCaption) + 1 + lstrlen(szFormat) + 1) < sizeof(szTemp)) {
        wsprintf(szTemp, szFormat, pCaption);
        pCaption = szTemp;
    }
    }

    SetWindowText(hDlg, pCaption);
}

int NEAR PASCAL RemoveButton(HWND hDlg, int idRemove, int idPrev)
{
    HWND hRemove = GetDlgItem(hDlg, idRemove);
    HWND hPrev = GetDlgItem(hDlg, idPrev);
    RECT rcRemove, rcPrev;

    GetWindowRect(hRemove, &rcRemove);
    GetWindowRect(hPrev, &rcPrev);

    ShowWindow(hRemove, SW_HIDE);

    return(rcRemove.right - rcPrev.right);
}



void NEAR PASCAL InitPropSheetDlg(HWND hDlg, LPPROPDATA ppd)
{
    char szTemp[128 + 50];
    int dxMax, dyMax, dxDlg, dyDlg, dyGrow, dxGrow;
    RECT rcMinSize, rcDlg, rcPage, rcOrigCtrl;
    int dxApplyButton = 0;
    UINT uPages;
#ifdef WIN32
    HIMAGELIST himl = NULL;
#endif
    TC_ITEMEXTRA tie;
    char szStartPage[128];
    LPCSTR pStartPage;
    UINT nStartPage;
#ifdef DEBUG
    BOOL fStartPageFound = FALSE;
#endif

    // set our instance data pointer
    SetWindowLong(hDlg, DWL_USER, (LONG)ppd);

    // Make sure this gets inited early on.
    ppd->nCurItem = 0;

    // do this here instead of using DS_SETFOREGROUND so we don't hose
    // pages that do things that want to set the foreground window

#ifdef WIN31
    if (GetWindowStyle(hDlg) & DS_3DLOOK)
        Win31MakeDlgNonBold(hDlg);
#endif

    if (!(ppd->psh.dwFlags & PSH_WIZARD)) {
        _SetTitle(hDlg, ppd);
    }

    if (ppd->psh.dwFlags & PSH_USEICONID)
    {
#ifndef WIN31
    ppd->psh.hIcon = LoadImage(ppd->psh.hInstance, ppd->psh.pszIcon, IMAGE_ICON, g_cxSmIcon, g_cySmIcon, LR_DEFAULTCOLOR);
#else
    ppd->psh.hIcon = NULL;
#endif
    }

    if ((ppd->psh.dwFlags & (PSH_USEICONID | PSH_USEHICON)) && ppd->psh.hIcon)
    SendMessage(hDlg, WM_SETICON, FALSE, (LPARAM)(UINT)ppd->psh.hIcon);

    ppd->hDlg = hDlg;
    ppd->hwndTabs = GetDlgItem(hDlg, IDD_PAGELIST);
    TabCtrl_SetItemExtra(ppd->hwndTabs, CB_ITEMEXTRA);

    // nStartPage is either ppd->psh.nStartPage or the page pStartPage
    nStartPage = ppd->psh.nStartPage;
    if (ppd->psh.dwFlags & PSH_USEPSTARTPAGE)
    {
    pStartPage = ppd->psh.pStartPage;
    nStartPage = 0; // default page if pStartPage not found

    if (!HIWORD(pStartPage))
    {
        szTemp[0] = '\0';
        LoadString(ppd->psh.hInstance, (UINT)LOWORD(pStartPage),
            szStartPage, sizeof(szStartPage));
        pStartPage = (LPCSTR)szTemp;
    }
    }

    dxMax = dyMax = 0;

    tie.tci.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
    tie.hwndPage = NULL;
    tie.tci.pszText = szTemp;
    tie.state = 0;

    SendMessage(ppd->hwndTabs, WM_SETREDRAW, FALSE, 0L);

    for (uPages = 0; uPages < ppd->psh.nPages; uPages++)
    {
    POINT pt;
    HICON hIcon = NULL;

    if (GetPageInfo(ppd->psh.phpage[uPages], szTemp, sizeof(szTemp), &pt, &hIcon))
    {
        // Add the page to the end of the tab list

        tie.tci.iImage = -1;
        if (hIcon) {
#ifdef WIN32
                if (!himl) {
                    himl = ImageList_Create(g_cxSmIcon, g_cySmIcon, TRUE, 8, 4);
                    TabCtrl_SetImageList(ppd->hwndTabs, himl);
                }

        tie.tci.iImage = ImageList_AddIcon(himl, hIcon);
#endif
        DestroyIcon(hIcon);
        }

        // BUGBUG? What if this fails? Do we want to destroy the page?
        if (TabCtrl_InsertItem(ppd->hwndTabs, 1000, &tie.tci) >= 0)
        {
        if (dxMax < pt.x)
            dxMax = pt.x;
        if (dyMax < pt.y)
            dyMax = pt.y;
        }

        // if the user is specifying the startpage via title, check it here
        if (ppd->psh.dwFlags & PSH_USEPSTARTPAGE &&
        !lstrcmpi(pStartPage, szTemp))
        {
        nStartPage = uPages;
#ifdef DEBUG
        fStartPageFound = TRUE;
#endif
        }
    }
    else
    {
        // Destroy this hpage and move all the rest down.
        // Do this because if we don't, then things get confused (GPF).
        // This is safe to do since we call DestroyPropSheetPage
        // in PropertySheet.  (Of course there we do it in inverse
        // order, and here it just happens on error... Oh well...)

        DebugMsg(DM_ERROR, "PropertySheet failed to GetPageInfo");

        DestroyPropertySheetPage(ppd->psh.phpage[uPages]);
        hmemcpy(&(ppd->psh.phpage[uPages]),
            &(ppd->psh.phpage[uPages+1]),
            sizeof(HPROPSHEETPAGE) * (ppd->psh.nPages - uPages - 1));
        uPages--;
        ppd->psh.nPages--;
    }
    }

    SendMessage(ppd->hwndTabs, WM_SETREDRAW, TRUE, 0L);

    if (ppd->psh.pfnCallback) {
        ppd->psh.pfnCallback(hDlg, PSCB_INITIALIZED, 0);
    }

    //
    // Now compute the size of the tab control.
    //

    // Compute rcPage = Size of page area in pixels
    rcPage.left = rcPage.top = 0;
    rcPage.right = dxMax;
    rcPage.bottom = dyMax;
    MapDialogRect(hDlg, &rcPage);

    // Get the size of the pagelist control in pixels.
    GetClientRect(ppd->hwndTabs, &rcOrigCtrl);

    // Now compute the minimum size for the page region
    rcMinSize = rcOrigCtrl;
    if (rcMinSize.right < rcPage.right)
    rcMinSize.right = rcPage.right;
    if (rcMinSize.bottom < rcPage.bottom)
    rcMinSize.bottom = rcPage.bottom;

    //
    //  If this is a wizard then set the size of the page area to the entire
    //  size of the control.  If it is a normal property sheet then adjust for
    //  the tabs, resize the control, and then compute the size of the page
    //  region only.
    //
    if (ppd->psh.dwFlags & PSH_WIZARD) {
        // initialize
    rcPage = rcMinSize;
    } else {
    int i;
    RECT rcAdjSize;

        // initialize

    for (i = 0; i < 2; i++) {
        rcAdjSize = rcMinSize;
        TabCtrl_AdjustRect(ppd->hwndTabs, TRUE, &rcAdjSize);

        rcAdjSize.right  -= rcAdjSize.left;
        rcAdjSize.bottom -= rcAdjSize.top;
        rcAdjSize.left = rcAdjSize.top = 0;

        if (rcAdjSize.right < rcMinSize.right)
        rcAdjSize.right = rcMinSize.right;
        if (rcAdjSize.bottom < rcMinSize.bottom)
        rcAdjSize.bottom = rcMinSize.bottom;

        SetWindowPos(ppd->hwndTabs, NULL, 0,0, rcAdjSize.right, rcAdjSize.bottom,
             SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    rcPage = rcMinSize = rcAdjSize;
    TabCtrl_AdjustRect(ppd->hwndTabs, FALSE, &rcPage);
    }
    //
    // rcMinSize now contains the size of the control, including the tabs, and
    // rcPage is the rect containing the page portion (without the tabs).
    //

    //
    // Resize the dialog to make room for the control's new size.  This can
    // only grow the size.
    //
    GetWindowRect(hDlg, &rcDlg);
    dxGrow = rcMinSize.right - rcOrigCtrl.right;
    dxDlg  = rcDlg.right - rcDlg.left + dxGrow;
    dyGrow = rcMinSize.bottom - rcOrigCtrl.bottom;
    dyDlg  = rcDlg.bottom - rcDlg.top + dyGrow;

//
// Cascade property sheet windows (only for comctl32 and commctrl)
//
#ifndef WIN31
    //
    // HACK: Putting CW_USEDEFAULT in dialog template does not work because
    //  CreateWindowEx ignores it unless the window has WS_OVERLAPPED, which
    //  is not appropriate for a property sheet.
    //
    {
#pragma data_seg(DATASEG_READONLY)
    const char c_szStatic[] = "Static";
#pragma data_seg()
    UINT swp = SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE;

        if (!IsWindow(ppd->psh.hwndParent)) {
            HWND hwndT = CreateWindowEx(0, c_szStatic, NULL,
                                   WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT,
                                   0, 0, NULL, NULL, HINST_THISDLL, NULL);
            if (hwndT) {
                GetWindowRect(hwndT, &rcDlg);
                swp = SWP_NOZORDER | SWP_NOACTIVATE;
                DestroyWindow(hwndT);
            }
        } else {
            GetWindowRect(ppd->psh.hwndParent, &rcDlg);
            if (IsWindowVisible(ppd->psh.hwndParent)) {
                rcDlg.top += g_cySmIcon;
                rcDlg.left += g_cxSmIcon;
            }
            swp = SWP_NOZORDER | SWP_NOACTIVATE;
        }
    SetWindowPos(hDlg, NULL, rcDlg.left, rcDlg.top, dxDlg, dyDlg, swp);
    }
#else
    SetWindowPos(hDlg, NULL, 0, 0, dxDlg, dyDlg, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif

    // Now we'll figure out where the page needs to start relative
    // to the bottom of the tabs.
    MapWindowPoints(ppd->hwndTabs, hDlg, (LPPOINT)&rcPage, 2);

    ppd->xSubDlg  = rcPage.left;
    ppd->ySubDlg  = rcPage.top;
    ppd->cxSubDlg = rcPage.right - rcPage.left;
    ppd->cySubDlg = rcPage.bottom - rcPage.top;

    //
    // move all the buttons down as needed and turn on appropriate buttons
    // for a wizard.
    //
    {
        RECT rcCtrl;
        HWND hCtrl;
        int i, numids;
        int iStartID = 0;


        if (ppd->psh.dwFlags & PSH_WIZARD) {
            numids = ARRAYSIZE(WizIDs);

            hCtrl = GetDlgItem(hDlg, IDD_DIVIDER);
            GetWindowRect(hCtrl, &rcCtrl);
            MapWindowRect(NULL, hDlg, &rcCtrl);
            SetWindowPos(hCtrl, NULL, rcCtrl.left, rcCtrl.top + dyGrow,
                        RECTWIDTH(rcCtrl) + dxGrow, RECTHEIGHT(rcCtrl),
                        SWP_NOZORDER | SWP_NOACTIVATE);

            EnableWindow(GetDlgItem(hDlg, IDD_BACK), TRUE);
            ShowWindow(GetDlgItem(hDlg, IDD_FINISH), SW_HIDE);
            ppd->idDefaultFallback = IDD_NEXT;
        } else {
            numids = ARRAYSIZE(IDs);
            ppd->idDefaultFallback = IDOK;
        }

    // If there's no help, then remove the help button.
    if (!(ppd->psh.dwFlags & PSH_HASHELP)) {
        numids--;
        dxGrow += RemoveButton(hDlg, IDHELP,
             (ppd->psh.dwFlags & PSH_WIZARD) ? IDCANCEL : IDD_APPLYNOW);
    }

        // If we are not a wizard, and we should NOT show apply now
        if ((ppd->psh.dwFlags & PSH_NOAPPLYNOW) &&
            !(ppd->psh.dwFlags & PSH_WIZARD))
    {
        dxApplyButton = RemoveButton(hDlg, IDD_APPLYNOW, IDCANCEL);
            iStartID = 1;
    }

    for (i = iStartID; i < numids; ++i)
    {
        int iCtrl = (ppd->psh.dwFlags & PSH_WIZARD) ? WizIDs[i] : IDs[i];
        hCtrl = GetDlgItem(hDlg, iCtrl);
        GetWindowRect(hCtrl, &rcCtrl);
        ScreenToClient(hDlg, (LPPOINT)&rcCtrl);
        //
        //  Relies on the fact that IDHELP is last in the array.  This is to
        //  properly adjust for sheets that have help, but no apply button.
        //
        if (iCtrl == IDHELP) {
        dxApplyButton = 0;
        }
        SetWindowPos(hCtrl, NULL, rcCtrl.left + dxGrow + dxApplyButton,
                         rcCtrl.top + dyGrow, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    // force the dialog to reposition itself based on its new size
#ifdef WIN31
    if (ppd->psh.dwFlags & PSH_CENTER)
        CenterDialog(hDlg);
    else
        Win31RepositionDlg( hDlg );
#else
    SendMessage(hDlg, DM_REPOSITION, 0, 0L);
    SetForegroundWindow(hDlg);
#endif  // WIN31

    // Now attempt to select the starting page.
    TabCtrl_SetCurSel(ppd->hwndTabs, nStartPage);
    PageChange(ppd, 1);
#ifdef DEBUG
    if (ppd->psh.dwFlags & PSH_USEPSTARTPAGE && !fStartPageFound)
    DebugMsg(DM_WARNING, "sh WN - Property start page '%s' not found.", pStartPage);
#endif

    // We set this to 1 if the user saves any changes.
    ppd->nReturn = 0;
}

HWND NEAR PASCAL _Ppd_GetPage(LPPROPDATA ppd, int nItem)
{
    if (ppd->hwndTabs)
    {
        TC_ITEMEXTRA tie;
        tie.tci.mask = TCIF_PARAM;
        TabCtrl_GetItem(ppd->hwndTabs, nItem, &tie.tci);
        return tie.hwndPage;
    }
    return NULL;
}

LRESULT NEAR PASCAL _Ppd_SendNotify(LPPROPDATA ppd, int nItem, int code, LPARAM lParam)
{
    PSHNOTIFY pshn;

    pshn.lParam = lParam;
    return SendNotify(_Ppd_GetPage(ppd,nItem), ppd->hDlg, code, (LPNMHDR)&pshn);
}

int FindPageIndex(LPPROPDATA ppd, int nCurItem, DWORD dwFind, int iAutoAdj)
{
    int nActivate;

    if (dwFind == 0) {
    nActivate = nCurItem + iAutoAdj;
    if (((UINT)nActivate) <= ppd->psh.nPages) {
        return(nActivate);
    }
    } else {
    for (nActivate = 0; (UINT)nActivate < ppd->psh.nPages; nActivate++) {
        if ((DWORD)(ppd->psh.phpage[nActivate]->psp.pszTemplate) ==
        dwFind) {
        return(nActivate);
        }
    }
    }
    return nCurItem + iAutoAdj;
}


void NEAR PASCAL SetNewDefID(LPPROPDATA ppd)
{
    HWND hDlg = ppd->hDlg;
#ifdef WIN31
    if( ppd->psh.dwFlags & PSH_WIZARD )
    {
        SetFocus(ppd->hwndCurPage);
        if( GetFocus()==NULL )
            SetFocus( GetDlgItem( hDlg, IDD_NEXT ) );
    }
#else
    HWND hwndFocus;
    hwndFocus = GetNextDlgTabItem(ppd->hwndCurPage, NULL, FALSE);
    Assert(hwndFocus);
    if (hwndFocus) {
        int id;
        if (((DWORD)SendMessage(hwndFocus, WM_GETDLGCODE, 0, 0L)) & DLGC_HASSETSEL)
        {
            // select the text
            Edit_SetSel(hwndFocus, 0, -1);
        }

        id = GetDlgCtrlID(hwndFocus);
        // HACKHACK....
        // if there is no tab stop, GetDlgCtrlID will return the first item
        // (don't ask me why it doesn't return NULL
        // so we need to check it and set the focus to the tabs if it's not a tabstop
        if ((GetWindowLong(hwndFocus, GWL_STYLE) & (WS_VISIBLE | WS_DISABLED | WS_TABSTOP)) == (WS_VISIBLE | WS_TABSTOP))
            SetFocus(hwndFocus);
        else {
            // in prop sheet mode, focus on tabs,
            // in wizard mode, tabs aren't visible, go to idDefFallback
            if (ppd->psh.dwFlags & PSH_WIZARD)
                SetFocus(GetDlgItem(hDlg, ppd->idDefaultFallback));
            else
                SetFocus(ppd->hwndTabs);
        }

        ResetWizButtons(ppd);
        if (SendDlgItemMessage(ppd->hwndCurPage, id, WM_GETDLGCODE, 0, 0L) & DLGC_UNDEFPUSHBUTTON)
            SendMessage(ppd->hwndCurPage, DM_SETDEFID, id, 0);
        else {
            SendMessage(hDlg, DM_SETDEFID, ppd->idDefaultFallback, 0);
        }
    }
#endif
}

/*
** we are about to change pages.  what a nice chance to let the current
** page validate itself before we go away.  if the page decides not
** to be de-activated, then this'll cancel the page change.
**
** return TRUE iff this page failed validation
*/
BOOL NEAR PASCAL PageChanging(LPPROPDATA ppd)
{
    BOOL bRet = FALSE;
    if (ppd && ppd->hwndCurPage)
    {
    bRet = (BOOL)_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_KILLACTIVE, 0);
    }
    return bRet;
}

void NEAR PASCAL PageChange(LPPROPDATA ppd, int iAutoAdj)
{
    HWND hwndCurPage;
    HWND hwndCurFocus;
    int nItem;
    HWND hDlg, hwndTabs;
    TC_ITEMEXTRA tie;
    int FlailCount = 0;
    LRESULT lres;

#ifdef WIN31
    LRESULT (WINAPI *_DefDlgProc)(HWND, UINT, WPARAM, LPARAM);
#endif

    if (!ppd)
    {
        return;
    }

    hDlg = ppd->hDlg;
    hwndTabs = ppd->hwndTabs;

    // NOTE: the page was already validated (PSN_KILLACTIVE) before
    // the actual page change.

    hwndCurFocus = GetFocus();

TryAgain:
    FlailCount++;
    if (FlailCount > 10)
    {
        DebugMsg(DM_TRACE, "PropSheet PageChange attempt to set activation more than 10 times.");
        return;
    }

    nItem = TabCtrl_GetCurSel(hwndTabs);
    if (nItem < 0)
    {
        return;
    }

    tie.tci.mask = TCIF_PARAM;

    TabCtrl_GetItem(hwndTabs, nItem, &tie.tci);
    hwndCurPage = tie.hwndPage;

    if (!hwndCurPage)
    {
        PSP FAR *hpage = ppd->psh.phpage[nItem];

        hwndCurPage = CreatePage(hpage, hDlg);
        if (hwndCurPage)
        {
            // tie.tci.mask    = TCIF_PARAM;
            tie.hwndPage = hwndCurPage;
            TabCtrl_SetItem(hwndTabs, nItem, &tie.tci);
#ifdef WIN31
            // Subclass for proper color handling
            // SubclassWindow(hwndCurPage, Win31PropPageWndProc);
            _DefDlgProc = SubclassWindow(hwndCurPage, Win31PropPageWndProc);
            SetProp(hwndCurPage,g_szLoSubclass,(HANDLE)LOWORD(_DefDlgProc));
            SetProp(hwndCurPage,g_szHiSubclass,(HANDLE)HIWORD(_DefDlgProc));

            // Make fonts non-bold
            if (GetWindowStyle(hwndCurPage) & DS_3DLOOK)
                Win31MakeDlgNonBold(hwndCurPage);

            // Remove the default button - it is in the main dialog
            RemoveDefaultButton(hwndCurPage,NULL);
#endif
        }
        else
        {
            /* Should we put up some sort of error message here?
            */
            TabCtrl_DeleteItem(hwndTabs, nItem);
            TabCtrl_SetCurSel(hwndTabs, 0);
            goto TryAgain;
        }
    }

        // THI WAS REMOVED as part of the fix for bug 18327.  The problem is we need to
        // send a SETACTIVE message to a page if it is being activated.
//    if (ppd->hwndCurPage == hwndCurPage)
//    {
//        /* we should be done at this point.
//        */
//        return;
//    }

    /* Size the dialog and move it to the top of the list before showing
    ** it in case there is size specific initializing to be done in the
    ** GETACTIVE message.
    */
    SetWindowPos(hwndCurPage, HWND_TOP, ppd->xSubDlg, ppd->ySubDlg, ppd->cxSubDlg, ppd->cySubDlg, 0);

    /* We want to send the SETACTIVE message before the window is visible
    ** to minimize on flicker if it needs to update fields.
    */

    //
    //  If the page returns non-zero from the PSN_SETACTIVE call then
    //  we will set the activation to the resource ID returned from
    //  the call and set activation to it.  This is mainly used by wizards
    //  to skip a step.
    //
    lres = _Ppd_SendNotify(ppd, nItem, PSN_SETACTIVE, 0);

    if (lres) {
    TabCtrl_SetCurSel(hwndTabs, FindPageIndex(ppd, nItem,
                    (lres == -1) ? 0 : lres, iAutoAdj));

        ShowWindow(hwndCurPage, SW_HIDE);
        goto TryAgain;
    }

    if (ppd->psh.dwFlags & PSH_HASHELP) {
    Button_Enable(GetDlgItem(hDlg, IDHELP),
          (BOOL)(ppd->psh.phpage[nItem]->psp.dwFlags & PSP_HASHELP));
    }

    //
    //  If this is a wizard then we'll set the dialog's title to the tab
    //  title.
    //
    if (ppd->psh.dwFlags & PSH_WIZARD) {
    TC_ITEMEXTRA tie;
    char szTemp[128 + 50];

    tie.tci.mask = TCIF_TEXT;
    tie.tci.pszText = szTemp;
    tie.tci.cchTextMax = sizeof(szTemp);
    //// BUGBUG -- Check for error. Does this return false if fails??
    TabCtrl_GetItem(hwndTabs, nItem, &tie.tci);
    if (szTemp[0])
        SetWindowText(hDlg, szTemp);
    }

    /* Disable all erasebkgnd messages that come through because windows
    ** are getting shuffled.  Note that we need to call ShowWindow (and
    ** not show the window in some other way) because DavidDs is counting
    ** on the correct parameters to the WM_SHOWWINDOW message, and we may
    ** document how to keep your page from flashing.
    */
    ppd->fFlags |= PD_NOERASE;
    ShowWindow(hwndCurPage, SW_SHOW);
    if (ppd->hwndCurPage && (ppd->hwndCurPage != hwndCurPage))
    {
        ShowWindow(ppd->hwndCurPage, SW_HIDE);
    }
    ppd->fFlags &= ~PD_NOERASE;

    ppd->hwndCurPage = hwndCurPage;
    ppd->nCurItem = nItem;

    /* Newly created dialogs seem to steal the focus, so we steal it back
    ** to the page list, which must have had the focus to get to this
    ** point.  If this is a wizard then set the focus to the dialog of
    ** the page.  Otherwise, set the focus to the tabs.
    */
    if (hwndCurFocus != hwndTabs)
    {
        SetNewDefID(ppd);
    }
    else
    {
    // The focus may have been stolen from us, bring it back
    SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hwndTabs, (LPARAM)TRUE);
    }
}

#define DECLAREWAITCURSOR  HCURSOR hcursor_wait_cursor_save
#define SetWaitCursor()   hcursor_wait_cursor_save = SetCursor(LoadCursor(NULL, IDC_WAIT))
#define ResetWaitCursor() SetCursor(hcursor_wait_cursor_save)

// return TRUE iff all sheets successfully handle the notification
BOOL NEAR PASCAL ButtonPushed(LPPROPDATA ppd, WPARAM wParam)
{
    HWND hwndTabs;
    int nItems, nItem;
    int nNotify;
    TC_ITEMEXTRA tie;
    BOOL bExit = FALSE;
    int nReturnNew = ppd->nReturn;
    int fSuccess = TRUE;
    DECLAREWAITCURSOR;
    LRESULT lres;
    LPARAM lParam = FALSE;

    switch (wParam) {
    case IDOK:
        lParam = TRUE;
    bExit = TRUE;

    // Fall through...

    case IDD_APPLYNOW:
    // First allow the current dialog to validate itself.
    if ((BOOL)_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_KILLACTIVE, 0))
        return FALSE;


    nReturnNew = 1;

    nNotify = PSN_APPLY;
    break;

    case IDCLOSE:
        lParam = TRUE;
    case IDCANCEL:
    bExit = TRUE;
    nNotify = PSN_RESET;
    break;

    default:
    return FALSE;
    }

    SetWaitCursor();

    hwndTabs = ppd->hwndTabs;

    tie.tci.mask = TCIF_PARAM;

    nItems = TabCtrl_GetItemCount(hwndTabs);
    for (nItem = 0; nItem < nItems; ++nItem)
    {
    
    TabCtrl_GetItem(hwndTabs, nItem, &tie.tci);

    if (tie.hwndPage)
    {
        /* If the dialog fails a PSN_APPY call (by returning TRUE),
        ** then it has invalid information on it (should be verified
        ** on the PSN_KILLACTIVE, but that is not always possible)
        ** and we want to abort the notifications.  We select the failed
        ** page below.
        */
        lres = _Ppd_SendNotify(ppd, nItem, nNotify, lParam);
            if (lres)
            {
                fSuccess = FALSE;
                bExit = FALSE;
                break;
            } else {
                // if we need a restart (Apply or OK), then this is an exit
                if ((nNotify == PSN_APPLY) && !bExit && ppd->nRestart) {
                    DebugMsg(DM_TRACE, "PropertySheet: restart flags force close");
                    bExit = TRUE;
                }
            }

        /* We have either reset or applied, so everything is
        ** up to date.
        */
        tie.state &= ~FLAG_CHANGED;
        // tie.tci.mask = TCIF_PARAM;    // already set
        TabCtrl_SetItem(hwndTabs, nItem, &tie.tci);
    }
    }

    /* If we leave ppd->hwndCurPage as NULL, it will tell the main
    ** loop to exit.
    */
    if (fSuccess)
    {
    ppd->hwndCurPage = NULL;
    }
    else if (lres != PSNRET_INVALID_NOCHANGEPAGE)
    {
    // Need to change to the page that caused the failure.
    // if lres == PSN_INVALID_NOCHANGEPAGE, then assume sheet has already
    // changed to the page with the invalid information on it
    TabCtrl_SetCurSel(hwndTabs, nItem);
    }

    if (fSuccess)
    {
    // Set to the cached value
    ppd->nReturn = nReturnNew;
    }

    if (!bExit)
    {
    // before PageChange, so ApplyNow gets disabled faster.
    if (fSuccess)
    {
        char szOK[30];
            HWND hwndApply;

            if (!(ppd->psh.dwFlags & PSH_WIZARD)) {
                // The ApplyNow button should always be disabled after
                // a successfull apply/cancel, since no change has been made yet.
                hwndApply = GetDlgItem(ppd->hDlg, IDD_APPLYNOW);
                Button_SetStyle(hwndApply, BS_PUSHBUTTON, TRUE);
                EnableWindow(hwndApply, FALSE);
                ResetWizButtons(ppd);
                SendMessage(ppd->hDlg, DM_SETDEFID, IDOK, 0);
                ppd->idDefaultFallback = IDOK;
            }

        // Undo PSM_CANCELTOCLOSE for the same reasons.
        if (ppd->fFlags & PD_CANCELTOCLOSE)
        {
        ppd->fFlags &= ~PD_CANCELTOCLOSE;
        LoadString(HINST_THISDLL, IDS_OK, szOK, sizeof(szOK));
        SetDlgItemText(ppd->hDlg, IDOK, szOK);
        EnableWindow(GetDlgItem(ppd->hDlg, IDCANCEL), TRUE);
        }
    }

    /* Re-"select" the current item and get the whole list to
    ** repaint.
    */
    if (lres != PSNRET_INVALID_NOCHANGEPAGE)
        PageChange(ppd, 1);
    }

    ResetWaitCursor();

    return(fSuccess);
}

//  Win3.1 USER didn't handle DM_SETDEFID very well-- it's very possible to get
//  multiple buttons with the default button style look.  This has been fixed
//  for Win95, but the Setup wizard needs this hack when running from 3.1.

// it seems win95 doesn't handle it well either..
void NEAR PASCAL ResetWizButtons(LPPROPDATA ppd)
{
    int id;

    if (ppd->psh.dwFlags & PSH_WIZARD) {

        for (id = 0; id < ARRAYSIZE(WizIDs); id++)
            SendDlgItemMessage(ppd->hDlg, WizIDs[id], BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
    }
}

void NEAR PASCAL SetWizButtons(LPPROPDATA ppd, LPARAM lParam)
{
    int iShowID = IDD_NEXT;
    int iHideID = IDD_FINISH;
    int idDef;
    BOOL bEnabled;
    BOOL bResetFocus;
    HWND hwndShow;
    HWND hwndFocus = GetFocus();
    HWND hwndHide;
    HWND hwndBack;
    HWND hDlg = ppd->hDlg;

    idDef = (int)LOWORD(SendMessage(hDlg, DM_GETDEFID, 0, 0));

    bEnabled = (lParam & PSWIZB_BACK) != 0;
    hwndBack = GetDlgItem(hDlg, IDD_BACK);
    EnableWindow(hwndBack, bEnabled);

    bEnabled = (lParam & PSWIZB_NEXT) != 0;
    if (lParam & (PSWIZB_FINISH | PSWIZB_DISABLEDFINISH)) {
        iShowID = IDD_FINISH;
        iHideID = IDD_NEXT;
        bEnabled = (lParam & PSWIZB_FINISH) != 0;
    }
    hwndHide = GetDlgItem(hDlg, iHideID);
    ShowWindow(hwndHide, SW_HIDE);

    hwndShow = GetDlgItem(hDlg, iShowID);
    ShowWindow(hwndShow, SW_SHOW);
    EnableWindow(hwndShow, bEnabled);

    // bResetFocus keeps track of whether or not we need to set Focus to our button
    bResetFocus = FALSE;
    if (hwndFocus)
    {
        // if the dude that has focus is a button, we want to steal focus away
        // so users can just press enter all the way through a property sheet,
        // getting the default as they go. this also catches the case
        // of where focus is on one of our buttons which was turned off.
        if (SendMessage(hwndFocus, WM_GETDLGCODE, 0, 0L) & (DLGC_UNDEFPUSHBUTTON|DLGC_DEFPUSHBUTTON))
            bResetFocus = TRUE;
    }
    if (!bResetFocus)
    {
        // if there is no focus or we're focused on an invisible/disabled
        // item on the sheet, grab focus.
        bResetFocus = !hwndFocus ||  !IsWindowVisible(hwndFocus) || !IsWindowEnabled(hwndFocus) ;
        Assert(!bResetFocus); // how can this be??
    }

    // We used to do this code only if we nuked a button which had default
    // or if bResetFocus. Unfortunately, some wizards turn off BACK+NEXT
    // and then when they turn them back on, they want DEFID on NEXT.
    // So now we always reset DEFID.
    {
        int ids[3] = { iShowID, IDD_BACK, IDCANCEL };
        int i;
        HWND hwndNewFocus = NULL;

        for (i = 0; i < ARRAYSIZE(ids); i++) {
            hwndNewFocus = GetDlgItem(hDlg, ids[i]);
            if (IsWindowVisible(hwndNewFocus) && IsWindowEnabled(hwndNewFocus)) {
                hwndFocus = hwndNewFocus;
                break;
            }
        }

        ppd->idDefaultFallback = ids[i];
        if (bResetFocus)
            SetFocus(hwndNewFocus);
        ResetWizButtons(ppd);
        SendMessage(hDlg, DM_SETDEFID, ids[i], 0);

    }
}


int NEAR PASCAL FindItem(HWND hwndTabs, HWND hwndPage,  TC_ITEMEXTRA FAR * lptie)
{
    int i;

    for (i = TabCtrl_GetItemCount(hwndTabs) - 1; i >= 0; --i)
    {
        TabCtrl_GetItem(hwndTabs, i, &lptie->tci);

        if (lptie->hwndPage == hwndPage)
        {
            break;
        }
    }

    //this will be -1 if the for loop falls out.
    return i;
}

// a page is telling us that something on it has changed and thus
// "Apply Now" should be enabled

void NEAR PASCAL PageInfoChange(LPPROPDATA ppd, HWND hwndPage)
{
    int i;
    TC_ITEMEXTRA tie;

    tie.tci.mask = TCIF_PARAM;
    i = FindItem(ppd->hwndTabs, hwndPage, &tie);

    if (i == -1)
        return;

    if (!(tie.state & FLAG_CHANGED))
    {
        // tie.tci.mask = TCIF_PARAM;    // already set
        tie.state |= FLAG_CHANGED;
        TabCtrl_SetItem(ppd->hwndTabs, i, &tie.tci);
    }

    EnableWindow(GetDlgItem(ppd->hDlg, IDD_APPLYNOW), TRUE);
}

// a page is telling us that everything has reverted to its last
// saved state.

void NEAR PASCAL PageInfoUnChange(LPPROPDATA ppd, HWND hwndPage)
{
    int i;
    TC_ITEMEXTRA tie;

    tie.tci.mask = TCIF_PARAM;
    i = FindItem(ppd->hwndTabs, hwndPage, &tie);

    if (i == -1)
        return;

    if (tie.state & FLAG_CHANGED)
    {
        tie.state &= ~FLAG_CHANGED;
        TabCtrl_SetItem(ppd->hwndTabs, i, &tie.tci);
    }

    // check all the pages, if none are FLAG_CHANGED, disable IDD_APLYNOW
    for (i = ppd->psh.nPages-1 ; i >= 0 ; i--)
    {
    // BUGBUG? Does TabCtrl_GetItem return its information properly?!?

    if (!TabCtrl_GetItem(ppd->hwndTabs, i, &tie.tci))
        break;
    if (tie.state & FLAG_CHANGED)
        break;
    }
    if (i<0)
    EnableWindow(GetDlgItem(ppd->hDlg, IDD_APPLYNOW), FALSE);
}

BOOL NEAR PASCAL AddPropPage(LPPROPDATA ppd, PSP FAR * hpage)
{
    POINT pt;
    HICON hIcon = NULL;
    char szTemp[128];
    TC_ITEMEXTRA tie;
#ifdef WIN32
    HIMAGELIST himl;
#endif

    if (ppd->psh.nPages >= MAXPROPPAGES)
        return FALSE; // we're full

    ppd->psh.phpage[ppd->psh.nPages] = hpage;
    ppd->psh.nPages++;
    tie.tci.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
    tie.hwndPage = NULL;
    tie.tci.pszText = szTemp;
    tie.state = 0;

#ifdef WIN32
    himl = TabCtrl_GetImageList(ppd->hwndTabs);
#endif

    if (GetPageInfo(hpage, szTemp, sizeof(szTemp), &pt, &hIcon))
    {
        // Add the page to the end of the tab list

        if (hIcon) {
#ifdef WIN32
            if (himl)
                tie.tci.iImage = ImageList_AddIcon(himl, hIcon);
#else
            tie.tci.iImage = -1;
#endif
            DestroyIcon(hIcon);
        } else {
            tie.tci.iImage = -1;
        }

        TabCtrl_InsertItem(ppd->hwndTabs, 1000, &tie.tci);
        return TRUE;
    }
    else
    {
    DebugMsg(DM_ERROR, "AddPropPage: GetPageInfo failed");
    ppd->psh.nPages--;
        return FALSE;
    }
}

// removes property sheet hpage (index if NULL)
void NEAR PASCAL RemovePropPage(LPPROPDATA ppd, int index, PSP FAR * hpage)
{
    int i = -1;
    BOOL fReturn = TRUE;
    TC_ITEMEXTRA tie;

    tie.tci.mask = TCIF_PARAM;
    if (hpage) {
        for (i = ppd->psh.nPages - 1; i >= 0; i--) {
            if (hpage == ppd->psh.phpage[i])
                break;
        }
    }
    if (i == -1) {
        i = index;

    // this catches i < 0 && i >= (int)(ppd->psh.nPages)
    if ((UINT)i >= ppd->psh.nPages)
        {
            DebugMsg(DM_ERROR, "RemovePropPage: invalid page");
        return;
        }
    }

    index = TabCtrl_GetCurSel(ppd->hwndTabs);
    if (i == index) {
    // if we're removing the current page, select another (don't worry
    // about this page having invalid information on it -- we're nuking it)
        PageChanging(ppd);

        if (index == 0)
            index++;
        else
            index--;

    if (SendMessage(ppd->hwndTabs, TCM_SETCURSEL, index, 0L) == -1) {
            // if we couldn't select (find) the new one, punt to 0th
            SendMessage(ppd->hwndTabs, TCM_SETCURSEL, 0, 0L);
        }
    PageChange(ppd, 1);
    }

    tie.tci.mask = TCIF_PARAM;
    TabCtrl_GetItem(ppd->hwndTabs, i, &tie.tci);
    if (tie.hwndPage) {
        if (ppd->hwndCurPage == tie.hwndPage)
            ppd->hwndCurPage = NULL;
        DestroyWindow(tie.hwndPage);
    }
    DestroyPropertySheetPage(ppd->psh.phpage[i]);

    ppd->psh.nPages--;
    hmemcpy(&ppd->psh.phpage[i], &ppd->psh.phpage[i+1],
            sizeof(ppd->psh.phpage[0]) * (ppd->psh.nPages - i));
    TabCtrl_DeleteItem(ppd->hwndTabs, i);

}

// returns TRUE iff the page was successfully set to index/hpage
// Note:  The iAutoAdj should be set to 1 or -1.  This value is used
//    by PageChange if a page refuses a SETACTIVE to either increment
//    or decrement the page index.
BOOL NEAR PASCAL PageSetSelection(LPPROPDATA ppd, int index, PSP FAR * hpage,
                  int iAutoAdj)
{
    int i = -1;
    BOOL fReturn = FALSE;
    TC_ITEMEXTRA tie;

    tie.tci.mask = TCIF_PARAM;
    if (hpage) {
        for (i = ppd->psh.nPages - 1; i >= 0; i--) {
            if (hpage == ppd->psh.phpage[i])
                break;
        }
    }
    if (i == -1) {
        if (index == -1)
            return FALSE;

        i = index;
    }
    if (i >= MAXPROPPAGES)
    {
    // don't go off the end of our HPROPSHEETPAGE array
    return FALSE;
    }

    fReturn = !PageChanging(ppd);
    if (fReturn)
    {
    index = TabCtrl_GetCurSel(ppd->hwndTabs);
    if (SendMessage(ppd->hwndTabs, TCM_SETCURSEL, i, 0L) == -1) {
        // if we couldn't select (find) the new one, fail out
        // and restore the old one
        SendMessage(ppd->hwndTabs, TCM_SETCURSEL, index, 0L);
        fReturn = FALSE;
    }
    PageChange(ppd, iAutoAdj);
    }
    return fReturn;
}

LRESULT NEAR PASCAL QuerySiblings(LPPROPDATA ppd, WPARAM wParam, LPARAM lParam)
{
    UINT i;
    for (i = 0 ; i < ppd->psh.nPages ; i++)
    {
    HWND hwndSibling = _Ppd_GetPage(ppd, i);
    if (hwndSibling)
    {
        LRESULT lres = SendMessage(hwndSibling, PSM_QUERYSIBLINGS, wParam, lParam);
        if (lres)
        return lres;
    }
    }
    return FALSE;
}

// REVIEW HACK This gets round the problem of having a hotkey control
// up and trying to enter the hotkey that is already in use by a window.
BOOL NEAR PASCAL HandleHotkey(LPARAM lparam)
{
    WORD wHotkey;
    char szClass[32];
    HWND hwnd;

    // What hotkey did the user type hit?
    wHotkey = (WORD)SendMessage((HWND)lparam, WM_GETHOTKEY, 0, 0);
    // Were they typing in a hotkey window?
    hwnd = GetFocus();
    GetClassName(hwnd, szClass, sizeof(szClass));
    if (lstrcmp(szClass, HOTKEY_CLASS) == 0)
    {
    // Yes.
    SendMessage(hwnd, HKM_SETHOTKEY, wHotkey, 0);
    return TRUE;
    }
    return FALSE;
}


//
//  Function handles Next and Back functions for wizards.  The code will
//  be either PSN_WIZNEXT or PSN_WIZBACK
//
BOOL NEAR PASCAL WizNextBack(LPPROPDATA ppd, int code)
{
    DWORD   dwFind;
    int iAutoAdj = (code == PSN_WIZNEXT) ? 1 : -1;

    dwFind = _Ppd_SendNotify(ppd, ppd->nCurItem, code, 0);

    if (dwFind == -1) {
    return(FALSE);
    }
    return(PageSetSelection(ppd, FindPageIndex(ppd, ppd->nCurItem, dwFind, iAutoAdj), NULL, iAutoAdj));
}

//
//
//
BOOL NEAR PASCAL Prsht_OnCommand(LPPROPDATA ppd, int id, HWND hwndCtrl, UINT codeNotify)
{
    if (!hwndCtrl)
        hwndCtrl = GetDlgItem(ppd->hDlg, id);

    switch (id) {

    case IDCLOSE:
    case IDCANCEL:
    if (_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_QUERYCANCEL, 0) == 0) {
            ButtonPushed(ppd, id);
        }
        break;

    case IDD_APPLYNOW:
    case IDOK:
        if (!(ppd->psh.dwFlags & PSH_WIZARD)) {
            ButtonPushed(ppd, id);
        } else {
            Assert(0);
        }
    break;

    case IDHELP:
    if (IsWindowEnabled(hwndCtrl))
    {
        _Ppd_SendNotify(ppd, ppd->nCurItem, PSN_HELP, 0);
    }
    break;

    case IDD_FINISH:
        // b#11346 - dont let multiple clicks on FINISH.
    EnableWindow(ppd->hDlg, FALSE);
    if (!_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_WIZFINISH, 0))
    {
        ppd->hwndCurPage = NULL;
        ppd->nReturn = 1;
    }
    else
    {
        EnableWindow(ppd->hDlg, TRUE);
    }
    break;

    case IDD_NEXT:
    case IDD_BACK:
        ppd->idDefaultFallback = id;
    WizNextBack(ppd, id == IDD_NEXT ? PSN_WIZNEXT : PSN_WIZBACK);
    break;

    default:
        FORWARD_WM_COMMAND(_Ppd_GetPage(ppd, ppd->nCurItem), id, hwndCtrl, codeNotify, SendMessage);
    }

    return TRUE;
}


BOOL NEAR PASCAL Prop_IsDialogMessage(LPPROPDATA ppd, LPMSG32 pmsg32)
{
    if ((pmsg32->message == WM_KEYDOWN) && (GetAsyncKeyState(VK_CONTROL) < 0))
    {
        BOOL bBack;

        switch (pmsg32->wParam) {
    case VK_TAB:
        bBack = GetAsyncKeyState(VK_SHIFT) < 0;
        break;

    case VK_PRIOR:  // VK_PAGE_UP
    case VK_NEXT:   // VK_PAGE_DOWN
        bBack = (pmsg32->wParam == VK_PRIOR);
        break;

    default:
        goto NoKeys;
    }

    if (ppd->psh.dwFlags & PSH_WIZARD)
    {
        WizNextBack(ppd, bBack ? PSN_WIZBACK : PSN_WIZNEXT);
    }
    else
    {
        int iCur = TabCtrl_GetCurSel(ppd->hwndTabs);
            // tab in reverse if shift is down
            if (bBack)
                iCur += (ppd->psh.nPages - 1);
        else
        iCur++;

        iCur %= ppd->psh.nPages;
        PageSetSelection(ppd, iCur, NULL, 1);
    }
        return TRUE;
    }
NoKeys:

#ifdef WIN31
    if (Win31IsKeyMessage(ppd->hDlg, pmsg32))
        return TRUE;

#ifdef IEWIN31_25
    // BUGBUG:
    // If focus is on the property sheet, then the accelerator for the
    // apply button is ignored.  So we check for this accelerator ourselves.
    // This is only necessary for real win31 systems.  The code below
    // works fine on Win95's 16-bit system without this hack. (stevepro)
    //
    if (pmsg32->message == WM_SYSCHAR)
    {
        // Find the apply button to get its accelerator
        HWND hwndApply = GetDlgItem(ppd->hDlg, IDD_APPLYNOW);
        if (hwndApply && (IsWindowVisible(hwndApply)))
        {
            // Get the accelerator for the apply button
            char buf[40];
            char FAR * pszAccel;

            GetWindowText(hwndApply, buf, sizeof(buf)-1);
            pszAccel = _fstrstr(buf, "&");
            if (pszAccel && (0 == StrCmpNI(&(char)pmsg32->wParam, ++pszAccel, 1)))
            {
                // We have a match, so process the accelerator 
//                if (IsDialogMessage32(ppd->hDlg, pmsg32, TRUE))
                SendMessage(hwndApply, BM_SETSTATE, TRUE, 0);
                SendMessage(hwndApply, BM_SETSTATE, FALSE, 0);
                SendMessage(GetParent(hwndApply), WM_COMMAND, IDD_APPLYNOW, MAKELPARAM(hwndApply, BN_CLICKED));
                return TRUE;
            }
        }
    }

#endif // IEWIN31_25

    if (IsDialogMessage32(ppd->hwndCurPage, pmsg32, TRUE))
        return TRUE;

    // BUGBUG:
    // User 3.1 doesn't handle accelerator keys properly between
    // the main and sub dialog. If a control in the main dialog has
    // focus and an accelerator for a control in the subdialog is
    // pressed, only the main dialog gets the message so the focus
    // doesn't change. Likewise if a control in the subdialog has focus.
    //
    // This will only be used during setup so we won't freak about it
    // (for now).

    if ((pmsg32->message == WM_SYSCHAR) && (pmsg32->hwnd != ppd->hwndCurPage))
    {
        SetFocus(ppd->hwndCurPage);
    }
#endif

    if (IsDialogMessage32(ppd->hDlg, pmsg32, TRUE))
        return TRUE;

    return FALSE;
}


#pragma data_seg(DATASEG_READONLY)
const static DWORD aPropHelpIDs[] = {  // Context Help IDs
    IDD_APPLYNOW, IDH_COMM_APPLYNOW,
    0, 0
};
#pragma data_seg()

BOOL CALLBACK PropSheetDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    HWND hwndT;
    LPPROPDATA ppd = (LPPROPDATA)GetWindowLong(hDlg, DWL_USER);

    switch (uMessage) {
    case WM_INITDIALOG:
    InitPropSheetDlg(hDlg, (LPPROPDATA)lParam);
    return FALSE;

    // REVIEW for dealing with hotkeys.
    // BUGBUG: This code might not work with 32-bit WM_SYSCOMMAND msgs.
    case WM_SYSCOMMAND:
    if (wParam == SC_HOTKEY)
        return HandleHotkey(lParam);
    else if (wParam == SC_CLOSE)
    {
        // system menu close should be IDCANCEL, but if we're in the
        // PSM_CANCELTOCLOSE state, treat it as an IDOK (ie, "Close").
        return Prsht_OnCommand(ppd, (ppd->fFlags & PD_CANCELTOCLOSE) ? IDOK : IDCLOSE, NULL, 0);
    }

    return FALSE;      // Let default process happen

    case WM_NCDESTROY:
    if (ppd)
    {
        int iPage;

        Assert(GetDlgItem(hDlg, IDD_PAGELIST) == NULL);

        ppd->hwndTabs = NULL;

        // NOTE: all of the hwnds for the pages must be destroyed by now!

        // Release all page objects in REVERSE ORDER so we can have
        // pages that are dependant on eachother based on the initial
        // order of those pages
        //
        for (iPage = ppd->psh.nPages - 1; iPage >= 0; iPage--)
        {
        DestroyPropertySheetPage(ppd->psh.phpage[iPage]);
        }

            // If we are modeless, we need to free our ppd.  If we are modal,
            // we let _RealPropertySheet free it since one of our pages may
            // set the restart flag during DestroyPropertySheetPage above.
            if (ppd->psh.dwFlags & PSH_MODELESS)
            {
#ifdef WIN32
                LocalFree((HLOCAL)ppd);
#else
                LocalFree((HLOCAL)LOWORD(ppd));
#endif
            }
    }
    //
    // NOTES:
    //  Must return FALSE to avoid DS leak!!!
    //
    return FALSE;

    case WM_DESTROY:
    if (ppd)
    {
#ifdef WIN32
            // Destroy the image list we created during our init call.
        HIMAGELIST himl = TabCtrl_GetImageList(ppd->hwndTabs);
            if (himl)
                ImageList_Destroy(himl);
#endif

            if ((ppd->psh.dwFlags & PSH_USEICONID) && ppd->psh.hIcon)
        DestroyIcon(ppd->psh.hIcon);
    }
#ifdef WIN31
    {
        // Clean up the non-bold font if we created one
        HFONT hFont = GetProp(hDlg, g_szNonBoldFont);
        if (hFont)
        {
        DeleteObject(hFont);
        }
        RemoveProp(hDlg, g_szNonBoldFont);
    }
#endif

    break;

    case WM_ERASEBKGND:
    return ppd->fFlags & PD_NOERASE;
    break;

    case WM_COMMAND:
    // Cannot use HANDLE_WM_COMMAND, because we want to pass a result!
    return Prsht_OnCommand(ppd, GET_WM_COMMAND_ID(wParam, lParam),
        GET_WM_COMMAND_HWND(wParam, lParam),
        GET_WM_COMMAND_CMD(wParam, lParam));

#ifdef WIN31
    case WM_CTLCOLOR:
        return LOWORD(HANDLE_WM_CTLCOLOR(hDlg, wParam, lParam, Win31OnCtlColor));

    case WM_GETDLGCODE:
        return DLGC_RECURSE;
#endif

    case WM_NOTIFY:

    switch (((NMHDR FAR *)lParam)->code)
        {
            case TCN_SELCHANGE:
            PageChange(ppd, 1);
                break;

            case TCN_SELCHANGING:
                SetWindowLong(hDlg, DWL_MSGRESULT, PageChanging(ppd));
                break;

        default:
            return FALSE;
    }
    return TRUE;

    case PSM_SETWIZBUTTONS:
        SetWizButtons(ppd, lParam);
    break;

    case PSM_SETFINISHTEXT:
    {
        HWND hFinish = GetDlgItem(hDlg, IDD_FINISH);
            HWND hwndFocus = GetFocus();
            HWND hwnd;
            BOOL fSetFocus = FALSE;

            hwnd = GetDlgItem(hDlg, IDD_NEXT);
            if (hwnd == hwndFocus)
                fSetFocus = TRUE;
        ShowWindow(hwnd, SW_HIDE);

            hwnd = GetDlgItem(hDlg, IDD_BACK);
            if (hwnd == hwndFocus)
                fSetFocus = TRUE;
        ShowWindow(hwnd, SW_HIDE);

        if (lParam) {
        Button_SetText(hFinish, (LPSTR)lParam);
        }
        ShowWindow(hFinish, SW_SHOW);
        Button_Enable(hFinish, TRUE);

            ResetWizButtons(ppd);
            SendMessage(hDlg, DM_SETDEFID, IDD_FINISH, 0);

            ppd->idDefaultFallback = IDD_FINISH;
            if (fSetFocus)
                SetFocus(hFinish);
    }
    break;


    case PSM_SETTITLE:
        ppd->psh.pszCaption = (LPCSTR)lParam;
    ppd->psh.dwFlags = ((((DWORD)wParam) & PSH_PROPTITLE) | (ppd->psh.dwFlags & ~PSH_PROPTITLE));
        _SetTitle(hDlg, ppd);
        break;

    case PSM_CHANGED:
        PageInfoChange(ppd, (HWND)wParam);
        break;

    case PSM_RESTARTWINDOWS:
        ppd->nRestart |= ID_PSRESTARTWINDOWS;
        break;

    case PSM_REBOOTSYSTEM:
        ppd->nRestart |= ID_PSREBOOTSYSTEM;
        break;

    case PSM_CANCELTOCLOSE:
    if (!(ppd->fFlags & PD_CANCELTOCLOSE))
    {
        char szClose[20];

        ppd->fFlags |= PD_CANCELTOCLOSE;
        LoadString(HINST_THISDLL, IDS_CLOSE, szClose, sizeof(szClose));
        SetDlgItemText(hDlg, IDOK, szClose);
        EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);
    }
        break;

    case PSM_SETCURSEL:
    SetWindowLong(hDlg, DWL_MSGRESULT,
        PageSetSelection(ppd, (int)wParam,(PSP FAR *)lParam, 1));
    break;

    case PSM_SETCURSELID:
    SetWindowLong(hDlg, DWL_MSGRESULT,
        PageSetSelection(ppd, FindPageIndex(ppd, ppd->nCurItem, (DWORD)lParam, 1),
                 NULL, 1));
    break;

    case PSM_REMOVEPAGE:
        RemovePropPage(ppd, (int)wParam,(PSP FAR *)lParam);
        break;

    case PSM_ADDPAGE:
        SetDlgMsgResult(hDlg, uMessage, AddPropPage(ppd,(PSP FAR *)lParam));
        break;

    case PSM_QUERYSIBLINGS:
    SetWindowLong(hDlg, DWL_MSGRESULT, QuerySiblings(ppd, wParam, lParam));
    break;

    case PSM_UNCHANGED:
    PageInfoUnChange(ppd, (HWND)wParam);
    break;

    case PSM_APPLY:
    // a page is asking us to simulate an "Apply Now".
    // let the page know if we're successful
    SetWindowLong(hDlg, DWL_MSGRESULT, ButtonPushed(ppd, IDD_APPLYNOW));
        break;

    case PSM_GETTABCONTROL:
    SetWindowLong(hDlg, DWL_MSGRESULT, (LONG)(UINT)ppd->hwndTabs);
        break;

    case PSM_GETCURRENTPAGEHWND:
        SetWindowLong(hDlg, DWL_MSGRESULT, (LONG)(UINT)ppd->hwndCurPage);
        break;

    case PSM_PRESSBUTTON:
    if (wParam <= PSBTN_MAX)
    {
        const static int IndexToID[] = {IDD_BACK, IDD_NEXT, IDD_FINISH, IDOK,
                   IDD_APPLYNOW, IDCANCEL, IDHELP};
        Prsht_OnCommand(ppd, IndexToID[wParam], NULL, 0);
    }
    break;

    case PSM_ISDIALOGMESSAGE:
        // returning TRUE means we handled it, do a continue
    // FALSE do standard translate/dispatch
    SetWindowLong(hDlg, DWL_MSGRESULT, Prop_IsDialogMessage(ppd, (LPMSG32)lParam));
    break;

        // these should be relayed to all created dialogs
    case WM_WININICHANGE:
    case WM_SYSCOLORCHANGE:
    if (ppd)
    {
        int nItem, nItems = TabCtrl_GetItemCount(ppd->hwndTabs);
        for (nItem = 0; nItem < nItems; ++nItem)
        {
        hwndT = _Ppd_GetPage(ppd, nItem);
        if (hwndT)
            SendMessage(hwndT, uMessage, wParam, lParam);
        }
        SendMessage(ppd->hwndTabs, uMessage, wParam, lParam);
    }
    break;

    //
    // send toplevel messages to the current page and tab control
    //
    case WM_ENABLE:
    case WM_QUERYNEWPALETTE:
    case WM_PALETTECHANGED:
    case WM_DEVICECHANGE:
    if (!ppd)
        return FALSE;
    if (ppd->hwndTabs)
        SendMessage(ppd->hwndTabs, uMessage, wParam, lParam);
    case WM_ACTIVATEAPP:
    case WM_ACTIVATE:
    if (ppd)
    {
        hwndT = _Ppd_GetPage(ppd, ppd->nCurItem);
        if (hwndT && IsWindow(hwndT))
        {
        //
        // By doing this, we are "handling" the message.  Therefore
        // we must set the dialog return value to whatever the child
        // wanted.
        //
        SetWindowLong(hDlg, DWL_MSGRESULT,
            SendMessage(hwndT, uMessage, wParam, lParam));

        break;
        }
    }
    return FALSE;

    case WM_CONTEXTMENU:
        if ((ppd->hwndTabs != (HWND)wParam) && (ppd->hwndCurPage != (HWND)wParam) &&
            (!(ppd->psh.dwFlags & PSH_WIZARD)))
            WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (DWORD)(LPVOID)aPropHelpIDs);
        break;

#ifndef IEWIN31_25
    case WM_HELP:
    hwndT = (HWND)((LPHELPINFO)lParam)->hItemHandle;
    if ((GetParent(hwndT) == hDlg) && (hwndT != ppd->hwndTabs))
            WinHelp(hwndT, NULL, HELP_WM_HELP, (DWORD)(LPSTR) aPropHelpIDs);
    break;
#endif

    default:
    return FALSE;
    }

    return TRUE;
}

#ifdef WIN31

LRESULT CALLBACK Win31PropPageWndProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LRESULT result;
    HFONT   hFont;
    LRESULT (WINAPI *_DefDlgProc)(HWND, UINT, WPARAM, LPARAM)=NULL;
        (FARPROC)_DefDlgProc =  (FARPROC)(MAKELONG((UINT) GetProp(hDlg, g_szLoSubclass),
                (UINT)GetProp(hDlg, g_szHiSubclass)));

    switch (uMessage)
    {
    case WM_CTLCOLOR:
        result = HANDLE_WM_CTLCOLOR(hDlg,wParam,lParam,Win31OnCtlColor);
        if (result)
            return result;
        break;

    case WM_GETDLGCODE:
        return DLGC_RECURSE;

    case WM_DESTROY:
        // Clean up the non-bold font if we created one
        hFont = GetProp(hDlg, g_szNonBoldFont);
        if (hFont)
        {
            DeleteObject(hFont);
            RemoveProp(hDlg, g_szNonBoldFont);
        }

        // Clean up subclass
        if( _DefDlgProc )
        {
            RemoveProp(hDlg, g_szLoSubclass);
            RemoveProp(hDlg, g_szHiSubclass);
            // Now put the subclass back.
        SetWindowLong(hDlg, GWL_WNDPROC, (LPARAM)(WNDPROC)(_DefDlgProc));
    }
        break;

    default:
        break;
    }

//    return DefDlgProc(hDlg,uMessage,wParam,lParam);
    if( _DefDlgProc )
        return _DefDlgProc(hDlg,uMessage,wParam,lParam);
    else
        return DefDlgProc(hDlg,uMessage,wParam,lParam);
}

// Centers a dialog relative to the desktop
VOID NEAR PASCAL CenterDialog ( HWND hdlg )
{
    HWND  hwndPar = GetDesktopWindow();
    RECT  rectDlg, rectPar;
    int   x, y;
    int   dyPar, dyDlg, dyOff;
    POINT pt;

    GetWindowRect(hdlg, &rectDlg);
    GetClientRect(hwndPar, &rectPar);

    if ((x = (rectPar.right - rectPar.left) / 2 -
        (rectDlg.right - rectDlg.left) / 2) < 0)
    {
        x = 0;
    }
    dyPar = rectPar.bottom - rectPar.top;
    dyDlg = rectDlg.bottom - rectDlg.top;
    if ((y = dyPar / 2 - dyDlg / 2) < 0)
    {
        y = 0;
    }

    if (y > 0)
    {
        /* Offset by 1/2 width of title bar and border.
        */
        pt.x = pt.y = 0;
        ClientToScreen(hwndPar, &pt);
        GetWindowRect(hwndPar, &rectPar);
        dyOff = (pt.y - rectPar.top) / 2;

        if (y + dyOff + dyDlg < dyPar)
            y += dyOff;
        else
            y = dyPar - dyDlg;
    }

    SetWindowPos(hdlg, NULL, x, y, 0, 0, (SWP_NOSIZE | SWP_NOZORDER));
}

// Win31RepositionDlg( HWND hDlg )
//
// Ensures that a dialog lies entirely on the screen, used as a substitute for
// the DM_REPOSITION message in non Win4.0 environments
//
void NEAR PASCAL Win31RepositionDlg( HWND hDlg )
{
    RECT    rcWindow;
    int     dX, dY;

    // set rcWindow to dimensions of property page
    GetWindowRect( hDlg, &rcWindow );

    // set dX if dlg needs to be moved to the right
    dX = max( -rcWindow.left, 0 );
    if( dX == 0 ){
    // didn't need to be moved right, maybe we need to move left
    dX = -max( rcWindow.right - (GetSystemMetrics( SM_CXSCREEN ) - 1 ), 0 );
    }

    // set dY if dlg needs to be moved down
    dY = max( -rcWindow.top, 0 );
    if( dY == 0 ){
    // didn't need to be moved down, maybe we need to move up
    dY = -max( rcWindow.bottom - (GetSystemMetrics( SM_CYSCREEN ) - 1 ), 0 );
    }

    if( dX != 0 || dY != 0 ) {

    // we need to move the dialog
    OffsetRect( &rcWindow, dX, dY );
    SetWindowPos( hDlg, NULL, rcWindow.left, rcWindow.top, 0, 0,
                  SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER );
    }
}


LRESULT NEAR PASCAL Win31OnCtlColor(HWND hDlg, HDC hdcChild, HWND hwndChild, int nCtlType)
{
    COLORREF    clrFore;
    COLORREF    clrBack;
    HBRUSH      hbrBack;

    // Set up the supplied DC with the foreground and background
    // colors we want to use in the control, and return a brush
    // to use for filling.

    switch (nCtlType)
    {
    case CTLCOLOR_STATIC:
    case CTLCOLOR_DLG:
    case CTLCOLOR_MSGBOX:
        clrFore = g_clrWindowText;
        clrBack = g_clrBtnFace;
        hbrBack = g_hbrBtnFace;
        break;

    case CTLCOLOR_BTN:
        clrFore = g_clrBtnText;
        clrBack = g_clrBtnFace;
        hbrBack = g_hbrBtnFace;
        break;

    case CTLCOLOR_EDIT:
        if (GetWindowStyle(hwndChild) & ES_READONLY)
        {
            clrFore = g_clrWindowText;
            clrBack = g_clrBtnFace;
            hbrBack = g_hbrBtnFace;
            break;
        }
        // else fall thru to default

    default:
        // cause defaults to be used
        return NULL;
    }

    SetTextColor(hdcChild, clrFore);
    SetBkColor(hdcChild, clrBack);
    return((LRESULT)(DWORD)(WORD)hbrBack);
}

BOOL NEAR PASCAL Win31MakeDlgNonBold(HWND hDlg)
{
    HFONT   hFont;
    LOGFONT LogFont;
    HWND    hwndCtl;

    GetObject(GetWindowFont(hDlg), sizeof(LogFont), &LogFont);

    // Check if already non-bold
    if (LogFont.lfWeight <= FW_NORMAL)
        return TRUE;

    // Create the non-bold font
    LogFont.lfWeight = FW_NORMAL;
    if ((hFont = CreateFontIndirect(&LogFont)) == NULL)
        return FALSE;

    // Save the font as a window prop so we can delete it later
    SetProp(hDlg, g_szNonBoldFont, hFont);

    // Set all controls non-bold
    for (hwndCtl = GetWindow(hDlg,GW_CHILD);
         hwndCtl != NULL;
         hwndCtl = GetWindow(hwndCtl,GW_HWNDNEXT))
    {
        SetWindowFont(hwndCtl,hFont,FALSE);
    }

    return TRUE;
}
#endif // WIN31

int NEAR PASCAL _RealPropertySheet(LPPROPDATA ppd)
{
    HWND    hwndMain;
    MSG32   msg32;
    HWND    hwndTopOwner;
    int     nReturn = -1;
    HWND    hwndOriginalFocus;

    if (ppd->psh.nPages == 0)
    {
        DebugMsg(DM_ERROR, "no pages for prop sheet");
        goto FreePpdAndReturn;
    }

    ppd->hwndCurPage = NULL;
    ppd->nReturn     = -1;
    ppd->nRestart    = 0;
    ppd->fFlags      = FALSE;

    hwndTopOwner = ppd->psh.hwndParent;
    hwndOriginalFocus = GetFocus();

    if (!(ppd->psh.dwFlags & PSH_MODELESS))
    {
        //
        // Like dialog boxes, we only want to disable top level windows.
        // NB The mail guys would like us to be more like a regular
        // dialog box and disable the parent before putting up the sheet.
        if (hwndTopOwner)
        {
        while (GetWindowLong(hwndTopOwner, GWL_STYLE) & WS_CHILD)
            hwndTopOwner = GetParent(hwndTopOwner);

        Assert(hwndTopOwner);       // Should never get this!
        if (EnableWindow(hwndTopOwner, FALSE))
        {
        // Oh, the window was already disabled. Don't enable it later.
        hwndTopOwner = NULL;
        }
        }
    }
#if  !defined(WIN32) && !defined(WIN31)
#ifdef FE_IME
    // Win95d-B#754
    // When PCMCIA gets detected, NETDI calls DiCallClassInstaller().
    // The class installer of setupx calls PropertySheet() for msgsrv32.
    // We usually don't prepare thread link info in imm for that process as
    // it won't use IME normaly but we need to treat this case as special.
    // 
    CreateImmTLink(GetCurrentThreadID());
#endif
#endif

    if (ppd->psh.pfnCallback)
    {
        HRSRC hrsrc;
        HGLOBAL hglbl;
        LPVOID pTemplate;
        LPVOID pTemplateMod;
        DWORD cbTemplate;

        // Setup for failure
        hwndMain = NULL;

        // We need to load the resource and put it into a writeable memory block
        if ((hrsrc = FindResource(HINST_THISDLL,
                MAKEINTRESOURCE((ppd->psh.dwFlags & PSH_WIZARD) ? DLG_WIZARD : DLG_PROPSHEET),
                        RT_DIALOG)) &&
                (hglbl = LoadResource(HINST_THISDLL, hrsrc)))
        {
            pTemplate = LockResource(hglbl);
            cbTemplate = SizeofResource(HINST_THISDLL, hrsrc);

            pTemplateMod = (LPVOID)GlobalAlloc(GPTR, cbTemplate * 2); //double it to give some play leeway
            if (pTemplateMod)
            {
                hmemcpy(pTemplateMod, pTemplate, cbTemplate);

                ppd->psh.pfnCallback(NULL, PSCB_PRECREATE, (LPARAM)(LPVOID)pTemplateMod);

                hwndMain = CreateDialogIndirectParam(HINST_THISDLL, pTemplateMod,
                        ppd->psh.hwndParent, PropSheetDlgProc, (LPARAM)(LPPROPDATA)ppd);

                GlobalFreePtr(pTemplateMod);
            }
            UnlockResource(hglbl);
        }

    }
    else
        hwndMain = CreateDialogParam(HINST_THISDLL,
                MAKEINTRESOURCE((ppd->psh.dwFlags & PSH_WIZARD) ? DLG_WIZARD : DLG_PROPSHEET),
                ppd->psh.hwndParent, PropSheetDlgProc, (LPARAM)(LPPROPDATA)ppd);

    if (!hwndMain)
    {
        int iPage;

        DebugMsg(DM_ERROR, "PropertySheet: unable to create main dialog");

        if (hwndTopOwner && !(ppd->psh.dwFlags & PSH_MODELESS))
        EnableWindow(hwndTopOwner, TRUE);

    // Release all page objects in REVERSE ORDER so we can have
    // pages that are dependant on eachother based on the initial
    // order of those pages
    //
    for (iPage = (int)ppd->psh.nPages - 1; iPage >= 0; iPage--)
        DestroyPropertySheetPage(ppd->psh.phpage[iPage]);

        goto FreePpdAndReturn;
    }

    if (ppd->psh.dwFlags & PSH_MODELESS)
        return (int)hwndMain;

    ShowWindow(hwndMain, SW_SHOW);

    while (ppd->hwndCurPage && GetMessage32(&msg32, NULL, 0, 0, TRUE))
    {
        // if (PropSheet_IsDialogMessage(ppd->hDlg, (LPMSG)&msg32))
        if (Prop_IsDialogMessage(ppd, &msg32))
        continue;

    TranslateMessage32(&msg32, TRUE);
    DispatchMessage32(&msg32, TRUE);
    }

    if (ppd->hwndCurPage)
    {
        // GetMessage returned FALSE (WM_QUIT)
        DebugMsg( DM_TRACE, "PropertySheet: bailing in response to WM_QUIT (and reposting quit)" );
        ButtonPushed(ppd, IDCANCEL);  // nuke ourselves
        PostQuitMessage(msg32.wParam);  // repost quit for next enclosing loop
    }

    // don't let this get mangled during destroy processing
    nReturn = ppd->nReturn;

    if (ppd->psh.hwndParent && (GetActiveWindow() == hwndMain)) {
        DebugMsg(DM_TRACE, "Passing activation up");
        SetActiveWindow(ppd->psh.hwndParent);
    }

    if (hwndTopOwner)
    EnableWindow(hwndTopOwner, TRUE);

    if (IsWindow(hwndOriginalFocus)) {
        SetFocus(hwndOriginalFocus);
    }

    DestroyWindow(hwndMain);

    // do pickup any PSM_REBOOTSYSTEM or PSM_RESTARTWINDOWS sent during destroy
    if ((nReturn > 0) && ppd->nRestart)
        nReturn = ppd->nRestart;

FreePpdAndReturn:

#ifdef WIN32
    LocalFree((HLOCAL)ppd);
#else
    LocalFree((HLOCAL)LOWORD(ppd));
#endif

    return nReturn;
}



#ifdef WIN32
//
// Description:
//   This function creates a 32-bit proxy page object for 16-bit page object.
//  The PSP_IS16 flag in psp.dwFlags indicates that this is a proxy object.
//
// Arguments:
//  hpage16 -- Specifies the handle to 16-bit property sheet page object.
//  hinst16 -- Specifies a handle to FreeLibrary16() when page is deleted.
//
//
HPROPSHEETPAGE WINAPI CreateProxyPage(HPROPSHEETPAGE hpage16, HINSTANCE hinst16)
{
    HPROPSHEETPAGE hpage = Alloc(sizeof(PSP));
    PROPSHEETPAGE * ppsp = MapSLFix(hpage16);

    Assert(hpage16 != NULL);

    if (hpage)
    {
    hpage->psp.dwSize = sizeof(hpage->psp);
    if (ppsp)
    {
        // copy the dwFlags so we can reference PSP_HASHELP from the 32 bit side.
        hpage->psp.dwFlags = ppsp->dwFlags | PSP_IS16;
    }
    else
    {
        hpage->psp.dwFlags = PSP_IS16;
    }
    hpage->psp.lParam = (LPARAM)hpage16;
    hpage->psp.hInstance = hinst16;
    }

    if (ppsp)
    {
    UnMapSLFixArray(1, &hpage16);
    }

    return hpage;
}
#endif

// PropertySheet API
//
// This function displays the property sheet described by ppsh.
//
// Since I don't expect anyone to ever check the return value
// (we certainly don't), we need to make sure any provided phpage array
// is always freed with DestroyPropertySheetPage, even if an error occurs.
//
int WINAPI PropertySheet(LPCPROPSHEETHEADER ppsh)
{
    PROPDATA NEAR *ppd;

    //
    // validate header
    //
    if (ppsh->dwSize > sizeof(PROPSHEETHEADER))
    {
        DebugMsg( DM_ERROR, "PropertySheet: dwSize > sizeof( PROPSHEETHEADER )" );
        goto invalid_call;
    }

    if (ppsh->dwSize < (sizeof(PROPSHEETHEADER) - sizeof(PFNPROPSHEETCALLBACK)))
    {
        DebugMsg( DM_ERROR, "PropertySheet: dwSize < sizeof( old PROPSHEETHEADER )" );
        goto invalid_call;
    }

    if (ppsh->dwFlags & ~PSH_ALL)
    {
        DebugMsg( DM_ERROR, "PropertySheet: invalid flags" );
        goto invalid_call;
    }

    // BUGBUG: is this >= for a reason?
    if (ppsh->nPages >= MAXPROPPAGES)
    {
        DebugMsg( DM_ERROR, "PropertySheet: too many pages ( use MAXPROPPAGES )" );
        goto invalid_call;
    }

    ppd = (PROPDATA NEAR *)LocalAlloc(LPTR, sizeof(PROPDATA) + MAXPROPPAGES * sizeof(HPROPSHEETPAGE));
    if (ppd == NULL)
    {
        DebugMsg(DM_ERROR, "failed to alloc property page data");

invalid_call:
        if (!(ppsh->dwFlags & PSH_PROPSHEETPAGE))
        {
        int iPage;

        // Release all page objects in REVERSE ORDER so we can have
        // pages that are dependant on eachother based on the initial
            // order of those pages

            for (iPage = (int)ppsh->nPages - 1; iPage >= 0; iPage--)
            {
        DestroyPropertySheetPage(ppsh->phpage[iPage]);
            }
    }
        return -1;          
    }

    // make a copy of the header so we can party on it

    hmemcpy(&ppd->psh, ppsh, ppsh->dwSize);

    // so we don't have to check later...
    if (!(ppd->psh.dwFlags & PSH_USECALLBACK))
        ppd->psh.pfnCallback = NULL;

    // fix up the page pointer to point to our copy of the page array

    ppd->psh.phpage = (HPROPSHEETPAGE FAR *)((LPBYTE)ppd + sizeof(PROPDATA));

    if (ppd->psh.dwFlags & PSH_PROPSHEETPAGE)
    {
        // for lazy clients convert PROPSHEETPAGE structures into page handles
        int iPage;
    LPCPROPSHEETPAGE ppsp = ppsh->ppsp;

    for (iPage = 0; iPage < (int)ppd->psh.nPages; iPage++)
    {
        ppd->psh.phpage[iPage] = CreatePropertySheetPage(ppsp);
        if (!ppd->psh.phpage[iPage])
        {
        iPage--;
        ppd->psh.nPages--;
        }
        ppsp = (LPCPROPSHEETPAGE)((LPBYTE)ppsp + ppsp->dwSize);      // next PROPSHEETPAGE structure
    }
    }
    else
    {
    // make a copy of the pages passed in, since we will party here
    hmemcpy(ppd->psh.phpage, ppsh->phpage, sizeof(HPROPSHEETPAGE) * ppsh->nPages);
    }
    //
    //  Walk all pages to see if any have help and if so, set the PSH_HASHELP
    //  flag in the header.
    //
    if (!(ppd->psh.dwFlags & PSH_HASHELP))
    {
        int iPage;
    for (iPage = 0; iPage < (int)ppd->psh.nPages; iPage++)
    {
        if (ppd->psh.phpage[iPage]->psp.dwFlags & PSP_HASHELP)
        {
        ppd->psh.dwFlags |= PSH_HASHELP;
        break;
        }
    }
    }

    return _RealPropertySheet(ppd);
}

#ifndef WIN32
#ifndef WIN31

// crap to make this run on M7 user for setup

LPVOID GetUserProc(UINT ord)
{
    return (LPVOID)GetProcAddress(GetModuleHandle("USER"), MAKEINTRESOURCE(ord));
}

// GetMessage32        = USER.820
BOOL WINAPI GetMessage32(LPMSG32 pmsg, HWND hwnd, UINT min, UINT max, BOOL f32)
{
    static BOOL (WINAPI *pfn)(LPMSG32, HWND, UINT, UINT, BOOL) = (LPVOID)-1;

    if (pfn == (LPVOID)-1)
        pfn = GetUserProc(820);

    if (pfn)
        return pfn(pmsg, hwnd, min, max, f32);
    else
        return GetMessage((LPMSG)pmsg, hwnd, min, max);
}

// PeekMessage32       = USER.819
BOOL WINAPI PeekMessage32(LPMSG32 pmsg, HWND hwnd, UINT min, UINT max, UINT flags, BOOL f32)
{
    static BOOL (WINAPI *pfn)(LPMSG32, HWND, UINT, UINT, UINT, BOOL) = (LPVOID)-1;

    if (pfn == (LPVOID)-1)
        pfn = GetUserProc(819);

    if (pfn)
        return pfn(pmsg, hwnd, min, max, flags, f32);
    else
        return PeekMessage((LPMSG)pmsg, hwnd, min, max, flags);
}

// TranslateMessage32  = USER.821
BOOL WINAPI TranslateMessage32(const MSG32 FAR* pmsg, BOOL f32)
{
    static BOOL (WINAPI *pfn)(const MSG32 FAR*, BOOL) = (LPVOID)-1;

    if (pfn == (LPVOID)-1)
        pfn = GetUserProc(821);

    if (pfn)
        return pfn(pmsg, f32);
    else
        return TranslateMessage((LPMSG)pmsg);
}

// DispatchMessage32   = USER.822
LONG WINAPI DispatchMessage32(const MSG32 FAR* pmsg, BOOL f32)
{
    static BOOL (WINAPI *pfn)(const MSG32 FAR*, BOOL) = (LPVOID)-1;

    if (pfn == (LPVOID)-1)
        pfn = GetUserProc(822);

    if (pfn)
        return pfn(pmsg, f32);
    else
        return DispatchMessage((LPMSG)pmsg);
}

// CallMsgFilter32     = USER.823
BOOL WINAPI CallMsgFilter32(LPMSG32 pmsg, int x, BOOL f32)
{
    static BOOL (WINAPI *pfn)(LPMSG32, int, BOOL) = (LPVOID)-1;

    if (pfn == (LPVOID)-1)
        pfn = GetUserProc(823);

    if (pfn)
        return pfn(pmsg, x, f32);
    else
        return CallMsgFilter((LPMSG)pmsg, x);
}

// IsDialogMessage32   = USER.824
BOOL WINAPI IsDialogMessage32(HWND hwnd, LPMSG32 pmsg, BOOL f32)
{
    static BOOL (WINAPI *pfn)(HWND, LPMSG32, BOOL) = (LPVOID)-1;

    if (pfn == (LPVOID)-1)
        pfn = GetUserProc(824);

    if (pfn)
        return pfn(hwnd, pmsg, f32);
    else
        return IsDialogMessage(hwnd, (LPMSG)pmsg);
}

#ifdef FE_IME
// To get internal APIs within USER 
//
LPVOID GetIMMProc(UINT ord)
{
    return (LPVOID)GetProcAddress(GetModuleHandle("USER"), MAKEINTRESOURCE(ord));
}
BOOL WINAPI CreateImmTLink(DWORD idThread) 
{
    static BOOL (WINAPI *pfn)(DWORD) = (LPVOID)-1;
    if (pfn == (LPVOID)-1)
        pfn = GetIMMProc(952);

    if(pfn) return pfn(idThread);
    else    return FALSE;
}
#endif
#endif
#endif
