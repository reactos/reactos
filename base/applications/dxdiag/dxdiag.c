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

////////////////////////////////////////////////////////////////////////////////
// Taken from WinSpy++ 1.7
// http://www.catch22.net/software/winspy
// Copyright (c) 2002 by J Brown
//

//
//	Copied from uxtheme.h
//  If you have this new header, then delete these and
//  #include <uxtheme.h> instead!
//
#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)

//
typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);

//
//	Try to call EnableThemeDialogTexture, if uxtheme.dll is present
//
BOOL EnableDialogTheme(HWND hwnd)
{
    HMODULE hUXTheme;
    ETDTProc fnEnableThemeDialogTexture;

    hUXTheme = LoadLibraryA("uxtheme.dll");

    if(hUXTheme)
    {
        fnEnableThemeDialogTexture =
            (ETDTProc)GetProcAddress(hUXTheme, "EnableThemeDialogTexture");

        if(fnEnableThemeDialogTexture)
        {
            fnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

            FreeLibrary(hUXTheme);
            return TRUE;
        }
        else
        {
            // Failed to locate API!
            FreeLibrary(hUXTheme);
            return FALSE;
        }
    }
    else
    {
        // Not running under XP? Just fail gracefully
        return FALSE;
    }
}

//---------------------------------------------------------------
VOID
DestroyTabCtrlDialogs(PDXDIAG_CONTEXT pContext)
{
    UINT Index;

    /* destroy default dialogs */
    for(Index = 0; Index < sizeof(pContext->hDialogs) / sizeof(HWND); Index++)
    {
       if (pContext->hDialogs[Index])
           DestroyWindow(pContext->hDialogs[Index]);
    }

    /* destroy display dialogs */
    for(Index = 0; Index < pContext->NumDisplayAdapter; Index++)
    {
       if (pContext->hDisplayWnd[Index])
           DestroyWindow(pContext->hDisplayWnd[Index]);
    }

    /* destroy audio dialogs */
    for(Index = 0; Index < pContext->NumSoundAdapter; Index++)
    {
       if (pContext->hSoundWnd[Index])
           DestroyWindow(pContext->hSoundWnd[Index]);
    }

}

//---------------------------------------------------------------
VOID
InsertTabCtrlItem(HWND hDlgCtrl, INT Position, LPWSTR uId)
{
    WCHAR szName[100];
    TCITEMW item;

    /* setup item info */
    memset(&item, 0, sizeof(TCITEMW));
    item.mask = TCIF_TEXT;

    /* load item name */
    if (!HIWORD(uId))
    {
        szName[0] = L'\0';
        if (!LoadStringW(hInst, LOWORD(uId), szName, 100))
            return;
        szName[99] = L'\0';
        item.pszText = szName;
    }
    else
    {
        item.pszText = uId;
    }


    SendMessageW(hDlgCtrl, TCM_INSERTITEM, Position, (LPARAM)&item);
}

VOID
TabCtrl_OnSelChange(PDXDIAG_CONTEXT pContext)
{
    INT Index;
    INT CurSel;

    /* retrieve new page */
    CurSel = TabCtrl_GetCurSel(hTabCtrlWnd);
    if (CurSel < 0 || CurSel > pContext->NumDisplayAdapter + pContext->NumSoundAdapter + 5)
        return;

    /* hide all windows */
    for(Index = 0; Index < 5; Index++)
        ShowWindow(pContext->hDialogs[Index], SW_HIDE);

    for(Index = 0; Index < pContext->NumDisplayAdapter; Index++)
        ShowWindow(pContext->hDisplayWnd[Index], SW_HIDE);

    for(Index = 0; Index < pContext->NumSoundAdapter; Index++)
        ShowWindow(pContext->hSoundWnd[Index], SW_HIDE);


    if (CurSel == 0 || CurSel > pContext->NumDisplayAdapter + pContext->NumSoundAdapter)
    {
        if (CurSel)
            CurSel -= pContext->NumDisplayAdapter + pContext->NumSoundAdapter;
        ShowWindow(pContext->hDialogs[CurSel], SW_SHOW);
        return;
    }

    if (CurSel -1 < pContext->NumDisplayAdapter)
    {
        ShowWindow(pContext->hDisplayWnd[CurSel-1], SW_SHOW);
        return;
    }

    CurSel -= pContext->NumDisplayAdapter + 1;
    ShowWindow(pContext->hSoundWnd[CurSel], SW_SHOW);
}

