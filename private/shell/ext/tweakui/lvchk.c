/*
 * lvchk - common dialog proc handler for check-listview pages
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  Checklist_OnInitDialog
 *
 *  Walk the items that can live on the checklist.
 *
 *  If the item can't report its state, then don't put it on the list.
 *
 *****************************************************************************/

void PASCAL
Checklist_OnInitDialog(HWND hwnd, PCCHECKLISTITEM rgcli, int ccli,
                       UINT ids, LPVOID pvRef)
{
    TCHAR tsz[MAX_PATH];
    int dids;

    for (dids = 0; dids < ccli; dids++) {
        BOOL f = rgcli[dids].GetCheckValue(rgcli[dids].lParam, pvRef);
        if (f >= 0) {
            LoadString(hinstCur, ids+dids, tsz, cA(tsz));
            LV_AddItem(hwnd, dids, tsz, -1, f);
        }
    }
}

/*****************************************************************************
 *
 *  Checklist_OnApply
 *
 *  Walk the items in the checklist and dork them if they have changed.
 *
 *****************************************************************************/

void PASCAL
Checklist_OnApply(HWND hdlg, PCCHECKLISTITEM rgcli, LPVOID pvRef)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_ICONLV);
    int cItems = ListView_GetItemCount(hwnd);
    LV_ITEM lvi;

    for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++) {
        BOOL fNew, fOld;
        lvi.stateMask = LVIS_STATEIMAGEMASK;
        Misc_LV_GetItemInfo(hwnd, &lvi, lvi.iItem, LVIF_PARAM | LVIF_STATE);
        fNew = LV_IsChecked(&lvi);
        fOld = rgcli[lvi.lParam].GetCheckValue(rgcli[lvi.lParam].lParam,
                                               pvRef);
        if (fOld >= 0 && fNew != fOld) {
            rgcli[lvi.lParam].SetCheckValue(fNew, rgcli[lvi.lParam].lParam,
                                            pvRef);
        }
    }
}
