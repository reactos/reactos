//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       hdwwiz.c
//
//--------------------------------------------------------------------------

#include "hdwwiz.h"
#include <htmlhelp.h>


#define CLICKMODE_LVSTYLEMASK (LVS_EX_ONECLICKACTIVATE | LVS_EX_TRACKSELECT)







LRESULT
OnNotify(
    HWND hDlg,
    PHARDWAREWIZ HardwareWiz,
    NMHDR FAR *pnmhdr
    )
{

    switch (pnmhdr->code) {
        
    case PSN_SETACTIVE:
        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
        HardwareWiz->PrevPage = IDD_HDWTASKS;
        HardwareWiz->DevInst = (DEVINST)0;
        HardwareWiz->TaskInitData = NULL;
        break;

    case PSN_WIZNEXT:

        HardwareWiz->EnterFrom = IDD_HDWTASKS;


        if (IsDlgButtonChecked(hDlg, IDC_HWTASK_ADDNEW)) {

            SetDlgMsgResult(hDlg, WM_NOTIFY, IDD_ADDDEVICE_PNPENUM);
            HardwareWiz->TaskInitData = FindTaskInitData(IDC_HWTASK_ADDNEW);
            break;

        } else if (IsDlgButtonChecked(hDlg, IDC_HWTASK_REMOVE)) {
        
            SetDlgMsgResult(hDlg, WM_NOTIFY, IDD_REMDEVICE_CHOICE);
            break;

        } else {
        
            SetDlgMsgResult(hDlg, WM_NOTIFY, -1);
            break;
        }

        break;


    case PSN_WIZBACK:
        SetDlgMsgResult(hDlg, WM_NOTIFY, IDD_HDWINTRO);
        break;

    default:
        break;
    }

    return 0;
}



//
// Wizard dialog proc.
//
LRESULT CALLBACK
HdwTasksDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:

   Main page of hardware wizard displaying list of tasks to perform.

Arguments:

   standard stuff.



Return Value:

   LRESULT

--*/

{
    PHARDWAREWIZ HardwareWiz;

    if (message == WM_INITDIALOG) {

        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        HardwareWiz = (PHARDWAREWIZ) lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        
        CheckRadioButton(hDlg,
                         IDC_HWTASK_ADDNEW,
                         IDC_HWTASK_REMOVE,
                         IDC_HWTASK_ADDNEW
                         );

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);


    switch (message) {

    case WM_DESTROY:

        if (HardwareWiz) {

            if (HardwareWiz->hfontTextNormal) {
                DeleteObject(HardwareWiz->hfontTextNormal);
                HardwareWiz->hfontTextNormal = NULL;
                }

            if (HardwareWiz->hfontTextHigh) {
                DeleteObject(HardwareWiz->hfontTextHigh);
                HardwareWiz->hfontTextHigh = NULL;
                }
            }

        break;

    case WM_NOTIFY:
        OnNotify(hDlg, HardwareWiz, (NMHDR FAR *)lParam);
        break;

    case WM_SYSCOLORCHANGE:
        HdwWizPropagateMessage(hDlg, message, wParam, lParam);
        break;


    case WM_SETCURSOR:
       return FALSE;

    default:
        return FALSE;
    }

    return TRUE;
}



BOOL
InitHdwIntroDlgProc(
   HWND hDlg,
   PHARDWAREWIZ HardwareWiz
   )
{
   HWND hwnd;
   HFONT hfont;
   HICON hIcon;
   LOGFONT LogFont, LogFontOriginal;
   int FontSize, PtsPixels;

   //
   // Set the windows icons, so that we have the correct icon
   // in the alt-tab menu.
   //

   hwnd = GetParent(hDlg);
   hIcon = LoadIcon(hHdwWiz,MAKEINTRESOURCE(IDI_HDWWIZICON));
   
   if (hIcon) {

       SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
       SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
   }

   hfont = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_HDWNAME), WM_GETFONT, 0, 0);
   GetObject(hfont, sizeof(LogFont), &LogFont);
   LogFontOriginal = LogFont;

   HardwareWiz->cyText = LogFont.lfHeight;

   if (HardwareWiz->cyText < 0) {

       HardwareWiz->cyText = -HardwareWiz->cyText;
   }

   HardwareWiz->hfontTextNormal = CreateFontIndirect(&LogFont);

   LogFont = LogFontOriginal;
   LogFont.lfUnderline = TRUE;
   HardwareWiz->hfontTextHigh = CreateFontIndirect(&LogFont);

   LogFont = LogFontOriginal;
   LogFont.lfWeight = FW_BOLD;
   HardwareWiz->hfontTextBold = CreateFontIndirect(&LogFont);

   LogFont = LogFontOriginal;
   LogFont.lfWeight = FW_BOLD;

   // bump up font height.
   PtsPixels = GetDeviceCaps(GetDC(hDlg), LOGPIXELSY);
   FontSize = 12;
   LogFont.lfHeight = 0 - (PtsPixels * FontSize / 72);

   HardwareWiz->hfontTextBigBold = CreateFontIndirect(&LogFont);

   if (!HardwareWiz->hfontTextNormal ||
       !HardwareWiz->hfontTextHigh   ||
       !HardwareWiz->hfontTextBold   ||
       !HardwareWiz->hfontTextBigBold )
   {
       return FALSE;
   }

   SetWindowFont(GetDlgItem(hDlg, IDC_HDWNAME), HardwareWiz->hfontTextBigBold, TRUE);

   return TRUE;
}





//
// Wizard intro dialog proc.
//
LRESULT CALLBACK
HdwIntroDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:


Arguments:

   standard stuff.



Return Value:

   LRESULT

--*/

{
    PHARDWAREWIZ HardwareWiz;

    if (message == WM_INITDIALOG) {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        HardwareWiz = (PHARDWAREWIZ) lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);

        if (!InitHdwIntroDlgProc(hDlg, HardwareWiz)) {
            return FALSE;
            }

        return TRUE;
        }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message) {

    case WM_DESTROY:
        if (HardwareWiz->hfontTextNormal) {
            DeleteObject(HardwareWiz->hfontTextNormal);
            HardwareWiz->hfontTextNormal = NULL;
            }

        if (HardwareWiz->hfontTextHigh) {
            DeleteObject(HardwareWiz->hfontTextHigh);
            HardwareWiz->hfontTextHigh = NULL;
            }

        if (HardwareWiz->hfontTextBold) {
            DeleteObject(HardwareWiz->hfontTextBold);
            HardwareWiz->hfontTextBold = NULL;
            }

        if (HardwareWiz->hfontTextBigBold) {
            DeleteObject(HardwareWiz->hfontTextBigBold);
            HardwareWiz->hfontTextBigBold = NULL;
            }

        break;

    case WM_COMMAND:
        break;

    case WM_NOTIFY: {
        NMHDR FAR *pnmhdr = (NMHDR FAR *)lParam;

        switch (pnmhdr->code) {
        case PSN_SETACTIVE: {
                
            DWORD ProblemDetected;
            int  nCmdShow;

            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
            HardwareWiz->PrevPage = IDD_HDWINTRO;

        }
        break;

        case PSN_WIZNEXT:
            HardwareWiz->EnterFrom = IDD_HDWINTRO;
            break;
        }
    }
    break;

    case WM_SYSCOLORCHANGE:
        HdwWizPropagateMessage(hDlg, message, wParam, lParam);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

