/*
 * PROJECT:     ReactOS Disk Cleanup
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */

#include "precomp.h"

UINT CleanmgrWindowMsg;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nCmdShow)
{
    HANDLE hMutex = 0;
    INITCOMMONCONTROLSEX InitControls;
    INT_PTR DialogButtonSelect;
    int nArgs = 0;
    LPWSTR* ArgList = NULL;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);
    
    /* Registering a window message for cleanmgr */
    CleanmgrWindowMsg = RegisterWindowMessageW(L"CleanmgrCreated");
    if (CleanmgrWindowMsg == 0)
    {
        DPRINT("RegisterWindowMessageW(): Failed to register a window message!\n");
    }

    InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitControls.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
    if (!InitCommonControlsEx(&InitControls))
    {
        DPRINT("InitCommonControlsEx(): Failed to register control classes!\n");
        return FALSE;
    }

    /* Creating a mutex of this specific program to check if multiple instances are running */
    hMutex = CreateMutexW(NULL, FALSE, L"cleanmgr.exe");
    if (hMutex)
    {
        /* If there is another instance of the program running, then just broadcast the 
           registered window message to all of the valid dialog boxes. */
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            DPRINT("An instance already exists!\n");
            SendMessageW(HWND_BROADCAST, CleanmgrWindowMsg, 0, 0);
            CloseHandle(hMutex);
            return TRUE;
        }
    }
    else
    {
        CloseHandle(hMutex);
    }

    /* Gather the program arguments which has been provided by the user */
    ArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (ArgList == NULL)
    {
        return FALSE;
    }
    else if (nArgs > 1)
    {
        if (UseAcquiredArguments(ArgList, nArgs))
        {
            return TRUE;
        }
    }
    /* If no arguments or invalid arguments have been provided by the user then just spawn the IDD_START dialog box. */
    DialogButtonSelect = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_START), NULL, StartDlgProc, 0);
    if (DialogButtonSelect == IDCANCEL)
    {
        return TRUE;
    }

    /* Spawn the IDD_PROGRESS_SCAN dialog box for scanning. */
    DialogButtonSelect = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_PROGRESS_SCAN), NULL, ProgressDlgProc, 0);
    if (DialogButtonSelect == IDCANCEL)
    {
        return TRUE;
    }

    /* Spawn the IDD_TAB_PARENT dialog box for selection. Setting lParam to false to tell the dialog box to start with regular procedure */
    DialogButtonSelect = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_TAB_PARENT), NULL, TabParentDlgProc, FALSE);
    if (DialogButtonSelect == IDCANCEL)
    {
        return TRUE;
    }

    /* Finally spawn the IDD_PROGRESS_DELETION dialog box for required folder deletion. */
    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_PROGRESS_DELETION), NULL, ProgressEndDlgProc, 0);

    /* Close the handle of the mutex */
    if (hMutex)
    {
        CloseHandle(hMutex);
    }
    return TRUE;
}
