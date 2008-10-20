/*
 * PROJECT:     ReactOS API Test GUI
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        
 * PURPOSE:     main dialog implementation
 * COPYRIGHT:   Copyright 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include <precomp.h>
#include <io.h>

#define BUFSIZE 4096

HINSTANCE hInstance;

WCHAR szPipeName[] = L"\\\\.\\pipe\\winetest_pipe";

typedef int (_cdecl *RUNTEST)(char **);


VOID
CreateClientProcess(PMAIN_WND_INFO pInfo,
                    LPWSTR lpExePath)
{

}


static BOOL
OnInitMainDialog(HWND hDlg,
                 LPARAM lParam)
{
    PMAIN_WND_INFO pInfo;

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

static VOID
RunSelectedTest(PMAIN_WND_INFO pInfo)
{
    HWND hRunCmd;
    WCHAR szTextCmd[MAX_RUN_CMD];
    LPWSTR lpExePath;
    INT sel;

    hRunCmd = GetDlgItem(pInfo->hMainWnd, IDC_TESTSELECTION);

    sel = SendMessageW(hRunCmd,
                       CB_GETCURSEL,
                       0,
                       0);
    if (sel != CB_ERR)
    {
        if (SendMessageW(hRunCmd,
                         CB_GETLBTEXT,
                         sel,
                         (LPARAM)szTextCmd) != CB_ERR)
        {
            lpExePath = (LPWSTR)SendMessage(hRunCmd,
                                            CB_GETITEMDATA,
                                            0,
                                            0);
            if (lpExePath)
            {
                CreateClientProcess(pInfo, lpExePath);
            }
        }
    }
}

static VOID
AddTestToCombo(PMAIN_WND_INFO pInfo)
{
    HWND hRunCmd;
    LPWSTR lpExePath;
    INT len;

    hRunCmd = GetDlgItem(pInfo->hMainWnd, IDC_TESTSELECTION);
    if (hRunCmd)
    {
        SendMessageW(hRunCmd,
                     CB_INSERTSTRING,
                     0,
                     (LPARAM)pInfo->SelectedTest.szRunString);

        len = (wcslen(pInfo->SelectedTest.szSelectedExe) + 1) * sizeof(WCHAR);
        lpExePath = HeapAlloc(GetProcessHeap(), 0, len);
        if (lpExePath)
        {
            wcsncpy(lpExePath,
                    pInfo->SelectedTest.szSelectedExe,
                    len / sizeof(WCHAR));
        }

        SendMessageW(hRunCmd,
                     CB_SETITEMDATA,
                     0,
                     (LPARAM)lpExePath);
        SendMessageW(hRunCmd,
                     CB_SETCURSEL,
                     0,
                     0);
    }
}

static VOID
FreeTestCmdStrings(PMAIN_WND_INFO pInfo)
{
    HWND hRunCmd;
    LPWSTR lpExePath;
    INT cnt, i;

    hRunCmd = GetDlgItem(pInfo->hMainWnd, IDC_TESTSELECTION);

    cnt = SendMessageW(hRunCmd,
                       CB_GETCOUNT,
                       0,
                       0);
    if (cnt != CB_ERR)
    {
        for (i = 0; i < cnt; i++)
        {
            lpExePath = (LPWSTR)SendMessage(hRunCmd,
                                            CB_GETITEMDATA,
                                            i,
                                            0);
            if (lpExePath)
            {
                HeapFree(GetProcessHeap(), 0, lpExePath);
            }
        }
    }
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
                {
                    INT_PTR ret;

                    ret = DialogBoxParamW(hInstance,
                                          MAKEINTRESOURCEW(IDD_TESTBROWSER),
                                          hDlg,
                                          (DLGPROC)BrowseDlgProc,
                                          (LPARAM)pInfo);
                    if (ret == IDOK)
                    {
                        AddTestToCombo(pInfo);
                    }

                    break;
                }
                case IDC_OPTIONS:
                    DialogBoxParamW(hInstance,
                                    MAKEINTRESOURCEW(IDD_OPTIONS),
                                    hDlg,
                                    (DLGPROC)OptionsDlgProc,
                                    (LPARAM)pInfo);
                    break;

                case IDC_RUN:
                    RunSelectedTest(pInfo);
                    break;

                case IDOK:
                    EndDialog(hDlg, 0);
                    break;
            }
        }
        break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            break;

        case WM_DESTROY:
             if (pInfo->hSmIcon)
                DestroyIcon(pInfo->hSmIcon);
            if (pInfo->hBgIcon)
                DestroyIcon(pInfo->hBgIcon);

            FreeTestCmdStrings(pInfo);

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
    HANDLE hThread;
    INT Ret = -1;

    UNREFERENCED_PARAMETER(hPrev);
    UNREFERENCED_PARAMETER(Cmd);
    UNREFERENCED_PARAMETER(iCmd); 

    hInstance = hInst;

    ZeroMemory(&iccx, sizeof(INITCOMMONCONTROLSEX));
    iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccx.dwICC = ICC_TAB_CLASSES;
    InitCommonControlsEx(&iccx);

    pInfo = HeapAlloc(GetProcessHeap(),
                      0,
                      sizeof(MAIN_WND_INFO));
    if (pInfo)
    {
        Ret = (DialogBoxParamW(hInstance,
                               MAKEINTRESOURCEW(IDD_WINETESTGUI),
                               NULL,
                               (DLGPROC)MainDlgProc,
                               (LPARAM)pInfo) == IDOK);

        HeapFree(GetProcessHeap(),
                 0,
                 pInfo);
    }

    return Ret;
}
