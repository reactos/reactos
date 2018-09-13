//
//  PickIcon.C
//
//  Copyright (C) Microsoft, 1994,1995 All Rights Reserved.
//
//  History:
//  ral 6/23/94 - First pass
//  3/20/95  [stevecat] - NT port & real clean up, unicode, etc.
//
//
#include "appwiz.h"


//
//  BUGBUG -- Size?
//
#define MAX_ICONS   75


//
//  Adds icons to the list box.
//

void PutIconsInList(HWND hLB, LPWIZDATA lpwd)
{
    HICON   rgIcons[MAX_ICONS];
    int     iTempIcon;
    int     cIcons;
    HCURSOR hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    RECT    rc;
    int     cy;

    ListBox_SetColumnWidth(hLB, g_cxIcon+12);

    //
    // compute the height of the listbox based on icon dimensions
    //

    GetWindowRect(hLB, &rc);

    cy = g_cyIcon + GetSystemMetrics(SM_CYHSCROLL) + GetSystemMetrics(SM_CYEDGE) * 3;

    SetWindowPos(hLB, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    ListBox_ResetContent(hLB);

    SendMessage(hLB, WM_SETREDRAW, FALSE, 0L);

#ifdef DEBUG
    {
    //
    //  This is necessary for Unicode (i.e. NT) builds because the shell32
    //  library does not support ShellMessageBoxA and W versions.
    //

    TCHAR szTemp[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0, lpwd->PropPrg.achIconFile, -1, szTemp, ARRAYSIZE(szTemp));

    WIZERROR(szTemp);

    }
#endif  //  DEBUG

    cIcons = (int)ExtractIconExA(lpwd->PropPrg.achIconFile, 0, rgIcons, NULL, MAX_ICONS);

    for (iTempIcon = 0; iTempIcon < cIcons; iTempIcon++)
    {
        ListBox_AddString(hLB, rgIcons[iTempIcon]);
    }

    ListBox_SetCurSel(hLB, 0);

    SendMessage(hLB, WM_SETREDRAW, TRUE, 0L);
    InvalidateRect(hLB, NULL, TRUE);

    SetCursor(hcurOld);
}


//
//
//

void PickIconInitDlg(HWND hDlg, LPARAM lParam)
{
    LPPROPSHEETPAGE lpp = (LPPROPSHEETPAGE)lParam;
    LPWIZDATA lpwd = InitWizSheet(hDlg, lParam, 0);

    PutIconsInList(GetDlgItem(hDlg, IDC_ICONLIST), lpwd);
}


//
//  Returns TRUE if a vaild icon is selected.
//

BOOL PickIconNextPressed(LPWIZDATA lpwd)
{
    int iIconIndex = ListBox_GetCurSel(GetDlgItem(lpwd->hwnd, IDC_ICONLIST));

    lpwd->PropPrg.wIconIndex = (WORD)iIconIndex;

    return(iIconIndex != LB_ERR);
}


//
//
//

BOOL CALLBACK PickIconDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpp = (LPPROPSHEETPAGE)(GetWindowLong(hDlg, DWL_USER));
    LPWIZDATA lpwd = lpp ? (LPWIZDATA)lpp->lParam : NULL;

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
            case PSN_SETACTIVE:

                    //
                    // If PIFMGR has assigned an icon for this app then
                    // we'll skip it.  This condition only happens when
                    // creating a shortcut to a single MS-DOS session.
                    //

                    if (lpwd->dwFlags & WDFLAG_APPKNOWN)
                    {
                        SetDlgMsgResult(hDlg, WM_NOTIFY, -1);
                    }
                    else
                    {
                        lpwd->hwnd = hDlg;
                        PropSheet_SetWizButtons(GetParent(hDlg),
                                               (lpwd->dwFlags & WDFLAG_SINGLEAPP) ?
                                                PSWIZB_BACK | PSWIZB_NEXT :
                                                PSWIZB_BACK | PSWIZB_FINISH);
                    }
                    break;

                case PSN_WIZNEXT:
                    if (!PickIconNextPressed(lpwd))
                    {
                        SetDlgMsgResult(hDlg, WM_NOTIFY, -1);
                    }
                    break;

                case PSN_WIZFINISH:
                    if (!(PickIconNextPressed(lpwd) && CreateLink(lpwd)))
                    {
                        SetDlgMsgResult(hDlg, WM_NOTIFY, -1);
                    }
                    break;

                case PSN_RESET:
                    CleanUpWizData(lpwd);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            PickIconInitDlg(hDlg, lParam);
            break;

        case WM_COMMAND:
            if ((GET_WM_COMMAND_ID(wParam, lParam) == IDC_ICONLIST) &&
                ((GET_WM_COMMAND_CMD(wParam, lParam) == LBN_DBLCLK)))
            {
                PropSheet_PressButton(GetParent(hDlg),
                                      (lpwd->dwFlags & WDFLAG_SINGLEAPP) ?
                                                 PSBTN_NEXT : PSBTN_FINISH);
            }
            break;

        //
        // owner draw messages for icon listbox
        //

        case WM_DRAWITEM:
            #define lpdi ((DRAWITEMSTRUCT FAR *)lParam)

            if (lpdi->itemState & ODS_SELECTED)
                SetBkColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
            else
                SetBkColor(lpdi->hDC, GetSysColor(COLOR_WINDOW));

            //
            // repaint the selection state
            //

            ExtTextOut(lpdi->hDC, 0, 0, ETO_OPAQUE, &lpdi->rcItem, NULL, 0, NULL);

            //
            // draw the icon
            //

            if ((int)lpdi->itemID >= 0)
              DrawIcon(lpdi->hDC, (lpdi->rcItem.left + lpdi->rcItem.right - g_cxIcon) / 2,
                                  (lpdi->rcItem.bottom + lpdi->rcItem.top - g_cyIcon) / 2, (HICON)lpdi->itemData);

            // InflateRect(&lpdi->rcItem, -1, -1);

            //
            // if it has the focus, draw the focus
            //

            if (lpdi->itemState & ODS_FOCUS)
                DrawFocusRect(lpdi->hDC, &lpdi->rcItem);

            #undef lpdi
            break;

        case WM_MEASUREITEM:
            #define lpmi ((MEASUREITEMSTRUCT FAR *)lParam)

            lpmi->itemWidth = g_cxIcon + 12;
            lpmi->itemHeight = g_cyIcon + 4;

            #undef lpmi
            break;

        case WM_DELETEITEM:
            #define lpdi ((DELETEITEMSTRUCT FAR *)lParam)

            DestroyIcon((HICON)lpdi->itemData);

            #undef lpdi
            break;

        default:
            return FALSE;

    }
    return TRUE;
}
