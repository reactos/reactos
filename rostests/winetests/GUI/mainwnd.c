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

DWORD WINAPI
PipeReadThread(LPVOID lpParam)
{
    PMAIN_WND_INFO pInfo;
    HWND hList, hEdit;
    DWORD dwRead;
    CHAR chBuf[BUFSIZE];
    BOOL bSuccess = FALSE;
    LVITEMA item;
    INT count;

    pInfo = (PMAIN_WND_INFO)lpParam;

    hList = GetDlgItem(pInfo->hMainWnd, IDC_LIST);
    hEdit = GetDlgItem(pInfo->hMainWnd, IDC_OUTPUT);

    ZeroMemory(&item, sizeof(LVITEMA));
    item.mask = LVIF_TEXT;

    while (TRUE)
    {
        dwRead = 0;
        bSuccess = ReadFile(pInfo->hStdOutRd,
                            chBuf,
                            BUFSIZE,
                            &dwRead,
                            NULL);
        if(!bSuccess || dwRead == 0)
            break;

        chBuf[dwRead] = 0;

        count = GetWindowTextLengthA(hEdit);
        SendMessageA(hEdit, EM_SETSEL, (WPARAM)count, (LPARAM)count);
        SendMessageA(hEdit, EM_REPLACESEL, 0, (LPARAM)chBuf);

        //item.iItem = ListView_GetItemCount(hList);
        //item.pszText = chBuf;
        //SendMessage(hEdit, LVM_INSERTITEMA, 0, (LPARAM)&item);
    }

    return 0;
}


DWORD WINAPI
CreateClientProcess(PMAIN_WND_INFO pInfo)
{
    SECURITY_ATTRIBUTES sa;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL bSuccess = FALSE;

    //
    // Set up the security attributes
    //
    sa.nLength= sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    //
    // Create a pipe for the child process's STDOUT
    //
    if (!CreatePipe(&pInfo->hStdOutRd,
                    &pInfo->hStdOutWr,
                    &sa,
                    0))
    {
        return FALSE;
    }

    //
    // Ensure the read handle to the pipe for STDOUT is not inherited
    //
    if (!SetHandleInformation(pInfo->hStdOutRd,
                              HANDLE_FLAG_INHERIT,
                              0))
    {
        return FALSE;
    }

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = pInfo->hStdOutWr;
    si.hStdOutput = pInfo->hStdOutWr;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    bSuccess = CreateProcessW(pInfo->lpCmdLine,
                              NULL,
                              NULL,
                              NULL,
                              TRUE,
                              0,//CREATE_SUSPENDED,
                              NULL,
                              NULL,
                              &si,
                              &pi);
    if (bSuccess)
    {
        //
        // Create thread to handle pipe input from child processes
        //
        pInfo->hPipeThread = CreateThread(NULL,
                                          0,
                                          PipeReadThread,
                                          pInfo,
                                          0,
                                          NULL);

        WaitForSingleObject(pi.hProcess, INFINITE);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return bSuccess;
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
            pInfo->lpCmdLine = (LPWSTR)SendMessage(hRunCmd,
                                                   CB_GETITEMDATA,
                                                   0,
                                                   0);
            if (pInfo->lpCmdLine)
            {
                //
                // Create a new thread to create the client process
                // and recieve any ouput via stdout
                //
                CreateThread(NULL,
                             0,
                             CreateClientProcess,
                             pInfo,
                             0,
                             NULL);
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
                     (LPARAM)pInfo->SelectedTest.szName);

        len = (wcslen(pInfo->SelectedTest.szRunCmd) + 1) * sizeof(WCHAR);
        lpExePath = HeapAlloc(GetProcessHeap(), 0, len);
        if (lpExePath)
        {
            wcsncpy(lpExePath,
                    pInfo->SelectedTest.szRunCmd,
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
