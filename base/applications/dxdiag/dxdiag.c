/*
 * PROJECT:     ReactX Diagnosis Application
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/dxdiag/dxdiag.c
 * PURPOSE:     ReactX diagnosis application entry
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald
 *
 */

#include "precomp.h"

/* globals */
HINSTANCE hInst = 0;
HWND hTabCtrlWnd;

//---------------------------------------------------------------
VOID
DestroyTabCtrlDialogs(PDXDIAG_CONTEXT pContext)
{
    UINT Index;

    for(Index = 0; Index < 7; Index++)
    {
       if (pContext->hDialogs[Index])
           DestroyWindow(pContext->hDialogs[Index]);
    }
}

//---------------------------------------------------------------
VOID
InsertTabCtrlItem(HWND hDlgCtrl, INT Position, UINT uId)
{
    WCHAR szName[100];
    TCITEMW item;

    /* load item name */
    szName[0] = L'\0';
    if (!LoadStringW(hInst, uId, szName, 100))
        return;
    szName[99] = L'\0';

    /* setup item info */
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szName;

    SendMessageW(hDlgCtrl, TCM_INSERTITEM, Position, (LPARAM)&item);
}

VOID
TabCtrl_OnSelChange(PDXDIAG_CONTEXT pContext)
{
    INT Index;
    INT CurSel;

    /* retrieve new page */
    CurSel = TabCtrl_GetCurSel(hTabCtrlWnd);
    if (CurSel < 0 || CurSel > 7)
        return;

    /* show active page */
    for(Index = 0; Index < 7; Index++)
    {
         if (Index == CurSel)
             ShowWindow(pContext->hDialogs[Index], SW_SHOW);
         else
             ShowWindow(pContext->hDialogs[Index], SW_HIDE);
    }
}


VOID
InitializeTabCtrl(HWND hwndDlg, PDXDIAG_CONTEXT pContext)
{
    /* create the dialogs */
    pContext->hDialogs[0] = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_SYSTEM_DIALOG), hwndDlg, SystemPageWndProc, (LPARAM)pContext);
    pContext->hDialogs[1] = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_DISPLAY_DIALOG), hwndDlg, DisplayPageWndProc, (LPARAM)pContext);
    pContext->hDialogs[2] = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_SOUND_DIALOG), hwndDlg, SoundPageWndProc, (LPARAM)pContext);
    pContext->hDialogs[3] = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_MUSIC_DIALOG), hwndDlg, MusicPageWndProc, (LPARAM)pContext);
    pContext->hDialogs[4] = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_INPUT_DIALOG), hwndDlg, InputPageWndProc, (LPARAM)pContext);
    pContext->hDialogs[5] = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_NETWORK_DIALOG), hwndDlg, NetworkPageWndProc, (LPARAM)pContext);
    pContext->hDialogs[6] = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_HELP_DIALOG), hwndDlg, HelpPageWndProc, (LPARAM)pContext);

    /* insert tab ctrl items */
    hTabCtrlWnd = GetDlgItem(hwndDlg, IDC_TAB_CONTROL);
    InsertTabCtrlItem(hTabCtrlWnd, 0, IDS_SYSTEM_DIALOG);
    InsertTabCtrlItem(hTabCtrlWnd, 1, IDS_DISPLAY_DIALOG);
    InsertTabCtrlItem(hTabCtrlWnd, 2, IDS_SOUND_DIALOG);
    InsertTabCtrlItem(hTabCtrlWnd, 3, IDS_MUSIC_DIALOG);
    InsertTabCtrlItem(hTabCtrlWnd, 4, IDS_INPUT_DIALOG);
    InsertTabCtrlItem(hTabCtrlWnd, 5, IDS_NETWORK_DIALOG);
    InsertTabCtrlItem(hTabCtrlWnd, 6, IDS_HELP_DIALOG);

    TabCtrl_OnSelChange(pContext);
}

VOID
InitializeDxDiagDialog(HWND hwndDlg)
{
    PDXDIAG_CONTEXT pContext;
    HICON hIcon;

    pContext = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DXDIAG_CONTEXT));
    if (!pContext)
        return;

    /* store the context */
    SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pContext);

    /* initialize the tab ctrl */
    InitializeTabCtrl(hwndDlg, pContext);

    /* load application icon */
    hIcon = LoadImageW(hInst, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, 16, 16, 0);
    if (!hIcon)
        return;
    /* display icon */
    SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
}


INT_PTR CALLBACK
DxDiagWndProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR         pnmh;
    PDXDIAG_CONTEXT pContext;

    pContext = (PDXDIAG_CONTEXT)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (message)
    {
        case WM_INITDIALOG:
            InitializeDxDiagDialog(hwndDlg);
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BUTTON_SAVE_INFO)
            {
               //TODO
               /* handle save information */
               return TRUE;
            }

            if (LOWORD(wParam) == IDC_BUTTON_NEXT)
            {
               //TODO
               /* handle next button */
               return TRUE;
            }

            if (LOWORD(wParam) == IDC_BUTTON_HELP)
            {
               //TODO
               /* handle help button */
               return TRUE;
            }

            if (LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDC_BUTTON_EXIT) {
                EndDialog(hwndDlg, LOWORD(wParam));
                return TRUE;
            }
            break;

        case WM_NOTIFY:
            pnmh = (LPNMHDR)lParam;
            if ((pnmh->hwndFrom == hTabCtrlWnd) && (pnmh->idFrom == IDC_TAB_CONTROL) && (pnmh->code == TCN_SELCHANGE))
            {
                TabCtrl_OnSelChange(pContext);
            }
            break;
        case WM_DESTROY:
            DestroyTabCtrlDialogs(pContext);
            return DefWindowProc(hwndDlg, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

    INITCOMMONCONTROLSEX InitControls;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitControls.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&InitControls);

    hInst = hInstance;
 
    DialogBox(hInst, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, (DLGPROC) DxDiagWndProc);
  
    return 0;
}
