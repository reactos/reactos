/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Main file
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */

#include "precomp.h"

UINT CleanmgrWindowMsg;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nCmdShow)
{
    HANDLE Obj = CreateMutexW(NULL, FALSE, L"cleanmgr.exe");;
    INITCOMMONCONTROLSEX InitControls;
    INT_PTR DialogButtonSelect;
    int nArgs = 0;
    LPWSTR* ArgList = NULL;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

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

    if (Obj)
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            DPRINT("An instance already exists!\n");
            SendMessageW(HWND_BROADCAST, CleanmgrWindowMsg, 0, 0);
            CloseHandle(Obj);
            return TRUE;
        }
    }
    else
    {
        CloseHandle(Obj);
    }

    ArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);

    if (ArgList == NULL)
    {
        return FALSE;
    }
    else if (nArgs > 1)
    {
        if(!UseAquiredArguments(ArgList, nArgs))
        {
            return FALSE;
        }
    }
 
    DialogButtonSelect = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_START), NULL, StartDlgProc, 0);
    if (DialogButtonSelect == IDCANCEL)
    {
        return TRUE;
    }

    DialogButtonSelect = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_PROGRESS), NULL, ProgressDlgProc, 0);
    if (DialogButtonSelect == IDCANCEL)
    {
        return TRUE;
    }

    DialogButtonSelect = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_CHOICE), NULL, ChoiceDlgProc, 0);
    if (DialogButtonSelect == IDCANCEL)
    {
        return TRUE;
    }

    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_PROGRESS_END), NULL, ProgressEndDlgProc, 0);
    return TRUE;
}
