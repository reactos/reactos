/*
 * ie4 - IE4 settings
 */

#include "tweakui.h"

#pragma message("Add help for IE4!")

#pragma BEGIN_CONST_DATA

const static DWORD CODESEG rgdwHelp[] = {
        IDC_SETTINGSGROUP,      IDH_GROUP,
        IDC_LISTVIEW,           IDH_IE4LV,
        0,                      0,
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  IE4_GetRest
 *
 *      Read a restriction.  The first character of the restriction is
 *      `+' if it is positive sense.  All restrictions default to 0.
 *
 *      Always returns exactly 0 or 1.
 *
 *****************************************************************************/

BOOL PASCAL
IE4_GetRest(LPARAM lParam, LPVOID pvRef)
{
    LPCTSTR ptszRest = (LPCTSTR)lParam;

    if (ptszRest[0] == TEXT('+')) {
        return !GetRestriction(ptszRest+1);
    } else {
        return GetRestriction(ptszRest);
    }
}

/*****************************************************************************
 *
 *  IE4_SetSmooth
 *
 *      Set the new restriction setting.
 *
 *      The first character of the restriction is
 *      `+' if it is positive sense.
 *
 *****************************************************************************/

void PASCAL
IE4_SetRest(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    LPCTSTR ptszRest = (LPCTSTR)lParam;
    PBOOL pf = pvRef;

    if (pf) {
        *pf = (BOOL)ptszRest;
    }

    if (ptszRest[0] == TEXT('+')) {
        f = !f;
        ptszRest++;
    }
    SetRestriction(ptszRest, f);

}

/*
 *  Note that this needs to be in sync with the IDS_IE4 strings.
 */
CHECKLISTITEM c_rgcliIE4[] = {
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszNoInternetIcon,        },
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszNoRecentDocsHistory,   },
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszNoRecentDocsMenu,      },
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszNoActiveDesktop,       },
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszNoActiveDesktopChanges,},
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszNoFavoritesMenu,       },
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszClearRecentDocsOnExit, },
};

/*****************************************************************************
 *
 *  IE4_OnWhatsThis
 *
 *****************************************************************************/

void PASCAL
IE4_OnWhatsThis(HWND hwnd, int iItem)
{
    LV_ITEM lvi;

    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_PARAM);

    WinHelp(hwnd, c_tszMyHelp, HELP_CONTEXTPOPUP,
            IDH_SHOWINTERNET + lvi.lParam);
}

/*****************************************************************************
 *
 *  IE4_OnCommand
 *
 *      Ooh, we got a command.
 *
 *****************************************************************************/

BOOL PASCAL
IE4_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {

    }

    return 0;
}

/*****************************************************************************
 *
 *  IE4_OnInitDialog
 *
 *  Initialize the listview with the current restrictions.
 *
 *****************************************************************************/

BOOL PASCAL
IE4_OnInitDialog(HWND hwnd)
{
#if 0
    HWND hdlg = GetParent(hwnd);
    TCHAR tsz[MAX_PATH];
    int dids;

    for (dids = 0; dids < cA(c_rgrest); dids++) {
        BOOL fState;

        LoadString(hinstCur, IDS_IE4+dids, tsz, cA(tsz));

        fState = GetRestriction(c_rgrest[dids].ptsz);
        LV_AddItem(hwnd, dids, tsz, -1, fState);

    }
#endif

    Checklist_OnInitDialog(hwnd, c_rgcliIE4, cA(c_rgcliIE4), IDS_IE4, 0);

    return 1;
}

/*****************************************************************************
 *
 *  IE4_OnApply
 *
 *****************************************************************************/

void PASCAL
IE4_OnApply(HWND hdlg)
{
    BOOL fChanged = FALSE;

    Checklist_OnApply(hdlg, c_rgcliIE4, &fChanged);

    if (fChanged) {
        PIDL pidl;

        /*
         *  Tell the shell that we changed the policies.
         */
        SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                    (LPARAM)c_tszIE4RegKeyChange);

        /*
         *  Tickle the Start Menu folder to force the Start Menu
         *  to rebuild with the new policies in effect.
         */
        if (SUCCEEDED(SHGetSpecialFolderLocation(hdlg,
                                                 CSIDL_STARTMENU, &pidl))) {
            SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidl, 0);
            Ole_Free(pidl);
        }
    }
}

/*****************************************************************************
 *
 *  Oh yeah, we need this too.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

LVV lvvIE4 = {
    IE4_OnCommand,
    0,                          /* IE4_OnInitContextMenu */
    0,                          /* IE4_Dirtify */
    0,                          /* IE4_GetIcon */
    IE4_OnInitDialog,
    IE4_OnApply,
    0,                          /* IE4_OnDestroy */
    0,                          /* IE4_OnSelChange */
    6,                          /* iMenu */
    rgdwHelp,
    0,                          /* Double-click action */
    lvvflCanCheck,              /* We need check boxes */
    {
        { IDC_WHATSTHIS,        IE4_OnWhatsThis },
        { 0,                    0 },
    },
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

BOOL EXPORT
IE4_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    return LV_DlgProc(&lvvIE4, hdlg, wm, wParam, lParam);
}
