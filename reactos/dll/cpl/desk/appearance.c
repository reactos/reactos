/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/appearance.c
 * PURPOSE:         Appearance property page
 * 
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 *                  Eric Kohl
 */

#include "desk.h"
#include "preview.h"

typedef struct _GLOBAL_DATA
{
    INT nItem;
} GLOBAL_DATA, *PGLOBAL_DATA;


static VOID
OnInitDialog(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    TCHAR szBuffer[256];
    UINT i, idx;

    /* Set the item names */
    for (i = IDS_ITEM_FIRST; i < IDS_ITEM_LAST; i++)
    {
        LoadString(hApplet, i, szBuffer, 256);
        idx = SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_UI_ITEM, CB_ADDSTRING, 0, (LPARAM)szBuffer);
        if (idx != CB_ERR)
        {
            SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_UI_ITEM, CB_SETITEMDATA, (WPARAM)idx, (LPARAM)i - IDS_ITEM_FIRST);
        }
    }

    pGlobalData->nItem = IDX_DESKTOP;
    SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_UI_ITEM, CB_SETCURSEL, pGlobalData->nItem, 0);
}


static VOID
OnItemChange(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    INT nSelection, nIdx;

    nIdx = SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_UI_ITEM, CB_GETCURSEL, 0, 0);
    if (nIdx == CB_ERR)
        return;

    nSelection = SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_UI_ITEM, CB_GETITEMDATA, (WPARAM)nIdx, 0);
    if (nSelection == CB_ERR)
        return;

    pGlobalData->nItem = nSelection;

    switch (nSelection)
    {
        case IDX_SCROLLBAR:
            EnableWindow(GetDlgItem(hwndDlg, IDC_APPEAR_SIZE), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_APPEAR_SIZE_UPDOWN), TRUE);
            SendDlgItemMessage(hwndDlg, IDC_APPEAR_SIZE_UPDOWN, UDM_SETPOS, 0,
                               (LPARAM)MAKELONG((short)SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_GETCXSCROLLBAR, 0, 0), 0));
            break;

        case IDX_MENU:
            EnableWindow(GetDlgItem(hwndDlg, IDC_APPEAR_SIZE), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_APPEAR_SIZE_UPDOWN), TRUE);
            SendDlgItemMessage(hwndDlg, IDC_APPEAR_SIZE_UPDOWN, UDM_SETPOS, 0,
                               (LPARAM)MAKELONG((short)SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_GETCYMENU, 0, 0), 0));
            break;

        case IDX_INACTIVE_BORDER:
        case IDX_ACTIVE_BORDER:
            EnableWindow(GetDlgItem(hwndDlg, IDC_APPEAR_SIZE), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_APPEAR_SIZE_UPDOWN), TRUE);
            SendDlgItemMessage(hwndDlg, IDC_APPEAR_SIZE_UPDOWN, UDM_SETPOS, 0,
                               (LPARAM)MAKELONG((short)SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_GETCYSIZEFRAME, 0, 0), 0));
            break;

        case IDX_INACTIVE_CAPTION:
        case IDX_ACTIVE_CAPTION:
            EnableWindow(GetDlgItem(hwndDlg, IDC_APPEAR_SIZE), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_APPEAR_SIZE_UPDOWN), TRUE);
            SendDlgItemMessage(hwndDlg, IDC_APPEAR_SIZE_UPDOWN, UDM_SETPOS, 0,
                               (LPARAM)MAKELONG((short)SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_GETCYCAPTION, 0, 0), 0));
            break;

        case IDX_CAPTION_BUTTON:
            EnableWindow(GetDlgItem(hwndDlg, IDC_APPEAR_SIZE), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_APPEAR_SIZE_UPDOWN), TRUE);
            SendDlgItemMessage(hwndDlg, IDC_APPEAR_SIZE_UPDOWN, UDM_SETPOS, 0,
                               (LPARAM)MAKELONG((short)SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_GETCYCAPTION, 0, 0), 0));
            break;

        default:
            SetDlgItemText(hwndDlg, IDC_APPEAR_SIZE, _T(""));
            EnableWindow(GetDlgItem(hwndDlg, IDC_APPEAR_SIZE), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_APPEAR_SIZE_UPDOWN), FALSE);

            break;
    }

}


INT_PTR CALLBACK
AppearancePageProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBAL_DATA)HeapAlloc(GetProcessHeap(),
                                                  HEAP_ZERO_MEMORY,
                                                  sizeof(GLOBAL_DATA));
            if (!pGlobalData)
                return -1;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);
            OnInitDialog(hwndDlg, pGlobalData);
            OnItemChange(hwndDlg, pGlobalData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_APPEARANCE_UI_ITEM:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        OnItemChange(hwndDlg, pGlobalData);
                    }
                    break;

                case IDC_APPEAR_SIZE:
                    if (pGlobalData && HIWORD(wParam) == EN_CHANGE)
                    {
                        int i = (int)LOWORD(SendDlgItemMessage(hwndDlg, IDC_APPEAR_SIZE_UPDOWN, UDM_GETPOS,0,0L));

                        switch (pGlobalData->nItem)
                        {
                            case IDX_INACTIVE_CAPTION:
                            case IDX_ACTIVE_CAPTION:
                            case IDX_CAPTION_BUTTON:
                                SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCYCAPTION, 0, i);
                                break;

                            case IDX_MENU:
                                SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCYMENU, 0, i);
                                break;

                            case IDX_SCROLLBAR:
                                SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCXSCROLLBAR, 0, i);
                                break;

                            case IDX_INACTIVE_BORDER:
                            case IDX_ACTIVE_BORDER:
                                SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCYSIZEFRAME, 0, i);
                                break;
                        }
                    }
                    break;
            }
            break;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pGlobalData);
            break;

        case WM_USER:
            SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_UI_ITEM, CB_SETCURSEL, lParam, 0);
            OnItemChange(hwndDlg, pGlobalData);
            break;
    }

    return FALSE;
}


