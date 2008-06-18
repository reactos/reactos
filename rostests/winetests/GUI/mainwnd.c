/*
 * PROJECT:     ReactOS Character Map
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        
 * PURPOSE:     main dialog implementation
 * COPYRIGHT:   Copyright 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include <precomp.h>

HINSTANCE hInstance;




static BOOL
OnInitMainDialog(HWND hDlg,
                 LPARAM lParam)
{
    PMAIN_WND_INFO pInfo;
    //HMENU hSysMenu;
    LPWSTR lpAboutText;

    pInfo = (PMAIN_WND_INFO)lParam;

    /* Initialize the main window context */
    pInfo->hMainWnd = hDlg;

    SetWindowLongPtr(hDlg,
                     GWLP_USERDATA,
                     (LONG_PTR)pInfo);

    pInfo->hSmIcon = LoadImageW(hInstance,
                         MAKEINTRESOURCEW(IDI_ICON),
                         IMAGE_ICON,
                         16,
                         16,
                         0);
    if (pInfo->hSmIcon)
    {
         SendMessageW(hDlg,
                      WM_SETICON,
                      ICON_SMALL,
                      (LPARAM)pInfo->hSmIcon);
    }

    pInfo->hBgIcon = LoadImageW(hInstance,
                         MAKEINTRESOURCEW(IDI_ICON),
                         IMAGE_ICON,
                         32,
                         32,
                         0);
    if (pInfo->hBgIcon)
    {
        SendMessageW(hDlg,
                     WM_SETICON,
                     ICON_BIG,
                     (LPARAM)pInfo->hBgIcon);
    }

    return TRUE;
}


static BOOL CALLBACK
MainDlgProc(HWND hDlg,
            UINT Message,
            WPARAM wParam,
            LPARAM lParam)
{
    PMAIN_WND_INFO pInfo;

    /* Get the window context */
    pInfo = (PMAIN_WND_INFO)GetWindowLongPtr(hDlg,
                                             GWLP_USERDATA);
    if (pInfo == NULL && Message != WM_INITDIALOG)
    {
        goto HandleDefaultMessage;
    }

    switch(Message)
    {
        case WM_INITDIALOG:
            return OnInitMainDialog(hDlg, lParam);

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_BROWSE:
                    DialogBoxParamW(hInstance,
                                    MAKEINTRESOURCEW(IDD_TESTBROWSER),
                                    hDlg,
                                    (DLGPROC)BrowseDlgProc,
                                    (LPARAM)pInfo);

                    break;

                case IDOK:
                    EndDialog(hDlg, 0);
                break;
            }
        }
        break;

        case WM_CLOSE:
            if (pInfo->hSmIcon)
                DestroyIcon(pInfo->hSmIcon);
            if (pInfo->hBgIcon)
                DestroyIcon(pInfo->hBgIcon);
            EndDialog(hDlg, 0);
            break;

HandleDefaultMessage:
        default:
            return FALSE;
    }

    return FALSE;
}


INT WINAPI
wWinMain(HINSTANCE hInst,
         HINSTANCE hPrev,
         LPWSTR Cmd,
         int iCmd)
{
    INITCOMMONCONTROLSEX iccx;
    PMAIN_WND_INFO pInfo;
    INT Ret = 1;

    hInstance = hInst;

    ZeroMemory(&iccx, sizeof(INITCOMMONCONTROLSEX));
    iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccx.dwICC = ICC_TAB_CLASSES;
    InitCommonControlsEx(&iccx);

    pInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(MAIN_WND_INFO));
    if (pInfo)
    {
        Ret = (DialogBoxParamW(hInstance,
                               MAKEINTRESOURCEW(IDD_WINETESTGUI),
                               NULL,
                               (DLGPROC)MainDlgProc,
                               (LPARAM)pInfo) == IDOK);

        HeapFree(GetProcessHeap(), 0, pInfo);

    }

    return Ret;
}
