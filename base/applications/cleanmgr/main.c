/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Main file
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */
 
#include "resource.h"
#include "precomp.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nCmdShow)
{
    INITCOMMONCONTROLSEX InitControls;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);
    
    HANDLE Obj;

    INT_PTR DialogButtonSelect;
    
    WCHAR TempText[MAX_PATH] = { 0 };

    LPWSTR* ArgList = NULL;
    int nArgs = 0;

    InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitControls.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
    if (!InitCommonControlsEx(&InitControls))
    {
        MessageBoxW(NULL, L"InitCommonControlsEx() failed!", L"Error", MB_OK | MB_ICONSTOP);
        return FALSE;
    }   
    dv.hInst = hInstance;
    
    Obj = CreateMutexW(NULL, FALSE, L"cleanmgr.exe");

    if (Obj)
    {
        DWORD err = GetLastError();

        if (err == ERROR_ALREADY_EXISTS)
        {
            LoadStringW(hInstance, IDS_ERROR_RUNNING, TempText, _countof(TempText));
            MessageBoxW(NULL, TempText, L"Error", MB_OK | MB_ICONSTOP);
            CloseHandle(Obj);
            return TRUE;
        }
    }

    ArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);

    if (ArgList == NULL)
    {
        return FALSE;
    }

    else if (nArgs > 1)
    {
        if(!ArgCheck(ArgList, nArgs))
        {
            return FALSE;
        }
    }

    else
    {
        DialogButtonSelect = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_START), NULL, StartDlgProc, 0);

        if (DialogButtonSelect == IDCANCEL)
        {
            return TRUE;
        }
    }
    
    DialogButtonSelect = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_PROGRESS), NULL, ProgressDlgProc, 0);

    if(DialogButtonSelect == IDCANCEL)
    {
        return TRUE;
    }

    DialogButtonSelect = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_CHOICE), NULL, ChoiceDlgProc, 0);

    if(DialogButtonSelect == IDCANCEL)
    {
        return TRUE;
    }

    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_PROGRESS_END), NULL, ProgressEndDlgProc, 0);
    
    if(Obj)
    {
        CloseHandle(Obj);
    }

    return TRUE;
}
