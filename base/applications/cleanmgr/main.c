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
    HANDLE Obj;
    INITCOMMONCONTROLSEX InitControls;
    INT_PTR DialogButtonSelect;
    int nArgs = 0;
    LPWSTR* ArgList = NULL;
    
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitControls.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
    if (!InitCommonControlsEx(&InitControls))
    {
        MessageBoxW(NULL, L"InitCommonControlsEx() failed!", L"Error", MB_OK | MB_ICONSTOP);
        return FALSE;
    }
    
    Obj = CreateMutexW(NULL, FALSE, L"cleanmgr.exe");

    if (Obj)
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            HWND hCleanMgr = NULL;
            WCHAR LogicalDrives[ARR_MAX_SIZE] = { 0 };
            WCHAR TempText[ARR_MAX_SIZE] = { 0 };
            
            DWORD NumOfDrives = GetLogicalDriveStringsW(_countof(LogicalDrives), LogicalDrives);
            if (NumOfDrives == 0)
            {
                MessageBoxW(NULL, L"GetLogicalDriveStringsW() failed!", L"Error", MB_OK | MB_ICONERROR);
                return FALSE;
            }
            
            LoadStringW(hInstance, IDS_START_DLG_TITLE, TempText, _countof(TempText));
            hCleanMgr = FindWindowW(NULL, TempText);
            
            if (hCleanMgr == NULL)
            {
                LoadStringW(hInstance, IDS_PROGRESS_DLG_TITLE, TempText, _countof(TempText));
                hCleanMgr = FindWindowW(NULL, TempText);
            }
            
            if (hCleanMgr == NULL)
            {
                if (NumOfDrives <= _countof(LogicalDrives))
                {
                    WCHAR* SingleDrive = LogicalDrives;
                    WCHAR RealDrive[ARR_MAX_SIZE] = { 0 };
                    WCHAR DlgTitle[ARR_MAX_SIZE] = { 0 };
                    while (*SingleDrive)
                    {
                        if (GetDriveTypeW(SingleDrive) == ONLY_PHYSICAL_DRIVE)
                        {
                            StringCchCopyW(RealDrive, _countof(RealDrive), SingleDrive);
                            RealDrive[wcslen(RealDrive) - 1] = '\0';
                            LoadStringW(hInstance, IDS_CHOICE_DLG_TITLE, TempText, _countof(TempText));
                            StringCchPrintfW(DlgTitle, _countof(DlgTitle), TempText, RealDrive);
                            hCleanMgr = FindWindowW(NULL, DlgTitle);
                            
                            if (hCleanMgr != NULL)
                            {
                                break;
                            }
                        }
                        SingleDrive += wcslen(SingleDrive) + 1;
                    }
                }
            }
            
            SendMessageW(hCleanMgr, WM_SYSCOMMAND, SC_RESTORE, 0);
            SetForegroundWindow(hCleanMgr);
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
    
    if (Obj)
    {
        CloseHandle(Obj);
    }

    return TRUE;
}