VOID
InitializeTabCtrl(HWND hwndDlg, PDXDIAG_CONTEXT pContext)
{
    /* get tabctrl */
    hTabCtrlWnd = GetDlgItem(hwndDlg, IDC_TAB_CONTROL);
    pContext->hTabCtrl = hTabCtrlWnd;

    /* create the dialogs */
    pContext->hDialogs[0] = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_SYSTEM_DIALOG), pContext->hTabCtrl, SystemPageWndProc, (LPARAM)pContext); EnableDialogTheme(pContext->hDialogs[0]);
    pContext->hDialogs[1] = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_MUSIC_DIALOG), pContext->hTabCtrl, MusicPageWndProc, (LPARAM)pContext); EnableDialogTheme(pContext->hDialogs[1]);
    pContext->hDialogs[2] = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_INPUT_DIALOG), pContext->hTabCtrl, InputPageWndProc, (LPARAM)pContext); EnableDialogTheme(pContext->hDialogs[2]);
    pContext->hDialogs[3] = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_NETWORK_DIALOG), pContext->hTabCtrl, NetworkPageWndProc, (LPARAM)pContext); EnableDialogTheme(pContext->hDialogs[3]);
    pContext->hDialogs[4] = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_HELP_DIALOG), pContext->hTabCtrl, HelpPageWndProc, (LPARAM)pContext); EnableDialogTheme(pContext->hDialogs[4]);

    /* insert tab ctrl items */
    InsertTabCtrlItem(hTabCtrlWnd, 0, MAKEINTRESOURCEW(IDS_SYSTEM_DIALOG));
    InitializeDisplayAdapters(pContext);
    InitializeDirectSoundPage(pContext);
    InsertTabCtrlItem(hTabCtrlWnd, pContext->NumDisplayAdapter + pContext->NumSoundAdapter + 1, MAKEINTRESOURCEW(IDS_MUSIC_DIALOG));
    InsertTabCtrlItem(hTabCtrlWnd, pContext->NumDisplayAdapter + pContext->NumSoundAdapter + 2, MAKEINTRESOURCEW(IDS_INPUT_DIALOG));
    InsertTabCtrlItem(hTabCtrlWnd, pContext->NumDisplayAdapter + pContext->NumSoundAdapter + 3, MAKEINTRESOURCEW(IDS_NETWORK_DIALOG));
    InsertTabCtrlItem(hTabCtrlWnd, pContext->NumDisplayAdapter + pContext->NumSoundAdapter + 4, MAKEINTRESOURCEW(IDS_HELP_DIALOG));
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

    /* store window handle */
    pContext->hMainDialog = hwndDlg;

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
               INT CurSel;

               /* retrieve current page */
               CurSel = TabCtrl_GetCurSel(hTabCtrlWnd);
               CurSel++;

               /* enable/disable next button */
               EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_NEXT),
                            (CurSel != TabCtrl_GetItemCount(hTabCtrlWnd) - 1));

               /* switch to next tab */
               SendMessageW(hTabCtrlWnd, TCM_SETCURSEL, CurSel, 0L);

               /* show next page */
               TabCtrl_OnSelChange(pContext);
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
               INT CurSel = TabCtrl_GetCurSel(hTabCtrlWnd);

               /* enable/disable next button */
               EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_NEXT),
                            (CurSel != TabCtrl_GetItemCount(hTabCtrlWnd) - 1));

                TabCtrl_OnSelChange(pContext);
            }
            break;
        case WM_DESTROY:
            DestroyTabCtrlDialogs(pContext);
            return DefWindowProc(hwndDlg, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPWSTR     lpCmdLine,
                      int       nCmdShow)
{

    INITCOMMONCONTROLSEX InitControls;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitControls.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_TREEVIEW_CLASSES;
    InitCommonControlsEx(&InitControls);

    hInst = hInstance;

    DialogBox(hInst, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, DxDiagWndProc);

    return 0;
}
