//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       intro.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"
#include <regstr.h>


INT_PTR
InitIntroDlgProc(
    HWND hDlg,
    PNEWDEVWIZ NewDevWiz
    )
{
    HFONT hfont;
    HICON hIcon;
    int FontSize, PtsPixels;
    HWND hwndParentDlg;
    HWND hwndList;
    LV_COLUMN lvcCol;
    LOGFONT LogFont;
    TCHAR Buffer[64];

    hIcon = LoadIcon(hNewDev,MAKEINTRESOURCE(IDI_NEWDEVICEICON));
    
    if (hIcon) {

        hwndParentDlg = GetParent(hDlg);
        SendMessage(hwndParentDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hwndParentDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    hfont = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_INTRO_MSG1), WM_GETFONT, 0, 0);
    GetObject(hfont, sizeof(LogFont), &LogFont);
    LogFont.lfWeight = FW_BOLD;

    // bump up font height.
    PtsPixels = GetDeviceCaps(GetDC(hDlg), LOGPIXELSY);
    FontSize = 12;
    LogFont.lfHeight = 0 - (PtsPixels * FontSize / 72);

    NewDevWiz->hfontTextBigBold = CreateFontIndirect(&LogFont);

    if (NewDevWiz->hfontTextBigBold ) {
        
        SetWindowFont(GetDlgItem(hDlg, IDC_INTRO_MSG1), NewDevWiz->hfontTextBigBold, TRUE);
    }

    if (NDWTYPE_UPDATE == NewDevWiz->WizardType) {

        SetDlgText(hDlg, IDC_INTRO_MSG1, IDS_INTRO_MSG1_UPGRADE, IDS_INTRO_MSG1_UPGRADE);
        SetDlgText(hDlg, IDC_INTRO_MSG2, IDS_INTRO_MSG2_UPGRADE, IDS_INTRO_MSG2_UPGRADE);
    
    } else {

        SetDlgText(hDlg, IDC_INTRO_MSG1, IDS_INTRO_MSG1_NEW, IDS_INTRO_MSG1_NEW);
        SetDlgText(hDlg, IDC_INTRO_MSG2, IDS_INTRO_MSG2_NEW, IDS_INTRO_MSG2_NEW);
    }

    return TRUE;
}




INT_PTR CALLBACK
IntroDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);


    if (wMsg == WM_INITDIALOG) {
        
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

        if (!InitIntroDlgProc(hDlg, NewDevWiz)) {
            
            return FALSE;
        }

        return TRUE;
    }


    switch (wMsg)  {
       
    case WM_COMMAND:
        break;

    case WM_DESTROY:

        if (NewDevWiz->hfontTextBigBold ) {

            DeleteObject(NewDevWiz->hfontTextBigBold);
            NewDevWiz->hfontTextBigBold = NULL;
        }

        break;


    case WM_NOTIFY:
       
        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
            NewDevWiz->PrevPage = IDD_NEWDEVWIZ_INTRO;
            break;

        case PSN_RESET:
            NewDevWiz->Cancelled = TRUE;
            break;

        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, IDD_DRVUPD);
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}
